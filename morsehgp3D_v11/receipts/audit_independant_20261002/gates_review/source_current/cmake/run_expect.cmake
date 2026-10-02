# Porte a code de sortie EXACT (docs/ARCHITECTURE.md, paragraphe 5) : execute une commande et exige un code precis.
# Port de morsehgp3D_v10/cmake/run_expect.cmake du raccord R2 (commit 865f5e6), lui-meme issu des v7 et v9.
#
# CTest compare mal les codes de retour (PASS_REGULAR_EXPRESSION les ignore). Ce script :
#   - exige EXPECTED dans {0, 1, 2, 3, 4} et un code de sortie exactement egal ;
#   - refuse tout arret anormal : signal, delai ou lancement impossible (execute_process rend alors un texte non
#     numerique) ; un arret par signal n'est jamais un succes ;
#   - juge une ligne exacte de la sortie standard (EXPECT_LINE) sur la MEME execution que le code ;
#   - saute la porte si le dossier nomme par une variable d'environnement manque (REQUIRE_DIR_ENV) : il ecrit le jeton
#     mhgp11_porte_sautee, que la propriete SKIP_REGULAR_EXPRESSION de la porte lit (cmake/gates.cmake).
#
# Arguments de la commande : NARGS et ARG0 .. ARG<NARGS-1>, une definition par argument, placees avant -P. Un argument
# garde ainsi ses espaces et ses points-virgules et peut etre vide. La v10 passait une chaine ARGS redecoupee sur les
# espaces. Passer les arguments apres "--" n'est pas possible : CMake 3.22.1 (celui de la VM G4) ne s'arrete pas a
# "--" en mode script et interprete la suite comme ses propres options (constate le 2 octobre 2026 ; 3.28 s'arrete).
#
# Verdict : une ligne "run_expect_verdict <mot>" sur la sortie standard, <mot> parmi conforme, sautee, arret_anormal,
# code, ligne_absente. Toute issue autre que conforme ou sautee termine par FATAL_ERROR (code 1 du script).
#
#   cmake -DCMD=<programme> -DNARGS=<n> -DARG0=<a0> ... -DEXPECTED=<code> [-DEXPECT_LINE=<ligne>]
#         [-DREQUIRE_DIR_ENV=<variable>] -P run_expect.cmake
cmake_minimum_required(VERSION 3.20)  # politiques recentes : une chaine entre guillemets n'est jamais dereferencee
if(NOT DEFINED CMD OR NOT DEFINED EXPECTED)
  message(FATAL_ERROR "run_expect.cmake exige CMD et EXPECTED")
endif()
if(NOT "${EXPECTED}" MATCHES "^[0-4]$")
  message(FATAL_ERROR "run_expect.cmake : EXPECTED=${EXPECTED} hors de {0, 1, 2, 3, 4}")
endif()
if(NOT DEFINED NARGS)
  set(NARGS 0)
endif()
if(NOT "${NARGS}" MATCHES "^[0-9]+$")
  message(FATAL_ERROR "run_expect.cmake : NARGS=${NARGS} n'est pas un entier")
endif()

function(run_expect_say text)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${text}")
endfunction()

if(DEFINED REQUIRE_DIR_ENV AND NOT "${REQUIRE_DIR_ENV}" STREQUAL "")
  set(required_dir "$ENV{${REQUIRE_DIR_ENV}}")
  if("${required_dir}" STREQUAL "" OR NOT IS_DIRECTORY "${required_dir}")
    run_expect_say("mhgp11_porte_sautee ${REQUIRE_DIR_ENV}")
    run_expect_say("run_expect_verdict sautee")
    return()
  endif()
endif()

# La commande est rejouee par cmake_language(EVAL CODE) avec un argument entre crochets par definition : aucun
# redecoupage, aucune evaluation de ${...} ni d'echappement dans les arguments. Un mot qui contiendrait la fermeture
# "]==]" est refuse.
set(run_expect_words CMD)
if(NARGS GREATER 0)
  math(EXPR run_expect_last "${NARGS} - 1")
  foreach(i RANGE 0 ${run_expect_last})
    list(APPEND run_expect_words ARG${i})
  endforeach()
endif()
set(run_expect_code "execute_process(COMMAND")
foreach(word IN LISTS run_expect_words)
  if(NOT DEFINED ${word})
    message(FATAL_ERROR "run_expect.cmake : ${word} absent (NARGS=${NARGS})")
  endif()
  string(FIND "${${word}}" "]==]" run_expect_bad)
  if(NOT run_expect_bad EQUAL -1)
    message(FATAL_ERROR "run_expect.cmake : ${word} contient ]==]")
  endif()
  string(APPEND run_expect_code " [==[${${word}}]==]")
endforeach()
string(APPEND run_expect_code " RESULT_VARIABLE rc OUTPUT_VARIABLE run_stdout ECHO_OUTPUT_VARIABLE)")
cmake_language(EVAL CODE "${run_expect_code}")

if(NOT "${rc}" MATCHES "^[0-9]+$")
  run_expect_say("run_expect_verdict arret_anormal")
  message(FATAL_ERROR "termine par signal, delai ou erreur d'execution : ${rc}")
endif()
if(NOT "${rc}" STREQUAL "${EXPECTED}")
  run_expect_say("run_expect_verdict code")
  message(FATAL_ERROR "code de sortie ${rc}, attendu ${EXPECTED}")
endif()
if(DEFINED EXPECT_LINE AND NOT "${EXPECT_LINE}" STREQUAL "")
  string(REPLACE "\r\n" "\n" run_expect_normalized "${run_stdout}")
  string(FIND "\n${run_expect_normalized}\n" "\n${EXPECT_LINE}\n" run_expect_pos)
  if(run_expect_pos EQUAL -1)
    run_expect_say("run_expect_verdict ligne_absente")
    message(FATAL_ERROR "ligne attendue ABSENTE de la sortie standard de la MEME execution : ${EXPECT_LINE}")
  endif()
endif()
run_expect_say("run_expect_verdict conforme")
