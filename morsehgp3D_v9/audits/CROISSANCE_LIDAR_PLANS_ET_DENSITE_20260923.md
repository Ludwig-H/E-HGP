# Croissance LiDAR : secteurs capteur et densité, deux axes distincts

23 septembre 2026. Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md)
contient trois trames sans sol de la séquence 08 à grille 1 mm,
K5/K10, s8/W8 : chaque
trame entière, ses deux moitiés `x<0`/`x≥0`, ses quatre quarts selon `y`,
et trois sous-nuages emboîtés. Les plans passent par le
[capteur](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md), mais les
côtés de cette matrice historique suivent le **signe quantifié** : un
retour de 08/000200 change de côté selon son signe float32 physique,
contre-éprouvé plus bas. Je relis ici
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
`expanded_pairs`, `cœur` est `core_sites`, la somme des tailles des disques
diamétraux. **Sur les chemins complets de ce binaire v12, c'est aussi le
nombre exact de formes calculées et écrites par `dead_.load` pour ces
cœurs**, extrémités `a,b` comprises. `dead_core_form_sites` en retranche
deux par charge : `core_sites = dead_core_form_sites + 2 × dead_core_loads`.
Ce sous-total hors extrémités reste utile, mais ne compte pas toutes les
formes matérialisées. Aucun des deux compteurs ne mesure à lui seul les
accès mémoire ; voir la [contrelecture du certificat par
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
serait mauvaise ici. Les trois sous-nuages emboîtés du même reçu donnent,
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
voisin `dead_core_form_sites`, limité aux sites hors extrémités, le fait
sur **4/28** relations :
`2,011/2,014` dans ce demi et `2,058/2,043` dans ce quart. Pour le quart
`x≥0,y<0`, `core_sites/n²` augmente de **2,503→2,586→2,651** lorsque
la densité augmente. Pour les **formes hors extrémités** à K10,
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
les deux extrémités de chaque arête, **bien que leurs formes soient aussi
calculées et écrites**.
Le code v12 fait donc au moins un traitement par forme comptée :
sur ce quart K10, **32,314 M→137,636 M→579,001 M** formes hors extrémités
aux trois densités. Le ratio `formes/n²` croissant signale un coût
réel du port v12 à ces tailles, sans établir une borne asymptotique pour
le LiDAR ni pour un futur certificat par blocs.

Une [extension sur les trames entières](lidar_density_full_3scenes_20260923/README.md)
applique la même graine et la même méthode à 08/000000 et 08/000100,
à K5/K10, avec huit nouvelles sondes. Sur les trois trames entières,
les pentes du sous-total `dead_core_form_sites` et du total réellement
calculé `core_sites` sont :

| 08/K | Sites 1/4 / 1/2 / 1 | `p` hors extrémités 1/4→1/2 / 1/2→1 | `p` toutes formes 1/4→1/2 / 1/2→1 |
| --- | ---: | ---: | ---: |
| 000000/K5 | 9 971 / 19 942 / 39 885 | 1,940 / **2,008** | 1,918 / 1,994 |
| 000000/K10 | 9 971 / 19 942 / 39 885 | 1,885 / 1,985 | 1,870 / 1,974 |
| 000100/K5 | 8 887 / 17 775 / 35 551 | 1,872 / 1,953 | 1,845 / 1,935 |
| 000100/K10 | 8 887 / 17 775 / 35 551 | 1,818 / 1,903 | 1,801 / 1,890 |
| 000200/K5 | 11 461 / 22 922 / 45 845 | 1,757 / 1,864 | 1,741 / 1,852 |
| 000200/K10 | 11 461 / 22 922 / 45 845 | 1,520 / 1,855 | 1,513 / 1,847 |

À trame entière, **aucune des douze** relations de toutes les formes ne
dépasse 2 : le sous-total seul le dépasse légèrement sur 000000/K5.
Les paires développées restent entre
`p=1,466` et `1,753` sur ces douze relations.

La [matrice complémentaire des six secteurs de
000000/000100](lidar_density_sectors_00_01_20260923/README.md) ajoute
**48 sondes** et réutilise leurs huit trames entières décimées ainsi que
les 28 secteurs pleins v12. Avec les sept secteurs de 000200, les trois
scènes donnent **84 relations adjacentes** secteur×K×densité. Nombre de
relations `p≥2` pour le sous-total hors extrémités et le total des formes
effectivement calculées :

| 08/K | Hors extrémités `p≥2` / 14 | Toutes formes `p≥2` / 14 | Maximum toutes formes et secteur |
| --- | ---: | ---: | --- |
| 000000/K5 | 2 | **0** | 1,994, scène entière |
| 000000/K10 | 1 | **1** | 2,037, quart `x≥0,y<0` |
| 000100/K5 | 4 | **3** | 2,082, demi `x≥0` |
| 000100/K10 | 3 | **3** | 2,015, demi `x≥0` |
| 000200/K5 | 0 | **0** | 1,905, quart `x≥0,y<0` |
| 000200/K10 | 4 | **3** | 2,046, quart `x≥0,y<0` |

Soit **10/84** relations pour toutes les formes effectivement matérialisées,
contre 14/84 pour le seul sous-total hors extrémités. Quatre
franchissements à peine au-dessus de 2 disparaissent dans le total ;
les autres restent, dont le quart chaud à K10 sur 000000 et 000200.
Le quart **`x≥0,y<0` est le seul secteur avec au moins un tel
franchissement dans chacune des trois trames** (à K5 ou K10). Sur les
56 relations nouvelles de 000000/000100, paires, visites/bornes des
nœuds du cover, émissions q3/q4, catalogue et CPU·s restent sous 2 ; les
sept franchissements du total des formes se recalculent à partir des
`core_sites` gardés dans le reçu. Les ratios spatiaux
parent→morceau demeurent un autre diagnostic : même à densité 1/4 ou
1/2, la somme des tours des morceaux ne reconstruit pas la tour globale.

Contrelecture du 24 septembre : les **126/126 cas** sans sol ont été
recoupés avec les 84 sorties de densité et les 42 reçus v12 aux effectifs,
`core_sites`, `expanded_pairs`, CPU·s et à l'identité
`core_sites = dead_core_form_sites + 2×dead_core_loads`, sans écart.
Le recalcul des 84 pentes de densité donne 10/84 `p_core≥2`, **0/84**
`p_CPU≥2` et **0/84** `p_expanded_pairs≥2`. Les 21 cas bruts K5 et les
21 K10 ont aussi été recoupés aux lignes de leurs reçus. Ces accords
contrôlent le calcul et la provenance des pentes, pas la complétude
absolue des catalogues.

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
échangés. À K10, les pentes du sous-total hors extrémités `1/4→1/2`
et `1/2→plein` restent
**2,058215** et **2,042880**, contre 2,058490 et 2,042542 avec la
sélection initiale ; les pentes de toutes les formes restent **2,046155**
et **2,035494**. La variation de boîte n'explique donc pas à elle seule
le signal de ce quart ; la distribution interne et l'index restent libres.
Cette ablation porte sur un seul secteur, une seule graine et le même
binaire v12, sans nouvelle conclusion sur le coût total.

Les quarts sans sol archivés à cette étape sont séparés selon le signe
**quantifié**. Une [contre-épreuve du signe float32 physique](lidar_scene02_physical_cut_20260923/README.md)
sur 08/000200/K10 déplace exactement un retour brut entre les deux
quarts `y<0`, sans changer ses coordonnées u18 ni la grille commune.
Le site porte l'ID 61939 dans le profil grille mais 61942 dans le profil
float32 : la jointure se fait par le retour brut 14826. Sur le quart
`x≥0,y<0`, `core_sites` diminue de **2 980 sur 583 000 415** et la
pente 1/2→plein reste **2,035350**. Le franchissement de 2 ne provient
donc pas de cet arrondi ; seule cette relation K10 est contre-éprouvée.

Le diagnostic détaillé porte encore sur une seule graine et trois trames
sans sol d'une **seule séquence 08**.
Le thinning par hash réduit des sites dans le même support spatial
approximatif ; il ne reproduit ni les faisceaux d'un autre capteur, ni des
passages superposés. Les temps proviennent d'un hôte CPU partagé et les
cas à densité entière portent le libellé de sonde historique
`grid=unspecified`, même si leur entrée 1 mm est attestée par le manifeste.
Un [premier reçu brut](lidar_raw_physical_scaling_20260923/README.md)
couvre désormais les **21** croisements des sept secteurs 08/000000/K5
définis par plans **float32 physiques** et des trois densités emboîtées.
Les densités brutes et sans sol ne sont pas une ablation appariée du masque :
le brut classe les **123 389 retours avant retrait du sol**, tandis que le
sans-sol classe seulement les sites **retenus après le masque de la trame
entière** (39 885, 35 551 ou 45 845 selon la scène). La même graine ne
sélectionne donc pas les mêmes retours aux fractions 1/4 et 1/2 ; de plus,
leurs effectifs et leurs conventions de coupe historiques diffèrent.
Comparer leurs pentes décrit deux régimes, sans isoler l'effet causal du sol.
Sur la scène entière 08/000000, après jointure par `raw_to_original.u32le`,
les sous-échantillons de densité 1/4 du brut restreint aux retours sans sol
et du nuage sélectionné *après* masque ont
respectivement **10 014 et 9 971** retours, dont seulement **2 539** communs ;
à demi-densité, **19 909 et 19 942**, dont **9 895** communs.
Reproduction : appliquer les classements des [générateurs brut](lidar_raw_physical_scaling_20260923/generate.py)
et [sans sol](lidar_density_full_3scenes_20260923/generate_crossscene.py)
avec leur graine commune, puis inverser la table
[`scene_00_grid/raw_to_original.u32le`](../../morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/raw_to_original.u32le)
pour ramener les IDs originaux du sans-sol aux IDs de retours bruts ;
les intersections ci-dessus portent sur ces derniers, pas sur des rangs de sites.
Pour une comparaison **appariée** du retrait du sol, tirer à chaque densité
les mêmes IDs de retours bruts sur la trame entière, puis appliquer le masque
figé seulement au bras sans sol avant les coupes capteur. Garder les effectifs
réels de chaque bras dans les pentes et chronométrer le masque à part ; un
retrait aléatoire à effectif égal servirait de contrôle de la géométrie du sol.
Les README bruts K5/K10 scellés par leurs `SHA256SUMS` appellent parfois
`dead_core_form_sites` « formes matérialisées » ; leurs nombres et pentes
restent ceux du sous-total **hors extrémités**, et les corrections exactes
ci-dessous utilisent `dead_core_form_sites + 2 × dead_core_loads` sans
modifier ces reçus.
Ses 14 pentes de densité à secteur fixe donnent deux franchissements du
total des formes payées par le cœur : **2,119** sur la trame entière
1/2→entière et **2,002** sur le quart `x≥0,y<0` 1/4→1/2. Le sous-total
hors extrémités donnait respectivement 2,136 et 2,060. Les pentes CPU restent
entre **1,200 et 1,328**. À densité 1/4, 1/2, entière, les moitiés
réunies portent **0,567 / 0,435 / 0,315** de toutes les formes du plein alors que
leurs charges de cœur restent à environ **0,97**. **Sept des 18** liens
spatiaux parent→enfant K5 ont une pente de toutes les formes supérieure à
2 ; ces coupes modifient aussi la géométrie. Le signal de masse
par cœur se renforce donc avec la densité sur cette trame, mais ce seul
cas ne remplace pas la matrice sans sol K5/K10 ci-dessus. Les murs des
12 nouvelles coupes ont subi de fortes interférences sur l'hôte partagé ;
privilégier les compteurs dans ce reçu. Répéter sur d'autres séquences et
plusieurs graines, puis comparer une sélection
stratifiée par azimut/rayon en gardant K/s/W, segmentation et préparation
séparés ;
publier sortie, catalogue,
paires, témoins, populations logiques et formes du cœur, atlas, q3/q4,
CPU, mur et mémoire. Ni ces pentes finies ni les ratios spatiaux ne
démontrent une borne sous-quadratique globale ou le contrat G4.

Le [complément brut K10](lidar_raw_k10_density_20260923/README.md) reprend
exactement les trois entrées **entières** du reçu K5 (30 847⊂61 694⊂123 389
retours) et mesure toute la tour K1..10. À 1/4→1/2 puis 1/2→entière, ses
pentes de toutes les formes calculées sont **1,634 / 1,922**, contre
**1,800 / 2,119** à K5 sur les mêmes octets ; ses pentes CPU·s sont
**1,149 / 1,239**. Le sous-total hors extrémités publié initialement
donne 1,650 / 1,936 à K10 et 1,823 / 2,136 à K5. Le plein K10 calcule
pourtant **1 254,25 M** formes, **11,387 M** boules de
catalogue et **8,219 GiB** de RSS. Les comptes des cinq premiers ordres
coïncident entre K5 et K10 à chaque taille, sans comparaison de leurs clés
ou de leurs flux complets. Le changement de K modifie donc le signal des
formes sur ce doublement fini, sans régler le coût absolu ni démontrer une
croissance sous-quadratique générale.

La même correction vaut pour le **cover complet** lorsque sa voie morte
charge les formes : il écrit aussi ses deux extrémités, absentes de
`dead_form_sites`. Sur les trois cas bruts K10, la somme réellement écrite
par les deux chargements vaut
`dead_core_form_sites + dead_form_sites + 2 × (dead_core_loads + dead_loads)`,
égale à `core_sites + cover_sites` dans ces sorties complètes,
soit **298,608→693,370→2 328,973 M** formes, de pentes finies
**1,215 / 1,748**. Le sous-total historique de la
[contrelecture B](CONTRE_AUDIT_B_LIDAR_BRUT_K10_20260923.md) vaut
292,965→681,708→2 304,229 M : il écarte jusqu'à **24,745 M** formes
pourtant écrites. Cela ne change pas le verrou architectural : la masse
par arête doit baisser, avec ses coûts de certificat et d'aval mesurés.

La [matrice brute K10 complète](lidar_raw_k10_sectors_20260923/README.md)
mesure maintenant les **sept secteurs physiques aux trois densités** sur
la même trame 08/000000. Sur les 14 liens de densité à secteur fixe, une
pente de toutes les formes dépasse 2 : le quart `x≥0,y<0` entre 1/2 et
entière atteint **2,029** (2,057 hors extrémités) ; le maximum des pentes
CPU·s est **1,262**. Sur les 18 liens spatiaux parent→enfant, **trois**
pentes du total des formes dépassent 2, contre quatre pour le sous-total.
À densité entière, la somme des quatre quarts contient **38,1 %** de
toutes les formes du plein (37,4 % hors extrémités), mais **95,8 %** de
ses charges de cœur et **99,1 %** des
boules du catalogue. Les six demi-scènes K10 ont été rejouées avec les
mêmes compteurs déterministes ; leurs murs restent sensibles à la
contention. Ainsi, le besoin d'éviter des formes **par cœur** persiste
à K10, même lorsque la pente du plein en densité reste sous 2. Les
secteurs et sous-échantillons ne reconstituent pas la tour du plein et
ne prouvent aucune borne asymptotique. Plusieurs séquences, graines et
une mesure appariée du coût total sur G4 restent à produire.

Une [contre-épreuve avec deux autres graines](lidar_raw_hot_quarter_multiseed_20260923/README.md)
garde le **même quart physique brut** `x≥0,y<0` de 08/000000 et refait
les décimations globales emboîtées à K5/K10. À K10, la pente du total
de formes calculées entre demi-densité et plein vaut **2,029** avec la
graine historique, puis **1,971** et **2,095** ; le franchissement de 2
n'est donc pas stable au tirage, mais la sensibilité de la masse du
cœur demeure. À K5, le premier lien donne **2,002 / 1,957 / 1,860**.
Les douze pentes de paires développées restent sous 2. Les huit
nouvelles sondes sont locales CPU/W8 et ne portent que sur ce quart :
elles ne qualifient ni la trame entière ni une borne sous-quadratique.

Le [rejeu S2/v17 sur le quart brut chaud](q34_batch_density_quarter_20260923/README.md)
garde les mêmes trois entrées et compare moteur et filtre par lots CPU,
K5/s8/W8. Leurs formes de cœur sont identiques :
**3,549→14,655→52,302 M**, de pentes **2,002/1,823**. À pleine densité,
le lot CPU ajoute 66,2 % de visites de témoins et 4,6 % de CPU de chaîne
dans cet essai ; le transfert sur G4 n'est pas mesuré. Le
[reçu S2 des arêtes appariées](edge_matched_core_20260923/README.md)
étend K5 aux **quatre quarts et à la trame entière**, aux trois densités :
les arêtes traversant les quarts portent, à densité pleine, **69,06 %**
des formes pour **3,94 %** des charges. Ce résultat localise le coût de
réunification de la scène sans supposer qu'un axe de coupe est toujours
défavorable. Le [panel S2 des deux moitiés](s2_half_density_k5_20260923/README.md)
compare maintenant moteur et lot CPU sur les neuf entrées K5 de la
trame entière et de ses deux moitiés, aux trois densités. Six compteurs
déterministes, dont formes du cœur et paires développées, coïncident
**9/9** avec v12. Le rapport formes des moitiés sur le plein y tombe
de **0,567→0,435→0,315** pendant que celui des charges reste vers
**0,97** : le raccord S2 n'a pas réduit cette masse sur ce panel.
K10 S2, plusieurs scènes, graines et séquences restent à mesurer. Les matrices v12 et ce panel
CPU ne prouvent aucune borne globale ni un temps G4.

Le [quart **physique** sans sol de 08/000200/K10, répété avec trois graines](lidar_ground_hot_quarter_multiseed_20260923/README.md)
confirme un cas sentinelle de cette matrice : `x≥0,y<0` donne des pentes
`core_sites` de **2,046 / 2,077 / 2,025** au premier doublement
1/4→1/2 ; au second, **2,035 / 1,916 / 2,048**. Le point voisin de
`x=0` qui changeait de quart sous le signe quantifié a été attribué par
son signe float32 physique ; les trois niveaux de chaque série utilisent
la même définition. Ces trois graines ne transforment pas deux
doublements finis en preuve asymptotique, mais rendent ce quart pertinent
pour un rejeu prioritaire.

Le [reçu G4 R15 S4a](CONTRELECTURE_G4_R15_S4A_20260923.md) apporte un gain
apparié sur trois trames **entières** sans sol de la séquence 08, sans
demi-scène, quart ni sous-échantillon. Un
[rejeu CPU S3/S4a apparié](s4a_ground_hot_quarter_20260923/README.md)
mesure maintenant le **quart physique** `x≥0,y<0` de 08/000200/K10 à
3 609 → 7 387 → 14 828 sites, avec la même sélection globale emboîtée.
Les six sorties ont les mêmes condensés de tour et de catalogue, les dix
ordres et sept comptes structurels communs ; une septième tentative,
géométriquement valide mais sous contention, est exclue des pentes.
Les deux bras conservent exactement **34,673 → 153,448 → 582,997 M**
`core_sites`, soit `p=2,077/1,916`. S4a déplace q3 après le cœur sans
réduire cette masse. Ses tests logiques de census par lanes croissent
**43,220 → 130,416 → 372,784 M**, `p=1,542/1,507` ; ils ne sont pas
des ballots physiques. Les CPU·s de chaîne croissent d'exposants
**1,391/1,410** pour S3 et **1,393/1,406** pour S4a sur ces deux
liens. Les murs sont descriptifs sur l'hôte partagé, et la sonde CPU ne
transfère pas ses pentes de temps à G4.

La suite est le panneau S4a des **sept secteurs physiques** aux densités
globales emboîtées 1/4, 1/2 et 1, avec K5/K10 et plusieurs trames puis
séquences. Apparier les digests et distinguer croissance des comptes de
travail, mur de chaîne, segmentation et préparation. Les tours des
morceaux ne recomposent pas la tour entière.

**Voie constructive à éprouver sur ce panneau.** Le chargement actuel
du cœur calcule une forme par site du disque diamétral **avant** le
certificat S3, sur CPU comme sur GPU. Le
[BVH de paires de gardes](paired_guard_group_bvh_20260923/README.md)
possède déjà un certificat exact sur boîtes de couples d'arêtes : il peut
réduire les masques q3/q4 des survivantes S2 **avant** les deux chemins
de chargement, puis laisser chaque voie indécise au repli actuel. Sur un
groupe favorable du plein brut K5, 24 537 arêtes fermées représentent
153,838 M formes de cœur évitables en principe ; ce potentiel n'est
pas un gain intégré. Le test décisif est le dispatch de **tous** les
groupes sur les quarts chauds et pleins aux densités appariées, avec
coût de préparation, `core_sites`, sorties exactes, catalogue et temps
de chaîne. Le [lemme des paires](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md)
garde les égalités en repli et des IDs de gardes disjoints ; les
ordinaux S2 et masques initiaux doivent survivre au regroupement.
