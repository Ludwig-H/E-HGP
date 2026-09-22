# Audit v8 → v9 : q4 par boules minimales locales certifiées

22 septembre 2026. Moteur v8 lu à `a74e90f2`, ouverture v9 `3595725a` ;
premier moteur v9 `d2700314` relu ensuite. Audit mathématique et
architectural, **aucun chrono v9 qualifié** : le premier essai FULL de la
passation reste exploratoire sans reçu. Le [jalon de temps v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md)
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

Après regroupement exact des `m` droites et de leurs multiplicités, des
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
signé. Sur `s` droites de graines,
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
arêtes : à K5, la borne agrégée `3Σm≤8,336` milliards est encore trop
lâche pour annoncer une seconde ou une croissance sous-quadratique.
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
