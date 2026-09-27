# FULL G4 : noyau diamétral ON/OFF et répétitions résidentes

Capture close et rejugée. Source `ddf4776d754a8db59a1333e11d56b39d8cb6f51a`.
Une seule trame : 08/000000 sans sol, 39 885 sites, grille 1 mm,
tour K1..5 explicite, s8, 48 workers. Quatre processus GPU et deux
témoins moteur CPU. Deux processus indépendants par bras GPU ;
quatre calculs de la même trame dans chacun, pas quatre scènes.

| Bras | Premier passage, médiane de deux processus (ms) | Médiane chaude par processus, puis médiane (ms) |
| --- | ---: | ---: |
| noyau ON | 943.023 | 923.417 |
| noyau OFF | 993.803 | 975.936 |

Les trois digests et tous les objets jugés sont égaux dans les 18
passages. Le registre des certificats de chaque bras GPU est aussi
comparé directement au témoin moteur de même option, y compris OFF.
Les sous-chronos détaillés décrivent seulement le premier passage.
Lecture, segmentation et digests restent hors chaîne ; le mur des
processus est publié séparément. Aucun nouveau contrat multis-scènes,
100 ms ou sous-quadratique global n’est certifié par ces répétitions.

Allocation GCE observée : 248.353 s.
Arrêt de la génération `2026-09-26T22:45:14.960-07:00` relu
`TERMINATED` ; ce temps n’est pas une facture.

Lecteur : `morsehgp3D_v9/audits/b_gpu_next_20260927/readback.py`,
normal puis `python3 -O`, avec le snapshot privé indiqué par son hash
dans `SUMMARY.json`. Le snapshot se reconstruit depuis le commit
et `plan.json`, vers un répertoire neuf. Aucun KITTI, snapshot,
archive ou clé SSH n’est ajouté à la v9.
