# Reçu : sélection et rétrécissement des amas de la tour sur `anisotropic` (dev, 29 septembre 2026)

`public_status=not_claimed`, graines **dev** seulement, calcul local : 128 scènes de 8 000 points (8 familles, 4
niveaux, bruit 0 et 0,1, 2 répliques). Script [`shrink_dev.py`](shrink_dev.py), sortie `shrink_dev_8000.csv.gz`,
lecture [`analyse_shrink.txt`](analyse_shrink.txt). Binaires du worktree à `24ac5fc51`.

## Question

Sur `anisotropic`, la tour domine sklearn sans remplissage (0,727 contre 0,416 au niveau difficile, K = 10), avec le
même nombre d'amas, mais perd après remplissage (`receipts/bench_dev_alloc_20260929`). L'hypothèse était que les
frontières des amas EOM, tracées jusqu'au col, affectent moins bien les points que le Voronoï de cœurs étroits.

## Variantes

Tour à K = 3 et 10, entrées `core` et `cover`, avec quatre sélections :

- EOM, avec le z du lot C ;
- feuilles ;
- EOM rétréci aux 50 % puis aux 70 % des points les plus denses de chaque amas.

Chacune reçoit deux remplissages : le point classé le plus proche (`full`), et b2.5 avec le Q95 de l'amas **avant**
rétrécissement, pour garder une couverture comparable. Référence : sklearn en feuilles, α = 2.

## Résultats (ARI_s moyen, remplissage b2.5)

| K | Variante | Toutes familles | `anisotropic` | `filaments` | `shells` |
| ---: | --- | ---: | ---: | ---: | ---: |
| 10 | tour, cover, EOM (v10-b) | 0,790 | 0,854 | 0,789 | 0,937 |
| 10 | tour, cover, EOM rétréci à 50 % | 0,795 | 0,852 | 0,792 | 0,941 |
| 10 | tour, core, feuilles | 0,752 | 0,887 | 0,835 | 0,604 |
| 10 | sklearn, feuilles, α = 2 | 0,781 | 0,883 | 0,845 | 0,832 |
| 3 | tour, cover, EOM (v10-b) | 0,784 | 0,857 | 0,726 | 0,965 |
| 3 | tour, cover, EOM rétréci à 50 % | 0,788 | 0,856 | 0,729 | 0,967 |
| 3 | tour, core, feuilles | 0,726 | 0,885 | 0,722 | 0,522 |
| 3 | sklearn, feuilles, α = 2 | 0,731 | 0,881 | 0,739 | 0,582 |

## Lecture

- **L'hypothèse est réfutée.** Rétrécir les amas EOM à leurs cœurs, puis les remplir, ne répare pas `anisotropic`
  (0,852 à 0,859 contre 0,854). Le gain moyen reste faible (+0,004 à +0,006), sous la marge.
- **Le levier est la sélection, pas l'objet.** La hiérarchie de la tour, avec sélection en feuilles, égale sklearn sur
  `anisotropic` (0,887 contre 0,883) et s'en approche sur `filaments`. Mais les feuilles s'effondrent sur `shells`
  (0,60 contre 0,94 en EOM).
- **Chaque famille veut sa sélection** : les feuilles pour `anisotropic` et `filaments`, EOM pour `shells`. La
  sélection par prominence, feuilles aux séparations significatives à la ToMATo, a déjà été essayée
  (`audits/tete_multik_20260929`) : à 0,003 de v10-b. Aucune règle fixe essayée ne prend le meilleur des deux.
