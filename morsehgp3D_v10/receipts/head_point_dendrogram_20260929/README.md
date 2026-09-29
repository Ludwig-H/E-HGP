# Reçu : dendrogramme de points de la tête sans tri général (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. Mesures locales (codespace, AMD EPYC 7763) ; la mesure de G4
reste à faire.

## Constat

Sur G4, la tête pesait 0,15 à 0,18 s dans une chaîne complète de 0,38 à 0,48 s à K = 5
(`receipts/g4_session3_j2_20260929`). Le banc [`headbench.cpp`](headbench.cpp) sépare ses étapes sur la trame 02 à
K = 5, entrée `cover` :

| Étape | Temps (s) |
| --- | ---: |
| `point_dendrogram` | 0,272 |
| `validate` | 0,003 |
| `cluster` (condensation, EOM, étiquettes) | 0,018 |

`point_dendrogram` triait les 655 221 clés (609 376 nœuds, 45 845 points) dans l'ordre exact des niveaux. Son
comparateur recalculait, à chaque comparaison, l'approximation double de deux niveaux rationnels I192/I128 : environ
25 millions de conversions.

## Changement (`src/tower/tower.cpp`, `point_dendrogram`)

- Les clés du catalogue (nœuds, et points en entrée `cover`) sont des rangs. Les niveaux du catalogue croissent
  strictement avec le rang, donc un tri par dénombrement des rangs est exact et linéaire.
- Les points en entrée `core` ont des niveaux entiers D_K. Ils sont triés comme entiers, puis fusionnés avec la suite
  du catalogue par comparaison exacte, avec un raccourci double quand les approximations s'écartent de plus de 1e-9
  en relatif, très au-dessus de l'erreur d'arrondi.
- L'approximation d'un niveau du catalogue est calculée une fois par rang.
- La règle de publication des niveaux est inchangée : doubles strictement croissants, rang partagé en cas de
  collision (porte `mhgp10_regression_level_collision`).
- Les clés exactement égales reçoivent le même rang, donc leur ordre relatif ne change pas la sortie.

Une étape intermédiaire, les doubles calculés une fois avec le même tri général, donnait 0,086 s à K = 5 (ventilation
par [`headbench2.cpp`](headbench2.cpp)).

## Résultat (trame 02, 1 fil pour cette étape)

| K | Nœuds | Avant (s) | Après (s) | Gain |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 609 376 | 0,272 | 0,016 | ×17 |
| 10 | 1 555 780 | 0,223 à 0,272 | 0,090 | ×2,5 à ×3 |

À K = 10, il reste le calcul des 1,5 million d'approximations distinctes et le dénombrement sur les rangs.

## Exactitude

- **Différentiel octet pour octet** ([`head_diff.py`](head_diff.py), sortie
  [`differentiel_tete.txt`](differentiel_tete.txt)) : étiquettes et arbre exporté (`--tree` : niveaux en `%.17g`,
  rangs, parents, attaches) du binaire d'origine contre le nouveau. 18 cas, 18 identiques :
  - trames LiDAR entières, scène de la porte de collision de niveaux, scènes `shells` et `filaments` ;
  - K = 1, 2, 3, 5, 8 et 10 ; entrées `core` et `cover`.
- **Portes** : 9 sur 9 ([`ctest_gate.txt`](ctest_gate.txt)), sur l'arbre qui porte aussi la correction du pool
  (`receipts/pool_race_fix_20260929`).
