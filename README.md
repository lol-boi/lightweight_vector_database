# Lightweight Vector Database

This project aims to implement a lightweight Hierarchical Navigable Small World (HNSW) vector database in C++.

## Current Status

**Phase 1: Foundations Implemented**
- **Data Structures**:
    - `VectorStorage`: Manages the storage of vectors.
    - `Node`: Represents a node in the HNSW graph, storing its ID, maximum layer, and adjacency lists for each layer.
    - `HNSW` class: Encapsulates the graph structure, including `VectorStorage` and `Node` management.
- **Distance Metric**:
    - `squared_l2_distance`: Calculates the Squared L2 Euclidean distance between two vectors.

**Phase 2: The Core Engine (Greedy Search) - In Progress**
- `search_layer` method placeholder added to `HNSW` class.

## Roadmap

The full roadmap can be found in `roadmap.txt`.

## How to Build (Placeholder)

(Instructions for building the project will be added here in future phases.)