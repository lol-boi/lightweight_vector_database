#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <fstream>
#include <vector>
#include "hnsw.h"

namespace hnsw {

enum class SyncMode {
    FULL,
    NORMAL,
    OFF
};

class Database {
public:
    Database(const std::string& db_path, size_t vector_dimension, int M = 16, int efConstruction = 200, int efSearch = 50, DistanceMetric metric = DistanceMetric::L2, bool read_only = false, size_t cache_size_mb = 0)
        : db_path_(db_path), hnsw_(vector_dimension, M, efConstruction, efSearch, metric), read_only_(read_only), cache_size_mb_(cache_size_mb) {
        if (read_only) {
            load();
        }
        // TODO: Implement caching logic using cache_size_mb_
    }

    void insert(const std::vector<float>& vec, const Metadata& meta = {}) {
        if (read_only_) {
            throw std::runtime_error("Database is in read-only mode.");
        }
        hnsw_.insert(vec, meta);
    }

    std::vector<QueryResult> query(const std::vector<float>& query, int k, const FilterFunc& filter = nullptr, const std::set<Include>& include = {Include::ID}) {
        return hnsw_.k_nearest_neighbors(query, k, filter, include);
    }

    void save(SyncMode sync_mode = SyncMode::FULL) {
        if (read_only_) {
            throw std::runtime_error("Database is in read-only mode.");
        }
        std::ofstream ofs(db_path_, std::ios::binary);
        
        // Save HNSW parameters
        int M = hnsw_.get_M();
        int efConstruction = hnsw_.get_efConstruction();
        int efSearch = hnsw_.get_efSearch();
        DistanceMetric metric = hnsw_.get_distance_metric();
        ofs.write(reinterpret_cast<const char*>(&M), sizeof(M));
        ofs.write(reinterpret_cast<const char*>(&efConstruction), sizeof(efConstruction));
        ofs.write(reinterpret_cast<const char*>(&efSearch), sizeof(efSearch));
        ofs.write(reinterpret_cast<const char*>(&metric), sizeof(metric));

        // Save nodes
        const auto& nodes = hnsw_.get_nodes();
        size_t num_nodes = nodes.size();
        ofs.write(reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes));
        for (const auto& node : nodes) {
            ofs.write(reinterpret_cast<const char*>(&node.id), sizeof(node.id));
            ofs.write(reinterpret_cast<const char*>(&node.max_layer), sizeof(node.max_layer));
            for (int layer = 0; layer <= node.max_layer; ++layer) {
                size_t num_neighbors = node.neighbors[layer].size();
                ofs.write(reinterpret_cast<const char*>(&num_neighbors), sizeof(num_neighbors));
                ofs.write(reinterpret_cast<const char*>(node.neighbors[layer].data()), num_neighbors * sizeof(int));
            }
        }

        // Save vector storage
        const auto& vector_storage = hnsw_.get_vector_storage();
        size_t num_vectors = vector_storage.size();
        size_t vector_dimension = vector_storage.get_vector_dimension();
        ofs.write(reinterpret_cast<const char*>(&num_vectors), sizeof(num_vectors));
        ofs.write(reinterpret_cast<const char*>(&vector_dimension), sizeof(vector_dimension));
        for (size_t i = 0; i < num_vectors; ++i) {
            const auto& vec = vector_storage.get_vector(i);
            const auto& meta = vector_storage.get_metadata(i);
            ofs.write(reinterpret_cast<const char*>(vec.data()), vector_dimension * sizeof(float));
            
            size_t meta_size = meta.size();
            ofs.write(reinterpret_cast<const char*>(&meta_size), sizeof(meta_size));
            for (const auto& pair : meta) {
                size_t key_size = pair.first.size();
                ofs.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
                ofs.write(pair.first.c_str(), key_size);

                size_t value_size = pair.second.size();
                ofs.write(reinterpret_cast<const char*>(&value_size), sizeof(value_size));
                ofs.write(pair.second.c_str(), value_size);
            }
        }

        if (sync_mode == SyncMode::FULL) {
            ofs.flush();
            // TODO: fsync
        } else if (sync_mode == SyncMode::NORMAL) {
            ofs.flush();
        }
    }

    void load() {
        std::ifstream ifs(db_path_, std::ios::binary);
        if (!ifs) {
            return; // File doesn't exist yet
        }

        // Load HNSW parameters
        int M, efConstruction, efSearch;
        DistanceMetric metric;
        ifs.read(reinterpret_cast<char*>(&M), sizeof(M));
        ifs.read(reinterpret_cast<char*>(&efConstruction), sizeof(efConstruction));
        ifs.read(reinterpret_cast<char*>(&efSearch), sizeof(efSearch));
        ifs.read(reinterpret_cast<char*>(&metric), sizeof(metric));

        // Load nodes
        size_t num_nodes;
        ifs.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));
        std::vector<Node> nodes;
        nodes.reserve(num_nodes);
        for (size_t i = 0; i < num_nodes; ++i) {
            uint32_t id;
            int max_layer;
            ifs.read(reinterpret_cast<char*>(&id), sizeof(id));
            ifs.read(reinterpret_cast<char*>(&max_layer), sizeof(max_layer));
            Node node(id, max_layer);
            for (int layer = 0; layer <= max_layer; ++layer) {
                size_t num_neighbors;
                ifs.read(reinterpret_cast<char*>(&num_neighbors), sizeof(num_neighbors));
                node.neighbors[layer].resize(num_neighbors);
                ifs.read(reinterpret_cast<char*>(node.neighbors[layer].data()), num_neighbors * sizeof(int));
            }
            nodes.push_back(node);
        }

        // Load vector storage
        size_t num_vectors, vector_dimension;
        ifs.read(reinterpret_cast<char*>(&num_vectors), sizeof(num_vectors));
        ifs.read(reinterpret_cast<char*>(&vector_dimension), sizeof(vector_dimension));
        
        VectorStorage vector_storage(vector_dimension);
        for (size_t i = 0; i < num_vectors; ++i) {
            std::vector<float> vec(vector_dimension);
            ifs.read(reinterpret_cast<char*>(vec.data()), vector_dimension * sizeof(float));

            size_t meta_size;
            ifs.read(reinterpret_cast<char*>(&meta_size), sizeof(meta_size));
            Metadata meta;
            for (size_t j = 0; j < meta_size; ++j) {
                size_t key_size, value_size;
                std::string key, value;

                ifs.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
                key.resize(key_size);
                ifs.read(&key[0], key_size);

                ifs.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));
                value.resize(value_size);
                ifs.read(&value[0], value_size);
                meta[key] = value;
            }
            vector_storage.add_vector(vec, meta);
        }

        hnsw_ = HNSW(vector_dimension, M, efConstruction, efSearch, metric, nodes, vector_storage);
    }

private:
    std::string db_path_;
    HNSW hnsw_;
    bool read_only_;
    size_t cache_size_mb_;
};

} // namespace hnsw

#endif // DATABASE_H
