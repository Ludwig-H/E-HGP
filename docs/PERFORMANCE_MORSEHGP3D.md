# Performances et déploiement de MorseHGP3D

## Statut actuel

> [!WARNING]
> Le dépôt ne contient encore **aucun résultat de performance qualifié pour le produit MorseHGP3D exact** à 50 000, 10 000 001 ou 30 000 000 de points. La campagne de référence est définie, mais sa porte d'entrée est fermée (`entry_gate_satisfied=false`) tant que le coordinateur et le binaire produit v4 ne satisfont pas le contrat complet. Les nombres historiques ci-dessous sont conservés pour la traçabilité ; ils ne constituent ni un SLO atteint, ni une promesse de capacité, ni une preuve d'exactitude.

Cette page distingue systématiquement quatre objets qui ne doivent jamais être comparés comme s'ils mesuraient le même calcul :

- un **résultat exact qualifié**, qui matérialise et certifie toute la source Morse résidente, les dix forêts horizontales, les neuf applications verticales adjacentes, la coupe fermée d'ordre 1, la réduction de Hartigan exacte et la vue finale ;
- une **exécution censurée**, arrêtée avant la fin et qui ne fournit qu'une borne inférieure sur sa durée ;
- un **profil de composant**, utile pour dimensionner une étape mais incomplet par construction ;
- un **surrogate rejeté**, qui calcule un autre objet et ne doit jamais être présenté comme MorseHGP3D.

Le statut normatif de la phase reste `in_progress`, avec `deployment_status=architecture_only` et `public_status=not_claimed` dans [implementation_status.toml](implementation_status.toml). Un benchmark ne peut ni démontrer à lui seul la complétude des incidences, ni promouvoir le statut public à `exact`.

Le test hôte et `morsehgp3d.point_hierarchy_sklearn_differential`, limité à une fixture multi-ordres $K=2$ de neuf points, sont des validations fonctionnelles bornées, pas des mesures de capacité. Le diagnostic local de projection 50 k documenté plus bas reste lui aussi non qualifiant. Une future ligne produit devra séparer le coût de construction de la tour source et celui de la projection multi-ordres, comme l'exige la section 17 de la [présentation mathématique](math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md).

## Diagnostic local du réducteur sur 50 000 points

Le harnais [`point_hierarchy_projection_benchmark.cpp`](../morsehgp3d/src/tools/point_hierarchy_projection_benchmark.cpp) mesure uniquement le réducteur public sur une tour résidente synthétique équilibrée d'ordre 1. Il se construit avec `MORSEHGP3D_BUILD_TOOLS=ON`, puis s'exécute ainsi :

```bash
build/morsehgp3d-cpu-release/morsehgp3d_point_hierarchy_projection_benchmark --points 50000
```

Trois processus Release frais, sans échauffement, ont été exécutés le 3 août 2026 sur deux processeurs logiques Intel Xeon Platinum 8370C, avec GCC 13.3.0. L'hôte n'était pas isolé. Les valeurs brutes, l'environnement, les digests du binaire et des quatre sorties sont scellés dans [`point_hierarchy_projection_50k_local_20260803.json`](validation/point_hierarchy_projection_50k_local_20260803.json).

| Étape | Médiane de trois exécutions |
|---|---:|
| génération de la fixture résidente | 74,509340 ms |
| scellement canonique du payload | 144,341287 ms |
| construction de la hiérarchie de points | 2 792,494187 ms |
| rendu DBSCAN | 2,170054 ms |
| rendu HDBSCAN-EOM | 16,718485 ms |
| replay des invariants de partition | 2,832847 ms |
| somme des étapes mesurées | 3 084,805842 ms |
| pic RSS du processus | 127 708 Kio, soit environ 124,7 Mio |

La fixture contient 50 000 simplexes ponctuels, 99 999 nœuds source et 99 998 arêtes. La sortie contient 100 000 nœuds. Le rendu DBSCAN produit 1 562 clusters et 16 points bruités; le rendu EOM produit 1 561 clusters et aucun point bruité. Les deux partitions sont disjointes et le replay retrouve exactement un terminal par point. Ces nombres valident la cohérence interne de cette fixture; ils ne constituent pas une vérité terrain de clustering.

Ce diagnostic n'est pas un p95 et ne couvre ni le nuage brut, ni la construction de la source HGP, ni la forêt complète de $T_1$ à $T_K$, ni les applications verticales, ni le streaming, ni CUDA, ni GCP. Il ne qualifie donc pas la cible produit 50 k et ne permet aucune extrapolation vers dix ou trente millions de points. `PointHierarchyBudget::large_resident_30m()` ne fait qu'ouvrir explicitement des plafonds fail-closed pour un processus disposant de la mémoire correspondante; ce profil n'est pas une preuve que la charge tient en mémoire ou termine.

## Profil local des supports trois et quatre

Le binaire de profil [`exact_higher_support_growth_profile.cpp`](../morsehgp3d/tests/profiling/exact_higher_support_growth_profile.cpp) mesure séparément la frontière exacte des triplets et quadruplets. Il utilise trois nuages dyadiques — uniforme, amas séparés et amas multi-échelles — pour $n\in\left\lbrace32,64,128\right\rbrace$ et $K\in\left\lbrace1,5,10\right\rbrace$. Le cas $K=1$ est explicitement non applicable aux arités trois et quatre. Chaque série s'arrête dès que le run précédent ne termine pas dans 5 000 unités ou dépasse une frontière de $8n$ entrées; ce coupe-circuit évite de transformer un profil négatif en campagne combinatoire.

Avant la sonde Morton locale, les six runs applicables à $n=32$ atteignent tous `work_unit_limit`. L'univers exact contient 4 960 triplets et 35 960 quadruplets, soit 40 920 supports. Après 5 000 unités, seulement 53 à 62 supports sont résolus, c'est-à-dire environ 0,13 à 0,15 %. De 4 859 à 4 863 visites, soit environ 97 % du budget, sont consommées par la recherche de témoins de rang; aucun prune de rang, aucune analyse terminale et aucune requête de boule fermée n'est atteinte. Le pic de frontière reste faible, entre 15 et 17 entrées : le défaut est donc le travail, pas une matérialisation de l'univers.

La version finale locale, `schema=6 / traversal=5`, applique d'abord une porte universelle de bon centrage : la borne inférieure du déterminant de Gram et celles de tous les numérateurs barycentriques doivent être strictement positives sur le produit. Un échec de cette porte reste inconclusif et poursuit la subdivision exacte; il n'élimine aucun support. Pour un produit admissible, au plus 91 positions Morton sont proposées. Elles sont regroupées sous une antichaîne de racines LBVH externes de masse au plus $H$, avec au plus 182 évaluations exactes cellule--ou--feuille : une cellule certifiée intérieure apporte toute sa masse, une cellule certifiée extérieure ou tangente est sautée et une cellule ambiguë retombe seulement sur ses feuilles effectivement proposées. `rank_frontier` reste vide; aucune DFS globale n'est cachée dans ce fallback.

Le profil post-sonde suivant a été exécuté une seule fois avec la même limite de 5 000 unités :

| Famille | $K$ | supports résolus sur 40 920 | visites de témoins de rang | produits refusés par la porte géométrique | Statut |
|---|---:|---:|---:|---:|---|
| uniforme | 5 | 1 793 | 1 063 | 2 158 | `NO-GO: work_unit_limit` |
| uniforme | 10 | 1 350 | 1 991 | 1 668 | `NO-GO: work_unit_limit` |
| amas séparés | 5 | 2 952 | 17 | 2 749 | `NO-GO: work_unit_limit` |
| amas séparés | 10 | 2 922 | 70 | 2 723 | `NO-GO: work_unit_limit` |
| amas multi-échelles | 5 | 2 838 | 505 | 2 507 | `NO-GO: work_unit_limit` |
| amas multi-échelles | 10 | 2 825 | 531 | 2 494 | `NO-GO: work_unit_limit` |

Ce résultat montre une amélioration locale nette face aux 53--62 supports de l'ancien parcours, mais les six cas atteignent encore le coupe-circuit : la porte de croissance reste fermée. Les runs $n=64$ et $n=128$ sont donc censurés; leurs univers auraient respectivement 677 040 et 11 009 376 supports. Le champ interne `ru_maxrss` est rejeté : il était déjà contaminé ou hérité avant le premier cas et `peak_rss_growth=0` dès ce cas. Une nouvelle exécution complète, surveillée extérieurement via `/proc/<pid>/status` toutes les 20 ms, a observé `EXTERNAL_PEAK_RSS_KIB=13896`, soit 13 896 Kio. C'est le pic échantillonné du processus complet, pas une mesure par famille ni une base d'extrapolation vers 50 000 points. Le harnais lit désormais `VmHWM` dans `/proc/self/status` sous Linux avant le fallback `getrusage` afin d'éviter ce high-water hérité lors des prochains profils.

Il serait invalide de lancer 50 000 points sur GCP ou d'extrapoler vers dix millions à partir de cette ligne. La campagne facturable reste bloquée avant démarrage; GCP n'a pas été utilisé pour ce profil.

## Campagne de qualité du clustering

Le plan [`point_hierarchy_quality_campaign_v2.json`](../morsehgp3d/tests/profiling/point_hierarchy_quality_campaign_v2.json), SHA-256 `0c41a6ad648816fbe19326db85dea7b3aca6535f7ac1c27022d328abd3c213e8`, fixe la comparaison demandée sans fabriquer de résultats. Il construit une seule source exacte complète $T_1,\ldots,T_{10}$ par nuage, réutilise ses préfixes $K\in\left\lbrace1,5,10\right\rbrace$, puis rend DBSCAN et HDBSCAN-EOM pour $\mathrm{expZ}\in\left\lbrace1,2,3\right\rbrace$ avec `min_cluster_size=20` et `min_samples=K`. Les poids d'ordre valent exactement $1/K$ et la distribution simplexe--point emploie le poids de rayon inverse.

À 50 000 points, la matrice comporte quatre difficultés : 8 boules séparées, 64 amas équilibrés multi-échelles, 24 filaments et 96 paires d'amas déséquilibrées reliées par des ponts. Les paliers 10 000 001 et 30 000 000 utilisent le profil multi-échelles 64. Les coordonnées dyadiques compressées, la vérité terrain et leurs digests sont matérialisés; aucune tour synthétique préconstruite ni source surrogate n'est admise.

Chaque rendu publiera la proportion classifiée, le nombre de clusters, ARI et NMI sur tous les points, sur les vrais inliers et sur les seuls points classifiés, la couverture des vrais inliers, la précision, le rappel et le F1 du bruit, ainsi que les erreurs de fragmentation et de fusion. Les baselines scikit-learn utilisent le même $K$ comme `min_samples`; DBSCAN reçoit le même rayon calibré par quantile exact et shell d'égalité complet. Les baselines massives peuvent être omises uniquement avec le statut explicite `not_run_resource_cap`.

Le protocole compte 84 transactions : 6 générations, 6 constructions exactes $T_1$ à $T_{10}$, 54 réductions/rendus et 18 baselines. Un producteur et un vérificateur scientifique distincts sont obligatoires. Le spool est transactionnel, reprend au prochain ordinal non committé et lie le bundle source durable à tous les préfixes. Le conteneur épingle NumPy, SciPy, scikit-learn, Joblib et threadpoolctl avec hashes. La porte reste `campaign_entry_gate_satisfied=false` tant que les exécutables réels n'existent pas; le test de contrat emploie seulement de faux exécutables et ne produit aucune mesure scientifique.

## Mesures existantes et provenance

| Mesure | Périmètre réellement chronométré | Observation | Statut utilisable |
|---|---|---:|---|
| 50 000 points, ordre maximal 10, `reference_cpu` | Tentative mono-processus allant de la génération CPU vers une forêt matérialisée ; arrêt pendant `sparse_pair_session` | durée totale censurée à droite : **au moins 300,000014 s**, donc plus de 300 s | diagnostic HGP complet inachevé ; aucune hiérarchie, aucun p95, GPU inutilisé |
| 50 000 points, 30 répétitions, ancien point-MST | Coordonnées déjà en mémoire jusqu'à dix hiérarchies substitutives matérialisées ; génération synthétique exclue | p50 **88,803800 ms**, p95 **95,791070 ms**, p99 et maximum **96,045749 ms** | **surrogate rejeté et archivé**, interdit comme résultat produit |
| 10 000 000 points, `cuda_g4` | Génération, canonicalisation, construction Morton/Yao48 et préfixe de frontière de paires tuilée | **26,618277005 s** internes ; **29,59 s** murales | profil de composants incomplet, sans forêt ni hiérarchie et à provenance insuffisante |
| 30 000 000 points, `cuda_g4` | Même sous-chaîne Morton/Yao48/frontière partielle | **78,132751789 s** internes ; **81,09 s** murales | profil de composants incomplet, sans forêt ni hiérarchie et à provenance insuffisante |

### Tentative HGP à 50 000 points : borne censurée

L'artefact [phase14_complete_resident_50k_k10_g4_d250d71.json](validation/phase14_complete_resident_50k_k10_g4_d250d71.json), de SHA-256 `67c8a45398c73dcfcd02b7e48d5acb6ec1d153775a9c9eae3ed191e79a8a6316`, correspond au code `d250d71756e4fb8a0f28e1a2d9f1d1b274b2af95`. Il porte explicitement les valeurs `pipeline_complete=false`, `scientific_result_materialized=false`, `qualification_claimed=false`, `public_status=not_claimed` et `p95_ms=null`.

La décomposition enregistrée est la suivante :

| Étape | Durée |
|---|---:|
| génération | 65,626 ms |
| canonicalisation | 3,099 ms |
| LBVH | 15,361 ms |
| support des paires, interrompu | 299 915,926 ms |
| étapes aval | 0 ms, non exécutées |
| total observé | 300 000,014 ms |

La limite opérationnelle de 300 s a été atteinte alors que la partition des paires n'était pas complète. Cette observation dit seulement que ce code, ce backend CPU de référence et cette entrée n'ont pas terminé avant la limite. Elle n'est ni une mesure de la forêt complète, ni un percentile, ni une mesure GPU. Son contexte est consigné dans [PHASE14_PROGRESS.md](validation/PHASE14_PROGRESS.md), le [plan de test](TEST_PLAN_MORSEHGP3D.md) et la [feuille de route](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md).

### Point-MST à 95,791070 ms : résultat à exclure

La synthèse historique [phase15_guarded_industrial_50k_summary_g4_d69539a.json](validation/phase15_guarded_industrial_50k_summary_g4_d69539a.json), SHA-256 `253fa3da6fa42a10f321e08c07a5b2d759f17c6cbe3807cbf7ca4e121545d72e`, et son [environnement](validation/phase15_guarded_industrial_environment_g4_d69539a.json), SHA-256 `16989a7c3310b418d276049e8bddde66fb1671c9f12c122aa151ec9f6ba18ecf`, décrivent 30 répétitions sur trois familles synthétiques. Les p95 par famille sont 90,238639 ms, 88,803800 ms et 96,045749 ms ; le p95 agrégé vaut 95,791070 ms.

Le reçu lie le commit `d69539a18adc1e5815bd354f70e773a4a8a1d0f6` au binaire de 1 883 544 octets, SHA-256 `51bb7d8aa5565ea6c39eebfe77e7a34232e92bcb5c9553807cbaf84b079f726d`. Le matériel déclaré est une RTX PRO 6000 Blackwell de 96 Go, pilote 580.173.02, et un AMD EPYC 9B45 à 48 processeurs logiques. Cette bonne provenance matérielle ne corrige pas l'inadéquation algorithmique du calcul.

Ces temps concernent un MST sur les points avec exactement `n-1` fusions. Le calcul omet les identités simplexe/facette, les cofaces, les incidences silencieuses, le journal de couverture et les applications verticales. Il ne calcule donc pas le produit MorseHGP3D. Son code source est déjà isolé hors construction, installation et API dans [archive/surrogates/point_mst_v6/README.md](../morsehgp3d/archive/surrogates/point_mst_v6/README.md), et l'abandon architectural est recensé dans [archive/abandoned/README.md](archive/abandoned/README.md).

Les JSON historiques restent des pièces d'audit. Ils doivent être étiquetés **archive uniquement**, exclus des tableaux de résultats produit et exclus de toute régression de performance servant de seuil à l'implémentation exacte.

### Profils de composants à 10 M et 30 M

Les artefacts [10 M](validation/phase15_device_frontier_run5_direct_10m_g4_20260730.json) et [30 M](validation/phase15_device_frontier_run5_direct_30m_g4_20260730.json) mesurent une source `morton_yao48_device_tiled_pair_frontier_direct_scale`. Ils s'arrêtent avec `stop_reason=candidate_capacity`, `coverage_complete=false` et `component_only=true`. Ils ne produisent ni les événements Morse complets, ni les forêts horizontales, ni les applications verticales, ni une hiérarchie sur les points.

| Champ | 10 000 000 points | 30 000 000 points |
|---|---:|---:|
| temps interne total | 26,618277005 s | 78,132751789 s |
| temps mural externe | 29,59 s | 81,09 s |
| génération | 16,335231839 s | 48,976825229 s |
| canonicalisation | 2,529860646 s | 7,730035364 s |
| construction | 6,136369533 s | 19,426792885 s |
| frontière partielle | 0,224411528 s | 0,274107145 s |
| ancres terminées | 4 897 | 7 308 |
| candidats conservés | 493 663 | 807 304 |
| paires non résolues | 49 999 983 007 247 | 449 999 958 292 914 |
| pic RSS hôte | 5 851 193 344 octets | 17 331 388 416 octets |
| minimum de mémoire GPU libre | 98 198 749 184 octets | 94 358 863 872 octets |
| mémoire GPU attribuée à la traversée | 1 919 999 920 octets | 5 759 999 920 octets |

Les temps muraux externes sont conservés dans [phase15_device_frontier_run5_direct_10m_time_g4_20260730.txt](validation/phase15_device_frontier_run5_direct_10m_time_g4_20260730.txt) et [phase15_device_frontier_run5_direct_30m_time_g4_20260730.txt](validation/phase15_device_frontier_run5_direct_30m_time_g4_20260730.txt). Le [manifeste SHA-256](validation/phase15_device_frontier_run5_artifacts_sha256_g4_20260730.txt) fixe les digests des quatre fichiers : les JSON 10 M et 30 M portent respectivement `92e2e6abb7ca0daffb5c904e28a0cc4a8ceef873573a2ecc6f4f35365cd920ab` et `21395348342537ebb3fb58e9acceb80d785ec5654796d09f8595732f1806d598`. Le [digest du binaire](validation/phase15_device_frontier_run5_binary_sha256_g4_20260730.txt) vaut `53bf71af11cb30ac024bb8c9508515a8e2067aa610029e702edb12b87ec44ae1`.

Les JSON déclarent le commit `10a5e76d29efe77d06ff931b66ac7bb14a583e06`, mais celui-ci n'est pas disponible localement. La provenance reste donc insuffisante : aucun patch source, journal de construction, résumé d'environnement, manifeste final du harnais ou reçu de cycle de vie GCP propre à `run5` n'est présent. [PHASE15_PROGRESS.md](validation/PHASE15_PROGRESS.md) interdit explicitement d'en faire une qualification. Ces deux points ne permettent aucune extrapolation linéaire du temps ou de la mémoire d'une hiérarchie exacte.

> [!NOTE]
> Le profil historique « 10 M » utilise exactement 10 000 000 de points. La campagne normative utilise **10 000 001** points afin de franchir sans ambiguïté le seuil de dix millions. Ce sont deux charges distinctes.

## Campagne reproductible de référence

La seule campagne à employer pour de futurs chiffres produit est le plan [phase15_true_hgp_scale_campaign_v4.json](../morsehgp3d/tests/profiling/phase15_true_hgp_scale_campaign_v4.json), SHA-256 `2de957bdd757ad4da3e81735dad3d19add258df14ef379ce597796b4c812a34a`, exécuté par [phase15_true_hgp_scale_campaign_v4.py](../morsehgp3d/tests/profiling/phase15_true_hgp_scale_campaign_v4.py), SHA-256 `da09840a6576d695545564547ba67a1365c5b511f0b0a1a5d5dd2f566b11debc`. Le stockage transactionnel et la reprise sont définis dans [campaign_runtime.py](../morsehgp3d/tests/profiling/campaign_runtime.py), et le contrat du harnais est testé par [test_phase15_true_hgp_scale_campaign_v4.py](../morsehgp3d/tests/profiling/test_phase15_true_hgp_scale_campaign_v4.py).

À la date de cette page, ce plan indique :

- `backend=cuda_g4`, `profile=hgp_reduced`, ordre maximal 10 ;
- `entry_gate_satisfied=false` ;
- `execution_status=blocked_until_true_hgp_v4_product_coordinator_binary_and_entry_gate` ;
- aucune exécution scientifique GCP de ce protocole.

Il est interdit de remplacer ce plan par le point-MST archivé, par un profil « pair-first », par le composant Morton/Yao48 ou par un oracle exhaustif placé dans le chemin produit.

### Matrice obligatoire

| Porte | Charge | Famille et répétitions | Plafond mural par exécution | Condition de passage |
|---|---:|---|---:|---|
| P0 | 50 000 | trois familles ; 2 échauffements puis 10 mesures fraîches par famille, soit 30 mesures | 30 s | tous les certificats valides et p95 agrégé `warm_e2e` strictement inférieur à 1 s |
| P1 | 1 000 000 | `affine_uniform_binary64`, graine 5101 | 600 s | P0 réussie ; checkpoint forcé, processus neuf, recertification et replay identiques |
| P2 | 10 000 001 | `affine_uniform_binary64`, graine 5101 | 3 600 s | P1 réussie ; même contrat de reprise et d'égalité des digests |
| P3 | 30 000 000 | `affine_uniform_binary64`, graine 5101 | 7 200 s | P2 réussie ; même contrat de reprise et d'égalité des digests |

Les trois familles à 50 000 points sont `affine_uniform_binary64`, `jittered_dyadic_grid3d` et `balanced_multiscale_clusters`. Leurs graines d'échauffement et de mesure sont fixées dans le plan v4 ; les changer crée une autre campagne. P0 compte 36 transactions au total, dont 30 mesures. Chaque mesure part d'un nuage frais. Une reprise après préemption n'est jamais comptée comme un échauffement gratuit ou comme une nouvelle mesure.

Le seuil de 1 s est une **porte de progression secondaire**, pas un résultat acquis. Le plan de test conserve un objectif produit primaire de p95 inférieur à 100 ms, lui aussi non démontré. Un échec à P0 interdit P1 ; un échec à P1 interdit P2 ; un échec à P2 interdit P3.

### Frontière du chronométrage

Le champ de référence est `timings_ns.warm_e2e` :

1. il commence avec les coordonnées brutes déjà présentes en mémoire hôte ;
2. il inclut canonicalisation, construction et recertification de la source résidente v7, coupe fermée d'ordre 1, dix réductions horizontales, neuf applications verticales, manifeste de Hartigan exact, validation de la vue et scellement de la sortie ;
3. il se termine seulement lorsque les artefacts de sortie et leurs digests sont scellés ;
4. la génération synthétique du nuage et la gestion externe du spool ne sont pas incluses, mais doivent être rapportées séparément.

Les temps par étape obligatoires sont `cloud_generation`, `canonicalization`, `source_construction`, `source_recertification`, `k1_closed_cut`, `horizontal_reduction`, `vertical_maps`, `hartigan_manifest`, `condensation`, `output_seal` et `warm_e2e`. Il faut publier les valeurs brutes en nanosecondes ainsi qu'une conversion lisible, sans additionner des étapes qui se chevauchent sur CPU et GPU.

### Conditions d'un résultat exact

Une ligne de résultat n'est publiable que si l'exécution certifie simultanément :

- tous les événements directs, toutes les premières incidences co-minimisantes et tous les porteurs canoniques nécessaires pour chaque ordre de 1 à 10 ;
- la saturation complète des fenêtres de rang, sans mosaïque de Delaunay d'ordre supérieur, sans `Gamma`, `Star` ou arène globale d'incidences facette-coface ;
- la partition `qR` complète de chaque lot et la réduction horizontale de chaque ordre ;
- la coupe fermée d'ordre 1 et les neuf applications verticales adjacentes, sans résultat partiel ou conditionnel ;
- le manifeste de niveaux de Hartigan sous forme rationnelle réduite canonique, jamais sérialisé en `binary64` ;
- la vue `at_least20`, dont la cardinalité est l'union des `PointId` distincts portés par les facettes de la composante ;
- zéro échec numérique, zéro obligation non résolue et zéro dégénérescence non supportée ;
- le replay exact après checkpoint avec égalité des digests de la source v7, de K1, des forêts horizontales, des applications verticales, de Hartigan et de la vue ;
- l'identité du code, du binaire, du plan, des capacités, de l'environnement et de la session GCP.

Une sortie plausible, un accord moyen avec un oracle ou un bon temps ne remplace aucun de ces certificats.

## Métriques à enregistrer

Le reçu v4 couvre déjà les temps algorithmiques, les tailles d'artefacts, les pics mémoire déclarés, les compteurs de travail et les chaînes de digests. Pour une publication de performance détaillée, il doit être accompagné d'un sidecar système scellé dans la même chaîne de provenance.

| Domaine | Champs minimaux à publier | Raison |
|---|---|---|
| identité | SHA Git, SHA-256 du binaire, du plan, des capacités, du conteneur et de chaque artefact | rendre l'exécution reconstruisible et empêcher le mélange de campagnes |
| CPU | modèle, sockets, cœurs logiques, affinité, nombre de workers, compilateur, temps utilisateur/système, utilisation, changements de contexte | distinguer parallélisme utile, attente GPU et contention |
| GPU | UUID, modèle, compute capability, pilote, CUDA, temps kernel par étape, nombre de lancements et synchronisations, octets H2D/D2H | séparer calcul, transferts et synchronisation |
| mémoire hôte | capacité, pic RSS du processus, pic cgroup, allocations de travail et scratch | démontrer que la charge tient réellement dans l'enveloppe |
| mémoire GPU | capacité, pic attribué, minimum libre, scratch et réservations par étape | détecter les plafonds et marges réelles de VRAM |
| I/O | octets lus/écrits par le processus, tailles checkpoint/spool/sortie, nombre et durée des `fsync`, type de disque et système de fichiers | quantifier le coût de durabilité et de reprise |
| travail Morse | événements directs, premières incidences, porteurs, lots, groupes `qR`, blocs saturés, nœuds visibles/invisibles et affectations verticales par ordre | expliquer le temps par la quantité de travail exacte |
| certification | drapeaux de complétude, compteurs d'échecs, digests K1/horizontaux/verticaux/Hartigan/vue, reçu de replay | interdire un temps rapide obtenu sur un calcul incomplet |
| cycle GCP | projet, zone, instance, modèle Spot, génération, début/fin UTC, préemption, échéances GCE/invité et reçu `TERMINATED` | rattacher coût, matériel et fermeture à la cible exacte |

Le sidecar doit préciser sa fréquence d'échantillonnage et son surcoût mesuré. Il n'acquiert valeur de provenance que si son chemin, sa taille et son SHA-256 sont liés au reçu de campagne ; un fichier autonome reste diagnostique.

### Statistiques et échecs

Pour les 50 000 points, le p95 suit la règle du rang le plus proche fixée par le plan : rang 10 sur 10 pour chaque famille et rang 29 sur 30 pour l'agrégat. La publication doit aussi fournir les 30 valeurs brutes, puis minimum, médiane, p95, p99 et maximum par famille et pour l'ensemble.

Un timeout, une préemption non reprise, un certificat invalide ou une sortie absente est un échec ou une observation censurée. Il ne doit jamais disparaître du dénominateur ni être remplacé par la dernière durée partielle. Les percentiles des seules réussites peuvent être montrés à titre diagnostique uniquement si le nombre total de tentatives, d'échecs et de censures est affiché dans le même tableau.

Pour les portes massives, chaque point est une campagne de capacité à graine fixée, pas une distribution statistique. Publier le temps total, chaque étape, les ressources, le volume de travail, le coût GCP estimé à partir de la durée facturée, et le résultat du replay. Ne pas extrapoler P2 ou P3 depuis les profils de composants historiques.

## Déploiement GCP gardé

La cible de référence documentée est une `g4-standard-48` Spot, 48 vCPU, 180 Go de mémoire hôte et une RTX PRO 6000 Blackwell Server Edition de 96 Go. Les couples autorisés par l'infrastructure sont `europe-west4-a/ehgp-blackwell-spot` et le repli `europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, dans le projet `devpod-gpu-exploration`. La documentation opérationnelle complète se trouve dans [gcp-migration/README.md](../gcp-migration/README.md).

> [!CAUTION]
> La campagne v4 est actuellement bloquée. Aucun benchmark scientifique ne doit démarrer tant que son binaire produit, son échange de capacités et sa porte d'entrée ne sont pas satisfaits. Les étapes suivantes décrivent la procédure future ; elles ne sont pas une attestation d'exécution.

Une session autorisée suit obligatoirement cet ordre :

1. fixer explicitement projet, zone et nom d'instance, puis vérifier les quotas avec [check_quotas.sh](../gcp-migration/check_quotas.sh) ;
2. si la cible n'existe pas, utiliser uniquement [deploy.sh](../gcp-migration/deploy.sh), qui exige `g4-standard-48`, le label `project=e-hgp`, le provisioning `SPOT`, `instanceTerminationAction=STOP` et une durée bornée ;
3. régler si nécessaire la durée avec [set_max_run_duration_and_verify.sh](../gcp-migration/set_max_run_duration_and_verify.sh), entre 30 secondes et huit heures et aussi courte que la porte à exécuter le permet ;
4. créer hors dépôt une clé ED25519 OS Login éphémère dont la durée couvre la session sans devenir persistante ;
5. démarrer exclusivement avec [start_and_verify.sh](../gcp-migration/start_and_verify.sh), jamais avec `gcloud compute instances start` ; le script doit certifier le coupe-circuit GCE et armer puis relire le coupe-circuit invité ;
6. exécuter [blackwell_preflight.sh](../gcp-migration/blackwell_preflight.sh) et exiger un bilan vert avant tout calcul ;
7. lancer le harnais v4 seulement si l'échange de capacités correspond exactement au plan et si la porte scientifique est ouverte ;
8. quel que soit le résultat, arrêter la même cible avec [stop_and_verify.sh](../gcp-migration/stop_and_verify.sh) en donnant `--yes --expected-last-start-timestamp HORODATAGE`, puis archiver la preuve finale `TERMINATED` ;
9. inventorier les autres VM `project=e-hgp` actives sans les arrêter ni se les attribuer.

Si l'état GCP, SSH, le coupe-circuit invité ou l'arrêt final ne peut pas être relu, la session est en échec fermé. Le rapport doit alors donner le projet, la zone, le nom, la génération, le dernier état connu et la commande exacte de contrôle. Aucun résultat de performance issu d'une cible dont l'arrêt n'est pas certifié ne doit être publié comme campagne complète.

## Manifeste minimal d'une publication

Chaque campagne qualifiée doit déposer, sous un identifiant immuable :

- le plan JSON et son SHA-256 ;
- le reçu de capacités du binaire et son SHA-256 ;
- le binaire ou une référence durable permettant de retrouver exactement son digest ;
- les requêtes et reçus JSONL chaînés, le journal de construction et les versions du compilateur ;
- les coordonnées ou le générateur, ses paramètres et ses graines ;
- les checkpoints, leurs tailles et digests, avec le reçu de reprise ;
- les artefacts source v7, K1, horizontaux, verticaux, Hartigan et vue, tous scellés ;
- le sidecar CPU/GPU/mémoire/I/O et sa fréquence d'échantillonnage ;
- le résumé statistique dérivé uniquement des reçus valides ;
- le reçu de cycle GCP jusqu'à l'état final `TERMINATED`.

Le résumé doit répéter `phase`, `backend`, `profile`, `mode`, `deployment_status` et `public_status`. Toute correction d'un reçu crée un nouvel artefact et un nouveau digest ; elle ne réécrit pas silencieusement l'original.

## Ce qui n'est pas promis

- Le p95 de 95,791070 ms ne mesure pas MorseHGP3D et n'est pas une base de comparaison produit.
- La tentative à 50 000 points ne prouve pas qu'une implémentation exacte terminera en 300 s ; elle fournit seulement une borne censurée pour un ancien chemin CPU.
- Les 26,6 s à 10 M et 78,1 s à 30 M ne sont pas des temps de forêt, de hiérarchie, de clustering ou d'extraction finale.
- Aucun besoin mémoire de la chaîne exacte à 10 000 001 ou 30 000 000 de points n'est encore qualifié.
- Aucun SLO inférieur à 100 ms ou à 1 s n'est actuellement atteint ou revendiqué.
- La capacité Spot et la disponibilité d'une G4 ne sont jamais garanties par le quota seul.
- Un résultat exact n'implique pas automatiquement que tous les jeux de données réels ont le même coût que les trois familles synthétiques.

Les décisions de preuve et les limites mathématiques restent gouvernées par [STATUT_PREUVES_ET_HEURISTIQUES.md](math/STATUT_PREUVES_ET_HEURISTIQUES.md), l'enchaînement des jalons par [ROADMAP_IMPLEMENTATION_MORSEHGP3D.md](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md), et les critères de qualification par [TEST_PLAN_MORSEHGP3D.md](TEST_PLAN_MORSEHGP3D.md). Cette page documente les mesures et le protocole ; elle n'ouvre ni ne ferme une phase.

**Traçabilité de cette page : GCP non utilisé.**
