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
  src/gen/lanes/q3_ball_census.cpp)
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
mhgp9_gate(mhgp9_chain_census_tower_bad_argument 2 mhgp9_chain_census_tower_gate
           ARGS --unknown LABELS "gate;chain" TIMEOUT 60)
mhgp9_gate(mhgp9_chain_census_tower_mutant_census 1 mhgp9_chain_census_tower_gate
           ARGS --mutant-census EXPECT_LINE "cause=T2.census.exhaustive_ball_inventory"
           LABELS "gate;chain;oracle;mutant" TIMEOUT 120)
