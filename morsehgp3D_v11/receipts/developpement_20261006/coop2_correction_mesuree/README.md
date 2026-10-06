# Coop2 : la correction des tables par valeur ne suffit pas ; cause trouvée dans la reconvergence des warps

6 octobre 2026. Session gardée `v11.20261006.claudecoop2`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `ee3eabe5e`. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Bancs `conforme` à K5/16 et K10/24 : dumps et registres identiques en CPU,
GPU un fil et GPU coopératif.

## Critère écrit d'avance : non atteint

Le critère demandait un exécuteur GPU un fil à moins de 10 % de la session L4.

| | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K5, L4 : exécuteur (ms) | 59 | 54 | 52 |
| K5, coop2 : exécuteur (ms) | 76 | 75 | 56 |
| K10, L4 : exécuteur (ms) | 340 | 301 | 318 |
| K10, coop2 : exécuteur (ms) | 458 | 421 | 410 |

Le comptage retrouve ses valeurs L4 (35, 29, 33 ms à K5 ; 214, 172, 196 ms à K10). La seconde passe d'écriture reste
doublée : 35, 41, 18 ms à K5 ; 232, 239, 201 ms à K10.

## Diagnostic

**Nsight Compute, noyau d'écriture, K10/24, ng00**

| | claudegpu6 (4 octobre) | coop2 |
| --- | ---: | ---: |
| Durée (ms) | 136 | 291 |
| Instructions exécutées (cycles × IPC) | ≈ 45 M | ≈ 124 M |
| Fils actifs par warp | 3,47 | 1,39 |

La grille est identique : 132 blocs, 4 196 feuilles.

**Sur l'hôte**, à un fil, à K10/24 sur ng00, l'ancien (830473218) et le nouveau code font le même travail : comptage de
7,1 s contre 7,1 s, écriture de 0,34 s contre 0,33 s.

**Le surcoût vient donc du code généré par nvcc.** La boucle `extend` appelait `extend_one`. La forme reste équivalente,
mais elle multiplie les barrières de reconvergence : 107 `BSSY` dans le noyau d'écriture, contre 85 en 830473218. Les
warps de feuilles lourdes reconvergent alors moins, et la passe, limitée par la latence, double.

## Correction (commit suivant)

La voie un fil reprend mot pour mot la boucle `extend` de 830473218, et `extend_one` ne sert plus qu'à la voie
coopérative. Le SASS local revient à 84 `BSSY`, 0 `CALL` et 9 504 instructions dans le noyau d'écriture (830473218 :
85 `BSSY` et 9 536 instructions). La mesure sur G4 reste à faire.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs |
| `gpu_profile_k10.json` | Nsight Compute du noyau un fil, K10/24, ng00 |

`SHA256SUMS` couvre les autres fichiers.
