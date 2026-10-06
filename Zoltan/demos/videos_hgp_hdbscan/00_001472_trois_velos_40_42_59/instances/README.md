# Trois vélos (trame 00/001472) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001472_trois_velos_40_42_59_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,8 cm : HGP, C, IoU maximal : 0,60 ; ✓ B et C retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : B 0,39  ·  C 0,22" src="00_001472_trois_velos_40_42_59_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 71 s, k = 5, 1920 × 1080 : [thème sombre](00_001472_trois_velos_40_42_59_instances_k5_sombre.mp4) · [thème clair](00_001472_trois_velos_40_42_59_instances_k5_clair.mp4) ; image finale : [sombre](00_001472_trois_velos_40_42_59_instances_k5_sombre_bilan.png) · [clair](00_001472_trois_velos_40_42_59_instances_k5_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001472_trois_velos_40_42_59_instances_k5_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 5, r = 10,7 cm : B, IoU maximal : 0,71 ; ✓ B et C retrouvés, encore séparés" src="00_001472_trois_velos_40_42_59_instances_k5_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 44 s : [thème sombre](00_001472_trois_velos_40_42_59_instances_k5_supports_sombre.mp4) · [thème clair](00_001472_trois_velos_40_42_59_instances_k5_supports_clair.mp4) ; image finale : [sombre](00_001472_trois_velos_40_42_59_instances_k5_supports_sombre_bilan.png) · [clair](00_001472_trois_velos_40_42_59_instances_k5_supports_clair_bilan.png). Arbre d'ordre 5 : 5 013 nœuds, 6 849 boules, 6 849 supports (1 511 arêtes q2, 4 025 triangles q3, 1 313 tétraèdres q4).

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001472_trois_velos_40_42_59_instances_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 14,2 cm : HGP, B, IoU maximal : 0,76 ; HDBSCAN au même r, IoU au même r : B 0,27" src="00_001472_trois_velos_40_42_59_instances_k10_clair_instant_cle.png">
</picture>

Vidéo de 71 s, k = 10, 1920 × 1080 : [thème sombre](00_001472_trois_velos_40_42_59_instances_k10_sombre.mp4) · [thème clair](00_001472_trois_velos_40_42_59_instances_k10_clair.mp4) ; image finale : [sombre](00_001472_trois_velos_40_42_59_instances_k10_sombre_bilan.png) · [clair](00_001472_trois_velos_40_42_59_instances_k10_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001472_trois_velos_40_42_59_instances_k10_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 10, r = 14,4 cm : B, IoU maximal : 0,72" src="00_001472_trois_velos_40_42_59_instances_k10_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 43 s : [thème sombre](00_001472_trois_velos_40_42_59_instances_k10_supports_sombre.mp4) · [thème clair](00_001472_trois_velos_40_42_59_instances_k10_supports_clair.mp4) ; image finale : [sombre](00_001472_trois_velos_40_42_59_instances_k10_supports_sombre_bilan.png) · [clair](00_001472_trois_velos_40_42_59_instances_k10_supports_clair_bilan.png). Arbre d'ordre 10 : 12 246 nœuds, 15 708 boules, 15 708 supports (1 174 arêtes q2, 7 580 triangles q3, 6 954 tétraèdres q4).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 382 points (A 82, B 139, C 161) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,84 / 0,72 / 0,60 | 0,84 / **0,41** / 0,57 | HGP réussit, HDBSCAN échoue |
| 10 | 0,84 / 0,76 / 0,57 | 0,84 / **0,43** / 0,57 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 10,9 cm | ✗ B et C réunis : B jamais retrouvé |
| 33,0 cm | ✓ C, IoU maximal : 0,57 |
| 36,2 cm | ✓ A, IoU maximal : 0,84 |
| 86,8 cm | ✗ A, B et C réunis : B jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 10,6 cm | ✓ B, IoU maximal : 0,72 | IoU au même r : B 0,37 |
| 10,8 cm | ✓ C, IoU maximal : 0,60 ; ✓ B et C retrouvés, encore séparés | IoU au même r : B 0,39  ·  C 0,22 |
| 10,8 cm | ✓ B et C réunis, chacun retrouvé avant |  |
| 26,9 cm | ✓ A, IoU maximal : 0,84 | IoU au même r : A 0,73 |
| 45,7 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Hiérarchie des supports à k = 5

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,84 / 0,71 / 0,64 (hiérarchie de points HGP : 0,84 / 0,72 / 0,60). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 10,7 cm | ✓ C, IoU maximal : 0,64 |
| 10,7 cm | ✓ B, IoU maximal : 0,71 ; ✓ B et C retrouvés, encore séparés |
| 10,8 cm | ✓ B et C réunis, chacun retrouvé avant |
| 26,9 cm | ✓ A, IoU maximal : 0,84 |
| 45,7 cm | ✓ A, B et C réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k5.json`](resultats_supports_k5.json).

## Événements de la vidéo à k = 10

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 14,9 cm | ✗ B et C réunis : B jamais retrouvé |
| 36,2 cm | ✓ A, IoU maximal : 0,84 |
| 50,4 cm | ✓ C, IoU maximal : 0,57 |
| 86,8 cm | ✗ A, B et C réunis : B jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 14,2 cm | ✓ B, IoU maximal : 0,76 | IoU au même r : B 0,27 |
| 14,4 cm | ✗ B et C réunis : C pas encore retrouvé | B et C encore séparés |
| 26,3 cm | ✓ C, IoU maximal : 0,57 | ✗ B et C déjà réunis |
| 32,5 cm | ✓ A, IoU maximal : 0,84 | IoU au même r : A 0,73 |
| 48,9 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Hiérarchie des supports à k = 10

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,84 / 0,72 / 0,57 (hiérarchie de points HGP : 0,84 / 0,76 / 0,57). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 14,4 cm | ✓ B, IoU maximal : 0,72 |
| 14,4 cm | ✗ B et C réunis : C pas encore retrouvé |
| 26,3 cm | ✓ C, IoU maximal : 0,57 |
| 32,5 cm | ✓ A, IoU maximal : 0,84 |
| 48,9 cm | ✓ A, B et C réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k10.json`](resultats_supports_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

