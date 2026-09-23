# Lentille 9 — pistes fermées, invariants, préparation parallèle et GPU

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=audit_independant (lecture seule)
public_status=not_claimed
GCP non utilisé.
```

**Base.** Worktree détaché `0125dc18`. Tous les chemins sont relatifs à la racine du dépôt. Les compléments postérieurs ont été lus en lecture seule dans `origin/main` `87ccf5fc` (sonde v12, reçu `lidar_scaling_local_20260923`) et sont marqués « post-pin ». Deux exécutions seulement, toutes deux légères :

- des recalculs Python sur les JSON des reçus ;
- un micro-banc de création de fils : 2 fils, `nice -n 19`, moins d'une seconde de CPU.

**Statuts employés.**

- **prouvé** : théorème ou lemme écrit.
- **testé** : porte bornée.
- **mesuré** : reçu épinglé.
- **indication** : reçus non appariés (autre hôte ou autre binaire).
- **estimation** : calcul d'audit.
- **supposé** : hypothèse non vérifiée.

## 1. Pistes fermées ou interdites (a)

**Règle de réouverture.** Une piste fermée ne revient qu'avec trois éléments : un nouveau théorème de complétude, une fixture qui falsifie le motif d'abandon, et une porte de coût distincte. Un benchmark ne suffit jamais. Sources : `CLAUDE.md:19`, `docs/archive/abandoned/README.md:20`, `morsehgp3D_v3/audits/PISTES_FERMEES.md:71-75`.

**Force d'une fermeture** (`morsehgp3D_v9/docs/FAUSSES_PISTES.md:3-17`) :

- une **preuve** ou une **fixture** est définitive ;
- une **mesure**, un **modèle** ou une **consigne** est révisable ;
- une fermeture « non épinglée » reste une présomption.

### 1.1 Architecture interdite (objet)

| Piste | Cause | Ce qui survit | Référence |
|---|---|---|---|
| Mosaïque de Delaunay d'ordre supérieur, Γ global, catalogue cellulaire ∝ C(n,k) comme produit | Matérialise les cellules, cofaces et incidences à éviter (interdit d'architecture) | Oracles exhaustifs bornés `reference/` | `docs/archive/abandoned/README.md:14` ; `CLAUDE.md:19` ; `morsehgp3D_v6/docs/PISTES_FERMEES.md:41-42` |
| Rhomboid tiling (bifiltration multicouverture) | Structure plus riche que H0, difficile à diffuser sur GPU. C'est une variante de la mosaïque d'ordre supérieur. « Ombre H0 » n'est qu'une interprétation | Référence de complexité seulement | `docs/math/CATALOGUE_CRITIQUE_3D.md:388` ; `docs/math/TOUR_BOULES_SATUREES.md:171` |
| Tour globale de boules saturées énumérée, O(n^4) supports | No-go à n=50 000 | Repli de preuve borné | `docs/archive/abandoned/README.md:15` ; `TOUR_BOULES_SATUREES.md:160-178` |
| Parcours global de l'arrangement relevé (BFS/GPU) | Énoncés faux hors position simple ; volume quadratique | `order_k_flats` local | `morsehgp3D_v3/audits/PISTES_FERMEES.md:58` ; v6 `PISTES_FERMEES.md:37-40` |
| Fold v4 (union des facettes émises) pris pour FULL | `false_in_general` : la fixture E5 crée une fausse naissance puis une fausse fusion | Fixture E5 | v9 `FAUSSES_PISTES.md:23` ; v3 `PISTES_FERMEES.md:53` |
| MST de mutual reachability sur les points ; fermeture par les paires de rang utile | Perd l'identité des simplexes ; triangle aigu dont les trois côtés sont de rang supérieur | Fixtures | `abandoned/README.md:8-9` |
| Minima Gabriel avec leurs seules adjacences induites | Deux fixtures à quatre points | Chemins datés | v9 `FAUSSES_PISTES.md:24` |

### 1.2 Localité, halos, fenêtres

| Piste | Cause | Survit | Référence |
|---|---|---|---|
| **Halo k-NN** et tuiles indépendantes | Un halo ne rend pas les tuiles indépendantes. Une coquille peut porter Θ(n) labels. Les lots de même niveau exigent une fusion globale | Index résident, streaming des supports | `morsehgp3D_v5/docs/ECHELLE.md:47-51` |
| Tuilage spatial du fold avec halo R_max | Halo dépendant des données : rayon jusqu'à 23 % du domaine sur `eight_clusters`, lecture ×3 à ×27 | Comptage première/dernière incidence | v5 `ECHELLE.md:121-137` ; v6 `ECHELLE.md:127-130` |
| Fenêtre Morton fixe ou préfixe k-NN comme autorité | Rang de voisinage non borné (fixture à 50 000 points) | Morton comme clé de tri | `abandoned/README.md:7` ; v5 `PISTES_FERMEES.md:9-12` |
| Seau Morton comme autorité d'unicité ; empreinte probabiliste | Famille cosphérique non bornée ; comptage exact exigé | Clé complète | v6 `ECHELLE.md:131-138` |
| Chambres Yao-48, cône par endpoint, fenêtre top-M | Chambre de 54,74° contre les < 35,26° exigés ; faux verts | Noyau ponctuel H>0 ; fixture δ²=4R² | v3 `PISTES_FERMEES.md:16-19,57` |

### 1.3 Numérique

| Piste | Cause | Survit | Référence |
|---|---|---|---|
| **Jitter** ; refus des coquilles pour rester en position générale | Change les composantes HGP (carré cocyclique) | Quotient de plateau exact ou refus explicite | `CLAUDE.md:117` ; v5 `PISTES_FERMEES.md:71-73` |
| « Arrondir le rayon d'une unité vers le haut est sûr » | Deux fixtures q3/q4 à norme irrationnelle | Arithmétique dirigée, comparaisons strictes | v5 `PISTES_FERMEES.md:22-25` |
| **Flottant non certifié** comme décision (PDEL/Geogram binary64 ; grille, DTM, ANN comme définition) | Décisions partielles ; filtration modifiée | Filtre certifié avec repli exact | `abandoned/README.md:11,16` |
| Voie float32 1 728 bits pour le contrat de temps ; filtre flottant supposé gratuit | ×20 à ×70 plus lent par prédicat ; consigne du 22 septembre | Voie dormante | v9 `FAUSSES_PISTES.md:51` ; `docs/audit_v8/12_parallelisme_gpu_perf.md:247` |
| Pas u16 réduit ; D en u32 ; scale·x ; carte Q=2^44 | Collisions et débordements à 18 bits | Bornes u18 vérifiées | v9 `FAUSSES_PISTES.md:46-50` |

### 1.4 Grilles alignées, tubes, capteur

- **Tubes comme chemin général de crédits.** Aucun crédit sur nuage irrégulier (7 818 cellules pour 8 060 sites). Référence : v9 `FAUSSES_PISTES.md:63`.
- **Filtre axial et colonnes.** Une rotation entière supprime toutes les colonnes exploitables : 256 M paires conservées. Références : v9 `FAUSSES_PISTES.md:62` ; v8 `FAUSSES_PISTES.md:265-286`.
- **Sélection axiale bornée (v4).** Opt-in négatif : +7 % de `t_gen`. Elle ne se rouvre sur GPU qu'avec un kernel mesuré sur G4. Référence : v5 `PISTES_FERMEES.md:42-47`.
- **Coupes capteur prises pour une validation.** Elles restent un diagnostic, jamais le contrat. Consigne utilisateur : aucune hypothèse d'alignement entre passages LiDAR. Références : `AGENTS.md:84-99` ; `morsehgp3D_v9/audits/ETAT_COURANT.md:31-35`.

### 1.5 Filtres par ligne, palettes, tests avant recherche

- **Test de lentille avant recherche.** Évite seulement 0,4 à 1 % des recherches. Référence : v9 `FAUSSES_PISTES.md:67`.
- **Filtre de ligne a×B** (`filter_q34_witnesses(singleton(a), box(B))`). Il rejette 15 à 17 M paires, mais coûte 49 à 87 G cycles : le CPU total passe de 156 à 164–171 s. Résultat **négatif mais non épinglé** (prototype hors dépôt). Références : `audits/COORDINATION_MORSEHGP3D_V9.md:375-385` ; `ETAT_COURANT.md:257-260`.
- **Palettes H_a (proches puis octant).** Elles restent en SHADOW. Le prototype du développeur donne −6 % de CPU q3/q4 et reste « en réserve ». Références : `COORDINATION…:626-631` ; `ETAT_COURANT.md:271-294`.
- **Proposeur plus large pour les rangées.** Pas de témoin universel sur les paires transversales. Référence : v8 `FAUSSES_PISTES.md:391`.

### 1.6 Budgets et plafonds

- **Cap de population dans le critère terminal WSPD.** Force au moins C(n,2)/C² rectangles. Référence : v5 `PISTES_FERMEES.md:13-16`.
- **Budget de nœuds du filtre de paires.** À 64 nœuds, on perd 1,25 M rejets pour environ 67 M nœuds gagnés. Négatif, **non épinglé**. Référence : `COORDINATION…:783-789`.
- **Quota de quatre millions de MEB.** Garde-fou d'essai, pas une nécessité mathématique. Référence : v7 `FAUSSES_PISTES.md:47`.
- **Descente plafonnée à K ou pivot du parent réutilisé.** De ×0,94 à ×6,00. Référence : v9 `FAUSSES_PISTES.md:69`.
- **Pile réduite à 8.** Un peigne Morton atteint 49. Référence : v7 `FAUSSES_PISTES.md:21`.
- **Coquille ≤ 12 prise pour un théorème.** C'est un refus de domaine, pas un théorème. Références : v9 `FAUSSES_PISTES.md:40` ; `CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md:60`.

### 1.7 Parallélisme et GPU (fermetures antérieures)

- **Jobs Coarse comme seule unité** : 1,9 à 11,1 CPU occupés sur 48.
- **Ajouter des fils sans réduire le travail.**
- **Donate, coopératif, lots de singletons.**
- **GPU par petits lots synchrones avec reconstruction hôte** : 189 ms de kernels pour 4,54 s de phase en v7 ; 154 ms pour 7 717 ms en v6.
- **« ×20 » par découplage des ordres** : plafond réel 11,4×.
- **Copier dix fois le constructeur.**

Référence de ce bloc : v9 `FAUSSES_PISTES.md:96-103`.

- **Reduce du fold sur GPU (F0).** Les racines union-find dépendent de l'historique ; ECL-CC ne préserve ni les lots ni les racines. Référence : `morsehgp3D_v6/docs/GPU.md:27-38`.
- **Prune-only piloté par l'hôte.** 3,229 s à cause des vagues. Référence : `abandoned/README.md:12`.
- **Parcours stackless relancé par paire ou par ancre.** Référence : `abandoned/README.md:13`.
- **Lien NVCC avec avertissements hôte/device pris pour une qualification.** Référence : v7 `FAUSSES_PISTES.md:16`.

### 1.8 Fermetures de l'ère v9, absentes de `docs/FAUSSES_PISTES.md`

Ce fichier n'a pas changé depuis `c399808e` (22 septembre). Les fermetures suivantes n'y figurent pas :

- scission naïve de rectangles (`Q34_BLOCS_LIDAR_SHADOW_20260923.md`) ;
- variante « têtes physiques » de la phase A : pas de gain, 54 à 159 Mio de RSS en plus (`ETAT_COURANT.md:484-490`) ;
- cover complet puis filtrage du cœur : 1,63 G contre 115 M incidences (`ETAT_COURANT.md:238-250`) ;
- borne défavorable de la reprise du cover depuis le cœur (`ETAT_COURANT.md:170-179`) ;
- les fermetures des § 1.5 et 1.6 marquées non épinglées.

**Réouverture conforme observée.** Le MEB proposé (Welzl à base ≤ 4, vérifié exactement, canonisé au bord, avec repli) rouvre légitimement les fermetures v7 `FAUSSES_PISTES.md:14` et v9 `FAUSSES_PISTES.md:101`. Il apporte un théorème (unicité du MEB) et une porte différentielle sur 34 957 ensembles (`COORDINATION…:748-760`).

## 2. Invariants qui contraignent toute implémentation alternative (b)

| Invariant | Source exacte | Garde dans la v9 |
|---|---|---|
| Décision entière exacte ; flottant = proposition ou filtre certifié à repli, jamais `-ffast-math` | `CLAUDE.md:13,117` ; `AGENTS.md:550-551` | `tower/lanes/level.hpp:68-89` (#error `__FAST_MATH__`) ; `full_ball_tower.hpp:974` (`FE_TONEAREST`) ; `anchor_meb.hpp:12-18` (« the double values decide nothing ») |
| Aucune mosaïque d'ordre supérieur ni catalogue ∝ C(n,k) | `CLAUDE.md:19` ; `AGENTS.md:545` ; `abandoned/README.md:14` | Catalogue = boules émises de rang ≤ K+1 (120 à 138 boules par site à K10, `CONTRAT_COUTS_ET_PARALLELISATION.md:85`) |
| s ∈ {8, 10, 12}, jamais s < 8 | `AGENTS.md:24-26` ; `PLAN_V9.md:17-19` | `tower_chain.cpp:291` seulement ; `gen/wspd/front.cpp:395` accepte s ≥ 1 |
| Kmax ≤ 10 | `CLAUDE.md:117` | `tower_chain.cpp:290` ; `front.cpp:395` |
| Statuts transactionnels, jamais de préfixe publié | `CLAUDE.md:117` ; `PLAN_V9.md:109-111` | `tower_chain.cpp:24-33,556-583` (résultat vidé en cas d'échec) ; refus des coquilles > 12 (`:478-481,519-522`) |
| `complete_relative` ≠ exact | `CLAUDE.md:13` ; `ETAT_COURANT.md:37-48` | Libellé `complete_relative_to_cross_checked_catalogue` (`tower_chain.cpp:557`) |
| Sorties bit-identiques quel que soit le nombre de fils ; parallélisme **mesuré** | `CLAUDE.md:135` ; `tower/parallel/pool.hpp:1-7` | Digest égal entre W24 et W48 (R1, R2, R3, R5 : vérifié par script) ; publication des ouvriers absente (constat F9-02) |
| Tailles d'intérêt 8k/16k/32k ; petites tailles réservées aux oracles | `CLAUDE.md:15` ; `docs/TEST_PLAN_MORSEHGP3D.md:142-160` | Aucune pente dans R7b ; campagne locale v12 post-pin |
| Pas de vérification exhaustive à l'échelle | `CLAUDE.md:16` ; `TEST_PLAN…:161-176` | Le recensus de toutes les clés en est une (constat F9-05) |
| Trame entière ; coupes = diagnostic ; aucun octet KITTI | `AGENTS.md:78-99` | Manifestes seuls |
| Reçu immuable pour toute mesure | `PLAN_V9.md:58-66` ; `CLAUDE.md:132` | SHA256SUMS des reçus G4 ; les mesures de harnais du canal sont hors dépôt |
| GCP par les scripts gardés seulement | `AGENTS.md:521-541` | `gcp-migration/tower_*_v9.py` |
| Borne de sortie Ω(N²) à K fixé | `HERITAGE_V7_V8.md:21` | Contrat exprimé en surcoût au-delà de la sortie |

## 3. Parallélisme CPU réel (c)

### 3.1 Topologie de la chaîne

`run_tower_chain` (`src/chain/tower_chain.cpp:285-603`) enchaîne des étages séparés par des barrières. L'étage suivant ne démarre qu'à la fin du précédent :

1. `prepare` puis index du générateur, en un seul fil (`:302-312`) ;
2. q2 : `run_joined_workers`, équipe Coarse (`:334-338`) ;
3. q3/q4 : file unique sous mutex (`gen/pipeline/wspd_q34.cpp:842-940`) ;
4. fusion : tri d'échantillonnage (`:88-153`) ;
5. index de la tour, en un seul fil (`:434-446`) ;
6. recensus : `parallel_for` à grain 256 (`:461-507`) ;
7. tour (`:529-535`).

À l'intérieur de la tour :

- la **phase 0** (`prepare_static_order`) s'exécute un ordre K après l'autre, chacune étant parallèle à l'intérieur (`full_ball_tower.hpp:459-467`) ;
- la **phase A** n'est parallèle qu'**entre** les ordres, donc sur au plus `kmax` fils (`:483-485`), et séquentielle niveau par niveau dans chaque ordre (`order_lots`, `:657-667`) ;
- la **phase B** commence par un parcours séquentiel de toutes les contributions (`:679-691`) ;
- la **phase C** est parallèle entre les ordres (`:492`).

### 3.2 Mesures

**Occupance (CPU·s / mur, recalculée).**

- R7b, W48 : 19,2 à 24,1 à K5 et 24,3 à 28,0 à K10, sur 48 fils logiques (24 cœurs SMT2, `vm/lscpu.stdout`).
- Campagne locale v12 (post-pin), W8 : 5,64 à 6,57 sur 8.

**W24 → W48 sur le même paquet G4** (seule efficacité mesurable sur une même machine) :

| Reçu | Chaîne | q3/q4 | Tour | q2 | Recensus | Fusion | CPU·s |
|---|---|---|---|---|---|---|---|
| R3 `b4e480fc`, 000000/K10 | ×1,10 | ×1,27 | ×1,02 | ×1,27 | ×1,21 | ×1,00–1,02 | ×1,44 |
| R5 `aae9da0e`, 000000/K10 | ×1,24–1,27 | ×1,42–1,45 | ×1,10–1,12 | ×1,24–1,28 | ×1,21 | ×1,02 | ×1,42 |
| R2 `0b29b6c3` (refusé, bruts), K10 | ×1,12–1,14 | ×1,22–1,25 | ×1,02–1,03 | ×1,23–1,27 | ×1,16–1,20 | ≈1 | ×1,54–1,59 |

Les digests W24 et W48 sont égaux. Il n'existe ni ligne W1 v9 sur G4, ni paire W8/W48 sur un même hôte.

**Indication, local W8 v12 → G4 W48 R7b** (hôtes et binaires différents), à K10 :

- recensus : ×8,6 à ×9,6 ;
- q3/q4 : ×7,1 à ×8,7 ;
- chaîne : ×6,2 à ×6,9 ;
- **tour : ×3,7 à ×4,7 seulement** ;
- à K5, la tour ne gagne que ×2,6.

**Lecture.** Le générateur et le recensus passent à l'échelle. La tour plafonne dès 24 fils. Le SMT coûte +42 à +59 % de CPU·s pour +22 à +45 % sur q3/q4.

**Plancher de CPU.** Même avec un partage idéal sur 48 fils, il reste 1,81 s (K5) et 5,58 s (K10) de CPU·s/48 au meilleur R7b (recalcul ; comparer `ETAT_COURANT.md:163-168`).

### 3.3 Surcoûts fixes invisibles aux phases

**Fils créés à chaque appel.** `pool.hpp` n'est pas un pool :

- `run_threads` crée `count` objets `std::thread` et n'admet l'équipe qu'une fois tous les fils créés (`:56-87`) ;
- `parallel_sort` lance trois équipes (`:203,220,224`) ;
- `prepare_static_order` en lance dix par ordre (`:1248,1281,1293,1302,1310,1371`).

Cela fait environ 90 équipes pour K=2..10, et de l'ordre de 100 par chaîne K10. Le micro-banc local donne 49 à 69 µs par création et jointure d'un fil, sur un hôte chargé. **Estimation : 0,1 à 0,3 s par chaîne K10**, non mesurée sur G4.

**Temps hors phase.** `chain_total − Σ phases` vaut 182 à 430 ms à K10 et 12 à 98 ms à K5 sur les 24 cas R7b. L'origine supposée est la destruction du catalogue (0,98 à 1,23 Go) et de l'index.

### 3.4 Ce que la sonde ne publie pas

La chaîne jette `r34.parallel`, `workers`, `worker_timings` et `tasks` (seul `pipeline.work` est lu, `tower_chain.cpp:375-416`). `tower_probe.cpp` ne publie ni `static_workers_created`, ni `static_lanes_used`, ni `parallel_orders`. Il n'y a ni CPU par phase, ni temps de verrou. C'est encore vrai en v12. Le chemin critique n'est donc pas localisable (constat F9-02).

## 4. Préparation GPU (c)

**État v9.**

- Aucun fichier `.cu` ou `.cuh`. `CMakeLists.txt:6` déclare `project(morsehgp3d_v9 CXX)` et aucune option CUDA.
- `GPU_executed=false` en dur dans `gcp-migration/tower_worker_v9.py:630,843` et dans le reçu R7b (`PACKAGE.json:4`).
- `MHGP9_HD` (`tower/core/device.hpp`) annote 31 primitives de la tour : `wide`, `keys`, `level`, q2/q3/q4. Elles n'ont jamais été compilées par nvcc.
- Le générateur (`src/gen`), qui pèse 55 à 64 % de la chaîne K10 et 69 à 74 % à K5 dans R7b, n'a **aucune** annotation. Il utilise `std::function` par candidat (`gen/lanes/q34_seed.hpp:39`), des bornes i128 (`q3_ball_census.cpp:13-23`) et des arêtes atomiques.
- Aucun job CI TSan ou nvcc (`.github/workflows/morsehgp3d-v9.yml`).

**Héritage mesuré sur G4.**

- **v6, série C** (préfiltre et census, uniforme 50k u16) : étage de 7 717 ms dont 154 ms de noyaux. 88 % du temps est hôte (sérialisation 2 641 ms, reconstruction 4 110 ms). Gain de mur −10,4 % au mieux (`v6 GPU.md:220` ; `ECHELLE.md:141-150`). C6 (baux `lot_ring`) n'a jamais été mesuré ; son WIP n'est pas commis et est défectueux (`CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md`).
- **v7, census CUDA** : 189,346 ms de kernels pour une phase de 4,540 s. Tour identique au CPU (418,9 s). Porte Blackwell : 16 627 contrôles (`RESULTATS_TOUR_CACHE_G4_20260910.md:32-39`).
- **v7, primitives** : MEB sur 605 cas, clé/PGCD/division 128 bits sur 13 573 cas, « aucun facteur d'accélération » (`RESULTATS_PRIMITIVES_GPU_20260911.md:18-19,44`).

**Écarts à combler avant un port u18.**

- Gardes de clé u16 → u18 : A < 2^76, B < 2^96, C < 2^116. Les comparateurs U192/U320 tiennent (`CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md:60-61`).
- La borne de pile 49 venait des clés Morton 48 bits (v7 `FAUSSES_PISTES.md:21`). La tour v9 utilise 54 bits (`tower/core/morton.hpp:26`) et une pile dynamique (`tower/pipeline/census.hpp:174-206`). La borne est à re-dériver.

**Obstacles chiffrés** (`OBSTACLES_GPU_SOUS_SECONDE_20260923.md:21-60`) :

- q3/q4 gratuit laisserait 4,35 s à K10 ;
- q3/q4 et FULL gratuits laisseraient encore 1,154 s ;
- incidences cœur+cover d'environ 0,9 à 1,12·n² ;
- 2 081 320 lots singletons à K10 (`PHASE_A_MAX_ID_COMPOSANTE_20260923.md:118`), qui rendent une barrière GPU par niveau non viable.

**Lien utile.** La fermeture v6 F0 venait de racines union-find dépendantes de l'historique. Le lemme du maximum d'ID (`PHASE_A_MAX_ID…`) rend l'ID canonique indépendant de l'ordre des unions, à plateau fermé. C'est la nature de théorème qu'exigeait F0 pour une réouverture. Il n'est ni porté ni testé sur le produit, et l'objet diffère du fold v6.

## 5. Leviers déjà proposés par A et B, avec leur statut (d)

| Levier | Référence | Statut |
|---|---|---|
| Census q3 sur feuille exacte ; saturation à K−1 | PASSATION l.56-61 ; `CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI` | porté, ON ; gain net non isolé |
| Voies mortes ; cache de nœuds témoins ; noyau diamétral | PASSATION l.73-126 | portés ; mesurés R3 (q3/q4 ÷2 à ÷3), R4b (CPU −5 à −19 %), R6 (CPU −8 à −21 %) |
| Seuil K−2 pour q4 seule, `retain_q3_fragments` coupé | `SEUIL_SATURATION_ATLAS_PAR_VOIE` | exact, **non porté** |
| Domination q4 par blocs | `DOMINATION_Q4_…` ; `CONTRE_AUDIT_B_DOMINATION` | sûr, non porté |
| Nœuds avant cover ; rectangles avant expansion | `PISTE_B_Q34_*` | proposés ; variante ligne négative (non épinglée) |
| Palettes H_a et octant | `SHADOW_HA_*` | shadow, en réserve |
| Scission de rectangles ; cover complet puis filtre | `Q34_BLOCS_…` ; ETAT l.238-250 | négatifs |
| Cover commun ; ticket d'antichaîne ; q3 partagé par cellule | `CONTRE_AUDIT_B_COVER_BATCH` ; `CACHE_TEMOINS_…` ; ETAT l.314-326 | proposés |
| Certificats k-Gabriel locaux ; niveaux orientés q4 | `AUDIT_A_ARCHITECTURE_K_GABRIEL` ; `CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES` | proposés ; invariant de génération à prouver |
| Filtres flottants certifiés (q3, atlas) | PLAN_V9 l.182-184 | rien |
| Sample-sort de fusion ; séparateurs pseudo-aléatoires ; tri unique | `CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE` ; `AUDIT_DISTRIBUTION_…` | porté (R7b : fusion 0,03 à 0,12 s) ; porté sans reçu G4 ; shadow |
| MEB proposé ; Welzl move-to-front ; index des clés | ETAT l.404-453 | porté (R7b : tour −5 à −8,4 %) ; les deux autres sans reçu G4 |
| Ordres K parallèles | PASSATION l.93-98 | porté (R5) |
| Préfixe d'intrus ; prefetch par K | `INTRUS_…` ; `PREFETCH_…` | lemmes sûrs, non portés |
| Phase A : pré-niveau figé, graphe temporel, max-ID | `PHASE_A_*` | proposés ; variante « têtes physiques » négative |
| Grandes coquilles sans table 2^u ; sortie adressable | `PLATEAUX_GRANDES_COQUILLES_B` ; ETAT l.368-383 | proposés |
| Plan plat, pool persistant, vol de travail, arêtes scindées | PLAN_V9 l.198-221 ; CONTRAT_COUTS l.354 | **non faits** |
| Temps par worker publiés | `CONTRE_AUDIT_A_MESURES_PLAN…:166-184` | non fait, y compris en v12 |

## 6. Conséquences pour l'étude d'alternatives

Une alternative crédible doit remplir quatre conditions :

1. Éviter les motifs fermés du § 1 : halo, tuiles, fenêtre-autorité, flottant décisionnel, union-find GPU sans théorème.
2. Réduire le travail avant l'expansion : CPU·s/48 ≥ 5,6 s à K10.
3. Rendre la phase A parallèle **à l'intérieur** d'un ordre.
4. Supprimer les surcoûts fixes (fils, destruction) avant toute ambition à 100 ms.

Toute mesure doit publier le CPU par phase et par ouvrier.