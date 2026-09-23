# Piste B — tuer des rectangles q3/q4 avant d'énumérer les arêtes

23 septembre 2026. Proposition mathématique indépendante, **non
implémentée et non mesurée**. Les deux pistes existantes de [domination
par blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md) et de
[témoins avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md)
économisent surtout du travail **après** expansion, sur une arête donnée.
Ici l'objet est le produit WSPD résiduel `A×B` lui-même : certifier qu'une
voie q3 ou q4 est vide pour **toutes** ses paires, avec des groupes de
témoins qui peuvent changer selon la région du centre. Le filtre de
rectangle actuel recherche déjà des témoins universels à tout le
« citron » ; cette variante est plus fine, mais potentiellement plus
coûteuse. Aucun succès sur LiDAR n'est présumé.

## Certificat exact

Pour une arête propriétaire `ab`, soit `D=|a−b|²`, `m=(a+b)/2` et `c`
le centre de sa boule positive. Le centre est dans le cube d'entrée
`[0,M]³` (`M=262143`) car il appartient à l'enveloppe convexe du
support. Comme `a,b` sont sur la sphère,
`|c−m|²=R²−D/4`. La formule barycentrique de variance et le fait que
`ab` est une arête la plus longue donnent `R²≤D/3` pour q3 et
`R²≤3D/8` pour q4. Donc toute voie q3 tient dans
`|c−m|²≤D/12`, toute voie q4 dans `|c−m|²≤D/8`. Une boîte extérieure
de tous ces centres pour `a∈A,b∈B` se construit à partir des boîtes A/B
et d'un majorant entier de D, **sans** parcourir `A×B` ; si elle est
large, le certificat peut devenir inutile. La partitionner en cellules
fermées `C` qui couvrent la région possible, avec contacts conservés.

Pour un nœud témoin `Z` de l'index, dont la population est disjointe des
deux facteurs, tester à **chaque sommet** `v` de `C` l'inégalité stricte

`max_{g∈box(Z)} |g−v|² < min_{x∈box(A)} |x−v|²`.

Les deux extrema sur boîtes axiales se calculent exactement axe par axe.
Pour chaque paire fixe `(g,a)`, la différence
`|g−c|²−|a−c|²` est affine en `c`. Si elle est négative à tous les
sommets, elle l'est dans toute la cellule fermée ; le test des extrema
est plus fort et le prouve simultanément pour **tout** `g∈Z,a∈A`.
Comme une boule candidate passe par `a`, tout site de Z est strictement
intérieur à cette boule, quels que soient `b∈B` et le centre admissible
dans C. Des nœuds Z en antichaîne spatiale donnent des populations
distinctes ; **K−1** témoins dans chaque cellule possible tuent la voie
q3 du produit, **K−2** tuent q4. Les gardes peuvent différer d'une
cellule à l'autre, mais leurs comptes ne s'additionnent pas entre
cellules ; on ne crédite ni les extrémités, ni les témoins au contact.

Une cellule peut aussi être exclue avant recherche de gardes : si à
tous ses sommets `min_{x∈box(A)}|x−v|² >
max_{y∈box(B)}|y−v|²` (ou symétriquement B>A), l'affinité des
différences prouve qu'aucun `a∈A,b∈B` ne peut être équidistant d'un
centre de cette cellule. Les cellules non exclues et non certifiées
restent **indécises** : les subdiviser, subdiviser A ou B par vrais
enfants de l'index, ou revenir exactement à l'expansion actuelle.
Cette dernière option garantit l'exactitude sans quota de recherche.
Une division A/B partitionne le produit initial en sous-produits
disjoints, sans perte ni doublon de paire ; ne pas émettre une même
arête une fois par cellule de centres.

Les points u18 permettent des bornes entières étroites aux coins
entiers. Une grille dyadique de centres exige cependant de **prouver la
largeur selon la profondeur choisie** avant toute arithmétique i128 ou
de recourir à un repli exact ; aucune conversion flottante approximative
ne décide un signe. Les égalités sont indécises, jamais des témoins
stricts. La partition et les antichaînes doivent être possédées par la
tâche et compatibles avec les workers privés/GPU, sans span de pile
transféré.

## Pourquoi cela vaut un test, et pourquoi rien n'est acquis

R3 sur 08/000000/K10 laissait environ **153,94 M** de masse de paires
après le front, puis **30,78 M** paires développées, **4,51 M** covers,
**7,80 Md** formes chargées et **25,31 Md** tests uniformes. Le
certificat ne peut éviter ces coûts que s'il tue des produits non
singleton de masse significative **avant** la double boucle de
`wspd_q34.cpp`. S'il ne reste que de petits facteurs ou si les cellules
de centres sont trop larges, il ajoutera simplement du travail. Même la
suppression idéale de q3/q4 ne résoudrait pas le temps FULL : R3/K10
prenait 35,56 s au total, dont 9,70 s pour ce poste.

Première porte, sans modifier les rejets : publier l'histogramme des
rectangles résiduels par `|A|`, `|B|` et masse, puis exécuter le nouveau
certificat **en observation** sur coupes capteur 8k/16k/32k d'une même
trame sans sol. Compter tâches, cellules, nœuds/témoins, tests de coins,
masse de paires prouvée avant expansion, replis, temps et RSS. Juger
ensuite le **travail total** `certificats + paires terminales + covers +
formes + catalogue + FULL`, avec identités de sorties et de coquilles,
avant activation. Répéter sur trames entières et brutes de plusieurs
séquences, K5/K10, s8/10/12. Ni le coût des cellules ni la masse
résiduelle ne bénéficient aujourd'hui d'une borne sous-quadratique
générale ou d'une validation LiDAR.
