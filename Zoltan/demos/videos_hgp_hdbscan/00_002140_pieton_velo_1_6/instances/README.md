# Un piéton et un vélo (trame 00/002140) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_002140_pieton_velo_1_6_instances_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 14,7 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="00_002140_pieton_velo_1_6_instances_k10_clair_instant_cle.png">
</picture>

Vidéo de 39 s, k = 10, 1920 × 1080 : [thème sombre](00_002140_pieton_velo_1_6_instances_k10_sombre.mp4) · [thème clair](00_002140_pieton_velo_1_6_instances_k10_clair.mp4) ; image finale : [sombre](00_002140_pieton_velo_1_6_instances_k10_sombre_bilan.png) · [clair](00_002140_pieton_velo_1_6_instances_k10_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 160 points (A 82, B 78) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,94 / 0,91 | 0,90 / 0,78 | les deux réussissent |
| 10 | 0,94 / 0,87 | 0,76 / **0,49** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 10)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 6,2 cm |  | ✓ A retrouvé · IoU 0,56 |
| 8,3 cm |  | ✗ A et B réunis : B jamais retrouvé |
| 9,5 cm | ✓ A retrouvé · IoU 0,51 |  |
| 14,7 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 17,3 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

