# MorseHGP3D — bibliothèque C++20

Ce répertoire contient le cœur C++20/CUDA, ses tests et son API publique. Le composant livrable actuel est un réducteur CPU exact relativement à une tour HGP multi-ordres déclarée complète et exacte par son producteur. Il vérifie la liaison du payload et les invariants structurels rejouables, mais n'authentifie pas cette déclaration amont. Il ne construit pas encore la tour depuis un nuage brut et ne revendique ni statut public exact, ni qualification GCP à 50 000 ou plusieurs millions de points.

## Utilisation publique

L'en-tête d'entrée est :

```cpp
#include <morsehgp3d/morsehgp3d.hpp>
```

Après installation du paquet CMake :

```cmake
find_package(MorseHGP3D CONFIG REQUIRED)
target_link_libraries(mon_programme PRIVATE morsehgp3d::morsehgp3d)
```

Le contrat complet est déclaré dans [`api/point_hierarchy.hpp`](include/morsehgp3d/api/point_hierarchy.hpp). `build_exact_point_hierarchy` reçoit un `CertifiedTowerInput` comprenant :

- les nœuds des forêts horizontales de $T_1$ à $T_K$ et leurs niveaux exacts;
- les arêtes horizontales et verticales avec leur niveau d'activation;
- les simplexes projectables, leurs `PointId`, leurs rayons de cofaces et leur niveau de sortie;
- les identifiants déclarés des trois autorités amont et l'identifiant canonique du payload complet.

La construction échoue fermé si un identifiant d'autorité ou une déclaration exigée manque, si la source est déclarée incomplète ou surrogate, si le payload ne correspond pas à son identifiant, si la tour est structurellement invalide ou si une comparaison dépasse son budget de certification. Ces champs publics forment un contrat avec le producteur; ils ne constituent pas à eux seuls une preuve non forgeable de la géométrie amont.

## Paramètres et sorties

| API | rôle exact |
|---|---|
| `PositiveRationalExponent exp_z` | exposant rationnel positif de $k/r^z$; la valeur par défaut est 3 |
| `SimplexPointWeighting::inverse_radius` | somme certifiée des contributions de rayon inverse élevé à `exp_z` |
| `SimplexPointWeighting::uniform` | contribution exacte égale par simplexe, sans approximation |
| `OrderWeight` | poids rationnel positif entre ordres, normalisé par point |
| `PointHierarchyBudget` | caps de structure, d'incidences et de précision; tout dépassement échoue fermé |
| `select_lambda_cut` | coupe exacte au niveau $k/r^z$ |
| `select_dbscan_radius` | rendu de type DBSCAN par rayon carré exact et ordre de référence explicite |
| `select_excess_of_mass` | condensation et sélection EOM de type HDBSCAN, avec comparaisons certifiées |

Le profil `PointHierarchyBudget::large_resident_30m()` augmente seulement les caps explicites pour une exécution résidente disposant de ressources suffisantes. Il ne préalloue rien et ne vaut ni qualification mémoire, ni promesse de passage à 30 millions de points; la source et la projection restent résidentes et non streamées dans cette version.

Le routage descendant est effectué une seule fois. `terminal_node_by_point()` contient exactement un terminal par point, bruit compris, et `validate_partition_invariants()` rejoue les comptes et les intervalles DFS. Toutes les sélections renvoient un tableau de labels unique et attestent la disjonction des clusters. Le cœur n'expose pas d'argument `splitting`.

Le `ExactPointHierarchyReceipt` lie les paramètres, les poids, les autorités source, le payload et la réduction. Il distingue explicitement :

- `exact_reduction_of_bound_payload`, qui certifie le calcul aval;
- `upstream_source_authority_replayed=false`, car ce module ne refait pas la preuve géométrique amont;
- `public_exact_status_claimed=false`, tant que les portes du produit complet restent ouvertes.

## Validation disponible

Le test dédié `morsehgp3d.api_point_hierarchy` couvre le comparateur de densité, une tour à deux ordres, la partition, les coupes DBSCAN, l'EOM, l'invariance par permutation, la liaison des poids et les rejets fail-closed. Il s'exécute avec :

```bash
ctest --test-dir build/morsehgp3d -R '^morsehgp3d\.api_point_hierarchy$' --output-on-failure
```

Le seul différentiel de cette API est `morsehgp3d.point_hierarchy_sklearn_differential`. Sur une fixture multi-ordres $K=2$ de neuf points, il compare les partitions DBSCAN et HDBSCAN-EOM à scikit-learn sans comparer les numéros arbitraires des labels. L'ordre de référence DBSCAN et `min_samples` valent tous deux 2. Le test est ignoré avec le code 77 si la version installée de scikit-learn ne fournit pas HDBSCAN. Il concerne uniquement les rendus plats : ce n'est ni un oracle de la tour HGP, ni une preuve de la réduction exacte, ni une qualification de performance.

Les obligations mathématiques, cas encore manquants et non-promesses sont détaillés dans la [présentation multi-ordres](../docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md), le [plan de tests](../docs/TEST_PLAN_MORSEHGP3D.md) et le [rapport de performances](../docs/PERFORMANCE_MORSEHGP3D.md).

## Construction et tests

```bash
cmake -S morsehgp3d -B build/morsehgp3d-cpu-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d-cpu-release --parallel
ctest --test-dir build/morsehgp3d-cpu-release --output-on-failure
```

L'installation CMake est minimale par défaut : elle exporte seulement
`morsehgp3d::morsehgp3d`, `morsehgp3d::point_hierarchy`,
`morsehgp3d::contract` et `morsehgp3d::exact`, avec leurs en-têtes publics.
Un consommateur appelle `find_package(MorseHGP3D CONFIG REQUIRED)` puis lie
uniquement `morsehgp3d::morsehgp3d`. Les cibles et en-têtes de recherche
historiques peuvent être ajoutés explicitement à un package de développement
avec `MORSEHGP3D_INSTALL_INTERNAL_TARGETS=ON`; les archives et surrogates
restent exclus dans les deux modes.

Les compilations GCC et Clang utilisent les avertissements stricts. Les targets scientifiques internes restent séparées de `morsehgp3d::morsehgp3d`; l'API de points dépend seulement des briques publiques `contract` et `exact`.

## Frontière produit et archives

La source amont active reste `exact_sparse_frontier` : supports minimaux deux à quatre, incidences utiles, lots exacts, forêts horizontales et verticales, sans mosaïque de Delaunay d'ordre supérieur ni catalogue global. Le réducteur public ne doit jamais devenir un substitut à cette source.

L'ancien point-MST est archivé dans [`archive/surrogates/point_mst_v6/`](archive/surrogates/point_mst_v6/README.md). Les prototypes Phase 15 sans cible ni validation sont dans [`archive/obsolete/phase15_prototypes/`](archive/obsolete/phase15_prototypes/README.md). Ces sources sont exclues du build, de l'installation et de l'API publics.

Toute campagne G4 suit les scripts gardés de [`gcp-migration`](../gcp-migration/) et ne commence qu'après fermeture de la porte scientifique correspondante.
