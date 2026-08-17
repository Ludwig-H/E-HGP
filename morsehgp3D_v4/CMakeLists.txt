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
add_executable(mhgp4_q2_fixture tests/q2_fixture_test.cpp)
add_executable(mhgp4_q2_prune_probe bench/q2_prune_probe.cpp)
add_executable(mhgp4_q234_prune_probe bench/q234_prune_probe.cpp)

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

# Fixture gravee de l'arrondi du rayon (floor obligatoire) et son mutant.
mhgp4_expect_code(mhgp4_q2_fixture_saine 0 mhgp4_q2_fixture "")
mhgp4_expect_code(mhgp4_q2_fixture_mutant 4 mhgp4_q2_fixture "--inject=radius-ceil")

# Descente ternaire q2 : ledger exact + juge fail-open sur les blocs morts +
# plancher de fermeture, avec vrai vivant exhaustif (oracle borne petit n).
mhgp4_expect_code(mhgp4_q2_prune_judge_uniform 0 mhgp4_q2_prune_probe
    "--family=uniform --n=1500 --s=8 --seed=3 --true-alive --min-dead-pct=50")
mhgp4_expect_code(mhgp4_q2_prune_judge_clusters 0 mhgp4_q2_prune_probe
    "--family=eight_clusters --n=1500 --s=8 --seed=3 --true-alive --min-dead-pct=30")
mhgp4_expect_code(mhgp4_q2_prune_judge_terrain 0 mhgp4_q2_prune_probe
    "--family=terrain --n=1500 --s=8 --seed=3 --true-alive --min-dead-pct=50")

# Trois lanes, une vague : ledgers exacts par lane + juge in_spindle sur les
# blocs morts. Sur two_lines les TEMOINS de fuseau existent bel et bien pour
# les trois lanes (un point collineaire entre a et b a Xi=0 et H>0 : il est
# dans W_4) — ce sont les PORTEURS aigus qui n'existent pas ; la porte ne
# verifie donc que les invariants et le juge, pas une vacuite de lane
# (rectificatif de l'audit du 17 aout).
mhgp4_expect_code(mhgp4_q234_judge_uniform 0 mhgp4_q234_prune_probe
    "--family=uniform --n=1200 --s=8 --seed=3 --min-dead-pct=25")
mhgp4_expect_code(mhgp4_q234_judge_two_lines 0 mhgp4_q234_prune_probe
    "--family=two_lines --n=1200 --s=8")
# Phase h_a/h_b : histogramme + juge des ancres tuees par la somme des trois
# comptes (theoreme de disjonction) — la famille adversariale des temoins.
mhgp4_expect_code(mhgp4_q234_ha_clusters 0 mhgp4_q234_prune_probe
    "--family=eight_clusters --n=1200 --s=8 --seed=3 --ha")

# Refus avant calcul : famille inconnue, n insuffisant, profil de capacite
# (K_max <= 10 ⟺ smax <= 11 : les tampons de certificats et d'interieurs
# sont dimensionnes h_3-1 <= 8 — refus explicite, jamais une troncature).
mhgp4_expect_code(mhgp4_wspd_refus_famille 2 mhgp4_wspd_scaling_probe
    "--family=inconnue")
mhgp4_expect_code(mhgp4_wspd_refus_n 2 mhgp4_wspd_scaling_probe "--n=1")
mhgp4_expect_code(mhgp4_q3_refus_smax 2 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --smax=12")

# Instruction q3 : ancres survivantes -> evenements, juge PAR IDENTITES
# contre l'oracle brut borne (tous les triangles). Le juge partage encore le
# predicat de profondeur du sujet (limite declaree : il valide l'enumeration,
# l'owner et la completude, pas l'arithmetique — l'oracle a arithmetique
# independante viendra avec oracle/).
add_executable(mhgp4_q3_events_probe bench/q3_events_probe.cpp)
mhgp4_expect_code(mhgp4_q3_events_judge_uniform 0 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --min-events=100")
mhgp4_expect_code(mhgp4_q3_events_judge_clusters 0 mhgp4_q3_events_probe
    "--family=eight_clusters --n=400 --s=8 --seed=3 --judge --min-events=100")

# Oracle rationnel independant q3 (obigint 384 bits, ecrit a neuf, durci par
# l'audit ebc8236) : accord total exige sur tous les triangles des petits
# nuages + fixtures cosphere/tetraedre + fixtures u16 EXTREMES (equilateral
# M=65535, presque-rectangle aigu, grande cosphere, rejeu translate au bord),
# plancher de limbes (le produit du niveau traverse le limbe 3) ; mutants
# sign-p, prune-ge, cramer-swap, level-4g et mul-carry-lost tues.
add_executable(mhgp4_q3_oracle tests/q3_oracle_test.cpp)
mhgp4_expect_code(mhgp4_q3_oracle_accord 0 mhgp4_q3_oracle "")
mhgp4_expect_code(mhgp4_q3_oracle_mutant_sign 4 mhgp4_q3_oracle "--inject=sign-p")
mhgp4_expect_code(mhgp4_q3_oracle_mutant_prune 4 mhgp4_q3_oracle "--inject=prune-ge")
mhgp4_expect_code(mhgp4_q3_oracle_mutant_cramer 4 mhgp4_q3_oracle "--inject=cramer-swap")
mhgp4_expect_code(mhgp4_q3_oracle_mutant_level4g 4 mhgp4_q3_oracle "--inject=level-4g")
mhgp4_expect_code(mhgp4_q3_oracle_mutant_carry 4 mhgp4_q3_oracle "--inject=mul-carry-lost")

# Ordre exact des niveaux q3 (contre-audit 489c617 § 2) : U192 contre
# l'oracle 384 bits sur toutes les paires de niveaux recoltes, plancher de
# plateaux (le cas des futurs macro-lots), fixture de largeur gravee ;
# mutant mot-haut-tronque tue.
add_executable(mhgp4_q3_level_cmp tests/q3_level_compare_test.cpp)
mhgp4_expect_code(mhgp4_q3_level_cmp_accord 0 mhgp4_q3_level_cmp "")
mhgp4_expect_code(mhgp4_q3_level_cmp_mutant 4 mhgp4_q3_level_cmp
    "--inject=level-trunc-hi")

# Fixture bloquante avant q4 (audit bc5b05d § 2) : l'ancre (a,b) est q3-morte
# (n3=9>=h3) mais q4-vivante (n4=0<h4=8) et porte un tetraedre q4 de
# profondeur zero — une source q4 branchee apres le filtre q3 le perd. Le
# mutant q4-seeds-from-q3-live grave ce decouplage architectural.
add_executable(mhgp4_q4_source_fixture tests/q4_source_fixture_test.cpp)
mhgp4_expect_code(mhgp4_q4_source_independante 0 mhgp4_q4_source_fixture "")
mhgp4_expect_code(mhgp4_q4_source_mutant 4 mhgp4_q4_source_fixture
    "--inject=q4-seeds-from-q3-live")

# Le juge du juge (audit ebc8236 P0.2) : selftest de l'arithmetique OBig
# contre boost::multiprecision::cpp_int, troisieme autorite, tests uniquement.
# Enregistre seulement si Boost est present (meme precedent que le temoin GMP
# optionnel de v3) ; l'oracle geometrique reste garde sans lui.
find_path(MHGP4_BOOST_INCLUDE boost/multiprecision/cpp_int.hpp)
if(MHGP4_BOOST_INCLUDE)
  add_executable(mhgp4_obig_selftest tests/obig_selftest.cpp)
  target_include_directories(mhgp4_obig_selftest SYSTEM PRIVATE ${MHGP4_BOOST_INCLUDE})
  mhgp4_expect_code(mhgp4_obig_selftest_gate 0 mhgp4_obig_selftest "")
  mhgp4_expect_code(mhgp4_obig_mutant_carry 4 mhgp4_obig_selftest
      "--inject=mul-carry-lost")
else()
  message(WARNING "mhgp4 : boost/multiprecision/cpp_int.hpp introuvable — "
      "selftest OBig (le juge du juge) non enregistre.")
endif()

# Paquets de temoins certifies + scan site-major (audit du 17 aout) : la
# porte causale packet=off passe par le meme juge d'identites ; le mutant
# packet-no-exclude (double compte des IDs du paquet) est tue.
mhgp4_expect_code(mhgp4_q3_packet_off_judge 0 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --packet=off --min-events=100")
mhgp4_expect_code(mhgp4_q3_packet_mutant 4 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --inject=packet-no-exclude")

# Cover rectangulaire (une traversee haute par rectangle, boite des sommes,
# filtre exact par ancre) : porte appariee cover=root et mutant Dmin tue.
mhgp4_expect_code(mhgp4_q3_cover_root_judge 0 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --cover=root --min-events=100")
mhgp4_expect_code(mhgp4_q3_cover_dmin_mutant 4 mhgp4_q3_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --inject=cover-dmin")
