# MorseHGP3D v9 — chantier actif (exploration_v9_hors_registre, not_claimed).
# C++20 sans extensions, -Wall -Wextra -Wpedantic -Werror. Boost est
# OBLIGATOIRE : les juges exacts (cpp_int, rationnels) ne disparaissent jamais
# en silence (defaut v8 corrige, cf. docs/PLAN_V9.md V9-0).
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v9 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

option(MHGP9_SANITIZE "Construire sous ASan et UBSan" OFF)
option(MHGP9_TSAN "Construire sous ThreadSanitizer" OFF)
if(MHGP9_SANITIZE AND MHGP9_TSAN)
  message(FATAL_ERROR "MHGP9_SANITIZE et MHGP9_TSAN sont exclusifs")
endif()
if(MHGP9_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
endif()
if(MHGP9_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

find_package(Threads REQUIRED)
find_path(MHGP9_BOOST_INCLUDE_DIR boost/multiprecision/cpp_int.hpp
          HINTS ${BOOST_ROOT}/include $ENV{BOOST_ROOT}/include)
if(NOT MHGP9_BOOST_INCLUDE_DIR)
  message(FATAL_ERROR "mhgp9: Boost headers are mandatory (libboost-dev, or -DBOOST_ROOT=<prefix> "
                      "pointing to an extracted libboost1.83-dev, see docs/PLAN_V9.md V9-0)")
endif()
enable_testing()

# Cible PRODUIT : aucun point d'injection de mutant compile.
function(mhgp9_product_executable name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Threads::Threads)
  target_include_directories(${name} SYSTEM PRIVATE ${MHGP9_BOOST_INCLUDE_DIR})
endfunction()

# Cible de TEST : les points d'injection MHGP9_MUTANT sont compiles.
function(mhgp9_test_executable name)
  mhgp9_product_executable(${name} ${ARGN})
  target_compile_definitions(${name} PRIVATE MHGP9_TESTING=1)
endfunction()

# Porte a code de sortie exact : 0 conforme, 1 juge, 2 refus, 3 plancher ou
# invariant, 4 mutant tue. Un crash par signal n'est jamais un succes.
function(mhgp9_gate name expected target)
  cmake_parse_arguments(G "" "TIMEOUT;EXPECT_LINE;EXPECT_PREFIX" "LABELS;ARGS" ${ARGN})
  string(JOIN " " joined ${G_ARGS})
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -DCMD=$<TARGET_FILE:${target}> "-DARGS=${joined}"
                   "-DEXPECT_LINE=${G_EXPECT_LINE}" "-DEXPECT_PREFIX=${G_EXPECT_PREFIX}"
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
  if(G_LABELS)
    set_tests_properties(${name} PROPERTIES LABELS "${G_LABELS}")
  else()
    set_tests_properties(${name} PROPERTIES LABELS "gate")
  endif()
  if(G_TIMEOUT)
    set_tests_properties(${name} PROPERTIES TIMEOUT ${G_TIMEOUT})
  endif()
endfunction()

# ---- Tour FULL (port de morsehgp3D_v7, dc57ffd5, au domaine u18).
# Les portes jugent la tour contre l'oracle rationnel borne ; le catalogue
# d'entree y est fourni par l'oracle (le producteur reel vient ensuite).
foreach(gate IN ITEMS full_certificate full_coverage_certificate anchor_meb local_plateau full_ball_tower arith_u18)
  mhgp9_product_executable(mhgp9_tower_${gate}_gate tests/tower/${gate}_gate.cpp)
  mhgp9_gate(mhgp9_tower_${gate} 0 mhgp9_tower_${gate}_gate
             ARGS --selftest LABELS "gate;tower;oracle" TIMEOUT 300)
  mhgp9_gate(mhgp9_tower_${gate}_bad_argument 2 mhgp9_tower_${gate}_gate
             ARGS --unknown LABELS "gate;tower" TIMEOUT 60)
endforeach()
# Banque de populations deplacee : meme banque que la surcharge copiante a
# 0..8 fils, memes refus, echec de lancement de fil = statut (MHGP9_TESTING).
mhgp9_test_executable(mhgp9_tower_population_bank_gate tests/tower/population_bank_gate.cpp)
mhgp9_gate(mhgp9_tower_population_bank 0 mhgp9_tower_population_bank_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 120)
# Tri parallele de la tour : permutation egale a std::sort pour tout nombre de
# fils, et son mutant compile (fusions remplacees par des copies) tue.
mhgp9_product_executable(mhgp9_tower_parallel_sort_gate tests/tower/parallel_sort_gate.cpp)
mhgp9_gate(mhgp9_tower_parallel_sort 0 mhgp9_tower_parallel_sort_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 300)
mhgp9_product_executable(mhgp9_tower_parallel_sort_mutant_gate tests/tower/parallel_sort_gate.cpp)
target_compile_definitions(mhgp9_tower_parallel_sort_mutant_gate PRIVATE MHGP9_PARALLEL_SORT_MUTANT_COPY_PAIRS=1)
mhgp9_gate(mhgp9_tower_parallel_sort_mutant_copy_pairs 1 mhgp9_tower_parallel_sort_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=parallel_sort.permutation" LABELS "gate;tower;mutant" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_full_certificate_rejects 0 mhgp9_tower_full_certificate_gate
           ARGS --rejects LABELS "gate;tower" TIMEOUT 60)
mhgp9_gate(mhgp9_tower_full_ball_static_cpu1 0 mhgp9_tower_full_ball_tower_gate
           ARGS --static-1 LABELS "gate;tower;oracle" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_full_ball_static_cpu4 0 mhgp9_tower_full_ball_tower_gate
           ARGS --static-4 LABELS "gate;tower;oracle" TIMEOUT 300)

# ---- Generateur exact (port de morsehgp3D_v8, 3f0d188f, espace mhgp9::gen).
# Port en bloc de la bibliotheque mhgp8_p0 (voie float32 exclue) ; le retrait
# des prototypes abandonnes suit la carte docs/audit_v8/16_carte_du_code_v8.md.
add_library(mhgp9_gen STATIC
  src/gen/pipeline/local_credits.cpp src/gen/pipeline/axis_q2.cpp
  src/gen/pipeline/q2_census.cpp src/gen/pipeline/q2_census_parallel.cpp
  src/gen/pipeline/prepared_cloud.cpp src/gen/wspd/front.cpp src/gen/lanes/q4_family.cpp
  src/gen/lanes/exact_ball.cpp src/gen/lanes/q34_seed.cpp
  src/gen/lanes/edge_cover.cpp src/gen/lanes/q34_cover.cpp
  src/gen/lanes/family_certificate.cpp src/gen/lanes/q34_pruning.cpp
  src/gen/lanes/q34_collective.cpp src/gen/lanes/q4_positive_domain.cpp
  src/gen/lanes/q4_center_map.cpp src/gen/lanes/q4_local_partition.cpp
  src/gen/lanes/q4_local.cpp src/gen/lanes/q4_shallow_set.cpp src/gen/lanes/q4_shallow.cpp
  src/gen/lanes/q4_window.cpp src/gen/pipeline/wspd_q34.cpp src/gen/lanes/q34_witness_search.cpp
  src/gen/lanes/q3_ball_census.cpp src/gen/lanes/q34_dead_lanes.cpp)
target_include_directories(mhgp9_gen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/gen")
target_link_libraries(mhgp9_gen PUBLIC Threads::Threads)

# ---- Chaine de bout en bout : generateur -> catalogue recoupe -> tour FULL.
add_library(mhgp9_chain STATIC src/chain/tower_chain.cpp)
target_include_directories(mhgp9_chain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/gen")
target_link_libraries(mhgp9_chain PUBLIC mhgp9_gen Threads::Threads)

mhgp9_product_executable(mhgp9_tower_probe bench/tower_probe.cpp)
target_link_libraries(mhgp9_tower_probe PRIVATE mhgp9_chain)

# Juge T2 sur la chaine reelle : catalogue compare a l'inventaire rationnel
# exhaustif, tour comparee au modele Gamma borne (n <= 14), K = 1..10,
# separations 8/10/12, un et quatre fils de chaine.
mhgp9_product_executable(mhgp9_chain_census_tower_gate tests/chain/chain_census_tower_gate.cpp)
target_link_libraries(mhgp9_chain_census_tower_gate PRIVATE mhgp9_chain)
foreach(kind IN ITEMS historical line12 shell14 spatial12 rejects)
  mhgp9_gate(mhgp9_chain_census_tower_${kind} 0 mhgp9_chain_census_tower_gate
             ARGS --${kind} LABELS "gate;chain;oracle" TIMEOUT 300)
endforeach()
# Voies de resolution de la tour sur un nuage de 1 500 sites : temporelle et
# statique 1/4/8 fils, meme tour ; le tri parallele des requetes y est exerce.
# Priorite des echecs de la tour (plus petit K sur les phases A et C) et
# travail paye compte apres echec : points de panne MHGP9_TESTING, chaine
# recompilee dans la cible de test (le generateur ne voit pas la tour).
mhgp9_test_executable(mhgp9_chain_order_failure_priority_gate
                      tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_order_failure_priority_gate PRIVATE mhgp9_gen)
mhgp9_gate(mhgp9_chain_order_failure_priority 0 mhgp9_chain_order_failure_priority_gate
           ARGS --selftest LABELS "gate;chain;tower" TIMEOUT 300)
foreach(mutant IN ITEMS PHASE_PRIORITY DROP_FAILED_STATS)
  string(TOLOWER ${mutant} lower)
  mhgp9_test_executable(mhgp9_chain_order_failure_${lower}_mutant_gate
                        tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(mhgp9_chain_order_failure_${lower}_mutant_gate PRIVATE mhgp9_gen)
  target_compile_definitions(mhgp9_chain_order_failure_${lower}_mutant_gate
                             PRIVATE MHGP9_FULL_ORDERS_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_chain_order_failure_mutant_phase_priority 1 mhgp9_chain_order_failure_phase_priority_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=priority" LABELS "gate;chain;tower;mutant" TIMEOUT 300)
mhgp9_gate(mhgp9_chain_order_failure_mutant_drop_failed_stats 1 mhgp9_chain_order_failure_drop_failed_stats_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=ledger.paid_work_lost" LABELS "gate;chain;tower;mutant" TIMEOUT 300)
mhgp9_product_executable(mhgp9_chain_static_paths_gate tests/chain/chain_static_paths_gate.cpp)
target_link_libraries(mhgp9_chain_static_paths_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_static_paths 0 mhgp9_chain_static_paths_gate
           ARGS --selftest LABELS "gate;chain" TIMEOUT 300)
mhgp9_gate(mhgp9_chain_census_tower_bad_argument 2 mhgp9_chain_census_tower_gate
           ARGS --unknown LABELS "gate;chain" TIMEOUT 60)
mhgp9_gate(mhgp9_chain_census_tower_mutant_assignment 1 mhgp9_chain_census_tower_gate
           ARGS --mutant-assignment EXPECT_LINE "cause=T2 unassigned subset"
           LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_chain_census_tower_mutant_open 1 mhgp9_chain_census_tower_gate
           ARGS --mutant-open EXPECT_LINE "cause=T2.historical.exact_Gamma"
           LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_chain_census_tower_mutant_adjacency 1 mhgp9_chain_census_tower_gate
           ARGS --mutant-adjacency EXPECT_LINE "cause=T2.historical.exact_Gamma"
           LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_chain_census_tower_mutant_census 1 mhgp9_chain_census_tower_gate
           ARGS --mutant-census EXPECT_LINE "cause=T2.census.exhaustive_ball_inventory"
           LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
# Refus d'arguments de la sonde (code 2) avant toute lecture d'entree.
mhgp9_gate(mhgp9_tower_probe_rejects_k_overflow 2 mhgp9_tower_probe
           ARGS absent.u32le 4294967297 1 LABELS "gate;probe" TIMEOUT 30)
mhgp9_gate(mhgp9_tower_probe_rejects_separation_below_8 2 mhgp9_tower_probe
           ARGS absent.u32le 5 1 --s=7 LABELS "gate;probe" TIMEOUT 30)
mhgp9_gate(mhgp9_tower_probe_rejects_grid_label 2 mhgp9_tower_probe
           ARGS absent.u32le 5 1 --grid=a/b LABELS "gate;probe" TIMEOUT 30)
# Raccord sonde REELLE / validateur du worker G4, avec la queue d'arguments
# exacte d'un cas G4 et des mutants de schema (session R2 du 23 septembre
# 2026 : treize sorties refusees faute d'une telle porte). Sous python3 -O aussi.
find_package(Python3 COMPONENTS Interpreter)
set(MHGP9_TOWER_WORKER ${CMAKE_CURRENT_SOURCE_DIR}/../gcp-migration/tower_worker_v9.py)
if(Python3_Interpreter_FOUND AND EXISTS ${MHGP9_TOWER_WORKER})
  foreach(mode IN ITEMS normal optimized)
    if(mode STREQUAL "optimized")
      set(py_flags "-B -O")
    else()
      set(py_flags "-B")
    endif()
    add_test(NAME mhgp9_probe_worker_contract_${mode}
             COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=${Python3_EXECUTABLE}
                     "-DARGS=${py_flags} ${CMAKE_CURRENT_SOURCE_DIR}/tests/chain/probe_worker_contract.py $<TARGET_FILE:mhgp9_tower_probe> ${MHGP9_TOWER_WORKER} ${CMAKE_CURRENT_BINARY_DIR}/probe_worker_contract_${mode}"
                     "-DEXPECT_PREFIX=probe_worker_contract mutants_killed=27/27"
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
    set_tests_properties(mhgp9_probe_worker_contract_${mode} PROPERTIES LABELS "gate;probe;protocol" TIMEOUT 300)
  endforeach()
endif()

# Mutant de compilation du noyau MEB (derniere paire maximale au lieu de la
# premiere) : la porte doit le refuser (code 1, support canonique faux).
mhgp9_product_executable(mhgp9_tower_anchor_meb_mutant_gate tests/tower/anchor_meb_gate.cpp)
target_compile_definitions(mhgp9_tower_anchor_meb_mutant_gate PRIVATE MHGP9_MEB_MUTANT_LAST_MAXIMUM=1)
mhgp9_gate(mhgp9_tower_anchor_meb_mutant_last_maximum 1 mhgp9_tower_anchor_meb_mutant_gate
           ARGS --selftest EXPECT_LINE "cause=support.first_positive_containing" LABELS "gate;tower;mutant" TIMEOUT 120)

# ---- Portes du generateur (port de morsehgp3D_v8/tests, 3f0d188f).
# Port mecanique des portes C++ v8 du chemin mesure, memes renommages que
# src/gen (mhgp8 -> mhgp9::gen, MHGP8_ -> MHGP9G_) ; fixtures, planchers et
# mutants compiles inchanges. Juges exacts Boost : tests/gen/exact_ball_oracle.hpp
# et tests/gen/oracle/{p0,q2_census}_oracle.hpp. En v8 chaque porte etait
# enregistree par add_test(<porte> --selftest), code attendu 0 ; les codes
# d'erreur v8 (1 sur toute exception, y compris d'usage pour la plupart) ne
# suivent pas encore la table 0/1/2/3/4 : seule la voie --selftest est jugee.
# tests/gen/q34_family_pruning_gate.cpp et q34_cover_gate.cpp servent aussi
# d'aides incluses par #define main (wspd_q34, q4_local, q4_seed_cells).
# Chaque porte est compilee une fois en bibliotheque OBJECT, partagee par
# l'executable produit et par ses mutants compiles.
function(mhgp9_gen_gate gate labels)
  add_library(mhgp9_gen_${gate}_gate_objects OBJECT tests/gen/${gate}_gate.cpp)
  target_link_libraries(mhgp9_gen_${gate}_gate_objects PRIVATE mhgp9_gen)
  target_include_directories(mhgp9_gen_${gate}_gate_objects SYSTEM PRIVATE ${MHGP9_BOOST_INCLUDE_DIR})
  mhgp9_product_executable(mhgp9_gen_${gate}_gate $<TARGET_OBJECTS:mhgp9_gen_${gate}_gate_objects>)
  target_link_libraries(mhgp9_gen_${gate}_gate PRIVATE mhgp9_gen)
  mhgp9_gate(mhgp9_gen_${gate} 0 mhgp9_gen_${gate}_gate
             ARGS --selftest LABELS "${labels}" TIMEOUT 600)
endfunction()
foreach(gate IN ITEMS wspd_q34 q4_local q4_seed_cells q4_saturating_atlas q3_ball_census
    exact_ball q34_witness_search q34_pair_bounds q34_cover q2_census wspd_front
    q2_prepared_bounds q2_joint_bounds q4_family q34_seed wspd_front_jobs q34_witness_cache
    q34_dead_lanes_owner)
  mhgp9_gen_gate(${gate} "gate;gen;oracle")
endforeach()
foreach(gate IN ITEMS wspd_q2_census u18_numeric_domain q2_node_pool q2_terminal_pool
    wspd_q2_parallel q2_sibling q2_witness_order q2_joint wspd_q2_proposals
    wspd_front_inheritance cloud_owner)
  mhgp9_gen_gate(${gate} "gate;gen")
endforeach()
# Refus d'argument (code 2) : seulement les portes v8 qui rendaient deja 2 sur
# une erreur d'usage. wspd_q34, q4_local, q4_seed_cells, q4_saturating_atlas,
# q3_ball_census, q34_witness_search, q34_pair_bounds et u18_numeric_domain
# rendent 1 (exception d'usage, comme en v8) : non enregistrees, a aligner.
foreach(gate IN ITEMS exact_ball q34_cover q2_census wspd_front q2_prepared_bounds
    q2_joint_bounds q4_family q34_seed wspd_front_jobs wspd_q2_census q2_node_pool
    q2_terminal_pool wspd_q2_parallel q2_sibling q2_witness_order q2_joint
    wspd_q2_proposals wspd_front_inheritance cloud_owner)
  mhgp9_gate(mhgp9_gen_${gate}_bad_argument 2 mhgp9_gen_${gate}_gate
             ARGS --unknown LABELS "gate;gen" TIMEOUT 60)
endforeach()
# Diagnostic d'echelle de l'atlas saturant (mode --scale de la porte v8, non
# enregistre en v8) : une arete fournie, racine certifiee profonde, aucun
# support q4 emis ; invariants verifies, temps non juges.
foreach(n IN ITEMS 8000 16000 32000)
  mhgp9_gate(mhgp9_gen_q4_saturating_atlas_scale${n} 0 mhgp9_gen_q4_saturating_atlas_gate
             ARGS --scale ${n} LABELS "gate;gen;scale${n}" TIMEOUT 120)
endforeach()

# ---- Mutants compiles du generateur (port des scripts v8 wspd_q34_mutations,
# q34_affine_mutations, q4_seed_cells_mutations, q4_saturating_mutations,
# q34_indexed_mutations). tests/gen/mutants.json donne, par mutant, l'unite de
# src/gen visee, les remplacements (sites exacts, uniques, appliques dans
# l'ordre) et le stderr causal exact. La copie mutee est ecrite au configure
# dans le build (src/gen n'est jamais modifie), compilee avec les memes options
# que mhgp9_gen et liee AVANT libmhgp9_gen.a, comme les scripts v8 liaient
# l'objet mute avant libmhgp8_p0.a. Un site absent ou non unique ne casse pas
# le build : le test correspondant echoue en le disant.
set(mhgp9_gen_mutant_spec "${CMAKE_CURRENT_SOURCE_DIR}/tests/gen/mutants.json")
file(READ "${mhgp9_gen_mutant_spec}" mhgp9_gen_mutant_text)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${mhgp9_gen_mutant_spec}")
string(JSON mhgp9_gen_mutant_count LENGTH "${mhgp9_gen_mutant_text}" mutants)
math(EXPR mhgp9_gen_mutant_last "${mhgp9_gen_mutant_count} - 1")
foreach(index RANGE ${mhgp9_gen_mutant_last})
  string(JSON family GET "${mhgp9_gen_mutant_text}" mutants ${index} family)
  string(JSON mutant GET "${mhgp9_gen_mutant_text}" mutants ${index} name)
  string(JSON gate GET "${mhgp9_gen_mutant_text}" mutants ${index} gate)
  string(JSON unit GET "${mhgp9_gen_mutant_text}" mutants ${index} source)
  string(JSON disabled ERROR_VARIABLE no_disabled GET "${mhgp9_gen_mutant_text}" mutants ${index} disabled)
  set(test_name mhgp9_gen_mutant_${family}_${mutant})
  set(original "${CMAKE_CURRENT_SOURCE_DIR}/src/gen/${unit}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
  file(READ "${original}" mutated_text)
  set(refusal "")
  string(JSON replacement_count LENGTH "${mhgp9_gen_mutant_text}" mutants ${index} replacements)
  math(EXPR replacement_last "${replacement_count} - 1")
  foreach(r RANGE ${replacement_last})
    string(JSON before GET "${mhgp9_gen_mutant_text}" mutants ${index} replacements ${r} 0)
    string(JSON after GET "${mhgp9_gen_mutant_text}" mutants ${index} replacements ${r} 1)
    string(FIND "${mutated_text}" "${before}" first)
    string(FIND "${mutated_text}" "${before}" last REVERSE)
    if(first EQUAL -1 OR NOT first EQUAL last)
      set(refusal "site de mutation absent ou non unique dans src/gen/${unit} (remplacement ${r})")
      break()
    endif()
    string(REPLACE "${before}" "${after}" mutated_text "${mutated_text}")
  endforeach()
  if(NOT refusal STREQUAL "")
    add_test(NAME ${test_name} COMMAND ${CMAKE_COMMAND} "-DREFUSE=${test_name}: ${refusal}"
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/gen/run_mutant.cmake)
  else()
    get_filename_component(unit_dir "${unit}" DIRECTORY)
    get_filename_component(unit_name "${unit}" NAME)
    set(mutated "${CMAKE_CURRENT_BINARY_DIR}/gen_mutants/${family}_${mutant}/${unit_name}")
    file(WRITE "${mutated}.staged" "${mutated_text}")
    configure_file("${mutated}.staged" "${mutated}" COPYONLY)
    mhgp9_product_executable(${test_name}_gate $<TARGET_OBJECTS:mhgp9_gen_${gate}_gate_objects> "${mutated}")
    target_include_directories(${test_name}_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gen"
                               "${CMAKE_CURRENT_SOURCE_DIR}/src/gen/${unit_dir}")
    target_link_libraries(${test_name}_gate PRIVATE mhgp9_gen)
    add_test(NAME ${test_name}
             COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:${test_name}_gate>
                     -DSPEC=${mhgp9_gen_mutant_spec} -DINDEX=${index}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/gen/run_mutant.cmake)
  endif()
  set_tests_properties(${test_name} PROPERTIES LABELS "gate;gen;mutant" TIMEOUT 300)
  if(no_disabled STREQUAL "NOTFOUND")
    set_tests_properties(${test_name} PROPERTIES DISABLED TRUE)
    message(STATUS "mhgp9: ${test_name} DISABLED (${disabled})")
  endif()
endforeach()
