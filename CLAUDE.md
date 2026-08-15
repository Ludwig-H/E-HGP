# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Le dépôt est documenté en français ; travailler en français.

## Règles absolues

`AGENTS.md` (racine) est normatif et prévaut. En particulier :

- **Jamais de branche Git** sans accord explicite de l'utilisateur. Commits sur `main`.
- **Jamais de VM GCP** hors des scripts gardés de `gcp-migration/` (`start_and_verify.sh` / `stop_and_verify.sh`, VM `g4-standard-48` SPOT, label `project=e-hgp`, double coupe-circuit, `maxRunDuration` entre 30 s et 8 h). Après toute session créée/démarrée : certifier `TERMINATED` sur exactement cette cible. Si GCP n'est pas utilisé, ne lancer aucune commande GCP mutante.
- **Aucun benchmark, accord moyen ou sortie plausible ne promeut `public_status=exact`** ; seuls les certificats et oracles prévus le peuvent. Distinguer proposition flottante, décision certifiée, réduction hiérarchique et statut public.
- Toute contradiction mathématique devient une **fixture minimale permanente** et met à jour `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant de continuer.
- Ouverture/fermeture de phase : mettre à jour `docs/implementation_status.toml` **dans le même commit**, puis `python tools/check_implementation_status.py`.
- Équations Markdown : une seule ligne physique, accolades explicites (`\mathbb{R}`), pas de `\operatorname`, pas de `\left\|`/`\left\{` (utiliser `\left\Vert`, `\left\lbrace`). `python tools/check_docs.py` vérifie tout cela.
- Invariant d'architecture : MorseHGP3D calcule la hiérarchie **sans matérialiser la mosaïque de Delaunay d'ordre supérieur** ni catalogue global de cellules/cofaces (∝ C(n,k) interdit). Les oracles exhaustifs restent bornés et hors du chemin produit. Les pistes de `docs/archive/abandoned/README.md` ne se rouvrent qu'avec un nouveau théorème de complétude + fixture, jamais sur un benchmark.

## Commandes

Bibliothèque produit `morsehgp3d/` (C++20, Boost ≥ 1.74, `-Werror`, FP certifié) :

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
python tools/check_docs.py
python tools/check_implementation_status.py
```

Exploration `morsehgp3D_v3/` (construit aussi v2 via `add_subdirectory`, 749 CTests, dont une majorité de portes négatives à code de sortie exact) :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
ctest --test-dir build/v3 --output-on-failure
```

Un seul test : `ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_flats_fixtures$'`. Préfixes : v3 = `mhgp3v_*`, v2 = `mhgp_*`, produit = `morsehgp3d.<nom>`.

Tests Python racine (unittest, pas pytest) :

```bash
python -m unittest discover -s tests/contracts -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
python tools/run_oracle_campaign.py --ci
```

Options CMake notables : `MORSEHGP3D_ENABLE_CUDA` / `MHGP3V_ENABLE_CUDA` (OFF par défaut, sm_120 ; le build CUDA produit exige un worktree git **propre**), `MORSEHGP3D_ENABLE_SANITIZERS` (incompatible CUDA). GMP optionnel côté v3 (second témoin du selftest arithmétique). Les tests `mhgp3v_gate_d_fold_f0*` ne sont enregistrés que si Python3 est trouvé.

## Chaîne d'autorité documentaire

1. `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, Parties I–II (pages PDF 35–134) : définition normative de l'objet HGP.
2. `docs/SPECIFICATION_MORSEHGP3D.md` : l'objet à calculer ; une optimisation ne modifie ni l'objet, ni les niveaux, ni les inclusions.
3. `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` : registre des preuves (statuts `theorem_external`…`false_in_general`), à jour **avant** tout changement de statut public.
4. `docs/implementation_status.toml` : source opérationnelle de vérité des phases (validée par `tools/check_implementation_status.py` contre la roadmap).
5. `docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` + `docs/TEST_PLAN_MORSEHGP3D.md` : phases, portes d'entrée/sortie, plan T0–T6. Ne pas commencer une phase dont la porte d'entrée n'est pas satisfaite ; annoncer phase, `backend`, `profile`, `mode`.

Vocabulaire des statuts : `backend` ∈ {reference_cpu, cuda, cuda_g4}, `profile` ∈ {hgp_reduced, full_pi0, generic_core}, `mode` ∈ {certified, budgeted, benchmark_only}, `public_status` ∈ {exact, conditional, budget_exhausted, unsupported_degeneracy, numeric_failure} — un mode `budgeted` n'obtient jamais `exact`.

## Architecture (vue d'ensemble)

- `morsehgp3d/` — bibliothèque produit. Cible publique unique `morsehgp3d::morsehgp3d` (INTERFACE) → `src/cpu/api/point_hierarchy.cpp` ; en-tête public unique `include/morsehgp3d/morsehgp3d.hpp` → `api/point_hierarchy.hpp` : `build_exact_point_hierarchy(CertifiedTowerInput, PointHierarchyOptions)` → merge tree multi-ordres, routage descendant irréversible, rendus `select_lambda_cut` / `select_dbscan_radius` / `select_excess_of_mass`. Le réducteur est exact **relativement** à une tour déclarée complète par son producteur (reçus liés par `tower_payload_id`) ; il n'authentifie pas la vérité amont. `morsehgp3d/archive/` (surrogate point-MST v6, prototypes obsolètes) est hors build/API.
- `morsehgp3D_v3/` — exploration courante (`exploration_v3_hors_registre`, aucun statut public). `prototype/` = sujets et portes, carte dans `prototype/README.md` ; `oracle/` = juge indépendant (bigint/rationnel propres, aucune primitive de production) ; `audits/` = cycle documentaire, avec `audits/PISTES_FERMEES.md` = mémo des tentatives fermées (idée, cause d'abandon, ce qui survit). Dépend de v2 comme « sémantique candidate et fixtures, jamais une autorité ».
- `morsehgp3D_v2/` — sujet jugé historique (lib `mhgp`), toujours construit par v3.
- `tests/SemanticKITTI/Zoltan/HierarchicalSelfAttention/` — dossier de recherche : une hiérarchie de densité HGP aide-t-elle la segmentation sémantique LiDAR ? **Commencer par `GUIDE.md`** (parcours d'entrée en neuf chapitres), avec `GLOSSAIRE.md`. Conception et falsification uniquement, aucune expérience apprise, `public_status=not_claimed`. Trois contraintes à connaître avant de proposer quoi que ce soit : l'oracle de partition est une porte de **réfutation** et non de promotion ; HGP retarde la naissance des objets **filiformes**, là où se trouve la marge de mIoU ; le descripteur de nœud est le **levier le plus faible**. La cible est l'état de l'art val en régime strict : $73{,}1$ (DOS) à battre depuis une baseline reproductible à $68{,}0$–$70{,}3$. Se comparer à DOS, jamais au scratch. La laminarité exigée par une attention sur arbre n'est en revanche **pas** un obstacle : d'après le § 9.1 du manuscrit, l'arbre est déjà une partition des $(K-1)$-simplexes, donc laminaire sur les facettes, et la partition de l'unité $w_{x\tau}=S_\tau/T_x$ qui relie points et facettes y est fournie.
- `HGP-old/` — Python historique figé, **licence non commerciale propre**, jamais importé.
- `reference/` — oracles Python exhaustifs bornés (n ≤ 12–14), vérité terrain, jamais un backend.
- `tools/` — contrôles câblés en CI : `check_docs`, `check_implementation_status`, `check_contracts`, `check_references`, `check_scope` (noms de prototypes retirés bannis), `check_gcp_workflows` (CI GCP strictement lecture seule), `check_oracle_independence`.
- `schemas/`, `tests/` (racine) — contrats JSON v1/v2 (IDs canoniques sha256, `additionalProperties:false`) et leurs tests ; les artefacts de reçus (`docs/validation/*.json`, `scale_probe`) sont **immuables**.
- `gcp-migration/`, `containers/` — scripts VM gardés et Dockerfiles épinglés. La CI GitHub ne touche jamais GCP en écriture.

## Conventions morsehgp3D_v3 (le chantier actif)

- Lire d'abord `morsehgp3D_v3/README.md` puis `audits/AUDIT_ETAT_COURANT.md` (autorité courante, ancrée aux hashes), puis la `NOTE_SOLUTION_*` du chantier visé.
- Cycle des audits : `NOTE_SOLUTION_*` (spécification de solution) → implémentation dans `prototype/` + portes CMake → `AUDIT_LIVE_*` / `AUDIT_RECEPTION_*` (réception, ancrée au hash court) → `AUDIT_REQUALIFICATION_*`. Dialogue : `QUESTION[S]_CLAUDE_*` ↔ `REPONSE_*` ; notes de Claude : `NOTE_CLAUDE_*`. Les audits motivent les corrections, ils ne certifient rien.
- Portes négatives à code de sortie **exact** via `mhgp3v_add_expected_code_test_for` (`cmake/expect_failure.cmake`) : 1 = désaccords du juge, 2 = campagne refusée avant calcul, 3 = plancher/invariant violé, 4 = mutant tué. Les crashs par signal sont refusés partout.
- Toute porte exige des **planchers de couverture** (`--min-*`) contre le vert-par-vacuité, des **fixtures gravées** aux coordonnées exactes, des **mutants tués** (`--inject`, `--force-*`), et l'équivariance par permutation. Un CTest à `PASS_REGULAR_EXPRESSION` est doublé d'une porte à code (le regex ignore le code de retour).
- L'oracle réécrit sa propre arithmétique (représentation volontairement différente de la production) pour qu'un défaut commun ne se compense pas ; le selftest `mhgp3v_arith_selftest` juge le juge (témoins `__int128`/GMP).
- Profil d'entrée : u16 quantifié seulement ; pas de position générale supposée ; les dégénérescences donnent un refus explicite, jamais un jitter.

## Licences

MIT (racine) sur le code actif et la doc, **sauf** : les poids pré-entraînés de la lignée Pointcept (Sonata, Concerto, Utonia) s'ils sont utilisés dans `tests/SemanticKITTI/`, tous en **CC-BY-NC 4.0** — à traiter comme `HGP-old/`, jamais importés dans la ligne produit ; `HGP-old/` (licence historique non commerciale) et les PDF de `docs/references/` (conditions fichier par fichier dans `references.toml`, intégrité par `python tools/check_references.py`).
