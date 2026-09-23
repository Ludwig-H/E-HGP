# S1 GPU : certifier le domaine numérique et les boîtes du filtre

23 septembre 2026. Relecture du commit publié `0d5ad2e89`, sans exécution
CUDA. Le lanceur S1 est une sonde du filtre témoin q3/q4, pas une chaîne HGP
sur GPU. Son `run_filters` vérifie maintenant, avant le device, K∈[3,10],
les pointeurs, les plages de rangs, les liens d'enfants, les IDs et masques
des rectangles ainsi que la limite `int` du scan CUB. Il répond donc à une
grande partie du préflight de B. La sonde normale fabrique un index u18
certifié et compare chaque masque device à sa référence CPU ; aucun reçu
G4 positif n'accompagne encore ce commit.

**Domaine oublié par la garde.** `validate` dans
[`filter_runner.cu`](../src/gpu/filter_runner.cu) ne lit ni les coordonnées `rank_points` ni les
bornes des `FlatNode`. Une entrée de trois nœuds valide pour ses contrôles
structurels suffit : racine `[0,2)` avec deux feuilles `[0,1)` et `[1,2)`,
un rectangle entre ces feuilles, `K=3`, masque `6`, et coordonnées x des
deux points `INT32_MIN` et `INT32_MAX` (y=z=0), avec boîtes feuilles
correspondantes. Elle passe `validate`. Puis `filter<true>` appelle
`prepare_pair` dans [`witness_filter.hpp`](../src/gpu/witness_filter.hpp) : sa différence x vaut
`2^32−1`, et `difference*difference` est évalué en entier signé 64 bits.
Le résultat dépasse `2^63−1` et provoque un débordement signé. Un appel
hôte direct à la primitive publiée, compilé avec Clang
`-fsanitize=signed-integer-overflow -fno-sanitize-recover=all`, a refusé à
`witness_filter.hpp:75:46` avec `4294967295 * 4294967295 cannot be represented
in type 'i64'`. Les deux fichiers source du worktree testés ont les mêmes
SHA-256 que les objets du commit publié : `filter_runner.cu`
`e4a8f8e722b7373919d373f6089974ea098c07f5e16fc1c66f6843aef1b6f1ba`,
`witness_filter.hpp`
`5d011aad968f5c64bca8ff0703fb021b5d368c93ed3a90d187802e1c109f820c`.

**Une simple borne u18 ne suffit pas.** Même avec deux points licites
`a=(0,0,0)` et `b=(2,0,0)`, trois nœuds (racine `[0,2)`, deux feuilles
singleton), le rectangle des feuilles, `K=3` et masque q4 `4`, la garde
accepte une boîte de racine forgée `x=[1,1]`. Les autres boîtes et les
plages restent correctes et disjointes. Le filtre hôte publié rend
`4` avec la boîte certifiée `x=[0,2]` (3 visites), mais **`0`** avec
la boîte forgée (1 visite) : il crédite alors à tort les deux points
sur la base d'une boîte prétendument entièrement intérieure, bien que
les deux points soient sur la coquille de la boule diamétrale. C'est
un **faux rejet q4**, donc un risque de complétude pour un appelant de
`FilterInput` brut. La sonde courante construit normalement les nœuds
avec `flatten_nodes(*index)` depuis l'index certifié du moteur ; ce
contre-exemple ne lui attribue pas de sortie LiDAR fausse.

Correction numérique : au même préflight O(n+R), exiger pour chaque coordonnée
de point et chaque borne de boîte le domaine entier u18 déclaré
`0..262143`, et `low≤high` par axe ; tester les deux extrêmes autorisés,
`-1`, `262144` et les deux `INT32_*`, avec refus **avant** tout appel CUDA.
Correction géométrique : transporter un **index certifié immuable** avec
la requête, ou vérifier une fois que les feuilles couvrent exactement
leurs rangs et points, que les enfants partitionnent la plage du parent
et que chaque boîte est l'enveloppe des points de sa plage. Une telle
certification peut être réutilisée entre lots ; une vérification complète
par appel annulerait une partie du gain du filtre GPU. Ajouter la
fixture `masque 4→0` à la porte d'entrée brute, en plus des refus de
coordonnées hors u18.
Si l'API veut un jour accepter d'autres coordonnées, elle devra plutôt
élargir les bornes arithmétiques et le contrat du filtre, puis refaire les
preuves et tests. Les produits `3*rank_count` et `count*sizeof(T)` ne
débordent pas en `size_t` 64 bits avec les plafonds actuels et les tailles
de types publiées ; ils ne constituent pas le défaut constaté ici.

Ce cas ne remet pas en cause les masques de la sonde pour les entrées u18
certifiées, mais empêche de traiter `run_filters` comme une frontière
numérique autonome. La recherche binaire par paire et la matérialisation
O(P) sont déjà relevées dans la [contrelecture B](CONTRE_AUDIT_B_PORTE_FILTRE_GPU_20260923.md)
et exigent une ablation séparée ; elles ne sont pas des erreurs du masque.
