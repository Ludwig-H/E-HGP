cmake_minimum_required(VERSION 3.25)

project(MorseHGP3DConsumer LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MorseHGP3D CONFIG REQUIRED)

add_executable(morsehgp3d_consumer main.cpp)
target_link_libraries(
  morsehgp3d_consumer
  PRIVATE
    morsehgp3d::contract
    morsehgp3d::spatial
    morsehgp3d::direct_sparse_facet_descent_batch_plan
    morsehgp3d::direct_sparse_gateway_historical_quotient
    morsehgp3d::direct_sparse_positive_facet_prefix_sweep
)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME morsehgp3d.consumer COMMAND morsehgp3d_consumer)
endif()
