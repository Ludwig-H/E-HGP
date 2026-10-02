# Robustesse géométrique de FULL et des attaches frontière

2026-10-02 10:01:13 UTC. Dérivation indépendante ; témoin analytique Fraction,
sans moteur natif, sans voie A/B de l'oracle et sans GCP. Cette preuve ne
qualifie ni un générateur ni un modèle statistique de clustering.

## 1. Ce qui est stable sous une erreur géométrique bornée

Apparier TOUS les retours de X=(x_i) et Y=(y_i), même N, dans le même repère
physique, avec max_i ||x_i−y_i||≤ε. Compter chaque retour, y compris ceux qui
fusionnent en sites ; aucune bijection des sites n'est exigée. Pour 1≤k≤N,
chaque distance au même centre z bouge d'au plus ε. Le k-ième ordre aussi :
si chaque entrée d'une liste change d'au plus ε, au moins k entrées restent
sous son ancien k-ième ordre +ε ; inverser les listes donne l'autre inégalité.

Donc |sqrt(D_k^X(z))−sqrt(D_k^Y(z))|≤ε pour tout z, et
L_k^X(r²)⊆L_k^Y((r+ε)²), avec la même inclusion en sens inverse.
Une composante connexe s'envoie dans l'unique composante qui la contient.
Les cartes se composent en l'inclusion à r+2ε, et commutent avec les
verticales k+1→k : c'est un ε-interleaving en rayon du diagramme des
composantes de la région continue. Cela ne donne pas de bijection des
boules critiques, supports, coquilles ou nœuds du catalogue.

La relation cover DYNAMIQUE à chaque r est stable dans le même sens : si
C∩B(x_i,r) contient z, son image contient encore z, et
z∈B(y_i,r+ε). Le centre témoin peut être quelconque dans la population
continue : restreindre aux centres MEB ne bénéficie pas de cette preuve.

La date de première couverture du retour i est
α_i(X)=min_z max(||z−x_i||, sqrt(D_k^X(z))). Le minimum est atteint par
continuité/coercivité. Les deux termes changent d'au plus ε, puis leur max
et leur minimum aussi : |α_i(X)−α_i(Y)|≤ε. En niveau carré β, la borne est
|β_X−β_Y|≤ε(sqrt(β_X)+sqrt(β_Y)), pas une erreur additive ε uniforme.

Pour une grille isotrope h, arrondi certifié au plus proche et sans clipping,
ε≤sqrt(3)h/2. Restituer l'origine dans le repère commun et les multiplicités.
Le refus natif actuel des entrées pondérées ne devient pas qualifié par
cette propriété mathématique.

## 2. Ce qui peut changer fortement : ensemble figé au premier instant et LCA

X={0,2,4} sur un axe, K2 : les deux lentilles AB et BC naissent à r=1,
fusionnent à r=2. Le retour médian 2 entre à r=1 dans deux composantes ;
la projection par premier ancêtre commun, datée de cet ancêtre, l'attache
à la racine à r=2.

Y={0,2,4+δ}, δ>0 arbitrairement petit : AB naît à 1, BC à 1+δ/2 et la
racine à 2+δ/2. Le retour médian entre encore à 1, cette fois dans AB seul ;
sa projection LCA est AB, datée 1. Déplacement maximal δ, dates FULL
modifiées d'au plus δ/2, mais date d'attache projetée changée de 1.
L'équivariante projection conservatrice reste une baseline valide ;
l'équivariant et le laminaire ne signifient pas stable sous perturbation.

Le cover à TOUS les rayons satisfait §1 ; figer l'ensemble des composantes
au PREMIER instant ne satisfait pas automatiquement ce contrat. Séparer ces
objets dans l'API/rapport. Ne pas déduire stabilité des masses, mcs ou labels,
ni faire porter la preuve sur une projection ou une sélection ultérieure.

## 3. Fixtures et portée

`check.py` suit uniquement les rayons analytiques des lentilles collinéaires
et de leur intersection triple. Trois couples entiers u18 sont proposés,
avec pas physique 1/M et M=4,256,32768. Pour M=32768, le dernier retour
passe de 131072 à 131073 : tous les mots restent u18. Les dates au carré
s'obtiennent exactement en élevant les rayons au carré ; aucun arrondi.
Le vrai différentiel FULL/points sur G4 reste à intégrer.

Les sorties normal/−O et leurs commandes sont conservées. Quinze gardes
arithmétiques n'exécutent ni le catalogue ni la projection du produit ;
la preuve de §1 est une dérivation, pas une conclusion de ces quinze gardes.
