# MorseHGP3D v6 — chantier actif. C++20 sans extensions, -Werror.
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v6 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Bornes a COMPILE_LANGUAGE:CXX (audit § 5.8) : sinon ces options globales
# atteignent nvcc quand MHGP6_ENABLE_CUDA est ON et le font refuser ; les
# avertissements hote des cibles CUDA passent separement par -Xcompiler.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>")

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
  cmake_parse_arguments(G "" "TIMEOUT;EXPECT_LINE" "LABELS;ARGS" ${ARGN})
  string(JOIN " " joined ${G_ARGS})
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -DCMD=$<TARGET_FILE:${target}> "-DARGS=${joined}"
                   "-DEXPECT_LINE=${G_EXPECT_LINE}"
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
# Dette d'echelle (balayage du 1er septembre) : caps de cardinalite a
# overshoot borne (arret pre-fusion globale), budget partiel de tampons
# nommes, refus resource_exhausted, objet inchange sous budget large.
mhgp6_gate(mhgp6_caps_refus 0 mhgp6_selftest ARGS --caps-refus)
mhgp6_gate(mhgp6_caps_mutant_emission 4 mhgp6_selftest ARGS --caps-refus --inject=caps-drop-emission)
mhgp6_gate(mhgp6_caps_mutant_late_wave 4 mhgp6_selftest ARGS --caps-refus --inject=caps-late-wave-check)
# Garde 2E de la fusion globale (§ 6.1) : sautee, le refus retombe sur le tri
# APRES la materialisation du payload logique nomme — message different, tue
# par la fenetre dediee du selftest.
mhgp6_gate(mhgp6_caps_mutant_prefusion 4 mhgp6_selftest ARGS --caps-refus --inject=caps-skip-prefusion-budget)
set_tests_properties(mhgp6_caps_refus mhgp6_caps_mutant_emission mhgp6_caps_mutant_late_wave mhgp6_caps_mutant_prefusion PROPERTIES TIMEOUT 600)
# G0' du plan GPU v6 : pool d'executeurs (hote pur, port contractuel v5 +
# confinement de panne COTE WORKER — la dent v5 reparee la ou la course
# causale du confinement producteur ne peut pas exister, REPONSE_AUDITEURS
# § 5.6 ; fixture permanente du travail poste-fatal, scenario 10).
mhgp6_executable(mhgp6_executor_pool_gate tests/executor_pool_gate.cpp)
mhgp6_gate(mhgp6_executor_pool 0 mhgp6_executor_pool_gate TIMEOUT 180)
mhgp6_gate(mhgp6_executor_pool_mutant_serial 4 mhgp6_executor_pool_gate ARGS --inject=pool-serial TIMEOUT 180)
mhgp6_gate(mhgp6_executor_pool_mutant_drop_exception 4 mhgp6_executor_pool_gate ARGS --inject=pool-drop-exception TIMEOUT 180)
mhgp6_gate(mhgp6_executor_pool_mutant_worker_resume 4 mhgp6_executor_pool_gate ARGS --inject=pool-worker-resume-after-fatal TIMEOUT 180)
mhgp6_gate(mhgp6_executor_pool_mutant_activate_late 4 mhgp6_executor_pool_gate ARGS --inject=pool-activate-after-unlock TIMEOUT 180)
# C2 : wire serie C v1 (docs/GPU.md § Wire) — brique hote pure, division
# hissee (six candidats u32 par boule), serialisation canonique + digest, refus
# fail-closed ; mutants gpu-index-drop-node et wire-t1-plus-one.
mhgp6_executable(mhgp6_wire_gate tests/wire_gate.cpp)
mhgp6_gate(mhgp6_wire 0 mhgp6_wire_gate TIMEOUT 180)
mhgp6_gate(mhgp6_wire_mutant_drop_node 4 mhgp6_wire_gate ARGS --inject=gpu-index-drop-node TIMEOUT 180)
mhgp6_gate(mhgp6_wire_mutant_t1 4 mhgp6_wire_gate ARGS --inject=wire-t1-plus-one TIMEOUT 180)
# C3/C4 : kernels prefiltre+census en STUB HOTE — bit-identite boule a boule
# contre le scalaire de production sur les candidats REELS (deux familles),
# ordre de pile du scalaire compris ; 4 mutants device par drapeaux (l'hote
# traduit le registre, le device ne le consulte jamais).
mhgp6_executable(mhgp6_census_device_stub_gate tests/census_device_stub_gate.cpp)
mhgp6_gate(mhgp6_census_device_stub 0 mhgp6_census_device_stub_gate TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_range_le 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-range-add-le TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_stack 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-stack-shallow TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_swap_push 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-swap-push-order TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_nonstrict 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-census-nonstrict TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_skip_write 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-skip-ball-write TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_nshell 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-nshell-overdomain TIMEOUT 600)
mhgp6_gate(mhgp6_census_stub_mutant_skip_count 4 mhgp6_census_device_stub_gate ARGS --inject=gpu-skip-count-write TIMEOUT 600)
# C5 (preuve locale) : le pipeline COMPLET avec la route serie C en stub —
# objet identique a la route CPU (digests, cartes, totaux), refus
# transactionnel du run entier sous mutants (jamais un prefixe publie).
mhgp6_executable(mhgp6_pilot_stub_gate tests/pilot_stub_gate.cpp)
mhgp6_gate(mhgp6_pilot_stub 0 mhgp6_pilot_stub_gate TIMEOUT 600)
# Le PILOTE lui-meme en stub (syntaxe + logique C++ hote du .cu, refus de
# parsing exerce ; la parite est prouvee par mhgp6_pilot_stub ci-dessus).
mhgp6_product_executable(mhgp6_pilote_stub_check tests/pilot_cuda_stub_check.cpp)
mhgp6_gate(mhgp6_pilote_stub_refus_n 2 mhgp6_pilote_stub_check ARGS --n=200x TIMEOUT 120)
mhgp6_gate(mhgp6_pilote_stub_parite_200 0 mhgp6_pilote_stub_check ARGS --family=uniform --n=200 --seed=3 --threads=4 TIMEOUT 600)
# FRONTIERE MULTI-LOTS (e9cfad9e) : --lot=17 avec plancher lots > 1 — base,
# offsets et fusion en ordre GLOBAL juges au-dela de base=0 ; variante TEST
# pour le mutant base-reset (le build produit refuse toute injection).
mhgp6_gate(mhgp6_pilote_stub_lot17 0 mhgp6_pilote_stub_check ARGS --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 TIMEOUT 600)
mhgp6_executable(mhgp6_pilote_stub_check_test tests/pilot_cuda_stub_check.cpp)
mhgp6_gate(mhgp6_pilote_stub_mutant_base 4 mhgp6_pilote_stub_check_test ARGS --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --inject=gpu-lot-base-reset TIMEOUT 600)
# JUGE DES RECORDS (§ 5.13) : la grammaire publiee par le pilote (cinq
# records exacts, ABBA/BAAB, signatures canoniques recalculees, formules
# d'octets 112/100/100 par boule, temps finis, lots) est jugee par
# tests/pilote_juge.py — le MEME juge que le validateur G4 applique aux
# reçus : les champs sont causaux avant toute depense device. Les deux
# ordres de base sont exerces ; les contre-fixtures integrees du juge
# doivent toutes etre tuees (jamais un vert par vacuite).
add_test(NAME mhgp6_pilote_juge_contre_fixtures
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --contre-fixtures"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_pilote_juge_contre_fixtures PROPERTIES LABELS "gate" TIMEOUT 120)
add_test(NAME mhgp6_pilote_stub_juge
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --binaire $<TARGET_FILE:mhgp6_pilote_stub_check> --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --repeat=4 --ordre=cpu-device"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_pilote_stub_juge PROPERTIES LABELS "gate" TIMEOUT 600)
add_test(NAME mhgp6_pilote_stub_juge_ordre_inverse
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --binaire $<TARGET_FILE:mhgp6_pilote_stub_check> --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --repeat=4 --ordre=device-cpu"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_pilote_stub_juge_ordre_inverse PROPERTIES LABELS "gate" TIMEOUT 600)
mhgp6_gate(mhgp6_pilot_stub_mutant_stack 4 mhgp6_pilot_stub_gate ARGS --inject=gpu-stack-shallow TIMEOUT 600)
mhgp6_gate(mhgp6_pilot_stub_mutant_nonstrict 4 mhgp6_pilot_stub_gate ARGS --inject=gpu-census-nonstrict TIMEOUT 600)
# Frontiere D2H causale (f3704e99) : ecriture omise et n_shell hors domaine
# refuses par le VALIDATEUR avant toute reconstruction.
mhgp6_gate(mhgp6_pilot_stub_mutant_skip_write 4 mhgp6_pilot_stub_gate ARGS --inject=gpu-skip-ball-write TIMEOUT 600)
mhgp6_gate(mhgp6_pilot_stub_mutant_nshell 4 mhgp6_pilot_stub_gate ARGS --inject=gpu-nshell-overdomain TIMEOUT 600)
mhgp6_gate(mhgp6_pilot_stub_mutant_skip_count 4 mhgp6_pilot_stub_gate ARGS --inject=gpu-skip-count-write TIMEOUT 600)
# C1 (suite) : temoin device en STUB HOTE — le .cu compile comme C++ pur via
# le wrapper (tests/cuda_stub.hpp, kernels en boucles sequentielles). Prouve
# localement syntaxe + logique + mutant AVANT toute session G4 ; jamais un
# recu device. Le meme .cu est la cible reelle du bloc MHGP6_ENABLE_CUDA.
mhgp6_executable(mhgp6_device_witness_stub tests/device_witness_stub.cpp)
mhgp6_gate(mhgp6_device_witness_stub 0 mhgp6_device_witness_stub TIMEOUT 180)
mhgp6_gate(mhgp6_device_witness_stub_mutant_carry 4 mhgp6_device_witness_stub ARGS --inject=witness-di128-lost-carry TIMEOUT 180)
mhgp6_gate(mhgp6_device_witness_stub_mutant_skip_write 4 mhgp6_device_witness_stub ARGS --inject=witness-skip-write TIMEOUT 180)
mhgp6_gate(mhgp6_device_witness_stub_mutant_skip_native 4 mhgp6_device_witness_stub ARGS --inject=witness-skip-native-write TIMEOUT 180)
# Contre-fixture gravee (5e contre-lecture § 5.8) : la DOUBLE injection
# skip+carry ne rend jamais un 4 aveugle — les oracles tranchent (code 1).
mhgp6_gate(mhgp6_device_witness_stub_contre_double 1 mhgp6_device_witness_stub ARGS "--inject=witness-skip-write,witness-di128-lost-carry" TIMEOUT 180)
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
# Signature CLI du budget partiel (semantique livree => porte) : UNE SEULE
# execution jugee sur le code 0 ET la ligne exacte — scope + les trois
# valeurs, cap effectif du MEME helper que l'execution (2^30 / 144 =
# 7456540). PASS_REGULAR ignore le code de sortie : run_expect capture et
# reemet stdout pour juger les deux ensemble.
mhgp6_gate(mhgp6_cli_budget_signature 0 mhgp6 ARGS --family=uniform --n=200 --s=8 --mem-budget=1073741824 TIMEOUT 120
  EXPECT_LINE "memory_budget_scope=partial_named_payload_proxy_v1 budget=1073741824 cap_brut_demande=4294967295 cap_brut_effectif=7456540 cap_fusion_budgetaire=3728270")

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
            par-drop-ball-chunk fold-inject-a-failure-k2
            ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice)
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

# ---- Cibles de PROFIL explicites + porte (§ 5.10) : l'identite de build est
# signee par la CIBLE (jamais par des macros de CMAKE_CXX_FLAGS) ; la porte
# atteste une PROJECTION DETERMINISTE NOMMEE (digest_all + digest_forest_K* +
# cardinalites K=) identique entre normal, profil, vivacite et join 0/1 —
# jamais « l'objet » complet — plus la structure (schemas exacts par K,
# planchers PAR K, fermeture somme/residuel aux bornes INTERNES du corps de
# reduce_fold, causalite de fold_join, discrimination des builds). Les
# intervalles rendent lisibles les recouvrements reduction/reduction et
# A/reduction. Le binaire de profil n'est JAMAIS un mur de debit.
mhgp6_product_executable(mhgp6_profile cli/mhgp6.cpp)
target_compile_definitions(mhgp6_profile PRIVATE MHGP6_PROFILE_REDUCE=1)
# Variante VIVACITE enregistree et exercee (contre-lecture 01bd14a9 : le
# garde de compilation seul ne couvre pas cette branche).
mhgp6_product_executable(mhgp6_profile_liveness cli/mhgp6.cpp)
target_compile_definitions(mhgp6_profile_liveness PRIVATE MHGP6_PROFILE_REDUCE=1 MHGP6_PROFILE_LIVENESS=1)
# SONDE D'ABLATION du reduce (2 septembre, arbre § 5.10) : binaire de PROFIL
# sous MHGP6_TESTING, accepte --inject=ablation-* (copie profonde retiree,
# tris retires, cle factice au remplissage). Chaque ablation change l'objet
# et est tuee (code 4) dans la boucle des mutants ; le binaire produit
# refuse --inject= (code 2). Jamais un mur : une attribution decomposee.
mhgp6_executable(mhgp6_profile_sonde cli/mhgp6.cpp)
target_compile_definitions(mhgp6_profile_sonde PRIVATE MHGP6_PROFILE_REDUCE=1)
mhgp6_gate(mhgp6_profile_refuse_inject 2 mhgp6_profile
           ARGS --family=uniform --n=400 --inject=ablation-mat-sans-copie)
mhgp6_gate(mhgp6_profile_sonde_refuse_inconnu 2 mhgp6_profile_sonde
           ARGS --family=uniform --n=400 --inject=ablation-inconnue)
add_test(NAME mhgp6_profil_identite
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/profil_gate.py $<TARGET_FILE:mhgp6> $<TARGET_FILE:mhgp6_profile> $<TARGET_FILE:mhgp6_profile_liveness>"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_profil_identite PROPERTIES LABELS "gate" TIMEOUT 600)
# CONTRE-FIXTURE AUTONOME des seuils (§ 5.13) : la scene « neuf composantes
# nulles, somme=0.008, residuel=0.012, mur=0.020 » doit etre tuee par les
# seuils serres 0.0051/0.006, et la scene aux arrondis %.3f honnetes rester
# verte — le juge exerce est le VRAI check_profile_output importe.
add_test(NAME mhgp6_profil_contre_fixture
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/profil_contre_fixture.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp6_profil_contre_fixture PROPERTIES LABELS "gate" TIMEOUT 120)
# Contrat d'echec sous profil, COMPILE (71528f8a : le CLI ne print jamais
# apres un refus — l'inspection porte sur le RunResult, jamais sur stdout).
add_executable(mhgp6_profil_contrat tests/profil_contrat_echec.cpp)
target_compile_definitions(mhgp6_profil_contrat PRIVATE MHGP6_PROFILE_REDUCE=1 MHGP6_TESTING=1)
target_link_libraries(mhgp6_profil_contrat PRIVATE Threads::Threads)
mhgp6_gate(mhgp6_profil_contrat_echec 0 mhgp6_profil_contrat TIMEOUT 600)
# Scene NON VACUEUSE (ac6b4bc1) : K1 publie puis panne A a K=2 — le refus
# budget minuscule precederait fold_profiles.assign et ne prouverait rien.
mhgp6_gate(mhgp6_profil_contrat_echec_k2 4 mhgp6_profil_contrat ARGS --inject=fold-inject-a-failure-k2 TIMEOUT 600)

# ---- CUDA (serie C, docs/GPU.md) : OFF par defaut, JAMAIS construit en CI.
# Les portes `gpu` ne s'executent qu'en session G4 (sm_120, RTX PRO 6000
# Blackwell). -fmad=false : aucune contraction implicite ; le socle serie C
# est 100 % entier, le flottant n'y entre pas. Incompatible avec tout futur
# profil sanitizers (motif v5 : refus explicite si les deux sont demandes).
option(MHGP6_ENABLE_CUDA "Construire les cibles device de la serie C (G4 seulement)" OFF)
if(MHGP6_ENABLE_CUDA)
  if(CMAKE_CXX_FLAGS MATCHES "fsanitize")
    message(FATAL_ERROR "MHGP6_ENABLE_CUDA est incompatible avec des flags sanitizers")
  endif()
  # sm_120 : a fixer AVANT enable_language, sinon CMake initialise
  # l'architecture par defaut (lecon v5, session G4 50fee05c).
  if(NOT CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120)
  endif()
  # § 5.15.2 : l'architecture serie C est CONTRACTUELLE (sm_120, G4/RTX PRO
  # 6000 Blackwell) — une surcharge (86, all, ...) fabriquerait un recu
  # device d'une autre machine : refus a la configuration.
  if(NOT CMAKE_CUDA_ARCHITECTURES STREQUAL "120")
    message(FATAL_ERROR "MHGP6_ENABLE_CUDA exige CMAKE_CUDA_ARCHITECTURES=120 (recu ${CMAKE_CUDA_ARCHITECTURES})")
  endif()
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  add_executable(mhgp6_device_witness src/gpu/device_witness.cu)
  target_compile_options(mhgp6_device_witness PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp6_device_witness PRIVATE MHGP6_TESTING=1)
  target_link_libraries(mhgp6_device_witness PRIVATE Threads::Threads)
  mhgp6_gate(mhgp6_device_witness 0 mhgp6_device_witness LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_device_witness_mutant_carry 4 mhgp6_device_witness ARGS --inject=witness-di128-lost-carry LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_device_witness_mutant_skip_write 4 mhgp6_device_witness ARGS --inject=witness-skip-write LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_device_witness_mutant_skip_native 4 mhgp6_device_witness ARGS --inject=witness-skip-native-write LABELS gpu TIMEOUT 600)
  # C3/C4 DEVICE : memes exigences que la porte stub, kernels sur le device
  # reel ; l'architecture compilee est SIGNEE dans le binaire (§ 5.8 : le
  # recu certifie arch compilee ET device observe).
  add_executable(mhgp6_census_device_gate tests/census_device_gate.cu)
  target_compile_options(mhgp6_census_device_gate PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp6_census_device_gate PRIVATE MHGP6_TESTING=1
    MHGP6_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp6_census_device_gate PRIVATE Threads::Threads)
  mhgp6_gate(mhgp6_census_device 0 mhgp6_census_device_gate LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_range_le 4 mhgp6_census_device_gate ARGS --inject=gpu-range-add-le LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_stack 4 mhgp6_census_device_gate ARGS --inject=gpu-stack-shallow LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_swap 4 mhgp6_census_device_gate ARGS --inject=gpu-swap-push-order LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_nonstrict 4 mhgp6_census_device_gate ARGS --inject=gpu-census-nonstrict LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_skip_write 4 mhgp6_census_device_gate ARGS --inject=gpu-skip-ball-write LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_nshell 4 mhgp6_census_device_gate ARGS --inject=gpu-nshell-overdomain LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_census_device_mutant_skip_count 4 mhgp6_census_device_gate ARGS --inject=gpu-skip-count-write LABELS gpu TIMEOUT 600)
  # C5 : PILOTE serie C — les deux routes sur le meme nuage, parite de TOUS
  # les digests exigee, couts wire/H2D/kernels/D2H separes (§ 5.5). La porte
  # 400 est la verification fonctionnelle ; les mesures 50k+ viennent du
  # profil de campagne, jamais d'un gate.
  add_executable(mhgp6_cuda cli/mhgp6_cuda.cu)
  target_compile_options(mhgp6_cuda PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp6_cuda PRIVATE MHGP6_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp6_cuda PRIVATE Threads::Threads)
  mhgp6_gate(mhgp6_pilote_parite_400 0 mhgp6_cuda ARGS --family=uniform --n=400 --seed=3 --threads=8 LABELS gpu TIMEOUT 600)
  mhgp6_gate(mhgp6_pilote_refus_n 2 mhgp6_cuda ARGS --n=200x LABELS gpu TIMEOUT 120)
  mhgp6_gate(mhgp6_pilote_lot17 0 mhgp6_cuda ARGS --family=uniform --n=400 --seed=3 --threads=8 --lot=17 --min-lots=2 LABELS gpu TIMEOUT 600)
  add_executable(mhgp6_cuda_test cli/mhgp6_cuda.cu)
  target_compile_options(mhgp6_cuda_test PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp6_cuda_test PRIVATE MHGP6_TESTING=1 MHGP6_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp6_cuda_test PRIVATE Threads::Threads)
  mhgp6_gate(mhgp6_pilote_mutant_base 4 mhgp6_cuda_test ARGS --family=uniform --n=400 --seed=3 --threads=8 --lot=17 --min-lots=2 --inject=gpu-lot-base-reset LABELS gpu TIMEOUT 600)
endif()
