# Reçu : sélection par scène et effet de la géométrie, graines de DÉVELOPPEMENT (28 septembre 2026)

Graines de l'espace `dev` seulement ; l'espace `test` n'a pas servi. Ce reçu oriente la conception et ne
revendique rien (`public_status=not_claimed`).

## Rejouer

- Binaires figés construits depuis `git archive 4a3d09d8a` (Release) ; scripts du même commit.
- `adaptive_dev.py --build <build> --out adaptive_dev_r2_n2000.csv --sizes 2000 --tower-k 1,2,3,4,5,6,8,10
  --hdb-ms 1,2,3,4,5,6,8,10,12,16,20 --alphas 1,2 --jobs 3 --threads 1`.
- `zhead_dev.py --build <build> --out zhead_dev_n2000.csv --jobs 3`. Ce script d'exploration, copié ici, n'est
  pas un outil du dépôt. Il porte en Python la tête C++ (`src/head/head.cpp`) et vérifie qu'il en donne
  exactement les étiquettes (0 écart sur 1 536 comparaisons). Il construit aussi l'atteignabilité mutuelle exacte
  MR_α (α ∈ {1, 2}), avec des poids entiers, des multifusions non binarisées et chaque point né à sa distance-cœur.
- Lecture : `bench/synthetic/analyse_adaptive.py`.
- Plan : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n = 2 000 × 2 graines, soit 128 unités ; mcs = √n ; ARI_s.
- sklearn 1.9.1 et hdbscan (pour DBCV) sur un codespace chargé. Les temps ne comptent pas.

## Résultats (moyenne d'ARI_s sur 128 unités)

**Configuration fixe unique :**

| Méthode | Configuration | ARI_s |
| --- | --- | ---: |
| sklearn HDBSCAN | `min_samples` = 20, feuilles, α = 1, remplissage complet | 0,7542 |
| tour v10 | K = 8, feuilles, remplissage complet | 0,7469 |
| tour v10 | K = 1, ẑ, EOM, sans remplissage | 0,7455 |
| sklearn HDBSCAN, meilleure sans remplissage | `min_samples` = 1, EOM | 0,6954 |

**Choix DBCV par scène :**

| Méthode | Grille | ARI_s |
| --- | --- | ---: |
| tour | grille K ≤ 5 | 0,7467 |
| tour | grille K ≤ 10 | 0,6779 |
| HDBSCAN | `min_samples` ≤ 10, α = 1 | 0,7637 |
| HDBSCAN | `min_samples` ≤ 20, α = 1 | 0,7389 |
| HDBSCAN | `min_samples` ≤ 20, α ∈ {1, 2} | 0,6438 |

DBCV n'est pas fiable sur une grille large. Il choisit sur `shells` des configurations à ARI 0,18–0,30 dès
qu'elle contient de grands K ou α = 2.

**Choix DBCV entre deux configurations cherchées sur dev** (biais de recherche égal pour les deux méthodes) :

| Méthode | Paire | ARI_s |
| --- | --- | ---: |
| tour | {K = 4 EOM z = 1 sans remplissage ; K = 8 feuilles remplissage complet} | 0,7717 |
| HDBSCAN | {`min_samples` = 12 EOM α = 1 sans remplissage ; 20 feuilles α = 1 remplissage complet} | 0,7788 |

**Meilleure configuration par famille** : égalité à 0,003 près sur 7 familles. Sur `hierarchical`, la tour atteint
0,636 contre 0,895 pour HDBSCAN (`min_samples` = 20, EOM, α = 2). Ce réglage de HDBSCAN s'effondre ailleurs :
0,18 sur `shells`, 0,33 sur `spherical`.

## Effet de la géométrie, à tête identique (`zhead_dev`)

- Le témoin MR_α reproduit sklearn à ±0,003 près dans chaque cellule (famille, K, sélection).
- À K égal, la tour et MR_1, c'est-à-dire HDBSCAN standard avec `min_samples` = K, donnent le même clustering.
  Par exemple, sur `hierarchical`, EOM, z = 1 : 0,636 contre 0,636 à K = 10. Sur ce banc, la connexité exacte de la
  multicouverture n'apporte **aucun gain** par rapport à l'atteignabilité mutuelle.
- À K = 1, les trois hiérarchies coïncident (union de boules, liaison simple). La tête ẑ-EOM y donne 0,7455 quelle
  que soit la source : **le gain sur HDBSCAN non rempli est un gain de tête**, disponible aussi sur la liaison simple.
- **L'échelle λ = r^(−z) avec z < 1 est réfutée** : elle dégrade toutes les sources à tout K (tour K = 1,
  EOM sans remplissage : 0,647 à z = 0,25 ; 0,652 à z = 0,5 ; 0,695 à z = 1 ; 0,746 à z = ẑ).

## Lecture

La prévision écrite d'avance (CLUSTER_v2 § 0) est confirmée sur dev : il y a non-infériorité géométrique. Le gain
de la v10 sur HDBSCAN par défaut (0,556 en campagne dev) et sur HDBSCAN non rempli vient de la tête (échelle ẑ,
EOM, mcs = √n, bruit conservé). Réglé et rempli, HDBSCAN garde environ 0,007 d'avance, parce que sa grille monte à
`min_samples` = 20 alors que la tour est bornée à K = 10.
