# P0 — première implémentation des crédits locaux

13 septembre 2026. `implementation_v8_p0 / cpu_reference /
quantized_u16_input_only / not_claimed`. Cette note décrit la première
brique, pas une tour HGP FULL livrée. Aucun résultat v7 n'est hérité.

## Ce que fait la brique

Elle reçoit **un rectangle déjà séparé** A×B, les coordonnées originales
et une liste facultative de sites proposés comme témoins extérieurs.
Elle produit des crédits certifiés pour chaque extrémité, puis une liste
compacte de sous-produits contenant les paires encore indécises. Elle
ne construit pas encore la WSPD, ne cherche pas toutes les boules, et
ne reconstruit pas encore les hiérarchies. Les paires restantes ne sont
donc ni les supports utiles définitifs ni la sortie FULL.

Le [code](../src/pipeline/local_credits.hpp) conserve la propriété du nuage
validé, l'ordre et les identifiants originaux. Les tableaux de crédits ne
sont pas une entrée que l'appelant pourrait falsifier. A et B sont deux
plages non vides, disjointes, de sites u16 distincts. Le validateur rejette
les doublons de coordonnées ; il ne les perturbe pas. Cette restriction
du premier module n'est pas une solution aux plateaux FULL ou aux profils
pondérés. Les coordonnées hors A∪B restent disponibles pour les propositions.

PreparedRectangle interdit explicitement copie, déplacement et affectation
de l'objet. Seuls ses pointeurs partagés sont copiables. Cette correction
ferme une contre-fixture de l'auditeur : un objet copié mutable pouvait
autrefois être réaffecté derrière un plan sans recalculer ses crédits.
La constance du pointeur seule n'était pas une preuve d'immuabilité.
La factory reçoit l'entrée par référence constante et copie les coordonnées
dans son stockage privé avant certification. Déplacer le vecteur ne suffit
pas : un appelant peut conserver un pointeur mutable vers son tampon.
Le test de cette seconde contre-fixture modifie une source toujours vivante
après préparation ; les plans et leurs points certifiés restent inchangés.
La copie O(n) est incluse dans le coût de préparation. Elle devra être
payée une seule fois par nuage, pas une fois par rectangle WSPD.

Le paramètre Kmax de cette interface couvre 1 à 10. Il fixe l'objet demandé,
pas un quota d'exploration. Une voie q est inactive si q>Kmax+1. Sinon,
son seuil est $h_q=K_{\max}+2-q$ ; les seuils à Kmax=10 sont donc 10, 9, 8.
La séparation contrôlée est
$\mathrm{gap}(A,B)\geq s\max(\mathrm{diam}(A),\mathrm{diam}(B))$, sur les
boîtes englobantes, avec s entier strictement positif. Cette convention
locale n'est pas une qualification de la construction WSPD v7 ou v8.

## Les décisions sont entières et strictes

Pour un site z, on pose $H=(z-a)\cdot(b-z)$ et
$\Xi=\Vert(z-a)\mathbin{\times}(b-z)\Vert^2$.
Le témoin q2 exige H>0 ; q3 exige aussi $3H^2>\Xi$ ; q4 exige
$2H^2>\Xi$. Une égalité n'est jamais un témoin intérieur.
Les produits sont calculés en entiers signés de 128 bits ; les différences
et produits non carrés intermédiaires tiennent en 64 bits sur u16.
La plus grande expression comparée est inférieure à $2^{73}$ en valeur
absolue, donc loin de la limite signée 128 bits. Aucun flottant ne décide.

Un crédit local pour a doit valoir pour **toute la boîte B**, pas pour
un seul b choisi. En q2, le minimum affine se calcule avec trois choix
de borne. En q3/q4, les huit coins suffisent par convexité du cône strict
en b, à a,z fixés. Le même argument, appliqué successivement en a et b,
certifie un témoin extérieur sur tout A×B. Une boîte trop large peut
empêcher un crédit pourtant valable sur les seuls sites ; ce défaut
d'efficacité ne devient jamais un rejet.

Chaque ID proposé pour le cœur doit être unique et hors A∪B. Les
propositions non témoins ne donnent aucun crédit. Le cœur h, les témoins
dans A privés de a, et les témoins dans B privés de b sont ainsi disjoints.
On élimine une paire uniquement si $h+h_a+h_b\geq h_q$.
Le module ne réemploie pas un compte hérité d'un rectangle parent.

## Trois méthodes à comparer

**Petit ensemble directionnel.** Une projection vers le centre de la
boîte opposée propose au plus besoin+1 sites du facteur. Chaque ancre
teste exactement ces propositions, en excluant son propre ID, et s'arrête
au besoin. Le choix par projection ne certifie rien : seuls les prédicats
géométriques donnent un crédit. La préparation du proposeur et les tests
sont O(h(|A|+|B|)) pour h borné par Kmax. Cela ne garantit pas un bon résidu.

**Parcours conjoint de blocs.** Un arbre spatial à coupes médianes de
l'intervalle géométrique représente chaque facteur. Une tâche associe un
bloc d'ancres U à un bloc de témoins Z, avec B fixe. Trois réponses existent :

- crédit : tous les sites de Z témoignent pour toutes les ancres de U et
  tous les b dans B ; créditer le cardinal de Z, saturé au besoin ;
- absence de crédit : un coin b0 fixe réfute le témoin pour tous les
  couples a,z de U×Z ; ne créditer personne, sans supprimer aucune ancre ;
- indécision : partager U ou Z et poursuivre ; aux feuilles, faire le
  test universel exact pour les deux sites distincts.

La borne positive minore H et majore Ξ sur trois boîtes. La borne négative
maximise H sur U×Z pour b0 fixé et, si nécessaire, minore Ξ. Elle ne confond
pas « aucun témoin commun » avec « aucun crédit par ancre », piège relevé
par l'auditeur complémentaire. Le maximum de H inclut le sommet demi-entier
de la parabole : comparer quatre fois ce maximum évite tout arrondi.

Les mises à jour d'un bloc d'ancres sont **différées dans l'arbre** : crédit
minimum et ajout en attente saturés au besoin. Un crédit collectif ne
parcourt pas toutes les ancres. La saturation commute avec les additions
positives et le minimum ; les enfants héritent des ajouts à leur visite.
Chaque partage décompose un produit en produits disjoints. Aucun témoin
n'est compté deux fois pour une ancre. H strictement positif interdit à
un bloc crédité de contenir le site de l'ancre elle-même.

Ce parcours donne les comptes universels-boîte exhaustifs **saturés**, mais
sans imposer une matrice quadratique initiale. Son pire cas de visites
reste potentiellement quadratique : ce n'est pas une preuve de préparation
sous-quadratique générale. Les compteurs et le résidu doivent décider de
son intérêt. L'arbre u16 a au plus 48 coupes le long d'un chemin, car chaque
coupe divise par deux une étendue entière positive ; c'est une propriété
du profil d'entrée, pas un arrêt de recherche. Aucun quota n'abandonne de tâche.

**Tubes et suffixes certifiés.** Cette troisième voie intègre la
[proposition de l'auditeur](../audits/P0_TUBES_ET_RANGS.md), sous sa propre
qualification C++. On transforme chaque site en projection entière
$t(p)=d\cdot p$ et coordonnées transverses $u(p)=d\times p$, avec
$d=2(c_B-c_A)$. Une grille de largeur fixée à quatre fois la plus grande
coordonnée absolue de d regroupe les points transverses. L'origine est le
minimum des coordonnées transformées : les divisions entières portent
sur des valeurs positives ou nulles, même si d est négatif.

Dans chaque cellule, Q majore la distance transverse au carré entre deux
sites. Après tri par cellule, projection et ID, un balayage monotone
compte les successeurs satisfaisant Δ>0 et, respectivement, $Q\leq9\Delta^2$,
$Q\leq\Delta^2$ ou $16Q\leq9\Delta^2$. Le suffixe n'est pas développé pour
le compter. Ces inégalités larges donnent néanmoins des témoins stricts,
grâce à la marge démontrée sous D≥10R. Les deux paramètres désignent la
distance des centres et le plus grand rayon de boîte. Le test entier est
**d²≥100 diam²**, et non 25 diam² : cette erreur de conversion a été
repérée en contrelecture avant le codage, et reçoit une fixture permanente.
Si le lemme de séparation ne s'applique pas, le module retourne des crédits
nuls, donc garde toutes les paires indécises ; il n'applique pas le cône
hors de ses hypothèses. La précondition s≥8 de nos campagnes suffit au lemme.

Les produits des tubes sont inférieurs à $2^{74}$ en valeur absolue et
sont élargis en i128 avant multiplication. Le tri donne O(m log m) et le
balayage O(m), mémoire O(m), pour un facteur de m sites. Ce premier raccord
calcule une voie par plan : **il refait le tri pour chaque voie**. Partager
ce travail entre q2/q3/q4 est une optimisation suivante, pas un gain déjà
implémenté. Des cellules trop fines ou trop larges peuvent laisser un
mauvais résidu ; cette méthode n'a donc pas la garantie de retrouver tous
les crédits que donne DualBlocks. Ne pas additionner les crédits issus
de méthodes ou grilles différentes : les sites témoins peuvent se recouvrir.

## Ne pas déplacer le carré vers une étape cachée

La validation de l'unicité coûte O(n log n) dans ce premier adaptateur qui
possède le nuage entier. Elle est mesurée à part et devra être mutualisée
quand un nuage alimentera plusieurs rectangles. Les arbres sont également
locaux à une invocation : aucun coût global de WSPD n'est encore évalué.

Les plages de cette première API sont contiguës dans l'ordre d'entrée.
Un vrai raccord WSPD devra représenter ses facteurs dans une permutation
spatiale partagée et conserver explicitement les IDs originaux, sans
recopier le nuage pour rendre chaque facteur contigu. Ne pas appeler
« identité originale » un simple rang Morton.

Le compte du cœur ne conserve pas encore les IDs certifiés dans la sortie.
Il est valable pour le plan courant, mais **ne peut pas être additionné
à un nouveau census extérieur** susceptible de revoir ces sites. Le
consommateur devra repartir de zéro ou transporter les identités et une
preuve de disjonction. Cette consigne de l'auditeur vise le futur raccord.

Les crédits sont regroupés par valeur avec un tri par comptage linéaire.
On ne développe que les couples de classes de somme inférieure au besoin,
soit au plus O(h²) descripteurs. Le nombre M de paires restantes est une
somme de produits de cardinaux, contrôlée contre les dépassements u64.
L'énumération optionnelle par rappel coûte réellement O(M) : son absence
du chronomètre du plan est explicite, pas un gain sur le traitement aval.

Des nappes ou plusieurs rails peuvent laisser beaucoup de paires malgré
une préparation courte. Un petit pool global ne couvre pas nécessairement
les témoins utiles à toutes les ancres. Comparer ensuite des groupes
directionnels/tubes et leur coût complet, sans annoncer P0 fermée sur un
seul amas favorable. Les compteurs de prédicats ne sont pas des opérations
homogènes : les tests de coins font partie des tests de points ; ne pas
additionner ces champs comme des opérations indépendantes.

## Qualification de cette tranche

L'oracle borné utilise des entiers multiprécision et énumère les coins et
témoins, sans appeler les prédicats produits pour juger les décisions.
Il vérifie les frontières, les grandes coordonnées, les crédits exacts
saturés du parcours, les minorants du pool, les rejets d'entrées, les IDs,
les résidus et l'expansion sans doublons. Il juge **Wq et les crédits de
boîte**, pas la complétude du producteur de supports ou des parents FULL.

La [sonde](../bench/P0_PROBE.md) mesure séparément génération, validation
et construction du plan, puis rapporte M sans le développer. Comparer
n=8 000, 16 000 et 32 000 en mono, avant de choisir la méthode ou de passer
au multi-CPU. Tester s8/10/12 sur le même rectangle vérifie uniquement la
précondition : le vrai compromis WSPD reste à mesurer sur une décomposition
effective. Aucun temps de cette brique ne qualifie le contrat 50k/1s.
