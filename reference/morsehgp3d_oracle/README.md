# Oracles exhaustifs CPU de référence

Ce paquet constitue une voie de référence indépendante du backend de production. Il énumère directement tous les sous-ensembles de points nécessaires aux objets de phase 1 et n'importe aucun code CUDA ou MorseHGP3D de production.

Le module `ordinary_diagram` fournit aussi l'oracle différentiel de Phase 8.4. Pour chaque sous-ensemble non vide $Q$ d'au plus huit sites, il construit directement l'intersection ordinaire clippée $K_Q$ par égalités d'équidistance, inégalités nearest, RREF rationnelle et énumération exhaustive des frontières actives. Les singletons donnent les cellules; les autres sous-ensembles donnent les contacts sans consommer les shells ou incidences du producteur. Cette voie valide seulement le domaine borné et ne modifie pas les certificats de phase 1 décrits ci-dessous.

L'atlas ne doit pas être étendu en implémentation générale de Voronoï. Pour une baseline plus large, la politique du dépôt impose d'évaluer d'abord Geogram ou une bibliothèque mature équivalente via un adaptateur épinglé; la baseline reste non autoritative et séparée du chemin GPU tant qu'un rejeu exact ne l'a pas certifiée.

Le chemin produit vise explicitement moins d'une seconde en p95 `warm_e2e` autour de 50 000 points avec $K_{\max}\leq10$, puis dix millions de points ou davantage en streaming transactionnel. Le catalogue exhaustif de ce paquet peut servir d'oracle bidirectionnel à 8.5, uniquement dans les tests.

## Complétude sans cascade de cellules

En dimension 3, tout support minimal d'une miniball contient au plus quatre points. Le catalogue de référence parcourt donc tous les supports de cardinal 1 à 4, rejette exactement les supports affinement dépendants ou non bien centrés, puis classe tous les points par rapport à chaque boule retenue. Ce parcours exhaustif remplace, pour ce backend seulement, la découverte par raffinement de cellules restreintes.

Par conséquent, aucune cellule enfant canonique ni incidence croisée de cellule n'est requise ou fermée par cette stratégie. Le certificat expose honnêtement `canonical_cells_closed=0`; `canonical_children_complete` et `active_cross_incidences_complete` signifient que tous les objets requis par la stratégie choisie sont clos, soit ici deux ensembles requis vides. Ces booléens sont calculés à partir des compteurs `required` et `closed`, et non affirmés indépendamment.

Cette justification ne s'étend pas au backend de production, qui n'est plus fondé sur les cellules. Le flux direct LBVH possède ses propres certificats : partition exacte du self-produit des supports, prunes rejoués, frontières vides par taille et classifications feuilles globales. Les booléens cellulaires de cet oracle ne doivent donc jamais être recyclés pour certifier le chemin produit.
