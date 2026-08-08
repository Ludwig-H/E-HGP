# WARNING 2 — le prototype courant n'est ni complet ni qualifiable à 50 k

> [!CAUTION]
> Second audit indépendant du 8 août 2026, réalisé pendant que `morsehgp3D_v2` était modifié en parallèle. Ce fichier ne modifie aucun fichier de conception ou de code du prototype. Il enregistre trois contre-exemples reproductibles et les obligations qui restent ouvertes. Les corrections apportées après cet instant doivent être rejouées contre ces fixtures avant de rendre cet avertissement caduc.

## 1. Réparations effectivement observées

Le prototype distingue maintenant explicitement son nuage entier quantifié de l'entrée binary64 originale. La condition barycentrique manquante du Théorème 3 a été ajoutée au document, l'ancien critère circulaire du Théorème 4 a été remplacé par une borne directionnelle *a priori*, et `big_cmp` ainsi que la prise de magnitude de `i128` ne présentent plus les deux erreurs signalées par le premier audit.

Pendant ce second audit, `well_centered4` a aussi été corrigé pour tenir compte du signe de l'orientation après la normalisation `den > 0` de `sphere4`. Il faut conserver une régression sur une permutation impaire d'un tétraèdre régulier : la fixture $a=(2,2,2)$, $b=(2,0,0)$, $c=(0,2,0)$ et $d=(0,0,2)$ doit construire la sphère de centre $(1,1,1)$, de niveau 3, et accepter le bon centrage quelle que soit la permutation des quatre sommets.

Ces réparations sont réelles, mais elles ne ferment pas les points suivants.

## 2. Le filtre par cliques de boules de faces était incomplet

Une révision observée pendant l'audit supposait qu'une boule circonscrite à une face d'un tétraèdre bien centré était incluse dans la circumboule du tétraèdre. L'intersection de la circumboule avec le plan de la face est bien le *disque* circonscrit de la face, mais la boule tridimensionnelle centrée dans ce plan n'est en général pas incluse dans la circumboule. Son rang peut donc être arbitrairement plus grand. Ce filtre a été supprimé de `src/catalogue.cpp` pendant l'audit.

Voici une fixture entière dans le domaine 16 bits. Posons les quatre sommets

$$U=\lbrace(160,160,160),(160,40,40),(40,160,40),(40,40,160)\rbrace.$$

Leur circumboule a pour centre $O=(100,100,100)$ et pour rayon carré $R^{2}=10800$. Pour chaque signe $\sigma$ dans

$$\Sigma=\lbrace(1,1,-1),(1,-1,1),(-1,1,1),(-1,-1,-1)\rbrace,$$

ajoutons les neuf points $w=O+70\sigma+\delta$, avec

$$\Delta=\lbrace(0,0,0),(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(1,1,0),(-1,-1,0)\rbrace.$$

Pour la face associée à $\sigma$, le centre de face vaut $C_{\sigma}=O+20\sigma$ et son rayon carré vaut 9600. Les neuf témoins vérifient strictement $\lVert w-C_{\sigma}\rVert^{2}<9600$ et $\lVert w-O\rVert^{2}>10800$. Chaque boule de face a donc un rang fermé au moins égal à $3+9=12$, tandis que la circumboule du tétraèdre contient exactement les quatre sommets de $U$. Le tétraèdre est régulier, bien centré, sans témoin supplémentaire sur son shell, et constitue une sphère critique de rang fermé 4.

Avec `s_max=11`, `threads=1`, `max_growth_rounds=100` et `cone_directions=42`, la révision antérieure avait parcouru le nuage entier autour du premier sommet et annoncé tous les points certifiés, mais avait omis ce support :

```text
n=40 spheres=1877 found=0 tetra_rank=4 tetra_wc=1 face_ranks=12,12,12,12
certified_p=1 nb_p=39
pairs=1560 triples=29394 quads=40922
```

La cause ne pouvait pas être réparée par un voisinage plus grand : les quatre faces sont volontairement de rang supérieur à 11. La révision courante retrouve désormais ce support avec le voisinage exhaustif. Cette fixture doit rester une régression permanente comparant le support, le rang, le shell et les membres à un oracle indépendant; aucun filtre de rang des boules de faces ne doit être réintroduit.

## 3. La borne directionnelle est mathématique, mais son implantation n'est pas un certificat

Le nouveau Théorème 4 et son argument par statistique d'ordre fournissent une voie mathématique plausible. En revanche, `make_cover` estime le rayon de couverture d'un ensemble de directions de Fibonacci sur seulement 20 000 échantillons, puis applique la constante heuristique `worst * 1.15 + 0.02`. Ce calcul ne prouve pas que les cônes couvrent la sphère. Les quotients, sinus, cosinus et le test terminal sont en outre calculés en `double`, sans intervalles dirigés ni repli exact.

Une révision observée pendant l'audit acceptait même `cone_directions=0`. La boucle des cônes était alors vide, `radius_bound` renvoyait zéro et chaque voisinage initial était déclaré complet. Une fixture exacte est $p=(100,0,0)$, les dix points $q_i=(99-i,0,0)$ pour $i=0,\ldots,9$, puis $z=(65535,0,0)$, avec $s_{\max}=11$. La boule diamétrale de $\lbrace p,z\rbrace$ a le rang fermé 2, car les dix autres points sont strictement à l'extérieur. Avant correction, le prototype retournait :

```text
pair=0 rank=2 p_certified=1 p_neighbours=10 uncertified=0 spheres=67
```

La révision courante marque désormais les recouvrements vides ou trop larges invalides et renvoie une borne infinie; les régressions `cone_directions=0,1,2` retrouvent la paire lointaine par épuisement. Le défaut direct est donc corrigé. En revanche, le recouvrement par défaut à 42 directions reste estimé par échantillonnage, et les divisions restent non dirigées. Tant qu'un rayon de couverture vérifié et des intervalles sortants ne remplacent pas ces heuristiques, `Catalogue::certified` ne peut porter aucune autorité d'exactitude.

## 4. Le lot exact devait contracter les fusions simultanées

Pendant l'audit, le tri des événements a été réparé pour appeler `sphere_cmp_beta`, les lots utilisent désormais l'égalité rationnelle, l'index des minima conserve la clé complète et `run` supprime les forêts lorsque le catalogue est censuré. Ces corrections doivent rester couvertes par des régressions.

Une révision observée pendant l'audit résolvait les événements de même niveau avant les unions, mais appliquait ensuite leurs unions une par une. Lorsque plusieurs événements reliaient transitivement les mêmes composantes, elle créait une chaîne de nœuds parent-enfant de même niveau au lieu d'un seul nœud de multifusion.

La fixture entière minimale est le carré de sommets `(0,0,0)`, `(2,0,0)`, `(2,2,0)` et `(0,2,0)` à l'ordre `k=1`. Ses quatre arêtes adjacentes portent quatre boules critiques de rang fermé 2 et de niveau 1. Elles relient simultanément les quatre minima de niveau 0; le merge tree contracté doit donc avoir un seul nœud de fusion à quatre enfants. Avec le voisinage exhaustif, la révision antérieure retournait :

```text
spheres=8 nodes=7 births=4 merges=3 killed=3 roots=1
node4 beta=1 kind=1 children=2 parent=5
node5 beta=1 kind=1 children=2 parent=6
node6 beta=1 kind=1 children=2 parent=-1
```

La chaîne de trois fusions binaires au même niveau contredisait la multifusion annoncée dans `DESIGN.md`. Elle ne changeait pas le nombre de composantes après le lot, raison pour laquelle l'oracle P2 ne la détectait pas. La révision courante contracte désormais l'hypergraphe du lot et retourne `births=4`, `merges=1`, `killed=3`, `nodes=5`, avec un unique nœud de niveau 1 à quatre enfants. Cette fixture doit rester permanente.

La descente abandonne encore après 4096 remplacements ou lorsqu'elle ne trouve pas d'intrus strict. `Forest::unresolved_arms` compte maintenant ces échecs, mais les racines résolues du même événement peuvent encore former une hyperarête partielle, et `run` ne supprime pas une forêt dont ce compteur est non nul. Une forêt partielle ne doit jamais être publiée : l'événement fautif doit être censuré atomiquement et l'autorité de toute la forêt retirée. L'oracle actuel ne ferme pas ces trous : son catalogue de force brute réutilise les prédicats du chemin testé; la comparaison P0 ne porte que sur les identifiants du support, pas sur le rang, les membres, le shell ou le niveau; les cas à shell supplémentaire sont exclus du résultat attendu; et P2 ne compare que des nombres de composantes à quelques niveaux.

Un témoin courant est le nuage $X=\lbrace(0,0,0),(1,0,0),(2,0,0),(1,2,0)\rbrace$, avec `K=3`, tous les ordres et le voisinage exhaustif. `run` publie encore :

```text
forests_suppressed=0
order=3 unresolved_arms=2 nodes=1 merges=0
```

La fixture est volontairement dégénérée : c'est précisément le domaine que `DESIGN.md` affirme traiter sans hypothèse de position générale. Une certification spatiale du catalogue ne certifie pas la descente topologique.

## 5. Complexité en $n$ et $K$, et réalité GPU du contrat 50 k

Pour un point $p$, notons $m_p=\lvert W_p\rvert$. Le code courant construit un bitset dense de taille $\Theta(m_p^{2})$. Il examine $\Theta(m_p^{2})$ triangles et peut scanner jusqu'à $m_p$ points pour chacun, soit $\Theta(m_p^{3})$. Il parcourt ensuite jusqu'à $\Theta(m_p^{3})$ cliques de taille trois et peut rescanner $m_p$ points pour chaque tétraèdre, soit $\Theta(m_p^{4})$. Sur tous les points, le coût est donc $\Theta(\sum_p m_p^{4})$, et atteint $\Theta(n^{5})$ lorsque les voisinages couvrent le nuage. Sous l'hypothèse non prouvée $m_p=O(K)$, il reste $O(nK^{4})$.

`RESULTATS.md` décrit correctement le nombre de triplets support quatre par $\Theta(\sum_p m_p^{3})$, mais ce n'est pas une borne du temps total : chaque triplet survivant appelle encore `classify`, qui peut parcourir $m_p$ points lorsque peu d'entre eux appartiennent à la boule. Les mesures observées avec arrêts précoces ne remplacent donc pas la borne précédente.

Le « peeling local » proposé dans `RESULTATS.md` n'est pas encore un algorithme output-sensitive démontré, et il n'est pas l'unique route de recherche. Énumérer le niveau peu profond complet d'un arrangement tridimensionnel autour de chaque point peut reconstruire en flux un objet de type Delaunay d'ordre supérieur et dupliquer ses cellules. Avant d'en faire le chemin produit, il faut borner séparément en $n$ et $K$ les plans, faces, cellules et sorties visités, ainsi que la mémoire et la duplication entre ancres.

Le filtre de propriétaire canonique annoncé dans `DESIGN.md` est actuellement vide : les payloads sont émis depuis plusieurs sommets puis dédupliqués après concaténation. La descente de chaque bras scanne aussi le nuage global et énumère les petits supports de la miniboule. Ces coûts ne matérialisent pas une mosaïque de Delaunay d'ordre supérieur, mais ils retrouvent un produit local d'arité quatre dont la taille n'est pas bornée de façon compatible avec le contrat.

Même l'hypothèse exploratoire $m_p\approx1700$ de `RESULTATS.md` n'est pas GPU-friendly pour ce code : le seul bitset dense `live` occupe environ 0,35 Mio par point, et $\binom{1700}{3}=817388900$ triplets sont possibles par point, soit environ $4,09\cdot10^{13}$ à 50 k points. Une tuile, un warp ou un bloc CUDA ne transforme pas cette masse en travail compatible avec une seconde.

Le CMake courant active uniquement `CXX`; aucun kernel CUDA, lease résident, parcours LBVH device, primitive de niveau peu profond ou pipeline G4 n'est présent. La couverture discrète et les flèches verticales annoncées comme sortie contractuelle ne sont pas non plus implémentées dans l'API. En conséquence, aucun test actuel ne permet d'espérer le contrat 50 k en moins d'une seconde sur G4.

## 6. Conditions minimales avant promotion

1. Conserver la suppression du filtre de rang des boules de faces et intégrer la fixture du §2 à un oracle réellement indépendant.
2. Remplacer le recouvrement sphérique échantillonné et les décisions `double` de certification par des bornes vérifiées, dirigées et fail-open; refuser toute configuration non couverte.
3. Conserver le tri rationnel, les clés complètes et la contraction des lots, puis censurer atomiquement tout événement ou toute forêt comportant un bras non résolu.
4. Tester les rangs, membres, shells, niveaux, dégénérescences et permutations, pas seulement les supports produits par les mêmes prédicats.
5. Fournir une source d'ancres ou de supports dont le travail mesuré et la mémoire sont sub-combinatoires sur 50 k, puis seulement un backend CUDA résident et une campagne G4 gardée.

Jusqu'à fermeture de ces obligations, `morsehgp3D_v2` reste une expérience de recherche utile mais non exacte, non complète et non qualifiée pour le contrat 50 k.
