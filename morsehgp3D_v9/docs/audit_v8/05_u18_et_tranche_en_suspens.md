# Lentille 5/12 (version contre-vérifiée) — Élargissement 18 bits (`a74e90f2`) et tranche non commise « reprise u18 / atlas saturant »

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only` (objet audité), `mode=audit_lecture_seule`, `public_status=not_claimed`. GCP non utilisé. Référence publiée : worktree détaché `origin/main = 12294241`. Tout ce qui vient de la tranche en suspens est marqué **[non commis]**.

Cette version reprend le rapport `05_u18_tranche_en_suspens.md` et y intègre les corrections de la contre-vérification. Chaque correction est listée dans la section finale. La contre-vérification n'a rien construit ni exécuté : ni ctest, ni script du dépôt. Elle n'a lancé aucune commande Git mutante. Elle s'est limitée à `git log/show/diff/grep/blame/rev-parse/ls-tree`, `cat/sed/grep/stat/du/sha256sum` et `python3 -c`, ce dernier pour lire du JSON ou du XML, recalculer des bornes et reproduire le filtre logique des compteurs. Le premier auditeur a déclaré avoir lancé une fois `git write-tree` par inadvertance. Cette commande écrit des objets arbres non référencés, sans toucher aux refs, à l'index ni à l'arbre de travail. Cette déclaration n'a pas été vérifiée.

## 1. Périmètre lu

Lu intégralement ou sur les passages pertinents :

- **État publié.**
  - `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` (125 lignes, lu en entier par la contre-vérification).
  - Sources : `src/core/types.hpp`, `src/pipeline/prepared_cloud.cpp`, `src/lanes/q4_local.cpp`, `src/lanes/q4_local_partition.cpp`, `src/lanes/q4_center_map.cpp`, `src/lanes/exact_ball.cpp`, `src/lanes/q3_ball_census.cpp`, `src/lanes/q4_family.cpp`, `src/lanes/family_certificate.cpp`, `src/lanes/q34_pair_bounds.hpp`, `src/lanes/edge_cover.hpp`, `src/spindle/q2_prepared_bounds.hpp`, `src/pipeline/q2_joint_bounds.hpp`, `src/spindle/predicates.hpp`.
  - Sondes : `bench/q4_lidar_probe.cpp` (lecteur, profil), `bench/wspd_q34_probe.cpp` (qui inclut ce lecteur), `bench/dynamic_probe_common.hpp`.
  - Tests : `tests/cloud_owner_gate.cpp`, `tests/q2_census_gate.cpp` (l. 455-480), `tests/q34_witness_search_gate.cpp` (l. 326-341) ; diff de `tests/q4_center_map_gate.cpp` dans `a74e90f2`.
  - `docs/JOURNAL_DEVELOPPEMENT_20260921.md` (« Pistes écartées », « Élargissement 18 bits »), `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` (D1, phase 5, Q2), `AGENTS.md` (l. 1-30), `PASSATION.md` (l. 25-45), `audits/ETAT_COURANT.md` (l. 20-25).
- **Commits.**
  - `a74e90f2` : message, stat complète, diff `src` et tests 18 bits, blame des lignes 18 bits publiées.
  - `git log a74e90f2..origin/main` : 13 commits d'auditeur de 10:36 à 19:40 UTC, aucun fichier commun avec l'index ni avec les modifications non indexées.
  - `0e2c18ca` : journal seul.
- **Reçus publiés** : `receipts/ground_phase1_20260921/` (BASELINE, BASELINE.only et sondes JSON) ; `full.u32le` de la scène 0 (sha256 recalculé depuis `origin/main`).
- **[non commis] sources et documents.**
  - `git diff --cached` (89 fichiers), dont la totalité des diffs `src/` de `types.hpp`, `q4_local.cpp` et `q4_local_partition.cpp`.
  - `git diff` non indexé (17 fichiers, dont 9 d'autres chantiers).
  - `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` (indexé et arbre de travail).
  - Tests : `tests/u18_numeric_domain_gate.cpp` (l. 60-160), `tests/q4_saturating_atlas_gate.cpp` (l. 20-90, 120-260), `tests/q4_saturating_mutations.py` (catalogue), `tests/u18_resume_checks_test.py` (l. 60-125).
  - Bancs : `bench/run_u18_resume_checks.py` (l. 150-240), `bench/run_ground_baseline.py` (l. 40-110, 170-210), diff de `bench/run_q34_affine_checks.py`.
- **[non commis] reçus.**
  - `receipts/ground_18bits_20260922/u16_identity/` : 6 lignes, comparées une à une à `ground_phase1`, y compris par le filtre v2.
  - `receipts/u18_resume_20260922/`, tous dossiers : README, JUnit, `ctest.json`, COMPLETION, MANIFEST, `compiler.json`, `scale_*.json`.
  - Sous-dossier `ground_1mm_first/` : MANIFEST, COMPLETION, BASELINE, sonde JSON à 541 champs, fichier `time`. Empreintes recalculées.

Non lu ou non vérifié :

- Les 42 portes jumelles une par une (échantillon : `q2_prepared_bounds_gate`, `p0_gate`, `q4_local_gate`) ; `tests/ground_baseline_test.py`.
- Les dossiers `dependencies/` (227 fichiers par capture).
- `audits/COORDINATION_MORSEHGP3D_V8.md`, hors blocs ajoutés.
- L'inventaire annoncé de 415 dépendances 16 bits et les « trois contrelectures » : aucun artefact.
- Les lectures LIVE `--check-live` (« 187 fichiers ») : non rejouées.
- La v7 : hors lentille.

## 2. Ce qui a été fait (chronologie, UTC)

| date / heure | acteur | fait | preuve |
| --- | --- | --- | --- |
| 21/09 | développeur (rôle attribué par l'audit du 21/09 ; auteur non vérifié) | décision **proposée** D1 : porter le moteur entier à 18 bits pour la grille 1 mm, question Q2 soumise à l'utilisateur ; D1 annonçait « puissance q3 < 2^116 », valeur fausse (360·M^6 = 2^116,49) | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:122-135`, `:211-213`, `:221-223` |
| 22/09 06:21 | développeur | commit `a74e90f2` : 86 fichiers (+681/−289), dont 36 sous `src/` (+238/−156) ; doc, journal et passation parlent d'une « décision utilisateur du 22 septembre » | `git show --stat a74e90f2` ; `ELARGISSEMENT_18_BITS_20260922.md:3-6` ; `PASSATION.md:36-39` ; `JOURNAL…:104` |
| 22/09 06:27–06:53 | développeur | six lignes u16/2 cm W8 (3 scènes × K5/K10), sonde `build/probe_a74e90f2_18bits` (sha `e750abfc…`, `src` propre) : sorties et `logical_sha256` identiques à `ground_phase1` | **[non commis]** `receipts/ground_18bits_20260922/u16_identity/BASELINE.only.json` (schéma v1, statut `partial`, sans README ni COMPLETION) |
| 22/09 ~10:00 | constructeur | reprise sur `a74e90f2` : 42 portes jumelles 18 bits laissées non commises, gardes de domaine, option `saturate_deep` | **[non commis]** `REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:8-59` |
| 22/09 10:00–10:09 | constructeur | préflight 78/79 : juge de centre inversé dans le test, pas dans le moteur | **[non commis]** `preflight_center_oracle/PRELIGHT_GATES.xml` (horodatage 10:00:29), `README.md` |
| 22/09 10:16–10:28 | constructeur | capture R1 Release (GCC 13.3) : 134 passés, 2 échecs de harnais Python de mutations, 3 désactivés ; SAN R1 interrompue par SIGINT | **[non commis]** `release/CTEST.xml` ; `sanitize/COMPLETION.json` (`CampaignInterrupted('interrupted by signal 2')`) |
| 22/09 10:31:59–10:33:44 | constructeur | première trame 1 mm sans sol entière (08/000000, 39 885 sites, K5/s8/W8) : 104,63 s mur | **[non commis]** `ground_1mm_first/` |
| 22/09 10:34:58–10:56:53 | constructeur | captures R2 Release (GCC 13.3, 136/136) et Clang 18.1.3 ASan/UBSan (131/131) ; **les deux COMPLETION portent `status: failed`** | **[non commis]** `release_r2/`, `sanitize_r2/` |
| 22/09 10:36–19:40 | auditeur | 13 commits `audit(v8)` sur `origin/main`, dont des mesures 1 mm sur l'arbre `src` `54a6d581`, identique à celui de `a74e90f2` (vérifié) | `git log a74e90f2..origin/main` ; `git rev-parse a74e90f2:morsehgp3D_v8/src` ; `audits/lidar_rectangles_20260922/README.md:3` |

État du dépôt partagé :

- `HEAD` local = `a74e90f2`, en retard de 13 commits sur `origin/main` (`0 13`).
- L'index porte la tranche du constructeur. Les 289 sources épinglées par le MANIFEST de R2 sont identiques à l'arbre de travail et aucune n'a de modification non indexée : **l'index porte exactement les sources qualifiées par R2**.
- L'arbre de travail compte en plus 17 fichiers modifiés non indexés.
  - 8 viennent de la tranche : `AGENTS.md`, `COORDINATION_MORSEHGP3D_V8.md` et 6 docs v8, écrits entre 10:29 et 10:36, soit +182/−20.
  - 9 datent du 15/09 et viennent d'autres chantiers : v6, v7 et `COORDINATION_MORSEHGP3D_V7.md`, soit +431/−86.
- `COORDINATION_MORSEHGP3D_V8.md` mêle un bloc d'un autre acteur, non indexé et daté du 13/09, et le bloc du constructeur.
- S'y ajoutent des fichiers non suivis : brouillon float32 global du 21/09 20:15, reçus `u18_resume_20260922/*` sauf le préflight.
- Dernière écriture de la tranche : `sanitize_r2/COMPLETION.json` à 10:56:53.

## 3. État par composant

| composant | statut | preuve |
| --- | --- | --- |
| `Coordinate = int32`, `coordinate_bits = 18`, `coordinate_limit = 262143`, `max_index_depth = 54`, `index_stack_frames = 55` | publié ; testé (`static_assert` et constantes gravées) | `src/core/types.hpp:20-29` ; `tests/cloud_owner_gate.cpp:228-229` |
| refus hors [0, 262143] et clés 3×18 bits dans `prepare_cloud` | publié ; testé (acceptation à 262143, refus à 262144, à −1 et à 2^20 ; contre-fixture (0,1,0)/(0,0,65536)) | `src/pipeline/prepared_cloud.cpp:54-65` ; `tests/cloud_owner_gate.cpp:226-256` |
| index par coupes au milieu à profondeur 54 et pile 55 | publié ; **testé aux extrêmes 18 bits dans l'état publié** : rectangle [0, 262143]^3 et index de census q2 avec `check_fixture`, recherche de témoins à profondeur 54 et pile 55 | `tests/q2_census_gate.cpp:462-475` ; `tests/q34_witness_search_gate.cpp:331-341` (lignes de `a74e90f2`, `git blame`) |
| bornes q2 préparées en `uint64_t` (D = 262143² > 2^32) | publié ; prouvé en commentaire ; testé aux extrêmes 18 bits seulement **[non commis]** | `src/spindle/q2_prepared_bounds.hpp:86-92`, `src/pipeline/q2_joint_bounds.hpp:83-89` |
| réécriture des bornes avec M = 262143 | publié ; prouvé en commentaire. Environ 20 bornes des commentaires recalculées ici, toutes vraies. **La note publiée contient au moins 8 énoncés faux** (§ 5 M2) | calcul exact `python3` ; `ELARGISSEMENT…:30,43-50,113` |
| localisation du centre q3 par division longue `scaled_floor` | publié ; prouvé en commentaire pour les centres générés ; testé contre un centre rationnel Boost **[non commis]** | `src/lanes/q4_local.cpp:72-91` ; `tests/q4_local_gate.cpp:455-480` (indexé) |
| carte des centres à Q = 2^42 (profondeur ≤ 42) | publié ; prouvé (96·M²·Q² = 2^126,58 < 2^127) ; testé : profondeur 43 refusée | `src/lanes/q4_center_map.cpp:14-17` ; diff `tests/q4_center_map_gate.cpp` de `a74e90f2` |
| atlas Local28 conservé à Q = 2^20 en i64 | publié ; prouvé, marge minimale (63·M²·Q = 2^61,98 < 2^62 < 2^63) | `src/lanes/q4_local_partition.cpp:240-247` |
| lecteur `.u32le` et profil `quantized_u18_input_only` | publié ; testé par usage (1 mm entier, sha d'entrée épinglé) | `bench/q4_lidar_probe.cpp:21-88`, inclus par `bench/wspd_q34_probe.cpp:4` ; `ground_1mm_first/MANIFEST.json` |
| identité u16 avant et après le port (6 lignes W8) | mesuré **[non commis]**. Sorties et `logical_sha256` v1 identiques ; **449 compteurs v2 identiques, recomptés ici** sur les 6 lignes. Les lignes W1 et les digests des campagnes spatiales, annoncés comme référence de régression, n'ont pas été rejoués | `u16_identity/*.json` contre `ground_phase1_20260921/*.json` ; `ELARGISSEMENT…:24-26` |
| effet mémoire du port (u16, scène 0, K5, W8) | mesuré (une exécution, hôte partagé) : entrée ×2, nuage ×2, index +13,7 %, RSS 13 144 → 15 364 KiB | champ `memory` des sondes `ground_phase1` et `u16_identity` |
| gardes de domaine des fabriques publiques | **[non commis]** ; testé (porte dédiée, planchers 200/80/13/19/5) ; incomplet (`point_witness`, `universal_witness`) | diff `types.hpp` ; `tests/u18_numeric_domain_gate.cpp:70-148` |
| domaine public de `certified_inside_count` (den, \|x\|, \|y\| < 2^117 ; hors racine avant division) | **[non commis]** ; prouvé (reste doublé < 2^118) ; testé (INT128_MIN/MAX, 2^126, 2^117) | `src/lanes/q4_local.cpp:278-290` (indexé) ; `tests/u18_numeric_domain_gate.cpp:129-146` |
| option `saturate_deep` (certificat terminal ≥ K−1 XOR fragment exact) | **[non commis]**. Testé : porte bornée comparant les sorties complètes, oracle indépendant par les 4 coins de cellule, 3 mutants tués en R2 Release. Mutants **non exécutés sous sanitizers**. Gain non mesuré sur LiDAR ; option absente de toute sonde | `src/lanes/q4_local.cpp:168-267` ; `tests/q4_saturating_atlas_gate.cpp:23-173` ; `release_r2/CTEST.xml` (3 `killed`) ; `git grep saturate_deep` |
| juge de centre corrigé | **[non commis]** ; testé : l'échec du préflight venait d'une inversion d'indices dans le test | `preflight_center_oracle/q4_local_gate.cpp:469-470` contre `tests/q4_local_gate.cpp:469-470` |
| qualification R2 Release et ASan/UBSan | CTest vert (136/136, 131/131 ; `UBSAN_OPTIONS=halt_on_error=1` ; aucun `runtime error`) mais **capture close en échec** à cause d'un défaut du lecteur | `release_r2/ctest.json` ; `sanitize_r2/MANIFEST.json` (`environment`) ; `release_r2/COMPLETION.json` ; § 5 H1 |
| première trame entière 1 mm sans sol (08/000000) | mesuré, **[non commis]**. Une répétition. Charge avant : 0,24 / 1,68 / 2,06 sur 1, 5 et 15 min | `ground_1mm_first/BASELINE.only.json`, `only_time_01_s00_k5_w8.txt` |
| K10 à 1 mm, scènes 000100 et 000200 à 1 mm, W1 à 1 mm | manquant | `ground_1mm_first/MANIFEST.json` (`selected` = 1 ligne sur 9) |
| identité u16 du **binaire de la tranche** sur trames 2 cm | manquant | seul le binaire `a74e90f2` (`e750abfc…`) a été rejoué en 2 cm |
| voie q2 à 1 mm | manquant (sonde dynamique en u16 seulement ; campagnes sans sol en `mask 6`) | `bench/dynamic_probe_common.hpp:58-86` ; MANIFEST `mask: 6` |
| sanitizers sur `a74e90f2` seul | manquant (aucun reçu) | journal publié : seule une suite Release est annoncée, sans reçu |

## 4. Chiffres clés

| grandeur | valeur | source épinglée | réserve |
| --- | --- | --- | --- |
| trame 08/000000 sans sol 1 mm, K5 s8 W8 : mur | 104,63 s | **[non commis]** `ground_1mm_first/only_time_01_s00_k5_w8.txt` (sha `4e645f89…`) ; sonde `57135518…` ; entrée `0baa4de1…` (versionnée, sha recalculé) | une répétition ; flux q3/q4 seul, ni q2, ni catalogue, ni tour ; `saturate_deep` désactivé |
| même ligne : CPU | 812,82 CPU·s (812,22 user + 0,60 sys), 776 % | idem | charge avant 0,24 / 1,68 / 2,06 ; 116 292 changements de contexte involontaires |
| même ligne : RSS max | 15 124 KiB | idem | — |
| sites | 39 885 (1 mm) contre 39 815 (2 cm) | MANIFEST `pins.inputs.00.n` ; `u16_identity` ligne 1 | 70 sites fusionnés par la grille 2 cm |
| supports émis 1 mm K5 | q3 691 284 ; q4 158 496 | sonde JSON `output` (sha `89be317e…`) | digest xor `a7a095ae…`, pas un oracle géométrique |
| supports émis 2 cm K5 (même trame) | q3 663 443 ; q4 157 889 | `u16_identity` = `ground_phase1` | entrées différentes : pas une identité |
| atlas 1 mm K5 : visites / bornes de blocs / tests ponctuels / IDs copiés | 11 432 872 749 / 3 251 647 806 / 7 315 978 426 / 5 546 746 151 | sonde JSON `work.local28.atlas.partition` | rapport de compteurs, pas un profil de temps |
| balayage q4 1 mm : comparaisons de tri | 163 678 269 | sonde JSON `work.local28.sweep.sort_comparisons` | idem |
| atlas 2 cm K10 scène 0 : blocs / points / IDs / tri | 10 718 436 649 / 24 196 349 343 / 18 307 295 244 / 593 448 405 | **[non commis]** `u16_identity/only_probe_02_s00_k10_w8.json` | binaire `a74e90f2` |
| rejets q3 par l'atlas (1 mm K5) | 153 036 427 sur 168 343 794 localisations (90,9 %) ; hors domaine 20 845 ; `root_lane_skips` = 0 | sonde JSON `work.q3_atlas` | le saut au niveau racine ne sert jamais |
| 2 cm scène 0 K5 W8, binaire `a74e90f2` | 105,0 s ; 803,35 CPU·s (user+sys) ; charge avant 9,01 | `u16_identity` ligne 1 | — |
| 2 cm scène 0 K5 W8, phase 1 | 108,04 s ; **741,50 CPU·s user+sys** (740,94 user) ; charge avant 2,82 | `ground_phase1_20260921/BASELINE.only.json` ligne 1 | bruit d'hôte dominant ; aucun A/B au calme |
| 2 cm K10 W8, scènes 0/1/2, binaire `a74e90f2` | 322,04 / 260,88 / 579,82 s | `u16_identity` | charges 8,41 à 9,59 |
| mémoire retenue u16 scène 0 (phase 1 → `a74e90f2`) | entrée 238 890 → 477 780 o ; nuage 1 194 450 → 2 388 900 o ; index 7 658 552 → 8 707 128 o ; RSS 13 144 → 15 364 KiB | champ `memory` et `gnu_time` des sondes `only_probe_01` | une exécution chacune |
| écart au contrat principal | mur local 104,63 s contre 1 s, soit ≈ ×105 ; en CPU, 812,82 CPU·s contre un budget de 48 CPU·s (1 s × 48 vCPU G4), soit ≈ ×17 | ci-dessus ; `AGENTS.md:7-12` ; `PASSATION.md:28-29` | flux K5 seul ; la cible comparable est le repli K=1..5, pas K=1..10 ; ordre de grandeur seulement |
| préflight | 78/79 | `preflight_center_oracle/PRELIGHT_GATES.xml` | build mutable |
| R1 Release | 139 enregistrés : 134 passés, 2 échecs, 3 désactivés ; 481,51 s | `release/CTEST.xml`, `ctest.json` | échecs dans deux harnais Python de mutations (inventaire affine ; routage du mutant `prefix_work_omitted`) |
| R2 Release (GCC 13.3) | 136/136 passés, 3 désactivés, 464,15 s ; COMPLETION `failed` | `release_r2/CTEST.xml`, `ctest.json` | défaut du lecteur (§ 5 H1) |
| R2 Clang 18.1.3 ASan/UBSan | 131/131 passés, 8 désactivés dont les 6 harnais de mutation, 994,77 s ; COMPLETION `failed` | `sanitize_r2/CTEST.xml`, `ctest.json` | aucun mutant sous sanitizers ; diagnostic de saturation à n = 257 seulement |
| binaires R1 et R2 | `libmhgp8_p0.a` `8e7ff107…` et `mhgp8_wspd_q34_probe` `57135518…` identiques | `release/COMPLETION.json` et `release_r2/COMPLETION.json` (`compiled.outputs`) | la sonde 1 mm (build R1) est donc octet pour octet le binaire qualifié par R2 |
| diagnostic « 8k/16k/32k » de saturation | 7 tests contre 4, identiques pour n = 8 000, 16 000, 32 000 ; prepare 2,84 / 5,84 / 11,85 ms | `release_r2/scale_{8000,16000,32000}.json` | constant par construction (une seule arête, sortie q4 vide) ; aucune information d'échelle |
| compteurs logiques appariés | 444 (filtre v1) contre 449 (v2) : 5 compteurs `terminal_*` exclus par erreur ; **identité à 449 vérifiée ici** (6/6 lignes, 0 écart) | `bench/run_ground_baseline.py:53,56` (indexé) ; recomptage `python3` | pas d'artefact archivé par le constructeur |
| marge de la largeur | Local28 : 2^61,98 (18 bits) → 2^63,98 (19 bits, débordement i64) ; carte des centres : 2^126,58 → 2^128,58 ; puissance q3 à 20 bits 2^128,49 | calcul exact `python3` | 18 bits = plafond **avec les échelles Q actuelles** |
| volume de la tranche | index : 89 fichiers +7 962/−979 (src 15 fichiers +291/−24 ; tests 47 fichiers +4 462/−859 ; reçus 16 fichiers +2 181) ; non indexé : 8 fichiers de la tranche +182/−20, plus 9 fichiers d'autres chantiers +431/−86 | `git diff --cached --shortstat`, `git diff --shortstat -- <chemins>` | mélangé à d'autres acteurs dans l'arbre |
| portes jumelles 18 bits | 45 fichiers de test (47 avec `bench/`), 323 occurrences de `262143` ou `coordinate_limit` dans l'index, contre 3 fichiers publiés ; 5 CTests seulement portent le label `u18` | `git grep --cached` ; `release_r2/ctest.json` (« u18 … (5 tests) ») | les fixtures u18 tournent dans des portes étiquetées `u16` |
| volume des reçus de reprise | 23 Mo, 959 fichiers ; 227 fichiers `dependencies/` et 5,1 à 6,0 Mo par capture | `du -sh`, `ls \| wc -l` | non suivis sauf `preflight_center_oracle/` |
| diagnostic sur un préfixe de 3 000 sites (publié) | 49 584 q3, 11 537 q4, 83 % rejetés, 6,8 s | `JOURNAL_DEVELOPPEMENT_20260921.md:140-143` | déclaré « pas un reçu » : non vérifiable |
| inventaire 16 bits | « 415 dépendances, 7 cassantes » | `ELARGISSEMENT_18_BITS_20260922.md:74-89` | 7 lignes présentes ; le chiffre de 415 est non vérifiable |

## 5. Défauts, risques et dettes

### Haute

**H1 — La tranche est en suspens avec une qualification formellement en échec, à cause du lecteur.**

- `judge_xml` ne compte comme désactivés que les `<testcase>` qui ont un enfant `<skipped>` (`bench/run_u18_resume_checks.py:176`).
- CTest écrit `status="disabled"` sans enfant (`release_r2/CTEST.xml:987-992` ; aucun `<skipped` dans le fichier).
- L'ensemble lu est donc vide, il diffère de `DISABLED`, et le lecteur lève `InvalidReceipt("disabled CTests differ: …")`. Pourtant `ctest.json` affiche « 100% tests passed, 0 tests failed out of 136 » en Release et 131 passés sous ASan/UBSan, avec `halt_on_error=1`.
- La détection des échecs fonctionne : CTest écrit bien `<failure>`, comme on le voit en R1.
- Le test unitaire du lecteur fabrique ses propres `status="notrun"` avec `<skipped>` (`tests/u18_resume_checks_test.py:74-77,117-119`) : le juge n'a jamais été confronté au format réel.
- Les documents de la tranche sont antérieurs aux verdicts R2 (10:46 et 10:56) : README du reçu à 10:36:08, `REPRISE_…` à 10:35:34. Ils décrivent le protocole R2 sans en donner le résultat.

Aucune capture de la tranche ne fait autorité selon son propre protocole, et un commit en l'état publierait des README muets sur deux échecs. La gravité relève du processus : les CTests sous-jacents sont verts et les binaires épinglés.

**H2 — Contradiction documentaire sur la décision utilisateur du 22 septembre.**

- **Publié** :
  - « Décision utilisateur du 22 septembre 2026 : le contrat temps … se poursuit sur le moteur entier élargi … le profil float32 … hors contrat temps » (`ELARGISSEMENT_18_BITS_20260922.md:3-6`, `JOURNAL…:104-107`) ;
  - « aucun développement float32 pour l'instant » (`PASSATION.md:36-39`).
- **Publié mais sans trace de la décision** : `AGENTS.md:18` dit encore « Le moteur existant demeure u16 » et `audits/ETAT_COURANT.md:21-22` dit « coordonnées float32 originales par défaut ».
- **[non commis]** :
  - « La passation du développeur … donne la priorité … ne remplace pas le contrat normatif … float32 original par défaut » (diff indexé d'`ELARGISSEMENT`, l. 3-8) ;
  - `AGENTS.md` non indexé : « il ne remplace ni le profil float32 par défaut ni le contrat brut entier ».
- La réponse de l'utilisateur à Q2 (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:221-223`) n'est pas dans le dépôt.

Le profil cible de la v9 en dépend.

### Moyenne

**M1 — État publié : fabriques publiques sans garde de domaine (comportement indéfini sur entrées forgées).**

- Sur `origin/main`, `Point3` stocke des `int32` arbitraires et `valid_box` ne vérifie que l'ordre (`src/core/types.hpp:81-90`).
- Calculent sans vérifier la plage :
  - `ExactBall::make_q2/q3/q4` (`src/lanes/exact_ball.cpp:72,83,104`) ;
  - `Q4FamilySeed::make`, `Q34FamilyCertificate::make`, `PreparedPairCitronBounds`, `Q2PreparedBounds` ;
  - `Q4LocalGeometry::bounds` (`q4_local_partition.cpp:141-147`, qui ne valide que la cellule).
- La porte en suspens documente un triangle aigu à `INT32_MAX` dont W_x vaut 4·INT32_MAX^5, soit environ 157 bits (`tests/u18_numeric_domain_gate.cpp:101-105`).
- `certified_inside_count` n'exige que `den > 0` (`q4_local.cpp:227-229`) : pour den > 2^126, le reste doublé de `scaled_floor` déborde i128.
- Le pipeline n'est pas exposé : ses points viennent de `PreparedCloud` et ses centres générés restent sous 2^117.

La tranche corrige ces frontières. **Restent sans garde** :

- `point_witness` (points `a`, `b`, `z`) et `universal_witness` (points `a` et `z` ; la boîte `b` passe désormais par `require_valid_box`) dans `src/spindle/predicates.hpp:130-165` ;
- les requêtes `noexcept`, dont la précondition n'est que documentée (`REPRISE…:31-34`).

**M2 — Au moins 8 énoncés faux dans la note de conception publiée ; le code est juste.** Dans `ELARGISSEMENT_18_BITS_20260922.md` (publié) :

| énoncé publié | ligne | valeur exacte |
| --- | --- | --- |
| stockage `std::uint32_t` | 30 | `int32_t` dans le code (et dans l'annexe B, l. 101, de la même note) |
| « l'échelle passe à Q = 2^18 » | 46-47 | le code garde 2^20 ; l'annexe A, l. 95, le dit aussi |
| \|p\|, \|q\| < 2^73 | 48 | 32·M^4 < 2^77 |
| \|det\| < 2^113 | 48 | 512·M^6 < 2^117 |
| Q·\|x\| < 2^131 | 49 | 2^137 |
| disque 96·M²·Q² < 2^76 | 49-50 | 2^82,6 à Q = 2^20 ; déjà faux à 16 bits, 2^78,6 |
| « \|C\| ≤ 144·M^6 < 2^115 » | 43-44 | 144·M^6 = 2^115,17 |
| « une garde à 2^100 rend l'arithmétique totale » | 113 | faux pour un dénominateur forgé > 2^126 (M1) |

La tranche corrige 7 de ces énoncés, dont la « totalité », qu'elle reformule en l. 116-119 indexées. **Seul « 144·M^6 < 2^115 » reste faux** (l. 47 indexée et de travail).

Aucun débordement dans le code : les commentaires utilisent 360·M^6 = 2^116,49 < 2^117 (`exact_ball.cpp:98-101`, `q3_ball_census.cpp:52-56`), et `q4_family.cpp:79` écrit correctement « 144·M^6 < 2^116 ».

Un échantillon d'environ 20 bornes de commentaires a été recalculé exactement, toutes vraies. Les plus serrées sont 512·M^6 < 2^117 et 8·M² < 2^39, strictes car M < 2^18, et la somme Local28 à 2^61,98.

Leçon : une table de bornes vérifiée par machine, pas par la prose.

**M3 — Au commit publié, la couverture des extrêmes 18 bits est partielle.**

- Les extrêmes 18 bits sont testés au nuage : refus de plage, clés (`tests/cloud_owner_gate.cpp:226-256`).
- Ils le sont aussi à l'index, sur deux fixtures :
  - index de census q2 sur [0, 262143]^3, profondeur ≥ 46 et ≤ 54 avec oracle de census (`tests/q2_census_gate.cpp:462-475`) ;
  - recherche de témoins à profondeur 54 et pile 55 (`tests/q34_witness_search_gate.cpp:331-341`).
- La profondeur 42 de la carte des centres est gravée : 43 est refusée.
- Ne sont couverts aux extrêmes que par la tranche non commise : les bornes q2 préparées, les boules q3/q4, les familles, l'atlas Local28 et le centre q3. Cela représente 45 fichiers de test et 323 occurrences dans l'index, contre 3 fichiers publiés.
- Le journal publié annonce « 79 portes courtes vertes, suite complète 129 exécutées » ainsi qu'un échec isolé de `mhgp8_wspd_q34_mutations` sous charge, sans reçu. Aucun sanitizer n'a tourné sur `a74e90f2` seul.

**M4 — Validateur v1 : identité incomplète des compteurs.**

- `NONLOGICAL` v1 exclut `terminal` (`bench/run_ground_baseline.py:53`).
- Cinq compteurs n'entraient donc ni dans l'identité « 444 compteurs » du message de `a74e90f2`, ni dans les comparaisons antérieures (liste recomptée à l'identique sur `u16_identity`, `ground_phase1` et `ground_1mm_first`) :
  - `work.local28.atlas.terminal_refinements` ;
  - `work.local28.atlas.terminal_deep_cells` ;
  - `work.q4_seed_cells.terminal_pairs` ;
  - `work.witness.pairs_bounds.mixed_terminal_nodes` ;
  - `work.witness.rectangles_bounds.mixed_terminal_nodes`.
- La v2 (index) corrige le filtre (`:56,82`).
- La contre-vérification a appliqué le filtre v2, recopié à l'identique, aux JSON archivés : les 449 compteurs sont identiques sur les 6 lignes W8 (0 écart). L'identité annoncée est donc exacte. Il manque seulement l'artefact de relecture dans le reçu.

**M5 — `saturate_deep` : correct sur le papier et sur porte bornée, gain inconnu.**

Correction :

- Un bloc n'est compté intérieur que si le majorant de puissance est strictement négatif sur toute la cellule fermée ; le mutant `<= 0` (`contact_credited_inside`) est tué.
- Tant que le compte reste sous K−2, un parent n'est pas `Deep`. Un enfant ne peut donc pas hériter d'un compte ≥ K−1.
- Toute cellule qui sature à K−1 aurait eu un compte complet ≥ K−2 et serait devenue `Deep` : les cellules `Deep` sont les mêmes.
- Les décisions q3 (`*certified >= k_-1`, `wspd_q34.cpp:544-549`) et le saut racine (`:514-515`) lisent `inside_count ≥ K−1` : même verdict.
- La porte compare les sorties complètes baseline/saturé (`q4_saturating_atlas_gate.cpp:124-173`) et un oracle indépendant par les 4 coins (`:23-84`).
- Les 3 mutants sont tués en R2 Release, **pas sous sanitizers** (harnais désactivés).

Mesure : l'option n'est raccordée à aucune sonde (`git grep saturate_deep` ne la trouve que dans les tests, `q4_local.*` et `REPRISE`).

Diagnostic « 8k/16k/32k » :

- La couverture de l'arête contient les n sites : c'est la boule fermée |2z−a−b|² ≤ 4|b−a|² (`edge_cover.hpp`).
- Les compteurs sont constants parce que le cylindre intérieur de la fixture est décidé par quelques bornes de blocs (7 tests contre 4, masse non visitée de 16).
- Il n'y a donc aucune information d'échelle, ce que la tranche reconnaît elle-même (`REPRISE…:75-79`).
- Rien ne mesure la part de travail d'atlas évitable sur une trame.

**M6 — Coûts non appariés.**

- (a) Le port double `Point3` (6 → 12 octets) et `Box3` (12 → 24). Il est mesuré en mémoire mais pas en temps.
  - Mémoire (une exécution) : nuage ×2, index +13,7 %, RSS 13 144 → 15 364 KiB.
  - Temps, sur la scène 0 2 cm K5 W8, sans A/B au calme : 108,04 s / 741,50 CPU·s (phase 1, charge 2,82) contre 105,0 s / 803,35 CPU·s (`a74e90f2`, charge 9,01).
- (b) La tranche n'est pas neutre.
  - Elle retire `validate_cell` de chaque test ponctuel de partition via `bounds_unchecked` : 7,3 G tests à 1 mm K5 (diff indexé `q4_local_partition.cpp:396-399`).
  - Elle ajoute jusqu'à 12 comparaisons à chaque `require_valid_box` (deux `valid_point`), y compris dans `box_corner` et dans les prédicats q2 et fuseau.
  - La ligne 1 mm (binaire de la tranche) et les lignes 2 cm (binaire `a74e90f2`) ne mesurent donc pas le port.
  - Le binaire de la tranche n'a jamais été rejoué sur les entrées 2 cm.

**M7 — Commit de la tranche risqué dans l'arbre partagé.**

- L'index est propre au constructeur et porte exactement les 289 sources épinglées par R2 : c'est un point positif.
- L'arbre de travail contient en revanche 9 fichiers d'autres chantiers, v6 et v7 du 15/09.
- `audits/COORDINATION_MORSEHGP3D_V8.md` porte, en non indexé, un bloc AUDITEUR_COMPLEMENTAIRE du 13/09 à côté du bloc indexé du constructeur.
- Les reçus `ground_1mm_first/`, `release*/` et `sanitize*/` ainsi que le `README.md` de reprise ne sont pas suivis, alors que les textes non indexés y renvoient.

Un commit du seul index laisserait des références mortes. À l'inverse, un `git add -A` embarquerait le travail d'autrui.

### Basse

- **B1** Défauts typographiques dans les ajouts non indexés : `AGENTS.md` (« à1mm :08/000000,39885sites », « et812,82CPU·s »), `PASSATION.md` (même motif) et `CONTRAT_TRAMES_…` (« le22 septembre »). `tools/check_docs.py` et `tools/check_passation.py` n'ont pas été exécutés.
- **B2** Le profil est déduit des valeurs et non de la grille déclarée : `input_profile` rend `quantized_u16_input_only` si toutes les coordonnées tiennent sur 16 bits (`bench/q4_lidar_probe.cpp:79-84`).
- **B3** `mhgp8_wspd_q2_dynamic_probe` ne lit que `.u16le` (`bench/dynamic_probe_common.hpp:58-86`) : pas de mesure q2 à 1 mm.
- **B4** `q4_local_gate` vérifie ses planchers 18 bits en interne (`tests/q4_local_gate.cpp:705-706`, indexé) mais ne les imprime plus, alors que le préflight le faisait (l. 723-724).
- **B5** Les lecteurs portent un double inventaire implicite (historique ou u18, détecté par la présence d'un champ) (`bench/run_q34_affine_checks.py`, diff indexé).
- **B6** Les reçus sont lourds (23 Mo, 959 fichiers) et leurs lecteurs dépendent de builds locaux non versionnés, ce que le README reconnaît.
- **B7** Les entrées 2 cm sont ignorées par Git (`audits/lidar08_20260914/.gitignore:2`) et épinglées par sha seulement ; l'entrée 1 mm est versionnée.
- **B8** 18 bits est le plafond **avec les échelles Q actuelles**. À 19 bits, Local28 (2^63,98) et la carte des centres (2^128,58) débordent ; à 20 bits, la puissance q3 (2^128,49) déborde aussi.
- **B9** Les « trois contrelectures », l'inventaire des 415 dépendances et la lecture LIVE « 187 fichiers » n'ont pas d'artefact : non vérifiables.
- **B10** `u16_identity` est un reçu faible : schéma v1, statut `partial`, sans README ni COMPLETION, sonde construite hors capture épinglée (`build/probe_a74e90f2_18bits`).
- **B11** Seuls 5 CTests portent le label `u18` : `ctest -L u18` ne sélectionne pas les fixtures jumelles.
- **B12** D1 annonçait « puissance q3 < 2^116 » (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:129`), valeur fausse (2^116,49). Le port a retenu 2^117.

## 6. Questions ouvertes

1. L'utilisateur a-t-il accepté D1/Q2 le 22 septembre (contrat temps sur l'entier 18 bits, float32 hors contrat) ? La v9 doit partir d'une seule formulation (H2).
2. La tranche doit-elle être commise en v8, après correction du lecteur et captures R3, ou reprise par la v9 comme source épinglée (`a74e90f2` + sha du diff d'index) ?
3. Quelle part du travail de partition d'atlas porte sur des cellules dont le minorant atteint K−1 avant la fin ? Seule une trame entière peut répondre, avec l'option câblée dans la sonde et des sorties appariées.
4. Question du constructeur (bloc indexé de la coordination) : réutiliser pour le census q3 les seules feuilles exactes de l'atlas, avec repli sur un certificat profond K−2 non saturant ? La question n'est pas évaluée et n'a pas de preuve écrite.
5. Faut-il une frontière de type (point certifié) plutôt que des gardes par fabrique et des préconditions `noexcept` ?
6. K10 à 1 mm et les trames 000100 et 000200 à 1 mm ne sont pas mesurées. En 2 cm, K10 coûte environ ×3,1 K5 sur la scène 0 (322,04 / 105,0 s).
7. La tranche change-t-elle un compteur des mesures 1 mm de l'auditeur (13 commits sur l'arbre `a74e90f2`) ? Non vérifié, et le binaire de la tranche n'a pas été rejoué en 2 cm.

## 7. À porter en v9 et à ne pas reprendre

### À porter

| quoi | où (source) | pin | pourquoi |
| --- | --- | --- | --- |
| largeur paramétrée par `coordinate_bits`, profondeurs et piles dérivées | `src/core/types.hpp:20-29` | `a74e90f2` | aucune constante 48/49/97 en dur ; les preuves suivent M |
| refus de plage à l'entrée, clés à trois champs et contre-fixture de collision | `src/pipeline/prepared_cloud.cpp:54-65` ; `tests/cloud_owner_gate.cpp:226-256` | `a74e90f2` | défaut réel de l'ancien empaquetage (0,1,0)/(0,0,65536) |
| fixtures d'égalité de profondeur prouvée (54 coupes, pile 55) | `tests/q2_census_gate.cpp:462-475` ; `tests/q34_witness_search_gate.cpp:331-341` | `a74e90f2` | une borne prouvée gravée à l'égalité, pas un cas intérieur |
| `int32` signé plutôt que `uint32` | `ELARGISSEMENT…` annexe B | `a74e90f2` | garde la promotion signée des différences |
| localisation du centre par division longue, domaine public < 2^117, rejet hors racine avant division | `src/lanes/q4_local.cpp` (`scaled_floor`, `certified_inside_count`) | `a74e90f2` + diff indexé | évite le produit de 137 bits ; rend l'arithmétique totale |
| carte des centres à Q = 2^42 ; Local28 à Q = 2^20 | `q4_center_map.cpp:14-17` ; `q4_local_partition.cpp:240-247` | `a74e90f2` | bornes vérifiées ; conserve les compteurs u16 |
| lecteur `.u32le` à empreinte calculée sur les valeurs | `bench/q4_lidar_probe.cpp:21-55` | `a74e90f2` | même hash quel que soit le conteneur |
| porte de domaine numérique (extrêmes INT32/INT64/INT128, planchers de refus) | `tests/u18_numeric_domain_gate.cpp` | **[non commis]** ; sha à épingler au port | ferme M1 ; modèle de porte « valeurs forgées » |
| type-résultat XOR `Q4LocalPartitionResult` et ses 3 mutants | `q4_local_partition.hpp` ; `tests/q4_saturating_atlas_gate.cpp` ; `tests/q4_saturating_mutations.py` | **[non commis]** | discipline de type ; oracle indépendant par coins ; option à mesurer avant tout usage |
| grand livre de travail : préfixes interrompus inclus dans le total, sous-compteur non additionnable | `q4_local.hpp` (`Q4LocalSaturationWork`) ; `q4_local.cpp:182-191` | **[non commis]** | évite le travail caché |
| filtre logique v2 des compteurs (avec `terminal_*`) | `bench/run_ground_baseline.py:56` | **[non commis]** | ferme M4 ; identité 449 vérifiée |
| politique sanitizer `halt_on_error=1` pour ASan et UBSan, enregistrée dans le MANIFEST | `bench/run_u18_resume_checks.py:232,239` | **[non commis]** | un UBSan qui récupère silencieusement ne prouve rien |
| couplage binaire entre builds (R1 = R2 au sha près) | `release*/COMPLETION.json` | **[non commis]** | preuve utile de reproductibilité, à rendre systématique |
| première référence 1 mm entière (08/000000, K5/s8/W8) comme point de départ | `ground_1mm_first/` | entrée `0baa4de1…`, sonde `57135518…` | seule mesure 1 mm entière ; une répétition |

### À ne pas reprendre (fausses pistes fermées)

| piste | mesure ou preuve qui la ferme |
| --- | --- |
| stocker D = (e−a)² en `uint32` | 262143² = 68 718 952 449 > 2^32 : troncature silencieuse (`q2_prepared_bounds.hpp:86-90`) |
| clés `(x<<32)\|(y<<16)\|z` | collision gravée (0,1,0)/(0,0,65536) (`cloud_owner_gate.cpp:230-233`) |
| former `scale·x` pour localiser un centre q3 | 2^137 > i128 (`q4_local.cpp:72-78`, `q4_local_partition.cpp:126-130`) |
| carte des centres à Q = 2^44 | 96·M²·Q² = 2^130,6 > 2^127 |
| abaisser Local28 à Q = 2^18 | inutile : Q = 2^20 tient à 2^61,98 et garde les compteurs ; jamais codé, seulement écrit dans la note publiée |
| cache des formes de points dans la frontière | 27,32 s contre 27,32 s (quart de 7 067 sites, W1). **Journal seulement** (`JOURNAL…:47-50`, commit `0e2c18ca` sans reçu) : fermeture non vérifiable, hors tailles d'intérêt |
| classification conjointe des quatre cellules filles | +18 % (31,7 s contre 26,8 s, quart, deux répétitions). Même source, même réserve : sans reçu |
| lire le diagnostic de saturation « 8k/16k/32k » comme une mesure d'échelle | 7 contre 4 tests constants pour tout n (`release_r2/scale_*.json`) ; la tranche le dit elle-même |
| un juge de reçu testé sur un format synthétique | `<skipped>` fabriqué contre `status="disabled"` réel (H1) |
| un préfixe (3 000 sites) cité comme preuve 1 mm | journal : « Diagnostic, pas un reçu » ; contredit `AGENTS.md:12-13` |
| des bornes recopiées en prose | 8 énoncés faux dans une note de 125 lignes (M2) |

## 8. Recommandations priorisées pour la v9

1. **Trancher H2 avec l'utilisateur avant tout code v9** : profil cible du contrat, soit l'entier 18 bits sur grille déclarée, soit le float32 par défaut avec grille optionnelle. L'écrire une seule fois dans `AGENTS.md` et aligner `ETAT_COURANT`, `PASSATION` et la note.
2. **Ne pas laisser la tranche orpheline.**
   - Option A, le constructeur la commet en v8 :
     - corriger `judge_xml` pour qu'il lise `status="disabled"` ;
     - remplacer la fixture synthétique par un extrait réel de `release_r2/CTEST.xml` ;
     - relancer R3 Release et ASan/UBSan ;
     - consigner les verdicts R2 et R3 dans le README et dans `REPRISE` ;
     - corriger la typographie et « 144·M^6 < 2^115 » ;
     - faire `git pull` (13 commits d'écart, sans fichier commun), puis indexer sélectivement, jamais par `git add -A`, en vérifiant `git diff --cached` et en excluant les 9 fichiers v6/v7 et le bloc du 13/09.
   - Option B, la v9 la reprend comme source épinglée (`a74e90f2` + sha du diff d'index, qui est celui des sources R2) et la requalifie elle-même.
   - Dans tous les cas, ne jamais citer R2 comme une qualification close.
3. **Frontière de type en v9** : un type de point certifié, et de boîte, que seules la préparation du nuage et une fabrique vérifiée produisent ; les API publiques ne prennent que ce type. Cela supprime la classe M1, y compris `point_witness` et `universal_witness`, sans contrôle par point dans les boucles chaudes. Cela évite aussi les 12 comparaisons par `require_valid_box`.
4. **Table de bornes vérifiée par la machine.** Chaque borne « k·M^d < 2^b » devient un `static_assert` sur `unsigned __int128` au point d'usage, généré depuis `coordinate_bits`. La note cite la table au lieu de recopier les nombres. La largeur maximale devient un `static_assert` explicite : 18 bits avec les échelles Q actuelles (B8).
5. **Mesurer le port et la saturation par A/B appariés.**
   - Conditions : au calme, sur trames entières (08/000000, 000100, 000200), en K5 et K10, en W1 et W8, avec au moins trois répétitions.
   - Premier A/B : binaire `a74e90f2` contre binaire de la tranche sur les entrées 2 cm, pour l'identité et le coût de l'élargissement.
   - Second A/B : `saturate_deep` désactivé puis activé sur le 1 mm, avec des sorties identiques et la part de travail d'atlas évitée. L'option doit être câblée dans la sonde par un jeton explicite.
6. **Fermer les trous de mesure 1 mm** : lecteur `.u32le` pour la voie q2, campagne 1 mm complète (les 9 lignes du lanceur), profil déclaré par la grille et non déduit des valeurs.
7. **Alléger les reçus et rendre les mutants visibles** : fermetures de dépendances sous forme de liste de sha agrégée. Couplage binaire systématique entre builds. Mutants aussi sous sanitizers, ou exclusion argumentée par un reçu.
8. **Garder des portes d'extrêmes jumelles** comme règle d'extension de largeur (anciennes u16 conservées, nouvelles u18 séparées, planchers de non-vacuité). Imprimer les compteurs vérifiés et étiqueter `u18` les portes qui les portent.
9. **Vocabulaire de statut** : la tranche est « testée » (portes bornées vertes sous CTest), pas « qualifiée » tant que le lecteur de capture ne rend pas PASS. Aucune conclusion d'échelle ni de contrat n'en découle. Les fermetures de pistes sans reçu (cache des formes, passe conjointe) restent « déclarées au journal ».

## Contre-vérification

Toutes les preuves citées par le premier auditeur ont été ouvertes : fichier et ligne, commit, reçu JSON ou XML. Corrections intégrées ci-dessus :

1. **Énoncés faux de la note publiée**. Le premier auditeur en comptait 6, dont 5 corrigés par la tranche, avec « 144·M^6 < 2^115 » restant. Correction :
   - la note publiée contient au moins **8** énoncés faux : les 6 listés, plus « 144·M^6 < 2^115 » (l. 43-44) et « une garde à 2^100 rend l'arithmétique totale » (l. 113) ;
   - la tranche corrige **les 6 listés et la totalité**, soit 7 ;
   - seul « 144·M^6 < 2^115 » reste faux (l. 47 indexée) ;
   - la note se contredit aussi elle-même : `uint32_t` en l. 30 contre `int32_t` en l. 101, Q = 2^18 en l. 47 contre Q = 2^20 en l. 95.
2. **Couverture 18 bits publiée**. « testée qu'au nuage » est **réfuté dans sa forme**. `a74e90f2` grave aussi l'index de census q2 sur [0, 262143]^3 (`tests/q2_census_gate.cpp:462-475`), la profondeur 54 et la pile 55 de la recherche de témoins (`tests/q34_witness_search_gate.cpp:331-341`) et le plafond 42 de la carte des centres. Ce qui manque aux extrêmes publiés : bornes q2 préparées, boules q3/q4, familles, atlas, centre q3.
3. **Nombre de fichiers jumeaux** : 45 fichiers de test (47 avec `bench/`), et non 46 ; les 323 occurrences sont confirmées.
4. **« 449 non vérifiable »** : l'identité à 449 compteurs a été **vérifiée ici** en appliquant le filtre v2, recopié à l'identique, aux JSON archivés. Résultat : 6/6 lignes, 0 écart. Seul l'artefact du constructeur manque.
5. **CPU de phase 1** : « 740,94 CPU·s » est le temps user seul. En user+sys, comme les autres lignes, la valeur est **741,50 CPU·s**.
6. **Garde de boîte de la tranche** : `valid_box` appelle deux fois `valid_point`, soit **jusqu'à 12 comparaisons** et non six.
7. **Diagnostic de saturation** : l'explication « couverture limitée à 18 sites » est **réfutée**. La couverture de l'arête contient les n sites (`edge_cover.hpp`) ; les compteurs sont constants parce que la fixture est décidée en quelques bornes de blocs. La conclusion (aucune information d'échelle) tient.
8. **Écart au contrat** : « ≈ ×100 » est précisé en ≈ ×105 en mur local, et ≈ ×17 en CPU·s contre le budget de 48 CPU·s de `PASSATION.md:28-29`. La cible comparable pour K5 est le repli K=1..5.
9. **Volume non indexé** : les 17 fichiers ne sont pas tous de la tranche. 8 fichiers (+182/−20) en viennent ; 9 fichiers v6/v7 du 15/09 (+431/−86) appartiennent à d'autres chantiers.
10. **Risque de commit, nuance positive** : les 289 sources épinglées par R2 n'ont aucune modification non indexée. L'index porte donc exactement les sources qualifiées.
11. **Charge de l'hôte à 1 mm** : « hôte calme, charge 0,24 » est complété par la charge sur 5 et 15 minutes, 1,68 et 2,06.
12. **Échecs R1** : « lecteurs Python » est précisé en harnais Python de mutations (inventaire affine ; routage du mutant `prefix_work_omitted`, tué mais attendu sur une autre assertion). Le constructeur le dit (`REPRISE`, non indexé).
13. **« L'atlas domine le tri »** : c'est un rapport de compteurs hétérogènes, pas un profil de temps.
14. **Fausses pistes « cache des formes » et « classification conjointe »** : elles n'ont pas de reçu (commit `0e2c18ca`, journal seul). Leur fermeture est « déclarée », non vérifiable.

Ajouts (omissions du premier rapport) :

- effet mémoire du port, mesuré sur les sondes archivées ;
- mutants absents de la capture sanitizer, politique `halt_on_error=1` et absence de `runtime error` ;
- compilateurs réels : GCC 13.3 pour Release, Clang 18.1.3 pour SAN ;
- faiblesse du reçu `u16_identity` (v1, `partial`, sans README) ;
- lignes W1 et campagnes spatiales non rejouées pour l'identité u16 ;
- binaire de la tranche jamais rejoué en 2 cm ;
- seuls 5 CTests portent le label `u18` ;
- `ETAT_COURANT.md:21-22` comme pièce supplémentaire de H2 ;
- borne fausse de D1 (« < 2^116 ») ;
- ancrage `git_commit = a74e90f2` des captures alors que les sources ne sont pas commises : l'état sale est enregistré dans `worktree_status` et les sources sont épinglées par sha.

Confirmés sans changement : toutes les autres affirmations, notamment :

- bornes et marges (Local28 2^61,98, carte 2^126,58, 144·M^6 = 2^115,17, 360·M^6 = 2^116,49) ;
- gardes absentes dans l'état publié et restes non gardés après la tranche ;
- défaut `judge_xml` et test unitaire synthétique ;
- R1 et R2, binaires identiques ;
- tous les chiffres de la trame 1 mm et de `u16_identity`, empreintes recalculées ;
- inversion du juge de centre au préflight ;
- contradiction H2 ;
- 13 commits d'auditeur sans fichier commun ;
- arbre `src` `54a6d581`.
