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
add_library(morsehgp3d::sanitizers ALIAS morsehgp3d_sanitizers)
set_target_properties(morsehgp3d_sanitizers PROPERTIES EXPORT_NAME sanitizers)
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

  find_package(Git REQUIRED)
  file(
    REAL_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    MORSEHGP3D_PHASE3_REPOSITORY_ROOT
  )
  execute_process(
    COMMAND
      "${GIT_EXECUTABLE}"
      -c "safe.directory=${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}"
      rev-parse HEAD
    WORKING_DIRECTORY "${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}"
    RESULT_VARIABLE _morsehgp3d_git_revision_status
    OUTPUT_VARIABLE MORSEHGP3D_PHASE3_GIT_SHA
    ERROR_VARIABLE _morsehgp3d_git_revision_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(LENGTH "${MORSEHGP3D_PHASE3_GIT_SHA}" _morsehgp3d_git_revision_length)
  if(
    NOT _morsehgp3d_git_revision_status EQUAL 0
    OR NOT _morsehgp3d_git_revision_length EQUAL 40
    OR NOT MORSEHGP3D_PHASE3_GIT_SHA MATCHES "^[0-9a-f]+$"
  )
    message(
      FATAL_ERROR
      "The Phase 3 CUDA build requires a canonical Git HEAD: "
      "${_morsehgp3d_git_revision_error}"
    )
  endif()
  execute_process(
    COMMAND
      "${GIT_EXECUTABLE}"
      -c "safe.directory=${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}"
      status --porcelain --untracked-files=normal
    WORKING_DIRECTORY "${MORSEHGP3D_PHASE3_REPOSITORY_ROOT}"
    RESULT_VARIABLE _morsehgp3d_git_status_result
    OUTPUT_VARIABLE _morsehgp3d_git_status
    ERROR_VARIABLE _morsehgp3d_git_status_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT _morsehgp3d_git_status_result EQUAL 0)
    message(
      FATAL_ERROR
      "The Phase 3 CUDA build cannot verify the Git worktree: "
      "${_morsehgp3d_git_status_error}"
    )
  endif()
  if(NOT _morsehgp3d_git_status STREQUAL "")
    message(
      FATAL_ERROR
      "The Phase 3 CUDA build requires a clean worktree so its manifest SHA "
      "identifies the compiled sources"
    )
  endif()

  add_library(
    morsehgp3d_cuda_environment_probe
    OBJECT
    EXCLUDE_FROM_ALL
    src/cuda/environment_probe.cu
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_cuda_environment_probe)

  include(cmake/MorseHGP3DPhase3Dependencies.cmake)

  add_executable(
    morsehgp3d_phase3_runtime
    src/cuda/phase3_runtime.cu
  )
  target_link_libraries(
    morsehgp3d_phase3_runtime
    PRIVATE morsehgp3d::phase3_dependencies
  )
  target_compile_definitions(
    morsehgp3d_phase3_runtime
    PRIVATE
      MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
      MORSEHGP3D_CUDA_AUDIT=$<IF:$<BOOL:${MORSEHGP3D_CUDA_AUDIT}>,1,0>
  )
  # nvcc compiles a cudafe-generated host translation unit in addition to the
  # project source.  Do not forward host warning bundles or -Werror to that
  # generated code; the native C++ targets below retain their strict policy.
  morsehgp3d_configure_cuda_target(morsehgp3d_phase3_runtime)

  nanobind_add_module(
    morsehgp3d_phase3
    NB_STATIC
    NB_SUPPRESS_WARNINGS
    NOMINSIZE
    src/python/phase3_module.cpp
  )
  set_target_properties(
    morsehgp3d_phase3
    PROPERTIES
      CXX_EXTENSIONS OFF
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
  )
  target_link_libraries(
    morsehgp3d_phase3
    PRIVATE morsehgp3d::phase3_dependencies
  )
  if(MSVC)
    target_compile_options(morsehgp3d_phase3 PRIVATE /W4 /WX)
  else()
    target_compile_options(
      morsehgp3d_phase3
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  unset(_morsehgp3d_git_revision_error)
  unset(_morsehgp3d_git_revision_length)
  unset(_morsehgp3d_git_revision_status)
  unset(_morsehgp3d_git_status)
  unset(_morsehgp3d_git_status_error)
  unset(_morsehgp3d_git_status_result)
  unset(MORSEHGP3D_PHASE3_REPOSITORY_ROOT)
endif()

find_package(Boost 1.74 REQUIRED CONFIG COMPONENTS headers)

add_library(
  morsehgp3d_contract
  STATIC
    src/cpu/contract/canonical_id.cpp
)
add_library(morsehgp3d::contract ALIAS morsehgp3d_contract)
set_target_properties(
  morsehgp3d_contract
  PROPERTIES
    EXPORT_NAME contract
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_contract PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_contract
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_contract
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_contract PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

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
  target_compile_options(
    morsehgp3d_exact
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/fp:strict>
  )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(
    morsehgp3d_exact
    INTERFACE
      $<$<COMPILE_LANGUAGE:CXX>:-fno-fast-math>
      $<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>
      $<$<COMPILE_LANGUAGE:CXX>:-frounding-math>
  )
else()
  message(FATAL_ERROR "The certified FP mode is not configured for ${CMAKE_CXX_COMPILER_ID}")
endif()

add_library(
  morsehgp3d_spatial
  STATIC
    src/cpu/spatial/point_cloud.cpp
    src/cpu/spatial/point_cloud_aabb.cpp
    src/cpu/spatial/brute_force.cpp
    src/cpu/spatial/h_polytope_reference.cpp
    src/cpu/spatial/lbvh.cpp
    src/cpu/spatial/lbvh_budgeted_top_k.cpp
    src/cpu/spatial/ordinary_cell_closure.cpp
    src/cpu/spatial/ordinary_diagram_closure.cpp
    src/cpu/spatial/power_cell_reference.cpp
)
add_library(morsehgp3d::spatial ALIAS morsehgp3d_spatial)
set_target_properties(
  morsehgp3d_spatial
  PROPERTIES
    EXPORT_NAME spatial
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_spatial PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_spatial
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_spatial
  PUBLIC morsehgp3d::exact
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_spatial PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_spatial
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_facet_miniball
  STATIC
    src/cpu/hierarchy/facet_miniball.cpp
)
add_library(morsehgp3d::facet_miniball ALIAS morsehgp3d_facet_miniball)
set_target_properties(
  morsehgp3d_facet_miniball
  PROPERTIES
    EXPORT_NAME facet_miniball
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_facet_miniball PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_facet_miniball
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_facet_miniball
  PUBLIC morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_facet_miniball PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_facet_miniball
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_pair_support
  STATIC
    src/cpu/hierarchy/pair_support_stream.cpp
)
add_library(morsehgp3d::pair_support ALIAS morsehgp3d_pair_support)
set_target_properties(
  morsehgp3d_pair_support
  PROPERTIES
    EXPORT_NAME pair_support
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_pair_support PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_pair_support
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_pair_support
  PUBLIC morsehgp3d::spatial morsehgp3d::contract
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_pair_support PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_pair_support
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_higher_support
  STATIC
    src/cpu/hierarchy/higher_support_product.cpp
    src/cpu/hierarchy/higher_support_stream.cpp
)
add_library(morsehgp3d::higher_support ALIAS morsehgp3d_higher_support)
set_target_properties(
  morsehgp3d_higher_support
  PROPERTIES
    EXPORT_NAME higher_support
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_higher_support PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_higher_support
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_higher_support
  PUBLIC morsehgp3d::spatial morsehgp3d::contract
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_higher_support PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_higher_support
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_support_terminal
  STATIC
    src/cpu/hierarchy/direct_support_terminal.cpp
)
add_library(
  morsehgp3d::direct_support_terminal
  ALIAS morsehgp3d_direct_support_terminal
)
set_target_properties(
  morsehgp3d_direct_support_terminal
  PROPERTIES
    EXPORT_NAME direct_support_terminal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_support_terminal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_support_terminal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_support_terminal
  PUBLIC
    morsehgp3d::pair_support
    morsehgp3d::higher_support
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_support_terminal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_support_terminal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_morse_event_journal
  STATIC
    src/cpu/hierarchy/direct_morse_event_journal.cpp
)
add_library(
  morsehgp3d::direct_morse_event_journal
  ALIAS morsehgp3d_direct_morse_event_journal
)
set_target_properties(
  morsehgp3d_direct_morse_event_journal
  PROPERTIES
    EXPORT_NAME direct_morse_event_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_event_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_event_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_event_journal
  PUBLIC morsehgp3d::direct_support_terminal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_event_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_event_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_saddle_arm_seed_journal
  STATIC
    src/cpu/hierarchy/direct_saddle_arm_seed_journal.cpp
)
add_library(
  morsehgp3d::direct_saddle_arm_seed_journal
  ALIAS morsehgp3d_direct_saddle_arm_seed_journal
)
set_target_properties(
  morsehgp3d_direct_saddle_arm_seed_journal
  PROPERTIES
    EXPORT_NAME direct_saddle_arm_seed_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_saddle_arm_seed_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_saddle_arm_seed_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_saddle_arm_seed_journal
  PUBLIC morsehgp3d::direct_morse_event_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_saddle_arm_seed_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_saddle_arm_seed_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_closed_saddle_incidence_journal
  STATIC
    src/cpu/hierarchy/direct_closed_saddle_incidence_journal.cpp
)
add_library(
  morsehgp3d::direct_closed_saddle_incidence_journal
  ALIAS morsehgp3d_direct_closed_saddle_incidence_journal
)
set_target_properties(
  morsehgp3d_direct_closed_saddle_incidence_journal
  PROPERTIES
    EXPORT_NAME direct_closed_saddle_incidence_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_closed_saddle_incidence_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_closed_saddle_incidence_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_closed_saddle_incidence_journal
  PUBLIC morsehgp3d::direct_saddle_arm_seed_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_closed_saddle_incidence_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_closed_saddle_incidence_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_positive_facet_locator
  STATIC
    src/cpu/hierarchy/direct_sparse_positive_facet_locator.cpp
)
add_library(
  morsehgp3d::direct_sparse_positive_facet_locator
  ALIAS morsehgp3d_direct_sparse_positive_facet_locator
)
set_target_properties(
  morsehgp3d_direct_sparse_positive_facet_locator
  PROPERTIES
    EXPORT_NAME direct_sparse_positive_facet_locator
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_positive_facet_locator PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_positive_facet_locator
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_positive_facet_locator
  PUBLIC morsehgp3d::contract
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_positive_facet_locator PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_positive_facet_locator
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_positive_facet_prefix_sweep
  STATIC
    src/cpu/hierarchy/direct_sparse_positive_facet_prefix_sweep.cpp
)
add_library(
  morsehgp3d::direct_sparse_positive_facet_prefix_sweep
  ALIAS morsehgp3d_direct_sparse_positive_facet_prefix_sweep
)
set_target_properties(
  morsehgp3d_direct_sparse_positive_facet_prefix_sweep
  PROPERTIES
    EXPORT_NAME direct_sparse_positive_facet_prefix_sweep
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_positive_facet_prefix_sweep PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_positive_facet_prefix_sweep
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_positive_facet_prefix_sweep
  PUBLIC morsehgp3d::direct_sparse_positive_facet_locator
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_positive_facet_prefix_sweep PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_positive_facet_prefix_sweep
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_facet_descent_step
  STATIC
    src/cpu/hierarchy/direct_sparse_facet_descent_step.cpp
)
add_library(
  morsehgp3d::direct_sparse_facet_descent_step
  ALIAS morsehgp3d_direct_sparse_facet_descent_step
)
set_target_properties(
  morsehgp3d_direct_sparse_facet_descent_step
  PROPERTIES
    EXPORT_NAME direct_sparse_facet_descent_step
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_facet_descent_step PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_facet_descent_step
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_facet_descent_step
  PUBLIC
    morsehgp3d::direct_sparse_positive_facet_locator
    morsehgp3d::facet_miniball
    morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_step PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_step
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_facet_descent_closure
  STATIC
    src/cpu/hierarchy/direct_sparse_facet_descent_closure.cpp
)
add_library(
  morsehgp3d::direct_sparse_facet_descent_closure
  ALIAS morsehgp3d_direct_sparse_facet_descent_closure
)
set_target_properties(
  morsehgp3d_direct_sparse_facet_descent_closure
  PROPERTIES
    EXPORT_NAME direct_sparse_facet_descent_closure
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_facet_descent_closure PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_facet_descent_closure
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_facet_descent_closure
  PUBLIC morsehgp3d::direct_sparse_facet_descent_step
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_closure PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_closure
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_first_incidence
  STATIC
    src/cpu/hierarchy/direct_sparse_first_incidence.cpp
)
add_library(
  morsehgp3d::direct_sparse_first_incidence
  ALIAS morsehgp3d_direct_sparse_first_incidence
)
set_target_properties(
  morsehgp3d_direct_sparse_first_incidence
  PROPERTIES
    EXPORT_NAME direct_sparse_first_incidence
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_first_incidence PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_first_incidence
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_first_incidence
  PUBLIC
    morsehgp3d::direct_sparse_positive_facet_locator
    morsehgp3d::facet_miniball
    morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_first_incidence PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_first_incidence
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_candidate_journal
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_candidate_journal.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_candidate_journal
  ALIAS morsehgp3d_direct_sparse_gateway_candidate_journal
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_candidate_journal
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_candidate_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_candidate_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_candidate_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_candidate_journal
  PUBLIC
    morsehgp3d::direct_sparse_first_incidence
    morsehgp3d::direct_closed_saddle_incidence_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_candidate_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_candidate_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_candidate_localization
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_candidate_localization.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_candidate_localization
  ALIAS morsehgp3d_direct_sparse_gateway_candidate_localization
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_candidate_localization
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_candidate_localization
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_candidate_localization PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_candidate_localization
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_candidate_localization
  PUBLIC
    morsehgp3d::direct_sparse_gateway_candidate_journal
    morsehgp3d::direct_sparse_positive_facet_locator
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_candidate_localization PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_candidate_localization
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_clock
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_clock.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_clock
  ALIAS morsehgp3d_direct_sparse_gateway_clock
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_clock
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_clock
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_clock PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_clock
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_clock
  PUBLIC
    morsehgp3d::direct_sparse_gateway_candidate_journal
    morsehgp3d::direct_sparse_positive_facet_locator
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_clock PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_clock
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_clock_authority
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_clock_authority.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_clock_authority
  ALIAS morsehgp3d_direct_sparse_gateway_clock_authority
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_clock_authority
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_clock_authority
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_clock_authority PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_clock_authority
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_clock_authority
  PUBLIC morsehgp3d::direct_sparse_gateway_clock
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_clock_authority PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_clock_authority
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_temporal_resolution
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_temporal_resolution.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_temporal_resolution
  ALIAS morsehgp3d_direct_sparse_gateway_temporal_resolution
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_temporal_resolution
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_temporal_resolution
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_temporal_resolution PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_temporal_resolution
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_temporal_resolution
  PUBLIC
    morsehgp3d::direct_sparse_gateway_candidate_localization
    morsehgp3d::direct_sparse_gateway_clock_authority
    morsehgp3d::direct_sparse_positive_facet_prefix_sweep
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_temporal_resolution PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_temporal_resolution
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_gateway_historical_quotient
  STATIC
    src/cpu/hierarchy/direct_sparse_gateway_historical_quotient.cpp
)
add_library(
  morsehgp3d::direct_sparse_gateway_historical_quotient
  ALIAS morsehgp3d_direct_sparse_gateway_historical_quotient
)
set_target_properties(
  morsehgp3d_direct_sparse_gateway_historical_quotient
  PROPERTIES
    EXPORT_NAME direct_sparse_gateway_historical_quotient
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_gateway_historical_quotient PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_gateway_historical_quotient
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_gateway_historical_quotient
  PUBLIC morsehgp3d::direct_sparse_gateway_temporal_resolution
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_historical_quotient PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_gateway_historical_quotient
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_frozen_root_quotient
  STATIC
    src/cpu/hierarchy/direct_frozen_root_quotient.cpp
)
add_library(
  morsehgp3d::direct_frozen_root_quotient
  ALIAS morsehgp3d_direct_frozen_root_quotient
)
set_target_properties(
  morsehgp3d_direct_frozen_root_quotient
  PROPERTIES
    EXPORT_NAME direct_frozen_root_quotient
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_frozen_root_quotient PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_frozen_root_quotient
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_frozen_root_quotient
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_frozen_root_quotient PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_frozen_root_quotient
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_k1_forest_journal
  STATIC
    src/cpu/hierarchy/direct_k1_forest_journal.cpp
)
add_library(
  morsehgp3d::direct_k1_forest_journal
  ALIAS morsehgp3d_direct_k1_forest_journal
)
set_target_properties(
  morsehgp3d_direct_k1_forest_journal
  PROPERTIES
    EXPORT_NAME direct_k1_forest_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_k1_forest_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_k1_forest_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_k1_forest_journal
  PUBLIC morsehgp3d::direct_saddle_arm_seed_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_k1_forest_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_k1_forest_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_morse_forest_journal
  STATIC
    src/cpu/hierarchy/direct_morse_forest_journal.cpp
)
add_library(
  morsehgp3d::direct_morse_forest_journal
  ALIAS morsehgp3d_direct_morse_forest_journal
)
set_target_properties(
  morsehgp3d_direct_morse_forest_journal
  PROPERTIES
    EXPORT_NAME direct_morse_forest_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_forest_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_forest_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_forest_journal
  PUBLIC
    morsehgp3d::direct_frozen_root_quotient
    morsehgp3d::direct_saddle_arm_seed_journal
    morsehgp3d::direct_sparse_facet_descent_closure
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_forest_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_forest_journal
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_morse_industrial_plan
  STATIC
    src/cpu/hierarchy/direct_morse_industrial_plan.cpp
)
add_library(
  morsehgp3d::direct_morse_industrial_plan
  ALIAS morsehgp3d_direct_morse_industrial_plan
)
set_target_properties(
  morsehgp3d_direct_morse_industrial_plan
  PROPERTIES
    EXPORT_NAME direct_morse_industrial_plan
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_industrial_plan PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_industrial_plan
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_industrial_plan
  PUBLIC morsehgp3d::direct_saddle_arm_seed_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_industrial_plan PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_industrial_plan
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_facet_descent_batch_plan
  STATIC
    src/cpu/hierarchy/direct_sparse_facet_descent_batch_plan.cpp
)
add_library(
  morsehgp3d::direct_sparse_facet_descent_batch_plan
  ALIAS morsehgp3d_direct_sparse_facet_descent_batch_plan
)
set_target_properties(
  morsehgp3d_direct_sparse_facet_descent_batch_plan
  PROPERTIES
    EXPORT_NAME direct_sparse_facet_descent_batch_plan
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_facet_descent_batch_plan PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_facet_descent_batch_plan
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_facet_descent_batch_plan
  PUBLIC morsehgp3d::direct_morse_industrial_plan
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_batch_plan PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_batch_plan
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_sparse_facet_descent_batch_executor
  STATIC
    src/cpu/hierarchy/direct_sparse_facet_descent_batch_executor.cpp
)
add_library(
  morsehgp3d::direct_sparse_facet_descent_batch_executor
  ALIAS morsehgp3d_direct_sparse_facet_descent_batch_executor
)
set_target_properties(
  morsehgp3d_direct_sparse_facet_descent_batch_executor
  PROPERTIES
    EXPORT_NAME direct_sparse_facet_descent_batch_executor
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_facet_descent_batch_executor PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_facet_descent_batch_executor
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_facet_descent_batch_executor
  PUBLIC
    morsehgp3d::direct_sparse_facet_descent_batch_plan
    morsehgp3d::direct_sparse_facet_descent_closure
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_batch_executor PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_facet_descent_batch_executor
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_pair_support_codec
  STATIC
    src/cpu/hierarchy/pair_support_stream_codec.cpp
)
add_library(
  morsehgp3d::pair_support_codec ALIAS morsehgp3d_pair_support_codec
)
set_target_properties(
  morsehgp3d_pair_support_codec
  PROPERTIES
    EXPORT_NAME pair_support_codec
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_pair_support_codec PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_pair_support_codec
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_pair_support_codec
  PUBLIC morsehgp3d::pair_support
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_pair_support_codec PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_pair_support_codec
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(UNIX)
  add_library(
    morsehgp3d_pair_support_durable
    STATIC
      src/cpu/hierarchy/pair_support_stream_durable.cpp
  )
  add_library(
    morsehgp3d::pair_support_durable ALIAS morsehgp3d_pair_support_durable
  )
  set_target_properties(
    morsehgp3d_pair_support_durable
    PROPERTIES
      EXPORT_NAME pair_support_durable
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(morsehgp3d_pair_support_durable PUBLIC cxx_std_20)
  target_include_directories(
    morsehgp3d_pair_support_durable
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_pair_support_durable
    PUBLIC morsehgp3d::pair_support_codec
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_pair_support_durable
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_hierarchy
  STATIC
    src/cpu/hierarchy/boruvka.cpp
    src/cpu/hierarchy/critical_arm.cpp
    src/cpu/hierarchy/critical_arm_gamma.cpp
    src/cpu/hierarchy/critical_catalog.cpp
    src/cpu/hierarchy/depth_zero_natural_support.cpp
    src/cpu/hierarchy/critical_catalog_arm_gamma_overlay.cpp
    src/cpu/hierarchy/critical_catalog_reduced_gamma_overlay.cpp
    src/cpu/hierarchy/critical_catalog_typed_gamma_journal.cpp
    src/cpu/hierarchy/critical_catalog_typed_gamma_durable_arm_key_catalog.cpp
    src/cpu/hierarchy/critical_catalog_typed_gamma_arm_root_composition.cpp
    src/cpu/hierarchy/critical_catalog_typed_gamma_arm_root_path_overlay.cpp
    src/cpu/hierarchy/critical_catalog_typed_gamma_root_overlay.cpp
    src/cpu/hierarchy/critical_event_gamma_overlay.cpp
    src/cpu/hierarchy/emst.cpp
    src/cpu/hierarchy/gamma.cpp
    src/cpu/hierarchy/gamma_transition.cpp
    src/cpu/hierarchy/morse_gamma_partition_sweep.cpp
    src/cpu/hierarchy/reduced_gamma_batch.cpp
    src/cpu/hierarchy/reduced_gamma_cut.cpp
    src/cpu/hierarchy/reduced_gamma_history.cpp
    src/cpu/hierarchy/gabriel.cpp
    src/cpu/hierarchy/k1_forest.cpp
    src/cpu/hierarchy/miniball.cpp
)
add_library(morsehgp3d::hierarchy ALIAS morsehgp3d_hierarchy)
set_target_properties(
  morsehgp3d_hierarchy
  PROPERTIES
    EXPORT_NAME hierarchy
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_hierarchy PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_hierarchy
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_hierarchy
  PUBLIC
    morsehgp3d::contract
    morsehgp3d::facet_miniball
    morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_hierarchy PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_hierarchy
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(MORSEHGP3D_ENABLE_CUDA)
  find_package(Threads REQUIRED)
  add_library(
    morsehgp3d_gpu_predicates
    STATIC
      src/cuda/phase2b_predicate_context_internal.hpp
      src/cuda/phase2b_predicate_context.cuh
      src/cuda/phase2b_distance_filter.cu
      src/cuda/phase2b_orientation_filter.cu
      src/cuda/phase2b_power_bisector_filter.cu
      src/gpu/predicate_filter.cpp
  )
  add_library(
    morsehgp3d::gpu_predicates
    ALIAS morsehgp3d_gpu_predicates
  )
  target_include_directories(
    morsehgp3d_gpu_predicates
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_predicates
    PUBLIC morsehgp3d::exact
    PRIVATE Threads::Threads
  )
  target_compile_features(morsehgp3d_gpu_predicates PUBLIC cxx_std_20)
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_predicates)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_predicates
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_predicates
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_predicate_replay
    src/tools/gpu_predicate_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_predicate_replay
    PRIVATE morsehgp3d::gpu_predicates
  )
  if(MSVC)
    target_compile_options(morsehgp3d_gpu_predicate_replay PRIVATE /W4 /WX)
  else()
    target_compile_options(
      morsehgp3d_gpu_predicate_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_predicate_context_harness
    src/tools/gpu_predicate_context_harness.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_predicate_context_harness
    PRIVATE morsehgp3d::gpu_predicates
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_predicate_context_harness PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_predicate_context_harness
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_spatial_reference
    STATIC
      src/cuda/phase4_spatial_reference_internal.hpp
      src/cuda/phase4_spatial_reference.cu
      src/gpu/spatial_reference.cpp
  )
  add_library(
    morsehgp3d::gpu_spatial_reference
    ALIAS morsehgp3d_gpu_spatial_reference
  )
  target_include_directories(
    morsehgp3d_gpu_spatial_reference
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_spatial_reference
    PUBLIC morsehgp3d::spatial
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_spatial_reference
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_spatial_reference)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_spatial_reference
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_spatial_reference
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_spatial_reference_replay
    src/tools/gpu_spatial_reference_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_spatial_reference_replay
    PRIVATE morsehgp3d::gpu_spatial_reference
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_spatial_reference_replay PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_spatial_reference_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_spatial_bounds
    STATIC
      src/cuda/phase4_spatial_bounds_internal.hpp
      src/cuda/phase4_spatial_bounds.cu
      src/gpu/spatial_bounds.cpp
      src/gpu/spatial_lbvh.cpp
  )
  add_library(
    morsehgp3d::gpu_spatial_bounds
    ALIAS morsehgp3d_gpu_spatial_bounds
  )
  add_library(
    morsehgp3d::gpu_spatial_lbvh
    ALIAS morsehgp3d_gpu_spatial_bounds
  )
  target_include_directories(
    morsehgp3d_gpu_spatial_bounds
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_spatial_bounds
    PUBLIC morsehgp3d::spatial
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_spatial_bounds
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_spatial_bounds)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_spatial_bounds
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_spatial_bounds
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_spatial_bounds_replay
    src/tools/gpu_spatial_bounds_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_spatial_bounds_replay
    PRIVATE morsehgp3d::gpu_spatial_bounds
  )
  if(MSVC)
    target_compile_options(morsehgp3d_gpu_spatial_bounds_replay PRIVATE /W4 /WX)
  else()
    target_compile_options(
      morsehgp3d_gpu_spatial_bounds_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_spatial_lbvh_replay
    src/tools/gpu_spatial_lbvh_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_spatial_lbvh_replay
    PRIVATE morsehgp3d::gpu_spatial_lbvh
  )
  if(MSVC)
    target_compile_options(morsehgp3d_gpu_spatial_lbvh_replay PRIVATE /W4 /WX)
  else()
    target_compile_options(
      morsehgp3d_gpu_spatial_lbvh_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_h_polytope_proposal
    STATIC
      src/cuda/phase7_h_polytope_proposal_internal.hpp
      src/cuda/phase7_h_polytope_proposal.cu
      src/gpu/rational_binary64_enclosure.hpp
      src/gpu/h_polytope_proposal.cpp
  )
  add_library(
    morsehgp3d::gpu_h_polytope_proposal
    ALIAS morsehgp3d_gpu_h_polytope_proposal
  )
  target_include_directories(
    morsehgp3d_gpu_h_polytope_proposal
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_h_polytope_proposal
    PUBLIC morsehgp3d::spatial
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_h_polytope_proposal
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_h_polytope_proposal)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_h_polytope_proposal
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_h_polytope_proposal
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_h_polytope_proposal_qualification
    src/tools/gpu_h_polytope_proposal_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_h_polytope_proposal_qualification
    PRIVATE morsehgp3d::gpu_h_polytope_proposal
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_h_polytope_proposal_qualification
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_h_polytope_proposal_qualification
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_pair_support_phi
    STATIC
      src/cuda/phase9_pair_support_phi_internal.hpp
      src/cuda/phase9_pair_support_phi.cu
      src/gpu/pair_support_phi.cpp
  )
  add_library(
    morsehgp3d::gpu_pair_support_phi
    ALIAS morsehgp3d_gpu_pair_support_phi
  )
  target_include_directories(
    morsehgp3d_gpu_pair_support_phi
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_pair_support_phi
    PUBLIC morsehgp3d::pair_support
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_pair_support_phi
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_pair_support_phi)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_pair_support_phi
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_pair_support_phi
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_pair_support_phi_qualification
    src/tools/gpu_pair_support_phi_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_pair_support_phi_qualification
    PRIVATE morsehgp3d::gpu_pair_support_phi
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_pair_support_phi_qualification PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_pair_support_phi_qualification
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_k1_boruvka
    STATIC
      src/cuda/phase5_k1_boruvka_internal.hpp
      src/cuda/phase5_k1_boruvka.cu
      src/gpu/k1_boruvka.cpp
      src/gpu/k1_boruvka_hybrid.cpp
      src/gpu/k1_boruvka_seeded_exact.cpp
      src/gpu/k1_boruvka_hierarchy.cpp
  )
  add_library(
    morsehgp3d::gpu_k1_boruvka
    ALIAS morsehgp3d_gpu_k1_boruvka
  )
  target_include_directories(
    morsehgp3d_gpu_k1_boruvka
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_k1_boruvka
    PUBLIC morsehgp3d::hierarchy
    PRIVATE Threads::Threads
  )
  target_compile_features(morsehgp3d_gpu_k1_boruvka PUBLIC cxx_std_20)
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_k1_boruvka)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_executable(
    morsehgp3d_gpu_k1_boruvka_replay
    src/tools/gpu_k1_boruvka_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_k1_boruvka_replay
    PRIVATE morsehgp3d::gpu_k1_boruvka
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_replay PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_k1_boruvka_full_replay
    src/tools/gpu_k1_boruvka_full_replay.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_k1_boruvka_full_replay
    PRIVATE morsehgp3d::gpu_k1_boruvka
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_full_replay PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_full_replay
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_k1_boruvka_morton_work_profile
    src/tools/gpu_k1_boruvka_morton_work_profile.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_k1_boruvka_morton_work_profile
    PRIVATE morsehgp3d::gpu_k1_boruvka
  )
  target_compile_definitions(
    morsehgp3d_gpu_k1_boruvka_morton_work_profile
    PRIVATE MORSEHGP3D_WORK_PROFILE_BACKEND=\"cuda_g4\"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_morton_work_profile PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_morton_work_profile
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile
    src/tools/gpu_k1_boruvka_exact_search_work_profile.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile
    PRIVATE morsehgp3d::gpu_k1_boruvka
  )
  target_compile_definitions(
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile
    PRIVATE MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND="cuda_g4"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_exact_search_work_profile PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_k1_boruvka_exact_search_work_profile
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()
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

  add_executable(
    morsehgp3d_direct_pair_support_smoke
    src/tools/direct_pair_support_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_direct_pair_support_smoke
    PRIVATE morsehgp3d::pair_support
  )
  morsehgp3d_apply_sanitizers(morsehgp3d_direct_pair_support_smoke)
  if(MSVC)
    target_compile_options(
      morsehgp3d_direct_pair_support_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_direct_pair_support_smoke
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
  TARGETS
    morsehgp3d_contract
    morsehgp3d_exact
    morsehgp3d_spatial
    morsehgp3d_facet_miniball
    morsehgp3d_pair_support
    morsehgp3d_higher_support
    morsehgp3d_direct_support_terminal
    morsehgp3d_direct_morse_event_journal
    morsehgp3d_direct_saddle_arm_seed_journal
    morsehgp3d_direct_closed_saddle_incidence_journal
    morsehgp3d_direct_sparse_positive_facet_locator
    morsehgp3d_direct_sparse_positive_facet_prefix_sweep
    morsehgp3d_direct_sparse_facet_descent_step
    morsehgp3d_direct_sparse_facet_descent_closure
    morsehgp3d_direct_sparse_first_incidence
    morsehgp3d_direct_sparse_gateway_candidate_journal
    morsehgp3d_direct_sparse_gateway_candidate_localization
    morsehgp3d_direct_sparse_gateway_clock
    morsehgp3d_direct_sparse_gateway_clock_authority
    morsehgp3d_direct_sparse_gateway_temporal_resolution
    morsehgp3d_direct_sparse_gateway_historical_quotient
    morsehgp3d_direct_frozen_root_quotient
    morsehgp3d_direct_k1_forest_journal
    morsehgp3d_direct_morse_forest_journal
    morsehgp3d_direct_morse_industrial_plan
    morsehgp3d_direct_sparse_facet_descent_batch_plan
    morsehgp3d_direct_sparse_facet_descent_batch_executor
    morsehgp3d_pair_support_codec
    morsehgp3d_hierarchy
    morsehgp3d_sanitizers
  EXPORT MorseHGP3DTargets
)
if(UNIX)
  install(
    TARGETS morsehgp3d_pair_support_durable
    EXPORT MorseHGP3DTargets
  )
endif()
if(MORSEHGP3D_BUILD_TOOLS)
  install(
    TARGETS
      morsehgp3d_replay_predicate
      morsehgp3d_direct_pair_support_smoke
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
  install(
    PROGRAMS tools/replay_predicate.py
    TYPE BIN
    RENAME morsehgp3d-replay-predicate
  )
endif()
install(
  DIRECTORY include/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  PATTERN "morsehgp3d/gpu" EXCLUDE
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
