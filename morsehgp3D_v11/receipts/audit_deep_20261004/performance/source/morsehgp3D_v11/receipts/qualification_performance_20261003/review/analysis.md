# Analyse indépendante appariée c40 — 3 octobre 2026

Lecture seule du membre `results/cmd/002_paired_full/files/full_paired.json` de l’archive rapatriée. Source `c40f40798375a0fc37917499401f16876cccbd2a`. Aucun produit, natif ou GCP exécuté. Le contrôleur et l’arrêt de VM sont vérifiés séparément ; cette analyse ne les qualifie pas.

81/81 invocations conformes, calendrier complet de 3 trames × 3 producteurs × 3 W × 3 prises ; sorties rapportées identiques par trame pour tous les producteurs. Contrôle indépendant de 405 bilans de pas, 81 bilans de droites, 81 pins binaires inchangés et trois groupes de hashes d’entrée/sortie. Les dumps natifs supprimés ne sont pas rehachés par cette lecture ; le banc les a comparés avant suppression.

| FULL W48, médiane [min–max] en ms | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| baseline | 1309.857 [1304.926–1371.808] | 1070.932 [1068.305–1106.618] | 1419.802 [1414.174–1436.339] |
| current2047 | 843.962 [813.134–846.330] | 678.702 [655.655–679.206] | 894.237 [890.857–901.447] |
| current16379 | 489.099 [468.730–493.124] | 345.066 [333.573–354.942] | 432.397 [415.678–444.127] |

La réduction du mur de 16379 face à la baseline vaut 62,66 / 67,78 / 69,55 %. Face au courant2047 : 42,05 / 49,16 / 51,65 %. Le gain est celui du paquet : graphe, table de populations, ordres concurrents et retrait du mémo changent ensemble. Aucun des 81 murs ne passe 200 ms ; la meilleure prise vaut 333,573 ms.

| Courant16379 W48, médianes indépendantes en ms | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Index | 0.412 | 0.353 | 0.416 |
| Domaine | 227.494 | 181.104 | 238.629 |
| Forêts | 240.789 | 163.588 | 197.193 |
| Classification globale | 2.508 | 2.225 | 2.722 |
| Naissances/états/jobs | 14.346 | 11.401 | 16.874 |
| Résolution régulière | 130.654 | 73.513 | 83.454 |
| Publication | 51.594 | 35.771 | 48.323 |
| Verticales | 41.796 | 36.681 | 28.270 |
| Génération catalogue single_pass | 169.085 | 131.325 | 176.495 |
| Préparation préfixes catalogue | 21.018 | 17.804 | 20.863 |
| Tri catalogue | 11.551 | 9.148 | 13.151 |

Ces lignes ne se somment pas : chaque médiane peut venir d’une autre prise, et les sous-phases catalogue appartiennent au domaine. Par prise, les phases globales forêt sont disjointes ; leur résidu est 4,28–5,32 ms. La variabilité de la résolution régulière ng00 est notable : 100,282–157,699 ms sur trois prises. Ne pas attribuer cette différence à un changement de travail : ses compteurs sont identiques entre ces prises.

Le mur domaine seul est supérieur à 200 ms à chacune des prises ng00 (221,543–266,405 ms) et ng02 (218,045–251,302 ms). Optimiser seulement les forêts ne peut donc atteindre ce jalon sur les deux scènes dans ce pipeline mesuré. Le catalogue single_pass reste le premier poste de génération : 131–176 ms ; le tri prend 9–13 ms et la classification globale 2–3 ms.

| Travail de forêt W48, courant2047 → courant16379 | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| Présentations MEB | 15 697 322 → 3 786 945 | 12 613 143 → 2 885 443 | 15 618 675 → 3 313 726 |
| Tests de points census | 21 014 683 → 21 150 016 | 15 417 314 → 15 518 017 | 14 661 246 → 14 758 563 |
| Appels census | 289 592 → 291 515 | 215 980 → 217 404 | 206 742 → 208 111 |
| Pas de descente | 4 685 673 → 4 797 474 | 3 827 526 → 3 924 630 | 4 772 726 → 4 889 688 |

Les MEB baissent de 75,88 / 77,12 / 78,78 %, avec 3,622 / 3,013 / 3,850 millions de hits de population. K5 concentre 69,69 / 69,15 / 68,87 % des présentations MEB restantes. Les census augmentent légèrement (environ 0,65–0,67 % pour les appels) et les pas augmentent : le mémo à zéro pas de 2047 est absent de 16379. Tous les bilans `census_calls + catalogue_hits + singleton_hits = descent_steps` et `population_hits <= catalogue_hits + singleton_hits` passent. Traces, unions, plateaux, cellules rejouées et réemplois verticaux restent identiques à W48 entre les producteurs.

Le catalogue de 16379 examine 8,20–10,26 millions de candidats q4 mais ne matérialise que 121 303–158 494 niveaux q4 : le calcul différé est déjà actif. Il rapporte 30,24–38,09 millions de tests census et 24,89–31,28 millions d’évaluations de droites. Les préfixes sont logiques ; ces nombres ne fournissent pas à eux seuls une attribution du temps entre tous les filtres physiques.

| Courant16379 W48, ressources et coûts hors FULL | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| CPU FULL, secondes | 13.537 | 10.661 | 12.501 |
| Pic réservé Buffer+Cloud, MiB | 345.969 | 298.672 | 368.336 |
| Processus natif, ms | 1046.463 | 832.181 | 1031.966 |
| Processus moins FULL par prise, ms | 562.526 | 478.944 | 599.569 |
| Inspection sémantique hors processus, ms | 173.099 | 146.406 | 189.495 |

Le pic réservé augmente de 16,413 / 18,933 / 15,516 MiB face au courant2047 ; ce n’est pas la RSS. La table de populations occupe 32,6–37,4 MiB et le mémo n’occupe plus rien ; les phases concurrentes gardent d’autres buffers simultanément. Ne pas additionner ces postes au pic déjà publié. Le CPU FULL correspond à environ 27,7–30,9 CPUs moyens ; ce rapport n’identifie pas un défaut d’ordonnanceur.

Entrée + Cloud + Pool restent faibles (environ 1,4–1,7 ms au-delà de FULL pour Cloud/Pool ; lecture environ 0,2–0,3 ms). L’écart processus−FULL de 479–600 ms comprend principalement d’autres activités natives, dont l’émission des dumps de 255–329 Mo, et la destruction ; aucun champ n’isole exactement le dump. Les 146–189 ms d’inspection sémantique de ces prises sont hors processus natif : 3 décodages initiaux, puis 78 réemplois avec rehachage. La campagne entière dure 530,322 s, compilation et contrôle compris. Aucun de ces coûts ne doit être ajouté au mur FULL comme s’il faisait partie du moteur.

## Priorités proposées

1. Réduire le travail de génération du catalogue. Le domaine dépasse déjà 200 ms sur deux scènes ; mesurer les opérations physiques de préfixes/census et l’équilibrage des feuilles avant de choisir une coupe certifiée par famille. Les minorants de témoins communs pour les extensions q4 sont une piste conditionnelle, à confronter à leur coût total ; ne pas tronquer U, changer les contacts ni réintroduire le calcul de niveaux q4 déjà différé.
2. Cibler les descentes non terminales de K5 dans la résolution régulière, où restent 69 % des présentations MEB et 15–21 millions de tests census. Une réutilisation locale de géométrie ou un meilleur filtrage doit préserver consultation du mémo/table avant MEB, support canonique et dates ; son gain reste à mesurer. Aucun partage de faces ou nouveau cache n’est qualifié par cette comparaison.
3. Publication et verticales sont les postes suivants, 36–52 et 28–42 ms. Leur amélioration peut compléter le gain, mais ne remplace pas le travail sur le domaine et les descentes. Index, classification, naissances et tri ne sont plus les premières cibles.

Le calendrier prouve ici FULL CPU/u21/K5 sur les trois trames entières sans sol de la seule séquence08, grille1mm. Il ne qualifie pas K10, GPU, segmentation/préparation, plusieurs séquences, multi-millions ou projection de points. Une nouvelle optimisation exige ses portes et un comparatif contrôlé ; aucune prévision 100/200 ms n’est déduite de ces résultats.

Contrôle autonome normal et −O : même dérivé SHA256 `a1824809cd95f993557f82c33d5423d56990818abc481c2e4946f193aaa35cb5`. La première tentative du lecteur attendait un champ population_hits absent du producteur historique895 ; seul ce producteur épinglé est maintenant adapté explicitement à zéro. Les champs courants restent obligatoires. Le script adapté à des chemins explicites et la provenance de dérivation sont conservés à côté de cette analyse. Le stderr initial de cette préparation n’a pas été copié avant disparition du scratch /tmp ; aucune qualification native n’en est déduite.
