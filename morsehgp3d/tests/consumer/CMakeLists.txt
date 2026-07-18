cmake_minimum_required(VERSION 3.25)

project(MorseHGP3DConsumer LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MorseHGP3D CONFIG REQUIRED)

add_executable(morsehgp3d_consumer main.cpp)
target_link_libraries(morsehgp3d_consumer PRIVATE morsehgp3d::spatial)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME morsehgp3d.consumer COMMAND morsehgp3d_consumer)
endif()
