# Trois vélos (trame 00/001472) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001472_trois_velos_40_42_59_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,7 cm : HGP, B et C retrouvés, encore séparés ; HDBSCAN, B et C déjà réunis" src="00_001472_trois_velos_40_42_59_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 47 s, k = 5, 1920 × 1080 : [thème sombre](00_001472_trois_velos_40_42_59_instances_k5_sombre.mp4) · [thème clair](00_001472_trois_velos_40_42_59_instances_k5_clair.mp4) ; image finale : [sombre](00_001472_trois_velos_40_42_59_instances_k5_sombre_bilan.png) · [clair](00_001472_trois_velos_40_42_59_instances_k5_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 382 points (A 82, B 139, C 161) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,84 / 0,72 / 0,60 | 0,84 / **0,41** / 0,57 | HGP réussit, HDBSCAN échoue |
| 10 | 0,84 / 0,76 / 0,57 | 0,84 / **0,43** / 0,57 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 5)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 5,4 cm |  | ✗ B et C réunis : B jamais retrouvé |
| 8,1 cm |  | ✓ A retrouvé · IoU 0,51 |
| 9,5 cm | ✓ B retrouvé · IoU 0,51 |  |
| 10,7 cm | ✓ B et C retrouvés, encore séparés | ✗ B et C déjà réunis |
| 10,8 cm | ✓ B et C réunis, chacun retrouvé avant |  |
| 13,5 cm | ✓ A retrouvé · IoU 0,59 |  |
| 43,4 cm |  | ✗ A, B et C réunis : B jamais retrouvé |
| 45,7 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

