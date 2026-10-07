# Préchargements combinés des publieurs : règle non atteinte, retiré (session claudepref2)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudepref2`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudepref2/receipt.json`). Source : `e35db29c3`, contre la variante `base` (archive de `35185c228`). Trames
LiDAR réelles ng00, ng01 et ng02, W48, modes de référence avec cache de blocs. Aucune mesure ne promeut un statut
public. Le code est retiré par `4de17f141`.

## Objet

Levier distinct de celui de `claudepref1` (reçu [`prechargement_graines`](../prechargement_graines/README.md)), qu'il
reprend. Les publieurs préchargeaient le nœud de chaque graine, 16 jobs en avance. Dans `touch`, ils préchargeaient
aussi le nœud courant de chaque composante touchée, que la clôture du plateau relit puis rattache.

## Règle écrite dans le plan avant la session

Exactitude : bancs conformes, vidages identiques entre variantes et égaux aux empreintes. Statistique : médiane des 6
processus à froid, K5 CPU feuilles 16 (`802811`) et K5 GPU feuilles 24 (`868347:400`), trois trames, soit 6 rapports
new/base. Gardé si la moyenne géométrique des 6 rapports de la somme des temps estimés des cellules et des clôtures du
publieur 5 (profil échantillonné) est ≤ 0,90, et celle de `forest_ms` ≤ 1,00. Le seuil de 0,90 tenait compte de la
dispersion observée dans `claudepref1`. Juge : `claudepref2/judge.py`.

## Verdict : règle non atteinte, retiré

Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10.

| Mode | Trame | Cellules + clôtures du publieur 5 (ms) | Rapport | `forest_ms` base → new | Rapport |
| --- | --- | --- | ---: | --- | ---: |
| CPU, 16 | ng00 | 94,4 → 88,1 | 0,933 | 120,2 → 117,6 | 0,978 |
| CPU, 16 | ng01 | 70,2 → 69,7 | 0,992 | 96,0 → 91,2 | 0,950 |
| CPU, 16 | ng02 | 87,0 → 80,7 | 0,927 | 117,9 → 115,6 | 0,980 |
| GPU 400 ‰, 24 | ng00 | 88,7 → 87,7 | 0,988 | 115,9 → 113,1 | 0,976 |
| GPU 400 ‰, 24 | ng01 | 71,5 → 64,9 | 0,908 | 93,3 → 93,2 | 0,999 |
| GPU 400 ‰, 24 | ng02 | 91,2 → 87,2 | 0,956 | 117,0 → 118,7 | 1,014 |

Moyennes géométriques : cellules + clôtures **0,950** (seuil 0,90, non atteint) ; `forest_ms` 0,983. Retiré.

## Lecture (descriptive)

- Les clôtures ne bougent pas : 31,9 → 31,3 ms en voie CPU sur ng00, 26,0 → 25,5 ms en voie GPU. Le préchargement du
  nœud courant n'y fait rien.
- Les cellules gagnent environ 6 % (64,6 → 60,6 ms, 64,9 → 61,5 ms sur ng00), moins que les 8 à 19 % de
  `claudepref1`. L'effet des préchargements est petit et se reproduit mal d'une session à l'autre.
- Conclusion : le coût du publieur de l'ordre 5 est structurel (environ 450 000 cellules et 440 000 clôtures, en
  série) ; des préchargements ne le changent guère. Un gain net demanderait de changer sa structure, par exemple en
  séparant unions et clôtures sur deux fils.

## Pièces

`claudepref2/` : `plan.json`, `judge.py`, `launch.json`, `receipt.json`, `gpu_ab_report_ab_k5_16_cpu.json`,
`gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
