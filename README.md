# E-HGP — MorseHGP3D

MorseHGP3D construit des hiérarchies 3D multi-ordres sans matérialiser la mosaïque de Delaunay d'ordre supérieur. Le dépôt sépare la source géométrique HGP sur les simplexes, la réduction aval en une hiérarchie laminaire de points et les rendus plats de clustering.

> [!IMPORTANT]
> État courant : la Phase 15 reste `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. Son réducteur aval utilise le mode `exact_relative_multi_order_laminar_point_projection_v1` : il est disponible et testé relativement à une tour de $T_1$ à $T_K$ déclarée complète et exacte par son producteur, puis liée à son payload par reçus. Le réducteur n'authentifie pas cette vérité amont. Le producteur géométrique complet de la tour, sa qualification GCP et les capacités 50 000 ou 10 000 001 points ne sont pas terminés. Aucun benchmark ne promeut ce statut.

## API de hiérarchie de points

L'en-tête public [`morsehgp3d/morsehgp3d.hpp`](morsehgp3d/include/morsehgp3d/morsehgp3d.hpp) et la cible CMake `morsehgp3d::morsehgp3d` exposent une seule voie aval :

1. recevoir les forêts horizontales de tous les ordres, leurs coutures verticales et les simplexes projectables avec leurs reçus;
2. ordonner exactement les niveaux de densité avec l'exposant rationnel positif `exp_z`;
3. distribuer les contributions simplexe--point selon `inverse_radius` ou `uniform`, puis appliquer des poids rationnels entre ordres;
4. construire le merge tree multi-ordres et router chaque point une seule fois, de façon descendante et irréversible;
5. produire une coupe `lambda_cut`, une coupe de rayon `dbscan_radius` ou une sélection `excess_of_mass` de type HDBSCAN.

Chaque point possède un terminal unique. Les clusters d'une coupe ou d'une sélection forment donc une antichaîne et sont deux à deux disjoints; un point ne peut pas recevoir deux étiquettes. Aucun argument `splitting` n'est présent dans le cœur.

La fonction `build_exact_point_hierarchy` refuse une source déclarée surrogate ou incomplète, une déclaration d'exactitude absente et un payload incohérent avec son identifiant. L'appelant peut recalculer cet identifiant : ce contrôle lie le contenu, mais n'authentifie pas la vérité scientifique de la déclaration amont. Le reçu annonce seulement `exact_reduction_of_bound_payload=true`; l'autorité scientifique de la tour n'est pas rejouée et `public_exact_status_claimed` reste faux.

## Architecture active

Le chemin produit amont vise une source sparse exacte : catalogue multi-ordre des paires de rang fermé utile, frontière indépendante des triangles aigus, frontière des tétraèdres bien centrés, incidences silencieuses, forêts horizontales et applications verticales. Il évite les catalogues globaux de cellules, cofaces et incidences; les oracles exhaustifs restent bornés et hors du chemin produit.

Le nouveau module de points ne remplace pas cette source. Il consomme une tour sous autorité externe et n'invente aucune complétude au moyen d'un MST de points, d'un graphe de voisinage ou d'une approximation numérique.

## Exactitude, tests et performances

- La [présentation mathématique](docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md) définit les niveaux multi-ordres, les poids, le canal `stay`, la laminarité et les trois rendus.
- Le [plan de validation](docs/TEST_PLAN_MORSEHGP3D.md) distingue les fixtures du réducteur, l'unique comparaison comportementale `morsehgp3d.point_hierarchy_sklearn_differential` sur neuf points et les preuves de la source.
- Le [rapport de performances](docs/PERFORMANCE_MORSEHGP3D.md) donne les mesures historiques avec leur provenance et leur périmètre exact, puis le protocole qui devra qualifier 50 000, 1 000 000, 10 000 001 et 30 000 000 points.

À ce jour, la tentative HGP de référence à 50 000 points est censurée après au moins 300,000014 s sans hiérarchie complète. Les mesures à 10 M et 30 M concernent seulement une frontière partielle de composant. Le p95 historique de 95,791070 ms appartient à un point-MST rejeté et archivé; ce n'est pas une mesure de MorseHGP3D.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
python tools/check_docs.py
python tools/check_implementation_status.py
```

## Lire le dépôt

1. Les Parties I et II du [manuscrit](docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134, définissent l'objet HGP source.
2. La [spécification](docs/SPECIFICATION_MORSEHGP3D.md) fixe les profils et statuts publics.
3. La [hiérarchie de points multi-ordres](docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md) fixe l'API aval exacte-relative.
4. Le [registre des preuves](docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), la [roadmap](docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md), le [plan de tests](docs/TEST_PLAN_MORSEHGP3D.md) et l'[état des phases](docs/implementation_status.toml) portent l'autorité opérationnelle.
5. L'[index documentaire](docs/README.md) relie les contrats, preuves, validations et archives.

## Archives et sécurité GCP

Les voies falsifiées sont recensées dans [`docs/archive/abandoned/`](docs/archive/abandoned/README.md). Le point-MST surrogate est isolé sous [`morsehgp3d/archive/surrogates/point_mst_v6/`](morsehgp3d/archive/surrogates/point_mst_v6/README.md) et les prototypes non livrés sous [`morsehgp3d/archive/obsolete/`](morsehgp3d/archive/obsolete/phase15_prototypes/README.md); rien de ces répertoires n'entre dans le build, l'installation ou l'API publics.

Toute session GPU passe par les scripts gardés de [`gcp-migration/`](gcp-migration/README.md), sur une G4 `SPOT` avec deux coupe-circuits, puis se termine par la certification `TERMINATED` de la cible exacte. Les règles normatives sont dans [`AGENTS.md`](AGENTS.md).

## Licences

La licence MIT couvre le code actif et la documentation du projet. Elle ne relicencie ni [`HGP-old/`](HGP-old/), qui conserve sa licence historique non commerciale, ni les PDF de [`docs/references/`](docs/references/), dont les conditions sont documentées fichier par fichier.
