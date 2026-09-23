# S1 GPU : domaine u18 et boîtes, correction et coût de certification

**Statut au commit publié `7565451fc` : les deux contre-exemples ci-dessous
sont refusés par la nouvelle garde hôte.** Le code et les fixtures du
snapshot initial `0d5ad2e89` restent ici pour expliquer la frontière
géométrique ; ils ne décrivent plus un défaut ouvert de `run_filters`.
Le protocole G4 et la sonde v2 sont publiés, mais aucun résultat CUDA/G4
positif ni chrono de tour GPU n'en découle.

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

## Relecture de la correction publiée

`validate_filter_input` dans `filter_runner.hpp` exige désormais le domaine
u18 des points et des boîtes, la couverture de tous les rangs par la racine,
la partition exacte des plages des enfants et l'inclusion de chaque point
de la plage dans la boîte du nœud. La porte hôte publiée refuse la boîte
forgée en vérifiant que la primitive non gardée ferait bien `4→0`, refuse
les coordonnées `−1`, `262144` et les extrêmes `int32`, et accepte les
deux bornes u18. C'est une fermeture causale des deux cas de cet audit.
La garde visite aussi les nœuds orphelins : leurs boîtes sont certifiées
même si un rectangle brut les référence. Son temps n'est pas inclus dans
les événements CUDA de `gpu.total_ms`.

**Coût à éviter lors du passage en flux borné.** La garde parcourt
actuellement tous les rangs de **chaque** nœud ; elle paie
`Σ_N |range(N)| = O(n·profondeur)` sur un arbre équilibré. S1 l'appelle
une fois par exécution, hors des répétitions chaudes. Une S2 qui appellerait
`run_filters` pour chaque tuile repaierait ce coût à chaque lot.
Une certification suffisante en `O(n + nombre_de_nœuds)` est possible :
faire de la racine `[0,n)` un arbre atteignable sans nœud orphelin ni
second parent, vérifier la partition des rangs, vérifier chaque point
**une seule fois dans sa feuille**, puis vérifier que la boîte de chaque
parent contient celles de ses deux enfants. Par induction, chaque boîte
contient alors tous les points de sa plage. Cette règle admet le producteur
normal, dont les boîtes sont les enveloppes exactes ; elle peut refuser
une entrée brute pourtant sûre avec boîtes larges non emboîtées. Si cette
généralité est requise, agréger plutôt les vraies enveloppes des feuilles
de bas en haut avant de comparer chaque boîte, toujours en temps linéaire.
Pour S2, porter cette preuve dans un objet d'index immuable **certifié une
fois**, réutilisé par les lots, évite de refaire même la vérification
linéaire et conserve le refus des entrées brutes malformées à leur création.
Comparer le coût de cette certification au mur complet du probe et à
`gpu.total_ms` séparément, sans l'imputer au noyau.
