cmake_minimum_required(VERSION 3.20)
project(mhgp9_b_s2_trace LANGUAGES CXX)

# Build the unmodified product library as a subproject, but only link its
# public CPU generator API. This audit target is not part of the product.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/product")
add_executable(mhgp9_b_s2_trace trace.cpp)
target_link_libraries(mhgp9_b_s2_trace PRIVATE mhgp9_gen)
target_include_directories(mhgp9_b_s2_trace SYSTEM PRIVATE "${MHGP9_BOOST_INCLUDE_DIR}")
target_compile_features(mhgp9_b_s2_trace PRIVATE cxx_std_20)
target_compile_options(mhgp9_b_s2_trace PRIVATE -Wall -Wextra -Wpedantic -Werror)
