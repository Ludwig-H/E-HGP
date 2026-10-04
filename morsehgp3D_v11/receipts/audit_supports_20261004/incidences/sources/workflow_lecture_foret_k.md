# Lecture de la forêt d'ordre K de la tour FULL v11 — rattachement boule critique → nœud

Date : 4 octobre 2026, 17 h 48 UTC. Lecture seule, aucun build, aucun test, aucune commande GCP (`GCP non utilisé`).
Source lue : worktree `/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11` au commit `7df199f73`.
Tous les chemins ci-dessous sont relatifs à `morsehgp3D_v11/` sauf mention contraire.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
```

## 0. Résumé

- L'**arbre de niveau K** est exactement `OrderForest` d'ordre K : nœuds `[0,b)` = naissances, triées par (rang, centre exact), puis fusions n-aires créées plateau par plateau ; un nœud a un rang de niveau, un parent, une plage d'enfants (`src/tower/forest.hpp:81-87`, `:152-203`).
- Une **boule critique d'ordre K** est une boule du catalogue dont la fenêtre `[p+q-1, p+m]` contient K (`src/tower/forest_build.cpp:150`). Elle est soit une **naissance** (`kinds=1`, un nœud propre), soit une **cellule à traces strictes** (`kinds=2`), qui réunit (fusion) ou referme un cycle (continuation, aucun nœud) (`src/tower/forest_plateau.cpp:69-98`).
- Le **rattachement** exact d'une cellule `b` est le nœud vivant à la **coupe fermée** `λ_b` qui contient une quelconque de ses graines : c'est `states[find(graine)].top` juste après `close(λ_b)`, ou, après coup, `ancestor_closed(graine, rang_b)`. Ce nœud est la fusion créée à `λ_b` si la composante de `b` a fusionné sur ce plateau, sinon le nœud vivant antérieur (liaison interne). Le résultat ne dépend pas de la graine choisie.
- Le code **ne garde aujourd'hui aucune relation boule → nœud** dans la forêt : seuls des compteurs agrégés existent (`ForestLedger`, `src/tower/forest.hpp:91-102`). Le banc `bench/points_export.cpp:178-214` (`ball_nodes`) reconstruit déjà cette relation **après coup** pour les seules boules *fortes* (`p+q ≤ k`), donc **sans** les cellules de jonction régulières qui portent la plupart des fusions.
- La forêt d'ordre K **se construit sans les ordres 1..K−1 ni les verticales** : `build_forest(domain, K, …)` existe (`src/tower/forest.hpp:209-211`, `src/tower/forest_build.cpp:414-421`) et ne lit que le domaine et sa propre forêt. Le catalogue `Cat_K`, lui, ne diminue pas. Le gain attendu porte sur environ 30 % du travail MEB des descentes et sur la phase verticale. En contrepartie, la voie pipeline (`kmax ≥ 2`) et la liaison de la table de populations (qui exige les K forêts) sont à adapter.
- Recommandation : **instrumenter** `cell()` et `regular_cell()` pour retenir **une graine de naissance par cellule** (4 octets par boule), puis faire **un balayage fermé** (`ClosedAncestorSweep`) dans l'ordre des boules, déjà trié par rang. La reconstruction par descente (le `ball_nodes` élargi à la fenêtre) sert de **juge différentiel**.

## 1. (a) Cellule, trace, boules critiques, naissance, fusion, continuation

### Boule critique et fenêtre

- Boule critique positive `b=(c,λ)`, `I_b` intérieur strict, `U_b` coquille, `p=|I_b|`, `m=|U_b|`, `q=q_min` (`docs/MATHEMATIQUES.md:19-28`). Champs catalogue : `support` (S* croissant), `rank`, `p`, `m`, `qmin` (`src/catalogue/catalogue.hpp:45-50`).
- Catalogue `Cat_K = {b : p+q ≤ K+1}`, fenêtre d'événements `[p+q−1, p+m] ∩ [1,K]` (`docs/MATHEMATIQUES.md:80-86`). Justification : pour `t=k−p ≤ q−2`, tout est séparable, donc il n'y a qu'un morceau et aucun événement. Pour `p ≥ k`, toutes les k-parties strictes sont reliées strictement (`docs/MATHEMATIQUES.md:175-176`, `:194-198`).
- Le catalogue est trié par niveau exact : `BallIdx` croissant implique un rang non décroissant (`docs/FULL_BIRTH_RUNS.md:3-5`). Les rangs sont les rangs denses des niveaux distincts (`docs/PERFORMANCE_FULL.md:142-144`), avec `levels()[0]` = niveau nul (`src/catalogue/catalogue.hpp:47`).

### Cellule et trace

- **Cellule** = couple `(b,k)` avec `p+qmin−1 ≤ k ≤ min(p+m,K)`. Hors de cette fenêtre, `build_cell` refuse (`src/tower/cells.cpp:241-253`, `docs/CELLS_AND_LOCATE.md:221-224`). On pose `t = k−p`, avec `1 ≤ t ≤ m`.
- **Trace** (`CellTrace`) = la k-partie `I ∪ A`, avec `A ⊂ U`, `|A| = t`, triée par `SiteIdx`, complétée par `kNone` (`src/tower/cells.hpp:8-12`). Elle est **stricte** ssi `A` est séparable (`c ∉ conv A`) ssi `β(I∪A) < λ` (T2, `docs/MATHEMATIQUES.md:167-173`). Les traces sont rangées dans l'ordre lexicographique des `A` dans `U` (`docs/CELLS_AND_LOCATE.md:225`).
- Une trace stricte appartient à une composante **antérieure** au plateau. Les k-parties non strictes de `P_b` (celles qui contiennent un support) sont de **nouveaux sommets** de `Γ_k`, au niveau `λ`.

### Classification à l'ordre k (`kinds`, 0 / 1 / 2)

- `kinds` : 0 hors fenêtre, 1 naissance, 2 traces strictes (`src/tower/forest_internal.hpp:392`). Le calcul se fait dans `classify_range` (`src/tower/forest_build.cpp:144-173`).
- **Coquille régulière** (`m = q`) : la fenêtre ne contient que `k = p+q−1` (jonction, `kinds=2`, avec q faces strictes analytiques) et `k = p+q` (naissance, `kinds=1`) (`src/tower/forest_build.cpp:151-159`, `docs/MATHEMATIQUES.md:196-197`).
- **Coquille étendue** : `classify_cell`. On a une naissance si `t = m`, une cellule stricte si `t < qmin`. Sinon on cherche le premier `A` lexicographique avec `β(A) < λ`. Si on n'en trouve aucun, c'est une naissance (`src/tower/cells_classify.cpp:292-314`).
- À l'**ordre 1**, les naissances sont les sites (`birth_key = SiteIdx`). Aucune boule n'y naît (`src/tower/forest_build.cpp:59-69`, `:315`). Les cellules sont les boules `p=0, qmin=2`, toutes strictes (`t=1 < qmin`).

### Naissance, fusion n-aire de plateau, continuation

- **Naissance** (T3, `|P_b| = k` ou aucune partie stricte) : un nœud sans enfant, `birth_key = BallIdx` (`src/tower/forest_build.cpp:102-112`). Les naissances sont ordonnées par (rang, centre exact) avec `num::compare_centers` sur les cohortes de même rang (`src/tower/forest_build.cpp:72-100`, `docs/FULL_FORESTS.md:17-24`).
- **Plateau** = toutes les cellules `kinds=2` de même rang. Voie sérielle : `src/tower/forest_plateau.cpp:122-138`. Voie par lots : `select` ferme au premier rang supérieur (`src/tower/forest_parallel.cpp:330-343`, `:400`). Ordres concurrents et pipeline : `publish` (`src/tower/forest_concurrent.cpp:27-56`).
- **Fusion n-aire** : à `close(level)`, chaque racine DSU survivante de `touched` dont la chaîne contient au moins deux anciennes composantes produit **un** nœud de niveau `level`. Ses enfants sont les anciens `top`, triés par `NodeIdx` (`src/tower/forest_plateau.cpp:69-98`). Aucune fusion binaire intermédiaire n'est publiée (`docs/FULL_FORESTS.md:47-50`, T4 `docs/MATHEMATIQUES.md:200-208`).
- **Continuation** : une chaîne d'une seule ancienne composante. Aucun nœud n'est créé, le compteur `continuations` est incrémenté (`src/tower/forest_plateau.cpp:78`). Cela couvre le cas « la population couverte augmente sans fusion » (T3, `docs/MATHEMATIQUES.md:191-192`) et les liaisons qui ferment un cycle.
- Numérotation : naissances `[0,b)`, puis fusions dans l'ordre (niveau, plus petite naissance descendante), puisque `touched` est trié par racine = plus petite naissance canonique (`src/tower/forest_plateau.cpp:31`, `:70`, `docs/FULL_FORESTS.md:17-19`).

## 2. (b) Où chaque boule critique d'ordre K tombe dans une composante, et à quel nœud

### Mécanique DSU d'un plateau

- `ForestState{parent, top, head, tail, next, touched}` (`src/tower/forest_internal.hpp:351-354`) : un état par **naissance**. L'indice DSU vaut donc le `NodeIdx` de la naissance. Initialement, `top = i` (`src/tower/forest_build.cpp:401`).
- `cell(b)` : `build_cell`, puis pour **chaque** trace `resolve_descent`. Contrôle 1 : la date initiale est strictement inférieure à `λ_b` (`src/tower/forest_plateau.cpp:53`). Contrôle 2 : la graine `birth_node(seed)` a un rang strictement inférieur à `rang_b` (`:56-57`). Ensuite `root = find(seed)`, `touch(root)`, et `unite_roots(first, root)` (`:59-62`).
- `regular_cell(b, seeds)` : mêmes contrôles et mêmes unions sur les q graines déjà résolues (`src/tower/forest_plateau.cpp:100-120`). Les graines sont produites par `ForestParallel::resolve_job`, dans l'ordre exact des faces de `build_cell` (`src/tower/forest_parallel.cpp:140-206`).
- `unite_roots` choisit comme racine la **plus petite** naissance et concatène les chaînes (`src/tower/forest_plateau.cpp:28-37`). `touch` met l'ancienne composante dans `touched` à sa première visite (`:17-24`). Le champ `top` reste celui d'avant le plateau jusqu'à `close` (`docs/FULL_FORESTS.md:39-45`).
- `close(level)` crée les fusions, pose les `parent` des enfants, remplace `states[root].top` par la fusion, puis remet `touched` à zéro (`src/tower/forest_plateau.cpp:69-98`).

### Rattachement exact de chaque boule critique d'ordre K

| Rôle | Condition | Nœud de rattachement | Où le lire |
|---|---|---|---|
| naissance | `kinds[b] = 1` (ordre K ≥ 2) | le nœud de naissance de `b` : `dense_[b]` ou recherche dans `lookup_` | `OrderForest::birth_node` (`src/tower/forest_vertical.cpp:11-26`) |
| fusion | `kinds[b] = 2` et la composante de `b` réunit au moins deux anciennes composantes **sur ce plateau** | la fusion n-aire créée à `λ_b` (rang égal à `rang_b`, `NodeIdx ≥ births()`) | `states[find(g)].top` après `close(λ_b)` |
| liaison interne | `kinds[b] = 2` et une seule ancienne composante pour toute la composante de plateau | le nœud vivant **antérieur** `v`, avec `a_v < λ_b < a_parent(v)` | idem : `top` inchangé par `close` (`src/tower/forest_plateau.cpp:78`) |

Ici `g` est **n'importe quelle** graine de naissance résolue par une trace de `b`. Après le plateau, toutes les traces de `b` sont dans la même composante, unies par `unite_roots`. Le nœud trouvé ne dépend donc ni de la trace ni de la politique de descente, mémo ou table de populations comprises (T5, `docs/MATHEMATIQUES.md:219-222`).

### Égalités de niveau

- Un plateau regroupe exactement les boules de **même rang**, c'est-à-dire de même niveau exact.
- Les traces doivent dater **strictement** avant le plateau (`src/tower/forest_plateau.cpp:53`, `src/tower/forest_parallel.cpp:193`). Une naissance de même niveau n'est donc jamais touchée : le rang de la graine doit être strictement inférieur à `rang_b` (`src/tower/forest_plateau.cpp:57`, `:109`). C'est T4, « les naissances du niveau restent isolées » (`docs/MATHEMATIQUES.md:202-206`).
- Une cellule « passagère » (une seule ancienne composante) dont la composante est fusionnée **au même niveau** par une autre cellule se rattache à la **fusion**, pas à l'ancien nœud. C'est cohérent avec la coupe fermée (`a ≥ λ`) et avec le graphe biparti de T4.
- Une liaison interne vérifie `a_v < λ_b` **strictement** et `λ_b < a_parent(v)` **strictement**. Si le parent était au même niveau, `b` serait dans ce plateau et se rattacherait au parent. Le nœud vivant à la coupe fermée vérifie `a_v ≤ a < a_parent` (`docs/MATHEMATIQUES.md:240-241`). `ancestor_closed` suit les parents de rang `≤` (`src/tower/forest_vertical.cpp:28-38`).
- Par suite, la partition des boules critiques d'ordre K sur les nœuds est bien définie. Le polyèdre « à la fin de vie » d'un nœud est l'union sur son sous-arbre, donc `P_enfant ⊂ P_parent`. Un instantané daté à `a ∈ [a_v, a_parent)` filtre en plus les boules par `λ_b ≤ a` : seules les liaisons internes peuvent avoir `λ_b > a_v`.

## 3. (c) Nombre de liaisons d'une boule à l'ordre K

Avec `t = K − p` :

- **K-parties de `P_b`** : `C(m,t)` parties `I ∪ A`. Elles sont **toutes connectées au seuil fermé `λ`** par `b` si `|P_b| ≥ K+1` (T3, `docs/MATHEMATIQUES.md:188-192`). Le binôme exact est calculé par `cell_binomial(m,t)` en u128, avec refus `tower_capacity` en cas de dépassement u64 (`src/tower/cells.cpp:109-120`).
- **Liaisons vers l'existant** : `S` = nombre de traces strictes, c'est-à-dire de `A` séparables. Les nouveaux sommets sont les `C(m,t) − S` restantes.
- Cas particuliers :
  - jonction régulière (`m = q`, `t = q−1`) : `C = S = q`. Les q faces sont strictes et reliées par l'unique `(K+1)`-partie `P_b`.
  - naissance avec `t = m` : une seule partie, `P_b`.
  - naissance étendue avec `t < m` (exemple des losanges, `docs/FULL_BIRTH_RUNS.md:76-81`) : `C(m,t)` parties, `S = 0`.
- **Composantes distinctes touchées** : le nombre de composantes antérieures distinctes que rencontrent les traces strictes de `b`. C'est la quantité géométrique pertinente pour « combien de branches ce support relie ». Elle reste **au plus `S`**, et ni `S` ni elle ne sont l'arité de la fusion (`docs/CELLS_AND_LOCATE.md:234-240`, surjection T2/T6 `docs/MATHEMATIQUES.md:182-186`).
- **Ce que le code voit aujourd'hui** :
  - classification : `kinds` seulement, avec `combinations = C(m,t)` pour une cellule étendue (`src/tower/cells_classify.cpp:294`) ou `birth ? 1 : qmin` en régulier (`src/tower/forest_build.cpp:157`). Le premier témoin y suffit, `S` n'est pas compté.
  - rejeu : `LocalCell::traces().size() = S` et `ledger.combinations = C(m,t)` (`src/tower/cells.hpp:68-70`, `src/tower/cells.cpp:207`). En régulier, `combinations += q` et `trace_resolutions += q` (`src/tower/forest_plateau.cpp:105-106`).
  - seuls des **totaux par ordre** sont conservés (`replayed_cells`, `trace_resolutions`, `unions`, `touched_components`, `continuations`, `src/tower/forest.hpp:91-102`), plus le nombre de graines par cellule régulière dans `seeds[4·job]` (`src/tower/forest_concurrent.cpp:43-47`).
  - le **nombre de composantes distinctes par boule n'est calculé nulle part** : le DSU déduplique silencieusement.
- **Proposition de champs publiables** avec chaque support : `kparts = C(m,t)` (u64, gratuit), `strict = S` (gratuit en rejeu, puisque `S = q` en régulier et `traces().size()` en étendu), et en option `components`.
  - Pour `components`, il faut, pour **chaque** graine, la composante d'avant le plateau. Pendant le plateau, on la trouve en remontant `parent` depuis la naissance jusqu'à `kNone` : les fusions de rang inférieur à `λ_b` sont déjà publiées, et celle du plateau ne l'est pas encore.
  - Une déduplication des racines DSU courantes cellule par cellule serait **fausse** : elle dépend des cellules du même plateau traitées avant et sous-estime le compte.
  - Le coût est de `S` remontées par cellule, sans borne de profondeur (`docs/FULL_FORESTS.md:95-97`).
- La même boule est critique à plusieurs ordres (`p+q−1 … p+m`). À K fixé, elle n'a qu'une cellule et un seul rattachement.

## 4. (d) Construire la forêt d'ordre K seule

### Dépendances réelles

- `ForestBuilder::run` exécute `classify → births → prepare_states → plateaus → finish` sur le seul domaine (`src/tower/forest_build.cpp:251-266`). Les descentes rendent des graines du **même** ordre (`BirthSeed(…, k)`, `src/tower/descent.cpp:89-102`), résolues par `result.birth_node` de la forêt en construction (`src/tower/forest_plateau.cpp:56`). **Aucune lecture des forêts inférieures.**
- `build_full` construit **toujours** les ordres 1..kmax et les verticales (`src/tower/forest_vertical.cpp:281-356`). `build_concurrent` fait de même (`src/tower/forest_concurrent.cpp:240-292`). Seules les verticales lisent la forêt `k−1` (`forest_verticals`, `src/tower/forest_vertical.cpp:199-210`).
- Options de `FullParams` (`src/tower/forest.hpp:117-127`) :
  - `reuse_regular_verticals` : ne sert qu'aux verticales. À l'ordre kmax, `remember` ne retient rien (`src/tower/regular_vertical_seeds.hpp:29-32`). Inutile pour K seul.
  - `population_lookup` : la table couvre `p+m ≤ K` (`src/tower/population_lookup.hpp:25-30`). Seules les entrées `p+m = K` peuvent servir à l'ordre K, puisque `hit` exige `|partie| = k`. La voie non liée `hit()` fonctionne avec un seul ordre (`src/tower/forest_parallel.cpp:175-182`). En revanche, `bind` **exige les K forêts** (`src/tower/population_lookup.hpp:55-59`), et la voie liée doit être restreinte à l'ordre K.
  - `concurrent_orders` / pipeline : `pipeline_lanes` rend 0 si `kmax < 2` (`src/tower/forest_pipeline.cpp:167-171`). La voie pipeline n'existe donc pas pour un seul ordre. La voie par lots (`ForestParallel::run`, `src/tower/forest_parallel.cpp:380-402`) fonctionne, mais c'est la voie historique à barrières (« ~600 barrières et ~330 ms de pilote seul sur G4 W48 », `src/tower/forest_concurrent.cpp:9`, mesure sur tous les ordres). Il faut une **variante pipeline à un ordre** : L voies de résolution, un publieur, zéro balayage.
  - `dense_birth_lookup`, `memo_capacity`, `reuse_census_workspace`, `descent_lanes` : indépendants de l'ordre.
- Propriétaire : `FullTower::order(k)` déréférence un `optional` sans contrôle (`src/tower/forest.hpp:222-223`). Il faut un nouveau propriétaire, par exemple « domaine + une forêt d'ordre K », ou un masque d'ordres explicite. `build_forest` emprunte le domaine (`src/tower/forest.hpp:205-211`).
- **Catalogue inchangé** : les jonctions d'ordre K exigent `p+q = K+1`, donc tout `Cat_K`. `locate` refuse une boule positive absente si `p+qmin ≤ K+1` (`docs/CELLS_AND_LOCATE.md:205`). Filtrer le catalogue sur la fenêtre de K demanderait un nouveau lemme.
- Les exports de points actuels lisent la forêt d'ordre 1 pour les singletons (`bench/points_export.cpp:306-313`). La sortie `supports` n'en a pas besoin.

### Coût relatif (reçu `receipts/qualification_performance_20261003/review/metrics.optimized.json`, producteur 16379, W48, u21, K5)

- Présentations MEB des descentes par ordre, `part_meb_presentations_by_k` :
  - ng00 : K2 = 64 721, K3 = 249 043, K4 = 833 989, **K5 = 2 639 192**, soit 69,7 % ;
  - ng01 : 69,2 % ;
  - ng02 : 68,9 %.
- Médianes à W48 : domaine (catalogue) 227 / 181 / 239 ms, forêts 241 / 164 / 197 ms, phase verticale globale 41,8 / 36,7 / 28,3 ms.
- **Estimation non mesurée** :
  - le catalogue reste entier ;
  - les forêts perdent les ordres 1..K−1 (environ 30 % du travail MEB de descente, plus les naissances et classifications, qui sont faibles) et toute la phase verticale ;
  - le gain mural dépend du recouvrement : le pipeline actuel superpose les ordres, si bien que le gain sera **inférieur** à la somme des parts ;
  - ordre de grandeur plausible : 30–45 % du temps forêts, c'est-à-dire environ 10–20 % du temps FULL. À mesurer sur G4.
- Mémoire : on ne garde qu'une forêt (`24(2b−1) + 4(2b−2)` octets plus le lookup, `docs/FULL_FORESTS.md:70-73`) au lieu de K, et il n'y a ni verticales (`4·Ncap`) ni `RegularVerticalSeeds` (`4B`).

## 5. (e) Deux façons d'enregistrer le rattachement boule → nœud

### E1 — Instrumenter le balayage des plateaux (recommandé)

- **Accroche minimale** : retenir **une graine de naissance par cellule**, jamais une racine ni un `top` (même doctrine que `src/tower/regular_vertical_seeds.hpp:35`). Les deux seuls endroits où une cellule est appliquée, dans les trois voies (sérielle, par lots, pipeline ou concurrente), sont :
  - `ForestBuilder::cell`, avec la graine de la première trace (`src/tower/forest_plateau.cpp:56`, `:58`) ;
  - `ForestBuilder::regular_cell`, avec `seeds.front()` (`src/tower/forest_plateau.cpp:100-117`).
- Ajouter un champ dédié dans `ForestBuilder` (`src/tower/forest_internal.hpp:380-440`). **Ne pas** réutiliser `representative`, qui n'est calculé que si `vertical_seeds != nullptr` (`src/tower/forest_plateau.cpp:58`) : le pipeline met `vertical_seeds` à `nullptr` (`src/tower/forest_pipeline.cpp:235`).
- **Résolution** :
  - variante E1a, dans `close` : juste après `close(level)`, poser `attach[b] = states[find(graine_b)].top` pour les boules du plateau. Coût O(α) par cellule, sans mémoire supplémentaire. Il faut suivre la **plage du plateau**, car la fermeture est différée par `select` dans la voie par lots (`src/tower/forest_parallel.cpp:330-343`, `:400`) et dans `publish` (`src/tower/forest_concurrent.cpp:36-42`, `:54`).
  - variante E1b, après `finish()` : un passage sur les boules dans l'ordre `BallIdx`, qui est déjà trié par rang. `ClosedAncestorSweep::advance(rang_b)`, puis `query(graine_b)` (`src/tower/forest_ancestor_sweep.hpp:35-67`). Coût O((B+N)·α) plus `12N` octets, pour un code indépendant des trois voies.
  - Les naissances prennent `birth_node`. Aucun `kinds` n'est nécessaire après coup : on a une naissance si `birth_key = b`, sinon la boule de la fenêtre est une cellule.
- **Champs supplémentaires gratuits** à l'enregistrement : `S` (`traces().size()` ou `q`) et `C(m,t)`. `components` est optionnel, voir § 3.
- **Avantages** : aucune descente supplémentaire ; exactitude par construction (c'est la même union DSU) ; déterminisme hérité, puisque les forêts sont identiques octet pour octet quel que soit W (`docs/PERFORMANCE_FULL.md:12-15`, `src/tower/forest_concurrent.cpp:8-9`) et que le nœud ne dépend pas de la graine.
- **Pièges** :
  1. stocker une racine DSU ou un `top` lu **pendant** le plateau donne l'ancien nœud au lieu de la fusion ;
  2. la fermeture est différée dans les voies par lots et pipeline ;
  3. le pipeline rend ses graines régulières bloc par bloc (`await_job`, `src/tower/forest_concurrent.cpp:46`, `:60-77`) : l'écriture de `attach` doit rester au seul publieur de l'ordre ;
  4. un abandon (`abandoned`, `src/tower/forest_pipeline.cpp:88-91`) ou un refus ne doit publier **aucun** rattachement partiel ;
  5. admettre `4B` octets dans le `MemoryBudget` avant l'allocation (doctrine de `docs/FULL_FORESTS.md:62-87`) ;
  6. K = 1 : naissances = sites (`birth_key = SiteIdx`), les supports n'y sont que des arêtes q2.

### E2 — Reconstruire après coup par descente et ancêtre fermé

- Le modèle existe déjà : `ball_nodes` dans `bench/points_export.cpp:178-214`. Une naissance se lit directement. Sinon, on descend la k-partie « intérieurs puis premiers sites de coquille » avec la borne `λ_b`, puis on prend `ancestor_closed(graine, rang_b)`.
- Pour la sortie `supports`, il suffit de remplacer le prédicat `strong` (`bench/points_export.cpp:131-133`, `p+q ≤ k`) par la **fenêtre** `p+q−1 ≤ k ≤ p+m`. La partie choisie pour une jonction régulière (`t = q−1 < q`) est une trace stricte : sa date initiale vaut moins de `λ_b`, et l'ancêtre fermé donne le bon nœud.
- **Avantages** : aucune modification du constructeur ; utilisable sur une forêt figée ou exportée ; **oracle différentiel naturel** pour E1.
- **Coûts** : une descente complète par cellule d'ordre K, soit environ `1/q` de la résolution régulière de l'ordre K (que E1 obtient gratuitement), plus les cellules étendues. `ancestor_closed` n'a pas de borne de profondeur (`docs/FULL_FORESTS.md:95-97`), mais il peut être remplacé par le balayage trié. Le parcours des cellules et descentes reste sériel s'il n'est pas distribué.
- **Pièges** :
  1. la date initiale doit être `≤ λ_b` (coupe fermée) et jamais remplacée par le niveau terminal (`src/tower/descent.hpp:95-97`) ;
  2. utiliser `rang_b` comme coupe **fermée**, jamais `rang_b − 1` ;
  3. les mémos et la table de populations changent le travail, pas le nœud.

### Juges et fixtures conseillés

- Juge borné : les composantes de `Γ_K(λ_b)` contenant les K-parties de `P_b`, calculées par l'oracle de définition (`definition.py` / `model.py`, `docs/FULL_FORESTS.md:101-106`), comparées au nœud publié.
- Fixtures d'égalité :
  - ligne `0,4,6,8,12` à K2 : naissances et non-naissance au même niveau 4 (`docs/FULL_BIRTH_RUNS.md:83-85`) ;
  - losanges : fusion à trois enfants à K3 (`docs/FULL_BIRTH_RUNS.md:74-81`) ;
  - une cellule passagère au niveau d'une fusion ;
  - une liaison interne qui ferme un cycle (continuation) ;
  - `{0,2,4}` à K2 : deux graines possibles, même nœud (`docs/MATHEMATIQUES.md:219-222`).
- Mutants :
  - lire `top` avant `close` ;
  - coupe stricte au lieu de fermée ;
  - stocker une racine au lieu d'une naissance ;
  - oublier les cellules de jonction (`strong` au lieu de la fenêtre).
- À l'échelle : invariants globaux. Chaque boule de fenêtre est rattachée exactement une fois. Rang du nœud = `rang_b` pour une naissance ou une fusion ; rang du nœud < `rang_b` < rang du parent pour une liaison interne. Toute fusion porte au moins une boule. Égalité E1 = E2 sur un échantillon.

## 6. Accroche vers les supports `Q_b` (hors périmètre, pour mémoire)

`positive_support` énumère par arité croissante et par ordre lexicographique, et **s'arrête au premier** support (`src/tower/canonical.cpp:15-52`). L'énumération de tous les `Q ∈ Q_b` réutiliserait ses trois prédicats sur toute la coquille : milieu (q2), triangle strictement aigu coplanaire au centre (q3), tétraèdre strictement intérieur (q4). Le coût est O(m^4) par boule, sans borne en K sur m (`docs/CELLS_AND_LOCATE.md:215-217`). En coquille régulière, `Q_b = {S*}`, déjà stocké dans `CatalogueBall::support`.
