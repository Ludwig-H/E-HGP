# MorseHGP3D v9 — juge d'un mutant compile du generateur (port des scripts
# Python de mutation v8, 3f0d188f : wspd_q34, q34_affine, q4_seed_cells,
# q4_saturating, q34_indexed). La porte native, liee a une copie mutee d'une
# unite de mhgp9_gen, doit rendre le code attendu (1, desaccord du juge) ET
# ecrire sur stderr le texte causal EXACT de tests/gen/mutants.json, dans la
# MEME execution. Un crash par signal, un autre code, un echec de plancher ou
# toute autre exception ne comptent jamais comme un mutant tue.
if(DEFINED REFUSE)
  message(FATAL_ERROR "mutant non construit : ${REFUSE}")
endif()
if(NOT DEFINED CMD OR NOT DEFINED SPEC OR NOT DEFINED INDEX)
  message(FATAL_ERROR "run_mutant.cmake exige CMD, SPEC et INDEX")
endif()
file(READ "${SPEC}" spec_text)
string(JSON name GET "${spec_text}" mutants ${INDEX} name)
string(JSON expected_code GET "${spec_text}" mutants ${INDEX} expected_exit_code)
string(JSON expected_stderr GET "${spec_text}" mutants ${INDEX} expected_stderr)
execute_process(COMMAND "${CMD}" --selftest RESULT_VARIABLE rc
                OUTPUT_VARIABLE run_stdout ERROR_VARIABLE run_stderr)
message("mutant=${name}\nstdout:\n${run_stdout}\nstderr:\n${run_stderr}")
if(NOT rc MATCHES "^[0-9]+$")
  message(FATAL_ERROR "mutant ${name} : termine par signal ou erreur d'execution (${rc}), jamais un mutant tue")
endif()
if(NOT rc EQUAL "${expected_code}")
  message(FATAL_ERROR "mutant ${name} : code de sortie ${rc}, attendu ${expected_code}")
endif()
if(NOT run_stderr STREQUAL expected_stderr)
  message(FATAL_ERROR "mutant ${name} : stderr non causal ; attendu exactement : ${expected_stderr}")
endif()
message("mutant ${name} tue : code ${rc}, cause exacte")
