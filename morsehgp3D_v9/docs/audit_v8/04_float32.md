# Lentille 4/12 — Profil float32 sans perte (audit v8 avant ouverture v9) — version contre-vérifiée

```text
phase=exploration_v8_hors_registre (audit de clôture, lecture seule)
backend=cpu_reference
profile=lossless_float32_input_only (voie auditée) ; moteur de référence quantized_u16/u18_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

GCP non utilisé. Ni compilation, ni ctest, ni benchmark n'ont été lancés par cette lentille ou par sa contre-vérification. Les seuls journaux d'exécution cités (`head_ctest.log`, `mut_ctest.log`) viennent de l'orchestrateur de cet audit : ils sont dans le scratchpad de la session, dans les builds `build/v9-audit-v8-head-12294241` (GCC 13.3, Release) et `v8_head/build/v9-audit-mutations`, et **ne sont pas des reçus épinglés**. Vocabulaire : **prouvé** = preuve écrite + fixture ; **testé** = porte bornée exécutée ; **mesuré** = reçu épinglé (commit, sha256, sorties) ; **proposé** = plan ou brouillon ; **manquant** = absent. Un chiffre sans reçu est **non vérifiable**. Les corrections de la contre-vérification sont intégrées ci-dessous et listées une à une dans la dernière section (C1 à C22).

## 1. Périmètre lu

**État publié de référence** : worktree détaché `v8_head` = `origin/main` `12294241`. Aucun fichier source, test ou banc de la voie float32 n'a changé après `a005f8aa`. La vérification par `git log a005f8aa..12294241`, chemin par chemin, est vide pour `src/core/float32_*`, `fixed_signed.hpp`, `src/lanes/float32_*`, `src/spatial/*`, `tests/float32_*`, `bench/*float32*` et `bench/prepare_lidar_precision.py`. Après `a005f8aa` n'ont changé que des documents et des métadonnées :

- `204b0620` ajoute 3 lignes au README du reçu census et deux paragraphes à `ANALYSE_CROISSANCE.md` ;
- `92d74c13` modifie `CMakeLists.txt`.

Lus intégralement :

| Fichier | Lignes |
|---|---:|
| `morsehgp3D_v8/docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md` | 165 |
| `morsehgp3D_v8/docs/INDEX_FLOAT32_ET_SUITE_Q34_20260921.md` | 207 |
| `morsehgp3D_v8/docs/BOULES_FLOAT32_Q3_Q4_20260921.md` | 159 |
| `morsehgp3D_v8/docs/IDENTITE_FLOAT32_ET_EVENEMENTS_Q4_20260921.md` | 137 |
| `morsehgp3D_v8/docs/CENSUS_Q3_FLOAT32_PARTAGE_20260921.md` | 221 |
| `morsehgp3D_v8/docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md` | 168 |
| `morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` | 234 |
| `morsehgp3D_v8/docs/JOURNAL_DEVELOPPEMENT_20260921.md` | 149 |
| les 14 sources de la voie : `src/core/float32_predicates.hpp`, `fixed_signed.hpp`, `float32_ball.*`, `float32_ball_key.*`, `float32_q4_events.*`, `float32_q3_block.*`, `src/lanes/float32_q3_census.*`, `src/spatial/float32_index.*` (sous `morsehgp3D_v8/`) | 2 093 |
| READMEs des cinq reçus `morsehgp3D_v8/receipts/float32_{precision,index,ball,identity,q3_census}_20260921/`, `float32_ball_20260921/mutations/README.md`, `float32_q3_census_20260921/ANALYSE_CROISSANCE.md` | — |
| `morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/README.md` (F1–F10) | 62 |

Lus partiellement :

- `morsehgp3D_v8/CMakeLists.txt` : l. 40–180, 400–475 et 550–618 ;
- `tests/float32_q3_census_gate.py` : l. 1–40 et 185–325 ;
- `tests/float32_q3_census_mutations.py` : en-tête et cibles ;
- `tests/float32_ball_mutations.py` : noms des mutants ;
- `morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md` : l. 640–990 ;
- `audits/COORDINATION_MORSEHGP3D_V8.md` : l. 3540–3860 ;
- `morsehgp3D_v8/PASSATION.md` : l. 1–80 ;
- `docs/ELARGISSEMENT_18_BITS_20260922.md` : l. 1–72 ;
- `AGENTS.md` : l. 1–40, plus un grep sur les sections float32 ;
- `docs/FAUSSES_PISTES.md` : l. 20–75 ;
- `README.md` : l. 1–45 ;
- `audits/ETAT_COURANT.md` : l. 1–35 ;
- `audits/BUDGET_CONTRAT_50K_20260914.md` : l. 1–50 ;
- `morsehgp3D_v7/src/pipeline/float_filter.hpp` : l. 1–80.

Contrôles refaits par la contre-vérification :

- sha256 de tous les `COMPLETION.json` des reçus float32 : égaux aux valeurs publiées ;
- sha256 des 14 sources au HEAD, comparés aux instantanés `sources/` des reçus `float32_q3_census_20260921/release/q3_census_4d38rabq` et `float32_identity_20260921/release/identity_0ecgriwi`. Le census contient 10 des 14 sources et l'identité 8 ; leur union donne les 14, **octet pour octet identiques** au HEAD. Seul diffère `bench/run_p0_matrix.py`, un auxiliaire copié dans les instantanés et hors de la voie ;
- champs `git_commit` des 14 `MANIFEST.json` ;
- volumes des reçus, des charges utiles KITTI et des builds épinglés ;
- journaux CTest de l'orchestrateur.

**Worktree partagé, NON COMMIS** (étiqueté comme tel partout) : brouillon de raccord global non suivi (`??`), écrit le 21 septembre entre 20:12 et 20:24 UTC. Il comprend 10 sources de **1 330 lignes** et 3 tests de 1 120 lignes, soit 2 450 lignes :

- sources : `src/wspd/float32_front.{hpp,cpp}`, `src/core/float32_edge_geometry.{hpp,cpp}`, `src/core/float32_q3_owned_block.{hpp,cpp}`, `src/lanes/float32_q3_owned.{hpp,cpp}`, `src/pipeline/float32_q3_global.{hpp,cpp}` ;
- tests : `tests/float32_front_probe.cpp`, `tests/float32_q3_global_probe.cpp`, `tests/float32_q3_global_test.py` ;
- s'y ajoute `receipts/float32_q3_global_20260921/preflight/missing_boost/`.

La tranche indexée et non indexée du constructeur, datée du 22 septembre entre 10:29 et 10:36 UTC, ne touche aucun fichier float32 : `git diff --cached --name-only` et `git diff --name-only`, filtrés sur `float32`, sont vides. Elle touche en revanche le texte du contrat de précision (`AGENTS.md`, `README.md`, `PASSATION.md`, `CONTRAT_TRAMES…`, `REPRISE_U18_ET_ATLAS_SATURANT_20260922.md`).

**Non lu** :

- les sondes C++ `tests/float32_*_probe.cpp`, hors en-têtes et greps ;
- les portes Python autres que le census ;
- les lanceurs `bench/run_float32_*_checks.py`, `bench/float32_index_probe.cpp`, `bench/analyze_float32_index.py` et `bench/prepare_lidar_precision.py` ;
- `GROWTH.json`, les transcriptions brutes et les flux natifs ;
- `BLOCK_ENVELOPE_FIXTURES.json` et `block_envelope_fixtures.py` ;
- `AGENTS.md` et `COORDINATION_MORSEHGP3D_V8.md` hors des sections citées.

La source primaire des chiffres « ×23 à ×70 par prédicat », « clé 78 µs » et « 68 à 185 ms par arête » est introuvable. Ces chiffres n'apparaissent que dans `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65` (et « ×20 à ×70 » l. 92) et dans le message du commit `8c050a33` (« float32 predicates x23-70 slower »). Ce même audit déclare l. 231-233 : « aucune mesure n'est épinglée ».

## 2. Ce qui a été fait

Le code de la voie float32 a été commis entre 14:56 et 19:38 UTC le 21 septembre, en cinq commits de code. Les fichiers du bloc et du census existaient déjà, non suivis, avant la relecture B de 19:30 (`DIALOGUE_AUDITEUR_B.md` l. 723-726). La voie a été gelée le 22 septembre.

| Date (UTC) | Commit | Objet | Reçu / preuve |
|---|---|---|---|
| 21/09 14:56 | `36724438` | Préparateur float32 sans perte et grille décimale exacte (1 mm par défaut) ; primitive q2 exacte (filtre par intervalles + accumulateur 576 bits) | `receipts/float32_precision_20260921/` : R1 historique, R2 fait autorité ; premier Sanitize FAILED au lien, conservé |
| 21/09 15:48 | `028a0f1d` | Index natif float32 : copie privée, rangs médians sur trois tris, profondeur ≤ ⌈log2 n⌉, boîtes aux bits exacts, liens `escape` | `receipts/float32_index_20260921/` : 54 mesures, 3 mutants compilés |
| 21/09 16:40 | `12d885d8` | `Float32Ball` q3/q4 (validité stricte, puissance exacte) ; `FixedSigned` à 54 mots, 1 728 bits | `receipts/float32_ball_20260921/` : 3 mutants compilés. Son lecteur LIVE **refuse désormais** l'en-tête étendu par `9923a6b9` (`float32_identity_20260921/README.md:36-38`) |
| 21/09 17:04 | `9923a6b9` | `Float32BallKey` (quintuplet primitif global) ; `Float32Q4Events` (comparateur réduit de degré 5) ; PGCD et division exacte | `receipts/float32_identity_20260921/` : 2 mutants compilés ; rejoue aussi les 1 636 cas de boules sur le nouveau code |
| 21/09 19:30 | `74fb0a6a` (B) | Relecture à quatre lentilles ; preuve écrite de `GΔ = P₂B₁ − P₁B₂` ; critique de l'enveloppe de centres | `morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md` l. 650–822 |
| 21/09 19:38 | `a005f8aa` | `Float32Q3Block` (enveloppe de centres conditionnelle, 6 paraboles et 18 évaluations de puissance par requête) ; census q3 d'**une** arête, modes `Individual` et `SharedPrefix` | `receipts/float32_q3_census_20260921/` : Release, Sanitize, matrice de 36 observations, 2 mutants |
| 21/09 19:38 | `21b85af2` (B) | Fixtures exactes F1–F8 des bornes de bloc | `morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/` |
| 21/09 19:57 | `f7b220c4` (B) | Relecture de `a005f8aa` : aucun défaut produit, trois risques de portée | `DIALOGUE_AUDITEUR_B.md` l. 824–934 |
| 21/09 20:04 | `ad9bbc9d` (B) | F9 (population des témoins) et F10 (égalité de propriété) | même dossier de fixtures |
| 21/09 20:08 | `204b0620` | Plan `RACCORD_NATIF_GLOBAL_PLAN_20260921.md` (quatre raccords A–D, « pas qualification », l. 3) ; ajouts textuels a posteriori au README et à `ANALYSE_CROISSANCE.md` du reçu census | plan |
| 21/09 20:12–20:24 | **non commis** | Brouillon de générateur q3 global : front avec rejet de rectangle optionnel par échantillons de milieu, géométrie d'arête, bloc propriétaire (ξ ≤ 1/3), voie propriétaire, pipeline, sondes, porte Fraction | seul artefact : un préflight FAILED à 20:21. Il porte sur une **première version** de la sonde, qui incluait Boost ; les sondes actuelles n'incluent plus Boost |
| 21/09 21:07 | `8c050a33` | Audit de reprise à neuf lentilles ; décision proposée D1 : le moteur entier va vers les contrats, le float32 reste « hors contrat temps » | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md` l. 123–135 |
| 21/09 21:36 | `92d74c13` | Cible CMake `mhgp8_f32` avec options flottantes PUBLIC ; 23 CTests au label `float32` (19 portes + 4 scripts de mutation) | message de commit : 132 tests, 121 verts + 2 DISABLED + 9 mutations ; pas de reçu |
| 22/09 06:21 | `a74e90f2` | Décision utilisateur : le contrat temps passe sur l'entier 18 bits ; « aucun développement float32 pour l'instant ». Le texte n'entre **que** dans ce commit, pas dans `93ba5bb4` (05:35, journal des phases 1-2) | `PASSATION.md:36-39`, `ELARGISSEMENT_18_BITS_20260922.md:1-6,67-70`, `JOURNAL_DEVELOPPEMENT_20260921.md:102-107` |

## 3. État par composant

| Composant | Statut | Preuve |
|---|---|---|
| Préparateur float32 / grille décimale exacte (`bench/prepare_lidar_precision.py`) | **testé + mesuré** | 15 tests de préparation, 42 nuages (3 trames × 2 profils × 7 objets), round-trip de 370 167 / 373 437 / 376 578 coordonnées, erreur de grille ≤ 0,5 mm ; clôtures R2 `09eacb2c…` (Release) et `8be076e9…` (ASan/UBSan) (`receipts/float32_precision_20260921/README.md:34-60,97-100`). Réserve du lecteur : binaires et dépendances non recontrôlés en fin de lecture ; vérification manuelle de 252 dépendances sur 255 (l. 89-95) |
| Prédicat q2 exact (`src/core/float32_predicates.hpp:119-243`) | **prouvé (capacité) + testé** | Borne écrite : chaque terme < 2^554 en unité 2^-298, douze termes < 2^558 < 576 bits (l. 76-81) ; 3 923 requêtes contre Fraction, 49 contrôles natifs, 4 modes d'arrondi. Réserves : FTZ/DAZ non exercé expérimentalement pour q2 (`PRECISION…:102-103` ; la sonde n'appelle que `fesetround`, l. 87-99), mais B argumente que le filtre tient sous FTZ/DAZ (`DIALOGUE_AUDITEUR_B.md` l. 710-711) ; 5 mutations de **résultats**, aucun mutant compilé, aucun script de mutation q2 en CTest |
| Entier fixe `FixedSigned` (`src/core/fixed_signed.hpp`) | **prouvé + testé** | Borne de degré 6 < 2^1677 < 1 728 bits écrite l. 23-31 et recalculée par B (1 671 bits au pire observé, l. 704-707) ; 948 + 9 051 contrôles (185 divisions exactes, 1 124 PGCD, 347 refus pour reste) (`receipts/float32_identity_20260921/README.md:59-62`) ; un seul mutant compilé touche ce fichier (décalage d'exposant normal, lot boules) ; aucun sur le PGCD ni la division |
| Index natif float32 (`src/spatial/float32_index.*`) | **prouvé + testé + mesuré** | Preuve O(n log n), profondeur ≤ ⌈log2 n⌉ (`INDEX_FLOAT32_ET_SUITE_Q34_20260921.md:43-66`) ; fixture de 278 points, profondeur 9 ; 27 fixtures, 404 requêtes, 35 refus, 151 contrôles natifs, 4 arrondis × 4 combinaisons FTZ/DAZ ; 3 mutants compilés tués (zéros signés, boîte ouverte, `escape` interne) ; 54 constructions épinglées ; clôtures `7419b09f…`, `da79e4a9…`, `572fcb3e…`. Sources inchangées depuis `028a0f1d` |
| Supports q3/q4 et puissances (`src/core/float32_ball.*`) | **prouvé + testé** | Formules et bornes (`BOULES_FLOAT32_Q3_Q4_20260921.md:64-105`) ; oracle indépendant par élimination rationnelle du centre, 1 636 cas ; 3 mutants compilés (poids nul, signe du déterminant dans la puissance, décalage d'exposant). **Le code au HEAD est celui de `9923a6b9`** : il est couvert par la régression du lot identité (1 636 cas rejoués, 1 287 contrôles natifs) et non par les clôtures `748aaae3…`, `14f71201…`, `07c97cb2…` du lot boules, dont le lecteur LIVE refuse l'en-tête étendu |
| Clé canonique `Float32BallKey` | **prouvé + testé** | Unicité du vecteur primitif `(A,Bx,By,Bz,C)` confirmée par B (`DIALOGUE_AUDITEUR_B.md` l. 658-666) ; 229 cas, 63 boules distinctes, dont 10 tri-arités qui sont **une seule configuration** (centre 0, rayon 5) sous dix similitudes (B l. 671-674) ; mutant « translation globale omise » tué. Limites : seulement `operator==` (`src/core/float32_ball_key.hpp:56`) ; mais l'encodage canonique est public (`serialized_words()`, l. 55), donc un ordre lexicographique ou un hachage externes sont possibles sans toucher la clé ; `operator=` est supprimé et un déplacement recopie le vecteur (l. 51-54), donc aucun tri en place |
| Comparateur d'événements q4 (`src/core/float32_q4_events.*`) | **prouvé + testé** | Preuve écrite par B de `GΔ = P₂B₁ − P₁B₂` et `sign(t₁ − t₂) = −sign(Δ)·sign(B₁)·sign(B₂)` (`DIALOGUE_AUDITEUR_B.md` l. 681-691) ; 960 cas (521 comparaisons valides, dont 202 égalités) ; mutant « signe du second dénominateur » tué ; rejeu B de 6 044 événements sans désaccord (l. 711-712) |
| Bornes de bloc q3 (`src/core/float32_q3_block.*`) | **prouvé (sûreté) + testé ; lâche** | Sûreté : B confirme 0 violation sur 297 boîtes (l. 736-737) ; minimum au sommet borné (F1). L'enveloppe `hull ∩ (a + W/(2G))` est plus large que l'enveloppe resserrée de A : rapport de largeur min 1,25, médiane 4,75, max 155. Face à l'enveloppe universelle de A : min 0,29, médiane 1,92, max 72 ; le constructeur y est parfois plus serré (l. 765-767). Repli hull sur 843 des 1 642 préparations des fixtures (51 %), 85 intersections vides (l. 853-856). **Aucun mutant compilé** sur ce fichier (`tests/float32_q3_census_mutations.py:40`, `TARGET` = census seul) |
| Census q3 d'une arête (`src/lanes/float32_q3_census.*`) | **testé + mesuré (synthétique)** | 18 fixtures, 612 appels Fraction, 1 798 émissions, 8 610 IDs de coquille, coquille maximale 30 ; 2 mutants compilés tués ; matrice de 36 observations (8k/16k/32k, K5/K10) ; clôtures `97d12e38…`, `f530d919…`, `706d96cb…`, `4520d1e6…`. Uniquement **une arête fournie**, familles `column`/`slab` à sortie constante, jamais une trame LiDAR |
| Enregistrement CMake/CTest | **testé (non épinglé)** | `morsehgp3D_v8/CMakeLists.txt:59-70` (cible `mhgp8_f32`, `-ffp-contract=off -fno-fast-math -frounding-math` PUBLIC, branche `MHGP8_SANITIZE`), l. 418-462 (6 selftests, 2 portes entières, 1 smoke d'index à 8 000 avec `--repeat 1` et 0,01 s, qui **n'est pas une mesure**, 10 portes Python normal/−O), l. 561-570 (4 scripts de mutation `slow`). Seules les sondes float32 lient `mhgp8_f32`. Au HEAD `12294241`, dans le build de l'orchestrateur, les 23 tests au label `float32` passent, 4 mutations comprises (121,86 s·proc). Les 4 mutations repassent dans un second build (87,81 s·proc). Ce sont des journaux, **pas un reçu** |
| Concordance HEAD / sources qualifiées | **vérifié** | sha256 des 14 fichiers au HEAD = instantanés des reçus census et identité (ex. `float32_q3_census.cpp` `a5edef038546…`, `fixed_signed.hpp` `cd3facd9e93d…`) |
| Sanitizers | **testé (reçus)** | Clang ASan/UBSan(/LSan) pour chaque lot ; aucun passage `MHGP8_SANITIZE` connu sur la cible CMake |
| Concurrence | **testée fonctionnellement, non qualifiée** | lectures concurrentes (128, 64 et 32 appels) sous ASan ; **aucune porte TSan** ; ni construction ni traversée parallèles |
| Front WSPD natif, filtres d'arête et de rectangle, propriété dans X, sortie globale q3 | **proposé (brouillon non commis)** | fichiers `??` du worktree partagé, jamais compilés dans un reçu. État de liaison **non établi** : l'audit de reprise dit « non liable (lane possédée sans définition), une sonde hors `-Werror` » (`AUDIT_REPRISE…:66`), mais `src/lanes/float32_q3_owned.cpp`, daté de 20:22, définit `run_float32_q3_owned_edge` |
| Voie q4 native (génération, census) et voie q2 native (génération) | **manquant** | n'existent que le support q4, la clé et le comparateur de racines ; le prédicat q2 est isolé |
| Catalogue dédupliqué, intérieurs, fold, tour K=1..10 | **manquant** | aucun ordre de clé ; `CENSUS_Q3_FLOAT32_PARTAGE_20260921.md:219-221` |
| Taux de repli exact sur LiDAR réel | **manquant** | tous les corpus sont adversariaux ou synthétiques (`BOULES…:151-153`, `receipts/float32_ball_20260921/README.md:73-77`) |
| GPU | **manquant** | aucun kernel ; les objets à tableaux fixes « ne constituent pas un port GPU » (`PRECISION…:105-106`) |
| Registre des preuves | **manquant** | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne contient aucune entrée float32 (grep vide) |

## 4. Chiffres clés

| Grandeur | Valeur | Source épinglée | Réserve |
|---|---|---|---|
| Construction de l'index, uniforme 8k/16k/32k (médiane CPU sur 3) | 7,071 / 15,355 / 33,173 ms | `receipts/float32_index_20260921/README.md:71-75`, clôture `7419b09f…` | mono, hôte partagé |
| Idem, terrain / amas à 32k | 33,259 / 33,433 ms | idem | idem |
| Comparaisons de tri par doublement | ×2,113 à ×2,214 ; exposants LiDAR 1,093 à 1,125 | idem l. 77, 96 | la preuve O(n log n) ne dépend pas de ces ratios |
| Index sur trame brute entière 0/100/200 | 126,906 / 129,596 / 95,983 ms pour 123 389 / 124 479 / 125 526 sites | idem l. 83-93 | index seul, mono. **Deux trames sur trois** dépassent 100 ms ; la plus grande est la plus rapide, écart non expliqué, et le README interdit de comparer deux scènes |
| Index sur quart de trame (30 265 à 31 391 sites) | 26,897 à 27,737 ms | idem l. 88-91 | seule taille LiDAR proche de 30k mesurée ; aucun nuage sans sol float32 indexé |
| Mémoire de l'index | `156n − 64` octets conservés, pic `180n − 64` ; 19,25 / 19,42 / 19,58 Mo | idem l. 101-104 | points 12 octets ; nœud de 64 octets (5 `size_t` + boîte), 2n−1 nœuds, soit ≈ 128 octets par site |
| Requêtes q2 contre Fraction | 3 923 (916 intérieurs, 1 208 contacts, 1 799 extérieurs) ; 2 715 filtrées, 1 208 replis | `receipts/float32_precision_20260921/README.md:35-39` | les replis sont exactement les contacts, que le filtre ne peut pas certifier ; pas un taux LiDAR |
| Cas de supports q3 / q4 | 294 + 155 refus / 731 + 456 refus (1 636 au total) | `receipts/float32_ball_20260921/README.md:39-47` | cas répétés, pas des boules distinctes |
| Taille d'un support préparé / d'une famille d'événements | 128 / 184 octets | `BOULES…:41`, `IDENTITE…:99` | temporaires exacts non comptés |
| Capacité exacte | 54 mots u32 = 1 728 bits ; bornes q3/q4 < 2^1677 ; déterminant réduit < 2^1397 | `fixed_signed.hpp:23-34`, `float32_q4_events.cpp:161` | le produit naïf de degré 9, ≈ 2^2511, serait hors capacité (B l. 679) |
| Taille d'un `FixedSigned` | 232 octets (54 × 4 + `size_t` + `bool`, aligné sur 8) | déduit de `fixed_signed.hpp:318-320` | calcul de cette lentille, non publié |
| Clés : 229 cas, encodages de 10 à 201 mots | 40 à 804 octets, 2 891 mots au total | `receipts/float32_identity_20260921/README.md:64-68` | corpus extrême ; les 10 boules tri-arités sont une seule configuration |
| Événements q4 | 960 cas : 41 seeds invalides, 398 coplanaires, 161 / 202 / 158 ordres −1 / 0 / +1 | idem l. 70-72 | aucune mantisse aléatoire (B) |
| Census, portes | 18 fixtures, 612 Fraction, 1 798 émissions, 8 610 IDs, 37 refus, 16 corruptions, 458 contrôles natifs | `receipts/float32_q3_census_20260921/README.md:29-35` | une arête par appel |
| Census K10 / 32k, `column` : visites Individual vs Shared1 | 640 422 vs 601 | `ANALYSE_CROISSANCE.md:60-61` | sortie constante (9 supports) : le régime le plus favorable au partage |
| Census K10 / 32k, `column` : temps du census seul | 1 501,299 ms vs 1,344 ms (+ index 13,486 / 13,190 ms) | `ANALYSE_CROISSANCE.md:87-88`, `CENSUS…:200-204` | une observation, hôte chargé : 13 chronos sur 36 chevauchent la capture de mutations ; B mesure un bruit de ×1,35 à ×1,77 sur un travail d'index identique (l. 880-881) |
| Coût moyen par visite géométrique (dérivé) | ≈ 2,34 µs (1 501,299 ms / 640 422) ; ≈ 2,24 µs en Shared1 | calcul sur le reçu ci-dessus | dérivé et non isolé, toutes opérations confondues |
| Coût moyen par évaluation de puissance d'intervalle (dérivé) | ≈ 153 ns (1 501,299 ms / (544 369 requêtes × 18)) ; ≈ 141 ns en Shared1 | `ANALYSE_CROISSANCE.md:40-41,60-61,87-88` | unité plus pertinente que la visite : 6 paraboles et 18 évaluations de puissance par requête de bloc |
| « Budget ≈ 10 ns par visite » | ≈ 10 ns par **visite de témoin du front v7** (4,95·10⁹ visites à 50k, 48 s séquentiels) | `audits/BUDGET_CONTRAT_50K_20260914.md:28` | budget dérivé du volume v7 ; note déclarée historique, « les chiffres ci-dessous ne valent pas pour les sources actuelles » (l. 8). La comparaison avec la visite du census float32 **n'est pas probante** |
| Croissance maximale par doublement (24 doublements, 122 postes) | ×2,116563 visites ; ×2,000250 préparations ; ×2,141703 requêtes ; ×2,217511 tri | `ANALYSE_CROISSANCE.md:36-46`, recalcul de B | familles synthétiques, une arête |
| Repli hull de l'enveloppe (fixtures Release) | 843 / 1 642 préparations (51 %), 85 intersections vides | `DIALOGUE_AUDITEUR_B.md` l. 853-856 | petites fixtures, pas des trames |
| Largeur d'enveloppe, constructeur face à A resserrée | min 1,25, médiane 4,75, max 155 (297 boîtes) | `DIALOGUE_AUDITEUR_B.md` l. 766-767 | face à A universelle : 0,29 / 1,92 / 72 |
| Mutants compilés tués dans la voie | 10 (index 3, boules 3, identité 2, census 2) | READMEs des reçus | aucun sur `float32_q3_block.cpp`, le prédicat q2, le PGCD ou la division |
| CTests float32 au HEAD | 23 (19 portes + 4 scripts de mutation), 121,86 s·proc ; mutations seules, second build : 87,81 s·proc | journaux de l'orchestrateur `head_ctest.log`, `mut_ctest.log` | **non épinglé** |
| Clôtures sha256 | census `97d12e38…` / `f530d919…` / matrice `706d96cb…` / mutations `4520d1e6…` ; index `7419b09f…` / `da79e4a9…` / `572fcb3e…` ; boules `748aaae3…` / `14f71201…` / `07c97cb2…` ; identité `94a76a4f…` / `c8af6062…` / `077df93f…` ; précision `09eacb2c…` / `8be076e9…` | recalculées = valeurs publiées | lecteurs LIVE |
| Volume des reçus float32 | 185 Mo (précision 87, identité 50, boules 19, index 15, census 14) | `du -sm` | — |
| Charges utiles KITTI versionnées sous `receipts/` | `.f32le` : 63 fichiers, 29,8 Mio (précision 42, sol 21). `.u32le` : 307 fichiers, 70,3 Mio (précision 48,4, sol 16,3, `lidar_spatial` 5,5, `q34_spatial` ≈ 0). S'y ajoutent `.u16le` 6,2 Mio (`lidar_spatial`) et `.u8` 2,5 Mio (sol) | `find`/`awk` de la contre-vérification | ≈ 109 Mio au total, cohérent avec les « 111 Mo » de `AUDIT_REPRISE…:105` ; tout n'est pas sous les reçus float32 |
| Builds float32 épinglés hors dépôt | 18 répertoires `build/v8_float32_*` ; 46,9 Mo apparents, 55 Mio sur disque | `du` | lecteurs LIVE : pas de rejeu sans eux |
| Brouillon global non commis | 10 sources (1 330 lignes) + 3 tests (1 120 lignes) = 2 450 lignes | `wc -l` dans le worktree partagé | aucun reçu |
| « ×23 à ×70 par prédicat contre l'u16 », « clé 78 µs », « 68 à 185 ms par arête sans sol » | — | `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:65`, message de `8c050a33` | **non vérifiable** : aucun reçu ni banc ; l'audit déclare lui-même « aucune mesure n'est épinglée » (l. 231-233) ; incohérence interne « ×20 à ×70 » (l. 92) |

## 5. Défauts, risques et dettes

| Gravité | Constat | Preuve |
|---|---|---|
| **Haute** | **Contrat de précision ambigu, y compris dans la passation.** Au HEAD, quatre documents posent « float32 original par défaut » : `AGENTS.md:14-19` (qui **ne mentionne nulle part** u18 ni 18 bits), `morsehgp3D_v8/README.md:1,26-31`, `audits/ETAT_COURANT.md:21-24` et `docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:35-38`. `PASSATION.md` se contredit : l. 36-39 « entier 18 bits, aucun développement float32 », mais l. 1 (titre « précision float32 ») et l. 54-55 « La précision cible est désormais float32 original sans perte ». `ELARGISSEMENT_18_BITS_20260922.md:1-6` et `JOURNAL…:104-107` placent le contrat temps sur u18. La tranche non commise du constructeur réaffirme « float32 par défaut » et une grille « OPTIONNELLE » : bloc ajouté à `AGENTS.md`, non indexé, et `REPRISE_U18…:19-21`, indexé (« une priorité technique… ne vaut pas modification de ces contrats »). Un contrat mesuré sur u18 pourrait donc ne pas être le contrat déclaré | lignes citées |
| **Haute si la voie est réactivée** (gelée aujourd'hui) | **Coût constant élevé.** Chaque opération d'intervalle appelle `std::nextafter` : 2 appels par différence, 8 par produit, 2 par addition (`float32_predicates.hpp:161-190`). Une requête de bloc compte 6 paraboles et 18 évaluations de puissance. Le reçu census donne ≈ 2,3 µs par visite, soit ≈ 150 ns par évaluation de puissance, sur un hôte chargé. La comparaison avec un budget de « 10 ns par visite » n'est pas probante (budget historique propre au volume v7). Le brouillon global énumère paire par paire chaque rectangle **résiduel** et relance pour chaque arête une traversée complète de l'index (`src/pipeline/float32_q3_global.cpp:10-41,76-89`, non commis). Il existe un rejet de rectangle optionnel par K−1 témoins proposés près du milieu (`src/wspd/float32_front.cpp:38-75,122-126`, non commis), jamais mesuré. Sans ce filtre, Σ\|A\|\|B\| = n(n−1)/2 paires | code cité ; `ANALYSE_CROISSANCE.md:40-41,87` |
| **Haute** | **Reçus non autonomes.** Les lecteurs sont LIVE : ils exigent les 18 builds non versionnés sous `build/` (`receipts/float32_ball_20260921/README.md:33-35`, `float32_identity…:33-34`, `float32_q3_census…:49`) et reconstruisent leurs entrées depuis les scans KITTI bruts de `morsehgp3D_v8/audits/lidar08_20260914/data/`, exclu par `.gitignore` et référencé par les manifestes index et précision. Le lecteur du lot boules **refuse déjà** au HEAD, parce que l'en-tête entier a été étendu par `9923a6b9` (`float32_identity_20260921/README.md:36-38`) | READMEs et manifestes cités |
| Moyenne | **Enveloppe de bloc lâche et non mutée.** Repli hull à 51 % sur les fixtures ; F2/F3 restent indécises là où l'enveloppe resserrée de A décide ; aucun mutant compilé sur `float32_q3_block.cpp` ; les planchers de couverture n'incluent ni `gram_unresolved` ni `center_intersection_fallbacks` (`tests/float32_q3_census_gate.py:314-318`) | B `f7b220c4` ; code cité |
| Moyenne | **Fixtures F1–F10 non raccordées.** Aucune porte commise ne les charge ; seule la porte du brouillon non commis (`tests/float32_q3_global_test.py:108-137`) charge F1–F7, F9 et F10, sans F8 | grep vide sur `tests/`, `bench/`, `src/`, `CMakeLists.txt` au HEAD |
| Moyenne | **Clé non prête pour un catalogue.** `Float32BallKey` n'a que `operator==` ; affectations supprimées, déplacement par copie, un `std::vector` par clé ; PGCD binaire par soustractions et division bit à bit (décalage complet à chaque bit) sur 1 728 bits (`fixed_signed.hpp:150-200`). Un ordre externe sur `serialized_words()` est possible, mais n'est ni écrit ni mesuré. Coût d'émission inconnu : les 78 µs sont non vérifiables | `float32_ball_key.hpp:51-56` |
| Moyenne | **Deux arithmétiques exactes coexistent** : l'accumulateur q2 à 18 mots en unité 2^-298 et `FixedSigned` à 54 mots en unité 2^-149. Le repli recalcule tous les coefficients à pleine largeur : plusieurs Ko de pile, 232 octets par entier | `float32_predicates.hpp:76-117` ; B l. 716-721 |
| Moyenne | **Unité 2^-149 coûteuse sur LiDAR** : une coordonnée de l'ordre du mètre porte environ 126 bits nuls de poids faible. Le produit d'école parcourt tous les mots actifs, y compris ces mots bas nuls (`fixed_signed.hpp:229-250`). La factorisation des puissances de deux communes est seulement évoquée (`INDEX…:201-202`) | déduction de cette lentille, non mesurée |
| Moyenne | **Taux de repli inconnu sur données réelles.** Aucun corpus à mantisses aléatoires. Pour les **comparaisons d'événements q4**, B observe 0 repli sur 182 tirages aux exposants LiDAR et 14 sur 70 sur toute la plage (non épinglé) | `DIALOGUE_AUDITEUR_B.md` l. 692-697 |
| Moyenne | **Brouillon non suivi dans l'index Git partagé** (2 450 lignes, `??`) : risque d'inclusion accidentelle (`git add -A`) ou de perte ; sort non tranché (Q1 de `AUDIT_REPRISE…:217-220` ; `PASSATION.md:29-30`) ; état de compilation inconnu | `git status --porcelain` du worktree |
| Moyenne | **Licences** : ≈ 109 Mio de dérivés KITTI (CC BY-NC-SA) versionnés sous `receipts/` (`float32_precision`, `lidar_ground`, `lidar_spatial`, `q34_spatial`) ; Q3 de la reprise non tranchée au HEAD | `find` ; `AUDIT_REPRISE…:104-106,224-227` |
| Moyenne | **Aucun parallélisme ni TSan** : construction mono, census mono, tests concurrents seulement fonctionnels | tous les READMEs de reçus |
| Basse | **Documentation décalée** : `AUDIT_REPRISE…:65` dit « hors CMake et CTest » alors que `92d74c13`, le même soir, enregistre 23 CTests ; « ×23 » (l. 65) contre « ×20 » (l. 92) ; titre de `README.md:1` « entrée float32 » | lignes citées |
| Basse | **Provenance des reçus** : pour **les cinq lots**, `git_commit` désigne le HEAD de lancement antérieur au code testé (précision `7e7c6cc4`, index `36724438`, boules `028a0f1d`, identité `12d885d8`, census `e2b09f94` pour Release/Sanitize et `74fb0a6a` pour la matrice ; champ absent des captures de mutations). L'identité du contenu repose sur les instantanés hachés (B l. 666-670, 888-891) | `MANIFEST.json` des reçus |
| Basse | **Textes de reçus retouchés a posteriori** : `204b0620` ajoute des paragraphes au README et à `ANALYSE_CROISSANCE.md` du reçu census. Les données closes (`COMPLETION.json`) ne changent pas, mais la doctrine des reçus immuables demanderait un addendum séparé | `git show 204b0620` |
| Basse | **Sortie de matrice non représentative** : `column`/`slab` ont K−1 supports constants ; aucun régime à sorties croissantes, aucun rejet extérieur partagé à l'échelle | B `f7b220c4` ; `ANALYSE_CROISSANCE.md:16-18` |
| Clos | Risque « discipline flottante absente de CMake » (B, `74fb0a6a`) : fermé par `92d74c13` (options PUBLIC sur `mhgp8_f32`, liée par les seules sondes float32) ; `float32_predicates.hpp:11-13` refuse `__FAST_MATH__` | `CMakeLists.txt:52-70,423-430` |

Cette lentille n'a trouvé aucun défaut de correction dans le code commis. Ont été relus :

- les formules q3 (`F>0`, `D−F>0`, `E−F>0`, `G>0` ; `W = E(D−F)d + D(E−F)u`) ;
- les formules q4 (poids `N·cofacteur_i / 2det²` testés avant orientation ; puissance `sign(det·|v|² − N·v)·sign(det)`) ;
- l'expansion globale `A=S, B=−2Sa−L, C=S|a|²+L·a` et l'expansion de Laplace 4×4 ;
- le compte partagé : ticket `(X, compte, curseur)` copié, feuille ambiguë scindée avant consommation, coquille globale ;
- les bornes par paraboles.

La contre-vérification a refait des contrôles ponctuels :

- bornes de l'accumulateur q2 et enclosure `nextafter` ;
- contrôles de capacité, PGCD et division exacte de `FixedSigned` ;
- préparation du bloc (`float32_q3_block.cpp:85-120`).

Ces contrôles concordent avec ce constat et avec les relectures de B (`74fb0a6a`, `f7b220c4`). Ce n'est pas une relecture intégrale indépendante.

## 6. Questions ouvertes

1. **Quel profil lie le contrat temps de la v9** : float32 sans perte par défaut, comme le disent `AGENTS.md` au HEAD, `PASSATION.md:54-55` et la tranche non commise ? Ou grille entière 1 mm u18, comme le disent `ELARGISSEMENT_18_BITS_20260922.md` et `PASSATION.md:36-39` ? Si c'est u18, la grille devient-elle l'entrée déclarée du contrat, ou reste-t-elle une option de diagnostic ?
2. **Sort du brouillon global non commis** : commit par son auteur sous un statut explicite de brouillon non qualifié dans la v8, ou retrait ? Aucune décision au HEAD (`PASSATION.md:29-30` : « sort à décider par l'utilisateur »). Est-il seulement liable ? L'audit de reprise dit non, mais le fichier de définition existe.
3. **Équivalence grille 1 mm / float32** : les trois trames n'ont aucune fusion à 1 mm (`receipts/float32_precision_20260921/README.md:50-54`), mais aucun calcul HGP apparié ne dit si les contacts, les profondeurs ou la tour changent. La question reste sans objet tant qu'aucun générateur float32 n'existe.
4. **Coût réel d'un prédicat float32 face au moteur entier** : le « ×23 à ×70 » est-il reproductible sur un banc épinglé (mêmes requêtes, même hôte) ?
5. **Taux de repli exact sur LiDAR** : mantisses et exposants réels, sur les nuages sans sol float32 déjà préparés (39 885 / 35 551 / 45 845 sites, `receipts/lidar_ground_20260921/README.md:49-51`).
6. **Données KITTI versionnées** (≈ 109 Mio) : sortir les charges utiles (hachage + recette de régénération) ou documenter l'exception non commerciale ? Et comment rendre rejouables des reçus dont les lecteurs exigent les scans bruts non versionnés ?
7. **Pourquoi la trame 200** (125 526 sites) indexe-t-elle en 95,98 ms contre 126,9 et 129,6 ms pour des trames plus petites ?
8. La v9 doit-elle **copier** la voie float32 ou seulement la **référencer par épinglage** dans la v8 ?

## 7. À porter en v9 et à ne pas reprendre

### À porter (dormant, qualifié, sans développement)

| Quoi | Où (v8) | Pin | Pourquoi |
|---|---|---|---|
| Voie float32 complète, à l'identique : prédicat q2, `FixedSigned`, supports, clé, événements q4, bloc q3, census d'une arête, index | `morsehgp3D_v8/src/core/float32_*`, `fixed_signed.hpp`, `src/lanes/float32_q3_census.*`, `src/spatial/float32_index.*` | `a005f8aa` (= HEAD `12294241`, sha256 identiques aux instantanés des reçus census et identité) | seule implémentation exacte sans perte ; qualifiée par reçus et 23 CTests ; à **référencer** dans la provenance v9, pas à recompiler dans le chemin actif |
| Portes, sondes et mutations float32 ; cible `mhgp8_f32` avec options PUBLIC | `tests/float32_*`, `tests/fixed_signed*_gate.cpp`, `CMakeLists.txt:52-70,416-462,557-570` | `92d74c13` | requalification immédiate si la voie est réactivée |
| Reçus float32, liste des 18 builds épinglés et dépendance aux scans bruts | `receipts/float32_*_20260921/` | clôtures sha256 du § 4 | preuves LIVE. Le lot boules n'est plus rejouable au HEAD ; celui de l'identité le remplace pour le code courant |
| Fixtures F1–F10 de B | `morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/` | `21b85af2`, `ad9bbc9d` | géométrie générale, valable aussi pour le moteur entier : sommet intérieur (F1), témoins mutuels (F6), population des témoins (F9), égalité de propriété (F10) |
| Mathématiques de la voie | `BOULES…:64-105`, `IDENTITE…:24-100`, `RACCORD…:52,88-126`, preuve `GΔ` de B (l. 681-691) | commits cités | bornes de degré ; clé primitive inter-arités ; `c = m + ξ·h(x)` avec `0 < ξ < 1/2` (acuité) et `ξ ≤ 1/3` (propriété d'arête) ; citron `H>0 ∧ α_q H² > Ξ` |
| Préparateur float32 / grille décimale exacte | `bench/prepare_lidar_precision.py` | `36724438` | produit aussi les entrées u32 de la grille 1 mm consommées par le moteur u18 |
| Doctrine du filtre statique v7 | `morsehgp3D_v7/src/pipeline/float_filter.hpp:1-80` | v7 | borne d'erreur calculée une fois ; filtre coupé sous `__FAST_MATH__` à la compilation et hors `FE_TONEAREST` à l'exécution (l. 27-28, 70-77). Modèle moins coûteux que des intervalles `nextafter` à chaque opération, pour les filtres du moteur entier comme pour une future voie float32 |

### À ne pas reprendre (fausses pistes et mesure de fermeture)

| Piste | Mesure ou preuve qui la ferme |
|---|---|
| Brouillon global non commis, tel quel | jamais compilé dans un reçu. Son seul préflight (FAILED, Boost absent) porte sur une version abandonnée de la sonde. Il énumère paire par paire chaque rectangle résiduel avec une traversée globale par arête ; son rejet de rectangle par échantillons n'a jamais été mesuré ; q3 seul, mono |
| Coupe au milieu géométrique pour l'index float32 | 278 points dyadiques : 277 niveaux, contre 9 au rang médian (`INDEX…:43-60`, `FAUSSES_PISTES.md:52-60`) |
| Réduire le pas u16 ou élargir `Point3` implicitement | collision `(0,1,0)`/`(0,0,65536)` ; `200000²` tronqué en `1345294336`, signe q2 inversé (`PRECISION…:62-70`) ; remplacé par le port explicite u18 `a74e90f2` |
| Grille provisoire 2,5 mm | jamais qualifiée, abandonnée (`FAUSSES_PISTES.md:62-71`) |
| Comparer les racines q4 par produits croisés de degré 9 | ≈ 2^2511, hors capacité ; le déterminant réduit de degré 5 suffit (B l. 677-679) |
| Orienter N avant de tester les quatre poids q4 | inverse trois poids si det < 0 (`FAUSSES_PISTES.md:42-50`). Écarté par construction et par les fixtures de signe ; le mutant compilé voisin tué est « signe du déterminant omis dans la puissance », **pas** cette erreur précise |
| Repartir de la racine avec le crédit hérité ; coquille au curseur | mutants compilés tués : profondeur 2 → 3, perte de a et b (`CENSUS…:184-187`) |
| Bornes de puissance aux seuls coins | F1 : coins [0, 0], vraie plage [−25, 0] |
| λ ≤ 2/3 sans propriété d'arête | F4 : λ = 24/25, 5304/5305, 1 − 10⁻⁸ |
| Restreindre Z aux graines propriétaires ou valides | F9 : deux témoins stricts (−67/8, −231/8) perdus |
| Identifier une boule par son support, son arité ou son seul rayon | `FAUSSES_PISTES.md:25-40` ; corpus de clés (une configuration tri-arité sous dix similitudes) |
| Transférer les chronos 20 mm au float32 ou au 1 mm | interdit par toutes les notes float32 |
| Citer « ×23–×70 », « 78 µs » ou « 68–185 ms par arête » comme mesures | aucun reçu ; la source se déclare non épinglée (`AUDIT_REPRISE…:231-233`) |

## 8. Recommandations priorisées pour la v9

1. **P0 — Obtenir un arbitrage utilisateur écrit** sur le profil qui lie le contrat temps (float32 par défaut ou u18 1 mm). Le reporter à l'identique dans `AGENTS.md` (qui ignore aujourd'hui u18), le README v9, la passation (qui se contredit aujourd'hui) et le contrat de trames. Sans cela, toute mesure v9 peut être déclarée hors contrat.
2. **P0 — Voie float32 dormante par épinglage** : la v9 ne copie pas `src/*float32*` dans son chemin actif. Sa provenance cite la liste des 14 fichiers et leurs sha256 au pin `a005f8aa`/`12294241`, les clôtures des reçus (en précisant que le lot identité couvre le code de boules courant) et les 23 CTests. Aucun développement tant que l'utilisateur n'a pas levé la consigne du 22 septembre.
3. **P0 — Faire trancher le brouillon non commis par son auteur** (commit sous statut « brouillon non qualifié », ou retrait) avant toute écriture v9 dans le même index Git. Ne jamais l'importer en v9.
4. **P1 — Graver F1–F10 dans les portes v9** de toute borne de bloc ou de census du moteur entier u18 : leur valeur est géométrique, pas propre au float32.
5. **P1 — Ne plus citer « ×23 à ×70 », « 78 µs » ni « 68 à 185 ms par arête »** sans banc épinglé. Si la question du coût revient, mesurer sur un même lot de requêtes u18 et float32, sur le même hôte, avec les répétitions publiées. Rapporter le coût par évaluation de puissance, pas par visite.
6. **P1 — Données et builds** : sortir les ≈ 109 Mio de charges utiles KITTI des reçus versionnés (hachage + recette), ou documenter l'exception. Déclarer en v9 que les reçus float32 exigent les 18 builds `build/v8_float32_*` et les scans bruts non versionnés, et qu'ils ne sont pas autonomes. Faire porter à tout reçu v9 le commit du **contenu** testé, pas le HEAD de lancement, et les commentaires ultérieurs dans un addendum.
7. **P2 — Raccord futur sans dette** (seulement si l'utilisateur rouvre la voie), dans cet ordre :
   (a) remplacer l'enveloppe `hull ∩ (a + W/(2G))` par `m + ξ·h(X)`, avec un repli fini `m + [0, 1/2]·h(X)` (acuité seule) ou `[0, 1/3]` (propriété) ; ajouter F1–F10, des mutants compilés sur `float32_q3_block.cpp` (sommet → extrémité, intersection omise, `gram.low < 0`, dénominateur G) et des planchers sur `gram_unresolved` et les replis ;
   (b) certificats au niveau du rectangle, **mesurés**, avant toute énumération d'arêtes ; jamais « paires × traversée globale » ;
   (c) filtre statique à borne d'erreur prouvée sous `FE_TONEAREST` (doctrine v7), sur une unité locale (puissance de deux commune du nuage), les intervalles `nextafter` restant le repli sûr ; mesurer le gain et le taux de repli sur les nuages sans sol float32 déjà préparés ;
   (d) une seule arithmétique exacte, factorisation exacte des puissances de deux, tampons de repli par worker ;
   (e) ordre lexicographique total sur `serialized_words()`, stockage en arène, puis catalogue (tri, RLE) ; voie q4 native indépendante ; voie q2 native ;
   (f) ordonnanceur multi-CPU avec porte TSan et sorties bit-identiques W1/W2/W4/W8, avant tout port GPU (le `FixedSigned` de 232 octets et les piles de plusieurs Ko sont à revoir pour un device).
8. **P2 — Registre des preuves** : si la voie est rouverte, inscrire dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, qui n'en contient aucun aujourd'hui, l'identité `GΔ`, l'enveloppe `ξ ≤ 1/3` sous propriété et la clé primitive, avec leurs fixtures d'égalité.

## Contre-vérification

La contre-vérification a été faite en lecture seule sur `v8_head` (`12294241`), sur le worktree partagé (non commis, étiqueté) et sur les journaux de l'orchestrateur. Chaque correction ci-dessous est intégrée plus haut.

| # | Affirmation d'origine | Verdict | Correction et preuve |
|---|---|---|---|
| C1 | La décision du 22/09 est portée par `93ba5bb4` et `a74e90f2` | corrigé | Seul `a74e90f2` (06:21) introduit ce texte (`git log -S` sur PASSATION, ELARGISSEMENT et JOURNAL) ; `93ba5bb4` ne contient que le journal des phases 1-2 |
| C2 | « PASSATION et ELARGISSEMENT disent u18 » | corrigé | `PASSATION.md` se contredit : l. 36-39 u18, mais l. 1 et l. 54-55 « La précision cible est désormais float32 original sans perte ». `AGENTS.md` au HEAD ne mentionne nulle part u18 ni 18 bits |
| C3 | L'exposant du préflight FAILED qualifie le brouillon | corrigé | Le préflight (`-fsyntax-only`, 20:21) porte sur une première version de `float32_front_probe.cpp` qui incluait Boost. Les sondes actuelles n'incluent plus Boost. L'état de compilation du brouillon est inconnu. `AUDIT_REPRISE…:66` le dit non liable, alors que `float32_q3_owned.cpp` (20:22) définit la voie |
| C4 | « sans certificat au niveau du rectangle, n(n−1)/2 paires » | corrigé | Le brouillon a un rejet de rectangle optionnel `MidpointSamples` : K−1 témoins proposés près du milieu, décision exacte par citron (`src/wspd/float32_front.cpp:38-75,122-126`, non commis). Seuls les rectangles résiduels sont énumérés paire par paire. n(n−1)/2 ne vaut que filtre désactivé |
| C5 | 10 sources du brouillon = 1 430 lignes | corrigé | 1 330 lignes (sources), plus 1 120 (tests) = 2 450 |
| C6 | Écart ≈ ×230 contre un budget de 10 ns par visite | corrigé | Les 10 ns sont un budget par **visite de témoin du front v7** (4,95·10⁹ visites), dans une note déclarée historique et non valable pour les sources actuelles (`BUDGET_CONTRAT_50K_20260914.md:8,28`) : la comparaison n'est pas probante. Unité ajoutée : ≈ 153 ns par évaluation de puissance (≈ 141 ns en Shared1). Bruit B de ×1,35 à ×1,77 |
| C7 | « 18 évaluations de paraboles par requête » | corrigé | 6 paraboles (`axis_parabolas`) et 18 évaluations de puissance (`power_evaluations`) par requête (B l. 882-884 ; 4 322 376 / 240 132 = 18) |
| C8 | Index « supérieur à 100 ms sur trame brute » | corrigé | 2 trames sur 3 ; la trame 200, pourtant la plus grande, prend 95,983 ms (`receipts/float32_index_20260921/README.md:92-93`) |
| C9 | Clôtures du lot boules comme preuve du code de boules au HEAD | corrigé | `fixed_signed.hpp` et `float32_ball.*` ont changé dans `9923a6b9`. Le lecteur LIVE du lot boules refuse l'en-tête étendu (`float32_identity_20260921/README.md:36-38`). Le code courant est couvert par la régression du lot identité |
| C10 | « Clé : catalogue impossible en l'état » | corrigé | `serialized_words()` (encodage canonique) est public : ordre et hachage externes possibles. Ce qui manque : ordre intégré, affectation (supprimée), déplacement sans copie |
| C11 | « Orienter N avant les poids : mutant tué » | corrigé | Le mutant tué est « signe du déterminant omis dans la puissance » (`float32_ball_20260921/mutations/README.md`) ; aucun mutant ne reproduit l'erreur d'ordre orientation/poids |
| C12 | `git_commit` = HEAD de lancement pour `e2b09f94`, `74fb0a6a`, `12d885d8` | corrigé | Décalage systématique sur les cinq lots : `7e7c6cc4` (précision), `36724438` (index), `028a0f1d` (boules), `12d885d8` (identité), `e2b09f94`/`74fb0a6a` (census) ; champ absent des captures de mutations |
| C13 | 30 Mo `.f32le` + 71 Mo `.u32le` sous précision + sol | corrigé | `.f32le` : 29,8 Mio (précision + sol), exact. `.u32le` : 70,3 Mio, dont 5,5 Mio sous `lidar_spatial`. S'y ajoutent `.u16le` 6,2 Mio et `.u8` 2,5 Mio. Total ≈ 109 Mio, cohérent avec les 111 Mo de la reprise |
| C14 | « 10 boules tri-arités du corpus » comme preuve | corrigé | une seule configuration sous dix similitudes (B l. 671-674) |
| C15 | FTZ/DAZ non exercé pour q2 | confirmé, nuancé | non exercé expérimentalement ; B argumente la tenue du filtre sous FTZ/DAZ (l. 710-711) |
| C16 | Chiffres « ×23–×70 », « 78 µs », « 68–185 ms » | confirmé non vérifiable | aussi dans le message de `8c050a33` ; la source déclare « aucune mesure n'est épinglée » (l. 231-233) |
| C17 | Reçus LIVE dépendant des 18 builds (46 Mo) | confirmé, complété | 46,9 Mo apparents (55 Mio sur disque) ; les lecteurs index et précision exigent aussi les scans KITTI bruts non versionnés (`audits/lidar08_20260914/data/`, `.gitignore`) |
| C18 | Largeur d'enveloppe médiane 4,75 | confirmé, complété | face à A resserrée ; face à A universelle 0,29 / 1,92 / 72 (le constructeur y est parfois plus serré) |
| C19 | Taux de repli B (0/182, 14/70) | précisé | il concerne les comparaisons d'événements q4, pas tous les prédicats |
| C20 | Omission : textes de reçus modifiés a posteriori | ajouté | `204b0620` modifie le README et `ANALYSE_CROISSANCE.md` du reçu census, hors `COMPLETION.json` |
| C21 | Omission : `STATUT_PREUVES_ET_HEURISTIQUES.md` | ajouté | aucune entrée float32 (grep vide) |
| C22 | Omission : smoke CTest d'index | ajouté | `mhgp8_float32_index_bench_uniform_8000` (`--repeat 1`, 0,01 s) porte le label `gate` : ce n'est pas une mesure |

Confirmés sans changement :

- gel de la voie ;
- identité octet pour octet des 14 sources ;
- 23 CTests verts au HEAD (19 portes + 4 mutations) ;
- capacités 576 et 1 728 bits et bornes < 2^1677 et < 2^1397 ;
- preuve `GΔ` ;
- index O(n log n) et fixture de 278 points ;
- census limité à une arête ;
- 51 % de repli hull et absence de mutant sur le bloc ;
- F1–F10 absentes des portes commises ;
- matrice synthétique à sortie constante ;
- tous les chiffres des reçus : index, précision, boules, identité, census, croissance ;
- toutes les clôtures sha256 ;
- volume de 185 Mo ;
- 18 builds ;
- 2 450 lignes du brouillon ;
- absence de défaut de correction par lecture (concordance avec B, sans relecture intégrale indépendante).
