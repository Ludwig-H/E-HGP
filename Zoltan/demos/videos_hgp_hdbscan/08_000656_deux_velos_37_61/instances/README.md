# Deux vélos (trame 08/000656) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (les deux réussissent, aucun gain HGP dans cette variante) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 17,4 cm : HGP, B, IoU maximal : 0,95 ; ✓ A et B retrouvés, encore séparés ; HDBSCAN au même r, IoU au même r : A 0,85  ·  B 0,89" src="08_000656_deux_velos_37_61_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 63 s, k = 5, 1920 × 1080 : [thème sombre](08_000656_deux_velos_37_61_instances_k5_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_instances_k5_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_instances_k5_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_instances_k5_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="08_000656_deux_velos_37_61_instances_k5_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 5, r = 17,4 cm : B, IoU maximal : 0,95 ; ✓ A et B retrouvés, encore séparés" src="08_000656_deux_velos_37_61_instances_k5_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 38 s : [thème sombre](08_000656_deux_velos_37_61_instances_k5_supports_sombre.mp4) · [thème clair](08_000656_deux_velos_37_61_instances_k5_supports_clair.mp4) ; image finale : [sombre](08_000656_deux_velos_37_61_instances_k5_supports_sombre_bilan.png) · [clair](08_000656_deux_velos_37_61_instances_k5_supports_clair_bilan.png). Arbre couvrant d'ordre 5 : 3 071 nœuds, 3 071 naissances et fusions, chacune avec son support S\* (3 071 supports : 644 arêtes q2, 1 832 triangles q3, 595 tétraèdres q4).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 291 points (A 52, B 239) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet (meilleur bloc ; seuls les points non étiquetés et aberrants, classes 0 et 1, sont exclus : « autre structure » et « autre objet » comptent comme du fond) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | 0,90 / 0,95 | 0,90 / 0,95 | les deux réussissent |
| 10 | 0,98 / 0,95 | 0,90 / 0,94 | les deux réussissent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 22,0 cm | ✓ A, IoU maximal : 0,90 |
| 24,6 cm | ✓ B, IoU maximal : 0,95 ; ✓ A et B retrouvés, encore séparés |
| 24,8 cm | ✓ A et B réunis, chacun retrouvé avant |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 15,2 cm | ✓ A, IoU maximal : 0,90 | IoU au même r : A 0,83 |
| 17,4 cm | ✓ B, IoU maximal : 0,95 ; ✓ A et B retrouvés, encore séparés | IoU au même r : A 0,85  ·  B 0,89 |
| 19,1 cm | ✓ A et B réunis, chacun retrouvé avant |  |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Hiérarchie des supports à k = 5

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,98 / 0,95 (hiérarchie de points HGP : 0,90 / 0,95). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 16,0 cm | ✓ A, IoU maximal : 0,98 |
| 17,4 cm | ✓ B, IoU maximal : 0,95 ; ✓ A et B retrouvés, encore séparés |
| 19,1 cm | ✓ A et B réunis, chacun retrouvé avant |

Nombres : [`resultats_supports_k5.json`](resultats_supports_k5.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

