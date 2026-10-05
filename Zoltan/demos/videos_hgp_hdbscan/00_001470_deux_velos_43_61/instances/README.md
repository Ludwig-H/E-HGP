# Deux vélos (trame 00/001470) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001470_deux_velos_43_61_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,3 cm : HGP, A, IoU maximal : 0,96 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,82  ·  B 0,41" src="00_001470_deux_velos_43_61_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 61 s, k = 5, 1920 × 1080 : [thème sombre](00_001470_deux_velos_43_61_instances_k5_sombre.mp4) · [thème clair](00_001470_deux_velos_43_61_instances_k5_clair.mp4) ; image finale : [sombre](00_001470_deux_velos_43_61_instances_k5_sombre_bilan.png) · [clair](00_001470_deux_velos_43_61_instances_k5_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 250 points (A 138, B 112) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,96 / 0,71 | 0,82 / **0,48** | HGP réussit, HDBSCAN échoue |
| 10 | 0,62 / **0,48** | 0,62 / **0,45** | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 16,2 cm | ✓ A, IoU maximal : 0,82 |
| 17,9 cm | ✗ A et B réunis : B jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 16,2 cm | ✓ B, IoU maximal : 0,71 | IoU au même r : B 0,41 |
| 16,3 cm | ✓ A, IoU maximal : 0,96 ; ✓ A et B retrouvés, encore séparés | IoU au même r : A 0,82  ·  B 0,41 |
| 16,4 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

