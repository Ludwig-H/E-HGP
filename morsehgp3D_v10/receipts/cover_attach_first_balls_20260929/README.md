# Reçu : attache par première couverture, une résolution par première boule couvrante (29 septembre 2026)

`public_status=not_claimed`. Changement d'implémentation sans changement d'objet.

## Ce qui change

L'entrée des points par première couverture (`--entry=cover`, commit `25a6c8fc9`) résolvait toutes les boules du
catalogue de poids au moins K, pour chaque K : 13,3 millions de boules sur une scène de 32 000 points à K ≤ 10. Or
seule la première boule couvrante de chaque site fixe son entrée. Le nouveau code :

1. calcule, en parallèle, la première boule couvrante de chaque site : le plus petit indice, dans l'ordre canonique
   du catalogue, d'une boule de poids au moins K qui le contient (minimum atomique, indépendant de l'ordre des fils) ;
2. ne résout que ces boules-là, au plus n par ordre.

La relation boule → nœud complète, nécessaire au seul vote de couverture, reste calculée quand `--label=vote` la
demande (`TowerParams::ball_nodes`).

## Contrôles

- Différentiel (`run.sh`, `run.log`) contre le binaire `25a6c8fc9` (sha256 dans `binaries.sha256`). Cinq entrées :
  trois synthétiques (8 000 et 16 000 points) et deux secteurs LiDAR. K = 5 et K = 10, 2 fils. Les **10 dumps sont
  identiques** octet pour octet ; leurs sha256 sont dans `dumps.sha256`, les dumps eux-mêmes, de 9 à 530 Mo, ne sont
  pas gardés.
- Porte `mhgp10_regression_batch_equivalence`, nouvelle. Sur deux scènes dev de 2 000 points, 96 contrôles, 0 écart :
  - `--label=vote`, qui résout toutes les boules, donne les mêmes étiquettes d'arbre que le chemin court ;
  - l'appel groupé du banc (un catalogue, `--k-list`, `--entry=core,cover`, `--configs`) égale les appels séparés ;
  - 1 fil égale 4 fils.
- Portes existantes vertes : `mhgp10_points_cover` (α exact contre force brute), `mhgp10_tower_oracle`,
  `mhgp10_regression_level_collision`.

## Temps de la tour (étage `tower_s`, 2 fils, machine chargée par d'autres calculs)

| Entrée | K | Avant (s) | Après (s) |
| --- | ---: | ---: | ---: |
| `syn_filaments_space_x1` (8 000) | 5 | 0,76 | 0,38 |
| `syn_filaments_space_x1` (8 000) | 10 | 10,10 | 3,19 |
| `syn_clusters_density_x2` (16 000) | 5 | 2,67 | 1,20 |
| `syn_clusters_density_x2` (16 000) | 10 | 40,75 | 13,28 |
| `syn_shells_space_x1` (8 000) | 10 | 2,53 | 0,71 |
| `lidar02_quarter_x_nonneg_y_nonneg` | 10 | 2,50 | 0,79 |
| `lidar01_quarter_x_neg_y_neg` | 10 | 3,12 | 1,04 |

Le catalogue n'a pas changé et domine désormais la chaîne en mode couverture comme en mode C∩X.
