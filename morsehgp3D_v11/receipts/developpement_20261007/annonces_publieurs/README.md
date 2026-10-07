# Annonces des publieurs tous les 1024 plateaux : règle non atteinte, retiré (session claudeann1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudeann1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudeann1/receipt.json`). Source : `04b00810d`, contre la variante `base` (archive de `3e6f88c7f`).
Trames LiDAR réelles ng00, ng01 et ng02, W48. Aucune mesure ne promeut un statut public. Le code est retiré par
`06fdf8013`.

## Objet

La chronologie G4 de `claudebirths1` montrait les publieurs des ordres 3 à 5 en calcul pendant tout le pipeline, sans
attendre : 86 à 100 ms. Chaque publieur annonçait sa progression aux balayages verticaux tous les 32 plateaux clos, par
`notify_all`. Mesuré en local (W12 à W16), un réveil coûte 1 à 2 µs, soit 9 à 18 ms par publieur des ordres 3 à 5. Le
levier annonçait tous les 1024 plateaux. Aucune décision n'en dépend : le balayage suit l'état publié, et `finish` clôt
tout.

## Règle écrite dans le plan avant la session

Exactitude : matrice TSan u21 conforme, bancs conformes, vidages des empreintes. Statistique : médiane des 6 processus
à froid, K5 CPU feuilles 16 (`278523`) et K5 GPU feuilles 24 (`344059:400`), trois trames, soit 6 rapports new/base.
Gardé si la moyenne géométrique des 6 rapports du temps de calcul du publieur de l'ordre 5
(`pipeline.orders[4].publish_cpu_ms`) est ≤ 0,90, et celle de `forest_ms` ≤ 1,00. Juge : `claudeann1/judge.py`.

## Exactitude

Matrice `gcc_tsan` 806/806 (`matrix_summary.json`). Les trois bancs sont `conforme`, avec les vidages des empreintes à
K5 et à K10.

## Verdict : règle non atteinte, retiré

| Mode | Trame | Publieur 5 base → new (ms) | Rapport | `forest_ms` base → new | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 97,6 → 96,3 | 0,987 | 118,7 → 113,2 | 0,954 |
| CPU, 16 | ng01 | 72,6 → 80,3 | 1,105 | 88,3 → 97,0 | 1,098 |
| CPU, 16 | ng02 | 100,4 → 91,6 | 0,913 | 118,6 → 115,5 | 0,973 |
| GPU 400 ‰, 24 | ng00 | 105,5 → 94,5 | 0,896 | 122,9 → 115,1 | 0,936 |
| GPU 400 ‰, 24 | ng01 | 72,7 → 71,2 | 0,979 | 91,1 → 86,5 | 0,949 |
| GPU 400 ‰, 24 | ng02 | 102,5 → 95,6 | 0,932 | 127,1 → 118,9 | 0,935 |

Moyennes géométriques : publieur 5 **0,966** (seuil 0,90, non atteint) ; `forest_ms` 0,973.

## Lecture (descriptive)

- Une prise isolée (ng00, voie GPU) montre une forte baisse : publieurs 3, 4 et 5 de 93,6 / 102,6 / 113,9 ms à
  60,6 / 76,0 / 83,1 ms. Les médianes de six processus varient pourtant de 0,896 à 1,105. Le temps de calcul d'un
  publieur est très bruité d'un processus à l'autre, et l'effet des réveils, s'il existe, est petit devant ce bruit.
- À chaud, `forest_ms` ne bouge pas de façon lisible (voie CPU 105,5 → 104,0, 84,9 → 81,9, 115,8 → 116,5 ms). À K10,
  il passe de 1326 / 990 / 1097 à 1312 / 956 / 1083 ms (descriptif).
- Prochaine étape pour les publieurs : mesurer d'abord où va leur temps sur G4 (compteurs par phase de la boucle de
  publication), avant de toucher à un levier.

## Pièces

`claudeann1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `gpu_ab_report_ab_k5_16_cpu.json`,
`gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`, `matrix_summary.json`, `matrix_stdout.txt`. Aucune
donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
