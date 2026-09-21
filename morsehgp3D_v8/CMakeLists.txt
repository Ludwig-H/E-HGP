cmake_minimum_required(VERSION 3.20)
project(morsehgp3D_v8 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(CTest)
find_package(Threads REQUIRED)
option(MHGP8_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
set(MHGP8_PATCHWORK_SOURCE_DIR "" CACHE PATH
  "Pinned Patchwork++ source tree (upstream commit 3e6903a1d5537a4cc2ace897b0bbb98a92d6014c, fetched offline by bench/build_patchwork_ground.py); empty = bench/patchwork_ground_probe.cpp is not built")
set(MHGP8_EIGEN_INCLUDE_DIR "/usr/include/eigen3" CACHE PATH
  "Eigen include tree used by the Patchwork++ ground probe")

# Boost (cpp_int / rational) is used only by the independent judges
# oracle/p0_oracle.hpp and tests/exact_ball_oracle.hpp. Every gate or probe
# that includes one of them (transitively) is registered only when Boost is
# found; the engine itself and the binary32 lane never depend on Boost.
find_package(Boost QUIET)
if(Boost_FOUND)
  message(STATUS "mhgp8: Boost ${Boost_VERSION} found in ${Boost_INCLUDE_DIRS}; exact judges enabled")
else()
  message(STATUS "mhgp8: Boost not found; the exact cpp_int/rational judges are NOT registered "
    "(25 C++ gates, 5 q4/q34 probes, the q34 spatial gate and the q34/q4 mutation scripts). "
    "Configure with -DBOOST_ROOT=<prefix> to enable them.")
endif()
function(mhgp8_use_boost target)
  target_include_directories(${target} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
endfunction()

add_library(mhgp8_p0 STATIC src/pipeline/local_credits.cpp src/pipeline/axis_q2.cpp
  src/pipeline/q2_census.cpp src/pipeline/q2_census_parallel.cpp
  src/pipeline/prepared_cloud.cpp src/wspd/front.cpp src/lanes/q4_family.cpp
  src/lanes/exact_ball.cpp src/lanes/q34_seed.cpp
  src/lanes/edge_cover.cpp src/lanes/q34_cover.cpp
  src/lanes/family_certificate.cpp src/lanes/q34_pruning.cpp
  src/lanes/q34_collective.cpp src/lanes/q4_positive_domain.cpp
  src/lanes/q4_center_map.cpp src/lanes/q4_local_partition.cpp
  src/lanes/q4_local.cpp src/lanes/q4_shallow_set.cpp src/lanes/q4_shallow.cpp
  src/lanes/q4_window.cpp src/pipeline/wspd_q34.cpp src/lanes/q34_witness_search.cpp src/lanes/q3_ball_census.cpp)
target_include_directories(mhgp8_p0 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(mhgp8_p0 PUBLIC cxx_std_20)
set_target_properties(mhgp8_p0 PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(mhgp8_p0 PUBLIC -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(mhgp8_p0 PUBLIC Threads::Threads)
if(MHGP8_SANITIZE)
  target_compile_options(mhgp8_p0 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(mhgp8_p0 PUBLIC -fsanitize=address,undefined)
endif()

# Binary32 lane (float32 balls, keys, q4 events, q3 block/census, spatial
# index). src/core/float32_predicates.hpp refuses __FAST_MATH__ and documents
# that its interval filters are only valid without contraction; the options
# are PUBLIC so every probe linked to the lane inherits the exact flags that
# bench/run_float32_*_checks.py pass on their g++ lines
# (-ffp-contract=off -fno-fast-math -frounding-math). The lane does not
# include any mhgp8_p0 header and stays independent of it.
add_library(mhgp8_f32 STATIC src/core/float32_ball.cpp src/core/float32_ball_key.cpp
  src/core/float32_q3_block.cpp src/core/float32_q4_events.cpp
  src/lanes/float32_q3_census.cpp src/spatial/float32_index.cpp)
target_include_directories(mhgp8_f32 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(mhgp8_f32 PUBLIC cxx_std_20)
set_target_properties(mhgp8_f32 PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(mhgp8_f32 PUBLIC -Wall -Wextra -Wpedantic -Werror
  -ffp-contract=off -fno-fast-math -frounding-math)
target_link_libraries(mhgp8_f32 PUBLIC Threads::Threads)
if(MHGP8_SANITIZE)
  target_compile_options(mhgp8_f32 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(mhgp8_f32 PUBLIC -fsanitize=address,undefined)
endif()

add_executable(mhgp8_p0_probe bench/p0_probe.cpp)
target_link_libraries(mhgp8_p0_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_batch_probe bench/batch_probe.cpp)
target_link_libraries(mhgp8_batch_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_axis_probe bench/axis_probe.cpp)
target_link_libraries(mhgp8_axis_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_additive_probe bench/additive_probe.cpp)
target_link_libraries(mhgp8_additive_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q2_census_probe bench/q2_census_probe.cpp)
target_link_libraries(mhgp8_q2_census_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_cloud_reuse_probe bench/cloud_reuse_probe.cpp)
target_link_libraries(mhgp8_cloud_reuse_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_front_probe bench/wspd_front_probe.cpp)
target_link_libraries(mhgp8_wspd_front_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_census_probe bench/wspd_q2_census_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_census_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_parallel_probe bench/wspd_q2_parallel_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_parallel_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_dynamic_probe bench/wspd_q2_dynamic_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_dynamic_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q2_census_resume_probe bench/q2_census_resume_probe.cpp)
target_link_libraries(mhgp8_q2_census_resume_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q2_census_split_probe bench/q2_census_split_probe.cpp)
target_link_libraries(mhgp8_q2_census_split_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_cooperative_probe bench/wspd_q2_cooperative_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_cooperative_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_ranges_probe bench/wspd_q2_ranges_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_ranges_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_batched_probe bench/wspd_q2_batched_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_batched_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_proposals_probe bench/wspd_q2_proposals_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_proposals_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_wspd_q2_inheritance_probe bench/wspd_q2_inheritance_probe.cpp)
target_link_libraries(mhgp8_wspd_q2_inheritance_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q4_family_probe bench/q4_family_probe.cpp)
target_link_libraries(mhgp8_q4_family_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q34_seed_probe bench/q34_seed_probe.cpp)
target_link_libraries(mhgp8_q34_seed_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q34_cover_probe bench/q34_cover_probe.cpp)
target_link_libraries(mhgp8_q34_cover_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q34_pruning_probe bench/q34_pruning_probe.cpp)
target_link_libraries(mhgp8_q34_pruning_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q34_collective_probe bench/q34_collective_probe.cpp)
target_link_libraries(mhgp8_q34_collective_probe PRIVATE mhgp8_p0)
add_executable(mhgp8_q4_center_map_probe bench/q4_center_map_probe.cpp)
target_link_libraries(mhgp8_q4_center_map_probe PRIVATE mhgp8_p0)
if(Boost_FOUND)
  # These probes include tests/exact_ball_oracle.hpp (cpp_int / rational).
  foreach(probe IN ITEMS q4_local q4_shallow q4_window q4_lidar wspd_q34)
    add_executable(mhgp8_${probe}_probe bench/${probe}_probe.cpp)
    target_link_libraries(mhgp8_${probe}_probe PRIVATE mhgp8_p0)
    mhgp8_use_boost(mhgp8_${probe}_probe)
  endforeach()
endif()
add_executable(mhgp8_float32_index_bench bench/float32_index_probe.cpp)
target_link_libraries(mhgp8_float32_index_bench PRIVATE mhgp8_f32)

# Patchwork++ ground-removal probe: depends on six pinned upstream files that
# are not in this repository (bench/build_patchwork_ground.py fetches them
# offline and records their SHA-256). Built only when the caller points
# MHGP8_PATCHWORK_SOURCE_DIR at such a tree.
if(MHGP8_PATCHWORK_SOURCE_DIR)
  set(mhgp8_patchwork_units
    "${MHGP8_PATCHWORK_SOURCE_DIR}/cpp/patchworkpp/src/patchworkpp.cpp"
    "${MHGP8_PATCHWORK_SOURCE_DIR}/cpp/common/src/plane_fit.cpp")
  foreach(relative IN ITEMS cpp/patchworkpp/src/patchworkpp.cpp cpp/common/src/plane_fit.cpp
      cpp/patchworkpp/include/patchwork/patchworkpp.h cpp/common/include/patchwork/plane_fit.h
      cpp/common/include/patchwork/types.h)
    if(NOT EXISTS "${MHGP8_PATCHWORK_SOURCE_DIR}/${relative}")
      message(FATAL_ERROR "mhgp8: MHGP8_PATCHWORK_SOURCE_DIR lacks ${relative}")
    endif()
  endforeach()
  if(NOT EXISTS "${MHGP8_EIGEN_INCLUDE_DIR}/Eigen/Core")
    message(FATAL_ERROR "mhgp8: MHGP8_EIGEN_INCLUDE_DIR lacks Eigen/Core")
  endif()
  add_executable(mhgp8_patchwork_ground_probe bench/patchwork_ground_probe.cpp ${mhgp8_patchwork_units})
  target_compile_features(mhgp8_patchwork_ground_probe PRIVATE cxx_std_20)
  set_target_properties(mhgp8_patchwork_ground_probe PROPERTIES CXX_EXTENSIONS OFF)
  # Same flags as bench/build_patchwork_ground.py: -Werror only on our wrapper,
  # third-party warnings are visible but not fatal.
  target_compile_options(mhgp8_patchwork_ground_probe PRIVATE -Wall -Wextra -Wpedantic
    -ffp-contract=off -fno-fast-math)
  set_source_files_properties(bench/patchwork_ground_probe.cpp PROPERTIES COMPILE_OPTIONS "-Werror")
  target_compile_definitions(mhgp8_patchwork_ground_probe PRIVATE EIGEN_DONT_PARALLELIZE
    MHGP8_PATCHWORK_COMMIT="3e6903a1d5537a4cc2ace897b0bbb98a92d6014c")
  target_include_directories(mhgp8_patchwork_ground_probe SYSTEM PRIVATE
    "${MHGP8_EIGEN_INCLUDE_DIR}"
    "${MHGP8_PATCHWORK_SOURCE_DIR}/cpp/patchworkpp/include"
    "${MHGP8_PATCHWORK_SOURCE_DIR}/cpp/common/include")
  target_link_libraries(mhgp8_patchwork_ground_probe PRIVATE Threads::Threads)
  message(STATUS "mhgp8: Patchwork++ ground probe enabled from ${MHGP8_PATCHWORK_SOURCE_DIR}")
else()
  message(STATUS "mhgp8: MHGP8_PATCHWORK_SOURCE_DIR is empty; bench/patchwork_ground_probe.cpp is not built "
    "(obtain the pinned upstream sources offline with bench/build_patchwork_ground.py, then pass the tree)")
endif()

if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  # Scratch area for gates that require absent --output/--build directories.
  set(MHGP8_FRESH_ROOT "${CMAKE_BINARY_DIR}/mhgp8_fresh")
  # add_test through cmake/fresh_run.cmake: FRESH directories are removed
  # before COMMAND runs; the command's exit code is propagated.
  function(mhgp8_add_fresh_test name)
    cmake_parse_arguments(ARG "" "" "FRESH;COMMAND" ${ARGN})
    add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
      "-DMHGP8_FRESH=${ARG_FRESH}" "-DMHGP8_COMMAND=${ARG_COMMAND}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fresh_run.cmake")
  endfunction()
  # Append labels to the listed tests (a test may carry several labels).
  function(mhgp8_label labels)
    foreach(test IN LISTS ARGN)
      set_property(TEST ${test} APPEND PROPERTY LABELS ${labels})
    endforeach()
  endfunction()

  # ---------------------------------------------------------------- u16 engine
  # C++ gates that do not include a Boost judge.
  foreach(gate IN ITEMS q2_node_pool q2_terminal_pool wspd_q2_parallel wspd_q2_dynamic
      q2_census_resume q2_census_detach q2_census_parallel wspd_q2_cooperative wspd_q2_ranges
      wspd_q2_batched wspd_q2_proposals wspd_front_inheritance q2_joint q2_witness_order
      q2_sibling wspd_q2_census cloud_owner batch plan_assignment)
    add_executable(mhgp8_${gate}_gate tests/${gate}_gate.cpp)
    target_include_directories(mhgp8_${gate}_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(mhgp8_${gate}_gate PRIVATE mhgp8_p0)
    add_test(NAME mhgp8_${gate}_gate COMMAND mhgp8_${gate}_gate --selftest)
    mhgp8_label("u16;gate" mhgp8_${gate}_gate)
  endforeach()
  set_tests_properties(mhgp8_wspd_q2_dynamic_gate PROPERTIES TIMEOUT 60)
  # C++ gates whose judge is oracle/p0_oracle.hpp or tests/exact_ball_oracle.hpp
  # (Boost cpp_int / rational, verified by preprocessing every gate with -M).
  if(Boost_FOUND)
    foreach(gate IN ITEMS p0 tube axis_q2 axis_additive q2_census q2_joint_bounds
        q2_prepared_bounds q34_pair_bounds wspd_front wspd_front_jobs wspd_front_dispatch
        exact_ball q4_family q34_seed q34_cover q34_family_pruning q34_collective
        q4_center_map q4_local q4_shallow q4_window wspd_q34 q34_witness_search
        q3_ball_census q4_seed_cells)
      add_executable(mhgp8_${gate}_gate tests/${gate}_gate.cpp)
      target_include_directories(mhgp8_${gate}_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
      mhgp8_use_boost(mhgp8_${gate}_gate)
      target_link_libraries(mhgp8_${gate}_gate PRIVATE mhgp8_p0)
      add_test(NAME mhgp8_${gate}_gate COMMAND mhgp8_${gate}_gate --selftest)
      mhgp8_label("u16;gate" mhgp8_${gate}_gate)
    endforeach()
    set_tests_properties(mhgp8_wspd_front_dispatch_gate PROPERTIES TIMEOUT 60)
  endif()
  # Python gates of the u16 engine (each in double: normal and python3 -O).
  foreach(opt IN ITEMS normal optimized)
    set(batched_python_flags -B)
    if(opt STREQUAL "optimized")
      list(APPEND batched_python_flags -O)
    endif()
    add_test(NAME mhgp8_wspd_q2_batched_receipts_${opt}_gate
      COMMAND ${Python3_EXECUTABLE} ${batched_python_flags}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_batched_receipts_gate.py"
      --probe $<TARGET_FILE:mhgp8_wspd_q2_batched_probe> --selftest)
  endforeach()
  foreach(opt IN ITEMS normal optimized)
    set(proposals_python_flags -B)
    if(opt STREQUAL "optimized")
      list(APPEND proposals_python_flags -O)
    endif()
    add_test(NAME mhgp8_wspd_q2_proposals_receipts_${opt}_gate
      COMMAND ${Python3_EXECUTABLE} ${proposals_python_flags}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_proposals_receipts_gate.py"
      --probe $<TARGET_FILE:mhgp8_wspd_q2_proposals_probe>
      --reference $<TARGET_FILE:mhgp8_wspd_q2_parallel_probe> --selftest)
  endforeach()
  foreach(opt IN ITEMS normal optimized)
    set(inheritance_python_flags -B)
    if(opt STREQUAL "optimized")
      list(APPEND inheritance_python_flags -O)
    endif()
    add_test(NAME mhgp8_wspd_q2_inheritance_receipts_${opt}_gate
      COMMAND ${Python3_EXECUTABLE} ${inheritance_python_flags}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_inheritance_receipts_gate.py"
      --probe $<TARGET_FILE:mhgp8_wspd_q2_inheritance_probe>
      --reference $<TARGET_FILE:mhgp8_wspd_q2_parallel_probe>
      --proposals-probe $<TARGET_FILE:mhgp8_wspd_q2_proposals_probe> --selftest)
  endforeach()
  foreach(opt IN ITEMS normal optimized)
    set(ranges_python_flags -B)
    if(opt STREQUAL "optimized")
      list(APPEND ranges_python_flags -O)
    endif()
    add_test(NAME mhgp8_wspd_q2_ranges_receipts_${opt}_gate
      COMMAND ${Python3_EXECUTABLE} ${ranges_python_flags}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_ranges_receipts_gate.py"
      --probe $<TARGET_FILE:mhgp8_wspd_q2_ranges_probe> --selftest)
  endforeach()
  foreach(opt IN ITEMS normal optimized)
    set(cooperative_python_flags -B)
    if(opt STREQUAL "optimized")
      list(APPEND cooperative_python_flags -O)
    endif()
    add_test(NAME mhgp8_wspd_q2_cooperative_receipts_${opt}_gate
      COMMAND ${Python3_EXECUTABLE} ${cooperative_python_flags}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_cooperative_receipts_gate.py"
      --probe $<TARGET_FILE:mhgp8_wspd_q2_cooperative_probe> --selftest)
  endforeach()
  add_test(NAME mhgp8_q2_split_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_split_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_split_probe> --selftest)
  add_test(NAME mhgp8_q2_split_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_split_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_split_probe> --selftest)
  add_test(NAME mhgp8_q2_resume_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_resume_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_resume_probe> --selftest)
  add_test(NAME mhgp8_q2_resume_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_resume_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_resume_probe> --selftest)
  add_test(NAME mhgp8_q3_q4_owner_independence_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q3_q4_owner_independence_gate.py" --selftest)
  add_test(NAME mhgp8_q3_q4_owner_independence_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q3_q4_owner_independence_gate.py" --selftest)
  add_test(NAME mhgp8_wspd_q2_dynamic_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_dynamic_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_dynamic_probe> --selftest)
  add_test(NAME mhgp8_wspd_q2_dynamic_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_dynamic_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_dynamic_probe> --selftest)
  add_test(NAME mhgp8_wspd_q2_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_census_probe> --selftest)
  add_test(NAME mhgp8_wspd_q2_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_census_probe> --selftest)
  add_test(NAME mhgp8_wspd_front_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_front_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_front_probe> --selftest)
  add_test(NAME mhgp8_wspd_front_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_front_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_front_probe> --selftest)
  add_test(NAME mhgp8_cloud_reuse_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cloud_reuse_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_cloud_reuse_probe> --selftest)
  add_test(NAME mhgp8_cloud_reuse_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cloud_reuse_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_cloud_reuse_probe> --selftest)
  add_test(NAME mhgp8_q2_revision_comparison_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_revision_comparison_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_probe> --selftest)
  add_test(NAME mhgp8_q2_revision_comparison_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_revision_comparison_gate.py"
    --probe $<TARGET_FILE:mhgp8_q2_census_probe> --selftest)
  foreach(gate IN ITEMS probe campaign)
    add_test(NAME mhgp8_q2_census_${gate}_gate COMMAND ${Python3_EXECUTABLE} -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_census_${gate}_gate.py"
      --probe $<TARGET_FILE:mhgp8_q2_census_probe> --selftest)
    add_test(NAME mhgp8_q2_census_${gate}_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/q2_census_${gate}_gate.py"
      --probe $<TARGET_FILE:mhgp8_q2_census_probe> --selftest)
  endforeach()
  add_test(NAME mhgp8_wspd_q2_parallel_receipts_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_parallel_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_parallel_probe> --selftest)
  add_test(NAME mhgp8_wspd_q2_parallel_receipts_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/wspd_q2_parallel_receipts_gate.py"
    --probe $<TARGET_FILE:mhgp8_wspd_q2_parallel_probe> --selftest)
  add_test(NAME mhgp8_docs_scope COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/docs_scope_gate.py" --selftest)
  add_test(NAME mhgp8_docs_scope_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/docs_scope_gate.py" --selftest)
  add_test(NAME mhgp8_probe_cli COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_probe_cli_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_campaign_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_campaign_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_batch_probe_cli COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/batch_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_batch_probe> --selftest)
  add_test(NAME mhgp8_batch_probe_cli_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/batch_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_batch_probe> --selftest)
  add_test(NAME mhgp8_axis_probe_cli COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/axis_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  add_test(NAME mhgp8_axis_probe_cli_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/axis_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  add_test(NAME mhgp8_additive_probe_cli COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/additive_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_additive_probe> --selftest)
  add_test(NAME mhgp8_additive_probe_cli_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/additive_probe_cli_gate.py"
    --probe $<TARGET_FILE:mhgp8_additive_probe> --selftest)
  add_test(NAME mhgp8_additive_campaign_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/additive_campaign_gate.py"
    --probe $<TARGET_FILE:mhgp8_additive_probe> --selftest)
  add_test(NAME mhgp8_additive_campaign_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/additive_campaign_gate.py"
    --probe $<TARGET_FILE:mhgp8_additive_probe> --selftest)
  add_test(NAME mhgp8_paired_campaign_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/paired_campaign_gate.py"
    --batch-probe $<TARGET_FILE:mhgp8_batch_probe>
    --axis-probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  add_test(NAME mhgp8_paired_campaign_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/paired_campaign_gate.py"
    --batch-probe $<TARGET_FILE:mhgp8_batch_probe>
    --axis-probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  add_test(NAME mhgp8_campaign_reader_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_reader_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_campaign_reader_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_reader_gate.py"
    --probe $<TARGET_FILE:mhgp8_p0_probe> --selftest)
  add_test(NAME mhgp8_paired_reader_gate COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/paired_reader_gate.py"
    --batch-probe $<TARGET_FILE:mhgp8_batch_probe>
    --axis-probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  add_test(NAME mhgp8_paired_reader_gate_optimized COMMAND ${Python3_EXECUTABLE} -B -O
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/paired_reader_gate.py"
    --batch-probe $<TARGET_FILE:mhgp8_batch_probe>
    --axis-probe $<TARGET_FILE:mhgp8_axis_probe> --selftest)
  # Labels of the Python u16 gates: receipt readers/ingestion versus CLI, docs
  # and independence gates.
  set(mhgp8_u16_python_receipts)
  set(mhgp8_u16_python_gates)
  foreach(stem IN ITEMS wspd_q2_batched_receipts wspd_q2_proposals_receipts
      wspd_q2_inheritance_receipts wspd_q2_ranges_receipts wspd_q2_cooperative_receipts)
    list(APPEND mhgp8_u16_python_receipts mhgp8_${stem}_normal_gate mhgp8_${stem}_optimized_gate)
  endforeach()
  foreach(stem IN ITEMS q2_split_receipts_gate q2_resume_receipts_gate wspd_q2_dynamic_receipts_gate
      wspd_q2_receipts_gate wspd_front_receipts_gate cloud_reuse_receipts_gate
      q2_revision_comparison_gate q2_census_probe_gate q2_census_campaign_gate
      wspd_q2_parallel_receipts_gate campaign_gate additive_campaign_gate paired_campaign_gate
      campaign_reader_gate paired_reader_gate)
    list(APPEND mhgp8_u16_python_receipts mhgp8_${stem} mhgp8_${stem}_optimized)
  endforeach()
  foreach(stem IN ITEMS q3_q4_owner_independence_gate docs_scope probe_cli batch_probe_cli
      axis_probe_cli additive_probe_cli)
    list(APPEND mhgp8_u16_python_gates mhgp8_${stem} mhgp8_${stem}_optimized)
  endforeach()
  mhgp8_label("u16;receipts" ${mhgp8_u16_python_receipts})
  mhgp8_label("u16;gate" ${mhgp8_u16_python_gates})

  # ------------------------------------------------------------ binary32 lane
  # Native probes of tests/ carry a --selftest mode; the Python oracles drive
  # the same binaries on fixtures (tests/float32_*_gate.py --binary --output),
  # exactly as bench/run_float32_*_checks.py and run_lidar_precision_checks.py do.
  foreach(probe IN ITEMS float32_ball float32_predicate float32_index float32_key
      float32_q4_events float32_q3_census)
    add_executable(mhgp8_${probe}_probe tests/${probe}_probe.cpp)
    target_link_libraries(mhgp8_${probe}_probe PRIVATE mhgp8_f32)
    add_test(NAME mhgp8_${probe}_selftest COMMAND mhgp8_${probe}_probe --selftest)
    mhgp8_label("float32;gate" mhgp8_${probe}_selftest)
  endforeach()
  # Exact fixed-point integer helpers of the lane (header-only src/core/fixed_signed.hpp).
  foreach(gate IN ITEMS fixed_signed fixed_signed_division)
    add_executable(mhgp8_${gate}_gate tests/${gate}_gate.cpp)
    target_link_libraries(mhgp8_${gate}_gate PRIVATE mhgp8_f32)
    add_test(NAME mhgp8_${gate}_gate COMMAND mhgp8_${gate}_gate --selftest)
    mhgp8_label("float32;gate" mhgp8_${gate}_gate)
  endforeach()
  # Synthetic measurement of the index at the smallest size of interest
  # (the launcher runs uniform/terrain/clusters at 8000/16000/32000 with --repeat 3).
  add_test(NAME mhgp8_float32_index_bench_uniform_8000
    COMMAND mhgp8_float32_index_bench --synthetic uniform --n 8000 --repeat 1)
  mhgp8_label("float32;gate" mhgp8_float32_index_bench_uniform_8000)
  foreach(opt IN ITEMS normal optimized)
    set(f32_python_flags -B)
    set(f32_suffix "")
    if(opt STREQUAL "optimized")
      list(APPEND f32_python_flags -O)
      set(f32_suffix "_optimized")
    endif()
    foreach(stem IN ITEMS float32_ball float32_predicate float32_index float32_q3_census)
      set(f32_output "${MHGP8_FRESH_ROOT}/${stem}_gate${f32_suffix}")
      mhgp8_add_fresh_test(mhgp8_${stem}_gate${f32_suffix}
        FRESH "${f32_output}"
        COMMAND ${Python3_EXECUTABLE} ${f32_python_flags}
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/${stem}_gate.py"
          --binary $<TARGET_FILE:mhgp8_${stem}_probe> --output "${f32_output}")
      mhgp8_label("float32;gate" mhgp8_${stem}_gate${f32_suffix})
    endforeach()
    set(f32_output "${MHGP8_FRESH_ROOT}/float32_identity_gate${f32_suffix}")
    mhgp8_add_fresh_test(mhgp8_float32_identity_gate${f32_suffix}
      FRESH "${f32_output}"
      COMMAND ${Python3_EXECUTABLE} ${f32_python_flags}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/float32_identity_gate.py"
        --binary $<TARGET_FILE:mhgp8_float32_key_probe>
        --events-binary $<TARGET_FILE:mhgp8_float32_q4_events_probe> --output "${f32_output}")
    mhgp8_label("float32;gate" mhgp8_float32_identity_gate${f32_suffix})
  endforeach()

  # ------------------------------------------------------------------- LiDAR
  # Pure-Python unittest gates of the preparation scripts (synthetic fixtures
  # in temporary directories; no scan, no native binary), invoked as by
  # bench/run_lidar_{precision,spatial,ground}_checks.py.
  foreach(opt IN ITEMS normal optimized)
    set(lidar_python_flags -B)
    set(lidar_suffix "")
    if(opt STREQUAL "optimized")
      list(APPEND lidar_python_flags -O)
      set(lidar_suffix "_optimized")
    endif()
    foreach(stem IN ITEMS lidar_precision_gate lidar_spatial_gate lidar_ground_test)
      add_test(NAME mhgp8_${stem}${lidar_suffix}
        COMMAND ${Python3_EXECUTABLE} ${lidar_python_flags} "${CMAKE_CURRENT_SOURCE_DIR}/tests/${stem}.py")
      mhgp8_label("lidar;gate" mhgp8_${stem}${lidar_suffix})
    endforeach()
  endforeach()
  if(TARGET mhgp8_patchwork_ground_probe)
    add_test(NAME mhgp8_patchwork_ground_describe COMMAND mhgp8_patchwork_ground_probe --describe)
    mhgp8_label("lidar;gate" mhgp8_patchwork_ground_describe)
  endif()

  # The q34 spatial stream gate and the compiled q34/q4 mutation scripts drive
  # THIS build tree: they read CMakeCache.txt (Release, MHGP8_SANITIZE=OFF),
  # libmhgp8_p0.a, the Boost gates and their object files, and require the
  # tree under <repo>/build/. Otherwise they are registered DISABLED.
  get_filename_component(MHGP8_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
  string(FIND "${CMAKE_BINARY_DIR}" "${MHGP8_REPO_ROOT}/build/" mhgp8_build_prefix)
  set(mhgp8_tree_reasons)
  if(NOT mhgp8_build_prefix EQUAL 0)
    list(APPEND mhgp8_tree_reasons "build tree is not under ${MHGP8_REPO_ROOT}/build/")
  endif()
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND mhgp8_tree_reasons "CMAKE_BUILD_TYPE is not Release")
  endif()
  if(MHGP8_SANITIZE)
    list(APPEND mhgp8_tree_reasons "MHGP8_SANITIZE is ON")
  endif()
  if(NOT Boost_FOUND)
    list(APPEND mhgp8_tree_reasons "Boost judges are absent")
  endif()
  if(mhgp8_tree_reasons)
    string(JOIN "; " mhgp8_tree_reason ${mhgp8_tree_reasons})
    message(STATUS "mhgp8: q34/q4 mutation tests registered DISABLED (${mhgp8_tree_reason})")
  endif()
  # tests/q34_spatial_gate.py drives bench/run_q34_spatial.py, whose authority()
  # binds every run to the closed native r2 build recorded in the pinned receipt
  # receipts/q4_seed_cells_20260921/qualification_r2/smoke_ewedfs4y/MANIFEST.json
  # ("build" field plus artifact hashes); any other tree is refused with
  # "native r2 authority/build mismatch". The gate is registered DISABLED
  # unless THIS build tree is that pinned build (never a false green).
  set(mhgp8_spatial_authority
    "${CMAKE_CURRENT_SOURCE_DIR}/receipts/q4_seed_cells_20260921/qualification_r2/smoke_ewedfs4y/MANIFEST.json")
  set(mhgp8_spatial_reasons ${mhgp8_tree_reasons})
  set(mhgp8_spatial_build "")
  if(EXISTS "${mhgp8_spatial_authority}")
    file(READ "${mhgp8_spatial_authority}" mhgp8_spatial_manifest)
    string(JSON mhgp8_spatial_build ERROR_VARIABLE mhgp8_spatial_json_error
      GET "${mhgp8_spatial_manifest}" build)
    if(mhgp8_spatial_json_error)
      set(mhgp8_spatial_build "")
    endif()
  endif()
  if(NOT mhgp8_spatial_build)
    list(APPEND mhgp8_spatial_reasons "r2 authority receipt unreadable: ${mhgp8_spatial_authority}")
  elseif(NOT CMAKE_BINARY_DIR STREQUAL mhgp8_spatial_build)
    list(APPEND mhgp8_spatial_reasons "this tree is not the pinned r2 authority build ${mhgp8_spatial_build}")
  endif()
  if(mhgp8_spatial_reasons)
    string(JOIN "; " mhgp8_spatial_reason ${mhgp8_spatial_reasons})
    message(STATUS "mhgp8: q34 spatial gate registered DISABLED (${mhgp8_spatial_reason})")
  endif()
  foreach(opt IN ITEMS normal optimized)
    set(spatial_python_flags -B)
    set(spatial_suffix "")
    if(opt STREQUAL "optimized")
      list(APPEND spatial_python_flags -O)
      set(spatial_suffix "_optimized")
    endif()
    set(spatial_output "${MHGP8_FRESH_ROOT}/q34_spatial_gate${spatial_suffix}")
    mhgp8_add_fresh_test(mhgp8_q34_spatial_gate${spatial_suffix}
      FRESH "${spatial_output}"
      COMMAND ${Python3_EXECUTABLE} ${spatial_python_flags}
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/q34_spatial_gate.py"
        --build "${CMAKE_BINARY_DIR}" --output "${spatial_output}")
    mhgp8_label("u16;lidar" mhgp8_q34_spatial_gate${spatial_suffix})
    if(mhgp8_spatial_reasons)
      set_tests_properties(mhgp8_q34_spatial_gate${spatial_suffix} PROPERTIES DISABLED TRUE)
    endif()
  endforeach()

  # --------------------------------------------------------------- mutations
  # Compiled causal mutations: each script recompiles mutated copies of a
  # product source with its own g++ lines (float32 family) or against this
  # tree's libmhgp8_p0.a and gate objects (q34/q4 family). Slow by nature;
  # registered once (normal Python), like their launchers ran them.
  foreach(stem IN ITEMS float32_ball_mutations float32_identity_mutations
      float32_q3_census_mutations float32_index_mutation_gate)
    set(mutation_root "${MHGP8_FRESH_ROOT}/${stem}")
    mhgp8_add_fresh_test(mhgp8_${stem}
      FRESH "${mutation_root}"
      COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/${stem}.py"
        --build "${mutation_root}/build" --output "${mutation_root}/capture"
        --compiler "${CMAKE_CXX_COMPILER}")
    mhgp8_label("float32;mutation;slow" mhgp8_${stem})
  endforeach()
  foreach(stem IN ITEMS q34_affine_mutations q4_seed_cells_mutations wspd_q34_mutations)
    set(mutation_root "${MHGP8_FRESH_ROOT}/${stem}")
    mhgp8_add_fresh_test(mhgp8_${stem}
      FRESH "${mutation_root}"
      COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/${stem}.py" run
        --build "${CMAKE_BINARY_DIR}" --output "${mutation_root}")
    mhgp8_label("u16;mutation;slow" mhgp8_${stem})
  endforeach()
  foreach(kind IN ITEMS witness census)
    set(mutation_root "${MHGP8_FRESH_ROOT}/q34_indexed_${kind}_mutations")
    mhgp8_add_fresh_test(mhgp8_q34_indexed_${kind}_mutations
      FRESH "${mutation_root}"
      COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/q34_indexed_mutations.py" run
        --build "${CMAKE_BINARY_DIR}" --output "${mutation_root}" --kind ${kind})
    mhgp8_label("u16;mutation;slow" mhgp8_q34_indexed_${kind}_mutations)
  endforeach()
  if(mhgp8_tree_reasons)
    set_tests_properties(mhgp8_q34_affine_mutations mhgp8_q4_seed_cells_mutations
      mhgp8_wspd_q34_mutations mhgp8_q34_indexed_witness_mutations
      mhgp8_q34_indexed_census_mutations PROPERTIES DISABLED TRUE)
  endif()
  # tests/q34_indexed_mutations.py --kind witness (written at d1b4dbc6) requires
  # each mutation site to be unique in src/lanes/q34_witness_search.cpp; since
  # 2629a536 (affine pair bounds) the site of "admitted_lane_recounted_in_children"
  # occurs twice, so the script refuses before compiling ("mutation site is not
  # unique"). Registered DISABLED until the script is realigned; neither the
  # script nor the product source is changed here.
  set_tests_properties(mhgp8_q34_indexed_witness_mutations PROPERTIES DISABLED TRUE)
  message(STATUS "mhgp8: mhgp8_q34_indexed_witness_mutations registered DISABLED "
    "(tests/q34_indexed_mutations.py: mutation site not unique in q34_witness_search.cpp since 2629a536)")

  # -------------------------------------------------------------------- slow
  # Measured above 10 s on a shared 8-vCPU host with ctest -j 3 (Release),
  # both twins of a normal/-O pair labelled together.
  set(mhgp8_slow_u16 mhgp8_wspd_q2_receipts_gate mhgp8_wspd_q2_receipts_gate_optimized
    mhgp8_campaign_gate mhgp8_campaign_gate_optimized
    mhgp8_wspd_q2_dynamic_receipts_gate mhgp8_wspd_q2_dynamic_receipts_gate_optimized
    mhgp8_wspd_q2_proposals_gate
    mhgp8_paired_reader_gate mhgp8_paired_reader_gate_optimized
    mhgp8_wspd_q2_inheritance_receipts_normal_gate mhgp8_wspd_q2_inheritance_receipts_optimized_gate
    mhgp8_q2_census_campaign_gate mhgp8_q2_census_campaign_gate_optimized
    mhgp8_wspd_q2_parallel_receipts_gate mhgp8_wspd_q2_parallel_receipts_gate_optimized
    mhgp8_wspd_front_inheritance_gate
    mhgp8_wspd_front_receipts_gate mhgp8_wspd_front_receipts_gate_optimized
    mhgp8_wspd_q2_proposals_receipts_normal_gate mhgp8_wspd_q2_proposals_receipts_optimized_gate
    mhgp8_q2_revision_comparison_gate mhgp8_q2_revision_comparison_gate_optimized)
  mhgp8_label("slow" ${mhgp8_slow_u16})
endif()
