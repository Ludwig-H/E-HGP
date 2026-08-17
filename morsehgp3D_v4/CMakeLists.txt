# MorseHGP3D v4 — exploration hors registre, reprise de zero.
#
#   cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/v4 --parallel
#   ctest --test-dir build/v4 --output-on-failure
#
# Prefixe des cibles et des tests : mhgp4_. Portes negatives a code de sortie
# exact via cmake/run_expect.cmake (1 = desaccords du juge, 2 = refus avant
# calcul, 3 = plancher/invariant viole, 4 = mutant tue).
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v4 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

enable_testing()

# ---- Executables -----------------------------------------------------------

add_executable(mhgp4_tree_selftest tests/tree_selftest.cpp)
add_executable(mhgp4_wspd_scaling_probe bench/wspd_scaling_probe.cpp)

# ---- Portes ----------------------------------------------------------------

set(_expect ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
function(mhgp4_expect_code name expected target args)
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected}
                   "-DCMD=$<TARGET_FILE:${target}>" "-DARGS=${args}"
                   -P ${_expect})
endfunction()

# Invariants structurels de l'arbre (fixtures gravees + familles, permutation).
mhgp4_expect_code(mhgp4_tree_selftest_gate 0 mhgp4_tree_selftest "")

# Ledger de masse exact + permutation, une configuration par famille de mesure.
foreach(fam uniform terrain eight_clusters scanline_single_pass)
  mhgp4_expect_code(mhgp4_wspd_ledger_${fam} 0 mhgp4_wspd_scaling_probe
      "--family=${fam} --n=2000 --s=8 --seed=3 --min-rect=1000 --check-permutation")
endforeach()

# Contre-famille deterministe : le ledger doit couvrir n(n-1)/2 malgre la
# geometrie adversariale des deux droites.
mhgp4_expect_code(mhgp4_wspd_ledger_two_lines 0 mhgp4_wspd_scaling_probe
    "--family=two_lines --n=2000 --s=8 --min-rect=100")

# Mutants : un rectangle perdu est tue par le ledger ; le cap v3 dans le
# critere terminal est tue par la porte discriminante appariee.
mhgp4_expect_code(mhgp4_wspd_mutant_drop_rect 4 mhgp4_wspd_scaling_probe
    "--family=uniform --n=2000 --s=8 --inject=drop-rect")
# `uniform` a n=2000 ne discrimine PAS ce mutant (« le cap ne mord pas »,
# constat v3 du 16 aout 2026) : il faut des blocs bien separes plus gros que
# le cap — `two_lines` en fournit par construction (facteur ~1,18 mesure).
mhgp4_expect_code(mhgp4_wspd_mutant_cap_terminal 4 mhgp4_wspd_scaling_probe
    "--family=two_lines --n=2000 --s=8 --discriminate-cap")

# Refus avant calcul : famille inconnue, n insuffisant.
mhgp4_expect_code(mhgp4_wspd_refus_famille 2 mhgp4_wspd_scaling_probe
    "--family=inconnue")
mhgp4_expect_code(mhgp4_wspd_refus_n 2 mhgp4_wspd_scaling_probe "--n=1")
