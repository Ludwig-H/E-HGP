# HGP échoue, HDBSCAN réussit

À un même ordre k au moins, la hiérarchie HGP manque un objet que la hiérarchie de HDBSCAN retrouve, et l'inverse ne se produit à aucun ordre.

## Bouts de scène

Parmi les 360 bouts mesurés, 3 tombent dans cette catégorie (3 de vélos). Chacun a un sous-dossier, à raison d'un par trame et par famille (le plus grand groupe d'abord).

| exemple | trame | objets (points) | écart | ordres concernés | k montré : pire objet HDBSCAN / HGP | IoU moyen HDBSCAN / HGP |
| --- | --- | --- | --- | --- | --- | --- |
| [Deux vélos](bout_06_000016_deux_velos_10_18/README.md) | 06/000016 | A vélo (83), B vélo (50) | 0,12 m | 5 | k = 5 : 0,54 / 0,44 | 0,59 / 0,53 |
| [Trois vélos](bout_06_000774_trois_velos_6_14_15/README.md) | 06/000774 | A vélo (141), B vélo (63), C vélo (86) | 0,03 m | 3 | k = 3 : 0,51 / 0,47 | 0,63 / 0,60 |

<!-- video:début -->
Vidéos HGP contre HDBSCAN, en deux variantes par exemple : [`videos_hgp_hdbscan/`](../videos_hgp_hdbscan/README.md).
<!-- video:fin -->

Critères, mesure et légende des images : [README de `demos/`](../README.md).
