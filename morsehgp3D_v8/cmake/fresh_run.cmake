# Run a command after removing the directories it requires to be absent.
#
# Several Python gates (tests/float32_*_gate.py, tests/*_mutations.py,
# tests/float32_index_mutation_gate.py) refuse an existing --output or
# --build directory, exactly like their bench/run_*_checks.py launchers do.
# CTest reruns the same test in the same build tree, so this script clears
# the per-test scratch area first, then executes the command and propagates
# its exit code. As a safety net it refuses to remove anything outside a
# path containing "/mhgp8_fresh/" (the scratch area chosen by CMakeLists.txt).
#
# Usage (from add_test):
#   cmake -DMHGP8_FRESH=<dir>[;<dir>...] -DMHGP8_COMMAND=<arg>[;<arg>...]
#         -P fresh_run.cmake
if(NOT DEFINED MHGP8_COMMAND OR MHGP8_COMMAND STREQUAL "")
  message(FATAL_ERROR "fresh_run.cmake: MHGP8_COMMAND is required")
endif()
foreach(directory IN LISTS MHGP8_FRESH)
  if(NOT directory MATCHES "/mhgp8_fresh/")
    message(FATAL_ERROR "fresh_run.cmake: refusing to remove '${directory}' (outside mhgp8_fresh)")
  endif()
  file(REMOVE_RECURSE "${directory}")
endforeach()
execute_process(COMMAND ${MHGP8_COMMAND} RESULT_VARIABLE mhgp8_exit_code)
if(NOT mhgp8_exit_code EQUAL 0)
  message(FATAL_ERROR "fresh_run.cmake: command exited with '${mhgp8_exit_code}'")
endif()
