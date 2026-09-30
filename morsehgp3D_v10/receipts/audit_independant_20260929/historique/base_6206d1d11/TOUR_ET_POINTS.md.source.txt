# Tour FULL et passage aux points — audit indépendant du 29 septembre 2026

Base figée : `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`. Sources relues dans le snapshot neuf
`build/v10-audit-independent-20260929/source/morsehgp3D_v10`, bibliothèque Release construite par l'auditeur
principal. Travail produit inchangé ; aucun build antérieur écrasé ; GCP non utilisé. Les sondes sont des contrôles
exhaustifs bornés de petits nuages, sans mesure de performance ni comparaison aux labels LiDAR.

**Conclusion.** Aucun défaut de topologie FULL, de quotient local, de plateau ou de verticale n'a été démontré dans
le domaine non pondéré servi. La séparation entre catalogue exact, naissance, descente et assemblage par plateau
est mathématiquement cohérente. Le point difficile suivant est une décision de modèle : une tour exacte ne fournit
pas automatiquement une hiérarchie laminaire unique de points à travers plusieurs K, ni sa robustesse statistique.
Deux limites de preuve sont concrètes : le juge vertical livré voit seulement les points core, et la tête reçoit
des niveaux double où des niveaux rationnels distincts peuvent être regroupés.

## 1. Objet mathématique relu

Pour un K fixé, $L_K(a)=\lbrace y:D_K(y)\leq a\rbrace$ est un emboîtement. La tour code ses composantes, leurs
naissances et leurs multifusions. La référence exhaustive Γ énumère les K-parties F de miniboule $\beta(F)\leq a$
et les relie via les (K+1)-parties de miniboule au plus a. Elle ne reprend ni l'index natif, ni les filtres flottants,
ni le catalogue généré, ni la descente du C++.

Les obligations décisives sont satisfaites par les mécanismes suivants, sous complétude du catalogue et les bornes
arithmétiques qualifiées ailleurs dans cet audit :

| Mécanisme | Raison mathématique et lecture du code |
| --- | --- |
| MEB certifiée | Contenir F et avoir le centre dans l'enveloppe fermée d'un support sur la sphère suffit à prouver l'unique miniboule. `verify_meb` applique ces deux conditions avant tout usage du résultat proposé en double. |
| Gordan | Une partie de coquille A permet un déplacement strictement rapprochant si et seulement si c n'est pas dans conv(A). Le centre de la coquille étant fixé et tous les points étant sur sa sphère, les tests paire/triangle/tétraèdre de `support.hpp` correspondent bien à Carathéodory en dimension 3. |
| Morceaux locaux | L'arête native « A∪B séparable », même sans imposer taille t+1, a les mêmes composantes que les échanges d'une seule position : si l'union est séparable, toutes les t-parties de cette union le sont et se relient par échanges. |
| Coquille régulière | Pour q positions affinement indépendantes autour de c intérieur relatif : naissance à t=q, q morceaux à t=q−1, un morceau pour t≤q−2. Les voies analytiques correspondent à ces cas. |
| Descente | Si au moins K intérieurs stricts existent, les K plus proches du centre tiennent dans une boule strictement plus petite. Sinon, une partie séparable autorise un rayon strictement plus petit. Les parties restent reliées dans la boule fermée précédente. |
| Arrêt sur cellule | Le memo représente une composante à la coupe fermée de la sphère. Plusieurs morceaux locaux peuvent descendre vers des naissances différentes, mais ils sont joints au niveau de cette sphère ; en consommer un représentant reste correct à ce niveau et aux niveaux ultérieurs. |
| Plateau | `kruskal` prend les racines pré-lot de toutes les jonctions du même rang exact, puis ferme le lot et crée une seule fusion par groupe. Il ne binarise pas artificiellement les égalités. |
| Verticale | Une (K−1)-partie d'une naissance est réalisée au centre ; son image est l'ancêtre à la coupe fermée dans K−1. Pour une fusion, les images de tous ses enfants doivent coïncider ; `vertical_naturality` contrôle cette condition. |

Le repli Welzl exact a été sondé directement sur des configurations coplanaires, alignées, cosphériques, proches
des extrêmes u18 et sous permutations. Aucun défaut du traitement dégénéré n'a été obtenu. Cette sonde n'est pas
un théorème général sur Welzl ; son oracle brute Fraction est distinct de la récursion native.

## 2. Qualification additionnelle réellement exécutée

Les fichiers [tower_probe.cpp](../../receipts/audit_independant_20260929/probes/tower_probe.cpp) et [tower_full_check.py](../../receipts/audit_independant_20260929/probes/tower_full_check.py) publient les
K-parties des naissances et interrogent la tour livrée. Le juge énumère toutes les K-parties et (K+1)-parties du
nuage, calcule les miniboules avec Fraction par force brute sur les supports, puis compare :

- les partitions des naissances à toutes les coupes critiques ouvertes et fermées ;
- chaque image verticale de chaque nœud, y compris les nœuds sans point core attaché ;
- `ball_node` contre la composante Γ de toutes les K-parties de la boule couvrante ;
- chaque entrée cover α contre la plus petite miniboule d'une K-partie contenant le point.

Six fixtures explicites : E5, cube, carré, ligne irrégulière, six points circulaires, octaèdre. Seize nuages
supplémentaires à graine fixe : grilles dégénérées, coplanaires, près de la borne u18, génériques. **Tous les ordres
jusqu'à n, n≤8**, entrées core et cover. Les sorties sémantiques sont égales à un et quatre fils.

| Contrôle | Nombre | Résultat |
| --- | ---: | --- |
| Nuages | 22 | PASS |
| Coupes des partitions de naissances | 3 264 | PASS |
| Verticales de tous les nœuds, core et cover | 1 846 | PASS |
| K-parties des boules couvrantes | 9 518 | PASS |
| Entrées α jugées en Fraction exact | 911 | PASS |
| MEB : Welzl exact direct et proposition certifiée, 20 géométries × 3 permutations | 120 sphères | PASS |

La dernière ligne provient de [tower_meb_probe.cpp](../../receipts/audit_independant_20260929/probes/tower_meb_probe.cpp) et
[tower_meb_check.py](../../receipts/audit_independant_20260929/probes/tower_meb_check.py). La proposition certifiée a toujours réussi dans ce lot : **zéro appel
effectif au repli dans `meb`**, malgré la sonde directe de cette routine de repli. Ne pas transférer cette
qualification à une fréquence de replis ou à des descentes longues en production.

Les commandes, sorties, empreintes des sources de sonde et binaires sont conservées dans
[tower_receipt.json](../../receipts/audit_independant_20260929/tower_receipt.json). Le premier lot de neuf nuages est conservé, ainsi que l'échec initial de
compilation du harnais MEB (sérialisation i128 incorrecte dans la sonde, corrigée ; aucun défaut produit déduit).

## 3. Limite réelle du juge vertical livré

`tests/oracle/test_tower_oracle.py::vertical_check` ne vérifie l'image d'une composante que si un site core est
déjà entré dans cette composante. Une naissance peut avoir un centre appartenant à L_K sans contenir aucun site
de X à ce niveau. Elle échappe alors à ce contrôle. Le phénomène ne disparaît pas avec l'entrée cover : ses
points peuvent avoir déjà été affectés à une autre première composante couvrante.

Dans le lot supplémentaire, **623 naissances core et 292 naissances cover**, comptées avec répétition entre les
deux projections, n'ont aucun point attaché à leur date. Le nouveau juge les contrôle via leurs K-parties. Ce
constat est une faiblesse de qualification du gate existant, **pas une image verticale incorrecte observée**.

Remède utile : garder dans la porte un représentant K-partie par naissance, puis vérifier toutes les images aux
niveaux de naissance/fusion par Γ. Le harnais fourni montre comment le faire sans reproduire la descente native.
Le coût reste petit sur les fixtures ; aucune nouvelle campagne LiDAR n'est nécessaire pour fermer ce trou.

## 4. Une hiérarchie laminaire de points existe déjà à K fixé

**Core a une définition intrinsèque.** Chaque point x entre à $D_K(x)$ dans la composante qui contient réellement
x. À toute coupe, les ensembles $C\cap X$ des composantes sont disjoints. À mesure que a augmente, un point déjà
entré ne sort jamais et sa composante ne fait que fusionner. Les ensembles de points des nœuds, avec leurs dates
d'entrée, forment donc une famille laminaire. Les composantes sans point peuvent être conservées pour l'objet
FULL et contractées pour un consommateur de points, en conservant les dates d'entrée.

Cette propriété découle des inclusions horizontales, pas d'une approximation de la géométrie. La descente des
K plus proches de x suivie d'un ancêtre à la coupe **fermée** respecte la définition. Le juge Γ des points core
est un vrai contrôle indépendant de cette appartenance.

**Cover construit une autre projection laminaire.** x choisit une première composante couvrante au rayon α, puis
la suit par les inclusions horizontales. Cela fournit aussi une affectation unique et une famille laminaire à
K fixé. Mais les amas discrets définis par toutes les boules couvrantes peuvent se recouvrir : la prolongation
d'une première affectation n'est pas leur relation complète d'appartenance. Le carré à K=2 donne déjà des points
couverts par plusieurs naissances de côté au même rayon ; `first` en choisit une selon l'ordre canonique.

Le code expose la relation plus riche sur demande via `ball_node`, et le vote utilise cette relation. La décision
hard de première entrée est explicitement annoncée dans `CLUSTER_v2` § 3.2 ; elle n'est donc pas un défaut caché.
Elle doit rester nommée comme une projection, surtout lorsque l'on attribue un avantage statistique à la tour.

## 5. Plusieurs K ne donnent pas spontanément une famille laminaire

Les verticales codent $L_{K+1}(a)\subseteq L_K(a)$ au **même a**. Elles n'imposent pas de comparabilité entre deux
coupes $(K,a)$ et $(J,b)$ où l'ordre et le niveau changent dans des directions opposées.

Fixture courte vérifiée par la tour compilée **et** par Γ indépendant : les quatre points collinéaires
$X=\lbrace0,1,4,7\rbrace\times\lbrace0\rbrace^2$.

| Coupe core fermée | Ensembles de points entrés |
| --- | --- |
| K=1, a=1/4 | {0,1}, {4}, {7} |
| K=4, a=36 | {1,4} |

À K=4, la composante géométrique unique naît à 49/4 ; les distances d'entrée des points 0,1,4,7 sont respectivement
49,36,16,49. À a=36, les points 1 et 4 sont donc seuls entrés. Les ensembles **{0,1} et {1,4} se croisent**.
Une fusion de toutes les branches de plusieurs K dans un unique arbre doit choisir une règle supplémentaire et
perd nécessairement une partie des deux relations de cette fixture.

La preuve calculée est [tower_point_crossing.py](../../receipts/audit_independant_20260929/probes/tower_point_crossing.py), reçu dans
[tower_receipt.json](../../receipts/audit_independant_20260929/tower_receipt.json). Ce n'est pas un défaut de la tour : la tour est un foncteur à deux
paramètres, pas un arbre unique de points multi-K.

Conséquence pour le développeur : avant une tête multi-K, choisir l'objet cible. Une chaîne de coupes totalement
ordonnées par les inclusions de L conserve une hiérarchie laminaire exacte. Un consensus de partitions, une
antichaîne ou une sélection sur des coupes incomparables ajoute un modèle de laminarisation ; les verticales en
contrôlent la cohérence locale mais ne justifient pas cette sélection à elles seules.

## 6. Exactitude de la tour et fiabilité statistique : deux obligations

La tour est bien l'arbre des niveaux du champ K-NN à K fixé. La conversion d'un rayon en valeur de densité demande
un modèle : dimension pertinente, mesure de référence, volume de boule, taille d'échantillon. Une échelle
$\lambda=r^{-z}$ change le poids donné aux niveaux dans EOM ; elle ne prouve ni la consistance de l'estimateur,
ni celle de la sélection de modes.

Une stratégie raisonnable est de qualifier d'abord la projection core à K fixé, car elle conserve l'appartenance
intrinsèque, puis de traiter cover et le remplissage comme choix explicites de sélection/affectation. Le domaine
exact K≤10 est un domaine produit fini ; une affirmation asymptotique à n croissant demanderait un régime K(n)
et des hypothèses statistiques distincts. Les portes géométriques ne suffisent pas à cette affirmation.

Pour rendre la tête fiable, les obligations utiles sont des effets mesurables : stabilité sous rééchantillonnage,
réponse aux perturbations des coordonnées, séparation des modes au niveau du col, puis masse affectée sous le
col. Une réussite de l'extraction de composantes ne démontre pas que le choix EOM/z est optimal ; inversement,
un faible ARI d'une affectation ne réfute pas les composantes exactes. C'est la séparation à maintenir dans les
prochains reçus.

## 7. Exactitude du consommateur et portée des conceptions

`point_dendrogram` garde l'ordre exact pour fusionner les clés de catalogue et les distances entières core, mais
publie des doubles. Si deux niveaux exacts distincts ont le même double, ou des approximations inversées d'un ulp,
ils partagent un rang publié. C'est intentionnel et couvert par `test_level_collision` pour éviter un refus
`rank_order`. Le FULL natif conserve ses rangs exacts ; **la tête reçoit alors un quotient numérique des niveaux**.

La phrase « condensation exacte » vaut pour la règle N-aire sur l'arbre de points publié. Elle ne fournit pas à
elle seule une garantie de décisions EOM identiques à un calcul rationnel distinguant toutes les dates. Si une
application exige cette dernière garantie, conserver un rang exact séparé de la valeur double pour trancher les
plateaux, et définir la politique de calcul des stabilités. Il ne faut pas forcer un autre double artificiel pour
simuler un écart de niveau sans en déclarer la signification.

`TOWER_v2` reste un plan plus large que le produit actuel : multiplicités natives, tables circulaires, contributions
datées, index d'intervalles, noyau T2 parallèle, Euler pondéré et juges d'échelle J-DESC/J-MF. Le code sert aujourd'hui
le domaine non pondéré, refuse les gros quotients locaux explicitement et conserve Kruskal par plateaux. `SPEC_V10`
et les entêtes le disent. Ces extensions non livrées ne sont pas des bugs du domaine actuel ; elles doivent rester
des jalons ouverts. Les petites sondes ci-dessus ne qualifient ni les longues descentes LiDAR, ni les multiplicités,
ni une borne globale de travail, ni les contrats G4.

## 8. Suite recommandée

1. Conserver le juge de verticales par naissances/K-parties dans les portes, avec une fixture sans attache core à
   la naissance ; il ferme une faiblesse concrète sans chantier produit.
2. Décider la projection de points : core intrinsèque, cover prolongée, ou appartenance complète par boules. Le
   choix conditionne la tête et l'interprétation statistique des masses.
3. Toute tête multi-K doit publier sa règle de laminarisation et passer la fixture des quatre points croisés.
   Une fusion silencieuse de branches ne peut préserver simultanément les deux ensembles.
4. Garder les niveaux exacts du FULL séparés des valeurs double de la tête. Documenter le quotient numérique de
   `point_dendrogram`, puis tester sa politique de plateau avec une fixture minuscule plutôt qu'un nuage 8k.
5. Poursuivre la qualification statistique sur les modes et la masse sous les cols, sans assimiler exactitude
   géométrique, avantage de projection et supériorité de la sélection.
