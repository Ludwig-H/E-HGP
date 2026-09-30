# Totalité de la bande de paires normalisée par alpha — K3/K5

Petit paquet privé Fraction. Aucun moteur, export natif, GammaForest, GPU
ou GCP importé/exécuté. Les niveaux de paires sont calculés exhaustivement
sur cinq points, et les MEB sont prouvées analytiquement. On distingue la
règle PROPOSÉE hardband-alpha à K>=3 du prototype MMp déjà écrit : ce
dernier utilise min ell comme échelle et ne souffre pas de ce W=0.
L’implémentation K2 existante n’est pas déclarée défectueuse par ce paquet.

## Définitions et sources

K inclut le point le plus proche quand le centre d’évaluation est un site.
Pour y!=x, ell_K(x,y)^2=max(|xy|²/4,D_K((x+y)/2)²), où D_K du milieu
est le K-ième ordre exact des n distances à tous les sites. Bande dure :
ell²<=alpha²(1+eta)², fermée. Une majorité nécessite W>0.

Les fonctions réelles poids_bande (mmc.py228–233) et votes_paires
(333–359) sont copiées intégralement et compilées AST sans modification.
Seul le contexte gf est une petite table de K-parties analytiques, avec
IDs de nœuds factices : on juge niveaux/échelle/poids, PAS les propriétaires
FULL ni un clustering. Les docs statistiques et majorités continues,
mmc.py intégral et paires_k2.py sont hachés avant/après la capture.

## FX-A9, K3

Sites collinéaires0,2,7,10,13, x=10. MEB d’un triplet = demi-étendue ;
alpha=min des MEB contenantx=3, avec{7,10,13}. Votes vers0/2/7/13 :
ell=5 /4 /9/2 /9/2. À eta1/4, bord=(5/4)alpha=15/4<4 : W=0.
Contrôle fermé à eta1/3 : vote vers2 entre exactement, W=1 ; à eta1/2,
les votes vers7 et13 entrent aussi, W=3. La fonction réelle votes_paires
renvoie A=min ell²=16, pas alpha²=9. A sert à sa bande SOUPLE.

## K5, point intérieur d’un tétraèdre régulier

x=(0,0,0), quatre autres sites (1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1).
Pour tout centre c, moyenne des quatre distances carrées =3+|c|²,
car la somme des quatre sommets est0. Leur maximum est donc>=3.
c=0 atteint3 et couvre aussix : MEB des cinq sites alpha²=3.

Au milieu y/2 d’une paire x,y : distances carrées àx ety=3/4 ; à
chacun des trois autres sommets=19/4 (produits scalaires y·z=-1).
D5²=19/4, donc les quatre ell5²=19/4. Le bord dur eta1/4 vaut
75/16<76/16=19/4 : W=0. Seuil exact eta*=sqrt(19/12)-1.
Contrôle au facteur carré19/12 : les quatre votes entrent sur la
coquille ; eta13/50=.26 est un contrôle rationnel au-dessus.
Le même nuage àK2 donnealpha²=3/4 et quatre votes ell2²=3/4 : nonvide.
Translation+(1,1,1) donne cinq sites u18 exacts0/1/2 et le même résultat.

## Borne générale et politique à choisir

Un K-ensemble F contenantx de MEB B(c,alpha) existe. Choisir y dans
F\{x} : son milieu m est dans cette boule par convexité ; tous les
K sites de F sont à distance<=2alpha de m, donc D_K(m)<=2alpha,
|xy|/2<=alpha. Par conséquent alpha<=min_yell_K(x,y)<=2alpha,
ou alpha²<=A<=4alpha². Cela ne rend PAS une bande eta1/4 nonvide.
Une bande alpha élargie àeta>=1 serait toujours nonvide, mais cela
change la cible (les autres fixtures montrent que les votes lointains
peuvent faire attendre). Remplacer alpha parsqrt(A) change aussi la règle.
Le développeur doit choisir explicitement une définition totale/fallback,
la mesurer et la valider statistiquement, pas effectuer un raccord implicite.

## Captures et portée

Code et données exhaustifs autonomes ; résultats normal/−O identiques.
Deux mutants causaux sont rejetés : identifierA etalpha², et ouvrir
la bande à la coquille exacte. Reader avec SHA manifeste externe exigé
avantJSON/replay, inventaire fermé7 fichiers, hashes après relecture.
Pas de chrono, de résultat LiDAR, de contratFULL/G4 ni de test deMMt.
Les anciens paquets packing/bord de bande sont préservés intacts.
