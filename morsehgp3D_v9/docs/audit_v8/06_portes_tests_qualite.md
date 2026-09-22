# Lentille 6/12 (version contre-vérifiée) — Portes, tests, oracles et qualité du code (Morse HGP 3D v8)

Cadre : `phase=exploration_v8_hors_registre` (audit de clôture avant ouverture v9), `backend=cpu_reference`, `profile=quantized_u16_input_only` puis `quantized_u18_input_only`, `mode=audit_lecture_seule`, `public_status=not_claimed`. GCP non utilisé. Ni compilation, ni `ctest`, ni script du dépôt exécuté, ni par la lentille d'origine ni par la contre-vérification.

Référence : worktree détaché `origin/main` **12294241** (22 septembre 2026, 21:40 +0200, soit 19:40 UTC). Les chemins sont relatifs à la racine du dépôt. Ce qui vient de la tranche **non commise** du worktree partagé `/workspaces/E-HGP` est marqué **[non commis]**. Dans cette tranche, certains fichiers sont indexés et d'autres ne sont même pas indexés (`??`) ; c'est le cas de `receipts/u18_resume_20260922/{release,release_r2,sanitize,sanitize_r2}/` et de son `README.md`. Les résultats CTest cités viennent de journaux **produits par d'autres acteurs** (build `build/v9-audit-v8-head-12294241` d'une autre lentille, builds d'audit du 21 septembre). Ils ont été relus, pas rejoués.

Ce document reprend le rapport `06_portes_tests_qualite.md` avec les corrections intégrées. La section finale « Contre-vérification » liste chaque verdict.

## 1. Périmètre lu

Lentille d'origine, lu en entier ou par sections ciblées :

- `morsehgp3D_v8/CMakeLists.txt` (618 lignes, entier), `morsehgp3D_v8/cmake/fresh_run.cmake` (entier).
- `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md`, `docs/JOURNAL_DEVELOPPEMENT_20260921.md`, `docs/ELARGISSEMENT_18_BITS_20260922.md`, `audits/PORTES_ET_TESTS_20260914.md` ; `docs/AUDIT_V7_SYNTHESE.md` § 5, `docs/VERROUS_ARCHITECTURE.md` l. 418–445, `docs/FAUSSES_PISTES.md` (grep), `audits/ETAT_COURANT.md` l. 1–80, `README.md` l. 1–50 et 815–879, `PASSATION.md` (grep et sections).
- Code : `src/pipeline/wspd_q34.hpp` (options), `src/pipeline/wspd_q34.cpp` l. 1–275, `src/core/types.hpp`, `src/lanes/exact_ball.cpp`, les cinq `owned()`, `src/core/float32_predicates.hpp`, `bench/wspd_q34_probe.cpp` (`main`), `bench/run_p0_matrix.py`, `bench/run_ground_baseline.py`.
- Portes et oracles : `oracle/p0_oracle.hpp`, `oracle/q2_census_oracle.hpp`, `tests/exact_ball_oracle.hpp`, `tests/wspd_q34_gate.cpp`, `tests/q34_indexed_mutations.py`, `tests/wspd_q34_mutations.py` l. 1–120, en-têtes de `tests/wspd_q2_receipts_gate.py` et `tests/docs_scope_gate.py`.
- v7 : `morsehgp3D_v7/CMakeLists.txt`, `morsehgp3D_v7/cmake/run_expect.cmake`, `.github/workflows/morsehgp3d-v7.yml`, `tools/check_v7_receipt_publication.py`, `.github/workflows/morsehgp3d-v8-lidar-audit.yml`.
- Reçus, audits et mesures structurelles : voir le rapport d'origine.

Ajouts de la contre-vérification (lecture seule) :

- `CTestTestfile.cmake`, `Testing/Temporary/LastTest.log` et `LastTestsDisabled.log` du build `build/v9-audit-v8-head-12294241`, avec recalcul des temps par label. Le build de l'autre lentille pointe sur les sources du worktree détaché (`CMAKE_HOME_DIRECTORY`) et sur le Boost extrait sous `build/v7_boost_gate/extracted/usr/include`.
- Décompte des invocations `mhgp7_gate(` par code attendu (`morsehgp3D_v7/CMakeLists.txt`) ; `.github/workflows/ci.yml` et `tools/check_docs.py` (périmètre v8).
- Les cinq scripts de mutations u16 et les quatre scripts float32, dont les listes `MUTATIONS` ont été lues par AST. Graphe d'imports des validateurs `bench/run_*`.
- `tests/wspd_front_inheritance_gate.cpp` l. 70–92 ; `tests/*_gate.cpp` et `bench/*_probe.cpp`, pour les codes de retour, les `#define main` et `operator new`.
- `receipts/ground_phase1_20260921/BASELINE*.json` (statut et lignes) ; en-tête de `audits/q34_stream_crosscheck_t32_8k_20260921/README.md` ; horodatages et `CMakeCache.txt` de tous les builds sous `build/`.
- `git ls-tree -r -l 12294241` (reçus v7/v8, audits), `git log --numstat`, `--diff-filter` (reçus), `git diff --cached` (CMakeLists, `types.hpp`, `exact_ball.cpp`). [non commis] : `COMPLETION.json` et `CTEST.xml` des quatre captures `u18_resume_20260922`, et `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` l. 1–60.

Non lu : `audits/COORDINATION_MORSEHGP3D_V8.md` au-delà d'un grep (3 850 lignes) ; `PASSATION.md` en entier (1 464 lignes) ; le détail des 58 scripts Python de `bench/` au-delà des imports, des inventaires et des appels git ; les internes des portes float32 et LiDAR ; le contenu des 16 016 fichiers de reçus. Aucun vert n'est certifié ici.

## 2. Ce qui a été fait (appareil de test)

| date | commit | fait vérifiable |
| --- | --- | --- |
| 13 sept. | 2b658cbe | ouverture v8 ; `CMakeLists.txt` initial ; `strict_mutants` présent dans `tests/p0_gate.cpp` dès 3589a2c9 (13 sept. 18:18 UTC, `git log -G`) |
| 14 sept. | 7009ec8b (audit B), 85015a8c, b2106c3c | audit « Portes et tests » écrit après 85015a8c et complété après e3af11a7 : 34 CTests ; 16/34 échouent hors de la disposition du dépôt (`audits/PORTES_ET_TESTS_20260914.md:144`). Suivi du soir : trois fixtures de frontière stricte et plancher `strict_mutants == 3` (`tests/p0_gate.cpp:701`) ; intermittence de `mhgp8_campaign_gate` corrigée à b2106c3c |
| 14–17 sept. | tranches q2 (f7edd646 … 3e94c868) | portes C++ par brique, mutants « de modèle » dans les portes (dix dans `tests/wspd_front_inheritance_gate.cpp:74–91`), onze captures TSan de la voie q2 (`receipts/*/tsan_*`) |
| 20–21 sept. | 785d0589 … 2629a536 | voies q3/q4 : 25 portes à juge rationnel Boost, mutations compilées (`tests/*_mutations.py`) ; reçus de mutants par brique (`receipts/q34_*`, `q4_*`, `lidar_global_20260921/mutations`) |
| 21 sept. 21:36 UTC | 92d74c13 | enregistrement CTest du périmètre float32/LiDAR/spatial/mutations avec labels ; Boost optionnel ; `cmake/fresh_run.cmake` ; 132 tests |
| 21 sept. 21:51–22:39 UTC | 748ec082, 0948d2d0, 5224ff4e, 5fdda963 | atlas i64, rejet q3 par l'atlas, file bornée de plages de rectangles, chronos par worker ; cinquième mutant `parallel_refused_range_dropped` (`tests/wspd_q34_mutations.py:57–61`) |
| 22 sept. 06:21 UTC | a74e90f2 | élargissement à 18 bits ; seuls 3 fichiers de test citent la nouvelle limite. D'après la note de reprise, une série de 42 portes est restée **non commise** après ce commit (`docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:13`, [non commis]) |
| 22 sept. | bf73e194 … 12294241 | 13 commits `audit(v8): …`, dont le workflow GitHub d'audit LiDAR sans CTest (a5447d06 … 12294241) |
| 22 sept. [non commis] | — | 88 fichiers indexés sous `morsehgp3D_v8` (+7 938/−979) : portes 18 bits, `tests/u18_numeric_domain_gate.cpp`, `tests/q4_saturating_atlas_gate.cpp`, `tests/q4_saturating_mutations.py` (sixième test de mutations lié à l'arbre canonique), gardes de domaine des fabriques ; `CMakeLists.txt` porté à 139 tests |

Comptage : 169 commits touchent `morsehgp3D_v8` dans `2b658cbe^..12294241`, dont 45 sujets `audit…` ; 201 commits en tout dans la plage (tous chemins). Le chiffre de 158 donné par l'orchestration n'est pas reproduit.

Croissance de la suite (README, PASSATION, canal) : 34 CTests (14 sept.) → 72 → 91/92 (tranche 31) → 96 (21 sept.) → 132 (92d74c13). Pour comparaison, les builds locaux v7 des 10 et 11 septembre enregistrent de 431 à 464 CTests (`build/v7-audit-2026091*/`, `build/v7_cuda_cmake_20260910/`, `CTestTestfile.cmake`). Ce nombre n'a pas été rejoué.

## 3. État par composant

| composant | statut | preuve |
| --- | --- | --- |
| Enregistrement CTest : 132 tests, labels `u16` 103, `gate` 81, `receipts` 40, `slow` 31, `float32` 23, `mutation` 9, `lidar` 8 | testé (inventaire) | `morsehgp3D_v8/CMakeLists.txt` ; recompté dans `build/v9-audit-v8-head-12294241/CTestTestfile.cmake` |
| Exécution à 12294241 : 125 exécutés, 7 DISABLED, 0 échec, 466,8 s cumulés | mesuré par un autre acteur, non rejoué | `LastTest.log` (19:50–19:52 UTC). Les 7 DISABLED y ont une commande vide, 0,00 s et « Test Passed. » |
| Juges rationnels indépendants (Boost `cpp_int`/`rational`) | testé | `oracle/p0_oracle.hpp:11–13`, `oracle/q2_census_oracle.hpp:9–11`, `tests/exact_ball_oracle.hpp:11–14` n'incluent que `core/types.hpp` ; 25 portes (`CMakeLists.txt:203–217`). Réserve : `types.hpp` porte le domaine (`Coordinate`, `coordinate_limit`) partagé avec le produit |
| Planchers de non-vacuité | testé | `tests/wspd_q34_gate.cpp:942–951`. Sortie à 12294241 : 58 nuages d'oracle, 143 810 tétraèdres, 18 410 contrôles, `task_refusals` 4 443, `permutations` 2, `allocation_failures` 4 |
| Injection de fautes d'allocation et de callback | testé | 10 portes C++ remplacent `operator new` global (ex. `tests/wspd_q34_gate.cpp:16–38`), ce qui donne des échecs d'allocation déterministes. Oublié par la lentille d'origine |
| Codes de sortie exacts (0/1/2/3/4) au niveau CTest | manquant | aucun `expect_code`/`WILL_FAIL` dans `CMakeLists.txt` ; `cmake/fresh_run.cmake` transforme tout code non nul en `FATAL_ERROR`. Les codes existants ne sont pas homogènes : `wspd_q34_gate` et `wspd_q34_probe` rendent 1 pour toute erreur (`tests/wspd_q34_gate.cpp:978–981`, `bench/wspd_q34_probe.cpp:304–305`) ; 35 des 46 portes C++ rendent 2 sur erreur d'usage (ex. `tests/axis_q2_gate.cpp:425`) ; les sondes de l'ère q2 rendent 2 pour toute exception (ex. `bench/p0_probe.cpp:146`) ; aucun code 3 ni 4 n'existe. Les scripts Python vérifient `returncode == 0` (59), `== 1` (39) et `== 2` (20) dans `tests/` |
| Discrimination causale des mutants | testé (par message) | un mutant n'est « tué » que si le code vaut 1 **et** si stderr vaut exactement la ligne causale attendue (`tests/wspd_q34_mutations.py:33, 62–67, 164, 282`). C'est l'équivalent partiel de l'`EXPECT_PREFIX` de la v7 |
| Mutants causaux float32 (10 mutants, 4 tests) | testé | `tests/float32_ball_mutations.py` (3), `float32_identity_mutations.py` (2), `float32_q3_census_mutations.py` (2), `float32_index_mutation_gate.py` (3) ; exécutés à 12294241 (label `mutation` : 104,9 s) |
| Mutants causaux du moteur u16 q3/q4 (17 mutants, 5 tests) | testé seulement dans l'arbre canonique ; DISABLED ailleurs | `CMakeLists.txt:487–509` et `:587–591` (build sous `<racine des sources>/build/`, Release, sans sanitizer, Boost). Dans le build de l'autre lentille, ils sont DISABLED parce que la racine des sources est le worktree détaché. Exécutés dans les reçus `q34_affine`, `q34_indexed`, `q4_seed_cells`, `lidar_global/mutations` (mesuré) |
| Mutants témoins `q34_indexed_witness` (3) | inexécutables depuis 2629a536 ; DISABLED sans condition depuis 92d74c13 | `CMakeLists.txt:592–600`. Le motif de `admitted_lane_recounted_in_children` apparaît 2 fois dans `src/lanes/q34_witness_search.cpp` (l. 155 et 169) |
| Porte spatiale q34 | inexécutable en pratique | activée seulement si le build est exactement `/workspaces/E-HGP/build/v8_q4_seed_cells_r2_20260921` (champ `build` du manifeste épinglé, `CMakeLists.txt:510–553`), où la règle interdit `ctest` (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:119`) |
| Parallélisme W1/W2/W4 sur petits nuages contre l'oracle | testé | `tests/wspd_q34_gate.cpp:532–558` ; à 4 workers, grain 2 et file de capacité 1 (l. 454) |
| Identité W1/W8 sur trames sans sol entières (K5) | mesuré, à plancher faible | `receipts/ground_phase1_20260921/README.md:12`. Les deux reçus (`BASELINE.json`, 6 lignes, scènes 01 et 02 ; `BASELINE.only.json`, 3 lignes, scène 00) ont le statut `partial`, donc le lecteur n'exige que `identity_pairs >= 1` (`bench/run_ground_baseline.py:190`) et non `>= 3` (l. 188). Total : 3 paires K5 sur 39 815 / 35 491 / 45 114 sites. La grille ne contient pas de W1 à K10 (`bench/run_ground_baseline.py:36`) |
| ASan/UBSan | option CMake ; mesuré par tranche jusqu'au 21 sept. 10:39 UTC ; manquant pour 748ec082 → a74e90f2 seuls | `MHGP8_SANITIZE` (`CMakeLists.txt:10`) ; aucun build `MHGP8_SANITIZE=ON` entre 2026-09-21 10:39 et 2026-09-22 10:19 UTC. [non commis] `sanitize_r2` (Clang, 10:36–10:40 UTC) : a74e90f2 plus la tranche, 131 exécutés, 0 échec, 8 DISABLED |
| TSan | mesuré pour la voie q2 et une sonde q34 ; manquant pour la file bornée | pas d'option CMake (drapeaux ad hoc) ; onze dossiers `receipts/*/tsan_*`, tous q2 ; q34 : une sonde 64/K5/W4 (`receipts/lidar_global_20260921/README.md:15–16`) ; dernier build TSan 2026-09-21 06:17 UTC, avant 5224ff4e (22:36 UTC) |
| Contre-vérification à 8 000 points (auditeur B) | mesuré sur d1b4dbc6 | `audits/q34_stream_crosscheck_t32_8k_20260921/README.md:38–39`. Sonde à d1b4dbc6, donc avant l'atlas (748ec082, 0948d2d0), la file bornée (5224ff4e) et les 18 bits ; « coquilles non comparées » (l. 15) |
| Domaine 18 bits | testé partiellement ; lacune à HEAD | porte du nuage : acceptation de 262 143, refus de 262 144, collision d'empaquetage. Seuls 3 fichiers de test citent la limite 18 bits (`cloud_owner_gate.cpp`, `q2_census_gate.cpp`, `q3_q4_owner_independence_gate.py`), contre **42** qui contiennent 65 535 (40 C++, 2 Python). `ExactBall::make_q2/q3/q4` ne contrôlent pas le domaine (`src/lanes/exact_ball.cpp:72/83/104`) ; `valid_box` ne borne pas (`src/core/types.hpp:81–84`) ; correctif **[non commis]** |
| Validateurs Python des reçus (40 CTests `receipts`) | testé | 295,9 s sur 466,8 s cumulés (63 %) ; hors du label `gate` |
| Portes hermétiques (hors git, autre arborescence) | manquant | c5308651 hors git : 26/91 échecs (`build/v8-audit-c5308651.ctest.log`) ; 30 scripts appellent `git rev-parse` (29 dans `bench/`, 1 dans `tests/`) ; 73 des 101 scripts Python de `tests/` et `bench/` écrivent en dur `morsehgp3D_v8` ; non mesuré à HEAD |
| CI GitHub de la v8 | manquant (code) ; partiel (docs) | aucun workflow CTest v8 ; `.github/workflows/morsehgp3d-v8-lidar-audit.yml:37` construit `mhgp8_p0` avec `-DBUILD_TESTING=OFF`. Seul `ci.yml:62` lance `tools/check_docs.py`, qui couvre README, PASSATION et docs de la v8 (`tools/check_docs.py:92–97`) |
| API publique, CLI, aval (forêts, tour, GPU) | manquant | `cli/`, `src/forest/`, `src/gpu/`, `src/io/`, `src/tree/`, `src/cloud/` ne contiennent que `.gitkeep` ; pas de cible `install`, pas d'en-tête public |

## 4. Chiffres clés

| grandeur | valeur | source épinglée | réserve |
| --- | --- | --- | --- |
| CTests enregistrés à 12294241 | 132 | `CMakeLists.txt` ; `CTestTestfile.cmake` du build de l'autre lentille | recompté |
| Tests conditionnés à Boost | 25 portes C++ non enregistrées sans Boost ; 2 spatiales et 5 mutations u16 alors enregistrées DISABLED ; 5 sondes non construites, dont la sonde moteur `mhgp8_wspd_q34_probe` | `CMakeLists.txt:20–27`, `:119–126`, `:203–217`, `:503–505` | sans Boost : 107 enregistrés, 100 exécutables |
| Exécutés / DISABLED / échecs à 12294241 | 125 / 7 / 0 | `LastTest.log`, `LastTestsDisabled.log` | autre lentille, non rejoué |
| Temps de la suite | somme des temps par test 466,8 s ; `receipts` 295,9 s ; `-L gate -LE slow` 79 tests, 50,1 s | même journal, recalculé | somme et non mur ; le journal annonce 19 s de mur pour le cycle court (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:14`) |
| Échecs hors git | 26/91 (build de c5308651, commit du 20 sept. 22:12 UTC) ; 16/34 au 14 sept. | `build/v8-audit-c5308651.ctest.log` ; `audits/PORTES_ET_TESTS_20260914.md:144` | non mesuré à HEAD |
| Mutants dans CTest | 10 float32 actifs ; 17 u16 conditionnels, dont 3 désactivés sans condition | listes `MUTATIONS` des scripts ; `CMakeLists.txt:557–600` | v7 : 102 portes à code 4, dont 87 `--inject=` et 14 `--mutant=` |
| Portes v7 à code exact | 319 invocations `mhgp7_gate(` : 115 × 0, 6 × 1, 86 × 2, 10 × 3, 102 × 4 | `morsehgp3D_v7/CMakeLists.txt` | 20 invocations sont dans des `foreach`, donc plus de tests réels |
| Lignes produit | 17 116 (48 en-têtes, 5 911 ; 30 `.cpp`, 11 205) | `wc -l` | 9 `.gitkeep` sous `src/` (12 dans le chantier) |
| Lignes de tests | 38 421 (C++ 25 423 ; Python 12 998) | `wc -l` | — |
| Lignes de bancs et validateurs | 31 502 (C++ 10 333 ; Python 21 169, 58 scripts) | `wc -l` | — |
| Accrétion v8 | src +17 976/−851 ; tests +38 587/−166 ; bench +32 595/−475 | `git log --numstat 2b658cbe^..12294241` | — |
| Plus gros fichier | `src/pipeline/q2_census.cpp`, 2 852 lignes, 17 commits | `wc`, `git log` | suivant : `wspd_q34.cpp`, 877 lignes |
| Registre `WspdQ34Work` | 429 mots u64 | somme des `static_assert` de `src/pipeline/wspd_q34.cpp:30–262` | recalculé |
| `counter_add` dans `src/` | 1 155 lignes (1 177 occurrences, définition comprise), plus 407 macros `MHGP8_ADD*` de fusion | grep | ≥ 120 dans `work_reduction.hpp` (fusions à froid) ; part en boucle chaude non établie ; surcoût jamais mesuré |
| Copies de `owned()` | 5 | `src/lanes/q4_local.cpp:27`, `q34_cover.cpp:29`, `q34_seed.cpp:22`, `q4_window.cpp:23`, `q4_shallow.cpp:23` | l'audit du 21 sept. compte « huit définitions des prédicats de propriété » (l. 82–83) |
| Code prototype hors moteur | 1 907 lignes `.cpp` + `.hpp` (`q34_cover`, `q34_pruning`, `q34_collective`, `family_certificate`, `q4_center_map`, `q4_shallow`) | aucun appel depuis un autre fichier de `src/` (grep) | `q4_window.hpp` inclut `q4_shallow.hpp` pour ses types |
| Exceptions levées dans `src/` | 357 (152 `logic_error`, 130 `invalid_argument`, 54 `overflow_error`, 14 `out_of_range`, 5 `length_error`, 2 `domain_error`) | grep `throw std::` | pas de vocabulaire de statut transactionnel |
| Reçus v8 versionnés | 979 311 698 octets, 16 016 fichiers, 8 728 contenus distincts | `git ls-tree -r -l 12294241` | v7 : 272 869 901 octets, 23 399 fichiers |
| Doublons dans les reçus | 7 288 fichiers, 121 124 446 octets | blobs git identiques | — |
| Répartition des reçus | JSON 643,8 Mo (8 374) ; `.u32le` 73,7 Mo ; `.stdout` 67,3 Mo (1 545) ; `.gz` 65,1 Mo ; `.jsonl` 54,4 Mo ; `.f32le` 31,3 Mo ; copies de sources : 250 `.cpp`, 216 `.py`, 129 `.hpp` | `git ls-tree -l` | deux `DEFAULT_COMPATIBILITY.json` de 24,6 et 24,4 Mo |
| Manifestes | 296 `MANIFEST.json`, 75 schémas, 0 `SHA256SUMS` ; 145 identifiants `mhgp8_*_vN` dans le code | `git ls-tree`, `python3 json` | v7 : 65 `SHA256SUMS` au nom exact, plus 20 variantes |
| Chaîne de validateurs | chemin de 15 modules (`run_q4_seed_cells_checks` → … → `inheritance_source_paths`) ; fermeture de 20 modules avec cycles (`run_p0_matrix` ↔ `paired_receipts`, `*_lidar` ↔ `*_checks`), dont les validateurs des prototypes | imports de `bench/` | — |
| Inventaires figés | `len(SOURCES) == 196`, `211`, `216` | `tests/wspd_q34_mutations.py:92`, `tests/q34_affine_mutations.py:36`, `tests/q4_seed_cells_mutations.py:37`, `bench/run_q34_spatial.py:322` | comptes `== NN` (≥ 2 chiffres) : 134 dans `bench/`, 274 avec `tests/` ; le chiffre de 148 n'est pas reproduit |
| Scripts appelant `git rev-parse` | 30 | grep `bench/` (29) et `tests/` (1) | — |
| Builds locaux v8 non versionnés | 150 dossiers `build/v8*` (123 builds CMake de sources v8) et 31 journaux, 13 Go ; 39 README de reçus citent `build/v8_*` | `find`, `du`, `CMakeCache.txt` | Boost de 121 builds sur 123 : `build/v7_boost_gate/extracted/usr/include` |
| Audits v8 versionnés | 2 920 fichiers, 39,2 Mo | `git ls-tree` | — |
| Fichiers de test citant la limite 18 bits / 65 535 | 3 / 42 | grep `tests/` à 12294241 | — |
| Dernier TSan / dernier ASan avant la reprise | 2026-09-21 06:17 UTC / 2026-09-21 10:39 UTC | horodatages de `libmhgp8_p0.a` | premier ASan suivant : 2026-09-22 10:19 UTC [non commis] |
| Contre-vérification 8k (q3/q4) | K5 93 914 / 10 756 ; K10 409 195 / 116 985, identiques | `audits/q34_stream_crosscheck_t32_8k_20260921/README.md:38–39` | sonde à d1b4dbc6 ; coquilles non comparées |
| [non commis] Release R1 | 139 tests, 2 échecs (`q34_affine_mutations`, `q4_saturating_mutations`), 3 DISABLED, 481 s ; `failed` | `receipts/u18_resume_20260922/release/` (non indexé) | l'un des deux lecteurs refuse les nouveaux champs u18 (README de la reprise) |
| [non commis] Release R2 | 139 tests, 0 échec, 3 DISABLED, 464 s ; `failed` (« disabled CTests differ ») | `release_r2/CTEST.xml`, `COMPLETION.json` (non indexés) | — |
| [non commis] ASan/UBSan R1 / R2 | R1 interrompu par SIGINT ; R2 : 139 tests, 0 échec, 8 DISABLED, 994 s ; `failed` | `sanitize/`, `sanitize_r2/` (non indexés) | même refus pour R2 |

## 5. Défauts, risques et dettes

### Gravité haute

1. **Aucune CI ne construit ni ne teste le code v8, et Boost est optionnel.** Le seul workflow v8 construit la bibliothèque avec `-DBUILD_TESTING=OFF` (`.github/workflows/morsehgp3d-v8-lidar-audit.yml:37`) ; `ci.yml` ne vérifie que la documentation v8 (`tools/check_docs.py:92–97`). Sans Boost, `CMakeLists.txt:20–27` émet un `STATUS` : les 25 juges rationnels disparaissent sans échec, et la sonde moteur `mhgp8_wspd_q34_probe` n'est pas construite (elle inclut `q4_lidar_probe.cpp`, qui inclut `tests/exact_ball_oracle.hpp`). La v7 rendait Boost obligatoire (`morsehgp3D_v7/CMakeLists.txt:146–148`, `FATAL_ERROR`) et sa CI lançait `ctest -L '^gate$' --no-tests=error` avec `libboost-dev`. Localement, 121 builds v8 sur 123 utilisent un Boost 1.83 extrait sous `build/` (non versionné, `README.md:834–836`).
2. **Parallélisme récent jamais passé sous TSan.** La file bornée à mutex et variable de condition (5224ff4e) et les chronos par worker (5fdda963) sont postérieurs au dernier build TSan (2026-09-21 06:17 UTC). TSan n'est pas une option CMake. L'identité des sorties W1/W8 sur trames entières (sonde à 0e2c18ca) est un indice et non une preuve d'absence de course.
3. **Élargissement 18 bits commis sans fixtures extrêmes ni garde de domaine des fabriques.** À 12294241, 3 fichiers de test citent la limite 18 bits contre 42 qui gravent 65 535, alors que la décision 5 promettait de doubler chaque fixture extrême (`docs/ELARGISSEMENT_18_BITS_20260922.md:58`). `ExactBall::make_q2/q3/q4` (`src/lanes/exact_ball.cpp:72/83/104`) acceptent tout `Point3` `int32`. Pour des coordonnées hors [0, 262 143], `dot_small` déborde déjà en i64 (comportement indéfini), avant même les bornes i128. Le moteur passe par `prepare_cloud`, qui valide, et la v8 n'a pas d'API publique : seul un appelant direct (porte, sonde, futur code) est exposé. La leçon est déjà payée : des portes de ≤ 128 sites laissaient passer une troncature de rangs (`docs/FAUSSES_PISTES.md:396`). Correctif et 42 portes **[non commis]**, laissés hors du commit par le développeur de a74e90f2 (`docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:13`).
4. **Autorité des reçus hors dépôt.** Les lectures LIVE exigent des builds locaux non versionnés (150 dossiers, 13 Go). La porte spatiale n'est active que dans un build précis où `ctest` est interdit. La note [non commise] le dit : « ce répertoire n'est pas une archive d'exécution autonome » (`receipts/u18_resume_20260922/README.md`, non indexé). Une v9 dans un autre environnement ne peut pas relire les preuves LIVE de la v8.

### Gravité moyenne

5. **Mutations du cœur q3/q4 liées à l'arbre canonique** (rétrogradé de « haute »). Les 17 mutants u16 exigent un build sous `<racine des sources>/build/`, Release, sans sanitizer, avec Boost (`CMakeLists.txt:487–509`, `:587–591`). Ils sont DISABLED dans tout worktree ou clone ailleurs, sous sanitizers, et dans la capture ASan R2 [non commis]. Les trois mutants de la recherche de témoins sont inexécutables depuis 2629a536. Les portes rationnelles restent actives dans tout build avec Boost : un défaut du moteur y reste jugé. Ce qui manque hors de l'arbre canonique, c'est la preuve que ces portes tuent bien les fautes ciblées. La tranche [non commise] ajoute un sixième test du même type (`mhgp8_q4_saturating_mutations`). La v7 enregistrait 101 mutants en portes à code 4 (87 `--inject=`, 14 `--mutant=`), compilés dans ses cibles de test (`MHGP7_TESTING=1`).
6. **Portes non hermétiques.** 30 scripts appellent `git rev-parse` ; `docs_scope` charge `tools/check_docs.py` à la racine (`tests/docs_scope_gate.py:28`) ; 73 scripts sur 101 écrivent en dur `morsehgp3D_v8` : copier ces portes dans `morsehgp3D_v9/` les casserait. Hors git : 26/91 échecs (c5308651). Défaut signalé dès le 14 septembre (§ 2.8), jamais corrigé ; non mesuré à HEAD.
7. **Codes de sortie non distingués et incohérents.** La sonde moteur et la porte q34 rendent 1 pour toute erreur ; les sondes de l'ère q2 rendent 2 pour toute exception ; les portes C++ rendent 2 sur erreur d'usage ; aucun code 3 ni 4. CTest ne vérifie que « nul / non nul », et `fresh_run.cmake` ramène tout échec à un `FATAL_ERROR`. Le message causal exact exigé par les scripts de mutations compense en partie. La doctrine 0/1/2/3/4 de `morsehgp3D_v7/cmake/run_expect.cmake` n'est pas portée.
8. **Validateurs qui figent les schémas.** Un chemin de 15 modules (fermeture de 20, avec cycles d'imports et validateurs de prototypes) unit des inventaires `SOURCES` aux tailles figées (196/211/216). Le port G4 exige d'étendre trois validateurs au schéma v5 « sans casser la lecture des reçus v4 » (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:87–89`). Démonstration [non commise] : en Release R1, le lecteur de `q34_affine_mutations` refuse les nouveaux champs u18, et les captures R2 à 0 échec sont déclarées `failed` par leur propre lecteur.
9. **Reçus d'environ 1 Go versionnés** (979 Mo, 16 016 fichiers, 121 Mo de doublons, 250 copies `.cpp`, deux JSON de plus de 24 Mo) ; 75 schémas de manifeste contre `SHA256SUMS` en v7. L'historique est en revanche en ajout seul : aucune suppression ni aucun renommage dans `receipts/` ; 2 commits sur 169 (204b0620, 9923a6b9) modifient des fichiers existants, seulement deux README et une analyse.
10. **Défauts de bibliothèque lents et variantes multiples.** `WspdQ34Options` garde `witness_mode Disabled`, `q3_census_mode ScalarCover`, `witness_bounds_mode Legacy` et `q3_atlas_consultation false` (`src/pipeline/wspd_q34.hpp:19–45`). Le partage de tâches est en revanche actif par défaut (`parallel_task_pairs 256`). Cela contredit D3 (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:143`) et la décision v7 « une chaîne v8 principale, anciens chemins différentiels » (`docs/AUDIT_V7_SYNTHESE.md:127`). Cinq entrées WSPD q2 coexistent (`run_wspd_q2_census`, `_batched`, `_cooperative`, `_parallel`, `_ranges`).
11. **Prototypes dans la bibliothèque produit, et portes couplées à eux.** 1 907 lignes (`q34_cover`, `q34_pruning`, `q34_collective`, `family_certificate`, `q4_center_map`, `q4_shallow`) sont compilées dans `mhgp8_p0` (`CMakeLists.txt:32–39`) sans être appelées ailleurs dans `src/`. Plus grave pour un port : la porte principale du moteur `tests/wspd_q34_gate.cpp` et cinq autres incluent `tests/q34_family_pruning_gate.cpp`, une porte de prototype, via `#define main` ; `q4_seed_cells_gate.cpp` inclut `q34_cover_gate.cpp` ; la sonde moteur inclut `q4_lidar_probe.cpp`, qui inclut `q34_cover_probe.cpp`. En tout, 7 portes et 7 sondes réutilisent un autre fichier `.cpp` par `#define main`. Le produit ne perd presque rien : src +17 976/−851.
12. **Monolithe et duplication** : `src/pipeline/q2_census.cpp`, 2 852 lignes ; cinq copies de `owned()` ; 49 fichiers Python de `tests/` et `bench/` redéfinissent `require`.
13. **Plancher dépendant de l'ordonnancement.** Le mutant `parallel_refused_range_dropped` n'est tué que si la file pleine refuse une plage (`tests/wspd_q34_gate.cpp:454` : capacité 1, quatre workers) ; `mhgp8_wspd_q34_mutations` a échoué une fois sous charge, puis passé seul (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:128–130`). Le lien causal est une **hypothèse**. Précédent de mutant non déterministe : `worker_digest` (`PASSATION.md:394`).

### Gravité basse

14. Registre de 429 mots dans le type produit ; 1 155 lignes `counter_add` contrôlées, sans interrupteur de compilation et sans mesure de surcoût. Une partie est en fusion à froid ; la part chaude n'est pas établie. Le motif `static_assert(sizeof(...) == N * sizeof(u64))` (`src/parallel/work_reduction.hpp:67`) protège bien la fusion.
15. Le journal donne « 132 tests, 121 verts + 2 DISABLED » (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:14`) : 9 tests non expliqués, **non vérifiable**. Le même journal donne 129 exécutés et 3 DISABLED à a74e90f2 dans l'arbre canonique (l. 125–126).
16. `LastTest.log` enregistre les DISABLED comme « Test Passed. » à 0,00 s avec une commande vide : un lecteur pressé compte 132 verts.
17. Métadonnées périmées : `tests/wspd_q34_mutations.py` annonce « three causal global-q34 mutations » et `SCOPE = "three_causal_product_mutations…"` pour cinq mutations. Ce texte est recopié dans les reçus. Les scripts de mutations ne sont enregistrés qu'en Python normal, sans jumeau `-O`.
18. Contre-vérification exhaustive des paires résiduelles à 8 000 points (`audits/q34_stream_crosscheck_t32_8k_20260921/q3_stream_probe.cpp:30–31`, mode `samples_per_lane = 0`) : utile, mais en tension avec la règle « jamais de vérification exhaustive à l'échelle ».
19. Documentation compressée : espaces manquants (« 612appels », `audits/ETAT_COURANT.md:39` ; « pas92 tests TSan », `receipts/lidar_global_20260921/README.md:16`) ; `README.md` de 879 lignes en journal ; 810 lignes Python de plus de 120 caractères dans `bench/`.
20. Nom `mhgp8_p0` conservé pour toute la bibliothèque ; dossiers `.gitkeep` jamais remplis.

## 6. Questions ouvertes

- La tranche [non commise] (42 portes 18 bits, gardes de domaine, atlas saturant) sera-t-elle commise avant l'ouverture v9 ? Ses captures R2 sont vertes pour CTest mais `failed` pour leur lecteur, et R1 contenait deux vrais échecs de lecteurs.
- Existe-t-il ailleurs (G4, autre hôte) un passage TSan de `run_wspd_q34_parallel` postérieur à 5224ff4e ? Non trouvé localement.
- Existe-t-il un passage ASan/UBSan de a74e90f2 seul ? Localement, seul l'état a74e90f2 + tranche [non commise] a été passé (`sanitize_r2`, 0 échec).
- Les portes sont-elles hermétiques à HEAD ? La mesure 26/91 date de c5308651 ; les 132 tests de HEAD n'ont pas été rejoués hors git.
- La v9 garde-t-elle la voie float32 (23 CTests, 121,9 s, hors contrat selon D1) et la voie q2 (5 069 lignes dans `src/pipeline/*q2*`, cinq entrées) ? Le test ne tranche pas.
- Boost devient-il obligatoire en v9, comme en v7 ?
- Qui porte le format de reçu unique : constructeur ou auditeur ?

## 7. À porter en v9 et à ne pas reprendre

### À porter (port explicite, épinglé, requalifié)

| quoi | où (pin 12294241 sauf mention) | pourquoi |
| --- | --- | --- |
| Juges rationnels Boost indépendants | `oracle/p0_oracle.hpp`, `oracle/q2_census_oracle.hpp`, `tests/exact_ball_oracle.hpp` | arithmétique distincte du produit ; dépendance réduite à `core/types.hpp` (constantes de domaine à dupliquer dans le juge v9) |
| Planchers de non-vacuité émis en JSON par chaque porte | motif de `tests/wspd_q34_gate.cpp:942–977` | contre le vert par vacuité |
| Fixtures d'égalité stricte et mutants tués | `tests/p0_gate.cpp:701` (`strict_mutants == 3`) | ont tué un mutant survivant réel (14 sept.) |
| Injection déterministe d'échecs d'allocation et de callback | remplacement de `operator new` dans 10 portes (ex. `tests/wspd_q34_gate.cpp:16–38`) | teste les chemins d'échec transactionnels sans dépendre de l'ordonnancement |
| Discrimination causale par ligne d'échec exacte | `tests/wspd_q34_mutations.py:62–67, 164` | un mutant n'est tué que pour la bonne raison ; à combiner avec des codes exacts |
| Garde de fusion des compteurs par `static_assert` sur la taille | `src/parallel/work_reduction.hpp:67` | un champ oublié ne compile pas |
| `counter_add` à débordement contrôlé | `src/core/types.hpp:115–120` | pas de compteur silencieusement enroulé ; ajouter un interrupteur |
| Nettoyage protégé des sorties de test | `cmake/fresh_run.cmake` | refuse toute suppression hors `mhgp8_fresh` ; à étendre aux codes exacts |
| Double exécution Python normale et `-O`, sans `assert` | paires `_optimized` de `CMakeLists.txt` ; 0 `assert` nu dans `tests/` et `bench/` | la porte tient sous `python3 -O` ; à étendre aux scripts de mutations |
| Garde `#error` sous `__FAST_MATH__` et tests en modes FTZ | `src/core/float32_predicates.hpp:11–13` | filtres flottants valides seulement sans contraction |
| Lecteur d'identité W1/W8 sur trame entière | `bench/run_ground_baseline.py:180–191` | bit-identité parallèle à l'échelle réelle ; en v9, plancher fixe indépendant du statut `partial` |
| Labels `gate`/`slow`/`receipts`/`mutation` et cycle court `-L gate -LE slow` | 92d74c13 | 79 tests, 50 s de temps cumulé (19 s de mur selon le journal) |
| Option CMake ASan/UBSan | `MHGP8_SANITIZE` (`CMakeLists.txt:10`) | acquis v8 absent de la v7 ; à compléter par TSan |
| **De la v7** : portes à code exact et `run_expect.cmake` | `morsehgp3D_v7/cmake/run_expect.cmake`, `mhgp7_gate` | 0/1/2/3/4 distingués, crash par signal refusé, `EXPECT_LINE`/`EXPECT_PREFIX` sur la même exécution |
| **De la v7** : mutants `--inject=` compilés dans les cibles de test, portes à code 4 | `morsehgp3D_v7/CMakeLists.txt:22–27` (`MHGP7_TESTING=1`) et `:308–321` | exécutables dans tout arbre et sous tous drapeaux |
| **De la v7** : workflow CI avec Boost obligatoire et `--no-tests=error` | `.github/workflows/morsehgp3d-v7.yml` | aucune régression silencieuse |
| **De la v7** : manifestes `SHA256SUMS` vérifiés depuis l'index git | `tools/check_v7_receipt_publication.py` (122 lignes) | un format, un contrôleur ; la v7 garde toutefois 20 fichiers de sommes aux noms variants |
| Harnais de contre-vérification de l'auditeur, en juge d'échantillon | `audits/q34_stream_crosscheck_t32_8k_20260921/` | code indépendant du moteur ; à rejouer sur le moteur courant (sonde à d1b4dbc6) |

### À ne pas reprendre

| fausse piste ou dette | mesure ou preuve qui la ferme |
| --- | --- |
| Mutations compilées par réécriture de texte, liées à un arbre de build canonique | 5 tests DISABLED hors `<racine des sources>/build/` (dont le build HEAD de l'autre lentille) ; 1 test (3 mutants) inexécutable depuis 2629a536 (site non unique) ; inopérantes sous sanitizers [non commis : « leurs lanceurs compilés ne savent pas lier ce profil »] |
| Autorité de porte liée à un chemin de build épinglé | porte spatiale activée seulement dans un build où `ctest` est interdit |
| Chaîne de validateurs à inventaires cumulés et comptes figés | chemin de 15 modules, cycles d'imports, 196/211/216 sources figées ; captures [non commises] refusées par leur lecteur (nouveaux champs u18, liste de DISABLED) |
| Un schéma de manifeste par tranche | 75 schémas, 145 identifiants de schéma |
| Copies de sources et gros JSON dans les reçus | 979 Mo, 121 Mo de doublons, 250 copies `.cpp`, JSON de 24 Mo |
| Portes dépendantes de git et du nom du dossier | 16/34 (14 sept.) puis 26/91 (c5308651) échecs hors dépôt ; 73/101 scripts écrivent `morsehgp3D_v8` en dur |
| Portes à petits nuages seules pour les largeurs d'entiers | troncature de rangs invisible jusqu'à 128 sites, supports perdus dès n = 300 (`docs/FAUSSES_PISTES.md:396`) ; 3 fichiers sur 42 portés à 18 bits |
| Réutilisation de portes et de sondes par `#define main` et inclusion de `.cpp` | 7 portes (6 sur `q34_family_pruning_gate.cpp`, 1 sur `q34_cover_gate.cpp`), dont la porte moteur ; 7 sondes, dont la chaîne de la sonde moteur |
| Prototypes laissés dans la bibliothèque produit | 1 907 lignes non appelées ailleurs dans `src/`, mais requises par les portes et sondes du moteur |
| Défauts historiques lents, options mesurées en opt-in | `src/pipeline/wspd_q34.hpp:19–45` ; D3 non appliqué |
| Mutants de modèle dans la porte, sans mutation du produit | dix mutants rejoués dans `tests/wspd_front_inheritance_gate.cpp:74–91` : ils jugent la sensibilité du juge, pas la couverture du code |
| Codes de sortie hétérogènes par sonde | 1 pour toute erreur (sonde q34) contre 2 pour toute exception (sondes q2) |
| Contre-vérification exhaustive à 8 000 points en routine | règle « jamais de vérification exhaustive » de `CLAUDE.md` ; garder le mode échantillon |

## 8. Recommandations priorisées pour la v9

1. **CI dès le premier commit v9** : workflow GitHub qui installe `libboost-dev`, configure avec Boost **obligatoire** (`FATAL_ERROR`), lance `ctest -L gate --no-tests=error`, plus un job ASan/UBSan et un job TSan déclarés comme options CMake (`MHGP9_SANITIZE`, `MHGP9_TSAN`). Critère : la suite passe depuis une archive des sources, sans `.git`, sans dossier `build/` préexistant et sous un autre nom de dossier.
2. **Portes à code exact** : porter `run_expect.cmake` de la v7, avec les codes 0 conforme, 1 juge, 2 refus avant calcul, 3 plancher ou invariant, 4 mutant tué. Garder en plus la ligne causale exacte de la v8. Toutes les sondes et portes suivent la même table. Un crash par signal n'est jamais un succès.
3. **Mutants du produit par `--inject=`**, compilés dans les cibles de test : contact de coquille compté dedans, rejet d'atlas à K − 2, plage refusée non développée, domaine non vérifié. Ils sont enregistrés à code 4 dans tout arbre, sanitizers compris. Pour la file parallèle, forcer le refus par un crochet injecté plutôt que par l'ordonnancement.
4. **Largeur d'entier** : chaque prédicat et chaque fabrique publique contrôlent le domaine ; fixtures jumelles aux extrêmes 0 et 2^18 − 1 ; au moins un nuage gravé de plusieurs centaines de sites par voie ; mutant « limite de plage non vérifiée » ; porte de valeurs hostiles (INT32/INT64 min et max) comme dans la tranche [non commise].
5. **Format de reçu unique** : `SHA256SUMS` plus un manifeste minimal versionné une fois (commit, commande, empreintes d'entrée, compteurs logiques, sorties résumées) ; un seul contrôleur générique en CI ; pas de copies de sources (le commit suffit) ; gros bruts hors git ou référencés par empreinte ; plafond de taille par reçu.
6. **Portes hermétiques** : racine dérivée de `__file__` sans nom de dossier codé en dur ; aucune dépendance à `git rev-parse` dans une porte (seulement dans les lanceurs de campagne, avec refus explicite) ; chemin de `check_docs.py` passé par option CMake.
7. **Architecture de code** : un en-tête unique pour les prédicats d'arête et de propriété ; aides de test partagées dans des en-têtes, sans `#define main` ; bibliothèque produit limitée au chemin du moteur ; prototypes et anciennes variantes dans une cible `legacy` réservée aux différentiels ; défauts = configuration mesurée ; `q2_census.cpp` découpé ; sonde moteur sans dépendance à Boost ni aux sondes de prototypes.
8. **Compteurs** : registre par étage, court et stable ; instrumentation désactivable à la compilation ; surcoût mesuré en cycles sur le moteur réel à 8 000 / 16 000 / 32 000 points et sur trame entière avant de la rendre permanente.
9. **Budget de tests** : cycle court sous 60 s ; validateurs de reçus réduits au contrôleur générique ; suite lente la nuit ; aucune porte n'exige un inventaire figé de sources.
10. **Doctrine d'échelle** : aux tailles 8 000 / 16 000 / 32 000 et sur trames entières, invariants globaux (registres de masse, `published = consumed`), identité W1/W8 à K5 **et K10** avec un plancher fixe, et juge d'échantillon. La contre-vérification exhaustive reste un acte d'audit ponctuel, jamais une porte.
11. **Avant l'ouverture v9** : trancher le sort de la tranche [non commise] (gardes de domaine, 42 portes, atlas saturant) et la reporter explicitement dans la `PROVENANCE` v9, sans transfert de qualification.

## Contre-vérification

Contre-vérification adversariale du 22 septembre 2026, en lecture seule, sur 12294241 et le worktree partagé. Verdicts : **confirmé**, **corrigé** (valeur ou formulation remplacée ci-dessus), **réfuté**, **non vérifiable**.

### Affirmations principales

| # | affirmation d'origine | verdict | note |
| --- | --- | --- | --- |
| 1 | 132 CTests ; labels u16 103, gate 81, receipts 40, slow 31, float32 23, mutation 9, lidar 8 | confirmé | recompté dans `CTestTestfile.cmake` |
| 2 | 125 exécutés, 7 DISABLED, 466,8 s ; receipts 63 % | confirmé | 466,79 s ; receipts 295,92 s ; somme des temps par test |
| 3 | Boost optionnel, 25 portes non enregistrées ; v7 obligatoire | confirmé | ajout : la sonde moteur `mhgp8_wspd_q34_probe` dépend aussi de Boost |
| 4 | aucune CI v8 ; v7 avec `libboost-dev` et `--no-tests=error` | confirmé, nuancé | `ci.yml` vérifie la documentation v8 via `tools/check_docs.py` |
| 5 | 17 mutants u16 DISABLED hors arbre canonique ; 3 sans condition depuis 2629a536 | confirmé, nuancé | 5 + 3 + 3 + 3 + 3 = 17 ; DISABLED codé depuis 92d74c13 ; condition : `<racine des sources>/build/` |
| 6 | porte spatiale active seulement dans le build épinglé | confirmé | manifeste : `/workspaces/E-HGP/build/v8_q4_seed_cells_r2_20260921` |
| 7 | pas de codes exacts ; v7 : 319 portes, 103/6/86/10/92 | corrigé | v7 : 115 × 0, 6 × 1, 86 × 2, 10 × 3, 102 × 4 (la répartition d'origine faisait 297, pas 319). En v8, 35/46 portes C++ rendent 2 sur usage et les sondes q2 rendent 2 pour toute exception : la dichotomie « 0 ou 1 » ne vaut que pour la porte et la sonde q34. Les mutants exigent une ligne stderr exacte |
| 8 | file bornée jamais sous TSan ; TSan non optionnel | confirmé | dernier TSan 06:17 UTC ; 5224ff4e à 22:36:12 UTC |
| 9 | aucun ASan/UBSan entre le 21 sept. 10:39 et le 22 sept. 10:19 UTC | confirmé, nuancé | `sanitize_r2` [non commis] couvre a74e90f2 plus la tranche : 131 exécutés, 0 échec |
| 10 | fabriques `ExactBall` sans garde de domaine ; correctif non commis | confirmé, précisé | le débordement survient d'abord en i64 (`dot_small`) ; exposition limitée aux appelants directs |
| 11 | 3 fichiers citent 262 143 contre 30 qui gravent 65 535 | corrigé | 3 contre **42** (40 C++, 2 Python) |
| 12 | 26/91 échecs hors git ; 16/34 le 14 sept. | confirmé | c5308651 est un commit du 20 sept. 22:12 UTC ; HEAD non mesuré |
| 13 | 15 validateurs chaînés, 196/211/216, 148 assertions ; R2 `failed` | corrigé en partie | chemin de 15 confirmé, fermeture de 20 avec cycles ; 148 non reproduit (134 dans `bench/`, 274 avec `tests/`) ; R2 confirmé ; R1 avait 2 vrais échecs de lecteurs |
| 14 | 979 Mo, 16 016 fichiers, 121 Mo de doublons, 250 `.cpp`, 75 schémas, 0 SHA256SUMS ; v7 273 Mo | confirmé | v7 : 23 399 fichiers, 65 `SHA256SUMS` exacts plus 20 variantes |
| 15 | environ 1 907 lignes de prototypes ; 5 `owned()` | confirmé | 1 907 exactement (`.cpp` + `.hpp`) ; omis : la porte moteur et la sonde moteur incluent des fichiers de prototypes |
| 16 | défauts de `WspdQ34Options` = chemins lents historiques | confirmé, nuancé | `parallel_task_pairs 256` : partage actif par défaut |
| 17 | 429 mots ; 1 155 `counter_add` ; surcoût non mesuré | confirmé, précisé | 429 recalculé ; 1 155 lignes, 1 177 occurrences, plus 407 macros de fusion ; « boucles chaudes » non établi |
| 18 | identité W1/W8 K5 avec plancher ≥ 3 paires | corrigé | les deux reçus sont `partial`, le plancher appliqué est ≥ 1 (`run_ground_baseline.py:190`) ; 3 paires en tout sur deux reçus ; pas de W1 à K10, confirmé |
| 19 | auditeur B : 8 000 points, 93 914 / 10 756 et 409 195 / 116 985 ; mode exhaustif | confirmé, nuancé | sonde à d1b4dbc6 (avant atlas, file et 18 bits) ; coquilles non comparées |

### Chiffres

| grandeur d'origine | verdict | note |
| --- | --- | --- |
| 132 CTests | confirmé | — |
| 125 / 7 / 0 | confirmé | — |
| 466,8 s ; receipts 295,9 s ; `-L gate -LE slow` 79 tests en 50,1 s | confirmé, précisé | somme des temps par test ; 19 s de mur selon le journal |
| Boost : 25 (+2 spatiales, +5 mutations) | confirmé, précisé | plus 5 sondes, dont la sonde moteur |
| 26/91 ; 16/34 | confirmé | — |
| 10 float32 actifs ; 17 u16 dont 3 sans condition | confirmé | — |
| v7 : 319 (103/6/86/10/92) | corrigé | 115/6/86/10/102 |
| 17 116 lignes (5 911 + 11 205) | confirmé | — |
| 38 421 lignes de tests | confirmé | — |
| 31 502 lignes de bancs, 58 scripts | confirmé | — |
| accrétion src/tests/bench | confirmé | — |
| 169 commits, 45 audit | confirmé | 201 commits tous chemins |
| `q2_census.cpp` 2 852 lignes, 17 commits | confirmé | — |
| 429 mots | confirmé | — |
| 1 155 `counter_add` | corrigé (précision) | 1 155 lignes, 1 177 occurrences, plus 407 macros |
| 5 `owned()` | confirmé | — |
| ≈ 1 907 lignes de prototypes | confirmé | exact |
| 357 exceptions | confirmé | ventilation complétée |
| 979 311 698 octets, 16 016, 8 728 | confirmé | — |
| doublons 7 288 / 121 124 446 | confirmé | — |
| JSON 643,8 Mo (8 374) | confirmé | omis : `.stdout` 67,3 Mo |
| 296 manifestes, 75 schémas, 0 SHA256SUMS, 145 identifiants | confirmé | — |
| v7 272 869 901 octets, 65 SHA256SUMS | confirmé | plus 20 variantes |
| 15 modules ; 196/211/216 ; 148 assertions | corrigé en partie | 148 non reproduit |
| 30 scripts `git rev-parse` | confirmé | 29 + 1 |
| 166 dossiers, 13 Go ; 39 README | corrigé | 150 dossiers `v8*` (123 builds v8) et 31 journaux ; 13 Go et 39 README confirmés |
| 3 / 30 fichiers (18 bits / 65 535) | corrigé | 3 / 42 |
| TSan 06:17 ; ASan 10:39 | confirmé | — |
| 93 914 / 10 756 | confirmé | — |
| [non commis] R2 Release / ASan | confirmé | fichiers non indexés ; R1 : 2 échecs ; ASan R1 interrompu |
| sortie de `wspd_q34_gate` (18 410, 58, 143 810, 4 443) | confirmé | — |
| `.gitkeep` : 11 non comptés | corrigé | 9 sous `src/`, 12 dans le chantier |
| « 2 commits sur 45 modifient des reçus » | corrigé | 2 commits sur 169 (204b0620, 9923a6b9) ; 0 suppression, 0 renommage |
| « sept portes couplées à `q34_family_pruning_gate.cpp` » | corrigé | 6 sur ce fichier, 1 (`q4_seed_cells`) sur `q34_cover_gate.cpp` |
| citation « les reçus seuls ne sont pas une archive d'exécution autonome » | corrigé | texte exact : « ce répertoire n'est pas une archive d'exécution autonome » |
| gravité « haute » des mutants hors arbre canonique | corrigé | rétrogradé en moyenne : les juges restent actifs ; seule la preuve de leur sensibilité manque |
| q2 « 6 376 lignes » (question ouverte) | non vérifiable | périmètre non précisé : 5 069 (`pipeline/*q2*`) à 7 115 lignes avec crédits et front |

### Omissions ajoutées

- Injection déterministe d'échecs d'allocation par remplacement de `operator new` dans 10 portes (acquis à porter).
- Discrimination causale des mutants par ligne stderr exacte (atténue l'absence de codes exacts).
- Codes de sortie incohérents entre sondes q2 (2) et sonde q34 (1).
- La porte moteur `wspd_q34_gate.cpp` et la sonde moteur dépendent de fichiers de prototypes par `#define main` ; 7 sondes suivent ce motif.
- La sonde moteur n'existe qu'avec Boost.
- 73/101 scripts Python écrivent en dur `morsehgp3D_v8` : une copie vers la v9 casse.
- Graphe des validateurs à cycles et fermeture de 20 modules, prototypes compris.
- Reçus W1/W8 au statut `partial`, plancher ≥ 1 seulement.
- Contre-vérification 8k faite à d1b4dbc6, antérieure aux tranches atlas, file et 18 bits.
- [non commis] Release R1 : deux vrais échecs de lecteurs de mutations, dont un refus des champs u18 ; ASan R1 interrompu ; un sixième test de mutations lié à l'arbre canonique ajouté.
- Métadonnées « three » pour cinq mutations dans `wspd_q34_mutations.py` ; scripts de mutations sans jumeau `-O`.
- `fresh_run.cmake` annonce propager le code de sortie mais le ramène à un `FATAL_ERROR`.
- Option CMake ASan/UBSan : un acquis de la v8 sur la v7, qui n'en avait pas (drapeaux ad hoc).
- La v7 enregistrait 431 à 464 CTests dans ses builds locaux des 10–11 septembre (non rejoué).
