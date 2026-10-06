# Levier B : hybride CPU/GPU du lot de feuilles (6 octobre 2026)

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé. Aucune branche, aucun commit, aucun push. Worktree `/tmp/v11-wf-B` (base `905fad2e1`), construction
`/tmp/v11-wf-B-build` (Release, u21). Patch : `/workspaces/E-HGP/build/v11-persist/wf_gpu/B.patch`.

## Verdict : levier abandonné

Le patch est exact (dumps et registres identiques sur toutes les voies, portes vertes, sept mutants tués, SASS des
noyaux inchangé), mais le levier perd du temps. La seule prédiction utile du travail d'une feuille exige la dominance en
O(m²) de `prepare`. Elle coûte entre 2 et 9 µs par feuille sur l'hôte, soit 13 à 17 % d'une passe de comptage complète
à K5/16 et environ 8 % à K10/24. Ce coût s'ajoute au parcours, qui est sur le chemin critique puisque le GPU attend la
fin du parcours. Garder les feuilles lourdes sur le CPU ne raccourcit presque pas le noyau : la distribution du travail
est lisse, et avec le tri par m les feuilles lourdes partent déjà en tête du noyau. Bilan estimé sur G4 : une perte
nette de 5 à 10 ms à K5/16 et de 40 à 90 ms à K10/24 pour le routage seul. Le gain que suggère le modèle vient de
l'ordre des fils par travail prédit (bit 524288), pas du routage, et le coût du prédicteur sur l'hôte l'annule (bilan à
peu près nul). Les chiffres suivent.

## Conception

Mise en file : `enumerate_leaf` (`src/catalogue/leaf.cpp`) appelle `run.deferred->push` (`TaskLeafQueue`,
`src/catalogue/leaf_queue.hpp`) pour toute feuille admissible (graphe de paires, 1 <= m <= 32). La voie CPU 16379
traite la feuille sur place par `device_leaf` : `run_leaf` avec `CountSink` pour la sonde, puis `run_leaf` avec
`AcceptSink`, qui calcule le Level par `Sphere::through`. Une feuille non résolue passe par `leaf.cpp`.

Ce que fait le patch :

1. `LeafQueue::push` rend `Result<bool>` : vrai si la feuille est en file, faux si elle est gardée.
   `TaskLeafQueue::bind(budget, cloud, params)` ;
   - si `order_by_work`, ou si `hybrid_work > 0` et que le majorant C(m+1,3) = C(m,2) + C(m,3) atteint le seuil, push
     calcule `predicted_work` ;
   - si `hybrid_work > 0` et que le travail prédit atteint le seuil, push rend faux et compte la feuille (`kept`).
2. `enumerate_leaf` : une feuille gardée est jouée sur place par `device_leaf`, même si `device_leaf` est faux, donc
   exactement comme la voie CPU ; une feuille non résolue passe par `leaf.cpp`.
3. `predicted_work` (`leaf_batch.hpp`, hôte) réutilise `Leaf<CountSink>::prepare` et `live_rows` de la source unique,
   sans dupliquer la formule. Le travail prédit compte les couples vivants (live[0], i < j) et les triplets vivants
   (k > j dans live[1][i] et dans live[1][j]). Il est borné par C(32,2) + C(32,3) < 2^13. C'est une heuristique
   d'ordonnancement : aucune décision n'en dépend.
4. `LeafJob::pad` devient `LeafJob::work`, même disposition.
   - `leaf_order` (`leaf_batch.cpp`, hôte, partagé avec CUDA) remplace `size_order` du `.cu`. C'est un tri par
     comptage stable et décroissant, par m (comportement inchangé) ou par travail prédit borné à 4095. Il contrôle
     ensuite que le résultat est une permutation, et refuse sinon (`catalogue_invariant`). Ses tampons sont admis au
     budget avant l'allocation.
   - Avec `order_by_work`, l'exécuteur hôte suit cet ordre au comptage, ce qui fait jouer la permutation par la porte.
5. Paramètres `CatalogueParams::hybrid_work` (u32, 0 : coupé) et `order_by_work`. Diagnostic
   `CatalogueTimings::batch_hybrid_jobs`, imprimé dans `leaf_batch.hybrid_jobs`.
6. Sonde `bench/full_probe.cpp` :
   - bit 262144 : voie hybride ; le seuil est le 13e argument (1..65535, après `passes`), 100 K par défaut ;
   - bit 524288 : ordre des fils par travail prédit ;
   - borne de parse relevée à 1048575 ;
   - les deux bits exigent 32768 ou 65536 ;
   - champ `hybrid_work` ajouté à la ligne `full`.

## Preuve d'exactitude

- Chaque feuille admissible est traitée une seule fois : soit en file (le lot la traite, `leaf.cpp` la refait si elle
  n'est pas résolue), soit gardée (`device_leaf`, puis `leaf.cpp` si elle n'est pas résolue). C'est le même partage que
  dans la voie CPU 16379, feuille par feuille.
- Les émissions sont identiques dans les deux cas : mêmes `Ball` de `leaf_device`, Level par `Sphere::through` de même
  arité (`AcceptSink` côté hôte, `Materialize` côté lot).
- Le registre est identique : `device_leaf` verse les `Counts` de la feuille comme `ledger_of` pour une feuille résolue
  du lot, et `leaf.cpp` donne les mêmes compteurs dans les deux voies.
- Le catalogue final est trié canoniquement : la place d'une émission (sortie de tâche ou bloc du lot) ne change pas
  le dump.
- Les sorties sont déterministes. Le prédicteur est une fonction pure de (sites, boîte, K) : le routage ne dépend pas
  du nombre de fils. L'ordre est un tri stable. Le compteur `kept` est une somme par file.
- Mémoire : aucune allocation nouvelle dans push (le `Leaf` du prédicteur, environ 3,7 Kio, est sur la pile). Les
  feuilles gardées utilisent le Workspace de la tâche, déjà admis. Les deux tampons de `leaf_order` sont admis avant
  l'allocation. Une permutation incomplète est refusée explicitement.

Vérifié sur ng00 : dump K5/16 `3a2bfb4f9f48...` et K10/24 `61a4245b91d9...`, registre `catalogue_work` identique à
16379 (empreinte JSON trié `d285bab7f4` à K5, `6ad687c887` à K10). Voies : 49147, 311291 (hybride, seuil par défaut),
835579 (hybride et ordre), 180219, 442363 (hybride sans réservoir, seuils 400 et 1000), à 2 fils.

## Portes jouées

| Porte | Résultat |
|---|---|
| `tests/tower/full_leaf_lanes.py`, étendue (lot_hybride K5 seuil 300, lot_hybride_ordre K10 seuil 900 ; gardées + lot = lot complet) | `full_leaf_lanes_verdict conforme sites3000 k5 boules212695 lot27048 non_resolues0 rejouees0 copiees23766 gardees2990 k10 boules1075008 lot35957 non_resolues0 rejouees0 copiees35131 gardees1834 rejouees_sans_reservoir555` (LINE regravée) |
| `ctest -R "full_leaf_lanes\|catalogue_(cache\|single_pass\|leaf\|small_pair\|parallel)\|full_bench_io"` (sources finales) | `100% tests passed, 0 tests failed out of 34` |
| `tests/tower/full_bench_io.py` | rouge AVANT le patch (base 905fad2e1 : `opt_large` = 131072 est valide depuis le bit replay_overflow) ; corrigé en 1048576, le premier masque invalide : `full_io_verdict conforme attempts427 successes413 refusals14` |
| `tools/check_style.py` | `style_ok fichiers=531` |
| `run_mutants.py --check` | `manifeste_ok module=tower mutants=155 plancher=155` |

## Mutants (porte `mhgp11_tower_full_leaf_lanes`)

| id | effet | verdict |
|---|---|---|
| hybride_feuille_perdue | la feuille gardée n'est pas traitée | TUÉ |
| hybride_double_traitement | la feuille est en file et aussi traitée sur l'hôte | TUÉ |
| hybride_seuil_inverse | routage inversé | TUÉ |
| hybride_gardee_aussi_en_file | la feuille est comptée comme gardée mais mise en file | TUÉ |
| hybride_majorant_faux | majorant C(m,2) | TUÉ |
| predicteur_triplets_faux | triplets comptés sans le masque de j | TUÉ (ligne) |
| ordre_travail_incomplet | `leaf_order` ne produit pas une permutation | TUÉ (refus) |

Première campagne : 6 tués sur 7. La septième version (`predicteur_sans_triplets`) ne se construisait pas (variable
inutilisée, -Werror). Elle est remplacée par `predicteur_triplets_faux`, rejoué seul : `mutants_ok module=tower
mutants=1 tues=1 plancher=1`. Bilan : 7 mutants sur 7 tués. Le plancher du manifeste passe de 148 à 155.

Limite : la propagation de `view.order_by_work` jusqu'à l'exécuteur CUDA n'est observable que sur GPU (l'ordre ne
change pas les sorties). Localement, seule la permutation est gardée, par le refus et le mutant ci-dessus.

## SASS avant/après (sm_120, Release, u21)

| Noyau | Base 905fad2e1 | Patch B |
|---|---|---|
| count | 11832 instructions, 134 BSSY, 3 CALL, 164 registres, pile 4144 o, LDL 358, STL 311 | identique |
| fill | 9920 instructions, 93 BSSY, 0 CALL, 168 registres, pile 4032 o, LDL 326, STL 289 | identique |

`cuobjdump -sass` avant/après : diff limité aux noms mutilés de l'espace anonyme (hash de l'unité de compilation).
`leaf_device.hpp` n'est pas touché ; seul le code hôte du `.cu` change (`leaf_order`).

## Distribution et routage (ng00, trace temporaire hors dépôt)

Travail = préfixes + tests de recensement réels de la feuille. Prédicteur exact du patch (égalité vérifiée sur
353 456 et 530 259 feuilles par un micro-banc).

Rang de Spearman contre le travail, K5/16 :

| m | nbr | couples vivants | triplets vivants | couples + triplets (retenu) | m × (couples + triplets) |
|---|---|---|---|---|---|
| 0,667 | 0,822 | 0,916 | 0,906 | 0,928 | 0,933 |

Les traits géométriques bon marché (sites dans la boîte, côté, étendue, distance au centre) restent tous sous m (0,21
à 0,61).

Par m, K5/16 : les m = 16 représentent 22 % des feuilles et 36 % du travail. Les 20 feuilles les plus lourdes se placent
entre 1 % et 44 % du lot trié par m : elles partent dans la première moitié du noyau.

K5/16 (353 456 feuilles, travail 157,5 M, maximum 6 015) :

| Seuil | Gardées | Part des feuilles | Part du travail | Travail maximal restant sur le GPU |
|---|---|---|---|---|
| 300 | 25 480 | 7,21 % | 20,73 % | 2 972 |
| 400 | 5 193 | 1,47 % | 5,86 % | 4 049 |
| 500 (défaut 100 K) | 719 | 0,20 % | 1,14 % | 4 414 |
| 600 | 43 | 0,01 % | 0,09 % | 5 846 |

K10/24 (530 259 feuilles, travail 732,3 M, maximum 27 020) :

| Seuil | Gardées | Part des feuilles | Part du travail | Travail maximal restant sur le GPU |
|---|---|---|---|---|
| 800 | 32 976 | 6,22 % | 20,34 % | 8 635 |
| 1000 (défaut) | 9 026 | 1,70 % | 7,53 % | 15 920 |
| 1200 | 2 215 | 0,42 % | 2,42 % | 15 920 |
| 1500 | 239 | 0,05 % | 0,39 % | 21 636 |

Le nombre de feuilles gardées par le moteur vaut exactement ces chiffres : 719 et 5 193 à K5, 9 026 et 32 976 à K10.
La queue diminue lentement : il faut garder 7 % des feuilles et 21 % du travail pour diviser par deux la feuille
restante la plus lourde.

## Mesures hôte appariées (codespace partagé, charge entre 11 et 16 sur 8 cœurs : descriptives seulement)

La sonde est à un fil, ng00. Le parcours est `single_pass_ns` ; R est le comptage du lot hôte (`count_ns`, une passe
`run_leaf` sur tout le lot).

| Voie | K5/16 parcours (3 rép.) | Δ parcours / R |
|---|---|---|
| 49147 | 1 921 ; 2 697 ; 2 631 ms (R 8 249 ; 8 988 ; 9 630) | 0 |
| 311291 (500) | 3 275 ; 3 230 ; 3 219 | +6 à +16 % |
| 311291 seuil 400 | 4 520 ; 5 177 ; 5 142 | +26 à +32 % |
| 311291 seuil 300 | 8 225 ; 8 274 ; 8 147 | +57 à +76 % |
| 573435 (ordre seul : prédicteur sur toutes les feuilles) | 4 767 ; 4 094 ; 3 984 | +14 à +33 % |

K10/24, 1 répétition :

| Voie | Parcours | Δ parcours / R |
|---|---|---|
| 49147 | 5 935 ms (R 46 646 ms) | 0 |
| 311291 (1000) | 14 419 ms | +18 % |
| 311291 seuil 800 | 37 907 ms | +69 % |
| 573435 (ordre seul) | 11 744 ms | +12 % |

Micro-banc mono-fil (mêmes feuilles, même processus, sous charge) :

| Configuration | Prédicteur | Feuilles gardées (2 × `run_leaf`) | R (1 × `run_leaf`, tout le lot) |
|---|---|---|---|
| K5/16 | 2,3 à 5,5 µs par feuille, soit 13 à 17 % de R ; dominance seule 0,6 à 2,4 µs | seuil 500 : 4,4 % de R | 30 µs par feuille sous charge |
| K10/24 | 8,7 à 9,2 µs par feuille, soit 8 % de R | seuil 1000 : 16 % de R | — |

Une version sans branche du prédicteur gagne seulement 10 à 20 %. Le majorant C(m+1,3) épargne le prédicteur à 45 %
des couples à K5 (seuil 500), mais à seulement 5 % à K10 (seuil 1000).

À un fil, l'exécuteur hôte avec l'ordre par travail compte plus lentement (count 10,1 et 11,0 s contre 8,2 et 9,0 s) :
la localité spatiale du lot est perdue.

## Effet attendu sur G4 : ESTIMATION

Hypothèses :

- R sur G4 (48 fils) se déduit des écarts G4 entre les voies CPU et GPU : environ 45 ms à K5/16 et environ 445 ms à
  K10/24. La voie CPU coûte environ 1,1 R sur l'hôte.
- Modèle d'ordonnancement du `count_kernel` : warps de 32 feuilles dans l'ordre des fils, 188 SM × 12 warps (164
  registres), durée d'un warp = a (max + β (somme − max)). a = 2,16 µs par unité, calé sur « une feuille lourde ≈ 13
  ms » ; β = 0,15 à 0,20, calé sur le noyau mesuré (39 ms à K5, 226 ms à K10). C'est un modèle grossier, non validé.

Noyau du modèle, en ms (base : 39 à K5/16, 226 à K10/24) :

| | Tri par m | Tri par travail prédit |
|---|---|---|
| K5, seuil 500 | 38,9 | 31,3 |
| K5, seuil 400 | 36,1 | 29,6 |
| K5, sans routage | 39,0 | 48,0 (warp de tête trop lourd) |
| K10, seuil 1000 | 203,7 | 162,8 |
| K10, seuil 1200 | 214,7 | 171,3 |
| K10, seuil 1500 | 225,2 | 183,0 |
| K10, sans routage | 226 | 250 |

Coût sur le parcours, en ms (prédicteur et feuilles gardées, appliqués à R) :

| Configuration | Coût du parcours |
|---|---|
| K5, seuil 500 | +4 + 2, soit environ +6 |
| K5, seuil 400 | environ +13 |
| K5, ordre (prédicteur sur tout le lot) | environ +7 de plus |
| K10, seuil 1000 | +34 + 78, soit environ +112 |
| K10, seuil 1200 | environ +53 |
| K10, seuil 1500 avec ordre | environ +40 |

Bilan net estimé :

- hybride seul :
  - K5/16 : +5 à +10 ms (perte) ;
  - K10/24 : +40 à +90 ms (perte).
- hybride avec ordre :
  - K5/16 : environ +1 ms ;
  - K10/24 : entre −3 et +50 ms.
  - Ce sont les meilleurs cas, et ils dépendent entièrement du modèle.

Le seul effet favorable du modèle est l'ordre par travail. Un ordre qui ne coûterait rien au parcours (prédiction sur
le GPU, ou ordre appris d'une passe précédente) serait un autre levier.

## Masques et paramètres de sonde pour G4 (si une falsification est souhaitée)

`mhgp11_full_bench XYZ IDS DUMP K F 256 0 4294967295 18446744073709551615 48 MASQUE [PASSES [SEUIL]]`

| Taille | Masques |
|---|---|
| K5/16 | 81915 (base) ; 344059 (81915 + 262144, seuil 500 par défaut) ; 606203 (81915 + 524288, ordre seul) ; 868347 (hybride avec ordre), avec un seuil de 500 puis 400 en 13e argument |
| K10/24 | les mêmes masques, avec des seuils de 1000, 1200 et 1500 |

À comparer : `leaf_batch.count_ns`, `single_pass_ns`, `domain_ns`, `hybrid_jobs`. Les dumps doivent rester
`3a2bfb4f9f48` et `61a4245b91d9`, et le registre doit être égal à 81915.

## Risques

- L'estimation G4 repose sur un modèle d'ordonnancement calé sur deux nombres. L'hypothèse « une feuille lourde
  ≈ 13 ms » n'est pas cohérente avec un modèle par warp purement « max ».
- L'ordre par travail détruit la localité spatiale du lot : accès aux coordonnées plus dispersés, comptage hôte plus
  lent de 20 % à un fil. Il n'est pas mesuré sur GPU.
- Le coût du prédicteur dépend fortement de la machine (prédiction de branchement de la dominance à trois issues).
- Les mesures locales ont été prises sous une charge de 11 à 16 sur 8 cœurs : ce sont des rapports, pas des temps.
- `full_bench_io` était rouge à la base (indépendamment de B) ; le patch le corrige en passant par la nouvelle borne.
