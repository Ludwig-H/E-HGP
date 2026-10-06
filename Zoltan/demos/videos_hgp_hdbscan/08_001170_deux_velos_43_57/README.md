# Deux vélos (trame 08/001170)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 08, trame 001170. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 241 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 392 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | vélo | 141 | 135 | 6 |
| B | vélo | 100 | 94 | 6 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,11 m. Dans la découpe sans sol : 2 autres instances (16, 103 points) ; 1 points void ; 667 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,94 / 0,65 | 0,70 / **0,43** | HGP réussit, HDBSCAN échoue |
| instances de la vérité terrain seules | 10 | 0,66 / 0,59 | 0,61 / **0,41** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 5 | 0,93 / 0,72 | 0,71 / **0,44** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 10 | 0,65 / 0,63 | 0,55 / **0,38** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 et k = 10 (instances) ; k = 5 et k = 10 (sans sol). Objets : instances SemanticKITTI A = 43, B = 57.

## Même scène

Autres groupes de la recherche qui partagent une instance avec celui-ci (même rangée, trames voisines) ; un seul exemple est montré par scène.

| groupe | trame | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 |
| --- | --- | --- | --- |
| 43, 56 | 08/001180 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 43, 56, 57 | 08/001180 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 56, 57 | 08/001180 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 43, 55, 56 | 08/001182 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 43, 56 | 08/001182 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 43, 56, 57 | 08/001182 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 55, 56 | 08/001182 | deux réussites / deux réussites | perte HGP / **gain HGP** |
| 55, 56, 57 | 08/001182 | deux réussites / **gain HGP** | perte HGP / **gain HGP** |
| 56, 57 | 08/001182 | deux réussites / deux réussites | perte HGP / **gain HGP** |

Les groupes sont nommés par les numéros d'instance SemanticKITTI de leurs objets.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 5 : [sombre](instances/08_001170_deux_velos_43_57_instances_k5_sombre.mp4) · [clair](instances/08_001170_deux_velos_43_57_instances_k5_clair.mp4) ; supports : [sombre](instances/08_001170_deux_velos_43_57_instances_k5_supports_sombre.mp4) · [clair](instances/08_001170_deux_velos_43_57_instances_k5_supports_clair.mp4) ; k = 10 : [sombre](instances/08_001170_deux_velos_43_57_instances_k10_sombre.mp4) · [clair](instances/08_001170_deux_velos_43_57_instances_k10_clair.mp4) ; supports : [sombre](instances/08_001170_deux_velos_43_57_instances_k10_supports_sombre.mp4) · [clair](instances/08_001170_deux_velos_43_57_instances_k10_supports_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_sombre.mp4) · [clair](sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_clair.mp4) ; supports : [sombre](sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_supports_sombre.mp4) · [clair](sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_supports_clair.mp4) ; k = 10 : [sombre](sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_sombre.mp4) · [clair](sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_clair.mp4) ; supports : [sombre](sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_supports_sombre.mp4) · [clair](sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_supports_clair.mp4) |

**Instances de la vérité terrain seules**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/08_001170_deux_velos_43_57_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,1 cm : HGP, B, IoU maximal : 0,65 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,49  ·  B 0,32" src="instances/08_001170_deux_velos_43_57_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,0 cm : HGP, B, IoU maximal : 0,72 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,51  ·  B 0,33" src="sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
