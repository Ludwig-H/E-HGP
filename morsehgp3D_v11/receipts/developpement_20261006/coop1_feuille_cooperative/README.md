# Feuille coopérative (un warp par feuille) : session G4 coop1, exacte, critère de vitesse non atteint

6 octobre 2026. Session gardée `v11.20261006.claudecoop1` sur la cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `c3df81805`, binaire CUDA sm_120 (`bench/gpu_ab.py`), trames entières sans sol
ng00, ng01 et ng02, W48. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed`.

## Exactitude : conforme

| Contrôle | Résultat |
| --- | --- |
| Portes hôte natives sur G4 | 6/6 : `mhgp11_catalogue_leaf_coop_*` et `mhgp11_tower_full_leaf_lanes` |
| Banc K5, feuilles de 16 | Verdict `conforme`. CPU 16379, GPU un fil 81915, GPU coopératif 212987 : dumps et registres du catalogue identiques, à froid comme à chaud |
| Banc K10, feuilles de 24 | Verdict `conforme`, même identité |
| Porte `coop_g4_gate` | 16 prises sur deux nuages synthétiques (CPU, GPU un fil, GPU coopératif, émulation). Lots identiques. 45 feuilles non résolues sur le nuage B à K10, rejouées sur le CPU. Les deux chemins d'écriture sont exercés |
| Compute Sanitizer | memcheck, racecheck, synccheck : 6 prises, 0 erreur, dumps identiques au CPU |

## Vitesse (médianes à chaud, ms) : critère écrit d'avance non atteint

| K, feuilles | Trame | `domain` CPU / GPU un fil / GPU coop | Exécuteur GPU un fil / coop |
| --- | --- | --- | --- |
| K5, 16 | ng00 | 208 / 249 / 250 | 83 / 84 |
| K5, 16 | ng01 | 168 / 212 / 207 | 78 / 67 |
| K5, 16 | ng02 | 202 / 215 / 235 | 59 / 77 |
| K10, 24 | ng00 | 827 / 843 / 775 | 483 / 414 |
| K10, 24 | ng01 | 655 / 728 / 612 | 439 / 325 |
| K10, 24 | ng02 | 780 / 817 / 728 | 456 / 366 |

**Critère** (plan, écrit avant la session) :
- K5 : exécuteur coopératif ≤ 0,5 × GPU un fil. Non atteint : 0,8 à 1,3.
- K10 : `domain` coopératif ≤ 0,85 × CPU. Non atteint : 0,93 à 0,94.

À K10, la voie coopérative bat tout de même le CPU de 6 à 7 % sur `domain`, et le GPU un fil de 14 à 26 % sur
l'exécuteur.

## Deux défauts trouvés par cette session

**1. Régression de la voie GPU un fil**, introduite par le refactoring de `c3df81805`. Le CPU est identique à la session
L4 du matin (207, 168, 202 ms à K5), donc la machine aussi. Pourtant l'exécuteur GPU un fil passe de 59, 54, 52 à 83,
78, 59 ms (K5), et de 340, 301, 318 à 483, 439, 456 ms (K10).

Le comptage ne bouge presque pas (+3 à 5 %). La seconde passe d'écriture double : 17 → 39 ms à K5 sur ng00, 111 → 244
ms à K10. Cette passe ne rejoue que quelques feuilles lourdes, un fil chacune : elle est limitée par la latence.

Cause : la feuille atteignait ses tables par une référence. En mémoire locale, chaque accès devient alors un chargement
générique précédé de celui du pointeur. Le SASS du noyau d'écriture le montre : 364 `LD` génériques contre 173 avant le
refactoring.

Correction (commit suivant) : la feuille d'un fil possède de nouveau ses tables par valeur (`Leaf<Sink, false>`). Le
SASS revient à 176 `LD` et 301 `LDL`, comme avant. La mesure reste à refaire sur G4.

**2. `atomicOr` sur la mémoire locale.** Dans la feuille d'un fil, le cache J2 simulé appelait l'atomique sur des tables
locales (avertissement de ptxas « Cannot do atomic on local memory »). L'atomique est désormais réservé aux tables
partagées (`seen_test_set<Shared>`).

Les registres du catalogue de cette session sont identiques à ceux du CPU dans tous les modes, cache compris. Cette
session ne montre donc aucune erreur de compte.

## Pourquoi la feuille coopérative gagne peu

Ce découpage répartit les **sous-arbres des paires** entre les lanes. Chaque lane suit alors un chemin de code différent
(q2, q3, q4, recensement) : la divergence SIMT reste entière. Seuls la queue et l'équilibrage à l'intérieur d'une feuille
s'améliorent.

S'y ajoutent deux coûts :
- les feuilles émettrices jouent deux fois leurs sous-arbres, une fois pour compter et une fois pour écrire dans leur
  case ;
- 198 registres limitent l'occupation à environ 10 warps par SM.

Le gain de J3 sur la v10 venait d'un parallélisme **de données** : un même préfixe, recensé sur tous les sites par
masques. La suite proposée aux auditeurs (section Q de la note) est donc une feuille où le warp entier parcourt le même
préfixe, avec les lanes réparties sur les sites du recensement et sur les combinaisons du support canonique.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs A/B |
| `coop_g4_gate.json` | Prises, lots et Compute Sanitizer |

`SHA256SUMS` couvre les autres fichiers.
