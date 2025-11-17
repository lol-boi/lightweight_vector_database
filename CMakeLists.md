cmake_minimum_required(VERSION 3.10)
project(LightweightVectorDatabase CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the hnsw library
add_executable(hnsw_test test/test.cpp src/hnsw.h)
target_include_directories(hnsw_test PRIVATE src)

enable_testing()
add_test(NAME HNSWBasicTest COMMAND hnsw_test)
