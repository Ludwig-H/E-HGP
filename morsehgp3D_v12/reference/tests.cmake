# Portes de l'oracle de reference exact (Python 3.10 nu, aucune construction C++). Inclus par CMakeLists.txt quand
# l'unite "reference" est demandee ; aides : cmake/gates.cmake. Ce que chaque etage etablit : reference/README.md.
#
#   mhgp12_reference_fast               15 faits graves (dont l'attendu d'intervalles sur 251 multiensembles alignes),
#                                       puis etage B contre etage A sur la suite rapide (342 nuages), compteurs
#                                       exacts ; une vingtaine de secondes sur un coeur
#   mhgp12_reference_fast_split         la meme suite en 3 processus (tranches, rapports, somme) : memes compteurs
#   mhgp12_reference_refusal            usage faux : code 2
#   mhgp12_reference_witness_t1         temoin WIT-T1-CARRE (LEM-T1 exige S inclus dans F) : 5 faits graves, 8 couples
#                                       support-partie ; mutant sans_inclusion : code 4 ; option inconnue : code 2
#   mhgp12_reference_transition         lecteur de transition du catalogue (T1, CONTRAT_CATALOGUE.md paragraphes 6.1
#                                       et 8 bis) : attendus graves des 6 temoins contre l'oracle (etage B), puis les
#                                       49 cas juges dans le processus (codes, categories exactes, lignes, motifs de
#                                       refus), trois d'entre eux par la ligne de commande du lecteur ; moins d'une
#                                       seconde
#   mhgp12_reference_transition_refusal usage faux : code 2
#   mhgp12_reference_transition_cas_<nom>     un cas par la ligne de commande du lecteur : son code (0 conforme, 1
#                                       desaccord, 2 refus d'entree), avec ses categories, sa ligne ou son motif
#                                       (liste : test_transition_catalogue.py --list-cases)
#   mhgp12_reference_transition_mutant_<nom>  mutant du lecteur (copie modifiee) : code 4, tue par chacun de ses cas
#                                       avec le code declare ; code 0 et ligne mutant_survives pour une garde defensive
#                                       declaree equivalente (liste : test_transition_catalogue.py --list-mutants)
#   mhgp12_reference_mutant_<nom>       mutant applique a une copie de hgp12_ref : code 4 (tue) ; code 0 et ligne
#                                       mutant_survives pour un mutant declare equivalent (liste : ref_mutants.py)
#   mhgp12_reference_full_<i>           tranche i de la suite complete (5 617 nuages, environ 35 minutes de CPU en
#                                       tout sous Python 3.10) ; label long
#   mhgp12_reference_full               somme des tranches, faits graves, planchers ; exige toutes les tranches
#   mhgp12_reference_diff_v10           dumps du catalogue et de la tour identiques, octet pour octet, a ceux du
#   mhgp12_reference_diff_v10_mutant_*  binaire fige de la v10 ; mutants de serialisation
#   mhgp12_reference_supports           oracle borne des supports d'ordre K (tranche S1, etage A seul) : faits graves
#                                       des fixtures de la spec et de l'audit (temoins D2 et E5 compris), puis 210
#                                       nuages a positions distinctes, K <= min(5, n - 1) : lemmes A a H et W,
#                                       invariance (permutation effective, reetiquetage, translation), compteurs
#                                       exacts, planchers, empreintes ; une trentaine de secondes sur un coeur sous
#                                       Python 3.10
#   mhgp12_reference_supports_refusal   usage faux : code 2
#   mhgp12_reference_supports_primitives primitives de l'oracle des supports sur sphere5 (24 sites, coquille mixte au
#                                       plafond natif ; apport des auditeurs du 5 octobre 2026) : Q_b par Gram, N_j
#                                       pour j <= 4 par combinaisons, comptes de K1 a K3, refus explicite (budget) de la
#                                       force brute du lemme F ; deux secondes : label fast, et non long
#   mhgp12_reference_supports_mutant_<nom>  mutant de l'oracle des supports (liste : test_supports.py
#                                       --list-mutants, table SUPPORT_MUTANTS de ref_mutants.py) : code 4, tue par sa
#                                       cause ; six d'entre eux prouvent la vivacite d'un controle (W.4, H, C.3, regle
#                                       du parent, vie d'une interne, M1)
#
# Les portes diff_v10 exigent les binaires figes mhgp10_catalogue et mhgp10_tower (commit c764e121a de la v10,
# construits en Release depuis git archive) dans le dossier MHGP12_V10_FROZEN_DIR (variable de cache, initialisee par
# la variable d'environnement du meme nom). Sans eux, ces portes ne sont pas enregistrees et la configuration le dit.
set(MHGP12_REFERENCE_SHARDS 16 CACHE STRING "Tranches de la suite complete de la reference : une porte par tranche")
set(MHGP12_V10_FROZEN_DIR "$ENV{MHGP12_V10_FROZEN_DIR}" CACHE PATH
    "Dossier des binaires figes mhgp10_catalogue et mhgp10_tower (v10, commit c764e121a) ; vide : portes diff_v10 absentes")
if(NOT MHGP12_REFERENCE_SHARDS MATCHES "^[1-9][0-9]?[0-9]?$" OR MHGP12_REFERENCE_SHARDS GREATER 256)
  message(FATAL_ERROR "MHGP12_REFERENCE_SHARDS=${MHGP12_REFERENCE_SHARDS} : entier de 1 a 256 attendu")
endif()

set(mhgp12_reference_dir ${CMAKE_CURRENT_LIST_DIR})

mhgp12_python_gate(mhgp12_reference_projection_contracts 0 test_projection_contracts.py
                   LINE "projection_contracts_ok faits=5" LABELS oracle fast)

# Temoin WIT-T1-CARRE (constat CST-0101) : LEM-T1 exige S inclus dans F ; la porte, son mutant et son refus.
mhgp12_python_gate(mhgp12_reference_witness_t1 0 test_witness_t1.py
                   LINE "wit_t1_carre_ok faits=5 couples_lemme=8" LABELS oracle fast)
mhgp12_python_gate(mhgp12_reference_witness_t1_mutant_sans_inclusion 4 test_witness_t1.py --mutant=sans_inclusion
                   LABELS oracle fast mutant)
mhgp12_python_gate(mhgp12_reference_witness_t1_refusal 2 test_witness_t1.py --option-inconnue LABELS oracle fast)

# Regle de resolution de la v12 (CONTRAT_TOUR.md, paragraphe 4.1) : arret sur la premiere cellule de fenetre, cibles
# << cellule >> lues sur la cellule deja traitee, trois politiques de saut, contre l'etage B sur la suite rapide ;
# fait grave du repli de G-L3 ; mutants de la regle ; refus d'usage.
mhgp12_python_gate(mhgp12_reference_resolution_v12 0 test_resolution_v12.py
                   LINE "resolution_v12_ok nuages=342 ordres=1362 politiques=3 cibles=18392 cellules=8448 inertes=1113 faits=4"
                   LABELS oracle fast)
foreach(mutant inertes_omises element_sans_racine arret_sous_fenetre plateau_coupe)
  mhgp12_python_gate(mhgp12_reference_resolution_v12_mutant_${mutant} 4 test_resolution_v12.py --inject=${mutant}
                     LABELS oracle fast mutant)
endforeach()
mhgp12_python_gate(mhgp12_reference_resolution_v12_refusal 2 test_resolution_v12.py --option-inconnue
                   LABELS oracle fast)

# Lecteur de transition du catalogue (tranche T1) : suite, refus d'usage, un cas par porte, mutants du lecteur. Les
# listes des cas et des mutants sont lues a la configuration (aucun cas ni mutant sans porte).
mhgp12_python_gate(mhgp12_reference_transition 0 test_transition_catalogue.py
                   LINE "transition_temoins_ok temoins=6 cas=49 conformes=10 desaccords=25 refus=14 boules=55 sstar_changes=4"
                   LABELS oracle fast)
mhgp12_python_gate(mhgp12_reference_transition_refusal 2 test_transition_catalogue.py --option-inconnue
                   LABELS oracle fast)
function(mhgp12_reference_transition_list out option)
  execute_process(COMMAND ${Python3_EXECUTABLE} -B ${mhgp12_reference_dir}/test_transition_catalogue.py ${option}
                  RESULT_VARIABLE rc OUTPUT_VARIABLE listing OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT rc STREQUAL "0" OR listing STREQUAL "")
    message(FATAL_ERROR "liste ${option} du lecteur de transition illisible (code ${rc})")
  endif()
  string(REPLACE "\n" ";" lines "${listing}")
  set(${out} "${lines}" PARENT_SCOPE)
endfunction()
mhgp12_reference_transition_list(mhgp12_transition_cases --list-cases)
foreach(line IN LISTS mhgp12_transition_cases)
  string(REPLACE " " ";" words "${line}")
  list(GET words 0 case)
  list(GET words 1 code)
  mhgp12_python_gate(mhgp12_reference_transition_cas_${case} ${code} test_transition_catalogue.py --cas=${case}
                     LABELS oracle fast)
endforeach()
mhgp12_reference_transition_list(mhgp12_transition_mutants --list-mutants)
foreach(line IN LISTS mhgp12_transition_mutants)
  string(REPLACE " " ";" words "${line}")
  list(GET words 0 mutant)
  list(GET words 1 kind)
  if(kind STREQUAL "equivalent")
    mhgp12_python_gate(mhgp12_reference_transition_mutant_${mutant} 0 test_transition_catalogue.py
                       --inject=${mutant} LINE "mutant_survives ${mutant}" LABELS oracle fast mutant)
  else()
    mhgp12_python_gate(mhgp12_reference_transition_mutant_${mutant} 4 test_transition_catalogue.py
                       --inject=${mutant} LINE "mutant_killed ${mutant}" LABELS oracle fast mutant)
  endif()
endforeach()

mhgp12_python_gate(mhgp12_reference_fast 0 test_ref.py --suite=fast
                   LINE "reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029" LABELS oracle fast)
mhgp12_python_gate(mhgp12_reference_fast_split 0 test_ref.py --suite=fast --jobs=3
                   LINE "reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029" LABELS oracle fast)
# cette porte et sa jumelle sous python3 -O lancent trois processus : CTest les compte
foreach(gate mhgp12_reference_fast_split mhgp12_reference_fast_split_opt)
  if(TEST ${gate})
    set_property(TEST ${gate} PROPERTY PROCESSORS 3)
  endif()
endforeach()
mhgp12_python_gate(mhgp12_reference_refusal 2 test_ref.py --suite=absente LABELS oracle fast)

# Mutants : la liste vient de ref_mutants.py, lue a la configuration (aucun mutant sans porte).
function(mhgp12_reference_mutant_list out script)
  execute_process(COMMAND ${Python3_EXECUTABLE} -B ${mhgp12_reference_dir}/${script} --list-mutants
                  RESULT_VARIABLE rc OUTPUT_VARIABLE listing OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT rc STREQUAL "0" OR listing STREQUAL "")
    message(FATAL_ERROR "liste des mutants de la reference illisible (${script}, code ${rc})")
  endif()
  string(REPLACE "\n" ";" lines "${listing}")
  set(${out} "${lines}" PARENT_SCOPE)
endfunction()

mhgp12_reference_mutant_list(mhgp12_reference_mutants test_ref.py)
foreach(line IN LISTS mhgp12_reference_mutants)
  string(REPLACE " " ";" words "${line}")
  list(GET words 0 mutant)
  list(GET words 1 kind)
  if(kind STREQUAL "equivalent")
    mhgp12_python_gate(mhgp12_reference_mutant_${mutant} 0 test_ref.py --inject=${mutant}
                       LINE "mutant_survives ${mutant}" LABELS oracle fast)
  else()
    mhgp12_python_gate(mhgp12_reference_mutant_${mutant} 4 test_ref.py --inject=${mutant}
                       LINE "mutant_killed ${mutant}" LABELS oracle fast)
  endif()
endforeach()

# Oracle borne des supports (tranche S1 de la sortie parametree) : etage A seul, charge sans le paquet.
set(mhgp12_reference_supports_line
    "reference_supports_ok nuages=210 ordres=951 boules=15062 supports=16943 noeuds=12441 coupes=48074")
mhgp12_python_gate(mhgp12_reference_supports 0 test_supports.py LINE "${mhgp12_reference_supports_line}"
                   LABELS oracle fast)
mhgp12_python_gate(mhgp12_reference_supports_refusal 2 test_supports.py --inject=absent LABELS oracle fast)
mhgp12_python_gate(mhgp12_reference_supports_primitives 0 test_supports.py --suite=primitives
                   LINE "reference_supports_primitives_ok sites=24 supports=828 q2=12 q3=24 q4=792 N2=12 N3=288 N4=3906 refus=2"
                   LABELS oracle fast)
mhgp12_reference_mutant_list(mhgp12_reference_supports_mutants test_supports.py)
foreach(line IN LISTS mhgp12_reference_supports_mutants)
  string(REPLACE " " ";" words "${line}")
  list(GET words 0 mutant)
  list(GET words 1 kind)
  if(kind STREQUAL "equivalent")
    mhgp12_python_gate(mhgp12_reference_supports_mutant_${mutant} 0 test_supports.py --inject=${mutant}
                       LINE "mutant_survives ${mutant}" LABELS oracle fast)
  else()
    mhgp12_python_gate(mhgp12_reference_supports_mutant_${mutant} 4 test_supports.py --inject=${mutant}
                       LINE "mutant_killed ${mutant}" LABELS oracle fast)
  endif()
endforeach()

# Suite complete : une porte par tranche (CTest les joue en parallele), puis la porte qui somme leurs rapports. Les
# tranches sont une fixture CTest : choisir la porte de somme entraine les tranches ; une tranche en echec la laisse
# non jouee.
set(mhgp12_reference_reports ${PROJECT_BINARY_DIR}/reference_full)
math(EXPR mhgp12_reference_last "${MHGP12_REFERENCE_SHARDS} - 1")
foreach(shard RANGE 0 ${mhgp12_reference_last})
  mhgp12_python_gate(mhgp12_reference_full_${shard} 0 test_ref.py --suite=full
                     --shard=${shard}/${MHGP12_REFERENCE_SHARDS}
                     --report=${mhgp12_reference_reports}/full_${shard}_${MHGP12_REFERENCE_SHARDS}.json
                     LABELS oracle long TIMEOUT 1800)
  set_property(TEST mhgp12_reference_full_${shard} PROPERTY FIXTURES_SETUP mhgp12_reference_full_shards)
endforeach()
mhgp12_python_gate(mhgp12_reference_full 0 test_ref.py --suite=full --collect=${mhgp12_reference_reports}
                   --shards=${MHGP12_REFERENCE_SHARDS} LABELS oracle long TIMEOUT 600)
set_property(TEST mhgp12_reference_full PROPERTY FIXTURES_REQUIRED mhgp12_reference_full_shards)

# Differentiel contre le binaire fige de la v10.
if(MHGP12_V10_FROZEN_DIR AND EXISTS "${MHGP12_V10_FROZEN_DIR}/mhgp10_catalogue"
   AND EXISTS "${MHGP12_V10_FROZEN_DIR}/mhgp10_tower")
  mhgp12_python_gate(mhgp12_reference_diff_v10 0 test_dump_v10.py --frozen-dir=${MHGP12_V10_FROZEN_DIR}
                     LINE "reference_diff_v10_ok nuages=190 catalogues=190 tours=640 lignes=26530"
                     LABELS diff_v10 fast)
  # grands nuages (24 a 32 points, K jusqu'a 10), etage B seul : 1 256 sauts de descente confirmes par le binaire
  mhgp12_python_gate(mhgp12_reference_diff_v10_large 0 test_dump_v10.py --frozen-dir=${MHGP12_V10_FROZEN_DIR} --large
                     LINE "reference_diff_v10_large_ok nuages=6 catalogues=6 tours=12 lignes=24698"
                     LABELS diff_v10 fast)
  mhgp12_python_gate(mhgp12_reference_diff_v10_refusal 2 test_dump_v10.py
                     --frozen-dir=${PROJECT_BINARY_DIR}/mhgp12_dossier_absent LABELS diff_v10 fast)
  mhgp12_reference_mutant_list(mhgp12_reference_dump_mutants test_dump_v10.py)
  # "ecriture" : mutant de la seule ecriture non reduite des niveaux, tue octet pour octet et indiscernable une fois
  # les niveaux reduits ; "objet" : mutant d'un ordre, d'un rang, d'un drapeau ou d'un choix.
  foreach(line IN LISTS mhgp12_reference_dump_mutants)
    string(REPLACE " " ";" words "${line}")
    list(GET words 0 mutant)
    list(GET words 1 kind)
    set(killed "mutant_killed ${mutant}")
    if(kind STREQUAL "ecriture")
      set(killed "mutant_killed ${mutant} ecriture_seule")
    endif()
    mhgp12_python_gate(mhgp12_reference_diff_v10_mutant_${mutant} 4 test_dump_v10.py
                       --frozen-dir=${MHGP12_V10_FROZEN_DIR} --inject=${mutant} LINE "${killed}" LABELS diff_v10 fast)
  endforeach()
else()
  message(STATUS "mhgp12 : portes diff_v10 de la reference absentes (MHGP12_V10_FROZEN_DIR='${MHGP12_V10_FROZEN_DIR}' "
                 "sans mhgp10_catalogue ni mhgp10_tower)")
endif()
