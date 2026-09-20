# q4 : compter par blocs, puis balayer seulement le résidu local

20 septembre2026, tranche28 après5ff70645. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## L'idée, simplement

Toutes les boules passant par une arête ont leur centre dans un même plan.
Une face incidente à l'arête restreint ces centres à une droite. La tranche27
partageait des certificats de rejet dans ce plan, mais une face survivante
devait encore relire tous les témoins de la région couverte.

Cette tranche construit un objet différent : dans chaque région du plan,
un **compte exact** des sites toujours intérieurs, et des blocs de sites
encore indécis. Les autres sites sont extérieurs partout dans cette région.
Le balayage d'une face ne lit alors que les blocs indécis rencontrés.
Les événements situés hors de la région ne sont pas triés : leur contribution
constante est intégrée au compte, exactement.

La [proposition de l'auditeur A5ff70645](../audits/q4_local_sweeps_20260920/README.md)
et son [clipping](../audits/q4_local_sweeps_20260920/CLIPPING.md) sont portés
explicitement, sans importer son scan scalaire de préparation, son code
ou ses qualifications. L'ancien raccord reste inchangé. La nouvelle entrée
`run_q4_local_edge_candidates` produit **q4 seulement**, pas q3, le catalogue
ou FULL. Elle ne dépend pas de l'acceptation de q2 ou de q3 et n'empile pas
le filtre26 devant ces nouveaux balayages.

## Une partition exacte, pas le cache27

On reprend la base entière du plan de l'arête et les formes affines
L_z égales à quatre fois la puissance du site z. Sur une cellule fermée C :

$$I_C=\{z:\max_C L_z<0\},\qquad E_C=\{z:\min_C L_z>0\},\qquad A_C=\text{cover}\setminus(I_C\cup E_C).$$

Le fragment possède c=card(I_C) et une frontière de nœuds spatiaux disjoints
contenant A_C, éventuellement un surensemble. Un bloc non classé reste
entièrement actif, ce qui conserve exactement, pour tout centre t de C :

$$p(t)=c+\sum_{z\in A_C}[L_z(t)<0],\qquad S(t)=\{z\in A_C:L_z(t)=0\}.$$

Le terme « actif » désigne ici la population effectivement conservée,
pas seulement les sites dont la forme coupe géométriquement la cellule.
Un budget de tests nul garde toute la population sans la classifier.
Les contacts min=0 restent actifs. Les extrémités a,b, nulles partout,
restent actives et paraissent une seule fois dans chaque coquille émise.
Elles ne sont pas encore factorisées hors des frontières des feuilles.

La géométrie possède cover/index/nuage immuables. Elle décompose une seule
fois les plages du cover en nœuds spatiaux complets, sans scan de coordonnées
ni liste globale de formes. Un bloc traversant une frontière de plage est
subdivisé avant tout emploi de sa population. Le domaine positif utilise
le préparateur par blocs27, complétions obtuses incluses ; les témoins hors
lentille ne disparaissent pas du cover.

Un enfant hérite du compte exact et classe seulement la frontière de son
parent. Un nœud crédité ou exclu n'est jamais revisité dans cette branche.
La subdivision Z le remplace par ses enfants disjoints. Si le budget de
tests expire, le bloc courant et le suffixe restent actifs : aucun trou
dans la population. Les nœuds actifs sont copiés par identifiants d'index,
pas par coordonnées. Cette première version ne possède pas encore une
frontière persistante avec partage des listes entre frères : leurs copies
et capacités sont donc réellement payées.

## Bornes simultanées sur témoins et centres

Pour w=2z−a−b et t=2c−a−b, L=|w|²−D−2w·t. À témoin fixé, L est affine
en t : il suffit des quatre coins de la cellule. À centre fixé, chaque
terme axial Qw²−2Tw est convexe sur l'intervalle de la boîte spatiale.
Son maximum est à une extrémité ; son minimum est atteint à T/Q projeté
sur cet intervalle. Lorsque T/Q est intérieur, on utilise −ceil(T²/Q),
arrondi vers le bas, jamais vers zéro. Ces bornes peuvent être larges,
mais ne peuvent retirer un intérieur ni une tangence.

Les cellules utilisent Q=2⁴⁴ et la profondeur est au plus44, domaine
arithmétique du format, pas arrêt d'une recherche exacte. Sous u16 :
|T|≤4MQ, T²≤16M²Q²<2¹²⁴, M=65535. Les minima/maxima résultants sont
O(M²Q). La géométrie du disque/hull conserve les majorations27. Tous
les produits sont promus en i128 avant multiplication.

## Atlas immuable et calculs indépendants

La factory construit l'arbre des centres avant les requêtes. Une cellule
est hors domaine, uniformément trop profonde, subdivisée, ou feuille exacte.
Les deux premiers états sont des rejets, pas des comptes de census réutilisés.
L'arrêt à profondeur/budget de nœuds ou petite population conserve une
feuille exacte complète. Avant de la partager entre les faces, sa frontière
Z encore non classée est terminée **une seule fois**, par blocs, sans budget
de tests résiduel. Cela évite de faire payer à chaque face les gros blocs
jamais traités pendant la construction bornée. Si le compte final suffit,
la cellule devient profonde ; sinon elle conserve les sites actifs exacts.
Cette préparation terminale, potentiellement linéaire dans son résidu,
reste payée et ne constitue pas une borne globale. Aucun minimum d'enfants
comprimés n'est calculé.

Après construction, l'atlas est immuable : différentes requêtes peuvent
le partager, chacune avec ses propres buffers et compteurs. L'appel par
arête réutilise ses buffers entre faces. Cette structure permet de partager
la préparation sans états mutables concurrents ; cette tranche ne livre
pas encore un ordonnanceur q4 multithread ni un backend GPU.

Le budget de nœuds doit contenir au moins la racine ; un budget1 signifie
un balayage exact dans cette unique feuille. Le budget Z borne les seuls
fragments initiaux/intermédiaires, **pas la finition terminale ni le total
de l'atlas** ; aucun budget ne borne le nombre de candidats.
Une erreur de construction ne publie aucun atlas partiel. Une exception
de callback laisse les sorties antérieures chez l'appelant ; il n'existe
pas de reçu de travail partiel présenté comme un succès.

## Clipping exact avant le tri

Pour la droite de la face et une cellule rencontrée, on construit un point
rationnel t₀ sur leur intersection fermée. Toute racine extérieure est
remplacée par le signe de sa forme en t₀, constant sur ce segment. Les
racines aux bords, même si le segment est réduit à un coin, restent des
événements. Les formes constantes nulles restent dans la coquille commune.

Le balayage initialise le compte à −∞ pour les seuls événements conservés,
avec le compte uniforme et les nouvelles constantes. Il retire les sorties,
lit le compte strict et la coquille du groupe, puis ajoute les entrées.
Ce compte n'est jamais saturé. L'option sans clipping constitue une
référence différentielle locale : elle trie toutes les racines actives,
mais ne publie que celles appartenant à la cellule.

Les cellules sont fermées pour les bornes. Une racine sur une frontière
interne appartient à droite/haut ; les bords externes sont conservés.
Cette propriété ne change que l'émission, pas les mises à jour du compte.
Après cela viennent profondeur, propriétaire de l'arête, positivité du
tétraèdre et face canonique, comme dans le raccord couvert24. **Seulement
après ces tests**, profondeur et coquille couvertes sont globales.
Les IDs intérieurs ne sont pas collectés ; cela reste un travail futur
après regroupement des boules identiques.

Les intersections de deux formes sont de degré quatre ; leurs localisations
dyadiques sont O(M⁴Q). Le point t₀ a des numérateurs O(M²Q) et sa vérification
de frontière inclut O(M²Q²), sous2¹²⁵. Son évaluation dans une forme est
majorée par616M⁴Q<2¹¹⁸. Le comparateur réduit existant de Q4FamilySeed
ordonne les racines ; aucun produit naïf de deux fractions de degré huit
n'est réintroduit.

## Travail, mémoire et limites à mesurer

Le coût ne se limite pas au nombre de candidats. Publier préparation du
nuage/index/cover, décomposition du cover, tests de blocs/points, copies de
frontière, construction des cellules, requêtes, W=lectures d'actives,
localisations, clipping, tris, groupes, présentations et coquilles.
Le clipping réduit les événements triés, **pas W**. Beaucoup de formes
presque concourantes peuvent encore maintenir un carré dans une feuille.
Ni la profondeur fixe ni le budget du constructeur ne donnent une borne
sous-quadratique sur le total ou sur l'ensemble des arêtes.

Les pics de construction comptent les capacités de frontières parentales
et des enfants encore simultanément vivants, plus le tableau de nœuds et
la géométrie propre. Après construction, le pic additionne atlas conservé
et buffers privés, puis prend le maximum avec la construction : pas une
somme de deux pics non simultanés. Les objets fixes fragment/contrôle de
shared_ptr, métadonnées/transitoires d'allocateur, callbacks, stockage global
partagé et RSS restent hors de ces capacités ; ne pas présenter cette
mesure comme une borne de mémoire totale.

## Qualification et résultat

89 CTests Release, sept gates/vingt sondes Clang ASan/UBSan passent ;
la gate locale totalise11981 contrôles. Trois mutants compilés sont tués,
vingt différentiels contre27 conservent le chemin précédent. Les72 mesures,
lecteurs normaux/−O et37 corruptions du lecteur sont documentés dans les
[reçus propres](../receipts/q4_local_20260920/README.md). Sources177 et
builds r2 épinglés ; premières captures et mutant survivant r1 conservés
avec leurs sources historiques. Le renfort r2 touche gate/runner seulement.

Les denses sont réellement balayés à8k/16k/32k, K5/10. W vaut
5,311M/13,799M/15,791M sur le préfixe et0,913M/3,579M/14,974M
sur la grille permutée. Cette dernière fait×3,922/×4,184 ; ses tris
font×4,564/×4,857. Le gain absolu ne ferme donc **pas le carré**.
À32k/K10,981463 groupes rejetés pour profondeur ne donnent que30
présentations de15 boules : priorité aux décisions collectives entre
faces dans les régions potentiellement peu profondes, pas à une hausse
uniforme de profondeur ni au simple déplacement du double parcours GPU.
Listes de conflits, copies, dégénérescences et volume de coquille sont
à inclure avant toute borne sensible à K. La réduction de W peut être
distincte de celle des comparaisons après clipping.

Le temps local du seul chemin nouveau32k/K10 est environ1,10s préfixe,
1,15s permuté, un essai sous charge concurrente : aucun gain temporel
stable ni contrat de tour n'en découle. Les détails de préparation,
validation, stockage et sorties sont conservés dans chaque reçu.
Aucun contrat FULL/G4 ou sous-quadratique global acquis. s8/10/12
n'intervient pas dans cette primitive à arête fournie. GCP non utilisé.
