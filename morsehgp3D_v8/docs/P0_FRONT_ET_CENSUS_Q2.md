# Front et census q2 : un seul index pour tout le nuage

14 septembre 2026. Huitième tranche, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Ce raccord produit les supports q2 et leurs populations, **pas la tour FULL**.

## Ce qui change concrètement

Le [front précédent](P0_FRONT_REEL.md) rendait des rectangles compacts.
Le census précédent travaillait sur un plan local séparé. Les relier en
construisant un plan pour chaque rectangle aurait répété des copies et
des parcours de facteurs des millions de fois. Le nouvel appel
`run_wspd_q2_census` consomme directement les deux nœuds spatiaux du
rectangle, dans le même index immuable que les témoins Z.

Le plus petit facteur fournit les points a ; le plus grand reste un groupe
B. Aucun tableau B, arbre B, plan Axis/Pool ou scan de couverture n'est
construit. Les tampons d'intérieurs et de coquille sont réutilisés pendant
tout l'appel. La variante `Pairwise` compte chaque paire du même résidu ;
`SharedBlocks` partage les décisions entre les points de B tant que les
bounds le permettent. Le choix du petit facteur diminue les démarrages,
pas nécessairement le travail des subdivisions ultérieures.

Le front est demandé **pour q2 seulement** : masque 1, aucun test Xi, pas
de branches ou masses q3/q4 sans consommateur. Son API indépendante garde
le masque 7 par défaut et intersecte la demande avec les voies actives
au seuil choisi. Les mesures du front à trois voies restent historiques ;
leur temps ne doit pas être soustrait du nouveau total pour inventer un
temps de census isolé.

## Exactitude et reprise du compte

Chaque paire non ordonnée est soit rejetée de façon sûre par le front,
soit comptée contre **tous les sites** de l'index, y compris ceux des deux
facteurs autres que ses propres extrémités. Pour q2, le support {a,b}
est conservé exactement lorsque moins de Kmax sites sont strictement
dans sa boule diamétrale. Les points sur la sphère ne sont pas intérieurs.

Le premier appel de chaque groupe commence avec compte zéro et curseur
racine. Aucun crédit du proposeur WSPD n'est ajouté à ce compte. Pendant
le census, un bloc Z uniformément intérieur contribue sa population ; un
bloc sans intérieur est sauté. L'état désigne toujours un préfixe consommé
de l'ordre DFS fixe et le premier sous-arbre non consommé. Si B est divisé,
les deux enfants héritent **ensemble** du compte acquis et de ce curseur.
Repartir de la racine en gardant le compte doublerait les crédits ; avancer
le curseur avant de transmettre les enfants ferait perdre des témoins.

Une fois un support accepté, la collecte parcourt réellement l'index et
émet tous les IDs intérieurs et toute la coquille, avec sa clé entière.
Les supports distincts d'une même boule sont encore tous émis : le cube
testé a quatre diagonales de même clé, chacune avec huit sites de coquille.
Il n'existe pas encore de catalogue global dédupliqué des boules. Une
subdivision peut atteindre une paire rejetée individuellement ; il serait
faux de dire que seules les paires acceptées sont jamais visitées.

L'entrée appelle elle-même le front ; elle n'adopte pas des handles
externes prétendus certifiés. Index et callback sont empruntés pour tout
l'appel synchrone. Les buffers sont privés à cet appel. Une exception du
callback se propage sans annuler les émissions précédentes ; un nouvel
appel sur le même index reste indépendant.

## Travail et mémoire : le carré n'est pas réputé résolu

Notons T les produits visités par le front, R ses rectangles émis,
L=Σmin(|A|,|B|), C les visites de comptage et E le coût de collecte et du
consommateur. Le chemin partagé coûte O(T(D+Kmax)+L+C+E) avec Samples,
O(T+L+C+E) avec Pure, après la préparation globale. D est la profondeur
de l'index ; **cette écriture est une comptabilité, pas une borne
sous-quadratique**, car T, L, C et E doivent encore être bornés ou mesurés.
Le census Pairwise ajoute explicitement l'expansion de toutes les
candidates. Shared peut lui aussi descendre aux paires sur un mauvais cas.

Compteurs de contrôle du nouveau raccord :

| Quantité | Sens |
| --- | --- |
| `input_rectangles`, `input_descriptors` | R, pas le nombre d'ancres |
| `anchor_queries` | L, dans les deux variantes |
| `count_root_starts` | L pour Shared ; candidates pour Pairwise |
| `query_tasks` | racines + 2 × subdivisions B |
| `query_build_*`, `query_cover_visits`, `query_index_ms` | zéro |
| `candidate_pairs` | masse résiduelle q2 du front |
| `accepted_pairs + rejected_pairs` | même masse, sans omissions |

Le ledger ne remplace pas l'oracle : un doublon et une omission pourraient
se compenser. Le gate confronte les objets individuels. La mémoire
persistante globale est linéaire ; piles et tampons s'ajoutent sans stocker
le front ni le catalogue. Les capacités publiées ne sont pas une mesure
RSS/VRAM de pointe. Les coquilles peuvent être grandes même à petit Kmax.

## Parallélisation : objets prêts, ordonnanceur encore absent

Les rectangles disjoints et leurs ancres peuvent devenir des jobs partageant
un index en lecture seule. Chaque worker devra garder ses propres tampons
et son état. Pour fractionner une longue tâche, transporter nœud B, ancre,
compte et curseur Z sans changer la permutation ; une file pleine suspend,
elle ne tronque pas le flux. Le callback courant n'est pas déclaré thread-safe.
Cette tranche n'introduit ni workers CPU, ni kernel GPU, ni vitesse GPU estimée.

Un meilleur partage pourrait garder **deux groupes** A et B pendant le
census, avec un état (A,B,compte,curseur Z), avant de choisir une ancre.
Les extrema de H sur trois boîtes peuvent certifier un bloc Z ; sinon
subdiviser A ou B transmet le même compte et le même curseur non consommé.
Il ne faut jamais exclure tous les sites de A∪B du rôle de témoin. Le test
d'intérieur strict ne suffit pas à la collecte de la coquille, qui garde
les égalités. Cette proposition n'est ni intégrée ni qualifiée et ne donne
pas, à elle seule, de borne globale sur le nombre de décisions.

## Contre-fixture de fragmentation prématurée

Le diagnostic suivant porte sur la politique de recherche, pas sur une
erreur géométrique. Prendre a=(1000,0,0) et B={(i,0,0):0≤i<m}, avec
m=8/16/32/64. A×B est séparé même pour s12 ; B est un véritable nœud
de l'index. Pour b_i, la profondeur exacte vaut m−1−i : les témoins
utiles sont précisément les points de B **à droite** de b_i.

Le DFS croissant parcourt d'abord les points à gauche, tous non intérieurs.
Quand le bloc Z coïncide avec le groupe B courant, sa borne est indécise
et sa diagonale égale celle de B. Le test strict `diag(Z)>diag(B)` choisit
alors de diviser B. Le même mécanisme se répète dans les enfants : aucune
requête ne reçoit de crédit avant d'être réduite à un singleton. Le nombre
de tâches B pour ce produit est donc 2m−1, quel que soit le petit K fixé. La qualification
du raccord reste exacte ; c'est le partage qui échoue.

Un modèle scalaire exact en mémoire, contrelu en parallèle, donne pour
m64/K10 127 tâches et 421 visites avec cet ordre, contre 27 tâches et
99 visites avec un DFS décroissant et **ses échappements propres** ; ce
dernier rejette 54 paires en groupes. À K2 : 127/349 contre 13/37, avec
62 rejets groupés. Ces nombres sont un diagnostic de modèle, sans reçu
de qualification du produit ni variante intégrée. La fixture et son
mécanisme restent ici pour un futur gate d'ordre/continuation.

A est déjà singleton : un état A×B×Z seul ne répare pas cette fixture.
Il faut aussi proposer des témoins du bon côté ou différer les blocs
indécis. Changer l'ordre Z exige une nouvelle identité d'ordre et des
échappements compatibles ; réordonner un suffixe en conservant aveuglément
le compte/cursor courant est interdit. Le lien causal quantitatif entre
cette fixture et les huit amas n'est pas encore profilé rectangle par
rectangle, mais le défaut de la politique est démontré sur ce cas simple.

Une expérience suivante minimale est maintenant contre-vérifiée : lors
de B→B_L/B_R, proposer à chaque enfant son **frère** comme bloc Z. Si
ce frère contient au moins Kmax sites et si Hmin(a,B_enfant,Z)>0, il
certifie seul le rejet de toutes les paires de l'enfant. Sinon, ne rien
retenir du test et reprendre compte/curseur inchangés. Le test utilise
deux nœuds déjà disponibles : aucune descente de proposeur, aucun
histogramme ou tableau de témoins local. Le crédit n'est **jamais ajouté**
au compte hérité ; ce certificat autonome reste sûr même si certains
de ses témoins ont déjà été parcourus. La stricte positivité interdit
automatiquement d'utiliser une extrémité comme témoin intérieur.

Sur la fixture m64/K10, ce certificat rejette 32 puis 16 paires en groupes,
soit 48 ; les 16 dernières nécessitent encore leur census, dont six
seront rejetées. Ne pas lui attribuer les 54 rejets groupés du modèle
à ordre inversé. Ce test et ses nouveaux compteurs restent à implémenter
dans une tranche/build distincte ; il n'établit aucune borne générale.

## Comment lire les mesures

La sonde paie génération, validation/copie du nuage, index global, front,
census, collecte, vérifications et hash canonique du callback, destructions.
Les IDs du payload sont réellement copiés, triés et contrôlés. Les sorties
de chaque support contribuent à une somme et un xor indépendants de l'ordre
d'émission ; ces empreintes ne remplacent pas un oracle géométrique massif.

`pipeline_total_ms` englobe front+census+collecte et destruction de ses
tampons ; `front_and_count_ms` inclut front et instrumentation. Le chrono
hérité `payload_ms` reste mesuré par groupe accepté, souvent par support.
Il n'y a pas de chrono par rectangle. Les champs détaillés de la pipeline
sont **imbriqués**, pas à ajouter une seconde fois au total.

Le lecteur compare le même digest canonique entre s8/10/12, Pure/Samples
et Pairwise/Shared à entrée et K identiques. Les compteurs peuvent varier
entre variantes ; leurs répétitions identiques doivent être stables.
Les données brutes, échecs, provenance et hashes de fermeture sont conservés.
Les reçus historiques sont relus avec leur propre version, jamais réécrits.

L'auditeur A a préparé un corpus de vrais scans LiDAR : le slab `terrain`
n'en est pas un substitut. Son [protocole indépendant](../audits/lidar08_20260914/README.md)
donne les collisions de quantification et les entrées ; le nouveau raccord
devra être mesuré sur ces mêmes fichiers avec un masque q2 apparié.
La [propagation étudiée par B](../audits/PROPAGATION_TEMOINS_20260914.md)
réduit certains résidus sans gain de temps de front universel. Elle ne
supprime presque pas le résidu transverse des amas/rangées. Ni ce prototype
ni ses mesures ne sont incorporés à cette qualification.

Les résultats propres à ce raccord et les tests sont consignés dans
le [reçu de la huitième tranche](../receipts/wspd_q2_census_20260914/README.md).
q3/q4, canonisation des boules, parents FULL, sous-quadraticité générale et
contrats G4 50k/massif restent ouverts. GCP non utilisé.
