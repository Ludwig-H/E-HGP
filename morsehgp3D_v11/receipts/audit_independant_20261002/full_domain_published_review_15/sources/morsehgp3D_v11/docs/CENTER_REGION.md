# Rejets exacts dans une région de centres

Tranche en préparation ; **qualification native G4 à venir**. Aucun temps de
v10 ni aucune qualification du catalogue n'est hérité par ces primitives.
Les fichiers et empreintes R2, `f8e78ad94` et `777406b82` sont épinglés dans
[`center_region.provenance.json`](../src/num/center_region.provenance.json).
Le corps historique du test de droite est identique dans les deux commits
publics ; leurs filtres de nœuds et leurs arbres diffèrent.

## Domaine et API

`num::CenterRegion::make(lo, hi)` copie et valide les six entiers :
`0 <= lo[j] < hi[j] <= 2^B`, pour B18/B21/B24. La région est la **fermeture**
`[lo,hi]` de la boîte propriétaire du catalogue. Ses contacts sont conservés,
y compris sur `hi`. L'appartenance du centre à la boîte demi-ouverte reste
un autre test ; ces rejets n'attribuent aucune propriété et n'émettent rien.
Le type `num::Box`, dont les extrémités sont des Point, ne permet pas `hi=2^B`
et n'est pas détourné. Les régions de largeur nulle sont refusées.

`bisector_meets(a,b,region)` teste exactement si le lieu équidistant de a et b
rencontre la fermeture. Des points égaux donnent le lieu entier, donc `true`.
`center_line_meets(a,b,c,region)` distingue `degenerate` (points alignés ou
doublons), `disjoint` et `intersects`. Aucun prédicat n'exige l'aiguïté du
triangle. Les arguments Point sont déjà validés, aucun coefficient brut ni
Sphere n'est construit. Les fonctions ne font aucune allocation.

## Preuve et budgets T0

Poser M=2^B, u=a−b, v=a−c, h=hi−lo,
A=Σ(a_j²−b_j²), C=Σ(a_j²−c_j²),
p₀=A−Σ(lo_j+hi_j)u_j et p₁=C−Σ(lo_j+hi_j)v_j.
Les deux équations d'équidistance sont A−2u·x=0 et C−2v·x=0.

Pour une paire, la forme affine atteint ses extrema aux coins déterminés
par les signes de u. La fermeture rencontre le plan si et seulement si
minimum≤0≤maximum. L'égalité conserve donc tout contact.

Pour trois points non alignés, l'image de la boîte fermée par les deux
formes affines est le zonogone
`(p₀,p₁) + Σ [-h_j,h_j] (u_j,v_j)` (les signes des générateurs n'importent pas).
Son rang vaut deux, car u et v sont indépendants et chaque h_j est positif.
Ses normales de côtés sont `(v_k,−u_k)` pour les générateurs non nuls.
Il contient l'origine si et seulement si, pour chaque telle normale,

`|v_k p₀ − u_k p₁| <= Σ(j != k) h_j |v_k u_j − u_k v_j|`.

Cela est exactement l'intersection de la droite équidistante et de la
fermeture. Le facteur deux commun aux deux membres de la formule historique
est supprimé algébriquement. Ce port n'utilise ni le repère T6 ni ses budgets.

Chaque coordonnée de Point est dans [0,M−1] ; chaque borne est dans [0,M].
Les bornes suivantes valent pour **chaque terme et somme partielle du code** :

| Expression | Borne stricte | Budget |
| --- | --- | --- |
| u_j, v_j | M | B |
| A, C | 3M² | 2B+2 |
| extrema de paire, p₀, p₁ | 9M² | 2B+4 |
| v_k u_j−u_k v_j | 2M² | 2B+1 |
| v_k p₀−u_k p₁ | 18M³ | 3B+5 |
| somme des deux termes non nuls de droite | 4M³ | 3B+2 |

`Affine=Int<2B+4>` est i64 aux trois profils. `Test=Int<3B+5>` est i64
à B18, i128 à B21/B24 (budgets59/68/77). Le code élargit **avant** chaque
produit du test final. Les négations et valeurs absolues restent dans ces
mêmes bornes ; aucun minimum signé n'est atteint. Les gardes de compilation
contrôlent les types et budgets. Il n'y a ni degré sept ni calcul de rayon.

## Raccord au catalogue et mémoire

La région est construite une fois par feuille. Une paire dont la bissectrice
manque la fermeture ne peut appartenir à aucun support de cette feuille.
Une face d'un tétraèdre non dégénéré est non alignée ; son centre doit
appartenir à la droite équidistante de chacune de ses **quatre faces**.
Tester ces quatre conditions est donc un rejet nécessaire sûr. Un triplet
obtus dont la droite rencontre la fermeture reste disponible pour q4.
Le raccord DFS consulte les deux sens du masque de dominance existant :
une dominance stricte équivaut à une bissectrice disjointe. Il ne recalcule
pas les coefficients de paire et ne réserve aucun tableau supplémentaire.
Chaque extension teste seulement les paires et faces nouvelles : la face
du préfixe q3 a déjà été conservée avant les trois autres faces q4. Les
seuils G3 restent distincts. Quatre compteurs par passe publient tests et
rejets de paires/droites ; les deux passes doivent être identiques.

Les primitives sont de coût et stockage constants. Ce fait ne borne pas le
nombre de feuilles ou candidats. Des masques de paires coûteraient
`8 m ceil(m/64)` octets par workspace ; la table historique de triplets
coûterait `8 m(m−1)/2 ceil(m/64)` octets. Toute table éventuelle doit être
un Buffer admis avant allocation, multiplié par le nombre de workspaces
simultanés et compté pendant les deux passes. Le raccord initial peut tester
les faces à la demande sans table cubique ; une limite de stockage ne doit
jamais supprimer silencieusement un candidat.

## Portes prévues

Trois groupes natifs couvrent domaine/possession, bissectrices et droites,
aux trois profils. Les cas comprennent hi=M, bornes invalides, contacts de
face/arête/sommet, duplications, alignements, permutations, produits cubiques
au-delà d'i64, trois bissectrices rencontrant la boîte alors que la droite
commune la manque, et les quatre faces d'un tétraèdre positif à face obtuse.

La sonde reçoit q puis trois Point et les deux bornes (15 coordonnées).
L'oracle indépendant résout les équations en Fraction et coupe une droite
paramétrique par les six faces ; il ne reprend pas le test du zonogone.
Normal/−O, refus, inventaire et corruptions du juge sont exigés. Six mutants
visent contacts, test supprimé, axe omis, signe et domaine supérieur.
Les preuves d'exécution seront celles du prochain lot G4, pas de cette note.
