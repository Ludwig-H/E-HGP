# Portes de l'oracle de reference exact (Python 3.10 nu, aucune construction C++). Inclus par CMakeLists.txt quand
# l'unite "reference" est demandee ; aides : cmake/gates.cmake. Ce que chaque etage etablit : reference/README.md.
#
#   mhgp11_reference_fast               15 faits graves (dont l'attendu d'intervalles sur 251 multiensembles alignes),
#                                       puis etage B contre etage A sur la suite rapide (342 nuages), compteurs
#                                       exacts ; une vingtaine de secondes sur un coeur
#   mhgp11_reference_fast_split         la meme suite en 3 processus (tranches, rapports, somme) : memes compteurs
#   mhgp11_reference_refusal            usage faux : code 2
#   mhgp11_reference_mutant_<nom>       mutant applique a une copie de hgp11_ref : code 4 (tue) ; code 0 et ligne
#                                       mutant_survives pour un mutant declare equivalent (liste : ref_mutants.py)
#   mhgp11_reference_full_<i>           tranche i de la suite complete (5 617 nuages, environ 35 minutes de CPU en
#                                       tout sous Python 3.10) ; label long
#   mhgp11_reference_full               somme des tranches, faits graves, planchers ; exige toutes les tranches
#   mhgp11_reference_diff_v10           dumps du catalogue et de la tour identiques, octet pour octet, a ceux du
#   mhgp11_reference_diff_v10_mutant_*  binaire fige de la v10 ; mutants de serialisation
#
# Les portes diff_v10 exigent les binaires figes mhgp10_catalogue et mhgp10_tower (commit c764e121a de la v10,
# construits en Release depuis git archive) dans le dossier MHGP11_V10_FROZEN_DIR (variable de cache, initialisee par
# la variable d'environnement du meme nom). Sans eux, ces portes ne sont pas enregistrees et la configuration le dit.
set(MHGP11_REFERENCE_SHARDS 16 CACHE STRING "Tranches de la suite complete de la reference : une porte par tranche")
set(MHGP11_V10_FROZEN_DIR "$ENV{MHGP11_V10_FROZEN_DIR}" CACHE PATH
    "Dossier des binaires figes mhgp10_catalogue et mhgp10_tower (v10, commit c764e121a) ; vide : portes diff_v10 absentes")
if(NOT MHGP11_REFERENCE_SHARDS MATCHES "^[1-9][0-9]?[0-9]?$" OR MHGP11_REFERENCE_SHARDS GREATER 256)
  message(FATAL_ERROR "MHGP11_REFERENCE_SHARDS=${MHGP11_REFERENCE_SHARDS} : entier de 1 a 256 attendu")
endif()

set(mhgp11_reference_dir ${CMAKE_CURRENT_LIST_DIR})

mhgp11_python_gate(mhgp11_reference_projection_contracts 0 test_projection_contracts.py
                   LINE "projection_contracts_ok faits=4" LABELS oracle fast)

mhgp11_python_gate(mhgp11_reference_fast 0 test_ref.py --suite=fast
                   LINE "reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029" LABELS oracle fast)
mhgp11_python_gate(mhgp11_reference_fast_split 0 test_ref.py --suite=fast --jobs=3
                   LINE "reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029" LABELS oracle fast)
# cette porte et sa jumelle sous python3 -O lancent trois processus : CTest les compte
foreach(gate mhgp11_reference_fast_split mhgp11_reference_fast_split_opt)
  if(TEST ${gate})
    set_property(TEST ${gate} PROPERTY PROCESSORS 3)
  endif()
endforeach()
mhgp11_python_gate(mhgp11_reference_refusal 2 test_ref.py --suite=absente LABELS oracle fast)

# Mutants : la liste vient de ref_mutants.py, lue a la configuration (aucun mutant sans porte).
function(mhgp11_reference_mutant_list out script)
  execute_process(COMMAND ${Python3_EXECUTABLE} -B ${mhgp11_reference_dir}/${script} --list-mutants
                  RESULT_VARIABLE rc OUTPUT_VARIABLE listing OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT rc STREQUAL "0" OR listing STREQUAL "")
    message(FATAL_ERROR "liste des mutants de la reference illisible (${script}, code ${rc})")
  endif()
  string(REPLACE "\n" ";" lines "${listing}")
  set(${out} "${lines}" PARENT_SCOPE)
endfunction()

mhgp11_reference_mutant_list(mhgp11_reference_mutants test_ref.py)
foreach(line IN LISTS mhgp11_reference_mutants)
  string(REPLACE " " ";" words "${line}")
  list(GET words 0 mutant)
  list(GET words 1 kind)
  if(kind STREQUAL "equivalent")
    mhgp11_python_gate(mhgp11_reference_mutant_${mutant} 0 test_ref.py --inject=${mutant}
                       LINE "mutant_survives ${mutant}" LABELS oracle fast)
  else()
    mhgp11_python_gate(mhgp11_reference_mutant_${mutant} 4 test_ref.py --inject=${mutant}
                       LINE "mutant_killed ${mutant}" LABELS oracle fast)
  endif()
endforeach()

# Suite complete : une porte par tranche (CTest les joue en parallele), puis la porte qui somme leurs rapports. Les
# tranches sont une fixture CTest : choisir la porte de somme entraine les tranches ; une tranche en echec la laisse
# non jouee.
set(mhgp11_reference_reports ${PROJECT_BINARY_DIR}/reference_full)
math(EXPR mhgp11_reference_last "${MHGP11_REFERENCE_SHARDS} - 1")
foreach(shard RANGE 0 ${mhgp11_reference_last})
  mhgp11_python_gate(mhgp11_reference_full_${shard} 0 test_ref.py --suite=full
                     --shard=${shard}/${MHGP11_REFERENCE_SHARDS}
                     --report=${mhgp11_reference_reports}/full_${shard}_${MHGP11_REFERENCE_SHARDS}.json
                     LABELS oracle long TIMEOUT 1800)
  set_property(TEST mhgp11_reference_full_${shard} PROPERTY FIXTURES_SETUP mhgp11_reference_full_shards)
endforeach()
mhgp11_python_gate(mhgp11_reference_full 0 test_ref.py --suite=full --collect=${mhgp11_reference_reports}
                   --shards=${MHGP11_REFERENCE_SHARDS} LABELS oracle long TIMEOUT 600)
set_property(TEST mhgp11_reference_full PROPERTY FIXTURES_REQUIRED mhgp11_reference_full_shards)

# Differentiel contre le binaire fige de la v10.
if(MHGP11_V10_FROZEN_DIR AND EXISTS "${MHGP11_V10_FROZEN_DIR}/mhgp10_catalogue"
   AND EXISTS "${MHGP11_V10_FROZEN_DIR}/mhgp10_tower")
  mhgp11_python_gate(mhgp11_reference_diff_v10 0 test_dump_v10.py --frozen-dir=${MHGP11_V10_FROZEN_DIR}
                     LINE "reference_diff_v10_ok nuages=190 catalogues=190 tours=640 lignes=26530"
                     LABELS diff_v10 fast)
  # grands nuages (24 a 32 points, K jusqu'a 10), etage B seul : 1 256 sauts de descente confirmes par le binaire
  mhgp11_python_gate(mhgp11_reference_diff_v10_large 0 test_dump_v10.py --frozen-dir=${MHGP11_V10_FROZEN_DIR} --large
                     LINE "reference_diff_v10_large_ok nuages=6 catalogues=6 tours=12 lignes=24698"
                     LABELS diff_v10 fast)
  mhgp11_python_gate(mhgp11_reference_diff_v10_refusal 2 test_dump_v10.py
                     --frozen-dir=${PROJECT_BINARY_DIR}/mhgp11_dossier_absent LABELS diff_v10 fast)
  mhgp11_reference_mutant_list(mhgp11_reference_dump_mutants test_dump_v10.py)
  # "ecriture" : mutant de la seule ecriture non reduite des niveaux, tue octet pour octet et indiscernable une fois
  # les niveaux reduits ; "objet" : mutant d'un ordre, d'un rang, d'un drapeau ou d'un choix.
  foreach(line IN LISTS mhgp11_reference_dump_mutants)
    string(REPLACE " " ";" words "${line}")
    list(GET words 0 mutant)
    list(GET words 1 kind)
    set(killed "mutant_killed ${mutant}")
    if(kind STREQUAL "ecriture")
      set(killed "mutant_killed ${mutant} ecriture_seule")
    endif()
    mhgp11_python_gate(mhgp11_reference_diff_v10_mutant_${mutant} 4 test_dump_v10.py
                       --frozen-dir=${MHGP11_V10_FROZEN_DIR} --inject=${mutant} LINE "${killed}" LABELS diff_v10 fast)
  endforeach()
else()
  message(STATUS "mhgp11 : portes diff_v10 de la reference absentes (MHGP11_V10_FROZEN_DIR='${MHGP11_V10_FROZEN_DIR}' "
                 "sans mhgp10_catalogue ni mhgp10_tower)")
endif()
