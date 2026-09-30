# Grille décimale et erreur des niveaux géométriques

30 septembre 2026. Audit mathématique borné, pas un moteur v10 large.
Sources copiées à hashes identiques : préparateur exact v8 et oracle MEB
v10. Aucun natif, GPU ou GCP ; aucun chrono de tour.

## Propriété mathématique

À univers de points étiquetés fixé, si chaque point se déplace d'au plus
ε, le rayon de sa plus petite boule englobante, pour chaque sous-ensemble,
change d'au plus ε. En effet, la même boule agrandie de ε englobe tous
les points déplacés ; l'argument inverse donne l'autre inégalité.

Pour une grille de pas h, ε est au plus la demi-diagonale d'une cellule,
soit √3·h/2. Sur les graphes Γ_K en rayon, les sommets K-parties et les
arêtes K+1-parties de la coupe r restent donc présents à r+ε dans
l'autre nuage. Cela donne un entrelacement des composantes intrinsèques,
**pas une identité des coupes**, ni une stabilité des attaches dures,
des décisions EOM ou de l'ARI.

L'univers étiqueté est essentiel : fusionner deux retours en un seul site
et ne pas conserver leurs multiplicités change l'estimateur K-NN.
Diffuser une étiquette aux deux retours ne rétablit pas sa densité.
La propriété n'est pas transférée au FULL v10 pondéré, encore refusé.

## Micro-jugement exact

Trois nuages 3D de quatre ou cinq retours sont préparés aux pas 1 mm,
0,1 mm et 0,01 mm depuis leurs mots float32 décodés, sans tirer les
coordonnées fines d'une ancienne grille. Chaque retour est reconstruit
avec sa correspondance et la translation entière commune.

Les 117 arrondis sont comparés à une division rationnelle indépendante.
Pour les 183 couples de rayons MEB de tous les sous-ensembles, le juge
vérifie sans racine flottante l'inégalité |√a−√b|≤√e, où e=ε² :
si a+b−e est positif, son carré doit être au plus 4ab.
Normal et `-O` donnent les mêmes observations.

Les cas exercent deux profils avec fusions, deux déplacements de coupe
capteur et six origines encodées hors plage u32. Ces derniers ne sont
pas des dépassements des coordonnées stockées : l'origine est une
métadonnée signée, distincte des coordonnées locales.

Le test énumère des sous-ensembles seulement pour ce petit oracle.
Il ne mesure aucune croissance de candidats, qualité de clustering,
FULL natif ou performance à grande taille.
