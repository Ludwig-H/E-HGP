# Deux vélos (trame 08/000656) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 17,1 cm : HGP, A, IoU maximal : 0,85 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,37  ·  B 0,71" src="08_000656_deux_velos_37_61_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 67 s, k = 10, 1920 × 1080 : [thème sombre](08_000656_deux_velos_37_61_sans_sol_k10_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_sans_sol_k10_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_sans_sol_k10_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_sans_sol_k10_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_sans_sol_k10_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 10, r = 17,4 cm : A, IoU maximal : 0,86 ; ✓ A et B retrouvés, encore séparés" src="08_000656_deux_velos_37_61_sans_sol_k10_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 41 s : [thème sombre](08_000656_deux_velos_37_61_sans_sol_k10_supports_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_sans_sol_k10_supports_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_sans_sol_k10_supports_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_sans_sol_k10_supports_clair_bilan.png). Arbre d'ordre 10 : 281 751 nœuds, 364 397 boules, 364 400 supports (27 034 arêtes q2, 178 384 triangles q3, 158 982 tétraèdres q4).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 5 863 points, dont 269 des objets (A 52, B 217) ; 22 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,51 / 0,96 | 0,80 / 0,96 | les deux réussissent |
| 10 | 0,85 / 0,97 | **0,37** / 0,97 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 10

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 17,3 cm | ✗ A fusionne avec la végétation · IoU 0,37 → 0,05 |
| 27,1 cm | ✓ B, IoU maximal : 0,97 |
| 28,7 cm | ✗ A et B réunis : A jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 17,1 cm | ✓ A, IoU maximal : 0,85 ; ✓ A et B retrouvés, encore séparés | IoU au même r : A 0,37  ·  B 0,71 |
| 17,8 cm | ✗ A fusionne avec la végétation · IoU 0,85 → 0,04 |  |
| 21,0 cm | ✓ B, IoU maximal : 0,97 | IoU au même r : B 0,91 |
| 21,3 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Hiérarchie des supports à k = 10

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,86 / 0,98 (hiérarchie de points HGP : 0,85 / 0,97). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 17,4 cm | ✓ A, IoU maximal : 0,86 ; ✓ A et B retrouvés, encore séparés |
| 17,8 cm | ✗ A fusionne avec la végétation · IoU 0,86 → 0,04 |
| 19,7 cm | ✓ B, IoU maximal : 0,98 |
| 21,3 cm | ✓ A et B réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k10.json`](resultats_supports_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

