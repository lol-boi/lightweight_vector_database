#ifndef HNSW_H
#define HNSW_H

#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <queue>    // For priority_queue
#include <set>      // For visited set
#include <random>  // For std::mt19937 and std::uniform_real_distribution
#include <algorithm> // For std::sort
#include <numeric>   // For std::inner_product
#include <map>       // For std::map
#include <functional> // For std::function

namespace hnsw {

// Enum for supported distance metrics
enum class DistanceMetric {
    L2,
    COSINE,
    IP // Inner Product
};

// Type alias for metadata
using Metadata = std::map<std::string, std::string>;

// Type alias for a filter function
using FilterFunc = std::function<bool(const Metadata&)>;

// Enum to control what data is returned in the search result
enum class Include {
    ID,
    DISTANCE,
    METADATA,
    VECTOR
};

// Struct for search results
struct QueryResult {
    int id;
    float distance;
    Metadata metadata;
    std::vector<float> vector;
};

// Represents a node in the HNSW graph.
struct Node {
    uint32_t id;
    int max_layer;
    std::vector<std::vector<int>> neighbors;

    Node(uint32_t id, int max_layer) : id(id), max_layer(max_layer) {
        neighbors.resize(max_layer + 1);
    }
};

// A simple container for vector data.
class VectorStorage {
public:
    VectorStorage(size_t vector_dimension) : vector_dimension_(vector_dimension) {}

    void add_vector(const std::vector<float>& vec, const Metadata& meta) {
        if (vec.size() != vector_dimension_) {
            throw std::invalid_argument("Vector dimension mismatch.");
        }
        vectors.push_back(vec);
        metadata.push_back(meta);
    }

    const std::vector<float>& get_vector(size_t index) const {
        return vectors[index];
    }

    const Metadata& get_metadata(size_t index) const {
        return metadata[index];
    }

    size_t size() const {
        return vectors.size();
    }

    size_t get_vector_dimension() const {
        return vector_dimension_;
    }

private:
    size_t vector_dimension_;
    std::vector<std::vector<float>> vectors;
    std::vector<Metadata> metadata;
};

class HNSW {
public:
    // M: maximum number of outgoing connections in the graph
    // efConstruction: size of the dynamic list for the nearest neighbors search during construction
    // efSearch: size of the dynamic list for the nearest neighbors search during query time
    // metric: The distance metric to use (L2, COSINE, IP)
    HNSW(size_t vector_dimension, int M = 5, int efConstruction = 10, int efSearch = 10, DistanceMetric metric = DistanceMetric::L2)
        : vector_storage(vector_dimension),
          entry_point_id(-1),
          M(M),
          efConstruction(efConstruction),
          efSearch(efSearch),
          distance_metric(metric),
          gen(std::random_device{}()),
          dist(0.0, 1.0) {
        m_L = 1.0 / log(1.0 * M);
    }

    HNSW(size_t vector_dimension, int M, int efConstruction, int efSearch, DistanceMetric metric, const std::vector<Node>& nodes, const VectorStorage& vector_storage)
        : vector_storage(vector_storage),
          nodes(nodes),
          entry_point_id(nodes.empty() ? -1 : nodes.back().id),
          M(M),
          efConstruction(efConstruction),
          efSearch(efSearch),
          distance_metric(metric),
          gen(std::random_device{}()),
          dist(0.0, 1.0) {
        m_L = 1.0 / log(1.0 * M);
    }

    // Implements the greedy search algorithm within a single layer.
    // Input:
    //   query: The query vector.
    //   entry_point_id: The ID of the starting node in the current layer.
    //   ef: The size of the candidate pool.
    //   layer_level: The specific layer to search.
    //   filter: An optional function to filter results based on metadata.
    // Output: A vector of node IDs representing the top-ef closest nodes found in this layer.
    std::vector<int> search_layer(const std::vector<float>& query, int entry_point_id, int ef, int layer_level, const FilterFunc& filter = nullptr) {
        // Pair: {distance, node_id}
        using Candidate = std::pair<float, int>;

        // Min-heap for candidates to explore (closest first)
        std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> candidate_queue;
        // Max-heap for results (furthest first, so smallest is at top)
        std::priority_queue<Candidate> result_queue;
        // Set to keep track of visited nodes
        std::set<int> visited_nodes;

        // Start with the entry point
        float dist_to_entry = calculate_distance(query, vector_storage.get_vector(entry_point_id));
        candidate_queue.push({dist_to_entry, entry_point_id});
        if (!filter || filter(vector_storage.get_metadata(entry_point_id))) {
            result_queue.push({dist_to_entry, entry_point_id});
        }
        visited_nodes.insert(entry_point_id);

        while (!candidate_queue.empty()) {
            Candidate current = candidate_queue.top();
            candidate_queue.pop();

            float current_dist = current.first;
            int current_node_id = current.second;

            // Optimization: If the current candidate is further than the worst result
            // and we already have 'ef' results, we can stop.
            if (result_queue.size() == ef && current_dist > result_queue.top().first) {
                break;
            }

            // Explore neighbors of the current node in the specified layer
            for (int neighbor_id : nodes[current_node_id].neighbors[layer_level]) {
                if (visited_nodes.find(neighbor_id) == visited_nodes.end()) {
                    visited_nodes.insert(neighbor_id);

                    float dist_to_neighbor = calculate_distance(query, vector_storage.get_vector(neighbor_id));
                    if (result_queue.size() < ef || dist_to_neighbor < result_queue.top().first) {
                        candidate_queue.push({dist_to_neighbor, neighbor_id});
                        if (!filter || filter(vector_storage.get_metadata(neighbor_id))) {
                            result_queue.push({dist_to_neighbor, neighbor_id});
                        }

                        // Maintain result_queue size
                        while (result_queue.size() > ef) {
                            result_queue.pop();
                        }
                    }
                }
            }
        }

        // Extract node IDs from the result_queue
        std::vector<int> final_results;
        while (!result_queue.empty()) {
            final_results.push_back(result_queue.top().second);
            result_queue.pop();
        }
        // The result_queue is a max-heap, so the elements are popped in descending order of distance.
        // We want the closest ones, so reverse the order.
        std::reverse(final_results.begin(), final_results.end());
        return final_results;
    }

    void insert(const std::vector<float>& vec, const Metadata& meta = {}) {
        uint32_t new_node_id = vector_storage.size();
        vector_storage.add_vector(vec, meta);

        int new_node_layer = random_level();
        nodes.emplace_back(new_node_id, new_node_layer);

        if (entry_point_id == -1) { // First node
            entry_point_id = new_node_id;
            return;
        }

        int current_node_id = entry_point_id;
        int current_max_layer = nodes[current_node_id].max_layer;

        // Phase 1: Find entry point for new node's layer
        for (int layer = current_max_layer; layer > new_node_layer; --layer) {
            std::vector<int> candidates = search_layer(vec, current_node_id, 1, layer);
            current_node_id = candidates[0];
        }

        // Phase 2: Insert node into layers from new_node_layer down to 0
        for (int layer = std::min(new_node_layer, current_max_layer); layer >= 0; --layer) {
            std::vector<int> neighbors_found = search_layer(vec, current_node_id, efConstruction, layer);

            // Connect new_node_id to M nearest neighbors
            std::vector<int> new_node_neighbors;
            for (int neighbor_id : neighbors_found) {
                if (new_node_neighbors.size() < M) {
                    new_node_neighbors.push_back(neighbor_id);
                } else {
                    break;
                }
            }

            for (int neighbor_id : new_node_neighbors) {
                nodes[new_node_id].neighbors[layer].push_back(neighbor_id);
                nodes[neighbor_id].neighbors[layer].push_back(new_node_id); // Bidirectional

                // Pruning neighbors if necessary
                if (nodes[neighbor_id].neighbors[layer].size() > M) {
                    // Find the furthest neighbor and remove it
                    float max_dist = -1.0f;
                    int furthest_neighbor_idx = -1;
                    const std::vector<float>& neighbor_vec = vector_storage.get_vector(neighbor_id);

                    for (size_t i = 0; i < nodes[neighbor_id].neighbors[layer].size(); ++i) {
                        int current_connected_neighbor_id = nodes[neighbor_id].neighbors[layer][i];
                        float dist = calculate_distance(neighbor_vec, vector_storage.get_vector(current_connected_neighbor_id));
                        if (dist > max_dist) {
                            max_dist = dist;
                            furthest_neighbor_idx = i;
                        }
                    }
                    if (furthest_neighbor_idx != -1) {
                        nodes[neighbor_id].neighbors[layer].erase(nodes[neighbor_id].neighbors[layer].begin() + furthest_neighbor_idx);
                    }
                }
            }
            current_node_id = neighbors_found[0];
        }

        // Update entry point if new node is in a higher layer
        if (new_node_layer > nodes[entry_point_id].max_layer) {
            entry_point_id = new_node_id;
        }
    }

    // Performs a k-nearest neighbors search.
    // Input:
    //   query: The query vector.
    //   k: The number of nearest neighbors to return.
    //   filter: An optional function to filter results based on metadata.
    //   include: A set of data to include in the results.
    // Output: A vector of QueryResult structs.
    std::vector<QueryResult> k_nearest_neighbors(const std::vector<float>& query, int k, const FilterFunc& filter = nullptr, const std::set<Include>& include = {Include::ID}) {
        if (entry_point_id == -1) {
            return {}; // No nodes in the graph
        }

        int current_node_id = entry_point_id;
        int current_max_layer = nodes[current_node_id].max_layer;

        // Phase 1: Descend from top layer to Layer 1 to find the entry point for Layer 0
        for (int layer = current_max_layer; layer > 0; --layer) {
            std::vector<int> candidates = search_layer(query, current_node_id, 1, layer, filter);
            if (!candidates.empty()) {
                current_node_id = candidates[0];
            }
        }

        // Phase 2: Perform search at Layer 0 with efSearch
        std::vector<int> results_ids = search_layer(query, current_node_id, std::max(k, efSearch), 0, filter);

        // Phase 3: Construct QueryResult objects
        std::vector<QueryResult> final_results;
        for (int id : results_ids) {
            if (final_results.size() >= k) {
                break;
            }
            QueryResult result;
            result.id = id;
            if (include.count(Include::DISTANCE)) {
                result.distance = calculate_distance(query, vector_storage.get_vector(id));
            }
            if (include.count(Include::METADATA)) {
                result.metadata = vector_storage.get_metadata(id);
            }
            if (include.count(Include::VECTOR)) {
                result.vector = vector_storage.get_vector(id);
            }
            final_results.push_back(result);
        }

        return final_results;
    }

    size_t size() const {
        return nodes.size();
    }

    const std::vector<Node>& get_nodes() const {
        return nodes;
    }

    int get_entry_point() const {
        return entry_point_id;
    }

    int get_M() const {
        return M;
    }

    int get_efConstruction() const {
        return efConstruction;
    }

    int get_efSearch() const {
        return efSearch;
    }

    DistanceMetric get_distance_metric() const {
        return distance_metric;
    }

    const VectorStorage& get_vector_storage() const {
        return vector_storage;
    }

private:
    VectorStorage vector_storage;
    std::vector<Node> nodes;
    int entry_point_id; // The ID of the node in the highest layer
    int M; // Max number of outgoing connections
    int efConstruction; // Size of dynamic list for nearest neighbors search during construction
    int efSearch; // Size of dynamic list for nearest neighbors search during query time
    DistanceMetric distance_metric; // The chosen distance metric
    double m_L;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;

    int random_level() {
        return static_cast<int>(floor(-log(dist(gen)) * m_L));
    }
    // Private helper functions for distance calculations
    float calculate_l2_distance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have the same dimension.");
        }
        float distance = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            distance += diff * diff;
        }
        return distance;
    }

    float calculate_cosine_distance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have the same dimension.");
        }
        float dot_product = std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
        float norm_a = std::sqrt(std::inner_product(a.begin(), a.end(), a.begin(), 0.0f));
        float norm_b = std::sqrt(std::inner_product(b.begin(), b.end(), b.begin(), 0.0f));

        if (norm_a == 0.0f || norm_b == 0.0f) {
            return 1.0f; // Or handle as an error, or return 0.0f for identical zero vectors
        }
        return 1.0f - (dot_product / (norm_a * norm_b)); // 1 - cosine similarity
    }

    float calculate_inner_product_distance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have the same dimension.");
        }
        return -std::inner_product(a.begin(), a.end(), b.begin(), 0.0f); // Negative to use with min-heap for max IP
    }

    float calculate_distance(const std::vector<float>& a, const std::vector<float>& b) const {
        switch (distance_metric) {
            case DistanceMetric::L2:
                return calculate_l2_distance(a, b);
            case DistanceMetric::COSINE:
                return calculate_cosine_distance(a, b);
            case DistanceMetric::IP:
                return calculate_inner_product_distance(a, b);
            default:
                throw std::runtime_error("Unknown distance metric.");
        }
    }
};

} // namespace hnsw

#endif // HNSW_H
