# Lentille 9 — Audits indépendants et canal de coordination de la v8 (version contre-vérifiée)

Cadre : `phase=exploration_v8_hors_registre` (audit de clôture pour ouverture v9), `backend=cpu_reference`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé. Lecture seule : aucune compilation, aucun CTest, aucun script du dépôt exécuté (seuls `git log/show/diff/rev-parse`, `grep`, `sha256sum`, `wc` et `python3` pour lire du JSON ou recalculer des rationnels). Référence : worktree détaché à `origin/main` **12294241**. Les éléments pris dans le worktree partagé `/workspaces/E-HGP` sont étiquetés **[non commis]**.

Ce document reprend `09_audits_independants.md` (premier auditeur de la lentille) après contre-vérification adversariale de chacune de ses affirmations. Les corrections sont intégrées au texte ; leur liste est dans la dernière section.

## Rôles

Rôles désignés d'après les en-têtes des fichiers et les sections du journal. Les métadonnées Git sont toutes au nom Ludwig-H et ne distinguent pas les acteurs.

| Étiquette | Où il écrit | Période active (commits) |
|---|---|---|
| Auditeur A | `morsehgp3D_v8/audits/DIALOGUE_COURANT.md`, `P0_*.md`, dossiers datés « auditeur A » | 13 sept. → 21 sept. 11:17 UTC (`32297105`) |
| Auditeur B | `DIALOGUE_AUDITEUR_B.md`, notes `*_20260914.md`, dossiers datés « Auditeur B » | auditeur 14–15 sept. (canal clos à `36bef318`), constructeur 17–20 sept. (tranches 19 à 21 : `8d615cfd`, `8190e7ab`, `3e94c868`), de nouveau auditeur le 21 sept. (dernier commit d'audit `ad9bbc9d`, 20:04 UTC), développeur ensuite |
| Auditeur complémentaire | `audits/morsehgp3D_v8_complementaire/` et sept sections du journal | 13 sept. seulement (7 commits, dernier `77b1abf0`, 21:07 UTC) |
| Auditeur(s) externe(s) sans canal | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` (cinq commits `docs(v8)`, 19–20 sept.) ; `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md`, `RECTANGLES_H_HA_HB_SUIVI_20260922.md`, `SYNTHESE_PRIORITES_LIDAR_20260922.md`, `lidar_rectangles_20260922/`, `collective_edge_20260922/`, `cascade_rectangles_20260922/` (treize commits `audit(v8)`, 22 sept.) | 19–20 et 22 sept. Commits horodatés +0200, hôtes AMD EPYC 9V74 et Intel Xeon 8370C, g++ Debian 14.2 : clone séparé, jamais le worktree partagé. N'écrit jamais dans le journal de coordination |
| Constructeurs et développeurs successifs | `morsehgp3D_v8/audits/ETAT_COURANT.md`, six rapports d'ouverture, `docs/`, `receipts/`, journal | ROOT/constructeur 13–15 sept. ; B 17–20 sept. ; un nouveau constructeur à partir du 20 sept. (tranches 22 à 34, briques float32, pilote sans sol, jusqu'à `204b0620`, 21 sept. 20:08 UTC) ; B développeur à partir du 21 sept. (déclaré 20:10 UTC, commit `8c050a33` à 21:07 UTC) jusqu'à `a74e90f2` (22 sept. 06:21 UTC) ; une reprise **[non commise]** le 22 sept., dont l'auteur appelle l'auteur de `a74e90f2` « le développeur précédent » |

Sources des changements de rôle : `audits/COORDINATION_MORSEHGP3D_V8.md:2155-2163` (17 sept.) ; `morsehgp3D_v8/PASSATION.md:6` (« Le constructeur a changé le 17 septembre, puis de nouveau à cette reprise du 20 septembre ») ; `morsehgp3D_v8/audits/ETAT_COURANT.md:6-12` (« L'auditeur B devient développeur », 20:10 UTC) ; `morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:10-14` **[non commis]**.

`morsehgp3D_v8/audits/ETAT_COURANT.md:3-4` le dit lui-même : « Audit constructeur […] Ce dossier n'est pas l'auditeur indépendant propriétaire des audits v7 ». `tools/check_docs.py:89-104` range six rapports parmi les documents constructeur : `FONDEMENTS_ET_OBJET`, `PERIMETRE_ET_PREUVES`, `WSPD_Q2_Q3_Q4`, `CONTRATS_ET_MESURES`, `IMPLEMENTATION_PARALLELISATION` et `ETAT_COURANT`. Ce rapport ne les traite que pour ce qu'ils disent de la v7.

## 1. Périmètre lu

### Lu intégralement

- `morsehgp3D_v8/audits/ETAT_COURANT.md` (770 lignes), `DIALOGUE_COURANT.md` (57), `DIALOGUE_AUDITEUR_B.md` (1 096), `SYNTHESE_PRIORITES_LIDAR_20260922.md` (97), `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md` (214), `RECTANGLES_H_HA_HB_SUIVI_20260922.md` (164), `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` (121), `VERROUS_MATHEMATIQUES_20260914.md` (199).
- `lidar_rectangles_20260922/README.md` et `probe.cpp`, le début de `run.py`, `cascade_rectangles_20260922/RESULTS.json`, `collective_edge_20260922/RESULTS.json` (empreintes et clés) et `.github/workflows/morsehgp3d-v8-lidar-audit.yml`.
- `morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md` (ajouté par la contre-vérification).
- `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` (234).
- **[non commis]** : la version indexée et la version de travail de `morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md`, le diff indexé de `morsehgp3D_v8/audits/ETAT_COURANT.md`, de `PASSATION.md` et du journal, et le diff non indexé du journal (section complémentaire de 53 lignes).

### Lu partiellement

- `PORTES_ET_TESTS_20260914.md` (§1, §2, §2 ter), `PROPAGATION_TEMOINS_20260914.md` (§1 à §2 bis), `CREDITS_TERMINAUX_20260914.md` (§1 à §3 bis), `REGIME_WSPD_20260914.md` (§2–3), `SEPARATION_20260914.md` (§1–2), `BUDGET_CONTRAT_50K_20260914.md` (§1–2), `P0_SOUS_RECTANGLES_ET_GROUPES.md` (§6), `q34_global_contract_20260921/README.md` et `SUPPORT_ET_CITRON.md` (§1 à §4), `front_options_lidar_20260920/README.md`, `CONTRATS_ET_MESURES.md` (§1–2).
- Les README des contrôles croisés de B (`q3_stream_crosscheck`, `q4_stream_crosscheck`, `q34_stream_crosscheck_t32/_t32_8k/_t33/_t34`, `README_Q4_BILATERAL*.md`) et `oracle_q3q4_20260915/README.md`.
- `audits/morsehgp3D_v8_complementaire/ETAT_COURANT.md` et `P0_GROUPES_RECOUVRANTS.md` (§1–2).
- `audits/COORDINATION_MORSEHGP3D_V8.md` (3 850 lignes) : plan complet des sections, lignes 1–40, 2 110–2 420, et relevés `grep`.
- `morsehgp3D_v8/receipts/ground_baseline_20260921/README.md` et son profil `gprof` (ajoutés par la contre-vérification).
- Tests : `p0_gate.cpp:250-270`, `exact_ball_gate.cpp:120-175`, `q34_seed_gate.cpp:310-335`, `q2_prepared_bounds_gate.cpp:262-285`.

### Vérifications directes faites

- Contre-exemple α=3 sur q4 recalculé en rationnels exacts. Poids barycentriques tous positifs. `ab` est l'arête maximale (carré 3 600). Pour z, H=608 et Ξ=1 036 800, et |z−o|² = 5222107613/4235364 > r².
- Empreintes SHA256 des quatre fichiers épinglés par `collective_edge_20260922/RESULTS.json` : les trois sources du dossier et la note `CERTIFICATS_COLLECTIFS`. Elles sont identiques (`73d2c463…`, `c3fef32f…`, `63f5588a…`, `3c83ef2b…`).
- `git rev-parse <c>:morsehgp3D_v8/src` vaut `54a6d58…` pour `a74e90f2`, `562d090c`, `ca73e96b`, `a5447d06` et `12294241` : aucun commit postérieur à `a74e90f2` ne touche `src/`.
- Recalcul de `cascade_rectangles_20260922/RESULTS.json` : masse totale 27 755 065 ; gain combiné 2,554 à 4,456 ; gain au-delà des plans seuls 1,016 à 1,345. La présélection seule donne **0,936 à 1,102**. Le nombre de `pair_searches` est identique en modes 0/2 et 1/3.
- Commits `src` après la reprise développeur : `748ec082`, `02987f18`, `0948d2d0`, `5224ff4e`, `5fdda963` et `a74e90f2`, soit **six** commits et non quatre.
- État du worktree partagé : HEAD `a74e90f2`, en retard de **13 commits** sur `origin/main` `12294241` (réf. distante rafraîchie par `fetch` à 19:45 UTC). Les treize commits en retard ne touchent aucun fichier indexé. Les fichiers d'audit du 22 septembre n'existent pas dans ce worktree.
- Index partagé : 89 fichiers indexés (47 tests, 12 `src/lanes`). S'y ajoutent **971 fichiers non suivis** sous `morsehgp3D_v8/` : 955 reçus `u18_resume_20260922`, 10 sources `src/` et 3 tests des brouillons float32 de l'ancien constructeur. On compte encore 11 fichiers non suivis du complémentaire, un `.partial.json` de B et des modifications non indexées (PASSATION, README, docs, journal).
- Recherche dans tout le dépôt des sources citées par les audits du 22 septembre (`cascade_filter.cpp`, `plan_gate.cpp`, `witness_representative.cpp`) et des archives `MorseHGP_*.zip` : **absentes**.

### Non lu

- Corps de `P0_TUBES_ET_RANGS.md`, `WSPD_Q2_Q3_Q4.md`, `IMPLEMENTATION_PARALLELISATION.md`, `PERIMETRE_ET_PREUVES.md`, `PREFIXES_TEMOINS.md`, `q34_prefix_order`, `q3_prefix_relay`, `q3_seed_block_power`, `q4_center_blocks`, `q4_local_sweeps`, `q4_kernel_composition` (au-delà des en-têtes) et des sept dossiers `q2_*` de A (en-têtes seulement).
- 15 des 21 notes complémentaires.
- La quasi-totalité des 2 920 fichiers suivis de `morsehgp3D_v8/audits` (39,2 Mo de contenu, 45 Mo sur disque).
- Le corps du journal hors des plages citées.
- Les archives « jointes à la conversation » et l'artefact GitHub Actions 10704262200 : inaccessibles depuis le dépôt.
- Aucun reçu n'a été rejoué (lecture seule).

## 2. Ce qui a été fait

### Avant la v8 — antécédent v7

- Le **lemme du citron** est déjà prouvé en v7, du 4 au 6 septembre (`morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md:5-15`, commits `4a1fc5fe` à `68713557`). La preuve passe par l'identité de variance du support positif et donne t=3 en q3, t=2 en q4, l'inégalité stricte et le seuil p<h_q. Elle justifie aussi les 64 coins par convexité séparée. A cite cette preuve le 13 septembre (`P0_SOUS_RECTANGLES_ET_GROUPES.md:183-186`). La note de A du 21 septembre ne la cite pas.

### 13–14 septembre — ouverture et P0

- ROOT/constructeur : six rapports d'audit de la v7 (base `dc57ffd5`) et cinq questions aux auditeurs (`audits/COORDINATION_MORSEHGP3D_V8.md:16-30`).
- Auditeur A : tubes et rangs (`P0_TUBES_ET_RANGS.md`, `dc246d6b`, 13 sept.), curseur Z unique du census (`f47559b1`, 13 sept.), **certificat de moments sur blocs** (`P0_SOUS_RECTANGLES_ET_GROUPES.md:174-229`, `7f4ba045`, 13 sept. 18:41), continuations bornées (`1bf806f0`, 14 sept.), partition des jobs du plan parent (`795a29dd`, 14 sept.).
- Auditeur complémentaire, sept passes le 13 sept. : quantificateurs, rails, résidu transverse, nappes 2D, rotations, **groupes recouvrants et capacités par ID** (`P0_GROUPES_RECOUVRANTS.md:10-40`, `65ac5ee6`, 18:40). Il juge aussi le census C++ q2 : 466 fixtures, 932 appels, sept mutants C++. Constats clos (`audits/morsehgp3D_v8_complementaire/ETAT_COURANT.md:129-139`) : affectation après panne, copie du propriétaire, alias du tampon d'entrée, admission de reçus mêlant Release et Debug.

### 14–15 septembre — premier front, chaîne q2

- Auditeur B :
  - régime du front WSPD pur v4 (`REGIME_WSPD_20260914.md`) et convention de séparation v8 encadrée par v4 s=14 et s=16 (`:103-114`) ;
  - budget du contrat 50k, verrous mathématiques (dont une seconde preuve du fuseau par Jung, `VERROUS_MATHEMATIQUES_20260914.md:73-95`) et campagne adversariale des portes ;
  - test de lentille réfuté, propagation des témoins chiffrée, crédits terminaux Pool, séparation s∈{8,10,12} ;
  - **chaîne q2 rejouée contre force brute à chaque tranche 8 à 18** (`chaine_q2_20260914/`), oracle q3/q4 i128, plafond de proposeur q2, bilan de la surproposition.
- Auditeur A : LiDAR KITTI08 (`lidar08_20260914/`), reprise de tâches du front (`front_tasks_20260914/`, commit d'audit `329e5b86`, contre le front publié `ba11e3ab`) et sept dossiers `q2_*` (front+census LiDAR, frère, complément, produit conjoint, raccord Pool, racines singleton, ordre des témoins).

### 17–20 septembre — B constructeur, A seul auditeur actif

- 17 sept. : B devient constructeur ; son canal est clos à `36bef318` (`audits/COORDINATION_MORSEHGP3D_V8.md:2155-2163`). B-constructeur demande deux fois une contrelecture « seule indépendante » à A et au complémentaire (`:2286-2291`, `:2372-2375`).
- 19–20 sept. : auditeur externe, facettes silencieuses et contrat de portage v7→v8 (cinq commits, de `ae98dbfa` à `c92aad13`).
- 20 sept. : A relit les tranches 20/21 (« cohérentes dans leur périmètre q2 », `front_options_lidar_20260920/README.md:7`), puis couvre q3/q4 (`q34_collectif_20260920`, `q4_center_blocks`, `q4_local_sweeps`, `q4_kernel_composition`). Un nouveau constructeur reprend (`COORDINATION:2393`). Le complémentaire ne répond pas.

### 21 septembre — B rouvert, contrôles croisés q3/q4

- B :
  - réponses aux questions du journal (`front_lanes_lidar_20260921/`) ;
  - rejeu indépendant des flux q3/q4 des tranches 31 à 34 : préfixes 1k/2k/4k à K5 et 1k/2k à K10 ; 8k seulement pour la tranche 32 ;
  - relais sans état, protocole spatial et validation bilatérale q4 sur les sept morceaux de la scène 0 (moteur de la tranche 34, `d6e1bd9e`) ;
  - relectures float32 (`74fb0a6a`, `f7b220c4`) et fixtures F1–F10.
- A : preuve support/citron et contrat global (`q34_global_contract_20260921/`, `462c29a1`), ordres figés, jointure graines×cellules q4, enveloppe de centres par bloc, relais compact de préfixe q3 (`32297105`).
- **Incident** : le commit d'audit B `4c3cdb0c` (305 fichiers) emporte 300 fichiers de la tranche 34 du constructeur présents dans l'index partagé (`DIALOGUE_AUDITEUR_B.md:26-45`).
- Soir : B devient développeur (`8c050a33`, 21:07 UTC, audit à neuf lentilles). Ces neuf lentilles sont des sous-agents du développeur, pas des auditeurs indépendants. L'une d'elles lance `ctest -N` dans quatre builds épinglés et réécrit leur `LastTest.log` (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:108-119`).
- Nuit du 21 au 22 : le développeur commet six changements de `src/`, jusqu'à `a74e90f2` (moteur 18 bits, 06:21 UTC). Aucune section du journal de coordination ne les annonce ; seul `docs/JOURNAL_DEVELOPPEMENT_20260921.md` les décrit.

### 22 septembre — reprise non commise et auditeur(s) externe(s)

- **[non commis]**, 07:00–10:56 UTC environ, base `a74e90f2` :
  - finalisation de 42 portes u18 et protection des API hors domaine ;
  - option `saturate_deep` de l'atlas, désactivée par défaut ;
  - « trois contrelectures » internes et une première capture Release R1 : 134 tests sur 136 passent, deux lecteurs de mutations échouent, la capture SAN est interrompue ;
  - une première trame sans sol entière à 1 mm (104,63 s mur, K5, W8).
- Externe (clone séparé) :
  - de `bf73e194` à `39c6b824` (10:36–10:40 UTC) : certificats collectifs d'arête et prototype autonome rejouable ;
  - `b1c91e44` : partage h+h_a+h_b et réemploi singleton (note seule, sources en archive) ;
  - de `7a121e44` à `a5447d06` : appelant natif LiDAR 1 mm, workflow Actions, capsule et mesures élargies locales ;
  - `12294241` (19:40 UTC) : synthèse et priorité à la « cascade de rectangles ».

Total : 166 commits touchent `morsehgp3D_v8/audits`, `audits/morsehgp3D_v8_complementaire` ou le journal, de `2b658cbe` inclus à `12294241`.

## 3. État par composant

| Objet d'audit | Statut | Preuve |
|---|---|---|
| Lemme du citron, α3=3, α4=2, arête maximale du support positif | **prouvé** trois fois (v7 le 4–6 sept. ; B par Jung le 14 sept. ; A par la variance le 21 sept.) ; **testé** q3 et q4 par instances ; porté | `morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md:5-15` ; `VERROUS_MATHEMATIQUES_20260914.md:73-95` ; `SUPPORT_ET_CITRON.md:34-63`. q3 : 13,2 M graines à 1k, 0 violation (`DIALOGUE_AUDITEUR_B.md:218-224`). q4 : 1 059 078 + 3 869 169 tétraèdres positifs sur toutes les paires rejetables à 1k, 0 violation (`q4_stream_crosscheck_20260921/README.md:46-50`). Code : `spindle/predicates.hpp:125` |
| α=3 appliqué à q4 | **réfuté** (contre-exemple entier u16, recalculé ici) | `DIALOGUE_AUDITEUR_B.md:142-151` |
| Seuils h_q = Kmax+2−q hors position générale | **testé** par un agent de B (250 nuages u16), scripts hors dépôt ; non gravé dans les fondements | `VERROUS_MATHEMATIQUES_20260914.md:19-37` |
| Proposition 6 / théorème 5 / K-MST | **faux en général** (fixture E5 ; 3 nuages sur 366 à K=2) | `VERROUS_MATHEMATIQUES_20260914.md:97-108` |
| Constante des tubes D≥10R | **prouvé** (A) + **testé** à la frontière (B : 1 105 configurations ; faux crédits dès D/R≈6,1 à 7,6) | `VERROUS_MATHEMATIQUES_20260914.md:188-199` |
| Tubes comme chemin général | **mesuré négatif** : 0 crédit sur nuage irrégulier | `CREDITS_TERMINAUX_20260914.md:44-64` |
| Test de lentille avant recherche | **mesuré négatif** : 0,4 à 1 % des recherches | `PROPAGATION_TEMOINS_20260914.md:20-44` |
| Propagation des témoins parent→enfants (q2) | **mesuré** (B) puis **porté** en option explicite (tranche 21, `3e94c868`, défaut inchangé) | résidu q2 ×0,40–0,53 hors amas et rangées, 0 rejet non sûr à n=600 (`PROPAGATION_TEMOINS_20260914.md:60-91`) ; `COORDINATION:2370` |
| Blocs Z certifiés le long de la descente | **mesuré, non tranché** : le front seul coûte plus cher (57,6 → 78,6 s à 32k) ; B renvoie la décision à une comparaison front+census jamais faite | `PROPAGATION_TEMOINS_20260914.md:113-139` |
| Pool terminal sur les gros facteurs | **mesuré** (B) puis **porté** (tranche 12, `ba11e3ab`) | résidu q2 amas 32k ÷38 (`CREDITS_TERMINAUX_20260914.md:73-117`) |
| Séparation s∈{8,10,12} | **testé** : même objet ; **mesuré** : coûts voisins ; s=8 confirmé | `SEPARATION_20260914.md:20-40` |
| Régime du front WSPD pur | **mesuré** (front v4, pas v8) : +≈90 rectangles/point par doublement de 8k à 256k | `REGIME_WSPD_20260914.md:67-88` |
| Chaîne q2 des tranches 8 à 18 | **testé** contre force brute exacte, 0 désaccord (9 lignes : masses de paires pour 3, nombres d'appels pour 6) | `DIALOGUE_AUDITEUR_B.md:1064-1080` |
| Flux q3/q4 des raccords 31–34 | **testé** (B, énumérations indépendantes) : préfixes 1k–4k de toutes les tranches, scan 0 à 8k pour la tranche 32 seulement ; coquilles q4 non comparées | `q3_stream_crosscheck_20260921`, `q4_stream_crosscheck_20260921`, `q34_stream_crosscheck_t32_8k_20260921/README.md:38-39` |
| Validité de chaque record q4 à l'échelle de la scène | **testé** (bilatéral) sur le moteur t34 (`d6e1bd9e`), u16 à 2 cm avec sol. Validité exhaustive. Complétude sur échantillon **mince** : sur la scène entière, 61 (K5) et 52 (K10) paires conservées, 1 et 22 boules énumérées | `README_Q4_BILATERAL.md` (ligne `full`), `README_Q4_BILATERAL_K10.md:48` |
| Énumération exhaustive q3 de la moitié x+ | **testé [non commis]** : reçu partiel, 528 575 records sonde = harnais, même SHA256 `574f4303…` | `q34_stream_crosscheck_spatial_20260921/Q3_STREAM_CROSSCHECK_SPATIAL_LARGE.partial.json` (1 ligne de `rows`) |
| Énumération exhaustive du quart x+y+ (q3 et q4) | **non vérifiable** : ligne observée, reçu jamais livré | `DIALOGUE_AUDITEUR_B.md:598-608` |
| Certificat collectif de moments (groupe fixe, 64 coins) | **prouvé** (A, 13 sept.), **re-prouvé** pour l'arête (22 sept.) ; **testé** (prototype du 22 sept.) ; porté seulement pour les familles de graine `abx` | `P0_SOUS_RECTANGLES_ET_GROUPES.md:174-229` ; `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md:25-59` |
| Capacités par ID ; triangle de paires = 2 crédits | **prouvé + testé borné** (complémentaire le 13 sept. ; triangle, cas Δ=2, le 22 sept.) ; non porté | `P0_GROUPES_RECOUVRANTS.md:10-40` ; `CERTIFICATS_COLLECTIFS…:63-92` |
| Garde `min{h, s+⌊2m/3⌋}<T` avant groupement | **prouvé** + **testé** (499 270 → 62 117 tests de paires, rejets identiques) | `CERTIFICATS_COLLECTIFS…:94-112` |
| Collectif d'arête sur LiDAR 1 mm | **mesuré hors dépôt** (archive) : 221 rejets q4 sur 763 arêtes | `lidar_rectangles_20260922/README.md:73-92` |
| Paire représentante (exclusion de blocs pour h) | **prouvé** (inclusion immédiate par définition de U comme intersection) ; **mesuré hors dépôt** ; seule, ×0,94 à ×1,10 sur le filtrage échantillonné | `lidar_rectangles_20260922/README.md:31-54` ; `cascade_rectangles_20260922/RESULTS.json` (mode 2) |
| Plans h+h_a+h_b sur gros rectangles | preuve de P0 (`docs/P0_CREDITS_LOCAUX.md:66-70`) ; **rejouable depuis le dépôt** pour les classes ≤ 65 536 paires (`probe.cpp`, modes 2–3, workflow) ; chiffres publiés (classes > 65 536) **hors dépôt** ; non porté | `lidar_rectangles_20260922/README.md:56-71` ; `probe.cpp:48-54` ; `run.py` |
| Réemploi singleton | **vérifié par lecture** (double appel `wspd_q34.cpp:407` puis `:487`) ; rejouable (mode 1 de `probe.cpp`) ; non porté | `RECTANGLES_H_HA_HB_SUIVI_20260922.md:29-42` |
| Cascade combinée | **mesurée sur échantillons** (×2,55 à ×4,46, somme de médianes par classe), sources de l'appelant combiné hors dépôt ; collectif exclu ; non portée | `SYNTHESE_PRIORITES_LIDAR_20260922.md:31-69` |
| Relais rectangle→paires sans état | **mesuré** (B), en comptes : 0,71–0,87 (q3) et 0,74–1,04 (q4) du travail ; non porté | `DIALOGUE_AUDITEUR_B.md:438-469` |
| Relais compact de préfixe q3 | **prouvé + testé** en modèle (A, 1 150 relais) ; non porté (aucune tranche 35) | `DIALOGUE_COURANT.md:8-41` |
| Couches duales q3 à T=K−1 | **prouvé valide**, **mesuré non rentable** sur ces régimes (coût ×1,5 à ×3,4 du census évité) | `DIALOGUE_AUDITEUR_B.md:280-311` |
| Briques float32 (clé, événements q4, prédicats) | **relues sans erreur** (B) ; dérivation Δ fournie ; options flottantes dans CMake | `DIALOGUE_AUDITEUR_B.md:650-720` ; `morsehgp3D_v8/CMakeLists.txt:52-66` (`92d74c13`) |
| Fixtures de dédoublonnage inter-voies et tétraèdre régulier | **partiellement gravées** : le tétraèdre (0,0,0),(1,1,0),(1,0,1),(0,1,1) et la sphère de B sont dans les portes ; le scénario « q3 rejette, q2 conserve » n'a pas de consommateur (aucun catalogue) | `tests/exact_ball_gate.cpp:132,142` ; `tests/q34_seed_gate.cpp:318` ; `tests/q4_family_gate.cpp:304` |
| Facettes silencieuses : sémantique `full_ball_tower.hpp`, MEB pivot4 canonique | **proposé** ; mesures hors dépôt (17 363 entrées, n=24/48) | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:8-90` |
| Six commits `src` du développeur (`748ec082`, `02987f18`, `0948d2d0`, `5224ff4e`, `5fdda963`, `a74e90f2`) | **aucun audit indépendant**. Exécutions indépendantes sur le moteur 18 bits limitées au filtre de témoins : selftest Actions (70 672 comparaisons), comparaisons scalaire et « plans forcés » de la cascade (540 216 + 984 960, sources en archive), 13 046 contextes aux masques identiques. Rien sur le rejet q3 par l'atlas, la file de plages ou les flux q3/q4 | `lidar_rectangles_20260922/README.md:23,41` ; `cascade_rectangles_20260922/RESULTS.json` (`checks`) ; **[non commis]** : « trois contrelectures » internes et « deux risques hors domaine corrigés » (diff indexé de `ETAT_COURANT.md`) |
| Tour FULL, catalogue, intérieurs, fold v8 | **manquant** : aucun audit ne peut porter dessus | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:57,69` |

## 4. Chiffres clés

| Grandeur | Valeur | Source épinglée | Réserve |
|---|---|---|---|
| Rectangles du front pur v4, uniforme s=8 | 3 435 133 (8k) → 219 063 683 (256k) ; 429 → 856 par point | `REGIME_WSPD_20260914.md:69-76`, reçu `wspd_regime_20260914/` | front v4 sans élimination ; 128k et 256k hors tailles d'intérêt |
| Convention s=8 de la v8 | encadrée par v4 s=14 et s=16 | `REGIME_WSPD_20260914.md:103-114` | encadrement, pas équivalence |
| Plafond d'un proposeur ponctuel (LiDAR, 18 exécutions) | 82–91 % de la masse q3, 75–89 % en q4 ; fenêtre 2K : 36–56 % (q3), 6–55 % (q4) | `DIALOGUE_AUDITEUR_B.md:56-66`, reçu `front_lanes_lidar_20260921/` | sources `c5308651`, u16 à 2 cm avec sol |
| Masse de graines après rejet de paire avant cover | ÷87 à ÷1 519 | `DIALOGUE_AUDITEUR_B.md:79-81` | 2 000 paires tirées par voie et exécution ; ×387 mesuré ensuite sur le moteur réel à 4k/K5 (`:230-232`) |
| Graines du moteur t31, K5 | 3,68 M → 21,9 M → 121,7 M (1k→4k), soit n^2,5 selon B | `DIALOGUE_AUDITEUR_B.md:227-230` | trois petites tailles : oracle, pas pente |
| Flux t32 à 8k, scan 0 | K5 : 93 914 q3 + 10 756 q4 ; K10 : 409 195 q3 + 116 985 q4 ; identiques aux énumérations | `q34_stream_crosscheck_t32_8k_20260921/README.md:38-39` | un scan ; coquilles q4 non comparées |
| Records q4 valides, scène 0 entière (119 142 sites, u16 2 cm) | 190 405 (K5) et 2 158 063 (K10), 0 invalide | `README_Q4_BILATERAL.md`, `README_Q4_BILATERAL_K10.md:48` | moteur t34 ; complétude : 1 (K5) et 22 (K10) boules énumérées sur la scène |
| Moitié x+ (59 953 sites), q3 exhaustif K5 | 528 575 records identiques | **[non commis]** `…SPATIAL_LARGE.partial.json` | reçu partiel, jamais commis |
| Plateaux cosphériques q4, quart x+y+ | 373 (28 854 présentations pour 28 481 boules) | `DIALOGUE_AUDITEUR_B.md:600-606` | **non vérifiable** (reçu non livré) |
| Relais sans état | travail total 0,71–0,87 (q3), 0,74–1,04 (q4) de la référence | `relais_temoins_20260921/README.md` ; `DIALOGUE_AUDITEUR_B.md:460-461` | comptes hétérogènes, part « paires » extrapolée, pas de temps |
| Couches duales q3 | graines/m = 0,15–0,21 contre un seuil de rentabilité de 0,27–0,53 | `DIALOGUE_AUDITEUR_B.md:296-306` | modèle de coût unitaire |
| Repli hull SharedPrefix float32 | 843 / 1 642 préparations (51 %), 85 intersections vides | `DIALOGUE_AUDITEUR_B.md:854-855` | fixtures de capture, pas une trame |
| Surproposition 2K (B, copie patchée) | temps q2 à 42–68 % de la référence hors rangées | `DIALOGUE_AUDITEUR_B.md:1086-1088` | copie d'audit |
| Options q2, LiDAR 50k, scan 0 (A) | 9,604 s → 5,307 s (`{2,16,true}`) → 4,774 s (`{4,all,true}`) ; visites census 313,9 M → 125,6 M → 77,8 M | `front_options_lidar_20260920/README.md:43-48` | mono, hôte partagé, médiane de 3 ; plages larges (4,750–8,710 s pour 4K) |
| Pool terminal, amas 32k | résidu q2 460,08 M → 12,18 M (÷38) ; survivantes Pool = 22× la vérité | `CREDITS_TERMINAUX_20260914.md:73-117` | résidu, pas temps de chaîne |
| Propagation des témoins, uniforme 32k | résidu q2 ×0,40, rectangles ×0,70 | `PROPAGATION_TEMOINS_20260914.md:82` | copie instrumentée de `da366f7f` |
| Tour FULL v7 à 50k (G4) | K1..10 : 418,873 s ; K1..5 : 33,853 s ; constructeur FULL mono 389,7 s | `CONTRATS_ET_MESURES.md:41-47` ; reçu v7 `full_ball_scale_gpu_20260910/…/cpu_n50000_k10_s8.summary.json` (`row.total_s` = 418,872898) | seule tour **FULL** du dépôt ; les v4–v6 produisaient des forêts horizontales K=1..10 de sémantique `verified_events_only` |
| Plancher de sortie v7 | 27 273 218 nœuds FULL (reçu v7), 1,75 Go à 64 o/nœud ; écriture 62,7 ms | `BUDGET_CONTRAT_50K_20260914.md:44-51` | nœuds vérifiés ; les 62,7 ms sont **non vérifiables** (aucun reçu) |
| Base sans sol épinglée, scène 00 (u16 2 cm, `92d74c13`) | K5 W8 : 298,0 s mur ; K10 W8 : 911,1 s. `gprof` K5 W1 : atlas q4 ≈ 57 %, census q3 ≈ 28 %, filtres de témoins ≈ 5–8 % | `receipts/ground_baseline_20260921/README.md:24-32` ; `gprof_scene00_k5_w1/gprof_flat_K5_W1.txt:6-24` | hôte partagé de 8 vCPU ; `gprof` sur un build `-pg` séparé ; mesure antérieure à `0948d2d0` et à `a74e90f2` |
| Écart au contrat, sans sol, 2 cm (développeur) | K5 W8 279,3 s ; ×13 à ×18 de travail en moins requis à K5, ×50 à ×65 à K10 | `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md:23-55` | **non vérifiable** : diagnostic non épinglé ; utiliser la base épinglée ci-dessus |
| Première trame sans sol entière à 1 mm, 08/000000, K5 W8 | 104,63 s mur, 812,82 s CPU ; atlas : 3,252 G bornes de blocs, 7,316 G tests ponctuels | **[non commis]** `REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` (version de travail) | une répétition, hôte partagé, reçu non commis |
| Cascade LiDAR 1 mm (960 contextes, 27 755 065 paires) | ×2,55 à ×4,46 (combiné) ; ×1,02 à ×1,35 au-delà des plans seuls ; présélection seule ×0,94 à ×1,10 | `cascade_rectangles_20260922/RESULTS.json` (recalculé) | somme des médianes par classe sur un échantillon, pas un temps de trame ; référence 000100/K10 = 11,59 ms seulement ; sources de l'appelant **hors dépôt** |
| Présélection par paire représentante, rectangles > 65 536 paires | temps du filtre commun ×5,13 à ×11,10 ; visites 413 200 → 32 088 (000200/K5) | `lidar_rectangles_20260922/README.md:41-52` | **non vérifiable** (archive) ; recherche de h seulement |
| Plan sélectif, tous les rectangles > 65 536 paires | ×2,55 à ×3,95 ; recherches 8 505 554 → 2 706 166 (000200/K10) | `lidar_rectangles_20260922/README.md:62-71` | **non vérifiable** pour ces classes (archive) |
| Collectif sur les arêtes LiDAR | 1,95–3,35 ms payés contre 3,28–60,66 ms de q4 évitable par (trame, K) ; 221 rejets sur 763 arêtes | `lidar_rectangles_20260922/README.md:77-92` | une répétition, stratifié, **non vérifiable** ; l'atlas sert aussi q3 |
| Collectif, corpus exact | 4 891 boules, 8 736 requêtes ; 280 rejets individuels contre 522 avec groupes ; 123 rejets supplémentaires non vides ; 0 faux minorant | `CERTIFICATS_COLLECTIFS…:140-147` | rejouable (`collective_edge_20260922/`, empreintes vérifiées) |
| Microbenchmark collectif | 1,5–3,5 µs contre 0,175–0,19 µs par requête individuelle | `CERTIFICATS_COLLECTIFS…:158-172` | pools présélectionnés hors chrono |
| Plans h+h_a+h_b, synthétique n≤1024 | ×0,97 (sphère K5) à ×14,07 | `RECTANGLES_H_HA_HB_SUIVI_20260922.md:72-80` | hors tailles d'intérêt ; **non vérifiable** (archive) |
| MEB pivot4 canonique | 0,262–0,271 du temps de référence ; 0,174–0,184 à K10 | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:73-76` | microbenchmark hors dépôt, n=24/48 : **non vérifiable** |
| Fichiers d'audit v8 à `12294241` | 66 Markdown (12 172 lignes), 2 920 fichiers, 39,2 Mo (45 Mo sur disque) ; complémentaire : 21 Markdown (2 385 lignes, 92 fichiers) ; journal : 3 850 lignes | `git ls-files`, `wc` | — |
| Reçus v8 suivis | 16 016 fichiers, 979 Mo | `git ls-files morsehgp3D_v8/receipts` | poids du dépôt, hors lentille stricte |
| Commits touchant les chemins d'audit v8 | 166 (de `2b658cbe` inclus à `12294241`) | `git log` | — |
| État de l'index partagé | 89 indexés (47 tests, 12 `src/lanes`) ; 971 non suivis sous `morsehgp3D_v8/` ; HEAD en retard de 13 commits sur `origin/main` | `git diff --cached`, `git status`, `git log HEAD..origin/main` | **[non commis]** |

## 5. Défauts, risques et dettes

### Gravité haute

**H1 — La dernière priorité recommandée n'est pas rejouable et vise le mauvais étage d'après les seules mesures épinglées.**

- La synthèse du 22 septembre recommande de porter « d'abord » les deux premiers étages de la cascade (`SYNTHESE_PRIORITES_LIDAR_20260922.md:85-88`).
- Son appelant combiné, sa présélection par paire représentante et ses mesures élargies ne sont connus que par leurs SHA256 (`cascade_rectangles_20260922/RESULTS.json`, clés `source_sha256` et `raw_sha256`). Ils renvoient à des archives « jointes à la conversation » (`SYNTHESE…:67-69` ; `lidar_rectangles_20260922/README.md:105`). Même chose pour tout `RECTANGLES_H_HA_HB_SUIVI_20260922.md` (`:10-13`) et pour le prototype MEB des facettes silencieuses.
- Seuls les plans et le réemploi singleton ont une sonde rejouable dans le dépôt (`lidar_rectangles_20260922/probe.cpp`, modes 0–3, classes ≤ 65 536 paires, 24 échantillons). La capsule Actions expire vers le 6 octobre 2026 (`retention-days: 14`, workflow ligne 85).
- Les données ne soutiennent pas la priorité donnée à la cascade :
  - Selon `RESULTS.json`, la présélection seule ne gagne rien sur le filtrage échantillonné (×0,94 à ×1,10). Le nombre de recherches de paires y est identique avec et sans elle ; presque tout le gain vient des plans et du réemploi singleton.
  - La cascade conserve les mêmes arêtes : elle « ne supprime pas d'atlas » (`SYNTHESE…:60-61`).
  - Le seul profil de temps épinglé (`receipts/ground_baseline_20260921/gprof_scene00_k5_w1/`) attribue ≈ 57 % du temps à l'atlas q4, ≈ 28 % au census q3 et seulement ≈ 5–8 % aux filtres de témoins. Ce profil date d'avant `0948d2d0` et vaut pour 2 cm.
  - Sur ce profil, un gain même infini sur le filtre ne rapporterait qu'environ ×1,05 à ×1,09 sur l'appel entier (loi d'Amdahl).
- Implication v9 : ces chiffres sont des hypothèses. La part des étages sur des trames entières à 1 mm doit être mesurée avant tout choix d'ordre.

**H2 — Aucun audit indépendant des six commits `src` du 21–22 septembre.**

- Le développeur (ex-B) a commis :
  - `748ec082` : atlas à l'échelle 2^20 ;
  - `02987f18` : allocation des fragments ;
  - `0948d2d0` : rejet exact des graines q3 par l'atlas q4, qui change une décision géométrique ;
  - `5224ff4e` : file bornée de plages de rectangles ;
  - `5fdda963` : chronos par worker dans `wspd_q34.cpp` ;
  - `a74e90f2` : moteur 18 bits, 86 fichiers.
- Couverture indépendante :
  - Le complémentaire est muet depuis le 13 septembre et A depuis le 21 septembre 11:17 (`32297105`).
  - Les audits du 22 septembre déclarent n'avoir « aucun moteur modifié » et ne citent aucun de ces commits.
  - Leurs exécutions sur la bibliothèque 18 bits ne jugent que `filter_q34_witnesses` : selftest Actions (70 672 comparaisons), comparaisons de la cascade (540 216 + 984 960, sources en archive) et 13 046 contextes aux masques identiques.
  - Rien ne juge le rejet q3 par l'atlas, la file de plages ni les flux q3/q4 depuis la tranche 34.
- La reprise **[non commise]** invoque « trois contrelectures » internes, non indépendantes. Elle déclare « deux risques hors domaine corrigés aux frontières publiques » (diff indexé de `morsehgp3D_v8/audits/ETAT_COURANT.md` ; `REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:23-35`).
- Sa capture R1 a échoué sur deux lecteurs de mutations (134/136) et la capture SAN a été interrompue (version de travail).
- Les bornes 18 bits annoncées (puissance q3 < 2^116, comparaisons q4 < 2^104, orientation < 2^121) sont des projections du 21 septembre, antérieures au code (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:129-131`). Elles doivent être contre-prouvées sur le code de `a74e90f2` avant tout port.

**H3 — Le risque d'emporter le travail d'un autre acteur est actif et plus large que l'index.**

- Précédent : `4c3cdb0c`, 305 fichiers dont 300 de la tranche 34 (`DIALOGUE_AUDITEUR_B.md:26-45`).
- Contenu actuel du worktree partagé : 89 fichiers indexés et 971 fichiers non suivis sous `morsehgp3D_v8/`. Parmi eux, 10 sources `src/` et 3 tests des brouillons float32 de l'ancien constructeur. La question Q1 de `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:217-220` sur leur sort reste sans réponse.
- S'y ajoutent 11 fichiers non suivis du complémentaire, un reçu partiel de B et des modifications non indexées de quatre documents et du journal. Ces fichiers appartiennent à au moins quatre acteurs.
- Le HEAD local (`a74e90f2`) est en retard de 13 commits sur `origin/main`.
- Conséquence : un commit d'ouverture v9 fait depuis ce worktree mélangerait les acteurs, ou partirait d'une base périmée.

**H4 — Les recommandations du 22 septembre ne sont ni reçues ni arbitrées. Ce n'est pas un refus.**

- Aucun document constructeur ne les cite (`README.md`, `PASSATION.md`, `docs/`, `AGENTS.md`, commis ou **[non commis]** ; `grep` sur les deux arbres).
- Chronologie :
  - Les audits ont été poussés depuis un clone séparé entre 10:36 et 19:40 UTC.
  - Le worktree partagé ne les contient pas.
  - La réf. `origin/main` n'y a été rafraîchie qu'à 19:45 UTC.
  - La reprise non commise a été écrite avant ou pendant leur arrivée (dernières écritures vers 10:35–10:56 UTC).
- Les deux directions ne sont pas exclusives : la cascade vise le filtre de témoins, la saturation d'atlas vise l'atlas q4. Il reste pourtant à trancher leur ordre. Le seul profil épinglé désigne l'atlas comme poste dominant (H1).
- Canal : le journal désigné par `AGENTS.md:200` pour « le dialogue et la coordination d'index » n'a reçu aucune section de l'auditeur externe. Il n'en a reçu aucune non plus pour les six commits `src` du développeur, puisque le dernier commit du journal est `204b0620`.

### Gravité moyenne

**M1 — Travail d'audit non publié et références mortes.**

- Le complémentaire a laissé onze fichiers non suivis :
  - deux notes, `P0_CONSTANTES_CENSUS_Q2.md` (13 sept. 21:23) et `P0_IDENTITES_NUAGE_ET_RECTANGLES.md` (21:25) ;
  - deux reçus JSON, six scripts et sondes, une archive d'état.
- Il a aussi laissé une section de 53 lignes : c'est une modification non indexée d'un fichier suivi (le journal, vers la ligne 1 115). Chaque réservation constructeur l'exclut (`COORDINATION:2119`, `:2223`, `:2234` et suivantes).
- `P0_IDENTITES_NUAGE_ET_RECTANGLES.md:19,73-74` cite `CLOUD_RECTANGLE_IDENTITY_CHECKS.json`, absent même du worktree.
- B a annoncé des reçus d'énumération exhaustive des quarts et de la scène (`DIALOGUE_AUDITEUR_B.md:593-610`). Seul un reçu partiel **[non commis]** existe. Il contient néanmoins la ligne « moitié x+ » avec des empreintes sonde et harnais identiques (528 575 records).

**M2 — Revendications de nouveauté non tracées, en chaîne.**

- Le « nouveau résultat » du triangle (`CERTIFICATS_COLLECTIFS…:63`) est le cas Δ=2, trois groupes, de la règle de capacités du complémentaire, qui donne un crédit ⌈3/2⌉=2 (`P0_GROUPES_RECOUVRANTS.md:24-28`, 13 sept.).
- Le certificat d'arête reprend le certificat de moments de A à poids unitaires, y compris les 64 coins avec un groupe commun (`P0_SOUS_RECTANGLES_ET_GROUPES.md:200-205`, 13 sept.).
- Le lemme du citron « prouvé » par A le 21 septembre (`SUPPORT_ET_CITRON.md`) est déjà écrit en v7 (`morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md:5-15`), que A citait lui-même le 13 septembre.
- Aucune de ces sources n'est citée par les documents plus récents. Les preuves concordent, donc le risque porte sur la traçabilité et le double travail, pas sur l'exactitude.

**M3 — Rejouabilité liée à la machine locale.**

- Le lecteur de A « contrôle aussi les binaires et entrées locaux ignorés par Git ». La reconstruction exige « les deux bibliothèques 30 épinglées », qui ne sont pas copiées dans Git (`q34_global_contract_20260921/README.md:108-113`).
- Les reçus spatiaux embarquent des chemins absolus vers un `data/` ignoré (`DIALOGUE_AUDITEUR_B.md:556-558`).
- Le champ `git_commit` des captures `float32_identity_20260921` désigne un commit où les sources testées n'existent pas (`:666-670`).
- La reprise **[non commise]** le reconnaît : « les reçus seuls ne sont pas une archive d'exécution autonome ».

**M4 — Lacunes de portes toujours ouvertes.**

- La fixture « maximum en z = 1,5 » de `classify_witness_block` reste non décisive (`morsehgp3D_v8/tests/p0_gate.cpp:260-262`, inchangée depuis `85015a8c`).
- La fixture décisive n'a pas été ajoutée : A={(0,0,0)}, B={(3,0,0)}, Z=[0,2]×[0,3]×{0}, attendu `h_max4=9` et `Uncertain` (`PORTES_ET_TESTS_20260914.md:76-85`).
- `q2_prepared_bounds_gate.cpp:262-275` couvre le même piège pour un autre chemin, pas pour `classify_witness_block`.
- Les lacunes 2.3, 2.4, 2.6 et 2.8 sont déclarées « inchangées » au 14 septembre (`:175-178`). La 2.8 dit que 16 tests sur 34 échouent sous un autre nom de dossier, ce qui compte pour une copie en v9. Aucune clôture ultérieure n'a été trouvée, sans vérification exhaustive.

**M5 — Méthode de croissance spatiale critiquée et non corrigée.**

- B montre qu'un compteur exactement linéaire reçoit des exposants de 0,78 à 1,27 selon la relation parent/enfant (`DIALOGUE_AUDITEUR_B.md:504-514`).
- L'état constructeur publie toujours des exposants par relation (`morsehgp3D_v8/audits/ETAT_COURANT.md:108-110`, 2,502 et 2,332), avec la précision « aucun sous-quadratique global ».
- `docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md` ne mentionne ni exposant poolé ni axe densité.

**M6 — Indépendance érodée par la rotation des rôles.**

- B a été auditeur, constructeur, auditeur puis développeur en huit jours.
- Trois autres changements de titulaire ont eu lieu (20 sept., 21 sept. au soir, 22 sept. **[non commis]**).
- Les tranches 19–21 n'ont été relues que par A. B-constructeur l'a écrit : « ses relecteurs sont des instances sans mémoire du chantier, pas un troisième regard humain » (`COORDINATION:2372-2375`).
- Depuis le 21 septembre au soir, aucun auditeur nommé et actif ne couvre le développement.

**M7 — Les rapports d'auditeurs échappent à `check_docs`, par doctrine.**

- `tools/check_docs.py:89-104` n'inclut que six rapports constructeur de `morsehgp3D_v8/audits/`. Le commentaire `:90-91` justifie cette exclusion : les rapports d'auditeurs ne sont pas reformatés par le script.
- Conséquence : `CERTIFICATS_COLLECTIFS…:29-31, 36-39, 49-52, 100-102` utilise des équations `\[ … \]` sur plusieurs lignes physiques et `\sqrt\alpha` sans accolades.
- B valide ses propres fichiers par `validate` (`DIALOGUE_AUDITEUR_B.md:1058-1059`) ; les autres auditeurs ne le font pas.
- Étendre le contrôle bloquant aux auditeurs contredirait la doctrine ; un lint non bloquant, lancé par l'auditeur lui-même, la respecte.

**M8 — Écriture d'auditeur hors de son dossier.**

- Le workflow `.github/workflows/morsehgp3d-v8-lidar-audit.yml` (`562d090c`, `ca73e96b`) a été ajouté par l'audit du 22 septembre.
- Il est en lecture seule, conforme à `AGENTS.md:457` : `permissions: contents: read`, actions épinglées par SHA, `git diff --exit-code` sur `src`, `bench`, `receipts`, pas de GCP.
- Il sort pourtant du périmètre `audits/` fixé par `AGENTS.md:200`. C'est le premier workflow qui construit la v8 ; `morsehgp3d-v7.yml` existait déjà pour la v7. `CLAUDE.md` ne décrit ni l'un ni l'autre.

**M9 — Validation bilatérale mince à l'échelle et moteur ancien.**

- Toute la validation à l'échelle d'une trame porte sur le moteur t34 (`d6e1bd9e`, u16 2 cm, avec sol).
- La direction « complétude » ne repose que sur 1 (K5) et 22 (K10) boules énumérées pour la scène entière.
- Aucune validation n'existe pour le moteur 18 bits sur des trames sans sol à 1 mm.

**M10 — Oracles d'audit bornés au profil u16.**

- L'oracle q3/q4 i128 de B annonce des intermédiaires « au plus 2^106 » et « au plus 2^89 » pour u16 (`oracle_q3q4_20260915/README.md:18-20`).
- Les harnais de flux et le prototype collectif (qui déclare, lui, M=262143) ont des largeurs propres.
- Avant tout réemploi en v9 sur u18, chaque juge doit requalifier ses largeurs. Sinon il perd son statut de vérité.

### Gravité basse

- **L1** — `receipts/lidar_global_20260921/global_vwtz76da.tar.gz` (28 256 326 octets) reste versionnée malgré la demande de B (`DIALOGUE_AUDITEUR_B.md:1011-1014`). Elle est entrée dans l'historique à `4dbe3024` : on peut cesser de la copier, pas l'effacer sans réécrire `main`. Les reçus v8 suivis pèsent au total 979 Mo pour 16 016 fichiers.
- **L2** — Des chronos ont été pris sous charge concurrente :
  - par les harnais de B eux-mêmes (1,74 cœur effectif sur 4, `DIALOGUE_AUDITEUR_B.md:528-532, 566-571`) ;
  - par les captures du constructeur (13 des 36 chronos de `a005f8aa`, `:876-882`) ;
  - pour la scène 02 de la base sans sol épinglée (`receipts/ground_baseline_20260921/README.md:37-45`).
  Seuls les compteurs sont lisibles.
- **L3** — États périmés :
  - `DIALOGUE_AUDITEUR_B.md:3-6` dit « Canal rouvert » alors que B est développeur ;
  - l'état complémentaire est daté du 13 septembre ;
  - `morsehgp3D_v8/audits/ETAT_COURANT.md` est un état constructeur rangé dans le dossier d'audit.
- **L4** — Patchwork++ (BSD-2) et 111 Mo de dérivés KITTI (CC BY-NC-SA) sous `receipts/` ne figurent pas dans la section Licences (`README.md:94-96`). La question Q3 de `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:224-227` n'a pas de réponse écrite.
- **L5** — Le diagnostic de la porte G4 R2 (350,6 s sans sortie, tuée) n'a jamais été établi par une trace (`DIALOGUE_AUDITEUR_B.md:983-995`).
- **L6** — Incident de processus d'audit : une lentille de l'audit développeur a lancé `ctest -N` dans quatre builds épinglés et perdu trois journaux CTest bruts (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:108-119`). La règle « ne jamais lancer `ctest` dans un build épinglé » n'est écrite que dans ce document.

## 6. Questions ouvertes

### Demandes des auditeurs restées sans réponse ou sans suite

| Demande | Auteur, source | État constaté |
|---|---|---|
| Porter les deux premiers étages de la cascade dans une option unique, puis mesurer l'appel q3/q4 complet sur trois trames | externe, `SYNTHESE_PRIORITES_LIDAR_20260922.md:83-97` | non reçue par le worktree partagé (H4) |
| Essayer un collectif d'arête avant `Q34EdgeCover::make`, abandonné si « probabilité × coût évité ≤ coût du filtre » | externe, `CERTIFICATS_COLLECTIFS…:174-194` | non reçue |
| Porter une variante MEB pivot4 canonique dans le résolveur réel ; rejouer Γ/plateaux et census→FULL K10 | externe, `FACETTES_SILENCIEUSES…:106-115` | sans objet en v8 (pas d'aval) ; ouverte |
| Fixture décisive « maximum en z » ; lacunes 2.3, 2.4, 2.6, 2.8 | B, `PORTES_ET_TESTS_20260914.md:76-160` | 2.2 vérifiée ouverte ; les autres non clôturées à ma connaissance |
| Graver les fixtures de dédoublonnage inter-voies et le tétraèdre régulier | B, `VERROUS_MATHEMATIQUES_20260914.md:65-71, 93-95` | **partiellement fait** (`exact_ball_gate.cpp:132,142`, `q34_seed_gate.cpp:318`) ; rétention inter-voies sans consommateur |
| Décider le domaine exact des entrées non régulières (fixtures AB/ABC, ABCZ, carré) ; porter le « graphe daté sur les naissances » comme source épinglée | B, `VERROUS_MATHEMATIQUES_20260914.md:110-134` | ouverte |
| Décider le format de nœud de sortie avant de viser 100 ms | B, `BUDGET_CONTRAT_50K_20260914.md:42-51` | ouverte |
| Exposant poolé, dispersion entre frères, axe densité ; convention de coupe x ≥ −0,01 m | B, `DIALOGUE_AUDITEUR_B.md:504-560` | ouverte (M5) |
| Taux de repli sur mantisses aléatoires, famille à supports croissants, mutants compilés sur `float32_q3_block.cpp`, planchers `gram_unresolved` | B, `DIALOGUE_AUDITEUR_B.md:690-700, 845-868` | non vérifié ; la voie float32 globale n'est plus poursuivie (D1) |
| Découpler les campagnes sans sol des labels ; ne pas régler sur la séquence 08 | B, `DIALOGUE_AUDITEUR_B.md:903-913` | repris par écrit (`docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md:143-146, 177-179`) ; aucune autre séquence mesurée |
| Sortir l'archive de 28 Mo de Git | B, `DIALOGUE_AUDITEUR_B.md:1011-1014` | ouverte (L1) |
| Fixture de coquille répétée (sphère de 398 sites, 5 supports) pour le catalogue ; API multi-seuil | complémentaire, `ETAT_COURANT.md:21-30` | ouverte (pas de catalogue) |
| Sort des brouillons float32 non suivis (session concurrente) | développeur, `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:217-220` (Q1) | ouverte : les fichiers sont toujours non suivis |

Demandes de B **closes** et vérifiées :

- clé de fixture du contrat 31 corrigée (`docs/Q34_GLOBAL_ET_LIDAR_20260921.md:340-343`) ;
- sémantique de `owner_tests` documentée (`docs/Q34_TEMOINS_INDEXES_ET_CENSUS_BOITES_20260921.md:117-120`) ;
- options flottantes dans CMake (`92d74c13`) ;
- mutant d'égalité W3/W4 tué (`PORTES_ET_TESTS_20260914.md:163-167`) ;
- intermittence de `mhgp8_campaign_gate` corrigée (`:168-170`) ;
- formulation du citron corrigée dans l'état constructeur (`morsehgp3D_v8/audits/ETAT_COURANT.md:192-195`).

Le chiffre « 210 987 arêtes » du journal n'apparaît plus dans les documents courants. La valeur 210 987 se retrouve toutefois comme `input_pair_mass` à 1k/K10 dans les reçus de B (`q34_stream_crosscheck_t32_20260921/Q34_STREAM_CROSSCHECK_T32.json`, `rows[3]`, et les mêmes reçus t33/t34). Les « 83,307 M incidences » et « 10,5 s » restent sans reçu.

### Questions du constructeur aux auditeurs restées sans réponse

| Question | Source | État |
|---|---|---|
| La construction Morton a-t-elle la borne de décomposition nécessaire, ou faut-il un fair-split qualifié ? | ROOT, `COORDINATION:16-18` | B mesure la croissance (+90 rectangles/point par doublement) sans en établir la cause (`REGIME_WSPD_20260914.md:78-88`) ; pas de preuve |
| Obligations minimales pour transporter la contraction parallèle vers les plateaux, contributions et verticales (graphe daté) | ROOT, `COORDINATION:22-25` | sans réponse d'auditeur v8 |
| Représentation implicite des sorties quadratiques : requêtes et coûts d'expansion | ROOT, `COORDINATION:26-28` | seulement le plancher de sortie de B ; pas de contrat |
| Contrelecture des tranches 20/21 par A ou le complémentaire | B-constructeur, `COORDINATION:2286-2291, 2372-2375` | faite par A seul (`front_options_lidar_20260920`) |
| Questions q3/q4 de la reprise du 20 septembre et des tranches 22–30 à A et au complémentaire | constructeur, `COORDINATION:2408-2420` et sections suivantes | A a répondu en partie par ses dossiers q3/q4 ; le complémentaire jamais |
| Le census q3 peut-il réutiliser seulement les feuilles exactes de l'atlas saturant, avec repli global K−2 ? | reprise, journal **[non commis, indexé]** | sans réponse ; posée « pour la prochaine revue indépendante » |

## 7. À porter en v9 et à ne pas reprendre

### À porter (quoi, où, pin, pourquoi)

| Quoi | Où le lire | Pin | Pourquoi |
|---|---|---|---|
| Lemme du citron et ses fixtures d'égalité : tangence stricte, α4=2, contre-exemple α=3 sur q4 | preuve source `morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md:5-15` ; `morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md` ; `DIALOGUE_AUDITEUR_B.md:142-151` | v7 `68713557`, v8 `12294241` | c'est le rejet qui divise la masse de graines par 87 à 1 519 ; son contre-exemple rend un mauvais α détectable ; citer la source v7 |
| Certificat de moments d'un groupe fixe, extension aux 64 coins avec le **même** groupe, contre-exemple aux coins indépendants | `P0_SOUS_RECTANGLES_ET_GROUPES.md:174-229` ; `CERTIFICATS_COLLECTIFS…:114-130` | `12294241` | seul moyen connu de rejeter quand aucun témoin ponctuel n'existe (rangées, amas) |
| Règle de capacités par ID (triangle = cas Δ=2) ; garde `min{h, s+⌊2m/3⌋}<T` | `audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md` ; `CERTIFICATS_COLLECTIFS…:63-112` | `12294241` | crédit sûr sans disjonction ; la garde coupe 88 % des tests de paires sans changer un rejet |
| Prototype rejouable du collectif d'arête (oracle Fraction/Gram, cinq mutants) | `morsehgp3D_v8/audits/collective_edge_20260922/` | empreintes de `RESULTS.json` | seul audit du 22 septembre entièrement dans le dépôt |
| Harnais de contrôle croisé q3/q4 et protocole bilatéral | `q3_stream_crosscheck_20260921/`, `q4_stream_crosscheck_20260921/`, `q34_stream_crosscheck_spatial_20260921/q4_bilateral_probe.cpp` | `12294241` | seul schéma qui passe à l'échelle d'une trame sans juge O(n^3) ; renforcer la complétude, requalifier les largeurs pour u18 (M9, M10) |
| Oracle q3/q4 i128 confronté au catalogue rationnel de `reference/` | `oracle_q3q4_20260915/` | `12294241` | vérité bornée indépendante (315 petits nuages, 0 désaccord) ; bornes énoncées pour u16 |
| Fixtures F1–F10 : sommet intérieur à Z, λ=24/25, G ambigu, témoins mutuels, propriété à égalité | `q3_bloc_float32_fixtures_20260921/` | `12294241` | chacune tue une erreur plausible de port des bornes de bloc |
| Fixture E5 (Prop. 6 / Th. 5 faux) ; obligations I1–I3 | `VERROUS_MATHEMATIQUES_20260914.md:39-108` | `12294241` | empêche de reprendre le K-MST comme autorité |
| Contrat des facettes silencieuses ; régression ABCDE à Kmax=2 ; limite v7 de 12 sites « ni tronquée ni transposée » | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:16-37, 113-115` | `12294241` (version longue à `4ccf8431`) | obligation du futur aval FULL, jamais construit en v8 |
| Critères de portes : planchers de non-vacuité, mutants causaux, fixture décisive « maximum en z » | `PORTES_ET_TESTS_20260914.md` | `12294241` | la lacune 2.2 montre qu'une fixture peut passer avec un mutant |
| Objet de tâche reprenable du front (`Task{a,b,mask,depth}`) ; avertissement sur le partitionnement statique | `front_tasks_20260914/README.md` | audit `329e5b86`, front `ba11e3ab` | granularité parallèle : un worker porte encore 18 à 39 % des paires le 21 septembre |
| Depuis la v7 (selon les auditeurs v8) : sémantique `full_ball_tower.hpp`, `anchor_meb.hpp`, graphe daté sur les naissances, quotient local non régulier, preuve de croissance de sortie m², mesures FULL 50k (419 s / 34 s) | `morsehgp3D_v7/src/forest/full_ball_tower.hpp`, `morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/README.md` §3, `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md`, `morsehgp3D_v8/audits/CONTRATS_ET_MESURES.md:33-52` | `dc57ffd5` (v7) | la v7 est la seule version du dépôt à avoir produit une tour FULL complète |
| Plans h+h_a+h_b et réemploi singleton, comme **hypothèse** à mesurer sur l'appel complet à 1 mm | `lidar_rectangles_20260922/probe.cpp` (rejouable), `SYNTHESE_PRIORITES_LIDAR_20260922.md` | `12294241`, arbre `src` `54a6d58` | gains annoncés non rejouables pour les gros rectangles (H1) ; étage de ≈ 5–8 % du temps sur le seul profil épinglé |

### À ne pas reprendre (avec la mesure qui a fermé la piste)

| Piste | Mesure qui la ferme |
|---|---|
| Tubes comme chemin général de crédits | 7 818 cellules pour 8 060 sites irréguliers, crédits nuls ; au meilleur réglage, 40× moins sélectif que Pool (`CREDITS_TERMINAUX_20260914.md:44-64`) |
| Test de lentille comme levier | 0,4 à 1 % des recherches portent sur une lentille vide ; 71 à 92 % des recherches sur lentille non vide ne rejettent rien (`PROPAGATION_TEMOINS_20260914.md:29-44`) |
| Élargir la fenêtre de K témoins **à la place d'**une descente saturante (q3/q4) | fenêtre 2K : 36–56 % (q3) contre 82–91 % pour un proposeur par descente ; témoins dans A∪B : 1–4 % (`DIALOGUE_AUDITEUR_B.md:56-66`). En q2, la fenêtre 2K reste un gain mesuré en option (`front_options_lidar_20260920`) |
| Couches duales pour filtrer les graines q3 (régimes mesurés) | coût 1,5 à 3,4 fois le census évité (`DIALOGUE_AUDITEUR_B.md:296-306`) ; à réexaminer seulement si graines/m dépasse le seuil |
| α=3 sur la voie q4 | contre-exemple entier u16 (`DIALOGUE_AUDITEUR_B.md:142-151`) |
| Bornes de puissance « aux coins seulement » sur boîte de centres × boîte Z | fixture F1 : coins [0,0] contre vraie plage [−25,0] (`DIALOGUE_AUDITEUR_B.md:742-751`) |
| Témoins individuels universels différents par coin pour certifier un rectangle | contre-exemple exact `b=(300,200,200)` (`CERTIFICATS_COLLECTIFS…:122-128`) |
| Restreindre Z aux graines propriétaires ou valides de X | F9 : deux témoins stricts perdus (`DIALOGUE_AUDITEUR_B.md:941-953`) |
| Exposant par relation parent/enfant comme verdict de croissance | compteur linéaire exact jugé de 0,78 à 1,27 (`DIALOGUE_AUDITEUR_B.md:504-514`) |
| Préfixes hachés comme protocole de croissance principal | remplacés par le protocole spatial ; gardés en régression (`DIALOGUE_COURANT.md:52-56`) |
| Lots singleton q2 | ×1,005 à ×1,225 sur 54 comparaisons (`morsehgp3D_v8/audits/ETAT_COURANT.md:398-403`) |
| Résultats d'audit dont les sources sont « jointes à la conversation » | aucune rejouabilité (H1) |

**Différées, non fermées** (à ne pas reprendre telles quelles, mais pas réfutées) :

| Piste | Statut réel |
|---|---|
| Blocs Z certifiés le long de la descente | uniforme 32k : résidu ×0,40 → ×0,34, front 57,6 → 78,6 s. B : « c'est cette comparaison, front plus census, qui doit trancher » (`PROPAGATION_TEMOINS_20260914.md:130-139`) ; jamais faite |
| Récursion qui relance l'index par sous-rectangle | 903,17 → 1 094,58 ms à 1024/amas/K10 (`RECTANGLES_H_HA_HB_SUIVI_20260922.md:108-113`) : n=1 024 synthétique, sources en archive. « Ne réfute pas le partage par rectangles ». Synthèse : « Différer » (`SYNTHESE…:94-96`) |
| Reprise par listes d'IDs partout | 449,74 → 511,18 ms, même corpus (`RECTANGLES…:99-106`) ; « première implémentation » ; différée |
| Rejeter q4 seul pour supprimer l'atlas | réserve de méthode, pas une réfutation : l'atlas sert aussi q3 (`lidar_rectangles_20260922/README.md:92` ; `SYNTHESE…:90-92`) |

## 8. Recommandations priorisées pour la v9

1. **Ouvrir la v9 hors de l'index partagé.**
   - Travailler dans un worktree isolé basé sur `origin/main` (`12294241` ou suivant), jamais depuis `/workspaces/E-HGP` (HEAD `a74e90f2`, 89 fichiers indexés, 971 non suivis sous `morsehgp3D_v8/`).
   - Exécuter `git diff --cached --quiet || exit` avant tout `git add`, et ajouter les chemins un par un.
   - Aucun fichier `morsehgp3D_v8/`, `audits/COORDINATION_MORSEHGP3D_V8.md` ou `audits/morsehgp3D_v8_complementaire/` d'un autre acteur (H3).
2. **Règle de dépôt des preuves d'audit.** Tout chiffre d'audit qui motive une décision doit avoir ses sources, ses entrées hachées et ses captures dans le dépôt. Sinon il reste « non vérifiable » jusqu'au dépôt. Rapatrier ou re-mesurer la cascade, la paire représentante, le collectif LiDAR, `RECTANGLES_H_HA_HB` et le prototype MEB avant usage (H1).
3. **Contre-audit indépendant immédiat des six commits `src` du 21–22 septembre et de la reprise u18.**
   - Contre-prouver les bornes à M=262143 sur le code.
   - Graver des fixtures aux extrêmes u18 et des mutants de débordement.
   - Rejouer les flux q3/q4 avec le rejet q3 par l'atlas (`0948d2d0`).
   - Ne rien hériter de la v8 par copie sans ce contre-audit (H2).
4. **Mesurer la part des étages avant d'ordonner les chantiers.** Sur trames entières sans sol à 1 mm, K5 et K10, publier un reçu épinglé qui ventile le temps entre front, filtre de témoins, cover, atlas q4, census q3 et sorties. Arbitrer ensuite par écrit entre la cascade (filtre) et la saturation d'atlas, avec ablation de chaque étage. Le seul profil épinglé donne l'atlas à ≈ 57 % et le filtre à ≈ 5–8 % (H1, H4).
5. **Un canal par auditeur et des rôles stables.**
   - Un fichier de dialogue par auditeur dans `morsehgp3D_v9/audits/`.
   - Obligation pour tout auditeur, externe compris, d'y consigner ses recommandations, et pour le développeur d'y répondre par écrit (accepté, refusé avec raison, différé).
   - Obligation de `git pull` avant chaque réponse.
   - Pas de rotation auditeur↔développeur sans passation écrite et remplaçant désigné (H4, M6).
6. **Porter dès l'ouverture les oracles et fixtures qui ont servi.** Harnais q3/q4 de B, protocole bilatéral, oracle i128, F1–F10, E5, fixtures du citron (en citant la preuve v7) et prototype collectif. Les enregistrer en CTest, avec labels et planchers, après requalification de leurs largeurs pour u18. Fermer la lacune 2.2 (M4, M10).
7. **Tracer les antécédents.** Toute revendication de nouveauté d'un audit cite la preuve antérieure de l'arbre, v7 comprise, ou déclare l'avoir cherchée (M2).
8. **Méthode de croissance.** Exposant poolé par parent, dispersion entre frères, axe densité compagnon, déficit de frontière publié. Aucune lecture de temps sous charge concurrente (M5, L2).
9. **Rejouabilité hors machine.** Aucun reçu ne dépend d'un chemin absolu ou d'un build ignoré sans le déclarer. Vérifier le champ `git_commit` contre la présence des sources (M3).
10. **Hygiène documentaire des auditeurs.** Fournir un lint non bloquant (équations, liens) que chaque auditeur lance sur ses fichiers, sans étendre le contrôle bloquant de `check_docs.py` contre sa doctrine (M7).
11. **Publier ou abandonner explicitement** le travail d'audit non commis (complémentaire du 13 septembre, reçu partiel de B) et les brouillons float32 de l'ancien constructeur (M1, Q1).
12. Décider tôt, pour la v9, le **format de nœud de sortie** et le **domaine exact des entrées non régulières**, que la v8 n'a jamais tranchés faute d'aval (§6).
13. Traiter les licences (Patchwork++, dérivés KITTI). Ne pas recopier en v9 l'archive de 28 Mo ni les reçus lourds non rejouables : l'historique de `main` ne se réécrit pas (L1, L4).

## Contre-vérification

Chaque affirmation du premier rapport a été confrontée à sa source. Verdicts : **confirmé**, **corrigé** (valeur ou formulation juste intégrée ci-dessus), **réfuté**, **non vérifiable**.

### Affirmations principales

| # | Affirmation d'origine | Verdict | Correction ou précision |
|---|---|---|---|
| 1 | Lemme du citron prouvé (A) et testé (B) | confirmé, provenance corrigée | Première preuve écrite en v7 (`FRONT_ET_TEMOINS_COURANT.md:5-15`, 4–6 sept.), deuxième par B via Jung (14 sept.), troisième par A (21 sept.). Test q4 aussi : 4 928 247 tétraèdres à 1k, 0 violation (`q4_stream_crosscheck_20260921/README.md:46-50`). Lignes justes de la preuve de A : `SUPPORT_ET_CITRON.md:34-63` |
| 2 | α=3 sur q4 est non sûr (contre-exemple) | confirmé | Recalculé en rationnels : poids positifs, ab maximale, H=608, Ξ=1 036 800, z strictement extérieur |
| 3 | Flux q3/q4 31–34 égaux aux énumérations (1k–4k et 8k) | confirmé, précisé | 8k seulement pour la tranche 32 (`d1b4dbc6`) ; coquilles q4 non comparées |
| 4 | 190 405 / 2 158 063 records q4 valides ; complétude échantillonnée | confirmé, précisé | Moteur t34 (`d6e1bd9e`), u16 2 cm avec sol ; complétude sur la scène : 1 et 22 boules énumérées |
| 5 | Chaîne q2 des tranches 8 à 18 rejouée, 0 désaccord | confirmé | Neuf lignes : trois en paires (104,7 à 327,3 M), six en appels |
| 6 | Cascade non rejouable, sources connues par SHA256 | corrigé | Vrai pour l'appelant combiné, la paire représentante, les classes > 65 536 et le collectif LiDAR. Les plans et le réemploi singleton ont une sonde rejouable (`probe.cpp`, modes 0–3, workflow). Le ×2,55–4,46 porte sur les deux premiers étages, collectif exclu. La présélection seule donne ×0,94–1,10 |
| 7 | Aucun audit indépendant de quatre commits moteur | corrigé | **Six** commits `src` (s'ajoutent `02987f18` et `5fdda963`). Exécutions indépendantes limitées au filtre de témoins (70 672 + 540 216 + 984 960 comparaisons, 13 046 contextes). Contrelectures internes de la reprise [non commis] |
| 8 | Un commit d'audit peut emporter le travail indexé d'un autre ; 89 fichiers indexés | confirmé, complété | 4c3cdb0c = 305 fichiers dont 300 du constructeur. S'y ajoutent 971 non suivis sous `morsehgp3D_v8/` et un HEAD local en retard de 13 commits |
| 9 | Recommandations du 22 septembre ni citées ni acquittées ; le développeur suit une autre piste sans arbitrage | corrigé (interprétation) | Fait exact, mais le worktree partagé ne contient pas ces audits (poussés depuis un clone séparé, réf. distante rafraîchie à 19:45 UTC) ; pas un refus. Les deux pistes visent des étages différents |
| 10 | Collectif d'arête exact mais redécouvre A (moments) et le complémentaire (capacités) | confirmé | `7f4ba045` et `65ac5ee6`, 13 sept. 18:40–18:41 ; empreintes vérifiées ; même défaut de traçabilité pour le citron (M2) |
| 11 | Singletons : double recherche de témoins ; gain mesuré seulement hors dépôt | confirmé, précisé | Lignes 407 et 487 ; le mode 1 de `probe.cpp` le rend rejouable |
| 12 | Pistes fermées : tubes, lentille, couches duales, fenêtre élargie, récursion par sous-rectangle, lots singleton | corrigé | La récursion est « différée », mesurée à n=1 024 synthétique avec sources en archive ; les blocs Z sont non tranchés ; la fenêtre 2K reste un gain q2 en option |
| 13 | Prop. 6 / Th. 5 / K-MST faux en général (E5) | confirmé | 3 nuages sur 366 à K=2 |
| 14 | Fixture « maximum en z » non décisive | confirmé | `p0_gate.cpp:260-262` inchangé depuis `85015a8c` |
| 15 | Complémentaire muet depuis le 13 sept. ; 11 fichiers et 53 lignes non suivis ; référence morte | confirmé, formulation corrigée | Les 53 lignes sont une modification non indexée d'un fichier suivi |
| 16 | Méthode d'exposants biaisée, non corrigée | confirmé | `ETAT_COURANT.md:108-110` ; protocole sans exposant poolé |
| 17 | La v7 est la seule version à avoir produit une tour FULL | confirmé, précisé | Reçu v7 vérifié (418,872898 s) ; les v4–v6 produisaient des forêts horizontales `verified_events_only`, pas FULL |

### Chiffres

| Chiffre d'origine | Verdict | Correction ou précision |
|---|---|---|
| Rectangles v4 3 435 133 → 219 063 683 ; 429 → 856 | confirmé | 128k et 256k hors tailles d'intérêt |
| s=8 v8 entre v4 s=14 et s=16 | confirmé | — |
| Plafond de proposeur 82–91 % / 75–89 % ; 2K 36–56 % / 6–55 % | confirmé | — |
| ÷87 à ÷1 519 | confirmé | Échantillon de 2 000 paires ; ×387 mesuré sur le moteur à 4k/K5 |
| Flux t32 8k | confirmé | — |
| Records q4 190 405 / 2 158 063 | confirmé | Moteur t34 |
| Relais 0,71–0,87 / 0,74–1,04 | confirmé | — |
| Couches duales 0,15–0,21 contre 0,27–0,53 | confirmé | — |
| Repli hull 843/1 642 | confirmé | — |
| Options q2 50k 9,604 → 5,307 → 4,774 s | confirmé | Plages 4,750–8,710 s pour 4K |
| Pool ÷38 | confirmé | — |
| Propagation ×0,40 / ×0,70 | confirmé | — |
| FULL v7 418,873 / 33,853 / 389,7 s | confirmé | Reçu v7 relu |
| Plancher 27 273 218 nœuds, 1,75 Go, 62,7 ms | corrigé | Nœuds vérifiés (reçu v7) ; 62,7 ms non vérifiable (sans reçu) |
| Cascade ×2,55–4,46 ; ×1,02–1,35 | confirmé (arithmétique), sources non vérifiables | Recalculé depuis `RESULTS.json` ; présélection seule ×0,94–1,10 ; somme de médianes par classe |
| Présélection ×5,13–11,10 | non vérifiable | Conforme au README ; recherche de h seulement |
| Plan sélectif ×2,55–3,95 | non vérifiable | Recalculé depuis le README : 2,555–3,952 |
| Collectif LiDAR 221/763 ; 1,95–3,35 contre 3,28–60,66 ms | non vérifiable | Sommes vérifiées (221, 763) |
| Collectif corpus exact 280 / 522 / 123 / 0 | confirmé | — |
| Garde 499 270 → 62 117 | confirmé | — |
| Microbenchmark 1,5–3,5 contre 0,175–0,19 µs | confirmé | — |
| MEB pivot4 0,262–0,271 ; 0,174–0,184 | non vérifiable | Hors dépôt, n=24/48 |
| Écart au contrat 279,3 s ; ×13–18 ; ×50–65 | corrigé | Diagnostic non épinglé ; base épinglée `ground_baseline_20260921` : 298,0 s (K5 W8), 911,1 s (K10 W8) |
| 66 MD, 12 172 lignes, 2 920 fichiers, 45 Mo ; complémentaire 21 MD, 2 385 lignes ; journal 3 850 | confirmé, précisé | 45 Mo sur disque, 39,2 Mo de contenu |
| 166 commits sur les chemins d'audit | confirmé | `2b658cbe` inclus |
| 89 fichiers indexés (47 tests, 12 `src/lanes`) | confirmé | — |
| Graines t31 3,68 M → 121,7 M | confirmé | — |
| Plateaux q4 373 | confirmé comme non vérifiable | — |
| Surproposition 42–68 % | confirmé | — |
| Plans synthétiques ×0,97–14,07 | confirmé (texte), non vérifiable (sources) | — |

### Autres corrections de détail

- Les dossiers `q2_*` de A sont **sept**, pas neuf.
- `1bf806f0` et `795a29dd` datent du 14 septembre, pas du 13.
- `8c050a33` est à 21:07 UTC ; 20:10 UTC est l'heure déclarée dans `ETAT_COURANT.md:6` et `PASSATION.md:12`.
- La propagation des témoins est portée en option explicite, le défaut historique restant inchangé.
- Les fixtures de dédoublonnage et le tétraèdre régulier demandés par B sont partiellement gravés, alors que l'original les donnait comme « non vérifié ».
- La valeur 210 987 existe dans les reçus de B (`input_pair_mass`, 1k/K10).
- La moitié x+ q3 a un reçu partiel **[non commis]** aux empreintes identiques.
- L'exclusion des auditeurs par `check_docs.py` est une doctrine explicite (`:90-91`). La recommandation d'étendre le contrôle bloquant est remplacée par un lint non bloquant.
- L'archive de 28 Mo ne peut pas être « exclue de l'historique v9 » sans réécrire `main` : on peut seulement cesser de la recopier.

### Omissions ajoutées

- Antécédent v7 du lemme du citron.
- Test q4 du citron par instances.
- Deux commits `src` supplémentaires non audités.
- Retard du worktree partagé sur `origin` et absence locale des audits du 22 septembre.
- 971 fichiers non suivis sous `morsehgp3D_v8/`, dont les brouillons float32 (Q1 ouverte).
- Changement de titulaire du 20 septembre et reprise du 22 septembre [non commis].
- Base sans sol épinglée et profil `gprof` : atlas ≈ 57 %, filtre ≈ 5–8 %.
- Présélection seule ×0,94–1,10.
- Règle de canal `AGENTS.md:200` non respectée par l'externe et par le développeur.
- Complétude bilatérale mince.
- Bornes des oracles énoncées pour u16.
- Incident `ctest -N` dans des builds épinglés.
- 979 Mo de reçus v8 suivis.
