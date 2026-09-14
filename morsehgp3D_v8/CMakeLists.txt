cmake_minimum_required(VERSION 3.20)
project(morsehgp3D_v8 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(CTest)
option(MHGP8_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(mhgp8_p0 STATIC src/pipeline/local_credits.cpp src/pipeline/axis_q2.cpp
  src/pipeline/q2_census.cpp src/pipeline/prepared_cloud.cpp)
target_include_directories(mhgp8_p0 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(mhgp8_p0 PUBLIC cxx_std_20)
set_target_properties(mhgp8_p0 PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(mhgp8_p0 PUBLIC -Wall -Wextra -Wpedantic -Werror)
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

if(BUILD_TESTING)
  # Boost arbitrary precision is used only by the bounded independent judge.
  find_package(Boost REQUIRED)
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
