add_executable(morsehgp3d_exact_types_tests test_exact_types.cpp)
target_link_libraries(morsehgp3d_exact_types_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_exact_types_dump exact_types_dump.cpp)
target_link_libraries(morsehgp3d_exact_types_dump PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_predicates_tests test_predicates.cpp)
target_link_libraries(morsehgp3d_predicates_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_affine_tests test_affine.cpp)
target_link_libraries(morsehgp3d_affine_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_centers_tests test_centers.cpp)
target_link_libraries(morsehgp3d_centers_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_support_tests test_support.cpp)
target_link_libraries(morsehgp3d_support_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_level_order_tests test_level_order.cpp)
target_link_libraries(morsehgp3d_level_order_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_expansion_tests test_expansion.cpp)
target_link_libraries(morsehgp3d_expansion_tests PRIVATE morsehgp3d::exact)

if(MSVC)
  target_compile_options(morsehgp3d_exact_types_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_exact_types_dump PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_predicates_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_affine_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_centers_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_support_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_level_order_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_expansion_tests PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_exact_types_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_exact_types_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_predicates_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_affine_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_centers_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_support_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_level_order_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_expansion_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_test(NAME morsehgp3d.exact_types COMMAND morsehgp3d_exact_types_tests)
add_test(NAME morsehgp3d.predicates COMMAND morsehgp3d_predicates_tests)
add_test(NAME morsehgp3d.affine COMMAND morsehgp3d_affine_tests)
add_test(NAME morsehgp3d.centers COMMAND morsehgp3d_centers_tests)
add_test(NAME morsehgp3d.support COMMAND morsehgp3d_support_tests)
add_test(NAME morsehgp3d.level_order COMMAND morsehgp3d_level_order_tests)
add_test(NAME morsehgp3d.expansion COMMAND morsehgp3d_expansion_tests)

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(
  NAME morsehgp3d.exact_contract_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_exact_types.py"
    "$<TARGET_FILE:morsehgp3d_exact_types_dump>"
)

if(TARGET morsehgp3d_replay_predicate)
  add_test(
    NAME morsehgp3d.predicate_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.predicate_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.predicate_decision_batch_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_decision_batch.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.affine_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.affine_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.affine_adaptive_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_adaptive_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.affine_adaptive_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_adaptive_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.center_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_center_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.center_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_center_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.support_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_adaptive_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_adaptive_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_adaptive_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_adaptive_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.level_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.level_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.level_checkpoint_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_checkpoint.py"
      "${PROJECT_SOURCE_DIR}/tools/level_run_checkpoint.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
endif()
