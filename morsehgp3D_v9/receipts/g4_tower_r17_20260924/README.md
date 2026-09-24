# Reçu : session G4 R17 (étape 1 tour + voies, sonde v22)

24 septembre 2026, 12 h 25 – 12 h 30 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-24T05:25:34.499-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-24T05:30:23.863-07:00`).

**Paquet** : construit depuis `b557fb75`, protocole au commit. Sonde v22,
même plan à 18 cas que R16 ; snapshot `31e653ef…`, manifeste `d8cb5b2a…`,
worker `f1ddc7c0…`.

**Contenu mesuré** (étape 1 des plans de
[conception](../../docs/tour_voies_conception_20260924/README.md)) :
- tour : arène de la phase 0 sans mise à zéro, images par rangs de plateau,
  sous-chronos E0 ;
- voies : buffers résidents réservés pendant q2, T1 (candidats
  indépendants à sortie anticipée), sous-chronos.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17. Aucun cas non apparié.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne.
- 18 cas `complete_relative`, Euler « holds », les six épingles de C
  reproduites.
- Les 12 comparaisons entre cas sont égales, condensé des présentations
  compris : `a2aa4b20ca392dfe` à K5 et `43ff64fb1c3846d9` à K10 sur
  08/000000, comme R16 et le reçu local.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | S4a GPU | **S4a + S4b GPU** | R16 (S4a + S4b) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,333 | | **1,138** | 1,214 |
| 000000 | 5 | 2,991 | 1,758 / 1,745 | **1,395 / 1,420** | 1,532 / 1,546 |
| 000200 | 5 | 3,267 | | **1,516** | 1,651 |
| 000100 | 10 | 6,892 | | **3,526** | 3,919 |
| 000000 | 10 | 9,393 | 6,342 / 6,351 | **4,624 / 4,671** | 5,159 / 5,174 |
| 000200 | 10 | 9,619 | | **4,569** | 5,007 |

## Phases à 08/000000 (ms)

| | K5 R16 | **K5 R17** | K10 R16 | **K10 R17** |
| --- | ---: | ---: | ---: | ---: |
| tour | 566 | **501 / 504** | 2 537 | **2 221 / 2 227** |
| validation | 86 | 91 / 94 | 348 | 339 / 350 |
| phase 0 (somme des K) | 253 | 204 / 210 | 1 481 | 1 300 / 1 321 |
| images | 80 | 31 / 28 | 300 | 115 / 126 |
| appel des voies | 275 / 278 | **199 / 202** | 884 / 896 | **645 / 652** |
| noyau des voies | 190 | 147 | 586 | 433 / 438 |
| installation / fin / conversion | — | 3,3 / 0,2 / 9 | — | 4,5 / 0,4 / 26 |

Sous-chronos E0 à K5 (cas 0) :
- **validation** 91 ms, dont :
  - tri des niveaux 31 ms et passe 1 22 ms ;
  - programmes 11,5 ms, tri des clés 8,8 ms, index 7,4 ms.
- **phase 0 de l'ordre 5** 81 ms : collecte 14, tris 29, groupes 3,
  résolution 35.
- **phase A** par ordre : 43, 67, 112, 157 et **227 ms** (ordre 5).

## Lecture

- **Gain de l'étape 1** (sessions distinctes, même plan) :
  - à 000000, −0,13 s à K5 et −0,50 à −0,54 s à K10 ;
  - sur les autres trames, −0,08 et −0,14 s à K5, −0,39 et −0,44 s à K10.
- **Tour.** Les images passent de 80 à 30 ms et la phase 0 perd 45 ms à K5.
  Le chemin critique à K5 est maintenant la **phase A de l'ordre 5**
  (227 ms, un seul fil), ouverte après sa phase 0 (81 ms). La queue
  (populations, images, banque, encodage) vaut environ 98 ms. À K10, la
  phase 0 séquentielle domine : 1,30 s, dont 224 ms de résolution et 65 ms
  de tris à l'ordre 10.
- **Voies.** L'hôte ne coûte plus que 13 ms de l'appel à K5 (60 ms avant) ;
  le noyau perd 43 ms à K5 et 150 ms à K10 avec T1. Il reste 147 ms de
  noyau, que les ouvriers attendent.
- **Meilleures chaînes** : K5 1,14 / 1,40 / 1,52 s, K10 3,53 / 4,62 /
  4,57 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,395 s) :
  - tour 0,50 s ;
  - appel des voies 0,20 s, certificats 0,14 s ;
  - front et filtre 0,19 s ;
  - q2 0,10 s, recensement 0,10 s ;
  - fusion et index 0,05 s.
