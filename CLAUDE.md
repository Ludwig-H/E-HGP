# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Le dépôt est documenté en français ; travailler en français.

## Règles absolues

`AGENTS.md` (racine) est normatif et prévaut. En particulier :

- **Jamais de branche Git** sans accord explicite de l'utilisateur. Commits sur `main`.
- **Jamais de VM GCP** hors des scripts gardés de `gcp-migration/` (`start_and_verify.sh` / `stop_and_verify.sh`, VM `g4-standard-48` SPOT, label `project=e-hgp`, double coupe-circuit, `maxRunDuration` entre 30 s et 8 h). Après toute session créée/démarrée : certifier `TERMINATED` sur exactement cette cible. Si GCP n'est pas utilisé, ne lancer aucune commande GCP mutante et dire `GCP non utilisé`.
- **Aucun benchmark, accord moyen ou sortie plausible ne promeut `public_status=exact`** ; seuls les certificats et oracles prévus le peuvent. Distinguer proposition flottante, décision certifiée, réduction hiérarchique et statut public.
- Toute contradiction mathématique devient une **fixture minimale permanente** et met à jour `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant de continuer.
- **Tailles de nuage d'intérêt : `n = 8000`, `16000`, `32000`** (`docs/TEST_PLAN_MORSEHGP3D.md` § 3.1 ; 64000 en extension côté v4). Toute conclusion sur le coût, la sélectivité, la mémoire ou l'échelle s'y mesure ; quelques centaines de points ne la remplacent pas. Les petites tailles gardent un rôle distinct et unique — oracle de correction — et n'établissent jamais une pente. À l'échelle : invariants globaux et juge d'échantillon, jamais un juge `O(n^3)` ni un tableau indexé par paire.
- **Jamais de vérification exhaustive** (`docs/TEST_PLAN_MORSEHGP3D.md` § 3.2). Ce qu'un théorème garantit est *invoqué*, pas re-parcouru ; on grave ses **fixtures d'égalité**, pas ses cas intérieurs. Ce qui reste à tester est la faute d'implémentation, et elle se voit sur un invariant global, un juge d'échantillon ou un mutant. Les oracles bornés T2 (`n <= 12`–`14`) sont exclus de la règle : ils *établissent* la vérité au lieu de la re-vérifier.
- Ouverture/fermeture de phase formelle : mettre à jour `docs/implementation_status.toml` **dans le même commit**, puis `python tools/check_implementation_status.py`. Ne pas toucher ce registre pour une exploration v3/v4 ordinaire.
- Équations Markdown : une seule ligne physique, accolades explicites (`\mathbb{R}`), pas de `\operatorname`, pas de `\left\|`/`\left\{` (utiliser `\left\Vert`, `\left\lbrace`). `python tools/check_docs.py` vérifie tout cela (mais exclut `tests/**`).
- Invariant d'architecture : MorseHGP3D calcule la hiérarchie **sans matérialiser la mosaïque de Delaunay d'ordre supérieur** ni catalogue global de cellules/cofaces (∝ C(n,k) interdit). Les oracles exhaustifs restent bornés et hors du chemin produit. Les pistes de `docs/archive/abandoned/README.md` et des `PISTES_FERMEES.md` ne se rouvrent qu'avec un nouveau théorème de complétude + fixture, jamais sur un benchmark.

## Cible de travail : morsehgp3D_v5 (chantier actif)

`morsehgp3D_v5/` **remplace `morsehgp3D_v4/` comme chantier actif** (`AGENTS.md` § « Cible de travail », `morsehgp3D_v5/README.md`). Cadre à annoncer au début de toute tâche v5 :

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v5 calcule **le même objet** que la v4 avec **une base de code neuve** : la v4 est un sujet différentiel et une source de contre-fixtures et de digests épinglés, jamais une base de code ni une autorité implicite. Tout port contractuel est explicite, épinglé et requalifié (`morsehgp3D_v5/docs/PROVENANCE.md`) ; la conformité se prouve par campagnes appariées v4/v5 (digests canoniques au format v4 sur les mêmes entrées). Les errances de fond de la v4 relevées par l'audit du 22 août 2026 (résidence des dix forêts, plafond mémoire menteur, monolithes, opt-in négatifs, témoins legacy) sont traitées dès la conception. Un **auditeur** travaille dans `morsehgp3D_v5/audits/` et pousse sur `main` : vérifier régulièrement (`git pull`) s'il a déposé un audit, lui poser les verrous mathématiques ou d'implémentation par `QUESTION_CLAUDE_*`, et toujours pousser sur `main`.

## Commandes

Chantier actif `morsehgp3D_v5/` (C++20 sans extensions, `-Wall -Wextra -Wpedantic -Werror`, portes à code exact via `cmake/run_expect.cmake`, labels CTest `gate` / `oracle` / `scale8000` / `scale16000` / `scale32000`) :

```bash
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --parallel
ctest --test-dir build/v5 --output-on-failure
ctest --test-dir build/v5 --output-on-failure -R '^mhgp5_wspd_ledger_uniform$'   # un seul test
ctest --test-dir build/v5 --output-on-failure -L scale8000                       # tailles d'interet
```

Référence différentielle `morsehgp3D_v4/` (C++20 sans extensions, `-Wall -Wextra -Wpedantic -Werror`, 147 CTests, ~1 min en Release sur 8 cœurs, ~20 min sous ASan/UBSan) :

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 --parallel
ctest --test-dir build/v4 --output-on-failure
ctest --test-dir build/v4 --output-on-failure -R '^mhgp4_forest_accord$'      # un seul test
./build/v4/mhgp4_wspd_scaling_probe --family=uniform --n=8000 --s=8 --seed=3  # probe d'échelle
./build/v4/mhgp4_forest_probe --family=uniform --n=400 --s=8 --seed=3 --judge --min-balls=1000 --min-fusions=1000
```

Bibliothèque produit `morsehgp3d/` (C++20, Boost ≥ 1.74 requis — absent du conteneur par défaut, la CI installe `libboost-dev` ; `-Werror`, FP certifié) :

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
```

Chantier précédent `morsehgp3D_v3/` (construit aussi v2 via `add_subdirectory`, ~820 CTests) :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_flats_fixtures$'
```

Contrôles Python racine (unittest, pas pytest) — tous câblés dans `.github/workflows/ci.yml` :

```bash
python tools/check_docs.py                     # équations, liens, Markdown actif
python tools/check_passation.py                # fraîcheur de morsehgp3D_v4/PASSATION.md
python tools/check_implementation_status.py    # registre des phases vs roadmap
python tools/check_scope.py                    # prototypes retirés bannis
python tools/check_contracts.py && python tools/check_references.py
python -m unittest discover -s tests/contracts -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
python tools/run_oracle_campaign.py --ci
```

Préfixes : v5 = `mhgp5_*` (macros `MHGP5_*`), v4 = `mhgp4_*`, v3 = `mhgp3v_*`, v2 = `mhgp_*`, produit = `morsehgp3d.<nom>`. **La CI GitHub ne construit ni la v5, ni la v4, ni la v3** (seulement `morsehgp3d/` en GCC, Clang et ASan/UBSan, plus les contrôles Python) : rapporter explicitement les résultats CTest locaux. Les tests `mhgp3v_gate_d_fold_f0*` (v3) et `mhgp4_obig_selftest` contre `cpp_int` (v4) ne sont enregistrés que si Python 3, respectivement Boost, sont trouvés.

Options CMake notables : `MORSEHGP3D_ENABLE_CUDA` / `MHGP3V_ENABLE_CUDA` (OFF par défaut, sm_120 ; le build CUDA produit exige un worktree git **propre**), `MORSEHGP3D_ENABLE_SANITIZERS` (incompatible CUDA). Ni la v5 ni la v4 n'ont d'option CUDA. GMP optionnel côté v3 (second témoin du selftest arithmétique).

## Chaîne d'autorité documentaire

1. `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, Parties I–II (pages PDF 35–134, Défs 20–31, Th. 2–7) : définition normative de l'objet HGP.
2. `docs/SPECIFICATION_MORSEHGP3D.md` : l'objet à calculer ; une optimisation ne modifie ni l'objet, ni les niveaux, ni les inclusions.
3. `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` : registre des preuves (statuts `theorem_external`…`false_in_general`), à jour **avant** tout changement de statut public.
4. `docs/implementation_status.toml` : source opérationnelle de vérité des phases (Phase 15, `backend=reference_cpu`, `mode=budgeted`, `public_status=not_claimed`), validée par `tools/check_implementation_status.py` contre la roadmap.
5. `docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` + `docs/TEST_PLAN_MORSEHGP3D.md` : phases, portes d'entrée/sortie, plan T0–T6. Ne pas commencer une phase dont la porte d'entrée n'est pas satisfaite ; annoncer phase, `backend`, `profile`, `mode`.

Vocabulaire des statuts : `backend` ∈ {reference_cpu, cuda, cuda_g4}, `profile` ∈ {hgp_reduced, full_pi0, generic_core}, `mode` ∈ {certified, budgeted, benchmark_only}, `public_status` ∈ {exact, conditional, budget_exhausted, unsupported_degeneracy, numeric_failure} — un mode `budgeted` n'obtient jamais `exact`.

## Architecture (vue d'ensemble)

- `morsehgp3D_v4/` — chantier actif, reprise de zéro (voir section suivante pour le parcours et les conventions). **Une seule structure spatiale** : l'arbre radix de Karras sur les clés de Morton 48 bits des *positions uniques* (`src/tree/radix_tree.hpp`) — « Morton » n'est qu'une clé de tri, il n'y a ni octree séparé ni second arbre. Pipeline : entrée u16 + `PointId` u32 → tri Morton → buckets uniques → arbre radix → WSPD par vagues ternaire (`src/wspd/wavefront.hpp`, rectangles morts/terminaux/scindés par lane) → trois lanes génératrices de boules **q2** (paires diamétrales), **q3** (circumboules de triangles aigus, arête max = ancre), **q4** (seed aigu + complétion) dans `src/pipeline/ball_stream.hpp` (**le cœur ; ses en-têtes sont la doctrine**) → sort/RLE par `BallKey` → un census par clé (`I_B` intérieur strict / `U_B` coquille, plateaux cosphériques par quotient exact `src/forest/sphere_plateau.hpp`) → événements → fold compact en dix forêts `K=1..10` (`src/forest/forest.hpp`, `build_forest_legacy` figé comme témoin) → rendu § 9.1 (`src/forest/render.hpp`). Doctrine d'exactitude : tout prédicat décidé en entier (i64/i128/U192/U320, `src/events/`), aucun jitter ; le flottant n'existe que comme filtre certifié à repli exact (borne prouvée, coupé sous `__FAST_MATH__` ou hors `FE_TONEAREST`). Complétude sous seuils `h_q = s_max − q + 1` (`K_max ≤ 10 ⟺ smax ≤ 11`, refus explicite au-delà). Statuts transactionnels `complete_regular | unsupported_degeneracy | resource_exhausted | numeric_failure | incomplete_continuation | invalid_input`, séquence `count → preflight → fill → validate → publish`, jamais un préfixe de payload publié. `bench/forest_probe.cpp` porte le pipeline aval complet et **toutes les portes CLI** (`--*-gate`, `--judge`, `--inject=<mutant>`, `--min-*`, `--guard=`, `--max-output-bytes`) ; `oracle/obigint.hpp` est le juge à arithmétique volontairement autre.
- `morsehgp3d/` — bibliothèque produit (ligne enregistrée). Cible publique unique `morsehgp3d::morsehgp3d` (INTERFACE) → `src/cpu/api/point_hierarchy.cpp` ; en-tête public unique `include/morsehgp3d/morsehgp3d.hpp` → `api/point_hierarchy.hpp` : `build_exact_point_hierarchy(CertifiedTowerInput, PointHierarchyOptions)` → merge tree multi-ordres, routage descendant irréversible, rendus `select_lambda_cut` / `select_dbscan_radius` / `select_excess_of_mass`. Le réducteur est exact **relativement** à une tour déclarée complète par son producteur (reçus liés par `tower_payload_id`) ; il n'authentifie pas la vérité amont. Aucun producteur (v3 ou v4) ne l'alimente encore. `morsehgp3d/archive/` (surrogate point-MST v6, prototypes obsolètes) est hors build/API.
- `morsehgp3D_v3/` — chantier précédent (`exploration_v3_hors_registre`, `AUDIT_ETAT_COURANT.md` daté du 15 août 2026), toujours construit et testé ; source de fixtures, de théorèmes et de `audits/PISTES_FERMEES.md` (tentatives fermées : idée, cause, ce qui survit). Ses familles de nuages sont portées **bit à bit** en v4 (`src/cloud/families.hpp`), donc les reçus v3 restent confrontables. `prototype/` = sujets et portes (carte dans `prototype/README.md`) ; `oracle/` = juge indépendant.
- `morsehgp3D_v2/` — sujet jugé historique (lib `mhgp`), toujours construit par v3.
- `tests/SemanticKITTI/Zoltan/HierarchicalSelfAttention/` — dossier de recherche : une hiérarchie de densité HGP aide-t-elle la segmentation sémantique LiDAR ? **Commencer par `GUIDE.md`** (parcours d'entrée en neuf chapitres), avec `GLOSSAIRE.md`. Conception et falsification uniquement, aucune expérience apprise, aucun code, `public_status=not_claimed`. Trois contraintes à connaître avant de proposer quoi que ce soit : l'oracle de partition est une porte de **réfutation** et non de promotion ; HGP retarde la naissance des objets **filiformes**, là où se trouve la marge de mIoU ; le descripteur de nœud est le **levier le plus faible**. La cible est l'état de l'art val en régime strict : $73{,}1$ (DOS) à battre depuis une baseline reproductible à $68{,}0$–$70{,}3$. Se comparer à DOS, jamais au scratch. La laminarité exigée par une attention sur arbre n'est en revanche **pas** un obstacle : d'après le § 9.1 du manuscrit, l'arbre est déjà une partition des $(K-1)$-simplexes, donc laminaire sur les facettes, et la partition de l'unité $w_{x\tau}=S_\tau/T_x$ qui relie points et facettes y est fournie.
- `HGP-old/` — Python historique figé, **licence non commerciale propre**, jamais importé.
- `reference/` — oracles Python exhaustifs bornés (n ≤ 12–14), vérité terrain, jamais un backend.
- `tools/` — contrôles câblés en CI : `check_docs`, `check_passation`, `check_implementation_status`, `check_contracts`, `check_references`, `check_scope` (noms de prototypes retirés bannis), `check_gcp_workflows` (CI GCP strictement lecture seule), `check_oracle_independence`, `check_paragram_source_pin`.
- `schemas/`, `tests/` (racine) — contrats JSON v1/v2 (IDs canoniques sha256, `additionalProperties:false`) et leurs tests ; les artefacts de reçus (`docs/validation/*.json`, `scale_probe`) sont **immuables**. `tests/gcp/` teste les scripts de cycle de vie sans jamais toucher GCP.
- `third_party/paragram/<sha>/` — série de patchs candidate (Apache-2.0) sur un commit Paragram épinglé (`docs/references/paragram_source_pin.toml`) ; évaluation seulement, hors produit, aucun statut exact.
- `gcp-migration/`, `containers/` — scripts VM gardés (`session_*_g4.sh` = sessions complètes pin → préflight → démarrage gardé → runner → validateur → arrêt certifié ; `selftest_*.sh` = portes transactionnelles à lancer **à la main** avant toute session payante, jamais depuis la CI) et Dockerfiles épinglés. La CI GitHub ne touche jamais GCP en écriture.

## Conventions morsehgp3D_v4

- Ordre de lecture : `morsehgp3D_v4/PASSATION.md` (**fait foi pour l'état courant** : résultats acquis théorème → code → porte, carte de l'implémentation, chantiers ouverts par priorité, protocole G4), puis `docs/MATHEMATIQUES.md` (objet, réduction q2/q3/q4, fuseaux `W_q`, élagage `h_coeur/h_a/h_b`, statuts, questions Q1–Q5), `docs/ARCHITECTURE.md` (**plan initial**, antérieur à une grande partie du pipeline réel — la passation le met à jour), `docs/PLAN_DE_TESTS.md`, puis `audits/ETAT_COURANT.md` (verdict mutable unique, ancré au HEAD) et `audits/` en ordre chronologique inverse.
- Cycle des audits : les **auditeurs poussent sur `main`** ; leurs `AUDIT_*` / `CONTRE_AUDIT_*` / `ADDENDUM_*` se lisent **et s'exécutent** avant toute dépense. Claude répond par `REPONSE_CLAUDE_*` / `NOTE_CLAUDE_*` / `QUESTION_CLAUDE_*` dans `audits/`, et chaque livraison produit un reçu **immuable** ancré au commit dans `receipts/<chantier>_<date>/`. Tout fichier est daté `_YYYYMMDD` et, s'il juge du code, ancré au hash court. Les audits motivent des corrections, ils ne certifient rien.
- `python tools/check_passation.py` refuse une référence morte dans la passation et un item OPEN qui cite un reçu déjà déclaré exécuté sans marquer explicitement `LIVRÉ`/`OPEN` : mettre la passation à jour dans le même commit que la livraison.
- Portes à code de sortie **exact** via `mhgp4_expect_code(name expected target args)` (`cmake/run_expect.cmake`) : 0 conforme, 1 = désaccords du juge, 2 = refus avant calcul, 3 = plancher/invariant violé, 4 = mutant tué. Les crashs par signal sont refusés partout. Un CTest à `PASS_REGULAR_EXPRESSION` est doublé d'une porte à code.
- Toute porte exige des **planchers de couverture** (`--min-*`) contre le vert-par-vacuité, des **fixtures gravées** aux coordonnées exactes, des **mutants causaux tués** (`--inject=<nom>`), l'équivariance par permutation, et des sorties **bit-identiques** quel que soit le nombre de fils (`--par-gate`, `--workers-gate`). Le parallélisme est **mesuré, jamais déclaré** (compteurs d'ouvriers par lane, affinité effective).
- Familles de mesure : `uniform`, `terrain`, `eight_clusters`, `scanline_single_pass`, `scanline_overlap_multiecho` ; contre-familles gravées `two_lines`, `collinear_seven` (réfutations, jamais des régimes). Séparations WSPD `s = 6/8/10`, graine par défaut 3, `n ≤ 2000` = oracle de correction seulement.
- Profil d'entrée : u16 quantifié seulement, `PointId` u32 arbitraires (≠ index dense ≠ rang Morton — porte `--relabel-gate`), positions dupliquées bucketisées ; pas de position générale supposée ; les dégénérescences donnent un refus explicite, jamais un jitter.
- Style : deux espaces, `snake_case` fonctions/variables, `PascalCase` types, namespace `mhgp4`, en-têtes only sous `src/`. Python PEP 8 ; une porte Python ne repose jamais sur `assert` (doit tenir sous `python3 -O`).
- Commits : sujets en minuscules sans préfixe Conventional Commits (les auditeurs utilisent `audit: …`) ; joindre code, fixture, audit/reçu et passation quand ils décrivent la même décision. Jamais de jeton ni de clé dans le dépôt ou les logs ; les refus des scripts gardés sont finaux.

## Conventions morsehgp3D_v3 (pour y lire ou y corriger)

- Lire `morsehgp3D_v3/README.md` puis `audits/AUDIT_ETAT_COURANT.md`, puis la `NOTE_SOLUTION_*` du chantier visé. Cycle : `NOTE_SOLUTION_*` → implémentation dans `prototype/` + portes CMake → `AUDIT_LIVE_*` / `AUDIT_RECEPTION_*` → `AUDIT_REQUALIFICATION_*`.
- Portes négatives via `mhgp3v_add_expected_code_test_for` (`cmake/expect_failure.cmake`), mêmes codes 1–4 que la v4 ; l'oracle réécrit sa propre arithmétique et `mhgp3v_arith_selftest` juge le juge (témoins `__int128`/GMP). Namespace `mhgp3v`.

## Licences

MIT (racine) sur le code actif et la doc, **sauf** : les poids pré-entraînés de la lignée Pointcept (Sonata, Concerto, Utonia) s'ils sont utilisés dans `tests/SemanticKITTI/`, tous en **CC-BY-NC 4.0** — à traiter comme `HGP-old/`, jamais importés dans la ligne produit ; `HGP-old/` (licence historique non commerciale) ; `third_party/paragram/` (Apache-2.0, hors produit) ; les PDF de `docs/references/` (conditions fichier par fichier dans `references.toml`, intégrité par `python tools/check_references.py`).
