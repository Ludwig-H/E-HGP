# Provenance des sources de la v12

7 octobre 2026. **Règle** : tout ce que la v12 reprend d'une version précédente est un **port explicite**, épinglé par
commit (et par SHA-256 du fichier quand il vient d'un reçu ou d'une archive), puis **requalifié** par les portes de la
v12. Aucune qualification n'est héritée. Un port se consigne dans ce fichier, au commit qui l'introduit : source, pin,
fichiers, adaptations, portes qui le requalifient.

## 1. Versions sources

| Source | Pin | Rôle pour la v12 |
| --- | --- | --- |
| v11, moteur gelé | `ac081a06f` (moteur identique à `733912e65`) | source différentielle principale ; empreintes de référence ; ports (harnais, `num`, index, oracle, session) |
| v11, documents de clôture | `aca764660` (passation, audit final), `33c2ae3c8` (audit géant) | état au gel, corrections, décisions |
| v10, moteur mesuré | `777406b82` (session G4 4 du 29 septembre) | référence de temps ; mécanismes de la tour (plus petite boule proposée puis certifiée, saut vers les $k$ plus proches, mémo de cellule) |
| v10 figée pour le différentiel | `c764e121a` ; archive `build/v11-full-data-20261002/v10_frozen_c764.tar.gz`, épinglée par `morsehgp3D_v11/tests/tower/v10_frozen_manifest.json` (34 fichiers) | portes `diff_v10` de l'oracle (190 nuages, 640 tours) |
| v10, raccord R2 | `865f5e6`, dépôt local `build/v10-integration-r2/src`, **jamais importé** | version la plus durcie des fondations v10 et de ses filtres (refus de fast-math, gardes FE) |
| v10, prototype de la feuille J3 | `morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/` (`leaf.hpp`, `J3_feuille_v3.patch`) | candidat de feuille data-parallèle ; **jamais dans le moteur v10** |
| v9 | preuve d'Euler par le nerf, `morsehgp3D_v9/audits/CONTRELEC_EULER_PAR_NERF_20260923.md` (`559c8ab84`) | preuve complète de `JUG-EULER` |

## 2. Conceptions et notes, versées le 7 octobre

| Pièce | Emplacement | SHA-256 (préfixe) |
| --- | --- | --- |
| conception de la tour de la v11 (D-G1 à D-G5, D-F1 à D-F3, D-V1, lemmes T1, T3–T7) | `morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md` | `01e217f0` |
| conception du générateur de la v11 (feuille par étages sur lots, arithmétique par paliers, F6 semi-statique, émission compacte) | `…/conception/CONCEPTION_GENERATEUR.md` | `0fc30de4` |
| pistes de rupture | `morsehgp3D_v11/receipts/notes_hors_depot_20261007/conception/PISTES_DE_RUPTURE.md` | `e093f292` |
| contrôles de la conception (noyau sans lots, contraction, historique d'attache, quotient des coquilles) | `…/conception/preuves_tour/`, `preuves_generateur/`, `preuves_pistes_de_rupture/` | `SHA256SUMS` du reçu |
| brouillon mathématique (dont le modèle par copies des multiplicités) | `…/mathematiques/brouillon/MATHEMATIQUES.assemble.md` | `f829827d` |
| audit de la v10 en lentilles (L01–L08, L10 ; preuves L01–L14) | `…/audit_v10/` | L02 `ecb3231a`, L03 `b84c5088` |
| plan 100 ms, audit des transpositions, cartes GPU (4 et 6 octobre) | `morsehgp3D_v11/receipts/notes_hors_depot_20261007/` | `SHA256SUMS` du reçu |
| rapports bruts de l'audit géant (A à H), scripts, vérification locale | `morsehgp3D_v11/receipts/audit_geant_v11_20261007/` | `SHA256SUMS` du reçu |

Ce sont des notes de travail : leurs estimations ne sont pas des mesures, et la v11 ne les a pas implantées.

## 3. Ce que la v12 porte de la v11 (`ac081a06f`)

| Pièce | Chemin dans la v11 | Porte de requalification attendue |
| --- | --- | --- |
| contrat mathématique | `docs/MATHEMATIQUES.md` § 1–8 et § 10 | réécrit avec identifiants uniques ([`OBJET_ET_CONTRAT_MATHEMATIQUE.md`](OBJET_ET_CONTRAT_MATHEMATIQUE.md)) |
| oracle borné et juge | `reference/` (étages A et B, intervalles, S1, familles SplitMix64, mutants) | suites rapide et complète, mutants de l'oracle |
| doctrine numérique | `src/num/` (budgets, prédicats, certificats, clés F3/F4, `Level`, racines, sommes de radicaux) | portes numériques aux bornes exactes, quatre modes d'arrondi |
| budget mémoire et tampons | `src/core/buffer.*` | refus sans publication partielle ; cache **compté** |
| index | `src/index/` (arbre radix de Morton, bornes sur sites) | identité des recensements |
| plateau, numérotation, naturalité | `src/tower/forest_plateau.cpp`, `forest_build.cpp` | oracle de la forêt, W1 contre W48 |
| supports, points, tête plate | `src/supports/`, `src/points/`, `src/head/` | différentiels contre Python et l'oracle |
| dossier transactionnel, manifeste | `src/io/`, `src/api/` | doubles échecs, publication atomique |
| harnais | `cmake/run_expect.cmake`, `cmake/gates.cmake`, `tests/mutants/run_mutants.py`, `tests/support/` | portes du harnais par injection de fautes |
| session G4 gardée | `gcp-migration/v11_session.py`, `v11_worker.sh`, `README_V11.md` | autotest hors ligne, puis une session à blanc |
| préparation des trames | `bench/points_lidar_prepare.py` | empreintes de la trame 08/000040 et du masque de 08/000000 |

### 3.1 Ports réalisés

| Tranche | Contenu | Table fichier par fichier | Qualification locale |
| --- | --- | --- | --- |
| T0, socle (7 octobre 2026) | modules `core`, `num`, `sched`, `cloud`, `io`, `index` (5 475 lignes de C++), harnais de portes, lanceur de mutants, contrôle de style, oracle borné `reference/` (`hgp12_ref`), sonde d'index ; profils u21 et u24 ; 18 refusé (D6), 32 refusé jusqu'au repère local | [`PORTS.md`](PORTS.md) : 210 fichiers, SHA-256 de chaque source à `ac081a06f`, 28 copies, 112 renommages, 69 adaptations décrites | Release u21 et u24 sans avertissement ; portes hors `long` vertes (u21 : 408 sur 409, la sentinelle LiDAR sautée faute de données ; 26 portes `diff_v10` contre la v10 figée) ; ni sanitizers, ni matrice G4, ni suite longue de l'oracle |

Le socle porte le code de la v11 **tel quel** pour $B\leq 24$ : les corrections du contrat numérique (repère local,
certificats liés à leur domaine, clé de Morton exacte, boîtes à 33 bits, test du milieu local) s'y appliqueront par
tranches, chacune avec ses portes ; tant qu'elles manquent, u32 reste refusé à la configuration.

## 4. Ce que la v12 porte de la v10 (`777406b82`, R2 `865f5e6`)

| Mécanisme | Où | Note |
| --- | --- | --- |
| plus petite boule proposée en flottant (Welzl), certifiée en exact, repli exact | `morsehgp3D_v10/src/tower/tower.cpp` (proposition l. 250–440, certificat l. 468–514) | 0 repli sur 7,66 M à K10 ; à reformuler sous F1–F6, sans marges flottantes figées |
| saut vers les $k$ plus proches | idem, l. 905–934 | comparateur exact au lieu de la marge `kApproxMargin` |
| mémo de cellule daté | idem, l. 951–991 | sous forme **déterministe** (`LEM-T3`), pas atomique *relaxed* |
| table M(K) des tailles de feuille (12 jusqu'à K = 3, 16 jusqu'à K = 6, 24 jusqu'à K = 10, 28 au-delà) | `morsehgp3D_v10/src/catalogue/generator.cpp`, l. 641 | une table par K **et par voie** (la voie GPU de la v11 préférait 24 dès K5) |
| frontière en largeur jusqu'à $64P$ tâches | R2, `generator.cpp` | candidate pour la référence CPU |

**Ne pas porter** de la v10 : les marges flottantes figées, le Kruskal par lots, la boule fermée entière du `SiteTree`
(jusqu'à 1 258 sites), les monolithes.

## 5. Données et outillage hors dépôt

| Élément | Emplacement | Note |
| --- | --- | --- |
| trames ng00–02 et uniformes | `build/v11-full-data-20261002/` (`manifest.json`, `restoration.json`, `restore.py`) | empreintes au manifeste ; jamais versionnées |
| scans bruts SemanticKITTI | `build/v11-persist/kitti_cache/` (201 trames de plusieurs séquences), `build/v11-persist/data_points3/` | base des nouvelles trames de la v12 |
| retrait du sol | Patchwork++ `3e6903a1d5537a4cc2ace897b0bbb98a92d6014c` (sonde de la v8) | masque de 08/000000 `9db3fe5c…` |
| VM G4 | cible gardée `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, projet `devpod-gpu-exploration` (`gcp-migration/README_V11.md`) | GCC 11.4, CMake 3.22.1, Python 3.10 sans pip, 48 fils, GPU sm_120 |
| `sklearn` | 1.7.2 sur G4, 1.9.1 sur le codespace | à épingler sur un même banc pour T5 |
