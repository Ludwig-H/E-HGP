# Reçu : politiques de bruit bornées sur dev (28 septembre 2026)

Graines de l'espace `dev` seulement ; l'espace `test` n'a pas servi. Ce reçu a introduit les politiques de bruit
bornées dans les grilles de choix. Le préenregistrement qu'il préparait (tour à K = 1 contre sklearn réglé librement)
a été retiré avant tout gel, sur la directive K = `min_samples` ; le choix final est fait par le panneau apparié
([reçu](../bench_dev_kmatch_20260928/README.md)).

## Rejouer

- Binaires figés construits depuis `git archive 4a3d09d8a` (Release) ; `methods.bounded_fill` et scripts du banc
  au commit de ce reçu.
- `fill_dev.py --build <build> --syn <bench/synthetic> --out fill_dev.csv --jobs 4` : politiques `none`, `full`,
  b(ρ) pour ρ ∈ {1 ; 1,25 ; 1,5 ; 2}, sur cinq configurations des deux méthodes.
- `stage2_dev.py --build <build> --syn <bench/synthetic> --configs stage2_configs.json --out stage2_dev.csv
  --jobs 5` : les 8 meilleures configurations distinctes de chaque méthode (reçu `bench_dev_20260928`), croisées
  avec `none`, `full`, b(1,5), b(2), b(2,5), b(3).
- `make_prereg.py` : choix par la règle commune et écriture d'un préenregistrement (retiré, voir plus haut).
- 256 scènes : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {2 000 ; 8 000} × 2 graines. Métrique ARI_s.

## Remplissage borné

b(ρ) (EVAL_v2 § 2.6) : un point de bruit reçoit l'amas du point classé le plus proche si sa distance-cœur, au
max(K, 5)-ième voisin, ne dépasse pas ρ fois le quantile à 95 % des distances-cœur de cet amas.

| Configuration | none | full | b(1,5) | b(2) |
| --- | ---: | ---: | ---: | ---: |
| tour K = 1, ẑ, EOM, √n | 0,7397 | 0,7444 | 0,7761 | 0,7764 |
| sklearn `min_samples` = 20, feuilles, α = 1, √n | 0,6393 | 0,7444 | 0,7690 | 0,7808 |
| sklearn `min_samples` = 12, feuilles, α = 2, √n | 0,5733 | 0,7410 | 0,7557 | 0,7801 |

Le remplissage borné améliore les deux méthodes d'environ 0,03 à 0,04 et ne change pas leur écart.

## Choix du préenregistrement retiré (règle commune, EVAL_v2 § 7.2)

Argmax de J, la moyenne d'ARI_s pondérée par cellule. Une égalité à 0,002 près se tranche par la configuration la
plus simple : K croissant, puis α = 1, z = 1, puis politique de bruit la plus simple.

- Tour : K = 1, mcs = √n, ẑ, EOM, b(1,5) ; J = 0,7761 (maximum 0,7764 avec b(2)).
- sklearn : `min_samples` = 12, mcs = √n, feuilles, α = 2, b(2) ; J = 0,7801 (maximum 0,7808 avec
  `min_samples` = 20 et α = 1).

La tour part avec 0,004 de retard sur dev quand sklearn choisit librement `min_samples` (12 ou 20).
