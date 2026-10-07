# L06 — Code de la tour de `morsehgp3D_v10` : audit pour la conception de la v11

```text
sujet=morsehgp3D_v10 (HEAD origin/main afb081774), lentille L06 code de la tour
cible=morsehgp3D_v11 (conception)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant, lecture seule du dépôt
public_status=not_claimed
GCP non utilisé (aucune commande GCP ; les temps G4 cités sont lus dans les reçus du dépôt)
```

Rédigé le 2 octobre 2026, mesures prises entre 05 h 26 et 07 h 10 UTC (`date -u`), sur le codespace partagé
(AMD EPYC 7763, 8 vCPU, charge moyenne de 4 à 19 pendant les mesures).

## 0. Résumé

La tour v10 calcule le bon objet sur tout ce que j'ai pu juger, et de façon déterministe. La porte du dépôt
s'arrête à $K = 5$ et $n = 12$ ; je l'ai étendue : oracle $\Gamma_k$ jusqu'à $K = 10$ (17 nuages, 25 400 coupes,
0 écart, dont 7 nuages choisis pour que la descente saute aux ordres élevés), juge EMST indépendant de l'ordre 1
sur une trame entière (39 884 arêtes, égalité), empreinte canonique
identique sous une isométrie de la grille, sorties identiques à 1, 3 et 4 fils, et sorties identiques à l'octet
près avec une descente alternative valide à $K = 10$ sur une trame (1,78 million de sauts).

Trois défauts de fond pèsent sur la v11 :

1. **Le plancher séquentiel.** Le Kruskal par lots (une tâche séquentielle par ordre) et les images verticales des
   fusions (séquentielles par ordre) font 33 à 47 ms sur 67 à 89 ms à $K = 5$ et 89 à 140 ms sur 334 à 472 ms à
   $K = 10$, à 48 fils sur G4 (reçu de session 4). La conception `TOWER_v2` avait mesuré ce plancher et prescrit un
   noyau sans lots ; il n'a pas été implémenté. Un banc indépendant sur les données réelles donne un noyau sans lots
   1,6 à 1,8 fois plus rapide avec ses multifusions (forêt identique), et 2,6 à 3,2 fois sans la matérialisation des
   nœuds.
2. **Les coquilles étendues** sont traitées par énumération brute : 24 points cosphériques coûtent 49 à 57 s à
   $K = 5$ (statut `ok`) et un refus après 82 s à $K = 10$. Le LiDAR à 1 mm n'y est pas exposé (coquille maximale de
   5 sites), les données en grille le sont.
3. **L'entrée `cover` n'est pas une fonction de la géométrie** aux ex æquo : elle dépend du rang de Morton. Sur la
   trame 00, 47 sites changent de classe d'attache sous un échange d'axes ; l'entrée `core` est invariante.

S'y ajoutent une dette nette (un fichier de 1 867 lignes, une fonction de 601 lignes, aucun test unitaire ni mutant,
une porte oracle que traversent trois fautes injectées sur cinq, deux plantages de CLI reproduits) et un écart large
entre la conception `TOWER_v2` et le code.

## 1. Périmètre lu

Lu en entier, ligne à ligne, dans `/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/` :

| Fichier | Lignes | Rôle |
| --- | ---: | --- |
| `src/tower/tower.cpp` | 1 867 | toute la tour : MEB, quotient local, atlas, descente, Kruskal, attaches, verticales, dendrogramme de points |
| `src/tower/tower.hpp` | 153 | types publics `Tower`, `OrderForest`, `TowerParams`, compteurs |
| `src/tower/rank_search.hpp` | 37 | dichotomie à deux étages |
| `cli/mhgp10_tower.cpp` | 199 | sonde, JSON, dump texte |
| `cli/mhgp10_cluster.cpp` | 243 | consommateur (`only_order`, entrées `core`/`cover`) |
| `tests/oracle/test_tower_oracle.py` | 188 | porte $\Gamma_k$ |
| `tests/regression/*.py` (4), `tests/points/test_cover_entry.py`, `tests/unit/rank_search.cpp` | 263 + 133 + 71 | régressions, entrée `cover`, recherche de rang |
| `reference/hgp10_ref.py` | 431 | oracle exact Python |
| `docs/conception/TOWER_v2.md` | 1 636 | conception |
| `docs/SPEC_V10.md`, `PASSATION.md` | 93 + 235 | objet tel qu'implémenté, état |

Lus pour ce dont la tour dépend : `src/catalogue/catalogue.hpp`, `src/catalogue/support.hpp`, `src/arith/geometry.hpp`
et `.cpp`, `src/arith/wide.hpp`, `src/cloud/site_tree.hpp` et `.cpp`, `src/cloud/cloud.hpp` et `.cpp`,
`src/core/*`, `src/sched/pool.*`, `src/points/dendrogram.*`, `CMakeLists.txt`, `cmake/*.cmake`.

Reçus lus (jamais pris pour preuve sans contrôle) : `receipts/g4_session4_j2c_20260929` (sorties brutes
`results/cmd/*/stdout`), `receipts/g4_session5_scale_20260929/scale.csv`. Le code de la tour n'a changé que de
21 lignes (raccord de `rank_search`) entre le commit de ces reçus (`777406b82`) et le HEAD, d'après
`git diff --stat 777406b82 HEAD -- morsehgp3D_v10/src/tower/` ; les temps par étage des reçus s'appliquent donc au
code lu.

Hors périmètre : le générateur du catalogue, la tête de clustering, les bancs.

## 2. Méthode

- **Lu** : le code ci-dessus, en entier.
- **Exécuté** : build Release hors source du HEAD sous `/tmp/v11-audit/l06_code_tour/build-release` (aucun fichier
  créé dans l'arbre de lecture, `PYTHONDONTWRITEBYTECODE=1` pour les portes Python) ; portes CTest de la tour ;
  oracles étendus écrits pour l'audit ; juge EMST ; harnais des MEB ; ASan, UBSan et TSan sur de petites entrées ;
  cas limites ; six mutants et une descente alternative.
- **Mesuré** : trame du contrat `lidar00_full.u32le` (08/000000 sans sol, 39 885 sites) à $K = 5$ et $K = 10$, à 1
  et 4 fils ; compteurs déterministes ; copie instrumentée sous `/tmp` (`patch_instr.py`, `patch_tsc.py`) qui ajoute
  des compteurs sans changer aucune décision.
- **Temps** : les temps locaux sont bruités (machine partagée, charge 4 à 19). Les temps absolus de ce rapport
  viennent des reçus G4 ; en local je ne cite que des compteurs, des rapports entre variantes d'une même passe, et
  des minima de passes entrelacées.
- **Vocabulaire** : « prouvé » = argument écrit et relu ; « mesuré » = exécuté ici ou lu dans un reçu ;
  « conjecturé » = estimation, avec son fondement.
- **Non fait** : aucune session G4 ; aucun sanitizer à la taille d'une trame ; les portes
  `mhgp10_regression_batch_equivalence`, `mhgp10_regression_mreach_border`, `mhgp10_head_condensation_vs_sklearn` et
  `mhgp10_catalogue_oracle` n'ont pas été rejouées (elles jugent la tête, le témoin d'atteignabilité et le
  catalogue) ; le recouvrement du noyau par l'étage G n'a pas été prototypé.

Les preuves (scripts, journaux, empreintes) sont dans
`/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l06_code_tour/` (§ 10).

## 3. L'algorithme réel, reconstitué depuis le code

Objet : pour $k = 1, \ldots, K$, l'arbre de fusion des composantes de $L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace$ quand $a$ croît, où $D_k(y)$ est la $k$-ième plus petite distance carrée de $y$ aux sites. Tout est dans `src/tower/tower.cpp` ; les numéros de ligne s'y rapportent.

### 3.1 Entrées, ordre total, tri des niveaux

- La tour ne trie aucun niveau. Elle hérite l'ordre du catalogue : boules en ordre canonique (niveau exact, support
  canonique $S^{*}$), `cat.rank[b]` = rang dense des niveaux exacts distincts, `cat.level[rang]` = un représentant
  rationnel `num/den` par rang. La tour contrôle seulement que les rangs sont croissants (l. 1220) et que $I$ et $U$
  sont strictement croissants par boule (l. 1221-1228).
- Rang d'un nœud = `cat.rank + 1` ; le rang 0 est le niveau nul des sites à $K = 1$.
- Refus d'entrée : `kmax` hors de `[1, cat.kmax]` (l. 1153), nuage de plus de 18 bits (l. 1156), toute multiplicité
  (l. 1157-1158, raison `multiplicity_unsupported`).

### 3.2 Étage L : index des supports, atlas, structures locales (l. 1164-1400)

- **Index des supports** (l. 1167-1176) : table à adressage ouvert `FlatIndex` (l. 78-128), insertion concurrente par
  CAS, clé non stockée et vérifiée sur `cat.support[b]`. C'est `t_prepare`.
- **Fenêtre** d'une boule ($p$ = intérieur strict, $m$ = coquille, $q$ = `qmin`) : ordres
  $k \in [p + q - 1, p + m]$, restreints aux ordres construits (l. 1229-1241). Une **cellule** est un couple
  (boule, $k$) de la fenêtre ; `BallInfo` (12 octets) donne `base`, `lo`, `hi`, `p`, `q`, `m`, `ext`.
- **Coquille régulière** ($m = q$) : analytique. $k = p + m$ est une **naissance** ; $k = p + m - 1$ est une
  **jonction** à $m$ représentants $I \cup U \setminus \lbrace u \rbrace$ (l. 1249-1254, `join_rep` l. 1020-1036).
- **Coquille étendue** ($m > q$) : `local_structure` (l. 556-632) énumère les parties de $U$ de taille $t = k - p$,
  garde les **séparables** (centre hors de l'enveloppe convexe fermée, `center_in_closed_hull`,
  `src/catalogue/support.hpp:17-51`), puis unit $A \sim A'$ quand $A \cup A'$ est séparable. Aucune partie
  séparable : naissance. Un morceau : cellule inerte. Au moins deux : jonction, un représentant par morceau. Refus
  si $m > 24$ ou si plus de 20 000 parties sont séparables (l. 22-23).
- Deux passes parallèles par morceaux fixes de 8 192 boules (comptage, préfixes, remplissage, l. 1199-1392) : les
  naissances et jonctions de chaque ordre sont rangées dans l'ordre des boules, donc par (rang, $S^{*}$).

### 3.3 Semis $H_k$ (l. 1403-1435)

Pour chaque naissance régulière dont la population $I \cup U$ a exactement $k$ sites, la population triée est copiée
(`spop`) et indexée par hachage. Un représentant qui est exactement une telle population est résolu par une
consultation, sans géométrie. Mesuré : 85,9 % des représentants à $K = 5$, 86,5 % à $K = 10$ (trame 00).

### 3.4 Étage G : descente `resolve` (l. 799-993)

Fonction pure de la $k$-partie $F$ (le mémo n'est qu'un cache) :

1. $k = 1$ : le site lui-même (l. 815-818).
2. Semis : si $F$ est une population semée, sa naissance (l. 819-833).
3. MEB exacte de $F$ (l. 528-547) : proposition en double (`DWelzl`, l. 250-440), certificat exact `verify_meb`
   (l. 468-514), repli sur Welzl exact (l. 187-239). Mesuré : 0 repli sur 7,66 millions de MEB.
4. Garde I3 : le niveau décroît strictement, filtre flottant à marge puis comparaison exacte (l. 836-844).
5. Recensement $(I, U)$ de la sphère : lu au catalogue si le support certifié de la MEB est le support canonique
   d'une boule (l. 865-896), sinon boule fermée par l'arbre k-d (l. 897-904). Une boule du catalogue sur 32 est
   recensée des deux façons et comparée (l. 884-894).
6. $p \geq k$ : **saut** aux $k$ sites les plus proches du centre parmi $I$ (l. 907-934).
7. Sinon, support canonique puis recherche de la boule (l. 935-950). Si elle existe et que $k$ est dans sa
   fenêtre : cellule de naissance (arrêt), ou mémo (arrêt), ou cellule à mémoriser (l. 951-973).
8. Sinon, ou après une cellule non résolue : premier représentant local `first_rep` (l. 645-683), et on recommence.

Chaque cellule traversée reçoit la naissance trouvée (l. 991). Les représentants de toutes les jonctions de tous les
ordres sont résolus dans une seule boucle parallèle, par lots de 32 avec préchargement (l. 1441-1494).

### 3.5 Étage T : Kruskal par plateaux (l. 1041-1116, lancé l. 1498-1516)

Une tâche **séquentielle** par ordre. Les jonctions de même rang forment un lot : racines d'avant le lot de chaque
représentant, unions par minimum d'indice, puis un nœud N-aire par groupe d'au moins deux racines, enfants triés.
Numérotation : naissances dans l'ordre des boules, puis fusions par (rang, plus petite naissance du sous-arbre).
Une seule racine exigée (`root_count`). Les pointeurs de saut de Myers sont bâtis dans la même tâche (l. 721-739).

### 3.6 Étage P : attaches des points (l. 1520-1632) et recherche de rang

- **`core`** (l. 1600-1627) : une requête des `kq` plus proches par site, puis par ordre la descente des $k$ plus
  proches, et l'ancêtre au rang « nombre de niveaux du catalogue $\leq D_k(x)$ ». Ce rang vient de `RankIndex`
  (l. 1004-1017) et de `rank_search::at_most` : dichotomie exacte sur un niveau sur 64, puis dans le bloc.
- **`cover`** (l. 1541-1599) : pour chaque site, la **première** boule du catalogue (plus petit indice) qui le
  contient et pèse au moins $k$ (+ `cover_extra`) ; le site entre au niveau de cette boule, dans la composante de
  son centre. Seules les premières boules sont résolues, sauf `ball_nodes`.

### 3.7 Étage V : verticales $k \to k - 1$ (l. 1658-1747)

- Naissance régulière : la descente du dernier représentant de la jonction de la même boule à l'ordre $k - 1$ est
  déjà connue (`ball_vnode`, l. 1486) ; l'image est son ancêtre au rang de la naissance. Boucle parallèle.
- Fusion : ancêtre au rang de la fusion de l'image de chaque enfant ; tous doivent s'accorder
  (`vertical_naturality`). Boucle **séquentielle** par ordre, dans l'ordre de création (l. 1709-1735).

### 3.8 `point_dendrogram` (l. 1753-1865)

Fusion exacte des rangs du catalogue (tri par dénombrement) et des niveaux entiers $D_k(x)$, publiée en doubles
strictement croissants ; deux niveaux exacts distincts dont les doubles coïncident partagent un rang (régression
`level_collision`).

### 3.9 Confrontation à la conception `TOWER_v2`

`docs/SPEC_V10.md` § 4 décrit fidèlement le code. `docs/conception/TOWER_v2.md` décrit un autre programme :

| Sujet | `TOWER_v2` | Code |
| --- | --- | --- |
| Découpage | 15 fichiers, aucun au-delà de 400 lignes (§ 15.1) | un fichier de 1 867 lignes |
| Forêt d'un ordre | noyau sans lots T2a, classement de liste T2b, règle cartésienne T2c (§ 6.5, D-T5) | Kruskal par lots séquentiel |
| Requête d'ancêtre | index d'intervalles `WA`, blocs de 16 (§ 6.6) | pointeurs de saut de Myers sur les parents |
| Minima | étage M par sauts de pointeurs (§ 6.3) | mémo par cellule, rempli pendant les descentes |
| Arêtes | étage T1, hyperarêtes triées (§ 6.4) | aucune ; le Kruskal lit les représentants |
| Saut hors catalogue | `knn_closed(c, kcat)` borné (§ 6.2) | boule fermée entière puis sélection |
| Coquilles | fenêtres circulaires en $O(u \log u)$, table 3D jusqu'à 16 (§ 5.3) | énumération brute jusqu'à 24 |
| Multiplicités | natives, Euler pondéré (§ 3.9, D-T14) | refusées |
| Euler (I2), sphère manquante (I5), I4, I7, I9, I10 | refus de produit (§ 8.1) | absents |
| Contrôles | modes `full` et `sampled` (§ 8.2) | juge 1/32 et naturalité complète toujours actifs |
| Sorties | ancres, contributions datées, localisateur de facettes (§ 5.7, § 6.6) | parents, rangs, images, attaches |
| Empreintes | `tower_digest_v10`, `tower_merkle_v10` (§ 6.7) | aucune |
| Ordonnancement | par $K$ décroissant, T2a recouvert par G (§ 11) | étages globaux, l'un après l'autre |
| Juges et mutants | J-CENSUS, J-DESC, J-MF, EMST, 24 mutants (§ 13) | aucun |

Ce que le code a bien pris de la conception : cellules et fenêtres, quotient de Gordan, descente pure par saut
K-NN à décroissance stricte, semis, MEB proposée en double puis certifiée, multifusions jamais binarisées,
numérotation (rang, plus petite feuille), échec transactionnel.

## 4. Constats

Gravité : **bloquant** = rend faux ou invalide un résultat ou un contrat ; **majeur** = à traiter dans la conception
de la v11 ; **mineur** ; **info**. Chaque constat dit comment il a été établi.

### 4.1 Exactitude

#### L06_CODE_TOUR-01 — info — L'objet est conforme sur des contrôles étendus au-delà des portes

**Fait.** Aucun écart, sur neuf contrôles, dont huit indépendants des portes du dépôt :

| Contrôle | Portée | Résultat |
| --- | --- | --- |
| Portes CTest de la tour au HEAD | `mhgp10_unit`, `mhgp10_rank_search`, `mhgp10_tower_oracle`, `mhgp10_points_cover`, `mhgp10_regression_level_collision`, `mhgp10_regression_multiplicity_refusal` | 6 sur 6 vertes (oracle : 272 s sous charge) |
| Oracle $\Gamma_k$ étendu aux ordres 6 à 10 | 10 nuages de 12 points, 5 familles, $K = 10$ | 11 654 coupes, 0 écart |
| Oracle $\Gamma_k$ sur nuages **choisis pour sauter** | famille « noyau serré et halo » de 14 points : 7 nuages retenus parmi 1 800 pour leurs sauts K-NN aux deux derniers ordres ; 3 à $K = 6$, 2 à $K = 8$, 2 à $K = 10$ | 13 746 coupes, 0 écart ; 546 sauts exercés, dont 252 aux ordres 6 à 10 |
| Juge EMST de l'ordre 1 | trame 00, 39 885 sites | multiensemble des niveaux de fusion = multiensemble des $d^{2}/4$ des 39 884 arêtes de l'arbre couvrant minimal (Delaunay flottant de Qhull, poids entiers exacts, Kruskal de scipy : juge indépendant, non certifié) |
| Isométrie de la grille | trame 00, $K = 5$, échange des axes $x$ et $y$, miroir de $z$ | empreintes canoniques de la forêt, des verticales et des attaches `core` identiques aux 5 ordres |
| Déterminisme | trames 00 et 02 à $K = 5$ (`core`, `cover`) à 1, 3 et 4 fils ; trame 01 à $K = 10$ à 1 et 4 fils | `sha256` des dumps identiques |
| MEB | les deux chemins de MEB de `tower.cpp` (repli Welzl exact ; proposition certifiée), appelés par un harnais, contre la force brute en `Fraction` : 2 100 ensembles de 2 à 10 points (grille, 24 points cosphériques, cocycliques, plans, alignés, génériques) | 0 écart ; 0 repli du chemin produit |
| ASan, UBSan, TSan | 9 petites entrées (dont grilles et coquilles étendues), $K = 5$ et $K = 10$, `core` et `cover`, 2 à 4 fils | 27 exécutions ASan et UBSan et 9 exécutions TSan sans aucun rapport |
| Différentiel de descente | une descente **alternative valide** (saut vers $k$ sites intérieurs quelconques, représentant du dernier morceau, derniers sites de la coquille), trame 00 à $K = 5$ et trame 01 à $K = 10$ (13,7 millions de descentes, 1,78 million de sauts) | dumps identiques à l'octet près à ceux du HEAD |

S'y ajoutent : forêts des ordres 1 à 5 identiques que le catalogue soit bâti à $K = 5$ ou à $K = 10$ (mêmes
empreintes) ; 0 repli de MEB et 0 sphère de fenêtre absente du catalogue sur la trame 00 à $K = 5$ et $K = 10$.

**Égalités de niveaux, plateaux, collisions.**

- *Ordre total* : (niveau exact, $S^{*}$) du catalogue, puis (rang, plus petite naissance) pour les fusions. Aucun
  départage ne dépend de l'ordre d'arrivée des fils.
- *Plateaux* : les jonctions de même rang exact forment un lot dont les racines sont lues **avant** toute union ;
  un groupe d'au moins deux racines devient un seul nœud N-aire (l. 1071-1111). Une naissance ne peut pas être
  jointe à son propre niveau : un représentant descend strictement (prouvé, garde I3 exécutée à chaque pas). Sur la
  trame 00 à $K = 5$, 63 522 lots sur 1 200 903 contiennent plusieurs jonctions, jusqu'à 48 représentants ; à
  l'ordre 5, 576 371 nœuds se partagent 560 383 niveaux exacts distincts.
- *Familles à égalités* : la grille $\lbrace 0, \ldots, 3 \rbrace^{3}$ et les nuages coplanaires de l'oracle
  étendu sont conformes aux dix ordres.
- *Collisions de doubles* : `point_dendrogram` donne le même rang à deux niveaux exacts distincts dont les doubles
  coïncident ou s'inversent (l. 1828-1840) ; l'ordre exact des nœuds et des attaches est conservé, seul le rang
  publié est partagé. La porte `mhgp10_regression_level_collision` est verte. Sur la trame 00 à $K = 5$, aucun des
  niveaux exacts ne collisionne en double (0 collision sur les cinq ordres, mesuré sur le dump).

**Preuve.** `preuves_l06_code_tour/oracles/` (`ctest_portes_tour.log`, `oracle_k10.py` et son journal,
`oracle_sauts.py` et ses trois journaux, `oracle_amas.log`, `emst_judge.py` et son journal),
`isometrie/merkle_iso.log`, `isometrie/anchors.jsonl`,
`determinisme/determinism.txt`, `mutants/descente_alternative_resultats.txt`, `meb/welzl_judge.log`,
`sanitizers/sanitizers.txt`, `instrumentation/instr_l00_k5_t1.err` (ligne `L06_LOOKUP2`).

**Vérification.** Exécuté.

**Limite.** À 12 points tirés sans précaution, les ordres 6 à 10 n'exercent presque pas la descente : 2 sauts
K-NN en tout sur les 10 nuages (journal de `oracle_k10.py`) ; quatre nuages à deux amas n'en font aucun aux ordres
5 et 6. Il faut choisir les nuages : la famille « noyau serré et halo » saute dans 459 à 599 tirages sur 600. Le
régime du LiDAR à $K = 10$ (2,05 millions de sauts, chaînes jusqu'à 12 pas, constat 05) reste hors de portée d'un
oracle exhaustif. Le différentiel de descente l'exerce : il montre que deux chemins de descente distincts
aboutissent aux mêmes composantes, pas que ces composantes sont justes. Pour $K \geq 2$ il n'existe aucun juge
indépendant à l'échelle.

**Conséquence v11.** La sémantique (cellules, fenêtres, quotient de Gordan, descente, lots) se porte. Les
empreintes de `anchors.jsonl` servent d'ancres v10 pour la campagne appariée v10/v11.

#### L06_CODE_TOUR-02 — majeur — L'entrée `cover` dépend du rang de Morton aux ex æquo

**Fait.** En entrée `cover`, un site est attaché à la composante de sa première boule couvrante, « première »
signifiant « plus petit indice de boule » (l. 1561-1570, attache l. 1584 et 1595). Deux boules couvrantes de même
niveau sont départagées par leur support canonique, donc par le rang de Morton des sites. Or, dans tous les cas
mesurés, deux boules couvrantes de même premier niveau sont à ce niveau dans des composantes **différentes** :
l'attache choisie n'est pas déterminée par la géométrie.

**Preuve mesurée.**

- Trame 00, $K = 5$ : 34, 53, 19 et 20 sites (ordres 2 à 5) ont au moins deux boules couvrantes au premier niveau,
  et pour **tous** ces sites elles tombent dans des composantes distinctes. Trame 02 : 89, 151, 85 et 86 sites
  (`instrumentation/instr_lidar00_k5_cover.err`, `instr_lidar02_k5_cover.err`, lignes `L06_COVER_TIES`).
- Sous l'isométrie (échange $x \leftrightarrow y$, miroir de $z$), 13, 25, 0 et 9 sites changent de classe
  d'attache aux ordres 2 à 5, et le nombre de classes change (24 259 contre 24 260 à l'ordre 3). Les niveaux
  d'entrée, eux, sont égaux. En entrée `core`, 0 site change (`isometrie/iso_cover.log`, `iso_core.log`).
- Petits nuages : 83 attaches ambiguës sur 20 nuages de 10 points ; la tour rend toujours l'une des composantes
  candidates (`oracles/oracle_cover_n10.log`).

**Vérification.** Lu, puis exécuté et mesuré.

**Conséquence v11.** La hiérarchie de points en entrée `cover` n'est pas un objet défini tant que la règle aux
ex æquo n'est pas écrite. Elle doit l'être avant de comparer à HDBSCAN (contrat 3), puis gravée en fixture (deux
voisins équidistants à $K = 2$) et jugée par une porte d'isométrie.

#### L06_CODE_TOUR-03 — majeur — Les portes de la tour sont étroites et peu sensibles

**Fait lu.**

- `mhgp10_tower_oracle` : 24 nuages tronqués à 12 points, $K \in \lbrace 1, 3, 5 \rbrace$, plus la fixture E5 à
  $K = 4$, soit 73 exécutions, `--threads=2` seulement ; un seul plancher (500 coupes). Les ordres 6 à 10 ne sont
  jamais jugés.
- Elle compare, à chaque niveau critique, le nombre de composantes vivantes et la partition des points entrés, en
  **relevant elle-même** les nœuds publiés (`top()`, `tests/oracle/test_tower_oracle.py:83-87`). Elle ne lit donc ni
  l'arité des fusions, ni la hauteur du nœud d'attache, ni la hauteur de l'image verticale.
- Le contrôle des verticales ne voit que les composantes qui contiennent un point entré (l. 90-114 du script) et
  ne compte dans aucun plancher (`cuts += 0 * c`, l. 153).
- `tests/points/test_cover_entry.py` juge le niveau d'entrée `cover`, pas la composante d'attache.
- Le déterminisme multi-fils n'est jugé que sur des étiquettes de clustering à 2 000 points
  (`tests/regression/test_batch_equivalence.py`) et sur l'arbre de points en `cover`, jamais sur la tour.
- Aucun test unitaire de la tour, aucun mutant, aucune porte aux tailles d'intérêt (aucun label `scale*`).
- Le repli Welzl exact (l. 187-239) n'est atteint par aucune porte : 0 repli sur 7,66 millions de MEB de la trame 00
  et sur 2 100 ensembles dégénérés. Je ne l'ai exécuté qu'en l'appelant depuis un harnais (constat 01).

**Fait exécuté : six mutants** de `tower.cpp`, compilés dans une copie sous `/tmp`, soumis à la porte du dépôt sur
12 nuages (37 exécutions, 12 056 coupes ; le témoin non muté rend 0) :

| Mutant | Faute | Porte oracle | Tué par |
| --- | --- | --- | --- |
| `BINARIZE` | multifusion remplacée par une chaîne de fusions binaires de même rang | **survit** | aucune porte : les 11 autres portes CTest du dépôt passent aussi ; sur la trame 00 la tour mutée rend `ok` avec 341 080 fusions à l'ordre 5 au lieu de 235 290 |
| `POINT_OPEN` | attache `core` un rang trop bas | **survit** | `validate()` du dendrogramme, mais dans `mhgp10_cluster` seulement (code 3, `rank_order`) |
| `VERT_OPEN` | image verticale d'une naissance sans remontée au niveau de la boule | **survit** | aucune porte : les 11 autres portes CTest du dépôt passent aussi |
| `JUMP_ANY` | saut vers $k$ sites intérieurs quelconques (plus petits indices) au lieu des $k$ plus proches | survit | mutant **équivalent** : sur la trame 00 à $K = 5$ le dump est identique à l'octet près au témoin (même `sha256`), pour 3 % de MEB et 7 % de sauts en plus |
| `DROP_LAST_REP` | dernière union omise dans les jonctions à trois représentants ou plus | tué | invariant `root_count` de la tour (tous les écarts journalisés) |
| `WINDOW_LO` | fenêtre basse $p + q$ | tué | invariant `root_count` de la tour |

Trois fautes réelles sur cinq traversent la porte. Les deux mutants tués le sont par l'invariant « une racine par
ordre » de la tour elle-même.

**Preuve.** `preuves_l06_code_tour/mutants/` (`make_mutants.py`, `run_mutants.sh`, `mutants_results.txt`,
`mutant_point_open_validate.txt`, `mutants_sur_trame00.txt`, `autres_portes_contre_mutants.txt`).

**Vérification.** Lu, exécuté.

**Conséquence v11.** Les propriétés « multifusions jamais binarisées » et « image verticale vivante à la coupe
fermée » n'ont aujourd'hui aucune porte ; « nœud d'attache vivant à la coupe fermée » n'est gardée que par
`validate()`, hors de la sonde de la tour. Le mutant équivalent montre en revanche une liberté de conception : la
sortie ne dépend pas de la règle du saut, qui peut être choisie pour son coût (§ 9.4 pour les portes).

### 4.2 Performance

#### L06_CODE_TOUR-04 — bloquant pour le contrat de temps — Plancher séquentiel de la tour

**Fait.** Deux boucles sont séquentielles par ordre et se trouvent sur le chemin critique : le Kruskal par lots
(l. 1041-1116, une tâche par ordre, l. 1503-1515) et les images verticales des fusions avec leur contrôle de
naturalité (l. 1709-1735). À 48 fils, le mur de ces étages est celui de l'ordre le plus chargé.

**Preuve mesurée (reçu G4, session 4, 48 fils, CPU seul).** Sorties brutes
`receipts/g4_session4_j2c_20260929/results/cmd/0{08..13}_tower_*/stdout`, extraites dans
`preuves_l06_code_tour/g4/extraction_recu_g4_session4.txt` :

| Trame, $K$ | Tour (ms) | G `t_resolve` | T `t_kruskal` | V `t_vertical` | Plancher séquentiel (ordre max : T + V) | Part |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 00, 5 | 88,5 | 28,6 | 31,7 | 17,6 | 31,6 + 15,1 = 46,7 | 53 % |
| 01, 5 | 67,3 | 22,2 | 20,9 | 14,8 | 20,8 + 12,5 = 33,3 | 49 % |
| 02, 5 | 89,3 | 27,1 | 27,5 | 21,5 | 27,5 + 18,7 = 46,2 | 52 % |
| 00, 10 | 472,0 | 242,9 | 90,4 | 64,8 | 90,3 + 49,6 = 139,9 | 30 % |
| 01, 10 | 333,9 | 179,3 | 52,4 | 47,5 | 52,3 + 36,4 = 88,7 | 27 % |
| 02, 10 | 406,2 | 203,4 | 66,4 | 64,1 | 66,4 + 49,7 = 116,1 | 29 % |

Avec une infinité de fils, la tour v10 ne descend pas sous 33 à 47 ms à $K = 5$ ni sous 89 à 140 ms à $K = 10$. À
$K = 10$, ce plancher seul prend 89 à 140 % du contrat de 100 ms, avant le catalogue (528 à 653 ms dans le même
reçu) et avant l'étage G.

**Preuve mesurée (banc indépendant, local).** J'ai vidé les entrées réelles du Kruskal (représentants résolus de la
trame 00) et rejoué trois variantes dans des passes entrelacées (`krbench/krbench.cpp`, minimum de 15 passes,
forêts comparées par signature canonique) :

| Ordre | Kruskal par lots (copie fidèle) | Noyau union-find arête par arête, sans lots ni nœuds | Noyau + multifusions à la volée | Forêt |
| --- | ---: | ---: | ---: | --- |
| 5 ($K = 5$) : 341 081 naissances, 1 350 122 représentants | 31,7 ms | 11,8 ms (÷ 2,7) | 19,7 ms (÷ 1,6) | identique |
| 10 ($K = 10$) : 979 350 naissances, 3 831 401 représentants | 90,6 ms | 34,8 ms (÷ 2,6) | 56,6 ms (÷ 1,6) | identique |

Une seconde série, sous charge plus forte, donne ÷ 3,1 à 3,2 et ÷ 1,7 à 1,8. La copie fidèle reproduit en local le
temps G4 de l'ordre maximal (31,7 contre 31,6 ms ; 90,6 contre 90,3 ms) : les rapports sont transposables.

**Lu.** `TOWER_v2` § 0 et § 6.5 avait mesuré ce coût (27 à 47 ms à $K = 5$) et décidé un noyau sans lots (D-T5),
recouvert par l'étage G des ordres inférieurs (§ 11.2). Rien de cela n'est dans le code. Les images des fusions
n'ont besoin d'aucun ordre séquentiel : l'image d'une fusion est l'ancêtre, au rang de la fusion, de l'image de
n'importe quelle naissance de son sous-arbre (c'est D-T8) ; le Kruskal connaît cette naissance (la racine par
minimum d'indice).

**Vérification.** Lu (code, conception), mesuré (reçu G4, banc local).

**Conséquence v11.** Concevoir dès le départ : (i) un noyau sans lots, (ii) son recouvrement par l'étage G, ce qui
exige un ordonnanceur capable de mener une tâche longue à côté d'une boucle parallèle, (iii) des verticales
parallèles par nœud, la naturalité passant dans les portes. Gain estimé au § 9.

#### L06_CODE_TOUR-05 — majeur — Étage G : où part le temps, et trois gaspillages

**Fait mesuré (trame 00, 1 fil, compteurs déterministes).**

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| Boules, cellules | 1 306 696 ; 2 164 763 | 5 512 670 ; 9 887 430 |
| Représentants résolus | 3 621 560 | 17 388 593 |
| Arrêts sur un semis | 3 111 203 (85,9 %) | 15 046 831 (86,5 %) |
| MEB exactes (replis) | 1 084 548 (0) | 7 656 889 (0) |
| Sauts K-NN | 275 230 | 2 049 772 |
| Boules fermées par l'arbre | 285 207 | 1 832 815 |
| Recherches de boule par support | 1 146 128 | 8 052 165 |
| Chaîne la plus longue (pas ; sauts) | 8 ; 6 | 12 ; 10 |

Répartition des cycles de `resolve` (compteur `rdtsc` par segment, copie instrumentée ; les rapports seuls sont
fiables) :

| Segment | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| Hachage et semis | 15,0 % | 9,6 % |
| MEB | 28,5 % | 37,4 % |
| Recherche de boule, recensement lu au catalogue, garde I3, juge 1/32 | 15,4 % | 12,8 % |
| Boule fermée par l'arbre | 26,3 % | 24,1 % |
| Sélection du saut | 3,8 % | 5,5 % |
| Support canonique, cellule | 6,2 % | 5,5 % |
| Premier représentant | 4,9 % | 5,1 % |

85 à 90 % des cycles de `resolve` vont aux pas qui ne s'arrêtent pas sur un semis, soit 14 % des représentants.

**Trois gaspillages, lus puis comptés.**

1. **Boule fermée entière là où $k$ plus proches suffisent.** Le saut n'a besoin que de $k$ sites strictement
   intérieurs ; le code recense et trie toute la boule (`SiteTree::closed_ball`, `src/cloud/site_tree.cpp:183-227`).
   Sur la trame 00, la taille moyenne est de 10,1 sites à $K = 5$ et 16,4 à $K = 10$, mais le maximum de 1 201 et
   1 258 sites, soit 3 % du nuage pour une requête. Sur une famille à fort contraste (un noyau dense et un halo
   épars), une requête rend **la moitié du nuage** pour en retenir 5 sites : 2 007, 4 006, 7 725 puis 15 881 sites à
   4 000, 8 000, 16 000 et 32 000 points (`instrumentation/contraste_noyau_halo.txt`). Le coût d'un pas est donc
   en $\Theta(n)$ au pire. La moyenne reste petite (9 à 16 sites) : le total mesuré reste linéaire sur cette
   famille, et un total quadratique est conjecturé, non exhibé. La conception prévoyait `knn_closed(c, kcat)`.
2. **Seconde recherche vouée à l'échec.** Quand le support certifié de la MEB n'est pas au catalogue et que la
   coquille est ce support, le code recherche une seconde fois la même clé (l. 936-949) : 62 056 des 62 057
   secondes recherches à $K = 5$, 397 450 des 397 487 à $K = 10$, pour 1 et 37 succès.
3. **Re-vérification d'un théorème dans le chemin produit.** Le juge de recensement 1/32 (l. 884-894) fait
   26 436 des 287 195 requêtes d'arbre à $K = 5$ (9,2 %) et 190 237 sur 1,84 million à $K = 10$ (10,3 %).

**Preuve.** `mesures/run_l00_*.json`, `instrumentation/instr_l00_k5_t1.err`, `instr_l00_k10_t2.err` (lignes
`L06_CB`, `L06_JUMP`, `L06_LOOKUP2`, `L06_CHAIN_JOIN`), `tsc_l00_k5.err`, `tsc_l00_k10.err`.

**Vérification.** Lu, exécuté, mesuré.

**Conséquence v11.** L'étage G est un lot d'évaluations pures et indépendantes : c'est la partie qui se prête au
GPU. Sur CPU, les trois corrections ne rendent que quelques pour cent en moyenne (conjecturé : 3 à 5 % de G) ;
leur intérêt est de **borner** le coût par pas. Toute sphère hors catalogue se traite par une seule requête des $k$
plus proches du centre (clé exacte, puis indice) : c'est le même ensemble que le « premier représentant » sous la
fenêtre (prouvé : sous la fenêtre, toute partie de taille $\leq q - 1$ est séparable, donc la première partie
séparable est formée des premiers sites de la coquille).

#### L06_CODE_TOUR-06 — majeur — Coquilles étendues : énumération brute, coût explosif, refus tardif

**Fait.** `local_structure` (l. 556-632) énumère les $\binom{m}{t}$ parties de la coquille, teste chacune par
`center_in_closed_hull` (paires, triangles, tétraèdres : $O(t^{4})$ prédicats larges), puis teste les paires de
parties séparables ($O(s^{2})$ tests). Les seuls garde-fous sont $m \leq 24$ et $s \leq 20\,000$, constatés
**pendant** l'énumération. Il n'y a ni budget a priori, ni borne de temps.

**Preuve mesurée** (`limites/edge_results.txt`, binaire Release du HEAD, 2 fils) :

| Entrée | $K$ | Issue | Tour | Dont étage L |
| --- | ---: | --- | ---: | ---: |
| 24 points cosphériques (permutations de $(\pm 2, \pm 1, 0)$) | 5 | `ok` | 49 à 57 s | tout |
| les mêmes | 10 | refus `shell_quotient_budget` (ordre 6) | 82 s | tout |
| 16 de ces points | 10 | `ok` | 3,7 à 3,8 s | tout |
| 24 points cocycliques ($x^{2} + y^{2} = 325$) et le centre | 5 | `ok` | 0,82 à 0,92 s | tout |
| grille $6^{3}$ (216 points) | 10 ; 11 ; 12 | `ok` ; `ok` ; délai dépassé | 0,58 s ; 4,9 à 7,5 s ; plus de 120 s | 0,52 s ; 7,3 s |
| grille $10^{3}$ (1 000 points) | 5 | `ok` | 0,20 à 0,23 s, soit 8 à 9 µs par boule | 0,18 s |

Sur le LiDAR à 1 mm, le chemin est marginal : 227 boules étendues sur 1,31 million à $K = 5$, 444 sur 5,51 millions
à $K = 10$, coquille maximale de 5 sites (`L06_EXT`).

**Vérification.** Lu, exécuté.

**Conséquence v11.** Le contrat 1 (LiDAR) n'est pas touché. Le contrat 3 l'est dès qu'une donnée synthétique ou
réelle est en grille, plane ou quantifiée grossièrement. Il faut soit un quotient polynomial (fenêtres angulaires
pour les coquilles coplanaires au centre, table par masques pour le 3D, comme `TOWER_v2` § 5.3 le prévoyait), soit
un refus typé décidé **avant** tout calcul, sur $\binom{m}{t}$.

#### L06_CODE_TOUR-07 — info — Mémoire : la tour ne fait pas le pic, le budget compté est nominal

**Fait mesuré.**

| Trame 00 | Pic RSS local (1 fil ; 4 fils) | Pic RSS G4 (48 fils) | RSS après la tour (local) | Tableaux de travail de la tour | Forêts publiées | Pointeurs de saut |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| $K = 5$ | 553 à 607 Mo ; 694 à 719 Mo | 728 Mo | 347 à 487 Mo | 109 Mo (84 o par boule) | 39 Mo | 12 Mo |
| $K = 10$ | 2,62 Go ; 2,71 à 2,75 Go | 3,21 Go | 1,52 à 1,66 Go | 562 Mo (102 o par boule) | 183 Mo | 59 Mo |

Le pic est atteint pendant la construction du catalogue (`peak_after_catalogue` = `peak_after_tower` dans toutes
les passes). Postes de la tour à $K = 10$ : tableaux par ordre 153 Mo, copie des populations semées `spop` 131 Mo,
atlas 116 Mo, tables de semis 71 Mo, index des supports 67 Mo.

`MemoryBudget` n'est jamais limité hors d'un test unitaire (`default_budget()` illimité), et les `std::vector` de
la tour (forêts, `atlas.info`, `first`, tampons) ne passent pas par lui : le « budget honnête par construction »
de `src/core/buffer.hpp` ne compte qu'une partie de la mémoire et ne refuse rien.

**Preuve.** `mesures/*.json` et `*.time.txt`, `instrumentation/instr_l00_k*.err` (`L06_MEM_*`),
`grep -rn MemoryBudget src cli`.

**Vérification.** Mesuré, lu.

**Conséquence v11.** Un plafond mémoire vrai suppose que **tous** les tableaux passent par le compteur, catalogue
compris. La copie `spop` et la table `ball_vnode` (4 octets par boule pour les seules verticales) sont à
reconsidérer.

#### L06_CODE_TOUR-08 — info — Aux tailles d'intérêt le travail par boule est constant, le temps par boule monte

**Fait mesuré.** Famille `clusters`, $K = 5$, 8 000, 16 000 et 32 000 sites, deux régimes : pas de descente par
boule 3,55 à 3,57 ; MEB par boule 0,92 à 0,93 ; recensement moyen 7,5 à 7,7 sites ; chaîne maximale 7 à 9 pas
(`instrumentation/echelle_clusters_*`). Le travail est linéaire.

Le mur par boule, lui, monte dans le reçu G4 d'échelle (48 fils, `receipts/g4_session5_scale_20260929/scale.csv`) :
`uniform` $K = 5$ : 75, 80, 86 ns par boule à 8 000, 16 000, 32 000 sites, puis 137 à 512 000 ; `uniform`
$K = 10$ : 89, 96, 102, puis 178. Le catalogue reste entre 82 et 104 ns par boule. À un million de sites la tour
coûte deux fois le catalogue.

**Vérification.** Mesuré (local, compteurs) et lu dans le reçu.

**Conséquence v11.** La dérive vient de ce qui n'est pas parallèle (constat 04) et des tables qui sortent des
caches, pas de l'algorithme. Elle disparaît en grande partie avec le noyau recouvert et les verticales parallèles
(conjecturé, à mesurer à 8 000, 16 000 et 32 000).

### 4.3 Dette

#### L06_CODE_TOUR-09 — majeur — Un monolithe non testable par morceaux, et une conception qui ne décrit pas le code

**Fait.** `tower.cpp` fait 1 867 lignes ; `build_tower` en fait 601 (l. 1151-1751) et enchaîne six étages dans une
seule fonction ; tout le reste (MEB, filtre d'orientation, quotient local, index plat, descente, Kruskal, pointeurs
de saut) vit dans un espace de noms anonyme (l. 15-1149). Aucune de ces pièces n'est atteignable par un test : il
n'existe aucun test unitaire de la tour dans `tests/unit/unit_main.cpp` (seuls les entiers larges, le pool, les
tampons, le nuage et l'arbre des sites y sont), et aucun mutant dans le dépôt (`grep -rn -i mutant` : une seule
occurrence, un commentaire). La tour a été écrite en 12 commits sur 27 heures (du 28 septembre 12 h 21 au
29 septembre 15 h 18), plus un raccord le 30 septembre (`git log -- morsehgp3D_v10/src/tower/`).

`TOWER_v2.md` prévoyait 15 fichiers d'au plus 400 lignes et 2 800 lignes de tests (§ 15.1). Le tableau du § 3.9
montre que ce document ne décrit pas le programme livré. Six raisons de refus sont déclarées et jamais émises
(`window_empty`, `arith_guard`, `nested_parallelism`, `leaf_unsplittable`, `device_unavailable`,
`k_out_of_range`, `src/core/reasons.def`), le champ `OrderForest::descents` n'est jamais écrit, `kMaxOrder = 10`
n'est lu nulle part (la tour sert $K = 11$ avec le statut `ok`, exécuté ; le refus n'arrive qu'à $K = 13$).

**États globaux et options.** La tour n'a pas d'état global propre ; elle dépend de trois états cachés :
`thread_local QueryScratch tls` (`src/cloud/site_tree.cpp:111`), `tls_in_region` (`src/sched/pool.cpp:8`) et le
budget statique `default_budget()` (`src/core/buffer.cpp:5-8`). Les compteurs et les temps sont rangés dans l'objet
publié (`OrderForest::stats`, `Tower::stats`). `TowerParams` porte sept options, dont trois se conditionnent en
silence (`only_order` coupe les verticales ; `ball_nodes` et `cover_extra` n'agissent qu'en entrée `cover`) ; le
CLI de la tour n'en expose que trois.

**Vérification.** Lu ; `grep` ; exécuté pour $K = 11$.

**Conséquence v11.** Découper par contrat (MEB, quotient local, atlas, descente, forêt, requêtes d'ancêtre,
attaches, verticales), chaque module avec son test, ses fixtures et ses mutants. Ne garder pour documentation que
ce qui décrit le code ; `SPEC_V10.md` § 4 est le bon modèle, `TOWER_v2.md` est une source d'idées, pas un état.

#### L06_CODE_TOUR-10 — mineur — Deux plantages de CLI reproduits, et une API qui accepte l'impossible

**Fait exécuté** (`limites/edge_results.txt`) :

- `mhgp10_tower IN --k=2 --no-points --dump=F` : erreur de segmentation (code 139), JSON et dump vides. La boucle
  de dump lit `ord.point_node[s]` sur un vecteur vide (`cli/mhgp10_tower.cpp:186-194`). Le mode `--no-points` est
  celui du contrat LiDAR : la tour FULL sans attaches ne peut donc pas être exportée.
- `mhgp10_cluster` avec moins de points que $K$ (3 points, `--k=5`) : erreur de segmentation. `build_tower` accepte
  `only_order` supérieur à l'ordre effectif, ne construit rien et rend `ok` (l. 1177-1184) ; le CLI indexe alors
  `orders[kk - 1]` hors du tableau (`cli/mhgp10_cluster.cpp:173`).
- Le refus d'une descente verticale est toujours publié comme `descent_no_terminal`, quelle que soit la raison
  enregistrée (l. 1744).
- Le JSON publie `"K"` demandé, non l'ordre effectif $\min(K, n)$.

**Conséquence v11.** Valider les paramètres à l'entrée de l'API (refus `invalid_input` typé), ne jamais rendre `ok`
sur une demande non servie, et n'avoir qu'un chemin d'export.

#### L06_CODE_TOUR-11 — mineur — Le dump n'est ni canonique, ni versionné, ni compact

**Fait.** Le seul export de la tour est un texte sans en-tête ni version (`cli/mhgp10_tower.cpp:171-197`) : 80 Mo
pour une trame à $K = 5$, 348 Mo à $K = 10$ (mesuré). Les niveaux y sont des fractions **non réduites**, celles du
représentant du rang : la même tour, sous isométrie, s'écrit autrement (2 niveaux d'entrée écrits différemment sur
la trame 00, `isometrie/iso_cover.log`). Une naissance n'y est qu'un niveau : ni boule, ni population. Les formats
`core` et `cover` se distinguent au nombre de jetons. Il n'y a aucune empreinte de l'objet.

**Vérification.** Lu, exécuté.

**Conséquence v11.** Un export binaire versionné et une empreinte canonique indépendante de la numérotation.
`isometrie/tower_merkle.py` en donne une définition exécutable (niveaux réduits, hachage de Merkle des
sous-arbres) et `anchors.jsonl` ses valeurs v10 sur les trois trames du contrat.

#### L06_CODE_TOUR-12 — mineur — La tour ne détecte pas un catalogue incomplet, et re-vérifie des théorèmes en produit

**Fait lu.** Quand la sphère d'une descente devrait être au catalogue (ordre dans sa fenêtre) et n'y est pas, la
descente continue en silence (l. 951-989) ; seul le cas « naissance absente » est refusé (l. 985-988). Il n'y a ni
contrôle d'Euler ni contrôle I5 ; le seul détecteur global est « une racine par ordre ». À l'inverse, deux
contrôles de théorèmes tournent à chaque exécution : le juge de recensement 1/32 et la naturalité complète des
verticales, celle-ci sur le chemin séquentiel (constat 04).

**Fait mesuré.** Sur la trame 00, 0 sphère de fenêtre absente à $K = 5$ et $K = 10$ (`L06_LOOKUP2`,
`miss_in_window=0`) : rien n'indique un catalogue incomplet, mais rien ne l'aurait signalé.

**Conséquence v11.** Décider ce qui est refus de produit (peu coûteux, détecte une faute) et ce qui est porte
(re-vérification d'un théorème), conformément à la règle « jamais de vérification exhaustive ».

#### L06_CODE_TOUR-13 — mineur — Gardes flottantes : marge figée pour 18 bits, garde de compilation contournable

**Fait.** Les filtres flottants de la tour reposent sur `kApproxMargin = 0.02` (l. 27), prouvée pour des coordonnées
de 18 bits ; la tour refuse au-delà (l. 1156). La garde contre `-ffast-math` est une recherche de chaîne dans
`CMAKE_CXX_FLAGS` (`CMakeLists.txt:20-22`) : `cmake -DCMAKE_CXX_FLAGS=-Ofast` se configure sans erreur (exécuté),
alors que `-Ofast` active `-ffast-math`. Aucun fichier source ne teste `__FAST_MATH__` ni le mode d'arrondi. Le
build `-Werror` est fragile : ajouter de simples compteurs à `resolve` déclenche un faux positif
`-Werror=array-bounds` dans `DWelzl::small` (GCC 13.3, `instrumentation/werror_array_bounds.log`) ; le code
contient déjà deux contournements du même faux positif (l. 32 et l. 867).

**Vérification.** Lu, exécuté.

**Conséquence v11.** La décision de l'utilisateur de dépasser 18 bits impose de recalculer chaque marge à partir du
nombre de bits. Une garde source (`#error` sous `__FAST_MATH__`, contrôle de `FE_TONEAREST` à l'entrée) coûte deux
lignes.

#### L06_CODE_TOUR-14 — info — Les multifusions sont la règle, et près de la moitié des jonctions n'unissent rien

**Fait mesuré (trame 00).** À l'ordre 5 : 235 290 fusions dont 147 275 binaires, 70 244 ternaires et 17 771 à
quatre enfants ou plus (37 % à trois enfants ou plus) ; à l'ordre 10 : 405 049, 188 223 et 65 951. Ce ne sont pas
des ex æquo de niveau : une jonction régulière de coquille à trois ou quatre sites réunit trois ou quatre morceaux
en un seul point critique. 213 448 des 448 755 jonctions de l'ordre 5 (48 %) n'unissent rien (leurs représentants
sont déjà dans la même composante). Seuls 8 914 lots sur 437 973 contiennent plusieurs jonctions.

**Preuve.** `instrumentation/instr_l00_k5_t1.err`, `instr_l00_k10_t2.err` (lignes `L06_KRUSKAL`),
`isometrie/anchors.jsonl` (champ `fusions_par_arite`).

**Conséquence v11.** Une tête qui suppose des fusions binaires déforme l'objet dès $K = 2$. Le traitement par lots
ne sert qu'à 2 % des lots : il ne justifie pas d'être le chemin commun.

## 5. Ce qui est séquentiel, ce qui est défavorable (synthèse des mesures)

**Mesures locales, trame 00, tour sans attaches** (dernière passe chaude ; machine chargée : seuls les rapports
entre étages d'une même passe sont à lire, les temps de contrat sont ceux du constat 04) :

| Étage (s) | $K = 5$, 1 fil | $K = 5$, 4 fils | $K = 10$, 1 fil | $K = 10$, 4 fils |
| --- | ---: | ---: | ---: | ---: |
| Index des supports `t_prepare` | 0,060 | 0,015 | 0,313 | 0,122 |
| Atlas, structures locales `t_local` | 0,052 | 0,025 | 0,275 | 0,133 |
| Semis `t_seeds` | 0,070 | 0,026 | 0,915 | 0,282 |
| Descentes `t_resolve` | 1,604 | 0,546 | 18,560 | 5,047 |
| Kruskal `t_kruskal` (somme des ordres) | 0,147 (0,147) | 0,056 (0,176) | 0,816 (0,816) | 0,246 (0,936) |
| Verticales `t_vertical` (somme des fusions) | 0,155 (0,061) | 0,060 (0,069) | 1,077 (0,366) | 0,328 (0,402) |
| Tour, passes | 1,87 à 2,09 | 0,73 à 1,05 | 16,6 à 22,0 | 6,2 à 8,5 |
| Catalogue, passes | 6,6 à 7,1 | 2,8 à 3,7 | 38,3 à 41,8 | 13,3 à 14,8 |

Compteurs déterministes de la même trame : 1 541 750 nœuds à $K = 5$ et 7 426 215 à $K = 10$ ; union-find du
Kruskal : 3,86 millions de pas de `find` pour 1,20 million de lots à $K = 5$, 18,8 millions pour 5,37 millions de
lots à $K = 10$ ; pas de remontée d'ancêtre des verticales : 10,0 et 66,9 millions ; requêtes de boule et de
recherche : tableau du constat 05.

**Séquentiel dans la tour** (lu, puis chronométré dans le reçu G4 quand un compteur existe) :

| Passage | Lieu | Poids |
| --- | --- | --- |
| Kruskal par lots, un ordre par tâche | l. 1041-1116, 1503-1515 | 20,8 à 31,6 ms à $K = 5$, 52,3 à 90,3 ms à $K = 10$ (ordre maximal) |
| Pointeurs de saut, dans la même tâche | l. 721-739, 1511 | compris ci-dessus |
| Images et naturalité des fusions, un ordre par tâche | l. 1709-1735 | 12,5 à 18,7 ms à $K = 5$, 36,4 à 49,7 ms à $K = 10$ |
| Entrée `cover` : boucle sur les ordres ; pour chacun, une passe sur **toutes** les boules du catalogue (l. 1563-1572), puis tri et dichotomie par site | l. 1542-1598 | `t_points` 21,7 ms à $K = 5$ (reçu, trame 00) ; en local à 4 fils et $K = 10$ : 0,75 s pour 50 MEB, contre 0,38 s en `core` |
| Premier contact de `atlas.info` (12 octets par boule), affectations `assign` par ordre | l. 1206, 1527-1528, 1667 | non mesuré isolément |
| Hors tour, dans le CLI : tri de Morton et arbre des sites | `cli/mhgp10_tower.cpp:71-75` | `prepare_s` 6,6 à 8,3 ms pour 36 000 à 46 000 sites (reçu) |

**Défavorable** :

- un pas de descente en $\Theta(n)$ (boule fermée entière ; mesuré : la moitié du nuage pour une requête) ;
- le quotient local des coquilles étendues, exponentiel en la taille de la coquille (mesuré : 49 à 57 s pour
  24 points) ;
- la longueur des chaînes de descente n'a pas de borne prouvée ; mesuré : 8 pas à $K = 5$ et 12 à $K = 10$ sur la
  trame 00, 7 à 9 pas sur les familles synthétiques à 8 000, 16 000 et 32 000 sites.

**Trame entière avec sol** (08/000000, 123 389 sites à 1 mm, aucun doublon, $K = 5$, 3 fils, local) : 2 822 052
boules (2,16 fois la trame sans sol pour 3,1 fois plus de points), 7 386 343 représentants, pic RSS 1,22 Go. Les
quatre trames brutes du cache n'ont aucun doublon à 1 mm et tiennent dans 18 bits (étendue 155 à 159 m).

## 6. Ce qui est solide et mérite un port explicite en v11

1. **La sémantique.** Cellule (boule, $k$), fenêtre $[p + q - 1, p + m]$, naissance et jonction par le quotient de
   Gordan, lots par niveau exact, multifusions N-aires jamais binarisées, coupe fermée pour les attaches. Jugée
   conforme jusqu'à $K = 10$ sur petits nuages, par EMST à l'ordre 1 sur une trame, invariante par isométrie.
2. **La descente pure.** Fonction de $(F, k)$ seule, garde de décroissance stricte du niveau en exact (I3), mémo par
   cellule qui n'est qu'un cache. C'est ce qui rend la sortie indépendante du nombre de fils.
3. **Le semis $H_k$.** 86 % des représentants résolus par une consultation vérifiée sur la clé.
4. **La MEB proposée en double puis certifiée en exact**, avec repli Welzl exact : 0 repli sur 7,66 millions.
5. **Le recensement lu au catalogue** quand le support certifié est le support canonique d'une boule.
6. **L'index plat sans verrou à clé vérifiée** (`FlatIndex`) : résultat indépendant de l'ordre d'insertion.
7. **La numérotation canonique** des nœuds : naissances en ordre (rang, $S^{*}$), fusions en ordre (rang, plus
   petite naissance).
8. **Les requêtes d'ancêtre en $O(\log)$** (pointeurs de saut), et la recherche de rang exacte à deux étages avec
   sa porte (`tests/unit/rank_search.cpp` : code exact, plancher, tailles jusqu'à $2^{32} - 1$).
9. **L'indépendance des ordres à catalogue donné**, vérifiée : mêmes empreintes des ordres 1 à 5 avec un catalogue
   bâti à $K = 5$ ou à $K = 10$.
10. **Les compteurs de travail par étage et par ordre** dans la sortie JSON : ils ont rendu cet audit possible.
11. **Les ancres** : `preuves_l06_code_tour/isometrie/anchors.jsonl` (empreintes canoniques des trames 00, 01 et 02
    à $K = 5$ et de la trame 00 à $K = 10$, avec le `sha256` des entrées et des dumps).

## 7. Ce qu'il ne faut pas refaire

1. Écrire une conception détaillée, puis livrer autre chose sans la mettre à jour.
2. Un fichier unique et une fonction de 600 lignes ; des pièces sans test ni mutant.
3. Un étage séquentiel par ordre sur le chemin critique, alors que la mesure de la conception le condamnait.
4. Des contrôles de théorèmes dans le chemin produit (naturalité complète, juge de recensement).
5. Une énumération brute des coquilles sans budget a priori.
6. Un recensement entier là où $k$ voisins suffisent.
7. Une attache définie par « le plus petit indice » sans règle géométrique aux ex æquo.
8. Une porte oracle qui relève elle-même ce qu'elle juge (`top()` remonte le nœud publié), sans planchers par
   régime ni mutants.
9. Un export texte sans version ni empreinte, des niveaux non réduits, un CLI qui plante sur une combinaison
   d'options documentée.
10. Rendre `ok` sur une demande non servie (`only_order` hors de portée).
11. Un budget mémoire déclaré « honnête » qui ne compte qu'une partie des tableaux.
12. Des marges flottantes en constantes littérales valables pour une seule largeur de coordonnées.

## 8. Questions ouvertes

1. **Verticales.** Elles sont calculées, jugées, et consommées par personne (la tête ne les lit pas ;
   `only_order` les désactive). Font-elles partie du contrat de la tour FULL en v11, et de son chronomètre ?
2. **Entrée des points.** Laquelle fait foi pour le contrat 3 : `core`, `cover`, `cover` avec `cover_extra` ? Et
   quelle règle aux ex æquo de `cover` : toutes les composantes candidates, leur premier ancêtre commun, un
   départage géométrique ?
3. **Juge à l'échelle pour $K \geq 2$.** L'oracle $\Gamma_k$ ne dépasse pas 12 à 14 points et n'exerce pas les
   descentes longues. Quel juge indépendant aux tailles 8 000, 16 000, 32 000 : descente par échange d'intrus sur
   échantillon (J-DESC), certificats de multifusion (J-MF), autre ?
4. **Coquilles étendues.** Quel périmètre : quotient polynomial pour toutes, ou refus typé a priori ? Les jeux 2D
   ($z = 0$) et les données en grille font-ils partie du contrat 3 ?
5. **Complétude.** Euler et le refus d'une sphère de fenêtre absente : produit ou porte ?
6. **Ordre du catalogue.** À l'ordre maximal, 47 % des MEB de la trame 00 à $K = 10$ (914 191 sur 1 965 797) ne
   trouvent pas leur sphère au catalogue et passent par l'arbre ; aux ordres inférieurs la part tombe sous 25 %.
   Un catalogue plus profond d'un ou deux ordres coûte-t-il moins que ces requêtes ? Non mesuré.
7. **Borne des chaînes de descente.** Aucune borne prouvée du nombre de pas ; 12 pas mesurés.
8. **Chronomètre du contrat.** Les 100 ms comprennent-elles la préparation (6,6 à 8,3 ms séquentielles sur G4) et
   l'export ?
9. **Au-delà de 18 bits.** Quelles marges, quelles largeurs d'entiers pour la MEB et les niveaux ?
10. **Multiplicités.** Les quatre trames brutes regardées n'ont aucun doublon à 1 mm. Refus assumé, dédoublonnage
    ou sémantique pondérée ?

## 9. Recommandations pour la v11

### 9.1 Garder

La liste du § 6, avec ses fixtures et ses portes. En particulier : descente pure et mémo-cache, semis, MEB
certifiée, numérotation canonique, sorties identiques quel que soit le nombre de fils.

### 9.2 Changer

| Sujet | Changement | Gain estimé (trame 00, G4, 48 fils) | Fondement |
| --- | --- | --- | --- |
| Forêt d'un ordre | noyau union-find sans lots ; multifusions à la volée (alias des nœuds d'un même rang) ou matérialisation parallèle | étage T de 31,7 à 12 à 20 ms ($K = 5$), de 90,4 à 34 à 57 ms ($K = 10$) | banc `krbench`, rapports ÷ 1,6 à ÷ 3,2 mesurés, forêt identique |
| Ordonnancement | ordres par $K$ décroissant ; le noyau de l'ordre maximal tourne pendant l'étage G des ordres inférieurs | étage T visible proche de 0 sur CPU | l'ordre maximal porte 37 % des représentants à $K = 5$ et 22 % à $K = 10$ : il reste 18 ms et 189 ms d'étage G pour recouvrir un noyau de 12 à 20 ms et de 34 à 57 ms (conjecturé, exige un ordonnanceur à tâches) |
| Verticales | une requête d'ancêtre par nœud, en parallèle, depuis l'image d'une naissance du sous-arbre ; naturalité dans les portes | étage V de 17,6 à environ 4 ms ($K = 5$), de 64,8 à environ 20 ms ($K = 10$) | part parallèle déjà mesurée (2,5 et 15,2 ms) ; le reste est 35,8 et 211 ms de CPU à répartir (conjecturé) |
| Saut hors catalogue | une requête des $k$ plus proches du centre, bornée | coût par pas borné ; quelques pour cent de G | maxima mesurés de 1 258 sites (LiDAR) et de la moitié du nuage (contraste) |
| Seconde recherche, juge 1/32 | supprimés du chemin produit | 3 à 5 % de G (conjecturé) | 62 056 et 397 450 recherches perdues ; 9 à 10 % des requêtes d'arbre |
| Coquilles étendues | quotient polynomial, ou refus a priori sur $\binom{m}{t}$ | de 49 s à un coût négligeable sur 24 points cosphériques | `limites/edge_results.txt` |
| Entrée `cover` | règle écrite aux ex æquo, fixture, porte d'isométrie | exactitude | constat 02 |
| Structure | modules par contrat, tests unitaires, mutants, paramètres validés à l'entrée | — | constats 03, 09, 10 |
| Export | binaire versionné, empreinte canonique | — | constat 11 |

**Bilan estimé de la tour sans attaches, trame 00, G4, 48 fils, CPU seul.**

| | v10 mesuré | v11, noyau visible | v11, noyau recouvert |
| --- | ---: | ---: | ---: |
| $K = 5$ | 88,5 ms | 54 à 62 ms | 42 à 44 ms |
| $K = 10$ | 472 ms | 348 à 371 ms | environ 315 ms |

Détail à $K = 5$ : préparation, atlas et semis 10,4 ms (inchangés) ; G 28,6 ms ramenés à environ 27,5 ; T 12 à 20 ms
ou 0 à 2 ms ; V environ 4 ms. À $K = 10$ : 62,1 ; 243 ramenés à environ 233 ; 34 à 57 ou 0 ; environ 20.

Ces chiffres sont des estimations : les étages viennent du reçu G4, les rapports du banc local, le recouvrement
n'a pas été mesuré.

**Ce que ces estimations disent du contrat.** À $K = 5$, une tour CPU de 42 à 62 ms laisse 38 à 58 ms au catalogue,
qui en prend 137 à 164 dans le même reçu : le contrat de 100 ms ne se joue pas dans la tour seule. À $K = 10$, la
tour reste au-dessus de 300 ms tant que l'étage G (233 ms) et les étages atlas et semis (51 ms) sont sur CPU : le
contrat de 100 ms à $K = 10$ demande de porter l'étage G sur GPU ou de réduire le nombre de descentes, pas
seulement de corriger le plancher séquentiel.

### 9.3 Supprimer

- le Kruskal par lots comme chemin commun (le garder dans `tests/` comme référence d'équivalence) ;
- la naturalité complète et le juge de recensement du chemin produit ;
- la seconde recherche de support identique ;
- la copie `spop` si une vérification directe sur le CSR du catalogue tient le débit (à mesurer) ;
- les raisons de refus jamais émises, les champs jamais écrits, `kMaxOrder` ou son absence de garde ;
- le dump texte comme interface ;
- les options qui se désactivent en silence (`only_order` coupe les verticales).

### 9.4 Portes à prévoir dès l'ouverture

- Oracle $\Gamma_k$ jusqu'à $K = 10$, avec planchers par régime (sauts, coquilles étendues, multifusions à trois
  enfants ou plus, attaches à l'égalité) et des nuages **choisis** pour sauter (`oracles/oracle_sauts.py` : à
  14 points, la famille « noyau serré et halo » saute aux ordres 9 et 10).
- Un juge qui lit les champs publiés **sans les relever** : nœud d'attache vivant à la coupe fermée du niveau
  d'entrée, image verticale vivante à la coupe fermée du niveau du nœud, arité des fusions.
- Mutants causaux tués, à commencer par les trois fautes qui survivent ici (constat 03).
- Juge EMST de l'ordre 1 aux tailles d'intérêt et sur trame (`oracles/emst_judge.py`).
- Porte d'isométrie et de déterminisme sur l'empreinte canonique (`isometrie/tower_merkle.py`).
- Campagne appariée v10/v11 sur `anchors.jsonl`.
- Compteurs par boule à 8 000, 16 000 et 32 000 sites (pas, MEB, recensement moyen et maximal, chaîne maximale).

## 10. Preuves

Dossier `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l06_code_tour/` ; calculs sous
`/tmp/v11-audit/l06_code_tour/` (perdus au redémarrage, rejouables par les scripts conservés).

| Sous-dossier | Contenu | Sert à |
| --- | --- | --- |
| `mesures/` | sorties JSON du binaire Release du HEAD, trame 00 à $K = 5$ et $K = 10$, 1 et 4 fils, avec et sans attaches ; trame avec sol ; pics RSS | constats 05, 07 |
| `g4/` | extraction par étage des sorties brutes du reçu G4 de session 4 | constat 04 |
| `instrumentation/` | `patch_instr.py`, `patch_tsc.py` (instrumentation d'une copie), compteurs `L06_*` à $K = 5$ et $K = 10$, ex æquo de `cover`, échelle 8 000 à 32 000, contraste de densité, faux positif `-Werror` | constats 02, 05, 06, 07, 08, 13, 14 |
| `krbench/` | `krbench.cpp` et ses résultats | constat 04 |
| `oracles/` | portes CTest ; `oracle_k10.py` ; `oracle_cover.py` ; `emst_judge.py` ; leurs journaux | constats 01, 02, 03 |
| `mutants/` | `make_mutants.py`, `run_mutants.sh`, résultats | constat 03 |
| `determinisme/` | `sha256` des dumps à 1, 3 et 4 fils | constat 01 |
| `isometrie/` | `iso_test.py`, `tower_merkle.py`, journaux, `anchors.jsonl` | constats 01, 02, 11 |
| `limites/` | générateur des cas limites, résultats, plantages | constats 06, 10 |
| `meb/` | harnais des MEB de `tower.cpp`, juge en `Fraction`, journal | constat 01 |
| `sanitizers/` | script et résultats ASan, UBSan, TSan | constat 01 |
