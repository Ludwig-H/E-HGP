cmake_minimum_required(VERSION 3.20)
project(morsehgp3D_v8 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(CTest)
find_package(Threads REQUIRED)
option(MHGP8_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(mhgp8_p0 STATIC src/pipeline/local_credits.cpp src/pipeline/axis_q2.cpp
  src/pipeline/q2_census.cpp src/pipeline/q2_census_parallel.cpp
  src/pipeline/prepared_cloud.cpp src/wspd/front.cpp)
target_include_directories(mhgp8_p0 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(mhgp8_p0 PUBLIC cxx_std_20)
set_target_properties(mhgp8_p0 PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(mhgp8_p0 PUBLIC -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(mhgp8_p0 PUBLIC Threads::Threads)
if(MHGP8_SANITIZE)
  target_compile_options(mhgp8_p0 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(mhgp8_p0 PUBLIC -fsanitize=address,undefined)
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

if(BUILD_TESTING)
  # Boost arbitrary precision is used only by the bounded independent judge.
  find_package(Boost REQUIRED)
  foreach(gate IN ITEMS q2_node_pool q2_terminal_pool wspd_front_jobs wspd_q2_parallel
      wspd_front_dispatch wspd_q2_dynamic q2_census_resume q2_census_detach q2_census_parallel
      wspd_q2_cooperative wspd_q2_ranges wspd_q2_batched wspd_q2_proposals wspd_front_inheritance)
    add_executable(mhgp8_${gate}_gate tests/${gate}_gate.cpp)
    target_include_directories(mhgp8_${gate}_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories(mhgp8_${gate}_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(mhgp8_${gate}_gate PRIVATE mhgp8_p0)
    add_test(NAME mhgp8_${gate}_gate COMMAND mhgp8_${gate}_gate --selftest)
  endforeach()
  set_tests_properties(mhgp8_wspd_front_dispatch_gate mhgp8_wspd_q2_dynamic_gate
    PROPERTIES TIMEOUT 60)
  add_executable(mhgp8_q2_joint_gate tests/q2_joint_gate.cpp)
  target_include_directories(mhgp8_q2_joint_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(mhgp8_q2_joint_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_joint_gate COMMAND mhgp8_q2_joint_gate --selftest)
  add_executable(mhgp8_q2_joint_bounds_gate tests/q2_joint_bounds_gate.cpp)
  target_include_directories(mhgp8_q2_joint_bounds_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_q2_joint_bounds_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_q2_joint_bounds_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_joint_bounds_gate COMMAND mhgp8_q2_joint_bounds_gate --selftest)
  add_executable(mhgp8_q2_witness_order_gate tests/q2_witness_order_gate.cpp)
  target_include_directories(mhgp8_q2_witness_order_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(mhgp8_q2_witness_order_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_witness_order_gate COMMAND mhgp8_q2_witness_order_gate --selftest)
  add_executable(mhgp8_q2_sibling_gate tests/q2_sibling_gate.cpp)
  target_include_directories(mhgp8_q2_sibling_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(mhgp8_q2_sibling_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_sibling_gate COMMAND mhgp8_q2_sibling_gate --selftest)
  add_executable(mhgp8_wspd_q2_census_gate tests/wspd_q2_census_gate.cpp)
  target_include_directories(mhgp8_wspd_q2_census_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_wspd_q2_census_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_wspd_q2_census_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_wspd_q2_census_gate COMMAND mhgp8_wspd_q2_census_gate --selftest)
  add_executable(mhgp8_wspd_front_gate tests/wspd_front_gate.cpp)
  target_include_directories(mhgp8_wspd_front_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_wspd_front_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_wspd_front_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_wspd_front_gate COMMAND mhgp8_wspd_front_gate --selftest)
  add_executable(mhgp8_cloud_owner_gate tests/cloud_owner_gate.cpp)
  target_include_directories(mhgp8_cloud_owner_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(mhgp8_cloud_owner_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_cloud_owner_gate COMMAND mhgp8_cloud_owner_gate --selftest)
  add_executable(mhgp8_p0_gate tests/p0_gate.cpp)
  target_include_directories(mhgp8_p0_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_p0_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_p0_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_p0_gate COMMAND mhgp8_p0_gate --selftest)
  add_executable(mhgp8_tube_gate tests/tube_gate.cpp)
  target_include_directories(mhgp8_tube_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_tube_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_tube_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_tube_gate COMMAND mhgp8_tube_gate --selftest)
  add_executable(mhgp8_batch_gate tests/batch_gate.cpp)
  target_include_directories(mhgp8_batch_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(mhgp8_batch_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_batch_gate COMMAND mhgp8_batch_gate --selftest)
  add_executable(mhgp8_axis_q2_gate tests/axis_q2_gate.cpp)
  target_include_directories(mhgp8_axis_q2_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_axis_q2_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_axis_q2_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_axis_q2_gate COMMAND mhgp8_axis_q2_gate --selftest)
  add_executable(mhgp8_axis_additive_gate tests/axis_additive_gate.cpp)
  target_include_directories(mhgp8_axis_additive_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_axis_additive_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_axis_additive_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_axis_additive_gate COMMAND mhgp8_axis_additive_gate --selftest)
  add_executable(mhgp8_plan_assignment_gate tests/plan_assignment_gate.cpp)
  target_link_libraries(mhgp8_plan_assignment_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_plan_assignment_gate COMMAND mhgp8_plan_assignment_gate --selftest)
  add_executable(mhgp8_q2_census_gate tests/q2_census_gate.cpp)
  target_include_directories(mhgp8_q2_census_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_q2_census_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_q2_census_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_census_gate COMMAND mhgp8_q2_census_gate --selftest)
  add_executable(mhgp8_q2_prepared_bounds_gate tests/q2_prepared_bounds_gate.cpp)
  target_include_directories(mhgp8_q2_prepared_bounds_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(mhgp8_q2_prepared_bounds_gate SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(mhgp8_q2_prepared_bounds_gate PRIVATE mhgp8_p0)
  add_test(NAME mhgp8_q2_prepared_bounds_gate COMMAND mhgp8_q2_prepared_bounds_gate --selftest)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
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
endif()
