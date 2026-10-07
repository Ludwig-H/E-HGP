# Un piéton et un vélo (trame 00/002140)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 00, trame 002140. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 160 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 1 221 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | piéton | 82 | 78 | 4 |
| B | vélo | 78 | 69 | 9 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,10 m. Dans la découpe sans sol : aucune autre instance ; 0 points des classes 0, 1, 52 et 99 (non étiqueté, aberrant, autre structure, autre objet) ; 628 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,94 / 0,91 | 0,90 / 0,78 | les deux réussissent |
| instances de la vérité terrain seules | 10 | 0,94 / 0,87 | 0,76 / **0,49** | HGP réussit, HDBSCAN échoue |
| sol retiré automatiquement (Patchwork++) | 5 | 0,95 / 0,93 | 0,95 / 0,87 | les deux réussissent |
| sol retiré automatiquement (Patchwork++) | 10 | 0,94 / 0,79 | 0,83 / 0,61 | les deux réussissent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 10 (instances) ; k = 5 (sans sol). Objets : instances SemanticKITTI A = 1, B = 6.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 10 : [sombre](instances/00_002140_pieton_velo_1_6_instances_k10_sombre.mp4) · [clair](instances/00_002140_pieton_velo_1_6_instances_k10_clair.mp4) ; supports : [sombre](instances/00_002140_pieton_velo_1_6_instances_k10_supports_sombre.mp4) · [clair](instances/00_002140_pieton_velo_1_6_instances_k10_supports_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_sombre.mp4) · [clair](sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_clair.mp4) ; supports : [sombre](sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_supports_sombre.mp4) · [clair](sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_supports_clair.mp4) |

**Instances de la vérité terrain seules**, k = 10 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/00_002140_pieton_velo_1_6_instances_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 17,2 cm : HGP, A, IoU maximal : 0,94 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, A et B déjà réunis" src="instances/00_002140_pieton_velo_1_6_instances_k10_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 12,2 cm : HGP, B, IoU maximal : 0,93 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,64  ·  B 0,75" src="sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
