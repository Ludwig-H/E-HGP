# Reçu : session G4 R15 (voie q3 sur l'appareil, S4a)

23 septembre 2026, 23 h 38 – 23 h 43 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T16:36:46.828-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-23T16:42:53.951-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition.

**Paquet** : construit depuis `8b47a75a`, protocole au commit. Sonde v20,
plan R15 à 18 cas ; snapshot `0324fc54…`, worker `89983e24…`.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17 (phases observées sur l'appareil). Aucun cas non
apparié.

## Préflights

Sur le nuage de préflight (1 500 sites, K5, tous leviers) :
- **préflight jugé** : chaque certificat et chaque voie q3 décidés sur
  l'appareil sont recalculés sur le CPU (voie q3 du moteur pour S4a) ;
- **jumeau moteur** : condensé `73490cf8`, égal ;
- **ardoises réduites** : certificats à 64 sites, voies q3 à 24 sites ;
  3 404 arêtes rendues au chemin moteur, 11 976 voies q3 rendues à la
  traîne du CPU, même objet.

## Objet

- 18 cas `complete_relative`, Euler « holds ».
- Les six condensés épinglés par C (tour et catalogue) sont reproduits, avec
  et sans S4a.
- Les 12 comparaisons entre cas sont égales : condensés, ordres, travail
  des certificats.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | S2 + S3 GPU | **S2 + S3 + S4a GPU** | R14 (S2 + S3) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,409 | | **1,446** | 1,563 |
| 000000 | 5 | 3,062 | 1,988 / 1,970 | **1,811 / 1,807** | 1,976 |
| 000200 | 5 | 3,333 | | **1,858** | 2,053 |
| 000100 | 10 | 7,211 | | **5,242** | 5,488 |
| 000000 | 10 | 9,708 | 7,095 / 7,120 | **6,721 / 6,711** | 6,995 |
| 000200 | 10 | 9,914 | | **6,693** | 7,051 |

## Phases (ms), 08/000000, paires répétées et entrelacées

| K | bras | arêtes (CPU) | appel q3 | appareil q3 (noyau / transferts) | attente | traîne | tour |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | S3 | 734 / 736 | | | | | 588 / 568 |
| 5 | S4a | 542 / 538 | 132 / 152 | 54 / 56 (28 / 26–27) | 0 | 4–5 | 564 / 561 |
| 10 | S3 | 2 913 / 2 914 | | | | | 2 575 / 2 606 |
| 10 | S4a | 2 440 / 2 421 | 381 / 399 | 202 / 204 (92–98 / 105–112) | 0 | 21–23 | 2 574 / 2 575 |

Voies q3 demandées et décidées : 701 678 à K5, 1 437 421 à K10 ; aucune mise
en attente ; 3 008 warps ; 691 284 et 2 898 219 boules.

## Lecture

- **Attribution de S4a** : paires répétées et entrelacées dans la même
  session. S4a retire 0,16–0,18 s à K5 (1,97–1,99 → 1,81 s) et 0,37–0,41 s
  à K10 (7,10–7,12 → 6,71–6,72 s).
- **Recouvrement** : l'appel q3 (noyau 28 ms à K5, 92–98 ms à K10) finit
  avant les voies q4 du CPU ; l'attente est nulle. La phase des arêtes ne
  garde que l'atlas et q4 (540 ms à K5, 2,43 s à K10), qui deviennent la
  cible de S4b.
- **Transferts** : le téléchargement des enregistrements (128 octets par
  boule) coûte autant que le noyau (26 ms à K5, environ 110 ms à K10). Il
  est caché ici, mais pas une fois q4 porté.
- **CPU de la chaîne** : 38–39 s contre 48 s à K5 (−19 %), 183 s contre
  207 s à K10.
- **Meilleures chaînes** : K5 1,45 / 1,81 / 1,86 s, K10 5,24 / 6,72 /
  6,69 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,81 s) : atlas et q4 CPU 0,54 s, tour 0,56 s,
  certificats 0,14 s, q2, front, filtre et recensement environ 0,40 s.

## Contenu

- `vm/` : sortie du worker (inventaire, versions, build, trois préflights,
  18 cas, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SUMMARY.json`, `SHA256SUMS`.
