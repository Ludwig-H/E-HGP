## Audit v8, rapport 16 : carte du code `morsehgp3D_v8/src` fichier par fichier

```text
phase=exploration_v8_hors_registre (audit de clôture v8, ouverture v9)
backend=cpu_reference
profile=quantized_u18_input_only (moteur entier) ; lossless_float32_input_only (voie dormante)
mode=audit_independant_lecture_seule
public_status=not_claimed
GCP non utilisé. Aucune compilation, aucun ctest, aucun script du dépôt, aucune commande git mutante.
```

Source de vérité : worktree détaché `v8_head` = `origin/main` **12294241**
(`git status --porcelain` vide au moment de la lecture). Chemins relatifs à
`morsehgp3D_v8/` sauf mention contraire. Ce rapport comble une lacune relevée
par la critique de complétude : aucune lentille n'avait dressé la carte des
78 sources, et plusieurs passages du chemin mesuré n'avaient été relus par
personne (`q4_local.cpp:520-763`, `edge_cover.cpp`, `q4_positive_domain.cpp`,
`wspd/front.cpp`, index de `q2_census.cpp:97-194`).

Abréviations des sources d'appui (dossier v9 publié à 3595725a) : **H** =
`morsehgp3D_v9/docs/HERITAGE_V7_V8.md` ; **L2, L3, L4, L6** =
`morsehgp3D_v9/docs/audit_v8/02, 03, 04, 06` (versions contre-vérifiées) ;
**FP** = `morsehgp3D_v9/docs/FAUSSES_PISTES.md`.

Statuts : **prouvé** (preuve écrite et fixture), **testé** (porte bornée),
**mesuré** (reçu épinglé), **proposé**, **manquant**, **non vérifiable**
(chiffre sans reçu). Les constats tirés de ma seule lecture du code sont
marqués **déduit (lecture)** : ils valent « proposé » tant qu'aucune fixture
ni aucun compteur ne les grave.

## 1. Méthode

- Inventaire : `git ls-tree -r HEAD morsehgp3D_v8/src` donne 87 entrées, dont
  9 `.gitkeep` (`src/`, `cloud/`, `forest/`, `gpu/`, `io/`, `lanes/`,
  `parallel/`, `tree/`, `wspd/`), soit **78 sources** : 30 `.cpp` et
  48 en-têtes, **17 116 lignes** (`wc -l`, total identique à L6 § 4).
- Empreintes : `sha256sum` de chaque fichier (annexe A, empreintes complètes).
- Graphe d'inclusion : `grep '#include "'` sur les 78 fichiers ; graphe
  d'appel : `grep` des symboles publics dans `src/`, `bench/` et `tests/`.
- Historique : commit de création (`git log --diff-filter=A`) et dernier
  commit par fichier (lecture seule).
- Bibliothèque : `CMakeLists.txt:32-41` (`mhgp8_p0`, 24 `.cpp`) et
  `CMakeLists.txt:59-61` (`mhgp8_f32`, 6 `.cpp`). Un en-tête appartient à la
  bibliothèque dont les sources l'incluent.
- Lu en entier : `pipeline/wspd_q34.{hpp,cpp}`, `lanes/q4_local.{hpp,cpp}`,
  `lanes/q4_seed_cells.hpp`, `lanes/q4_local_partition.{hpp,cpp}`,
  `lanes/q4_positive_domain.{hpp,cpp}`, `lanes/edge_cover.{hpp,cpp}`,
  `wspd/front.{hpp,cpp}`, `lanes/q3_ball_census.cpp`,
  `lanes/q34_pair_bounds.hpp`, `spindle/predicates.hpp`, `core/types.hpp`,
  `parallel/joined_workers.hpp`, `pipeline/q2_census.hpp`,
  `pipeline/prepared_cloud.hpp`. Lu en partie : `q2_census.cpp:1-260`
  (index et début du moteur) et sa table des définitions,
  `prepared_cloud.cpp:40-137`, `exact_ball.cpp:60-155`,
  `work_reduction.hpp:1-70`, `q34_witness_search.cpp:70-95, 150-175`,
  `local_credits.cpp:295-315`, commentaires d'en-tête de tous les autres.
- Non lu ligne à ligne : corps de `q2_census.cpp:260-2852`, des prototypes
  T24-T27 et T29-T30, de `axis_q2.cpp`, `local_credits.cpp`,
  `tube_credits.hpp`, et des 14 fichiers float32 (la lentille 4 les a relus ;
  leurs empreintes sont identiques aux instantanés des reçus, L4 § 3).

### Définition du « chemin mesuré »

| code | chemin | preuve |
| --- | --- | --- |
| **Q34** | `bench/wspd_q34_probe.cpp:199-270` : `prepare_cloud` → `make_q2_cloud_index` → `run_wspd_q34_parallel`, options `samples digest rectangle-pair boxes affine live 64 atlas`, masque 6, backend 28, s = 8, `Q4LocalOptions` par défaut (`Positive, 7, 4096, 512, 32, true`, `lanes/q4_local.hpp:9-18`) ; W1 passe aussi par l'entrée parallèle | mesuré : `receipts/ground_phase1_20260921/only_probe_0*.json` (champs `front_mode` … `q3_atlas_mode`, sonde 0e2c18ca) |
| **Q2** | `run_wspd_q2_census` (série) et `run_wspd_q2_census_parallel` en Coarse, configuration gagnante `{2,16,true}`, Pool64, `ComplementFirst`, frère saturant, `SharedBlocks` | mesuré : `receipts/q2_front_inheritance_20260917` (L2 § 7.1, § 9.2) |
| **Q2−** | entrées q2 mesurées sans gain : `run_wspd_q2_census_cooperative` (T17), `_ranges` (T18), `_batched` (T19), ordonnancement `Donate` (T14) | mesuré, négatif (L2 § 2, FP « Voie q2 ») |
| **lié** | compilé et lié dans le binaire de mesure, jamais exécuté dans la configuration mesurée | déduit (lecture) |
| **types** | en-tête dont seuls des types servent au chemin | déduit (lecture) |
| **inclus sans usage** | en-tête tiré par la chaîne d'inclusion sans qu'aucun de ses types ne serve au chemin | déduit (lecture) |
| **non** | appelé seulement par ses propres portes ou sondes | grep |
| **f32** | bibliothèque `mhgp8_f32`, liée par les seules sondes float32 (`CMakeLists.txt:127-128`) | L4 § 3 |

La sonde de mesure lie les prototypes T24-T27 et T29 sans les exécuter :
`bench/wspd_q34_probe.cpp:4` inclut `q4_lidar_probe.cpp`, qui inclut
`q34_cover_probe.cpp` par `#define main` (`bench/q4_lidar_probe.cpp:5-7`) et
appelle `run_q4_shallow_*`. `q2_census.cpp` référence `AxisQ2Plan`
(`q2_census.cpp:228-234`, `700-726`) : `axis_q2.o` et `local_credits.o` sont
donc liés dans tout binaire qui construit l'index.

### Dispositions

| disposition | sens |
| --- | --- |
| **PORTER** | port explicite, épinglé à 12294241 (ou au commit cité), requalifié en v9 (portes, mutants, reçu) |
| **LEGACY** | reste en v8 comme cible différentielle épinglée ; jamais compilé dans le chemin v9 |
| **RÉFÉRENCER** | voie float32 dormante : citée par pin et empreinte, non compilée dans le chemin actif (H § 6) |
| **ABANDONNER** | prototype ou ordonnanceur fermé par une mesure sans gain ; la preuve éventuelle survit par sa note, pas par le code |

## 2. Résumé

| disposition | fichiers | lignes (fichiers entiers) | lignes après découpage (§ 4) |
| --- | ---: | ---: | ---: |
| PORTER | 36 | 9 887 | ≈ 7 660 |
| LEGACY | 8 | 1 446 | ≈ 1 650 |
| RÉFÉRENCER | 14 | 2 093 | 2 093 |
| ABANDONNER | 20 | 3 690 | ≈ 5 715 |
| **total** | **78** | **17 116** | **17 116** |

Trois fichiers du chemin mesuré sont mixtes et doivent être découpés au port :
`q2_census.cpp` (≈ 1 085 lignes à porter sur 2 852), `wspd/front.cpp`
(≈ 500 sur 762, sans le dispatcher `Donate`) et `q4_local.cpp` (≈ 560 sur
763, sans les parcours `Individual` et `Joined`). Environ 45 % des lignes
de `src/` servent le chemin mesuré ; ce chiffre concorde avec L2 § 7.1
(≈ 3 500 lignes pour q2) et L3 § 4 (≈ 4 443 lignes pour le flux q3/q4).

Constats principaux, détaillés au § 6 :

1. Aucun défaut de correction dans les passages relus pour la première fois
   (`q4_local.cpp:520-763`, `edge_cover.cpp`, `q4_positive_domain.cpp`,
   `wspd/front.cpp`, `q2_census.cpp:97-194`) ; toutes les bornes d'entiers
   à 18 bits recalculées tiennent.
2. `outside_domain` (question ouverte 10 de L3) s'explique par lecture : seul
   un atlas dont la racine est `Outside` faute de deux complétions
   (`q4_local_partition.cpp:218`) peut laisser un centre q3 sans certificat.
   Le compteur vaut donc le nombre de graines q3 portées par une arête dont la
   lentille ne contient qu'un site, ce qui est indépendant de K dès K ≥ 4.
   Le défaut « localisation Outside perdue » de L3 § 5.17 est vide.
3. L'arbre de plages de `PreparedCloud`, que H § 2 demande de porter, ne sert
   qu'au prototype T1 abandonné (`local_credits.cpp:307-308`) et occupe 80 %
   de la mémoire du nuage.
4. H § 2 omet quatre groupes de sources du chemin mesuré : `edge_cover.*`,
   `q4_positive_domain.*`, `q4_seed_cells.hpp` et `q34_seed.hpp`. L'énumération
   `Q4CenterDomainMode`, dont l'atlas a besoin, est définie dans un en-tête du
   prototype T27.

## 3. Carte fichier par fichier

Colonnes : lignes, sha256 (12 premiers caractères, empreintes complètes en
annexe A), module (répertoire, tranche, commit de création), rôle, chemin
mesuré (codes du § 1), bibliothèque, disposition, justification.

### 3.1 `src/core`

| fichier | l. | sha256 | module | rôle | chemin | bibl. | disposition | justification |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| `core/fixed_signed.hpp` | 323 | `cd3facd9e93d` | core, float32 (12d885d8) | entier signé fixe de 54 mots (1 728 bits), PGCD, division exacte | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Voie float32 dormante (H § 6) ; empreinte égale aux instantanés des reçus identité et census (L4 § 3). |
| `core/float32_ball.cpp` | 222 | `3c89462ee22b` | core, float32 (12d885d8) | supports q3/q4 float32, validité stricte, puissance exacte | f32 | `mhgp8_f32` | RÉFÉRENCER | Code de 9923a6b9, couvert par le lot identité et non par les clôtures du lot boules (L4 § 3). |
| `core/float32_ball.hpp` | 73 | `c673372c58e5` | core, float32 (12d885d8) | interface `Float32Ball` | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |
| `core/float32_ball_key.cpp` | 94 | `58b5f98ea921` | core, float32 (9923a6b9) | clé primitive globale (A, Bx, By, Bz, C) | f32 | `mhgp8_f32` | RÉFÉRENCER | Prouvée et testée, mais sans ordre total ni stockage de catalogue (L4 § 5). |
| `core/float32_ball_key.hpp` | 66 | `e02a24bfa436` | core, float32 (9923a6b9) | interface de la clé | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |
| `core/float32_predicates.hpp` | 245 | `1f6175bb27cd` | core, float32 (36724438) | prédicat q2 exact (intervalles `nextafter`, accumulateur 576 bits), garde `#error` sous `__FAST_MATH__` | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Voie dormante ; seul le motif de garde `__FAST_MATH__` se porte comme doctrine (L6 § 7). |
| `core/float32_q3_block.cpp` | 169 | `4e9ce4f4e371` | core, float32 (a005f8aa) | bornes de bloc q3 (enveloppe de centres, 6 paraboles) | f32 | `mhgp8_f32` | RÉFÉRENCER | Enveloppe lâche, aucun mutant compilé (L4 § 5) ; ses fixtures F1-F10 servent aussi l'entier. |
| `core/float32_q3_block.hpp` | 61 | `1737edb1090b` | core, float32 (a005f8aa) | interface du bloc | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |
| `core/float32_q4_events.cpp` | 171 | `a1b55373271b` | core, float32 (9923a6b9) | comparateur d'événements q4 de degré 5 (identité GΔ) | f32 | `mhgp8_f32` | RÉFÉRENCER | Prouvé par B et testé (L4 § 3) ; dormant. |
| `core/float32_q4_events.hpp` | 63 | `526d340b4aca` | core, float32 (9923a6b9) | interface du comparateur | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |
| `core/types.hpp` | 122 | `dbe746853b3a` | core (3589a2c9, 18 bits a74e90f2) | `Coordinate` int32 à 18 bits, `max_index_depth` = 54, `Point3`, `Box3`, `valid_box`, `counter_add` | Q34 + Q2 | en-tête `mhgp8_p0` et oracles | PORTER | H § 2 ligne 1, avec un type de point certifié : `valid_box` ne borne pas le domaine (`types.hpp:81-90`, L2 § 5.5). |

### 3.2 `src/lanes`

| fichier | l. | sha256 | module | rôle | chemin | bibl. | disposition | justification |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| `lanes/edge_cover.cpp` | 124 | `783de307434b` | lanes, T24 (77f659e4) | cover fermé d'une arête `‖2z−a−b‖² ≤ 4D` en plages de rangs, parcours sans pile | Q34 | `mhgp8_p0` | PORTER | Inclusion de la boule propriétaire prouvée (L3 § 3, § 7) ; absent de H § 2 alors que l'atlas en dépend. |
| `lanes/edge_cover.hpp` | 80 | `4c45472745a0` | lanes, T24 (77f659e4) | `Q34EdgeCover`, `Q34EdgeCoverWork` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem. |
| `lanes/exact_ball.cpp` | 155 | `c4f8acc1e3c3` | lanes, T23 (785d0589) | boule exacte q2/q3/q4, clé primitive (A > 0, pgcd 1), puissance i128 | Q34 (`make_q3`, `make_q4`, `power`) | `mhgp8_p0` | PORTER | H § 2 ; fabriques sans garde de domaine (`exact_ball.cpp:72/83/104`, L6 § 5.3). |
| `lanes/exact_ball.hpp` | 43 | `1280c2ee3940` | lanes, T23 (785d0589) | interface `ExactBall` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; commentaire « CPU u16 primitive » périmé (l. 19). |
| `lanes/family_certificate.cpp` | 92 | `55285a47ab62` | lanes, T25 (8d0a0f0f) | certificat familial d'une graine (corde, J = D(3G−2EX)) | lié | `mhgp8_p0` | ABANDONNER | Intégration T25-T27 fermée : résidu dense, régressions jusqu'à ×1,56 (FP « Voies q3/q4 », L3 § 7) ; la preuve survit par sa note (H § 1). |
| `lanes/family_certificate.hpp` | 50 | `2b9ed0da7c1f` | lanes, T25 (8d0a0f0f) | interface du certificat | lié | en-tête `mhgp8_p0` | ABANDONNER | Idem. |
| `lanes/float32_q3_census.cpp` | 215 | `a5edef038546` | lanes, float32 (a005f8aa) | census q3 float32 d'une arête (`Individual`, `SharedPrefix`) | f32 | `mhgp8_f32` | RÉFÉRENCER | Une seule arête fournie, jamais une trame (L4 § 3) ; dormant. |
| `lanes/float32_q3_census.hpp` | 78 | `d279d4005841` | lanes, float32 (a005f8aa) | interface du census float32 | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |
| `lanes/q34_collective.cpp` | 236 | `4a92602ffbf9` | lanes, T26 (ddaafdc0) | minimum collectif du pool et corde resserrée par graine | lié | `mhgp8_p0` | ABANDONNER | Même fermeture que T25 (FP) ; ne pas confondre avec le certificat collectif **d'arête** de l'audit du 22/09, à porter (H § 1). |
| `lanes/q34_collective.hpp` | 110 | `e8a9167cdfea` | lanes, T26 (ddaafdc0) | interface, `Q34PoolOptions` | inclus sans usage (chaîne d'inclusion) | en-tête `mhgp8_p0` | ABANDONNER | Tiré dans le chemin par `q4_local_partition.hpp:3` → `q4_center_map.hpp:3` ; chaîne à casser (L3 § 8.1). |
| `lanes/q34_cover.cpp` | 568 | `eea82b98405e` | lanes, T24 (77f659e4) | générateur par graine sur cover (« covered path ») et compositions avec T25-T27 | lié (sonde) | `mhgp8_p0` | LEGACY | Référence différentielle de `tests/q4_local_gate.cpp:135` (« original covered path ») ; son usage comme stockage partagé est fermé (FP). |
| `lanes/q34_cover.hpp` | 51 | `552926a97d6d` | lanes, T24 (77f659e4) | interface du générateur couvert | lié | en-tête `mhgp8_p0` | LEGACY | Idem. |
| `lanes/q34_pair_bounds.hpp` | 84 | `3ec4aedaa4ef` | lanes, T33 (2629a536) | bornes affines exactes de 4H et de Ξ pour une paire fixée | Q34 (mode `Affine`) | en-tête `mhgp8_p0` | PORTER | H § 2 (recherche de témoins par paire, bornes affines) ; formule de Ξ relue, exacte. |
| `lanes/q34_pruning.cpp` | 51 | `63e9d10ad1ac` | lanes, T25 (8d0a0f0f) | pool universel de témoins proposés par arête | lié | `mhgp8_p0` | ABANDONNER | Fermé : résidu dense (FP, L3 § 7). |
| `lanes/q34_pruning.hpp` | 77 | `10cb7fee6d35` | lanes, T25 (8d0a0f0f) | `Q34WitnessPool`, interface | inclus sans usage (chaîne d'inclusion) | en-tête `mhgp8_p0` | ABANDONNER | Idem ; même chaîne d'inclusion. |
| `lanes/q34_seed.cpp` | 136 | `3d1ba36ad426` | lanes, T23 (785d0589) | générateur scalaire q3/q4 d'une graine (balayage en O(n)) | non | `mhgp8_p0` | LEGACY | Référence scalaire simple de deux portes, sans mesure d'échelle ; utile comme juge différentiel par graine. |
| `lanes/q34_seed.hpp` | 65 | `55378ea41543` | lanes, T23 (785d0589) | type émis `Q34SeedCandidate`, `Q34SeedConsumer` | Q34 (types) | en-tête `mhgp8_p0` | PORTER | Type du flux mesuré, absent de H § 2 ; IDs `size_t` et vues empruntées à revoir (L2 § 5.12). |
| `lanes/q34_witness_search.cpp` | 227 | `af48d0bdbda8` | lanes, T32-T33 (d1b4dbc6) | recherche des témoins du citron, seuils K−1 et K−2, modes `Legacy`, `Exclusion`, `Affine` | Q34 | `mhgp8_p0` | PORTER | H § 2 ; réactiver les mutants α4 (site dupliqué l. 155/169, L6 § 3) ; message « u16 DFS depth » périmé (l. 87). |
| `lanes/q34_witness_search.hpp` | 126 | `e5e278fe747c` | lanes, T32 (d1b4dbc6) | interface, registres de recherche et de bornes | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; commentaire « at most49 … sixteen » périmé (l. 84-86). |
| `lanes/q3_ball_census.cpp` | 177 | `1f72e612d7d1` | lanes, T32 (d1b4dbc6) | census q3 `GlobalBoxes` : saturation stricte, puis coquille complète (`PreparedPower`) | Q34 | `mhgp8_p0` | PORTER | H § 2 ; relu en entier : pile de 55 cadres suffisante, minimum entier exact par axe (plancher et plafond du sommet). |
| `lanes/q3_ball_census.hpp` | 86 | `d80558a46bf3` | lanes, T32 (d1b4dbc6) | interface, `Q3BallCensusWork` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; commentaire « local49-frame … three16-bit » périmé (l. 78-79). |
| `lanes/q4_center_map.cpp` | 345 | `a932c3cc0e11` | lanes, T27 (66b1551f) | carte des centres q4 à la demande (cache de rejet), Q = 2^42 | lié | `mhgp8_p0` | ABANDONNER | Fermé comme cache de rejet (FP) ; marge de 0,4 bit sous 2^127 (L3 § 3). |
| `lanes/q4_center_map.hpp` | 94 | `bc775983c9dd` | lanes, T27 (66b1551f) | interface de la carte et énumération `Q4CenterDomainMode` (l. 10) | types (`Q4CenterDomainMode`) | en-tête `mhgp8_p0` | ABANDONNER, sauf l'énumération | Seule `Q4CenterDomainMode` sert l'atlas (`q4_local.hpp:10`, `q4_local_partition.hpp:69`) : la déplacer dans l'en-tête de l'atlas. |
| `lanes/q4_family.cpp` | 193 | `32b94adef45c` | lanes, T22 (9ae4e28b) | famille à un paramètre d'une graine aiguë : `side`, `power`, `compare_roots` sans produit P1·B2 ; `run_q4_family` | Q34 (`Q4FamilySeed`) | `mhgp8_p0` | PORTER | H § 2 ; `run_q4_family` (balayage scalaire) reste hors chemin et ne se porte pas. |
| `lanes/q4_family.hpp` | 86 | `44c816707d97` | lanes, T22 (9ae4e28b) | interface `Q4FamilySeed` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem. |
| `lanes/q4_local.cpp` | 763 | `ebe0087c79d8` | lanes, T28 (c051bdb0), T34, 0948d2d0, a74e90f2 | atlas Local28 (quadtree de centres, Deep ≥ K−2), certificat q3, balayage local, parcours `LiveOnly` et `Joined` | Q34 (l. 12-363, 452-605, 719-761) ; `Individual` l. 364-447 et `Joined` l. 607-717 liés, non exécutés | `mhgp8_p0` | PORTER (découpé, § 4) | H § 2, poste dominant ; `Joined` sans gain stable (FP) et `Individual` historique restent LEGACY. |
| `lanes/q4_local.hpp` | 113 | `7a1e25185f07` | lanes, T28 (c051bdb0) | `Q4LocalOptions`, `Q4LocalAtlas`, certificats `certified_inside_count` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; `root_certified_inside_count` sert une branche morte (L3 § 5.11). |
| `lanes/q4_local_partition.cpp` | 374 | `210caaa63bd2` | lanes, T28, 748ec082, a74e90f2 | géométrie du plan des centres (base, domaine, enveloppe, `node_bounds` en i64 à Q = 2^20), fragments exacts, `q3_center` | Q34 | `mhgp8_p0` | PORTER | H § 2 ; `node_bounds_unchecked` pèse 40,32 % du profil de référence (L3 § 4, gprof épinglé). |
| `lanes/q4_local_partition.hpp` | 171 | `6835f7712363` | lanes, T28 (c051bdb0) | `Q4LocalCell`, `Q4LocalGeometry`, `Q4LocalFragment` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; inclut `q4_center_map.hpp` pour une seule énumération. |
| `lanes/q4_positive_domain.cpp` | 145 | `8f81fa77a171` | lanes, T27 (66b1551f) | AABB exacte des complétions de la lentille (domaine positif q4) | Q34 | `mhgp8_p0` | PORTER | Appelé par `Q4LocalGeometry` en mode `Positive` (défaut, `q4_local_partition.cpp:66-70`) ; absent de H § 2 ; 481,4 M visites à K5, scène 0 (L3 § 4). |
| `lanes/q4_positive_domain.hpp` | 84 | `a414a31d3fce` | lanes, T27 (66b1551f) | interface, `Q4PositiveDomainWork` | Q34 | en-tête `mhgp8_p0` | PORTER | Idem. |
| `lanes/q4_seed_cells.hpp` | 102 | `4974562d79b9` | lanes, T34 (4c3cdb0c) | API `LiveOnly` / `Joined`, registre `Q4SeedCellWork` | Q34 | en-tête `mhgp8_p0` | PORTER (`LiveOnly`) | Absent du motif `q4_local*` de H § 2 ; créé par le commit d'audit 4c3cdb0c (incident de provenance, L3 § 2) ; défaut `Individual` à remplacer (D3). |
| `lanes/q4_shallow.cpp` | 187 | `937d301ee26f` | lanes, T29 (31b0243a) | balayage à couches convexes duales (Shallow29) | lié (sonde) | `mhgp8_p0` | ABANDONNER | Fermé comme remplacement et comme filtre (adversaire K10 ×10,27, FP) ; aucune entrée du flux ne l'appelle. |
| `lanes/q4_shallow.hpp` | 46 | `84dae0c614c6` | lanes, T29 (31b0243a) | types de balayage repris par Window30 | lié (types de Window30) | en-tête `mhgp8_p0` | LEGACY | Dépendance de type de `q4_window.hpp` ; suit Window30. |
| `lanes/q4_shallow_set.cpp` | 206 | `c185375f2a00` | lanes, T29 (31b0243a) | ensemble de couches (témoins) consommé par Window30 | lié | `mhgp8_p0` | LEGACY | Suit Window30 (`q4_window.cpp:273-274`). |
| `lanes/q4_shallow_set.hpp` | 72 | `bc669ff04a80` | lanes, T29 (31b0243a) | interface `Q4ShallowSet` | lié | en-tête `mhgp8_p0` | LEGACY | Idem ; commentaire « u16 coefficient bounds » périmé (`.cpp:75`). |
| `lanes/q4_window.cpp` | 296 | `2ebc96296053` | lanes, T30 (f07fbd8c) | backend q4 Window30 (fenêtre fermée de faible profondeur) | lié (branche `wspd_q34.cpp:527-531`) | `mhgp8_p0` | LEGACY | Plus lent en médiane (1,153 sur 144 essais) mais plus rapide à 32k K5 sur une observation : cible différentielle exacte de l'atlas (L3 § 7). |
| `lanes/q4_window.hpp` | 71 | `21cf68c4936c` | lanes, T30 (f07fbd8c) | interface Window30 | lié | en-tête `mhgp8_p0` | LEGACY | Idem. |

### 3.3 `src/parallel`, `src/spindle`, `src/wspd`, `src/spatial`

| fichier | l. | sha256 | module | rôle | chemin | bibl. | disposition | justification |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| `parallel/joined_workers.hpp` | 67 | `91455d121702` | parallel, T13 (b268cf6f) | équipe jointe, exceptions par emplacement, lanceur injectable | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER | H § 2 ; à remplacer à terme par des files par worker et une porte TSan (L6 § 5.2). |
| `parallel/work_reduction.hpp` | 167 | `2d7e17f473c5` | parallel, T13 (b268cf6f) | fusion des compteurs du front et du census, `static_assert` de taille | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER (motif) | L6 § 7 ; inclut l'en-tête d'entrée `wspd_q2_census.hpp` pour de simples types. |
| `spindle/predicates.hpp` | 251 | `8ea1509a309c` | spindle, T1 (3589a2c9) | `h_minimum`, `xi_bounds`, `h_maximum_times_four`, témoins ponctuels, universels et de bloc | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER | H § 2 ; bornes 18 bits relues (Ξ ≤ 12M^4, 432M^4 < 2^81) ; empreinte sans garde de domaine (L2 § 5.5). |
| `spindle/q2_prepared_bounds.hpp` | 94 | `022c141855e0` | spindle, T5 (3c29ea1e) | bornes préparées `4H = D − (2z−C)²` sur {a}×B×Z, constantes u64 | Q34 (via bornes conjointes et de paire) + Q2 | en-tête `mhgp8_p0` | PORTER | H § 2 ; fixtures 262 143 à ajouter (L2 § 5.4). |
| `wspd/front.cpp` | 762 | `4ce25409737c` | wspd, T7 (da366f7f), T13, T14, T20-T21 | front WSPD `box_gap_diameter_v1`, `MidpointSamples`, fenêtre 2K/4K, héritage de témoins, plan de jobs, dispatcher `Donate` (l. 502-762) | Q34 + Q2 (Front, Jobs) ; Q2− (`Donate`) | `mhgp8_p0` | PORTER (découpé, § 4) | H § 2 ; ×0,35 à ×0,65 hors rangées (mesuré, L2) ; `Donate` sans gain (FP). |
| `wspd/front.hpp` | 322 | `5921a333d7c7` | wspd, T7 (da366f7f) | `WspdFrontProposals`, `WspdRectangle`, `WspdFrontWork`, `WspdFrontJobs`, `WspdFrontDispatch` | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER (sans `WspdFrontDispatch`) | Idem. |
| `spatial/float32_index.cpp` | 196 | `2b602f47e72e` | spatial, float32 (028a0f1d) | index natif float32 (rangs médians, boîtes exactes) | f32 | `mhgp8_f32` | RÉFÉRENCER | Prouvé, testé et mesuré (L4 § 3) ; dormant. |
| `spatial/float32_index.hpp` | 117 | `4137b8129faf` | spatial, float32 (028a0f1d) | interface de l'index float32 | f32 | en-tête `mhgp8_f32` | RÉFÉRENCER | Idem. |

### 3.4 `src/pipeline`

| fichier | l. | sha256 | module | rôle | chemin | bibl. | disposition | justification |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| `pipeline/axis_q2.cpp` | 581 | `96bb1aad6a27` | pipeline, T2-T3 (8e406f9b) | filtre axial q2, addition de colonnes, intersection de résidus (`AxisQ2Plan`) | lié (références de `q2_census.cpp`) | `mhgp8_p0` | ABANDONNER | Rotation : 256 M paires conservées ; le LiDAR n'a pas d'alignements (L2 § 7.2, FP). |
| `pipeline/axis_q2.hpp` | 129 | `39d297b6d462` | pipeline, T2 (8e406f9b) | interface `AxisQ2Plan` | inclus sans usage (`q2_census.hpp:3`) | en-tête `mhgp8_p0` | ABANDONNER | Idem ; l'index doit en être découplé (L2 § 8.10). |
| `pipeline/local_credits.cpp` | 465 | `ffc9d9782da8` | pipeline, T1 (3589a2c9) | crédits locaux Pool et DualBlocks sur un rectangle isolé, fabrique `Rectangle` | lié | `mhgp8_p0` | ABANDONNER | Hors chemin WSPD, remplacé par `Q2NodePoolPlan` (L2 § 7.2) ; seul client de l'arbre de plages du nuage (l. 307-308). |
| `pipeline/local_credits.hpp` | 253 | `a68fed7a4004` | pipeline, T1 (3589a2c9) | `Rectangle`, `CreditPlan`, `CreditBatch` | inclus sans usage (chaîne `q2_census.hpp` → `axis_q2.hpp`) | en-tête `mhgp8_p0` | ABANDONNER | Idem. |
| `pipeline/tube_credits.hpp` | 244 | `146fda33096f` | pipeline, T1 (3589a2c9) | crédits Tubes (lemme d² ≥ 100 diam²) | lié (dans `local_credits.o`) | en-tête `mhgp8_p0` | ABANDONNER | 0 crédit sur nuage irrégulier (FP « Voie q2 »). |
| `pipeline/prepared_cloud.cpp` | 137 | `764bba343924` | pipeline, T6 (85015a8c) | copie privée, refus de plage [0, 2^18), unicité par clés 3×18 bits, arbre de plages en ordre d'entrée | Q34 + Q2 (arbre de plages : non) | `mhgp8_p0` | PORTER sans l'arbre de plages | H § 2 ; l'arbre (l. 79-94, 99-126) ne sert qu'à T1 et pèse 80 % du nuage (§ 6, D1). |
| `pipeline/prepared_cloud.hpp` | 75 | `e9ce29e529c8` | pipeline, T6 (85015a8c) | `Range`, `CloudPtr`, `PreparedCloud`, `CloudWork` | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER | Idem ; retirer `bounds(Range)` et `range_tree_` (l. 58-71). |
| `pipeline/q2_census.cpp` | 2 852 | `f9a3faf9613b` | pipeline, T4-T21 (f4815cd4, 17 commits) | index global (l. 97-194), moteur census (195-773), lots singletons (774-1033), `run_q2_census` axial (1034-1062), entrées WSPD série et parallèle (1063-1374), continuation, coopérative, plages, lots (1375-2852) | Q34 (index seul) ; Q2 (1-773, 1063-1374) ; Q2− (reste) | `mhgp8_p0` | PORTER (découpé, § 4) | L2 § 7.1 ; monolithe dont 1 381 lignes servent des ordonnanceurs sans gain (L2 § 5.6). |
| `pipeline/q2_census.hpp` | 147 | `903f5ee92d72` | pipeline, T4 (f4815cd4) | `Q2Support`, `Q2BallKey`, `Q2CensusIndex`, `Q2SpatialNode`, `run_q2_census` | Q34 + Q2 | en-tête `mhgp8_p0` | PORTER (sans `axis_q2.hpp`) | Index de toutes les voies (L2 § 3.2) ; retirer `run_q2_census` et `make_q2_census_index(RectanglePtr)`. |
| `pipeline/q2_census_parallel.cpp` | 189 | `6f2f7f9fb5fb` | pipeline, T16 (897085f8) | parallélisme d'une seule ancre par continuations et dons de frères B | non | `mhgp8_p0` | ABANDONNER | Détachement : 8 comparaisons favorables sur 72 (FP) ; jamais raccordé au WSPD. |
| `pipeline/q2_census_parallel.hpp` | 71 | `585152695dd9` | pipeline, T16 (897085f8) | `run_q2_anchor_parallel` | non | en-tête `mhgp8_p0` | ABANDONNER | Idem. |
| `pipeline/q2_census_resume.hpp` | 185 | `e71d26f37b26` | pipeline, T15 (d09e2207) | continuation reprenable d'une ancre (6 272 octets en u16) | Q2− (coopérative) ; lié | en-tête `mhgp8_p0` | ABANDONNER | +38 à +45 % sur un fil ; taille à 18 bits jamais publiée (L2 § 5.16, § 7.2). |
| `pipeline/q2_joint_bounds.hpp` | 91 | `c1e383e501e2` | pipeline, T11 (b2106c3c) | extrema exacts de 4H sur A×B×Z (`Q2JointPreparedBounds`) | Q34 (recherche de témoins) ; Q2 non (modes conjoints négatifs) | en-tête `mhgp8_p0` | PORTER | H § 2 ; seul survivant utile de la tranche 11 (L2 § 7.2). |
| `pipeline/q2_node_pool.hpp` | 263 | `49c989b057f0` | pipeline, T12 (ba11e3ab) | Pool terminal par facteurs (bandes h_a + h_b) | Q2 | en-tête `mhgp8_p0` | PORTER | ×3,7 à ×17,2 sur amas (mesuré, L2 § 7.1). |
| `pipeline/wspd_q2_batched.hpp` | 108 | `50e57789ed4f` | pipeline, T19 (8d615cfd) | entrée à lots de singletons entrelacés | Q2− | en-tête `mhgp8_p0` | ABANDONNER | ×1,005 à ×1,225, aucune comparaison plus rapide (FP). |
| `pipeline/wspd_q2_census.hpp` | 144 | `23dc8751e9f0` | pipeline, T8-T21 (f7edd646) | entrée série `run_wspd_q2_census`, énumérations frère, ordre, ancres, Pool, propositions | Q2 | en-tête `mhgp8_p0` | PORTER | Défauts = configuration mesurée (L2 § 5.3) ; absent de H § 2. |
| `pipeline/wspd_q2_cooperative.hpp` | 112 | `dd1069355d06` | pipeline, T17 (beee3341) | équipe persistante front et census | Q2− | en-tête `mhgp8_p0` | ABANDONNER | 17 comparaisons de rangées sur 18 plus lentes (FP). |
| `pipeline/wspd_q2_parallel.hpp` | 86 | `af975aeff41c` | pipeline, T13-T14 (b268cf6f) | entrée parallèle `Coarse` / `Donate` | Q2 (Coarse) | en-tête `mhgp8_p0` | PORTER (Coarse seul) | W4 ×3,4 à ×3,9 (L2) ; `Donate` sans gain général (FP). |
| `pipeline/wspd_q2_ranges.hpp` | 111 | `ea540220ced7` | pipeline, T18 (2741d614) | plages d'ancres, parents Pool possédés | Q2− | en-tête `mhgp8_p0` | ABANDONNER | Pas de gain stable (L2 § 2, FP). |
| `pipeline/wspd_q34.cpp` | 877 | `79ae04fe5056` | pipeline, T31 (4dbe3024) à a74e90f2 | flux global q3/q4 : validation, moteur par rectangle et par arête, file bornée de plages, registres additifs | Q34 (branches `ScalarCover` l. 561-585 et Window30 l. 527-531 liées, non exécutées) | `mhgp8_p0` | PORTER | H § 2, en configuration unique ; défauts historiques lents (L3 § 5.7, L6 § 5.10). |
| `pipeline/wspd_q34.hpp` | 232 | `ac9840848a3e` | pipeline, T31 (4dbe3024) | options, registres, entrées mono et parallèle | Q34 | en-tête `mhgp8_p0` | PORTER | Idem ; l'entrée mono `run_wspd_q34_candidates` ne sert qu'à `tests/wspd_q34_gate.cpp`. |

## 4. Fichiers à découper

| fichier | segment | lignes | disposition | raison |
| --- | --- | --- | --- | --- |
| `q2_census.cpp` | aides, index, moteur census | 1-773 | PORTER | L2 § 7.1 (index 97-194, moteur 197-774) |
| `q2_census.cpp` | `Q2SingletonBatch`, `enqueue_singleton` | 774-1033 | ABANDONNER | T19 négatif |
| `q2_census.cpp` | `run_q2_census` sur plan axial | 1034-1062 | ABANDONNER | T4 hors WSPD, dépend d'`AxisQ2Plan` |
| `q2_census.cpp` | `consume_wspd_rectangle`, `run_wspd_q2_census`, `run_wspd_q2_census_parallel` | 1063-1374 | PORTER, sans la branche `Donate` (l. 1263-1290, 1341) | entrées mesurées gagnantes |
| `q2_census.cpp` | continuation, coopérative, plages, lots | 1375-2852 | ABANDONNER | T15-T19 sans gain |
| `wspd/front.cpp` | `Front`, `validate_front`, `run_wspd_front`, `WspdFrontJobs` | 1-500 | PORTER | H § 2 |
| `wspd/front.cpp` | `WspdFrontDispatch` (`Donate`) | 502-762 | ABANDONNER | T14 sans gain général |
| `q4_local.cpp` | atlas, localisation, balayage local | 1-363 | PORTER | poste dominant, H § 2 |
| `q4_local.cpp` | parcours `Individual` (`visit`, `seed`, générateur d'arête) | 364-447 | LEGACY | ancien défaut, cible différentielle de `LiveOnly` |
| `q4_local.cpp` | moteur `LiveOnly` (`prepare_live`, filtres, `visit_live`, `live_only`) | 452-605 | PORTER | configuration mesurée |
| `q4_local.cpp` | `Joined` (`prepare_cache`, `joined`) | 606-718 | LEGACY | sans gain stable (FP) |
| `q4_local.cpp` | `run` et entrées | 719-763 | PORTER | configuration mesurée |
| `prepared_cloud.*` | arbre de plages (`range_tree_`, `bounds`) | `.cpp:79-94, 99-126` ; `.hpp:58-59, 69-71` | ABANDONNER | seul client : T1 (`local_credits.cpp:307-308`) |
| `q4_center_map.hpp` | `enum class Q4CenterDomainMode` | l. 10 | PORTER (à déplacer) | seul symbole du fichier utilisé par l'atlas |

## 5. Relecture des passages qu'aucune lentille n'avait relus

### 5.1 `lanes/q4_local.cpp:520-763` (`LiveOnly`, `Joined`, entrées)

- `prepare_live` (l. 508-533) : parcours en ID décroissant, valide parce que
  les enfants ont des ID supérieurs au parent (`nodes.resize(first+4)` dans
  `build`, l. 209-215) ; topologie contrôlée (l. 523-524) ; somme finale
  comparée à `leaf_cells` (l. 531-532). `live` est un membre neuf à chaque
  appel (le moteur est construit par arête, l. 758), donc sans reliquat pour
  les cellules `Deep` et `Outside`.
- `visit_live` (l. 566-581) : même ordre de visite des quadrants que
  `visit` (l. 364-374) ; seule différence, l'élagage des sous-arbres sans
  feuille vivante avant le test de droite. Sorties identiques et dans le même
  ordre par construction, ce que la porte T34 juge (testé, L3 § 3).
- `live_only` (l. 583-605) : même générateur de graines par boîtes que
  l'entrée `Individual` (l. 417-441), avec les mêmes bornes (`min_a`, `min_b`,
  `max_sum`, somme < 2^39 à 18 bits).
- `joined` (l. 624-717) : produit graines × cellules, partition exacte du
  produit (chaque couple graine singleton × feuille est atteint au plus une
  fois : les découpages de X et de cellule sont des partitions) ; pile fixe de
  187 descripteurs, suffisante car `Q4LocalCell::max_depth` = 20 ≤ 44
  (`stack_capacity`, l. 463) ; cache par bloc réinitialisé par `resize`
  (l. 607-622).
- Ordre d'initialisation des membres de `Q4SeedCellEngine` (l. 465-486) :
  conforme à l'ordre de déclaration (`engine` avant `nodes`, `points` et
  `edge` avant `a` et `b`).

Aucun défaut de correction. Deux observations (D4, D5 au § 6).

### 5.2 `lanes/edge_cover.cpp`

Cover fermé `‖2z−a−b‖² ≤ 4‖b−a‖²` (l. 38-45). Bornes à 18 bits recalculées :
déplacement doublé dans [−2M, 2M], norme ≤ 12M² < 2^40 (commentaire l. 31-34
exact). Parcours sans pile par échappements (l. 73-116) : admission d'un bloc
seulement si le maximum exact de la boîte tient, rejet si le minimum exact
dépasse ; plages fusionnées en ordre croissant. Les ID d'arête sont triés à la
fabrique (l. 18), ce qui aligne la règle de propriété de `wspd_q34.cpp:606`
et `q4_local.cpp:28`. Aucun défaut. Rappel : en `GlobalBoxes`, le cover d'une
arête q3 seule ne sert à rien (L3 § 5.13, 172 874 covers à K5, scène 0).

### 5.3 `lanes/q4_positive_domain.cpp`

AABB **exacte** de Z = {z ≠ a, b : ‖z−a‖² ≤ D et ‖z−b‖² ≤ D} : l'union de
boîtes d'index serrées (calculées depuis les points, `q2_census.cpp:111-115`)
de sous-ensembles disjoints est la boîte serrée de l'union ; les extrémités
sont exclues au niveau des feuilles (l. 98-99) et tout bloc qui pourrait les
contenir est raffiné (l. 122-134). Bornes : distances ≤ 3M² < 2^38 (l. 57-62,
exact). La projection de la boîte sur le plan bissecteur, jointe à zéro, couvre
le centre de toute boule q4 positive propriétaire de l'arête (le centre est
combinaison convexe de a, b, x, y ; a et b se projettent sur zéro) : vérifié
avec `prepare_hull` (`q4_local_partition.cpp:149-205`). Aucun défaut.
Observation D6 : le parcours repart de la racine de l'index.

### 5.4 `wspd/front.cpp`

- Séparation `gap² ≥ s²·max(diag²)` en i128 (l. 148-149) ; seuils par voie
  `Kmax − voie` pour les voies actives `voie < Kmax` (l. 90-95), conformes à
  h_q = Kmax + 2 − q ; masques et options refusés avant tout calcul
  (l. 393-414).
- Filtre (l. 214-357) : `h_minimum` exact sur les boîtes continues ; crédit q3
  si `3h² > Ξ_max`, q4 si `2h² > Ξ_max` (l. 318-326) : lemme du citron
  (α3 = 3, α4 = 2). Les propositions dans A ou B sont sautées sans crédit.
  Fenêtre 2K/4K et héritage refusés dès qu'une voie q3/q4 est active
  (l. 406-413), donc hors du chemin Q34.
- `Task` fait 72 octets (16 + 8 + 4 + 1 + 1, puis `WitnessList` de 40 octets
  alignée à 4 : l. 52-73) même quand l'héritage est refusé ; coût mesuré
  +2 à +3,5 % au microbanc du front (L2 § 5.15).
- Plan de jobs (l. 427-465) : préparation en largeur jusqu'à la granularité
  demandée, terminaux comptés dans le préfixe puis rejoués ; aucune
  soustraction non signée possible (court-circuit l. 449).
- Dispatcher `Donate` (l. 502-762) : anneau borné, offres non bloquantes,
  annulation sous le même mutex que l'attente (l. 528-538). Aucun
  interblocage trouvé à la lecture ; code sans gain (FP).

Aucun défaut de correction.

### 5.5 Index global, `pipeline/q2_census.cpp:97-194`

- Coupe au milieu de l'axe le plus étendu (l. 128-141) ; le refus de
  doublons de `prepare_cloud` garantit une coupe non triviale (l. 143-145).
  Profondeur ≤ 3 × 18 = 54 : chaque coupe ramène l'étendue de l'axe coupé à au
  plus ⌊E/2⌋. Validation structurelle des échappements (l. 160-166).
- L'ordre spatial est canonique : les feuilles sont des singletons rangés en
  préordre, et les ensembles de chaque nœud ne dépendent que des coupes. Le
  `std::partition` non stable (l. 138) ne change donc ni l'arbre ni les rangs.
- Observations D2 (capacité non réservée) et D8 (message périmé l. 104).

### 5.6 Autres relectures complètes

`q4_local_partition.cpp` (374 l., dont 300-374 non relues par L3) :
fragments conformes au contrat « min = 0 reste actif » (l. 339-354) ;
`node_bounds_unchecked` (l. 240-276) recalculé : chaque terme d'axe
< 1,25 × 2^60, somme < 2^62, arrondi du minimum vers le bas ; `outside`
(l. 207-231) : disque de rayon √(D/8) en coordonnées de cellule
(2Q × coefficient réel), puis complétions, puis facettes. `q3_center`
(l. 118-139) : solution de Cramer vérifiée, tailles < 2^117.
`q3_ball_census.cpp` (177 l.) : deux passes, pile de 55 cadres suffisante.
`q34_pair_bounds.hpp` : composantes de `d × (z−a)` exactes, Ξ ≤ 12M^4.
Aucun défaut.

## 6. Défauts et observations

Gravité : haute, moyenne, basse. Aucun défaut de correction n'a été trouvé ;
les constats sont des dettes de port, de mémoire ou de documentation.

| # | gravité | constat | preuve | statut |
| --- | --- | --- | --- | --- |
| D1 | moyenne (port) | `PreparedCloud` construit et garde un arbre de plages en ordre d'entrée (2n boîtes de 24 octets) que seul T1 utilise ; à 39 815 sites il pèse 1 911 120 octets sur 2 388 900, soit 80 % du nuage. H § 2 demande pourtant de porter l'« arbre de plages ». | `prepared_cloud.cpp:79-94` ; unique client `local_credits.cpp:307-308` ; les 2,39 Mo (1,19 Mo en u16) du journal de a74e90f2 se retrouvent exactement par ce calcul | déduit (lecture et arithmétique) ; le chiffre du journal reste non épinglé |
| D2 | basse | `Q2CensusIndex::nodes_` grandit par `push_back` sans `reserve(2n−1)` : à 39 815 sites, capacité 131 072 × 64 octets = 8 388 608 octets pour 79 629 nœuds, soit 38 % de mémoire d'index gaspillée. Les 8,71 Mo (7,66 Mo en u16, nœud de 56 octets) du journal correspondent exactement à ce calcul. | `q2_census.cpp:116-117`, `sizeof(Q2SpatialNode)` = 16 + 24 + 24 | déduit ; gain non mesuré |
| D3 | basse (risque) | `run_wspd_q34_parallel` n'installe pas de crochet `on_cancel` (`wspd_q34.cpp:752`) et le prédicat d'attente de la file ignore l'atomique `cancel` (l. 783-784). L'annulation après un échec de lancement ne réveille les attentes que lorsque le dernier worker occupé termine. Pas d'interblocage à la lecture, mais aucun passage TSan depuis 5224ff4e (L6 § 5.2). | `wspd_q34.cpp:752, 783-793, 824-836` ; `parallel/joined_workers.hpp:36-59` | déduit ; à couvrir par une porte TSan et un lanceur défaillant injecté |
| D4 | basse | Double test de propriété par graine q4 : `seed_pass` (l. 559-561) puis `owned` à nouveau (l. 593-594) en `LiveOnly` ; même chose en `Individual` (l. 425 puis 384). Le compteur `seed_owner_rejections` ne peut jamais valoir plus que zéro et `seed_owner_tests` compte deux fois. | `q4_local.cpp:384, 425, 559-561, 593-594` | déduit |
| D5 | basse | `LiveOnly` compte vivante toute feuille, y compris une feuille sans site actif, qui ne peut rien émettre ; chaque graine y paie encore un test de droite et un balayage vide. | `q4_local.cpp:519-520` ; balayage l. 283-311 | proposé ; gain non mesuré |
| D6 | basse | Le domaine positif reparcourt tout l'index depuis la racine pour chaque arête, alors que sa lentille est incluse dans le cover (‖2z−a−b‖ ≤ ‖z−a‖ + ‖z−b‖ ≤ 2√D). Avec 481,4 M visites à K5 sur la scène 0, c'est la deuxième traversée d'index par arête après les graines q3. | `q4_positive_domain.cpp:91-137` ; compteurs `geometry.domain.node_visits` de `only_probe_00` | proposé ; levier de quelques pour cent (L3 § 5.14) |
| D7 | moyenne (doc) | La lentille 3 laisse `outside_domain` inexpliqué (question 10) et signale une « localisation Outside perdue » (§ 5.17). Par lecture, un centre q3 ne tombe jamais dans une cellule `Outside` de disque ou de facette : le seul cas sans certificat est un atlas dont la racine est `Outside` faute de deux complétions. Le § 5.17 est donc vide. | § 7 ci-dessous | déduit (preuve esquissée), non testé |
| D8 | basse | Commentaires et messages périmés non listés par L3 : « newly derived CPU u16 primitive » ; « certified rectangle has no sites » pour un index de nuage. | `exact_ball.hpp:19` ; `q2_census.cpp:104` | constaté |
| D9 | basse (port) | Couplages d'en-têtes : `q4_local_partition.hpp:3` inclut `q4_center_map.hpp` pour la seule énumération `Q4CenterDomainMode`, ce qui tire `q34_collective.hpp` → `q34_pruning.hpp` → `q34_cover.hpp` ; `work_reduction.hpp:3` inclut l'en-tête d'entrée `wspd_q2_census.hpp` ; `q2_census.hpp:3` inclut `axis_q2.hpp` → `local_credits.hpp`. | lignes citées | constaté (L3 § 5.7 et L2 § 5.7 pour les deux derniers) |
| D10 | basse | Le fichier du front, au cœur du chemin mesuré, garde 261 lignes du dispatcher `Donate` sans gain. De même, la sonde de mesure lie les prototypes T24-T27 et T29 (`bench/q4_lidar_probe.cpp:5-7`). | `wspd/front.cpp:502-762` | constaté |

Rappel des défauts déjà établis par les lentilles et confirmés à la lecture :
branche `root_lane_skips` morte (`wspd_q34.cpp:514-515`, L3 § 5.11) ;
recherche de témoins répétée sur les rectangles singletons (l. 406-420 puis
485-494, L3 § 5.12) ; commentaires de découpage contraires au code, qui publie
toutes les plages (l. 424-426 et 738-740 contre 431-435, L3 § 5.15) ;
absence de garde de domaine dans les fabriques publiques (L2 § 5.5, L6 § 5.3).

## 7. `outside_domain` : explication déduite du code

Constat mesuré (reçus `ground_phase1_20260921`, sonde 0e2c18ca) :
`work.q3_atlas.outside_domain` vaut 18 067, 15 611 et 17 447 sur les
scènes 0, 1 et 2, **identique à K5 et K10**, alors que `edges_with_atlas`
passe de 1 530 177 à 3 525 081 (scène 0).

Raisonnement (déduit, non testé) :

1. `certified_inside_count` rend `nullopt` seulement si le point sort de la
   racine ou tombe dans une cellule `Outside` (`q4_local.cpp:233-237`).
2. Centre q3 O d'une graine aiguë de plus longue arête ab : R² ≤ D/3, donc
   ‖t_O‖² ≤ D/12 < D/8. Le test de disque d'une cellule utilise un minorant de
   ‖t‖² (`q4_local_partition.cpp:209-217`) ; une cellule fermée qui contient O
   n'est donc jamais déclarée hors disque, et O n'est jamais hors de la racine.
3. Graine aiguë : O est combinaison convexe à poids positifs de a, b et x, et
   x appartient à la lentille Z. Sa projection est dans l'enveloppe de zéro et
   des coins de l'AABB de Z. Une cellule entièrement au-delà d'une facette de
   cette enveloppe (l. 219-229) ne peut pas contenir O.
4. Reste la seule condition `completion_count() < 2` (l. 218), qui rend la
   **racine** `Outside`. Aucun fragment n'est alors construit
   (`q4_local.cpp:171`) et aucun compte parent n'existe.
5. Une graine q3 x est dans Z. Le cas « une seule complétion » est donc une
   arête dont la lentille ne contient que x. Les témoins du citron sont dans
   la boule diamétrale, incluse dans Z, donc au plus un témoin existe. L'arête
   survit alors aux filtres q3 et q4 dès que K − 2 ≥ 2, c'est-à-dire K ≥ 4, et
   reçoit un atlas (`wspd_q34.cpp:508-512`). Sa graine est dénombrée de la
   même façon à K5 et à K10.

Conséquences :

- `outside_domain` = nombre de graines q3 portées par une arête dont la
  lentille ne contient qu'un site. L'indépendance à K est attendue pour
  K ≥ 4. À K = 3, le seuil q4 vaut 1 et peut rejeter ces arêtes.
- Le défaut L3 § 5.17 (« le compte de la cellule parente serait utilisable »)
  ne concerne aucune localisation observée : il n'y a pas de parent.
- Test proposé pour la v9 : un compteur `root_outside_atlases` et une fixture
  à trois points (a, b, x aigu, sans autre site dans la lentille), dont
  `outside_domain` doit valoir 1 à K4, K5 et K10.

## 8. Ce qui manque dans les documents v9

1. **Aucune carte des sources.** `PLAN_V9.md` (principe 5) demande les
   « anciens chemins dans une cible différentielle séparée » sans les nommer.
   Les cibles LEGACY (§ 3) n'apparaissent nulle part : `q34_cover.cpp`, qui
   sert de référence à `tests/q4_local_gate.cpp:135`, `q34_seed.cpp`,
   Window30 avec `q4_shallow_set.*` et `q4_shallow.hpp`, et les segments
   `Individual` et `Joined` de `q4_local.cpp`.
2. **H § 2 omet des sources du chemin mesuré** : `lanes/edge_cover.*` (citée
   par L3 § 7 seulement), `lanes/q4_positive_domain.*` (citée nulle part comme
   port), `lanes/q4_seed_cells.hpp` (hors du motif `q4_local*`),
   `lanes/q34_seed.hpp` (type du flux), `pipeline/q2_census.hpp`,
   `pipeline/wspd_q2_census.hpp` et `pipeline/wspd_q2_parallel.hpp`.
3. **H § 2 demande de porter l'arbre de plages** de `PreparedCloud`, qui ne
   sert qu'au prototype T1 abandonné et représente 80 % de la mémoire du nuage
   (D1).
4. **L'énumération `Q4CenterDomainMode`** est définie dans l'en-tête du
   prototype T27 (`q4_center_map.hpp:10`) : le port de l'atlas doit la
   déplacer (D9).
5. **Découpage des fichiers mixtes** : ni le dispatcher `Donate`
   (`front.cpp:502-762`) ni les segments de `q2_census.cpp` au-delà de la
   l. 774 (hors mention globale de L2 § 5.6) n'ont de disposition écrite dans
   H. `q2_census_parallel.*`, `wspd_q2_cooperative.hpp`, `wspd_q2_ranges.hpp`
   et `wspd_q2_batched.hpp` ne sont cités nommément dans aucun document v9.
6. **Explication de `outside_domain`** (§ 7) et caducité de L3 § 5.17.
7. **Mémoire des structures portées** : l'index à capacité doublée (D2) et
   l'arbre de plages (D1) expliquent exactement les chiffres non épinglés du
   journal ; aucun document v9 ne le dit.

## 9. Recommandations de port

1. Porter les 36 fichiers PORTER, découpés selon le § 4 (environ 7 660
   lignes), dans une bibliothèque v9 distincte des cibles différentielles.
   Créer d'abord un en-tête de géométrie de centres qui porte
   `Q4CenterDomainMode`, puis des en-têtes de types séparés des entrées.
2. `PreparedCloud` v9 : copie, refus de plage et unicité seulement, sans
   arbre de plages ; index avec `reserve(2n − 1)` et porte de mémoire
   (octets retenus = formule exacte) aux tailles 8k/16k/32k.
3. Épingler les cibles LEGACY par pin et empreinte (annexe A) dans la
   provenance v9. Les campagnes appariées v8/v9 les exécutent depuis le build
   v8 : `LiveOnly` contre `Individual`, Local28 contre Window30, et couvert
   contre atlas via `q34_cover.cpp`.
4. RÉFÉRENCER la voie float32 par ses 14 empreintes au pin a005f8aa/12294241
   (L4 § 8, point 2), sans compilation.
5. Graver les fixtures déduites ici : arête à une seule complétion (§ 7),
   feuille vivante sans site actif (D5), annulation après échec de lancement
   (D3, sous TSan).
6. Supprimer la double vérification de propriété (D4) et mesurer le départ du
   domaine positif depuis la décomposition du cover (D6) par ablation sur les
   trois trames, sorties identiques.

## 10. Limites

- Lecture seule : rien n'a été compilé ni exécuté. Les « déduit » (D1-D7,
  § 7) sont des preuves esquissées ou des calculs de capacité, sans fixture ni
  mesure ; ils valent « proposé ».
- Les corps de `q2_census.cpp` au-delà de la l. 260, des prototypes
  T24-T27, T29-T30, de `axis_q2.cpp` et de `local_credits.cpp` n'ont pas été
  relus ligne à ligne. Leur disposition s'appuie sur les mesures de fermeture
  des lentilles 2 et 3 et de FP, pas sur une relecture de correction.
- Les 14 fichiers float32 n'ont pas été relus. Leur disposition s'appuie sur
  L4 et sur l'égalité de leurs empreintes avec les reçus (L4 § 3).
- « Lié » repose sur la lecture des références entre objets et des
  inclusions de la sonde, pas sur une table de symboles d'un binaire construit.
- Les chiffres de mémoire des journaux de a74e90f2 (1,19 → 2,39 Mo ;
  7,66 → 8,71 Mo) restent **non vérifiables** comme mesures. Seule leur
  concordance arithmétique avec les capacités est établie ici.
- La tranche non commise du constructeur (gardes de domaine, `saturate_deep`)
  est hors du périmètre : ce rapport décrit 12294241 seulement.

## Annexe A : empreintes sha256 complètes à 12294241

Calculées par `sha256sum` dans le worktree détaché `v8_head` (12294241, arbre propre). Total : 78 fichiers, 17 116 lignes.

| fichier (sous `morsehgp3D_v8/`) | lignes | sha256 |
| --- | ---: | --- |
| `src/core/fixed_signed.hpp` | 323 | `cd3facd9e93ddfe668f87a4e2f7faab9c0217596d2835b17f6c01cb0cbf3a3f8` |
| `src/core/float32_ball.cpp` | 222 | `3c89462ee22b3636e21f67d3ec1d3eb5bc440e76e95864fa8dd2814ac8943a46` |
| `src/core/float32_ball.hpp` | 73 | `c673372c58e5d64931827d54bb13a44b69d3835e069e6cc611d9939ae3408973` |
| `src/core/float32_ball_key.cpp` | 94 | `58b5f98ea921c9243a237900c25bb7ecce2217a8d5aa48262acad12ddb8ca3fb` |
| `src/core/float32_ball_key.hpp` | 66 | `e02a24bfa436c81c5ed977d28a1ad7860e1af9c57087c4c69d56a5013f4cc39d` |
| `src/core/float32_predicates.hpp` | 245 | `1f6175bb27cd7543c2d68916efabe68ebd23ac413dadc7aad82cabb30651997d` |
| `src/core/float32_q3_block.cpp` | 169 | `4e9ce4f4e371885cfcd24e0ebe76863481ac109b6ea422b25ce8c10a2851a1d7` |
| `src/core/float32_q3_block.hpp` | 61 | `1737edb1090bcc30220e0ea64b1fab079b288aa22ba7c91483f3e2e55382d944` |
| `src/core/float32_q4_events.cpp` | 171 | `a1b55373271b4d3a8e889ad6cebf9d6972729c18be100bb9d367a401321d4531` |
| `src/core/float32_q4_events.hpp` | 63 | `526d340b4aca9bc52f9aee305707937d95f6e2e20c037756444ea161b5a27c6b` |
| `src/core/types.hpp` | 122 | `dbe746853b3af64f2a82b70cd030ccea05004d5611c4fc7922f0536e392fa578` |
| `src/lanes/edge_cover.cpp` | 124 | `783de307434bd42add85a0631278c72b414d52e260130557104991377fbee47c` |
| `src/lanes/edge_cover.hpp` | 80 | `4c45472745a08f573f1b12bb4b9ae9a73b986400fd6127a1ee60c446734940bc` |
| `src/lanes/exact_ball.cpp` | 155 | `c4f8acc1e3c37e3063b2e3b7290cd51f1b2e025d9338a53c433ba80b6eee39ff` |
| `src/lanes/exact_ball.hpp` | 43 | `1280c2ee3940c542096b5445b186aa0f164dddc9f9eadc7507677f0cdedfbaa4` |
| `src/lanes/family_certificate.cpp` | 92 | `55285a47ab6206691ad428216ac28b2d4cc9beebd199163e6794195f76e4dd08` |
| `src/lanes/family_certificate.hpp` | 50 | `2b9ed0da7c1f3264c4732452ac70bfde5565951f9711e2e9d0bc2133436f9653` |
| `src/lanes/float32_q3_census.cpp` | 215 | `a5edef03854654cdf6bdb8ec26164cf17043e59ca7e7a91c6e76d805c2625c35` |
| `src/lanes/float32_q3_census.hpp` | 78 | `d279d4005841bf3bc45874ec107711965fb3b6e6079b8d35223aed355711da07` |
| `src/lanes/q34_collective.cpp` | 236 | `4a92602ffbf924efdcaee4722746e6cb1007f93df1bd1cdd97000b240a56a5ac` |
| `src/lanes/q34_collective.hpp` | 110 | `e8a9167cdfea4d7246b6fad2a133392ea123e348aa5b4cc05227e95a89ebf1bb` |
| `src/lanes/q34_cover.cpp` | 568 | `eea82b98405e8459f52d1c12ad5f46e40d4f13330c836ade9b6c49a08c6187a7` |
| `src/lanes/q34_cover.hpp` | 51 | `552926a97d6d9232a4f1baf6cb584a3722efa45c3fb3843a0195aef56304b05d` |
| `src/lanes/q34_pair_bounds.hpp` | 84 | `3ec4aedaa4efa7b04292ab5ebc868688b90d930a6bea6afe45fb4b6361538aa6` |
| `src/lanes/q34_pruning.cpp` | 51 | `63e9d10ad1acc455e8cccead74f37017838d3d265ab0446b5192578f126c961f` |
| `src/lanes/q34_pruning.hpp` | 77 | `10cb7fee6d3511b65d8b1ca32dd8ee0dfbeac26710edb70dec3c89dbb4ee7c65` |
| `src/lanes/q34_seed.cpp` | 136 | `3d1ba36ad42690d108795568dba771dad3234e64d094d9c68c75b13e542de5eb` |
| `src/lanes/q34_seed.hpp` | 65 | `55378ea41543eee31ef0537fd2299c0199249f04a933b32c1fea05f981bd929b` |
| `src/lanes/q34_witness_search.cpp` | 227 | `af48d0bdbda8f30801ba41b85487ce8c82eb98d641a09f8738cdf9b5af5eff86` |
| `src/lanes/q34_witness_search.hpp` | 126 | `e5e278fe747c0c82f511a1905f27082e1c89187ee05dd10867b0e92d9f72f5d4` |
| `src/lanes/q3_ball_census.cpp` | 177 | `1f72e612d7d19a66ce69a055fffbd9938a36930c668a5fda56d92ce0bcdfd504` |
| `src/lanes/q3_ball_census.hpp` | 86 | `d80558a46bf38414198fb3f26a3fb8993f3cf9ba9c6a7f94a6f2210bb37dec0f` |
| `src/lanes/q4_center_map.cpp` | 345 | `a932c3cc0e11324e1ec85cb97a4875d7856ca3ae976ae3b842f2a7d2a317fc9d` |
| `src/lanes/q4_center_map.hpp` | 94 | `bc775983c9ddd56517345b99997d0401e04f336044d3416a2096aa7319adb0b4` |
| `src/lanes/q4_family.cpp` | 193 | `32b94adef45cdbebe771fb45ef233354a04564cf3a74209515918a1be3a327ef` |
| `src/lanes/q4_family.hpp` | 86 | `44c816707d97933dfc6094c63669da988914c2f36044d17bd11e718a44fab614` |
| `src/lanes/q4_local.cpp` | 763 | `ebe0087c79d8f6bd137063e14156eb34fecc01d5a74aaf550e1bbe0333546ef3` |
| `src/lanes/q4_local.hpp` | 113 | `7a1e25185f07d9f8cc9056fdb52338bd3a8c926bdab469a20500f2f0054df081` |
| `src/lanes/q4_local_partition.cpp` | 374 | `210caaa63bd247e35e5f745833aba7b6e5686982ce6124b56840d1dc851db8e2` |
| `src/lanes/q4_local_partition.hpp` | 171 | `6835f77123635bf49e731651f34cc02bff79fab2b497b152f011f1bb69f522be` |
| `src/lanes/q4_positive_domain.cpp` | 145 | `8f81fa77a1714893480fccde1215cbc47141a7d52cff9e73cdc0bea83e190c5b` |
| `src/lanes/q4_positive_domain.hpp` | 84 | `a414a31d3fce4cd91ea648369b9b43dd4dde0c1d54b9554f2e5d45a5b794057f` |
| `src/lanes/q4_seed_cells.hpp` | 102 | `4974562d79b9b54341aeadccfa1452d96887af34282c693b32f206530cd70622` |
| `src/lanes/q4_shallow.cpp` | 187 | `937d301ee26f47892ca3d676da67e9c95842471699b3def93cf388f487341003` |
| `src/lanes/q4_shallow.hpp` | 46 | `84dae0c614c642b36c9a8834fd31b97564390651e568c542e30454ee7973fae6` |
| `src/lanes/q4_shallow_set.cpp` | 206 | `c185375f2a00bfca80cd5ba200002e41274998e1b87a98d7689d9b3ba2072525` |
| `src/lanes/q4_shallow_set.hpp` | 72 | `bc669ff04a801d64c16d1edad6d506837f293d01dbce28b13eecdfb3eb00afe9` |
| `src/lanes/q4_window.cpp` | 296 | `2ebc962960531fd9d6483873bcb8cc37a5dfc2c2e9596e1729254e6ce8aeb7bd` |
| `src/lanes/q4_window.hpp` | 71 | `21cf68c4936c5341e81ecf65274d7645ff8bc236fdfe04261f28f943660d7770` |
| `src/parallel/joined_workers.hpp` | 67 | `91455d1217026ed1f70ded9e25775535a116b43515dd276c3c315a061db54493` |
| `src/parallel/work_reduction.hpp` | 167 | `2d7e17f473c5195c5d1befabd07f206878539bb26226eda1d81a7378c86a35a6` |
| `src/pipeline/axis_q2.cpp` | 581 | `96bb1aad6a27089dd75941780afd21fbb69b7d1888c1887ae32c8236a09594a9` |
| `src/pipeline/axis_q2.hpp` | 129 | `39d297b6d4620ad51a187d1f9652185a0f95a58f0b3495afa6b5d24a2b540a8d` |
| `src/pipeline/local_credits.cpp` | 465 | `ffc9d9782da8fe16523d89be7b005f774f7e36da225dd757981add0b981b8240` |
| `src/pipeline/local_credits.hpp` | 253 | `a68fed7a40041ddc76b2a401d35cdafeaa66711ab3cd1d79ff41f3666b29a3d9` |
| `src/pipeline/prepared_cloud.cpp` | 137 | `764bba343924b3784a03d676d55d9fd702205f39a89312344f0ff9dd096f7581` |
| `src/pipeline/prepared_cloud.hpp` | 75 | `e9ce29e529c8ff25e2b8e770dde151164ab4dd166cc04938e7ab189b1e191f39` |
| `src/pipeline/q2_census.cpp` | 2852 | `f9a3faf9613bc5fcd913f998e86a153aef5d8e514880ea87f83d27dee6ef4dbd` |
| `src/pipeline/q2_census.hpp` | 147 | `903f5ee92d72e0131096deefe04c51ba5f18000b214ee09276fa359b3972b8f7` |
| `src/pipeline/q2_census_parallel.cpp` | 189 | `6f2f7f9fb5fb4d063b6487013d221f32fd3e5dec8430b377eb5c119d0fa9bbaa` |
| `src/pipeline/q2_census_parallel.hpp` | 71 | `585152695dd9f98af66c698f817895fa12ed000533ed84662d4b71c407e2e98e` |
| `src/pipeline/q2_census_resume.hpp` | 185 | `e71d26f37b26aaf935d4c1241f1a005e07d74e8e63a87176aaf96623578946c6` |
| `src/pipeline/q2_joint_bounds.hpp` | 91 | `c1e383e501e2c02a6dad052401c0251c889c7aacdd9df6c34d2307656a40935c` |
| `src/pipeline/q2_node_pool.hpp` | 263 | `49c989b057f0ee84648dbb8bcc78a94ffc9870c9d93a916ec886a6b23228aa9a` |
| `src/pipeline/tube_credits.hpp` | 244 | `146fda33096ff0eafcfe889f06e4d26f600a555e11f46e0c7daa8b54c3005b8b` |
| `src/pipeline/wspd_q2_batched.hpp` | 108 | `50e57789ed4f8eb586dc702d710c104de83eadab54f92551cafca5cd7273e51d` |
| `src/pipeline/wspd_q2_census.hpp` | 144 | `23dc8751e9f0775763a3c9882240cdf6650d2d86e87686773df7966618b36ae8` |
| `src/pipeline/wspd_q2_cooperative.hpp` | 112 | `dd1069355d06a9603750d3118d00fd41cb5aa3d955ee31e6babbe2774e8a7041` |
| `src/pipeline/wspd_q2_parallel.hpp` | 86 | `af975aeff41c6d284320626fdee34a12ce999c885a929e7eb547b193fd736c75` |
| `src/pipeline/wspd_q2_ranges.hpp` | 111 | `ea540220ced7bd3aa92686fc098896f02b403f633ed786834582b77a882bb627` |
| `src/pipeline/wspd_q34.cpp` | 877 | `79ae04fe505671ab2ebbb15d7b2b546a9beb4a3fd427f8df0af40670b318084e` |
| `src/pipeline/wspd_q34.hpp` | 232 | `ac9840848a3e6eeaf32f659a17f5cce9385f1b4b3c28470146396a61bb5a3483` |
| `src/spatial/float32_index.cpp` | 196 | `2b602f47e72e088ce8187154c80849beeb451833ed15279e4d79b2e773adcf4c` |
| `src/spatial/float32_index.hpp` | 117 | `4137b8129fafdb6cc893d32f3969c1ab15eef0cc016a02170913cc1ba1a6a6f5` |
| `src/spindle/predicates.hpp` | 251 | `8ea1509a309ca83b63a766728a929f5a4f8cb68a2f44ca0804756eee540ba088` |
| `src/spindle/q2_prepared_bounds.hpp` | 94 | `022c141855e0f8fcd8f256cf7d88e7b4b1b6bc22756b7aad591f5cf3c83fa7de` |
| `src/wspd/front.cpp` | 762 | `4ce25409737c48fb61707d2d6078478a7265e8501b594cf8d0057b280fc8bbec` |
| `src/wspd/front.hpp` | 322 | `5921a333d7c7e4c6667420f150cbd0d42a5b0cb958e2a40d2c1acf0d25728696` |
