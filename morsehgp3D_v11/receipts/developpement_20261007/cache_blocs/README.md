# Cache de blocs du budget : adopté pour les modes de référence (session claudecache1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudecache1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudecache1/receipt.json`). Source : `ccdd4db75`. Trames LiDAR réelles ng00, ng01 et ng02, W48. Aucune
mesure ne promeut un statut public.

## Objet

`claudetas1` (reçu [`retention_tas`](../retention_tas/README.md)) a montré, avec un réglage de glibc, que garder les
grands blocs d'une passe à l'autre fait gagner le mur à chaud (moyenne géométrique 0,905). `ccdd4db75` en est
l'implémentation propre dans le moteur. Avec `MemoryBudget(limite, cache)`, les blocs d'au moins 256 Kio rendus par
les `Buffer` du budget sont gardés, jusqu'à `cache` octets inactifs, puis repris par la réservation suivante de la
même classe de taille (pas de 2^(1/8), au plus 32 blocs par classe). Le compte du budget (`used`, `peak`, `admit`,
`released`) reste celui des `Buffer` vivants, inchangé. Les blocs inactifs ont leur propre borne explicite, et sont
empoisonnés sous ASan. Sans cache, le comportement est celui d'avant. La sonde active un cache de 4 Gio avec le bit
524288. Chaque mode est comparé à son jumeau avec cache, dans la même source.

## Règle écrite dans le plan avant la session

Exactitude : cinq mutants du module core tués ; matrices ASan/UBSan et TSan conformes ; bancs conformes, vidages
identiques entre modes et égaux aux empreintes. Statistiques : 6 rapports cache/nu, K5 CPU feuilles 16 (`278523`
contre `802811`) et K5 GPU feuilles 24 (`344059:400` contre `868347:400`), trois trames. Adopté (le bit 524288 entre
dans les modes de référence) si la moyenne géométrique des 6 rapports du mur à chaud (médiane des passes 2 à 10 d'un
processus) est ≤ 0,95, et celle des 6 rapports du mur à froid (médiane de 6 processus) ≤ 1,00. Juge :
`claudecache1/judge.py`.

## Exactitude

- Mutants (`mut_core.txt`) : `cache_reprise_sans_retrait` (tué par signal : la porte voit le bloc partagé, puis la
  double restitution fait avorter le processus), `cache_capacite_ignoree`, `cache_inactif_non_decompte`,
  `cache_reprise_jamais` et `liberation_non_comptee` (reciblé), les quatre derniers au code de sortie.
- Matrice (`matrix_summary.json`) : `gcc_asan_ubsan` 804/804, `gcc_tsan` 804/804. Ces portes comprennent le groupe
  `block_cache`, avec huit fils concurrents.
- Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10.

## Verdict : adopté

| Mode | Trame | Mur à chaud nu → cache (ms) | Rapport | Mur à froid nu → cache (ms) | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 334,7 → 305,3 | 0,912 | 341,5 → 326,2 | 0,955 |
| CPU, 16 | ng01 | 262,6 → 258,2 | 0,983 | 276,6 → 284,6 | 1,029 |
| CPU, 16 | ng02 | 328,9 → 311,0 | 0,945 | 350,9 → 327,5 | 0,933 |
| GPU 400 ‰, 24 | ng00 | 285,9 → 249,9 | 0,874 | 356,6 → 349,3 | 0,980 |
| GPU 400 ‰, 24 | ng01 | 230,5 → 211,4 | 0,917 | 311,4 → 302,1 | 0,970 |
| GPU 400 ‰, 24 | ng02 | 279,9 → 254,7 | 0,910 | 357,3 → 350,8 | 0,982 |

Moyennes géométriques : chaud **0,923** (seuil 0,95), froid **0,974** (seuil 1,00). **Le bit 524288 entre dans les
modes de référence : `802811` en voie CPU (feuilles 16), `868347:400` en voie GPU à K5 (feuilles 24).** Le défaut de
l'API ne change pas ici : une `Session` crée encore son budget sans cache. Ce choix attend la réponse des auditeurs
sur le compte de la mémoire retenue (section Y, question 2).

## Lecture (descriptive)

- À chaud, la restitution des tampons passe de 14,2–14,5 ms à 0,8 ms en voie GPU, et de 8,1–9,5 ms à 3,7–4,4 ms en
  voie CPU. L'étage `domain` perd 18 à 25 ms en voie GPU.
- Le gain reste un peu en deçà du réglage glibc de `claudetas1` (0,923 contre 0,905). Le cache ne voit que les
  `Buffer` d'au moins 256 Kio, alors que le réglage couvrait toutes les allocations du processus.
- À K10 (voie GPU sans partage), le mur à chaud passe de 1895 / 1423 / 1633 ms à 1767 / 1327 / 1508 ms, et l'étage
  `domain` de 593 / 477 / 564 ms à 490 / 397 / 472 ms.
- Premier profil échantillonné des publieurs sur G4 (K5, voie GPU avec cache, ng00, médianes à froid). Le publieur
  de l'ordre 5 calcule 107 ms : cellules régulières environ 65 ms (145 ns par cellule), clôtures de plateaux environ
  29 ms (66 ns par plateau), le reste environ 13 ms. Les résolutions finissent vers 82 ms : le publieur 5 reste le
  chemin critique de l'étage, avec environ 25 ms d'avance sur elles. À K10, les résolutions et les publieurs
  finissent ensemble, vers 1213 ms.

## Pièces

`claudecache1/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `mut_core.txt`,
`gpu_ab_report_ab_k5_16_cpu.json`, `gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`,
`matrix_summary.json`, `matrix_stdout.txt`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers
sauf lui-même.
