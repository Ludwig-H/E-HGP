# Projet factice (hors produit, sans compilateur) qui enregistre des portes par les aides de cmake/gates.cmake, pour
# juger ces aides elles-memes : leurs refus a la configuration (MHGP12_FIXTURE_FAULT) et les proprietes des portes
# qu'elles creent (tests/support/test_gate_properties.py). Configure par les portes mhgp12_support_gates_*.
#
#   cmake -S tests/support/gate_fixture -B <dossier> -DMHGP12_ROOT=<racine v12> [-DMHGP12_FIXTURE_FAULT=<faute>]
cmake_minimum_required(VERSION 3.20)
project(mhgp12_gate_fixture NONE)
include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
include(${MHGP12_ROOT}/cmake/gates.cmake)

set(MHGP12_FIXTURE_FAULT "" CACHE STRING "Faute d'enregistrement que la configuration doit refuser")
set(script ${MHGP12_ROOT}/tests/support/test_gate_helper.py)

# Portes conformes : une de chaque forme, dont les proprietes sont lues par test_gate_properties.py.
mhgp12_expect_code(mhgp12_fixture_plain 0 ${CMAKE_COMMAND} -E true LABELS unit fast)
mhgp12_expect_code(mhgp12_fixture_timeout 0 ${CMAKE_COMMAND} -E true LABELS oracle TIMEOUT 42
                   ENV MHGP12_FIXTURE=1 LINE "ligne attendue")
mhgp12_expect_code(mhgp12_fixture_long 0 ${CMAKE_COMMAND} -E true LABELS unit long)
mhgp12_expect_code(mhgp12_fixture_scale 0 ${CMAKE_COMMAND} -E true LABELS scale8000)
mhgp12_expect_code(mhgp12_fixture_lidar 0 ${CMAKE_COMMAND} -E true LABELS lidar)
# Un programme qui ecrit lui-meme le jeton de saut puis reussit : la porte doit echouer, pas etre sautee.
mhgp12_expect_code(mhgp12_fixture_usurper 0 ${CMAKE_COMMAND} -E echo "mhgp12_porte_sautee MHGP12_DATA_DIR"
                   LABELS lidar)
mhgp12_python_gate(mhgp12_fixture_python 0 ${script} LABELS fast)
mhgp12_python_gate(mhgp12_fixture_python_long 1 ${script} un "deux mots" LABELS oracle long)
mhgp12_expect_refusal(mhgp12_fixture_refusal TOKEN jeton COMMAND ${CMAKE_COMMAND} -E true REFUSAL faux
                      LABELS diff_v10)
mhgp12_expect_abnormal_stop(mhgp12_fixture_abort ${CMAKE_COMMAND} -E true LABELS unit)
mhgp12_expect_abnormal_stop(mhgp12_fixture_abort_forged ${Python3_EXECUTABLE}
                            ${MHGP12_ROOT}/tests/support/fake_abnormal_stop.py LABELS unit)

# Fautes : chacune doit etre refusee a la configuration avec son message.
if(MHGP12_FIXTURE_FAULT STREQUAL "label")
  mhgp12_expect_code(mhgp12_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS rapide)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "no_label")
  mhgp12_expect_code(mhgp12_fixture_fault 0 ${CMAKE_COMMAND} -E true)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "fast_long")
  mhgp12_expect_code(mhgp12_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast long)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "name")
  mhgp12_expect_code(fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "code")
  mhgp12_expect_code(mhgp12_fixture_fault 5 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "duplicate")
  mhgp12_expect_code(mhgp12_fixture_plain 0 ${CMAKE_COMMAND} -E true LABELS fast)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "program")
  mhgp12_expect_code(mhgp12_fixture_fault 0 programme_relatif LABELS fast)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "timeout")
  mhgp12_expect_code(mhgp12_fixture_fault 0 ${CMAKE_COMMAND} -E true LABELS fast TIMEOUT bientot)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "script")
  mhgp12_python_gate(mhgp12_fixture_fault 0 ${MHGP12_ROOT}/tests/support/script_absent.py LABELS fast)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "direct")
  add_test(NAME mhgp12_fixture_fault COMMAND ${CMAKE_COMMAND} -E true)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "subdirectory")
  add_subdirectory(enfant)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "pass_regex")
  set_tests_properties(mhgp12_fixture_plain PROPERTIES PASS_REGULAR_EXPRESSION "vert")
elseif(MHGP12_FIXTURE_FAULT STREQUAL "will_fail")
  set_tests_properties(mhgp12_fixture_plain PROPERTIES WILL_FAIL TRUE)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "disabled")
  set_tests_properties(mhgp12_fixture_plain PROPERTIES DISABLED TRUE)
elseif(MHGP12_FIXTURE_FAULT STREQUAL "skip_regex")
  set_tests_properties(mhgp12_fixture_plain PROPERTIES SKIP_REGULAR_EXPRESSION "saute")
elseif(MHGP12_FIXTURE_FAULT STREQUAL "lidar_regex")
  set_tests_properties(mhgp12_fixture_lidar PROPERTIES SKIP_REGULAR_EXPRESSION "saute")
elseif(NOT MHGP12_FIXTURE_FAULT STREQUAL "")
  message(FATAL_ERROR "faute inconnue : ${MHGP12_FIXTURE_FAULT}")
endif()

mhgp12_check_gate_registry()
