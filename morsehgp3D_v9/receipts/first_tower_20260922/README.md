# Reçu : première tour FULL v9 sur trames LiDAR sans sol à 1 mm

22 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. GCP non utilisé. Mesure locale sur l'hôte partagé
(AMD EPYC 7763, 8 CPU logiques, 4 cœurs physiques) : ce n'est pas la machine
du contrat.

## Ce qui est mesuré

La chaîne v9 complète (`mhgp9_tower_probe`) : générateur exact q2 + q3/q4,
catalogue canonique recoupé, tour HGP FULL K = 1..K, sur les trois trames
SemanticKITTI 08 sans sol à 1 mm versionnées par la v8
(`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`),
huit fils pour le générateur et le census, tour en un fil (voie temporelle
de la v7). Commit, empreinte de la sonde, CPU hôte et sha256 des entrées :
[`raw/context.txt`](raw/context.txt). Chaque ligne a sa sortie JSON, son
`/usr/bin/time -v` et la charge avant/après.

## Résultats

| trame | sites | K | mur (s) | CPU·s | q3/q4 (s) | tour (s) | boules | nœuds | RSS (Go) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 | 39 885 | 5 | 142,8 | 834 | 128,5 | 10,8 | 1 306 696 | 1 541 750 | 1,06 |
| 000100 | 35 551 | 5 | 131,6 | 668 | 120,1 | 8,9 | 1 095 926 | 1 306 721 | 0,87 |
| 000200 | 45 845 | 5 | 263,9 | 1 456 | 246,5 | 12,9 | 1 407 885 | 1 683 088 | 1,18 |
| 000000 | 39 885 | 10 | 522,6 | 2 539 | 381,2 | 130,0 | 5 512 670 | 7 426 215 | 4,08 |
| 000100 | 35 551 | 10 | 381,3 | 1 888 | 278,1 | 94,3 | 4 383 302 | 5 954 045 | 3,25 |
| 000200 | 45 845 | 10 | 802,2 | 4 214 | 686,9 | 102,8 | 5 483 320 | 7 468 379 | 4,11 |

Toutes les lignes sont `complete_relative` : aucune divergence entre les deux
implémentations recoupées par le catalogue, aucune coquille au-delà de
12 sites (maximum 5 ; 227 à 444 boules à coquille étendue par ligne). q2 prend
1 à 4 s, le census du catalogue 0,7 à 4 s. La charge de l'hôte avant chaque
ligne est consignée ; les deux premières lignes K5 ont démarré sous une
charge résiduelle de 12 à 17 (fin d'une autre suite de tests).

Lecture : le générateur q3/q4 domine (73 à 93 % du mur) ; à K10 la tour en un
fil pèse 13 à 25 %. Les budgets du contrat (1 s sur G4, 48 fils) sont loin :
ces nombres sont la base de temps de la v9, pas une qualification.

Hors reçu, sur la même trame 000000 K5 : la voie statique parallèle de la tour
(`--static=8`) donne le même condensé de tour (`67450c64611075b1`) mais ne
réduit la tour que de 10,8 à 9,3 s, la partie séquentielle dominant. Un profil
gprof (tous fils) attribue environ 54 % du CPU à l'atlas q4 (bornes de nœuds,
partitions de fragments), 13 % au census q3, 16 % aux filtres de témoins et
6 % à la localisation des centres q3.

## Relire

```bash
python3 morsehgp3D_v9/receipts/first_tower_20260922/summarize.py
python3 -O morsehgp3D_v9/receipts/first_tower_20260922/summarize.py
sha256sum -c morsehgp3D_v9/receipts/first_tower_20260922/SHA256SUMS
```

Le lecteur refuse une ligne incomplète, un statut non complet, un nombre de
sites ou d'ordres faux, un catalogue incohérent ou une coquille hors domaine,
et récrit `SUMMARY.json`. Relancer : `run_campaign.sh` depuis la racine du
dépôt, avec `PROBE=<binaire>`.
