# Certificat B — paires de gardes pour tout un rectangle WSPD q3/q4

23 septembre 2026. Proposition **mathématiquement exacte mais non portée**.
Un [shadow local ultérieur](rect_pair_shadow_b_20260923/README.md) en a
mesuré la première palette bornée, sans gain intégré de chaîne.
Elle relève le [certificat ponctuel de A](paired_guards_precore_20260923/README.md)
du niveau d'une arête `ab` à **toutes les arêtes réelles d'un rectangle**
`A×B`. L'objectif est de décider avant le cœur, et si possible avant
l'expansion du produit, sans histogrammes quadratiques par facteur.
Elle ne remplace ni S2 ni le chemin exact en cas d'échec. Les boîtes
fermées `box(A),box(B)` doivent contenir les facteurs du même index
préparé ; l'identité des sites et le propriétaire de la plus longue
arête restent ceux du moteur.

## Lemme des 64 couples de coins

Fixer deux sites préparés distincts `g,h`, poser `d=b−a`,
`D=|d|²`, `w_g=2g−a−b`, `w_h=2h−a−b`, puis

`H=2D−|w_g|²−|w_h|²`, `C=d×(w_g+w_h)`.

Pour chaque couple parmi les **huit coins de `box(A)` et les huit
coins de `box(B)`**, exiger `H>0` et l'inégalité stricte de la voie :

| Voie active | Test exact à chacun des 64 couples |
| --- | --- |
| q3 | `3H²>4|C|²` |
| q4 | `H²>2|C|²` |

Alors cette même paire donne au moins **un site strictement intérieur**
à toute boule q3 aiguë ou q4 positive possédée par **n'importe quelle**
arête réelle `ab∈A×B`, respectivement. Les coins sont virtuels : il
n'est pas nécessaire qu'ils soient des arêtes émises ou des sites du
nuage. Une boîte dégénérée répète simplement certains tests.

Preuve : les deux expressions se réécrivent

`H/4=−2a·b+(g+h)·(a+b)−|g|²−|h|²`,

`C=2[(b−a)×(g+h)+2a×b]`.

Le couple `(H,C)` est donc **affine en `a` à `b` fixé**, et affine en
`b` à `a` fixé. Le test q3 équivaut à `H>(2/√3)|C|`, le test q4 à
`H>√2|C|` ; chacun définit un **cône convexe ouvert**. À `b` coin
fixé, toute valeur à l'intérieur de `box(A)` est une combinaison
convexe des huit valeurs aux coins de A et reste dans ce cône ; répéter
sur les coins de B. Autrement dit, le minimum de
`H−α|C|`, concave séparément dans `a` et `b`, est atteint à un couple
de coins. Les 64 tests sont ainsi même nécessaires et suffisants pour
la **boîte continue** et cette paire fixe, mais seulement suffisants
pour les facteurs discrets : un échec à un coin virtuel ne rejette
aucune arête réelle. La stricte positivité et les tangences sont
respectées ; `D=0` virtuel rendra normalement le test indécis.

Le [lemme ponctuel](paired_guards_precore_20260923/README.md) borne
les centres possédés par `|t|²≤D/12` en q3 et `≤D/8` en q4. Son
inégalité de paire dit `P_g(t)+P_h(t)>0` pour **tous** ces centres :
au moins un de `g,h` est donc réellement intérieur, même si son
identité change avec `ab` et le centre. Pour K5, quatre paires
disjointes ferment q3 et trois ferment q4 ; en général les seuils
sont `K−1` et `K−2` pour les voies admissibles. Les ensembles de
paires peuvent différer entre voies, mais on n'additionne jamais les
crédits des deux voies. Si seule q4 ferme, les arêtes gardent q3 ;
seul un masque totalement fermé permet de supprimer le segment
entier **avant le cœur**, ou le rectangle entier **avant son expansion**
si le test est placé au front.

Les IDs des **sites préparés** doivent être disjoints entre paires
d'une même preuve. Mathématiquement, ils peuvent appartenir à `A∪B` :
si `g=a`, alors `P_g=0` et une somme positive force `h` strictement
intérieur. Toutefois le contrat du premier shadow ponctuel exclut
les extrémités ; le premier port peut conserver, plus simplement,
des gardes **hors `A∪B`**, sans perdre la sûreté. Lever cette restriction
exige un test explicite et l'interdiction de cumuler un même ID avec
des crédits S2 ou singleton. Sous le domaine u18, les 64 coins virtuels
restent dans `[0,2^18−1]³`, donc les bornes i128 du certificat ponctuel
s'appliquent, à condition de promouvoir **avant** chaque produit.

### Fixture arithmétique non triviale

Prendre `A={(5,10,10),(6,10,10)}`, `B={(15,10,10)}` et les quatre
paires distinctes `((x,13,10),(x,7,10))`, `x=8,9,10,11`. Aux deux
couples de coins distincts, les minima de `H` par paire sont
`40,72,88,88` ; `C=0`, donc q3 et q4 passent strictement pour
**chaque** paire. Aucun des huit témoins pris isolément ne satisfait
le test universel q3 ou q4 aux deux arêtes : avec
`H_single=(g−a)·(b−g)` et
`Xi_single=|(g−a)×(b−g)|²`, sa meilleure marge q3
`3H_single²−Xi_single` sur ces arêtes reste négative pour chaque
paire (`−468,−225,−132,−225`). C'est un exemple de fermeture commune
qu'une simple union de témoins singletons S2 ne voit pas. Les deux
points de A forment ici une boîte dégénérée ; un oracle de port doit
aussi tester huit coins distincts, les contacts, un coin virtuel non
strict et les bornes u18 extrêmes. Cette fixture mathématique ne
mesure pas une décomposition WSPD réelle ni un gain temporel.

## Sélection et coût : le verrou déplacé, pas résolu

La preuve utilise une **palette commune au rectangle** ; celle du
[shadow de A](paired_guards_precore_20260923/README.md) balaie au
contraire les `n` sites **pour chaque arête** et ne peut être portée
comme telle. Sur la trame brute 08/000000/K5/s8, les
[11 174 segments S2 d'au moins 16 survivantes](s2_segment_mass_20260923/README.md)
portent 396 481 arêtes, soit 85,52 % des formes du cœur. Leur
longueur est connue après S2 et avant le cœur ; elle fournit un
déclencheur expérimental bon marché, non un seuil universel. À titre
de simple comptabilité, quatre recherches indexées par **segment**
font 44 696 recherches, contre 1 585 924 par arête : le rapport
35,5 n'est un gain que si une palette commune ferme assez de voies
pour payer recherches, paires, 64 coins et replis. Une recherche
best-first unique avec quatre quotas directionnels est aussi à comparer.
L'orientation de cette recherche peut provenir d'un représentant du
rectangle : elle ne sert **qu'à proposer** des gardes ; les 64 coins
décident seuls la preuve, quel que soit l'ordre des arêtes réelles.

Ce déclencheur sur la longueur du segment intervient **après** que S2
a énuméré et filtré les paires : il peut économiser le cœur et l'aval,
**pas l'expansion S2 déjà payée**. Pour tuer réellement un produit WSPD
avant la boucle `A×B`, il faut essayer le même théorème sur le masque
du rectangle avec un déclencheur disponible au front : taille
`|A||B|`, boîtes et scores certifiés déjà disponibles. Sur la trame
brute K5/s8, les 1 747 rectangles ouverts de produit au moins 1 024
portent 71,00 % des formes du cœur, mais certains grands produits
n'ont aucune survivante ; ce n'est pas un taux de réussite du
certificat. Les deux placements doivent être mesurés séparément.

Budgéter le **travail de preuve** seulement : `Q` nœuds visités et
`B` candidats par direction (par exemple `Q=64/128/256`, `B=4/8/16`),
au plus `O(B²)` propositions de paires et 64 tests exacts par paire
soumise. Essayer q4 puis q3 peut réutiliser les mêmes `H,C`, sans
additionner leurs crédits. Arrêt sur première preuve complète ;
budget épuisé, coin tangent, absence de palette ou masque seulement
partiel ⇒ **repli exact** sur les arêtes/voies restantes. Les
`B=4` candidats par quatre directions ne suffisent même pas aux
18 sites distincts de q3/K10 : un échec à ce budget ne donne aucune
information négative. Une palette limitée au cœur diamétral reste
conservatrice ; les gardes valides peuvent être ailleurs.

Le budget de coin est décisif : à `B=16`, jusqu'à 64 sites donnent
`64·63/2=2 016` paires, donc **129 024 tests de coins par segment**
si tout le graphe est essayé sans arrêt précoce. Sur les 11 174
segments ciblés, cela ferait jusqu'à **1,442 milliard** de tests,
contre 559,662 millions de formes du cœur dans la trame entière.
Ce plafond purement arithmétique interdit de lire le facteur 35,5
sur le nombre de recherches comme un facteur de vitesse. Crible
conservateur `H_min/X_max`, rejet au premier coin défavorable,
propositions prometteuses et arrêt dès `K−1`/`K−2` paires doivent
être chronométrés ; un matching optimal par arête n'est pas gratuit
et le graphe n'est pas nécessairement biparti par quadrants. Par
exemple, `a=(4,10,10)`, `b=(16,10,10)`, `g=(10,13,11)` et
`h=(10,11,13)` placent les deux gardes dans le même quadrant `(+, +)`
du plan `(y,z)` :
ils échouent isolément en q4, mais leur paire a `H=208`,
`|C|²=18 432`, donc `H²=43 264>2|C|²=36 864`.
Comparer aussi une **palette partagée par tuile d'arêtes** du même
segment, mais avec le test ponctuel exact refait par arête : cela
amortit la recherche sans imposer qu'une paire passe aux coins de
tout le rectangle. Les tuiles gardent masques/ordinals et repli exact.
Leurs tests d'arêtes restent proportionnels au nombre de survivantes ;
une recherche représentative inadaptée ne prouve simplement rien.

Une variante encore à juger cherche deux nœuds témoins `U,V` de
populations **disjointes**, hors `A∪B` pour un premier port. Si un
majorant de `|C|²` et un minorant de `H` prouvent le test pour
**tout** `g∈box(U),h∈box(V),a∈box(A),b∈box(B)`, alors les deux
populations créditent `min(|U|,|V|)` paires distinctes sans charger
les IDs. Pour plusieurs crédits, une somme naïve
`Σmin(|U_i|,|V_i|)` est fausse si un site est réutilisé ; choisir une
antichaîne/allocation disjointe et ne pas additionner avec des crédits
S2 d'identité inconnue. La première cible simple est **un seul**
couple de nœuds de population au moins `K−1` ou `K−2` de chaque côté.
Les bornes découplées sont sûres mais conservatrices ;
un repli **exact pour les quatre boîtes continues** consiste à tester
les `8^4=4096` quadruplets de coins. En effet `H` est affine
séparément en `a,b` et concave en `g,h`, `C` est affine séparément
dans les quatre variables : `H−α|C|` est concave séparément et son
minimum tombe sur des coins par quatre applications de Jensen.
Cette preuve exige bien tous les quadruplets croisés, pas quatre
coins appariés ni un représentant de nœud. Le coût peut être
prohibitif ; essayer d'abord des bornes entières conservatives et
réserver les 4096 coins à des nœuds lourds proches de l'admission.
Une fixture positive non singleton pour les **quatre boîtes** est
`A={(5,10,10),(6,10,10)}`, `B={(14,10,10),(15,10,10)}`,
`U={(x,13,10):8≤x≤11}`, `V={(x,7,10):8≤x≤11}`.
Sur tous les quadruplets de coins, `C=0` et `H≥24` ; les deux nœuds
peuvent donc fournir quatre paires disjointes pour K5. Un représentant
de U qui passe ne suffirait pas à créditer tout U sans ce contrôle.
À défaut de certificat positif, revenir sans omission au chemin
courant. Le [shadow A par nœuds](paired_guard_node_blocks_20260923/README.md)
mesure déjà une sélection **par arête fixe** : cap4/budget64 ferme
32/120 arêtes lourdes contre 21/120 pour les points, puis 67/120
contre 67/120 à budget256. Il vérifie 758 certificats de blocs,
mais **pas** la condition uniforme sur `A×B` ni sa sélectivité ;
les arêtes sont tirées selon `F` connu après le cœur.

La première porte est un **shadow exact, sans port moteur** : sur les
8k/16k/32k emboîtés, conserver pour chaque segment son rectangle,
masques d'arêtes et `F`, comparer palette indexée commune à palette
par arête à budgets égaux et les deux types de certificat. Publier
tentatives, nœuds/feuilles, paires et coins testés, crédits, segments
fermés par voie, arêtes réellement évitées **avant** `load`, formes et
covers économisés, coût des replis, CPU·s/mur/RSS et identité du
catalogue **clé par clé** puis FULL. Répéter brut et sans sol de
plusieurs séquences, K5/K10 et s8/10/12. Les boîtes se resserrent
quand `s` croît, mais leur nombre et les frais de sélection peuvent
augmenter. Un budget constant par segment ne démontre pas une borne
sous-quadratique si le nombre de segments, l'expansion restante ou
l'aval croissent trop vite. Aucun contrat 1 s/100 ms ni dizaines de
millions de points n'est acquis par cette preuve seule.

**Mise à jour du shadow :** sur tous les 1 747 rectangles ouverts
`|A||B|≥1 024` de la trame brute 08/000000/K5/s8, cette palette
commune ferme complètement 72 des 299 rectangles ayant des
survivantes S2, soit 5 059 809 des 559 661 741 formes du cœur
(0,904 %). Un appariement maximum n'ajoute que trois fermetures
positives au glouton. Les 979 rectangles supplémentaires fermés
avaient déjà un segment S2 vide : ils pourraient réduire le coût S2,
pas le cœur. Le shadow paie 26,845 millions de couples de coins et
ne mesure aucun gain moteur/G4. Le résultat borne **cette palette et
ce budget**, non le théorème ou les variantes adaptatives.
