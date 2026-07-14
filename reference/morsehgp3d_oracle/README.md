# Oracle exhaustif CPU de phase 1

Ce paquet constitue une voie de référence indépendante du backend de production. Il énumère directement tous les sous-ensembles de points nécessaires aux objets de phase 1 et n'importe aucun code CUDA ou MorseHGP3D de production.

## Complétude sans cascade de cellules

En dimension 3, tout support minimal d'une miniball contient au plus quatre points. Le catalogue de référence parcourt donc tous les supports de cardinal 1 à 4, rejette exactement les supports affinement dépendants ou non bien centrés, puis classe tous les points par rapport à chaque boule retenue. Ce parcours exhaustif remplace, pour ce backend seulement, la découverte par raffinement de cellules restreintes.

Par conséquent, aucune cellule enfant canonique ni incidence croisée de cellule n'est requise ou fermée par cette stratégie. Le certificat expose honnêtement `canonical_cells_closed=0`; `canonical_children_complete` et `active_cross_incidences_complete` signifient que tous les objets requis par la stratégie choisie sont clos, soit ici deux ensembles requis vides. Ces booléens sont calculés à partir des compteurs `required` et `closed`, et non affirmés indépendamment.

Cette justification ne s'étend pas au backend de production fondé sur les cellules. Pour ce dernier, les mêmes booléens ne peuvent devenir vrais qu'après fermeture et réconciliation effectives de chaque cellule requise.
