# Proposition consolidée MorseHGP3D v3

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document fixe la route candidate actuelle. Il ne promeut aucune phase et ne
remplace ni les spécifications sous `docs/`, ni le verdict mutable
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Une brique est
« reçue » seulement lorsque son autorité indépendante, ses mutants, sa
complétude, ses statuts d'échec et ses ressources sont exercés.

## 1. Objectif et interdits d'architecture

La cible secondaire est `p95 warm_e2e<1 s` pour `n=50000`, `K_max=10`, sur un
seul G4; la cible principale reste `100 ms`. Le chronomètre officiel couvre
entrée, construction, source, census, fold, dix forêts, verticales, payload,
synchronisation et sortie exigée par `BenchmarkOutputContract-v1`.

Le chemin produit ne construit jamais :

- mosaïque de Delaunay d'ordre supérieur ;
- arrangement global de plans ou de droites ;
- matrice globale de cofaces ;
- catalogue résident de tous les supports ;
- tableau indexé par toutes les paires, triplets ou quadruplets.

Un oracle exhaustif à petit `n` est obligatoire pour juger le chemin produit,
mais il ne devient jamais son ordonnance.

## 2. Identités et statuts avant toute optimisation

### 2.1 Identités

- `PointId` est une identité stable, distincte de l'index dense, de Morton et
  de `GenerationRank`.
- `SupportKey` est le tuple trié des vrais `PointId` du support.
- L'owner d'un support maximise la longueur carrée de ses arêtes puis choisit,
  en cas d'égalité, la plus petite `EdgeKey=(min PointId,max PointId)`.
- La `BallKey` géométrique est formée **avant** le census. Elle contient
  l'identité du nuage, le profil, le schéma et les cinq coefficients primitifs
  de `A||z||^2+B dot z+C`, normalisés par pgcd et signe `A>0`.
- `I_B` et `U_B` appartiennent au `BallEvent` ou au `SphereRun`; ils ne créent
  pas une seconde clé post-census.
- `PrimitiveSphereKey` peut nommer un codec interne des cinq coefficients. Il
  ne remplace ni l'epoch du nuage, ni le schéma de la `BallKey` persistante.

Le code historique où une clé contient `shell_min` n'est pas l'ABI produit :
une donnée issue du census rendrait le RLE pré-census circulaire.

### 2.2 Statuts transactionnels

Chaque exécution termine dans un état typé :

```text
complete_regular
unsupported_degeneracy
resource_exhausted
numeric_failure
incomplete_continuation
invalid_input
```

Un cap, une allocation refusée, un overflow ou une continuation non consommée
ne publie jamais un préfixe de payload. La séquence est
`count -> preflight -> fill -> validate -> publish` et les identités
`planned=filled=consumed`, `pending=0` sont bloquantes.

## 3. Étape zéro : recevoir l'objet avant la parcimonie

La chaîne de réception est :

```text
0A  BallForm exhaustive -> BallKey/RLE -> census -> BallEvent exact
0B  BallEvent -> spool scellé -> tri/merge exact -> lots -> fold
    -> dix forêts -> verticales -> BenchmarkOutputContract-v1
```

L'émission par ancre n'a aucun watermark monotone. Le chemin peut écrire des
runs spillables, mais il scelle la source et merge globalement les niveaux
avant le premier commit d'un lot. « Streamé » signifie mémoire résidente
bornée, jamais fold en ligne sur une source encore ouverte.

### 3.1 Statut de `BallFormToBallEvent-v0`

Le pin historique `2b89ea1` fournit un candidat borné `coord<=64`. Il n'est pas
reçu pour le profil u16 :

- des numérateurs q3/q4 de 67 à 81 bits sont rabattus vers `int64` ;
- des carrés jusqu'à environ 162 bits sont formés en `i128` avant le pgcd ;
- le juge de Gram forme lui-même un numérateur de rayon d'environ 167 bits sur
  un triangle u16 valide ;
- la positivité n'est pas jugée indépendamment ;
- l'owner ne sépare pas index dense et `PointId` ;
- le mutant de clé s'auto-déclare fautif sans clé de référence ;
- le census est exécuté par support avant le groupement par sphère ;
- `kUnsupported` est compté, mais le diagnostic publie encore `accord=OUI` et
  sort zéro ;
- aucun `BallEvent` versionné, niveau exact, lane ou manifeste transactionnel
  n'est construit ;
- le générateur `clusters --points=5 --coord=4` peut boucler sans fin faute de
  preflight de sa capacité.

L'autorité reçue localement se limite à deux constructions rationnelles du
centre et au signe de puissance sur les supports conservés par le sujet.

### 3.2 Réparation arithmétique

Pour un centre rationnel `N/den` et un point `p` de la sphère, ne pas former
`den^2` et `N^2` avant réduction. Employer directement :

```text
A = den
B = -2*N
C = 2*N dot p - den*||p||^2
```

Pour q3, la forme compacte de § 5.3 évite entièrement le centre gonflé. Pour
q3 et q4, la positivité se décide sur les numérateurs barycentriques
Gram--Cramer. Le juge u16 emploie BigInt/rationnels ou un codec homogène dont
toutes les largeurs sont prouvées ; le `i128` du sujet et celui du juge ne se
recertifient pas mutuellement. Toute conversion rétrécissante est précédée
d'un preflight exact ou supprimée.

### 3.3 Ordre RLE/census obligatoire

```text
formes positives
  -> BallKey + SupportRecord
  -> count/sort/RLE BallKey
  -> range-count saturé par BallKey unique
  -> census complet par BallKey survivante
  -> joindre I_B/U_B à tous les supports incidents
```

La porte exige `census_calls=unique_BallKeys`, deux supports pour une clé et un
mutant `census-par-support`. L'oracle juge séparément dépendance affine,
positivité, clé primitive, niveau, owner, `I_B/U_B` et activation de lane.

### 3.4 Statut du probe nommé stage 0B

Le successeur `91aa287`, encore présent au `HEAD=3c11bc8`, ne ferme pas 0B. Il
trie les runs réguliers, unionne les membres `I_B union U_B` dans une seule DSU
de `PointId`, puis compare ses goulots à Floyd--Warshall sur les mêmes runs,
membres, dispositions et niveaux.

Ce probe reçoit au plus la fermeture de cet hypergraphe fourni. Il est faux
pour la sémantique HGP dès `k=2` : les générateurs `S={0,1,2}` et
`T={0,3,4}` ont une intersection de taille un, donc restent distincts à
l'ordre deux, tandis que la DSU de points les fusionne par `0`.

Il manque toujours `q_min`, lanes, naissances, facettes, macro-lots, coverage,
dix forêts, verticales et payload. Son comparateur lit les limbs i128 de la
clé et vérifie les overflows **après** des multiplications signées : il n'est
ni total, ni sûr, ni opaque au profil. La réparation réutilise
`saturated_fold.hpp`, `gamma_forest_judge.cpp` et les fixtures Gate D par
ordre, au lieu d'étendre la DSU de points.

Le fold reçoit seulement un token préflighté :

```text
ExactLevelToken = schema + numerator>=0 + denominator>0 + canonical_encoding
```

Tous les tokens sont construits en arithmétique exacte avant le tri. Leur
comparateur est pur, total et sans chemin d'échec. Un overflow signé détecté
après multiplication est déjà un comportement indéfini ; une faute numérique
reste `numeric_failure` sous toute injection et ne tue jamais un mutant d'un
autre composant.

## 4. Fenêtre d'arêtes certifiée

Pour chaque lane, `E_q(a)` contient les seconds endpoints dont la paire n'est
pas fermée par assez de crédits universels à IDs disjoints. Une vraie arête
maximale canonique de support doit rester dans cette fenêtre. Le reporter porte
des spans de `GenerationRank`, jamais une table de `PairId`.

Les fates sont exclusifs :

```text
input_mass = closed_mass + open_mass + pending_mass
```

`pending_mass=0` est nécessaire à une fenêtre finale. Deux range-adds par span
ouvert puis un scan calculent les degrés orientés et leur somme sans expansion
des paires. Une égalité avec l'expansion PairId est exigée sur petit `n`.

Après la fenêtre, les compteurs doivent distinguer le premier facteur carrier
du vrai produit quaternaire :

```text
F3 = CarrierBlocks physiques initiaux
C4_carrier = sum over open q4 owner edges of acute geometric carriers
F4 = WST4Blocks physiques après prune carrier
M4_apex = masse logique exact-once (owner edge, primary acute carrier, apex)
W4_positive = masse après barycentriques strictes
H4_rank = masse après rang/census
N4_event, Z4_const, R4_bundle, T4_site = coûts distincts de la sweep
```

Pour `e={a,b}`, `L_e` contient les sites dont les deux distances à `a,b` ne
dépassent pas `D_e=||b-a||^2`, et `C_e` ceux dont `abx` est aigu avec owner
longueur/`EdgeKey`. `M4_apex` compte les paires `{x,y}` de `L_e` dont la sixième
distance ne dépasse pas `D_e`, dont l'owner parmi six arêtes est `e`, dont au
moins un membre appartient à `C_e`, et dont l'orientation 3D est non nulle. Si
les deux faces sont aiguës, le plus petit `PointId` est le carrier primaire.

Une fenêtre d'arêtes sparse ne borne ni `C4_carrier`, ni `M4_apex`, ni les
sphères uniques. Un compteur qui scanne seulement `x` vise `C4_carrier`, jamais
`M4_apex`. Tout bloc `MIXED` contribue à `M4_apex_pending` ou à des bornes
`M4_apex_L/M4_apex_U`; `pending=0` est requis avant une valeur finale. Les
anciens reçus `M=sum m_ab` sont renommés `L4_form_v0` et ne sont pas
réinterprétés silencieusement.

### 4.1 Raffinement local porteur de preuves

Le raffinement des terminaux q4 ouverts est sûr : une fermeture universelle du
parent reste vraie sur les enfants, tandis qu'un parent inconclusif peut devenir
certifiable après restriction. À `n=3000,s=8`, profondeur quatre, il réduit
`E4` de `4 045 644` à `2 597 699` sur `eight_clusters` et de `1 027 538` à
`464 599` sur `uniform`.

Ce gain ne reçoit pas encore le coût. Les recertifications passent
respectivement de `31 538 327` à `199 169 436` et de `108 858 186` à
`193 020 841`. Les compteurs de tête double-comptent aussi les parents jetés et
impriment jusqu'à `380,15 %` de masse q2 fermée ; seul le ledger terminal est
exclusif.

`ProofCarryingLocalRefinement-v0` transporte donc avec chaque split :

```text
credit_spans ALL disjoints + PointId/digest
none_spans définitivement élagués
frontier de tâches MIXED ou non visitées
credit_count et continuation persistante
```

Les enfants héritent `ALL/NONE` et ne rejouent que `MIXED`. Les statistiques de
tentatives restent distinctes des fates terminaux. Le gate compare les mêmes
`E_q` au parcours depuis racine, mesure les lectures évitées et poursuit
jusqu'à `M4`, BallRuns, census et fold. Une baisse de `E4` seule ne décide pas
la rentabilité.

### 4.2 `CKPairTape` : source q2 factorisée exacte

La fenêtre doit désormais être portée par une vraie décomposition de
Callahan--Kosaraju. Une WSPD canonique produit `O(s^3 n)` rectangles
`A×B` en dimension trois et partitionne exactement toutes les paires non
ordonnées. Une paire n'est un support géométrique q2 propre que si
`D=||b-a||^2>0`. L'entrée doit donc rejeter ou quotienter les positions
dupliquées, ou le tape filtrer `D=0` exactement. Sous cette porte, il est la
source q2 complète ; ce n'est pas un simple proposer.

Chaque rectangle porte une partition persistante du witness tree et deux
bornes : masse strictement intérieure garantie `L_open`, et cardinalité de
l'union fixe des IDs encore possiblement dans une boule fermée `U_closed`.
Pour `H=(z-a) dot (b-z)`, `max H<=0` exclut seulement l'intérieur ; seul
`max H<0` exclut aussi le shell et permet de retirer un span de `U_closed`.
Sous `smax=11`, `L_open>=10` ferme tout le rectangle ; `U_closed<=9` donne un
packet d'au plus neuf IDs qui suffit à rejouer exactement intérieur et shell de
chaque paire. Les égalités et endpoints masqués relationnellement restent dans
`U_closed`; seuls les facteurs encore indécis sont scindés et les preuves
s'héritent.

Le tape physique peut être linéaire alors que `sum |A||B|` est quadratique.
Un bloc accepté reste donc paresseux jusqu'à un consommateur factorisé reçu ou
jusqu'au preflight d'une expansion atomique. La WSPD ne rend égaux ni niveaux,
ni `BallKey`, ni census.

## 5. Générateur q3 recommandé

### 5.1 Réduction exacte arête-owner × porteur

Pour `d=b-a`, `u=x-a`, poser :

```text
D = d dot d
E = u dot u
F = d dot u
X = D+E-2F
V = 2u-d
```

Si `ab` est maximale faible, donc `D>=E` et `D>=X`, les deux angles adjacents
à `ab` sont strictement aigus. La positivité q3 équivaut alors à :

```text
V dot V > D
```

car `V dot V-D=2(E+X-D)`. L'égalité est un triangle droit. Chaque q3 est donc
une incidence canonique `owner EdgeKey(ab) × PointId(x)`, pas un triplet libre.

### 5.2 `OwnedCK-WST3` : extension exacte aux triplets

La source doit être une **partition** CK des paires non ordonnées de
`PointId`, pas une WSPD qui les couvre éventuellement plusieurs fois. Chaque
paire possède un unique `RectId` stable. À l'échelle carrier choisie pour ce
rectangle, les `CellId` half-open forment une antichaîne qui partitionne les
IDs de la fenêtre : un parent et l'un de ses descendants ne coexistent jamais.
Tout raffinement remplace atomiquement un atome par l'ensemble complet de ses
enfants disjoints. Ces trois propriétés, avec l'owner total
`(distance^2 maximale, EdgeKey minimale)`, portent la preuve exact-once ; elles
ne peuvent pas être remplacées par une déduplication après émission.

Pour un rectangle CK `R=(A,B)`, choisir une boule déterministe
`B_R=(o_R,r_R)` contenant `A union B`, puis un niveau Morton dont la maille est
comparable à `eta*r_R`. Énumérer les cellules non vides `C` de ce niveau qui
rencontrent `2B_R`. Cette constante est sûre et sharp : avec
`a=m-h,b=m+h,x=m+q`, les endpoints donnent
`||m||^2+||h||^2<=r_R^2`, la maximalité donne
`||q||<=sqrt(3)||h||`, puis `||x-o_R||<=2r_R`.

Si `A` est dans `B(c_A,r_A)`, `B` dans `B(c_B,r_B)` et `U_AB` majore les
distances endpoint, intersecter encore avec
`B(c_A,U_AB+r_A)` et `B(c_B,U_AB+r_B)`. Chaque facteur est une enveloppe sûre ;
un test AABB--boule indécis conserve la cellule.

Chaque triangle choisit l'arête de longueur maximale, puis la plus petite
`EdgeKey` à égalité. Cette arête appartient à un rectangle CK unique et le
carrier à une cellule half-open unique : le tuple retenu
`OwnedCK-WST3(A,B,C)` est exact-once. Pour `0<eta<=1`, son nombre de blocs
initiaux est `O(s^3*eta^-3*n)`. Aucun théorème n'impose
`eta=Theta(1/s)` : commencer avec `eta=Theta(1)` et compter séparément les
splits `MIXED`. Ce majorant ne couvre ni la localisation, ni les diagonales, ni
tous les raffinements `MIXED`.

Après owner, q3 est positif si `E+X-D>0` et `G=D*E-F^2>0`. Les blocs reçoivent
`ALL_ACUTE/NONE_ACUTE/MIXED` par bornes entières sûres ; les égalités
descendent. `ALL_ACUTE` reste une source factorisée exacte, jamais la preuve
que sa masse cubique, son rang, son shell ou ses `BallKey` sont bon marché. Le
tape carrier est requis dès que `q3_open || q4_open`; une fermeture de rang q3
ne supprime jamais la relation géométrique nécessaire à q4.

Le classifieur d'acuité de boîte peut être exact sur l'enveloppe continue à
coût constant. Poser `K=(a-x) dot (b-x)`. Sous owner `ab` et IDs distincts,
les deux angles adjacents à `ab` sont déjà strictement aigus ; `K>0` décide le
troisième et exclut la collinéarité. Par axe, minimiser
`(a_j-x_j)(b_j-x_j)` revient à essayer les quatre couples d'endpoints `a_j,b_j`
et `x_j=clip((a_j+b_j)/2,C_j)` ; le maximum essaie les mêmes couples aux deux
endpoints de `C_j`. Les trois axes sont indépendants, donc leurs minima/maxima
s'additionnent. `K_min>0` donne `ALL_ACUTE`, `K_max<=0` donne `NONE_ACUTE`,
sinon le bloc se scinde. L'owner emploie parallèlement les bornes exactes de
distance AABB et descend toute égalité jusqu'au tie-break `EdgeKey`.

Avant l'extension, `JungDiskDepth9` peut fermer une paire singleton ou un
microtile dont toutes les paires sont explicitement rejouées. Il n'est pas un
certificateur de rectangle CK tant qu'un `BlockJungDiskDepth(A,B,G)` uniforme
n'est pas prouvé : le plan, le disque et les demi-plans varient avec `a,b`. Sur
le résiduel, un quatrième facteur witness applique `L_open/U_closed`; neuf
intérieurs ferment, tandis qu'au plus huit IDs de l'union fermée possible
donnent un packet exact. Pour la puissance q3, `min P>=0` exclut seulement
l'intérieur et `min P>0` exclut aussi le shell.

### 5.3 Pied unique et puissance exacte

Pour un carrier owner, poser :

```text
G = D*E-F^2
W = E*(D-F)*d + D*(E-F)*u
```

`G>0` est l'indépendance affine. Le centre q3 vaut `a+W/(2G)`. Pour
`v=z-a`, le prédicat de puissance mis à l'échelle est :

```text
P_x(z) = G*||v||^2-v dot W
```

`P_x(z)<0` signifie intérieur strict et `P_x(z)=0` coquille. Les cinq
coefficients pré-census sont :

```text
A = G
B = -(2G*a+W)
C = G*||a||^2+a dot W
```

Ils sont réduits par pgcd/signe puis intégrés à la `BallKey`. Sous u16, les
évaluations exigent environ 105 bits signés; le chemin device emploie des limbs
explicites, jamais un `i64` implicite.

### 5.4 `Q3FootPowerRange-v0`

Après RLE, une wavefront LBVH traite `(BallRun,NodeKey,count<=9)`. Sur une AABB
entière, `P_x` est somme de trois quadratiques convexes. Par axe, le minimum
entier se trouve aux deux entiers voisins de `W_i/(2G)` clipés et le maximum à
une extrémité.

- `max P<0` crédite toute la population du nœud ;
- `min P>=0` élague le nœud pour le rang strict ;
- sinon la tâche se scinde ;
- le neuvième intérieur rejette q3 sous `smax=11` ;
- une survivante paie un seul census complet.

Ce backend est le v0 parce qu'il est simple à juger, pas parce qu'il domine
asymptotiquement tous les cas. Le ledger publie tâches run×nœud, populations
créditées, feuilles, rejets au neuvième, opérations larges, octets et HWM.

### 5.5 Préfixes Morton et niveaux shallow

Une AABB serrée ne donne aucun packing. Pour un terminal reçu avec `Dlo>0` et
`kappa=Dhi/Dlo` borné, choisir un niveau Morton dyadique commun dont la maille
est comparable à `sqrt(Dlo)/s3`. Le nombre de cellules alignées rencontrant la
région carrier est conditionnellement :

```text
O(s3^3 * kappa^(3/2)) par terminal
```

Cette borne ne contrôle ni le nombre de terminaux, ni leur population, ni
`M3`. Les cellules half-open, limites de profondeur et cas `Dlo=0` sont
explicitement délégués.

Si les visites LBVH sont rouges, `Q3FootLevelLocate-v1` construit seulement les
`k+1` bas niveaux des formes P et hauts niveaux des formes N. La complexité
combinatoire des niveaux est `O(mk)`, mais le coût conserve les point-locations
des `f` pieds, les bundles pondérés et les concurrences. Sans sweep reçu, une
cible honnête inclut `f log m`.

### 5.6 Limite de sortie

Des configurations réelles en dimension trois possèdent un nombre quadratique
de triangles Delaunay critiques, aigus et vides. Cette obstruction interdit une
promesse universelle de catalogue q3 sous-quadratique; elle ne donne pas encore
un pire cas `50000` sur grille u16 fixe. Une fixture u16 cocyclique de 384
points porte déjà `2322560` supports aigus pour une seule sphère. Le RLE sauve
le census, jamais une provenance de supports explicitement exigée.

Le contrat q3 est donc output- et resource-sensitive. Sur les familles sparse,
la route ci-dessus peut être rapide; sur une sortie lourde, elle fournit un
preflight exact, un quotient reçu ou `resource_exhausted`.

## 6. Crédits universels : LP, rectangles et cages

### 6.1 Théorème LP projectif

Translater l'ancre en zéro. Pour `s_i=z_i-a`, `q_i=||s_i||^2`, `d=b-a` et
`D=||d||^2`, définir :

$$\kappa_P(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

Le pool `P` place au moins un intérieur dans toute sphère passant par `a,b` si
et seulement si `d` appartient au cône positif de `P` et `kappa_P(d)<D`.
L'égalité reste ouverte. Un optimum basique emploie au plus trois IDs.

Ce théorème décide `UniversalSphereDepth-1`, propriété plus forte que les
seules sphères Morse. Sur un pool capé, le succès est sûr et l'échec fail-open.
La complétude exige le pool mondial authentifié.

Si un groupe `G` crédite une fois, la profondeur universelle vérifie exactement :

```text
C_0(P,d) = true
C_h(P,d) iff, for every z in G, C_(h-1)(P without z,d)
```

Avec des bases de taille au plus trois et le contrat `smax=11`, les profondeurs
`h=8/9/10` demandent au plus `3280/9841/29524` appels LP. Leur association
q4/q3/q2 vient donc de ce contrat de rang, pas des lanes seules. Ce sont des
nombres d'appels d'oracle borné, pas une borne du reporter ni un hot path par
paire. Rangs un/deux, stricte, shell, suppressions par `PointId` et coût du
solveur exact restent explicites.

### 6.2 `JungDiskDepth` : restreindre aux centres Morse admissibles

Translater `a` en zéro, poser `y=2c`, `d=b-a` et `D=||d||^2`. Les domaines
nécessaires des centres d'une sphère q3 ou q4 dont `ab` est l'arête maximale
sont :

$$K_3(d)=\left\lbrace y:y\mathbin{\cdot}d=D,\ ||y-d||^2\leq\frac{D}{3}\right\rbrace,$$

$$K_4(d)=\left\lbrace y:y\mathbin{\cdot}d=D,\ ||y-d||^2\leq\frac{D}{2}\right\rbrace.$$

Un témoin relatif `s=z-a` est intérieur lorsque
`y dot s>||s||^2`. L'intersection de `K_q(d)` avec les demi-plans opposés d'un
groupe est vide si ce groupe fournit au moins un intérieur pour tout centre
admissible. Helly dans ce plan affine donne une sous-base d'au plus trois IDs.
Neuf groupes disjoints ferment q3 et huit ferment q4.

Ce théorème fixe une paire ponctuelle. Pour un rectangle `A×B`, `d`, le plan,
le disque et les demi-plans changent avec les endpoints. Il faut donc soit
scinder jusqu'à une paire ou un microtile explicitement rejoué, soit prouver un
nouveau classifieur `BlockJungDiskDepth(A,B,G)` quantifié sur tout le produit.
La fixture `A={(0,0,0),(0,100,0)}`, `B={(100,0,0),(100,100,0)}` et
`z_j=(50,j,0)`, `j=0,...,7`, ferme q4 pour la paire basse mais ne fournit même
aucun témoin q2 pour la paire haute. Elle interdit toute promotion depuis un
représentant vers le rectangle.

Un certificat dual donne néanmoins une voie uniforme falsifiable. Écrire
`m=(a+b)/2`, `h=(b-a)/2`, `c=m+w`, avec `w dot h=0` et
`||w||<=kappa||h||`, où `kappa^2=1/3` pour q3 et `1/2` pour q4. Pour un groupe
de témoins, le minimax sur le disque de `w` et le simplexe fournit des poids
rationnels `lambda_z>=0`, de somme un. Avec
`alpha=||h||^2-sum lambda_z||m-z||^2` et `p=m-sum lambda_z z`, la couverture
de la paire équivaut à :

```text
q3 : alpha>0 et 3*alpha^2 > 4*(||h||^2||p||^2-(p dot h)^2)
q4 : alpha>0 et   alpha^2 > 2*(||h||^2||p||^2-(p dot h)^2)
```

Helly borne une base à trois IDs. Pour un témoin singleton, ces deux conditions
se réduisent exactement à `H>0 && 4H^2>EX` et
`H>0 && 3H^2>EX` : `BlockJungDualTile` généralise donc `SOC64`.

La forme entière évite toute division. Pour des poids entiers `w_z>=0`, poser
`L=sum w_z`, `Z=sum w_z*z`, `Q=sum w_z*||z||^2`,
`A=-L*(a dot b)+(a+b) dot Z-Q` et
`C=(L*a-Z) cross (L*b-Z)`. Le même reçu vérifie :

```text
q3 : A>0 et 3*A^2*L^2 > ||C||^2
q4 : A>0 et 2*A^2*L^2 > ||C||^2
```

La largeur dépend du dénominateur commun `L`; un cap dépassé rend `UNKNOWN`.

Une forme équivalente, directement adaptée à une implémentation sans fractions,
pose `W=sum_z w_z`, `D=||b-a||^2`,
`A=W*D-sum_z w_z||a+b-2z||^2`,
`P=W*(a+b)-2*sum_z w_z*z` et
`R=D||P||^2-(P dot (b-a))^2`. Elle décide :

```text
q3 : A>0 et 3*A^2>4*R
q4 : A>0 et   A^2>2*R
```

Sous u16, la preuve de largeur i128 exige `W=sum_z w_z<=65535`, contrôlé sans
overflow avant le prédicat. Le prototype live ne possède pas encore ce
preflight, ni un juge géométrique indépendant pour les groupes de deux ou trois
témoins ; il reste donc un proposer ponctuel, pas un certificateur de bloc reçu.

L'ABI de vérification doit nommer exactement ce qu'elle décide. Une primitive
`verify_dual_weights_lane` reçoit une paire propre `D>0`, de un à trois
`PointId` authentifiés, des poids strictement positifs et un cap vérifié sur
`L`. Elle certifie uniquement **ces poids fournis**. Son échec ne dit pas que
le groupe ne couvre pas le disque : un autre vecteur rationnel peut réussir,
et des vecteurs différents peuvent certifier q3 et q4. Le wrapper de profondeur
garantit la disjonction des groupes de `PointId`; une simple routine de
coordonnées ne peut pas porter cette propriété. Coordonnées hors u16, somme de
poids hors cap, paire dégénérée, overflow potentiel ou recherche incomplète
rendent `UNKNOWN` avant tout calcul étroit.

Le proposant peut essayer une petite banque de poids puis scinder, mais son
gain n'est qu'un minorant. Le juge `k>1` compare la couverture du disque continu
à une faisabilité exacte des demi-plans, pas à quelques centres échantillonnés
ni au même prédicat dual. Les mutants de largeur emploient une arithmétique
définie ; un overflow signé volontaire n'est pas une contradiction causale.

Au niveau rectangle, une base et des poids rationnels proposés ne créditent
`ALL` que si les deux inégalités sont prouvées sur tout `A×B`. Une première
borne entière peu coûteuse utilise
`alpha=-a dot b+(a+b) dot zbar-qbar`, puis une enveloppe de `h cross p` ;
Bernstein/SOS ou un split traite le résiduel. L'échec reste `MIXED`. Les coins
seuls ne suffisent pas : deux témoins peuvent couvrir tous les couples extrêmes
et n'être que shell pour une paire médiane.

Cette promotion est seulement un certificat suffisant :
`for all (a,b) exists lambda(a,b)` n'implique pas l'existence de poids communs
au rectangle. Les poids fixes vérifiés donnent `ALL`; leur échec, une marge
nulle ou un dénominateur au-delà du cap donnent `MIXED/UNKNOWN`. La rationalité
découle de la marge stricte par densité, sans borne universelle de dénominateur.

La contre-fixture cartésienne
`z1=(9,1,0)`, `z2=(2,2,0)`,
`A={(4,0,0),(2,1,0)}` et `B={(0,10,0),(6,9,0)}` rend ce swap concret. Chaque
paire possède un poids q4 valide, mais `a1,b2` impose approximativement
`0.290<lambda(z1)<0.366`, tandis que `a2,b1` impose
`0<=lambda(z1)<0.058`. Aucun poids commun ne ferme donc le produit. Scinder
est une partie mathématique de l'algorithme, pas une faiblesse du proposer.

Cette porte paire-level est strictement plus adaptée que le LP sur tout le
plan. Un échec du LP global ne diagnostique pas une pénurie de témoins sur
`K_q`. Le hot path propose et valide de petits groupes ; l'arbre de suppressions
reste un oracle. Sur un rectangle non prouvé uniformément, le résultat reste
`MIXED`.

### 6.3 Base primale exacte et profondeur Jung

Le dual sert au hot path ; une autorité indépendante se formule directement
comme une projection convexe en dimension deux. Pour une paire fixe, poser
`d=b-a`, `D=||d||^2`, `u_z=a+b-2z` et `s=2w`. Les mauvais centres, où `z`
n'est pas strictement intérieur, vérifient :

```text
s dot d = 0
2*s dot u_z >= D-||u_z||^2
q3 : 3*||s||^2 <= D
q4 : 2*||s||^2 <= D
```

Pour un groupe `G`, soit `P_G` l'intersection des demi-plans mauvais dans le
plan orthogonal à `d`. Si `P_G` est vide, `G` couvre le disque. Sinon poser
`r_G^2=min_{s in P_G}||s||^2`. Le reçu exact est :

```text
q3 : 3*r_G^2 > D
q4 : 2*r_G^2 > D
```

L'inégalité est stricte : l'égalité fournit un centre de shell et ne crédite
aucun intérieur. En dimension deux, le minimiseur est nécessairement l'origine,
la projection de l'origine sur une frontière, ou l'intersection de deux
frontières. Pour une base de trois IDs au plus, le vérificateur énumère donc un
nombre constant de candidats, contrôle toutes les inégalités en entier et ne
partage aucune formule `A/P/R` avec le sujet. Pour un pool plus grand, une
intersection de demi-plans exacte construit le polygone fermé `P_G`. S'il est
vide, Helly fournit un conflit d'au plus trois demi-plans. S'il ne l'est pas,
le point de norme minimale est calculé ; lorsqu'il est hors du disque de Jung,
ses conditions KKT n'emploient qu'un ou deux demi-plans actifs. Une base de
trois n'est donc nécessaire que lorsque l'intersection des demi-plans est vide.
Le coût complet est `O(N log N)` pour le tri angulaire puis `O(N)` pour
l'intersection et le point le plus proche. Le hot path peut seulement proposer
une base, car une base vérifiée suffit à fermer.

Dans la récurrence leave-out, l'ordre angulaire est trié une fois pour la paire
puis filtré en `O(N)` à chaque état. Les demi-plans parallèles et les
coordonnées dupliquées ne sont jamais coalescés définitivement : supprimer le
`PointId` portant la contrainte la plus forte peut rendre la suivante active.
Il faut conserver tous les IDs, ou au moins les `h` contraintes les plus fortes
par direction pour une décision de profondeur `h`.

Une écriture entière évite de construire une base orthonormée du plan. Poser
`v_z=2*(D*u_z-(u_z dot d)*d)` et
`c_z=D*(D-||u_z||^2)`. Comme `s dot d=0`, le mauvais demi-plan devient
`s dot v_z>=c_z`. Une projection sur un bord a
`r^2=c_z^2/||v_z||^2`; une intersection de deux bords se résout par leur Gram.

La promesse i256 exige toutefois une réduction avant les comparaisons. Pour les
contraintes `i,j,k`, poser :

```text
g_i    = D-||u_i||^2
K_ij   = D*(u_i dot u_j)-(u_i dot d)*(u_j dot d)
Delta  = K_ii*K_jj-K_ij^2
N      = g_i^2*K_jj-2*g_i*g_j*K_ij+g_j^2*K_ii
```

On a `v_i dot v_j=4*D*K_ij` et `c_i=D*g_i`. Si `K_ii=0`, `g_i>0` rend
l'intersection vide et `g_i<=0` rend la contrainte redondante. Sinon une
projection `i` est faisable si `g_i*K_ik>=g_k*K_ii` pour tout `k`, puis ferme
q4 si `g_i^2>2*K_ii` et q3 si `3*g_i^2>4*K_ii`. Pour deux bords, exiger
`Delta>0`, rejouer chaque contrainte `k` par :

```text
(g_i*K_jj-g_j*K_ij)*K_ki
 +(g_j*K_ii-g_i*K_ij)*K_kj >= g_k*Delta
```

puis fermer q4 si `N>2*Delta` et q3 si `3*N>4*Delta`. Sous u16,
`|g|<2^36` et `|K|<2^71`; ces formes réduites restent sous environ 180 bits.
i256 est donc sûr sur cette route. Une évaluation naïve en `v/c/Gram` peut
dépasser 256 bits lors du replay contre le troisième demi-plan et doit rester
GMP. Cas `D=0`, normale nulle, contraintes parallèles, `Delta=0` et tout
dénominateur non positif rendent le candidat non admissible ou la tâche
`UNKNOWN`; chaque candidat retenu est rejoué contre toutes les contraintes.
Le dual `A/P/R` i128 reste le vérificateur GPU plus compact.

La profondeur exacte utilise ensuite un DAG de suppressions. Écrire
`Depth(P,0)=true`. Pour `h>0`, trouver une base `G`, de taille au plus trois,
qui couvre le disque, puis vérifier :

```text
Depth(P,h) = AND over z in G of Depth(P minus {z},h-1)
```

La récurrence est un iff si `G` est réellement couvrant. Au centre considéré,
au moins un `z` de `G` est intérieur ; le fils qui retire cet ID fournit les
`h-1` autres intérieurs distincts. Le nombre maximal d'appels de recherche de
base est `(3^h-1)/2` : `3280` pour `h=8` et `9841` pour `h=9`. Ces valeurs
correspondent à q4/q3 seulement sous le contrat `smax=11`. C'est un oracle
borné, pas le hot path par paire. Même avec seulement 32 octets par nœud
interne, le pire cas représente environ 102,5 KiB pour q4 et 307,5 KiB pour q3
par paire. Un DAG partagé est authentifié par
`(PairId, lane, h, RemovedIdSet)` ; partager seulement sur la base courante
serait faux. La disjonction porte sur chaque chemin racine--feuille. Des
branches sœurs peuvent réutiliser légitimement le même ID.

La promotion CK se définit sans changer de quantificateur. Pour une proof-tile
`Q` et l'ensemble des IDs déjà retirés `S`, poser `BlockJD(Q,S,h)` égal à
« pour toute paire propre de `Q`, la profondeur du pool authentifié privé de
`S` est au moins `h` ». À chaque nœud, les mêmes IDs de base et les mêmes poids
sont vérifiés uniformément sur tout `A×B`, puis les enfants portent
`S union {z}`. Un échec de proposition, de largeur ou de borne rend `MIXED` ;
une partition disjointe des vrais enfants CK remplace alors la tuile, jusqu'à
l'oracle paire singleton. Un reçu conserve au minimum `RectId`, `TileId`,
lane, `h`, `smax`, digest de `RemovedIdSet`, IDs/poids de base, cap de largeur,
bornes polynomiales, handles enfants et preuve de partition.

Il faut en outre garantir `D>0` sur la tuile. Un témoin qui devient l'un des
endpoints est shell et ne crédite rien : l'ABI simple exige donc que les IDs de
base soient disjoints des facteurs endpoint, ou scinde explicitement les
stripes relationnelles correspondantes. Si la politique d'entrée conserve
deux `PointId` à la même position, leur multiplicité reste réelle pour la
profondeur. Par exemple, pour `a=(0,0,0)`, `b=(4,0,0)`, deux IDs distincts en
`(2,0,0)` donnent profondeur deux ; un quotient silencieux par coordonnées la
détruirait.

#### Noyau de Helly avec tolérance et hypergraphe exact

Le DAG exponentiel n'est pas la seule forme de reçu. Pour une paire fixe,
noter `B_z=K_q intersect H_z` l'ensemble convexe fermé des centres admissibles
où `z` n'est pas strictement intérieur. Un centre appartient à tous les `B_z`
sauf au plus `h-1` exactement lorsqu'il possède au plus `h-1` témoins
intérieurs. Ainsi `Depth(P,h)` signifie que la famille `{B_z:z in P}` n'a
aucun point commun avec tolérance `h-1`.

Le théorème de Helly avec tolérance de Montejano--Oliveros donne alors, par
contraposée, un sous-pool `C` qui certifie déjà la même profondeur et vérifie
`|C|<=eta(3,h)`. La borne d'Erdős--Gallai--Tuza satisfait
`eta(3,h)<binom(h+2,2)+binom(h+1,2)=(h+1)^2`. Il existe donc toujours un reçu
de **80 IDs au plus pour q4** et de **99 IDs au plus pour q3**, sous
`smax=11`. La source primaire du transfert tolérant est
[Tolerance in Helly Type Theorems](https://doi.org/10.1007/s00454-010-9296-6),
théorème 3.1.

Il existe une formulation algorithmique encore plus directe. Construire
l'hypergraphe `E_q(P)` dont les sommets sont les `PointId` témoins et dont les
hyperarêtes sont tous les groupes `G`, `1<=|G|<=3`, qui couvrent `K_q`. Alors :

```text
Depth_q(P) = tau(E_q(P))
```

où `tau` est la taille minimale d'un transversal d'hyperarêtes. En effet,
l'ensemble des témoins intérieurs en un centre frappe chaque base couvrante,
donc `tau<=Depth`. Réciproquement, si `R` frappe toutes les bases et si les
mauvais demi-plans de `P minus R` avaient une intersection vide, Helly
fournirait une base couvrante disjointe de `R`, contradiction. Il existe donc
un centre où tous les IDs hors `R` sont mauvais, d'où `Depth<=|R|`. Les trois
quantités du diagnostic ont ainsi un sens combinatoire exact : `u` compte les
arêtes singleton, `p=nu(E)` est le packing maximal et `d=tau(E)`, avec
`u<=p<=d`.

Un constructeur exact fonctionne par séparation de coupes. Il maintient un
petit ensemble `F` d'hyperarêtes déjà certifiées et résout le transversal borné
`tau(F)`. Si `tau(F)>=h`, le reçu ferme. Sinon il choisit un transversal
`R`, `|R|<h`, et appelle l'intersection de demi-plans sur `P minus R`. Un
contre-centre prouve `Depth<h`; une intersection vide rend une nouvelle base
`G` disjointe de `R`, qui est ajoutée à `F`. Sous cap, l'absence de terminaison
rend `UNKNOWN`. Le replay GPU vérifie chaque petite base géométrique une seule
fois, puis résout `tau(F)>=h` par branches bitset de facteur au plus trois ; il
ne répète plus la géométrie à chaque nœud leave-out.

Un `ToleranceKernel` contient ces IDs authentifiés et les hyperarêtes utilisées,
avec leurs reçus. Une autre autorité recalcule la profondeur en parcourant
toutes les faces de l'arrangement de leurs droites, clipé au disque de Jung.
Cet arrangement possède `O(|C|^2)` faces ; un replay simple de tous les IDs par
face coûte `O(|C|^3)`, constant mais encore trop cher comme boucle universelle.
Les faces de dimension zéro et un sont indispensables, car une égalité est
shell et peut porter le minimum. Un proposer exact peut extraire un noyau par
suppression d'IDs avec l'oracle, puis le device ne reçoit que la liste bornée
et le problème combinatoire de transversal.

Ce résultat rend le **payload** de succès sparse ; il ne borne ni le coût de
recherche, ni le nombre de proof-tiles CK. Surtout, il vaut pour une paire fixe :
`for all pair exists kernel` n'implique pas `exists kernel for all pair`. Sur
une tuile, un même noyau doit être vérifié uniformément ou conduire à un split.
Le `ToleranceKernel` complète donc le packing et peut remplacer le stockage du
pire DAG, sans abolir `BlockJD`, ses masques d'endpoints ni ses continuations.

Le fast path GPU cherche plutôt huit ou neuf groupes couvrants disjoints, ce
qui donne immédiatement la même profondeur avec huit ou neuf reçus. S'il
échoue, le DAG de suppressions peut poursuivre sous budget ; sinon la tâche
reste `MIXED`. Un `ProofNode` stocke au plus trois `PointId`, leurs poids ou le
reçu primal, trois handles enfants et le niveau. Les files SoA exécutent
`propose -> verify -> count/scan/fill`; un cap produit `PENDING`, jamais
`CLOSED`. Sur un rectangle CK, chaque nœud propose au représentant puis vérifie
uniformément ses polynômes ; un échec scinde `A/B`. La continuité d'une marge
stricte garantit qu'un reçu ponctuel reste valable sur un voisinage assez fin,
sans jamais promettre un poids commun sur une tuile grossière.

Le packing disjoint est strictement incomplet. Pour q4, prendre
`a=(0,100,100)`, `b=(40,100,100)`, les six témoins universels
`u_j=(j,100,100)`, `j=1,...,6`, puis
`g1=(20,111,100)`, `g2=(20,92,108)`, `g3=(20,92,92)`. Dans le disque
`Y^2+Z^2<=200`, les trois régions mauvaises sont :

```text
Y <= -279/22
Y-Z >= 17
Y+Z >= 17
```

Elles sont non vides mais deux à deux disjointes. Les gadgets fournissent donc
toujours deux intérieurs et la profondeur totale vaut huit. Pourtant seuls les
six `u_j` couvrent seuls ; parmi les trois gadgets, une seule paire disjointe
peut former un septième groupe. Le packing maximal vaut sept. L'analogue q3
emploie sept `u_j` et les gadgets `(20,113,100)`, `(20,91,109)`,
`(20,91,91)` dans `3(Y^2+Z^2)<=400` : profondeur neuf, packing maximal huit.
Ces fixtures rendent obligatoire le fallback leave-out pour une autorité
complète. Elles ont pourtant des preuves très compactes. Après la chaîne des
six singletons q4, prendre `G={g1,g2}` à profondeur deux ; ses deux fils à
profondeur un utilisent respectivement `{g2,g3}` et `{g1,g3}`. Le DAG q4 n'a
que neuf nœuds internes. L'analogue q3 en a dix après ses sept singletons. Le
réemploi de `g3` dans deux branches sœurs est valide et tue tout mutant qui
impose une disjonction globale au lieu d'une disjonction par chemin. Les trois
bases de gadgets possèdent aussi des reçus duals à poids `(1,1)` : pour q4,
`A^2-2R` vaut `3923216`, `3923216`, puis `1458176`; pour q3,
`3A^2-4R` vaut `8074928`, `8074928`, puis `2581248`.

### 6.4 `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2` :

```text
q2 : H>0
q3 : H>0 et 4H^2>EX
q4 : H>0 et 3H^2>EX
```

À `t` fixé puis à `e` fixé, chaque lane est une section convexe de cône de
Lorentz. Si les 64 couples de coins du produit relaxé
`(C-A)×(B-C)` passent, le vrai rectangle est `ALL`. Un échec reste `UNKNOWN`,
car les deux différences partagent réellement le même `z`.

Pour `a,b` fixés, le spindle est aussi convexe en `z`. Le prédicat est donc
séparément convexe en `a,b,z`; les 512 triples de coins caractérisent exactement
`ALL` sur l'enveloppe AABB continue. Un coin fictif échouant ne donne jamais
`NONE` sur les seuls points du nœud.

Le classifieur `JungSpindleRect-v0` du pin `7d2efcb` est une autre borne : il
combine des extrema séparés de `D,V,T`. Son disjonctif est sûr mais n'a gagné
qu'environ trois centièmes de point dans le diagnostic déclaré à `n=6000,s=8`.
Cette mesure ne réfute ni `SOC64` ni `CORNER512`. Leur primitive et leur probe
borné existent désormais dans le worktree, sans recevoir encore le shadow
WSPD ni son coût transitif. Elle réfute seulement l'espoir que les extrema
décorrélés suffisent sur des boîtes grossières.

Même exact pour `ALL`, `CORNER512` ne prouve aucune parcimonie. Compter ses
early exits, opérations larges, tâches et gain transitif avant de le porter.

Le micro-jalon prioritaire est `SOC64-shadow-q4`, avant toute promotion des
cages : au plus 4096 tâches `central-MIXED` par famille, 65 prédicats au pire
par tâche prétest central compris, aucun changement de fate. Il publie early
exits, masse créditable, opérations larges, temps et HWM. La fixture
`A=[0,99]x{100}x{100}`, `B=[101,200]x{100}x{100}`,
`C={(100,100,100)}` ferme q4 par `SOC64` alors que les extrema scalaires
échouent.

Si le signal est non vide, brancher `SOC64` avant le raffinement pour éviter
les splits. `CORNER512` vient seulement sur les tâches de forte masse où ses
513 prédicats, prétest compris, peuvent être amortis. Un échec reste
`AABB_envelope_not_all/UNKNOWN`, jamais `NONE` pour les points stockés.

Le premier raccord live du 14 août n'implémentait pas ce protocole : il
additionnait `soc_cred` d'un ancêtre SOC-`ALL` aux crédits baseline du même
nœud ou de ses descendants. Claude l'a réécrit après contre-audit. Le shadow
courant emploie deux parcours logiques : baseline inchangée et union combinée.
Dans l'union, il applique d'abord les fallbacks baseline ; si le verdict final
reste `MIXED`, il essaie SOC ; tout `ALL` arrête conceptuellement la branche.
Le flip est `baseline_open && combined_closed`, jamais
`cred+soc_cred>=seuil`.

Le replay `uniform,n=120` donne zéro faux sur `624` verdicts SOC et mesure `41`
fermetures de masse `95`, tandis que la somme réfutée en annoncerait `127` de
masse `316`. Cette contradiction doit devenir une porte. Le raccord n'a
toujours pas de cap maximal et a soumis environ `988000` tâches à `n=1000`.
Ajouter un échantillonnage déterministe capé, un statut tronqué, un juge d'IDs
distincts, une CTest intégrée et les mutants de chevauchement avant de publier
une masse gagnée ou un temps extrapolé.

Avec `floor>q2`, un retour inférieur au plancher signifie seulement que le
seuil est impossible ; ce n'est pas la lane `ALL` exacte. L'appel q4 peut lire
le booléen `retour>=q4`, mais l'API expose sinon `UNKNOWN_BELOW_FLOOR`. La
fixture `A={(0,0,0)}`, `C={(1,1,1)}`, `B=[0,4]x{1}x{1}` distingue q3 au centre
de `NONE` au minimum réel.

### 6.5 Cages de quatre à six sites

Une cage ancre-globale réduite est une positive basis inclusion-minimale de
dimension trois et peut contenir quatre, cinq ou six sites. Un constructeur
tétra-only est un fast path exact mais incomplet. Une cellule de Voronoï locale
à six facettes a au
plus huit sommets; sa fleur fournit donc un nombre constant de formes
quadratiques vérifiables sur un `BNode`.

Huit cages à unions d'IDs disjointes ferment q4, neuf ferment q3. Une banque
bornée est un proposer : échec, égalité, reuse d'ID, rang inférieur, overflow
ou cap délègue toujours. `0` intérieur au hull inversé prouve seulement un
rayon borné; une cible est fermée lorsque son point inversé est strictement
dans le hull, ou lorsque toutes les facettes transformées passent.

Les budgets doivent suivre la taille réellement validée des cages. Huit cages
six-sites peuvent consommer 48 IDs et jusqu'à 64 formes de fleur; neuf peuvent
en consommer 54 et jusqu'à 72. `P=48` est donc une capacité q4, jamais une
preuve d'existence ni un budget q3. Pour une base positive minimale 3D,
`omega`, le nombre maximal de ses vecteurs dans un demi-espace ouvert, vaut au
plus quatre. La condition `delta>=4h-3` est une borne suffisante pour extraire
gloutonnement `h` positive bases, soit `delta>=29` pour q4. Son optimalité
globale n'est pas démontrée et elle ne prouve pas que les fleurs obtenues
créditent une cible donnée ; elle ne devient jamais un rejet scientifique.

Le tri exact des rayons rationnels peut être plus large que les formes
ponctuelles; multiprécision ou majorant conservateur puis replay des formes est
requis. Le test directionnel d'une forme tient dans environ 87 bits signés sous
u16, alors qu'une comparaison naïve de deux rayons rationnels peut approcher
240 bits. Une cage minimisée doit recalculer sa cellule et sa fleur : retirer
une contrainte agrandit la cellule et peut perdre une fermeture. Aucune loi
globale de coût ne découle de la seule existence de cages.

### 6.6 LP comme diagnostic de cause

Avant d'investir dans un constructeur de cages GPU ou d'ajouter de la
profondeur, appliquer le LP projectif sur un échantillon déterministe du même
résiduel scellé après le masque central et `SOC64-shadow` :

- succès LP avec échec des rectangles : perte de factorisation ou de boîte ;
- échec de huit extractions gloutonnes mais succès de l'arbre exact : packing
  insuffisant ;
- échec avec pool mondial : profondeur universelle insuffisante, sans conclure
  à l'absence d'un support Morse ;
- échec sur pool capé : résultat inconnu.

Cette matrice choisit entre meilleur rectangle, packing directionnel, cage
quatre--six sites et moteur local. L'arbre q4 à 3280 appels reste un oracle
borné ; il ne devient jamais le hot path. Le solveur fraction-free ou
multiprécision reçoit sa propre preuve de largeur : `i128` suffit au replay
d'une forme, pas à toutes les comparaisons de valeurs rationnelles du
constructeur LP.

### 6.7 `OriginOnionDepth-h` collectif

Inverser une banque autour de l'ancre par
`p_z=(z-a)/||z-a||^2`. Poser `P_0=P`, puis retirer à chaque couche un ID
canonique pour chaque sommet géométrique non nul de
`K_j=conv({0} union P_j)`. Si `p_b` appartient à l'intérieur relatif des `h`
coques, toute sphère passant par `a,b` contient au moins un ID distinct de
chaque couche.

La preuve vient de l'équation inversée : la frontière vaut `y dot p=1`, son
intérieur `y dot p>1`, et `y dot 0=0`. Un point de valeur un dans l'intérieur
relatif force donc un sommet de la coque à valeur strictement supérieure à un.
Même rang affine entre `K_0` et `K_(h-1)`, puis appartenance stricte à la
dernière coque, est une condition suffisante moins chère.

Une facette `u dot p<=v` devient, pour `d=b-a`,
`v||d||^2-u dot d>=1`. Son minimum sur une AABB est séparable et son replay
reste sous environ 87 bits dans le profil u16. Rang qui chute, égalité, banque
capée ou IDs non authentifiés donnent `UNKNOWN`. `h=8/9/10` ferme
respectivement q4/q3/q2. Ce certificat crédite collectivement des BNodes, mais
reste universel.

### 6.8 Limite commune des certificats universels

Pour `A_i=(i,0,0)` et `B_j=(0,j,65535)`, avec
`1<=i,j<=25000`, chaque paire croisée possède une sphère vide admissible dans
le disque de Jung q4. LP, Jung, onion et cages peuvent donc laisser
`n^2/4` paires. Pourtant tous les triangles sont obtus, donc la vraie source
q3/q4 est vide.

La masse universelle ne devient plus une gate préalable à une source
factorisée. Ces certificats sont des OR de prune et des diagnostics ; la
positivité géométrique doit appartenir au générateur principal. La fixture
porte une masse quadratique dans quelques `RectKey`, puis exige zéro carrier
aigu et zéro sweep sans tableau de `PairId`.

## 7. q4 : `OwnedCK-WST4`, puis shallow local si nécessaire

Pour le même rectangle owner `R=(A,B)`, réutiliser les cellules carrier de
la fenêtre `2B_R`--lentille et former des couples non ordonnés `(C,D)`. Si `C!=D`, le bloc porte
`A×B×C×D`; si `C=D`, il porte `A×B×binom(C,2)` sans IDs répétés. Tout q4 dont
`ab` est l'arête maximale possède ses deux autres sommets dans cette fenêtre, donc un
unique bloc `OwnedCK-WST4`. Pour `0<eta<=1`, le nombre de blocs initiaux avant
filtres vaut `O(s^3*eta^-6*n)`. Commencer à `eta=Theta(1)` et raffiner les
seuls `MIXED`; ce majorant ne couvre ni localisation ni
raffinements `MIXED`.

La partition q4 doit rester explicite sous raffinement. Pour des enfants
half-open disjoints `C_i`, l'atome diagonal est remplacé par tous les
`binom(C_i,2)` et tous les `C_i×C_j` avec `i<j`; pour deux cellules distinctes,
`C×D` est remplacé par tous les `C_i×D_j`. Le remplacement est atomique et le
parent disparaît. Sur la diagonale, les IDs suivent `x<y`. Ces identités
partitionnent les couples restants sans omission ni doublon ; elles interdisent
une coexistence parent--enfant dans la wavefront.

Le `CellPair` reste non ordonné jusqu'au test géométrique. Le carrier primaire
sert seulement à orienter une sweep ou une émission terminale : il ne crée pas
une seconde copie du même `CellPair`. Une émission depuis chacune des deux
faces aiguës duplique un q4, tandis qu'une jointure exigeant deux faces aiguës
perd les q4 qui n'en ont qu'une. `SupportKey` est décidé avant tout RLE par
`BallKey` ; deux supports distincts partageant une sphère restent deux
provenances.

Même avec `C!=D`, les facteurs peuvent recouper les ensembles d'IDs `A` ou
`B`. Les masses et preflights retirent donc explicitement les diagonales
`A/C`, `A/D`, `B/C`, `B/D`, en plus de `C=D`. Le test terminal de quatre IDs
distincts protège la sûreté, mais ne rendrait pas seul les compteurs exacts.

Un tétraèdre q4 positif possède au moins une face aiguë adjacente à son arête
maximale. `WST4` peut donc choisir le plus petit `PointId` des carriers aigus
comme carrier primaire et l'autre comme apex. Cette relation q3 est
**géométrique et pré-rang**. Elle ne vient jamais des événements q3 retenus.
Changer seulement l'owner ne réduit ni les vrais supports ni les 4-ensembles à
attribuer : il repartit une relation exact-once. Une autre broad phase pourrait
avoir un surensemble différent, mais exige une nouvelle preuve de couverture.
L'arête maximale fournit immédiatement un diamètre `D` qui majore les cinq
autres distances ; le carré propre d'une arête plus courte ne le ferait pas.
Une `BallKey` n'existe qu'après résolution du support et serait donc un owner de
génération circulaire.

La jointure exige **au moins un** carrier aigu, jamais deux. La fixture u16
`p0=(8,2,12)`, `p1=(1,3,9)`, `p2=(4,0,0)`, `p3=(10,5,1)` a pour owner unique
`p0p2`; la face de carrier `p1` est non aiguë, celle de `p3` est aiguë, et les
poids q4 valent
`(1459/3750,977/11250,3613/11250,761/3750)`, tous positifs. Une route par
couples non ordonnés teste donc `Acute(x) OR Acute(y)`. Une route orientée prend
le plus petit `PointId` parmi les carriers aigus comme primaire et laisse
l'autre sommet arbitraire. Exiger deux faces aiguës perd la fixture ; émettre
depuis chaque face aiguë duplique les cas qui en ont deux.

La fixture de tie
`p0=(0,0,0)`, `p1=(0,1,1)`, `p2=(1,0,1)`, `p3=(1,1,0)`
a ses six distances au carré égales à deux, son centre en
`(1/2,1/2,1/2)` et quatre poids `1/4`. Avec les IDs `0<1<2<3`, l'owner q4
est exactement `EdgeKey(0,1)`, le `CellPair` porte `{2,3}` et le primary de la
sweep vaut `2`. Elle tue les owners non déterministes, l'émission depuis les
deux carriers et la coexistence d'un parent avec ses enfants.

Le preflight `M4_apex` n'a pas besoin de développer carrier × apex. Pour une
arête owner `e`, retirer les sites collinéaires, définir `V_e` par les quatre
arêtes endpoint qui ne battent pas `e`, `A_e` par l'acuité stricte et
`N_e=V_e minus A_e`. Si `E_e(S)` compte les paires de `S` dont l'arête ne bat
pas `e`, alors `E_e(V_e)-E_e(N_e)` compte exactement les paires incidentes à
au moins un carrier aigu. Grouper ensuite par la direction projective primitive
`PlaneKey_e(z)=(b-a) cross (z-a)` et soustraire le même count dans chaque
classe retire exactement l'orientation nulle :

```text
M4_e = E_e(V_e)-E_e(N_e)
       - sum_pi (E_e(V_e,pi)-E_e(N_e,pi))
```

Chaque `E_e` est un range-count dual-tree/cell-pair `ALL/NONE/MIXED`, avec
tie-break `EdgeKey` exact. L'identité donne un count et des bornes avant fill,
pas les barycentriques, le rang ou les BallKeys.

La construction ne forme pas aveuglément tous les couples `(C,D)`. Elle
classe d'abord `CarrierBlock(A,B,C)` en `ALL_ACUTE/NONE_ACUTE/MIXED`, élimine
`NONE_ACUTE`, puis associe seulement les carriers aigus possibles aux cellules
d'apex. La famille u16 à deux droites doit ainsi produire zéro face aiguë et
zéro bloc q4 malgré `n^2/4` paires non fermées par les certificats universels.

Pour une face exacte `(a,b,x)`, poser `d=b-a`, `u=x-a`,
`D=d dot d`, `E=u dot u`, `F=d dot u`, `G=DE-F^2`,
`n=d cross u` et `W=E(D-F)d+D(E-F)u`. Les centres compatibles forment la
droite `c(tau)=a+(W+tau*n)/(2G)`. Pour `s=z-a`, la puissance mise à l'échelle
est `G||s||^2-W dot s-tau*n dot s`. Chaque apex non coplanaire donne donc un
événement rationnel sur une sweep 1D, plutôt qu'une intersection nouvelle de
deux lignes dans un arrangement 2D. Les valeurs égales sont groupées avant le
census ; leurs comparaisons peuvent demander environ 155 bits sous u16. Si
`n dot s=0`, la puissance est constante : négative pour un intérieur permanent,
nulle pour un shell permanent, positive pour un extérieur permanent. Ces sites
ne sont jamais jetés ; les IDs du support sont masqués séparément.

La positivité q4 se teste aussi sans solveur générique. Pour l'apex `y`, poser
`s=y-a`, `A_y=G||s||^2-W dot s` et `B_y=n dot s`. Son poids barycentrique vaut
exactement `lambda_y=A_y/(2B_y^2)`. Sous la face owner aiguë, les quatre poids
sont strictement positifs si et seulement si `B_y!=0`, `0<A_y<2B_y^2` et :

```text
F*X*B_y^2 > A_y*(G+(F-E)*(d dot s)+(F-D)*(u dot s))
E*(D-F)*B_y^2 > A_y*(E*(d dot s)-F*(u dot s))
D*(E-F)*B_y^2 > A_y*(D*(u dot s)-F*(d dot s))
```

Les trois lignes sont les poids de `a`, `b` et `x`. Elles viennent des
barycentriques du pied q3
`(F*X,E*(D-F),D*(E-F))/(2G)` auxquels on soustrait
`lambda_y` fois la projection barycentrique de `y`. Elles donnent un
`ApexWellCenteredBlock` `ALL/NONE/MIXED` par bornes corrélées avant toute
Cramer 4×4. Sous u16, les produits de comparaison peuvent atteindre environ
174 bits : employer trois limbs/BigInt, pas i128. Les égalités, `B_y=0`,
`A_y=0` et `A_y=2B_y^2` sont des fixtures obligatoires.

Avant de matérialiser les apex, exploiter la même droite comme certificateur de
profondeur. Son intersection avec `K_4(ab)` est un segment fermé `J_f`. Avec
`c_0=a+W/(2G)`, `m=(a+b)/2` et `h=(b-a)/2`, il s'écrit
`J_f={tau:tau^2<=T_f}`, où
`T_f=4G*(||h||^2/2-||c_0-m||^2)`. Pour chaque témoin, poser
`A_z=G||z-a||^2-W dot (z-a)` et
`B_z=n dot (z-a)` ; l'absence d'intérieur est le demi-intervalle
`A_z-tau*B_z>=0`. Un groupe couvre toute la face si :

```text
J_f intersect intersection_z {tau : A_z-tau*B_z>=0} = empty
```

Helly avec tolérance en dimension un donne ici un reçu constructif, plus simple
que le DAG leave-out. Classer d'abord les témoins qui sont intérieurs sur tout
`J_f` ; en conserver `p=min(h,n_permanents)`. Chaque autre témoin utile est une
demi-droite ouverte `tau<alpha` ou `tau>beta`. Poser `k=h-p`, garder les `k`
plus grands `alpha` et les `k` plus petits `beta`, avec leurs `PointId`.

Pour tout `tau`, le nombre de demi-droites gauches retenues vaut
`min(k,n_gauches(tau))`, et de même à droite. Si la famille complète a
profondeur au moins `h`, leur somme vaut donc au moins `k` partout ; la famille
retenue a la même profondeur minimale. La réciproque est immédiate puisqu'il
s'agit d'un sous-pool. Ainsi un `AxisToleranceKernel-h` exact contient au plus
`p+2k=2h-p` IDs : **16 pour q4**, **18 pour q3**, et souvent moins.

Un seul scan top-k suivi du replay des deux bouts de `J_f` et des seuils groupés
décide donc exactement la profondeur d'une face fixe en `O(n)` avec `O(h)`
mémoire. L'égalité est shell et ne crédite rien. Les bouts de `J_f` étant
généralement irrationnels, les comparaisons emploient signe puis carré exact ;
`B_z=0` conserve les trois cas constant intérieur/shell/extérieur. Cette forme
est la spécialisation constructive de `eta(2,h)=2h`.

Un `FaceAxisJungDepth8Block` propose ce petit noyau sur un représentant, puis
vérifie orientations, signes, ordre des seuils et marges sur tout `A×B×C`.
`ALL` s'hérite ; toute inversion d'ordre, égalité ou borne indécise scinde
fail-open. Un LBVH/range-extrema peut proposer directement les top-k sans
matérialiser les faces ou les apex.

Après ajout d'une cellule apex, `BlockBallDepth8(A,B,C,D)` restreint encore la
famille de centres. Si `B_y` change de signe sur `D`, son image en `tau` peut
être non bornée ou disjointe : reprendre tout `J_f` ou rendre `MIXED`. Sinon un
intervalle conservateur est vérifié. Le déterminant in-sphere ponctuel tient
dans `i128` sous u16 ; son produit par l'orientation peut dépasser 128 bits, donc
les deux signes sont classés séparément. Une enveloppe de bloc ne traite jamais
une AABB levée comme le convexe de ses seuls coins : elle borne `||p||^2`
séparément ou utilise Bernstein/SOS exact.

Une première enveloppe entière exploitable choisit une origine locale `o` et
un support représentant, puis écrit sa BallForm témoin
`P0(r)=A0||r||^2+B0 dot r+C0`, avec `r=z-o`. Si `dA,dB_i,dC` bornent les
variations de coefficients sur `A×B×C×D`, alors sur un witness node `Z` :

```text
E = maxabs(dA)*Qmax + sum_i maxabs(dB_i)*maxabs(r_i) + maxabs(dC)
J(block,Z) subset [min_Z P0-E, max_Z P0+E]
```

Les extrema de `P0` se calculent axe par axe aux endpoints et aux entiers
`floor/ceil(-B0_i/(2*A0))` clipés. Un signe strict opposé à l'orientation sur
tout le produit crédite la population du nœud ; huit IDs témoins disjoints des
quatre facteurs ferment q4. L'égalité reste `MIXED/shell`. Les coins seuls sont
insuffisants pour la forme quadratique.

La hiérarchie q4 devient donc : Jung edge 2D, porte aiguë, Jung axe 1D,
`OwnedCK-WST4` **broad-phase symbolique**, puis `BlockBallDepth8` et les
barycentriques sur ce produit carrier--apex. Seul le résiduel est émis vers la
sweep ou le fill. `BlockBallDepth8` peut précéder la positivité lorsqu'il est
moins cher, mais il ne précède pas l'existence du bloc WST4 qui quantifie
`A×B×C×D`. Cette ordonnance attaque `C4_carrier/M4_apex` avant toute expansion ;
une borne linéaire sur le nombre de blocs ne suffirait pas sans ce consommateur
factorisé.

L'autorité q4 reste directe : quatre IDs distincts, orientation non nulle,
owner parmi six arêtes et quatre numérateurs barycentriques du circumcentre
strictement positifs. « Quatre faces aiguës » n'est ni nécessaire ni suffisant.
Après `JungDiskDepth8`, seules les paires singleton ou microtiles effectivement
fermés évitent leurs couples de cellules. Un rectangle CK ne les évite que si
un futur classifieur uniforme le prouve. Le `count` et le preflight précèdent
impérativement le `fill`.

Si la masse de couples carrier reste rouge, le moteur local du plan médiateur
reste le successeur : q2 interroge son point central, q3 le pied unique d'une
ligne et q4 les intersections shallow `P-P/N-N/P-N`. Il ne construit aucun
arrangement global et ne s'instancie qu'après les portes `F4/M4_apex`, tâches et
HWM. Le code exhaustif actuel reste son oracle, pas son modèle GPU.

La gate porte sur les objets physiques `F4`, blocs aigus, `N4_event`,
`Z4_const`, `R4_bundle`, `T4_site`, octets et HWM, pas sur la seule masse
sémantique `E4`. Une masse
quadratique peut rester factorisée ; inversement, un scan de tous les sites par
face est rouge même si peu de supports survivent.

Le pin `4515a8b` observe une quadrature `arête × porteur` de pente proche de
`2,97` sur `eight_clusters`. Elle réfute l'expansion ponctuelle, pas WST4 : un
bloc `ALL_ACUTE` porte sa masse par un seul record, `NONE_ACUTE` disparaît et
seuls les `MIXED` se divisent. L'ordre physique garde donc le carrier
symbolique, forme WST4, applique profondeur et barycentriques au niveau bloc
avant tout fill, puis ne
matérialise une face que si le résiduel justifie la sweep.

Le sampler v2 du HEAD ne fournit pas encore une estimation reçue : sa borne
Hoeffding est correcte sous i.i.d. uniforme, mais le mapping multiply-high reste
sans rejet, les streams n'ont pas de contrat d'indépendance, `W4` n'a pas
d'intervalle et le contrôle ne juge pas le décodeur rang--`PairId` dans un mode
exhaustif déterministe. La vue SOC reste absente.
Son option `--rang` peut réussir sans lancer le sampler, ignore les extra-shells
et conditionne le tirage sans les poids nécessaires. Le brute-force q4 reçoit
une énumération bornée, mais recopie les prédicats du sujet et appelle à tort
`H4` le seul test `I<=7`. Aucun de ces chiffres ne justifie une rampe G4 50k.

Le mur avant rang est néanmoins réel sur une famille explicite. Pour
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)`, `y=(31,19,31)`, `ab` est
owner unique, les deux faces adjacentes sont aiguës et les poids q4 valent
`(47,3,55,55)/160`. Ces propriétés persistent sur quatre petits sous-cubes :
la masse `W4_positive` y est quartique. Mais les huit points
`(20+i,20+j,30+k)`, `i,j,k` dans `{0,1}`, sont tous strictement intérieurs ;
par continuité, un cinquième sous-cube ferme uniformément ce produit à
`smax=11`. Cette fixture exige que la profondeur de bloc agisse avant le fill,
et tue toute route qui compte sur les seules barycentriques pour réduire
l'exposant.

## 8. Plateaux, fold et sortie

Le profil u16 n'exclut pas les cosphères. La politique candidate régulière est
fail-closed : un extra-shell **pertinent pour une lane admise** retourne
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture de `RelevantGP` reste elle-même à prouver.

Un `SphereRun` interne lossless conserve `BallKey`, `I_B/U_B`, supports ou
handle de provenance et disposition. Il ne rend pas un plateau publiable. Un
quotient saturé H0 doit encore recevoir joins par intersection, lots gelés,
coverage, dix forêts et verticales contre Gamma exhaustif. Si le contrat exige
chaque `SupportKey`, ni quotient non reçu, ni streaming ne supprime la borne de
sortie.

Le fold contractuel consomme des références exactes opaques, pas les
coordonnées ni les limbs natifs. Le probe `91aa287` ne respecte pas encore ce
contrat : il lit directement `PrimitiveSphereKey`. Dix millions de points
n'imposent pas binary64 : la grille u16 3D contient `2^48` sites. Le codec
d'index dense peut devenir u32 séparément. Un futur profil binary64 exact est
une phase distincte derrière `ExactKernel` et une sérialisation BigInt/dyadique
variable.

## 9. Ordre de réalisation vers le G4

1. Fermer 0A sur tout u16 : largeurs, vrais `PointId`, juge indépendant,
   `BallKey`, RLE avant census, lanes et statuts transactionnels.
2. Raccorder les événements aux générateurs/facettes Gamma et fermer 0B sur
   petit `n` jusqu'au payload complet, sous permutations, tilings, caps et
   reprises.
3. Recevoir `CKPairTape` comme partition q2 exacte et son certificateur
   `[L_open,U_closed]`, puis mesurer `F2` et la masse logique séparément.
4. Ajouter `JungDiskDepth9/8` au niveau paire/microtile, puis
   `BlockJungDualTile` avec preuve uniforme ou split,
   `OriginOnionDepth` et un shadow `SOC64` à union de preuves disjointe, capé et
   jugé, avant toute multiplication carrier ; ne promouvoir Jung au rectangle
   qu'après un théorème uniforme.
5. Construire `OwnedCK-WST3` counter-only dans la fenêtre `2B_R`--lentille,
   recevoir couverture/owner/acuité,
   puis raccorder `BallKey -> Q3FootPowerRange` et le census.
6. Construire `OwnedCK-WST4` depuis la relation aiguë géométrique pré-rang,
   éliminer les blocs sans carrier, recevoir le count factorisé `M4_e`, puis
   appliquer `FaceAxisJungDepth8Block` et `BlockBallDepth8` avant fill. Tester la
   sweep 1D seulement après preflight ; recertifier directement les quatre
   barycentriques, le rang fermé `I+U` et la fixture où aucun sous-événement
   q2/q3 n'est retenu.
7. Employer LP/cages comme diagnostics du même résiduel ; appliquer le
   raffinement porteur de preuves aux tâches encore `MIXED`, puis regater
   `F3/C4_carrier/F4/M4_apex/T4_site` et choisir le shallow local q4 seulement
   si nécessaire.
8. Porter la tranche entière par count--scan--fill, tâches persistantes, SoA,
   radix/RLE et arithmétique large reçue.
9. Exécuter des microgates `1500/3000/6000`, puis
   `12500/25000/50000` uniquement si tâches, octets, HWM et sorties passent.
10. Qualifier trente répétitions chaudes à `50000` avec le payload officiel.

La session CPU sur VM G4 du pin `3c11bc8`, publiée au `HEAD=35fcea8`, a terminé
ses quarante processus et certifié l'arrêt ciblé. Elle ne ferme toutefois pas
la porte : quatre tailles restent quatre processus et les pentes sont calculées
après coup ; `fenetre_finale` n'est ni conservé ni gaté ; `terrain` contient
effectivement des continuations q3/q4 ; les temps et recertifications sont
supprimés du log ; et le chemin d'échec du script n'est pas reçu. Réparer ces
points, exiger tous les codes zéro **et** `pending=0`, puis archiver aussi les
échecs. Cette campagne reste CPU et ne reçoit aucun débit GPU.

Le résultat borné est néanmoins décisionnel : la configuration
`Central-VWave + s=8 + window=512 + raffinement<=4` garde des pentes `sum_E4`
proches de `1,9` sur `eight_clusters` et doit être remplacée sur cette famille.
Cela ne réfute ni `SOC64/CORNER512`, ni LP/PWC, ni les cages. Sur `uniform`, la
pente E4 seule reste verte mais ne borne pas `M4_apex` ni le payload. Le SLO officiel
ne peut pas être qualifié sur `uniform` seule : le plan évalue les objectifs sur
Poisson uniforme **et** le mélange équilibré de huit amas, et G6 exige les deux
familles favorables.

La porte composée publie au minimum :

```text
E3/E4, max par ancre, CLOSED/OPEN/PENDING
F2/F3/F4 physiques et masses logiques factorisées
C4_carrier, M4_apex, W4_positive, H4_rank
N4_event, Z4_const, R4_bundle, T4_site, tâches, splits, visites
BallKeys brutes/uniques, supports, census, shells
sortie H, opérations larges, octets/HWM
temps par phase et warm_e2e
commandes, seeds, HEAD, diff, hashes, codes de sortie
```

Une pente seule, un taux de fermeture, un `OK` CPU ou une extrapolation de
bande passante ne qualifie aucun SLO.

## 10. Fixtures permanentes prioritaires

- owner équilatéral/isocèle sous permutation indépendante stockage/PointId ;
- q3 aigu, droit, obtus, `G=0`, `P=-1/0/+1`, seuil huit/neuf ;
- triangle et tétraèdre u16 maximaux, clés et barycentriques attendus ;
- deux supports pour une `BallKey`, un census, deux `SupportRecord` ;
- juge BigInt sur triangle u16 maximal et générateur capacité plus un ;
- niveau exact sous changement d'échelle, fractions voisines, transitivité et
  overflow sous UBSan ; une faute numérique sous mutant garde son statut ;
- cosphère 384 sans troncature et extra-shell pertinent fail-closed ;
- partition CK exacte-once, carrier dans la fenêtre `2B_R`--lentille,
  diagonales `A/C,A/D,B/C,B/D` et `C=D` sans ID répété ;
- égalité sharp `2B_R`, dual Jung q3/q4 à égalité shell, poids `lambda`
  pairwise sans poids commun au rectangle et mutant `corners-only` ;
- count factorisé `M4_e` contre expansion : tie `EdgeKey`, site collinéaire,
  deux carriers aigus, même `PlaneKey` et plans distincts ;
- Jung axe : événements opposés égaux, trois constantes `B=0`, matching de
  taille deux détruit par un choix glouton ;
- quatre sous-cubes q4 positifs et huit témoins uniformes du cinquième pour
  recevoir `BlockBallDepth8` avant fill ;
- cosphère centre `(4,4,4)`, rayon carré 25, shell de 24 permutations de
  `(±3,±4,0)` : `I=0,U=24`, donc jamais q4 régulier
  retenu à `smax=11` ; mutant `drop-extra-shell` ;
- site de sweep à dénominateur nul avec puissance constante négative, nulle ou
  positive, plus mutant `drop-zero-denominator` ;
- tétraèdre entier aux six arêtes égales pour tuer la maximalité faible sans
  tie-break `EdgeKey` ;
- q4 positif avec aucun de ses six q2 ni quatre q3 retenu à `smax=11` ;
- q4 positif à deux faces obtuses et quatre faces aiguës avec q4 négatif ;
- partenaires q3 et q4 au-delà d'un cutoff de rang 128 ;
- dix témoins q2 dans la boule diamétrale qui ne ferment pas q4 ;
- `SOC64` succès axial et faux-échec du produit relaxé ;
- retour SOC sous `floor=q4` différent de la vraie lane minimale ;
- shadow `SOC-ALL(parent)` puis `central-ALL(child)`, union et somme distinctes ;
- `JungDiskDepth8` positif alors que le LP global échoue à profondeur un ;
- `OriginOnionDepth` à huit/neuf/dix couches, égalité, chute de rang et ID
  dupliqué ;
- deux droites u16 : résiduel universel `n^2/4`, zéro carrier aigu et zéro
  sweep sans allocation par paire ;
- q4 bien centré avec arête non maximale choisie comme faux owner, pour mordre
  un usage trop large du lemme aigu ;
- `CORNER512` coin omis, égalité, axe dégénéré et largeur 70 bits ;
- LP rang un/deux/trois, égalité `kappa=D`, pool capé et IDs supprimés ;
- cage octaédrique six-sites, tétra-only, reuse d'ID et rayon mal arrondi ;
- fold `k=2` avec générateurs partageant un point mais moins de deux IDs ;
- raffinement profondeur zéro à quatre, héritage de preuves et zéro double
  comptage terminal ;
- analyseur de rampe : taille manquante, code non nul, pending positif et pente
  exactement au seuil ;
- caps exacts puis moins un, continuation, permutation, tuilage et reprise ;
- comparaison lot par lot des dix forêts, coverage et verticales.

Deux fixtures q4 exactes empêchent de réintroduire un cutoff de rang ou de
confondre q2 et q4. Pour la première :

```text
a=(100,100,100), b=(200,100,100)
x=(150,30,120), y=(150,30,80)
c=(150,80,100), R2=2900
z_i=(150+i,140,100), i=-4,...,5
```

`ab` est l'unique arête maximale du tétraèdre, `c` est strictement intérieur
avec poids `(5/14,5/14,1/7,1/7)`, et les quatre sommets sont cosphériques.
Chaque `z_i` est strictement dans la boule diamétrale de `ab`, mais strictement
hors de cette sphère. Dix crédits q2 ne ferment donc pas le support q4 vide.

Pour la seconde :

```text
c=(30000,30000,30000)
a=(5000,40000,30000), b=(55000,40000,30000)
x=(30000,5000,40000), y=(30000,5000,20000)
z_j=(5000,40000+j,30000), j=1,...,4381
```

Les quatre sommets ont `R2=725000000`, le même centre intérieur et les mêmes
poids strictement positifs ; `ab` est leur unique arête maximale. Tous les
`z_j` sont plus proches de `a` que `b`, mais strictement hors de la sphère.
Le support q4 vide subsiste donc avec `b` au-delà du rang 4380. Un préfixe kNN
peut proposer des témoins ou une continuation, jamais fermer par son échec.

## 11. Non-claims

Il existe une voie q2/q3/q4 exacte et GPU-factorisable en blocs :
`CKPairTape -> OwnedCK-WST3 -> OwnedCK-WST4 -> BallKey/RLE -> profondeur
batchée`. « Sparse » qualifie seulement les enregistrements physiques restant
factorisés. Il n'existe aucune garantie universelle sous-quadratique pour la
sortie explicite ou pour le raffinement des frontières `MIXED`.

`SOC64`, `CORNER512`, LP projectif et cages sont des certificateurs ou
falsificateurs précis. Aucun n'a encore démontré le contrat 50000/G4. Le
contrat reste ouvert jusqu'à production du même `BenchmarkOutputContract-v1`
complet dans le p95 déclaré.

GCP non utilisé par l'auditeur pour cette proposition. Le reçu CPU G4 relu
certifie l'arrêt ciblé de la session qu'il documente.

La preuve de couverture, les owners, les fixtures et le contre-audit live sont
dans
[`audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).
Le contre-audit SOC live, la contre-famille universelle, la sweep par porteur
aigu et le certificat par pelages sont dans
[`audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md).
La première réponse aux six questions de Claude, la nomenclature
`C4_carrier/M4_apex`, la preuve sharp de `2B_R` et le contre-audit des
échantillonneurs v0/v1 sont dans
[`audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit du sampler v2 et du brute-force, la réponse à la question 7,
la forme entière de `BlockBallDepth8` et le fallback shallow sont dans
[`audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).
