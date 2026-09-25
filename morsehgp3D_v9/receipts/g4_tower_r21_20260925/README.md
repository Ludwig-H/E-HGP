# Reçu : session G4 R21 (tour intégrée, session d'appareil, trames brutes, sonde v27)

25 septembre 2026, 07 h 04 – 07 h 11 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-25T00:04:19.594-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-25T00:11:07.597-07:00`).

**Paquet** : construit depuis `d054c1c5`, protocole au commit. Sonde v27.
Snapshot `9bd2c651…`, manifeste `3cc55001…`, worker `d331dea9…`.

**Contenu mesuré** :
- **tour intégrée** : queue en pipeline E4, regroupement haché de la
  phase 0 et pool persistant E2, par options nommées ;
- **session d'appareil** ouverte par le processus avant la chaîne
  (`device_session`, sonde v26), coût publié à part, hors `chain_total` ;
- **passe fusionnée L15 désactivée** (mesurée plus lente en R20) ;
- **trames brutes avec sol**, pour la première fois sur G4.

**Plan R21 à 34 cas** :
- les trames sans sol 08/000000, 000100 et 000200 : bras GPU et jumeau
  moteur, à K5 et K10 ;
- à 00 :
  - paires répétées et entrelacées froid (`gpu_cold`, sans session) et
    chaud ;
  - bras `gpu_tower_witness`, sans les trois leviers de la tour, deux
    fois à K5 et à K10 ;
- les trames brutes b00, b01 et b02 (123 389, 124 479 et 125 526 sites,
  grille 1 mm) : bras GPU et jumeau moteur, à K5 et K10.

**Reçus** : hôte et worker **`completed`**. Les 34 cas sont
`complete_relative`, y compris les 12 cas bruts. `GPU_completed_cases` =
0, 2, 4, 6, 8, 10, 12 à 22, 24, 26, 28, 30 et 32. Aucun cas non apparié.
Mur utile du worker : 256 s.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne.
- Euler « holds » partout.
- **Les douze épingles** sont reproduites : les six sans sol de C, et les six
  brutes de C (`audits/c_raw_pins_20260924`, que le juge à supports
  indépendants de C a validées à 06 h 42).
- Les 22 comparaisons entre cas sont égales, condensé des présentations
  compris. Bras GPU, froid, témoin de la tour et moteur donnent le même
  objet.
- Sur le brut, le chemin GPU reporte des arêtes au moteur. L'objet reste
  identique :
  - certificats : 22 à b00/K5 et 57 à b00/K10 ;
  - voies : 8 à b02/K10.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | **GPU (session chaude)** | froid | témoin de la tour | R20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,125 | **0,805** | | | 1,010 |
| 000000 | 5 | 2,712 | **0,972 / 0,998** | 1,074 / 1,034 | 1,105 / 1,118 | 1,109 / 1,193 |
| 000200 | 5 | 2,996 | **1,026** | | | 1,260 |
| 000100 | 10 | 6,132 | **2,446** | | | 3,147 |
| 000000 | 10 | 8,404 | **3,165 / 3,212** | 3,201 / 3,194 | 3,810 / 3,929 | 3,873 / 3,940 |
| 000200 | 10 | 8,594 | **3,136** | | | 3,860 |
| brut 000000 | 5 | 5,629 | **2,094** | | | |
| brut 000100 | 5 | 4,766 | **1,914** | | | |
| brut 000200 | 5 | 5,375 | **2,180** | | | |
| brut 000000 | 10 | 14,954 | **6,487** | | | |
| brut 000100 | 10 | 12,438 | **5,711** | | | |
| brut 000200 | 10 | 14,325 | **6,334** | | | |

La session d'appareil elle-même (contexte, puis ardoises des voies)
prend 122 à 172 ms de contexte et environ 2 ms de réservations, hors
chaîne.

## Tour (ms), 08/000000 K5

| phase | GPU | témoin de la tour |
| --- | ---: | ---: |
| tour entière | 300 / 315 | 424 / 423 |
| validation | 50 | 73 |
| phase 0 (K2 à K5) | 83 (8, 14, 23, 37) | 209 (27, 45, 61, 77) |
| dont tri, ordre 5 | 1 | 26 |
| phase A restante | 104 | 60 |
| populations / images / banque / encodage | 11 / 20 / 1 / 31 | 19 / 26 / 3 / 33 |
| fils d'aide créés | 409 | 2 638 |

À K10 (08/000000), la tour vaut 1 266 et 1 280 ms, contre 1 886 et
1 944 ms pour le témoin.

## Lecture

- **Seuil d'une seconde à K5 sans sol : atteint sur deux trames sur
  trois.** 08/000000 : 0,972 et 0,998 s ; 08/000100 : 0,805 s. 08/000200
  reste à 1,026 s, soit 26 ms au-dessus. Ce sont des trames de la
  séquence 08, sans sol, avec la session d'appareil ouverte par le
  processus. Aucune qualification de contrat n'en découle.
- **Tour** : −120 ms à K5 et −620 à −664 ms à K10, par rapport au témoin
  apparié. La phase 0 est divisée par 2,5. La fenêtre est maintenant bornée
  par la phase A des ordres bas après la phase 0 : 104 ms à K5.
- **Session d'appareil** : l'attente de préparation disparaît (0 ms, contre
  30 à 72 ms pour le bras froid). Les paires froid/chaud diffèrent de 36 à
  102 ms à K5. À K10, l'écart est dans le bruit : le front, plus long,
  cachait déjà la préparation.
- **Trames brutes avec sol** : K5 1,91 à 2,18 s et K10 5,71 à 6,49 s, soit
  2,0 à 2,4 fois les trames sans sol, pour 3,1 fois plus de sites. Le pic
  de RSS atteint 8,9 Gio à K10. Le chemin GPU reporte déjà des arêtes, sans
  refus de capacité.
- **Reste à 08/000200/K5** (1,026 s) :
  - tour 0,30 s ;
  - q34 0,55 s : front 0,10, filtre 0,10, certificats 0,13, voies 0,13
    (dont 32 ms de transfert) ;
  - recensement 0,10 s.
