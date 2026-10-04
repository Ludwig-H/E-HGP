# Deux vélos (trame 08/000656) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 17,1 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="08_000656_deux_velos_37_61_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 39 s, k = 10, 1920 × 1080 : [thème sombre](08_000656_deux_velos_37_61_sans_sol_k10_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_sans_sol_k10_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_sans_sol_k10_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_sans_sol_k10_clair_bilan.png).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 5 863 points, dont 269 des objets (A 52, B 217) ; 22 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,51 / 0,96 | 0,80 / 0,96 | les deux réussissent |
| 10 | 0,85 / 0,97 | **0,37** / 0,97 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié.

## Événements de la vidéo (k = 10)

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 6,8 cm |  | ✓ B retrouvé · IoU 0,54 |
| 12,8 cm | ✓ B retrouvé · IoU 0,60 |  |
| 14,4 cm | A et B encore séparés | ✗ A et B réunis : A jamais retrouvé |
| 17,1 cm | ✓ A et B retrouvés, encore séparés | ✗ A et B déjà réunis |
| 21,3 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo) ; nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

