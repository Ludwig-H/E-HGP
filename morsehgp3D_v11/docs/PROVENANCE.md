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

« R2: » désigne `morsehgp3D_v10/` dans le dépôt local du raccord R2 au commit `865f5e6`. Les modules `sched`, `io` et le CLI s'ajoutent à leur livraison (`io` : section « Module io »). Aucun compte de portes de la v10 n'est hérité : seules les
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
au même pin. La [baseline 895680ff8 et son manifeste](../receipts/qualification_performance_20261003/README.md)
servent à comparer deux sources v11 ; aucune qualification historique de cette baseline n'est héritée.

## Mécanismes repris le 4 octobre 2026 (vers 100 ms)

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/num/geometry.hpp`, `src/num/sphere.cpp`, `src/num/predicates.cpp` (`Q3Candidate`), `src/catalogue/leaf.cpp` (`q3_of`), `src/tower/meb.cpp` (`consider_q3`) | modèle de l'auditeur `receipts/audit_heritage_20261004/q3_deferred/README.md` (sur la v7 `src/forest/anchor_meb.hpp` l. 67–89 et 125–149, et le générateur v10 777 l. 262–275) | `0241387044bc2f01f8849b229718dc136f816838e044f24aef50381c9fe22eb6` | candidat privé à tag 3 et certificats de `Sphere::through3` ; Level brut de degré six inchangé, calculé à la matérialisation ; le PGCD de la v7 n'est pas porté ; aucun retag q4 ; aucun compteur nouveau | `mhgp11_num_unit_q3_candidate` (u18, u21, u24), `mhgp11_tower_meb_deferred_q3_witness` ; mutants `presentation_q3_taguee_q4_u21` (déplacé dans `materialize`), `candidat_q3_tague_q4_u21`, `candidat_q3_niveau_brut_change`, `q3_candidat_obtus_admis` |
| `src/catalogue/leaf.cpp` (`prepare`, `census_and_emit`), `src/catalogue/internal.hpp`, `src/catalogue/catalogue.cpp` (`dominated`) | feuille J3 de la v10, `morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/src/morsehgp3D_v10/src/catalogue/leaf.hpp` (phase D l. 102–134, recensement, lemme R, l. 136–170) | `7b360ccac44f79ee3a3532881620cb7534257e8f886ccb282873c180fbf033e8` | seul le lemme R est porté : transposée de dominance remplie dans la boucle existante, masques par mot (feuilles larges comprises), parcours et arrêt du census complet conservés pour garder le ledger ; refus si un site est à la fois intérieur et extérieur ; ni table H, ni étages, ni voie i64, ni poids | `mhgp11_catalogue_fraction` et dumps FULL identiques ; mutants `lemme_r_exterieur_par_dominants`, `lemme_r_interieur_en_coquille`, `lemme_r_transposee_inversee` |
| `src/catalogue/leaf.cpp` (`doubled_envelope_meets`, `q3_of`, `q4_of`) | même feuille J3 (`leaf.hpp`, lemme M3 l. 238–249 et 301–309, lemme E4 l. 330–374) | `7b360ccac44f79ee3a3532881620cb7534257e8f886ccb282873c180fbf033e8` | repère T0 de la v11 (aucun décalage T) ; milieux et sommets doublés en i64 ; la non-dégénérescence q4 reste comptée avant E4 pour garder `q4_candidates` ; ni table H, ni droite sans branchement, ni voie i64 | `mhgp11_catalogue_region_median_envelope`, dumps FULL identiques ; mutants `enveloppe_bord_bas_ferme`, `enveloppe_tetra_trois_sommets`, `enveloppe_q4_avant_comptage` |
| `bench/catalogue_euler.hpp`, `bench/catalogue_euler.cpp`, `tests/catalogue/euler_oracle.py`, `tests/catalogue/euler_limits.py` (juge d'Euler à K+2 et restriction J1) | v9 `origin/main` : `morsehgp3D_v9/src/chain/tower_chain.cpp` (`euler_add` l. 866, coquilles étendues l. 977–990, terme n et refus l. 2024–2039) ; notes de l'auditeur C `audits/NOTE_C_INVARIANT_EULER_20260923.md` et `audits/c_euler_20260923/euler_check.cpp` ; preuve par le nerf `audits/CONTRELEC_EULER_PAR_NERF_20260923.md` ; contre-exemple `audits/CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md` | `tower_chain.cpp 7d1eeac6d08ebd1f92142dfae28447824c570f39ed4d18817f736da4117e5297 ; NOTE_C 8cfc6287c7f2d97d664ec01045740627504a9fee89e38548c94bfced3f07171e ; euler_check.cpp bf89867ec424f03b29ddbf9c2df5ad02c88f60a7b37b8f723e2b0cf388a239b7 ; CONTRELEC f6cfcd2cf53e7ec24b8c7434b35b40b9aedae577cc158da3e177bb5af3bf6785 ; CONTRE_EXEMPLE fc9cf50b80ff28db35493ec6c9c08504eb7ae18eba3777e5d7cee80c248f8f92` | Hors produit (la v9 calculait dans la chaîne, au recensement) : le juge lit un `Catalogue` construit. Coquilles étendues : supports minimaux refaits par les prédicats de `num`, fermeture par transformée de zêta, borne 24 au lieu de 12, refus explicite au-delà. Restriction J1 clé par clé avec rangs recalculés et niveau brut de la table (la v9 comparait un compte et une somme de hachés). $\mathrm{Cat}_{12}$ admis, donc K = 10 jugé (la v9 s'arrêtait à K ≤ 10 pour le catalogue, donc K ≤ 8 jugé). Contrôles locaux ajoutés : niveau exact, signes de puissance de I et U, minimalité de S*, qmin et S* des coquilles étendues. Harnais d'omissions (`--omit`, `--omit-k`, `--omit-k2`) pour graver les limites. Générateur uniforme : port de `random.Random(graine).getrandbits(18)` de CPython, empreinte égale aux fichiers épinglés. Nombres de boules gravés sur les trames : catalogue v10 de l'audit L01 (`catalogue_lidar_comptes.txt`, sha256 79ff6a6570d5185688cb575d6d182bbcbddc3b152af20e87103fb651bd65517b, hors dépôt). | `mhgp11_catalogue_euler_oracle`, `mhgp11_catalogue_euler_limits` (et leurs jumelles `_opt`), `mhgp11_catalogue_euler_scale8000`, `_scale16000`, `_scale32000`, `mhgp11_catalogue_euler_lidar_ng00_k5` à `_ng02_k10` ; neuf mutants `euler_*` de `tests/mutants/catalogue.json` |

## En-tête public de la tour (tranche S2, 4 octobre 2026)

`src/tower/meb.hpp` reprend **à l'octet** `src/tower/tower.hpp` tel qu'il était au commit `1bf4be68f` (sha256
`fd307ad49e3f0aa807d080e2467a577697db01501732bdfd7b6502dec9760380`) ; `src/tower/tower.hpp` devient le parapluie public
(commit `257aabb92`). Les épingles historiques qui désignent la MEB par le chemin `src/tower/tower.hpp`
(`src/tower/source_pins.json`, `receipts/qualification_performance_20261003/baseline_source_manifest.json`) visent ce
contenu, désormais dans `meb.hpp`. Portes : `mhgp11_tower_public_header_umbrella` et
`mhgp11_tower_public_header_inventaire`.

## Module io : lecture, empreintes et transaction de dossier (tranche S4, 4 octobre 2026)

Port explicite annoncé par la spécification de la sortie paramétrée (§ 3.4). Sources : raccord R2 de la v10,
dépôt local `build/v10-integration-r2/src`, commit `865f5e64ddd08bedf6ab8f94e8bb94812e380e79` ; bibliothèque
`morsehgp3d/` de `main` au commit `1bf4be68f` (fichier inchangé depuis `375a46590`) ; lecteur des bancs de la v11.
Aucune porte ni aucun compte de mutants de la v10 n'est hérité.

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/io/input.cpp` (`read_u32le`, `check_u32le_sizes`) | R2:`src/cloud/u32le_input.hpp` ; second fichier d'identifiants repris de `bench/whole_input.hpp` (v11, `80e77544e`, sha256 `95ee29ceda8d47d18c92fda9b83409e3da80ddd5dfc8ae0c2137ad532cd963c2`) | `0ebd33a164ab0eeb5a7ba4b9c1b059b7338c95b93798d586006a875b376ffda3` | Gardé : lecture entière ou refus, nature et taille par `fstat` sans allocation avant tout tampon, `index_overflow_u32` sur la taille annoncée, lecture incomplète ou fichier qui change de taille refusés. Changé : deux fichiers (`points.u32le`, `ids.u32le`) au lieu du rang d'entrée pour `PointId` ; tube et périphérique refusés (`input_unreadable`, ouverture `O_NONBLOCK`) au lieu d'une lecture en flux, car un tableau sans taille annoncée ne peut pas être admis avant l'allocation ; tailles incohérentes `input_unreadable` (comme `whole_input.hpp`) au lieu de `size_mismatch` ; fichiers vides admis, le nuage vide restant refusé par `prepare_cloud` (`empty_input`) ; quatre `Buffer` admis (16 octets par point) au lieu de `std::vector` ; décodage petit-boutiste par décalages ; empreintes SHA-256 des deux fichiers au fil de la lecture. | `mhgp11_io_unit_input`, `_input_sizes`, `_input_unreadable`, `_input_budget`, `mhgp11_io_fault_starvation` ; mutants `lecture_tronquee_acceptee`, `fin_non_controlee`, `type_non_regulier_admis`, `controle_apres_allocation`, `admission_omise`, `lecture_gros_boutiste` |
| `src/io/writer.cpp` (`FileWriter`) | R2:`src/core/cli_output.hpp`, étape 3 (`OutputSet::write`, `FileCloser`) | `ff7d96a52f5c60ceb77e78d644c80b4338e0ffb37c2cc3444eef092bb2f972c7` | Gardé : propriétaire du `FILE*` dès l'ouverture, `fflush`, `ferror` et `fclose` contrôlés, création exclusive `O_EXCL` et `O_NOFOLLOW`. Changé : fichier créé sous son nom final dans `D.pending/` (ni temporaire par fichier, ni copie de droits) ; `fsync` avant `fclose` ; écritures successives (`bytes`, `u32s`, `u64s`, `pad8`) en petit-boutiste au lieu d'un rappel, taille et empreinte tenues au fil de l'écriture, erreur définitive ; tampon `setvbuf` fixe de 64 Kio ; ni sortie spéciale ni `std::bad_alloc` (aucun `operator new`). | `mhgp11_io_transaction_commit`, `_write_failure`, `_discard` ; mutants `ecriture_non_controlee`, `empreinte_ecriture_omise`, `bourrage_toujours`, `mots_gros_boutistes` |
| `src/io/directory.cpp` (`OutputDirectory`) | R2:`src/core/cli_output.hpp`, étapes 1, 4 et 6 (`declare`, `resolve`, `commit`, destructeur) | `ff7d96a52f5c60ceb77e78d644c80b4338e0ffb37c2cc3444eef092bb2f972c7` | Gardé : déclaration préalable sans rien créer, sortie jamais sur une entrée (chemin résolu), dossier absent ou non inscriptible `output_unwritable`, tout ou rien, destructeur sans publication qui ne retire que ce que l'appel a créé. Changé : la transaction porte sur un dossier `D` qui ne doit pas exister (`D` ou `D.pending` présent, lien pendant compris : `output_conflict`) ; un seul renommage `renameat2(RENAME_NOREPLACE)`, refus `output_unwritable` s'il est indisponible, jamais `rename` ; ni sauvegarde `.bak` ni restauration (rien n'est jamais remplacé) ; le retour arrière de R2 (`rollback` après
publication, quand la sortie standard échoue) devient `retract()` : `D` est renommé en `D.pending` sans remplacement
puis retiré, et l'objet ne publie plus rien ; `D.pending` orphelin refusé et jamais retiré ; manifeste `manifeste.json` réservé, écrit en dernier après la fermeture contrôlée de tous les fichiers ; `fsync` de `D.pending` et du parent ; chemins dans des tableaux fixes, `realpath` vers un tampon, descripteurs `*at` sur le parent ; l'identité par inode de la source n'a plus d'objet (aucune destination n'existe) et le contrôle des entrées par chemin résolu recouvre celui de l'existence de `D`. | `mhgp11_io_transaction_plan_syntax`, `_plan_parent`, `_plan_conflicts`, `_plan_inputs`, `_create_names`, `_parent_unwritable`, `_commit`, `_noreplace`, `_orphan`, `_discard`, `_retract`, `_write_failure` ; mutants `conflit_ignore`, `orphelin_ignore`, `renommage_ecrasant`, `pending_non_retire`, `publication_defaite_au_destructeur`, `manifeste_avant_donnees`, `nom_reserve_admis`, `nom_double_admis`, `retrait_sans_renommage` |
| `src/io/sha256.cpp` (`Sha256`, `to_hex`) | `morsehgp3d/src/cpu/contract/canonical_id.cpp`, lignes 18-45 (constantes) et 49-221 (`CanonicalSha256Builder`) | `07f23c844b38534f5cf7bd69d7b5bc4a74aa878707b0d5ad61874b012785567a` | Gardé : constantes, compression à mots de travail tournés par leur nom, absorption en trois phases, remplissage final. Changé : aucune exception (la source levait sur une mise à jour après finalisation et sur un débordement du compte de bits) ; `finish()` calcule sur une copie et peut être rappelé ; longueur bornée par `kMaxMessageBytes` (2^61 − 1 octets), gardée par `FileWriter` ; écriture hexadécimale sans allocation. | `mhgp11_io_unit_sha256` (vecteurs de FIPS 180-4, découpages), `mhgp11_io_unit_hex`, `mhgp11_io_sha256` (différentiel contre `hashlib`, 1 242 empreintes) ; mutants `sha_tronque`, `sha_longueur_en_octets` |

## Sortie paramétrée : ports annoncés (contrat S0, 4 octobre 2026)

**Rien n'est porté en L0** : la tranche S0 n'écrit que des documents ([contrat des sorties](SORTIES.md)). Les ports
ci-dessous sont **annoncés** par la spécification de la sortie paramétrée. Chacun sera épinglé de nouveau à sa tranche
(sha256 de la source au commit du port), avec ses adaptations et les portes v11 qui le requalifient.

Sources : `morsehgp3D_v11/` au commit `f98aeed67`, où ces six fichiers sont inchangés depuis `57dd21be1`. Ce sont des
bancs et des sondes de la v11 elle-même, et non la v10.

| Futur fichier v11 | Source | sha256 de la source à `f98aeed67` | Tranche | Ce qui sera repris, et sa requalification |
| --- | --- | --- | --- | --- |
| `tests/tower/attach_judge.cpp` : juge E2, **de test seulement**, jamais dans le produit | `bench/points_export.cpp`, `ball_nodes` (avec `descend`) | `f77ca2c22d0abc359cc202257d44e89ec32a782ebf8dc84f4b1de79e8f730a4d` | S3 (L1) | Descente d'une $K$-partie de $P_b$, puis ancêtre à la coupe fermée de la boule. La fenêtre des événements $p+q_{\min}-1\leq K\leq p+m$ remplace le prédicat `strong` ($p+q_{\min}\leq K$), qui écarte les événements faibles. Pour une $K$-partie quelconque, le juge admet un niveau initial $\beta(F)\leq\lambda_b$ (la paire extrême de la ligne $0,1,2$ à $K=2$ a le niveau de sa boule) et remonte à l'ancêtre **fermé** ; le journal des traces strictes garde $\beta(F)<\lambda_b$ ; les feuilles de sites à $K=1$ sont un cas séparé (réponse D.4 de l'auditeur, `aef7182b3`). Le témoin D2 ($41<64<1681/25$, MATHEMATIQUES § 10.11) est l'une de ses fixtures, avant toute optimisation ; aucune garde native n'exige $\beta(F)\leq\ell(r_b-1)$. Porte `mhgp11_tower_attach_e1e2` (E1 = E2) |
| `src/supports/enumerate.cpp` | `bench/catalogue_euler.hpp`, `mark_supports` et `closure_counts` | `f293df6ea1a2cc8d5fef6e78453de66343728a61d3013a5d1613cfdbb374dd18` | S6 (L1) | Supports positifs minimaux sur toute la coquille, par les prédicats stricts de `num` ; plafond 24 et refus `support_shell_capacity` ; sphère reconstruite depuis $S^*$, niveau contrôlé. $\mathcal{Q}_b$ est extrait des marques **avant** la fermeture zêta, qui sert ensuite aux comptes $N_j$ : après elle, les 6 supports du cube deviennent 177 parties. Aucun filtre par $\lvert Q\rvert\leq K+1$ ni par `cofaces` positif : les deux tétraèdres du cube restent dans $\mathcal{Q}_b$ à $K=1$, avec 0 coface (gardes de l'auditeur, `de4ab58a8`). Portes `mhgp11_supports_fraction` (contre l'oracle S1) et juge d'échantillon ; mutants `supports` |
| `src/api/write_full.cpp` | `bench/full_probe.cpp`, `serialize` et `birth_sphere` | `2d3a37ccf93a0b917800550a3e285ceb8b28dda0365494c31ebd7c39dba8629c` | S5 (L2) | Octets de `MHGP11FUL1` inchangés ; écriture transactionnelle par `io`. Porte `mhgp11_cli_full_identity` (sha256 brut égal au dump de `mhgp11_full_bench`) |
| `src/num/` : repli exact des sommes de radicaux | `bench/points_radius.py` (`sqrt_bounds`, `radical_classes`, `sign_of_radicals`) et `bench/points_flat.py` (`group_classes`, `RadSum.sign`, `Level.phi_exact`) | `457b997fa7eecea92dd0f27cd29f514655dbd0ff716ce050825fc65aaf5783d1` et `4647de0762b500fb36ca917941a34120d58931f0b0033b570490821860fdd3bc` | S8 (L3) | Mêmes décisions qu'en Python, mais par des encadrements entiers jusqu'à 8 192 bits, puis refus `radical_sign_budget`. Différentiel contre l'`int` de Python |
| `src/points/` | `bench/points_hierarchy.py` (`qualify` à `first_points`, `Hanging`), `bench/points_radius.py` (`floor_rank_radius`, `ancestor_at_radius`, `hang_margin_radius`), `bench/points_flat.py` (`PointTree`, `tower_point_tree`) | `fa12f16f3ce599a5299737d740bd252454cddcb9c475edf593d7aab3cd426c17`, `457b997f…` et `4647de07…` (ci-dessus) | S9 (L3) | Décisions exactes en entiers, $m(1)=1$ et $\kappa=1$. Les incidences fortes sont tirées du rattachement, au lieu des descentes de `ball_nodes`. Porte `mhgp11_points_vs_python` (identité site par site) |
| `src/head/` | `bench/points_flat.py` (`condense`, `select`, `labels`) | `4647de07…` (ci-dessus) | S10 (L4, reportable) | Étiquettes dans l'ordre d'entrée, sans `out[pt.ids]`, qui suppose des `PointId` denses. Porte `mhgp11_head_vs_python` |

Ports livrés depuis, chacun épinglé dans sa propre section ci-dessous : `src/supports/enumerate.cpp` (tranche S6a,
intégrée en L1 le 5 octobre 2026).

## Module supports : Q_b et comptes du lemme G (tranche S6a, 4 octobre 2026)

Port explicite annoncé par la spécification de la sortie paramétrée (§ 3.3). Source : le juge d'Euler de la v11,
`bench/catalogue_euler.hpp`, inchangé depuis le commit `462dca187` et lu à `f98aeed67` ; il est lui-même un port de
la v9 (section « Mécanismes repris le 4 octobre 2026 » ci-dessus). Aucune porte ni aucun compte de mutants du juge
d'Euler n'est hérité. Les comptes (`counts.hpp`, `counts.cpp`) sont écrits à neuf d'après le lemme G ; seule l'idée
d'une table de Pascal `constexpr` vient du même fichier (`kBinomial`, l. 34–42).

| Fichier v11 | Source | sha256 de la source | Adaptations | Portes v11 |
| --- | --- | --- | --- | --- |
| `src/supports/enumerate.cpp` (`ball_supports`, `enumerate`, `zeta_or`, `Filler::counted`, `canonical_sphere`) | `bench/catalogue_euler.hpp` : `mark_supports` (l. 135–165), `closure_counts` (l. 108–128), sphère refaite depuis S* et contrôle du niveau dans `judge_ball` (l. 230–241), premier support égal à S* (l. 273–276) | `f293df6ea1a2cc8d5fef6e78453de66343728a61d3013a5d1613cfdbb374dd18` | Gardé : les trois boucles de positions croissantes et leurs prédicats (`is_midpoint` ; `strictly_acute` puis orientation nulle du centre ; `strictly_inside`), l'ordre (arité, positions) qui place S* en tête, les masques de positions `u32`, la transformée de zêta en OU par mots et le décompte par poids, `Sphere::through` de l'arité qmin et l'égalité exacte au niveau du catalogue. Changé : chaque support est **écrit** au fil de l'énumération, avant la fermeture qui réécrit les mêmes mots (audit `de4ab58a8` : les 6 supports du cube deviennent 177 parties) ; la boule est contrôlée par la fonction et non par l'appelant (`BallIdx`, plafond par `check_shell`, tailles de `out` et du brouillon avant toute écriture) ; un écart est un refus `supports_invariant`, jamais un compte de fautes ; N_j en `u32` ; la coquille régulière (m = qmin) rend {S*} et N_j = [j = qmin] sans sphère ni prédicat ; plafond 24 inchangé, refus `support_shell_capacity` au lieu du refus du juge ; ni sommes d'Euler, ni recensement I/U (le catalogue en fait foi, G2), ni comptes de fautes ; registre `SupportLedger` (tests de prédicats) sur succès seulement, non protégé contre la concurrence : un registre par fil, sommé par `SupportLedger::add` après la jointure (contre-lecture S6a, constat F1). | `mhgp11_supports_unit_*` (17 groupes : fixtures 1, 2, 3, 9, 10, 11, 12 du § 2.9 de la spécification, coquille mixte de l'audit `de4ab58a8`, bornes 24/25, refus ; puis, à l'intégration L1, les témoins des auditeurs `sphere5`, `sphere5_k12`, `square_k10`, `sphere9_refus` et `impossible_arity`), `mhgp11_supports_shell_capacity` (fixture 13), `mhgp11_supports_fraction` (différentiel contre l'oracle S1, intégration L1), `mhgp11_supports_judge_small`, `mhgp11_supports_sample_judge_*`, `mhgp11_supports_registers_*` ; mutants `triangle_droit_accepte`, `drapeau_q4_presentation`, `arret_premier_support`, `fermeture_omise`, `coquille_sans_plafond` (`tests/mutants/supports.json`) |
| `src/supports/counts.hpp`, `src/supports/counts.cpp` | écrits à neuf (lemme G) ; table de Pascal inspirée de `kBinomial` (même fichier, l. 34–42) | `f293df6ea1a2cc8d5fef6e78453de66343728a61d3013a5d1613cfdbb374dd18` | Table `u64` de C(a, b) pour a ≤ 35, `static_assert` : tout binôme lu (bas ≤ 13, ou haut ≤ 24) tient en `u32` ; `kparties_reliees` (nom de la décision utilisateur du 4 octobre, au lieu de `k_parts`), `compressed_parts`, `strict_traces`, `cofaces`, `gabriel_cofaces` par boule, `support_cofaces` et `support_gabriel_cofaces` par support ; aucun compte stocké ; `Shape` ne se construit que contrôlée, `Closure` est vide (constructeur par défaut, refusée par `ball_counts`) ou construite par `ball_supports` ; `support_cofaces` et `support_gabriel_cofaces` rendent 0 pour une arité hors de 2..4 **ou supérieure à m** (audit général `a65903a7b`, P2, corrigé à l'intégration L1). | mêmes portes, et `mhgp11_supports_unit_impossible_arity` ; mutants `cofaces_ordre_k`, `fermeture_sans_qmin`, `fermeture_sous_qmin`, `arite_impossible_admise` |

**Gardes sans porte possible** (contre-lecture S6a, constat F3). Les contrôles ci-dessous protègent des invariants du
catalogue (G2) ou les budgets de `num`. Ils ne se déclenchent que sur un domaine corrompu : `FullDomain` ne se
construit que par `prepare_full_domain`, et la règle 6 d'`ARCHITECTURE.md` interdit tout crochet dans le produit.
Aucune porte ne peut donc les atteindre. Un mutant qui en retire un est équivalent sur tout domaine préparé et survit
par construction : la contre-lecture l'a constaté pour `niveau_non_controle` et `premier_support_non_controle`, qui
survivent à toutes les portes et à son contre-juge. Ces mutants n'entrent pas au manifeste
`tests/mutants/supports.json`. Ce que ces gardes protègent est jugé en amont par le juge d'Euler du catalogue :
`well_formed` et les fautes `degenerate`, `level` et `canonical` de `bench/catalogue_euler.hpp`, portes
`mhgp11_catalogue_euler_*`. Chaque garde est marquée « sans porte » dans le code.

| Fichier, fonction | Gardes sans porte possible |
| --- | --- |
| `enumerate.cpp`, `ball_supports` | `qmin` hors de 2..4 ; `m < qmin` |
| `enumerate.cpp`, `site_point` | `SiteIdx` hors du nuage ; refus de `Point::make` |
| `enumerate.cpp`, `canonical_sphere` | refus de `Sphere::through` ; S* affinement dépendant ; rang hors de la table des niveaux ; niveau de la sphère de S* différent de celui du catalogue |
| `enumerate.cpp`, `enumerate` | refus de `orientation` et de `strictly_inside` |
| `enumerate.cpp`, `extended_supports` | brouillon nul (`closure_words(m) = 0`, exclu par `check_shell` : défense en profondeur du plafond, qui rend le mutant `coquille_sans_plafond` causal) ; coquille de taille différente de m ; premier support absent ou différent de S* |
| `counts.cpp`, `ball_counts` | $N_m\neq 1$ ; $N_j>\binom{m}{j}$ ; `cofaces` au-delà de $2^{32}-1$ (Vandermonde) |

Tous les autres refus du module ont leur porte. `mhgp11_supports_unit_refusals` juge : `BallIdx` hors du catalogue
(`parameter_out_of_range`, par `ball_supports` et `ball_shape`) ; `out` et brouillon trop courts ; le domaine de
`make_shape`, clause par clause ; une boule hors de $\mathrm{Cat}_K$ par `ball_shape` ; une fermeture étrangère à la
forme (taille, $N_{q_{\min}}=0$, $N_j\neq 0$ sous $q_{\min}$). Le plafond (`check_shell`) est jugé par
`mhgp11_supports_unit_shell_bound`, `mhgp11_supports_shell_capacity`, `mhgp11_supports_unit_refusals` et
`mhgp11_supports_unit_sphere9_refus`.

**Intégration L1 (5 octobre 2026) : apports des auditeurs.** Commit d'intégration de S6a sur `238734f1d`. Les attendus
viennent des reçus des auditeurs (`receipts/audit_supports_contract_20261005/qb`,
`receipts/audit_native_integration_20261005/qb` et `receipts/audit_geant_20261005/native`), jamais du produit. Le
produit ne change que par la correction P2 ci-dessous.
- **P2 de l'audit général `a65903a7b`** (`src/supports/counts.hpp`) : `support_cofaces` admettait une arité
  supérieure à $m$ (`support_cofaces(make_shape(2, 2, 2, 3), 3)` rendait $\binom{1}{1}=1$ ; 51 des 300 couples (forme,
  arité $>m$) du domaine de `Shape` étaient non nuls). La garde exige désormais $a\leq m$, dans les deux fonctions ;
  pour `support_gabriel_cofaces`, la clause est une défense, $\binom{m-a}{t+1-a}$ étant déjà nul, et son retrait est un
  mutant équivalent. Porte `mhgp11_supports_unit_impossible_arity` (formes $(1,2,2,2)$, $(2,2,2,3)$, $(1,3,3,3)$,
  arités 3 et 4, puis balayage des 4 401 formes du domaine) ; mutant `arite_impossible_admise` (plancher du
  manifeste 8 → 9). Aucun support émis par `ball_supports` n'est concerné.
- `tests/supports/witness_test.cpp` (portes `mhgp11_supports_unit_sphere5`, `_sphere5_k12`, `_square_k10`,
  `_sphere9_refus`) : la sphère $x^2+y^2+z^2=5$ translatée de $(2,2,2)$, 24 sites admis, 12, 24 et 792 supports,
  $N_2=12$, $N_3=288$, $N_4=3906$ et les comptes de $K=1$ à $K=3$ ; à $K=12$, la fermeture de la boule centrale de
  $\mathrm{Cat}_1$ par `ball_supports`, puis `make_shape(0, 24, 2, 12)` et `ball_counts` : 2 704 156, 116, 2 496 144
  et 149 954 688 incidences ; le petit témoin à $K=10$ (66, 6, 4, 12, 4 ; 20 incidences) ; le refus de la primitive à
  25 sites (25 des 30 sites de $x^2+y^2+z^2=9$, les six points axiaux gardés). Le refus de l'appel `supports` entier
  viendra avec l'assemblage (S6b).
- `tests/supports/fraction_diff.py` (porte `mhgp11_supports_fraction`) : la sonde native contre l'oracle S1
  (`reference/hgp11_ref/supports.py`), sur les 210 nuages et les 951 ordres de sa suite ; mêmes boules de $W_K$,
  mêmes $(p,m,q_{\min})$, mêmes comptes, mêmes comptes par support réordonnés ; 15 062 boules. Reprend le
  différentiel de lecture de la contre-lecture S6a (`verif_s6/l0diff.py`, hors dépôt), sur les nuages de l'oracle.
- `reference/hgp11_ref/supports.py` et `reference/test_supports.py` : budget explicite de la force brute du lemme F
  (`BudgetRefusal`, $2^{m}-1$ candidats au plus) et primitives sur sphere5 (porte
  `mhgp11_reference_supports_primitives`) ; voir `reference/README.md`.
