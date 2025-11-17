# HNSW Implementation Documentation

This document provides a detailed explanation of the `hnsw.h` file, which contains the core implementation of the HNSW (Hierarchical Navigable Small World) algorithm for our lightweight vector database.

### 1. Overall Structure

The `hnsw.h` file is the core of our vector database. It contains the following key components:

*   **`hnsw` namespace:** All the code is wrapped in the `hnsw` namespace to avoid naming conflicts with other libraries.
*   **Enums and Type Aliases:**
    *   `DistanceMetric`: An enum that defines the supported distance metrics (L2, COSINE, IP).
    *   `Metadata`: A type alias for `std::map<std::string, std::string>`, used to store metadata for each vector.
    *   `FilterFunc`: A type alias for `std::function<bool(const Metadata&)>`, used to define a filter function for queries.
    *   `Include`: An enum that defines what data to include in the search results (ID, DISTANCE, METADATA, VECTOR).
    *   `QueryResult`: A struct that holds the results of a query.
*   **`Node` struct:** Represents a node in the HNSW graph. Each node has an ID, a maximum layer, and a list of neighbors for each layer.
*   **`VectorStorage` class:** A simple container for storing the vectors and their metadata.
*   **`HNSW` class:** The main class that implements the HNSW algorithm. It contains the logic for inserting vectors, building the graph, and performing k-NN searches.

### 2. `VectorStorage` Class

The `VectorStorage` class is a straightforward container for our vectors and their associated metadata.

*   **`VectorStorage(size_t vector_dimension)`:** The constructor takes the vector dimension as an argument and stores it.
*   **`add_vector(const std::vector<float>& vec, const Metadata& meta)`:** This method adds a new vector and its metadata to the storage. It also checks if the vector dimension matches the one defined in the constructor.
*   **`get_vector(size_t index)` and `get_metadata(size_t index)`:** These methods return the vector and metadata at a given index.
*   **`size()` and `get_vector_dimension()`:** These methods return the number of vectors in the storage and the vector dimension, respectively.

### 3. `Node` Struct

The `Node` struct represents a node in the HNSW graph.

*   **`id`:** The unique ID of the node.
*   **`max_layer`:** The maximum layer of the node in the graph.
*   **`neighbors`:** A vector of vectors, where `neighbors[i]` contains the list of neighbors of the node in layer `i`.

### 4. `HNSW` Class

The `HNSW` class is where the magic happens. It implements the HNSW algorithm for approximate nearest neighbor search.

#### 4.1. Constructor

*   **`HNSW(size_t vector_dimension, int M, int efConstruction, int efSearch, DistanceMetric metric)`:** The constructor initializes the HNSW graph with the given parameters:
    *   `vector_dimension`: The dimension of the vectors.
    *   `M`: The maximum number of outgoing connections for each node.
    *   `efConstruction`: The size of the dynamic list for the nearest neighbors search during construction.
    *   `efSearch`: The size of the dynamic list for the nearest neighbors search during query time.
    *   `metric`: The distance metric to use.

#### 4.2. `insert()` Method

The `insert()` method inserts a new vector into the HNSW graph. Here's a step-by-step explanation of how it works:

1.  **Get a new node ID:** The new node ID is simply the current size of the vector storage.
2.  **Add the vector to the storage:** The new vector and its metadata are added to the `VectorStorage`.
3.  **Get a random layer for the new node:** The `random_level()` method returns a random layer for the new node, based on a probability distribution. This is the "hierarchical" part of HNSW.
4.  **Create a new node:** A new `Node` is created with the new node ID and the random layer.
5.  **Find the entry point for the new node:** The algorithm starts from the top layer of the graph and greedily searches for the nearest neighbor of the new vector in each layer, until it reaches the layer of the new node.
6.  **Insert the new node into the graph:** The algorithm inserts the new node into the graph by connecting it to its `M` nearest neighbors in each layer, from the new node's layer down to layer 0.
7.  **Prune connections:** If a node has more than `M` connections in a layer, the algorithm prunes the connections by removing the furthest neighbor.
8.  **Update the entry point:** If the new node has a higher layer than the current entry point, the new node becomes the new entry point.

#### 4.3. `search_layer()` Method

The `search_layer()` method implements the greedy search algorithm within a single layer. It takes a query vector, an entry point ID, the size of the candidate pool (`ef`), and the layer level as input. It returns a vector of the `ef` closest nodes found in that layer.

#### 4.4. `k_nearest_neighbors()` Method

The `k_nearest_neighbors()` method performs a k-nearest neighbors search for a given query vector.

1.  **Find the entry point for the search:** The algorithm starts from the top layer of the graph and greedily searches for the nearest neighbor of the query vector in each layer, until it reaches layer 0.
2.  **Perform the final search:** The algorithm performs a greedy search in layer 0 with a candidate pool of size `efSearch`.
3.  **Return the `k` nearest neighbors:** The algorithm returns the `k` closest nodes from the candidate pool.

### 5. Advanced Features

Now, let's talk about the advanced features we've implemented:

*   **Configurable Distance Metrics:** The `DistanceMetric` enum and the `calculate_distance()` method allow you to choose between L2, Cosine, and Inner Product distance metrics.
*   **Metadata Filtering:** The `FilterFunc` type alias and the `filter` parameter in the `k_nearest_neighbors()` method allow you to filter the search results based on metadata.
*   **Data Inclusion:** The `Include` enum and the `include` parameter in the `k_nearest_neighbors()` method allow you to control what data is returned in the search results.

### What is the use of `VectorStorage`?

The `VectorStorage` class serves as a **centralized, contiguous, and indexed repository for all the actual vector data and their associated metadata.** Think of it as the "raw data" layer of our vector database.

Here's why it's crucial and how it works:

1.  **Separation of Concerns:**
    *   The HNSW graph (`HNSW` class) is primarily concerned with **graph topology** (how nodes are connected) and **efficient navigation** to find approximate nearest neighbors. It doesn't directly store the large vector data itself.
    *   `VectorStorage` handles the **actual storage of the high-dimensional vectors and their metadata**. This separation keeps the HNSW graph structure lean and focused on its algorithmic task.

2.  **Efficient Data Access:**
    *   When the HNSW algorithm needs to calculate distances between vectors during insertion or search, it needs quick access to the vector data. `VectorStorage` provides this through its `get_vector(index)` method.
    *   Storing vectors in a `std::vector<std::vector<float>>` (or similar contiguous structure) allows for efficient memory access, which is important for performance-critical distance calculations.

3.  **Metadata Management:**
    *   Each vector can have associated metadata (e.g., `{"author": "John Doe", "timestamp": "2023-10-26"}`). `VectorStorage` stores this metadata alongside its corresponding vector.
    *   This is vital for features like metadata filtering, where we might want to search only among vectors that match certain criteria.

4.  **Dimension Enforcement:**
    *   The `VectorStorage` constructor takes `vector_dimension`. The `add_vector` method then enforces that all incoming vectors conform to this specified dimension. This prevents errors and ensures data consistency.

5.  **Simplified HNSW Logic:**
    *   By abstracting away the actual vector and metadata storage, the `HNSW` class can work with simple integer IDs (node IDs) when building and traversing its graph. It delegates the responsibility of fetching the actual vector data or metadata to `VectorStorage` when needed. This makes the HNSW algorithm's implementation cleaner and easier to reason about.

**In essence, `VectorStorage` is the memory bank for your vectors and their attributes, while the `HNSW` graph is the intelligent index that helps you quickly navigate and find relevant entries within that bank.**

### How does the `Node` class exactly work?

The `Node` class represents a single point (or vertex) within the HNSW graph. It's a lightweight structure that primarily stores information about its identity and its connections to other nodes at different layers of the hierarchy.

Let's look at its components:

1.  **`uint32_t id;`**:
    *   This is the **unique identifier** for the node.
    *   Crucially, this `id` directly corresponds to the **index of the vector in the `VectorStorage`** that this node represents. So, if a `Node` has `id = 5`, it means this node in the HNSW graph corresponds to `vector_storage.get_vector(5)` and `vector_storage.get_metadata(5)`. This is the link between the graph structure and the actual data.

2.  **`int max_layer;`**:
    *   This indicates the **highest layer** in the HNSW graph that this particular node exists in.
    *   HNSW is a multi-layer graph. A node might exist in layer 0, layer 1, layer 2, etc. `max_layer` tells us the "ceiling" for this node.
    *   A node exists in all layers from 0 up to and including its `max_layer`. For example, if `max_layer` is 3, the node is present in layers 0, 1, 2, and 3.

3.  **`std::vector<std::vector<int>> neighbors;`**:
    *   This is the core of the node's connectivity information. It's a 2D vector (a vector of vectors).
    *   `neighbors[layer_level]` will contain a `std::vector<int>` of node IDs that are direct neighbors of *this* node at `layer_level`.
    *   The size of the outer vector (`neighbors.size()`) is `max_layer + 1`, because a node exists from layer 0 up to `max_layer`.
    *   The number of neighbors in each `neighbors[layer_level]` vector is limited by the `M` parameter (maximum connections) for that layer.

4.  **`Node(uint32_t id, int max_layer)` constructor**:
    *   Initializes the node with its `id` and `max_layer`.
    *   It then `resize`s the `neighbors` vector to `max_layer + 1`, ensuring there's a sub-vector for each layer this node will participate in.

**How `Node`s work in the HNSW graph:**

When the `HNSW::insert()` method adds a new vector:
*   It first gets a unique `id` (the next available index in `VectorStorage`).
*   It randomly determines a `max_layer` for this new node.
*   It creates a `Node` object using this `id` and `max_layer`.
*   Then, during the insertion process, it populates the `neighbors` lists for this new `Node` by finding its closest neighbors in various layers and adding their `id`s to the appropriate `neighbors[layer_level]` vectors.
*   Existing nodes also have their `neighbors` lists updated to include the new node if it's one of their closest neighbors.

So, the `Node` class doesn't store the heavy vector data itself; it stores the lightweight "map" of where that data is (`id` -> `VectorStorage` index) and how it's connected to other "maps" (other `Node`s) within the hierarchical graph structure.

### Is there a better way to ID the vectors?

That's an excellent and very practical question. The current method of using the `std::vector` index as the node ID is simple and efficient for a self-contained, in-memory system, but it has significant limitations, especially as we move towards more "industry-standard" features.

You're right to question it. There are indeed better ways to handle vector IDs, each with its own trade-offs.

#### The Limitations of Using the Vector Index as an ID

1.  **No Deletes:** This is the biggest issue. If we delete a vector from the middle of our `VectorStorage`, all the indices of the subsequent vectors shift. This would instantly invalidate all the `id`s stored in our HNSW graph's `neighbors` lists, effectively corrupting the entire index.
2.  **No Updates:** While you could technically update a vector in-place at its index, this is dangerous. If the updated vector is in a completely different part of the vector space, its connections in the graph are now likely wrong, leading to poor search performance. The correct way to handle an update is a `delete` followed by an `insert`, which we can't do.
3.  **No User-Defined IDs:** Users often want to use their own identifiers for vectors, such as a UUID, a database primary key (like a product SKU), or a file path. Our current system doesn't allow this; the ID is an internal implementation detail.
4.  **Not Scalable for Distributed Systems:** If you were to distribute this database across multiple machines, you would need a globally unique way to identify vectors, which a simple integer index within a single process cannot provide.

#### Better Ways to ID Vectors

Here are some common and more robust approaches:

##### 1. **Internal, Stable IDs with a Mapping Layer (The "SQLite" Approach)**

This is the most common and robust solution for a single-node database.

**How it works:**

*   **User-Facing ID:** The user interacts with a `std::string` or `int64_t` ID (e.g., a UUID or their own primary key).
*   **Internal ID:** The database internally uses a simple, dense integer ID (like our current `uint32_t id`) for the HNSW graph and `VectorStorage` because it's fast and efficient for array indexing.
*   **The Mapping Layer:** You create two `std::map` or `std::unordered_map` objects to bridge the two worlds:
    *   `std::unordered_map<std::string, uint32_t> user_id_to_internal_id;`
    *   `std::unordered_map<uint32_t, std::string> internal_id_to_user_id;`

**The Workflow:**

*   **Insert:**
    1.  A user provides a vector and a `string` ID (e.g., `"product-123"`).
    2.  You generate a *new* internal ID (e.g., the next available integer, `6`).
    3.  You store the mappings: `user_id_to_internal_id["product-123"] = 6;` and `internal_id_to_user_id[6] = "product-123";`.
    4.  You use the internal ID (`6`) to create the `Node` in the HNSW graph and store the vector at index `6` in `VectorStorage`.
*   **Query:**
    1.  The HNSW search returns a list of internal IDs (e.g., `[6, 10, 2]`).
    2.  Before returning the results to the user, you use `internal_id_to_user_id` to look up the original user-facing IDs.
*   **Delete:**
    1.  A user requests to delete `"product-123"`.
    2.  You look up the internal ID: `internal_id = user_id_to_internal_id["product-123"]`.
    3.  You can now mark this internal ID as "deleted" (a "soft delete"). You'd need a `std::set<uint32_t> deleted_ids;`.
    4.  During search, you would filter out any results that are in the `deleted_ids` set.
    5.  The maps would also be cleaned up.
    6.  This also enables true `update` (a delete followed by an insert).

**Pros:**
*   **Enables Deletes and Updates:** This is the primary benefit.
*   **User-Friendly:** Allows for arbitrary, user-defined string or integer IDs.
*   **Stable:** The internal IDs used by the graph never change, even if other vectors are deleted.

**Cons:**
*   **Memory Overhead:** The two maps introduce additional memory usage to store the mappings.
*   **Slight Performance Cost:** There's a small lookup cost to translate between ID types, but this is usually negligible compared to the vector search itself.

##### 2. **Directly Use `int64_t` or UUIDs as IDs (More Complex)**

Instead of using a dense `uint32_t` that maps to a `std::vector` index, you could change the graph to use a sparse `int64_t` or a UUID directly.

**How it works:**

*   The `Node`'s `id` field would become `int64_t`.
*   The `neighbors` lists would store `int64_t`s.
*   `VectorStorage` would need to be changed from a `std::vector` to a `std::unordered_map<int64_t, std::vector<float>>` to store the vectors.

**Pros:**
*   **Directly uses user IDs.** No mapping layer needed.

**Cons:**
*   **Performance Impact:** `std::unordered_map` lookups are slower than `std::vector` indexing. This would affect every single distance calculation, as you'd need to look up vector data from the map instead of accessing it directly by index. This can significantly slow down the database.
*   **Increased Memory Usage:** `std::unordered_map` has a higher memory overhead per entry than a `std::vector`.
*   **More Complex Implementation:** You would need to refactor a significant portion of the code that currently relies on dense integer IDs for indexing.

### Recommendation

For our project, **Approach #1 (Internal, Stable IDs with a Mapping Layer)** is by far the better choice. It's the standard way this problem is solved in high-performance, single-node vector databases. It provides the flexibility we need (deletes, user-defined IDs) with minimal performance impact.

It's a perfect example of adding a layer of abstraction to gain significant functionality without compromising the performance of the core algorithm.