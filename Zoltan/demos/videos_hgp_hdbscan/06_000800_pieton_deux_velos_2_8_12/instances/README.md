# Un piéton et deux vélos (trame 06/000800) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 17,6 cm : HGP, A, B et C retrouvés, encore séparés ; HDBSCAN, A, B et C déjà réunis" src="06_000800_pieton_deux_velos_2_8_12_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 52 s, k = 5, 1920 × 1080 : [thème sombre](06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre.mp4) · [thème clair](06_000800_pieton_deux_velos_2_8_12_instances_k5_clair.mp4) ; image finale : [sombre](06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre_bilan.png) · [clair](06_000800_pieton_deux_velos_2_8_12_instances_k5_clair_bilan.png).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="06_000800_pieton_deux_velos_2_8_12_instances_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 23,4 cm : HGP, C, IoU maximal : 0,70 ; ✓ A, B et C retrouvés, encore séparés ; HDBSCAN, A, B et C déjà réunis" src="06_000800_pieton_deux_velos_2_8_12_instances_k10_clair_instant_cle.png">
</picture>

Vidéo de 50 s, k = 10, 1920 × 1080 : [thème sombre](06_000800_pieton_deux_velos_2_8_12_instances_k10_sombre.mp4) · [thème clair](06_000800_pieton_deux_velos_2_8_12_instances_k10_clair.mp4) ; image finale : [sombre](06_000800_pieton_deux_velos_2_8_12_instances_k10_sombre_bilan.png) · [clair](06_000800_pieton_deux_velos_2_8_12_instances_k10_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 304 points (A 117, B 86, C 101) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,99 / 0,64 / 0,69 | 0,99 / **0,46** / 0,56 | HGP réussit, HDBSCAN échoue |
| 10 | 0,99 / 0,64 / 0,70 | 0,98 / 0,50 / **0,43** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 10,7 cm | B et C encore séparés | ✗ B et C réunis : B jamais retrouvé |
| 13,3 cm |  | ✓ A, IoU maximal : 0,99 |
| 13,9 cm |  | ✓ C, IoU maximal : 0,56 |
| 15,2 cm | A, B et C encore séparés | ✗ A, B et C réunis : B jamais retrouvé |
| 15,6 cm | ✓ A, IoU maximal : 0,99 |  |
| 16,4 cm | ✓ B, IoU maximal : 0,64 |  |
| 17,6 cm | ✓ A, B et C retrouvés, encore séparés | ✗ A, B et C déjà réunis |
| 18,8 cm | ✓ C, IoU maximal : 0,69 |  |
| 18,9 cm | ✓ B et C réunis, chacun retrouvé avant |  |
| 24,6 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Événements de la vidéo à k = 10

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 12,0 cm |  | ✗ B et C réunis : C jamais retrouvé |
| 15,0 cm |  | ✓ B, IoU maximal : 0,503 |
| 15,1 cm |  | ✓ A, IoU maximal : 0,98 |
| 16,1 cm | A, B et C encore séparés | ✗ A, B et C réunis : C jamais retrouvé |
| 19,3 cm | ✓ A, IoU maximal : 0,99 |  |
| 23,2 cm | ✓ B, IoU maximal : 0,64 |  |
| 23,4 cm | ✓ C, IoU maximal : 0,70 ; ✓ A, B et C retrouvés, encore séparés | ✗ A, B et C déjà réunis |
| 23,9 cm | ✓ B et C réunis, chacun retrouvé avant |  |
| 29,4 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

