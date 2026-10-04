# HGP et HDBSCAN réussissent

À tous les ordres mesurés (k = 2, 3, 5, 10), les deux hiérarchies contiennent un groupe qui recouvre chaque objet à plus de la moitié.

## Démos de scène entière

| démo | trame | objets suivis | issue par ordre (k = 2 / 3 / 5 / 10) | pire objet HDBSCAN / HGP à k = 5 |
| --- | --- | --- | --- | --- |
| [Témoin](05_temoin_voitures_en_file/README.md) | 08/002554 | trois voitures garées à 0,7–1,2 m l'une de l'autre | les deux réussissent / les deux réussissent / les deux réussissent / les deux réussissent | 0,86 / 0,90 |

## Bouts de scène

Parmi les 360 bouts mesurés, 333 tombent dans cette catégorie (58 de vélos, 12 de vélos et piétons, 263 de voitures). Seuls des représentants ont un sous-dossier : les objets les plus serrés de chaque famille et la file de trois voitures de la démo 05, isolée. La liste complète est dans [`../bouts_evalues.json`](../bouts_evalues.json).

| exemple | trame | objets (points) | écart | ordres concernés | k montré : pire objet HDBSCAN / HGP | IoU moyen HDBSCAN / HGP |
| --- | --- | --- | --- | --- | --- | --- |
| [Trois voitures](bout_00_000600_trois_voitures_288_508_509/README.md) | 00/000600 | A voiture (1872), B voiture (3257), C voiture (2515) | 0,03 m | 2, 3, 5, 10 | k = 5 : 1,00 / 0,97 | 1,00 / 0,99 |
| [Deux voitures](bout_00_002770_deux_voitures_193_525/README.md) | 00/002770 | A voiture (3651), B voiture (4139) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,98 / 1,00 | 0,99 / 1,00 |
| [Deux voitures](bout_00_003630_deux_voitures_323_325/README.md) | 00/003630 | A voiture (864), B voiture (1286) | 0,03 m | 2, 3, 5, 10 | k = 5 : 0,95 / 0,95 | 0,97 / 0,96 |
| [Deux voitures](bout_02_000530_deux_voitures_259_260/README.md) | 02/000530 | A voiture (716), B voiture (2338) | 0,01 m | 2, 3, 5, 10 | k = 5 : 0,50 / 0,50 | 0,64 / 0,64 |
| [Deux voitures](bout_02_000550_deux_voitures_51_257/README.md) | 02/000550 | A voiture (317), B voiture (793) | 0,03 m | 2, 3, 5, 10 | k = 5 : 0,99 / 0,99 | 0,99 / 0,99 |
| [Un piéton et un vélo](bout_02_001604_pieton_velo_5_5/README.md) | 02/001604 | A vélo (356), B piéton (853) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,99 / 1,00 | 1,00 / 1,00 |
| [Deux vélos](bout_02_001610_deux_velos_3_5/README.md) | 02/001610 | A vélo (342), B vélo (397) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,90 / 0,93 | 0,93 / 0,96 |
| [Un piéton et deux vélos](bout_02_001610_pieton_deux_velos_3_5_5/README.md) | 02/001610 | A vélo (342), B vélo (397), C piéton (653) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,75 / 0,76 | 0,89 / 0,90 |
| [Trois vélos](bout_08_001182_trois_velos_43_56_57/README.md) | 08/001182 | A vélo (300), B vélo (132), C vélo (444) | 0,03 m | 2, 3, 5, 10 | k = 5 : 0,52 / 0,58 | 0,80 / 0,82 |
| [Trois voitures](bout_08_002560_trois_voitures_169_170_171/README.md) | 08/002560 | A voiture (2969), B voiture (3427), C voiture (2491) | 0,57 m | 2, 3, 5, 10 | k = 5 : 0,99 / 0,99 | 1,00 / 1,00 |
| [Deux vélos](bout_08_002696_deux_velos_13_52/README.md) | 08/002696 | A vélo (138), B vélo (169) | 0,04 m | 2, 3, 5, 10 | k = 5 : 0,73 / 0,73 | 0,74 / 0,81 |

<!-- video:début -->
Vidéos HGP contre HDBSCAN, en deux variantes par exemple : [`videos_hgp_hdbscan/`](../videos_hgp_hdbscan/README.md).
<!-- video:fin -->

Critères, mesure et légende des images : [README de `demos/`](../README.md).
