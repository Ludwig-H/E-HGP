# Reçu : session G4 R13, certificats de voie morte sur GPU (S3)

23 septembre 2026, 20 h 09 – 20 h 15 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=cuda_g4`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T13:08:51.251-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu
  en lecture seule (dernier arrêt `2026-09-23T13:15:12.865-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition (pilote 580.173.02,
  97 887 Mio, sm 12.0), nvcc 12.9.41.

**Paquet** : construit depuis `46c50432`, protocole au commit. Sonde v18,
plan par défaut R13 à 18 cas ; snapshot `6a9ac67b…`, manifeste
`227b3018…`, contrôleur `eac0ad61…`, worker `a47d7211…`, binaire
`deac2f8b…`.

**Reçus** : hôte et worker **`completed`**, backend `cuda_g4`,
`GPU_executed` vrai. `GPU_completed_cases` vaut 0, 2, 4, 6, 8, 10, 12, 14
et 16 (phases observées sur l'appareil). Aucun cas non apparié.

## Préflights (1 500 sites, K5, avant tout cas LiDAR)

- **GPU complet, juge** : filtre et certificats sur l'appareil. Chaque
  survivant décidé est recalculé par la référence CPU (masque par arête,
  travail sommé) ; tous sont égaux. Condensé `73490cf8`, identique à son
  jumeau moteur, avec le même travail des certificats.
- **Mise en attente sur l'appareil, juge** : ardoise de 64 sites par warp.
  3 404 arêtes sur 55 523 ont été rendues au chemin moteur, exactement le
  compte calculé en local par le port hôte. Même tour, même catalogue et
  même travail des certificats que le préflight par défaut ; les arêtes
  décidées sur l'appareil ont toutes été jugées égales au CPU.

## Objet

- Les 18 cas sont `complete_relative`, Euler « holds ».
- Chaque cas reproduit les **six condensés épinglés par l'auditeur C** (tour
  et catalogue des trois trames à K5 et K10) ; le lecteur l'exige.
- Les 12 comparaisons entre cas (condensés de tour et de catalogue, ordres,
  travail des certificats et de la couverture) sont toutes égales.

Le condensé du catalogue est un FNV-64 de la vue canonique de C, pas une
égalité littérale.

## Chaîne de bout en bout (s)

W48, tour statique 48, s = 8.

| trame | K | moteur | lots CPU | S2 GPU seul | **S2 + S3 GPU** | R12 (S2 GPU) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,56 | | | **1,74** | 2,01 |
| 000000 | 5 | 3,27 | 3,84 | 2,34 | **2,20** | 2,47 / 2,57 |
| 000200 | 5 | 3,54 | | | **2,35** | 2,69 |
| 000100 | 10 | 7,71 | | | **5,91** | 6,63 |
| 000000 | 10 | 10,50 | 11,02 | 8,41 | **7,83** | 8,32 |
| 000200 | 10 | 10,75 | | | **7,97** | 8,70 |

À 000000/K5, W24 GPU complet : 2,55 s ; W1 moteur : 65,7 s.

## Phases q3/q4 du chemin GPU complet (ms)

| trame | K | front | filtre (appareil) | certificats (appareil) | survivants | survivants S2 seul | covers reconstruits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 75 | 81 (48) | 170 (149) | 541 | | 610 811 |
| 000000 | 5 | 98 | 91 (72) | 211 (189) | 731 | 1 082 | 708 686 |
| 000200 | 5 | 95 | 94 (73) | 239 (216) | 749 | | 749 534 |
| 000100 | 10 | 122 | 97 (71) | 390 (361) | 2 244 | | 1 258 362 |
| 000000 | 10 | 165 | 146 (113) | 519 (486) | 2 922 | 3 951 | 1 463 362 |
| 000200 | 10 | 161 | 154 (120) | 591 (557) | 3 086 | | 1 554 880 |

Le noyau des certificats tourne sur 1 504 warps (188 SM × 8). Ce nombre
vient de la requête d'occupation : 250 registres par fil. Aucun cas n'a
mis d'arête en attente (ardoise de 65 536 sites).

## Lecture

- **Préchauffage** (contexte CUDA et index plat pendant q2) : l'appel du
  filtre passe de 205–315 ms (R12) à 81–154 ms. Le filtre GPU seul, à
  000000/K5, donne ainsi 2,34 s contre 2,47–2,57 à R12.
- **S3** : à 000000/K5, les certificats coûtent 211 ms (dont 189 d'appareil)
  et retirent 351 ms à la phase des survivants. Le gain net est de 140 ms
  (2,34 → 2,20 s) ; à K10, de 580 ms (8,41 → 7,83 s).
- **Meilleures chaînes** : K5 1,74 / 2,20 / 2,35 s, K10 5,91 / 7,83 /
  7,97 s ; −11 à −13 % contre R12 à K5, −6 à −11 % à K10. Ce sont
  toujours des trames sans sol de la seule séquence 08, sans contrat.
- **Lots CPU** (bras demandé par B) : ils sont plus lents que le moteur
  (3,84 contre 3,27 s à K5) : sans cache de ligne, le filtre par lots coûte
  1,6 s sur CPU. Le gain de R12 et R13 est donc celui de l'appareil, pas du
  seul découpage par lots.
- **Coût du noyau S3** : 189 ms pour 2,04 M arêtes à K5, bien au-dessus de
  la passe de filtre (72 ms pour 23,7 M paires). Le warp par arête garde un
  parcours d'arbre et une récursion uniformes ; l'occupation est limitée à
  8 warps par SM par les registres. C'est une marge de travail, pas une
  limite de principe.
- **Reste à K5** (000000, 2,20 s) : survivants 0,73 s (atlas, q3 et q4 des
  arêtes vivantes, plus 0,7 M covers reconstruits), tour 0,75 s,
  certificats 0,21 s, q2 et recensement environ 0,2 s. Le contrat exige de
  réduire ensemble les survivants et la tour.
- **Résidu du protocole** : le lot vide sur l'appareil n'a pas été exercé
  (aucun cas sans survivant).

## Contenu

- `vm/` : sortie du worker (inventaire, versions, configuration CUDA, build,
  trois préflights, 18 cas, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SUMMARY.json` (schéma `mhgp9_g4_tower_summary_v4`),
  `SHA256SUMS`.
