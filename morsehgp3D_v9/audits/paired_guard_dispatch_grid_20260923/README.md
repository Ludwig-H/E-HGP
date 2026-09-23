# Routage avant le cœur et concentration des arêtes S2 LiDAR

23 septembre 2026. Diagnostic CPU **audit-only**, sans modification du produit,
sur la trame brute complète SemanticKITTI 08/000000 à 1 mm/u18, K5/s8. Le
[reçu](RESULT.tsv) parcourt les **3 986 433 arêtes survivantes S2** des huit
traces épinglées. Il calcule avec leurs seules extrémités `D=|b−a|²` et le
nombre de sites dans la cellule de grille contenant le milieu entier
`⌊(a+b)/2⌋` du repère u18 commun, puis compare
ces descripteurs avec la taille `F` du cœur connue **après** sa construction.
Le routage n'utilise jamais `F` : il décide seulement d'essayer un certificat
positif, avec repli exact sous quota. L'occupation de la cellule **n'est pas
un minorant de F** : la cellule peut dépasser le disque diamétral.

Les deux panels de 60 arêtes du
[certificat ponctuel](../paired_guards_precore_20260923/README.md) et du
[sélecteur indexé](../paired_guard_index_shadow_20260923/README.md) sont
stratifiés **a posteriori** selon `F≥1 000`. Leur nombre d'arêtes fermables
retenues décrit seulement ce panel, jamais un taux de succès du flux complet.
`F` comprend les deux extrémités et les formes effectivement calculées par
`load`. Les 91 267 cœurs `F≥1 000` portent 429 563 593 des 559 661 741
sites de cœur cumulés du flux S2 ; cela reste une charge observée, pas une
borne de croissance.

| Crible sur `D` et la cellule 4 096 mm du milieu | Arêtes routées / 3 986 433 | Grosses `F≥1 000` routées / 91 267 | Leur `F` / 429 563 593 | Fermables B16 retenues / 67 du panel |
| --- | ---: | ---: | ---: | ---: |
| `D≥2²²`, cellule non vide | 838 565 | 90 855 | 428 994 946 | 67 |
| `D≥2²³`, cellule non vide | 502 157 | 87 917 | 425 630 327 | 63 |
| `D≥2²³`, occupation ≥256 | 247 738 | 87 175 | 424 616 622 | 62 |
| `D≥2²³`, occupation ≥1 024 | **147 406 (3,70 %)** | **85 023** | **421 519 574** | **61** |
| `D≥2²⁴`, occupation ≥1 024 | 89 822 | 81 863 | 417 549 864 | 55 |

La quatrième ligne contient 93,16 % des grosses arêtes et 98,13 % de
leur masse `F`, mais exige encore **147 406 tentatives** de certificat.
Parmi elles, 85 023 sont grosses ; les autres peuvent aussi coûter cher
au sélecteur. Les 61 fermables et leurs 273 077 `F` sont des preuves du
sélecteur B16 **déjà obtenues sur le panel**, pas des fermetures du crible.
La longueur seule `D≥2²³` aurait routé 572 621 arêtes sur la même trace
([diagnostic](../paired_guard_index_shadow_20260923/TRIGGER_D.json)).
Ce gain de sélectivité n'établit aucun gain de temps : quatre recherches
spatiales et l'appariement par arête restent trop nombreux pour un contrat
de tour rapide. La grille 4 096 mm contient 969 cellules occupées, préparées
en `O(n)` ; un routage à une échelle utilise trois différences et produits
pour `D`, le milieu entier et **une** recherche de cellule. Le programme
de mesure construit et interroge cinq grilles (1 024 à 16 384 mm) afin de
comparer les seuils, donc ses durées ne sont pas celles d'un port à une
échelle. Sur le domaine u18, une table dense `64³` de compteurs u32
occuperait **1 Mio** et remplacerait le hash du shadow par un adressage
direct, piste CPU/GPU à mesurer. Les comptes, pas un chrono extrapolé,
fondent la décision.

## Le groupe dominant rend la mutualisation concrète

Parmi les 147 406 arêtes routées à `D≥2²³` et occupation ≥1 024, seules
**22 cellules de milieu** et **52 paires (cellule, axe dominant)** sont
présentes. Les égalités d'axe sont départagées `x`, puis `y`, puis `z`.
La cellule `(19,18,6)` et l'axe `x` regroupent **67 827 arêtes**,
dont **66 869** grosses portant **367 510 224 `F`** à elles seules,
soit 73,27 % des grosses arêtes, 85,56 % de leur masse `F`, et déjà
65,67 % de **tout** le `F` S2 de cette trame. La tranche
`D≥2²⁴` de ce même groupe comporte **66 779 arêtes**, toutes grosses,
et **367 396 217 `F`** ; `D` va de 16 830 770 à 25 207 726. Les
extrémités réorientées gauche/droite selon `x` sont dans les boîtes u18/mm :

| Facteur | `x` | `y` | `z` |
| --- | ---: | ---: | ---: |
| Gauche | 75 894–78 059 | 74 020–76 651 | 25 996–28 160 |
| Droite | 80 351–81 213 | 74 681–78 957 | 26 746–27 945 |

La largeur de ces boîtes, notamment en `y`, est un obstacle réel à un
certificat uniforme sur tout le groupe. **Partager cellule et axe ne
transfère aucune preuve**. Le [théorème de B sur les rectangles](../CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md)
donne le test exact sur boîtes continues pour une paire de gardes fixe ;
un groupe de cette taille appelle des subdivisions de couples d'extrémités
et des bornes corrélées, puis un repli. Le groupage par cellule peut
simplement garder les arêtes, masques et ordinaux pour essayer cette
hiérarchie ; il ne change ni propriétaire ni catalogue.

Une variante à mesurer est un BVH des **couples** `(a,b)` du groupe, avec
boîtes A/B, masques par arête et ensemble sûr des IDs d'extrémités. Tester
des paires de gardes par blocs de nœuds, subdiviser les couples si besoin,
et arrêter sous quota pour revenir au chemin exact. À K5, trois paires
disjointes ferment q4 et quatre ferment q3 ; une fermeture q4 seule
conserve la voie q3 dans chaque masque. Les nœuds G/H choisis
pour une même preuve doivent former une antichaîne disjointe par voie et
être disjoints des extrémités du bloc, ou ces IDs doivent être soustraits
explicitement avec un appariement fixe. Les plages de rangs suffisent
pour un **rectangle WSPD unique** ; un groupe cellule/axe peut mélanger
des rectangles et exige une union d'IDs ou une autre structure sûre. Ni
la construction du BVH, ni sa mémoire, ni le nombre de visites, ni les
`F` vraiment évités avant `load` ne sont encore mesurés.

Comme préfiltre conservateur avant les 64/4096 coins du théorème B,
pour des boîtes scalaires de `a,b,g,h`, l'identité
`H/4=Σ_i[(g_i−a_i)(b_i−g_i)+(h_i−a_i)(b_i−h_i)]` donne par axe un minimum
aux **16 coins** : concavité séparée en `g_i,h_i`, affinité séparée en
`a_i,b_i`. C'est le minimum exact de cette **relaxation de boîtes
scalaires**, et seulement un minorant pour les ensembles discrets.
Pour `u=g+h`, chaque composante de
`C=(b−a)×(w_g+w_h)` s'écrit
`2[(b_j−a_j)(u_k−a_k−b_k)−(b_k−a_k)(u_j−a_j−b_j)]`.
Elle est multi-affine sur six scalaires : extrema signés aux 64 coins,
ou 16 couples de coins d'extrémités puis choix signé des bornes de `u`.
La somme des plus grands carrés par composante donne `Xmax` sûr, mais
perd les corrélations. Si `Hmin>0` et `3Hmin²>4Xmax` (q3) ou
`Hmin²>2Xmax` (q4), **tout** le bloc passe. `Hmin≤0` bloque seulement
ce certificat sur la boîte continue, pas des paires discrètes ; un échec
du test avec `Xmax` peut encore réussir au test corrélé de B. Jamais un
échec de préfiltre ne rejette une arête réelle. La
[fixture](test_group_bounds.py) contrôle 30 petites boîtes entières par
énumération exhaustive, les extrema signés et une égalité q4 qui ne
doit pas créditer. Aucun rendement de ce préfiltre n'est encore mesuré.

## Rejeu et portée

[`PROVENANCE.json`](PROVENANCE.json) épingle les SHA des coordonnées,
IDs, huit traces et deux panels. [`replay.py`](replay.py) vérifie sans
données temporaires tous les comptes, les partitions de groupes, les
monotonies et le [résumé](SUMMARY.json). La [liste SHA](SHA256SUMS)
scelle les sept fichiers du reçu. Pour un rejeu LIVE, régénérer
les entrées par
[`generate.py`](../lidar_raw_physical_scaling_20260923/generate.py),
reconstituer les huit traces S2 du reçu
[`edge_matched_core_20260923`](../edge_matched_core_20260923/README.md),
puis fournir leurs chemins ; le script refuse tout SHA différent et
recompile [`measure.cpp`](measure.cpp) avant de comparer les 480 lignes
octet pour octet :

```sh
python3 -B morsehgp3D_v9/audits/paired_guard_dispatch_grid_20260923/replay.py
python3 -B morsehgp3D_v9/audits/paired_guard_dispatch_grid_20260923/test_group_bounds.py
python3 -B morsehgp3D_v9/audits/paired_guard_dispatch_grid_20260923/replay.py \
  --points /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  --ids /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le \
  --trace /tmp/mhgp9-edge-core-audit-20260923/full/trace
```

Ce diagnostic porte sur **une seule trame brute complète**. Ses 3,70 %, la
cellule dominante et les taux du panel ne se transfèrent ni au sans-sol,
ni aux demi-scènes/quarts physiques, ni aux densités 1/2 et 1/4, ni au
profil float32, ni à d'autres séquences. La suite utile est une mesure
**appariée** du même routage et des preuves réellement closes sur les
15 cas S2 de [coupes/densités déjà présents](../s2_half_density_k5_20260923/README.md),
puis plusieurs trames et
séquences, avec coût total de préparation, groupage, certificats, repli,
cover/catalogue et FULL. Aucune borne sous-quadratique globale ni contrat
G4 ne découle de ce reçu.
