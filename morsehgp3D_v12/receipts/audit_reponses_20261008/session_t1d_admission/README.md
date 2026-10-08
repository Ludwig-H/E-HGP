# T1-d1 et T1-d2 : admission des journaux, coût C et budgets

Contrelecture du 8 octobre 2026, sans moteur, compilation, données de scènes ni appel distant. Cadre : exploration v12, u21, catalogue CUDA et finition CPU, hors statut public. **T1-d2 satisfait la règle de coût du catalogue préannoncée ; T1-d1 garde son refus de protocole FULL.** Ce reçu ne transforme pas ces essais en nouveau contrat FULL ou multi-millions.

Deux archives fermées : `t1d`, source `5f8e777cffbeddfe90e92fc616a920c28c1b985c`, et `t1d2`, source `c31beaf2200d1a7f8eed09b0c1c7c0f6af3b74f3`. Le plan commun `d43b915a…` prescrit 10 tours, 10 passes, W48. L'archive AVANT est bien **4171b2653**, SHA `048a965d…`, malgré le commentaire historique « 27eca » du pilote. Les 360 fichiers pertinents de cette archive et les 368 de chacun des paquets APRÈS correspondent exactement à leurs objets Git (périmètres et inventaires dans [capture.json](capture.json)).

Chaque retour comporte 219 entrées vérifiées du manifeste. Commande `t1d_flux` code 0, 346,013 puis 348,640 s ; worker/DONE 0, aucune erreur, résultats vérifiés, arrêt ciblé certifié code 0, état RUNNING→TERMINATED. Ces durées sont celles de la commande entière, constructions et portes comprises. Les primaires CTest contiennent 741 Passed, aucun échec ni saut ; les deux portes appareil rendent leurs marqueurs attendus (9 témoins / 14 appels sous budget). Aucun test natif relancé par l'audit.

Les 168 processus de mesure **par campagne** sont fermés : 18 identités catalogue CPU/appareil, 42 prises de flux, 18 FUL1 et 90 prises C décisives. Le lecteur lie les 168 journaux natifs au rapport, à leurs options, à l'entrée déclarée, au bras et aux cardinalités ; total 972 passes catalogue réussies et 36 passes FULL. Les 24 refus mémoire supplémentaires arrivent tous dès la passe 0. Le rapport publié conserve exactement les données numériques du retour. Les codes individuels sont ceux capturés par le pilote épinglé et cohérents avec les fins natives ; les fichiers `.log` ne contiennent pas de second enregistrement indépendant de ces codes.

## Coût catalogue admis

Pour chaque processus, médiane des neuf passes chaudes ; puis médiane des dix valeurs pour la colonne de temps. Les ratios jugés sont des rapports appariés par tour, suivis d'une moyenne géométrique et d'un bootstrap 10 000 tirages (graine 20261008), **pas le quotient des colonnes de temps**.

| T1-d2, K5/W48/appareil | Sites | AVANT C, ms | APRÈS C, ms | GM APRÈS/AVANT | IC95 % non arrondi, borne haute | A/A GM |
|---|---:|---:|---:|---:|---:|---:|
| ng00 | 39 885 | 26,260994 | 26,227960 | 0,9997722451133368 | 1,0034137665321277 | 1,0002011269476487 |
| ng01 | 35 551 | 23,043780 | 23,199119 | 1,0023697150961666 | 1,0062609006893204 | 1,0015178357343206 |
| ng02 | 45 845 | 26,550580 | 26,459969 | 1,0023949581534 | 1,0082419913880922 | 1,002219902818345 |

Les trois bornes sont ≤1,01, et les trois A/A dans [0,985 ; 1,015]. Recalcul indépendant depuis les passes : exactement les GM/IC du rapport. Les deux juges originaux et le [port strict c31 proposé](../t1d_admission/port_c31.patch), appliqué en copie, conservent les verdicts et, pour T1-d2, le jugement complet. T1-d1 conserve 18 refus FULL de schéma/options, sans autre rejet. Aucun seuil postérieur substitué.

`catalogue_probe.cpp:246–307` mesure `build_catalogue_device` avec Cloud, Pool et contexte appareil déjà ouverts ; P, lecture des fichiers, ouverture, empreintes/export, destruction finale sont hors de ce mur C. La préparation des sorties anticipées appartient à la construction. Les 900 sommes diagnostiques C par campagne sont incluses dans leurs murs ; le reliquat reste distinct. Les trois bras utilisent un processus neuf par tour/trame ; la première passe de chaque processus est écartée. Les deux empreintes ELF catalogue avant/après campagne sont égales pour chacun des deux binaires ; A/A emploie le même chemin AVANT. Les sondes FULL n'ont pas de hash ELF séparé archivé par ce pilote. Les observations GPU vide sont deux extrémités, pas une surveillance continue.

## Flux et identités : portée exacte

Par campagne : six prises libres, puis 36 budgets dérivés exactement de leurs capacités (1/2, 1/3, 1/4, 1/6, 1/8, 1/32). Douze prises rendent F2, soit 24 passes, et 24 refusent `resource_exhausted/memory_budget` à la première passe. Aucun préfixe réussi suivi d'un refus n'est présent dans ces primaires ; le garde-fou de préfixe est une protection vérifiée séparément par les témoins du port, pas un scénario natif observé ici.

Chaque passe réussie sous budget a capacité liée à la ligne catalogue, **capacité ≤ pic propre cumulatif ≤ budget**, sans décroissance du pic. Sans budget propre, le zéro publié est une convention : il ne mesure pas un pic appareil nul. Les refus avant résultat ne publient pas ce pic. Il s'agit de la comptabilité du budget du produit, pas d'une mesure externe de toute la mémoire physique ou des métadonnées non imputées. Les valeurs précises figurent dans [results.json](results.json).

| Prise réussie la plus serrée | Budget octets | Pic propre octets | Tranches | Lots d'arène rapatriés |
|---|---:|---:|---:|---:|
| ng00 K5 | 368 545 696 | 345 937 376 | 3 | 1 |
| ng01 K5 | 303 619 960 | 277 692 632 | 3 | 1 |
| ng02 K5 | 372 438 052 | 326 813 116 | 3 | 1 |
| ng00 K10 | 641 906 635 | 614 718 376 | 6 | 6 |
| ng01 K10 | 517 917 027 | 444 652 500 | 6 | 7 |
| ng02 K10 | 618 737 101 | 587 772 136 | 7 | 6 |

Les neuf cas CPU/appareil sans budget propre rendent le même MHGP12DP, les mêmes valeurs des niveaux, réponses `find_support` (`--digest-complet`) et grand livre ; les F2 épinglés concordent. Ces prises d'identité n'émettent pas `tranches`, donc aucun nombre de tranches n'en est déduit. **Les prises sous budget n'activent pas `--digest-complet`** : MHGP12DP n'encode ni les niveaux ni la table (`catalogue_probe.cpp:18–20`). Leur F2, comptes et tranches sont acquis ; l'identité des valeurs des niveaux et des réponses de table sous budget GPU reste à compléter contre la référence CPU. `table_sha256` porte les résultats `find_support` des supports S* émis, pas les octets internes du tableau. Le FULL n'est pas joué sous ces budgets.

Pour chacun des neuf cas, les 18 processus FUL1 / 36 passes sont stables et identiques AVANT/APRÈS ; les six couples ng/K retrouvent la session K. T1-d2 impose `--sequentiel`. T1-d1 émet le schéma recouvert : ses 36 passes sont admises séparément avec LF15437, mais cette compatibilité indépendante n'efface pas le refus de son protocole initial. Deux passes par processus d'identité, aucun nouveau banc FULL de latence, ni nouvelle campagne CPU seul.

Les entrées sont ng00–02 ci-dessus et uniformes 8k/16k/32k ; aucun essai T1-d massif. La conséquence du README développeur « toute scène de plus de 5 M refusée par L1p » est trop large : [L1p admise](../session_l1p_admission/README.md) réussit Meadow 6 181 091 sites (30,564533 s chaude) et Marseille brut 6 709 045 (26,602018 s froide). Aucune borne universelle de taille n'en découle. Prochaine preuve utile : mêmes budgets avec empreintes des niveaux et réponses de table, puis FULL massif explicite ; cela ne change pas rétroactivement le critère C adopté ici.

## Relecture

[check.py](check.py) lit exclusivement Git, les deux paquets de sources, les métadonnées locales et archives de résultats ; aucun payload de scène. Le lecteur FULL renforcé et le helper de manifeste déjà publiés sont réutilisés, leurs hashes sont épinglés. Le patch c31 est référencé, jamais dupliqué. Python normal et `-O` rendent le même résultat.

```sh
python -B check.py --repo /workspaces/E-HGP --sessions /workspaces/.ehgp-sessions --patch ../t1d_admission/port_c31.patch
python -B -O check.py --repo /workspaces/E-HGP --sessions /workspaces/.ehgp-sessions --patch ../t1d_admission/port_c31.patch
```

Les archives externes restent nécessaires au rejeu ; aucun journal natif, compte, cible distante, ELF ni donnée de scène n'est recopié dans ce reçu. Les seules modifications proposées au produit appartiennent au reçu du juge, pas à cette admission.
