# Trois vélos (trame 00/001472)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 00, trame 001472. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 382 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 4 943 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | vélo | 82 | 80 | 2 |
| B | vélo | 139 | 126 | 13 |
| C | vélo | 161 | 155 | 6 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,36 m, B–C : 0,07 m. Dans la découpe sans sol : aucune autre instance ; 0 points void ; 284 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,84 / 0,72 / 0,60 | 0,84 / **0,41** / 0,57 | HGP réussit, HDBSCAN échoue |
| instances de la vérité terrain seules | 10 | 0,84 / 0,76 / 0,57 | 0,84 / **0,43** / 0,57 | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 5 | **0,25** / 0,69 / 0,57 | **0,23** / **0,38** / **0,23** | les deux échouent |
| sol retiré automatiquement (Patchwork++) | 10 | **0,19** / 0,51 / **0,26** | **0,11** / **0,21** / **0,17** | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 (instances), k = 5 (sans sol). Objets : instances SemanticKITTI A = 40, B = 42, C = 59.

## Même scène

Autres groupes de la recherche qui partagent une instance avec celui-ci (même rangée, trames voisines) ; un seul exemple est montré par scène.

| groupe | trame | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 |
| --- | --- | --- | --- |
| 42, 59 | 00/001466 | deux échecs / **gain HGP** | deux échecs / deux échecs |

Les groupes sont nommés par les numéros d'instance SemanticKITTI de leurs objets.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéo | [README](instances/README.md) · k = 5 : [sombre](instances/00_001472_trois_velos_40_42_59_instances_k5_sombre.mp4) · [clair](instances/00_001472_trois_velos_40_42_59_instances_k5_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_sombre.mp4) · [clair](sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_clair.mp4) |

**Instances de la vérité terrain seules** :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/00_001472_trois_velos_40_42_59_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,7 cm : HGP, B et C retrouvés, encore séparés ; HDBSCAN, B et C déjà réunis" src="instances/00_001472_trois_velos_40_42_59_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)** :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,7 cm : HGP, B et C retrouvés, encore séparés ; HDBSCAN, B et C déjà réunis" src="sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
