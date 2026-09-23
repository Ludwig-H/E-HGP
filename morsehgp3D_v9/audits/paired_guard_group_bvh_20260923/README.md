# Certificats de paires partagés dans un BVH de couples d'arêtes

23 septembre 2026. Shadow CPU **hors produit et hors registre**, sur la trame
brute SemanticKITTI 08/000000 (u18, grille commune 1 mm), K5/s8. Le groupe
retenu est celui de plus grande **cardinalité d'arêtes** parmi les 52 groupes
`(cellule de milieu 4 096 mm, axe dominant)` du crible S2 `D≥2²³`, occupation
de cellule ≥1 024 : **67 827 arêtes**, contre 9 548 pour le second. Ce choix
emploie les extrémités et les survivantes **après expansion S2**, mais pas la
taille `F` du cœur. Il reste favorable et observé sur une seule trame ; le
parcours des 3 986 433 arêtes S2 pour former les groupes est payé dans le
[diagnostic de dispatch](../paired_guard_dispatch_grid_20260923/README.md).
Le [fichier binaire compact](dominant_group.bin) archive les 67 827 arêtes,
masques et `F` diagnostiques ; le [rejeu LIVE](run.py) peut le réextraire des
huit traces de SHA épinglés. Les IDs sont les IDs de retours bruts.

## Construction avant `load`

Le [sidecar](group_bvh.cpp) répartit les couples orientés `(a,b)` par médiane
sur la plus large des six coordonnées, jusqu'à une taille de feuille donnée.
Ses boîtes fermées `A×B` sont exactes. À profondeur quatre, ses 16 pivots
fournissent des représentants choisis **uniquement par coordonnées
d'extrémités**, avant toute lecture de `F`. Pour chacun, un balayage complet
des 123 389 sites cherche les 16 gardes les plus proches du milieu dans
chaque quadrant perpendiculaire, parmi les sites du disque diamétral. Les
paires positives q3/q4 de cette palette sont triées par marge entière et
sélectionnées sans réutilisation de site ; l'union donne **51 paires fixes**.
Cette recherche fait réellement **1 974 224 visites de sites**, dont 86 678
passent le test diamétral ; 967 sites entrent dans les 16 palettes B16,
**28 948 paires** y sont évaluées. Elle est pré-cœur, mais reste un balayage
mono CPU ; un index pourrait le remplacer seulement après qualification.

Pour une paire fixe `g,h` et des boîtes d'extrémités `A,B`, on minore
`H/4 = Σᵢ[(gᵢ−aᵢ)(bᵢ−gᵢ)+(hᵢ−aᵢ)(bᵢ−hᵢ)]` par les quatre coins
`(aᵢ,bᵢ)` de chaque axe. Chaque composante de
`(b−a)×[2(g+h−a−b)]` est multi-affine en quatre coordonnées d'extrémités :
ses extrema signés sont aux 16 coins ; la somme des maxima carrés donne un
`Xmax` sûr. Le test strict `Hmin>0`, `3Hmin²>4Xmax` certifie q3 ;
`Hmin²>2Xmax` certifie q4. Les corrélations entre composantes peuvent faire
échouer ce certificat conservateur. Une réussite vaut pour **toutes** les
arêtes du nœud ; un échec conduit à la subdivision ou au repli exact. Une
table `ID de garde → bits de propositions`, puis les OR des extrémités et des
enfants, interdisent de créditer une paire qui contient une extrémité du
nœud. Dans chaque feuille et chaque voie, le matching glouton prend des
paires aux **IDs tous disjoints** ; q4 ne ferme pas q3 lorsque les deux
voies sont actives. Les crédits de voies ne s'additionnent pas.

| Feuille max. | Nœuds / feuilles | Tests uniformes | Matchings / inspections de propositions | Arêtes entièrement fermées | `F` de ces arêtes |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 12 773 / 6 387 | 571 162 | 10 892 / 399 884 | 29 610 | 183 784 954 |
| **64** | **4 095 / 2 048** | **187 790** | **4 107 / 162 622** | **24 537** | **153 837 726** |
| 256 | 1 023 / 512 | 47 514 | 1 213 / 51 732 | 17 313 | 109 836 328 |

À feuilles 64, les masques initiaux q3 seul/q4 seul/q3+q4 ont
**1 225/31 329/35 273** arêtes. Les fermetures *intégrales* sont
**357/18 421/5 759** respectivement, avec `F` diagnostiques
**1 964 409/118 135 449/33 737 868**. Le certificat ferme donc 24 537
arêtes, soit **153 837 726 des 368 004 895 sites de cœur** de ce groupe
potentiellement évitables avant `load` ; **43 290 arêtes et 214 167 169
sites de cœur restent**. Le coût payé comprend aussi construction et tri du
BVH, préparation des 51 paires, collisions ID, 187 790 tests uniformes et
matchings. Les collisions se préparent en 102 insertions de bits, une visite
des 67 827 arêtes en feuilles puis OR de 4 mots par nœud ; le programme
compte 6 943 propositions non testées à cause d'un ID d'extrémité. Sur une
exécution Release locale indicative : 15,7 ms de BVH, 8,7 ms de palette,
2,7 ms de collisions, 29,5 ms de tests de boîtes, 1,1 ms de propagation et
matching. Le programme complet prend 0,16 s mur / 0,15 CPU utilisateur,
13 Mio RSS ; il **inclut** 98,8 ms de contrôle ponctuel sur *toutes* les
arêtes, plus coûteux que le seul repli ouvert, mais **exclut** le produit,
son cœur et son aval. L'extracteur séparé parcourt les 3 986 433 traces S2
en 0,06 s mur local ; ces durées non appariées et bruitées ne sont pas un
chrono de tour ni une preuve du contrat 100 ms.

Un repli **optionnel et distinct** qui teste les 51 propositions sur les
43 290 arêtes ouvertes fait 2 196 475 tests ponctuels (balayage complet de
la palette, donc majorant d'un arrêt dès preuve positive) ; le glouton en
ferme 13 875 de plus, `F=79 640 454`. Les 29 415 restantes gardent
`F=134 526 715` pour la chaîne exacte. Les 3 445 238 tests ponctuels sur
**toutes** les arêtes avec cette palette donnent la même fermeture finale,
38 412 ; le stage BVH réduit leur nombre dans cette expérience, mais ses
tests de boîtes, balayages de préparation, mémoire et aval coûtent aussi.
`F` ne sert jamais à choisir groupe, représentants, palette ou certificat :
il ne mesure qu'après coup les formes `load` qui auraient pu être évitées.

La palette de contrôle de **101 paires** tirée des preuves ponctuelles
[du panel](../paired_guard_node_blocks_20260923/README.md) utilise au
contraire une sélection **post-`F`** ; à feuilles 64 elle ferme 25 832
arêtes (`F=160 795 484`) après 381 203 tests uniformes. Ce bras mesure
seulement une capacité géométrique conditionnelle, sans coût de découverte
pré-cœur. Le
[shadow B de grands rectangles WSPD](../rect_pair_shadow_b_20260923/README.md)
traite une autre unité de partage ; ses nombres et ceux du BVH d'arêtes ne
constituent pas une comparaison à budget identique.

Le mode `-DVERIFY` contrôle **chaque incidence ponctuelle** des tuiles
positives (`2 587 862/1 761 439/882 955` pour feuilles 16/64/256), les
IDs d'extrémités et les deux prédicats entiers ; `group_only=0` face au
contrôle ponctuel complet. Une contrelecture indépendante a comparé la
table collision à un scan naïf sur **1 290 073 couples nœud-proposition**
sans divergence ; Clang ASan/UBSan `-DVERIFY`, ainsi que Release strict,
passent. Le [résumé](SUMMARY.json) et les [empreintes](SHA256SUMS) donnent
les comptes exacts. Le lecteur statique fonctionne sans données temporaires :

```sh
python3 -B morsehgp3D_v9/audits/paired_guard_group_bvh_20260923/run.py
python3 -B -O morsehgp3D_v9/audits/paired_guard_group_bvh_20260923/run.py
```

Pour recompiler et vérifier le groupe, les palettes et toutes les tuiles
contre les coordonnées originales, régénérer les fichiers via
[`generate.py`](../lidar_raw_physical_scaling_20260923/generate.py), les
traces via le [reçu S2](../edge_matched_core_20260923/README.md), puis :

```sh
python3 -B morsehgp3D_v9/audits/paired_guard_group_bvh_20260923/run.py \
  --points /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  --ids /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le \
  --trace /tmp/mhgp9-edge-core-audit-20260923/full/trace
```

Ce reçu est une piste de mutualisation constructive dans **un seul groupe
favorable d'une seule trame**. Il ne démontre ni un gain net du pipeline,
ni la croissance sous-quadratique du générateur, ni les contrats K5/K10,
FULL ou G4. La suite décisive est de mesurer le **dispatch + BVH + palette
+ certificats + replis + cœur + cover/catalogue/FULL** sur tous les groupes,
puis sur demi-scènes, quarts et densités 1/2, 1/4 de plusieurs trames,
avec temps mur CPU/GPU et empreinte mémoire.
Un port dans le produit devra conserver avec chaque couple son **ordinal S2
global** et son masque d'origine pour réinjecter les survivantes et sorties
sans permutation ; le binaire compact de cette expérience conserve les IDs
et l'ordre du replay, mais pas cet ordinal explicite. Le nœud ne représente
qu'un sous-ensemble de couples d'extrémités réels : une preuve uniforme sur
sa boîte les certifie tous, et un échec de boîte ne supprime aucune arête.
