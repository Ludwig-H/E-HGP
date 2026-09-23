# Reçu : session G4 R3, ablation appariée du certificat de voie morte

23 septembre 2026, 01:51–02:01 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : une session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, AMD EPYC 9B45, 24 cœurs,
48 fils), génération `2026-09-22T18:51:14.877-07:00`, arrêt ciblé certifié
(`targeted_shutdown_certified: true`) et état `TERMINATED` relu après la
session (dernier arrêt `2026-09-22T19:01:10.231-07:00`). GPU non utilisé.

Paquet construit depuis le commit **`b4e480fc`** (protocole v5 committé,
`PACKAGE.json`, snapshot `761cc6e6…`, manifeste `03bb470a…`), compilé sur
la VM par GCC 11.4 en Release strict. Reçu hôte et worker : **`completed`**,
quatorze cas `complete_relative`, preflight natif accepté (1 500 sites,
condensé `73490cf88c02af30`), huit comparaisons d'objet entre cas
appariés toutes égales.

## Plan et résultats

Trois trames sans sol à 1 mm de la séquence 08, s = 8, 48 fils, tour
statique à 48, atlas saturant et census q3 sur feuille actifs ; chaque
(trame, K) est mesuré **avec puis sans** le certificat de voie morte, seul
levier qui varie. Deux cas en plus : 000000 K10 avec certificat répété, et
sur 24 fils.

| trame | K | q3/q4 sans → avec (s) | total sans → avec (s) | CPU·s sans → avec | condensé |
| --- | ---: | ---: | ---: | ---: | --- |
| 000000 | 5 | 8,19 → 4,14 | 12,93 → 8,89 | 398 → 165 | `67450c64611075b1` |
| 000100 | 5 | 6,43 → 2,48 | 10,12 → 6,19 | 314 → 116 | `dbf799c8ed83f53f` |
| 000200 | 5 | 14,63 → 5,81 | 19,71 → 10,93 | 642 → 190 | `8240af3d4dce3d45` |
| 000000 | 10 | 22,57 → 9,70 | 48,33 → 35,56 | 1 161 → 497 | `ac108f7f71096c3f` |
| 000100 | 10 | 16,63 → 6,24 | 36,29 → 25,96 | 862 → 349 | `9ddbf7430c9086cc` |
| 000200 | 10 | 37,75 → 12,49 | 62,99 → 37,75 | 1 799 → 544 | `ba973af0c8da95bd` |

Répétition 000000 K10 : 35,46 s (q3/q4 9,73 s). Sur 24 fils : 38,92 s
(q3/q4 12,39 s, tour 23,16 s).

## Lecture

- Le certificat divise le temps q3/q4 par 2,0 à 2,6 à K5 et 2,3 à 3,0 à K10,
  et le CPU total par 2,3 à 3,4, **sans changer l'objet** : condensés,
  catalogue et ordres égaux entre cas appariés, et égaux à R1/R2.
- La tour devient le poste dominant à K10 (17,5 à 22,8 s sur 26 à 38 s) et
  ne gagne presque rien de 24 à 48 fils : sa partie séquentielle est le
  prochain verrou. RSS inchangé (≈ 1,2 Go à K5, 4,9 Go à K10).
- Écart au contrat d'une seconde : ×6 à ×11 à K5, ×26 à ×38 à K10. Une
  répétition seulement par cas apparié ; pas de qualification de contrat.

## Contenu

- `vm/` : sorties du worker (preflight, sondes JSON, `/usr/bin/time -v`,
  résumés, compilateur, lscpu, preuves de garde, manifestes de sources,
  reçu du worker `completed`).
- `host/` : enregistrements du contrôleur, reçu hôte `completed`, marques de
  garde, journaux de démarrage et d'arrêt **expurgés** (adresse du compte
  masquée). Sorties `oslogin_add`, clé éphémère et archive non versionnées
  (paquet reconstructible depuis `b4e480fc`).
- `SUMMARY.json` (dérivé des sorties brutes), `SHA256SUMS`.
