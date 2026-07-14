cmake_minimum_required(VERSION 3.25)

project(
  MorseHGP3D
  VERSION 0.1.0
  DESCRIPTION "Certified MorseHGP3D implementation"
  LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)

option(
  MORSEHGP3D_BUILD_TESTS
  "Build the MorseHGP3D CPU unit tests"
  ${PROJECT_IS_TOP_LEVEL}
)

find_package(Boost 1.74 REQUIRED CONFIG COMPONENTS headers)

add_library(morsehgp3d_exact INTERFACE)
add_library(morsehgp3d::exact ALIAS morsehgp3d_exact)
set_target_properties(morsehgp3d_exact PROPERTIES EXPORT_NAME exact)

target_compile_features(morsehgp3d_exact INTERFACE cxx_std_20)
target_include_directories(
  morsehgp3d_exact
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(morsehgp3d_exact INTERFACE Boost::headers)

if(MSVC)
  target_compile_options(morsehgp3d_exact INTERFACE /fp:strict)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(morsehgp3d_exact INTERFACE -fno-fast-math -ffp-contract=off)
else()
  message(FATAL_ERROR "The certified FP mode is not configured for ${CMAKE_CXX_COMPILER_ID}")
endif()

if(MORSEHGP3D_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests/unit)
  endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
  TARGETS morsehgp3d_exact
  EXPORT MorseHGP3DTargets
)
install(
  DIRECTORY include/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
  EXPORT MorseHGP3DTargets
  FILE MorseHGP3DTargets.cmake
  NAMESPACE morsehgp3d::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MorseHGP3D"
)

configure_package_config_file(
  cmake/MorseHGP3DConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/MorseHGP3DConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MorseHGP3D"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/MorseHGP3DConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MorseHGP3DConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MorseHGP3DConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/MorseHGP3D"
)
