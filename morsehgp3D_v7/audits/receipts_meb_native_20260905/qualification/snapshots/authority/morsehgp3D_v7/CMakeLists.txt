# MorseHGP3D v7 — chantier actif. C++20 sans extensions, -Werror.
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v7 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Bornes a COMPILE_LANGUAGE:CXX (audit § 5.8) : sinon ces options globales
# atteignent nvcc quand MHGP7_ENABLE_CUDA est ON et le font refuser ; les
# avertissements hote des cibles CUDA passent separement par -Xcompiler.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>")

find_package(Threads REQUIRED)
enable_testing()

# Cible PRODUIT : aucun mutant compile (pas de MHGP7_TESTING).
function(mhgp7_product_executable name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

# Cibles de TEST : les points d'injection de mutants sont compiles.
function(mhgp7_executable name)
  add_executable(${name} ${ARGN})
  target_compile_definitions(${name} PRIVATE MHGP7_TESTING=1)
  target_link_libraries(${name} PRIVATE Threads::Threads)
endfunction()

# Porte a code de sortie exact : 0 conforme, 1 juge, 2 refus, 3 plancher ou
# invariant, 4 mutant tue. Labels optionnels apres les arguments.
function(mhgp7_gate name expected target)
  cmake_parse_arguments(G "" "TIMEOUT;EXPECT_LINE;EXPECT_PREFIX" "LABELS;ARGS" ${ARGN})
  string(JOIN " " joined ${G_ARGS})
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -DCMD=$<TARGET_FILE:${target}> "-DARGS=${joined}"
                   "-DEXPECT_LINE=${G_EXPECT_LINE}"
                   "-DEXPECT_PREFIX=${G_EXPECT_PREFIX}"
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
mhgp7_product_executable(mhgp7 cli/mhgp7.cpp)

# ---- Selftests du cœur (etablis en premier ; chaque porte a son code exact).
mhgp7_executable(mhgp7_selftest tests/selftest.cpp)
mhgp7_gate(mhgp7_arith_selftest 0 mhgp7_selftest ARGS --arith)
mhgp7_gate(mhgp7_sha256_selftest 0 mhgp7_selftest ARGS --sha256)
mhgp7_gate(mhgp7_families_fixture 0 mhgp7_selftest ARGS --families)
# Mutant de cardinalite des scanlines : tue par la divergence du digest grave
# de scanline_overlap_multiecho (n=2000, graine 3). Sur scanline_single_pass
# il est structurellement invisible (multi_echo=false : la garde de push est
# redondante avec la borne de boucle) — voir tests/selftest.cpp.
mhgp7_gate(mhgp7_families_mutant_overshoot 4 mhgp7_selftest ARGS --families --inject=family-scanline-overshoot)
mhgp7_gate(mhgp7_tree_selftest 0 mhgp7_selftest ARGS --tree)
mhgp7_gate(mhgp7_wspd_ledger 0 mhgp7_selftest ARGS --wspd-ledger)
# Requalification de la descente fusionnee (cinquieme cycle) : ownership par
# paire (exactement une fois), litteral n-1 par position, separation de
# chaque rectangle, grand-livre sans masse tuee a h infini, fixtures gravees
# du nombre de rectangles ; mutants cap/split retargetes sur la route fusionnee.
mhgp7_gate(mhgp7_wspd_ownership 0 mhgp7_selftest ARGS --wspd-ownership)
mhgp7_gate(mhgp7_wspd_mutant_cap 4 mhgp7_selftest ARGS --wspd-ownership --inject=wspd-cap-terminal)
mhgp7_gate(mhgp7_wspd_mutant_split 4 mhgp7_selftest ARGS --wspd-ownership --inject=wspd-split-heaviest)
# Fixture bibliotheque des contrats d'echec (P2 cinquieme cycle) : entree
# invalide + run complet a callbacks (sequence K stricte) ; sous mutant,
# chaque echec (census, fold K2 x inflight {1,2,8}) invalide TOUT champ
# provisoire et les callbacks restent provisoires (authority=status_terminal).
mhgp7_gate(mhgp7_contrat_echec 0 mhgp7_selftest ARGS --failure-contract TIMEOUT 600)
mhgp7_gate(mhgp7_contrat_echec_census 4 mhgp7_selftest ARGS --failure-contract --inject=census-nonstrict TIMEOUT 600)
mhgp7_gate(mhgp7_contrat_echec_fold_k2 4 mhgp7_selftest ARGS --failure-contract --inject=fold-inject-a-failure-k2 TIMEOUT 600)
# FUITE DE PREFIXE DE PAYLOAD (relecture adverse du 2 septembre) : Sigma|parents|
# DECRIT le payload publie et entre donc dans invalidate_provisional ; il
# manquait aux deux projections `provisoires_vides`. Mutant CONTRE-CONTRAT
# `provisional-keep-sum-parents` : il ne l'efface plus, et le refus a K=2
# laisse alors visible le Sigma|parents| de la foret K=1 — un prefixe exact de
# payload. Injecte AVEC le mutant qui provoque le refus (sans echec, rien a
# observer) ; polarite inversee et nommee dans le mode.
mhgp7_gate(mhgp7_contrat_echec_fuite_sum_parents 4 mhgp7_selftest
           ARGS --failure-contract --inject=fold-inject-a-failure-k2,provisional-keep-sum-parents TIMEOUT 600)
# ETAGE NOMME D'UN bad_alloc (alerte G4 du 2 septembre) : une panne
# d'allocation devient un refus transactionnel resource_exhausted qui NOMME
# l'etage atteint — jamais un abort (code 134) qui ne dit rien. Le temoin
# (code 0) interdit le vert par vacuite ; les deux scenes injectees exigent le
# code EXACT du refus (2) ET la ligne portant le nom de l'etage, sur la MEME
# execution. La scene `fold` couvre le seul etage a WORKERS.
mhgp7_executable(mhgp7_bad_alloc_gate tests/bad_alloc_gate.cpp)
mhgp7_gate(mhgp7_bad_alloc_temoin 0 mhgp7_bad_alloc_gate ARGS --threads=4 TIMEOUT 600)
mhgp7_gate(mhgp7_bad_alloc_etage_provision 2 mhgp7_bad_alloc_gate
           ARGS --inject=caps-throw-bad-alloc-provision --etage=entree --threads=4
           EXPECT_PREFIX "caps-throw-bad-alloc-provision inflight=1 : resource_exhausted : allocation impossible a l'etage entree (rss_mb apres_generation=" TIMEOUT 600)
mhgp7_gate(mhgp7_bad_alloc_etage_census 2 mhgp7_bad_alloc_gate
           ARGS --threads=4 --inject=caps-throw-bad-alloc-census --etage=census
           EXPECT_PREFIX "caps-throw-bad-alloc-census inflight=1 : resource_exhausted : allocation impossible a l'etage census (rss_mb apres_generation=" TIMEOUT 600)
mhgp7_gate(mhgp7_bad_alloc_etage_fold 2 mhgp7_bad_alloc_gate
           ARGS --threads=4 --inject=caps-throw-bad-alloc-fold --etage=fold
           EXPECT_PREFIX "caps-throw-bad-alloc-fold inflight=1 : resource_exhausted : allocation impossible a l'etage fold (rss_mb apres_generation=" TIMEOUT 600)
# Etage E6 (--e6-grille) : objet et multiensemble emis BIT-IDENTIQUES sous la
# grille raffinee, W_sweep1 jamais augmente, plancher de grilles construites.
mhgp7_gate(mhgp7_e6_grille_objet 0 mhgp7_selftest ARGS --e6-equal TIMEOUT 600)
mhgp7_gate(mhgp7_fused_descent 0 mhgp7_selftest ARGS --fused-descent)
mhgp7_gate(mhgp7_sweep_oracle 0 mhgp7_selftest ARGS --sweep-oracle LABELS "gate;oracle")
# Dette d'echelle (balayage du 1er septembre) : caps de cardinalite a
# overshoot borne (arret pre-fusion globale), budget partiel de tampons
# nommes, refus resource_exhausted, objet inchange sous budget large.
mhgp7_gate(mhgp7_caps_refus 0 mhgp7_selftest ARGS --caps-refus)
mhgp7_gate(mhgp7_caps_mutant_emission 4 mhgp7_selftest ARGS --caps-refus --inject=caps-drop-emission)
mhgp7_gate(mhgp7_caps_mutant_late_wave 4 mhgp7_selftest ARGS --caps-refus --inject=caps-late-wave-check)
# Garde 2E de la fusion globale (§ 6.1) : sautee, le refus retombe sur le tri
# APRES la materialisation du payload logique nomme — message different, tue
# par la fenetre dediee du selftest.
mhgp7_gate(mhgp7_caps_mutant_prefusion 4 mhgp7_selftest ARGS --caps-refus --inject=caps-skip-prefusion-budget)
set_tests_properties(mhgp7_caps_refus mhgp7_caps_mutant_emission mhgp7_caps_mutant_late_wave mhgp7_caps_mutant_prefusion PROPERTIES TIMEOUT 600)
# G0' du plan GPU v6 : pool d'executeurs (hote pur, port contractuel v5 +
# confinement de panne COTE WORKER — la dent v5 reparee la ou la course
# causale du confinement producteur ne peut pas exister, REPONSE_AUDITEURS
# § 5.6 ; fixture permanente du travail poste-fatal, scenario 10).
mhgp7_executable(mhgp7_executor_pool_gate tests/executor_pool_gate.cpp)
mhgp7_gate(mhgp7_executor_pool 0 mhgp7_executor_pool_gate TIMEOUT 180)
mhgp7_gate(mhgp7_executor_pool_mutant_serial 4 mhgp7_executor_pool_gate ARGS --inject=pool-serial TIMEOUT 180)
mhgp7_gate(mhgp7_executor_pool_mutant_drop_exception 4 mhgp7_executor_pool_gate ARGS --inject=pool-drop-exception TIMEOUT 180)
mhgp7_gate(mhgp7_executor_pool_mutant_worker_resume 4 mhgp7_executor_pool_gate ARGS --inject=pool-worker-resume-after-fatal TIMEOUT 180)
mhgp7_gate(mhgp7_executor_pool_mutant_activate_late 4 mhgp7_executor_pool_gate ARGS --inject=pool-activate-after-unlock TIMEOUT 180)
# C2 : wire serie C v1 (docs/GPU.md § Wire) — brique hote pure, division
# hissee (six candidats u32 par boule), serialisation canonique + digest, refus
# fail-closed ; mutants gpu-index-drop-node et wire-t1-plus-one.
mhgp7_executable(mhgp7_wire_gate tests/wire_gate.cpp)
mhgp7_gate(mhgp7_wire 0 mhgp7_wire_gate TIMEOUT 180)
mhgp7_gate(mhgp7_wire_mutant_drop_node 4 mhgp7_wire_gate ARGS --inject=gpu-index-drop-node TIMEOUT 180)
mhgp7_gate(mhgp7_wire_mutant_t1 4 mhgp7_wire_gate ARGS --inject=wire-t1-plus-one TIMEOUT 180)
# C3/C4 : kernels prefiltre+census en STUB HOTE — bit-identite boule a boule
# contre le scalaire de production sur les candidats REELS (deux familles),
# ordre de pile du scalaire compris ; 4 mutants device par drapeaux (l'hote
# traduit le registre, le device ne le consulte jamais).
mhgp7_executable(mhgp7_census_device_stub_gate tests/census_device_stub_gate.cpp)
mhgp7_gate(mhgp7_census_device_stub 0 mhgp7_census_device_stub_gate TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_range_le 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-range-add-le TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_stack 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-stack-shallow TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_swap_push 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-swap-push-order TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_nonstrict 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-census-nonstrict TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_skip_write 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-skip-ball-write TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_nshell 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-nshell-overdomain TIMEOUT 600)
mhgp7_gate(mhgp7_census_stub_mutant_skip_count 4 mhgp7_census_device_stub_gate ARGS --inject=gpu-skip-count-write TIMEOUT 600)
# C5 (preuve locale) : le pipeline COMPLET avec la route serie C en stub —
# objet identique a la route CPU (digests, cartes, totaux), refus
# transactionnel du run entier sous mutants (jamais un prefixe publie).
mhgp7_executable(mhgp7_pilot_stub_gate tests/pilot_stub_gate.cpp)
mhgp7_gate(mhgp7_pilot_stub 0 mhgp7_pilot_stub_gate TIMEOUT 600)
# Le PILOTE lui-meme en stub (syntaxe + logique C++ hote du .cu, refus de
# parsing exerce ; la parite est prouvee par mhgp7_pilot_stub ci-dessus).
mhgp7_product_executable(mhgp7_pilote_stub_check tests/pilot_cuda_stub_check.cpp)
mhgp7_gate(mhgp7_pilote_stub_refus_n 2 mhgp7_pilote_stub_check ARGS --n=200x TIMEOUT 120)
mhgp7_gate(mhgp7_pilote_stub_parite_200 0 mhgp7_pilote_stub_check ARGS --family=uniform --n=200 --seed=3 --threads=4 TIMEOUT 600)
# FRONTIERE MULTI-LOTS (e9cfad9e) : --lot=17 avec plancher lots > 1 — base,
# offsets et fusion en ordre GLOBAL juges au-dela de base=0 ; variante TEST
# pour le mutant base-reset (le build produit refuse toute injection).
mhgp7_gate(mhgp7_pilote_stub_lot17 0 mhgp7_pilote_stub_check ARGS --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 TIMEOUT 600)
mhgp7_executable(mhgp7_pilote_stub_check_test tests/pilot_cuda_stub_check.cpp)
mhgp7_gate(mhgp7_pilote_stub_mutant_base 4 mhgp7_pilote_stub_check_test ARGS --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --inject=gpu-lot-base-reset TIMEOUT 600)
# JUGE DES RECORDS (§ 5.13) : la grammaire publiee par le pilote (cinq
# records exacts, ABBA/BAAB, signatures canoniques recalculees, formules
# d'octets 112/100/100 par boule, temps finis, lots) est jugee par
# tests/pilote_juge.py — le MEME juge que le validateur G4 applique aux
# reçus : les champs sont causaux avant toute depense device. Les deux
# ordres de base sont exerces ; les contre-fixtures integrees du juge
# doivent toutes etre tuees (jamais un vert par vacuite).
add_test(NAME mhgp7_pilote_juge_contre_fixtures
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --contre-fixtures"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_pilote_juge_contre_fixtures PROPERTIES LABELS "gate" TIMEOUT 120)
add_test(NAME mhgp7_pilote_stub_juge
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --binaire $<TARGET_FILE:mhgp7_pilote_stub_check> --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --repeat=4 --ordre=cpu-device"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_pilote_stub_juge PROPERTIES LABELS "gate" TIMEOUT 600)
add_test(NAME mhgp7_pilote_stub_juge_ordre_inverse
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pilote_juge.py --binaire $<TARGET_FILE:mhgp7_pilote_stub_check> --family=uniform --n=200 --seed=3 --threads=4 --lot=17 --min-lots=2 --repeat=4 --ordre=device-cpu"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_pilote_stub_juge_ordre_inverse PROPERTIES LABELS "gate" TIMEOUT 600)
mhgp7_gate(mhgp7_pilot_stub_mutant_stack 4 mhgp7_pilot_stub_gate ARGS --inject=gpu-stack-shallow TIMEOUT 600)
mhgp7_gate(mhgp7_pilot_stub_mutant_nonstrict 4 mhgp7_pilot_stub_gate ARGS --inject=gpu-census-nonstrict TIMEOUT 600)
# Frontiere D2H causale (f3704e99) : ecriture omise et n_shell hors domaine
# refuses par le VALIDATEUR avant toute reconstruction.
mhgp7_gate(mhgp7_pilot_stub_mutant_skip_write 4 mhgp7_pilot_stub_gate ARGS --inject=gpu-skip-ball-write TIMEOUT 600)
mhgp7_gate(mhgp7_pilot_stub_mutant_nshell 4 mhgp7_pilot_stub_gate ARGS --inject=gpu-nshell-overdomain TIMEOUT 600)
mhgp7_gate(mhgp7_pilot_stub_mutant_skip_count 4 mhgp7_pilot_stub_gate ARGS --inject=gpu-skip-count-write TIMEOUT 600)
# C1 (suite) : temoin device en STUB HOTE — le .cu compile comme C++ pur via
# le wrapper (tests/cuda_stub.hpp, kernels en boucles sequentielles). Prouve
# localement syntaxe + logique + mutant AVANT toute session G4 ; jamais un
# recu device. Le meme .cu est la cible reelle du bloc MHGP7_ENABLE_CUDA.
mhgp7_executable(mhgp7_device_witness_stub tests/device_witness_stub.cpp)
mhgp7_gate(mhgp7_device_witness_stub 0 mhgp7_device_witness_stub TIMEOUT 180)
mhgp7_gate(mhgp7_device_witness_stub_mutant_carry 4 mhgp7_device_witness_stub ARGS --inject=witness-di128-lost-carry TIMEOUT 180)
mhgp7_gate(mhgp7_device_witness_stub_mutant_skip_write 4 mhgp7_device_witness_stub ARGS --inject=witness-skip-write TIMEOUT 180)
mhgp7_gate(mhgp7_device_witness_stub_mutant_skip_native 4 mhgp7_device_witness_stub ARGS --inject=witness-skip-native-write TIMEOUT 180)
# Contre-fixture gravee (5e contre-lecture § 5.8) : la DOUBLE injection
# skip+carry ne rend jamais un 4 aveugle — les oracles tranchent (code 1).
mhgp7_gate(mhgp7_device_witness_stub_contre_double 1 mhgp7_device_witness_stub ARGS "--inject=witness-skip-write,witness-di128-lost-carry" TIMEOUT 180)
mhgp7_gate(mhgp7_sweep_mutant_depth 4 mhgp7_selftest ARGS --sweep-oracle --inject=sweep-nonstrict-depth)
mhgp7_gate(mhgp7_fused_mutant_mask 4 mhgp7_selftest ARGS --fused-descent --inject=fused-mask-stuck)

# ---- Conformite differentielle v5 ≡ v6 (digests graves au pin 3bad233d).
set(REFS ${CMAKE_CURRENT_SOURCE_DIR}/receipts/conformite_v5)
mhgp7_executable(mhgp7_conformity tests/conformity_v5.cpp)
foreach(spec "uniform;400" "terrain;400" "eight_clusters;400" "scanline_single_pass;400"
             "collinear_seven;600" "two_lines;200" "uniform;2000" "terrain;2000")
  list(GET spec 0 fam)
  list(GET spec 1 sz)
  mhgp7_gate(mhgp7_conformity_${fam}_${sz} 0 mhgp7_conformity
             ARGS --family=${fam} --n=${sz} --threads=4 --expected=${REFS}/${fam}_${sz}.txt TIMEOUT 600)
endforeach()
# Mutants du sweep tues par la divergence de conformite (digest_balls, forets).
mhgp7_gate(mhgp7_sweep_mutant_exit 4 mhgp7_conformity
           ARGS --family=terrain --n=400 --inject=sweep-drop-exit-root --expected=${REFS}/terrain_400.txt)
# rle-drop laisse l'objet intact (union-find idempotent) : son detecteur est
# la monnaie post-prefiltre (les doublons de survivants changent le digest).
mhgp7_gate(mhgp7_conformity_mutant_rle 4 mhgp7_conformity
           ARGS --family=uniform --n=400 --inject=rle-drop --expected=${REFS}/uniform_400.txt
                --postprefilter=97be65b6e0c66e3b3b2262510bd7274f8e557ae4bdb78024467c1f9ee05c4d72)

# ---- Refus du CLI (parsing exact, profil).
mhgp7_gate(mhgp7_cli_refus_s7 2 mhgp7 ARGS --family=uniform --n=200 --s=7)
mhgp7_gate(mhgp7_cli_refus_s_suffix 2 mhgp7 ARGS --family=uniform --n=200 --s=8junk)
mhgp7_gate(mhgp7_cli_refus_n_suffix 2 mhgp7 ARGS --family=uniform --n=200x)
mhgp7_gate(mhgp7_cli_ok_s8 0 mhgp7 ARGS --family=uniform --n=200 --s=8 TIMEOUT 120)
# Signature CLI du budget partiel (semantique livree => porte) : UNE SEULE
# execution jugee sur le code 0 ET la ligne exacte — scope + les trois
# valeurs, cap effectif du MEME helper que l'execution (2^30 / 144 =
# 7456540). PASS_REGULAR ignore le code de sortie : run_expect capture et
# reemet stdout pour juger les deux ensemble.
mhgp7_gate(mhgp7_cli_budget_signature 0 mhgp7 ARGS --family=uniform --n=200 --s=8 --mem-budget=1073741824 TIMEOUT 120
  EXPECT_LINE "memory_budget_scope=partial_named_payload_proxy_v1 budget=1073741824 cap_brut_demande=4294967295 cap_brut_effectif=7456540 cap_fusion_budgetaire=3728270")

# ---- Tailles d'interet (labels scale*) : conformite aux digests v5 graves.
foreach(fam uniform terrain eight_clusters scanline_single_pass scanline_overlap_multiecho)
  foreach(sz 8000 16000 32000)
    mhgp7_gate(mhgp7_conformity_${fam}_${sz} 0 mhgp7_conformity
               ARGS --family=${fam} --n=${sz} --threads=8
                    --expected=${CMAKE_CURRENT_SOURCE_DIR}/receipts/conformite_v5/${fam}_${sz}.txt
               LABELS scale${sz} TIMEOUT 3600)
  endforeach()
endforeach()

# ---- PREFIXE EXACT (palier P1) : la tour a smax=S est le prefixe de la tour a
# smax=11. Une porte par reference gravee (23), a smax=6 : memes digest_forest_K
# et memes cardinalites pour K = 1..5, contre les DIX ordres de la reference
# (dont on exige qu'elle soit strictement plus longue). Les planchers --min-*
# sont graves a ~50 % des valeurs MESUREES le 2 septembre sur ce depot.
# Motif : les mutants prefix-tamper-* sont gardes par kmax < 10 ; sans une porte
# a smax < 11 ils sont ORPHELINS (un nom au registre sans porte qui le tue).
# PORTEE EXACTE de --min-orders (rectifiee le 2 septembre) : `cmp_orders` vaut
# kmax_eff PAR CONSTRUCTION (la boucle va de 1 a kmax_eff sans continuation) ;
# --min-orders=5 n'est donc PAS un plancher de couverture mais le TEMOIN QUE
# LE CABLAGE DES PLANCHERS EST VIVANT (contre-fixture mhgp7_prefix_plancher_cable,
# --min-orders=99, code 3). Les vrais planchers de couverture sont
# --min-deltas/--min-facets/--min-events, accumules sur les cardinalites DU RUN.
set(PREFIX_SMAX 6)
foreach(spec "uniform;400;4;9200;30000;11000"
             "terrain;400;4;3000;11000;4100"
             "eight_clusters;400;4;7400;21900;8200"
             "scanline_single_pass;400;4;1900;12300;4600"
             "collinear_seven;600;4;5;42;15"
             "two_lines;200;4;9;1475;490"
             "uniform;2000;4;52000;181000;68000"
             "terrain;2000;4;15000;60000;22000")
  list(GET spec 0 fam)
  list(GET spec 1 sz)
  list(GET spec 2 th)
  list(GET spec 3 mind)
  list(GET spec 4 minf)
  list(GET spec 5 mine)
  mhgp7_gate(mhgp7_prefix_${fam}_${sz} 0 mhgp7_conformity
             ARGS --family=${fam} --n=${sz} --threads=${th} --smax=${PREFIX_SMAX} --prefix
                  --expected=${REFS}/${fam}_${sz}.txt
                  --min-orders=5 --min-deltas=${mind} --min-facets=${minf} --min-events=${mine}
             TIMEOUT 600)
endforeach()
foreach(spec "uniform;8000;223000;799000;301000"
             "uniform;16000;447000;1655000;622000"
             "uniform;32000;866000;3391000;1275000"
             "terrain;8000;61000;256000;92000"
             "terrain;16000;122000;522000;187000"
             "terrain;32000;238000;1048000;375000"
             "eight_clusters;8000;210000;712000;266000"
             "eight_clusters;16000;436000;1507000;565000"
             "eight_clusters;32000;899000;3153000;1183000"
             "scanline_single_pass;8000;29000;265000;99000"
             "scanline_single_pass;16000;55000;536000;201000"
             "scanline_single_pass;32000;105000;1066000;400000"
             "scanline_overlap_multiecho;8000;58000;373000;136000"
             "scanline_overlap_multiecho;16000;121000;764000;279000"
             "scanline_overlap_multiecho;32000;247000;1558000;570000")
  list(GET spec 0 fam)
  list(GET spec 1 sz)
  list(GET spec 2 mind)
  list(GET spec 3 minf)
  list(GET spec 4 mine)
  mhgp7_gate(mhgp7_prefix_${fam}_${sz} 0 mhgp7_conformity
             ARGS --family=${fam} --n=${sz} --threads=8 --smax=${PREFIX_SMAX} --prefix
                  --expected=${REFS}/${fam}_${sz}.txt
                  --min-orders=5 --min-deltas=${mind} --min-facets=${minf} --min-events=${mine}
             LABELS scale${sz} TIMEOUT 3600)
endforeach()
# TEMOIN DU MULTIENSEMBLE D'EVENEMENTS (--prefix-witness) : second run smax=11
# dans le meme processus, evenements TRIES compares ordre par ordre. Le seul
# observable qui voit prefix-tamper-event-order — l'echange interior[0]/[1] ne
# change NI les facettes (ensembles tries), NI le digest, NI une cardinalite :
# aucun plancher ni aucun digest ne peut le tuer, seule la signature canonique
# des evenements le peut. Sous --prefix seul il SURVIT (code 3) : c'est grave
# ci-dessous comme contre-fixture, pour que la porte ne mente pas sur sa portee.
mhgp7_gate(mhgp7_prefix_temoin_evenements 0 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix-witness
                --expected=${REFS}/uniform_400.txt
                --min-orders=5 --min-deltas=9200 --min-facets=30000 --min-events=11000 TIMEOUT 600)
mhgp7_gate(mhgp7_prefix_mutant_event_order 4 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix-witness
                --expected=${REFS}/uniform_400.txt --inject=prefix-tamper-event-order TIMEOUT 600)
mhgp7_gate(mhgp7_prefix_contre_event_order_sans_temoin 3 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix
                --expected=${REFS}/uniform_400.txt --inject=prefix-tamper-event-order TIMEOUT 600)
# prefix-tamper-batch-levels altere r.batch_levels APRES le digest de la foret :
# aucun digest ne le voit ; l'invariant de lot batch_levels.size() == batches,
# verifie au callback, le tue.
mhgp7_gate(mhgp7_prefix_mutant_batch_levels 4 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix
                --expected=${REFS}/uniform_400.txt --inject=prefix-tamper-batch-levels TIMEOUT 600)
# depth-threshold-minus-one exerce a smax=6 (il l'etait deja a smax=11) : le
# seuil h_q = smax + 1 - arite depend de smax, le defaut doit mourir aux deux.
mhgp7_gate(mhgp7_prefix_mutant_depth_smax6 4 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix
                --expected=${REFS}/uniform_400.txt --inject=depth-threshold-minus-one TIMEOUT 600)
# Refus du mode prefixe mal arme (jamais un vert par defaut).
mhgp7_gate(mhgp7_prefix_refus_smax11 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=11 --prefix --expected=${REFS}/uniform_400.txt)
mhgp7_gate(mhgp7_prefix_refus_sans_reference 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix)
# Reference AUSSI COURTE que le prefixe : ce n'est plus la lecture d'un prefixe,
# la porte se validerait elle-meme. Refus 2, jamais une comparaison.
mhgp7_gate(mhgp7_prefix_refus_reference_courte 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_400_cinq_ordres.txt)
# Plancher de couverture CABLE (contre-fixture : un plancher inatteignable rend 3).
mhgp7_gate(mhgp7_prefix_plancher_cable 3 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4 --smax=${PREFIX_SMAX} --prefix
                --expected=${REFS}/uniform_400.txt --min-orders=99)

# ---- PORTE DE RESIDENCE (paliers P2/P3) : jalons memoire et residence de
# l'etage de census, JAMAIS une preuve de correction. Label `gate;residence` :
# elle tourne avec la suite mais ne compte PAS parmi les portes de correction
# (`ctest -L residence` les isole ; `ctest -L gate -LE residence` compte les
# autres). Les six `rss_mb` sont des INSTANTANES et manquent structurellement
# tout pic ne et mort entre deux jalons ; `residence_hwm_mb` releve VmHWM (pic
# HISTORIQUE) aux MEMES frontieres, et `residence_increment_mb` en publie les
# increments — seule quantite imputable a un etage.
mhgp7_executable(mhgp7_residence_gate tests/residence_gate.cpp)
# PLANCHERS : uniquement les quantites DETERMINISTES (fonctions de la seule
# entree, insensibles a l'allocateur et a la machine), gravees a ~50 % du
# mesure. Les anciens planchers memoire (--min-hwm-mb, --min-ecart-fin-mb) sont
# RETIRES : MESURE du 2 septembre, `MALLOC_TRIM_THRESHOLD_` suffit a faire
# tomber l'ecart de fin a 0 sur du code sain (code 3 indiscernable d'une
# regression), et un plancher sur la memoire rougirait sur un palier reussi.
# PLAFOND : --max-coexistence-census-pct borne la coexistence MESUREE de la
# fusion du census (octets copies + octets encore detenus par les tranches non
# consommees, maximises sur les pas), en pour-cent de survivants x
# sizeof(BallData). Compteur DETERMINISTE : fonction de l'entree, invariant par
# nombre de fils, insensible a l'allocateur. C'est la garde du palier P3 et le
# tueur de `keep-ball-chunks`, et il va dans le SENS du chantier (il rougit
# quand la residence MONTE, la ou un plancher rougirait quand elle baisse).
# MESURE du 2 septembre (uniform n=2000, fils 1/4/8, valeurs identiques) :
# v7 : census direct sain 100 %, shadow conserve sous mutant 200 % ;
# plafond grave 110 % pour les deux profils. Un plafond sur le RSS
# ne separait PAS (increment de pic du census 201 Mo contre 213) : glibc ne rend
# une tranche liberee a l'OS qu'au-dela de son seuil dynamique de mmap.
# --min-jalons ferme le vert par vacuite du compte de jalons.
mhgp7_gate(mhgp7_residence_jalons 0 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11 --min-jalons=6
                --max-coexistence-census-pct=110
                --min-census-balls=337000 --min-plateau-balls=2600
                --min-sum-parents=668000 TIMEOUT 900)
# Mutant `hwm-instant-rss` : rend l'instantane a la place du pic — le defaut
# historique des six jalons. Tue par la SONDE auto-portee (mmap/touch/munmap :
# le noyau retire les pages de facon deterministe, l'allocateur n'a aucune
# autorite sur le verdict), et non par la retombee du RSS a la fin du fold.
mhgp7_gate(mhgp7_residence_mutant_hwm_instantane 4 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11 --min-jalons=6
                --max-coexistence-census-pct=110 --inject=hwm-instant-rss TIMEOUT 900)
# Mutant `drop-stage-milestone` : le jalon apres_rle n'est plus releve du tout.
# Sans --min-jalons la porte le sautait et restait VERTE en annoncant 6 jalons.
mhgp7_gate(mhgp7_residence_mutant_jalon_manquant 4 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11 --min-jalons=6
                --max-coexistence-census-pct=110 --inject=drop-stage-milestone TIMEOUT 900)
# Mutant `keep-ball-chunks` (palier P3) : les trois liberations par tranche
# retirees — la fusion reporte DEUX FOIS la residence de l'etage. Aucun digest,
# aucune cardinalite, aucun plancher ne peut le voir : seul ce plafond le tue.
mhgp7_gate(mhgp7_residence_mutant_tranches_gardees 4 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11 --min-jalons=6
                --max-coexistence-census-pct=110 --inject=keep-ball-chunks TIMEOUT 900)
# Contre-fixture du PLAFOND : cable, pas decoratif (un plafond inatteignable
# rend 3 sur un run sain).
mhgp7_gate(mhgp7_residence_plafond_cable 3 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11 --min-jalons=6
                --max-coexistence-census-pct=0 TIMEOUT 900)
# Contre-fixture du PLANCHER de jalons : cable de la meme facon.
mhgp7_gate(mhgp7_residence_plancher_cable 3 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=11
                --min-census-balls=999999999 TIMEOUT 900)
mhgp7_gate(mhgp7_residence_refus_smax 2 mhgp7_residence_gate LABELS "gate;residence" ARGS --smax=12)
# Jalons a smax=6 : le prefixe a sa propre residence (les deux paliers se
# mesurent sur la meme instrumentation).
mhgp7_gate(mhgp7_residence_jalons_prefixe 0 mhgp7_residence_gate LABELS "gate;residence"
           ARGS --family=uniform --n=2000 --threads=4 --smax=6 --min-jalons=6
                --max-coexistence-census-pct=110
                --min-census-balls=67000 --min-plateau-balls=1090
                --min-sum-parents=105000 TIMEOUT 900)

# ---- Barriere de sortie : linked_arcs_u16 (contrat sortie-sensible, barriere
# d'Edelsbrunner–Pach transposee en entiers u16 : arcs enlaces, q3 = 2n(n+1) et
# q4 = n² cles de profondeur zero ; oracle local OBig384 + route produit).
mhgp7_executable(mhgp7_linked_arcs tests/linked_arcs_gate.cpp)
mhgp7_gate(mhgp7_linked_arcs_u16 0 mhgp7_linked_arcs LABELS "gate;oracle")
mhgp7_gate(mhgp7_linked_arcs_oracle_i64 4 mhgp7_linked_arcs ARGS --oracle-i64)

# ---- Oracle BRUT de la grille de cellules (porte de la v5, quatrieme tour
# gcp/etat : parametre EXPLICITEMENT sur G=8 ET G=16 — evaluation directe
# i128 des (2G+1)^2 sommets, localisateur rationnel 256 bits, extremes u16,
# fixtures F9/F10/F11, sites synthetiques ~2^100 ; trois mutants tues).
mhgp7_executable(mhgp7_cell_grid_oracle tests/cell_grid_oracle.cpp)
mhgp7_gate(mhgp7_cell_grid_oracle 0 mhgp7_cell_grid_oracle LABELS "gate;oracle" TIMEOUT 900)
mhgp7_gate(mhgp7_cell_grid_oracle_mutant_ns 4 mhgp7_cell_grid_oracle ARGS --inject=cell-kill-nonstrict TIMEOUT 900)
mhgp7_gate(mhgp7_cell_grid_oracle_mutant_eps 4 mhgp7_cell_grid_oracle ARGS --inject=cell-locate-eps-zero TIMEOUT 900)
mhgp7_gate(mhgp7_cell_grid_oracle_mutant_h1 4 mhgp7_cell_grid_oracle ARGS --inject=cell-kill-h-minus-one TIMEOUT 900)

# ---- Fixtures de frontiere du sweep (audit du 31 aout : signes de B, B = 0,
# racines egales, extremite de Jung — egalite exacte 2P² = J·B² par le
# tetraedre regulier entier —, completion dans le facteur du rectangle,
# profondeur gravee h4 − 1 ; verite par objet contre oracle exhaustif).
mhgp7_executable(mhgp7_sweep_fixtures tests/sweep_fixtures.cpp)
mhgp7_gate(mhgp7_sweep_frontieres 0 mhgp7_sweep_fixtures)
mhgp7_gate(mhgp7_sweep_frontieres_mutant_depth 4 mhgp7_sweep_fixtures ARGS --inject=sweep-nonstrict-depth)
mhgp7_gate(mhgp7_sweep_frontieres_mutant_exit 4 mhgp7_sweep_fixtures ARGS --inject=sweep-drop-exit-root)

# ---- Coefficient de cover q4 (P0 audit du 31 aout : contre-fixture au
# tetraedre regulier + z interieur hors cover-3 ; le coefficient 4 tue a la
# generation, le mutant q4-cover-coef3 retablit le defaut herite v5).
mhgp7_executable(mhgp7_cover_fixture tests/cover_fixture.cpp)
mhgp7_gate(mhgp7_cover_coef4 0 mhgp7_cover_fixture)
mhgp7_gate(mhgp7_cover_coef3_mutant 4 mhgp7_cover_fixture ARGS --inject=q4-cover-coef3)

# ---- Golden post-prefiltre (non-regression interne v6, tag
# mhgp6-digest-v1:postprefilter-candidates) : grave apres le correctif cover.
mhgp7_gate(mhgp7_postprefilter_uniform_400 0 mhgp7_conformity
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
  mhgp7_gate(mhgp7_mutant_${mut} 4 mhgp7_conformity
             ARGS --family=eight_clusters --n=400 --threads=4 --inject=${mut}
                  --expected=${REFS}/eight_clusters_400.txt)
endforeach()

# ---- Contrats d'echec et validateurs (audit du 31 aout, quatrieme cycle).
# Le juge refuse un narrowing hors domaine et une reference tronquee ; le
# validateur de campagne est prouve fail-closed par sa porte Python ; la
# descente fusionnee tue wspd-drop-rect par son grand-livre (route produit).
mhgp7_gate(mhgp7_juge_refus_narrowing 2 mhgp7_conformity
           ARGS --family=uniform --n=4294967696 --threads=4 --expected=${REFS}/uniform_400.txt)
mhgp7_gate(mhgp7_juge_refus_reference_tronquee 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_400_tronque_k1_absent.txt)
# Cinquieme cycle : K1 correct + K10 EN TROP a n=2 (kmax_eff=1) — l'ensemble
# des clefs doit EGALER {1..kmax_eff}, une clef en trop est refusee (code 2),
# jamais silencieusement ignoree.
mhgp7_gate(mhgp7_juge_refus_k_en_trop 2 mhgp7_conformity
           ARGS --family=uniform --n=2 --threads=1
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_2_k10_en_trop.txt)
# Troisieme tour gcp : K11 SURNUMERAIRE (hors domaine global [1,10]) refuse
# par le chargeur lui-meme (code 2), jamais silencieusement ignore.
mhgp7_gate(mhgp7_juge_refus_k11 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --threads=4
                --expected=${CMAKE_CURRENT_SOURCE_DIR}/tests/refs/uniform_400_k11_en_trop.txt)
mhgp7_gate(mhgp7_cover_refus_mutant_hors_cible 2 mhgp7_cover_fixture ARGS --inject=rle-drop)
mhgp7_gate(mhgp7_sweep_refus_inject_vide 2 mhgp7_sweep_fixtures ARGS --inject=)
mhgp7_gate(mhgp7_fused_mutant_droprect 4 mhgp7_selftest ARGS --fused-descent --inject=wspd-drop-rect)
add_test(NAME mhgp7_pentes_validateur
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/pentes_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_pentes_validateur PROPERTIES LABELS "gate")
# Provenance d'executable des campagnes locales (alerte « campagne CPU
# mixte ») : copie privee immuable, hash avant/apres chaque tuple, INVALID
# sans DONE sur reconstruction concurrente simulee.
add_test(NAME mhgp7_campagne_provenance
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/campagne_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_campagne_provenance PROPERTIES LABELS "gate")
# Agregateur inter-graines PREENREGISTRE (regle figee avant la campagne
# jugee) : nominal + refus campagne invalide + activation par T_lourde.
add_test(NAME mhgp7_agregateur_intergraines
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/agregateur_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_agregateur_intergraines PROPERTIES LABELS "gate")

# ---- Cibles de PROFIL explicites + porte (§ 5.10) : l'identite de build est
# signee par la CIBLE (jamais par des macros de CMAKE_CXX_FLAGS) ; la porte
# atteste une PROJECTION DETERMINISTE NOMMEE (digest_all + digest_forest_K* +
# cardinalites K=) identique entre normal, profil, vivacite et join 0/1 —
# jamais « l'objet » complet — plus la structure (schemas exacts par K,
# planchers PAR K, fermeture somme/residuel aux bornes INTERNES du corps de
# reduce_fold, causalite de fold_join, discrimination des builds). Les
# intervalles rendent lisibles les recouvrements reduction/reduction et
# A/reduction. Le binaire de profil n'est JAMAIS un mur de debit.
mhgp7_product_executable(mhgp7_profile cli/mhgp7.cpp)
target_compile_definitions(mhgp7_profile PRIVATE MHGP7_PROFILE_REDUCE=1)
# Variante VIVACITE enregistree et exercee (contre-lecture 01bd14a9 : le
# garde de compilation seul ne couvre pas cette branche).
mhgp7_product_executable(mhgp7_profile_liveness cli/mhgp7.cpp)
target_compile_definitions(mhgp7_profile_liveness PRIVATE MHGP7_PROFILE_REDUCE=1 MHGP7_PROFILE_LIVENESS=1)
# SONDE D'ABLATION du reduce (2 septembre, arbre § 5.10) : binaire de PROFIL
# sous MHGP7_TESTING, accepte --inject=ablation-* (copie profonde retiree,
# tris retires, cle factice au remplissage). Chaque ablation change l'objet
# et est tuee (code 4) dans la boucle des mutants ; le binaire produit
# refuse --inject= (code 2). Jamais un mur : une attribution decomposee.
# ---- C6 jalon 2 : ENCODEUR PUR a offsets fixes (pack == append octet pour
# octet). Le format gpu_wire_v1 est inchange ; append_ball_in reste le chemin
# de production, la bascule est C6a.
mhgp7_executable(mhgp7_wire_pack_gate tests/wire_pack_gate.cpp)
mhgp7_gate(mhgp7_wire_pack 0 mhgp7_wire_pack_gate TIMEOUT 180)
mhgp7_gate(mhgp7_wire_pack_mutant_stride 4 mhgp7_wire_pack_gate ARGS --inject=wire-pack-stride-short TIMEOUT 180)
mhgp7_gate(mhgp7_wire_pack_mutant_slack 4 mhgp7_wire_pack_gate ARGS --inject=wire-pack-slack-size TIMEOUT 180)
# Contre-fixture de SELECTIVITE : deux injections a la fois ne prouvent plus
# quel defaut a ete vu (code 1, jamais un 4 aveugle).
mhgp7_gate(mhgp7_wire_pack_contre_double 1 mhgp7_wire_pack_gate
           ARGS "--inject=wire-pack-stride-short,wire-pack-slack-size" TIMEOUT 180)
mhgp7_gate(mhgp7_wire_pack_refus_arg 2 mhgp7_wire_pack_gate ARGS --min-balls=1x TIMEOUT 60)
mhgp7_gate(mhgp7_wire_pack_plancher 3 mhgp7_wire_pack_gate ARGS --min-tails=99999 TIMEOUT 180)
# ---- C6 jalons 1 et 3 : CONTRAT DES BAUX de l'anneau de lots (hote pur —
# entree/device/sortie a durees de vie SEPAREES, retraite ordonnee par
# base_global, granularite lot_ring_granularite_v1, regle d'erreur
# lot_ring_erreur_v1) juge sous un modele d'achevements DIFFERES pilote par le
# test : aucun CUDA, aucun fil, aucun sleep. tests/cuda_stub.hpp reste
# SEQUENTIEL et intact ; ce modele est un AUTO-TEST DU SCHEDULER, il ne prouve
# ni le device ni l'absence de course.
mhgp7_executable(mhgp7_lot_ring_gate tests/lot_ring_gate.cpp)
mhgp7_gate(mhgp7_lot_ring 0 mhgp7_lot_ring_gate
           ARGS --min-lots=30 --min-rotations=40 --min-tails=2 --min-inversions=36
                --min-erreurs=2 --min-blocages=4 --min-triples=2 TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_plancher 3 mhgp7_lot_ring_gate ARGS --min-tails=9 TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_refus_arg 2 mhgp7_lot_ring_gate ARGS --min-tails=2x TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_refus_mutant 2 mhgp7_lot_ring_gate ARGS --inject=c6-inconnu TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_mutant_reuse 4 mhgp7_lot_ring_gate ARGS --inject=c6-reuse-before-lease TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_mutant_merge 4 mhgp7_lot_ring_gate ARGS --inject=c6-merge-by-completion TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_mutant_epoch 4 mhgp7_lot_ring_gate ARGS --inject=c6-wrong-epoch TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_mutant_prefixe 4 mhgp7_lot_ring_gate ARGS --inject=c6-publish-prefix TIMEOUT 180)
mhgp7_gate(mhgp7_lot_ring_mutant_rebuild_tot 4 mhgp7_lot_ring_gate
           ARGS --inject=c6-rebuild-before-validate TIMEOUT 180)

# ---- P5 (docs/ECHELLE.md) : les DEUX GARDES DURES du fold, exercables a petit
# n par les crochets fold_events_cap_for_tests / fold_incidences_cap_for_tests
# (MHGP7_TESTING seulement). Le temoin non abaisse est l'anti-vacuite ; le refus
# doit nommer le plafond STRUCTUREL et l'ordre fautif, ne publier aucun callback
# et ne laisser aucun provisoire, aux trois ordonnancements.
mhgp7_executable(mhgp7_fold_caps_gate tests/fold_caps_gate.cpp)
mhgp7_gate(mhgp7_fold_cap_evenements 0 mhgp7_fold_caps_gate
           ARGS --garde=evenements --threads=4 --min-evenements=10000 --min-incidences=100000 TIMEOUT 300)
mhgp7_gate(mhgp7_fold_cap_incidences 0 mhgp7_fold_caps_gate
           ARGS --garde=incidences --threads=4 --min-evenements=10000 --min-incidences=100000 TIMEOUT 300)
mhgp7_gate(mhgp7_fold_cap_mutant_evenements 4 mhgp7_fold_caps_gate
           ARGS --garde=evenements --threads=4 --inject=caps-fold-guard-skip TIMEOUT 300)
mhgp7_gate(mhgp7_fold_cap_mutant_incidences 4 mhgp7_fold_caps_gate
           ARGS --garde=incidences --threads=4 --inject=caps-fold-guard-skip TIMEOUT 300)
mhgp7_gate(mhgp7_fold_cap_plancher 3 mhgp7_fold_caps_gate
           ARGS --garde=evenements --threads=4 --min-evenements=99999999 TIMEOUT 300)
mhgp7_gate(mhgp7_fold_cap_refus_arg 2 mhgp7_fold_caps_gate ARGS --garde=bidon TIMEOUT 60)

mhgp7_executable(mhgp7_profile_sonde cli/mhgp7.cpp)
target_compile_definitions(mhgp7_profile_sonde PRIVATE MHGP7_PROFILE_REDUCE=1)
mhgp7_gate(mhgp7_profile_refuse_inject 2 mhgp7_profile
           ARGS --family=uniform --n=400 --inject=ablation-mat-sans-copie)
mhgp7_gate(mhgp7_profile_sonde_refuse_inconnu 2 mhgp7_profile_sonde
           ARGS --family=uniform --n=400 --inject=ablation-inconnue)
# ALLOWLIST de la sonde (retour auditeur KeyCSR) : vide, item vide et mutant de
# production presents au registre sont refuses AVANT tout calcul.
mhgp7_gate(mhgp7_profile_sonde_refuse_inject_vide 2 mhgp7_profile_sonde ARGS --family=uniform --n=400 --inject=)
mhgp7_gate(mhgp7_profile_sonde_refuse_inject_virgule 2 mhgp7_profile_sonde ARGS --family=uniform --n=400 --inject=,)
mhgp7_gate(mhgp7_profile_sonde_refuse_mutant_production 2 mhgp7_profile_sonde
           ARGS --family=uniform --n=400 --inject=render-active-only)
add_test(NAME mhgp7_profil_identite
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/profil_gate.py $<TARGET_FILE:mhgp7> $<TARGET_FILE:mhgp7_profile> $<TARGET_FILE:mhgp7_profile_liveness>"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_profil_identite PROPERTIES LABELS "gate" TIMEOUT 600)
# CONTRE-FIXTURE AUTONOME des seuils (§ 5.13) : la scene « neuf composantes
# nulles, somme=0.008, residuel=0.012, mur=0.020 » doit etre tuee par les
# seuils serres 0.0051/0.006, et la scene aux arrondis %.3f honnetes rester
# verte — le juge exerce est le VRAI check_profile_output importe.
add_test(NAME mhgp7_profil_contre_fixture
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/profil_contre_fixture.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_profil_contre_fixture PROPERTIES LABELS "gate" TIMEOUT 120)
# PRE-INSCRIPTION KeyCSR : generateur de plan a graine derivee (bench/plan_keycsr.py),
# fixture des orientations et du flux SplitMix64, determinisme bit a bit.
add_test(NAME mhgp7_plan_keycsr_gate
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/plan_keycsr_gate.py"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_plan_keycsr_gate PROPERTIES LABELS "gate" TIMEOUT 120)
# PORTE DE VACUITE de la sonde d'ablation (ALERTE_SONDE_ABLATION_REDUCE du
# 2 septembre) : faux binaire rapide, plan de Williams, copie privee hashee
# avant/apres chaque tuple, agregateur et manifeste fatals, binaire produit
# refuse — le lanceur et l'agregateur de bench/ sont exerces tels quels.
add_test(NAME mhgp7_sonde_ablation_gate
         COMMAND ${CMAKE_COMMAND} -DEXPECTED=0 -DCMD=python3
                 "-DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/sonde_ablation_gate.py ${CMAKE_CURRENT_SOURCE_DIR}/bench"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_sonde_ablation_gate PROPERTIES LABELS "gate" TIMEOUT 300)
# Contrat d'echec sous profil, COMPILE (71528f8a : le CLI ne print jamais
# apres un refus — l'inspection porte sur le RunResult, jamais sur stdout).
add_executable(mhgp7_profil_contrat tests/profil_contrat_echec.cpp)
target_compile_definitions(mhgp7_profil_contrat PRIVATE MHGP7_PROFILE_REDUCE=1 MHGP7_TESTING=1)
target_link_libraries(mhgp7_profil_contrat PRIVATE Threads::Threads)
mhgp7_gate(mhgp7_profil_contrat_echec 0 mhgp7_profil_contrat TIMEOUT 600)
# Scene NON VACUEUSE (ac6b4bc1) : K1 publie puis panne A a K=2 — le refus
# budget minuscule precederait fold_profiles.assign et ne prouverait rien.
mhgp7_gate(mhgp7_profil_contrat_echec_k2 4 mhgp7_profil_contrat ARGS --inject=fold-inject-a-failure-k2 TIMEOUT 600)

# ---- CSR de FacetKey (GO exploratoire REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902) :
# fixtures gravees (born-only, parents-only, continuation, multi-parents, plusieurs
# racines par lot, foret vide, S1/S1a/S1b, R2/R2b), comparateur first_divergence
# (lecteur tiers, tests/forest_witness.hpp), rejeu catalogue+deltas -> partition
# (seconde autorite, deux layouts), matrice fils x inflight x join x layout,
# copie autonome, validateur d'offsets, gardes de capacite (append ET majorant).
mhgp7_executable(mhgp7_fold_csr_gate tests/fold_csr_gate.cpp)
mhgp7_gate(mhgp7_fold_csr_fixtures 0 mhgp7_fold_csr_gate
           ARGS --fixtures --min-fixtures=13 --min-deltas=25 --min-continuations=1 --min-multi-root-lots=3 --min-vides=1
                --min-refus=1 TIMEOUT 120)
mhgp7_gate(mhgp7_fold_csr_offsets 0 mhgp7_fold_csr_gate ARGS --offsets TIMEOUT 60)
mhgp7_gate(mhgp7_fold_csr_overflow 0 mhgp7_fold_csr_gate ARGS --overflow --threads=4 TIMEOUT 300)
mhgp7_gate(mhgp7_fold_csr_copie 0 mhgp7_fold_csr_gate ARGS --copy-alias --threads=4 TIMEOUT 300)
# Planchers a ~50 % de l'observe (matrice 7 familles, 112 cellules : 68 ordres
# publies, 818 069 deltas, 6 395 137 cles de reference ; uniform 400 seul en
# fournit 97 236 / 736 335 — les anciens 20000/20000 etaient vacus a 6 familles sur 7).
mhgp7_gate(mhgp7_fold_csr_pipeline 0 mhgp7_fold_csr_gate
           ARGS --pipeline --threads=8 --min-orders=60 --min-deltas=400000 --min-keys=3000000 TIMEOUT 1800)
# Identite de la matrice a une taille d'INTERET (label scale8000, jamais en gate).
# Planchers a ~50 % de l'observe (premier run : 16 cellules, 10 ordres,
# 2 791 148 deltas, 22 256 288 cles de reference, 0 desaccord, 1705 s).
mhgp7_gate(mhgp7_fold_csr_pipeline_uniform_8000 0 mhgp7_fold_csr_gate
           ARGS --pipeline --family=uniform --n=8000 --threads=8 --min-orders=10 --min-deltas=1400000 --min-keys=11000000
           LABELS scale8000 TIMEOUT 3600)
mhgp7_gate(mhgp7_fold_csr_refus_inject_vide 2 mhgp7_fold_csr_gate ARGS --fixtures --inject=)
mhgp7_gate(mhgp7_fold_csr_refus_mutant_inconnu 2 mhgp7_fold_csr_gate ARGS --fixtures --inject=csr-inconnu)
mhgp7_gate(mhgp7_fold_csr_refus_mutant_hors_cible 2 mhgp7_fold_csr_gate ARGS --fixtures --inject=drop-nonmerge)
mhgp7_gate(mhgp7_fold_csr_refus_pipeline_sans_inject 2 mhgp7_fold_csr_gate ARGS --pipeline-refus)
# Mutants csr-* : code 4 SEULEMENT si la premiere divergence (first_divergence)
# sur la fixture annoncee porte le champ annonce ; tout autre resultat = 1.
foreach(spec "csr-order-by-output;delta[0].output" "csr-keep-continuation;delta_count"
             "csr-stale-level;delta[2].level" "csr-stale-output;delta[0].output"
             "csr-unsorted-born;delta[0].born[0]" "csr-unsorted-parents;delta[2].parents[0]"
             "csr-drop-delta;delta_count" "csr-dup-delta;delta_count" "csr-shift-offset;delta[0].born.size"
             "csr-offset-hole;storage_violations" "csr-offset-overlap;storage_violations"
             "csr-offset-end;storage_violations" "csr-offset-domain;storage_violations")
  list(GET spec 0 mut)
  list(GET spec 1 field)
  mhgp7_gate(mhgp7_fold_csr_mutant_${mut} 4 mhgp7_fold_csr_gate
             ARGS --fixtures --inject=${mut} --expect-divergence=${field} TIMEOUT 120)
endforeach()
foreach(mut csr-offset-hole csr-offset-overlap csr-offset-end csr-offset-domain)
  mhgp7_gate(mhgp7_fold_csr_refus_${mut} 4 mhgp7_fold_csr_gate ARGS --pipeline-refus --threads=4 --inject=${mut} TIMEOUT 300)
endforeach()
mhgp7_gate(mhgp7_fold_csr_mutant_csr-guard-skip 4 mhgp7_fold_csr_gate
           ARGS --overflow --threads=4 --inject=csr-guard-skip --expect-divergence=storage_message TIMEOUT 300)
# Panne d'allocation injectee (bad_alloc d'arene apres un append partiel) :
# capturee dans le fold -> resource_exhausted, payload vide, zero callback,
# provisoires vides ; jamais une exception hors de reduce_fold.
mhgp7_gate(mhgp7_fold_csr_mutant_csr-inject-bad-alloc 4 mhgp7_fold_csr_gate
           ARGS --overflow --threads=4 --inject=csr-inject-bad-alloc --expect-divergence=storage_message TIMEOUT 300)
# Conformite v5 sous --layout=csr : MEMES recus (l'objet est le meme), petites
# tailles en gate, tailles d'interet en scale* ; refus 2 d'un layout inconnu.
foreach(spec "uniform;400" "terrain;400" "eight_clusters;400" "scanline_single_pass;400"
             "collinear_seven;600" "two_lines;200" "uniform;2000" "terrain;2000")
  list(GET spec 0 fam)
  list(GET spec 1 sz)
  mhgp7_gate(mhgp7_conformity_csr_${fam}_${sz} 0 mhgp7_conformity
             ARGS --family=${fam} --n=${sz} --threads=4 --layout=csr --expected=${REFS}/${fam}_${sz}.txt TIMEOUT 600)
endforeach()
foreach(fam uniform terrain eight_clusters scanline_single_pass scanline_overlap_multiecho)
  foreach(sz 8000 16000 32000)
    mhgp7_gate(mhgp7_conformity_csr_${fam}_${sz} 0 mhgp7_conformity
               ARGS --family=${fam} --n=${sz} --threads=8 --layout=csr --expected=${REFS}/${fam}_${sz}.txt
               LABELS scale${sz} TIMEOUT 3600)
  endforeach()
endforeach()
mhgp7_gate(mhgp7_conformity_refus_layout_inconnu 2 mhgp7_conformity
           ARGS --family=uniform --n=400 --layout=fid --expected=${REFS}/uniform_400.txt)
# Boucle de mise a mort sous csr : les csr-* ne sont exerces que si le juge
# tourne en csr ; les sondes d'ablation et les mutants du fold y restent tues.
# Seuls les mutants qui CHANGENT L'OBJET y figurent (divergence de digest sur
# le recu) : csr-offset-* en sont EXCLUS — sous la conformite, tout statut non
# complet vaut 4 par vacuite de refus, ce qui ne prouverait rien ; leur preuve
# (refus AVANT vue, zero callback, provisoires vides) est portee par
# mhgp7_fold_csr_refus_csr-offset-* et mhgp7_fold_csr_mutant_csr-offset-*.
# csr-guard-skip et csr-inject-bad-alloc exclus de meme (portes --overflow).
foreach(mut csr-order-by-output csr-keep-continuation csr-stale-level csr-stale-output
            csr-unsorted-born csr-unsorted-parents csr-drop-delta csr-dup-delta csr-shift-offset
            ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice
            binary-ties repr-ties attach-prebatch drop-nonmerge canonical-is-uf-root)
  mhgp7_gate(mhgp7_mutant_csr_${mut} 4 mhgp7_conformity
             ARGS --family=eight_clusters --n=400 --threads=4 --layout=csr --inject=${mut}
                  --expected=${REFS}/eight_clusters_400.txt)
endforeach()
# Signature CLI des deux routes (code 0 ET ligne exacte en UNE execution) ; refus
# exact d'un layout inconnu ou vide (option PRODUIT, hors MHGP7_TESTING).
mhgp7_gate(mhgp7_cli_layout_classic_signature 0 mhgp7 ARGS --family=uniform --n=200 --s=8 TIMEOUT 120
  EXPECT_LINE "forest_layout=classic forest_storage_kind=vector_component_delta_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10")
mhgp7_gate(mhgp7_cli_layout_csr_signature 0 mhgp7 ARGS --family=uniform --n=200 --s=8 --layout=csr TIMEOUT 120
  EXPECT_LINE "forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10")
mhgp7_gate(mhgp7_cli_refus_layout_inconnu 2 mhgp7 ARGS --family=uniform --n=200 --layout=fid)
mhgp7_gate(mhgp7_cli_refus_layout_vide 2 mhgp7 ARGS --family=uniform --n=200 --layout=)

# ---- v7: close unregistered v6 WIP before qualifying the port.
mhgp7_executable(mhgp7_perm_sort_gate tests/perm_sort_gate.cpp)
mhgp7_executable(mhgp7_perm_residence_gate tests/perm_residence_gate.cpp)
mhgp7_gate(mhgp7_perm_residence 0 mhgp7_perm_residence_gate TIMEOUT 120)
foreach(mut perm-apply-scatter perm-apply-partial perm-tie-desc)
  mhgp7_gate(mhgp7_perm_residence_${mut} 4 mhgp7_perm_residence_gate
             ARGS --inject=${mut} TIMEOUT 120)
endforeach()
mhgp7_gate(mhgp7_perm_sort 0 mhgp7_perm_sort_gate TIMEOUT 600)
mhgp7_gate(mhgp7_perm_sort_plancher 3 mhgp7_perm_sort_gate
           ARGS --min-candidats=999999999 TIMEOUT 600)
mhgp7_gate(mhgp7_perm_sort_refus_arg 2 mhgp7_perm_sort_gate ARGS --min-candidats=1x)
foreach(mut perm-apply-scatter perm-apply-partial perm-tie-desc census-stack-per-ball parallel-sort-unstable)
  mhgp7_gate(mhgp7_perm_sort_mutant_${mut} 4 mhgp7_perm_sort_gate
             ARGS --inject=${mut} TIMEOUT 600)
endforeach()

# C6a is a CPU stub witness only. It carries no CUDA or overlap claim.
mhgp7_executable(mhgp7_route_c6_gate tests/route_c6_gate.cpp)
mhgp7_gate(mhgp7_route_c6_stub 0 mhgp7_route_c6_gate TIMEOUT 600)
mhgp7_gate(mhgp7_route_c6_stub_plancher 3 mhgp7_route_c6_gate
           ARGS --min-scenes=999999 TIMEOUT 600)
mhgp7_gate(mhgp7_route_c6_stub_refus_arg 2 mhgp7_route_c6_gate ARGS --min-scenes=1x)
foreach(mut gpu-lot-base-reset c6-wrong-epoch c6-reuse-before-lease c6-publish-prefix
            gpu-stack-shallow gpu-census-nonstrict gpu-skip-ball-write gpu-nshell-overdomain
            gpu-skip-count-write)
  mhgp7_gate(mhgp7_route_c6_stub_mutant_${mut} 4 mhgp7_route_c6_gate
             ARGS --inject=${mut} TIMEOUT 600)
endforeach()

mhgp7_executable(mhgp7_thread_failure_gate tests/thread_failure_gate.cpp)
mhgp7_gate(mhgp7_thread_failure 0 mhgp7_thread_failure_gate TIMEOUT 30)
mhgp7_gate(mhgp7_thread_failure_mutant_admission 4 mhgp7_thread_failure_gate
           ARGS --inject=parallel-admit-partial-launch TIMEOUT 30)
mhgp7_gate(mhgp7_thread_failure_refus_arg 2 mhgp7_thread_failure_gate ARGS --unknown TIMEOUT 30)

mhgp7_executable(mhgp7_census_direct_gate tests/census_direct_gate.cpp)
mhgp7_gate(mhgp7_census_direct 0 mhgp7_census_direct_gate TIMEOUT 120)
mhgp7_gate(mhgp7_census_direct_offset 4 mhgp7_census_direct_gate ARGS --inject=census-direct-offset TIMEOUT 120)
mhgp7_gate(mhgp7_census_direct_transaction 4 mhgp7_census_direct_gate
           ARGS --inject=census-direct-publish-prefix TIMEOUT 120)
mhgp7_gate(mhgp7_census_direct_residence 4 mhgp7_census_direct_gate ARGS --inject=keep-ball-chunks TIMEOUT 120)

mhgp7_executable(mhgp7_silent_incidence_gate tests/silent_incidence_gate.cpp)
mhgp7_gate(mhgp7_silent_incidence 0 mhgp7_silent_incidence_gate TIMEOUT 120)
mhgp7_gate(mhgp7_silent_incidence_mutant 4 mhgp7_silent_incidence_gate
           ARGS --mutant=silent-drop-coface TIMEOUT 120)
mhgp7_gate(mhgp7_reduced_latent_parent_mutant 4 mhgp7_silent_incidence_gate
           ARGS --mutant=reduced-latent-parent TIMEOUT 120)
mhgp7_gate(mhgp7_reduced_drop_materialization_mutant 4 mhgp7_silent_incidence_gate
           ARGS --mutant=reduced-drop-materialization TIMEOUT 120)
mhgp7_gate(mhgp7_silent_drop_nonmerge_mutant 4 mhgp7_silent_incidence_gate
           ARGS --mutant=drop-nonmerge TIMEOUT 120)
mhgp7_gate(mhgp7_silent_csr_stale_level_mutant 4 mhgp7_silent_incidence_gate
           ARGS --mutant=csr-stale-level TIMEOUT 120)

add_test(NAME mhgp7_archive
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/archive_gate.py --binary $<TARGET_FILE:mhgp7>)
add_test(NAME mhgp7_archive_optimized
         COMMAND python3 -O ${CMAKE_CURRENT_SOURCE_DIR}/tests/archive_gate.py --binary $<TARGET_FILE:mhgp7>)
set_tests_properties(mhgp7_archive mhgp7_archive_optimized PROPERTIES LABELS gate TIMEOUT 120)
add_test(NAME mhgp7_compare_campaign
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_campaign_gate.py)
add_test(NAME mhgp7_compare_campaign_optimized
         COMMAND python3 -O ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_campaign_gate.py)
set_tests_properties(mhgp7_compare_campaign mhgp7_compare_campaign_optimized PROPERTIES LABELS gate TIMEOUT 120)
mhgp7_executable(mhgp7_archive_api_gate tests/archive_api_gate.cpp)
target_link_options(mhgp7_archive_api_gate PRIVATE -Wl,--wrap=fsync)
mhgp7_gate(mhgp7_archive_api 0 mhgp7_archive_api_gate TIMEOUT 120)
mhgp7_executable(mhgp7_archive_cleanup_gate tests/archive_cleanup_gate.cpp)
target_link_options(mhgp7_archive_cleanup_gate PRIVATE -Wl,--wrap=openat -Wl,--wrap=fsync)
mhgp7_gate(mhgp7_archive_cleanup 0 mhgp7_archive_cleanup_gate TIMEOUT 120 EXPECT_PREFIX "archive_cleanup_gate=passed persistent=1 all_order_files=10 ")
add_test(NAME mhgp7_incidence_campaign
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/incidence_campaign_gate.py)
add_test(NAME mhgp7_incidence_campaign_optimized
         COMMAND python3 -O ${CMAKE_CURRENT_SOURCE_DIR}/tests/incidence_campaign_gate.py)
set_tests_properties(mhgp7_incidence_campaign mhgp7_incidence_campaign_optimized
                     PROPERTIES LABELS gate TIMEOUT 120)
add_test(NAME mhgp7_expect_line
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_line_gate.py
                 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp7_expect_line PROPERTIES LABELS gate TIMEOUT 30)

# Vrai mono : interposition reelle pthread_create, controles positifs du
# chemin asynchrone et quatre tours 1..5/1..10 dans les deux semantiques.
mhgp7_executable(mhgp7_mono_inline_gate tests/mono_inline_gate.cpp)
target_link_libraries(mhgp7_mono_inline_gate PRIVATE ${CMAKE_DL_LIBS})
mhgp7_gate(mhgp7_mono_inline 0 mhgp7_mono_inline_gate TIMEOUT 120
           EXPECT_PREFIX "mono_inline_gate=passed successful=4 rejected=8 reference_threads=30 failures=0")
mhgp7_gate(mhgp7_mono_inline_late_a 0 mhgp7_mono_inline_gate TIMEOUT 120
           ARGS --inject=fold-inject-a-failure-k2 EXPECT_PREFIX "mono_inline_injected=passed cases=2 failures=0")
mhgp7_gate(mhgp7_mono_inline_late_b 0 mhgp7_mono_inline_gate TIMEOUT 120
           ARGS --inject=fold-inject-b-exception-k3 EXPECT_PREFIX "mono_inline_injected=passed cases=2 failures=0")
mhgp7_gate(mhgp7_mono_inline_early_alloc 0 mhgp7_mono_inline_gate TIMEOUT 120
           ARGS --inject=caps-throw-bad-alloc-fold EXPECT_PREFIX "mono_inline_injected=passed cases=2 failures=0")

# Census : argmin entier global puis clip par boite, oracle OBig exhaustif
# borne et mutants a divergence explicite (pas n'importe quel echec).
mhgp7_executable(mhgp7_axis_bounds_gate tests/axis_bounds_gate.cpp)
mhgp7_gate(mhgp7_axis_bounds 0 mhgp7_axis_bounds_gate TIMEOUT 30
           EXPECT_PREFIX "OK axis_bounds boxes=")
foreach(mut IN ITEMS axis-argmin-floor-only axis-argmin-ceil-always axis-argmin-no-clip
                     axis-argmin-narrow-coefficient axis-argmin-max-min)
  mhgp7_gate(mhgp7_axis_bounds_${mut} 4 mhgp7_axis_bounds_gate TIMEOUT 30
             ARGS ${mut} EXPECT_PREFIX "DIVERGENCE axis_bounds")
endforeach()
# Mesure synthetique facultative : hors ALL, sans mutant et sans seuil CTest.
add_executable(mhgp7_axis_bounds_microbench EXCLUDE_FROM_ALL bench/axis_bounds_microbench.cpp)
target_link_libraries(mhgp7_axis_bounds_microbench PRIVATE Threads::Threads)

# ---- CUDA (serie C, docs/GPU.md) : OFF par defaut, JAMAIS construit en CI.
# Les portes `gpu` ne s'executent qu'en session G4 (sm_120, RTX PRO 6000
# Blackwell). -fmad=false : aucune contraction implicite ; le socle serie C
# est 100 % entier, le flottant n'y entre pas. Incompatible avec tout futur
# profil sanitizers (motif v5 : refus explicite si les deux sont demandes).
option(MHGP7_ENABLE_CUDA "Construire les cibles device de la serie C (G4 seulement)" OFF)
if(MHGP7_ENABLE_CUDA)
  if(CMAKE_CXX_FLAGS MATCHES "fsanitize")
    message(FATAL_ERROR "MHGP7_ENABLE_CUDA est incompatible avec des flags sanitizers")
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
    message(FATAL_ERROR "MHGP7_ENABLE_CUDA exige CMAKE_CUDA_ARCHITECTURES=120 (recu ${CMAKE_CUDA_ARCHITECTURES})")
  endif()
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  add_executable(mhgp7_device_witness src/gpu/device_witness.cu)
  target_compile_options(mhgp7_device_witness PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp7_device_witness PRIVATE MHGP7_TESTING=1)
  target_link_libraries(mhgp7_device_witness PRIVATE Threads::Threads)
  mhgp7_gate(mhgp7_device_witness 0 mhgp7_device_witness LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_device_witness_mutant_carry 4 mhgp7_device_witness ARGS --inject=witness-di128-lost-carry LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_device_witness_mutant_skip_write 4 mhgp7_device_witness ARGS --inject=witness-skip-write LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_device_witness_mutant_skip_native 4 mhgp7_device_witness ARGS --inject=witness-skip-native-write LABELS gpu TIMEOUT 600)
  # C3/C4 DEVICE : memes exigences que la porte stub, kernels sur le device
  # reel ; l'architecture compilee est SIGNEE dans le binaire (§ 5.8 : le
  # recu certifie arch compilee ET device observe).
  add_executable(mhgp7_census_device_gate tests/census_device_gate.cu)
  target_compile_options(mhgp7_census_device_gate PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp7_census_device_gate PRIVATE MHGP7_TESTING=1
    MHGP7_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp7_census_device_gate PRIVATE Threads::Threads)
  mhgp7_gate(mhgp7_census_device 0 mhgp7_census_device_gate LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_range_le 4 mhgp7_census_device_gate ARGS --inject=gpu-range-add-le LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_stack 4 mhgp7_census_device_gate ARGS --inject=gpu-stack-shallow LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_swap 4 mhgp7_census_device_gate ARGS --inject=gpu-swap-push-order LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_nonstrict 4 mhgp7_census_device_gate ARGS --inject=gpu-census-nonstrict LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_skip_write 4 mhgp7_census_device_gate ARGS --inject=gpu-skip-ball-write LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_nshell 4 mhgp7_census_device_gate ARGS --inject=gpu-nshell-overdomain LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_census_device_mutant_skip_count 4 mhgp7_census_device_gate ARGS --inject=gpu-skip-count-write LABELS gpu TIMEOUT 600)
  # C5 : PILOTE serie C — les deux routes sur le meme nuage, parite de TOUS
  # les digests exigee, couts wire/H2D/kernels/D2H separes (§ 5.5). La porte
  # 400 est la verification fonctionnelle ; les mesures 50k+ viennent du
  # profil de campagne, jamais d'un gate.
  add_executable(mhgp7_cuda cli/mhgp7_cuda.cu)
  target_compile_options(mhgp7_cuda PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp7_cuda PRIVATE MHGP7_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp7_cuda PRIVATE Threads::Threads)
  mhgp7_gate(mhgp7_pilote_parite_400 0 mhgp7_cuda ARGS --family=uniform --n=400 --seed=3 --threads=8 LABELS gpu TIMEOUT 600)
  mhgp7_gate(mhgp7_pilote_refus_n 2 mhgp7_cuda ARGS --n=200x LABELS gpu TIMEOUT 120)
  mhgp7_gate(mhgp7_pilote_lot17 0 mhgp7_cuda ARGS --family=uniform --n=400 --seed=3 --threads=8 --lot=17 --min-lots=2 LABELS gpu TIMEOUT 600)
  add_executable(mhgp7_cuda_test cli/mhgp7_cuda.cu)
  target_compile_options(mhgp7_cuda_test PRIVATE
    "$<$<COMPILE_LANGUAGE:CUDA>:-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror>")
  target_compile_definitions(mhgp7_cuda_test PRIVATE MHGP7_TESTING=1 MHGP7_CUDA_ARCHS="${CMAKE_CUDA_ARCHITECTURES}")
  target_link_libraries(mhgp7_cuda_test PRIVATE Threads::Threads)
  mhgp7_gate(mhgp7_pilote_mutant_base 4 mhgp7_cuda_test ARGS --family=uniform --n=400 --seed=3 --threads=8 --lot=17 --min-lots=2 --inject=gpu-lot-base-reset LABELS gpu TIMEOUT 600)
endif()

# Two bounded primitive gates, never a pipeline or performance qualification.
mhgp7_executable(mhgp7_arithmetic_integer_gate tests/arithmetic_integer_gate.cpp)
target_include_directories(mhgp7_arithmetic_integer_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
mhgp7_gate(mhgp7_arithmetic_integer 0 mhgp7_arithmetic_integer_gate
  EXPECT_PREFIX "arithmetic_integer_gate=passed case=all mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_u320 0 mhgp7_arithmetic_integer_gate
  ARGS --case=u320-only
  EXPECT_PREFIX "arithmetic_integer_gate=passed case=u320-only mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_oracle 0 mhgp7_arithmetic_integer_gate
  ARGS --case=oracle-only
  EXPECT_PREFIX "arithmetic_integer_gate=passed case=oracle-only mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_u320_word4 4 mhgp7_arithmetic_integer_gate
  ARGS --case=u320-only --mutant=level-trunc-hi
  EXPECT_PREFIX "arithmetic_integer_gate=mutant_killed case=u320-only mutant=level-trunc-hi divergence=W2.u320_word4" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_oracle_carry 4 mhgp7_arithmetic_integer_gate
  ARGS --case=oracle-only --mutant=obig-carry-lost
  EXPECT_PREFIX "arithmetic_integer_gate=mutant_killed case=oracle-only mutant=obig-carry-lost divergence=oracle.W2_carry" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_bad_case 2 mhgp7_arithmetic_integer_gate
  ARGS --case=unknown TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_bad_mutant 2 mhgp7_arithmetic_integer_gate
  ARGS --case=u320-only --mutant=missing TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_integer_case_mismatch 2 mhgp7_arithmetic_integer_gate
  ARGS --case=all --mutant=level-trunc-hi TIMEOUT 30)
mhgp7_executable(mhgp7_arithmetic_lanes_gate tests/arithmetic_lanes_gate.cpp)
target_include_directories(mhgp7_arithmetic_lanes_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
mhgp7_gate(mhgp7_arithmetic_lanes 0 mhgp7_arithmetic_lanes_gate
  EXPECT_PREFIX "arithmetic_lanes_gate PASS" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_q4_level 0 mhgp7_arithmetic_lanes_gate
  ARGS --case=q4-level-only
  EXPECT_PREFIX "arithmetic_lanes_gate PASS" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_oracle 0 mhgp7_arithmetic_lanes_gate
  ARGS --case=oracle-only
  EXPECT_PREFIX "arithmetic_lanes_gate PASS" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_oracle_overflow 3 mhgp7_arithmetic_lanes_gate
  ARGS --case=oracle-overflow
  EXPECT_PREFIX "first_divergence=oracle.overflow inject=" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_q3_factor 4 mhgp7_arithmetic_lanes_gate
  ARGS --inject=q3-level-4g
  EXPECT_PREFIX "first_divergence=q3.level inject=q3-level-4g" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_q4_parity 4 mhgp7_arithmetic_lanes_gate
  ARGS --inject=q4-center-parity
  EXPECT_PREFIX "first_divergence=q4.center inject=q4-center-parity" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_q4_face_boundary 4 mhgp7_arithmetic_lanes_gate
  ARGS --inject=q4-eq-nonstrict
  EXPECT_PREFIX "first_divergence=q4.face_power inject=q4-eq-nonstrict" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_q4_u192_word2 4 mhgp7_arithmetic_lanes_gate
  ARGS --case=q4-level-only --inject=level-trunc-hi
  EXPECT_PREFIX "first_divergence=q4.level.num inject=level-trunc-hi" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_oracle_carry 4 mhgp7_arithmetic_lanes_gate
  ARGS --case=oracle-only --inject=obig-carry-lost
  EXPECT_PREFIX "first_divergence=oracle.literal-carry inject=obig-carry-lost" TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_bad_case 2 mhgp7_arithmetic_lanes_gate
  ARGS --case=unknown TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_bad_inject 2 mhgp7_arithmetic_lanes_gate
  ARGS --inject=missing TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_duplicate_case 2 mhgp7_arithmetic_lanes_gate
  ARGS --case=all --case=all TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_duplicate_inject 2 mhgp7_arithmetic_lanes_gate
  ARGS --inject=q3-level-4g --inject=q3-level-4g TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_empty_inject 2 mhgp7_arithmetic_lanes_gate
  ARGS --inject= TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_u192_case_mismatch 2 mhgp7_arithmetic_lanes_gate
  ARGS --case=all --inject=level-trunc-hi TIMEOUT 30)
mhgp7_gate(mhgp7_arithmetic_lanes_oracle_case_mismatch 2 mhgp7_arithmetic_lanes_gate
  ARGS --case=all --inject=obig-carry-lost TIMEOUT 30)

# Exact relative containment before local MEB materialization, test-only cost witness.
mhgp7_executable(mhgp7_meb_lazy_gate tests/meb_lazy_gate.cpp)
target_include_directories(mhgp7_meb_lazy_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
mhgp7_gate(mhgp7_meb_lazy 0 mhgp7_meb_lazy_gate
  EXPECT_PREFIX "meb_lazy_gate=passed mutant=none divergence=none" TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_q2_reject_shell 4 mhgp7_meb_lazy_gate
  ARGS --mutant=silent-meb-q2-reject-shell
  EXPECT_PREFIX "meb_lazy_gate=mutant_killed mutant=silent-meb-q2-reject-shell divergence=differential.status_reason" TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_q3_reject_shell 4 mhgp7_meb_lazy_gate
  ARGS --mutant=silent-meb-q3-reject-shell
  EXPECT_PREFIX "meb_lazy_gate=mutant_killed mutant=silent-meb-q3-reject-shell divergence=differential.status_reason" TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_q4_reject_shell 4 mhgp7_meb_lazy_gate
  ARGS --mutant=silent-meb-q4-reject-shell
  EXPECT_PREFIX "meb_lazy_gate=mutant_killed mutant=silent-meb-q4-reject-shell divergence=differential.status_reason" TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_eager_materialization 4 mhgp7_meb_lazy_gate
  ARGS --mutant=silent-meb-eager-materialization
  EXPECT_PREFIX "meb_lazy_gate=mutant_killed mutant=silent-meb-eager-materialization divergence=logical_rejected_materialization" TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_unknown_mutant 2 mhgp7_meb_lazy_gate
  ARGS --mutant=unknown TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_empty_mutant 2 mhgp7_meb_lazy_gate
  ARGS --mutant= TIMEOUT 30)
mhgp7_gate(mhgp7_meb_lazy_duplicate_mutant 2 mhgp7_meb_lazy_gate
  ARGS --mutant=silent-meb-q3-reject-shell --mutant=silent-meb-q3-reject-shell TIMEOUT 30)

# F: same witness counts, DFS nodes/corners, inline stack and safe overflow.
# The reference is an explicitly adapted E count-path snapshot, not an oracle.
mhgp7_executable(mhgp7_witness_stack_gate tests/witness_stack_gate.cpp)
mhgp7_gate(mhgp7_witness_stack 0 mhgp7_witness_stack_gate
  EXPECT_PREFIX "witness_stack_gate=passed case=all reason=none mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_helper 0 mhgp7_witness_stack_gate
  ARGS --case=helper
  EXPECT_PREFIX "witness_stack_gate=passed case=helper reason=none mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_witness 0 mhgp7_witness_stack_gate
  ARGS --case=witness
  EXPECT_PREFIX "witness_stack_gate=passed case=witness reason=none mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_lane_mask 0 mhgp7_witness_stack_gate
  ARGS --case=lane-mask
  EXPECT_PREFIX "witness_stack_gate=passed case=lane-mask reason=none mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_lane_mask_mutant 4 mhgp7_witness_stack_gate
  ARGS --case=lane-mask --mutant=witness-no-lane-mask
  EXPECT_PREFIX "witness_stack_gate=mutant_killed case=lane-mask reason=witness.lane_mask_double_credit mutant=witness-no-lane-mask" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_unknown_case 2 mhgp7_witness_stack_gate
  ARGS --case=unknown TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_empty_case 2 mhgp7_witness_stack_gate
  ARGS --case= TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_duplicate_case 2 mhgp7_witness_stack_gate
  ARGS --case=all --case=all TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_unknown_argument 2 mhgp7_witness_stack_gate
  ARGS --unknown TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_unknown_mutant 2 mhgp7_witness_stack_gate
  ARGS --case=lane-mask --mutant=missing TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_empty_mutant 2 mhgp7_witness_stack_gate
  ARGS --case=lane-mask --mutant= TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_duplicate_mutant 2 mhgp7_witness_stack_gate
  ARGS --case=lane-mask --mutant=witness-no-lane-mask --mutant=witness-no-lane-mask TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_mutant_case_mismatch 2 mhgp7_witness_stack_gate
  ARGS --case=witness --mutant=witness-no-lane-mask TIMEOUT 30)
# Native sanitizer companion: no replaceable new/delete interception. It
# qualifies semantics, never zero allocations or injected allocation failure.
mhgp7_executable(mhgp7_witness_stack_semantic_gate tests/witness_stack_gate.cpp)
target_compile_definitions(mhgp7_witness_stack_semantic_gate PRIVATE MHGP7_WITNESS_STACK_NO_ALLOC_OBSERVER=1)
mhgp7_gate(mhgp7_witness_stack_semantic 0 mhgp7_witness_stack_semantic_gate
  ARGS --case=witness
  EXPECT_PREFIX "witness_stack_gate=passed case=witness reason=none mutant=none" TIMEOUT 30)
mhgp7_gate(mhgp7_witness_stack_semantic_lane_mask_mutant 4 mhgp7_witness_stack_semantic_gate
  ARGS --case=lane-mask --mutant=witness-no-lane-mask
  EXPECT_PREFIX "witness_stack_gate=mutant_killed case=lane-mask reason=witness.lane_mask_double_credit mutant=witness-no-lane-mask" TIMEOUT 30)
