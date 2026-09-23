# q3/q4 — conservation de l’arête propriétaire avant l’atlas

23 septembre 2026. Revue statique du moteur publié jusqu’à `4530644b` : ce
commit ne change pas `src/gen`. Cette note ferme l’obligation **amont** laissée
distincte dans [l’induction q4](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md) :
une boule q3 ou q4 de support minimal positif et de profondeur admissible
conserve la voie de son arête propriétaire jusqu’au traitement local. Il
s’agit d’une preuve conditionnelle de programme, sous les invariants de
l’index immuable et des prédicats entiers ; elle ne qualifie ni le backend
`Window30`, ni l’inventaire q2, ni le catalogue/FULL ou la tour publique.

## Lemme exact derrière les trois filtres de témoins

Soit `e={a,b}` la plus longue arête d’un support, `D=|b−a|²>0`,
`m=(a+b)/2`, et `c=m+s` le centre de sa miniballe. Alors `s⊥(b−a)` et
`R²=D/4+|s|²`. Les poids barycentriques positifs et la maximalité de
`e` donnent, par l’identité de variance, `R²≤D/3` en q3 et
`R²≤3D/8` en q4. Donc `|s|²≤D/12` ou `D/8`, respectivement.

Pour tout site `z`, posons `H=(z−a)·(b−z)` et
`Ξ=|(z−a)×(b−z)|²`. Puisque
`H=D/4−|z−m|²` et `Ξ=D|proj_{(b−a)⊥}(z−m)|²`, sa puissance par rapport
à **cette** boule satisfait

```text
power(z) = −H−2(z−m)·s
         ≤ −H+sqrt(Ξ/3)   (q3),
         ≤ −H+sqrt(Ξ/2)   (q4).
```

Ainsi `H>0` et `3H²>Ξ` en q3, ou `2H²>Ξ` en q4, certifient un intérieur
**strict**, jamais un simple contact. Les bornes sur boîtes et nœuds de
[`spindle/predicates.hpp`](../src/gen/spindle/predicates.hpp),
[`q34_pair_bounds.hpp`](../src/gen/lanes/q34_pair_bounds.hpp) et
[`q34_witness_search.cpp`](../src/gen/lanes/q34_witness_search.cpp) ne
créditent que si la version conservatrice de cette inégalité est vraie
pour **tous** les points des boîtes concernées. Les égalités ne créditent
rien. Le crédit de chaque lane concerne donc des sites strictement
intérieurs à toute miniballe positive possédée par `e`.

## Un unique produit WSPD, aucune voie perdue

Dans [`front.cpp:106–169`](../src/gen/wspd/front.cpp), le produit diagonal
`X×X` est remplacé par `L×L`, `L×R`, `R×R`, qui partitionnent les paires
non ordonnées. Un produit disjoint est soit émis, soit partagé selon un
facteur en deux produits disjoints. L’index certifié porte des sites
géométriquement distincts ; deux feuilles distinctes ont donc des
boîtes ponctuelles séparées et finissent émises. Par induction, chaque
paire non ordonnée a un unique rectangle terminal **avant ses rejets de
lane**. La file de tâches partage ensuite les rangs `A` en intervalles
disjoints ; un intervalle refusé par la file est développé sur place
([`wspd_q34.cpp:457–506`](../src/gen/pipeline/wspd_q34.cpp)). Sur un
appel réussi, aucune arête résiduelle n’est perdue par le parallélisme.

Le `MidpointSamples` du front choisit des rangs distincts hors `A∪B` et
les teste avec `H_min>0` et `αH_min²>Ξ_max`, `α=3` en q3 et `α=2` en q4
([`front.cpp:214–356`](../src/gen/wspd/front.cpp)). Les options
d’héritage des crédits du front sont réservées à q2, non exposées par le
générateur q3/q4. Au niveau rectangle puis paire, la recherche parcourt
des nœuds spatiaux disjoints ; l’admission borne encore `H` par-dessous
et `Ξ` par-dessus, et l’exclusion éventuelle ne fait que **retirer des
possibilités de crédit** ([`q34_witness_search.cpp:48–221`](../src/gen/lanes/q34_witness_search.cpp)). Le cache pair revalide l’inégalité
sur la nouvelle arête et l’antichaîne par voie avant d’additionner les
populations de nœuds ; si une voie reste ouverte, la recherche normale
reprend sans lui ajouter les crédits du cache
([`q34_witness_search.cpp:225–281`](../src/gen/lanes/q34_witness_search.cpp),
[`wspd_q34.cpp:517–547`](../src/gen/pipeline/wspd_q34.cpp)).

Pour une boule de profondeur `p<K−1` en q3 ou `p<K−2` en q4, aucun de
ces filtres ne peut trouver respectivement `K−1` ou `K−2` sites
**distincts** strictement intérieurs. Sa voie survit donc au rectangle et
à la paire. Les masques q3 et q4 sont indépendants : le rejet de toutes
les faces q3 d’une clé n’éteint pas q4.

## Covers et preuve de voies mortes

La boule et tous ses intrus/contacts sont dans le cover **fermé** de
centre `m` et de rayon `√D` :

```text
R+|s| ≤ (sqrt(3)+1) sqrt(D/8) < sqrt(D)  (q4),
R+|s| ≤ sqrt(3D)/2 < sqrt(D)              (q3).
```

Le constructeur du cover décide les boîtes par bornes fermées et les
feuilles exactement ([`edge_cover.cpp:85–124`](../src/gen/lanes/edge_cover.cpp)).
Le cœur diamétral optionnel est un sous-ensemble de ce cover. Dans
[`q34_dead_lanes.cpp:109–212`](../src/gen/lanes/q34_dead_lanes.cpp),
une voie n’est prouvée morte que si chaque cellule du disque de centres
est dehors, ou possède au moins son seuil de sites uniformément
strictement intérieurs. Une cellule non résolue, un contact ou un budget
de profondeur insuffisant laisse la voie ouverte. Le centre de la
boule admissible appartient à son disque (`3|2s|²≤D` en q3,
`2|2s|²≤D` en q4) et à la racine `[-2,2]²` ; la preuve de voie morte,
sur cœur puis cover complet, contredirait `p` et ne peut la supprimer.

En q3, la recherche de troisième sommet ne rejette un nœud que si
`min|a−x|²>D`, `min|b−x|²>D` ou `max(|a−x|²+|b−x|²)≤D` ; aucune
condition ne tient pour le sommet aigu `x` d’un support dont `ab` est
maximale ([`wspd_q34.cpp:698–761`](../src/gen/pipeline/wspd_q34.cpp)).
Le certificat d’atlas q3 ne rejette ensuite qu’à `K−1` intérieurs
stricts ; l’induction q4 locale traite séparément la graine aiguë et
son événement positif. Ce sont les deux obligations aval à joindre à
la présente preuve, pas des crédits du front transférés à leur census.

Cette conservation est **qualitative**. Elle ne borne ni le nombre de
produits résiduels ni les lectures du cover, des graines et des feuilles.
Les [visites désormais publiées](LEDGER_VISITES_CACHEES_Q34_20260923.md)
et la pente LiDAR appariée restent nécessaires pour juger le coût total.
