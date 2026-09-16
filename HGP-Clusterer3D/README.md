# HGP-Clusterer 3D

HGP-Clusterer (version 1.0) regroupe les points de nuages 3D par densité, selon l'algorithme HGP (Hypergraphe-Percol'). Le cœur de calcul est écrit en C++, avec [Geogram](https://github.com/BrunoLevy/geogram) comme moteur géométrique. L'interface Python suit l'API de scikit-learn.

- Référence : [hal-05369659](https://inria.hal.science/hal-05369659)
- Équipe : [Inria AYANA](https://team.inria.fr/ayana/)
- Fiche logicielle : [`fiche_HGP-Clusterer3D.json`](fiche_HGP-Clusterer3D.json) (CodeMeta)

## Contexte théorique

L'algorithme repose sur les deux premières parties du manuscrit de thèse de Louis Hauseux.

**Partie I — Modèle de Hartigan et clustering par densité**

- **Modèle de Hartigan** (1975, 1981) : les clusters de haute densité sont les composantes connexes des ensembles de sur-niveau $E(\lambda) = \left\lbrace x \in \mathbb{R}^{d} : f(x) \ge \lambda \right\rbrace$ de la densité $f$.
- **Limites du single linkage euclidien** : le single linkage souffre de l'effet de chaînage. Quelques points isolés dans une région peu dense suffisent à relier deux clusters denses distincts.
- **Interactions d'ordre supérieur** : pour surmonter cet écueil sans imposer de forme a priori, comme le font les k-moyennes ou les mélanges gaussiens, HGP étend les graphes de voisinage aux hypergraphes géométriques et aux complexes simpliciaux. Ceux-ci sont construits à partir des triangulations de Delaunay et des $\alpha$-formes d'ordre $k$.

**Partie II — Percolation d'hypergraphes et algorithmique HGP**

- **Percolation d'ordre $k$** : HGP étudie la connectivité des simplexes d'ordre $k$ (paires pour $k=1$, triangles pour $k=2$, tétraèdres pour $k=3$), filtrés par le rayon de leur boule englobante minimale.
- **Adjacences et réduction par $K$-MST** : la thèse démontre l'équivalence entre la multicouverture de boules, les polyèdres de Gabriel d'ordre $k$ et la réduction des composantes non triviales par arbre couvrant de poids minimal d'ordre $k$ ($K$-MST).
- **Découpage dynamique** : les clusters se découpent sous différents seuils de densité ou d'échelle, sans recalculer la triangulation.

Le détail du calcul se trouve dans [docs/ALGORITHME.md](docs/ALGORITHME.md).

## Installation

Prérequis :

- Python 3.9 ou plus récent ;
- un compilateur C++17 et CMake 3.18 ou plus récent ;
- Eigen 3 ;
- TBB, requis par Geogram sous Linux ;
- Geogram (version testée : 1.10.1).

Sous Debian ou Ubuntu, installer d'abord les outils et Geogram :

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3-dev python3-venv libeigen3-dev libtbb-dev

git clone --depth 1 --branch v1.10.1 --recurse-submodules https://github.com/BrunoLevy/geogram.git
cmake -S geogram -B geogram/build -DCMAKE_BUILD_TYPE=Release -DGEOGRAM_LIB_ONLY=ON \
      -DGEOGRAM_WITH_GRAPHICS=OFF -DGEOGRAM_WITH_LUA=OFF -DGEOGRAM_WITH_LEGACY_NUMERICS=OFF \
      -DGEOGRAM_WITH_HLBFGS=OFF -DGEOGRAM_WITH_TETGEN=OFF -DGEOGRAM_WITH_TRIANGLE=OFF
cmake --build geogram/build --parallel
sudo cmake --install geogram/build --prefix /usr/local
sudo ldconfig
```

Puis installer le paquet dans un environnement virtuel, depuis le dossier `HGP-Clusterer3D` :

```bash
cd HGP-Clusterer3D
python3 -m venv .venv
. .venv/bin/activate
pip install .
```

Si Geogram est installé ailleurs que dans `/usr/local`, indiquer son préfixe à la compilation :

```bash
GEOGRAM_INSTALL_PREFIX=/chemin/vers/geogram pip install .
```

## Utilisation

```python
import numpy as np
from hgp_clusterer import HGPClusterer

rng = np.random.default_rng(0)
X = np.vstack([
    rng.normal((0, 0, 0), 0.5, (500, 3)),
    rng.normal((4, 0, 0), 0.5, (500, 3)),
    rng.uniform(-2, 6, (100, 3)),
])

model = HGPClusterer(K=2, min_cluster_size=30)
labels = model.fit_predict(X)  # -1 : bruit

leaves = model.refine_clusters("leaf")   # feuilles de l'arbre condensé
cut = model.refine_clusters(0.5)         # coupe au rayon 0.5
```

| Paramètre | Défaut | Rôle |
| --- | --- | --- |
| `K` | `2` | Ordre des simplexes (1 : paires, 2 : triangles, 3 : tétraèdres, ...). |
| `min_cluster_size` | `round(sqrt(n))` | Masse minimale d'un cluster, en nombre de points. |
| `method` | `"eom"` | `"eom"` (excès de masse), `"leaf"`, ou un rayon de coupe. |
| `splitting` | `None` | Règle de découpage dynamique `f(parent_points, children_points) -> bool`. |
| `expZ` | `2.0` | Exposant de la densité : un simplexe de rayon englobant `r` pèse `(r / h) ** -expZ`, où `h` est la médiane des rayons des simplexes. |
| `verbose` | `False` | Affichage des tailles intermédiaires. |

L'API complète est décrite dans [docs/API.md](docs/API.md).

## Notebook

[`notebooks/HGP-Clusterer3D_demo.ipynb`](notebooks/HGP-Clusterer3D_demo.ipynb) applique le paquet à un nuage 3D jouet (deux spirales, un amas et du bruit). Dans Google Colab, il suffit de déposer le dossier `HGP-Clusterer3D` (ou son archive `HGP-Clusterer3D.zip`) dans `/content` : la première cellule compile Geogram et installe le paquet. Il compare HGP à HDBSCAN et illustre le raffinement sans recalcul. Hors de Colab, il demande en plus Jupyter, `matplotlib` et `scikit-learn>=1.3`.

## Tests

```bash
pip install ".[test]"
pytest
```

Deux fichiers de tests jouets en 3D :

- `tests/test_order_k.py` confronte les simplexes d'ordre $k$ et leurs rayons à un calcul exhaustif (programmation linéaire), pour $K = 1$ à $4$. Il couvre aussi les petits nuages, les points dupliqués et les entrées invalides.
- `tests/test_clustering.py` vérifie la séparation d'amas bruités, le raffinement sans recalcul et l'invariance par translation et par changement d'échelle.

## Organisation

```text
HGP-Clusterer3D/
├── CMakeLists.txt               construction des extensions (scikit-build-core)
├── pyproject.toml
├── src/hgp_clusterer/
│   ├── estimator.py             HGPClusterer
│   ├── hypergraph.py            simplexes d'ordre k et hypergraphe dual
│   ├── hierarchy.py             sélection des clusters et découpage dynamique
│   ├── _geometry.cpp            Delaunay d'ordre k (Geogram)
│   ├── enclosing_ball.hpp       boules englobantes minimales
│   └── _hierarchy.pyx           graphe dual, Kruskal, arbre condensé
├── tests/
├── notebooks/
└── docs/
```

## Auteurs

- Louis Hauseux (Université Côte d'Azur, programmes 3IA-DS4H ; louis.hauseux@inria.fr, louis.hauseux@gmail.com) : architecture, développement, tests, documentation.
- Josiane Zerubia (Inria) : conception, tests.
- Konstantin Avrachenkov (Inria) : conception, documentation.

## Licence

HGP-Clusterer n'est pas un logiciel libre. Il est distribué sous la licence non commerciale de [LICENSE](LICENSE) : l'usage académique, éducatif ou personnel est autorisé, et tout usage commercial est interdit sans l'autorisation écrite préalable de l'auteur. Les bibliothèques tierces utilisées à la compilation (Geogram, Eigen, pybind11, TBB) conservent leurs propres licences, qui ne changent rien à celle de HGP-Clusterer.

Une licence devra être définie avec le STIP d'Inria dans le cas d'un dépôt à l'APP (Agence pour la protection des programmes) et pour le projet Percolia avec l'Inria Startup Studio.
