# HGP réussit, HDBSCAN échoue

À un même ordre k au moins, la hiérarchie de HDBSCAN (`min_samples` = k) ne contient aucun groupe qui recouvre l'un des objets à plus de la moitié, alors que la hiérarchie de points HGP en contient un pour chaque objet.

## Bouts de scène

Parmi les 360 bouts mesurés, 13 tombent dans cette catégorie (11 de vélos, 2 de vélos et piétons). Chacun a un sous-dossier, à raison d'un par trame et par famille (le plus grand groupe d'abord).

| exemple | trame | objets (points) | écart | ordres concernés | k montré : pire objet HDBSCAN / HGP | IoU moyen HDBSCAN / HGP |
| --- | --- | --- | --- | --- | --- | --- |
| [Deux vélos](bout_00_001466_deux_velos_42_59/README.md) | 00/001466 | A vélo (192), B vélo (118) | 0,03 m | 10 | k = 10 : 0,38 / 0,52 | 0,51 / 0,60 |
| [Deux vélos](bout_00_001470_deux_velos_43_61/README.md) | 00/001470 | A vélo (138), B vélo (112) | 0,12 m | 3, 5 | k = 5 : 0,48 / 0,71 | 0,65 / 0,84 |
| [Trois vélos](bout_00_001472_trois_velos_40_42_59/README.md) | 00/001472 | A vélo (82), B vélo (139), C vélo (161) | 0,07 m | 2, 5, 10 | k = 5 : 0,41 / 0,60 | 0,61 / 0,72 |
| [Deux vélos](bout_00_001502_deux_velos_28_66/README.md) | 00/001502 | A vélo (146), B vélo (63) | 0,03 m | 5, 10 | k = 5 : 0,40 / 0,60 | 0,57 / 0,69 |
| [Un piéton et un vélo](bout_00_002140_pieton_velo_1_6/README.md) | 00/002140 | A piéton (82), B vélo (78) | 0,10 m | 10 | k = 10 : 0,49 / 0,87 | 0,62 / 0,90 |
| [Un piéton et deux vélos](bout_06_000800_pieton_deux_velos_2_8_12/README.md) | 06/000800 | A piéton (117), B vélo (86), C vélo (101) | 0,04 m | 2, 3, 5, 10 | k = 5 : 0,46 / 0,64 | 0,67 / 0,77 |
| [Trois vélos](bout_06_000800_trois_velos_8_12_13/README.md) | 06/000800 | A vélo (86), B vélo (101), C vélo (77) | 0,04 m | 2, 3, 5 | k = 5 : 0,46 / 0,64 | 0,67 / 0,78 |
| [Deux vélos](bout_08_001170_deux_velos_43_57/README.md) | 08/001170 | A vélo (141), B vélo (100) | 0,11 m | 5, 10 | k = 5 : 0,43 / 0,65 | 0,56 / 0,80 |
| [Trois vélos](bout_08_001182_trois_velos_55_56_57/README.md) | 08/001182 | A vélo (73), B vélo (132), C vélo (444) | 0,03 m | 10 | k = 10 : 0,37 / 0,51 | 0,66 / 0,76 |
| [Deux vélos](bout_08_002776_deux_velos_17_64/README.md) | 08/002776 | A vélo (198), B vélo (95) | 0,13 m | 3, 5 | k = 5 : 0,36 / 0,53 | 0,56 / 0,61 |
| [Deux vélos](bout_08_002852_deux_velos_6_51/README.md) | 08/002852 | A vélo (157), B vélo (122) | 0,05 m | 5 | k = 5 : 0,44 / 0,83 | 0,59 / 0,84 |
| [Deux vélos](bout_10_000424_deux_velos_2_3/README.md) | 10/000424 | A vélo (163), B vélo (98) | 0,04 m | 5 | k = 5 : 0,50 / 0,51 | 0,64 / 0,65 |

<!-- video:début -->
## Vidéos : HGP contre HDBSCAN

Une vidéo par bout, à l'ordre montré, en thème sombre et clair : les deux hiérarchies côte à côte, au même ordre k, le niveau r commun croissant, une pause à chaque événement. Lecture : [README de `demos/`](../README.md#vidéos-hgp-contre-hdbscan-des-bouts).

| bout (trame) | k | durée | vidéo | instant clé |
| --- | --- | --- | --- | --- |
| [00/001466](bout_00_001466_deux_velos_42_59/README.md) | 10 | 37 s | [sombre](bout_00_001466_deux_velos_42_59/bout_00_001466_deux_velos_42_59_hgp_hdbscan_k10_sombre.mp4) · [clair](bout_00_001466_deux_velos_42_59/bout_00_001466_deux_velos_42_59_hgp_hdbscan_k10_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [00/001470](bout_00_001470_deux_velos_43_61/README.md) | 5 | 39 s | [sombre](bout_00_001470_deux_velos_43_61/bout_00_001470_deux_velos_43_61_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_00_001470_deux_velos_43_61/bout_00_001470_deux_velos_43_61_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [00/001472](bout_00_001472_trois_velos_40_42_59/README.md) | 5 | 46 s | [sombre](bout_00_001472_trois_velos_40_42_59/bout_00_001472_trois_velos_40_42_59_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_00_001472_trois_velos_40_42_59/bout_00_001472_trois_velos_40_42_59_hgp_hdbscan_k5_clair.mp4) | HGP : B et C retrouvés, encore séparés ; HDBSCAN : B et C déjà réunis |
| [00/001502](bout_00_001502_deux_velos_28_66/README.md) | 5 | 38 s | [sombre](bout_00_001502_deux_velos_28_66/bout_00_001502_deux_velos_28_66_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_00_001502_deux_velos_28_66/bout_00_001502_deux_velos_28_66_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [00/002140](bout_00_002140_pieton_velo_1_6/README.md) | 10 | 38 s | [sombre](bout_00_002140_pieton_velo_1_6/bout_00_002140_pieton_velo_1_6_hgp_hdbscan_k10_sombre.mp4) · [clair](bout_00_002140_pieton_velo_1_6/bout_00_002140_pieton_velo_1_6_hgp_hdbscan_k10_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [06/000800](bout_06_000800_pieton_deux_velos_2_8_12/README.md) | 5 | 47 s | [sombre](bout_06_000800_pieton_deux_velos_2_8_12/bout_06_000800_pieton_deux_velos_2_8_12_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_06_000800_pieton_deux_velos_2_8_12/bout_06_000800_pieton_deux_velos_2_8_12_hgp_hdbscan_k5_clair.mp4) | HGP : A, B et C retrouvés, encore séparés ; HDBSCAN : A, B et C déjà réunis |
| [06/000800](bout_06_000800_trois_velos_8_12_13/README.md) | 5 | 48 s | [sombre](bout_06_000800_trois_velos_8_12_13/bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_06_000800_trois_velos_8_12_13/bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_clair.mp4) | HGP : A, B et C retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [08/001170](bout_08_001170_deux_velos_43_57/README.md) | 5 | 39 s | [sombre](bout_08_001170_deux_velos_43_57/bout_08_001170_deux_velos_43_57_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_08_001170_deux_velos_43_57/bout_08_001170_deux_velos_43_57_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [08/001182](bout_08_001182_trois_velos_55_56_57/README.md) | 10 | 48 s | [sombre](bout_08_001182_trois_velos_55_56_57/bout_08_001182_trois_velos_55_56_57_hgp_hdbscan_k10_sombre.mp4) · [clair](bout_08_001182_trois_velos_55_56_57/bout_08_001182_trois_velos_55_56_57_hgp_hdbscan_k10_clair.mp4) | HGP : A, B et C retrouvés, encore séparés ; HDBSCAN : A, B et C déjà réunis |
| [08/002776](bout_08_002776_deux_velos_17_64/README.md) | 5 | 38 s | [sombre](bout_08_002776_deux_velos_17_64/bout_08_002776_deux_velos_17_64_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_08_002776_deux_velos_17_64/bout_08_002776_deux_velos_17_64_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [08/002852](bout_08_002852_deux_velos_6_51/README.md) | 5 | 38 s | [sombre](bout_08_002852_deux_velos_6_51/bout_08_002852_deux_velos_6_51_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_08_002852_deux_velos_6_51/bout_08_002852_deux_velos_6_51_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |
| [10/000424](bout_10_000424_deux_velos_2_3/README.md) | 5 | 38 s | [sombre](bout_10_000424_deux_velos_2_3/bout_10_000424_deux_velos_2_3_hgp_hdbscan_k5_sombre.mp4) · [clair](bout_10_000424_deux_velos_2_3/bout_10_000424_deux_velos_2_3_hgp_hdbscan_k5_clair.mp4) | HGP : A et B retrouvés, encore séparés ; HDBSCAN : A et B déjà réunis |

<!-- video:fin -->

Critères, mesure et légende des images : [README de `demos/`](../README.md).
