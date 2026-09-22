# Lentille 7/12 — Mesures, reçus et écart aux contrats (audit v8 → v9), version contre-vérifiée

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`, `mode=audit_lecture_seule`, `public_status=not_claimed`. GCP non utilisé. Aucune compilation, aucun ctest, aucun benchmark, aucun script du dépôt exécuté : lecture de fichiers, `git log/show/diff`, `sha256sum`, `lscpu` et `python3 -c` sur des JSON.

Ce document reprend le rapport `07_mesures_recus.md` après une contre-vérification adversariale (22 septembre 2026, 20:47 UTC). Chaque chiffre a été rouvert dans sa source citée ; les corrections sont intégrées au texte et récapitulées au § 9. Le rapport d'origine n'est pas modifié.

Convention de statut : **prouvé** (preuve écrite + fixture), **testé** (porte bornée), **mesuré** (reçu épinglé : commit/sources, sha256, sorties brutes), **proposé**, **manquant**, **non vérifiable** (chiffre sans reçu), **inférence** (déduction de cet audit, non mesurée).

## 1. Périmètre lu

### 1.1 Sources

| Source | Nature | Usage |
| --- | --- | --- |
| Worktree détaché = `origin/main` 12294241 | état publié | autorité principale |
| Worktree partagé `/workspaces/E-HGP`, HEAD a74e90f2 | en retard de 13 commits d'audit sur `origin/main` (bf73e194 … 12294241, et non « 7a121e44 … ») | seulement pour ce qui manque à 12294241, étiqueté « non commis » |
| Tranche non commise du constructeur (fichiers indexés ou non suivis, dernière écriture 10:56 UTC le 22 septembre) | `receipts/ground_18bits_20260922/` (13 fichiers indexés), `receipts/u18_resume_20260922/` (3 fichiers indexés, le reste non suivi), `receipts/float32_q3_global_20260921/` (2 fichiers non suivis), `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md`, diff d'`AGENTS.md` | lue, jamais traitée comme publiée |
| Hôte local | `codespaces-0bcb78`, 8 vCPU = 4 cœurs × 2 SMT ; modèle observé aujourd'hui : AMD EPYC 7763 | voir § 5, risque 9 : le modèle a changé depuis le 14 septembre |

### 1.2 Lu intégralement ou par extraction ciblée

- Les 47 `README.md` de `morsehgp3D_v8/receipts/*/` (le 48e répertoire, `audit_v7_20260913/`, n'a pas de README ; ses quatre JSON ont été lus).
- `ANALYSE_CROISSANCE.md` de `q34_indexed_20260921`, `q34_affine_20260921`, `float32_q3_census_20260921` ; `PARTIAL_GLOBAL_8K.md` de `lidar_global_20260921`.
- JSON de mesure lus et recalculés :
  - `ground_baseline_20260921/BASELINE.json`, `ground_phase1_20260921/BASELINE.json` et `BASELINE.only.json`, les 21 `probe_*.json` (compteurs des trois scènes, pas seulement la scène 0) et leurs `workers_timing_ms` ;
  - les sept `record_000*.json` de `q34_spatial_20260921/performance/spatial_9kscvyt0/`, `GROWTH_SCAN0_K5_S8.json` (tous les compteurs des six relations), `gcp_r1/READBACK.json` ;
  - les records LiDAR des tranches 32/33/34 (`lidar_86twby55`, `lidar_gladtapx`, `lidar_kk34w49h`, `lidar_5vvvq0bh`), y compris leurs compteurs de témoins ;
  - `q2_dynamic_front_20260914/campaigns/{lidar_growth,lidar50k_other_scans,lidar50k_repeat}/MEASURES.jsonl` et `lidar_growth/MANIFEST.json` (qui consigne le modèle de CPU).
- Non commis : `ground_18bits_20260922/u16_identity/BASELINE.only.json` et ses six sondes ; `u18_resume_20260922/README.md`, `ground_1mm_first/*.json` (pins complets), les quatre `COMPLETION.json` et les quatre `ctest.json` de qualification, `release_r2/scale_*.json`.
- Documents :
  - `AGENTS.md` l. 1-60 et le diff non commis ;
  - `morsehgp3D_v8/PASSATION.md` l. 1-80 et 165-230 ;
  - `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` l. 1-200 ;
  - `docs/JOURNAL_DEVELOPPEMENT_20260921.md`, publié et diff non commis ;
  - `docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` l. 1-90 ;
  - `audits/CONTRATS_ET_MESURES.md` l. 25-95 ;
  - `audits/SYNTHESE_PRIORITES_LIDAR_20260922.md` l. 1-90, `audits/lidar_rectangles_20260922/README.md` l. 1-80, `audits/cascade_rectangles_20260922/RESULTS.json` ;
  - `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` (non commis) l. 60-126.

### 1.3 Vérifications d'intégrité (toutes concordantes)

- sha256 déclarés dans les README :
  - neuf recalculés par la contre-vérification : `global_vwtz76da.tar.gz` 134f03c5…, `gcp_r3_completed/host_capture.tar.gz` 360313f2…, `q2_joint_20260914/pre_fix_sources.tgz` 7cd2c962…, `lidar_86twby55` MANIFEST 2af1415b… et COMPLETION c26f59df…, `q34_affine_20260921/analyze_growth.py` 052b7d6e…, et les trois `record.py.snapshot` 16e0959b…, d0588d26…, 482a258b… ;
  - les cinq autres des quatorze de l'audit d'origine n'ont pas été rejoués.
- 25 `probe_json_sha256` des cinq reçus sans sol (9+6+3+6+1 lignes) : 25/25 égaux aux fichiers.
- `GROWTH_SCAN0_K5_S8.json` et `GROWTH_SCAN0_K5_S8_OPTIMIZED.json` : sha256 identiques, 93b027d0…, conforme au README.
- Entrées sans sol :
  - `audits/lidar08_20260914/prepared/ground_u16/scene_0{0,1,2}/full.u16le` : b0918930…, 461c34fa…, fb49f73a…, présents dans `lidar_ground_u16_20260921/MANIFEST.json` ;
  - `lidar_ground_20260921/release/ground_fq64xq_6/scene_0{0,1,2}_grid/full.u32le` : 0baa4de1…, ba15adc6…, a4bbc86d….
  - **Précision** : les morceaux u16 ne sont **pas versionnés**. Ils sont couverts par le `.gitignore` de `audits/lidar08_20260914/` (motif `/prepared/`) et régénérables seulement depuis les `.bin` KITTI, eux aussi non versionnés. Les morceaux 1 mm, eux, sont versionnés.
- Identité logique des six lignes u16 à a74e90f2 : `logical_sha256` et sorties égaux ligne à ligne à `ground_phase1_20260921` (6/6).
- Binaires :
  - la sonde de `ground_baseline` (925daeaa…) existe dans `build/v8-dev-bench/`, `build/v8_lens3_moteur_u16/` et `build/v8_q4_seed_cells_r2_20260921/`, octet pour octet. **La base « 92d74c13 » est donc le binaire de la tranche 34** ; ses neuf sources épinglées sont identiques à celles de 70de84f2 ;
  - la sonde de `ground_phase1` (bb7f01fc…) était déposée dans `…/b64b3f68-…/scratchpad/probe_phase1`, le scratchpad de session de cet audit. Elle n'existe plus, ni là ni sous `build/`.
- Immuabilité : 46 des 48 répertoires publiés ne sont touchés que par leur commit d'introduction. `float32_ball` n'a reçu ensuite que 7 lignes de README (9923a6b9). `float32_q3_census` a reçu 3 lignes de README **et 11 lignes d'`ANALYSE_CROISSANCE.md`** (204b0620). Tous ces ajouts sont purement additifs.

### 1.4 Non lu

- Les ~16 000 fichiers de captures un par un : seuls les README, résumés, manifestes, records et sondes cités ont été ouverts.
- `PASSATION.md` l. 81-164 et 231-1464, `docs/PLAN_DE_REFONTE.md`, `docs/FAUSSES_PISTES.md` hors grep, `docs/VERROUS_ARCHITECTURE.md`, `audits/COORDINATION_MORSEHGP3D_V8.md` (racine).
- Les archives `.tar.gz` (seul le manifeste de `global_vwtz76da` a été listé), les profils `gprof`, les sorties CTest XML.
- Les reçus v7 eux-mêmes : seule la synthèse v8 `audits/CONTRATS_ET_MESURES.md` a été lue.
- Les pièces hors dépôt citées par les audits du 22 septembre :
  - archive `MorseHGP_LiDAR_cascade_2026-09-22.zip` ;
  - artefact GitHub Actions 10704262200 (runs 35747981707 et 35748470640).

## 2. Ce qui a été fait (campagnes mesurées, chronologie)

45 commits de `origin/main` touchent `morsehgp3D_v8/receipts/`. 169 commits touchent `morsehgp3D_v8/` sur 2b658cbe^..12294241, qui en compte 201 au total. Le commit 4c3cdb0c, intitulé « audit: measured stateless relay… », introduit aussi les reçus de la tranche 34 : c'est un commit mixte de 305 fichiers et +5,5 M lignes.

| Date | Commits (introduction des reçus) | Objet mesuré | Régime |
| --- | --- | --- | --- |
| 13 sept. | 2b658cbe, 3589a2c9, 8e406f9b, f5430f57, f4815cd4 | audit v7 ; crédits locaux, filtre axial, addition, census q2 sur UN rectangle | synthétique 8k/16k/32k, mono ; hôte EPYC 9V74 |
| 14 sept. | 3c29ea1e, 85015a8c, da366f7f, f7edd646, 39b58f37, e3af11a7, b2106c3c, ba11e3ab, b268cf6f, 4e878754, d09e2207 | bornes préparées, nuage partagé, premier front WSPD, front+census q2, frère, ordre, conjoint, Pool terminal, workers, redistribution, reprise | synthétique + préfixes LiDAR avec sol 8k-50k (scans 0/100/200) ; hôte EPYC 9V74 |
| 15-17 sept. | 897085f8, beee3341, 2741d614, 8d615cfd, 8190e7ab, 3e94c868 | détachement, équipe persistante, plages d'ancres, lots (négatif), surproposition, héritage de témoins | synthétique 8k/16k/32k (+70k frontière) ; modèle de CPU non consigné |
| 20 sept. | 9ae4e28b, 785d0589, 77f659e4, 8d0a0f0f, ddaafdc0, 66b1551f, c051bdb0, 31b0243a, f07fbd8c | tranches 22-30 : famille q4, graines q3/q4, covers, élagage, collectif, carte de centres, Local28, couches duales, fenêtre | UNE arête fournie, synthétique dense |
| 21 sept. | 4dbe3024, d1b4dbc6, 2629a536, 4c3cdb0c, 759ce2b0, 70de84f2 | tranches 31-34 : raccord global q3/q4, témoins indexés, bornes affines, graines×cellules ; partitions spatiales ; G4 trames entières | préfixes LiDAR scan0 64 à 50k, trames entières avec sol, G4 W48 |
| 21 sept. | 36724438, 028a0f1d, 12d885d8, 9923a6b9, a005f8aa, 204b0620, ec2bc503 | briques float32 (précision, index, boules, clés, census q3 d'une arête) ; pilote sans sol Patchwork++ ; nuages sans sol u16 | primitives, trames f32/1 mm, entrées sans sol |
| 22 sept. | 72f125c6 | base sans sol (binaire tranche 34, contexte 92d74c13) et campagne appariée phase 1-2 (contexte 0e2c18ca) | trames sans sol entières 35-45k |
| 22 sept. (audit) | bf73e194 … 12294241 (13 commits) | certificats collectifs, cascade de rectangles, présélection négative, sur l'arbre `src` natif 54a6d581 (= celui de a74e90f2) | trames sans sol 1 mm, **rectangles échantillonnés** ; Actions + hôte Intel Xeon 8370C |
| 22 sept. | non commis (a74e90f2 + tranche en cours) | identité u16 après élargissement 18 bits ; première ligne 1 mm ; quatre qualifications u18 | trames sans sol entières |

Après la reprise du 21 septembre (20:10 UTC), le développeur a livré, sans reçu propre, **quatre tranches moteur** : 748ec082, 02987f18, 0948d2d0 et 5224ff4e. S'y ajoute **l'instrumentation 5fdda963** (`workers_timing_ms`). Le commit 0e2c18ca, lui, ne modifie que le journal : c'est seulement le contexte épinglé de la sonde. Ces tranches ne sont mesurées que par deux sources :

- la campagne appariée `ground_phase1_20260921` ;
- des chronos de journal non épinglés, sur le quart sans sol de 7 067 sites (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:12-18`).

## 3. État des reçus et des composants

### 3.1 Inventaire des 51 répertoires

« HEAD ctx » = commit de contexte inscrit dans les JSON : l'autorité est l'ensemble des sha256 de sources. « Échecs conservés » = captures FAILED ou incidents archivés et non promus. Les 48 premières lignes sont reprises de l'audit d'origine. La contre-vérification a contrôlé les comptes de mesures des lignes 2, 3, 4, 16, 22 et 23, ainsi que les lignes 34-51 ; les autres lignes n'ont pas été rouvertes une à une.

| # | Répertoire | Ce qu'il mesure | HEAD ctx / sonde épinglée | Entrées | Statut | Échecs conservés, contaminations déclarées |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | audit_v7_20260913 | relecture v7, inventaire 26 777 chemins, 0 benchmark | v7 dc57ffd5 | dépôt v7 | passed (publication) | pas de README |
| 2 | p0_local_credits_20260913 | crédits Pool/DualBlocks/Tubes sur un rectangle, 729 mesures | e9e97e64, 7cea0eaf, 27ff2098 | grid/sheet/skew 8k/16k/32k, rails 2718 | 4 campagnes r3 closes | deux passes antérieures + 2 XML en échec ; hôte partagé |
| 3 | shared_axis_20260913 | préparation partagée, filtre axial q2, 594 mesures | 90d22425 | synthétique 8k/16k/32k | completed/passed | préliminaires exclus avec motif |
| 4 | additive_q2_20260913 | addition/intersection q2, 648 mesures | 28bcd9fb | synthétique 8k/16k/32k | passed | hôte partagé |
| 5 | q2_census_20260913 | census q2 individuel/partagé, 204 mesures appariées (+50k) | 256957a5 | synthétique | passed | lecteur corrigé à filiation distincte |
| 6 | q2_prepared_bounds_20260914 | bornes q2 préparées, 124 lignes | 1c523fbe | synthétique 8k/16k/32k | passed | archive BEFORE_LINKER_PROFILE_CHECK |
| 7 | cloud_reuse_20260914 | nuage/index partagés, 66 invocations | 7a56d852 | synthétique 8k/16k/32k | passed | échec LSan/sandbox |
| 8 | wspd_front_20260914 | premier front WSPD, 72 mesures | 795a29dd | 4 familles 8k/16k/32k (LiDAR cité en qualification) | passed | échec environnemental |
| 9 | wspd_q2_census_20260914 | front + census q2 complet, 53 mesures | 1ca8f62d | 4 familles 8k/16k/32k | passed | qualifications concurrentes |
| 10 | q2_sibling_20260914 | certificat frère, 32 mesures | 24a717d9 | 4 familles | passed (r2) | Release 43/44 et LSan FAILED |
| 11 | q2_witness_order_20260914 | ordre des témoins, 56 mesures | a1ee8cb0, 39b58f37 | 4 familles | passed | préflight invalidé |
| 12 | q2_joint_20260914 | census conjoint, 36 mesures historiques | 7e315009 | 4 familles 8k | historique | ASan 46/47 FAILED |
| 13 | q2_joint_r2_20260914 | conjoint r2, 44 mesures | fbbecc01 | 4 familles 8k/16k/32k | passed | aucun |
| 14 | q2_terminal_pool_20260914 | Pool terminal, 80 mesures | 7aa9fccf | 4 familles | passed | aucun |
| 15 | q2_front_workers_20260914 | front+census q2 multi-CPU, 86+48 mesures | 329e5b86 | 4 familles + 50k | passed | différentiel initial FAILED |
| 16 | q2_dynamic_front_20260914 | redistribution du front, 192 mesures | 4c0bfe1e (contexte) ; introduit par 4e878754 | synthétique + LiDAR scans 0/100/200, préfixes 8k-50k | passed | TSan GCC SIGSEGV |
| 17 | q2_census_resume_20260914 | census reprenable, 144+12 mesures | 61b86a54, 7b86e36b | 4 familles | passed | coexistence ASan |
| 18 | q2_census_split_20260915 | détachement de branches, 144+12 | ddd915f4 | 4 familles | passed | `tsan_gva7p_55` FAILED |
| 19 | q2_cooperative_20260915 | équipe persistante, 174 mesures | f7d0b019 | 4 familles | passed | ASan interrompu |
| 20 | q2_anchor_ranges_20260915 | plages d'ancres, 174 mesures | e7b951e8 | 4 familles | passed | chevauchement ASan |
| 21 | q2_singleton_batch_20260915 | lots de singletons (négatif), 172 mesures | fdd5fb81 | 4 familles | passed | 2 captures FAILED |
| 22 | q2_front_proposals_20260917 | fenêtre de propositions 2K/4K, 684 mesures | 8d615cfd | 4 familles 8k/16k/32k | passed | aucun |
| 23 | q2_front_inheritance_20260917 | témoins hérités, 854 mesures | 8190e7ab | 4 familles + uniforme 70k | passed | aucun |
| 24 | reprise_20260920 | audit des preuves 19-21, régression 82 CTests | 3e94c868 | — | passed | préflights distincts |
| 25 | q4_family_20260920 | famille q4 d'une seed, 9 mesures | 3e94c868 | seed + fond 8k/16k/32k | passed | aucun |
| 26 | q34_seed_20260920 | boules canoniques d'une seed, 18 mesures | 9ae4e28b | seed + fonds lointains | passed | aucun |
| 27 | q34_cover_20260920 | covers partagés d'une arête, 20 mesures | 785d0589 | fonds 8k/16k/32k, adversaire 32-256 | passed | aucun |
| 28 | q34_pruning_20260920 | élagage par témoins universels, 96 mesures | 77f659e4 | idem | passed | aucun |
| 29 | q34_collective_20260920 | filtre collectif, 208 + 48 prévisions | 8d0a0f0f | idem | passed | aucun |
| 30 | q4_center_map_20260920 | carte de centres q4, 128 + 48 | 2920b8b5 | idem | passed | aucun |
| 31 | q4_local_20260920 | fragments Local28, 72 mesures | 5ff70645 | dense 8k/16k/32k | passed (r2) | mutant R1 survivant |
| 32 | q4_shallow_20260920 | couches duales, 52 mesures | c051bdb0 | idem | passed | aucun |
| 33 | q4_window_20260920 | fenêtre Window30, 52+52 mesures | 31b0243a | idem | passed (reprise) | régression 90/91 FAILED |
| 34 | lidar_global_20260921 | raccord global q3/q4 : 144 mesures d'arêtes, 48 globales préliminaires (n 64/256), global 8k local, G4 r1/r2/r3 | c5308651 | LiDAR scan0 préfixes 64 à 50k | passed + échecs | `global_tsd9ofnm` FAILED ; G4 r1 et r2 FAILED |
| 35 | q34_indexed_20260921 | tranche 32, 29 grandes + 36 petites | d9251d10 | scan0 préfixes 8k/16k/32k | passed | 2 smokes FAILED |
| 36 | q34_affine_20260921 | tranche 33, 30 grandes mesures | 462c29a1 | scans 0/100/200 préfixes | passed | préflight sanitizer FAILED |
| 37 | q4_seed_cells_20260921 | tranche 34, 30 grandes mesures | 46fac512 ; introduit par 4c3cdb0c | scans 0/100/200 préfixes | passed (R2) | mutant R1 survivant |
| 38 | lidar_spatial_20260921 | partitions spatiales (21 nuages), 11 tests | — | trames brutes 0/100/200 | passed/prepared | aucune mesure moteur |
| 39 | q34_spatial_20260921 | 7 mesures locales scan0 + 4 mesures G4 W48 | 06c31e02 ; sonde 925daeaa… (local) | 7 morceaux scan0 ; trames 0/100/200 | validated_complete | local sur hôte partagé |
| 40 | float32_precision_20260921 | préparation f32/1 mm, primitive q2 | 7e7c6cc4 | 3 trames | passed (R2) | sanitize R1 FAILED |
| 41 | float32_index_20260921 | index f32, 54 constructions | 36724438 | synthétique + 3 trames entières | passed (R2) | sanitize R1 FAILED |
| 42 | float32_ball_20260921 | prédicats boules q3/q4 f32 | 028a0f1d / 12d885d8 | fixtures | passed | lecteurs LIVE inapplicables aux sources courantes |
| 43 | float32_identity_20260921 | clés et événements q4 f32 | 12d885d8 | fixtures | passed | aucun |
| 44 | float32_q3_census_20260921 | census q3 f32 d'une arête, 36 observations | e2b09f94, 74fb0a6a | column/slab 8k/16k/32k | passed | 13 des 36 chronos chevauchent les mutations |
| 45 | lidar_ground_20260921 | masque Patchwork++ 3e6903a1, préparations f32/1 mm (versionnées) | 3e6903a1, f7b220c4 | 3 trames | passed | deux audits en parallèle pendant les chronos |
| 46 | lidar_ground_u16_20260921 | préparation u16 sans sol (21 morceaux **non versionnés**, manifeste seul) | préparateur 3e9000b9… | 3 trames | prepared | aucune mesure |
| 47 | ground_baseline_20260921 | base phase 0, 9 lignes | contexte 92d74c13 ; sonde 925daeaa… (= tranche 34) | 3 trames sans sol u16 | passed | scène 02 contaminée par la campagne suivante |
| 48 | ground_phase1_20260921 | campagne appariée phase 1-2, 6+3 lignes | contexte 0e2c18ca ; sonde bb7f01fc… (perdue) | idem | partial (×2) | 3 lignes non vérifiables ; charge jusqu'à 16 ; ligne K10 W8 « au calme » à charge 9,25 (auto-induite) |
| 49 | ground_18bits_20260922 (**indexé, non commis**) | identité u16 après 18 bits, 6 lignes W8 | a74e90f2 ; sonde e750abfc… | idem | partial | pas de README |
| 50 | u18_resume_20260922 (**3 fichiers indexés, reste non suivi**) | 4 qualifications u18 + première ligne 1 mm | a74e90f2 + index ; sonde 57135518… (= binaire des captures `release` et `release_r2`) | trame 0 sans sol 1 mm | qualifications FAILED ×4 (dont 2 à CTest vert) ; mesure passed/partial | README muet sur `release_r2` et `sanitize_r2` |
| 51 | float32_q3_global_20260921 (**non suivi**) | préflight du brouillon global f32 | — | — | aucune mesure | 2 fichiers (Boost absent), 21 sept. 20:21 UTC |

### 3.2 État par composant (point de vue des mesures)

| Composant | Statut | Preuve |
| --- | --- | --- |
| Voie q2 complète (front + census + collecte) | **mesurée** : 4 familles synthétiques 8k/16k/32k, K5/K10, s8/10/12, W1/W4 ; LiDAR avec sol à la tranche 14 seulement (scan0 8k-50k à K5/K10, scans 100/200 à 50k K10) | `q2_front_inheritance_20260917/README.md:77-128` ; `q2_dynamic_front_20260914/campaigns/{lidar_growth,lidar50k_other_scans,lidar50k_repeat}/MEASURES.jsonl` |
| Voie q2 sur trames sans sol | **manquant** | seuls `ground_*` et `lidar_ground*` référencent `ground_u16` |
| Flux q3/q4 global sur préfixes LiDAR 8k/16k/32k | **mesuré** (travail) aux tranches 32-34 ; temps sous charge, sans CPU·s | `q34_indexed_20260921/ANALYSE_CROISSANCE.md` ; `q4_seed_cells_20260921/README.md:100-122` |
| Flux q3/q4 global, moteur courant à 8k/16k/32k | **manquant** | aucune ligne postérieure à la tranche 34 (4c3cdb0c) à ces tailles |
| Trames entières avec sol, K5 | **mesuré** local W4 (scène 0) et G4 W48 (trois scènes), moteur 34 | `q34_spatial_20260921/README.md:47-121` ; `gcp_r1/READBACK.json` |
| Trames entières avec sol, K10, s10/s12 | **manquant** | `PASSATION.md:201-202` |
| Trames sans sol entières u16, K5/K10, W1/W8 | **mesuré**, local, binaire tranche 34 et binaire 0e2c18ca ; identité a74e90f2 non commise | `ground_baseline_20260921`, `ground_phase1_20260921`, `ground_18bits_20260922` |
| Trames sans sol 1 mm | **mesuré**, une ligne (scène 0, K5, W8), non suivie, binaire d'une qualification en échec | `u18_resume_20260922/ground_1mm_first/BASELINE.only.json` |
| Filtre de rectangles sur trames sans sol 1 mm (sources a74e90f2) | **mesuré par un audit** sur rectangles échantillonnés ; dépend de pièces hors dépôt | `audits/lidar_rectangles_20260922/README.md` ; `audits/cascade_rectangles_20260922/RESULTS.json` |
| Croissance spatiale (trame → moitiés → quarts) | **mesurée** scan0 avec sol, K5, moteur 34 ; **manquante** sans sol, à K10, sur les scènes 100/200 en local et avec le moteur courant | `q34_spatial_20260921/GROWTH_SCAN0_K5_S8.json` |
| Parallélisme G4 | **mesuré** avant la file de tâches (moteur 34) : 1,93 à 11,13 CPU occupés sur 48 | `q34_spatial_20260921/README.md:97-121` |
| Parallélisme local après file de tâches | **mesuré** 684-686 % sur 8 vCPU (scène 0) ; CPU par worker 274,0-279,9 s à K10 | `ground_phase1_20260921/only_probe_02_s00_k10_w8.json` (`workers_timing_ms`, ajouté par 5fdda963) |
| Tour FULL (catalogue, intérieurs, fold, K hiérarchies), q2 dans l'appel q3/q4, GPU | **manquant** | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56-57,69-70` |
| Briques float32 | **testées** ; coûts par primitive **non vérifiables** (aucun reçu ne contient « ×23 à ×70 », « 78 µs » ni « 68 à 185 ms par arête ») | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65` ; grep des `receipts/float32_*` |
| Référence v7 FULL 50k uniforme G4 | **mesurée** en v7 (hors v8) | `audits/CONTRATS_ET_MESURES.md:33-80` ; reçu introduit en v7 par ad7ffd28 |

## 4. Chiffres clés

### 4.1 Voie q2 à 8k/16k/32k (s8, flux complet de supports, intérieurs et coquilles)

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| q2 K10, uniforme, W1, référence | 5,18 / 12,24 / 29,03 s | `q2_front_inheritance_20260917/README.md:79-81` (minimum de captures, 8190e7ab) | synthétique ; hôte partagé |
| Même, fenêtre 2K + héritage | ×0,41 / ×0,40 / ×0,36 de la référence | idem | option explicite, défaut inchangé |
| q2 K10, amas, W1 | 2,76 / 7,71 / 19,80 s ; 2K + héritage ×0,56 / ×0,49 / ×0,44 | idem l. 85-87 | — |
| q2 K5, uniforme, W1 | 1,95 / 4,58 / 9,55 s | idem l. 96-98 | — |
| q2 K10, 32k, W4, 2K + héritage | uniforme 2,74 s ; terrain 0,67 s ; amas 2,26 s ; rangées 0,25 s | idem l. 125-128 | une observation |
| Croissance des visites Z du census, K10, 2K + héritage | uniforme ×2,386 / ×2,224 ; amas ×2,580 / ×2,440 ; rangées ×2,070 / ×2,067 | idem l. 155-158 | constante divisée, exposant inchangé |
| Pool terminal, amas K10 32k, W1 | 184,306 s → 19,180 s (×9,61) ; visites 12,360 G → 648,2 M | `q2_terminal_pool_20260914/README.md:110-119` | famille amas seulement |
| q2 LiDAR scan0 avec sol (préfixes), W4, Coarse, K5 | 0,251 / 0,481 / 0,840 s ; 50k : 1,449 s | `q2_dynamic_front_20260914/campaigns/lidar_growth/MEASURES.jsonl` (`timings.pipeline_wall_ms`) | moteur tranche 14 ; hôte EPYC 9V74 |
| Même, K10 | 0,559 / 1,061 / 1,914 s ; 50k : 3,003 s (`worker_ms_sum` 10,094 s, somme de durées de workers, pas un CPU·s) | idem | répétitions 50k scan0 Coarse : 5,469 / 5,391 s (`lidar50k_repeat`) |
| q2 LiDAR 50k K10 W4, scans 100 / 200, Coarse | 2,878-3,500 s / 3,857-4,440 s | `lidar50k_other_scans/MEASURES.jsonl` | préfixes, avec sol |

### 4.2 Flux q3/q4 global à 8k/16k/32k (préfixes LiDAR scan0 avec sol, 2 cm, s8, W4, Local28)

Les préfixes sont des sous-échantillons imbriqués par priorité hash : la densité varie avec n. Le contrat les déclare diagnostics, pas preuves de croissance (`PASSATION.md:171-174`).

| Grandeur | 8k | 16k | 32k | Source | Réserve |
| --- | ---: | ---: | ---: | --- | --- |
| Temps pipeline K5, tranche 32 | 3,849 s | 22,364 s | 45,000 s | `q34_indexed_20260921/README.md:127-131` | sous charge, sans CPU·s |
| Temps pipeline K10, tranche 32 | 18,373 s | 91,485 s | 153,418 s | idem | idem |
| Temps pipeline K5, tranche 33 | 7,725 s | 24,224 s | 52,295 s | `q34_affine_20260921/performance/lidar_gladtapx/record_000{0,4,8}.json` | mêmes sorties, **moins** de travail de témoins que la tranche 32 |
| Temps pipeline K10, tranche 33 | 33,563 s | 75,357 s | 195,547 s | `.../record_00{02,06,10}.json` | idem |
| Temps pipeline K5, tranche 34, Individual | 4,576 s | 18,622 s | 72,304 s | `q4_seed_cells_20260921/performance_r2/lidar_kk34w49h/record_000{0,3,6}.json` | sorties identiques |
| Temps pipeline K10, tranche 34, LiveOnly | 38,414 s | 120,414 s | 227,213 s | `.../lidar_5vvvq0bh/record_000{0,1,2}.json` | idem |
| Sorties q3 / q4, K5 | 93 914 / 10 756 | 189 966 / 21 872 | 381 123 / 45 520 | records ci-dessus | — |
| Sorties q3 / q4, K10 | 409 195 / 116 988 | 844 053 / 252 216 | 1 709 820 / 524 458 | idem | — |
| Bornes q3 préparées (compte + coquille), K5 | 72,9 M | 225,2 M | 760,0 M (×3,09 / ×3,37) | `q34_indexed_20260921/ANALYSE_CROISSANCE.md:78` | travail déterministe |
| Bornes q3 préparées, K10 | 342,9 M | 861,7 M | 2 253,8 M (×2,51 / ×2,62) | idem l. 97 | — |
| Visites de carte q4 Local28, K5 (Individual) | 25,0 M | 100,4 M | 562,4 M (**×4,02 / ×5,60**) | idem l. 120 | poste super-quadratique observé |
| Mêmes visites en LiveOnly (tranche 34) | 5,92 M | 15,91 M | 38,85 M (×2,69 / ×2,44) | `q4_seed_cells_20260921/README.md:115-118` | bornes de blocs d'atlas ×4,112 au dernier doublement |

Constat corrigé : à 32k/K5, le temps passe de 45,0 s (tranche 32) à 52,3 s (tranche 33) puis 72,3 s (tranche 34 Individual). Le travail logique n'est **pas** identique :

- la tranche 33 réduit les visites de recherche de témoins par paire de 564,4 M à 120,8 M, et ses points testés de 227,3 M à 11,8 M ;
- les tranches 33 et 34 Individual ont les mêmes compteurs principaux ;
- les bornes q3, l'atlas et les covers sont identiques dans les trois tranches.

Moins de travail et plus de temps mur : ces captures ne séparent pas le moteur de la charge de l'hôte. Aucun de ces records ne contient de temps CPU.

### 4.3 Trames LiDAR entières avec sol (profil u16 2 cm, K5, s8, Local28 LiveOnly, moteur tranche 34)

| Grandeur | Valeur | Source | Réserve |
| --- | --- | --- | --- |
| Scène 0 locale, 119 142 sites, W4 | **383,31 s de mur de pipeline**, 1 417,52 CPU·s (3,70 CPU occupés) | `q34_spatial_20260921/performance/spatial_9kscvyt0/record_0006.json` | l'en-tête de colonne `README.md:47` dit « Pipeline CPU (s) » pour un temps mur ; le texte `README.md:57-59` donne pourtant correctement 1 417,521 s CPU |
| G4 W48, trames 0 / 100 / 200 (119 142 / 119 942 / 120 725 sites) | 165,214 / 34,319 / 505,479 s de mur | `q34_spatial_20260921/gcp_r1/READBACK.json` (`measurements[1..3]`) | une observation par cas ; CPU seulement |
| Même, CPU cumulé | 691,65 / 382,00 / 973,37 CPU·s | idem, `gnu_time` user+system | processus entier |
| Même, CPU logiques occupés | 4,19 / 11,13 / 1,93 sur 48 | idem | VM EPYC 9B45, 24 cœurs, SMT 2 |
| Boules q3 construites / émises | 244,8 M / 1,253 M ; 122,7 M / 1,213 M ; 273,1 M / 1,370 M | `q34_spatial_20260921/README.md:104-110` | **×195 / ×101 / ×199** de travail avant rejet (et non « ×196 à ×199 ») |
| Rapport local W4 / G4 W48 du temps de pipeline | ×2,907 (quart), ×2,320 (trame 0) | `gcp_r1/READBACK.json` (`local_comparison`) | hôtes et nombres de workers différents |
| Rapport local / G4 du CPU pour un même travail | ×2,05 (trame 0 : 1 417,5 / 691,65) ; ×1,45 (quart : 230,9 / 159,0) | records locaux et READBACK | calcul de cet audit ; `logical_work_equal=true` ; CPU local non consigné dans le reçu |
| Pilote G4 tranche 31, scan0 préfixes W48 | 1k 0,863 s ; 2k 8,470 s ; 4k 59,274 s ; 8k 614,744 s (1 986,83 CPU·s) | `lidar_global_20260921/README.md:129-151` | moteur 31, périmé par le census par boîtes ; 1k-4k sont des tailles d'oracle |
| Global 8k local, tranche 31 | 1 360,996 s ; 780,66 M boules q3 ; 361,2 G tests ponctuels q3 (99,096 % extérieurs) | `lidar_global_20260921/PARTIAL_GLOBAL_8K.md:4,37-39,68` | campagne FAILED à 16k |
| Référence v7 : tour FULL 50k uniforme, G4 | K10 418,873 s dont constructeur FULL 389,668 s ; K5 33,853 s dont 27,228 s ; amont 12,231 s (K10) et 4,464 s (K5) | `audits/CONTRATS_ET_MESURES.md:43-45,69-77` | autre entrée ; FULL mono-thread à 93 % du total à K10 et 80 % à K5 |

### 4.4 Trames entières sans sol (39 815 / 35 491 / 45 114 sites u16 ; hôte local 8 vCPU = 4 cœurs SMT)

La colonne « base » a été mesurée avec le binaire de la tranche 34 (sonde 925daeaa…), sous le contexte 92d74c13. Les deux dernières colonnes ne sont pas commises.

| Scène / K / W | Base mur / CPU (s) | Phase 1-2 (0e2c18ca) mur / CPU (s) | a74e90f2 u16 mur / CPU (s) | CPU/mur par worker, phase 1 → a74e90f2 | Source |
| --- | --- | --- | --- | --- | --- |
| 00 / K5 / W1 | 889,5 / 888,7 | **453,3 / 453,3** (charge 1,6) | — | — | `ground_phase1_20260921/README.md:31` |
| 00 / K5 / W8 | 298,0 / 1 283,4 | **108,0 / 741,5** (686 %, charge 2,8) | 105,0 / 803,4 (765 %) | 0,86 → 0,96 | l. 32 ; `ground_18bits…/BASELINE.only.json` |
| 00 / K10 / W8 | 911,1 / 3 876,5 | **323,0 / 2 212,1** (684 %, charge **9,25**, auto-induite) | 322,0 / 2 365,1 | 0,86 → 0,92 | l. 33 |
| 01 / K5 / W1 | 742,9 / 741,6 | 370,7 / 370,6 (charge 11,4) | — | — | l. 34 |
| 01 / K5 / W8 | 273,2 / 992,8 | 98,4 / 601,8 | 87,7 / 646,4 | 0,76 → 0,92 | l. 35 |
| 01 / K10 / W8 | 761,8 / 2 891,5 | 337,2 / 1 665,2 | 260,9 / 1 755,3 | 0,62 → 0,84 | l. 36 |
| 02 / K5 / W1 | 2 043,8 / 1 886,9 (contaminée) | 852,2 / 852,0 (charge 13,8) | — | — | l. 37 |
| 02 / K5 / W8 | 1 245,6 / 2 052,0 (contaminée) | 255,2 / 1 340,1 | 197,9 / 1 424,9 | 0,66 → 0,90 | l. 38 |
| 02 / K10 / W8 | 3 533,9 / 5 860,2 (contaminée) | **824,1 / 3 811,0** (charge 16,0) | 579,8 / 4 008,6 | 0,58 → 0,86 | l. 39 |
| 00 / K5 / W8, grille 1 mm, 39 885 sites | — | — | 104,63 / 812,82 (charge 0,24), q3 691 284, q4 158 496 | 0,97 | `u18_resume_20260922/ground_1mm_first/BASELINE.only.json` (non suivi) |

Lecture de la colonne « CPU/mur par worker », tirée de `workers_timing_ms` : dans les lignes a74e90f2, les workers ont obtenu autant ou plus de CPU que dans la scène 0 « au calme » de la phase 1. Deux conséquences :

- la charge 8,4 à 10,0 relevée avant ces lignes est surtout auto-induite : c'est la moyenne de charge sur une minute, qui inclut la ligne W8 précédente ou une compilation ;
- le surcoût CPU de +5,2 à +8,3 % à travail logique égal reste inexpliqué. Il peut venir d'une occupation SMT plus forte (672-765 % contre 462-686 %) ou de l'élargissement 18 bits ; une seule répétition ne tranche pas.

Effet de la phase 1-2 sur le travail, par scène (compteurs déterministes, `probe_0*` contre `only_probe_0*` / `probe_0*` de la phase 1) :

| Poste | 00 K5 | 00 K10 | 01 K5 | 01 K10 | 02 K5 | 02 K10 |
| --- | --- | --- | --- | --- | --- | --- |
| Boules q3 construites | 179,7 M → 31,0 M (−82,7 %) | 463,1 M → 35,4 M (−92,4 %) | 127,8 M → 20,2 M (−84,2 %) | 310,8 M → 26,1 M (−91,6 %) | 273,4 M → 33,0 M (−87,9 %) | 713,4 M → 35,4 M (−95,0 %) |
| Bornes de census q3 | 6,03 G → 1,11 G (÷5,4) | 17,26 G → 1,68 G (÷10,3) | 4,70 G → 0,79 G (÷6,0) | 12,96 G → 1,29 G (÷10,0) | 9,93 G → 1,24 G (÷8,0) | 29,73 G → 1,67 G (÷17,8) |
| Atlas q4 : blocs + points (inchangés) | 3,16 + 7,09 G | 10,72 + 24,20 G | 2,82 + 6,47 G | 8,22 + 19,47 G | 7,36 + 14,82 G | **22,25 + 44,54 G** |
| IDs de frontière copiés (inchangés) | 5,41 G | 18,31 G | 4,59 G | 13,65 G | 10,90 G | **32,45 G** |
| Visites de témoins par paire (inchangées) | 1,09 G | 1,92 G | 0,56 G | 1,08 G | 0,95 G | 1,98 G |
| Sites de covers (inchangés) | 2,90 G | 7,69 G | 1,75 G | 4,09 G | 3,44 G | 8,87 G |

La phase 1-2 a supprimé 82,7 à 95,0 % des constructions de boules q3 et divisé les bornes de census par 5,4 à 17,8. Elle n'a réduit **aucun** compte de l'atlas q4, des covers ni des recherches par paire. Le régime le plus dur est la scène 02 à K10 : l'atlas y totalise 66,8 G évaluations et 32,5 G copies par trame. À titre de comparaison, la scène 0 à K10 en compte 34,9 G et 18,3 G.

### 4.5 Exposants de croissance spatiaux (scan0 avec sol, K5, s8, W4, moteur 34)

Effectifs réels ; exposant = log(rapport de travail) / log(rapport d'effectifs). Source : `q34_spatial_20260921/GROWTH_SCAN0_K5_S8.json` (sha256 93b027d0…, versions normal et `-O` identiques).

| Relation (rapport d'effectifs) | Bornes q3 préparées | Bornes de blocs q4 | Tests ponctuels q4 | Sites de covers | Sorties q3 / q4 |
| --- | --- | --- | --- | --- | --- |
| trame → moitié x+ (1,9873) | ×5,576, **2,502** | ×4,962, **2,332** | ×4,004, **2,020** | ×5,384, **2,451** | 1,256 / 1,877 |
| trame → moitié x− (2,0129) | ×2,254, 1,162 | ×2,366, 1,231 | ×2,189, 1,120 | ×2,500, 1,310 | 0,792 / 0,463 |
| moitié x− → quart x−y− (2,0320) | 1,021 | 1,297 | 1,204 | 1,188 | 1,160 / 1,350 |
| moitié x− → quart x−y+ (1,9690) | 1,246 | 0,615 | 0,778 | 0,919 | 0,863 / 0,725 |
| moitié x+ → quart x+y− (1,9966) | 1,596 | 1,534 | 1,521 | 1,245 | 1,046 / 1,161 |
| moitié x+ → quart x+y+ (2,0034) | 0,838 | 0,743 | 0,708 | 0,994 | 0,966 / 0,861 |

Constat corrigé :

- **Sur la relation trame → moitié x+**, les quatre colonnes de travail du tableau dépassent 2, et non deux. D'autres compteurs de cette relation dépassent aussi 2 :
  - graines et boules q3 : 2,461 ;
  - masses de paires des témoins par rectangle : 2,531 à 2,617 ;
  - au total, plus de 60 compteurs.
- **Sur les cinq autres relations**, les postes du tableau restent sous 2 (maximum 1,596). Des populations de sites rejetés ou exclus y dépassent pourtant 2 dans trois relations, entre 2,048 et 2,113.
- **Sorties** : tous les exposants restent sous 1,9.
- **Portée temporelle** :
  - l'exposant des bornes q3 (2,502) décrit un poste que 0948d2d0 a divisé par 5 à 18 : il est **périmé** ;
  - l'exposant des bornes de blocs q4 (2,332) porte sur des comptes que la phase 1-2 n'a pas changés. Il reste plausiblement valide pour le moteur courant ; c'est une **inférence**, fondée sur l'égalité des comptes d'atlas avant et après la phase 1-2 sur les trois scènes sans sol.
- Aucune croissance spatiale n'a été mesurée sans sol, à K10, ni sur les scènes 100/200 en local.

### 4.6 Écart au contrat

Contrat (`docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:9-22`, `AGENTS.md:5-27`) :

- toute la tour HGP K=1..10 d'une trame SemanticKITTI entière brute en moins de 1 s sur G4 ;
- repli sur K=1..5, puis objectif de 100 ms ;
- le régime sans sol est « un régime prioritaire supplémentaire, pas un remplacement » (`CONTRAT…:71-75`).

Budget idéal retenu : 1 s × 48 CPU logiques = **48 CPU·s** (4,8 CPU·s pour 100 ms), à parallélisme parfait sur 48 fils SMT (24 cœurs physiques). Facteur de travail à gagner : F = CPU·s mesurés / 48.

| Mesure (flux q3/q4 seulement) | CPU·s | F pour 1 s | F pour 100 ms | Mur actuel / 1 s | Source |
| --- | ---: | ---: | ---: | ---: | --- |
| Trame brute 0, K5, G4 W48 | 691,65 | ×14,4 | ×144 | ×165 | `q34_spatial…/gcp_r1/READBACK.json` |
| Trame brute 100, K5, G4 W48 | 382,00 | ×8,0 | ×80 | ×34 | idem |
| Trame brute 200, K5, G4 W48 | 973,37 | ×20,3 | ×203 | ×505 | idem |
| Trame brute, K10 | non mesuré | — | — | — | — |
| Sans sol scène 0, K5, W1 (0e2c18ca) | 453,3 | ×9,4 | ×94 | ×453 | `ground_phase1…/BASELINE.only.json` |
| Sans sol scène 0, K5, W8 | 741,5 | ×15,4 | ×154 | ×108 | idem |
| Sans sol scène 0, K10, W8 | 2 212,1 | ×46,1 | ×461 | ×323 | idem |
| Sans sol scène 01, K5 W1 / K10 W8 | 370,6 / 1 665,2 | ×7,7 / ×34,7 | ×77 / ×347 | ×371 / ×337 | `ground_phase1…/BASELINE.json` (charge 9,5 à 11,4) |
| Sans sol scène 02, K5 W1 / K10 W8 | 852,0 / 3 811,0 | ×17,8 / ×79,4 | ×178 / ×794 | ×852 / ×824 | idem (charge 13,8 à 16,0) |
| Sans sol scène 02, K10, W8, a74e90f2 | 4 008,6 | ×83,5 | ×835 | ×580 | non commis |
| Sans sol 1 mm scène 0, K5, W8 | 812,82 | ×16,9 | ×169 | ×105 | non suivi |

Ce que cela implique :

- **Borne inférieure seulement.** Ces flux n'incluent ni la voie q2 dans le même appel, ni la déduplication et le catalogue, ni les intérieurs, ni le fold des K hiérarchies.
  - En v7, sur 50k uniforme, le constructeur FULL **mono-thread** faisait 93 % du temps de la tour K10 et 80 % à K5 (`audits/CONTRATS_ET_MESURES.md:74-78`). Cette part dépend de ce que le FULL n'était pas parallélisé.
  - La voie q2 seule, sur 50k LiDAR avec sol à la tranche 14, cumulait 10,1 s de durées de workers à K10 (`worker_ms_sum`, pas un CPU·s), soit environ un cinquième du budget.
- **K10 coûte ×2,72 à ×3,02 le K5** en CPU (W8, sans sol, trois scènes, trois binaires) : le repli K5 ne donne pas une marge suffisante pour K10.
- **Le parallélisme parfait n'existe pas encore.**
  - Localement, passer de W1 à W8 gonfle le CPU de ×1,57 à ×1,64 avec la file de tâches, et de ×1,34 à ×1,44 dans la base (scène 02 exclue).
  - Sur G4 (moteur 34, avant la file de tâches), 4 à 23 % des 48 CPU sont occupés.
  - Aucune mesure G4 n'a eu lieu depuis la file de tâches (5224ff4e).
- **Le facteur G4 réel est incertain d'un facteur 2.** Pour un même travail, G4 a consommé 1,45 à 2,05 fois moins de CPU·s que l'hôte local. Les facteurs calculés sur le CPU local surestiment peut-être l'écart G4 ; ce rapport n'a pas été mesuré sans sol.
- **Ordre de grandeur arithmétique (hypothèse, pas une mesure).** À 1 ns par évaluation, l'atlas q4 seul coûterait :
  - 34,9 CPU·s à K10 sur la scène 0, soit 73 % du budget ; en comptant aussi les copies à 1 ns, 53,2 CPU·s ;
  - **66,8 CPU·s sur la scène 02, soit plus que le budget sans même compter les copies**.

  Le jalon 1 s K10 exige donc très probablement de **réduire les comptes** de l'atlas, pas seulement leur coût unitaire. C'est une inférence. Pour 100 ms, il faudrait deux ordres de grandeur de moins, ou un portage GPU de ces postes.
- **En synthèse**, pour le seul flux q3/q4 et à parallélisme parfait, le travail doit baisser de :

  | Régime | Facteur à gagner |
  | --- | --- |
  | Sans sol K5 (W1, trois scènes) | ×8 à ×18 |
  | Sans sol K10 (W8) | ×35 à ×84 |
  | Trame brute K5 (G4) | ×8 à ×20 |
  | Trame brute K10 | non mesuré ; ×22 à ×61 en appliquant le rapport K10/K5 observé (estimation non épinglée) |

  Viser 100 ms ajoute un facteur ×10 partout.

## 5. Défauts, risques et dettes

### Haute

1. **Les reçus du régime prioritaire sont les moins disciplinés.**
   - `ground_baseline` et `ground_phase1` utilisent le schéma `mhgp8_ground_baseline_v1` : pas de `COMPLETION.json` ni de `MANIFEST.json` de fermeture, pas de relecture normal/`-O` archivée.
   - **Quatre des cinq lanceurs épinglés sont hors de tout commit** :
     - 0e50cd31 (base), 1f7c3846 (`BASELINE.only.json`) et d110a052 (identité u16) : dans aucune révision ;
     - 9c0971df (ligne 1 mm) : seulement dans l'index non commis.
   - Seul 8f314d87 (`ground_phase1/BASELINE.json`) correspond à une version commise, celle de 5224ff4e. Les autres versions commises sont 7314c67c à 72f125c6 et c3c89732 à a74e90f2.
   - La sonde 0e2c18ca (bb7f01fc…) était dans un scratchpad de session et n'existe plus.
   - Preuve : `pins.runner_sha256` et `pins.probe` des cinq `BASELINE*.json` ; `git show <c>:morsehgp3D_v8/bench/run_ground_baseline.py | sha256sum`.
2. **Contamination de charge sur les chiffres qui servent à piloter.**
   - La base scène 02 est contaminée par la campagne suivante (`ground_baseline_20260921/README.md:39-45`).
   - Phase 1, scènes 01/02 : chevauchement jusqu'à une charge de 16 ; le CPU par worker n'y fait que 0,58 à 0,76 du mur.
   - Trois lignes de la scène 0 sont non vérifiables.
   - `ground_phase1_20260921/README.md:25` affirme « charge < 3 avant chaque ligne » pour la reprise au calme, alors que la ligne K10 W8 démarre à 9,25. Cette charge est très probablement auto-induite par la ligne W8 précédente, mais l'affirmation est fausse telle qu'écrite.
3. **Voie q2 jamais mesurée sans sol, ni sur LiDAR depuis la tranche 14.**
   - Parmi les 16 répertoires `q2_*`, seul `q2_dynamic_front_20260914` référence des entrées `lidar08_20260914/prepared` (13 JSON ; scans 0/100/200, préfixes avec sol).
   - Les tranches 17-21 (surproposition ×0,36 à ×0,67, héritage) ne sont mesurées que sur synthétique.
   - `q2_terminal_pool_20260914/README.md:215` écrit « Aucun nouveau test produit 50k/LiDAR ». Or la tour du contrat exige q2.
4. **Aucune mesure de l'aval de la tour.** La v8 ne mesure qu'un flux de candidats en mode digest. La seule mesure de tour disponible est celle de la v7 (50k uniforme, FULL mono-thread à 93 %). L'écart au contrat du § 4.6 est une borne inférieure d'amplitude inconnue.
5. **Le moteur courant n'a aucune mesure aux tailles d'intérêt 8k/16k/32k** (dernière : tranche 34, reçus introduits par 4c3cdb0c), ni de croissance spatiale. L'exposant 2,502 des bornes q3 est périmé par 0948d2d0.
6. **La tranche u18 non commise n'a aucune qualification close en succès.**
   - Les quatre `COMPLETION.json` de `u18_resume_20260922` sont `failed` :
     - `release` : CTest à 134/136, deux lecteurs de mutations en échec ;
     - `sanitize` : interrompue par SIGINT ;
     - `release_r2` et `sanitize_r2` : **CTest vert** (136/136 et 131/131, `exit_code` 0), rejetés seulement par le validateur de capture (« disabled CTests differ », ensembles de 3 et 8 tests désactivés).
   - Le README ne mentionne que `release` et `sanitize`. La note `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:80-85` décrit la reprise R2 sans dire que sa capture est `failed`.
   - La première ligne 1 mm a été mesurée avec la sonde 57135518…, le binaire des captures `release` (échouée) et `release_r2` (identique octet pour octet).

### Moyenne

7. **Pas de CPU·s dans les grandes mesures LiDAR des tranches 32-34** : les records n'ont ni `child_cpu_seconds` ni GNU time. Les temps muraux varient à rebours du travail (45,0 → 52,3 s à 32k/K5 alors que la tranche 33 fait 4,7 fois moins de visites de témoins par paire), ce qui interdit toute comparaison de temps entre tranches.
8. **Aucune mesure G4 depuis la file de tâches.** L'occupation de 1,93 à 11,13 CPU date du moteur 34. Le raccord G4 est verrouillé sur l'ancienne autorité de 216 sources (`docs/JOURNAL_DEVELOPPEMENT_20260921.md:71-99`).
9. **Modèle de CPU local non consigné et variable.**
   - Les reçus du 13 au 15 septembre consignent un hôte `codespaces-0bcb78` sous **AMD EPYC 9V74** (Zen 4, 67 occurrences, par exemple `q2_dynamic_front_20260914/campaigns/lidar_growth/MANIFEST.json`, `cpuinfo`).
   - Le même hôte expose aujourd'hui un **AMD EPYC 7763** (Zen 3).
   - Les reçus du 16 au 22 septembre ne consignent que le nom d'hôte et le noyau.
   - Les temps locaux de dates différentes, ainsi que les rapports local/G4, peuvent donc comparer des microarchitectures différentes sans que le reçu le dise.
   - Les mesures d'audit du 22 septembre sont en plus faites sur GitHub Actions et sur un Intel Xeon Platinum 8370C (`audits/lidar_rectangles_20260922/README.md`, § 1).
10. **Décisions fondées sur des chiffres non épinglés.**
    - Coûts float32 : « ×23 à ×70 par prédicat, clé 78 µs, 68 à 185 ms par arête » (`docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65,92`). Aucun reçu `float32_*` ne contient ces chiffres. Ils appuient la décision proposée D1, qui invoque aussi « ×270 de travail inutile » et « 1 728 bits » (l. 123-135). La décision finale « entier 18 bits » est une décision utilisateur (`PASSATION.md:36-37`).
    - Chronos du quart sans sol de 7 067 sites : 48,5 → 39,9 → 27,3 s ; W8 7,2 s ; cache de formes neutre ; passe conjointe +18 % sur deux répétitions. Ils ne figurent que dans le journal (`JOURNAL…:12-18,45-58`), sur une taille inférieure à 8k.
11. **Pseudo-mesure d'échelle.** `release_r2/scale_{8000,16000,32000}.json` rapportent 7 tests, 7 visites et 0 sortie à toutes les tailles (une arête fournie, sortie q4 vide). Le README (`u18_resume_20260922/README.md:37-39`) et `AGENTS.md` non commis le déclarent eux-mêmes. Le risque n'existe que si ces fichiers sont lus comme une croissance.
12. **Preuves d'audit hors dépôt.**
    - 12294241 renvoie ses sources et captures à `MorseHGP_LiDAR_cascade_2026-09-22.zip`, « jointe à la conversation ». L'appelant a été compilé par g++ 14.2 Debian (`audits/cascade_rectangles_20260922/RESULTS.json`).
    - a5447d06 (`audits/lidar_rectangles_20260922/`) s'appuie sur l'artefact GitHub Actions 10704262200 (ZIP c1507232…), dont la durée de conservation est limitée.
    - Les gains de ×2,55 à ×4,46 ne sont donc pas rejouables depuis le dépôt seul, même si l'appelant `probe.cpp` et `run.py` sont versionnés.
13. **Ambiguïté de cible contractuelle.**
    - `AGENTS.md:29-39` et le contrat maintiennent la trame brute entière comme référence.
    - La reprise du 21 septembre traite le sans-sol comme le lieu du contrat (`AUDIT_REPRISE…:18-20`).
    - La décision « entier 18 bits » (`PASSATION.md:36-43`) se heurte au texte non commis d'`AGENTS.md` : « ne remplace ni le profil float32 par défaut ni le contrat brut entier ».
14. **Dépendance aux builds et entrées non versionnés.**
    - Les lecteurs `--check-live` exigent les 181 répertoires `build/v8*`.
    - Les entrées u16 sans sol (`audits/lidar08_20260914/prepared/`, ignorées par git) et les `.bin` KITTI sont locales.
    - La v9 perdra ces preuves si l'environnement est recréé.
15. **Une seule observation par configuration** dans toutes les grandes mesures (hors trois répétitions q2 à 50k). Le contrat demande des répétitions à froid et à chaud (`CONTRAT…:18-21`) ; `audits/CONTRATS_ET_MESURES.md:26` le relevait déjà.

### Basse

16. En-tête de colonne « Pipeline CPU (s) » pour un temps mur (`q34_spatial_20260921/README.md:47`) ; le texte l. 57-59 est correct. Le ×13 à ×18 de l'audit de reprise (`AUDIT_REPRISE…:53-55`) se reconstruit exactement avec un rapport local/G4 implicite de 1,45 : 897,8 / (48 × 1,45) = 12,9 et 1 239 / (48 × 1,45) = 17,8. Ce rapport n'est pas énoncé.
17. Volume : 974 Mo et 16 016 fichiers de reçus publiés, dont 1 686 fichiers binaires de données (124,6 Mo) et 9 archives (65,4 Mo). Des dérivés KITTI sous licence non commerciale sont versionnés. Le commit 4c3cdb0c insère à lui seul +5,5 M lignes.
18. `audit_v7_20260913` n'a pas de README ; `ground_18bits_20260922` non plus.

## 6. Questions ouvertes

1. Quelle est la cible chiffrée de la v9 : trame brute entière (≈120k sites) ou trame sans sol (35-46k) ; float32 ou entier 18 bits ? Tant que ce n'est pas tranché, le facteur à gagner varie de ×8 à ×84 pour le seul flux q3/q4.
2. Combien coûte la tour complète (q2 + q3/q4 + catalogue + fold K=1..10) sur une trame sans sol ? Quelle part y prendrait un aval parallélisé, alors que la v7 ne mesure qu'un FULL mono-thread ?
3. Le moteur q3/q4 de la v8 est-il meilleur ou pire que l'amont de la v7 sur la même entrée ? Aucune campagne appariée v7/v8 n'existe : le flux global v8 n'a été exécuté que sur des entrées LiDAR (préfixes de 64 à 50k et trames), jamais sur le nuage uniforme 50k de la v7.
4. Le surcoût CPU de ×1,57 à ×1,64 entre W1 et W8 vient-il du SMT, du partage de cache ou de la file de tâches ? Qu'il soit plus fort dans la phase 1 (occupation 686 %) que dans la base (430 %) oriente vers le SMT, sans le prouver.
5. Le rapport CPU local/G4 (×1,45 à ×2,05) est-il stable sur les trames sans sol, et sur quel modèle de CPU local ?
6. Les exposants 2,33 (blocs d'atlas) et 2,45 (covers) subsistent-ils sans sol ? Les 21 morceaux u16 sans sol existent (manifeste `lidar_ground_u16_20260921/MANIFEST.json`) mais n'ont jamais été mesurés, à l'exception du quart de 7 067 sites dans le journal.
7. L'élargissement 18 bits coûte-t-il 5 à 8 % de CPU ? La comparaison entre a74e90f2 et 0e2c18ca mêle binaire, occupation SMT et charge.

## 7. À porter en v9 et à ne pas reprendre

### 7.1 À porter

| Quoi | Où | Pin | Pourquoi |
| --- | --- | --- | --- |
| Protocole de capture : MANIFEST avant, COMPLETION après, sha256 des sources, binaires, entrées et dépendances, commandes et sorties brutes en base64, refus d'écraser, échecs conservés et non promus | `bench/run_*_checks.py` des tranches 23-34, par exemple `run_q34_indexed_lidar.py`, `run_q4_seed_cells_checks.py` | 4c3cdb0c (reçus de la tranche 34) | 14 hash de README (dont 9 recontrôlés ici) et 25 hash de sondes, sans écart ; immuabilité tenue sur 46/48 répertoires, les deux autres n'ayant reçu que des ajouts |
| Schéma v2 du reçu sans sol : fermeture, `--check-live`, sources et entrées hachées, `source_coupling` explicite | `bench/run_ground_baseline.py` de l'index non commis (sha256 9c0971df…) | à commettre **avant** toute nouvelle mesure et à requalifier | corrige les défauts du schéma v1 (§ 5.1) |
| Entrées sans sol figées : masque Patchwork++ 3e6903a1 et morceaux 1 mm versionnés ; morceaux u16 **non versionnés**, régénérables par `prepare_lidar_ground_u16.py read` | `receipts/lidar_ground_20260921`, `receipts/lidar_ground_u16_20260921` (manifeste seul) | préparateur 3e9000b9…, masques 9db3fe5c… | entrées vérifiées ; décider en v9 si les u16 sont versionnés ou régénérés |
| Protocole spatial à densité constante : trame, deux moitiés, quatre quarts, effectifs réels, six relations publiées, y compris les défavorables | `docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md`, `bench/run_q34_spatial.py` | 70de84f2 | seul diagnostic de croissance légitime sur LiDAR entier |
| Identités de parallélisme : `logical_sha256`, 439 à 449 compteurs, W1/W8 bit-identiques, registre `published = consumed`, `workers_timing_ms` | sonde `mhgp8_wspd_q34_probe` schéma v5 | 5224ff4e (file de tâches, registre) et **5fdda963** (`workers_timing_ms`) | mesure le parallélisme au lieu de le déclarer ; `workers_timing_ms` a permis ici de dé-contaminer la lecture de charge |
| Protocole G4 relu a posteriori : `validated_complete`, `GPU_executed=false`, `targeted_shutdown_certified`, archive hôte en liste blanche | `gcp-migration/q34_spatial_*_v8.py`, `receipts/q34_spatial_20260921/gcp_r1` | 70de84f2 | une session SPOT (budget utile 900 s), quatre cas achevés, arrêt certifié |
| Vecteur de travail non additionné : populations distinctes des lectures, sous-comptes non sommés | `q34_indexed_20260921/ANALYSE_CROISSANCE.md:36-54` | d1b4dbc6 | a permis de localiser l'atlas q4 et les recherches par paire |
| Référence v7 de la tour FULL 50k (temps, RSS 15,5 Gio, 21,47 M boules) comme seule mesure d'aval | `morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910` | reçu introduit par ad7ffd28 (tête v7 dc57ffd5) | l'aval absent en v8 doit être comparé à quelque chose ; FULL mono-thread |
| Consigner le modèle de CPU (`cpuinfo`) dans chaque reçu | comme les reçus du 13 au 15 septembre (`MANIFEST.json`, champ `cpuinfo`) | 4e878754 | l'hôte a changé de microarchitecture sans que les reçus récents le disent |

### 7.2 À ne pas reprendre (pistes fermées par une mesure)

| Piste | Mesure qui l'a fermée | Source |
| --- | --- | --- |
| Lots compacts de singletons q2 (8 ou 16 voies) | 54 comparaisons à n ≥ 8k : lots/Coarse de ×1,005 à ×1,225, médiane ×1,112, aucune plus rapide | `q2_singleton_batch_20260915/README.md:104` |
| Census conjoint A/B équilibré | rangées 8k : 0,242 s → 1,888 s ; amas : 1,29 M ancres → 15,4 M relais | `q2_joint_r2_20260914/README.md:105-120` |
| Équipe persistante coopérative q2 | 17 des 18 comparaisons rangées/Pool0 plus lentes que Coarse (32k : 317,8 → 398,0 ms W4) | `q2_cooperative_20260915/README.md:68-78` |
| Détachement de census q2 sur une équipe neuve | 8 comparaisons W1/W4 favorables sur 72, ratio médian 0,0738 | `q2_census_split_20260915/README.md:81-83` |
| Plages d'ancres comme accélérateur | Coarse W4/ranges W4 : médiane 1,039 ; zéro bande Pool filtrée sur 174 mesures | `q2_anchor_ranges_20260915/README.md:69-73,93-95` |
| Addition seule des colonnes axiales | 237-238 ms contre 56-58 ms pour la référence | `additive_q2_20260913/README.md:72-79` |
| Joined (graines × cellules) q4 comme défaut | 57,242 M initialisations de cache à 32k/K5 ; 113,7 s contre 72,3 s Individual (une observation) | `PASSATION.md:226-227` ; records `lidar_kk34w49h` |
| Window30 comme remplaçant de Local28 | K10/32k : 22,038 G comparaisons de sélection, 240,96 s contre 153,42 s. À K5/32k, Window30 était plus rapide en mur (40,54 s contre 45,00 s, sous charge) : la fermeture repose sur K10 et sur le travail | `q34_indexed_20260921/ANALYSE_CROISSANCE.md:7` ; `README.md:127-132` |
| Variance + Collective, carte de centres q4 comme défaut | régressions jusqu'à ×1,56 ; aucun rejet sur les grands fonds | `q34_collective_20260920/README.md:89-91`, `q4_center_map_20260920/README.md:119` |
| Couches duales q4 (29) seules | adversaire K10/256 : régression ×10,27 | `q4_shallow_20260920/README.md:119` |
| Préfixes LiDAR à densité variable comme preuve de croissance | déclarés diagnostics seulement | `PASSATION.md:171-174` |
| Cache de formes dans la frontière des fragments ; passe conjointe des quatre cellules filles | 27,32 s contre 27,32 s ; +18 % (31,7 s contre 26,8 s) | `JOURNAL…:45-58` (**non épinglé**, 7 067 sites < 8k : à reconfirmer par un reçu si la piste est rouverte) |
| Temps mur local sous charge, ou pris sur un hôte non identifié, comme critère de décision entre tranches | 45,0 / 52,3 / 72,3 s à 32k/K5 pour un travail décroissant | records des tranches 32-34 |

## 8. Recommandations priorisées pour la v9

1. **Figer la cible et le chronomètre avant toute mesure.** Écrire une ligne contractuelle unique : entrée brute ou sans sol, profil 2 cm, 1 mm ou float32, K5 et K10, s8. Écrire aussi les budgets de 48 CPU·s et 4,8 CPU·s, et publier la frontière D2. Chaque mesure v9 indique à quel contrat elle se rattache.
2. **Imposer un schéma de reçu v2** à la place du schéma v1 sans sol :
   - lanceur commis avant la mesure ;
   - sonde conservée sous `build/` épinglé, jamais dans un scratchpad ;
   - `COMPLETION.json` et relectures normal/`-O` archivées ;
   - charge consignée avant et après, et `workers_timing_ms` pour juger la contention, au lieu de la seule moyenne de charge ;
   - modèle de CPU (`cpuinfo`) ;
   - CPU·s GNU time sur toute grande mesure.
3. **Rebaser sur un reçu propre dès l'ouverture.**
   - Mesurer les trois trames sans sol × K5/K10 × W1/W8 avec le moteur de départ de la v9, sur hôte calme, avec deux répétitions.
   - Ajouter les 21 morceaux sans sol (croissance spatiale).
   - Ajouter les préfixes 8k/16k/32k, pour respecter les tailles d'intérêt.
4. **Mesurer la tour, pas un flux.** Dès qu'un aval minimal existe (q2 dans l'appel, catalogue canonique, fold K=1..10), chronométrer la tour entière sur une trame. Jusque-là, publier chaque facteur d'écart comme borne inférieure.
5. **Mener une campagne appariée v7/v8/v9** sur le nuage uniforme 50k de la v7 et sur une trame LiDAR, avec des digests au même format. Elle dira si le générateur exact v8 régresse par rapport à l'amont v7 (12,2 s à K10 sur G4).
6. **Viser les comptes, pas les constantes.** Ces postes n'ont pas bougé pendant la phase 1-2 :
   - l'atlas q4 : jusqu'à 66,8 G évaluations et 32,5 G copies à K10 sur la scène 02 ;
   - les covers : jusqu'à 8,9 G sites ;
   - les recherches par paire : environ 2 G visites.

   Chaque tranche v9 publie ces compteurs avant et après, sur les trois scènes.
7. **Tenir une session G4 CPU sans sol** dès que le local gagne au moins ×5. Elle mesure l'occupation réelle avec la file de tâches et le rapport CPU local/G4. Porter ensuite sur GPU le poste dominant mesuré.
8. **Commettre ou retirer la tranche u18.**
   - Corriger la liste des tests désactivés attendus par le validateur, qui a fait échouer `release_r2` et `sanitize_r2` alors que leur CTest était vert.
   - Obtenir une qualification close en succès.
   - Documenter les quatre échecs dans le README.
9. **Rapatrier dans le dépôt toute preuve d'audit citée** : archive de 12294241, artefact Actions 10704262200. À défaut, la déclarer non vérifiable.
10. **Maîtriser le volume des reçus** (974 Mo en v8) : versionner les dérivés régénérables par leurs hash plutôt que par leurs octets, et écrire une politique de licence KITTI explicite.

## 9. Contre-vérification

Chaque affirmation principale et chaque chiffre du rapport d'origine ont été rouverts dans leur source.

**Confirmé sans changement :**

- les 51 répertoires et leur statut git ;
- les 25 sha256 de sondes, 9 sha256 de README et l'identité des deux fichiers `GROWTH` ;
- les murs, CPU·s et occupations G4 ;
- les 383,311 s de mur et 1 417,52 CPU·s locaux ;
- les exposants 2,502 / 2,332 et les six relations du tableau ;
- les CPU·s des bases sans sol, de la phase 1, de l'identité u16 et de la ligne 1 mm ;
- les facteurs d'écart au budget ;
- les comptes de la phase 1-2 sur la scène 0 ;
- les chiffres q2 synthétiques et LiDAR ;
- les temps des tranches 32-34 ;
- le pilote G4 de la tranche 31 et le global 8k ;
- la référence v7 ;
- les chiffres des pistes fermées ;
- le volume (974 Mo, 16 016 fichiers) ;
- l'absence de reçu pour les coûts float32 ;
- les quatre `COMPLETION.json` u18 en `failed` ;
- la pseudo-échelle de l'atlas saturant ;
- l'archive zip hors dépôt.

### 9.1 Corrections

| # | Affirmation d'origine | Verdict | Valeur ou formulation juste | Preuve |
| ---: | --- | --- | --- | --- |
| 1 | « 3 des 5 lanceurs sont absents de git » | corrigé | 4 sur 5 hors de tout commit : 0e50cd31, 1f7c3846 et d110a052 dans aucune révision, 9c0971df (ligne 1 mm) seulement dans l'index. Seul 8f314d87 est commis (5224ff4e). | `pins.runner_sha256` des cinq JSON ; `git show :…` et `git show <c>:…` |
| 2 | Identité u16 « sous une charge de 8,4 à 10,0 », comprise comme une contamination | corrigé | Charge surtout auto-induite (moyenne sur une minute incluant la ligne W8 précédente ou une compilation). CPU/mur par worker de 0,84 à 0,96, supérieur ou égal à la référence calme de 0,86. Surcoût CPU de +5,2 à +8,3 % sur les six lignes (+6,9 à +8,3 % sur la scène 0), non attribuable. | `workers_timing_ms` des sondes |
| 3 | Scène 0 de la phase 1 « au calme » | corrigé | 2 lignes sur 3 démarrent sous une charge inférieure à 3 ; la ligne K10 W8 démarre à 9,25, contrairement à `ground_phase1_20260921/README.md:25`. Son CPU par worker (0,86) n'indique pas de contention. | `BASELINE.only.json` (`load_before`) |
| 4 | « La phase 1-2 retire 83 à 92 % des census q3 ; bornes divisées par 5 à 10 » | corrigé | Scène 0 : boules −82,7 % / −92,4 % et bornes ÷5,4 / ÷10,3. Sur les trois scènes : boules −82,7 à −95,0 %, bornes ÷5,4 à ÷17,8. | `probe_0*` contre `only_probe_0*` / `probe_0*` |
| 5 | Atlas « 34,9 G + 18,3 G à K10 » comme pire cas | complété | Scène 02 K10 : 66,8 G évaluations et 32,5 G copies. | `probe_08_s02_k10_w8.json` des deux reçus |
| 6 | « Même à 1 ns par opération, cela dépasse le budget » | corrigé | Scène 0 K10 : 34,9 CPU·s d'évaluations (73 % du budget) ; le dépassement (53,2 CPU·s) suppose des copies à 1 ns. Scène 02 K10 : 66,8 CPU·s sans les copies, dépassement net. | calcul |
| 7 | « Deux postes super-quadratiques sur une relation sur six » | corrigé | Quatre colonnes du tableau dépassent 2 sur trame → moitié x+ (2,502 / 2,332 / 2,020 / 2,451), plus de 60 compteurs au total. Des sites rejetés dépassent 2 dans trois autres relations. | `GROWTH_SCAN0_K5_S8.json` |
| 8 | Exposants « périmés par 0948d2d0 » | nuancé | Périmé pour les bornes q3. Les comptes d'atlas sont inchangés par la phase 1-2 sur les trois scènes sans sol : 2,332 reste plausiblement valide (inférence). | § 4.4 |
| 9 | Tranches 32/33/34 : « travail logique identique » | réfuté | Mêmes sorties, mais la tranche 33 fait 564,4 M → 120,8 M visites de témoins par paire. Les temps (45,0 / 52,3 / 72,3 s) montent quand le travail baisse. | records `lidar_86twby55/0008`, `lidar_gladtapx/0008`, `lidar_kk34w49h/0006` |
| 10 | « ×196 à ×199 de travail avant rejet » | corrigé | ×195 / ×101 / ×199 (la trame 100 fait ×101). | `q34_spatial_20260921/README.md:109-110` |
| 11 | « K10/K5 ×2,77 à ×3,02 », calculé aussi sur l'identité u16 | corrigé | ×2,72 à ×3,02 avec l'identité u16 ; ×2,77 à ×3,02 sur la base et la phase 1 seules. | rapports ligne à ligne |
| 12 | « Surcoût W1 → W8 ×1,44 (base) » | corrigé | Base : ×1,34 (scène 01) à ×1,44 (scène 00), scène 02 contaminée. Phase 1 : ×1,57 à ×1,64 confirmé. | idem |
| 13 | Cinq tranches moteur, dont 0e2c18ca | corrigé | 0e2c18ca ne modifie que le journal. Quatre tranches moteur (748ec082, 02987f18, 0948d2d0, 5224ff4e), plus l'instrumentation 5fdda963. | `git show --stat` |
| 14 | `workers_timing_ms` épinglé sur 5224ff4e | corrigé | Ajouté par 5fdda963 : 5224ff4e n'en contient aucune occurrence. | `git show <c>:…/wspd_q34_probe.cpp` |
| 15 | Worktree partagé en retard de « 7a121e44 … 12294241 » | corrigé | 13 commits, bf73e194 … 12294241. | `git log a74e90f2..12294241` |
| 16 | Entrées sans sol « figées » (u16 et 1 mm) | nuancé | Les morceaux u16 ne sont pas versionnés (`.gitignore` `/prepared/`) ; seuls le manifeste et les morceaux 1 mm le sont. | `git check-ignore -v` ; `lidar_ground_u16_20260921/README.md` |
| 17 | Base « moteur 92d74c13 » | précisé | Sonde 925daeaa = binaire de la tranche 34 (`build/v8_q4_seed_cells_r2_20260921`) ; neuf sources identiques à 70de84f2. | `sha256sum` ; `git show` des sources |
| 18 | « Les quatre qualifications u18 sont FAILED » | nuancé | Statuts confirmés, mais `release_r2` (136/136) et `sanitize_r2` (131/131) ont un CTest vert : l'échec vient du validateur de la liste des tests désactivés. La ligne 1 mm utilise le binaire de ces captures. | `u18_resume_20260922/*/ctest.json` |
| 19 | Voie q2 LiDAR « scan0 avec sol, 4 workers, K10 » | complété | Aussi scans 100/200 à 50k K10, et K5 sur scan0 de 8k à 50k ; toujours avec sol et à la tranche 14. | `lidar50k_other_scans/MEASURES.jsonl` |
| 20 | « Aval FULL = 93 % » | nuancé | 93 % à K10 et 80 % à K5, avec un constructeur FULL mono-thread. | `audits/CONTRATS_ET_MESURES.md:37,74` |
| 21 | Pin de la référence v7 dc57ffd5 | précisé | Reçu introduit par ad7ffd28 ; dc57ffd5 est la tête v7 auditée. | `git log -- morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910` |
| 22 | Immuabilité : « seulement des ajouts aux README » | précisé | 204b0620 ajoute aussi 11 lignes à `float32_q3_census_20260921/ANALYSE_CROISSANCE.md` ; ajouts purement additifs. | `git show --numstat 204b0620` |
| 23 | La v8 n'a exécuté le flux global que sur « des entrées LiDAR à n ≥ 8 000 » | corrigé | Préfixes LiDAR de 64 à 50k (48 mesures préliminaires à n = 64 et 256, pilote G4 de 1k à 8k) et trames ; jamais le nuage uniforme 50k. | `lidar_global_20260921/README.md:21-22,129-135` |
| 24 | Window30 fermé par 240,96 s contre 153,42 s | nuancé | Vrai à K10 ; à K5/32k, Window30 était plus rapide en mur (40,54 s contre 45,00 s, sous charge). | `q34_indexed_20260921/README.md:129-130` |
| 25 | « Les 383 s : la colonne s'intitule à tort Pipeline CPU » | confirmé, précisé | Seul l'en-tête l. 47 est faux ; `README.md:57-59` donne correctement 1 417,521 s CPU. | README |
| 26 | Le ×13 à ×18 de l'audit de reprise dépend d'un rapport implicite de 1,45 | confirmé par reconstruction | 897,8 / (48 × 1,45) = 12,9 ; 1 239 / (48 × 1,45) = 17,8. | `AUDIT_REPRISE…:37-39,53-55` |
| 27 | D1 justifiée par les coûts float32 non épinglés | nuancé | La proposition D1 invoque surtout « ×270 de travail inutile » et « 1 728 bits » ; les coûts ×20 à ×70 apparaissent l. 65 et l. 92. La décision « entier 18 bits » est une décision utilisateur. | `AUDIT_REPRISE…:123-135` ; `PASSATION.md:36-37` |

### 9.2 Omissions ajoutées par la contre-vérification

- Le modèle de CPU de l'hôte local a changé (EPYC 9V74 le 14 septembre, EPYC 7763 aujourd'hui), et les reçus du 16 au 22 septembre ne le consignent pas (§ 5.9).
- Les mesures d'audit du 22 septembre sur le moteur courant (arbre `src` 54a6d581) :
  - `audits/lidar_rectangles_20260922` : 6 902 contextes de rectangles, 118,9 M paires ;
  - `audits/cascade_rectangles_20260922` : gains de filtrage ×2,55 à ×4,46 sur rectangles échantillonnés, sans atlas ni temps de trame ;
  - dépendances : artefact Actions 10704262200, dont la conservation est limitée, et archive zip hors dépôt (§ 5.12).
- Le pire régime (scène 02 K10) et les réductions de la phase 1-2 sur les trois scènes (§ 4.4).
- La lecture de la contention par `workers_timing_ms`, qui renverse l'interprétation de la charge pour l'identité u16 (§ 4.4).
- La ligne 1 mm mesurée avec le binaire de qualifications `failed` (§ 5.6).
- Une seule observation par configuration, contre l'exigence de répétitions du contrat (§ 5.15).
- L'empreinte mémoire du flux est minime (RSS de 13 à 15 Mo en local, 23 Mo sur G4), alors que la tour v7 atteint 15,5 Gio à 50k K10. La mémoire deviendra un sujet de mesure dès que l'aval existera.
