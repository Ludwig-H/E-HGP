# Audit v8 → v9 : q4 par boules minimales locales certifiées

22 septembre 2026. Moteur v8 lu à `a74e90f2`, ouverture v9 `3595725a` ;
premier moteur v9 `d2700314` relu ensuite. Audit mathématique et
architectural ; les six lignes FULL locales du
[premier reçu](../receipts/first_tower_20260922/README.md) donnent une base de
coût relative aux clés émises, sans qualification du contrat G4. Le
[jalon de temps v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md)
vise d'abord les trames LiDAR entières sans sol en u18/1 mm. La trame brute
entière reste une obligation distincte, dont la portée temporelle v9 est à
préciser ; float32 reste le défaut d'entrée antérieur, son développement
temporel v9 étant suspendu. La cible est la tour complète K=1..10 en moins
d'une seconde sur
GCP G4, avec repli K=1..5 puis objectif 100 ms.

## Diagnostic qui doit guider le choix

La v8 a acquis plusieurs objets exacts utiles : balayage d'une famille q4
au lieu d'un census par tétraèdre, [fragments exacts de cellules](../../morsehgp3D_v8/docs/Q4_FRAGMENTS_ET_BALAYAGES_LOCAUX_20260920.md),
[fenêtre fermée de faible profondeur](../../morsehgp3D_v8/docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md),
[jointure graines × cellules](../../morsehgp3D_v8/docs/Q4_GRAINES_ET_CELLULES_20260921.md)
et [rejet q3 par l'atlas](../../morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md).
Les tests de contact, de propriété et les registres de coûts sont des acquis
à conserver. La fenêtre30 prouve déjà qu'à seuil fixe il ne faut trier que
O(K) événements intérieurs par famille ; elle paie encore un ou deux scans
des témoins retenus **par famille**. Réinventer cette fenêtre serait une
micro-variante sans réponse au verrou.

La [reprise u18 inventoriée dans l’ouverture v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md)
mesure, sur **une** trame 08/000000 sans sol entière à 1 mm, 39 885 sites,
K5/s8/huit workers, 104,63 s mur et 812,82 CPU·s pour q3/q4 seulement :
3,252 milliards de bornes de blocs, 7,316 milliards de tests ponctuels,
5,547 milliards d'IDs de frontière copiés dans l'atlas q4 ; 163,678
millions de comparaisons de balayage. Les 691 284 supports q3 et 158 496
q4 émis ne sont ni catalogue ni FULL. L'option `saturate_deep` était
désactivée. Cette unique ligne ne mesure ni sa valeur ni la croissance.
L'arrêt anticipé à K−1 est néanmoins un certificat exact à conserver :
il n'a pas besoin de finir un fragment déjà rejeté.

Sur l'ancien profil u16, LiveOnly réduit fortement les visites d'atlas,
mais la préparation garde des sous-postes au-delà de ×4 au doublement et
Joined crée 57,242 millions d'entrées de cache à 32k/K5. Les couches
duales29 peuvent garder tous les sites ; la fenêtre30 laisse alors le
produit graines × témoins. Les [mesures spatiales](../../morsehgp3D_v8/docs/Q34_MESURES_SPATIALES_20260921.md)
sur trois trames brutes entières ont donné 165,214/34,319/505,479 s en CPU
G4 W48 pour le flux q3/q4, avec occupation moyenne 4,19/11,13/1,93 CPU
logiques. Ces nombres ne sont pas des mesures GPU et ne portent pas FULL.
Un partage de file ne peut pas, à lui seul, effacer les milliards de tests.

### Certifier les graines **avant** de partitionner les témoins Z

La [piste v8 graines × cellules](../../morsehgp3D_v8/docs/Q4_BLOCS_SEEDS_PISTE_20260921.md)
propose un rejet exact de produits `X×C`, mais le moteur construit aujourd'hui
le fragment Z de chaque cellule dans `Q4LocalAtlas::Impl::build` **avant** que
`Q4SeedCellEngine::joined` consulte ces produits. Elle ne peut donc pas
économiser la construction de l'atlas, poste dominant du reçu 1 mm. Une
expérience v9 distincte consiste à tester, sur la cellule fermée **avant**
`Q4LocalFragment::child`, si une droite de graine peut la traverser. Pour un
bloc de graines possibles X, les bornes existantes de `node_bounds(X,C)`
autorisent le rejet seulement si `min L_x(C)>0` pour **tous** les x de X ou
`max L_x(C)<0` pour tous ; un zéro, même sur un coin ou côté, reste actif.
Si tous les blocs couvrant les graines possibles sont ainsi exclus, aucun
centre de support q3 aigu propriétaire ou q4 strictement positif propriétaire
de cette arête ne se trouve dans C : la partition Z de C peut être omise.
Les sites de X demeurent témoins des autres cellules.

Le [reçu 1 mm désormais versionné](../../morsehgp3D_v8/receipts/u18_resume_20260922/README.md)
donne `1 133 440` arêtes q4 sans feuille vivante sur `1 872 168`
requêtes (60,5 %), constat fait **après** le coût de l'atlas. Sur l'ensemble
des `1 872 168` arêtes q4, `1 545 198` portent aussi q3 et réutilisent l'atlas
pour ses rejets ; il ne reste que `326 970` arêtes q4 seules. Le nombre
de `whole_atlas_skips` n'est donc **pas** le nombre d'atlas économisables
par le filtre proposé : les rejets profonds par Z peuvent en être la
cause, et l'intersection avec q3 n'est pas publiée. Instrumenter une table
par masque `q4_seul/q3+q4` × cause `Outside/Deep/aucune_droite/Leaf`
avant d'attribuer un gain au préfiltre.

Une porte moins coûteuse à tester est **l'existence d'une seule graine**
`abx` strictement aiguë et possédée par `ab`, après le filtre de témoins
de paire mais **avant** `Q34EdgeCover::make`. Les prédicats entiers
`spatial_pass`/`seed_pass` de `q4_local.cpp:595–629` fournissent la base
d'un parcours d'index avec arrêt au premier témoin, sans atlas ni liste
de graines. Si aucune graine n'existe, la voie q3 ne peut rien émettre ;
la voie q4 non plus, par le lemme de la complétion aiguë ci-dessous. Le
cover partagé peut alors être omis pour cette arête. Si une graine existe,
le prétest est du travail supplémentaire : le mesurer en mode *shadow*
sur un échantillon stratifié avant d'en faire un défaut. Croiser masques
q3/q4, absence de graine, absence de feuille q4, coût du prétest et
`440 194 038` visites de construction de cover du reçu 1 mm. La
[contrelecture B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) montre aussi
qu'un rejet `NoQ4Seed` **dans** l'atlas doit garder q3 et ses certificats
Z, ou payer leur repli : `153 036 427` graines q3 furent rejetées grâce
à cet atlas dans le reçu. Le filtre `X×C` ne doit pas matérialiser comme
entrée les seules `9 550 974` graines q4 comptées **après** atlas.

Pour q4, la condition « graine possible » est justifiée : si `ab` est
l'arête propriétaire maximale d'un tétraèdre **strictement positif**, au
moins une de ses deux complétions est hors de la boule diamétrale fermée de
`ab`. Sinon cette boule enfermerait les quatre sommets et leur miniball
serait portée par `ab`, incompatible avec quatre poids barycentriques
strictement positifs. Cette complétion forme un triangle `abx` aigu puisque
`ab` est maximale. Il ne faut surtout pas exiger que les **deux** le soient.
Fixture u18 entière : `a=(0,50,0)`, `b=(200,50,0)`,
`x=(100,149,0)`, `y=(100,0,100)` ; `|ab|²=40000` est maximal,
`(x−a)·(x−b)=−199`, `(y−a)·(y−b)=2500`, et le centre q4 exact est
`(100,9701/198,4751/396)`. Ses poids barycentriques dans l'ordre
`a,b,x,y` sont `3252301/7840800` pour chacun des deux premiers,
`3955/78408` et `4751/39600` : tous sont positifs. `ab` échoue q2,
mais q4 survit via `y` ; le domaine et les témoins doivent garder `x`.
Pour q3, chaque triangle aigu propriétaire a directement sa droite
`L_x=0`. L'arête avec une seule complétion reste un cas q3 autonome :
`(0,0,0),(4,0,0),(2,3,0)` donne une graine q3 aiguë alors que la racine
q4 est `Outside`. Aucun rejet pré-atlas ne peut supprimer cette voie.

Ce déplacement du filtre ne gagne du temps que s'il évite plus de bornes Z,
tests ponctuels et copies d'IDs qu'il ne paie de tests `X×C`. Avant de
modifier le constructeur, instrumenter des arêtes lourdes puis des trames
entières : cellules éliminées **avant Z**, visites et bornes X×C, travail Z
évitable, contacts conservés, temps total et sorties exactes q3/q4/FULL.
Recalculer les largeurs arithmétiques du filtre pour `M=262143` et le
`Q=2^20` u18 actuel : celles de la note v8 supposaient u16 et `Q=2^44`.
La preuve locale ne borne ni le nombre de produits X×C ni le coût global.

## Objet mathématique à énumérer

Pour un tétraèdre strictement positif de sommets `a,b,x,y`, centre `c` et
rayon `R`, les quatre poids barycentriques `λ_i` de `c` sont strictement
positifs. Sa circumboule est aussi la **boule minimale englobante** de ces
quatre sites : pour tout centre `d` d'une autre boule les contenant,

`Σ_i λ_i |p_i−d|² = R² + |c−d|²`.

Le rayon de cette autre boule est donc au moins `R`. C'est la base d'une
énumération locale de *boules minimales k-Gabriel* ; cela ne réduit pas à
lui seul le nombre de quadruples. Pour q4, une boule de profondeur stricte
`d` appartient à la voie de niveau K seulement si `d < T4=K−2` ; le même
`d` peut alimenter plusieurs K sans régénérer la boule. La clé canonique
globale et le plus petit support positif `q_min` restent à déterminer après
regroupement. Les voies q2/q3/q4 sont indépendantes : les
[contre-fixtures](../../morsehgp3D_v8/docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md)
ont déjà un q4 admissible dont l'arête q2 ou la face q3 est rejetée.

Pour n'importe quel support `a` de la boule, la puissance d'un site `z`
s'écrit

`|z−c|²−|a−c|² = |z|²−|a|²−2(z−a)·c`.

Chaque site définit donc un demi-espace **affine en centre**. La profondeur
est le nombre de sites strictement plus proches de `c` que `a`, autrement
dit le rang strict de `a` dans l'ordre de Voronoï. Cette identité peut
partager des certificats entre arêtes et voies, mais construire toutes les
étoiles de Voronoï d'ordre K ou toute la tétraédralisation de Delaunay3D
serait un engagement beaucoup plus coûteux que nécessaire. La complexité
de Delaunay3D peut déjà être quadratique pour K=0 ; ce fait ne donne pas une
borne de sortie pour la sous-famille positive du modèle HGP
([Erickson, construction de référence](https://arxiv.org/abs/cs/0103017)).
Les cellules C proposées ci-dessous sont des **boîtes de calcul créées à la
demande**, et non les cellules combinatoires de ce diagramme.

### Une borne linéaire pour les centres q4 peu profonds d'une arête

Fixer une arête `ab` avec son `Q34EdgeCover` **complet pour les centres
admissibles**, puis grouper les formes non constantes `L_z=0` du cover
par **droite géométrique** ; soit `m` le nombre de droites distinctes.
Cette complétude a une marge stricte pour q4 positif. Si `l=|ab|` est
la plus longue arête du tétraèdre positif et `λ_i>0` ses poids au centre,
`R²=Σ_{i<j}λ_iλ_j|p_i−p_j|²≤(3/8)l²` par l'identité de variance.
Pour le milieu `m_ab`, `|c−m_ab|²=R²−l²/4≤l²/8` ; chaque site de
l'intérieur **ou de la coquille** satisfait donc
`|z−m_ab|≤R+|c−m_ab|≤(√3+1)l/(2√2)<l`. La boule fermée de rayon
`l` du `Q34EdgeCover` les contient tous, sans hypothèse d'alignement
du LiDAR. Cette propriété n'autorise pas à écarter les sites du cover
qui pourraient être des témoins d'autres centres.
Les formes toujours négatives ajoutent `p₀` au compte, celles toujours
nulles sont des contacts et les positives ne contribuent pas. Un centre
q4 strictement positif appartient à l'intersection de deux droites
indépendantes. À la tour K, les centres conservés ont au plus
`d=K−3−p₀` autres intérieurs stricts ; si `d<0`, il n'y en a aucun.

Sur une droite `λ`, prendre les deux intersections extrêmes de profondeur
au plus `d`. Toute autre droite qui coupe `λ` entre elles a un demi-plan
strictement négatif contenant au moins une extrémité. Il y a donc au
plus `2d` croisements intérieurs distincts, et au plus `2d+2`
intersections peu profondes sur `λ`. Un sommet appartient à au moins
deux droites : le nombre de **centres distincts** peu profonds est donc
au plus `m(d+1)≤m(K−2)`. La [borne publiée pour les demi-plans en
position générale](https://www.math.tau.ac.il/~michas/k_depth.pdf)
(Har-Peled et Sharir, 2016, lemme 2.5) suit ce comptage ; ici les
parallèles, concurrences et droites coïncidentes se traitent en comptant
les croisements distincts et la multiplicité des sites dans la
profondeur. Les contacts restent de profondeur zéro sur leur droite.

On peut réduire les **droites parcourues** sans perdre la deuxième
complétion obtuse : soit `s` le nombre de droites distinctes portant au
moins une graine `abx` aiguë et possédée. Toute q4 positive de propriétaire
`ab` en possède au moins une. Il suffit d'énumérer les sommets peu
profonds **sur ces s droites**, mais de les croiser avec **toutes les m
droites** du cover. Avant dédoublonnage, cela donne le plafond local
`s(2d+2)` en plus du plafond symétrique `m(d+1)`. Si `s=0`, l'arête est
rejetable avant atlas ; restreindre également les croisements aux seules
graines aiguës serait faux. Il s'agit de graines **géométriquement
possibles avant le census q3**, non des triangles q3 acceptés : la
[fixture B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) possède une q4
admise alors que ses quatre faces q3 sont rejetées.

**Sélection sans arrangement quadratique sur une droite.** Paramétrer une
droite de graine `λ` par `t` croissant. Chaque forme du cover y devient
`f_z(t)=α_z t+β_z`. Les constantes négatives ajoutent `p_λ` à la
profondeur ; les constantes nulles restent des contacts. Pour `α_z≠0`,
l'événement exact est `τ_z=−β_z/α_z`. À un événement `t₀`, les intérieurs
variables sont exactement les formes `α_z>0, τ_z>t₀` et
`α_z<0, τ_z<t₀`, avec multiplicité de sites, tandis que les égalités
restent sur la coquille. Poser `d_λ=K−3−p_λ` ; si `d_λ<0`, aucune boule q4
utile ne se trouve sur cette droite. Si un événement de profondeur
au plus `d_λ` porte une pente positive, il appartient nécessairement aux
`d_λ+1` **plus grands τ distincts** des pentes positives ; sinon il porte
une pente négative et appartient aux `d_λ+1` **plus petits τ distincts**
des pentes négatives. En effet, `d_λ+1` événements plus lointains du bon
signe imposeraient déjà plus de `d_λ` sites intérieurs. Il suffit donc de
retenir au plus `2d_λ+2` valeurs candidates, puis de recalculer leur
profondeur exacte et leurs contacts. Les valeurs égales sont groupées,
mais leurs sites sont tous comptés ; cette preuve accepte concurrences,
droites coïncidentes et signes opposés.

Normaliser d'abord les formes en clés primitives de droite, avec deux
orientations et leurs listes d'IDs : ce prétraitement sur les `h` sites
du cover coûte au moins `Ω(h)` et typiquement `O(h log h)` par tri exact,
ou `O(h)` attendu par hachage, avec mémoire `O(h)`.
Après ce regroupement exact des `m` droites et de leurs multiplicités, des
buffers top/bottom de taille `O(K)` sélectionnent ces valeurs en
`O(Km)` comparaisons rationnelles par droite de graine ; les `O(K)` scans
de profondeur coûtent aussi `O(Km)`. La largeur du comparateur est
bornable dans le domaine de `Q4LocalGeometry::form` : ses coefficients
vérifient `|constant|<2^40`, `|x|,|y|<2^39`. Pour la droite de graine
`c₀+x₀u+y₀v=0` avec `y₀≠0`, prendre `t=u`. La forme restreinte vaut
`(α_z t+β_z)/y₀`, où `α_z=x_z y₀−y_z x₀` et
`β_z=c_z y₀−y_z c₀` ; `|α_z|<2^79`, `|β_z|<2^80`.
Le produit croisé direct de deux racines pourrait occuper 160 bits,
mais son signe se **factorise**. Pour les formes `F₀=(c₀,x₀,y₀)`,
`F_z=(c_z,x_z,y_z)` et `F_w`, poser
`D=det(F₀,F_z,F_w)`. Alors
`β_z α_w−β_w α_z = y₀ D`. Chaque terme du déterminant vaut au plus
`960M⁶` et `|D|≤5760M⁶<2^121` pour `M=262143` : **i128 signé suffit**
pour le calcul et la comparaison des racines. Le comparateur prend le
signe de `−y₀D/(α_z α_w)` sans former le produit `y₀D`.
Si `y₀=0`, paramétrer par `t=v`, échanger les axes et utiliser
`β_z α_w−β_w α_z=−x₀D`. Le signe de pente est celui de
`α_z/y₀` (ou `α_z/x₀` dans l'autre branche), pas de `α_z` seul
lorsque le pivot est négatif. Les groupes de racines égales et le cas `α_z=0` doivent être
traités explicitement. Une même droite géométrique peut porter des
formes proportionnelles **de signes opposés** : conserver les IDs et
multiplicités des deux orientations, plutôt qu'un seul représentant
signé. Les buffers top/bottom peuvent s'arrêter avant `d_λ+1` racines
si le **poids cumulé** des racines plus lointaines dépasse déjà `d_λ` ;
un groupe concurrent reste un événement unique et ses contacts ne se
comptent pas comme intérieurs à son propre sommet. Sur `s` droites de graines,
le coût pessimiste demeure `O(sKm)` plus regroupement, census, MEB et
catalogue : aucun sous-quadratique global n'en découle si `s≈m`.
Cette route évite toutefois le tri `m log m` sur chaque droite et la
construction explicite de toutes les intersections de l'arrangement.
Elle examine encore les `m` droites pour chacune des `s` graines. Mesurer
`m`, `s`,
événements retenus, contacts, centres positifs, coûts de sélection et de
census sur les arêtes LiDAR avant d'en faire un défaut. L'[oracle de
sélection](check_q4_shallow_lines_20260922.py) confronte les valeurs
retenues à toutes les intersections sur **7 210 cas** exacts, avec
coïncidences et signes opposés ; elle contrôle 500 restrictions de formes
u18 et un sommet propriétaire aigu qui n'est pas une miniballe. Les
deux modes Python normal et `-O` doivent rendre le même PASS.

### Si `s≈m` : deux familles de niveaux peu profonds

Le balayage par droite de graine reste `O(sKm)` dans ce cas. Une réduction
combinatoire plus forte sépare les formes non constantes
`f_z(u,v)=c_z+x_z u+y_z v` : pour `y_z>0`, leur intérieur est
`v<r_z(u)` ; pour `y_z<0`, il est `v>r_z(u)`, avec
`r_z(u)=−(c_z+x_z u)/y_z`. Après les `p₀` constantes négatives,
poser `d=K−3−p₀` et `q=d+1`. À un sommet de profondeur au plus d,
chaque droite de contact `y>0` est parmi les **q plus hautes** racines
de sa famille à cette abscisse, chaque droite `y<0` parmi les **q plus
basses** de l'autre, en conservant **tous les ex æquo** : toute racine strictement au-delà est déjà un
site intérieur. Ignorer les multiplicités pour cette présélection donne
un surensemble sûr ; les poids et IDs reviennent à l'exactification.
Si deux droites indépendantes de contact appartiennent à la même
famille, leur sommet est un événement de ses premiers q niveaux ; sinon
c'est un croisement des deux familles. Les concurrences se groupent au
même sommet exact, sans traiter les lignes coïncidentes de signes opposés
comme deux directions indépendantes.

Les droites verticales `y=0` se traitent par leur abscisse `u=τ` : leur
profondeur verticale stricte se calcule par préfixes/suffixes des deux
orientations. Au plus `2d+2` abscisses de droites verticales peuvent
avoir ce compte au plus d : chaque abscisse entre les deux extrêmes
éligibles rend au moins une forme strictement négative à l'un des
extrêmes. Pour chacune, les contacts non verticaux
utiles sont parmi les q racines extrêmes de leur famille. En position
générale, les premiers q niveaux de **chaque famille homogène** ont
`O(mq)` morceaux et une construction `O(m log m+mq)` est connue
([Everett–Robert–van Kreveld](https://doi.org/10.1142/S0218195996000186)).
Deux morceaux sélectionnés de familles opposées ne se croisent qu'à
profondeur **non pondérée au plus `2d` dans le sous-arrangement non
vertical** ; les formes `y=0` peuvent ensuite rendre leur profondeur
totale arbitrairement grande. Le lemme des demi-plans borne donc les
intersections indépendantes de cette présélection à `O(m(d+1))`
([Har-Peled–Sharir, lemme 2.5](https://www.math.tau.ac.il/~michas/k_depth.pdf)).
Cela indique une route locale quasi linéaire en `m` pour K5/K10 fixé ; **ce
n'est pas encore un algorithme v9 ni une borne du pipeline**. Une première
version peut construire les niveaux **sans poids** comme surensemble,
puis compter les multiplicité/IDs aux seuls centres retenus : les poids
positifs ne peuvent que relever la profondeur. Le port doit néanmoins
traiter parallèles, concurrences, contacts isolés et recouvrements de
droites opposées, puis prouver la largeur de
ses comparateurs exacts. Le traitement symbolique des dégénérescences
de niveaux homogènes ([Halperin et al., §4](https://sarielhp.org/p/20/max_level/max_level.pdf))
ne se transfère **pas** tel quel aux deux familles orientées. Une
perturbation générique peut même effacer un sommet exact de profondeur
zéro : pour `2(d+1)` formes `±n_i·(u,v)` concourantes à l'origine,
remplacer chacune par `±n_i·(u,v)−ε` crée au moins `d+1` intérieurs
partout. Il faut grouper les concurrences exactement, ou démontrer une
perturbation orientée qui conserve tous les sommets à rabattre. La
construction effective de ces niveaux reste à qualifier.

Une **perturbation sortante** donne toutefois un protocole de couverture
plus précis. Grouper d'abord les formes par droite *orientée* primitive,
avec poids et IDs (`g≤2m`) ; conserver à part les formes constantes
négatives et celles identiquement nulles. Pour chaque groupe non
vertical, poser symboliquement `f_i*=f_i+ε+ε^{i+2}` pour des indices
distincts, **sans changer la pente**. Les verticales reçoivent le même
décalage de constante et restent dans leur branche d'abscisses.
Le terme commun `+ε` domine : à un sommet original `v` de profondeur
`p≤d`, tous ses contacts sont du côté **non intérieur**. Les contacts
originaux contiennent deux normales indépendantes. L'intersection
`P*=∩_{i contact}{f_i*≥0}` contient `v` strictement et est pointée.
Minimiser sur `P*` la somme de deux formes **perturbées** dont les
normales originales sont indépendantes :
la valeur au point `v` est `O(ε)` et les deux formes sont non négatives.
Leur sous-niveau borné contient donc un sommet `v*=v+O(ε)` ; après
inversion de la matrice `2×2`, aucun contact n'est négatif en `v*` et les
autres signes sont stables. Sa profondeur perturbée pondérée est `p`,
donc sa profondeur **non pondérée** est au plus d. Les termes d'ordres
distincts cassent toute concurrence de trois droites non parallèles ;
les parallèles restent parallèles. Chaque paire active au sommet `v*`
est donc indépendante **aussi avant perturbation**, et son intersection
originale est exactement `v`. Le catalogue des niveaux peu profonds
doit proposer ce sommet, puis la clé rationnelle originale est
recertifiée. Une perturbation `−ε` échoue déjà pour `±u,±v` à l'origine,
profondeur zéro devenue au moins deux partout.
L'[oracle rationnel autonome](check_q4_outward_levels_20260922.py) compare
les signes des polynômes en ε sans valeur flottante : 2 004 cas,
5 926 centres exacts peu profonds et 7 351 sommets perturbés, avec
parallèles, concurrences, orientations opposées et multiplicités. Les
exécutions normales et `-O` donnent le même PASS. Cela contrôle le lemme
local, sans tester le constructeur de niveaux ni le moteur v9.

**Ordre de possession des cellules.** La fixture u18 ajoutée à cet
[oracle](check_q4_outward_levels_20260922.py) donne cinq contacts
`a=(0,6,3)`, `b=(16,6,3)`, `x=(2,0,7)`, `y=(7,12,12)`, `z=(2,5,0)`
sur la sphère de centre `(8,6,6)`, de rayon carré 73. `ab` est strictement
maximale dans le tétraèdre positif `abxy` ; la coquille a cinq sites et
`q_min=4`. Dans le plan local de `ab`, le centre original est
`(α,β)=(0,3/8)`, coin de la cellule dyadique droite/haute
`[0,1/32]×[3/8,13/32]`. Les trois formes primitives des autres contacts
sont `(3,12,−8)`, `(9,−16,−24)`, `(−9,8,24)`. Sous la perturbation
sortante, les trois sommets ont pour dérives de premier ordre
`(−1/26,7/104)`, `(−1/11,−1/88)`, `(1/4,−1/8)` fois `ε` : chacun
sort de la cellule propriétaire, bien que les trois soient de profondeur
zéro et se rabattent sur son coin. Un port qui filtre les **sommets
perturbés** par la cellule avant de calculer l'intersection originale
perd cette vraie clé q4. Calculer d'abord le centre rationnel original,
dédoublonner, puis appliquer la possession de cellule et les tests exacts.
L'oracle normal et `-O` passe 2 004 cas plus cette fixture ; il ne qualifie
pas encore un constructeur de niveaux.

Le dédoublonnage **précède** les tests coûteux : chaque sommet perturbé
donne une paire de droites originales indépendantes. On normalise
exactement leur intersection
`(N_u/D,N_v/D)`, `D>0`, PGCD commun, et on trie/hache la clé. Sous les
bornes u18 des coefficients ci-dessus, `|N_u|,|N_v|<2^80`, `|D|<2^79` ;
comparer deux clés par produits croisés peut demander environ 160 bits,
donc une voie entière 256 bits est une cible prudente à qualifier.
Plusieurs sommets perturbés d'une concurrence se rabattent sur **une**
seule clé. Après unicité, un arbre binaire des groupes de racines
`y>0` (enveloppe supérieure par nœud) et `y<0` (inférieure) peut
rapporter les stricts intérieurs par priorité exacte, arrêter à
`d+1` poids négatif, puis, seulement si admis, rendre **tous** les
contacts égaux, y compris ceux invisibles dans les niveaux perturbés.
Les verticales se joignent par préfixes/suffixes et groupe d'abscisse ;
les formes identiquement nulles par leur liste d'IDs. Préparer les
enveloppes vise `O(m log²m)` temps et `O(m log m)` mémoire ; interroger
un centre vise `O((K+t)log²m)` pour `t` contacts. Avec `O(mK)` sommets
perturbés et les clés dédoublonnées, la cible **locale conditionnelle**
est `O(h log h+m log²m+mK log(mK)+mK²log²m+Ilog²m)` hors sorties,
où `I` est le nombre d'incidences de coquille distinctes. La borne de
deux extrêmes donne
`I≤(2d+2)h` pour les centres effectivement admis, hors endpoints
permanents. Ce compte suppose un constructeur exact de niveaux et des
comparateurs symboliques à coût borné, encore à concevoir et à juger :
ce n'est **ni** une borne du générateur entier **ni** un résultat GPU.
Les parallèles sont explicitement admises dans la position générale de
[Halperin–Har-Peled–Mehlhorn–Oh–Sharir, annexe A](https://sarielhp.org/p/20/max_level/max_level.pdf),
qui donne `O(m log m+mK)` pour les premiers niveaux homogènes. Le lemme
de profondeur cité plus haut suppose une position générale plus forte ;
la borne utilisée ici pour les parallèles suit directement du comptage
sur chaque droite, sans invoquer ce transfert. Une préparation de
`O(m log m)` **par arête** serait ruineuse sur les millions de petites
arêtes : déclencher cette voie seulement quand `s≈m` et `m` est grand,
puis mesurer la somme des coûts et la mémoire simultanée.
L'arête est une tâche indépendante, ses deux familles peuvent être
préparées séparément ; les arêtes lourdes peuvent distribuer l'overlay
par intervalles d'abscisse avec dédoublonnage rationnel aux frontières.

Le lemme des deux extrêmes donne au plus `2d+2` sommets peu
profonds par droite géométrique, donc `O(h(d+1))` incidences explicites
de sites de coquille pour h formes du cover, hors les deux endpoints
identiquement nuls. Cela concerne le **catalogue de centres**, pas le
flux potentiellement bien plus gros de tous les supports cosphériques.
Les niveaux ne certifient pas non plus l'**existence d'un tétraèdre
strictement positif propriétaire** sur une coquille : la paire de
droites ayant révélé le centre peut échouer alors qu'une autre paire
réussit. Pour reproduire le **flux des présentations q4** et ses
incidences, il faut un oracle d'existence sur la coquille, compatible
avec la plus longue arête, ou garder le balayage actuel comme repli ;
énumérer toutes les paires de contacts recréerait le carré sur un plateau.
Pour un **catalogue de boules canoniques** seulement, la condition
`c∈conv(U)` après census complet est plus puissante : elle certifie la
miniballe de U, et Carathéodory donne un support positif minimal d'arité
`q_min≤4`, éventuellement sur une autre arête. Le FULL porté consomme
clé, niveau, I/U et `q_min`, pas le flux brut des supports. Le producteur
de centres q4 peut ne publier que les clés de `q_min=4`, qui ont alors
un tétraèdre strictement positif sur U, en laissant les `q_min=2,3`
aux voies q2/q3 complètes ; cette voie
peut donc être correcte sans inventaire q4, **si** le quotient local
reconstruit toutes les incidences requises et si la complétude des clés
est prouvée. La voie FULL actuelle refuse explicitement `|U|>12` et
son quotient emploie `2^|U|` masques : l'argument de catalogue ne lève
pas ce verrou de représentation ; il exige le quotient compact et sa
preuve séparés. La garde actuelle `chain_qmin_differs_from_min_presented_arity`
devrait alors être remplacée par une certification directe de `q_min`,
jugée contre les petits inventaires exhaustifs. Les exemples
octaèdre/cube ci-dessous distinguent ces deux contrats.
Pour gagner le poste q4 dominant, cette route doit éviter la construction
des fragments Z de l'atlas, tout en conservant ou remplaçant ses rejets
q3 sur les arêtes communes ; une insertion **après** l'atlas ne suffirait
pas. Mesurer d'abord `m,s,h`, événements et contacts par arête lourde,
prétraitement des niveaux, scans évités et sorties FULL identiques.
Sur les 1,87 million d'arêtes q4 du seul reçu 1 mm, même un
`O(m log m)` **par arête** peut coûter trop cher : une sélection adaptative
des arêtes lourdes doit comparer ce coût complet à l'atlas courant.
La somme `Σ_e m_e log m_e` et le front WSPD restent à borner ou mesurer ;
aucun sous-quadratique global n'est acquis par le lemme local.

**Partage prudent avec q3.** Pour une graine aiguë propriétaire `abx`,
`D=|ab|²` et le circumrayon de sa face vérifie `R₃²≤D/3` ; le centre
`c₃` est à distance carrée au plus `D/12` du milieu de `ab`. Tout site
intérieur ou contact de sa boule satisfait donc
`|z−milieu(ab)|≤√(D/3)+√(D/12)=√(3D/4)<√D` : le
`Q34EdgeCover` de rayon `√D` contient **tout** son intérieur et sa
coquille. Si le passage q4 doit déjà parcourir les groupes de formes
pour cette droite, il peut simultanément établir le census q3 exact à
`c₃`, par un seul `ExactBall::power` sur un représentant de chaque
orientation et les multiplicités/IDs du groupe. Les formes nulles sur
tout le plan sont des contacts permanents. Cela ne supprime pas à
priori la consultation de l'atlas q3, qui rejette beaucoup de graines
avant tout census, ni ne prouve un gain : comparer le scan partagé au
census q3 par boîtes sur les arêtes où les deux voies vivent. Ne pas
conditionner q3 à une feuille, un domaine positif ou un événement q4 :
sa voie autonome reste nécessaire quand q4 est écarté. L'évaluation
naïve de `Q4LocalForm` au centre rationnel q3 peut atteindre environ
158 bits ; `ExactBall::power`
reste dans son domaine u18 certifié.
Avant port actif du comparateur, juger les **formes réellement produites**
par `Q4LocalGeometry::form` aux extrêmes u18 contre un oracle rationnel,
avec pivot négatif, racines égales, groupes de signes opposés et mutations
de l'orientation du déterminant. Les 500 formes arbitraires bornées du
script ne remplacent pas cette gate native.

Cette borne est **locale et combinatoire**, pas un algorithme
`O(Km)` déjà construit. Les présentations de support sur une grande
coquille et le coût de découverte des sommets restent ouverts. Sous une
couverture complète des arêtes propriétaires, une route pour la voie q4
consiste à énumérer **une fois**
les sommets peu profonds de l'arrangement d'une arête propriétaire,
choisir à chaque sommet deux droites incidentes indépendantes pour
reconstruire la BallKey, puis faire le census global I/U. La clé n'est
admissible que si le centre appartient à `conv(U)` : c'est le critère de
miniballe des contacts, vérifiable exactement, et non une conséquence de
la profondeur. Calculer ensuite `q_min`, garder la fenêtre de rang
`p+q_min−1≤Kmax` et dédupliquer les clés. Toute boule utile de `q_min=4`
possède un tétraèdre positif, donc une arête propriétaire dont les deux
contacts restants donnent bien deux droites indépendantes. Les voies
q2/q3 conservent leurs objets propres. La paire de droites choisie peut
ne pas être un support positif ; elle sert à construire **une clé à
certifier**, jamais à publier directement une boule.
Le test `c∈conv(U)` certifie la miniballe de la coquille, **pas** une
présentation q4 : six sites `±e_i` ont cette propriété mais aucun
tétraèdre strictement positif contenant leur centre. `q_min<4` ne prouve
pas non plus l'absence d'une présentation q4 **sur la même coquille** :
les huit sommets `(±1,±1,±1)` ont une paire antipodale (`q_min=2`) et
un tétraèdre alterné strictement positif autour du même centre.
Un catalogue FULL peut fusionner les clés après census et quotient exacts ;
le juge différentiel des supports doit conserver la positivité et les
incidences de la voie qui a réellement émis chaque présentation.

La condition `c∈conv(U)` est indispensable même avec arête maximale et
graine aiguë. Fixture entière : `a=(8,5,1)`, `b=(9,5,8)`,
`x=(8,1,5)`, `y=(9,2,5)`. Ces quatre points non coplanaires sont sur
la sphère de centre `c=(5,5,5)`, rayon 5 ; `|ab|²=50` est strictement la
plus grande distance, `abx` est aigu (`(a−x)·(b−x)=4>0`), et les deux
droites de contact de `x,y` se coupent en `c` avec profondeur zéro.
Pourtant tous les contacts ont première coordonnée ≥8, donc
`c∉conv(U)` : cette sphère n'est la miniballe d'aucun support. Un
émetteur « un sommet peu profond = une boule » ajouterait une fausse
BallKey. Le test convexe peut partager le calcul du futur quotient de
grande coquille ; une simple boîte englobante ne suffit pas.
L'[oracle entier local](check_qmin_planes_u18_20260922.py) vérifie cette
fixture et la sous-coquille q4 u12 à trois parents K10. Il ne construit
pas l'arrangement ni un générateur de tour.

L'objet de sortie prometteur est donc le **centre peu profond canonique
certifié**, avec profondeur exacte et droites incidentes, plutôt que
chaque paire de complétions `(x,y)` : dédupliquer la BallKey, puis payer
le census global I/U une fois par clé et le quotient local des grandes
coquilles.
Retrouver toutes les incidences et payer ce census sont des obligations
comptables, pas des coûts supprimés par la borne. Sur la
ligne 1 mm, `Σ cover_sites` q4 vaut `2 778 563 938` pour `1 872 168`
arêtes. Ce compteur est une **population logique** :
`Q4LocalGeometry::decompose_cover` ajoute `node.range.size()` lors de la
copie d'un **seul ID de nœud**, sans lire tous ses sites. Il majore le
nombre de formes qu'une construction de niveaux matérialisée **par arête**
devrait examiner ; ce n'est pas le nombre d'opérations effectivement
payées par le moteur actuel. À K5, la borne agrégée
`3Σm≤8,336` milliards reste trop lâche pour annoncer une seconde ou une
croissance sous-quadratique. Le reçu distingue les visites du premier
cover, celles de sa décomposition et les IDs réellement copiés : les
facturer séparément avant tout port des niveaux.

**Réutilisation exacte de la première traversée du cover.** Le DFS de
`Q34EdgeCover::build` connaît déjà les nœuds admis. Pour une arête q4,
une pile postordre peut fusionner deux enfants entièrement admis en leur
parent et produire la frontière maximale de nœuds couverts ; les ranges
triés/fusionnés se déduisent de cette frontière. C'est la même partition
de sites que celle reconstruite aujourd'hui par
`Q4LocalGeometry::decompose_cover`, sans seconde visite globale de
l'index. Dans le reçu v8 1 mm, cette seconde traversée compte
**315 736 960 visites** et copie **62 914 199 IDs de nœuds** ; le
premier cover compte déjà **440 194 038 visites**. Le port doit conserver
la même frontière, les mêmes compteurs de populations et les réponses
q3/q4/FULL ; il ne s'impose pas aux arêtes q3 seules. La lentille du
domaine positif q4 est contenue dans le cover, car
`|z−(a+b)/2|²=(|z−a|²+|z−b|²)/2−|a−b|²/4≤3|a−b|²/4`.
Une traversée conjointe peut donc transmettre les exclusions, mais
redémarrer aveuglément le domaine sur les seuls nœuds du cover pourrait
augmenter les visites si cela perd un rejet à leur ancêtre. Tester le
DFS conjoint et son coût de bornes avant port. Ces économies de
préparation ne touchent pas les **11,43 milliards** de visites de
partition d'atlas du même reçu : elles ne ferment pas le verrou principal.
Comparer, sur les mêmes arêtes échantillonnées, `m`, centres peu profonds
exacts, centres positifs possédés, temps d'énumération et les
`11,433` milliards de visites de l'atlas ; une construction complète
de l'arrangement en `m²` déplacerait simplement le coût.

## Certificat local de rayon : proposition v9 exacte

Une *cellule de centres* `C` est une boîte rationnelle fermée en 3D,
créée **à la demande** depuis le même index spatial du nuage déclaré.
Tout centre q4 positif appartient au tétraèdre de ses quatre supports :
la boîte englobante du nuage entier est donc un domaine initial complet.
Les centres q3 aigus et les milieux q2 y appartiennent également.
Cette racine globale n'est qu'une preuve de couverture : puisque toutes
les boîtes Z de l'index sont incluses dans C, `gap²(C,box(Z))=0` et le
certificat de rayon n'y écarte aucun site. La question algorithmique est
de produire des cellules plus petites **sans payer déjà** toutes les
graines et tous les atlas ; le front doit conserver des familles tant
que leur expansion n'est pas justifiée.
Choisir `T=Kmax−2>0` IDs distincts **de sites géométriques**
`g_1,…,g_T` du nuage préparé, par exemple des voisins du milieu de C.
Deux retours LiDAR fusionnés ne sont pas deux gardes. Leur proximité n'est
qu'une heuristique ; la preuve demande seulement des sites distincts.
Définir exactement

`U_C = max_{1≤i≤T} max_{c∈C} |g_i−c|²`.

Pour une boîte, chaque maximum axial est à une extrémité : `U_C` est donc
calculable sur les huit coins sans racine ni approximation. Si une boule
q4 admissible a son centre `c∈C` et son rayon carré `R²>U_C`, alors les
`T` gardes vérifient toutes `|g_i−c|²≤U_C<R²` : elle a au moins `T`
intérieurs stricts, contradiction. **Toute telle boule vérifie
`R²≤U_C`.** En particulier, on peut écarter comme support, intérieur et
contact chaque nœud spatial `Z` tel que

`gap²(C, box(Z)) > U_C`,

où `gap²` est le carré exact de la distance minimale entre deux boîtes.
La stricte inégalité est obligatoire : à égalité, un site peut porter la
coquille. Appelons `S_C` l'union des nœuds non écartés, développée en IDs
seulement si nécessaire. Pour toute boule q4 acceptée centrée dans C,
ses quatre supports, **tous ses intérieurs et toute sa coquille** sont
dans `S_C`; les sites extérieurs à `S_C` ont une puissance strictement
positive. C'est donc un contrat plus fort qu'un simple filtre de graines.

On peut choisir une liste ordonnée de dix gardes et calculer séparément
`U_8`, `U_9`, `U_10` pour les seuils maximums q4, q3 et q2 ; partager la
cellule et l'index, sans confondre leurs seuils. Pour K plus petit, le
certificat au seuil maximum reste sûr mais peut être large. Sur une arête
propriétaire q4 de carré `D`, la borne géométrique déjà démontrée
`R²≤3D/8` peut être intersectée avec `U_C`, si son contexte de propriété
est certifié ; ne jamais l'appliquer aux graines non propriétaires.

L'argument est indépendant de la disposition, de l'alignement des passages
LiDAR et du profil numérique. Pour float32, les coordonnées et bornes de
cellules sont représentées comme rationnels dyadiques exacts ; le moteur
u18/i128 ne qualifie pas automatiquement cette voie. Les gardes issues d'un
masque sans sol sont prises dans **ce même sous-nuage**, jamais dans la
trame brute. Une cellule utilise des bornes fermées pour prouver et une
convention de possession demi-ouverte pour n'émettre chaque centre qu'une
fois, y compris sur les faces des cellules.

### Comment l'utiliser sans construire une mosaïque complète

Première expérience peu intrusive : prendre les cellules encore visitées
de Local28, y calculer `U_C` et mesurer les nœuds/IDs que ce certificat
retire **avant** les copies de frontières et le balayage. Une
`Q4LocalCell` existante est un carré `(α,β)` dans le plan bissecteur,
dont l'image par la carte affine des centres est un **parallélogramme
3D**, et non la boîte XYZ C du lemme. Calculer `U_C` sur ses **quatre
coins mappés** ; pour un premier gap sûr, utiliser la boîte XYZ qui les
enveloppe, dont la distance à Z minore celle du parallélogramme. Cette
surboîte peut perdre des rejets : mesurer ce coût avant un prédicat exact
parallélogramme–boîte, et requalifier les largeurs numériques. Les gardes et
leur preuve sont immuables et peuvent être réutilisés par plusieurs
graines de l'arête ; une petite table de cellules spatiales partagée entre
arêtes n'est utile que si ses réutilisations payent sa construction.
`U_C` ne devient ni un crédit de profondeur, ni un fragment exact ; il
borne le rayon des boules encore admises. Le certificat de saturation v8
reste un autre état terminal.

Un raccord q3 constructif peut aussi exploiter une feuille **exacte** déjà
payée par Local28 pour la même arête : son compte uniforme `c0` et ses
nœuds actifs disjoints donnent, au centre de la boule q3 valide,
`d = c0 + #{z actif : puissance_q3(z)<0}`. Les extérieurs de cette partition
sont strictement extérieurs dans toute la cellule fermée, et la coquille se collecte parmi
les actifs ; le cover de l'arête restitue ensuite le nuage entier. Cela
demande une vue typée `ExactLeaf` possédant atlas/cover/index et cellule,
avec un appel synchrone ou une tâche qui prolonge leur durée de vie.
`certified_inside_count()` seul ne suffit pas : un nœud `Deep` peut ne
porter qu'un minorant après saturation et n'a plus de frontière exacte.
`Deep ≥ K−1` peut rejeter q3 ; `Deep = K−2` exige le census q3 global ou
un autre certificat. `Outside` ne transmet rien. Ne pas relancer le census
à la racine avec `c0`, ce qui doublerait les intérieurs déjà classés.
Cette feuille donne un compte, pas la liste des IDs uniformément intérieurs
du catalogue FULL ; conserver les nœuds concernés ou les recollecter une
fois par clé de boule après déduplication.

Seconde expérience, seulement si les listes locales restent réellement
petites : énumérer directement les boules minimales q4 des `S_C` des
cellules demandées, tester centre dans la cellule possédante, positivité,
profondeur exacte, clé et `q_min`. La référence simple peut essayer les
quadruples locaux puis vérifier `S_C`; un générateur performant devra
remplacer cette combinatoire par des événements peu profonds locaux.
Le contexte contient propriétaire du nuage/index, cellule, gardes, seuil,
liste de nœuds retenus et identités des sites ; les workers empruntent ce
contexte immuable avec buffers privés. Une limite de taille déclenche un
repli exact sur la cellule, jamais sa suppression.

La [fenêtre30](../../morsehgp3D_v8/docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md)
fournit déjà l'invariant du traitement local : pour la seed q4, profondeur
au groupe de racine `r` = intérieurs permanents `c0` + entrées à racine
`<r` + sorties à racine `>r`. Seules les `H=T−c0` premières entrées et
les `H` dernières sorties, avec tous leurs ex æquo, peuvent porter un
groupe peu profond. Les seuls événements *strictement entre* les deux
bornes sont au plus `2H−2` IDs. L'index dual proposé dans
[l'audit v8](../../morsehgp3D_v8/audits/q4_kernel_composition_20260920/WINDOW_INDEX.md)
pourrait trouver ces rangs sans deux scans par seed ; ses chaînes,
plateaux, pôles et constantes `c=0` restent à implémenter et à payer.
Sur `S_C` petit, le scan local existant est une meilleure référence.

## Ce qui interdit une promesse sous-quadratique prématurée

Noter `C*` les cellules vraiment construites, `V` les visites de l'index
**y compris la sélection des gardes**, `M=Σ_C |S_C|` les incidences
cellule–site retenues, `A_C` le coût exact d'énumération et de
vérification local, et `L` les octets des clés, coquilles, intérieurs et
sorties. Le budget de la **tour entière** paie préparation de l'index,
front et covers, construction des cellules, `V+M+Σ A_C+L`, q2/q3,
déduplication, parents FULL et transferts CPU/GPU. Le prototype naïf a
`A_C=O(binomial(|S_C|,4)·|S_C|)` ; sa validité ne suffit pas à son coût.
Un régime de taille de cellules bornée, avec `|C*|=O(n)` et `V+M=O(n log n)`,
donnerait une voie sous-quadratique **conditionnelle** à sortie compacte.
La v8 ne démontre aucune de ces conditions sur le LiDAR.
À titre de garde-fou concret, scanner 39 885 sites pour choisir des gardes
dans chacune des 38,795 millions de cellules du reçu 1 mm ferait environ
**1,55×10¹² examens** ; stocker huit IDs de 64 bits par cellule ajouterait
environ **2,48 Go d'écritures logiques**. Réemployer les gardes d'un parent
ou les chercher dans l'index ne vaut que si son propre coût est mesuré.

Deux contre-régimes sont indispensables aux gates :

1. **Un graphe k-NN fixe manque des q4.** Prendre les quatre sommets
   `p_i=(5000,5000,5000)+1000 σ_i`, avec
   `σ_i∈{(+,+,+),(+,−,−),(−,+,−),(−,−,+)}`. Leur sphère centrée en
   `(5000,5000,5000)` est vide, leur tétraèdre est strictement positif.
   Ajouter pour chaque sommet les `m` points distincts
   `p_{i,j}=(5000,5000,5000)+(1000+j)σ_i`, `1≤j≤m<1000`.
   Tous sont **hors** de la sphère initiale, donc le q4 reste de profondeur
   zéro ; les `m` voisins radiaux de chaque sommet sont pourtant plus
   proches que chacun des trois autres sommets. Pour `m≥k`, ses arêtes
   peuvent manquer d'un graphe des k plus proches voisins. Un k-NN local
   est une proposition ; seul un certificat de complétude autorise à
   éliminer son repli.
2. **Coquille massive / vide.** `n` points exactement cosphériques autour
   d'un centre `c` donnent, même pour la cellule ponctuelle `{c}`,
   `U_C=R²` et `S_C` contenant tous les points par égalité. Raffiner la
   cellule ne réduit pas ce résidu. Des nuages sur parois autour d'une
   cavité peuvent garder aussi de grands `S_C` sans cosphéricité exacte.
   Regrouper par clé de boule et conserver la coquille complète ; ne pas
   énumérer aveuglément tous les quadruples d'une même sphère.

Le cas limite `K≤2` n'a pas de sortie q4 (`T≤0`) ; la fabrique de gardes
ne doit pas demander un nombre nul de points pour ensuite conclure à un
rayon fini. Si `n<T`, le seuil ne peut être atteint par des gardes :
repli exact sur ce petit nuage. Les échecs de gardes et cellules trop
larges restent des tâches exactes visibles. Une simple triangulation
Delaunay d'ordre zéro ne suffit pas non plus lorsque `0<d<T`.

## Ordre de preuve et de mesure proposé

1. **Gate autonome de rayon** sur petits nuages avec `Fraction` : tous les
   quadruples positifs, centres rationnels, gardes arbitraires et proches,
   boîte du centre sur frontière, `gap²=U_C`, coquille 30, copie de masque
   sans sol, contre-fixture k-NN ci-dessus. Comparer supports, profondeur
   et coquille sur le **nuage entier** ; muter `>` en `≥` et fusionner des
   IDs de gardes pour vérifier la causalité des tests.
2. **Capture sans moteur v9** sur les mêmes trames complètes et masques
   figés : distribution de `|S_C|`, `M`, `V`, cellules divisées, cellules
   difficiles, gardes recherchées, `Σ binomial(|S_C|,4)`, récidives de
   cellules entre arêtes, égalités/shells, et sortie minimale par niveau.
   Mesurer avant/après gardes tous les coûts de Local28, notamment copies
   de frontière. Ne pas se contenter du nombre de cellules rejetées.
3. **Raccord exact minimal** seulement si la capture réduit le travail
   total : une arête/cellule puis flux q4 global, comparaison aux sorties
   v8 et à un oracle indépendant borné. Publier sans sol et brut avec sol
   séparément, puis plusieurs scènes ; les moitiés/quarts restent des
   diagnostics de croissance. K5/K10 et float32/grille ne se mélangent pas.
4. **CPU/GPU G4** : tâches de cellules possédées, groupées par longueur de
   `S_C`, seuils d'allocation et file de débordement exacts ; propositions
   numériques rapides contrôlées par signes entiers exacts ou repli
   certifié. Les groupes de racines égales et les clés de boules passent
   une réduction déterministe. Mesurer temps mur, CPU, GPU, transferts,
   mémoire de pointe, débordements et catalogue/FULL sur la même entrée.
   Les entiers 1728 bits float32 et leur fréquence de repli exigent une
   qualification GPU propre ; l'i128 u18 n'est pas un substitut.

**Décision recommandée :** conserver le balayage et la fenêtre v8 comme
oracles, vérifier d'abord si le certificat de rayon fait tomber les
incidences cellule–site sur les vrais LiDAR. La prochaine avancée utile
doit réduire simultanément préparation, graines, scans et copies ; une
accélération du seul tri ou une redistribution des mêmes milliards de
tests ne satisfera pas le contrat.
