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
option(MHGP5_ENABLE_SANITIZERS "ASan + UBSan sur toutes les cibles (incompatible CUDA)" OFF)
if(MHGP5_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()
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

# Cible de TEST : les points d'injection des mutants y sont compiles
# (MHGP5_TESTING). Une cible produit (mhgp5_product_executable) ne les a pas.
function(mhgp5_executable name source)
  add_executable(${name} ${source})
  target_compile_definitions(${name} PRIVATE MHGP5_TESTING=1)
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()
function(mhgp5_product_executable name source)
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

mhgp5_product_executable(mhgp5 cli/mhgp5.cpp)

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
# Petites tailles (portes rapides, label gate) : toutes les familles a n=400,
# uniform/eight_clusters a 1200, two_lines a 2000.
foreach(fam eight_clusters terrain scanline_single_pass scanline_overlap_multiecho)
  mhgp5_gate(mhgp5_conformity_${fam}_n400 0 mhgp5_conformity_v4 "--receipt=${_recu} --family=${fam} --n=400 --threads=8")
endforeach()
mhgp5_gate(mhgp5_conformity_uniform_n1200 0 mhgp5_conformity_v4 "--receipt=${_recu} --family=uniform --n=1200 --threads=8")
mhgp5_gate(mhgp5_conformity_eight_clusters_n1200 0 mhgp5_conformity_v4 "--receipt=${_recu} --family=eight_clusters --n=1200 --threads=8")
mhgp5_gate(mhgp5_conformity_two_lines_n2000 0 mhgp5_conformity_v4 "--receipt=${_recu} --family=two_lines --n=2000 --threads=8" LABELS slow TIMEOUT 1800)
mhgp5_gate(mhgp5_conformity_refus_absent 2 mhgp5_conformity_v4 "--receipt=${_recu} --family=collinear_seven --n=9")

# ---- Registre des mutants (Python 3 requis, comme les portes F0 de la v3) -------
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_test(NAME mhgp5_mutants_gate
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 "-DCMD=${Python3_EXECUTABLE}"
                   "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/mutants_gate.py" -P ${_mhgp5_expect})
  set_tests_properties(mhgp5_mutants_gate PROPERTIES LABELS gate)
else()
  message(WARNING "mhgp5 : Python 3 introuvable — porte du registre des mutants non enregistree")
endif()

# ---- Mutants du pipeline (tues par divergence de digest, code 4) ---------------
# Les mutants FAIL-OPEN (ils ne changent pas l'objet sur une famille reguliere :
# q3-prune-ge, q4-eq-nonstrict, q4-seeds-from-q3-live, q4-cover-coef3,
# q4-no-canonical, q4-seed-core-nonstrict, jung-swap-bounds, cover-rect-dmin,
# level-trunc-hi) ont chacun leur porte DEDIEE a fixture discriminante (oracles
# q3/q4, fixture 13 points, level_cmp, affine) — jamais cette boucle.
foreach(m rle-drop census-nonstrict genfilter-nonstrict depth-threshold-minus-one range-add-max-le-zero skip-full-census
          binary-ties repr-ties attach-prebatch drop-nonmerge canonical-is-uf-root dense-pointid
          q3-level-4g q4-center-parity q4-eq-sign q4-i64-drop-factor q4-i64-pair-min)
  mhgp5_gate(mhgp5_pipeline_mutant_${m} 4 mhgp5_conformity_v4
      "--receipt=${_recu} --family=eight_clusters --n=400 --threads=8 --inject=${m}")
endforeach()

# ---- Parallelisme et relabeling ------------------------------------------------
mhgp5_executable(mhgp5_par_gate tests/par_gate.cpp)
mhgp5_gate(mhgp5_par_gate_uniform 0 mhgp5_par_gate "--family=uniform --n=400 --threads=8")
mhgp5_gate(mhgp5_par_gate_clusters 0 mhgp5_par_gate "--family=eight_clusters --n=400 --threads=8")
mhgp5_gate(mhgp5_par_mutant_drop_shard 4 mhgp5_par_gate "--family=uniform --n=400 --threads=8 --inject=par-drop-shard")
mhgp5_gate(mhgp5_par_mutant_drop_chunk 4 mhgp5_par_gate "--family=uniform --n=400 --threads=8 --inject=par-drop-ball-chunk")
mhgp5_gate(mhgp5_par_mutant_one_worker 4 mhgp5_par_gate "--family=uniform --n=400 --threads=8 --inject=parallel-one-worker")
mhgp5_executable(mhgp5_relabel_gate tests/relabel_gate.cpp)
mhgp5_gate(mhgp5_relabel_gate 0 mhgp5_relabel_gate "--n=300")
mhgp5_gate(mhgp5_relabel_mutant_dense 4 mhgp5_relabel_gate "--n=300 --inject=dense-pointid")

# ---- Rendu § 9.1 -----------------------------------------------------------------
mhgp5_executable(mhgp5_render_gate tests/render_gate.cpp)
mhgp5_gate(mhgp5_render_gate 0 mhgp5_render_gate "")
mhgp5_gate(mhgp5_render_mutant_active 4 mhgp5_render_gate "--inject=render-active-only")
mhgp5_gate(mhgp5_render_mutant_collapse 4 mhgp5_render_gate "--inject=render-collapse-mult")
mhgp5_gate(mhgp5_render_mutant_birth 4 mhgp5_render_gate "--inject=birth-from-events")

# ---- Garde d'arrondi du filtre flottant ------------------------------------------
mhgp5_executable(mhgp5_float_rounding_gate tests/float_rounding_gate.cpp)
mhgp5_gate(mhgp5_float_rounding_gate 0 mhgp5_float_rounding_gate "")

# ---- Kernel affine et intervalles de Jung ----------------------------------------
mhgp5_executable(mhgp5_q3_affine_gate tests/q3_affine_gate.cpp)
mhgp5_gate(mhgp5_q3_affine_gate 0 mhgp5_q3_affine_gate "")
mhgp5_gate(mhgp5_q3_affine_mutant_small 4 mhgp5_q3_affine_gate "--inject=float-small-threshold")
mhgp5_gate(mhgp5_q3_affine_mutant_jung 4 mhgp5_q3_affine_gate "--inject=jung-swap-bounds")

# ---- Oracle : le juge du juge et l'ordre des niveaux -----------------------------
mhgp5_executable(mhgp5_obig_selftest tests/obig_selftest.cpp)
mhgp5_gate(mhgp5_obig_selftest 0 mhgp5_obig_selftest "" LABELS oracle gate)
mhgp5_gate(mhgp5_obig_mutant_carry_lost 4 mhgp5_obig_selftest "--inject=obig-carry-lost")
mhgp5_gate(mhgp5_obig_refus_mutant_inconnu 2 mhgp5_obig_selftest "--inject=inconnu")
mhgp5_executable(mhgp5_level_cmp tests/level_cmp.cpp)
mhgp5_gate(mhgp5_level_cmp_uniform 0 mhgp5_level_cmp
    "--family=uniform --n=300 --seed=3 --min-levels=3000 --min-q4=400 --min-ties=50000 --min-repr-diff=30000" LABELS oracle gate)
mhgp5_gate(mhgp5_level_cmp_eight_clusters 0 mhgp5_level_cmp
    "--family=eight_clusters --n=400 --seed=3 --min-levels=3000 --min-q4=500 --min-ties=50000 --min-repr-diff=30000" LABELS oracle gate)
mhgp5_gate(mhgp5_level_mutant_trunc_hi 4 mhgp5_level_cmp "--family=uniform --n=300 --seed=3 --inject=level-trunc-hi")
mhgp5_gate(mhgp5_level_refus_famille 2 mhgp5_level_cmp "--family=inconnue")
mhgp5_gate(mhgp5_level_refus_mutant_inconnu 2 mhgp5_level_cmp "--n=300 --inject=inconnu")

# ---- Oracle rationnel independant de la lane q3 ----------------------------------
mhgp5_executable(mhgp5_q3_oracle tests/q3_oracle.cpp)
mhgp5_gate(mhgp5_q3_oracle_uniform 0 mhgp5_q3_oracle
    "--family=uniform --n=80 --seed=3 --min-acute=30000 --min-events=5000 --min-shell=900 --min-deep=5000 --min-order-pairs=1000000" LABELS oracle gate)
mhgp5_gate(mhgp5_q3_oracle_eight_clusters 0 mhgp5_q3_oracle
    "--family=eight_clusters --n=100 --seed=3 --min-acute=50000 --min-events=4000 --min-shell=900 --min-deep=5000 --min-order-pairs=1000000" LABELS oracle gate)
mhgp5_gate(mhgp5_q3_oracle_mutant_prune_ge 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=q3-prune-ge")
mhgp5_gate(mhgp5_q3_oracle_mutant_level_4g 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=q3-level-4g")
mhgp5_gate(mhgp5_q3_oracle_mutant_trunc_hi 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=level-trunc-hi")
mhgp5_gate(mhgp5_q3_oracle_mutant_carry_lost 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=obig-carry-lost")
mhgp5_gate(mhgp5_q3_oracle_mutant_sign_p 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=q3-sign-p")
mhgp5_gate(mhgp5_q3_oracle_mutant_cramer_swap 4 mhgp5_q3_oracle "--family=uniform --n=80 --seed=3 --inject=q3-cramer-swap")
mhgp5_gate(mhgp5_q3_oracle_refus_famille 2 mhgp5_q3_oracle "--family=inconnue")
mhgp5_gate(mhgp5_q3_oracle_refus_mutant_inconnu 2 mhgp5_q3_oracle "--n=80 --inject=inconnu")

# ---- Gardes d'API, fixture differentielle q4, oracle q4, source q4, juge de foret -----
mhgp5_executable(mhgp5_api_guard_gate tests/api_guard_gate.cpp)
mhgp5_gate(mhgp5_api_guard_gate 0 mhgp5_api_guard_gate "")
mhgp5_executable(mhgp5_q4_cover_fixture tests/q4_cover_fixture.cpp)
mhgp5_gate(mhgp5_q4_cover_fixture 0 mhgp5_q4_cover_fixture "")
mhgp5_gate(mhgp5_q4_cover_mutant_coef4 4 mhgp5_q4_cover_fixture "--inject=q4-cover-coef4")
mhgp5_gate(mhgp5_wspd_mutant_cap_terminal 4 mhgp5_wspd_gate "--family=two_lines --n=2000 --s=8 --inject=wspd-cap-terminal")
mhgp5_gate(mhgp5_wspd_mutant_split_heaviest 4 mhgp5_wspd_gate "--family=eight_clusters --n=2000 --s=8 --seed=3 --inject=wspd-split-heaviest")
mhgp5_executable(mhgp5_q4_oracle tests/q4_oracle.cpp)
mhgp5_gate(mhgp5_q4_oracle 0 mhgp5_q4_oracle "" LABELS oracle gate)
foreach(m q4-center-parity q4-eq-nonstrict q4-eq-sign q4-i64-drop-factor q4-i64-pair-min)
  mhgp5_gate(mhgp5_q4_oracle_mutant_${m} 4 mhgp5_q4_oracle "--inject=${m}")
endforeach()
mhgp5_executable(mhgp5_q4_source_fixture tests/q4_source_fixture.cpp)
mhgp5_gate(mhgp5_q4_source_fixture_22 0 mhgp5_q4_source_fixture "--fixture=22")
mhgp5_gate(mhgp5_q4_source_fixture_13p8 0 mhgp5_q4_source_fixture "--fixture=13+8")
mhgp5_gate(mhgp5_q4_source_mutant_q3_live 4 mhgp5_q4_source_fixture "--fixture=22 --inject=q4-seeds-from-q3-live")
mhgp5_executable(mhgp5_forest_judge tests/forest_judge.cpp)
mhgp5_gate(mhgp5_forest_judge 0 mhgp5_forest_judge "" LABELS oracle gate)
foreach(m binary-ties repr-ties attach-prebatch drop-nonmerge canonical-is-uf-root rle-drop census-nonstrict genfilter-nonstrict dense-pointid)
  mhgp5_gate(mhgp5_forest_judge_mutant_${m} 4 mhgp5_forest_judge "--inject=${m}")
endforeach()

# ---- Les quatre fixtures manquantes de l'audit 87e915bd ---------------------------
mhgp5_executable(mhgp5_detector_gate tests/detector_gate.cpp)
mhgp5_gate(mhgp5_detector_gate 0 mhgp5_detector_gate "")
mhgp5_gate(mhgp5_detector_mutant_disabled 4 mhgp5_detector_gate "--inject=attach-detector-disabled")
mhgp5_executable(mhgp5_cover_gate tests/cover_gate.cpp)
mhgp5_gate(mhgp5_cover_gate 0 mhgp5_cover_gate "--n=400 --min-anchors=1000")
mhgp5_gate(mhgp5_cover_mutant_dmin 4 mhgp5_cover_gate "--n=400 --inject=cover-rect-dmin")
mhgp5_executable(mhgp5_q4_once_gate tests/q4_once_gate.cpp)
mhgp5_gate(mhgp5_q4_once_gate 0 mhgp5_q4_once_gate "")
mhgp5_gate(mhgp5_q4_once_mutant_no_canonical 4 mhgp5_q4_once_gate "--inject=q4-no-canonical")
mhgp5_executable(mhgp5_q4_seed_core_fixture tests/q4_seed_core_fixture.cpp)
mhgp5_gate(mhgp5_q4_seed_core_fixture 0 mhgp5_q4_seed_core_fixture "")
mhgp5_gate(mhgp5_q4_seed_core_mutant_nonstrict 4 mhgp5_q4_seed_core_fixture "--inject=q4-seed-core-nonstrict")

# ---- Tri stable parallele et banc apparie du fold -----------------------------------
mhgp5_executable(mhgp5_parallel_sort_gate tests/parallel_sort_gate.cpp)
mhgp5_gate(mhgp5_parallel_sort_gate 0 mhgp5_parallel_sort_gate "--bench-pairs=3 --bench-threads=8")
mhgp5_gate(mhgp5_parallel_sort_mutant_unstable 4 mhgp5_parallel_sort_gate "--bench-pairs=0 --inject=parallel-sort-unstable")
mhgp5_gate(mhgp5_parallel_sort_refus_mutant_inconnu 2 mhgp5_parallel_sort_gate "--inject=inconnu")
# Banc apparie contrebalance (instrument, pas une porte de vitesse) : signature identique exigee.
mhgp5_executable(mhgp5_fold_bench tests/fold_bench.cpp)
mhgp5_gate(mhgp5_fold_bench_signature 0 mhgp5_fold_bench "--family=uniform --n=1200 --K=10 --pairs=2 --threads=4")

# ---- SHA-256 : vecteurs FIPS et egalite portable / SHA-NI ---------------------------
mhgp5_executable(mhgp5_sha256_gate tests/sha256_gate.cpp)
mhgp5_gate(mhgp5_sha256_gate 0 mhgp5_sha256_gate "")
