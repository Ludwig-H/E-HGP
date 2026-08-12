# Réponses auditées — source par supports et front inverse

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Question auditée :
[`QUESTIONS_CLAUDE_SOURCE_FRONT_INVERSE_20260812.md`](QUESTIONS_CLAUDE_SOURCE_FRONT_INVERSE_20260812.md).
Le `HEAD` observé est
`8c00ab07695ef353e673ab73a778a6f260c87509`. Les conclusions mathématiques
ci-dessous ne dépendent pas du statut Git des prototypes.

## Verdict court

| question | réponse | conséquence |
| --- | --- | --- |
| Q1 | non à l'égalité littérale; oui comme catalogue génératif | un support minimal n'est ni une coface ni une boule de rang fermé au plus onze |
| Q2 | l'arrangement décrit des incidences de shell | l'auto-centrage est un filtre à la projection, et tous les supports d'une même `BallKey` doivent rester attachés au record |
| Q3 | faux | le graphe des seules sorties auto-centrées n'est pas connexe et les deux transitions proposées ne suivent pas le vrai arrangement |
| Q4 | aucune borne sortie-sensible | une requête vide peut visiter `Theta(n)` nœuds et une coquille admise par le test proposé peut contenir `Theta(n)` labels |
| Q5 | le bloc saturé est connexe, le raccourci est faux | l'unité de contraction est un générateur saturé muni d'un join complet, jamais une facette arbitraire |
| Q6 | oui mathématiquement | sortir q2 profond de `k=1`, mais employer Yao-1 plutôt que matérialiser Gabriel |

Ces verdicts portent sur les énoncés de la question, pas sur tout successeur.
Un prototype qui suit le vrai premier croisement, conserve les transits et
transporte les lots corrige les transitions réfutées de Q3. Il ne devient pas
pour autant une source complète : q2/q3, auto-centrage, tous les supports,
plateaux et coût restent à recevoir. Le vrai arrangement shallow est une voie
exacte conditionnelle, mais son parcours peut coûter `Theta(n V)` pour `V`
sommets; il n'est donc pas sortie-sensible par construction.

## Notation non ambiguë

Pour une boule `B=(c,a)`, on note :

- `I_B` l'ensemble global des points strictement intérieurs;
- `U_B` le shell global fermé;
- `S` un support minimal propre positif, donc `S` inclus dans `U_B`;
- `E_extra=U_B\S` l'extra-shell relative à ce support;
- `S_B=I_B union U_B=X cap B` le saturé fermé.

Le rang fermé contractuel est

$$s(B)=|I_B|+|U_B|.$$

Il ne doit jamais être confondu avec `|I_B|+|S|`, ni avec la cardinalité d'une
coface particulière.

## Q1 — ce que la Source S couvre réellement

Soit `S` un support minimal propre positif de taille `q` entre deux et quatre,
et `p=|I_B|`. La condition `p+q<=11` définit exactement les **témoins minimaux
pertinents par support**. Elle ne définit ni les cofaces une à une, ni les
boules de rang fermé au plus onze.

Pour une boule fixée, ses cofaces directes de cardinalité au plus onze sont les
ensembles

$$Q=I_B\cup A,\qquad A\subseteq U_B,\qquad c\in\mathrm{conv}(A),\qquad |Q|\leq11.$$

Tout `A` admissible contient, par minimalité puis Carathéodory en dimension
trois, au moins un support propre positif `S` de taille au plus quatre. Ainsi
`|I_B|+|S|<=|Q|<=11` : aucune coface de cardinalité au plus onze n'échappe au
catalogue de supports exhaustif. Réciproquement, chaque support de la Source S
engendre au moins la coface minimale `I_B union S`.

Cette correspondance est seulement générative : plusieurs supports peuvent
désigner la même boule et un support peut engendrer plusieurs `A`. La Source S
munie de `I_B`, `U_B` et de la `BallKey` permet de reconstruire la famille; elle
ne l'énumère pas et ne certifie pas à elle seule son quotient horizontal.

### Contre-fixture `source_support_rang_ferme_12`

Prendre le centre `O=(100,100,100)`, le rayon `65` et, dans le plan `z=100`, les
douze points

```text
(165,100) (35,100) (100,165) (100,35)
(163,116) (37,84) (163,84) (37,116)
(160,125) (40,75) (160,75) (40,125)
```

Chaque distance carrée à `O` vaut `4225`. La paire antipodale
`(165,100,100),(35,100,100)` est un support q2 avec `p=0`, donc satisfait
`p+q=2`, tandis que le rang fermé global vaut `s(B)=12>11`. La Source S
contient donc un objet qui n'est pas une boule de rang fermé au plus onze.

### Contre-fixture `supports_distincts_des_cofaces`

Pour le carré `(9,9,0),(11,11,0),(9,11,0),(11,9,0)`, la même boule possède deux
supports minimaux, les diagonales, mais sept choix admissibles de `A` : les deux
diagonales, les quatre triples et le carré. Dédupliquer les supports en un seul
objet effacerait l'hypergraphe; compter les supports ne compterait pas les
cofaces.

Le record sûr sépare donc au minimum :

```text
BallRecord {
  BallKey, beta_exacte,
  I_B_ids_complets, U_B_ids_complets,
  supports_minimaux_ids_complets,
  relevant_by_min_support,
  accepted_closed_rank,
  plateau_status, owner, provenance
}
```

Un shell pertinent avec `s(B)>11` n'est pas automatiquement inexact : il peut
être traité par un quotient saturé reçu. Sans ce quotient, il doit être refusé
fermement; il ne doit jamais être tronqué ni assimilé à un événement régulier.

## Q2 — arrangement, projection et supports multiples

Pour une ancre `x` et des labels `p_i`, l'intersection `L_S` des plans
bissecteurs est le lieu des centres de sphères passant par `x` et les `p_i`.
Elle décrit une **incidence de shell**, pas une famille de miniboules ayant
automatiquement `S={x,p_i}` pour support.

Lorsque `S` est affinement indépendant, son circumcentre est la projection
orthogonale de `x` sur `L_S`. Il est un support Morse exactement si cette
projection appartient à `relint conv(S)`. Sur une face bissectrice q2, seul le
milieu de la paire est son centre de miniboule; sur une arête q3, seul le
circumcentre projeté du triangle est le candidat q3. Les autres points de la
strate sont seulement des centres de sphères incidentes.

Si le centre tombe sur le bord relatif de `conv(S)`, `S` n'est pas un support
minimal : il faut conserver la coface et le shell, puis rattacher la ou les
sous-faces minimales positives. Par exemple
`(0,0,0),(2,0,0),(1,1,0)` forment un triangle rectangle dont le support réel est
la paire diamétrale; le triple reste pourtant une coface admissible de la même
boule.

Si plus de trois bissecteurs se coupent, la `BallKey` réunit la boule, mais ne
supprime pas ses supports. Un `BallRecord` conserve le shell global et la liste
ou l'hypergraphe de **tous** les supports minimaux positifs. Deux supports
distincts, voire d'arités distinctes, peuvent partager la même boule. La bonne
opération est donc `RLE par BallKey + agrégation des supports`, pas
`déduplication des supports par BallKey`.

## Q3 — le graphe proposé dans la question n'est pas complet

L'énoncé de connexité demandé est faux pour quatre raisons indépendantes.

### 1. Les transitions ne relient pas les arités

Les deux transitions décrites remplacent un sommet d'un tétraèdre et
préservent l'arité quatre. Elles ne donnent aucune arête aux supports q2 et q3.
Avec le triangle aigu u16
`(10,10,10),(12,10,10),(11,12,10)`, les trois supports q2 de niveau zéro et le
support q3 de niveau zéro sont quatre sommets isolés du graphe déclaré. Aucun
choix de germe q4 ne les atteint.

Le défaut persiste même si l'on retire q2/q3 et toute dégénérescence. Prendre
les six points u16

```text
p0=(3,5,5)    p1=(27,14,22)  p2=(5,1,10)
p3=(14,10,2)  p4=(14,22,8)   p5=(11,21,23)
```

L'énumération rationnelle des quinze quadruplets trouve exactement quatre
supports q4 propres positifs : `0235` de niveau 1 avec intérieur `{4}`, `0245`
de niveau 0, `1235` de niveau 0 et `1245` de niveau 1 avec intérieur `{3}`.
Les deux seules sorties de niveau zéro, `0245` et `1235`, ne partagent que
l'arête `25`, jamais une facette. Elles ne sont donc reliées par aucune des
transitions q4 proposées; comme leurs intérieurs sont vides, aucune transition
de niveau ne les relie non plus. Les quinze déterminants affines sont non nuls
et aucun quintuple n'est cosphérique : ce contre-exemple n'utilise ni plateau ni
coplanarité.

### 2. `argmin beta` n'est pas le voisin d'arrangement

Pour une facette triangulaire `F`, les centres des sphères incidentes sont sur
une droite `c(t)=o_F+tN`. Depuis un sommet courant de paramètre `t_0`, le voisin
exact est le plus petit `t_y>t_0` dans une direction et le plus grand
`t_y<t_0` dans l'autre, avec tous les ex æquo groupés. Minimiser le rayon, qui
est quadratique en `t`, peut sauter des croisements et ne préserve pas le
niveau. Le `t_0` numérique n'est toutefois pas indispensable si l'état porte la
chambre de signes complète : l'intérieur courant, le shell orienté et le sens
peuvent définir exactement les candidats situés après `t_0`, puis un pivot vide
choisir le premier. Cette construction, absente de l'énoncé Q3, exige son
invariant et son différentiel propres; elle n'est pas l'`argmin beta` proposé.

Les trois API exactes doivent rester distinctes. Pour `F=(a,b,c)`, poser
`u=b-a`, `v=c-a`, `N=u cross v`, `G=N dot N` et
`P=||u||^2(v cross N)+||v||^2(N cross u)`. Pour un candidat `y`, poser
`d=y-a`, `h=N dot d` et `T=G||d||^2-P dot d`. On obtient

$$c_y=a+\frac{P}{2G}+\frac{T}{2Gh}N,\qquad \beta_y=\beta_F+\frac{T^2}{4Gh^2},\qquad \lambda_y=\frac{T}{2h^2}.$$

Ainsi, un minimum de `beta` parmi les tétraèdres propres positifs doit d'abord
imposer le bon signe de `h`, `T>0` et la positivité des trois autres poids
barycentriques, puis comparer exactement `T_y|h_z|` à `T_z|h_y|`, avec
`PointId` comme tie-break. Un successeur d'arrangement ordonne au contraire le
paramètre signé `tau=T/(2Gh)` relativement au `tau` courant. Une recherche
par premier témoin intérieur ne réalise aucune de ces deux opérations.

Sur le profil u16, les bornes conservatrices sont `|N_i|<2^33`, `G<2^68`,
`|P_i|<2^85`, `|T|<2^104` et `|h|<2^51`. Le produit du comparateur peut donc
atteindre 155 bits : `__int128` ne suffit pas. Employer le type multiprécision
192/256 bits déjà disponible, et graver séparément les trois sémantiques.

Le contre-exemple minimal du code courant prend
`F={(0,0,0),(4,0,0),(0,4,0)}` et les candidats
`y_3=(0,0,1)`, `y_4=(1,1,1)`, `y_5=(2,2,1)`. Les rayons carrés valent
`33/4`, `57/4`, `81/4`; le pivot termine pourtant sur `y_5`. Aucun des trois
tétraèdres n'est auto-centré. La routine ne minimise donc pas `beta` et son
résultat n'est pas un support q4 de la Source S.

Même la sortie du pivot peut ne pas être q4. Pour le tétraèdre régulier centré
en `(100,100,100)` de sommets
`(103,103,103),(103,97,97),(97,103,97),(97,97,103)`, prendre pour `F` les trois
derniers sommets et `y=(99,99,98)` sur l'autre côté. Le centre de `mb(F)` est
`(99,99,99)`, son rayon carré vaut `24` et `y` est strictement intérieur à
cette boule. Ainsi `mb(F union {y})=mb(F)` : le support réel reste q3.

### 3. La baisse du rayon n'ordonne pas le niveau

Poser
`u=(120,120,120)`, `a=(120,80,80)`, `b=(80,120,80)`, `c=(80,80,120)`,
`z=(115,115,115)` et `w_j=(79,79,79),(78,78,78),(77,77,77)`.
Le support `S={u,a,b,c}` est centré en `(100,100,100)`, a
`beta(S)=1200` et exactement un intérieur, `z`.

Après remplacement de `u` par `z`, le tétraèdre `T={z,a,b,c}` reste propre
positif : son centre vaut `(2495/26,2495/26,2495/26)`, son rayon carré
`735075/676<1200`, et ses barycentriques sont `41/338` pour `z` et `99/338`
pour chacun des trois autres sommets. Pourtant les trois `w_j` sont strictement
intérieurs, avec les distances carrées
`583443/676`, `654267/676` et `729147/676`, tandis que `u` est extérieur à
`1171875/676`. Le niveau passe donc de `1` à `3`, pas de `p` à `p+1`.

### 4. Le graphe des seules sorties peut perdre les transits

La région `ell_x<=k` est bien étoilée par rapport à `x` : en ramenant le centre
vers `x`, aucun point nouveau ne devient strictement plus proche que `x`.
Cette connexité continue ne prouve pas la connexité du sous-graphe formé par
les seules projections auto-centrées. Des sommets non auto-centrés et des
strates sans sortie peuvent être nécessaires comme transits.

La voie exacte connue consiste à parcourir le **vrai arrangement shallow** :

- voisin consécutif dans les deux sens du pinceau, ordonné par `t` orienté;
- égalités groupées et transport du niveau par lots, jamais supposé `+/-1`;
- sommets non auto-centrés conservés comme transits;
- auto-centrage appliqué seulement au moment de l'émission;
- branches explicites pour q2, q3, `affdim<3` et coquilles multiples;
- owner global et census terminal avant `BallRecord`.

Le théorème conditionnel de
[`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md)
porte sur ce vrai 1-squelette, pas sur un graphe de quatre identifiants ni sur
son coût. Un germe par ancre ne répare pas le graphe de sorties et peut
reconstruire `n` arrangements.

## Q4 — une requête LBVH de petite sortie peut rester linéaire

La prémisse « sortie bornée par onze » est déjà fausse : `p+q<=11` borne
l'intérieur et le support choisi, mais pas `|E_extra|`. Une boule pertinente
peut avoir `I_B` vide et `|U_B|=Theta(n)`; publier le census fermé coûte alors
au moins la taille du shell.

Même si l'on ne demande que les intérieurs, une sortie vide n'implique pas
`O(profondeur)`. Dans un plan, prendre des points distincts
`p_i=(x_i,y_i)` sur un arc de cercle du premier quadrant, avec `x_i<x_j` et
`y_i>y_j` pour `i<j`. Pour tout AABB contenant deux points `p_i,p_j`, il
contient le coin `(x_i,y_j)`, et

$$x_i^2+y_j^2<x_j^2+y_j^2=R^2.$$

Le minorant de distance de cette boîte est donc strictement inférieur au
rayon. Aucun nœud interne contenant au moins deux points ne peut être écarté,
alors que toutes les feuilles sont sur le shell et que la sortie intérieure est
vide. Le parcours visite `Theta(n)` nœuds. Ajouter un support antipodal de la
même sphère donne simultanément `p=0`, un support auto-centré et une
extra-shell de taille `Theta(n)`.

Le pivot ne possède pas davantage de borne sortie-sensible. Avec une normale
`N` de `F`, son circumcentre planaire `o_F` et son rayon `R_F`, le point `y`
hors du plan coupe le pinceau au paramètre

$$t_F(y)=\frac{\left\Vert y-o_F\right\Vert^2-R_F^2}{2N\mathbin{\cdot}(y-o_F)}.$$

Le voisin exige un ordre exact sur le quotient **orienté**. Un AABB fournit en
général un intervalle qui traverse zéro ou chevauche l'incumbent; une requête
peut donc descendre jusqu'à `Theta(n)` feuilles. Minimiser seulement
`R_F^2+t^2\left\Vert N\right\Vert^2` perd le sens. Les produits croisés du
quotient exigent en outre une borne de bits dédiée; le seul commentaire
`InSphere tient en i128` ne la fournit pas.

Une implémentation GPU diagnostique doit séparer au minimum :

- `ball_queries`, `ball_node_visits`, `ball_ambiguous_nodes`,
  `ball_leaf_tests`, `interior_ids`, `shell_ids`;
- `pivot_queries`, `pivot_node_visits`, `pivot_ambiguous_nodes`,
  `pivot_leaf_tests`, `exact_ratio_tests`, `best_updates`, `tie_mass`;
- transitions tentées par type, états de transit, supports, `BallKey`, doublons
  et files visitées;
- `sum_shell_ids`, octets de sortie, arènes, queue et high-water;
- histogrammes par requête et totaux `W_ball`, `W_pivot`.

Le ledger d'une requête terminale partitionne tous les labels hors support en
`extérieur + intérieur + shell`. Un arrêt au onzième intérieur autorise un
rejet hors fenêtre; il n'autorise jamais un `I_B/U_B` prétendument complet. Un
cap physique rend `resource_exhausted`, jamais `empty`.

À `12 500/25 000/50 000`, chaque compteur dominant, le nombre d'états et les
octets doivent avoir deux pentes acceptables; une moyenne de visites par sortie
ne remplace pas les sommes. Tant que ce ledger manque, le pire cas du front est
`Theta(Mn)` pour `M` états et aucun port G4 n'est justifié.

## Q5 — contraction exacte d'un plateau

Le fait local correct est le suivant. À la coupe fermée `a=beta(B)`, tout
sous-ensemble de `S_B=X cap B` est couvert par `B`. Pour un ordre `k` fixé, le
bloc contient donc implicitement le graphe de Johnson `J(|S_B|,k)`, qui est
connexe. Toutes les `k`-facettes du saturé appartiennent bien à une même
composante **fermée** locale.

Il n'en résulte pas qu'une facette par coface suffit.

### Contre-fixture `plateau_carre_multifusion`

Prendre `A=(0,0,0)`, `B=(2,0,0)`, `C=(2,2,0)`, `D=(0,2,0)`, centre
`(1,1,0)`, `beta=2`, `I_B` vide et `U_B={A,B,C,D}`. À l'ordre `k=2`, les
quatre triples portent la même boule. Au lot fermé `beta=2`, leur union est
`J(4,2)` et connecte les six paires.

Choisir une facette canonique par triple puis les relier à une facette commune
ne touche qu'au plus cinq des six paires. De plus, aucune paire n'appartient aux
quatre triples. Au moins une facette reste isolée : une multifusion ou une
naissance est perdue.

La compression exacte possible est un token
`SaturatedGenerator(BallKey,beta,S_B_ids_complets)`. Il représente le bloc de
Johnson sans énumérer tous les `A`, à condition de disposer aussi de :

- racines strictes pré-lot gelées et atomicité de `(k,beta)`;
- memberships complets et lookup de containment;
- join exhaustif entre générateurs par `|S_B intersection S_C|>=k`, ou preuve
  équivalente de toutes les interfaces;
- couverture et owner rejouables.

La fixture E5 du dépôt explique la dette future : le saturé `ACDE` doit garder
l'interface `AC` afin que la boule `ABC` puisse la rejoindre plus tard. Une
facette canonique différente et l'absence de lookup retardent cette fusion.
Les seuls cardinaux `(|I_B|,|U_B|)` ne suffisent jamais; les identités complètes
et les joins sont nécessaires.

Il n'existe donc pas de dilemme « exact ou dégénérescence non supportée » : un
quotient saturé peut rester exact, mais son coût et son index d'interfaces
doivent franchir les portes industrielles. En l'absence de cette autorité, le
refus fermé reste la seule sortie exacte.

## Q6 — `k=1` n'a pas besoin du Gabriel d'ordre dix

Oui. Soit `uv` une arête d'un EMST et `w` un tiers distinct dans ou sur la boule
diamétrale fermée de `uv`. Le théorème de Thalès donne

$$2(w-u)\mathbin{\cdot}(w-v)=\left\Vert w-u\right\Vert^2+\left\Vert w-v\right\Vert^2-\left\Vert u-v\right\Vert^2\leq0.$$

Les deux arêtes `uw` et `vw` sont alors strictement plus courtes que `uv`.
Après retrait de `uv` de l'arbre, l'une traverse sa coupe, contradiction. Tout
EMST est donc contenu dans le Gabriel fermé de rang deux, a fortiori dans le
Gabriel ouvert défini par `I_B` vide.

Une MSF du seul graphe `I_B=empty` reconstruit donc un EMST du graphe complet.
Les longueurs égales sont traitées dans un lot atomique. Si l'identité interne
de l'arbre est contractuelle, Kruskal emploie partout la même clé
`(distance_squared,min_PointId,max_PointId)`; sinon les invariants publics sont
les partitions à chaque seuil, le poids et les multifusions.

Cette réduction est exacte mais n'est pas la meilleure architecture : le
Gabriel 3D peut avoir un degré non borné et une taille quadratique. La lane
industrielle `k=1` doit employer le transcript Yao-1 déjà prouvé, avec au plus
`48n` candidats dirigés, puis déduplication et Kruskal/Borůvka sparse. q2
profond sort ainsi du chemin critique de `k=1` sans disparaître des ordres
supérieurs.

L'expression « Gabriel d'ordre dix » doit rester définie : `|I_B|<=9` décrit un
graphe diamétral à niveau intérieur borné, mais son rang fermé peut dépasser
onze si le shell contient des égalités.

## Statut du prototype concurrent

Le prototype a évolué plusieurs fois pendant cette réponse. Ses empreintes, ses
tests et son verdict logiciel sont maintenus uniquement dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md); aucun snapshot périmé n'est
recopié ici. Le successeur observé vise désormais le premier croisement du
pinceau dans les deux sens, transporte l'intérieur par lots et quotiente les
triplets coplanaires. Ces choix répondent constructivement à une partie de Q3.

La portée demeure `q4_arrangement_witness_only` tant que q2/q3, dimension
affine basse, auto-centrage, `BallKey`, tous les supports, plateau, owner et
payload ne sont pas raccordés. Un accord final sur les sommets ne remplace pas
un oracle transitionnel qui compare chaque `(sommet,flat,sens)` et son lot. Le
germe et le coût restent des portes séparées.

## Ordre de travail recommandé vers `50 k / 1 s`

1. Graver les fixtures de cet audit et conserver la portée explicite
   `q4_arrangement_witness_only` du front; juger chaque transition consécutive,
   son flat fermé, ses ex æquo et son transport de census.
2. Sortir immédiatement q2 profond de `k=1` grâce au transcript Yao-1 exact;
   c'est une réduction prouvée de travail, contrairement au catalogue Gabriel.
3. Si le front est poursuivi, comparer les identités complètes à un oracle
   rationnel et publier `incomplete` pour les branches q2/q3/affdim manquantes.
   La récolte par propriétaire est complète en dimension affine trois, mais le
   niveau projeté doit être recensé et n'est pas hérité du sommet.
4. Représenter les plateaux par générateurs saturés et construire d'abord le
   join de containment/overlap; sans lui, ne publier aucun fold exact.
5. Prototyper en parallèle les listes imbriquées de cellules de centres de
   [`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md),
   qui évitent le census global par tuple sans matérialiser l'arrangement.
6. Mesurer `M`, visites pinceau, `sum|U_B|`, listes cellulaires, octets et
   high-water aux trois tailles. Deux pentes rouges ferment une ordonnance
   avant CUDA.
7. Ne porter sur G4 que les primitives d'une source mathématiquement complète;
   le payload officiel reste dix forêts, verticales, lots, certificat minimal
   et retour hôte dans le même `warm_e2e`.

Aucun tableau global de cellules, cofaces ou incidences de Delaunay d'ordre
supérieur n'est autorisé dans ce chemin. Un parcours du vrai arrangement qui
matérialiserait toutes ses cellules changerait seulement le nom de la mosaïque
interdite.

GCP non utilisé.
