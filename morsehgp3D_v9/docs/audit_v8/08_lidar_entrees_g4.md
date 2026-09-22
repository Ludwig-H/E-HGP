# Lentille 8/12 — Entrées LiDAR, sans-sol, licences et protocole G4 (v8), version contre-vérifiée

Cadre : `phase=audit_general_v8_avant_v9`, `backend=cpu_reference` (lecture seule), `mode=audit_independant`, `public_status=not_claimed`. **GCP non utilisé** : aucune commande `gcloud`, aucune compilation, aucun ctest, aucun script du dépôt exécuté. État de référence : worktree détaché `origin/main` **12294241**. Tout élément tiré du worktree partagé `/workspaces/E-HGP` hors de ce commit est marqué **[NON COMMIS]**. On précise **[NON SUIVI]** quand le fichier n'est même pas indexé.

Ce document reprend le rapport `08_lidar_entrees_g4.md` après une contre-vérification adversariale. Chaque affirmation et chaque chiffre du rapport d'origine a été rouvert à sa source : fichier et ligne, commit, JSON ou archive lue en flux. Les corrections sont intégrées au texte ; la section 9 les liste une à une.

Vocabulaire des statuts : **prouvé** (preuve écrite + fixture), **testé** (porte bornée), **mesuré** (reçu épinglé : commit, sha256, sorties), **proposé**, **manquant**. Un chiffre sans reçu est **non vérifiable**.

## 1. Périmètre lu

### Lu intégralement ou sur les passages utiles

| Source | Portée |
|---|---|
| `morsehgp3D_v8/audits/lidar08_20260914/{README.md, FETCH_MANIFEST.json, PREPARATION_CHECKS.json, fetch_kitti08.py, .gitignore}` | audit LiDAR du 14 septembre, fetch borné |
| `morsehgp3D_v8/docs/{LIDAR_SANS_SOL_PROTOCOLE, PILOTE_LIDAR_SANS_SOL, CONTRAT_TRAMES_SEMANTICKITTI}_20260921.md`, `ELARGISSEMENT_18_BITS_20260922.md` (l. 1–70), `JOURNAL_DEVELOPPEMENT_20260921.md` (l. 40–105), `AUDIT_REPRISE_DEVELOPPEUR_20260921.md` (l. 30–130, 210–235), `PASSATION.md` (l. 10–50) | protocoles, contrat, contexte |
| Reçus `lidar_ground_20260921/README.md` (intégral), `lidar_ground_u16_20260921/{README.md, MANIFEST.json}`, `lidar_spatial_20260921/README.md`, `float32_precision_20260921/README.md`, `lidar_global_20260921/README.md` + `gcp_r{1,2,3}*/ARCHIVE.json` + archives `host_capture.tar.gz` (lues en flux), `q34_spatial_20260921/{README.md, GCP_PLAN.json, GCP_PREFLIGHT_KEY_MODE.json, gcp_r1/READBACK.json, gcp_r1_optimized/READBACK.json, performance/spatial_9kscvyt0/record_0006.json}`, `ground_baseline_20260921/README.md`, `ground_phase1_20260921/README.md`, `q4_seed_cells_20260921/qualification_r2/smoke_ewedfs4y/MANIFEST.json` | mesures, préparations, autorité native |
| Archives `q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz` et `gcp_r1{,_optimized}/host_evidence.tar.gz` : inventaires, `receipt.json` du worker, commandes horodatées, `guarded_{start,stop}.*`, `oslogin_add.stdout` | protocole G4 exécuté |
| `gcp-migration/q34_spatial_{worker,snapshot,session}_v8.py` (passages cités), `README_CPU_PROBE_V8.md` (l. 110–160), `receipts/q34_spatial_20260921/local_zzork064/{COMPLETION,normal,optimized}.json` | protocole G4 |
| `morsehgp3D_v8/bench/{patchwork_ground_probe.cpp (l. 38–131), build_patchwork_ground.py (l. 25–40), run_ground_baseline.py (l. 25–125), wspd_q34_probe.cpp (l. 199–221), q4_lidar_probe.cpp (l. 20–50)}`, `CMakeLists.txt` (l. 32–70, 120–165, 468–484, 550) | code d'entrée, CMake |
| `.github/workflows/morsehgp3d-v8-lidar-audit.yml` (intégral), `audits/lidar_rectangles_20260922/{README.md, run.py}` | audit LiDAR 1 mm du 22 septembre |
| `AGENTS.md` (l. 1–40), `README.md` racine (Licences), `CLAUDE.md` (Licences), `LICENSE`, `audits/COORDINATION_MORSEHGP3D_V8.md` (recherche « licence / KITTI / TERMINATED ») | normes |
| [NON COMMIS] `receipts/u18_resume_20260922/README.md`, `ground_1mm_first/{BASELINE.only.json, only_time_01_s00_k5_w8.txt}` [NON SUIVI], `receipts/ground_18bits_20260922/u16_identity/BASELINE.only.json` (indexé), `git diff HEAD -- AGENTS.md` | tranche du constructeur (dernière écriture 10:56 UTC) |

### Vérifications actives (lecture seule)

- Hachage **en flux** (`tar -xzf … -O | sha256sum`) des trois `RAW.bin` du snapshot G4 et des deux copies imbriquées du snapshot : empreintes égales à `FETCH_MANIFEST.json`, rien écrit sur disque.
- Recomptage des tailles versionnées par `git ls-tree -r -l origin/main`, par répertoire et par extension.
- Recalcul des 216 empreintes de l'autorité native contre les fichiers de 12294241, et comparaison de l'inventaire des unités `.cpp` du worker avec `src/` et avec `add_library(mhgp8_p0 …)`.
- Recalcul des durées des quatre sessions depuis `guarded_start.command.json` et `guarded_stop.command.json`, et du budget du worker depuis les horodatages de ses 40 commandes.
- Comparaison ligne à ligne des `logical_sha256` et des sorties de l'identité u16 [NON COMMIS] avec la référence embarquée.

### Non lu

`start_and_verify.sh` et `stop_and_verify.sh` en entier ; `read_cpu_probe_v8.py`, `read_q34_spatial_gcp.py`, `run_lidar_*_checks.py` ; `prepare_lidar_precision.py` et `prepare_lidar_spatial.py` en entier ; `Q34_GLOBAL_ET_LIDAR_20260921.md`, `RACCORD_NATIF_GLOBAL_PLAN_20260921.md`, `PRECISION_FLOAT32_ET_GRILLE_20260921.md` ; `SYNTHESE_PRIORITES_LIDAR_20260922.md` ; `gcp_r3_completed/LECTURE_RESULTATS.md`, `ANALYSIS.json`, `CPU_COSTS.json` ; le canal `COORDINATION_MORSEHGP3D_V8.md` hors recherches ; les archives jointes « à la conversation » par l'auditeur (`MorseHGP_LiDAR_natif_resultats_2026-09-22.zip`, `MorseHGP_LiDAR_cascade_2026-09-22.zip`), absentes du dépôt ; les artefacts GitHub Actions ; la visibilité publique ou privée du dépôt ; les pages officielles de licence KITTI et SemanticKITTI ; les labels SemanticKITTI (jamais téléchargés) ; [NON COMMIS] `u18_resume_20260922/{release,release_r2,sanitize,sanitize_r2}` en détail.

## 2. Ce qui a été fait

| Date | Commit | Événement |
|---|---|---|
| 14 sept. | 9ae8a268 | Audit « lidar08 » : fetch borné de 7 scans de la séquence 08 (000000–000004, 000100, 000200), calibration et poses, par 37 requêtes Range (19 378 167 octets, 9 fichiers). Préparation u16 à 2 cm dans le repère du scan 0, préfixes emboîtés 8k/16k/32k/50k ordonnés par priorité BLAKE2b, 36 appels du front sur les sources da366f7f (`audits/lidar08_20260914/README.md:15-36, 70-78, 119-121`). |
| 21 sept., 07:16–08:12 UTC | 4dbe3024 (08:18) | Raccord global q3/q4 et trois sessions G4 CPU : R1 échec de compilation, R2 gate bloquée, R3 cinq mesures, toutes sur des préfixes du fichier 8k du scan 0 (`receipts/lidar_global_20260921/README.md:98-163`). |
| 21 sept. | 759ce2b0 | Protocole spatial : scène entière, moitiés `x<0` / `x>=0`, quatre quarts, repère capteur propre, 21 nuages à 2 cm (`docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md`, `receipts/lidar_spatial_20260921/README.md:20-24`). |
| 21 sept., 11:55–12:09 UTC | 70de84f2 (12:16) | Contrat « trame SemanticKITTI entière » ; protocole G4 `q34_spatial_*_v8.py` ; session SPOT sur un quart et trois trames entières, K5/s8/W48 ; arrêt certifié TERMINATED à 12:09:34 UTC (`receipts/q34_spatial_20260921/README.md:87-156`). |
| 21 sept. | 36724438 | Préparateur float32 sans perte et grille 1 mm, 42 nuages, primitive q2 exacte (`receipts/float32_precision_20260921/README.md:34-62`). |
| 21 sept. | a005f8aa | Protocole sans sol : recherche bibliographique, Patchwork++ recommandé (`docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md:22-36`). |
| 21 sept. | 204b0620 | Pilote Patchwork++ 3e6903a1 : masque u8 par retour, 44 commandes Release et 17 ASan/UBSan/LSan, 42 nuages sans sol float32 et 1 mm (`receipts/lidar_ground_20260921/README.md:9-75`). |
| 21 sept. | ec2bc503 | Nuages sans sol au profil u16 à 2 cm : 39 815 / 35 491 / 45 114 sites (`receipts/lidar_ground_u16_20260921/README.md:26-30`). |
| 21 sept. | 92d74c13 | Portes float32, LiDAR (`lidar_precision_gate`, `lidar_spatial_gate`, `lidar_ground_test`, normal et `-O`) et spatiale enregistrées dans CTest, label `lidar` (`CMakeLists.txt:468-484, 550`). |
| 21–22 sept. | 92d74c13 → 0e2c18ca, reçus 72f125c6 | Base de temps sans sol (sonde 92d74c13), puis campagne appariée après les phases 1–2 (sonde 0e2c18ca, jeton `atlas`) (`receipts/ground_baseline_20260921`, `receipts/ground_phase1_20260921`). |
| 22 sept. | a74e90f2 | Élargissement du moteur entier à 18 bits (grille 1 mm, ±131 m). Entrées 1 mm = `scene_0X_grid/full.u32le` du reçu `lidar_ground_20260921` (`PASSATION.md:36-43`). |
| 22 sept. | 7a121e44 → 12294241 | Auditeur : workflow Actions `morsehgp3d-v8-lidar-audit.yml` (introduit par 7a121e44), mesures natives 1 mm de filtres de rectangles, cascade recommandée (`audits/lidar_rectangles_20260922/README.md`). |
| 22 sept. [NON COMMIS] | base a74e90f2, arbre sale | Reprise u18. Première ligne entière sans sol à 1 mm (08/000000, 39 885 sites, K5/s8/W8) [NON SUIVI]. Identité u16 après le port 18 bits sur six lignes W8 (indexée, non commise). |

Sessions G4 de la v8 : **quatre**, toutes le 21 septembre, toutes CPU, toutes arrêtées et certifiées TERMINATED. S'y ajoute un préflight refusé **avant toute action cloud** (`GCP_PREFLIGHT_KEY_MODE.json` : clé en mode 0644, `vm_started=false`). Aucune session le 22 septembre : aucun reçu `gcp-migration/receipts/` postérieur au 21, aucun commit sous `gcp-migration/` depuis, et la reprise [NON COMMIS] déclare « Aucun usage GCP ».

## 3. État par composant

| Composant | Statut | Preuve | Limite |
|---|---|---|---|
| Acquisition KITTI 08 (7 scans, calib, poses) | **mesuré** (fetch clos, empreintes) | `FETCH_MANIFEST.json` : Range 206, ETag, `Content-Range`, CRC32 ZIP ; sha256 *enregistrés* ; empreintes des trois scans utilisés recontrôlées | séquence 08 seule ; `FRAMES` codé en dur (`fetch_kitti08.py:28`) ; aucun label ; le script **n'exige pas** de sha256 attendu, il les consigne (l. 146-165) |
| Préparation u16 2 cm, repère scan 0, préfixes (14 sept.) | **testé** (18 contrôles, normal et −O) | `PREPARATION_CHECKS.json` | historique ; scans 100/200 dans le repère du scan 0, d'où 119 995 / 120 759 sites contre 119 942 / 120 725 |
| Préparation spatiale 2 cm, repère propre, 7 morceaux | **testé** + **mesuré** | 11 tests normal et −O, 11 commandes, 78 artefacts (`lidar_spatial_20260921/README.md:8-11, 39-54`) ; porte dans CTest | profil u16 historique |
| Préparation float32 sans perte / grille 1 mm | **testé** | R2 : 15 tests, 3 923 requêtes Fraction, 49 contrôles natifs, 42 nuages (`float32_precision_20260921/README.md:34-48`) | préparation Python hors chrono ; aucune fusion 1 mm sur ces trames |
| Masque sans sol Patchwork++ 3e6903a1 | **testé** (partition, transport, déterminisme) ; qualité sémantique **manquante** | Release 44 et SAN 17 commandes, 20 tests, 3 trames × 3 états neufs identiques (`lidar_ground_20260921/README.md:9-44`) ; `mhgp8_lidar_ground_test` dans CTest ; `--describe` seulement si `MHGP8_PATCHWORK_SOURCE_DIR` est fourni (`CMakeLists.txt:130-165, 483`) | paramètres KITTI fixes (`patchwork_ground_probe.cpp:45`) ; aucun label lu ; lecteurs LIVE dépendants de `build/` |
| Nuages sans sol u16 2 cm | **mesuré** (préparation) | `lidar_ground_u16_20260921/MANIFEST.json`, préparateur `3e9000b9…` identique à 12294241 | reçu minimal (README + MANIFEST, pas de `COMPLETION.json` ni de relecture −O capturée) ; payloads non versionnés, régénérables depuis les `.bin` locaux |
| Nuages sans sol 1 mm (u32le, 18 bits) | **mesuré** (préparation) | `lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/` | ces payloads **versionnés** sont devenus l'entrée du développement (`run_ground_baseline.py:29, 103-121`) et du workflow Actions (`run.py:13-14`) |
| Flux q3/q4 u16 sur trames brutes entières, G4 W48 | **mesuré** (une observation par cas) | `q34_spatial_20260921/gcp_r1/READBACK.json` : `validated_complete`, 4 portes natives, sources fermées | moteur 34R2 (avant l'atlas q3 et la file de plages) ; K5/s8 seulement ; pas de K10, s10/s12 ni sans sol ; 1,93 à 11,13 CPU logiques occupés sur 48 |
| Flux q3/q4 sans sol 2 cm, local W1/W8 | **mesuré** (hôte partagé, charges déclarées) | `ground_baseline_20260921`, `ground_phase1_20260921` | trois lignes de la scène 0 « non vérifiables », reprises au calme dans `BASELINE.only.json` ; référence de la scène 0 sous charge 3,1 à 5,2 |
| Flux q3/q4 sans sol 1 mm | [NON COMMIS, NON SUIVI] **mesuré** sur une ligne | `u18_resume_20260922/ground_1mm_first/BASELINE.only.json` | arbre sale au lancement (`pins.worktree_status`), une répétition, statut `partial`, zéro paire W1/W8, zéro référence |
| Protocole G4 v8 (`cpu_probe_*`, `q34_spatial_*`) | **testé** localement (9 tests normal et −O, faux gcloud) et **mesuré** en réel (4 sessions TERMINATED) | `gcp-migration/receipts/q34_spatial_20260921/local_zzork064/` ; `ARCHIVE.json` et `READBACK.json` | figé sur l'autorité native à 216 hashes du 21 septembre ; inexécutable tel quel au HEAD (§ 5, R3) |
| Tour FULL, GPU, contrat 1 s / 100 ms | **manquant** | `GPU_executed=false`, `FULL_executed=false`, `full_contract_qualified=false` dans tous les reçus G4 | aucun aval (catalogue, intérieurs, fold) en v8 (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56-57`) |
| K10 sur trame brute entière | **manquant** | aucun reçu, ni local ni G4 : les seules lignes à 119 142 / 119 942 / 120 725 sites sont K5 (`GCP_PLAN.json`, `performance/spatial_9kscvyt0/MANIFEST.json`) | le contrat principal vise pourtant K=1..10 sur la trame brute |
| Évaluation sémantique du retrait du sol | **proposé** | `LIDAR_SANS_SOL_PROTOCOLE_20260921.md:146-160` (classes 40/44/48/49/60/72) | aucun label téléchargé (`labels_downloaded=false`) |
| GroundGrid, LineFit, séquence causale | **proposé** | `LIDAR_SANS_SOL_PROTOCOLE_20260921.md:22-30, 182-197` | jamais implémenté |
| Licences des données KITTI | **manquant** (nom et conditions) | section Licences racine (`README.md:94-96`) et `CLAUDE.md` muettes ; `audits/lidar08_20260914/README.md:77-78` dit seulement « leur licence est celle du fournisseur, distincte du code du dépôt » ; seule mention nommée : `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:104-106` (« CC BY-NC-SA », sans version) | question Q3 (l. 224-227) sans réponse dans le dépôt, ni commise ni non commise |

## 4. Chiffres clés

| Grandeur | Valeur | Source épinglée | Réserve |
|---|---|---|---|
| Retours bruts 08/000000, 000100, 000200 | 123 389 / 124 479 / 125 526 | `lidar_ground_20260921/README.md:49-51` ; tailles des `.bin` = 16 × retours | trois trames d'une seule séquence |
| Sites u16 2 cm, repère propre | 119 142 / 119 942 / 120 725 | `lidar_spatial_20260921/README.md:22-24` | les séries « 100/200 » du 14 septembre (repère du scan 0) valent 119 995 / 120 759 (`audits/lidar08_20260914/README.md:87-90`) |
| Retours prédits sol par Patchwork++ | 83 504 / 88 928 / 79 681 (67,7 % / 71,4 % / 63,5 %) | `lidar_ground_20260921/README.md:49-51` ; `lidar_ground_u16_20260921/MANIFEST.json` (`returns_ground`) | décision du segmentateur, pas vérité terrain |
| Indécis conservés (tous hors portée) | 41 / 32 / 34 | `lidar_ground_20260921/README.md:49-54` | — |
| Sites retenus float32 et 1 mm | 39 885 / 35 551 / 45 845 | idem, l. 47-51 | aucun doublon XYZ ni fusion 1 mm sur ces trames (l. 59-60) |
| Sites retenus u16 2 cm | 39 815 / 35 491 / 45 114 ; sites mixtes 0 / 1 / 1 | `lidar_ground_u16_20260921/README.md:28-30` | 70 / 60 / 731 fusions à 2 cm parmi les retours conservés |
| Maxima encodés à 1 mm (x, y, z) | (158 607, 158 284, 30 595), (159 495, 155 301, 9 016), (159 832, 95 896, 16 573) | `float32_precision_20260921/README.md:58-59` | tient sous 262 143 ; un capteur de plus longue portée peut le dépasser |
| Segmentateur Patchwork++, médiane mono | 21,186 / 21,141 / 21,271 ms | `lidar_ground_20260921/README.md:81-85` | hôte partagé, trois répétitions |
| Lecture brute → masque fermé, médiane | 30,358 / 29,843 / 29,927 ms | idem | exclut la préparation Python (l. 87-93) |
| Préparation Python u16 sans sol, trois scènes | 6,687 s | `lidar_ground_u16_20260921/MANIFEST.json`, champ `seconds` | voie hors chrono, non industrielle |
| G4, pipeline q3/q4 K5/s8/W48 | quart 29 128 sites 45,594 s ; trames 165,214 / 34,319 / 505,479 s | `q34_spatial_20260921/README.md:97-102` ; `READBACK.json` (`timings_ms.pipeline_including_shared_preparation`) | une observation par cas ; flux de candidats, pas FULL ; moteur 34R2 |
| G4, CPU logiques occupés en moyenne | 3,49 / 4,19 / 11,13 / 1,93 sur 48 | `READBACK.json`, `gnu_time.average_busy_logical_CPUs` | processus entier ; 48 CPU logiques = 24 cœurs × 2 SMT (README l. 91-92) |
| G4, q3 / q4 émis sur trames entières | 1 252 577 / 190 405 ; 1 212 999 / 164 753 ; 1 369 563 / 209 223 | `READBACK.json` (`row.output`) | identiques aux sorties locales W4 sur la trame 0 |
| G4, boules q3 construites | 244,805 / 122,737 / 273,138 M | `q34_spatial_20260921/README.md:108-110` | travail avant rejet ≈ ×195 les émissions q3 (trame 0) ; ×101 et ×199 pour les trames 100 et 200 |
| G4 contre local, même trame 0 K5, même moteur, mêmes sorties | CPU 691,65 s (G4 W48) contre 1 417,52 s (local W4), rapport 0,49 ; mur 165,2 contre 383,3 s | `READBACK.json` ; `performance/spatial_9kscvyt0/record_0006.json` (`child_cpu_seconds`, `collector_wall_seconds`) | une seule paire ; CPU et hôtes différents ; non transférable par règle |
| Croissance défavorable publiée, trame 0 entière → moitié x+ | bornes q3 ×5,576 (exposant 2,502), blocs q4 ×4,962 (2,332), tests points q4 ×4,004 (2,020) | `q34_spatial_20260921/README.md:65-73` | diagnostic d'une scène, local W4, pas une loi |
| G4 R3, préfixes du fichier 8k du scan 0 | 1k W1 3,849 s ; 1k W48 0,863 s ; 2k 8,470 s ; 4k 59,274 s ; 8k 614,744 s | `lidar_global_20260921/README.md:129-135` | tests ponctuels q3 ×10,765 de 4k à 8k (l. 139-140) : croissance non sous-quadratique observée |
| Durée des sessions G4 (démarrage gardé → arrêt certifié) | R1 3,94 min ; R2 9,80 min ; R3 15,03 min ; q34 16,67 min | `guarded_{start,stop}.command.json` des archives hôte (recalcul) | aucun coût monétaire déduit |
| Budget utile consommé par le worker q34 | 781,9 s sur 900 s : compilation et édition de liens 25,9 s de mur (11:55:15,9 → 11:55:41,8), portes 2,2 s, sondes 750,7 s | `receipt.json` du worker (`elapsed_before_closure_seconds` et commandes) | réserve de 118 s sur le budget utile ; marge de fermeture **distincte** de 300 s imposée par le worker (`q34_spatial_worker_v8.py:278`, `guards.closing_margin_seconds=300`) |
| Sans sol 2 cm, référence scène 0 : K5 W1 / K5 W8 / K10 W8 | 889,5 / 298,0 / 911,1 s de mur | `ground_baseline_20260921/README.md:24-26` | charge avant ligne 4,2 / 3,1 / 5,2 sur 8 vCPU |
| Sans sol 2 cm après phases 1–2, scène 0 au calme | K5 W1 453,3 s ; K5 W8 108,0 s (686 %) ; K10 W8 323,0 s | `ground_phase1_20260921/README.md:31-33` | une répétition ; charge < 3. Le rapport CPU ×1,73 à ×1,96 est plus robuste que le rapport mur ×2,76 à ×2,82, qui compare une référence chargée à une reprise calme |
| Sans sol 2 cm après phases 1–2, scène 2 K10 W8 | 824,1 s de mur ; 3 811,0 CPU·s | idem, l. 39 | chevauchement déclaré avec la référence (charge jusqu'à 16, l. 55-57) |
| Écart au contrat, scène 0 K10 | « ×46 de travail en moins à parallélisme parfait » | `ground_phase1_20260921/README.md:46-49` | 2 212,1 / 48 ; ordre de grandeur, pas une prédiction |
| Sans sol 1 mm, 08/000000 K5/s8/W8 | 104,63 s de mur ; 812,82 CPU·s ; 691 284 q3 ; 158 496 q4 ; RSS 15 124 KiB | [NON COMMIS, NON SUIVI] `ground_1mm_first/only_time_01_s00_k5_w8.txt`, `BASELINE.only.json` | arbre sale, statut `partial`, une ligne, saturation désactivée |
| Identité u16 après port 18 bits | six lignes W8 : `logical_sha256` et sorties égaux à la référence (recontrôlé) ; scène 2 K10 579,82 s | [NON COMMIS] `ground_18bits_20260922/u16_identity/BASELINE.only.json` | charge avant ligne 8,4 à 10,0 sur 8 vCPU ; arbre sale |
| Charges binaires dérivées de KITTI versionnées | **114,08 Mo** : float32_precision 77,67 ; lidar_ground 24,12 ; lidar_spatial 12,29 | recomptage `git ls-tree` à 12294241, extensions `.f32le`, `.u32le`, `.u16le`, `.u8` | l'audit du 21 septembre annonçait « 111 Mo » ; les 0,17 Mo de `.bin` sous `lidar_ground_20260921` sont des fixtures synthétiques ; hors archives compressées |
| Scans KITTI bruts redistribués | 3 `RAW.bin` (1 974 224 / 1 991 664 / 2 008 416 octets) dans `q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz` (11 362 680 octets, sha256 `6a1e4027…`), recopié à l'identique dans les deux `host_evidence.tar.gz` | empreintes des `RAW.bin` = `FETCH_MANIFEST.json` | le snapshot porte 58 membres `data/` (18 371 930 octets décompressés) : les bruts plus les nuages 2 cm des 7 morceaux et leurs correspondances ; introduit par 70de84f2 |
| Poids total versionné de `morsehgp3D_v8/` | 1 024 211 023 octets, 19 281 fichiers ; dont `receipts/` 979 311 698 octets (95,6 %), 16 016 fichiers | recomptage `git ls-tree` | `.git` local : pack 192,80 Mio |
| Autorité native épinglée par le worker G4 | 216 sources ; **87 diffèrent** à 12294241 ; une seule source moteur nouvelle hors inventaire (`src/core/fixed_signed.hpp`, incluse seulement par du float32) | recomptage contre `smoke_ewedfs4y/MANIFEST.json` (`b4682fd3…`) | protocole à réautoriser |
| Fetch KITTI 08 | 19 378 167 octets, 37 requêtes Range, 9 fichiers | `FETCH_MANIFEST.json` | — |

## 5. Défauts, risques et dettes

### Haute gravité

**R1 — Données KITTI brutes et dérivées redistribuées dans Git, en contradiction avec leurs propres reçus.** Le fetch déclare `raw_data_git_tracked: false` (`audits/lidar08_20260914/FETCH_MANIFEST.json:29`) et le README promet des données « exclues de Git » (`audits/lidar08_20260914/README.md:77-78`). Or les trois scans bruts utilisés sont versionnés depuis 70de84f2 dans `receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz`, et le même snapshot est imbriqué dans `gcp_r1/` et `gcp_r1_optimized/host_evidence.tar.gz`. Les trois empreintes coïncident avec celles du fetch (vérifié en flux). Le constructeur du paquet ajoute délibérément `RAW.bin` (`gcp-migration/q34_spatial_snapshot_v8.py:54`), parce que le worker reconstruit les préparations depuis le brut (`q34_spatial_worker_v8.py:207-209`). Le snapshot porte aussi les nuages 2 cm des sept morceaux par scène et leurs correspondances.

S'y ajoutent **114,08 Mo** de coordonnées dérivées hors archives : float32 exactes de chaque retour, grilles, masques. S'y ajoutent aussi les préfixes quantifiés du scan 0 imbriqués dans les archives `lidar_global/gcp_r*` (`scan0_n8000/16000/32000/50000.u16le` dans R1 et R2, `scan0_n8000.u16le` dans R3). Enfin, le workflow Actions recopie les `scene_XX.u32le` dans un artefact conservé 14 jours (`.github/workflows/morsehgp3d-v8-lidar-audit.yml:75, 85`).

Le retrait au HEAD ne suffit pas : l'historique contient les mêmes fichiers. `git log --all` compte 405 fichiers `.u16le`, `.u32le` et `.f32le` ajoutés, aucun supprimé, les 405 étant présents au HEAD ; une trentaine sont de petites fixtures de préflight. La licence n'est nommée qu'à `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:106` (« CC BY-NC-SA », sans version). La question Q3 (l. 224-227) n'a reçu aucune réponse dans le dépôt, ni commise ni non commise.

Connaissance externe, non vérifiée dans cette session : KITTI est publié sous CC BY-NC-SA 3.0 et les labels SemanticKITTI sous CC BY-NC-SA 4.0. Seuls les scans KITTI odometry sont concernés, puisqu'aucun label n'a été téléchargé.

**R2 — Dépendance de l'outillage courant aux dérivés versionnés.** `run_ground_baseline.py:29, 103-121` lit les entrées 1 mm dans `receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`. Le workflow Actions de l'auditeur les lit aussi, via `audits/lidar_rectangles_20260922/run.py:13-14` qui importe `inputs_1mm`. La mesure 1 mm [NON COMMIS] passe ce même chemin à la sonde. Sortir ces octets de Git casse ces trois chemins tant qu'une régénération depuis les `.bin` n'est pas câblée. À l'inverse, les entrées u16 2 cm sans sol ne sont pas versionnées (`lidar_ground_u16_20260921/README.md:34-37`). Deux politiques contradictoires coexistent donc.

**R3 — Le protocole G4 v8 ne peut pas exécuter le moteur actuel.** Les blocages vérifiés sont au nombre de quatre.

- Autorité native : le worker exige que les 216 empreintes épinglées soient égales à celles du paquet (`q34_spatial_worker_v8.py:53-55, 113, 158-165`) ; or 87 d'entre elles ont changé à 12294241.
- Constructeur de paquet : il revérifie chaque source contre ces pins (`q34_spatial_snapshot_v8.py:34-37`) et part du build local non versionné `build/v8_q4_seed_cells_r2_20260921` (l. 21, 28, 85).
- Entrée : le plan n'accepte que des fichiers `.u16le` à enregistrements de 6 octets (l. 125, 137-138, 154), alors que l'entrée 1 mm est un `.u32le` de 12 octets par site.
- Commande : `validate_probe` exige exactement les 14 arguments sans `atlas` (l. 233-236, 260-263). La sonde du HEAD accepte encore cette commande (`wspd_q34_probe.cpp:200-210`, `atlas` optionnel, défaut `no-atlas`, schéma v4). Le protocole produirait donc une mesure, mais **sans** les graines q3 certifiées par l'atlas acquises en phases 1–2.

Contrairement au rapport d'origine, la « recette à 24 unités » **n'est pas** un blocage. `mhgp8_p0` compte toujours exactement ces 24 unités (`CMakeLists.txt:32-42`). Les six unités supplémentaires de `src/` sont du float32 (`mhgp8_f32`), hors de l'inventaire explicite de l'autorité. Par lecture du code, et sans exécution, le constructeur de paquet et l'autotest local échoueraient au HEAD sur les pins. Le plan du journal (`JOURNAL_DEVELOPPEMENT_20260921.md:71-99`) le reconnaît mais n'est pas implémenté ; il reste écrit pour des morceaux u16.

**R4 — Aucun résultat G4 ne porte sur le régime prioritaire.** Les quatre sessions portent sur des nuages bruts u16 à 2 cm, K5/s8, avec le moteur 34R2, pour un flux de candidats. Rien n'a été mesuré sur G4 sans sol, à 1 mm, en K10 ni en s10/s12 (`q34_spatial_20260921/README.md:119-121`). Le schéma de plan v1 accepte pourtant K ∈ {5, 10} et s ∈ {8, 10, 12} (`q34_spatial_worker_v8.py:128-129`) : ce sont les choix de `GCP_PLAN.json`, pas le protocole, qui ont limité la session. Aucune mesure K10 sur trame brute entière n'existe, ni locale ni sur G4. L'occupation de 1,93 à 11,13 CPU sur 48 date d'avant la file de plages (5224ff4e, 22:36 UTC le 21) ; son effet sur G4 est inconnu.

**R5 — Le contrat lui-même n'est pas mesurable en v8.** La tour K=1..10 n'existe pas : pas de catalogue, d'intérieurs ni de fold (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56-57`). Tous les reçus G4 portent `full_contract_qualified=false`. Aucun chrono ne couvre la préparation depuis l'entrée déclarée, pourtant exigée par `CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:15-22`. La déduplication et la grille sont écrites en Python (6,687 s pour trois scènes), et le masque est produit dans un processus séparé.

### Gravité moyenne

**R6 — Une seule séquence, sans vérité terrain.** Trois trames de la séquence 08. Le contrat exige plusieurs scènes choisies avant les chronos, et un élargissement de la diversité avant qualification (`CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:24-30`). Le fetcher est codé en dur sur 08 (`fetch_kitti08.py:28`) et ne compare les octets à aucun sha256 attendu : il vérifie ETag, `Content-Range` et CRC32, puis consigne le sha256. Aucun label n'a été téléchargé : la qualité du retrait n'est pas évaluée. La « hauteur médiane du sol z = −1,734 m » (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:67`) n'a pas de reçu : **non vérifiable**. De même, « par site, le nuage sans sol coûte 2,6 à 3,9 fois plus qu'avec le sol » (l. 51-52) n'est épinglé nulle part. Aucune campagne ne mesure la trame brute et la trame sans sol au même commit.

**R7 — Paramètres Patchwork++ propres à KITTI.** La hauteur capteur 1,723 m et la portée ]2,7 ; 80] m sont des valeurs par défaut de l'adaptateur (`patchwork_ground_probe.cpp:45` ; `PILOTE_LIDAR_SANS_SOL_20260921.md:24-27`). Le filtre de portée utilise une distance double approchée, sans portabilité inter-libm revendiquée (`lidar_ground_20260921/README.md:100-103`). Un autre capteur exige des paramètres déclarés avant les chronos.

**R8 — Reçus « LIVE » non autonomes.** Les lecteurs de `lidar_ground_20260921` dépendent des builds `build/v8_ground_patchwork_*` et des bruts non versionnés (`lidar_ground_20260921/README.md:131-134`). Le paquet G4 dépend de `build/v8_q4_seed_cells_r2_20260921`. Les reçus `u18_resume` [NON COMMIS] déclarent la même dépendance. Sur un autre conteneur, ces reçus ne se rejouent pas.

L'incident `ctest -N` du 21 septembre (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:108-119`) montre leur fragilité. Il a créé ou réécrit un `LastTest.log` dans quatre builds épinglés, sans changer aucun binaire. Il a fait échouer deux lectures Release du pilote sans sol jusqu'à suppression du répertoire `Testing/`. Il a perdu le journal CTest brut d'origine de trois builds.

**R9 — Chaîne d'outils de la VM différente du local.** Sur G4 : GCC 11.4 et Boost système d'Ubuntu 22.04. En local : GCC 13.3 dans ce codespace et sur Actions, GCC 14.2 pour les appelants locaux de l'auditeur (`audits/lidar_rectangles_20260922/README.md:21`), Clang 18.1.3 pour les sanitizers. R1 a échoué sur un `-Werror=maybe-uninitialized` propre à GCC 11.4 ; R2 a bloqué sur `boost::rational == 0` (`gcp-migration/README_CPU_PROBE_V8.md:120-151`). Le worker refuse toute installation (`q34_spatial_worker_v8.py:326-329`). Il compile en `-O3 -Werror` sans `-ffp-contract=off -fno-fast-math -frounding-math` (l. 336-337).

CMake impose déjà ces options à `mhgp8_f32` (`CMakeLists.txt:57-66`) et, sauf `-frounding-math`, à la sonde Patchwork++ (l. 153-154). Seules les lignes `g++` écrites à la main du worker en sont privées. Tant que le moteur entier reste sans flottant, c'est sans effet ; ce sera bloquant pour tout filtre flottant certifié compilé sur la VM.

**R10 — Budget de 900 s serré, mais l'infaisabilité de K10 n'est pas établie.** La session q34 a consommé 781,9 s sur 900 s pour K5 seul, dont 750,7 s de sondes. La scène 2 sans sol K10 coûte 3 811 CPU·s en local, sous chevauchement de charge (`ground_phase1_20260921/README.md:39`). Sur la seule paire comparable (trame 0 brute, K5, même moteur, mêmes sorties), G4 a consommé 0,49 fois les CPU·s locaux : 691,65 contre 1 417,52. Les CPU·s locaux ne se transfèrent donc pas tels quels. Une scène K10 peut tenir dans la fenêtre si l'occupation G4 dépasse quelques CPU en moyenne, ce qui reste à mesurer.

**R11 — Contamination et perte de mesures.** Une sentinelle `baseline.done` périmée a lancé la campagne appariée en concurrence avec la référence. Les murs de la scène 2 de la référence sont contaminés, et les fichiers bruts de trois lignes de la scène 0 de la campagne appariée ont été écrasés : ces valeurs sont **non vérifiables** (`ground_phase1_20260921/README.md:19-27, 53-58` ; `ground_baseline_20260921/README.md:39-45`). La référence de la scène 0 elle-même a couru sous charge 3,1 à 5,2. Les six lignes d'identité 18 bits [NON COMMIS] ont couru avec une charge de 8,4 à 10,0 sur 8 vCPU.

**R12 — Preuves de l'auditeur hors dépôt.** Les mesures élargies et la cascade du 22 septembre renvoient à des archives « jointes à la conversation » et à un artefact Actions de 14 jours (`audits/lidar_rectangles_20260922/README.md:17-19, 105`). Seul `audits/cascade_rectangles_20260922/RESULTS.json` est versionné comme résultat. Passé l'expiration, ces facteurs deviennent non vérifiables.

**R13 — Normes contradictoires sur le périmètre du contrat.** Le rapport d'origine disait le régime sans sol absent de la section normative d'`AGENTS.md` ; c'est inexact. `AGENTS.md:29-39` en fait « aussi un régime prioritaire », mais précise qu'« une réussite sans sol ne remplace pas le contrat principal sur la trame brute entière » (l. 37-38). À l'inverse, `PASSATION.md:15-18`, `audits/ETAT_COURANT.md:11` et `ELARGISSEMENT_18_BITS_20260922.md:3-6` poursuivent les contrats temps sur le sans-sol de 30 000 à 60 000 sites, avec le moteur u18. Côté profil, `AGENTS.md:14-17` garde le float32 par défaut et la grille 1 mm en option. Le diff [NON COMMIS] d'`AGENTS.md` précise que le port 18 bits « sert la grille OPTIONNELLE 1 mm ; il ne remplace ni le profil float32 par défaut ni le contrat brut entier ». Le développement ne vise pourtant plus que le 1 mm. Il faut trancher la cible en v9.

### Gravité basse

**R14 — Documentation de licence incomplète.** Patchwork++ (BSD-2) est bien redistribué avec son `LICENSE` d'empreinte épinglée (`b2ae335f…`, `build_patchwork_ground.py:32`), en deux copies sous `receipts/lidar_ground_20260921/builds/*/third_party/`, avec les cinq fichiers source amont. La clause de notice est donc respectée, et les licences Patchwork++, GroundGrid et LineFit sont décrites dans `LIDAR_SANS_SOL_PROTOCOLE_20260921.md:81-96`. Mais la section Licences racine ne les mentionne pas, et `CMakeLists.txt:130-131` affirme que ces sources « ne sont pas dans ce dépôt ».

**R15 — Identifiants personnels et d'infrastructure dans les archives hôte versionnées.** `q34_spatial_v8_host/oslogin_add.stdout` figure dans les deux `host_evidence.tar.gz` q34, et un fichier `oslogin_add.*` dans chacune des trois captures `lidar_global`. Il contient le profil OS Login : identifiant numérique du compte Google, e-mail, uid POSIX, projet GCP et cinq clés SSH publiques. Aucune clé privée : `private_key_included=false`, conforme au README (l. 140-146). Ce ne sont pas des secrets, et l'e-mail figure déjà dans d'autres fichiers versionnés (`docs/validation/phase15_*_time.txt`). Ce sont cependant des données personnelles inutiles à la preuve.

**R16 — Dérive documentaire.** `CLAUDE.md` affirme que la CI GitHub ne construit ni la v5, ni la v4, ni la v3, et reste muet sur la v7 et la v8. Or `morsehgp3d-v7.yml` existe, et `morsehgp3d-v8-lidar-audit.yml` construit `mhgp8_p0` sur push de chemins ciblés. Les séries « scan 100/200 » désignent deux jeux distincts selon la tranche (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:101-103`). `AUDIT_REPRISE:96-98` (options flottantes « absentes de CMake ») est périmé depuis 92d74c13.

## 6. Questions ouvertes

1. **Licence (bloquant)** : faut-il sortir de Git les octets KITTI bruts et dérivés et réécrire l'historique (hors du pouvoir d'un agent sans accord explicite), ou documenter une exception non commerciale pour `morsehgp3D_v8/receipts/` à la manière de `HGP-old/` ? Le dépôt est-il public ? Les artefacts Actions ont-ils été consultés hors du projet ?
2. **Cible du contrat v9** : trame brute entière (environ 120 000 sites, `AGENTS.md:7-12, 37-38`) ou trame sans sol de 30 000 à 60 000 sites (`PASSATION.md:15-18`) ? Et quel profil : float32 original, grille 1 mm u18, ou les deux mesurés séparément ?
3. Quelles séquences et trames SemanticKITTI, choisies **avant** les chronos, pour la qualification multi-scènes ? Télécharge-t-on les labels pour évaluer le masque, avec quelle politique de stockage ?
4. Le protocole sans sol (`LIDAR_SANS_SOL_PROTOCOLE_20260921.md:27-30`) fige méthode et paramètres **après** l'évaluation de segmentation et **avant** les chronos de qualification. `AGENTS.md:38-39` dit que les labels ne bloquent pas les chronos sur masque figé. Les chronos de développement restent-ils libres, et seuls ceux de qualification attendent-ils les labels ?
5. La préparation (grille, déduplication, masque) entre-t-elle dans le chrono du contrat ? Si oui, il faut une voie native mesurée.
6. Reprend-on la VM `ehgp-v7-4fa0e0789a7d5bb06b787d35`, codée en dur dans le worker (`q34_spatial_worker_v8.py:29-30, 296`), ou en crée-t-on une v9 par les scripts gardés ? Épingle-t-on une image ou un conteneur avec GCC et Boost identiques au local ?
7. Le budget utile de 900 s reste-t-il la règle, ou le plan v9 est-il découpé en plusieurs sessions (une seule SPOT active à la fois) ?

## 7. À porter en v9, et à ne pas reprendre

### À porter

| Quoi | Où (source v8) | Pin | Pourquoi |
|---|---|---|---|
| Fetch borné : Range, ETag, `Content-Range`, CRC32, fichiers existants relus et vérifiés au lieu d'être écrasés | `morsehgp3D_v8/audits/lidar08_20260914/fetch_kitti08.py` | script sha256 `e32fe83a…` (`FETCH_MANIFEST.json`) | reproductibilité sans versionner les données ; à paramétrer (séquences, trames, labels) et à doter d'une **vérification contre sha256 attendus** (ceux du `FETCH_MANIFEST.json` v8) |
| Contrat de masque u8 : 0 indécis, 1 sol, 2 non-sol ; seul 1 retiré ; indécis conservés ; IDs retirés publiés | `bench/patchwork_ground_probe.cpp`, `bench/prepare_lidar_ground.py` | Patchwork++ `3e6903a1…`, empreintes amont `build_patchwork_ground.py:31-38` | séparation nette entre segmentation approximative et exactitude HGP |
| Règle « site conservé si un de ses retours l'est », correspondances complètes, 7 morceaux même vides | `prepare_lidar_ground.py`, `prepare_lidar_ground_u16.py` | `3e9000b9…` (u16) | transport prouvé par reconstruction depuis le brut |
| Protocole spatial scène / moitiés / quarts, repère capteur, rapports r réels, relations défavorables publiées | `docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md` | 759ce2b0 | remplace les préfixes hachés |
| Grille 1 mm à translation commune calculée sur la trame brute entière | `prepare_lidar_precision.py`, reçu `float32_precision_20260921/release_r2` | R2 `precision_a1drpf9i` | l'origine ne dépend ni du masque ni du morceau |
| Portes LiDAR Python dans CTest, normal et `-O`, label `lidar` | `CMakeLists.txt:468-484, 550` | 92d74c13 | les préparations restent sous porte automatique |
| Primitives de cycle de vie G4 : contrôle `TERMINATED` préalable, SPOT/STOP/3600 s, garde invité 30 min, génération jamais devinée, arrêt ciblé dans `finally`, archive hôte sans clé privée, marge de fermeture ≥ 300 s | `gcp-migration/q34_spatial_session_v8.py:43-57, 269-324` (arrêt ciblé l. 298-315) ; `q34_spatial_worker_v8.py:278` ; helpers v7 épinglés (`full_probe_session_v7.py` `177b25a0…`, `full_probe_worker_v7.py` `da967163…`) | contrôleur `d830651e…`, worker `39596c76…` | quatre sessions sur quatre arrêtées et certifiées ; hérité de la v7 par port épinglé |
| Comparaison multi-workers sur le travail logique, seuls les pics de capacité neutralisés | `q34_spatial_worker_v8.py:268-273` | idem | identité des sorties quel que soit W |
| Lecture post hoc normal et −O des captures G4 | `gcp-migration/read_cpu_probe_v8.py`, `bench/read_q34_spatial_gcp.py` | lecteurs `5d6f79e0…` et `0502d42a…` | séparer exécution distante et verdict |
| Porte d'identité u16 → u18 : entrées 2 cm bit-identiques après élargissement | `docs/ELARGISSEMENT_18_BITS_20260922.md:22-26` ; [NON COMMIS] `ground_18bits_20260922/u16_identity` | a74e90f2 + arbre sale | régression de référence de tout élargissement |

### À ne pas reprendre

| Piste | Mesure ou décision qui l'a fermée |
|---|---|
| Préfixes par priorité de hash 8k/16k/32k/50k comme expérience de croissance LiDAR | décision utilisateur du 21 septembre (`PROTOCOLE_LIDAR_SPATIAL_20260921.md:3-4, 21-25`) |
| Assemblage de cinq scans voisins présenté comme captures recalées | déclassé en contrôle secondaire (`audits/lidar08_20260914/README.md:80-85`) |
| Scans 100/200 exprimés dans le repère du scan 0 pour des coupes capteur | les plans ne passeraient plus par leur capteur (`PROTOCOLE_LIDAR_SPATIAL_20260921.md:29-32`) |
| Grille u16 2 cm comme profil de contrat | remplacée par le float32 et la grille 1 mm (`AGENTS.md:14-19`) ; conservée seulement comme régression d'identité |
| Retrait du sol par seuil z constant | rejeté par le protocole (`LIDAR_SANS_SOL_PROTOCOLE_20260921.md:32-36`), jamais mesuré |
| Jobs Coarse indivisibles sur 48 workers | G4 : 4,19 / 11,13 / 1,93 CPU occupés (`q34_spatial_20260921/README.md:100-102, 112-116`) ; R3 : 3,72 puis 3,23 CPU (`lidar_global_20260921/README.md:150-152`) |
| Versionner des paquets ou des archives hôte G4 contenant `RAW.bin` ou des nuages dérivés | contradiction avec `raw_data_git_tracked=false` ; risque de licence R1. Le plan du journal (`JOURNAL_DEVELOPPEMENT_20260921.md:90-93`) retransporterait `RAW.bin` : ne garder dans Git que l'empreinte |
| Archiver `oslogin_add.stdout` brut dans les reçus | données personnelles sans valeur de preuve (R15) |
| Enchaîner des campagnes par fichier sentinelle | incident `baseline.done` périmé (`ground_phase1_20260921/README.md:53-58`) |
| Lancer `ctest`, même `-N`, dans un build épinglé | incident du 21 septembre (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:108-119`) |
| Deux variantes de partition de l'atlas | cache de formes neutre (27,32 s contre 27,32 s) ; passe conjointe des quatre filles +18 % (31,7 s contre 26,8 s) (`JOURNAL_DEVELOPPEMENT_20260921.md:45-58`) |

## 8. Recommandations priorisées pour la v9

1. **P0 — Trancher la licence avant tout nouveau reçu LiDAR.** Poser la question à l'utilisateur : retrait avec réécriture d'historique, ou exception documentée. En v9, ne versionner aucun octet KITTI : seulement des manifestes (sha256, tailles, effectifs, paramètres) et des fixtures synthétiques. Ajouter à la section Licences racine KITTI, SemanticKITTI (versions à vérifier sur les pages officielles) et Patchwork++ BSD-2. Ajouter un contrôle CI qui refuse tout fichier dont le sha256 figure dans un manifeste de données tierces. Ce contrôle doit aussi couvrir les membres des archives `.tar.gz` versionnées.
2. **P0 — Régénérer les entrées au lieu de les lire dans `receipts/`.** Créer un `data/` v9 ignoré par Git, alimenté par un fetcher paramétré et vérifié contre des sha256 attendus (séquences, trames et labels choisis avant les chronos), puis par les préparateurs, avec des manifestes versionnés. `inputs_1mm()` et le workflow Actions doivent consommer ce chemin et échouer explicitement si les données manquent. L'artefact Actions ne doit plus embarquer les nuages.
3. **P0 — Trancher la cible du contrat** : trame brute entière ou trame sans sol, et profil float32 ou 1 mm. Inscrire la décision dans `AGENTS.md` v9, qui doit cesser de contredire la passation (R13).
4. **P1 — Réécrire le protocole G4 v9 à partir des primitives v8.** Il faut :
   - une autorité native produite à un commit v9 propre et épinglée dans le worker ;
   - un constructeur de paquet indépendant de `build/` local ;
   - un plan v2 acceptant `.u32le` 18 bits (FNV sur valeurs entières, `ELARGISSEMENT_18_BITS_20260922.md:51-54`) et les sept morceaux sans sol ;
   - une commande de sonde autorisant le jeton `atlas`, et des validateurs de sonde étendus au schéma v5 (`atlas`, `q3_atlas`, `tasks`, `workers_tasks`, `workers_timing_ms`) sans casser la lecture v4 ;
   - une préparation sans sol relocalisable ;
   - un filtre de l'archive hôte : ni `RAW.bin` ni nuages dans la partie versionnée, empreintes seulement, et pas de `oslogin_add.stdout` brut ;
   - une cible VM paramétrée ;
   - des autotests sous faux gcloud au vert au commit exact.
5. **P1 — Préflight de chaîne d'outils sur la VM** (compilation seule, avant toute mesure) : épingler GCC et Boost de l'image, ou un conteneur, et les enregistrer dans le reçu. Si des filtres flottants certifiés entrent dans le moteur, reprendre dans les commandes du worker les options déjà imposées par CMake à `mhgp8_f32` (`-ffp-contract=off -fno-fast-math -frounding-math`).
6. **P1 — Plan G4 v9 ordonné du moins cher au plus cher.** D'abord les trois scènes (sans sol ou brutes selon la décision 3) en 1 mm K5/W48, puis K10, avec un cas d'identité W1/W48 sur un petit morceau. Garder la marge de fermeture de 300 s imposée par le worker, plus une réserve explicite dans le budget utile. Découper en plusieurs sessions plutôt que dépasser 900 s. Conserver `GPU_executed=false` tant qu'aucun backend GPU n'existe, et certifier `TERMINATED` à chaque session. Mesurer aussi une trame brute entière K10 : aucune n'existe.
7. **P2 — Campagne appariée brute contre sans sol au même commit** (mêmes trames, K5/K10, W1/W8) pour épingler ou réfuter le « ×2,6 à ×3,9 par site » (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:51-52`).
8. **P2 — Voie d'entrée native et chronométrée** : lecture du `.bin`, grille, déduplication, masque et construction du nuage en C++ mesuré, pour que le chrono couvre l'entrée déclarée. Les préparateurs Python restent des oracles de transport.
9. **P2 — Qualifier le masque sur labels dans une campagne séparée** : plusieurs séquences, classes du score fixées d'avance (`LIDAR_SANS_SOL_PROTOCOLE_20260921.md:146-160`), paramètres capteur déclarés, GroundGrid comparé ensuite. Les chronos de développement n'attendent pas ce diagnostic ; ceux de qualification suivent la règle de gel du protocole (question 4).
10. **P2 — Reçus autonomes** : chaque reçu v9 doit se rejouer sans `build/` local, avec sources et commandes de build versionnées, binaires hachés et une capsule minimale sans données tierces.
11. **P3 — Documentation** : `CLAUDE.md` doit citer les workflows v7 et v8 ; supprimer le double sens « scan 100/200 » ; corriger le commentaire `CMakeLists.txt:130-131`.

## 9. Contre-vérification

Toutes les affirmations principales et tous les chiffres du rapport d'origine ont été rouverts à leur source. Les éléments non listés ci-dessous sont **confirmés** tels quels. Voici les corrections et compléments intégrés.

### Corrections

1. **Poids des dérivés KITTI** : 114,25 Mo → **114,08 Mo**. Les 165 794 octets de `.bin` sous `receipts/lidar_ground_20260921` sont des fixtures synthétiques (`plane`, `reflectance`, `empty`, `nonfinite`, etc.), pas des dérivés KITTI. La part `lidar_ground` vaut 24,12 Mo, non 24,29.
2. **Blocage « 24 unités contre 30 `.cpp` »** : réfuté comme blocage. `mhgp8_p0` compte toujours exactement les 24 unités de la recette (`CMakeLists.txt:32-42`). Les six autres sont float32 (`mhgp8_f32`), absentes de l'inventaire explicite de l'autorité. Une seule source moteur est nouvelle hors inventaire, `src/core/fixed_signed.hpp`, et seul du code float32 l'inclut. Les blocages réels sont les 87 pins, le constructeur lié à `build/`, l'entrée `.u16le` et la commande figée.
3. **Commande « à 14 arguments sans atlas »** : ce n'est pas une incompatibilité de la sonde du HEAD. Elle l'accepte (`wspd_q34_probe.cpp:200-210`, `atlas` optionnel, schéma v4). C'est le worker (`validate_probe`, l. 260-263) qui interdit `atlas` et empêcherait de mesurer les gains des phases 1–2.
4. **Licence « n'apparaît que dans un audit »** : à nuancer. `audits/lidar08_20260914/README.md:77-78` dit que « leur licence est celle du fournisseur, distincte du code du dépôt », sans la nommer. `LIDAR_SANS_SOL_PROTOCOLE_20260921.md:81-96` documente les licences Patchwork++, GroundGrid et LineFit. Le nom « CC BY-NC-SA » n'apparaît bien qu'à `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:106`.
5. **Régime sans sol absent d'`AGENTS.md`** : réfuté. `AGENTS.md:29-39` en fait un régime prioritaire. Ce qui manque est la taille de 30 000 à 60 000 sites et le report du contrat sur ce régime. `AGENTS.md:37-38` dit l'inverse : le sans-sol ne remplace pas le contrat brut. D'où R13, relevé en gravité moyenne.
6. **Recommandation « marge de fermeture de 120 s »** : le worker impose déjà `closing_margin_seconds >= 300` (l. 278, `guards.closing_margin_seconds=300`). Les 118 s observées étaient la réserve restante sur le **budget utile**, une grandeur distincte.
7. **Recommandation « imposer les options flottantes dans CMake »** : déjà fait pour `mhgp8_f32` (`CMakeLists.txt:57-66`) et, sauf `-frounding-math`, pour la sonde Patchwork++ (l. 153-154). Il ne manque que le worker et les lanceurs `g++` écrits à la main.
8. **R10 (budget)** : le chiffre local de 3 811 CPU·s n'est pas transférable. Sur la seule paire comparable, G4 consomme 0,49 fois les CPU·s locaux (691,65 contre 1 417,52, sorties identiques). Le risque reste « serré », mais l'infaisabilité n'est pas établie.
9. **Ratio boules q3 / émissions** : « ≈ ×196 » → **×195** (244,805 M / 1,2526 M = 195,4).
10. **Références de lignes** : maxima 1 mm à `float32_precision_20260921/README.md:58-59` (non 48-50) ; identité u16 → u18 à `ELARGISSEMENT_18_BITS_20260922.md:22-26` (non 20-24) ; tableau du 14 septembre à `audits/lidar08_20260914/README.md:87-90`.
11. **Chronologie** : le workflow Actions de l'auditeur est introduit par **7a121e44**, non 562d090c.
12. **Incident `ctest -N`** : « a déjà altéré quatre builds épinglés » est trop fort. Il a créé ou réécrit un `LastTest.log` sans changer de binaire, fait échouer deux lectures Release jusqu'à suppression de `Testing/`, et perdu trois journaux CTest bruts (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:110-119`).
13. **Statut de la ligne 1 mm** : `ground_1mm_first/` est **non suivi** (`??`), pas seulement non commis. `ground_18bits_20260922/u16_identity` est indexé. L'identité des six lignes a été recontrôlée (`logical_sha256` et sorties égaux à la référence). La charge avant ligne va de 8,4 à 10,0.
14. **Chaîne d'outils « 13 ou 14 »** : 13.3 est celle du codespace et d'Actions ; 14.2 est celle des appelants locaux de l'auditeur (`lidar_rectangles_20260922/README.md:21`). Clang 18.1.3 sert aux sanitizers.

### Compléments (omissions du rapport d'origine)

1. Le snapshot G4 versionné porte aussi les nuages 2 cm des sept morceaux et leurs correspondances : 58 membres `data/`, 18 371 930 octets décompressés, dont 5 974 304 de `RAW.bin`.
2. Les archives hôte versionnées contiennent le profil OS Login : e-mail, identifiant de compte, uid et cinq clés SSH publiques (R15).
3. Aucune mesure K10 sur trame brute entière n'existe, ni locale ni sur G4. Les trames brutes n'ont été mesurées qu'avec le moteur 34R2, avant l'atlas q3 et la file de plages.
4. Le schéma de plan G4 v1 accepte déjà K10 et s10/s12 (`q34_spatial_worker_v8.py:128-129`) ; la limitation venait du plan.
5. Le fetcher ne vérifie aucun sha256 attendu : il vérifie ETag, `Content-Range` et CRC32 (`fetch_kitti08.py:146-165`).
6. Le « ×2,6 à ×3,9 par site » du sans-sol (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:51-52`) n'a pas de reçu. Aucune campagne ne compare brut et sans sol au même commit.
7. Les rapports mur des phases 1–2 comparent une référence chargée (3,1 à 5,2) à une reprise calme (< 3). Le rapport CPU (×1,73 à ×1,96) est le chiffre robuste.
8. Tension entre protocole et `AGENTS.md` sur le gel du masque avant les chronos de qualification (question 4).
9. Diagnostics de croissance défavorables publiés sur la trame 0 : exposant 2,502 des bornes q3 de la trame à la moitié x+ (`q34_spatial_20260921/README.md:65-73`).
10. Les portes LiDAR sont dans CTest avec le label `lidar` depuis 92d74c13 (`CMakeLists.txt:468-484, 550`), ce qui rend périmée la remarque « hors CTest » de l'audit du 21 septembre.
11. La VM cible est codée en dur dans le worker (`q34_spatial_worker_v8.py:29-30, 296`) sous un nom v7.
12. Un préflight refusé avant toute action cloud (`GCP_PREFLIGHT_KEY_MODE.json`, clé en mode 0644, `vm_started=false`) complète l'inventaire des tentatives G4 ; ce n'est pas une session.
