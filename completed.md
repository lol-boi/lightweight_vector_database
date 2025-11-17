### Phase 1: Foundations

*   [x] **Data Structures**:
    *   [x] **Vector Storage**: `VectorStorage` class to hold vector data.
    *   [x] **Node Structure**: `Node` struct with ID, max layer, and adjacency lists.
*   [x] **Distance Metric**:
    *   [x] Implemented **Squared L2 Euclidean distance**.

### Phase 2: The Core Engine (Greedy Search)

*   [x] **`search_layer` function**: Implemented and tested the greedy search algorithm for a single layer.

### Phase 3: Single Layer (NSW)

*   [x] **Insertion Logic (Simplified)**: Implemented insertion for a single-layer graph (Navigable Small World), including bidirectional connections and pruning.

### Phase 4: The Hierarchy (HNSW)

*   [x] **Probabilistic Layer Assignment**: Implemented the random level generator function.
*   [x] **Full Insertion Algorithm**: Implemented the full HNSW insertion algorithm, including the "zoom-out" and "zoom-in" phases.
*   [x] **Dynamic Entry Point**: The global entry point is now updated correctly when a new node with a higher layer is inserted.

### Phase 5: The Final k-NN Search

*   [x] **`k_nearest_neighbors` function**: Implemented the multi-layer k-NN search algorithm, including descending through layers and performing the final search at Layer 0.

### Phase 6: Database Features

*   [x] **Configurable Distance Metrics**: Added support for L2, Cosine, and Inner Product distance metrics.
*   [x] **Metadata Filtering**: Implemented metadata filtering for queries.
*   [x] **Data Inclusion**: Implemented data inclusion options for queries.
*   [x] **Database Persistence**: Implemented database persistence with `save` and `load` methods.
*   [x] **Read-only Mode**: Implemented read-only mode for the database.
*   [x] **Sync Mode**: Implemented sync mode for write durability.
*   [x] **Cache Size**: Added a cache size parameter for future caching implementation.
