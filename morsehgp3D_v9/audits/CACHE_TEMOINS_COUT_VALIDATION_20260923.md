# Cache de témoins q3/q4 : coût du contrôle d'antichaîne

23 septembre 2026. Code publié depuis `a1d7a9bc`, reçu CPU G4 R5.
Le cache réévalue exactement, pour la paire suivante, les nœuds témoins
tracés par la paire précédente de même ancre. Le chemin interne forme
une antichaîne par voie. L'API publique accepte aussi un span fourni par
l'appelant : sa vérification de disjonction avant tout crédit est
nécessaire, car deux copies d'un seul nœud témoin pourraient faussement
satisfaire le seuil q3/q4. Ce défaut a été corrigé dans `a1d7a9bc`.

La validation du span coûte cependant `m(m−1)/2` tours de boucle **à
chaque** requête cache, avant les tests géométriques. Une trace contient
au plus `2K−3` nœuds. Si `Q` est le nombre de requêtes et `T` celui de
leurs tests géométriques, le nombre de tours vaut au moins

`Q·C(floor(T/Q),2) + (T mod Q)·floor(T/Q)`.

En effet, chaque requête teste au plus `m` nœuds et la somme des
`C(m,2)` est minimale quand les longueurs sont aussi égales que
possible. Les six premières répétitions du [reçu R5](../receipts/g4_tower_r5_20260923/README.md)
comptent `Q=123,228 M` et `T=554,501 M` : **au moins 1,090 milliard**
de tours de validation, non inclus dans `node_tests`. Même en répartissant
les nœuds testés au mieux entre les deux voies, au moins **424,330
millions** de couples partagent une voie et déclenchent la comparaison
d'intervalles. Ce sont des minorants combinatoires de travail, **pas**
un temps mesuré ni une preuve de coût dominant.

Mesurer `validation_pair_iterations`, `interval_compares` et les tailles
de traces, puis ablater le cache à mêmes trame/K/s/W et sortie FULL.
Une voie interne pourrait porter un ticket opaque immuable, fabriqué
par une recherche tracée ou une factory validante, lié à l'index et à
son propriétaire. Elle réutiliserait la preuve d'antichaîne tout en
retestant exactement la géométrie de chaque nouvelle paire ; l'API
publique continuerait à refuser doublons, ancêtres et indices invalides.
Comparer ticket, API publique et oracle sur mêmes et autres paires.
Cette optimisation retire du contrôle par paire ; elle ne réduit pas
l'expansion des produits ni les covers/formes.
