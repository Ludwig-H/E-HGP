# Deux vélos (trame 08/002852) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_002852_deux_velos_6_51_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 15,0 cm : HGP, B, IoU maximal : 0,81 ; HDBSCAN au même r, IoU au même r : B 0,44" src="08_002852_deux_velos_6_51_sans_sol_k5_clair_instant_cle.png">
</picture>

Vidéo de 69 s, k = 5, 1920 × 1080 : [thème sombre](08_002852_deux_velos_6_51_sans_sol_k5_sombre.mp4) · [thème clair](08_002852_deux_velos_6_51_sans_sol_k5_clair.mp4) ; image finale : [sombre](08_002852_deux_velos_6_51_sans_sol_k5_sombre_bilan.png) · [clair](08_002852_deux_velos_6_51_sans_sol_k5_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_002852_deux_velos_6_51_sans_sol_k5_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 5, r = 12,9 cm : A, IoU maximal : 0,77" src="08_002852_deux_velos_6_51_sans_sol_k5_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 43 s : [thème sombre](08_002852_deux_velos_6_51_sans_sol_k5_supports_sombre.mp4) · [thème clair](08_002852_deux_velos_6_51_sans_sol_k5_supports_clair.mp4) ; image finale : [sombre](08_002852_deux_velos_6_51_sans_sol_k5_supports_sombre_bilan.png) · [clair](08_002852_deux_velos_6_51_sans_sol_k5_supports_clair_bilan.png). Arbre couvrant d'ordre 5 : 13 663 nœuds, 13 664 naissances et fusions, chacune avec son support S\* (13 664 supports : 2 798 arêtes q2, 8 955 triangles q3, 1 911 tétraèdres q4).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_002852_deux_velos_6_51_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 19,2 cm : HGP, B, IoU maximal : 0,504 ; HDBSCAN au même r, IoU au même r : B 0,29" src="08_002852_deux_velos_6_51_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 66 s, k = 10, 1920 × 1080 : [thème sombre](08_002852_deux_velos_6_51_sans_sol_k10_sombre.mp4) · [thème clair](08_002852_deux_velos_6_51_sans_sol_k10_clair.mp4) ; image finale : [sombre](08_002852_deux_velos_6_51_sans_sol_k10_sombre_bilan.png) · [clair](08_002852_deux_velos_6_51_sans_sol_k10_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_002852_deux_velos_6_51_sans_sol_k10_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 10, r = 17,0 cm : A, IoU maximal : 0,64" src="08_002852_deux_velos_6_51_sans_sol_k10_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 40 s : [thème sombre](08_002852_deux_velos_6_51_sans_sol_k10_supports_sombre.mp4) · [thème clair](08_002852_deux_velos_6_51_sans_sol_k10_supports_clair.mp4) ; image finale : [sombre](08_002852_deux_velos_6_51_sans_sol_k10_supports_sombre_bilan.png) · [clair](08_002852_deux_velos_6_51_sans_sol_k10_supports_clair_bilan.png). Arbre couvrant d'ordre 10 : 29 382 nœuds, 29 382 naissances et fusions, chacune avec son support S\* (29 382 supports : 2 528 arêtes q2, 17 063 triangles q3, 9 791 tétraèdres q4).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 1 283 points, dont 267 des objets (A 148, B 119) ; 12 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet (meilleur bloc ; seuls les points non étiquetés et aberrants, classes 0 et 1, sont exclus : « autre structure » et « autre objet » comptent comme du fond) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,77 / 0,81 | 0,68 / **0,44** | HGP réussit, HDBSCAN échoue |
| 10 | 0,62 / 0,50 | 0,65 / **0,36** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 14,0 cm | ✓ A, IoU maximal : 0,68 |
| 14,2 cm | ✗ A fusionne avec le bâtiment · IoU 0,68 → 0,14 |
| 15,2 cm | ✗ A et B réunis : B jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 13,1 cm | ✓ A, IoU maximal : 0,77 | IoU au même r : A 0,64 |
| 13,4 cm | ✗ A fusionne avec le bâtiment · IoU 0,77 → 0,16 |  |
| 15,0 cm | ✓ B, IoU maximal : 0,81 | IoU au même r : B 0,44 |
| 15,1 cm | ✗ B fusionne avec le fond · IoU 0,81 → 0,22 |  |
| 15,8 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Hiérarchie des supports à k = 5

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,77 / 0,82 (hiérarchie de points HGP : 0,77 / 0,81). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 12,9 cm | ✓ A, IoU maximal : 0,77 |
| 13,4 cm | ✗ A fusionne avec le bâtiment · IoU 0,77 → 0,16 |
| 15,0 cm | ✓ B, IoU maximal : 0,82 |
| 15,1 cm | ✗ B fusionne avec le fond · IoU 0,82 → 0,23 |
| 15,8 cm | ✓ A et B réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k5.json`](resultats_supports_k5.json).

## Événements de la vidéo à k = 10

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 18,8 cm | ✓ A, IoU maximal : 0,65 |
| 18,9 cm | ✗ A fusionne avec le bâtiment · IoU 0,65 → 0,14 |
| 20,5 cm | ✗ A et B réunis : B jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 17,1 cm | ✓ A, IoU maximal : 0,62 | IoU au même r : A 0,43 |
| 17,3 cm | ✗ A fusionne avec le bâtiment · IoU 0,62 → 0,18 |  |
| 19,2 cm | ✓ B, IoU maximal : 0,504 | IoU au même r : B 0,29 |
| 19,5 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Hiérarchie des supports à k = 10

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,64 / 0,52 (hiérarchie de points HGP : 0,62 / 0,50). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 17,0 cm | ✓ A, IoU maximal : 0,64 |
| 17,3 cm | ✗ A fusionne avec le bâtiment · IoU 0,63 → 0,18 |
| 19,0 cm | ✓ B, IoU maximal : 0,52 |
| 19,5 cm | ✓ A et B réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k10.json`](resultats_supports_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

