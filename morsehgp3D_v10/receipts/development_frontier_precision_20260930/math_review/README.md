# Petit audit exact : ambiguïté intérieure K3 et bandes de couverture

Cadre : oracle mathématique borné, n≤7, entiers u18 ; aucun export natif,
moteur, compilation, GCP, score statistique ou contrat de temps. Script
`check.py`, sorties normal/−O identiques, sources externes hachées avant/après.
Ce paquet ne prétend pas avoir exécuté les bras de production : il calcule
les mêmes définitions via le juge Γ indépendant en Fraction.

## 1. Contre-exemple volumique, sans contact de x

IDs x=0,a=1,b=2,c=3,d=4. Avant translation :

    x=(0,0,0), a=(-4S,5S,0), b=(-4S,-5S,0),
    c=(4S,0,5S), d=(4S,0,-5S).

Ajouter (5S,5S,5S) à chaque point. Après perturbation, remplacer a_y=10S
par 10S−1 et b_y=0 par 1. Le déplacement maximal est 1 unité ; les autres
points ne bougent pas. S=1024 et 2048 restent u18 et affinement 3D.
Pour ces deux S, x reste STRICTEMENT INTÉRIEUR aux deux boules diamètre ab
et cd avant/après : distance au centre=4S < 5S−1.

Initialement, les deux boules ont β=25S², p=1, q_min=2 : témoins forts K3,
population {x,a,b} ou {x,c,d}. Elles donnent deux composantes Γ3 distinctes.
Les quatre sommets croisés {x,u,v}, u∈{a,b},v∈{c,d}, naissent à57S²/2.
Les quatre (K+1)-parties contenant x ont MEB3249S²/89 ; à cette date leurs
facettes réunissent les six composantes. La dernière 4-partie {a,b,c,d}
arrive à41S², sans retarder cette réunion. Les boules triangles de fusion
ont p1/q_min3 : elles ne sont pas des atomes forts K3, mais leurs événements
FULL K+1 doivent rester dans la forêt.

À l'égalité initiale, A5 attend cette réunion. A6 l'attend aussi : les six
atomes forts de x sont dans six composantes distinctes avant la réunion ;
aucun poids 1/β ne dépasse la moitié de leur somme.

Après contraction, première couverture de x à (5S−1)², par UNE composante,
p1/q_min2. A5 et A6 l'attachent immédiatement. Leurs hauteurs projetées
u(x,a) passent donc de3249S²/89 à(5S−1)². Pour S2048, le rayon projeté
change d'environ2134,99 unités pour un jitter maximal1.

À la coupe commune β26S², le graphe Γ est pourtant EXACTEMENT le même dans
les deux nuages : deux sommets isolés {x,a,b} et {x,c,d}, qui tous deux
couvrent x. Les partitions du head passent de
`[[x],[a,b],[c,d]]` à `[[x,a,b],[c,d]]`.
La masse fractionnaire uniforme est identique :5/6 sur chaque composante,
réserve10/3. La masse inverseβ change continûment dans cette famille à
catalogue fort constant ; sa conservation n'en fait pas une partition dure.

S=1 est conservé comme contrôle distinct : après son jitter, x est sur la
coquille gauche, pas strictement intérieur. Ne pas lui attribuer la portée
intérieure des deux captures prioritaires S1024/2048.

En géométrie réelle normalisée, remplacer la contraction par δ→0 donne un
saut limite de rayon57/√89−5>0. Sur la grille u18 finie, les deux captures
montrent une forte amplification, pas une limite asymptotique infinie.
Il ne s'agit ni d'un défaut de laminarité, ni du contre-exemple précédent
de changement du dénominateur inverseβ : c'est la règle «unicité à α seul».

## 2. Pourquoi A5/A6 corrigent peu A1 hors égalités

Si les rayons positifs des boules critiques dédupliquées sont distincts,
la première date α_K(x) appartient à une seule boule. Sa composante est
unique ; A5=A6=A1 pour chaque point. C'est la situation générique sous
perturbations continues hors relations algébriques de rayons égaux.
Une règle robuste d'ambiguïté doit donc examiner plus que les égalités
exactes à l'instant minimum. Les scores futurs doivent mesurer cette
différence, pas supposer que A5/A6 améliorent déjà les données génériques.

## 3. Théorème : univers fort complet pour les composantes couvrantes

Pour une boule critique B de population≥K, si p+q_min>K, toute incidence
x∈I∪U admet une K-partie F' contenant x avec moins de q_min points sur U :
prendre autant d'intérieurs que possible, et x s'il est sur U, puis compléter
avec des contacts. Si x est extérieur à I et p≥K−1, prendre x et K−1
intérieurs ; si x∈I et p≥K, prendre K intérieurs dont x.

La MEB(F') est strictement plus petite que B : sinon B serait son unique
MEB et un sous-ensemble de ses contacts porterait le centre, en contradiction
avec q_min. Les K-parties de pop(B) sont toutes dans la même composante à
β_B : le graphe de Johnson des K-parties est connexe et chaque (K+1)-union
est contenue dans B.

Partir d'un sommet Γ_K F contenant x, et appliquer cette réduction à sa
MEB tant que p+q_min>K. La descente stricte termine sur l'ensemble fini des
rayons des K-parties ; elle conserve x et la composante à la coupe initiale.
Elle aboutit à une boule forte p+q_min≤K, active dans cette composante et
contenant x. Réciproquement, chaque boule forte active donne une K-partie
contenant x dans sa composante. Ainsi, À TOUTE COUPE, les composantes
couvrant x dans Γ sont exactement celles des témoins forts actifs de x,
remontés puis dédupliqués. Ceci n'impose pas que leurs ball_node soient des
feuilles : les entrées internes K3/K5 sont incluses.

Corollaire à la toute première couverture : aucune boule faible contenant x
ne peut atteindre cette date, car F' donnerait une couverture plus tôt.
Le script contrôle54 premières incidences,167 réductions strictes et696
comparaisons de couvertures par composante sur dix petits nuages K2/K3/K5.
Cette preuve de couverture ne rend pas les poids catalogue intrinsèques,
ne prouve aucune complexité sous-quadratique et ne dispense pas du catalogue
fort COMPLET avec toutes incidences fermées I∪U.

## 4. Candidat minimal recommandé après revue du développeur

Choisir une bande de rayon (1+η)α, soit seuil carré B=(1+η)²α². Sélectionner
tous les témoins forts de x à β≤B, avec leurs ball_node vivants À LEUR
PROPRE DATE. J=LCA de ces nœuds ; attacher à
`t=max(α², naissance(J))`, propriétaire `anc(J,t)` en coupe fermée.

Le témoin sélectionné à α prouve que x est déjà couvert à t dans J : il
n'est pas nécessaire d'attendre le dernier β sélectionné. C'est un
look-ahead local déclaré, pas un comptage de témoins tous actifs à t.
Remonter tous les nœuds à B avant le LCA est plus conservateur : lorsque
J existe déjà à B, cette variante donne anc(J,B), et peut donc différer x
lors d'une fusion avec une branche qui ne couvre pas x.

Laminarité : attache fixée une fois puis ascendance. η-monotonie : ensembles
sélectionnés emboîtés, J remonte et t ne décroît pas ; à coupe fixée la
partition pour η plus grand raffine celle pour η plus petit, et les hauteurs
projetées ne décroissent pas. η0 redonne A5. À η1/32, les deux premières
branches du contre-exemple sont sélectionnées avant/après jitter S1024/2048,
mais pas les quatre naissances croisées ; la réunion de x attend la fusion
dans les deux cas, tandis que a/b et c/d gardent leurs attaches précoces.

Cette règle n'a pas ici de reçu d'exécution : le développeur l'implémente
séparément. Seuil de bande, composantes fantômes et catalogue mobile
nécessitent des tests : aucune robustesse universelle n'est annoncée.
Le travail est proportionnel aux incidences examinées et aux requêtes LCA,
pas à nchooseK ; le nombre réel d'incidences reste à mesurer.

Obligation suivante : regression native bornée des deux fixtures ; puis
condensation/EOM z1/z2 et min_cluster_size2/3 sur les deux nuages, avec dates
d'entrée des points et singletons de complétion correctement séparés.
La conservation fractionnaire72 contrôles du reçu n'est pas une sélection
EOM, ni une preuve de stabilité d'une partition dure ou de score amélioré.
