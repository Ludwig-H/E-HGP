# Lentille 1/12 — Trajectoire, contrats et gouvernance de la v8 (version contre-vérifiée)

Audit en lecture seule, 22 septembre 2026, pour l'ouverture de la v9. Ce fichier reprend le rapport `01_trajectoire_contrats.md` (inchangé) et y intègre la contre-vérification adversariale ; chaque correction est listée dans la dernière section.

```text
phase=exploration_v8_hors_registre (audit de clôture, préparation v9)
backend=cpu_reference (aucun calcul lancé par cette lentille ni par sa contre-vérification)
profile=quantized_u16_input_only puis quantized_u18_input_only (moteur entier) ; lossless_float32_input_only (briques natives)
mode=audit_independant_trajectoire_contrats_gouvernance
public_status=not_claimed
```

GCP non utilisé. Aucune compilation, aucun ctest, aucun script du dépôt exécuté. État publié de référence : `origin/main` = `12294241` (worktree détaché). Tout ce qui vient du worktree partagé et n'est pas dans `12294241` porte l'étiquette **[NON COMMIS]**. Les journaux d'un build de l'orchestrateur (`scratchpad/head_ctest.log`, `mut_configure.log`, `mut_ctest.log`) sont lus, pas rejoués.

Statuts employés : **prouvé** (preuve écrite + fixture), **testé** (porte bornée), **mesuré** (reçu épinglé : commit, sha256, sorties), **proposé**, **manquant**. Un chiffre sans reçu est **non vérifiable**.

## 1. Périmètre lu

### Lu intégralement ou sur la partie utile

| Source | Portée de la lecture |
| --- | --- |
| `AGENTS.md` (à `12294241` : 480 lignes, 121 071 octets) | intégral : contrats, sections v8 datées, tranches 1 à 34, protocole spatial, cadre v7, règles générales |
| `CLAUDE.md` (à `12294241`) | « Cible de travail », « Commandes », « Conventions » |
| `morsehgp3D_v8/README.md` | l. 1-80 et 850-879 ; sections historiques par titres |
| `morsehgp3D_v8/PASSATION.md` | l. 1-80 et 630-640 ; historique par titres |
| `morsehgp3D_v8/audits/ETAT_COURANT.md` | l. 1-40 |
| `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` | intégral (234 lignes) |
| `morsehgp3D_v8/docs/JOURNAL_DEVELOPPEMENT_20260921.md` | intégral (149 lignes) + différence [NON COMMIS] |
| `morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` | intégral (88 lignes) + différence [NON COMMIS] |
| `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` | en-tête + différences [NON COMMIS] indexée et non indexée |
| `morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md` | l. 10-62 et 110-135 |
| `morsehgp3D_v8/docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md` | l. 1-14 |
| `morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md` | intégral (98 lignes) |
| `morsehgp3D_v8/docs/FAUSSES_PISTES.md` | l. 50-72, 100-125, lignes de tableau 373-421 |
| `morsehgp3D_v8/docs/PLAN_DE_REFONTE.md`, `VERROUS_ARCHITECTURE.md` | passages cités (l. 470-478 ; l. 410-418) |
| `morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md` | l. 1-60, 209-214, 373-378, titres |
| `morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md` | intégral (97 lignes) |
| `morsehgp3D_v8/audits/RECTANGLES_H_HA_HB_SUIVI_20260922.md`, `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md` | en-têtes, bases, décisions |
| `morsehgp3D_v8/audits/BUDGET_CONTRAT_50K_20260914.md` | l. 1-60 |
| `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` | passages de pin et de sémantique |
| `audits/COORDINATION_MORSEHGP3D_V8.md` (3 850 lignes à `12294241`, 3 946 sur disque) | historique Git ; différences [NON COMMIS] indexée (+24) et non indexée (+74) |
| `morsehgp3D_v8/src/pipeline/wspd_q34.hpp` | l. 1-50 et 90-135 ; inventaire de `src/` |
| `morsehgp3D_v8/CMakeLists.txt` | l. 480-610 (logique DISABLED) |
| `.github/workflows/morsehgp3d-v8-lidar-audit.yml` | intégral utile |
| Reçus `ground_baseline_20260921`, `ground_phase1_20260921` (README, `BASELINE*.json`, `only_time_*.txt`), `q34_spatial_20260921` (README), `lidar_global_20260921` (§ R3), `lidar_ground_20260921`, `lidar_ground_u16_20260921` (tableaux) | lus |
| [NON COMMIS] `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md`, `receipts/u18_resume_20260922/*/COMPLETION.json`, `release_r2/ctest.json`, `release_r2/scale_8000.json`, `ground_1mm_first/`, `receipts/ground_18bits_20260922/u16_identity/` | intégral utile |

### Historique Git

- `git log 2b658cbe^..origin/main` : **201 commits** (193 de `Ludwig-H`, 8 du robot `github-actions[bot]` pour la présentation Inria-Szeged) ; **169 touchent `morsehgp3D_v8/`**.
- Le chiffre de **158** donné par la tâche correspond aux commits qui touchent `morsehgp3D_v8/` entre `2b658cbe^` et `a74e90f2`. C'est le HEAD local du worktree partagé, qui a 13 commits de retard sur `origin/main`.
- Commits v8 par jour, en date d'auteur : 19 (13 sept.), 49 (14), 19 (15), 0 (16), 3 (17), 0 (18), 3 (19), 16 (20), 46 (21), 14 (22). Le 16 septembre ne porte que la présentation et `HGP-Clusterer3D`.

### Non lu

- Les notes techniques par tranche (`docs/P0_*`, `Q3_Q4_*`, `Q4_*`, `Q34_*`, notes float32), au-delà de leur résumé dans `AGENTS.md`.
- Le code : 38 421 lignes de `tests/`, 32 120 de `bench/`, 17 116 de `src/` hors les deux fichiers cités.
- L'essentiel des JSON de reçus.
- La coordination, au-delà de son historique et des différences non commises.
- Les sources v7 elles-mêmes.
- Les archives zip « jointes à la conversation », absentes du dépôt.
- Les runs du workflow CI d'audit.

## 2. Ce qui a été fait

### 2.1 Chronologie

| Date (UTC) | Événement | Commits | Rôle |
| --- | --- | --- | --- |
| 13 sept. | Audit général v7, ouverture v8 ; P0 « supprimer O(\|A\|²+\|B\|²) » ; verrous B1-B5 | `2b658cbe`, `f375d2c6`, `e409aa46` | ROOT / constructeur 1 |
| 13 sept. | Tranches 1 à 4 : crédits locaux, Tubes partagés + filtre axial, addition/intersection, census q2 | `3589a2c9`, `8e406f9b`, `f5430f57`, `f4815cd4` | constructeur 1 ; auditeurs A et complémentaire |
| 14 sept. | Tranches 5 à 15 (bornes préparées → continuations possédées) | `3c29ea1e` … `d09e2207` | constructeur 1 ; auditeur B ouvert (`77bcd0b8`, 07:37) |
| 15 sept. | Tranches 16 à 18 : détachement, équipe persistante, plages d'ancres | `897085f8`, `beee3341`, `2741d614` | constructeur 1 |
| 17 sept. | Changement de constructeur : l'auditeur B livre 19 (négatif), 20, 21 | `8d615cfd`, `8190e7ab`, `3e94c868` | B constructeur (`AGENTS.md:266`) |
| 19-20 sept. | Audit indépendant des facettes silencieuses (portage v7), base `4ccf8431` | `ae98dbfa` → `c92aad13` | auditeur indépendant (docs(v8)) |
| 20 sept. | Reprise du constructeur 2 ; tranches 22 à 30 (famille q4, candidats par graine, covers, rejet familial, collectif, carte, fragments, couches duales, fenêtre) | `9ae4e28b`, `785d0589`, `77f659e4`, `8d0a0f0f`, `ddaafdc0`, `66b1551f`, `c051bdb0`, `31b0243a`, `f07fbd8c` | constructeur 2 |
| 21 sept. matin | Tranche 31 (raccord global, priorité LiDAR, G4 R1/R2 échoués, R3 clos) ; 32, 33, 34 ; protocole spatial ; trames entières + G4 CPU | `4dbe3024`, `d1b4dbc6`, `2629a536`, `4c3cdb0c` + `d6e1bd9e`, `759ce2b0`, `70de84f2` | constructeur 2 ; B rouvert auditeur (`09361c3c`) |
| 21 sept. après-midi | Contrat trames entières ; précision float32 ; index, boules, clés et événements q4, census q3 float32 (une arête) ; protocole et pilote sans sol | `36724438`, `028a0f1d`, `12d885d8`, `9923a6b9`, `a005f8aa`, `204b0620` | constructeur 2 |
| 21 sept. 20:10-21:07 | Reprise développeur (B) : nuages sans sol u16 (`ec2bc503`, 20:40), audit à neuf lentilles (`8c050a33`) | `ec2bc503`, `8c050a33` | B développeur (`PASSATION.md:12-18`) |
| 21-22 sept. | Phases 0-2 : CMake et labels, atlas i64 2^20, fragments, graines q3 certifiées par l'atlas, file de tâches, chronos par worker, deux variantes rejetées ; reçus appariés | `92d74c13`, `748ec082`, `02987f18`, `0948d2d0`, `5224ff4e`, `5fdda963`, `0e2c18ca`, `72f125c6`, `93ba5bb4` | développeur |
| 22 sept. 06:21 | Élargissement du moteur entier à 18 bits | `a74e90f2` | développeur |
| 22 sept. 06:29-06:53 | [NON COMMIS] Campagne d'identité u16 sur le moteur u18 (6 lignes W8) ; 42 portes extrêmes laissées en cours ; 53 fichiers indexés datent de cette fenêtre | aucun | développeur |
| 22 sept. 09:58-10:56 | [NON COMMIS] Reprise u18 : option `saturate_deep`, correctifs de domaine numérique, captures R1/R2, première trame sans sol à 1 mm | aucun ; entrée « Constructeur » indexée dans la coordination | acteur signant « Constructeur » |
| 22 sept. 10:36-19:40 | Audit externe : certificats collectifs d'arête, rectangles h+h_a+h_b, cascade LiDAR, workflow CI d'audit (12 commits d'un fichier, le dernier en touche deux) | 13 commits `bf73e194` → `12294241` | auditeur externe (`audit(v8):`), depuis un clone distinct basé sur `a74e90f2` |

### 2.2 Les 34 tranches en quatre périodes

- **Tranches 1-21, voie q2 (13-17 sept.).** La P0 est attaquée par crédits locaux. La chaîne front WSPD + census q2 est ensuite construite, parallélisée et déclinée en variantes. CTests : 26 (tranche 3) → 78 (tranche 20). Aucune tranche n'a fermé P0 : « P0 global, borne sous-quadratique, q3/q4 produit, FULL, GPU et contrats G4 restent ouverts » (`AGENTS.md:270`).
- **Tranches 22-30, primitives q3/q4 (20 sept.).** Famille q4 exacte, clés `ExactBall`, candidats par graine, covers, certificats universel et collectif, carte, fragments, couches duales, fenêtre. Chacune est testée sur des arêtes fournies. CTests : 82 → 91.
- **Tranches 31-34, raccord global et LiDAR (21 sept.).** `run_wspd_q34_candidates` et `run_wspd_q34_parallel` consomment le front multivoie entier. S'y ajoutent les témoins indexés, le census q3 par boîtes, les bornes affines et le parcours graines×cellules. CTests : 92 → 96. Premières sessions G4 CPU.
- **Reprise développeur (21-22 sept.).** Réduction du travail sur le moteur entier (×1,96 en CPU à K5 W1 sur la scène 0 sans sol), file de tâches, puis port 18 bits.

### 2.3 Contrats successifs et leur état exact au 22 septembre

| # | Date, source | Énoncé | État au 22 sept. |
| --- | --- | --- | --- |
| C0 | hérité v7 ; `AGENTS.md:354` | Tour FULL K=1..10 sur 50 000 points < 1 s sur G4 ; repli K=1..5 ; puis 100 ms | Jamais mesuré en v8 (aucune tour). Référence v7 : 418,873 s (K10) et 33,853 s (K5), 50k uniforme u16, FULL mono-thread (`AUDIT_V7_SYNTHESE.md:47-51`). Remplacé le 21 sept. |
| C1 | 13 sept. ; `AGENTS.md:198` | P0 : supprimer les histogrammes O(\|A\|²+\|B\|²), travail aval inclus | **Non clos** ; tranche 19 négative ; tranches 20-21 = « une constante divisée, pas un exposant » (`AGENTS.md:270`) |
| C2 | 21 sept. ; `AGENTS.md:314` | La croissance mesurée sur les régimes visés (SemanticKITTI) prime sur une borne sous-quadratique universelle ; exactitude inchangée | Appliqué (mesures G4 et spatiales) ; aucun sous-quadratique acquis sur toute la chaîne |
| C3 | 21 sept. ; `CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:9-13` | Tour HGP K=1..10 d'une **trame SemanticKITTI entière** < 1 s sur G4, repli 1..5, puis 100 ms ; plusieurs scènes ; aucun sous-échantillonnage | **Non atteint, non mesurable** : aucune tour. Seul un flux de candidats q3/q4 K5 a été chronométré sur trames entières (34,3 à 505,5 s sur G4 W48) |
| C4 | 21 sept. ; `PRECISION_FLOAT32_ET_GRILLE_20260921.md:3-9` | Coordonnées float32 originales **par défaut** ; grille isotrope optionnelle, 1 mm par défaut | Briques float32 qualifiées (index, prédicats, clés, événements q4, census q3 d'**une** arête) ; aucun générateur global float32 commis ; brouillons non suivis |
| C5 | 21 sept. ; `AGENTS.md:29-40`, `CONTRAT_TRAMES…:61-75` | LiDAR **sans sol** = régime prioritaire **supplémentaire**, pas un remplacement de la trame brute | Pilote Patchwork++ `3e6903a1` qualifié ; masques sur trois trames |
| C6 | 21 sept. 20:10 ; `PASSATION.md:14-18`, `AUDIT_REPRISE…:16-20` | Directive au développeur : régime prioritaire = sans sol 30 000-60 000 sites ; contrats temps 1 s puis 100 ms et K5/K10 **doivent passer là** ; multi-CPU puis GPU ; feu vert G4 | Mesuré sur flux de candidats seulement ; écart de ×9,4 à ×46 selon K et le décompte CPU (§ 4). **Absent d'`AGENTS.md`** (aucune occurrence de « 30 000 », « 18 bits » ou « u18 » à `12294241`) |
| C7 | 21 sept. ; `CONTRAT_TRAMES…:15-22,67-69` puis `AUDIT_REPRISE…:137-141` (D2) | Frontière du chronomètre | **Partiellement inscrite.** Le contrat fixe déjà les principes : chronométrer depuis l'entrée déclarée, transferts compris ; séparer le calcul en mémoire, la préparation, le disque, le froid et le chaud ; mesurer séparément segmentation, HGP et leur somme ; ne pas choisir la frontière après mesure. La frontière définitive est renvoyée au futur lanceur FULL. D2 (lecture incluse, segmentation publiée à part) reste une **proposition** non reportée dans le contrat |
| C8 | 22 sept. ; `PASSATION.md:36-40`, `ELARGISSEMENT_18_BITS_20260922.md:3-6`, `JOURNAL…:104-107` | « Décision utilisateur » : le contrat temps (sans sol 30-60k, K5/K10) se poursuit sur le **moteur entier 18 bits** ; float32 qualifié mais hors contrat temps et hors développement | Réponse consignée à la question Q2 de `AUDIT_REPRISE…:221-223` ; aucun texte de l'utilisateur n'est cité dans le dépôt (provenance non vérifiable). Port `a74e90f2` commis sans reçu. À `12294241`, trois textes restent sur l'état antérieur : `AGENTS.md:18` (« Le moteur existant demeure u16 »), `CONTRAT_TRAMES…:46` (« Le moteur actuel reste `quantized_u16_input_only` ») et `PASSATION.md:54-55` (« précision cible … float32 original »), ce dernier dans le même fichier que C8. [NON COMMIS] La reprise réécrit la provenance (§ 5, D3) |

### 2.4 Ce que la v8 calcule aujourd'hui face aux contrats

| Objet exigé par C3/C6 | Existe à `12294241` ? | Preuve |
| --- | --- | --- |
| Flux de supports q2 (census, coquille et intérieurs) | Oui, entrées séparées `run_wspd_q2_*`, hors de l'appel q3/q4 | `AGENTS.md` tranches 4 et 8 ; `AUDIT_REPRISE…:64` |
| Flux de **candidats** q3/q4 global, exact en entier, parallèle | Oui : `run_wspd_q34_parallel`. Commentaire du code : « CANDIDATE STREAM … not a deduplicated ball catalogue, interior-ID payload, HGP forest or FULL tower » ; « Different presentations/edges may still emit the same ball » | `src/pipeline/wspd_q34.hpp:100-101,114` |
| Catalogue canonique dédupliqué, arité minimale | **Non** | idem ; `AUDIT_REPRISE…:69` |
| Intérieurs des boules q3/q4 | **Non** (coquille seulement) | idem |
| Parents, fold, forêts K=1..10, tour FULL | **Non** : `src/forest`, `src/tree`, `src/io`, `src/cloud` et `src/gpu` ne contiennent qu'un `.gitkeep` | inventaire de `src/` ; `AUDIT_REPRISE…:56-57` |
| Lanceur FULL chronométré | **Non** | `CONTRAT_TRAMES…:21-22` |
| GPU | **Non** ; G4 utilisé en CPU seulement | `AUDIT_REPRISE…:70` ; `q34_spatial_20260921/README.md:91` |
| Entrée float32 native de bout en bout | **Non** ; briques isolées | `morsehgp3D_v8/README.md:74-80` |
| Entrée grille 1 mm | Oui depuis `a74e90f2` (`.u32le`, profil `quantized_u18_input_only`) | message de `a74e90f2` ; `JOURNAL…:102-124` |

**Conclusion.** La v8 livre un générateur de candidats q3/q4 et une chaîne q2 séparée. Aucun contrat de tour n'est mesurable : il manque l'aval que la v7 possédait, puisque la v7 produisait des tours FULL 50k (`AUDIT_V7_SYNTHESE.md:17-19`).

### 2.5 Gouvernance : rôles et canaux

| Acteur | Période | Écritures | Traçabilité | Preuve |
| --- | --- | --- | --- | --- |
| ROOT / constructeur 1 | 13-15 sept. | sources, docs, reçus, `ETAT_COURANT.md`, coordination | aucun trailer | `COORDINATION_MORSEHGP3D_V8.md:3-130` |
| Auditeur complémentaire | 13 sept. (dernier commit `77b1abf0`, 21:07) | `audits/morsehgp3D_v8_complementaire/` | aucun trailer ; fichiers non suivis depuis le 13 sept. 21:23 | `git log` du dossier ; `git status` |
| Auditeur A | 13-21 sept. (dernier commit `32297105`, 21 sept. 11:17) | `DIALOGUE_COURANT.md`, notes `P0_*` | aucun trailer | `git log` |
| Auditeur B | auditeur les 14-15 sept., constructeur du 17 au 20 (tranches 19-21), de nouveau auditeur le 21 au matin (jusqu'à 20:04), développeur à partir du 21 à 20:10 | `DIALOGUE_AUDITEUR_B.md`, puis sources | trailer `Co-Authored-By: Claude Fable 5.1` sur 80 commits ; trailer `Claude-Session` sur 43 commits des 14-15 sept. | `DIALOGUE_AUDITEUR_B.md:3-11` ; `PASSATION.md:6-14` ; `git log` des trailers |
| Constructeur 2 | 20-21 sept. | tranches 22-34, float32, pilote sans sol | aucun trailer | `PASSATION.md:635-637` |
| « Constructeur » [NON COMMIS] | 22 sept. 09:58-10:56 | reprise u18, entrée de coordination indexée | aucun commit ; identité avec le constructeur 2 probable mais non établie | `git diff --cached -- audits/COORDINATION_MORSEHGP3D_V8.md` |
| Auditeur externe | 22 sept. 10:36-19:40 | 13 commits `audit(v8):` sous `morsehgp3D_v8/audits/` et le workflow ; sources dans des zip hors dépôt | aucun trailer ; clone séparé | `SYNTHESE_PRIORITES_LIDAR…:68-69` ; `RECTANGLES_H_HA_HB…:9-12` |

- **Un seul auteur Git.** Les 193 commits non robotisés ont le même auteur (`Ludwig-H`). Seule la lignée de B est reconnaissable, par ses trailers. Les autres rôles se lisent aux préfixes (`audit:`, `audit b:`, `audit(v8):`) et à la propriété des fichiers.
- **Canal de coordination abandonné.** Le dernier commit qui touche la coordination est `204b0620` (constructeur 2). Le développeur n'y a rien commis ; son canal effectif est `JOURNAL_DEVELOPPEMENT_20260921.md`.
- **Indépendance limitée.** La lignée B est à la fois auditeur, constructeur puis développeur. Elle partage aussi la session de cet audit de clôture. Le trailer `Claude-Session` des commits de B des 14-15 septembre est celui de la session courante, et la sonde de la campagne phase 1 était épinglée dans le scratchpad de cette même session (`ground_phase1_20260921/BASELINE.only.json`, `pins.probe`).
- **Aucune contre-lecture des commits du développeur.** Personne n'a relu dans le dépôt `0948d2d0`, `5224ff4e` ni `a74e90f2` : A est silencieux depuis le 21 à 11:17, B est devenu développeur, et l'audit externe du 22 part de `a74e90f2` mais porte sur un prototype autonome (`CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md:3-4`).

### 2.6 Volume produit

| Grandeur | Valeur à `12294241` | Source |
| --- | ---: | --- |
| Reçus suivis `morsehgp3D_v8/receipts/` | 979 311 698 octets, 16 016 fichiers, 48 dossiers | `git ls-files` + `du -b` dans le worktree détaché |
| Plus gros reçu | `q4_seed_cells_20260921` : 221 344 585 octets | idem |
| Audits suivis `morsehgp3D_v8/audits/` | 39 232 414 octets ; 257 Mo sur disque avec les répertoires ignorés | idem |
| Documentation `docs/` | 59 fichiers, 12 072 lignes | `wc` |
| Code `src/` | 78 fichiers source (17 116 lignes) + 9 `.gitkeep` ; `tests/` 38 421 lignes ; `bench/` 32 120 | `find`, `wc` |
| `AGENTS.md` | 31 786 octets à `2b658cbe` (30 856 avant) → 121 071 à `12294241` (×3,8) | `git show` |
| Répertoires `build/v8*` | 150 (141 `v8_*`, 9 `v8-*`), 13 Go ; `build/` total 25 à 27 Go selon l'heure | `find`, `du` |
| Sessions G4 | 4 (R1 et R2 échouées, R3, pilote spatial), toutes CPU, TERMINATED certifiées | `lidar_global_20260921/README.md` § R3 ; `q34_spatial_20260921/README.md:147-156` |

## 3. État par composant

| Composant | Statut | Preuve | Limite |
| --- | --- | --- | --- |
| Crédits locaux / Tubes / axes (tranches 1-3) | testé ; mesuré (8k/16k/32k synthétiques) | reçus `p0_local_credits_20260913`, `shared_axis_20260913`, `additive_q2_20260913` | ne raccordent pas la P0 globale |
| Front WSPD `MidpointSamples` + index u16→u18 | testé ; mesuré | tranches 7, 13 ; `a74e90f2` | s8 v8 ≠ s8 v4 (`AGENTS.md`, tranche 6) |
| Chaîne q2 (census, Pool, multi-CPU, fenêtre `{2,16,true}`) | testé ; mesuré (synthétique + préfixes LiDAR) | reçus `q2_*` | hors appel q3/q4 ; jamais mesurée sans sol (`AUDIT_REPRISE…:64`) |
| Flux q3/q4 global `run_wspd_q34_parallel` | testé (oracles rationnels ; énumérations indépendantes de B de 1k à 8k) ; mesuré (trames 2 cm, sans sol 2 cm) | reçus `q34_*`, `lidar_global`, `q34_spatial`, `ground_*` ; `DIALOGUE_AUDITEUR_B.md:209-214,373-378` | flux non dédupliqué ; aucun oracle exhaustif sur une trame |
| Graines q3 certifiées par l'atlas q4 | **prouvé par son auteur** : lemme écrit, porte à planchers, un mutant causal (`q3_atlas_rejects_at_k_minus_2`) dans un script qui en compte quatre | `Q3_CERTIFICAT_ATLAS_20260921.md:30-45,87-98` ; `0948d2d0` | aucune contre-lecture indépendante ; bornes écrites pour M = 65 535 ; option explicite, défaut faux |
| File de tâches par plages de rectangles | testé (identités de registre, 5e mutant) ; mesuré localement | `JOURNAL…:18,29-43` ; `5224ff4e` | jamais mesurée sur G4 |
| Élargissement 18 bits | testé selon le commit (79 portes courtes, 129 tests exécutés, 3 désactivés) | message de `a74e90f2` ; `JOURNAL…:125-133` | **aucun reçu commis** ; `mhgp8_wspd_q34_mutations` a échoué une fois sous charge ; [NON COMMIS] la reprise a trouvé un défaut de domaine des fabriques publiques et 5 compteurs oubliés par l'identité |
| [NON COMMIS] Reprise u18 + `saturate_deep` | essais **en échec**. `release` : ctest en échec. `sanitize` : interrompu (signal 2). `release_r2` : `failed`, « disabled CTests differ » sur 3 tests, alors que les 136 tests exécutés passent. `sanitize_r2` : `failed` sur 8 tests désactivés | `receipts/u18_resume_20260922/*/COMPLETION.json`, `release_r2/ctest.json` | captures **non suivies** (ni indexées ni commises) ; tranche inachevée à 10:56 UTC |
| Briques float32 (index, prédicats, boules, clés, événements q4, census q3 d'une arête) | testé (Fraction, sanitizers, mutants) ; mesuré (synthétique 8k/16k/32k, trames pour l'index) | reçus `float32_*_20260921` ; 23 CTests labellisés `float32` | pas de générateur ; ×23 à ×70 par prédicat contre l'u16 (`AUDIT_REPRISE…:65`) ; hors contrat temps selon D1 et C8 |
| Brouillons float32 globaux | manquant (non suivis, non liables) | `git status` : `src/wspd/float32_front.*` etc. (21 sept. 20:16) | à ne pas reprendre sans décision |
| Pilote sans sol Patchwork++ | testé (44 commandes Release, 17 SAN, 20 tests, 42 nuages) ; mesuré (≈30 ms de la lecture au masque) | `receipts/lidar_ground_20260921` ; `AGENTS.md:42-56` | aucune qualité sémantique ; licence BSD-2 non déclarée |
| Préparation spatiale et précision | testé | `lidar_spatial_20260921`, `float32_precision_20260921` | entrées 2 cm historiques |
| Catalogue, intérieurs q3/q4, fold, forêts, tour FULL | **manquant** | `src/forest` : `.gitkeep` seul ; `wspd_q34.hpp:100-101` | le contrat porte sur la tour |
| Lanceur FULL / frontière définitive du chronomètre | **manquant** (principes inscrits, frontière proposée par D2) | `CONTRAT_TRAMES…:15-22` ; `AUDIT_REPRISE…:137-141` | — |
| GPU | **manquant** | `src/gpu` : `.gitkeep` seul | v5-v7 : kernels à 2-4 % de leur étage (`AUDIT_REPRISE…:70`) |
| CTest v8 | testé : 132 inscrits. Build de l'orchestrateur hors de `<v8_head>/build` : 125 exécutés verts, 7 DISABLED. Build de mutation sous `<v8_head>/build` : 8 portes de mutation exécutées vertes, seule `mhgp8_q34_indexed_witness_mutations` désactivée | `CMakeLists.txt:486-600` ; `scratchpad/head_ctest.log`, `mut_ctest.log` (non rejoués) | porte spatiale liée à un build épinglé unique ; mutations de témoins désactivées depuis `2629a536` |
| CI GitHub | la CI principale ne construit pas la v8. Un workflow d'audit (`contents: read`) construit `mhgp8_p0` et une sonde d'audit sans `-Werror` | `.github/workflows/morsehgp3d-v8-lidar-audit.yml` (`562d090c`, `ca73e96b`) | ne rejoue aucune porte v8 |

## 4. Chiffres clés

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| Tour FULL v7 50k K=1..10 / K=1..5 | 418,873 s / 33,853 s | `AUDIT_V7_SYNTHESE.md:47-51` (runs v7 du 10 sept.) | uniforme u16, FULL mono-thread ; seule tour de référence |
| Tour v7 50k K10 : nœuds, catalogue, RSS | 27,27 M ; 21,47 M boules ; ≈15,5 Gio | `AUDIT_V7_SYNTHESE.md:56-58` | — |
| Plancher d'écriture de la sortie v7 K10 50k | 1,75 Go à 64 o/nœud ; 62,7 ms mono | `BUDGET_CONTRAT_50K_20260914.md:42-51` | hôte local, pas G4 ; note déclarée **historique** (l. 8) : « les chiffres … ne valent pas pour les sources actuelles » |
| Budget du contrat 1 s sur 48 CPU | 48 CPU·s | `AUDIT_REPRISE…:53` | parallélisme parfait supposé ; 48 CPU logiques = 24 cœurs SMT sur G4 |
| Trame entière 2 cm, sites | 119 142 / 119 942 / 120 725 (retours 123 389 / 124 479 / 125 526) | `CONTRAT_TRAMES…:46-47` | séquence 08 seule |
| G4 W48, flux q3/q4 K5 s8, trames 0/100/200 | pipeline 165,214 / 34,319 / 505,479 s ; CPU 691,65 / 382,00 / 973,37 s | `q34_spatial_20260921/README.md:97-102` | une observation par cas ; pas FULL ; chargement disque et sérialisation exclus |
| Écart au contrat, G4 trame brute (flux q3/q4 seul) | mur ×34 à ×505 face à 1 s ; CPU ×8,0 à ×20,3 face à 48 CPU·s | calcul sur la ligne précédente | K5 seulement |
| Occupation G4 (CPU logiques moyens sur 48) | trames : 4,19 / 11,13 / 1,93 ; préfixe 8k (R3) : 3,23 | `q34_spatial…:97-102` ; `lidar_global_20260921/README.md:150-151` | jobs Coarse indivisibles |
| Boules q3 construites / émises (G4, trames) | 244,805 / 122,737 / 273,138 M pour 1,253 / 1,213 / 1,370 M | `q34_spatial…:108-110` | travail avant rejet |
| Local W4, trame 0 entière K5 | 383,311 s de pipeline (383,319 s de mur) ; 1 417,521 CPU·s | `q34_spatial…:47-59` | colonne intitulée « Pipeline CPU (s) » mais valeur murale ; hôte partagé |
| Exposant trame→moitié x+ (bornes q3 ; blocs q4) | 2,502 ; 2,332 | `q34_spatial…:67-73` | diagnostic d'une scène |
| G4 R3, préfixe scan 0 2 cm, K5 W48 : 8k | 614,744 s ; tests q3 ×10,765 de 4k à 8k | `lidar_global_20260921/README.md:135,140` | tailles 1k-4k = diagnostic |
| Sites sans sol u16 2 cm | 39 815 / 35 491 / 45 114 | `lidar_ground_u16_20260921/README.md:28-30` | nuages non versionnés (`audits/lidar08_20260914/.gitignore:2`) |
| Sites sans sol float32 / 1 mm | 39 885 / 35 551 / 45 845 (mêmes effectifs pour les deux profils) | `lidar_ground_20260921/README.md:49-51` ; `AGENTS.md:52` | ni doublon ni fusion 1 mm sur ces trames |
| Base sans sol scène 0 : K5 W1 ; K5 W8 ; K10 W8 | 889,5 s ; 298,0 s (1 283,4 CPU·s) ; 911,1 s (3 876,5 CPU·s) | `ground_baseline_20260921/README.md:24-26` (sonde 92d74c13, sha `925daeaa…`) | hôte partagé (harnais d'audit sur un cœur) |
| Après phases 1-2, scène 0 au calme : K5 W1 ; K5 W8 ; K10 W8 | 453,3 s (453,3 CPU·s) ; 108,0 s (741,5 CPU·s, 686 %) ; 323,0 s (2 212,1 CPU·s) | `ground_phase1_20260921/README.md:31-33`, `BASELINE.only.json` (statut `partial`, 3 lignes) | sonde `bb7f01fc…` épinglée dans le scratchpad `/tmp`, aujourd'hui absente ; worktree sale à la mesure (runner modifié, brouillons float32 non suivis) |
| Scène 2 K10 W8 après phases 1-2 | 824,1 s ; 3 811,0 CPU·s | idem l. 39 | charge croisée déclarée |
| Écart en CPU·s au budget de 48 (sans sol 2 cm, scène 0, flux q3/q4) | K5 : ×9,4 en W1, ×15,4 en W8 ; K10 : ×46,1 en W8 | calcul : 453,3/48, 741,5/48, 2 212,1/48 | hôte local 8 vCPU sur 4 cœurs SMT : le CPU·s en W8 est gonflé de 1,64× face au W1 pour le même travail ; sans q2 ni aval |
| Rejet q3 par l'atlas | 90,7 % (quart sans sol, 7 067 sites) ; 73,8 % (préfixe 8k avec sol) | `Q3_CERTIFICAT_ATLAS_20260921.md:78-81` | diagnostic, pas un reçu |
| Profil gprof (quart, après atlas) | atlas q4 ≈52 %, census q3 12 %, filtres de témoins 13 % | `JOURNAL…:62-64` | diagnostic ; avant l'atlas : atlas 40 %, census 28 % (`ground_baseline…:54-57`) |
| [NON COMMIS] Trame 0 sans sol 1 mm K5 W8 | 104,63 s ; 812,82 CPU·s (×16,9 face à 48) ; 691 284 q3 / 158 496 q4 | `u18_resume_20260922/ground_1mm_first/` (`COMPLETION.json` `passed`) | une répétition ; sonde du build R1 (qualification en échec, lecteurs Python) ; reçu non suivi |
| [NON COMMIS] Entrées u16 sur le moteur u18, W8, 6 lignes | CPU +5,1 % à +8,3 % par rapport à la phase 1 (scène 0 : 802,71 / 2 362,80 CPU·s contre 741,5 / 2 212,1) | `ground_18bits_20260922/u16_identity/only_time_*.txt`, `BASELINE.only.json` | non comparable : charge enregistrée élevée (load 8,4 à 10,0 sur 8 vCPU avant chaque ligne), une répétition |
| CTests v8 inscrits | 26 (t3) → 78 (t20) → 96 (t34) → 132 (`92d74c13`) → 139 [NON COMMIS] | `AGENTS.md` par tranche ; `CMakeLists.txt` ; `release_r2/ctest.json` | journal : « 132 tests, 121 verts + 2 DISABLED » (`JOURNAL…:14`), somme non réconciliée |
| Tranche 20 q2 (fenêtre 2K) | ×0,42 à ×0,71 ; rangées ×1,01 à ×1,18 | `AGENTS.md:270` ; reçus `q2_front_proposals_20260917` | constante, pas exposant |
| Tranche 19 (lots singleton) | ×1,005 à ×1,225 (négatif) | `AGENTS.md:266` | fermé |
| Cascade rectangles (audit externe) | ×2,55 à ×4,46 sur échantillons de filtrage ; 1 525 176 comparaisons exactes sans désaccord | `SYNTHESE_PRIORITES_LIDAR_20260922.md:43-65` | proposé, non porté ; atlas, graines, index/front et FULL exclus ; « ne suppriment pas d'atlas supplémentaires » |

## 5. Défauts, risques et dettes

### Haute gravité

- **D1. Aucune tour : le contrat est invérifiable.** Le seul objet produit est un flux de candidats non dédupliqué (`wspd_q34.hpp:100-114`). Catalogue, intérieurs, fold et forêts n'existent pas (`src/forest` ne contient qu'un `.gitkeep`). La phase 3 du plan développeur, « aval de la tour… lanceur FULL chronométré » (`AUDIT_REPRISE…:193-198`), n'a pas démarré. La v7 avait une tour FULL (418,873 s à 50k K10). La v8 a régressé en périmètre pendant neuf jours.
- **D2. Écart de travail d'un à deux ordres de grandeur, avant l'aval.** Sur le seul flux q3/q4 sans sol, scène 0, en CPU·s face au budget de 48 : ×9,4 (K5 W1) à ×15,4 (K5 W8) et ×46,1 (K10 W8) (`ground_phase1_20260921`). Sur trame brute, G4 W48, K5 : mur ×34 à ×505. Le plancher d'écriture de la sortie v7 (1,75 Go à 50k K10, 62,7 ms mono sur l'hôte local) ne laisserait que < 40 ms au reste de la tour pour 100 ms. Cette note est déclarée historique (`BUDGET_CONTRAT_50K…:8,42-51`) : le format de nœud reste à décider avant toute cible de 100 ms.
- **D3. Contrat ambigu et provenance contestée.**
  - Quatre énoncés coexistent : C3 trame brute ≈120k ; C5 sans sol « supplémentaire » ; C6 sans sol 30-60k « où les contrats doivent passer » ; C8 moteur entier 18 bits pour le contrat temps.
  - À `12294241`, `AGENTS.md` n'intègre ni C6 ni C8. Trois documents d'entrée restent à u16 ou float32 : `AGENTS.md:18`, `CONTRAT_TRAMES…:46`, `PASSATION.md:54-55`. `PASSATION.md` se contredit : C8 aux l. 36-40, float32 cible aux l. 54-55.
  - [NON COMMIS] La reprise insère en tête d'`AGENTS.md` un paragraphe (modification **non indexée**) : le port 18 bits « sert la grille OPTIONNELLE 1 mm ; il ne remplace ni le profil float32 par défaut ni le contrat brut entier ». Elle remplace aussi l. 18 par « Le moteur entier est désormais élargi à u18 ».
  - [NON COMMIS] Elle réécrit la première phrase d'`ELARGISSEMENT_18_BITS_20260922.md` : « Décision utilisateur du 22 septembre 2026 » devient « La passation du développeur du 22 septembre donne la priorité… ».
  - [NON COMMIS] Elle garde pourtant « décision utilisateur de poursuivre en entier 18 bits » dans la partie historique de `PASSATION.md` et `JOURNAL…:127`.
  - Portée des deux textes : ils ne sont pas strictement contradictoires. C8 vise le contrat temps sans sol, `AGENTS.md` le contrat principal brut. Mais une décision consignée comme décision de l'utilisateur y est requalifiée en préférence de développeur, sans nouvelle décision utilisateur citée.
- **D4. Tranche non commise sur un worktree en retard.**
  - HEAD local et index reposent sur `a74e90f2` ; `origin/main` a 13 commits de plus.
  - 89 fichiers sont indexés (7 962 insertions, 979 suppressions) : 53 datent de 06:29-06:53 (développeur), les autres de 09:58-10:36 (« Constructeur »). Les reçus indexés se limitent à `ground_18bits_20260922/u16_identity` (13 fichiers) et `u18_resume_20260922/preflight_center_oracle` (3).
  - Les captures R1/R2, `ground_1mm_first` et le README de `u18_resume_20260922` (≈955 fichiers) sont **non suivis**. Leurs statuts : `release`, `release_r2` et `sanitize_r2` `failed`, `sanitize` interrompu.
  - Cinq fichiers ont une version indexée différente du disque (`PASSATION.md`, `README.md`, `ELARGISSEMENT…`, `REPRISE_U18…`, coordination). `AGENTS.md`, `CONTRAT_TRAMES…` et `JOURNAL…` sont modifiés mais non indexés.
  - Un commit de l'index tel quel publierait donc un état incohérent. Un `git add -A` emporterait en plus : les brouillons float32, les modifications v6/v7 du 15 sept., les fichiers non suivis de l'auditeur complémentaire, et `claude-install.sh` à la racine (non suivi, 19:39 UTC). Précédent : `4c3cdb0c`, 305 fichiers dont 300 du constructeur (`DIALOGUE_AUDITEUR_B.md:26-45`).

### Gravité moyenne

- **D5. Reçus non autonomes.**
  - La sonde de la campagne phase 1 est épinglée à `/tmp/claude-1000/…/scratchpad/probe_phase1` (`BASELINE.only.json`, `pins.probe`) ; elle est absente aujourd'hui.
  - Les nuages sans sol 2 cm sont sous un `.gitignore` (`audits/lidar08_20260914/.gitignore:2`).
  - L'audit du 22 sept. place sources et captures dans des zip hors dépôt (`SYNTHESE_PRIORITES_LIDAR…:68-69`, `RECTANGLES_H_HA_HB…:9-12`).
  - La reprise u18 écrit que « les reçus seuls ne sont pas une archive d'exécution autonome » (`REPRISE_U18…:84-85`, [NON COMMIS]).
  - La sonde u16/u18 est dans `build/probe_a74e90f2_18bits/` ; elle est présente, et son sha256 correspond au pin.
  - Les sha256 sont épinglés, mais le rejeu dépend d'artefacts volatils.
- **D6. Portes désactivées selon l'emplacement du build.**
  - `mhgp8_q34_spatial_gate` et sa jumelle `-O` ne sont actives que dans le build r2 épinglé (`CMakeLists.txt:509-552`).
  - `mhgp8_q34_indexed_witness_mutations` est DISABLED sans condition depuis `2629a536`, car le site de mutation n'est plus unique (`CMakeLists.txt:593-600`).
  - Cinq portes de mutation se désactivent hors de `<repo>/build/`, hors Release, sous sanitizers ou sans Boost (l. 486-508, 588-591).
  - Effet mesuré par l'orchestrateur : 7 DISABLED sur 132 hors arbre ; dans l'arbre, ces portes s'exécutent et passent (8/8).
  - Le défaut est la désactivation silencieuse, pas une régression.
- **D7. Défauts ≠ configuration mesurée.**
  - Toutes les mesures utilisent `rectangle-pair boxes affine live 64` (+ `atlas` après `0948d2d0`). La commande de sonde compte 15 arguments après le binaire (`only_time_01_s00_k5_w8.txt`).
  - Les défauts restent `Disabled`, `ScalarCover`, `Legacy`, `Individual` et `q3_atlas_consultation=false` (`wspd_q34.hpp:24-26,35`).
  - La décision D3 du développeur (défauts = configuration mesurée, anciens modes dans une cible `legacy`, `AUDIT_REPRISE…:143-149`) n'est pas appliquée.
  - La voie q2 cumule cinq entrées et plusieurs options négatives (`AGENTS.md`, tranches 14-21).
- **D8. Parallélisme non démontré sur G4.**
  - Sur G4 : 1,93 à 11,13 CPU occupés sur 48 pour les trames, 3,23 pour le préfixe 8k R3.
  - La file de tâches (`5224ff4e`) n'a été mesurée qu'en local, sur 8 vCPU (686 %).
  - La session G4 de phase 2 est seulement planifiée (`JOURNAL…:71-99`). Son seuil de déclenchement, « ≥ ×5 sur la base » (`AUDIT_REPRISE…:190-191`), n'est pas atteint : ×1,96 en CPU à K5 W1, ×2,82 au mieux en mur.
- **D9. Port 18 bits sans reçu commis ni contre-lecture.**
  - `a74e90f2` n'ajoute aucun fichier sous `receipts/`.
  - L'identité « 444 compteurs » n'est affirmée que dans le message et le journal. [NON COMMIS] La relecture de la reprise la corrige : **449 compteurs**, dont cinq `terminal_*` que le filtre excluait par mégarde, tous identiques.
  - `mhgp8_wspd_q34_mutations` a échoué une fois sous charge (`JOURNAL…:128-130`).
  - [NON COMMIS] La reprise a trouvé des fabriques publiques sans contrôle de domaine : des entrées forgées pouvaient déborder (`REPRISE_U18…:25-35`). Elle a aussi trouvé un juge de centre à coordonnées inversées dans les 42 portes en cours (l. 63-68).
- **D10. Incidents de processus répétés.**
  - `ctest -N` a été lancé dans quatre builds épinglés (`AUDIT_REPRISE…:108-119`).
  - Une sentinelle `baseline.done` périmée a lancé deux campagnes en concurrence et écrasé les fichiers bruts de la scène 0 (`ground_phase1…:19-27,51-58`).
  - Un commit d'audit a emporté la tranche 34 (`4c3cdb0c`).
  - Des lecteurs et des portes erronés ont été conservés (tranches 30, 32, 34 ; u18 R1/R2).
  - Chaque incident est documenté. La cause commune est le worktree et l'index partagés par plusieurs acteurs. L'auditeur externe du 22 sept., qui travaillait depuis un clone séparé, n'a rien emporté.
- **D11. Documentation d'entrée périmée.**
  - En-têtes de `morsehgp3D_v8/README.md:5-11`, `PASSATION.md:3-5` et `JOURNAL…:3-4` : `quantized_u16_input_only`, `implementation_v8_p0`.
  - `README.md:852-879` : priorité P0, verdict 50k, « douze tranches mono », alors que GCP a servi en tranche 31.
  - `audits/ETAT_COURANT.md` n'est pas mis à jour depuis `8c050a33` et ignore le port 18 bits.
  - `CLAUDE.md:21-33` et `AGENTS.md:364-420,470` désignent encore la v5 comme chantier actif.
- **D12. Indépendance de l'audit.**
  - Les travaux du développeur (lemme de l'atlas, file de tâches, port 18 bits) n'ont aucune contre-lecture commise.
  - L'audit de clôture est conduit dans la même session Claude que la lignée B (auditeur, constructeur, développeur). Preuves : trailer `Claude-Session`, chemin de sonde dans le scratchpad de cette session.
  - Ses verdicts sur les commits de B doivent être relus par un acteur distinct avant d'engager la v9.

### Gravité basse

- **D13. Diversité des données.** Toutes les mesures LiDAR portent sur la séquence 08, trames 000000/000100/000200 (`CONTRAT_TRAMES…:27-30`). K10 et s10/s12 ne sont pas mesurés sur trame brute entière (`AGENTS.md:346`).
- **D14. Licences.** Patchwork++ (BSD-2) et ≈111 Mo de dérivés KITTI (CC BY-NC-SA) sont versionnés sans mention dans la section Licences (`AUDIT_REPRISE…:104-106`, question Q3 sans réponse ; `README.md` racine, section « Licences »). Le workflow CI d'audit consomme des nuages 1 mm dérivés.
- **D15. Hygiène du worktree.**
  - Brouillons float32 non suivis depuis le 21 sept. 20:16.
  - Modifications v6/v7 non commises datées du 15 sept. 12:15 ; fichiers v6/v7 non suivis plus anciens.
  - Deux entrées de coordination jamais commises : « AUDITEUR_COMPLEMENTAIRE » du 13 sept. et « Constructeur — raccord q3 global float32 ».
  - Fichiers de l'auditeur complémentaire non suivis depuis le 13 sept.
  - 150 builds `v8*` (13 Go), en tension avec `PLAN_DE_REFONTE.md:474-475` (« Ne pas maintenir plusieurs copies concurrentes du moteur dans `build/` »). La plupart sont toutefois des builds d'autorité épinglés, exigés par les lecteurs LIVE.
- **D16. Tailles d'intérêt détournées.** [NON COMMIS] `release_r2/scale_8000.json` porte `n=8000` pour une arête fournie de 7 tests à sortie vide (portée déclarée : `one_supplied_edge_empty_q4_stream_not_pipeline`). Les ratios G4 R3 viennent de 1k-4k. Les deux sont annoncés comme diagnostics, mais leur nommage invite à les confondre avec la règle 8k/16k/32k de `CLAUDE.md`.
- **D17. Deux moteurs.** Le moteur entier et la ligne float32 coexistent (≈9 sources ; 23 CTests étiquetés `float32`). À `12294241`, D1 et C8 placent les briques float32 en « profil sans perte qualifié, hors contrat temps » (`AUDIT_REPRISE…:132-133`). La reprise [NON COMMIS] les remet en « profil par défaut ». Leur sort reste donc disputé.

## 6. Questions ouvertes

- **Q1.** Quel est **le** régime contractuel de la v9 : trame brute entière (≈120k sites, C3), sans sol 30-60k (C6), ou les deux avec des priorités explicites ?
- **Q2.** La décision du 22 septembre (moteur entier 18 bits pour le contrat temps, float32 hors contrat temps) remplace-t-elle « float32 original par défaut » (C4) pour le contrat temps, pour le contrat principal, ou pour les deux ? L'utilisateur doit trancher entre `ELARGISSEMENT…` à `12294241` et la réécriture [NON COMMIS], avec citation de sa décision.
- **Q3.** Frontière définitive du chronomètre (D2 développeur, principes de `CONTRAT_TRAMES…:15-22`) : lecture disque incluse ? Segmentation incluse pour le régime sans sol ? Démarrage à froid ou à chaud ?
- **Q4.** Format de sortie de la tour : explicite (1,75 Go à 50k K10 au format v7) ou implicite déclaré ? C'est une condition préalable au 100 ms.
- **Q5.** Sort de la tranche [NON COMMIS] du 22 sept. : commit par ses deux auteurs après correction du lecteur (« disabled CTests differ »), avec suivi des captures R2, ou archivage ?
- **Q6.** Sort des briques et brouillons float32 : oracle de référence, archive ou retrait.
- **Q7.** Données : quelles séquences et trames, choisies avant les chronos de qualification (`CONTRAT_TRAMES…:24-27`) ? Quel statut pour les dérivés KITTI versionnés ?
- **Q8.** Organisation : un clone par acteur, ou un seul écrivain par index ?
- **Q9.** Qui audite la v9, dans une session et un clone distincts de ceux du développeur ?

## 7. À porter en v9, et à ne pas reprendre

### À porter

| Quoi | Où | Pin | Pourquoi |
| --- | --- | --- | --- |
| Contrat trame entière (principes de chronométrage, plusieurs scènes, pas de préfixe) | `morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` | `12294241` | seul texte qui définit le périmètre mesuré ; à compléter par Q1-Q4 et à corriger l. 46 |
| Protocoles sans sol et spatial, pilote Patchwork++ | `docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md`, `docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md`, `bench/*patchwork*` | `204b0620`, `759ce2b0`, Patchwork++ `3e6903a1` | masque sur IDs, float32 et IDs conservés, coupes capteur ; qualifié |
| Moteur entier u18 : index, front, flux q3/q4 parallèle, options mesurées | `src/` | `a74e90f2`, à requalifier avec les correctifs de domaine [NON COMMIS] et l'identité à 449 compteurs | seul générateur complet, exact et parallèle |
| Certificat q3 par l'atlas q4 | `docs/Q3_CERTIFICAT_ATLAS_20260921.md` | `0948d2d0` | lemme écrit + porte + mutant ; 73,8-90,7 % des graines rejetées sans census ; à faire contre-lire et à réécrire pour M = 262 143 |
| File de tâches par plages de rectangles | `src/pipeline/wspd_q34.*` | `5224ff4e`, `5fdda963` | premier gain d'occupation (686 % sur 8 vCPU) |
| Cascade rectangles (h commun, h_a/h_b, réemploi singleton, présélection négative) | `audits/SYNTHESE_PRIORITES_LIDAR_20260922.md` | `12294241` | ×2,55 à ×4,46 sur échantillons de filtrage, 1,5 M comparaisons exactes sans désaccord ; **proposé**, non porté ; ne réduit pas l'atlas |
| Chaîne q2 au profil `{2,16,true}` | `src/pipeline/wspd_q2_*` | `3e94c868`, `9ae4e28b` | nécessaire à la tour ; à mesurer sans sol |
| Discipline de preuve : oracles rationnels, mutants compilés, lecteurs normal/−O, échecs conservés, `git diff --cached --quiet \|\| exit` | `tests/`, `bench/`, `DIALOGUE_AUDITEUR_B.md:44-45` | — | a détecté des fautes réelles (mutant survivant en t34, juge de centre inversé en u18) |
| Protocole G4 v8 (budget utile 900 s, arrêt ciblé) | `gcp-migration/cpu_probe_*_v8.py`, `q34_spatial_*_v8.py` | `4dbe3024`, `70de84f2` | quatre sessions sans VM oubliée |
| De la v7 : sémantique FULL de `full_ball_tower.hpp`, facettes silencieuses, fixture ABCDE, MEB canonique à quatre pivots | `morsehgp3D_v7/src/forest/full_ball_tower.hpp` ; `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` | `324f6192` (dernière modification de la source v7), audit à `c92aad13` (base `4ccf8431`) | seule tour FULL existante ; invariants de parents ; MEB ×0,262-0,271 à K5 (microbenchmark) |
| De la v7 : décisions de reprise (propriétaire immuable, supports q ≤ 4, petits oracles, GPU résident) | `docs/AUDIT_V7_SYNTHESE.md:114-131` | `2b658cbe` | constat toujours valable |
| Budget par poste et plancher de sortie | `audits/BUDGET_CONTRAT_50K_20260914.md` § 1-2 | `f9bcb50a` (statut historique ajouté en `09361c3c`) | méthode réutilisable ; chiffres à refaire sur la tour v9 |

### À ne pas reprendre

| Piste | Mesure qui l'a fermée |
| --- | --- |
| Lots de singletons q2 | ×1,005 à ×1,225 plus lent sur 54 comparaisons (`AGENTS.md:266`, `8d615cfd`) |
| Redistribution `Donate`, équipe coopérative, plages d'ancres comme défaut | gains instables, régression sur amas 8k ; 17/18 observations rangées sans Pool régressent (tranches 14, 17, 18) |
| Nouvelles micro-variantes q2 | consigne répétée des tranches 19 à 34 ; la fenêtre 2K ne divise qu'une constante |
| `Window30` comme remplacement universel de `Local28` | plus rapide 55 fois sur 144, médiane 30/28 = 1,153 (`FAUSSES_PISTES.md:103-104`) |
| Couches duales 29 comme remplacement universel | adversaire K10 : run256 ×10,27 plus lent (`AGENTS.md:306`) |
| Carte q4 27 | 10 667 → 10 155 lectures seulement (`AGENTS.md:298`) |
| `Joined` graines×cellules | aucun gain supplémentaire stable face à `LiveOnly` (tranche 34) |
| Cache de formes des fragments ; passe conjointe des quatre cellules filles | neutre (27,32 s contre 27,32 s) ; +18 % (`JOURNAL…:45-58`) |
| Récursion relançant l'index par sous-rectangle ; reprise par listes d'IDs | plus lentes (`SYNTHESE_PRIORITES_LIDAR…:94-96`) |
| Répéter les grandes tailles sur un chemin inchangé | 8k local 1 360,996 s, 16k interrompu (`AGENTS.md`, complément 31 ; `FAUSSES_PISTES.md:122`) |
| Découpe au milieu u16 pour le float32 ; simple réduction du pas u16 ; compromis 2,5 mm | 277 niveaux sur 278 points contre 9 ; collisions et débordements (`FAUSSES_PISTES.md:52-71`) |
| Échelle d'atlas Q = 2^18 pour le port 18 bits | non retenue : Q = 2^20 conservé avec division longue exacte ([NON COMMIS] `ELARGISSEMENT…`, différence) |
| Brouillons float32 globaux non suivis | non liables, q3 seul, mono (`AUDIT_REPRISE…:66`) |
| Assimiler une sonde d'une arête ou un quart spatial à une mesure d'échelle | `CONTRAT_TRAMES…:52-58` ; D16 |

## 8. Recommandations priorisées pour la v9

1. **Clore la v8 proprement avant d'ouvrir la v9.**
   - Faire commettre ou archiver par leurs auteurs la tranche [NON COMMIS] : 89 fichiers indexés, plus ≈955 fichiers de captures non suivis dont les statuts sont `failed`, et des versions indexées différentes du disque.
   - Resynchroniser avec `origin/main` (13 commits d'avance).
   - Ne jamais utiliser `git add -A`.
   - Vérifier `git diff --cached --quiet` avant le premier commit v9 (précédent `4c3cdb0c`).
   - Commettre la v9 depuis un clone propre.
2. **Écrire un contrat v9 unique et court en tête d'`AGENTS.md`**, avec date et provenance :
   - le régime (Q1) ;
   - la précision (Q2), en citant la décision de l'utilisateur telle que consignée à `12294241`, sans la requalifier ;
   - la frontière du chronomètre (Q3) et le format de sortie (Q4) ;
   - des scènes figées (Q7).

   Déplacer les paragraphes de tranches hors d'`AGENTS.md`. Corriger `CONTRAT_TRAMES…:46`, `PASSATION.md:54-55` et `CLAUDE.md` (qui désigne encore la v5).
3. **Construire d'abord la tour de bout en bout, même lente** : q2 dans le même appel, catalogue canonique et arité minimale, intérieurs, fold K=1..5 puis 1..10, lanceur FULL chronométré. Porter explicitement la sémantique de `full_ball_tower.hpp` v7 (`324f6192`) avec la fixture ABCDE. Sans cela, aucun gain ne se rapporte au contrat.
4. **Un seul moteur, entier 18 bits.**
   - Requalifier `a74e90f2` avec les correctifs de domaine et l'identité à 449 compteurs, dans un reçu commis.
   - Faire des défauts la configuration mesurée ; placer les modes historiques dans une cible différentielle.
   - Figer le statut des briques float32 : oracle ou archive.
5. **Réduire le travail là où il est mesuré** : partition de l'atlas (≈52 %), cascade rectangles (×2,55-×4,46 sur échantillons, sans effet sur l'atlas), certificats collectifs d'arête. Ajouter un filtre flottant certifié à repli exact avant tout GPU (`AUDIT_REPRISE…:179-181`).
6. **Mesurer le parallélisme sur G4 dès la tour minimale** : occupation par worker ; W1/W8/W48 bit-identiques ; trois scènes, sans sol et brutes ; K5/K10. Publier le CPU·s en W1 à côté du CPU·s en SMT.
7. **Rendre les reçus rejouables.**
   - Sondes dans des builds `build/v9_*` épinglés, jamais dans `/tmp`.
   - Entrées versionnées, ou régénérables par un script haché.
   - Preuves d'audit dans le dépôt, pas en pièce jointe.
   - Budget de volume pour les reçus (979 Mo en v8) ; suppression des builds de développement.
8. **Portes.**
   - Réactiver `mhgp8_q34_indexed_witness_mutations` (ou son équivalent v9).
   - Découpler la porte spatiale d'un build unique.
   - Échouer bruyamment plutôt que désactiver silencieusement selon l'emplacement du build.
   - Inscrire les portes courtes v9 dans la CI GitHub.
9. **Gouvernance.**
   - Un clone Git par acteur, ou un seul écrivain par index.
   - Rôles distingués par un trailer de commit obligatoire, étendu à tous les acteurs (seule la lignée B en portait).
   - Un canal de coordination réellement utilisé par le développeur.
   - Un auditeur v9 dans une session distincte de celle du développeur.
   - Interdire `ctest` dans un build épinglé, et interdire les sentinelles réutilisées.
10. **Données et licences** : au moins une seconde séquence, choisie avant qualification ; une décision sur les dérivés KITTI ; la mention de Patchwork++ (BSD-2).

## Contre-vérification

Chaque affirmation principale et chaque chiffre du rapport d'origine ont été confrontés à leur preuve : fichier et ligne à `12294241`, commit, reçu JSON, journal de l'orchestrateur. Corrections intégrées ci-dessus :

1. **Commits.** 201 et 169 sont confirmés. Le chiffre de 158 de la tâche est retrouvé : commits touchant `morsehgp3D_v8/` entre `2b658cbe^` et `a74e90f2` (HEAD local). Le rapport d'origine disait qu'il était introuvable.
2. **Heures de l'audit externe.** 10:36-19:40 UTC (12:36-21:40 +0200), et non « 12:36-19:40 », qui mélangeait deux fuseaux. Douze de ses commits touchent un fichier, le dernier deux. Il travaillait depuis un clone séparé basé sur `a74e90f2`.
3. **Chronologie.** Les commits `ae98dbfa` → `c92aad13` (19-20 sept.) sont l'audit indépendant des facettes silencieuses, pas le constructeur 2. `ec2bc503` (20:40) est du développeur : trailer, `AUDIT_REPRISE…:8`, message.
4. **Tranche non commise.** Elle ne date pas seulement de 10:02-10:56. Elle mêle deux acteurs : 53 fichiers indexés de 06:29-06:53 (développeur, après `a74e90f2`) et la reprise « Constructeur » de 09:58-10:56.
5. **Captures R2.** Elles ne font pas partie des 89 fichiers indexés : elles sont **non suivies** (≈955 fichiers sous `u18_resume_20260922`). Les seuls reçus indexés sont `u16_identity` (13) et `preflight_center_oracle` (3). `sanitize_r2` échoue sur 8 tests désactivés, pas sur 3.
6. **Retard sur `origin/main`.** Le HEAD local, et pas seulement l'index, est en retard de 13 commits. Cinq fichiers ont une version indexée différente du disque. La modification d'`AGENTS.md` est non indexée.
7. **Réécriture d'`AGENTS.md` [NON COMMIS].** Elle corrige aussi la l. 18 (« élargi à u18 »). `PASSATION.md` (partie historique) et `JOURNAL…:127` gardent « décision utilisateur ». La portée de C8 (contrat temps sans sol) et celle d'`AGENTS.md` (contrat principal) sont précisées.
8. **Écart au budget.** Le calcul ×15,4 / ×46,1 est juste, mais il porte sur du CPU·s en W8 sur un hôte SMT. En W1, K5 donne ×9,4. Sur G4, sur trame brute, K5 donne ×8,0 à ×20,3 en CPU et ×34 à ×505 en mur. Ajouts.
9. **C7.** Le contrat contient déjà des règles de chronométrage (`CONTRAT_TRAMES…:15-22,67-69`) : C7 est « partiellement inscrit », pas seulement « proposé ».
10. **Plancher d'écriture.** Le plancher de 1,75 Go ne rend pas le 100 ms « incompatible » : il laisse < 40 ms au reste. La mesure a été faite sur l'hôte local, et la note est déclarée historique (`BUDGET…:8`). Le pin d'origine `85015a8c` a été remplacé par `f9bcb50a`/`09361c3c`.
11. **Certificat de l'atlas.** Il y a un mutant causal propre au certificat, dans un script de quatre mutants (pas « 4 mutants »). Aucune contre-lecture indépendante ; bornes écrites pour M = 65 535.
12. **Commande de sonde.** 15 arguments après le binaire, pas « 13 jetons ».
13. **Pilote sans sol.** 44 et 17 sont des commandes Release et SAN, pour 20 tests (`AGENTS.md:50`).
14. **Identité de `a74e90f2`.** Portée à 449 compteurs par la reprise [NON COMMIS] : 5 `terminal_*` avaient été omis. Ajout du défaut de domaine des fabriques publiques et du juge de centre inversé.
15. **Mesures u16 sur u18 [NON COMMIS].** La charge n'était pas « non déclarée » : le reçu enregistre load 8,4 à 10,0 avant chaque ligne. L'écart CPU est de +5,1 % à +8,3 %.
16. **Volumes à `12294241`.** Reçus : 979 311 698 octets et 16 016 fichiers (le rapport d'origine comptait 16 fichiers indexés non commis). Audits : 39 232 414 octets. `src/` : 78 fichiers source + 9 `.gitkeep`, pas 87 fichiers. Les dossiers `src/forest` etc. contiennent un `.gitkeep`, ils ne sont pas vides à 0 fichier.
17. **Portes désactivées.** Dans un build sous `<v8_head>/build`, les portes de mutation dépendantes de l'emplacement s'exécutent et passent (8/8, `mut_ctest.log`) ; seule la mutation de témoins reste désactivée. D6 est reformulé comme un défaut de désactivation silencieuse.
18. **Gouvernance.** La lignée B est traçable par trailers : `Co-Authored-By` sur 80 commits, `Claude-Session` sur 43. La durée des rôles de B est de 8 jours (14-21 sept.), pas 5. Ajout de D12 : même session Claude pour B, le développeur et cet audit ; aucune contre-lecture des commits du développeur.
19. **Pins et références corrigés.** Protocole G4 : `4dbe3024` + `70de84f2`. Source v7 : `324f6192`. `PASSATION.md:635-637`. `CMakeLists.txt` et `wspd_q34.hpp:24-26,35`. Pour R3, l'occupation de 3,23 CPU est ajoutée (`lidar_global…:150-151`).
20. **Cascade rectangles.** Réserve ajoutée : atlas, graines et FULL sont exclus, et la cascade « ne supprime pas d'atlas supplémentaires ».
21. **Documents d'entrée.** Ajout des contradictions : `PASSATION.md:36-40` contre `54-55`, `CONTRAT_TRAMES…:46`, et `ETAT_COURANT.md` resté à `8c050a33`.
22. **Mesures et sondes.** La campagne phase 1 a été mesurée sur un worktree sale, avec un runner différent entre `BASELINE.json` et `BASELINE.only.json`. La seule mesure 1 mm utilise la sonde du build R1, dont la qualification est en échec (lecteurs Python).
23. **Hygiène.** Ajout de `claude-install.sh` (non suivi, racine) et des fichiers non suivis de l'auditeur complémentaire.
24. **Builds.** « Contraire au plan » (150 builds) devient « en tension » : ce sont surtout des builds d'autorité épinglés.
25. **Confirmés sans changement.** Tous les chiffres G4 (trames et R3), locaux W4, de base et de phase 1-2 ; les exposants ; les effectifs sans sol ; l'incident `4c3cdb0c` (305 fichiers = 5 + 300) ; la sentinelle ; le `.gitignore` ; les zip hors dépôt ; `AGENTS.md` ×3,8 ; les 150 builds (13 Go) ; les 4 sessions G4 ; les fausses pistes et leurs mesures.
