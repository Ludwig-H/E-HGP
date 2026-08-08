if(NOT DEFINED PROGRAM OR NOT DEFINED EXPECTED)
  message(FATAL_ERROR "expect_failure.cmake exige PROGRAM et EXPECTED")
endif()

execute_process(
  COMMAND "${PROGRAM}" ${ARGUMENTS}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE standard_output
  ERROR_VARIABLE standard_error)

set(combined_output "${standard_output}${standard_error}")
if(result EQUAL 0)
  message(FATAL_ERROR
    "la commande devait echouer mais a rendu 0\n--- sortie ---\n${combined_output}")
endif()
if(NOT combined_output MATCHES "${EXPECTED}")
  message(FATAL_ERROR
    "diagnostic attendu absent : ${EXPECTED}\ncode=${result}\n--- sortie ---\n${combined_output}")
endif()

message(STATUS "echec attendu confirme : code=${result}, diagnostic=${EXPECTED}")
