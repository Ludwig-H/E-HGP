# Petites portes autonomes proposées — aucune exécutée

`public_status=not_claimed`. Plan de qualification ciblé, pas ajout au
registre ni au CMake pendant le gel. Les preuves de largeur restent dans
le [grand-livre](ARITHMETIQUE_PRIMITIVES.md) : aucune quantité de tests
aléatoires ne les remplace. Le [reçu de port](../receipts/arithmetic_review_20260904/README.md)
conserve la provenance statique ; les chemins de source cités sont relatifs
à `morsehgp3D_v7/`.
Poser $M=65535$ et $R=2^{64}$.

## 1. Juges indépendants et non-vacuité

Deux portes suffisent pour un premier raccord, chacune de quelques
dizaines de fixtures déterministes, sans génération de nuage important :

1. **Formes et Cramer u16** : oracle OBig384 à limbs 32 bits ; calculer les
   déterminants par les six permutations, pas par l'adjugée optimisée du
   produit. Pour q3, résoudre les deux équations 2d·v=D, 2u·v=E et la
   contrainte n·v=0 avec n=d×u dans l'oracle ; cela donne une forme
   proportionnelle à la forme de Gram, sans recopier sa construction.
   Comparer toutes les composantes par produits croisés, puis les clés
   primitives avec un PGCD binaire indépendant.
2. **Entiers, réductions et niveaux** : OBig384 contre des identités à
   limbs LITTÉRALES et, si disponible, une seconde autorité Boost cpp_int
   distincte. Le PGCD binaire et la division longue indépendants existent
   déjà dans `tests/linked_arcs_gate.cpp:80–139` : en faire une petite porte
   autonome explicitement portée, pas une dépendance au chemin produit.

OBig doit être qualifié sur les valeurs réellement traversées. Un produit
exact connu avec retenues au-delà de 160 bits doit réfuter son mutant
`obig-carry-lost`. Son drapeau de débordement doit interdire tout verdict.
La présence éventuelle de Boost doit être déclarée ; son absence ne peut
silencieusement transformer une porte « deux autorités » en succès.

Planchers à publier séparément : formes q2/q3/q4 non vides ; zéro de rang
q3 et q4 ; signe de det brut positif ET négatif ; centres dedans, dehors,
sur une face ; power négative, nulle et positive ; PGCD nul, 1, >2^64 ;
numérateur q4 ayant un mot U192 haut non nul ; produit U320 générique
ayant un mot w[4] non nul ; au moins un test par conversion/refus de domaine.
Un total global de cas ne remplace pas ces compteurs par obligation.

## 2. Géométrie : six petites familles précises

### G1 — q2, extrêmes et coquilles multiples

Points a=(0,0,0), b=(M,M,M). Clé attendue
$A=1$, $B=(-M,-M,-M)$, $C=0$, niveau $3M^2/4$.
Les huit coins du cube ont exactement une puissance nulle ; un point dont
les trois coordonnées sont strictement entre 0 et M a une puissance
négative. Inverser a/b doit conserver la clé. Ajouter une paire plus
petite, a=(0,0,0), b=(2,0,0), pour obtenir avec (M,M,M) une puissance
strictement positive et ne pas laisser cette catégorie vide.

### G2 — q3 équilatéral sur coordonnées entières

a=(0,0,0), b=(M,M,0), x=(M,0,M). On a
$D=E=X=2M^2$, $F=M^2$, $G=3M^4$ et
$W=(4M^5,2M^5,2M^5)$ ; rayon carré $2M^2/3$.
Le centre vaut $(2M/3,M/3,M/3)$ et est intérieur au triangle.
Comme M est divisible par 3, le PGCD de la forme brute est $3M^4$ :
la clé primitive a A=1 et B=(-4M/3,-2M/3,-2M/3), C=0.
Cette fixture fait traverser les bits hauts de G/W AVANT réduction.

Ajouter x'=(M,0,M-1) et les permutations des trois coordonnées pour casser
les grands facteurs communs. Les valeurs attendues doivent alors provenir
du juge Cramer OBig, pas être recalculées avec q3_form du produit.
Conserver les valeurs de référence exactes dans le futur reçu.

### G3 — frontières q3 et centres extérieurs

- Colinéaire : (0,0,0), (2,0,0), (1,0,0), G=0. Former G est autorisé ;
  ne pas appeler la division du helper axis_min sur ce résultat.
- Rectangle : (0,0,0), (2,0,0), (0,2,0). Le triangle n'est pas un support
  q3 strict ; la boule est portée par son hypoténuse.
- Obtus : (0,0,0), (4,0,0), (1,1,0), centre (2,-1,0). Le calcul brut du
  circumcentre reste exact, mais l'émission q3 stricte doit être refusée.

Le test de rang/signe doit être celui du produit appelé ou un validateur
de fixture explicitement nommé. Ne pas présenter le rejet par le juge
comme un statut que q3_form lui-même ne rend pas.

### G4 — tétraèdre régulier, grands numérateurs et égalité inter-lanes

a=(0,0,0), b=(M,M,0), x=(M,0,M), y=(0,M,M).
Le déterminant brut est négatif ; après canonisation,
$\mathrm{det}=16M^3$ et $N'=(8M^4,8M^4,8M^4)$.
Le centre vaut (M/2,M/2,M/2), strictement intérieur, et le niveau est
$192M^8/(256M^6)=3M^2/4$. La clé primitive est celle de G1.

Le niveau q4 brut a un numérateur dépassant 128 bits et un dénominateur
dépassant 64 bits. Le comparer au q2 de G1 doit rendre l'égalité sémantique
malgré des représentations différentes. Permuter deux sommets inverse
le déterminant brut sans changer la clé ni le niveau final. Parcourir les
24 permutations pour vérifier tous les signes de face et la canonisation.
Le mot U320 w[4] doit néanmoins rester nul : ce n'est pas une fixture de
couverture du cinquième mot, mais une fixture géométrique des autres mots.

### G5 — Cramer petit déterminant, centre lointain, rejet strict

a=(0,0,0), b=(M,1,0), x=(M-1,1,0), y=(0,0,1).
Le volume orienté vaut 1, donc le déterminant vaut 8. Le centre exact est
$(M-1/2,(-M^2+M+1)/2,1/2)$, situé hors du tétraèdre.
Les numérateurs canoniques attendus sont
$(8M-4,4(-M^2+M+1),4)$. Cette fixture sépare la sûreté de Cramer brut
de la condition géométrique de bien-centrage : le second doit être faux.

### G6 — det zéro et centre sur une face

Coplanaires : (0,0,0), (M,0,0), (0,M,0), (M,M,0), det=0.
Pour une égalité de face non dégénérée, choisir a=(0,0,0), b=(4,0,0),
x=(2,3,0), y=(2,0,2). Le centre $c=(2,5/6,0)$ est dans l'intérieur
relatif de la face AIGUË abx ; les quatre distances carrées à c valent
$169/36$. Le volume vaut 24, donc le déterminant vaut 192 et
$N'=(384,160,0)$. Le prédicat strict doit rejeter exactement ce zéro de
face, sans que le tétraèdre soit coplanaire ou le triangle rectangle.

## 3. PGCD et canonicalisation : cas ciblés

| Cas | Résultat attendu / obligation |
| --- | --- |
| ugcd128(0,0), ugcd128(0,2^128-1), ugcd128(2^128-1,0) | Respectivement 0 et deux fois 2^128-1 ; aucun modulo par zéro |
| ugcd128(R+1,R-1), arguments inversés | 1 ; passage 128→64 non symétrique couvert dans les deux sens |
| ugcd128(2^127,R) | R ; cas PGCD non représentable en u64 mais représentable en i128 |
| ugcd128(2^127+1,R) | 1 ; transition vers un reste très petit |
| Couples voisins de Fibonacci jusqu'au dernier représentable en u128 | PGCD 1 ; chaîne longue, valeur attendue par identité, pas par le PGCD produit |
| uabs128(INT128_MIN), uabs128(-1), uabs128(0) | 2^127, 1, 0 ; aucune négation interdite dans le juge |
| BallForm A=6, B=(12,18,24), C=5 | PGCD final 1 ; C doit participer après les B |
| BallForm A=6, B=(12,-18,24), C=-30 | Clé (1,(2,-3,4),-5) ; signes préservés, exactitude de toutes les divisions |
| BallForm A positif, B=0, C=0 | Clé (1,0,0) ; le PGCD vaut initialement A |
| Rational128 6/8, -6/8, 0/8 | 3/4, -3/4, 0/1 ; ne pas passer la fraction négative au comparateur unsigned |

Ces fixtures génériques de réduction ne prétendent pas toutes être
produites par une lane u16. Le contrat A>0 / den>0 suffit à la preuve de
réduction sur ce domaine numérique, contrairement à la puissance d'une
clé, qui a des préconditions de largeur supplémentaires.

## 4. U192/U320 et comparateurs : séparer les domaines

### W1 — U192 avec retenues et précondition de capacité

Produits valides : $(2^{96}-1)^2$ et
$(2^{127}-1)(2^{64}+1)$. Tous les mots doivent être comparés à OBig et à
des identités symboliques, pas seulement à la commutativité.
Le produit $2^{96}\cdot2^{96}=2^{192}$ est HORS précondition U192 : le
validateur de fixture le refuse AVANT appel. Le produit ne comporte pas
de contrôle de capacité et ne doit pas recevoir crédit pour ce rejet.

### W2 — cinquième mot U320, vrai domaine générique

Prendre $n=R^3-1$ et $d=R^2-1$. Le produit vaut
$R^5-R^3-R^2+1$, soit, de bas en haut,
$[1,0,R-1,R-2,R-1]$. Ce littéral qualifie les reports du produit U320
et sert à juger OBig au-delà de 160 bits, sans recycler les produits du
code testé dans la référence. La non-vacuité exige w[4] non nul.

La variante ExactLevel doit conserver den>0 en i128 : comparer
$x=2^{190}/1$ à $y=1/2^{126}$. Le produit croisé gauche vaut 2^316,
le droit 1. Mettre à zéro w[4] inverse la comparaison au lieu de seulement
changer un octet invisible. Ces niveaux sont valides dans le domaine
numérique du comparateur, mais NE proviennent PAS de supports u16.

### W3 — somme de carrés et garde collective de fixture

Cas valide : trois fois $(2^{95}-1)^2$ ; comparer chacun des trois mots et
imposer des reports non nuls. Cas hors capacité collective :
$a=b=3\cdot2^{94}$ et $c=0$. Chaque carré tient individuellement en
U192, mais leur somme $18\cdot2^{188}$ dépasse 2^192. Le validateur
doit rejeter la SOMME, pas seulement chacun de ses termes. Le produit ne
rend aucun statut de refus ici : ne pas fabriquer une garantie d'API.

### W4 — égalités de niveaux et contrats négatifs

Comparer 1/2 à 2/4, les zéros 0/1 et 0/7, puis une paire différant d'une
unité seulement dans un mot haut du numérateur. Vérifier l'antisymétrie,
les égalités sémantiques et les inégalités de représentation attendues.
La paire q2/q4 G1/G4 assure en plus une égalité provenant de vraies lanes.

Préconditions négatives : den=0, den<0 et num<0 pour compare_rational,
ainsi que produit croisé hors U192 pour compare_rational. Exemple démontré
de dépassement : $x=2^{100}/1$, $y=0/2^{100}$ ; le produit croisé gauche
vaut 2^200, qui disparaîtrait d'un U192, donnant une fausse égalité.
Ce n'est PAS un contre-exemple sur le domaine q2/q3 admis ; c'est la
raison d'écrire la précondition de ce comparateur bas niveau.

Le helper floor_div128 a deux entrées interdites à valider SANS appel :
den=0 et `(INT128_MIN,-1)`. Cas définis à tester séparément :
(-5,2)→-3, (5,-2)→-3, (-4,2)→-2, (INT128_MIN,1)→INT128_MIN.
La correction de quotient doit rester distincte de la validation du domaine.

## 5. Mutants proposés et statut futur

Les mutants ci-dessous sont des propositions, ABSENTS du registre courant :
oubli d'un cofacteur Cramer, défaut de canonisation d'un seul N', omission
de C dans le PGCD, narrowing prématuré du reste Euclide, report milieu
U192/U320 perdu, confusion égalité de représentation/sémantique.
Chacun doit avoir un point d'injection dans la vraie primitive, une porte
indépendante positive et une divergence nommée avec plancher pertinent.
Un mutant d'oracle doit être réfuté par des littéraux/une autre autorité,
pas par le produit qu'il était censé juger.

Le mutant EXISTANT `level-trunc-hi` peut déjà exercer W2 sans changement
de source, si la porte appelle directement U320 ou compare_exact_level
avec des numérateurs littéraux (sans construction par somme U192).
Cela isole son deuxième site causalement malgré le nom partagé. Une
scission ultérieure des deux noms reste préférable pour la traçabilité.

Ordre proposé après GO : construire ces deux petites portes dans un
overlay, vérifier d'abord les juges, ensuite les nominales, puis chaque
mutant et chaque refus de fixture ; enfin ASAN/UBSAN. Aucun besoin de GCP.
La réussite qualifie des exécutions bornées de primitives déjà prouvées
sous leurs préconditions, jamais l'ensemble du pipeline par extrapolation.
