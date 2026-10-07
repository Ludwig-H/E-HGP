# Tri des cohortes de naissances par tranches : gardé (session claudebirths1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudebirths1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudebirths1/receipt.json`). Source : `5734ca6e8`, contre la variante `base` (archive de `d050eb7bd`).
Trames LiDAR réelles ng00, ng01 et ng02, W48. Aucune mesure ne promeut un statut public.

## Objet

Dans les naissances par blocs (ordres concurrents, table dense), la phase 2 trie les cohortes de naissances de même
rang par centre exact. Elle formait une tâche par ordre, si bien que l'ordre K bornait la phase. Elle forme désormais
(K-1) x 32 tranches alignées sur les cohortes. Le pilote lit les bornes avant toute écriture : chaque tranche ne lit
et n'écrit que ses propres naissances. Il admet et alloue ensuite un tampon de la plus longue cohorte par ouvrier
(au plus 93 naissances de 160 octets sur ces trames) et un registre par tranche, puis ajoute les registres dans l'ordre
des tranches. Les décisions ne changent pas : mêmes cohortes, même tri, mêmes présentations et comparaisons.

## Règle écrite dans le plan avant la session

Exactitude : quatre mutants tués ; matrices TSan u21 (portes ordinaires et trame ng00) et ASan/UBSan conformes ;
bancs conformes ; vidages identiques entre variantes et égaux aux empreintes des trames LiDAR. Statistique : médiane des
6 processus à froid par trame et mode, à K5 en voie CPU avec des feuilles de 16 (`278523`) et en voie GPU de référence
avec des feuilles de 24 (`344059:400`), sur les trois trames, soit 6 rapports new/base. Gardé si (1) la moyenne
géométrique des 6 rapports de la phase des naissances (`pipeline.phases.births_ns`) est ≤ 0,80, et (2) celle des 6
rapports de `forest_ms` est ≤ 1,00. La phase visée ne pèse que 8 à 10 ms sur environ 110 ms : un plafond par rapport
sur `forest_ms` serait dominé par le bruit A/A (±9 à 12 %), d'où la statistique de la phase et la seule moyenne sur
`forest_ms`. Juge : `claudebirths1/judge.py`.

## Exactitude

- Mutants (`mut_tower.txt`), tous tués au code de sortie par `mhgp11_tower_pipeline_equivalence` :
  `births_blocs_cohortes_omises` (reciblé), `cohortes_registre_omis`, `cohortes_bornes_non_alignees` et
  `cohortes_premiere_tranche_omise`. La porte compare désormais les présentations et les comparaisons de centres de
  la voie concurrente à celles de la voie séquentielle, avec un plancher de 600 ordres à cohortes (768 observés).
- Matrice (`matrix_summary.json`, `matrix_stdout.txt`) : `gcc_asan_ubsan` 803/803, `gcc_tsan` 803/803,
  `gcc_tsan_lidar_ng00` 13/13.
- Les trois bancs sont `conforme`. À K5 comme à K10, toutes les prises des deux variantes rendent les vidages des
  empreintes des trames.

## Verdict : gardé

| Mode | Trame | Naissances base → new (ms) | Rapport | `forest_ms` base → new | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 8,57 → 4,68 | 0,546 | 122,3 → 115,6 | 0,945 |
| CPU, 16 | ng01 | 6,79 → 4,03 | 0,593 | 98,7 → 90,3 | 0,915 |
| CPU, 16 | ng02 | 11,59 → 5,31 | 0,459 | 131,7 → 121,7 | 0,924 |
| GPU 400 ‰, 24 | ng00 | 8,70 → 4,63 | 0,532 | 122,2 → 116,3 | 0,951 |
| GPU 400 ‰, 24 | ng01 | 6,77 → 4,04 | 0,597 | 93,9 → 91,6 | 0,976 |
| GPU 400 ‰, 24 | ng02 | 11,40 → 5,28 | 0,463 | 123,1 → 115,7 | 0,940 |

Moyennes géométriques : naissances **0,529** (seuil 0,80), `forest_ms` **0,942** (seuil 1,00). Les deux conditions
sont tenues.

## Lecture (descriptive)

- La phase des naissances perd 3 à 6 ms. Sur `forest_ms`, le gain médian (6 à 10 ms) dépasse ce que la phase seule
  explique : il peut tenir en partie au bruit à froid.
- À chaud, `forest_ms` donne 118,7 → 116,3 / 97,4 → 95,6 / 111,9 → 105,8 ms en voie CPU, et 130,9 → 103,8 /
  105,4 → 100,9 / 126,6 → 108,5 ms en voie GPU de référence.
- À K10 (voie GPU sans partage), les naissances passent de 22–30 à 19–23 ms ; `forest_ms` reste à 0,97–1,01 fois sa
  valeur, l'étage y durant 1,0 à 1,3 s.

## Pièces

`claudebirths1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `mut_tower.txt`,
`gpu_ab_report_ab_k5_16_cpu.json`, `gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`,
`matrix_summary.json`, `matrix_stdout.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers
sauf lui-même.
