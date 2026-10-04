# Deux vélos (trame 08/000656) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (les deux réussissent, aucun gain HGP dans cette variante) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 10,4 cm : HGP, A et B retrouvés, encore séparés ; HDBSCAN, —" src="08_000656_deux_velos_37_61_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 42 s, k = 5, 1920 × 1080 : [thème sombre](08_000656_deux_velos_37_61_instances_k5_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_instances_k5_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_instances_k5_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_instances_k5_clair_bilan.png).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 291 points (A 52, B 239) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,90 / 0,95 | 0,90 / 0,95 | les deux réussissent |
| 10 | 0,98 / 0,95 | 0,90 / 0,94 | les deux réussissent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Le niveau r croît pour les deux colonnes à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 5,8 cm |  | ✓ B retrouvé · IoU 0,66 |
| 6,3 cm |  | ✓ A et B retrouvés, encore séparés |
| 10,3 cm | ✓ A retrouvé · IoU 0,52 |  |
| 10,4 cm | ✓ A et B retrouvés, encore séparés |  |
| 12,4 cm |  | ✓ A et B réunis, chacun retrouvé avant |
| 19,1 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

