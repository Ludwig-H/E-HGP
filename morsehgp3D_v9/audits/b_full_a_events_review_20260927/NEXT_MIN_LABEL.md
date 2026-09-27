# Proposition : étiquettes minimales stables et historiques de nœuds

27 septembre 2026. **Analyse mathématique seulement**, après la porte
événementielle R1 ; aucun code, test, gain ou qualification nouveaux.
Ne pas modifier R1 pour lui attribuer cette représentation.

## Ce qui peut réellement disparaître

La nécessité des historiques muets dans R1 dépend de son étiquette de
composante, choisie par un enracinement arbitraire. Choisissons plutôt le
minimum des **ordinals de naissance des sommets**, pas le minimum des
PointId géométriques. Pour K1, les sites occupent les premiers ordinals,
puis les blocs ; pour les autres K, les ordinals suivent le programme.

Un bloc nouveau a un ordinal supérieur à tous ses représentants strictement
antérieurs. Un groupe qui rejoint un seul composant ancien ne peut donc
pas en changer le minimum. Il ne change pas non plus son nœud HGP vivant.
Un historique étiqueté par ce minimum peut omettre une telle entrée, même
si le groupe porte une contribution. Cette contribution doit bien sûr
rester une action datée dans la sortie.

Il faut conserver les naissances et les fusions de **plusieurs composants
anciens**, même sans contribution : ces événements changent le nœud
vivant. « Sans contribution » et « inerte à un parent » ne sont pas des
critères interchangeables. La mutation R1 qui retire toutes les entrées
sans contribution est donc plus forte que la simplification proposée.

## Obtenir ce minimum sans un deuxième arbre à 2V sommets

On peut construire une forêt de fusions **virtuelle** pendant le même
parcours croissant des occurrences :

1. Une DSU de travail garde rang et minimum de chaque composant.
2. Lors d'une union acceptée de composants de minima distincts a et b au
   rang r, écrire `parent[max(a,b)] = min(a,b)` et `weight[max(a,b)] = r`.
3. Mettre à jour le minimum du composant DSU résultant. Conserver cet
   arbre séparé de la compression de chemins de la DSU.

Le sommet perdant son statut de minimum ne peut jamais le retrouver :
chaque case parent est écrite au plus une fois. Les IDs parents diminuent
strictement, et les poids des vraies arêtes ne diminuent pas en remontant
vers la racine. Les racines se pointent elles-mêmes ; l'index maximum
existant traite leur convention de poids sans modifier les coupes.

Ce n'est **pas** nécessairement un sous-graphe du graphe des représentants.
La justification est plus simple : par induction sur les unions, remplacer
une arête entre deux composants par une arête entre leurs minima fusionne
exactement les mêmes composants. Toute coupe ouverte ou fermée d'un rang
contient les mêmes unions que l'original. Les égalités de poids doivent
toutes être incluses ou toutes exclues ; leur ordre de traitement n'ajoute
aucun niveau HGP.

Dans cette forêt, le sommet le plus haut atteint sous la coupe est justement
le minimum du composant. Les requêtes par ancêtres/maximum de R1 restent
applicables. La construction sérielle proposée coûte `O(E α(V)+V)` avec
DSU par rang et minimum séparé. Elle supprime potentiellement le CSR
d'adjacence, la réserve d'arêtes de forêt et le parcours d'enracinement ;
elle n'exige pas un arbre de reconstruction binaire supplémentaire.

Un minimum le long du seul chemin dans la forêt R1 **ne suffit pas** : le
minimum du composant peut être dans une autre branche. Il faut changer
la forêt comme ci-dessus ou payer une vraie requête de minimum de composant.

### Variante proposée par le développeur : une seule table `up`

La monotonie des poids permet aussi de supprimer la table des maxima.
Posons `perte[v] = weight[v]` pour un sommet non racine et `INF = absent32`
pour une racine. Tous les rangs admis sont strictement inférieurs à INF.
La table `up[p][v]` contient l'ancêtre à distance `2^p`, racines fixes.

Pour une coupe fermée r, parcourir p en ordre décroissant : si
`perte[up[p][v]] <= r`, remplacer v par cet ancêtre. Puis, **une fois**,
si `perte[v] <= r`, remplacer v par son parent. La coupe ouverte utilise
strictement `< r` dans les deux tests.

Preuve : les pertes ne diminuent pas vers la racine. Le test sur l'ancêtre
certifie donc toutes les arêtes sautées **et** l'arête qui sort de cet
ancêtre. Les sauts trouvent le plus haut sommet dont l'arête sortante est
autorisée ; le dernier pas franchit justement cette dernière arête.
Si l'arête initiale est interdite, tous les tests sont faux et v reste
inchangé. INF évite tout saut à travers la racine. Les égalités de poids
font traverser tout un plateau en fermé et aucune de ses arêtes en ouvert.

Contre-fixtures minimales à conserver dans une future porte :

- Chaîne `2 → 1 → 0`, pertes `3,10,INF`, coupe 5 : depuis 2, réponse 1.
  Tester seulement la perte de départ pour autoriser un grand saut donnerait
  0 ; omettre le dernier pas donnerait 2.
- Même chaîne, pertes `7,7,INF` : depuis 2, fermé 7 donne 0, ouvert 7
  donne 2. Ne pas créer une frontière artificielle à l'intérieur du plateau.
- Racine seule : tous les sauts restent en place, quelle que soit la coupe
  finie admise.
- Une chaîne de pertes décroissantes vers la racine invalide le lemme :
  elle ne peut pas être produite par la construction chronologique et ne
  doit pas être admise comme index arbitraire externe.

Avec u64 pour `up` et u32 pour `perte`, cela donne `8 V L + 4 V` octets
logiques, contre `12 V L` pour ancêtres et maxima de R1, `L=bit_width(V)`.
Ce n'est pas un pic d'allocation : DSU, minima, sorties, capacités et leur
coexistence restent à compter. La proposition n'a pas encore de port testé.

## Historiques plus petits, sans doublement de redirections

Après groupement par `(rang, minimum fermé)`, compter les minima distincts
des composants parents ouverts. Ce compte ne dépend pas des NodeId.
Les groupes dont le compte diffère de un créent des nœuds ; un préfixe
dans l'ordre natif leur assigne directement les IDs.

Les historiques par minimum peuvent alors contenir seulement ces groupes
créateurs. Pour un parent ouvert au rang r, le dernier créateur de son
minimum strictement avant r est son nœud vivant. Pour l'ancre d'un bloc,
le dernier créateur de son minimum fermé au rang r, inclusivement, donne
son nœud après le plateau. Le groupe à un parent retrouve directement
l'ID précédent : pas de graphe de redirections ni de doublement nécessaire.

Les groupes entièrement inertes peuvent être écartés de la liste des
actions après classification. Leurs blocs ne disparaissent pas du
manifeste : il faut toujours écrire leurs ancres, rendre les racines de
leurs occurrences si demandées, et payer leurs comptes sémantiques. Une
requête par bloc vers l'historique fermé suffit en principe à retrouver
ces ancres sans un historique muet persistant.

## Bilan honnête et prochaine contre-fixture

Il s'agit d'une suppression réelle de certaines structures intermédiaires
(historiques muets, redirections, CSR/DFS d'enracinement, table des maxima
dans la variante monotone), pas d'une
suppression du travail géométrique ou de toutes les occurrences. Restent
les tableaux V, la DSU de construction, l'index `O(V log V)` dans cette
version, les tris/groupements, les données E et les sorties explicites.
Des tables de requêtes peuvent dominer le gain ; aucun chrono n'est acquis.
La construction DSU reste sérielle ici : sa parallélisation demeure à payer.

Une porte distincte devrait comparer toutes les coupes et tous les champs
A à R1 et au natif, notamment : composant ancien dont le minimum est dans
une branche latérale ; longue chaîne de blocs inertes ; continuation
contributrice ; fusion sans contribution ; plusieurs unions au même rang ;
permutations des égalités ; K1 singleton et IDs non identitaires. Les
parents historiques, niveaux représentés et contributions datées ne
doivent subir aucun changement.
