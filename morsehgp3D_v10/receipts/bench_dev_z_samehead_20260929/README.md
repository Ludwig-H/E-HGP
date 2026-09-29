# Reçu dev : exposant z de l'EOM, même tête sur la tour et sur HDBSCAN, entrée à α_{K+1} (29 septembre 2026)

Graines de l'espace `dev` seulement : 256 scènes, 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {2 000 ; 8 000} × 2
graines. J = moyenne d'ARI_s pondérée par cellule ; mcs = round(√n). `public_status=not_claimed`. Ce reçu motive la
tête v10-b et l'audit du 29 septembre ; il ne revendique rien.

## Fichiers et binaires

- Tour en entrée `cover`, têtes EOM à z ∈ {1, ẑ, 2, 3, 4} : `kcover_dev_zgrid.csv.gz` ; z ∈ {5, 6, 8} :
  `kcover_dev_zgrid2.csv.gz`. Binaire figé `8b8d66f6e`.
- Entrée `cover1` (boule couvrante de poids ≥ K + 1, entrée à α_{K+1}, sémantique de HGP-old), z ∈ {1, ẑ, 3, 4} :
  `kcover_dev_extra1.csv.gz`. Binaire de développement avec `--cover-extra`, dont le code est commité avec ce
  reçu.
- Même tête sur les hiérarchies d'atteignabilité mutuelle MR₁ et MR₂ (l'objet d'HDBSCAN, α = 1 et 2) :
  `samehead_dev.csv.gz`, script `samehead_dev.py`. On utilise le témoin `mhgp10_mreach_cluster`, qui reproduit
  sklearn aux égalités de plateau près (ARI de 0,99 à 1,00 sur 32 contrôles). Têtes : EOM à z ∈ {1, 3, 4}, et
  feuilles.
- Deux scènes dev à K = 8 manquaient au reçu `bench_dev_cover_20260929` : ce sont les refus `rank_order` d'avant le
  correctif `2d8be1bfc`. Elles sont recalculées dans `kcover_dev_k8cap_missing.csv.gz` (`rerun_k8_missing.py`).
- sha256 des binaires : `binaries.sha256`. Journaux : `*.log`, 0 échec partout.
- `condense_dev.py` : condensation Python d'un arbre exporté, outil de diagnostic. `make_prereg_v10b.py` : brouillon
  du générateur de préenregistrement du lot C.

## 1. Exposant z

Meilleure politique de bruit par tête :

| K | z = 1 | ẑ | z = 2 | z = 3 | z = 4 | z = 5 | z = 6 | z = 8 | sklearn |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0,715 | 0,776 | 0,759 | 0,776 | 0,781 | 0,780 | 0,778 | 0,756 | 0,715 |
| 2 | 0,720 | 0,776 | 0,756 | 0,775 | 0,784 | 0,782 | 0,780 | 0,762 | 0,726 |
| 3 | 0,723 | 0,783 | 0,762 | 0,783 | 0,782 | 0,782 | 0,781 | 0,770 | 0,748 |
| 5 | 0,718 | 0,778 | 0,762 | 0,779 | 0,783 | 0,783 | 0,783 | 0,769 | 0,765 |
| 8 | 0,671 | 0,778 | 0,758 | 0,784 | 0,787 | 0,785 | 0,785 | 0,766 | 0,777 |
| 10 | 0,646 | 0,755 | 0,733 | 0,779 | 0,786 | 0,787 | 0,782 | 0,761 | 0,782 |

- Il existe un plateau pour z entre 3 et 5 ; z = 8 retombe vers les feuilles. Sans remplissage, z = 3 et z = 4 sont
  à 0,003 près à tous les K.
- ẑ (Levina–Bickel global) vaut environ 3 sur sept familles et 2,1 sur `shells`. C'est pourquoi il suit z = 3
  jusqu'à K = 5 et décroche à K = 10 : EOM y retient 3 parents au lieu des 8 coquilles.
- z = 3 est l'échelle de la densité K-NN ambiante, λ ∝ K / (n r³). La documentation antérieure (« t^(−p) moins bon
  que ẑ ») est contredite par cette grille.
- **À K = 1, la tour avec z = 1 et b(1,5) donne exactement le score de sklearn (0,7150)** : les hiérarchies
  coïncident, et tout l'écart à K = 1 vient de z.

## 2. Même tête sur la tour et sur HDBSCAN

J, tête EOM à z = 3, sans remplissage. On compare ainsi les hiérarchies à tête égale :

| K | Tour (`cover`) | MR₁ (α = 1) | MR₂ (α = 2) | Tour − MR₁ |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0,739 | 0,739 | 0,739 | 0 |
| 2 | 0,744 | 0,739 | 0,689 | +0,005 |
| 3 | 0,754 | 0,723 | 0,668 | +0,031 |
| 5 | 0,758 | 0,705 | 0,648 | +0,053 |
| 8 | 0,767 | 0,692 | 0,606 | +0,075 |
| 10 | 0,765 | 0,688 | 0,546 | +0,077 |

- À K = 1, les trois hiérarchies donnent des partitions identiques pour toutes les têtes : c'est un contrôle des
  outils.
- À tête EOM égale (z = 1, 3 ou 4), la hiérarchie de la tour avec l'entrée des amas discrets bat MR₁ et MR₂ à
  K ≥ 2, avec un écart qui croît avec K.
- Avec les feuilles, l'ordre s'inverse : à K = 10 avec b(1,5), 0,644 contre 0,745 et 0,758. En entrée `cover`,
  chaque point est porté par une naissance plus légère que mcs, et la lignée multiplie les petites feuilles.
- Le remplissage b(ρ) réduit l'écart : il se calibre sur les membres de chaque méthode.

## 3. Entrée à α_{K+1} (HGP-old)

- `cover1` contre `cover` : écarts de ±0,0015 dès z ≥ 3, à tous les K.
- L'entrée plus stricte de HGP-old n'aide qu'à z = 1 (+0,006 à K = 10).

## 4. Sans remplissage

Écarts dev de la meilleure tête sans remplissage de chaque méthode (tour contre sklearn à `min_samples` = K) :

| K | 1 | 2 | 3 | 5 | 8 | 10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Écart | +0,042 | +0,047 | +0,069 | +0,085 | +0,115 | +0,115 |

Avec remplissage, ces écarts tombent à +0,065, +0,057, +0,035, +0,018, +0,010 et +0,004. La politique b(ρ) fait
l'essentiel du score de sklearn en sélection par feuilles avec α = 2.
