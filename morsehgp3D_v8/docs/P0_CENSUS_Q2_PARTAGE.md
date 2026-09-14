# Census q2 : compter ensemble, conserver les bonnes identités

13 septembre 2026. Quatrième tranche P0, mono-thread, entrées u16.
`public_status=not_claimed`. Ce module consomme le résidu d'un rectangle ;
ce n'est ni le constructeur WSPD ni la tour HGP FULL.

## Ce que l'on veut savoir

Une paire de sites a,b définit la boule dont elle est un diamètre. Pour
conserver son support dans la fenêtre Kmax, il faut que le nombre p de
sites **strictement intérieurs** vérifie p<Kmax. Une fois Kmax atteint,
le compte exact au-delà n'est pas nécessaire. En revanche, pour une paire
conservée, il faut les identités de tous les sites intérieurs et de toute
la coquille. Les points sur la sphère ne sont pas des points intérieurs.

Pour un site z, le test exact est $H(a,b,z)=(z-a)\cdot(b-z)$ : H>0 donne
l'intérieur, H=0 la coquille. L'identité $4H=\lVert a-b\rVert^2-\lVert 2z-a-b\rVert^2$
permet le calcul entier même si le centre est demi-entier. Tous les produits
sont effectués après promotion des coordonnées u16 vers des entiers signés
64 bits. Il n'y a ni tolérance flottante ni arrondi du centre.

Le compte repart de zéro sur un index de **tous** les sites du propriétaire,
pas seulement ceux de A et B. Les témoins du préfiltre sont donc rencontrés
dans cet index comme tous les autres : leur crédit n'est jamais ajouté à
l'avance. Le préfiltre reste une élimination certifiée, pas un certificat
d'appartenance que le census pourrait réinterpréter librement.

## Deux parcours à comparer sur les mêmes candidates

La référence interroge l'index pour chaque paire représentée par les plages
du filtre. Une boîte de sites entièrement intérieure est comptée d'un coup ;
une boîte sans intérieur est ignorée ; sinon le parcours descend. Il s'arrête
à Kmax. Il ne balaie pas systématiquement les n sites pour chaque paire.

Le parcours partagé garde a fixe et traite plusieurs b ensemble. Son état
contient un groupe B, un compte uniforme déjà acquis et un curseur désignant
les boîtes de témoins **encore à examiner**. Les bornes exactes de
l'[auditeur](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
permettent trois opérations :

- Si toute une boîte Z est intérieure pour toutes les paires du groupe,
  ajouter sa population une seule fois au compte du groupe.
- Si aucun site de Z ne peut être intérieur, retirer cette boîte du travail.
- Sinon, subdiviser les témoins Z ou les requêtes B ; à une seule paire,
  employer le même test de distance que la référence.

Lorsqu'on partage B, les deux enfants héritent du même compte et du même
curseur. Repartir de la racine avec ce compte serait faux : certains témoins
seraient crédités deux fois. L'index est numéroté une fois dans l'ordre du
parcours en profondeur ; chaque nœud indique où reprendre après son sous-arbre.
Consommer un bloc saute à cette position ; le partager descend à son premier
enfant. Le curseur représente ainsi tout le suffixe non consommé, sans liste
de continuation par requête. C'est la simplification démontrée en
[section 9.1](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#91-une-continuation-de-census-peut-tenir-dans-un-seul-curseur-z).
Elle exige de garder cet ordre Z fixe. L'ordonnancement des groupes B reste
libre, puisque chacun possède son propre état.

## La coquille et la sortie sont réellement payées

Le passage de comptage peut ignorer une boîte dont le maximum de H vaut
zéro : elle n'ajoute aucun intérieur. Ce raccourci ne peut pas être repris
tel quel pour collecter la coquille. Une seconde recherche indexée collecte
les intérieurs et les égalités des supports sous le seuil. Son coût et celui
du consommateur synchrone de sortie appartiennent au temps mesuré. Le nombre
d'intérieurs collectés doit correspondre au compte initial, sinon l'exécution
échoue ; cette vérification ne remplace pas le juge indépendant.

Chaque émission conserve les deux IDs du support, les IDs intérieurs et de
coquille, et la clé entière `(a+b, |a-b|²)`. Deux diamètres peuvent avoir la
même clé. Il s'agit d'un **flux d'incidences de supports**, pas d'une liste
déjà dédupliquée de boules. La déduplication globale, q3/q4 et les vrais
parents FULL restent à construire. Le cardinal de coquille ne remplace
jamais le support minimal q_min=2. Une paire sans intérieur mais avec une
coquille supplémentaire n'est pas automatiquement une feuille Gabriel
stricte du profil régulier.

L'index, le plan et le consommateur sont empruntés pendant tout l'appel :
ils doivent rester vivants et non invalidés, callbacks compris. Les vues
d'IDs ne survivent pas au callback. Une exception se propage sans annuler
les émissions déjà reçues ; une reprise recommence depuis zéro. Le client
doit donc distinguer ou abandonner le flux incomplet précédent, jamais
l'annoncer comme un census achevé.

## Ce que le partage ne prouve pas

Il évite de matérialiser le produit A×B et de préparer des histogrammes
A×A/B×B. Il n'implique pas que toutes les boîtes soient faciles à classer.
Des limites géométriques défavorables peuvent imposer beaucoup de visites,
même si le résultat final est petit. L'index, ses requêtes, les continuations
et les IDs effectivement émis doivent donc tous être mesurés. Les échappements
de l'index occupent O(n) places partagées ; aucun historique de listes n'est
alloué par requête. Cette économie de représentation n'est pas à elle seule
une accélération mesurée.

Plus précisément, l'index global a 2n−1 nœuds et la construction paie
$\sum_{z}(2\mathrm{depth}(z)+1)\leq 97n$ visites comptées : un passage
pour la boîte et un autre pour le partage à chaque niveau interne. La
profondeur au plus 48 vient des coordonnées u16 et des coupures au milieu,
pas d'un arrêt imposé. L'arbre B est construit de bas en haut avec 2|B|−1
nœuds et |B| lectures de sites. Pour D descripteurs, leur couverture paie
O(D(1+log |B|)). À cela s'ajoutent **toutes** les classifications géométriques
J, les visites de collecte et les IDs émis : aucun de ces termes n'est
absorbé abusivement dans O(n). La représentation propre au census occupe
O(n+|B|) hors plan d'entrée et éventuel stockage demandé par le callback ;
l'état d'une tâche est de taille constante et la pile reste logarithmique
en |B|, plus la profondeur de l'index Z.

La croissance à n=8 000/16 000/32 000 qualifie les familles exécutées, pas
une borne sous-quadratique uniforme. La lecture de chaque support accepté
et de ses IDs a son propre coût de sortie. Depuis la
[sixième tranche](P0_NUAGE_ET_INDEX_PARTAGES.md), l'index appartient au nuage
et sert plusieurs rectangles/seuils ; le seuil vient du plan et non de Z.
Le raccord à une vraie WSPD reste distinct. Les paramètres s8/10/12 de ces
fixtures vérifient la séparation d'un rectangle fixé, sans mesurer encore
trois décompositions Callahan–Kosaraju différentes.

Sources mathématiques : [bornes exactes](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches),
[census individuel de comparaison](../../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md),
[fixtures de coquille et d'identité](../../audits/morsehgp3D_v8_complementaire/P0_CENSUS_Q2_ET_COQUILLE.md).
Ces sources d'audit ne sont pas une qualification héritée de l'implémentation.

## Qualification et décision

Les 31 CTests Release et Clang ASan/UBSan passent, avec 277 cas du juge
census et des comparaisons physiques aux intérieurs/coquilles cpp_int.
Les [204 mesures appariées](../receipts/q2_census_20260913/README.md)
incluent l'aval et des essais de composant à 50k, sans qualifier la tour.
L'addition et l'intersection peuvent gagner sur le coût total. Shared
gagne sur certains grands résidus mais perd après une forte sélection ;
aucun choix automatique n'est introduit.

Prochaine économie à tester : préparer, pour chaque extrémité e de B et
chaque axe, les constantes C=a+e et D=(e−a)² une fois par tâche. Les extrema
de 4H s'obtiennent par D moins les distances carrées à l'intervalle 2Z,
avec réemploi des carrés des extrémités. Cette identité évite des produits
répétés sans table A×B ; son gain effectif, y compris ce que le compilateur
économise déjà, reste à mesurer. La comparaison directe de Pool sans axe,
la vraie WSPD et le raccord FULL ne doivent pas être oubliés derrière
les optimisations de ce seul composant.
