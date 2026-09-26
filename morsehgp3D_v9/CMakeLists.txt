# MorseHGP3D v9 — chantier actif (exploration_v9_hors_registre, not_claimed).
# C++20 sans extensions, -Wall -Wextra -Wpedantic -Werror. Boost est
# OBLIGATOIRE : les juges exacts (cpp_int, rationnels) ne disparaissent jamais
# en silence (defaut v8 corrige, cf. docs/PLAN_V9.md V9-0).
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v9 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# C++ only: the optional CUDA unit (MHGP9_ENABLE_CUDA) has its own flags.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>")

option(MHGP9_SANITIZE "Construire sous ASan et UBSan" OFF)
option(MHGP9_TSAN "Construire sous ThreadSanitizer" OFF)
option(MHGP9_ENABLE_CUDA "Construire le filtre temoin sur GPU (sm_120, sonde S1)" OFF)
if(MHGP9_SANITIZE AND MHGP9_TSAN)
  message(FATAL_ERROR "MHGP9_SANITIZE et MHGP9_TSAN sont exclusifs")
endif()
if(MHGP9_ENABLE_CUDA AND (MHGP9_SANITIZE OR MHGP9_TSAN))
  message(FATAL_ERROR "MHGP9_ENABLE_CUDA est incompatible avec les sanitizers")
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
# v9 E4 : appartenance exacte par domaine dense (0..n-1 : p < n) ; mutant
# compile (l'id n admis) tue par le juge lineaire de la porte.
mhgp9_test_executable(mhgp9_tower_population_bank_dense_inclusive_mutant_gate tests/tower/population_bank_gate.cpp)
target_compile_definitions(mhgp9_tower_population_bank_dense_inclusive_mutant_gate PRIVATE
                           MHGP9_BANK_MUTANT_DENSE_INCLUSIVE=1)
mhgp9_gate(mhgp9_tower_population_bank_mutant_dense_inclusive 1 mhgp9_tower_population_bank_dense_inclusive_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=bank.dense_membership" LABELS "gate;tower;mutant" TIMEOUT 120)
# Tri parallele de la tour : permutation egale a std::sort pour tout nombre de
# fils, et son mutant compile (un seau laisse non trie) tue.
mhgp9_product_executable(mhgp9_tower_parallel_sort_gate tests/tower/parallel_sort_gate.cpp)
mhgp9_gate(mhgp9_tower_parallel_sort 0 mhgp9_tower_parallel_sort_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 300)
mhgp9_product_executable(mhgp9_tower_parallel_sort_mutant_gate tests/tower/parallel_sort_gate.cpp)
target_compile_definitions(mhgp9_tower_parallel_sort_mutant_gate PRIVATE MHGP9_PARALLEL_SORT_MUTANT_UNSORTED_BUCKET=1)
mhgp9_gate(mhgp9_tower_parallel_sort_mutant_unsorted_bucket 1 mhgp9_tower_parallel_sort_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=parallel_sort.permutation" LABELS "gate;tower;mutant" TIMEOUT 300)
# Pool persistant de la tour (v9 E2, parallel/pool.hpp) : --unit juge le pool
# seul (indices, jonction, ouvriers engages mesures, exceptions et leur ordre,
# imbrication sous chien de garde, fils tiers, environnement flottant, echec
# de lancement du pool et de la voie par appel) ; --priority juge qu'une
# entree hors domaine reste invalide quand le lancement du pool echoue ;
# --fixtures juge la tour pool actif et coupe a 1, 2, 3, 4 et 8 fils
# (condense, tower_work, compteurs deterministes, travaux du pool egaux a
# chaque W) et le refus de ressource ; la trame 08/000000 a K5 (label lidar)
# avec ses condenses epingles et exactement 58 travaux du pool. Le mutant
# d'execution parallel-admit-partial-launch (voie par appel) et neuf mutants
# compiles du pool sont tues.
mhgp9_test_executable(mhgp9_tower_task_pool_gate tests/tower/task_pool_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_tower_task_pool_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_compile_definitions(mhgp9_tower_task_pool_gate PRIVATE MHGP9_POOL_GATE_CHAIN=1)
mhgp9_gate(mhgp9_tower_task_pool_unit 0 mhgp9_tower_task_pool_gate
           ARGS --unit EXPECT_PREFIX "task_pool_gate unit" LABELS "gate;tower" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_task_pool_priority 0 mhgp9_tower_task_pool_gate
           ARGS --priority EXPECT_PREFIX "task_pool_gate priority" LABELS "gate;tower" TIMEOUT 120)
mhgp9_gate(mhgp9_tower_task_pool_fixtures 0 mhgp9_tower_task_pool_gate
           ARGS --fixtures EXPECT_PREFIX "task_pool_gate fixtures" LABELS "gate;tower;chain" TIMEOUT 1200)
mhgp9_gate(mhgp9_tower_task_pool_bad_argument 2 mhgp9_tower_task_pool_gate
           ARGS --unknown LABELS "gate;tower" TIMEOUT 60)
mhgp9_gate(mhgp9_tower_task_pool_mutant_legacy_admit_partial 1 mhgp9_tower_task_pool_gate
           ARGS --unit --inject=parallel-admit-partial-launch EXPECT_PREFIX "cause=legacy.partial_launch_admitted"
           LABELS "gate;tower;mutant" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_task_pool_lidar_k5 0 mhgp9_tower_task_pool_gate
           ARGS --frame=${CMAKE_CURRENT_SOURCE_DIR}/../morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
                --k=5 --tower=67450c64611075b1 --catalogue=5ad1fe09354411ba
           EXPECT_PREFIX "task_pool_gate frame towers=10" LABELS "lidar;tower" TIMEOUT 3600)
foreach(mutant IN ITEMS ADMIT_PARTIAL RETURN_BEFORE_JOIN NESTED_QUEUE STALE_FENV BYPASS JOIN_CLOSED DECLARED STALE_ERROR)
  string(TOLOWER ${mutant} lower)
  mhgp9_test_executable(mhgp9_tower_task_pool_${lower}_mutant_gate tests/tower/task_pool_gate.cpp)
  target_compile_definitions(mhgp9_tower_task_pool_${lower}_mutant_gate PRIVATE MHGP9_POOL_MUTANT_${mutant}=1)
endforeach()
foreach(pair IN ITEMS "admit_partial:pool.partial_launch_admitted" "return_before_join:pool.returned_before_join"
                      "nested_queue:pool.nested_deadlock" "stale_fenv:pool.stale_fenv" "bypass:pool.bypassed"
                      "join_closed:pool.joined_closed_job" "declared:pool.declared_workers"
                      "stale_error:pool.stale_error")
  string(REPLACE ":" ";" parts ${pair})
  list(GET parts 0 lower)
  list(GET parts 1 cause)
  mhgp9_gate(mhgp9_tower_task_pool_mutant_${lower} 1 mhgp9_tower_task_pool_${lower}_mutant_gate
             ARGS --unit EXPECT_PREFIX "cause=${cause}" LABELS "gate;tower;mutant" TIMEOUT 300)
endforeach()
# Mutant de la tour : le pool cree avant les controles seriels du domaine (une
# entree invalide deviendrait un refus de ressource).
mhgp9_test_executable(mhgp9_tower_task_pool_before_domain_mutant_gate tests/tower/task_pool_gate.cpp)
target_compile_definitions(mhgp9_tower_task_pool_before_domain_mutant_gate PRIVATE MHGP9_POOL_GATE_TOWER=1
                           MHGP9_POOL_MUTANT_BEFORE_DOMAIN=1)
mhgp9_gate(mhgp9_tower_task_pool_mutant_before_domain 1 mhgp9_tower_task_pool_before_domain_mutant_gate
           ARGS --priority EXPECT_PREFIX "cause=tower.domain_after_launch" LABELS "gate;tower;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_tower_full_certificate_rejects 0 mhgp9_tower_full_certificate_gate
           ARGS --rejects LABELS "gate;tower" TIMEOUT 60)
mhgp9_gate(mhgp9_tower_full_ball_static_cpu1 0 mhgp9_tower_full_ball_tower_gate
           ARGS --static-1 LABELS "gate;tower;oracle" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_full_ball_static_cpu4 0 mhgp9_tower_full_ball_tower_gate
           ARGS --static-4 LABELS "gate;tower;oracle" TIMEOUT 300)
# v9 E4 : queue de la tour en pipeline (voie statique recouverte, 2 et 4 fils),
# jugee par l'oracle, appariee a la voie temporelle et au temoin
# (assign_populations apres la jointure) : memes IDs de population, meme
# charge utile, meme travail ; planchers sur les references differees (coquilles
# etendues qui contribuent a deux ordres). Mutant compile : decalage compte
# sur les contributions au lieu de la premiere rencontre, tue par le carre.
foreach(threads IN ITEMS 2 4)
  mhgp9_gate(mhgp9_tower_full_ball_pipelined_cpu${threads} 0 mhgp9_tower_full_ball_tower_gate
             ARGS --pipelined-${threads} LABELS "gate;tower;oracle" TIMEOUT 300)
endforeach()
# Second mutant : references differees jamais renommees (banque hors bornes).
foreach(mutant IN ITEMS OFFSET_ON_CONTRIBUTIONS DEFERRED_UNNAMED)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_tower_${lower}_mutant_gate tests/tower/full_ball_tower_gate.cpp)
  target_compile_definitions(mhgp9_tower_${lower}_mutant_gate PRIVATE MHGP9_TOWER_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_tower_full_ball_mutant_offset_on_contributions 1 mhgp9_tower_offset_on_contributions_mutant_gate
           ARGS --pipelined-4 EXPECT_PREFIX "refusal=full_ball_population_offset" LABELS "gate;tower;mutant" TIMEOUT 300)
mhgp9_gate(mhgp9_tower_full_ball_mutant_deferred_unnamed 1 mhgp9_tower_deferred_unnamed_mutant_gate
           ARGS --pipelined-4 EXPECT_PREFIX "refusal=full_ball_structural_certificate" LABELS "gate;tower;mutant" TIMEOUT 300)

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
# mhgp9_gpu (defini plus bas : lanceur CUDA ou stub) porte l'appel par lots
# du levier q34_gpu_filter ; sans MHGP9_ENABLE_CUDA la chaine le refuse.
target_link_libraries(mhgp9_chain PUBLIC mhgp9_gen mhgp9_gpu Threads::Threads)

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
# Priorite des echecs de la tour (plus petit K, puis lots, populations et
# images de K) et travail paye compte apres echec : points de panne
# MHGP9_TESTING, chaine recompilee dans la cible de test (le generateur ne
# voit pas la tour) ; voies classique, recouverte temoin et recouverte en
# pipeline (v9 E4 : 3 x 11 scenarios x 3 voies de fils, delai 600 s).
mhgp9_test_executable(mhgp9_chain_order_failure_priority_gate
                      tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_order_failure_priority_gate PRIVATE mhgp9_gen mhgp9_gpu)
mhgp9_gate(mhgp9_chain_order_failure_priority 0 mhgp9_chain_order_failure_priority_gate
           ARGS --selftest LABELS "gate;chain;tower" TIMEOUT 600)
foreach(mutant IN ITEMS PHASE_PRIORITY DROP_FAILED_STATS)
  string(TOLOWER ${mutant} lower)
  mhgp9_test_executable(mhgp9_chain_order_failure_${lower}_mutant_gate
                        tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(mhgp9_chain_order_failure_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_chain_order_failure_${lower}_mutant_gate
                             PRIVATE MHGP9_FULL_ORDERS_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_chain_order_failure_mutant_phase_priority 1 mhgp9_chain_order_failure_phase_priority_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=priority" LABELS "gate;chain;tower;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_chain_order_failure_mutant_drop_failed_stats 1 mhgp9_chain_order_failure_drop_failed_stats_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=ledger.paid_work_lost" LABELS "gate;chain;tower;mutant" TIMEOUT 600)
# Correctif v9 E4 : une exception autre qu'une panne (allocation refusee en
# phase 0, coureur non lance) deroule la portee de la voie recouverte pendant
# que les coureurs tournent ; refus de ressource propre sur les trois voies,
# travail des coureurs compte apres la jointure (planchers). Le mutant
# TIMERS_AFTER_JOIN (chronos et `publish` declares apres la jointure, donc
# liberes avant elle) n'ecrit qu'en memoire liberee : il n'est tue que sous
# ASan (-DMHGP9_SANITIZE=ON), rapport sur stdout. Duree locale : 25 s en
# Release, 300 s sous ASan.
mhgp9_gate(mhgp9_chain_order_failure_unwind 0 mhgp9_chain_order_failure_priority_gate
           ARGS --unwind EXPECT_PREFIX "order_failure_unwind_gate checks=64" LABELS "gate;chain;tower" TIMEOUT 900)
# v28 (C, constats 2 et 3) : le dimensionnement des lignes rate sur la queue
# en pipeline ; le mutant le rapporte comme une erreur de phase A (avant la
# panne des lots K3 que le temoin rapporte) et meurt sur le cas croise.
mhgp9_test_executable(mhgp9_chain_order_failure_rows_error_first_mutant_gate
                      tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_order_failure_rows_error_first_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_compile_definitions(mhgp9_chain_order_failure_rows_error_first_mutant_gate
                           PRIVATE MHGP9_FULL_ORDERS_MUTANT_ROWS_ERROR_FIRST=1)
mhgp9_gate(mhgp9_chain_order_failure_mutant_rows_error_first 1 mhgp9_chain_order_failure_rows_error_first_mutant_gate
           ARGS --unwind EXPECT_PREFIX "cause=unwind.rows" LABELS "gate;chain;tower;mutant" TIMEOUT 900)
# Revue avant R22 : une erreur hors panne des images d'un pas que le temoin
# n'execute pas (sous une panne plus basse) est ecartee ; le mutant la
# rapporte et meurt sur le cas « lots K2 + images K4 ».
mhgp9_test_executable(mhgp9_chain_order_failure_image_errors_unbounded_mutant_gate
                      tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_order_failure_image_errors_unbounded_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_compile_definitions(mhgp9_chain_order_failure_image_errors_unbounded_mutant_gate
                           PRIVATE MHGP9_FULL_ORDERS_MUTANT_IMAGE_ERRORS_UNBOUNDED=1)
mhgp9_gate(mhgp9_chain_order_failure_mutant_image_errors_unbounded 1
           mhgp9_chain_order_failure_image_errors_unbounded_mutant_gate
           ARGS --unwind EXPECT_PREFIX "cause=unwind.images" LABELS "gate;chain;tower;mutant" TIMEOUT 900)
if(MHGP9_SANITIZE)
  mhgp9_test_executable(mhgp9_chain_order_failure_timers_after_join_mutant_gate
                        tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(mhgp9_chain_order_failure_timers_after_join_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_chain_order_failure_timers_after_join_mutant_gate
                             PRIVATE MHGP9_FULL_ORDERS_MUTANT_TIMERS_AFTER_JOIN=1)
  mhgp9_gate(mhgp9_chain_order_failure_mutant_timers_after_join 1
             mhgp9_chain_order_failure_timers_after_join_mutant_gate
             ARGS --unwind EXPECT_PREFIX "SUMMARY: AddressSanitizer: heap-use-after-free"
             LABELS "gate;chain;tower;mutant" TIMEOUT 1200)
  set_tests_properties(mhgp9_chain_order_failure_mutant_timers_after_join PROPERTIES
                       ENVIRONMENT "ASAN_OPTIONS=log_path=stdout")
endif()
# Invariant d'Euler du catalogue (note C, preuve par le nerf de B) : sommes
# egales a 1 sur K <= min(Kmax-2, n) pour des fixtures degenerees (carre plan,
# deux sites, solides cospheriques) et des nuages entiers aleatoires ; statut
# not_checkable sous Kmax 3. Deux mutants de la chaine recompilee (coquille
# etendue traitee comme reguliere ; terme des sites oublie) : refus attendu.
mhgp9_product_executable(mhgp9_chain_euler_gate tests/chain/chain_euler_gate.cpp)
target_link_libraries(mhgp9_chain_euler_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_euler 0 mhgp9_chain_euler_gate ARGS --selftest LABELS "gate;chain;oracle" TIMEOUT 300)
mhgp9_gate(mhgp9_chain_euler_bad_argument 2 mhgp9_chain_euler_gate ARGS --unknown LABELS "gate;chain" TIMEOUT 60)
# S2 : chemin q3/q4 par lots (filtre CPU de reference) contre chemin moteur :
# memes candidats, meme tour FULL ; levier GPU refuse sans appareil ; mutants
# causaux du filtre par lots tues.
mhgp9_product_executable(mhgp9_chain_batch_filter_gate tests/chain/chain_batch_filter_gate.cpp)
target_link_libraries(mhgp9_chain_batch_filter_gate PRIVATE mhgp9_chain)
target_include_directories(mhgp9_chain_batch_filter_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_chain_batch_filter 0 mhgp9_chain_batch_filter_gate ARGS --n=1000
           EXPECT_PREFIX "chain_batch_filter_gate n=1000" LABELS "gate;chain;oracle" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_batch_filter_bad_argument 2 mhgp9_chain_batch_filter_gate ARGS --n=abc
           LABELS "gate;chain" TIMEOUT 60)
# Condense canonique du catalogue (auditeur C, 23 septembre 2026) : fixture
# gravee de coquilles etendues (arites 2, 3 et 4), recalcul local, egalite
# moteur 1/4 fils, lots CPU et certificats S3 CPU, option coupee neutre pour
# la tour, sensibilite a six alterations, et des boules redondantes de la zone
# aveugle (q2 et q3) que le condense FULL ne voit pas (plancher par arite).
# Deux mutants de la chaine recompilee (derniere boule omise ; coquille etendue
# tronquee a l'arite) refuses.
mhgp9_product_executable(mhgp9_catalogue_digest_gate tests/chain/catalogue_digest_gate.cpp)
target_link_libraries(mhgp9_catalogue_digest_gate PRIVATE mhgp9_chain)
target_include_directories(mhgp9_catalogue_digest_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_catalogue_digest 0 mhgp9_catalogue_digest_gate ARGS --n=1500
           EXPECT_PREFIX "catalogue_digest_gate n=1500" LABELS "gate;chain;oracle" TIMEOUT 900)
mhgp9_gate(mhgp9_catalogue_digest_bad_argument 2 mhgp9_catalogue_digest_gate ARGS --n=abc
           LABELS "gate;chain" TIMEOUT 60)
foreach(mutant IN ITEMS SKIP_LAST SHELL_ARITY_ONLY)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_catalogue_digest_${lower}_mutant_gate tests/chain/catalogue_digest_gate.cpp
                           src/chain/tower_chain.cpp)
  target_include_directories(mhgp9_catalogue_digest_${lower}_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gen"
                             "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_link_libraries(mhgp9_catalogue_digest_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_catalogue_digest_${lower}_mutant_gate PRIVATE MHGP9_CATALOGUE_DIGEST_MUTANT_${mutant}=1)
  mhgp9_gate(mhgp9_catalogue_digest_mutant_${lower} 1 mhgp9_catalogue_digest_${lower}_mutant_gate ARGS --n=1500
             EXPECT_LINE "cause=catalogue_digest.recompute cospheric K5" LABELS "gate;chain;mutant" TIMEOUT 900)
endforeach()
# Juges d'echantillon q2/q3 independants du generateur (auditeur C, R-20) :
# regression CPU ECHANTILLONNEE du catalogue, pas une qualification de la
# completude FULL (run_tower=false, ancres tirees). Pour des sites tires, toute
# boule diametrale (q2) ou circonscrite a un triangle aigu (q3) admissible,
# recensee en entiers par balayage, doit etre au catalogue avec son niveau exact
# (Boost cpp_int), sa cle, ses interieurs et sa coquille ; sens inverse (EXTRA) ;
# anti-vacuite par une cle retiree puis rejugee. Zone visee : l'angle mort
# d'Euler et de la tour (fusions seules a Kmax : q2 a p = Kmax-1, q3 a
# p = Kmax-2). La garde d'index porte sur l'index RECONSTRUIT par le juge.
# Controles globaux : cle en double ; boule etendue non appariee quand tous
# les sites sont tires (fixtures). La coquille q4 a 5 sites de la fixture
# cospherique n'est vue par aucun juge (porte du condense seulement).
# Porte rapide : fixtures gravees (egalite, strate CRL, coquilles etendues a
# comptes exacts), familles uniformes 2 000 en u16 et transportees dans le haut
# du domaine u18, mutants du catalogue lu (niveau, doublon, cle, coquille
# etendue tronquee, boule etendue dupliquee ou fantome) et de l'elagage tues
# avec leur marqueur, mutants d'index refuses en code 2.
# scale8000 : trois familles a K5 et K10, une famille u18 haute, un parcours
# brut q3 compare a l'elagage, et les coupes LiDAR 1 mm de 8 000 sites des
# trois trames sans sol, regenerees et epinglees (hors depot versionne).
foreach(judge IN ITEMS q2 q3)
  mhgp9_product_executable(mhgp9_${judge}_sample_judge tests/chain/${judge}_sample_judge.cpp)
  target_link_libraries(mhgp9_${judge}_sample_judge PRIVATE mhgp9_chain)
  target_include_directories(mhgp9_${judge}_sample_judge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  mhgp9_gate(mhgp9_${judge}_sample_judge_bad_argument 2 mhgp9_${judge}_sample_judge ARGS bogus
             LABELS "gate;chain" TIMEOUT 60)
  # Comptes exacts de la fixture (tous les sites tires) : q2 34, q3 3 (voir l'en-tete des juges).
  if(judge STREQUAL "q2")
    set(expect_extended 34)
  else()
    set(expect_extended 3)
  endif()
  mhgp9_gate(mhgp9_${judge}_judge_fixture_cospheric 0 mhgp9_${judge}_sample_judge
             ARGS fixture-cospheric --expect-extended=${expect_extended}
             EXPECT_PREFIX "fixture_cospheric n=417 kmax=5" LABELS "gate;chain;oracle" TIMEOUT 300)
  mhgp9_gate(mhgp9_${judge}_judge_mutant_shell_trim 1 mhgp9_${judge}_sample_judge ARGS fixture-cospheric --inject=shell-trim
             EXPECT_PREFIX "fixture_cospheric MISSING a=" LABELS "gate;chain;oracle;mutant" TIMEOUT 300)
  # Controles globaux : boule etendue dupliquee (cle en double), boule etendue fantome (non appariee alors que
  # tous les sites sont tires).
  mhgp9_gate(mhgp9_${judge}_judge_mutant_ext_dup 1 mhgp9_${judge}_sample_judge ARGS fixture-cospheric --inject=ext-dup
             EXPECT_PREFIX "fixture_cospheric DUPLICATE_KEY" LABELS "gate;chain;oracle;mutant" TIMEOUT 300)
  mhgp9_gate(mhgp9_${judge}_judge_mutant_ext_phantom 1 mhgp9_${judge}_sample_judge ARGS fixture-cospheric --inject=ext-phantom
             EXPECT_PREFIX "fixture_cospheric EXTRA_EXTENDED" LABELS "gate;chain;oracle;mutant" TIMEOUT 300)
endforeach()
mhgp9_gate(mhgp9_q3_judge_fixture_eq 0 mhgp9_q3_sample_judge ARGS fixture-eq --compare
           EXPECT_PREFIX "fixture_eq n=34 kmax=5" LABELS "gate;chain;oracle" TIMEOUT 120)
mhgp9_gate(mhgp9_q3_judge_fixture_crl 0 mhgp9_q3_sample_judge ARGS fixture-crl --compare --sites=0 --min-crl=1
           EXPECT_PREFIX "fixture_crl n=33 kmax=5" LABELS "gate;chain;oracle" TIMEOUT 120)
foreach(mode IN ITEMS family family-u18)
  if(mode STREQUAL "family")
    set(tag "")
  else()
    set(tag "_u18")
  endif()
  mhgp9_gate(mhgp9_q3_judge_uniform_2000${tag}_k5 0 mhgp9_q3_sample_judge ARGS ${mode} uniform 2000 5 20 4 --min-top=100 --min-crl=1
             EXPECT_PREFIX "uniform_2000${tag} n=2000 kmax=5" LABELS "gate;chain;oracle" TIMEOUT 300)
  mhgp9_gate(mhgp9_q2_judge_uniform_2000${tag}_k5 0 mhgp9_q2_sample_judge ARGS ${mode} uniform 2000 5 50 4 --min-top=100
             EXPECT_PREFIX "uniform_2000${tag} n=2000 kmax=5" LABELS "gate;chain;oracle" TIMEOUT 300)
endforeach()
# Mutants du catalogue lu par le juge (niveaux faux, coquille a doublon, cle
# seule faussee) : omission declaree (code 1, marqueur MISSING).
foreach(inj IN ITEMS level shell-dup key)
  string(REPLACE "-" "_" tag ${inj})
  mhgp9_gate(mhgp9_q3_judge_mutant_${tag} 1 mhgp9_q3_sample_judge ARGS fixture-eq --inject=${inj}
             EXPECT_PREFIX "fixture_eq MISSING a=" LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
  mhgp9_gate(mhgp9_q2_judge_mutant_${tag} 1 mhgp9_q2_sample_judge ARGS family uniform 2000 5 50 4 --inject=${inj}
             EXPECT_PREFIX "uniform_2000 MISSING a=" LABELS "gate;chain;oracle;mutant" TIMEOUT 300)
endforeach()
# Mutants de l'elagage du juge q3 : sur-elagage sur la fixture d'egalite ;
# arete longue et strate CRL retirees sur la fixture CRL (desaccord CRL).
mhgp9_gate(mhgp9_q3_judge_mutant_overprune 1 mhgp9_q3_sample_judge ARGS fixture-eq --compare --inject=overprune
           EXPECT_PREFIX "fixture_eq PRUNE_DISAGREES a=" LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
foreach(inj IN ITEMS drop-long drop-crl)
  string(REPLACE "-" "_" tag ${inj})
  mhgp9_gate(mhgp9_q3_judge_mutant_${tag} 1 mhgp9_q3_sample_judge
             ARGS fixture-crl --compare --sites=0 --min-crl=1 --inject=${inj}
             EXPECT_PREFIX "fixture_crl PRUNE_DISAGREES_CRL" LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
endforeach()
# Garde d'index du juge : refus en code 2 avant tout tirage.
foreach(pair IN ITEMS out-of-range:INDEX_ID_OUT_OF_RANGE duplicate:INDEX_ID_DUPLICATE missing:INDEX_ID_MISSING)
  string(REPLACE ":" ";" parts ${pair})
  list(GET parts 0 inj)
  list(GET parts 1 marker)
  string(REPLACE "-" "_" tag ${inj})
  mhgp9_gate(mhgp9_q3_judge_index_${tag} 2 mhgp9_q3_sample_judge ARGS fixture-eq --compare --inject=index-${inj}
             EXPECT_PREFIX "fixture_eq ${marker}" LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
  mhgp9_gate(mhgp9_q2_judge_index_${tag} 2 mhgp9_q2_sample_judge ARGS family uniform 2000 5 50 4 --inject=index-${inj}
             EXPECT_PREFIX "uniform_2000 ${marker}" LABELS "gate;chain;oracle;mutant" TIMEOUT 300)
endforeach()
# scale8000 : trois familles, K5 et K10, planchers de cles de tete et de strate CRL.
foreach(family IN ITEMS uniform terrain clusters)
  foreach(k IN ITEMS 5 10)
    mhgp9_gate(mhgp9_q2_judge_${family}_8000_k${k} 0 mhgp9_q2_sample_judge ARGS family ${family} 8000 ${k} 50 4 --min-top=100
               EXPECT_PREFIX "${family}_8000 n=8000 kmax=${k}" LABELS "scale8000;chain;oracle" TIMEOUT 1200)
    mhgp9_gate(mhgp9_q3_judge_${family}_8000_k${k} 0 mhgp9_q3_sample_judge
               ARGS family ${family} 8000 ${k} 20 4 --min-top=100 --min-crl=1
               EXPECT_PREFIX "${family}_8000 n=8000 kmax=${k}" LABELS "scale8000;chain;oracle" TIMEOUT 1800)
  endforeach()
endforeach()
mhgp9_gate(mhgp9_q3_judge_terrain_8000_k10_compare 0 mhgp9_q3_sample_judge
           ARGS family terrain 8000 10 10 4 --compare --min-top=100 --min-crl=100
           EXPECT_PREFIX "terrain_8000 n=8000 kmax=10" LABELS "scale8000;chain;oracle" TIMEOUT 1800)
mhgp9_gate(mhgp9_q2_judge_terrain_8000_u18_k10 0 mhgp9_q2_sample_judge ARGS family-u18 terrain 8000 10 50 4 --min-top=100
           EXPECT_PREFIX "terrain_8000_u18 n=8000 kmax=10" LABELS "scale8000;chain;oracle" TIMEOUT 1200)
mhgp9_gate(mhgp9_q3_judge_terrain_8000_u18_k10 0 mhgp9_q3_sample_judge
           ARGS family-u18 terrain 8000 10 20 4 --min-top=100 --min-crl=1
           EXPECT_PREFIX "terrain_8000_u18 n=8000 kmax=10" LABELS "scale8000;chain;oracle" TIMEOUT 1800)
# Coupes LiDAR 1 mm (8 000 sites des trames sans sol 08/000000, 000100 et
# 000200 du recu v8), regenerees par bench/run_lidar_scaling.py et epinglees.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  set(MHGP9_JUDGE_CUTS ${CMAKE_CURRENT_BINARY_DIR}/lidar_judge_cuts)
  add_test(NAME mhgp9_lidar_judge_cuts
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=-B ${CMAKE_CURRENT_SOURCE_DIR}/tests/chain/lidar_judge_cuts.py ${MHGP9_JUDGE_CUTS}"
                   "-DEXPECT_PREFIX=lidar_judge_cuts ok" -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp9_lidar_judge_cuts PROPERTIES LABELS "scale8000;chain;oracle" TIMEOUT 600
                       FIXTURES_SETUP mhgp9_lidar_judge_cuts)
  foreach(scene IN ITEMS s00 s01 s02)
    foreach(k IN ITEMS 5 10)
      mhgp9_gate(mhgp9_q2_judge_lidar_${scene}_8000_k${k} 0 mhgp9_q2_sample_judge
                 ARGS file ${MHGP9_JUDGE_CUTS}/lidar_${scene}_8000.u32le ${k} 50 4 --min-top=20
                 EXPECT_PREFIX "lidar_${scene}_8000.u32le n=8000 kmax=${k}" LABELS "scale8000;chain;oracle" TIMEOUT 1200)
      mhgp9_gate(mhgp9_q3_judge_lidar_${scene}_8000_k${k} 0 mhgp9_q3_sample_judge
                 ARGS file ${MHGP9_JUDGE_CUTS}/lidar_${scene}_8000.u32le ${k} 20 4 --min-top=20
                 EXPECT_PREFIX "lidar_${scene}_8000.u32le n=8000 kmax=${k}" LABELS "scale8000;chain;oracle" TIMEOUT 1800)
      set_tests_properties(mhgp9_q2_judge_lidar_${scene}_8000_k${k} mhgp9_q3_judge_lidar_${scene}_8000_k${k}
                           PROPERTIES FIXTURES_REQUIRED mhgp9_lidar_judge_cuts)
    endforeach()
  endforeach()
endif()
mhgp9_product_executable(mhgp9_chain_batch_certificates_gate tests/chain/chain_batch_certificates_gate.cpp)
target_link_libraries(mhgp9_chain_batch_certificates_gate PRIVATE mhgp9_chain)
target_include_directories(mhgp9_chain_batch_certificates_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_chain_batch_certificates 0 mhgp9_chain_batch_certificates_gate ARGS --n=1000
           EXPECT_PREFIX "chain_batch_certificates_gate n=1000" LABELS "gate;chain;oracle" TIMEOUT 1200)
mhgp9_gate(mhgp9_chain_batch_certificates_bad_argument 2 mhgp9_chain_batch_certificates_gate ARGS --n=abc
           LABELS "gate;chain" TIMEOUT 60)
# S4a : voie q3 par lots (levier q34_batch_q3) dans la chaine, memes condenses.
mhgp9_product_executable(mhgp9_chain_batch_q3_gate tests/chain/chain_batch_q3_gate.cpp)
target_link_libraries(mhgp9_chain_batch_q3_gate PRIVATE mhgp9_chain)
target_include_directories(mhgp9_chain_batch_q3_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_chain_batch_q3 0 mhgp9_chain_batch_q3_gate ARGS --n=1000
           EXPECT_PREFIX "chain_batch_q3_gate n=1000" LABELS "gate;chain;oracle" TIMEOUT 1200)
mhgp9_gate(mhgp9_chain_batch_q3_bad_argument 2 mhgp9_chain_batch_q3_gate ARGS --n=abc
           LABELS "gate;chain" TIMEOUT 60)
# v28 : recensement des cles q2 cote q2 (levier q2_early_census, pendant les
# appels q34) : memes octets que le recensement apres la fusion (statut,
# raison, condenses, catalogue boule par boule, statistiques et Euler,
# tower_work, ordres) sur cinq familles (dont coquilles q2 etendues et refus
# de coquille cote q2), K1..K10 et W 1, 3, 8 ; priorite des pannes par
# points de panne de la cible de test (plus petite cle en echec, puis q2,
# q34 et fusion avant le census) ; voie de repli du cote q2 (panne hors du
# census cle par cle, annulation par un echec de q34) ; cinq mutants compiles
# tues ; trame 08/000000 a K5 (label lidar) aux condenses epingles sur les
# deux bras.
foreach(variant IN ITEMS "" DROP_STATS EARLY_ERROR_FIRST ERROR_AT_JOIN IGNORE_FAILURE NO_CANCEL)
  if(variant STREQUAL "")
    set(target mhgp9_chain_q2_early_census_gate)
  else()
    string(TOLOWER ${variant} lower)
    set(target mhgp9_chain_q2_early_census_${lower}_mutant_gate)
  endif()
  mhgp9_test_executable(${target} tests/chain/chain_q2_early_census_gate.cpp src/chain/tower_chain.cpp)
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}/src/gen")
  target_link_libraries(${target} PRIVATE mhgp9_gen mhgp9_gpu)
  if(NOT variant STREQUAL "")
    target_compile_definitions(${target} PRIVATE MHGP9_Q2_EARLY_CENSUS_MUTANT_${variant}=1)
  endif()
endforeach()
mhgp9_gate(mhgp9_chain_q2_early_census 0 mhgp9_chain_q2_early_census_gate ARGS --selftest --n=1000
           EXPECT_PREFIX "chain_q2_early_census_gate selftest n=1000" LABELS "gate;chain;oracle" TIMEOUT 3600)
mhgp9_gate(mhgp9_chain_q2_early_census_priority 0 mhgp9_chain_q2_early_census_gate ARGS --priority
           EXPECT_PREFIX "chain_q2_early_census_gate priority n=1000" LABELS "gate;chain" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_fallback 0 mhgp9_chain_q2_early_census_gate ARGS --fallback
           EXPECT_PREFIX "chain_q2_early_census_gate fallback n=1000" LABELS "gate;chain" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_bad_argument 2 mhgp9_chain_q2_early_census_gate ARGS --n=abc
           LABELS "gate;chain" TIMEOUT 60)
mhgp9_gate(mhgp9_chain_q2_early_census_mutant_ignore_failure 1 mhgp9_chain_q2_early_census_ignore_failure_mutant_gate
           ARGS --fallback EXPECT_PREFIX "cause=fallback.early_keys uniform/K2/W1/after_census"
           LABELS "gate;chain;mutant" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_mutant_no_cancel 1 mhgp9_chain_q2_early_census_no_cancel_mutant_gate
           ARGS --fallback EXPECT_PREFIX "cause=fallback.cancel cancel/W1/on" LABELS "gate;chain;mutant" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_mutant_drop_stats 1 mhgp9_chain_q2_early_census_drop_stats_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=object." LABELS "gate;chain;mutant" TIMEOUT 3600)
mhgp9_gate(mhgp9_chain_q2_early_census_mutant_early_error_first 1
           mhgp9_chain_q2_early_census_early_error_first_mutant_gate
           ARGS --priority EXPECT_PREFIX "cause=priority late_before_q2/W1/on" LABELS "gate;chain;mutant" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_mutant_error_at_join 1 mhgp9_chain_q2_early_census_error_at_join_mutant_gate
           ARGS --priority EXPECT_PREFIX "cause=priority merge_before_census/W1/on" LABELS "gate;chain;mutant"
           TIMEOUT 900)
mhgp9_gate(mhgp9_chain_q2_early_census_lidar_k5 0 mhgp9_chain_q2_early_census_gate
           ARGS --frame=${CMAKE_CURRENT_SOURCE_DIR}/../morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
                --k=5 --tower=67450c64611075b1 --catalogue=5ad1fe09354411ba --pairs=1
           EXPECT_PREFIX "chain_q2_early_census_gate frame n=39885 K=5 pairs=1 arm=-1" LABELS "lidar;chain" TIMEOUT 3600)
# v28 (revue) : le chemin de la chaine sous q34_lanes_pinned sur le CPU (bail
# du bassin resident du jumeau hote) contre le temoin pageable ; le mutant
# PINNED_READS_VECTOR (conversion lisant le vecteur vide au lieu du bail) est
# tue des la premiere trame qui a des enregistrements.
mhgp9_gate(mhgp9_chain_batch_q3_pinned 0 mhgp9_chain_batch_q3_gate ARGS --n=1000 --pinned
           EXPECT_PREFIX "chain_batch_q3_pinned n=1000 cases=32" LABELS "gate;chain;oracle" TIMEOUT 1200)
mhgp9_gate(mhgp9_chain_batch_q3_pinned_bad_argument 2 mhgp9_chain_batch_q3_gate ARGS --pinned --n=abc
           LABELS "gate;chain" TIMEOUT 60)
mhgp9_product_executable(mhgp9_chain_pinned_reads_vector_mutant_gate tests/chain/chain_batch_q3_gate.cpp
                         src/chain/tower_chain.cpp)
target_include_directories(mhgp9_chain_pinned_reads_vector_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gen"
                           "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(mhgp9_chain_pinned_reads_vector_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_compile_definitions(mhgp9_chain_pinned_reads_vector_mutant_gate PRIVATE MHGP9_CHAIN_MUTANT_PINNED_READS_VECTOR=1)
mhgp9_gate(mhgp9_chain_batch_q3_pinned_mutant_reads_vector 1 mhgp9_chain_pinned_reads_vector_mutant_gate
           ARGS --n=1000 --pinned EXPECT_PREFIX "cause=pinned.status uniform/K2/W1"
           LABELS "gate;chain;mutant" TIMEOUT 600)
foreach(mutant IN ITEMS REGULAR_SHELLS_ONLY NO_SITE_TERM)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_chain_euler_${lower}_mutant_gate tests/chain/chain_euler_gate.cpp src/chain/tower_chain.cpp)
  target_include_directories(mhgp9_chain_euler_${lower}_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gen")
  target_link_libraries(mhgp9_chain_euler_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_chain_euler_${lower}_mutant_gate PRIVATE MHGP9_EULER_MUTANT_${mutant}=1)
  mhgp9_gate(mhgp9_chain_euler_mutant_${lower} 1 mhgp9_chain_euler_${lower}_mutant_gate
             ARGS --selftest
             EXPECT_PREFIX "cause=euler.chain_refused square K3 s8 W1 reason=chain_catalogue_euler_violated"
             LABELS "gate;chain;mutant" TIMEOUT 300)
endforeach()
# Juge d'echantillon des cles JAMAIS emises (question de l'auditeur B) :
# supports candidats de 2 a 4 sites voisins, MEB et recensement exacts ; toute
# boule admissible doit etre au catalogue ; des omissions plantees doivent
# etre trouvees. n = 2 000 dans la porte rapide, 8 000 sous scale8000.
mhgp9_product_executable(mhgp9_chain_absent_keys_gate tests/chain/chain_absent_keys_gate.cpp)
target_link_libraries(mhgp9_chain_absent_keys_gate PRIVATE mhgp9_chain)
target_include_directories(mhgp9_chain_absent_keys_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_chain_absent_keys 0 mhgp9_chain_absent_keys_gate ARGS --selftest --n=2000
           EXPECT_PREFIX "chain_absent_keys_gate n=2000" LABELS "gate;chain;oracle" TIMEOUT 600)
mhgp9_gate(mhgp9_chain_absent_keys_scale8000 0 mhgp9_chain_absent_keys_gate ARGS --selftest --n=8000
           EXPECT_PREFIX "chain_absent_keys_gate n=8000" LABELS "scale8000;chain;oracle" TIMEOUT 1800)
mhgp9_gate(mhgp9_chain_absent_keys_bad_argument 2 mhgp9_chain_absent_keys_gate ARGS --n=abc LABELS "gate;chain" TIMEOUT 60)

# ---- Voie GPU (23 septembre 2026) : port portable (hote/device) du filtre
# temoin exact q3/q4, juge ici requete par requete contre le produit.
mhgp9_product_executable(mhgp9_gpu_witness_filter_port_gate tests/gpu/witness_filter_port_gate.cpp)
target_link_libraries(mhgp9_gpu_witness_filter_port_gate PRIVATE mhgp9_gen)
mhgp9_gate(mhgp9_gpu_witness_filter_port 0 mhgp9_gpu_witness_filter_port_gate ARGS --n=2000 --pair-stride=5
           EXPECT_PREFIX "witness_filter_port_gate n=2000" LABELS "gate;gpu;oracle" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_witness_filter_port_scale8000 0 mhgp9_gpu_witness_filter_port_gate ARGS --n=8000
           EXPECT_PREFIX "witness_filter_port_gate n=8000" LABELS "scale8000;gpu;oracle" TIMEOUT 1800)
mhgp9_gate(mhgp9_gpu_witness_filter_port_bad_argument 2 mhgp9_gpu_witness_filter_port_gate ARGS --n=abc
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_product_executable(mhgp9_gpu_witness_cache_port_gate tests/gpu/witness_cache_port_gate.cpp)
target_link_libraries(mhgp9_gpu_witness_cache_port_gate PRIVATE mhgp9_gen)
mhgp9_gate(mhgp9_gpu_witness_cache_port 0 mhgp9_gpu_witness_cache_port_gate
           EXPECT_PREFIX "witness_cache_port_gate queries=" LABELS "gate;gpu;oracle" TIMEOUT 180)
mhgp9_gate(mhgp9_gpu_witness_cache_port_bad_argument 2 mhgp9_gpu_witness_cache_port_gate
           ARGS --bad LABELS "gate;gpu" TIMEOUT 60)
mhgp9_product_executable(mhgp9_gpu_witness_cache_no_retest_mutant_gate tests/gpu/witness_cache_port_gate.cpp)
target_link_libraries(mhgp9_gpu_witness_cache_no_retest_mutant_gate PRIVATE mhgp9_gen)
target_compile_definitions(mhgp9_gpu_witness_cache_no_retest_mutant_gate PRIVATE MHGP9_WITNESS_CACHE_MUTANT_NO_RETEST=1)
mhgp9_gate(mhgp9_gpu_witness_cache_no_retest_mutant 1 mhgp9_gpu_witness_cache_no_retest_mutant_gate
           EXPECT_PREFIX "cause=witness_cache.endpoint_retest" LABELS "gate;gpu;mutant" TIMEOUT 60)
mhgp9_product_executable(mhgp9_gpu_certificate_port_gate tests/gpu/certificate_port_gate.cpp)
target_link_libraries(mhgp9_gpu_certificate_port_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_certificate_port_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_gpu_certificate_port 0 mhgp9_gpu_certificate_port_gate ARGS --n=2000
           EXPECT_PREFIX "gpu_certificate_port_gate n=2000" LABELS "gate;gpu;oracle" TIMEOUT 900)
mhgp9_gate(mhgp9_gpu_certificate_port_scale8000 0 mhgp9_gpu_certificate_port_gate ARGS --n=8000 --k=5
           EXPECT_PREFIX "gpu_certificate_port_gate n=8000" LABELS "scale8000;gpu;oracle" TIMEOUT 2400)
mhgp9_gate(mhgp9_gpu_certificate_port_bad_argument 2 mhgp9_gpu_certificate_port_gate ARGS --k=11
           LABELS "gate;gpu" TIMEOUT 60)
# Mode fichier (24 septembre 2026) : --workers, --min-edges et --device
# seulement avec --file (2) ; un seul K (2) ; sans CUDA, --device est un
# refus explicite (2), jamais un vert par vacuite. La trame epinglee
# 08/000000 (v8, sol retire) a K5 : chaque survivante, les deux parcours,
# les deux chargements et le prouveur produit (label lidar, hors campagne
# gate : environ 3 minutes sur 4 fils).
mhgp9_gate(mhgp9_gpu_certificate_port_device_without_file 2 mhgp9_gpu_certificate_port_gate ARGS --device
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_certificate_port_file_two_k 2 mhgp9_gpu_certificate_port_gate
           ARGS --file=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpu/fixtures/bound_tetra.u32le --k=3,5
           LABELS "gate;gpu" TIMEOUT 60)
if(NOT MHGP9_ENABLE_CUDA)
  mhgp9_gate(mhgp9_gpu_certificate_port_device_absent 2 mhgp9_gpu_certificate_port_gate
             ARGS --file=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpu/fixtures/bound_tetra.u32le --k=3 --workers=1 --device
             EXPECT_PREFIX "certificate_device_compare absent" LABELS "gate;gpu" TIMEOUT 60)
endif()
mhgp9_gate(mhgp9_gpu_certificate_port_lidar_k5 0 mhgp9_gpu_certificate_port_gate
           ARGS --file=${CMAKE_CURRENT_SOURCE_DIR}/../morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
                --k=5 --workers=4 --min-edges=2000000
           EXPECT_PREFIX "certificate_port_file K=5 survivors=2043612 decided=2043612 deferred=0"
           LABELS "lidar;gpu;oracle" TIMEOUT 3600)
# Parcours et chargement par blocs du certificat (24 septembre 2026,
# build_cover_chunked, load_forms_chunked) : quatre mutants compiles du
# produit, chacun tue (code 1) par la porte de port : successeur hors bloc
# pris pour le debut du bloc suivant et fils gauche de la derniere voie
# saute (walk., parcours par blocs contre build_cover) ; fenetre de
# chargement qui recommence sa premiere plage (load., formes contre
# load_forms) ; norme partagee comparee au facteur q3 pour la voie q4
# (port., prouveur produit).
foreach(mutant IN ITEMS CHUNK_FAR_ESCAPE CHUNK_SPLIT_BOUNDARY LOAD_WINDOW_RESTART SHARED_NORM_FACTOR)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_gpu_certificate_${lower}_mutant_gate tests/gpu/certificate_port_gate.cpp)
  target_link_libraries(mhgp9_gpu_certificate_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_include_directories(mhgp9_gpu_certificate_${lower}_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_definitions(mhgp9_gpu_certificate_${lower}_mutant_gate PRIVATE MHGP9_CERTIFICATE_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_gpu_certificate_mutant_chunk_far_escape 1 mhgp9_gpu_certificate_chunk_far_escape_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=walk." LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_certificate_mutant_chunk_split_boundary 1 mhgp9_gpu_certificate_chunk_split_boundary_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=walk." LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_certificate_mutant_load_window_restart 1 mhgp9_gpu_certificate_load_window_restart_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=load." LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_certificate_mutant_shared_norm_factor 1 mhgp9_gpu_certificate_shared_norm_factor_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=port." LABELS "gate;gpu;mutant" TIMEOUT 600)
# S4a : port portable de la voie q3 sans atlas (gpu/lanes.hpp, emulation
# hote), juge arete par arete contre la voie q3 du moteur, executeur par lots,
# capacites reduites, mutants de sortie et garde d'entree.
mhgp9_product_executable(mhgp9_gpu_lanes_port_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_port_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_port_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_gpu_lanes_port 0 mhgp9_gpu_lanes_port_gate ARGS --n=2000
           EXPECT_PREFIX "lanes_port_gate n=2000" LABELS "gate;gpu;oracle" TIMEOUT 1800)
mhgp9_gate(mhgp9_gpu_lanes_port_scale8000 0 mhgp9_gpu_lanes_port_gate ARGS --n=8000 --k=5
           EXPECT_PREFIX "lanes_port_gate n=8000" LABELS "scale8000;gpu;oracle" TIMEOUT 2400)
mhgp9_gate(mhgp9_gpu_lanes_port_bad_argument 2 mhgp9_gpu_lanes_port_gate ARGS --k=11
           LABELS "gate;gpu" TIMEOUT 60)
# v28 : bassin de blocs residents des enregistrements des voies
# (gpu/record_pool.hpp, levier pinned_records) sur une memoire de test :
# croissance a la demande, exclusivite des baux, retour a la destruction,
# baux concurrents, surete aux exceptions, chemin du jumeau hote octet pour
# octet. Quatre mutants tues (code 1).
mhgp9_product_executable(mhgp9_gpu_record_pool_gate tests/gpu/record_pool_gate.cpp)
mhgp9_gate(mhgp9_gpu_record_pool 0 mhgp9_gpu_record_pool_gate ARGS --rounds=64
           EXPECT_PREFIX "record_pool_gate cases=31" LABELS "gate;gpu" TIMEOUT 120)
mhgp9_gate(mhgp9_gpu_record_pool_bad_argument 2 mhgp9_gpu_record_pool_gate ARGS --rounds=abc
           LABELS "gate;gpu" TIMEOUT 60)
foreach(mutant IN ITEMS RECORD_POOL_MUTANT_TAKE_LEASED RECORD_POOL_MUTANT_NO_RETURN
                        RECORD_POOL_MUTANT_KEEP_TAKEN_ON_THROW LANES_MUTANT_PINNED_SHORT_COPY)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_gpu_${lower}_gate tests/gpu/record_pool_gate.cpp)
  target_compile_definitions(mhgp9_gpu_${lower}_gate PRIVATE MHGP9_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_gpu_record_pool_mutant_take_leased 1 mhgp9_gpu_record_pool_mutant_take_leased_gate
           EXPECT_LINE "cause=exclusive.same_block" LABELS "gate;gpu;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_gpu_record_pool_mutant_no_return 1 mhgp9_gpu_record_pool_mutant_no_return_gate
           EXPECT_LINE "cause=grow.returned" LABELS "gate;gpu;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_gpu_record_pool_mutant_keep_taken_on_throw 1 mhgp9_gpu_record_pool_mutant_keep_taken_on_throw_gate
           EXPECT_LINE "cause=throw.growth" LABELS "gate;gpu;mutant" TIMEOUT 120)
mhgp9_gate(mhgp9_gpu_lanes_mutant_pinned_short_copy 1 mhgp9_gpu_lanes_mutant_pinned_short_copy_gate
           EXPECT_LINE "cause=twin.bytes n=1" LABELS "gate;gpu;mutant" TIMEOUT 120)
# Ablation hote du telechargement (mesure indicative, sans GPU) : pageable
# neuf contre bloc resident, paires entrelacees.
mhgp9_product_executable(mhgp9_lanes_download_ablation bench/lanes_download_ablation.cpp)
mhgp9_gate(mhgp9_lanes_download_ablation_bad_argument 2 mhgp9_lanes_download_ablation ARGS 0 3
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_lanes_download_ablation_small 0 mhgp9_lanes_download_ablation ARGS 1000 2
           EXPECT_PREFIX "lanes_download_ablation records=1000" LABELS "gate;gpu" TIMEOUT 60)
# T1 (etape 1 des voies) : un mutant qui arrete la passe d'une classe un
# site interieur trop tot rejette des classes de profondeur T-1 ; la porte
# de port (voies q4 contre le moteur) le tue a K3.
mhgp9_product_executable(mhgp9_gpu_lanes_port_early_exit_mutant_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_port_early_exit_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_port_early_exit_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(mhgp9_gpu_lanes_port_early_exit_mutant_gate PRIVATE MHGP9_Q4_LANES_MUTANT_EARLY_EXIT_BELOW_T=1)
mhgp9_gate(mhgp9_gpu_lanes_port_mutant_early_exit 1 mhgp9_gpu_lanes_port_early_exit_mutant_gate ARGS --n=2000 --k=3
           EXPECT_PREFIX "cause=" LABELS "gate;gpu;mutant" TIMEOUT 600)
# S4b taches (etape 2 des voies, gpu/lanes_tasks.hpp) : la porte de port
# compare octet pour octet les trois etapes P/T/C du jumeau au chemin a une
# tache par arete (B en {1, defaut, infini}, fils, fenetres, capacites
# reduites, refus d'arene, repliques gravees). Cinq mutants, chacun tue
# (code 1) par le controle qui le vise : derniere plage partielle oubliee
# (split.count), placement par ordre d'achevement emule par des taches
# placees entieres a rebours (tasks.bytes), segments q3 et q4 permutes
# (tasks.bytes), compteurs d'une arete en attente ajoutes (tasks.small_bytes),
# preseance attente/panne inversee dans le rejeu par balayage (replay.).
foreach(mutant IN ITEMS LAST_RANGE_FORGOTTEN COMPLETION_ORDER SEGMENTS_SWAPPED DEFERRED_COUNTERS FAULT_FIRST)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_gpu_lanes_tasks_${lower}_mutant_gate tests/gpu/lanes_port_gate.cpp)
  target_link_libraries(mhgp9_gpu_lanes_tasks_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_include_directories(mhgp9_gpu_lanes_tasks_${lower}_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_definitions(mhgp9_gpu_lanes_tasks_${lower}_mutant_gate PRIVATE MHGP9_LANES_TASKS_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_gpu_lanes_tasks_mutant_last_range_forgotten 1 mhgp9_gpu_lanes_tasks_last_range_forgotten_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=split.count" LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_lanes_tasks_mutant_completion_order 1 mhgp9_gpu_lanes_tasks_completion_order_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=tasks.bytes" LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_lanes_tasks_mutant_segments_swapped 1 mhgp9_gpu_lanes_tasks_segments_swapped_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=tasks.bytes" LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_lanes_tasks_mutant_deferred_counters 1 mhgp9_gpu_lanes_tasks_deferred_counters_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=tasks.small_bytes" LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_gpu_lanes_tasks_mutant_fault_first 1 mhgp9_gpu_lanes_tasks_fault_first_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=replay." LABELS "gate;gpu;mutant" TIMEOUT 600)
# L11 (etape 3 des voies, lemme V9-S4) : les sites du cover hors du disque
# des centres sont retires de l'ordre de balayage. Mutant « disque trop
# petit » (16|c-m|^2 <= D au lieu de 8) tue par la porte de port (code 1) ;
# build sans elagage (MHGP9_LANES_PRUNE=0) : meme objet, porte verte.
mhgp9_product_executable(mhgp9_gpu_lanes_prune_small_disk_mutant_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_prune_small_disk_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_prune_small_disk_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(mhgp9_gpu_lanes_prune_small_disk_mutant_gate PRIVATE MHGP9_LANES_MUTANT_PRUNE_SMALL_DISK=1)
mhgp9_gate(mhgp9_gpu_lanes_mutant_prune_small_disk 1 mhgp9_gpu_lanes_prune_small_disk_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=" LABELS "gate;gpu;mutant" TIMEOUT 600)
mhgp9_product_executable(mhgp9_gpu_lanes_port_unpruned_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_port_unpruned_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_port_unpruned_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(mhgp9_gpu_lanes_port_unpruned_gate PRIVATE MHGP9_LANES_PRUNE=0)
mhgp9_gate(mhgp9_gpu_lanes_port_unpruned 0 mhgp9_gpu_lanes_port_unpruned_gate ARGS --n=2000 --k=3,5
           EXPECT_PREFIX "lanes_port_gate n=2000" LABELS "gate;gpu" TIMEOUT 1200)
# L10 (etape 3 des voies) : ordre de balayage axial en 26 classes entieres.
# Tout ordre fixe donne le meme objet : le build aux anneaux
# (MHGP9_LANES_SCAN_AXIAL=0) reste vert. Un defaut de la seule classe ne
# change que l'ordre ; le mutant vise donc le tri par comptage elargi a cinq
# bits : la passe de comptage lit les classes 16 a 25 comme 0 a 9, les
# decalages ne sont plus ceux de la dispersion (trous, sites perdus), tue par
# la porte de port (code 1).
mhgp9_product_executable(mhgp9_gpu_lanes_port_rings_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_port_rings_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_port_rings_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(mhgp9_gpu_lanes_port_rings_gate PRIVATE MHGP9_LANES_SCAN_AXIAL=0)
mhgp9_gate(mhgp9_gpu_lanes_port_rings 0 mhgp9_gpu_lanes_port_rings_gate ARGS --n=2000 --k=3,5
           EXPECT_PREFIX "lanes_port_gate n=2000" LABELS "gate;gpu" TIMEOUT 1200)
mhgp9_product_executable(mhgp9_gpu_lanes_axial_four_bits_mutant_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_axial_four_bits_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_include_directories(mhgp9_gpu_lanes_axial_four_bits_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(mhgp9_gpu_lanes_axial_four_bits_mutant_gate PRIVATE MHGP9_LANES_MUTANT_AXIAL_FOUR_BITS=1)
mhgp9_gate(mhgp9_gpu_lanes_mutant_axial_four_bits 1 mhgp9_gpu_lanes_axial_four_bits_mutant_gate
           ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=" LABELS "gate;gpu;mutant" TIMEOUT 600)
# L15 (etape 3 des voies) : passe fusionnee q3 + q4 par graine dans une
# tache T, comparee octet pour octet (registre compris) au chemin a une tache
# par arete, qui garde les deux phases separees. Deux mutants tues (code 1) :
# recensement fusionne arrete a K-2 (edge.records ou tasks.bytes) ; passe de
# lentilles arretee avec le recensement (tasks.bytes).
foreach(mutant IN ITEMS FUSED_Q3_AT_K_MINUS_2 FUSED_JOINT_STOP)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_gpu_lanes_${lower}_mutant_gate tests/gpu/lanes_port_gate.cpp)
  target_link_libraries(mhgp9_gpu_lanes_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_include_directories(mhgp9_gpu_lanes_${lower}_mutant_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_compile_definitions(mhgp9_gpu_lanes_${lower}_mutant_gate PRIVATE MHGP9_LANES_MUTANT_${mutant}=1)
  mhgp9_gate(mhgp9_gpu_lanes_mutant_${lower} 1 mhgp9_gpu_lanes_${lower}_mutant_gate
             ARGS --n=2000 --k=3 EXPECT_PREFIX "cause=" LABELS "gate;gpu;mutant" TIMEOUT 600)
endforeach()
# Mode --compare (revue S4b) : planchers exiges et seulement avec --compare,
# --compare seulement sur un fichier, nuage refuse = 2 ; deux points (fixture
# de l'auditeur) = comparaison vide sous plancher (3) ; tetraedre a racine sur
# une borne interieure = toutes les aretes demandees comparees (0).
set(mhgp9_lanes_fixtures "${CMAKE_CURRENT_SOURCE_DIR}/tests/gpu/fixtures")
mhgp9_gate(mhgp9_gpu_lanes_port_floor_without_compare 2 mhgp9_gpu_lanes_port_gate ARGS --min-q4-seeds=1
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_bad_floor 2 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/two_points.u32le --k=3 --compare --min-q3-records=x
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_without_file 2 mhgp9_gpu_lanes_port_gate ARGS --compare --min-q3-records=1
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_without_floor 2 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/two_points.u32le --k=3 --compare LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_refused_cloud 2 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/duplicate_sites.u32le --k=3 --compare --min-q4-records=1
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_empty 3 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/two_points.u32le --k=3 --workers=1 --compare --min-q4-records=1
           EXPECT_PREFIX "lanes_compare K=3 survivors=1" LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_bound 0 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/bound_tetra.u32le --k=3 --workers=1 --compare --all-asked
                --min-q4-seeds=1 --min-q4-records=1
           EXPECT_PREFIX "lanes_compare K=3" LABELS "gate;gpu" TIMEOUT 60)
# L11 : fixture minimale de quatre points (recherche hors depot, variante
# qui n'elaguait que les sites non graines entre les deux disques) ; un site
# entre le disque 16|c-m|^2 <= D et le disque des centres decide d'une boule
# q3. Le produit l'egale au moteur (0), le mutant « disque trop petit » est
# tue (1, compare.q3).
mhgp9_gate(mhgp9_gpu_lanes_port_compare_prune_annulus 0 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/prune_annulus.u32le --k=3 --workers=1 --compare --all-asked
                --min-q3-records=1
           EXPECT_PREFIX "lanes_compare K=3" LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_mutant_prune_small_disk_fixture 1 mhgp9_gpu_lanes_prune_small_disk_mutant_gate
           ARGS --file=${mhgp9_lanes_fixtures}/prune_annulus.u32le --k=3 --workers=1 --compare --all-asked
                --min-q3-records=1
           EXPECT_PREFIX "cause=compare.q3" LABELS "gate;gpu;mutant" TIMEOUT 60)
# --device (S4b taches, pour une session G4) : l'appel de l'appareil compare
# octet pour octet au chemin a une tache ; --device seulement avec --compare
# (2) ; sans CUDA, refus explicite (2), jamais un vert par vacuite.
mhgp9_gate(mhgp9_gpu_lanes_port_device_without_compare 2 mhgp9_gpu_lanes_port_gate ARGS --device
           LABELS "gate;gpu" TIMEOUT 60)
# --unfused (L15) : les taches sans passe fusionnee (mesure appariee sur G4) ;
# seulement avec --compare (2) ; meme sortie que le chemin a une tache (0).
mhgp9_gate(mhgp9_gpu_lanes_port_unfused_without_compare 2 mhgp9_gpu_lanes_port_gate ARGS --unfused
           LABELS "gate;gpu" TIMEOUT 60)
mhgp9_gate(mhgp9_gpu_lanes_port_compare_unfused 0 mhgp9_gpu_lanes_port_gate
           ARGS --file=${mhgp9_lanes_fixtures}/bound_tetra.u32le --k=3 --workers=1 --compare --all-asked
                --min-q4-seeds=1 --min-q4-records=1 --unfused
           EXPECT_PREFIX "lanes_compare K=3" LABELS "gate;gpu" TIMEOUT 60)
if(NOT MHGP9_ENABLE_CUDA)
  mhgp9_gate(mhgp9_gpu_lanes_port_device_absent 2 mhgp9_gpu_lanes_port_gate
             ARGS --file=${mhgp9_lanes_fixtures}/bound_tetra.u32le --k=3 --workers=1 --compare --all-asked
                  --min-q4-seeds=1 --device
             EXPECT_PREFIX "lanes_device_compare absent" LABELS "gate;gpu" TIMEOUT 60)
endif()

# Sonde S1 : meme population que la chaine (front, rectangles, paires), CPU
# puis GPU, masques compares requete par requete. Sans MHGP9_ENABLE_CUDA le
# passage GPU est un stub (`--cpu-only` seul utile).
if(MHGP9_ENABLE_CUDA)
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
  add_library(mhgp9_gpu STATIC src/gpu/filter_runner.cu)
  set_target_properties(mhgp9_gpu PROPERTIES CUDA_ARCHITECTURES 120 CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON)
  target_compile_options(mhgp9_gpu PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall,-Wextra>")
  target_link_libraries(mhgp9_gpu PUBLIC CUDA::cudart_static)
else()
  add_library(mhgp9_gpu STATIC src/gpu/filter_runner_stub.cpp)
endif()
mhgp9_product_executable(mhgp9_gpu_filter_probe bench/gpu_filter_probe.cpp)
target_link_libraries(mhgp9_gpu_filter_probe PRIVATE mhgp9_gen mhgp9_gpu)
mhgp9_gate(mhgp9_gpu_filter_probe_bad_argument 2 mhgp9_gpu_filter_probe ARGS missing.u32le 5 1
           LABELS "gate;gpu" TIMEOUT 60)
# Empty cloud: a documented input refusal (code 2), never an abort.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gpu_probe_empty.u32le "")
mhgp9_gate(mhgp9_gpu_filter_probe_empty_input 2 mhgp9_gpu_filter_probe
           ARGS ${CMAKE_CURRENT_BINARY_DIR}/gpu_probe_empty.u32le 5 1 --cpu-only LABELS "gate;gpu" TIMEOUT 60)
# v9 E4 : queue de la tour en pipeline sur la chaine (1 500 sites et six
# carres plantes, K5) : 1, 2, 3, 4 et 8 fils et le temoin a 4 et 8 fils,
# meme condense, memes IDs de population, meme travail publie ; planchers sur
# les references differees. Le mutant du decalage compte sur les
# contributions, compile dans la chaine, est refuse a 2 fils.
mhgp9_product_executable(mhgp9_chain_tower_tail_gate tests/chain/tower_tail_gate.cpp)
target_link_libraries(mhgp9_chain_tower_tail_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_tower_tail 0 mhgp9_chain_tower_tail_gate
           ARGS --selftest EXPECT_PREFIX "tower_tail_gate digest=" LABELS "gate;chain;tower" TIMEOUT 600)
mhgp9_gate(mhgp9_chain_tower_tail_bad_argument 2 mhgp9_chain_tower_tail_gate
           ARGS --unknown LABELS "gate;chain" TIMEOUT 60)
mhgp9_product_executable(mhgp9_chain_tower_tail_offset_mutant_gate tests/chain/tower_tail_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_tower_tail_offset_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
target_compile_definitions(mhgp9_chain_tower_tail_offset_mutant_gate PRIVATE MHGP9_TOWER_MUTANT_OFFSET_ON_CONTRIBUTIONS=1)
mhgp9_gate(mhgp9_chain_tower_tail_mutant_offset_on_contributions 1 mhgp9_chain_tower_tail_offset_mutant_gate
           ARGS --selftest EXPECT_LINE "cause=chain.status static=2 pipelined=1 reason=tower: full_ball_population_offset"
           LABELS "gate;chain;tower;mutant" TIMEOUT 600)
mhgp9_product_executable(mhgp9_chain_static_paths_gate tests/chain/chain_static_paths_gate.cpp)
target_link_libraries(mhgp9_chain_static_paths_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_static_paths 0 mhgp9_chain_static_paths_gate
           ARGS --selftest LABELS "gate;chain" TIMEOUT 300)
# Images par rangs de plateau (E1) : la voie statique compare des rangs u32,
# la voie temporelle garde les niveaux exacts (temoin). Deux mutants de la
# chaine recompilee : coupe fermee lue ouverte, niveau zero au rang du plus
# bas niveau du catalogue ; E3 : une facette oubliee par le decompte de la
# phase A maigre (cibles statiques non consommees). La voie statique s'ecarte
# du temoin.
foreach(mutant IN ITEMS RUNS_OPEN_CUT RUNS_ZERO_COLLIDES COUNT_SKIPS_FACET)
  string(TOLOWER ${mutant} lower)
  mhgp9_product_executable(mhgp9_chain_static_paths_${lower}_mutant_gate
                           tests/chain/chain_static_paths_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(mhgp9_chain_static_paths_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_chain_static_paths_${lower}_mutant_gate PRIVATE MHGP9_TOWER_MUTANT_${mutant}=1)
  mhgp9_gate(mhgp9_chain_static_paths_mutant_${lower} 1 mhgp9_chain_static_paths_${lower}_mutant_gate
             ARGS --selftest EXPECT_PREFIX "cause=" LABELS "gate;chain;tower;mutant" TIMEOUT 300)
endforeach()
# R-29 (auditeur C, catalogue scelle) : positivite des supports reguliers et
# domaine des cles etablis par le recensement de la chaine, puis levier
# tower_sealed_catalogue (passe 1 de la tour sur l'echantillon 1/64). Cible
# de test (chaine recompilee avec MHGP9_CHAIN_TEST_SEAM : presentations
# forgees a la fusion, fautes de plomberie du catalogue). Refus directs de la
# tour (triangle obtus, tetraedre a centre exterieur), refus de la chaine
# (chain_nonpositive_regular_support, sous le sceau et sans ; refus types
# d'arite et d'ID), faute systematique prise par l'echantillon, residu isole
# declare, egalite scelle / non scelle (condenses, tower_work, ordres) a K3,
# K5, K10, 1 et 4 fils et voie CPU par lots (tower_work entier, echantillon
# compte a chaque controle), ordre des refus du recensement (plus petit
# indice de cle a 1, 2, 4 et 8 fils). Quatre mutants tues (code 1) : chaine
# sans positivite, tour sans positivite du support declare, echantillon
# jamais execute (tue par la section 3, et par la section 4 seule), premier
# refus du recensement dans le temps.
function(mhgp9_sealed_catalogue_executable name)
  mhgp9_product_executable(${name} tests/chain/sealed_catalogue_gate.cpp src/chain/tower_chain.cpp)
  target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gen")
  target_link_libraries(${name} PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(${name} PRIVATE MHGP9_CHAIN_TEST_SEAM=1 ${ARGN})
endfunction()
mhgp9_sealed_catalogue_executable(mhgp9_chain_sealed_catalogue_gate)
mhgp9_gate(mhgp9_chain_sealed_catalogue 0 mhgp9_chain_sealed_catalogue_gate ARGS --selftest
           EXPECT_PREFIX "sealed_catalogue_gate direct=4 forged=8 typed=3 faults=3 sealed_runs=10"
           LABELS "gate;chain;tower" TIMEOUT 900)
mhgp9_gate(mhgp9_chain_sealed_catalogue_bad_argument 2 mhgp9_chain_sealed_catalogue_gate ARGS --unknown
           LABELS "gate;chain" TIMEOUT 60)
foreach(pair IN ITEMS CHAIN_MUTANT_NO_POSITIVITY:forged.q3+sealed=1+W=1
                      TOWER_MUTANT_SUPPORT_NO_POSITIVITY:tower_direct.q3+threads=0
                      TOWER_MUTANT_SEAL_NO_SAMPLE:fault.systematic+sealed=1+W=4
                      CHAIN_MUTANT_CENSUS_FIRST_IN_TIME:census_order.delayed+W=2)
  string(REPLACE ":" ";" parts ${pair})
  list(GET parts 0 mutant)
  list(GET parts 1 cause)
  string(REPLACE "+" " " cause_words ${cause})
  string(REPLACE "CHAIN_MUTANT_" "" short ${mutant})
  string(REPLACE "TOWER_MUTANT_" "" short ${short})
  string(TOLOWER ${short} lower)
  mhgp9_sealed_catalogue_executable(mhgp9_chain_sealed_catalogue_${lower}_mutant_gate MHGP9_${mutant}=1)
  mhgp9_gate(mhgp9_chain_sealed_catalogue_mutant_${lower} 1 mhgp9_chain_sealed_catalogue_${lower}_mutant_gate
             ARGS --selftest EXPECT_PREFIX "cause=${cause_words}" LABELS "gate;chain;tower;mutant" TIMEOUT 900)
endforeach()
mhgp9_gate(mhgp9_chain_sealed_catalogue_mutant_seal_no_sample_equality 1
           mhgp9_chain_sealed_catalogue_seal_no_sample_mutant_gate ARGS --selftest --only=equality
           EXPECT_PREFIX "cause=equality.seal_path fixture K5 W1 sealed=1" LABELS "gate;chain;tower;mutant" TIMEOUT 900)
# Regroupement de la phase 0 sans tri des requetes de 56 o (classes de hachage
# exactes, tests/tower/static_grouping_gate.cpp) : temoin trie contre voie
# hachee, cibles statiques et premieres requetes octet pour octet a chaque
# ordre, W = 1, 2, 3, 4, 8, meme tour et memes compteurs. Cibles de test
# (motif 0xA5 des tampons non ecrits, table de groupes comprise) ; la chaine
# est recompilee dans chaque cible, pour que les defines du hachage et des
# mutants voient toute instanciation de la tour. Variante a hachage faible
# (quatre valeurs) : collisions forcees, separees par la comparaison exacte
# (planchers, classes et index des graines). Trois mutants tues (code 1) :
# etiquette de hachage prise pour l'egalite des cles des classes, puis des
# graines (avec le hachage faible) ; premiere requete d'une classe prise a
# l'ordinal maximal. La trame 08/000000 (label lidar, hors campagne
# gate) : condenses epingles, temoin a 8 fils, voie hachee a 1, 2, 3, 4, 8.
function(mhgp9_grouping_executable name)
  mhgp9_test_executable(${name} tests/tower/static_grouping_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(${name} PRIVATE mhgp9_gen mhgp9_gpu)
  if(ARGN)
    target_compile_definitions(${name} PRIVATE ${ARGN})
  endif()
endfunction()
mhgp9_grouping_executable(mhgp9_tower_static_grouping_gate)
mhgp9_grouping_executable(mhgp9_tower_static_grouping_weak_gate MHGP9_TOWER_GROUP_TEST_WEAK_HASH=1)
mhgp9_grouping_executable(mhgp9_tower_static_grouping_trust_hash_mutant_gate
                          MHGP9_TOWER_GROUP_TEST_WEAK_HASH=1 MHGP9_TOWER_GROUP_MUTANT_TRUST_HASH=1)
mhgp9_grouping_executable(mhgp9_tower_static_grouping_seed_trust_hash_mutant_gate
                          MHGP9_TOWER_GROUP_TEST_WEAK_HASH=1 MHGP9_TOWER_GROUP_MUTANT_SEED_TRUST_HASH=1)
mhgp9_grouping_executable(mhgp9_tower_static_grouping_last_ordinal_mutant_gate MHGP9_TOWER_GROUP_MUTANT_LAST_ORDINAL=1)
mhgp9_gate(mhgp9_tower_static_grouping 0 mhgp9_tower_static_grouping_gate ARGS --selftest
           EXPECT_PREFIX "static_grouping_gate mode=hash" LABELS "gate;tower" TIMEOUT 600)
mhgp9_gate(mhgp9_tower_static_grouping_weak_hash 0 mhgp9_tower_static_grouping_weak_gate ARGS --selftest
           EXPECT_PREFIX "static_grouping_gate mode=weak_hash" LABELS "gate;tower" TIMEOUT 600)
mhgp9_gate(mhgp9_tower_static_grouping_bad_argument 2 mhgp9_tower_static_grouping_gate ARGS --unknown
           LABELS "gate;tower" TIMEOUT 60)
mhgp9_gate(mhgp9_tower_static_grouping_mutant_trust_hash 1 mhgp9_tower_static_grouping_trust_hash_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=grouping.firsts" LABELS "gate;tower;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_tower_static_grouping_mutant_seed_trust_hash 1 mhgp9_tower_static_grouping_seed_trust_hash_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=grouping.targets" LABELS "gate;tower;mutant" TIMEOUT 600)
mhgp9_gate(mhgp9_tower_static_grouping_mutant_last_ordinal 1 mhgp9_tower_static_grouping_last_ordinal_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=grouping.firsts" LABELS "gate;tower;mutant" TIMEOUT 600)
set(MHGP9_FRAME_000000 ${CMAKE_CURRENT_SOURCE_DIR}/../morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le)
mhgp9_gate(mhgp9_tower_static_grouping_lidar_k5 0 mhgp9_tower_static_grouping_gate
           ARGS --file=${MHGP9_FRAME_000000} --k=5 --digest=67450c64611075b1 --catalogue-digest=5ad1fe09354411ba
                --threads=1,2,3,4,8 --witness-threads=8 --min-requests=1000000
           EXPECT_PREFIX "static_grouping_frame K=5" LABELS "lidar;grouping" TIMEOUT 3600)
mhgp9_gate(mhgp9_tower_static_grouping_lidar_k10 0 mhgp9_tower_static_grouping_gate
           ARGS --file=${MHGP9_FRAME_000000} --k=10 --digest=ac108f7f71096c3f --catalogue-digest=a6e959d227f3dafa
                --threads=1,2,3,4,8 --witness-threads=8 --min-requests=3000000
           EXPECT_PREFIX "static_grouping_frame K=10" LABELS "lidar;grouping" TIMEOUT 5400)
# Porte d'echelle de l'auditeur C : invariant d'Euler par ordre K sur le
# catalogue reel de la chaine (NOTE_C_INVARIANT_EULER_20260923, preuve sans
# position generale CONTRELEC_EULER_PAR_NERF_20260923). Trois familles v8
# epinglees a n = 8 000, chaine complete K5 (Euler K <= 3), puis K7 sur
# `uniform` (Euler K <= 5 et restriction p+q_min <= 6 egale cle par cle au
# catalogue K5). Condition NECESSAIRE de completude, jamais une preuve.
# Label scale8000, hors de la CI rapide `gate`.
mhgp9_product_executable(mhgp9_chain_euler_scale_gate tests/chain/euler_scale_gate.cpp)
target_link_libraries(mhgp9_chain_euler_scale_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_euler_scale8000 0 mhgp9_chain_euler_scale_gate
           ARGS --selftest EXPECT_PREFIX "euler_scale ok n=8000" LABELS "scale8000;chain;euler" TIMEOUT 1800)
mhgp9_gate(mhgp9_chain_euler_scale_bad_argument 2 mhgp9_chain_euler_scale_gate
           ARGS --unknown LABELS "gate;chain" TIMEOUT 60)
# Mutant d'omission du generateur que les recoupements de la chaine ne voient
# pas et que l'invariant doit tuer (depuis v13 par le refus Euler de la chaine) (disque q3 du certificat de voie morte trop
# petit, mutant v8 de tests/gen/mutants.json) : copie mutee de l'unite liee
# AVANT libmhgp9_gen, code 1 et cause euler exacte.
function(mhgp9_euler_scale_mutant mutant expect)
  set(found FALSE)
  string(JSON count LENGTH "${mhgp9_gen_mutant_text}" mutants)
  math(EXPR last "${count} - 1")
  foreach(i RANGE ${last})
    string(JSON name GET "${mhgp9_gen_mutant_text}" mutants ${i} name)
    if(name STREQUAL mutant)
      set(found TRUE)
      string(JSON unit GET "${mhgp9_gen_mutant_text}" mutants ${i} source)
      file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/gen/${unit}" text)
      string(JSON rcount LENGTH "${mhgp9_gen_mutant_text}" mutants ${i} replacements)
      math(EXPR rlast "${rcount} - 1")
      foreach(r RANGE ${rlast})
        string(JSON before GET "${mhgp9_gen_mutant_text}" mutants ${i} replacements ${r} 0)
        string(JSON after GET "${mhgp9_gen_mutant_text}" mutants ${i} replacements ${r} 1)
        string(FIND "${text}" "${before}" first)
        string(FIND "${text}" "${before}" final REVERSE)
        if(first EQUAL -1 OR NOT first EQUAL final)
          message(FATAL_ERROR "mhgp9: site du mutant ${mutant} absent ou non unique")
        endif()
        string(REPLACE "${before}" "${after}" text "${text}")
      endforeach()
      get_filename_component(unit_dir "${unit}" DIRECTORY)
      get_filename_component(unit_name "${unit}" NAME)
      set(mutated "${CMAKE_CURRENT_BINARY_DIR}/euler_scale_mutants/${mutant}/${unit_name}")
      file(WRITE "${mutated}.staged" "${text}")
      configure_file("${mutated}.staged" "${mutated}" COPYONLY)
      mhgp9_product_executable(mhgp9_chain_euler_scale_mutant_${mutant}_gate tests/chain/euler_scale_gate.cpp "${mutated}")
      target_include_directories(mhgp9_chain_euler_scale_mutant_${mutant}_gate PRIVATE
                                 "${CMAKE_CURRENT_SOURCE_DIR}/src/gen" "${CMAKE_CURRENT_SOURCE_DIR}/src/gen/${unit_dir}")
      target_link_libraries(mhgp9_chain_euler_scale_mutant_${mutant}_gate PRIVATE mhgp9_chain)
      mhgp9_gate(mhgp9_chain_euler_scale_mutant_${mutant} 1 mhgp9_chain_euler_scale_mutant_${mutant}_gate
                 ARGS --selftest EXPECT_PREFIX "${expect}" LABELS "scale8000;chain;euler;mutant" TIMEOUT 1800)
    endif()
  endforeach()
  if(NOT found)
    message(FATAL_ERROR "mhgp9: mutant ${mutant} absent de tests/gen/mutants.json (porte Euler d'echelle)")
  endif()
endfunction()
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
                     "-DEXPECT_PREFIX=probe_worker_contract mutants_killed=61/61"
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
    set_tests_properties(mhgp9_probe_worker_contract_${mode} PROPERTIES LABELS "gate;probe;protocol" TIMEOUT 300)
  endforeach()
endif()
# Lecteur du runner de pente LiDAR (contre-audit B du 23 septembre 2026) : un
# cas archive dont l'entree est un morceau v8 versionne est accepte, et 46
# alterations (FNV d'entree, format, grille, fils statiques, champs lus apres
# validation, IDs du morceau, chemin absolu d'un autre checkout...) sont
# refusees ou re-ancrees. Sous python3 -O aussi.
set(MHGP9_LIDAR_SCALING_CASE
    ${CMAKE_CURRENT_SOURCE_DIR}/receipts/lidar_scaling_local_20260923/out/s01_k5_w8_r0/s01_k5_s8_w8_r0_piece_quarter_x_nonneg_y_nonneg.json)
if(Python3_Interpreter_FOUND AND EXISTS ${MHGP9_LIDAR_SCALING_CASE})
  foreach(mode IN ITEMS normal optimized)
    if(mode STREQUAL "optimized")
      set(py_flags "-B -O")
    else()
      set(py_flags "-B")
    endif()
    add_test(NAME mhgp9_lidar_scaling_reader_${mode}
             COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=${Python3_EXECUTABLE}
                     "-DARGS=${py_flags} ${CMAKE_CURRENT_SOURCE_DIR}/bench/run_lidar_scaling.py --selftest ${MHGP9_LIDAR_SCALING_CASE}"
                     "-DEXPECT_PREFIX=lidar_scaling_selftest mutants_killed=47/47"
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
    set_tests_properties(mhgp9_lidar_scaling_reader_${mode} PROPERTIES LABELS "gate;protocol" TIMEOUT 120)
  endforeach()
  # Revalidation du reçu archive contre sa matrice ANNONCEE (six campagnes,
  # binaire e1ba126f, commit 4530644b), puis la meme matrice augmentee d'une
  # campagne absente : refus (contre-audit B, sous-ensemble non vide).
  set(MHGP9_LIDAR_SCALING_TAGS
      "s00_k10_s8_w8_r0,s00_k5_s8_w8_r0,s01_k10_s8_w8_r0,s01_k5_s8_w8_r0,s02_k10_s8_w8_r0,s02_k5_s8_w8_r0")
  foreach(variant IN ITEMS matrix missing_campaign)
    if(variant STREQUAL "matrix")
      set(tags "${MHGP9_LIDAR_SCALING_TAGS}")
      set(expected 0)
      set(line "lidar_scaling_revalidate cases=60 failures=0")
    else()
      set(tags "${MHGP9_LIDAR_SCALING_TAGS},s00_k5_s10_w8_r0")
      set(expected 1)
      set(line "lidar_scaling_revalidate cases=60 failures=1")
    endif()
    add_test(NAME mhgp9_lidar_scaling_revalidate_${variant}
             COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -DCMD=${Python3_EXECUTABLE}
                     "-DARGS=-B ${CMAKE_CURRENT_SOURCE_DIR}/bench/run_lidar_scaling.py --revalidate ${CMAKE_CURRENT_SOURCE_DIR}/receipts/lidar_scaling_local_20260923/out --work ${CMAKE_CURRENT_BINARY_DIR}/lidar_revalidate_${variant} --expect-campaigns ${tags} --expect-probe-sha256 e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80 --expect-commit 4530644b"
                     "-DEXPECT_PREFIX=${line}"
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
    set_tests_properties(mhgp9_lidar_scaling_revalidate_${variant} PROPERTIES LABELS "gate;protocol" TIMEOUT 300)
  endforeach()
endif()

# Mutant de compilation du noyau MEB (derniere paire maximale au lieu de la
# premiere) : la porte doit le refuser (code 1, support canonique faux).
# MEB propose (Welzl en double, verification exacte, canonisation au bord) :
# meme resultat que l'enumeration de reference, et son mutant sans
# canonisation tue.
mhgp9_product_executable(mhgp9_tower_anchor_meb_proposed_gate tests/tower/anchor_meb_proposed_gate.cpp)
mhgp9_gate(mhgp9_tower_anchor_meb_proposed 0 mhgp9_tower_anchor_meb_proposed_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 300)
mhgp9_product_executable(mhgp9_tower_anchor_meb_proposed_fallback_gate tests/tower/anchor_meb_proposed_gate.cpp)
target_compile_definitions(mhgp9_tower_anchor_meb_proposed_fallback_gate PRIVATE MHGP9_MEB_PROPOSED_TEST_CORRUPT=1)
mhgp9_gate(mhgp9_tower_anchor_meb_proposed_fallback 0 mhgp9_tower_anchor_meb_proposed_fallback_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 300)
mhgp9_product_executable(mhgp9_tower_anchor_meb_proposed_mutant_gate tests/tower/anchor_meb_proposed_gate.cpp)
target_compile_definitions(mhgp9_tower_anchor_meb_proposed_mutant_gate PRIVATE MHGP9_MEB_PROPOSED_MUTANT_NO_CANONICAL=1)
mhgp9_gate(mhgp9_tower_anchor_meb_proposed_mutant_no_canonical 1 mhgp9_tower_anchor_meb_proposed_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=proposed.differs" LABELS "gate;tower;mutant" TIMEOUT 300)
# Index exact des cles de la tour : une cle jamais indexee est detectee par
# le juge T2 (tour Gamma bornee), voie statique a quatre fils.
mhgp9_product_executable(mhgp9_tower_key_index_mutant_gate tests/tower/full_ball_tower_gate.cpp)
target_compile_definitions(mhgp9_tower_key_index_mutant_gate PRIVATE MHGP9_KEY_INDEX_MUTANT_DROP_FIRST=1)
target_include_directories(mhgp9_tower_key_index_mutant_gate SYSTEM PRIVATE ${MHGP9_BOOST_INCLUDE_DIR})
mhgp9_gate(mhgp9_tower_key_index_mutant_drop_first 1 mhgp9_tower_key_index_mutant_gate
           ARGS --static-4 LABELS "gate;tower;mutant" TIMEOUT 300)
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

# Mutant d'omission tue par la porte d'echelle d'Euler (auditeur C) ; appele
# apres la lecture de tests/gen/mutants.json.
# Depuis la sonde v13, la chaine refuse elle-meme un catalogue qui viole Euler :
# le mutant est tue par ce refus, avant le calcul independant de la porte.
mhgp9_euler_scale_mutant(dead_q3_disk_too_small
                         "cause=chain.status family=uniform kmax=5 reason=chain_catalogue_euler_violated")
