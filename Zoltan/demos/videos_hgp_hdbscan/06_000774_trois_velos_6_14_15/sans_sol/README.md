# Trois vélos (trame 06/000774) — sol retiré automatiquement (Patchwork++)

[Exemple](../README.md) · autre variante : [instances de la vérité terrain seules](../instances/README.md) · [liste des exemples](../../README.md)

**k = 10** (HGP réussit, HDBSCAN échoue) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="06_000774_trois_velos_6_14_15_sans_sol_k10_sombre_instant_cle.png">
  <img alt="Instant clé, k = 10, r = 19,8 cm : HGP, A, B et C retrouvés, encore séparés ; HDBSCAN, B et C déjà réunis" src="06_000774_trois_velos_6_14_15_sans_sol_k10_clair_instant_cle.png">
</picture>

Vidéo de 48 s, k = 10, 1920 × 1080 : [thème sombre](06_000774_trois_velos_6_14_15_sans_sol_k10_sombre.mp4) · [thème clair](06_000774_trois_velos_6_14_15_sans_sol_k10_clair.mp4) ; image finale : [sombre](06_000774_trois_velos_6_14_15_sans_sol_k10_sombre_bilan.png) · [clair](06_000774_trois_velos_6_14_15_sans_sol_k10_clair_bilan.png).

Points : tout ce que Patchwork++ (paramètres de la v8) ne classe pas en sol dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs : 2 706 points, dont 175 des objets (A 83, B 41, C 51) ; 115 points des objets retirés comme sol. Aucune étiquette ne sert au nettoyage : elles ne servent qu'à mesurer.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B / C) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,65 / 0,57 / 0,69 | 0,64 / 0,59 / 0,69 | les deux réussissent |
| 10 | 0,63 / 0,51 / 0,61 | 0,55 / **0,48** / **0,47** | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 10

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 14,5 cm |  | ✓ A retrouvé · IoU 0,51 |
| 15,0 cm | B et C encore séparés | ✗ B et C réunis : B et C jamais retrouvés |
| 18,5 cm | ✓ B retrouvé · IoU 0,51 |  |
| 19,5 cm | ✓ A retrouvé · IoU 0,51 |  |
| 19,8 cm | ✓ A, B et C retrouvés, encore séparés | ✗ B et C déjà réunis |
| 20,7 cm | A, B et C encore séparés | ✗ A, B et C réunis : B et C jamais retrouvés |
| 21,4 cm | ✓ B et C réunis, chacun retrouvé avant |  |
| 31,4 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k10.json`](resultats_duel_k10.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

