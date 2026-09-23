# Reçu : session G4 R6, ablation appariée du noyau diamétral

23 septembre 2026, 04:43–04:51 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, 24 cœurs × 2 fils),
génération `2026-09-22T21:43:10.709-07:00`, arrêt ciblé certifié et
`TERMINATED` relu (dernier arrêt `2026-09-22T21:50:54.848-07:00`). GPU non
utilisé.

Paquet `78ce9fd4` (sonde v8, plan v5, snapshot `38c7e674…`, manifeste
`d8506da8…`, protocole pris au commit). Reçu hôte et worker **`completed`** :
24 cas `complete_relative`, préflight natif non vacant (tous leviers ON,
condensé `73490cf88c02af30`), dix-huit comparaisons d'objet toutes égales ;
réception liée à la marque et au calendrier vérifiés par l'hôte
(`verified_guard`). Seul levier variant : `q34_dead_core` ; 48 fils, tour
statique 48, s = 8, deux répétitions entrelacées ON/OFF.

Un premier lancement a été refusé **avant tout appel GCP** : clé de session
créée en mode 644 par l'ACL du répertoire, le contrôleur exige 600
(`host/refused_before_gcp/`). La clé a été passée en 600 et la session relancée
dans le même répertoire.

## Résultats (deux répétitions ; OFF → ON)

| trame | K | total (s) | q3/q4 (s) | CPU·s | formes chargées (G) | condensé |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| 000100 | 5 | 4,21 / 4,24 → 4,15 / 4,15 | 2,71 → 2,62 | 101 → 91 | 1,79 → 0,39 | `dbf799c8ed83f53f` |
| 000000 | 5 | 6,03 / 6,00 → 6,00 / 5,76 | 3,95 → 3,84 | 131 → 113 | 2,96 → 0,61 | `67450c64611075b1` |
| 000200 | 5 | 7,52 / 7,42 → 6,87 / 6,92 | 5,26 → 4,65 | 154 → 123 | 3,59 → 0,65 | `8240af3d4dce3d45` |
| 000100 | 10 | 12,01 / 12,07 → 11,73 / 11,90 | 5,51 → 5,29 | 310 → 279 | 4,15 → 1,13 | `9ddbf7430c9086cc` |
| 000000 | 10 | 17,18 / 17,03 → 16,77 / 16,67 | 8,41 → 8,11 | 422 → 368 | 7,80 → 1,77 | `ac108f7f71096c3f` |
| 000200 | 10 | 19,34 / 19,82 → 18,06 / 18,11 | 11,31 → 9,78 | 457 → 384 | 9,28 → 2,03 | `ba973af0c8da95bd` |

Formes chargées : `dead_form_sites + dead_core_form_sites`. Arêtes closes par
le noyau : 0,95 à 2,90 M selon le cas ; covers construits −55 à −59 %.

## Lecture

- Objets identiques ON/OFF dans chaque paire (générateur, catalogue, ordres,
  condensé) ; le levier ne change que le travail.
- CPU de chaîne **−10 à −20 %**, mur −2 à −8 % : à 48 fils (24 cœurs à deux
  fils matériels), le gain de CPU·s ne se traduit qu'en partie en mur.
- Masse de formes chargées −77 à −82 %.
- Le défaut ON de la chaîne est gardé. Contrat (1 s puis 100 ms) non atteint :
  aucune qualification.

## Contenu

`vm/` (sorties du worker, préflight, sondes, GNU time, résumés, preuves de
garde, reçu `completed`), `host/` (contrôleur, reçu hôte `completed` avec
`verified_guard`, journaux de démarrage et d'arrêt expurgés de l'adresse du
compte, refus local du premier lancement ; `oslogin_add`, clé et archives non
versionnées), `PACKAGE.json`, `SUMMARY.json`, `SHA256SUMS`.
