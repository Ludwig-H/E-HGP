# Reçu : choix sur dev du panneau apparié K = min_samples, lot A (28 septembre 2026)

Graines de l'espace `dev` seulement ; l'espace `test` n'a pas servi. Ce reçu est la base du
[préenregistrement](../../bench/synthetic/prereg/PREREG_V10_KMATCH_A_20260928.json) du lot A (K = 1, 2, 3). La
comparaison à K = `min_samples` suit la directive de l'utilisateur du 28 septembre 2026.

## Rejouer

- Binaires figés construits depuis `git archive 4a3d09d8a` (Release), avec les scripts du banc au commit de ce reçu.
- `kmatch_dev.py --build <build> --syn <bench/synthetic> --out kmatch_dev_A.csv --k 1,2,3 --jobs 5`.
- `make_prereg_kmatch.py` : choix par K, puis écriture du préenregistrement avec ses épingles.
- 256 scènes : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {2 000 ; 8 000} × 2 graines. Métrique ARI_s,
  mcs = round(√n) des deux côtés.

## Grilles symétriques

- Tour à l'ordre K : EOM avec λ = r^(−1), EOM avec λ = r^(−ẑ), ou feuilles.
- sklearn HDBSCAN à `min_samples` = K : EOM ou feuilles, α = 1 ou 2, `kd_tree`.
- Politiques de bruit, les mêmes pour les deux méthodes : `none`, `full`, b(1,5), b(2), b(2,5), b(3), avec la
  distance-cœur au max(K, 5)-ième voisin.

## Choix

Pour chaque K, on retient l'argmax de J, la moyenne d'ARI_s pondérée par cellule. Une égalité à 0,002 près se
tranche par l'échelle par défaut, puis la politique la plus simple, puis EOM.

| K | Tour | J | sklearn | J | Écart dev |
| ---: | --- | ---: | --- | ---: | ---: |
| 1 | EOM ẑ, b(1,5) | 0,7761 | EOM α = 1, b(1,5) | 0,7150 | +0,061 |
| 2 | EOM ẑ, b(2) | 0,7640 | feuilles α = 2, b(2) | 0,7261 | +0,038 |
| 3 | EOM ẑ, b(2) | 0,7525 | feuilles α = 2, b(2) | 0,7479 | +0,005 |

**Lecture.**
- À ordre égal, la tête ẑ-EOM de la tour domine à K = 1 et K = 2 ; à K = 3, les deux méthodes sont à égalité.
- Quand K augmente, sklearn rattrape son retard par la sélection par feuilles avec α = 2.
- À K = 1, les hiérarchies sont les mêmes (liaison simple) : l'écart y vient de la tête.
