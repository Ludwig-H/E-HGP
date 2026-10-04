# Deux vélos (trame 08/002852) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_002852_deux_velos_6_51_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 13,5 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_002852_deux_velos_6_51_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 39 s, k = 5, 1920 × 1080 : [thème sombre](08_002852_deux_velos_6_51_instances_k5_sombre.mp4) · [thème clair](08_002852_deux_velos_6_51_instances_k5_clair.mp4) ; image finale : [sombre](08_002852_deux_velos_6_51_instances_k5_sombre_bilan.png) · [clair](08_002852_deux_velos_6_51_instances_k5_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 279 points (A 157, B 122) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,85 / 0,83 | 0,74 / **0,44** | HGP réussit, HDBSCAN échoue |
| 10 | 0,85 / **0,50** | 0,73 / **0,44** | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 5)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 6,2 cm |  | ✓ A retrouvé · IoU 0,503 |
| 7,6 cm |  | ✗ A et B réunis : B jamais retrouvé |
| 10,3 cm | ✓ A retrouvé · IoU 0,503 |  |
| 13,5 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 15,8 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

