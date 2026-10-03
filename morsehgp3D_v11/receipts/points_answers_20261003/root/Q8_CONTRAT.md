# Q8 — date en rayon, comparateurs et budgets prouvés

3 octobre 2026. Proposition de contrat, pas de port ni qualification native.
Sources v11 : `num/budgets.hpp`, `num/integer.hpp`, `num/wide.hpp`,
`bench/points_export.cpp` relus. B désigne ici les bits de coordonnées.

## 1. Type de date et ordre des événements

Nom proposé : **PointRadiusDate**. Il référence trois `LevelRank` t,M,q du
même domaine immuable, pour la valeur √t+√M−√q. Invariants vérifiés :
0≤t≤q≤M, rangs valides, même propriétaire de niveaux. Pour une marge nulle,
M=q=0 est une représentation spéciale, ou utiliser M=q=t pour conserver
les trois inégalités. Le constructeur distingue explicitement ces deux cas.

Les rangs évitent de recopier trois `Level` par site ; trois u32 occupent
12 octets hors contexte/alignement (480 MB pour40 millions de sites, par
ordre). Owners, tris, incidences, arènes de travail et K ordres s'ajoutent.
Un budget massif ne peut ignorer ce facteur K. Le domaine doit vivre aussi
longtemps que les dates ; une date ne devient pas un rang FULL.

L'égalité est sémantique, pas celle du triplet : (2,162,50) et(8,98,32)
valent5√2. L'ordre est celui des réels algébriques exacts. Comparer à une
naissance FULL a se réduit à √t+√M contre √a+√q ; comparer deux dates peut
nécessiter six radicandes. Le traitement des plateaux doit rassembler les
événements FULL et les entrées de points à une même valeur exacte, suivre
les multifusions fermées et publier le propriétaire vivant au plateau.
Un indice original sert éventuellement à ordonner le stockage dans un
plateau, jamais à scinder ce plateau ou choisir un propriétaire géométrique.

`compare` retourne un résultat avec signe{-1,0,+1} ou refus motivé.
Si une limite de travail est atteinte, publier zéro est interdit ; aucune
exception Python ni sortie partielle ne constitue le contrat du module C++.
Le refus doit annuler la construction entière, avec scratch réservé dans
MemoryBudget. Un cache flottant n'a autorité que sous borne d'erreur prouvée.

## 2. Quatre racines : réduction entière sans extraction de racine

Pour chaque niveau a_i=N_i/D_i, 0≤N_i<2^A et1≤D_i<2^D.
Les budgets du moteur sont A=8B+12,D=6B+8 ; les fractions peuvent être
réduites mais la preuve ne l'exige pas. Poser P=∏_{i=1}^4 D_i et
n_i=N_i∏_{j≠i}D_j. Le facteur√P s'annule dans la comparaison.
Chaque n_i<2^L, L=A+3D=26B+36.

Comparer √n1+√n2 et√n3+√n4. Poser x=n1 n2,y=n3 n4,
U=n3+n4−n1−n2. Le signe recherché est celui de
2(√x−√y)−U. Si x−y et U ont des signes opposés, le résultat est immédiat
(zéros séparés). Sinon échanger les côtés si nécessaire pour x>y,U>0 :

    W=4(x−y)−U².

Si W<0, résultat négatif ; si W=0, égalité seulement lorsque y=0 ; si W>0,
comparer **W² à16 U² y**. Les carrés sont permis uniquement après ces
gardes de signe, sinon l'ordre n'est pas préservé.

| Valeur | Borne stricte de magnitude |
|---|---|
| n_i |2^L |
| x,y |2^(2L) |
| U |2^(L+1) |
| W dans le cas normalisé |2^(2L+2) |
| W² |2^(4L+4) |
|16 U² y |2^(4L+6) |

Le signe doit être stocké séparément de cette magnitude. Le comparateur ne
soustrait pas aveuglément les deux derniers produits : il compare leurs
magnitudes. Les arrondis de stockage à64 bits et les temporaires de produit
doivent eux aussi être réservés.

| Profil | A/D | L | Bits finaux prouvés | Mots64 minimum / produit après narrowing |
|---|---|---:|---:|---:|
|u18 |156/116 |504 |2022 |32/32 |
|u21 |180/134 |582 |2334 |37/38 |
|u24 |204/152 |660 |2646 |42/42 |

Pour le seul format exporté192/192, L=768 et le produit final est borné
par3078 bits (49 mots, temporaires de produits pouvant en demander50).
Ces bornes sont conservatrices, pas des minima ni des mesures de coût.
`Int<Bits>` est actuellement limité à1024 et `Wide<Words>` à32 mots :
la réduction ci-dessus dépasse la capacité actuelle dès u21. Ce constat
n'exclut pas un comparateur plus serré ; il interdit le raccord sans preuve.

## 3. Égalité par classes de carrés, sans factorisation

Deux radicandes a,b>0 sont dans la même classe si a/b est un carré rationnel.
Former N_a D_b et D_a N_b (<2^(A+D)), réduire par PGCD, puis vérifier par
isqrt que numérateur et dénominateur sont carrés. Leur racine rationnelle
q a numérateur/dénominateur <2^R, R=(A+D)/2=7B+10.

Grouper au plus six radicandes sur un représentant choisi. Les coefficients
signés sont des sommes d'au plus six tels rationnels : un dénominateur commun
produit est <2^(6R), et le numérateur de la somme est <2^(6R+3).

| Profil | Produit pour test de classe | Coefficient groupé, numérateur/dénominateur |
|---|---:|---:|
|u18 |272 bits |819/816 bits |
|u21 |314 bits |945/942 bits |
|u24 |356 bits |1071/1068 bits |

Des racines appartenant à des classes rationnelles distinctes sont Q-linéairement
indépendantes : choisir une base indépendante des classes dans Q*/Q*²,
puis appliquer les changements indépendants de signes de l'extension
multiquadratique. Les caractères distincts isolent chaque coefficient.
L'égalité est donc prouvée si et seulement si tous les coefficients sont nuls.
L'exemple des classes2,3,6 reste indépendant additivement, malgré leur relation
multiplicative. La représentation des coefficients reste contrôlée par ces
bornes ; un PGCD ou un produit n'est pas gratuit en temps.

## 4. Six racines non nulles : une borne de séparation finie

Voici une borne volontairement large qui garantit la terminaison dans tout
le domaine, sans supposer8192 bits suffisants. Soit
S=Σ_{i=1}^s σ_i√(N_i/D_i) non nul, σ_i∈{−1,+1}, s≤6.
Les radicandes nuls sont retirés. Poser P=∏D_i,
z_i=N_iD_i, b_i=σ_i P/D_i : α=PS=Σb_i√z_i est un entier algébrique.
Le corps Q(√z_i) a degré d≤2^s≤64. Pour s=6, toute conjuguée deα a
magnitude <6·2^T<2^(T+3), T=(A+11D)/2=37B+50.
Le produit des d conjuguées est un entier non nul, de magnitude≥1. Donc

    |S|>2^(-E), E=6D+63(T+3)=2367B+3387.

Le produit ci-dessus est le norm du corps ; toutes les racines d'entiers
sont entières algébriques, leur somme aussi, et son norm rationnel est entier.
Des dépendances entre les classes diminuent d, ce qui ne détériore pas la borne.

Encadrer chaque racine avec

    l_i=isqrt((N_iD_i)·2^(2p)),
    l_i/(D_i2^p) ≤√(N_i/D_i)<(l_i+1)/(D_i2^p).

L'intervalle de S a largeur≤6·2^(-p). Avec p=E+3, elle est strictement
inférieure à2^(-E) ; le signe non nul est donc nécessairement séparé.
Ne pas calculer ces Fractions avec des dénominateurs produit contenant
six copies de2^p : sommer les bornes entières après mise au dénominateur
commun **P2^p**, multiplicateurs P/D_i.

| Profil | E | p suffisant | Entier donné à isqrt | Somme signée, magnitude |
|---|---:|---:|---:|---:|
|u18 |45993 |45996 |92264 bits |46715 bits |
|u21 |53094 |53097 |106508 bits |53927 bits |
|u24 |60195 |60198 |120752 bits |61139 bits |

Le cas uniforme192/192 donne E73917,p73920,isqrt148224 bits et somme75075 bits.
Les additions finales utilisent des magnitudes <2^(p+T+3), signe séparé.
Cette réserve extrême ne prédit ni un cas difficile réalisable par un nuage,
ni son coût moyen. Les formes t≤q≤M peuvent permettre des bornes plus serrées.
Elle suffit à montrer que8192 bits ne sont pas une borne de complétude prouvée
par les seules tailles des niveaux. Un budget inférieur avec refus explicite
est exact sur les appels admis, mais doit être annoncé comme tel.

Une mise en œuvre adaptative peut séparer dès96/192/384 bits, exploiter une
ou deux classes exactement, et ne préparer le scratch extrême qu'en repli.
La borne ci-dessus ne justifie pas d'élargir globalement `Wide` : prévoir
une arithmétique de scratch propre aux comparaisons de points, la borner,
et qualifier séparément ses opérations/refus. L'isqrt général n'est pas
déjà qualifié par les prédicats du catalogue.

## 5. Portes nécessaires et format u24

Avant publication native : identité sémantique de triplets différents,
antisymétrie/transitivité, classes2/3/6, radicandes0, égalité d'une entrée
avec une multifusion FULL, propriétaire vivant après plateau, refus sans
préfixe publié, budgets avant allocation et déterminisme W1/W48. Le témoin
d'annulation doit exercer le filtre puis le repli exact, sans classer les
décimales proches par tolérance.

Le format MHGP11PH v1 exporte chaque entier en3 mots et refuse s'il reste
un mot haut non nul : aucune troncature implicite. Mais le budget natif du
numérateur u24 est204 bits ; ce format192 bits ne couvre pas tout son domaine.
Un port u24 complet doit modifier/versionner le format ou prouver une
réduction suffisante ; les campagnes u21 ne le qualifient pas.
