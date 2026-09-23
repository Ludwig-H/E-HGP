# Reçu : session G4 R5, ordres de la tour en parallèle

23 septembre 2026, 03:20–03:27 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, AMD EPYC 9B45, 24 cœurs ×
2 fils), génération `2026-09-22T20:21:12.746-07:00`, arrêt ciblé certifié et
`TERMINATED` relu (dernier arrêt `2026-09-22T20:26:44.382-07:00`). GPU non
utilisé.

Paquet `aae9da0e` (protocole v6, snapshot `c2e5b996…`, manifeste
`5696158c…`). Reçu hôte et worker **`completed`** : treize cas
`complete_relative`, préflight natif non vacant accepté (condensé
`73490cf88c02af30`), sept comparaisons d'objet toutes égales. Code, depuis R4b :
les K ordres de la tour construits en parallèle (lots par ordre, populations
numérotées dans l'ordre séquentiel, images verticales après coup, `684d8fc7`),
banque de populations déplacée et validée en parallèle (`133c8653`), formes du
certificat de voie morte chargées sans branche depuis les coordonnées rangées
de l'index (`47f8a5da`, `aae9da0e`). Tous les leviers actifs, s = 8 ; chaque cas
est répété, plus 000000 K10 à 24 fils.

## Résultats (48 fils, tour statique 48 ; deux répétitions)

| trame | K | q3/q4 (s) | tour (s) | total (s) | CPU·s | RSS (Mo) | condensé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 000100 | 5 | 2,79 / 2,72 | 0,86 / 0,84 | **4,33 / 4,26** | 103 | 1 070 | `dbf799c8ed83f53f` |
| 000000 | 5 | 3,97 / 3,87 | 1,02 / 1,04 | **5,98 / 5,96** | 131 | 1 130 | `67450c64611075b1` |
| 000200 | 5 | 5,19 / 5,18 | 1,09 / 1,10 | **7,38 / 7,40** | 154 | 1 248 | `8240af3d4dce3d45` |
| 000100 | 10 | 5,50 / 5,56 | 4,15 / 4,01 | **12,09 / 12,07** | 311 | 3 845 | `9ddbf7430c9086cc` |
| 000000 | 10 | 8,57 / 8,35 | 5,37 / 5,26 | **17,37 / 17,00** | 425 | 4 652 | `ac108f7f71096c3f` |
| 000200 | 10 | 11,20 / 10,89 | 4,85 / 4,96 | **19,57 / 19,28** | 461 | 4 833 | `ba973af0c8da95bd` |

000000 K10 à 24 fils : q3/q4 12,15 s, tour 5,89 s, total 21,59 s, 300 CPU·s.
Les condensés sont ceux des sessions R1 à R4b.

## Lecture

- Tour, contre R4b (même cible, autre paquet, cache actif) : K10
  14,89 → 5,37 s, 11,42 → 4,15 s, 14,58 → 4,85 s ; K5 2,47 → 1,02 s,
  2,01 → 0,86 s, 2,65 → 1,09 s. Comparaison entre sessions, pas une ablation
  appariée ; les objets sont égaux.
- Chaîne complète : **4,3 / 6,0 / 7,4 s à K5** (R4b 5,4 / 7,4 / 9,2 s) et
  **12,1 / 17,4 / 19,6 s à K10** (R4b 19,4 / 26,7 / 29,5 s).
- q3/q4 est désormais le premier poste : 65 à 70 % du total à K5, 45 à 57 %
  à K10. Le chargement des formes ne le déplace que de 0 à 5 %.
- 24 → 48 fils sur 000000 K10 : q3/q4 ×1,42, tour ×1,10, total ×1,24, pour
  +42 % de CPU·s : les 48 fils sont 24 cœurs à deux fils matériels. La tour
  est bornée par son plus gros ordre (dix tâches au plus).
- Contrat (1 s puis 100 ms) non atteint : aucune qualification.

## Contenu

`vm/` (sorties du worker, préflight, sondes, GNU time, résumés, preuves de
garde, reçu `completed`), `host/` (contrôleur, reçu hôte `completed`, journaux
de démarrage et d'arrêt expurgés de l'adresse du compte ; `oslogin_add`, clé et
archives non versionnées), `PACKAGE.json`, `SUMMARY.json`, `SHA256SUMS`.
