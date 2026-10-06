# Deux vélos (trame 00/001300) — instances de la vérité terrain seules

[Exemple](../README.md) · autre variante : [sol retiré automatiquement (Patchwork++)](../sans_sol/README.md) · [liste des exemples](../../README.md)

**k = 5** (les deux échouent, aucun gain HGP dans cette variante) :

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001300_deux_velos_53_67_instances_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 15,7 cm : HGP, A et B réunis : A jamais retrouvé ; HDBSCAN au même r, A et B encore séparés" src="00_001300_deux_velos_53_67_instances_k5_clair_instant_cle.png">
</picture>

Vidéo de 58 s, k = 5, 1920 × 1080 : [thème sombre](00_001300_deux_velos_53_67_instances_k5_sombre.mp4) · [thème clair](00_001300_deux_velos_53_67_instances_k5_clair.mp4) ; image finale : [sombre](00_001300_deux_velos_53_67_instances_k5_sombre_bilan.png) · [clair](00_001300_deux_velos_53_67_instances_k5_clair_bilan.png).

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="00_001300_deux_velos_53_67_instances_k5_supports_sombre_instant_cle.png">
  <img alt="Hiérarchie des supports, k = 5, r = 18,0 cm : B, IoU maximal : 0,86" src="00_001300_deux_velos_53_67_instances_k5_supports_clair_instant_cle.png">
</picture>

Hiérarchie des supports q2, q3, q4 au même ordre, 36 s : [thème sombre](00_001300_deux_velos_53_67_instances_k5_supports_sombre.mp4) · [thème clair](00_001300_deux_velos_53_67_instances_k5_supports_clair.mp4) ; image finale : [sombre](00_001300_deux_velos_53_67_instances_k5_supports_sombre_bilan.png) · [clair](00_001300_deux_velos_53_67_instances_k5_supports_clair_bilan.png). Arbre couvrant d'ordre 5 : 3 268 nœuds, 3 268 naissances et fusions, chacune avec son support S\* (3 268 supports : 768 arêtes q2, 1 903 triangles q3, 597 tétraèdres q4).

Points : les seuls points des objets du groupe (vérité terrain SemanticKITTI), 296 points (A 56, B 240) : ni sol, ni fond, ni autre objet.

Meilleur IoU de chaque objet, même mesure que la campagne G4 (points void exclus) :

| k | HGP, meilleur IoU (A / B) | HDBSCAN, meilleur IoU | issue |
| --- | --- | --- | --- |
| 5 | **0,36** / 0,87 | **0,36** / 0,95 | les deux échouent |
| 10 | **0,36** / 0,83 | **0,29** / 0,84 | les deux échouent |

En gras : objet à 0,5 ou moins, qu'aucun groupe de la hiérarchie ne recouvre à plus de la moitié. Une vidéo par ordre où HGP réussit et HDBSCAN échoue ; sans gain, une seule, à k = 5.

## Événements de la vidéo à k = 5

Mêmes textes que les bandeaux. Premier balayage, HDBSCAN seul (HGP attend) :

| r | HDBSCAN |
| --- | --- |
| 22,7 cm | ✓ B, IoU maximal : 0,95 |
| 23,9 cm | ✗ A et B réunis : A jamais retrouvé |

Second balayage, HGP ; HDBSCAN le suit au même r, sans pause propre :

| r | HGP | HDBSCAN au même r |
| --- | --- | --- |
| 15,7 cm | ✗ A et B réunis : A jamais retrouvé | A et B encore séparés |
| 18,0 cm | ✓ B, IoU maximal : 0,87 | IoU au même r : B 0,79 |

Nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

## Hiérarchie des supports à k = 5

Meilleur IoU des sites des supports du nœud qui suit chaque objet : 0,36 / 0,86 (hiérarchie de points HGP : 0,36 / 0,87). Événements, mêmes textes que les bandeaux :

| r | supports |
| --- | --- |
| 15,7 cm | ✗ A et B réunis : A jamais retrouvé |
| 18,0 cm | ✓ B, IoU maximal : 0,86 |

Nombres : [`resultats_supports_k5.json`](resultats_supports_k5.json).

Lecture, légende et convention de niveau : [README de la liste](../../README.md#lire-une-vidéo).

## Données

`bout.json` décrit la variante (trame, empreintes, instances, découpe, mesures). Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git ; ils se refont depuis les archives officielles (README de la liste, « Reproduire »).

