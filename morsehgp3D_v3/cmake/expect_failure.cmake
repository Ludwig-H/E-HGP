if(NOT DEFINED PROGRAM OR NOT DEFINED EXPECTED)
  message(FATAL_ERROR "expect_failure.cmake exige PROGRAM et EXPECTED")
endif()

execute_process(
  COMMAND "${PROGRAM}" ${ARGUMENTS}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE standard_output
  ERROR_VARIABLE standard_error)

set(combined_output "${standard_output}${standard_error}")
# UN CRASH N'EST PAS UN REJET CONTRACTUEL. `execute_process` rend une CHAINE
# quand le processus meurt par signal; l'ancien harness l'acceptait des que le
# diagnostic attendu avait deja ete imprime (audit live `f37341d`, reproduction
# par SIGSEGV apres ecriture du marqueur). Le code de sortie doit etre un
# ENTIER non nul.
if(NOT result MATCHES "^[0-9]+$")
  message(FATAL_ERROR
    "la commande est morte par signal (${result}) — un crash n'est pas un rejet"
    " contractuel\n--- sortie ---\n${combined_output}")
endif()
if(result EQUAL 0)
  message(FATAL_ERROR
    "la commande devait echouer mais a rendu 0\n--- sortie ---\n${combined_output}")
endif()
# QUAND LE CONTRAT FIXE LE CODE, IL EST EXIGE. 2 = campagne refusee avant tout
# calcul, 3 = plancher ou invariant, 1 = desaccords du juge.
if(DEFINED EXPECTED_CODE AND NOT result EQUAL "${EXPECTED_CODE}")
  message(FATAL_ERROR
    "code de sortie ${result} au lieu du code contractuel ${EXPECTED_CODE}\n"
    "--- sortie ---\n${combined_output}")
endif()
if(NOT combined_output MATCHES "${EXPECTED}")
  message(FATAL_ERROR
    "diagnostic attendu absent : ${EXPECTED}\ncode=${result}\n--- sortie ---\n${combined_output}")
endif()

message(STATUS "echec attendu confirme : code=${result}, diagnostic=${EXPECTED}")
