# API Reference

This document provides a reference for the public API of the lightweight vector database.

## `hnsw::Database`

The `Database` class is the main entry point for interacting with the vector database.

### Constructor

```cpp
Database(const std::string& db_path, size_t vector_dimension, int M = 16, int efConstruction = 200, int efSearch = 50, DistanceMetric metric = DistanceMetric::L2, bool read_only = false, size_t cache_size_mb = 0, bool sq_enabled = false);
```

- `db_path`: Path to the database file.
- `vector_dimension`: The dimension of the vectors to be stored.
- `M`: The maximum number of connections per node in the HNSW graph.
- `efConstruction`: The size of the dynamic list for candidates during index construction.
- `efSearch`: The size of the dynamic list for candidates during search.
- `metric`: The distance metric to use. See `hnsw::DistanceMetric`.
- `read_only`: If true, the database is opened in read-only mode.
- `cache_size_mb`: (Not yet implemented)
- `sq_enabled`: If true, scalar quantization is enabled.

### `insert`

```cpp
uint32_t insert(const std::vector<float>& vec, const Metadata& meta = {});
```

Inserts a vector into the database.

- `vec`: The vector to insert.
- `meta`: Optional metadata to associate with the vector.
- **Returns**: The ID of the inserted vector.

### `update_vector`

```cpp
uint32_t update_vector(uint32_t id, const std::vector<float>& new_vec, const Metadata& new_meta = {});
```

Updates an existing vector. This is currently implemented as a delete followed by an insert.

- `id`: The ID of the vector to update.
- `new_vec`: The new vector.
- `new_meta`: The new metadata.
- **Returns**: The new ID of the updated vector.

### `delete_vector`

```cpp
void delete_vector(uint32_t id);
```

Marks a vector as deleted. The vector is not immediately removed from the database but will be excluded from search results.

- `id`: The ID of the vector to delete.

### `query`

```cpp
std::vector<QueryResult> query(const std::vector<float>& query, int k, const FilterFunc& filter = nullptr, const std::set<Include>& include = {Include::ID});
```

Performs a k-nearest neighbor search.

- `query`: The query vector.
- `k`: The number of nearest neighbors to return.
- `filter`: An optional function to filter results based on metadata.
- `include`: A set of `Include` enums to specify what data to return in the results.
- **Returns**: A vector of `QueryResult` objects.

### `rebuild_index`

```cpp
void rebuild_index();
```

Rebuilds the HNSW index. This is useful after a large number of deletions to reclaim space and improve performance. If scalar quantization is enabled, it will also retrain the quantizer.

### `save`

```cpp
void save(SyncMode sync_mode = SyncMode::FULL);
```

Saves the database to disk.

- `sync_mode`: The synchronization mode. See `hnsw::SyncMode`.

### `load`

```cpp
void load();
```

Loads the database from disk. This is called automatically by the constructor if the database file exists.

## Enums and Type Aliases

### `hnsw::SyncMode`

```cpp
enum class SyncMode {
    FULL,
    NORMAL,
    OFF
};
```

- `FULL`: Flushes the output stream to disk.
- `NORMAL`: (Not yet implemented)
- `OFF`: Does not flush the output stream.

### `hnsw::DistanceMetric`

```cpp
enum class DistanceMetric {
    L2,
    COSINE,
    IP // Inner Product
};
```

- `L2`: Euclidean distance.
- `COSINE`: Cosine distance.
- `IP`: Inner product.

### `hnsw::Include`

```cpp
enum class Include {
    ID,
    DISTANCE,
    METADATA,
    VECTOR
};
```

Used in `query` to specify what data to include in the results.

### `hnsw::Metadata`

```cpp
using Metadata = std::map<std::string, std::string>;
```

A map of string key-value pairs to store metadata.

### `hnsw::FilterFunc`

```cpp
using FilterFunc = std::function<bool(const Metadata&)>;
```

A function that takes metadata as input and returns `true` if the vector should be included in the search results.

## `hnsw::QueryResult`

```cpp
struct QueryResult {
    int id;
    float distance;
    Metadata metadata;
    std::vector<float> vector;
};
```

- `id`: The ID of the vector.
- `distance`: The distance from the query vector.
- `metadata`: The metadata of the vector.
- `vector`: The vector itself.
