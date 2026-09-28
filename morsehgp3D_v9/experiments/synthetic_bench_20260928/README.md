# Banc synthetique du 28 septembre 2026

Banc de comparaison a HDBSCAN sur nuages 3D synthetiques, **calibre** : chaque
niveau de difficulte est defini par le score que la reference y obtient, pas
par un parametre geometrique.

`phase=exploration_v9_hors_registre`, `public_status=not_claimed`.

## Pourquoi un nouveau banc

Le banc du 27 septembre mesurait presque tout a une seule difficulte
(separation 4 entre centres gaussiens), ou HDBSCAN plafonne entre 0,07 et 0,28
d'ARI. Deux methodes qui echouent ensemble ne se departagent pas : une marge
mesuree dans ce regime ne dit rien. La calibration du 28 septembre place les
quatre niveaux dans la zone ou la reference passe reellement de la reussite a
l'echec.

## Les huit familles

| Famille | Dimension intrinseque | Ce qu'elle piege |
| --- | --- | --- |
| `spherical` | 3 | cas de reference, gaussiennes isotropes |
| `anisotropic` | 3 | composantes tres allongees, orientations libres |
| `heteroscedastic` | 3 | densites tres inegales entre groupes |
| `unbalanced` | 3 | effectifs dans un rapport 32 |
| `shells` | **2** | spheres creuses : surface a densite uniforme, aucun pic |
| `bridge` | 3 | ponts minces entre groupes : piege du chainage |
| `hierarchical` | 3 | amas d'amas : piege du **niveau**, pas de l'ecart |
| `filaments` | **1** | segments fins qui se croisent, proche du LiDAR |

La colonne « dimension intrinseque » est l'exposant `z` attendu du poids
`psi(t) = 1/t^z` de la tour : un filament vit en dimension un, une surface en
dimension deux, une gaussienne en dimension trois. Le banc n'impose pas `z`,
il publie la valeur attendue pour que le choix d'une campagne se lise a cote
de son score. Pour une comparaison equitable a HDBSCAN, `z = 1`.

## Les quatre niveaux

Un niveau est **defini par sa difficulte mesuree** : ARI de
`HDBSCAN(min_cluster_size=20)` au point de calibration (n = 2000, 8 groupes,
sans bruit, moyenne de cinq graines), vise a 0,95 / 0,70 / 0,40 / 0,15.

| Famille | easy | medium | hard | extreme |
| --- | --- | --- | --- | --- |
| `spherical` | 0,96 | 0,73 | 0,32 | 0,16 |
| `anisotropic` | 0,95 | 0,66 | 0,43 | 0,23 |
| `heteroscedastic` | 0,97 | 0,72 | 0,42 | 0,18 |
| `unbalanced` | 0,96 | 0,70 | 0,40 | 0,16 |
| `shells` | 1,00 | 0,84 | 0,34 | 0,18 |
| `bridge` | 0,96 | 0,74 | 0,46 | 0,15 |
| `hierarchical` | 1,00 | 0,70 | 0,51 | **0,46** |
| `filaments` | 0,95 | 0,75 | 0,34 | 0,14 |

`hierarchical` ne descend pas sous 0,46 et c'est son propos : 0,46 est l'ARI
d'un surdecoupage systematique en trois sous-amas. Franchir ce plancher
demande de choisir le bon **niveau** de la hierarchie, ce qu'une coupe unique
ne peut pas faire. C'est la famille ou une methode hierarchique doit gagner.

La porte `test_levels_match_the_published_calibration` refuse une derive de
plus de 0,15 sur une cellule, et toute inversion entre deux niveaux.

## Deux familles ou la reference se degrade quand on lui donne plus de points

Mesure du 28 septembre, niveau `medium`, 8 groupes, ARI de HDBSCAN :

| Famille | n = 500 | n = 2000 | n = 8000 |
| --- | --- | --- | --- |
| `shells` | 1,00 | 1,00 | **0,18** |
| `hierarchical` | 1,00 | 0,63 | **0,43** |
| `spherical` | 0,33 | 0,74 | 0,78 |
| `filaments` | 0,51 | 0,71 | 0,94 |

Les familles volumiques s'ameliorent avec `n`, comme attendu. Les deux
familles a structure — la surface uniforme et l'amas d'amas — **empirent**.
Une surface a densite uniforme n'a aucun pic de densite : plus il y a de
points, plus `min_cluster_size` fragmente la coquille. C'est le regime LiDAR,
et c'est une propriete mesuree de la reference, pas un defaut du banc.

## Le plan

Un axe varie a la fois autour du point central (spherique, n = 2000,
8 groupes, `medium`, sans bruit), plus deux grilles serrees la ou la
comparaison se joue. Tailles 500 / 2000 / 8000 / 32000, groupes 2 / 4 / 8 /
16 / 32, bruit uniforme 0 / 10 / 30 %, cinq graines partout : **51 scenes,
255 mesures par methode**.

## Les references

- `hdbscan_default` — usage reel, `min_cluster_size = 20` fixe d'avance.
- `hdbscan_oracle` — **le meilleur HDBSCAN par scene**, choisi contre la
  verite terrain sur une grille de huit tailles. C'est une borne superieure
  inatteignable en usage reel, declaree comme telle : la battre est la seule
  preuve qui vaille.
- `single_linkage` — temoin negatif au nombre exact de groupes.

La couverture est publiee a cote de l'ARI, pour qu'un score obtenu en
rejetant la moitie des points se voie.

## Commandes

```bash
python3 -m unittest discover -s . -p 'test_*.py'   # 14 portes, aucune sur assert
python3 run_baselines.py --out baselines.csv       # campagne complete
python3 run_baselines.py --out light.csv --light   # sans les scenes a 32000
```
