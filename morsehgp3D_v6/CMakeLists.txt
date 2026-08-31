# MorseHGP3D v6 — chantier actif. C++20 sans extensions, -Werror.
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v6 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

find_package(Threads REQUIRED)
enable_testing()

# Cible PRODUIT : aucun mutant compile (pas de MHGP6_TESTING).
function(mhgp6_product_executable name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

# Cibles de TEST : les points d'injection de mutants sont compiles.
function(mhgp6_executable name)
  add_executable(${name} ${ARGN})
  target_compile_definitions(${name} PRIVATE MHGP6_TESTING=1)
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

# Porte a code de sortie exact : 0 conforme, 1 juge, 2 refus, 3 plancher ou
# invariant, 4 mutant tue. Labels optionnels apres les arguments.
function(mhgp6_gate name expected target)
  cmake_parse_arguments(G "" "TIMEOUT" "LABELS;ARGS" ${ARGN})
  string(JOIN " " joined ${G_ARGS})
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -DCMD=$<TARGET_FILE:${target}> "-DARGS=${joined}"
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

# ---- Pilote produit.
mhgp6_product_executable(mhgp6 cli/mhgp6.cpp)

# ---- Selftests du cœur (etablis en premier ; chaque porte a son code exact).
mhgp6_executable(mhgp6_selftest tests/selftest.cpp)
mhgp6_gate(mhgp6_arith_selftest 0 mhgp6_selftest ARGS --arith)
mhgp6_gate(mhgp6_sha256_selftest 0 mhgp6_selftest ARGS --sha256)
mhgp6_gate(mhgp6_families_fixture 0 mhgp6_selftest ARGS --families)
# Mutant de cardinalite des scanlines : tue par la divergence du digest grave
# de scanline_overlap_multiecho (n=2000, graine 3). Sur scanline_single_pass
# il est structurellement invisible (multi_echo=false : la garde de push est
# redondante avec la borne de boucle) — voir tests/selftest.cpp.
mhgp6_gate(mhgp6_families_mutant_overshoot 4 mhgp6_selftest ARGS --families --inject=family-scanline-overshoot)
mhgp6_gate(mhgp6_tree_selftest 0 mhgp6_selftest ARGS --tree)
mhgp6_gate(mhgp6_wspd_ledger 0 mhgp6_selftest ARGS --wspd-ledger)
# Requalification de la descente fusionnee (cinquieme cycle) : ownership par
# paire (exactement une fois), litteral n-1 par position, separation de
# chaque rectangle, grand-livre sans masse tuee a h infini, fixtures gravees
# du nombre de rectangles ; mutants cap/split retargetes sur la route fusionnee.
mhgp6_gate(mhgp6_wspd_ownership 0 mhgp6_selftest ARGS --wspd-ownership)
mhgp6_gate(mhgp6_wspd_mutant_cap 4 mhgp6_selftest ARGS --wspd-ownership --inject=wspd-cap-terminal)
mhgp6_gate(mhgp6_wspd_mutant_split 4 mhgp6_selftest ARGS --wspd-ownership --inject=wspd-split-heaviest)
# Fixture bibliotheque des contrats d'echec (P2 cinquieme cycle) : entree
# invalide + run complet a callbacks (sequence K stricte) ; sous mutant,
# chaque echec (census, fold K2 x inflight {1,2,8}) invalide TOUT champ
# provisoire et les callbacks restent provisoires (authority=status_terminal).
mhgp6_gate(mhgp6_contrat_echec 0 mhgp6_selftest ARGS --failure-contract TIMEOUT 600)
mhgp6_gate(mhgp6_contrat_echec_census 4 mhgp6_selftest ARGS --failure-contract --inject=census-nonstrict TIMEOUT 600)
mhgp6_gate(mhgp6_contrat_echec_fold_k2 4 mhgp6_selftest ARGS --failure-contract --inject=fold-inject-a-failure-k2 TIMEOUT 600)
# Etage E6 (--e6-grille) : objet et multiensemble emis BIT-IDENTIQUES sous la
# grille raffinee, W_sweep1 jamais augmente, plancher de grilles construites.
mhgp6_gate(mhgp6_e6_grille_objet 0 mhgp6_selftest ARGS --e6-equal TIMEOUT 600)
mhgp6_gate(mhgp6_fused_descent 0 mhgp6_selftest ARGS --fused-descent)
mhgp6_gate(mhgp6_sweep_oracle 0 mhgp6_selftest ARGS --sweep-oracle LABELS "gate;oracle")
mhgp6_gate(mhgp6_sweep_mutant_depth 4 mhgp6_selftest ARGS --sweep-oracle --inject=sweep-nonstrict-depth)
mhgp6_gate(mhgp6_fused_mutant_mask 4 mhgp6_selftest ARGS --fused-descent --inject=fused-mask-stuck)

# ---- Conformite differentielle v5 ≡ v6 (digests graves au pin 3bad233d).
set(REFS ${CMAKE_CURRENT_SOURCE_DIR}/receipts/conformite_v5)
mhgp6_executable(mhgp6_conformity tests/conformity_v5.cpp)
foreach(spec "uniform;400" "terrain;400" "eight_clusters;400" "scanline_single_pass;400"
             "collinear_seven;600" "two_lines;200" "uniform;2000" "terrain;2000")
  list(GET spec 0 fam)
  list(GET spec 1 sz)
  mhgp6_gate(mhgp6_conformity_${fam}_${sz} 0 mhgp6_conformity
             ARGS --family=${fam} --n=${sz} --threads=4 --expected=${REFS}/${fam}_${sz}.txt TIMEOUT 600)
endforeach()
# Mutants du sweep tues par la divergence de conformite (digest_balls, forets).
mhgp6_gate(mhgp6_sweep_mutant_exit 4 mhgp6_conformity
           ARGS --family=terrain --n=400 --inject=sweep-drop-exit-root --expected=${REFS}/terrain_400.txt)
# rle-drop laisse l'objet intact (union-find idempotent) : son detecteur est
# la monnaie post-prefiltre (les doublons de survivants changent le digest).
mhgp6_gate(mhgp6_conformity_mutant_rle 4 mhgp6_conformity
           ARGS --family=uniform --n=400 --inject=rle-drop --expected=${REFS}/uniform_400.txt
                --postprefilter=97be65b6e0c66e3b3b2262510bd7274f8e557ae4bdb78024467c1f9ee05c4d72)

# ---- Refus du CLI (parsing exact, profil).
mhgp6_gate(mhgp6_cli_refus_s7 2 mhgp6 ARGS --family=uniform --n=200 --s=7)
mhgp6_gate(mhgp6_cli_refus_s_suffix 2 mhgp6 ARGS --family=uniform --n=200 --s=8junk)
mhgp6_gate(mhgp6_cli_refus_n_suffix 2 mhgp6 ARGS --family=uniform --n=200x)
mhgp6_gate(mhgp6_cli_ok_s8 0 mhgp6 ARGS --family=uniform --n=200 --s=8 TIMEOUT 120)

# ---- Tailles d'interet (labels scale*) : conformite aux digests v5 graves.
foreach(fam uniform terrain eight_clusters scanline_single_pass scanline_overlap_multiecho)
  foreach(sz 8000 16000 32000)
    mhgp6_gate(mhgp6_conformity_${fam}_${sz} 0 mhgp6_conformity
               ARGS --family=${fam} --n=${sz} --threads=8
                    --expected=${CMAKE_CURRENT_SOURCE_DIR}/receipts/conformite_v5/${fam}_${sz}.txt
               LABELS scale${sz} TIMEOUT 3600)
  endforeach()
endforeach()

# ---- Barriere de sortie : linked_arcs_u16 (contrat sortie-sensible, barriere
# d'Edelsbrunner–Pach transposee en entiers u16 : arcs enlaces, q3 = 2n(n+1) et
# q4 = n² cles de profondeur zero ; oracle local OBig384 + route produit).
mhgp6_executable(mhgp6_linked_arcs tests/linked_arcs_gate.cpp)
mhgp6_gate(mhgp6_linked_arcs_u16 0 mhgp6_linked_arcs LABELS "gate;oracle")
mhgp6_gate(mhgp6_linked_arcs_oracle_i64 4 mhgp6_linked_arcs ARGS --oracle-i64)

# ---- Oracle BRUT de la grille de cellules (porte de la v5, quatrieme tour
# gcp/etat : parametre EXPLICITEMENT sur G=8 ET G=16 — evaluation directe
# i128 des (2G+1)^2 sommets, localisateur rationnel 256 bits, extremes u16,
# fixtures F9/F10/F11, sites synthetiques ~2^100 ; trois mutants tues).
mhgp6_executable(mhgp6_cell_grid_oracle tests/cell_grid_oracle.cpp)
mhgp6_gate(mhgp6_cell_grid_oracle 0 mhgp6_cell_grid_oracle LABELS "gate;oracle" TIMEOUT 900)
mhgp6_gate(mhgp6_cell_grid_oracle_mutant_ns 4 mhgp6_cell_grid_oracle ARGS --inject=cell-kill-nonstrict TIMEOUT 900)
mhgp6_gate(mhgp6_cell_grid_oracle_mutant_eps 4 mhgp6_cell_grid_oracle ARGS --inject=cell-locate-eps-zero TIMEOUT 900)
mhgp6_gate(mhgp6_cell_grid_oracle_mutant_h1 4 mhgp6_cell_grid_oracle ARGS --inject=cell-kill-h-minus-one TIMEOUT 900)

# ---- Fixtures de frontiere du sweep (audit du 31 aout : signes de B, B = 0,
# racines egales, extremite de Jung — egalite exacte 2P² = J·B² par le
# tetraedre regulier entier —, completion dans le facteur du rectangle,
# profondeur gravee h4 − 1 ; verite par objet contre oracle exhaustif).
mhgp6_executable(mhgp6_sweep_fixtures tests/sweep_fixtures.cpp)
mhgp6_gate(mhgp6_sweep_frontieres 0 mhgp6_sweep_fixtures)
mhgp6_gate(mhgp6_sweep_frontieres_mutant_depth 4 mhgp6_sweep_fixtures ARGS --inject=sweep-nonstrict-depth)
mhgp6_gate(mhgp6_sweep_frontieres_mutant_exit 4 mhgp6_sweep_fixtures ARGS --inject=sweep-drop-exit-root)

# ---- Coefficient de cover q4 (P0 audit du 31 aout : contre-fixture au
# tetraedre regulier + z interieur hors cover-3 ; le coefficient 4 tue a la
# generation, le mutant q4-cover-coef3 retablit le defaut herite v5).
mhgp6_executable(mhgp6_cover_fixture tests/cover_fixture.cpp)
mhgp6_gate(mhgp6_cover_coef4 0 mhgp6_cover_fixture)
mhgp6_gate(mhgp6_cover_coef3_mutant 4 mhgp6_cover_fixture ARGS --inject=q4-cover-coef3)

# ---- Golden post-prefiltre (non-regression interne v6, tag
# mhgp6-digest-v1:postprefilter-candidates) : grave apres le correctif cover.
mhgp6_gate(mhgp6_postprefilter_uniform_400 0 mhgp6_conformity
           ARGS --family=uniform --n=400 --threads=4 --expected=${REFS}/uniform_400.txt
                --postprefilter=97be65b6e0c66e3b3b2262510bd7274f8e557ae4bdb78024467c1f9ee05c4d72
                --expect-compat=2b50f878e2ddb641f07358dc82c9d52e9db7482ae55a94d0e033b13c71f1e6f4
                --expect-counts=105076/1134/103942)

# ---- Couverture executee des mutants (audit du 31 aout : 20 mutants deja
# tues par la divergence d'objet de la conformite sur eight_clusters n=400 ;
# fold-inject-b-exception-k3 exclu — termine par signal, juge dedie a porter).
foreach(mut wspd-drop-rect census-nonstrict genfilter-nonstrict depth-threshold-minus-one
            range-add-max-le-zero skip-full-census binary-ties repr-ties
            attach-prebatch drop-nonmerge dense-pointid canonical-is-uf-root
            q3-level-4g q4-center-parity q4-eq-sign q4-i64-drop-factor
            q4-i64-pair-min witness-no-lane-mask par-drop-shard
            par-drop-ball-chunk fold-inject-a-failure-k2)
  mhgp6_gate(mhgp6_mutant_${mut} 4 mhgp6_conformity
             ARGS --family=eight_clusters --n=400 --threads=4 --inject=${mut}
                  --expected=${REFS}/eight_clusters_400.txt)
endforeach()

# ---- Contrats d'echec et validateurs (audit du 31 aout, quatrieme cycle).
# Le juge refuse un narrowing hors domaine et une reference tronquee ; le
# validateur de campagne est prouve fail-closed par sa porte Python ; la
# descente fusionnee tue wspd-drop-rect par son grand-livre (route produit).
mhgp6_gate(mhgp6_juge_refus_narrowing 2 mhgp6_conformity
           ARGS --family=uniform --n=4294967696 --threads=4 --expected=${REFS}/uniform_400.txt)
mhgp6_gate(mhgp6_juge_refus_reference_tronquee 2 mhgp6_conformity
           ARGS --family=uniform --n=400 --threads=4
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_400_tronque_k1_absent.txt)
# Cinquieme cycle : K1 correct + K10 EN TROP a n=2 (kmax_eff=1) — l'ensemble
# des clefs doit EGALER {1..kmax_eff}, une clef en trop est refusee (code 2),
# jamais silencieusement ignoree.
mhgp6_gate(mhgp6_juge_refus_k_en_trop 2 mhgp6_conformity
           ARGS --family=uniform --n=2 --threads=1
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_2_k10_en_trop.txt)
# Troisieme tour gcp : K11 SURNUMERAIRE (hors domaine global [1,10]) refuse
# par le chargeur lui-meme (code 2), jamais silencieusement ignore.
mhgp6_gate(mhgp6_juge_refus_k11 2 mhgp6_conformity
           ARGS --family=uniform --n=400 --threads=4
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_400_k11_en_trop.txt)
mhgp6_gate(mhgp6_cover_refus_mutant_hors_cible 2 mhgp6_cover_fixture ARGS --inject=rle-drop)
mhgp6_gate(mhgp6_sweep_refus_inject_vide 2 mhgp6_sweep_fixtures ARGS --inject=)
mhgp6_gate(mhgp6_fused_mutant_droprect 4 mhgp6_selftest ARGS --fused-descent --inject=wspd-drop-rect)
add_test(NAME mhgp6_pentes_validateur
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pentes_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_pentes_validateur PROPERTIES LABELS "gate")
# Provenance d'executable des campagnes locales (alerte « campagne CPU
# mixte ») : copie privee immuable, hash avant/apres chaque tuple, INVALID
# sans DONE sur reconstruction concurrente simulee.
add_test(NAME mhgp6_campagne_provenance
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/campagne_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_campagne_provenance PROPERTIES LABELS "gate")
# Agregateur inter-graines PREENREGISTRE (regle figee avant la campagne
# jugee) : nominal + refus campagne invalide + activation par T_lourde.
add_test(NAME mhgp6_agregateur_intergraines
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/agregateur_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_agregateur_intergraines PROPERTIES LABELS "gate")
