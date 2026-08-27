# MorseHGP3D v5 — porte a code de sortie EXACT.
#
# CTest compare mal les codes de retour (PASS_REGULAR_EXPRESSION les ignore) :
# ce script execute une commande et exige un code PRECIS. Un crash par signal
# n'est jamais un succes (execute_process rend alors un texte non numerique).
if(NOT DEFINED EXPECTED OR NOT DEFINED CMD)
  message(FATAL_ERROR "run_expect.cmake exige EXPECTED et CMD")
endif()
separate_arguments(arg_list UNIX_COMMAND "${ARGS}")
execute_process(COMMAND "${CMD}" ${arg_list} RESULT_VARIABLE rc)
if(NOT rc MATCHES "^[0-9]+$")
  message(FATAL_ERROR "termine par signal ou erreur d'execution : ${rc}")
endif()
if(NOT rc EQUAL "${EXPECTED}")
  message(FATAL_ERROR "code de sortie ${rc}, attendu ${EXPECTED}")
endif()
