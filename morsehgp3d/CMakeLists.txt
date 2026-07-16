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
option(
  MORSEHGP3D_BUILD_TOOLS
  "Build the MorseHGP3D diagnostic replay tools"
  ${PROJECT_IS_TOP_LEVEL}
)
option(
  MORSEHGP3D_ENABLE_CUDA
  "Enable the opt-in CUDA 12.9 sm_120 build envelope"
  OFF
)
option(
  MORSEHGP3D_CUDA_AUDIT
  "Add compile-time CUDA audit diagnostics"
  OFF
)
option(
  MORSEHGP3D_ENABLE_SANITIZERS
  "Instrument project-owned CPU targets with AddressSanitizer and UBSan"
  OFF
)

if(MORSEHGP3D_CUDA_AUDIT AND NOT MORSEHGP3D_ENABLE_CUDA)
  message(FATAL_ERROR "MORSEHGP3D_CUDA_AUDIT requires MORSEHGP3D_ENABLE_CUDA=ON")
endif()
if(MORSEHGP3D_ENABLE_SANITIZERS AND MORSEHGP3D_ENABLE_CUDA)
  message(FATAL_ERROR "The Phase 3.1 sanitizer profile is CPU-only")
endif()

add_library(morsehgp3d_sanitizers INTERFACE)
if(MORSEHGP3D_ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(
      morsehgp3d_sanitizers
      INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(
      morsehgp3d_sanitizers
      INTERFACE -fsanitize=address,undefined
    )
  else()
    message(
      FATAL_ERROR
      "The Phase 3.1 sanitizer profile is unsupported for ${CMAKE_CXX_COMPILER_ID}"
    )
  endif()
endif()

function(morsehgp3d_apply_sanitizers target_name)
  if(NOT TARGET "${target_name}")
    message(FATAL_ERROR "Cannot sanitize missing target ${target_name}")
  endif()
  target_link_libraries("${target_name}" PRIVATE morsehgp3d_sanitizers)
endfunction()

if(MORSEHGP3D_ENABLE_CUDA)
  include(cmake/MorseHGP3DCuda.cmake)
  add_library(
    morsehgp3d_cuda_environment_probe
    OBJECT
    EXCLUDE_FROM_ALL
    src/cuda/environment_probe.cu
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_cuda_environment_probe)
endif()

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
  target_compile_options(
    morsehgp3d_exact
    INTERFACE -fno-fast-math -ffp-contract=off -frounding-math
  )
else()
  message(FATAL_ERROR "The certified FP mode is not configured for ${CMAKE_CXX_COMPILER_ID}")
endif()

if(MORSEHGP3D_BUILD_TOOLS)
  add_executable(
    morsehgp3d_replay_predicate
    src/tools/predicate_replay.cpp
  )
  target_link_libraries(morsehgp3d_replay_predicate PRIVATE morsehgp3d::exact)
  morsehgp3d_apply_sanitizers(morsehgp3d_replay_predicate)
  if(MSVC)
    target_compile_options(morsehgp3d_replay_predicate PRIVATE /W4 /WX)
  else()
    target_compile_options(
      morsehgp3d_replay_predicate
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()
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
if(MORSEHGP3D_BUILD_TOOLS)
  install(TARGETS morsehgp3d_replay_predicate RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(
    PROGRAMS tools/replay_predicate.py
    TYPE BIN
    RENAME morsehgp3d-replay-predicate
  )
endif()
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
