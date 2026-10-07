# Deux vélos (trame 00/001300)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 00, trame 001300. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 296 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 3 527 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | vélo | 56 | 36 | 20 |
| B | vélo | 240 | 149 | 91 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,02 m. Dans la découpe sans sol : 1 autre instance (865 points) ; 4 points des classes 0, 1, 52 et 99 (non étiqueté, aberrant, autre structure, autre objet) ; 2091 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | **0,36** / 0,87 | **0,36** / 0,95 | les deux échouent |
| instances de la vérité terrain seules | 10 | **0,36** / 0,83 | **0,29** / 0,84 | les deux échouent |
| sol retiré automatiquement (Patchwork++) | 5 | 0,56 / 0,72 | 0,56 / 0,73 | les deux réussissent |
| sol retiré automatiquement (Patchwork++) | 10 | 0,56 / 0,75 | **0,44** / 0,73 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 (instances) ; k = 10 (sans sol). Objets : instances SemanticKITTI A = 53, B = 67.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 5 : [sombre](instances/00_001300_deux_velos_53_67_instances_k5_sombre.mp4) · [clair](instances/00_001300_deux_velos_53_67_instances_k5_clair.mp4) ; supports : [sombre](instances/00_001300_deux_velos_53_67_instances_k5_supports_sombre.mp4) · [clair](instances/00_001300_deux_velos_53_67_instances_k5_supports_clair.mp4) | [README](sans_sol/README.md) · k = 10 : [sombre](sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_sombre.mp4) · [clair](sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_clair.mp4) ; supports : [sombre](sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_supports_sombre.mp4) · [clair](sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_supports_clair.mp4) |

**Instances de la vérité terrain seules**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/00_001300_deux_velos_53_67_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 15,7 cm : HGP, A et B réunis : A jamais retrouvé ; HDBSCAN au même r, A et B encore séparés" src="instances/00_001300_deux_velos_53_67_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 10 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 17,1 cm : HGP, B, IoU maximal : 0,75 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,28  ·  B 0,30" src="sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_clair_instant_cle.png">
</picture>

<!-- video:fin -->
