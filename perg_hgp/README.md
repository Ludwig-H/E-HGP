# `perg_hgp` — prototype de référence `PowerCover3D`

> [!WARNING]
> Ce package est conservé comme **référence historique exécutable**. Il évalue un champ de puissance régularisé sur une grille cubique et construit sa forêt $H_0$. Il n'implémente ni le futur backend exact `MorseHGP3D`, ni le futur backend haute dimension `HomogeneousLensTower`.

## Pourquoi ce code reste présent

`PowerCover3D` contient encore des éléments utiles pour la suite du projet :

- normalisation et contrats numériques explicites;
- régularisation locale en lots;
- routage CPU/CUDA et budgets mémoire;
- top-$K$ de puissance exact sur CPU et gardé sur CUDA;
- forêt couvrante cubique déterministe;
- sérialisation, diagnostics et tests de non-régression.

Sa grille uniforme, sa connectivité 26-voisins et ses sorties `inner/outer` sont propres à ce prototype. Elles ne doivent pas migrer implicitement vers les nouveaux backends.

L'ancien atlas `PERGHGPClusterer` et ses dépendances Torch ont été retirés. L'historique complet demeure disponible dans le [snapshot `0b8a1a1`](https://github.com/Ludwig-H/E-HGP/tree/0b8a1a11750b931f486ce666265eed4b6e95e2b1/perg_hgp).

## Installation CPU

Depuis la racine du dépôt :

```bash
python -m pip install -e 'perg_hgp[test]'
```

Le chemin CPU dépend uniquement de NumPy et SciPy. Les bibliothèques CUDA/RAPIDS sont chargées de façon optionnelle par le runtime spécialisé.

## Exemple minimal

```python
import numpy as np

from perg_hgp import PowerCover3D, PowerCoverConfig

points = np.random.default_rng(0).normal(size=(2_000, 3)).astype("float32")

config = PowerCoverConfig(
    K=3,
    kappa=2.0,
    m_reg=8,
    grid_shape=(24, 24, 24),
    candidate_k_initial=8,
    candidate_k_max=64,
    neighbor_audit_queries=0,
    require_neighbor_audit=False,
)

hierarchy = PowerCover3D(config, backend="cpu").fit(points)
print(hierarchy.report.to_dict())
```

Cette sortie est une hiérarchie cubique régularisée. Elle ne doit pas être citée comme une reconstruction exacte de la multicouverture HGP continue.

## API publique conservée

- `PowerCover3D`
- `PowerCoverHierarchy`
- `PowerCoverConfig`
- `pilot_kappa_calibration`

Les oracles exhaustifs de petite taille vivent dans `perg_hgp.reference`; les briques CPU/CUDA détaillées restent accessibles sous `perg_hgp.backends.power_cover_3d_cuda`.

## Tests

```bash
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python -m pytest -q perg_hgp/tests
```

Les tests couvrent le chemin CPU, les contrats d'import, les oracles de puissance, la forêt cubique, la couverture des sites et les interfaces CUDA simulables sans GPU.

## Expérience conservée

Le notebook [`experiments/powercover3d/blackwell_30m.ipynb`](../experiments/powercover3d/blackwell_30m.ipynb) est gardé comme protocole historique Blackwell. Il n'est ni un benchmark des futurs backends ni une preuve de leur latence.
