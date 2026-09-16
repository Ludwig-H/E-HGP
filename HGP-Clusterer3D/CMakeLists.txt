cmake_minimum_required(VERSION 3.18...3.31)
project(hgp_clusterer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Python 3.9 REQUIRED COMPONENTS Interpreter Development.Module NumPy)
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(TBB REQUIRED)
else()
  find_package(TBB QUIET)
endif()

set(GEOGRAM_INSTALL_PREFIX "$ENV{GEOGRAM_INSTALL_PREFIX}" CACHE PATH "Geogram installation prefix")
find_path(GEOGRAM_INCLUDE_DIR geogram/basic/common.h
  HINTS "${GEOGRAM_INSTALL_PREFIX}" /usr/local
  PATH_SUFFIXES include/geogram1 include)
find_library(GEOGRAM_LIBRARY NAMES geogram
  HINTS "${GEOGRAM_INSTALL_PREFIX}" /usr/local
  PATH_SUFFIXES lib lib64)
if(NOT GEOGRAM_INCLUDE_DIR OR NOT GEOGRAM_LIBRARY)
  message(FATAL_ERROR "Geogram not found: set GEOGRAM_INSTALL_PREFIX to its installation prefix.")
endif()
get_filename_component(GEOGRAM_LIBRARY_DIR "${GEOGRAM_LIBRARY}" DIRECTORY)
message(STATUS "Geogram: ${GEOGRAM_LIBRARY}")

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hgp_clusterer")

pybind11_add_module(_geometry MODULE "${SOURCE_DIR}/_geometry.cpp")
target_include_directories(_geometry PRIVATE "${GEOGRAM_INCLUDE_DIR}")
target_link_libraries(_geometry PRIVATE "${GEOGRAM_LIBRARY}" Eigen3::Eigen)
target_compile_definitions(_geometry PRIVATE EIGEN_MPL2_ONLY)
set_target_properties(_geometry PROPERTIES INSTALL_RPATH "${GEOGRAM_LIBRARY_DIR}")
if(TARGET TBB::tbb)
  target_link_libraries(_geometry PRIVATE TBB::tbb)
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/_hierarchy.cpp"
  COMMAND Python::Interpreter -m cython --cplus -3
          "${SOURCE_DIR}/_hierarchy.pyx" -o "${CMAKE_CURRENT_BINARY_DIR}/_hierarchy.cpp"
  DEPENDS "${SOURCE_DIR}/_hierarchy.pyx"
  VERBATIM)
python_add_library(_hierarchy MODULE "${CMAKE_CURRENT_BINARY_DIR}/_hierarchy.cpp" WITH_SOABI)
target_link_libraries(_hierarchy PRIVATE Python::NumPy)
target_compile_definitions(_hierarchy PRIVATE NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION)

install(TARGETS _geometry _hierarchy LIBRARY DESTINATION hgp_clusterer)
