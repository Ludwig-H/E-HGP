# Deux vélos (trame 08/001170) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 14,4 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_001170_deux_velos_43_57_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 40 s, k = 5, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_instances_k5_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_instances_k5_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_instances_k5_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_instances_k5_clair_bilan.png).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_001170_deux_velos_43_57_instances_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 20,1 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_001170_deux_velos_43_57_instances_k10_clair_instant_cle.png">
</picture>

Vidéo de 38 s, k = 10, 1920 × 1080 : [thème sombre](08_001170_deux_velos_43_57_instances_k10_sombre.mp4) · [thème clair](08_001170_deux_velos_43_57_instances_k10_clair.mp4) ; image finale : [sombre](08_001170_deux_velos_43_57_instances_k10_sombre_bilan.png) · [clair](08_001170_deux_velos_43_57_instances_k10_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 241 points (A 141, B 100) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,94 / 0,65 | 0,70 / **0,43** | HGP réussit, HDBSCAN échoue |
| 10 | 0,66 / 0,59 | 0,61 / **0,41** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 8,1 cm |  | ✓ A retrouvé · IoU 0,504 |
| 8,7 cm | A et B encore séparés | ✗ A et B réunis : B jamais retrouvé |
| 14,4 cm | ✓ B retrouvé · IoU 0,51 |  |
| 14,4 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 16,2 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Événements de la vidéo à k = 10

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 11,3 cm |  | ✗ A et B réunis : B jamais retrouvé |
| 18,4 cm | ✓ A retrouvé · IoU 0,504 |  |
| 20,1 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 22,9 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

