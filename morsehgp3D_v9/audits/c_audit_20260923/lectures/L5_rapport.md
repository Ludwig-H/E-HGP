## 0. Cadre et méthode

`phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. GCP non utilisé. Lecture seule du worktree `build/v9-audit-c-worktree` (0125dc18) ; le commit non poussé 4530644b du développeur ne touche pas `src/tower/`. Deux petits contrôles locaux (`nice -n 19`, un fil, < 10 s CPU) dans le scratchpad `agents/lentille5/` : `sizes.cpp` (tailles ABI x86-64 GCC : `BallData` 224 o, `level` à l'offset 80, `interior_ids` 132, `shell_ids` 168 ; `ExactLevel` 48 o, `FullNode` 64 o, `FullDatedContribution` 80 o, `FullCoverageBatch` 80 o, `FullCoveragePopulation` 48 o, requête statique 56 o) et `check_level_filter.py` (filtre flottant des niveaux, § 9). Abréviations : **FBT** = `morsehgp3D_v9/src/tower/forest/full_ball_tower.hpp`, **MEB** = `…/forest/anchor_meb.hpp`, **LP** = `…/forest/local_plateau.hpp`, **FCC** = `…/forest/full_coverage_certificate.hpp`.

Statuts utilisés : *prouvé* (référence citée), *testé* (porte/fixture), *mesuré* (reçu), *estimé* (calcul de lecture), *supposé*.

## 1. L'objet

Manuscrit, Déf. 20–22 et Th. 2 (PDF 57–61, `theorem_external`) : pour chaque K et chaque rayon r, les K-polyèdres sont les composantes du graphe Γ_K(X,r) dont les sommets sont les K-sous-ensembles de rayon de naissance ≤ r, adjacents quand leur union (K+1 points, Prop. 5) est encore un simplexe de Čech ; ils coïncident avec les amas discrets de forte densité K-NN. La **tour FULL** encode, pour K=1..Kmax, l'histoire datée de ces composantes : naissances (un K-sous-ensemble isolé à sa naissance, c'est-à-dire exactement la population d'une boule de rayon minimal : un « K-set »), multifusions à parents multiples, continuations qui ajoutent des points sans fusion, et une image verticale de chaque nœud dans l'ordre K−1.

La Prop. 6 et le Th. 5 du manuscrit (graphe de Gabriel / K-MST) sont **faux en général** : registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:40-41` et `:129` (fixture E5). Le moteur v9 ne s'appuie pas sur eux : il suit les adjacences de Γ_K, y compris par des simplexes non Gabriel, grâce à la descente géométrique (§ 4). C'est le point conceptuel central de cette lentille.

## 2. Entrée et validation du catalogue

Une `BallData` (`morsehgp3D_v9/src/tower/forest/ball_data.hpp:24-33`) porte une `BallKey` primitive, un niveau exact (rayon au carré, U192/i128 non réduit pour q4), `arity = q_min`, les intérieurs stricts I (≤ 9) et la coquille complète U (≤ 12). L'autorité est **relative à un census complet et exact fourni** (FBT:19-20).

`validate_catalogue` (FBT:877-1002) procède ainsi :
- domaine : profil u18, identités distinctes, positions distinctes (FBT:879-891) ;
- unicité des clés : balayage si le catalogue est déjà strictement trié (cas de la chaîne), sinon tri (FBT:898-905) ; index exact par adressage ouvert (FBT:769-798) ;
- passe 1, parallèle, par boule (FBT:822-849) : bornes u18 de la clé, puissance < 0 sur I et = 0 sur U, et pour une boule régulière (|U| = arité) le recalcul exact clé+niveau depuis le support déclaré (FBT:851-875) ;
- passe 2, série : pour une coquille étendue, `ShellTable::prepare` (LP:47-91), égalité `q_min = arity` et témoin MEB exact du premier support minimal (FBT:945-961) ; fenêtre `p+q_min ≤ min(Kmax+1,n)` (FBT:962-963) ;
- tri par niveau exact : filtre double certifié puis U320, départage par rang de clé, donc ordre total strict (FBT:973-993) ;
- programmes : chaque boule entre dans `programs[K]` pour `K ∈ [p+q_min−1, min(Kmax,p+u)]` (FBT:994-1000).

La complétude de I∪U vis-à-vis du nuage n'est pas vérifiée : c'est une précondition. La chaîne la garantit par son recensus (`morsehgp3D_v9/src/chain/tower_chain.cpp:462-498`).

## 3. Calendrier des ancres et représentants stricts

*Prouvé* (v7, `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md` §§1-3, modèle borné ; registre `:131-134` `conditional_theorem` sous régularité). Sous K = p+q_min−1, la boule est inerte : ses K-facettes strictes forment une seule composante qui couvre I∪U (`LocalRank::inert_sufficient`, LP:108). Au-dessus de p+u, elle ne contient aucune K-facette. Entre les deux, **une ancre par (K, boule)** représente toutes ses K-facettes, car le graphe de Johnson des K-sous-ensembles de I∪U est connexe au niveau fermé de la boule.

`visit_block_at` (FBT:1463-1504) émet les représentants et la contribution :
- **boule régulière** (99,99 % sur LiDAR : 444 coquilles étendues sur 5 512 670 boules, R7b probe_4) : à K = p+u, naissance, sans représentant, contribution = I∪U ; à K = p+u−1, les u facettes I∪U∖{s}, toutes strictes puisque le support est positif, et aucune contribution ;
- **coquille étendue** : `ShellTable::rank(K)` (LP:100-178) calcule, dans le quotient local sur les masques de U, les composantes strictes (étoile des (t+1)-masques stricts, t = K−p, LP:144-157), un représentant par composante (I + masque) et la contribution U ∖ ∪(couvertures strictes locales) (LP:173-176) ; sans composante stricte, c'est une naissance couvrant tout I∪U.

## 4. Résolution d'un représentant : MEB, intrus, échange

Pour une facette F de K sites (stricte, donc ρ(F) < niveau L du bloc) :

1. **MEB exact** (MEB:196-365). La référence essaie la première paire maximale (seule candidate q2, preuve `meb_diameter_20260911`), puis triples et quadruples dans l'ordre lexicographique ; le premier support positif qui contient tous les sites est la MEB par unicité. La variante proposée (défaut) lance un Welzl *move-to-front* en `double` qui **ne décide rien** : le support proposé est vérifié par formes et puissances entières (MEB:314-320). Si la coquille exacte est plus grande que le support, la même énumération est reprise restreinte au bord exact (MEB:328-346), ce qui redonne le support canonique de la référence ; sinon on se replie sur l'énumération complète. *Testé* : 34 957 ensembles et un mutant tué (MEB_PROPOSITION_EXACTE).
2. **Clé → catalogue** (FBT:1057-1066) : si la boule est cataloguée et admissible à K (`p+q_min−1 ≤ K ≤ p+u`), c'est le terminal.
3. Sinon, **intrus** : premier site strictement intérieur hors de F, dans l'ordre Morton (FBT:1019-1044). Il existe forcément : sans intrus, F est « faible », donc I ⊆ F et la fenêtre contient K ; son absence du catalogue est détectée (`full_ball_static_missing_weak_terminal`, FBT:1069).
4. **Échange** : le premier site du support est remplacé par l'intrus, puis la facette est triée à nouveau (FBT:1072-1073). F et F′ sont adjacentes dans Γ_K au niveau ρ(F) par la coface F∪{z}. Le couple (rayon, taille de coquille sélectionnée) décroît strictement dans l'ordre lexicographique (FBT:1094-1101) ; le rayon seul peut rester égal, et la fixture `actual_equal_radius_descent` l'exerce.
5. **Semis** : une facette égale à la population complète I∪U d'une boule d'ordre K = p+u est résolue par cette boule, sans MEB, au départ (FBT:1354-1360) comme après échange (FBT:1075-1092, lemme `SEMIS_APRES_ECHANGE`).

Le terminal T contient la facette finale et vérifie L_T < L. Son ancre, fermée avant le lot consommateur, est donc la composante pré-lot de F. N'importe quelle boule admissible contenant F et antérieure donnerait la même composante.

**Voie temporelle** (`static_threads=0`, FBT:1393-1440) : la résolution a lieu pendant les lots ; le succès se lit par `anchors[found] != absent`, avec un cache facette→jeton de 16n cases, remis à zéro à chaque K et semé des K-sets fermés (FBT:253-314, FBT:1442-1455). **Voie statique** (défaut de la chaîne) : en phase 0, pour chaque K, les requêtes (facette triée, consommateur, ordinal) sont collectées dans l'ordre du programme, triées par (clé, ordinal) et regroupées par clé ; chaque groupe est résolu une fois en `BallId`, avec le niveau du premier consommateur comme borne `before` (FBT:1237-1380). Pendant les lots, la cible est lue dans `static_targets[ordinal]`, puis traduite en ancre et en racine (FBT:545-566 et FBT:1381-1388). Les deux voies aboutissent au même terminal ; la porte T2 le vérifie en 0/1/4 fils.

## 5. Lots, union-find, parents, naissances, continuations

Pour chaque K, `programs[K]` est parcouru par **lots maximaux de niveau exactement égal** (`same_exact_level`, FBT:655-661 et FBT:355-358). Tous les représentants du lot sont résolus sur l'état **pré-lot** (`prior_count`, FBT:1009-1018 et FBT:524-533 : la racine doit être antérieure et vivante), avant toute écriture. Les blocs sont ensuite groupés par racines pré-lot communes (DSU locale sur les paires (racine, bloc), FBT:600-612). Deux boules distinctes de même niveau ne partagent aucune facette nouvellement née, par unicité de la MEB : ce groupement suffit. Chaque groupe produit :
- 0 parent : **naissance** ; la garde exige un seul bloc et une seule contribution (FBT:631-632) ;
- 1 parent : **continuation**, émise seulement si elle apporte une contribution, sinon bloc inerte ; aucun nœud n'est créé ;
- ≥ 2 parents : **multifusion** vers un nouveau nœud, `next[parent]` et `compressed[parent]` pointant vers lui (FBT:535-543).

Les ancres de toutes les boules du lot ne sont publiées **qu'après** la fermeture du lot (FBT:640-643 et FBT:1627-1633). Les IDs de nœuds suivent l'ordre des groupes, lui-même celui du plus petit indice de bloc, et restent déterministes. Le long de `next`, les niveaux croissent strictement : les parents sont des racines des lots antérieurs (FBT:531 et FBT:175). La **banque I/U** est immuable et partagée par tous les ordres. Une ligne de population n'est créée que pour une contribution (FBT:1518-1526, ou phase B), et la couverture d'une racine à une coupe est l'union de ses contributions datées (FCC:333-365).

## 6. Images verticales K → K−1

Une multifusion à K a pour image la racine, dans l'histoire de K−1 et à son niveau fermé, de l'image de son premier parent ; toutes les images de parents doivent coïncider, c'est le contrôle de naturalité. Une naissance a pour image l'ancre **de la même boule** à K−1, normalisée au même niveau fermé (FBT:717-727 et FBT:1527-1534, FBT:1557-1565). Cette ancre existe toujours : une naissance vérifie K ≥ p+q_min, donc K−1 est dans la fenêtre (BALL_ANCHORS § 5). La normalisation passe par `MonotoneHistory` (FBT:160-233), qui active les arêtes inférieures dans l'ordre chronologique avec union par rang et identité séparée du propriétaire. Le lecteur public normalise ensuite à la coupe demandée (FBT:1662-1670).

## 7. Ordres K en parallèle et statuts

`run_orders_parallel` (FBT:458-507) enchaîne cinq phases :
- **phase 0** : cibles statiques de K=2..Kmax, un ordre après l'autre, en parallèle à l'intérieur de chaque ordre ;
- **phase A** : les lots de chaque K, un ordre par tâche, donc au plus Kmax tâches ; les contributions y désignent des `BallId` étiquetés (FBT:440) ;
- **phase B** : numérotation des populations dans l'ordre séquentiel exact, puis construction des lignes en parallèle (FBT:679-705) ;
- **phase C** : images de K depuis l'histoire close de K−1 (FBT:707-734) ;
- **finish** : banque déplacée, puis forêts encodées en parallèle et publiées dans l'ordre K (FBT:388-427).

Les états sont privés par K et les entrées lues sont immuables. Je n'ai trouvé aucune course (A et B concluent de même). En cas d'échec, `Failure` est conservé par K et le plus petit K l'emporte entre A et C (FBT:469-502). Sur échec, les statistiques sont fusionnées une fois. Aucune tour partielle n'est publiée (FBT:1658). Statuts possibles : `kCompleteRelative`, `kInvalidInput`, `kResourceExhausted` ou `kInvariantViolated` (FBT:33).

## 8. Complexité et coûts mesurés

Notations : B boules, R_K représentants, U_K clés uniques, S_K semis, D échanges, N nœuds, C contributions, P références de points.

| Phase | Coût |
| --- | --- |
| Validation | O(B·(p+u)) puissances + O(B log B) tri par niveau + O(B) index |
| Phase 0 par K | O(R_K log R_K) tri + chaînes : MEB O(K²)+Welzl+vérification O(K) ; intrus O(nœuds visités), sans borne sous-linéaire (B) |
| Phase A par K | O(R_K·α) + tris locaux, **séquentiel** |
| Phase C par K | O((N_{K−1}+N_K)·α), **séquentiel** |
| Encodage | O(N_K+E_K+C_K) par K ; banque O(P log n) |

*Mesuré*, R7b `vm/probe_4` (08/000000, 39 885 sites, K10, W48, MEB proposé ON) :

| Grandeur | Valeur |
| --- | --- |
| Tour / chaîne / condensé | 4 062 ms / 13 930 ms / 999 ms |
| RSS du processus | 4,81 Gio |
| Boules B | 5 512 670 |
| Représentants | 17 389 031 (436 par site) |
| MEB (= recherches de clé) | 11 309 383 = 4 212 696 hits + 7 096 687 intrus |
| Nœuds d'index visités | 403 429 577 (56,8 par intrus) |
| Distances de paires pour la MEB | 341 878 199 (30,2 par MEB) |
| Nœuds N (K1..K10) | 7 426 215 (186 par site) |
| Contributions C | 4 414 230 |

L'identité MEB = hits + intrus est vérifiée sur **24/24** cas R7b. Parents par multifusion : 2,00 à K1, 2,31 à 2,49 de K2 à K10 : les fusions à 3 ou 4 parents sont courantes.

Sur 60/60 exécutions G4 (R5, R6, R7b, trois trames, K5 et K10), **contributions = naissances à chaque ordre** : aucune continuation contributive ni contribution de fusion sur LiDAR.

Ventilation locale W8 du développeur (coordination 06 h 30, avant MEB proposé) : cibles statiques 15,3 s (dont K10 5,8 s et K9 3,5 s), validation 1,8 s, lots 2,8 s, populations 0,7 s, images 0,6 s, finition 0,8 s. Sidecar A (local) : phase A de K10 2 179,7 ms CPU, 2 117 675 blocs, 3 831 431 facettes. Le plancher séquentiel de la tour est donc l'ordre K10, en phase A, puis en phase C et à l'encodage. Aucun reçu G4 ne ventile la tour par phase.

## 9. Évaluation de la preuve

Si le catalogue est complet (toutes les boules telles que p+q_min ≤ Kmax+1) et exact, la tour publiée est Γ_K pour tout K et toute coupe par la chaîne d'arguments suivante :
- tout changement de Γ_K au niveau L est porté par des boules de niveau L situées dans leur fenêtre ; les boules hors fenêtre sont inertes (théorème de fenêtre et quotient local) ;
- aucune facette nouvellement née n'est partagée entre deux boules ;
- un représentant par composante stricte locale suffit ;
- la descente termine sur un terminal faible, admissible et fermé ;
- le groupement par racines pré-lot donne les composantes du graphe biparti du lot ;
- les naissances sont les K-sets ;
- les verticales découlent de L_K ⊆ L_{K−1}.

Sources : BALL_ANCHORS §§1-5, reçus `receipts_plateaux_full_20260906`, registre `:131-134` (`conditional_theorem`) et `:142`. Le moteur y ajoute l'exactitude entière, avec les bornes u18 de la porte arithmétique, la MEB vérifiée et un ordre des niveaux exact.

*Testé* : le juge T2 Gram/Γ sur des fixtures de 8 points au plus (`tests/tower/full_ball_tower_gate.cpp`), et le raccord à la chaîne pour n ≤ 14.

*Contrôlé par script* : le filtre flottant des niveaux (`lanes/level.hpp:79-87`). Sur 200 000 cas exacts (Fraction), l'erreur relative maximale vaut 2^-51,39, sous la borne annoncée de 2^-49 ; 33 744 décisions en quasi-égalité sont toutes correctes.

**Non garanti** :
- la complétude du catalogue (obligation du générateur) ;
- la complétude de I/U pour un appelant public ;
- les coquilles de plus de 12 sites (refus explicite).

Le README (l. 23) dit « définie et prouvée en v7 » : c'est plus fort que ce statut conditionnel.

## 10. Défauts recherchés

J'ai vérifié les points suivants sans trouver de défaut :
- égalités de niveau : lots par égalité exacte, lots strictement croissants, `before` égal au plus petit consommateur ;
- ordre des lots et association facette ↔ cible par ordinal (même `visit_block_at`, même ordre) ;
- lecture d'ancre : jamais avant la fermeture (contrôles FBT:562-565 et FBT:1384-1387) ;
- u18 : AxisBounds < 2^118, prédicats de plateau en S192, niveaux q4 < 2^280 en U320 ;
- concurrence entre K : états disjoints ; la phase C lit K−1 figé ;
- mémoire : les limites u32 sont des refus explicites.

**Aucun défaut de correction critique ni haut.** Les constats ci-dessous relèvent de la performance, de la mesure, de la complétude, du protocole et de la documentation.

## 11. Autres implémentations pour le contrat (LiDAR, K5/K10, G4)

À K10 sur 40 k sites, toute implémentation exacte doit résoudre de l'ordre de 17 M incidences facette→boule et écrire 7,4 M nœuds. Atteindre 100 ms exige ≥ 170 M facettes/s et ≥ 74 M nœuds/s, donc un calcul massivement parallèle **et** la suppression des sections séquentielles par K. Leviers, par ordre de rendement attendu (tous *estimés*, à mesurer) :

1. **Phase A « maigre » avant toute parallélisation** : tableaux compacts par ordre, préparés en phase 0 dans l'ordre du programme ; rangs de niveau u32 ; aucune lecture du catalogue de 1,23 Go dans la boucle séquentielle ; aucune allocation par lot. Voir F5-01, F5-02 et F5-03.
2. **Ordonnancement en pipeline** : la phase 0 traite K = Kmax en premier et lance aussitôt sa phase A, qui recouvre ainsi la géométrie des autres ordres ; même chose pour la phase C. Voir F5-04.
3. **Rangs de niveau u32 et encodage CSR direct**, populations référencées par `BallId`. Sur LiDAR, contributions = naissances.
4. **Phase C parallèle** : comme les niveaux croissent strictement le long de `next`, `root_at` est une requête d'ancêtre pondéré, parallélisable par saut binaire ou hors ligne. Voir F5-13.
5. **GPU pour la phase 0** : travaux indépendants MEB+clé+intrus, avec une proposition Welzl en FP32 que la conception autorise ; la couture `FullBallBatchResolver` est à rendre compatible avec les ordres K parallèles. Voir F5-12. Le dédoublonnage par hachage avec minimum atomique du rang du consommateur est une alternative exacte au tri des requêtes de 56 o.
6. **Phase A parallèle** par le lemme du maximum d'ID et une forêt minimale (A, PHASE_A_MAX_ID), après le point 1.

La tour est un verrou secondaire derrière q3/q4 : à K10, le meilleur cas vaut 5,2 s pour q3/q4 contre 3,2 s pour la tour, et le digest synchrone ajoute 0,8 à 1,0 s.