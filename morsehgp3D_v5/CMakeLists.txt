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
# Options GCC bornees au C++ (nvcc recoit les siennes via -Xcompiler dans la
# cible CUDA) — audit 9762daaf.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>")
option(MHGP5_ENABLE_SANITIZERS "ASan + UBSan sur toutes les cibles (incompatible CUDA)" OFF)
option(MHGP5_ENABLE_CUDA "Temoin device (nvcc, sm_120, -fmad=false) — docs/GPU.md" OFF)
if(MHGP5_ENABLE_SANITIZERS)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address,undefined;-fno-omit-frame-pointer>")
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

# ---- Surete du fold concurrent (P1 audit 28 aout 2026) -------------------------
# -fno-elide-constructors : `return rr` copie/deplace la valeur de retour AVANT la
# destruction des locales ; sans jonction explicite un fil B y ecrirait encore
# (course prouvee sous TSan sur l'ancien code). Codes : 0 conforme, 2 refus,
# 3 contrat/plancher d'entrelacement viole, 4 defaut injecte arbitre selon le contrat.
mhgp5_executable(mhgp5_fold_inflight_safety_gate tests/fold_inflight_safety_gate.cpp)
target_compile_options(mhgp5_fold_inflight_safety_gate PRIVATE -fno-elide-constructors)
mhgp5_gate(mhgp5_fold_inflight_safety 0 mhgp5_fold_inflight_safety_gate "--n=300 --threads=4")
mhgp5_gate(mhgp5_fold_inflight_mutant_a_failure_k2 4 mhgp5_fold_inflight_safety_gate "--n=300 --threads=4 --inject=fold-inject-a-failure-k2")
mhgp5_gate(mhgp5_fold_inflight_mutant_b_exception_k3 4 mhgp5_fold_inflight_safety_gate "--n=300 --threads=4 --inject=fold-inject-b-exception-k3")
mhgp5_gate(mhgp5_fold_inflight_refus_mutant_inconnu 2 mhgp5_fold_inflight_safety_gate "--inject=inconnu")
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
mhgp5_executable(mhgp5_q3_skinny_center_gate tests/q3_skinny_center_gate.cpp)
mhgp5_gate(mhgp5_q3_skinny_center 0 mhgp5_q3_skinny_center_gate "")
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
# Pool d'executeurs device borne et persistant (G0 des auditeurs, src/gpu/executor_pool.hpp, hote pur).
mhgp5_executable(mhgp5_executor_pool_gate tests/executor_pool_gate.cpp)
mhgp5_gate(mhgp5_executor_pool 0 mhgp5_executor_pool_gate "" TIMEOUT 180)
mhgp5_gate(mhgp5_executor_pool_mutant_serial 4 mhgp5_executor_pool_gate "--inject=pool-serial" TIMEOUT 180)
mhgp5_gate(mhgp5_executor_pool_mutant_drop_exception 4 mhgp5_executor_pool_gate "--inject=pool-drop-exception" TIMEOUT 180)
# Porte de rejeu « catalogue + deltas -> partition » (docs/ECHELLE.md § 8 bis, T5) : autorite independante du fold.
mhgp5_executable(mhgp5_delta_replay_gate tests/delta_replay_gate.cpp)
mhgp5_gate(mhgp5_delta_replay 0 mhgp5_delta_replay_gate "")
mhgp5_gate(mhgp5_delta_replay_mutant_drop_nonmerge 4 mhgp5_delta_replay_gate "--inject=drop-nonmerge")
mhgp5_gate(mhgp5_delta_replay_mutant_attach_prebatch 4 mhgp5_delta_replay_gate "--inject=attach-prebatch")

# Reducteur vivant (ECHELLE L2, theoreme T6) : egalite avec le fold resident,
# invariant components <= alias <= pic exact, plafond de relocalisations.
# TIMEOUT explicite (audit du 28 aout : une porte non bornee n'est pas une
# porte). Nominal mesure a 51 s ; les mutants meurent sur les cas synthetiques
# adverses joues en tete, en moins d'une seconde.
mhgp5_executable(mhgp5_fold_live_gate tests/fold_live_gate.cpp)
mhgp5_gate(mhgp5_fold_live 0 mhgp5_fold_live_gate "" TIMEOUT 300)
mhgp5_gate(mhgp5_fold_live_mutant_free_on_absorb 4 mhgp5_fold_live_gate "--inject=free-on-absorb" TIMEOUT 60)
mhgp5_gate(mhgp5_fold_live_mutant_root_key_mutable 4 mhgp5_fold_live_gate "--inject=root-key-mutable" TIMEOUT 60)
mhgp5_gate(mhgp5_fold_live_mutant_canon_not_min 4 mhgp5_fold_live_gate "--inject=canon-not-min-on-union" TIMEOUT 60)
mhgp5_gate(mhgp5_fold_live_mutant_last_mark_shifted 4 mhgp5_fold_live_gate "--inject=last-mark-shifted" TIMEOUT 60)
mhgp5_gate(mhgp5_fold_live_mutant_physical_root 4 mhgp5_fold_live_gate "--inject=physical-root-is-logical-root" TIMEOUT 60)
mhgp5_gate(mhgp5_fold_live_mutant_slot_cap 4 mhgp5_fold_live_gate "--inject=slot-cap-minus-one" TIMEOUT 60)

# Six fixtures gravees du fold (ECHELLE § 8 bis item 1) : sortie litterale,
# egalite resident/vivant, invariance par hachage d'adressage constant.
mhgp5_executable(mhgp5_fold_fixtures_gate tests/fold_fixtures_gate.cpp)
mhgp5_gate(mhgp5_fold_fixtures 0 mhgp5_fold_fixtures_gate "" TIMEOUT 120)

# Raffinement post-separation : digests identiques a L=0..3, grand-livre exact,
# route q2 interdite, contre-fixture refine-hist-wakeup.
mhgp5_executable(mhgp5_postsep_refine_gate tests/postsep_refine_gate.cpp)
mhgp5_gate(mhgp5_postsep_refine 0 mhgp5_postsep_refine_gate "" TIMEOUT 600)
mhgp5_gate(mhgp5_postsep_refine_mutant_h1 4 mhgp5_postsep_refine_gate "--inject=postsep-kill-h-minus-one" TIMEOUT 300)
mhgp5_gate(mhgp5_postsep_refine_mutant_drop 4 mhgp5_postsep_refine_gate "--inject=postsep-drop-child" TIMEOUT 300)
# Prefixe exact de la tour (docs/ECHELLE.md § 3.3) : ordres K <= smax − 1 identiques a l'objet complet.
mhgp5_executable(mhgp5_prefix_gate tests/prefix_gate.cpp)
mhgp5_gate(mhgp5_prefix_k5_uniform 0 mhgp5_prefix_gate "--family=uniform --n=1200 --smax=6 --min-events=1000")
mhgp5_gate(mhgp5_prefix_k5_clusters 0 mhgp5_prefix_gate "--family=eight_clusters --n=1200 --smax=6 --min-events=1000")
mhgp5_gate(mhgp5_prefix_k5_scanline 0 mhgp5_prefix_gate "--family=scanline_single_pass --n=1500 --smax=6 --min-events=500")
mhgp5_gate(mhgp5_prefix_k4_uniform 0 mhgp5_prefix_gate "--family=uniform --n=1200 --smax=5 --min-events=1000")
mhgp5_gate(mhgp5_prefix_mutant_h1 4 mhgp5_prefix_gate "--family=uniform --n=1200 --smax=6 --min-events=1000 --inject=anchor-kill-h-minus-one")
mhgp5_gate(mhgp5_prefix_k5_ex_aequo 0 mhgp5_prefix_gate "--family=uniform --n=48 --coord=14 --smax=6 --min-events=100 --min-tie-excess=20")
mhgp5_gate(mhgp5_prefix_mutant_event_order 4 mhgp5_prefix_gate "--family=uniform --n=1200 --smax=6 --min-events=1000 --inject=prefix-tamper-event-order")
mhgp5_gate(mhgp5_prefix_mutant_batch_levels 4 mhgp5_prefix_gate "--family=uniform --n=1200 --smax=6 --min-events=1000 --inject=prefix-tamper-batch-levels")
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
mhgp5_executable(mhgp5_plateau_shell_relevance_gate tests/plateau_shell_relevance_gate.cpp)
mhgp5_gate(mhgp5_plateau_shell_relevance 0 mhgp5_plateau_shell_relevance_gate "")

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

# ---- Arithmetique 128 bits portable et formes device (preparation du port GPU) ----
mhgp5_executable(mhgp5_dint_gate tests/dint_gate.cpp)
mhgp5_gate(mhgp5_dint_gate 0 mhgp5_dint_gate
    "--cases=200000 --min-cases=200000 --min-triangles=60000 --min-tetra=150000 --min-inside=12000 --min-spindle=380000")
mhgp5_gate(mhgp5_dint_mutant_mulhi_dropped 4 mhgp5_dint_gate "--cases=20000 --inject=dint-mulhi-dropped")
mhgp5_gate(mhgp5_dint_refus_mutant_inconnu 2 mhgp5_dint_gate "--inject=inconnu")
mhgp5_gate(mhgp5_dint_refus_argument 2 mhgp5_dint_gate "--argument-inconnu")

# ---- Scan q3 en forme de kernel (DI128) ≡ lane q3 de production ---------------------
mhgp5_executable(mhgp5_q3_scan_shaped_gate tests/q3_scan_shaped_gate.cpp)
mhgp5_gate(mhgp5_q3_scan_shaped_uniform 0 mhgp5_q3_scan_shaped_gate "--family=uniform --n=400 --min-seeds=1000 --min-fallback=10")
mhgp5_gate(mhgp5_q3_scan_shaped_clusters 0 mhgp5_q3_scan_shaped_gate "--family=eight_clusters --n=400 --min-seeds=1000 --min-fallback=10")
mhgp5_gate(mhgp5_q3_scan_shaped_cocirc 0 mhgp5_q3_scan_shaped_gate "--family=uniform --n=300 --coord=40 --min-seeds=500 --min-fallback=100")
mhgp5_gate(mhgp5_q3_scan_shaped_mutant_flip 4 mhgp5_q3_scan_shaped_gate "--family=uniform --n=300 --coord=40 --inject=q3-shaped-strict-flip")

# Cœur de seed q4 en forme de kernel (livraison 4b) : egalite par seed avec la production.
mhgp5_executable(mhgp5_q4_core_shaped_gate tests/q4_core_shaped_gate.cpp)
# En position generale les replis exacts sont rares (2 et 0 a n=400) : leurs
# planchers sont portes par la famille cocirculaire (coord=40 : 26 replis de
# Jung, 163 replis flottants) ; les familles generales gardent les planchers
# de seeds, de non-temoins certifies et de replis flottants.
mhgp5_gate(mhgp5_q4_core_shaped_uniform 0 mhgp5_q4_core_shaped_gate "--family=uniform --n=400 --min-jung-fallback=0 --min-float-fallback=10")
mhgp5_gate(mhgp5_q4_core_shaped_clusters 0 mhgp5_q4_core_shaped_gate "--family=eight_clusters --n=400 --min-jung-fallback=0 --min-float-fallback=5")
mhgp5_gate(mhgp5_q4_core_shaped_cocirc 0 mhgp5_q4_core_shaped_gate "--family=uniform --n=300 --coord=40 --min-seeds=500 --min-jung-fallback=20 --min-float-fallback=100")
mhgp5_gate(mhgp5_q4_core_shaped_mutant_skip 4 mhgp5_q4_core_shaped_gate "--family=uniform --n=300 --coord=40 --min-seeds=500 --inject=q4-shaped-jung-skip-kills")

# Completions q4 en forme de kernel (livraison 4c) : etage atteint et forme de Cramer identiques.
mhgp5_executable(mhgp5_q4_completion_shaped_gate tests/q4_completion_shaped_gate.cpp)
mhgp5_gate(mhgp5_q4_completion_shaped_uniform 0 mhgp5_q4_completion_shaped_gate "--family=uniform --n=400")
mhgp5_gate(mhgp5_q4_completion_shaped_clusters 0 mhgp5_q4_completion_shaped_gate "--family=eight_clusters --n=400")
mhgp5_gate(mhgp5_q4_completion_shaped_cocirc 0 mhgp5_q4_completion_shaped_gate "--family=uniform --n=300 --coord=40 --min-emit=200 --min-deep=20 --min-center=20")
mhgp5_gate(mhgp5_q4_completion_shaped_mutant_once 4 mhgp5_q4_completion_shaped_gate "--family=uniform --n=400 --inject=q4-shaped-once-flip")

# Contrat structurel des lots q3/q4 et contrat d'exception des primitives paralleles (P1 audit d3144fb3).
mhgp5_executable(mhgp5_batch_contract_gate tests/batch_contract_gate.cpp)
mhgp5_gate(mhgp5_batch_contract 0 mhgp5_batch_contract_gate "")
mhgp5_executable(mhgp5_parallel_exception_gate tests/parallel_exception_gate.cpp)
mhgp5_gate(mhgp5_parallel_exception 0 mhgp5_parallel_exception_gate "")

# ---- Instrument device recevable (audit 20260828 ; partie hote, sans nvcc) ------
mhgp5_executable(mhgp5_gpu_instrument_gate tests/gpu_instrument_gate.cpp)
mhgp5_gate(mhgp5_gpu_instrument 0 mhgp5_gpu_instrument_gate "--n=400 --threads=4 --gauge-threads=8 --min-flushes=100")
mhgp5_gate(mhgp5_gpu_instrument_mutant_class 4 mhgp5_gpu_instrument_gate "--n=400 --threads=4 --gauge-threads=8 --min-flushes=100 --inject=log2hist-class-shift")
mhgp5_gate(mhgp5_gpu_instrument_mutant_gauge 4 mhgp5_gpu_instrument_gate "--n=400 --threads=4 --gauge-threads=8 --min-flushes=100 --inject=gauge-no-peak")
mhgp5_gate(mhgp5_gpu_instrument_plancher 3 mhgp5_gpu_instrument_gate "--n=400 --min-flushes=1000000")
mhgp5_gate(mhgp5_gpu_instrument_refus 2 mhgp5_gpu_instrument_gate "--gauge-threads=1")

# Lane q3 par lots (etage hote de la lane device) : egalite vecteur a vecteur avec la production.
# Sonde de mesure (jamais un claim) pour la lane par rectangle : tailles aplaties par rectangle, quantiles.
mhgp5_executable(mhgp5_q4_chord_probe bench/q4_chord_probe.cpp)
mhgp5_executable(mhgp5_q4_stage_probe bench/q4_stage_probe.cpp)
target_compile_definitions(mhgp5_q4_stage_probe PRIVATE MHGP5_PROFILE_Q4=1)
mhgp5_executable(mhgp5_rect_probe bench/rect_probe.cpp)
# Sonde MIROIR resident/vivant (audit du 28 aout) : meme perimetre des deux
# cotes, deux PROCESSUS separes pour que le pic de RSS soit attribuable.
mhgp5_executable(mhgp5_fold_live_probe bench/fold_live_probe.cpp)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE _mhgp5_probe_pin OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git status --porcelain -- src bench tests CMakeLists.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE _mhgp5_probe_dirty OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(_mhgp5_probe_dirty STREQUAL "")
  set(_mhgp5_probe_dirty "non")
else()
  set(_mhgp5_probe_dirty "OUI")
endif()
target_compile_definitions(mhgp5_rect_probe PRIVATE MHGP5_PROBE_PIN="${_mhgp5_probe_pin}" MHGP5_PROBE_DIRTY="${_mhgp5_probe_dirty}")

# Test d'ancre par secteurs : un mutant non strict cree des faux positifs -> divergence des digests v4 (code 4).
# Le mutant non strict est INVISIBLE en position generale (aucune egalite) : la fixture gravee
# (sites exactement sur la sphere diametrale, seed survivant) le tue ; les conformites v4 prouvent l'objet.
mhgp5_executable(mhgp5_sector_kill_fixture tests/sector_kill_fixture.cpp)
mhgp5_gate(mhgp5_sector_kill_fixture 0 mhgp5_sector_kill_fixture "")
mhgp5_gate(mhgp5_sector_kill_mutant_nonstrict 4 mhgp5_sector_kill_fixture "--inject=sector-kill-nonstrict")
mhgp5_executable(mhgp5_anchor_tests_oracle tests/anchor_tests_oracle.cpp)
mhgp5_gate(mhgp5_anchor_tests_oracle 0 mhgp5_anchor_tests_oracle "" LABELS oracle)
mhgp5_gate(mhgp5_anchor_tests_oracle_mutant_cell_h1 4 mhgp5_anchor_tests_oracle "--inject=cell-kill-h-minus-one" LABELS oracle)
mhgp5_executable(mhgp5_anchor_kill_fixture tests/anchor_kill_fixture.cpp)
mhgp5_gate(mhgp5_anchor_kill_fixture 0 mhgp5_anchor_kill_fixture "")
mhgp5_gate(mhgp5_anchor_kill_mutant_h1 4 mhgp5_anchor_kill_fixture "--inject=anchor-kill-h-minus-one")
mhgp5_gate(mhgp5_anchor_kill_mutant_nonstrict 4 mhgp5_anchor_kill_fixture "--inject=sector-kill-nonstrict")
mhgp5_gate(mhgp5_anchor_kill_mutant_chord 4 mhgp5_anchor_kill_fixture "--inject=chord-nonstrict")
mhgp5_gate(mhgp5_anchor_kill_mutant_cell_nonstrict 4 mhgp5_anchor_kill_fixture "--inject=cell-kill-nonstrict")
# Fixture F11 (frontieres de la localisation) : le mutant a marge nulle consulte une seule colonne.
mhgp5_gate(mhgp5_anchor_kill_mutant_cell_eps0 4 mhgp5_anchor_kill_fixture "--inject=cell-locate-eps-zero")
# Oracle brut de la grille de cellules (reception du theoreme 10.5, 28 aout 2026) : compteur deux pointeurs
# contre evaluation directe i128 des 289 sommets (toutes les paires de trois petits nuages, F9/F10/F11, ancre u16
# extreme, synthetiques i128), localisateur binaire64 contre le rationnel exact 256 bits ; garde d'environnement.
# Label `oracle gate` : 4,5 s, doit etre rejoue par la commande G4 `-L gate` (remarque des auditeurs).
mhgp5_executable(mhgp5_cell_grid_oracle tests/cell_grid_oracle.cpp)
mhgp5_gate(mhgp5_cell_grid_oracle 0 mhgp5_cell_grid_oracle "" LABELS oracle gate)
mhgp5_gate(mhgp5_cell_grid_oracle_mutant_nonstrict 4 mhgp5_cell_grid_oracle "--inject=cell-kill-nonstrict" LABELS oracle gate)
mhgp5_gate(mhgp5_cell_grid_oracle_mutant_h1 4 mhgp5_cell_grid_oracle "--inject=cell-kill-h-minus-one" LABELS oracle gate)
mhgp5_gate(mhgp5_cell_grid_oracle_mutant_eps0 4 mhgp5_cell_grid_oracle "--inject=cell-locate-eps-zero" LABELS oracle gate)

# Contrat structurel du fold (P0 audit 9762daaf) : rejets permanents et limites positives.
mhgp5_executable(mhgp5_fold_contract_gate tests/fold_contract_gate.cpp)
mhgp5_gate(mhgp5_fold_contract 0 mhgp5_fold_contract_gate "")

mhgp5_executable(mhgp5_q3_lane_batched_gate tests/q3_lane_batched_gate.cpp)
mhgp5_gate(mhgp5_q3_lane_batched_uniform 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=1200 --threads=3")
mhgp5_gate(mhgp5_q3_lane_batched_clusters 0 mhgp5_q3_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3")
mhgp5_gate(mhgp5_q3_lane_batched_terrain 0 mhgp5_q3_lane_batched_gate "--family=terrain --n=1200 --threads=2")
mhgp5_gate(mhgp5_q3_lane_batched_cocirc 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=300 --coord=40 --min-candidates=200 --min-fallback=100")
mhgp5_gate(mhgp5_q3_lane_batched_mutant_dead 4 mhgp5_q3_lane_batched_gate "--family=uniform --n=400 --min-candidates=200 --inject=q3-batched-emit-dead")
# Lane q4 par lots (etage hote de la lane q4 device) : egalite post-RLE et dix-neuf compteurs.
mhgp5_executable(mhgp5_q4_lane_batched_gate tests/q4_lane_batched_gate.cpp)
mhgp5_gate(mhgp5_q4_lane_batched_uniform 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=1200 --threads=3")
mhgp5_gate(mhgp5_q4_lane_batched_clusters 0 mhgp5_q4_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3")
mhgp5_gate(mhgp5_q4_lane_batched_terrain 0 mhgp5_q4_lane_batched_gate "--family=terrain --n=1200 --threads=2 --min-candidates=200")
mhgp5_gate(mhgp5_q4_lane_batched_cocirc 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20")
mhgp5_gate(mhgp5_q4_lane_batched_ordre 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --threads=1 --min-candidates=200")
mhgp5_gate(mhgp5_q4_lane_batched_mutant_deep 4 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --min-candidates=200 --inject=q4-batched-emit-deep")
mhgp5_gate(mhgp5_q4_lane_batched_uniform_8000 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=8000 --threads=8 --min-candidates=100000" LABELS scale8000 TIMEOUT 1800)

mhgp5_gate(mhgp5_q3_lane_batched_petit_lot 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=400 --threads=1 --seeds-per-launch=1000 --min-candidates=200 --min-flushes=100")
mhgp5_gate(mhgp5_q4_lane_batched_petit_lot 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --threads=1 --seeds-per-launch=1000 --min-candidates=200 --min-flushes=100")
mhgp5_gate(mhgp5_q3_lane_batched_seuil_zero 2 mhgp5_q3_lane_batched_gate "--family=uniform --n=400 --seeds-per-launch=0")
mhgp5_gate(mhgp5_q4_lane_batched_seuil_negatif 2 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --seeds-per-launch=-1")
# Routage adaptatif (executeur externe = hote shaped ici) : l'objet post-RLE ne depend pas de la politique ;
# NON-VACUITE contractuelle des routes (--expect-route) et mutant route-ignore-threshold tue en code 4.
# (uniform n=1200 : apres les tests d'ancre W_3 + secteurs, plus aucune ancre survivante n'a un cover >= 256 — les
#  covers denses sont exactement les ancres mortes ; la porte mixte q3 se mesure sur eight_clusters, comme q4.)
mhgp5_gate(mhgp5_q3_lane_batched_route_256 0 mhgp5_q3_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3 --device-min-sites=256 --expect-route=mixed")
mhgp5_gate(mhgp5_q4_lane_batched_route_256 0 mhgp5_q4_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3 --device-min-sites=256 --expect-route=mixed")
mhgp5_gate(mhgp5_q3_lane_batched_route_tout_hote 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=400 --threads=2 --device-min-sites=1000000000 --min-candidates=200 --min-flushes=0 --expect-route=host")
mhgp5_gate(mhgp5_q4_lane_batched_route_tout_hote 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --threads=2 --device-min-sites=1000000000 --min-candidates=200 --min-flushes=0 --expect-route=host")
mhgp5_gate(mhgp5_q3_lane_batched_route_mutant 4 mhgp5_q3_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3 --device-min-sites=256 --expect-route=mixed --inject=route-ignore-threshold")
mhgp5_gate(mhgp5_q4_lane_batched_route_mutant 4 mhgp5_q4_lane_batched_gate "--family=eight_clusters --n=1200 --threads=3 --device-min-sites=256 --expect-route=mixed --inject=route-ignore-threshold")
# Ancre trop grande pour un lot (seuil de seeds minuscule) : corps hote, egalite conservee, anchors_oversized > 0.
mhgp5_gate(mhgp5_q3_lane_batched_ancre_trop_grande 0 mhgp5_q3_lane_batched_gate "--family=eight_clusters --n=400 --threads=2 --seeds-per-launch=50 --min-candidates=200 --expect-route=device")
mhgp5_gate(mhgp5_q4_lane_batched_ancre_trop_grande 0 mhgp5_q4_lane_batched_gate "--family=eight_clusters --n=400 --threads=2 --seeds-per-launch=50 --min-candidates=200 --expect-route=device")
mhgp5_gate(mhgp5_q4_lane_batched_petit_lot_paires 0 mhgp5_q4_lane_batched_gate "--family=eight_clusters --n=400 --threads=2 --pairs-per-launch=200000 --min-candidates=200 --min-flushes=20")
mhgp5_gate(mhgp5_q4_lane_batched_petit_lot_sites 0 mhgp5_q4_lane_batched_gate "--family=uniform --n=400 --threads=2 --sites-per-launch=20000 --min-candidates=200 --min-flushes=50")
mhgp5_gate(mhgp5_q3_lane_batched_petit_lot_sites 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=400 --threads=2 --sites-per-launch=20000 --min-candidates=200 --min-flushes=50")
mhgp5_gate(mhgp5_q3_lane_batched_uniform_8000 0 mhgp5_q3_lane_batched_gate "--family=uniform --n=8000 --threads=8 --min-candidates=100000" LABELS scale8000 TIMEOUT 1800)

# ---- Temoin device (docs/GPU.md, livraison 3) — nvcc seulement -------------------------
if(MHGP5_ENABLE_CUDA)
  if(MHGP5_ENABLE_SANITIZERS)
    message(FATAL_ERROR "MHGP5_ENABLE_CUDA et MHGP5_ENABLE_SANITIZERS sont incompatibles")
  endif()
  # sm_120 (RTX PRO 6000 Blackwell) : a fixer AVANT enable_language, sinon
  # CMake initialise l'architecture par defaut (sm_52, session G4 50fee05c).
  if(NOT CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120)
  endif()
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  add_executable(mhgp5_device_witness src/gpu/device_witness.cu)
  # -fmad=false : aucune contraction hors des fma EXPLICITES de la sequence figee du filtre.
  target_compile_options(mhgp5_device_witness PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp5_device_witness PRIVATE MHGP5_TESTING=1)
  target_link_libraries(mhgp5_device_witness PRIVATE Threads::Threads)
  mhgp5_gate(mhgp5_device_witness 0 mhgp5_device_witness "" LABELS gpu)
  mhgp5_gate(mhgp5_device_witness_mutant_warp 4 mhgp5_device_witness "--inject=witness-no-warp-correction" LABELS gpu)
  # Lane q3 device (executeur k_scan) contre la production : egalite post-RLE.
  add_executable(mhgp5_q3_lane_device_gate tests/q3_lane_device_gate.cu)
  target_compile_options(mhgp5_q3_lane_device_gate PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp5_q3_lane_device_gate PRIVATE MHGP5_TESTING=1)
  target_link_libraries(mhgp5_q3_lane_device_gate PRIVATE Threads::Threads)
  mhgp5_gate(mhgp5_q3_lane_device_uniform 0 mhgp5_q3_lane_device_gate "--family=uniform --n=1200" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_clusters 0 mhgp5_q3_lane_device_gate "--family=eight_clusters --n=1200 --threads=4" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_cocirc 0 mhgp5_q3_lane_device_gate "--family=uniform --n=300 --coord=40 --min-candidates=200" LABELS gpu)
  # G1 : wire par INDICES u32 + geometrie residente — memes verdicts, vecteur et compteurs que la production (et que le wire SoA).
  mhgp5_gate(mhgp5_q3_lane_device_uniform_wire_index 0 mhgp5_q3_lane_device_gate "--family=uniform --n=1200 --wire=index" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_clusters_wire_index 0 mhgp5_q3_lane_device_gate "--family=eight_clusters --n=1200 --threads=4 --wire=index" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_cocirc_wire_index 0 mhgp5_q3_lane_device_gate "--family=uniform --n=300 --coord=40 --min-candidates=200 --wire=index" LABELS gpu)
  add_executable(mhgp5_q4_lane_device_gate tests/q4_lane_device_gate.cu)
  target_compile_options(mhgp5_q4_lane_device_gate PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp5_q4_lane_device_gate PRIVATE MHGP5_TESTING=1)
  target_link_libraries(mhgp5_q4_lane_device_gate PRIVATE Threads::Threads)
  mhgp5_gate(mhgp5_q4_lane_device_uniform 0 mhgp5_q4_lane_device_gate "--family=uniform --n=1200" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_clusters 0 mhgp5_q4_lane_device_gate "--family=eight_clusters --n=1200 --threads=4" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_cocirc 0 mhgp5_q4_lane_device_gate "--family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_petit_lot 0 mhgp5_q4_lane_device_gate "--family=uniform --n=400 --seeds-per-launch=1000 --min-candidates=200 --min-flushes=100" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_uniform_wire_index 0 mhgp5_q4_lane_device_gate "--family=uniform --n=1200 --wire=index" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_clusters_wire_index 0 mhgp5_q4_lane_device_gate "--family=eight_clusters --n=1200 --threads=4 --wire=index" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_cocirc_wire_index 0 mhgp5_q4_lane_device_gate "--family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20 --wire=index" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_mutant_deep 4 mhgp5_q4_lane_device_gate "--family=uniform --n=400 --min-candidates=200 --inject=q4-batched-emit-deep" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_uniform_8000 0 mhgp5_q4_lane_device_gate "--family=uniform --n=8000 --threads=8 --min-candidates=100000" LABELS gpu scale8000 TIMEOUT 1800)
  # Pilote CUDA (produit : sans mutants) : --gpu remplace les lanes q3/q4 par le device.
  add_executable(mhgp5_cuda cli/mhgp5_cuda.cu)
  target_compile_options(mhgp5_cuda PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_link_libraries(mhgp5_cuda PRIVATE Threads::Threads)
  mhgp5_gate(mhgp5_q4_lane_device_route_256 0 mhgp5_q4_lane_device_gate "--family=eight_clusters --n=1200 --threads=4 --device-min-sites=256 --expect-route=mixed" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_route_256 0 mhgp5_q3_lane_device_gate "--family=eight_clusters --n=1200 --threads=4 --device-min-sites=256 --expect-route=mixed" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_route_mutant 4 mhgp5_q3_lane_device_gate "--family=eight_clusters --n=1200 --threads=4 --device-min-sites=256 --expect-route=mixed --inject=route-ignore-threshold" LABELS gpu)
  # PREUVE DE BRANCHE du wire G1 : le mutant retombe silencieusement sur SoA
  # (verdicts et digests verts) ; seuls les compteurs de branche le voient.
  mhgp5_gate(mhgp5_q3_lane_device_mutant_wire_force_soa 4 mhgp5_q3_lane_device_gate "--family=uniform --n=1200 --wire=index --inject=wire-index-force-soa" LABELS gpu)
  mhgp5_gate(mhgp5_q4_lane_device_mutant_wire_force_soa 4 mhgp5_q4_lane_device_gate "--family=uniform --n=1200 --wire=index --inject=wire-index-force-soa" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_petit_lot 0 mhgp5_q3_lane_device_gate "--family=uniform --n=400 --seeds-per-launch=1000 --min-candidates=200 --min-flushes=100" LABELS gpu)
  mhgp5_gate(mhgp5_q3_lane_device_uniform_8000 0 mhgp5_q3_lane_device_gate "--family=uniform --n=8000 --threads=8 --min-candidates=100000" LABELS gpu scale8000 TIMEOUT 1800)
endif()
