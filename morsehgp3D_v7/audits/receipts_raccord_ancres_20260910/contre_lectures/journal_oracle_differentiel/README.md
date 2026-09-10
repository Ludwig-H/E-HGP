# Oracle différentiel indépendant du journal FULL daté (`full_dated_coverage_forest_v2`)

Sous-audit du commit `1fbe49d3` (`morsehgp3D_v7/src/forest/full_coverage_certificate.hpp`).
Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé. Aucun fichier écrit dans le dépôt :
tout est dans ce répertoire de scratch.

## Pièces

| Fichier | Rôle |
| --- | --- |
| `bridge.cpp` | Pont C++ : lit un journal JSON minimal, appelle `build_full_coverage_populations`, `build_full_coverage_certificate`, `full_coverage_root_at`, `full_coverage_at` du header PRODUIT (inclus tel quel), imprime pour chaque nœud (plus deux sondes hors domaine : `N` et `kFullCoverageAbsent`) et chaque coupe (niveaux des lots + coupes extra, ouverte et fermée) le statut, la racine et la couverture triée. Code 2 si le journal est irreprésentable (JSON, U192/i128/u64/u16). |
| `journal_model.py` | Modèle Python indépendant de la sémantique du contrat de l'auditeur (LOCAL_DIAGNOSTICS §1) : validation par ensemble de codes de refus, racine historique à la coupe, union ensembliste par racine sans dédoublonnage entre racines. Aucun `assert`. |
| `generator.py` | Générateur seedé de journaux valides (profils `random`, `chain` (successeurs ≥ 3), `overlap`, `k1`, `wide_mask` (16 bits)) et de journaux invalides par mutation unique (43 mutations, 12 raisons produit distinctes attendues). |
| `geometry_journals.py` | Journaux dérivés d'une géométrie réelle via les modèles de l'auditeur (`plateau_model.py`, `coverage_contribution_model.py`, lecture seule) : 7 nuages (ABCZ, carré, triangle rectangle, pont extérieur, ABCZXY, coquille 7, tétraèdre+origine), tous les ordres K=1..n, variantes `gap` et `whole`, attentes = couvertures Gamma (multi-ensemble) et lecture par token du modèle. |
| `run_oracle.py` | Orchestrateur : génère, exécute le pont, compare, imprime un rapport JSON déterministe. Code 0 sans divergence. |
| `run_mutants.sh` | Non-vacuité : ponts compilés contre huit headers mutés (3 du développeur, 1 de l'auditeur historique, 4 propres). |
| `report_normal.json` / `report_optimized.json` | Rapports `python3 -B` et `python3 -B -O` (octets identiques, cf. `sha256sums.txt`). |
| `run_normal/`, `run_optimized/` | `corpus/*.json` (journaux) et `bridge_out/*.json` (sorties produit). |
| `mut_<nom>/report.json` | Rapport de l'oracle contre chaque mutant. |

## Reproduction

```bash
S=<ce répertoire>
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I/workspaces/E-HGP/morsehgp3D_v7 $S/bridge.cpp -o $S/build/bridge
cd $S && PYTHONDONTWRITEBYTECODE=1 python3 -B    run_oracle.py --bridge build/bridge --out run_normal    --valid 320 --invalid 320 > report_normal.json
cd $S && PYTHONDONTWRITEBYTECODE=1 python3 -B -O run_oracle.py --bridge build/bridge --out run_optimized --valid 320 --invalid 320 > report_optimized.json
cmp report_normal.json report_optimized.json && echo identical
./run_mutants.sh   # ponts mutés (répertoires mutants/<nom>/src copiés du dépôt, un seul header modifié)
```

`-B` et `PYTHONDONTWRITEBYTECODE=1` évitent tout `__pycache__` dans le dépôt (les modèles de
l'auditeur sont importés depuis `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/`).

## Format du journal JSON

```json
{"order": 3, "domain": [0,1,2,3],
 "populations": [{"interior": [1], "shell": [0,2]}, {"interior": [], "shell": [0,1,2,3]}],
 "batches": [{"level": ["16","1"], "actions": [{"parents": [], "contributions": [{"population": 0, "shell_mask": 3, "include_interior": true}]}]},
             {"level": ["25","1"], "actions": [{"parents": [0], "contributions": [{"population": 1, "shell_mask": 8, "include_interior": false}]}]}],
 "cuts": [["50","2"], ["1","0"]]}
```

Les niveaux sont des chaînes décimales `[num, den]` (num → U192, den → i128 signé) : les représentations
non réduites et les numérateurs multi-limbs sont transmis tels quels au produit.

## Résultats (HEAD audité 1fbe49d3 ; header byte-identique à 514038ed, sha256 `e8e65b21…`)

| Corpus | Journaux | Coupes | Évaluations nœud×coupe | Divergences |
| --- | ---: | ---: | ---: | ---: |
| valides aléatoires (5 profils) | 320 | 10 922 | 159 300 | 0 |
| géométrie réelle (7 nuages, K=1..n, gap+whole) | 66 | 2 088 | 26 244 | 0 (Gamma et lecture modèle) |
| invalides par mutation unique (41 classes appliquées, 12 raisons produit) | 320 | — | — | 0 (raison produit ∈ ensemble du modèle, primaire = attendue dans 320/320) |
| chaîne longue n=1500 (K1, K2 ; 2 999 nœuds, successeurs 1 499) | 2 | 32 | 96 032 | 0 |

`report_normal.json` == `report_optimized.json` (sha256 `13cf1cbf…`) ; `large_chain/report_*.json` identiques.

### Non-vacuité : huit headers mutés (`mutants/<nom>/src/forest/full_coverage_certificate.hpp`)

| Mutant | Origine | Oracle (journaux touchés / 186) | Gate constructeur 1fbe49d3 (`gate_vs_mutants/`) |
| --- | --- | ---: | --- |
| drop_continuation | développeur | 60 | tué (`growth.no_fake_node`) |
| future_contribution | développeur | 25 | tué (`growth.no_future_leak`) |
| final_root | développeur | 94 | tué (`replay.live_identity`) |
| parents_zero (`parents_.push_back(0)`) | auditeur historique (514038ed) | 94 (`nodes_mismatch`) | **survit**, 710 contrôles verts |
| open_as_closed (`admitted` := `cmp <= 0`) | propre | 126 | tué (`gram_gamma.coverage_multiset`) |
| birth_size_lax (`|I|+|U| + 1 < K`) | propre | 2 (+ fixture `fixtures/birth_size_k_minus_1.json`) | **survit**, 710 contrôles verts |
| continuation_new_node | propre | 69 | tué (`growth.no_fake_node`) |
| dedup_across_roots | propre | 83 | tué (`replay.coverage`) |

### Fixtures minimales (`fixtures/`)

- `birth_size_k_minus_1.json` : K3, population de taille 2 en naissance → nominal `coverage_birth_population` ; mutant `birth_size_lax` accepte et lit `[0,1]`.
- `k1_continuation_overlap.json` : K1, continuation ajoutant un point déjà racine → accepté ; à 1 fermé, racines 0→[0,1] et 1→[1] (recouvrement impossible en géométrie K1 ; autorité `structural_only`).
- `include_interior_on_empty_interior.json` : `include_interior=true` sur I vide (sérialisation du modèle auditeur) → `coverage_empty_or_invalid_mask`.
