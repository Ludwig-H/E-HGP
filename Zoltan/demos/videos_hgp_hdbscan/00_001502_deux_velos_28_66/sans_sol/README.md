# Deux vélos (trame 00/001502) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001502_deux_velos_28_66_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 7,7 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="00_001502_deux_velos_28_66_sans_sol_k5_clair_instant_cle.png">
</picture>

Vidéo de 38 s, k = 5, 1920 × 1080 : [thème sombre](00_001502_deux_velos_28_66_sans_sol_k5_sombre.mp4) · [thème clair](00_001502_deux_velos_28_66_sans_sol_k5_clair.mp4) ; image finale : [sombre](00_001502_deux_velos_28_66_sans_sol_k5_sombre_bilan.png) · [clair](00_001502_deux_velos_28_66_sans_sol_k5_clair_bilan.png).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 2 964 points, dont 164 des objets (A 113, B 51) ; 45 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,58 / 0,51 | **0,47** / **0,37** | HGP réussit, HDBSCAN échoue |
| 10 | 0,62 / 0,53 | **0,38** / **0,29** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 5)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 4,6 cm | A et B encore séparés | ✗ A et B réunis : A et B jamais retrouvés |
| 6,8 cm | ✓ A retrouvé · IoU 0,504 |  |
| 7,7 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 8,1 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

