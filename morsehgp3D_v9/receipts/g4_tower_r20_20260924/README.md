# Reçu : session G4 R20 (étape 3 des voies, préparation en deux étapes, sonde v25)

24 septembre 2026, 19 h 30 – 19 h 35 UTC (VM). Cadre :
`exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-24T12:29:54.978-07:00`.
- **Arrêt** : ciblé et certifié par le contrôleur, puis `TERMINATED` relu en
  lecture seule (dernier arrêt `2026-09-24T12:34:45.629-07:00`), relu une
  seconde fois après un redémarrage du conteneur local.

**Paquet** : construit depuis `48791e72`, protocole au commit. Sonde v25.
Plan R20 à 18 cas : pour chaque trame et chaque K, le bras GPU (tous les
leviers, passe fusionnée L15 comprise) et son jumeau moteur ; à 08/000000,
des paires répétées et entrelacées fusionné / phases séparées
(`q34_lanes_fused`). Snapshot `5684b27c…`, manifeste `2941a59b…`, worker
`f3e199f7…`.

**Contenu mesuré** :
- **étape 3 des voies** : étape C sans boucle sur les tâches d'une arête,
  élagage exact L11, ordre de balayage L10, passe fusionnée L15 (levier) ;
- **préparation de l'appareil en deux étapes**, lancée à l'entrée de la
  chaîne.

**Reçus** : hôte et worker **`completed`**. `GPU_completed_cases` = 0, 2,
4, 6, 8, 10 et 12 à 17. Aucun cas non apparié.

## Préflights et objet

- Préflight jugé et jumeau moteur (`73490cf8`), ardoises réduites : 3 404
  arêtes rendues au moteur et 11 993 voies en traîne.
- 18 cas `complete_relative`, Euler « holds », les six épingles de C
  reproduites.
- Les 12 comparaisons entre cas sont égales, condensé des présentations
  compris : passe fusionnée, phases séparées et moteur donnent le même
  objet.

## Chaîne de bout en bout (s), W48

| trame | K | moteur | **GPU, fusionné** | GPU, séparé | R19 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 2,297 | **1,010** | | 1,026 |
| 000000 | 5 | 2,826 | **1,109 / 1,193** | 1,208 / 1,214 | 1,175 / 1,214 |
| 000200 | 5 | 3,142 | **1,260** | | 1,311 |
| 000100 | 10 | 6,767 | **3,147** | | 3,214 |
| 000000 | 10 | 9,144 | **3,873 / 3,940** | 4,010 / 3,977 | 4,060 / 4,012 |
| 000200 | 10 | 9,420 | **3,860** | | 4,053 |

Les écarts de chaîne entre les bras fusionné et séparé à K5 viennent de
l'attente de la préparation (11 et 39 ms contre 65 et 78 ms), pas des voies.

## Phases (ms)

| cas | front | filtre | préparation / attente | certificats (noyau) | voies (noyau : P / T / C) | transfert des voies | recensement | tour |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 K5 | 106 | 105 | 129 / 11 | 117 (90) | 112 (57 : 15 / 40 / 1) | 30 | 99 | 422 |
| 000100 K5 | 77 | 146 | 174 / 82 | 97 (71) | 94 (43 : 12 / 30 / 1) | 27 | 84 | 374 |
| 000200 K5 | 98 | 171 | 173 / 60 | 130 (101) | 128 (67 : 17 / 49 / 1) | 33 | 107 | 456 |
| 000000 K10 | 171 | 156 | 172 / 0 | 266 (229) | 454 (241 : 40 / 198 / 3) | 151 | 495 | 1 955 |

## Paires à 08/000000 : passe fusionnée L15

| | T fusionné | T séparé | noyau fusionné | noyau séparé |
| --- | ---: | ---: | ---: | ---: |
| K5 | 40,3 / 40,3 | 38,6 / 38,6 | 56,5 / 56,7 | 55,0 / 54,9 |
| K10 | 198,2 / 198,4 | 191,7 / 191,5 | 241,2 / 241,3 | 234,7 / 234,4 |

La passe fusionnée lit 40 % de paquets de moins (mesure hôte de PROVENANCE),
mais elle **ralentit** l'étape T de 4 % à K5 et de 3,5 % à K10. Le
débordement de registres qu'elle ajoute coûte plus que les paquets de
recensement qu'elle retire. Décision : le levier reste désactivé par défaut
et le plan suivant garde les phases séparées. Le lemme L15 reste prouvé.

## Lecture

- **Étape 3 des voies** (000000, par rapport à R19) : appel des voies
  145 → 112 ms à K5 et 580 → 454 ms à K10 ; noyau 91 → 57 ms et
  368 → 241 ms. L'étape C passe de 32 à 1 ms (K5), T de 49 à 40 ms.
- **Préparation en deux étapes : sans effet sur l'attente.** L'étape A dure
  129 à 189 ms, et c'est presque entièrement la création du contexte CUDA
  (`cudaFree(0)`) : l'aplatissement de l'index de 40 000 sites ne coûte que
  quelques millisecondes. Le filtre l'attend encore 11 à 82 ms à K5 (aucune
  attente à K10, où le front est plus long). Ce coût est payé une fois par
  processus. Un flux LiDAR à 10 Hz le paie une seule fois, au démarrage.
  Prochain poste : une session d'appareil ouverte par le processus avant la
  trame, avec son coût à froid publié à part.
- **Transfert des voies** : 30 ms à K5 et 151 ms à K10. Ce sont 850 000 et
  4,63 M enregistrements de 128 o, téléchargés en mémoire pageable (environ
  4 Go/s). Prochain poste : un tampon hôte épinglé résident.
- **Meilleures chaînes** : K5 1,01 / 1,11 / 1,26 s, K10 3,15 / 3,87 /
  3,86 s. Ce sont des trames sans sol de la séquence 08, sans contrat.
- **Reste à 000000/K5** (1,109 s) :
  - tour 0,42 s ;
  - q34 0,53 s : front 0,11, filtre 0,105 (dont 0,011 d'attente),
    certificats 0,12, voies 0,11 ;
  - recensement 0,10 s, fusion 0,03 s.
