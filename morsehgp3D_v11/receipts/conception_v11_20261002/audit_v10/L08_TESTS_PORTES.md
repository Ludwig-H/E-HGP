# L08 — Tests, portes et oracles de `morsehgp3D_v10` : audit pour la conception de la v11

```text
phase=exploration_v11_hors_registre (audit de la v10)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_tests_portes_oracles
public_status=not_claimed
GCP non utilisé
```

Rédigé le 2 octobre 2026 ; heures lues par `date -u` (début des travaux 07 h 03 UTC, heure de clôture en fin de
rapport). Sujet : `morsehgp3D_v10` au commit `afb081774`. Le worktree de lecture est depuis passé à `52687f8e5` (ouverture
de la v11) ; `git diff --stat afb081774 HEAD` est vide sur `src`, `tests`, `cmake`, `reference`, `cli`, `bench` et
`CMakeLists.txt` de la v10, dont le dernier commit de code est `4b7d70422`. Ce rapport ne promeut aucun statut. Un constat est un fait : il dit s'il est **lu**, **exécuté**
ou **mesuré**, et où.

## 0. Résumé

- **La suite du dépôt est verte et stable** : 13 portes sur 13 en GCC 13.3 Release, en Clang 18.1 Release et sous
  ASan + UBSan ; référence Python 4 tests sur 4. Mêmes comptes de contrôles sous les deux compilateurs.
- **Mais la porte qui fonde l'exactitude de la tour ne compare pas la forêt.** Le juge T2 de HEAD compte les
  composantes et compare la partition des points entrés ; il accepte 1 978 dumps faux sur 2 073 que j'ai fabriqués à
  partir de dumps vrais (échange de deux naissances entre deux fusions : 649 sur 649 ; plateau binarisé : 149 sur 149 ;
  image verticale descendante : 779 sur 779). La phrase de `docs/SPEC_V10.md` § 6 (« forêts […] égales à l'oracle »)
  n'est donc pas établie par la porte. Le juge corrigé existe, hors dépôt (raccord R2) : il refuse ces 2 073 dumps,
  et il accepte la forêt de HEAD sur les 73 entrées de la porte. Le défaut est dans la porte, pas dans la tour.
- **Aucun mutant dans le dépôt, et des mutants simples survivent** aux 13 portes : attache `core` rendue stricte
  (109 attaches changées sur une trame du contrat, étiquettes changées sur une fixture de 5 points), rang de l'entrée
  `cover`, image verticale des naissances, exposant `z` ignoré par la tête, refus de tout K ≥ 10.
- **K = 10 n'est jugé nulle part** (oracle : K ≤ 5, 12 sites au plus pour la tour, 22 pour le catalogue) et n'est
  exécuté par aucune porte sur le chemin produit. Le mode chronométré (tour FULL sans attaches) ne peut pas être
  exporté : `--no-points --dump` s'arrête par `SIGSEGV`. **Aucun juge à l'échelle** (8 000, 16 000, 32 000 points, trames) :
  ceux de la conception (`EMST`, Euler, recensement, descentes, fusions) n'ont pas été livrés.
- **Ce que j'ai ajouté comme preuves indépendantes, toutes positives pour le moteur v10** : oracles rejoués à pleine
  magnitude u18 (336 contrôles, 0 écart), aux ordres 7, 8 et 10 (voir § 5.4), arbre couvrant exact contre l'ordre 1
  à 8 000, 16 000, 32 000 points et sur les trois trames, **invariant d'Euler par ordre jusqu'à K = 10 sur les trois
  trames du contrat** (coquilles étendues comprises), identité des dumps selon le nombre de fils et sous
  permutation. Ces juges tiennent en 60 à 110 lignes de Python : ce sont les premières portes d'échelle à écrire
  en v11.
- **Leçon de méthode** : l'appareil de test le plus fort de la v10 (82 portes, juges à témoins, 425 mutants) vit dans
  un dépôt jetable jamais importé, et ses campagnes visent la périphérie (CLI, pool, bancs, options flottantes), pas
  le cœur géométrique. La v11 doit livrer chaque porte dans le commit du code qu'elle garde, et dépenser ses mutants
  d'abord sur le catalogue, la tour et l'attache des points.

## 1. Périmètre lu

| Objet | Chemin | Lecture |
| --- | --- | --- |
| Portes CMake | `morsehgp3D_v10/CMakeLists.txt` (135 l.), `cmake/gates.cmake` (25 l.), `cmake/run_expect.cmake` (26 l.) | intégrale |
| Portes C++ | `tests/unit/unit_main.cpp` (405 l.), `tests/unit/rank_search.cpp` (71 l.), `tests/unit/grid32_primitives.cpp` (211 l.) | intégrale |
| Portes Python | `tests/oracle/test_catalogue_oracle.py` (161 l.), `tests/oracle/test_tower_oracle.py` (188 l.), `tests/head/test_condensation_vs_sklearn.py`, `tests/points/test_cover_entry.py`, `tests/points/test_cover_band.py`, `tests/points/test_dev_quotas.py`, `tests/regression/*.py` (4 fichiers) | intégrale |
| Témoin hors produit | `tests/head/mreach.cpp`, `mreach.hpp`, `mreach_cluster.cpp` | intégrale |
| Référence exacte | `reference/hgp10_ref.py` (431 l.), `reference/test_ref.py` (67 l.) | intégrale |
| Surface jugée | `cli/mhgp10_catalogue.cpp`, `cli/mhgp10_tower.cpp`, `cli/mhgp10_cluster.cpp`, `src/core/reasons.def`, `src/core/status.hpp`, `src/tower/tower.hpp`, `src/points/dendrogram.*`, `src/head/head.*`, `src/arith/wide.hpp`, `src/sched/sort.hpp` | intégrale |
| Moteur | `src/tower/tower.cpp` (attaches, verticales, Kruskal, descente, `point_dendrogram`), `src/catalogue/generator.cpp` (admission, tailles de feuille) | parties citées |
| Documents | `PASSATION.md`, `docs/SPEC_V10.md`, `docs/conception/CONCEPTION_V10.md` § 11 | parties citées |
| Pistes (jamais des preuves) | `receipts/audit_geant_developpeur_20260930/` (rapport et suivi), `audits/AUDIT_ETAT_COURANT.md`, `audits/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md`, notes de mémoire du développeur | survol ciblé |
| Hors dépôt (lecture seule) | `/workspaces/E-HGP/build/v10-integration-r2/` (notes, journaux, arbre `src` au commit `865f5e6`), `/workspaces/E-HGP/build/v10-giant-audit/moteur_livre/`, `/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/`, `/workspaces/E-HGP/build/workflows/EXIGENCES_TESTS_UTILISATEUR.md` | ciblée |
| Cadre v11 déjà posé | `morsehgp3D_v11/README.md`, `docs/ARCHITECTURE.md` § 5 et § 6, `docs/PROVENANCE.md` (commit `52687f8e5`) | ciblée |

Non lu en entier : `src/catalogue/generator.cpp`, `src/cloud/site_tree.cpp`, `src/arith/geometry.*` (objet d'autres
lentilles) ; les 4 828 fichiers de `receipts/` ; les bancs `bench/synthetic` au-delà de leurs imports.

## 2. Méthode

1. Copie de lecture par `git archive afb081774` sous `/tmp/v11-audit/l08_tests_portes/src` (90 fichiers ; empreinte
   de l'arbre de code identique à celle du worktree : `cff024bd…9929`). Rien n'est écrit dans le worktree ni dans les
   dossiers privés ; aucune commande git mutante ; aucune commande GCP.
2. Trois builds hors source (GCC 13.3 Release, Clang 18.1 Release, GCC ASan + UBSan), zéro avertissement, puis
   `ctest` complet en série pour chacun (plus un build TSan de deux binaires) ; référence par `python3 -m unittest discover`. Interpréteur retenu par
   CMake : `/home/codespace/.python/current/bin/python3` (3.12.1, numpy 2.5.3, scipy 1.18.1, scikit-learn 1.9.1).
3. Lecture intégrale des portes, puis **épreuves ciblées**, toutes rejouables depuis
   `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l08_tests_portes/scripts/` :
   strates réellement exercées par les oracles ; dumps faux présentés au juge ; dix mutants appliqués à des copies ;
   oracles rejoués hors de leurs strates (pleine magnitude, K = 7, 8, 10) ; juges d'échelle indépendants (arbre
   couvrant exact, Euler) ; identité selon les fils et sous permutation ; dépendances et vacuité.
4. Machine partagée de 8 cœurs, charge de 7 à 36 pendant tout l'audit : **les durées ci-dessous ne sont pas des
   mesures de performance**. Seuls les comptes et les verdicts font foi.

Vocabulaire : « porte » = CTest enregistré ; « juge » = programme qui décide d'un écart ; « oracle » = juge qui
*établit* la vérité par une voie indépendante ; « mutant » = défaut volontaire appliqué à une copie.

## 3. Résultats d'exécution (exécuté)

### 3.1 Suite CTest complète

13 tests enregistrés (`ctest -N`). Labels : `gate` 13, `regression` 6, `fast` 4, `points` 3, `oracle` 2, `head` 1.
Aucun label d'échelle, de trame, de mutant ni de juge.

| Build | Résultat | Durée totale (charge) | Journal |
| --- | --- | --- | --- |
| GCC 13.3 Release | **13 / 13**, code 0 | 483 s (charge 7 à 14) | `preuves_l08_tests_portes/journaux/ctest_gcc_release.txt` |
| Clang 18.1 Release | **13 / 13**, code 0 | 1 150 s (charge 15 à 28) | `…/journaux/ctest_clang_release.txt` |
| GCC ASan + UBSan (`-DMHGP10_SANITIZE=ON`) | **13 / 13**, code 0, 0 rapport de sanitizer | 2 523 s (charge 15 à 36) | `…/journaux/ctest_asan_ubsan.txt` |

Comptes publiés par les portes (identiques en GCC et en Clang) :

| # | Porte | Durée GCC | Compte publié | Nature |
| --- | --- | ---: | --- | --- |
| 1 | `mhgp10_unit` | 1,3 s | `wide_checks 100000`, `site_tree_checks 6000`, `site_tree_rational_checks 7155 shells 1099` | C++, juges internes |
| 2 | `mhgp10_rank_search` | 0,01 s | `rank_search_checks 36047` | C++ |
| 3 | `mhgp10_grid32_primitives` | 0,1 s | `grille32_checks 212684` | C++, brique non raccordée au moteur |
| 4 | `mhgp10_cover_band_structural` | 0,1 s | 5 tests `unittest` | Python nu, banc `bench/frontier` |
| 5 | `mhgp10_dev_quotas` | 0,1 s | 642 allocations, 51 886 contrôles, 19 refus | Python nu, banc `bench/frontier` |
| 6 | `mhgp10_catalogue_oracle` | 179 s | `catalogue_oracle_checks 161 fails 0 balls 14132` | oracle Python nu |
| 7 | `mhgp10_tower_oracle` | 216 s | `tower_oracle_checks 73 fails 0 cuts 23444` | oracle Python nu |
| 8 | `mhgp10_head_condensation_vs_sklearn` | 13 s | `hdbscan_checks 360 mismatches 0` | numpy + scikit-learn |
| 9 | `mhgp10_regression_level_collision` | 11 s | 509 710 et 502 080 niveaux publiés, statut ok | numpy (scène de 8 000 points) |
| 10 | `mhgp10_points_cover` | 15 s | `controles 4584 ecarts 0` | numpy |
| 11 | `mhgp10_regression_batch_equivalence` | 45 s | 136 contrôles | numpy + scipy (import) |
| 12 | `mhgp10_regression_mreach_border` | 3 s | 44 contrôles ; couverture bord ≥ cœur dans 98 % des cas | numpy + scipy (import) |
| 13 | `mhgp10_regression_multiplicity_refusal` | 0,1 s | `multiplicity_refusal_ok` | Python nu |

### 3.2 Référence exacte

`python3 -m unittest discover -s morsehgp3D_v10/reference -p 'test_*.py'` : **4 tests, OK, 489,8 s** (460 s de CPU
utilisateur, 18 Mio résidents). Elle n'est pas enregistrée dans CTest.

### 3.3 Tests non enregistrés ou conditionnels

- **Sans Python, 10 portes sur 13 disparaissent sans bruit.** `find_package(Python3 COMPONENTS Interpreter)` n'est
  pas `REQUIRED` (`CMakeLists.txt:87-88`). Configuré avec `-DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE` : code 0,
  aucun avertissement, `ctest -N` liste 3 tests (exécuté).
- `tests/points/test_cover_band_native.py` n'est enregistré nulle part ; il exige `--foundations` et `--exporter`,
  absents du build (lu).
- `reference/test_ref.py` : hors CTest (lu).
- Aucun workflow de `.github/workflows/` ne cite la v10 (`git grep` vide ; les v7, v8 et v9 ont le leur).
- Aucune porte ne dépend de Boost, de GMP ni de CUDA.

### 3.4 Stabilité

Aucun échec intermittent observé : trois suites complètes, `mhgp10_unit` rejoué 15 fois sous charge 16 (15 succès),
et les 11 portes hors oracles rejouées une fois par mutant. Le seul précédent connu est la course du pool du
29 septembre (une fois sur environ 2 400 exécutions, corrigée en `8e3b76245`, porte de stress ajoutée) ; je ne l'ai
pas revue. ThreadSanitizer : `mhgp10_unit` (dont le stress du pool à 4 et 8 fils) et `mhgp10_tower` (2 000 points uniformes, K = 5, 4 fils : catalogue, tour FULL, attaches) construits avec `-DMHGP10_TSAN=ON` et lancés par `setarch -R` : codes 0, aucun rapport. La suite complète n'a pas été jouée sous TSan.

## 4. Conception des portes : constats

Chaque constat porte un identifiant `L08_TESTS_PORTES-nn`, une gravité (bloquant : rend faux ou invalide un résultat
ou un contrat ; majeur : à traiter dans la conception v11 ; mineur ; info), son mode d'établissement et sa preuve.

### 4.1 Mécanique des portes

**L08_TESTS_PORTES-01 (info, exécuté) — `run_expect.cmake` est solide et se porte tel quel.** Onze cas joués à la
main contre `cmake/run_expect.cmake` : code attendu exact (0 et 2), code faux refusé, `SIGSEGV` refusé même si l'on
attend « 139 », binaire absent refusé, ligne attendue exigée *exacte* (une sous-chaîne est refusée), lue sur la
sortie standard de la *même* exécution (une ligne écrite sur l'erreur standard est refusée). Aucune porte du dépôt ne
teste ce script lui-même : la v11 doit graver ces cas (`mhgp11_gate_selftest`).

**L08_TESTS_PORTES-02 (mineur, lu et exécuté) — codes de sortie : la convention existe, les portes Python la
tiennent mal.**
- Les 13 portes attendent le code 0. Aucune porte négative (refus attendu, code 2 ; invariant, code 3 ; mutant tué,
  code 4) n'est enregistrée. Le code 4 n'existe pas en v10 (`cmake/gates.cmake:5-6`).
- Deux portes passent par `add_test` nu, hors `run_expect` (`CMakeLists.txt:89-94`).
- Toute exception Python rend 1, le code du « désaccord d'un juge » : module absent, binaire tué (`check=True`
  dans `tests/head/test_condensation_vs_sklearn.py:59-60`), fichier manquant. Joué avec `/usr/bin/python3` (sans
  numpy) : 5 portes sur 13 rendent 1 sur `ModuleNotFoundError` (exécuté) ; une dépendance absente a le même code
  qu'une tour fausse.
- Les sorties d'ASan et d'UBSan (code 1 par défaut) se confondent aussi avec ce code.

**L08_TESTS_PORTES-03 (mineur, exécuté) — planchers : présents dans les portes C++ et les deux oracles, absents ou
globaux ailleurs.**
- Bien : `mhgp10_unit` (planchers par section, code 3), `rank_search` (35 000), `grid32` (5 planchers), oracle
  catalogue (`checks ≥ 4 × nuages`, `balls ≥ 1000`), oracle tour (`cuts ≥ 500`), tête (`checks ≥ 300`),
  refus des multiplicités (témoin positif).
- **Vert par vacuité reproduit** : `tests/regression/test_level_collision.py:38-39` saute une variante dont le
  binaire répond « option inconnue ». Avec un faux `mhgp10_cluster` qui rend 2 et écrit « option inconnue », la porte
  rend **0** sans rien juger (exécuté ; `journaux/divers.txt`, § 4).
- Sans plancher : `test_cover_entry.py` (`:129`), `test_batch_equivalence.py` (`checks == 0` seulement, `:76`),
  `test_mreach_border.py` (`:62`, plus un critère statistique « 90 % des cas », `:58`).
- Le contrôle des verticales de l'oracle de tour compte ses contrôles puis les annule : `cuts += 0 * c`
  (`tests/oracle/test_tower_oracle.py:153`).
- Les planchers des oracles sont globaux. Aucun ne garantit une strate : coquilles étendues, multifusions à trois
  enfants ou plus, descentes à plusieurs pas, replis exacts, arbres de boîtes à plusieurs feuilles (§ 5.2).

**L08_TESTS_PORTES-04 (mineur, lu) — les « fixtures » des portes de régression sont régénérées, pas gravées.**
`test_level_collision`, `test_batch_equivalence` et `test_mreach_border` tirent leurs scènes de
`bench/synthetic/scenes.py` : `np.random.default_rng(seed)`, `rng.standard_normal`, `np.linalg.qr` (`:114`, `:270`),
puis quantification par `floor(x / h + 1/2)` (`:303`). La scène de 8 000 points qui portait la collision de niveaux
du 29 septembre dépend donc du flux de `Generator` (non garanti d'une version de numpy à l'autre), de LAPACK et de
l'arithmétique flottante de la machine. Aucune empreinte de la scène n'est contrôlée et aucun plancher ne compte les
collisions fusionnées. Contre-épreuve : le correctif retiré (mutant M08, § 5.5), la porte tombe sur cette machine (`mhgp10_regression_level_collision`) : la scène régénérée porte donc encore la collision ici. Rien ne le garantit sur une autre version de numpy ou une autre machine, et la porte ne le dirait pas.

**L08_TESTS_PORTES-05 (mineur, lu et exécuté) — dépendances et portée des labels.**
- numpy : 5 portes sur 13 ; scipy : 2 (import de `bench/synthetic/methods.py:8`) ; scikit-learn : 1. Aucune version
  n'est épinglée ni contrôlée par les portes (seuls les préenregistrements des bancs notent les versions).
- `ctest -L fast` = 4 portes, dont deux jugent des modules Python de banc ; **aucun oracle n'est `fast`**, alors que
  les deux oracles sont en Python nu. Sur la VM G4 (ni pip ni numpy) : la session 1 a joué `ctest -L gate` au commit
  `8b8d66f6e` — les deux oracles y sont **passés** (111 s et 124 s) et les quatre portes à numpy y ont « échoué » en
  0,05 s, d'où un statut de session `failed_remote` (`receipts/g4_session1_20260929/results/cmd/000_gates/stdout`) ;
  les sessions 2 à 5 n'ont plus joué que `ctest -L fast` (2 portes sur 2 à la session 4), après J2, J2c, l'assemblage
  parallèle et la correction du pool. Faute d'empreinte canonique publiée par les CLI (aucune occurrence de `digest`,
  `sha256` ou `merkle` dans `src` ni `cli`), rien ne relie une sortie chronométrée là-bas à une sortie jugée ici.
- `mhgp10_head_condensation_vs_sklearn` compare à scikit-learn 1.9.1 une égalité exacte de partitions. Or
  `sklearn.cluster._hdbscan.hdbscan._process_mst` trie les arêtes par `np.argsort` sans stabilité (lu dans le paquet
  installé) : à égalité de poids, l'ordre dépend de l'implémentation du tri, donc du processeur. La réponse du
  développeur du 2 octobre le mesure (« un étiquetage sur deux diffère à 2 000 points entre G4 et ici »,
  `audits/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md:148-151`). Je n'ai pas pu changer le tri de numpy sur
  cette machine (AVX2 est dans son socle) : la stabilité de cette porte d'une machine à l'autre est **non vérifiée**.

**L08_TESTS_PORTES-06 (majeur, lu et exécuté) — refus, garde-fous et arrêts par signal : presque rien n'est gardé, et
le mode du contrat ne s'exporte pas.**
- 23 raisons de refus déclarées (`src/core/reasons.def`). Provoquées par une porte : 4 (`empty_input`,
  `coordinate_out_of_domain`, `duplicate_point_id` dans `mhgp10_unit` ; `multiplicity_unsupported` par CLI).
  Jamais émises par le code : 6 (`k_out_of_range`, `device_unavailable`, `arith_guard`, `window_empty`,
  `nested_parallelism`, `leaf_unsplittable`). Émises mais jamais provoquées : 13, dont tous les invariants
  (`root_count`, `census_mismatch`, `vertical_naturality`, `rank_order`, `csr_bounds`, `descent_no_terminal`) et
  `shell_quotient_budget`.
- Les garde-fous du moteur sont pourtant réels et utiles : descente strictement décroissante, juge d'échantillon du
  recensement (1 boule sur 32, `tower.cpp:884`), « naissance absente de la table : catalogue incomplet »
  (`tower.cpp:985`), une racine par ordre (`tower.cpp:1114`), naturalité des verticales. Deux épreuves le montrent :
  48 points exactement cosphériques sont refusés proprement (`unsupported_degeneracy / shell_quotient_budget`,
  code 2, y compris à K = 1) ; un catalogue amputé fait tomber `root_count` (mutant M07) ou `census_mismatch`
  (mutant M10) sur une trame (§ 5.5). Aucune porte ne grave ces comportements.
- `validate()` du dendrogramme de points n'a aucun test négatif.
- La doctrine « un arrêt par signal n'est jamais admis » n'est gardée par aucune porte côté CLI. Rejoué (piste CL1
  de l'audit géant) : `mhgp10_tower … --repeat=0` rend 139 (`SIGSEGV`), `mhgp10_tower … --k=abc` rend 134
  (`SIGABRT`, `stoi` non rattrapé), `mhgp10_cluster` à K = 6 sur 5 points rend 139 ; `mhgp10_tower --k=11` sur
  5 points rend 0 et publie `"K":11`.
- **Le mode du contrat ne s'exporte pas.** `mhgp10_tower --no-points --dump=…` (tour FULL sans attaches, le mode
  chronométré sur G4) rend 139 sur 5 points comme sur une trame à K = 10 (là, après avoir écrit « status ok ») :
  l'écriture du dump lit `point_node`, vide sans attaches (`cli/mhgp10_tower.cpp:186-194`). Aucune porte ne joue cette
  combinaison ; la tour FULL seule n'a donc ni dump, ni empreinte, ni juge.

### 4.2 Couverture par module (lu ; « sous oracle » = jugé par une voie indépendante)

| Module (lignes) | Porte directe | Sous oracle | Trous constatés |
| --- | --- | --- | --- |
| `core` : statuts, tampons (309) | `mhgp10_unit`, 10 contrôles | — | budget mémoire ; 19 raisons sur 23 jamais provoquées |
| `sched/pool` (147) | `mhgp10_unit` : couverture, imbrication, 50 000 travaux courts à 4 et 8 fils | — | exceptions ; TSan hors CTest |
| `sched/sort` (66) | **aucune** | — | chemin parallèle (`n ≥ 8192`) jamais comparé à `std::sort` |
| `cloud/cloud` (110) | `mhgp10_unit` : permutation, doublons, 3 refus | — | `size_mismatch`, `parameter_out_of_range`, `index_overflow_u32` |
| `cloud/site_tree` (351) | `mhgp10_unit` : 13 155 requêtes contre force brute | oui (force brute entière) | centres lointains ; le juge des requêtes rationnelles emploie `geom::side_key` et `geom::center*`, non jugés eux-mêmes |
| `cloud/grid32_primitives` (59) | porte dédiée, 212 684 contrôles | oui | brique non raccordée au moteur |
| `arith/wide` (164) | `mhgp10_unit` : juge décimal, 100 000 contrôles | oui | opérandes de 127 bits au plus : comparaison, addition et produit à trois mots pleins non tirés |
| `arith/geometry` (244) | **aucune** | par les oracles, à 10 bits | bornes u18 de l'en-tête jamais approchées (fermé par ma contre-épreuve § 5.4) |
| `catalogue/*` (1 081) | oracle brut, 161 contrôles | oui, n ≤ 22, K ≤ 5 | K de 6 à 12 ; multiplicités ; feuilles stagnantes ; niveaux publiés non jugés ; ensembles I et U comparés sans ordre ni doublon |
| `tower/tower` (2 020), `rank_search` (37) | oracle Γ, 73 contrôles ; `rank_search` 36 047 | partiel : comptes et partitions, n ≤ 12, K ≤ 5, entrée `core` | forêt, verticales, plateaux, attache à l'égalité (§ 5.3) ; entrée `cover` ; replis exacts ; budget de coquille |
| `points/dendrogram` (69) | **aucune** | — | `validate()` sans test négatif ; niveaux en `double` |
| `head/head` (219) | scikit-learn sur le témoin, K = 1 et 2 | oui, hors de la forme des arbres de la tour | condensation des départs de points, z, `allow_single_cluster`, poids |
| `cli/*` (556) | refus des multiplicités | — | options et signaux (rejoué, L08-06) ; sorties écrasées (constat SO1 de l'audit géant, non rejoué) ; aucun export du mode `--no-points` |
| `bench/*` (Python) | `cover_band`, `dev_quotas` seulement | — | `decide`, `run_test`, `metrics`, `merge_sessions`, `scale_run` |
| `reference/hgp10_ref.py` (431) | `reference/test_ref.py`, hors CTest | contre Γ, n ≤ 8, k ≤ 4 | hors CTest ; aucun cache |

## 5. Oracles : indépendance, portée, pouvoir de discrimination

### 5.1 Trois objets à ne pas confondre

| Objet | Où | Ce qu'il calcule | Qui il juge | Tailles et ordres |
| --- | --- | --- | --- | --- |
| Oracle Γ | `reference/hgp10_ref.py` (`meb`, `gamma_cuts`, `knn_vertex`, `entry_level`) et `tests/oracle/test_tower_oracle.py:38-80` (`gamma_sweep`) | toutes les k-parties et (k+1)-parties, miniboule exacte par force brute sur les supports, `Fraction` partout | la tour C++ | n ≤ 12 (`P[:12]`, `:164`), K ∈ {1, 3, 5} et E5 à K = 4 (`:165`, `:175`) : 73 contrôles |
| Oracle brut du catalogue | `reference/hgp10_ref.py:211-240` (`critical_balls`, `catalogue`) | toutes les sphères de support de 1 à 4 points, recensement exhaustif, admission | le catalogue C++ | n de 5 à 22, K ∈ {1, 2, 3, 5} (`test_catalogue_oracle.py:116`, `:140`) : 160 contrôles + 1 invariance par translation |
| Tour de référence Python | `reference/hgp10_ref.py:245-367` (`local_structure`, `descend`, `build_tower`) | la tour par boules critiques, morceaux locaux (Gordan) et descente : **les idées du moteur** | elle-même, contre Γ, dans `reference/test_ref.py` | n de 4 à 8, k ≤ 4 (68 nuages) ; **jamais utilisée pour juger le C++** |

**L08_TESTS_PORTES-07 (info, lu) — l'oracle Γ est indépendant du moteur par la méthode ; ce qu'il partage est
identifié.**
- Rien de commun dans le calcul : ni boules critiques, ni séparabilité de Gordan, ni descente, ni arbre spatial, ni
  flottant. Miniboule par énumération des supports ; composantes par union-find sur le graphe.
- Ce qu'il partage avec le moteur : (i) le passage de l'objet du contrat (composantes de la région couverte par au
  moins k boules) au graphe $\Gamma_k$, c'est-à-dire le théorème 2 du manuscrit. L'argument est élémentaire et tient
  aussi en cas dégénéré : $L_k(a)$ est l'union des convexes $R_F = \bigcap_{x \in F} B(x, \sqrt{a})$ pour les
  k-parties $F$ ; $R_F \cap R_{F'} \neq \emptyset$ équivaut à $\beta(F \cup F') \leq a$ ; et deux k-parties d'une même
  partie $G$ de rayon $\beta(G) \leq a$ se relient en changeant un point à la fois, par des unions de $k+1$ points
  incluses dans $G$. (ii) La définition de l'appartenance d'un point (`C ∩ X`, entrée au niveau $D_k(x)$, sommet des
  k plus proches) ; le départage des ex æquo par indice est sans effet, toutes les k-parties candidates étant dans la
  boule fermée de centre $x$. (iii) L'auteur et le fichier : `meb`, `knn_vertex` et `entry_level` viennent du même
  module que la tour de référence.
- L'oracle du catalogue est un oracle **de définition** : il partage avec le moteur la définition d'une boule
  critique et la règle d'admission $p + q_{\min} \leq K + 1$. Si cette règle était insuffisante pour la tour, lui ne
  le verrait pas ; l'oracle Γ le verrait, jusqu'à K = 5 et 12 sites.
- Limite de taille : au-delà de n = 12 et K = 5, plus rien d'exact ne juge. La référence recalcule toutes les
  miniboules à chaque coupe (`point_partition_gamma`, sans cache) : 490 s pour 68 nuages de 8 points au plus.

### 5.2 Ce que les oracles exercent vraiment (mesuré)

Rejeu des mêmes nuages (mêmes graines, générateur importé), compteurs lus sur les lignes JSON des CLI :
`scripts/strates_oracles.py`, résultat `journaux/strates_oracles.json`.

| Strate | Oracle catalogue (160 contrôles) | Oracle tour (73 contrôles) | Trame `lidar00`, K = 5, tour sans attaches (aucun juge) |
| --- | --- | --- | --- |
| Plus grande coordonnée | **998** (10 bits) | ≤ 998 | 158 607 (18 bits) |
| Boules jugées | 14 132 (q2 9 027, q3 4 418, q4 687) | 4 024 | 1 306 696 |
| Coquilles étendues | 3 288, coquille maximale 9 | non comptées (mêmes familles de nuages) | 227 sur 1 306 696, coquille maximale 5 |
| Arbre de boîtes à plusieurs feuilles | 94 contrôles sur 160 (32 feuilles au plus) | jamais (n ≤ 12 ≤ taille de feuille) | toujours |
| Feuilles stagnantes, `wide_leaf` | 0 | — | 0 |
| Multifusions à 3 enfants ou plus | — | 756 sur 1 963 fusions | 87 à l'ordre 1 |
| Sauts K-NN de la descente | — | 37 en tout (36 jonctions, 1 point) | 275 243 |
| Pas par descente | — | 1,07 à 1,12 | 1,21 |
| Repli Welzl exact (`meb_fallbacks`) | — | **0** | **0** |
| Garde I3 tranchée en exact (`level_exact`) | — | **0** | **0** |
| Saut K-NN tranché en exact (`jump_exact`) | — | **0** | 19 |
| Tri parallèle (`n ≥ 8192`, `sched/sort.hpp:20`) | seulement le contrôle de translation | jamais | toujours |
| Multiplicités (coquille pondérée) | **jamais** (nuages à points distincts) | refus | — |

**L08_TESTS_PORTES-08 (majeur, mesuré) — les chemins qui portent la doctrine « filtre flottant certifié, repli
exact » ne sont exercés par aucun oracle, et à peine par les données.** Les trois replis (`meb_fallbacks`,
`level_exact`, `jump_exact`) valent 0 sur les 73 contrôles de l'oracle de tour ; sur une trame entière à K = 5, deux
valent encore 0 et le troisième 19. Ces branches ne se déclenchent que sur des quasi-égalités à grande magnitude :
il faut les **provoquer** par des fixtures construites, avec un plancher sur le compteur. À K = 10 sur la même trame, avec attaches et à 1 fil : `meb_fallbacks` 0 ; `level_exact` 7 ; `jump_exact` 159 — moins de deux cents cas sur des millions de descentes, toujours sans juge. De même, les coordonnées
des oracles s'arrêtent à 10 bits pour un domaine de 18 : les bornes de `src/arith/geometry.hpp:1-8` (numérateur
< 2^162, dénominateur < 2^124, produits croisés sur 320 bits) ne sont jamais approchées sous oracle, et le juge
décimal de `mhgp10_unit` ne tire que des opérandes de 127 bits au plus (`unit_main.cpp:97`).
Contre-épreuve positive : § 5.4.

### 5.3 Pouvoir de discrimination du juge de la tour (mesuré)

Le juge de HEAD (`tests/oracle/test_tower_oracle.py:127-154`) fait trois contrôles, à chaque niveau critique de
$\Gamma_k$, coupes ouverte et fermée : le **nombre** de nœuds vivants égale le nombre de composantes (`:136-139`) ;
la **partition des points entrés**, lue en remontant depuis l'attache par `top()`, égale celle de l'oracle
(`:140-146`) ; et, pour chaque point entré, l'image verticale de sa composante, elle aussi **remontée par `top()`**,
contient le point (`:90-114`). Il ne compare ni l'identité des composantes sans point, ni le nœud exact d'attache,
ni le nœud exact de l'image verticale, ni l'atomicité des plateaux.

Épreuve (`scripts/juge_tour_aveugle.py`) : six nuages de la porte (n = 7 à 12, K = 5), dumps vrais de
`mhgp10_tower`, acceptés par le juge ; chaque dump est rendu **faux** par un opérateur, puis rejoué par les fonctions
mêmes du juge (`parse`, `gamma_sweep`, `top`, `vertical_check` importées, corps de `check()` recopié).

| Opérateur (dump faux) | Contrat violé | Dumps | Acceptés par le juge de HEAD | Acceptés par le juge R2 |
| --- | --- | ---: | ---: | ---: |
| `SWAP` : deux naissances sans point échangées entre deux fusions de niveaux différents (arbre non isomorphe à l'original, vérifié par forme canonique) | la forêt elle-même | 649 | **649** | 0 |
| `BIN` : multifusion de 3 enfants ou plus binarisée au même niveau exact | plateaux atomiques (`docs/SPEC_V10.md` § 2) | 149 | **149** | 0 |
| `V_desc` : `lower[u]` remplacé par un enfant de la bonne image | « nœud vivant au niveau de création » (`src/tower/tower.hpp:108`) | 779 | **779** | 0 |
| `V_autre` : `lower[u]` remplacé par une autre composante vivante | carte verticale | 471 | **376** | 0 |
| `ATT` : attache déplacée vers un enfant absorbé exactement au niveau d'entrée | convention fermée de l'attache | 25 | **25** | 0 |
| Total | | 2 073 | **1 978** | **0** |

Pourquoi `SWAP` passe : si $b_1$ meurt à la fusion $m_1$ et $b_2$ à $m_2$, les échanger conserve le nombre de nœuds
vivants à toute coupe dès que chacun est né avant la plus basse des deux fusions ; sans point attaché, la partition
des points ne bouge pas. La dernière colonne rejoue les mêmes 2 073 dumps (dump `--dump-births` du binaire R2, mêmes
opérateurs) devant le juge de l'arbre intégré R2 (`scripts/juge_r2_contre_faux.py`) : tous refusés, par les témoins
de naissance et la bijection sur les composantes de $\Gamma_k$.

**L08_TESTS_PORTES-09 (bloquant pour la revendication, mesuré) — la porte `mhgp10_tower_oracle` n'établit pas que
« les forêts sont égales à l'oracle ».** `docs/SPEC_V10.md` § 6 et la ligne « Tour FULL » des acquis de
`PASSATION.md` s'appuient sur elle. Elle établit seulement l'égalité des nombres de composantes et des partitions de
points entrés. Portée exacte du constat : il **n'établit pas** que la tour de HEAD soit fausse — le complément
ci-dessous montre au contraire que le juge à témoins l'accepte sur les 73 entrées de la porte —, il établit que la
preuve *versée au dépôt* ne couvre ni la forêt, ni les verticales, ni l'attache à l'égalité. Conséquence : toute comparaison « v11 égale v10 octet pour octet » hérite de cette lacune si elle n'est
pas doublée du juge à témoins.

Complément. J'ai aussi rejoué la porte de tour de l'arbre R2 sur son propre binaire (`journaux/juge_r2_porte_complete.txt`) : code 0 en 37 s (le juge de HEAD, plus faible, en demande 216, faute de cache des miniboules), `tower_oracle_checks 73 fails 0 cuts 23444`, 12 574 bijections, 3 978 verticales jugées dont 2 905 sans point entré, 1 512 mutants de dump tués sur 1 512, à 1, 2 et 4 fils. Et les dumps par défaut de `mhgp10_tower` sont identiques octet pour octet entre HEAD et R2 sur les 73 entrées de la porte (`scripts/head_egale_r2.py`). **Sur ces 73 entrées, la forêt, les attaches `core` et les verticales publiées par HEAD sont donc celles que le juge à témoins accepte** : le défaut est dans la porte du dépôt, pas dans la tour.

### 5.4 Les oracles rejoués hors de leurs strates (exécuté ; résultats positifs)

Mêmes fonctions `check()` que les portes, seuls les nuages ou K changent.

| Épreuve | Script | Contrôles | Écarts | Refus |
| --- | --- | ---: | ---: | ---: |
| Pleine magnitude u18, six familles (uniforme sur tout le domaine, coin supérieur, deux amas aux deux bouts de la diagonale, 5 à 7 points exactement cosphériques à grand rayon, quasi-cosphérique à une unité près, alignés à grand pas), n = 6 à 10 | `scripts/oracle_pleine_magnitude.py` | 192 (catalogue, K = 1, 2, 3, 5) + 144 (tour, K = 1, 3, 5 ; 28 746 coupes) | **0** | 0 |
| Catalogue aux ordres K = 8 et K = 10, les 40 nuages de la porte (n = 5 à 22) | `scripts/oracle_k10.py` | 80 (K = 8 : 10 227 boules jugées ; K = 10 : 11 920) | **0** | — |
| Tour aux ordres K = 7 et K = 10, premiers nuages de la porte (n = 7 à 12 ; arrêté faute de temps, environ 6 minutes par nuage sous charge) | `scripts/oracle_k10.py` | 10 (K = 7 : 5 nuages, 4 790 coupes ; K = 10 : 5 nuages, 5 214 coupes) | **0** | — |

**L08_TESTS_PORTES-10 (majeur, lu et exécuté) — K = 10 n'est jugé par aucune porte et n'est exécuté par aucune sur
le chemin produit.** Ordres joués par les portes : oracle catalogue {1, 2, 3, 5} ; oracle tour {1, 3, 5} et E5 à 4 ;
`mhgp10_cluster` jusqu'à K = 8 (`test_level_collision`, `test_batch_equivalence`, catalogue à 9 pour l'entrée
`cover1`) ; K = 10 n'apparaît que pour le témoin d'atteignabilité mutuelle (`test_mreach_border.py:41`), qui ne passe
ni par le catalogue ni par la tour. Le mutant M04 (le catalogue refuse tout K ≥ 10) laisse les 13 portes vertes
(§ 5.5). Les temps G4 à K = 10 portent donc sur des sorties qu'aucune porte du dépôt n'a jugées. Les lignes 2 et 3
du tableau ci-dessus jugent le moteur de HEAD à ces ordres ; elles sont favorables, sur de très petits nuages (à
12 sites, l'ordre 10 n'a presque plus de structure : le vrai besoin est un oracle à 20 sites et plus, § 11). S'y
ajoute l'invariant d'Euler jusqu'à K = 10 sur les trois trames (§ 5.6), qui juge le catalogue, pas la tour.

### 5.5 Mutants causaux (exécuté)

Il n'y a **aucun mutant dans le dépôt** : ni option `--inject`, ni label `mutant`, ni cible dédiée (recherche de
`inject` et `mutant` dans `src`, `cli`, `tests`, `cmake` : seulement un commentaire de `src/core/types.hpp`). Les
campagnes vivent dans des reçus d'auditeurs et dans le dépôt jetable R2 (§ 7).

J'ai appliqué dix défauts à des copies (`scripts/mutants.py`, remplacements exacts, motif exigé une seule fois) et
joué la suite de HEAD. M01 : les 13 portes complètes. M02 à M10 : les 11 portes hors oracles (`ctest -LE oracle` ;
pour M02, la suite complète arrêtée avant ses oracles), et les deux portes d'oracle par un différentiel exact (`scripts/oracles_rapides.py` : sur une entrée où le dump du
mutant est identique octet pour octet à celui de HEAD, le verdict du juge, déterministe, est celui de HEAD ; le vrai
juge n'est rejoué que sur les dumps qui diffèrent ; le contrôle de translation est rejoué tel quel).

| Mutant | Défaut (fichier:ligne) | 11 portes hors oracles | 2 portes d'oracle | Verdict | Effet mesuré hors portes |
| --- | --- | --- | --- | --- | --- |
| M01 | attache `core` : `niveau ≤ e` devient `niveau < e` (`tower.cpp:999`) | vertes | vertes (13 / 13 en suite complète) | **survit** | 109 attaches changées sur 199 425 (`lidar00`, K = 5) ; fixture de 5 points : étiquettes `[-1,-1,-1,-1,-1]` (HEAD) contre `[-1,0,0,1,1]` |
| M02 | attache `cover` : ancêtre au rang de la boule sans `+ 1` (`tower.cpp:1559`) | vertes | vertes (233 dumps sur 233 identiques à HEAD) | **survit** | fixture de 9 points (K = 3, mcs = 2, z = 3, `cover`) : `[0,0,0,0,0,0,1,1,0]` (HEAD) contre `[2,2,1,1,1,-1,0,0,-1]` ; aucune attache changée sur `lidar00` à K = 5 |
| M03 | image verticale d'une naissance prise un rang trop bas (`tower.cpp:1685`, `:1702`) | vertes | vertes (184 dumps sur 233 identiques à HEAD ; 49 rejugés, 0 écart) | **survit** | viole `tower.hpp:108` |
| M04 | le catalogue refuse tout K ≥ 10 (`generator.cpp:635`) | vertes | vertes (233 dumps sur 233 identiques à HEAD) | **survit** | contrat K = 10 perdu |
| M05 (témoin) | seuil de masse de la condensation strict (`head.cpp:85`, `:89`) | **rouge** : `mhgp10_head_condensation_vs_sklearn` | vertes (233 dumps sur 233 identiques à HEAD) | tué | — |
| M06 | exposant z ignoré par la tête (`head.cpp:13`) | vertes | vertes (233 dumps sur 233 identiques à HEAD) | **survit** | `lidar00`, K = 5, mcs = 200, z = 3, `cover` : 48 amas au lieu de 66, 32 723 étiquettes changées sur 39 885 |
| M07 (témoin) | admission q = 4 décalée d'une couche (`generator.cpp:262`) | **rouge** : `mhgp10_points_cover`, `mhgp10_regression_batch_equivalence`, `mhgp10_regression_level_collision`, `mhgp10_regression_multiplicity_refusal` | catalogue **rouge** (55 écarts sur 55 dumps rejugés) ; tour **rouge** (31 écarts sur 31 dumps rejugés) | tué | `lidar00`, K = 5 : `invariant_violated / root_count` |
| M08 | correctif de la collision de niveaux retiré (`tower.cpp:1839`) | **rouge** : `mhgp10_regression_level_collision` | vertes (233 dumps sur 233 identiques à HEAD) | tué | — |
| M09 | boule q2 de la dernière couche perdue si son diamètre carré d2 > 2^24 et d2 mod 2048 = 5 | **rouge** : `mhgp10_regression_batch_equivalence` | vertes (233 dumps sur 233 identiques à HEAD) | tué | uniforme 8 000, K = 5 : 11 boules perdues sur 599 711, tour publiée `ok` avec 274 143 nœuds à l'ordre 5 au lieu de 274 144 ; `lidar00` : 1 boule perdue, statut `ok` ; Euler à kcat = 7 inchangé ; seule la restriction cat(5) contre cat(7) le voit à l'échelle (11 boules q2, p = 4) |
| M10 | même perte sur toutes les couches | **rouge** : `mhgp10_regression_batch_equivalence`, `mhgp10_regression_level_collision` | vertes (233 dumps sur 233 identiques à HEAD) | tué | uniforme 8 000 : 90 boules perdues à K = 5 ; Euler à kcat = 7 : 20, −2, 8, −1, −9 au lieu de 1 ; tour : `invariant_violated / census_mismatch` (code 3) à 8 000 points et sur `lidar00` |

**L08_TESTS_PORTES-11 (majeur, exécuté) — des défauts d'une ligne, qui changent des sorties sur les entrées du
contrat, traversent toute la suite.** Sur dix mutants, **5 survivent aux 13 portes** : M01 (attache `core` stricte), M02 (rang de l'attache `cover`), M03 (image verticale des naissances), M04 (refus de K ≥ 10), M06 (exposant z ignoré). 5 sont tués : M05 (seuil de masse strict), M07 (admission q = 4 décalée), M08 (collision de niveaux non corrigée), M09 (boule rare perdue en dernière couche), M10 (boule rare perdue en toute couche). Lecture : (i) la convention d'attache à l'égalité, les verticales, l'ordre 10 et l'exposant z n'ont aucune porte ; (ii) les deux oracles ne voient rien de ce qui dépend de la magnitude (M09 et M10 y rendent des dumps identiques à HEAD) ; (iii) les portes de régression à 2 000 et 8 000 points tuent ce qui déclenche un garde-fou interne (M07 : `root_count` ; M10 : `census_mismatch`), et M09, qui publie `ok` partout, n'est tué que par ricochet : 4 contrôles sur 136 de `mhgp10_regression_batch_equivalence`, à K = 1, parce que cette porte compare le catalogue d'ordre K à la restriction d'un catalogue d'ordre 9 — c'est, au niveau des étiquettes et à 2 000 points, l'invariant de restriction qu'il faut promouvoir en porte d'échelle ; (iv) la tête n'a qu'un seul juge, scikit-learn sur le témoin (M05), et il ne voit pas z. M06 change 32 723 étiquettes sur 39 885 et le nombre d'amas (66 contre 48) sur `lidar00` à K = 5, mcs = 200, z = 3, entrée `cover`.

### 5.6 Portes à l'échelle : ce qui existe, et ce que des juges indépendants disent de la v10 (mesuré)

**L08_TESTS_PORTES-12 (majeur, lu et mesuré) — aucune porte ne juge le moteur aux tailles d'intérêt.**
- Tailles jouées par les portes : 8 000 points une fois (`mhgp10_regression_level_collision`, K = 8 : statut `ok` et
  niveaux strictement croissants, rien d'autre) ; 3 000 (translation du catalogue : auto-cohérence) ; 2 000 et 1 500
  (égalités entre deux exécutions du même binaire). **Rien à 16 000, rien à 32 000, aucune trame.** Les trames ne
  passent que par les bancs (`bench/scaling/scale_run.py`), qui mesurent sans juger.
- À l'échelle, `docs/SPEC_V10.md:84-85` s'appuie sur deux faits : les **comptes** par $(q_{\min}, p)$ du catalogue
  d'une trame égalent ceux de la v9, et chaque ordre a une seule racine. Ni l'un ni l'autre ne juge la forêt.
- Les invariants internes (S11) tournent à chaque exécution, c'est un vrai filet : les mutants M07 et M10 les font
  tomber sur une trame. Mais ils ne sont pas indépendants du moteur, et une erreur rare qui retarde une fusion sans
  créer de seconde racine leur échappe : M09 publie `ok` à 8 000 points et sur une trame, et n'est tué que par
  ricochet (§ 5.5).
- Les juges d'échelle prévus par la conception (arbre couvrant à l'ordre 1, Euler à kcat + 2, recensement, descentes,
  fusions, parcours local) ne sont pas livrés (§ 7).

J'ai donc écrit trois juges indépendants, sans tableau indexé par paire ni juge cubique, et les ai joués sur le
binaire de HEAD. Ils passent tous.

| Juge | Ce qu'il établit | Entrées | Résultat |
| --- | --- | --- | --- |
| Arbre couvrant exact (`scripts/emst_k1.py`, 104 lignes) | l'ordre 1 de la tour égale le lien simple exact : multiensemble des (niveau, taille de la composante créée) des fusions, Kruskal par plateaux sur un arbre couvrant de Prim en entiers | uniforme u18 de 8 000, 16 000, 32 000 points ; trames `lidar00`, `lidar01`, `lidar02` (39 885, 35 551, 45 845 sites ; 87, 87 et 272 multifusions) | **6 sur 6 conformes**, 0 écart |
| Euler, forme close (`scripts/euler_echelle.py`, 66 lignes) | complétude globale du catalogue : $E_K = n \cdot [K = 1] + \sum_{B} e_K(B) = 1$ pour $K \leq K_{\mathrm{cat}} - 2$, lue sur les comptes par $(q, p)$ de la ligne JSON | uniforme 8 000, 16 000, 32 000 à kcat = 7 (1 306 627, 2 705 608, 5 582 288 boules, aucune coquille étendue) ; uniforme 8 000 à kcat = 12 (4 961 747 boules) | **$E_K = 1$** pour K = 1 à 5 aux trois tailles, et pour **K = 1 à 10** à 8 000 points |
| Euler général (`scripts/euler_general.py`, 72 lignes ; cellules de l'arrangement de grands cercles par la fonction `chi_cells` de l'auditeur C de la v9) | le même invariant quand des coquilles sont étendues | validation : 8 grilles dégénérées de 14 à 40 sites (31 à 504 coquilles étendues de 3 à 15 sites), kcat = 7 ; puis les trois trames | grilles : **$E_K = 1$**, K = 1 à 5, 8 sur 8 ; trames : **$E_K = 1$** sur les trois trames, pour K = 1 à 5 à kcat = 7 (2 565 656, 2 104 698 et 2 675 990 boules ; 320, 204 et 865 coquilles étendues) et pour **K = 1 à 10** à kcat = 12 (8 314 472, 6 492 748 et 8 025 829 boules ; 529, 341 et 1 559 coquilles étendues) |

Lecture : ces trois juges ne remplacent pas un oracle (Euler est un scalaire par ordre et ne voit pas les deux
dernières couches du catalogue ; l'arbre couvrant ne juge que l'ordre 1). Mais ils sont indépendants du moteur, ils
coûtent quelques secondes à quelques minutes, et ils auraient pu être des portes `scale8000`, `scale16000`,
`scale32000` et `lidar` dès la v10. Sur une trame, la forme close ne suffit pas : `lidar00` à kcat = 7 a
320 coquilles étendues sur 2 565 656 boules, et la somme close y vaut −48, 61, −6, −22, 22 ; il faut le calcul
général.

### 5.7 Déterminisme : fils, permutation, compilateur (mesuré)

**L08_TESTS_PORTES-13 (mineur, lu et mesuré) — la v10 est déterministe, mais aucune porte ne le garde pour le
catalogue ni pour la tour FULL.**
- Ce que les portes comparent entre 1 et 4 fils : des **étiquettes** (`test_batch_equivalence`), l'arbre de points
  d'**un** ordre en entrée `cover` (`test_cover_entry`, empreinte du fichier `--tree`) et le témoin d'atteignabilité.
  Les deux oracles tournent à `--threads=2` seulement. Aucune porte ne compare deux dumps du catalogue ni de la
  tour FULL (tous ordres, verticales), ni ne permute l'entrée ; `mhgp10_unit` ne juge la permutation que pour la
  préparation du nuage. Les CLI fixent `PointId` au rang d'entrée : des identifiants arbitraires ne traversent
  jamais catalogue, tour et tête.
- Mesure (`scripts/determinisme.py` et `scripts/determinisme_tour.py`, empreinte sha256 des dumps texte) :

| Entrée | Dump | Selon le nombre de fils | Entrée permutée |
| --- | --- | --- | --- |
| uniforme 8 000, K = 5 | catalogue (105 Mo) | identiques à 1, 2, 4 fils | identique |
| uniforme 8 000, K = 5 | tour FULL avec attaches (44 Mo) | identiques à 1, 2, 4 fils | identique |
| `lidar00`, K = 5 | catalogue | identiques à 1, 2, 4 fils (203 Mo) | identique |
| `lidar00`, K = 5 | tour FULL avec attaches (80 Mo) | identiques à 1, 2, 4 fils | identique |
| `lidar00`, K = 10 | catalogue (1 256 Mo) | identiques à 1 et 4 fils | identique |
| `lidar00`, K = 10 | tour FULL avec attaches (433 Mo) | identiques à 1 et 4 fils | identique |

- Compilateur : les comptes publiés par les 13 portes sont identiques en GCC et en Clang (§ 3.1), jusqu'aux
  509 710 niveaux de la scène de 8 000 points.

## 6. Hiérarchie de points, tête et bancs : ce que les portes gardent

**L08_TESTS_PORTES-14 (majeur, lu et exécuté) — aucun oracle ne juge la tête sur les dendrogrammes de la tour, ni
l'attache `cover`, ni l'exposant z, ni les outils qui décident « mieux qu'HDBSCAN ».**
- La seule porte de la tête compare à scikit-learn la condensation d'un **témoin d'atteignabilité mutuelle**
  (`tests/head/mreach.cpp`), à K = 1 et 2 seulement (`test_condensation_vs_sklearn.py:55`), où chaque point est sa
  propre feuille. Les dendrogrammes de la tour ont une autre forme : points attachés à des nœuds internes à leur
  niveau d'entrée, plateaux N-aires. C'est précisément là que l'auditeur continu a trouvé le défaut de la
  condensation (départs de points sans contrôle de masse, `PASSATION.md:11-20`) : la porte ne pouvait pas le voir.
- L'entrée `cover`, celle des résultats de clustering publiés (lot C), n'a pour juge que son **niveau** d'entrée,
  comparé à une force brute à 1e-9 près en relatif sur 2 nuages de 14 points, K = 2, 3, 4
  (`tests/points/test_cover_entry.py:87-97`). Le **nœud** d'attache n'est jugé par rien : l'oracle de tour ne joue
  que l'entrée `core` (aucun `--entry=cover` dans `tests/oracle/`). Le mutant M02 le confirme (§ 5.5).
- L'exposant z de l'échelle $\lambda = r^{-z}$ n'est comparé à aucune valeur attendue : les portes qui l'emploient
  (z = 2,5, 3 ou estimé) comparent deux exécutions du même binaire. Le mutant M06, qui ignore z, passe (§ 5.5).
- Le témoin `mhgp10_mreach_cluster` réimplémente l'objet d'HDBSCAN avec la tête v10. Il n'est égalé à
  scikit-learn qu'à K = 1 et 2 ; au-delà, « les partitions peuvent différer » (docstring de la porte). Toute phrase
  « à même entrée et même tête, la tour égale la hiérarchie d'HDBSCAN » s'appuie donc sur un témoin, pas sur
  scikit-learn : le contrat de la v11 (« jamais réimplémenté ») l'interdit comme référence.
- Les niveaux du dendrogramme de points sont publiés en `double`, les niveaux exacts indiscernables étant fusionnés
  dans un même rang (`src/tower/tower.cpp:1828-1839`) : la sortie livrée à la tête n'est pas exacte, et aucune porte
  ne juge ce regroupement autrement que par « strictement croissant ».
- Outils sans aucune porte dans le dépôt : `bench/synthetic/decide.py`, `run_test.py`, `run_campaign.py`,
  `metrics.py`, `choose_config.py`, `bench/g4/merge_sessions.py`, `lot_runner.py`, `bench/scaling/*.py`
  (recherche de chaque nom dans `tests/`, `reference/`, `CMakeLists.txt` : aucune occurrence). Ce sont eux qui
  produisent les écarts et les p-valeurs publiés contre HDBSCAN.

## 7. Où vit l'appareil de test

**L08_TESTS_PORTES-15 (majeur, lu et exécuté) — l'essentiel de la qualification de la v10 est hors dépôt, et vise
la périphérie.**
- Dépôt (`afb081774`) : 13 portes ; 2 284 lignes sous `tests/` pour 4 876 lignes de `src/` et 556 de `cli/`.
- Dépôt jetable `/workspaces/E-HGP/build/v10-integration-r2/src` (commit `865f5e6`, 30 commits, série
  `series/0001` à `0030`, `final.patch` de 7,4 Mo) : 82 portes déclarées vertes en GCC et en Clang, 80 sous
  ASan et sous TSan (d'après ses notes), 18 481 lignes sous `tests/`, juges à témoins, harnais de fautes
  d'allocation, 425 mutants relus (d'après ses notes). **Jamais importé** : la note `notes/A_FAIRE_APRES_RACCORD.md` finit par « import de la série : à faire par
  le développeur ». J'ai construit son `mhgp10_tower` et rejoué son juge de tour : § 5.3.
- Répartition des campagnes de mutants de ce raccord (`logs/final/v8_relecture.txt`, 347 lignes) : bancs Python
  120, entrées de la CLI 63, tête 60, pool 47, juges 41, options des sondes 8 ; plus une campagne gravée de
  125 mutants (arbre des sites, coupure des filtres flottants selon le mode d'arrondi, options de compilation). **Aucune campagne ne mute
  l'énumération du catalogue, la structure locale, la descente, le Kruskal par plateaux ni les prédicats exacts** :
  `src/tower/tower.cpp` n'y apparaît que pour la coupure des filtres selon le mode d'arrondi et la conversion de
  `bad_alloc` ; `src/arith/geometry` n'y apparaît pas.
- Même dans cet arbre, aucun label `scale8000`, `scale16000`, `scale32000` ni `lidar` : les invariants d'échelle n'y
  sont que des scripts de reçus (`receipts/raccord_r2_20260930/reparation/outils/invariants_echelle.py`), et ils
  sont structurels (une racine, plateaux, attaches, verticales vivantes) ; leur en-tête le dit : « ce n'est pas un
  oracle de complétude géométrique ».
- Près de 2 000 lignes (`cmake/fp_flags.cmake` 486, `tests/regression/fp_flags_configure.cmake` 518,
  `tests/regression/test_fp_contract.py` 942) servent à refuser `-ffast-math` sous toutes ses écritures ; le commit de clôture
  (`35de8c0`) déclare encore quatre canaux « hors contrat ». Le rejeu final de ce raccord a pris 6 heures
  (note de mémoire du développeur, non contrôlé).
- Le plan de portes de la conception (`docs/conception/CONCEPTION_V10.md:862-905`) prévoyait pour le cœur : oracle
  à K jusqu'à 12 sur au moins 1 000 nuages avec second oracle C++, empreintes épinglées, Euler à kcat + 2, juge des
  boîtes, identité à 1, 2, 8, 48 fils et sous permutation, 16 puis 24 mutants, juges d'échelle (arbre couvrant,
  recensement, descentes, fusions), labels `scale8000` à `lidar`. **Rien de cela n'est livré** ; `PASSATION.md:222`
  le range parmi les chantiers ouverts (« juges d'échelle (K = 1 contre EMST, Euler à kmax + 2) »), et
  `src/core/types.hpp:33` garde la trace d'un juge d'Euler qui n'existe pas (`kMaxCatalogueOrder = 12`).

**L08_TESTS_PORTES-16 (mineur, lu et exécuté) — enregistrement et intégration continue.** Aucun workflow ne
construit la v10 ; la référence exacte est hors CTest ; Python n'est pas requis à la configuration (3 portes sur 13
sans lui, § 3.3) ; un test (`test_cover_band_native.py`) est présent mais ni enregistré ni exécutable depuis le
dépôt.

## 8. Ce qui est solide et mérite un port explicite en v11

| # | Acquis | Source à épingler | Pourquoi |
| --- | --- | --- | --- |
| S1 | `run_expect.cmake` : code exact, signal refusé, ligne exacte lue sur la même exécution | `morsehgp3D_v10/cmake/run_expect.cmake` (26 l.) | 11 cas joués, tous conformes (L08-01) ; y ajouter son auto-test |
| S2 | Oracle Γ exhaustif en `Fraction`, coupes ouvertes **et** fermées, à tous les niveaux critiques | `reference/hgp10_ref.py` (`meb`, `gamma_cuts`), `tests/oracle/test_tower_oracle.py:38-80` | indépendant du moteur par la méthode (L08-07) ; porter avec le cache par support du raccord R2 (`class Balls`) |
| S3 | Oracle brut du catalogue (supports de 1 à 4 points, recensement exhaustif) | `reference/hgp10_ref.py:211-240` | coût indépendant de K : s'étend à K = 10 et à 40 sites sans effort (§ 5.4) |
| S4 | Familles de nuages dégénérés des oracles (générique, grille 0..3, coplanaire, grille groupée) et leurs graines | `tests/oracle/test_catalogue_oracle.py:114-130`, graines 20260928 et 20260929 | 3 288 coquilles étendues, 756 multifusions mesurées |
| S5 | Fixtures gravées : E5, carré, cube, grilles de `test_ref.py` ; AUDIT3, TRIANGLE, LINE5, OCTA, PAIR, LINE4 du juge R2 ; 5 points d'égalité d'attache `core` et 9 points d'attache `cover` ; 48 points cosphériques (refus attendu) | `reference/test_ref.py:10,59-63` ; `build/v10-integration-r2/src/morsehgp3D_v10/tests/oracle/test_tower_oracle.py:597-610` ; `build/v10-giant-audit/moteur_livre/fixture_egalite_attache_min5.txt` et `verif_moteur_livre/preuves/fixtures_egalite.txt` ; § 4.1 et § 11.4 de ce rapport | chacune tue ou isole un défaut nommé |
| S6 | Juge décimal « d'école » des entiers larges, juge de Morton par flux inverse, distance par magnitudes non signées | `tests/unit/unit_main.cpp:31-94`, `tests/unit/grid32_primitives.cpp:25-54` | juges à algorithme volontairement autre ; étendre aux opérandes pleins |
| S7 | Porte de stress du pool (travaux d'un indice alternés avec des travaux de 64) | `tests/unit/unit_main.cpp:182-203` | construite pour ouvrir la fenêtre d'une course réelle (`8e3b76245`) |
| S8 | Requêtes de l'arbre des sites contre force brute, avec plancher de coquilles | `tests/unit/unit_main.cpp:274-387` | 13 155 contrôles, 1 099 coquilles |
| S9 | Contrôle d'invariance par translation du catalogue | `tests/oracle/test_catalogue_oracle.py:84-111` | seul contrôle sous juge qui sorte du régime des petites coordonnées ; a tué un refus réel (`wide_leaf`) |
| S10 | Refus transactionnels à raison nommée, témoin positif dans la porte | `tests/regression/test_multiplicity_refusal.py` | modèle d'une porte de refus (code 2, raison, statut, témoin) |
| S11 | Garde-fous du moteur toujours actifs : descente strictement décroissante, recensement d'échantillon 1 / 32, naissance absente de la table, une racine par ordre, naturalité des verticales | `src/tower/tower.cpp:836-844, 884-893, 985-988, 1114, 1719-1727` | font tomber un catalogue amputé à l'échelle (M07, M10) ; à garder comme invariants produit, chacun avec sa porte négative |
| S12 | Juge de tour à témoins du raccord R2 (bijection nœuds vivants / composantes de Γ, plateaux atomiques, convention fermée, verticales à chaque nœud, en-têtes, mutants de dump `--inject`) | `build/v10-integration-r2/src/morsehgp3D_v10/tests/oracle/test_tower_oracle.py` (1 253 l., commit `865f5e6`) | refuse les 2 073 dumps faux de l'épreuve, dont 1 978 acceptés par le juge de HEAD ; porte complète rejouée ici, verte en 37 s (§ 5.3) |
| S13 | Juge du catalogue du raccord R2 (enregistrements dans l'ordre publié, support canonique, niveaux exacts, rangs denses, poids, fixtures aux extrêmes u18, feuilles forcées) | même dépôt, `tests/oracle/test_catalogue_oracle.py` (834 l.) | lu seulement ; ferme les manques du juge de HEAD (ensembles sans ordre, niveaux non jugés) |
| S14 | Porte de la tête « par définition » et tests négatifs de `validate()` du raccord R2 | même dépôt, `tests/head/head_gate.cpp` (1 165 l.) | lu seulement ; ne remplace pas un oracle de la condensation (L08-14) |
| S15 | Invariant de Morse–Euler par ordre (v9, `proved_here`) | `morsehgp3D_v9/audits/NOTE_C_INVARIANT_EULER_20260923.md` | juge global du catalogue à l'échelle, une passe, rejoué ici sur la v10 (§ 5.6) |
| S16 | Bibliothèque de fixtures « tour vers points » (35 fixtures, 180 variantes, K = 2 à 10, dont les deux triangles) | `build/v10-verrou-points/fixtures_cibles/` (`CATALOGUE_FIXTURES_CIBLES.md`, `fixtures_catalogue.json`) | hors dépôt ; lue en survol seulement |

## 9. Ce qu'il ne faut pas refaire

1. **Un juge qui compte au lieu d'identifier.** Nombre de composantes et partition des points entrés ne déterminent
   pas la forêt (§ 5.3). Corollaire : ne jamais remonter par `top()` avant de comparer une attache ou une image
   verticale, cela efface l'erreur qu'on cherche.
2. **Des oracles cantonnés à 10 bits de coordonnées et à K ≤ 5** quand le domaine est de 18 bits (bientôt 24 puis 32)
   et le contrat à K = 10. L'oracle brut du catalogue ne coûte rien de plus à K = 10 ; il suffisait de l'y jouer.
3. **Des replis exacts que rien ne déclenche.** Un filtre flottant dont le repli vaut 0 dans toutes les portes est du
   code non testé, quelle que soit la preuve de sa marge.
4. **Qualifier dans une copie privée.** 82 portes et 425 mutants hors dépôt valent, pour le lecteur du dépôt,
   13 portes et aucun mutant.
5. **Dépenser les mutants sur la périphérie** (bancs, CLI, options de compilation) avant le cœur géométrique.
6. **Un plan de portes plus grand que la livraison.** Le plan de la conception v10 était juste ; il est resté un
   plan. Mieux vaut six juges d'échelle de cent lignes livrés que vingt promis.
7. **Des portes de régression dont la fixture est régénérée** par un générateur flottant et un tirage numpy, sans
   empreinte ni plancher sur le phénomène gardé.
8. **Un saut silencieux dans une porte** (`continue` sur « option inconnue ») et un compteur annulé (`0 * c`).
9. **Le code 1 pour tout** : désaccord, module absent, binaire tué, sanitizer.
10. **Chronométrer sur G4 une sortie que rien ne relie à une sortie jugée** : pas d'empreinte, oracles non rejoués
    après les changements du moteur, K = 10 jamais jugé.
11. **Appeler « HDBSCAN » un témoin maison.** La référence adverse est scikit-learn, appelé tel quel ; sa
    dépendance à l'ordre des ex æquo se traite en publiant l'étendue, pas en le remplaçant.
12. **Des refus et des garde-fous sans porte négative**, et des raisons de refus déclarées que rien n'émet.

## 10. Questions ouvertes

1. **Convention d'attache à l'égalité.** HEAD attache un point au nœud *vivant* à la coupe fermée de son entrée
   (niveau du nœud ≤ entrée < niveau du parent). Sur la fixture de 5 points (K = 2, mcs = 2), cette convention donne
   « tout bruit » et la convention stricte deux clusters, comme scikit-learn sur l'objet voisin. Laquelle est le
   contrat de la v11 ? Tant qu'elle n'est pas écrite et gardée, la hiérarchie de points n'est pas définie à
   l'égalité.
2. **Un oracle exact à K = 10 de taille utile.** Γ en Python juge K = 10 à 12 sites (fait ici) ; à 20 sites il faut
   un second oracle en C++ à arithmétique autre, ou un oracle tronqué en niveau. Lequel la v11 écrit-elle, et avec
   quelle preuve que la troncature est complète sous le niveau choisi ?
3. **Euler sur les trames.** La forme close ne vaut que pour les coquilles régulières ; les trames du contrat ont des
   coquilles étendues (200 à 1 600 par trame, de 3 à 5 sites). Le calcul général par cellules de la v9 (`chi_cells`)
   suffit comme juge (§ 5.6). Pour un invariant *produit*, faut-il le porter tel quel ou écrire la forme circulaire
   de la conception v10 (PO-T19, PO-T20), et qui en relit la preuve ?
4. **Complétude à l'échelle au-delà d'Euler.** Euler est un scalaire par ordre : deux erreurs peuvent se compenser, et
   les deux dernières couches lui échappent. La restriction $\mathrm{cat}(K) = \mathrm{restrict}(\mathrm{cat}(K+2))$
   compare le produit à lui-même. Quel juge indépendant couvre ces deux couches à 32 000 points : le juge des boîtes
   par échantillon suffit-il, avec quel plancher ?
5. **Multiplicités.** Le catalogue accepte les poids, aucune porte ne les juge ; la tour les refuse. Les trois trames
   du contrat n'ont aucun doublon à 1 mm. La v11 traite-t-elle les poids, et sinon retire-t-elle le chemin pondéré
   du catalogue ?
6. **Coquilles au-delà du budget.** 48 points cosphériques sont refusés dès K = 1. Est-ce acceptable pour des trames
   avec sol (plans, grilles), ou faut-il une réponse exacte ?
7. **Référence scikit-learn dépendante de la machine.** Publier l'étendue entre ordres d'ex æquo, ou fixer un
   départage canonique en amont (perturbation symbolique des poids) sans toucher au code de scikit-learn ?
8. **Niveaux publiés en `double`.** La tête reçoit des niveaux flottants regroupés ; la v11 publie-t-elle des rangs
   exacts, la vue flottante n'étant qu'un affichage ?
9. **Portée de `diff_v10`.** La v10 figée n'a été jugée par un oracle à témoins que jusqu'à K = 5 et 12 sites ; à
   l'échelle, mes juges ne couvrent que l'ordre 1 de la tour (arbre couvrant) et le catalogue (Euler jusqu'à K = 10,
   trames comprises). **La tour aux ordres 2 à 10 n'a aucun juge indépendant à l'échelle.** Une égalité v11 = v10
   sur une trame est donc une non-régression, pas une preuve : est-ce écrit ainsi dans la conformité (§ 6 de
   l'architecture) ?
10. **Non vérifié dans cet audit** : la stabilité de la porte scikit-learn sur une machine à AVX-512 ; TSan
    (joué sur `mhgp10_unit` et une tour de 2 000 points seulement) ; les campagnes de mutants du raccord R2 (lues, non rejouées) ; les juges R2 du catalogue et de
    la tête (lus seulement).

## 11. Recommandations : le plan de tests de la v11, dès le premier commit

Le cadre déjà posé (`morsehgp3D_v11/docs/ARCHITECTURE.md` § 5 et § 6 : codes 0 à 4, plancher par porte, labels
`unit`, `oracle`, `diff_v10`, `scale8000`, `scale16000`, `scale32000`, `lidar`, `mutant`, `fast`, mutants dans
`tests/mutants/`) est le bon. Ce qui suit le rend exécutable et ferme, un par un, les trous constatés en v10.

### 11.1 Règles (R)

| # | Règle | Constat fermé |
| --- | --- | --- |
| R1 | **Une porte naît dans le commit du code qu'elle garde.** Aucun correctif ni aucun juge ne vit dans une copie privée ; un chantier de réparation se fait par petites séries importées au fil de l'eau. | L08-15 |
| R2 | **Toute égalité se juge sur une empreinte canonique publiée par le produit** (`catalogue_digest`, `tower_digest` par ordre et global, `points_digest`), calculée sur une sérialisation définie (sites dans l'ordre de Morton, rangs exacts, parents, témoins, images verticales, attaches). Les portes d'identité (fils, permutation, renumérotation des `PointId`, translation, compilateur, machine G4) comparent des empreintes ; les reçus de performance la publient à côté du temps, **y compris dans le mode chronométré** (tour FULL sans attaches), que la v10 ne sait pas exporter. | L08-05, L08-06, L08-13 |
| R3 | **Codes** : 0 conforme ; 1 désaccord d'un juge ; 2 refus avant calcul, *dépendance absente comprise* ; 3 plancher, invariant ou erreur du harnais ; 4 mutant tué. Aucune trace d'exception Python ne rend 1 : un `main()` gardé convertit. `ASAN_OPTIONS=exitcode=…` distinct de 1. | L08-02 |
| R4 | **Planchers par strate**, publiés puis exigés (`--min-*`), jamais un total. Strates minimales : par famille de nuage, par q (2, 3, 4), coquilles étendues par taille, multifusions à 3 enfants ou plus, descentes à 1, 2, 3 pas et plus, sauts K-NN, **chaque repli exact** (compteur > 0), arbres de boîtes à plusieurs feuilles, classes de magnitude, multiplicités, **chaque ordre du contrat (K = 5 et K = 10)**. | L08-03, L08-08, L08-10 |
| R5 | **Fixtures gravées en octets** (coordonnées dans le dépôt, sha256 contrôlé par la porte). Aucune fixture régénérée par numpy. Les scènes de banc tirées au hasard restent des bancs. | L08-04 |
| R6 | **`fast` = Python nu et C++**, moins d'une minute, et contient un **échantillon de chaque oracle** et les empreintes épinglées des fixtures : c'est ce qui tourne sur la VM G4 avant tout chronomètre. | L08-05 |
| R7 | **Le juge de la tour juge l'identité** : témoin de naissance par nœud (une K-partie), bijection des nœuds vivants sur les composantes de $\Gamma_k$ à chaque coupe, plateaux atomiques, attache à la convention écrite (fermée : niveau du nœud ≤ entrée < niveau du parent), image verticale *exacte* à chaque naissance et fusion. Jamais un compte seul. | L08-09 |
| R8 | **Mutants d'abord sur le cœur** : catalogue, structure locale, descente, Kruskal, verticales, attache, tête. Chaque mutant nomme la porte qui doit le tuer (code 4) ; un survivant est un constat ouvert, pas une ligne de reçu. Budget : moins de 10 minutes pour le label `mutant`. | L08-11 |
| R9 | **Un juge d'échelle est indépendant ou n'est pas un juge** : la comparaison à la v10 (`diff_v10`) est un différentiel, jamais la seule preuve à 8 000 points et plus. | L08-12 |
| R10 | **Chaque raison de refus a sa porte** (code 2 ou 3, raison et statut exacts, témoin positif), chaque garde-fou interne son mutant ou son entrée fabriquée. Une raison jamais émise est retirée. | L08-06 |
| R11 | **Dépendances déclarées** : `Python3 REQUIRED` ; `tests/requirements.txt` épinglé ; les portes à numpy ou scikit-learn portent un label propre (`sklearn`) et refusent (code 2) hors des versions épinglées. | L08-05 |
| R12 | **CI dès le premier commit** : build GCC et Clang, `ctest -L fast`, ASan + UBSan, TSan (`setarch -R`), sans GCP. | L08-16 |

### 11.2 Portes, dans l'ordre des tranches

Tranche 0 — avant tout code produit.

| Porte | Label | Contenu | Plancher |
| --- | --- | --- | --- |
| `mhgp11_gate_selftest` | `fast`, `unit` | les 11 cas de `run_expect` du § 4.1 (codes, signal, ligne exacte, binaire absent) | 11 cas |
| `mhgp11_ref_gamma` | `oracle` | l'oracle Γ contre la définition directe (graphe complet des intersections $\beta(F \cup F') \leq a$, sans la restriction à $k + 1$ points) et contre la tour de référence ; fixtures de S5 | n ≤ 9, K ≤ 6 ; ≥ 200 nuages ; ≥ 50 par famille |
| `mhgp11_ref_fixtures` | `fast`, `oracle` | fixtures gravées avec empreinte attendue de leur tour (E5, carré, cube, octaèdre, lignes, triangle, deux triangles équilatéraux, 5 points d'égalité) | toutes |
| `mhgp11_digest_vectors` | `fast`, `unit` | vecteurs de l'empreinte (SHA-256 du NIST), sérialisation canonique d'une petite tour écrite à la main | 10 vecteurs |

Tranche 1 — fondations.

| Porte | Label | Contenu | Plancher |
| --- | --- | --- | --- |
| `mhgp11_num_wide` | `fast`, `unit` | entiers à budget contre un juge décimal **et** contre des vecteurs engendrés par Python (entiers non bornés), opérandes de pleine largeur à chaque taille, débordements refusés | 10^5 contrôles par largeur, ≥ 10^3 à largeur pleine |
| `mhgp11_num_predicates` | `oracle` | côté, niveaux, comparaison de niveaux, centres q2, q3, q4 contre `Fraction`, aux coins du domaine, quasi-égalités à une unité de grille | ≥ 10^4 par prédicat, ≥ 10^3 égalités exactes, ≥ 10^3 quasi-égalités |
| `mhgp11_num_filters` | `unit` | chaque filtre flottant : cas certifiés, cas **forcés au repli** (fixtures de quasi-égalité à grande magnitude), les quatre modes d'arrondi | repli atteint ≥ 100 fois par filtre |
| `mhgp11_sched_pool`, `mhgp11_sched_sort` | `fast`, `unit` | stress du pool (S7) ; tri parallèle contre `std::sort` pour 1, 2, 5, 8 fils, au-dessus et au-dessous du seuil | 50 000 travaux ; 100 tableaux |
| `mhgp11_cloud` | `fast`, `unit` | sites, multiplicités, permutation, `PointId` arbitraires, les refus un par un | chaque raison |
| `mhgp11_io_refusals` | `fast` | une porte à code exact par refus de la CLI (option inconnue, valeur illisible, K hors domaine, entrée = sortie, sortie non inscriptible) | chaque raison |

Tranche 2 — catalogue.

| Porte | Label | Contenu | Plancher |
| --- | --- | --- | --- |
| `mhgp11_catalogue_oracle` | `oracle` (+ échantillon `fast`) | juge R2 (S13) : enregistrements dans l'ordre publié, support canonique, niveau exact, rang dense, poids ; **K de 1 à 12** ; n jusqu'à 40 ; feuilles forcées petites ; classes de magnitude (petite, pleine, coin, deux amas, cosphérique à grand rayon, quasi-cosphérique, aligné : § 5.4) ; multiplicités | par strate (R4) |
| `mhgp11_catalogue_euler` | `fast`, `scale*`, `lidar` | invariant produit : $E_K = n \cdot [K = 1] + \sum_{B} e_K(B) = 1$ pour $K \leq K_{\mathrm{cat}} - 2$, coquilles étendues comprises (formule générale de la note v9), en une passe | à chaque exécution d'échelle |
| `mhgp11_catalogue_restriction` | `scale*`, `lidar` | $\mathrm{cat}(K)$ égale la restriction de $\mathrm{cat}(K + 2)$ (empreinte), ce qui juge les deux dernières couches qu'Euler ne voit pas | 8 000, 16 000, 32 000, trois trames, K = 5 et 10 |
| `mhgp11_catalogue_boxes` | `scale*`, `lidar` | juge d'échantillon : énumération brute des boules dans des boîtes tirées par strate | ≥ 200 boîtes par entrée |
| `mhgp11_catalogue_identity` | `scale8000`, `lidar` | empreinte identique à 1, 2, 8 fils, sous permutation, sous translation, sous renumérotation des `PointId` | 3 entrées × 4 transformations |
| `mhgp11_catalogue_diff_v10` | `diff_v10` | empreinte du dump v10 (binaire figé) sur les mêmes entrées | 8 000 à 32 000, trames, K = 5 et 10 |

Tranche 3 — tour.

| Porte | Label | Contenu | Plancher |
| --- | --- | --- | --- |
| `mhgp11_tower_oracle` | `oracle` (+ échantillon `fast`) | juge à témoins (R7, S12) contre Γ ; en Python avec cache des miniboules jusqu'à n = 12 ; **K jusqu'à 10** à taille utile par un second oracle C++ à arithmétique autre (n ≤ 20) et par un oracle Γ tronqué en niveau (n ≤ 60, coupes basses) ; pleine magnitude | par strate (R4) ; ≥ 200 multifusions, ≥ 100 descentes à 3 pas |
| `mhgp11_tower_emst` | `scale*`, `lidar` | ordre 1 contre l'arbre couvrant exact (multiensemble des (niveau, taille) des fusions) | chaque entrée d'échelle |
| `mhgp11_tower_certificates` | `scale*`, `lidar` | juge d'échantillon exact : pour des fusions tirées, la boule témoin a le niveau du nœud et relie ses représentants ; pour des descentes tirées, chaque pas est une arête de $\Gamma_k$ de niveau décroissant ; pour des points tirés, l'entrée est la k-ième distance par force brute | ≥ 1 000 fusions et 1 000 descentes par ordre |
| `mhgp11_tower_verticals` | `scale*` | naturalité sur tous les nœuds, image vivante au niveau de création | tous les nœuds |
| `mhgp11_tower_identity` | `scale8000`, `lidar` | empreinte de la tour FULL identique à 1, 2, 8 fils, sous permutation, renumérotation, translation | 3 entrées × 4 transformations |
| `mhgp11_tower_refusals` | `fast` | 48 points cosphériques (refus de dégénérescence ou, mieux, réponse exacte), multiplicités, K hors domaine | chaque raison |
| `mhgp11_tower_diff_v10` | `diff_v10` | dump canonique égal à celui du binaire v10 figé | trames, K = 5 et 10 |

Tranche 4 — hiérarchie de points et tête.

| Porte | Label | Contenu | Plancher |
| --- | --- | --- | --- |
| `mhgp11_points_oracle` | `oracle` | la règle retenue pour passer de FULL aux points, jugée contre sa **définition** calculée sur Γ par force brute (niveaux exacts, pas à 1e-9 près) | n ≤ 14, K ≤ 5 |
| `mhgp11_points_fixtures` | `fast` | 5 points d'égalité d'attache ; 9 points de l'attache `cover` ; deux triangles équilatéraux ; fixtures retenues de la bibliothèque S16, chacune avec son verdict attendu | toutes |
| `mhgp11_head_oracle` | `oracle` | condensation et sélection contre un oracle `Fraction` écrit depuis la définition, sur dendrogrammes quelconques (points attachés à des nœuds internes, plateaux N-aires, poids), exposant z compris | ≥ 10^4 dendrogrammes ; ≥ 10^3 où z change la sélection |
| `mhgp11_head_sklearn` | `sklearn` | égalité à scikit-learn là où les objets coïncident, version épinglée, entrées sans ex æquo vérifiées par la porte | 360 contrôles |

Tranche 5 — bancs et contrat LiDAR.

| Porte | Label | Contenu |
| --- | --- | --- |
| `mhgp11_bench_metrics` | `fast` | IoU par appariement optimal, précision, rappel, ARI : petits cas calculés à la main ; schéma des préenregistrements validé avant tout calcul |
| `mhgp11_lidar_pins` | `lidar` | sha256 des trois trames du contrat (`0baa4de1…`, `ba15adc6…`, `a4bbc86d…`), empreintes épinglées de leur tour à K = 5 et K = 10 ; données absentes = porte non enregistrée et `ctest --no-tests=error -L lidar` en échec, jamais un vert |
| `mhgp11_zoltan_demos` | `lidar`, `sklearn` | par démo et par objet suivi, le meilleur IoU d'un nœud de la hiérarchie HGP à côté de la valeur enregistrée d'HDBSCAN ; le calcul d'HDBSCAN passe par scikit-learn (`dbscan_clustering` pour les coupes), jamais par une réimplémentation |

### 11.3 Mutants à livrer avec les portes

| Mutant | Défaut | Porte tueuse attendue |
| --- | --- | --- |
| attache `core` stricte ; rang `cover` sans + 1 | égalité d'attache | `points_fixtures`, `tower_oracle` |
| image verticale un rang trop bas | verticale d'une naissance | `tower_oracle`, `tower_verticals` |
| fusion binarisée ; union sautée une fois sur 2 048 | Kruskal | `tower_oracle`, `tower_emst`, `tower_certificates` |
| représentant de jonction résolu vers une mauvaise naissance | descente | `tower_oracle` (bijection), `tower_certificates` |
| admission décalée d'une couche, par q | catalogue | `catalogue_oracle`, `catalogue_euler` |
| boule perdue rarement, à grande magnitude, dernière couche puis toutes couches | complétude à l'échelle | `catalogue_restriction`, `catalogue_euler` |
| recensement : intérieur strict devenu large | coquille | `catalogue_oracle`, juge d'échantillon |
| repli exact supprimé, filtre laissé seul | doctrine flottante | `num_filters` |
| comparaison d'entiers larges ignorant le mot de tête | arithmétique | `num_wide` |
| seuil de masse strict ; exposant z ignoré | tête | `head_oracle` |
| niveaux publiés non fusionnés à la collision de doubles | sortie | `points_fixtures` |
| refus de tout K ≥ 10 | domaine | `tower_oracle` à K = 10 |

Dix de ces défauts (attache `core`, rang `cover`, verticale, admission, les deux pertes rares, seuil de masse, z,
collision de niveaux, refus de K ≥ 10) ont été joués contre la suite de la v10 dans cet audit (§ 5.5) ; cinq ont
survécu. Les autres restent à écrire. La colonne « porte tueuse » dit ce qui manquait.

### 11.4 Fixtures permanentes à reprendre (coordonnées gravées, verdict attendu, origine)

| Fixture | Coordonnées ou source | Ce qu'elle garde |
| --- | --- | --- |
| E5 | `(0,0,7) (0,9,6) (1,4,0) (0,0,1) (4,1,2)`, K = 4 (`reference/test_ref.py:10`) | petit nuage générique historique, toutes les coupes |
| Carré, cube | `{0,2}^2 × {0}`, `{0,2}^3` (`reference/test_ref.py:59-63`) | cocyclicité, cosphéricité, K = 3 |
| AUDIT3, LINE5, LINE4, PAIR | alignés (`tests/oracle/test_tower_oracle.py:597-610` de l'arbre R2) | verticales avant l'entrée des points, bloc d'attaches vide |
| TRIANGLE, OCTA | même source | fusion d'ordre 2 sans point entré ; coquille de 6 |
| Égalité d'attache, 5 points | `(2,1,1) (3,0,0) (3,1,0) (3,1,2) (3,2,2)`, K = 2, mcs = 2, z = 3 | convention fermée : `[-1,-1,-1,-1,-1]` en v10 ; la convention stricte donne `[-1,0,0,1,1]` |
| Attache `cover`, 9 points | `(0,2,1) (1,0,2) (2,0,1) (2,1,0) (3,0,0) (3,2,2) (3,2,3) (3,4,1) (4,2,0)`, K = 3, mcs = 2, z = 3 (`build/v10-giant-audit/verif_moteur_livre/preuves/fixtures_egalite.txt`) | rang de l'attache `cover` : `[0,0,0,0,0,0,1,1,0]` en v10 |
| 48 points cosphériques | permutations signées de `(3,5,7)` translatées de 100 | refus `shell_quotient_budget` en v10, y compris à K = 1 ; à remplacer par la réponse exacte si la v11 lève le budget |
| Grilles dégénérées de l'audit | `scripts/euler_general.py` (graine 5) : 8 nuages de 14 à 40 sites, coquilles jusqu'à 15 | Euler général et oracle du catalogue au-delà de 22 sites |
| Six familles de pleine magnitude | `scripts/oracle_pleine_magnitude.py` (graine 20261002) | arithmétique exacte aux bornes du domaine ; à régénérer au domaine de chaque profil |
| Translation | 3 000 points de `[0, 20000]^3` et leur translaté de 200 000 (`test_catalogue_oracle.py:84-111`) | régression `wide_leaf` du 28 septembre |
| Multiplicité | 40 points dont un doublon (`test_multiplicity_refusal.py`, graine 20260929) | refus nommé, témoin positif |
| Collision de niveaux | scène `anisotropic`, 8 000 points, graine 171554247519005774, K = 8 : **à graver en octets** avec son sha256, ou à réduire à une fixture minimale | deux niveaux exacts distincts de même `double` |
| Course du pool | alternance de travaux de 1 à 3 indices et de 64, 50 000 fois, 4 et 8 fils | régression `8e3b76245` |
| Recherche de rang | tailles autour de 2^31 et 2^32 (`tests/unit/rank_search.cpp:44-52`) | dépassements de l'ancienne dichotomie |
| Deux triangles équilatéraux et bibliothèque « tour vers points » | `build/v10-verrou-points/fixtures_cibles/fixtures_catalogue.json` (35 fixtures, hors dépôt) | cas où la hiérarchie d'HDBSCAN échoue dès K = 2 ; cibles de la règle FULL vers points |
| Trames du contrat | sha256 `0baa4de1…abaf`, `ba15adc6…036f`, `a4bbc86d…08af` (jamais les octets) ; comptes de boules et comptes par (q, p) de `journaux/divers.txt` § 8 | épingles de non-régression, K = 5 et K = 10 |

## 12. Reproduction et preuves

Dossier de calcul (non durable) : `/tmp/v11-audit/l08_tests_portes/`. Preuves durables (moins de 1 Mo) :
`/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l08_tests_portes/`.

```text
# copie de lecture et builds
git -C /workspaces/E-HGP/build/v11-worktree archive afb081774 morsehgp3D_v10/{CMakeLists.txt,cmake,src,cli,tests,reference,bench} | tar -x -C SRC
cmake -S SRC/morsehgp3D_v10 -B build-gcc   -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake -S SRC/morsehgp3D_v10 -B build-clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake -S SRC/morsehgp3D_v10 -B build-asan  -DCMAKE_BUILD_TYPE=Release -DMHGP10_SANITIZE=ON
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir build-gcc --output-on-failure
(cd SRC && python3 -m unittest discover -s morsehgp3D_v10/reference -p 'test_*.py')

# epreuves (scripts/ du dossier de preuves)
python3 strates_oracles.py build-gcc SRC/morsehgp3D_v10
python3 juge_tour_aveugle.py build-gcc SRC/morsehgp3D_v10 6 5
python3 juge_r2_contre_faux.py build-r2 R2SRC/morsehgp3D_v10 6 5        # arbre R2 : git archive 865f5e6
python3 oracle_pleine_magnitude.py build-gcc SRC/morsehgp3D_v10 20261002
python3 oracle_k10.py build-gcc SRC/morsehgp3D_v10 40 8
python3 mutants.py prepare M01_core_strict ; python3 oracles_rapides.py build-gcc mut/M…/build SRC/morsehgp3D_v10
python3 gen_uniforme.py 8000 3 uniforme_8000_g3.u32le
python3 emst_k1.py build-gcc uniforme_8000_g3.u32le 1
python3 euler_echelle.py build-gcc 8000 12 3 1 uniforme_8000_g3.u32le
python3 euler_general.py build-gcc SRC/morsehgp3D_v10 V9/audits/c_euler_20260923/euler_degenerate.py lidar00_full.u32le 7 2
python3 determinisme.py build-gcc lidar00_full.u32le 5 --points 1 2 4
python3 determinisme_tour.py build-gcc lidar00_full.u32le 10 1 4
python3 head_egale_r2.py build-gcc build-r2 SRC/morsehgp3D_v10
```

| Preuve | Fichier (sous `preuves_l08_tests_portes/journaux/`) |
| --- | --- |
| Suites CTest | `ctest_gcc_release.txt`, `ctest_clang_release.txt`, `ctest_asan_ubsan.txt` |
| Référence | `reference_unittest.txt` |
| Strates des oracles | `strates_oracles.json` |
| Dumps faux, juge de HEAD puis juge R2 ; porte R2 complète | `juge_tour_aveugle.out`, `juge_r2_contre_faux.out`, `juge_r2_porte_complete.txt` |
| Oracles hors strates | `oracle_pleine_magnitude.out`, `oracle_k10.out` |
| Mutants | `mutants_bilan.txt`, `mutant_M01_core_strict.txt`, `mutants_rares_echelle.json` |
| Juges d'échelle | `emst_k1.txt`, `euler_echelle.txt`, `euler_general.txt` |
| Déterminisme | `determinisme_uniforme8000_k5.json`, `determinisme_lidar00_k5.json`, `determinisme_lidar00_k10.json` (catalogue ; y figurent aussi les trois arrêts par signal de `--no-points --dump`), `determinisme_tour_lidar00_k10.json` |
| Divers | `divers.txt` (labels, configuration sans Python, numpy absent, vacuité, `run_expect`, fixtures de 5 et 9 points, 48 points cosphériques, compteurs des trames, arrêts par signal des CLI, TSan) |

Empreintes utiles à la v11 (sha256 des dumps texte de la v10, `afb081774`, GCC 13.3 Release ; valeurs complètes dans
les fichiers JSON) : `lidar00` à K = 5, catalogue `3982c3ab…7676`, tour FULL avec attaches `core` `091f2620…d048` ;
`lidar00` à K = 10, catalogue `7b2d4055…a89c`, tour FULL avec attaches `core` `4de0a478…78c7`. Comptes de boules des
trois trames à K = 5 : 1 306 696, 1 095 926, 1 407 885 ; à K = 10 : 5 512 670, 4 383 302, 5 483 320 (ceux de la trame
02 sont les nombres de `docs/SPEC_V10.md:84-85`).

Limites de cet audit. Mesures locales sur machine chargée : aucun temps n'est une performance. Les campagnes de
mutants du raccord R2 et ses juges du catalogue et de la tête sont lus, pas rejoués. La stabilité de la porte
scikit-learn entre machines n'est pas vérifiée. GCP non utilisé.

Clôture : 08 h 33 UTC (lue par `date -u`).
