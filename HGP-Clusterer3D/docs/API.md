# API

```python
from hgp_clusterer import HGPClusterer, order_k_simplices
```

## `HGPClusterer`

```python
HGPClusterer(K=2, min_cluster_size=None, method="eom", splitting=None, expZ=2.0, verbose=False)
```

Estimateur scikit-learn (`fit`, `fit_predict`, `get_params`, `set_params`), compatible avec `sklearn.base.clone`.

### Paramètres

| Paramètre | Type | Défaut | Rôle |
| --- | --- | --- | --- |
| `K` | `int`, de 1 à 63 | `2` | Ordre des simplexes : 1 pour les paires (single linkage), 2 pour les triangles, 3 pour les tétraèdres. |
| `min_cluster_size` | `float > 0` ou `None` | `None` | Masse minimale d'un cluster, en nombre de points. `None` donne `round(sqrt(n_samples))`. |
| `method` | `"eom"`, `"leaf"` ou `float > 0` | `"eom"` | Sélection des clusters : excès de masse, feuilles de l'arbre condensé, ou coupe au rayon donné (dans l'unité des données). |
| `splitting` | appelable ou `None` | `None` | Règle de découpage dynamique (voir plus bas). |
| `expZ` | `float`, dans ]0, 20] | `2.0` | Exposant de la densité : un simplexe de rayon englobant `r` pèse `(r / h) ** -expZ`, où `h` est la médiane des rayons des simplexes. |
| `verbose` | `bool` | `False` | Affiche la taille des structures intermédiaires. |

### Attributs après `fit`

| Attribut | Forme | Contenu |
| --- | --- | --- |
| `labels_` | `(n_samples,)`, `int32` | Cluster de chaque point, `-1` pour le bruit, numéroté de `0` à `m - 1`. |
| `min_cluster_size_` | `float` | Taille minimale effective. |
| `faces_unique_` | `(n_faces, K)`, `int32` | $(K-1)$-faces distinctes, nœuds de la hiérarchie. |
| `S_faces_` | `(n_faces,)`, `float64` | $S_\tau$ : somme des poids `(r / h) ** -expZ` des simplexes contenant la face. |
| `T_points_` | `(n_samples,)`, `float64` | $T_x$ : somme des $S_\tau$ des faces contenant le point. |
| `W_nodes_` | `(n_faces,)`, `float32` | $W_\tau$ : masse de la face. |
| `forest_` | `list[dict]` | Une entrée par composante connexe : arbre condensé (`"tree"`) et indices de ses faces (`"nodes"`). L'arbre donne, pour chaque cluster, ses enfants (`children`), son niveau de naissance (`r`), sa masse à la naissance (`size`), sa stabilité (`stability`) et `lambda_birth` / `lambda_death` ; pour chaque face, le premier cluster rejoint (`initial_membership`) et le niveau correspondant (`join_r`). Les niveaux (`r`, `join_r`) sont des rayons relatifs `r / h` et les densités valent `(r / h) ** -expZ`, alors que `method=ρ` s'exprime dans l'unité des données. |

Les grandeurs $S_\tau$, $T_x$ et $W_\tau$ sont définies dans [ALGORITHME.md](ALGORITHME.md#5-masses-des-faces).

### Méthodes

- `fit(X)` : `X` est un tableau `(n_samples, 3)` de valeurs finies, dont aucun point ne s'écarte de la médiane de plus de $10^{8}$ fois l'écart interquartile. La méthode calcule la hiérarchie, sélectionne les clusters et renvoie l'estimateur.
- `fit_predict(X)` : appelle `fit(X)` et renvoie `labels_`.
- `refine_clusters(method="eom", splitting=None)` : sélectionne de nouveau les clusters sur la hiérarchie déjà calculée, met à jour `labels_` et le renvoie. Aucune triangulation n'est recalculée, et la valeur de `expZ` utilisée est celle du dernier `fit`.

### Découpage dynamique

`splitting` est appelé sur chaque cluster sélectionné qui possède des enfants :

```python
def splitting(parent_points, children_points):
    ...
    return True  # remplacer le cluster par ses enfants
```

Ses deux arguments sont les suivants :

- `parent_points` : indices triés des points portés par les faces rattachées aux feuilles du sous-arbre du cluster ;
- `children_points` : liste de tableaux d'indices, un par enfant, formant une partition de `parent_points`. Chaque point y est attribué à l'enfant où il a le plus de poids.

Si la fonction renvoie `True`, le cluster est remplacé par ses enfants, et chacun est examiné à son tour ; les faces rattachées directement au cluster découpé sortent de la sélection. Sinon, le cluster est conservé tel quel.

Exemple : découper tant que chaque enfant garde au moins 20 % des points.

```python
def balanced(parent_points, children_points):
    return min(len(c) for c in children_points) >= 0.2 * len(parent_points)

labels = model.refine_clusters("eom", splitting=balanced)
```

## `order_k_simplices`

```python
simplices, radii = order_k_simplices(X, K)
```

Cette fonction renvoie les $K$-simplexes de la filtration de Delaunay d'ordre $k$ d'un nuage `X` de forme `(n_samples, 3)`. Les coordonnées doivent être finies, et aucun point ne doit s'écarter de la médiane de plus de $10^{8}$ fois l'écart interquartile ; sinon, une `ValueError` est levée.

- `simplices` : tableau `(n_simplices, K + 1)` d'entiers `int32`. Chaque ligne est un ensemble de $K+1$ points dont la cellule de Voronoï d'ordre $K+1$ est non vide ; les lignes sont triées.
- `radii` : tableau `(n_simplices,)` des rayons des boules englobantes minimales, dans l'unité de `X`.

## Parallélisme

Les triangulations, les unions d'ensembles et les rayons englobants s'exécutent sur le groupe de fils de Geogram, qui utilise tous les cœurs disponibles. Les grands tris (plus de 65 535 éléments) passent par `GEO::sort`, c'est-à-dire par la STL parallèle (TBB) avec GCC ; ils sont séquentiels avec Clang. Le graphe dual et l'arbre condensé sont calculés séquentiellement.
