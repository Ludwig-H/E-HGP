# Index global exact pour les descentes

Tranche v11 du 2 octobre 2026, CPU, profils entiers 18/21/24, défaut 21.
Qualification G4 en préparation. Cette brique ne calcule ni MEB ni FULL.
Le catalogue qualifié à `ffc2ff95f` reste inchangé.

## Propriété et domaine

`build_index(Cloud&&, IndexParams, MemoryBudget&)` construit un `GlobalIndex`
immuable. Il ne transfère le Cloud qu'au succès : un refus laisse le Cloud
et ses vues intacts. L'index possède alors nuage et arbre ; son déplacement
transfère les tableaux sans conserver de pointeur vers l'ancien objet Cloud.
Les budgets respectifs doivent survivre aux tableaux. Construire le futur
catalogue avec `index.cloud()` conservera la même interprétation des SiteIdx.
Les objets déplacés deviennent vides et une nouvelle requête les refuse.

Les sites suivent l'ordre Morton déjà certifié par Cloud. L'index conserve
tous les sites, leurs poids et tous les PointId. Le census compte **les sites
géométriques distincts**, sans développer les poids. Cette convention ne
qualifie pas FULL pondéré. Aucun masque, sous-échantillonnage ni copie des
coordonnées n'est introduit par l'index.

Une requête prend une Sphere valide q1..4 et un seuil u32 strictement positif.
Ses supports peuvent être extérieurs au Cloud ; elle ne suppose ni centre
dans les supports ni centre dans leur boîte. Elle rend obligatoirement :

- `saturated` si au moins K sites sont strictement intérieurs : exactement
  K témoins distincts, triés par SiteIdx, coquille vide ;
- `complete` sinon : tout l'intérieur I et toute la coquille U, chacun trié.

Les témoins ne sont pas certifiés K plus proches. Le résultat possède ses
listes et ne dépend plus de la durée de vie de l'index pour lire ses IDs.
Leur interprétation géométrique exige toujours le même Cloud.
L'index ne matérialise ni cellules de Delaunay, ni cofaces, ni graphe global
de couples de sites ; son seul intermédiaire global est cet arbre linéaire.

## Arbre et parcours

L'arbre divise chaque plage Morton en deux moitiés jusqu'à `leaf_size`
(défaut 8, domaine 1..256). Il est stocké en préordre ; chaque nœud porte
une boîte fermée exacte, sa plage et le premier indice après son sous-arbre.
Les feuilles scannent leurs points une fois à la construction ; les boîtes
internes réunissent celles des deux enfants. Construction O(n), mémoire
O(n), profondeur au plus ceil(log2 n)+1. Ces bornes commencent **après** la
préparation Cloud et ne bornent pas une tour ou le nombre de requêtes FULL.

Avant l'allocation, le nombre de nœuds se calcule en O(log n). Au premier
niveau de largeur w=2^d où q=floor(n/w)≤leaf_size, les plages ont q ou q+1
sites. Si q=leaf_size, les r=n mod w plages de taille q+1 se divisent encore ;
sinon elles sont toutes feuilles. Ainsi L=w+(q=leaf_size ? r : 0) et le
nombre de nœuds vaut 2L−1. Il reste inférieur à 2n≤2^33.

Le census parcourt ces liens sans pile. Sur la boîte, `num::power_bounds`
borne H(x)=D‖x−a‖²−2N·(x−a). Un minorant **strictement positif** écarte tout
le bloc ; un majorant **strictement négatif** accepte ses sites intérieurs.
Les égalités sont raffinées, puis jugées exactement aux feuilles. Toute
coquille étendue est donc conservée ; aucune tolérance n'intervient.
Les boîtes sont globales au propriétaire, jamais celles d'une liste locale
du catalogue réutilisée après déplacement du centre d'une MEB.

Les bornes séparent les extrema quadratiques et linéaires sur chaque axe :
elles sont sûres mais pas nécessairement atteintes au même point. Aucun N²
ni degré dix n'est introduit. Les majorants absolus de chaque expression
restent ceux de la puissance : moins de 72 M^5 pour q4 et 216 M^6 pour q3,
M=2^B. Q3 emploie Wide en 21/24 ; q1/q2/q4 et q3 en18 restent natifs i128.
La validité ne suppose aucune positivité barycentrique.

## Ressources, déterminisme et portée

Un premier parcours compte jusqu'à K ; le second remplit des buffers de
taille exacte. Réservation propre du résultat : 4K octets si saturation,
sinon 4(|I|+|U|). La coquille abandonnée d'une réponse saturée n'est jamais
allouée. Un échec restitue les réservations de cet appel, sans publier de
préfixe. Le travail des deux parcours est **cumulé** dans CensusLedger.
L'arbre réserve exactement `nodes()*sizeof(Node)` octets ; aucune allocation
de taille dépendant de n n'est extérieure aux Buffer. Les appels ont une
pile bornée à 33 cadres à la construction et constante à la requête.

L'ordre gauche/droite suit les plages Morton, y compris lors de l'acceptation
d'un bloc : les sorties sont déjà croissantes. L'index partagé reste constant ;
chaque requête possède son état et son budget avec pilote unique. Au pire,
une requête visite O(n) nœuds/sites, et ses deux passes doublent ce travail.
Aucune borne sous-quadratique sur FULL ne découle de cette brique.

Les portes indépendantes utilisent Gram/Fraction puis un scan global pour
les petites entrées. Le banc massif compare aussi chaque réponse à un scan
global utilisant la primitive `num::side` qualifiée : ce second contrôle
est indépendant du parcours, **pas** un oracle arithmétique indépendant.
Les requêtes artificielles du banc ne représentent pas encore les descentes
d'une tour. Les coûts Cloud, arbre, requêtes et scan témoin sont séparés.
