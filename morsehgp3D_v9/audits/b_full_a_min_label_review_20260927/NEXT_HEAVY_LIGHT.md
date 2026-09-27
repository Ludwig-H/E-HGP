# Suite proposée : coupes pondérées en espace linéaire

27 septembre 2026. Contre-analyse de la proposition du développeur.
**Proposition seulement** : aucun nouveau code, test, benchmark ou gain.
Ne modifie ni ne qualifie le port min-label à table `up` en cours.

## Conditions indispensables

On conserve la forêt virtuelle de minima, distincte de la DSU de travail.
Pour chaque sommet non racine, `parent[v] < v` et la perte de l'arête
`v → parent[v]` ne diminue pas en remontant vers la racine. Une racine
se pointe elle-même et porte `INF = absent32` ; toute coupe admise est
strictement inférieure à INF. Les sommets sont des ordinals chronologiques,
pas des PointId géométriques.

Ces propriétés suffisent à remplacer la table `up` en O(V log V) par une
décomposition heavy-light en O(V). Le fils lourd est un fils de taille
maximale de sous-arbre ; ce choix, et non le seul nom « lourd », est
nécessaire à la borne du nombre de sauts.

## Construction proposée

Un passage dans l'ordre décroissant des IDs accumule les tailles des
sous-arbres et choisit le fils lourd de chaque parent. Chaque enfant a
déjà reçu toutes les contributions de ses descendants. Ne pas accumuler
une racine dans elle-même. Les égalités peuvent être départagées par ID.

Un passage croissant calcule ensuite la tête et la profondeur relative
de chaîne : le fils lourd hérite de la tête du parent ; les autres fils
ouvrent leur propre chaîne. Les parents ont déjà été traités. Les longueurs
de chaînes, un préfixe puis un scatter placent leurs sommets dans des
plages contiguës, de la tête vers les descendants. Il faut garder de quoi
retrouver le sommet et son parent à partir de cette position ; un tableau
de pertes seul ne suffit pas à restituer son identité.

Cela évite CSR d'adjacence, DFS d'enracinement et table d'ancêtres : la
relation parent et son ordre suffisent. Les tableaux de taille, fils lourd,
tête, profondeur, longueurs/préfixes et permutation restent tous O(V).
Leur réutilisation et leur coexistence doivent être comptées, sans déduire
un nombre d'octets d'un simple ordre asymptotique. Les additions de tailles
et d'offsets restent contrôlées.

## Requête : plusieurs sauts légers, une seule recherche

Pour une coupe fermée c, « autorisée » signifie `perte <= c` ; pour une
coupe ouverte, remplacer ce test par `perte < c`.

À partir de v, soit h la tête de sa chaîne :

1. Si `perte[h]` est autorisée, toutes les arêtes de v à h puis l'arête
   qui sort de h le sont. Remplacer v par `parent[h]` et recommencer.
   Cette dernière arête est légère. Le cas h racine ne peut pas arriver,
   puisque sa perte INF est toujours interdite.
2. Sinon, si `perte[v]` est interdite, renvoyer v.
3. Sinon, chercher dans la plage h..v le premier sommet w dont la perte
   est autorisée, puis renvoyer `parent[w]`.

Dans cette plage, les pertes sont non croissantes de h vers v. Le dernier
cas est donc une recherche binaire. Toutes les arêtes jusqu'à w sont
autorisées ; l'arête qui sort de son parent est interdite. La réponse
est définitive : il n'y a pas une recherche binaire par chaîne.

Chaque saut léger au moins double la taille du sous-arbre en remontant,
donc il y en a au plus O(log V). Une seule recherche finale coûte O(log V).
La requête totale reste O(log V), pas O(log² V), en espace auxiliaire O(1).
Une chaîne pure ne déclenche aucun long parcours parent par parent.

## Ce qui est supprimé et ce qui reste

L'index de coupes devient O(V), à la place de la table `up` O(V log V).
La construction des tailles/têtes/scatter proposée est O(V) mais encore
séquentielle. La construction DSU de la forêt, les tris de groupes, les
occurrences E, les historiques des créateurs, les ancres et la sortie
explicite restent à payer. Aucun changement de la règle des continuations
ou des contributions n'est proposé.

Cette baisse de mémoire de l'index ne prouve ni une baisse de temps sur
GPU, ni une borne sous-quadratique du générateur LiDAR. L'adaptation d'une
construction massivement parallèle reste une tâche distincte ; multiplier
les parcours de parents pour calculer les têtes pourrait restaurer un
travail quadratique sur une chaîne.

## Porte minimale avant tout remplacement

Comparer toutes les coupes distinctives à un parcours du graphe original,
et les objets A complets à la version `up` et à la référence chronologique.
Inclure chaîne longue, étoile, arbre équilibré, minima en branche latérale,
plusieurs composants, égalités de tailles lourdes, plateaux de pertes égales,
racine seule, coupe zéro et dernière perte finie `absent32−1`.

La chaîne `2 → 1 → 0`, pertes `3,10,INF`, coupe 5 doit rendre 1 ; avec
`7,7,INF`, fermé 7 rend 0 et ouvert 7 rend 2. Les pertes non monotones
ou une forêt externe non certifiée ne sont pas des entrées admissibles
de cette couture. Aucun test ni remplacement n'a été exécuté ici.
