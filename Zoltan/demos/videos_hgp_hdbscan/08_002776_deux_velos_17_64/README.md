# Deux vélos (trame 08/002776)

Exemple vidéo HGP contre HDBSCAN ([liste](../README.md)) : SemanticKITTI, séquence 08, trame 002776. Deux variantes, chacune dans son sous-dossier :

- [`instances/`](instances/README.md) : les seuls points des objets (vérité terrain), 293 points ;
- [`sans_sol/`](sans_sol/README.md) : tout ce que Patchwork++ ne classe pas en sol dans la boîte des objets élargie de 1 m, 6 141 points (aucune étiquette ne sert au nettoyage).

| objet | classe | points (instances) | points gardés sans sol | points retirés comme sol |
| --- | --- | --- | --- | --- |
| A | vélo | 198 | 197 | 1 |
| B | vélo | 95 | 86 | 9 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,13 m. Dans la découpe sans sol : aucune autre instance ; 2247 points void ; 1582 points de sol retirés.

| variante | k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- | --- |
| instances de la vérité terrain seules | 5 | 0,70 / 0,53 | 0,75 / **0,36** | HGP réussit, HDBSCAN échoue |
| instances de la vérité terrain seules | 10 | 0,70 / **0,49** | 0,70 / **0,45** | les deux échouent |
| sol retiré automatiquement (Patchwork++) | 5 | **0,23** / **0,46** | **0,20** / **0,36** | les deux échouent |
| sol retiré automatiquement (Patchwork++) | 10 | **0,25** / **0,42** | **0,19** / **0,38** | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Vidéos : k = 5 (instances) ; k = 5 (sans sol). Objets : instances SemanticKITTI A = 17, B = 64.

<!-- video:début -->
## Vidéos

| | instances de la vérité terrain seules | sol retiré automatiquement (Patchwork++) |
| --- | --- | --- |
| vidéos | [README](instances/README.md) · k = 5 : [sombre](instances/08_002776_deux_velos_17_64_instances_k5_sombre.mp4) · [clair](instances/08_002776_deux_velos_17_64_instances_k5_clair.mp4) | [README](sans_sol/README.md) · k = 5 : [sombre](sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_sombre.mp4) · [clair](sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_clair.mp4) |

**Instances de la vérité terrain seules**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="instances/08_002776_deux_velos_17_64_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,0 cm : HGP, B, IoU maximal : 0,53 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,56  ·  B 0,28" src="instances/08_002776_deux_velos_17_64_instances_k5_clair_instant_cle.png">
</picture>

**Sol retiré automatiquement (Patchwork++)**, k = 5 :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 11,3 cm : HGP, A et B réunis : A et B jamais retrouvés ; HDBSCAN au même r, A et B encore séparés" src="sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_clair_instant_cle.png">
</picture>

<!-- video:fin -->
