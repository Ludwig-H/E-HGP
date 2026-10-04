# Lecture : énumération exacte des supports positifs minimaux $\mathcal{Q}_b$ (moteur v11)

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`
(profil par défaut, `CMakeLists.txt:62`), `public_status=not_claimed`. Lecture seule du worktree
`/workspaces/E-HGP/build/v11-claude-20261003` à `57dd21be1` (4 oct. 2026, 17:47 UTC) ; rien construit, rien commité.
GCP non utilisé. Sauf mention, les chemins `src/…`, `docs/…`, `bench/…`, `tests/…` sont relatifs à
`morsehgp3D_v11/` dans ce worktree.

## Résumé

1. Le catalogue stocke par boule $S^*$ (indices de sites triés, `kNone` au-delà de $q_{\min}$), le rang du niveau exact,
   $p$, $m$, $q_{\min}$, et une population CSR « I croissant puis U croissante ». Il **ne stocke pas le centre** :
   il faut le refaire depuis $S^*$ par `Sphere::through` (comme le juge d'Euler). $U_b$ = `Catalogue::shell(b)`,
   complète par G2.
2. $\mathcal{Q}_b$ s'énumère **sans test de minimalité explicite** : avec les prédicats stricts déjà publics de `num`
   (milieu, triangle strictement aigu et coplanaire au centre, tétraèdre contenant strictement le centre), l'indépendance
   affine et la minimalité par inclusion sont automatiques (lemme ci-dessous). Le code existe déjà quatre fois, avec arrêt
   au premier support (catalogue, tour, feuille device) ou sans arrêt (juge d'Euler, `mark_supports`).
3. Coquille régulière ($m=q_{\min}$) : $\mathcal{Q}_b=\{S^*\}$, **aucun calcul**. Coquille étendue : $\binom{m}{2}+\binom{m}{3}+\binom{m}{4}$
   prédicats. Sur les trames, $m\leq5$ et la mesure locale du juge d'Euler donne 326 supports pour 320 coquilles
   étendues (ng00, $\mathrm{Cat}_7$) : $\lvert\mathcal{Q}_b\rvert$ vaut presque toujours 1. Le coût est négligeable.
4. Chaque support $Q$ détermine sa boule ($B(Q)=b$ par M1) : les $\mathcal{Q}_b$ sont **disjoints entre boules**, et
   « un même support pour de nombreuses liaisons » se lit sur les parties de $P_b$, pas sur plusieurs boules. Formule
   exacte proposée : à l'ordre $K$, le nombre de $(K+1)$-parties (arêtes de $\Gamma_K$) contenant $Q$ vaut
   $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$.
5. Piège principal sur LiDAR : 97 % des coquilles étendues ont $m=3$, $q_{\min}=2$ (paire diamétrale plus un site), et
   **le troisième site n'appartient à aucun support**. Le squelette omet donc des sites de coquille, pas seulement les
   intérieurs.

## (a) Ce que le catalogue stocke par boule, et comment retrouver $U_b$

- **Identité d'une boule** : (centre, rayon carré) ; $S^*$ est un témoin canonique, pas l'identité géométrique
  (`src/catalogue/catalogue.hpp:3-4`, `docs/CATALOGUE.md:24-26`).
- **`CatalogueBall`** (`src/catalogue/catalogue.hpp:45-50`) :
  - `support` : `std::array<SiteIdx,4>`, $S^*$ croissant, `kNone` au-delà de $q_{\min}$ (l. 46) ;
  - `rank` : `LevelRank` ≥ 1 vers `levels()` ; `levels()[0]` vaut toujours zéro (l. 47, `docs/CATALOGUE.md:21-22`).
    `num::Level` est un rationnel exact non réduit, dénominateur > 0 (`src/num/level.hpp:1-31`). C'est le **rayon carré**
    $\lVert N\rVert^2/D^2$ (`docs/MATHEMATIQUES.md:65-69`) ;
  - `p`, `m` : cardinaux de l'intérieur strict et de la coquille (l. 48) ;
  - `qmin` ∈ 2..4, avec $p+q_{\min}\leq K+1$ (l. 49).
- **Population** : CSR `population_offsets()` (u64) et `population()` (`catalogue.hpp:155-156`). Accesseurs
  `interior(b)` (les `p` premiers) et `shell(b)` (le reste), « I croissant puis U croissante, disjoints »
  (`catalogue.hpp:159-165`). Ordre global des boules : (niveau exact, support canonique) (`docs/CATALOGUE.md:21`).
- **Centre non stocké** : ni `N`, ni `D`, ni ancre dans `CatalogueBall`. Pour un prédicat, il faut refaire la sphère
  depuis $S^*$ par `Sphere::through` de l'arité $q_{\min}$. C'est ce que fait le juge d'Euler, qui contrôle aussi
  l'égalité du niveau (`bench/catalogue_euler.hpp:229-238`). Même chose dans `AcceptSink` de la voie device
  (`src/catalogue/leaf.cpp:399-414`).
- **Retrouver $U_b$** : `catalogue.shell(BallIdx)`. Depuis la tour : `FullDomain::catalogue()`
  (`src/tower/full_domain.hpp:22`), et les naissances de la forêt portent la `BallIdx` dans `ForestNode::birth_key`
  (`src/tower/forest.hpp:12`). Les fusions portent `kNone` : les boules de fusion et de liaison interne ne sont **pas**
  conservées dans la forêt aujourd'hui (même ligne). Si seule la sphère est connue, `census(index, sphere, threshold)`
  rend I et **toute** U quand $\lvert I\rvert$ < seuil (`src/index/index.hpp:59, 89-96`).
- **Complétude de $U_b$** :
  - G2 : centre dans la boîte et $p<K$ ⇒ $P_b\subseteq L$ (`docs/MATHEMATIQUES.md:102-106`, `docs/CATALOGUE.md:42-45`) ;
  - le census de feuille parcourt tous les sites de la liste et range chaque contact dans `work.shell`
    (`src/catalogue/leaf.cpp:254-278`) ;
  - seul l'excès d'intérieurs interrompt le census, sans émission (l. 273) ;
  - le lemme R (masques de dominance) décide certains signes sans test de puissance, sans changer les listes
    (l. 243-253, `docs/CATALOGUE.md:113-122`).

  Une boule publiée a donc sa coquille entière.
- **Ce que le catalogue sait déjà de $\mathcal{Q}_b$** :
  - si $m=q$, l'émission utilise la présentation génératrice, déjà strictement positive (`leaf.cpp:283-290`) :
    q3 strictement aigu (`leaf.cpp:195`), q4 de poids barycentriques strictement positifs (`leaf.cpp:216`) ;
  - si $m\neq q$, `canonical_support` parcourt la coquille entière et **s'arrête au premier** support
    (`src/catalogue/support.cpp:89-111`). Seul $S^*$ survit ; les autres supports ne sont pas conservés.

## (b) Algorithme exact pour énumérer $\mathcal{Q}_b$

### Définition et lemme d'équivalence

Définition de l'utilisateur (option 2 de `Zoltan/FoundationModel/JETON.md:66-72`) :
$\mathcal{Q}_b=\lbrace Q\subseteq U_b : c_b\in\mathrm{relint}(\mathrm{conv}\,Q),\ Q \text{ affinement indépendant},\ 2\leq\lvert Q\rvert\leq4\rbrace$.
C'est la définition même du « support » de `docs/MATHEMATIQUES.md:24-26`, sans restriction au cardinal minimal.

**Lemme (Carathéodory strict).** Pour $Q\subseteq U_b$, il y a équivalence entre :
(i) $Q$ est affinement indépendant et $c\in\mathrm{relint}\,\mathrm{conv}\,Q$ ;
(ii) $c\in\mathrm{conv}\,Q$ et aucun $Q'\subsetneq Q$ ne vérifie $c\in\mathrm{conv}\,Q'$.

- (i)⇒(ii) : les coordonnées barycentriques sont uniques et toutes > 0. Une représentation sur $Q'\subsetneq Q$ en
  donnerait une autre, avec un zéro.
- (ii)⇒(i) : par minimalité, tous les poids sont > 0. Une dépendance affine $\sum\mu_i x_i=0$, $\sum\mu_i=0$, $\mu\neq0$
  permet de translater les poids jusqu'à en annuler un : on obtient un sous-ensemble propre, contradiction.

En conséquence :

- $\lvert Q\rvert\leq4$ en dimension trois ;
- $\lvert Q\rvert\geq2$, car $c\notin U_b$ pour une boule positive ;
- « minimal par inclusion » n'est **pas un test supplémentaire** dès qu'on utilise des prédicats stricts.

La v9 fait l'inverse : prédicats **fermés** puis rejet des masques dont un sous-masque contient déjà $c$
(`morsehgp3D_v9/src/tower/forest/local_plateau.hpp:193-207`). Les deux formulations donnent le même ensemble.
`docs/CATALOGUE.md:203-205` et l'en-tête du juge (`bench/catalogue_euler.hpp:9-11`) utilisent le même lemme.

### Traduction en prédicats entiers, par cardinal

Tous les points de $Q$ sont sur la sphère de centre $c$. Les prédicats sont déjà publics dans `num`
(`src/num/geometry.hpp:155-169`).

| $\lvert Q\rvert$ | test exact | justification | code |
| --- | --- | --- | --- |
| 2 | `is_midpoint(sphere, a, b)` | un point du segment ouvert équidistant des deux bouts en est le milieu ; sites distincts garantis par les poids un | `src/num/predicates.cpp:232-240` |
| 3 | `strictly_acute(a,b,c)` puis `orientation(a,b,c,sphere)==0` | coplanaire ⇒ $c$ est le centre circonscrit dans le plan ; il est dans le triangle ouvert ssi le triangle est strictement aigu ; l'acuité stricte exclut l'alignement | `predicates.cpp:305-309`, `183-214`, `311-320` ; `geometry.hpp:160-161` |
| 4 | `strictly_inside(sphere, a,b,c,d)` | chaque face non dégénérée, centre strictement du même côté que le sommet opposé ; quatre points coplanaires ⇒ `false` (signe 0), et Carathéodory fournit alors un triangle ou une paire | `predicates.cpp:216-230` |

Aucun test d'indépendance affine séparé n'est nécessaire :

- deux sites distincts sont toujours indépendants ;
- l'acuité stricte exclut l'alignement ;
- `center_inside` refuse un tétraèdre plat (`predicates.cpp:223-224`).

Un triangle **droit** a son centre sur l'hypoténuse : la paire est un support, le triangle ne l'est pas, et
`strictly_acute` l'exclut. Un triangle **obtus** a son centre hors du triangle.

Ne **pas** utiliser `q4_presentation_strictly_inside()` pour un tétraèdre de $U_b$ autre que celui de la fabrique : ce
drapeau ne vaut que pour la présentation qui a construit la sphère (`src/num/geometry.hpp:59-60`,
`src/num/q4_weights.hpp:1`).

### Code existant (quatre copies des mêmes boucles)

1. `src/catalogue/support.cpp:16-111` : paires, triangles, tétraèdres, arrêt au premier, puis
   `fail(catalogue_invariant)` si rien n'est trouvé (l. 110).
2. `src/tower/canonical.cpp:15-69` : port explicite, au cardinal $q$ donné, arrêt au premier.
3. `src/catalogue/leaf_device.hpp:189-226` : copie hôte/device ; tout chemin non certifié rend `unresolved`.
4. `bench/catalogue_euler.hpp:137-165` (`mark_supports`) : **énumère tous les supports sans arrêt**, dans l'ordre
   (cardinal, positions lexicographiques). Le premier trouvé est $S^*$ (contrôle l. 273-276), et le compteur `supports`
   les additionne.

C'est déjà $\mathcal{Q}_b$, à ceci près que les supports sont marqués dans des masques de $2^m$ bits plutôt que rendus
en liste. La copie 4 est la base naturelle du port.

### Algorithme proposé (exact, déterministe, sans dépendance aux identifiants)

```text
supports(cat, cloud, b):
  ball = cat.balls_data()[b] ; U = cat.shell(b)            # triée par SiteIdx
  si ball.m == ball.qmin : rendre [ball.support[0..qmin)]  # régulière : {S*}, aucun prédicat
  si ball.m > kMaxSupportShell : refus explicite (pas de préfixe publié)
  sphere = Sphere::through(points de S*)                   # même arité que qmin ; contrôle : level == levels()[rank]
  pour chaque paire i<j de U       : is_midpoint            -> émettre
  pour chaque triplet i<j<k de U   : strictly_acute && orientation(.,sphere)==0 -> émettre
  pour chaque quadruplet i<j<k<l   : strictly_inside(sphere, .)                 -> émettre
  invariant : premier émis == S* (cardinal minimal puis ordre lexicographique), sinon refus d'invariant
```

- **Sortie** : des tuples de `SiteIdx` triés, de 2 à 4 sites, plus l'arité. Aucune coordonnée flottante n'est produite.
  La géométrie se relit dans le `Cloud`.
- **Mode transactionnel** :
  - deux passes (compter puis remplir), comme le catalogue (`docs/CATALOGUE.md:149-150`) ;
  - ou une case bornée par boule, puisque $\lvert\mathcal{Q}_b\rvert\leq\binom{m}{2}+\binom{m}{3}+\binom{m}{4}$
    sous le plafond de coquille.
- **Accélérations exactes facultatives** (inutiles pour $m\leq5$) :
  - un site a au plus un antipode, cherché comme $2c-x$ dans $U$, ce qui rend la boucle des paires en $O(m\log m)$ ;
  - on peut ignorer les triplets qui contiennent une paire diamétrale : le centre est alors sur une arête, jamais
    dans le triangle ouvert.

### Budgets de bits (expressions de `src/num/budgets.hpp:12-28`)

$M=2^B$, profils u18 / u21 / u24.

| quantité | expression | u18 | u21 | u24 | type |
| --- | --- | ---: | ---: | ---: | --- |
| produit scalaire (acuité) | $2B+2$ | 38 | 44 | 50 | i64 (`budgets.hpp:13`, `predicates.cpp:305-309`) |
| produit vectoriel | $2B+1$ | 37 | 43 | 49 | i64 |
| déterminant de 4 points (faces q4) | $3B+3$ | 57 | 66 | 75 | i64 en u18, i128 sinon (`predicates.cpp:287-293`) |
| test du milieu | $<2^{5B+7}$ | 97 | 112 | 127 | i128, `static_assert` (`predicates.cpp:233-234`) |
| centre q3 : N / D | $5B+5$ / $4B+5$ | 95/77 | 110/89 | 125/101 | i128 (`budgets.hpp:16-17`) |
| centre q4 : N / D | $4B+5$ / $3B+4$ | 77/58 | 89/67 | 101/76 | i128 (`budgets.hpp:18-19`) |
| orientation du centre (q3, q4) | $7B+9$ | 135 | 156 | 177 | i128 seulement sous certificat, sinon `Wide` (`predicates.cpp:183-214`) |
| puissance (recontrôle du census) | $6B+8$ | 116 | 134 | 152 | i128 natif pour q1/q2/q4 ; q3 sous certificat, sinon essai vérifié puis `Wide` (`predicates.cpp:40-101`) |
| niveau : numérateur / dénominateur | $8B+12$ / $6B+8$ | 156/116 | 180/134 | 204/152 | `Wide` (`budgets.hpp:26-27`) |

Certificat d'orientation i128 : $D<2^{124-3B}$ et $\lvert N_j\rvert<2^{124-2B}$, soit $2^{70}/2^{88}$ en u18,
$2^{61}/2^{82}$ en u21, $2^{52}/2^{76}$ en u24 (`src/num/orientation_certificate.hpp:7-16`). Il est presque toujours
vrai pour les petites boules LiDAR, mais `Wide` prend le relais sans refus. Toutes les décisions restent exactes ; le
certificat ne choisit que la voie de calcul.

**À ajouter.** Aucun prédicat nouveau n'est nécessaire. Je propose :

1. Une fonction unique `num::is_positive_support(const Sphere&, std::span<const Point>)` pour $\lvert Q\rvert$ = 2..4.
   Elle composerait les trois tests ci-dessus et éviterait une cinquième copie. Les quatre copies existantes pourraient
   y converger plus tard, sous leurs portes.
2. Un énumérateur `for_each_support(Cloud, Catalogue, BallIdx, sink)` dans le module de sortie. Je recommande un module
   neuf `supports`, ou `head`, plutôt que `catalogue`, car le catalogue reste sur le chemin chaud de 100 ms. Il aurait :
   - un plafond déclaré `kMaxSupportShell` (24 comme le juge, `bench/catalogue_euler.hpp:30`, ou 12 comme la v9,
     `local_plateau.hpp:50`) ;
   - un refus explicite au-delà.
3. En option, pour le modèle de fondation : les **poids barycentriques exacts** de $c$ dans chaque support (rationnels).
   Aujourd'hui seuls ceux de la présentation q4 existent (`src/num/q4_weights.hpp:21-38`).
4. Un oracle différentiel existe déjà : le juge Fraction `tests/catalogue/euler_oracle.py:75-114`. Il énumère les
   supports par « centre circonscrit égal au centre de la boule et poids strictement positifs » (formulation de Gram
   indépendante). Pour $m\leq8$, il les recoupe par l'enveloppe faible. Il suffit de lui faire rendre la **liste**, et
   non plus seulement le compte.

## (c) Taille de $\mathcal{Q}_b$ et coût

**Bornes.**

- Paires : au plus $\lfloor m/2\rfloor$. La symétrie $x\mapsto 2c-x$ est une involution sans point fixe sur la sphère,
  donc chaque site a au plus un antipode dans $U$.
- Triangles : au plus $\binom{m}{3}$. Ils sont portés par des grands cercles (plans passant par $c$).
- Tétraèdres : au plus $\binom{m}{4}$.
- Borne triviale de $\lvert\mathcal{Q}_b\rvert$ : $\lfloor m/2\rfloor+\binom{m}{3}+\binom{m}{4}$, soit 17 pour $m=5$.
- Fait externe **non vérifié ici** : la profondeur simpliciale maximale en dimension 3 est asymptotiquement de l'ordre
  de $\binom{m}{4}/8$ (Wendel donne 1/8 pour quatre points symétriques aléatoires).
- Régulière ($m=q_{\min}$) : $\lvert\mathcal{Q}_b\rvert=1$ exactement.
- $m=3$ étendue : forcément $q_{\min}=2$, et $\lvert\mathcal{Q}_b\rvert=1$. Le triangle contient la paire diamétrale,
  et le troisième site n'a pas d'antipode.

**$m$ ≤ ?**

- Le catalogue ne borne pas $m$ par $K$ : seuls $p\leq K-1$ et $p+q_{\min}\leq K+1$ sont bornés.
- $U_b$ est inclus dans la liste de feuille (G2), et la coquille est écrite dans `work.shell` de capacité égale à la
  feuille (`src/catalogue/catalogue.cpp:49-64`, `leaf.cpp:276`). Donc $m\leq$ `max_leaf` (256 par défaut, au plus
  1 024 : `catalogue.hpp:27-28`, `internal.hpp:12`).
- $\binom{1024}{4}\approx4{,}6\cdot10^{10}$ : sans plafond, l'énumération n'est pas bornée en pratique.
- FULL plafonne déjà la partie énumérée d'une cellule étendue à $t\leq12$ (`kMaxMebSites`,
  `src/tower/cells.cpp:16-17`), mais pas $m$.

**Mesures.**

- Comptes gravés (portes `mhgp11_catalogue_euler_lidar_*`, `tests/catalogue/tests.cmake:142-168`,
  `docs/CATALOGUE.md:238-243`). Ils portent sur **$\mathrm{Cat}_{K+2}$** (le juge travaille sur `large`,
  `bench/catalogue_euler.cpp:379`), et majorent donc $\mathrm{Cat}_K$ :

  | trame | étendues à $K=5$ (sur $\mathrm{Cat}_7$) | étendues à $K=10$ (sur $\mathrm{Cat}_{12}$) |
  | --- | ---: | ---: |
  | ng00 | 320 | 529 |
  | ng01 | 204 | 341 |
  | ng02 | 865 | 1 559 |

  `coquille_max=5` sur les trames, 4 sur l'uniforme (`tests.cmake:140, 164`), et « m ≤ 5 jusqu'à $\mathrm{Cat}_{12}$ »
  (`docs/CATALOGUE.md:207-208`).

- Mesure locale du 4 oct. 2026 (JSON du juge, **non reçue, non ancrée à un commit**),
  `/workspaces/E-HGP/build/v11-local/euler_tmp/lidar_*.json` :

  | trame, $K$ | répartition par $m$ (3 / 4 / 5) | supports dans les coquilles étendues |
  | --- | --- | ---: |
  | ng00, 5 | 313 / 2 / 5 | 326 |
  | ng01, 5 | 202 / 1 / 1 | 206 |
  | ng02, 5 | 848 / 8 / 9 | 878 |
  | ng00, 10 | 519 / 2 / 8 | 538 |
  | ng01, 10 | 339 / 1 / 1 | 343 |
  | ng02, 10 | 1 534 / 11 / 14 | 1 578 |

  Sur toutes les coquilles étendues, $\lvert\mathcal{Q}_b\rvert$ moyen vaut entre 1,01 et 1,02. Le nombre total de
  supports est donc à peine supérieur au nombre de boules.

**Coût.**

- Boules régulières : nul. On recopie $S^*$, sans sphère ni prédicat.
- Boules étendues : une `Sphere::through` (q3/q4 : `materialize`, `src/num/sphere.cpp:59-67, 101-118`), puis au plus
  $\binom{5}{2}+\binom{5}{3}+\binom{5}{4}=25$ prédicats. Sur ng02 à $K=10$, cela fait moins de $4\cdot10^4$ évaluations.
- Comparaison : le juge d'Euler entier (recensement de toutes les boules compris) coûte 0,5 à 2,4 s sur
  $\mathrm{Cat}_{K+2}$ (`docs/CATALOGUE.md:233-245`). La part $\mathcal{Q}_b$ est sans effet sur le contrat de 100 ms.
  Le coût dominant de la sortie « supports » sera la **taille de sortie** (≈ 20 octets par support : 4 `SiteIdx` et
  l'arité) et le rattachement des boules aux nœuds, pas la géométrie.
- Boules concernées à l'ordre $K$ : la fenêtre d'événements est $[p+q-1,p+m]$ (`docs/MATHEMATIQUES.md:84`, T3
  l. 188-198). Comme $p+q\leq K+1$ dans $\mathrm{Cat}_K$, la condition se réduit à $p+m\geq K$. Je n'ai pas ce compte
  sur les trames ; il est à mesurer.

**Nombre de liaisons d'un support (proposition exacte, sans identifiants).**

- Par M2 (`docs/MATHEMATIQUES.md:42-44`), toute partie $G$ avec $Q\subseteq G\subseteq P_b$ a $B(G)=b$.
- À l'ordre $K$, les arêtes de $\Gamma_K$ au niveau $\lambda_b$ qui contiennent $Q$ sont donc les $(K+1)$-parties de
  $P_b$ contenant $Q$. Leur nombre est $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$ (Vandermonde sur intérieurs et
  coquille). Les sommets ($K$-parties) contenant $Q$ sont au nombre de $\binom{p+m-\lvert Q\rvert}{K-\lvert Q\rvert}$.
- Contrôle régulier : $p=0$, $m=q=3$.
  - $K=2$ : une arête, $\lbrace a,b,c\rbrace$, qui joint ses trois faces ;
  - $K=3$ : zéro arête, un sommet (la naissance).
- Les comptes par cardinal des parties non séparables de $U$ existent déjà dans `closure_counts`
  (`bench/catalogue_euler.hpp:108-123`). Sommer la formule sur $Q$ compte plusieurs fois une partie qui contient
  plusieurs supports. C'est à trancher avec l'utilisateur (question 2).

## (d) Pourquoi $\mathcal{Q}_b$ ne dépend pas des identifiants

- La boule est identifiée géométriquement par (centre, rayon carré) (`catalogue.hpp:3-4`). $U_b$ est l'ensemble des
  sites à puissance nulle, et G2 le donne complet quelle que soit la feuille propriétaire (`docs/MATHEMATIQUES.md:102-106`).
- Les trois prédicats ne lisent que des coordonnées entières et la sphère. Celle-ci est refaite depuis n'importe quel
  support : un autre support donne le même centre et le même rayon, et l'arithmétique exacte rend les mêmes signes.
  L'ancre et le certificat changent la voie de calcul, pas la décision.
- Seuls dépendent des `SiteIdx` le **choix de $S^*$** (premier lexicographique, `docs/MATHEMATIQUES.md:26-28`) et
  l'**ordre** d'énumération. L'ensemble $\mathcal{Q}_b$ n'en dépend pas, contrairement au représentant que dénonce
  `Zoltan/FoundationModel/JETON.md:49-53`. La fixture du carré (deux diagonales, invariance sous 24 permutations)
  le vérifie en 2D : `Zoltan/FoundationModel/reference/verify_support_carrier.py:75-93`.
- **Disjonction entre boules** : $Q\in\mathcal{Q}_b$ ⇒ $c_b\in\mathrm{conv}\,Q$ avec $Q$ sur la sphère ⇒ $B(Q)=b$
  (M1, `docs/MATHEMATIQUES.md:30-40`). Un support appartient donc à une seule boule. Comme une boule de la fenêtre
  touche un seul nœud de l'arbre d'ordre $K$ (T3 : toutes les $K$-parties de $P_b$ sont connectées à $\lambda_b$,
  `docs/MATHEMATIQUES.md:188-192`), chaque support apparaît dans la liste propre d'un **seul** nœud.
- Portée exacte de l'invariance :
  - permutations des identifiants : oui ;
  - symétries exactes de la grille (translations entières, permutations et réflexions d'axes dans le domaine) : oui ;
  - rotations quelconques : non, car la quantification à 1 mm casse l'équivariance (fait 5 de `CLAUDE.md`, Zoltan).
- Pour un encodage canonique sans identifiants, trier la sortie par coordonnées, ou par (arité, coordonnées), et non
  par `SiteIdx`.

## (e) Pièges

1. **Sites absents du squelette.**
   - Les intérieurs d'abord (`Zoltan/FoundationModel/JETON.md:82-85`).
   - Aussi des sites de coquille. Si $m=3$ avec $q_{\min}=2$, le troisième site n'est dans aucun support. C'est
     ≈ 97 % des coquilles étendues sur LiDAR (313/320 sur ng00 à $K=5$).
   - $\bigcup\mathrm{conv}\,Q$ n'est ni $\mathrm{conv}(U_b)$ ni la population (fixture du carré,
     `verify_support_carrier.py:110-130`).
   - Un nœud sans boule (naissances d'ordre 1, sites) n'a aucun support : prévoir `has_support_geometry=false`
     (`JETON.md:82-85`).
2. **Centre absent du catalogue** : le refaire depuis $S^*$, à l'arité $q_{\min}$. Contrôler l'égalité du niveau
   (`bench/catalogue_euler.hpp:237-238`) ; ne jamais réduire ni réancrer.
3. **Coquilles dégénérées** :
   - points cocirculaires sur un grand cercle : paires et triangles, aucun tétraèdre formé de quatre points de ce cercle ;
   - points cocirculaires sur un petit cercle : le centre n'est pas dans leur plan, donc seulement des tétraèdres mêlant
     d'autres sites ;
   - réseaux cubiques synthétiques : beaucoup de points entiers sur une même sphère, donc $m$ grand et coût en
     $\binom{m}{4}$.

   La voie exacte gère tout cela sans jitter ; seul le plafond de $m$ doit être déclaré et refusé explicitement.
4. **Coplanarité** : quatre points coplanaires ne forment jamais un support q4 (`predicates.cpp:223-224`). Le bon témoin
   est le triangle ou la paire que Carathéodory fournit, et il est énuméré. Un triangle droit n'est pas un support
   (sa paire l'est).
5. **Drapeau de présentation q4** : `q4_presentation_strictly_inside()` ne vaut que pour le tétraèdre générateur
   (`geometry.hpp:59-60`). Pour les autres quadruplets de $U_b$, utiliser `strictly_inside`.
6. **Exactitude** :
   - pas de flottant dans les décisions ;
   - `Wide` hors certificat ;
   - un refus `arithmetic_invariant` ne peut venir que d'un budget violé : c'est un défaut, jamais un rejet géométrique ;
   - en u21/u24, la puissance q3 n'est en i128 que sous certificat (`docs/CATALOGUE.md:81-83`).
7. **Quatre copies du même code** (support, tour, device, juge) : un cinquième port doit être explicite, épinglé et
   requalifié (`docs/PROVENANCE.md`). L'invariant « premier support émis = $S^*$ » est le contrôle croisé gratuit.
8. **Entrée** : poids différents de un refusés (`src/catalogue/catalogue.cpp:23-24`). Les supports portent des
   `SiteIdx` du `Cloud` source, pas les identifiants de points d'entrée ; prévoir la correspondance sites → retours.
9. **Rattachement aux nœuds** : la forêt ne garde que la boule de naissance (`src/tower/forest.hpp:12`). Pour
   « naissance + fusions + liaisons internes », il faut enregistrer pendant les plateaux les boules
   d'événement/continuation de l'ordre $K$ (hors du champ de cette lecture).
10. **Redondance utile** : $\mathcal{Q}_b$ est exactement la famille des parties non séparables minimales de la cellule.
    Par T2 et Carathéodory, $A\subseteq U_b$ est strict ssi $A$ ne contient aucun $Q\in\mathcal{Q}_b$
    (`docs/MATHEMATIQUES.md:171-174`, `docs/CATALOGUE.md:203-205`). Les `bounded_meb` des cellules étendues
    (`src/tower/cells.cpp:76-87`) pourraient se remplacer par ce test de masques. C'est une optimisation possible, à
    porter seulement avec sa porte.

## Portes suggérées (sans exécution ici)

- **Oracle borné** : la liste $\mathcal{Q}_b$ native contre `euler_oracle.py`, étendu à rendre ses masques
  (`tests/catalogue/euler_oracle.py:75-114`), sur les 43 petits nuages cosphériques et cocycliques existants
  (`docs/CATALOGUE.md:218-219`).
- **Fixtures gravées** :
  - carré (deux diagonales) ;
  - triangle droit avec point en plus (un seul support, troisième site orphelin) ;
  - octaèdre régulier (trois paires, aucun triangle ni tétraèdre minimal) ;
  - cube entier ($m=8$ : quatre paires diagonales, et des tétraèdres comme $\lbrace(0,0,0),(1,1,0),(1,0,1),(0,1,1)\rbrace$).
- **Invariants globaux à l'échelle** ($n$ = 8 000 / 16 000 / 32 000 et trames) :
  - $\sum_b\lvert\mathcal{Q}_b\rvert$ = boules régulières + compteur `supports` du juge ;
  - premier support = $S^*$ ;
  - permutation des identifiants ⇒ même multiensemble de supports en coordonnées.
- **Mutants** : triangle droit accepté ; drapeau q4 de présentation utilisé pour un autre quadruplet ; arrêt au premier
  support.
