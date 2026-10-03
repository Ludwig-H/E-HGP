# Provenance

La v11 est une base de code neuve. Ce qui vient de la v10 est un **port explicite** : la source est épinglée, les
adaptations sont dites, et le composant porté a ses propres portes dans la v11. Rien n'est repris implicitement.

La qualification reste liée à une source et à une capture. L'[état courant](DEVELOPPEMENT.md)
et le [reçu G4 du 3 octobre](../receipts/qualification_performance_20261003/README.md)
identifient la source jugée `c40f40798`, distincte de la publication des lecteurs et documents.
Les chemins d'anciens worktrees décrivent leur lieu de capture ; leurs empreintes restent les identités
des sources après nettoyage et sauvegarde.

## Sources

- **v10 publiée** : `origin/main` au commit `afb081774`, dossier `morsehgp3D_v10/`.
- **Raccord R2 de la v10** : série de réparation issue des audits des 29 et 30 septembre 2026, dont les campagnes documentées sur l’extraction `5c2fe1f` annoncent
  GCC/Clang 82/82, ASan/TSan 80/80 et 425 mutants relus ; jamais importée dans `main`.
  Ces résultats ne qualifient pas automatiquement tous les changements ultérieurs de `865f5e6`.
  Dépôt local `build/v10-integration-r2/src`, commit `865f5e6`. Les fichiers portés depuis ce dépôt sont épinglés par
  leur empreinte sha256 ; l'archive de la série est conservée hors dépôt (`build/v10-integration-r2/series`).

## Règle

Une ligne par fichier porté : fichier v11, source (dépôt, commit, chemin, sha256), adaptations, portes v11 qui le
requalifient. Un fichier écrit à neuf n'apparaît pas ici.

## Table

Reprise développeur du 2 octobre 2026, après `b104028f5` :
`tests/support/expect_abnormal_stop.py` est un juge nouveau du statut POSIX,
issu du contre-exemple de faux verdict de l'audit, sans port de code v10.
`cmake/gates.cmake` l'appelle à la place des deux wrappers textuels imbriqués ;
les portes et mutants correspondants sont dans `tests/support` et
`tests/mutants/core.json`. Le correctif existant de lancement impossible
dans `run_expect.cmake` est conservé.
`reference/test_projection_contracts.py` est un attendu nouveau, complété à quatre
faits, dérivé des fixtures exactes de
`receipts/audit_full_hierarchie_20261002/suivi_verrous/points_review` et
`tower_review`, puis raccordé aux deux étages de référence existants.
Il ne contient ni moteur HDBSCAN ni sélecteur. Les nouvelles portes sont
qualifiées séparément sur G4 ; voir `DEVELOPPEMENT.md` pour leur état courant.

« R2: » désigne `morsehgp3D_v10/` dans le dépôt local du raccord R2 au commit `865f5e6`. Les modules `sched`, `io` et le CLI s'ajouteront à leur livraison. Aucun compte de portes de la v10 n'est hérité : seules les
portes citées en dernière colonne qualifient le fichier porté.

### Socle

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/core/types.hpp` | R2:src/core/types.hpp ; garde reprise de src/core/fp_strict.hpp (sha256 29a64fdbae5821058c6ce7870d64cce0056c75b8014b75a517b30d4bbe1da4c8) | `e4148ac778dbd443bf743495fe17743aff08e976e77db6d154bd7765e2db8d8c` | namespace mhgp11 ; kCoordinateBits devient kCoordBits = MHGP11_COORD_BITS (definition de compilation obligatoire, static_assert 18, 21 ou 24) ; kCoordinateLimit (i64) devient kCoordMax (u32) ; kMaxOrder et kMaxCatalogueOrder retires ; concept StrongId sur idx et make_id ; les cinq domaines d'identifiants gardes (architecture 7.3) ; kNone declare sentinelle des rangs denses seulement ; de fp_strict.hpp ne reste que le #error sur __FAST_MATH__ plus un static_assert binaire64 ; static_assert de plate-forme (size_t 64 bits, i128). | mhgp11_core_unit_types, mhgp11_core_fast_math_refusal, mhgp11_core_ofast_refusal, mhgp11_core_coord_bits_refusal, mhgp11_core_coord_bits_absent, mhgp11_core_strong_id_naked, mhgp11_core_strong_id_mixed ; mutants garde_fast_math_retiree, profil_de_20_bits_admis, coordonnee_maximale_decalee, identifiants_confondus |
| `src/core/status.hpp` | R2:src/core/status.hpp ; guarded repris de src/core/cli_options.hpp (sha256 931dd43764d8c0dee7ffa45c10ad0827a795d120cf5c9329c5e7e79316dbb917) | `f6983f95f60195eb5b5f73f73d9cc97b48d694d0877efd226072d6240cf84038` | Outcome [[nodiscard]] et operator== ; fail(Reason::none) rend refusal_without_reason ; Result a stockage discrimine (std::optional), constructeurs noexcept, T a deplacement non levant, value() et take() verifies (std::terminate sur un refus), take() && ; ajouts merge (fusion deterministe), exit_code (0, 2, 3), kReasonCount, MHGP11_TRY ; cli::guarded devient guarded, outil commun des frontieres de module ; X-macro a trois arguments. | mhgp11_core_unit_reasons, _outcome, _macros, _result, _guarded ; mhgp11_core_fault_refusal ; mhgp11_core_result_value_ok, _value_on_refusal, _const_value_on_refusal, _take_on_refusal ; mhgp11_core_refusal_ignored, mhgp11_core_result_throwing_move ; douze mutants de status.hpp dans tests/mutants/core.json |
| `src/core/reasons.def` | R2:src/core/reasons.def | `ccd20d6930e1b13f66e84ffefc4c0ee25764f0f01ade1833733b1ce1417898f9` | Table reprise a neuf : 30 raisons deviennent 14. Douze gardees dans leur ordre relatif de la v10 (none, empty_input, size_mismatch, coordinate_out_of_domain, duplicate_point_id, parameter_out_of_range, memory_budget, index_overflow_u32, session_overhead, input_unreadable, output_unwritable, output_conflict), deux nouvelles en fin de table (refusal_without_reason, budget_not_released), dix-huit retirees (moteur et raisons jamais emises, csr_bounds comprise). Troisieme argument : module emetteur, lu par check_style. | mhgp11_core_unit_reasons (table gravee, 53 controles), mhgp11_style (regles raison_morte et raison_sans_porte), mhgp11_support_check_style ; mutants statut_d_une_raison, ordre_de_la_table |
| `src/core/buffer.hpp` | R2:src/core/buffer.hpp | `2b5f66fc39e4cb550a6a13585efa5b116e3fc34cdeface94271a15c9fe8f7c60` | default_budget retire ; limite explicite ; reserve et release retires de l'interface publique ; compte partage entre MemoryBudget et tampons ; ajouts admit, restart_peak, released ; allocate rend Outcome ; allocate_zero retire ; garde kMaxCount avant le produit ; static_assert d'alignement ; empoisonnement deplace dans buffer.cpp ; Csr a decalages u64 et rows() u64 (architecture 7.3). | mhgp11_core_unit_budget (57 controles), _budget_threads (14), _buffer (1035), _csr (25) ; mhgp11_core_fault_alloc_fault ; mhgp11_core_buffer_trivial_only ; mhgp11_core_poison (sous MHGP11_POISON) ; treize mutants de buffer.hpp et buffer.cpp |
| `src/core/buffer.cpp` | R2:src/core/buffer.cpp ; corps de MemoryBudget::reserve et de Buffer::allocate repris de buffer.hpp (sha256 2b5f66fc39e4cb550a6a13585efa5b116e3fc34cdeface94271a15c9fe8f7c60) | `d4d67d329a0fadc59ab0870c094f380a1a501b369a0da4915cc8a5e66dfe5d81` | default_budget (seul contenu du fichier source) abandonne ; le fichier recoit la reservation par echange compare, le suivi du pic, l'allocation sans exception, la restitution de la reservation quand l'allocation echoue et l'empoisonnement 0xA5 sous MHGP11_POISON (definition PRIVATE). | mhgp11_core_unit_budget, mhgp11_core_unit_budget_threads, mhgp11_core_fault_alloc_fault, mhgp11_core_poison ; mutants budget_depasse, pic_non_suivi, reservation_gardee_apres_echec, liberation_non_comptee, poison_efface |
| `cmake/run_expect.cmake` | R2:cmake/run_expect.cmake | `dfb1b6456913276d1bd24e1f0a0bac523c548111a7efef5ad563899d03de40d1` | Arguments passes par definitions numerotees (NARGS, ARG0...) rejouees par cmake_language(EVAL CODE) au lieu d'une chaine ARGS redecoupee ; EXPECTED borne a 0..4 ; lancement par /bin/sh -c exec pour constater a l'execution un lancement impossible (126, 127) ; ligne de verdict run_expect_verdict ; saut des portes lidar reserve au precontrole, jeton usurpe refuse ; cmake_minimum_required 3.20 dans le script. | mhgp11_support_code_exact, _line_present, _arguments, _code_mismatch, _signal_refused, _program_absent, _program_not_executable, _program_bad_interpreter, _program_relative, _line_absent, _expected_out_of_range, _skip_token, _data_present, _skip_token_forged, _lidar_sentinel, _abnormal_stop* ; sept mutants de run_expect.cmake |
| `cmake/gates.cmake` | R2:cmake/gates.cmake | `a304bee130b92d68d49d362ce8d629ef0ed072483a1f4402ce3daf906ca48efc` | mhgp10_gate a mots-cles remplace par mhgp11_add_unit, mhgp11_expect_code(nom code cible args), mhgp11_python_gate (jumelle _opt sous PYTHONOPTIMIZE=1 sauf label long, ordonnee apres la porte), mhgp11_expect_refusal, mhgp11_expect_compile_failure, mhgp11_expect_abnormal_stop ; liste fermee de labels, fast et long exclusifs ; PYTHONDONTWRITEBYTECODE sur chaque porte ; RUN_SERIAL pour scale*, lidar, mutant ; registre recursif et proprietes interdites ; jetons graves mhgp11_porte_*. | mhgp11_support_gate_properties (65 controles), seize portes mhgp11_support_gates_refuse_* ; douze mutants de gates.cmake |
| `CMakeLists.txt` | R2:CMakeLists.txt | `8a2453e32d1494b2894a5d5951a5c4605e8478ba39e88926cc1814807cd19ec4` | Options MHGP10_* deviennent MHGP11_* ; ajouts MHGP11_MODULES, MHGP11_COORD_BITS, MHGP11_MUTANT_JOBS ; bibliotheque unique mhgp11 alimentee par src/<module>/module.cmake, portes par tests/<module>/tests.cmake ; bloc Clang -shared-libsan porte ; fp_flags.cmake, fp_probe.cpp, -ffp-contract=off, compile_commands et MHGP10_FAST_TARGETS non portes ; garde fast-math par interrogation du compilateur (-dM -E) ; portes mhgp11_style et mhgp11_mutants_<unite>. | mhgp11_support_configure_fast_math_cxx_flags, mhgp11_support_configure_fast_math_release_ofast, mhgp11_style, mhgp11_mutants_core_manifest ; mutant sonde_fast_math_retiree ; configuration constatee sous CMake 3.28.3 et 3.22.1 |
| `cmake/expect_refusal.cmake` | R2:tests/regression/site_tree_fast_math.cmake (principe du temoin positif puis du refus avec jeton grave) | `0cf96ef3628c49cf2d4b387d51abef6c441f58f9b3c5b8c222e07a467ddf9fe2` | Seul le principe est repris, generalise a toute commande (compilation ou configuration) ; listes d'options par compilateur et controle de contraction FMA abandonnes. | mhgp11_support_refusal_ok, _refusal_control_red, _refusal_absent, _refusal_token_absent, _refusal_signal ; mutants temoin_du_refus_ignore, jeton_du_refus_ignore, refus_non_exige |
| `tests/core/status_test.cpp et tests/core/buffer_test.cpp` | R2:tests/unit/unit_main.cpp, fonction test_status_and_buffer | `d9dda6e2a67051558f371c6680faef1e092301be04d0b42a8e6bcd0b44f5fa27` | Les dix attentes de la v10 (statuts, priorite du plus petit K, budget de 1000 octets, liberation, CSR) sont reprises dans le cadre tests/support/test.hpp et etendues, avec un plancher par test. | mhgp11_core_unit_* (onze groupes), mhgp11_core_unit_inventaire |

### Oracle de référence

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `reference/hgp11_ref/definition.py` | R2:reference/hgp10_ref.py (commit 865f5e6 ; fonctions solve, circumcenter, meb, DSU, gamma_cuts, entry_level, knn_vertex, point_partition_gamma) | `2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104` | Étage A isolé, sans aucune notion du moteur. Doublons permis (points par indice). Boule minimale = plus petite sphère circonscrite d'un support de 1 à 4 points qui contient la partie (le filtre barycentrique de la v10 est retiré : il était superflu), boule fermée testée en entiers. Ajouts : arbre de fusion canonique (naissances, fusions N-aires par plateau), application verticale, entrées core et cover (ensemble de tous les nœuds couvrants), coupes ouvertes et fermées par nœud, numérotation canonique et lecture de coupe propres à l'étage. | mhgp11_reference_fast, mhgp11_reference_fast_split, mhgp11_reference_full_<i> et mhgp11_reference_full, mhgp11_reference_diff_v10 (tours sérialisées depuis A), mutants edge_at_vertex_level, meb_largest, numbering_by_sweep |
| `reference/hgp11_ref/constructive.py` | R2:reference/hgp10_ref.py (critical_balls, catalogue, local_structure, descend, build_order, point_partition_tower) ; règles de src/tower/tower.cpp (fenêtres, raccourci des coquilles régulières, first_rep, resolve, kruskal, verticales, entrée cover) et de src/catalogue/generator.cpp (admission) | `hgp10_ref.py 2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104 ; tower.cpp a23ed15546195ed75044b7d8e9ea7920c7079d3dfc1f3b23ee5851dcfe07671e ; generator.cpp 4647e90297b5ade1195f17c715ad79841409799fbd92d056991e82a85aa34cc1` | Prédicats en entiers (formules du moteur) au lieu de Fraction. Sites en ordre de Morton avec poids ; boules de rayon nul gardées dans le catalogue interne. Descente du moteur (saut aux k plus proches, premier représentant) au lieu du représentant de plus petit niveau. Kruskal par plateaux produisant des nœuds de fusion N-aires ; verticales par naturalité ; relation de couverture par boule ; coupes par balayage. Règle d'admission au choix ('v10' par défaut, 'single'). Numérotation canonique et recherche d'ancêtre propres à l'étage. | mhgp11_reference_fast, mhgp11_reference_fast_split, mhgp11_reference_full, mhgp11_reference_diff_v10 et mhgp11_reference_diff_v10_large, 19 portes mhgp11_reference_mutant_<nom> visant ce fichier |
| `reference/hgp11_ref/intgeom.py` | R2:src/arith/geometry.hpp, src/arith/geometry.cpp, src/catalogue/support.hpp, src/cloud/cloud.cpp (morton3) | `geometry.hpp e5954da39ab6c216bfb05535158cf862eca9a4ba78a81f7dd0c33b6be1fed673 ; geometry.cpp 0e98cf5050c64e880386736de9aa47779fae377db0c9fe7acc64dfcf6c039de2 ; support.hpp 7cc62930f623e99b598954c6b9f2f5e73e4ee4f0438426c55d18041d56d1a11d ; cloud.cpp 9cfe9f5b7c86eef3430d9d3d4835ebab5c70add30bf57480edbad1e830f44679` | Entiers Python sans borne : aucun budget de bits ni garde de débordement. Les tests d'enveloppe convexe fermée deviennent une liste de témoins (masques) calculée une fois par coquille. Cas du rayon nul ajouté. | mhgp11_reference_fast (fait test_minimal_balls_two_routes : 635 parties, entiers contre fractions), mhgp11_reference_diff_v10, mutants hull_no_pairs, hull_no_tetra, hull_strict_triangle (équivalent) |
| `reference/hgp11_ref/dumps.py` | R2:cli/mhgp10_catalogue.cpp et cli/mhgp10_tower.cpp ; emitted_level de src/catalogue/generator.cpp ; format effectivement comparé : CLI du binaire figé build/v10-bench-c764e121a/src/cli/ | `R2 mhgp10_catalogue.cpp d5b2c057a4e865199950695306ebe64e1216da76d2019a9a8a29108e1e3338c8 ; R2 mhgp10_tower.cpp 0a2197692b888cd67fbf30f08b799fe404b1dccd8495e201ce8668a5131c52bf ; figé mhgp10_catalogue.cpp 993800fc0296fc1497b3d67f20f1a49677ab06cde4ac071068e57c2db2543f2c ; figé mhgp10_tower.cpp de2bf2750e2df25e58ce89d6f0d155764f50a9325555f81fefb3116e571e604e ; binaires figés mhgp10_catalogue e6f2cd70055c0a7c74c59d4851d7041696be46392f884c043dd859d5ab8b60c5, mhgp10_tower 7261274b26c510b0c7ab6079187edbf47d33888390eb475b752b0225d9f662b8` | Sérialisation depuis le modèle canonique : renumérotation vers la convention v10, écriture non réduite des niveaux, choix cover de la première boule couvrante. Options des juges du raccord R2 non reproduites. Ajout de reduce_tower_levels. | mhgp11_reference_diff_v10, mhgp11_reference_diff_v10_large, 10 portes mhgp11_reference_diff_v10_mutant_<nom> |
| `reference/hgp11_ref/families.py` | R2:tests/oracle/test_catalogue_oracle.py (FIXTURES, clouds), tests/oracle/test_tower_oracle.py (AUDIT3, E5, TRIANGLE, LINE5, SQUARE, OCTA, CUBE, PAIR, LINE4), reference/test_ref.py | `test_catalogue_oracle.py 1f40aae68e08116f37c4de9caa25da7841f54bea19c70eba791ab1306d1c3d86 ; test_tower_oracle.py 5f4be00c5fc9fe72062d09d220b3758e7c0281c9bf7224438f23a05ecee6ac35 ; test_ref.py cdfed34cdbcd625fd4152eee8b2fce0c3b1e06c1d9359645eafba2e41f1e8006` | Coordonnées ramenées dans [0, 2^18). Générateur SplitMix64 écrit dans le fichier (la suite ne dépend plus du module random). Fixtures ajoutées : deux triangles de la thèse (trois ponts), collisions de niveaux, poids, égalité cover, tétraèdre et centre. Familles ajoutées : alignés, cocycliques, cosphériques, extrêmes u18, doublons. | mhgp11_reference_fast (compteurs exacts, faits test_generator_is_engraved et suivants) |
| `reference/hgp11_ref/judge.py` | R2:tests/oracle/test_tower_oracle.py (règles de structure : plateaux atomiques, fusion d'au moins deux composantes antérieures, nœud vivant à la coupe fermée) | `5f4be00c5fc9fe72062d09d220b3758e7c0281c9bf7224438f23a05ecee6ac35` | Compare deux résultats canoniques champ par champ au lieu de juger un dump par témoins. La variante admise par le juge R2 (naissance de durée nulle sous une fusion) n'est pas admise. | mhgp11_reference_fast, 25 portes mhgp11_reference_mutant_<nom> |
| `reference/test_ref.py` | R2:reference/test_ref.py | `cdfed34cdbcd625fd4152eee8b2fce0c3b1e06c1d9359645eafba2e41f1e8006` | Faits gravés à valeurs exactes, attendu d'intervalles, tranches et somme de rapports, compteurs exacts, mutants, codes de sortie des portes. La v10 ne déclarait pas ce fichier comme porte CTest. | mhgp11_reference_fast, mhgp11_reference_fast_split, mhgp11_reference_refusal, mhgp11_reference_full |

### Outillage de session G4

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `gcp-migration/v11_session.py (sha256 f32d057b0535cc890c25859324ef82ff051b1399ce37a996ee3a6ec7d1f6aa72)` | gcp-migration/v10_session.py (fichier inchangé depuis le commit 11d7ad25f, v10 publiée) | `60423f8015555ee36c75886f840c2b4a605d84a88cf69bb1dabc984ae634ab1a` | Lignée (morsehgp3D_v11, mhgp11*, $HOME/ehgp-v11/, ehgp-v11-runs, schémas ehgp.v11.* à v1) ; verrou gardé égal à .ehgp-v10.lock (constante ROOT_LOCK_NAME) ; mode --snapshot (resolve_snapshot, open_beneath, read_regular, snapshot_files, build_snapshot_package) ; validate_plan : python_packages, default_build, cibles par leur forme, cmake_executables supprimée ; reçu : source_kind, evidence_grade, source, vm_facts ; worker lancé avec --source. Les 75 autres fonctions, dont tout le cycle de vie, sont identiques après normalisation. | gcp-migration/v11_selftest.py (58 scénarios hors ligne, dont les 47 de la v10) ; relevé build/v11-persist/fondations/g4_diff_check.py |
| `gcp-migration/v11_worker.sh (sha256 bafdb355f1d12a81ed8d4c3431ae293dc3306139ee1b22604fd90145a324b5c0)` | gcp-migration/v10_worker.sh (commit 11d7ad25f) | `a81e81183e84d8df9f705085a1ed803d8d0df8c15e246474bb4ff80ee5cfb1a6` | Lignée ; --source NATURE:SHA à la place de --commit, forme contrôlée ; PLAN_PYTHON_PINNED (aucun contrôle pip par défaut) et PLAN_DEFAULT_BUILD (construction facultative, statut not_requested) ; faits de la VM dans env/vm_facts.txt (versions, fils, mémoire, sondes de sanitizers, drapeaux du processeur) ; export de MHGP11_DATA_DIR ; binaires relevés mhgp11*. 14 fonctions sur 16 identiques (run_step, close_group, enforce_results_cap, on_signal…). | gcp-migration/v11_selftest.py : le vrai worker tourne dans chaque scénario de session ; test_worker_refuses_a_malformed_source, test_default_build_false_…, test_python_packages_…, test_completed_session (faits de la VM) |
| `gcp-migration/v11_selftest.py (sha256 47dcf61b9a647a5afcf38aa4d1f975c6c2e8b39e864108b4624c30002c635ce1)` | gcp-migration/v10_selftest.py (commit 11d7ad25f) | `ddde38fe4bc7f1129abc5aaaeb29dfaa7d317c074f369c63d3c0cddc4864163b` | Les 47 scénarios repris ; mini projet à cibles déclarées par une fonction d'aide CMake, avec le CLI mhgp11 ; scénarios pip passés à python_packages = pinned ; cas « binaire inconnu » retiré, 17 cas de plan invalide ajoutés ; 11 scénarios nouveaux (instantané, python_packages, default_build, source du worker, matrice dans une session, exclusion mutuelle avec le vrai v10_session.py). | Il est lui-même la porte ; exécuté trois fois en entier (résultats dans « essais ») |
| `gcp-migration/README_V11.md (sha256 10f6d44aa8b4d5429c26b8b7c69982e89f8f78cd9fbe00f74c0ac26eeae91805)` | gcp-migration/README_V10.md | `f300bc98986311cfbf87461754a64b4261af8a449bafcc0b39457aa7f542e920` | Réécrit en mode d'emploi court (86 lignes) : il renvoie à README_V10.md pour le déroulé et les garanties inchangés, et ne décrit que les deux modes de source, le plan, le verrou partagé, la reprise et la valeur d'un reçu d'instantané. | tools/check_docs.py : aucune ligne sur ce fichier (le contrôle échoue par ailleurs pour des raisons antérieures) |

## Cloud : reprise explicite du travail préparatoire

Source intermédiaire : WIP non publié `build/v11-worktree`, HEAD `2f9eb838a`,
sans qualification héritée. Les empreintes complètes, dont les deux sources
v10 R2 sous-jacentes, sont dans [`source_pins.json`](../tests/cloud/source_pins.json).

| Fichier v11 | Source WIP | SHA256 de la source | Adaptation | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/cloud/cloud.cpp` | WIP:`src/cloud/cloud.cpp` | `9271c4641acad6d54ebc656f8784f34dc5fb42d6b246e97341b5c2b4e0ca111e` | Factory amie, vues mutables internes seulement | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `src/cloud/cloud.hpp` | WIP:`src/cloud/cloud.hpp` | `2bb40a6d372857a036801eb23236eccd1b42f0d744bd27ef114cec972874c1fa` | Stockage privé, vues constantes, copie/affectation interdites, déplacement sans allocation | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `src/cloud/module.cmake` | WIP:`src/cloud/module.cmake` | `81d1470bc740f0fa7aca00e56eab47ee72700b848bae91b99a9fad4998a050b5` | Tri séquentiel, dépendance core seule ; calcul Morton conservé | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `src/cloud/morton.hpp` | WIP:`src/cloud/morton.hpp` | `a58062c4fa9225558b2154faf176ad55b1ea4f638abce8242a4a6f99bc04ba71` | Tri séquentiel, dépendance core seule ; calcul Morton conservé | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `tests/cloud/cloud_fault.cpp` | WIP:`tests/cloud/cloud_fault.cpp` | `c848841a72d5a91bb5d89fcac95874d8354ae6cd537b1713fbb4897b52277d99` | Ajout des tests de propriété, refus API, budget partagé ; frontière sans inlining pour injection | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `tests/cloud/cloud_test.cpp` | WIP:`tests/cloud/cloud_test.cpp` | `fa05de6bd057cabbe0e763bb525671e0ba9435a2fb1851b6b8f0c212cc89b8b1` | Ajout des tests de propriété, refus API, budget partagé ; frontière sans inlining pour injection | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `tests/cloud/tests.cmake` | WIP:`tests/cloud/tests.cmake` | `a0cd902598542be3b25d07852bf706e594ebaca82e3ec424e30e21342c8c85c7` | Ajout des tests de propriété, refus API, budget partagé ; frontière sans inlining pour injection | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |
| `tests/cloud/width_probe.cpp` | WIP:`tests/cloud/width_probe.cpp` | `cd6509cdd9e5e34af2024584cd6ad6313e7a88be91f30a63c64d50f3f2ff426a` | Ajout des tests de propriété, refus API, budget partagé ; frontière sans inlining pour injection | `mhgp11_cloud_*`, 16 mutants cloud ; G4 `a97180667`, reprise3 |

## Noyau numérique exact

Source : R2 `865f5e64ddd08bedf6ab8f94e8bb94812e380e79` ; empreintes et adaptations
dans [`source_pins.json`](../tests/num/source_pins.json). Les types `Int<bits>`,
les budgets et les fabriques validées sont nouveaux. Aucun filtre flottant
n’est porté dans cette tranche. La nouvelle raison `arithmetic_invariant`
sépare une borne interne violée d’une entrée publique hors domaine.

| Fichier v11 | Source R2 | SHA256 source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/num/wide.hpp` | `src/arith/wide.hpp` | `9cd1a34563501fa1c26d9ec79d510f755f49e6fc0a452e432dfc271a209f4800` | constexpr/noexcept, conversions explicites ; sorties transactionnelles, sans allocation | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |
| `src/num/geometry.hpp` | `src/arith/geometry.hpp` | `e5954da39ab6c216bfb05535158cf862eca9a4ba78a81f7dd0c33b6be1fed673` | Point/Sphere validés et privés ; domaine fermé ; dépendance affine rend optional vide | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |
| `src/num/geometry_internal.hpp` | `src/arith/geometry.hpp` | `e5954da39ab6c216bfb05535158cf862eca9a4ba78a81f7dd0c33b6be1fed673` | Différences, dot et cross bornés par le profil ; gardes des produits larges | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |
| `src/num/sphere.cpp` | `src/arith/geometry.cpp` | `0e98cf5050c64e880386736de9aa47779fae377db0c9fe7acc64dfcf6c039de2` | Centres q2/q3/q4, niveau q3 réduit, dénominateur positif ; types calculés | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |
| `src/num/predicates.cpp` | `src/arith/geometry.cpp` | `0e98cf5050c64e880386736de9aa47779fae377db0c9fe7acc64dfcf6c039de2` | Puissance, orientation, convexité et milieu exacts, bornes par expression | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |
| `src/num/level.hpp` | `src/arith/geometry.cpp` | `0e98cf5050c64e880386736de9aa47779fae377db0c9fe7acc64dfcf6c039de2` | Fabrique validée ; produit croisé complet sans approximation | `mhgp11_num_*`, Fraction aux trois profils et 9 mutants ; G4 `a97180667`, reprise3 |

## Catalogue séquentiel

Le port de `generator.cpp`, `support.hpp` et `catalogue.hpp`
est épinglé par fichier dans
[`src/catalogue/source_pins.json`](../src/catalogue/source_pins.json).
Les primitives géométriques viennent du module num épinglé ci-dessus.
Les adaptations et preuves sont dans [CATALOGUE.md](CATALOGUE.md) : T=0,
réservoir de témoins borné, DFS local, deux passes et tableaux budgétés,
sortie immuable, aucun index ni filtre flottant. Le juge Gram/Fraction est
nouveau et ne dépend pas des formules R2. La [capture du catalogue](../receipts/catalogue_20261002/README.md)
qualifie sa source initiale ; les ports ultérieurs gardent leurs propres captures,
listées dans [DEVELOPPEMENT.md](DEVELOPPEMENT.md). Aucun compte ou temps v10 n'est hérité.

## Index global et bornes de puissance

`src/index/` est une implémentation neuve : arbre équilibré de plages Morton,
boîtes réunies de bas en haut, liens de sortie et census en deux passes.
Elle consomme les modules Cloud et num déjà épinglés, sans copie implicite
des anciens SiteTree ou des index float32 v8. Le contrat, les coûts et
limites sont dans [INDEX.md](INDEX.md). Les nouvelles bornes séparables de
`num::power_bounds` réemploient les budgets prouvés de la puissance ; leurs
changements sont épinglés dans `tests/num/source_pins.json`. Les nouveaux
oracles Gram/Fraction restent distincts du parcours d'index et des formules
de centre du produit. Qualification G4 propre à cette tranche à `e8520481d` :
[capture close](../receipts/index_20261002/README.md), sans transfert à FULL.

## Pool, frontière possédée, cellules et localisation

Le Pool reprend explicitement les [sources R2 par empreinte](../src/sched/source_pins.json) :
callback emprunté, CAS saturant et nettoyage des constructions partielles,
avec garde membre, sans TLS ni allocation par appel. La frontière possédée,
les offsets par ordinal, les deux passes et leur admission mémoire sont neufs.
Voir [CATALOGUE_PARALLELE.md](CATALOGUE_PARALLELE.md).

Les [pins tower](../src/tower/source_pins.json) couvrent cellules, localisation
et canonicalisation. Le principe T2 est repris ; toutes les traces strictes
sont gardées, sans copier le quotient local quadratique. Le juge indépendant
utilise la faisabilité barycentrique. Voir [CELLS_AND_LOCATE.md](CELLS_AND_LOCATE.md).
Ces ajouts passent leurs portes G4 à `c1046dfc7` ; les captures antérieures
restent épinglées. Les forêts/verticales ajoutées ensuite ont leur propre qualification.

## Cache J2 et tri indirect

Deux options neuves sont décrites avec leurs sources d'inspiration exactes
dans [CATALOGUE_OPTIMISATIONS.md](CATALOGUE_OPTIMISATIONS.md). Le cache ne
reprend pas les masques G3 de R2 ; le tri n'importe ni PSRS, ni bandes
flottantes, ni vecteurs non budgétés. Portes et ablations natives propres
restent requises avant toute conclusion de performance.

## Voie rapide FULL du 3 octobre 2026

Inspirations critiques lues dans la v10 publiée (`morsehgp3D_v10/` au commit `895680ff`, dernière
modification `c2c3e0323`) : `src/catalogue/generator.cpp` (sha256
`d5996feaf0df9e9ff274eeb6831b08b54b1a1ca4fd28c7fb81aeff9595dd8662`) pour le filtre de nœud à témoins
prétraités, l'ordre G3 avant la droite des centres, les paires vivantes `live2` et la population SWAR ;
`src/tower/tower.cpp` (sha256 `d919bee049838ca9744218565afeb62b69e04c1a1437f42869a9f7d1fb2e39ae`)
pour le semis H_K consulté à chaque pas et la résolution des jonctions de tous les ordres en une
distribution. Aucun fichier n'est porté : table de populations, lemme, voie concurrente, lignes
vivantes sans table de triplets, compte logique des préfixes, signes natifs des bornes et unions de
racines courantes sont écrits à neuf, avec leurs portes et mutants propres
([PERFORMANCE_FULL.md](PERFORMANCE_FULL.md)). Ni les masques de triplets, ni le mémo partagé par
cellule, ni les filtres flottants des MEB de la v10 ne sont repris. La révision des bornes est
ajoutée à `tests/num/source_pins.json`.

Les corrections suivantes contrôlent mémo/workspace avant tout hit de population et majorent
les census possédés selon les IDs physiques des workers. Le tri F3/F4 reçoit des portes sous
arrondis mixtes, FTZ/DAZ et pannes. Le protocole valide l'inventaire réel G4 avant la reconstruction
au même pin. La [baseline895 et son manifeste](../receipts/qualification_performance_20261003/README.md)
servent à comparer deux sources v11 ; aucune qualification historique de cette baseline n'est héritée.
