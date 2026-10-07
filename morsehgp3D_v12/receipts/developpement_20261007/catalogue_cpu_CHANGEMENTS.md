# Tranche T1, première partie : module produit `catalogue` de la v12 (voie CPU de référence) — changements fichier par fichier

7 octobre 2026. Base : `main` au HEAD `76adb8fa9` (travail commencé sur `df9140b5d` ; le HEAD a avancé à `1f7642e10`
puis `76adb8fa9` sans toucher `src/` hors d'un commentaire de `num/geometry.hpp`). Correctif : `patch_catalogue.diff`
(chemins `a/morsehgp3D_v12/…`, `b/morsehgp3D_v12/…`, 37 fichiers), à appliquer par `git apply` depuis la racine du
dépôt (`git apply --check` passe sur `76adb8fa9` et sur `576e7aaf9`, HEAD à 18:42, qui n'a changé que `microbancs/`,
`receipts/` et `audits/`). Copie de travail finale : `scratchpad/v12_catalogue/fresh76/`
(`git archive 76adb8fa9` + correctif, dépôt jetable) ; première copie de travail : `scratchpad/v12_catalogue/repo/`
(base `df9140b5d`). Aucune écriture sous `/workspaces/E-HGP`, aucune commande git d'écriture, GCP non utilisé.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie GPU du catalogue : à venir, même source)
objet=full_pi0 (étage C : Cat_K)
quantification=quantized_u21_input_only (profils 21, 24 et 32 construits et jugés)
public_status=not_claimed
```

Empreintes des sources portées : `src/catalogue/source_pins.json` (SHA-256 et commit de chaque source lue).

## 1. `src/catalogue/` — module neuf (nouvelle ligne de la table des modules)

| Fichier | Rôle | Source portée (empreinte dans `source_pins.json`) |
| --- | --- | --- |
| `catalogue.hpp` | en-tête public : `CatalogueParams` (K 1..12, feuille, `max_leaf` ≤ 256), `CatalogueBall`, `CatalogueLedger` (20 compteurs logiques de la v11), `CatalogueDiagnostics` (physiques), `Catalogue` (boules, niveaux, CSR I puis U, table $S^{*}\to$ boule `find_support`), `build_catalogue`, `export_catalogue` (MHGP12DP v1), `catalogue_digest` (SHA-256 de l'export) | API de `src/catalogue/catalogue.hpp` de la v11, réduite au chemin unique (aucun masque d'options) |
| `simt.hpp` | warp en source unique `__host__ __device__` ; largeur N : 32 (appareil et hôte), 256 (warp virtuel de l'hôte pour les feuilles de 33 à 256 sites, masques `Bits<4>`) ; collectives du parcours | `simt.hpp` de MES-M2 et `warp.hpp` de MES-M5 |
| `leaf_arith.hpp` | politiques arithmétiques de la feuille en repère local : `Narrow` (s ≤ 16, i32/i64/i128 natifs, bornes prouvées dans l'en-tête), `Exact` (toute s ≤ 33, entier exact de 320 bits à dépassement collant) ; le repli exact est la même source J3 | nouveau (contrat numérique § 3) |
| `leaf_predicates.hpp` | prédicats exacts de la feuille (côté, côté q2, orientation, centre intérieur, milieu local, triangle aigu, lemme Z, centre dans la boîte, enveloppes M3/E4, fabriques q2/q3/q4) génériques en la politique | `predicates.hpp` de MES-M2 (port de `leaf_device_predicates.hpp` de la v11) |
| `leaf_common.hpp` | mémoire d'un warp, préparation (dominances, graphe de paires, lignes vivantes) en coordonnées locales, compteurs : forme close de la voie « graphe de paires » (m ≤ 32) et de la voie DFS historique de la v11 (m > 32 : tests de couples, replis du cache J2) | `leaf_common.hpp` de MES-M2 |
| `leaf_census.hpp` | census par vote, support canonique **par positions** (CST-0113), admission, plafond déclaré de coquille (64 sites) | `leaf_common.hpp` de MES-M2 ; ordre d'énumération de `support.cpp` de la v11 |
| `leaf_j3.hpp` | feuille J3 par phases (paires, triplets et table H, quadruplets), générique en N et en politique | `leaf_j3.hpp` de MES-M2 |
| `traversal_records.hpp`, `traversal_kernels.hpp`, `traversal_scan.hpp` | parcours des boîtes en largeur : enregistrements, arithmétique du réservoir et de G1 dans le repère du parent, noyaux Select, Merge, Filter, Close, ScanA/B/C, Scatter, Emit, Iota | `bfs.hpp` de MES-M5, sans mutants |
| `traversal.hpp`, `traversal.cpp` | pilote des niveaux et exécuteur hôte : tableaux du front en `Buffer` du budget, noyaux répartis sur le Pool, feuilles remises au consommateur niveau par niveau (flux) ; refus `wide_leaf`, `index_overflow_u32`, profondeur > 3B (`catalogue_invariant`) | `driver.hpp` de MES-M5 |
| `internal.hpp`, `leaves.cpp` | étage des feuilles : lots de 16 384 feuilles au plus, comptage (feuille J3, case de 64 émissions par feuille), décalages contrôlés, réservation exacte, écriture (copie des cases, rejeu des feuilles qui débordent) ; choix de la largeur du warp et de la politique avant de jouer la feuille ; compteurs comptés une fois, au comptage, sommes contrôlées (`catalogue_counter_overflow`) | nouveau (contrat du catalogue § 4, CST-0211) |
| `sort.hpp`, `sort.cpp` | ordre canonique : niveau exact (clés F3, décision F4, repli exact), puis $S^{*}$ par positions ; tri parallèle par blocs et co-rangs | `sort_indices.cpp` et `sort_level_key.hpp` de la v11 |
| `assemble.cpp` | fin d'étage sur l'hôte : niveaux (`num::Sphere::through` de $S^{*}$), rangs, CSR, refus des doublons | `assemble.cpp` de la v11 |
| `table.cpp` | table $S^{*}\to$ boule (CSR par premier site, recherche par dichotomie) et `Catalogue::find_support` | nouveau |
| `catalogue.cpp` | validation, refus des multiplicités (D8), séquence parcours → feuilles → fin d'étage | `catalogue.cpp` de la v11 |
| `export.cpp` | export MHGP12DP v1 genre catalogue (SITEXYZ, BALLS, POPOFF, POPVAL, NLEVELS) sur `io::FileWriter`, empreinte canonique par le même code ; nom de trame d'au plus 23 octets ASCII imprimables, sinon `parameter_out_of_range` (`CST-0227` : le lecteur de transition refuse tout autre nom) | disposition de `format.hpp` de MES-M3/M4, écrite à neuf |
| `module.cmake`, `source_pins.json` | sources de la bibliothèque ; empreintes des ports | — |

## 2. Socle et documents

| Fichier | Changement |
| --- | --- |
| `src/core/reasons.def` | six raisons en fin de table (module `catalogue`) : `kmax_out_of_range`, `multiplicity_unsupported`, `wide_leaf`, `shell_capacity`, `catalogue_invariant`, `catalogue_counter_overflow` (statuts de la v11 ; `shell_capacity` nouveau, `unsupported_degeneracy`) |
| `cmake/modules.cmake` | module `catalogue`, dépendances `num sched cloud io` |
| `docs/ARCHITECTURE.md` | § 5 : ligne `catalogue` ajoutée à la table des modules présents, retirée des modules prévus ; § 7 : variable `MHGP12_V11_CATALOGUE_DIR` (vidages de la v11 des portes `diff_v11`) |
| `README.md` | ligne d'option `-DMHGP12_V11_CATALOGUE_DIR=…` dans les commandes |
| `tests/core/status_test.cpp` | table gravée des raisons : six lignes ajoutées (module `catalogue`), plancher 65 → 83, dernière raison `catalogue_counter_overflow` |

## 3. Portes et outils de test

| Fichier | Rôle |
| --- | --- |
| `bench/catalogue_probe.cpp` (`mhgp12_catalogue_probe`) | sonde : entrée `u32le` + `ids.u32le` ou nuage synthétique `--uniform=N,GRAINE,BITS` ; JSON (statut, comptes, grand livre, diagnostics) ; `--digest` ; export MHGP12DP par dossier transactionnel (`--out`) |
| `tests/catalogue/tests.cmake` | portes : unités, oracle, échelle, Euler, LiDAR, différentiel v11 (si `MHGP12_V11_CATALOGUE_DIR`) |
| `tests/catalogue/unit.cpp` | témoins `WIT-T1-CARRE`, rectangle des deux conventions de $S^{*}$, `WIT-TRANSL`, triangle long (feuille d'étendue B + 1, repli exact), `WIT-SPHERE50`, `WIT-FEUILLES`, profondeurs 60 et 63 (CST-0205) avec grands livres de la v11, refus (paramètres, multiplicités, feuille large, budget, nom de trame), somme des compteurs, paliers (homothétie aux deux bords du domaine, racine fermée à $2^{32}$ au profil 32, CST-0204), parcours u32 contre l'oracle de MES-M5, déterminisme 1/4/8 fils |
| `tests/catalogue/oracle.py` | oracle borné : égalité avec l'étage B de `reference/hgp12_ref` sur la suite rapide |
| `tests/catalogue/diff_case.py` | porte du différentiel contre la v11 : la sonde exporte Cat_5 (trame du cas), puis le lecteur de transition général `reference/transition_catalogue.py` le juge contre le vidage de la v11 ; codes et ligne du lecteur. Le lecteur provisoire écrit d'abord pour cette tranche (`diff_v11.py`) est retiré du correctif depuis la parution du lecteur général |
| `tests/catalogue/catalogue_dump.py` | lecteur MHGP12DP des portes oracle et Euler (sections, sites, boules, populations ; centre exact et rayon carré de $S^{*}$, ordre publié de la v12 pour l'oracle) ; nom de trame imprimable exigé ; aucun comparateur |
| `tests/catalogue/ledger_gate.py`, `v11_counts.json` | échelle et déterminisme : comptes et 20 compteurs égaux à ceux de la v11 (comptes gravés, aucune donnée), mêmes octets à 1, 4 et 8 fils |
| `tests/catalogue/euler.py` | filets JUG-EULER à K+2 et restriction J1 ; option de recensement exact de chaque site listé |
| `tests/mutants/catalogue.json` | six mutants causaux |
