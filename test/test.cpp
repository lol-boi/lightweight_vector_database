#include <iostream>
#include <vector>
#include <cassert>
#include <cmath> // For std::abs
#include "hnsw.h"

// Helper to compare floats with tolerance
bool float_equals(float a, float b, float epsilon = 1e-6) {
    return std::abs(a - b) < epsilon;
}

void test_l2_distance_hnsw() {
    hnsw::HNSW hnsw_graph(2, 5, 5, hnsw::DistanceMetric::L2);

    hnsw_graph.insert({0.0f, 0.0f}); // Node 0
    hnsw_graph.insert({1.0f, 0.0f}); // Node 1
    hnsw_graph.insert({0.0f, 1.0f}); // Node 2

    std::vector<int> results = hnsw_graph.k_nearest_neighbors({0.1f, 0.1f}, 1);
    assert(results.size() == 1);
    assert(results[0] == 0); // (0,0) is closest to (0.1,0.1)

    std::cout << "test_l2_distance_hnsw passed." << std::endl;
}

void test_cosine_distance_hnsw() {
    hnsw::HNSW hnsw_graph(2, 5, 5, hnsw::DistanceMetric::COSINE);

    hnsw_graph.insert({1.0f, 0.0f}); // Node 0 (angle 0)
    hnsw_graph.insert({0.0f, 1.0f}); // Node 1 (angle 90)
    hnsw_graph.insert({1.0f, 1.0f}); // Node 2 (angle 45)
    hnsw_graph.insert({-1.0f, 0.0f});// Node 3 (angle 180)

    // Query vector (1, 0.1) - very close to (1,0)
    std::vector<int> results = hnsw_graph.k_nearest_neighbors({1.0f, 0.1f}, 1);
    assert(results.size() == 1);
    assert(results[0] == 0);

    // Query vector (0.1, 1) - very close to (0,1)
    results = hnsw_graph.k_nearest_neighbors({0.1f, 1.0f}, 1);
    assert(results.size() == 1);
    assert(results[0] == 1);

    // Query vector (1,1) - should be closest to Node 2
    results = hnsw_graph.k_nearest_neighbors({1.0f, 1.0f}, 1);
    assert(results.size() == 1);
    assert(results[0] == 2);

    std::cout << "test_cosine_distance_hnsw passed." << std::endl;
}

void test_inner_product_distance_hnsw() {
    hnsw::HNSW hnsw_graph(2, 5, 5, hnsw::DistanceMetric::IP);

    hnsw_graph.insert({1.0f, 1.0f}); // Node 0 (IP with (1,1) = 2)
    hnsw_graph.insert({1.0f, 0.0f}); // Node 1 (IP with (1,1) = 1)
    hnsw_graph.insert({-1.0f, -1.0f});// Node 2 (IP with (1,1) = -2)

    // Query vector (1,1). We want max inner product, which means min negative inner product.
    std::vector<int> results = hnsw_graph.k_nearest_neighbors({1.0f, 1.0f}, 1);
    assert(results.size() == 1);
    assert(results[0] == 0); // Node 0 has highest IP (2)

    std::cout << "test_inner_product_distance_hnsw passed." << std::endl;
}

void test_node_structure() {
    hnsw::Node node(10, 3); // ID 10, max_layer 3
    assert(node.id == 10);
    assert(node.max_layer == 3);
    assert(node.neighbors.size() == 4); // Layers 0, 1, 2, 3

    std::cout << "test_node_structure passed." << std::endl;
}

void test_vector_storage() {
    hnsw::VectorStorage storage;
    std::vector<float> v1 = {1.0f, 2.0f};
    std::vector<float> v2 = {3.0f, 4.0f};

    storage.add_vector(v1);
    storage.add_vector(v2);

    assert(storage.size() == 2);
    assert(storage.get_vector(0) == v1);
    assert(storage.get_vector(1) == v2);

    std::cout << "test_vector_storage passed." << std::endl;
}

void test_search_layer() {
    hnsw::HNSW hnsw_graph; // Default L2 metric

    // Add vectors
    hnsw_graph.vector_storage.add_vector({0.0f, 0.0f}); // Node 0
    hnsw_graph.vector_storage.add_vector({1.0f, 1.0f}); // Node 1
    hnsw_graph.vector_storage.add_vector({0.1f, 0.1f}); // Node 2 (closer to 0)
    hnsw_graph.vector_storage.add_vector({5.0f, 5.0f}); // Node 3 (far from 0)
    hnsw_graph.vector_storage.add_vector({0.2f, 0.2f}); // Node 4 (closer to 0)

    // Add nodes
    hnsw_graph.nodes.emplace_back(0, 0); // Node 0, layer 0
    hnsw_graph.nodes.emplace_back(1, 0); // Node 1, layer 0
    hnsw_graph.nodes.emplace_back(2, 0); // Node 2, layer 0
    hnsw_graph.nodes.emplace_back(3, 0); // Node 3, layer 0
    hnsw_graph.nodes.emplace_back(4, 0); // Node 4, layer 0

    // Manually set up neighbors for layer 0
    // Node 0 neighbors: 1, 2, 4
    hnsw_graph.nodes[0].neighbors[0].push_back(1);
    hnsw_graph.nodes[0].neighbors[0].push_back(2);
    hnsw_graph.nodes[0].neighbors[0].push_back(4);

    // Node 1 neighbors: 0
    hnsw_graph.nodes[1].neighbors[0].push_back(0);

    // Node 2 neighbors: 0
    hnsw_graph.nodes[2].neighbors[0].push_back(0);

    // Node 4 neighbors: 0
    hnsw_graph.nodes[4].neighbors[0].push_back(0);


    // Query vector
    std::vector<float> query = {0.05f, 0.05f}; // Very close to Node 0, 2, 4

    // Test 1: ef = 1, entry_point = Node 0
    std::vector<int> results1 = hnsw_graph.search_layer(query, 0, 1, 0);
    assert(results1.size() == 1);
    // The closest should be Node 0 (0.0,0.0), Node 2 (0.1,0.1), Node 4 (0.2,0.2)
    // The exact order might depend on tie-breaking, but Node 0 is the entry point and very close.
    // Let's assume Node 0 is returned for ef=1 due to being the entry point and very close.
    // More robust testing would involve checking distances.
    // For now, let's check if one of the very close nodes is returned.
    bool found_close_node = false;
    if (results1[0] == 0 || results1[0] == 2 || results1[0] == 4) {
        found_close_node = true;
    }
    assert(found_close_node);


    // Test 2: ef = 3, entry_point = Node 0
    std::vector<int> results2 = hnsw_graph.search_layer(query, 0, 3, 0);
    assert(results2.size() == 3);
    // Expected closest nodes are 0, 2, 4. Order might vary.
    std::sort(results2.begin(), results2.end());
    assert(results2[0] == 0);
    assert(results2[1] == 2);
    assert(results2[2] == 4);

    std::cout << "test_search_layer passed." << std::endl;
}

void test_full_hnsw_insertion() {
    // Test with M=2, efConstruction=5
    hnsw::HNSW hnsw_graph(2, 5); // Default L2 metric

    // Insert a few vectors
    hnsw_graph.insert({0.0f, 0.0f}); // Node 0
    hnsw_graph.insert({1.0f, 1.0f}); // Node 1
    hnsw_graph.insert({0.1f, 0.1f}); // Node 2
    hnsw_graph.insert({10.0f, 10.0f});// Node 3
    hnsw_graph.insert({10.1f, 10.1f});// Node 4

    // Assertions to check the state of the graph.
    // These will be simple checks, as the exact structure is non-deterministic.
    assert(hnsw_graph.size() == 5);

    // Check that connections are within M for each layer of each node
    for (const auto& node : hnsw_graph.get_nodes()) {
        for (const auto& layer_neighbors : node.neighbors) {
            assert(layer_neighbors.size() <= 2); // M=2
        }
    }

    // Check that the entry point is the node with the highest layer
    int entry_point_id = hnsw_graph.get_entry_point();
    int max_layer = -1;
    int node_with_max_layer = -1;
    for(const auto& node : hnsw_graph.get_nodes()){
        if(node.max_layer > max_layer){
            max_layer = node.max_layer;
            node_with_max_layer = node.id;
        }
    }
    assert(entry_point_id == node_with_max_layer);

    std::cout << "test_full_hnsw_insertion passed." << std::endl;
}

void test_k_nearest_neighbors() {
    hnsw::HNSW hnsw_graph(2, 5, 5); // Default L2 metric

    // Insert some vectors
    hnsw_graph.insert({0.0f, 0.0f}); // Node 0
    hnsw_graph.insert({1.0f, 1.0f}); // Node 1
    hnsw_graph.insert({0.1f, 0.1f}); // Node 2
    hnsw_graph.insert({0.2f, 0.2f}); // Node 3
    hnsw_graph.insert({10.0f, 10.0f});// Node 4
    hnsw_graph.insert({10.1f, 10.1f});// Node 5

    // Query vector close to (0,0)
    std::vector<float> query = {0.05f, 0.05f};

    // Search for k=3 nearest neighbors
    std::vector<int> results = hnsw_graph.k_nearest_neighbors(query, 3);

    assert(results.size() == 3);

    // The expected closest nodes are 0, 2, 3. The order might vary, so sort for comparison.
    std::sort(results.begin(), results.end());
    assert(results[0] == 0);
    assert(results[1] == 2);
    assert(results[2] == 3);

    std::cout << "test_k_nearest_neighbors passed." << std::endl;
}

int main() {
    test_l2_distance_hnsw();
    test_cosine_distance_hnsw();
    test_inner_product_distance_hnsw();
    test_node_structure();
    test_vector_storage();
    test_search_layer();
    test_full_hnsw_insertion();
    test_k_nearest_neighbors();

    std::cout << "All tests passed!" << std::endl;

    return 0;
}

