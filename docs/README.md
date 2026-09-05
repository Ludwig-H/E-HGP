# Documentation MorseHGP3D

L'entrée courante est le [chantier MorseHGP3D v7](../morsehgp3D_v7/README.md) et sa [passation](../morsehgp3D_v7/PASSATION.md). La documentation conserve aussi les lignes historiques : source géométrique HGP, projection laminaire de points et rendus plats. Aucune couche aval ne peut promouvoir une source incomplète ou surrogate.

> [!IMPORTANT]
> Chantier courant au 5 septembre 2026 : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucun contrat de performance FULL à 50 k ni de capacité massive n'est atteint ou revendiqué. Les états Phase 15/v4 ci-dessous sont historiques, sans transfert automatique à la v7.

## Priorité v7 : certificat FULL et performance

La [note sur les niveaux et le certificat FULL](../morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) distingue les minima Gabriel, isolés inclus, les vraies multifusions et leurs rattachements certifiés. Le certificat structurel ne prouve pas à lui seul la complétude géométrique, les parents ni les ancres verticales. Cette cible est distincte des sorties historiques `verified_events_only` et réduites ; elle ne demande pas de matérialiser Gamma exhaustif.

Le [contrat de performance v7](../morsehgp3D_v7/docs/CONTRAT_PERFORMANCE.md) fixe l'ordre mono-thread, multi-CPU, puis GPU : 50 000 points, **toute la tour 1..10 sous une seconde**, avec **toute la tour 1..5** comme repli si nécessaire ; **100 ms ensuite**, à périmètre et exactitude déclarés constants. Comparer WSPD s=8,10,12 sur les mêmes entrées, sans confondre s et `smax`. Les nuages de plusieurs dizaines de millions de points sur G4 constituent des paliers distincts, sous sessions `SPOT` gardées et arrêt ciblé certifié.

La [session G4 v7 du 4 septembre](../morsehgp3D_v7/docs/RESULTATS_G4_20260904.md) et son [reçu](../morsehgp3D_v7/receipts/gcp_requalified_20260904/published/receipt.json) ne livrent pas le contrat FULL 50 k : les tours achevées y portent `verified_events_only`, tandis que la complétion candidate 50 k refuse une dégénérescence. Les observations [mono v7 locales](../morsehgp3D_v7/docs/RESULTATS_MONO_F_20260905.md) restent elles aussi attachées à leur objet et à leur binaire. Aucun de ces temps n'est une mesure du nouveau certificat FULL.

## Parcours conseillé

Le [contrat du composant FULL livré](../morsehgp3D_v7/docs/CONTRAT_CERTIFICAT_FULL.md)
et ses [reçus dédiés](../morsehgp3D_v7/receipts/full_certificate_20260905/README.md)
précisent la première surface C++ qualifiée : structure uniquement, sans
constructeur géométrique ni activation dans la CLI.

1. Lire les Parties I et II du [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134, puis la [spécification](SPECIFICATION_MORSEHGP3D.md).
2. Pour la source, lire la [définition HGP 3D](math/DEFINITION_HGP_3D.md), le [catalogue exact des paires diamétrales](math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md), la [frontière des supports trois et quatre](math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md), l'[audit RNG--Jung et niveaux peu profonds](math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md), les [incidences silencieuses](math/INCIDENCES_SILENCIEUSES_GAMMA.md) et les [attaches par miniball](math/ATTACHES_DESCENTE_MINIBALL.md).
3. Pour le chantier courant, lire la [note FULL v7](../morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) et la [passation](../morsehgp3D_v7/PASSATION.md). Pour la projection de points de la ligne historique, lire la [hiérarchie laminaire multi-ordres](math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md), puis son [contrat C++ public](../morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp).
4. Pour la qualification, lire le [plan de tests](TEST_PLAN_MORSEHGP3D.md) et le [rapport de performances et déploiement](PERFORMANCE_MORSEHGP3D.md).
5. Vérifier enfin le [registre des preuves](math/STATUT_PREUVES_ET_HEURISTIQUES.md), la [roadmap](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md) et le [registre des phases](implementation_status.toml).

## Contrat historique de la projection de points

La Phase 15 documentait `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. La réduction implémentée porte le mode `exact_relative_multi_order_laminar_point_projection_v1`. Ce contrat et ses tests ne qualifient pas automatiquement la nouvelle source FULL v7 ni son supplément pondéré.

La tour de $T_1$ à $T_K$, déclarée complète et exacte sous une autorité amont externe, est fusionnée selon les niveaux exacts $k/r^z$, où `exp_z` est un rationnel positif. Les contributions des simplexes sont distribuées vers leurs points par poids de rayon inverse ou uniformes, puis par poids rationnels entre ordres. Un routage descendant irréversible, avec canal `stay`, donne un terminal unique à chaque point. Il impose la laminarité et exclut tout double étiquetage à chaque niveau.

Les trois rendus publics sont :

- une coupe de lambda exacte;
- une coupe de rayon carré de type DBSCAN avec ordre de référence explicite;
- une condensation suivie d'une sélection `excess_of_mass` de type HDBSCAN.

Il n'existe pas d'argument `splitting` dans le cœur. `morsehgp3d.point_hierarchy_sklearn_differential` est l'unique différentiel de cette API : il compare DBSCAN et HDBSCAN-EOM sur une fixture multi-ordres $K=2$ de neuf points, avec ordre de référence et `min_samples` tous deux égaux à 2, et seulement les partitions à renommage de labels près. Il ne certifie ni la tour HGP, ni l'exactitude scientifique amont, ni la capacité produit.

## Statuts historiques et non-promesses

| objet historique | statut de la ligne concernée | ce qu'il ne prouve pas |
|---|---|---|
| réducteur public de la tour vers les points | exact relativement au payload lié et aux déclarations amont; test hôte dédié | authentification de la vérité géométrique de la source ou statut public `exact` |
| source sparse amont | Phase 15, non qualifiée | tour complète pour un nuage arbitraire |
| ancienne tentative à 50 000 points | censurée après au moins 300,000014 s | temps de hiérarchie complète, mesure v7 actuelle ou SLO |
| profils 10 M et 30 M | composants partiels censurés | forêt, clustering, mémoire ou temps end-to-end |
| point-MST historique sous 100 ms | surrogate rejeté | résultat ou performance MorseHGP3D |

Les chiffres, champs de provenance et anciennes portes séquentielles 50 000, 1 000 000, 10 000 001 puis 30 000 000 sont conservés sans extrapolation dans [PERFORMANCE_MORSEHGP3D.md](PERFORMANCE_MORSEHGP3D.md), séparément de l'entrée v7 actuelle. La censure de 300 s et le point-MST sous 100 ms ne sont pas des mesures courantes de la v7.

## Carte documentaire

| ensemble | rôle | autorité produit |
|---|---|---|
| [`math/`](math/README.md) | définitions, théorèmes, contre-exemples et obligations | oui, selon le statut de chaque énoncé |
| [`contracts/`](contracts/README.md) | schémas sérialisés et matrices de traçabilité | oui pour leur version déclarée |
| [`research/`](research/README.md) | replis exacts ou oracles maintenus et bornés | non par défaut |
| [`reference/`](../reference/README.md) | vérité terrain indépendante de petite taille | oracle seulement |
| [`validation/`](validation/README.md) | revues, reçus, JSON et transcripts | preuve d'exécution, jamais architecture |
| [`archive/`](archive/README.md) | décisions remplacées et expériences scellées | non |

Le point-MST rejeté est isolé sous [`morsehgp3d/archive/surrogates/point_mst_v6/`](../morsehgp3d/archive/surrogates/point_mst_v6/README.md). Les prototypes sans livraison sont sous [`morsehgp3d/archive/obsolete/phase15_prototypes/`](../morsehgp3d/archive/obsolete/phase15_prototypes/README.md). Les motifs d'abandon sont indexés dans [`archive/abandoned/`](archive/abandoned/README.md).

## Invariants communs

- aucune mosaïque de Delaunay d'ordre supérieur, population globale de cellules, cofaces ou incidences dans le chemin produit;
- proposition flottante, décision certifiée, réduction hiérarchique et statut public toujours distincts;
- tout cap produit un échec fermé, jamais une absence déclarée exacte;
- les oracles exhaustifs restent petits et indépendants du producteur;
- une mesure de composant ou de surrogate n'est jamais publiée comme performance du produit;
- toute session GCP utilise une G4 `SPOT`, deux coupe-circuits et un arrêt ciblé certifié `TERMINATED`.
