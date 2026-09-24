# Reçu : session G4 R18 (étape 2 : tour E3 et E2, voies en tâches, sonde v23)

24 septembre 2026, 15 h 07 – 15 h 12 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-24T08:07:35.630-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-24T08:12:26.063-07:00`).
- **Tentative précédente** (15 h 06 UTC) : l'authentification GCP était
  indisponible pendant le contrôle des préconditions, et le démarrage gardé
  a refusé de lancer la VM (`start not certified`). Aucune VM n'a démarré :
  la date du dernier démarrage de la cible est restée celle de R17. La
  session a été relancée une minute plus tard. Les traces expurgées sont
  dans `tentative_150601/`.

**Paquet** : construit depuis `446b45f7`, protocole au commit. Sonde v23,
même plan à 18 cas que R16 et R17 ; snapshot `20edc24e…`, manifeste
`1de7687c…`, worker `89bc20fe…`.

**Contenu mesuré** : étape 2 des plans de
[conception](../../docs/tour_voies_conception_20260924/README.md).
- **Tour E3**, phase A maigre ; **tour E2**, sections série de la
  validation.
- **Voies en tâches** (arête, plage de graines), en trois étapes P, T et C
  sur l'appareil.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17. Aucun cas non apparié.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne.
- 18 cas `complete_relative`, Euler « holds », les six épingles de C
  reproduites.
- Les 12 comparaisons entre cas sont égales, condensé des présentations
  compris. C'est la **première exécution sur l'appareil des voies en
  tâches** : le même objet que le moteur sur les trois trames, à K5 et K10.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | S4a GPU | **S4a + S4b GPU** | R17 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,261 | | **1,046** | 1,138 |
| 000000 | 5 | 2,872 | 1,676 / 1,670 | **1,251 / 1,287** | 1,395 / 1,420 |
| 000200 | 5 | 3,196 | | **1,355** | 1,516 |
| 000100 | 10 | 6,811 | | **3,258** | 3,526 |
| 000000 | 10 | 9,186 | 6,105 / 6,104 | **4,268 / 4,301** | 4,624 / 4,671 |
| 000200 | 10 | 9,377 | | **4,181** | 4,569 |

## Phases à 08/000000 (ms)

| | K5 R17 | **K5 R18** | K10 R17 | **K10 R18** |
| --- | ---: | ---: | ---: | ---: |
| tour | 501 / 504 | **417 / 433** | 2 221 / 2 227 | **1 971 / 1 978** |
| validation | 91 / 94 | 71 / 72 | 339 / 350 | 245 / 240 |
| phase A de l'ordre le plus haut | 227 | 157 | 801 | 424 |
| appel des voies | 199 / 202 | **147 / 151** | 645 / 652 | **590 / 601** |
| noyau (P / T / C) | 147 | 93 (12 / 49 / 32) | 433 | 377 (30 / 266 / 81) |
| tâches ; travail maximal par tâche | — | 2 500 659 ; 7 114 | — | 9 581 649 ; 27 057 |

## Lecture

- **Gain de l'étape 2** (sessions distinctes, même plan) :
  - à K5 : −0,14 s à 000000, −0,09 s à 000100, −0,16 s à 000200 ;
  - à K10 : −0,36 à −0,37 s à 000000.
- **Tour.** La phase A de l'ordre le plus haut perd 31 % à K5 et 47 % à
  K10, et la validation perd 20 ms à K5. À K5, la fenêtre de la tour est
  maintenant bornée par la phase 0 **séquentielle** des ordres (207 ms),
  suivie de la phase A des ordres inférieurs. La queue (populations,
  images, banque, encodage) vaut environ 90 ms.
- **Voies.** Le noyau passe de 147 à 93 ms à K5 et de 433 à 377 ms à K10.
  L'étape T fait 49 ms à K5, pour 67–78 ms projetés sur le noyau équilibré
  entier. Le compactage C coûte 32 ms à 000000, contre 3 ms à 000100 :
  c'est le prochain poste de l'appel.
- **Meilleures chaînes** : K5 1,05 / 1,25 / 1,36 s, K10 3,26 / 4,27 /
  4,18 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,251 s) :
  - tour 0,42 s ;
  - appel des voies 0,15 s, certificats 0,15 s ;
  - front et filtre environ 0,18 s ;
  - q2 0,10 s, recensement 0,10 s.
