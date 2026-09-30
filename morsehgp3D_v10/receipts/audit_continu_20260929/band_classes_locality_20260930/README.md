# Classes dans la bande : réponses exactes Q1 et Q2

Capture Fraction du 30 septembre 2026, 18:05:34–18:05:35 UTC. Aucune
exécution du moteur, mesure LiDAR ou GCP. Le script privé, identique avant
et après les deux commandes, vérifie six paramètres rationnels et quinze
classes. Les sorties normal et `-O` sont identiques et de code zéro.

## Q1 : voisinage de portée, pas seulement K voisins

Si x appartient à F et si la boule minimale B(F) a un rayon au plus R,
tous les sites de F, et tous les sites contenus dans B(F), appartiennent
à B(x, 2R). Cette observation borne p par l'occupation de ce voisinage.
Si m compte ses sites, x compris, le nombre de classes est au plus
Σ(j=1..4) C(m,j) : en dimension trois, une boule minimale possède un
support d'au plus quatre sites. Ce n'est pas une borne en fonction de K.

Pour K=3, prendre x=0 et m sites
u(t)=((1−t²)/(1+t²), 2t/(1+t²), 0), avec 0<t<1/4, tous distincts.
Chaque triangle {x,u(t_i),u(t_j)} est strictement aigu. Son rayon carré est

β_ij = (1+t_i²)(1+t_j²) / [4(1+t_i t_j)²].

En effet les trois sommets sont sur la boule circonscrite et ses angles
sont aigus ; le calcul rationnel de son centre donne cette formule.
L'identité du numérateur donne
1/4 < β_ij = [1+(t_i−t_j)²/(1+t_i t_j)²]/4 < 17/64.
Comme chaque F contenant x contient un autre site à distance un,
α_x² ≥ 1/4. Ainsi β_ij < 17/64 < 9/32 ≤ (9/8)α_x² : toutes les classes
sont dans la bande R=√(1+η′)α_x, même avec η′=1/8.

Les centres sont tous distincts, sans hypothèse générique : un centre c
commun imposerait c·u=1/2 à trois points distincts du cercle unité,
alors qu'une droite coupe ce cercle en au plus deux points. Il y a donc
C(m,2) classes. Après tri des paramètres, le nombre de sites du cap
strictement intérieurs à la boule ij est p=j−i−1. Le test vérifie ce compte.
Des perturbations radiales rationnelles suffisamment petites enlèvent les
ex aequo de distances sans perdre ces inégalités ni ces classes distinctes.

Cette famille met en défaut une garantie générale sous-quadratique par
énumération explicite de toutes les classes de bande. Elle ne prouve
aucun comportement quadratique du régime LiDAR, et n'exclut pas un
comptage implicite plus rapide.

## Q2 : IDs fixes et requêtes adaptatives

Les rayons de boules minimales de K-parties étiquetées fixes sont
1-lipschitziens pour un déplacement maximal δ des sites ; leur minimum
α_x l'est aussi. La sélection des IDs par KNN, en revanche, peut changer
au passage d'un ex aequo au rang K. La continuité du rayon KNN ne rend
pas cet univers d'IDs continu.

Un oracle limité aux K voisins de x ne détermine pas les votes exacts.
Pour K=3, la base {0,(1/2,0,0),(1,0,0)} a α_x=1/2. Ajouter m sites
(101/100)u(t) conserve ces trois plus proches voisins et α_x : toute
K-partie utilisant un nouveau site a un rayon au moins 101/200>1/2.
Cependant les C(m,2) triangles avec x sont tous dans la bande puisque
(101/100)²·17/64 < 9/32. Avec le même nombre total de sites placés loin,
ces votes de bande sont absents, malgré les mêmes KNN à x et le même α_x.

L'univers exact reste celui des K-parties étiquetées, ou une représentation
équivalente de leurs masses. Une requête de portée B(x,2R) peut fournir
le voisinage nécessaire ; des requêtes KNN adaptatives peuvent l'émuler,
mais leur taille retournée n'est pas bornée par K. Rien ici ne démontre
l'impossibilité d'un algorithme implicite utilisant tout le nuage ou des
requêtes à plusieurs centres.

## Lecture du reçu

Le manifeste épingle ce texte, le script et les deux captures. Le lecteur
strict vérifie le manifeste, les empreintes et les sorties, puis rejoue
le petit script en normal et `-O`, sans dépendance au moteur.
