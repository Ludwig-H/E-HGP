# Diagnostic RAW LiDAR : plans physiques et densité, 08/000000 K5

23 septembre 2026. Reçu **exploratoire local** du binaire v9 `mhgp9_tower_probe_v12`
SHA-256 `e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
Une seule trame SemanticKITTI 08/000000 **avec sol**, 123 389 retours distincts,
sans fusion sur la grille 1 mm. Les prédicats géométriques utilisent les
coordonnées entières u18 déclarées, pas les float32 d'origine. K5, s8,
8 workers CPU et 8 fils
statiques, `nice 19`, tous les leviers v12 actifs, aucun GPU ou GCP. Les **21**
sondes retournent `complete_relative` : le catalogue émis est recoupé, sa
complétude envers les clés jamais émises n'est pas établie. Une exécution par
cas sur hôte CPU partagé ; les temps sont descriptifs.

Les plans `x=0`, `y=0` passent par le capteur dans le repère **float32 physique**.
`generate.py` lit les IDs des sept secteurs float32 préparés par la v8, joint
chaque retour aux coordonnées u18 de la **trame entière** via les deux tables
`raw_to_full.u32le` distinctes, et conserve la translation globale. Les IDs
canonisés float32 et u18 portent les mêmes entiers 0..n−1 mais leur sens
change après tri : une jointure directe serait fausse. Les deux maps sont
bijectives ici ; le générateur vérifie les SHA source, les signes float32,
l'emboîtement des sélections et la reconstruction disjointe. Les trois retours
dont le secteur serait différent après arrondi 1 mm ont les IDs originaux
`49349, 57964, 118789` ; leur secteur **physique** est utilisé. Le hash
`splitmix64(raw_return_id XOR d1da73a520260923)` sélectionne globalement
30 847 ⊂ 61 694 ⊂ 123 389 retours, puis chaque secteur intersecte ces
ensembles. Les coordonnées des retours conservés ne sont jamais déplacées.

Les trois densités de la **scène entière** donnent :

| densité | sites | mur chaîne s | CPU·s | q3/q4 s | `dead_core_loads` | `dead_core_form_sites` | boules |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 30 847 | 9,318 | 49,920 | 7,005 | 774 494 | 35 460 916 | 733 773 |
| 1/2 | 61 694 | 20,486 | 116,088 | 15,686 | 1 684 675 | 125 483 471 | 1 476 772 |
| entière | 123 389 | 48,357 | 285,329 | 38,942 | 3 986 433 | 551 688 875 | 2 822 052 |

Pour `p=log(W_b/W_a)/log(n_b/n_a)`, les pentes 1/4→1/2 puis 1/2→entière
sont : **formes cœur 1,823 / 2,136**, charges de cœur 1,121 / 1,243,
CPU 1,218 / 1,297, mur chaîne 1,137 / 1,239, q3/q4 1,163 / 1,312,
paires développées 1,454 / 1,495. Le franchissement quadratique observé
concerne donc le traitement/matérialisation des formes, **pas** le temps
total mesuré. Les formes par charge passent de 45,79 à 74,49 puis 138,39.

À densité entière, les **secteurs physiques** donnent :

| secteur | sites | mur chaîne s | CPU·s | q3/q4 s | charges cœur | formes cœur |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| entier | 123 389 | 48,357 | 285,329 | 38,942 | 3 986 433 | 551 688 875 |
| moitié x<0 | 61 045 | 28,049 | 151,963 | 22,687 | 2 097 960 | 91 507 182 |
| moitié x≥0 | 62 344 | 18,919 | 104,870 | 14,554 | 1 765 369 | 77 246 395 |
| quart x<0,y<0 | 30 265 | 12,790 | 67,918 | 10,250 | 990 609 | 44 164 932 |
| quart x<0,y≥0 | 30 780 | 16,091 | 80,389 | 12,726 | 1 088 446 | 45 656 014 |
| quart x≥0,y<0 | 31 391 | 10,368 | 44,764 | 8,109 | 828 338 | 50 645 635 |
| quart x≥0,y≥0 | 30 953 | 11,140 | 56,321 | 9,108 | 923 776 | 24 891 617 |

La somme des deux moitiés vaut 97,0 % des **charges de cœur** du plein,
mais seulement **30,6 % des formes**. La masse moyenne par charge passe
d'environ 43,68 formes dans les moitiés à 138,39 sur la trame. La somme des
quatre quarts donne 96,1 % des charges et 30,0 % des formes du plein.
Le nombre de cœurs ne révèle donc pas ce verrou ; il faut borner ou partager
le traitement des sites **par cœur** lorsque le domaine s'étend.
Ce n'est pas un compteur seulement logique : la voie v12 appelle
`dead_.load` pour chaque cœur (`wspd_q34.cpp:556`), puis calcule et stocke
les formes des sites (`q34_dead_lanes.cpp:52–79`) ; le compteur exclut
seulement les deux extrémités de chaque arête. Le
[certificat par nœuds](../CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md) donne une
voie exacte pour prouver certaines voies mortes **avant** ce chargement,
avec repli sur le cœur actuel. Son [premier port hors produit](../../receipts/dead_node_credit_negative_20260923/README.md)
est pourtant **négatif** sur 16k sans sol : cœur seul, formes divisées par
4,7 mais CPU +7 %, puis cœur+cover, CPU +27 % à K5. Il ne faut donc pas
rejouer la même borne coûteuse sur ce brut en présumant un gain. Une piste
distincte (borne beaucoup moins chère, arrêt plus tôt, ou partage entre
cœurs apparentés) doit d'abord compter formes réellement évitées, bornes
et visites ajoutées, voies survivantes, cover aval, sorties q3/q4, FULL,
CPU/mur et RSS en ablation appariée. La baisse des formes seule ne suffit pas.

Le repère quadratique homogène `B=Σ(n_morceau/n_plein)²` vaut 0,500 pour
les moitiés et 0,250 pour les quarts. Le rapport
`R=Σ(formes_morceau)/formes_plein` vaut 0,306 et 0,300 respectivement.
Le passage moitiés→plein est défavorable à ce repère ; quarts→plein est
plus favorable. Parmi les six liens parent→enfant, `full→half_x_neg` et
`full→half_x_nonneg` ont des pentes finies des formes de 2,553 et 2,880.
Les sommes des CPU·s morceaux/plein sont 0,900 et 0,874 ; les sommes des
murs chaîne 0,971 et 1,042. Ces rapports spatiaux changent la géométrie
et les frontières : ni eux ni les pentes de densité ne démontrent une loi
asymptotique, et la somme des tours des morceaux n'est pas la tour globale.

Les **12 sondes complémentaires** croisent maintenant les deux densités
réduites avec les six secteurs. Les 21 cas forment la matrice complète
7 secteurs × 3 densités, tous issus des mêmes sélections globales emboîtées.
Chaque pente compare des effectifs **réels** dans un secteur fixe :

| secteur physique | sites 1/4 / 1/2 / entière | p formes 1/4→1/2 / 1/2→entière | p CPU·s 1/4→1/2 / 1/2→entière |
| --- | ---: | ---: | ---: |
| trame entière | 30 847 / 61 694 / 123 389 | 1,823 / **2,136** | 1,218 / 1,297 |
| demi `x<0` | 15 437 / 30 644 / 61 045 | 1,670 / 1,873 | 1,247 / 1,236 |
| demi `x≥0` | 15 410 / 31 050 / 62 344 | 1,258 / 1,473 | 1,209 / 1,298 |
| quart `x<0,y<0` | 7 649 / 15 217 / 30 265 | 1,658 / 1,864 | 1,245 / 1,292 |
| quart `x<0,y≥0` | 7 788 / 15 427 / 30 780 | 1,688 / 1,898 | 1,200 / 1,244 |
| quart `x≥0,y<0` | 7 692 / 15 619 / 31 391 | **2,060** / 1,844 | 1,209 / 1,328 |
| quart `x≥0,y≥0` | 7 718 / 15 431 / 30 953 | 1,731 / 1,981 | 1,314 / 1,307 |

**2/14** liens de densité franchissent `p_formes=2`. Les 14 pentes CPU
restent entre **1,200 et 1,328**, les charges de cœur entre 1,119 et
1,296 et les paires développées entre 1,287 et 1,769. À population
fixée par densité, `R=Σ formes(morceau)/formes(plein)` évolue ainsi :

| densité | repère quadratique moitiés / quarts | `R_formes` moitiés / quarts | `R_charges` moitiés / quarts | `R_CPU` moitiés / quarts |
| --- | ---: | ---: | ---: | ---: |
| 1/4 | 0,500 / 0,250 | 0,549 / 0,365 | 0,968 / 0,951 | 0,917 / 0,872 |
| 1/2 | 0,500 / 0,250 | 0,421 / 0,357 | 0,970 / 0,959 | 0,924 / 0,882 |
| entière | 0,500 / 0,250 | 0,306 / 0,300 | 0,969 / 0,961 | 0,900 / 0,874 |

La part des formes qui disparaît sous la coupe en deux croît avec la
densité : le verrou n'est pas seulement le nombre de cœurs chargés.
Sur les 18 liens spatiaux parent→enfant (six à chaque densité), **7**
ont `p_formes≥2` ; la pente maximale est 2,880 entre la trame entière
et le demi `x≥0` à densité pleine. Le maximum CPU sur ces liens est
1,466. Ces coupes changent les arêtes et les frontières ; elles ne
constituent pas une preuve de complexité du générateur global.

Les 12 nouvelles exécutions ont subi une forte contention de l'hôte
partagé : le demi `x<0` à densité 1/2 a pris 54,95 s mur pour seulement
64,82 CPU·s, alors que ce demi entier prend 28,05 s mur pour 151,96
CPU·s. Les pentes murales entre ces deux cas seraient trompeuses ; les
compteurs déterministes sont le signal de croissance de ce reçu.

`generate.py` produit 21 entrées réversibles sous `/tmp` depuis les sources
v8 versionnées. `run.py` exécute les 21 cas, garde stdout/stderr et le reçu
`CASES.jsonl` ; `summarize.py` vérifie SHA, binaire/commande, options,
entrée FNV, jointure
par IDs, emboîtement strict, reconstruction spatiale et les sorties, puis
calcule `SUMMARY.json`. Préparation Python **hors chronos**. `chain_total_ms`
mesure la chaîne en mémoire et diffère du mur externe Python ; `chain_cpu_s`
est le CPU cumulé de la sonde. Le plein prend 49,174 s de mur externe et
1,93 GiB RSS. Les 21 payloads binaires peuvent être régénérés avec
`python3 generate.py --repo /workspaces/E-HGP --out <répertoire>`. Copier
ensuite `CASES.jsonl` et les 21 `*.stdout`/`*.stderr` archivés dans ce
répertoire. La commande
`python3 summarize.py --repo /workspaces/E-HGP --out <répertoire>` relit le
reçu sans relancer HGP. Ces scripts utilisent des assertions de contrôle :
ils refusent explicitement le mode Python `-O` au lieu de déclarer une
relecture réussie sans ces contrôles. Un rejeu depuis les sources v8
versionnées a reconstruit les entrées, validé les 21 sorties et reproduit
`SUMMARY.json` octet pour octet ; une corruption du hash du binaire et
`-O` ont bien refusé. Ces deux gardes du lecteur et le refus de `-O`
dans les scripts ont été ajoutés **après la capture** ; la génération
normale et les 21 commandes HGP restent celles inscrites au reçu. La
première capture comptait neuf cas ; les douze secteurs décimés ont été
ajoutés ensuite sous le même binaire épinglé, sans modifier les neuf
premières lignes ni leurs sorties.

Le catalogue non émis, K10, plusieurs scènes ou séquences, G4 et le
profil float32 natif restent hors de ce reçu.
