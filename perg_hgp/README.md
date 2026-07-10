# PERG-HGP

Ce package contient désormais deux backends nettement séparés :

- `PERGHGPClusterer`, prototype historique d’atlas Rank-Gabriel. Sa connectivité est exacte pour l’atlas numérique produit, mais la complétude de cet atlas vis-à-vis du HGP de puissance reste inconnue.
- `PowerCover3D`, backend spatial 3D qui calcule directement un encadrement de la hiérarchie (H_0) du champ K-ième distance de puissance, conditionnel à son contrat numérique float32 déclaré. C’est le chemin prévu pour (30,000,000) de points et (K=10).

Le nouveau backend effectue la régularisation entropique en streaming, applique un garde top-K de puissance sous une enveloppe float32 explicite à partir de candidats euclidiens globaux, évalue un champ cubique intérieur/extérieur, puis construit un MSF 26-connexe sans matérialiser le graphe des cubes. La sortie primaire est une paire de hiérarchies cubiques, pas une partition plate forcée.

Exemple CPU de référence :

```python
import numpy as np
from perg_hgp import PowerCover3D, PowerCoverConfig

X = np.random.default_rng(0).normal(size=(2_000, 3)).astype("float32")
config = PowerCoverConfig(
    K=10,
    kappa=4.0,      # distinct de K
    m_reg=64,       # support local dur, distinct de K et kappa
    grid_shape=(32, 32, 32),
    candidate_k_initial=64,
    candidate_k_max=512,
    neighbor_audit_queries=0,
    require_neighbor_audit=False,
)

hierarchy = PowerCover3D(config, backend="cpu").fit(X)
print(hierarchy.report.to_dict())
inner_labels, outer_labels = hierarchy.components_at_cut(radius=0.5)
```

Le mode CPU est un oracle pour petites tailles, pas un chemin 30 M. Le mode CUDA exige CuPy, RAPIDS cuML/cuVS et un GPU compatible :

```python
assert X_gpu.shape[0] >= 1_050_625  # 1025 <= floor(sqrt(N)), sans fallback RBC
cuda_config = PowerCoverConfig(
    K=10,
    kappa=4.0,
    m_reg=64,
    grid_shape=(128, 128, 128),
    chunk_size=262_144,
    power_chunk_size=65_536,
    candidate_k_initial=64,
    candidate_k_max=1_024,
    neighbor_audit_queries=32,
    require_neighbor_audit=True,
)
hierarchy = PowerCover3D(cuda_config, backend="cuda").fit(X_gpu)
hierarchy.save("/content/perg_hgp_run")
```

Le notebook [PERG_HGP_Blackwell_30M.ipynb](../PERG_HGP_Blackwell_30M.ipynb) installe la pile RAPIDS officielle, audite Random Ball Cover contre cuVS brute force, compare le MSF GPU à la référence CPU, puis exécute les campagnes jusqu’à 30 M.

Garanties et limites :

- la borne publiée porte sur le paramètre rayon et sur (H_0), jamais directement sur une densité ou des labels plats ;
- le rapport machine-lisible expose séparément la distorsion entropique `s`, la demi-diagonale cubique `H` et la marge numérique `delta_num` ;
- la borne réellement utilisée est `s + 2 * (H + delta_num)` ;
- une requête de puissance qui ne passe pas l'enveloppe déclarée lève une erreur en mode strict ; aucun fallback ANN n’est maquillé en résultat exact ;
- (K=10) fixe est une résolution opérationnelle finie, pas une revendication de consistance asymptotique ;
- l’appartenance canonique d’un site à une composante demande l’intersection de sa boule de puissance avec cette composante. Une simple localisation de `x_i` ou `z_i` dans un cube ne la remplace pas.

Voir [POWER_COVER_3D.md](POWER_COVER_3D.md) pour le contrat détaillé et [Rapport_revue_complete_PERG_HGP_2026-07-10.md](Rapport_revue_complete_PERG_HGP_2026-07-10.md) pour l’audit, les tests et le statut GPU.
