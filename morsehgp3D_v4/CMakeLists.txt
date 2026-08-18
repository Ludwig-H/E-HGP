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

# Fixture bloquante avant q4, RENFORCEE (audit bc5b05d § 2 puis audit apres
# f6b29e1) : 22 points — les quatre faces du tetraedre sont q3-profondes,
# les deux ancres maximales ab/xy sont q3-mortes mais q4-vivantes, et le
# tetraedre q4 garde profondeur zero. Une source q4 branchee sur les ancres
# q3 vivantes OU sur le flux des evenements q3 le perd : les deux mutants
# gravent le decouplage complet des lanes.
add_executable(mhgp4_q4_source_fixture tests/q4_source_fixture_test.cpp)
mhgp4_expect_code(mhgp4_q4_source_independante 0 mhgp4_q4_source_fixture "")
mhgp4_expect_code(mhgp4_q4_source_mutant 4 mhgp4_q4_source_fixture
    "--inject=q4-seeds-from-q3-live")
mhgp4_expect_code(mhgp4_q4_source_mutant_events 4 mhgp4_q4_source_fixture
    "--inject=q4-seeds-from-q3-events")

# La lane q4 productrice (MATHEMATIQUES.md § 4.5) : juge brut C(n,4) sur
# records complets, fixture 13+1 points bout en bout (ancre q3-morte/
# q4-vivante, tetraedre profondeur 1 dont l'interieur n'est visible que du
# cover coefficient 4), mutants du branchement interdit / du coefficient /
# de l'exact-once.
add_executable(mhgp4_q4_events_probe bench/q4_events_probe.cpp)
mhgp4_expect_code(mhgp4_q4_fixture_bout_en_bout 0 mhgp4_q4_events_probe
    "--fixture --s=8 --judge --min-events=1")
mhgp4_expect_code(mhgp4_q4_events_judge_uniform 0 mhgp4_q4_events_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --min-events=5")
mhgp4_expect_code(mhgp4_q4_events_judge_clusters 0 mhgp4_q4_events_probe
    "--family=eight_clusters --n=120 --s=8 --seed=3 --judge --min-events=5")
mhgp4_expect_code(mhgp4_q4_mutant_source_q3 4 mhgp4_q4_events_probe
    "--fixture --s=8 --inject=seeds-from-q3-live")
mhgp4_expect_code(mhgp4_q4_mutant_source_events 4 mhgp4_q4_events_probe
    "--fixture --s=8 --inject=seeds-from-q3-events")
mhgp4_expect_code(mhgp4_q4_mutant_coef3 4 mhgp4_q4_events_probe
    "--fixture --s=8 --inject=cover-coef3")
mhgp4_expect_code(mhgp4_q4_mutant_no_canonical 4 mhgp4_q4_events_probe
    "--family=uniform --n=120 --s=8 --seed=3 --inject=no-canonical")

# La foret HGP (MATHEMATIQUES.md § 5) : union-find a macro-lots
# `same_exact_level`, juge par miniboule INDEPENDANTE (recherche de support
# propre, OBig) + cliques completes du manuscrit + Kruskal propre a lots ;
# invariant des rayons de naissance (attach_violations = 0) ; fixtures du
# nœud ternaire colineaire et du tie q4/q2 a representants differents ;
# mutants chronologie-binaire et egalite-de-representation tues.
add_executable(mhgp4_forest_selftest tests/forest_selftest.cpp)
mhgp4_expect_code(mhgp4_forest_accord 0 mhgp4_forest_selftest "")
mhgp4_expect_code(mhgp4_forest_mutant_binaire 4 mhgp4_forest_selftest
    "--inject=binary-ties")
mhgp4_expect_code(mhgp4_forest_mutant_repr 4 mhgp4_forest_selftest
    "--inject=repr-ties")
# Audit bloquant « coquilles u16 » : supprimer les boules a coquille change
# les composantes HGP (carre cocyclique) — le mutant qui retablit l'ancien
# refus doit mourir sur la fixture square_cospherical_K2_plateau.
mhgp4_expect_code(mhgp4_forest_mutant_drop_shell 4 mhgp4_forest_selftest
    "--inject=drop-shell-plateau")
# Audit « facettes nees dans le lot » : les attachements nes au niveau ne
# sont jamais des enfants du nœud — le mutant qui retablit l'ancienne
# convention (tout le lot en pre-lot) gonfle les arites et meurt.
mhgp4_expect_code(mhgp4_forest_mutant_attach 4 mhgp4_forest_selftest
    "--inject=attach-prebatch")
# Audits « naissances et croissances » : le payload complet (ComponentDelta)
# porte naissances, croissances et fusions avec identites — le mutant qui
# retablit le squelette de fusions (deltas non-merge supprimes) meurt.
mhgp4_expect_code(mhgp4_forest_mutant_nonmerge 4 mhgp4_forest_selftest
    "--inject=drop-nonmerge")
# Audit « rendu § 9.1 » : F_K^render = TOUTES les facettes (nees au lot
# comprises), multiplicite = CHAQUE K-simplexe incident, naissance d'une
# facette = sa miniboule exacte. Trois mutants : rendu active-only (vide a
# la naissance du carre K=3), multiplicites ecrasees a 1 (compression par
# arbre couvrant — fausse pour S_tau), naissance lue sur la premiere
# incidence (50 contre 100 sur les cotes du carre K=2).
mhgp4_expect_code(mhgp4_forest_mutant_render_active 4 mhgp4_forest_selftest
    "--inject=render-active-only")
mhgp4_expect_code(mhgp4_forest_mutant_render_collapse 4 mhgp4_forest_selftest
    "--inject=render-collapse-mult")
mhgp4_expect_code(mhgp4_forest_mutant_birth_events 4 mhgp4_forest_selftest
    "--inject=birth-from-events")

# La foret sur FLUX REELS : WSPD -> RLE par BallKey inter-lanes -> un census
# par cle (SpherePlateau d'echelle) -> expansion -> fold. Le juge borne
# refait la meme semantique en brut (completude WSPD + census d'arbre + RLE
# juges) ; mutants rle-drop et census-nonstrict tues.
add_executable(mhgp4_forest_probe bench/forest_probe.cpp)
find_package(Threads REQUIRED)
target_link_libraries(mhgp4_forest_probe PRIVATE Threads::Threads)
mhgp4_expect_code(mhgp4_forest_probe_judge_uniform 0 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --min-balls=100 --min-fusions=100")
mhgp4_expect_code(mhgp4_forest_probe_judge_clusters 0 mhgp4_forest_probe
    "--family=eight_clusters --n=120 --s=8 --seed=3 --judge --min-balls=100 --min-fusions=100")
mhgp4_expect_code(mhgp4_forest_probe_smoke_n400 0 mhgp4_forest_probe
    "--family=uniform --n=400 --s=8 --seed=3 --min-balls=1000 --min-fusions=1000")
mhgp4_expect_code(mhgp4_forest_probe_mutant_rle 4 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --inject=rle-drop")
mhgp4_expect_code(mhgp4_forest_probe_mutant_census 4 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --inject=census-nonstrict")
# FRONTIERE D'IDENTITE (audits e7e4d5e) : PointId != index dense != rang
# Morton. Porte de relabeling : ids brouilles (> 2^31), bijection π sur les
# ids a positions fixes (sortie transportee par π), permutation physique des
# couples (id, position) (sortie bit-identique), aucune cle publique hors des
# ids fournis. Le mutant dense-pointid (cast du rang — l'ancien code) meurt.
mhgp4_expect_code(mhgp4_forest_probe_relabel_gate 0 mhgp4_forest_probe
    "--relabel-gate")
mhgp4_expect_code(mhgp4_forest_probe_mutant_dense_pointid 4 mhgp4_forest_probe
    "--relabel-gate --inject=dense-pointid")
# Prefiltre EXACT par boule (audit ec683b) : passe count-only au seuil
# h_qmin par arite minimale (10/9/8 interieurs pour q2/q3/q4), boites
# strictement interieures avalees en O(1), coquilles jamais comptees ;
# census complet sur les seules survivantes, compte recoupe. Porte de
# profondeur : mort exacte au seuil par arite SANS census, jumelles a h-1
# survivantes au compte exact. Mutants : threshold-minus-one (jumelles
# tuees a tort), range-add-max-le-zero (coquilles comptees), et au flux
# juge : range-add (boules a plateau perdues) et skip-full-census (la
# passe 1 ne connait pas U_B et ne remplace jamais la passe 2).
mhgp4_expect_code(mhgp4_forest_probe_depth_gate 0 mhgp4_forest_probe
    "--depth-gate")
mhgp4_expect_code(mhgp4_forest_probe_depth_mutant_threshold 4 mhgp4_forest_probe
    "--depth-gate --inject=threshold-minus-one")
mhgp4_expect_code(mhgp4_forest_probe_depth_mutant_range_add 4 mhgp4_forest_probe
    "--depth-gate --inject=range-add-max-le-zero")
mhgp4_expect_code(mhgp4_forest_probe_mutant_range_add_judged 4 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --inject=range-add-max-le-zero")
mhgp4_expect_code(mhgp4_forest_probe_mutant_skip_census 4 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --inject=skip-full-census")
# Garde d'entree (audit § 5) : PointId dupliques et coordonnees hors u16
# refuses (invalid_input) AVANT la construction de l'arbre.
mhgp4_expect_code(mhgp4_probe_guard_dup_id 2 mhgp4_forest_probe
    "--guard=dup-id")
mhgp4_expect_code(mhgp4_probe_guard_coord_range 2 mhgp4_forest_probe
    "--guard=coord-range")
# Profondeur AVANT coquille (audit e7e4d5) : au double debordement le
# verdict est dead_depth par la passe 1, jamais resource_exhausted — le
# mutant shell-cap-before-depth (census a une passe) rencontre la coquille
# d'abord et rend le mauvais statut.
mhgp4_expect_code(mhgp4_forest_probe_depth_mutant_shell_first 4
    mhgp4_forest_probe "--depth-gate --inject=shell-cap-before-depth")
# smax DYNAMIQUE (audit e7e4d5) : le parametre amont existe en aval — caps
# de census par arite smax_eff - q, expansion, folds et totaux. Fixture de
# frontiere K_max=5/6 (evenement q2 de profondeur cinq, exactement K=6) ;
# mutant fold-hardcodes-kmax10 (constantes 9/11/10 retablies) tue ; profil
# dynamique juge de bout en bout a smax=6.
mhgp4_expect_code(mhgp4_forest_probe_kmax_gate 0 mhgp4_forest_probe
    "--kmax-gate")
mhgp4_expect_code(mhgp4_forest_probe_kmax_mutant 4 mhgp4_forest_probe
    "--kmax-gate --inject=fold-hardcodes-kmax10")
mhgp4_expect_code(mhgp4_forest_probe_judge_smax6 0 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --smax=6 --seed=3 --judge --min-balls=100 --min-fusions=100")
# Filtre de profondeur A LA GENERATION : #{z ∈ cover : P_B(z) < 0} MINORE
# |I_B| (le cover est un sous-ensemble — un sous-compte ne tue jamais a
# tort) ; atteindre h_q tue le candidat AVANT l'emission. Le mutant
# nonstrict (P <= 0 : coquilles et supports comptes) tue des boules a
# plateau a tort — le juge voit les evenements manquants.
mhgp4_expect_code(mhgp4_forest_probe_mutant_genfilter 4 mhgp4_forest_probe
    "--family=uniform --n=120 --s=8 --seed=3 --judge --inject=genfilter-nonstrict")
# Selection axiale bornee (reponse auditeur « axial borne ») : baseline
# enumeree CONTRE axial borne apres tri/RLE — cles, arite et REPRESENTATION
# identiques sur deux familles + la sphere cospherique R²=50 (84 points).
# Resultat CPU honnete : NEGATIF (+7 % t_gen a n=1600, la lentille baseline
# rejette en 3 operations la ou le balayage axial paye tous les sites) —
# production CPU = baseline, chemin axial OPT-IN pour le port GPU (travail
# borne regulier). Mutants du chemin axial : prefixe trop court, ties de
# frontiere supprimes, premier representant au lieu du minimum canonique
# (discrimine PRE-RLE : la re-canonicalisation inter-seeds le masque apres).
mhgp4_expect_code(mhgp4_forest_probe_axial_pair 0 mhgp4_forest_probe
    "--axial-pair-gate")
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_short 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-short-group")
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_ties 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-drop-ties")
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_first 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-first-rep")
# Sweep a DEUX COTES (contre-audit 63d364a) : d_cover exact lu dans les deux
# ordres axiaux, mort du groupe AVANT valid_completion, assertion de
# reception d_j == scan complet (kAxialVerify) sur chaque run de la porte,
# et fixture § 5 (sphere 1513/49 : trois interieurs tous du cote OPPOSE au
# completeur — absente a smax=6, presente au bon niveau a smax=7). Mutants :
# d_j sans le cote oppose, suffixe negatif inverse, profondeur non stricte
# (le groupe compte sa propre coquille).
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_opposite 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-ignore-opposite-side")
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_reverse 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-reverse-negative")
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_nonstrict 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=axial-depth-nonstrict")
# Cœur universel du seed (audit « axial arbre et cœur de seed » § 1) :
# 2·mu² <= J = D(3G−2EX) par Jung — un site P < 0, 2P² > J·B² est
# strictement interieur a toute sphere admissible du seed ; compte sur
# l'antichaine (ALL/NONE par boite, U320), arret a h_4, commun aux deux
# chemins. La fixture-cœur (six points cocirculaires R²=25, sphere
# 3721/144) grave la frontiere : l'egalite 2P² = J·B² (points du faisceau
# P=0, B=0) n'est PAS comptee — le mutant qui la compte tue le seed et
# perd la cle.
mhgp4_expect_code(mhgp4_forest_probe_axial_mutant_seed_core 4 mhgp4_forest_probe
    "--axial-pair-gate --inject=seed-core-nonstrict")
# Primitive du sweep extraite et testable seule (audit « sweep reçu et
# kernel sans alloc » § 3) : multisets synthetiques graves — la CAUSALITE
# des mutants de fenetre et de d_j s'etablit ici (une positive mu=0 sous
# L doit etre conservee par un vrai ignore-opposite ; reverse-negative
# inverse les verdicts de mu=1 et mu=3), la fixture geometrique 1513/49
# restant la porte d'integration.
mhgp4_expect_code(mhgp4_forest_probe_sweep_gate 0 mhgp4_forest_probe
    "--axial-sweep-gate")
mhgp4_expect_code(mhgp4_forest_probe_sweep_mutant_short 4 mhgp4_forest_probe
    "--axial-sweep-gate --inject=axial-short-group")
mhgp4_expect_code(mhgp4_forest_probe_sweep_mutant_ties 4 mhgp4_forest_probe
    "--axial-sweep-gate --inject=axial-drop-ties")
mhgp4_expect_code(mhgp4_forest_probe_sweep_mutant_opposite 4 mhgp4_forest_probe
    "--axial-sweep-gate --inject=axial-ignore-opposite-side")
mhgp4_expect_code(mhgp4_forest_probe_sweep_mutant_reverse 4 mhgp4_forest_probe
    "--axial-sweep-gate --inject=axial-reverse-negative")
mhgp4_expect_code(mhgp4_forest_probe_sweep_mutant_nonstrict 4 mhgp4_forest_probe
    "--axial-sweep-gate --inject=axial-depth-nonstrict")
# Parallelisme par rectangle (directive « paralléliser ») : l'objet
# post-RLE et les compteurs de generation sont INDEPENDANTS du decoupage
# — egalite au bit pres 1 fil / 4 fils sur les deux chemins. Le mutant
# par-drop-shard (la fusion oublie un ouvrier) prouve que la porte
# compare pour de vrai.
mhgp4_expect_code(mhgp4_forest_probe_par_gate 0 mhgp4_forest_probe
    "--par-gate")
mhgp4_expect_code(mhgp4_forest_probe_par_mutant_drop 4 mhgp4_forest_probe
    "--par-gate --inject=par-drop-shard")
# L'AVAL est parallele lui aussi (prefiltre par candidat, census par
# survivante, expansion par tranche de boules, folds par K — fusions en
# ordre de tranche, bit-identiques au sequentiel). Le mutant qui oublie
# une tranche de census a la fusion doit mourir sur la comparaison AVAL.
mhgp4_expect_code(mhgp4_forest_probe_par_mutant_chunk 4 mhgp4_forest_probe
    "--par-gate --inject=par-drop-ball-chunk")
# Contre-audits Poisson q2 (contrat de sortie 30M) : la porte
# q2_birth_lower_bound ETABLIT sur un nuage reel l'argument d'injection
# (diametre unique de chaque tau_z, distinction globale des tau) avec
# planchers N_0..N_2 et la distinction K=1 (N_0 >= 2(n−1) : le nombre de
# certificats de Gabriel n'est pas le nombre d'aretes critiques) ; le
# preflight compte la sortie sans la materialiser (fumee : code 0).
mhgp4_expect_code(mhgp4_forest_probe_q2_birth_gate 0 mhgp4_forest_probe
    "--q2-birth-gate")
mhgp4_expect_code(mhgp4_forest_probe_q2_birth_mutant_dup 4 mhgp4_forest_probe
    "--q2-birth-gate --inject=birth-dup-tau")
mhgp4_expect_code(mhgp4_forest_probe_preflight_smoke 0 mhgp4_forest_probe
    "--family=uniform --n=400 --s=8 --seed=3 --output-preflight-only")
# Plafond transactionnel (audit bloquant C829 § 5.3) : count -> preflight
# -> fill — un plafond minuscule refuse (code 2) AVANT toute allocation
# d'ev_k ; un plafond genereux laisse le run aboutir a l'identique.
mhgp4_expect_code(mhgp4_forest_probe_max_output_refus 2 mhgp4_forest_probe
    "--family=uniform --n=400 --s=8 --seed=3 --min-balls=100 --min-fusions=100 --max-output-bytes=4096")
mhgp4_expect_code(mhgp4_forest_probe_max_output_passe 0 mhgp4_forest_probe
    "--family=uniform --n=400 --s=8 --seed=3 --min-balls=100 --min-fusions=100 --max-output-bytes=1000000000")
# Fold COMPACT (reponse d'audit « fold compact » § 1-2) : canonique par
# min-fid u32, tables a epoque pour les racines pre/post-lot, partition
# finale DENSE (facet_keys + final_canon_fid, invariants structurels
# permanents), vue map de compatibilite sur demande. La porte a deux
# backends compare le fold compact au backend FIGE build_forest_legacy
# (compteurs, niveaux, nœuds, deltas paire a paire, partition map ET vue
# dense) ; le mutant canonical-is-uf-root (canonique = racine UF au lieu
# du minimum) doit diverger.
mhgp4_expect_code(mhgp4_forest_probe_fold_compact_gate 0 mhgp4_forest_probe
    "--fold-compact-gate")
mhgp4_expect_code(mhgp4_forest_probe_fold_mutant_canon_root 4 mhgp4_forest_probe
    "--fold-compact-gate --inject=canonical-is-uf-root")
# Etage flottant certifie du signe de P (reçu « filtre flottant » —
# enregistrement CTest REPARE : la porte existait sans etre cablee) puis
# KERNEL AFFINE par ancre (audit « filtre certifie et niveaux q3 » § 1) :
# sous kFloatVerify chaque signe certifie est recoupe par l'exact (zero
# desaccord), planchers des deux signes et du repli (fixture-cœur
# cocirculaire ×1999). La porte affine grave EN PLUS l'identite
# L = 4·q3_power et la divisibilite L ≡ 0 (mod 4) sur TOUS les triples
# de petits nuages (y compris z ∈ {a, b, x}), et le temoin de forte
# annulation ± (§ 6.1) aux constantes gravees (deux termes ~2^102
# s'annulent a ±petit : la borne saine declare incertain les deux
# variantes, la borne retrecie certifie le bruit contre l'exact). Le
# mutant float-threshold-too-small (borne /2^20) meurt dans les DEUX.
mhgp4_expect_code(mhgp4_forest_probe_float_gate 0 mhgp4_forest_probe
    "--float-gate")
mhgp4_expect_code(mhgp4_forest_probe_float_mutant_small 4 mhgp4_forest_probe
    "--float-gate --inject=float-threshold-too-small")
mhgp4_expect_code(mhgp4_forest_probe_q3_affine_gate 0 mhgp4_forest_probe
    "--q3-affine-gate")
mhgp4_expect_code(mhgp4_forest_probe_q3_affine_mutant_small 4 mhgp4_forest_probe
    "--q3-affine-gate --inject=float-threshold-too-small")
# Garde d'arrondi executable (contre-audit 04c71a2 § 4) : sous
# FE_UPWARD le filtre se desactive seul (zero certification, sortie
# inchangee), sous FE_TONEAREST il certifie ; le mutant qui ignore la
# garde certifie sous FE_UPWARD et meurt.
mhgp4_expect_code(mhgp4_forest_probe_float_rounding_gate 0 mhgp4_forest_probe
    "--float-rounding-gate")
mhgp4_expect_code(mhgp4_forest_probe_float_rounding_mutant 4 mhgp4_forest_probe
    "--float-rounding-gate --inject=float-ignore-rounding")

# La lane q2 productrice : W_2 est la boule diametrale ouverte, le census
# est une requete de cover a coefficient 1. Juge brut toutes-paires sur
# records complets ; fixture a coquille exacte gravee et mutant sign-le
# (coquille comptee interieure => publication a tort) tue.
add_executable(mhgp4_q2_events_probe bench/q2_events_probe.cpp)
mhgp4_expect_code(mhgp4_q2_events_judge_uniform 0 mhgp4_q2_events_probe
    "--family=uniform --n=400 --s=8 --seed=3 --judge --min-events=100")
mhgp4_expect_code(mhgp4_q2_events_judge_clusters 0 mhgp4_q2_events_probe
    "--family=eight_clusters --n=400 --s=8 --seed=3 --judge --min-events=100")
mhgp4_expect_code(mhgp4_q2_fixture_coquille 0 mhgp4_q2_events_probe
    "--fixture --s=8 --judge")
mhgp4_expect_code(mhgp4_q2_mutant_sign_le 4 mhgp4_q2_events_probe
    "--fixture --s=8 --inject=sign-le")

# Selection axiale (MATHEMATIQUES.md § 4.6) : accelerateur RECU contre la
# baseline enumeree — memes records exiges par le meme juge brut sur les
# trois sources, fixture de justesse au bord du rang (profondeur h_4 - 1,
# sept predecesseurs du meme cote), mutant a un groupe de moins tue.
mhgp4_expect_code(mhgp4_q4_axial_fixture 0 mhgp4_q4_events_probe
    "--fixture --s=8 --axial --judge --min-events=1")
mhgp4_expect_code(mhgp4_q4_axial_judge_uniform 0 mhgp4_q4_events_probe
    "--family=uniform --n=120 --s=8 --seed=3 --axial --judge --min-events=5")
mhgp4_expect_code(mhgp4_q4_axial_judge_clusters 0 mhgp4_q4_events_probe
    "--family=eight_clusters --n=120 --s=8 --seed=3 --axial --judge --min-events=5")
mhgp4_expect_code(mhgp4_q4_axial_tight 0 mhgp4_q4_events_probe
    "--fixture-tight --s=8 --axial --judge --min-events=1")
mhgp4_expect_code(mhgp4_q4_axial_tight_baseline 0 mhgp4_q4_events_probe
    "--fixture-tight --s=8 --judge --min-events=1")
mhgp4_expect_code(mhgp4_q4_axial_mutant_rank 4 mhgp4_q4_events_probe
    "--fixture-tight --s=8 --axial --inject=axial-rank-cut")

# Ordre SEMANTIQUE des niveaux q4 (audit « lemme prefixe et niveau » § 2) :
# U320 contre l'oracle 384 bits, plancher de plateaux semantiques a
# representations DIFFERENTES (la grande cosphere les garantit — le cas que
# operator== raterait et que les macro-lots doivent grouper), coherence de
# la promotion q3, fixture de largeur w[4] ; mutant tronque tue.
add_executable(mhgp4_q4_level_cmp tests/q4_level_compare_test.cpp)
mhgp4_expect_code(mhgp4_q4_level_cmp_accord 0 mhgp4_q4_level_cmp "")
mhgp4_expect_code(mhgp4_q4_level_cmp_mutant 4 mhgp4_q4_level_cmp
    "--inject=level320-trunc-hi")

# Oracle rationnel independant q4 (memes standards que q3 : obigint,
# primitives locales, fixtures u16 extremes dont le tetraedre regulier a
# M=65535 et la grande cosphere, compteurs de limbes) ; mutants cramer-swap,
# mul-carry-lost et sign-p4 tues.
add_executable(mhgp4_q4_oracle tests/q4_oracle_test.cpp)
mhgp4_expect_code(mhgp4_q4_oracle_accord 0 mhgp4_q4_oracle "")
mhgp4_expect_code(mhgp4_q4_oracle_mutant_cramer 4 mhgp4_q4_oracle
    "--inject=cramer-swap")
mhgp4_expect_code(mhgp4_q4_oracle_mutant_carry 4 mhgp4_q4_oracle
    "--inject=mul-carry-lost")
mhgp4_expect_code(mhgp4_q4_oracle_mutant_sign 4 mhgp4_q4_oracle
    "--inject=sign-p4")

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
