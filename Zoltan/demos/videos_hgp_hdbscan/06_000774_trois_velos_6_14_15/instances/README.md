# Trois vélos (trame 06/000774) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (les deux échouent, aucun gain HGP dans cette variante) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="06_000774_trois_velos_6_14_15_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 9,4 cm : HGP, B et C encore séparés ; HDBSCAN, B et C réunis : B jamais retrouvé" src="06_000774_trois_velos_6_14_15_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 49 s, k = 5, 1920 × 1080 : [thème sombre](06_000774_trois_velos_6_14_15_instances_k5_sombre.mp4) · [thème clair](06_000774_trois_velos_6_14_15_instances_k5_clair.mp4) ; image finale : [sombre](06_000774_trois_velos_6_14_15_instances_k5_sombre_bilan.png) · [clair](06_000774_trois_velos_6_14_15_instances_k5_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 290 points (A 141, B 63, C 86) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,80 / **0,47** / 0,58 | 0,80 / **0,49** / 0,55 | les deux échouent |
| 10 | 0,80 / **0,44** / 0,52 | 0,76 / **0,43** / 0,53 | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 9,4 cm | B et C encore séparés | ✗ B et C réunis : B jamais retrouvé |
| 14,6 cm |  | ✓ C, IoU maximal : 0,55 |
| 14,6 cm | ✗ B et C réunis : B jamais retrouvé |  |
| 14,8 cm |  | ✓ A, IoU maximal : 0,80 |
| 15,0 cm |  | ✗ A, B et C réunis : B jamais retrouvé |
| 16,7 cm | ✓ A, IoU maximal : 0,80 |  |
| 19,9 cm | ✓ C, IoU maximal : 0,58 |  |
| 21,5 cm | ✗ A, B et C réunis : B jamais retrouvé |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

