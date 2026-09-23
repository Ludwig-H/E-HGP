# SHADOW `H_a` orienté vers le facteur B : rejet de lignes q3/q4

23 septembre 2026. Expérience CPU mono **hors produit et hors registre**, complément de [la palette des proches](SHADOW_HA_Q34_LIDAR_20260923.md). La [source reproductible](shadow_ha_octant_q34_u18_20260923.cpp), les [replays 08/000000 K10](shadow_ha_octant_00_k10_s8_replay_20260923.txt), [08/000000 K5](shadow_ha_octant_00_k5_s8_replay_20260923.txt), [08/000100 K5](shadow_ha_octant_01_k5_s8_replay_20260923.txt), les [autres lignes](shadow_ha_octant_rows_20260923.txt) et les [ressources](shadow_ha_octant_resources_20260923.txt) sont conservés ensemble. Aucun rejet nouveau n'est activé dans Morse HGP 3D.

## Construction et sûreté

Pour chaque site préparé `a`, le même balayage des `8K` rangs spatiaux adjacents que dans le shadow précédent répartit les sites proposés dans huit octants signés autour de `a`. Il garde, dans chaque octant, au plus `2K` sites triés par distance à `a`. Une ligne résiduelle `a×B_node`, avec `|B|≥8`, choisit l'octant du centre de la boîte B vu depuis `a`. Cette direction est **une heuristique de proposition** : les coordonnées ne sont supposées ni alignées ni issues d'une grille de scans. L'expérience compare, sur **les mêmes rectangles et masques q3/q4**, la palette des `2K` proches, l'octant seul, et leur union dédoublonnée. `H=(z−a)·(b−z)>0` et le seuil exact sur `H²` expliquent pourquoi regarder vers B peut augmenter les succès ; l'octant ne décide aucun signe.

Seul `universal_witness(q3/q4,a,box(B),z)` du produit donne un crédit : ses huit coins et ses inégalités entières strictes certifient chaque vrai `b∈B`. Le site `a` n'est jamais proposé, et tout rang appartenant à B est sauté ; les crédits sont des IDs de sites préparés distincts, jamais des retours LiDAR bruts. Pour une même proposition, q4 certifiée implique q3 ; les seuils restent respectivement `K−2` et `K−1`, sans addition entre voies. Une palette insuffisante laisse la ligne indécise et déclenche le chemin ordinaire. Aucun alignement des points ni complétude de la sélection heuristique n'est requis pour l'exactitude.

## Mesures sur trames sans sol entières, grille 1 mm

| Trame, K/s | Paires après rectangles | Fermées, proches | Fermées, octant | Fermées, union | Requêtes exactes proches → octant |
| --- | ---: | ---: | ---: | ---: | ---: |
| 08/000000, K10/s8 | 30 777 213 | 3 499 305 | **5 177 835** | 7 960 608 | 19 876 621 → 6 264 098 |
| 08/000000, K10/s10 | 24 383 658 | 2 599 260 | **3 714 471** | 5 801 775 | 18 879 195 → 5 834 744 |
| 08/000000, K10/s12 | 20 097 638 | 1 987 584 | **2 728 936** | 4 334 742 | 17 959 651 → 5 442 174 |
| 08/000100, K10/s8 | 17 488 839 | 1 619 540 | **2 357 680** | 3 666 013 | 12 984 381 → 4 228 395 |
| 08/000000, K5/s8 | 23 686 751 | 5 591 944 | **5 731 315** | 8 757 717 | 7 453 220 → 2 160 004 |
| 08/000100, K5/s8 | 11 960 420 | **2 499 743** | 2 440 799 | 3 734 501 | 4 256 687 → 1 234 581 |

Les deux trames sont de **la même séquence 08** ; ce n'est pas une qualification multi-séquence. `s8/s10/s12` sont les séparations WSPD, pas des nombres de workers. Les lignes s10/s12 et 08/000100 K10 sont des mesures de propositions seulement : aucun replay pair/cache complet n'est prétendu pour elles. La préparation commune proches+octants coûte `0,214 CPU·s` en K10/s8 sur 08/000000 et `0,148 CPU·s` en K5/s8 ; elle **surfacture** donc la voie octant seule. La passe de lignes octant ajoute respectivement `0,227` et `0,092 CPU·s`. La structure dense du sidecar réserve `8×20` rangs par site même en K5, avant les autres buffers ; seuls `2K` par octant peuvent être proposés. Sa mémoire et son transfert éventuel doivent être mesurés dans la chaîne réelle.

Sur trois cas, le replay parcourt **toutes** les paires résiduelles avec des caches indépendants. Il compare chaque masque de paire au parcours de référence et prouve que chaque ligne entièrement sautée y avait un OU nul. Les 30 777 213, 23 686 751 et 11 960 420 masques sont identiques ; les paires pouvant construire un cover restent exactement `4 507 278`, `2 043 612` et `1 732 176`. Le sidecar ne construit aucun cover et ne chronomètre ni chargement de formes, ni catalogue, ni FULL.

| Replay complet, octant seul | Paires filtrées référence → octant | Visites filtre paire référence → octant | CPU filtre/cache référence → octant | Gain CPU local après préparation + lignes |
| --- | ---: | ---: | ---: | ---: |
| 08/000000, K10/s8 | 30,78 M → 25,60 M | 959,84 M → 902,28 M | 40,244 → 35,958 s | 3,845 s |
| 08/000000, K5/s8 | 23,69 M → 17,96 M | 436,07 M → 389,50 M | 16,004 → 13,945 s | 1,819 s |
| 08/000100, K5/s8 | 11,96 M → 9,52 M | 287,54 M → 266,18 M | 16,129 → 15,738 s | **0,193 s** |

Le gain observé porte sur l'expansion, le cache et le filtre ponctuel, **zéro cover**. Le faible net K5 de 08/000100 empêche toute conclusion K5 générale. L'union ferme davantage de lignes mais ses temps de replay sont contradictoires entre répétitions et charge de l'hôte, parfois supérieurs au chemin sans palette malgré moins de visites ; la recommander sur les seules masses serait une erreur. Les CPU ci-dessus viennent d'une passe séquentielle non randomisée sur hôte partagé ; les temps mur des [ressources](shadow_ha_octant_resources_20260923.txt) montrent la contention. La mémoire maximale K10 de `244 064 KiB` inclut notamment les rectangles et les 30,78 M masques mémorisés pour contre-vérification, pas une estimation de la future activation.

**Décision constructive.** Garder cette palette en SHADOW et tester d'abord **octant seul** dans une ablation chaîne ON/OFF K10, puis K5, avec préparation et mémoire réellement incluses, caches par worker, identités des supports/coquilles/catalogue/FULL et débit GPU/G4. Le coût de sélection par site peut se paralléliser, mais la ligne reste payée pour chaque produit résiduel ; aucun sous-quadratique global ni contrat <1 s ne résulte de ces replays mono. Répéter sur plusieurs séquences, trames brutes avec sol et profil float32 avant tout changement de défaut.

## Provenance

Snapshot produit privé propre `610264dad26427f3c15e1850910b67cf6d5ab4f6`, GCC 13.3, compilation sidecar `-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`, bibliothèque statique SHA-256 `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`. Source sidecar SHA-256 `99d1e9c0bed32d18f0b188f7616413c8f4a6cf66230a1538c484b15ef5040b24`, binaire local SHA-256 `c22366840cb40cd23c8068573d465114ecf5cb35e6435d901a6fc925e40cad6f`. Entrées v8 locales `receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_{00,01}_grid/full.u32le`, SHA-256 `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf` et `ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f`. Ces entrées, le binaire et la bibliothèque sont des dépendances LIVE, non des objets qualifiés par cette note. Commande : `shadow_ha_octant_q34_u18_20260923.cpp` compilé contre `morsehgp3D_v9/src/gen` et `libmhgp9_gen.a`, puis `<binaire> <full.u32le> K s [rows]`; sans `rows`, le replay complet est exécuté.
