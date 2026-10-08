# MES-B1t : admission des FULL massives, 8 octobre 2026

Les 19 processus et 23 passes FULL complètes de la [livraison L1t](../../g4_mesb1t_20261008/README.md) sont relus : **14 prises réussies, cinq refus mémoire**, dont un après un préfixe valide. Les huit passes chaudes sont des observations uniques, pas huit répétitions par scène. Aucun moteur ni appel distant exécuté par l'audit.

**Source et fermeture.** Paquet `366675f6…`, Git `caf9585e4`, source native et `full_probe.cpp` identiques à `47feedc96` : **R1 est déjà présent**, avant la publication ultérieure de son reçu. 369 fichiers source/build/tests et le lecteur sont reliés au Git, ainsi que le pilote et `banc_full.py`. Archive `63d7d8c3…`, 44 067 octets, 82 entrées du manifeste ; worker et deux commandes code 0, aucun incident du contrôleur, DONE 0, arrêt ciblé certifié. La copie publiée en `8f03d29bb` possède exactement les 38 fichiers JSONL/stderr et les blocs numériques des deux rapports. Les pins complets sont dans [capture.json](capture.json).

Deux commandes, W48/u21, Session recouverte, cache de blocs par défaut 8 Gio et budget hôte 160 Gio :

| Lot | Budget appareil | Processus | FULL complètes | Chaudes | Issues |
| --- | ---: | ---: | ---: | ---: | --- |
| Identité Boreas 10 sans sol | 8 Gio (CPU sans appareil) | 2 | 3 | 1 | 2 succès |
| Quinze scènes / 17 configurations | 88 Gio | 17 | 20 | 7 | 12 succès, 5 refus |

Le pilote prend l'effectif de l'entrée `distinct` lorsque `bundled=distinct` : les tailles et hashes déclarés XYZ/IDs sont raccordés à ces sites, sans ouverture des payloads. Aucune coupe ajoutée par cette campagne ; les masques et regroupements de captures sont ceux du manifeste.

## Temps FULL admis

Mur de `prepare_cloud` à la fin de `build_tower`, catalogue et verticales compris. Lecture des entrées, ouverture Session/GPU/Pool, validation externe, digest et libération restent hors mur. G est une fenêtre pendant laquelle la forêt travaille aussi ; `TMVR` désigne ici la queue après G, pas la somme des temps cumulés T/M/V/R.

| Scène (millions de sites distincts) | K | GPU, secondes | Régime |
| --- | ---: | ---: | --- |
| Boreas 1 sans sol (0,146) | 5 | 0,305449 | chaude |
| Boreas 1 (0,216) | 5 | 0,438878 | chaude |
| Boreas 10 sans sol (1,513) | 5 | 7,184661 | chaude |
| Boreas 10 (2,153) | 5 | 8,827338 | chaude |
| Marseille sans sol (2,465) | 5 | 8,094787 | chaude |
| Scion sans sol (3,439) | 5 | 36,221713 | chaude |
| Meadow (6,181) | 5 | 29,620722 | chaude |
| Scion entier (3,589) | 5 | 30,369913 | froide seule |
| TU Wien entier (6,236) | 5 | 45,543974 | froide seule |
| Marseille entier (6,709) | 5 | 25,076427 | froide seule |
| Boreas 10 sans sol (1,513) | 10 | 38,620355 | froide seule |
| Marseille sans sol (2,465) | 10 | 38,521377 | froide seule |

La commande d'identité sous 8 Gio donne **8,200726 s à chaud sur GPU**, puis **19,483532 s sur CPU en une seule passe froide**. Ces deux temps ne constituent pas une comparaison CPU/GPU à régime apparié. Sur le même Boreas, le lot à 88 Gio donne 7,184661 s à chaud ; ni A/A ni répétitions indépendantes ne permettent d'en faire un gain causal.

Les nombres entiers en nanosecondes, froid/chaud, CPU·s, RSS, pics, budgets et derniers étages sont conservés dans [results.json](results.json). Les statistiques et critères sont recalculés depuis les JSONL : lot principal **B1/B2/B4 non tenus ; B3 non évalué**, verdict `non tenu`, sans écart au rapport. Le code 0 du pilote signifie « rapport rendu », jamais « contrat tenu ».

## Refus et portée des preuves

TU Wien sans sol (5 199 758 sites) donne une première FULL valide de **32,855789464 s**, puis une libération de 1,818896782 s, puis `resource_exhausted/memory_budget`, code 2. La seconde passe demandée ne publie aucune FULL. Ce préfixe froid n'est ni effacé ni qualifié comme prise chaude réussie. NIBIO sans sol/entier (7,794/7,826 M) et Boreas 50 sans sol/entier (7,857/10,767 M) refusent sans FULL. Les sorties ne nomment pas le budget ni l'étage du refus ; la cause reste à établir.

Les empreintes FUL1 demandées jusqu'à 1,6 M sites sont conformes au plan : quatre clés scène/K, constantes entre les passes disponibles. Boreas K5 a la même `49f90d23…` sur CPU, GPU 8 Gio et GPU 88 Gio. FUL1 porte la sortie sémantique encodée (niveaux/forêts/verticales), **pas la CSR des branches R ni les octets internes du catalogue**. Les scènes plus grandes n'émettent pas FUL1 dans ce plan ; leurs lignes FULL réussies et la validation native ne remplacent pas une identité différentielle archivée. Pas de nouveau transfert de qualification numérique u24/u32.

Le même ELF initial `f950fcd9…` est annoncé avant chacune des deux commandes. Le pilote ne rehache pas le binaire en fin de campagne : **fermeture ELF finale non attestée**. Ce manque est distinct des sources, de l'intégrité des journaux et de l'arrêt certifié.

## Mémoire et correction documentaire proposée

`pic_appareil_octets` est le pic du budget propre, remis à zéro à chaque passe FULL ; `appareil_octets` est la capacité gardée en fin de passe. `pic_nvidia_smi_mio` est un maximum échantillonné toutes les 250 ms sur le processus entier, disponible aussi sur un refus et potentiellement inférieur à un pic bref. La colonne du README développeur utilise ce second relevé. Ne pas mélanger les deux : Boreas sous 8 Gio a un pic de budget **6 203 669 056 octets = 5,78 Gio**, contre un relevé SMI de 5,0 Gio ; le préfixe TU Wien sans sol atteint **85 886 193 388 octets = 79,99 Gio** dans le budget, contre 87,6 Gio SMI sur le processus. Le pic hôte actif exclut les blocs inactifs du cache ; RSS est cumulatif sur le processus et exclut la VRAM.

[documentation.patch](documentation.patch), non appliqué, cible seulement le README `8f03d29bb` : préciser les pics et la prise CPU froide, retirer l'affirmation erronée d'un plafond L1p universel à 5 M (Meadow et Marseille entiers passaient déjà), et conserver la mémoire hôte comme hypothèse des refus plutôt que cause établie. Ces corrections ne modifient aucun verdict ni seuil.

## Rejeu

`check.py` réutilise les lecteurs publiés L1r et LF15437, avec un port explicite du schéma recouvert, du pin source, de l'indice de commande et des budgets 8/88 Gio. Il exige les deux cohortes externes exactes, les codes/raisons connus, tout préfixe valide avant refus, les types/champs/temps/mémoires et toutes les empreintes demandées. Les JSONL existent dans le reçu développeur ; aucun brut ni source complet n'est dupliqué ici. La copie persistante de travail ne contient que rapport et journaux métadonnées.

```sh
python -B morsehgp3D_v12/receipts/audit_reponses_20261008/session_b1t_admission/check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.mesb1t --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/mes_b1t_admission_20261008
python -O -B morsehgp3D_v12/receipts/audit_reponses_20261008/session_b1t_admission/check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.mesb1t --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/mes_b1t_admission_20261008
```

Les deux modes rendent les mêmes capture/résultats. La proposition documentaire est vérifiée par application puis inversion sur une copie temporaire du README épinglé. Aucune modification produit ni nouveau banc.
