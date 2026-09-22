## Lentille 12/12 — Parallélisme multi-CPU, GPU et performance (audit v8 pour ouverture v9, version contre-vérifiée)

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only` (moteur mesuré ; `quantized_u18_input_only` pour la tranche non commise), `mode=audit_lecture_seule`, `public_status=not_claimed`. GCP non utilisé. Aucune compilation, aucun ctest, aucun benchmark, aucun script du dépôt exécuté. État publié de référence : `origin/main` à `12294241`. Les éléments tirés du worktree partagé non commis (fichiers indexés « A/AM » ou non suivis) sont marqués **[non commis]**.

Statuts : **prouvé** (preuve écrite et fixture), **testé** (porte bornée), **mesuré** (reçu épinglé : commit, sha256, sorties), **proposé**, **manquant**. Un chiffre sans reçu est **non vérifiable**. Tout ce qui figure sous « estimation » est un calcul d'audit à partir de reçus, jamais une mesure.

Cette version reprend le rapport `12_parallelisme_gpu_perf.md` et y intègre les corrections de la contre-vérification. La section finale les liste une par une.

## 1. Périmètre lu

Rapport d'origine : entrées v8 (README, PASSATION, JOURNAL, AUDIT_REPRISE, VERROUS, CONTRAT_TRAMES, IMPLEMENTATION_PARALLELISATION, PLAN_DE_REFONTE § 2/7/9/10, ELARGISSEMENT_18_BITS l. 1–100, SYNTHESE_PRIORITES_LIDAR, CERTIFICATS_COLLECTIFS l. 1–80, ETAT_COURANT l. 1–60), sources parallèles, reçus `ground_*`, `q34_spatial`, `lidar_global`, `q2_*`, héritage v6/v7, `gcp-migration/cpu_probe_session_v8.py`.

Relu ou ajouté par la contre-vérification (worktree détaché `12294241` sauf mention) :

- Code : `morsehgp3D_v8/src/pipeline/wspd_q34.cpp` l. 370–470 et 686–877 (file, verrous, chronos), l. 29–229 (`static_assert` de réduction) ; `src/pipeline/wspd_q34.hpp:41,44,230` (grain 256, capacité 4 096, 16 jobs par worker) ; `src/parallel/joined_workers.hpp` entier ; `src/parallel/work_reduction.hpp:55-70` ; `src/lanes/q3_ball_census.cpp:1-40` ; `src/lanes/q4_local_partition.cpp:1-30,125-180,255-270` et `.hpp:21-22` ; `src/core/float32_predicates.hpp:1-40` ; `CMakeLists.txt` (options, sanitizers, absence de `-march`).
- Portes : `tests/wspd_q34_gate.cpp:440-535,942` ; `tests/wspd_q34_mutations.py` (mutant `parallel_refused_range_dropped`).
- Reçus : tous les champs `output`, `work`, `tasks`, `workers_work`, `workers_timing_ms`, `timings_ms`, `local28.atlas.partition`, `q3`, `q3_blocks`, `q3_atlas` des 18 JSON de `receipts/ground_baseline_20260921/` et `receipts/ground_phase1_20260921/` ; les 18 `time_*.txt` ; `gprof_scene00_k5_w1/` (`run.sh`, `time_K5_W1.txt`, profil plat) et les sha256 de ces fichiers et des trois `BASELINE*.json` ; `receipts/q34_spatial_20260921/README.md:33-156` ; `receipts/lidar_global_20260921/README.md:1-30,110-160` ; sections résultats des sept reçus `q2_*` cités ; `q2_front_workers_20260914/thread_sanitizer/README.md` ; `audits/collective_edge_20260922/microbench.py` (un « pool borné » de 32 sites, sans rapport avec un pool de threads).
- Docs : `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:50-72,175-212` ; `docs/JOURNAL_DEVELOPPEMENT_20260921.md:40-145` ; `docs/ELARGISSEMENT_18_BITS_20260922.md:40-50,92-100` ; `audits/SYNTHESE_PRIORITES_LIDAR_20260922.md` entier ; `docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md:126-136` ; `docs/Q4_BLOCS_SEEDS_PISTE_20260921.md:192-201` ; `docs/PLAN_DE_REFONTE.md:610-636` ; `docs/VERROUS_ARCHITECTURE.md:345-358` ; `AGENTS.md:5-40,354` ; `docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:1-30` ; fin de `audits/COORDINATION_MORSEHGP3D_V8.md` (l. 3740–3850) et recherche par mots-clés.
- Héritage : `morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md:1-30`, `RESULTATS_TOUR_CACHE_G4_20260910.md:1-40`, `PARALLELISATION_PAR_LOTS_20260911.md:25,53,55,61` ; `morsehgp3D_v6/docs/GPU.md:205-300` ; `morsehgp3D_v6/docs/ECHELLE.md:136-160` ; `morsehgp3D_v6/src/gpu/lot_ring.hpp:1-40`.
- Historique : messages complets de `0948d2d0`, `5224ff4e`, `5fdda963`, `748ec082`, `02987f18`, `0e2c18ca`, `72f125c6`, `5368d1ce`, `a5447d06`, `12294241` ; dates UTC des commits cités ; `git log` des commits v8 postérieurs au pilote G4.
- **[non commis]** : `receipts/ground_18bits_20260922/u16_identity/` (six JSON et six `time_*.txt`), `receipts/u18_resume_20260922/ground_1mm_first/` (JSON et `time`), `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` entier, `CMakeCache.txt` des builds `build/v8_u18_resume_sanitize*_20260922`, `build/v8-dev-tests`, `build/v8-18fix` (drapeaux seulement), fichiers v6 non commis `src/gpu/route_c6.hpp`, `tests/route_c6_gate.cpp`, `tests/perm_sort_gate.cpp` et diff de `src/gpu/lot_ring.hpp` (en-têtes seulement).

**Non lu** : l'essentiel du canal `audits/COORDINATION_MORSEHGP3D_V8.md` (3 850 lignes), `audits/DIALOGUE_COURANT.md`, `audits/DIALOGUE_AUDITEUR_B.md`, la majorité des `docs/P0_*.md`, les reçus float32 (seul le constat de coût d'AUDIT_REPRISE est repris), le contenu des captures TSan (seuls manifestes et README), le graphe d'appels `gprof_call_K5_W1.txt.gz`, les sources CUDA v6/v7, `morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md` (modifié dans le worktree), `RESIDENCE_MASSIVE.md`, les diffs des sources u18 non commises, l'archive `MorseHGP_LiDAR_cascade_2026-09-22.zip` (hors dépôt), les scripts `gcp-migration/` au-delà des lignes `GPU_executed`.

## 2. Ce qui a été fait (parallélisme, GPU, performance)

### Voie q2 : équipes, files et continuations (13–17 septembre)

| Tranche | Commit | Objet | Résultat de temps |
| --- | --- | --- | --- |
| 13 | `b268cf6f` | Sous-arbres du front répartis entre workers ; index partagé en lecture, moteurs et tampons privés | ×3,91 uniforme, ×3,40 terrain, ×3,89 amas, ×1,93 rangées de 1 à 4 workers sur 4 cœurs physiques, 8k/K10, médianes de trois essais (`receipts/q2_front_workers_20260914/README.md:116-123`) |
| 14 | `4e878754` | `Donate` : file bornée des produits pendants | Pas de gain général ; médianes plus basses sur uniforme 8k (1,532 contre 1,645 s) et LiDAR 50k, mais étendues communes ; régression amas 8k 0,857 → 1,176 s (`receipts/q2_dynamic_front_20260914/README.md:56-70`) |
| 15 | `d09e2207` | Census q2 reprenable (continuation possédée) | Aucun gain revendiqué |
| 16 | `897085f8` | Détachement des frères B et répartiteur par ancre | 8 comparaisons favorables sur 72, ratio médian 0,0738 ; coût d'équipe par ancre dominant (`receipts/q2_census_split_20260915/README.md:82-87`) |
| 17 | `beee3341` | Équipe persistante front + census | 17 des 18 comparaisons rangées plus lentes que Coarse ; 6 272 octets par continuation (`receipts/q2_cooperative_20260915/README.md:69-80`) |
| 18 | `2741d614` | Plages d'ancres et plans Pool immuables partagés | Ratio Coarse/plages médian 1,039, 31 sur 54 plus rapides, pas de gain général établi (`receipts/q2_anchor_ranges_20260915/README.md:94-96`) |
| 19 | `8d615cfd` | Lots de singletons entrelacés | Négatif qualifié : **plus lents** que Coarse de ×1,005 à ×1,225, médiane 1,112, sur 54 comparaisons (`receipts/q2_singleton_batch_20260915/README.md:103-105`) |

Chaque entrée possède une capture Clang TSan dans son reçu (`thread_sanitizer/`, `tsan_*`). Coarse reste le défaut. Les tranches 20 (`8190e7ab`) et 21 (`3e94c868`) réduisent le travail q2 (×0,42 à ×0,71, puis ×0,86 à ×0,96) sans toucher l'ordonnancement. Les rangées régressent en tranche 20 (×1,01 à ×1,18, `morsehgp3D_v8/README.md:411-412`).

### Voies q3/q4 : équipe Coarse puis file de plages (21 septembre, dates UTC)

- `4dbe3024` (08:18, tranche 31) : `run_wspd_q34_parallel`, équipe Coarse de sous-arbres du front, arête atomique ; pilote G4 CPU sur préfixes 1k–8k (`receipts/lidar_global_20260921/README.md:116-152`).
- `70de84f2` (12:16) : référence G4 sur trames brutes entières 0/100/200, K5/s8, W48 CPU. Session TERMINATED à 12:09 UTC (`receipts/q34_spatial_20260921/README.md:87-156`).
- `748ec082` (21:51) : bornes de blocs et de points de l'atlas q4 en i64 (échelle 2^20, un carré T² reste en i128) ; compteurs logiques identiques ; 48,5 → 39,9 s sur le quart x+y+ (message de commit, non épinglé).
- `02987f18` (21:56) : une allocation par fragment d'atlas ; aucun gain mesurable (40,3 contre 39,9 s, message de commit).
- `0948d2d0` (22:16) : graines q3 rejetées par l'atlas q4 avant census ; l'atlas est désormais construit **avant** la voie q3 (90,7 % de rejet sur le quart, message de commit).
- `5224ff4e` (22:36) : tout rectangle résiduel survivant est publié comme tâche (plages de rangs de A si masse > 256 paires) dans une file bornée (4 096) sous mutex unique ; refus → développement local par l'éditeur ; pop LIFO.
- `5fdda963` (22:39) : chronos par worker (mur, CPU du fil par `CLOCK_THREAD_CPUTIME_ID`, attente sur la variable de condition).
- `0e2c18ca` (22:49) : deux variantes d'atlas rejetées (journal seulement).
- `72f125c6` (22 sept., 05:33) : reçus `ground_baseline_20260921` (sonde `92d74c13`) et `ground_phase1_20260921` (sonde `0e2c18ca`) ; le lanceur refuse désormais d'écraser un reçu.
- `a74e90f2` (06:21) : élargissement 18 bits du moteur entier (commis).
- **[non commis]** : option `Q4LocalOptions::saturate_deep` (désactivée par défaut), identité u16 après élargissement, première mesure 1 mm sans sol.

### GPU

La v8 n'a rien exécuté sur GPU. Le plan GPU reste écrit : phase 4 de `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:200-209` et § 7 de `docs/PLAN_DE_REFONTE.md:615-634`. Les phases 2 à 4 de ce plan n'ont livré que la file de plages et les chronos (voir § 5).

## 3. État par composant

| Composant | Statut | Preuve |
| --- | --- | --- |
| Équipe jointe `run_joined_workers` (jointure avant toute propagation, lanceur injectable pour l'échec partiel) | testé | `src/parallel/joined_workers.hpp:24-65` ; `std::thread` créé à chaque appel (l. 16), aucun pool persistant de processus |
| Réduction explicite des compteurs avec `static_assert` de taille | testé (compilation) | `src/parallel/work_reduction.hpp:67` (front) et `src/pipeline/wspd_q34.cpp:29-229` (vingt registres q3/q4) |
| Front + census q2 multi-CPU Coarse | testé (Clang TSan, oracle scalaire, 4 336 appels vérifiés par l'auditeur B) et mesuré | `receipts/q2_front_workers_20260914/README.md:10-40,116-123` |
| Variantes q2 Donate, détachement, coopératif, plages, lots | testées (TSan) ; mesurées sans gain de temps général | § 2 ; options explicites hors défaut |
| q2 sur nuage sans sol | **manquant** | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:64` ; aucun reçu `q2_*` après le 17 septembre ; sonde q34 au masque 6 |
| Équipe q34 Coarse (sous-arbres) | testé (porte W1/W2/W4 contre oracle rationnel ; une sonde n=64/K5/W4 sous Clang TSan) et mesuré (G4, avant la file) | `tests/wspd_q34_gate.cpp:449-478` ; `receipts/lidar_global_20260921/README.md:15-16` |
| File bornée de plages q34 (`5224ff4e`) | testé (identités `published == consumed`, `completed_jobs == jobs`, grain 2 et file 1 à W4, planchers `task_splits>0` et `task_refusals>0`, 5e mutant tué) ; mesuré en local W8 ; **non mesuré sur G4** ; **jamais sous TSan**, ni commis ni non commis | `src/pipeline/wspd_q34.cpp:742-872` ; `tests/wspd_q34_gate.cpp:454,519-529,942` ; seul build TSan q34 : `build/v8_lidar_global_tsan_20260921` (21 sept. 06:17, avant `4dbe3024`) ; builds u18 « sanitize » = ASan/UBSan |
| Chronos par worker | mesuré, mais **incomplet** : l'attente ne compte que la variable de condition, pas le blocage sur le mutex | `src/pipeline/wspd_q34.cpp:782-788,841` ; `workers_timing_ms` des JSON |
| Découpage intra-arête (blocs de graines, cellules d'atlas, parent immuable partagé) | proposé | `docs/Q4_BLOCS_SEEDS_PISTE_20260921.md:197-199` ; `docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md:131-133` |
| Déterminisme des sorties | testé comme **multiensemble** (petites portes normalisées) ; digests commutatifs xor/somme à l'échelle ; aucun ordre canonique | `docs/P0_FRONT_WORKERS_Q2.md:46` ; `bench/wspd_q34_probe.cpp:141` ; `tests/wspd_q34_gate.cpp:475-478` |
| Vol de travail, files par worker | manquant | une file unique sous mutex, `src/pipeline/wspd_q34.cpp:742-749` |
| SIMD | manquant | aucun `-march` ni intrinsèque dans `CMakeLists.txt` et `src/` ; l'hôte local (AMD EPYC 7763) n'a pas d'AVX-512 ; le jeu d'instructions de la G4 n'est archivé dans aucun reçu |
| Filtre flottant certifié à repli exact **sur le moteur u16** | proposé | `docs/JOURNAL_DEVELOPPEMENT_20260921.md:66-68` ; `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:179-181` |
| Filtres d'intervalle binary32 à repli exact (voie float32 séparée) | testé et mesuré ; bibliothèque `mhgp8_f32` séparée du moteur, enregistrée dans CMake/CTest depuis `92d74c13` ; **×20 à ×70 plus lent par opération** que l'u16 | `src/core/float32_predicates.hpp:11-13,37` (`Float32PredicateMode::Filtered`) ; `CMakeLists.txt:59-70` ; `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65,92` |
| Arithmétique des deux postes chauds | census q3 : bornes de puissance i128 (< 2^117) ; atlas : bornes de blocs i64 (< 2^62 à 18 bits) avec un carré i128 par axe | `src/lanes/q3_ball_census.cpp:12-40` ; `src/lanes/q4_local_partition.cpp:28-31,170-175,255-270` |
| GPU (code, build, exécution) | **manquant** | 0 fichier `.cu`/`.cuh` sous `morsehgp3D_v8/` ; `CMakeLists.txt:2` `LANGUAGES CXX` ; `src/gpu/` ne contient que `.gitkeep` ; `GPU_executed: false` dans **10** JSON G4 (3 sous `receipts/q34_spatial_20260921/`, 7 sous `receipts/lidar_global_20260921/gcp_r{1,2,3}_*`), plus le script `archive_gcp.py` qui recopie ce champ ; le protocole l'impose (`gcp-migration/cpu_probe_session_v8.py:89`, `gcp-migration/q34_spatial_session_v8.py:98`) |
| Plan GPU v8 | proposé | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:200-209` |
| Mémoire | mesuré pour le flux en mode digest seulement : RSS 12 504 à 14 516 Kio | `time_*.txt` des deux reçus ; catalogue et tour absents |
| Profil par symbole | mesuré avant la phase 1 seulement (build `-pg`) ; profil post-phase 1 non vérifiable | `receipts/ground_baseline_20260921/gprof_scene00_k5_w1/` ; `docs/JOURNAL_DEVELOPPEMENT_20260921.md:60-65` sans reçu |
| TSan dans CMake | manquant | `CMakeLists.txt:10,47-49,68-70` : `address,undefined` seulement ; chaque TSan v8 est un arbre ad hoc |
| Protocole G4 CPU v8 | testé et exercé ; sessions certifiées TERMINATED | `receipts/q34_spatial_20260921/README.md:148-156` ; `receipts/lidar_global_20260921/README.md:115-122` |
| Portage G4 du moteur à file | proposé (cinq étapes non exécutées) | `docs/JOURNAL_DEVELOPPEMENT_20260921.md:71-99` |

## 4. Chiffres clés

Toutes les mesures locales viennent d'un hôte partagé de 8 vCPU (AMD EPYC 7763, 4 cœurs physiques SMT), une répétition. « CPU·s » = temps utilisateur + système de GNU time pour le processus entier (ex. 740,94 + 0,56 = 741,5).

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| Scène 0 sans sol, K5, W1 : mur / CPU | 889,5 s / 888,7 s → 453,3 s / 453,3 s (×1,96) | `receipts/ground_baseline_20260921` (sonde `92d74c13`, `BASELINE.json` sha256 `4c8da44d…`) → `receipts/ground_phase1_20260921/README.md:31` (sonde `0e2c18ca`, `BASELINE.only.json` sha256 `d2a3d52f…`) | flux q3/q4 seul ; référence sous charge 4,2 avant ligne, reprise sous charge < 3 ; binaire de sonde épinglé par sha256 (`bb7f01fc…`) mais non archivé |
| Scène 0, K5, W8 : mur / CPU / occupation | 298,0 s / 1 283,4 / 430 % → 108,0 s / 741,5 / 686 % | idem, l. 32 | ×4,20 de W1 à W8 ; CPU W8 = 1,636 × CPU W1 |
| Scène 0, K10, W8 : mur / CPU / occupation | 911,1 s / 3 876,5 / 425 % → 323,0 s / 2 212,1 / 684 % | idem, l. 33 | pas de ligne W1 à K10 |
| Scène 1 (35 491 sites) K5 W1 / K10 W8 CPU | 370,6 / 1 665,2 CPU·s | `receipts/ground_phase1_20260921/README.md:34-36` | chevauchement de charge déclaré |
| Scène 2 (45 114 sites) K5 W1 / K10 W8 CPU | 852,0 / 3 811,0 CPU·s ; mur K10 W8 824,1 s | idem, l. 37-39 | chevauchement de charge ; référence contaminée |
| CPU du fil par worker après la file | 90,7–94,4 s (S0 K5), 274,0–279,9 s (S0 K10), 464,6–492,0 s (S2 K10) : écart 2 à 6 % | `workers_timing_ms` des JSON phase 1 | attente sur variable de condition 0 à 3,8 ms (S0 K5), jusqu'à 25,5 ms (S0 K10) et 45,1 ms (S2 K5) |
| CPU du fil / mur, par worker | 84–87 % (S0 K5 et K10, reprise « calme » avec un harnais sur un cœur) ; 97 % (**[non commis]** 1 mm, hôte sans autre charge lourde) | `workers_timing_ms` ; `receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json` | l'écart de 13–16 % n'est pas instrumenté (blocage mutex ou préemption) ; compatible avec le harnais (12,5 % de l'hôte) |
| Paires développées par worker avant/après la file (S0 K5 W8) | 1,33 M–9,40 M (×7,05) → 2,18 M–4,34 M (×1,99) | `workers_work` de `ground_baseline_20260921/probe_01…json` et `ground_phase1_20260921/only_probe_01…json` | — |
| Tâches publiées / paires en tâches / refus / pic de file (S0 K5 W8) | 1 037 747 / 18 799 445 sur 23 957 225 / 171 082 / 4 096 | `tasks` de `only_probe_01_s00_k5_w8.json` | seulement 10 802 rectangles scindés : le reste des tâches sont des rectangles entiers ; file pleine dans les 6 lignes W8 ; refus 108 078 à 330 061 |
| Occupation G4 W48 avant la file, trames brutes K5 | 4,19 / 11,13 / 1,93 CPU logiques ; murs 165,214 / 34,319 / 505,479 s ; CPU 691,65 / 382,00 / 973,37 s | `receipts/q34_spatial_20260921/README.md:97-102` | trames brutes 119–121k sites, moteur `70de84f2` antérieur à la phase 1 ; 768/769/768 jobs Coarse |
| Même trame 0, local W4 contre G4 W48, travail identique | CPU 1 417,5 s contre 691,65 s (rapport 2,05) ; mur 383,3 s contre 165,2 s (rapport 2,32) | `receipts/q34_spatial_20260921/README.md:55-59,100,117,130-132` | seule indication de la vitesse par fil G4/local ; confondue par la charge et le SMT locaux |
| Occupation G4 W48, préfixes 4k / 8k | 3,72 / 3,23 CPU ; 8k en 614,744 s ; 1k W1→W48 ×4,459 | `receipts/lidar_global_20260921/README.md:128-152` | petites tailles : diagnostic, pas une pente |
| Occupation G4 W48 après la file | **manquant** | — | aucune session G4 après `5224ff4e` |
| Machine G4 | 48 CPU logiques, 24 cœurs / 2 SMT, AMD EPYC 9B45 ; GPU RTX PRO 6000 Blackwell Server Edition SM 12.0 | `receipts/q34_spatial_20260921/README.md:91-92` ; `morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md:12` | drapeaux CPU (AVX-512) non archivés |
| Atlas q4, S0 K10 | 10 718 436 649 bornes de blocs, 24 196 349 343 tests ponctuels, 18 307 295 244 IDs de frontière copiés (et 37,9 G visites de nœuds) | `local28.atlas.partition` de `ground_phase1_20260921/only_probe_02…json`, identiques dans la référence | compteurs logiques, pas des temps |
| Atlas q4, S2 K10 | 22,25 G bornes de blocs, 44,54 G tests ponctuels, 32,45 G IDs copiés | `ground_phase1_20260921/probe_08…json` | idem |
| Graines q3 / boules construites / bornes de census q3, S0 K10 | graines 463,1 M (inchangées) ; boules 463,1 M → 35,4 M ; bornes préparées 17,26 G → 1,68 G | JSON `probe_02` référence et phase 1 (`q3.ball_builds`, `q3_blocks.count_bounds_prepared`) | effet de `0948d2d0` ; 427,7 M rejets par l'atlas |
| Arêtes développées (paires) | S0 : 23,96 M (K5), 30,69 M (K10) ; S1 : 11,83 M / 17,34 M ; S2 : 21,06 M / 30,38 M | `work.expanded_pairs` des JSON phase 1 | — |
| Plus gros cover d'une arête | 20 718 sites (S0 K5, 52 % de n) ; 30 923 (S0 K10, 78 %) | `work.max_cover_sites` | coût par arête non chronométré |
| Sorties du flux S0 K10 | 4 560 557 candidats, 15 411 422 IDs de support, 15 459 391 IDs de coquille | `output` de `only_probe_02…json` | doublons possibles ; pas un catalogue |
| Profil gprof S0 K5 W1 (avant phase 1) | `node_bounds_unchecked` 40,32 % (3 158 810 450 appels = `block_bound_tests`), `census_q3_ball` 24,08 % + 4,42 %, constructeur de fragments 8,95 %, `form` 4,91 %, `retain` 2,97 % | `receipts/ground_baseline_20260921/gprof_scene00_k5_w1/gprof_flat_K5_W1.txt` (sha256 `61faf50d…`) | build `-pg` RelWithDebInfo, 1 663,75 s utilisateur contre 888 s en Release ; l'instrumentation par appel surpondère les fonctions à milliards d'appels |
| RSS maximal du flux | 12 504 à 14 516 Kio | `time_*.txt` des deux reçus | mode `digest`, rien de matérialisé |
| Index u16 (construction) | 14,5 à 15,4 ms à 39 815 sites (S0) ; 12,9 à 15,7 ms sur les autres lignes ; 40,7 ms à S2 K10 (contaminée) | `timings_ms.index` des JSON phase 1 | mono-thread |
| q2 composant, 4 workers, K10/s8, 50k | uniforme 13,998 s ; amas 10,270 s ; LiDAR 50k Coarse médianes 5,460 / 3,057 / 3,904 s | `receipts/q2_front_workers_20260914/README.md:134-139` ; `receipts/q2_dynamic_front_20260914/README.md:64-66` | avec sol, préfixes ; un essai (synthétique 50k) ; jamais sans sol |
| Identité u16 après élargissement 18 bits **[non commis]** | sorties et compteurs logiques identiques (seuls des octets de pile changent) ; CPU W8 +5,2 à +8,3 % ; occupation 672–765 % contre 462–686 % ; mur S0 K5 105,0 s contre 108,0 s | `receipts/ground_18bits_20260922/u16_identity/` (worktree) | une répétition, aucune ligne W1 : le surcoût CPU n'est **pas attribuable** au 18 bits (voir § 5) |
| Première trame sans sol 1 mm **[non commis]** | 39 885 sites, K5 W8 : 104,63 s mur, 812,82 CPU·s, 776 %, RSS 15 124 Kio ; atlas 3,252 G bornes, 7,316 G tests ponctuels, 5,547 G IDs | `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:100-112` ; `receipts/u18_resume_20260922/ground_1mm_first/` (worktree) | pas de paire W1/W8 |
| v7 50k K10, G4 | tour 418,873 s (CPU) / 418,921 s (hybride) ; FULL 389,668 / 390,481 s ; kernels census 189,346 ms pour une phase de 4,540 s | `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md:21-36` | uniforme, historique |
| v6 50k, étage device | 7 717 ms dont 154 ms de noyaux (88 % hôte mono-fil) ; gain net sur le mur plafonné à −10,4 % | `morsehgp3D_v6/docs/GPU.md:219-222` | historique |
| v6, 1 → 48 fils sur G4 à 16k | ×13,02 (uniforme), ×15,68 (huit amas) ; fraction séquentielle 5,7 % / 4,4 % | `morsehgp3D_v6/docs/ECHELLE.md:144-146` | autre moteur |

## 4 bis. Ce qu'il faudrait gagner pour 1 s puis 100 ms (estimation)

### Périmètre de l'estimation

Ces facteurs portent sur le **régime sans sol u16 à 2 cm**. L'utilisateur l'a déclaré prioritaire (`AGENTS.md:29-39`), mais il ne remplace pas le contrat principal. Celui-ci porte sur la tour entière d'une trame brute entière, en float32 par défaut ou sur une grille de 1 mm (`AGENTS.md:5-27` ; `docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:9-13`). Les mesures u16 à 2 cm n'y transfèrent aucune qualification. Le périmètre mesuré est le **flux de candidats q3/q4** : ni q2, ni déduplication, ni catalogue, ni fold, ni tour. Les facteurs sont donc des **minorants** du gain nécessaire au contrat. En v7, l'aval FULL représentait 390 s sur 419 s de tour à 50k K10.

### Modèle et hypothèses

Le facteur requis vaut $R = C_{1} / (P_{\mathrm{eff}} \cdot s \cdot T)$. $C_{1}$ est le CPU mono-fil local, $P_{\mathrm{eff}}$ le nombre d'équivalents mono-fil sur G4, $s$ le rapport de vitesse par fil G4/local, et $T$ la cible murale.

- Variante A (rapport d'origine) : $P_{\mathrm{eff}} \in \lbrace 24, 48 \rbrace$ et $s = 1$. À K10, $C_{1}$ est estimé par CPU W8 divisé par 1,636 / 1,624 / 1,573.
- Variante B (contre-vérification) : localement, 8 fils sur 4 cœurs donnent seulement 1,13 à 1,22 équivalent mono-fil par cœur, puisque 8/1,636 = 4,89, et 8/1,77 = 4,51 si l'on rapporte le CPU W8 de l'identité u18 **[non commis]** (autre binaire) au W1 de la phase 1. Si la G4 se comporte de même, 48 fils sur 24 cœurs valent 27 à 29 équivalents mono-fil, et non 48. Cela revient à écrire $R = C_{W8} / (48 \cdot s \cdot T)$ avec le CPU W8 local. Le reçu `q34_spatial` apparie la trame 0 brute à travail identique : 1 417,5 CPU·s en local W4 contre 691,65 CPU·s sur G4 W48 (4,19 CPU occupés, donc peu de concurrence SMT). Le rapport vaut 2,05, d'où $s$ compris entre environ 1,25 et 2,05 selon la part SMT et charge du local. C'est une **indication indirecte, non une mesure**.

### Facteur requis sur le flux seul, cible 1 s (100 ms : multiplier par 10)

| Scène | K | $C_{1}$ (CPU·s) | CPU W8 (CPU·s) | A : P=48, s=1 | A : P=24, s=1 | B : s=1 | B : s=1,25 | B : s=2,05 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| S0 (39 815) | 5 | 453,3 (mesuré) | 741,5 | ×9,4 | ×18,9 | ×15,4 | ×12,4 | ×7,5 |
| S1 (35 491) | 5 | 370,6 (charge croisée) | 601,8 | ×7,7 | ×15,4 | ×12,5 | ×10,0 | ×6,1 |
| S2 (45 114) | 5 | 852,0 (charge croisée) | 1 340,1 | ×17,8 | ×35,5 | ×27,9 | ×22,3 | ×13,6 |
| S0 | 10 | ≈ 1 352 (estimé) | 2 212,1 | ×28 | ×56 | ×46 | ×37 | ×22 |
| S1 | 10 | ≈ 1 025 (estimé) | 1 665,2 | ×21 | ×43 | ×35 | ×28 | ×17 |
| S2 | 10 | ≈ 2 423 (estimé) | 3 811,0 | ×50 | ×101 | ×79 | ×64 | ×39 |

Le « ×46 » de `receipts/ground_phase1_20260921/README.md:47-48` divise le CPU W8 par 48. Ce n'est pas une erreur de compensation : c'est exactement la variante B avec $s = 1$, qui transporte le SMT local sur la G4. La variante A avec P=48 suppose au contraire qu'un fil SMT vaut un cœur, ce que les mesures locales contredisent. L'incertitude dominante n'est pas le SMT mais $s$, jamais mesuré directement.

Conclusion de calcul : pour 1 s sur le flux seul, il faut environ ×6 à ×36 de CPU en moins à K5 et ×17 à ×101 à K10, selon la scène, $P_{\mathrm{eff}}$ et $s$. 100 ms ajoute ×10. Aucune de ces fourchettes ne s'obtient par le seul parallélisme CPU.

Sur trames brutes (contrat principal), l'estimation est grossière. Le moteur G4 d'avant la phase 1 dépensait 382 à 973 CPU·s à K5, faiblement concurrents, soit ×13 à ×41 pour 1 s avec $P_{\mathrm{eff}}$ entre 24 et 29. La phase 1 n'a jamais été mesurée sur trame brute, et K10 n'a jamais été mesuré sur trame brute.

### Budget par arête et par opération

- Coût moyen actuel du flux ramené par arête développée, en W1 : 18,9 µs (S0 K5), 31,3 µs (S1 K5), 40,4 µs (S2 K5). En équivalent W1 estimé : ≈ 44 µs (S0 K10) et ≈ 80 µs (S2 K10). Ce CPU inclut front, filtres de rectangles et atlas : c'est une moyenne, pas le coût d'une arête isolée.
- Budget pour 1 s : 1,0 à 2,0 µs par arête à S0 K5, 0,78 à 1,56 µs à S0 K10 (variante A). Pour 100 ms : 78 à 200 ns par arête, selon $P_{\mathrm{eff}}$ et K. À S0, un coût de 100 ns par arête ne dépasse ce budget qu'avec P=24 (budget de 78 ns à K10 et 100 ns à K5) ; avec P=48, le budget est de 156 à 200 ns. Dans tous les cas, 24 à 31 M arêtes résiduelles à plusieurs dizaines de µs chacune imposent de réduire leur nombre ou leur coût de deux ordres de grandeur.
- Atlas à S0 K10 : 53,2 G opérations élémentaires hétérogènes (10,7 G bornes de blocs + 24,2 G tests ponctuels + 18,3 G copies d'IDs). Même si l'atlas était seul, 24 à 48 CPU·s par seconde imposeraient 0,45 à 0,9 ns par opération. Une borne de bloc coûtait ≈ 175 ns sous gprof avant `748ec082`, soit ≈ 93 ns par proportion en Release. Ce chiffre est **dérivé, non mesuré**, et probablement surestimé : `-pg` surpondère les fonctions à milliards d'appels. Le **nombre** d'opérations d'atlas doit baisser d'au moins ×10 pour 1 s à K10, et d'environ ×100 pour 100 ms. S2 K10 double ce volume (99,2 G).

### Répartition plausible des leviers (proposition, aucune promesse)

| Levier | Ce que la v8 a montré | Ce qu'il peut apporter | Condition |
| --- | --- | --- | --- |
| Parallélisme CPU | 686 % sur 8 vCPU locaux (776 % sans charge **[non commis]**) ; G4 non remesuré après la file | déjà compté dans $P_{\mathrm{eff}}$ ; aucun gain de CPU·s | files sans contention mesurées à W48, arêtes scindées, 24 contre 48 fils |
| Vitesse par fil G4 | indication ×1,25 à ×2,05 (`q34_spatial`) | réduit $R$ d'autant | à mesurer par un W1 G4 sur trame entière |
| Constantes (i64, arène, disposition mémoire, SIMD entier) | ×1,96 de CPU à S0 K5 sur la phase 1, en partie algorithmique (`0948d2d0`) ; `748ec082` seul : 48,5 → 39,9 s sur le quart (message de commit, non épinglé) | ×2 à ×4 plausibles sur les boucles chaudes | bornes de blocs i64 vectorisables sans flottant ; census q3 i128 : filtre flottant ou arithmétique multi-mots |
| Algorithmique (moins d'arêtes jusqu'à l'atlas, atlas partagé par rectangle, certificats collectifs) | cascade de filtres ×2,55 à ×4,46 sur l'**étape de filtrage** échantillonnée, sans supprimer d'atlas (`audits/SYNTHESE_PRIORITES_LIDAR_20260922.md:58-61`) | le reste : ×2 à ×50 pour 1 s | mesurer les atlas réellement évités |
| GPU | aucun code ni exécution v8 ; v6/v7 : noyaux = 2 à 4 % de leur étage | débit supérieur si prédicats compatibles et données résidentes | voir § 8 ; ne pas compter dessus avant une mesure de bout en bout |

## 5. Défauts, risques et dettes

| Gravité | Constat | Preuve |
| --- | --- | --- |
| Haute | Le flux seul est à un ou deux ordres de grandeur du contrat 1 s, trois pour 100 ms. La tour (q2, catalogue, fold) n'existe pas, donc son coût total ne se mesure pas. L'estimation ne couvre que le régime sans sol u16 à 2 cm, pas le contrat principal (trame brute, float32 ou 1 mm) | § 4 bis ; `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:52-57,69` ; `AGENTS.md:5-27` |
| Haute | Aucun code, build ni exécution GPU en v8 ; le protocole G4 v8 **exige** `GPU_executed is False` | § 3 ; `gcp-migration/cpu_probe_session_v8.py:89` |
| Haute | Le census q3 reste en i128 (bornes < 2^117) sans filtre flottant : non portable tel quel en SIMD ou GPU. Les bornes de blocs de l'atlas sont en i64 (< 2^62), avec un carré i128 par axe : portables en principe (SIMD entier 64 bits, entiers 64 bits sur GPU) sans filtre flottant, mais aucun essai | `src/lanes/q3_ball_census.cpp:12-40` ; `src/lanes/q4_local_partition.cpp:28-31,170-175,255-270` |
| Haute | La file de plages n'a jamais tourné sur G4 ; l'occupation mesurée à 48 fils (1,93 à 11,13 CPU) date d'avant `5224ff4e` | `receipts/q34_spatial_20260921/README.md:97-121` ; aucun reçu G4 postérieur à 12:16 UTC le 21 septembre |
| Moyenne | Un seul mutex protège push, pop, `task_pairs` et `busy` : **quatre** acquisitions par tâche publiée puis consommée, une par refus. Tout rectangle survivant est publié, même non scindé (10 802 scindés pour 1 037 747 tâches à S0 K5). Le régime actuel fait ≈ 9 600 tâches/s ; au régime 1 s ce serait ≈ 10^6 (K5) à 2 × 10^6 (K10) tâches/s sur 24 à 48 fils. Contention probable (**analyse, non mesurée**) | `src/pipeline/wspd_q34.cpp:429-435,761-773,782-806,812-816,831-835` |
| Moyenne | Les chronos par worker ne comptent pas le temps bloqué sur le mutex (seule l'attente sur la variable de condition est mesurée). L'écart CPU/mur de 13 à 16 % par worker (S0, reprise « calme ») n'est donc pas attribué. `5fdda963` l'impute à la charge de l'hôte sans mesure ; la course 1 mm sans charge (97 %) va dans ce sens | `src/pipeline/wspd_q34.cpp:782-788` ; message de `5fdda963` ; `workers_timing_ms` |
| Moyenne | Le gain d'occupation 430 → 686 % n'est pas isolé : la référence tournait sous charge 3,1 à 5,2 avec builds intermittents, la reprise sous charge < 3 ; trois autres commits séparent les deux sondes ; aucune ablation file active/inactive sur la scène entière (seulement le quart, message de `5224ff4e`). Le déséquilibre ×7,05 → ×1,99 des paires par worker soutient l'attribution sans la prouver | `receipts/ground_baseline_20260921/README.md:20-45` ; `receipts/ground_phase1_20260921/README.md:24-26` |
| Moyenne | Commentaires faux : « the first one here, the others as published tasks » (l. 424-426) et « publishes ranges of a heavy rectangle while it expands the first one » (l. 738-740), alors que la boucle publie toutes les plages, première comprise | `src/pipeline/wspd_q34.cpp:424-426,738-740` contre `:431-435` |
| Moyenne | Arête atomique : un cover peut contenir 78 % du nuage (30 923 sites à S0 K10) ; aucun chrono ni histogramme par arête ; chemin critique inconnu | `work.max_cover_sites` ; `src/pipeline/wspd_q34.cpp:818` ; `docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md:132-133` |
| Moyenne | Le chemin q34 avec file n'a jamais passé TSan (seule capture q34 : n=64/K5/W4, tranche 31) ; CMake ne propose pas TSan ; les builds « sanitize » u18 non commis sont ASan/UBSan | `receipts/lidar_global_20260921/README.md:15-16` ; `CMakeLists.txt:10,47-49` ; `build/v8_u18_resume_sanitize_20260922/CMakeCache.txt` |
| Moyenne | La porte de mutations `mhgp8_wspd_q34_mutations` a échoué une fois sous charge concurrente, puis a passé seule ; échec non expliqué | `docs/JOURNAL_DEVELOPPEMENT_20260921.md:128-130` |
| Moyenne | Couplage q3 → atlas q4 par arête (`0948d2d0`) : l'atlas est construit avant la voie q3, ce qui allonge la chaîne intra-arête et empêche de supprimer un atlas quand seule q4 est rejetée | message de `0948d2d0` ; `audits/SYNTHESE_PRIORITES_LIDAR_20260922.md:90-92` |
| Moyenne | Sorties parallèles comparées comme multiensemble ou par digests commutatifs : aucun ordre canonique ; le tri de la tour n'est pas compté | `docs/P0_FRONT_WORKERS_Q2.md:46` ; `bench/wspd_q34_probe.cpp:141` |
| Moyenne | Plan de la phase 2 non livré : sorties triées par clé canonique, porte W1/W2/W4/W8 bit-identique sur nuage sans sol, mutants `--inject` du chemin parallèle (job perdu, dupliqué, fusion décalée). Seuls la file et les chronos ont été livrés | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:185-192` |
| Moyenne | Le plan fixait la première session G4 « quand le local a gagné ≥ ×5 sur la base » ; le gain est ×1,96 en CPU W1 et ×2,8 en mur W8. Le portage G4 du moteur à file demande cinq étapes non faites, avec un budget utile de 900 s incompatible avec des lignes W1 à K10 | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:190-192` ; `docs/JOURNAL_DEVELOPPEMENT_20260921.md:71-99` |
| Moyenne | q2 jamais mesuré sans sol, exclu de la sonde q34 (masque 6) ; à K10, q2 seul coûte déjà 3 à 5,5 s sur 4 workers pour des préfixes LiDAR 50k avec sol | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:64` ; `receipts/q2_dynamic_front_20260914/README.md:64-66` |
| Moyenne | Mesures sur hôte partagé, une répétition ; lignes S1/S2 de la phase 1 et S2 de la référence contaminées (sentinelle périmée) ; trois lignes S0 de la phase 1 non vérifiables. Les rapports CPU W8/W1 de S1/S2 (1,624 et 1,573) héritent de cette contamination | `receipts/ground_phase1_20260921/README.md:19-29,51-58` ; `receipts/ground_baseline_20260921/README.md:37-45` |
| Moyenne | CPU W8 = 1,57 à 1,64 × CPU W1 (SMT, cache, fréquence ; non isolé faute de ligne W4) : ne jamais comparer un CPU·s W8 à un CPU·s W1, ni deux CPU·s W8 d'occupations différentes | `time_*.txt` des deux reçus ; identité u18 **[non commis]** |
| Basse | Marge d'un seul bit en i64 pour les bornes de blocs de l'atlas à 18 bits ; le document garde la conception Q=2^18 (l. 45-47) alors que l'implémentation garde Q=2^20 (l. 94-96), sans réconciliation du texte | `docs/ELARGISSEMENT_18_BITS_20260922.md:45-47,94-96` ; `src/lanes/q4_local_partition.hpp:21` |
| Basse | Profil par symbole seulement avant la phase 1 ; profil post-phase 1 (atlas ≈ 52 %, census q3 12 %, filtres 13 %) sans reçu | `docs/JOURNAL_DEVELOPPEMENT_20260921.md:60-65` |
| Basse | Les deux variantes rejetées de l'atlas n'ont qu'une trace de journal | `git show --stat 0e2c18ca` : un seul fichier |
| Basse | Binaire de sonde épinglé par sha256 mais non archivé (chemin de scratchpad disparu) : le rejeu exige une reconstruction à `0e2c18ca` | `receipts/ground_phase1_20260921/BASELINE.only.json` (`probe`, `probe_sha256`) |
| Basse | Threads créés à chaque appel ; construction de l'index mono-thread (14,5 à 15,4 ms à 40k en u16 ; 96 à 130 ms pour l'index float32 des trames brutes) : négligeable à 1 s, pas à 100 ms | `src/parallel/joined_workers.hpp:16` ; `morsehgp3D_v8/README.md:114-115` |
| Basse | Entrées LiDAR préparées non versionnées ; reproductibles par recette, sha256 épinglé (`b0918930…` vérifié pour S0) | `morsehgp3D_v8/audits/lidar08_20260914/.gitignore:2` ; `BASELINE.only.json` |
| Basse | Mémoire non qualifiée pour la tour : 12,2 à 14,2 Mio en mode digest (15,0 à 16,5 Mio **[non commis]**) ; la v7 atteignait 15,5 Gio à 50k/K10 | `docs/VERROUS_ARCHITECTURE.md:350-354` |
| Basse | **[non commis]** Route GPU C6a de la v6 (`src/gpu/route_c6.hpp`, `tests/route_c6_gate.cpp`, `tests/perm_sort_gate.cpp`, non suivis depuis le 2 septembre) et `lot_ring.hpp` modifié (15 septembre) dans le worktree partagé : un portage v9 qui lirait le worktree prendrait un code jamais qualifié | `git status --short morsehgp3D_v6` (worktree) |

## 6. Questions ouvertes

1. Quelle occupation et quelle accélération la file donne-t-elle sur G4 à W24 et W48, sur les trois nuages sans sol ? Quel est $s$ mesuré directement par un W1 G4 sur trame entière, et le SMT de l'EPYC 9B45 se comporte-t-il comme celui de l'hôte local (1,13 à 1,22 équivalent mono-fil par cœur) ?
2. Combien de temps les fils passent-ils bloqués sur le mutex à W48, par opposition à l'attente de variable de condition ?
3. Quel est le temps de l'arête la plus lourde, et quelle est la distribution des coûts par arête (cover, graines, opérations d'atlas, ns) ? Tant que l'arête reste atomique, ce maximum borne inférieurement le mur.
4. Combien d'arêtes et d'opérations d'atlas les certificats collectifs et la cascade de rectangles suppriment-ils réellement sur l'appel q3/q4 complet ?
5. Quelle borne d'erreur prouvée permet un filtre flottant pour les bornes de census q3 (degré 6 en M, < 2^117), et à quel taux de repli ? La voie float32 v8, filtrée mais ×20 à ×70 plus lente, montre qu'un filtre ne suffit pas à gagner. Les bornes de blocs i64 de l'atlas gagnent-elles davantage en SIMD entier (AVX-512 sur la G4, non documenté) ?
6. Quel est le coût de la voie q2 sans sol, puis de la déduplication, du catalogue et du fold à K5 et K10 ? Le « 1 s » porte sur la tour entière d'une trame brute (`docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:9-13`).
7. Combien coûte la phase 1 sur trames brutes, et à K10 ?
8. La file à mutex unique tient-elle 10^6 à 2 × 10^6 tâches/s, ou faut-il des files par worker avec vol de travail ?
9. Quelle première étape GPU donne un gain de bout en bout mesurable, transferts et reconstruction hôte comptés (v6 : 88 % d'hôte ; v7 : kernels à 4,2 % de la phase) ?
10. La v9 garde-t-elle le seuil « ≥ ×5 en local avant G4 » du plan v8, ou le remplace-t-elle par une session de mesure d'information (s, $P_{\mathrm{eff}}$) ?

## 7. À porter en v9 et à ne pas reprendre

### À porter (quoi, où, pin, pourquoi)

| Quoi | Où (pin `12294241` sauf mention) | Pourquoi |
| --- | --- | --- |
| Équipe jointe sûre (jointure avant propagation, lanceur injectable pour l'échec partiel) | `morsehgp3D_v8/src/parallel/joined_workers.hpp` | discipline d'exception et de durée de vie testée par portes |
| Réduction explicite des compteurs avec `static_assert` de taille | `morsehgp3D_v8/src/parallel/work_reduction.hpp:67` ; `src/pipeline/wspd_q34.cpp:29-229` | un champ oublié casse la compilation, pas la porte |
| Registre des tâches : `published == consumed`, `completed_jobs == jobs`, refus → développement local, jamais de perte ; mutant « plage refusée non développée » | `morsehgp3D_v8/src/pipeline/wspd_q34.cpp:863-872` ; `tests/wspd_q34_gate.cpp:519-529,942` ; `tests/wspd_q34_mutations.py:53-67` (commit `5224ff4e`) | invariants de conservation du travail, valables quel que soit l'ordonnanceur |
| Porte parallèle contre oracle rationnel indépendant (W1/W2/W4, grain 2 et file 1, planchers de scissions et de refus) | `morsehgp3D_v8/tests/wspd_q34_gate.cpp:449-529,942` | juge le parallèle contre la vérité, pas seulement contre le mono |
| Chronos par worker (mur, CPU du fil, attente), **à compléter** par le temps de verrou | `morsehgp3D_v8/src/pipeline/wspd_q34.cpp:720-735,841` (commit `5fdda963`) | sépare le déséquilibre du reste, mais pas encore la contention de la charge de l'hôte |
| Lanceur de campagne appariée qui refuse d'écraser un reçu | `morsehgp3D_v8/bench/run_ground_baseline.py:133-145` (commits `5224ff4e`, `72f125c6`) | leçon de la sentinelle périmée |
| Chiffres de référence sans sol | `receipts/ground_baseline_20260921` (`92d74c13`) et `receipts/ground_phase1_20260921` (`0e2c18ca`) | base différentielle : sorties, 439 compteurs, temps |
| Front q2 Coarse multi-CPU | `b268cf6f`, `receipts/q2_front_workers_20260914` | seule variante q2 parallèle avec gain établi (×3,4 à ×3,9 sur 4 cœurs physiques, hors rangées) |
| Protocole G4 CPU v8 (sources gelées, inventaire exact archivé, arrêt certifié) et son plan de portage en cinq étapes | `gcp-migration/cpu_probe_*_v8.py`, `gcp-migration/q34_spatial_*_v8.py` ; `docs/JOURNAL_DEVELOPPEMENT_20260921.md:71-99` | à étendre en protocole GPU v9 (nvcc, contrôle device, `GPU_executed` vrai, budget utile élargi) |
| Contrat des baux d'un anneau de lots (hôte pur, jamais branché ni mesuré) | `morsehgp3D_v6/src/gpu/lot_ring.hpp` **version commise** (pas la version modifiée du worktree) | sa porte prouve la discipline d'ordonnancement sur un modèle déterministe sans fil ni flux ; elle ne prouve ni le matériel, ni l'absence de course, ni les transferts asynchrones (`lot_ring.hpp:10-27`) |
| Exécutions device du dépôt : v6 (étage device C1–C5, 154 ms de noyaux sur 7 717 ms) ; v7 tour 50k avec census CUDA (189 ms de kernels, porte Blackwell 16 627 contrôles sur 4 116 boules) ; v7 primitives (MEB par lots 605 cas ; clé/PGCD/division 128 bits 13 573 cas) | `morsehgp3D_v6/docs/GPU.md:219-222` ; `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md:32-39` ; `morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md:16-19` | seules preuves device ; les annotations HD et l'ABI 104/232 octets de la variante terminale par lots (238 registres, 1 392 octets de pile) n'ont **aucun résultat device** (`morsehgp3D_v7/docs/PARALLELISATION_PAR_LOTS_20260911.md:25,53,55`) |
| Leçons chiffrées de résidence | `morsehgp3D_v6/docs/GPU.md:219-222` ; `morsehgp3D_v6/docs/ECHELLE.md:148-150` ; `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md:32-36` | kernels à 2 à 4 % de leur étage, −10,4 % du mur au mieux : concevoir la résidence avant les kernels |

### À ne pas reprendre

| Piste | Mesure qui l'a fermée |
| --- | --- |
| Jobs Coarse par sous-arbres entiers comme seule unité | G4 W48 : 1,93 à 11,13 CPU occupés (`receipts/q34_spatial_20260921/README.md:97-114`) ; 3,23 CPU à 8k (`receipts/lidar_global_20260921/README.md:149-152`) |
| q2 `Donate` par défaut | pas de gain général, étendues communes, régression amas 8k (`receipts/q2_dynamic_front_20260914/README.md:56-70`) |
| Équipe créée par ancre (détachement + répartiteur) | 8/72 comparaisons favorables, ratio médian 0,0738 (`receipts/q2_census_split_20260915/README.md:82-87`) |
| Équipe coopérative à continuations de 6 272 octets | 17/18 comparaisons rangées plus lentes (`receipts/q2_cooperative_20260915/README.md:69-80`) |
| Plages d'ancres comme levier de vitesse | ratio médian 1,039, pas de gain général établi (`receipts/q2_anchor_ranges_20260915/README.md:94-96`) |
| Lots de singletons entrelacés | plus lents que Coarse de ×1,005 à ×1,225, médiane 1,112, sur 54 comparaisons (`receipts/q2_singleton_batch_20260915/README.md:103-105`) |
| Reprise exacte de la descente du proposeur q2 | petit gain ×0,94 à ×0,98, jugé insuffisant et non porté (`morsehgp3D_v8/README.md:388-389`) : pas un négatif, une piste à faible rendement |
| Cache de formes dans la frontière des fragments d'atlas | 27,32 s contre 27,32 s (journal seulement, **non vérifiable**, `docs/JOURNAL_DEVELOPPEMENT_20260921.md:47-50`) |
| Classification conjointe des quatre cellules filles | +18 % (31,7 s contre 26,8 s ; journal seulement, **non vérifiable**, `docs/JOURNAL_DEVELOPPEMENT_20260921.md:51-58`) |
| Mode `Joined` graines × cellules | aucun gain supplémentaire stable (`morsehgp3D_v8/README.md:166-168`) |
| GPU par petits lots synchrones avec reconstruction hôte ligne par ligne | v7 : 0,189 s de kernels pour 4,540 s de phase ; v6 : 154 ms pour 7 717 ms |
| Ajouter des fils sans réduire le travail | les compteurs géométriques ne dépendent pas des workers ; le gain de CPU·s est nul |
| Filtre flottant supposé gratuit | la voie binary32 filtrée coûte ×20 à ×70 par opération contre l'u16 (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65,92`) |

## 8. Recommandations priorisées pour la v9

### Architecture parallèle et GPU proposée (sans promesse)

1. **Plan de travail plat, commun CPU/GPU**, par étages synchrones avec compaction : front et filtres de rectangles → liste compacte des arêtes résiduelles (sommes préfixes) → covers en plages CSR → graines en CSR → cellules d'atlas → enregistrements de candidats → tri parallèle par clé canonique et RLE. L'ordre canonique remplace les callbacks et rend la sortie déterministe (`docs/PLAN_DE_REFONTE.md:615-634`).
2. **CPU** : pool persistant au niveau du processus, réutilisé par toutes les phases, affinité mesurée, 24 et 48 fils comparés. Files par worker avec vol de travail, grains pondérés par le travail estimé (masse de paires × taille de cover). Publier seulement les rectangles au-dessus d'un seuil de travail, pas chaque survivant. Arêtes scindées en tâches (arête × bloc de graines) et (arête × sous-arbre d'atlas) qui partagent un parent immuable (`docs/Q4_BLOCS_SEEDS_PISTE_20260921.md:197-199`). Conserver les identités de registre du § 7.
3. **Instrumentation obligatoire** : histogramme du coût par arête (les 100 arêtes les plus lourdes avec cover, graines, opérations d'atlas et ns), temps par phase, **temps de verrou**, attente et vol par worker, estimation du chemin critique.
4. **Numérique pour SIMD/GPU** : d'abord les bornes de blocs i64 de l'atlas en SIMD entier, sans flottant ni repli. Pour le census q3 en i128, un filtre flottant certifié à repli exact (borne d'erreur écrite, fixture de contact, mutant, compteur de replis publié), jugé contre la leçon de la voie float32 (×20 à ×70 plus lente).
5. **GPU, ensuite seulement** : socle sans carte (format de fil versionné, stub hôte, validateur transactionnel, époques, baux de `lot_ring` dans sa version commise), index résident (7,7 Mo u16, 8,7 Mo u18, `docs/JOURNAL_DEVELOPPEMENT_20260921.md:138-139`), premier kernel sur la famille la plus volumineuse (tests de l'atlas par lots arête × cellule × bloc), puis bornes de census q3. Critère d'arrêt : si le code hôte dépasse la moitié de l'étage GPU, corriger la résidence avant d'ajouter des kernels.
6. **Protocole G4 v9** : étendre les scripts gardés (nvcc, contrôle device, budget, `GPU_executed` vrai), toujours SPOT, deux coupe-circuits, arrêt certifié.

### Priorités

1. **Mesurer avant d'optimiser plus loin, dans le budget réel** : exécuter les cinq étapes de portage du journal, puis des sessions G4 CPU séparées. Une session W24/W48 sur les trois scènes sans sol à K5 et K10, avec chronos, temps de verrou et histogramme par arête. Une session courte W1 (S0 K5, une trame brute) pour mesurer $s$. W1 à K10 dépasse le budget utile de 900 s : ne pas le planifier dans une seule session. Trancher explicitement le seuil « ≥ ×5 local » du plan v8.
2. **Raccorder la tour avant de revendiquer un budget** : voie q2 sans sol, déduplication, catalogue et fold dans le même appel chronométré. Le flux q3/q4 n'est qu'un minorant, et le contrat principal porte sur la trame brute.
3. **Réduire d'au moins ×10 à K10 le nombre d'opérations d'atlas** : certificats d'arête et cascade de rectangles avant l'atlas, atlas partagé entre les arêtes d'un même rectangle. Mesurer les atlas évités, pas le seul filtre. C'est le levier principal pour 1 s.
4. **Remplacer la file à mutex unique** par des files par worker avec vol de travail, et scinder les arêtes lourdes. Porte TSan intégrée à CMake (option dédiée), porte W1/W2/W4/W8 bit-identique sur sorties triées canoniquement, mutants du chemin parallèle (tâche perdue, dupliquée, fusion décalée) : c'est la phase 2 du plan v8, non livrée.
5. **Arithmétique vectorisable** : SIMD entier pour l'atlas, filtre flottant certifié pour le census q3.
6. **Premier étage GPU** seulement après 1 à 5, transferts et reconstruction comptés, sorties bit-identiques au CPU après tri canonique.
7. **Hygiène de mesure** : hôte calme, au moins trois répétitions, jamais deux campagnes simultanées, CPU·s W8 jamais comparés à W1 ni entre occupations différentes, ligne W1 obligatoire pour tout surcoût revendiqué, profils par symbole épinglés en reçu après chaque tranche, binaires de sonde archivés avec leur sha256.

## Contre-vérification

Chaque affirmation principale et chaque chiffre du rapport d'origine a été rouvert à sa source (fichier:ligne, JSON, `time_*.txt`, message de commit). Corrections intégrées :

1. **`GPU_executed` dans « 11 JSON »** → **10 JSON** (3 sous `q34_spatial_20260921`, 7 sous `lidar_global_20260921/gcp_r{1,2,3}_*`) ; le onzième fichier est le script `lidar_global_20260921/archive_gcp.py`. Le constat « aucun GPU en v8 » est confirmé.
2. **« Occupation 425–430 % → 684–686 %, avec des sorties et des compteurs identiques »** → les sorties et les compteurs d'émission sont identiques ; les compteurs de travail q3 diffèrent (28 champs, dont `q3.ball_builds` 179,7 M → 31,0 M à S0 K5), ceux de l'atlas restent identiques. Le gain d'occupation est confondu avec la charge de l'hôte (référence sous charge 3,1 à 5,2) et avec trois autres commits ; aucune ablation sur scène entière.
3. **« Le census q3 et l'atlas … ne sont donc portables ni en SIMD ni en GPU »** → vrai pour le census q3 (i128). Les bornes de blocs de l'atlas sont en i64 depuis `748ec082` (< 2^62 à 18 bits, un carré i128 par axe) : portables en principe par SIMD entier ou GPU sans filtre flottant.
4. **« Le filtre certifié n'est que proposé »** → vrai pour le moteur u16. La voie float32 v8 possède déjà des prédicats d'intervalle filtrés à repli exact (`Float32PredicateMode::Filtered`), mesurés ×20 à ×70 plus lents par opération que l'u16.
5. **« ≥ 3 acquisitions par tâche »** → **4** acquisitions par tâche publiée puis consommée (push, pop, `task_pairs`, `busy--`), une par refus.
6. **Commentaire faux** : un second commentaire faux existe, `wspd_q34.cpp:738-740`.
7. **Chronos par worker, « seule mesure qui sépare déséquilibre et charge de l'hôte »** → ils ne mesurent pas le blocage sur le mutex ; l'écart CPU/mur de 13 à 16 % par worker n'est pas attribué.
8. **Budget « s = 1 faute de mesure »** → le reçu `q34_spatial` apparie local W4 et G4 W48 à travail identique sur la trame 0 : rapport CPU·s de 2,05 et de mur de 2,32 (`README.md:55-59,100,117,130-132`). $s$ serait plutôt de 1,25 à 2,05 (indication confondue).
9. **Critique du « ×46 » (« les deux biais se compensent partiellement ; ×28 à ×56 plus défendable »)** → diviser le CPU W8 par 48 revient exactement à transporter le SMT local sur la G4 (27 à 29 équivalents mono-fil). La borne P=48 de la variante A contredit ce SMT mesuré. L'incertitude dominante est $s$. Fourchette révisée pour 1 s sur le flux seul : ×6 à ×36 à K5, ×17 à ×101 à K10 selon la variante.
10. **Périmètre du budget** → l'estimation ne vaut que pour le régime sans sol u16 à 2 cm. Le contrat principal (trame brute, float32 par défaut ou 1 mm) n'est pas couvert. Estimation grossière ajoutée pour les trames brutes : ×13 à ×41 à K5 avec le moteur d'avant la phase 1.
11. **« À 100 ms, plus de 100 ns par arête dépasse déjà le budget »** → le budget vaut 78 à 200 ns à S0 selon $P_{\mathrm{eff}}$ et K ; 100 ns ne le dépasse qu'avec P=24 (avec P=48, 156 à 200 ns). La conclusion d'ordre de grandeur est inchangée.
12. **« Gonflement SMT »** → rapport CPU W8/W1 mesuré (1,636 / 1,624 / 1,573). Son attribution au seul SMT n'est pas isolée (aucune ligne W4). Les valeurs S1/S2 héritent de la charge croisée.
13. **Index « 14,5 ms »** → 14,5 à 15,4 ms à S0 selon la ligne ; 12,9 à 15,7 ms ailleurs ; 40,7 ms sur la ligne contaminée S2 K10.
14. **« CPU·s = temps utilisateur »** → utilisateur + système (ex. 740,94 + 0,56 = 741,5).
15. **[non commis] « L'élargissement à 18 bits coûte +5 à +8 % de CPU »** → valeurs exactes (+5,2 à +8,3 %). L'attribution n'est pas établie : l'occupation passe de 462–686 % à 672–765 % et le mur ne monte pas (105,0 contre 108,0 s à S0 K5, 322,0 contre 323,0 s à S0 K10). Sans ligne W1, le surcoût CPU W8 peut venir du SMT. Les compteurs logiques sont identiques, seuls des octets de pile changent.
16. **« Les seules exécutions device du dépôt sont les primitives v7 »** → **réfuté** : la v6 a exécuté son étage device (154 ms de noyaux, `GPU.md:219-222`), et la v7 a exécuté son census CUDA dans la tour 50k (189 ms de kernels, porte Blackwell de 16 627 contrôles, `RESULTATS_TOUR_CACHE_G4_20260910.md:32-39`). L'ABI 104/232 octets et les 238 registres relèvent d'une variante sans résultat device.
17. **`lot_ring` « ordonnancement des transferts asynchrones déjà prouvé par porte »** → la porte prouve la discipline d'ordonnancement sur un modèle déterministe sans fil, flux ni horloge. Elle ne prouve ni le matériel ni l'absence de course (`lot_ring.hpp:10-27`). Porter la version commise : le worktree porte une version modifiée et une route C6a non suivies.
18. **Donate, « pas de gain général »** → confirmé, avec une nuance : les médianes Donate sont plus basses sur uniforme 8k et LiDAR 50k, mais les étendues se recouvrent.
19. **Lots de singletons « ×1,005 à ×1,225 »** → ce sont des **ralentissements** (lots plus lents que Coarse).
20. **Reprise de la descente du proposeur « ×0,94 à ×0,98, non portée »** → petit gain non porté, pas un négatif.
21. **Recommandation P1 (session G4 W1/W24/W48, trois scènes, K5 et K10)** → infaisable en une session : budget utile de 900 s, et W1 K10 S2 estimé à environ 2 400 CPU·s locaux. Elle contredit aussi le seuil « ≥ ×5 local » du plan v8, et exige d'abord les cinq étapes de portage du journal. Scindée en sessions distinctes.
22. **Écart de CPU entre workers « attentes de quelques ms »** → 0 à 3,8 ms à S0 K5, jusqu'à 25,5 ms à S0 K10 et 45,1 ms à S2 K5 : négligeable face au mur, mais pas « quelques ms » partout.
23. **Profil gprof** → chiffres confirmés, avec une réserve : `-pg` instrumente chaque appel et surpondère `node_bounds_unchecked` (3,16 G appels) ; la proportion Release « ≈ 93 ns » est probablement surestimée.
24. **Ajouts d'omissions** : plan de la phase 2 non livré (sorties canoniques, porte bit-identique sur nuage sans sol, mutants `--inject`) ; porte de mutations q34 instable sous charge (`JOURNAL:128-130`) ; binaire de sonde non archivé ; route C6a v6 et `lot_ring.hpp` modifiés non commis dans le worktree ; régression rangées ×1,01 à ×1,18 de la tranche 20 ; `static_assert` de réduction aussi dans `wspd_q34.cpp:29-229`.

Confirmés sans changement : absence de code GPU v8 et `CMakeLists.txt:2` ; file jamais mesurée sur G4 ; chiffres S0/S1/S2 des reçus de référence et de phase 1 ; CPU par worker (écart 2 à 6 %) ; paires par worker ×7,05 → ×1,99 ; tâches, refus et pic de file ; occupation G4 avant la file et préfixes 4k/8k ; compteurs d'atlas S0/S2 K10 identiques entre les deux reçus ; graines et bornes de census q3 ; arêtes développées ; plus gros cover ; sorties S0 K10 ; RSS ; front q2 Coarse ×3,91/×3,40/×3,89/×1,93 ; q2 50k ; v6/v7 historiques ; absence de TSan sur la file (y compris dans les builds non commis) ; q2 jamais mesuré sans sol ; sha256 `4c8da44d…`, `d2a3d52f…`, `61faf50d…`, `b0918930…` ; valeurs non commises de la première trame 1 mm.
