# Tranche S2 — tour publique : compte rendu d'implémentation

4 octobre 2026, 19 h 29 UTC (`date -u`). Worktree `build/v11-impl-s2` (checkout partiel, HEAD `1bf4be68f`).
Rien n'est commité ni poussé. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only (défaut de compilation)
public_status=not_claimed
```

## 1. Fichiers changés

| Fichier | Changement | Raison |
| --- | --- | --- |
| `src/tower/meb.hpp` (nouveau) | Copie **octet pour octet** de l'ancien `tower.hpp` (`cmp` contre `git show HEAD:…/tower.hpp` : identique ; sha256 `fd307ad4…0380`). | § 3.2, point 1 : `kMaxMebSites`, `MebLedger`, `BoundedMeb`, `bounded_meb`, `MebCensus`, `meb_census`. L'en-tête de commentaire reste exact pour ce fichier (« MEB exacte… aucune forêt FULL construite ici »). |
| `src/tower/cells.hpp:4`, `locate.hpp:4`, `census_slots.hpp:3`, `meb.cpp:2` | `"tower/tower.hpp"` → `"tower/meb.hpp"` (une ligne chacun) | § 3.2, point 2 : casse le cycle `tower.hpp → forest.hpp → descent_memo.hpp → descent.hpp → cells.hpp → tower.hpp`. |
| `src/tower/tower.hpp` | Devient le parapluie public : inclut `full_domain.hpp`, `meb.hpp`, `forest.hpp` ; `using tower_detail::{ForestNode, ForestLedger, OrderForest, FullTower, FullParams, FullTimings, OrderTimings, build_full}` dans `namespace mhgp11`. | § 3.2, point 3. Aucun code ne quitte `tower_detail` (point 4). |
| `tests/tower/public_header_test.cpp` (nouveau) | Unité `umbrella` (plancher 19) : n'inclut que `"tower/tower.hpp"` (et `test.hpp`) ; `static_assert` d'identité des sept types publics avec ceux de `tower_detail`, `&mhgp11::build_full == &tower_detail::build_full`, carré à K = 1..4 construit par les seuls noms publics (`FullTower`, `OrderForest`, `ForestNode`, `ForestLedger`, `FullParams`, `FullTimings`), plus `bounded_meb` sur la diagonale. | Porte prévue par `slices_finales.json` (S2) : « une unité hors module incluant `tower/tower.hpp` voit `FullTower` et `build_full` ». |
| `tests/tower/tests.cmake` | `mhgp11_add_unit(mhgp11_tower_public_header SOURCES public_header_test.cpp GROUPS umbrella LABELS fast)` après `mhgp11_tower_memo_fault`. | Enregistrement par l'aide permise (`cmake/gates.cmake`). |

Non modifiés, volontairement : les bancs (`bench/full_probe.cpp:8`, `bench/points_export.cpp:20` incluent toujours
`tower/forest.hpp`), `tests/tower/test_support.hpp` et `bench/meb_checks.hpp` (ils incluent `tower/tower.hpp`, qui reste
valable puisque le parapluie inclut `meb.hpp`), `src/tower/source_pins.json` (l'entrée `meb_deferred_revision`
épingle `src/tower/tower.hpp` **au commit de base `30aa39c2`** : c'est une provenance historique, juste telle
quelle), `docs/FULL_DOMAIN.md:14` (« exposée par tower.hpp » reste vrai), les manifestes de mutants (aucun motif ne
vise `tower.hpp` ni les lignes d'inclusion modifiées).

## 2. Absence de cycle et règle `[inclusion]`

- Graphe des inclusions de `src/` (98 fichiers) parcouru en profondeur : **aucun cycle** ; aucun fichier interne de
  `src/` n'inclut plus `tower/tower.hpp` (liste vide).
- Autonomie des en-têtes : `g++ -fsyntax-only -std=c++20 -Wall -Wextra -Wpedantic -Werror` sur une unité qui n'inclut
  que `tower/meb.hpp`, `tower/tower.hpp`, `tower/forest.hpp`, `tower/cells.hpp`, `tower/locate.hpp`,
  `tower/census_slots.hpp` : six succès.
- Règle `[inclusion]` : depuis un autre module, `"tower/tower.hpp"` est désormais le seul en-tête nécessaire pour
  atteindre FULL ; la porte `mhgp11_tower_public_header_umbrella` le prouve à la compilation.

## 3. Aucun octet de sortie changé

- **Préprocesseur** : `g++ -E -P` de chaque unité de `src/*/*.cpp`, `bench/*.cpp` et `tests/tower/*.cpp`, à HEAD
  (`git archive`) et après la tranche : toutes les unités de `src/` et des bancs produits (`full_probe.cpp`,
  `points_export.cpp`, …) sont **identiques**. Diffèrent seulement les unités de test qui passent par
  `test_support.hpp` et `bench/meb_probe.cpp` (via `meb_checks.hpp`) : elles voient en plus les déclarations du
  parapluie, sans aucun changement de code produit (37 unités sur 89).
- **Binaires** : `libmhgp11.a` et `mhgp11_full_bench` construits à HEAD et après la tranche (Release, u21) sont
  **identiques à l'octet** (`cmp`).
- **Dumps `MHGP11FUL1`** : 400 points aléatoires (graine 3), K = 1, 3, 5, en W1 masque 0 et W3 masque 16 379 : six
  dumps de même sha256 avant et après (`4583256c…`, `52c2f58b…`, `89c1d49a…`).

## 4. Portes jouées (build local `/tmp/v11-impl-s2/b21`, Release, u21, `-j 3`)

- Construction complète `-Werror` : succès.
- `python3 -S tools/check_style.py --root morsehgp3D_v11` : `style_ok fichiers=409` ; `--units tower` :
  `style_ok fichiers=121`.
- Manifestes de mutants, `run_mutants.py --check` sous `python3 -S`, tous `manifeste_ok` : catalogue 68/68, cloud
  16/16, core 78/78, index 11/11, num 53/53, sched 8/8, **tower 125 (plancher 123)**.
- Suite rapide entière `ctest -L fast -j 3 --timeout 600` : **701 passées sur 703**, une sautée
  (`mhgp11_support_lidar_sentinel`, pas de `MHGP11_DATA_DIR`, attendu).
  - Les deux échecs, `mhgp11_tower_full_paired_protocol` et sa jumelle `_opt`, viennent du checkout partiel :
    `receipts/` est exclu par le sparse-checkout du worktree (`FileNotFoundError …/receipts/qualification_performance_20261003/baseline_source_manifest.json`).
    Rejouées avec un lien temporaire `receipts → /workspaces/E-HGP/morsehgp3D_v11/receipts` (retiré aussitôt) :
    **2/2 passées** (`native0`, aucun binaire en jeu).
  - Portes d'identité nommées : `mhgp11_tower_full_bench_io` et `_opt` (ligne exacte inchangée), 
    `mhgp11_tower_full_leaf_lanes` et `_opt`, `mhgp11_tower_fraction`, `mhgp11_tower_forest_fraction` et `_opt`, toutes
    les `mhgp11_tower_unit_*`, `mhgp11_style` et `_opt`, les sept `mhgp11_mutants_*_manifest` et leurs jumelles :
    passées.
  - Nouvelle porte : `mhgp11_tower_public_header_umbrella` et `mhgp11_tower_public_header_inventaire` : passées.
- `python tools/check_docs.py` (racine) : deux liens manquants **hors v11**, dus au checkout partiel
  (`docs/validation/PHASE4_PROGRESS.md:38`, `PHASE7_PRIMITIVE_AUDIT.md:54`) ; aucun document touché par la tranche.

Non joué ici (hors codespace, § 8.1 et § 8.8) : matrice G4 (GCC Release u21/u24, ASan/UBSan, TSan), campagne de
mutants, reçu immuable.

## 5. Écarts à la spécification

1. **`ForestLedger` ajouté** aux `using` (absent de la liste du § 3.2). C'est le type rendu par
   `OrderForest::ledger()` ; I4 et I10 (S3, S7) comparent ces registres, et un module client doit pouvoir le nommer
   sans `tower_detail`. Ajout purement nominal, retirable sans effet.
2. **`build_forest` non exposé** (« si utile ») : sa signature porte `DescentMemo*` et `ForestParallel*`, deux types
   internes ; l'exposer publierait de fait la mécanique interne. `build_order` (S3) est le point d'entrée public
   prévu pour un ordre seul.
3. **`full_domain.hpp` inclus explicitement** par le parapluie (il l'était déjà transitivement via `meb.hpp`) : le
   parapluie dit ainsi lui-même qu'il expose `FullDomain` et `prepare_full_domain` (`docs/FULL_DOMAIN.md:14`).

Aucun conflit avec les réponses de l'utilisateur : la tranche est un pur déplacement de déclarations.

## 6. Questions et remarques pour la suite

- S0 pourra mentionner dans `docs/PROVENANCE.md` que `meb.hpp` (sha256 `fd307ad4…`) est le contenu de `tower.hpp` à
  `1bf4be68f`, pour que la provenance MEB suive le fichier.
- S3 ajoutera `#include "tower/order_tree.hpp"` et les `using` `OrderTree`, `WindowAttachment`, `BallRole`,
  `build_order` au parapluie, et pourra étendre `public_header_test.cpp`.
- Les tests qui incluent `tower/tower.hpp` (`test_support.hpp`) compilent désormais aussi `forest.hpp` : coût de
  compilation seulement, aucun effet de code. On pourrait les passer à `tower/meb.hpp` si le temps de compilation
  compte ; je ne l'ai pas fait pour garder la tranche minimale.
