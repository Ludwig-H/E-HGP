# Lentille 6 — Portes, oracles, mutants et CI

Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. **GCP non utilisé.** Rien n'a été compilé ni exécuté sur l'hôte en dehors d'un script Python de 20 lignes en arithmétique entière (`/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/agents/lentille6/diam_depth.py`). Les résultats d'exécution cités viennent des journaux GitHub Actions (lecture seule, `gh run view`). Lecture dans `build/v9-audit-c-worktree` à `0125dc18`. Les commits suivants jusqu'à `origin/main` `7312beb7` ne touchent que `probe_worker_contract.py` (31 → 33 mutants de schéma), le ledger de la chaîne et `tower_selftest_v9.py`. Tous les constats ci-dessous restent valides à ce HEAD.

Statuts employés : **prouvé** (théorème ou argument écrit ici), **testé** (porte CTest, avec taille), **mesuré** (reçu ou journal CI), **déduit** (lecture de code ou script), **supposé**.

## 1. Inventaire : ce que CTest enregistre

La CI de `0125dc18` (exécution 35833313204) liste **128 tests**. Résultat : `100% tests passed, 0 tests failed out of 127`, le 128e étant `…admitted_lane_recounted_in_children` marqué `Not Run (Disabled)`. Durée totale : 40,95 s (**mesuré**, journal CI). Répartition par préfixe : 86 `gen`, 26 `tower`, 14 `chain`, 2 `probe`. 45 noms contiennent `mutant`.

Tous les tests portent le label `gate` (`CMakeLists.txt:55-63`). Le label `oracle` est posé par famille, pas selon le contenu :

- les six portes tour, y compris `full_certificate`, qui « neither calls geometry » (`tests/tower/full_certificate_gate.cpp:1-3`), reçoivent toutes `gate;tower;oracle` (`CMakeLists.txt:71-77`) ;
- à l'inverse, onze portes q2 contiennent un oracle brute force écrit dans la porte (`wspd_q2_parallel_gate.cpp:89-93`, n ≤ 140 ; `q2_terminal_pool_gate.cpp:59-60`, n ≤ 69) mais n'ont que `gate;gen` (`CMakeLists.txt:262-268`).

Les labels `scale8000/16000/32000` n'existent que sur trois diagnostics `mhgp9_gen_q4_saturating_atlas_scale*` (`CMakeLists.txt:276-279`). Ce point est traité au § 6.

Carte par étage :

| Étage | Portes principales | Juge | n | K / s / fils |
| --- | --- | --- | --- | --- |
| Primitives tour (MEB, plateaux, arithmétique u18) | `anchor_meb`, `local_plateau`, `arith_u18`, `anchor_meb_proposed` | Rationnel Boost indépendant (`oracle/tower/local_plateau_oracle.hpp`, n ≤ 8, l. 27-34) ; `cpp_int` par formules distinctes (`arith_u18_gate.cpp:1-30`) ; différentiel contre `anchor_meb` pour la proposition | ≤ 10 sites | — |
| FULL sur catalogue fourni par l'oracle | `full_ball_tower` `--selftest/--static-1/--static-4`, `full_coverage_certificate`, `full_certificate --rejects` | Modèle Γ exhaustif (`full_ball_tower_gate.cpp:233-289`) | 16 fixtures, n ≤ 8 (l. 119-146) | kmax ≤ 6, et 8 sur une fixture statique seule (l. 506-513) ; fils 0/1/4 |
| Générateur q2 | 11 portes `q2_*`, `wspd_q2_*`, `wspd_front*` | Brute force entier indépendant ; compteurs gravés à n = 2 000 (`wspd_front_gate.cpp:795-816`) | ≤ 140 (brute force), 2 000 (compteurs) | K 1..10, fils 1/2/3/4 |
| Générateur q3/q4 | `wspd_q34`, `q4_local`, `q4_seed_cells`, `q3_ball_census`, `q34_*` | Rationnel Boost (`tests/gen/exact_ball_oracle.hpp:16-22`) | ≤ 30 (`shell30`) | K ∈ {1,2,3,5,10}, s ∈ {8,10,12}, fils 1/2/4 (`wspd_q34_gate.cpp:313-316, 596-598, 701`) |
| Chaîne de bout en bout contre Γ | `chain_census_tower_{historical,line12,shell14,spatial12,rejects}` + 4 mutants | Inventaire rationnel exhaustif + modèle Γ (`tests/tower/census_tower_oracle.hpp`, n ≤ 14, l. 17-18) | 12 à 14 | **K = 10 seulement**, s 8/10/12, chaîne W1/W4, tour 0/1/4 |
| Chaîne, différentiel | `chain_static_paths`, `chain_order_failure_priority` (+2 mutants), `probe_worker_contract` | Égalité de condensé ; leviers ON = OFF | 1 500 et 360 | K = 5 ; tour 0/1/4/8, chaîne 1/4/7 |
| Primitives parallèles | `parallel_sort` (+mutant), `population_bank` | `std::sort` ; surcharge copiante | aléatoire | **0 à 48 fils** (`parallel_sort_gate.cpp:46,75`) |

## 2. Indépendance des oracles

**Arithmétique.** Les juges T2 de la tour et du générateur calculent en `boost::rational<cpp_int>` (`local_plateau_oracle.hpp:18-19`, `exact_ball_oracle.hpp:11-22`). Le produit calcule en i128/U192/U320. Les deux arithmétiques sont bien différentes.

**Code.** Le modèle Γ de `census_tower_oracle.hpp:28-98` suit mot pour mot la Définition 21 du manuscrit (sommets = (K−1)-simplexes de Čech, arêtes = union encore simplexe ; extrait texte l. 1893-1905) :

- MEB par support positif puis enclosure ;
- coupes fermées `≤` et ouvertes `<` ;
- adjacence par les (K+1)-cofaces.

Ce modèle ne partage avec le produit que `P3` et `p3_in_profile` (`local_plateau_oracle.hpp:4`).

**Limite.** Aux tailles n = 12 à 14, le modèle Γ n'est recoupé par **aucun second oracle** : `--historical` ne le compare à `local_plateau_oracle` que sur les fixtures n ≤ 8 (`chain_census_tower_gate.cpp:17-43`). L'oracle Python exhaustif `reference/morsehgp3d_oracle` constituerait un troisième juge d'une autre langue et d'une autre époque. PLAN_V9 le range parmi les portes (« les oracles de correction bornés (`reference/`, T2) sont des portes », `docs/PLAN_V9.md:31`), mais aucun test v9 ne l'appelle (**déduit**, grep).

**Catalogue.** Le catalogue « indépendant » de T2 applique la même règle d'admission que le produit (`interior + qmin ≤ min(K+1,n)`, `chain_census_tower_gate.cpp:76` contre `src/chain/tower_chain.cpp:485-486`). Ce n'est pas un défaut : la règle est une définition, et le juge Γ la valide indirectement. Mais la comparaison des catalogues ne peut pas, à elle seule, réfuter cette règle.

## 3. La preuve de bout en bout, pas à pas (T2 chaîne)

`large_case` (`chain_census_tower_gate.cpp:167-224`) procède ainsi :

1. Il construit le modèle Γ et le catalogue rationnel, avec K = 10 codé en dur (l. 174).
2. Pour s ∈ {8,10,12}, il appelle la chaîne réelle avec `run_tower=false` et compare le catalogue clé par clé : clé, niveau, arité, populations (l. 114-131).
3. Il construit FULL à 0/1/4 fils, compare toutes les coupes ouvertes et fermées de K = 1..10 et les images verticales au modèle Γ (l. 133-165), et exige des payloads identiques entre les voies (`same_payload`, `full_ball_tower_gate.cpp:291-319`).
4. Il juge la tour **publiée** par la chaîne (`run_tower=true`, W1/W4, s = 8) contre Γ (l. 195-219).

Trois faiblesses, toutes **déduites** du code :

- **K fixé à 10.** À n = 12, la fenêtre de rang admet presque toutes les boules. Les seuils des certificats d'élagage valent K−1 = 9 pour q3 et K−2 = 8 pour q4 (`src/gen/lanes/q34_dead_lanes.cpp:97-98`) ; le seuil de rejet q2 est K. Condition nécessaire à toute voie morte ou rejetée par témoins : le disque des centres admissibles contient le milieu m (borne 1/12, `wspd_q34_gate.cpp:688`), donc la **boule diamétrale** de l'arête doit contenir au moins le seuil de sites. Le script exact `diam_depth.py` donne :
  - `spatial12`, seule fixture 3D générique : profondeur diamétrale maximale **6** sur 66 paires. **Aucune** voie ne peut être élaguée à K = 10 ;
  - `shell14` : au plus 3 arêtes (q3) et 4 arêtes (q4) remplissent la condition ;
  - `line12` : colinéaire, aucun support q3/q4 positif n'existe.

  À K = 5, `spatial12` donne 8/15 paires candidates ; à K = 3, 26/39. Les leviers qui éliminent le travail à l'échelle (voie morte, cœur, cache, filtres de témoins) ne passent donc presque jamais sous le juge Γ. Ils sont jugés au niveau composant (§ 1) et différentiellement à 360 sites (`probe_worker_contract.py:127`).
- **Le repli K1..5 du contrat n'est jamais jugé de bout en bout contre Γ.** Seul `chain_static_paths` à 1 500 sites en K5 compare des condensés entre voies (`chain_static_paths_gate.cpp:47-69`).
- **Une seule permutation d'entrée réelle pour le générateur.** Les appels de catalogue passent les positions dans l'ordre Morton de l'index de la tour (`chain_census_tower_gate.cpp:94-99` ; tri Morton `src/tower/tree/cloud_index.hpp:158-160`). Les variantes 0 et 1 donnent donc **la même entrée** au générateur. Seul le bloc « tour publiée » varie l'ordre (l. 196-203), et seulement à s = 8. Aucun plancher ne porte sur le ledger du générateur (l. 274-277).

## 4. Mutants

- **Générateur (35 mutants compilés, `tests/gen/mutants.json`).** Chaque copie mutée est liée avant `libmhgp9_gen.a`. Le mutant n'est tué que si le code vaut 1 **et** que stderr est exactement celui attendu, dans la même exécution (`tests/gen/run_mutant.cmake:52-63`). C'est la meilleure dent de la v9 : 34 mutants actifs, tous q3/q4. Le mutant 16, `admitted_lane_recounted_in_children`, est DISABLED parce que son site n'est plus unique : `q34_witness_search.cpp:168` et `:182` sont identiques. Or le site à deux lignes `admitted_mask |= bit;\n        frame.mask &= …(~bit);` (l. 181-182) est unique (vérifié par script) : le mutant peut être réactivé tel quel. **Aucun mutant produit ne vise la voie q2** (`q2_census.cpp`, 2 858 lignes, sur chaque trame). Les « model_mutants » des portes q2 mutent le modèle attendu, pas le produit (`wspd_q2_census_gate.cpp:470-519`).
- **Tour et chaîne (6 mutants produit compilés).**
  - `parallel_sort` unsorted bucket, `anchor_meb` last maximum, `anchor_meb_proposed` no canonical, `key_index` drop first, et deux mutants `order_failure` (priorité, ledger).
  - `key_index` est accepté sur le seul code 1, sans cause (`CMakeLists.txt:219-224`). La porte FULL écrit ses causes sur stderr, que `run_expect.cmake:13` ne capture pas.
  - Le cœur FULL lui-même n'a **aucun** mutant produit : lots de même niveau, fusions, croissance datée, coupes ouvertes/fermées, contributions, images verticales. La recoupe de la chaîne (`tower_chain.cpp:458-486`) n'en a pas non plus.
  - Les trois mutants `--mutant-{assignment,open,adjacency}` mutent l'**oracle** et `--mutant-census` le harnais (`chain_census_tower_gate.cpp:252-255`). Ils montrent la sensibilité du juge, pas celle du produit.
- **Mutants d'exécution `MHGP9_MUTANT("…")`.** On compte 18 sites dans le chemin produit (`census.hpp:76-180`, `q3.hpp:83,143`, `q4.hpp:82-123`, `wide.hpp:39,61`, `pool.hpp:28,82`). Ils ne sont compilés que sous `MHGP9_TESTING` et ne sont activables que par `mutants_enable` (`src/tower/core/mutants.hpp:153-181`). **Aucun test ne l'appelle.** Le registre `kMutants` compte 132 noms, dont 114 sans site (script sur `mutants.hpp:35-138`). La doctrine de l'en-tête (« chaque nom = un point d'injection + une porte code 4 ») n'est donc pas tenue. B l'avait relevé pour `level-trunc-hi` seulement (`CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md:88-100`, `CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md:70-80`). Les mutants `--inject=` promis par la phase V9-0 (`docs/PLAN_V9.md:52-53`) n'existent pas.
- **Convention de code.** L'en-tête de `mhgp9_gate` annonce « 4 mutant tué » (`CMakeLists.txt:49-50`), mais toutes les portes de mutant de la v9 attendent 1.

## 5. Équivariance, bit-identité, sanitizers, ressources

**Permutations et identifiants.**

- FULL : deux variantes. L'identité, puis l'ordre inversé avec des `PointId` épars (0xFFFFFFFF, 2^31, 0…, `full_ball_tower_gate.cpp:149-156`), et le catalogue inversé (l. 329).
- Générateur : fixtures inversées ou tournées, plancher `nonidentity_orders ≥ 10` pour le front.
- Au-delà de 30 sites, **aucune** porte ne permute l'entrée de la chaîne. Or, sur une grille de 1 mm, les égalités de longueurs et les coquilles liées abondent, et les départages par identifiant (arête propriétaire, graine canonique) sont précisément ce qu'une permutation met à l'épreuve. La sonde de B n'a exercé que deux permutations sur une seule clé (`COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md:44-66`).

**Bit-identité selon le nombre de fils (testé).**

- 0 à 48 fils seulement pour `parallel_sort` ;
- au plus 8 fils pour la tour (`chain_static_paths` : 0/1/4/8) et la chaîne (1/4/7) ;
- au plus 4 fils pour le générateur.

Sur G4, R5 **mesure** l'égalité des condensés W24/W48 sur un seul cas (08/000000 K10, `receipts/g4_tower_r5_20260923/README.md:31-33`).

**Sanitizers.** Les options `MHGP9_SANITIZE` et `MHGP9_TSAN` existent (`CMakeLists.txt:13-25`), mais la CI ne construit qu'en Release GCC (`.github/workflows/morsehgp3d-v9.yml:48-51`). Il y a eu des passages ad hoc de B : Clang ASan/UBSan sur une T2 publique, TSan sur FULL `--static-4` (`PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md:41-43, 96-99`). Le moteur parallèle q3/q4 n'est « jamais passé sous TSan » (`docs/HERITAGE_V7_V8.md:61`).

**Ressources.**

- Testés : l'échec de lancement d'un fil dans la banque de populations (`population_bank_gate.cpp`), celui du tri des présentations de la chaîne (`order_failure_priority_gate.cpp:92-104`), et `bad_alloc` injecté dans le générateur et sous l'API FULL.
- Non enregistré : le mode `--expect-launch-failure` de la porte FULL (`full_ball_tower_gate.cpp:564-575`).
- Toute `std::system_error` est classée « thread_launch_failed » (`full_ball_tower.hpp:1655-1656`, `tower_chain.cpp:570-575`).
- Aucun plafond mémoire déclaré : sous Linux, un OOM est un signal, pas un refus typé.

**Bornes.**

- K > n : la troncature `kmax = min(requested, input_count)` (`full_ball_tower.hpp:884`) n'est jamais testée, puisque toutes les fixtures FULL ont kmax ≤ n et la chaîne T2 n ≥ 12 > K + 1.
- u18 extrême : jugé aux composants (`arith_u18`, `u18_numeric_domain`, jumeaux 18 bits de `wspd_q34_gate.cpp:379-394`). Les coordonnées de T2 restent en revanche sous 100 : la composition clé v8 = clé v7 de la chaîne n'est pas jugée aux bornes.
- Plateaux : la coquille à 12 sites est testée (`shell14`, plancher `shell12 == 6`). Le **refus** au-delà de 12 (`tower_chain.cpp:478-481, 518-522`, `chain_shell_above_12`) ne l'est pas, alors que la fixture `shell30` existe déjà dans le générateur.

## 6. Échelle : les tailles d'intérêt sont absentes des portes

Les trois tests `scale8000/16000/32000` exécutent `q4_saturating_atlas --scale n` (`tests/gen/q4_saturating_atlas_gate.cpp:225-259`) :

- un cylindre synthétique ;
- **une seule arête fournie** ;
- une racine d'atlas certifiée profonde, **zéro** support q4 émis ;
- durée 0,01 s chacun en CI (**mesuré**).

Le commentaire CMake (`CMakeLists.txt:272-275`) dit honnêtement « invariants vérifiés, temps non jugés ». Mais le label fait croire qu'une commande `ctest -L scale8000` couvre la taille d'intérêt au sens de CLAUDE.md (TEST_PLAN § 3.1).

Aucune porte ne lance la chaîne à 8 000 sites ou plus. La plus grande taille est de 1 500 sites, en K5, et seulement en différentiel. À l'échelle, on n'a que :

- les recoupes internes de la chaîne (clé v8 = v7, profondeur et coquille recensées, q_min ; `tower_chain.cpp:458-486`) ;
- la détection partielle d'ancres manquantes par FULL ;
- les ablations ON/OFF appariées des reçus G4.

Aucun de ces contrôles ne voit une clé **entièrement omise** par tous les chemins, obligation déjà reconnue ouverte (`audits/ETAT_COURANT.md:37-48` ; `CONTRE_AUDIT_B_G4_R7B_20260923.md:28`).

**Juge d'échantillon proposé.** Il est sain (prouvé ci-dessous) et en O(m·n), donc compatible avec la règle « pas de juge O(n³) ».

1. Tirer des sites p, énumérer les supports S de 2 à 4 sites parmi les m ≈ K+4 plus proches voisins de p.
2. Pour chaque S positif (poids barycentriques > 0 en rationnels), calculer sa boule B et sa profondeur stricte d sur **tous** les sites.
3. Si d + |S| ≤ min(K+1, n) et que la coquille fait au plus 12 sites, exiger `key(B)` dans le catalogue de la chaîne (`keep_catalogue=true`).

Preuve : q_min(B) ≤ |S|, donc d + q_min ≤ d + |S| ≤ la fenêtre. B appartient alors au catalogue tel que la chaîne et T2 le définissent (`tower_chain.cpp:485-486`, `chain_census_tower_gate.cpp:60-76`).

Complément côté Γ : pour un K-ensemble échantillonné σ de rayon MEB r, il doit exister, à la coupe fermée r, une racine d'ordre K dont la couverture contient σ. Cela demande une requête « point → racines vivantes » que l'API n'offre pas encore.

## 7. CI

Le workflow v9 (`.github/workflows/morsehgp3d-v9.yml`) :

1. construit en Release ;
2. lance `ctest -L '^gate$' --no-tests=error` ;
3. lance ensuite `gcp-migration/tower_selftest_v9.py` en mode normal puis `-O`.

**129 exécutions consécutives ont échoué** : de `b4e480fc` (01:49:50Z) jusqu'à `4291c538`, la dernière réussite étant `c9db64cd` à 01:49:19Z (**mesuré**, `gh run list`). La cause est la même dans les exécutions examinées (`b4e480fc`, `78ce9fd4`, `ec6d1b74`, `0125dc18`) :

- `tower_selftest_v9.py:1063` appelle `git rev-parse HEAD~1` ;
- or `actions/checkout@v7` n'a pas de `fetch-depth` (`morsehgp3d-v9.yml:35-36`) et fait un clone superficiel ;
- résultat : `CalledProcessError … exit status 128`, soit 20/21 selftests.

L'étape CTest passe dans chacune de ces exécutions (106, 123 puis 127 tests). Aucune régression moteur n'est donc cachée **à ce jour**. Mais le badge rouge permanent n'apporte plus aucun signal, et la porte de sortie V9-0 (« suite verte en CI », `docs/PLAN_V9.md:75`) ainsi que l'exigence de portes hermétiques « aucun `git rev-parse` » (`docs/PLAN_V9.md:54-56`) ne sont pas tenues. Le correctif persiste à HEAD `7312beb7`. Le « 21/21 » des audits est un résultat local (`ETAT_COURANT.md:19-20, 506-507`).

## 8. Hygiène des portes

- `tests/tower/census_tower_gate.cpp` : fichier orphelin, non enregistré. Il inclut `../src/pipeline/generate.hpp`, qui n'existe pas en v9 : c'est un reste v7 non compilable, remplacé par la porte de chaîne.
- `tests/gen/q34_family_pruning_gate.cpp` n'est plus qu'une aide incluse : son selftest n'est jamais exécuté (`wspd_q34_gate.cpp:1-5`).
- La porte v8 `q34_collective` n'est pas portée, alors que `lanes/q34_collective.cpp`, `q34_pruning.cpp` et `family_certificate.cpp` sont toujours compilés dans `mhgp9_gen` (`CMakeLists.txt:104-113`) sans servir la chaîne (aucun pool dans `wspd_q34.cpp`).
- `wspd_q34_gate.cpp:69` renvoie à `bench/run_q34_indexed_checks.py`, absent de la v9.
- PASSATION annonce encore « 20 CTests » (`PASSATION.md:16`) et des mutants `--inject` (l. 197).

## 9. Recommandations ordonnées

Pour la v9, et pour toute implémentation alternative (GPU, certificats q3/q4 avant expansion, phase A par maximum d'ID) visant le contrat LiDAR K5/K10 sur G4 :

1. Réparer la CI : `fetch-depth: 0`, ou un dépôt Git temporaire dans le selftest. Ajouter ensuite un job Clang ASan/UBSan sur `-L gate`, et un job Clang TSan sur `-R 'parallel|static|chain|wspd_q2_parallel|wspd_q34|population'`.
2. Faire tourner `chain_census_tower` à K ∈ {1,2,3,5,10}, avec un plancher sur `ledger.dead_q3_proved/dead_q4_proved/core_closed_edges/witness_rejected_pairs` aux petits K. Donner à chaque variante un ordre d'entrée réellement permuté pour le générateur. Ajouter `n ≤ K+1` (par exemple n = 5, K = 10), une translation vers 262 143, et `shell30`, avec refus attendu `unsupported_degeneracy`.
3. **Score de mutation du juge de bout en bout** : lier chacun des 34 mutants du générateur à `chain_census_tower` (K = 3/5/10) et publier les survivants.
4. Porte `scale8000/16000/32000` réelle, sur familles synthétiques versionnées (`uniform`, `terrain`, `clusters`) :
   - le juge d'échantillon du § 6 ;
   - la projection de catalogue triée (clé, niveau, q_min, profondeur, coquille), invariante par permutation, comparée entre une permutation et W1/W8 ;
   - le condensé de la tour après réétiquetage.

   Toute implémentation alternative doit reproduire cette projection octet pour octet sur les coupes LiDAR 8k/16k/32k avant tout chrono G4.
5. Convertir les 18 sites `MHGP9_MUTANT` en mutants compilés, comme `MHGP9_KEY_INDEX_MUTANT`, avec code et cause ; purger `kMutants`. Ajouter des mutants du cœur FULL (coupe ouverte prise pour fermée, lot de même niveau scindé, image verticale décalée, fenêtre `K` au lieu de `K+1`) et de la recoupe de la chaîne.
6. Réactiver le mutant 16 avec son site à deux lignes. Donner une cause stdout au mutant `key_index`.
7. Raccorder `reference/` comme troisième juge sur les fixtures génériques. Réaligner les labels `oracle` et PASSATION.
