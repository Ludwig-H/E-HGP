# Contre-lecture de la tranche S2 (tour publique)

4 octobre 2026, 19 h 44 UTC (`date -u`). Contre-lecteur indépendant. Worktree lu : `build/v11-impl-s2`, HEAD
`1bf4be68f`, diff non commité. Je n'ai rien modifié dans ce worktree. Tous mes builds sont sous
`/tmp/v11-impl-s2-verif/`, à 3 cœurs au plus. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only (défaut de compilation ; u24 contrôlé par préprocesseur et syntaxe)
public_status=not_claimed
```

## Verdict

**Aucun constat bloquant. Aucune correction de code exigée.**

La tranche fait exactement ce que demandent le § 3.2 (points 1 à 5) et le § 9.1 (S2) de `SPECIFICATION_FINALE.md`.

- Elle ne change aucun octet produit. J'ai vérifié par une construction de référence indépendante à HEAD : les
  binaires et les dumps sont identiques à l'octet.
- Le cycle d'inclusion est cassé.
- La porte nouvelle est verte, avec un plancher exact.

Le compte rendu `impl_s2.md` est exact sur tous les points que j'ai rejoués. Il y a une seule nuance : sous charge,
une porte Python sans binaire a dépassé son délai (remarque R5).

## 1. Diff lu

| Fichier | Constat |
| --- | --- |
| `src/tower/meb.hpp` (nouveau) | `cmp` contre `git show HEAD:morsehgp3D_v11/src/tower/tower.hpp` : identique. sha256 `fd307ad49e3f0aa807d080e2467a577697db01501732bdfd7b6502dec9760380`. Conforme au point 1 (« sans modification »). |
| `src/tower/cells.hpp:4`, `locate.hpp:4`, `census_slots.hpp:3`, `meb.cpp:2` | Une ligne chacun, `"tower/tower.hpp"` devient `"tower/meb.hpp"`. Conforme au point 2. |
| `src/tower/tower.hpp:1-21` | Parapluie : inclut `full_domain.hpp`, `meb.hpp`, `forest.hpp`, puis 8 `using tower_detail::…` dans `mhgp11`. Aucune définition. Conforme aux points 3 et 4. sha256 `b7c5039b…589c`. |
| `tests/tower/public_header_test.cpp` (nouveau, 57 lignes) | N'inclut que `test.hpp` et `tower/tower.hpp`. 7 `static_assert` d'identité, égalité de pointeurs de fonction, carré K = 1..4 construit par les noms publics, `bounded_meb` sur la diagonale. |
| `tests/tower/tests.cmake:100-101` | `mhgp11_add_unit(mhgp11_tower_public_header … GROUPS umbrella LABELS fast)`. |

Rien d'autre n'est touché. Les bancs ne changent pas (point 5) : `bench/full_probe.cpp:8` et
`bench/points_export.cpp:20` incluent toujours `tower/forest.hpp`.

## 2. Contrôles rejoués par moi

### 2.1 Absence de cycle, autonomie des en-têtes

- **Cycles.** J'ai parcouru en profondeur le graphe des `#include "…"` de `src/` (97 fichiers `.hpp`/`.cpp`) : aucun
  cycle.
  - Aucun fichier de `src/` n'inclut le parapluie.
  - `tower/meb.hpp` est inclus par `locate.hpp`, `cells.hpp`, `census_slots.hpp`, `tower.hpp` et `meb.cpp`, comme
    prévu.
- **Autonomie, GCC.** `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only` sur une unité qui n'inclut que
  `tower/meb.hpp`, `tower.hpp`, `cells.hpp`, `locate.hpp`, `census_slots.hpp` ou `forest.hpp` : 12/12 réussis, en u21
  et en u24.
- **Autonomie, Clang 18.1.3.** `-fsyntax-only -Werror` sur `public_header_test.cpp` et `meb.cpp`, en u21 et u24 :
  4/4 réussis.
- **Ambiguïté de noms.** Une unité qui fait à la fois `using namespace mhgp11;` et
  `using namespace mhgp11::tower_detail;` puis nomme `FullParams` et `ForestNode` compile sans ambiguïté. La
  déclaration `using` désigne la même entité, donc les tests existants qui ouvrent les deux espaces ne cassent pas.
- **Initialisations dynamiques.** Aucune variable `inline` ou `static` à initialisation dynamique dans `tower/*.hpp`
  et `sched/*.hpp`. Les unités de test qui voient désormais `forest.hpp` par `test_support.hpp` ne gagnent donc aucun
  effet d'exécution.

### 2.2 Aucun octet changé (construction de référence indépendante)

J'ai extrait HEAD par `git archive` dans `/tmp/v11-impl-s2-verif/base`, puis construit en Release u21 `mhgp11`,
`mhgp11_full_bench` et `mhgp11_meb_bench`.

- **Préprocesseur** (`g++ -E -P`, u21 **et** u24) sur les 47 unités `src/*/*.cpp` et `bench/*.cpp`, worktree contre
  HEAD :
  - 94 sorties sur 96 sont identiques ;
  - les deux qui diffèrent sont `bench/meb_probe.cpp` en u21 et en u24. Elle passe par `bench/meb_checks.hpp:4`, qui
    voit désormais `forest.hpp` : ce sont des déclarations ajoutées, pas du code.
- **Binaires** (`cmp`) : `libmhgp11.a` identique (sha256 `e13e3ec87bdc1c6e…` des deux côtés), `mhgp11_full_bench`
  identique, `mhgp11_meb_bench` identique. Le seul banc dont le texte préprocessé change donne donc le même binaire.
- **Dumps `MHGP11FUL1`.** Deux nuages aléatoires distincts de ceux de l'implémenteur : 600 points (graine 5, boîte
  4000³) et 1500 points (graine 11, boîte 2²⁰). K = 3 et 5, 3 ouvriers, masques 0 et 16379. Résultat : 16 exécutions,
  toutes de code 0, et chaque paire avant/après a le même sha256 brut :

  | Nuage | K | sha256 brut |
  | --- | --- | --- |
  | 600 points | 3 | `54df7c9d…` |
  | 600 points | 5 | `2fba17e6…` |
  | 1500 points | 3 | `85b3fa90…` |
  | 1500 points | 5 | `3a40ce15…` |

  Le magique `MHGP11FUL1` est vérifié en tête de fichier.

### 2.3 Portes

- **Style.**
  - `python3 -S tools/check_style.py --root morsehgp3D_v11` donne `style_ok fichiers=409` (code 0).
  - `--units tower` donne `style_ok fichiers=121` (code 0).
  - Les portes CTest `mhgp11_style` et `_opt` passent.
- **Manifestes de mutants**, `run_mutants.py --check` sous `python3 -S` : tous `manifeste_ok`.

  | Module | Mutants | Plancher |
  | --- | --- | --- |
  | catalogue | 68 | 68 |
  | cloud | 16 | 16 |
  | core | 78 | 78 |
  | index | 11 | 11 |
  | num | 53 | 53 |
  | sched | 8 | 8 |
  | tower | 125 | 123 |

  - Aucun motif ne vise `src/tower/tower.hpp`.
  - Les 18 motifs de `src/tower/meb.cpp` ne portent pas sur la ligne d'inclusion modifiée.
  - Les 14 portes CTest `mhgp11_mutants_*_manifest` et leurs jumelles passent.
- **Nouvelle porte.** `mhgp11_tower_public_header umbrella` affiche `test umbrella controles=19 echecs=0 plancher=19`
  (code 0), et `--inventaire umbrella` donne `inventaire_ok tests=1`. Le plancher 19 est **exact** :
  - 5 contrôles hors boucle ;
  - 3 contrôles × 4 ordres ;
  - 2 contrôles MEB.

  Une boucle tronquée ou un `REQUIRE` sauté descend donc sous le plancher.
- **Suite rapide entière** (`ctest -L fast -j 3 --timeout 600`, codespace partagé avec un autre agent, charge 6,6) :
  - **699 passées sur 703**, 1 sautée (`mhgp11_support_lidar_sentinel`, pas de `MHGP11_DATA_DIR`) ;
  - 3 échecs, tous étrangers à la tranche :
    - `mhgp11_tower_full_paired_protocol` et `_opt` : `FileNotFoundError …/receipts/qualification_performance_20261003/baseline_source_manifest.json`.
      Le checkout partiel exclut `receipts/`. Rejoué sur une **copie** du worktree complétée par les `receipts/` de
      HEAD (`git archive`), sans rien poser dans le worktree, en normal et sous `-O` :
      `full_paired_protocol_verdict conforme checks72 native0`.
    - `mhgp11_tower_full_campaign` : **Timeout à 120 s** (`tests/tower/tests.cmake:87-89`). Rejoué seul, il passe en
      82 s. Sa jumelle `_opt` est passée dans la suite en 108,45 s. C'est une porte Python `native0`, sans aucun
      binaire, donc insensible à la tranche. L'implémenteur l'a vue passer : sa charge était plus faible.
  - Portes d'identité nommées par la spécification, toutes passées avec leurs jumelles :
    - `mhgp11_tower_full_bench_io`, ligne exacte `full_io_verdict conforme attempts427 successes413 refusals14` ;
    - `mhgp11_tower_full_leaf_lanes` ;
    - `mhgp11_tower_fraction` ;
    - `mhgp11_tower_forest_fraction`.
- **Pas joués** (hors codespace) : la matrice G4 (GCC Release u21/u24, ASan/UBSan, TSan), la campagne de mutants
  `tower` et le reçu immuable.
  - La campagne de mutants n'apporte rien ici : la bibliothèque et les bancs sont identiques à l'octet, et aucun motif
    ne change. Un mutant de `meb.cpp` s'applique au même texte et produit le même binaire qu'à HEAD.
  - La matrice G4 reste due à la clôture formelle de la tranche (§ 8.8).

### 2.4 Contrats (Outcome, budget, raisons, transactions)

La tranche ne touche ni l'émission des `Outcome`, ni le budget, ni `reasons.def`, ni aucune transaction. Le texte
préprocessé de toutes les unités produit est identique.

Le test nouveau suit les conventions :
- `MemoryBudget` explicite (`kUnlimited`) ;
- `REQUIRE(…ok())` avant tout `value()`, donc pas d'accès vérifié sur un refus.

## 3. Constats

### Bloquant

Aucun.

### À corriger

Aucun dans le code de la tranche. Deux points sont à régler au moment du commit et du reçu, sans toucher au code.

- **C1, nom de la porte.** `tests/tower/tests.cmake:101` enregistre, par `GROUPS`, deux portes :
  `mhgp11_tower_public_header_umbrella` et `mhgp11_tower_public_header_inventaire` (`cmake/gates.cmake:279-289`). La
  spécification (§ 9.1 S2) et `slices_finales.json` parlent de `mhgp11_tower_public_header`, qui n'existe pas comme
  test CTest. Le reçu S2 et la matrice G4 doivent citer les deux noms réels, sinon un `-R '^mhgp11_tower_public_header$'`
  ne sélectionne rien et le vert serait par vacuité. Il vaut mieux corriger le texte de la spécification ou du reçu que
  le code : `GROUPS` apporte la porte d'inventaire.
- **C2, provenance.** Ajouter dans `docs/PROVENANCE.md` (S0, ou le commit de S2) que `src/tower/meb.hpp`
  (sha256 `fd307ad4…0380`) est le contenu de `src/tower/tower.hpp` à `1bf4be68f`. Les épingles historiques, par
  exemple `src/tower/source_pins.json:110` (`meb_deferred_revision`, base `30aa39c2`) et
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`, nomment `tower.hpp` pour le contenu MEB.
  Sans cette note, un auditeur qui suit la MEB par son chemin perd la trace.
  - Aucun outil ne consomme `source_pins.json` (grep vide), donc rien ne casse.
  - La note est de traçabilité.

### Remarques

- **R1.** `src/tower/meb.hpp:1` garde le commentaire « Premier raccord de tower : … ». C'était juste pour l'ancien
  `tower.hpp`, c'est un peu daté pour `meb.hpp`. La spécification exige une copie « sans modification » et ce
  commentaire ne change aucun octet produit. On peut le reformuler plus tard, dans un commit séparé, en mettant à jour
  la note C2.
- **R2. Surface publique nominale.** Le parapluie inclut `forest.hpp`, qui tire transitivement tout `tower_detail` :
  `descent_memo`, `descent`, `cells` et `sched`. Le mot « public » est une convention de nommage, pas une
  encapsulation.
  - `ForestLedger` contient `ClassificationLedger`, `CellLedger` et `DescentLedger`, qui ne sont pas réexportés. Un
    client qui voudrait nommer ces sous-registres devrait écrire `tower_detail::`.
  - `FullParams` expose une douzaine de leviers d'optimisation, dont `concurrent_orders`, `descent_lanes` et
    `population_lookup`.

  C'est cohérent avec la décision 12 (« aucun code ne quitte `tower_detail` »). L'esprit critique demandé par
  l'utilisateur invite pourtant à relever que la vraie frontière publique sera la `Session` de S5. C'est elle, et non
  `tower.hpp`, qui devra fixer les leviers et ne publier que les produits. Je ne recommande pas de façade
  supplémentaire en S2 : elle changerait du code et contredirait « aucun octet ».
- **R3. `ForestLedger` ajouté** (écart 1 de l'implémenteur). Je le juge justifié : c'est le type de retour de
  `OrderForest::ledger()`, et le test le nomme. Ne pas exposer `build_forest` (écart 2) est aussi justifié, car sa
  signature porte `DescentMemo*` et `ForestParallel*`.
- **R4. Coût de compilation.** Les 37 unités de test qui passent par `tests/tower/test_support.hpp:8`, ainsi que
  `bench/meb_checks.hpp:4`, compilent maintenant `forest.hpp`. Les binaires sont identiques (vérifié pour
  `mhgp11_meb_bench`), mais le temps de compilation augmente. On pourrait les passer à `tower/meb.hpp` dans une tranche
  ultérieure, si le temps de construction sur G4 compte.
- **R5. Marge de `mhgp11_tower_full_campaign`.** `tests/tower/tests.cmake:89` (`TIMEOUT 120`) : 82 s seule, 108 s
  sous charge pour la jumelle, et un dépassement observé à `-j 3` sur un codespace partagé. Ce n'est pas lié à S2,
  mais c'est un faux rouge probable lors des contre-lectures parallèles des tranches suivantes. À signaler aux
  auditeurs, sans changer ce délai dans S2.
- **R6. Fichier ignoré dans le worktree.** `morsehgp3D_v11/tests/support/__pycache__/` est présent et ignoré par Git
  (`git status --ignored`). C'est sans effet, mais il faudrait le nettoyer avant tout `git add` large. La mémoire du
  projet interdit de toute façon `git add -A`.
- **R7.** Aucune réponse de l'utilisateur n'est en jeu dans S2 : c'est un pur déplacement de déclarations. Je ne vois
  aucune raison de revenir sur un arbitrage.

## 4. Artefacts de la vérification

- Build du worktree : `/tmp/v11-impl-s2-verif/b` (journal de la suite rapide : `/tmp/v11-impl-s2-verif/fast.log`).
- Référence HEAD : sources dans `/tmp/v11-impl-s2-verif/base`, build dans `/tmp/v11-impl-s2-verif/bb`.
- Dumps et entrées : `/tmp/v11-impl-s2-verif/dumps/`.
- Copie du worktree avec ses reçus, pour la porte du protocole apparié : `/tmp/v11-impl-s2-verif/wtc`.
