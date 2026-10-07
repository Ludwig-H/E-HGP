# Vidéos : HGP contre HDBSCAN

Douze scènes SemanticKITTI (deux ou trois vélos, ou des vélos et un piéton, très proches) où la hiérarchie de points HGP de
Morse HGP 3D v11 contient un groupe pour chaque objet alors que celle de HDBSCAN, au même ordre k, réunit des objets avant
de les avoir tous retrouvés. Chaque exemple est montré en deux variantes, chacune dans son sous-dossier :

- `instances/` : les seuls points des instances concernées (vérité terrain), concaténés : ni sol, ni fond, ni autre
  objet. Ce sont les vidéos les plus propres ;
- `sans_sol/` : la même scène nettoyée automatiquement, sans aucune étiquette : tout ce que Patchwork++ (paramètres de la
  v8, `tools/ground.py`) ne classe pas en sol, dans la boîte horizontale des objets élargie de 1 m, à toutes hauteurs.
  On y voit les murs, la végétation, les autres objets, et le sol que Patchwork++ laisse.

Chaque configuration où HGP réussit et HDBSCAN échoue, variante (instances seules, sans sol) × ordre (k = 5, k = 10),
a sa vidéo, en thème sombre et en thème clair ; une variante sans gain n'est montrée qu'à k = 5. Les mesures des deux
ordres sont dans chaque README. Chacune de ces vidéos a sa jumelle (suffixe `_supports`) où se dessine la hiérarchie
des supports q2, q3, q4 de l'arbre couvrant au même ordre (section « Hiérarchie des supports »).

```text
phase=demonstration_hors_registre
backend=cpu_reference (morsehgp3D_v11 : export natif de la tour FULL + bench/points_radius.py ; scikit-learn 1.7.2)
profile=quantized_u21_input_only (grille de 1 mm)
mode=illustration
public_status=not_claimed
```

## Exemples

« gain HGP » : HGP réussit et HDBSCAN échoue ; « perte HGP » : l'inverse ; « deux échecs », « deux réussites ».
Réussite : chaque objet du groupe a, dans la hiérarchie, un groupe d'IoU > 1/2.

<!-- liste:début -->
| exemple | objets | instances seules : k = 5 / 10 | sans sol : k = 5 / 10 | vidéos |
| --- | --- | --- | --- | --- |
| [00/001300](00_001300_deux_velos_53_67/README.md) | A vélo, B vélo | deux échecs / deux échecs | deux réussites / **gain HGP** | instances k = 5 : [sombre](00_001300_deux_velos_53_67/instances/00_001300_deux_velos_53_67_instances_k5_sombre.mp4) · [clair](00_001300_deux_velos_53_67/instances/00_001300_deux_velos_53_67_instances_k5_clair.mp4), supports [sombre](00_001300_deux_velos_53_67/instances/00_001300_deux_velos_53_67_instances_k5_supports_sombre.mp4) · [clair](00_001300_deux_velos_53_67/instances/00_001300_deux_velos_53_67_instances_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](00_001300_deux_velos_53_67/sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_sombre.mp4) · [clair](00_001300_deux_velos_53_67/sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_clair.mp4), supports [sombre](00_001300_deux_velos_53_67/sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_supports_sombre.mp4) · [clair](00_001300_deux_velos_53_67/sans_sol/00_001300_deux_velos_53_67_sans_sol_k10_supports_clair.mp4) |
| [00/001470](00_001470_deux_velos_43_61/README.md) | A vélo, B vélo | **gain HGP** / deux échecs | deux échecs / deux échecs | instances k = 5 : [sombre](00_001470_deux_velos_43_61/instances/00_001470_deux_velos_43_61_instances_k5_sombre.mp4) · [clair](00_001470_deux_velos_43_61/instances/00_001470_deux_velos_43_61_instances_k5_clair.mp4), supports [sombre](00_001470_deux_velos_43_61/instances/00_001470_deux_velos_43_61_instances_k5_supports_sombre.mp4) · [clair](00_001470_deux_velos_43_61/instances/00_001470_deux_velos_43_61_instances_k5_supports_clair.mp4) ; sans sol k = 5 : [sombre](00_001470_deux_velos_43_61/sans_sol/00_001470_deux_velos_43_61_sans_sol_k5_sombre.mp4) · [clair](00_001470_deux_velos_43_61/sans_sol/00_001470_deux_velos_43_61_sans_sol_k5_clair.mp4), supports [sombre](00_001470_deux_velos_43_61/sans_sol/00_001470_deux_velos_43_61_sans_sol_k5_supports_sombre.mp4) · [clair](00_001470_deux_velos_43_61/sans_sol/00_001470_deux_velos_43_61_sans_sol_k5_supports_clair.mp4) |
| [00/001472](00_001472_trois_velos_40_42_59/README.md) | A vélo, B vélo, C vélo | **gain HGP** / **gain HGP** | deux échecs / deux échecs | instances k = 5 : [sombre](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k5_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k5_clair.mp4), supports [sombre](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k5_supports_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k5_supports_clair.mp4) ; instances k = 10 : [sombre](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k10_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k10_clair.mp4), supports [sombre](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k10_supports_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/instances/00_001472_trois_velos_40_42_59_instances_k10_supports_clair.mp4) ; sans sol k = 5 : [sombre](00_001472_trois_velos_40_42_59/sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_clair.mp4), supports [sombre](00_001472_trois_velos_40_42_59/sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_supports_sombre.mp4) · [clair](00_001472_trois_velos_40_42_59/sans_sol/00_001472_trois_velos_40_42_59_sans_sol_k5_supports_clair.mp4) |
| [00/001502](00_001502_deux_velos_28_66/README.md) | A vélo, B vélo | **gain HGP** / **gain HGP** | **gain HGP** / **gain HGP** | instances k = 5 : [sombre](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k5_sombre.mp4) · [clair](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k5_clair.mp4), supports [sombre](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k5_supports_sombre.mp4) · [clair](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k5_supports_clair.mp4) ; instances k = 10 : [sombre](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k10_sombre.mp4) · [clair](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k10_clair.mp4), supports [sombre](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k10_supports_sombre.mp4) · [clair](00_001502_deux_velos_28_66/instances/00_001502_deux_velos_28_66_instances_k10_supports_clair.mp4) ; sans sol k = 5 : [sombre](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_sombre.mp4) · [clair](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_clair.mp4), supports [sombre](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_supports_sombre.mp4) · [clair](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_sombre.mp4) · [clair](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_clair.mp4), supports [sombre](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_supports_sombre.mp4) · [clair](00_001502_deux_velos_28_66/sans_sol/00_001502_deux_velos_28_66_sans_sol_k10_supports_clair.mp4) |
| [00/002140](00_002140_pieton_velo_1_6/README.md) | A piéton, B vélo | deux réussites / **gain HGP** | deux réussites / deux réussites | instances k = 10 : [sombre](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_sombre.mp4) · [clair](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_clair.mp4), supports [sombre](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_supports_sombre.mp4) · [clair](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_supports_clair.mp4) ; sans sol k = 5 : [sombre](00_002140_pieton_velo_1_6/sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_sombre.mp4) · [clair](00_002140_pieton_velo_1_6/sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_clair.mp4), supports [sombre](00_002140_pieton_velo_1_6/sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_supports_sombre.mp4) · [clair](00_002140_pieton_velo_1_6/sans_sol/00_002140_pieton_velo_1_6_sans_sol_k5_supports_clair.mp4) |
| [06/000774](06_000774_trois_velos_6_14_15/README.md) | A vélo, B vélo, C vélo | deux échecs / deux échecs | deux réussites / **gain HGP** | instances k = 5 : [sombre](06_000774_trois_velos_6_14_15/instances/06_000774_trois_velos_6_14_15_instances_k5_sombre.mp4) · [clair](06_000774_trois_velos_6_14_15/instances/06_000774_trois_velos_6_14_15_instances_k5_clair.mp4), supports [sombre](06_000774_trois_velos_6_14_15/instances/06_000774_trois_velos_6_14_15_instances_k5_supports_sombre.mp4) · [clair](06_000774_trois_velos_6_14_15/instances/06_000774_trois_velos_6_14_15_instances_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](06_000774_trois_velos_6_14_15/sans_sol/06_000774_trois_velos_6_14_15_sans_sol_k10_sombre.mp4) · [clair](06_000774_trois_velos_6_14_15/sans_sol/06_000774_trois_velos_6_14_15_sans_sol_k10_clair.mp4), supports [sombre](06_000774_trois_velos_6_14_15/sans_sol/06_000774_trois_velos_6_14_15_sans_sol_k10_supports_sombre.mp4) · [clair](06_000774_trois_velos_6_14_15/sans_sol/06_000774_trois_velos_6_14_15_sans_sol_k10_supports_clair.mp4) |
| [06/000800](06_000800_pieton_deux_velos_2_8_12/README.md) | A piéton, B vélo, C vélo | **gain HGP** / **gain HGP** | **gain HGP** / **gain HGP** | instances k = 5 : [sombre](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_clair.mp4), supports [sombre](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_supports_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k5_supports_clair.mp4) ; instances k = 10 : [sombre](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_clair.mp4), supports [sombre](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_supports_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/instances/06_000800_pieton_deux_velos_2_8_12_instances_k10_supports_clair.mp4) ; sans sol k = 5 : [sombre](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_clair.mp4), supports [sombre](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_supports_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_clair.mp4), supports [sombre](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_supports_sombre.mp4) · [clair](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_supports_clair.mp4) |
| [08/000656](08_000656_deux_velos_37_61/README.md) | A vélo, B vélo | deux réussites / deux réussites | deux réussites / **gain HGP** | instances k = 5 : [sombre](08_000656_deux_velos_37_61/instances/08_000656_deux_velos_37_61_instances_k5_sombre.mp4) · [clair](08_000656_deux_velos_37_61/instances/08_000656_deux_velos_37_61_instances_k5_clair.mp4), supports [sombre](08_000656_deux_velos_37_61/instances/08_000656_deux_velos_37_61_instances_k5_supports_sombre.mp4) · [clair](08_000656_deux_velos_37_61/instances/08_000656_deux_velos_37_61_instances_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](08_000656_deux_velos_37_61/sans_sol/08_000656_deux_velos_37_61_sans_sol_k10_sombre.mp4) · [clair](08_000656_deux_velos_37_61/sans_sol/08_000656_deux_velos_37_61_sans_sol_k10_clair.mp4), supports [sombre](08_000656_deux_velos_37_61/sans_sol/08_000656_deux_velos_37_61_sans_sol_k10_supports_sombre.mp4) · [clair](08_000656_deux_velos_37_61/sans_sol/08_000656_deux_velos_37_61_sans_sol_k10_supports_clair.mp4) |
| [08/001170](08_001170_deux_velos_43_57/README.md) | A vélo, B vélo | **gain HGP** / **gain HGP** | **gain HGP** / **gain HGP** | instances k = 5 : [sombre](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k5_sombre.mp4) · [clair](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k5_clair.mp4), supports [sombre](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k5_supports_sombre.mp4) · [clair](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k5_supports_clair.mp4) ; instances k = 10 : [sombre](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k10_sombre.mp4) · [clair](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k10_clair.mp4), supports [sombre](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k10_supports_sombre.mp4) · [clair](08_001170_deux_velos_43_57/instances/08_001170_deux_velos_43_57_instances_k10_supports_clair.mp4) ; sans sol k = 5 : [sombre](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_sombre.mp4) · [clair](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_clair.mp4), supports [sombre](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_supports_sombre.mp4) · [clair](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_sombre.mp4) · [clair](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_clair.mp4), supports [sombre](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_supports_sombre.mp4) · [clair](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k10_supports_clair.mp4) |
| [08/002776](08_002776_deux_velos_17_64/README.md) | A vélo, B vélo | **gain HGP** / deux échecs | deux échecs / deux échecs | instances k = 5 : [sombre](08_002776_deux_velos_17_64/instances/08_002776_deux_velos_17_64_instances_k5_sombre.mp4) · [clair](08_002776_deux_velos_17_64/instances/08_002776_deux_velos_17_64_instances_k5_clair.mp4), supports [sombre](08_002776_deux_velos_17_64/instances/08_002776_deux_velos_17_64_instances_k5_supports_sombre.mp4) · [clair](08_002776_deux_velos_17_64/instances/08_002776_deux_velos_17_64_instances_k5_supports_clair.mp4) ; sans sol k = 5 : [sombre](08_002776_deux_velos_17_64/sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_sombre.mp4) · [clair](08_002776_deux_velos_17_64/sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_clair.mp4), supports [sombre](08_002776_deux_velos_17_64/sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_supports_sombre.mp4) · [clair](08_002776_deux_velos_17_64/sans_sol/08_002776_deux_velos_17_64_sans_sol_k5_supports_clair.mp4) |
| [08/002852](08_002852_deux_velos_6_51/README.md) | A vélo, B vélo | **gain HGP** / deux échecs | **gain HGP** / **gain HGP** | instances k = 5 : [sombre](08_002852_deux_velos_6_51/instances/08_002852_deux_velos_6_51_instances_k5_sombre.mp4) · [clair](08_002852_deux_velos_6_51/instances/08_002852_deux_velos_6_51_instances_k5_clair.mp4), supports [sombre](08_002852_deux_velos_6_51/instances/08_002852_deux_velos_6_51_instances_k5_supports_sombre.mp4) · [clair](08_002852_deux_velos_6_51/instances/08_002852_deux_velos_6_51_instances_k5_supports_clair.mp4) ; sans sol k = 5 : [sombre](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_sombre.mp4) · [clair](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_clair.mp4), supports [sombre](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_supports_sombre.mp4) · [clair](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_supports_clair.mp4) ; sans sol k = 10 : [sombre](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k10_sombre.mp4) · [clair](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k10_clair.mp4), supports [sombre](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k10_supports_sombre.mp4) · [clair](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k10_supports_clair.mp4) |
| [10/000424](10_000424_deux_velos_2_3/README.md) | A vélo, B vélo | **gain HGP** / deux échecs | deux échecs / deux échecs | instances k = 5 : [sombre](10_000424_deux_velos_2_3/instances/10_000424_deux_velos_2_3_instances_k5_sombre.mp4) · [clair](10_000424_deux_velos_2_3/instances/10_000424_deux_velos_2_3_instances_k5_clair.mp4), supports [sombre](10_000424_deux_velos_2_3/instances/10_000424_deux_velos_2_3_instances_k5_supports_sombre.mp4) · [clair](10_000424_deux_velos_2_3/instances/10_000424_deux_velos_2_3_instances_k5_supports_clair.mp4) ; sans sol k = 5 : [sombre](10_000424_deux_velos_2_3/sans_sol/10_000424_deux_velos_2_3_sans_sol_k5_sombre.mp4) · [clair](10_000424_deux_velos_2_3/sans_sol/10_000424_deux_velos_2_3_sans_sol_k5_clair.mp4), supports [sombre](10_000424_deux_velos_2_3/sans_sol/10_000424_deux_velos_2_3_sans_sol_k5_supports_sombre.mp4) · [clair](10_000424_deux_velos_2_3/sans_sol/10_000424_deux_velos_2_3_sans_sol_k5_supports_clair.mp4) |
<!-- liste:fin -->

## Ce que disent les 97 groupes

Les candidats sont les 97 groupes de deux ou trois vélos ou piétons de la recherche de [`../tools/chercher_bouts.py`](../tools/chercher_bouts.py)
(séquences 00 à 10, objets d'au moins 50 points à moins de 0,6 m puis 1 m l'un de l'autre, la trame la plus serrée de
chaque groupe ; reçu [`bouts_g4`](../../../morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/README.md)). Les 263
groupes de voitures n'ont pas été mesurés sans sol : sur des trames entières sans sol, HDBSCAN n'a manqué aucune des 2 188
voitures du criblage ([README de `demos/`](../README.md)). Issues, mêmes critères pour les deux variantes :

| variante | k | gain HGP | perte HGP | deux échecs | deux réussites |
| --- | --- | --- | --- | --- | --- |
| instances seules | 5 | 10 | 1 | 14 | 72 |
| instances seules | 10 | 7 | 0 | 18 | 72 |
| sans sol (Patchwork++) | 5 | 8 | 9 | 35 | 45 |
| sans sol (Patchwork++) | 10 | 25 | 0 | 38 | 34 |

- Le fond durcit le test : deux échecs passent de 14 à 35 à k = 5 ; les objets touchent des murs, des haies, d'autres
  objets, et Patchwork++ retire une partie des roues (86 groupes sur 97 y perdent des points d'objet).
- Sans sol, à k = 5, HGP perd autant qu'il gagne : 8 gains, 9 pertes. Les 9 pertes sont toutes des groupes d'une même
  rangée de quatre vélos, vue dans les trames 08/001180 et 08/001182 (instances 43, 55, 56, 57) ; l'exemple
  [`08_001170_deux_velos_43_57`](08_001170_deux_velos_43_57/README.md) est un autre groupe de cette rangée, gagnant
  dans les deux variantes.
- Sans sol, à k = 10, HGP gagne 25 fois et ne perd jamais.
- 31 groupes ont au moins un gain ; beaucoup sont des sous-groupes d'une même rangée, d'où douze scènes.

Tableau complet, groupe par groupe : [`exemples.json`](exemples.json).

## Critères

Fixés avant la lecture des résultats (`tools/choisir_exemples.py`) :

- à l'ordre k (5 et 10) et dans une variante, une méthode réussit si chaque objet du groupe a un bloc d'IoU > 1/2, au
  sens de la qualité panoptique (meilleur bloc, points void exclus ; un point du fond compte comme un point quelconque) ;
- un groupe est un exemple s'il a au moins un gain HGP (k = 5 ou 10, l'une ou l'autre variante) ;
- deux groupes d'une même séquence qui partagent une instance montrent la même scène : un seul exemple par scène, celui
  qui a le plus de gains, puis le plus de gains à k = 5, puis le plus d'objets ;
- une variante a une vidéo à chaque ordre où HGP gagne ; sans gain, une seule, à k = 5 (au moins quatre vidéos par
  scène : deux variantes, deux thèmes).

Le meilleur bloc est une borne optimiste : il suppose un oracle qui choisirait, objet par objet, le meilleur niveau. Toute
extraction à partir de la même hiérarchie (EOM, feuilles) rend des blocs de cette hiérarchie : elle fait au mieux aussi
bien. Les sorties plates des bouts sont dans leurs dossiers de catégorie (section « Sortie plate »).

## Lire une vidéo

- **Deux colonnes, mêmes réglages** : à gauche la hiérarchie de points HGP de `morsehgp3D_v11` (Hʳₖ₊₁,
  [`HIERARCHIE_POINTS.md`](../../../morsehgp3D_v11/docs/HIERARCHIE_POINTS.md)), à droite l'arbre complet de HDBSCAN
  (scikit-learn 1.7.2, `min_samples` = k). Mêmes points, même k, même caméra.
- **Début** : la vérité terrain reste immobile 1,6 s (objets en couleur, nommés, halos renforcés ; le fond en petits
  points pâles), puis la caméra tourne jusqu'à sa pose, prise du côté du capteur sauf si un autre angle sépare mieux les
  objets à l'écran ou évite qu'un mur les masque. Elle reste ensuite immobile.
- **Pendant le balayage** : le halo coloré sous les points garde la vérité terrain (A bleu, B ambre, C vert d'eau).
  Gros point de la couleur d'un objet : le groupe de la hiérarchie qui suit cet objet ; gros point rouge : un groupe qui
  réunit les groupes de deux objets ou plus ; point gris moyen : un autre groupe ; petit point pâle : point encore seul.
  Les points du fond ont les mêmes états, en plus petit.
- **Deux balayages, même échelle** : HDBSCAN balaie d'abord r en échelle logarithmique (HGP attend, marqué
  « ensuite »), revient au départ, puis HGP balaie ; HDBSCAN le suit alors au même r, sans pause propre. r est à la
  même échelle spatiale dans les deux colonnes : rayon des boules d'ordre k pour HGP, distance d'atteignabilité mutuelle
  (rayon de la boule des k voisins centrée sur un point) pour HDBSCAN, sans le facteur 1/2 de la thèse, qui faisait
  détecter HDBSCAN à des r environ deux fois plus petits. Le verdict de chaque colonne n'en dépend pas.
- **Pauses**, à tous les ordres et dans les deux variantes : chaque objet reconnu au moment où l'IoU de sa branche est
  **maximal** (pas à son premier passage au-dessus de 0,5) ; les objets tous reconnus et encore séparés, au moment où le
  moins bien reconnu l'est le mieux ; chaque **effondrement** de l'IoU d'une branche (baisse d'au moins 0,10 et d'au
  moins un quart) quand elle absorbe le fond (le mur, la végétation, le sol laissé par Patchwork++, nommés d'après la
  classe SemanticKITTI majoritaire des points absorbés) ou une partie d'un autre objet : après son maximum pour un objet
  reconnu, à chaque fois pour un objet jamais reconnu ; chaque fusion de branches, en rouge si un objet réuni n'avait
  pas encore été retrouvé, en vert sinon. Quand HGP retrouve un objet, la colonne HDBSCAN donne au même r les
  objets déjà réunis et l'IoU des autres (en rouge s'il ne dépasse pas 0,5) ; à une mauvaise fusion de HGP, elle dit
  si ces objets y sont encore séparés. L'instant clé (affiche du README) est pris dans le balayage de HGP. Sous chaque vue, l'IoU du groupe qui suit chaque objet en fonction de r.
- **Fin** : le meilleur IoU de chaque objet dans chaque hiérarchie.
- **Groupe qui suit un objet** : une graine dans le meilleur bloc de l'objet ; à chaque niveau, le bloc qui la contient.
  Il passe par le meilleur bloc, et son meilleur IoU est celui des tableaux (contrôlé).

Formats : MP4 H.264 (profil High, yuv420p, 1920 × 1080, 30 i/s, sans son, `+faststart`), thèmes de Percolia.com
(`*_sombre.mp4` sur fond marine, `*_clair.mp4` sur fond blanc), avec l'instant clé et l'image finale en PNG pour les
affiches. Lecteur interactif : `../player/duel.html?scene=../videos_hgp_hdbscan/<exemple>/<variante>/data/duel_k<k>.js`.

## Hiérarchie des supports

Les vidéos `*_supports_*.mp4` montrent l'arbre couvrant d'ordre k de Morse HGP 3D v11, tel que le publie
`mhgp11 --sortie=supports` depuis le 6 octobre 2026 (format `MHGP11SP` version 2,
[docs/SORTIES.md](../../../morsehgp3D_v11/docs/SORTIES.md) § 6) : ses arêtes de Kruskal, les naissances et les fusions
de l'arbre d'ordre k, chacune avec son seul support S\*, arête (q2), triangle (q3) ou tétraèdre (q4). Les liaisons
internes (boules qui relient des parties d'un même nœud) n'y sont plus, et sur un plateau (plusieurs fusions au même
niveau) une fusion n'est gardée que si elle réunit encore des branches distinctes (union-find sur les enfants du nœud,
dans l'ordre des boules). Un nœud est réalisé par les supports des boules de son sous-arbre.

- **Balayage** : r croît en échelle logarithmique (rayon de la sphère de S\* de chaque boule) ; chaque support
  apparaît au niveau de sa boule, et la hiérarchie se dessine. Compteurs des supports dessinés en haut à droite.
- **Couleurs** : supports du nœud qui suit un objet dans la couleur de l'objet (faces translucides, arêtes pleines), en
  rouge le nœud qui réunit deux objets ou plus, en gris tous les autres nœuds ; halos de la vérité terrain sous les
  points, comme dans les vidéos du duel.
- **Nœud qui suit un objet** : le nœud de meilleur IoU des sites de ses supports (complet, juste avant la naissance de
  son parent) ; en dessous, l'enfant de meilleur IoU à chaque étage ; au-dessus, ses ancêtres. Les IoU se comptent sur
  les sites des supports, points void exclus : les sites intérieurs, qu'aucun support ne porte, n'y sont pas.
- **Pauses** : les mêmes règles que les vidéos du duel (IoU maximal, objets encore séparés, effondrement quand le nœud
  absorbe le fond, fusion d'objets).
- **Accord avec la hiérarchie de points** : sur les 32 configurations, le meilleur IoU par les supports s'écarte d'au plus
  0,11 de celui de la hiérarchie de points Hʳₖ₊₁ (0,001 sur 08/002852 sans sol à k = 5) ; la chronologie des
  événements est la même à quelques millimètres près. Retirer les liaisons internes ne change aucun meilleur IoU : leurs
  sites étaient déjà portés par les naissances et les fusions de leur nœud.
- **Taille** : de 2 534 supports (00/001502, instances, k = 5) à 281 758 (08/000656, sans sol, k = 10), 76 % des
  supports de la version 1 (toutes les boules critiques). Le lecteur (`../player/supports.html`, `supports.js`) peint
  chaque groupe de couleur dans un calque hors écran, complété au fil du balayage.
- **Plateaux** : la première version de l'arbre couvrant (0cc9cbec4) gardait toutes les fusions d'un nœud ; sur un
  plateau, certaines ne réunissaient rien de plus et fermaient un cycle. Le sélecteur de Kruskal des auditeurs, intégré
  en 07428324e, en retire 447 sur 1,18 million dans les 32 scènes, presque toutes dans les variantes sans sol (104 sur
  82 569 pour 08/002776) : 15 vidéos sur 32 ont été refaites, les 17 autres sont identiques. Aucun meilleur IoU ne
  change.

## Contrôle des calculs

Mesures locales (codespace, GCP non utilisé) avec l'export natif `mhgp11_points_export` compilé depuis ce dépôt (tour
FULL exacte, kmax = 10) et scikit-learn 1.7.2, comme les sessions G4 des bouts. Variante « instances » : 388 comparaisons
sur 388 identiques aux mesures G4 `claudebouts1` (meilleurs IoU à k = 5 et 10, HGP et HDBSCAN, 97 groupes). Recompilé
après la correction amont de l'export (commit 3bd4d734e, format inchangé en u21), l'export redonne les 1 552 valeurs
des deux variantes à l'identique (meilleurs IoU et nombres de blocs). Chaque
scène refuse un meilleur IoU différent de la mesure ; `tools/test_duel.py` vérifie que le lecteur rejoue les mêmes
groupes que Python et les couleurs (contraste, daltonisme). Le masque de sol de Patchwork++ est identique octet pour
octet à celui de la v8 sur 08/000000.

Vidéos de supports : `mhgp11 --sortie=supports` compilé depuis `main` (07428324e, profil u21, format `MHGP11SP`
version 2) ; chaque dossier publié passe `check_directory` et `read_supports` de `morsehgp3D_v11/bench/mhgp11_formats.py`
(arbre, rôles, ordre canonique, S\* support positif de sa sphère en entiers exacts, structure couvrante : chaque fusion
gardée réunit au moins deux branches distinctes, enfants finalement connexes ; même version au manifeste et au
fichier) avant d'être lu, et
`tools/supports_scene.py` refuse un fichier d'une autre version ou une boule hors de l'arbre couvrant ; commit, version,
empreintes du fichier et du manifeste dans `resultats_supports_k<k>.json`. `tools/test_supports.py` : un arbre fait à la main (deux objets et un mur, chaînes,
IoU, fusion et effondrement connus) et le contrat des 32 scènes locales (supports triés, chaînes emboîtées, suivi
cohérent, écart à la hiérarchie de points sous 0,15).

## Reproduire

Points et scènes dans les dossiers `data/`, ignorés par git (CC BY-NC-SA) ; `CACHE` contient l'archive des étiquettes,
les trames sont lues à distance.

```bash
cmake -S morsehgp3D_v11 -B BUILD -DCMAKE_BUILD_TYPE=Release && cmake --build BUILD --target mhgp11_points_export
pip install scikit-learn==1.7.2 numpy scipy pypatchworkpp==1.4.1 imageio-ffmpeg    # + Node.js, Playwright, Chromium
T=Zoltan/demos/tools
python3 $T/chercher_bouts.py --cache CACHE --out LOT --rebuild morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/bouts_lot1.json
python3 $T/chercher_bouts.py --cache CACHE --out SANS_SOL --rebuild LOT/bouts_refaits.json --sans-sol --marge 1 --noms NOMS
python3 $T/mesurer_bouts.py --bouts LOT/bouts_refaits.json --data LOT/data --variante instances --noms NOMS --export BUILD/mhgp11_points_export --out MESURES/instances
python3 $T/mesurer_bouts.py --bouts SANS_SOL/bouts_sans_sol.json --data SANS_SOL/data --variante sans_sol --export BUILD/mhgp11_points_export --out MESURES/sans_sol
python3 $T/choisir_exemples.py --bouts LOT/bouts_refaits.json --sans-sol SANS_SOL/bouts_sans_sol.json --mesures MESURES \
    --data-instances LOT/data --data-sans-sol SANS_SOL/data --out Zoltan/demos/videos_hgp_hdbscan
python3 $T/duel_scene.py --export BUILD/mhgp11_points_export Zoltan/demos/videos_hgp_hdbscan/*/instances Zoltan/demos/videos_hgp_hdbscan/*/sans_sol
node $T/render_duel.cjs Zoltan/demos/videos_hgp_hdbscan/00_001502_deux_velos_28_66/sans_sol    # deux thèmes
cmake --build BUILD --target mhgp11_cli                                                       # supports
python3 $T/supports_scene.py --mhgp11 BUILD/mhgp11 --source SHA --k 5 Zoltan/demos/videos_hgp_hdbscan/08_002852_deux_velos_6_51/sans_sol
node $T/render_duel.cjs Zoltan/demos/videos_hgp_hdbscan/08_002852_deux_velos_6_51/sans_sol --lecteur supports --k 5
python3 $T/duel_readme.py                                                                      # README
python3 -O -m unittest discover -s $T -p 'test_*.py'
```

`NOMS` : les groupes de vélos et de piétons (`kind` ≠ « voitures » dans `bouts.json`).

## Versions anglaises

Quatre vidéos existent aussi en anglais (thème clair), pour une présentation : mêmes points, mêmes niveaux, mêmes
pauses et même minutage que les vidéos françaises ; seuls les mots changent (titres, bandeaux, légende, nombres à
point décimal).

| Vidéo française | Version anglaise |
| --- | --- |
| [un piéton et un vélo, 00/002140, instances, k = 10](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_clair.mp4) | [anglais](00_002140_pieton_velo_1_6/instances/00_002140_pieton_velo_1_6_instances_k10_clair_en.mp4) |
| [un piéton et deux vélos, 06/000800, sans sol, k = 10](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_clair.mp4) | [anglais](06_000800_pieton_deux_velos_2_8_12/sans_sol/06_000800_pieton_deux_velos_2_8_12_sans_sol_k10_clair_en.mp4) |
| [deux vélos, 08/001170, sans sol, k = 5](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_clair.mp4) | [anglais](08_001170_deux_velos_43_57/sans_sol/08_001170_deux_velos_43_57_sans_sol_k5_clair_en.mp4) |
| [deux vélos, 08/002852, sans sol, k = 5](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_clair.mp4) | [anglais](08_002852_deux_velos_6_51/sans_sol/08_002852_deux_velos_6_51_sans_sol_k5_clair_en.mp4) |

Les textes d'une scène déjà construite se réécrivent sans recalculer les hiérarchies (`duel_scene.py --relabel en`,
qui écrit `data/duel_k<k>_en.js`) ; le lecteur prend `lang=en` dans son adresse. En français, `--relabel fr` redonne
chaque scène à l'octet près (32 scènes sur 32, et un test le vérifie) : les vidéos françaises ne changent pas.

```bash
python3 $T/duel_scene.py --relabel en --k 5 Zoltan/demos/videos_hgp_hdbscan/08_002852_deux_velos_6_51/sans_sol
node $T/render_duel.cjs Zoltan/demos/videos_hgp_hdbscan/08_002852_deux_velos_6_51/sans_sol --k 5 --theme clair --lang en
```

## Limites

- Les groupes viennent d'une recherche par la vérité terrain : ce sont des cas difficiles choisis, pas un échantillon
  de la route. Plusieurs scènes sont corrélées (même rangée vue sous plusieurs groupes ou trames).
- La boîte élargie de 1 m est un choix : plus large, elle ajouterait du fond ; la hiérarchie est calculée sur la
  découpe, pas sur la trame entière.
- Le meilleur bloc est un oracle optimiste, pas un découpage automatique.
- Une victoire se joue parfois à 0,51 ; les tableaux donnent toutes les valeurs.
