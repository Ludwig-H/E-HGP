# Porte de refus : une commande temoin doit reussir ; la meme commande, suivie d'options de refus, doit echouer avec
# un jeton grave dans sa sortie (standard ou d'erreur). Sans temoin, un refus pour une autre cause (inclusion absente,
# option inconnue) rendrait la porte verte par vacuite : lecon du raccord R2 de la v10
# (tests/regression/site_tree_fast_math.cmake, controle positif), dont ce script generalise le principe.
#
# Sert aux refus a la compilation (static_assert, #error : cmake/gates.cmake, mhgp11_expect_compile_failure) et aux
# refus a la configuration. Arguments passes comme dans run_expect.cmake (une definition par argument).
#
# Verdict : une ligne "expect_refusal_verdict <mot>" sur la sortie standard, <mot> parmi conforme, temoin_en_echec,
# refus_absent, jeton_absent, arret_anormal. Toute issue autre que conforme termine par FATAL_ERROR (code 1).
#
#   cmake -DCMD=<programme> -DNARGS=<n> -DARG0=<a0> ... -DNREFUSAL=<m> -DREFUSAL0=<r0> ... -DTOKEN=<jeton>
#         [-DSCRATCH=<dossier efface avant chaque execution>] -P expect_refusal.cmake
cmake_minimum_required(VERSION 3.20)  # politiques recentes : une chaine entre guillemets n'est jamais dereferencee
if(NOT DEFINED CMD OR NOT DEFINED TOKEN OR "${TOKEN}" STREQUAL "")
  message(FATAL_ERROR "expect_refusal.cmake exige CMD et TOKEN")
endif()
foreach(count NARGS NREFUSAL)
  if(NOT DEFINED ${count})
    set(${count} 0)
  endif()
  if(NOT "${${count}}" MATCHES "^[0-9]+$")
    message(FATAL_ERROR "expect_refusal.cmake : ${count}=${${count}} n'est pas un entier")
  endif()
endforeach()
if(NREFUSAL EQUAL 0)
  message(FATAL_ERROR "expect_refusal.cmake : aucune option de refus (NREFUSAL=0), la porte serait vide")
endif()

function(expect_refusal_say text)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${text}")
endfunction()

# Ajoute a `code` les mots <prefix>0 .. <prefix><count-1>, chacun entre crochets (aucun redecoupage).
function(expect_refusal_words code prefix count)
  set(text "${${code}}")
  if(count GREATER 0)
    math(EXPR last "${count} - 1")
    foreach(i RANGE 0 ${last})
      if(NOT DEFINED ${prefix}${i})
        message(FATAL_ERROR "expect_refusal.cmake : ${prefix}${i} absent")
      endif()
      string(FIND "${${prefix}${i}}" "]==]" bad)
      if(NOT bad EQUAL -1)
        message(FATAL_ERROR "expect_refusal.cmake : ${prefix}${i} contient ]==]")
      endif()
      string(APPEND text " [==[${${prefix}${i}}]==]")
    endforeach()
  endif()
  set(${code} "${text}" PARENT_SCOPE)
endfunction()

string(FIND "${CMD}" "]==]" expect_refusal_bad)
if(NOT expect_refusal_bad EQUAL -1)
  message(FATAL_ERROR "expect_refusal.cmake : CMD contient ]==]")
endif()
set(expect_refusal_base "execute_process(COMMAND [==[${CMD}]==]")
expect_refusal_words(expect_refusal_base ARG ${NARGS})
set(expect_refusal_tail " RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)")

# 1. Temoin : sans les options de refus, la commande reussit.
if(DEFINED SCRATCH AND NOT "${SCRATCH}" STREQUAL "")
  file(REMOVE_RECURSE "${SCRATCH}")
endif()
cmake_language(EVAL CODE "${expect_refusal_base}${expect_refusal_tail}")
if(NOT "${rc}" STREQUAL "0")
  expect_refusal_say("expect_refusal_verdict temoin_en_echec")
  message(FATAL_ERROR "temoin : code ${rc}, attendu 0 :\n${out}\n${err}")
endif()

# 2. Refus : avec les options, la commande echoue (code numerique non nul) et sa sortie porte le jeton.
if(DEFINED SCRATCH AND NOT "${SCRATCH}" STREQUAL "")
  file(REMOVE_RECURSE "${SCRATCH}")
endif()
set(expect_refusal_full "${expect_refusal_base}")
expect_refusal_words(expect_refusal_full REFUSAL ${NREFUSAL})
cmake_language(EVAL CODE "${expect_refusal_full}${expect_refusal_tail}")
if(NOT "${rc}" MATCHES "^[0-9]+$")
  expect_refusal_say("expect_refusal_verdict arret_anormal")
  message(FATAL_ERROR "refus : termine par signal, delai ou erreur d'execution : ${rc}")
endif()
if("${rc}" STREQUAL "0")
  expect_refusal_say("expect_refusal_verdict refus_absent")
  message(FATAL_ERROR "refus : la commande reussit avec les options de refus, un echec etait attendu")
endif()
string(FIND "${out}\n${err}" "${TOKEN}" expect_refusal_pos)
if(expect_refusal_pos EQUAL -1)
  expect_refusal_say("expect_refusal_verdict jeton_absent")
  message(FATAL_ERROR "refus : code ${rc} sans le jeton grave ${TOKEN} :\n${out}\n${err}")
endif()
if(DEFINED SCRATCH AND NOT "${SCRATCH}" STREQUAL "")
  file(REMOVE_RECURSE "${SCRATCH}")
endif()
expect_refusal_say("expect_refusal_verdict conforme ${TOKEN}")
