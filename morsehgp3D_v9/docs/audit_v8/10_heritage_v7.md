# Lentille 10/12 — Ce qui était bon en v7 et doit guider la v9 (version contre-vérifiée)

Audit en lecture seule du 22 septembre 2026, préparé pour l'ouverture de la v9. Cette version reprend le rapport `10_heritage_v7.md` après une contre-vérification adversariale. Chaque affirmation principale et chaque chiffre y ont été confrontés à la source citée. Les corrections sont intégrées au texte et récapitulées au § 9.

```text
phase=audit_general_v8_vers_v9_hors_registre
backend=aucun (lecture seule)
profile=quantized_u16_input_only (objet v7 audité)
mode=audit_independant_math_and_architecture
public_status=not_claimed
GCP non utilisé
```

**Référence.** Le worktree détaché est `origin/main` à `12294241`. Le dernier commit qui touche `morsehgp3D_v7/` est `dc57ffd5` (11 septembre 2026, 20:29 UTC). Tous les fichiers v7 cités sont donc dans l'état de `dc57ffd5`, et les chemins sont relatifs à la racine du dépôt.

**Ce qui n'a pas été fait.** Aucun build, test, benchmark ni script du dépôt n'a été exécuté. Les lecteurs `verify.py` n'ont pas été relancés ; leur exécution du 13 septembre est citée depuis le reçu v8 qui la consigne.

**Vocabulaire de statut.**

- **prouvé** : preuve écrite et fixture.
- **testé** : porte bornée.
- **mesuré** : reçu épinglé avec sources, sha256 et sorties.
- **proposé**, **manquant**.
- **non vérifiable** : chiffre sans reçu.

Aucun temps v7 ne provient d'une trame LiDAR, et **toutes les tours FULL v7 portent sur la famille uniforme u16**. Les petites tailles servent d'oracles de correction.

## 1. Périmètre lu

### Lu intégralement (auditeur initial, puis contre-vérifié sur les lignes citées)

| Fichier | Rôle |
| --- | --- |
| `morsehgp3D_v7/README.md`, `morsehgp3D_v7/PASSATION.md` | état v7 à `dc57ffd5` |
| `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md` | preuve de l'objet FULL régulier |
| `morsehgp3D_v7/docs/CONTRAT_CERTIFICAT_FULL.md`, `CONTRAT_DIGEST_FULL.md`, `CONTRAT_PRODUCTEUR_FULL_GABRIEL.md`, `CONTRAT_NORMALISATION_FULL.md` | certificat, digest, producteur, normalisation |
| `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md`, `SQUELETTE_MINIMA_GABRIEL.md`, `CROISSANCE_ET_BORNE_DE_SORTIE.md` | plateaux, minima, borne de sortie |
| `morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md`, `RESULTATS_TOUR_BOULES_20260910.md`, `RESULTATS_TOUR_CACHE_G4_20260910.md` | tour par boules et mesures |
| `morsehgp3D_v7/docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md`, `GARDES_RANGS_CERTIFIES_20260911.md`, `PARALLELISATION_PAR_LOTS_20260911.md`, `CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md`, `RESULTATS_PRIMITIVES_GPU_20260911.md` | T2 K10, rangs, lots, naissances, GPU |
| `morsehgp3D_v7/docs/FAUSSES_PISTES.md` (version commise) | fausses pistes |
| `morsehgp3D_v7/audits/ETAT_COURANT.md`, `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md`, `CONTRAT_VERTICAL_COURANT.md`, `CERTIFICAT_FULL_CPP_COURANT.md`, `CONTRAT_MASSES_VOTE_COURANT.md` | audits v7 courants |
| `morsehgp3D_v7/audits/NOTE_CLAUDE_COEUR_MEB_20260911.md`, `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md` (intégral lors de la contre-vérification) | second auditeur : noyau MEB, découpe de `tower_s` |
| `morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md`, `morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md`, `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md`, `CONTRATS_ET_MESURES.md` | décisions et audits v8 sur la v7 |

### Lu partiellement

| Fichier | Portion |
| --- | --- |
| `morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md` | § 1 à 3 (l. 11–145), titres du reste |
| `morsehgp3D_v7/docs/RESIDENCE_MASSIVE.md` | titres, § 4–5 (l. 334–428) |
| `morsehgp3D_v8/audits/IMPLEMENTATION_PARALLELISATION.md` | § 2 (l. 41–73), § 7 (l. 255–292) |
| `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` | l. 50–95, 185–205 |
| `morsehgp3D_v8/audits/BUDGET_CONTRAT_50K_20260914.md` | l. 38–55 |
| `morsehgp3D_v8/PASSATION.md` | l. 1–45, 663–696 (contexte de section vérifié) |
| `AGENTS.md` | l. 1–60 (contrat trame entière, sans sol) |
| `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md`, `CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` | l. 1–12, 40–52 |
| `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` | l. 38–42, 125–136 |
| `morsehgp3D_v4/docs/MATHEMATIQUES.md` | § 2.1 (l. 130–158), § 5.1–5.2bis (l. 496–578) |
| `morsehgp3D_v7/src/forest/full_ball_tower.hpp` | l. 109, 215, 451, 478–480, 651, 756, 859, 921–925 |
| `gcp-migration/cpu_probe_session_v8.py`, `q34_spatial_session_v8.py` | en-têtes et import du contrôleur v7 |
| `audits/COORDINATION_MORSEHGP3D_V8.md` (racine) | recherche par motifs `v7`, `FULL`, `full_ball_tower` seulement |

### Reçus et données relus directement (JSON, stderr)

| Reçu | Ce qui a été relu |
| --- | --- |
| `morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/{cpu,gpu}_n50000_k{10,5}_s8.summary.json`, `pair_n50000_k{10,5}_s8.json`, `device_gate.summary.json`, `*.stderr`, `cpu_compiled_dependencies.json` | tous les champs cités ; en-tête compilé `full_ball_tower.hpp` = `910f45ba…` |
| `…/full_ball_scale_gpu_20260910/gcp/optimized/host/receipt.json`, `guarded_stop.stdout` | `targeted_shutdown_certified: true`, contrôleur `177b25a0…`, worker `80b94a5a…`, « état GCE TERMINATED » |
| `…/full_ball_scale_gpu_20260910/local/*.stderr`, `manifest.json` (`c9913094…`) | RSS 8k/16k/32k et s10/s12 |
| `morsehgp3D_v7/receipts/ball_resolver_residence_20260910/summary.json`, `MANIFEST.json` | en-tête qualifié `910f45ba…` |
| `morsehgp3D_v7/receipts/census_tower_permanent_20260911/README.md`, `full_ball_batch_permanent_20260911/README.md` | qualifications propres à `83f1c78e…` |
| `morsehgp3D_v7/receipts/post_exchange_scale_20260911/storage_map.json` et objets de résultats | en-tête `6763a877…` ; triplet mono 8k/32k |
| `morsehgp3D_v7/receipts/parallel_birth_streaming_20260911/objects/*` | triplet CPU4 8k/16k/32k, paire 1/1/1→1/4/1 |
| `morsehgp3D_v7/receipts/rank_guard_streaming_20260911/MANIFEST.json`, objet `8cc9dadd…` | décomposition 32k, drapeaux `physical_comparison_executed` et `geometric_oracle_executed` |
| `morsehgp3D_v7/receipts/atlas_graph_full_20260911/README.md`, `rank_atlas_20260911/manifest.json` | mode de comparaison du graphe daté ; emplacement des sources privées |
| `morsehgp3D_v7/receipts/gpu_primitives_g4_20260911/objects/{2436eaa8…,041bc9ce…,279cbf8f…}` | deux gates exécutées sur carte |
| `morsehgp3D_v8/receipts/audit_v7_20260913/CONTRACT_CHECKS.json` (`30d83a7f…`) | six exécutions `verify.py` du 13 septembre, codes 0 |

Vingt-deux empreintes sha256 de fichiers v7 ont été recalculées sur le worktree détaché ; toutes coïncident avec les pins du § 7.

### Historique

`git log -- morsehgp3D_v7` compte 122 commits, de `e0e6396b` (4 septembre) à `dc57ffd5` (11 septembre). Ils se répartissent sur **cinq journées d'activité** : 7 le 4, 33 le 5, 26 le 6, 10 le 10 et 46 le 11 septembre ; il n'y en a aucun du 7 au 9. Les dates et sujets des 51 commits cités au § 2 ont été contrôlés.

Commits d'introduction des audits v8 sur la v7 :

- `2b658cbe` (13/09) ;
- `ae98dbfa` (19/09 22:30) puis `c92aad13` (20/09) ;
- `8c050a33` (21/09).

### Non lu

- Sources v7 hors lignes échantillonnées (`src/` compte 18 189 lignes), `bench/`, les 83 fichiers de `tests/` et `cli/`.
- Environ 120 des 134 paquets `morsehgp3D_v7/receipts/` et la plupart des 48 paquets `morsehgp3D_v7/audits/receipts_*`.
- `morsehgp3D_v7/docs/` : `ELIMINATION_BLOCS_WSPD.md`, `PROPOSITION_MEB_ET_BUDGETS.md`, `RESULTATS_MEB_*`, `RESULTATS_MONO_*`, `PORT_GPU_RESOLUTIONS.md`, `GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md`, `RESOLUTION_STATIQUE_CPU_20260911.md`, `SEMIS_APRES_ECHANGE_20260911.md`, `INCIDENCES_SILENCIEUSES.md`.
- `morsehgp3D_v7/audits/DIALOGUE_COURANT.md`, `COORDINATION_AUDITEURS.md`, `NOTE_CLAUDE_COUT_PORTE_ET_GPU_20260911.md`, `NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md`.
- `audits/COORDINATION_MORSEHGP3D_V7.md`, en entier.
- `audits/COORDINATION_MORSEHGP3D_V8.md` (3 850 lignes), hors recherche par motifs.
- Worktree partagé : la tranche v8 non commise n'a été lue que par une recherche de `v7`/`FULL`. Seul le delta v7 non commis a été inspecté (§ 5).

## 2. Ce qui a été fait en v7

La v7 s'étend du 4 au 11 septembre 2026, sur `main`, avec cinq journées de commits. Un constructeur et deux auditeurs indépendants y ont travaillé ; tous les commits portent la même identité Git.

| Date | Jalon | Commits | Statut |
| --- | --- | --- | --- |
| 04/09 | Port v6 épinglé, primitives exactes mono, portes arithmétiques, G4 sur l'objet réduit | `e0e6396b`, `d9e4ee01`, `d2b27058`, `b8aef528` | testé ; objet réduit, pas FULL |
| 05/09 | Objet FULL : niveaux Gabriel suffisants, certificat compact v1, producteur horizontal eager/lazy, lots unitaires, normalisation v2, MEB à double budget | `94a3513b`, `a3b1b271`, `f4c0734c`, `6446e248`, `98bb6578`, `6126b373`, `b2f0dc08`, `5633bc5a`, `b44e35be` | prouvé conditionnel + testé relatif |
| 06/09 | Borne de sortie quadratique, tour K-NN par descente de facettes, quotient des minima, refus G4 50k (coquilles supplémentaires), ancres de boule, parents globaux des coquilles réelles 50k, journal daté v2 | `08cf65dc`, `dad414cb`, `a9ce3639`, `638205bb`, `30d2a4dd`, `a22a65f9`, `1fbe49d3` | prouvé + testé ; refus 50k conservés |
| 10/09 | Tour par boules avec cartes verticales ; cache exact de résolutions (en-tête `910f45ba…`) ; tours 50k K1..10 et K1..5 sur G4 | `d188e3de`, `11cde758`, `ad7ffd28` | testé borné + mesuré |
| 11/09 matin | Résolution statique dédoublonnée (en-tête `33e7d05e…`) ; rejeu du cache ; primitives GPU sur G4 ; T2 réel census→FULL jusqu'à K10 ; semis après échange (en-tête `6763a877…`) | `ce842a3f`, `223a3897`, `c03f6be8`, `47cd85b4`, `44668dd9` | testé + mesuré local |
| 11/09 après-midi | Couture transactionnelle par lots (en-tête final `83f1c78e…`) et portes permanentes ; tentatives G4 closes sans calcul ; objets parallèles, certificats composables, flux ordonné | `324f6192`, `0db1e775`, `a97ee819`, `f2bea998`, `679f4a6f`, `069bb6a2`, `ac3e8b9f` | testé ; prototypes privés |
| 11/09 soir | Contraction des pivots sur naissances, workers géométriques, préparation partagée, gardes par rangs certifiés | `03198682`, `34db4b3e`, `e3903b2a`, `19049adf`, `dc57ffd5` | testé + mesuré CPU4 ; privé |
| 11/09 (auditeurs) | Noyau MEB mesuré, réfuté puis réparé ; découpe de `tower_s` (sur `6763a877…`) ; retraits des erreurs × 20 et « séquentiel par construction » ; lots groupés ; marques au premier parcours | `d2ed4d48`, `552ef940`, `9fdf0b5f`, `abc960ac`, `602adc51`, `a0f358e9`, `30c10246`, `1b56359a`, `7ccd9d6e`, `93f4d110`, `2970d679` | prototypes d'audit hors moteur |
| après `dc57ffd5` | Delta constructeur « marques au premier parcours » raccordé aux vrais census, **jamais commis** | worktree partagé seulement | non vérifiable (§ 5) |

Volume de la v7 à HEAD :

- 26 777 fichiers, pour 471 Mo ;
- 134 paquets de reçus constructeur et 48 paquets de reçus auditeurs ;
- `src/` de 18 189 lignes.

### Lignée des en-têtes du Builder

Chaque chiffre a été acquis sur un en-tête précis ; aucun n'est transférable à un autre.

| En-tête `full_ball_tower.hpp` | Delta | Qualification | Mesures |
| --- | --- | --- | --- |
| `910f45ba…` (10/09) | cache exact, semis fermés, résidence | 28 nuages, 112 ordres, 2 508 coupes, 45 948 verticales, 170 320 contrôles O2/SAN, six mutants (`receipts/ball_resolver_residence_20260910`) | **seules tours 50k G4** ; triplet mono « cache » 8k/16k/32k |
| `33e7d05e…` (11/09) | résolution statique optionnelle | 30 nuages, 124 ordres, 75 136 verticales, 4 498 contrôles physiques ; T2 K10 (sources `c03f6be8`) | triplet statique 8k/16k/32k (MEB −32,8/33,8/34,2 %) |
| `6763a877…` (11/09) | semis après échange | 34 nuages, 150 ordres, 87 230 verticales, 5 704 comparaisons ; T2 propre ; 449/449 CTests (second auditeur, 3 426 s) | triplet mono 141,366/318,968/694,459 s ; noyau MEB et découpe des auditeurs |
| `83f1c78e…` (11/09, actif à `dc57ffd5`) | couture optionnelle par lots, callback transactionnel | couture 34 nuages/150 ordres/87 230 verticales par bras ; callback 298 742 contrôles Gram/Gamma (n ≤ 8) ; T2 K10 par overlay (12 315 725 contrôles) ; 40 CTests ciblés Release | **aucune mesure 8k–50k** ; sonde n200 seulement |

Sources de ce tableau :

- `morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md:136-153` ;
- `morsehgp3D_v7/PASSATION.md:77-124` ;
- `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md:76-77` ;
- `morsehgp3D_v7/docs/PARALLELISATION_PAR_LOTS_20260911.md:39-47` ;
- `morsehgp3D_v7/audits/NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:293-295` ;
- `git show 49b793be:…` (sha `6763a877…`).

## 3. État par composant

### 3.1 Objet mathématique et certificats

| Composant | Statut | Preuve |
| --- | --- | --- |
| Objet FULL régulier : feuilles = minima Gabriel de cardinal K ; nœuds internes = vraies multifusions aux niveaux Gabriel de cardinal K+1 ; parents pré-lot ; couverture = union des feuilles | **prouvé** (théorème conditionnel, contrelecture indépendante, fixtures) | `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md:59-114`, `:268-298` ; `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:5-17` ; `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:131-132` |
| Portails silencieux internes au constructeur, pas nœuds de sortie, mais nécessaires aux parents | **prouvé** conditionnel | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:133` ; E5 `AUDIT_NIVEAUX_GABRIEL_20260905.md:138-162` |
| Réfutation du fold des seules cofaces Gabriel comme FULL | **prouvé** (`false_in_general`) | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:129`, `:40` (Prop. 6) ; `AUDIT_NIVEAUX_GABRIEL_20260905.md:30` ; fixture `tests/fixtures/regressions/gabriel_point_set_counterexample.json` (`0c74c429…`) |
| Réfutation des minima munis de leurs seules adjacences induites | **prouvé**, avec deux fixtures u16 distinctes (détail ci-dessous) | `SQUELETTE_MINIMA_GABRIEL.md:44-78` + porte `tests/full_gabriel_minima_quotient_gate.py:283-285` ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:29` + `audits/receipts_gabriel_vertices_20260906/` |
| Extension non régulière : naissances couvrant plus de K points, contributions datées éventuellement redondantes, ancres par (K, BallKey) même inertes, fenêtre p+q_min ≤ min(Kmax+1, n) | **prouvé** conditionnel + **testé** local | `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md:31-47`, `:76-107` ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:19-25` |
| Parents globaux des coquilles réelles 50k : 174406 (K5, un parent), 254569 (K2, deux), 996863 (K6, deux) | **prouvé** sur l'entrée épinglée (certificats rationnels) | `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/GLOBAL_PARENTS.md` ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:25` |
| Verticale : ancre inférieure de la même boule après fermeture du plateau, puis normalisation à la coupe | **prouvé** conditionnel + **testé** (T2) | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:134` ; `CONTRAT_VERTICAL_COURANT.md:3-31` |
| Borne de sortie : m² minima FULL de cardinal 2 sur N = 2m sites réguliers, N = 2m+K−2 à K fixé | **prouvé** + porte rationnelle (2 008 identités, huit mutants) | `CROISSANCE_ET_BORNE_DE_SORTIE.md:30-77`, `:142-161` ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:35-39` |
| Poids de l'Algorithme 1 (univers contributif, dates d'affectation) | **proposé** (contrat) ; **manquant** (code) | `CONTRAT_MASSES_VOTE_COURANT.md:3-27` ; `STATUT_PREUVES_ET_HEURISTIQUES.md:135-136` |
| Certificat compact v1 `full_minima_merge_forest_v1` | **testé** (`structural_only`) | `CONTRAT_CERTIFICAT_FULL.md:7` |
| Journal daté v2 `full_dated_coverage_forest_v2` | **testé** O2/SAN, audit indépendant du tableau des parents | `CONTRAT_COUVERTURES_DATEES.md` ; `CERTIFICAT_FULL_CPP_COURANT.md:3` |
| Digest sémantique FULL | **testé** (porte Boost : 160 combinaisons de bornes, 512 vecteurs) | `CONTRAT_DIGEST_FULL.md:64` |
| Normalisation des successeurs v2 (3d−1 au lieu de 3d+1) | **prouvé** + **testé** | `CONTRAT_NORMALISATION_FULL.md:46` |

Les deux fixtures de réfutation des adjacences induites :

- **Constructeur.** A=(1,1,7), B=(5,2,1), C=(7,2,2), D=(5,2,8). La vraie fusion a lieu à 477/34. Le graphe hérité laisse deux composantes, et la variante généreuse retarde la fusion à 31/2. Ces valeurs sont gravées dans la porte `bee615b5…`.
- **Auditeur.** Une autre fixture retarde la fusion de 169/9 à 41/2.

**Précision sur le fold v4.** Le fold v4 porte sur le K-graphe de Gabriel de la Déf. 29 : chaque boule peu profonde est un K-simplexe de Gabriel qui unit ses K+1 facettes (`morsehgp3D_v4/docs/MATHEMATIQUES.md:130-158`, `:496-533`). Il publie aussi des naissances et des croissances (`ComponentDelta`, `:534-578`) : ce n'est donc pas un simple flot de fusions. Il reste pourtant le graphe élagué du manuscrit, que le registre déclare `false_in_general` (`STATUT:40`, `:129`) :

- E5 y produit une fausse naissance et une fausse fusion ;
- les minima Gabriel isolés sans coface Gabriel de cardinal K+1 y sont absents, ainsi que la feuille K=n.

`CLAUDE.md` indique que la v5 calcule « le même objet » que la v4. La divergence d'objet entre la lignée v4/v5 et la v7 est donc antérieure à la v8.

### 3.2 Constructeurs, résolveur et parallélisation

| Composant | Statut | Preuve |
| --- | --- | --- |
| Producteur horizontal `full_gabriel.hpp` (catalogues Gabriel fournis) | **testé** relatif : 7/7 CTests Release et SAN, 100 ordres, 16 506 coupes ; autorité `kCompleteRelative` | `CONTRAT_PRODUCTEUR_FULL_GABRIEL.md:51`, `:188-200` |
| Tour par boules `full_ball_tower.hpp`, en-tête final `83f1c78e…` | **testé** borné : couture 34 nuages, 150 ordres, 87 230 verticales par bras ; callback 298 742 contrôles Gram/Gamma, 3 916 coupes (n ≤ 8) ; T2 K10 par overlay ; 40 CTests ciblés. Les 28 nuages et 170 320 contrôles appartiennent à `910f45ba…` | `PARALLELISATION_PAR_LOTS_20260911.md:39-47` ; `receipts/full_ball_batch_permanent_20260911/README.md` ; `TOUR_FULL_PAR_BOULES.md:136-142` |
| MEB à coquille libre `anchor_meb.hpp` (`386072c8…`) | **testé** : 605 cas dont 197 extra-shells, 300 permutations, trois mutants | `TOUR_FULL_PAR_BOULES.md:132-133` |
| Raccord réel génération→census→FULL jusqu'à K10 (T2) | **testé** borné : 54 tours par build, trois géométries (ligne12, coquille12+centre+extérieur = 14, spatial12), s8/10/12, cache/statique1/4, 13 000 coupes, 8 103 948 vérifications verticales, 12 315 725 contrôles, neuf refus, quatre mutants | `QUALIFICATION_TOUR_CENSUS_K10_20260911.md:40-93` ; `receipts/census_tower_permanent_20260911/README.md` |
| Tours 8k/16k/32k (mono, plusieurs en-têtes) et 50k K1..10/K1..5 (G4, `910f45ba…`) | **mesuré** (§ 4) ; famille uniforme u16 seulement | `receipts/full_ball_scale_gpu_20260910/`, `post_exchange_scale_20260911/`, `static_resolution_scale_20260911/` |
| Cache exact de résolutions | **testé** + **mesuré** : 1 174 515 → 583 337 MEB à n1000 appariés | `morsehgp3D_v7/README.md:88-89` ; `docs/OPTIMISATIONS_CACHE_ET_GPU_20260910.md:50` |
| Résolution statique dédoublonnée (`--static-threads`) | **testé** + **mesuré** : MEB −32,8/33,8/34,2 % à 8k/16k/32k, sans gain de temps revendiqué | `morsehgp3D_v7/PASSATION.md:77-97` |
| Semis après échange | **testé** + **mesuré** : −5,68/5,71/5,73 % de MEB | `morsehgp3D_v7/PASSATION.md:99-117` |
| Couture par lots et callback transactionnel | **testé** : 40 CTests ciblés ; GPU en émulation hôte uniquement | `PARALLELISATION_PAR_LOTS_20260911.md:5`, `:39-47` |
| Graphes datés sur naissances, atlas, contraction des pivots, workers persistants, gardes par rangs | **testé** O2/SAN (114 census, 912 essais, 506 448 terminales, 48 775 524 contrôles par build) + **mesuré** CPU4 | `GARDES_RANGS_CERTIFIES_20260911.md:59-75` ; `CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md:45-60` |
| Mode de comparaison du graphe daté | égalité modulo une **bijection explicite des identités natives**, pas égalité brute des numéros | `receipts/atlas_graph_full_20260911/README.md` (« Ce n'est pas une égalité brute des numéros internes ») |
| Graphe daté à 32k | **aucune comparaison physique ni oracle** contre le Builder : `physical_comparison_executed: false`, `geometric_oracle_executed: false` | objet `8cc9dadd…` |
| Sources privées du graphe daté | scellées dans cinq paquets de reçus, hors `src/` | `rank_atlas_20260911`, `atlas_graph_full_20260911`, `birth_streaming_20260911`, `ordered_streaming_20260911`, `rank_guard_streaming_20260911` |
| Marques résolues au premier parcours des fusions | prototype d'audit **testé** (30 essais, 6 828 comparaisons BFS, deux mutants) ; raccord constructeur **non commis** | `morsehgp3D_v7/audits/ETAT_COURANT.md:5-12` ; § 5 |
| Noyau MEB accéléré (paire diamétrale, extrêmes d'abord, Welzl réparé, canonisation sur coquille) | prototype d'audit **testé** : 198 000 cas, zéro divergence, zéro repli, ce qui est une évidence empirique et non une preuve. **Mesuré** sur flux réel 8k/16k, sur l'en-tête `6763a877…` avec `--static-threads=1`, hors moteur | `NOTE_CLAUDE_COEUR_MEB_20260911.md:103-157`, `:333-347` |
| Découpe de `tower_s` (géométrie / calendrier / prologue / épilogue) | **mesuré** par instrumentation isolée sur `6763a877…`, reproduit indépendamment (16,0/62,7/16,2/4,4 %) | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:14-31`, `:179-181` |
| Résidence massive, checkpoint, archive FULL, reprise | **proposé** ; **manquant** | `morsehgp3D_v7/docs/RESIDENCE_MASSIVE.md:334-428` |

Le graphe daté donne les mêmes MEB et le même nombre de nœuds que le Builder aux trois tailles, mais sous sa propre convention de digest `mhgp7-graph-FULL-dense-raw-level-bank-v1`. Son égalité avec le Builder n'est établie que sur les 114 census bornés, dont six uniformes n32.

### 3.3 GPU

| Composant | Statut | Preuve |
| --- | --- | --- |
| Route CUDA préfiltre/census dans une tour 50k | **mesuré** sur G4 SM 12.0 : gate device de 16 627 contrôles sur 4 116 boules (q2 846, q3 2 085, q4 1 185), trois extra-shells, 17 rejets | `device_gate.summary.json` ; `RESULTATS_TOUR_CACHE_G4_20260910.md:32-40` |
| Effet de la route CUDA sur la tour | Préfiltre plus census passent de 5,397 s (CPU48) à 4,540 s à K10, et de 0,849 à 0,846 s à K5. La tour reste inchangée : 418,873 s contre 418,921 s | `cpu_n50000_k10_s8.summary.json` et `gpu_…` |
| Primitive de sélection MEB par lots | **testé sur carte** : 605 cas, 21 432 contrôles, 44 rejets, 197 extra-shells ; aucun débit mesuré | objet `2436eaa8…` |
| Primitive clé/PGCD/division 128 bits | **testé sur carte** : 13 573 cas, 325 752 mots, 22 refus | objet `041bc9ce…` |
| Terminal géométrique composé K1..10, lots résidents semés | **testé** hôte + compilation NVCC SM120 stricte (238 registres, 1 392 octets de pile) ; **jamais exécuté sur carte** ; trois générations G4 closes sans calcul | `PARALLELISATION_PAR_LOTS_20260911.md:5-7`, `:53-57` |
| Forêts, contributions, verticales sur GPU | **manquant** | `morsehgp3D_v8/audits/IMPLEMENTATION_PARALLELISATION.md:259-270` |

### 3.4 Croisement avec les décisions v8

La v8 a fixé le 13 septembre (`2b658cbe`) ce qu'elle garderait de la v7 (`morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md:116-127`). Constat à HEAD 12294241 :

| Décision v8 | Réalisé en v8 ? | Preuve |
| --- | --- | --- |
| Objet FULL avec vrais parents : conserver contrat et contre-preuves | Contrat conservé **en texte** ; **aucun code** | `morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md:31-57` ; `morsehgp3D_v8/src/forest/` ne contient que `.gitkeep` |
| Supports q ≤ 4, clés exactes, census partagé : reprendre et requalifier | Réécrits de zéro sans code v7 : census q2 complet, census q3 d'une arête, flux q3/q4 de candidats | `morsehgp3D_v8/src/lanes/exact_ball.hpp:19` ; `q4_family.hpp:19` ; `src/pipeline/wspd_q34.hpp:100-101` |
| Petits oracles et mutants comme juges | Oracles v8 propres (`oracle/p0_oracle.hpp`, `oracle/q2_census_oracle.hpp`) ; juge census→FULL v7 **non porté** | inventaire `morsehgp3D_v8/oracle/` |
| Résolveur statique et facettes uniques comme base | **Non réalisé** | `src/forest/` vide |
| Catalogue, rangs et masques sous un propriétaire immuable | **Non réalisé** pour les boules | `src/pipeline/prepared_cloud.hpp`, `src/spatial/float32_index.hpp` |
| Remplacer le calendrier mutable ; forêts datées et contraction | **Non réalisé** | idem |
| GPU autour d'objets résidents | **Non réalisé** (`src/gpu/` vide) | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:70` |
| Ne pas reprendre CLI/archive F comme FULL | Respecté (`cli/` vide) | inventaire |
| Une seule chaîne principale | Deux moteurs coexistent (entier u16, élargi en u18 par `a74e90f2`, et float32) | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:89-93` |
| Héritage non moteur effectivement repris | Contrôleurs G4 v8 fondés sur les helpers du contrôleur v7 ; en-têtes Boost extraits sous `build/v7_boost_gate` | `gcp-migration/cpu_probe_session_v8.py:1-35`, `q34_spatial_session_v8.py:7-35` ; `morsehgp3D_v8/README.md:834-837` |

Les contrôleurs `cpu_probe_session_v8.py` et `q34_spatial_session_v8.py` vérifient le sha de `full_probe_session_v7.py` (`177b25a0…`), puis l'exécutent pour reprendre ses helpers de cycle de vie.

L'audit v8 des facettes silencieuses a fixé le portage : « Reprendre la sémantique de `full_ball_tower.hpp` », avec la régression ABCDE obligatoire. Il a été introduit par `ae98dbfa` (19/09), puis condensé par `c92aad13` (20/09) ; son texte est daté du 20 septembre (`morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:3`, `:10-14`, `:34-37`).

Le portage de la tranche FULL minimale n'apparaît dans la passation v8 que dans la section historique « Passation précédente du 17 septembre » (`morsehgp3D_v8/PASSATION.md:663`, `:692-694`). Le plan courant qui fait foi est celui de l'audit de reprise. Sa phase 3 propose le fold v4 (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:196-197`), et son tableau d'état déclare l'aval « inexistant » (`:69`).

**Conclusion : la v8 a laissé de côté tout l'aval de la tour**, c'est-à-dire :

- le catalogue canonique et la résolution des parents ;
- les histoires datées, les contributions et les verticales ;
- la sortie FULL.

## 4. Chiffres clés

Toutes les tours ci-dessous portent sur la famille uniforme u16, graine 3, s8, sauf mention contraire. Chacune est une observation unique.

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| Tour 50k K1..10 CPU : total sonde / processus / constructeur FULL | 418,872898 / 419,809931 / 389,667561 s (FULL = 93 %) | `…/gcp/optimized/output/cpu_n50000_k10_s8.summary.json` (`total_s`, `process_wall_seconds`, `tower_s`) | en-tête `910f45ba…` ; 48 fils amont, FULL mono ; `contract_qualified=false` |
| Tour 50k K1..10 hybride (census CUDA + FULL CPU) | 418,920854 s ; FULL 390,480778 s | `gpu_n50000_k10_s8.summary.json` | `pair_n50000_k10_s8.json` : `matched` |
| Tour 50k K1..5 CPU / hybride | 33,852713 / 33,569245 s (processus 34,006 / 33,790 s) ; FULL 27,228 / 26,983 s | `cpu_n50000_k5_s8.summary.json`, `gpu_…` | idem |
| Amont 50k CPU48 K10 (génération / tri / préfiltre / census / digest) | 12,231 / 3,126 / 2,641 / 2,756 / 8,371 s | `cpu_n50000_k10_s8.summary.json` | le digest de 8,4 s est inclus dans `total_s` |
| Amont 50k CPU48 K5 (génération / tri / préfiltre / census) | 4,464 / 0,323 / 0,410 / 0,439 s | `cpu_n50000_k5_s8.summary.json` | — |
| Nœuds FULL retenus K10 / K5 | 27 273 218 / 4 209 792 | `signature.nodes` | — |
| Boules census K10 / K5 | 21 468 368 / 4 010 348 | `row.balls` | quatre / trois extra-shells |
| MEB du résolveur K10 / K5 | 41 986 201 / 4 383 525 | `resolver_meb_calls` | — |
| Supports testés K10 / K5 | 3 898 856 828 / 44 413 779 | `resolver_supports_tested` | de K5 à K10 : nœuds ×6,48, supports ×87,8 |
| Contributions / références verticales K10 | 16 495 216 / 27 173 227 | `row.contributions`, `row.vertical_refs` | — |
| RSS externe K10 CPU / hybride | 16 206 376 / 16 318 064 KiB | `cpu_n50000_k10_s8.stderr`, `gpu_…stderr` | 15,46 Gio |
| RSS externe K5 CPU / hybride | 3 785 592 / 3 949 772 KiB | stderr correspondants | — |
| Kernels census K10 / phase / reconstruction hôte / empaquetage | 189,346 ms / 4,540 s / 2 931,852 ms / 904,948 ms | `gpu_n50000_k10_s8.summary.json` | kernels ≈ 4,2 % de leur phase ; route CPU équivalente 5,397 s |
| Digests de payload K10 / K5 ; entrée | `1244fee7…` / `59070288…` ; `3f7c6dd4…` | summaries | — |
| Gate device SM 12.0 | 16 627 contrôles, 4 116 boules, 17 rejets, trois extra-shells | `device_gate.summary.json` | gate, pas benchmark |
| Arrêt G4 ciblé | `targeted_shutdown_certified: true` ; « état GCE TERMINATED » | `…/gcp/optimized/host/receipt.json`, `guarded_stop.stdout` | — |
| Tour mono locale « cache » 8k/16k/32k | 235,724 / 354,144 / 736,819 s ; FULL 89,571 / 155,883 / 352,009 s | `RESULTATS_TOUR_CACHE_G4_20260910.md:52-56` | `910f45ba…` ; hôte partagé |
| Nœuds 8k/16k/32k | 3 976 472 / 8 310 399 / 17 166 975 | idem | ×2,090 puis ×2,066 ; mêmes nœuds sur tous les en-têtes et routes |
| RSS 8k/16k/32k (cache) | 2 136 656 / 4 427 688 / 9 108 756 KiB | `…/local/n{8000,16000,32000}_s8.stderr` | — |
| s8/s10/s12 à 8k | même digest `cdd77e30…` ; 235,724 / 149,179 / 157,080 s | `RESULTATS_TOUR_CACHE_G4_20260910.md:63-71` | ne désigne pas d'optimum |
| Première tour par boules mono 8k/16k/32k | 215,169 / 417,627 / 965,053 s ; FULL 119,636 / 242,284 / 565,502 s | `RESULTATS_TOUR_BOULES_20260910.md:21-25` | JSON non relu |
| **Meilleur triplet mono (semis après échange)** | total 141,366 / 318,968 / 694,459 s ; FULL 58,977 / 134,329 / 292,341 s ; pic RSS 8,687 Gio à 32k | `receipts/post_exchange_scale_20260911/` (objets relus pour 8k et 32k) ; `PASSATION.md:110-117` | `6763a877…` ; un processus par taille |
| Triplet statique (1 amont + 4 statiques) | totaux 466,761 / 686,049 / 802,278 s | `PASSATION.md:92-93` | hôte partagé ; aucun gain chronométrique revendiqué |
| Triplet CPU4 naissances denses + workers | 92,963 / 215,381 / 489,601 s ; RSS 2 874 428 / 5 803 964 / 11 608 968 KiB | objets de `parallel_birth_streaming_20260911` (relus) | graphe daté privé |
| Triplet CPU4 gardes par rangs | 96,401 / 223,447 / 478,616 s ; RSS 32k 11 607 552 KiB | `GARDES_RANGS_CERTIFIES_20260911.md:68-72` ; objet `8cc9dadd…` (32k relu) | pas de gain reproductible contre le parent |
| Décomposition 32k CPU4 (rangs) | génération 92,196 ; tri 4,370 ; préfiltre 21,523 ; census 16,176 ; validation 42,562 ; atlas+géométrie+réduction 158,710 ; histoires 50,467 ; export 92,044 s | objet `8cc9dadd…` | aval hors MEB ≈ 185 s ; sans comparaison physique |
| Volumes 32k (rangs) | 13 502 432 boules ; 45 453 599 occurrences ; 10 380 964 naissances ; 23 851 396 marques ; 19 784 213 MEB ; 2 133 246 557 tests de puissance | idem | — |
| Paire 1/1/1 → 1/4/1 threads à 8k | 187,214 → 164,703 s ; phase 58,649 → 34,976 s | `morsehgp3D_v8/audits/CONTRATS_ET_MESURES.md:172-185` ; objets relus | une paire, même travail |
| Résolution statique, MEB | 4 185 184 / 8 779 465 / 18 244 853 (−32,8/33,8/34,2 %) | `morsehgp3D_v7/PASSATION.md:90-93` | capacités temporaires 1 235 849 528 octets à 32k |
| Semis après échange : MEB payées / évitées | 3 947 627 / 8 278 207 / 17 199 233 ; 237 557 / 501 258 / 1 045 620 (−5,68/5,71/5,73 %) | `PASSATION.md:112-113` | — |
| Noyau MEB auditeur sur flux réel 8k | supports facturés 340 615 272 → 104 791 833 (×3,25) ; `tower_s` 61,23 → 45,13 s (×1,36 ; trois exécutions ×1,357 à ×1,365) ; digest identique | `NOTE_CLAUDE_COEUR_MEB_20260911.md:121-157` | `6763a877…`, `--static-threads=1` ; hors moteur ; hôte partagé |
| Même noyau à 16k | 139,15 → 107,03 s (×1,300), digest identique | idem `:143-148` | — |
| Coquille sélectionnée = support, flux réel 8k | 100 % de 3 947 627 résolutions (q2 13,4 %, q3 54,4 %, q4 32,2 %) | idem `:202-248` | coquille locale de la facette (≤ 10 sites), pas la cosphéricité globale ; une famille, un nuage |
| Découpe `tower_s` 8k | géométrie 62,8 %, calendrier 16,8 %, prologue 15,4 %, épilogue 5,0 % | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:23-26` | reproduite indépendamment à 16,0/62,7/16,2/4,4 % |
| Exposants locaux 8k→16k | géométrie 1,094 ; calendrier 1,243 | idem `:71-75` | une seule paire de tailles |
| Plafond de parallélisme sous découplage des ordres | 11,4× (8k) / 11,2× (16k), et non 20× | idem `:146-172` | plafond, pas un gain |
| Allocation indicative 50k après noyau MEB et parallélisme | ≈ 102 s K1..10 ; ≈ 7 s K1..5 | idem `:77-104`, `:269-273` | **proposé** : proportions transportées, pas une mesure |
| Part des blocs d'ancrage en lots groupés | 0,0086 % (8k) → 0,0264 % (16k) → 0,0843 % (32k) | idem `:246-258` | ×3 par doublement ; sans plancher ni mutant |
| Qualification T2 K10 | 54 tours/build ; 13 000 coupes ; 8 103 948 verticales ; 12 315 725 contrôles (12 315 605 + 120 de métadonnées) | `QUALIFICATION_TOUR_CENSUS_K10_20260911.md:44-90` | n ≤ 14 ; sources `c03f6be8`, puis overlay `83f1c78e…` |
| Qualification de `910f45ba…` | 28 nuages, 112 ordres, 2 508 coupes, 45 948 verticales, 170 320 contrôles O2/SAN, six mutants | `TOUR_FULL_PAR_BOULES.md:136-141` ; `receipts/ball_resolver_residence_20260910/summary.json` | pas l'en-tête final |
| Refus G4 FULL du 6/09 | K10 21,372 s ; K5 5,646 s ; zéro ordre construit | `morsehgp3D_v8/audits/CONTRATS_ET_MESURES.md:128` | refus, pas un temps de tour |
| Plancher d'écriture de la sortie 50k K10 au format v7 | 1,75 Go à 64 o/nœud (arithmétique vérifiée) ; 62,7 ms mono, 61,6 ms à huit fils | `morsehgp3D_v8/audits/BUDGET_CONTRAT_50K_20260914.md:42-51` | **non vérifiable** : aucun reçu épinglé pour 62,7 ms |
| MEB pivot4 canonique (audit v8) | supports 573 295 → 79 173 ; rapport des médianes 0,262–0,271 (K10 : 0,174–0,184) | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:64-76` | microbenchmark n = 24/48, hors Builder ; aucun gain à K2/K3 |
| Témoins de borne de sortie | 9/25/81/289 feuilles K2 aux tailles 6/10/18/34 | `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:39` | ni acceptation par le moteur, ni performance |
| Taille des cibles v9 (LiDAR) | Trame brute : 123 389 / 124 479 / 125 526 retours (≈ 119 142 / 119 942 / 120 725 sites à 2 cm, historique). Sans sol : 39 885 / 35 551 / 45 845 sites float32, 39 815 / 35 491 / 45 114 sites u16 à 2 cm | `CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:46-47` ; `AGENTS.md:52` ; `morsehgp3D_v8/PASSATION.md:25` | aucune tour FULL mesurée sur ces trames ; comptes u18 à 1 mm non lus |

## 5. Défauts, risques et dettes

| Gravité | Constat | Preuve |
| --- | --- | --- |
| Haute | **Risque de changer d'objet en v9.** L'audit de reprise v8 recommande de porter « le fold en forêts K = 1..10 … de la v4 (§ 9.1) … digests au format v4 ». Le fold v4 opère sur le K-graphe de Gabriel (Déf. 29). Le registre classe « le flot des seules cofaces Gabriel de cardinal K+1 reconstruit FULL » comme `false_in_general` : E5, minima isolés, K=n. Porter ce fold seul produirait un objet réfuté, contraire au contrat « tour HGP FULL ». Non vérifié par exécution ici. | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:196-197` ; `STATUT_PREUVES_ET_HEURISTIQUES.md:40`, `:129` ; `morsehgp3D_v4/docs/MATHEMATIQUES.md:496-533` ; `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:10-14` |
| Haute | **Aucune tour FULL en v8.** Les seules tours complètes du dépôt récent sont celles de la v7, toutes uniformes u16. Le contrat v9 (LiDAR sans sol et trame brute) n'est pas mesurable tant que l'aval manque. | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56-57`, `:69` |
| Haute | **Aucune tour FULL sur une famille non uniforme.** Aucun reçu v7 de tour ne porte sur terrain, amas ou LiDAR ; les seules familles non uniformes de la v7 sont des paires `verified_events_only` du 4/09. Le régime LiDAR (lignes de balayage, sol retiré, grandes boules vides) peut changer la part MEB/calendrier et la fréquence des extra-shells. | `receipts/local_paired_20260904/runs.json`, `gcp_requalified_20260904/` ; `CONTRATS_ET_MESURES.md:127` |
| Haute | **Aucune mesure d'échelle sur l'en-tête final.** Les 419 s / 34 s de 50k datent de `910f45ba…`, avant la résolution statique, le semis après échange et la couture. `83f1c78e…` n'a qu'une sonde n200 et des gates ; le graphe daté n'a jamais tourné à 50k. | `RESULTATS_TOUR_CACHE_G4_20260910.md:76-77` ; `PARALLELISATION_PAR_LOTS_20260911.md:47` |
| Haute | **Domaine arithmétique u16 du Builder v7.** Il exige `full_ball_u16_domain` et des clés A < 2^68, B < 2^87, C < 2^105. Depuis le 22/09, le contrat temps de la v8 passe par le moteur entier u18 (`a74e90f2`, `quantized_u18_input_only`) ; le float32 reste qualifié hors contrat temps. Le Builder ne peut pas consommer ces catalogues sans nouvelles bornes et requalification. | `full_ball_tower.hpp:451`, `:478-480` ; `ELARGISSEMENT_18_BITS_20260922.md:3-9` |
| Haute | **Limites de représentation v7.** Le format admet au plus neuf sites intérieurs et douze sites de coquille, avec des masques de contribution u16 et des `BallId` u32. Il refuse explicitement au-delà, sans jamais tronquer. La fréquence des grandes coquilles sur la grille LiDAR 1 mm n'est pas mesurée. | `TOUR_FULL_PAR_BOULES.md:37-43` ; `full_ball_tower.hpp:109`, `:215`, `:756`, `:921-925` ; `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:114-115` |
| Haute | **Volume de sortie.** On compte 27,3 M nœuds et 15,5 Gio de RSS à 50k uniforme K10. Au format v7, l'écriture seule prendrait 62,7 ms selon un réfutateur, chiffre non vérifiable faute de reçu. La sortie FULL d'une trame n'a jamais été mesurée. | § 4 ; `BUDGET_CONTRAT_50K_20260914.md:42-51` |
| Moyenne | **Défaut latent du chemin des lots groupés.** Sa part triple à chaque doublement (0,0843 % à 32k) et l'arité dépasse deux à 16k/32k (excès 6 et 186). Ce chemin n'a ni plancher de couverture ni mutant causal. | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:242-265` |
| Moyenne | **`payload_digest` aveugle à la renumérotation des populations.** Selon la lecture du code, un calendrier à ordres découplés passerait le digest et échouerait `paired.bank_rows`, `paired.exact_contribution` et `bank.shared_across_orders`. Ce n'est pas une exécution. Un mutant voisin, la banque triée par `BallId`, a bien été exécuté et produit dix rejets `physical.bank_rows`. | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:211-240` ; `audits/receipts_historical_export_20260911/README.md:17`, `:79` |
| Moyenne | **Calendrier séquentiel tel qu'écrit.** Son exposant local vaut 1,243, contre 1,094 pour la géométrie. Optimiser la MEB fait tomber le plafond de parallélisme de 2,69× à 1,98×. Le « séquentiel par construction » est retiré : c'est une propriété du moteur, pas de l'objet. | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:33-75`, `:130-145` ; `FAUSSES_PISTES.md:112-114` |
| Moyenne | **Prototypes privés hors `src/`.** Atlas, graphes datés, contraction, workers et gardes par rangs n'existent que comme sources scellées dans cinq paquets de reçus. Ils sont comparés au Builder par bijection d'identités, et sans comparaison physique à 32k. | `IMPLEMENTATION_PARALLELISATION.md:41-73` ; `atlas_graph_full_20260911/README.md` ; objet `8cc9dadd…` |
| Moyenne | **ThreadSanitizer jamais exécuté** sur les workers v7 : refus d'environnement, code 66. | `CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md:108-110` ; `GARDES_RANGS_CERTIFIES_20260911.md:66` |
| Moyenne | **GPU aval jamais exécuté sur carte.** Le terminal composé et les lots résidents sont seulement compilés ; les trois générations G4 se sont closes sans calcul. | `PARALLELISATION_PAR_LOTS_20260911.md:5-7`, `:55-57` |
| Moyenne | **Suite complète non attestée sur l'en-tête final.** Le résultat 449/449 porte sur `6763a877…` ; `83f1c78e…` n'a que 40 CTests ciblés. | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:293-295` ; `IMPLEMENTATION_PARALLELISATION.md:63-65` |
| Moyenne | **Profil pondéré absent.** Les minima FULL ne sont pas les feuilles pondérées de l'Algorithme 1. Les scores ne demandent pas toutes les cofaces Gamma, mais un supplément explicite. | `STATUT_PREUVES_ET_HEURISTIQUES.md:135-136` ; `CONTRAT_MASSES_VOTE_COURANT.md:3-27` |
| Moyenne | **Delta v7 non commis** dans le worktree partagé : `morsehgp3D_v7/docs/MARQUES_PREMIER_PARCOURS_20260911.md` (non suivi, 6 153 octets, écrit le 11/09 à 20:46 UTC), `receipts/fused_history_streaming_20260911/` (non suivi, 44 Ko), et des modifications de `PASSATION.md` (écrit le 15/09 à 12:15 UTC), `README.md`, `FAUSSES_PISTES.md` et `OBJETS_PARALLELES_TOUR_20260911.md`. Il annonce 49 519 620 contrôles et « Mesures grandes encore en cours ». Il était déjà présent à l'ouverture v8. **Non commis, non vérifiable.** | `git status -- morsehgp3D_v7` (worktree partagé) ; `AUDIT_V7_SYNTHESE.md:74-77` ; `audits/COORDINATION_MORSEHGP3D_V8.md:7-8` |
| Basse | Mesures v7 à une seule observation sur hôte partagé. s10/s12 n'ont jamais été exécutés à 50k ; le protocole contractuel (échauffements, dix nuages frais, p95) non plus. | `RESULTATS_TOUR_CACHE_G4_20260910.md:41-43` ; `CONTRAT_PERFORMANCE.md:81` ; `CONTRATS_ET_MESURES.md:20-27`, `:328` |
| Basse | Les gates v8 dépendent d'en-têtes Boost extraits dans `build/v7_boost_gate/extracted/usr`, non versionnés. | `morsehgp3D_v8/README.md:834-837` |
| Basse | Les contrôleurs G4 v8 exécutent le code de `full_probe_session_v7.py` après vérification de son sha : ce fichier v7 ne peut plus évoluer sans casser la v8. | `gcp-migration/cpu_probe_session_v8.py:26-32` |

## 6. Questions ouvertes

1. **Quel objet la v9 calcule-t-elle ?** Deux candidats s'opposent : FULL v7 (feuilles minima, multifusions, parents pré-lot, contributions datées, ancres et verticales, K=n) ou fold v4 sur le K-graphe de Gabriel. Les deux recommandations v8 se contredisent (§ 5, première ligne), et la lignée v4/v5 porte déjà l'autre objet. Le contrat utilisateur dit « tour HGP FULL ».
2. **Quel profil arithmétique pour la tour ?** Le contrat temps retient u18 à 1 mm depuis le 22/09, la trame brute par défaut est float32. Quelles bornes de clés remplacent 2^68/2^87/2^105 ?
3. **Quel format de nœud ?** Le budget de 100 ms exige un nœud plus compact que 64 octets, ou une sortie implicite déclarée comme contrat distinct (`BUDGET_CONTRAT_50K_20260914.md:48-51`). Le plancher d'écriture reste à mesurer avec reçu.
4. **Grandes coquilles sur LiDAR.** Quelle est la fréquence des coquilles de plus de 12 sites, et des intérieurs de plus de 9, sur les trames à 1 mm ? Comment les traiter sans troncature ?
5. **Profil pondéré.** La v9 doit-elle livrer les incidences contributives et leurs dates d'affectation pour la piste SemanticKITTI ?
6. **Hybride J=1 / descente à cardinal K.** Il n'a jamais été mesuré sur flux réel (`SQUELETTE_MINIMA_GABRIEL.md:208-227`).
7. **Contraction parallèle des histoires (RCTT, PANDORA).** L'adaptation aux dates égales et aux multifusions n'est pas prouvée (`OBJETS_PARALLELES_TOUR_20260911.md:112-125`).
8. **Numérotation de référence.** La v9 adopte-t-elle l'égalité physique au format du Builder, ou une canonisation déclarée, comparée par bijection comme le graphe daté ?
9. **Sort du delta v7 non commis** : commit par son auteur, archivage ou abandon documenté ?

## 7. À porter en v9, et à ne pas reprendre

### 7.1 À porter (quoi, où, pin, pourquoi)

Pin commun : commit `dc57ffd5`, identique à HEAD 12294241 pour `morsehgp3D_v7/`. Les empreintes sha256 tronquées ont été recalculées lors de la contre-vérification.

| Quoi | Où | Pin | Pourquoi |
| --- | --- | --- | --- |
| Définition de l'objet FULL et ses théorèmes conditionnels | `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md` ; `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md` ; `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:129-136` | `dc57ffd5` | seule définition prouvée et contrelue de la tour à livrer |
| Extension non régulière (contributions datées, ancres inertes, fenêtre p+q_min ≤ Kmax+1) | `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md`, `CONTRAT_COUVERTURES_DATEES.md` | `dc57ffd5` | les nuages u16/u18 ne sont pas réguliers : quatre extra-shells à 50k |
| Parents globaux des coquilles réelles 50k | `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/GLOBAL_PARENTS.md` | `dc57ffd5` | trois fixtures réelles à parents certifiés (174406, 254569, 996863) |
| Borne de sortie et protocole de croissance par unité de sortie | `morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md` ; `morsehgp3D_v7/tests/full_output_lower_bound_gate.py` | `01fd4010…` | interdit la promesse sous-quadratique universelle ; impose de mesurer sortie et surcoût séparément |
| Builder de référence différentiel | `morsehgp3D_v7/src/forest/full_ball_tower.hpp` | `83f1c78e…` (1 113 l.) ; mesures d'échelle sur `910f45ba…` et `6763a877…` | représentants résolus pré-lot, regroupement par racines, fermeture atomique, ancres inertes, verticales à la coupe fermée |
| MEB à coquille libre (référence lexicographique) | `morsehgp3D_v7/src/forest/anchor_meb.hpp` | `386072c8…` | contrat « premier support admissible » à préserver par tout proposeur |
| Journal daté v2 | `morsehgp3D_v7/src/forest/full_coverage_certificate.hpp` | `7608e70e…` | format de sortie avec contributions et successeurs historiques |
| Quotient local de coquille | `morsehgp3D_v7/src/forest/local_plateau.hpp` | `df56fbf3…` | plateaux sans énumérer les intérieurs |
| Certificat v1 et producteur horizontal (différentiels réguliers) | `morsehgp3D_v7/src/forest/full_certificate.hpp`, `full_gabriel.hpp` | `463724b7…`, `a946e31d…` | juges structurels et de parents relatifs |
| Oracle Gamma borné et arithmétique indépendante | `morsehgp3D_v7/oracle/full_gamma.hpp`, `obig.hpp` | `a17732d2…`, `0e3e9241…` | juge indépendant des coupes ouvertes et fermées |
| Juge T2 census→FULL (Gram/Gamma, n ≤ 14) | `morsehgp3D_v7/tests/census_tower_oracle.hpp`, `census_tower_gate.cpp` | `b992e76c…`, `6b749074…` | première porte v9 ; ne jamais alimenter le constructeur par le catalogue du juge ; gardes `order()==K` et taille de `lower_nodes` incluses |
| Portes FULL et leurs mutants | `morsehgp3D_v7/tests/full_ball_tower_gate.cpp`, `full_certificate_gate.cpp`, `full_gabriel_gate.cpp`, `full_coverage_certificate_gate.cpp`, `anchor_meb_gate.cpp`, `local_plateau_gate.cpp` | `c4f39462…`, `17f5e2ba…`, `577a689b…`, `2ebd45d8…`, `5e3ee812…`, `22bb006e…` | mutants déjà écrits : croissance omise, ancre inerte omise, image future, descente strictement en rayon, parent→0 |
| Portes rationnelles de quotient et de descente | `morsehgp3D_v7/tests/full_gabriel_minima_quotient_gate.py`, `full_gabriel_descent_comparison_gate.py` | `bee615b5…`, `5e357ead…` | réfutent le graphe induit (477/34 → 31/2) et le remplacement systématique de J=1 |
| Fixtures de contradiction | voir la liste sous ce tableau | `dc57ffd5` | toute régression v9 doit mourir sur l'une d'elles |
| Architecture des objets parallèles | `morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md:11-145` | `dc57ffd5` | atlas clairsemé `cell(B,K)`, rang entier commun, graphes datés sur naissances, forêt minimale, verticales hors ligne sans MEB |
| Sources privées des graphes datés (scellées) | voir la liste sous ce tableau | sha des manifestes | seule implémentation testée de la contraction des pivots (φ = identité de naissance, pas racine DSU) et des workers géométriques |
| Accélérations MEB exactes | `morsehgp3D_v7/audits/receipts_coeur_meb_20260911/` (`welzl2.cpp`, `realflow_patch.py`) ; `morsehgp3D_v7/receipts/meb_diameter_20260911/` ; `morsehgp3D_v7/audits/receipts_certified_support_20260911/` ; variante pivot4 v8 | `dc57ffd5` ; `c92aad13` | ×3,25 supports et ×1,36 tour mesurés sans changer le digest ; raccourci coquille = support sur 100 % du flux 8k, avec repli obligatoire |
| Outillage G4 v7 | `gcp-migration/full_ball_worker_v7.py`, `gcp-migration/full_probe_session_v7.py` | `80b94a5a…`, `177b25a0…` (identiques aux pins du reçu 50k) | protocole de paire CPU/hybride déjà exécuté et clos ; helpers de cycle de vie déjà réemployés par la v8 |
| Discipline de mesure | schéma `mhgp7-full-ball-tower-probe-v1` (champs `signature` séparés des temps) ; lecteurs `verify.py` des paquets | reçu `full_ball_scale_gpu_20260910` (`manifest.json` `c9913094…`) | compteurs de travail appariés, succès relatif explicite, `contract_qualified=false` |

Fixtures de contradiction à porter :

- E5 : `tests/fixtures/regressions/gabriel_point_set_counterexample.json` (`0c74c429…`) ;
- les deux « quatre points » : `SQUELETTE_MINIMA_GABRIEL.md:44` et `audits/receipts_gabriel_vertices_20260906/` ;
- ABCDE : `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:34-37` ;
- MEB K7 : `NOTE_CLAUDE_COEUR_MEB_20260911.md:305-313` ;
- descente forcée K4 : `FACETTES_SILENCIEUSES:86-90` ;
- coquille à sept points : `FAUSSES_PISTES.md:68` ;
- triangle rectangle : `AUDIT_NIVEAUX_GABRIEL_20260905.md:108-114` ;
- carré K2 à quatre parents ;
- peigne Morton de 49 références : `FAUSSES_PISTES.md:21`.

Sources privées des graphes datés, scellées dans les reçus :

| Fichier | Empreinte | Paquet de reçus |
| --- | --- | --- |
| `rank_atlas.hpp` | `9fc118a2…` | `rank_atlas_20260911` |
| `atlas_graph.hpp` | `bb2b84d9…` | `atlas_graph_full_20260911` |
| `historical_chains.hpp` | `686c2137…` | `atlas_graph_full_20260911` |
| `graph_full.hpp` | `bad5051f…` | `atlas_graph_full_20260911` |
| `composable_msf.hpp` | `dba0ae64…` | `atlas_graph_full_20260911` |
| `window_workers.hpp` | `4ec1a206…` | `birth_streaming_20260911` |
| `streaming_graph.hpp` (première version) | `fb9f0c0c…` | `ordered_streaming_20260911` |
| `streaming_graph.hpp` (seconde version) | `0e50808c…` | `rank_guard_streaming_20260911` |
| `rank_guards.hpp` | `81a32d8c…` | `rank_guard_streaming_20260911` |

### 7.2 À ne pas reprendre (avec la mesure ou la preuve qui a fermé la piste)

| Piste | Fermée par | Source |
| --- | --- | --- |
| Porter le fold v4 (K-graphe de Gabriel seul) comme tour FULL | `false_in_general` : E5 crée une fausse naissance puis une fausse fusion ; minima isolés et K=n absents | `STATUT_PREUVES_ET_HEURISTIQUES.md:40`, `:129` ; `AUDIT_NIVEAUX_GABRIEL_20260905.md:138-162` |
| Garder les minima avec leurs seules adjacences induites | fixture constructeur : deux composantes, fusion généreuse retardée de 477/34 à 31/2 ; fixture auditeur : 169/9 → 41/2 | `SQUELETTE_MINIMA_GABRIEL.md:44-78` ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:29` ; `FAUSSES_PISTES.md:35` |
| Publier tous les niveaux Gamma | preuve de suffisance des minima et des multifusions | `FAUSSES_PISTES.md:34` |
| Identifier une composante par sa couverture de points | deux identités peuvent avoir la même couverture | `FAUSSES_PISTES.md:40` |
| Filtre p+u ≤ smax pour les coquilles non régulières | une coquille à sept points donne une naissance K5 | `FAUSSES_PISTES.md:68` |
| Omettre l'ancre d'une boule localement inerte | contre-exemple à cinq points : résolveur en échec | `PLATEAUX_FULL_ET_ANCRES.md:104-107` ; `FAUSSES_PISTES.md:67` |
| Installer toutes les facettes incidentes comme alias (eager) | plafond atteint à 16k/K9 et 32k/K7 | `FAUSSES_PISTES.md:41` |
| Présenter moins d'alias (lazy) comme une accélération | −28 % de pic mémoire, sans gain de temps sur trois paires 8k | `FAUSSES_PISTES.md:42` |
| Welzl à base non bornée, ou cas de base MEB(R) | ×2,2 pire en candidats ; `canon_fail` sur la contre-fixture K7 | `NOTE_CLAUDE_COEUR_MEB_20260911.md:250-258`, `:305-331` ; `FAUSSES_PISTES.md:14` |
| Comptabilité de supports incomplète | rapport annoncé 14,75, ramené à 3,25 une fois toutes les branches facturées | `NOTE_CLAUDE_COEUR_MEB_20260911.md:129-141` |
| Terminal systématique à un seul passage avec coins | −2,79 % de visites, +100,77 % de coins, front 37,767 → 38,287 s | `FAUSSES_PISTES.md:58` |
| Blocs forcés pour tous les histogrammes d'extrémité | 93,819 → 186,560 ms sur uniforme 8k | `FAUSSES_PISTES.md:59` |
| Crédit de sous-arbres sur grands facteurs (front WSPD) | visites presque quadratiques ; à 32k, q4 13,446 s contre 10,697 s au scalaire | `FAUSSES_PISTES.md:60` |
| Flux de terminales fenêtré comme optimisation mono | W = 65 536 : 188,638 → 250,408 s, +10,4 % de MEB | `FAUSSES_PISTES.md:75-80` |
| Intégrer le journal incrémental owning | +7,34 % d'appels `new`, +30,02 % de mémoire retenue à n800 | `FAUSSES_PISTES.md:23` |
| Présenter le tri-unique statique comme un gain mémoire | +308 Mo de capacités temporaires à 8k | `FAUSSES_PISTES.md:27` |
| Mémoriser seulement le hash ou l'ancienne racine d'une facette | collision non identifiante ; racine historique déjà fusionnée | `FAUSSES_PISTES.md:24` |
| Tronquer la pile de recherche à huit | peigne Morton à 49 références | `FAUSSES_PISTES.md:21` |
| Espérer 1 s par le seul port préfiltre/census | plus de 15 s de génération et de tri CPU48 avant FULL à 50k (12,23 + 3,13 s) ; 189 ms de kernels pour 4,540 s de phase et 418,9 s de tour | `FAUSSES_PISTES.md:33` ; § 4 |
| Route census CUDA synchrone par petits lots | 2,932 s de reconstruction hôte contre 0,189 s de kernels | `IMPLEMENTATION_PARALLELISATION.md:276-284` |
| Copier dix fois le constructeur | multiplie allocations et calendriers ; les boules sont communes | `FAUSSES_PISTES.md:109-111` |
| Prendre le calendrier actuel pour un obstacle mathématique | les horizontales sont indépendantes par K, les verticales se calculent hors ligne ; coût mesuré, pas universel | `FAUSSES_PISTES.md:112-114` ; `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:130-145` |
| Remplacer partout J=1 par la descente | deux MEB là où J=1 en demande une, sur une fixture régulière | `FAUSSES_PISTES.md:36` ; `SQUELETTE_MINIMA_GABRIEL.md:208-217` |
| Garder un quota de quatre millions d'appels MEB | garde-fou d'essai, pas nécessité ; refus K9 à 32k | `FAUSSES_PISTES.md:46-47` |
| Juger un terminal par la seule composante finale | deux mutants survivants : une mauvaise boule rejoint la bonne racine | `FAUSSES_PISTES.md:18` |
| Déclarer un parallélisme ×20 sous découplage des ordres | plafond réel 11,4× ; découplage qui, selon la lecture du code, échouerait trois contrôles physiques | `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:146-172`, `:211-240` |
| Juger une refonte par le seul `payload_digest` | aveugle à la renumérotation des populations | idem `:217-222` |
| Supposer qu'un nuage uniforme u16 est régulier | refus G4 50k : quatre extra-shells à K10, trois à K5 | `FAUSSES_PISTES.md:64` |
| Transférer un chiffre d'un en-tête du Builder à un autre | chaque delta a sa propre capture ; « les signatures présentes ne lui seront pas transférées » | `QUALIFICATION_TOUR_CENSUS_K10_20260911.md:78-80` |

## 8. Recommandations priorisées pour la v9

1. **Figer l'objet avant le code.**
   - Écrire dans l'entrée v9 que la tour à livrer est l'objet FULL v7, extension non régulière incluse (`STATUT_PREUVES_ET_HEURISTIQUES.md:131-134`).
   - Écrire aussi que le fold v4 sur le K-graphe de Gabriel n'est pas ce produit (`:40`, `:129`).
   - Trancher explicitement la contradiction entre `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:196-197` et `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:10-14`.
2. **Première porte v9 : le juge T2 porté.**
   - Reprendre `census_tower_oracle.hpp` et `census_tower_gate.cpp`, gardes de métadonnées incluses.
   - Y graver E5, ABCDE, les deux fixtures à quatre points, la coquille à sept points, le triangle rectangle, K7, la descente forcée K4 et les trois coquilles réelles 50k.
   - Les adapter au profil arithmétique v9.
   - Comparer terminales avant normalisation, parents, successeurs, populations et verticales, jamais le seul digest.
3. **Construire en mono la tranche aval minimale.**
   - Travailler en différentiel contre le Builder `83f1c78e…`, sur entrées u16 communes.
   - Enchaîner : catalogue canonique dédoublonné depuis le flux de candidats v8, puis résolution statique par facettes uniques, contraction des pivots sur naissances, histoires datées, enfin contributions et verticales.
   - Décider dès la conception d'une numérotation des populations indépendante de l'ordre de découverte, ou d'une canonisation comparée par bijection explicite.
4. **Premier chiffre utile : une tour FULL complète sur le régime prioritaire.**
   - Cible : LiDAR sans sol, trois scènes 08/000000, 000100 et 000200, profil u18 à 1 mm, K5 puis K10.
   - Mesurer ensuite la trame brute entière, qui reste le contrat principal.
   - Publier nœuds, contributions, références verticales, RSS et octets de sortie.
   - Ne transférer ni les 419 s / 34 s uniformes, ni les triplets uniformes.
5. **Requalifier le domaine arithmétique du Builder pour u18, puis float32.**
   - Fixer de nouvelles bornes de clés.
   - Passer `BallId` et les offsets en 64 bits là où les objets dérivés dépassent 2^32.
   - Traiter ou refuser explicitement les coquilles de plus de 12 sites et les intérieurs de plus de 9.
   - Mesurer leur fréquence sur LiDAR.
6. **Intégrer le noyau MEB exact accéléré.**
   - Paire diamétrale d'abord, confinement par extrêmes, proposition, puis canonisation sur la coquille sélectionnée, avec repli exact compté.
   - Porte K7 obligatoire et mesure sur flux réel LiDAR.
   - Ne pas promettre le produit des facteurs mono et parallèle.
7. **Fermer les angles morts connus.**
   - Plancher de couverture et mutant pour les lots groupés.
   - Porte TSan réelle des workers.
   - `power_tests` dans la sonde du Builder.
   - Découpe géométrie / calendrier / prologue / épilogue dans la sonde.
   - Suite CTest complète attestée sur l'en-tête porté.
8. **Décider le format de nœud**, ou une sortie implicite contractualisée, avant de viser 100 ms. Mesurer le plancher d'écriture sur G4, avec reçu.
9. **GPU seulement après la tranche CPU.**
   - Objets résidents : index, catalogue, semis.
   - Terminal composé exécuté sur carte, transferts froids et reconstruction compris.
   - Ne pas reconduire la route census synchrone.
10. **Régler la dette du worktree partagé.**
    - Le delta v7 « marques au premier parcours » doit être commis par son auteur ou archivé avec une note, jamais hérité implicitement.
    - Découpler les contrôleurs G4 v9 de l'exécution de `full_probe_session_v7.py`, ou geler ce fichier explicitement.

## 9. Contre-vérification

### 9.1 Verdicts sur les affirmations principales du rapport d'origine

| # | Affirmation d'origine (abrégée) | Verdict | Note |
| --- | --- | --- | --- |
| 1 | Sous régularité, FULL = minima Gabriel de cardinal K + vraies multifusions à K+1 avec parents pré-lot ; portails internes | confirmé | `STATUT:131-133` et `AUDIT_NIVEAUX:59-114`, `:268-298` relus |
| 2 | Le fold des seules cofaces Gabriel (type v4) ne reconstruit pas FULL (E5, minima isolés, K=n) | confirmé | précisé : le fold v4 publie aussi naissances et croissances (`ComponentDelta`), mais sur le K-graphe de Gabriel ; `STATUT:40`, `:129` ; fixture `0c74c429…` |
| 3 | Quatre points u16 réguliers : fusion retardée de 169/9 à 41/2, preuve `SQUELETTE:46-78` | corrigé | deux fixtures distinctes : `SQUELETTE:44-78` et la porte `bee615b5` donnent 477/34 → 31/2 ; 169/9 → 41/2 est la fixture de l'auditeur (`NIVEAUX:29`, `receipts_gabriel_vertices_20260906`) |
| 4 | Hors régularité : naissances de plus de K points, contributions datées, ancres inertes, fenêtre p+q_min | confirmé | `PLATEAUX:31-47`, `:76-107` |
| 5 | Sortie FULL quadratique dès K=2 | confirmé | 2 008 identités, huit mutants (`CROISSANCE:144-161`) |
| 6 | Builder `83f1c78e` : 28 nuages, 112 ordres, 45 948 verticales, 170 320 contrôles, six mutants | corrigé | ces chiffres qualifient `910f45ba…` ; `83f1c78e…` a 34 nuages, 150 ordres, 87 230 verticales par bras, un callback à 298 742 contrôles et le T2 par overlay |
| 7 | T2 census→FULL K10 : 54 tours, 13 000 coupes, 8 103 948 verticales, neuf refus, quatre mutants | confirmé | précisé : sources `c03f6be8` (en-tête `33e7d05e…`), requalifié sur `6763a877…` et en overlay `83f1c78e…` ; 12 315 725 inclut +120 contrôles de métadonnées |
| 8 | Tour 50k G4 : 418,873 / 418,921 s ; FULL 389,668 s ; 33,853 / 33,569 s ; seule tour FULL complète jamais mesurée | corrigé | valeurs exactes, en-tête `910f45ba…` ; c'est la seule tour **à 50k et sur G4**, des tours complètes 8k/16k/32k existent ; toutes uniformes |
| 9 | Census CUDA : 189 ms de kernels pour 4,54 s de phase | confirmé | précisé : la route CPU prend 5,397 s, gain négligeable devant la tour |
| 10 | Deux primitives exactes exécutées sur G4 ; terminal composé jamais sur carte | confirmé | objets `2436eaa8…`, `041bc9ce…` relus |
| 11 | Graphes datés et workers produisent les mêmes tours ; CPU4 92,963 / 215,381 / 489,601 s | corrigé | temps confirmés, mais l'égalité se fait modulo une bijection explicite des identités natives, et sans comparaison physique à 32k (`8cc9dadd…`) |
| 12 | À 32k CPU4 : validation 42,6 s, histoires 50,5 s, export 92,0 s sur 478,6 s | confirmé | objet `8cc9dadd…` relu |
| 13 | Noyau MEB : ×3,25 supports, ×1,36 tour, digest identique, coquille = support à 100 % | confirmé | précisé : en-tête `6763a877…`, `--static-threads=1` ; coquille locale ≤ 10 sites ; zéro repli sur 198 000 cas n'est pas une preuve |
| 14 | Calendrier plus rapide que la géométrie (1,243 contre 1,094) ; plafond 2,69 → 1,98 | confirmé | nuance : séquentiel « tel qu'écrit » (§ 4bis), pas un obstacle mathématique (`FAUSSES_PISTES:112-114`) |
| 15 | La v8 n'a porté aucun code v7 | corrigé | aucun code moteur, mais les contrôleurs G4 v8 exécutent les helpers de `full_probe_session_v7.py` (`177b25a0…`) ; en-têtes Boost sous `build/v7_boost_gate` |
| 16 | Contradiction entre l'audit du 19/09 (sémantique du Builder) et celui du 21/09 (fold v4) | confirmé | texte daté du 20/09 (commits `ae98dbfa` 19/09, `c92aad13` 20/09) |
| 17 | Builder limité au domaine u16, coquilles ≤ 12, masques u16, `BallId` u32 | confirmé | précisé : `BallId` défini à `:109`, limite de neuf intérieurs ; depuis le 22/09 le contrat temps est u18, float32 hors contrat temps |

### 9.2 Verdicts sur les chiffres

| Chiffre d'origine | Verdict | Note |
| --- | --- | --- |
| Tour 50k K10 CPU 418,872898 / 419,809931 / 389,667561 s | confirmé | JSON relu |
| Hybride K10 418,920854 s ; FULL 390,480778 s | confirmé | JSON relu |
| K5 33,852713 / 33,569245 s ; 34,006 / 33,790 s | confirmé | JSON relu |
| Nœuds 27 273 218 / 4 209 792 | confirmé | — |
| Boules 21 468 368 (4) / 4 010 348 (3) | confirmé | — |
| MEB 41 986 201 / 4 383 525 | confirmé | — |
| Supports 3 898 856 828 / 44 413 779 | confirmé | — |
| Contributions et verticales K10 16 495 216 / 27 173 227 | confirmé | — |
| RSS 16 206 376 / 16 318 064 ; 3 785 592 / 3 949 772 KiB | confirmé | stderr relus un par un |
| Kernels 189,346 ms / 4,540 s / 2 931,852 ms | confirmé | — |
| Gate device 16 627 / 4 116 / 17 | confirmé | + trois extra-shells, q2/q3/q4 846/2 085/1 185 |
| Sélection MEB G4 605 / 21 432 / 44 / 197 | confirmé | — |
| Clé/PGCD G4 13 573 / 325 752 / 22 | confirmé | — |
| Mono cache 8k/16k/32k | confirmé | en-tête `910f45ba…` |
| RSS mono cache | confirmé | stderr relus |
| s8/s10/s12 à 8k | confirmé | — |
| Première tour par boules 215,169 / 417,627 / 965,053 s | confirmé | document relu, JSON non relu |
| CPU4 92,963 / 215,381 / 489,601 s ; RSS 32k 11 608 968 | confirmé | objets relus |
| CPU4 rangs 32k 478,615725 s ; 11 607 552 KiB | confirmé | — |
| Décomposition 32k | confirmé | — |
| Paire de threads 187,214 → 164,703 ; 58,649 → 34,976 | confirmé | objets relus |
| Statique 4 185 184 / 8 779 465 / 18 244 853 | confirmé | — |
| Semis 237 557 / 501 258 / 1 045 620 ; −5,68/5,71/5,73 % | confirmé | pourcentages recalculés |
| Noyau MEB 8k et 16k | confirmé | en-tête `6763a877…` |
| Coquille = support 100 % | confirmé | portée locale |
| Découpe 62,8 / 16,8 / 15,4 / 5,0 % | confirmé | — |
| Exposants 1,094 / 1,243 | confirmé | — |
| Plafond 11,4× / 11,2× | confirmé | — |
| Lots groupés 0,0086 / 0,0264 / 0,0843 % | confirmé | — |
| T2 K10 54 / 13 000 / 8 103 948 / 12 315 725 | confirmé | — |
| Gate tour par boules 28 / 112 / 2 508 / 45 948 / 170 320 | corrigé | en-tête `910f45ba…`, pas `83f1c78e…` |
| Refus G4 du 6/09 21,372 / 5,646 s | confirmé | `CONTRATS_ET_MESURES:128` |
| Plancher d'écriture 1,75 Go ; 62,7 ms | non vérifiable | 1,75 Go = 27 273 218 × 64 octets exact ; aucun reçu pour 62,7 ms |
| Pivot4 573 295 → 79 173 ; 0,262–0,271 | confirmé | microbenchmark n = 24/48 |
| Témoins de borne 9/25/81/289 | confirmé | `NIVEAUX:39` |
| Trame 119 142 / 119 942 / 120 725 sites à 2 cm | corrigé | valeurs historiques à 2 cm ; cibles v9 = trame brute float32 (123 389 / 124 479 / 125 526 retours) et sans sol (39 885 / 35 551 / 45 845 sites float32) |
| Volume v7 : 122 commits, 26 777 fichiers, 471 Mo, 134 + 48 reçus, 18 189 lignes | confirmé | recompté ; activité sur cinq jours, pas huit |

### 9.3 Corrections intégrées

1. Attribution des qualifications et mesures par en-tête du Builder (§ 2, tableau de lignée ; § 3.2 ; § 4).
2. « Seule tour FULL complète jamais mesurée » devient « seule tour FULL complète à 50k et sur G4 » ; toutes les tours sont uniformes.
3. Les deux fixtures à quatre points sont distinguées, avec leurs niveaux.
4. Le mode de comparaison du graphe daté (bijection) est précisé, ainsi que l'absence de comparaison physique à 32k.
5. L'héritage non moteur repris par la v8 (contrôleur G4 v7, en-têtes Boost) est ajouté.
6. Le plancher d'écriture de 62,7 ms est reclassé non vérifiable.
7. Les cibles LiDAR sont corrigées : régime sans sol u18 prioritaire depuis le 22/09, trame brute float32 comme contrat principal.
8. L'effet réel de la route CUDA est chiffré (5,397 → 4,540 s).
9. Citations corrigées :
   - `BallId` défini à `full_ball_tower.hpp:109` ; limite de neuf intérieurs ;
   - `PASSATION.md:692-694` appartient à la section historique ;
   - date du texte des facettes silencieuses ;
   - `CONTRAT_PERFORMANCE.md:81` pour p95 et dix nuages frais ;
   - raison de `FAUSSES_PISTES.md:33`.
10. L'échec des « trois contrôles » est requalifié comme déduction par lecture du code, appuyée par un mutant voisin exécuté.
11. Le calendrier est requalifié : séquentiel tel qu'écrit, pas un obstacle mathématique.
12. La chronologie compte cinq journées d'activité, pas huit jours.

### 9.4 Omissions ajoutées

- La lignée des quatre en-têtes du Builder et la non-transférabilité des chiffres entre eux.
- Le meilleur triplet mono (`6763a877…`) : 141,366 / 318,968 / 694,459 s ; FULL 58,977 / 134,329 / 292,341 s ; 8,687 Gio à 32k.
- L'absence totale de tour FULL sur famille non uniforme.
- L'absence de mesure 8k–50k sur l'en-tête final `83f1c78e…`.
- Les parents globaux certifiés des coquilles réelles 50k, à porter comme fixtures.
- La suite 449/449 attestée sur `6763a877…` seulement.
- L'amont v7 à 50k CPU48 : génération 12,23 s, tri 3,13 s ; digest de 8,37 s inclus dans `total_s`.
- L'antériorité de la divergence d'objet v4/v5 contre v7 (`CLAUDE.md`).
- L'allocation indicative ≈ 102 s / ≈ 7 s, classée « proposé ».
- La présence du delta v7 non commis dès l'ouverture v8.
- `STATUT:136` : les scores ne demandent pas toutes les cofaces Gamma.
