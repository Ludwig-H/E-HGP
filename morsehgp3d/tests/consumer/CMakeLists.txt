cmake_minimum_required(VERSION 3.25)

project(MorseHGP3DConsumer LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MorseHGP3D CONFIG REQUIRED)

foreach(
  _morsehgp3d_public_target
  IN ITEMS
    morsehgp3d::morsehgp3d
    morsehgp3d::point_hierarchy
    morsehgp3d::contract
    morsehgp3d::exact
)
  if(NOT TARGET "${_morsehgp3d_public_target}")
    message(FATAL_ERROR "Missing public target ${_morsehgp3d_public_target}")
  endif()
endforeach()
foreach(
  _morsehgp3d_internal_target
  IN ITEMS
    morsehgp3d::spatial
    morsehgp3d::hierarchy
    morsehgp3d::sanitizers
)
  if(TARGET "${_morsehgp3d_internal_target}")
    message(FATAL_ERROR "Internal target leaked: ${_morsehgp3d_internal_target}")
  endif()
endforeach()

add_executable(morsehgp3d_consumer main.cpp)
target_link_libraries(
  morsehgp3d_consumer
  PRIVATE morsehgp3d::morsehgp3d
)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME morsehgp3d.consumer COMMAND morsehgp3d_consumer)
endif()
