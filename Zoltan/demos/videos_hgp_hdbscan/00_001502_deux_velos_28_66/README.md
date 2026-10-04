# Deux vélos (trame 00/001502)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 00, trame 001502. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 209 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 2 964 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | vélo | 146 | 113 | 33 |
| B | vélo | 63 | 51 | 12 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,03 m. Dans la découpe sans sol : aucune autre instance ; 0 points void ; 205 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,78 / 0,60 | 0,74 / **0,40** | HGP réussit, HDBSCAN échoue |
| instances de la vérité terrain seules | 10 | 0,97 / 0,60 | 0,82 / **0,43** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 5 | 0,58 / 0,51 | **0,47** / **0,37** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 10 | 0,62 / 0,53 | **0,38** / **0,29** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 et k = 10 (instances) ; k = 5 et k = 10 (sans sol). Objets : instances SemanticKITTI A = 28, B = 66.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 5 : [sombre](instances/00_001502_deux_velos_28_66_instances_k5_sombre.mp4) · [clair](instances/00_001502_deux_velos_28_66_instances_k5_clair.mp4) ; k = 10 : [sombre](instances/00_001502_deux_velos_28_66_instances_k10_sombre.mp4) · [clair](instances/00_001502_deux_velos_28_66_instances_k10_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_sombre.mp4) · [clair](sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_clair.mp4) ; k = 10 : [sombre](sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_sombre.mp4) · [clair](sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_clair.mp4) |

**Instances de la vérité terrain seules**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/00_001502_deux_velos_28_66_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 9,5 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="instances/00_001502_deux_velos_28_66_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 7,7 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
