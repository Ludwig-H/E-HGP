# Inverse du rayon carré : une incidence de coquille change la normalisation

30 septembre 2026. Audit mathématique K2 sur quatre sites, sans invocation
du moteur, sans changement de source de production, sans GCP.

## Résultat vérifié

Une majorité stricte avec poids fixes `1/β`, sur les boules fortes canoniques
`population≥2` et `p+q_min≤2`, reste laminaire sur chaque arbre fixé.
**Elle n'est pas continue lorsque son univers d'incidences est recalculé
après perturbation.** Cela ne contredit pas la preuve conditionnelle qui
suppose les mêmes témoins et les mêmes poids dans les deux arbres.

Le nuage est le tétraèdre

`C=(0,0,0), A=(4M,0,0), B=(0,5M,0), D=(0,0,100M)`.

Déplacer seulement C vers `(1,1,1)`. Les tétraèdres ont une dimension affine
trois. Les quatre valeurs `M=10,100,1024,2048` sont dans le domaine u18.
Le déplacement apparié vaut `sqrt(3)` unité de grille.

Avant déplacement, il y a six atomes forts. Leurs carrés de rayons sont
`4M²,25M²/4,2500M²,41M²/4,2504M²,10025M²/4`.
Tous sont incidents à C : les trois premiers l'ont dans leur support,
les trois derniers dans leur coquille complète. À la naissance de CA,
la masse de cette branche vaut `1/(4M²)`. Elle est inférieure à la moitié
de la masse totale de C. C reste singleton jusqu'à la fusion ABC,
`β=41M²/4`, bien qu'il soit couvert dès `β=4M²`.

Après déplacement, C est strictement intérieur aux diamètres AB, AD et BD :
leur puissance est respectivement `3−9M`, `3−104M`, `3−105M`.
Leurs paramètres sont maintenant `p=1,q_min=2`, donc ils ne sont plus des
atomes **forts de couverture K2**. Les trois diamètres CA, CB et CD restent
forts. CA porte désormais plus de la moitié de la masse de C : les points
C et A sont réunis dès `β=((4M−1)²+2)/4`.

**Ces trois événements ne doivent pas être supprimés de FULL.** Le
catalogue K2 admet encore `p+q_min=K+1=3`. L'oracle Γ complet confirme
notamment que la fusion ABC reste exactement `β=41M²/4` avant et après.
Le saut provient ici de la tête de points, pas d'une disparition de cette
fusion spatiale.

En coordonnées normalisées par M, le déplacement tend vers zéro ; la
hauteur projetée C/A en rayon passe de `sqrt(41)/2` à une valeur tendant
vers 2. C'est une discontinuité du modèle réel. Sur la grille finie u18,
`M=2048` donne une amplification, pas une limite continue à l'intérieur
du domaine fini : un déplacement de 1,732 unités change le rayon projeté
de 6556,799 à 4095,500 unités, soit 2461,299 unités.

À 1 mm, les nombres précédents sont en millimètres. Ce petit cas K2 avant
condensation n'est ni un résultat de clustering LiDAR, ni une qualification
de min_cluster_size, d'EOM ou du contrat FULL K5/G4.

## Vérification et portée des fichiers

`check_shell.py` calcule le catalogue critique exact avec le référentiel
Fraction figé, puis construit directement Γ à partir de toutes les 2- et
3-parties et de leurs MEB. Les masses et propriétaires sont rationnels,
sans IDs d'événements natifs. Les couvertures sont contre-vérifiées avec
la fonction indépendante `gamma_cuts` du même référentiel. Cela partage
la primitive géométrique MEB ; ce n'est pas une deuxième implémentation
indépendante de l'arithmétique.

Les lectures normale et `python3 -O` terminent avec code 0. Leurs huit
nuages et résultats sémantiques sont identiques ; chaque mode effectue
576 contrôles de non-scission de paires. Le code 0 signifie que le
contre-exemple est reproduit, pas qu'une nouvelle tête est qualifiée.
Sources d'audit et référence sont hachées avant/après chaque exécution.

Commande de rejeu :

`python3 -B check_shell.py frozen_reference.py`

La capture d'origine a utilisé le même fichier de référence, au chemin
absolu indiqué dans les reçus. Le snapshot local a le même SHA256. Les
reçus contiennent les résultats complets et les paramètres, pas un digest
qui remplacerait Γ. Aucun binaire n'est requis.

## Conséquence pour la conception

Changer seulement les poids uniformes en `1/β` répare les deux paires
lointaines de la capture précédente, mais ne résout pas le caractère
combinatoire des incidences/coquilles du catalogue. À perturbation fixée,
un univers donné et une majorité stricte restent un arbre ; la robustesse
entre deux univers est une autre exigence.

Une masse intrinsèque sur des parties **étiquetées** conserverait les
incidences lorsque le catalogue change. L'univers de toutes les K-parties
est cependant interdit comme architecture de production : sa taille est
combinatoire. Le mentionner éclaire l'obstacle, sans le déplacer vers une
énumération cachée. De plus, des poids géométriques variables peuvent
encore changer une décision près du seuil majoritaire.

La piste légère est un ancrage à la première couverture réellement
unique, après déduplication par composante, puis ascendance figée. Elle
réunit CA tôt dans ce cas, sans normalisation par des atomes futurs.
Une quasi-égalité de premières couvertures entre branches peut cependant
la rendre instable : il faut une marge déclarée ou publier l'ambiguïté,
pas annoncer une robustesse universelle.

La persistance de **branches** plutôt que le nombre de boules éliminerait
la dépendance aux subdivisions du catalogue si la correspondance des
branches est acquise. Mais une nouvelle incidence x→longue branche peut
apparaître près de sa mort sans que cette branche soit fantôme. Un poids
plus prudent serait `φ(t_firstcover(x,v))−φ(t_death(v))`, pour une φ finie
et décroissante, plutôt que toute la persistance de v. Ce n'est ici qu'une
piste conceptuelle. L'apparition d'un descendant couvert très brièvement
ne doit pas exclure discontinuement son ancêtre long du dénominateur.
Ancêtres futurs, antichaînes et normalisation restent donc à définir et
à tester. Aucun nouveau chantier moteur n'est justifié par cette capture.
