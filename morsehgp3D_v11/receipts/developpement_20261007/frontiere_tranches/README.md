# Rondes de la frontière préparées par tranches : règle non atteinte, retiré (session claudefront1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudefront1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudefront1/receipt.json`). Source : `3e6f88c7f`, contre la variante `base` (archive de `b0150926d`).
Trames LiDAR réelles ng00, ng01 et ng02, W48. Aucune mesure ne promeut un statut public. Le code est retiré par
`64746f985`.

## Objet

Sur une trame LiDAR, les premières rondes de la frontière adaptative n'ont qu'un à quelques parents de dizaines de
milliers de sites, donc deux à quelques dizaines de tâches pour 48 ouvriers. Dès qu'un parent dépassait 2048 sites,
chaque enfant était préparé par tranches. Le Pool calculait le réservoir de témoins de chaque tranche ; le pilote les
fusionnait dans l'ordre (distance, rang dans le parent). Le Pool filtrait ensuite chaque tranche, puis le pilote
compactait les listes. Le réservoir et le test G1 étaient partagés avec `prepare_node`, et les nœuds identiques octet
pour octet.

## Règle écrite dans le plan avant la session

Exactitude : six mutants tués, matrices TSan u21 et ASan/UBSan conformes, bancs conformes, vidages des empreintes.
Statistique : médiane des 6 processus à froid, K5 CPU feuilles 16 (`278523`) et K5 GPU feuilles 24 (`344059:400`),
trois trames, soit 6 rapports new/base. Gardé si la moyenne géométrique des 6 rapports de `prefix_ms` (préparation de
la frontière) est ≤ 0,80 et celle de `domain_ms` ≤ 1,00. Juge : `claudefront1/judge.py`.

## Exactitude

- Mutants (`mut_catalogue.txt`), tous tués au code de sortie : `prepare_tranches_fusion_rang_inverse`,
  `reservoir_egalite_gardee`, `prepare_tranches_compaction_omise`, `prepare_tranches_enveloppe_sans_increment`,
  `adaptive_un_seul_enfant_paye` (reciblé) et `dominance_egalite_retiree` (test G1 partagé).
- Matrice (`matrix_summary.json`) : `gcc_asan_ubsan` 806/806, `gcc_tsan` 806/806.
- Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10.

## Verdict : règle non atteinte, retiré

| Mode | Trame | Frontière base → new (ms) | Rapport | `domain` base → new (ms) | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 20,74 → 17,72 | 0,855 | 226,5 → 232,5 | 1,027 |
| CPU, 16 | ng01 | 17,93 → 16,21 | 0,904 | 198,3 → 179,6 | 0,906 |
| CPU, 16 | ng02 | 21,29 → 17,52 | 0,823 | 221,4 → 212,7 | 0,961 |
| GPU 400 ‰, 24 | ng00 | 21,46 → 17,87 | 0,833 | 233,8 → 234,6 | 1,003 |
| GPU 400 ‰, 24 | ng01 | 18,59 → 17,41 | 0,937 | 214,1 → 211,5 | 0,988 |
| GPU 400 ‰, 24 | ng02 | 22,27 → 18,08 | 0,812 | 237,0 → 236,6 | 0,998 |

Moyennes géométriques : frontière **0,859** (seuil 0,80, non atteint) ; `domain` 0,980. Le gain existe mais reste
petit. Les 17 à 18 ms qui restent ne tiennent pas surtout aux rondes étroites : préparation de la racine, sélection
et publication des rondes par le pilote, et une vingtaine de distributions du Pool, peut-être.

## Lecture (descriptive) : les nouveaux sous-chronos sur G4

Médianes à froid de la variante `base` (`b0150926d`, premiers relevés de `release_ms` et `lookup_ms` sur G4) :

| Mode | Restitution des tampons (`release_ms`) | Table support → boule (`lookup_ms`) | Tri | Frontière |
| --- | --- | --- | --- | --- |
| K5 CPU, 16 | 8,1 à 9,6 | 3,1 à 3,2 | 9,9 à 13,4 | 17,9 à 21,3 |
| K5 GPU 400 ‰, 24 | 11,2 à 17,0 | 3,0 à 3,2 | 8,7 à 12,5 | 18,6 à 22,3 |
| K10 GPU, 24 | 47,7 à 57,8 | 9,4 à 12,2 | 39,5 à 53,1 | 27,1 à 33,5 |

La restitution des tampons de la passe unique se fait sur un seul fil. Elle coûte plus que tout le gain visé ici :
11 à 17 ms à K5 en voie GPU, 48 à 58 ms à K10. En local, garder les grands blocs dans le tas de glibc (seuil `mmap`
relevé) la ramène de 12–17 ms à 2–4 ms, et supprime les fautes de page des passes suivantes. Cette piste se mesure à
part, sur G4.

## Pièces

`claudefront1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `mut_catalogue.txt`,
`gpu_ab_report_ab_k5_16_cpu.json`, `gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`,
`matrix_summary.json`, `matrix_stdout.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers
sauf lui-même.
