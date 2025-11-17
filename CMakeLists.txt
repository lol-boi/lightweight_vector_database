# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(HNSW_VECTOR_DB)

# Set C++ standard to 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the main test executable
add_executable(hnsw_test test/test.cpp)
target_include_directories(hnsw_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define the deletion test executable
add_executable(delete_test test/test_delete.cpp)
target_include_directories(delete_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable testing with CTest
enable_testing()

# Add test cases that run the test executables
add_test(NAME RunHNSWTests COMMAND hnsw_test)
add_test(NAME RunHNSWDeleteTests COMMAND delete_test)

# Define the update test executable
add_executable(update_test test/test_update.cpp)
target_include_directories(update_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add a test case that runs the update test executable
add_test(NAME RunHNSWUpdateTests COMMAND update_test)
