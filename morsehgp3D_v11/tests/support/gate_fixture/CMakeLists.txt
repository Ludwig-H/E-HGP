# Projet factice (hors produit, sans compilateur) qui enregistre des portes par les aides de cmake/gates.cmake, pour
# juger ces aides elles-memes : leurs refus a la configuration (MHGP11_FIXTURE_FAULT) et les proprietes des portes
# qu'elles creent (tests/support/test_gate_properties.py). Configure par les portes mhgp11_support_gates_*.
#
#   cmake -S tests/support/gate_fixture -B <dossier> -DMHGP11_ROOT=<racine v11> [-DMHGP11_FIXTURE_FAULT=<faute>]
cmake_minimum_required(VERSION 3.20)
project(mhgp11_gate_fixture NONE)
include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
include(${MHGP11_ROOT}/cmake/gates.cmake)

set(MHGP11_FIXTURE_FAULT "" CACHE STRING "Faute d'enregistrement que la configuration doit refuser")
set(script ${MHGP11_ROOT}/tests/support/test_gate_helper.py)

# Portes conformes : une de chaque forme, dont les proprietes sont lues par test_gate_properties.py.
mhgp11_expect_code(mhgp11_fixture_plain 0 ${CMAKE_COMMAND} -E true LABELS unit fast)
mhgp11_expect_code(mhgp11_fixture_timeout 0 ${CMAKE_COMMAND} -E true LABELS oracle TIMEOUT 42
                   ENV MHGP11_FIXTURE=1 LINE "ligne attendue")
mhgp11_expect_code(mhgp11_fixture_long 0 ${CMAKE_COMMAND} -E true LABELS unit long)
mhgp11_expect_code(mhgp11_fixture_scale 0 ${CMAKE_COMMAND} -E true LABELS scale8000)
mhgp11_expect_code(mhgp11_fixture_lidar 0 ${CMAKE_COMMAND} -E true LABELS lidar)
# Un programme qui ecrit lui-meme le jeton de saut puis reussit : la porte doit echouer, pas etre sautee.
mhgp11_expect_code(mhgp11_fixture_usurper 0 ${CMAKE_COMMAND} -E echo "mhgp11_porte_sautee MHGP11_DATA_DIR"
                   LABELS lidar)
mhgp11_python_gate(mhgp11_fixture_python 0 ${script} LABELS fast)
mhgp11_python_gate(mhgp11_fixture_python_long 1 ${script} un "deux mots" LABELS oracle long)
mhgp11_expect_refusal(mhgp11_fixture_refusal TOKEN jeton COMMAND ${CMAKE_COMMAND} -E true REFUSAL faux
                      LABELS diff_v10)
mhgp11_expect_abnormal_stop(mhgp11_fixture_abort ${CMAKE_COMMAND} -E true LABELS unit)
mhgp11_expect_abnormal_stop(mhgp11_fixture_abort_forged ${Python3_EXECUTABLE}
                            ${MHGP11_ROOT}/tests/support/fake_abnormal_stop.py LABELS unit)

# Fautes : chacune doit etre refusee a la configuration avec son message.
if(MHGP11_FIXTURE_FAULT STREQUAL "label")
  mhgp11_expect_code(mhgp11_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS rapide)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "no_label")
  mhgp11_expect_code(mhgp11_fixture_fault 0 ${CMAKE_COMMAND} -E true)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "fast_long")
  mhgp11_expect_code(mhgp11_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast long)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "name")
  mhgp11_expect_code(fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "code")
  mhgp11_expect_code(mhgp11_fixture_fault 5 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "duplicate")
  mhgp11_expect_code(mhgp11_fixture_plain 0 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "program")
  mhgp11_expect_code(mhgp11_fixture_fault 0 programme_relatif LABELS fast)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "timeout")
  mhgp11_expect_code(mhgp11_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast TIMEOUT bientot)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "script")
  mhgp11_python_gate(mhgp11_fixture_fault 0 ${MHGP11_ROOT}/tests/support/script_absent.py LABELS fast)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "direct")
  add_test(NAME mhgp11_fixture_fault COMMAND ${CMAKE_COMMAND} -E true)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "subdirectory")
  add_subdirectory(enfant)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "pass_regex")
  set_tests_properties(mhgp11_fixture_plain PROPERTIES PASS_REGULAR_EXPRESSION "vert")
elseif(MHGP11_FIXTURE_FAULT STREQUAL "will_fail")
  set_tests_properties(mhgp11_fixture_plain PROPERTIES WILL_FAIL TRUE)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "disabled")
  set_tests_properties(mhgp11_fixture_plain PROPERTIES DISABLED TRUE)
elseif(MHGP11_FIXTURE_FAULT STREQUAL "skip_regex")
  set_tests_properties(mhgp11_fixture_plain PROPERTIES SKIP_REGULAR_EXPRESSION "saute")
elseif(MHGP11_FIXTURE_FAULT STREQUAL "lidar_regex")
  set_tests_properties(mhgp11_fixture_lidar PROPERTIES SKIP_REGULAR_EXPRESSION "saute")
elseif(NOT MHGP11_FIXTURE_FAULT STREQUAL "")
  message(FATAL_ERROR "faute inconnue : ${MHGP11_FIXTURE_FAULT}")
endif()

mhgp11_check_gate_registry()
