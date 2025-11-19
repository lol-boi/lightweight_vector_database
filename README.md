# Lightweight Vector Database

A lightweight, header-only Hierarchical Navigable Small World (HNSW) vector database in C++.

## How to Build

This is a header-only library, so there is no need to build it separately. Just include `database.h` in your project.

A simple example of how to use the library can be found in `test/test.cpp`.

## Features

### CRUD Operations

The database supports full Create, Read, Update, and Delete operations.

*   **`insert(vector, metadata)`**: Adds a new vector to the database.
*   **`query(vector, k)`**: Finds the `k` nearest neighbors to a query vector.
*   **`delete_vector(id)`**: Marks a vector as deleted using a soft-delete mechanism.
*   **`update_vector(id, vector, metadata)`**: Updates a vector. This performs a soft-delete on the old vector, inserts the new vector, and returns the new ID. Note that the ID of the vector will change.

### Deletion and Index Rebuilding

This database uses a **soft-delete** strategy for performance and simplicity.

*   **How it works**: When you call `delete_vector(id)`, the vector is not immediately removed from the index. Instead, it is marked as "deleted" and will be excluded from all future query results.

*   **Performance Implications**: Because the "deleted" vectors (nodes) remain in the graph, a very high number of deletions can eventually slow down search performance as the algorithm traverses these now-useless nodes. Memory is also not reclaimed.

*   **`rebuild_index()`**: To solve this, the `Database` class provides a `rebuild_index()` method. This function permanently removes all deleted vectors by building a new, clean index from scratch. You should call this method periodically if your application involves a high volume of deletions to maintain optimal performance and reclaim memory.

### Scalar Quantization

Scalar quantization can be used to reduce memory usage by quantizing vectors to 8-bit integers. This can be enabled in the `Database` constructor:

```cpp
hnsw::Database db("my_db.bin", 128, 16, 200, 50, hnsw::DistanceMetric::L2, false, 0, true);
```

When scalar quantization is enabled, the `rebuild_index()` method will train the quantizer and encode all vectors. Subsequent queries will use Asymmetric Distance Computation (ADC) to calculate distances between the full-precision query vector and the quantized database vectors.