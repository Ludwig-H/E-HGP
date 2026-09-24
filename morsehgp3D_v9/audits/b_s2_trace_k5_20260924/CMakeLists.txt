cmake_minimum_required(VERSION 3.20)
project(mhgp9_b_s2_trace_k5 LANGUAGES CXX)

# Audit-only CPU sidecar; product sources are compiled unchanged.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/product")
add_executable(mhgp9_b_s2_trace_k5 trace.cpp)
target_link_libraries(mhgp9_b_s2_trace_k5 PRIVATE mhgp9_gen)
target_include_directories(mhgp9_b_s2_trace_k5 SYSTEM PRIVATE "${MHGP9_BOOST_INCLUDE_DIR}")
target_compile_features(mhgp9_b_s2_trace_k5 PRIVATE cxx_std_20)
target_compile_options(mhgp9_b_s2_trace_k5 PRIVATE -Wall -Wextra -Wpedantic -Werror)
