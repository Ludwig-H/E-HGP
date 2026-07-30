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
  find_package(CUDAToolkit REQUIRED)

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
find_package(Threads REQUIRED)

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
  morsehgp3d_morton_triangular_block_pair_schedule
  STATIC
    src/cpu/hierarchy/morton_triangular_block_pair_schedule.cpp
)
add_library(
  morsehgp3d::morton_triangular_block_pair_schedule
  ALIAS morsehgp3d_morton_triangular_block_pair_schedule
)
set_target_properties(
  morsehgp3d_morton_triangular_block_pair_schedule
  PROPERTIES
    EXPORT_NAME morton_triangular_block_pair_schedule
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_morton_triangular_block_pair_schedule PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_morton_triangular_block_pair_schedule
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_morton_triangular_block_pair_schedule
  PUBLIC morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_morton_triangular_block_pair_schedule PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_morton_triangular_block_pair_schedule
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
    src/cpu/hierarchy/exact_block_rank_prune_receipt.cpp
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
  morsehgp3d_anchored_pair_witness_bank
  STATIC
    src/cpu/hierarchy/anchored_pair_witness_bank.cpp
    src/cpu/hierarchy/grouped_anchored_pair_certificate.cpp
    src/cpu/hierarchy/morton_grouped_anchored_pair_candidate_cursor.cpp
    src/cpu/hierarchy/morton_grouped_anchored_pair_schedule.cpp
)
add_library(
  morsehgp3d::anchored_pair_witness_bank
  ALIAS morsehgp3d_anchored_pair_witness_bank
)
set_target_properties(
  morsehgp3d_anchored_pair_witness_bank
  PROPERTIES
    EXPORT_NAME anchored_pair_witness_bank
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_anchored_pair_witness_bank PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_anchored_pair_witness_bank
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_anchored_pair_witness_bank
  PUBLIC
    morsehgp3d::pair_support
    morsehgp3d::morton_triangular_block_pair_schedule
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_anchored_pair_witness_bank PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_anchored_pair_witness_bank
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_anchored_pair_candidate_classifier
  STATIC
    src/cpu/hierarchy/anchored_pair_candidate_classifier.cpp
)
add_library(
  morsehgp3d::anchored_pair_candidate_classifier
  ALIAS morsehgp3d_anchored_pair_candidate_classifier
)
set_target_properties(
  morsehgp3d_anchored_pair_candidate_classifier
  PROPERTIES
    EXPORT_NAME anchored_pair_candidate_classifier
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_anchored_pair_candidate_classifier PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_anchored_pair_candidate_classifier
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_anchored_pair_candidate_classifier
  PUBLIC morsehgp3d::pair_support
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_classifier PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_classifier
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_sparse_anchored_pair_session
  STATIC
    src/cpu/hierarchy/sparse_anchored_pair_session.cpp
    src/cpu/hierarchy/symmetric_grouped_anchored_pair_prune_receipt.cpp
)
add_library(
  morsehgp3d::sparse_anchored_pair_session
  ALIAS morsehgp3d_sparse_anchored_pair_session
)
set_target_properties(
  morsehgp3d_sparse_anchored_pair_session
  PROPERTIES
    EXPORT_NAME sparse_anchored_pair_session
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_sparse_anchored_pair_session PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_sparse_anchored_pair_session
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_sparse_anchored_pair_session
  PUBLIC
    morsehgp3d::anchored_pair_candidate_classifier
    morsehgp3d::anchored_pair_witness_bank
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_session PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_session
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
    src/cpu/hierarchy/exact_low_order_gabriel_skeleton.cpp
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
    morsehgp3d::sparse_anchored_pair_session
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
  morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
  STATIC
    src/cpu/hierarchy/direct_sparse_facet_top_k_proposal_transcript.cpp
)
add_library(
  morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
  ALIAS morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
)
set_target_properties(
  morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
  PROPERTIES
    EXPORT_NAME direct_sparse_facet_top_k_proposal_transcript
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_sparse_facet_top_k_proposal_transcript PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
  PUBLIC
    morsehgp3d::direct_sparse_positive_facet_locator
    morsehgp3d::exact
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_sparse_facet_top_k_proposal_transcript PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
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
  morsehgp3d_relative_morse_boruvka
  STATIC
    src/cpu/hierarchy/relative_morse_boruvka.cpp
)
add_library(
  morsehgp3d::relative_morse_boruvka
  ALIAS morsehgp3d_relative_morse_boruvka
)
set_target_properties(
  morsehgp3d_relative_morse_boruvka
  PROPERTIES
    EXPORT_NAME relative_morse_boruvka
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_relative_morse_boruvka PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_relative_morse_boruvka
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_relative_morse_boruvka
  PUBLIC morsehgp3d::direct_frozen_root_quotient
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_relative_morse_boruvka PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_relative_morse_boruvka
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
  morsehgp3d_direct_morse_forest_source_authority
  STATIC
    src/cpu/hierarchy/direct_morse_forest_source_authority.cpp
    src/cpu/hierarchy/direct_morse_forest_source_wire.cpp
)
add_library(
  morsehgp3d::direct_morse_forest_source_authority
  ALIAS morsehgp3d_direct_morse_forest_source_authority
)
set_target_properties(
  morsehgp3d_direct_morse_forest_source_authority
  PROPERTIES
    EXPORT_NAME direct_morse_forest_source_authority
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_forest_source_authority PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_forest_source_authority
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_forest_source_authority
  PUBLIC morsehgp3d::direct_morse_forest_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_forest_source_authority PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_forest_source_authority
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_direct_morse_vertical_journal
  STATIC
    src/cpu/hierarchy/direct_morse_vertical_journal.cpp
)
add_library(
  morsehgp3d::direct_morse_vertical_journal
  ALIAS morsehgp3d_direct_morse_vertical_journal
)
set_target_properties(
  morsehgp3d_direct_morse_vertical_journal
  PROPERTIES
    EXPORT_NAME direct_morse_vertical_journal
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_vertical_journal PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_vertical_journal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_vertical_journal
  PUBLIC morsehgp3d::direct_morse_forest_journal
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_vertical_journal PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_vertical_journal
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
  morsehgp3d_direct_morse_budget
  STATIC
    src/cpu/hierarchy/direct_morse_budget.cpp
)
add_library(
  morsehgp3d::direct_morse_budget
  ALIAS morsehgp3d_direct_morse_budget
)
set_target_properties(
  morsehgp3d_direct_morse_budget
  PROPERTIES
    EXPORT_NAME direct_morse_budget
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_direct_morse_budget PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_direct_morse_budget
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_budget
  PUBLIC morsehgp3d::contract
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_direct_morse_budget PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_direct_morse_budget
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(UNIX)
  add_library(
    morsehgp3d_atomic_linear_run_store
    STATIC
      src/cpu/hierarchy/atomic_linear_run_store.cpp
  )
  add_library(
    morsehgp3d::atomic_linear_run_store
    ALIAS morsehgp3d_atomic_linear_run_store
  )
  set_target_properties(
    morsehgp3d_atomic_linear_run_store
    PROPERTIES
      EXPORT_NAME atomic_linear_run_store
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_atomic_linear_run_store PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_atomic_linear_run_store
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_atomic_linear_run_store
    PUBLIC morsehgp3d::contract
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_atomic_linear_run_store
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_library(
    morsehgp3d_sparse_anchored_pair_chunk_run
    STATIC
      src/cpu/hierarchy/sparse_anchored_pair_chunk_run.cpp
  )
  add_library(
    morsehgp3d::sparse_anchored_pair_chunk_run
    ALIAS morsehgp3d_sparse_anchored_pair_chunk_run
  )
  set_target_properties(
    morsehgp3d_sparse_anchored_pair_chunk_run
    PROPERTIES
      EXPORT_NAME sparse_anchored_pair_chunk_run
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_sparse_anchored_pair_chunk_run PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_sparse_anchored_pair_chunk_run
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_sparse_anchored_pair_chunk_run
    PUBLIC
      morsehgp3d::atomic_linear_run_store
      morsehgp3d::sparse_anchored_pair_session
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_chunk_run
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_library(
    morsehgp3d_sparse_anchored_pair_h0_run
    STATIC
      src/cpu/hierarchy/sparse_anchored_pair_h0_run.cpp
  )
  add_library(
    morsehgp3d::sparse_anchored_pair_h0_run
    ALIAS morsehgp3d_sparse_anchored_pair_h0_run
  )
  set_target_properties(
    morsehgp3d_sparse_anchored_pair_h0_run
    PROPERTIES
      EXPORT_NAME sparse_anchored_pair_h0_run
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_sparse_anchored_pair_h0_run PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_sparse_anchored_pair_h0_run
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_sparse_anchored_pair_h0_run
    PUBLIC morsehgp3d::sparse_anchored_pair_chunk_run
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_h0_run
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_library(
    morsehgp3d_sparse_direct_h0_candidate_merge
    STATIC
      src/cpu/hierarchy/sparse_direct_h0_candidate_merge.cpp
  )
  add_library(
    morsehgp3d::sparse_direct_h0_candidate_merge
    ALIAS morsehgp3d_sparse_direct_h0_candidate_merge
  )
  set_target_properties(
    morsehgp3d_sparse_direct_h0_candidate_merge
    PROPERTIES
      EXPORT_NAME sparse_direct_h0_candidate_merge
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_sparse_direct_h0_candidate_merge PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_sparse_direct_h0_candidate_merge
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_sparse_direct_h0_candidate_merge
    PUBLIC
      morsehgp3d::higher_support
      morsehgp3d::sparse_anchored_pair_h0_run
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_sparse_direct_h0_candidate_merge
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_library(
    morsehgp3d_sparse_higher_support_h0_chunk_run
    STATIC
      src/cpu/hierarchy/sparse_higher_support_h0_chunk_run.cpp
  )
  add_library(
    morsehgp3d::sparse_higher_support_h0_chunk_run
    ALIAS morsehgp3d_sparse_higher_support_h0_chunk_run
  )
  set_target_properties(
    morsehgp3d_sparse_higher_support_h0_chunk_run
    PROPERTIES
      EXPORT_NAME sparse_higher_support_h0_chunk_run
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_sparse_higher_support_h0_chunk_run PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_sparse_higher_support_h0_chunk_run
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_sparse_higher_support_h0_chunk_run
    PUBLIC
      morsehgp3d::atomic_linear_run_store
      morsehgp3d::sparse_direct_h0_candidate_merge
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_sparse_higher_support_h0_chunk_run
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_library(
    morsehgp3d_sparse_direct_h0_atomic_run_reader
    STATIC
      src/cpu/hierarchy/sparse_direct_h0_atomic_run_reader.cpp
  )
  add_library(
    morsehgp3d::sparse_direct_h0_atomic_run_reader
    ALIAS morsehgp3d_sparse_direct_h0_atomic_run_reader
  )
  set_target_properties(
    morsehgp3d_sparse_direct_h0_atomic_run_reader
    PROPERTIES
      EXPORT_NAME sparse_direct_h0_atomic_run_reader
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_sparse_direct_h0_atomic_run_reader PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_sparse_direct_h0_atomic_run_reader
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_sparse_direct_h0_atomic_run_reader
    PUBLIC morsehgp3d::sparse_direct_h0_candidate_merge
    PRIVATE morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_sparse_direct_h0_atomic_run_reader
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
  morsehgp3d_direct_morse_forest_reducer
  STATIC
    src/cpu/hierarchy/direct_morse_forest_reducer.cpp
)
add_library(
  morsehgp3d::direct_morse_forest_reducer
  ALIAS morsehgp3d_direct_morse_forest_reducer
)
set_target_properties(
  morsehgp3d_direct_morse_forest_reducer
  PROPERTIES
    EXPORT_NAME direct_morse_forest_reducer
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_direct_morse_forest_reducer PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_direct_morse_forest_reducer
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_direct_morse_forest_reducer
  PUBLIC
    morsehgp3d::direct_morse_forest_journal
    morsehgp3d::direct_morse_forest_source_authority
    morsehgp3d::direct_sparse_facet_descent_batch_executor
    morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_direct_morse_forest_reducer PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_direct_morse_forest_reducer
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(UNIX)
  add_library(
    morsehgp3d_direct_morse_chunk_run
    STATIC
      src/cpu/hierarchy/direct_morse_chunk_run.cpp
      src/cpu/hierarchy/direct_morse_durable_live_commit.cpp
  )
  add_library(
    morsehgp3d::direct_morse_chunk_run
    ALIAS morsehgp3d_direct_morse_chunk_run
  )
  set_target_properties(
    morsehgp3d_direct_morse_chunk_run
    PROPERTIES
      EXPORT_NAME direct_morse_chunk_run
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_features(
    morsehgp3d_direct_morse_chunk_run PUBLIC cxx_std_20
  )
  target_include_directories(
    morsehgp3d_direct_morse_chunk_run
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(
    morsehgp3d_direct_morse_chunk_run
    PUBLIC
      morsehgp3d::atomic_linear_run_store
      morsehgp3d::direct_morse_budget
      morsehgp3d::direct_morse_forest_reducer
      morsehgp3d::direct_sparse_facet_descent_batch_executor
    PRIVATE
      morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
      morsehgp3d::sanitizers
  )
  target_compile_options(
    morsehgp3d_direct_morse_chunk_run
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
  morsehgp3d_yao48_geometry
  STATIC
    src/cpu/hierarchy/yao48_cone.cpp
    src/cpu/hierarchy/yao48_rank_cutoff.cpp
)
add_library(morsehgp3d::yao48_geometry ALIAS morsehgp3d_yao48_geometry)
set_target_properties(
  morsehgp3d_yao48_geometry
  PROPERTIES
    EXPORT_NAME yao48_geometry
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(morsehgp3d_yao48_geometry PUBLIC cxx_std_20)
target_include_directories(
  morsehgp3d_yao48_geometry
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_yao48_geometry
  PUBLIC morsehgp3d::exact
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(morsehgp3d_yao48_geometry PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_yao48_geometry
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
    src/cpu/hierarchy/yao48_emst.cpp
    src/cpu/hierarchy/exact_lbvh_yao48_emst.cpp
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
    morsehgp3d::yao48_geometry
  PRIVATE morsehgp3d::sanitizers
)

add_library(
  morsehgp3d_yao48_ranked_pair_candidates_reference
  STATIC
    src/cpu/hierarchy/yao48_ranked_pair_candidates.cpp
)
add_library(
  morsehgp3d::yao48_ranked_pair_candidates_reference
  ALIAS morsehgp3d_yao48_ranked_pair_candidates_reference
)
set_target_properties(
  morsehgp3d_yao48_ranked_pair_candidates_reference
  PROPERTIES
    EXPORT_NAME yao48_ranked_pair_candidates_reference
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_yao48_ranked_pair_candidates_reference PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_yao48_ranked_pair_candidates_reference
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_yao48_ranked_pair_candidates_reference
  PUBLIC
    morsehgp3d::spatial
    morsehgp3d::yao48_geometry
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_yao48_ranked_pair_candidates_reference PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_yao48_ranked_pair_candidates_reference
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_library(
  morsehgp3d_exact_ranked_diametral_pair_catalog_reference
  STATIC
    src/cpu/hierarchy/exact_ranked_diametral_pair_catalog_reference.cpp
)
add_library(
  morsehgp3d::exact_ranked_diametral_pair_catalog_reference
  ALIAS morsehgp3d_exact_ranked_diametral_pair_catalog_reference
)
set_target_properties(
  morsehgp3d_exact_ranked_diametral_pair_catalog_reference
  PROPERTIES
    EXPORT_NAME exact_ranked_diametral_pair_catalog_reference
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(
  morsehgp3d_exact_ranked_diametral_pair_catalog_reference PUBLIC cxx_std_20
)
target_include_directories(
  morsehgp3d_exact_ranked_diametral_pair_catalog_reference
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(
  morsehgp3d_exact_ranked_diametral_pair_catalog_reference
  PUBLIC
    morsehgp3d::anchored_pair_candidate_classifier
    morsehgp3d::yao48_ranked_pair_candidates_reference
  PRIVATE morsehgp3d::sanitizers
)
if(MSVC)
  target_compile_options(
    morsehgp3d_exact_ranked_diametral_pair_catalog_reference PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_exact_ranked_diametral_pair_catalog_reference
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()
if(MSVC)
  target_compile_options(morsehgp3d_hierarchy PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_hierarchy
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Host/fake-only Phase 15 contract.  The archive intentionally leaves its
# device launcher unresolved; unit tests provide a hostile fake and the future
# CUDA target will provide the sole production definition.  This target makes
# no CUDA or scientific implementation claim.
add_library(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
  STATIC
    src/cuda/phase15_ranked_diametral_pair_catalog_internal.hpp
    src/gpu/ranked_diametral_pair_catalog.cpp
)
add_library(
  morsehgp3d::gpu_ranked_diametral_pair_catalog_host_contract
  ALIAS morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
)
target_include_directories(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
  PUBLIC morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_ranked_diametral_pair_catalog_host_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Exact host/fake executable specification for the fused Morton/Yao48 pair
# frontier.  It emits only leaf survivors or exact prune regions; owned LBVH
# subtrees are never forwarded as unfiltered pair work.
add_library(
  morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
  STATIC
    src/gpu/morton_yao48_pair_frontier.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_pair_frontier_host_reference
  ALIAS morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
  PUBLIC
    morsehgp3d::spatial
    morsehgp3d::yao48_geometry
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_pair_frontier_host_reference
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Bounded anchor-tile compositor for the exact host/fake Morton/Yao48
# frontier.  Fixed per-point caps make hostile work terminally censored with
# an explicit unresolved mass; this target contains no dense fallback.
add_library(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
  STATIC
    src/gpu/morton_yao48_tiled_pair_frontier.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_tiled_pair_frontier_host_reference
  ALIAS morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
  PUBLIC morsehgp3d::gpu_morton_yao48_pair_frontier_host_reference
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_host_reference
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Device-resident Phase 15 Morton/Yao48 anchor-tile contract.  The host-only
# archive intentionally leaves traversal adoption and the launcher unresolved;
# unit tests provide hostile fakes, while the CUDA target below provides the
# only production definitions.  Candidate and prune payloads remain resident.
add_library(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  STATIC
    src/cuda/phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp
    src/gpu/morton_yao48_device_tiled_pair_frontier.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  ALIAS
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  PUBLIC morsehgp3d::spatial
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_host_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Resident exact multi-order consumer for the device-tiled frontier.  The
# host-only target intentionally leaves the classifier launcher unresolved:
# unit tests provide the hostile fake below, while the CUDA target provides
# the sole production definition.  Candidate, prune and payload arrays never
# cross this contract to the host.
add_library(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
  STATIC
    src/cuda/phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp
    src/gpu/morton_yao48_ranked_pair_tile_classifier.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
  ALIAS
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
  PUBLIC
    morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier_host_contract
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Standalone Phase 15 radial-subtree filter contract.  It is intentionally
# not wired into the product frontier: CUDA proposes only the conservative
# minDist_down >= max(3*D_up) range cut and exact CPU replay is mandatory.
add_library(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
  STATIC
    src/cuda/phase15_morton_yao48_radial_subtree_filter_internal.hpp
    src/gpu/morton_yao48_radial_subtree_filter.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_radial_subtree_filter_host_contract
  ALIAS morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
  PUBLIC
    morsehgp3d::spatial
    morsehgp3d::yao48_geometry
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
  PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_host_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Benchmark-only host aggregation for bounded Morton-window Yao48 seed
# transcripts.  It preserves unresolved mass and never promotes a heuristic
# omission to an exact pair-frontier prune.
add_library(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_support
  STATIC
    src/gpu/morton_yao48_seed_work_profile.cpp
)
add_library(
  morsehgp3d::gpu_morton_yao48_seed_work_profile_support
  ALIAS morsehgp3d_gpu_morton_yao48_seed_work_profile_support
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_support
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_support
  PUBLIC morsehgp3d::spatial
  PRIVATE Threads::Threads morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_support PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_seed_work_profile_support PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_seed_work_profile_support
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

# Host/fake contract for the exact point predicate used by Phase 15. The
# launcher symbol is intentionally supplied by a unit-test fake in CPU builds;
# this target exercises validation, exact BigInt replay, fallback closure and
# publication poisoning without claiming CUDA execution.
add_library(
  morsehgp3d_gpu_exact_diametral_phi_host_contract
  STATIC
    src/cuda/phase15_exact_diametral_phi_fixed.cuh
    src/cuda/phase15_exact_diametral_phi_internal.hpp
    src/gpu/exact_diametral_phi.cpp
)
add_library(
  morsehgp3d::gpu_exact_diametral_phi_host_contract
  ALIAS morsehgp3d_gpu_exact_diametral_phi_host_contract
)
target_include_directories(
  morsehgp3d_gpu_exact_diametral_phi_host_contract
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_exact_diametral_phi_host_contract
  PUBLIC morsehgp3d::exact
  PRIVATE morsehgp3d::sanitizers
)
target_compile_features(
  morsehgp3d_gpu_exact_diametral_phi_host_contract PUBLIC cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_diametral_phi_host_contract PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_diametral_phi_host_contract
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(MORSEHGP3D_ENABLE_CUDA)
  find_package(Threads REQUIRED)
  add_library(
    morsehgp3d_gpu_exact_diametral_phi
    STATIC
      src/cuda/phase15_exact_diametral_phi_fixed.cuh
      src/cuda/phase15_exact_diametral_phi_internal.hpp
      src/cuda/phase15_exact_diametral_phi.cu
      src/gpu/exact_diametral_phi.cpp
  )
  add_library(
    morsehgp3d::gpu_exact_diametral_phi
    ALIAS morsehgp3d_gpu_exact_diametral_phi
  )
  target_include_directories(
    morsehgp3d_gpu_exact_diametral_phi
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_exact_diametral_phi
    PUBLIC morsehgp3d::exact
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_exact_diametral_phi PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(morsehgp3d_gpu_exact_diametral_phi)
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_exact_diametral_phi
      PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_exact_diametral_phi
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
    morsehgp3d_gpu_exact_diametral_phi_qualification
    src/tools/gpu_exact_diametral_phi_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_exact_diametral_phi_qualification
    PRIVATE morsehgp3d::gpu_exact_diametral_phi
  )
  target_compile_features(
    morsehgp3d_gpu_exact_diametral_phi_qualification PRIVATE cxx_std_20
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_exact_diametral_phi_qualification PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_exact_diametral_phi_qualification
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

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

  add_executable(
    morsehgp3d_gpu_pair_support_rank_search_component_smoke
    src/tools/gpu_pair_support_rank_search_component_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_pair_support_rank_search_component_smoke
    PRIVATE
      morsehgp3d::gpu_pair_support_phi
      morsehgp3d::pair_support
  )
  target_compile_definitions(
    morsehgp3d_gpu_pair_support_rank_search_component_smoke
    PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_pair_support_rank_search_component_smoke
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_pair_support_rank_search_component_smoke
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

  add_library(
    morsehgp3d_gpu_morton_lbvh_build
    STATIC
      src/cuda/phase14_morton_lbvh_build_internal.hpp
      src/cuda/phase14_morton_lbvh_build.cu
      src/gpu/morton_lbvh_build.cpp
  )
  add_library(
    morsehgp3d::gpu_morton_lbvh_build
    ALIAS morsehgp3d_gpu_morton_lbvh_build
  )
  target_include_directories(
    morsehgp3d_gpu_morton_lbvh_build
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_lbvh_build
    PUBLIC morsehgp3d::spatial
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_morton_lbvh_build PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_morton_lbvh_build
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_lbvh_build
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_lbvh_build
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_library(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter
    STATIC
      src/cuda/phase15_morton_yao48_radial_subtree_filter_internal.hpp
      src/cuda/phase15_morton_yao48_radial_subtree_filter.cu
      src/gpu/morton_yao48_radial_subtree_filter.cpp
  )
  add_library(
    morsehgp3d::gpu_morton_yao48_radial_subtree_filter
    ALIAS morsehgp3d_gpu_morton_yao48_radial_subtree_filter
  )
  target_include_directories(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter
    PUBLIC
      morsehgp3d::gpu_morton_lbvh_build
      morsehgp3d::spatial
      morsehgp3d::yao48_geometry
  )
  target_compile_features(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_radial_subtree_filter
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_radial_subtree_filter
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_library(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
    STATIC
      include/morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp
      src/cuda/phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp
      src/cuda/phase15_morton_yao48_device_tiled_pair_frontier.cu
      src/gpu/morton_yao48_device_tiled_pair_frontier.cpp
  )
  add_library(
    morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier
    ALIAS morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
  )
  target_include_directories(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
    PUBLIC
      morsehgp3d::gpu_morton_lbvh_build
      morsehgp3d::spatial
  )
  target_compile_features(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    )
  endif()

  add_library(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
    STATIC
      include/morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp
      src/cuda/phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp
      src/cuda/phase15_morton_yao48_ranked_pair_tile_classifier.cu
      src/gpu/morton_yao48_ranked_pair_tile_classifier.cpp
  )
  add_library(
    morsehgp3d::gpu_morton_yao48_ranked_pair_tile_classifier
    ALIAS morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
  )
  target_include_directories(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
    PUBLIC
      morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier
      morsehgp3d::spatial
  )
  target_compile_features(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
    PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier
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
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
    src/tools/gpu_morton_yao48_device_tiled_pair_frontier_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
    PRIVATE
      morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier
      CUDA::cudart
  )
  target_compile_features(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
    PRIVATE cxx_std_20
  )
  target_compile_definitions(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
    PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
    src/tools/gpu_morton_yao48_radial_subtree_filter_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
    PRIVATE morsehgp3d::gpu_morton_yao48_radial_subtree_filter
  )
  target_compile_features(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
    PRIVATE cxx_std_20
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_binary64_lbvh_top_k
    STATIC
      include/morsehgp3d/gpu/binary64_lbvh_top_k.hpp
      src/cuda/phase14_binary64_lbvh_top_k_internal.hpp
      src/cuda/phase14_binary64_lbvh_top_k.cu
      src/gpu/binary64_lbvh_top_k.cpp
  )
  add_library(
    morsehgp3d::gpu_binary64_lbvh_top_k
    ALIAS morsehgp3d_gpu_binary64_lbvh_top_k
  )
  target_include_directories(
    morsehgp3d_gpu_binary64_lbvh_top_k
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_binary64_lbvh_top_k
    PUBLIC morsehgp3d::gpu_morton_lbvh_build
  )
  target_compile_features(
    morsehgp3d_gpu_binary64_lbvh_top_k PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_binary64_lbvh_top_k
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_binary64_lbvh_top_k
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_binary64_lbvh_top_k
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
    morsehgp3d_gpu_morton_lbvh_build_component_smoke
    src/tools/gpu_morton_lbvh_build_component_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_morton_lbvh_build_component_smoke
    PRIVATE morsehgp3d::gpu_morton_lbvh_build
  )
  target_compile_definitions(
    morsehgp3d_gpu_morton_lbvh_build_component_smoke
    PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_morton_lbvh_build_component_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_morton_lbvh_build_component_smoke
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
      -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_anchored_pair_candidate_proposal
    STATIC
      src/cuda/phase14_anchored_pair_candidate_proposal_internal.hpp
      src/cuda/phase14_anchored_pair_candidate_proposal.cu
      src/gpu/anchored_pair_candidate_proposal.cpp
      src/cpu/hierarchy/anchored_pair_candidate_recertifier.cpp
  )
  add_library(
    morsehgp3d::gpu_anchored_pair_candidate_proposal
    ALIAS morsehgp3d_gpu_anchored_pair_candidate_proposal
  )
  target_include_directories(
    morsehgp3d_gpu_anchored_pair_candidate_proposal
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_anchored_pair_candidate_proposal
    PUBLIC morsehgp3d::gpu_morton_lbvh_build
    PRIVATE
      morsehgp3d::anchored_pair_witness_bank
      Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_anchored_pair_candidate_proposal PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_anchored_pair_candidate_proposal
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_anchored_pair_candidate_proposal
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_anchored_pair_candidate_proposal
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
    morsehgp3d_gpu_anchored_pair_candidate_proposal_component_smoke
    src/tools/gpu_anchored_pair_candidate_proposal_component_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_anchored_pair_candidate_proposal_component_smoke
    PRIVATE morsehgp3d::gpu_anchored_pair_candidate_proposal
  )
  target_compile_definitions(
    morsehgp3d_gpu_anchored_pair_candidate_proposal_component_smoke
    PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_anchored_pair_candidate_proposal_component_smoke
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_anchored_pair_candidate_proposal_component_smoke
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Werror
    )
  endif()

  add_library(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
    STATIC
      src/cuda/phase14_facet_top_k_proposal_internal.hpp
      src/cuda/phase14_facet_top_k_proposal.cu
      src/gpu/exact_center_binary64_projection.hpp
      src/gpu/rational_binary64_enclosure.hpp
      src/gpu/direct_sparse_facet_top_k_proposal.cpp
      src/gpu/direct_sparse_facet_top_k_integrated_adapter.cpp
  )
  add_library(
    morsehgp3d::gpu_direct_sparse_facet_top_k_proposal
    ALIAS morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
  )
  target_include_directories(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
  )
  target_link_libraries(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
    PUBLIC
      morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
      morsehgp3d::gpu_morton_lbvh_build
      morsehgp3d::spatial
    PRIVATE Threads::Threads
  )
  target_compile_features(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal PUBLIC cxx_std_20
  )
  morsehgp3d_configure_cuda_target(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_direct_sparse_facet_top_k_proposal
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
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification
    src/tools/gpu_direct_sparse_facet_top_k_proposal_qualification.cpp
  )
  target_link_libraries(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification
    PRIVATE
      morsehgp3d::gpu_direct_sparse_facet_top_k_proposal
      morsehgp3d::facet_miniball
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification
      PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Werror
    )
  endif()

  if(MORSEHGP3D_BUILD_TOOLS)
    add_executable(
      morsehgp3d_gpu_morton_yao48_seed_work_profile
      src/cuda/phase14_morton_window_knn_internal.hpp
      src/cuda/phase14_morton_window_knn.cu
      src/tools/gpu_morton_yao48_seed_work_profile.cpp
    )
    target_include_directories(
      morsehgp3d_gpu_morton_yao48_seed_work_profile
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
    )
    target_link_libraries(
      morsehgp3d_gpu_morton_yao48_seed_work_profile
      PRIVATE
        morsehgp3d::gpu_morton_lbvh_build
        morsehgp3d::gpu_morton_yao48_seed_work_profile_support
        Threads::Threads
    )
    target_compile_definitions(
      morsehgp3d_gpu_morton_yao48_seed_work_profile
      PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
    )
    morsehgp3d_configure_cuda_target(
      morsehgp3d_gpu_morton_yao48_seed_work_profile
    )
    if(MSVC)
      target_compile_options(
        morsehgp3d_gpu_morton_yao48_seed_work_profile PRIVATE /W4 /WX
      )
    else()
      target_compile_options(
        morsehgp3d_gpu_morton_yao48_seed_work_profile
        PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:-Wall>
          $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
          $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
          $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
          $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
          $<$<COMPILE_LANGUAGE:CXX>:-Werror>
      )
    endif()
  endif()
endif()

if(MORSEHGP3D_BUILD_TOOLS)
  add_executable(
    morsehgp3d_exact_lbvh_yao48_emst_smoke
    src/tools/exact_lbvh_yao48_emst_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_exact_lbvh_yao48_emst_smoke
    PRIVATE morsehgp3d::hierarchy
  )
  morsehgp3d_apply_sanitizers(morsehgp3d_exact_lbvh_yao48_emst_smoke)
  if(MSVC)
    target_compile_options(
      morsehgp3d_exact_lbvh_yao48_emst_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_exact_lbvh_yao48_emst_smoke
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

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

  add_executable(
    morsehgp3d_anchored_pair_witness_bank_smoke
    src/tools/anchored_pair_witness_bank_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_anchored_pair_witness_bank_smoke
    PRIVATE morsehgp3d::anchored_pair_witness_bank
  )
  morsehgp3d_apply_sanitizers(
    morsehgp3d_anchored_pair_witness_bank_smoke
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_anchored_pair_witness_bank_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_anchored_pair_witness_bank_smoke
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_anchored_pair_candidate_pipeline_smoke
    src/tools/anchored_pair_candidate_pipeline_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_anchored_pair_candidate_pipeline_smoke
    PRIVATE
      morsehgp3d::anchored_pair_witness_bank
      morsehgp3d::anchored_pair_candidate_classifier
  )
  morsehgp3d_apply_sanitizers(
    morsehgp3d_anchored_pair_candidate_pipeline_smoke
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_anchored_pair_candidate_pipeline_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_anchored_pair_candidate_pipeline_smoke
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_direct_morse_product_runner
    src/tools/direct_morse_product_runner.cpp
  )
  target_link_libraries(
    morsehgp3d_direct_morse_product_runner
    PRIVATE
      morsehgp3d::sparse_anchored_pair_session
      morsehgp3d::higher_support
      morsehgp3d::direct_support_terminal
      morsehgp3d::direct_morse_forest_reducer
  )
  morsehgp3d_apply_sanitizers(morsehgp3d_direct_morse_product_runner)
  if(MSVC)
    target_compile_options(
      morsehgp3d_direct_morse_product_runner PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_direct_morse_product_runner
      PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
  endif()

  add_executable(
    morsehgp3d_massive_sparse_pair_prefix_smoke
    src/tools/massive_sparse_pair_prefix_smoke.cpp
  )
  target_link_libraries(
    morsehgp3d_massive_sparse_pair_prefix_smoke
    PRIVATE morsehgp3d::sparse_anchored_pair_session
  )
  target_compile_definitions(
    morsehgp3d_massive_sparse_pair_prefix_smoke
    PRIVATE
      MORSEHGP3D_MASSIVE_PREFIX_HAS_CUDA=$<BOOL:${MORSEHGP3D_ENABLE_CUDA}>
  )
  if(MORSEHGP3D_ENABLE_CUDA)
    target_link_libraries(
      morsehgp3d_massive_sparse_pair_prefix_smoke
      PRIVATE morsehgp3d::gpu_morton_lbvh_build
    )
    target_compile_definitions(
      morsehgp3d_massive_sparse_pair_prefix_smoke
      PRIVATE MORSEHGP3D_GIT_SHA="${MORSEHGP3D_PHASE3_GIT_SHA}"
    )
  else()
    target_compile_definitions(
      morsehgp3d_massive_sparse_pair_prefix_smoke
      PRIVATE MORSEHGP3D_GIT_SHA="unavailable"
    )
  endif()
  morsehgp3d_apply_sanitizers(
    morsehgp3d_massive_sparse_pair_prefix_smoke
  )
  if(MSVC)
    target_compile_options(
      morsehgp3d_massive_sparse_pair_prefix_smoke PRIVATE /W4 /WX
    )
  else()
    target_compile_options(
      morsehgp3d_massive_sparse_pair_prefix_smoke
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
    morsehgp3d_morton_triangular_block_pair_schedule
    morsehgp3d_facet_miniball
    morsehgp3d_pair_support
    morsehgp3d_anchored_pair_witness_bank
    morsehgp3d_anchored_pair_candidate_classifier
    morsehgp3d_sparse_anchored_pair_session
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
    morsehgp3d_relative_morse_boruvka
    morsehgp3d_direct_k1_forest_journal
    morsehgp3d_direct_morse_forest_journal
    morsehgp3d_direct_morse_forest_source_authority
    morsehgp3d_direct_morse_forest_reducer
    morsehgp3d_direct_morse_vertical_journal
    morsehgp3d_direct_morse_industrial_plan
    morsehgp3d_direct_morse_budget
    morsehgp3d_direct_sparse_facet_descent_batch_plan
    morsehgp3d_direct_sparse_facet_descent_batch_executor
    morsehgp3d_direct_sparse_facet_top_k_proposal_transcript
    morsehgp3d_pair_support_codec
    morsehgp3d_yao48_geometry
    morsehgp3d_hierarchy
    morsehgp3d_yao48_ranked_pair_candidates_reference
    morsehgp3d_exact_ranked_diametral_pair_catalog_reference
    morsehgp3d_sanitizers
  EXPORT MorseHGP3DTargets
)
if(UNIX)
  install(
    TARGETS
      morsehgp3d_pair_support_durable
      morsehgp3d_atomic_linear_run_store
      morsehgp3d_sparse_anchored_pair_chunk_run
      morsehgp3d_sparse_anchored_pair_h0_run
      morsehgp3d_sparse_direct_h0_candidate_merge
      morsehgp3d_sparse_higher_support_h0_chunk_run
      morsehgp3d_sparse_direct_h0_atomic_run_reader
      morsehgp3d_direct_morse_chunk_run
    EXPORT MorseHGP3DTargets
  )
endif()
if(MORSEHGP3D_BUILD_TOOLS)
  install(
    TARGETS
      morsehgp3d_replay_predicate
      morsehgp3d_direct_pair_support_smoke
      morsehgp3d_anchored_pair_witness_bank_smoke
      morsehgp3d_anchored_pair_candidate_pipeline_smoke
      morsehgp3d_direct_morse_product_runner
      morsehgp3d_massive_sparse_pair_prefix_smoke
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
