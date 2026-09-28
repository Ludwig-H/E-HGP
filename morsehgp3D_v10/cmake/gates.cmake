# mhgp10_gate(NAME <nom> COMMAND <cible> [ARGS <args...>] CODE <code> [LINE "<ligne exacte>"]
#             LABELS <labels...> [TIMEOUT <secondes>])
#
# Enregistre un CTest qui execute la cible via run_expect.cmake : code de sortie EXACT, signal refuse,
# ligne attendue lue dans la meme execution. Codes des portes : 0 conforme, 1 desaccord d'un juge,
# 2 refus avant calcul, 3 plancher ou invariant viole.
function(mhgp10_gate)
  cmake_parse_arguments(G "" "NAME;COMMAND;CODE;LINE;TIMEOUT" "ARGS;LABELS" ${ARGN})
  if(NOT G_NAME OR NOT G_COMMAND OR G_CODE STREQUAL "")
    message(FATAL_ERROR "mhgp10_gate exige NAME, COMMAND et CODE")
  endif()
  if(NOT G_LABELS)
    message(FATAL_ERROR "mhgp10_gate ${G_NAME} : au moins un label")
  endif()
  string(REPLACE ";" " " args_str "${G_ARGS}")
  add_test(NAME ${G_NAME}
           COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:${G_COMMAND}> "-DARGS=${args_str}"
                   -DEXPECTED=${G_CODE} "-DEXPECT_LINE=${G_LINE}"
                   -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set(timeout 300)
  if(G_TIMEOUT)
    set(timeout ${G_TIMEOUT})
  endif()
  set_tests_properties(${G_NAME} PROPERTIES LABELS "${G_LABELS}" TIMEOUT ${timeout})
endfunction()
