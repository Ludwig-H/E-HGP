# Reçu : session G4 R14 (tour allégée, noyau S3 à 16 warps, paires S2/S3)

23 septembre 2026, 22 h 12 – 22 h 17 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T15:12:31.072-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-23T15:17:34.709-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition (pilote 580.173.02),
  nvcc 12.9.41.

**Paquet** : construit depuis `b68b6761`, protocole au commit. Sonde v19,
plan R14 à 18 cas ; snapshot `c2ba9fab…`, manifeste `d6482c80…`,
contrôleur `eac0ad61…`, worker `7ab07529…`, binaire `9d2eed6a…`.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17 (phases observées sur l'appareil). Aucun cas non
apparié.

## Ce que la session mesure

Par rapport à R13 (`46c50432`), trois changements :
- **Tour allégée** : rangs de plateau exacts, lots singletons sans
  allocation inerte, brouillon plat.
- **Noyau des certificats S3** : 128 registres, 16 warps par SM.
- **Sonde v19** : temps d'appareil ventilés entre noyau et transferts.

Les deux préflights GPU sont jugés arête par arête contre la référence CPU
et identiques à leur jumeau moteur : condensé `73490cf8`, et 3 404 mises en
attente à l'ardoise de 64 sites.

## Objet

- 18 cas `complete_relative`, Euler « holds ».
- Les six condensés épinglés par C (tour et catalogue) sont reproduits.
- Les 12 comparaisons entre cas sont égales : condensés, ordres et travail
  des certificats.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | S2 GPU seul | **S2 + S3 GPU** | R13 (S2 + S3) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,378 | | **1,563** | 1,738 |
| 000000 | 5 | 3,055 | 2,197 / 2,209 | **1,976 / 1,979** | 2,202 |
| 000200 | 5 | 3,315 | | **2,053** | 2,347 |
| 000100 | 10 | 7,165 | | **5,488** | 5,914 |
| 000000 | 10 | 9,747 | 7,789 / 7,825 | **6,995 / 7,040** | 7,825 |
| 000200 | 10 | 9,877 | | **7,051** | 7,967 |

## Phases (ms), 08/000000, S2 + S3 GPU

| K | q2 | front | filtre (noyau) | certificats (noyau / transferts) | survivants | recensement | tour |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 104 | 98 | 88 (63) | 144 (114 / 2) | 733 | 101 | 587 |
| 10 | | 160 | 145 (101) | 335 (293 / 3) | 2 888 | | 2 514 |

Tour à K5, par phase : validation 92, statique 250, lots 89, populations 24,
images 90, banque 11, encodage 31.

## Lecture

- **Attribution de S3** : paires répétées et entrelacées, dans la même
  session. S3 retire 0,22 s à K5 (2,20–2,21 → 1,98 s) et 0,79 s à K10
  (7,79–7,83 → 7,00–7,04 s). Les répétitions varient de moins de 1 % à K5.
- **Noyau S3** : 114 ms à K5 et 293 ms à K10 ; les transferts coûtent 2 à
  3 ms. L'appel sur l'appareil passe de 189 à 116 ms à K5, et de 486 à
  295 ms à K10 (R13 → R14, entre sessions). C'est l'effet attendu des
  compteurs par arête et des 16 warps par SM.
- **Tour** : 0,745 → 0,587 s à 000000/K5 (−21 %) et 3,04 → 2,51 s à K10
  (−17 %), entre sessions, à objet égal (épingles). La phase A de l'ordre
  K5 n'est plus le chemin critique : la fenêtre statique + lots vaut
  339 ms, contre 464 en R13.
- **Meilleures chaînes** : K5 1,56 / 1,98 / 2,05 s, K10 5,49 / 7,00 /
  7,05 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,98 s) :
  - survivants 0,73 s (atlas, q3, q4 et covers reconstruits) ;
  - tour 0,59 s ;
  - certificats 0,14 s ;
  - q2, recensement, front et filtre, environ 0,39 s.

  S4 (voies q3/q4 sur l'appareil) vise le premier poste.

## Contenu

- `vm/` : sortie du worker (inventaire, versions, configuration CUDA, build,
  trois préflights, 18 cas, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SUMMARY.json` (schéma `mhgp9_g4_tower_summary_v5`),
  `SHA256SUMS`.
