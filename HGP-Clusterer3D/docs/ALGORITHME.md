# Algorithme

Cette page décrit le calcul effectué par `HGPClusterer.fit`, étape par étape, tel qu'il découle des Parties I et II du manuscrit de thèse. Les lettres $S$, $T$ et $W$ correspondent aux attributs `S_faces_`, `T_points_` et `W_nodes_` de l'estimateur.

## 1. Préparation du nuage

Le nuage $X \subset \mathbb{R}^{3}$ de $n$ points est centré sur sa médiane, puis divisé par son plus grand écart interquartile (ou, pour une distribution dégénérée, par la plus grande étendue entre ses quantiles à 1 % et 99 %, puis de sa boîte englobante). Des points aberrants ne dégradent donc pas la précision numérique du reste du nuage. Un nuage dont un point s'écarte de la médiane de plus de $10^{8}$ fois cette échelle est refusé.

Chaque point reçoit ensuite une perturbation gaussienne déterministe (graine fixe), d'amplitude relative $10^{-8}$. Elle lève les dégénérescences exactes (points dupliqués, grilles régulières, points cosphériques) : les ex æquo y sont départagés de façon arbitraire mais reproductible. Le résultat est invariant par translation et par changement d'échelle du nuage.

## 2. Simplexes de Delaunay d'ordre $k$

Pour un ensemble $S$ de $k$ points, la cellule de Voronoï d'ordre $k$ est :

$$V_k(S) = \left\lbrace x \in \mathbb{R}^{3} : \max_{p \in S} \Vert x - p \Vert \le \min_{q \notin S} \Vert x - q \Vert \right\rbrace$$

La moyenne des carrés des distances aux points de $S$ est une distance de puissance :

$$\frac{1}{k} \sum_{p \in S} \Vert x - p \Vert^{2} = \Vert x - c_S \Vert^{2} - w_S, \quad c_S = \frac{1}{k} \sum_{p \in S} p, \quad w_S = -\frac{1}{k} \sum_{p \in S} \Vert p - c_S \Vert^{2}$$

Le diagramme de Voronoï d'ordre $k$ est donc le diagramme de puissance des barycentres $c_S$ pondérés par $w_S$. Deux cellules adjacentes d'ordre $k$ diffèrent d'un seul point, et leur union est un ensemble de $k+1$ points dont la cellule d'ordre $k+1$ est non vide. Tous ces ensembles s'obtiennent ainsi.

Le calcul est itératif :

1. Les arêtes de la triangulation de Delaunay du nuage donnent les ensembles de 2 points.
2. Pour passer de $k$ à $k+1$, on calcule la triangulation régulière des barycentres $c_S$, dont les arêtes relient les cellules de puissance adjacentes. On forme ensuite l'union de chaque paire adjacente, et l'on conserve les unions de $k+1$ points sans doublon.

Les triangulations sont calculées par Geogram (`PDEL`, parallèle). Le mode pondéré relève chaque barycentre en quatrième coordonnée $\sqrt{W - w_S}$, avec $W = \max_S w_S$.

Des barycentres peuvent être exactement coplanaires, par exemple les milieux des côtés d'un quadrilatère, qui forment un parallélogramme. La triangulation régulière ne traite pas ces configurations. Chaque barycentre reçoit donc une perturbation déterministe, fonction des indices de son ensemble, d'amplitude relative $10^{-12}$. Elle est $10^{4}$ fois plus faible que celle des points et ne modifie pas les ensembles obtenus sur des nuages ordinaires. Elle peut en revanche trancher autrement quelques configurations presque dégénérées : sur des entrées exactement dégénérées (doublons, grilles, points coplanaires ou cosphériques), jusqu'à environ $2 \cdot 10^{-3}$ des ensembles pour $K \le 5$ peuvent dépendre de la numérotation des points, et un ensemble isolé contenant un point très éloigné du nuage peut être retenu à tort. Les étiquettes n'en sont pas affectées en pratique.

Au terme de $K-1$ itérations, on obtient les $K$-simplexes : les ensembles $\sigma$ de $K+1$ points tels que $V_{K+1}(\sigma) \neq \emptyset$. Ce sont des paires pour $K=1$, des triangles pour $K=2$ et des tétraèdres pour $K=3$.

Deux cas limites se traitent sans triangulation. Si $K+1 > n$, il n'existe aucun $K$-simplexe. Si $n \le 3$, tous les ensembles de $K+1$ points sont retenus, puisque les points perturbés sont en position générale.

## 3. Filtration par la boule englobante minimale

Chaque $K$-simplexe reçoit le rayon $r(\sigma)$ de sa boule englobante minimale :

- pour 2, 3 et 4 points, ce rayon est donné par des formules closes ;
- une boule englobante minimale est déterminée par au plus 4 points, donc de 5 à 8 points, le rayon est le plus grand des rayons des tétraèdres extraits du simplexe ;
- au-delà, il est calculé par l'algorithme de Welzl.

Soit $h$ la médiane de ces rayons. Le simplexe entre dans la filtration au niveau relatif $\ell(\sigma) = r(\sigma) / h$, avec la densité :

$$\lambda(\sigma) = \ell(\sigma)^{-\mathrm{expZ}}$$

Dans le calcul de $\lambda$, le niveau est borné inférieurement par $10^{-12}$ ; `expZ` est compris entre 0 (exclu) et 20. $\lambda$, $S$, $T$, les masses de l'arbre, les stabilités et les poids par point du découpage sont calculés en double précision ; les niveaux et $W$ sont stockés en simple précision.

## 4. Hypergraphe dual et réduction $K$-MST

Les nœuds sont les $(K-1)$-faces $\tau$ (ensembles de $K$ points) des $K$-simplexes. Un simplexe $\sigma = \left\lbrace u_0 < \dots < u_K \right\rbrace$ relie ses $K+1$ faces $\tau_j = \sigma \setminus \left\lbrace u_j \right\rbrace$ par le chemin $\tau_0 - \tau_1 - \dots - \tau_K$, dont toutes les arêtes sont au niveau $\ell(\sigma)$.

Un chemin suffit : à tout niveau, il connecte les mêmes faces que l'hyperarête $\sigma$. L'algorithme de Kruskal donne ensuite une forêt couvrante de poids minimal, la $K$-MST, dont chaque arbre décrit la percolation d'une composante connexe.

## 5. Masses des faces

Les poids suivants définissent la masse de chaque face :

$$S_\tau = \sum_{\sigma \supset \tau} \lambda(\sigma), \quad T_x = \sum_{\tau \ni x} S_\tau, \quad W_\tau = \sum_{x \in \tau} \frac{S_\tau}{T_x}$$

Pour un point $x$, les coefficients $S_\tau / T_x$ forment une partition de l'unité sur les faces qui le contiennent. La somme des $W_\tau$ vaut donc le nombre de points couverts, et `min_cluster_size` s'exprime en nombre de points.

## 6. Arbre condensé

La construction suit celle de HDBSCAN :

- les arêtes de la $K$-MST sont parcourues par niveau croissant, et les arêtes de même niveau sont traitées ensemble ;
- une composante devient un cluster dès que sa masse atteint `min_cluster_size`, à une tolérance relative de $10^{-6}$ près ;
- lorsque plusieurs clusters se rejoignent, ils meurent et un cluster parent naît.

La stabilité d'un cluster $C$ est :

$$\mathcal{S}(C) = \sum_{\tau \in C} W_\tau \left( \lambda_\tau - \lambda_{\mathrm{mort}}(C) \right)$$

où $\lambda_\tau = \ell_\tau^{-\mathrm{expZ}}$, $\ell_\tau$ étant le niveau auquel la face $\tau$ a rejoint $C$. $\lambda_{\mathrm{mort}}(C)$ se définit de même au niveau où $C$ rejoint un autre cluster, et vaut $0$ si $C$ ne meurt pas.

## 7. Sélection des clusters

- `method="eom"` (excès de masse) : sélection d'une famille de clusters disjoints de stabilité totale maximale. Un parent est remplacé par ses enfants lorsque la somme de leurs meilleures stabilités le dépasse strictement.
- `method="leaf"` : les feuilles de l'arbre condensé.
- `method=ρ` (réel positif) : coupe horizontale au rayon $\rho$, exprimé dans l'unité des données. On retient les clusters nés au niveau $\rho / h$ ou avant et dont le parent naît après, en excluant les faces arrivées au-delà de ce niveau.

## 8. Découpage dynamique

Si `splitting` est fourni, il est appelé sur chaque cluster sélectionné qui a des enfants, avec deux arguments :

- les indices des points portés par les faces rattachées aux feuilles de son sous-arbre ; les faces rattachées directement à un nœud interne ne sont pas comptées ;
- leur partition entre ses enfants, chaque point étant attribué à l'enfant où sa masse cumulée $S$ est maximale.

S'il renvoie `True`, le cluster est remplacé par ses enfants, et ce récursivement. Les faces rattachées directement au cluster découpé sortent alors de la sélection. `refine_clusters` relance la sélection et le découpage sur la hiérarchie déjà calculée, sans refaire la triangulation.

## 9. Des faces aux points

Chaque point reçoit le cluster qui maximise $\sum_{\tau \ni x, \tau \in C} S_\tau$. Un point dont aucune face n'est étiquetée est du bruit (`-1`). Les étiquettes sont ensuite renumérotées de `0` à `m - 1`.

## Limites

La mosaïque de Delaunay d'ordre $K$ est matérialisée intégralement. Sa taille, donc la mémoire et le temps de calcul, croît avec $K$ : en pratique, $K$ reste petit (de 1 à 5).
