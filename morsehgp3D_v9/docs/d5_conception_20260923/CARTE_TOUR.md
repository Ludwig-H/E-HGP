## Cadre

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=tranche_verticale_v9_1 (carte de code, lecture seule)
public_status=not_claimed
```
GCP non utilisé. Je n'ai rien compilé ni modifié. Les chemins cités sont relatifs à `morsehgp3D_v9/` ; l'en-tête `src/tower/forest/full_ball_tower.hpp` est abrégé **FBT**.

**Route produit.** `src/chain/tower_chain.cpp:928-932` appelle `build_full_ball_tower(ix, balls, kmax, static_threads = W, {}, tower_meb_proposal = true, tower_overlap_static = true)`. Le résolveur par lots est vide. Sur G4, W = 48, ce qui mène à `Builder::run` (FBT:356), `run_orders_parallel` (FBT:366, 501) puis `run_orders_overlapped` (FBT:504, 576-667).

---

## 1. Les phases et ce que chacune calcule

Mesures de `receipts/g4_tower_r13_20260923/SUMMARY.json`, probe_0 (08/000000, K5, W48) et probe_2 (K10).

| Phase (champ `FullBallTimes`, FBT:118-122) | Code | Calcul | K5 / K10 (ms) |
|---|---|---|---|
| validate | `validate_catalogue` FBT:1040-1218, chronométrée FBT:357-359 | Voir détail (a) | 86 / 339 |
| static (phase 0) | `prepare_static_order` FBT:1453-1597 et `static_terminal` FBT:1266-1319 ; boucle par K décroissant FBT:619-633 | Une cible `BallId` pour chaque facette de K ≥ 2 | 256 / 1564 (somme de `static_by_k`) |
| lots (phase A) | `order_lots` FBT:806-837, `order_block` 705-730, `order_lot` 732-804, `order_root` 684-693, `order_new_node` 695-703 | Voir détail (b) | 226 / 340 (`lots_ms` = fenêtre − static, FBT:637) |
| populations (phase B) | `assign_populations` FBT:839-865 | Voir détail (c) | 32 / 159 |
| images (phase C) | `order_images` FBT:867-894, `MonotoneHistory` 191-264 | Voir détail (d) | 87 / 337 |
| bank | `finish` FBT:438-446, `build_full_coverage_populations` | Banque immuable partagée par les K ordres | 12 / 97 |
| encode | `finish` FBT:452-468, `build_full_coverage_certificate` (`full_coverage_certificate.hpp:206-313`) | Voir détail (e) | 45 / 190 |

**Détail des phases.**

- **(a) validate.** Elle se fait en trois temps :
  - Validation de l'entrée et de l'identité `domain` (PointId triés), puis tri `by_key`, ou certification « présortie » par un seul balayage (FBT:1061-1068).
  - Construction de l'index de clés exact `build_key_index`, à adressage ouvert et CAS (FBT:932-953). La recherche se fait par `find_key` (FBT:955-961).
  - **Passe 1**, parallèle, `check_ball_locally` (FBT:985-1012) : forme, bornes u18 de la clé, puissances intérieur < 0 et coquille = 0, support déclaré des boules régulières (FBT:1014-1038).
  - **Passe 2** : fenêtre de rang et comptes. Les coquilles étendues sont traitées en série : `ShellTable::prepare`, q_min, support minimal, et une MEB témoin payée dans `st.validation_work` (FBT:1130-1152).
  - Tri `by_level` : filtre double certifié puis U320 exact, égalités départagées par le rang de clé (FBT:1162-1182).
  - `programs[K]` = boules telles que p+q−1 ≤ K ≤ min(Kmax, p+u), dans l'ordre `by_level` (FBT:1185-1216).
- **(b) lots.** Pour chaque ordre K, sur son propre fil : lots (suites de niveaux exacts égaux), blocs, racines pré-lot, actions (naissance, continuation, fusion), ancres par boule et brouillon (`Draft`). Les contributions nomment la **boule**, `kBallTag|ball` (FBT:742, 782). Aucune image verticale n'est calculée à ce stade.
- **(c) populations.** Attribution des IDs dans l'ordre séquentiel : singletons du domaine, puis ordre K, lot, action, contribution, au premier rencontré. Les lignes sont ensuite remplies en parallèle.
- **(d) images.** Pour chaque nœud créé dans l'ordre K, on calcule son image dans l'ordre K−1 par des coupes fermées monotones :
  - naissance : `root_at(lower->anchors[birth_ball], λ, closed)` ;
  - fusion : racine de `lower_nodes[parent]`, avec contrôle de naturalité sur tous les parents (FBT:879-887).
- **(e) encode.** La forêt `FullCoverageCertificate` de chaque K est construite en parallèle. Les IDs de nœuds suivent l'ordre des actions, les continuations étant sautées (contrôle FBT:466).

**Chemins séquentiels.** Avec W ≤ 1, `run()` exécute la boucle FBT:370-417 :
- `prepare_block` (FBT:1722) ;
- puis `resolve_static` si `geometry_threads` > 0 (FBT:1606), ou `resolve` pour la voie temporelle (FBT:1610) ;
- `close_lot` (FBT:1760-1851) ;
- `new_node` (FBT:1744), qui calcule l'image en ligne.

Ces chemins produisent les mêmes objets.

---

## 2. Résolution d'une facette : cible terminale, puis racine

### Structures

- **Facette.** `ResolverCache::Key` = `std::array<i32, kFacetMaxK=10>` (FBT:286). Ses K sites sont des **index de positions uniques** dans `ix.upos`, pas des PointId. Le préfixe est trié et sans doublon (FBT:1473-1476).
- **Émission.** `visit_block_at(k, ball)` (FBT:1680-1721) :
  - boule régulière à K = p+u : aucune facette, contribution = coquille pleine, `interior` = (p ≠ 0) ;
  - boule régulière à K = p+u−1 : une facette I ∪ U∖{s} par site de coquille ;
  - coquille étendue : une facette représentante par composante stricte de `ShellTable::rank(K)` (`local_plateau.hpp:100-178`).

  `rank` est recalculé en phase 0 et à nouveau en phase A.
- **Graines.** `FullBallBatchSeed{key, ball}` pour chaque boule telle que K = p+u : clé = I ∪ U triée (FBT:1480-1487). Les clés sont triées et doivent être distinctes (FBT:1519-1521).
- **Requêtes.** `FullBallBatchRequest{key, consumer, ordinal}` (FBT:80-84). L'`ordinal` est la position dans la collecte séquentielle, c'est-à-dire programme × ordre de `visit_block`. La concaténation se fait aux offsets (FBT:1492-1504).

### Phase 0 (par K ≥ 2) : `prepare_static_order` FBT:1453-1597

1. Collecte parallèle par blocs de 4096 positions de programme (FBT:1461-1489). Compteur `static_requests[K]`.
2. `parallel_sort` des requêtes sur (key, ordinal), qui est un ordre total strict (FBT:1510-1513). Découpage en groupes de clés égales (FBT:1523-1537). Compteur `static_unique[K]`.
3. `static_targets.assign(requests.size(), max)` (FBT:1539). Si un `batch_resolver` existe, on passe par `prepare_external_batch` (FBT:1542-1545) ; ce n'est pas la route produit.
4. `job` (FBT:1566-1586) pour chaque groupe :
   - `before` = niveau du **premier** consommateur, celui d'ordinal minimal ;
   - si la clé est une graine, `target = seed.ball`, avec `level < before` exigé (`full_ball_static_seed_not_strict`) et `seeded++` ;
   - sinon `target = static_terminal(key, before, w.work, w.scratch, seeds)` ;
   - tous les ordinaux du groupe reçoivent `target`, avec `before ≤ level(consumer)` exigé (`full_ball_static_request_chronology`, FBT:1581).
5. `parallel_ranges` sur les groupes (FBT:1588). Les compteurs de chaque ouvrier sont fusionnés par `account` et `merge_static_work` (FBT:1548-1565, 1321-1342).

### `static_terminal` (FBT:1266-1319) : la descente par échange d'intrus

La fonction est `const`. Elle lit seulement `ix`, `balls`, `key_slots`, `current_k` et `propose_meb`.

```
D = meb(F)                                   // anchor_meb_proposed, FBT:970-981
loop:
  require level(D) < before                  // full_ball_static_not_strict
  key_lookups++; T = find_key(D.key)
  if T found: require same_exact_level(T.level, D.level)
     if p_T+q_T-1 <= K <= p_T+u_T: anchor_hits++; return T      // « règle 0 », déjà présente
  z = intruder_work(D.key, F)                // FBT:1235-1260
  require z >= 0                             // full_ball_static_missing_weak_terminal
  F[D.support_slots[0]] = z; sort(F)
  post_seed_queries[K]++
  if F is a seed: require K == p+u and level < level(D), < before
     post_seed_hits++, descending_steps++, return seed.ball     // aucune MEB payée
  D' = meb(F); require level(D') <= level(D)                    // radius_increased
  if equal: require D'.key == D.key and shell_count(D') == shell_count(D) - 1
     same_radius_steps++
  else descending_steps++
  D = D'
```

`intruder_work` renvoie le premier site strictement intérieur (puissance < 0) hors de F. Le parcours est un DFS gauche d'abord sur l'arbre radix, élagué par `census_detail::AxisBounds` (`pipeline/census.hpp:61-104`) ; une plage entièrement intérieure est rendue sans test de puissance (`interior_ranges`).

**Terminaison.** Le couple (niveau, taille de coquille sélectionnée) décroît lexicographiquement à chaque pas. Il n'y a pas de plafond de pas ; `max_chain_steps` est seulement relevé.

### Phase A : consommation (FBT:705-730, 806-837)

- **Découpage en lots.** Un lot est une suite maximale de `same_exact_level` dans `programs[K]`. On fixe `prior_count = o.current.levels.size()` **avant** le lot (FBT:824) ; c'est la coupe ouverte λ⁻.
- **Chaque facette de chaque bloc**, parcourue dans l'ordre de `visit_block_at` (identique à la collecte) :
  - `target = o.static_targets[o.static_cursor++]` ;
  - exigences : `level(target) < λ` (`full_ball_static_target_not_strict`) et `o.anchors[target] ≠ kAbsent32` (`full_ball_static_closed_anchor_missing`) ;
  - `root = order_root(o, anchors[target], prior_count)`. C'est un DSU `compressed` avec compression de chemin, et la racine doit être vivante : `next == absent` et `< prior_count` (FBT:684-693).
- **K = 1.** La facette est un site ; la racine est le singleton initial `lower_bound(domain, PointId)` (FBT:711-717).
- **Racines du bloc.** `block.roots` est trié et dédoublonné (FBT:728-729). Aucun nœud du lot n'est créé avant que tous les blocs soient préparés (FBT:825-826).
- **`order_lot`** :
  - lot singleton (FBT:736-758) ;
  - lot groupé (FBT:760-803) : DSU des blocs qui partagent une racine, puis groupes indexés par le plus petit bloc ;
  - parents = union triée et dédoublonnée des racines ; contributions dans l'ordre des blocs ;
  - 0 parent : naissance ; 1 parent : continuation, sans nœud ; 2 parents ou plus : fusion ;
  - bloc inerte = continuation sans contribution, sans action ;
  - les ancres `o.anchors[ball] = target` sont publiées **après** le lot entier (FBT:800-803).
- **Contrôles de fin d'ordre** (FBT:829-835) : curseur entièrement consommé (`full_ball_static_unconsumed_targets`) et une seule racine vivante.

### Invariant clé : le choix de la boule cible est inobservable

Pour une facette F, toute cible T qui remplit les trois conditions suivantes donne la **même** racine, pour chaque consommateur de niveau ≥ `before` :
- T appartient à `programs[K]` ;
- `level(T) < before` ;
- l'ancre de T est dans la composante de F dans L_K(before⁻).

La raison est la monotonie de la forêt. Les populations sont attribuées aux boules des **blocs**, et les images aux `birth_ball` et ancres de K−1. Ni l'une ni l'autre ne dépend de la cible. Seul le travail change.

### Autres voies

- **Voie temporelle (W = 0)** : `resolve` (FBT:1610-1657). Même descente, mais la règle 0 y est « ancre déjà posée » ; elle utilise `ResolverCache` (FBT:284-345, graines FBT:1659-1672) et n'a pas de raccourci post-graine. Elle sert de témoin différentiel dans les portes.
- **Couture externe** : `prepare_external_batch` (FBT:1344-1444).

---

## 3. Compteurs du grand livre (`FullBallStats`, FBT:39-67)

**Résolution** (écrits en phase 0 par ouvrier, fusionnés par `merge_static_work` FBT:1321-1342) :
- `key_lookups`, `anchor_hits` ;
- `intruder_queries`, `intruder_nodes`, `intruder_power_tests`, `interior_ranges` ;
- `same_radius_steps`, `descending_steps`, `max_chain_steps` ;
- `static_post_seed_{queries,hits,terminals}[K]` ;
- `resolve_work` (`AnchorMebWork`, `anchor_meb.hpp:38-51`) : `calls`, `power_tests`, `pair_distances`, `materializations`, `supports_by_size[5]`, `proposals`, `verified_proposals`, `boundary_canonicalizations`, `proposal_fallbacks`.

**Côté `prepare_static_order`** : `static_requests/unique/seeded[K]`, octets de pic, `static_lanes_used`, `static_workers_created`.

**Côté phases A et C** (via `merge_order_stats` FBT:669-682) :
- `representatives` : une par facette, K1 compris (FBT:712, 719) ;
- `anchor_blocks`, `regular_blocks`, `extra_blocks` ;
- `births`, `merges`, `contributions` ;
- `singleton_lots`, `grouped_lots`, `lot_dsu_slots`, `inert_blocks` ;
- `lower_*`.

**Voie temporelle seulement** : `resolver_cache_*` (valeur 0 en produit, car `configure` n'est appelé que si W = 0, FBT:363). **Validation** : `validation_work`.

**Identités exactes de la voie statique**, vérifiées par `tests/tower/full_ball_tower_gate.cpp:365-386` et exigées par FBT:1415-1418 :
- `resolve_work.calls == anchor_hits + intruder_queries == key_lookups` ;
- `Σ post_seed_queries == intruder_queries == descending_steps + same_radius_steps` ;
- `post_seed_hits == post_seed_terminals` ;
- `anchor_hits + Σ post_seed_terminals == Σ static_unique − Σ static_seeded`.

**Publication.** `bench/tower_probe.cpp:297-315` ne publie qu'un sous-ensemble : `representatives`, `anchor_hits`, `key_lookups`, `intruder_queries`, `intruder_nodes`, `meb_calls` (= `resolve_work.calls`), `meb_power_tests`, les champs `meb_*`, `births`, `merges`, `contributions`, `grouped_lots`, `resolver_cache_hits`. **Ne sont pas publiés** : `static_requests/unique/seeded`, `post_seed_*`, `same/descending_steps`, `max_chain_steps`, `intruder_power_tests`, `interior_ranges`.

Le schéma du worker est **exact** : `gcp-migration/tower_worker_v9.py:443-446` impose `set(value) == TOWER_WORK_KEYS`. Ajouter un champ oblige donc à mettre à jour le worker et la porte `probe_worker_contract` (61/61).

**Valeurs probe_0 (K5).**

| Compteur | Valeur |
|---|---:|
| representatives | 3 621 785 |
| meb_calls = key_lookups | 1 289 447 |
| anchor_hits | 628 672 |
| intruder_queries | 660 775 (somme avec anchor_hits = 1 289 447 ✓) |
| intruder_nodes | 31 708 173 |
| meb_power_tests | 7 799 324 |
| supports tentés, tailles 1 à 4 | [0 ; 1 289 447 ; 591 476 ; 63 376] |
| propositions vérifiées (sans repli) | 654 852 |

---

## 4. Points d'insertion d'une résolution alternative (par exemple le saut au centre)

### 4.1 Point principal : le corps de `static_terminal` (FBT:1266-1319)

Il est appelé depuis un seul site, FBT:1578, dans `job`. Il est déjà pur (`const`), sans état partagé, avec des compteurs et une pile propres à chaque ouvrier. Remplacer cet appel, derrière un levier sur le modèle de `propose_meb` (constructeur FBT:350, `meb()` FBT:976, option chaîne `tower_chain.hpp:83`), laisse **tout l'aval intact** :
- `static_targets[ordinal]` ;
- le swap vers `OrderState` (FBT:510, 625) ;
- `order_block`, `order_lot`, B, C, la banque et l'encodage.

La même phase 0 alimente aussi la voie statique séquentielle (W = 1, `resolve_static_target` FBT:1598-1605). La voie temporelle (FBT:1623-1656) doit rester l'ancienne descente : c'est le témoin.

**Contrat que la routine doit tenir (post-conditions).**
1. Elle renvoie un `BallId < balls.size()`.
2. p+q−1 ≤ K ≤ p+u pour la cible, faute de quoi l'ancre est absente et la phase A refuse.
3. `level < before`, contrôlé en aval (FBT:722-724).
4. **La cible est dans la composante de F dans L_K(before⁻).** Cette condition n'est **contrôlée nulle part** en aval. Une mauvaise composante produit de faux parents et ne se voit qu'au `same_payload`, au digest ou au juge Γ. Contre-fixture : les sites collinéaires 0, 1, 10, 11 à K2 (`CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md`).
5. `static_targets` garde les mêmes ordinaux et la même cardinalité.
6. Refus seulement par `Failure` : aucune écriture partielle, les compteurs payés restent comptés.
7. Si la routine est appelée hors de la phase 0 (par exemple plusieurs K en parallèle), elle doit prendre K explicitement au lieu de lire `current_k` (FBT:1268, 1279, 1297).

L'état `(D, F)` et la règle 0 (FBT:1273-1283) sont réutilisables tels quels. Le census de la boule fermée D existe déjà : `ball_census(ix, key, caps, &I, &U, &DepthStats, &scratch)` (`pipeline/census.hpp:174-208`, avec plafonds et statut de débordement). Pour une clé présente au catalogue, `BallData` porte I ∪ U (`ball_data.hpp:24-33`).

### 4.2 Ce qui doit rester identique octet pour octet

**Doit être identique :**
- pour **chaque occurrence** de facette (chaque ordinal, jugée au niveau de son propre consommateur), la racine `order_root(anchors[target], prior_count)`, **pas** le `BallId` ;
- par conséquent, `block.roots` après tri et dédoublonnage ;
- les actions : parents triés, ordre des groupes par plus petit bloc, contributions dans l'ordre des blocs, blocs inertes ;
- les IDs de nœuds, les ancres, `Draft.batches` ;
- les IDs de population (FBT:839-865) et `lower_nodes` ;
- les forêts : `nodes` (niveau, `first`, `parent_count`), `parents`, `successors`, `contributions` (niveau, segment, population, masque, intérieur) ;
- la banque et le domaine ;
- `tower_digest` (`tower_chain.cpp:468-501`, qui hache le contenu des lignes et non leur ID) et `same_payload` (`full_ball_tower_gate.cpp:291-319`, qui compare aussi `ref.population` numériquement) ;
- les statuts et raisons sur la voie de succès ;
- les digests épinglés du worker `PINNED_DIGESTS` (`tower_worker_v9.py:138-143`, dont `67450c64611075b1` pour 00/K5/s8, que probe_0 reproduit) ;
- les compteurs issus de la charge utile : `representatives`, `births`, `merges`, `contributions`, `*_lots`, `inert_blocks`, `lot_dsu_slots`, `static_requests/unique/seeded` si la déduplication et les graines sont gardées, et `lower_*`.

**Peut changer, et demande un nouveau schéma de travail :** `key_lookups`, `anchor_hits`, `intruder_*`, `interior_ranges`, `*_steps`, `max_chain_steps`, `post_seed_*`, `resolve_work.*`. Les identités de FBT:1415-1418 et de la porte FBT-gate:376-384 deviennent alors fausses pour la nouvelle route. Il faut les versionner, jamais maquiller les comptes (contre-audit B, piège 2).

### 4.3 Porte par facette (shadow d'audit)

À FBT:720-725 (et FBT:1598-1605 pour W = 1) : garder dans `OrderState` une seconde table `static_targets_ref` calculée par l'ancien `static_terminal`. Pour chaque ordinal, comparer les deux racines au `prior_count` à l'aide d'une lecture de la DSU **sans compression** (`order_root` compresse, FBT:688-690), avant `order_lot`.

Deux mutants minimaux à tuer :
- `fx_cz` sans la règle 0 ;
- une cible de niveau valide choisie dans une autre composante (0, 1, 10, 11).

Les fixtures sont dans `audits/c_alternatives_20260923/experiences/refute_d5_full/` (`fx_cz`, `fx_ico12`, `lat5_*`, `lat6_*`, `plan_*`).

### 4.4 La couture `FullBallBatchResolver` : à éviter telle quelle

Trois obstacles à `prepare_external_batch` (FBT:1344-1444) :
1. `run()` ne parallélise les ordres que si `!batch_resolver.resolve` (FBT:366). Brancher par ce callback **sérialise les K**.
2. FBT:1409-1418 impose les identités de l'échange d'intrus (`full_ball_batch_work_identity`).
3. Les contrôles FBT:1422-1431 portent sur l'ordinal, le domaine, la fenêtre et le niveau, mais **pas sur la composante**.

### 4.5 Plafond du gain

Arithmétique sur le reçu R13, **pas une mesure**. En mode chevauché, la phase A de l'ordre K démarre après Σ_{j≥K} `static_by_k[j]`, et fenêtre = max_K(Σ_{j≥K} static_j + `lots_by_k[K]`).

| Cas | Fenêtre actuelle | Chemin critique | Plancher si phase 0 gratuite | Gain maximal |
|---|---:|---|---:|---:|
| probe_0 (K5) | 482,8 ms | K5 : 104,0 + 378,5 | 378,5 ms | ≈ 104 ms sur 745 |
| probe_2 (K10) | 1904 ms | K9 : 715,7 + 1188,4 | 1365,8 ms (`lots_by_k[10]`) | ≈ 539 ms sur 3039 |

Réserve : `lots_by_k` est mesuré sous la contention du pool de la phase 0. La phase A d'un ordre est mono-fil et devient ensuite le plancher.

---

## 5. Portes existantes qui jugeraient ce changement

Lignes de `CMakeLists.txt`.

| Porte | Ce qu'elle juge |
|---|---|
| `mhgp9_tower_full_ball_tower` (--selftest, 76-82) | Juge T2 Gram/Γ borné, voie temporelle : coupes ouvertes et fermées, verticales, fixtures `square`, `growth_ABCZ`, `shell7_window`, `actual_equal_radius_descent` (exige `same_radius_steps` > 0 et `intruder_queries` > 0), `*_doubled_lot` |
| `mhgp9_tower_full_ball_static_cpu1` / `_cpu4` (99-102) | Voie statique contre T2, **`same_payload` contre la voie temporelle**, identités du grand livre (gate:365-386), fixtures `present_but_wrong_rank`, `post_seed_ABEZW`, `post_seed_square_partial` ; planchers de non-vacuité (gate:519-528 : intrus, rayon égal, post-graine ≥ 4). Note : `overlap_static` y vaut false |
| `mhgp9_tower_key_index_mutant_drop_first` (443-447) | Une clé non indexée doit être vue, code 1 |
| `mhgp9_tower_anchor_meb*`, dont `_proposed`, `_fallback`, `_mutant_no_canonical`, `_mutant_last_maximum` (76, 430-451) | Noyau MEB |
| `mhgp9_chain_census_tower_{historical,line12,shell14,spatial12,rejects}` et mutants `assignment/open/adjacency/census` (135-140, 339-352) | T2 sur le catalogue réel ; tour à 0, 1 et 4 fils (`same_payload`) ; tour publiée par la chaîne à 1 et 4 workers, donc voie **chevauchée** |
| `mhgp9_chain_static_paths` (279-282) | Digest temporel = statique 1/4/8 sur 1 500 sites ; planchers ≥ 8 192 requêtes et ≥ 2 ouvriers ; catalogue renversé |
| `mhgp9_chain_order_failure_priority` et mutants `phase_priority`, `drop_failed_stats` (146-162) | Priorité des échecs static/lots/images, chevauché ou non ; travail payé conservé |
| `mhgp9_probe_worker_contract_{normal,optimized}` (364-377) | Schéma exact de `tower_work` (61/61) |
| `mhgp9_tower_parallel_sort` (+ mutant) | Tri des requêtes |

**Trous.**
- Aucune porte ne compare les **racines facette par facette**.
- Aucune porte n'impose `overlap_static` en dehors de la chaîne.
- Aucune porte de tour au label `scale8000/16000/32000`. Seules `mhgp9_chain_euler_scale8000` et `absent_keys_scale8000` existent, et elles jugent le catalogue.
- `tests/tower/census_tower_gate.cpp` n'est pas enregistré, ni le mode `--expect-launch-failure` de `full_ball_tower_gate`.