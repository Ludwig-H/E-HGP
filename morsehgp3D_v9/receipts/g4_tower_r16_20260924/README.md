# Reçu : session G4 R16 (voies q3 et q4 sur l'appareil, S4b)

24 septembre 2026, 08 h 37 – 08 h 43 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-24T01:37:56.607-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-24T01:43:02.881-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition.

**Paquet** : construit depuis `931a0862`, protocole au commit. Sonde v21,
plan R16 à 18 cas ; manifeste `c625ab3c…`, worker `bcff0254…`.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17. Aucun cas non apparié.

## Préflights

Sur le nuage de préflight (1 500 sites, K5, tous leviers) :
- **préflight jugé** : chaque certificat et chaque voie q3 ou q4 décidés
  sur l'appareil sont recalculés sur le CPU (voies q3 et q4 du moteur) ;
- **jumeau moteur** : condensé `73490cf8`, égal ;
- **ardoises réduites** : certificats à 64 sites, voies à 24 sites ;
  3 404 arêtes rendues au chemin moteur et 11 993 voies rendues à la traîne
  du CPU, pour le même objet.

## Objet

- 18 cas `complete_relative`, Euler « holds ».
- Les six condensés épinglés par C (tour et catalogue) sont reproduits par
  les bras S4a, S4b et moteur.
- Les 12 comparaisons entre cas sont égales. Elles portent sur les
  condensés, les ordres et le travail des certificats, et, nouveauté de la
  v21, sur le **condensé des présentations** et les comptes q2, q3 et q4
  émis et présentés. Chaque trame et chaque K ont donc le même multiensemble
  (clé, arité, support) sur l'appareil et dans le moteur. À 08/000000 :
  `a2aa4b20ca392dfe` (K5) et `43ff64fb1c3846d9` (K10), égaux aux bras
  hôte du reçu local S4b.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | S2 + S3 + S4a GPU | **S2 + S3 + S4a + S4b GPU** | R15 (S4a) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,395 | | **1,214** | 1,446 |
| 000000 | 5 | 3,037 | 1,811 / 1,807 | **1,532 / 1,546** | 1,811 |
| 000200 | 5 | 3,309 | | **1,651** | 1,858 |
| 000100 | 10 | 7,142 | | **3,919** | 5,242 |
| 000000 | 10 | 9,665 | 6,752 / 6,744 | **5,159 / 5,174** | 6,721 |
| 000200 | 10 | 9,933 | | **5,007** | 6,693 |

## Phases (ms), 08/000000, paires répétées et entrelacées

| K | bras | arêtes (CPU) | appel des voies | appareil (noyau / transferts) | attente | traîne | certificats (noyau) | tour |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | S4a | 541 / 537 | 137 / 144 | 57 (28 / 28) | 0 | 4–5 | 141 / 140 (112) | 564 / 571 |
| 5 | S4b | 2 / 2 | 275 / 278 | 215 / 216 (190 / 25–26) | 274 / 277 | 5 | 140 / 141 (112–114) | 566 / 568 |
| 10 | S4a | 2 418 / 2 422 | 380 / 372 | 195 (95–96 / 99) | 0 | 18–19 | 324 / 325 (285–288) | 2 639 / 2 618 |
| 10 | S4b | 2 / 2 | 884 / 896 | 711 / 712 (586–587 / 125) | 882 / 894 | 26–28 | 326 (285–291) | 2 537 / 2 530 |

Voies demandées et décidées : 708 686 à K5 et 1 463 362 à K10 (q3 ou q4), sans
mise en attente ; 3 008 warps. Enregistrements : 849 780 à K5 et 4 630 767 à
K10, dont 158 496 et 1 732 548 tétraèdres.

## Lecture

- **Attribution de S4b** : paires répétées et entrelacées dans la même
  session. S4b retire 0,26–0,28 s à K5 (1,81 → 1,53–1,55 s) et
  1,57–1,59 s à K10 (6,74–6,75 → 5,16–5,17 s).
- **CPU de la chaîne** : 17,9 s contre 38,2 s à K5 (−53 %), 79,3–79,8 s
  contre 182 s à K10 (−56 %). Il ne reste plus d'arête à traiter sur le CPU
  pendant l'appel.
- **Chemin critique** : les ouvriers n'ont plus rien à faire pendant l'appel
  des voies, et l'attente égale l'appel. Le noyau (190 ms à K5, 586 ms à
  K10, une arête par warp, sans tâches ni raffinement) entre dans le
  chemin critique. Les transferts des enregistrements restent de 25 et
  125 ms.
- **Meilleures chaînes** : K5 1,21 / 1,53 / 1,65 s, K10 3,92 / 5,16 /
  5,01 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,53 s) :
  - tour 0,57 s ;
  - appel des voies 0,28 s (noyau 0,19 s) ;
  - certificats 0,14 s ;
  - q2 0,10 s et recensement 0,10 s ;
  - front, filtre, fusion et index environ 0,34 s.

  Le contrat de 1 s à K5 exige maintenant la tour et le noyau des voies
  ensemble.
