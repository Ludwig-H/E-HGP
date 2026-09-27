# Dendrogramme explicite de points

27 septembre 2026. Expérience distincte, `public_status=not_claimed`.
Le module matérialise exactement les partitions du
[routage Fraction figé](../weighted_clustering_20260927/POINT_ROUTING_REFERENCE.md),
à K et z fixés. Aucun changement de moteur, de géométrie, de vote ou de
source/capture antérieure ; pas de construction native ni de GCP.
Ce n'est ni le squelette FULL natif, ni toutes ses composantes géométriques,
ni une restitution implicite du clusterer historique.

## Interface

```python
from point_tree import build_point_tree, validate_point_tree, cut, to_jsonable

tree = build_point_tree(routing)  # objet Python renvoyé par route_points
validate_point_tree(tree)        # None ; ValueError si invariant invalide
blocks = cut(tree, beta, closed=True)
wire = to_jsonable(tree)         # json.dumps(wire, sort_keys=True)
```

Sortie `mhgp9_exact_point_dendrogram_v1` :

- `n_points`, `n_leaves` : n ; les feuilles sont exactement les PointId
  `0..n−1`, avec `leaf_birth_betas = [Fraction(0)] * n`.
- `children`, `squared_levels` : nœuds internes `n..n+M−1`, multifusions
  d'au moins deux enfants, β rationnel exact. Les IDs sont ordonnés par
  `(β, minimum PointId descendant)` ; enfants et racines par minimum PointId.
- `parents`, `roots`, `point_counts` : forêt explicite et cardinalités
  entières des points, **pas les masses fractionnaires des facettes**.
- `attachments` : une ligne par point, conservant `source_node`, `beta`,
  `source_root`, `reason`. Ces IDs source sont dans un autre espace que
  les IDs de l'arbre ponctuel. Aucun revote n'est effectué.
- `source_to_point_node` : représentant ponctuel de la classe source
  fermée au plateau ; `None` si elle est vide, descendant conservé si
  elle est unaire. Ce n'est pas une copie des descendants source bruts.
- `merge_source_nodes` : sources d'un plateau ayant produit une fusion
  ponctuelle ; les sources supprimées restent dans la table précédente.
- `statistics` : volumes source, classes de plateau vides/unaires,
  feuilles, multifusions, arêtes, racines et points extérieurs.

Les `Fraction` restent l'autorité. `to_jsonable` encode les rationnels
comme `{"num":"…","den":"…"}` et les clés entières comme chaînes JSON.
C'est un encodeur sans perte, **pas** un lecteur de JSON édité : le
validateur attend l'objet Python à clés entières. La sérialisation ne
recalcule aucune géométrie ni aucun vote.

## Dates et preuve des coupes

Chaque point existe comme singleton dès β=0, même si son attache est plus
tardive. Seules les fusions utilisent `≤ β` (fermé) ou `< β` (strict).
À la coupe stricte zéro, toutes les feuilles restent présentes ; une
multifusion à zéro n'est admise qu'en coupe fermée zéro. Une feuille point
à zéro peut donc avoir un parent interne à zéro : ne pas la supprimer.

Les points `uncovered` ou `root_tie` restent des racines singleton
indépendantes, sans racine artificielle ni fusion à l'infini. Une forêt
peut avoir plusieurs racines ; n=0 produit la forêt vide. Supprimer ces
points de la représentation rendrait la partition partielle.

La construction part de la forêt source avec les feuilles-points attachées
à leur date terminale. Elle contracte d'abord toutes les arêtes source de
même β, y compris facette–parent : à une coupe stricte elles sont toutes
absentes, à une coupe fermée toutes présentes. Puis un postordre itératif
supprime les classes sans point et transmet le seul enfant des classes
unaires. Aucune de ces deux opérations ne change une partition de points.
Chaque classe ayant au moins deux enfants ponctuels produit une fusion
à sa date source. Les événements restants sont strictement croissants
entre nœuds internes. Ainsi les coupes sont celles de `cut(routing, β)`,
et leur emboîtement résulte du parent unique, sans regroupement anticipé
de résidus. Les dates d'attache peuvent être retardées par les égalités
de vote ; elles ne deviennent pas des dates de première couverture.

Le validateur contrôle topologie, atomisation, numérotation, comptes et
cohérence de provenance. Des intervalles DFS vérifient qu'une source
représentée contient effectivement son point. Il ne rejuge pas les votes,
les MEB, la complétude Gabriel ou les attestations géométriques amont.
Les caches LCA du routage ne sont ni consultés ni recopiés.

## Coûts et limites

Pour V nœuds source atomisés par le routage et n points, le quotient et
le postordre visitent chacun O(V+n) éléments. Les tris de présentation
donnent O((V+n) log(V+n)) opérations et O(V+n) stockage supplémentaire.
Aucune table n×V, aucun ensemble de descendants par nœud, aucune récursion
de profondeur V. La sortie contient au plus n−R multifusions pour R
racines, donc moins de 2n nœuds si n>0 ; sa provenance peut rester O(V).
Une coupe parcourt O(n) nœuds, avec O(n log n) pour trier ses blocs.
Le validateur effectue O(V+n) opérations. Ces bornes comptent les opérations
rationnelles, pas leur coût binaire ni le volume géométrique V en fonction
de n. Le routage sparse amont et son index LCA restent des coûts séparés.

Condensation et EOM sont des consommateurs séparés. Cette forêt ne
garantit ni domination de HDBSCAN, ni qualité statistique, ni vitesse de
production. Un raccord binary64 doit déclarer et contrôler la conversion
des dates : β=1 et β=1+2⁻⁸⁰ restent distincts ici, mais pas en `float`.
Les forêts/n=0 ne doivent jamais être raccordés à un consommateur
mono-racine au moyen d'une racine artificielle silencieuse.

## Tests légers

`test_point_tree.py` passe en normal et `-O` : **181 fixtures, 3 124
coupes strictes/fermées, 27 refus, trois intégrations FULL qualifiées**.
Chaque mode donne ces mêmes comptes, ils ne sont pas additionnés.
Couverture : extérieurs, forêt vide et multiracine, attaches tardives,
date zéro, plateaux facette–parent, multifusions, permutations des IDs
source et des points, 160 arbres aléatoires déterministes, profondeur
5 000, peigne ponctuel de 2 400 points, multifusion de 3 000 points,
3 000 racines extérieures distinctes, dates rationnelles très proches,
immutabilité et sérialisation stable.

E5 K2, carré K2 et carré K1 sont lus depuis les sorties natives **déjà
qualifiées** et épinglées. Les helpers et sorties sont hachés avant/après.
L'arbre ponctuel intrinsèque complet est aussi identique à celui produit
depuis l'oracle Čech indépendant. E5 conserve en amont la naissance
silencieuse AC à 33/2 ; carré K2 fusionne à β=2, carré K1 à β=1.

```sh
python3 -B morsehgp3D_v9/experiments/point_dendrogram_20260927/test_point_tree.py --qualified-fixtures /workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1
python3 -B -O morsehgp3D_v9/experiments/point_dendrogram_20260927/test_point_tree.py --qualified-fixtures /workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1
```

Ces tests nécessitent les petites sorties privées indiquées ; ne pas
présenter le document seul comme une archive autonome. Les éventuels
reçus de qualification du raccord EOM sont gérés séparément.
