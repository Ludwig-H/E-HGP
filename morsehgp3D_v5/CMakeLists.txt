# MorseHGP3D v5 — exploration hors registre, reprise a propre de la v4.
#
#   cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/v5 --parallel
#   ctest --test-dir build/v5 --output-on-failure
#
# Prefixe des cibles et des tests : mhgp5_. Portes a code de sortie exact
# (cmake/run_expect.cmake) : 0 conforme, 1 desaccord du juge, 2 refus avant
# calcul, 3 plancher/invariant viole, 4 mutant tue. Crash par signal refuse.
#
# Labels CTest : `oracle` (petits n, verite etablie), `gate` (fixtures, mutants,
# invariants), `scale8000` / `scale16000` / `scale32000` (tailles d'interet du
# plan de test — les seules ou une conclusion de cout ou d'echelle se lit).
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v5 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -Wpedantic -Werror)
find_package(Threads REQUIRED)
enable_testing()

set(_mhgp5_expect ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)

# mhgp5_gate(<nom> <code attendu> <cible> "<args>" [LABELS l1 l2] [TIMEOUT s])
function(mhgp5_gate name expected target args)
  cmake_parse_arguments(G "" "TIMEOUT" "LABELS" ${ARGN})
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected}
                   "-DCMD=$<TARGET_FILE:${target}>" "-DARGS=${args}"
                   -P ${_mhgp5_expect})
  if(NOT G_LABELS)
    set(G_LABELS gate)
  endif()
  set_tests_properties(${name} PROPERTIES LABELS "${G_LABELS}")
  if(G_TIMEOUT)
    set_tests_properties(${name} PROPERTIES TIMEOUT ${G_TIMEOUT})
  endif()
endfunction()

function(mhgp5_executable name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

# ---- Fondations --------------------------------------------------------------

# Index spatial : invariants structurels, equivariance, garde d'entree.
mhgp5_executable(mhgp5_tree_selftest tests/tree_selftest.cpp)
mhgp5_gate(mhgp5_tree_selftest 0 mhgp5_tree_selftest "")

# Familles de nuages : port bit a bit de la v4 prouve par digests graves
# (receipts/conformite_v4/familles_v4.txt) ; contrat de cardinalite et son mutant.
mhgp5_executable(mhgp5_families_fixture tests/families_fixture.cpp)
mhgp5_gate(mhgp5_families_fixture 0 mhgp5_families_fixture "")
mhgp5_gate(mhgp5_families_mutant_overshoot 4 mhgp5_families_fixture "--inject=family-scanline-overshoot")
mhgp5_gate(mhgp5_families_refus_mutant_inconnu 2 mhgp5_families_fixture "--inject=inconnu")

# ---- WSPD --------------------------------------------------------------------

# Ledger exact + plancher + equivariance, une configuration par famille de
# mesure ; contre-famille two_lines ; mutants (rectangle perdu -> ledger ;
# cap dans le critere terminal et scission du plus peuple -> portes appariees).
mhgp5_executable(mhgp5_wspd_gate tests/wspd_gate.cpp)
foreach(fam uniform terrain eight_clusters scanline_single_pass scanline_overlap_multiecho)
  mhgp5_gate(mhgp5_wspd_ledger_${fam} 0 mhgp5_wspd_gate
      "--family=${fam} --n=2000 --s=8 --seed=3 --min-rect=1000 --check-permutation")
endforeach()
mhgp5_gate(mhgp5_wspd_ledger_two_lines 0 mhgp5_wspd_gate "--family=two_lines --n=2000 --s=8 --min-rect=100")
mhgp5_gate(mhgp5_wspd_mutant_drop_rect 4 mhgp5_wspd_gate "--family=uniform --n=2000 --s=8 --inject=wspd-drop-rect")
mhgp5_gate(mhgp5_wspd_discriminate_cap 0 mhgp5_wspd_gate "--family=two_lines --n=2000 --s=8 --discriminate-cap")
mhgp5_gate(mhgp5_wspd_discriminate_split 0 mhgp5_wspd_gate "--family=eight_clusters --n=2000 --s=8 --seed=3 --discriminate-split")
mhgp5_gate(mhgp5_wspd_refus_famille 2 mhgp5_wspd_gate "--family=inconnue")
mhgp5_gate(mhgp5_wspd_refus_n 2 mhgp5_wspd_gate "--n=1")

# ---- Fuseaux et temoins ------------------------------------------------------

# Juge fail-open du comptage fusionne (cœur <= vrai compte pour toute ancre
# echantillonnee), fixtures ponctuelles W_2/W_3/W_4, boule-cœur ⊆ fuseau,
# planchers de mort ; mutants de credit fantome tues.
mhgp5_executable(mhgp5_spindle_gate tests/spindle_gate.cpp)
mhgp5_gate(mhgp5_spindle_judge_uniform 0 mhgp5_spindle_gate "--family=uniform --n=1500 --s=8 --seed=3 --min-dead-pct=50")
mhgp5_gate(mhgp5_spindle_judge_clusters 0 mhgp5_spindle_gate "--family=eight_clusters --n=1500 --s=8 --seed=3 --min-dead-pct=30")
mhgp5_gate(mhgp5_spindle_judge_terrain 0 mhgp5_spindle_gate "--family=terrain --n=1500 --s=8 --seed=3 --min-dead-pct=50")
mhgp5_gate(mhgp5_spindle_judge_two_lines 0 mhgp5_spindle_gate "--family=two_lines --n=1200 --s=8")
mhgp5_gate(mhgp5_spindle_mutant_ceil 4 mhgp5_spindle_gate "--family=uniform --n=1500 --s=8 --seed=3 --inject=core-ball-ceil-distance")
mhgp5_gate(mhgp5_spindle_mutant_mask 4 mhgp5_spindle_gate "--family=uniform --n=1500 --s=8 --seed=3 --inject=witness-no-lane-mask")

# ---- Pilote ------------------------------------------------------------------

mhgp5_executable(mhgp5 cli/mhgp5.cpp)

# ---- Conformite v4 ≡ v5 --------------------------------------------------------

# Digests canoniques (format v4) calcules par la v4 sur les memes entrees :
# receipts/conformite_v4/digests_v4.txt. Les tailles d'interet portent leur
# label ; les petits n sont des portes rapides. Tous a 8 fils : la sortie est
# bit-identique quel que soit le nombre de fils (porte --par-gate).
mhgp5_executable(mhgp5_conformity_v4 tests/conformity_v4.cpp)
set(_recu ${CMAKE_CURRENT_SOURCE_DIR}/receipts/conformite_v4/digests_v4.txt)
foreach(fam uniform terrain eight_clusters scanline_single_pass)
  mhgp5_gate(mhgp5_conformity_${fam}_n8000 0 mhgp5_conformity_v4
      "--receipt=${_recu} --family=${fam} --n=8000 --threads=8" LABELS scale8000 TIMEOUT 3600)
  mhgp5_gate(mhgp5_conformity_${fam}_n16000 0 mhgp5_conformity_v4
      "--receipt=${_recu} --family=${fam} --n=16000 --threads=8" LABELS scale16000 TIMEOUT 7200)
  mhgp5_gate(mhgp5_conformity_${fam}_n32000 0 mhgp5_conformity_v4
      "--receipt=${_recu} --family=${fam} --n=32000 --threads=8" LABELS scale32000 TIMEOUT 14400)
endforeach()
