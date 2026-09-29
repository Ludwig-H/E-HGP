# Contre-vérification : squelette laminaire et projection multi-K

29 septembre 2026. Moteur `6206d1d11` intact, `public_status=not_claimed`.
Complément de l'audit pool/tête, centré sur la nouvelle question de projection des
points depuis FULL. Aucun nouveau générateur, aucune dépense GCP.

## 1. FULL multi-K n'est pas déjà un arbre unique sur les points

Contre-exemple natif exact : cinq points collinéaires d'abscisses 0, 20, 22, 50, 52.

- À K = 1 et rayon 10, C∩X donne `{0,20,22}` et `{50,52}`.
- À K = 2 et rayon 15, C∩X donne `{20,22,50,52}` ; 0 n'est pas encore entré.

Les ensembles `{0,20,22}` et `{20,22,50,52}` se croisent : ils ne sont ni disjoints
ni inclus l'un dans l'autre. Il est donc impossible de conserver toutes ces composantes
comme nœuds d'une même hiérarchie de partitions. Les verticales exactes ne l'empêchent
pas : les paramètres `(K=1,r=10)` et `(K=2,r=15)` sont incomparables pour l'inclusion.

Le petit exécutable compilé depuis le snapshot produit ces deux coupes, code 0 :
[`crossing_report.json`](../../../receipts/audit_continu_20260929/pool_head/crossing_report.json), [`crossing.txt`](../../../receipts/audit_continu_20260929/pool_head/crossing.txt).
Rejeu : `probe_multik.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY`.
Cette contradiction concerne une fusion naïve des sorties, pas l'exactitude de FULL.

## 2. Ce qui donne effectivement un squelette laminaire

**Core à K fixé.** Les composantes de L_K(r) sont emboîtées lorsque r croît ; leur
intersection avec X reste emboîtée. Chaque point entre à sa distance K-NN et suit
une unique composante. Avant son entrée, le laisser singleton complète les partitions
sur tout X, mais ce singleton est une convention de sortie : pas une composante déjà
existante de L_K. Sa masse ne doit pas être antidatée dans l'EOM.

**Tranche monotone de la tour.** Si r ne diminue jamais et K n'augmente jamais,
les inclusions sont composables. Cette tranche fournit encore un squelette exact.
Des choix libres de K et de r par composante ne bénéficient pas de cette propriété.

**Affectation durable sur un squelette.** Attacher chaque point une seule fois à un
nœud, puis suivre les parents, conserve la laminarité. En cas de plusieurs candidats,
leur plus petit ancêtre commun est un lieu d'attache cohérent, avec deux précautions :

1. La date devient au moins la naissance de cet ancêtre.
2. Si la date proposée dépasse sa mort, prendre son ancêtre vivant à cette date.

Une telle affectation peut modifier l'appartenance géométrique. Laminarité exacte
ne signifie donc pas fidélité aux composantes de multicouverture après affectation.
Prendre le LCA uniquement aux égalités exactes ne supprime pas la discontinuité
de première couverture : des choix presque égaux peuvent déjà être sur deux branches.

## 3. Garantie limitée de stabilité pour core

Supposons une correspondance entre deux nuages de même taille telle que chaque point
se déplace d'au plus ε. À centre fixe, toutes les distances, donc la K-ième distance,
changent d'au plus ε. Un chemin de L_K du premier nuage au rayon r appartient ainsi
au niveau r+ε du second. Pour relier ses extrémités aux nouveaux points correspondants,
les deux petits segments ajoutés demandent au plus ε supplémentaire.

Par conséquent, la date de réunion core de deux points étiquetés, **en rayon**, change
d'au plus 2ε ; l'argument inverse donne la valeur absolue. Les dates d'entrée vérifient
la même borne. C'est une stabilité déterministe pour bruit de position apparié, pas
une garantie face aux points aberrants, à un changement d'effectif, à l'EOM ou à une
affectation supplémentaire. En rayon carré, une constante dépendant de la portée
est nécessaire.

## 4. Consensus d'ultramétriques : vrai, mais pas gratuit ni automatiquement utile

En conservant à part les dates d'entrée, chaque arbre core donne une ultramétrique
sur les paires distinctes, diagonale nulle. Le maximum de plusieurs ultramétriques
est encore une ultramétrique : appliquer l'inégalité forte dans chaque vue puis
prendre le maximum. Le maximum est non expansif pour la norme uniforme.

Cependant, sans recalibration, les ultramétriques core sont croissantes avec K,
puisque L_{K+1}(r) est inclus dans L_K(r). Leur maximum est simplement celle du
plus grand K : aucun gain multi-K. Des recalibrations fixes croissantes conservent
l'inégalité ultramétrique ; une garantie quantitative de stabilité exige en plus
leur continuité contrôlée, par exemple une constante de Lipschitz. Un recalibrage
appris sur le nuage n'hérite pas automatiquement de cette borne.

Moyenne, minimum et médiane ne conservent pas en général l'ultramétricité. Sur trois
points A,B,C, prendre une première vue `(AB,BC,AC)=(1,2,2)` et une seconde
`(2,1,2)` : minimum et moyenne violent l'inégalité forte. Ajouter la vue `(1,1,1)`
donne une médiane `(1,1,2)`, également invalide. Une fermeture par liaison simple
répare la forme, mais peut réunir une chaîne malgré l'absence d'accord direct sur
ses extrémités ; elle change donc le sens du consensus.

**Recommandation bornée.** Garder core à K fixé comme témoin structurel stable ;
comparer ensuite une tranche monotone ou une affectation durable clairement déclarée.
Ne pas ajouter un consensus plus coûteux avant d'avoir mesuré un gain reproductible.
Ni ces preuves de structure ni les petits contre-exemples ne démontrent un gain
statistique sur HDBSCAN ou une supériorité uniforme.
