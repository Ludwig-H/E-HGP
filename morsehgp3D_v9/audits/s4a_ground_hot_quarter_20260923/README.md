# S4a CPU, quart physique chaud 08/000200 : reçu apparié

24 septembre 2026. **Six cas temporels clos, sept tentatives conservées.**
Les trois densités ont chacune un bras S3 et un bras S3+S4a du même binaire
CPU. La tentative 3 (`s1_half/s3`) a une sortie valide, mais son chrono est
rejeté pour contention : charge 1 minute 7,29→13,22, 70,387 s mur pour
77,024 CPU·s. La tentative 4 reprend ce bras à 11,390 s mur et 76,136
CPU·s ; aucun fichier de la tentative 3 n'a été réécrit. Les six chronos
retenus satisfont la garde de charge 1 minute ≤8 avant et après le cas.
Le statut `complete_relative` des sorties est relatif au catalogue contrôlé,
pas une qualification du contrat FULL.

Le panneau minimal fixe le quart `x≥0,y<0` par les signes float32 physiques
de SemanticKITTI 08/000200, **après** le masque sans sol de la trame entière.
Les points donnés à HGP restent les coordonnées u18/1 mm de cette trame, dans
leur ordre d'origine. Le classement global des 45 845 sites retenus par
`splitmix64(ID_original_grille XOR 7d1c9a5eb3f24680)` donne deux densités
emboîtées ; le quart physique est intersecté ensuite. Le générateur existant
[`run_and_check.py`](../lidar_ground_hot_quarter_multiseed_20260923/run_and_check.py)
et son [manifeste](../lidar_ground_hot_quarter_multiseed_20260923/MANIFEST.json)
sont épinglés par SHA dans `PREPARED.json`. La génération et les contrôles de
SHA/FNV ont réussi. `inputs/` contient uniquement ces trois payloads :

| Entrée | Sites | SHA-256 des points | FNV-64 sonde | Digest de tour v12 |
| --- | ---: | --- | --- | --- |
| `s1_quarter.u32le` | 3 609 | `a5cdb880271d3100b618274d762d54b2d621d87c71e7cf76b82b474e9185cf06` | `3832d655049a8e97` | `8684be3da6e71984` |
| `s1_half.u32le` | 7 387 | `ed85a028af0a1a1daf3c3a17cbfea7d484f2428e7c5a916a2f3b4b6e3bb104d2` | `9b54bf67cf6c6787` | `5d1e579b4817cb2a` |
| `physical_full.u32le` | 14 828 | `f4b372962872a91575010af427ff101fb018a62f05dcf2c537bccf07c55e5bb0` | `d42e56787f9a03a0` | `06a78c55c8c65435` |

Les deux premières références viennent du
[reçu multi-graine](../lidar_ground_hot_quarter_multiseed_20260923/README.md),
la dernière du [reçu de coupe physique](../lidar_scene02_physical_cut_20260923/README.md).
Leurs sorties v12 ont le statut `complete_relative`. Leur catalogue n'avait
pas de digest canonique publié ; le lecteur compare celui des deux bras v20
sur **la même entrée**. Il contrôle le digest de tour v12, les dix ordres,
les objets catalogue et générateur et sept compteurs structurels communs
S3/S4a. Les comptes q3 propres à chaque chemin restent séparés. Les trois
digests de catalogue v20 communs aux bras sont respectivement
`0483ce1f9b3bbf3e`, `745214ebc4600418` et `d8d9bde1214ee3af` ; les
catalogues ont 309 932, 698 589 et 1 639 642 boules.
Le JSON v12 du plein physique porte l'ancien libellé `grid=unspecified` ; ses
octets u18/1 mm, son FNV et son SHA sont ceux vérifiés ici.

Le binaire local CPU Release épinglé est
`build/v9-open-worktree/build/v9-exp/mhgp9_tower_probe`, SHA-256
`eea3040cf4150599ca7ab5c71a4f0e3738f2975483584527f339d3d2600d08e9`.
Le snapshot source épinglé est `7ceadffad1de860e30325ae357ba3243f269d48b`.
Avant chaque sonde, le runner relit **dans ce commit** les objets Git de
`src/`, `bench/tower_probe.cpp` et `CMakeLists.txt`, puis le SHA du binaire ;
il journalise le HEAD courant sans lui confier l'identité du binaire figé.
Ce binaire est distinct du
binaire G4 R15 (`9e7c0265…`) et du v12 CPU (`e1ba126f…`). Les commandes
S3 et S4a n'activent aucun levier GPU et gardent tous les leviers CPU v12
ainsi que S2/S3. S4a ajoute seulement `q34_batch_q3=1` ; chaque bras utilise
K10/s8/W8/static8, `--grid=1mm`, `--catalogue-digest` et `nice 19`.

Depuis la racine du dépôt, pour préparer/reprendre et relire le reçu :

```sh
python3 -B morsehgp3D_v9/audits/s4a_ground_hot_quarter_20260923/run_and_check.py prepare
python3 -B morsehgp3D_v9/audits/s4a_ground_hot_quarter_20260923/run_and_check.py run --max-load=8
python3 -B morsehgp3D_v9/audits/s4a_ground_hot_quarter_20260923/run_and_check.py verify
python3 -B -O morsehgp3D_v9/audits/s4a_ground_hot_quarter_20260923/run_and_check.py verify
```

`run` refuse de démarrer un cas si la charge moyenne 1 minute dépasse 8
(réglable par `--max-load`) ou si le code épinglé, le binaire, les références ou les
entrées ont dérivé. Son délai est de 900 s par cas. Il exécute les six cas
dans l'ordre de densité, S3 puis S4a. Chaque tentative reçoit ses propres
fichiers `attempt_XXXX_*.stdout/stderr` ; une reprise ne réécrit pas un échec.
`CASES.jsonl` garde commandes, SHA, issues, mur externe et CPU de **toutes** les
tentatives. Le verdict de sortie géométrique et l'acceptation du chrono sont
séparés : une charge supérieure au seuil à la fin d'un cas conserve sa sortie
mais exclut son temps, puis arrête la campagne. Les trois premières lignes
précèdent l'ajout de ces deux champs explicites ; le lecteur infère le rejet
de la troisième de sa charge après calcul. Après les six succès temporels,
`verify` contrôle aussi les fichiers des échecs et des chronos rejetés,
recalcule les pentes finies avec les effectifs réels et scelle
`SUMMARY.json` et `SHA256SUMS` sur toutes les tentatives. La relecture
archivée ne consulte ni HEAD, ni le binaire vivant, ni les sorties v12
externes : elle utilise `PREPARED.json`, les sorties capturées et les entrées
régénérées depuis les sources v8 avec le générateur historique épinglé.
Le lecteur ne juge pas la complétude des clés absentes du catalogue. Les
chronos excluent la segmentation et la génération des entrées. Les
relectures `verify` normale et `-O` passent ; `sha256sum -c SHA256SUMS` dans
ce répertoire valide les 21 fichiers scellés (runner, manifeste, journal,
résumé, trois entrées et quatorze sorties d'essais).

| Sites | S3 mur / chaîne / CPU | S4a mur / chaîne / CPU |
| ---: | ---: | ---: |
| 3 609 | 6,613 s / 6,406 s / 27,954 CPU·s | 5,319 s / 5,121 s / 28,021 CPU·s |
| 7 387 | 11,390 s / 10,945 s / 75,690 CPU·s | 12,447 s / 11,989 s / 76,004 CPU·s |
| 14 828 | 30,116 s / 29,067 s / 202,212 CPU·s | 31,538 s / 30,482 s / 202,498 CPU·s |

Avec `p=ln(y₂/y₁)/ln(n₂/n₁)` sur les tailles **réelles** 3 609→7 387 puis
7 387→14 828, les pentes de temps mur sont 0,759/1,395 pour S3 et
1,187/1,334 pour S4a. Celles du temps chaîne sont 0,748/1,402 et
1,188/1,339 ; celles des CPU·s sont 1,391/1,410 et 1,393/1,406.

| Travail discret | 3 609 → 7 387 → 14 828 | Pentes adjacentes |
| --- | ---: | ---: |
| `core_sites`, identique S3/S4a | 34 672 784 → 153 447 866 → 582 997 435 | 2,077 / 1,916 |
| `expanded_pairs`, identique S3/S4a | 929 662 → 2 944 182 → 9 794 837 | 1,609 / 1,725 |
| `q3_edges`, identique S3/S4a | 115 292 → 238 333 → 483 205 | 1,014 / 1,014 |
| `lanes_census_point_tests`, S4a seul | 43 220 043 → 130 415 570 → 372 783 993 | 1,542 / 1,507 |
| `lanes_seed_tests`, S4a seul | 17 925 032 → 50 729 255 → 132 932 042 | 1,452 / 1,383 |

S4a décide les 115 292/238 333/483 205 voies q3 demandées sans report.
Son `q34_batch.edges_ms` baisse d'environ 20 % sur les trois tailles,
mais son temps mur total n'est meilleur que sur le plus petit cas :
−19,6 %, puis +9,3 % et +4,7 % face à S3. Il s'agit d'un passage par bras
sur des densités emboîtées d'**une** seule coupe physique, sans isolation
stricte de l'hôte. La garde de charge et la tentative rejetée rendent
la sélection des chronos explicite ; elles ne constituent pas des répétitions
statistiques. Ces pentes finies ne bornent pas la croissance générale et ne
qualifient ni une trame entière, ni FULL, ni G4/GPU.
