# Index radix et raccord aux consommateurs

La partition de l'index et des parcours est établie sous les préconditions du chemin produit : positions u16, PointId distincts, n≤2^30−1 gardé avant `build_cloud_index`, ABI CPU i32/u32/u64 déclarée et opérations conformes. L'index accepte des positions répétées avec IDs distincts ; le pipeline les refuse ensuite. `public_status=not_claimed`. Les [sources et exécutions épinglées](receipts_20260905/index_summary.json) identifient la portée de cette preuve, utilisée par la [pile de témoins constructeur](../docs/OPTIMISATION_PILE_TEMOINS.md#3-pourquoi-64-suffit-au-parcours-produit).

## Morton, buckets et trie

L'interlacement des seize bits de chaque axe place le bit b de l'axe a au rang 3b+a. Ces 48 positions sont disjointes : le décodage restitue la position. Chaque étage masque/décalage distribue sur le OU ; contrôler ses seize bits élémentaires et zéro ferme le raccord aux masques écrits. Le tri `(Morton,PointId)` puis les frontières des clés égales produisent des buckets CSR non vides, contigus, disjoints et couvrant exactement les identités. Leur somme télescopique donne les poids de plage ; une permutation physique conserve l'objet.

Pour des clés croissantes k₀,…,kₘ₋₁, noter Dᵢ le préfixe commun des voisins, avec frontières −1. Dans tout intervalle non singleton, le premier bit discriminant sépare une plage de zéros d'une plage de uns : le minimum des Dᵢ est atteint à une unique coupure. Les deux préfixes adjacents à une clé sont donc distincts quand ils sont valides.

L'algorithme choisit le voisin de préfixe le plus long et utilise l'autre comme seuil. Le prédicat « préfixe supérieur au seuil » décrit un bloc contigu : recherches exponentielle puis binaire retrouvent exactement son extrémité. Inversement, pour un nœud du trie de plage [a,b], les préfixes aux deux frontières sont distincts hors racine et inférieurs au préfixe du nœud. La frontière la plus longue désigne l'unique indice reconstructeur : a si elle est à gauche, b sinon. À la racine, les deux frontières valent −1 et l'indice est 0. Les m−1 indices correspondent donc bijectivement aux internes.

À la coupure s de [a,b], les fils non singletons ont précisément les indices s et s+1 ; les feuilles utilisent −1−u. Les plages enfants sont disjointes et partitionnent le parent. La recherche de coupure conserve `split≤s` et `s−split<step` jusqu'à `step=1`. Aucun cycle, doublon de feuille ou interne inaccessible n'est possible. La remontée en ordre inverse de visite calcule par union les boîtes serrées. Le préfixe augmente strictement à chaque arête interne, donc la hauteur est au plus 48. Pour m=1 la racine est la feuille −1 ; pour m=0 elle ne doit pas être déréférencée.

Tous les appels internes à `key_delta` utilisent des indices distincts ; un voisin hors plage retourne −1 avant lecture. Le XOR des clés valides est non nul, donc `clzll` est défini. La garde n≤2^30−1 borne `lmax` par 2^30, `i+lmax*d` entre −2^30 et 2^31−3 et `split+step` par 2m−2<2^31. Casts, références signées, différences et incréments sont représentables. Les poids sont au plus n. L'appel bas niveau direct doit annoncer cette même borne ; elle n'est pas contrôlée par l'index lui-même.

## Partition du front et des covers

Toute paire de feuilles possède un unique plus bas ancêtre commun : elle appartient à l'unique graine `(left,right)` correspondante. Scinder un facteur partitionne son produit cartésien. Par lane, tâches en attente, émissions et morts partitionnent ces graines ; un bit retiré ne passe pas aux descendants. Les shards couvrent des tranches disjointes, fusionnées une fois. Une scission diminue la somme des hauteurs ; deux feuilles distinctes sont séparées. Caps ou exceptions interdisent le succès terminal, auquel la couverture est conditionnée.

Pour les covers, un nœud est rejeté, émis sans descente, ou remplacé par ses enfants. La pile et les plages émises restent disjointes : les handles forment une antichaîne. La boîte des sommes contient tout a+b, et Dmax² majore toute distance d'ancre. Rejeter seulement `gap²>coef*Dmax²` ne perd donc aucun site admissible ; l'égalité reste accessible. Chaque plage conservée est parcourue entièrement et chaque site une fois.

Avec M=65535, distances, bornes des coefficients 3/4 et `bound+1` sont sous 2^36. Pour un site retenu, `32*d2/(bound+1)` est dans 0..31, produit promu en i128. Les préfixes de comptes ≤n attribuent des plages disjointes : le tri par bins est une permutation stable. Une égalité de masses seule ne remplacerait aucun de ces invariants.

## Preuve exécutée et limites

Le [juge C++](index_probe_20260905.cpp) reconstruit le trie par balayage du bit discriminant, sans Karras ni `clzll`, vérifie IDs, CSR, parents, atteignabilité et boîtes. Le [pilote](index_probe_20260905.py) a exécuté O2 et UBSan, tous deux code 0 : 196 608 valeurs Morton axiales, 237 212 nuages, 1 730 634 internes, 1 967 845 feuilles, 171 570 permutations et hauteur 48 atteinte. Sept mutations de résultat sont rejetées en code 3 par build ; elles testent le juge, sans constituer des mutations du produit.

Les octets, codes et planchers restent dans le reçu. `cell_of_prefix` rend le cube ancêtre des triplets complets, en oubliant au plus deux bits : il surcouvre la plage, sans prouver le commentaire de packing. La preuve n'impose aucune structure Delaunay et ne donne aucune borne industrielle du nombre de rectangles. Aucun nouveau test dans cette condensation.
