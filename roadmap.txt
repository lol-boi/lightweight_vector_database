### Phase 1: Foundations (✓ Completed)

*   [x] **Data Structures**:
    *   [x] **Vector Storage**: `VectorStorage` class to hold vector data.
    *   [x] **Node Structure**: `Node` struct with ID, max layer, and adjacency lists.
*   [x] **Distance Metric**:
    *   [x] Implemented **Squared L2 Euclidean distance**.

---

### Phase 2: The Core Engine (Greedy Search) (✓ Completed)

*   [x] **`search_layer` function**: Implemented and tested the greedy search algorithm for a single layer.

---

### Phase 3: Single Layer (NSW) (✓ Completed)

*   [x] **Insertion Logic (Simplified)**: Implemented insertion for a single-layer graph (Navigable Small World), including bidirectional connections and pruning.

---

### Phase 4: The Hierarchy (HNSW) (✓ Completed)

Now, add the layers.

1.  **Probabilistic Layer Assignment**:
    *   [x] Implement the random level generator function: `floor(-ln(uniform_random(0,1)) * m_L)`.
    *   [x] Most nodes will get level 0. A few will get level 1, even fewer level 2, etc.

2.  **Full Insertion Algorithm**:
    *   [x] Assign new node `Q` a max layer `L`.
    *   [x] Start at the current global `entry_point` (the node present in the highest layer).
    *   [x] **Phase 1 (Zoom-out)**: From the top layer down to `L+1`, use greedy search (with `ef=1`) just to find the single closest node in each layer to act as the entry point for the next layer down.
    *   [x] **Phase 2 (Zoom-in & Build)**: From layer `L` down to 0:
        *   [x] Use `search_layer` (with `efConstruction`) to find candidate neighbors.
        *   [x] Select the best `M` candidates.
        *   [x] Add edges bidirectionally.
        *   [x] Prune edges if any node exceeds `M_max` for that layer.
    *   [x] *Check*: If the new node `Q` has a higher max layer than the current global `entry_point`, update the global entry point to `Q`.

---

### Phase 5: The Final k-NN Search (✓ Completed)

1.  [x] **Implement `k_nearest_neighbors(query, k)`**:
    *   [x] Start at global `entry_point`.
    *   [x] Descend from top layer to Layer 1, finding the closest node in each layer (greedy search with `ef=1`).
    *   [x] At **Layer 0**, use the best node found in Layer 1 as the entry point. Run `search_layer` with `efSearch` (which should be >= `k`).
    *   [x] Return the top `k` results from that final search.

### Summary Checklist for Success
* [x] Reliable distance function.
* [x] Robust `search_layer` (this is 80% of the complex logic).
* [ ] Correctly managing neighbor lists for *different* layers (a node's neighbors in Layer 0 are different from its neighbors in Layer 1).
* [x] Handling edge pruning when `M` is exceeded.