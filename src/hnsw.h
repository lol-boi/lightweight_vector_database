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

namespace hnsw {

// Calculates the Squared L2 Euclidean distance between two vectors.
float squared_l2_distance(const std::vector<float>& a, const std::vector<float>& b) {
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
    void add_vector(const std::vector<float>& vec) {
        vectors.push_back(vec);
    }

    const std::vector<float>& get_vector(size_t index) const {
        return vectors[index];
    }

    size_t size() const {
        return vectors.size();
    }

private:
    std::vector<std::vector<float>> vectors;
};

class HNSW {
public:
    // M: maximum number of outgoing connections in the graph
    // efConstruction: size of the dynamic list for the nearest neighbors search during construction
    // efSearch: size of the dynamic list for the nearest neighbors search during query time
    HNSW(int M = 5, int efConstruction = 10, int efSearch = 10) 
        : entry_point_id(-1), 
          M(M), 
          efConstruction(efConstruction), 
          efSearch(efSearch),
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
    // Output: A vector of node IDs representing the top-ef closest nodes found in this layer.
    std::vector<int> search_layer(const std::vector<float>& query, int entry_point_id, int ef, int layer_level) {
        // Pair: {distance, node_id}
        using Candidate = std::pair<float, int>;

        // Min-heap for candidates to explore (closest first)
        std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> candidate_queue;
        // Max-heap for results (furthest first, so smallest is at top)
        std::priority_queue<Candidate> result_queue;
        // Set to keep track of visited nodes
        std::set<int> visited_nodes;

        // Start with the entry point
        float dist_to_entry = squared_l2_distance(query, vector_storage.get_vector(entry_point_id));
        candidate_queue.push({dist_to_entry, entry_point_id});
        result_queue.push({dist_to_entry, entry_point_id});
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
                    float dist_to_neighbor = squared_l2_distance(query, vector_storage.get_vector(neighbor_id));

                    if (result_queue.size() < ef || dist_to_neighbor < result_queue.top().first) {
                        candidate_queue.push({dist_to_neighbor, neighbor_id});
                        result_queue.push({dist_to_neighbor, neighbor_id});

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

    void insert(const std::vector<float>& vec) {
        uint32_t new_node_id = vector_storage.size();
        vector_storage.add_vector(vec);

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
                        float dist = squared_l2_distance(neighbor_vec, vector_storage.get_vector(current_connected_neighbor_id));
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
    // Output: A vector of node IDs representing the k nearest neighbors.
    std::vector<int> k_nearest_neighbors(const std::vector<float>& query, int k) {
        if (entry_point_id == -1) {
            return {}; // No nodes in the graph
        }

        int current_node_id = entry_point_id;
        int current_max_layer = nodes[current_node_id].max_layer;

        // Phase 1: Descend from top layer to Layer 1 to find the entry point for Layer 0
        for (int layer = current_max_layer; layer > 0; --layer) {
            std::vector<int> candidates = search_layer(query, current_node_id, 1, layer);
            current_node_id = candidates[0];
        }

        // Phase 2: Perform search at Layer 0 with efSearch
        std::vector<int> results = search_layer(query, current_node_id, std::max(k, efSearch), 0);

        // Return the top k results
        if (results.size() > k) {
            results.resize(k);
        }
        return results;
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

    public: // Temporarily make these public for testing
        VectorStorage vector_storage;
        std::vector<Node> nodes;
    private:    int entry_point_id; // The ID of the node in the highest layer
    int M; // Max number of outgoing connections
    int efConstruction; // Size of dynamic list for nearest neighbors search during construction
    int efSearch; // Size of dynamic list for nearest neighbors search during query time
    double m_L;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;

    int random_level() {
        return static_cast<int>(floor(-log(dist(gen)) * m_L));
    }
};

} // namespace hnsw

#endif // HNSW_H
