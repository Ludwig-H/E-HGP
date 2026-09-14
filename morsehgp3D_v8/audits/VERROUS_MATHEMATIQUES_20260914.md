# Verrous mathématiques relus pour la tour v8

14 septembre 2026, après **85015a8c**. Auditeur indépendant B.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Cette note fixe ce qui est démontré, ce qui est seulement inscrit au plan
et ce qui reste à décider, pour les objets que la v8 n'a pas encore
implémentés (q3/q4, catalogue, rattachements, histoire). Elle s'appuie sur
la lecture des Parties I–II du manuscrit (Défs 20–31, Th. 2–7, Prop. 5–9)
et sur des vérifications exactes en rationnels exécutées par mes agents
(scripts hors dépôt ; les énoncés ci-dessous sont reproductibles à la main).
Aucun défaut du code publié n'a été trouvé : la v8 n'implémente encore
que le census q2 d'un rectangle, et il est conforme.

## 1. Seuils `h_q = Kmax + 2 − q` : démontrés, y compris hors position générale

Soit une boule B de rayon strictement positif, p sites strictement
intérieurs, coquille S, et q_min la taille du plus petit support positif
(2, 3 ou 4 en dimension trois). Un simplexe σ a B pour plus petite boule
englobante si et seulement si σ est inclus dans la boule fermée et contient
un support positif. Pour être Gabriel au sens du théorème 4 (aucun site
étranger strictement intérieur), σ contient tous les intérieurs : σ ⊇ I ∪ F,
donc |σ| ≥ p + q_min. Un K-simplexe de la tour a K + 1 ≤ Kmax + 1 sites :
la boule est inerte pour toute la tour dès que p ≥ Kmax + 2 − q_min.

Sans position générale, ce raisonnement reste une condition suffisante
d'inertie ; il ne dit pas que la boule crée un événement à chaque ordre de
la fenêtre. Un agent l'a vérifié en rationnels exacts sur 250 nuages u16 à
coquilles dégénérées (aucun changement de H_0 hors de la fenêtre) et a
démontré le lemme de connexité des cônes locaux qui le justifie. La
formule est donc une autorité écrite hors régularité, ce que le manuscrit
ne fournit que sous la définition 26 ; consigner cette preuve ou la citer
dans `FONDEMENTS_ET_OBJET.md` §6 fermerait la question.

## 2. Élimination par lane, rétention par boule : inscrit au plan, à graver

Une lane de support q ne présente une boule qu'avec q ≥ q_min, donc
`h_q ≤ h_{q_min}` : une élimination de lane est toujours sûre pour la boule.
Une boule vivante (`p < h_{q_min}`) est produite par la lane q_min avec son
seuil exact : paire diamétrale (q2, toutes les paires sont couvertes),
triangle aigu de circumboule B trouvé par sa plus longue arête (q3),
tétraèdre à circumcentre intérieur trouvé par sa plus longue arête (q4).
Trois obligations en découlent pour le futur catalogue :

- I1, sûreté : une boule tuée par une lane est inerte (démontré ci-dessus).
- I2, complétude : toute boule pertinente est générée par au moins une lane,
  avec sa coquille complète.
- I3, fusion : les présentations d'une même boule issues de lanes différentes
  (paire diamétrale `ad` et triangle aigu `abc` sur la même sphère ; triangle
  `abc` et tétraèdre `abns`) doivent être fusionnées par une clé de boule
  commune aux trois lanes, centre homogène et rayon carré réduits. Le census
  q2 actuel émet des incidences par support sans cette fusion, ce que son
  contrat annonce ; aucune clé q3/q4 n'existe encore.

Le constructeur a inscrit la rétention par boule au plan après ma
première note. Il reste à graver deux fixtures de dédoublonnage
inter-lanes : la sphère de centre (10,10,10) et de rayon 5 portant
a=(15,10,10), d=(5,10,10), b=(7,14,10), c=(7,6,10) (q_min = 2 : à
p = Kmax − 1 la lane q3 élimine, la lane q2 conserve, la boule est
vivante) ; la même sphère avec un triangle positif et un tétraèdre
strictement positif partageant une arête (q_min = 3).

## 3. Fuseaux W3/W4 et plus longue arête : preuve refaite, règle à documenter

Avec m = (a+b)/2, u = (b−a)/2 et p = z − m : `H = |u|² − |p|²` et
`Ξ = 4|u|²|p_⊥|²`. Un site z est strictement intérieur à toute boule passant
par a et b de rayon au plus ρ·|ab| si et seulement si `H > 0` et
`m_q·H² > Ξ`, avec m_3 = 3 (ρ = 1/√3, borne de Jung planaire : le
circumrayon d'un triangle aigu de plus longue arête ab est au plus |ab|/√3,
égalité au triangle équilatéral) et m_4 = 2 (ρ = √(3/8), Jung en dimension
trois : tétraèdre de plus longue arête ab et circumcentre intérieur, égalité
au tétraèdre régulier). L'inégalité stricte est nécessaire : au point
d'égalité, z est sur la sphère du cas extrémal. Le code (`predicates.hpp`)
est conforme ; 5 006 triplets de boîtes et 42 cas d'égalité mis à l'échelle
jusqu'à 32 767 concordent avec une réimplémentation exacte indépendante.

La condition d'usage est que l'ancre ab soit une **plus longue arête** du
support, sinon Jung ne s'applique pas. La v7 le prouve (`S1_COURANT.md` §1 :
si les deux autres sommets étaient dans la boule diamétrale de ab, la MEB
du tétraèdre serait cette boule, incompatible avec un circumcentre
strictement intérieur) et départage les ex æquo par la plus petite
`EdgeKey`. Aucune doc v8 ne le restitue encore, et les fixtures v8 exigent
toutes une plus longue arête unique. À graver avant la lane q4 : le
tétraèdre régulier sur grille (0,0,0), (1,1,0), (1,0,1), (0,1,1), dont les
six arêtes ont le même carré 2.

## 4. Proposition 6, théorème 5 et K-MST : faux en général, remplaçant à définir

Le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` classe déjà la
proposition 6 et le théorème 5 en `false_in_general` (fixture E5
`gabriel-point-set-counterexample-5-points-v1`). Un agent l'a réexécuté :
E5 = A(0,0,7), B(0,9,6), C(1,4,0), D(0,0,1), E(4,1,2) est en position
générale, et au niveau de naissance de ABC le graphe Γ_2 donne une seule
composante {A,B,C,D,E} alors que le K-graphe de Gabriel de la définition 29
en donne deux, AC étant rattachée silencieusement par des cofaces
non-Gabriel. Sur 366 nuages u16 aléatoires en position générale, trois
violent la proposition 6 à K=2. Le K-MST de la définition 30 et l'algorithme 1
ne sont donc pas des autorités de correction pour la tour.

Deux corrections documentaires en découlent. `FONDEMENTS_ET_OBJET.md` §3
attribue la réfutation au « contre-exemple régulier à quatre points » ; or
cette fixture réfute seulement le graphe induit sur les seuls minima Gabriel
(construction absente du manuscrit) : la proposition 6 littérale y est vraie.
La vraie réfutation est E5, à citer avec l'entrée du registre. Ensuite, le
« graphe daté sur les naissances » qui remplace le K-MST n'a de définition
et de preuve (forêt à L − R liens, lots de même niveau) que dans
`../../morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/README.md` §3 ;
la v8 le décrit en prose seulement. Le porter comme source épinglée avant
la porte « Histoire par K » du plan.

## 5. Entrées u16 non régulières : décider le domaine exact

Les théorèmes 4 à 7 et le fait 12 supposent la définition 26 ; seuls le
théorème 2 et la proposition 5 s'en passent. Le manuscrit ne prouve rien
sur les plateaux. La v8 retient à juste titre le quotient local non
régulier de la v7 (coquille de taille arbitraire, lots atomiques, aucun
jitter), mais la preuve de ce quotient n'est acquise que sur des oracles
bornés (`../../morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md`) et le
registre garde la chaîne silencieuse de même niveau en « quotient prouvé ou
`unsupported_degeneracy` ». Ce qui manque est une décision écrite pour le
contrat de sortie : quel domaine est annoncé exact, quel statut fail-closed
est rendu ailleurs, et les fixtures AB/ABC, ABCZ et carré avec leur résultat
attendu. Sur une grille u16, tout angle droit axial met un troisième point
sur la sphère diamétrale : ce cas n'est pas rare.

Un glossaire éviterait une confusion voisine : « Gabriel » au sens de la
définition 28 (boule ouverte vide) n'est pas « minimum Gabriel strict »
(boule fermée vide, sommet isolé de Γ_K à sa naissance). Les deux énoncés
sont utilisés dans les docs v8 sans être nommés.

## 6. Réponse à la question du constructeur : rejeter des produits ancêtres avant séparation

La question du 14 septembre demande si des produits U×V d'un front WSPD
peuvent être rejetés par K témoins avant d'être séparés. Trois réponses.

**Sûreté : oui, sans aucune hypothèse de séparation.** Les prédicats de
boîtes (`box_witness`, `classify_witness_block`) sont exacts sur des boîtes
continues. Un site z hors de U ∪ V, universel sur les deux boîtes, est
strictement intérieur à la boule diamétrale de toute paire de U×V ; K
sites distincts de ce type avec K ≥ h_q tuent le produit entier pour la
lane q. Tuer un ancêtre tue exactement l'union des paires de ses
descendants : la partition WSPD est inchangée, seule l'énumération est
évitée. La couverture résiduelle se prouve par un ledger **par lane** :
masse des produits tués plus masse des rectangles terminaux émis égale le
nombre de paires de positions distinctes.

**Utilité : bornée par une condition géométrique testable avant toute
recherche.** Un témoin universel existe pour U×V si et seulement si la
lentille, intersection des boules diamétrales ouvertes de toutes les
paires, est non vide. Elle se teste exactement en maximisant sur z la
quantité `h_minimum(U, V, {z})`, concave et séparable par axe. Pour deux
cubes de côté w alignés et distants de T (bord à bord T − w), le milieu des
centres est universel dès que `T − w > √2·w` ; en rayons de boîte, la
condition `D > 2(1+√2)R` est suffisante pour deux boules, et les boîtes
font mieux. Les paires frères de l'arbre radix, au départ des vagues, ont
une lentille vide et ne peuvent être tuées : un test de lentille par
produit évite d'y lancer la recherche d'index qui a coûté 5·10⁹ visites au
front v7. Les kills n'ont lieu que dans la bande entre la lentille vide
et la séparation à `s` ; c'est là que le front fusionné v7 réduisait
3 435 133 rectangles purs à 754 686 terminaux sur l'uniforme 8k.

**Coût : ne pas repartir de la racine.** Un enfant U'×V' a une lentille
plus grande que son parent, donc les témoins certifiés du parent restent
universels pour lui. Transmettre leurs identifiants (au plus h_q − 1 par
lane survivante) avec exclusion des nouveaux crédits est sûr par
restriction ; seul le double comptage est à interdire, comme le note déjà
`WSPD_Q2_Q3_Q4.md` §9 A2. Les rejets négatifs, eux, ne se transmettent pas.
Cette transmission et le test de lentille sont les deux objets à mesurer
sur le pilote de front, avec les compteurs de rectangles tués avant
séparation et de visites d'index.

## 7. Lemme des tubes : marges réelles, constante à ne pas relâcher

Le lemme du cône (`P0_TUBES_ET_RANGS.md` §1) est correct : avec
`x = e·(z−a)`, `|u_⊥| ≤ ρx`, `y ≥ D − 2R ≥ 8R` et `|v_⊥| ≤ 2R ≤ y/4`, l'identité
de Gram donne `Ξ ≤ (ρ + 1/4)²x²y²` et les marges annoncées. Une attaque
exacte à la frontière `D = 10R` (1 105 configurations, 3,45 millions de
tests aux coins) ne trouve aucun faux témoin ; les pires marges observées
valent 0,16, 0,48 et 0,527 fois `x²y²` pour q2, q3, q4. Sous la frontière,
des faux crédits exacts apparaissent dès `D/R ≈ 7,6` (q3), 6,3 (q2) et 6,1
(q4) : la constante 100 n'est ni arbitraire ni très lâche, et ne peut pas
descendre à 25. Toute version plus fine exigerait une nouvelle preuve et
une fixture d'égalité.
