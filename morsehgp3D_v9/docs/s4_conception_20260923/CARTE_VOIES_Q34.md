# Carte du code des voies q3/q4 (phase des survivants) : v9 @ a6d08f05

Cadre : `exploration_v9_hors_registre`, lecture seule, `public_status=not_claimed`. **GCP non utilisé.**

Les chiffres viennent des ledgers R13 : `vm/probe_0.stdout` (08/000000, K5, W48) et `vm/probe_2.stdout` (K10). Il y a une seule indication locale sans reçu (§6.3), signalée comme telle.

Abréviations : `P` = `morsehgp3D_v9/src/gen/pipeline/wspd_q34.cpp`, `L/` = `morsehgp3D_v9/src/gen/lanes/`.

## 1. Entrée et configuration

- **Options** (`src/chain/tower_chain.cpp:615–631`) :
  - Local28 et `Q4LocalOptions{}` par défaut (`L/q4_local.hpp:9–25`) : domaine Positive, profondeur 7, 4096 cellules, 512 tests, feuille ≤ 32 sites, découpage des événements.
  - En plus : `saturate_deep`, `retain_q3_fragments`, `GlobalBoxes`, `LiveOnly`, `q3_atlas_consultation`, `q3_leaf_census`.
- **Phase 3** (`P:1432–1460`) : 48 fils, blocs dynamiques de 64 arêtes, un `Engine` par fil.
  - `certified_edge` (`P:491–497`) reconstruit le cover **sans le compter**, car S3 l'a déjà compté.
  - Il appelle ensuite `generate_lanes` (`P:602–635`).

## 2. Séquence exacte pour une arête ouverte (a,b)

**E1. Cover** (`L/edge_cover.cpp:37–131`)
- Boule fermée |2z−a−b|² ≤ 4|b−a|².
- Parcours préfixe **sans pile** de l'index global, par les liens `escape`.
- Un bloc est borné par les extrema exacts de la norme sur sa boîte (`:104–124`). Une feuille reçoit le prédicat exact (`:53–60`).
- Arithmétique i64, < 2^40.
- Sortie : `vector<Range>` (16 o par plage, plages adjacentes fusionnées) et `site_count`.
- Les compteurs `Q34EdgeCoverWork` ne sont pas incrémentés sur ce chemin.

**E2. Atlas** (`P:609–614`)
- Il est construit seulement si q3 et q4 sont ouverts tous les deux.
- Pour une arête q4 seule, il est construit dans l'entrée q4 (`L/q4_local.cpp:838–853`).
- Pour une arête q3 seule, **il n'y a pas d'atlas**.

Étapes :
- **E2a. Géométrie** (`L/q4_local_partition.cpp:47–74`) : base entière (A,B) orthogonale à v=b−a, Gram < 2^33.
- **E2b. Domaine positif** (`L/q4_positive_domain.cpp:86–144`) : **2e parcours** de l'index.
  - Lentille fermée |z−a|² ≤ D et |z−b|² ≤ D, en i64 < 2^38.
  - Donne l'AABB exacte des complétions Z et leur population.
- **E2c. Enveloppe** (`L/q4_local_partition.cpp:159–215`) : 8 coins projetés, chaîne monotone, au plus 9 facettes (i128 < 2^119).
- **E2d. Décomposition du cover** (`:76–106`) : **3e parcours**. Produit `cover_nodes_`, les IDs des nœuds entièrement contenus dans une plage.
- **E2e. Quadtree des centres** (`L/q4_local.cpp:197–271`).
  - Cellules dyadiques du plan bissecteur (α,β)·2^20, racine [−2,2]².
  - DFS récursive à 4 enfants, profondeur ≤ 7, au plus 4096 nœuds.

Traitement de chaque cellule de E2e :
- `outside` (`L/q4_local_partition.cpp:217–241`) teste le disque 2|t|² ≤ |v|² et les facettes (i128 < 2^83 et < 2^81). Si le test réussit, la cellule est Outside.
- Sinon, un fragment reclasse la **frontière du parent** par une sous-DFS préfixe (`Q4LocalFragment`, `L/q4_local_partition.cpp:364–433` ; les lignes suivantes de ce point sont dans le même fichier).
  - Un singleton reçoit sa forme L_z (`:108–117`, i64 < 2^40), bornée sur la cellule (`:152–157`, i64 < 2^62).
  - Un bloc reçoit `node_bounds_unchecked` (`:250–286`) : 4 coins × 3 axes en i64, plus un carré i128 < 2^80 décalé de 20 bits.
  - Si max<0, le nœud est intérieur et son compte est hérité. Si min>0, il est extérieur. Sinon il reste actif ou il est scindé.
- Deux arrêts s'appliquent :
  - le **budget séquentiel de 512 tests** (`L/q4_local_partition.cpp:392–395`), qui garde actif ce qui n'est pas examiné ;
  - la saturation, dès que le compte atteint K−1 (`:409–413`) : la cellule devient Deep, sans fragment.
- États finaux :
  - **Deep** si le compte est ≥ K−2. Le fragment n'est gardé, pour q3, que si le compte est < K−1.
  - **Leaf** si les sites actifs sont ≤ 32, ou à la profondeur 7, ou au budget de nœuds. Un raffinement terminal sans budget classe alors les blocs non examinés (`L/q4_local.cpp:227–257`).
  - **Branch** sinon.
- Mémoire (estimation) :
  - `Node` ≈ 80 o.
  - Fragment `make_shared` ≈ 280 o, dont 160 o de compteurs, plus un `vector<size_t>` de frontière réservé à la taille du parent.
  - Le fragment est libéré après ses enfants, sauf en feuille.
- Si la racine a un compte ≥ K−1, la voie q3 est sautée (`P:616–617`). Aucun cas dans R13.

**E3. Graines q3** (`P:725–790`)
- **4e parcours**, sur l'index global et non sur le cover.
- Un bloc est rejeté si min|z−a|² > D, si min|z−b|² > D, ou si max(|z−a|²+|z−b|²) ≤ D (i64 < 2^39, `P:765–787`).
- Une feuille est acceptée à deux conditions :
  - le triangle est strictement aigu (`P:747`) ;
  - ab est l'arête propriétaire, c'est-à-dire la plus longue, les égalités étant départagées par la paire d'IDs triée (`P:750–756`).

**E4. Pour chaque graine q3** (`P:637–723`). Les graines sont indépendantes entre elles.
1. **Centre q3** (`L/q4_local_partition.cpp:119–140`) : rationnel i128 avec |x|, |y|, den < 2^117.
   - Il est localisé par `certified_cell` (`L/q4_local.cpp:311–348`) : division longue exacte en 20 doublements i128 (`:79–96`), puis descente sur au plus 7 niveaux.
   - Un compte certifié ≥ K−1 rejette la graine sans construire de boule (`P:649–652`).
2. **`ExactBall::make_q3`** (`L/exact_ball.cpp:85–105`) : Gram et W en i128, translation, puis **réduction primitive par pgcd i128** (`:41–58`). Bornes < 2^117.
3. **Recensement.**
   - Si la cellule est ExactLeaf, **recensement de feuille** (`P:662–684`). La profondeur vaut le compte certifié plus les sites de frontière de puissance < 0. La coquille est formée des sites de puissance 0, a, b et x compris. Arrêt à K−1.
   - Sinon (arête q3 seule, ou centre hors du domaine), **recensement global** `census_q3_ball` (`L/q3_ball_census.cpp:76–175`).
     - DFS depuis la racine de l'index, avec une pile fixe de 55 cadres.
     - Bornes min/max de la puissance sur une boîte en i128 (`:35–58`, < 2^117).
     - L'enfant de plus petit minimum passe d'abord, avec saturation à K−1.
     - Une seconde passe collecte la coquille ; elle n'apparaît pas dans le ledger.
4. **Émission** si la graine est acceptée : tri de la coquille et des IDs, puis émission (`P:714–722`).

**E5. q4 en mode LiveOnly** (`L/q4_local.cpp:855–868`, `825–835`)
- Un atlas sans feuille saute toute la voie (`whole_atlas_skips`).
- Le résumé `live` compte les feuilles vivantes par cellule (`vector<u64>`, passe ascendante `:614–639`).
- `live_only` (`:689–711`) fait le **5e parcours**, avec **le même prédicat de lentille que E3** (`spatial_pass :641–659`, `seed_pass :661–670`). Le test propriétaire y est payé deux fois (`:666` puis `:700`).
- Pour chaque graine x : `Q4FamilySeed::make` (`L/q4_family.cpp:52–77`), la forme de la droite de x, puis la descente `visit_live` (`:672–687`), qui teste la droite sur chaque cellule en i64.
- Pour chaque feuille croisée, `sweep` (`:374–468`) :
  - **Balayage des sites actifs.** `side` est calculé en i64 (< 2^57).
    - Si side = 0, `power` en i128 (< 2^116) classe le site en intérieur constant ou en coquille constante.
    - Sinon, avec le découpage, on calcule la forme, `intersection` (degré 4, i128) et `contains` (échelle 2^20).
    - Une racine hors de la cellule est comptée par `evaluate` au point de référence. Les autres deviennent des événements.
  - **Tri** des événements par `compare_roots` (6 mineurs, i128 < 2^97, `L/q4_family.cpp:94–108`), regroupement des racines égales, puis préfixe du compte intérieur.
  - **Présentations.** Pour un groupe possédé par la cellule avec un intérieur < K−2 (`:441`), on essaie les y dans l'ordre :
    - `owned` sur les 6 arêtes ;
    - `make_q4` positif (i128 < 2^117, avec pgcd) ;
    - canonicité : y < x avec aby aigu entraîne un rejet.
    - La première présentation valide est émise.
  - **Mémoire** : `events` et `shell` (`vector<size_t>`) sont réalloués à chaque arête ; `live` coûte 8 o par cellule.

## 3. Format émis

`Q34SeedCandidate` (`L/q34_seed.hpp:16–25`) :
- arité 3 ou 4 ;
- `support_ids[4]` triés (pour q3, le 4e vaut SIZE_MAX) ;
- `ExactBall` : 5 coefficients i128 primitifs {A, Bx, By, Bz, C}, avec A > 0 et pgcd 1 (clé exacte) ;
- `depth` exact (intérieur strict, jamais tronqué) ;
- `shell_first` et `shell_second` : IDs originaux triés. En q3, la coquille complète. En q4, les IDs du groupe de racine, puis la coquille constante, qui contient a, b et x.

Le consommateur de la tour (`tower_chain.cpp:633–641`) ne garde qu'une `Presentation` d'environ 112 o :
- clé `Key5` de 80 o ;
- arité ;
- 4 × u32 de support ;
- `depth` u32 ;
- **taille** de la coquille en u32.

La coquille est recalculée par le recensement de la tour, qui compare seulement sa taille (`:840`). Le produit n'a donc besoin que de (arité, support, clé, depth, taille de coquille). Le juge arête par arête, lui, compare les IDs de coquille.

## 4. Moyennes R13 (ledger)

Répartition des arêtes ouvertes :
- **K5** : 708 686 arêtes, soit 569 448 q3+q4, 132 230 q3 seules et 7 008 q4 seules.
- **K10** : 1 463 362 arêtes, soit 1 332 053 q3+q4, 105 368 q3 seules et 25 941 q4 seules.
- Il y a un atlas par arête q4 (576 456 à K5). 467 918 arêtes q4 sont effectivement balayées à K5.

| grandeur | par | K5 | K10 |
| --- | --- | ---: | ---: |
| CPU au plus (edges_ms × 48 / arêtes) | arête | 49,5 µs | 95,8 µs |
| sites du cover* | cover | 280 | 452 |
| visites de nœuds du cover* | cover | 137 | 172 |
| cellules d'atlas | atlas | 7,78 | 17,4 |
| dont feuille / Deep / Outside / Branch | atlas | 1,53 / 1,89 / 2,66 / 1,69 | 3,13 / 4,48 / 5,70 / 4,10 |
| visites de nœuds de l'atlas | atlas | 570 | 1 311 |
| tests de formes (singletons) | atlas | 462 | 1 081 |
| bornes de blocs (carré i128) | atlas | 96 | 201 |
| IDs de frontière copiés | atlas | 304 | 699 |
| visites du domaine / de la décomposition | atlas | 88,5 / 88,6 | 114 / 112 |
| visites du parcours de graines q3 | arête q3 | 111 (86 bornes, 25 points) | 150 |
| graines q3 | arête q3 | 13,3 | 23,2 |
| graines q3 sur q3+q4 / sur q3 seule | arête | 11,7 / **20,3** | 22,0 / **38,7** |
| graines localisées rejetées par l'atlas | graine | 68 % | 72 % |
| boules q3 construites | arête q3 | 6,83 | 8,49 |
| recensements de feuille (tests de points chacun) | — | 2,09 M (26,0) | 8,11 M (22,6) |
| part rejetée des recensements de feuille | — | 69 % | 65 % |
| recensements globaux (bornes / points chacun) | — | 2,70 M (41,5 / 2,8) | 4,09 M (47,9 / 4,2) |
| part rejetée des recensements globaux | — | **98,2 %** | 99,3 % |
| q3 émises | arête q3 | 0,99 | 2,02 |
| graines q4 | arête q4 balayée | 12,4 | 21,5 |
| visites du parcours de graines q4 | arête q4 balayée | 110 | 146 |
| sites actifs balayés | graine q4 | 14,6 | 19,9 |
| sites actifs balayés | arête q4 balayée | 182 | 429 |
| événements gardés et triés | graine q4 | 4,84 | 5,60 |
| q4 émises | graine / arête q4 | 0,027 / 0,275 | 0,066 / 1,28 |

\* Moyenne sur les 900 377 covers du lot S3. La reconstruction des 708 686 covers n'est pas comptée.

Identités vérifiées à K5 :
- graines = rejets par l'atlas (4 525 051) + rejets de feuille (1 447 279) + rejets globaux (2 654 872) + émises (691 284) ;
- recensements globaux = graines des arêtes sans atlas (2 683 058) + centres hors domaine (20 845).

Donc **99,2 % des recensements globaux viennent des arêtes q3 seules**.

## 5. Régulier ou irrégulier, et poids

**Régulier, data-parallèle :**
- **Entre arêtes** : indépendance totale ; seules les cases de sortie sont propres à chaque fil.
- **Parcours de l'index** : E1, E2b, E2d, E3 et le parcours de E5 sont cinq parcours préfixes sans pile, avec des prédicats de boîte en i64.
  - C'est la forme déjà portée en S3 (un warp par arête).
  - Ils totalisent environ 535 visites par arête q3+q4 (137 + 88,5 + 88,6 + 111 + 110), du même ordre que les 570 visites de l'atlas.
- **Balayages de feuille** : recensement de feuille q3 (26 puissances i128) et balayage q4 (14,6 sites par graine).
- **Travail par graine** : localisation, `make_q3`, famille q4. Les graines d'une même arête sont indépendantes.

**Irrégulier, récursif :**
- **E2e, l'atlas.**
  - Quadtree à arrêts dépendant des données : Outside, K−2, saturation K−1, ≤ 32 sites, profondeur 7.
  - Dans chaque fragment, une sous-DFS avec un **budget de tests séquentiel** et un arrêt précoce.
  - Le contenu des fragments, l'arbre des cellules et les compteurs dépendent donc de l'ordre des tests. Le flux émis, lui, n'en dépend pas.
  - Un port parallèle doit reproduire cet ordre (préfixe du nombre de tests) ou déclarer un nouveau ledger.
- **Recensement global q3** : DFS à pile ordonnée par minimum, sortie anticipée, seconde passe.
- **Balayage q4** : tri par `compare_roots` en i128, groupes, préfixe. Il reste petit (4,8 événements par graine) et émet rarement.

**Poids par comptes (K5).** À rapprocher des cycles locaux du reçu `q34_survivor_phases_20260923` : atlas 22 %, q3 22 %, q4 20 % à K5 ; 29 %, 15 %, 28 % à K10.
1. **Atlas** : 570 visites, 462 formes, 96 bornes de blocs et 304 copies par arête. Il pèse environ 2,3 fois plus à K10.
2. **Balayage q4** : 182 sites actifs par arête.
   - Chaque site coûte `side`, une forme, une intersection i128 et un `contains`.
   - S'y ajoutent 60 événements par arête, triés avec des comparaisons i128.
   - À K10 : 429 sites et 120 événements.
3. **Voie q3** :
   - 112 M bornes i128 dans les recensements globaux, dont 99 % sur les arêtes q3 seules, avec 98 % de rejets ;
   - 54 M puissances i128 dans les recensements de feuille ;
   - 4,79 M appels `make_q3` avec pgcd, pour 691 k boules émises.
4. **Cinq parcours de l'index** : environ 535 visites i64 par arête.

## 6. Constats utiles au port et leviers possibles (non mesurés sauf mention)

1. **Parcours dupliqués.**
   - Sur 569 k arêtes, E3 et E5 énumèrent le même ensemble de graines, aiguës et possédées : 11,7 et 12,4 graines par arête, environ 110 visites chacun.
   - E2b parcourt la même lentille, et E1 et E2d reparcourent l'index.
   - Une seule passe « lentille + cover » par arête peut produire les plages, `cover_nodes`, la boîte Z et la liste des graines. Les compteurs changeraient (à déclarer), pas le flux.
2. **Cover déjà calculé par S3.** Le noyau S3 construit le cover sur GPU mais ne renvoie que les masques (`Q34CertificateBatch`, `wspd_q34.hpp:330–341`). Le CPU reconstruit 708 686 covers sans les compter.
3. **`make_q3` avant le recensement.**
   - Il y a 4,79 M constructions pour 691 k émissions.
   - Le recensement n'a besoin que du signe, qui ne change pas quand on divise par un pgcd positif.
   - La forme relative non réduite suffirait : c'est exactement `Q4FamilySeed::power` à μ=0 (`L/q4_family.cpp:79–86`), sans pgcd. On ne réduirait qu'à l'acceptation.
   - Indication locale **sans reçu**, à mesurer avec reçu : `make_q3` ≈ 0,4–0,8 µs, puissance ≈ 10–23 ns, `Q4FamilySeed::make` ≈ 55 ns. Micro-banc `/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/s4-lanemap/bench.cpp`, sur un hôte à charge 17 pour 8 cœurs.
4. **Arêtes q3 seules.**
   - 132 k arêtes (19 %) produisent 2,68 M graines et presque tous les recensements globaux, avec 41,5 bornes i128 chacun et seulement 49 k acceptations.
   - Aucun atlas n'est construit pour elles (`P:610`).
   - Pistes candidates : un atlas du disque q3, ou un recensement local au cover.
5. **Fusion q3/q4 en feuille (hypothèse).** La boule q3 est le membre μ=0 de la famille q4 de x. Dans la feuille qui contient le centre q3, le balayage q4 calcule déjà `side` et `power` sur les mêmes sites actifs. Cela exige un théorème et une fixture ; rien n'est revendiqué.
6. **Allocations.**
   - Environ 20 à 30 allocations par arête : cover, géométrie, domaine, `Impl`, vecteur de nœuds, environ 5 fragments et leurs frontières, `events`/`shell`/`live`.
   - S'y ajoutent des compteurs atomiques de `shared_ptr`.
   - L'ensemble de travail fait quelques Ko (estimation), compatible avec un slab par warp comme en S3.
   - `peak_edge_buffer_bytes` (`P:520–529`) n'est pas imprimé par la sonde.
7. **Trous du ledger.** Pour calibrer un port, il manque :
   - la reconstruction du cover et la passe de coquille q3 ;
   - les `leaf_queries` q4, les comparaisons de tri et les groupes ;
   - les `make_q4`, le nombre de fragments et les pics mémoire.
8. **Commentaires périmés.** `L/q3_ball_census.hpp:54` annonce encore « <2^105 », et `:78–80` « local49-frame … three16-bit ». Le `.cpp` (`:52–56`, `:96–99`) porte les bornes justes à 18 bits : < 2^117 et 55 cadres.