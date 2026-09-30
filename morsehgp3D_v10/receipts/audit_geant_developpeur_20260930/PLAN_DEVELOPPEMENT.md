# Plan de développement v10 — tranches après l'audit géant

30 septembre 2026. Plan seulement : rien n'est livré ni qualifié par ce document.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_geant_developpeur
public_status=not_claimed
GCP non utilisé
```

Constats : [TRACKER.md](TRACKER.md). Rapport : [RAPPORT_AUDIT_GEANT.md](RAPPORT_AUDIT_GEANT.md).
Plans de lentilles repris : `raccord_r2/PLAN_INTEGRATION_R2.md`, `precision/PLAN_PRECISION.md`,
`frontiere/RAPPORT_FRONTIERE_20260930.md` § 10, corrigés par leurs vérificateurs.

## 0. Principes communs

- **Un seul arbre intégré par tranche**, un seul jeu de binaires. Une preuve faite sur une
  copie ne vaut pas pour le binaire intégré.
- **Sorties identiques** octet pour octet à la tranche précédente, sauf changement voulu,
  annoncé et compté (seule T2 change des sorties : la tête).
- **Tailles** : 8 000 / 16 000 / 32 000 sites et trames LiDAR entières pour tout constat de
  coût ou d'échelle. Les petites tailles servent d'oracle seulement.
- **Pas de vérification exhaustive** : invariants globaux, juges d'échantillon, mutants
  causaux, fixtures d'égalité gravées.
- **Portes à code exact** (`run_expect.cmake`), planchers contre le vert par vacuité. Un
  crash par signal fait échouer la porte. Un mutant qui rend 0 sans refus d'un juge survit.
- **Sanitizers** : absence d'alerte prouvée par le code de sortie ou le stderr conservé,
  jamais par des fichiers `log_path` vides (PT9).
- **Reçu immuable** ancré au commit ; PASSATION mise à jour dans le même commit.
- **Contradiction mathématique** : fixture permanente et ligne du registre avant de
  continuer.
- **Worktree partagé** : jamais `git add -A` ; index vide vérifié avant `git add` ; pousser par
  cherry-pick sur `origin/main`.
- **GCP non utilisé dans T0–T4.** Une session G4 ne vient qu'après les portes CPU d'un
  palier, par les scripts gardés, avec arrêt certifié `TERMINATED`.

Décisions de détail prises par le développeur, avec leur raison : rapport, § 5.

## 1. Vue d'ensemble

| Tranche | Objet | Dépend de | Sorties changées | Ferme |
| --- | --- | --- | --- | --- |
| T0 | Raccord R2 (en cours) | — | non, sauf entrées désormais refusées | RC1–RC10, SO1, SO2, CL1–CL3, TT2–TT5, PL1, PL2, ST1, JG1–JG3, BN1–BN3, DC1, DC4–DC6, ER1, PT9 ; en partie RG1, MM1, MM2, ST2 |
| T1 | Registre v10, portes de couverture, gardes bon marché | T0 | non | RG1, AT1, AT2, PT1–PT7, FP1, CU1, MM5, MM8, PR9, PR10 (garde), DC2, DC3, CI1–CI5 ; en partie MM1, MM2 |
| T2 | Tête : condensation des départs de points | T0 | oui (tête seule) | TT1 |
| T3 | Précision : palier u24 (P1a interne B21, puis P1b) | T0, T1 (AT1) ; empreintes u18 prises à la sortie de T2 | non en u18 | PR1–PR8, PR11–PR15, TT6, ST2 |
| T4 | Campagne frontière | T0, T1, T2 | non (banc) | FR1–FR12, PT6 |
| Suites | u32 complet, massif, CI v10, G4, performance | selon décisions | — | PR10, MM3, MM4, MM6, MM7, MM9, MM10, BN4, CU2, PF1, PF2, PT8 |

Ordre proposé : T0 → T1 → T2 → T3 → T4. T4 ne dépend pas de T3 : elle peut passer avant si
l'utilisateur le préfère (question 4 du rapport).

## 2. T0 — Raccord R2

### 2.1 Objet et état

Intégrer les sept groupes R2 sur le HEAD courant dans un arbre unique, avec les corrections
trouvées par l'audit, puis qualifier ce binaire. Le raccord tourne dans
`build/v10-integration-r2` (base `a8252527e`) : `faits_math` y est appliqué ; la suite
CTest de la base y rend 0 (386 s).

### 2.2 Étapes

| # | Groupe | Source | Fichiers | Zones de conflit | Ajouts de l'audit |
| --- | --- | --- | --- | ---: | --- |
| 1 | faits_math | `65a86d2b` | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, `CMakeLists.txt`, `PASSATION.md`, `README.md`, `bench/synthetic/prereg/README.md`, `cli/mhgp10_cluster.cpp` (commentaire), `docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`, `docs/conception/CLUSTER_v2.md`, `src/tower/tower.hpp` (commentaire), `tests/regression/test_projection_facts.py`, `test_projection_registry.py` | 0 | Relecture indépendante ; cas V1/V2 et P5/P9 ; `StopIteration` → code 2 ; les trois lieux « amas discrets » ajoutés par le vérificateur (DC1). |
| 2 | sitetree réparé | base → `82b49a9` (sha256 `4343401c…`) | `src/cloud/site_tree.{hpp,cpp}`, `CMakeLists.txt`, `tests/regression/site_tree_far_center.cpp`, `site_tree_fast_math.cmake`, `site_tree_old_api.cmake`, `tests/unit/unit_main.cpp` | 0 | Publier le patch et un reçu (RC3). Graver G1 et les trois supports lointains (ST1). Précondition 19–21 bits (ST2). |
| 3 | bancs | `e47ab923` | `bench/g4/merge_sessions.py`, `bench/scaling/scale_run.py`, `bench/synthetic/decide.py`, `tests/regression/test_decide_completeness.py`, `test_scale_run_timeout.py`, `CMakeLists.txt` | 0 | Schéma du préenregistrement validé avant tout calcul (BN1, P6). S1, S2, XD1–XD4, XM1–XM4 ; `TypeError` → code 2 ; reçus A et C dans l'extraction, absents → code 2 (BN3). |
| 4 | pool | `db5e04e4` | `src/sched/pool.{hpp,cpp}`, `src/catalogue/generator.cpp`, `src/tower/tower.cpp`, `tests/head/mreach_cluster.cpp`, `tests/regression/test_worker_bad_alloc.py`, `tests/unit/fault_main.cpp`, `fault_preload.cpp`, `unit_main.cpp`, `CMakeLists.txt` | 1 | Porte de participation contre MV3 ; valeur du décompte aligné (MBX1) ; commentaire du CAS ; `std::system_error` → refus (PL1, PL2). |
| 5 | entrees_cli | `b1c0b530` | `cli/mhgp10_{catalogue,tower,cluster}.cpp`, `src/core/{cli_options,cli_output}.hpp`, `src/cloud/u32le_input.hpp`, `src/core/reasons.def`, `src/catalogue/{catalogue.hpp,generator.cpp}`, `tests/head/mreach_cluster.cpp`, `tests/regression/test_cli_input_frontiers.py`, `test_fast_targets.py`, `tests/unit/unit_main.cpp`, `docs/conception/{ARCH_v2,CONCEPTION_V10}.md`, `CMakeLists.txt` | 6 | `OutputSet` réécrit (SO2, SO1). Cas isolants XA3, XA5, XA8, XB2, XD1, XE1–XE4, XE6 (RC2). Chemins vides refusés (CL2). `build_catalogue_unguarded` ; `try`/`catch` du pool gardé dans `mreach_cluster.cpp` (RC7). Portes `fast` de HEAD sous `run_expect` (RC5). |
| 6 | oracles | `ea79324e` | `tests/oracle/test_{catalogue,tower}_oracle.py`, `cli/mhgp10_{catalogue,tower}.cpp`, `docs/SPEC_V10.md`, `CMakeLists.txt` | 5 | Drapeaux `--dump-levels`/`--dump-births` dans le parseur strict et les écrivains d'`OutputSet` (JG3). Feuille `max(8, K+3)` (RC6). Règle stricte des naissances ; convention fermée ; lecteur en flux ; erratum du reçu § 4 (JG2). |
| 7 | tete | `af48faf8` | `src/head/head.{hpp,cpp}`, `src/points/dendrogram.{hpp,cpp}`, `cli/mhgp10_cluster.cpp`, `src/core/reasons.def`, `tests/head/{head_complexity.cpp,head_gate.cpp,mreach_cluster.cpp,test_head_frontier.py}`, `CMakeLists.txt` | 12 | Frontière de tête unique (RC4). Fixtures G12, G17, R01 (RC2). Ordre des raisons (RC9). Tout ou rien multi-K. Tests `:383` et `:406-407` alignés. |

Après l'étape 7 : `mhgp10_fast_targets` doit rendre 0 ; relire l'en-tête fusionné de
`cli/mhgp10_cluster.cpp` puis rejouer `mhgp10_regression_projection_registry`.

### 2.3 Recette `OutputSet` (SO1, SO2)

1. Avant toute réservation : refuser les destinations concurrentes et l'entrée (chemin
   normalisé, puis `st_dev`/`st_ino` pour un fichier existant), en refus comme en succès.
2. Fichier régulier : temporaire dans le même dossier, `rename` au `commit`. Rien de
   préexistant n'est tronqué ni retiré.
3. Autre destination (`/dev/full`, périphérique) : écriture vérifiée (`fwrite`, `fflush`,
   `fclose`), refus `output_unwritable`. Lien symbolique : suivi, pas remplacé.
4. Multi-K : toutes les configurations calculées, tous les temporaires écrits, puis les
   `rename`. Un refus à un ordre ne laisse aucune sortie.

### 2.4 Portes (binaire intégré unique)

| # | Vérification | Critère |
| --- | --- | --- |
| V1 | Release GCC 13 et Clang 18 `-Werror` ; ASan+UBSan ; TSan sous `setarch -R` | 0 avertissement, 0 rapport |
| V2 | `ctest -L gate` complet, oracles compris | tout vert, codes exacts |
| V3 | Plan G4 simulé : `MHGP10_FAST_TARGETS`, `ctest --no-tests=error -L fast` | rc 0 ; `mhgp10_fast_targets` rc 0 |
| V4 | Portes Python en `python3` et `python3 -O` ; bancs aussi en Python 3.10 | mêmes codes |
| V5 | `differentiel_j2c.sh` contre HEAD | 10/10 identiques à 1 et 4 fils |
| V6 | Dumps de tour contre HEAD : 8 000, 16 000, 32 000, trames LiDAR entières ; K5 et K10 ; core et cover ; 1 et 4 fils | identiques octet pour octet (SHA-256 complet) |
| V7 | `head_diff.py` contre HEAD | 18/18 identiques |
| V8 | Campagnes de mutants de chaque groupe, rejouées | tués ou équivalents prouvés |
| V9 | Sondes des auditeurs et des vérificateurs (liste ci-dessous) | refus propres, code 2 ou 3, rien de détruit, aucun signal |
| V10 | Invariants globaux et juge d'échantillon à 8 000, 16 000, 32 000 | aucun écart ; jamais de juge O(n^3) |
| V11 | `check_docs`, `check_passation`, `check_scope`, `check_implementation_status` | pas de nouveau constat |
| V11b | Bancs sur les CLI intégrées : `scale_run.py run`, re-décision des lots A et C, re-fusion c1+c2 ; commandes des plans G4 passées au parseur strict, hors ligne | identiques aux archives ; aucune option refusée |
| V12 | Reçu d'intégration immuable : commit, sha256 des binaires, commandes, sorties, errata des reçus R2 | publié avec le commit |

Sondes de V9 : collision étiquettes/arbre (alias `./` et lien symbolique compris) ; entrée =
sortie en refus et en succès ; fichier préexistant sur refus ; `/dev/full` ; `--tree` vers un
dossier absent (aucun préfixe) ; `--configs` avec une ligne illisible ; K supérieur au nombre
de sites ; options illisibles ou hors plage ; `--repeat=0` et `-1` ; `--no-points --dump` ;
K=1 mcs=1 ; z ∈ {0, nan, inf, −1, 17} ; mcs ∈ {0, −1} ; reste de 1 à 11 octets ; alpha=2,
alpha=NaN, en-têtes dupliqués, méthode non enregistrée ; lecteur en flux (ordre manquant,
sites étrangers, tailles fausses).

### 2.5 Mutants

| Groupe | Campagne à rejouer sur l'arbre intégré |
| --- | --- |
| pool | 6 portes existantes + porte de participation (tue MV3) + contrôle de valeur (tue MBX1) |
| sitetree | 28 mutants de la lentille + 6 du vérificateur (dont `in_domain`, `chemin_*`, `drapeau_*`) |
| entrees_cli | 13 mutants du vérificateur ; les 10 survivants tués par cas isolants |
| oracles | 24 mutants gravés ; campagnes systématiques 1 168 et 1 656 |
| tete | 41 tués + G12, G17, R01 |
| bancs | S1 (sous 3.10 et 3.12), S2 sous le délai CTest, XD1–XD4, XM1–XM4 |
| faits_math | V1, V2, P1, P3, P4, P5, P9, P10, P11 |

### 2.6 Critères de sortie

1. V1–V12 et V11b verts sur le même commit.
2. Mutants tués ou équivalents prouvés.
3. Différentiels identiques sur entrées valides ; les seules différences de sortie sont des
   refus nouveaux, listés.
4. Reçu immuable ; un commit par groupe, ou une série documentée.
5. PASSATION mise à jour (DC4) ; navigation `audits/README.md` complétée (DC6) ;
   `REPONSE_CLAUDE` aux auditeurs avec les chiffres corrigés (ER1, DC5).

## 3. T1 — Registre v10, portes de couverture, gardes bon marché

### 3.1 Objet

Fermer le bloquant, les trous de porte et les gardes qui ne changent aucune sortie.

### 3.2 Contenu et fichiers

| Bloc | Contenu | Fichiers | Lignes |
| --- | --- | --- | --- |
| Registre | Section v10 complétée : § 9 masses uniformes, § 10 1/β non robuste (marge −1,72 % → +21,8 %), § 11 entrées internes K3/K5, cinq sites, croisement K2/K3, bord de bande (B_η et antichaîne), quasi-égalité K3 (A5/A6). Une fixture permanente par ligne. | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ; `tests/regression/test_frontier_facts.py` (nouveau) | RG1 |
| Attaches | Juge T2 sur core **et** cover ; invariant exact `niveau(v) <= e < niveau(parent(v))` sur les niveaux exacts du dump ; porte d'échelle linéaire ; fixtures 3, 5 et 9 points, étiquettes HEAD gravées avec l'explication mreach/sklearn. `validate()` inchangée. | `tests/oracle/test_tower_oracle.py` ; `tests/regression/test_attach_invariant.py`, `test_attach_equality.py` (nouveaux) ; contrat dans `src/points/dendrogram.hpp` | AT1, AT2 |
| Gardes d'indices | Compteur de boules par feuille, paramètre caché `ball_limit` ; `std::min<u64>(kmax, n)` ; contrôle de `ext_reps.size()` ; refus si N_K + n ≥ kNone quand les points sont demandés ; `static_assert(kMaxCatalogueOrder <= 64)`, `static_assert(kMaxShellEnum <= 32)` ; `static_assert` liant `kCoordinateBits` au chemin K-NN u64 ; refus si plus de `INT32_MAX` amas. | `src/catalogue/generator.cpp` ; `src/tower/tower.cpp:1177, :1288-1296, :1618, :1776-1840` ; `src/head/head.cpp:151` | MM2, MM5, MM8, PR10 |
| Mémoire | En-tête de `buffer.hpp` corrigé : il ne doit plus affirmer un budget honnête. | `src/core/buffer.hpp:1-2` | MM1 |
| Flottant | `#error` sous `__FAST_MATH__`, `__ASSOCIATIVE_MATH__`, `__RECIPROCAL_MATH__`, `__FLT_EVAL_METHOD__ != 0` dans `tower.cpp` ; `-ffp-contract=off` sur `tower.cpp` ; CMake refuse `fast-math` dans `CMAKE_CXX_FLAGS_<CONFIG>` et `-Ofast` ; contrôle `FE_TONEAREST` à l'entrée de `build_catalogue` et `build_tower`, raison `fp_environment` ajoutée après les raisons R2. | `src/tower/tower.cpp` ; `CMakeLists.txt:20-22` ; `src/core/reasons.def` | FP1 |
| Portes | Section 3 de `rank_search.cpp` bornée ; label `fast` sur `rank_search` et `grid32` ; variantes `-O` ; `PYTHONDONTWRITEBYTECODE` ; `find_package(Python3 REQUIRED)` ; planchers de `test_level_collision.py` ; porte `morton96 == morton3` sur u21. | `tests/unit/rank_search.cpp:54-65` ; `CMakeLists.txt` ; `tests/regression/test_level_collision.py` ; `tests/unit/grid32_primitives.cpp` | PT1–PT5, PT7, PR9 |
| Bancs | `cuda_probe.py` : conversions capturées ; `attempt.json` sur tout échec. | `bench/g4/cuda_probe.py` | CU1 |
| Doc | Limites statistiques à côté des scores ; phrase « plafond de Bayes » corrigée et erratum ; ARCH_v2 § 13 marqué « non livré ». | `PASSATION.md`, `README.md`, `receipts/ERRATA.md`, `docs/conception/ARCH_v2.md` | DC2, DC3, CI3 |
| Dépôt | `check_docs` : exclure les copies de provenance sous `receipts/`, sans réécrire de reçu ; périmètre de `check_scope` inchangé ; workflows v7 et v9 rejoués localement avant de pousser. `git add -f` des preuves ignorées du développeur ; `QUESTION_CLAUDE_*` pour celles des auditeurs et le manifeste périmé. Signalement de la CI `main` rouge aux propriétaires ; proposer un job Python séparé. | `tools/check_docs.py` ; reçus `g4_session3_j2`, `g4_session4_j2c`, `catalogue_fitted_split_j2c` | CI1, CI2, CI4, CI5 |

### 3.3 Portes

| Porte | Labels | Attendu | Plancher |
| --- | --- | --- | --- |
| `mhgp10_regression_frontier_facts` (normal, `-O`) | gate;regression | une ligne par fait, code 0 | toutes les fixtures jouées |
| `mhgp10_tower_oracle` (core et cover) | gate;oracle | 0 écart ; invariant d'égalité jugé | les deux entrées exécutées |
| `mhgp10_attach_invariant` | gate;regression | 0 violation sur lidar02 K5/K10 et 8 000/16 000/32 000, core et cover | au moins 10^6 attaches jugées |
| `mhgp10_attach_equality` | gate;fast | fixtures 3, 5, 9 points : attaches et étiquettes gravées | 3 fixtures |
| `mhgp10_ball_guard` | gate;fast | `ball_limit = B` refusé en `index_overflow_u32`, `B+1` admis ; raison aval fixée | 2 cas |
| `mhgp10_fp_environment` | gate;regression | refus sous arrondi dirigé ; sorties identiques sous `FE_TONEAREST` | 4 modes |
| `mhgp10_fast_math_flags` (`cmake -P`) | gate | `-Ofast` et `CMAKE_CXX_FLAGS_RELEASE=-ffast-math` refusés | 2 cas |
| `mhgp10_morton_compat` | gate;fast | `morton96 == morton3` sur u21 | 4 000 512 cas ou échantillon borné gravé |

### 3.4 Mutants

`level_at_most` strict ; rang cover sans `+1` ; garde des boules retirée ; garde décalée d'un ;
contrôle `FE_TONEAREST` retiré ; test CMake des drapeaux retiré ; fixture de registre inversée.
Les gardes de très grandes tailles passent par des helpers purs testés sur des tailles
virtuelles, comme `rank_search`.

### 3.5 Différentiels et critères de sortie

- Catalogue, tour et tête identiques à T0 (V5–V7 rejoués).
- Portes vertes en Release, ASan+UBSan et TSan pour les parties parallèles ; mutants tués.
- `check_docs` : 0 erreur dans le corpus actif v10, aucune nouvelle ailleurs.
- Reçu immuable ; PASSATION.

## 4. T2 — Tête : condensation des départs de points

### 4.1 Objet

Corriger TT1 : un départ de points directement attachés est aussi une division. Dès qu'une
cohorte de départs laisse moins de `mcs` unités, les points restants quittent la branche au
même niveau. La hiérarchie FULL n'est pas touchée. **Les sorties de tête changent
volontairement.**

### 4.2 Préalable

Rejouer d'abord, sur le binaire de T1, les reçus de l'auditeur :
`receipts/audit_continu_20260929/point_condensation_20260930/` (paquet natif),
`point_condensation_cover_r2_20260930/`, et
`receipts/audit_independant_20260930/point_condensation_followup/recheck.json`.
Le défaut n'a pas été rejoué par l'audit géant.

### 4.3 Contenu et fichiers

| Fichier | Changement |
| --- | --- |
| `src/head/head.{hpp,cpp}` | cohortes de départ par rang exact (pas par double) ; masse active jugée à chaque cohorte ; stabilité recalculée |
| `tests/head/test_point_departures.py` (nouveau) | contre-exemple du reçu : A (β = 1, 4, 9), B (β = 16), R (β = 25), C (β = 100), racine (β = 1 600) ; version ×3 à mcs5 |
| `tests/head/test_condensation_vs_sklearn.py` | cas de départs ajoutés ; sklearn 1.9.1 sur l'ultramétrique équivalente (jamais de réimplémentation d'HDBSCAN) |

### 4.4 Portes, mutants, différentiels

| Élément | Détail |
| --- | --- |
| Porte causale `mhgp10_head_point_departures` | stabilités exactes en Fraction (A 11/15, B 1/10, R 1, A+B 5/6 à z1/mcs2) ; sélection EOM R/C racine exclue ; normal et `-O` |
| Porte sklearn étendue | égalité des étiquettes sur les ultramétriques équivalentes |
| Mutants | contrôle `mcs` aux seules divisions géométriques (code actuel) ; cohorte par double au lieu du rang exact (tué par une fixture de collision) ; seuil `<` au lieu de `<=` (tué par une fixture d'égalité) |
| Différentiels | catalogue et tour identiques à T1 ; `head_diff` : différences seulement là où une cohorte franchit `mcs`, comptées et publiées ; lots A et C rejoués en tête seule sur les tours archivées, écart publié |

### 4.5 Critères de sortie

Contre-exemples de l'auditeur corrigés ; égalité sklearn ; comptes publiés ; contre-exemple
gravé au registre ; erratum si une décision historique change (reçus non réécrits) ; reçu
et PASSATION.

## 5. T3 — Précision : palier u24

### 5.1 Objet et décision

Décision utilisateur du 30 septembre (`408d1ffe4`) : grille u32 à pas décimal exact, portée
par paliers u24 puis u32 complet ; refus explicite hors domaine ; float32 natif non
développé. La lentille précision recommandait un premier palier B21 ; son vérificateur le
classe incertain (PR1). Le plan garde son contenu comme **étape interne P1a** : le refus
public reste u18 jusqu'à la fin de P1b, sauf accord pour un palier public B21.

Pourquoi deux étapes : à B21, seuls `side`, `side_key`, `orient_center`, `Level` et la marge
cèdent. Les qualifier d'abord isole les erreurs arithmétiques des changements de repère,
d'index et de filtre de P1b.

### 5.2 P1a — arithmétique suffisante pour B21 (interne)

| Fichier | Changement | Lignes |
| --- | --- | --- |
| `src/core/types.hpp` | `kEngineMaxBits` (palier servi) ; `kCoordinateBits = 18` reste le défaut | — |
| `src/arith/wide.hpp` | `[[nodiscard]]` sur `add`, `sub`, `resize` ; `from_u64` ; `Wide<1>::from_u128` interdit ; `to_double(const Wide<L>&)` générique, borne prouvée en commentaire | PR6 |
| `src/arith/geometry.{hpp,cpp}` | `Level { I192 num; I192 den; }` ; `side`/`side_key` dispatchés (voie i128 si les magnitudes réelles tiennent, sinon Wide3) ; `orient_center` vers `orient_center_wide` ; `level4` : D² par `mul` puis `resize` vérifié, somme des N_i² vérifiée, `Reason::arith_guard` ; `compare` en Wide6 ; `level3` : `resize` vérifié | PR2, PR3, PR4 |
| `src/cloud/site_tree.{hpp,cpp}` | `filter_margin(bits) = max(0,02 ; 5·2^(2B−49))` sous garde `FE_TONEAREST` (réparation de T0) | PR5 |
| `src/catalogue/generator.cpp` | refus `bits > kEngineMaxBits` ; drapeau par feuille `E_feuille < 2^19` dans `judge` ; `strictly_inside_tetra` dispatché ; compteurs `side_wide`, `orient_wide` | PR3, PR4 |
| `src/tower/tower.cpp` | marge selon `cloud.bits` ; `level_at_most` : comparaison directe `num` contre `e·den` en Wide4, sans raccourci (après AT1) ; clés Wide3 dans `jump_exact` si besoin | PR8 |
| `cli/*.cpp` | `--bits=N` et `--arith`, `--filter` sous `MHGP10_TEST_HOOKS` seulement en P1a | — |
| `bench/precision/prepare_grid.py`, `check_manifest.py` (nouveaux) | port épinglé (sha256) de `morsehgp3D_v8/bench/prepare_lidar_precision.py` ; masque de retours par IDs bruts ; manifeste v10 : h rationnel, origine signée, bits déclarés et effectifs, hashes, cartes d'IDs | PR12 |
| `tests/unit/precision_b21.cpp`, `tests/regression/test_similarity.py`, `test_modes.py` (nouveaux) ; oracles | fixtures et portes ci-dessous | — |
| commentaires `tower.cpp:443-447`, `cloud.cpp:30, :38` | corrigés | PR15 |

### 5.3 P1b — palier u24

| Fichier | Changement | Lignes |
| --- | --- | --- |
| `src/cloud/cloud.{hpp,cpp}` | clés Morton96 (u128) si bits > 21 ; regroupement par égalité xyz ; domaine mesuré privé posé par la fabrique | PR9, PR11 |
| `src/catalogue/generator.cpp` | T6 recadré : `lx2` relatif à `Q.lo`, lemme Z et dominance recadrés ; D-loc dispatché par nœud (i64 si L ≤ 2^29, sinon i128) ; `center_in_box` en `(Q.lo − 64a)·D <= 64N < (Q.hi − 64a)·D`, puis dispatch ; `static_assert` de la ligne 33 revu (il autorise aujourd'hui B ≤ 23) | — |
| `src/arith/geometry.{hpp,cpp}` | `num` en Wide4 (202 bits) ; `compare` en Wide7 | PR2 |
| `src/cloud/site_tree.cpp`, `src/tower/tower.cpp` | filtre relatif certifié (prototype `relative_filter` de l'auditeur) : descripteur par requête, intervalles extérieurs, AMBIGU → exact, boîtes → minorant, `nearest` par bornes hautes ; refus hors `FE_TONEAREST` et sous FTZ/DAZ ; remplace `filter_margin` | PR5 |
| `src/tower/tower.cpp`, `src/points/dendrogram.hpp` | rang dense exact des niveaux (catalogue et K-NN) publié ; la table double reste une vue métrique | TT6 |
| `src/head/head.cpp` | garde M·λmax exprimée en unité physique (β_phys = h²·β_grille) | PR7 |
| catalogue, tour, CLI | refus levé à bits ≤ 24 ; bits > 24 refusés explicitement | PR1 |

### 5.4 Fixtures gravées

| Id | Contenu | Attendu |
| --- | --- | --- |
| FX-T21 | tétraèdre (0,0,0) (M,M,0) (M,0,M) (0,M,M), M = 2^21−1 | niveau 3M²/4 = 13194126950403/4 ; num ≠ 0 ; den exact |
| FX-E21 | triangle (0,0,0) (M,M,0) (M,0,M), requête (0,M,M) | côté +1 |
| FX-O21 | tétraèdre de FX-T21, quatre faces | orientations [1, −1, 1, −1] ; UBSan propre |
| FX-D22 | tétraèdre M = 2^21 (B22) | P1a : refus ; P1b : den = 2^130 exact, jamais 0 |
| FX-O22 | (1048575,4194303,0) (1048575,2097151,3145727) (3145727,2097151,0) (102441,125431,204948) | `strictly_inside_tetra` = 1 |
| FX-B24 | tétraèdre régulier M = 2^24−1 | niveau 844424829468675/4 exact |
| FX-F18 | trois supports lointains u18 et G1 | coquilles complètes, intérieur vide |
| FX-L | N = 2^200, D = 2^193, e = 1 ; tétraèdre B24 (12M⁸, 16M⁶), e = 2^45 | `level_at_most` faux |
| FX-M | formule de marge, B = 1..21 | `filter_margin(B) >= 2·borne(B)`, recalculée indépendamment ; 0,02 si B ≤ 20 |
| FX-Mo | (0,0,0) et (2^21,0,0) à B22 | deux sites distincts |

### 5.5 Portes

| Porte | Étape | Attendu | Plancher |
| --- | --- | --- | --- |
| `mhgp10_precision_b21` (+ variante sanitize) | P1a | code 0, `precision_b21_ok` | toutes les fixtures FX-* de l'étape |
| `mhgp10_similarity_b21` | P1a | x → 8x : catalogue (coordonnées normalisées /8) et tour identiques ; niveaux ×64 exacts | 3 trames K5 et K10 ; nuages synthétiques couvrant tout le cube avec `side_wide`, `orient_wide`, `den_wide` > 0 (PR13) |
| `mhgp10_similarity_b24` | P1b | x → 64x (B18 → B24) : même critère | idem, Morton96 exercé |
| `mhgp10_arith_modes` | P1a, P1b | `--arith=wide` ≡ dispatch ; `--arith=narrow` refuse FX-T21 (`arith_guard`) | `side_wide > 0` en mode wide |
| `mhgp10_filter_modes` | P1a, P1b | `--filter=exact` ≡ `on` | au moins 10^6 décisions filtrées |
| `mhgp10_fp_environment` (de T1) | P1a, P1b | refus sous arrondi dirigé | 4 modes |
| `mhgp10_oracle_b21`, `mhgp10_oracle_b24` | P1a, P1b | catalogue et tour égaux à Γ_k (`reference/hgp10_ref.py`) | au moins 20 nuages n ≤ 12 ; extrêmes 0 et 2^B−1 ; cosphériques ; nuages épars aléatoires |
| `mhgp10_refusal_bits` | P1a, P1b | bits hors palier → code 2 et raison ; reste de fichier → 2 | — |
| `mhgp10_u18_digest` | P1a, P1b | dumps 1 mm des trois trames K5 identiques à la sortie de T2 | empreintes épinglées |
| 13 portes existantes + portes de T0–T2 | toutes | inchangées | inchangés |

La similitude voit les erreurs qui brisent l'équivariance (débordement, troncature, filtre
lié à l'échelle absolue). Elle ne voit pas une formule fausse de façon covariante : le
digest u18 et l'oracle couvrent ce cas.

### 5.6 Mutants

| Mutant | Tué par |
| --- | --- |
| `den_u128` (D² en u128) | FX-T21, oracle |
| `side_narrow_only` | FX-E21, oracle, sanitize |
| `orient_narrow_only` | sanitize à B21 ; FX-O22 dès B22 |
| `margin_fixed_0p02` | FX-M seulement (non observable en sortie à B21 : déclaré) |
| `margin_zero` | `mhgp10_filter_modes` |
| `fe_guard_removed` | `mhgp10_fp_environment` |
| `far_center_unguarded` | FX-F18 |
| `level_at_most_shortcut` | FX-L (B24) |
| `level_at_most_strict` | invariant d'attache (T1) |
| `refusal_off_by_one` (palier + 1 admis) | `mhgp10_refusal_bits` |
| `level3_den_2ww` (erreur covariante) | digest u18, oracle ; invisible à la similitude |
| `morton64_kept` | FX-Mo |
| `t6_absolute` (repère absolu) | oracle B24 aux extrêmes, sanitize |

Un `resize` non vérifié ne compile plus sous `[[nodiscard]]` et `-Werror` : garde statique.

### 5.7 Différentiels et mesures

- u18 identique à la sortie de T2 : `differentiel_j2c` 10/10, dumps de tour V6, `head_diff`
  18/18.
- Mesures sans porte, 8 cœurs locaux : trois trames sans sol refaites depuis les float32
  bruts et les masques v8 (`kept_return_ids.u32le`), jamais par ×10 des entiers à 1 mm ;
  0,1 mm (B21) et 0,01 mm (B24) ; K5 et K10. Publier temps, RSS, `side_wide`, `orient_wide`,
  `den_wide`, décisions exactes, taux de repli du filtre (`path_counts()`), surcoût u18 du
  dispatch et de `Level` (56 → 64 → 72 o par niveau et par `Rec`).
- Les parts de voies courtes annoncées (100 % à 0,1 mm, 96–99,7 % à 0,01 mm) sont des
  extrapolations : les mesurer.

### 5.8 Critères de sortie

1. Portes de § 5.5 vertes en Release ; `precision_b21` vert sous sanitize.
2. Mutants tués, ou déclarés non observables avec la porte qui les couvre.
3. Sorties u18 identiques à la sortie de T2.
4. Trois trames traitées de bout en bout à 0,01 mm (catalogue, tour, attaches, tête),
   compteurs et temps publiés.
5. Doc du palier, reçu immuable, PASSATION dans le même commit. Pas de G4 dans T3.

### 5.9 Risques

| Risque | Parade |
| --- | --- |
| surcoût du dispatch dans `judge` | drapeau par feuille ; test par appel hors feuille courte seulement |
| `Level` +8 o puis +16 o par niveau et par `Rec` | mesuré ; compaction sans octet de signe plus tard |
| similitude lente à K10 (20–35 s par trame sur 3 fils) | label `precision`, hors `fast` |
| arbre de boîtes différent sous ×8 (milieux entiers, seuil absolu) | l'objet ne change pas ; un refus `wide_leaf` pourrait différer : le signaler |
| fixtures trop régulières | nuages épars aléatoires sur tout le cube à l'oracle |
| vert par vacuité sur trames réelles | planchers de voies larges sur nuages étendus (PR13) |

## 6. T4 — Campagne frontière

### 6.1 Préalables

1. T0 et T2 livrés : tête corrigée avant toute comparaison (TT1). T1 : invariant d'attache.
2. Fondations versionnées sous `morsehgp3D_v10/bench/frontier/` : `frontier_core.py`,
   `arms.py`, `design_gate.py`, juge, `export_frontier.cpp` avec cible CMake optionnelle ;
   CTest normal et `-O` pour les tests structurel, natif et de conception (PT6).
3. Préenregistrement et amendement 01 versionnés ; **amendement 02** daté et scellé avant le
   premier bras : pin moteur dérivé du binaire (FR6) ; bras et η figés ; porte étendue ;
   diagnostics ; D1 par coupes échantillonnées à l'échelle (FR11) ; graine du test scellé
   dérivée d'une valeur publique future, par exemple un commit postérieur au gel (FR11) ;
   ε en millimètres (FR12) ; contraintes gardées (dénominateur fixe, futurs inclus,
   continuations sans double compte, masses actives + réserves = n).
4. Générateur d'échelle par `dev_quotas.py` ; retours, sites et doublons publiés (FR10).
5. Banc : tous les témoins validés (FR8) ; marge publiée comme diagnostic (FR9) ; fixture
   de bord de bande avec son saut (FR2) ; « K fixé » écrit dans la doc (FR7).
6. Les bras issus du chantier de recherche « FULL → hiérarchie laminaire »
   (`build/v10-verrou-points`) entrent par amendement, avant le gel.

### 6.2 Bras

| Rôle | Bras | Paramètres |
| --- | --- | --- |
| contrôle stable | A0 core | — |
| contrôle précoce | A1 cover | — |
| candidat | B_η^min (antichaîne, LCA par deux extrêmes d'Euler) | η ∈ {1/32, 1/8} |
| contrôle d'implémentation | B_η (HEAD) | mêmes η |
| K2 seulement | A2 | η préenregistrés {0, 1/4, 1} |
| témoins négatifs | A3, A4 | attendus en échec |
| rapportés, non interprétés | A5 (= B_0), A6 | — |
| diagnostics continus | masse fractionnaire 1/β et uniforme ; durée couverte | pas des bras |

### 6.3 Porte de conception étendue

G1–G8 et GB actuels ; quasi-égalité K3 (A5/A6/B_0 sautent ; B_{1/32}, B_{1/8} stables) ; bord
de bande (B_η et antichaîne sautent ; A1/A5 stables) ; ligne 0,1,2,6,9 (croisement K2/K3 à
β = 9) ; entrées internes K3/K5 pour B_η et B_η^min ; invariance par l'univers (antichaîne
invariante, B_η non). Mutants de `cover_band.py` : seuil non carré, égalité de bord exclue,
LCA ignoré, date sans `max(α²)`. Le test structurel les tue tous ; le test natif n'en tue
que deux (date, LCA) : la fixture de bord doit tuer les deux autres.

### 6.4 Diagnostics fixés d'avance

D1 couverture à l'entrée et emboîtement ; **D2, décisif : rappel frontière avant la première
fusion parasite** ; D2bis/D2ter ; D3 avec le compte t > D_K ; D4 ; D5 absolu (2ε) **et** à
l'échelle des amas condensés ; D5bis sans perturbation (ρ, proches du seuil, saut potentiel) ;
D6 à 8 000 / 16 000 / 32 000 avec pentes ; D7 EOM z = 1/2 contre HDBSCAN scikit-learn
(secondaire).

### 6.5 Scènes

| Jeu | Tailles | K | Diagnostics |
| --- | --- | --- | --- |
| portes | 3 à 7 sites | 2, 3, 5 | porte |
| préenregistrement dev | 600 à 1 500 | 2, 5 | D1–D5, D7 |
| bhc à quotas | 8 000 / 16 000 / 32 000 | 2, 5 | D1–D6 |
| entrées d'échelle synthétiques | 8 000 / 16 000 / 32 000 | 2, 5 | D1, D3, D5bis, D6 |
| LiDAR : découpes, demi-trames, trames | 8 000 à ≈ 46 000 | 2, 5 ; 10 à 8 000 | D1, D3, D5bis, D6 |
| test scellé, après le gel | 8 000 / 16 000 / 32 000 + trames tenues à part | 2, 5 | D1–D5, une seule exécution |

Coût mesuré : export complet à 32 000 points K5 = 532,6 Mo, chargement 57 s, pic 5,6 Go.
Faisable ; un export allégé reste une optimisation.

### 6.6 Règle de décision et sortie

- Règles R1–R6 du préenregistrement. Un port natif n'est envisagé que si un bras domine A1
  selon R4 sur la majorité des scènes dev **et** sur le test scellé, sans perte de
  couverture. Sinon, A1 (participation précoce) et A0 (stabilité) restent les sorties
  déclarées. Ne pas élargir η sans D2.
- Différentiels : moteur inchangé ; références core identiques au pin.
- Sortie : reçu de campagne (commit, sha256 des binaires, empreintes du préenregistrement et
  des amendements, tous les diagnostics) ; décision publiée selon la règle ; registre mis à
  jour si un contre-exemple apparaît ; PASSATION.

## 7. Suites, hors plan immédiat

| Suite | Contenu | Condition |
| --- | --- | --- |
| T5 palier u32 complet | K-NN u128 (`kth_distance`, `within`, `box_d2_exact`, `point_level`, seuil de `RankIndex`, sentinelle), `nearest` K-NN entier ; centres Wide3, côtés Wide4, orientation Wide4, niveaux Wide5/Wide4, comparateur 8 mots avec retenue ; X2 et lemme Z en i128 ; rang exact obligatoire ; similitude ×2^14 ; mesures à 1 µm (B28) | après T3 |
| T6 régime massif | admission mémoire par formule dès l'énumération (MM1) ; garde des boules budgétée (MM2) ; préflight de l'atlas (MM3) ; note de décision (MM4) ; formules republiées (MM6) ; mesures sans `--repeat` ≥ 2 (MM7) ; segments à u32 local et u64 aux frontières (MM9) ; certificat Q×Z mesuré (MM10) ; runner S5 (BN4) | selon la question 5 |
| CI v10 | job minimal : Release, `ctest -L fast`, sans GCP, `ubuntu-24.04` épinglé (CI3) | après T0 |
| G4 | une session par palier après ses portes CPU ; sonde CUDA (CU2) ; scripts gardés, arrêt certifié | autorisation déjà donnée ; jamais avant les portes CPU |
| Performance | bornes CSR avant d'intégrer le prototype ordre/tête (PF1) ; crédit de moments de groupe pour élaguer les boîtes q3/q4 (proposition de l'auditeur, à mesurer avant tout port) ; aucune revendication de 100 ms (PF2) | après T3 |
| float32 sans perte | non développé (décision du 30 septembre) ; demanderait un palier dyadique B45–B49, au-delà de u32 | décision utilisateur |

## 8. Correspondance tranches → lignes du suivi

| Tranche | Lignes |
| --- | --- |
| T0 | RC1–RC10, SO1, SO2, CL1, CL2, CL3, TT2, TT3, TT4, TT5, PL1, PL2, ST1, JG1, JG2, JG3, BN1, BN2, BN3, DC1, DC4, DC5, DC6, ER1, PT9 ; en partie RG1 (`faits_math`), MM1 (`bad_alloc`), MM2 (garde R2), ST2 |
| T1 | RG1, AT1, AT2, PT1, PT2, PT3, PT4, PT5, PT7, FP1, CU1, MM5, MM8, PR9 (porte), PR10 (garde), DC2, DC3, CI1, CI2, CI3, CI4, CI5 ; en partie MM1 (en-tête), MM2 (compteur par feuille) |
| T2 | TT1 |
| T3 | PR1, PR2, PR3, PR4, PR5, PR6, PR7, PR8, PR9 (Morton96), PR11, PR12, PR13, PR14, PR15, TT6, ST2 |
| T4 | PT6, FR1–FR12 |
| Suites | PR10 (u128), MM3, MM4, MM6, MM7, MM9, MM10, BN4, CU2, PF1, PF2, PT8 ; MM1 (admission) |
