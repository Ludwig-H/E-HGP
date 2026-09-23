# D5, étape 1 : saut au centre dans la phase 0 de la tour FULL

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=conception (rien compilé ni modifié ; worktree f3409f711 en lecture)
public_status=not_claimed
```

GCP non utilisé. Les chemins sont relatifs à `morsehgp3D_v9/`, et **FBT** désigne `src/tower/forest/full_ball_tower.hpp`. Le même texte est dans /tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/d5-designer/design.md.

## 0. Décision

- **Ce que fait l'étape.** On remplace le corps de `static_terminal` (FBT:1266-1319), derrière un levier désactivé par défaut. La nouvelle descente applique la règle 0 à chaque état, puis saute aux k sites les plus proches du centre.
  - Inchangés : la phase A, les populations, les images, la banque et l'encodage.
  - Ajoutés : une ombre qui compare les racines facette par facette, un grand livre versionné et des sous-chronos de la phase 0.
- **L'exactitude ne demande aucun théorème nouveau.** Le saut s'appuie sur le lemme de connexité qu'utilise déjà l'échange actuel : tout k-sous-ensemble de D̄ est relié à l'état courant sous r_D.
  - Seule la progression, c'est-à-dire la baisse stricte du niveau, dépend de la complétude du catalogue. Elle est contrôlée à l'exécution.
  - Si elle manque, la tour refuse. Elle ne rend jamais une fausse racine.
- **Attendu sur G4.** C'est une projection, pas une mesure : environ 5–10 ms à K5 et 100–125 ms à K10 (voir § 6).
  - L'étape vaut d'abord par son grand livre et ses sous-chronos.
  - Ils montreront ce qui domine la phase 0 : la résolution, ou bien la collecte et le tri des requêtes.

## 1. Algorithme

### 1.1 Descente

La descente s'exécute pour chaque groupe de requêtes non semé. k est passé explicitement, jamais lu dans `current_k`.

```text
static_terminal_jump(k, key, before, work, js, seeds) const :
  s := key[0..k) ; D := meb(s)                                   # meb[k]++
  boucle :
    require(level(D) < before, "full_ball_static_not_strict")
    t := find_key(D.key)                                         # key_lookups[k]++
    si t : require(same_exact_level(t.level, D.level), "full_ball_static_anchor_level")
           si t.p + t.q_min - 1 <= k <= t.p + t.u : rendre t     # règle 0, à CHAQUE état
    jumps[k]++
    G := k plus petits (puissance, u) de D̄
         t existe -> interior() (puissance calculée) ∪ shell() (0)   # catalogue_census[k]++
         sinon    -> ball_k_nearest_closed(ix, D.key, k, s)          # tree_census[k]++
    require(|G| == k, "full_ball_static_jump_census_short")
    si G (trié par u) est une graine T :                         # seed_lookups[k]++
      require(k == T.p + T.u && level(T) < level(D) && level(T) < before,
              "full_ball_static_jump_seed_not_strict") ; rendre T
    E := meb(G) ; require(level(E) < level(D), "full_ball_static_jump_not_strict")
    s := G ; D := E
```

- **Aucun repli vers l'échange.** Un repli masquerait une omission du catalogue, que l'échange refuse déjà (`full_ball_static_missing_weak_terminal`).
- **Sentinelle de test.** Sous `MHGP9_TESTING` seulement, `testing_jump_watchdog` coupe à 4096 sauts par facette. Elle ne sert qu'à tuer le mutant M4.

### 1.2 Exactitude

- **Certificats d'appartenance.**
  - D = MEB(s) est vérifiée en arithmétique entière.
  - Pour le census d'arbre, G ⊂ D̄ est certifié par le test `pw <= 0` à chaque feuille.
  - Pour le census catalogué, la passe 1 de `validate_catalogue` a déjà vérifié que l'intérieur est < 0 et la coquille = 0 (FBT:1005-1006).
- **Témoin.** Le centre c_D appartient à C_s(r) ∩ C_G(r) pour tout r ≥ r_D. Ces cellules sont convexes, donc s et G sont dans la même composante de L_K(r) pour r ∈ [r_D, before).
- **Conclusion.** Les niveaux décroissent et r_{D₀} < before. La cible a donc son ancre dans la composante de F à `before⁻` ; c'est la post-condition 4 de la carte.
  - Par la règle 0, la cible est MEB(s_fin).
  - Par une graine, la cible est MEB(G).
- **Lien avec l'échange.** L'échange produit est le cas particulier s' = s∖{support} ∪ {z} ⊂ D̄.
- **Corollaire.** Sans règle 0, la route ne peut que **refuser**. Une mauvaise composante ne peut venir que d'une cible prise hors de la chaîne, ce que simule le mutant M3.

### 1.3 Progression et terminaison

- **Terminaison.** Le niveau baisse strictement à chaque saut.
- **Progression** (renforcement proposé par C). Sous catalogue complet, si D n'est pas terminale, alors k < p_D + q_min − 1 :
  - le cas k > p_D + u_D est exclu, car s ⊂ D̄ ;
  - une boule absente du catalogue a p + q_min − 1 > Kmax.
- **Conséquence.** G contient au plus q_min − 2 sites de coquille, donc aucun support. Par unicité de la MEB, MEB(G) < r_D.
- **Statut.** C'est un invariant contrôlé à l'exécution, pas une hypothèse de correction. Il doit être rédigé dans le document de preuves v9, au statut `proved_here` en attente de contrelecture, avant tout reçu.

### 1.4 Les k plus proches du centre dans D̄, avec le `CloudIndex` existant

Nouvelle fonction dans `src/tower/pipeline/census.hpp`, après `ball_census` (:174-208) :
`ball_k_nearest_closed(ix, key, k, seed_sites, KBest&, KnnStats*, std::vector<KnnFrame>*)`.

- **Ordre.** On trie par (puissance, u) en ordre lexicographique.
  - P(z) = a·(‖z−c‖² − r²) avec a > 0 : l'ordre des puissances est celui des distances au centre, sans division.
  - u est le rang de Morton de la position unique. Les positions sont distinctes, donc l'ordre est total strict.
- **Tas.**
  - C'est un tableau trié d'au plus 10 couples (i128, i32).
  - Il est **amorcé par s** au prix de k tests de puissance ; comme s ⊂ D̄, il est plein dès le départ.
  - Le seuil τ est son pire élément, en général (0, u_max des supports).
- **Parcours.**
  - DFS sur l'arbre radix, avec une pile de (NodeRef, mn) hissée par ouvrier.
  - `AxisBounds::bounds` (census.hpp:61-104) donne mn, le minimum exact sur les points entiers de la boîte.
  - Les enfants sont empilés par mn décroissant.
- **Élagage.** On coupe un nœud si `mn > 0`, si `mn > τ.p`, ou si `mn == τ.p && range_of(z).first > τ.u`.
- **Feuille.** Un site entre dans le tas si `pw <= 0`, si `(pw,u) < τ`, et s'il n'y est pas déjà.
- **Coût.**
  - Jamais plus qu'un `ball_census` fermé.
  - Pas de plafond d'intérieurs : une D absente du catalogue peut en contenir bien plus que 9.
  - Coût attendu O(h + k) nœuds ; pire cas O(|D̄∩P|) sur une coquille cosphérique massive (`sq_*`).
- **Census catalogué.** Au plus 9 calculs de puissance et 12 zéros. Sous `MHGP9_TESTING`, on le recoupe avec la requête d'arbre et G doit être identique (`full_ball_static_jump_census_crosscheck`).

### 1.5 Arithmétique u18 (M = 262 143)

- **Clés.** Les clés rendues par `meb()` sont primitives, avec les bornes de census.hpp:63-66 : A < 2^76, |B_i| < 2^96, |C| < 2^116.
- **Puissance.** A‖z‖² < 2^114 et |B·z| < 2^116, donc |P| < 2^118 : le calcul en i128 est exact.
- **Autres calculs.** `AxisBounds` reste sous 2^118. Les niveaux se comparent par `compare_exact_level` (U320).
- **MEB.** Elle est proposée en double puis vérifiée en entier. Elle est déjà jugée par les portes `mhgp9_tower_anchor_meb*`.
- **Aucun nouveau flottant.**

### 1.6 Coût et mémoire

- **Coût par saut :**
  - une requête knn : le sidecar mesure en moyenne 108 nœuds par census complet à 8k/K10, et l'élagage ne peut que réduire ce chiffre ;
  - une recherche de graine en O(k log S) ;
  - une MEB sur au plus 10 sites ;
  - un `find_key` en O(1) (FBT:955-961), avec un index déjà payé dans `validate`.
- **Mémoire.**
  - Aucun index ni aucune table globale.
  - Par ouvrier : environ 320 o de tas et quelques Ko de pile.
  - L'ombre ajoute `static_targets_ref` : 4 o par requête, soit environ 70 Mo pour une trame à K10. Elle est interdite pendant les mesures de temps.

## 2. Modifications

| Ancre | Changement |
|---|---|
| FBT:34-35 | `kFullBallStaticJumpAccounting = "static_exact_sort_unique_seeds_rule0_every_state_closed_k_nearest_jump_v1"` |
| FBT:39-67, 118-122 | `FullBallStats` reçoit `StaticJumpStats jump` et `StaticShadowStats shadow`. `FullBallTimes` reçoit `static_{collect,sort,resolve}_by_k`, pour les deux routes |
| FBT:350, 356-366 | Le constructeur prend `FullBallStaticRoute{jump, shadow}`. Refus `kInvalidInput` dans trois cas : ombre sans saut ; saut avec `geometry_threads == 0` (la voie temporelle reste le témoin) ; saut avec `batch_resolver` |
| FBT:484-492, 510, 625 | Ajout de `OrderState::static_targets_ref`, échangé (`swap`) en même temps que `static_targets` |
| FBT:684 et ~1244 | `peek_root(...) const`, **sans compression** |
| FBT:720-725, 1598-1605 | En mode ombre, **avant** `order_root`, exiger `peek_root(anchors[ref]) == peek_root(anchors[jump])`, sinon `full_ball_static_jump_shadow_root_mismatch`. La DSU et le payload restent ceux du mode sans ombre |
| FBT:1266-1319 | `static_terminal` ne change pas ; `static_terminal_jump` est ajouté à côté |
| FBT:1321-1342, 1548 | Fusion des nouveaux champs ; le `Worker` reçoit `KBest`, la pile knn et `shadow_work` |
| FBT:1566-1586 | Aiguillage de l'appel FBT:1578. En mode ombre, `static_terminal` tourne aussi (dans `shadow_work`), `ref` est écrit par ordinal et `terminal_differs` est compté |
| FBT:1596 | `check_jump_ledger(k)`, appelé après `account()` |
| FBT:1859-1865 | Nouveau paramètre, ajouté **en dernier** |
| `census.hpp:208+` | `ball_k_nearest_closed`, `KBest`, `KnnStats` |
| `tower_chain.hpp:93+`, `tower_chain.cpp:931-932` | Options `tower_static_jump = false` et `tower_static_jump_shadow = false`, transmises à la tour |
| `bench/tower_probe.cpp:158-161, 203-214, 297-315` | Nouveaux leviers ; objet JSON `tower_static` ; schéma `mhgp9_tower_probe_v19`. `tower_work` garde ses 61 clés |
| `gcp-migration/tower_worker_v9.py:190-194, 443-455` | Jeu exact `TOWER_STATIC_KEYS` ; J1–J9 revérifiées sans `assert` ; jumeaux 0/1 exigés à `tower_digest` égal ; `PINNED_DIGESTS` inchangé |
| `bench/run_lidar_scaling.py:58, 197` | Transmission de `--lever` ; les leviers attendus sont ceux de la commande |
| `tests/tower/full_ball_tower_gate.cpp:365-386, 510-528, 561-576` | Identités selon la route, fixtures D5, planchers, options `--static-jump-{1,4}` |

## 3. Grand livre `tower_static` (toujours publié)

- **Commun aux deux routes** (rien de cela n'est publié aujourd'hui) :
  - `accounting`, `route`, `shadow` ;
  - `requests`, `unique`, `seeded` par K ;
  - `collect_ms`, `sort_ms`, `resolve_ms` par K ;
  - `max_chain_steps`.
- **Route échange :** `post_seed_*` par K, `same_radius_steps`, `descending_steps`, `intruder_power_tests`, `interior_ranges`, `depth0_hits`.
- **Route saut, par K :**
  - `facets`, `rule0_initial`, `rule0_after_jump`, `jumps` (nombre de sélections des k plus petits) ;
  - `catalogue_census`, `tree_census`, `tree_nodes`, `tree_leaf_tests` ;
  - `seed_lookups`, `seed_after_jump`, `meb`, `key_lookups` ;
  - `shell_selected` : sites de puissance 0 retenus dans G ;
  - `boundary_ties` : cas où un candidat de même puissance que le k-ième est écarté par u.
- **Route saut, en global :** `jumps_hist[0..8+]`, `jumps_sum`, `max_jumps`.
- **Ombre, par K :** `occurrences_compared`, `root_mismatches` (doit valoir 0), `terminal_differs`, `exchange_meb`, `exchange_intruder_nodes`, `exchange_depth0_hits`.
- **MEB évitées.** Elles ne sont jamais publiées comme nombre dérivé. Le lecteur les calcule par `exchange_meb − meb`, ou en comparant les jumeaux.

**Identités.** La tour, la porte et le worker les vérifient toutes.

| | Identité |
|---|---|
| J1 | `facets = unique − seeded` |
| J2 | `facets = rule0_initial + rule0_after_jump + seed_after_jump` |
| J3 | `jumps = catalogue_census + tree_census = seed_lookups` |
| J4 | `meb = key_lookups = facets + jumps − seed_after_jump` |
| J5 | `Σmeb = resolve_work.calls` et `anchor_hits = Σ(rule0_initial + rule0_after_jump)` |
| J6 | `Σjumps_hist = Σfacets` et `jumps_sum = Σjumps` |
| J7 | En route saut, `intruder_*`, `post_seed_*`, `same_radius_steps` et `descending_steps` valent 0 ; `max_chain_steps = max_jumps` |
| J8 | En mode ombre, `exchange_depth0_hits = rule0_initial` (même premier MEB, même recherche) |
| J9 | Le grand livre est identique à l'octet quel que soit le nombre de fils (fonction pure par groupe, sans mémo) |

## 4. Portes

### 4.1 Ombre par facette

- **Ce qui est exigé.** Pour **chaque ordinal**, au `prior_count` de son consommateur, la racine de la cible du saut doit égaler celle de la cible de l'échange.
- **Ce qui n'est pas exigé.** Les terminaux n'ont pas à être égaux : dans le sidecar, 18 % des facettes distinctes changent de terminal. Seule la racine est invariante.
- **Où l'ombre tourne.** Dans le binaire produit, comme option d'exécution, pour juger les trames réelles. Cela répond à la réserve de B : le juge ne doit pas exister seulement sous `MHGP9_TESTING`.

### 4.2 Fixtures

Les fichiers `.u32le` de `refute_d5_full/` sont copiés dans `tests/fixtures/d5_refute/`. On y joint un MANIFEST SHA-256 et la provenance : auditeur C, commit `12a5f28f`, générateur `gen.py`.

| Porte | Fixtures | Jugement |
|---|---|---|
| `mhgp9_tower_static_jump_cpu{1,4}` (T2 borné) | `e1_collinear` {(0,0,0),(1,0,0),(10,0,0),(11,0,0)} à K2 ; `fx_cz`, `fx_czm`, `fx_c34`, `fx_cm34` ; `fx_ico12` à K≤10 ; plus toutes les fixtures existantes (38 passages avec voie statique, gate:517) | Juge Gram/Γ, `same_payload` contre la voie temporelle, ombre et J1–J9 |
| `mhgp9_chain_static_jump_degenerate` | `lat4_*`, `lat5_1..3`, `lat6_1..3`, `plan_*`, `sq_*`, avec Kmax = 10 | Le digest temporel à W1 doit égaler le digest du saut avec ombre à W4 et W8. Plancher d'occurrences épinglé au premier passage (C en compte 221 556) |
| `mhgp9_chain_static_jump_paths` (1 500 sites, sur le modèle de `chain_static_paths`) | — | Temporel = échange statique = saut, à 1, 4 et 8 fils ; J9 |

- Si l'oracle borné refuse `e1_collinear`, cette fixture passe par la chaîne. Elle reste obligatoire.
- **Planchers**, chacun ≥ 1 : `tree_census`, `catalogue_census`, `rule0_after_jump`, `seed_after_jump`, `shell_selected`, `boundary_ties`, `terminal_differs`.

### 4.3 Mutants causaux

Ce sont des macros de compilation, chacune dans un exécutable séparé. Chaque mutant doit rendre le code exact 1, comme `mhgp9_tower_key_index_mutant_drop_first`, et la raison attendue est vérifiée.

| Mutant | Effet | Tué par |
|---|---|---|
| M1 `NO_RULE0` | Règle 0 supprimée | Refus `jump_not_strict` sur au moins une des quatre variantes `fx_*`, celle où l'ordre de Morton donne G = {a,b} |
| M2 `RULE0_INITIAL_ONLY` | Variante « prose » : pas de règle 0 après un saut | `jump_not_strict` sur les grilles |
| M3 `FOREIGN_TARGET` | Rend `programs[k][0]` quand son niveau est < before : cible structurellement valide mais étrangère | `shadow_root_mismatch` sur `e1_collinear` à K2, facette {1,10} |
| M4 `NONSTRICT` | Accepte `level(E) <= level(D)` | Sur une `fx_*` privée de la boule MEB({a,b}) : la version nominale rend `jump_not_strict` (le plancher prouve que ce chemin est atteint), le mutant rend `testing_jump_watchdog` |
| M5 `OPEN_CENSUS` | N'admet que `pw < 0` | `jump_census_short` sur toute D avec p_D < k |
| M6 `KNN_PRUNE_TIES` | Élague dès que `mn >= τ.p` | `jump_census_crosscheck` sur les grilles avec `boundary_ties` > 0 |
| M7 `CATALOGUE_DROP_SHELL` | Retire un site de coquille au census catalogué | `jump_census_crosscheck` |
| M8 `LEDGER_DROP_TREE` | Ne compte pas `tree_census` | `jump_ledger_identity` (J3) |
| MT `TIE_REVERSED` | Départage par u décroissant | Porte métamorphique : code 0, payload et racines identiques, mais grand livre **différent** sur les grilles |

`MHGP9_KEY_INDEX_MUTANT_DROP_FIRST` et `census-nonstrict` sont aussi rejoués avec le saut actif.

### 4.4 Contrats et échelle

- **Contrat worker.** `mhgp9_probe_worker_contract_*` passe au schéma v19 ; toute altération de `tower_static` doit être refusée.
- **Échelle.** Portes `mhgp9_tower_static_jump_scale{8000,16000,32000}`, avec les labels du même nom. Elles tournent à K5 sur la scène 01, avec des coupes dérivées à la volée par `run_lidar_scaling.py`. Elles exigent :
  - l'ombre, sans aucun écart ;
  - un digest égal à celui obtenu levier coupé ;
  - J1–J9 et les planchers par K ;
  - jamais de juge O(n³).

## 5. Mesure locale (8 cœurs, Release, build dans le scratchpad)

**Entrées.** Scènes 00, 01 et 02 (trames 08/000000, 000100 et 000200, sans sol, grille 1 mm, reçu v8 `lidar_ground_20260921`). Coupes emboîtées de 8 000, 16 000 et 32 000 sites, plus la trame entière. K5 et K10, s = 8.

1. **M0 : sous-chronos seuls.** Levier coupé, dans un commit qui ne change pas l'objet. On mesure la part `resolve_ms / static_by_k` sur les trames entières, à `--static=8`. Ce commit accompagne aussi la prochaine session G4.
2. **M1 : exactitude.** Saut avec ombre sur les 24 cas. On exige zéro écart, les mêmes digests de tour et de catalogue que levier coupé, et J1–J9.
3. **M2 : déterminisme.** `--static=1` contre `--static=8`, à 32k et sur les trames entières. Digest et grand livre doivent être identiques à l'octet.
4. **M3 : compteurs par K.** MEB, nœuds, histogramme des sauts et travail pondéré P = 3 680·MEB + 136·nœuds (coûts Zen 3 du développeur). On en tire les exposants de 8k à 32k.
5. **M4 : temps.** Sans ombre, levier 0/1 **entrelacé**, 5 paires par cas, sur 32k et les trames entières, en K5 et K10. On relève la médiane et le minimum de `resolve_ms`, `static_by_k`, la fenêtre (static + lots), `tower_ms` et le RSS.
   - Ne jamais lire `lots_ms` seul : il vaut fenêtre − static, donc il **monte** quand static baisse.

**Reçu.** `receipts/tower_static_jump_local_<date>/`, avec commit, SHA-256 de la sonde, FNV des entrées, commandes, sorties brutes, SUMMARY et SHA256SUMS.

**Critères d'arrêt.**

- **S-exact.** Au premier écart de racine, digest différent, identité violée ou refus sur une entrée valide : arrêt, fixture minimale permanente, mise à jour du document de preuves.
- **S-travail.** Si P_OFF/P_ON < 1,3 à 32k/K10 (somme sur K2..K10) : reçu négatif, étape close sans chronométrage.
- **S-échelle.** Arrêt si les sauts par facette sautante, ou les nœuds par census, augmentent de plus de ×1,3 à chaque doublement de n. Arrêt aussi si `max_jumps` double à chaque doublement.
- **S-temps.** Sur les trames entières en K10, si `resolve_ms` ON/OFF > 0,8 ou si la fenêtre ON/OFF > 0,97 : reçu négatif, et pas de session G4 dédiée.
- **S-K5.** Si `static_by_k[5]` ON ≥ OFF, on le consigne. Une activation par K serait un levier séparé.
- **Poursuite.** Sinon, jumeaux G4 dans la prochaine session : W48, 3 trames × K5/K10 × levier 0/1 × 2 répétitions, avec `tower_digest` égal exigé.

## 6. Estimation sur R13 (calcul sur le reçu, pas une mesure)

**Ce qui peut bouger.** Seule la **part résolution** de la phase 0 (`static`).
- `validate` ne bouge pas, puisque l'index est réutilisé.
- `populations`, `images`, `bank`, `encode` et le travail propre de `lots` ne bougent pas non plus.
- Le temps de mur suit le chemin critique chevauché : max_K(Σ_{j≥K} static_j + lots_K).

**Part de la résolution.** Travail pondéré relevé dans `vm/probe_*.stdout`, converti en mur en supposant 3 GHz et 48 fils parfaits :

| Cas | MEB | Nœuds d'intrus | Cycles | ≈ mur à W48 | Phase 0 | Part |
|---|---:|---:|---:|---:|---:|---:|
| probe_0 K5 | 1,29 M | 31,7 M | 9,1·10⁹ | 63 ms | 256 ms | ≈ 25 % |
| probe_2 K10 | 11,3 M | 403 M | 96,5·10⁹ | 670 ms | 1 564 ms | ≈ 43 % |

Le reste de la phase 0 se répartit entre la collecte (`visit_block` et `ShellTable::rank`), le tri de 3,6 M ou 17,4 M requêtes de 56 o, et la formation des groupes. M0 tranchera.

**K5.**
- Le chemin critique est K5 : 104,0 + 378,5 = 482,5 ms (K4 : 457,7 ms).
- À Kmax, tout census passe par l'arbre ; C projette un facteur ÷1,3 à ÷1,5.
- Gain ≈ 0,25 × 104 × (1 − 1/1,4), soit **5–10 ms sur 745 ms**. `static` baisserait d'environ 20–27 ms, mais la fenêtre reste bornée par lots_5.

**K10.**
- Chemins critiques : K9 = 1 904 ms, K8 = 1 904 ms, K10 = 1 768 ms.
- Hypothèses : part résolution de 43 %, facteur ÷1,3 à K10 et ÷1,8 à ÷2,6 pour K < 10.
- Résultat : K9 ≈ 1 904 − (40 + 60 à 83) ms, et K10 ≈ 363 + 1 366 = 1 729 ms.
- La fenêtre tombe à 1 780–1 805 ms, soit **un gain d'environ 100–125 ms sur 3 039 ms**. `static` baisserait de 260 à 350 ms.
- Plafond, si la phase 0 était gratuite : 539 ms. Plancher : lots_10 = 1 366 ms (phase A mono-fil).

**Inconnue favorable, non comptée.** Les 48 fils de la phase 0 partagent les 48 vCPU avec les fils de la phase A. Moins de CPU consommé en phase 0 peut raccourcir `lots_by_k`, qui est le vrai plancher. Seuls les jumeaux G4 le diront.

**Si M0 montre que la collecte et le tri dominent,** le prochain levier de la phase 0 est là, et non dans la descente.

## 7. Hors de cette étape (non prouvé, ou mesuré négatif)

- **Proposition F et FULL compact.** La proposition est fausse (contre-exemples lat5_3 à K8 et lat5_1 à K9). Chaque exception doit porter la position du premier bloc de son groupe. Portage séparé.
- **Lemme C (images verticales).** La contrelecture ferme la borne basse, mais le lemme n'est pas encore au statut `proved_here`. La phase C et le garde-fou `full_ball_vertical_birth_anchor` restent inchangés.
- **Phase A maigre.** Le tampon `r[13]` du sidecar a un comportement indéfini sur `fx_ico12`, qui produit 32 parents. La phase A du produit n'est pas touchée.
- **Index des selles (lemme A).** Prouvé, mais mesuré négatif seul (`saddle_index_negative_20260923`). Il ne reviendra que comme second bras « saut + index ».
- **`jmemo`.** Correct en principe, mais c'est un état partagé entre ouvriers : le travail dépendrait du découpage, et J9 tomberait.
- **Chiffres de C** (MEB ÷5,3 à ÷5,7, 2 456 ms, 87 ns par bloc). Ils incluent l'index, le mémo et le census catalogué. Les projections pour le saut seul ne sont pas des mesures.
- **« Jamais de repli » et égalité des terminaux.** Ce ne sont pas des invariants. Le premier est remplacé par un refus contrôlé, la seconde est traitée au § 4.1.

## 8. Risques

- **Catalogue incomplet.** Le saut peut réussir là où l'échange refuse. Exemple : sur une `fx_*` privée de MEB({a,b}), si c précède a ou b en u, G contient c et mène à un diamètre catalogué, qui est une cible correcte.
  - La porte `reject.missing_required_weak_terminal_anchor` (gate:494-499) doit être rejouée avec le saut.
  - Toute acceptation doit être documentée : l'autorité relative ne promet pas de détecter toutes les omissions.
- **Coquilles cosphériques massives** (`sq_*`, `plan_*`). Le census d'arbre y tend vers O(|D̄|). Le compteur `tree_nodes` par K le montrera.
- **Schéma v19.** `run_lidar_scaling.py --revalidate` doit pouvoir relire les archives v18 : les schémas connus sont étendus, pas remplacés.
