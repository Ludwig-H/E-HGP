
## 5. « Tous ordres ensemble par étages, verticales à pointeurs de saut » : énoncés, preuves, hypothèses

Cette construction (commit `025fb7782`, 29 septembre) est une réorganisation de la tour d'ouverture (`6b4b4ccee`, `9af04985c`, 28 septembre) ; elle ne change pas l'objet. Les énoncés qui la fondent, et ce qui en est écrit :

| Énoncé | Où il est écrit | Preuve écrite | Jugé |
| --- | --- | --- | --- |
| Les ordres sont indépendants à catalogue donné ; les étages (atlas, semis, descentes, Kruskal, attaches, verticales) peuvent être menés pour tous les ordres à la fois | `SPEC_V10.md:62-63`, `tower.hpp:16-29` | immédiat (théorème E par ordre ; seules les verticales couplent $k$ et $k - 1$, après les deux forêts) | préfixe : ordres 1 à 5 identiques avec le catalogue à 7 et à 12 (trame 01, comptes par ordre) |
| La descente est une fonction pure de $(F, k)$ ; le mémo par cellule n'est qu'un cache | `tower.hpp:22-23`, `tower.cpp:1448-1450` | absente du dépôt ; ici, corollaire du théorème D (la valeur mémorisée est l'aboutissement de la descente du premier représentant de la cellule, toujours le même) | dumps identiques à 1, 2 et 4 fils (trame 01, $K = 5$) |
| Le semis $H_k$ (population d'une naissance régulière de $k$ sites) donne le même terminal que le chemin général | `tower.hpp:21` | absente ; immédiate : la boule minimale de $I \cup U$ est $b$ (lemme 1), cellule de naissance | variante `v_noseed` : dump identique |
| Recensement $(I, U)$ lu au catalogue quand le support certifié de la boule minimale est le support canonique d'une boule | `tower.cpp:848-858` | repose sur l'exactitude du recensement du générateur (hors lentille) ; juge d'échantillon 1 boule sur 32 (l. 884-894) | 0 refus `census_mismatch` sur toutes les entrées |
| Kruskal par plateaux : racines pré-lot figées, un nœud par groupe | `tower.cpp:1038-1040` | absente ; ici théorème C | juge L02 ; invariant (D) à l'échelle |
| Pointeurs de saut de Myers : `ancestor(v, r)` est l'ancêtre le plus haut de rang $\leq r$, égal à la remontée parent par parent | `tower.cpp:717-720` | classique ; hypothèse : rangs croissants vers la racine (strictement, par l'isolement des naissances) | dumps identiques au binaire figé d'avant les pointeurs (journal privé `build/v10-persist/g4/verify_tower.log`, 8 entrées) ; empreinte de la trame 01 à $K = 5$ retrouvée ici (`6ebb1eb4255a0903…`) |
| Image verticale d'une naissance régulière = ancêtre du dernier représentant de la jonction de la même boule à l'ordre $k - 1$ | `tower.hpp:28-29`, `tower.cpp:1678-1687` | absente ; ici théorème F (toute $(k-1)$-partie de $P_b$ convient) | juge L02 (tous les nœuds) ; invariant (C) à l'échelle |

**Hypothèses réelles de l'ensemble** : sites distincts ; catalogue complet et exact pour $p + q \leq K + 1$ ; coordonnées de 18 bits (marges des filtres flottants) ; coquilles étendues de 24 sites au plus et de 20 000 parties séparables au plus par cellule.

**Cas dégénérés.** Égalités de niveaux : lots par rang exact, théorème C ; sur la trame 00 à l'ordre 1, 7 350 rangs portent au moins deux fusions. Coquilles cosphériques ou cocycliques : quotient local par énumération, théorème B ; jugé jusqu'à $m = 12$ (campagnes : @@EXT_BALLS@@ boules étendues, histogramme au § 6.2). Sites alignés ou coplanaires : supports de 2 ou 3 sites, rien de particulier ; jugés (familles `line`, `plane`, `circle`). Sites répétés : refus.

## 6. Ce que les oracles établissent, et ce qu'ils n'établissent pas

### 6.1 Les quatre « preuves par oracle » de la v10

| Oracle ou contrôle | Ce qu'il compare | Ce qu'il établit | Ce qu'il n'établit pas |
| --- | --- | --- | --- |
| `reference/test_ref.py` (référence Python contre $\Gamma_k$) | à chaque niveau critique, coupes ouverte et fermée : la liste des **couvertures** des composantes ; la partition $C \cap X$ | que la théorie (catalogue brut, morceaux, descente) reproduit les couvertures de $\Gamma_k$ sur 25 nuages génériques et 40 grilles $\lbrace 0, 1, 2 \rbrace^{3}$ de 4 à 8 points, le carré, le cube et E5, $K \leq 4$ (rejoué ici au HEAD : @@TESTREF@@) | rien sur le C++ ; rien sur les verticales (la référence n'en a pas) ; rien au-delà de 8 points et de $K = 4$ ; non enregistré dans CTest |
| `mhgp10_tower_oracle` (C++ contre $\Gamma_k$) | à chaque niveau critique, coupes ouverte et fermée : le **nombre** de nœuds vivants ; la partition des sites entrés (`test_tower_oracle.py:136-146`) ; pour $k \geq 2$, l'image de la composante d'un site entré, **après remontée** (l. 83-87, 90-114) | le nombre de composantes à toutes les coupes et la partition $C \cap X$, sur 24 nuages de 12 points au plus, $K \in \lbrace 1, 3, 5 \rbrace$, et E5 à $K = 4$ : 73 exécutions, 23 444 coupes, 0 écart (rejoué ici) | quelles composantes fusionnent quand aucun site n'y est entré ; l'arité des fusions et l'atomicité des plateaux ; que `lower` est le nœud vivant à la coupe fermée ; que le nœud d'attache est vivant à la coupe fermée ; les ordres 6 à 10 ; un seul plancher (500 coupes) ; les contrôles verticaux ne comptent dans aucun plancher (l. 153) |
| « dumps identiques au binaire figé `4a3d09d8a` sur 8 entrées » (`PASSATION.md:88`) | sorties de deux versions du même algorithme | la neutralité d'une optimisation, le 29 septembre | l'exactitude ; ce n'est pas une porte : la trace est un journal privé (`build/v10-persist/g4/verify_tower.log`), sans reçu |
| « 1 fil = 4 fils » | étiquettes de `mhgp10_cluster` sur deux scènes de 2 000 points (`tests/regression/test_batch_equivalence.py`) ; dumps de la tour dans le même journal privé (2 entrées) | l'invariance des étiquettes au nombre de fils à 2 000 points | l'invariance de la tour elle-même aux tailles d'intérêt (rejouée ici : identique à 1, 2 et 4 fils sur la trame 01) |

`SPEC_V10.md:80` décrit la porte C++ par « forêts et partitions C∩X égales à l'oracle $\Gamma_k$ à tous les niveaux critiques » : c'est plus que ce qu'elle compare.

### 6.2 Le juge de cet audit

`l02_judge.py` construit, pour chaque ordre, l'arbre de fusion $T_k$ de la définition du § 4.1 par un balayage exhaustif de $\Gamma_k$ en `Fraction`, puis exige du dump C++ :

- **B** : bijection des naissances par (niveau exact, couverture $= I \cup U$ de la boule de naissance) ;
- **M** : pour chaque fusion, niveau exact, ensemble des feuilles et ensembles de feuilles de chaque enfant identiques ;
- **V** : pour **chaque** nœud d'ordre $k \geq 2$, `lower` est le nœud de l'ordre $k - 1$ dont l'ensemble de feuilles est celui de la composante de $\Gamma_{k-1}$, à la coupe fermée du niveau du nœud, d'une $(k-1)$-partie d'un sommet du nœud ;
- **P** : pour chaque site, niveau d'entrée $D_k(x)$ et nœud vivant à la coupe fermée de ce niveau ;
- **Euler** : $\chi_{K'} = 1$ pour tout $K' \leq \min(n, K)$, sur le catalogue construit à $K + 2$.

Résultats sur le build de référence (HEAD) :

| Campagne | Nuages | Ordres | Naissances | Fusions (dont $\geq 3$ parents) | Verticales | Attaches | Euler | Écarts |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| graine 11 : $n$ de 7 à 11, $K = 5$ (80 %) ou 7 | 240 | 1 308 ordres jugés | 14 885 | 8 613 (3 590) | 19 849 | 11 830 | 1 308 | 0 |
@@BIG_ROWS@@
@@FIX_ROW@@

Couverture de ces campagnes (mesurée par `l02_coverage.py` sur les mêmes graines) : @@COVERAGE@@.

**Limite.** Les descentes y sont courtes : @@JUMPS@@ sauts K-NN pour @@RESOLVES@@ résolutions de représentants. Le régime long du LiDAR (environ 205 000 sauts pour 3,0 millions de résolutions de représentants sur la trame 08/000100 à $K = 5$ ; environ 1,49 million de sauts à $K = 10$) n'est jugé par aucun oracle borné ; il l'est à l'échelle par la neutralité des variantes (§ 7), qui montre que deux descentes différentes aboutissent aux mêmes composantes, pas que ces composantes sont justes.

### 6.3 Six mutants

Chaque mutant est une copie de `tower.cpp` modifiée en un point (`make_mutants.py`). « Porte du dépôt » : `tests/oracle/test_tower_oracle.py` complet (73 exécutions). « Juge L02 » : 64 nuages de 6 à 9 points. « Invariant d'échelle » : trame 01, $K = 5$.

| Mutant | Faute simulée | Porte du dépôt | Juge L02 | Invariant d'échelle qui le tue |
| --- | --- | --- | --- | --- |
| `m_bin` | multifusion remplacée par une chaîne de fusions binaires de même rang | **survit** (0 écart, 23 444 coupes) | tué, 64 nuages sur 64 | (D) plateaux : 216 726 violations ; structure N-aire de l'ordre 1 contre l'EMST |
| `m_seq` | chaque jonction est son propre lot (plateau non atomique) | **survit** | tué, 45 sur 64 | (D) : 278 violations ; structure N-aire de l'ordre 1 |
| `m_vopen` | image verticale d'une naissance prise à la coupe ouverte | **survit** | tué, 64 sur 64 | (C) image vivante : 350 834 violations |
| `m_vnoclimb` | image d'une fusion = image brute du premier enfant, naturalité non vérifiée | **survit** | tué, 64 sur 64 | (C) : 395 796 violations |
| `m_strict` | `niveau <= e` remplacé par `<` dans le rang d'entrée `core` (constat AT1 de l'audit géant) | **survit** | tué, 29 sur 64 | (A) attache vivante : 133 violations |
| `m_onepiece` | coquille étendue jamais jointe | tué (35 exécutions en écart sur 73, dont des refus `root_count`) | tué sur 50 nuages : 44 refus `root_count` du produit, **6 forêts fausses publiées** avec une seule racine | non mesuré |

Cinq fautes sur six traversent la porte enregistrée. Deux d'entre elles (`m_bin`, `m_seq`) contredisent une propriété que tous les documents annoncent (« multifusions N-aires jamais binarisées », « plateaux atomiques ») : cette propriété n'a aujourd'hui aucune porte. Remarque : l'égalité des **poids** de l'ordre 1 avec ceux de l'arbre couvrant minimal ne tue pas ces deux mutants (les multiensembles restent égaux) ; c'est la comparaison de la **structure N-aire** qui les tue.

## 7. Invariants globaux : lesquels existent, lesquels sont des portes

### 7.1 Tableau

| Invariant | Coût | En v10 | Mesuré ici |
| --- | --- | --- | --- |
| Une racine par ordre | $O(\text{nœuds})$ | invariant de produit (`tower.cpp:1112-1114`, raison `root_count`) | 0 refus sur toutes les exécutions d'échelle de cet audit |
| Niveau strictement décroissant à chaque pas de descente | par pas | invariant de produit (l. 836-844) | 0 refus |
| Naturalité des verticales (tous les enfants de toutes les fusions) | $O(\text{nœuds})$ | invariant de produit (l. 1719-1732) | 0 refus |
| Recensement d'une boule sur 32 parmi celles qu'une descente atteint | par boule | invariant de produit (l. 884-894) | 0 refus |
| **Euler** $\chi_K = 1$ | $O(\text{boules})$ une fois le catalogue construit à $K + 2$ | **absent** : aucun code, aucune porte (`grep -ri euler src cli tests` : un commentaire de `types.hpp`) | $\chi_K = 1$ partout, tableaux ci-dessous |
| **Ordre 1 = arbre couvrant minimal euclidien** | juge indépendant (scikit-learn, Prim en $O(n^{2})$ : 13 à 29 s à ces tailles) | **absent** | égal, poids et structure N-aire |
| (A) attache `core` vivante à la coupe fermée : $\text{niveau}(v) \leq D_k(x) < \text{niveau}(\text{parent}(v))$ | $O(nK \log)$ | absent de la tour et de sa porte ; côté tête, `validate()` n'en contrôle qu'une version large qui admet l'égalité avec le niveau du parent (`src/points/dendrogram.cpp:27-28`) | 0 violation |
| (B) cohérence points–verticales à la coupe $D_k(x)$ | $O(nK \cdot \text{profondeur})$ | porte bornée seulement (petits nuages) | 0 violation ; ne tue aucun des cinq mutants survivants |
| (C) `lower[v]` vivant à la coupe fermée du niveau de $v$ | $O(\text{nœuds})$ | absent | 0 violation |
| (D) plateaux atomiques : aucun nœud n'a un parent de même rang ; toute fusion a au moins deux enfants | $O(\text{nœuds})$ | absent ; `validate()` admet un enfant de même rang que son parent (`src/points/dendrogram.cpp:18`) | 0 violation |
| Neutralité des règles de descente valides | deux builds | absent | dumps identiques à l'octet près |
| Invariance au nombre de fils | deux exécutions | étiquettes à 2 000 points seulement | dumps identiques à 1, 2 et 4 fils |
| Nombre de naissances | lu dans le JSON | publié, aucune porte | cohérent : identité de forêt (naissances $- 1 = \sum (\text{arité} - 1)$) ; 361 211 naissances régulières d'ordre 5 sur 08/000200, égal au décompte du prototype de la critique de conception (`TOWER_v2.md:1038`) |
| Différentiel v9 | — | comptes du catalogue par $(q, p)$ comparés une fois (`SPEC_V10.md:84-85`) | 1 407 885 boules à $K = 5$ sur 08/000200 et 1 306 696 sur 08/000000, retrouvés |

Aucun des invariants en gras n'est une porte de la v10 ; `PASSATION.md:222` les range dans les chantiers ouverts.

### 7.2 Trois trames du contrat (sans sol, grille de 1 mm)

Euler avec le catalogue à $K + 2 = 7$ ; ordre 1 contre scikit-learn ; invariants linéaires à $K = 5$.

| Trame | Sites | Boules à 7 | Boules étendues ($m$ max) | $\chi_K$, $K = 1 \ldots 5$ | $\chi_6$, $\chi_7$ (hors portée) | Arêtes EMST = liens de l'ordre 1 | Nœuds N-aires de l'ordre 1, identiques | (A) attaches | (C) images | (D) nœuds |
| --- | ---: | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | 39 885 | 2 565 656 | 320 (5) | 1, 1, 1, 1, 1 | 304 161 ; −958 732 | 39 884 | 39 796 | 199 425 | 1 462 069 | 1 541 750 |
| 08/000100 | 35 551 | 2 104 698 | 204 (5) | 1, 1, 1, 1, 1 | 223 844 ; −741 137 | 35 550 | 35 461 | 177 755 | 1 235 709 | 1 306 721 |
| 08/000200 | 45 845 | 2 675 990 | 865 (5) | 1, 1, 1, 1, 1 | 262 257 ; −909 878 | 45 844 | 45 563 | 229 225 | 1 591 680 | 1 683 088 |

Les colonnes (A), (C), (D) donnent le nombre de contrôles ; toutes à 0 violation. Les valeurs hors portée montrent que l'identité n'est pas triviale : dès qu'il manque des sphères, la somme s'écarte de $1$ par centaines de milliers.

Trame 08/000100 à $K = 10$, catalogue à 12 (6 492 748 boules, 2,0 Gio de mémoire résidente) : $\chi_K = 1$ pour $K = 1, \ldots, 10$ ($\chi_{11} = 612\,326$, $\chi_{12} = -1\,695\,373$ hors portée) ; une racine par ordre ; (A) 355 510, (B) 319 959, (C) 5 883 033, (D) 5 954 045 contrôles, 0 violation ; ordres 1 à 5 identiques (nœuds, naissances, fusions, fusions à trois parents ou plus) à ceux du calcul à $K = 5$. Le rapport voisin `build/v11-persist/audit_v10/L01_MATH_CATALOGUE.md` (juge d'Euler écrit séparément, catalogue à 12 sur les trois trames) obtient le même nombre de boules pour cette trame et la même identité aux ordres 1 à 10 : deux calculs indépendants concordent.

Structure de la tour de 08/000200 à $K = 5$ (pour dimensionner les planchers de la v11) :

| Ordre | Nœuds | Naissances | Fusions | dont $\geq 3$ parents | Rangs portant $\geq 2$ fusions | Naissances à coquille étendue |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 91 408 | 45 845 | 45 563 | 272 | 7 069 | 0 |
| 2 | 207 884 | 118 797 | 89 087 | 29 536 | 4 012 | 0 |
| 3 | 314 861 | 182 823 | 132 038 | 46 059 | 3 714 | 93 |
| 4 | 459 559 | 273 879 | 185 680 | 77 692 | 1 153 | 125 |
| 5 | 609 376 | 361 326 | 248 050 | 95 623 | 1 855 | 115 |

Les multifusions à trois parents ou plus sont un tiers à 40 % des fusions dès l'ordre 2 ; les égalités exactes de niveaux se comptent par milliers. Ce sont les deux propriétés que la porte du dépôt ne voit pas.

**Neutralité et fils** (trame 08/000100, $K = 5$, dump complet de 68 400 175 octets, forêts, verticales et attaches) : `sha256 = 6ebb1eb4255a0903a581d304c7d7194a7ef0c0cafae8bac0ad8701032f1bf0cd` pour le HEAD à 1, 2 et 4 fils, pour `v_noseed` (sans semis : 3 437 356 boules minimales calculées au lieu de 841 427) et pour `v_alt` (saut vers les $k$ sites intérieurs les plus **éloignés** du centre, autres représentants : 348 302 sauts au lieu de 205 437). Les seize premiers chiffres sont ceux du journal privé du 29 septembre pour la même entrée.

### 7.3 Tailles d'intérêt : 8 000, 16 000, 32 000

Entrées existantes de `build/v10-scale-inputs` (régime spatial). Euler avec le catalogue à 7 ; ordre 1 contre scikit-learn.

| Famille | $n$ | Boules à 7 | $\chi_K = 1$, $K = 1 \ldots 5$ | Ordre 1 = EMST (poids, structure) | Fusions des ordres 1 à 5 (dont $\geq 3$ parents) | Sauts K-NN |
| --- | ---: | ---: | --- | --- | ---: | ---: |
@@SCALE_ROWS@@

Quinze entrées sur quinze : Euler vrai aux cinq ordres, ordre 1 identique, aucune racine multiple, aucun refus.

### 7.4 Ce que coûterait un catalogue « Euler-complet »

Sur 08/000200, par $(q, p)$ (comptes du CLI `mhgp10_catalogue`) : le catalogue du produit à $K = 5$ a 1 407 885 boules ; l'ensemble nécessaire à Euler jusqu'à l'ordre 5 (toutes les sphères avec $p \leq 4$) en a 2 089 870, soit $\times 1{,}48$ ; le catalogue à $K + 2 = 7$ en a 2 675 990, soit $\times 1{,}90$. Le calcul de l'identité elle-même est négligeable (0,05 à 0,7 s ici, un fil, non optimisé ; 3,2 s pour 6,5 millions de boules).

### 7.5 Catalogues amputés : ce que la tour voit seule, ce qu'Euler voit

Expérience prévue par `TOWER_v2` § 13.2 (« amputated ») et jamais menée en v10. Sur 64 nuages de 8 à 10 points (huit familles), $K = 4$, catalogue construit à 6 : chaque boule admissible du produit ($p + q \leq 5$) est retirée tour à tour, soit 3 062 retraits ; la tour est rejugée par l'arbre étiqueté, Euler est recalculé.

| Issue | Retraits | Part |
| --- | ---: | ---: |
| la tour refuse (`census_mismatch` : 1 848 ; `root_count` : 629) | 2 477 | 80,9 % |
| la tour publie la forêt exacte (boule sans effet sur $\pi_0$) | 290 | 9,5 % |
| **la tour publie une forêt fausse, sans refus** | **295** | **9,6 %** |
| Euler détecte le retrait ($\chi_{K'} \neq 1$ pour un $K' \leq 4$) | 3 008 | 98,2 % |
| forêt fausse publiée **et** Euler muet | 0 | 0 % |

Les 54 retraits qu'Euler ne voit pas laissent tous la forêt exacte. Par type de boule, les forêts fausses publiées viennent surtout des sphères régulières à support de trois sites (157 sur 953 retraits) et de deux sites (61 sur 1 534), puis des coquilles étendues (57 sur 513). Le premier ordre en écart est l'ordre maximal $K$ dans 293 cas sur 295, et 238 de ces boules vérifient $p + q = K + 1$ : elles n'ont qu'une cellule de jonction, à l'ordre $K$, et aucune cellule de naissance construite dont l'absence ferait échouer une descente. C'est la zone aveugle annoncée par `TOWER_v2.md:760-775` ; elle est donc réelle, et large à l'ordre le plus élevé.
