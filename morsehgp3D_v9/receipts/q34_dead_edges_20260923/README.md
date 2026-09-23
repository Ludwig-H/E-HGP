# Reçu : arêtes q3/q4 sans sortie et certificat de voie morte

23 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP non utilisé.** Hôte local partagé à huit
cœurs, charge variable (les chronos sont indicatifs ; les comptes et les
condensés font foi).

## 1. Où va le temps q3/q4 (`harness/`)

Instrumentation d'une copie du générateur au commit `e54f727c`
(`instrumentation_*.patch`, horloge `rdtsc` par arête) lancée par
`q34_harness.cpp` (générateur q3/q4 seul, options de la chaîne, W8) sur
08/000000 sans sol à 1 mm, 39 885 sites. Classe d'une arête : ses voies après
le filtre de témoins de paire, puis « vivante » si elle émet au moins une
boule. Cycles (K5, `s00_k5_w8_base_instrumented.out`) :

| poste | arêtes | cycles (G) | part |
| --- | ---: | ---: | ---: |
| filtre de témoins de paire | 23 686 751 paires | 184,3 | 12 % |
| q3 seule, sans sortie | 165 929 | 139,4 | 9 % |
| q4 seule, sans sortie | 326 970 | 305,2 | 19 % |
| q3+q4, sans sortie | 1 238 649 | 942,8 | 60 % |
| arêtes qui émettent | 312 064 | 47,5 | 3 % |

Les arêtes sans sortie coûtent 91 % du temps (atlas 602 G, voie q3 284 G
sur les arêtes mixtes) et portent des covers de 1 453 sites contre 43 pour
les arêtes vivantes. La racine d'atlas n'a presque jamais de site
uniformément intérieur (1,62 M racines sur 1,77 M à zéro) et garde en
moyenne 1 551 sites actifs.

Prototype du certificat (`prototype_*`, balayage linéaire des formes du
cover, avant la version à frontière héritée) : q3/q4 **592 → 205 CPU·s** à
K5 (mur W8 86,9 → 28,4 s) et **1 600 → 557 CPU·s** à K10 (346,6 → 80,8 s),
flux émis identique (condensés du harnais `f31e41f3e76e03a9` K5 et
`c1437caaf8278f22` K10, avec et sans certificat). Le certificat lui-même
coûte alors 150 G (K5) et 472 G (K10) cycles : prochain poste.

## 2. Sonde de chaîne (`local_probe/`)

`run.sh` : `mhgp9_tower_probe` du worktree de développement (certificat à
balayage linéaire, WIP non commité), trois trames, K5 puis K10, W8, tour
statique par défaut. Les six condensés de tour sont **identiques** aux
sessions G4 R1 et R2 :

| trame | K5 | K10 |
| --- | --- | --- |
| 000000 | `67450c64611075b1` | `ac108f7f71096c3f` |
| 000100 | `dbf799c8ed83f53f` | `9ddbf7430c9086cc` |
| 000200 | `8240af3d4dce3d45` | `ba973af0c8da95bd` |

Voies prouvées mortes (q3 / q4) à K5 sur 000000 : 1 014 964 / 1 295 712,
contre 701 678 / 576 456 voies ouvertes. `frontier_s01_k*.json` : version à
frontière héritée (celle du commit), 000100 sans tour : mêmes preuves et
mêmes boules, tests du certificat 9,94 G → 6,75 G à K5 et 30,1 G → 17,7 G à
K10.

Ce reçu ne mesure pas G4 et n'est pas une ablation appariée sur un hôte
calme : la session G4 suivante porte les paires avec et sans certificat.
