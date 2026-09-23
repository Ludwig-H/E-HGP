# Croissance LiDAR : secteurs capteur et densité, deux axes distincts

23 septembre 2026. Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md)
contient trois scènes 08 sans sol à grille 1 mm, K5/K10, s8/W8 : chaque
trame entière, ses deux moitiés `x<0`/`x≥0`, ses quatre quarts selon `y`,
et trois disques emboîtés. Les plans sont ceux du
[capteur](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md). Je relis ici
les six résumés, **42 cas de secteurs** au total. Une seule exécution par
cas, sur hôte CPU partagé ; les compteurs de travail sont le signal
principal, les temps CPU restent indicatifs. Statut `complete_relative` :
la complétude des clés absentes du catalogue n'est pas prouvée.

## Croissance quand le domaine spatial s'étend

Pour chaque partition en moitiés H ou en quarts Q, je calcule
`R=Σ W(morceau)/W(full)` sur les compteurs publiés et
`B=Σ (n_morceau/n_full)²`, valeur repère pour un coût quadratique homogène.
Une valeur `R<B` signale plus de travail que ce repère lors du passage
des morceaux à la scène ; `R>B` signale moins. Le repère n'est **pas** un
test asymptotique : la géométrie, les frontières, les certificats et les
sorties changent avec la coupe. `CPU` est `chain_cpu_s`, `paires` est
`expanded_pairs`, `cœur` est `core_sites`, la population **logique** des
disques diamétraux. Elle est proche, dans ce moteur, du nombre de formes
effectivement chargées `dead_core_form_sites` mais n'est pas elle-même un
compteur d'accès mémoire ; voir la [contrelecture du certificat par
nœuds](CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md).

| 08/K | `B_H/B_Q` | `R_H` CPU / paires / cœur | `R_Q` CPU / paires / cœur | liens cœur avec `p>2` | max `p_CPU` |
| --- | --- | --- | --- | ---: | ---: |
| 000000/K5 | 0,527 / 0,265 | 0,892 / 0,856 / **0,454** | 0,772 / 0,419 / 0,296 | 2/6 | 1,430 |
| 000000/K10 | 0,527 / 0,265 | 0,913 / 0,873 / 0,559 | 0,813 / 0,462 / 0,363 | 2/6 | 1,425 |
| 000100/K5 | 0,502 / 0,256 | 0,949 / 0,884 / 0,962 | 0,811 / 0,592 / 0,480 | 2/6 | 1,281 |
| 000100/K10 | 0,502 / 0,256 | 0,940 / 0,864 / 0,952 | 0,824 / 0,657 / 0,514 | 2/6 | 1,302 |
| 000200/K5 | 0,508 / 0,286 | 0,933 / 0,853 / 0,881 | 0,855 / 0,620 / 0,715 | 3/6 | 1,650 |
| 000200/K10 | 0,508 / 0,286 | 0,938 / 0,781 / 0,899 | 0,864 / 0,591 / 0,729 | 2/6 | 1,550 |

Les six liens par ligne sont `full→2 moitiés` et `chaque moitié→ses 2
quarts`. Leur pente finie est
`p=log(W_parent/W_enfant)/log(n_parent/n_enfant)` : 0/36 pentes CPU,
8/36 pentes de paires développées et **13/36 pentes de population de cœur**
dépasse 2. Le maximum du cœur vaut **3,940** sur
`000100/K5`, `half_x_neg→quarter_x_neg_y_nonneg`. Sur 000000/K5,
la somme des cœurs des moitiés est 45,4 % du plein, sous son repère
quadratique 52,7 %. À l'inverse, les maxima CPU restent à 1,65 ou moins.
Le temps apparent masque donc certaines masses internes défavorables.

Le déséquilibre n'est pas aléatoire : pour 000200/K10, le quart
`x≥0,y<0` contient **14 829/45 845** sites (32,3 %) mais environ
**583,0/1 069,2 M** `core_sites` (54,5 % du plein, 74,7 % de la somme
des quatre quarts). Une distribution uniforme des travaux entre quarts
serait mauvaise ici. Les trois disques emboîtés du même reçu donnent,
sur 000200 de 16k à 32k, `p_core=3,05` à K5 et `2,86` à K10 : ces
deux diagnostics changent la géométrie et ne se réfutent pas mutuellement.

## Densité dans des secteurs fixes : première mesure sur 08/000200

Le [reçu d'audit exploratoire](lidar_density_scene02_20260923/README.md)
garde **chacun des sept secteurs capteur fixe**. Il classe globalement les
IDs d'origine par `splitmix64(ID XOR d1da73a520260923)`, sélectionne
11 461⊂22 922⊂45 845 sites, puis intersecte chaque sélection avec les
secteurs. Chaque série reste emboîtée et conserve ses coordonnées ; les
moitiés et les quarts reconstruisent le plein à chaque densité. Le même
binaire Release de `4530644b`, K5/K10, s8/W8 et la grille 1 mm sont
utilisés. Il y a **28 nouvelles sondes** aux densités 1/4 et 1/2, plus
les 14 cas à densité entière du reçu v12 et un témoin plein rejoué.
Les SHA des entrées, le FNV, les options, le statut, les ordres et les
pentes recalculées à partir des effectifs réels ont été contrôlés ; les
sorties brutes et la méthode sont conservées dans le sous-dossier.

`p` ci-dessous utilise `log(W_b/W_a)/log(n_b/n_a)` et non un facteur 2
imposé aux secteurs. Les deux valeurs de chaque cellule sont `1/4→1/2`,
puis `1/2→1` pour la **population logique du cœur** :

| Secteur | Sites 1/4 / 1/2 / 1 | K5 : `p_core` | K10 : `p_core` |
| --- | ---: | ---: | ---: |
| Scène entière | 11 461 / 22 922 / 45 845 | 1,741 / 1,852 | 1,513 / 1,847 |
| `x<0` | 6 506 / 12 893 / 25 730 | 1,681 / 1,873 | 1,307 / 1,672 |
| `x≥0` | 4 955 / 10 029 / 20 115 | 1,827 / 1,798 | **1,999 / 2,006** |
| `x<0,y<0` | 4 146 / 8 220 / 16 262 | 1,690 / 1,879 | 1,719 / 1,629 |
| `x<0,y≥0` | 2 360 / 4 673 / 9 468 | 1,375 / 1,576 | 1,526 / 1,569 |
| `x≥0,y<0` | 3 630 / 7 339 / 14 829 | 1,905 / 1,816 | **2,046 / 2,035** |
| `x≥0,y≥0` | 1 325 / 2 690 / 5 286 | 1,400 / 1,439 | 1,430 / 1,468 |

Sur les **28 relations adjacentes** secteur×K×densité, les paires
développées ont `p=1,256–1,876`, les CPU·s `p=1,274–1,427`, et les
boules distinctes du catalogue `p=1,091–1,265`. En revanche,
`core_sites` atteint ou dépasse 2 sur **3/28** relations, toutes à K10 :
une dans le demi `x≥0` et les deux dans le quart `x≥0,y<0`. Le compteur
physique voisin `dead_core_form_sites` le fait sur **4/28** relations :
`2,011/2,014` dans ce demi et `2,058/2,043` dans ce quart. Pour le quart
`x≥0,y<0`, `core_sites/n²` augmente de **2,503→2,586→2,651** lorsque
la densité augmente. Pour les **formes réellement chargées** à K10,
`dead_core_form_sites/n²` descend de **0,777→0,557→0,504** sur la scène
entière mais monte de **2,452→2,555→2,633** sur ce quart. La pente
favorable de la scène entière ne décrit donc pas tous ses secteurs.
Les visites et bornes des nœuds de construction du
cœur restent, sur ces 28 relations, sous 2 ; la matérialisation des formes
est ici le signal plus net.

Il s'agit d'un **travail payé par la v12**, pas d'une population seulement
logique : au commit `4530644b`, `wspd_q34.cpp:549–557` appelle `dead_.load`
pour chaque cœur, puis `q34_dead_lanes.cpp:52–79` parcourt ses plages et
calcule/stocke une forme par site. `dead_core_form_sites` écarte du compte
les deux extrémités de chaque arête, bien qu'elles soient aussi parcourues.
Le code v12 fait donc au moins un traitement par forme comptée :
sur ce quart K10, **32,314 M→137,636 M→579,001 M** formes hors extrémités
aux trois densités. Le ratio `formes/n²` croissant signale un coût
réel du port v12 à ces tailles, sans établir une borne asymptotique pour
le LiDAR ni pour un futur certificat par blocs.

Une [extension sur les trames entières](lidar_density_full_3scenes_20260923/README.md)
applique la même graine et la même méthode à 08/000000 et 08/000100,
à K5/K10, avec huit nouvelles sondes. Sur les trois trames entières,
les pentes `dead_core_form_sites` sont :

| 08/K | Sites 1/4 / 1/2 / 1 | `p_formes` 1/4→1/2 / 1/2→1 |
| --- | ---: | ---: |
| 000000/K5 | 9 971 / 19 942 / 39 885 | 1,940 / **2,008** |
| 000000/K10 | 9 971 / 19 942 / 39 885 | 1,885 / 1,985 |
| 000100/K5 | 8 887 / 17 775 / 35 551 | 1,872 / 1,953 |
| 000100/K10 | 8 887 / 17 775 / 35 551 | 1,818 / 1,903 |
| 000200/K5 | 11 461 / 22 922 / 45 845 | 1,757 / 1,864 |
| 000200/K10 | 11 461 / 22 922 / 45 845 | 1,520 / 1,855 |

À trame entière, **une des douze** relations adjacentes de formes dépasse
donc légèrement 2, sur 000000/K5. Les paires développées y restent entre
`p=1,466` et `1,753` sur ces douze relations.

La [matrice complémentaire des six secteurs de
000000/000100](lidar_density_sectors_00_01_20260923/README.md) ajoute
**48 sondes** et réutilise leurs huit trames entières décimées ainsi que
les 28 secteurs pleins v12. Avec les sept secteurs de 000200, les trois
scènes donnent **84 relations adjacentes** secteur×K×densité. Nombre de
relations `p≥2` pour les formes du cœur :

| 08/K | Relations `p_formes≥2` / 14 | Plus forte pente et secteur |
| --- | ---: | --- |
| 000000/K5 | 2 | 2,014, quart `x≥0,y<0` |
| 000000/K10 | 1 | 2,063, quart `x≥0,y<0` |
| 000100/K5 | 4 | 2,123, demi `x≥0` |
| 000100/K10 | 3 | 2,039, demi `x≥0` |
| 000200/K5 | 0 | maximum 1,920, quart `x≥0,y<0` |
| 000200/K10 | 4 | 2,058, quart `x≥0,y<0` |

Soit **14/84** relations pour les formes effectivement matérialisées.
Le quart **`x≥0,y<0` est le seul secteur avec au moins un tel
franchissement dans chacune des trois trames** (à K5 ou K10). Sur les
56 relations nouvelles de 000000/000100, paires, visites/bornes des
nœuds du cover, émissions q3/q4, catalogue et CPU·s restent sous 2 ; les
dix exceptions de formes y sont isolées dans le reçu. Les ratios spatiaux
parent→morceau demeurent un autre diagnostic : même à densité 1/4 ou
1/2, la somme des tours des morceaux ne reconstruit pas la tour globale.

Le **secteur** reste fixe, mais l'étendue des sites retenus ne l'est pas.
Dans le quart `x≥0,y<0` de 000200, un seul ID original `122516`, présent
seulement à densité pleine, abaisse le minimum `z` encodé de 8 573 à 0
unités de 1 mm. Une [ablation appariée de cet
ID](lidar_density_scene02_20260923/README.md#contre-épreuve-dun-extrême-géométrique)
ramène ce minimum à 8 573 sans faire disparaître le franchissement K10 :
`dead_core_form_sites` passe de 579 000 541 à 578 914 981 et la pente
1/2→pleine de 2,042542 à 2,042528. Cet extrême ne cause donc pas à lui
seul le signal observé ; les frontières réelles des autres échantillons,
l'index et les certificats changent néanmoins avec la sélection.

Une [contre-épreuve à boîte exactement fixe](lidar_density_bbox_fixed_20260923/README.md)
impose les six extrema du quart plein aux niveaux 1/4 et 1/2, en gardant
les mêmes effectifs et l'emboîtement : seulement trois puis deux IDs sont
échangés. À K10, les pentes des formes `1/4→1/2` et `1/2→plein` restent
**2,058215** et **2,042880**, contre 2,058490 et 2,042542 avec la
sélection initiale. La variation de boîte n'explique donc pas à elle seule
le signal de ce quart ; la distribution interne et l'index restent libres.
Cette ablation porte sur un seul secteur, une seule graine et le même
binaire v12, sans nouvelle conclusion sur le coût total.

Le diagnostic détaillé porte encore sur une seule graine et trois scènes
sans sol d'une **seule séquence 08**.
Le thinning par hash réduit des sites dans le même support spatial
approximatif ; il ne reproduit ni les faisceaux d'un autre capteur, ni des
passages superposés. Les temps proviennent d'un hôte CPU partagé et les
cas à densité entière portent le libellé de sonde historique
`grid=unspecified`, même si leur entrée 1 mm est attestée par le manifeste.
Un [premier reçu brut](lidar_raw_physical_scaling_20260923/README.md)
couvre désormais les **21** croisements des sept secteurs 08/000000/K5
définis par plans **float32 physiques** et des trois densités emboîtées.
Ses 14 pentes de densité à secteur fixe donnent deux franchissements des
formes payées par le cœur : **2,136** sur la trame entière 1/2→entière
et **2,060** sur le quart `x≥0,y<0` 1/4→1/2. Les pentes CPU restent
entre **1,200 et 1,328**. À densité 1/4, 1/2, entière, les moitiés
réunies portent **0,549 / 0,421 / 0,306** des formes du plein alors que
leurs charges de cœur restent à environ **0,97**. Le signal de masse
par cœur se renforce donc avec la densité sur cette trame, mais ce seul
cas ne remplace pas la matrice sans sol K5/K10 ci-dessus. Les murs des
12 nouvelles coupes ont subi de fortes interférences sur l'hôte partagé ;
privilégier les compteurs dans ce reçu. Répéter sur d'autres séquences,
plusieurs graines et K10, puis comparer une sélection stratifiée par
azimut/rayon en gardant K/s/W, segmentation et préparation séparés ;
publier sortie, catalogue,
paires, témoins, populations logiques et formes du cœur, atlas, q3/q4,
CPU, mur et mémoire. Ni ces pentes finies ni les ratios spatiaux ne
démontrent une borne sous-quadratique globale ou le contrat G4.
