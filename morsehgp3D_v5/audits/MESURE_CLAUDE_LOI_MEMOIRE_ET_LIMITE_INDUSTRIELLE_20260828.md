# Mesure Claude — la loi mémoire est linéaire, et elle fixe la limite industrielle (28 août 2026)

Ancrage : lignes `rss_mb` des reçus de production, pin `839cf1ec`, quatre
familles × quatre tailles. Cadre : `phase=exploration_v5_hors_registre`,
`backend=cpu_reference`, `mode=mesure`, `public_status=not_claimed`.

Après la rétractation sur les exposants de temps
(`RETRACTATION_CLAUDE_EXPOSANTS_TERRAIN_20260828.md`), le mur du passage à
l'échelle est la mémoire. Ce document la mesure au lieu de l'estimer.

## 1. La loi est linéaire — c'est ce qui rend l'extrapolation fiable

Exposants locaux du RSS par phase (8 000 → 16 000 → 32 000 → 50 000) :

| famille | après génération | après census | pic du fold | ko par point au pic (50 000) |
|---|---|---|---|---|
| `uniform` | 1,03 / 1,01 / 0,61 | 0,90 / 0,55 / 0,83 | 1,02 / 0,85 / 0,92 | **363,0** |
| `eight_clusters` | 1,04 / 1,05 / 0,68 | 1,07 / 0,31 / 1,07 | 1,07 / 0,90 / 0,93 | **333,3** |
| `terrain` | 1,07 / 1,08 / 0,90 | 0,99 / 1,00 / 1,10 | 0,98 / 0,87 / 1,08 | **78,6** |
| `scanline_single_pass` | 1,08 / 0,98 / 1,00 | 0,97 / 0,95 / 1,10 | 0,89 / 0,88 / 1,07 | **73,9** |

Tous les exposants sont dans $[0{,}31 ; 1{,}10]$ et la grande majorité autour
de 1 : **la mémoire croît linéairement en $n$**, sans dérive. C'est le seul
endroit de ce projet où une extrapolation à 10 M repose sur une pente stable
plutôt que sur une tendance.

## 2. Ce que cela donne à 10 M et 30 M, et ce que la VM peut

La VM gardée est un `g4-standard-48` avec **180 Gio** de RAM.

| famille | 10 M | 30 M | **taille maximale résidente sur 180 Gio** |
|---|---|---|---|
| `uniform` | 3,63 To | 10,9 To | **≈ 0,5 M points** |
| `eight_clusters` | 3,33 To | 10,0 To | ≈ 0,54 M points |
| `terrain` | 786 Go | 2,36 To | **≈ 2,3 M points** |
| `scanline_single_pass` | 739 Go | 2,22 To | ≈ 2,4 M points |

**Voilà la limite industrielle d'aujourd'hui, mesurée : 2,3 à 2,4 millions de
points pour un nuage de type LiDAR, 0,5 million pour `uniform`.** L'objectif
est de 10 à 30 M : il manque donc un facteur **4 à 13** sur les familles
réalistes, et **20 à 60** sur `uniform`.

## 3. Le fold ne suffit pas — l'amont doit être streamé aussi

Décomposition du pic à 50 000 points :

| famille | après census | pic du fold | part propre au fold |
|---|---|---|---|
| `uniform` | 7 610 Mo | 17 726 Mo | 10 116 Mo (**57 %**) |
| `eight_clusters` | 7 048 Mo | 16 272 Mo | 9 224 Mo (57 %) |
| `terrain` | 2 598 Mo | 3 836 Mo | 1 238 Mo (**32 %**) |
| `scanline_single_pass` | 2 468 Mo | 3 606 Mo | 1 138 Mo (32 %) |

Conséquence directe : **même en ramenant à zéro la mémoire du fold**, il
resterait 2 468 Mo à 50 000 points sur `scanline`, soit 50,5 ko par point,
soit **505 Go à 10 M** — encore 2,8 fois la RAM de la VM. Le réducteur vivant
(L2) attaque 32 % du pic sur les familles réalistes et 57 % sur les autres ;
il est nécessaire et **notoirement insuffisant seul**. L'amont — boules,
census, événements — doit être streamé au même titre : c'est L4 de
`docs/ECHELLE.md`, et cette mesure en fait une condition, pas une option.

## 4. Ce que « industriel » veut dire, chiffré

Pour tenir 10 M sur cette VM sans disque, il faut passer de 73,9 ko à
**18,4 ko par point** sur `scanline` (facteur 4,0) et de 363 ko à **18,4 ko**
sur `uniform` (facteur 19,7). Avec le disque prévu par `ECHELLE.md`
(Hyperdisk 100 Go à 290 Mio/s, extensible), la contrainte devient un débit :
739 Go d'état à écrire et relire au moins une fois, soit ≈ 1,4 h de disque
incompressible à 10 M sur `scanline` — à comparer aux ≈ 1,3 min de calcul de
génération extrapolés. **À 10 M, le passage à l'échelle est un problème
d'entrées-sorties, pas de calcul.**

## 5. Réserves

- Le RSS est un pic de processus : il inclut l'allocateur et ne se décompose
  pas en postes exacts. Les « parts propres au fold » sont des différences de
  paliers, pas une comptabilité.
- Les quatre tailles viennent de deux binaires (`mhgp5_conformity_v4` pour
  8/16/32 k, `mhgp5_probe` pour 50 k) : la série appariée reste à faire.
- Les tailles maximales résidentes supposent que la loi linéaire tient
  jusqu'à 2 M points, ce qui n'est mesuré nulle part au-delà de 50 000.
- Le débit disque de 290 Mio/s est celui d'un Hyperdisk Balanced de 100 Go ;
  il croît avec la taille du volume et n'a pas été mesuré à la taille visée.
