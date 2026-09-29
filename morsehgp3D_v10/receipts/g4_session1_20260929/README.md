# Reçu : première session G4 v10, CPU seul (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. **Aucun calcul sur GPU** : la v10 n'a pas encore de voie CUDA,
et le GPU de la VM n'a pas servi.

## Session

- Protocole `gcp-migration/v10_session.py` (commit `11d7ad25f`), code v10 au commit `8b8d66f6e`, plan
  `plan_s1b.json` (20 commandes), 51 fichiers de données (trames et secteurs LiDAR sans sol, nuages synthétiques).
- VM `g4-standard-48` SPOT : AMD EPYC 9B45, 24 cœurs physiques et 48 fils, g++ 11.4. Les temps ne sont pas
  comparables à ceux du codespace (g++ 13.3, machine partagée).
- Démarrage gardé à 07:27 UTC. Arrêt certifié : `stop_and_verify.sh` (code 0), puis `describe` TERMINATED sur la
  cible exacte, avec `lastStartTimestamp` = notre génération `2026-09-29T00:27:21.203-07:00` et
  `lastStopTimestamp` = `00:36:36.335-07:00`. Clé OS Login retirée, clé privée effacée.
- Statut `failed_remote` : 19 commandes sur 20 ont réussi. Seule `gates` a échoué (code 8), parce que la VM n'a ni
  `pip` ni `numpy` (`results/setup/`) : les portes Python (oracles, sklearn, couverture) n'ont pas pu y tourner. Elles
  sont vertes en local, 7 sur 7. Aucune commande de mesure n'en dépend.
- `receipt.json` et `preflight.json` sont copiés avec l'adresse du compte masquée (`<compte>`). Les sha256 des
  originaux sont dans `ORIGINAUX.sha256`.

## Trames LiDAR entières (tour FULL, sans attaches, dernière passe chaude, 48 fils)

| Trame | Sites | K | Catalogue (s) | Tour (s) | Total (s) | RSS max (Gio) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 00 | 39 885 | 5 | 0,738 | 0,080 | 0,818 | 0,82 |
| 01 | 35 551 | 5 | 0,518 | 0,066 | 0,584 | 0,67 |
| 02 | 45 845 | 5 | 1,105 | 0,085 | 1,190 | 0,91 |
| 00 | 39 885 | 10 | 3,085 | 0,457 | 3,542 | 3,17 |
| 01 | 35 551 | 10 | 2,123 | 0,330 | 2,453 | 2,46 |
| 02 | 45 845 | 10 | 4,231 | 0,407 | 4,638 | 3,13 |

- Trame 02 à K = 5 selon le nombre de fils : catalogue 8,463 s à 1 fil, 1,188 s à 24 fils, 1,105 s à 48 fils
  (×7,7) ; tour 1,154 s, 0,108 s et 0,085 s (×13,6). **Le catalogue domine et passe mal à l'échelle.**
- Entrée des points par première couverture (`--entry=cover`), étage de la tour à 48 fils : 0,10 à 0,11 s à K = 5,
  et 0,39 à 0,50 s à K = 10, soit 0,02 à 0,09 s de plus que la tour seule.
- Chaîne complète jusqu'aux étiquettes (`mhgp10_cluster`, K = 5, couverture, EOM, z = 2, mcs = 200, 48 fils) :
  catalogue 0,57 à 1,16 s, tour 0,04 à 0,06 s, tête 0,14 à 0,17 s, soit 0,75 à 1,39 s. 45 à 59 amas par trame.

## Mise à l'échelle (48 fils, K = 5 pour le synthétique, K = 5 et 10 pour le LiDAR)

Exposant = log2 du rapport des compteurs, divisé par log2 du rapport des effectifs. Les compteurs déterministes
(boules, jugements, nœuds, pas) sont la mesure fiable ; les temps muraux à 48 fils sont bruités.

- **Synthétique, croissance spatiale** (effectif doublé à densité constante, ×1 → ×2 → ×4 depuis 8 000 points) :
  exposant de 1,00 à 1,05 sur les cinq familles. Linéaire.
- **Synthétique, croissance de densité** (même domaine, effectif doublé) : de 1,03 à 1,31. Le surcoût se concentre
  sur `filaments` et `shells`, où la densité croissante résout l'épaisseur du support : le voisinage local passe
  d'une géométrie de courbe ou de surface à une géométrie de volume.
- **LiDAR, secteurs coupés au capteur** (quart → moitié → trame, coupes parallèles aux axes par l'origine) : de 0,7
  à 1,4 selon les secteurs, qui ne sont pas homogènes. Globalement proche de 1.

## Lecture

- Le catalogue fait 85 à 93 % du temps. Il plafonne pour deux raisons :
  - son assemblage final (collecte, tri canonique, rangs, copie) était séquentiel, environ 0,5 s à 48 fils ;
  - l'énumération des boîtes ne gagne presque plus au-delà des 24 cœurs physiques.
- Le premier point est corrigé au commit suivant : assemblage parallèle, catalogue identique octet pour octet. Le
  second demande soit une voie GPU pour l'énumération des feuilles, soit une réduction du travail par feuille.
- Le contrat de la v9, 1 s à K = 5 sur G4, est tenu sur deux trames sur trois en CPU seul, et manqué de 0,19 s sur
  la trame 02.
