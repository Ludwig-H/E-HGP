# Deux sessions G4 S1 : filtre exact mesuré, tour toujours ouverte

23 septembre 2026. Contrelecture B, sans nouveau GCP, des reçus
[`attempt1`](../receipts/g4_gpu_s1_attempt1_20260923/README.md) et
[`S1 positif`](../receipts/g4_gpu_s1_20260923/README.md). Le premier
manifeste a 73/73 empreintes valides : session SPOT réellement démarrée,
mais configuration CMake refusée avant compilation CUDA, aucun masque
ni temps GPU. L'arrêt ciblé puis `TERMINATED` sont reçus. La correction
CUDA17 est validée sur la **deuxième** session, non par les seuls logs
de replay local annoncés dans le premier README.

Le second manifeste a **166/166 empreintes valides** ; les sources
avant/après, les 164 fichiers de code/protocole du commit exécuté
`6e0e43a0d`, les trois entrées 1 mm et les dépendances compilées
concordent. Hôte et worker sont `completed`, le préflight natif positif
sur 431 037 paires, le mutant `pair_mask` détecté sur exactement une
paire, les six sorties en code 0, et la VM est arrêtée sur la bonne
génération. Les six cas rendent **zéro désaccord de masque** GPU/CPU
et les mêmes totaux de visites ; le cache CPU donne aussi les mêmes
masques. Aucune incohérence arithmétique observée dans cette portée.
Les faux drapeaux `GCP_used=false`/`GPU_executed=false` du `PACKAGE.json`
sont ceux du **paquet préparatoire**, non le statut des reçus d'exécution.

| 08 sans sol, 1 mm | K | rectangles | paires | GPU filtre, ms | CPU filtre avec cache, 48 fils, ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000000 | 5 | 3 133 819 | 23 686 751 | **63,801** | 1 178,989 |
| 000100 | 5 | 2 348 056 | 11 960 420 | **43,388** | 810,312 |
| 000200 | 5 | 2 964 033 | 22 722 345 | **64,175** | 1 156,693 |
| 000000 | 10 | 4 782 714 | 30 777 213 | **103,039** | 2 222,484 |
| 000100 | 10 | 3 549 478 | 17 488 839 | **70,265** | 1 537,491 |
| 000200 | 10 | 4 695 935 | 32 789 701 | **106,756** | 2 539,181 |

La porte S1 annoncée d'avance (08/000000/K5, filtres rectangles et
paires sans cache sous 100 ms transferts compris) est **franchie** :
63,801 ms, ×18,48 contre le filtre CPU cache actif de la sonde. Tous
les K5 sont sous 100 ms, un seul K10 sur trois l'est. Le GPU traite
toutes les paires sans cache ; sa population et ses masques concordent
avec la référence. Les frontières de jobs du cache CPU de la sonde
diffèrent légèrement de la chaîne R11 (7 161 593 recherches contre
7 162 322 sur 000000/K5), donc ×18–24 est un rapport **de filtre
isolé**, pas un speedup de chaîne appariée.

`gpu.total_ms` est le meilleur de trois passages **après** création du
contexte CUDA et allocation initiale : H2D, filtres, scan et D2H sont
inclus. `first_total_ms` commence lui aussi après ces opérations :
« passe froide » dans le README du reçu et commentaire `cold` du code
ne signifient **pas** un vrai démarrage froid. Les premiers passages
vont de 64,084 à 109,044 ms. `gnu_time_max_rss_kb` mesure la RAM hôte,
pas la VRAM. Le mur du probe (3,773–10,733 s) paie en outre front CPU
séquentiel et trois références CPU. Le temps GPU exclut garde de l'index,
front WSPD, allocations initiales, survivants CPU, cœur/certificat,
catalogue, FULL et condensé.

Ces trois entrées sont des **sous-nuages sans sol** Patchwork++ de la
seule séquence 08 (35–46 k sites), **pas** les trames brutes entières
de 123–126 k retours ; `full.u32le` signifie ici tout le sous-nuage
retenu. Segmentation et préparation 1 mm sont hors du chrono GPU.
R11, sur ces mêmes sous-nuages mais autre exécution CPU G4, laisse
`chain_s−q34_s=0,874/1,062/1,106 s` à K5 et au moins 3,235 s à K10.
Si les autres phases restent inchangées et séquentielles, même supprimer
gratuitement tout q3/q4 ne suffit pas sur toutes ces lignes. Une
substitution **arithmétique inter-reçus**, non une mesure hybride, du
filtre CPU par S1 sur 000000/K5 suggère environ **2,095 s** avant
raccord. Aucun contrat FULL, 1 s, 100 ms ou passage à l'échelle massif
n'est acquis. La prochaine preuve est la chaîne intégrée appariée,
avec identité/catalogue/digest et temps mur complet, puis tuilage et
garde d'index linéaire ; voir les [portes S2](PROPOSITION_B_GPU_STREAMING_S2_20260923.md).
