# Supports : stabilité et incidence, modèle indépendant

Lecture de conception du 4 octobre 2026, sans exécution native. Le script emploie
uniquement la bibliothèque standard et Fraction. Les gardes sont actives sous -O.
Le modèle reconstruit les supports par Gram/barycentres, les MEB des petits
ensembles et les composantes du graphe à cofaces de cardinal K+1.

## Un carrier canonique peut être discontinu

Sur le cercle unité, prendre A=(1,0), B=(0,1), C=(-1,0), D=(0,-1).
La famille Q_b contient seulement les deux diamètres AC et BD. Remplacer B
par B_t=(2t/(1+t²),(1-t²)/(1+t²)), avec 0<t<1/2. La boule ne change pas.
Q_b contient alors AC et le triangle strict B_tCD.

Pour b_x,b_y les coordonnées de B_t, les poids de son centre sont
(1,b_x,b_y)/(1+b_x+b_y), tous positifs. Le point fixe w=(-1/4,1/4) possède
dans B_tCD les poids (1/s,b_x/s+1/4,b_y/s-1/4), s=1+b_x+b_y.
Ils sont strictement positifs car 3b_y>1+b_x équivaut à 2t²+t-1<0.
Donc w appartient au nouveau carrier, alors que sa distance aux deux
diamètres anciens est 1/4. La distance de Hausdorff est au moins 1/4,
malgré un déplacement d'entrée 2t/sqrt(1+t²) tendant vers zéro.

Cela ne contredit pas l'entrelacement de FULL : la stabilité de l'arbre
n'implique pas celle de cette réalisation géométrique. Le choix exact Q_b
reste cohérent ; sa robustesse est un contrat séparé à mesurer.
Le modèle vérifie neuf cas rationnels t=1/n, leur immersion entière exacte
par l'échelle n²+1 et une translation commune. Tous tiennent dans u21,
jusqu'à n=1023. Le domaine u21 fixé est fini : on ne lui attribue pas
la limite analytique de perturbations arbitrairement petites.

## Deux gardes pour le futur export

Le carré (±1,±1), K2, niveau carré1 a quatre composantes de côtés distinctes.
Leurs supports géométriques partagent des sommets ; ces intersections
ne donnent pas les composantes FULL. Au niveau2, les six K-parties sont
dans une seule composante. À K1, sa racine naît au niveau1 mais ses
supports diagonaux n'apparaissent qu'au niveau2 : conserver leurs dates.

Le triangle aigu (0,0),(2,0),(1,2), K2, a trois composantes strictes avant
25/16, puis une au seuil fermé. La boule q3 a p=0,m=3. Sa fenêtre
p+qmin-1≤K≤p+m comprend K2 ; le prédicat strong de l'export points
p+qmin≤K≤p+m l'écarte. C'est un domaine différent, pas un défaut du
produit points actuel.

Exécution : `python3 -B -S check.py` et `python3 -B -O -S check.py`.
75 gardes, sorties identiques, stderr vides. Aucun oracle natif, chrono,
claim statistique ou GPU n'est acquis par ces gardes.

Une mutation scalaire admet les poids nuls comme supports positifs. Elle
est rejetée causalement par la première garde du carré, code1 normal/−O ;
source mutée et sorties sont conservées. Ce n’est pas un mutant natif.
