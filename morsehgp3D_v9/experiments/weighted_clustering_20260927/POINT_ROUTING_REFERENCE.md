# Référence exacte de routage ponctuel

27 septembre 2026. Implémentation séparée du
[plan de partitions emboîtées](PLAN_PARTITIONS_POINTS.md), sans modification
des benchmarks ou du moteur. Elle construit des partitions de **points**,
à K et z fixés ; ce n'est ni EOM ni un nouvel algorithme géométrique FULL.

## API et conventions

Dans [point_routing_reference.py](point_routing_reference.py) :

```python
result = route_points(n_points, facets, scores, children,
                      squared_levels, roots, leaf_birth_betas)
blocks = cut(result, beta, closed=True)
```

`facets` contient les K PointId de chaque facette, sans doublon. Les feuilles
ont les IDs `0..F-1` ; `children` et `squared_levels` décrivent les autres
nœuds de l'arbre **augmenté des attaches datées**, avec des clés entières
strictes. Les IDs internes peuvent être discontinus. Les racines sont
celles de la vraie forêt, sans fusion ajoutée entre elles. Un parent unique,
l'absence de cycles et la monotonie des niveaux sont contrôlés.

`scores` contient des `Fraction` ou entiers strictement positifs : les scores
Sτ, **pas les masses de facettes**. Floats et booléens sont refusés.
Les niveaux et naissances sont des rayons carrés β exacts, acceptés comme
`Fraction`, entiers ou dictionnaires `{num, den}` décimaux canoniques.
`leaf_birth_betas` fournit la vraie naissance MEB, jamais le zéro virtuel
de l'adaptateur EOM. Le résultat reste un objet Python avec Fraction,
pas un nouveau format JSON.

Le vote descendant est calculé une fois, indépendamment d'une coupe ou
d'un seuil de condensation. Une racine gagnante est choisie par vote
positif maximal ; une égalité entre racines laisse le point extérieur.
Dans cette racine, le chemin reste dans l'enfant gagnant. Une égalité
interne arrête le point au parent, à sa date ; atteindre une facette
l'attache à sa naissance MEB. Les plateaux internes sont atomisés avant
les choix. Le résultat conserve n attaches, leurs dates/raisons, l'index
commun et les compteurs de travail sparse.

`cut` renvoie une partition totale des n PointId. Un point extérieur ou
pas encore attaché reste son **propre singleton**. Les événements sont
admis par `≤ beta` en coupe fermée, `< beta` en coupe stricte. En augmentant
β, seules des unions sont possibles : chaque attache a un parent unique
et suit sa chaîne d'ancêtres. En augmentant λ, le sens s'inverse.
Cette propriété concerne un arbre K,z fixé, pas les sélections EOM selon m
ni un changement de K ou z. Les dates projetées peuvent être retardées par
un arrêt sur égalité ; elles ne sont pas une nouvelle première couverture.

## Coûts et portée

Soient V les nœuds source, I=KF les incidences et d_x les facettes incidentes
au point x. L'index DFS/LCA et les parcours sont itératifs. Chaque point
utilise les seules racines incidentes, puis l'arbre virtuel de ses facettes
dans la racine choisie, avec au plus `2d_x−1` nœuds. Il n'y a pas de tableau
n×V ni de remontée indépendante de chaque incidence sur toute la profondeur.

À K fixé, le coût est
`O(V log V + I log V + Σ d_x log d_x + n)` opérations d'index/arithmétique,
avec `O(V log V + I + n)` stockage. Si K varie, compter aussi le tri des IDs
pour valider l'unicité des facettes : `O(I log K)`. Une coupe utilise
`O(n log V)` sauts, puis présente les blocs en ordre canonique.
Les numérateurs/dénominateurs Fraction ne sont **pas** de taille constante :
ces bornes ne sont ni des coûts binaires constants ni une borne sur F ou V
en fonction du nombre de points. Aucun gain industriel n'est mesuré.

La géométrie, les attaches et les scores sont fournis par des producteurs
distincts. Le module vérifie leur structure, pas la complétude du catalogue
ni leurs MEB. Les intégrations utilisent le producteur FULL déjà qualifié
et un oracle Čech indépendant sur petits nuages. Aucun EOM, score de qualité,
GPU/GCP, nouveau calcul natif ou benchmark comparatif n'est exécuté ici.

## Qualification close

[test_point_routing_reference.py](test_point_routing_reference.py) et son
oracle dense passent en normal et `-O` : **133 fixtures, 2 160 coupes,
14 refus, 3 intégrations qualifiées**. Chaque mode donne ces mêmes comptes,
ils ne sont pas additionnés. Couverture : exclusivité/emboîtement,
plateaux, égalités, permutations, multifusions, forêt, points extérieurs,
chaîne de profondeur 5 000 et 120 arbres aléatoires déterministes.

Les intégrations sont E5 K2, carré K2 et carré K1, avec scores exacts z2.
La naissance silencieuse AC à `33/2` est vérifiée même si cette facette
n'est pas choisie par un point ; le carré K2 s'attache par égalité à β=2 ;
K1 conserve les naissances ponctuelles zéro et la fusion du carré à β=1.

Reçu privé :
`/tmp/mhgp9-point-routing-qualification-20260927-i6Tfcm/receipt.json`,
SHA256 `75f41d14a1f271dd3f4cf5712c422ff8d7c55ba584412612636d40217a5137a4`.
Deux commandes code0, stdout identiques, stderr vides, 668 pins avant/après
identiques. Les dépendances géométriques héritées figurent dans ces pins,
sans avoir été recompilées ou exécutées nativement.

Sources gelées : `point_routing_reference.py`
SHA256 `41c16627b7e69736301063607289886d727aeed7ff45b598ca75b1fecd9af88d` ;
test SHA256 `aaecb225bfc8e24fc3093a0cf0ccd18f1e1bb1b7d6a5bfbc219db3758c714d08`.
Rejeu depuis la racine du worktree :

```sh
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/test_point_routing_reference.py --qualified-fixtures /workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1
python3 -B -O morsehgp3D_v9/experiments/weighted_clustering_20260927/test_point_routing_reference.py --qualified-fixtures /workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1
```
