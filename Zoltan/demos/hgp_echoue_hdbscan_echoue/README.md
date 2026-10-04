# HGP et HDBSCAN échouent

À un ordre k au moins, les deux hiérarchies manquent un objet, et aucune ne réussit seule à un autre ordre.

## Démos de scène entière

| démo | trame | objets suivis | issue par ordre (k = 2 / 3 / 5 / 10) | pire objet HDBSCAN / HGP à k = 5 |
| --- | --- | --- | --- | --- |
| [Vélos garés en rang](01_velos_en_rang/README.md) | 08/001176 | quatre vélos garés en deux paires ; trois suivis | les deux échouent / les deux échouent / les deux échouent / les deux échouent | 0,41 / 0,42 |
| [Vélos contre une façade](02_velos_contre_facade/README.md) | 08/000882 | trois vélos contre un mur | les deux échouent / les deux échouent / les deux échouent / les deux échouent | 0,18 / 0,30 |
| [Piéton près d'une façade](03_pieton_contre_facade/README.md) | 08/000048 | un piéton près d'une façade et d'un groupe, un piéton isolé, un vélo | les deux échouent / les deux échouent / les deux échouent / les deux échouent | 0,44 / 0,46 |
| [Vélos en rang, sol conservé](04_velos_en_rang_avec_sol/README.md) | 08/001176 | les trois vélos de 01 | les deux échouent / les deux échouent / les deux échouent / les deux échouent | 0,22 / 0,30 |

## Bouts de scène

Parmi les 360 bouts mesurés, 11 tombent dans cette catégorie (9 de vélos, 2 de vélos et piétons). Chacun a un sous-dossier, à raison d'un par trame et par famille (le plus grand groupe d'abord).

| exemple | trame | objets (points) | écart | ordres concernés | k montré : pire objet HDBSCAN / HGP | IoU moyen HDBSCAN / HGP |
| --- | --- | --- | --- | --- | --- | --- |
| [Deux vélos](bout_00_001300_deux_velos_53_67/README.md) | 00/001300 | A vélo (56), B vélo (240) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,36 / 0,36 | 0,65 / 0,61 |
| [Un piéton et deux vélos](bout_02_001606_pieton_deux_velos_3_5_5/README.md) | 02/001606 | A vélo (138), B vélo (309), C piéton (778) | 0,02 m | 2, 3, 5, 10 | k = 5 : 0,39 / 0,40 | 0,65 / 0,67 |
| [Trois vélos](bout_06_000016_trois_velos_10_17_18/README.md) | 06/000016 | A vélo (83), B vélo (87), C vélo (50) | 0,04 m | 2, 3, 5, 10 | k = 5 : 0,40 / 0,40 | 0,60 / 0,57 |
| [Trois vélos](bout_08_000882_trois_velos_38_58_59/README.md) | 08/000882 | A vélo (52), B vélo (98), C vélo (147) | 0,09 m | 2, 3, 5, 10 | k = 5 : 0,45 / 0,40 | 0,71 / 0,69 |
| [Trois vélos](bout_08_001170_trois_velos_43_55_56/README.md) | 08/001170 | A vélo (141), B vélo (60), C vélo (142) | 0,07 m | 2, 3, 5, 10 | k = 5 : 0,30 / 0,30 | 0,67 / 0,69 |
| [Deux vélos](bout_10_000430_deux_velos_2_3/README.md) | 10/000430 | A vélo (144), B vélo (86) | 0,11 m | 2, 3, 5, 10 | k = 5 : 0,38 / 0,38 | 0,54 / 0,55 |

Critères, mesure et légende des images : [README de `demos/`](../README.md).
