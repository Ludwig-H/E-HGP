# Porte a code de sortie EXACT (docs/ARCHITECTURE.md, paragraphe 5) : execute une commande et exige un code precis.
# Port de morsehgp3D_v10/cmake/run_expect.cmake du raccord R2 (commit 865f5e6), lui-meme issu des v7 et v9.
#
# CTest compare mal les codes de retour (PASS_REGULAR_EXPRESSION les ignore). Ce script :
#   - exige EXPECTED dans {0, 1, 2, 3, 4} et un code de sortie exactement egal ;
#   - refuse tout arret anormal, signal ou delai (execute_process rend alors un texte non numerique) : un arret par
#     signal n'est jamais un succes ;
#   - distingue le programme qui n'a pas pu etre lance (absent, non executable, interprete introuvable) : aucun juge
#     n'a alors tourne, et ce n'est ni un code ni un signal. Le constat se fait a l'execution, pas par une liste de
#     causes : le programme est lance par /bin/sh -c 'exec "$0" "$@"'. Si exec echoue, le shell rend 126 ou 127 ; s'il
#     reussit, le programme a pris la place du shell et son code ou son signal sont lus tels quels. Un programme qui
#     rendrait de lui-meme 126 ou 127 est donc compte comme impossible a lancer : jamais comme un mutant tue ;
#   - juge une ligne exacte de la sortie standard (EXPECT_LINE) sur la MEME execution que le code ;
#   - saute la porte si le dossier nomme par une variable d'environnement manque (REQUIRE_DIR_ENV) : il ecrit le jeton
#     mhgp11_porte_sautee, que la propriete SKIP_REGULAR_EXPRESSION de la porte lit (cmake/gates.cmake). Le saut
#     n'appartient qu'a ce precontrole : quand le dossier existe, la sortie du programme est retenue, et un programme
#     qui ecrirait lui-meme le jeton est refuse (verdict jeton_usurpe) au lieu de faire sauter sa propre porte.
#
# Arguments de la commande : NARGS et ARG0 .. ARG<NARGS-1>, une definition par argument, placees avant -P. Un argument
# garde ainsi ses espaces et ses points-virgules et peut etre vide. La v10 passait une chaine ARGS redecoupee sur les
# espaces. Passer les arguments apres "--" n'est pas possible : CMake 3.22.1 (celui de la VM G4) ne s'arrete pas a
# "--" en mode script et interprete la suite comme ses propres options (constate le 2 octobre 2026 ; 3.28 s'arrete).
#
# Verdict : la DERNIERE ligne "run_expect_verdict <mot>" de la sortie standard, <mot> parmi conforme, sautee,
# lancement_impossible, arret_anormal, code, ligne_absente, jeton_usurpe. Toute issue autre que conforme ou sautee
# termine par FATAL_ERROR (code 1 du script).
#
#   cmake -DCMD=<programme, chemin absolu> -DNARGS=<n> -DARG0=<a0> ... -DEXPECTED=<code> [-DEXPECT_LINE=<ligne>]
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
set(run_expect_skip_token "mhgp11_porte_sautee")

function(run_expect_say text)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${text}")
endfunction()

set(run_expect_guarded FALSE)
if(DEFINED REQUIRE_DIR_ENV AND NOT "${REQUIRE_DIR_ENV}" STREQUAL "")
  set(required_dir "$ENV{${REQUIRE_DIR_ENV}}")
  if("${required_dir}" STREQUAL "" OR NOT IS_DIRECTORY "${required_dir}")
    run_expect_say("${run_expect_skip_token} ${REQUIRE_DIR_ENV}")
    run_expect_say("run_expect_verdict sautee")
    return()
  endif()
  set(run_expect_guarded TRUE)
endif()

# Lancement : un chemin absolu seulement (un nom nu serait cherche dans PATH), par le shell du systeme.
if(NOT EXISTS "/bin/sh")
  message(FATAL_ERROR "run_expect.cmake exige /bin/sh")
endif()
if(NOT IS_ABSOLUTE "${CMD}")
  run_expect_say("run_expect_verdict lancement_impossible")
  message(FATAL_ERROR "programme donne par un chemin relatif : ${CMD}")
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
set(run_expect_code "execute_process(COMMAND [==[/bin/sh]==] [==[-c]==] [==[exec \"$0\" \"$@\"]==]")
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
if(run_expect_guarded)
  # Sortie retenue : elle n'atteint CTest qu'une fois debarrassee du jeton de saut.
  string(APPEND run_expect_code " RESULT_VARIABLE rc OUTPUT_VARIABLE run_stdout ERROR_VARIABLE run_stderr)")
  cmake_language(EVAL CODE "${run_expect_code}")
  string(FIND "${run_stdout}" "${run_expect_skip_token}" run_expect_in_out)
  string(FIND "${run_stderr}" "${run_expect_skip_token}" run_expect_in_err)
  string(REPLACE "${run_expect_skip_token}" "[jeton de saut retire]" run_expect_shown_out "${run_stdout}")
  string(REPLACE "${run_expect_skip_token}" "[jeton de saut retire]" run_expect_shown_err "${run_stderr}")
  message("${run_expect_shown_out}${run_expect_shown_err}")
  if(NOT run_expect_in_out EQUAL -1 OR NOT run_expect_in_err EQUAL -1)
    run_expect_say("run_expect_verdict jeton_usurpe")
    message(FATAL_ERROR "le programme ecrit lui-meme le jeton de saut : seul le precontrole des donnees peut sauter")
  endif()
else()
  string(APPEND run_expect_code " RESULT_VARIABLE rc OUTPUT_VARIABLE run_stdout ECHO_OUTPUT_VARIABLE)")
  cmake_language(EVAL CODE "${run_expect_code}")
endif()

if(NOT "${rc}" MATCHES "^[0-9]+$")
  run_expect_say("run_expect_verdict arret_anormal")
  message(FATAL_ERROR "termine par signal ou delai : ${rc}")
endif()
if("${rc}" STREQUAL "126" OR "${rc}" STREQUAL "127")
  run_expect_say("run_expect_verdict lancement_impossible")
  message(FATAL_ERROR "programme impossible a lancer (code ${rc} du shell : absent, non executable ou interprete "
                      "introuvable) : ${CMD}")
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
