# Reçu dev : objet exact contre hiérarchie d'HDBSCAN, et plafond de Bayes des familles gaussiennes (29 septembre 2026)

Graines de l'espace `dev` seulement. `public_status=not_claimed`. Ce reçu sert de base à la famille « objet » du
préenregistrement `PREREG_V10_COVER_C_20260929`, et il répond à une objection de l'utilisateur : « la tour est
censée identifier exactement les niveaux de densité K-NN ; si le mélange gaussien est séparable, on devrait les
identifier ».

## 1. Même entrée et même tête : la tour contre MR-bord

`samehead_border_dev.csv.gz` (z ∈ {1, 3, 4}, 256 scènes) et `samehead_border_dev_z56.csv.gz` (z ∈ {5, 6},
n = 8 000). MR_α-bord désigne la hiérarchie d'atteignabilité mutuelle d'HDBSCAN (α = 1 ou 2), avec l'entrée des points par la
règle des points-bord de DBSCAN (analogue de l'entrée par première couverture) et la tête v10. Elle est construite
par le témoin `mhgp10_mreach_cluster --entry=border` (commit `c764e121a`).

J sur les 128 scènes de 8 000 points, EOM, remplissage b(1,5) ; les valeurs sont données dans l'ordre tour, MR₁-bord,
MR₂-bord :

| K | z = 1 | z = 3 | z = 4 | z = 5 | z = 6 |
| ---: | --- | --- | --- | --- | --- |
| 2 | 0,715 / 0,721 / 0,739 | 0,769 / 0,775 / 0,782 | 0,784 / 0,781 / 0,784 | 0,783 / 0,784 / 0,787 | 0,784 / 0,785 / 0,779 |
| 3 | 0,719 / 0,721 / 0,714 | 0,779 / 0,775 / 0,765 | 0,782 / 0,781 / 0,780 | 0,786 / 0,784 / 0,786 | 0,787 / 0,785 / 0,789 |
| 5 | 0,710 / 0,721 / 0,695 | 0,769 / 0,775 / 0,781 | 0,782 / 0,786 / 0,789 | 0,790 / 0,786 / 0,791 | 0,794 / 0,791 / 0,790 |
| 8 | 0,631 / 0,734 / 0,649 | 0,778 / 0,769 / 0,765 | 0,786 / 0,783 / 0,795 | 0,790 / 0,785 / 0,792 | 0,795 / 0,793 / 0,787 |
| 10 | 0,630 / 0,729 / 0,647 | 0,771 / 0,777 / 0,718 | 0,785 / 0,783 / 0,762 | 0,791 / 0,794 / 0,789 | 0,794 / 0,797 / 0,787 |

- Dès z ≥ 4, les trois hiérarchies sont à 0,01 près. Aux têtes retenues pour le lot C, l'écart tour − MR₂-bord vaut
  de +0,001 à +0,008.
- À z = 1, MR₁-bord devance nettement la tour à K = 8 et 10.
- Lecture, cohérente avec l'audit du 29 septembre : à entrée et tête égales, l'objet exact n'apporte rien de mesurable
  sur dev. L'avantage de la tour sur HDBSCAN vient de l'entrée des amas discrets et de la tête, applicables aussi à la
  hiérarchie d'HDBSCAN. Le lot C le mettra à l'épreuve sur 960 scènes de test.

## 2. Plafond de Bayes des familles gaussiennes

`gauss_ceiling_dev.csv.gz`, script `gauss_ceiling_dev.py`. 128 scènes dev : `spherical`, `anisotropic`,
`heteroscedastic` et `unbalanced`, n = 2 000 et 8 000, bruit 0 et 0,1.

Le plafond est la partition MAP du mélange, avec les moyennes, les covariances et les poids estimés sur la vérité, et
un bruit uniforme sur la boîte du générateur. La meilleure coupe est la meilleure coupe horizontale de la
hiérarchie, les composantes de moins de mcs points comptant comme bruit.

Colonnes du tableau (K = 10) :
- **Plafond** : partition MAP du mélange.
- **Coupe** : meilleure coupe horizontale de la hiérarchie de la tour.
- **EOM** : tête v10, z = 3, sans remplissage.
- **EOM b(1,5)** : la même tête, avec le remplissage borné.
- **MR₂-bord coupe**, **MR₂-bord b(1,5)** : les mêmes mesures sur la hiérarchie d'HDBSCAN (α = 2), en entrée bord.
- **MR₁ b(1,5)** : hiérarchie d'HDBSCAN à α = 1, entrée cœur.

| Famille | Niveau | Plafond | Coupe | EOM | EOM b(1,5) | MR₂-bord coupe | MR₂-bord b(1,5) | MR₁ b(1,5) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| spherical | easy | 0,982 | 0,982 | 0,965 | 0,961 | 0,982 | 0,964 | 0,975 |
| spherical | medium | 0,965 | 0,925 | 0,941 | 0,946 | 0,941 | 0,949 | 0,927 |
| spherical | hard | 0,943 | 0,813 | 0,880 | 0,920 | 0,854 | 0,924 | 0,868 |
| spherical | extreme | 0,897 | 0,639 | 0,752 | 0,846 | 0,666 | 0,848 | 0,181 |
| anisotropic | easy | 0,993 | 0,969 | 0,964 | 0,961 | 0,975 | 0,969 | 0,976 |
| anisotropic | medium | 0,974 | 0,851 | 0,911 | 0,932 | 0,873 | 0,933 | 0,900 |
| anisotropic | hard | 0,940 | 0,673 | 0,775 | 0,842 | 0,693 | 0,829 | 0,732 |
| anisotropic | extreme | 0,888 | 0,528 | 0,609 | 0,676 | 0,561 | 0,679 | 0,575 |
| heteroscedastic | easy | 0,989 | 0,984 | 0,962 | 0,955 | 0,984 | 0,956 | 0,977 |
| heteroscedastic | medium | 0,896 | 0,808 | 0,810 | 0,815 | 0,809 | 0,803 | 0,817 |
| heteroscedastic | hard | 0,778 | 0,615 | 0,638 | 0,643 | 0,605 | 0,628 | 0,623 |
| heteroscedastic | extreme | 0,515 | 0,413 | 0,403 | 0,334 | 0,417 | 0,362 | 0,330 |
| unbalanced | easy | 0,983 | 0,981 | 0,971 | 0,963 | 0,980 | 0,963 | 0,939 |
| unbalanced | medium | 0,951 | 0,904 | 0,917 | 0,908 | 0,911 | 0,907 | 0,864 |
| unbalanced | hard | 0,851 | 0,655 | 0,661 | 0,675 | 0,691 | 0,681 | 0,574 |
| unbalanced | extreme | 0,748 | 0,529 | 0,506 | 0,586 | 0,567 | 0,592 | 0,518 |

**Lecture.**

- **Les modes sont identifiés.** Aux niveaux faciles, la meilleure coupe de la tour atteint le plafond de Bayes
  (0,982 contre 0,982 sur `spherical`). La hiérarchie contient alors la partition optimale.
- **L'écart au plafond aux niveaux difficiles vient de la notion d'ensemble de niveau, pas de l'objet.** Pour séparer
  deux modes, il faut couper au-dessus du col. Sur `spherical` hard, le col est à 14 % du pic, et une gaussienne 3D
  a environ 27 % de sa masse sous ce seuil. La partition de Bayes, elle, affecte ces points de queue. Aucun ensemble
  de niveau ne les contient ; il faut une règle d'affectation, et le remplissage borné en est une : il ramène
  `spherical` et `anisotropic` à 0,02–0,05 du plafond, jusqu'au niveau hard.
- **La tour exacte et MR₂-bord se valent**, ici aussi : leurs meilleures coupes et leurs têtes sont à 0,01–0,04 près,
  tantôt en faveur de l'une, tantôt de l'autre. MR₁ en entrée cœur s'effondre sur `spherical` extreme (0,181).
- **La tête perd sur la meilleure coupe aux niveaux faciles** (0,965 contre 0,982) : c'est une erreur de sélection de
  l'EOM, pas un défaut de la hiérarchie.
