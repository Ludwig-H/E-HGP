# Audit v8, lentille 2/12 : la chaîne q2 (tranches 1 à 21), version contre-vérifiée

```text
phase=exploration_v8_hors_registre (audit de clôture v8, préparation v9)
backend=cpu_reference
profile=quantized_u16_input_only (moteur q2 ; élargi à 18 bits par a74e90f2)
mode=audit_independant_lecture_seule (contre-vérification adversariale)
public_status=not_claimed
GCP non utilisé. Aucune compilation, aucun ctest, aucun benchmark lancé.
```

État de référence : `origin/main` **12294241**, lu dans le worktree détaché du
bloc-notes. Tous les chemins sont relatifs à la racine du dépôt. Ce qui vient
de l'état **non commis** du worktree partagé `/workspaces/E-HGP` est étiqueté
`[NON COMMIS]`. Le worktree partagé a pour HEAD `a74e90f2`, donc treize commits
d'audit (`bf73e194` … `12294241`) derrière `origin/main`. Sa tranche indexée
non commise est bâtie sur `a74e90f2`.

Ce document reprend le rapport `02_chaine_q2.md` après contre-vérification de
chaque affirmation et de chaque chiffre. Les corrections sont intégrées au
texte. La section 9 les liste une par une.

Légende des statuts :

- **prouvé** : preuve écrite et fixture.
- **testé** : porte bornée (oracle, mutants).
- **mesuré** : reçu épinglé (commit, sha256, sorties).
- **proposé** : écrit, sans implémentation.
- **manquant**.
- **non vérifiable** : chiffre dont aucun reçu n'a été retrouvé.

## 1. Périmètre lu

### Lu par le premier auditeur et relu ici aux lignes citées

- Les 22 notes `morsehgp3D_v8/docs/P0_*.md` (dénombrées : 22 fichiers).
- `morsehgp3D_v8/PASSATION.md` (1 464 lignes) et `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md`.
- `docs/REPRISE_DEVELOPPEMENT_20260920.md`, `docs/AUDIT_V7_SYNTHESE.md`, `docs/VERROUS_ARCHITECTURE.md` (l. 230-238).
- `docs/JOURNAL_DEVELOPPEMENT_20260921.md` (l. 44-58 et la section 18 bits ajoutée par `a74e90f2`).
- README des 22 reçus q2/P0 : `p0_local_credits`, `shared_axis`, `additive_q2`, `q2_census`, `q2_prepared_bounds`, `cloud_reuse`, `wspd_front`, `wspd_q2_census`, `q2_sibling`, `q2_witness_order`, `q2_joint_r2`, `q2_terminal_pool`, `q2_front_workers`, `q2_dynamic_front`, `q2_census_resume`, `q2_front_proposals`, `q2_front_inheritance`.

### Recalculs de la contre-vérification

Tous les recalculs sont faits par `python3 -c` ou `sha256sum`.

- `receipts/q2_front_inheritance_20260917/analysis_8ui2bes5/SUMMARY.json` (sha256 `157a9eae…` confirmé) : 372 comparaisons, extraction de toutes les lignes n = 32 000 des campagnes `scale` et `parallel`.
- `scale_jzsscavz/record_0000.json` (commande et stdout décodé) : pour savoir ce que recouvre la « référence ».
- `receipts/q2_dynamic_front_20260914/qualification/analysis_v8_47i6o/SUMMARY.json` : les 20 configurations LiDAR (préfixes 8k à 50k, K5/K10, quatre workers).
- `receipts/q2_front_proposals_20260917/analysis_33ezki5_/SUMMARY.json` : sha256 `8e54885e…` confirmé.
- sha256 des onze sources à porter (§ 7.1) : tous confirmés.
- `wc -l` du périmètre q2 de `src/` ; `du -sh` des reçus ; `git log` et `git show` de `a74e90f2` et des créations de fichiers.

### Lu ici en plus du premier rapport

- Sources : `src/pipeline/wspd_q34.hpp:95-135`, `src/wspd/front.cpp:393-414`, `src/pipeline/wspd_q2_parallel.hpp:70-86`, `src/core/types.hpp` (validation), `src/pipeline/prepared_cloud.cpp:55-80`.
- CMake : `CMakeLists.txt:13-22` (bibliothèque) et `CMakeLists.txt:186-218` (portes natives).
- Audits : `audits/front_lanes_lidar_20260921/README.md` (l. 1-80), `audits/RECTANGLES_H_HA_HB_SUIVI_20260922.md` (l. 1-140), `audits/lidar_rectangles_20260922/README.md` (greps), `audits/lidar08_20260914/README.md` (l. 1-120).
- Reçus d'entrée : `receipts/lidar_ground_20260921/README.md:59` et `receipts/float32_precision_20260921/README.md` (greps).
- Canal racine `audits/COORDINATION_MORSEHGP3D_V8.md` : lignes 3548-3580 et 3690-3710.
- `[NON COMMIS]` : `git diff --cached` de `src/core/types.hpp`, `src/spindle/q2_prepared_bounds.hpp` et `src/pipeline/q2_joint_bounds.hpp`, plus la statistique des tests.
- `[NON COMMIS]` : un sondage JSON de `receipts/ground_18bits_20260922/u16_identity/only_probe_01_s00_k5_w8.json`.

### Non lu

- Les bruts `MEASURES.jsonl`, `record_*.json` (sauf un) et `COMPLETION.json` des campagnes.
- `src/pipeline/q2_census.cpp` ligne à ligne. Seule sa structure a été vérifiée : index l. 97-194, moteur l. 197-774, lots l. 774-1062, entrées l. 1168/1222/2006/2387/2710.
- Les sources des portes C++ et Python, sauf les diffs 18 bits.
- Les prototypes des auditeurs.
- Le canal de coordination hors des lignes citées.
- Aucune porte n'a été rejouée. Chaque « PASS » cité est une lecture de reçu ou de journal.

## 2. Ce qui a été fait

La chaîne q2 est la **seule voie v8 qui produit, de bout en bout, un flux
exact de supports avec les IDs intérieurs** : toutes les paires diamétrales {a,b} dont la boule a strictement
moins de Kmax sites intérieurs, avec tous les IDs intérieurs et toute la
coquille. Le flux q3/q4 se déclare « not … interior-ID payload »
(`src/pipeline/wspd_q34.hpp:100-101`). La chaîne a été construite en
21 tranches du 13 au 17 septembre. Depuis, le constructeur ne l'a plus modifiée,
sauf par `a74e90f2` (22 septembre, port 18 bits), seul commit à toucher ses
sources (`git log 3e94c868..12294241` sur les sources q2 et le front).

Elle a cependant été **mesurée depuis** :

- l'auditeur A l'a mesurée le 20 septembre sur des préfixes LiDAR 50k avec sol, à `3e94c868` (`morsehgp3D_v8/audits/front_options_lidar_20260920/README.md`) ;
- la reprise du 20 septembre a rejoué 82/82 CTests Release (`receipts/reprise_20260920/README.md`).

| Tr. | Commit | Date | Objet | Verdict mesuré |
| ---: | --- | --- | --- | --- |
| 1 | 3589a2c9 | 13/09 | Crédits locaux Pool/DualBlocks/Tubes sur UN rectangle | Alternative aux histogrammes v7 ; les nappes gardent n²/4 paires |
| 2 | 8e406f9b | 13/09 | `CreditBatch` (tri Tubes partagé), filtre axial q2 | Comparaisons de tri ÷3 exactement ; axe limité aux alignements exacts |
| 3 | f5430f57 | 13/09 | Addition des colonnes, intersection des résidus | Résidu −39,4 %, mais sélection ×4,2 plus lente |
| 4 | f4815cd4 | 13/09 | Census q2 exact, curseur Z à échappements | Premier census exact ; Shared pas toujours gagnant |
| 5 | 3c29ea1e | 14/09 | Bornes préparées `4H = D − (2z − C)²` (48 o en u16) | −4,0 à −9,5 % sur Shared (bruit du bras inchangé : −4,2 à +9,6 %) |
| 6 | 85015a8c | 14/09 | `PreparedCloud` et index global partagés | Copies globales ÷R ; terme R·\|B\| encore quadratique |
| 7 | da366f7f | 14/09 | Premier front WSPD réel, `MidpointSamples` | Moins de rectangles ; front **à trois voies** ×7,7 plus lent que Pure |
| 8 | f7edd646 | 14/09 | Front consommé directement par le census | Chaîne q2 complète ; amas quasi quadratiques |
| 9 | 39b58f37 | 14/09 | Certificat autonome du frère | Rangées ×6,2 ; aucun effet sur uniforme/terrain |
| 10 | e3af11a7 | 14/09 | Ordre `ComplementFirst` | Gain modeste sur amas ; visites amas toujours ×4,1/×4,2 |
| 11 | b2106c3c | 14/09 | Census conjoint A×B | Négatif (A/B) ou nul (A seul) ; crée `Q2JointPreparedBounds` |
| 12 | ba11e3ab | 14/09 | Pool terminal sur nœuds globaux | **Amas K10 32k : 184 s → 19 s ; croissance locale < 2** |
| 13 | b268cf6f | 14/09 | Jobs du front, workers privés (Coarse) | W4 ×3,4–3,9 à 8k ; rangées ×1,9 |
| 14 | 4e878754 | 14/09 | Redistribution `Donate` | Pas de gain général ; **première mesure q2 sur LiDAR (préfixes 8k–50k, W4)** |
| 15 | d09e2207 | 14/09 | Continuation d'ancre reprenable | Infrastructure non raccordée |
| 16 | 897085f8 | 15/09 | Détachement des frères B | Infrastructure ; peu de workers actifs |
| 17 | beee3341 | 15/09 | Équipe persistante front+census | Pas de gain ; les rangées régressent dans 17 observations sur 18 |
| 18 | 2741d614 | 15/09 | Plages d'ancres, parents Pool possédés | Pas de gain stable |
| 19 | 8d615cfd | 17/09 | Lots de singletons entrelacés | **Négatif** : ×1,005 à ×1,225 |
| 20 | 8190e7ab | 17/09 | Fenêtre de propositions 2K/4K | **×0,42–0,71 hors rangées ; rangées ×1,01–1,18** |
| 21 | 3e94c868 | 17/09 | Témoins hérités (rangs) | **×0,86–0,96 par rapport à la jumelle 2K** |
| — | a74e90f2 | 22/09 | Port 18 bits (constantes q2 en u64) | Adaptation de compilation des portes q2 ; identité u16 vérifiée sur q3/q4 seulement |

Après le 17 septembre, la priorité est passée à q3/q4. La décision de reprise le dit :
« sans prolonger la recherche de quelques pourcents sur q2 »
(`docs/REPRISE_DEVELOPPEMENT_20260920.md:19`). Le 21 septembre, elle est passée au LiDAR sans sol.

## 3. État par composant

### 3.1 Tableau de statut

| Composant | Objet mathématique | Statut | Preuve |
| --- | --- | --- | --- |
| Prédicats q2 entiers | `H=(z−a)·(b−z)` > 0 pour un intérieur strict, = 0 sur la coquille ; i64/i128 | prouvé, testé en u16 | `docs/P0_CREDITS_LOCAUX.md:48-57` ; `src/spindle/predicates.hpp` ; porte `mhgp8_p0_gate` |
| Crédits locaux Pool/DualBlocks/Tubes (T1) | Témoins universels sur toute la boîte opposée ; `h+h_a+h_b ≥ h_q` avec trois populations disjointes | prouvé (lemme Tubes `d² ≥ 100 diam²`, l. 131), testé, mesuré sur rectangle isolé | `receipts/p0_local_credits_20260913/README.md` (729 mesures) |
| Filtre axial, addition, intersection (T2–T3) | Colonnes exactes disjointes hors de l'ancre | prouvé (fixture de l'addition interdite), testé, mesuré | `docs/P0_ADDITION_ET_INTERSECTION.md` ; `receipts/additive_q2_20260913/README.md:73-79` |
| Census q2 exact (T4) | Compte strict saturé à Kmax ; curseur Z = suffixe non consommé ; collecte complète des intérieurs et de la coquille | prouvé, testé (277 cas du juge, 8 376 paires), mesuré | `docs/P0_CENSUS_Q2_PARTAGE.md:35-56,124` ; `receipts/q2_census_20260913/README.md` |
| Bornes préparées (T5, en u64 depuis a74e90f2) | `4H = D − (2z−C)²`, extrema continus exacts | prouvé, testé (1 424 cas en u16), mesuré ; **domaine 18 bits non validé à l'entrée à 12294241** (voir § 5) | `src/spindle/q2_prepared_bounds.hpp:81-92` ; `receipts/q2_prepared_bounds_20260914/README.md:104` |
| `PreparedCloud` et index global (T6) | Propriétaire immuable ; bissection au milieu ; profondeur ≤ 3·bits (54 en 18 bits) ; 2n−1 nœuds ; échappements | testé, mesuré ; refus des sites dupliqués | `src/pipeline/prepared_cloud.cpp:74` ; `receipts/cloud_reuse_20260914/README.md` |
| Front WSPD réel (T7) | `box_gap_diameter_v1`, couverture unique LL/LR/RR, rejets par voie hérités | testé (727 parcours, oracle multiprécision), mesuré | `docs/P0_FRONT_REEL.md:33-66` ; `receipts/wspd_front_20260914/README.md:39` |
| Raccord front → census `run_wspd_q2_census` (T8) | Toute paire est rejetée par certificat ou comptée contre tous les sites | testé (1 255 appels, 46 762 supports), mesuré | `docs/P0_FRONT_ET_CENSUS_Q2.md:31-54` ; `receipts/wspd_q2_census_20260914/README.md:20-21` |
| Certificat frère (T9) | Frère ≥ K sites et `Hmin > 0` rejettent l'enfant, sans crédit ajouté | prouvé (fixtures `{0,5,10,11}` K2, tangence Hmin = 0), testé, mesuré | `docs/P0_CERTIFICAT_FRERE_Q2.md:17-37` ; `receipts/q2_sibling_20260914/README.md:80-86` |
| Ordre `ComplementFirst` (T10) | Complément de B₀ sans a, puis B₀ ; l'ancre est exclue du seul comptage | prouvé (fixture B₀={0,1,2,3}³, a, 12 W = 77 sites), testé, mesuré | `docs/P0_ORDRE_TEMOINS_Q2.md:39-45,73-78` |
| Census conjoint (T11) | Extrema de H sur trois boîtes, relais singleton | prouvé, testé (5 336 appels), mesuré, **négatif** | `receipts/q2_joint_r2_20260914/README.md:33,102-127` |
| Pool terminal (T12) | `h_a+h_b` disjoints, au plus K bandes `A_i × préfixe B`, survivants recomptés à zéro | prouvé (lemme de la paire la plus courte, `docs/P0_POOL_TERMINAL_Q2.md:57-63`), testé (2 059 plans, 3 088 appels), mesuré | `receipts/q2_terminal_pool_20260914/README.md:109-146` |
| Workers Coarse (T13) | Partition des sous-arbres du front, moteurs privés | testé (TSan, 32 configurations), mesuré jusqu'à quatre workers | `receipts/q2_front_workers_20260914/README.md` |
| `Donate` (T14) | File bornée ; fin = graines épuisées ∧ file vide ∧ aucune activité | testé (TSan), mesuré (synthétique et LiDAR), **sans gain général** | `receipts/q2_dynamic_front_20260914/README.md:45-68` |
| Continuation reprenable (T15) | `M = T+V+O+A+S` transitions ; 6 272 o de pile par objet **en u16** | testé (768 reprises), mesuré sur UNE ancre | `docs/P0_CENSUS_REPRENABLE_Q2.md:81-103` |
| Détachement (T16) | Frère B non visité exporté avec compte, curseur et phase | prouvé (impossibilité d'une admission multiple), testé, mesuré sur une ancre | `docs/P0_DETACHEMENT_CENSUS_Q2.md:111-131` |
| Équipe persistante (T17) | Une fermeture commune graines et branches | testé, mesuré, **sans gain** | `docs/P0_EQUIPE_PERSISTANTE_Q2.md:155-171` |
| Plages d'ancres (T18) | Au plus A−R dons, gestion en O(A+R) | testé, mesuré, **sans gain stable** | `docs/P0_PLAGES_ANCRES_Q2.md:111-145` |
| Lots de singletons (T19) | Entrelacement d'états de 72 o | testé, mesuré, **négatif** | `docs/P0_LOTS_SINGLETON_Q2.md:72-100` |
| Fenêtre 2K/4K (T20) | Même pivot ; la fenêtre historique est incluse ; intervalles disjoints | prouvé (fixtures `K2_tangent_second`, `K2_second_in_extension`), testé (10 mutants, rejeu indépendant de 1 575 exécutions), mesuré | `docs/P0_SURPROPOSITION_TEMOINS_Q2.md:204-220` ; `receipts/q2_front_proposals_20260917/README.md:38-43` |
| Témoins hérités (T21) | **Théorème H** : `h_minimum` croît par restriction ; rangs distincts ; domination | prouvé (`docs/P0_TEMOINS_HERITES_Q2.md:36-66`, trois fixtures de 5 points, nuage de 320 sites), testé (10 mutants dont 4 non sûrs), mesuré | `receipts/q2_front_inheritance_20260917/README.md` |
| Port 18 bits des bornes q2 | `D ≤ M²` ne tient plus en u32 : constantes u64 (48 → 96 o, 96 → 192 o) | **adapté** : les portes compilent et 79 portes courtes sont déclarées vertes au journal, sans reçu. Fixtures jumelles 262 143 et validation de domaine `[NON COMMIS]`. Identité u16 **non mesurée pour q2** | `a74e90f2` ; `docs/ELARGISSEMENT_18_BITS_20260922.md:85` (version commise) |
| Registre racine des preuves | Théorème H, frère, ordre, bandes Pool, non-admission | **manquant** : `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` n'a pas été modifié depuis `f4c0734c` (5 septembre) | `git log` du registre |
| Mesure q2 sur trames entières ou sans sol | — | **manquant** | aucun q2 dans `receipts/lidar_*`, `ground_*`, `q34_spatial_*` (les masques y valent 6) |
| Clé commune, catalogue dédupliqué, tour K=1..10 | — | **manquant** | `src/forest`, `src/io`, `src/cloud` vides |

### 3.2 Utilité pour le flux q3/q4 LiDAR actuel

| Brique q2 | Utilisée par `run_wspd_q34_parallel` ? | Preuve |
| --- | --- | --- |
| `PreparedCloud`, `Q2CensusIndex` | **Oui** : c'est l'index global de toutes les voies | `src/lanes/q34_witness_search.hpp`, `edge_cover.hpp` et `q3_ball_census.hpp` incluent `q2_census.hpp` |
| Front WSPD (options par défaut) | **Oui**, fenêtre historique seulement | `src/pipeline/wspd_q34.hpp:104-106` ; le masque est limité à 2/4/6 (l. 116) |
| Plan de jobs Coarse, `run_joined_workers` | **Oui** | `src/pipeline/wspd_q34.cpp:704,752` |
| `Q2JointPreparedBounds` (créé par la tranche 11, négative) | **Oui** : bornes 4H de la recherche de témoins q3/q4 | `src/lanes/q34_witness_search.cpp:4,60,134` |
| Prédicats `h_minimum`, `xi_bounds` | **Oui** | `src/spindle/predicates.hpp:61,84` |
| Pool terminal | Non ; son extension à q3/q4 est recommandée par l'audit du 22/09 (voir la réserve au § 5) | `morsehgp3D_v8/audits/RECTANGLES_H_HA_HB_SUIVI_20260922.md:17-28` |
| Fenêtre 2K/4K, héritage | Non : refusés dès qu'une voie q3/q4 est active | `src/wspd/front.cpp:406-412` |
| Frère, ordre, conjoint, continuations, Donate, équipe, plages, lots | Non | aucun usage dans `src/lanes` ni dans `wspd_q34.cpp` (grep) |
| `local_credits`, `axis_q2`, `tube_credits` | Non, mais inclus transitivement (`q2_census.hpp:3` → `axis_q2.hpp:3` → `local_credits.hpp`) et **compilés dans la même bibliothèque statique `mhgp8_p0`** que q3/q4 | `CMakeLists.txt:32-37` |

### 3.3 Réponse à la question centrale : K = 1..10 complet des paires ?

**Oui pour le census des paires, non pour le niveau de tour.** Un appel
`run_wspd_q2_census*` à Kmax ≤ 10 émet chaque paire non ordonnée de sites
distincts dont la boule diamétrale a strictement moins de Kmax intérieurs.
Chaque émission porte la liste exacte des intérieurs, toute la coquille `H = 0`
(extrémités comprises, non plafonnée) et la clé entière `(a+b, |a−b|²)`
(`src/pipeline/q2_census.hpp:58-73`). La v7 plafonnait la coquille à 12
(`morsehgp3D_v8/PASSATION.md:656-657`). Toute autre paire est rejetée de deux
façons :

- soit par un certificat de Kmax témoins stricts distincts (front, Pool, frère) ;
- soit par le census, dont le compte sature à Kmax.

Un seul appel à Kmax = 10 fournit donc les supports diamétraux de tous les
niveaux 1..10 : le niveau se lit sur le nombre d'intérieurs. La preuve est de deux ordres :

- **testé** contre des oracles force brute bornés, par exemple 990 appels des cinq entrées, 39 450 paires et 293 788 supports, coquille maximale 24 (`receipts/q2_front_inheritance_20260917/README.md:49-50`) ;
- **apparié par empreintes** aux tailles 8k–32k, jamais par oracle exhaustif à l'échelle.

Quatre limites empêchent d'appeler cela « le niveau q2 de la tour » :

1. C'est un flux d'incidences, pas un catalogue : une même boule est émise par chacun de ses diamètres. Le cube en donne l'exemple : quatre diagonales, huit sites de coquille chacune (`docs/P0_FRONT_ET_CENSUS_Q2.md:48-52`).
2. La clé `Q2BallKey` est propre à q2 et distincte de celle des voies q3/q4. Les boules cosphériques ne sont pas dédupliquées entre voies.
3. Les sites dupliqués sont refusés (`src/pipeline/prepared_cloud.cpp:74`). À 2 cm, la fusion préalable des retours (4 247 à 4 767 par trame, `audits/lidar08_20260914/README.md:87-91`) **change l'objet HGP** : deux témoins confondus donnent une profondeur 2 avec multiplicité, mais 1 après fusion (idem l. 114-117). À 1 mm, les trois trames n'ont ni doublon ni fusion (`receipts/lidar_ground_20260921/README.md:59`).
4. La voie est séparée de l'appel q3/q4. Il n'y a ni fold, ni forêt, ni parent.

## 4. Chiffres clés

Toutes les mesures proviennent de l'hôte local partagé (AMD EPYC 9V74, 8 CPU logiques), avec une à
trois répétitions. Familles synthétiques :

- `uniform` ;
- `terrain` (nappe mince, pas un capteur) ;
- `clusters` (huit amas) ;
- `rows` (deux rangées adverses).

Aucun temps ne qualifie un contrat G4.

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| Histogrammes v7, deux amas, trois tailles | 2,04 → 8,39 → 31,08 s | `morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md:102-104` | Motif fondateur de P0 |
| Nappes, toutes méthodes T1, n32k | 256 000 000 paires restantes | `receipts/p0_local_credits_20260913/README.md:110-121` | Rectangle isolé |
| Rails q4 n2718 : Pool / Tubes | 1 846 881 / 2 916 paires | idem l. 150-158 | Contre-fixture |
| Filtre axial `sheet_full` 32k | 6 483 670 candidates, 56,68–57,20 ms | `receipts/shared_axis_20260913/README.md:76-82` | Alignements exacts |
| Grille 3D 32k : axe / Pool | 44 078 400 / 378 840 paires | idem l. 100-102 | L'axe perd hors alignement |
| Addition seule, nappe 32k | 3 928 390 candidates en 237–238 ms, contre 56–58 ms | `receipts/additive_q2_20260913/README.md:73-79` | Sélection plus lente |
| Census grille 32k K10 (intersection, individuel) | 130,6–131,9 ms | `receipts/q2_census_20260913/README.md:19-26` | Rectangle isolé |
| Bornes préparées, Shared 32k | −4,0 à −9,5 % | `receipts/q2_prepared_bounds_20260914/README.md:18-25` | Bruit −4,2 à +9,6 % sur le bras inchangé |
| Copies de restrictions, R doublant avec n | ×3,94 puis ×3,97 | `receipts/cloud_reuse_20260914/README.md:75-84` | Terme R·\|B\| quadratique démontré |
| Front seul (trois voies) uniforme 32k s8 : Pure / Samples | 4,871 / 37,375 s ; 954 257 811 pas d'index | `receipts/wspd_front_20260914/README.md:74-86` | Front à trois voies, sans census |
| Chaîne q2 T8, amas s8 : visites Z 8k/16k/32k | 0,973 / 4,199 / 17,665 G (×4,315 / ×4,207) ; 228,532 s à 32k | `receipts/wspd_q2_census_20260914/README.md:92-97` | Quasi quadratique ; **configuration égale aux défauts actuels de l'API** |
| Frère, rangées 8k | 1,494 → 0,241 s | `receipts/q2_sibling_20260914/README.md:80-86` | Aucun effet sur uniforme/terrain |
| Complement/frère, amas 8k/16k/32k | 11,378 / 43,750 / 173,471 s (visites ×4,106 / ×4,229) | `receipts/q2_witness_order_20260914/README.md:108-113` | — |
| Conjoint A/B, rangées 8k | 0,242 → 1,888 s ; rejets frère 2 138 020 → 10 (amas) | `receipts/q2_joint_r2_20260914/README.md:102-118` | Contre-résultat |
| **Pool64, amas K10 8k/16k/32k** | 13,412 / 47,179 / 184,306 → 3,589 / 7,614 / 19,180 s | `receipts/q2_terminal_pool_20260914/README.md:109-116` (sonde sha256 `67ca5595…`) | Uniforme/terrain : aucun plan sélectionné (l. 165-167) |
| Pool64, amas K5 32k | 121,708 → 7,083 s (×17,18) | idem | — |
| Exposant local des visites census, Pool64 | 1,03 à 1,57 (quatre familles, K5/K10) | idem l. 136-146 | Trois tailles, pas une borne |
| W4 sur quatre cœurs physiques, 8k K10 | ×3,91 uniforme, ×3,40 terrain, ×3,89 amas, ×1,93 rangées | `receipts/q2_front_workers_20260914/README.md:118-123` | Médianes de trois essais |
| Part des visites census portée par un worker, rangées 8k K10 | 78,8 % | idem l. 171-172 | **Corpus « quatre SMT » : deux cœurs physiques, quatre threads matériels (l. 65-66)**, pas le corpus à quatre cœurs physiques |
| Donate / Coarse W4, amas 8k | 0,857 → 1,176 s | `receipts/q2_dynamic_front_20260914/README.md:62` | Régression |
| **q2 sur préfixes LiDAR avec sol, W4, K10, 50k (trois scans)** | médianes Coarse 3,06–5,46 s ; Donate 2,64–3,72 s (plage de tous les essais : 2,60–5,51 s) | `receipts/q2_dynamic_front_20260914/README.md:64-66` ; SUMMARY `analysis_v8_47i6o` | Build T14, sans fenêtre ni héritage ; trois essais par cellule |
| **q2 sur préfixe LiDAR avec sol, W4, K5, 50k (scan 0)** | Coarse 1,485 s ; Donate 1,293 s | SUMMARY `analysis_v8_47i6o` | Une seule observation |
| Lots / Coarse, 54 comparaisons | ×1,005 à ×1,225, médiane ×1,112 | `docs/P0_LOTS_SINGLETON_Q2.md:83-86` | Négatif |
| Fenêtre 2K petits facteurs, n ≥ 8k | ×0,42–0,54 uniforme, ×0,51–0,62 amas, ×0,64–0,71 terrain, ×1,01–1,18 rangées | `receipts/q2_front_proposals_20260917/README.md:105-108` (SUMMARY sha256 `8e54885e…`) | Constante, pas exposant |
| Héritage / jumelle, fenêtre 2K | ×0,86–0,93 uniforme, ×0,88–0,96 amas, ×0,89–0,96 terrain, ×0,99–1,03 rangées | `receipts/q2_front_inheritance_20260917/README.md` (SUMMARY sha256 `157a9eae…`) | Idem |
| **Uniforme 32k K10 W1 : référence / 2K / 2K + héritage** | 29,031 / 12,126 / 10,506 s | SUMMARY `analysis_8ui2bes5`, campagne `scale`, minimum de trois | La « référence » = fenêtre historique **avec Pool64, Complement et frère** (`scale_jzsscavz/record_0000.json`), pas les défauts de l'API |
| Configuration 2K/16 + héritage, 32k, W1, K10 (amas / terrain / rangées) | 8,715 / 2,556 / 0,968 s | idem | Pour les rangées, le meilleur temps est la **référence** (0,925 s) : 2K+H les ralentit ×1,047 |
| Même configuration, 32k, W1, K5 (uniforme / amas / terrain / rangées) | 4,127 / 3,549 / 1,092 / 0,523 s | idem | Rangées : la référence vaut 0,509 s |
| Même configuration, 32k, W4, K10 (uniforme / amas / terrain / rangées) | 2,743 / 2,262 / 0,666 / 0,246 s | idem (campagne `parallel`, une observation) | Rangées : la référence vaut 0,234 s |
| Répartition des cycles, uniforme K10, 2K | descente 15,3–17,1 % ; fenêtre et tests H 25–26 % ; census 53,1–54,0 % | `docs/P0_TEMOINS_HERITES_Q2.md:14-21` | Copie jetable instrumentée |
| Croissance des visites census, configuration optimisée | ×2,03 à ×2,58 par doublement | `docs/REPRISE_DEVELOPPEMENT_20260920.md:75-76` | Trois tailles |
| LiDAR 50k **préfixe** avec sol, 2 cm, K10 mono, scan 0 : front par défaut / {2,16,true} / {4,all,true} | 9,604 / 5,307 / 4,774 s ; 1 040 133 supports | `morsehgp3D_v8/audits/front_options_lidar_20260920/README.md:43-50` (audit A sur 3e94c868) | Pool64, Complement et frère actifs dans les quatre bras ; scans 100/200 : 4,883/6,790 s (2K+H) et 4,708/4,980 s (4K+H), une observation |
| Pool sur LiDAR 50k préfixe | 13,175 → 10,899 s (−17 à −19 %) | `docs/P0_POOL_TERMINAL_RACCORD.md:82-92` (prototype A) | Prototype, pas le port |
| « LiDAR 50k préfixes : 1,45 s K5, 2,8 à 5,5 s K10 à 4 cœurs » (audit de reprise) | **traçable, approximatif** | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:64` → `receipts/q2_dynamic_front_20260914` | K10 conforme (2,6–5,5 s) ; « 1,45 » ne correspond exactement à aucune valeur (1,485 Coarse, 1,293 Donate) |
| Taille des sources q2 de `src/` | **7 906 lignes** sur 25 fichiers (8 028 avec `core/types.hpp`), dont `q2_census.cpp` 2 852 | `wc -l` à 12294241 | Le « 11 644 » du premier rapport n'est pas reproduit |
| Tests `slow` du CMake | 22, tous du périmètre q2/P0 ; **20 relisent des reçus**, 2 sont des juges natifs (`wspd_q2_proposals_gate`, `wspd_front_inheritance_gate`) | `morsehgp3D_v8/CMakeLists.txt:602-617` et `:190-199` | — |
| Volume des reçus q2/P0 | 202 Mo sur 974 Mo de `receipts/` v8 (`du`) | `du -sh` à 12294241 | Taille apparente : 192 Mo sur 934 Mo |
| Coût mémoire du passage à 18 bits (entrée u16, scène 0 sans sol) | nuage 1,19 → 2,39 Mo ; index 7,66 → 8,71 Mo | journal ajouté par `a74e90f2` (`docs/JOURNAL_DEVELOPPEMENT_20260921.md`, section 18 bits) | Mesuré sur le flux q3/q4 ; sans reçu épinglé |

## 5. Défauts, risques et dettes

### Gravité haute

1. **La voie q2 n'a jamais été mesurée sur le régime du contrat.** Le contrat actif porte sur des trames SemanticKITTI entières sans sol : 39 815, 35 491 et 45 114 sites à 2 cm (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:68`). Les seules mesures q2 sont :
   - synthétiques, de 8k à 32k ;
   - des préfixes LiDAR avec sol jusqu'à 50k, d'une seule captation par scan (`audits/lidar08_20260914/README.md:54-55`) : reçu constructeur `q2_dynamic_front_20260914` (W4) et audits A `q2_pool_bridge`, `q2_small_roots`, `front_options_lidar` (mono).

   Aucun reçu `lidar_*`, `ground_*` ou `q34_spatial_*` ne contient q2 : leurs sondes utilisent le masque 6. L'audit de reprise montre que l'élagage par témoins s'effondre sans sol pour q3/q4 (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:40-51`), et rien ne garantit que q2 y échappe. La tour exige pourtant q2 (idem l. 94-95).
2. **q2 est hors du pipeline de tour.** L'appel est séparé : `run_wspd_q34_parallel` n'accepte que les masques 2/4/6 (`src/pipeline/wspd_q34.hpp:116`). La clé `Q2BallKey` est propre, et il n'y a ni catalogue ni fold. Une tour K=1..10 impose de refaire l'intégration : front commun ou deux fronts, clé commune, déduplication, niveaux.
3. **Les défauts de l'API sont la configuration lente.** Tous les leviers gagnants sont opt-in : `pool_min_factor = 0`, `Q2SiblingMode::Disabled`, `Q2WitnessOrder::GlobalDfs`, `WspdFrontProposals{1, max, false}`. C'est vrai pour l'entrée série (`src/pipeline/wspd_q2_census.hpp:139-143`) comme pour l'entrée parallèle (`wspd_q2_parallel.hpp:76-84`) ; voir aussi `src/wspd/front.hpp:25-41`. Le « ×2,8 » (29,03 s contre 10,51 s à uniforme 32k K10) compare la fenêtre historique **déjà dotée** de Pool64, Complement et frère à la meilleure fenêtre : ce n'est pas une mesure appariée des défauts purs. Sur uniforme, ces trois leviers n'ont pas d'effet mesuré (Pool ne sélectionne rien, frère sans effet), si bien que l'ordre de grandeur reste plausible. Sur amas 32k K10, les défauts purs ont été mesurés à T8 (228,5 s), et la variante sans Pool (Complement et frère) à T12 (184,3 s). Comparés aux 8,72 s de la meilleure configuration à T21, cela donne ×26 et ×21, mais **entre révisions différentes**.

### Gravité moyenne

4. **Port 18 bits non qualifié pour q2 à l'état publié.** À `a74e90f2`/`12294241`, les portes q2 n'ont reçu qu'une adaptation de compilation : types et limites, 2 à 24 lignes par porte (`git show a74e90f2 --stat -- morsehgp3D_v8/tests`). Aucune fixture extrême à 262 143 n'exerce encore l'élargissement u64 des bornes. Le journal annonce « 79 portes courtes vertes » sans reçu, et range les fixtures jumelles dans la « suite immédiate ». Elles existent `[NON COMMIS]` : +4 753 lignes dans 62 fichiers `src`/`tests`, dont `q2_prepared_bounds_gate.cpp` +132. La campagne d'identité u16 `[NON COMMIS]` (`receipts/ground_18bits_20260922/u16_identity/`) ne contient que des sondes q3/q4 (schéma `mhgp8_wspd_q34_probe_v5`, `mask: 6`). Ni l'identité bit à bit des reçus q2, ni le coût en temps des constantes u64 ne sont mesurés.
5. **Domaine arithmétique non validé à l'entrée des primitives q2 publiées.** Depuis `a74e90f2`, `Coordinate` est un `int32` dont le stockage est plus large que le domaine certifié `[0, 262143]`. Or, à 12294241, `require_valid_box` ne vérifie que `low ≤ high`, et `Q2PreparedBounds(a, b)` ne valide pas l'ancre (`src/core/types.hpp:81-90` ; `src/spindle/q2_prepared_bounds.hpp:23-35`). Les bornes prouvées (`−12M² ≤ 4H ≤ 3M²` en i64) ne couvrent donc pas un appel direct hors domaine. Le chemin moteur n'est pas exposé, car `PreparedCloud` refuse les coordonnées hors plage. La correction (`valid_point`, `require_valid_point`, `valid_box` borné) n'existe que `[NON COMMIS]`. Les empreintes proposées au § 7.1 pour `predicates.hpp`, `q2_prepared_bounds.hpp` et `q2_joint_bounds.hpp` (`8ea1509a…`, `022c1418…`, `c1e383e5…`) sont celles des versions **sans** cette garde.
6. **Monolithe et combinatoire d'options.** Cinq entrées publiques (`src/pipeline/q2_census.cpp:1168,1222,2006,2387,2710`). L'entrée série a onze paramètres au total, dont un agrégat `WspdFrontProposals` de trois champs, soit douze réglages. `q2_census.cpp` compte 2 852 lignes, dont 1 381 (l. 1472–2852 : continuation, coopérative, plages, lots) servent des ordonnanceurs sans gain mesuré. `Donate` occupe en plus une partie de l'entrée parallèle (l. 1222–1375).
7. **Couplage de l'index global à du code hors chemin.** `q2_census.hpp:3` inclut `axis_q2.hpp`, qui inclut `local_credits.hpp`. Toutes ces sources et les cinq entrées q2 sont compilées dans la même bibliothèque statique `mhgp8_p0` que q3/q4 (`CMakeLists.txt:32-37`).
8. **Preuves hors registre.** Le théorème H, le certificat frère, la conservation de l'ordre Complement, le lemme des bandes Pool, l'impossibilité d'admission multiple et la contenance des fenêtres sont prouvés dans les notes v8, avec fixtures. Aucun ne figure dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, dont la dernière modification date du 5 septembre (`f4c0734c`).
9. **Parallélisme non démontré au-delà de quatre workers.** Aucune mesure q2 n'utilise plus de quatre threads (`threads` ∈ {1, 2, 4} dans tous les reçus q2). Le plan Coarse plafonne les rangées à ×1,93 sur quatre cœurs physiques. Le déséquilibre de 78,8 % sur un worker a été observé dans le corpus à deux cœurs physiques et quatre SMT. Pour q3/q4 sur G4, le même plan n'occupait que 1,93 à 11,13 CPU logiques sur 48 (`morsehgp3D_v8/PASSATION.md:192-195`).
10. **Coût du front dominant.** La descente depuis la racine représente 15–17 % des cycles. La reprise exacte de la descente (×0,94–0,98) n'a pas été portée (`docs/P0_TEMOINS_HERITES_Q2.md:173,181-197`). En 18 bits, la profondeur **maximale** passe de 48 à 54 (`docs/ELARGISSEMENT_18_BITS_20260922.md:37-38`, version commise). La profondeur réelle sur les nuages 1 mm n'est pas mesurée pour q2 : l'allongement de la descente reste une hypothèse.
11. **Réserve sur la généralisation du Pool à q3/q4.** L'audit du 22/09 qui la recommande mesure un prototype autonome à n = 512–1 024 (`audits/RECTANGLES_H_HA_HB_SUIVI_20260922.md:68-80`), hors tailles d'intérêt. L'audit B sur LiDAR trouve que les témoins h_a/h_b ne pèsent que 1,2–2,5 % (q3) et 1,5–3,5 % (q4) des témoins exacts (`audits/front_lanes_lidar_20260921/README.md:66-68`). L'audit natif 1 mm du 22/09 obtient ×2,55–3,95 sur le **filtre** des produits de plus de 65 536 paires, mais une sortie de paires identique (`audits/lidar_rectangles_20260922/README.md:60-71`). Le levier accélère le filtre ; il ne réduit pas le résidu aval.
12. **Format de sortie non transportable.** Les IDs de `Q2Support` sont des `std::size_t`. Les vues sont empruntées pendant un callback synchrone. La collecte est atomique, en O(n + coquille) (`docs/P0_CENSUS_REPRENABLE_Q2.md:51-53`). Il n'y a ni lots possédés, ni arène, ni format GPU.

### Gravité basse

13. Le « 1,45 s K5 » de l'audit de reprise ne correspond exactement à aucune valeur. La source la plus proche est Coarse 1,485 s (une observation) dans `receipts/q2_dynamic_front_20260914`. Il faut donc citer le reçu, pas l'audit.
14. La convention `box_gap_diameter_v1` n'est pas comparable aux s v4 (`docs/P0_FRONT_REEL.md:35-38`). Aucun s gagnant n'est établi : s10/12 changent les visites amas de moins de 0,05 % (`receipts/wspd_q2_census_20260914/README.md:119`).
15. La tâche du front passe de 32 à 72 octets pour **toute** option, y compris le défaut utilisé par q3/q4 : +2 à +3,5 % de temps mur au microbanc du front seul (`docs/P0_TEMOINS_HERITES_Q2.md:87-92`).
16. La taille de continuation publiée (6 272 o = 49 cadres × 128 o) est une valeur u16. En 18 bits, les cadres passent à 55 et contiennent des bornes de 96 o au lieu de 48. La taille réelle n'est ni publiée ni mesurée.
17. Les 22 tests `slow` et les 202 Mo de reçus q2 alourdissent la suite et le dépôt pour une voie gelée.
18. Toutes les mesures sont faites sur un hôte partagé, avec une à trois répétitions. Les écarts de quelques pourcents ne sont pas attribuables.

## 6. Questions ouvertes

1. Quel est le coût q2 en configuration {2,16,true}, Pool64, Complement et frère sur les trois trames entières sans sol, en u16 2 cm puis en 1 mm (sans fusion à 1 mm), pour K5/K10 et W1/W8 ? C'est la mesure manquante qui décide si q2 compte dans le budget de 1 s.
2. Faut-il calculer q2 dans le **même** parcours du front que q3/q4 (masque 7, une seule descente par produit), ou dans un front q2 seul, sans Ξ ? Le compromis n'a jamais été mesuré, et `docs/P0_FRONT_ET_CENSUS_Q2.md:23-29` interdit de soustraire les deux temps.
3. Les témoins certifiés du rectangle émis (héritage, extérieurs à A∪B) peuvent-ils s'ajouter à `h_a+h_b` du Pool et au census ? La PASSATION répond « non conçu, non mesuré » (`morsehgp3D_v8/PASSATION.md:681-682`).
4. Une recherche de témoins par descente saturante bat-elle la fenêtre de rangs pour q2 ? L'audit B le montre pour q3/q4 sur LiDAR (`audits/front_lanes_lidar_20260921/README.md:69-75`) :
   - la fenêtre 2K retire 36–56 % de la masse q3 ;
   - un proposeur parfait en retirerait 82–91 %.

   Rien n'est mesuré pour q2.
5. La borne de packing de la WSPD à bissection au milieu reste une lacune de preuve (`docs/VERROUS_ARCHITECTURE.md:235-238`).
6. Le passage des constantes q2 et des boîtes à 64 bits coûte-t-il du temps ? La mémoire de l'index croît de 14 % sur la scène 0 d'après le journal ; le temps n'est pas mesuré.
7. Le saut de recherche pour les produits de masse 1 (×0,88–0,93 au prototype, « non instruit ») mérite-t-il une mesure propre ?
8. À 2 cm, la hiérarchie des sites fusionnés n'est pas celle du multiensemble des retours. Le contrat v9 doit dire s'il porte sur les sites ou sur les retours. La question disparaît à 1 mm pour les trois trames mesurées.

## 7. À porter en v9, et à ne pas reprendre

### 7.1 À porter (port explicite, épinglé, requalifié)

Le noyau cité fait 2 229 lignes :

- `prepared_cloud.*` : 212 lignes ;
- `predicates.hpp`, `q2_prepared_bounds.hpp`, `q2_joint_bounds.hpp` : 436 lignes ;
- `front.*` : 1 084 lignes ;
- `q2_node_pool.hpp` : 263 lignes ;
- `parallel/*` : 234 lignes.

Avec l'index et le moteur census de `q2_census.cpp` (l. 97-774), l'en-tête `q2_census.hpp` et les entrées série et parallèle, on arrive à environ 3 500 lignes, soit ≈ 45 % des 7 906 lignes du périmètre q2 de `src/`. Ce noyau porte tout ce qui a produit un gain mesuré.

| Quoi | Où (v8) | Pin (sha256 court à 12294241) | Pourquoi |
| --- | --- | --- | --- |
| Prédicats et bornes q2 exacts | `src/spindle/predicates.hpp`, `src/spindle/q2_prepared_bounds.hpp`, `src/pipeline/q2_joint_bounds.hpp` | `8ea1509a…`, `022c1418…`, `c1e383e5…` (**sans la garde de domaine** : reprendre `valid_point`/`require_valid_point` `[NON COMMIS]` ou la réécrire, avec une fixture hors domaine) | Identité `4H`, extrema continus exacts, déjà réutilisés par q3/q4 ; u64 pour 18 bits |
| Propriétaire et index global | `src/pipeline/prepared_cloud.*`, index de `src/pipeline/q2_census.cpp:97-194` | `764bba34…` (.cpp), `e9ce29e5…` (.hpp), `f9a3faf9…` | Une copie, échappements DFS, curseur Z sans pile ; base de toutes les voies |
| Front WSPD avec fenêtre 2K et héritage | `src/wspd/front.*` | `4ce25409…` (.cpp), `5921a333…` (.hpp) | ×0,35–0,65 hors rangées ; théorème H et domination prouvés ; extension par voie avec les trois gardes de l'audit A |
| Moteur census q2 (Shared, Complement, frère, collecte) | `src/pipeline/q2_census.cpp:197-774` | `f9a3faf9…` | Exact, qualifié par plus de 1 000 appels d'oracle ; coquille non plafonnée |
| Pool terminal par facteurs | `src/pipeline/q2_node_pool.hpp` | `49c989b0…` | ×3,7 à ×17,2 sur amas (K10/K5, 8k–32k) ; aucun plan sur uniforme/terrain ; −17 à −19 % sur un préfixe LiDAR 50k (prototype A) |
| Plan de jobs Coarse et jointure | `src/parallel/joined_workers.hpp`, `work_reduction.hpp`, `make_wspd_front_jobs` (`front.hpp:205`) | `91455d12…`, `2d7e17f4…` | Déjà partagé avec q3/q4 ; `static_assert` de taille sur la fusion des compteurs (`work_reduction.hpp:67`) |
| Juges | Rejeu indépendant du front, oracle force brute des cinq entrées, dix mutants causaux par levier, fixtures nommées | portes natives `wspd_q2_proposals_gate`, `wspd_front_inheritance_gate` (aujourd'hui étiquetées `slow`) | Le rejeu calcule le vrai contrefactuel ; les mutants non sûrs sont tués par perte de supports |
| Fixtures permanentes | `{0,5,10,11}` K2 ; cube 8 sommets ; B₀={0,1,2,3}³ + 12 W ; A={(100,0,0),(100,4,0)} K2 ; rangées `A_i=(1000,i,0)`, `B_j=(60000,j,0)` ; rails n2718 ; `A={(0,0,0),(1,0,0)}, B={(100,0,0)}` ; contre-exemple de fusion K=2 ; collision `(0,1,0)`/`(0,0,65536)` | notes P0 citées au § 3 ; `lidar08_20260914/README.md:114-117` ; `ELARGISSEMENT_18_BITS_20260922.md:83` | Chacune a tué un raccourci précis |
| Acquis v7 conservés | seuils `h_q = Kmax+2−q` ; décomposition `h+h_a+h_b` disjointe ; prédicats entiers stricts ; parallélisation par rectangles | `morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md:80-100` | Validés par toute la chaîne P0 |

### 7.2 À ne pas reprendre

| Piste | Mesure qui l'a fermée | Source |
| --- | --- | --- |
| Histogrammes locaux v7 O(\|A\|²+\|B\|²) | 2,04 → 8,39 → 31,08 s sur deux amas | `morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md:102-107` |
| Plafond de coquille 12 de la v7 | Coquilles de 30 sites exercées ; la coquille n'est pas bornée par K | `morsehgp3D_v8/PASSATION.md:656-657` |
| Filtre axial, addition, intersection | Rotation : 256 M paires conservées ; grille 3D 44 M contre 378 840 (Pool) ; le LiDAR n'a pas d'alignements | `docs/P0_PARTAGE_ET_FILTRE_AXIAL.md:97-103` ; `lidar08_20260914/README.md:8-13` |
| DualBlocks, Tubes, `CreditBatch` par rectangle isolé | Hors chemin WSPD ; plan DualBlocks de 193 ms sur grille **q4** 32k (q2 : 43 ms au total) ; remplacés par `Q2NodePoolPlan` | `receipts/p0_local_credits_20260913/README.md:104-141` |
| Fabrique de rectangle recopiant le nuage | ×R sur les préparations | `receipts/cloud_reuse_20260914/README.md` |
| Census conjoint A/B équilibré | Rangées 0,242 → 1,888 s | `receipts/q2_joint_r2_20260914/README.md:102-118` |
| Conjoint A seul (`SharedAnchors`) | Gain non concluant ; croissance des visites toujours ×4,107/×4,231 (garder ses bornes conjointes, réutilisées par q3/q4) | idem l. 121-150 |
| Lots de singletons entrelacés | ×1,005–1,225 ; une ou seize voies donnent les mêmes temps | `docs/P0_LOTS_SINGLETON_Q2.md:83-100` |
| Continuation complète par petite ancre | +38 à +45 % sur un fil (audit B) ; 6 272 o par objet en u16 | `docs/P0_PLAGES_ANCRES_Q2.md:16-20` |
| Équipe coopérative front+census | Aucun gain ; les rangées régressent dans 17 observations sur 18 | `docs/P0_EQUIPE_PERSISTANTE_Q2.md:164-167` |
| `Donate` par défaut | Amas 8k 0,857 → 1,176 s ; étendues LiDAR communes | `receipts/q2_dynamic_front_20260914/README.md:58-68` |
| Réutiliser le pivot du parent sans redescendre | ×0,94 à ×6,00 | `docs/P0_TEMOINS_HERITES_Q2.md:170` |
| Descente exacte plafonnée à K | 38 à 91 bornes conjointes par rectangle | idem l. 199-201 |
| Certificat de bloc du chemin | Dominé par la fenêtre 2K | idem l. 202 |
| Test préalable de lentille | 0–1 % de recherches évitables | `morsehgp3D_v8/PASSATION.md:1142-1143` |
| `Global` sur les seules racines singleton | Pas de gain stable sur LiDAR 50k (audit A d608cc28) | `docs/P0_POOL_TERMINAL_Q2.md:167-173` |
| Toute nouvelle micro-variante q2 isolée | Consigne répétée | `docs/REPRISE_DEVELOPPEMENT_20260920.md:18-19` |

Une ligne du premier rapport est retirée de ce tableau : « Cache de formes, passe conjointe quatre cellules ». Elle concerne l'atlas q4 (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:44-58`), pas la chaîne q2, et relève de la lentille q3/q4.

## 8. Recommandations priorisées pour la v9

1. **Mesurer q2 sur les trames entières sans sol avant tout port.**
   - Trois scènes, en u16 2 cm et en 1 mm, K5/K10, W1/W8.
   - Configuration {2,16,true}, Pool64, Complement et frère, contre les **défauts purs** de l'API, mesurés dans la même campagne.
   - Reçu épinglé.
   - Sans ce chiffre, la part de q2 dans le budget de 1 s est inconnue.
2. **Une seule entrée q2, avec pour défauts la configuration gagnante mesurée sur le régime du contrat.** Sur synthétique, 2K ralentit les rangées (×1,01–1,18). L'audit A recommande `{2,16,true}` comme référence et `{4,all,true}` comme candidat LiDAR, « sans en faire un choix universel ». Le front historique reste une cible différentielle `legacy`.
3. **Intégrer q2 à la tour.**
   - Clé exacte commune aux trois voies et IDs u32.
   - Déduplication par clé avec conservation des incidences ; niveaux lus sur le compte d'intérieurs.
   - Décider par mesure entre front partagé (masque 7) et front q2 dédié.
   - Trancher sites ou retours : question ouverte 8.
4. **Porter les primitives q2 avec la garde de domaine 18 bits.**
   - Fixture hors domaine : coordonnée négative, et 2^18.
   - Fixtures jumelles à 262 143 dans les portes q2.
   - Campagne d'identité u16 appariée `3e94c868` contre le port, sur les reçus q2 d'échelle 8k/16k/32k.
   - Mesure du coût en temps des constantes u64.
5. **Porter l'héritage de témoins par voie** (un niveau par rang, q4 ⊂ q3 ⊂ q2). Les trois gardes d'A servent de fixtures d'entrée : promotion, liste après recherche sautée, extension vide. Ajouter un juge indépendant des bornes Ξ.
6. **Généraliser le Pool par facteurs à q3/q4 uniquement comme accélérateur du filtre des gros produits.** L'architecture `Q2NodePoolPlan` sert de base, avec des seuils distincts par voie et des bandes par tuples de crédits. Les mesures LiDAR (audit B ; `lidar_rectangles_20260922`) ne montrent aucune réduction du résidu. Mesurer à 8k/16k/32k et sur trames entières, pas à n ≤ 1 024.
7. **Combiner `h` (témoins hérités du rectangle émis) avec `h_a` et `h_b` dans le Pool.** Écrire la preuve de disjonction, graver une fixture, puis mesurer.
8. **Remplacer Coarse par un partage de rectangles ou de plages dans une équipe persistante.**
   - Chronos par worker.
   - Mutants : job perdu, job dupliqué, fusion décalée.
   - Mesures de W1 à W48 : aucune mesure q2 n'existe au-delà de W4.
9. **Inscrire les théorèmes q2 au registre** `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant le port : théorème H, frère, ordre, bandes Pool, non-admission, fenêtres.
10. **Découpler l'index global** des modules `axis_q2` et `local_credits`, et le renommer. Séparer les bibliothèques. Ne pas porter T1–T3, T11 (sauf `Q2JointPreparedBounds`), ni T14–T19.
11. **Hygiène des tests et du dépôt.**
    - Portes q2 v9 courtes.
    - Garder les deux juges natifs (en version courte) ; ne pas porter les 20 portes `slow` de relecture de reçus historiques.
    - Reçus volumineux hors dépôt, ou compressés avec leur hash.
12. **Réévaluer la reprise exacte de la descente** (×0,94–0,98 à 48 niveaux) après mesure de la profondeur réelle de l'index sur les trames 1 mm.

## 9. Contre-vérification

### 9.1 Affirmations principales (top_claims)

| # | Affirmation du premier auditeur | Verdict | Constat |
| ---: | --- | --- | --- |
| 1 | Émission exacte de toutes les paires à moins de Kmax intérieurs, avec intérieurs, coquille complète et clé `(a+b, \|a−b\|²)` | confirmé | `q2_census.hpp:58-73` ; `P0_FRONT_ET_CENSUS_Q2.md:31-54` ; 990 appels, 293 788 supports (`q2_front_inheritance…/README.md:49-50`). Le census rejette aussi par compte saturé. |
| 2 | Le flux n'est pas un niveau de tour | confirmé | `P0_FRONT_ET_CENSUS_Q2.md:48-52` ; `prepared_cloud.cpp:74`. Ajout : à 2 cm, la fusion change l'objet (`lidar08…/README.md:114-117`) ; à 1 mm, aucune fusion. |
| 3 | Pool terminal 184,3 → 19,2 s, 121,7 → 7,1 s ; exposants 1,03–1,57 | confirmé | `q2_terminal_pool…/README.md:109-146`. |
| 4 | Fenêtre 2K ×0,42–0,54 / 0,51–0,62 / 0,64–0,71 ; rangées ×1,01–1,18 | confirmé | `q2_front_proposals…/README.md:105-108` ; SUMMARY `8e54885e…` recalculé. |
| 5 | Théorème H ; ×0,86–0,96 ; 29,03 → 12,13 → 10,51 s | confirmé | SUMMARY `157a9eae…` : 29 030,6 / 12 126,0 / 10 506,3 ms. |
| 6 | « Les gains sont des constantes, pas des exposants » | corrigé | Vrai pour T20/T21. Faux pour T12 : sur amas, le Pool fait passer la croissance des visites de ×4,1–4,2 à ×2,7–3,0 par doublement (`q2_terminal_pool…/README.md:148-151`). |
| 7 | Jamais mesurée sur le régime du contrat ; seules mesures LiDAR = préfixes avec sol jusqu'à 50k | confirmé | Le premier rapport omet le reçu constructeur `q2_dynamic_front_20260914` (LiDAR 8k–50k, K5/K10, W4). |
| 8 | q2 hors de l'appel q3/q4 | confirmé | Citation plus forte : `wspd_q34.hpp:116` (masques 2/4/6). |
| 9 | Briques q2 réutilisées par q3/q4 | confirmé | `q34_witness_search.cpp:4,60,134` ; `wspd_q34.cpp:704,752`. |
| 10 | Défauts d'API lents : ×2,8 sur uniforme, jusqu'à ×21 sur amas | corrigé | La référence à 29,03 s inclut Pool64, Complement et frère (`scale_jzsscavz/record_0000.json`). Les défauts purs n'ont pas été remesurés dans cette campagne. ×21 et ×26 comparent des révisions différentes. |
| 11 | Lots de singletons ×1,005–1,225 | confirmé | `P0_LOTS_SINGLETON_Q2.md:83-86`. |
| 12 | Donate, équipe, plages, continuations et détachement sans gain stable | confirmé | Notes et reçus cités. |
| 13 | W4 ×3,4–3,9 ; rangées ×1,93 ; 78,8 % sur un worker | corrigé | Les 78,8 % viennent du corpus à deux cœurs physiques et quatre SMT (`q2_front_workers…/README.md:65-66,171-172`), pas du corpus à quatre cœurs physiques. |
| 14 | Port 18 bits : troncature corrigée, 48 → 96 o et 96 → 192 o, identité mesurée sur q3/q4 seulement ; « testé par portes » | corrigé | Tailles et identité confirmées. La note est **commise** dans `a74e90f2`, à la ligne 85 (la ligne 89 est la numérotation non commise). Les portes q2 publiées ne sont qu'adaptées. Fixtures 262 143 et garde de domaine `[NON COMMIS]`. |
| 15 | Théorèmes absents du registre racine | confirmé | Registre inchangé depuis `f4c0734c` (5/09). |
| 16 | « 1,45 s K5 ; 2,8–5,5 s K10 » non vérifiable | réfuté | Traçable à `receipts/q2_dynamic_front_20260914` : K10 W4 50k de 2,60 à 5,51 s ; K5 1,485 s (Coarse) ou 1,293 s (Donate). « 1,45 » reste approximatif. |
| 17 | Meilleure mesure LiDAR mono : 9,604 / 5,307 / 4,774 s, 1 040 133 supports | confirmé | `front_options_lidar…/README.md:43-50`. Précision : « défaut » désigne les propositions par défaut, avec Pool64, Complement et frère actifs. |

### 9.2 Chiffres (numbers)

| Chiffre | Verdict | Constat |
| --- | --- | --- |
| v7 : 2,04 → 8,39 → 31,08 s | confirmé | `AUDIT_V7_SYNTHESE.md:102-104` |
| 256 000 000 paires | confirmé | `p0_local_credits…/README.md:108-121` |
| Rails : 1 846 881 / 2 916 | confirmé | idem l. 150-158 |
| Axial : 6 483 670, 56,68–57,20 ms | confirmé | `shared_axis…/README.md:76-82` |
| Grille 3D : 44 078 400 / 378 840 | confirmé | idem l. 100-102 |
| Addition : 3 928 390, 237–238 ms | confirmé | `additive_q2…/README.md:73-79` |
| Census grille : 130,6–131,9 ms | confirmé | `q2_census…/README.md:19-26` |
| Bornes préparées : 4,0–9,5 % | confirmé | `q2_prepared_bounds…/README.md:18-25` |
| Copies : ×3,94 / ×3,97 | confirmé | `cloud_reuse…/README.md:75-84` |
| Front seul : 4,871 / 37,375 s ; 954 257 811 | confirmé | Précision : front à trois voies. |
| T8 : visites 0,973 / 4,199 / 17,665 G ; 228,532 s | confirmé | `wspd_q2_census…/README.md:92-97` |
| Frère : 1,494 → 0,241 s | confirmé | `q2_sibling…/README.md:80-86` |
| Complement : 11,378 / 43,750 / 173,471 s | confirmé | `q2_witness_order…/README.md:108-113` |
| Conjoint : 0,242 → 1,888 s ; 2 138 020 → 10 | confirmé | `q2_joint_r2…/README.md:102-118` |
| Pool64 K10 et K5 | confirmé | `q2_terminal_pool…/README.md:109-116` |
| Exposants 1,03–1,57 | confirmé | idem l. 136-146 |
| W4 : ×3,91 / 3,40 / 3,89 / 1,93 | confirmé | `q2_front_workers…/README.md:118-123` |
| 78,8 % | corrigé | Corpus « quatre SMT » (deux cœurs physiques). |
| Donate : 0,857 → 1,176 s | confirmé | `q2_dynamic_front…/README.md:62` |
| Lots : 1,005–1,225, médiane 1,112 | confirmé | `P0_LOTS_SINGLETON_Q2.md:83-86` |
| Fenêtre 2K par famille | confirmé | `q2_front_proposals…/README.md:105-108` |
| Héritage / jumelle | confirmé | `q2_front_inheritance…/README.md` et SUMMARY |
| 29,031 / 12,126 / 10,506 s | confirmé | SUMMARY recalculé ; la référence inclut Pool64, Complement et frère. |
| « Meilleure configuration » 32k K10 W1 : 8,715 / 2,556 / 0,968 s | corrigé | Valeurs exactes pour 2K/16+H, mais pour les rangées la meilleure configuration est la référence (0,925 s). |
| K5 : 4,127 / 3,549 / 1,092 / 0,523 s | corrigé | Même réserve : rangées, référence à 0,509 s. |
| W4 : 2,743 / 2,26 / 0,67 / 0,25 s | confirmé | Une observation ; rangées, référence à 0,234 s. |
| Cycles : 15,3–17,1 / 25–26 / 53,1–54,0 % | confirmé | `P0_TEMOINS_HERITES_Q2.md:14-21` |
| Croissance ×2,03–2,58 | confirmé | `REPRISE_DEVELOPPEMENT_20260920.md:75-76` |
| LiDAR 50k : 9,604 / 5,307 / 4,774 s | confirmé | `front_options_lidar…/README.md:43-50` |
| Pool, prototype A : 13,175 → 10,899 s | confirmé | `P0_POOL_TERMINAL_RACCORD.md:82-92` |
| Continuation : 6 272 o | confirmé | Valeur u16, obsolète en 18 bits (55 cadres, bornes de 96 o), non remesurée. |
| Constantes : 48 → 96 o et 96 → 192 o | confirmé | `a74e90f2` ; note commise l. 85 |
| Tâche du front : 32 → 72 o | confirmé | `P0_TEMOINS_HERITES_Q2.md:87-92` |
| Sources : 11 644 lignes ; noyau ≈ 3 400 (≈ 30 %) | corrigé | 7 906 lignes (25 fichiers de `src`). Le noyau de 2 229 lignes est confirmé. Portable ≈ 3 500, soit ≈ 45 %. |
| 22 tests `slow` | corrigé | Nombre et périmètre confirmés ; mais 2 sont des juges natifs, pas des relectures de reçus. |
| Reçus : 202 Mo sur 974 Mo | confirmé | `du -sh` |
| G4 : 4,19 / 11,13 / 1,93 CPU | confirmé | `PASSATION.md:192-195` (q3/q4, K5, W48) |
| « 1,45 s K5 ; 2,8–5,5 s K10 » | réfuté (non vérifiable → traçable) | `receipts/q2_dynamic_front_20260914` |

### 9.3 Autres corrections du texte

- « La voie q2 n'a plus été ni modifiée ni mesurée depuis » (§ 2) : **corrigé**. Elle n'a pas été modifiée par le constructeur, mais l'auditeur A l'a mesurée le 20/09, et la régression CTest de reprise (82/82) date du même jour.
- « DualBlocks 193 ms de plan à 32k » : **corrigé**. C'est la grille q4 ; sur la grille q2, DualBlocks coûte 42,975 ms au total.
- « `docs/ELARGISSEMENT_18_BITS_20260922.md:89` [MM, NON COMMIS] » : **corrigé**. Le fait est commis (l. 85 à 12294241) ; seule sa numérotation change dans l'état non commis.
- « Onze paramètres hors consommateur » : **corrigé**. Onze au total, dont le consommateur et un agrégat de trois champs.
- Recommandation « généraliser le Pool à q3/q4 » : **nuancée**. Le prototype est mesuré à n ≤ 1 024, et les mesures LiDAR ne montrent aucune réduction du résidu (§ 5, point 11).

### 9.4 Omissions ajoutées au rapport

1. Le reçu constructeur `q2_dynamic_front_20260914` est la seule mesure q2 commise sur LiDAR (préfixes 8k–50k, K5/K10, W4). C'est aussi la source du chiffre de l'audit de reprise.
2. Les primitives q2 publiées n'ont pas de garde de domaine 18 bits, ce qui affecte les pins du § 7.1. La correction n'existe que `[NON COMMIS]`.
3. Les portes q2 publiées n'exercent pas l'extrême 262 143. Le journal donne un « 79 portes vertes » sans reçu.
4. La « référence » de la campagne d'héritage n'est pas le défaut de l'API.
5. Pour les rangées, la meilleure configuration mesurée est la référence.
6. L'audit B et `lidar_rectangles_20260922` montrent sur LiDAR que h_a/h_b pèsent peu et laissent la sortie de paires identique.
7. À 2 cm, la fusion des doublons change l'objet HGP ; à 1 mm, les trois trames n'en ont aucun.
8. q2 et q3/q4 partagent la même bibliothèque statique `mhgp8_p0`.
9. La continuation en 18 bits a des cadres plus gros et plus nombreux, sans taille publiée.
10. Le coût mémoire du passage à 18 bits est mesuré au journal (nuage ×2, index +14 %), sans reçu.
11. Une primitive q2 float32 exacte existe : 3 923 requêtes contre `Fraction`, `receipts/float32_precision_20260921/README.md:35`. Elle n'est pas raccordée, et le float32 est gelé par la décision du 22/09.
12. Le worktree partagé est bâti sur `a74e90f2`, treize commits d'audit derrière `origin/main`.
