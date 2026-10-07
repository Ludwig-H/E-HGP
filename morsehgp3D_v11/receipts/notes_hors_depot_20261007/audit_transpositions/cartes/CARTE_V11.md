# Carte de la v11 actuelle : moteur FULL, temps, mémoire, K = 10, voies essayées, budget de 100 ms

4 octobre 2026, 11 h 55 UTC (`date -u`). Rédigée pour les autres auditeurs de l'audit des transpositions
(`../CONTEXTE.md`). Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé. Aucune construction, aucune exécution native, aucune commande git qui écrit. Lecture de
`origin/main` = `c22be4e41` (worktree `build/v11-claude-20261003`, extraction partielle : les reçus sont lus par
`git archive origin/main`), puis petits calculs Python sur les JSON des reçus. Le script
[`CARTE_V11_derive.py`](CARTE_V11_derive.py) recalcule les tableaux du § 2 et du § 3 depuis les deux archives G4
(`python3 -B CARTE_V11_derive.py`, code 0 vérifié le 4 octobre).

Légende : **M** = mesuré (reçu G4 nommé), **E** = estimé (arithmétique sur des mesures), **C** = conjecturé.

## 0. Versions et sources

| Objet | Identité | Ce qui la qualifie |
|---|---|---|
| Moteur courant | sources de **`b87285378`** (3 oct., 19 h 40 UTC, dernier commit touchant `src/`) ; `git diff b87285378 origin/main -- morsehgp3D_v11/src` est vide ; `src/` identiques à `e02a6c235` et `8f68622b2` d'après [DEEP] | qualification de développement `claudeab7` : suite `fast` 666/666, TSan 7/7, 11 mutants tués, 36 prises aux dumps identiques [AB7] |
| Dernier moteur pleinement qualifié | **`c40f40798`** | 4073/4073 portes, 326 mutants, GCC18/21/24, ASan24, TSan21 ; 81 prises appariées W1/W8/W48 [Q] |
| Entrées du contrat | trames SemanticKITTI 08/000000, 08/000100, 08/000200 sans sol (masque Patchwork++ v8), grille 1 mm, poids unitaires : **39 885 / 35 551 / 45 845 sites** (notées ng00/ng01/ng02) | manifeste `reuse1`, empreintes dans [Q] `full_paired.json` |
| Réglage mesuré | K = 5, u21, feuille 16, `max_leaf` 256, plafond 8 Gio, mode d'options **16379** | argv des prises [Q], `ab_g4.py` `ARGS_TAIL` [AB7] |

Sources abrégées (chemins relatifs à `morsehgp3D_v11/` sur `origin/main`) :

- [Q] `receipts/qualification_performance_20261003/` : `README.md`, `review/analysis.md`, `review/metrics.normal.json`,
  et `captures/paired/results/results.tar.gz` → `results/cmd/002_paired_full/files/full_paired.json`.
- [AB7] `receipts/developpement_20261003/pipeline_g4/` : `README.md` et `sessions/claudeab7/results.tar.gz` →
  `results/cmd/000_ab/files/t_{base,new}_lidar_ng0X_w{1,48}_rN.stdout`, `perf_new_self.stdout`, `ab_report.json`.
  « new » = tranche 3 (b872), « base » = `a45daff3a` (même mode 16379).
- [PROF1] `receipts/developpement_20261003/pipeline_g4/sessions/claudeprof1/results.tar.gz` (variantes `-march`,
  profils `perf` W1/W48 de la base).
- [NP] `receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md.snapshot` ;
  [NV] `receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_V11_20261003.md.snapshot`.
- [ECART] `receipts/developpement_20261003/ecart_v10_v11/README.md` et `sim.txt` (mesures **locales**, 4 cœurs).
- [DEEP] `receipts/audit_deep_20261004/performance/README.md` ; [AUD] `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`
  (version réécrite le 4 octobre).
- [PTS4] `receipts/pts4_review_20261003/README.md` et `case_metadata.json.gz`.
- Ablations G4 successives : [FULL3] `receipts/full_20261002/`, [SWEEP2] `receipts/full_sweep_20261002/`,
  [MEMO1] `receipts/full_memo_20261003/memo1/`, [FOREST3] `receipts/full_parallel_20261003/forest3/`,
  [COMB3] `receipts/catalogue_single_full_20261003/combined3/`, [ASM1] `receipts/catalogue_assembly_20261003/assembly1/`,
  [CENS2] `receipts/full_census_20261003/census2/`, [REUSE1] `receipts/full_regular_vertical_20261003/reuse1/`,
  [PAR5] `receipts/catalogue_parallel_20261002/`, [CAT3] `receipts/catalogue_20261002/`.
- v10 : [L06] `build/v11-persist/audit_v10/L06_CODE_TOUR.md` ; [SYN] `morsehgp3D_v11/docs/AUDIT_V10_SYNTHESE.md` § 3.

## 1. Les étages du moteur FULL et leurs fichiers

Le banc `bench/full_probe.cpp` (l. 200–260) chronomètre **FULL = index + domaine + forêts** ; il publie aussi les
sous-étages du catalogue (`CatalogueTimings`, `src/catalogue/catalogue.hpp` l. 69–77) et cinq phases globales de forêt
(`FullTimings`, `src/tower/forest.hpp` l. 49–64). Enchaînement réel du code : `build_index` →
`prepare_full_domain` (`src/tower/full_domain.hpp` l. 53–61) → `build_full` (`src/tower/forest_vertical.cpp` l. 280) →
`build_concurrent` (`src/tower/forest_concurrent.cpp` l. 224) → `pipeline_orders` (`src/tower/forest_pipeline.cpp` l. 115).

| # | Étage (champ chronométré) | Ce qu'il fait | Fichiers principaux | Parallélisme |
|---|---|---|---|---|
| 0 | Hors FULL : lecture, Cloud, Pool | tri par base sur (Morton, PointId), sites, multiplicités ; Pool synchrone (mutex + condition) | `bench/whole_input.hpp`, `src/cloud/cloud.cpp`, `morton.hpp`, `src/sched/pool.cpp` | séquentiel |
| 1 | Index (`index_ns`) | arbre de plages Morton, feuilles de 8, boîtes exactes | `src/index/build.cpp`, `index.hpp` | séquentiel |
| 2a | Préambule du catalogue (`prefix_ns`) | filtre G1 de la racine, plan adaptatif « lourds d'abord » (≤ 1024 feuilles, rondes), rejeu figé | `src/catalogue/adaptive_prepare.cpp`, `adaptive_frontier.cpp`, `adaptive_replay.cpp`, `frontier.cpp`, `frontier_dispatch.hpp`, `boxes.cpp` | rondes du sommet presque sérielles [NV § 6] |
| 2b | Génération en une passe (`single_pass_ns`) | par tâche : DFS des boîtes T0 (dominance G1, ajustement, coupe), feuilles de 16 sites au plus (mesuré : `max_leaf` = 16) : dominateurs, graphe de paires et lignes vivantes, G3, droites de centres J2 (+ cache), centre dans la boîte, census local G2, S* canonique, émission en arènes | `boxes.cpp`, `leaf.cpp` (`extend`, `enumerate_leaf`, `census_and_emit`), `small_pair_graph.hpp`, `center_line_cache.hpp`, `support.cpp`, `single_pass.cpp`, `single_pass_storage.hpp` ; `src/num/center_region.cpp`, `sphere.cpp`, `predicates.cpp`, `q4_weights.hpp` | 1023 tâches réclamées en LPT |
| 2c | Compactage (`compact_ns`) | copie des arènes vers B émissions et I incidences | `single_pass.cpp` | parallèle |
| 2d | Tri (`sort_ns`) | permutation exacte : clés F3/F4 binary64 puis repli exact, blocs de 2048, fusions par co-rangs | `sort_indices.cpp`, `sort_level_key.hpp` | parallèle |
| 2e | Rangs et assemblage (`level_scan_ns`, `assembly_ns`, `allocation_ns`) | rangs denses des niveaux, CSR I puis U | `assembly_parallel.cpp`, `assemble.cpp` | parallèle par blocs |
| 2f | Table des supports (dans le résidu du domaine) | S* → BallIdx, sondage linéaire rempli par CAS | `src/tower/full_domain.cpp` | parallèle (CAS) |
| 3a | Contextes FULL (résidu des forêts) | table de populations I∪U → boule, graines verticales régulières, espaces census, `ForestParallel` | `population_lookup.cpp` (`make`), `regular_vertical_seeds.hpp`, `census_slots.hpp`, `src/index/census_workspace.cpp`, `forest_parallel.cpp` ; appelés dans `build_full` | table par CAS ; reste séquentiel |
| 3b | A Classification (`classify_ns`) | type de chaque cellule à chaque ordre | `forest_concurrent.cpp` (`classify_parallel`), `cells_classify.cpp`, `cells.cpp` | blocs de boules |
| 3c | B Naissances (`births_ns`) | naissances par blocs, cohortes de même rang triées par centre exact, états DSU, jobs ; liaison de la table (`bind`) | `forest_build.cpp`, `src/num/centers.cpp`, `population_lookup.cpp` | trois distributions ; cohortes = une tâche par ordre |
| 3d | C Résolution régulière (`regular_ns`) | chaque face d'une cellule régulière : trace → descente datée (table de populations avant chaque pas ; sinon MEB bornée, localisation, census global) → graine | `forest_parallel.cpp` (`resolve_job` l. 48, `resolve_lane`, préchargement l. 144), `population_lookup.cpp`, `descent.cpp`, `locate.cpp`, `meb.cpp`, `canonical.cpp`, `full_domain.cpp` (`find_support`), `census_workspace.cpp`, `predicates.cpp` | L = W − (2K − 1) résolveurs |
| 3e | D Publication (`publish_ns`) | plateaux atomiques : unions DSU, multifusions N-aires, parents ; cellules étendues (m > qmin) résolues ici | `forest_concurrent.cpp` (`publish`), `forest_plateau.cpp`, `forest_build.cpp` | **une tâche par ordre** |
| 3f | E Verticales (`verticals_ns`) | images basses des naissances (graine régulière réemployée ou descente), balayage fermé par DSU, contrôle des enfants | `forest_vertical_parallel.cpp`, `forest_vertical.cpp`, `forest_ancestor_sweep.hpp`, `forest_vertical_seed.hpp` | images parallèles ; **un balayage par ordre** |
| 3g | Pipeline C+D+E | recouvre 3d–3f si W ≥ 2K et au moins 2K espaces census ; sinon étages séparés | `forest_pipeline.cpp` (`pipeline_lanes` l. 109–113) | K5/W48 : 39 résolveurs, 5 publieurs, 4 suiveurs ; K10/W48 : 29/10/9 [AUD] |
| 4 | Hors FULL : dump, export, Python | sérialisation 255–329 Mo ; export POINTS (`bench/points_export.cpp`) ; H^r_{k+1} et sortie plate en Python (`bench/points_radius.py`, `points_flat.py`) | — | — |

Remarques de lecture :

- À W1 et W8 le pipeline est inactif (W < 2K) : publication et verticales sont des étages complets. À W48 les champs
  `publish_ns` et `verticals_ns` ne sont que des **queues** après la fin de la dernière résolution
  (`docs/PERFORMANCE_FULL.md` l. 170–177) : on ne les compare pas aux valeurs W1.
- Le chrono FULL exclut lecture (0,2–0,3 ms), Cloud (0,9–1,1 ms), création du Pool (~0,6 ms), dump et destruction
  (processus − FULL = 479–600 ms à W48), décodage Python et segmentation du sol [Q analysis.md l. 44–54]. **M**

## 2. Temps mesurés par étage, K = 5

### 2.1 Moteur courant b872, W48 (prise médiane de cinq) — [AB7] **M**

Les étages d'une même prise sont disjoints : la colonne se somme au FULL (aux résidus près, explicités).

| Étage | ng00 ms (%) | ng01 ms (%) | ng02 ms (%) |
|---|---:|---:|---:|
| **FULL** | **412,4** (100) | **351,7** (100) | **380,7** (100) |
| Index | 0,4 (0,1) | 0,35 (0,1) | 0,4 (0,1) |
| **Domaine** | **239,8** (58,1) | **218,3** (62,1) | **234,2** (61,5) |
| · préambule (2a) | 20,1 (4,9) | 18,8 (5,3) | 20,7 (5,4) |
| · **passe unique (2b)** | **180,3** (43,7) | **167,0** (47,5) | **170,2** (44,7) |
| · compactage (2c) | 4,8 (1,2) | 4,0 (1,1) | 5,5 (1,5) |
| · tri (2d) | 11,5 (2,8) | 9,1 (2,6) | 13,6 (3,6) |
| · rangs + assemblage + allocation (2e) | 9,7 (2,3) | 7,7 (2,2) | 10,2 (2,7) |
| · résidu (2f, contrôles) | 13,4 (3,3) | 11,8 (3,4) | 13,9 (3,7) |
| **Forêts** | **172,2** (41,7) | **133,0** (37,8) | **146,0** (38,4) |
| · contextes (3a, résidu) | 9,1 (2,2) | 7,8 (2,2) | 9,8 (2,6) |
| · classification (3b) | 2,6 (0,6) | 2,3 (0,6) | 2,7 (0,7) |
| · naissances + liaison (3c) | 8,4 (2,0) | 6,8 (1,9) | 13,2 (3,5) |
| · **résolution régulière (3d)** | **115,8** (28,1) | **86,4** (24,6) | **96,4** (25,3) |
| · queue de publication (3e) | 36,2 (8,8) | 29,7 (8,4) | 23,9 (6,3) |
| · queue des verticales (3f) | 0,0 | 0,0 | 0,0 |
| CPU FULL (s) | 13,73 | 10,79 | 12,88 |

Plages sur les cinq prises W48 : FULL 373,7–464,2 / 327,7–362,6 / 350,5–438,4 ms ; passe unique 176,5–211,4 /
140,8–190,1 / 151,5–201,0 ms ; résolution 115,2–126,9 / 85,1–93,5 / 96,4–105,8 ms. Médianes indépendantes publiées :
FULL **412 / 352 / 381 ms**, domaine 253,0 / 219,3 / 222,2 ms, forêts 171,0 / 133,0 / 157,7 ms [AB7 README l. 40–44].
Meilleure prise de la source courante : **327,7 ms** (ng01) ; toutes v11 confondues : 320,4 ms (ng01, base `a45daff3a`
compilée `-march=x86-64-v4`, [PROF1]). **M**

### 2.2 Moteur courant b872, W1 (une prise) et accélérations W1 → W48 — [AB7] **M**, rapports **E**

| Étage | ng00 W1 ms (S) | ng01 W1 ms (S) | ng02 W1 ms (S) |
|---|---:|---:|---:|
| **FULL** | 9 133 (×22,1) | 7 067 (×20,1) | 8 467 (×22,2) |
| Domaine | 5 374 (×22,4) | 4 285 (×19,6) | 5 157 (×22,0) |
| · préambule | 85,5 (×4,3) | 71,2 (×3,8) | 85,9 (×4,1) |
| · passe unique | 4 752 (×26,3) | 3 794 (×22,7) | 4 462 (×26,2) |
| · compactage | 33,4 (×6,9) | 28,6 (×7,2) | 37,6 (×6,8) |
| · tri | 262,6 (×22,9) | 195,4 (×21,5) | 317,0 (×23,2) |
| · rangs + assemblage + allocation | 209,1 (×21,6) | 167,4 (×21,9) | 218,9 (×21,4) |
| · résidu | 32,2 (×2,4) | 29,1 (×2,5) | 36,1 (×2,6) |
| Forêts | 3 758 (×21,8) | 2 781 (×20,9) | 3 309 (×22,7) |
| · contextes | 45,8 (×5,0) | 43,3 (×5,6) | 60,2 (×6,1) |
| · classification | 37,2 (×14,2) | 31,6 (×13,9) | 40,4 (×15,0) |
| · naissances | 53,7 (×6,4) | 42,4 (×6,2) | 66,1 (×5,0) |
| · résolution régulière | 3 424 (×29,6) | 2 500 (×28,9) | 2 919 (×30,3) |
| · publication + verticales (étages complets) | 130,9 + 66,6 | 108,7 + 55,1 | 148,7 + 75,2 |
| CPU (s) | 9,13 | 7,07 | 8,47 |

Efficacité parallèle de FULL (G4 = 24 cœurs × 2 SMT) : S/48 = **46 / 42 / 46 %** par fil, S/24 = **92 / 84 / 93 %**
par cœur physique. Le CPU total à W48 vaut **×1,50–1,53** celui de W1 (≈ 31–34 fils occupés en moyenne), à compteurs
logiques identiques entre W1 et W48 (census, pas, MEB : mêmes valeurs dans les deux prises) ; ce surcoût n'est pas
attribué (SMT, contention, attentes : **C**). Le Pool attend par mutex/condition, pas par attente active
(`src/sched/sched.hpp`) ; le profil W48 de la base montre 1,6 % de `native_queued_spin_lock_slowpath` noyau [PROF1].

Étages mal parallélisés (S ≤ 7,2 à 48 fils) : préambule, résidu du domaine, compactage, contextes, naissances ; à W48
ils pèsent ensemble **49–63 ms** (E : 20,1+13,4+4,8+9,1+8,4 = 55,8 ; 18,8+11,8+4,0+7,8+6,8 = 49,2 ; 20,7+13,9+5,5+9,8+13,2 =
63,1 ms ; avec la queue de publication : 92 / 79 / 87 ms).

### 2.3 Qualification c40 : W1/W8/W48, médianes de trois prises — [Q] **M**

| c40 / 16379 | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| FULL W1 / W8 / W48 (ms) | 10 317 / 1 416 / **489,1** | 8 033 / 1 104 / **345,1** | 9 688 / 1 336 / **432,4** |
| Accélération W8 ; W48 | ×7,29 ; ×21,1 | ×7,27 ; ×23,3 | ×7,25 ; ×22,4 |
| Domaine W1 → W48 | 5 413 → 227,5 | 4 338 → 181,1 | 5 209 → 238,6 |
| Passe unique W1 → W8 → W48 | 4 796 → 614 → 169,1 | 3 841 → 490 → 131,3 | 4 510 → 576 → 176,5 |
| Forêts W1 → W48 | 4 882 → 240,8 | 3 702 → 163,6 | 4 467 → 197,2 |
| Résolution régulière W1 → W8 → W48 | 4 552 → 565 → 130,7 | 3 419 → 427 → 73,5 | 4 082 → 507 → 83,5 |
| Publication W48 (étage séparé) ; dont ordre 5 | 51,6 ; 51,5 | 35,8 ; 35,7 | 48,3 ; 47,2 |
| Verticales W48 (étage séparé) ; dont ordre 5 | 41,8 ; 39,5 | 36,7 ; 31,2 | 28,3 ; 25,7 |
| Naissances / classification / tri / préambule W48 | 14,3 / 2,5 / 11,6 / 21,0 | 11,4 / 2,2 / 9,1 / 17,8 | 16,9 / 2,7 / 13,2 / 20,9 |
| CPU W1 / W48 (s) ; parallélisme effectif W8 ; W48 | 10,32 / 13,54 ; 7,47 ; 27,7 | 8,03 / 10,66 ; 7,50 ; 30,9 | 9,69 / 12,50 ; 7,43 ; 28,7 |

Lectures **E** sur ces mesures :

- La passe unique gagne ×3,3–3,7 de W8 à W48 pour ×3 cœurs physiques (8 → 24) : elle est au plafond matériel SMT,
  comme l'écrit [NV § 2] ; seule une baisse de son travail CPU la raccourcit.
- La résolution régulière accélère plus que 24 cœurs × SMT ne le permettent à travail égal : c40 W1 → W48 ×35–49,
  W8 → W48 ×4,3–6,1 pour ×3 cœurs (b872 W1 → W48 : ×29–30, à la limite). Ses compteurs de travail sont pourtant
  identiques à W1, W8 et W48 : la voie W1 (étages, 48 lanes logiques sur un fil) coûte plus par cœur. Ces rapports ne
  sont donc pas des efficacités ; la cause n'est pas mesurée (**C** : cache agrégé plus grand à 48 fils).
- Variabilité à travail identique : résolution ng00 100,3–157,7 ms entre trois prises [Q analysis.md l. 29] ;
  domaine ±30 ms d'une prise à l'autre [AB7 README l. 46].

### 2.4 Où va le CPU (profil `perf`, W1, ng00, b872, 19 227 échantillons) — [AB7] `perf_new_self.stdout` **M**

| Bloc | Fonctions (part du CPU du processus) |
|---|---|
| Catalogue | `extend` 16,1 % ; `filter` (G1) 8,0 ; `enumerate_leaf` 6,8 ; `center_line_meets` 5,0 ; `Q4Candidate::through` 3,0 ; `Sphere::through` 2,4 + 0,6 ; `center_in_box` 1,8 ; `AssemblyPlan::scan_block` 1,5 ; tri (`merging` 1,4 + `sift` 1,2) ; `strictly_acute` 1,0 |
| Partagés catalogue/tour | `num::side` 7,1 + 0,6 ; `power_bound_signs` 5,0 ; `bound_terms` 2,0 |
| Forêts | `PopulationLookup::bound` 2,9 ; `FullDomain::find_support` 2,7 ; `resolve_job` 2,6 ; `PopulationLookup::find` 2,3 ; `CensusWorkspace::query` 2,1 ; `descend_each_step` 2,1 ; `strict_trace` 1,3 ; `hit` 1,1 ; `birth_node` 1,0 ; `visit_located_part` 0,9 ; `bounded_meb` 0,9 ; `regular_cell` 0,8 |
| Hors FULL | écriture du dump (`ostream::write`, `xsputn`, …) ≈ 2,5 |

Le profil W48 de la base `a45daff3a` a la même tête (`extend` 15,5 %, `filter` 7,3, `enumerate_leaf` 6,1, `side` 6,0,
`power_bound_signs` 5,3) [PROF1 `report_w48_self.stdout`]. **M**

### 2.5 Travail logique (identique pour tous W et toutes les voies) — [Q] **M**

| Compteur | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Boules de Cat5 / incidences I∪U | 1 306 696 / 6 097 121 | 1 095 926 / 5 085 683 | 1 407 885 / 6 514 697 |
| Nœuds / feuilles de boîtes ; profondeur max | 783 071 / 353 456 ; 36 | 637 505 / 284 835 ; 36 | 735 601 / 323 879 ; 36 |
| Tests du filtre G1 | 379,4 M | 308,6 M | 379,9 M |
| Préfixes logiques ; candidats jugés | 120,4 M ; 3,27 M | 96,0 M ; 2,65 M | 110,3 M ; 3,31 M |
| Candidats q4 → niveaux q4 matérialisés | 10,26 M → 158 494 | 8,20 M → 121 303 | 9,40 M → 143 105 |
| Droites J2 demandées / évaluées | 75,0 M / 31,3 M | 59,7 M / 24,9 M | 68,7 M / 28,7 M |
| Naissances / nœuds K1..5 / verticales | 897 776 / 1 541 750 / 1 462 069 | 761 726 / 1 306 721 / 1 235 709 | 982 670 / 1 683 088 / 1 591 680 |
| Cellules / plateaux | 2,16 M / 1,20 M | 1,82 M / 1,02 M | 2,35 M / 1,22 M |
| Pas de descente / succès de table | 4,80 M / 3,62 M | 3,92 M / 3,01 M | 4,89 M / 3,85 M |
| Appels census / tests de points | 291 515 / 21,15 M | 217 404 / 15,52 M | 208 111 / 14,76 M |
| Présentations MEB de parties | 3,79 M | 2,89 M | 3,31 M |

À ng00, l'ordre 5 porte 81 % des tests de points census (17,06 M), 70 % des présentations MEB (2,64 M), 40 % des pas
(1,92 M) et 438 011 des 1 201 009 plateaux ; c'est aussi l'ordre dont la publication et le balayage sont les plus
longs (§ 2.3). Coût moyen **E** de la passe unique à W1 : 4,75 s / 1,307 M boules ≈ 3,6 µs par boule émise.

## 3. Mémoire

Le moteur publie des **réservations `MemoryBudget` (Buffer + Cloud), pas une RSS** ; aucune RSS du seul FULL n'est
mesurée [Q README l. 24, DEEP]. **M** sauf mention.

| Poste (MiB) | ng00 | ng01 | ng02 | Source |
|---|---:|---:|---:|---|
| Pic b872, W48 | 354,2 (= 371,4 Mo) | 305,7 (320,5 Mo) | 377,4 (395,7 Mo) | [AB7], [DEEP] |
| Pic b872, W1 | 345,4 | 297,8 | 367,3 | [AB7] |
| Pic c40, W48 ; W1 | 346,0 ; 338,8 | 298,7 ; 292,3 | 368,3 ; 360,1 | [Q] |
| Réservé après FULL (tour retenue : catalogue, table des supports, forêts, lookups) | 231,6 | 199,2 | 244,8 | [AB7] |
| Arènes de la passe unique (capacité) | 169,7 | 145,6 | 181,2 | [AB7] |
| Table de populations b872 (lignes K+3 mots ; c40 : K+1) | 42,2 (c40 35,6) | 38,2 (32,6) | 44,6 (37,4) | [AB7], [Q] |
| Entrées de la table | 857 891 | 726 175 | 936 824 | [AB7] |
| Lookups denses des naissances | 20,1 | 16,9 | 21,7 | [AB7] |
| Espaces census à W48 (4·n·48 octets) | 7,3 | 6,5 | 8,4 | [AB7] |
| Graines verticales régulières | 5,0 | 4,2 | 5,4 | [AB7] |
| Dump FULL (octets, hors moteur) | 300 883 482 | 255 161 594 | 328 855 058 | [Q] |

- Le pic de b872 dépasse sa base de +8,6 / +7,3 / +9,5 Mo (lignes K+3 de la voie liée et coexistence de phases) [DEEP].
- Les postes ne s'additionnent pas au pic : arènes, sorties compactées, table et forêts ne coexistent pas tous.
  Le moment exact du pic n'est pas publié : le banc FULL ne rend qu'un pic global, alors qu'ARCHITECTURE § 7.1 demande
  que « chaque étage publie son pic d'octets réservés ».
- La v10 mesurait une RSS de 728 Mo à K5 et 3,21 Go à K10 sur G4, pic atteint pendant le catalogue [L06-07] ; les
  deux grandeurs (RSS v10, réservations v11) ne se comparent pas.

## 4. État de K = 10

- **Jamais mesuré dans les conditions du contrat.** Les six essais K10 de [FULL3] (`c6ca345e0`) et ceux de [SWEEP2]
  ont été omis par le budget de campagne ; au niveau du seul catalogue, les six K10 de [PAR5] (`c1046dfc7`) ont expiré
  au plafond de 15 s et les cinq de [CAT3] ont expiré aussi (code ancien, catalogue à deux passes). Aucune session
  postérieure (memo1 → claudeab7) n'a joué K10 sur les trames du contrat. [NP § 5] : « K = 10 n'est pas mesuré ». **M**
- **K1..10 tourne jusqu'au bout hors contrat** : campagnes points `claudepts3/4` (instantané de développement, HEAD
  `e26b48055`), voie rapide complète (d'après `bench/points_export.cpp` l. 370–377 de la source courante : les treize
  options de 16379), **4 fils par processus et 22 processus simultanés** sur la G4 (plan `claudepts3`,
  `--jobs 22 --native-workers 4`). Sur 125–127 scènes de la séquence 08 (criblage, témoins, voisines ; préparées sans
  sol par `bench/points_lidar_prepare.py`), 32 462–98 560 sites : `full_ns` **16–109 s** (médiane 55–66 s),
  **3,51–16,23 M boules** (médiane 138 boules par site) ; près de 40 000 sites : 43–50 s pour 4,25–4,42 M boules
  [PTS4 `case_metadata.json.gz`]. Ni W48, ni processus isolé : aucune durée K10 contractuelle n'en découle. **M**
- Taille de l'objet : Cat10 de 08/000000 compte **5 512 670 boules** et 9 887 430 cellules en v10, contre 1 306 696 et
  2 164 763 à K5 (×4,22 et ×4,57) ; descentes résolues ×4,8, MEB ×7,1 [L06-05]. Le catalogue étant le même objet et
  les cardinalités K5 v10/v11 étant égales [DEEP], la v11 aurait les mêmes 5 512 670 boules (**E**, non mesuré en v11).
- Temps v10 à K10 (G4, 48 fils) : 861–1 125 ms catalogue + FULL [SYN § 3] ; tour seule 334–472 ms, plancher séquentiel
  89–140 ms ; catalogue 528–653 ms [L06-04]. Rapport v10 K10/K5 ≈ ×4,2–4,4 (**E**, bornes des intervalles).
- Structure v11 à K10 : `kMaxMebSites` = 12 (K ≤ 12) ; feuille ≥ K+3 = 13 (feuille 16 admise, 8 refusée) ; graphe de
  paires seulement pour les feuilles ≤ 32 sites ; pipeline W48 : 29 résolveurs, 10 publieurs, 9 suiveurs [AUD] ; la
  publication reste une tâche par ordre, donc dix chaînes séquentielles dont la plus lourde est l'ordre 10.
- Ordre de grandeur **C** : au rapport v10 (×4,2–4,4), la v11 b872 serait vers 1,5–1,8 s à K10 sur W48 ; le contrat
  « si possible K = 10 » en 100 ms est à ×15–18 de ce point, sans mesure pour le confirmer.

## 5. Ce qui a déjà été tenté, gardé ou retiré

### 5.1 Options : toutes inactives par défaut

`FullParams{}` (`src/tower/forest.hpp` l. 38–48) et `CatalogueParams{}` (`src/catalogue/catalogue.hpp` l. 25–37) gardent
la voie de référence lente : frontière fixe, deux passes, feuille 32, aucune option. La voie rapide est un masque du
banc (`bench/full_probe.cpp` l. 290–312) ; **16379 = les quatorze bits sauf le mémo (bit 4)**. [NV A1–A2] : quatorze
bits, 16 384 combinaisons, voie qualifiée entièrement opt-in ; aucun préréglage par défaut n'a été livré. Aucune mesure
G4 du mode 0 sur la source courante (la note avance « 2 à 3 fois plus lent », sans reçu dédié).

| Bit | Option | Dans 16379 | Effet mesuré sur G4, W48, ng00 u21 sauf mention | Reçu |
|---:|---|:---:|---|---|
| 1 | cache J2 des droites de centres (catalogue) | oui | 43,7 M succès sur 75,0 M demandes ; pas d'ablation isolée | [Q] compteurs |
| 2 | tri indirect + clés F3/F4 à repli exact | oui | tri 9–13 ms à W48 ; local W1 1,71 → 0,38 s | [Q], [NP § 3] |
| 4 | mémo de descente (65 536) + mémos de lane (4 096) | **non** | FULL 18 642 → 14 546 ms (×1,28) sur l'ancien code ; mémos de lane 2,6 % de succès ; incompatible avec le pipeline (`pipeline_lanes` rend 0 si mémo) | [MEMO1], [NP C3] |
| 8 | lots réguliers parallèles Q = 4096, L = 48 | oui | FULL 14 790 → 6 163 ms (mode 7 → 15) | [FOREST3] |
| 16 | frontière adaptative, « lourds d'abord », LPT | oui | avec le bit 32 : domaine 3 031 → 1 521 ms ; plus longue tâche 0,885 → 0,035 s (local W1) | [COMB3], [ECART] |
| 32 | assemblage parallèle | oui | catalogue 3 352 → 2 928 ms (mode 3 → 11) | [ASM1] |
| 64 | passe unique en arènes | oui | domaine 1 521 → 845 ms (mode 63 → 127) | [COMB3] |
| 128 | verticales parallèles | oui | FULL 2 929 → 1 732 ms (127 → 255) | [CENS2] |
| 256 | census emprunté réutilisé | oui | FULL 1 732 → 1 588 ms (255 → 511) | [CENS2] |
| 512 | lookup dense des naissances | oui | FULL 1 622 → 1 436 ms (511 → 1023) | [REUSE1] |
| 1024 | réemploi des verticales régulières | oui | 1 436 → 1 463 ms sur ng00 (sans gain), 1 219 → 1 155 ng01, 1 543 → 1 515 ng02 | [REUSE1] |
| 2048 | graphe de paires, lignes vivantes, coupe d'union du préfixe | oui | pas d'ablation G4 isolée ; somme des tâches de la passe unique (local W1) 9,30 → 7,99 s | [ECART `sim.txt`] |
| 4096 | table de populations I∪U → boule | oui | présentations MEB 15,70 → 3,79 M (paquet 2047 → 16379) | [Q] |
| 8192 | ordres concurrents | oui | paquet 2047 → 16379 : 844,0 → 489,1 ms (graphe, table, concurrence et retrait du mémo ensemble) | [Q] |

Chaque reçu est une session distincte, une prise par cellule sauf [Q] et [AB7] : ces gains ne sont ni appariés
entre sessions ni isolés (une ablation par paquet).

### 5.2 Changements toujours actifs (remplacent la voie, sorties identiques)

- Numérique : voie native i128 q1/q2/q4 aux trois profils, certificat ou essai i128 pour q3 avant repli `Wide`
  (`docs/CATALOGUE.md`, `PREDICATS_*.md`) ; orientation certifiée ; niveaux q4 différés (`Q4Candidate`, qualifiés à
  `ffc2ff95f`) : 10,26 M candidats pour 158 494 niveaux construits sur ng00.
- Catalogue : contacts du support réutilisés dans le census de feuille ; filtre G1 prétraité par nœud ; G3 avant les
  droites J2 ; popcount SWAR ; table des supports remplie par CAS (`docs/PERFORMANCE_FULL.md` l. 20–58).
- Tour : MEB par diamètre exact puis q3/q4 et constructions différées (`MEB_DIAMETRE.md`, `MEB_CONSTRUCTIONS_DIFFEREES.md`) ;
  classification sans traces et balayage vertical fermé par DSU (`FULL_OPTIMISATIONS.md` : 235 M marches de parents
  à 08/000000 ; FULL 21 264 → 18 974 ms entre [FULL3] et [SWEEP2], sources différentes, [SWEEP2] en mode 3 avec cache
  J2 et tri indirect) ; cohortes de naissances triées par centre ; BirthRuns ;
  unions de racines courantes ; préchargement en trois étages de la table ; census en signes natifs.
- Tranche 3 (b872, active dès que concurrence + table + W ≥ 2K) : voie liée de la table (naissance et rang dans la
  ligne), naissances par blocs, pipeline résolution/publication/verticales à attentes futex. **M** [AB7] : W48
  446,5 / 439,7 / 440,4 → **412,4 / 351,7 / 380,7 ms** ; forêts 209 / 209 / 205 → 171 / 133 / 158 ms ; résolution
  régulière W1 −26 à −30 % (4,61 → 3,42 s sur ng00).

### 5.3 Essayé puis retiré, ou mesuré sans gain

| Essai | Résultat | Source |
|---|---|---|
| Filtre F6 flottant des signes de `power` et de ses bornes de boîte (census de l'index et des feuilles) | conforme, quatre mutants tués, gain ≈ 1 % du CPU ; **retiré** (« le coût vient des accès et des branches ») | `docs/PERFORMANCE_FULL.md` l. 186–188 ; session `claudeab1` [AB7] |
| Préchargement des états DSU pendant la publication | aucun gain mesurable ; **retiré** | [NP § 7] |
| `-march=x86-64-v3` / `v4` | W48, trois prises : 417 → 426 / 433 ms (ng00), 358 → 343 / 342 (ng01), 459 → 425 / 400 (ng02) : dans le bruit ; non adopté (`MHGP11_MARCH` vide) | [PROF1] |
| Mémo de descente et mémos de lane | gain ×1,28 sur l'ancien code, 2,6 % de succès en lanes ; sortis de la voie rapide | [MEMO1], [NP C3] |
| Réemploi des verticales régulières | sans gain sur ng00, gardé dans 16379 | [REUSE1] |

### 5.4 Leviers v10 non portés, et pistes nommées mais non faites

- Non portés après examen [NV § 4] : mémo « atlas » partagé par cellule (« ~0,57 M pas non terminaux à K = 5 ; gain
  borné, à remesurer ») ; MEB double Welzl + certification (`bounded_meb` ≈ 1 % du CPU) ; Kruskal par lots et
  pointeurs de saut (remplacés par le recouvrement) ; recensement flottant `SiteTree::filtered` (essayé en F6, retiré).
- Pistes écrites, non réalisées : compteurs accumulés en registres dans `extend` (≈ 300 M `checked_add` par trame),
  filtre G1 en binary64 exact (F2), pré-test flottant certifié du centre q3/q4, planification de la frontière par un
  gros nœud partagé, moins de pas non terminaux à K5, queue de publication de l'ordre 5, catalogue GPU [NV § 6].
- Relevées par les audits du 4 octobre : niveau q3 construit **avant** le rejet propriétaire (`src/catalogue/leaf.cpp`
  l. 229–233 : `sphere_of` puis `center_in_box`), à différer comme la v10 ; extrema q2 couplés pour le census ;
  partition des centres v10 au 1/64 de maille (gardes de bits u24 à revoir) [AUD, DEEP]. Aucune part de temps mesurée.
- Hors moteur : projection native sur les points et sortie plate (Python seulement).

### 5.5 Trajectoire mesurée de FULL K5, W48, ng00 u21 (sessions G4 distinctes)

| Source exécutée | Voie | FULL ms | Reçu |
|---|---|---:|---|
| `c6ca345e0` | première FULL | 21 207–21 294 | [FULL3] |
| `12f49d0ca` | classification et balayage, mode 3 | 18 974 (médiane) | [SWEEP2] |
| `c2c3e0323` | mode 3 → 7 (mémo) | 18 642 → 14 546 | [MEMO1] |
| `3dbfd1c32` | mode 7 → 15 (lots réguliers) | 14 790 → 6 163 | [FOREST3] |
| `90dd48bd2` | 15 → 63 → 127 (frontière, assemblage, passe unique) | 5 206 → 3 710 → 3 143 | [COMB3] |
| `cc93360a3` | 127 → 255 → 511 (verticales, census) | 2 929 → 1 732 → 1 588 | [CENS2] |
| `ae817d09e` | 511 → 1023 → 2047 (lookup dense, réemploi) | 1 622 → 1 436 → 1 463 | [REUSE1] |
| `895680ff8` → `c40f40798` | 2047 → 2047 → 16379 (médianes de trois) | 1 309,9 → 844,0 → 489,1 | [Q] |
| `a45daff3a` → `b87285378` | 16379 (médianes de cinq) | 446,5 → **412,4** | [AB7] |

## 6. Où casse le budget de 100 ms, chiffré

Contrat : FULL K1..5 sur trame entière sans sol, 100 ms sur G4 (48 fils). Toutes les valeurs W48 sont celles du
moteur courant b872 [AB7] sauf mention.

| Poste mesuré à W48 | Valeur | Rapport au budget |
|---|---:|---:|
| FULL (médianes) | 352–412 ms | ×3,5–4,1 |
| FULL, meilleure prise (source courante ; toutes v11) | 327,7 ms ; 320,4 ms | ×3,2–3,3 |
| Domaine seul | 218–240 ms (médianes indépendantes 219–253) | ×2,2–2,5 |
| Passe unique du catalogue seule | 167–180 ms (plage des 15 prises : 141–211) | ×1,7–1,8 |
| Forêts seules | 133–172 ms | ×1,3–1,7 |
| Résolution régulière seule | 86–116 ms | ×0,9–1,2 |
| **Tout le reste** (FULL − passe unique − résolution) | **98–116 ms** | ×1,0–1,2 |

Cinq lectures, de la plus robuste à la plus fragile :

1. **Chacun des deux grands étages est déjà au budget à lui seul** (**M**). La passe unique est au plafond SMT
   (×3,3–3,7 de W8 à W48 pour ×3 cœurs, § 2.3) : plus de fils n'y changeront rien, seul son travail CPU compte.
2. **Les « petits » étages consomment à eux seuls tout le budget** (**M**, décomposition des prises médianes) :
   préambule 19–21, tri 9–14, compactage + rangs + assemblage 11–16, résidu du domaine 12–14, contextes 8–10,
   classification 2–3, naissances 7–13, queue de publication 24–36 ms, index 0,4 ms, soit 98–116 ms. Rendre gratuites
   la passe unique et la résolution ne suffirait pas. (La queue de publication dépend du recouvrement : ce point est une
   décomposition mesurée, pas un plancher prouvé.)
3. **Squelette séquentiel de la structure actuelle** (**E**) : index + préambule (S ≈ ×4) + résidu du domaine
   (S ≈ ×2,5) + contextes (S ≈ ×5–6) + classification + naissances (S ≈ ×5–6) + publication de l'ordre 5, qui reste une
   seule tâche (41,8 / 34,3 / 46,0 ms à W1 ; 35,7–51,5 ms à W48 en étage séparé c40) = **96 / 82 / 107 ms**
   (ng00 / ng01 / ng02), avant toute passe unique ni résolution ; 103–136 ms avec les étages parallèles mineurs à leurs
   valeurs W48. La plus longue tâche de la passe unique (35 ms en local, ≈ 21 ms sur G4 au facteur local/G4 de 1,65 de
   [NP § 4]) borne en plus cette passe par le bas. Le balayage vertical de l'ordre 5 (28–39 ms à W1) suit la même chaîne.
4. **Le travail CPU doit baisser, pas seulement mieux se répartir** (**E**). FULL coûte 7,07–9,13 s à W1 ; à
   l'accélération mesurée (×20–22), 100 ms exige ≤ 2,0–2,2 s de CPU W1, soit **÷3,5–4,1** ; même à ×48 idéal, ≤ 4,8 s,
   soit ÷1,5–1,9. Le CPU W48 (10,8–13,7 s) gonfle encore de ×1,5 par rapport à W1 (§ 2.2).
5. **Aucune version n'a fait 100 ms à K5** (**M**). La v10 mesurait 204–254 ms (catalogue seul 137–164 ms, forêts et
   verticales 67–89 ms, plancher séquentiel de sa tour 33–47 ms) [DEEP, L06-04] ; l'écart v11/v10 descriptif est
   ×1,50–1,72, entre captures non appariées [AUD]. À K10, rien n'est mesuré dans le contrat (§ 4).

Conséquence pour juger une transposition : une idée ne rapproche des 100 ms que si elle (a) divise le CPU de la
passe unique et de la résolution régulière (87–90 % du temps W1 : 4,75 + 3,42 s sur 9,13 s à ng00) d'un facteur de
l'ordre de 3 à 4, **et** (b) casse le squelette séquentiel : planification de la frontière, table des supports et
contextes, naissances par ordre, publication et balayage par ordre (≈ 80–110 ms à eux seuls). Une idée qui ne touche
qu'un étage de moins de 15 ms ne change pas la conclusion.

FIN
