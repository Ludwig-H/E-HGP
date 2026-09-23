# Reçu : session G4 R8, sonde v13 (Euler, occupation q34, phases de la tour)

23 septembre 2026, 09:39–09:46 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la cible
fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, AMD EPYC 9B45, 24 cœurs ×
2 fils), génération `2026-09-23T02:39:28.958-07:00`, arrêt ciblé certifié et
`TERMINATED` relu (dernier arrêt `2026-09-23T02:46:01.426-07:00`). GPU non
utilisé.

Paquet `515b3666` (sonde v13, plan v6, snapshot `c6a965a8…`, manifeste
`16a0c35a…`, worker `49e80b6f…`, contrôleur `cd6ff697…`, protocole au commit).
Reçus hôte et worker **`completed`**. Les 20 cas sont `complete_relative`, avec
un préflight natif non vacant. Les huit comparaisons d'objet entre nombres de
fils sont égales. Tous les leviers sont ON.

Plan (20 cas) :
- trois trames × K5/K10 × W48, deux répétitions ;
- s = 10 et 12 à K5 ;
- 08/000100 K5 à W24 et W1.

## Chaîne à W48, s = 8 (répétitions 0 / 1)

`chain_total` exclut le condensé de vérification, publié à part. Définitions
des deux colonnes q34 :
- **occupation** : CPU des fils q3/q4 / (fils × mur q34) ;
- **attente** : temps passé à attendre la file de tâches / (fils × mur).

| trame | K | chaîne (s) | q2 | q34 | tour | condensé (s) | occupation q34 | attente q34 | CPU·s |
| --- | ---: | --- | ---: | --- | --- | ---: | --- | --- | ---: |
| 000100 | 5 | 3,67 / 3,66 | 0,25 | 2,52 / 2,54 | 0,70 / 0,73 | 0,16 | 64 % / 63 % | 35 % / 36 % | 88 |
| 000100 | 10 | 9,40 / 9,52 | 0,41 | 5,24 / 5,34 | 2,97 / 3,02 | 0,80 | 83 % / 83 % | 16 % / 16 % | 263 |
| 000000 | 5 | 5,46 / 5,50 | 0,45 | 3,95 / 3,90 | 0,87 / 0,94 | 0,19 | 53 % / 54 % | 45 % / 44 % | 114 |
| 000000 | 10 | 13,69 / 13,73 | 0,78 | 8,03 / 8,07 | 3,90 / 3,89 | 0,99 | 71 % / 71 % | 28 % / 29 % | 346 |
| 000200 | 5 | 6,39 / 6,32 | 0,50 | 4,71 / 4,64 | 0,92 / 1,00 | 0,21 | 50 % / 50 % | 48 % / 49 % | 127 |
| 000200 | 10 | 15,19 / 15,37 | 0,82 | 9,76 / 10,07 | 3,59 / 3,61 | 1,00 | 64 % / 63 % | 35 % / 36 % | 367 |

Condensés de tour identiques à R7b sur les six couples trame/K.

## Phases de la tour (répétition 0, secondes de mur)

| trame | phase 0 | lots | populations | images | validation | banque | encodage | tour |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 K5 | 0,19 | 0,26 | 0,02 | 0,07 | 0,11 | 0,01 | 0,04 | 0,70 |
| 000100 K10 | 1,12 | 0,80 | 0,13 | 0,26 | 0,43 | 0,10 | 0,13 | 2,97 |
| 000000 K5 | 0,24 | 0,33 | 0,03 | 0,09 | 0,13 | 0,01 | 0,04 | 0,87 |
| 000000 K10 | 1,46 | 1,19 | 0,15 | 0,31 | 0,55 | 0,08 | 0,15 | 3,90 |
| 000200 K5 | 0,24 | 0,35 | 0,04 | 0,09 | 0,14 | 0,01 | 0,05 | 0,92 |
| 000200 K10 | 1,31 | 1,02 | 0,16 | 0,32 | 0,55 | 0,08 | 0,14 | 3,59 |

Les lots sont bornés par l'ordre K le plus lent (un fil) : à K10, lots_by_k[10]
vaut 0,80 à 1,19 s, exactement la phase.

## Lecture

- **Euler « holds » sur les trois trames réelles**, aux ordres 1..3 (K5) et
  1..8 (K10). C'est une condition nécessaire, pas une preuve de complétude.
- **Temps** : pas de gain net par rapport à R7b. K5 : 3,67 / 5,46 / 6,39 s ;
  K10 : 9,40 / 13,69 / 15,19 s, soit −1 à −2 % (Welzl à déplacement en tête).
- **q34 à W48 affamé** : entre 35 et 49 % du temps des fils à K5, et entre 16
  et 36 % à K10, s'écoulent à attendre la file de tâches. Cause :
  - tous les jobs du front (16 par fil) sont réclamés tôt ;
  - le front, le filtre de rectangle et les petits rectangles restent dans
    les quelques jobs longs ;
  - seul un rectangle de plus de 256 paires est publié comme tâche.

  À W8 en local, l'effet était invisible (0,05 à 0,2 % d'attente). **Premier levier
  d'ordonnancement** : sans changement d'objet, gain estimé jusqu'à ×1,5 sur
  q34 à K5.
- **Mise à l'échelle** (000100 K5) : q34 prend 42,5 s à W1, 3,57 s à W24 et
  2,52 s à W48, soit ×16,9 à 48 fils sur 24 cœurs. Le CPU total passe de 49 à
  88 s (fils jumeaux et attente).
- **Séparation s** : s = 10 et 12 donnent le même objet, en plus lent.
  Chaîne K5 de 000100 : 3,67 (s8), 3,77 (s10) et 4,19 s (s12). s = 8 reste le
  choix.
- **Tour K10** : phase 0 (MEB + intrus, parallèle) 1,1–1,5 s, lots
  (séquentiels par ordre) 0,8–1,2 s, validation 0,4–0,55 s, images 0,26–0,32 s.
- Contrat (1 s puis 100 ms) non atteint : aucune qualification.

## Contenu

- `vm/` : sorties du worker, préflight synthétique, 20 sondes, GNU time,
  preuves de garde, reçu `completed`.
- `host/` : contrôleur, reçu hôte `completed` avec `verified_guard`, journaux
  de démarrage et d'arrêt expurgés de l'adresse du compte. `oslogin_add`, la
  clé et les archives ne sont pas versionnés.
- `PACKAGE.json`, `SUMMARY.json` (schéma `mhgp9_g4_tower_summary_v2`, avec les
  blocs v13), `SHA256SUMS`.
