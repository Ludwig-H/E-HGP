# Test G1 du filtre par masque AVX2 : règle non atteinte, retiré (session claudeg1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudeg1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudeg1/receipt.json`). Source : `b6fd3796d`, contre la variante `base` (archive de `733912e65`). Trames
LiDAR réelles ng00, ng01 et ng02, W48, modes de référence avec cache de blocs. Aucune mesure ne promeut un statut
public. Le code est retiré par `23b759dfe`.

## Objet

Le diagnostic `claudediag1` montrait que, en voie lot, la passe unique est à peu près entièrement faite du filtre G1 des
nœuds internes (1,4 à 1,6 s de CPU à K5). Le levier calculait le test G1 par un masque AVX2 de tous les témoins, choisi
à l'exécution, en arithmétique entière seulement. Le rang du K-ième dominateur redonnait l'arrêt et le compte de tests
de la boucle de référence, si bien que décisions et registre restaient identiques. La boucle scalaire restait le repli.
En local (W8), la passe unique de la voie lot baissait de 28 à 36 % et la frontière de 30 à 39 %, à registre identique.

## Règle écrite dans le plan avant la session

Exactitude : cinq mutants tués ; matrices ASan/UBSan et TSan conformes ; bancs conformes, vidages des empreintes.
Statistique : médiane des 6 processus à froid, K5 CPU feuilles 16 (`802811`) et K5 GPU feuilles 24 (`868347:400`),
trois trames, soit 6 rapports new/base. Gardé si la moyenne géométrique des 6 rapports de `single_pass_ms + prefix_ms`
est ≤ 0,85 et celle de `domain_ms` ≤ 1,00. Juge : `claudeg1/judge.py`.

## Exactitude

- Mutants (`mut_catalogue.txt`), tous tués au code de sortie : `dominance_egalite_retiree` (reciblé sur la boucle de
  référence), `g1_avx2_egalite_comptee`, `g1_avx2_arret_decale`, `g1_avx2_max_omis` et `g1_filtre_avx2_garde_inverse`.
- Matrice (`matrix_summary.json`) : `gcc_asan_ubsan` 807/807, `gcc_tsan` 807/807.
- Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10.

## Verdict : règle non atteinte, retiré

| Mode | Trame | Passe unique + frontière base → new (ms) | Rapport | `domain_ms` base → new | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 186,8 → 171,2 | 0,916 | 217,9 → 200,6 | 0,921 |
| CPU, 16 | ng01 | 148,7 → 146,3 | 0,984 | 174,2 → 172,4 | 0,990 |
| CPU, 16 | ng02 | 176,6 → 159,8 | 0,905 | 211,2 → 193,8 | 0,918 |
| GPU 400 ‰, 24 | ng00 | 59,9 → 49,5 | 0,827 | 215,4 → 215,5 | 1,001 |
| GPU 400 ‰, 24 | ng01 | 47,4 → 41,3 | 0,872 | 204,9 → 203,0 | 0,991 |
| GPU 400 ‰, 24 | ng02 | 56,0 → 49,1 | 0,876 | 224,6 → 217,8 | 0,969 |

Moyennes géométriques : passe unique + frontière **0,895** (seuil 0,85, non atteint) ; `domain_ms` 0,964. Retiré.

## Lecture (descriptive)

- Le gain est réel mais plus faible que localement. En voie GPU, passe unique et frontière perdent 12 à 17 %. En voie
  CPU, la passe unique contient aussi les feuilles, que le levier ne touche pas : la statistique y diluait l'effet.
  C'est une erreur de conception de la règle, que je relève sans la corriger après coup.
- À chaud, le mur passe de 313,5 / 255,1 / 313,2 à 307,9 / 242,6 / 297,5 ms en voie CPU, et de 251,3 / 212,2 / 255,4 à
  250,9 / 205,0 / 243,3 ms en voie GPU. L'étage `domain` baisse de 6 à 11 ms à chaud dans les deux voies.
- À K10, l'étage `domain` à chaud passe de 489 / 398 / 471 à 481 / 390 / 462 ms (descriptif).
- En local, après vectorisation, le réservoir de témoins (270 ms) pèse déjà plus d'un tiers du filtre (464 ms pour G1).

## Pièces

`claudeg1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `mut_catalogue.txt`,
`gpu_ab_report_ab_k5_16_cpu.json`, `gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`,
`matrix_summary.json`, `matrix_stdout.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers
sauf lui-même.
