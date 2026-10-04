# Deux vélos (trame 08/001170) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,0 cm : HGP, B, IoU maximal : 0,72 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_001170_deux_velos_43_57_sans_sol_k5_clair_instant_cle.png">
</picture>

Vidéo de 47 s, k = 5, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k5_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k5_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k5_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k5_clair_bilan.png).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 21,2 cm : HGP, B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_001170_deux_velos_43_57_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 50 s, k = 10, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_sans_sol_k10_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_sans_sol_k10_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_sans_sol_k10_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_sans_sol_k10_clair_bilan.png).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 392 points, dont 229 des objets (A 135, B 94) ; 12 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,93 / 0,72 | 0,71 / **0,44** | HGP réussit, HDBSCAN échoue |
| 10 | 0,65 / 0,63 | 0,55 / **0,38** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 8,7 cm |  | ✓ A, IoU maximal : 0,71 |
| 8,7 cm | A et B encore séparés | ✗ A et B réunis : B jamais retrouvé |
| 10,1 cm |  | ✗ A fusionne avec un autre vélo · IoU 0,56 → 0,37 |
| 15,9 cm | ✓ A, IoU maximal : 0,93 |  |
| 16,0 cm | ✓ B, IoU maximal : 0,72 ; ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 16,2 cm | ✓ A et B réunis, chacun retrouvé avant |  |
| 18,7 cm | ✗ A fusionne avec un autre vélo · IoU 0,61 → 0,38 |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Événements de la vidéo à k = 10

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 11,3 cm |  | ✗ A et B réunis : B jamais retrouvé |
| 12,6 cm |  | ✓ A, IoU maximal : 0,55 |
| 12,7 cm |  | ✗ A fusionne avec un autre vélo · IoU 0,55 → 0,37 |
| 21,2 cm | ✓ B, IoU maximal : 0,63 ; ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 21,2 cm | ✗ B fusionne avec une partie de A · IoU 0,63 → 0,42 |  |
| 22,9 cm | ✓ A et B réunis, chacun retrouvé avant |  |
| 23,0 cm | ✓ A, IoU maximal : 0,65 |  |
| 23,6 cm | ✗ A fusionne avec un autre vélo · IoU 0,65 → 0,40 |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

