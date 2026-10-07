# Un piéton et deux vélos (trame 06/000800)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 06, trame 000800. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 304 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 1 674 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | piéton | 117 | 117 | 0 |
| B | vélo | 86 | 64 | 22 |
| C | vélo | 101 | 80 | 21 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,15 m, A–C : 0,57 m, B–C : 0,04 m. Dans la découpe sans sol : 1 autre instance (42 points) ; 187 points des classes 0, 1, 52 et 99 (non étiqueté, aberrant, autre structure, autre objet) ; 699 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,99 / 0,64 / 0,69 | 0,99 / **0,46** / 0,56 | HGP réussit, HDBSCAN échoue |
| instances de la vérité terrain seules | 10 | 0,99 / 0,64 / 0,70 | 0,98 / 0,50 / **0,43** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 5 | 0,97 / 0,74 / 0,73 | 0,96 / **0,43** / 0,70 | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 10 | 0,99 / 0,74 / 0,74 | 0,97 / **0,44** / **0,50** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 et k = 10 (instances) ; k = 5 et k = 10 (sans sol). Objets : instances SemanticKITTI A = 2, B = 8, C = 12.

## Même scène

Autres groupes de la recherche qui partagent une instance avec celui-ci (même rangée, trames voisines) ; un seul exemple est montré par scène.

| groupe | trame | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 |
| --- | --- | --- | --- |
| 12, 13 | 06/000800 | deux réussites / deux réussites | deux réussites / **gain HGP** |
| 8, 12 | 06/000800 | **gain HGP** / deux réussites | **gain HGP** / **gain HGP** |
| 8, 12, 13 | 06/000800 | **gain HGP** / deux réussites | **gain HGP** / **gain HGP** |
| 2, 12 | 06/000800 | deux réussites / deux réussites | deux réussites / **gain HGP** |
| 2, 12, 13 | 06/000800 | deux réussites / deux réussites | deux réussites / **gain HGP** |
| 2, 8 | 06/000800 | deux réussites / deux réussites | **gain HGP** / **gain HGP** |
| 2, 8, 13 | 06/000800 | deux réussites / deux réussites | **gain HGP** / **gain HGP** |

Les groupes sont nommés par les numéros d'instance SemanticKITTI de leurs objets.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 5 : [sombre](instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre.mp4) · [clair](instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_clair.mp4) ; supports : [sombre](instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_supports_sombre.mp4) · [clair](instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_supports_clair.mp4) ; k = 10 : [sombre](instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_sombre.mp4) · [clair](instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_clair.mp4) ; supports : [sombre](instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_supports_sombre.mp4) · [clair](instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_supports_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_sombre.mp4) · [clair](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_clair.mp4) ; supports : [sombre](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_supports_sombre.mp4) · [clair](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_supports_clair.mp4) ; k = 10 : [sombre](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_sombre.mp4) · [clair](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_clair.mp4) ; supports : [sombre](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_supports_sombre.mp4) · [clair](sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_supports_clair.mp4) |

**Instances de la vérité terrain seules**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 17,6 cm : HGP, A, B et C retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,93  ·  B 0,29  ·  C 0,45" src="instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,8 cm : HGP, B, IoU maximal : 0,74 ; ✓ A, B et C retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,91  ·  B 0,23  ·  C 0,49" src="sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
