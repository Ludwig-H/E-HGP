# Deux vélos (trame 08/001170) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,0 cm : HGP, B, IoU maximal : 0,72 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,51  ·  B 0,33" src="08_001170_deux_velos_43_57_sans_sol_k5_clair_instant_cle.png">
</picture>

Vidéo de 67 s, k = 5, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k5_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k5_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k5_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k5_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k5_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 5, r = 16,0 cm : B, IoU maximal : 0,70 ; ✓ A et B retrouvés, encore séparés" src="08_001170_deux_velos_43_57_sans_sol_k5_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 41 s : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k5_supports_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k5_supports_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k5_supports_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k5_supports_clair_bilan.png). Arbre couvrant d'ordre 5 : 6 074 nœuds, 6 074 naissances et fusions, chacune avec son support S\* (6 074 supports : 950 arêtes q2, 3 549 triangles q3, 1 575 tétraèdres q4).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 21,2 cm : HGP, B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,27  ·  B 0,32" src="08_001170_deux_velos_43_57_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 71 s, k = 10, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k10_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k10_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k10_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k10_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k10_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 10, r = 20,5 cm : B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés" src="08_001170_deux_velos_43_57_sans_sol_k10_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 42 s : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k10_supports_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k10_supports_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k10_supports_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k10_supports_clair_bilan.png). Arbre couvrant d'ordre 10 : 16 655 nœuds, 16 655 naissances et fusions, chacune avec son support S\* (16 655 supports : 860 arêtes q2, 6 881 triangles q3, 8 914 tétraèdres q4).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 392 points, dont 229 des objets (A 135, B 94) ; 12 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,93 / 0,72 | 0,71 / **0,44** | HGP réussit, HDBSCAN échoue |
| 10 | 0,65 / 0,63 | 0,55 / **0,38** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 17,3 cm | ✓ A, IoU maximal : 0,71 |
| 17,4 cm | ✗ A et B réunis : B jamais retrouvé |
| 20,1 cm | ✗ A fusionne avec un autre vélo · IoU 0,56 → 0,37 |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 15,9 cm | ✓ A, IoU maximal : 0,93 | IoU au même r : A 0,51 |
| 16,0 cm | ✓ B, IoU maximal : 0,72 ; ✓ A et B retrouvés, encore séparés | IoU au même r : A 0,51  ·  B 0,33 |
| 16,2 cm | ✓ A et B réunis, chacun retrouvé avant |  |
| 18,7 cm | ✗ A fusionne avec un autre vélo · IoU 0,61 → 0,38 |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Hiérarchie des supports à k = 5

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,91 / 0,70 (hiérarchie de points HGP : 0,93 / 0,72). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 15,5 cm | ✓ A, IoU maximal : 0,91 |
| 16,0 cm | ✓ B, IoU maximal : 0,70 ; ✓ A et B retrouvés, encore séparés |
| 16,2 cm | ✓ A et B réunis, chacun retrouvé avant |
| 18,7 cm | ✗ A fusionne avec un autre vélo · IoU 0,60 → 0,38 |

Nombres : [`resultats_supports_k5.json`](resultats_supports_k5.json).

## Événements de la vidéo à k = 10

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 22,7 cm | ✗ A et B réunis : B jamais retrouvé |
| 25,3 cm | ✓ A, IoU maximal : 0,55 |
| 25,3 cm | ✗ A fusionne avec un autre vélo · IoU 0,55 → 0,37 |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 21,2 cm | ✓ B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés | IoU au même r : A 0,27  ·  B 0,32 |
| 21,2 cm | ✗ B fusionne avec une partie de A · IoU 0,63 → 0,42 |  |
| 22,9 cm | ✓ A et B réunis, chacun retrouvé avant |  |
| 23,0 cm | ✓ A, IoU maximal : 0,65 | ✗ A et B déjà réunis |
| 23,6 cm | ✗ A fusionne avec un autre vélo · IoU 0,65 → 0,40 |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Hiérarchie des supports à k = 10

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,62 / 0,63 (hiérarchie de points HGP : 0,65 / 0,63). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 20,5 cm | ✓ B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés |
| 21,2 cm | ✗ B fusionne avec une partie de A · IoU 0,59 → 0,41 |
| 22,9 cm | ✓ A, IoU maximal : 0,62 ; ✓ A et B réunis, chacun retrouvé avant |
| 23,6 cm | ✗ A fusionne avec un autre vélo · IoU 0,62 → 0,39 |

Nombres : [`resultats_supports_k10.json`](resultats_supports_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

