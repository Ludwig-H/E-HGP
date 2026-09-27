# Vrais manifestes A : mesures R1 du 27 septembre 2026

Les quatre captures passent : **120 comparaisons champ à champ** des sorties
min-label et événementielles aux sorties A natives capturées, soit cinq K
et six appels candidats par entrée. Min-label est plus rapide et utilise
moins de capacité observée que le prototype événementiel sur ces manifestes.
Cette comparaison ne remplace aucun constructeur moteur et ne mesure aucun
gain FULL, GPU ou G4.

Cadre : `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `real_catalogue_serial_A_comparison`, `not_claimed`.
GCP non utilisé. Sources, builds et reçus R1 figés.

## Preuves et convention de mesure

La [qualification](receipts/r1/qualification/summary.json) ferme 69 commandes
et 489 dépendances pré-épinglées, Release et Clang ASan/UBSan/LSan. Chaque
profil passe six petits cas, 26 ordres et 156 comparaisons, avec 35 positions
d'index non identitaires, deux continuations et 16 blocs non réguliers ;
les deux smokes uniforme64 et les quatre refus CLI passent aussi.
La reprise LIVE normal/−O passe sans reconstruire ; les 35 corruptions du
lecteur sont rejetées dans chacun des deux modes. Les quatre mesures
suivantes passent leurs lecteurs LIVE normal/−O et réutilisent uniquement
le binaire Release qualifié.

| entrée | sites | hash u64 des points | boules du catalogue | reçu brut |
| --- | ---: | ---: | ---: | --- |
| ng00, 08/000000 sans sol entière | 39 885 | 9245360528374966039 | 1 306 696 | [ng00](receipts/r1/ng00/measure.stdout) |
| uniforme, seed3 | 8 000 | 11005134305876494197 | 594 386 | [8k](receipts/r1/uniform_8000/measure.stdout) |
| uniforme, seed3 | 16 000 | 10721094081198136556 | 1 234 454 | [16k](receipts/r1/uniform_16000/measure.stdout) |
| uniforme, seed3 | 32 000 | 4734505976279850663 | 2 531 823 | [32k](receipts/r1/uniform_32000/measure.stdout) |

Toutes les entrées utilisent K=1..5, s8, W4 pour l'amont et le Builder natif.
Les candidats sont **séquentiels**, un manifeste K à la fois. Une capture
par entrée ; trois répétitions internes par candidat et K, dans l'ordre
M/E, E/M, M/E. Ici M désigne min-label et E le constructeur événementiel R1.
Les tableaux donnent la médiane des trois appels, puis éventuellement la
somme de ces médianes sur K. Cette somme n'est ni un mur parallèle, ni la
médiane d'une tour complète. Les appels incluent leurs allocations,
validations, tris et libérations internes ; les destructions de résultats
restent séparées. Les compteurs Work viennent du premier appel, sans
multiplication par trois.

Hôte local partagé, sans isolation ni campagne de répétitions de processus.
Les quatre géométries ont été exécutées séquentiellement ; les relectures
LIVE des trois premiers reçus ont chevauché la mesure 32k. Les temps ne
constituent donc pas une mesure dédiée de débit ni un gain portable.

## Volumes réellement consommés

V compte les sommets du manifeste (sites initiaux de K1 et blocs), E toutes
les occurrences de représentants, G les groupes et C les contributions.
Ces masses sont additionnées sur K=1..5, sans assimiler boules, blocs,
occurrences ou nœuds de sortie.

| entrée | somme V | somme E | somme G | somme C |
| --- | ---: | ---: | ---: | ---: |
| ng00 entière | 2 204 648 | 3 621 785 | 2 200 265 | 897 776 |
| uniforme8k | 967 085 | 1 760 607 | 967 030 | 372 698 |
| uniforme16k | 2 004 457 | 3 662 704 | 2 004 066 | 770 002 |
| uniforme32k | 4 106 113 | 7 519 475 | 4 103 556 | 1 574 290 |

Les trois domaines de parents sont distincts : incidences événementielles
(y compris événements muets), incidences du draft (continuations comprises),
arcs de la forêt explicite (sans continuations).

| entrée | somme P_event | somme P_draft | somme P_forest | historiques M | groupes sans historique M |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 entière | 2 200 260 | 1 541 745 | 1 541 745 | 1 541 750 | 658 515 |
| uniforme8k | 967 025 | 629 399 | 629 399 | 629 404 | 337 626 |
| uniforme16k | 2 004 061 | 1 301 789 | 1 301 789 | 1 301 794 | 702 272 |
| uniforme32k | 4 103 551 | 2 660 307 | 2 660 307 | 2 660 312 | 1 443 244 |

Dans ces quatre captures seulement, aucune continuation n'est présente :
P_draft=P_forest. Les deux continuations de la petite gate restent le test
positif de leur conservation. Les sommes sur K des blocs non réguliers
valent respectivement 523/3/2/0. L'absence de continuation dans ces mesures
n'autorise pas à supprimer cette sémantique.

Détail de ng00 par K, avant toute agrégation :

| K | V | E | G | P_event | P_draft | P_forest |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 141 023 | 202 227 | 139 743 | 139 742 | 79 680 | 79 680 |
| 2 | 267 355 | 410 863 | 266 516 | 266 515 | 178 126 | 178 126 |
| 3 | 415 755 | 673 257 | 414 903 | 414 902 | 285 909 | 285 909 |
| 4 | 590 629 | 985 266 | 589 874 | 589 873 | 421 660 | 421 660 |
| 5 | 789 886 | 1 350 172 | 789 229 | 789 228 | 576 370 | 576 370 |

## Temps des candidats et travail du harnais

Durées en millisecondes, sommes des médianes K sauf mention contraire :

| entrée | appels M | appels E | rapport E/M | destruction résultat M | destruction résultat E |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 entière | 3 148,510 | 4 099,105 | 1,302 | 6,062 | 9,072 |
| uniforme8k | 1 156,023 | 1 449,603 | 1,254 | 7,472 | 8,159 |
| uniforme16k | 2 957,628 | 3 630,904 | 1,228 | 10,199 | 15,501 |
| uniforme32k | 8 025,170 | 10 101,250 | 1,259 | 24,878 | 30,788 |

Sur ng00, M baisse de 23,2 % la somme des temps d'appels face à E.
Par K, les médianes M/E sont 105,963/135,220 ; 287,515/392,965 ;
620,215/732,043 ; 870,158/1 187,879 ; 1 264,659/1 650,999 ms.
La validation reste coûteuse : les sommes des médianes de sa phase sont
1 027,304 ms pour M et 946,926 ms pour E. Les phases parents valent
599,253 et 1 367,290 ms ; les ancêtres 198,899 et 536,278 ms.
Les médianes par phase ne s'additionnent pas nécessairement à la médiane
totale. Tous les appels et toutes les phases figurent dans les reçus bruts.

Le harnais paie aussi les vrais catalogues, leur copie, la reconstruction
exacte de l'index, la capture native **sans sceau**, les manifestes, leurs
validations et toutes les comparaisons. Les durées suivantes sont celles
d'une exécution du harnais, et non celles d'un candidat FULL :

| entrée | chaîne+cata, mur | capture FULL native instrumentée, mur | construction manifestes, somme K | binding, somme K | processus complet |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 entière | 25 456,009 | 1 612,897 | 3 637,637 | 1 570,124 | 54 812,592 |
| uniforme8k | 4 975,655 | 580,422 | 1 284,701 | 542,113 | 15 527,449 |
| uniforme16k | 10 865,140 | 1 318,687 | 3 658,434 | 1 535,964 | 37 843,975 |
| uniforme32k | 26 214,309 | 2 950,579 | 10 900,644 | 4 181,683 | 100 832,518 |

Pour ng00, reconstruction d'index 12,708 ms ; copies de capture 964,690 ms
en **somme de hooks**, déjà incluses dans la capture native. Les comparaisons
des 30 appels candidats prennent au total 85,558 ms, la libération des
manifestes 15,946 ms et celle des slots possédés 48,431 ms. `external`
vaut 54 810,712 ms ; `process` ajoute entrée et libération des points.
Ni `external` ni `process` n'inclut la sérialisation JSON finale.

Les lots A natifs sont instrumentés et les K se recouvrent. Leurs temps
par K ne sont donc **pas une baseline comparable** aux candidats séquentiels.
Ne soustraire ni `capture_copy_sum` du mur natif, ni une somme de phases K
du mur FULL. La copie `keep_catalogue` reste dans le mur de chaîne ; sa durée
isolée n'est pas mesurée. Masque sans sol et préparation 1 mm sont figés,
hors de tous ces chronos. Aucun temps de ce tableau ne remplace le chrono
du moteur de production ou une mesure G4.

## Mémoire : capacités séparées du RSS

Mo décimaux (1 Mo = 1 000 000 octets). Capture initiale = toutes les entrées
et sorties A possédées avant consommation des K. Les colonnes manifeste,
M et E sont le maximum observé sur K, sans somme de maxima entre objets.
Les capacités candidates combinent leurs temporaires et leur sortie,
excluent l'entrée, la pile, l'allocateur et l'intérieur des réallocations.

| entrée | index | catalogue | capture initiale | manifeste max | M combiné max | E combiné max | pic RSS processus, KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 entière | 7,624 | 292,700 | 840,653 | 125,989 | 210,372 | 290,674 | 1 542 092 |
| uniforme8k | 1,351 | 133,142 | 400,680 | 62,947 | 102,311 | 140,588 | 741 500 |
| uniforme16k | 2,702 | 276,518 | 809,552 | 125,893 | 216,953 | 300,856 | 1 395 828 |
| uniforme32k | 5,404 | 567,128 | 1 629,994 | 251,786 | 457,628 | 635,663 | 2 725 940 |

Sur ng00, capacité combinée maximale M inférieure de 27,6 % à E.
L'index, le catalogue et la forêt native sont libérés avant les candidats ;
les deux candidats et leurs résultats ne coexistent pas. La capture conserve
encore domaine et métadonnées de catalogue par K : les 840,653 Mo restent
payés, sans partage ajouté. Les colonnes ci-dessus ne s'additionnent donc
pas en un pic mémoire global.

## Croissance observée sur uniforme8k/16k/32k

Les ratios sont `valeur(2n)/valeur(n)`. Les compteurs sont les sommes sur K,
les temps candidats les sommes des médianes d'appels. Les capacités candidates
sont leurs maxima sur K, et le RSS celui du processus entier.

| grandeur | 8k → 16k | 16k → 32k |
| --- | ---: | ---: |
| boules du catalogue | 2,077 | 2,051 |
| V | 2,073 | 2,048 |
| E | 2,080 | 2,053 |
| G | 2,072 | 2,048 |
| C | 2,066 | 2,045 |
| P_event | 2,072 | 2,048 |
| P_draft | 2,068 | 2,044 |
| P_forest | 2,068 | 2,044 |
| pas DSU M | 2,084 | 2,056 |
| entrées ancêtres M | 2,185 | 2,154 |
| pas ancêtres M | 2,185 | 2,152 |
| pas prédécesseurs M | 2,255 | 2,181 |
| temps appels M | 2,558 | 2,713 |
| temps appels E | 2,505 | 2,782 |
| construction manifestes | 2,848 | 2,980 |
| capacité M | 2,121 | 2,109 |
| capacité E | 2,140 | 2,113 |
| capture initiale | 2,020 | 2,013 |
| mur chaîne+cata | 2,184 | 2,413 |
| mur processus complet | 2,437 | 2,664 |
| pic RSS processus | 1,882 | 1,953 |

M conserve 17 903 320 / 39 125 281 / 84 269 249 entrées d'ancêtres sur
l'ensemble des K ; E en conserve exactement deux fois autant. Les pas
d'ancêtres M valent 53 273 628 / 116 385 392 / 250 419 196 et ceux de
prédécesseurs 35 252 497 / 79 483 787 / 173 346 996. L'index `V log V`
et les tris/DSU sériels restent donc des coûts effectifs, même lorsque
la taille du manifeste croît ici presque linéairement.

Ces trois tailles d'une famille synthétique ne prouvent aucune borne
sous-quadratique générale et ne mesurent pas une croissance LiDAR. Ng00
est une seule trame sans sol, pas plusieurs scènes ou séquences. Les masses
et temps observés exposent le coût de validation, de préparation et des
tables d'ancêtres, mais **ne justifient pas un port moteur comme gain acquis**.
Ce lot ne comporte aucune capture Nsight. La priorité suivante est de
profiler la chaîne FULL avant de choisir une refonte importante ; capture,
validation, construction des manifestes, B/C, géométrie et encodage FULL
doivent garder leurs périmètres distincts. Aucun nouveau chantier de
parallélisme n'est ouvert par ces seules mesures.
