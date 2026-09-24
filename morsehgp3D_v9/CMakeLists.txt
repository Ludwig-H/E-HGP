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
# Tri parallele de la tour : permutation egale a std::sort pour tout nombre de
# fils, et son mutant compile (un seau laisse non trie) tue.
mhgp9_product_executable(mhgp9_tower_parallel_sort_gate tests/tower/parallel_sort_gate.cpp)
mhgp9_gate(mhgp9_tower_parallel_sort 0 mhgp9_tower_parallel_sort_gate
           ARGS --selftest LABELS "gate;tower" TIMEOUT 300)
mhgp9_product_executable(mhgp9_tower_parallel_sort_mutant_gate tests/tower/parallel_sort_gate.cpp)
target_compile_definitions(mhgp9_tower_parallel_sort_mutant_gate PRIVATE MHGP9_PARALLEL_SORT_MUTANT_UNSORTED_BUCKET=1)
mhgp9_gate(mhgp9_tower_parallel_sort_mutant_unsorted_bucket 1 mhgp9_tower_parallel_sort_mutant_gate
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
# Priorite des echecs de la tour (plus petit K sur les phases A et C) et
# travail paye compte apres echec : points de panne MHGP9_TESTING, chaine
# recompilee dans la cible de test (le generateur ne voit pas la tour).
mhgp9_test_executable(mhgp9_chain_order_failure_priority_gate
                      tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
target_link_libraries(mhgp9_chain_order_failure_priority_gate PRIVATE mhgp9_gen mhgp9_gpu)
mhgp9_gate(mhgp9_chain_order_failure_priority 0 mhgp9_chain_order_failure_priority_gate
           ARGS --selftest LABELS "gate;chain;tower" TIMEOUT 300)
foreach(mutant IN ITEMS PHASE_PRIORITY DROP_FAILED_STATS)
  string(TOLOWER ${mutant} lower)
  mhgp9_test_executable(mhgp9_chain_order_failure_${lower}_mutant_gate
                        tests/chain/order_failure_priority_gate.cpp src/chain/tower_chain.cpp)
  target_link_libraries(mhgp9_chain_order_failure_${lower}_mutant_gate PRIVATE mhgp9_gen mhgp9_gpu)
  target_compile_definitions(mhgp9_chain_order_failure_${lower}_mutant_gate
                             PRIVATE MHGP9_FULL_ORDERS_MUTANT_${mutant}=1)
endforeach()
mhgp9_gate(mhgp9_chain_order_failure_mutant_phase_priority 1 mhgp9_chain_order_failure_phase_priority_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=priority" LABELS "gate;chain;tower;mutant" TIMEOUT 300)
mhgp9_gate(mhgp9_chain_order_failure_mutant_drop_failed_stats 1 mhgp9_chain_order_failure_drop_failed_stats_mutant_gate
           ARGS --selftest EXPECT_PREFIX "cause=ledger.paid_work_lost" LABELS "gate;chain;tower;mutant" TIMEOUT 300)
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
mhgp9_product_executable(mhgp9_gpu_certificate_port_gate tests/gpu/certificate_port_gate.cpp)
target_link_libraries(mhgp9_gpu_certificate_port_gate PRIVATE mhgp9_gen)
target_include_directories(mhgp9_gpu_certificate_port_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_gpu_certificate_port 0 mhgp9_gpu_certificate_port_gate ARGS --n=2000
           EXPECT_PREFIX "gpu_certificate_port_gate n=2000" LABELS "gate;gpu;oracle" TIMEOUT 900)
mhgp9_gate(mhgp9_gpu_certificate_port_scale8000 0 mhgp9_gpu_certificate_port_gate ARGS --n=8000 --k=5
           EXPECT_PREFIX "gpu_certificate_port_gate n=8000" LABELS "scale8000;gpu;oracle" TIMEOUT 2400)
mhgp9_gate(mhgp9_gpu_certificate_port_bad_argument 2 mhgp9_gpu_certificate_port_gate ARGS --k=11
           LABELS "gate;gpu" TIMEOUT 60)
# S4a : port portable de la voie q3 sans atlas (gpu/lanes.hpp, emulation
# hote), juge arete par arete contre la voie q3 du moteur, executeur par lots,
# capacites reduites, mutants de sortie et garde d'entree.
mhgp9_product_executable(mhgp9_gpu_lanes_port_gate tests/gpu/lanes_port_gate.cpp)
target_link_libraries(mhgp9_gpu_lanes_port_gate PRIVATE mhgp9_gen)
target_include_directories(mhgp9_gpu_lanes_port_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
mhgp9_gate(mhgp9_gpu_lanes_port 0 mhgp9_gpu_lanes_port_gate ARGS --n=2000
           EXPECT_PREFIX "lanes_port_gate n=2000" LABELS "gate;gpu;oracle" TIMEOUT 900)
mhgp9_gate(mhgp9_gpu_lanes_port_scale8000 0 mhgp9_gpu_lanes_port_gate ARGS --n=8000 --k=5
           EXPECT_PREFIX "lanes_port_gate n=8000" LABELS "scale8000;gpu;oracle" TIMEOUT 2400)
mhgp9_gate(mhgp9_gpu_lanes_port_bad_argument 2 mhgp9_gpu_lanes_port_gate ARGS --k=11
           LABELS "gate;gpu" TIMEOUT 60)
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
mhgp9_product_executable(mhgp9_chain_static_paths_gate tests/chain/chain_static_paths_gate.cpp)
target_link_libraries(mhgp9_chain_static_paths_gate PRIVATE mhgp9_chain)
mhgp9_gate(mhgp9_chain_static_paths 0 mhgp9_chain_static_paths_gate
           ARGS --selftest LABELS "gate;chain" TIMEOUT 300)
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
                     "-DEXPECT_PREFIX=lidar_scaling_selftest mutants_killed=46/46"
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
