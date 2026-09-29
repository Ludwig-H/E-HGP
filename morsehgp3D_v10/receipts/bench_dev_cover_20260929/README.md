# Reçu : entrée des points par première couverture, graines de DÉVELOPPEMENT (29 septembre 2026)

Graines de l'espace `dev` seulement, et un prototype qui n'est pas encore dans le code du dépôt. Ce reçu motive une
tête ; il ne revendique rien (`public_status=not_claimed`).

## Ce qui change

La tête v10 du 28 septembre faisait entrer chaque point x dans la hiérarchie à son propre rayon K-NN d_K(x), dans la
composante de L_K qui le contient : c'est la sémantique des **cœurs** (C∩X). Le théorème 2 de la thèse et son
chapitre 7 conduisent à une autre sémantique, celle des amas **discrets** : x appartient à une composante C de
L_K(r) dès que d(x, C) ≤ r.

Le point entre donc à sa **première couverture** α_K(x), le plus petit rayon d'une boule fermée contenant x et au moins
K−1 autres points, avec d_K(x)/2 ≤ α_K(x) ≤ d_K(x). Il entre dans la composante de cette boule. La thèse explique par
là le mauvais rappel des cœurs (tableau 7.1, vitesse de percolation en 3D à K = 3 : 0,41 pour les cœurs contre 0,69
pour les polyèdres).

Réalisation exacte, dans le prototype `cover_prototype.patch` appliqué à `05870c4b1` :

- la première boule couvrante de x est la première boule du catalogue, dans l'ordre canonique donc par niveau
  croissant, dont la boule fermée contient x et au moins K sites ;
- toutes les K-parties d'une boule fermée contiennent son centre dans leur région témoin, donc une seule résolution
  par boule donne la composante qui couvre chacun de ses points (table boule → nœud) ;
- variante `--label=vote` : l'étiquette est celle de l'amas retenu qui couvre le point par sa boule de plus bas
  niveau ;
- à K = 1, α = 0 et la sémantique coïncide avec C∩X.

Contrôle logiciel (`cover_check.py`) : le niveau d'entrée vaut exactement α²_K(x), calculé par force brute en
`Fraction` sur des K-parties (192 points, K = 2, 3, 4, 0 écart). C'est un test du code, pas un oracle du clustering.

## Plan

- 256 scènes `dev` : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {2 000 ; 8 000} × 2 graines. ARI_s.
- mcs = √n des deux côtés.
- Politiques de bruit communes : `none`, `full`, b(1,5), b(2), b(2,5), b(3).
- Tour : EOM à z = 1, EOM à z = ẑ, feuilles. sklearn à `min_samples` = K : EOM ou feuilles, α ∈ {1, 2}.
- Binaire du prototype figé, sha256 `ef5f6f65…` (`frozen2`) ; C∩X et sklearn sont repris des reçus
  `bench_dev_kmatch_20260928` (K = 1, 2, 3) et `kcover_dev_k5cap` (K = 5, ce reçu).

## Résultats

J = moyenne d'ARI_s pondérée par cellule. On retient la meilleure configuration (tête, politique) de chaque méthode à
chaque K.

| K | Tour C∩X | Tour, première couverture | sklearn, `min_samples` = K | Couverture − sklearn |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0,7764 | = C∩X | 0,7150 | +0,061 |
| 2 | 0,7640 | 0,7763 | 0,7261 | +0,050 |
| 3 | 0,7525 | **0,7831** | 0,7479 | +0,035 |
| 5 | 0,7591 | 0,7782 | 0,7651 | +0,013 |

- À tête et politique égales, la couverture apporte +0,05 à +0,10 sans remplissage et +0,02 à +0,04 avec b(1,5). La
  meilleure tête reste EOM à z = ẑ.
- La sélection par feuilles se dégrade avec la couverture (0,67 à 0,68). Les points entrent plus tôt, donc plus de
  petites feuilles atteignent le seuil.
- Le vote de couverture égale à peu près l'étiquette par l'arbre suivie de b(1,5), à 0,002 près.
- La meilleure tour sur dev (K = 3, couverture, ẑ-EOM, b(1,5) : 0,7831) dépasse aussi le meilleur sklearn réglé
  librement jusqu'à `min_samples` = 20 (0,7808, reçu `bench_dev_fill_20260928`).
- Par famille, contre sklearn à K = 5 (feuilles, α = 2) :
  - la tour domine `shells` (0,962 contre 0,775), `unbalanced` (0,772 contre 0,676) et `heteroscedastic` ;
  - elle reste derrière sur `filaments` (0,705 contre 0,790) et `anisotropic` (0,833 contre 0,883).

## Lecture

- La sémantique de couverture corrige un défaut réel de la tête v10 : l'optimum n'est plus bloqué à K = 1.
- Il reste deux leviers ouverts : le choix de K, et les familles allongées, où sklearn à grand `min_samples` avec
  feuilles reste meilleur.
- Prochaines étapes : porter le mode couverture dans le code du dépôt, avec ses portes ; mesurer K = 8 et 10 et
  n = 16 000 et 32 000 sur G4 ; préenregistrer une campagne de test sur un nouvel espace de graines, puisque la tête a
  changé (EVAL_v2 D11).
