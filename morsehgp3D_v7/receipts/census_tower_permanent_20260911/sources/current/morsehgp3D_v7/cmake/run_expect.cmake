# MorseHGP3D v6 — porte a code de sortie EXACT.
#
# CTest compare mal les codes de retour (PASS_REGULAR_EXPRESSION les ignore) :
# ce script execute une commande et exige un code PRECIS. Un crash par signal
# n'est jamais un succes (execute_process rend alors un texte non numerique).
if(NOT DEFINED EXPECTED OR NOT DEFINED CMD)
  message(FATAL_ERROR "run_expect.cmake exige EXPECTED et CMD")
endif()
separate_arguments(arg_list UNIX_COMMAND "${ARGS}")
# stdout CAPTURE puis reemis : la meme execution peut etre jugee sur son code
# ET sur une ligne exacte (EXPECT_LINE) — jamais deux processus distincts
# (PASS_REGULAR_EXPRESSION ignore le code de sortie).
execute_process(COMMAND "${CMD}" ${arg_list} RESULT_VARIABLE rc OUTPUT_VARIABLE run_stdout)
message("${run_stdout}")
if(NOT rc MATCHES "^[0-9]+$")
  message(FATAL_ERROR "termine par signal ou erreur d'execution : ${rc}")
endif()
if(NOT rc EQUAL "${EXPECTED}")
  message(FATAL_ERROR "code de sortie ${rc}, attendu ${EXPECTED}")
endif()
if(DEFINED EXPECT_PREFIX AND NOT EXPECT_PREFIX STREQUAL "")
  string(REPLACE "\r\n" "\n" normalized_stdout "${run_stdout}")
  string(FIND "\n${normalized_stdout}" "\n${EXPECT_PREFIX}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "prefixe de ligne attendu ABSENT : ${EXPECT_PREFIX}")
  endif()
endif()
if(DEFINED EXPECT_LINE AND NOT EXPECT_LINE STREQUAL "")
  string(REPLACE "\r\n" "\n" normalized_stdout "${run_stdout}")
  string(FIND "\n${normalized_stdout}\n" "\n${EXPECT_LINE}\n" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "ligne attendue ABSENTE de stdout de la MEME execution : ${EXPECT_LINE}")
  endif()
endif()
