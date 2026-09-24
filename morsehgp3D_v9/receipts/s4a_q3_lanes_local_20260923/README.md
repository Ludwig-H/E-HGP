# Reçu local : voie q3 par lots sans atlas (S4a), trame 08/000000

24 septembre 2026, codespace (hôte partagé, `nproc` dans `out/nproc.txt`).
Cadre : `exploration_v9_hors_registre`, `backend=reference_cpu`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
**GCP non utilisé.**

- **Commit** `50758024` (`out/commit.txt`). Le script refuse de tourner si
  les sources suivies de `morsehgp3D_v9` diffèrent de HEAD.
- **Empreintes** de la trame, de la sonde, de la porte de port, de
  l'archive `libmhgp9_gen.a` et du binaire de mesure en ordre de rang :
  `out/inputs.sha256`.
- **Trame** : sans sol 08/000000 de la v8, 39 885 sites, s = 8, W8.
- **Relance** depuis la racine : `bash
  morsehgp3D_v9/receipts/s4a_q3_lanes_local_20260923/run.sh <build> <out>`,
  avec un build Release au même commit.

Les compteurs sont déterministes. Les temps sont indicatifs : l'hôte était
partagé avec d'autres calculs.

## Travail par arête de la voie q3 (`out/stats_*.txt`)

Arêtes certifiées dont la voie q3 reste ouverte, émulation hôte de
`gpu/lanes.hpp`. Les pas de warp sont une borne inférieure : `⌈tests de
points / 32⌉` par arête.

| K | ordre | tests de points (somme / p99 / max) | pas de warp (somme / p99 / max) |
| ---: | --- | ---: | ---: |
| 5 | rang (1 anneau) | 5 285 780 658 / 51 712 / 24 782 835 | 165 476 132 / 1 616 / 774 464 |
| 5 | **8 anneaux** | **145 783 580** / 1 575 / 976 330 | **4 854 160** / 50 / 30 511 |
| 10 | 8 anneaux | 980 035 984 / 6 012 / 4 567 949 | 31 266 508 / 188 / 142 749 |

Couplage graines × cover par arête (auditeur, AUDIT_S4_COUPLAGE) :
- K5 : 701 678 arêtes, cover p50 / p99 / max 24 / 2 375 / 18 864 sites,
  graines 3 / 159 / 9 415, boules 0 / 6 / 14 ;
- K10 : 1 437 421 arêtes, cover 48 / 3 624 / 20 114, graines 7 / 273 /
  10 473, boules 1 / 11 / 27.

Les huit anneaux autour du milieu divisent par 36 le travail des
recensements à K5 (arrêt des recensements rejetés).

## Chaîne (`out/k*_{s3,q3,q3_judged}.json`, `out/SUMMARY.json`)

| K | bras | condensés tour / catalogue | chaîne (ms) | arêtes (ms) | appel q3 (ms) |
| ---: | --- | --- | ---: | ---: | ---: |
| 5 | S3 CPU | `67450c64…` / `5ad1fe09…` | 26 569 | 7 136 | |
| 5 | S3 + q3 CPU | idem | 27 493 | 4 320 | 3 364 |
| 5 | S3 + q3 CPU jugé | idem | 32 481 | 4 215 | 8 827 |
| 10 | S3 CPU | `ac108f7f…` / `a6e959d2…` | 77 298 | 27 363 | |
| 10 | S3 + q3 CPU | idem | 81 664 | 21 357 | 10 565 |
| 10 | S3 + q3 CPU jugé | idem | 103 718 | 21 226 | 32 319 |

- Les condensés sont les épingles de l'auditeur C sur les six exécutions.
  Toutes les voies demandées sont décidées (701 678 à K5, 1 437 421 à K10),
  et toutes sont rejugées par la voie q3 du moteur dans les bras jugés.
- Sur le CPU, l'appel q3 précède les ouvriers et coûte un peu plus que la
  voie q3 à atlas qu'il remplace (chaîne +3 à +6 %). Le levier CPU est une
  référence ; le gain vient de l'appareil, qui recouvre q4 (reçu
  [R15](../g4_tower_r15_20260923/README.md)).

## Contenu

`run.sh`, `out/` (sorties, `SUMMARY.json`, `SHA256SUMS`).
