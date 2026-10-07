# Préchargement du nœud des graines par les publieurs : règle manquée de peu, retiré (session claudepref1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudepref1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudepref1/receipt.json`). Source : `1950c3727`, contre la variante `base` (archive de `22a9e00c7`).
Trames LiDAR réelles ng00, ng01 et ng02, W48, modes de référence avec cache de blocs. Aucune mesure ne promeut un
statut public. Le code est retiré par `caca9d8d7`.

## Objet

Le profil échantillonné de `claudecache1` donne au publieur de l'ordre 5, chemin critique de l'étage des forêts, 145 ns
par cellule régulière. `regular_cell` lit le rang du nœud de chaque graine (contrôle d'invariant) dans `result.nodes_`,
un tableau d'environ 17 Mo à K5 lu au hasard. Le préchargement des publieurs (16 jobs en avance) couvrait `parents` et
`states`, mais pas ce tableau. Le levier y ajoutait le nœud de la graine.

## Règle écrite dans le plan avant la session

Exactitude : bancs conformes, vidages identiques entre variantes et égaux aux empreintes. Statistique : médiane des 6
processus à froid, modes de référence K5 CPU feuilles 16 (`802811`) et K5 GPU feuilles 24 (`868347:400`), trois
trames, soit 6 rapports new/base. Gardé si la moyenne géométrique des 6 rapports du temps estimé des cellules
régulières du publieur 5 (`pipeline.orders[4].publish_cells_est_ms`) est ≤ 0,85, et celle de `forest_ms` ≤ 1,00. Juge :
`claudepref1/judge.py`.

## Verdict : règle manquée, retiré

Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10.

| Mode | Trame | Cellules du publieur 5 base → new (ms) | Rapport | `forest_ms` base → new | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 66,0 → 58,3 | 0,883 | 116,1 → 117,3 | 1,011 |
| CPU, 16 | ng01 | 48,7 → 40,4 | 0,830 | 95,9 → 91,7 | 0,956 |
| CPU, 16 | ng02 | 63,7 → 51,4 | 0,806 | 117,7 → 112,5 | 0,956 |
| GPU 400 ‰, 24 | ng00 | 62,8 → 55,8 | 0,888 | 116,6 → 112,4 | 0,964 |
| GPU 400 ‰, 24 | ng01 | 50,6 → 46,4 | 0,918 | 92,8 → 95,5 | 1,030 |
| GPU 400 ‰, 24 | ng02 | 60,3 → 52,4 | 0,868 | 116,4 → 116,4 | 0,999 |

Moyennes géométriques : cellules du publieur 5 **0,865** (seuil 0,85, non atteint) ; `forest_ms` 0,986. Retiré.

## Lecture (descriptive)

Le coût des cellules baisse sur les six rapports, de 8 à 19 %. L'effet paraît réel, mais il reste en deçà du seuil
écrit. Les clôtures de plateaux, environ 29 ms au publieur 5, relisent et rattachent elles aussi un nœud lu au hasard
pour chaque composante fusionnée. Un levier distinct, qui précharge ensemble le nœud des graines et le nœud courant de
chaque composante touchée, sera jugé sur des prises neuves, avec sa propre règle écrite avant elles. Il reprend le
levier retiré ici, et le dit.

## Pièces

`claudepref1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `gpu_ab_report_ab_k5_16_cpu.json`,
`gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
