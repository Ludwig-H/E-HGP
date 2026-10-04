# Deux vélos (trame 10/000424) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="10_000424_deux_velos_2_3_sans_sol_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,1 cm : HGP, A et B encore séparés ; HDBSCAN, A et B réunis : B jamais retrouvé" src="10_000424_deux_velos_2_3_sans_sol_k5_clair_instant_cle.png">
</picture>

Vidéo de 37 s, k = 5, 1920 × 1080 : [thème sombre](10_000424_deux_velos_2_3_sans_sol_k5_sombre.mp4) · [thème clair](10_000424_deux_velos_2_3_sans_sol_k5_clair.mp4) ; image finale : [sombre](10_000424_deux_velos_2_3_sans_sol_k5_sombre_bilan.png) · [clair](10_000424_deux_velos_2_3_sans_sol_k5_clair_bilan.png).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 1 555 points, dont 207 des objets (A 124, B 83) ; 54 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,59 / **0,24** | 0,70 / **0,32** | les deux échouent |
| 10 | 0,71 / **0,25** | 0,53 / **0,21** | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 5)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 8,2 cm |  | ✓ A retrouvé · IoU 0,56 |
| 10,1 cm | A et B encore séparés | ✗ A et B réunis : B jamais retrouvé |
| 13,5 cm | ✓ A retrouvé · IoU 0,51 |  |
| 16,2 cm | ✗ A et B réunis : B jamais retrouvé |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

