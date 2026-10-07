# Diagnostic de l'étage domain : frontière et passe unique sur G4 (session claudediag1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudediag1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudediag1/receipt.json`). Source : `4f0c6cb9c`. Trames LiDAR réelles ng00, ng01 et ng02, W48, modes de
référence (avec cache de blocs). **Session de diagnostic, sans règle d'adoption** : elle relève les sous-chronos
ajoutés par `c80c12012` (frontière adaptative : racine, rondes, sélections, publications ; somme et maximum des tâches
de la passe unique). Les deux bancs sont `conforme`, avec les vidages des empreintes.

## Médianes à froid (ms, 3 processus)

| Mode | Trame | `domain` | Frontière | dont racine / rondes / sélection / publication | Passe unique | Somme des tâches | Tâche max | Tri |
| --- | --- | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| CPU, 16 | ng00 | 203,0 | 20,6 | 0,9 / 14,4 / 0,7 / 4,5 | 153,3 | 7073 | 26,1 | 10,6 |
| CPU, 16 | ng01 | 171,0 | 17,8 | 0,8 / 12,2 / 0,8 / 3,8 | 127,4 | 5853 | 30,7 | 9,0 |
| CPU, 16 | ng02 | 203,4 | 21,0 | 1,1 / 14,7 / 0,7 / 4,4 | 149,8 | 6917 | 47,4 | 12,6 |
| GPU 400 ‰, 24 | ng00 | 221,5 | 21,4 | 0,9 / 15,3 / 0,7 / 4,3 | 37,2 | 1637 | 20,1 | 10,0 |
| GPU 400 ‰, 24 | ng01 | 202,9 | 18,7 | 0,9 / 13,2 / 0,8 / 3,7 | 30,5 | 1431 | 17,7 | 8,5 |
| GPU 400 ‰, 24 | ng02 | 220,1 | 21,1 | 1,1 / 15,0 / 0,7 / 4,3 | 33,7 | 1545 | 24,5 | 12,2 |

1023 tâches de frontière (1022 sur ng02) dans tous les cas.

## Lecture

- La frontière se passe surtout en rondes : 12 à 15 ms, soit environ 0,5 ms pour chacune des 27 à 28 rondes, même à
  48 ouvriers. La racine, la sélection et la publication font ensemble 5 à 6 ms.
- La passe unique de la voie GPU, dont les feuilles sont en lot, consomme encore 1,4 à 1,6 s de CPU, réparties à 92 %
  sur les 48 ouvriers. Ce temps va surtout aux filtres des nœuds internes (réservoir de témoins et tests G1). Accélérer
  ce filtre réduirait directement l'étage.
- En voie CPU, la passe unique (feuilles comprises) consomme 5,9 à 7,1 s de CPU, réparties à 96 %.

## Pièces

`claudediag1/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_diag_k5_16_cpu.json`,
`gpu_ab_report_diag_k5_24_gpu.json`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf
lui-même.
