# Reçu : session G4 R19 (q2 pendant l'appareil, certificats par blocs, sonde v24)

24 septembre 2026, 17 h 42 – 17 h 47 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-24T10:42:03.968-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-24T10:47:14.007-07:00`).

**Paquet** : construit depuis `e9f0a104`, protocole au commit. Sonde v24.
Plan R19 à 18 cas : pour chaque trame et chaque K, le bras GPU (tous les
leviers) et son jumeau moteur ; à 08/000000, des paires répétées et
entrelacées q2 séquentiel / q2 recouvert. Snapshot `91df54b2…`, manifeste
`1215a26d…`, worker `a90f8e16…`.

**Contenu mesuré** :
- **q2 pendant les appels de l'appareil** (levier `q2_during_device`) ;
- **certificats S3 par blocs** : parcours d'arbre par blocs de 32 nœuds,
  chargement des formes par fenêtres, norme du disque partagée.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17. Aucun cas non apparié.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne.
- 18 cas `complete_relative`, Euler « holds », les six épingles de C
  reproduites.
- Les 12 comparaisons entre cas sont égales, condensé des présentations
  compris : q2 recouvert, q2 séquentiel et moteur donnent le même objet.
  C'est aussi la première exécution des certificats par blocs sur
  l'appareil.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | q2 séquentiel | **q2 recouvert** | R18 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,263 | | **1,026** | 1,046 |
| 000000 | 5 | 2,858 | 1,272 / 1,273 | **1,175 / 1,214** | 1,251 / 1,287 |
| 000200 | 5 | 3,162 | | **1,311** | 1,355 |
| 000100 | 10 | 6,705 | | **3,214** | 3,258 |
| 000000 | 10 | 9,111 | 4,191 / 4,246 | **4,060 / 4,012** | 4,268 / 4,301 |
| 000200 | 10 | 9,385 | | **4,053** | 4,181 |

## Phases (ms)

| cas | q2 (recouvert) | attente de q2 | front | filtre | préparation / attente | certificats (noyau) | voies (noyau) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 K5, q2 recouvert | 104 | 0 | 110 / 101 | 118 / 130 | 136 / 26 et 137 / 36 | 118 (90) | 145 (91) |
| 000000 K5, q2 séquentiel | 105 | — | 102 / 100 | 91 / 89 | 200 / 0 et 167 / 0 | 119 (90) | 151 (91) |
| 000100 K5, q2 recouvert | 75 | 0 | 75 | 166 | 177 / 102 | 98 (71) | 100 (47) |
| 000200 K5, q2 recouvert | 114 | 0 | 104 | 183 | 188 / 86 | 131 (102) | 154 (91) |
| 000000 K10, q2 recouvert | 205 / 204 | 0 | 170 / 166 | 210 / 160 | 201 / 32 et 138 / 0 | 269 (229) | 580 (368) |

## Lecture

- **Attribution du recouvrement de q2** (paires entrelacées à 000000) :
  −60 à −97 ms à K5, −130 à −234 ms à K10. q2 est entièrement caché
  (attente nulle).
- **Contrepartie mesurée.** La préparation de l'appareil (contexte, index
  plat, ardoises résidentes : 136 à 224 ms) n'est plus cachée que par le
  front. Le premier appel (le filtre) l'attend 26 à 36 ms à 000000, 86 ms à
  000200 et 102 ms à 000100, dont le front est plus court. C'est le
  prochain poste : lancer la préparation plus tôt, ou la réduire.
- **Certificats par blocs** : noyau 117 → 90 ms à K5 et 300 → 229 ms à
  K10 (000000, par rapport à R18).
- **Meilleures chaînes** : K5 1,03 / 1,18 / 1,31 s, K10 3,21 / 4,01 /
  4,05 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,175 s) :
  - tour 0,44 s ;
  - appel des voies 0,15 s, certificats 0,12 s, filtre 0,12 s (dont
    l'attente de la préparation), front 0,11 s ;
  - recensement 0,10 s, fusion 0,03 s.
