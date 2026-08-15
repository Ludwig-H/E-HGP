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

- structure de Delaunay de quelque ordre que ce soit, y compris le graphe ou
  la triangulation d'ordre un ;
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

Le probe actuellement nommé stage 0B ne ferme pas 0B. Il trie les runs
réguliers, unionne les membres `I_B union U_B` dans une seule DSU de `PointId`,
puis compare ses goulots à Floyd--Warshall sur les mêmes runs, membres,
dispositions et niveaux.

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
`D=||b-a||^2>0`. Le tape filtre exactement cette paire endpoint dégénérée. Une
implémentation peut bucketiser les positions dupliquées pour la géométrie, mais
elle conserve tous les vrais `PointId` et leur multiplicité dans les pools
témoins et les produits. Les paires de chacun de ces IDs vers une troisième
position gardent leur multiplicité ; un quotient silencieux changerait la
profondeur.
Sous cette porte, il est la source q2 complète ; ce n'est pas un simple
proposer.

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

`MidballInterval-u16` doit être une autorité unique partagée, non un doublon de
`rect_h_interval`. Son wrapper vérifie `0<=lo<=hi<=65535`, paire propre et
identités ; toute violation rend `INVALID/UNKNOWN`. Le minimum de `H` est exact
sur l'AABB continue comme sur le réseau. Le maximum actuellement utile au
profil quantifié est exact sur `integer_lattice_u16_aabb_envelope` et son verdict
doit donc être nommé `NONE_LATTICE_U16`, jamais `NONE_CONTINUOUS`. Si un
consommateur exige le continu, pour chaque paire d'extrémités `a,b` d'un axe il
calcule `s=a+b`, `y=clip(s,[2*zl,2*zh])`, puis
`max4=(b-a)^2-(y-s)^2`; la somme des trois maxima d'échelle quatre décide sans
flottant. L'égalité exclut l'intérieur mais reste shell : elle ne retire jamais
un span du census fermé.

Dans le certificateur central, Midball entre d'abord comme disjonctif
**ALL-only**. Le fast path calcule uniquement les trois minima, soit 24 produits
i64 par bloc, et ne remplace jamais le `NONE` d'un autre certificateur par son
propre `MIXED`. Le maximum n'est payé que par une voie qui consomme réellement
`NONE_LATTICE_U16`. Appels, gains nouveaux, lectures évitées, temps, `pending`
et coûts des deux vues sont publiés séparément. Le statut logiciel live et les
fixtures bloquantes restent autoritaires dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Le tape physique peut être linéaire alors que `sum |A||B|` est quadratique.
Un bloc accepté reste donc paresseux jusqu'à un consommateur factorisé reçu ou
jusqu'au preflight d'une expansion atomique. La WSPD ne rend égaux ni niveaux,
ni `BallKey`, ni census.

### 4.3 Miniboule unique et plan médiateur fini

Pour tout support minimal positif affinement indépendant `S`, le centre
intrinsèque dans `aff(S)` et la miniboule sont uniques. L'ensemble des supports
complets du nuage est fini ; en revanche, un prune portant sur **toutes les
complétions** d'une ancre partielle garde légitimement un domaine continu de
centres. Ce domaine continu n'est jamais une source d'événements HGP et ne doit
être ni énuméré ni matérialisé : il sert seulement à un certificat facultatif
qui évite de générer certaines complétions. La source normative est finie :

```text
q2 : paire distincte -> boule diamétrale unique
q3 : triangle strictement aigu -> centre/rayon intrinsèques, miniboule ambiante canonique unique
q4 : tétraèdre affinement indépendant bien centré -> circumsphère unique
support non positif -> rejet de ce SupportKey ; sa miniboule est d'arité inférieure
```

Cette boule unique décide uniquement l'événement dont `S` est le support
minimal. Le census de la boule q2 ne décide pas les complétions q3/q4 de la
paire, et celui de la boule q3 ne décide pas ses complétions q4 : leurs
supports, centres et boules canoniques sont différents.

La preuve tient dans l'identité barycentrique suivante. Si
`o=sum(lambda_i*p_i)`, tous les `lambda_i>0`, leur somme vaut un et
`||p_i-o||=R`, alors pour tout centre `y` :

```text
sum_i lambda_i*||p_i-y||^2 = R^2+||o-y||^2
```

Toute boule centrée en `y` qui contient `S` a donc un rayon carré au moins
`R^2+||o-y||^2`, avec égalité minimale uniquement pour `y=o`. Un troisième ou
quatrième site sur le shell ne devient pas automatiquement membre du support :
une barycentrique nulle ramène la miniboule du tuple à l'arité inférieure. Cela
ne signifie pas que cette source inférieure porte la même circumsphère ; une
autre base positive du shell peut produire sa `BallKey`.

La même preuve donne un microkernel entier unique pour q2, q3 et q4. Écrire le
support sous la forme `S={a,a+m_1,...,a+m_k}`, avec `k=1,2,3`, puis poser :

```text
M = [m_1 ... m_k]
G = M^T*M
ell = diag(G)
Delta = det(G)
r = adj(G)*ell
t = M*r
```

`Delta>0` est exactement l'indépendance affine. Pour `k=1`, cela impose des
coordonnées distinctes, donc `D>0` ; deux `PointId` au même site conservent leur
multiplicité dans le census mais ne forment pas un support q2. Prendre
explicitement `adj([D])=[1]`. Les poids du circumcentre sont alors, sans solveur
ni division dans la décision :

```text
lambda_{i+1} = r_i/(2*Delta)
lambda_a = (2*Delta-sum_i r_i)/(2*Delta)
```

Le support est minimal positif si et seulement si ces `k+1` numérateurs sont
strictement positifs. Pour `s=z-a`, définir :

```text
Phi_S(z) = Delta*||s||^2 - s^T*t
```

On a exactement `Phi<0` à l'intérieur, `Phi=0` sur le shell et `Phi>0` à
l'extérieur. Pour `k=1`, cette identité redonne
`||z-a||^2-(z-a) dot (b-a)<0`, donc la boule diamétrale. Pour `k=2`, elle
redonne la boule q3 ambiante. Pour `k=3`, `Delta=O^2` et `Phi=O*J` : aucune
orientation n'est nécessaire.

La forme primitive destinée au RLE est obtenue directement, toujours sans
centre :

```text
BallForm = (Delta, -2*Delta*a-t, Delta*||a||^2+a dot t)
```

Réduire ses coefficients par leur pgcd et imposer le premier coefficient
positif donne la clé géométrique avant ajout du profil/epoch. Au support q4
ponctuel, la division algébrique par `|O|` redonne la forme `T/Qbar` plus courte
décrite plus bas et tient en i128. L'implémentation terminale calcule directement
`T/Qbar` puis le pgcd ; former d'abord la grande BallForm de Gram annulerait ce
bénéfice. Pour `k=1`, la forme primitive se réduit directement à
`(1,-a-b,a dot b)`.

Sur un produit de boîtes dont le signe varie, la valeur finale vérifie
`|Phi|<432*65535^8<2^137`, mais une enclosure Gram naïve peut former des
intermédiaires jusqu'à `1944*65535^8<2^139`. Il faut donc jusqu'à 140 bits
signés d'intermédiaire ; i192 est sûr, i256 reste une autorité simple. Les huit
coins éliminent uniquement la variable témoin : pour fermer un `WitnessNode`,
prouver `sup_support Phi_S(q)<0` à chacun de ses huit coins, uniformément sur
`A×B×C×D`, après `Delta>0` et positivité stricte des poids. La convexité en `z`
rend ces huit tests nécessaires et suffisants pour chaque support fixé ; elle
ne rend pas exacte l'enclosure des facteurs support. Une borne indécise splitte
ou rend `PENDING`, et huit coins extérieurs ne prouvent jamais `NONE`. Une route
G4 candidate garde `O/J` séparés en i128 lorsque le signe est uniforme, réserve
un entier élargi au résiduel réellement indécis et ne paie le pgcd que pour les
survivants émis. Trois limbs i192 suffisent arithmétiquement, mais il faut
mesurer fraction résiduelle, registres, spills et occupation face à i256 avant
de conclure sur le coût.

Le contrôle algébrique Python corrélé qui falsifie `Delta=O^2` et `Phi=O*J` sur
10 000 q4 u16 non dégénérés, sans appeler le microkernel C++ ni recevoir les
enclosures ou la performance, est conservé
dans [`audits/AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md`](audits/AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md).

Chaque support positif produit donc une seule `BallKey` candidate, mais aucun
census n'est payé avant le RLE. Les coïncidences cosphériques peuvent envoyer plusieurs
`SupportKey` vers la même `BallKey` ; le RLE conserve toute cette provenance et
paie exactement un census par `BallKey` unique. Un support non positif est
simplement rejeté de cette source ; la complétude vient de Carathéodory : si le
centre `c` d'une boule appartient à l'enveloppe convexe de son shell `U_B`, il
existe dans `U_B` une base affinement indépendante de taille au plus quatre
dont `c` est une combinaison strictement positive.

Cette source positive est une supersource exhaustive de miniboules candidates ;
elle ne reçoit pas à elle seule un événement Morse dégénéré.
Pour une boule `B` de rayon strictement positif, définir :

```text
P(B) = {S subset U_B : S affinement indépendant, 2<=|S|<=4,
        c appartient à relint(conv(S))}
```

Alors `P(B)!=empty` si et seulement si `c appartient à conv(U_B)`. Cela prouve
que `B` est une miniboule saturée engendrée par au moins une base positive ; ce
n'est pas le critère Morse régulier normatif. Sous les hypothèses de
Reani--Bobrowski, celui-ci exige encore
`c appartient à relint(conv(U_B))`. Ainsi `BallKey/RLE -> census U_B ->
disposition` reste obligatoire pour le rang fermé, la provenance et la
criticité. Sous `RelevantGP`, une lane pertinente
`|I_B|+|S|<=smax` vérifie `U_B=S`, donc la positivité de `S` suffit. Hors de ce
domaine, une base positive sur une face de `conv(U_B)` n'autorise ni rejet
silencieux ni publication : tant qu'une politique dégénérée indépendante n'est
pas reçue, elle donne `unsupported_degeneracy/plateau_pending`. Une même
`BallKey` peut conserver des supports positifs d'arités différentes.

La conséquence industrielle directe est un unique backend de census q2/q3/q4.
Pour une `BallForm=(A,B,C)` fixée avec `A>0`, poser :

```text
P(z) = A*||z||^2+B dot z+C = C+sum_j (A*z_j^2+B_j*z_j)
```

Sur une AABB entière, chaque minimum axial est atteint à l'un des deux entiers
voisins de `-B_j/(2A)`, clipés dans l'intervalle, et chaque maximum à une
extrémité. Les extrema sont donc exacts sans centre ni flottant. `max P<0`
crédite toute la population du nœud ; `min P>=0` exclut son intérieur strict ;
seul `min P>0` exclut aussi le shell. Chaque lane-support ferme au seuil
`h_q=smax-q+1`, soit `10/9/8` pour q2/q3/q4 sous `smax=11`. Sur une `BallKey`
partagée entre arités, `active_arity_mask` ne contient que les arités incidentes
et efface le bit `q` dès que `I_B>=h_q`. Un masque nul autorise l'arrêt pour la
fenêtre demandée ; sinon le parcours termine le census `I_B/U_B` et la
disposition des bits survivants. Saturer globalement à huit perdrait un q2 ou
q3 encore pertinent. Ce masque est distinct du `active_sign_mask` du
certificateur Corner8 bisigne et n'en répare pas le caller. Ce
`BallFormRange-u16` remplace les
backends de census dupliqués après RLE ; il ne réduit pas le nombre de supports
candidats et ne transforme donc pas à lui seul une source quadratique en route
50k.

Une source shallow peut transporter un reçu de census déjà complet sans rendre
la `BallKey` circulaire : la clé reste calculée uniquement depuis le support,
puis le RLE attache et valide le reçu. En particulier, `Q4SeedAxisTopR4` reconstruit
exactement `I_B/U_B` à partir de ses racines extrémales et de son equality
report. `BallFormRange-u16` reste l'autorité/fallback, mais un second parcours
global q4 serait du travail redondant lorsque ces IDs et leur complétude sont
reçus. Le défaut du pin `3507b5e` — un shell attendu de 100 IDs tronqué à 99 —
est réparé au `33766f6` par une capacité de 163, les comptes requis et des fates
typés. `a369452` refuse ensuite toute sélection autre que `OUVERT` et tout apex
dont le compte intérieur retenu atteint `r4`. Aucun consommateur ne peut encore
sauter le fallback tant que les entrées de `PointId` non injectives ou non
disjointes ne sont pas refusées par l'API.

Avec `d=b-a`, `D=d dot d`, `w=2*c-a-b` et `U_z=2*z-a-b`, poser :

```text
F_z(w) = D - ||U_z||^2 + 2*U_z dot w, avec w dot d = 0
```

Le signe de `F_z` donne exactement intérieur, shell ou extérieur. Avec la
convention `Pow<0` pour l'intérieur et `t=c-(a+b)/2`, on a
`F_z(2t)=-4*Pow_z(t)` ; cette parité de signe doit être testée. q2 interroge
seulement `w=0`, la boule de diamètre `ab`. q3 prend le point de norme minimale
sur la ligne `F_x=0`, c'est-à-dire le pied auto-centré du troisième site. q4
prend l'intersection de `F_x=0` et `F_y=0`. Positivité, owner, shell et
`BallKey` restent des recertifications séparées.

Pour q3, le centre est intrinsèque au plan du triangle mais la boule et son
census sont ambiants en dimension trois. Par exemple
`a=(0,0,0),b=(4,0,0),c=(2,3,0)` donnent
`o=(2,5/6,0)`, `R^2=169/36`; le site `z=(2,1,1)`, hors du plan, a une
puissance `-11/3` et compte comme intérieur q3. Le circumdisque planaire
n'apparaît que comme cœur commun des sphères incidentes, jamais comme domaine
du census canonique.

Plus généralement, si `o` est le circumcentre intrinsèque de `S`, toute sphère
incidente a pour centre `o+w`, avec `w` orthogonal à l'espace directeur
`aff(S)-o`, et rayon carré `R^2+||w||^2`. La puissance de `z` est affine en
`w`. L'intersection de toutes ces boules ouvertes est exactement
`aff(S) intersect int(B(o,R))` : segment
ouvert pour q2, circumdisque planaire pour une face q3, boule entière pour q4.
Ce `UnboundedAffineCoreCount` est un prune sûr des complétions, mais souvent
vide en position générique.

Pour un domaine borné `K` contenant le centre canonique, noter `C` son nombre
d'intérieurs, `U_K` les singletons intérieurs pour tout centre et `D_K` la
profondeur collective minimale. Toujours :

```text
U_K <= D_K <= C
```

Ainsi `C<h` court-circuite seulement le certificateur relaxé Jung/BJD,
`U_K>=h` ferme, et `U_K<h<=C` appelle `tau(F)`, une sweep ou un split. Les seuls
circumcentres effectivement réalisables forment un sous-ensemble de `K` qui ne
contient généralement pas le centre q2 ; `C<h` ne décide donc aucune coface.

La profondeur q2 n'est pas héréditaire vers les cofaces. Deux fixtures u16
séparées partagent `a,b` et dix IDs dans la boule diamétrale : la première garde
une boule q3 ambiante positive vide, la seconde une circumsphère q4 positive
vide, dont l'owner `ab` est fixé par l'ordre `EdgeKey`. Réunir les deux nuages
créerait des extra-shell et détruirait les rangs réguliers annoncés. Une
troisième fixture ferme une face q3 au rang douze tout en gardant une
coface q4 de rang quatre. Le disque de Jung/BJD est donc un prune collectif
suffisant avant cette source finie, jamais la définition de l'événement. Après les fermetures de bloc,
le résiduel exact emploie les pieds q3 et les intersections shallow q4 `0..7`,
pas toutes les sphères passant par l'ancre. Preuve et fixtures :
[`audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md).

Cette réduction est exacte **pour une paire fixée**. Sur un rectangle CK,
`a,b,D,U_z`, le plan médiateur, les pieds et les intersections varient ensemble.
Aucun arrangement shallow commun à `A×B` n'est encore prouvé. Le raccord doit
donc soit fournir un classifieur paramétré uniforme, soit développer les
`PairId` sous un cap explicite ; l'unicité rend la source finie, jamais sparse.
Les bornes locales de niveaux `0..8/0..7` ne bornent ni la construction, ni le
census, ni la somme globale sur toutes les ancres.

Le diagnostic `--fenetre-exacte`, introduit au pin `5809bd2` puis mesuré au
commit `694920a`, décide bien q2 sur les
paires tirées. Pour q3/q4, ses singletons universels minorent seulement la
profondeur ; sa masse ouverte est un majorant, sans pieds, intersections,
owner ou census. Son flux SplitMix à seed fixe ne reçoit pas l'hypothèse
d'indépendance de Hoeffding. Il reste un diagnostic
`PairUniversalCoreSample`, pas une mesure exacte ou un reçu statistique. En
particulier, `U_K<h` ne prouve pas l'existence d'une sphère peu profonde et
n'autorise ni le nom « squelette exact », ni aucune structure de proximité
supplémentaire. Toute Delaunay est de toute façon hors contrat.

Le commit `8fd6f59` ajoute un mode exhaustif et les libellés typés. Il ne
change pas l'objet mathématique : q3/q4 comptent exactement `U<h`, toujours un
majorant. Ce mode doit rester un oracle borné : au delta observé il est cubique,
n'a pas de cap d'opérations, retourne après la première taille d'une liste et
nomme `u_moyen` la moyenne du seul préfixe de mille paires. Son retour anticipé
précède aussi les portes BJD/finales et peut rendre code zéro avec un plancher
impossible ou un mutant survivant ; toute combinaison doit être refusée ou les
gates communes doivent précéder le retour. `--fenetre-exacte/--fenetre-seed`
doivent être incompatibles avec ce mode plutôt qu'acceptées puis ignorées.

### 4.3 bis Trois énumérateurs shallow autonomes

Le contrat de génération est désormais explicite et ne fait intervenir aucune
structure de Delaunay :

```text
q2 : paire propre -> boule diamétrale -> I_B<=9
q3 : triangle strictement aigu -> miniboule ambiante unique -> I_B<=8
q4 : tétraèdre bien centré -> circumsphère unique -> I_B<=7
```

Sous `RelevantGP`, `U_B=S`. Pour une arité `q`, le maximum accepté est
`d_q=smax-q` et le premier compte rejeté est `h_q=d_q+1`. À `smax=11`, les
maxima acceptés sont `9/8/7` et les seuils de mort `10/9/8`. Ainsi
`Q3MiniballDepth9` signifie « certifier au moins neuf intérieurs et rejeter » ;
il ne signifie jamais accepter `I_B<=9`.

Le split-tree Morton et une `NeutralPairPartition` immuable peuvent être
mutualisés comme index pur. Ils alimentent ensuite trois générateurs dont les
queues, records, verdicts, caps, continuations et preuves de complétude sont
disjoints :

```text
PointStore + MortonIndex + NeutralPairPartition
  |- Lane2(Pair2)                         -> Positive2 -> B2 -> reject_at_10
  |- Lane3(PairAnchor3,Third3)            -> Positive3 -> B3 -> reject_at_9
  `- Lane4(PairAnchor4,Q4Seed3,Fourth4)   -> Positive4 -> B4 -> reject_at_8
```

`Q4Seed3` est un préfixe ternaire créé et possédé exclusivement par `Lane4`.
Ce n'est ni un support q3, ni un événement de `Lane3`, ni un record que q4
serait autorisé à y lire. Réciproquement, `PairAnchor3` n'est pas une sortie de
`Lane2`. Le partage permis porte seulement sur les coordonnées, l'index Morton,
la partition neutre et des fonctions géométriques pures sans état de lane.

Le profil courant de ces générateurs est u16 seulement. Le contrat binary64 de
la spécification garde un statut distinct : aucun arrondi ponctuel fixe,
non injectif et sans side-channel ne peut préserver universellement les signes
InSphere, le shell et l'ordre des niveaux. Une similarité-lattice exacte
enregistrée est un sous-domaine sûr ; toute autre équivalence après
quantification demande un certificat complet propre à l'entrée.

La complétude de J0 ne se déduit pas du plus grand diamètre observé sous une
coupure `--dmax`. La `NeutralPairPartition` doit conserver toute la masse des
paires ; chaque bloc non descendu porte un certificat exact propre à sa lane ou
reste une continuation. Les rayons locaux par calottes peuvent fermer certaines
incidences d'endpoint ; une paire reste au résiduel tant qu'aucun endpoint ne la
ferme.
Avant `unresolved_pair_mass=0`, owner et shell exacts, les comptes publiés sont
des ledgers tronqués sans ordre garanti et non la taille de l'objet : une ancre
omise sous-compte, tandis qu'un owner dupliqué ou un extra-shell silencieux
surcompte.

Le prototype J0 commis à `acd792d`, puis paramétré à `0195480`, fournit une
réfutation permanente du garde empirique. Sur `two_lines,n=10`, son exécution
non vérifiée rend code zéro, `q2=20` et `diam_max/dmax=0,007`; le même binaire
avec `--verifie` trouve `brute_q2=45` et rend code un. Même la borne de
soixante-quatre espacements conserve `20` contre `45`. Le maximum observé ne
peut donc jamais remplacer la conservation des ancres ou les calottes
certifiées.

Cette sonde duplique aussi q3 lorsque deux arêtes maximales égales partagent
leur premier endpoint : l'owner ne compare pas le second composant de
`EdgeKey`. Enfin, son vecteur `acu` est consommé matériellement par q3 puis q4.
Il s'agit d'un diagnostic monolithique, pas des producteurs autonomes spécifiés
ci-dessus. Les `11/11` CTests ciblés, verts en `8,82 s`, n'ont ni juge q3, ni
fixture `two_lines`, ni records `I_B/U_B/BallKey`; ils ne reçoivent donc pas
J0. Le détail reproductible est dans
[`audits/AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md`](audits/AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md).

La rampe CPU 48 cœurs confirme la forme du verrou sans recevoir la source. À
`uniform,50000`, le ledger vaut `21 432 482` candidats sous `smax=11` et
`4 004 994` sous `smax=6`, soit une réduction `5,35` plutôt que douze. Le
premier cas matérialise encore `6 091 112 797` paires de lentille pour
`9 768 840` q4, un ratio `623,5`. Sur les amas, le cutoff refuse dès 12 500
après jusqu'à `24 135 659 695` paires. Ces compteurs imposent de retirer le
produit q4 et de mesurer séparément les `Q4Seed3`, pas d'élargir la fenêtre de
la grille. Reçu et limites :
[`audits/AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md`](audits/AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md).

La première proposition de « cellules admissibles à `r` » ne répare pas la
complétude. Son lemme antipodal est correct, mais un partenaire admissible à
`D>=r` peut avoir `||v-x||>r`; le voisinage à `r` le perd. Une enveloppe sûre
des directions possibles est l'union, sur `s=v-x`, des calottes
`2(s.u)>max(r,||s||)`. Toute cellule qui **intersecte** cette union est
potentielle et doit être couverte entièrement, ou scindée fail-open. Les
seuils restent autonomes `10/9/8`. Le théorème, la version shell plus serrée et
les contre-fixtures sont dans
[`audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md`](audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md).

Le pin `95b41b7` matérialise l'intersection du cône, les partenaires globaux et
la comparaison q4 au diamètre `2R`. Ses mesures bornées passent de `23,00 %` à
`88,00 %` sur `uniform`, et de `0,00 %` à `14,33 %` sur `terrain`, mais ces
pourcentages ne sont pas encore des certificats : `floor(sqrt(ds))+1`
sur-approxime le seuil et peut omettre une cellule potentielle. L'ABI exacte
transmet `T2=max(r*r,ds)` et compare les carrés sans racine. La fixture
`s=(10,0,0)` avec un sommet `(3,0,-5)` sépare les deux versions. Après ce
correctif, q3 et la masse résiduelle resteront encore à fermer.

La construction positive ne s'arrête pas à cette réfutation. Sur une arête
maximale exacte `e=(a,b)`, l'égalité de shell et Jung bornent les directions du
centre. Minimiser le quotient de puissance sur ce cap redonne les cœurs
universels `W2/W3/W4` : dix, neuf ou huit témoins stricts ferment donc l'ancre
de leur lane avant toute complétion. Au niveau WSPD, un bloc est fermé par une
enclosure corrélée reçue, scindé ou conservé comme continuation ; jamais omis
par une fenêtre.

Pour une arête résiduelle, les centres des sphères passant par ses endpoints
vivent dans son plan médiateur et chaque site y définit une forme affine.
Lane3 évalue le point propre à chacun de ses `Third3`. Lane4 sélectionne les
premiers/derniers roots sur chaque `Q4Seed3` et remplace ainsi
`sum_e binom(m_e,2)` par des visites de BVH et au plus `2*r4*m_e` groupes
d'incidences. Le cutoff rationnel `theta=p/q` se traite par la quadratique
séparable `Q_theta=q*A-p*B`; son minimum continu taille un nœud et son maximum
aux coins crédite le census. L'architecture, la preuve du primary aigu q4,
les ledgers et les fixtures sont dans
[`audits/NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md`](audits/NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md).

Le raccord ponctuel de `2d8aa5f` valide le remplacement : à
`n=6000,smax=6`, sortie q4 identique `89796`, mais `48 791 131` couples
deviennent `830 044` roots. Le temps reste `25--28 s`, car le prototype relit
encore chaque liste `inner` par seed. Le port device de `2c14313` est donc une
baseline de flat scan et de parité, pas encore l'architecture : sa prochaine
étape peut immédiatement remplacer le rescan de census par
`census_replay(sel,apex,seed3,pw)`, avec fates séparés et vrais `I_B/U_B`.
Avant J2, `Lane4EdgeBatch` factorise le voisinage par arête et la sélection
plate passe de cinq scans à deux. Ensuite, la forme J2 remplace leurs visites
par les wavefronts
`(SeedId,WitnessNode,side,cutoff)` et réutilise `Q_theta` pour top-k, census et
shell. `DEBORDEMENT` est une continuation ou un refus, jamais une mort. Le reçu
borné, les limites du batch et la gate causale sont dans
[`audits/AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md`](audits/AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md).

Le brut du premier attempt G4 est maintenant récupéré et committé à `c03c0ee`.
Les douze CSR fournis donnent `ecarts=0` sur les champs fixes de `SeedOut`, soit
`18 617 211` seeds. La parité host/device du noyau plat est donc un résultat
positif ; la complétude de source est une porte distincte. Seuls les trois lots
`uniform,smax=6,n=1500/3000/6000` ont `cap=0`. Le dernier parcourt `293,6 M`
incidences en `55,33 ms` (`5 307 Msites/s`) ; son extrapolation à 50k vaut
environ `0,46--0,48 s` pour le kernel seul à K=5. Tous les lots `smax=11` sont
tronqués, donc le reçu ne mesure pas K=10. Il ne chronomètre ni construction,
ni H2D/D2H, ni census, ni aval, et la configuration annonce `52` plutôt qu'une
cible Blackwell native explicitement reçue. La prochaine porte doit fixer
`CMAKE_CUDA_ARCHITECTURES=120-real`, publier les phases temporelles, hacher une
copie immuable du runner et séparer parité `SeedOut`, `SOURCE_BATCH_COMPLETE`
et `warm_e2e`.

La projection rend aussi le raccord physique monolithique impossible :
`2,45 G` incidences dépassent `INT_MAX`, valent environ `9,8 Go` d'IDs et les
`24,6 M` `SeedOut` fixes environ `5,42 Go`. Le jalon industriel n'élargit donc
pas simplement l'offset : il produit les seeds dans une tuile device, descend
le BVH `Q_theta`, compacte uniquement les groupes/fates survivants, puis libère
la tuile. Le tuilage exact ci-dessous donne la frontière de travail ; les
offsets 64 bits ne restent qu'un garde de diagnostic, pas une raison de
matérialiser le CSR global.

Le premier format device utile est donc factorisé par arête de Lane4 :
`EdgeBatch={a,b,D2,Third4[],S_ab[]}`, où
`S_ab={z!=a,b:||2z-a-b||^2<=4D2}` est stocké une fois et le kernel ignore le
`Third4=x` courant. Aucun objet q3 n'entre dans cette ABI. Une instrumentation
locale au pin `c03c0ee` trouve `10,90/11,26/11,46` seeds par arête sur
`uniform,n=1500/3000/6000`. Cela localise une répétition proche de onze au
niveau carrier, mais le facteur H2D/HWM exact doit publier
`sum_e c_e*(m_e-1) / sum_e m_e`, car `c_e` et `m_e` peuvent être corrélés.
Cette factorisation ne réduit pas encore les visites témoins. Le noyau exact
enchaîne alors deux scans — classification et top-`r4` simultanés, puis
range-report des deux cutoffs — au lieu des cinq
scans courants, et `census_replay` supprime le rescan aval. Le BVH remplace
ensuite ce second scan sans modifier l'ABI logique.

La matérialisation peut être bornée par un tuilage exact du domaine déjà couvert
par `dmax`. Chaque support est émis dans la tuile de son sommet
lexicographiquement minimal ; un halo L-inf
`H=ceil(3*dmax/2)` contient tous ses sommets, intérieurs et shell, car
`2R<=sqrt(3/2)*D` pour q4 et les constantes q2/q3 sont plus petites. La somme
des multiensembles de tuiles doit égaler le global avec IDs inchangés. Ce
recollement borne la HWM, mais ne donne aucune autorité aux ancres omises par
la coupure.

Chaque générateur généralise WSPD jusqu'au census de **sa propre** miniboule ;
une lentille ou un support positif sans profondeur n'est qu'une supersource.
Le contrat précis et la réponse négative à Q14 sont dans
[`audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).

### 4.4 Midball q2 : consolider, puis spécialiser le hot path

Le même commit ajoute `MidballBlockDepth` et neuf portes. Ses extrema sont
exacts sur le produit des **AABB du réseau entier u16**, pas sur les seuls
`PointId` occupés ; `ALL` est aussi exact continûment, tandis que `NONE` ne doit
pas prétendre exclure le point continu à demi-coordonnée. Cette primitive
duplique `rect_h_interval/rect_classify` déjà présents dans `rect_front.hpp`,
avec leurs fixtures réseau/continu, milieu impair et extrêmes u16. Une seule
autorité doit survivre.

Pour le raccord q2, une seule dominance est générale :

```text
central ALL => Hmin>0
```

Midball doit donc être essayé sur tout verdict central non `ALL`, y compris
`NONE`. La corrélation perdue entre `D` et le score interdit le converse : pour
`A=[0,8]`, `B=[10,100]`, `C={9}`, on a `Hmin=1>0`, mais `Dmin=4` et
`smin=8100`, donc Midball `ALL` face à central `NONE`. Le fallback ne reprend
que `MIXED` et ne couvre pas ce cas. Seul `Hmin` est sémantiquement nécessaire,
soit 24 produits. Le source appelle encore la primitive complète, mais
l'inlining Release élimine déjà le maximum et laisse 24 multiplications dans
le bloc machine ; une ABI min-only explicite sert surtout l'autorité commune et
le futur device.

Le raccord exploratoire applique cette règle, mais sa réception reste dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). La proposition
ne conserve ni pin, ni nombre de CTests, ni mesure mutable.

### 4.5 Borne duale de vague : maintenir `upper`, pas `reste`

Une coupure exacte du **certificateur singleton** complète la saturation par le
bas. Pour chaque ledger `L` et lane `q`, initialiser :

```text
upper[L,q] = cred[L,q] + population disjointe représentée par sa pile
```

Il est plus sûr de maintenir directement `upper` que de soustraire un parent
puis réajouter ses enfants :

```text
ALL                    -> upper inchangé (reste devient crédit)
MIXED interne          -> upper inchangé (les enfants partitionnent le parent)
NONE consommé          -> upper -= pop(parent)
MIXED sans continuation-> upper -= pop(parent)
upper < need[q]         -> SINGLETON_CERTIFICATE_EXHAUSTED
```

Cette écriture rend impossible le mutant `drop-mixed-children` qui a réfuté la
première révision `--borne-sup`. Les préconditions sont impératives : piles en
antichaîne, populations/IDs authentifiés, un `upper` séparé pour `cred/mask` et
`ccred/cmask`, et conservation exacte lors d'un split. `--climb` inclut ou
borne aussi la feuille centrale omise. Une source de crédit post-boucle comme
BJD impose son propre potentiel de banque ; en son absence, la combinaison est
refusée.

Le statut n'est jamais `EVENT_OPEN`. Un échec du cœur universel ne réfute ni un
groupe Jung, ni une complétion q3/q4 : il signifie seulement que cette source
de crédits ne peut plus atteindre le seuil. Pour rendre la coupure utile, une
ordonnance adaptative peut traiter d'abord les gros nœuds probablement `NONE`
quand `upper-need` est petit, et les nœuds proches probablement `ALL` quand
`need-cred` est petit. L'ordre reste heuristique ; les extrema et les deux
bornes restent exacts. Les portes OFF/ON exigent fates, masses, pending et
sorties bit-identiques avant de mesurer les visites évitées.

### 4.6 Préfiltre HC q3/q4 et autorité `Corner512`

Pour `e=z-a`, `t=b-z`, `H=e dot t` et `C=t cross e`, les lanes vérifient :

```text
q2 : H>0
q3 : H>0 et 3 H^2>||C||^2
q4 : H>0 et 2 H^2>||C||^2
```

Sur trois AABB, `Hmin` est exact. Enfermer chaque composante de `C` par deux
intervalles de produits puis sommer les maxima absolus carrés donne un majorant
sûr de `||C||^2`. La comparaison avec `2Hmin^2/3Hmin^2` est donc un préfiltre
`ALL` fail-open. Elle n'est pas exacte : les corrélations entre les deux termes
d'une composante, entre composantes et avec `Hmin` sont perdues. Ce certificat
compte des témoins universels d'une ancre partielle ; il ne remplace pas les
supports complets et leurs boules uniques.

L'autorité exacte d'enveloppe existe déjà : `corner512_all_lane` vérifie les
`8*8*8` triples de coins. L'équivalence vient de la convexité du cône de
Lorentz, de l'affinité en `a,b`, et de l'affinité de `C` avec la concavité de
`H` en chaque coordonnée de `z`. L'ordonnance coût-adaptative est donc
`central -> HCIntervalAll -> SOC64 -> Corner512/split`, avec arrêt au premier
`ALL`; aucun de ces échecs ne vaut `NONE` géométrique.

HC reste un préfiltre, pas un jalon de source. L'intégration industrielle
calcule sur un nœud les trois prédicats purs demandés par les trois producteurs
autonomes et mesure leur gain marginal face au fallback déjà actif. Elle ne
partage aucun verdict ni état de complétude entre eux. Les défauts logiciels et les portes
présentes sont maintenus uniquement dans l'audit courant.

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

### 5.2 `WST3CandidateCover`, puis `OwnedCK-WST3`

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
troisième ID à un span unique de son antichaîne. Cela reçoit seulement le
routage `WST3CandidateCover`. La relation exacte est son intersection avec
trois `PointId` distincts, le vrai prédicat owner et la positivité q3. Un juge
qui choisit d'abord l'owner puis ignore les autres rectangles prouve la
couverture adressée par owner ; il ne prouve pas que la relation brute émise
est exact-once.

Sous une vraie décomposition fair/compressed en cellules de maille comparable à
`eta*r_R`, un argument de packing donne conditionnellement
`O(s^3*eta^-3*n)` blocs initiaux pour `0<eta<=1`. Cette borne ne se transfère
pas à un arbre de plages Morton scindé par population avec AABB serrées sans
preuve supplémentaire. Elle ne couvre ni les splits owner/acuité, ni les
diagonales, ni la masse logique, ni les `BallKey`, ni le census. Toute pente ou
constante « blocs par rectangle » reste une mesure de `CandidateCover`, pas un
théorème sur `OwnedCK-WST3`.

Le probe counter-only reçoit seulement cette couverture adressée : WST3 `5/5`
et WST4 `1/1` sur leurs campagnes bornées au pin audité. Il rejette les
positions dupliquées, départage les owners par rang Morton, laisse les facteurs
témoin recouvrir `A/B` et ne cherche aucune copie sous une ancre non-owner. Son
juge partage les deux premières conventions et ignore les deux dernières par
construction. Les comptes bruts se nomment donc `CandidateCover`, jamais
`OwnedCK-WST3/WST4`. À `n=8000`, le produit WST4 atteint déjà environ
`187` millions de blocs ; owner, distinct-ID, acuité et profondeur doivent
précéder `sum k_t^2`. Le contre-audit reçu est
[`audits/AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](audits/AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md).

Le classifieur normatif emploie `H=(x-a) dot (b-x)=F-E` et l'identité
`E+X-D=-2H`. Ses extrema exacts se lisent donc
`Hmax<0 => ALL_ACUTE`, `Hmin>=0 => NONE_ACUTE`, sinon `MIXED`. La fixture du
tétraèdre régulier, owner `EdgeKey(0,1)`, quatre poids `1/4` et deux carriers
`H=-1`, est permanente. Toute mesure issue de la révision historique qui
classait `H>0` comme aiguë est invalidée. Le statut logiciel, les pins et la
portée du juge sont maintenus uniquement dans l'audit courant.

Après owner, q3 est positif si `E+X-D>0` et `G=D*E-F^2>0`. Les blocs historiques reçoivent
`ALL_ACUTE/NONE_ACUTE/MIXED` par bornes entières sûres ; les égalités
descendent. `ALL_ACUTE` ne signifie ni owner commun, ni `BallKey` commune. La
relation n'est exacte qu'après intersection de tous les filtres, et cela ne
prouve jamais que sa masse, son rang, son shell ou ses `BallKey` sont bon marché. Le
Le tape commun `q3_open || q4_open` décrit le probe réfuté et n'appartient pas
à l'architecture active. `Lane3` construit ses triples ; `Lane4` construit
indépendamment ses `Q4Seed3` depuis la partition neutre.

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
de triangles q3 critiques, aigus et vides. Cette obstruction interdit une
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

Un certificat dual donne néanmoins une voie uniforme falsifiable. La bonne
orientation du minimax est
`min_w max_z Phi_z(w)=max_lambda min_w sum_z lambda_z Phi_z(w)` ; le commentaire
actuel de `prototype/jung_dual.hpp` l'inverse, même si ses formules emploient la
bonne identité. Écrire
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
`A0=-L*(a dot b)+(a+b) dot Z-Q` et
`C=(L*a-Z) cross (L*b-Z)`. Le même reçu vérifie :

```text
q3 : A0>0 et 3*A0^2*L^2 > ||C||^2
q4 : A0>0 et 2*A0^2*L^2 > ||C||^2
```

La largeur dépend du dénominateur commun `L`; un cap dépassé rend `UNKNOWN`.

Une forme équivalente, directement adaptée à une implémentation sans fractions,
pose `W=sum_z w_z`, `D=||b-a||^2`,
`A4=W*D-sum_z w_z||a+b-2z||^2=4*A0`,
`P=W*(a+b)-2*sum_z w_z*z` et
`R=D||P||^2-(P dot (b-a))^2`. Elle décide :

```text
q3 : A4>0 et 3*A4^2>4*R
q4 : A4>0 et   A4^2>2*R
```

Sous u16, la preuve de largeur i128 exige `W=sum_z w_z<=65535`, contrôlé sans
overflow avant le prédicat. Au pin `5809bd2`, seul
`BlockJungDual64::make_base` préflighte la somme en accumulation large puis la
borne à 65 535. `dual_lane` ne possède pas encore ce cap et le symbole
contractuel `verify_dual_weights_lane` n'existe pas dans le logiciel. Le futur
wrapper **doit** préflighter coordonnées u16, `lo<=hi`, paire propre, IDs
authentifiés et contrat de preuves ; toute violation rend `UNKNOWN` avant un
cast étroit. Le juge primal `k=2/3` et la profondeur appartiennent à l'oracle,
jamais au vérificateur de poids.

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

#### Proposant exact de poids communs pour deux IDs

Une base de deux témoins n'exige pas une banque finie. Paramétrer les poids
normalisés par `lambda in (0,1)`, avec
`Z(lambda)=z2+lambda*(z1-z2)` et
`Q(lambda)=||z2||^2+lambda*(||z1||^2-||z2||^2)`. Pour chacun des 64 couples de coins, écrire
`A_i(lambda)=alpha_i+beta_i*lambda` et
`C_i(lambda)=u_i+v_i*lambda`. La lane q4 impose
`A_i(lambda)>0` et
`2*A_i(lambda)^2-||C_i(lambda)||^2>0`; q3 remplace `2` par `3`.

La préimage de l'intérieur du cône de Lorentz par cette droite affine est un
intervalle ouvert. Intersecter exactement les 64 intervalles et `(0,1)` décide
donc s'il existe un même poids réel sur toute la tuile. Les bornes sont des
racines de quadratiques à coefficients entiers ; leur ordre se compare en
BigInt sans les arrondir. Dans l'intervalle final, Stern--Brocot ou les
fractions continues donnent le rationnel de plus petit dénominateur. Pour
`lambda=p/q`, employer les poids réduits `(p,q-p)`, exiger `q<=65535`, puis
rejouer `BlockJungDual64`. Intervalle vide, frontière seule ou dénominateur trop
grand donnent `MIXED/UNKNOWN` et un split, jamais `NONE`.

Cette primitive sépare proprement le proposant du vérificateur et évite de
retester plusieurs poids arbitraires pour la même paire. Ses fixtures sont :
intervalle commun non trivial ; coins individuellement faisables mais
intersection vide ; intervalle sans rationnel de dénominateur au plus 65 535 ;
extrémité `lambda=0/1` rabattue vers le singleton ; invariance par pgcd et
permutation des deux témoins. Le branch-and-cut appelle ce proposant seulement
sur une base qui réfute son transversal courant.

Au niveau rectangle, les poids **fixes** admettent un classifieur exact beaucoup
plus simple. Poser `A0=-L*(a dot b)+(a+b) dot Z-Q` et
`C0=L*(a cross b)-a cross Z-Z cross b`. Alors
`C=(L*a-Z) cross (L*b-Z)=L*C0`, `P cross (b-a)=2*C0`, donc
`R=4*||C0||^2`, et le reçu devient :

```text
q3 : A0>0 et 3*A0^2 > ||C0||^2
q4 : A0>0 et 2*A0^2 > ||C0||^2
```

Les identités de contrôle sont `alpha=A0/L`, `p=P/(2L)`, `A4=4*A0` et
`R=4*||C0||^2`. Elles relient directement le cône rectangle aux deux formes
ponctuelles précédentes et empêchent toute confusion de facteur quatre.

Pour `b` fixé, `(A0,C0)` est affine en `a`, et réciproquement. Chaque lane est
l'intérieur d'un cône de Lorentz convexe
`{(t,c):t>0,||c||<sqrt(k)*t}`. Si les huit coins de `A` passent pour chacun des
huit coins de `B`, la convexité séparée propage donc le verdict à tout
`A×B`; la réciproque est triviale. `BlockJungDual64` est ainsi une
**équivalence exacte sur l'enveloppe AABB continue** pour cette base et ces
poids : 64 prédicats de coins, plus au besoin un prétest intérieur de rejet
sûr, puis `ALL_GROUP` ou `MIXED`. Si le prétest est conservé, la télémétrie et
le budget annoncent 65 évaluations au pire.

Un fast path suffisant évite souvent ces 64 prédicats complets. Pour chaque axe,
les extrema de `g_i(x,y)=-L*x*y+Z_i*(x+y)` sur `A_i×B_i` sont aux quatre coins ;
ainsi `Amin=sum_i min(g_i)-Q` est le minimum exact de `A0`. Pour une permutation
cyclique `(i,p,q)`, poser `f_pq(x,y)=L*x*y-x*Z_q-Z_p*y` ; comme
`C_i=f_pq(a_p,b_q)-f_qp(a_q,b_p)` sépare deux couples de variables, son
intervalle coordonnée est exact à partir de deux fois quatre coins. Avec
`Mi=max(abs(Ci_lo),abs(Ci_hi))`, les tests stricts
`2*Amin^2>sum_i Mi^2` et `3*Amin^2>sum_i Mi^2`, sous `Amin>0`, certifient
respectivement q4 et q3. L'échec retombe sur les 64 coins ou `MIXED` ; il ne
vaut jamais `NONE`. Cette porte `BJD-BilinearBounds` paie 36 valeurs
bilinéaires scalaires, garde toutes les préconditions u16/poids/IDs et doit être
reçue par l'implication `FAST_ALL => BJD64_ALL` sur petits produits exacts.

Le fallback ne reforme pas 64 dot/cross. La bilinéarité donne une table exacte
composée de la valeur au coin bas, de six différences premières et de neuf
différences mixtes `Delta a_i Delta b_j` pour `(A0,C0)`. Chaque couple de coins
se reconstruit ensuite par additions `i128`, avant les carrés et comparaisons de
cône. La porte de réception impose la parité bit-à-bit avec l'évaluation directe
sur les 64 coins, y compris aux égalités et aux bornes u16.

Sous u16, poser `U=65535`. Avec `1<=L<=U`, les écritures
`A0=-sum_z w_z*(z-a) dot (z-b)` et
`C0=sum_z w_z*(a-z) cross (b-z)` donnent
`|A0|<=3*L*U^2<2^50` et `|C0_i|<=2*L*U^2<2^49`; les produits comparés restent
donc dans i128. Le widening précède impérativement `a+b`, chaque produit
dot/cross et `||z||^2`; le preflight de `L` somme en type large ou saturant
avant tout cast étroit. Précalculer `L,Z,Q`, rejouer les 64 couples et publier
la plus petite lane universelle. Un coin échoué ne réfute ni les seuls points stockés
ni une autre pondération ; il impose un split ou `MIXED`. La contre-fixture
« coins bons, paire médiane shell » ne concernait que des poids reproposés
séparément aux coins : elle ne réfute pas ce théorème à poids communs.

L'ABI logicielle du pin stable ne porte pas encore exactement ce contrat.
`make_base` préflighte `L` en `i128`, mais `bjd_lane_box` rend `kLaneNone` pour
une base invalide. Le callsite q4 courant reste fail-open parce qu'il ne teste
que `retour>=q4`; un futur consommateur pourrait néanmoins confondre invalidité
et `NONE` géométrique. Le résultat reçu est un type séparé
`ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`, ou une valeur `UNKNOWN` explicite pour
toute base invalide. `dual_lane` doit en outre recevoir son propre preflight de
poids avant de prétendre au même domaine.

`ALL_GROUP` ne vaut qu'une contrainte de profondeur. Le wrapper reçu doit
conserver les vrais ensembles de `PointId`. Le packing minimal impose des groupes deux à
deux disjoints et disjoints de tous les singletons ou proof-spans déjà crédités
dans la même vue ; deux vues logiques ont deux ledgers d'identités. La route
plus complète garde aussi les groupes recouvrants comme hyperarêtes et ferme
sur `tau(F)>=h`. Additionner `cred` et un nombre de groupes sans l'un de ces
reçus est le mutant `sum-instead-of-union`. Un juge capé publie
`PARTIEL/UNKNOWN` ; son absence ne vaut pas accord.

Le commit `5809bd2` met en œuvre le packing : il exclut les feuilles déjà
créditées dans les deux vues, impose des groupes disjoints et tue deux mutants
dans trois CTests. Ce n'est pas encore une réception. À ce pin,
`--juge-bjd=1` laisse des groupes et fermetures sautés avec
`fenetre_finale=OUI`, `OK` et le code zéro ; sans `--vwave`, l'option BJD peut
réussir avec zéro essai. Le commit `694920a` répare ces deux statuts, grave la
fixture collinéaire et porte la sous-suite BJD à huit CTests verts. Le parent
`8fd6f59` refuse ensuite `--exige-q4-ouvert` sans juge et une cardinalité autre
que neuf pour `collinear_seven`. La réception reste ouverte : surtout, le
packing est calculé après la descente et n'économise aucune recertification. Le
contre-audit et la fixture source u16 sont dans
[`audits/AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`](audits/AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md).
Le juge singleton `--judge-vwave` n'est pas composable avec SOC/BJD : il doit
être refusé dans ces modes tant qu'il ne rejoue pas leurs hyperarêtes et seuils.
De même, le shadow `--judge-soc64` ne juge pas un succès déjà promu par le
chemin actif ; une porte active-only est requise.

Le mapping G4 naturel affecte une proof-tile à un CTA de 64 threads, un couple
de coins par thread, puis deux ballots warp et une réduction. Chaque thread
forme `A0,C0` avec widening et compare environ 106 bits via deux limbs u64 ;
`MHGP_HD __int128` ne constitue pas une primitive device reçue. Le proposant
de poids reste séparé du vérificateur, afin que le tape rejoue exactement la
base et les poids authentifiés.

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
intérieurs. Ainsi `Depth(P,h)` signifie exactement que, pour tout
`R subset P` avec `|R|<=h-1`, l'intersection des `B_z`, `z in P minus R`, est
vide. C'est l'absence de point commun avec tolérance `h-1`.

Le théorème de Helly avec tolérance de Montejano--Oliveros donne alors, par
contraposée, un sous-pool `C` qui certifie déjà le même **seuil**
`Depth>=h` et vérifie
`|C|<=eta(3,h)`. La borne d'Erdős--Gallai--Tuza satisfait
`eta(3,h)<binom(h+2,2)+binom(h+1,2)=(h+1)^2`. Il existe donc toujours un reçu
de **80 IDs au plus pour q4** et de **99 IDs au plus pour q3**, par stricte
inégalité entière et non comme valeurs exactes de `eta`, sous `smax=11`. La
source primaire du transfert tolérant est
[Tolerance in Helly Type Theorems](https://doi.org/10.1007/s00454-010-9296-6),
théorème 3.1 ; la borne hypergraphique vient de
[Critical hypergraphs and intersecting set-pair systems](https://doi.org/10.1016/0095-8956(85)90043-7).

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

Ce principe n'est pas propre au disque de Jung. Pour tout domaine convexe de
centres de dimension affine `r`, lorsque chaque région mauvaise est convexe,
former les bases couvrantes d'au plus `r+1` IDs donne encore
`Depth=tau(E)` par Helly. La chaîne q4 réduit donc naturellement le rang du
certificat : disque pair-level de dimension deux, hyperarêtes de taille trois ;
segment d'axe après une face, hyperarêtes de taille deux ; centre/BallKey fixé,
singletons. Chaque facteur de `OwnedCK-WST4` doit restreindre le domaine avant
le fill, au lieu d'énumérer les supports puis de refaire le census.

Un constructeur exact fonctionne par séparation de coupes. Il maintient un
petit ensemble `F` d'hyperarêtes déjà certifiées et résout le transversal borné
`tau(F)`. Si `tau(F)>=h`, le reçu ferme. Sinon il choisit un transversal
`R`, `|R|<h`, et appelle l'intersection de demi-plans sur `P minus R`. Un
contre-centre prouve `Depth<h`; une intersection vide rend une nouvelle base
`G` disjointe de `R`, qui est ajoutée à `F`. Sous cap, l'absence de terminaison
rend `UNKNOWN`. Le replay GPU vérifie chaque petite base géométrique une seule
fois, puis résout `tau(F)>=h` par branches bitset de facteur au plus trois ; il
ne répète plus la géométrie à chaque nœud leave-out.

Sur une proof-tile CK `Q`, définir de même `E_Q` comme les groupes dont la
couverture est certifiée **uniformément pour toute paire propre de `Q`**. Le
test `tau(E_Q)>=h` suffit alors à fermer tout le rectangle sans PairId. S'il
échoue, un transversal `R` de taille inférieure à `h` indique quels IDs retirer
au prochain appel du proposant sur un représentant. La base ponctuelle rendue
n'est ajoutée à `E_Q` qu'après preuve polynomiale uniforme ; sinon la tuile se
scinde. Cette boucle partage le même `RectId` et ne modifie jamais la partition
source. Elle est volontairement incomplète sur une tuile grossière : des bases
qui varient avec `(a,b)` peuvent fermer toutes les paires sans qu'un petit
hypergraphe commun soit encore visible.

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

Le fast path conserve `U`, les singletons universels obligatoires, puis une
petite famille `F` d'hyperarêtes uniformément reçues. Il teste directement
`tau(F_res)>=h-|U|` par branchement bitset : au plus `3^7` feuilles pour q4 et
`3^8` pour q3 avant mémoïsation. Le packing disjoint reste une ablation sûre,
pas la décision principale. Un transversal trop petit fournit la coupe qui
demande une nouvelle base au proposant ; chaque base géométrique est vérifiée
une fois avant d'entrer dans `F`. Un cap produit `PENDING`, jamais `CLOSED`.
Sur un rectangle CK, un échec de preuve uniforme scinde `A/B`. La continuité
d'une marge stricte garantit qu'un reçu ponctuel reste valable sur un
voisinage assez fin, sans promettre un poids commun sur une tuile grossière.
Le raffinement candidat part du front CK coarse reçu, proche de `s=2`, plutôt
que d'un front global `s=8`. Pour `k=2`, une intersection vide conserve un
certificat minimal : un coin infaisable, deux intervalles de coins
incompatibles, ou un conflit coin--frontière `lambda=0/1`. Le raffinement
scinde d'abord le facteur ou l'axe qui sépare le support de ce conflit. Pour un échec BJD
plus général, il choisit le facteur qui maximise la variation bornée de
`A0/C0`. Ce choix est une heuristique d'ordonnance, pas une preuve ; l'exactitude
vient de la partition exhaustive des enfants, qui héritent toutes les
hyperarêtes et pondérations déjà uniformes. La fixture de réception exige un
parent sans poids commun, deux enfants `ALL` et une somme des masses enfants
exactement égale à celle du parent.

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

Le classifieur `JungSpindleRect-v0` qui combine des extrema séparés de `D,V,T`
est une autre borne. Son faible gain historique ne réfute ni `SOC64` ni
`CORNER512`. Même exact pour `ALL`, `CORNER512` ne prouve aucune parcimonie.

Les états d'implémentation, campagnes et reçus SOC sont maintenus dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). La porte
normative est : shadow apparié sans changement de fate ; oracle d'union par
vrais `PointId` ; puis branchement actif seulement avec proof-ledger, cap et
`PENDING` propres. `central-NONE` n'autorise pas à déclarer le domaine exact
vide ; une vue combinée complète descend sur `SOC-UNKNOWN`.

Mesurer tâches, early exits, opérations larges, octets, HWM et coût transitif
avant toute rampe. `CORNER512` ne vient que sur les tâches de masse suffisante
pour amortir ses 512 coins, plus un éventuel prétest. Un échec reste
`AABB_envelope_not_all/UNKNOWN`, jamais `NONE` pour les points stockés.

La fixture `A=[0,99]x{100}x{100}`, `B=[101,200]x{100}x{100}` et
`C={(100,100,100)}` ferme q4 par `SOC64` alors que les extrema scalaires
échouent. Le mutant `sum-instead-of-union` additionne les preuves d'un ancêtre
et de ses descendants ; l'oracle d'identités doit le tuer.

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

## 6bis. Préfiltre d'ancre combiné : cœur, `h_a`, `h_b`

Cette section formalise la proposition du 15 août 2026 et rapporte sa mesure.
Elle ne reçoit aucun logiciel et ne promeut aucun statut ; `public_status`
reste `not_claimed`.

### 6bis.1 L'énoncé

Un support positif d'arité `q` est possédé par sa **paire diamétrale** `(a,b)`.
Tout site strictement intérieur à sa miniboule appartient au fuseau
`W_q(a,b)`, intersection de toutes les boules admissibles de cette ancre à
cette arité. Le filtre d'ancre du contrat de source tue l'ancre dès que
`|P inter W_q(a,b)| >= h_q`, avec `h_q = s_max - q + 1` — soit `10 / 9 / 8` à
`s_max = 11`, et `5 / 4 / 3` à `s_max = 6`. Il y a donc bien **un `h` par
arité**, décroissant avec elle.

Compter `|P inter W_q|` exactement coûte une requête par paire. On le **minore**
par trois comptes, calculés une fois par rectangle `A x B` de la partition
Callahan--Kosaraju :

- `h_coeur` — témoins universels sur **tout** le rectangle, pris hors `A` et
  hors `B` ;
- `h_a` — témoins de `A` universels sur `{a} x B`, un compte par `a` ;
- `h_b` — témoins de `B` universels sur `A x {b}`, un compte par `b`.

**Théorème de disjonction.** Les trois ensembles sont deux à deux disjoints, et
la disjonction est acquise deux fois. Par construction `h_coeur` exclut
`A union B`, `h_a` vit dans `A`, `h_b` dans `B`, et `A inter B` est vide par
définition de la partition CK. Mais elle est aussi **automatique** : pour `z`
dans `A`, le choix `a = z` donne `H = 0`, donc `z` n'est jamais certifié témoin
universel du rectangle. Aucune hypothèse géométrique n'est requise, et en
particulier aucune séparation minimale.

**Corollaire.** `|P inter W_q(a,b)| >= h_coeur + h_a + h_b`, donc l'ancre meurt
dès que cette somme atteint `h_q`. Le filtre est fail-open : il ne ferme jamais
à tort.

C'est l'**union commune** que le contre-audit de la gate à trois voies
réclamait, et que sa juxtaposition de trois mesures ne fournissait pas.

### 6bis.2 La décision ne touche jamais une paire

`h_coeur` ne dépend que du rectangle, `h_a` que de `a`, `h_b` que de `b`. Le
nombre d'ancres survivantes vaut donc

`somme_{a dans A} |{ b dans B : h_b < h_q - h_coeur - h_a }|`

et, tous les comptes étant écrêtés à `h_q <= 10`, un histogramme de `h_b` sur
onze cases suffit. Le coût par rectangle est `O(|A| + |B|)` une fois les `h`
connus, **jamais `O(|A| |B|)`** : aucune paire n'est matérialisée.

### 6bis.3 Les bornes, et pourquoi les sommets suffisent

Pour `w = z-a` et `d = b-a` : `H = d.w - |w|^2` et `Xi = |d x w|^2`, avec
`W_2 : H>0`, `W_3 : H>0 et 3H^2>Xi`, `W_4 : H>0 et 2H^2>Xi`.

`H` s'écrit `somme_i [ z_i (a_i + b_i) - a_i b_i ] - |z|^2`. Le crochet est
**séparable par axe** et bilinéaire en `(a_i,b_i)` : son minimum sur le
rectangle plan est à l'un de ses quatre coins. Trois axes, quatre coins : le
minimum obtenu est **exact** sur le produit des deux boîtes.

Pour `Xi` la réponse est moins évidente et une première lecture la donne
fausse. En développant,

`(b-a) x (z-a) = b x z - b x a - a x z + a x a = b x z - b x a - a x z`,

le terme `a x a` étant nul : le produit vectoriel est **affine en `a`**. Donc
`Xi = |affine(a)|^2` est **convexe** en `a`, et son maximum sur une boîte est
atteint à un **sommet**. Tester les huit sommets est donc exact, et c'est le
majorant le plus serré possible — donc le `h` le plus grand qui reste
rigoureux. Une enveloppe d'intervalles serait sûre également, mais plus lâche :
elle est conservée comme mutant de mesure, et l'écart est considérable — sur
`uniform,n=8000,s=6,K=10`, la fermeture q4 passe de `47,6 %` par intervalles à
`91,0 %` par sommets.

### 6bis.4 Ce que la descente doit borner

Élaguer la recherche du cœur sur `max H` est beaucoup trop lâche : l'ensemble
`{z : max_{a,b} H > 0}` est en gros l'**union** des boules diamétrales du
rectangle. Ce qu'il faut borner est `max_{z dans Z} min_{a,b} H`, la quantité
qui décide, majorée par l'inégalité minimax `max_z min_c f_c <= min_c max_z f_c`.
S'y ajoute une voie rapide : si `min H` sur `A x B x Z` est positif, **tout**
point de `Z` est témoin q2 et sa population est créditée sans descendre.

Les trois lanes partagent une seule descente, les fuseaux étant emboîtés
`W_4 < W_3 < W_2` : un nœud élagué pour q2 l'est pour les trois. Et `(H, Xi)`
ne dépendant pas de l'arité, une seule évaluation par point sert les trois
compteurs.

### 6bis.5 Mesure du 15 août 2026

Campagne `counter-only`, graine `3`, profil u16, ledger bouclant exactement sur
`C(n,2)` à chaque point. Fermeture des **ancres** en pourcentage de `C(n,2)`.
Ce ne sont ni des supports, ni un débit, ni une pente reçue.

`uniform` :

| `n` | `s` | `K` | rect. | cell. max | q2 | q3 | q4 | q4 survivantes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `8 000` | `6` | `5` | `2,46 M` | `48` | `99,57 %` | `97,86 %` | `97,55 %` | `782 638` |
| `8 000` | `6` | `10` | `2,46 M` | `48` | `99,22 %` | `93,95 %` | `90,98 %` | `2 886 951` |
| `8 000` | `8` | `5` | `3,99 M` | `46` | `99,59 %` | `98,49 %` | `98,50 %` | `480 817` |
| `8 000` | `8` | `10` | `3,99 M` | `46` | `99,29 %` | `96,30 %` | `95,41 %` | `1 467 542` |
| `16 000` | `6` | `5` | `5,59 M` | `116` | `99,78 %` | `98,87 %` | `98,69 %` | `1 674 232` |

Trois lectures, et aucune n'est une extrapolation.

**`s = 8` domine `s = 6` à `K` égal.** En q4, `95,41 %` contre `90,98 %`, soit
**deux fois moins d'ancres survivantes**, pour `1,62` fois plus de rectangles.
Le compromis est donc favorable dans ce régime, ce qui n'allait pas de soi :
augmenter `s` rétrécit `A` et `B`, donc appauvrit `h_a` et `h_b`, mais enrichit
le cœur plus vite qu'il ne les appauvrit.

**`K = 5` ferme davantage que `K = 10`**, mécaniquement : les seuils passent de
`10/9/8` à `5/4/3`. L'écart en q4 est de `90,98 %` à `97,55 %` à `s = 6`.

**La fermeture ne se dégrade pas avec `n`** sur cette famille : de `n=8 000` à
`n=16 000` à `s=6, K=5`, q4 passe de `97,55 %` à `98,69 %`. Deux points ne font
pas une pente ; c'est J0 qui tranchera.

Le résiduel absolu reste le seul chiffre qui compte pour le contrat : à
`n=8 000, s=8, K=10`, il reste `1,47` million d'ancres q4, soit `184` par
point — à comparer aux `428` supports par point mesurés à `n=50 000`. Le
préfiltre ramène donc le candidat dans le même ordre de grandeur que la sortie,
ce qui n'était pas acquis. Il ne le rend pas *output-sensitive* pour autant :
`184` ancres par point n'est pas `428` supports par point, et chaque ancre reste
à instruire.

**Campagnes restant à rendre** : `eight_clusters` et `terrain`, et les tailles
`32 000`. C'est `eight_clusters` qui décide — la famille a résisté au cœur
commun, au fuseau sur rectangle et aux cinq certificats de bloc — et aucune
conclusion de famille obligatoire ne sera tirée avant.

### 6bis.6 Ce que la mesure ne dit pas

Le probe compte des **ancres survivantes**. Ce n'est pas un nombre de supports,
ni un débit, ni une pente reçue. Un rectangle dont une extrémité dépasse le cap
n'est pas décidé et toutes ses paires sont comptées survivantes : la mesure
**majore** donc toujours le résiduel. Aucun circumcentre n'est formé, aucun
rang n'est décidé.

## 7. q4 : générateur autonome et sélection axiale extrémale

La route active ne forme plus `OwnedCK-WST4`, `CellPair`, `Sym2` ou
carrier×apex. `Lane4` construit elle-même ses `Q4Seed3Block` depuis son univers
quaternaire WSPD, sans lire aucun objet de `Lane3`, puis demande au noyau axial
les seules racines pouvant porter une circumsphère shallow. Le fait qu'un
`Q4Seed3` ait trois IDs et soit géométriquement un triangle ne lui donne aucun
statut q3. Une fois le quatrième ID connu, owner6 et le plus petit vrai
`PointId` parmi les deux préfixes aigus choisissent la provenance primaire
exact-once ; positivité q4 et régularité s'appliquent avant toute émission.

La factorisation WST4 décrite ci-dessous reste une identité combinatoire, un
oracle de masse et un fallback réfutable ; elle n'est plus l'ordonnance P0. En
particulier, son `Sym2(A) disjoint_union Cross(A,N)` ne doit plus être raccordé
au chemin produit. Le théorème génératif qui le remplace est au § 7.2.

### 7.1 Factorisation CK de référence, non active en P0

La généralisation utile du WSPD est d'abord combinatoire. Soit un rectangle
pair-exact `R=(A,B)` et une antichaîne de cellules témoins disjointes
`C_1,...,C_k` couvrant le domaine nécessaire de ses complétions. Pour tout
`q>=2`, chaque vecteur entier

```text
alpha_i>=0, sum_i alpha_i=q-2, alpha_i<=|C_i|
```

définit exactement un atome symbolique :

```text
A x B x product_i binom(C_i,alpha_i)
```

La relation de supports complets est l'intersection de ces atomes avec
`INJECTIVE(PointId)`, l'owner total parmi toutes les arêtes et la positivité
circumcentrique. À q3, un seul `alpha_i` vaut un. À q4, on obtient exactement
les produits `C_i x C_j`, `i<j`, et les diagonales `binom(C_i,2)`. Cette
partition porte votre intuition sans quantifier sur des sphères incidentes à un
support partiel : chaque tuple complet survivant possède ensuite une seule
boule canonique à recenser.

Le nombre d'atomes symboliques non vides est le coefficient :

```text
[z^(q-2)] product_i (1+z+...+z^min(|C_i|,q-2))
```

Il est au plus `binom(k+q-3,q-2)`. Si, et seulement si, une construction
fair/compressed reçoit `k=O(eta^-3)` cellules par rectangle et
`O(s^3*n)` rectangles, le nombre de blocs initiaux est donc conditionnellement
`O(s^3*n*eta^(-3*(q-2)))`. Cette borne ne couvre ni les splits
owner/positivité/profondeur, ni la masse de sortie. Le LBVH actuel et son arrêt
par AABB serrée ne reçoivent pas encore l'hypothèse de packing.

Pour des endpoints fixés, poser `n_i=|C_i minus {a,b}|` et
`k_+=#{i:n_i>=1}`. La masse de l'atome `alpha` est
`product_i binom(n_i,alpha_i)` et la somme sur tous les `alpha` vaut exactement
`binom(sum_i n_i,q-2)`. Ainsi q3 possède `k_+` types non vides ; q4 en possède
`binom(k_+,2)+#{i:n_i>=2}`. Une représentation par slots ordonnés
doit quotienter par `product_i alpha_i!`. En dimension trois, la positivité est
vide pour `q>4`, faute de support minimal affinement indépendant plus grand.

Aucune séparation CK entre chaque paire `C_i,C_j` n'est requise pour la
complétude. Une telle séparation peut rendre un prédicat uniforme plus mordant ;
son échec déclenche un split atomique ou `PENDING`, jamais la suppression de
l'atome. En particulier :

```text
Sym2(C) = Sym2(L) disjoint_union (L x R) disjoint_union Sym2(R)
```

quand `L,R` partitionnent `C`. Un compteur `|C|-1` des nœuds internes ne vaut
pas implémentation de cette relation : la jonction aval doit réellement porter
les deux facteurs disjoints et retirer le parent.

Un sampler diagnostique de cette masse ne choisit jamais les **blocs** témoins
uniformément. Sa loi cible est déclarée : tuple brut, `SupportKey` owner et
`BallKey` après RLE sont trois populations différentes. À q4, sur une antichaîne
de cellules témoins disjointes, poser `N=sum_i |C_i|`, puis pondérer chaque
rectangle par la
masse u128 `|A||B|*binom(N,2)`, tirer un indice combinadique uniforme dans
`binom(N,2)`, puis mapper les deux offsets par les préfixes de populations vers
les cellules. Cela échantillonne exactement les paires non ordonnées, diagonale
comprise, sans matérialiser `Sym2`. Les endpoints sont ensuite tirés dans `A,B` ;
distinct-ID, owner avec vraie `EdgeKey`, indépendance affine et positivité sont
des rejets explicites. Un rejet conditionnel conserve l'uniformité des
`SupportKey` survivants seulement si l'univers brut et le cumul sont exacts,
les tirages dans `A` et `B` indépendants et uniformes, et chaque support owner
possède exactement un représentant brut. Une loi uniforme sur les `BallKey`
exige le RLE puis une nouvelle pondération.

Les cumuls et le générateur uniforme restent u128 ; un `double` n'est pas une
CDF exacte pour une masse au-delà de `2^53`. Le rapport publie seed, digest,
masse brute, rejets par strate, taille effective et intervalle statistique. Le
census de chaque échantillon coûte `O(n)` sans index reçu : le cap porte donc
sur `samples*n`, calculé en u128 avant le premier tirage. Le mode exige
explicitement `ordre=4`, une seule taille, une famille/seed/digest publiés et
refuse toute option sans effet. Ce sampler falsifie une hypothèse de densité ;
il ne prouve ni complétude de la source, ni exactitude produit, ni SLO.

On peut même pondérer directement par la masse injective. Sous `A intersect
B=empty`, soit `W=disjoint_union_i C_i`, `N=|W|`, `a=|A|`, `b=|B|`,
`p=|A intersect W|` et `q=|B intersect W|`. La masse non ordonnée exacte de
quatre IDs distincts vaut :

```text
(a-p)*(b-q)*binom(N,2)
+ (p*(b-q)+(a-p)*q)*binom(N-1,2)
+ p*q*binom(N-2,2)
```

La convention est `binom(m,2)=0` pour `m<2`, testée avant toute soustraction
u128. L'intersection de deux spans Morton est `O(1)`, mais sommer `A intersect
W` et `B intersect W` sur `k` cellules coûte `O(k)` sans index, `O(log k)`
après tri et préfixes, et `O(1)` seulement avec une table de populations dédiée.
Cette identité permet un tirage stratifié u128 sans rejeter les collisions avec
les endpoints ; elle ne remplace ni l'owner, ni la positivité. Une proposition
ordonnée avec remise peut aussi être uniforme après rejets, mais seulement si
chaque **point** de `W`, et non chaque bloc, a la même probabilité et si chaque
support survivant possède exactement ses deux ordres.

Un oracle borné des **supports réellement retenus** applique une chaîne plus
forte que `orientation!=0` puis `I<=7` : quatre vrais `PointId`, indépendance
`Delta>0`, quatre poids Gram strictement positifs, `BallForm -> BallKey`, puis
RLE des sphères égales et un census par `BallKey`. L'énumération globale
`i<j<k<l` visite déjà chaque ensemble une fois et n'a pas besoin d'un owner ;
l'owner total redevient obligatoire pour juger ou rejoindre `OwnedCK`, attribuer
l'arête génératrice ou mesurer une route ancrée. La pertinence
du support q4 teste `|I_B|+4<=11`. Séparément, le census publie tout le shell
`U_B`, le rang fermé `|I_B|+|U_B|` et la disposition régulière, plateau ou
unsupported ; un extra-shell ne supprime pas silencieusement un support
pertinent. Sans positivité, un tétraèdre dont la miniboule est portée par une
face ou une arête est faussement compté q4 ; sans cette séparation de statuts,
une cosphère fait confondre `binom(n,4)` supports incidents et événements
réguliers.

La localité se mesure après cette chaîne. Le rayon exact est
`R^2=(ell^T adj(G) ell)/(4 Delta)`, pas la plus grande arête ; une distance de
plus proche voisin nulle sous positions dupliquées rend tout ratio local
indéfini. Après positivité seulement, si `D` est la plus grande arête, alors
`D<=2R` et `R<=D` : `D/sqrt(esp)` est au mieux un proxy à facteur deux de
`R/sqrt(esp)`, jamais le circumrayon calculé. Un rang kNN porte une clé totale
`(distance^2,PointId)` et doit nommer la relation qu'il cherche à capturer :
source ancrée, owner dirigé, clique symétrisée-OR ou clique mutuelle. Avec le
rang zéro-based `rho_u(v)`, une source ancrée exige
`min_u max_{v!=u} rho_u(v)<k`, la clique OR exige
`max_{u<v} min(rho_u(v),rho_v(u))<k`, et la clique mutuelle remplace ce dernier
`min` par `max`. Le maximum des douze rangs dirigés ne teste que cette dernière
route ; il n'est ni nécessaire pour une route owner/ancrée, ni une preuve
qu'aucune autre construction locale n'est exacte.

Cet oracle reste `small-n`. Le preflight évalue en u128
`binom(n,4)*(n-4)` pour le census et le pire travail additionnel de rang avant
de lancer la boucle. Le seul cap `n<=260` autoriserait encore jusqu'à
`47 627 157 760` appels in-sphere, puis environ `580 455 985 200`
itérations de boucle de rang, dont `575 990 939 160` comparaisons de distances
si tous les quadruplets survivent ; il ne constitue pas un
plafond industriel.

Pour le même rectangle owner `R=(A,B)`, réutiliser les cellules carrier de
la fenêtre `2B_R`--lentille et former paresseusement des couples non ordonnés
`(C,D)`. Si `C!=D`, le bloc candidat porte `A×B×C×D`; si `C=D`, il porte
`A×B×binom(C,2)` sans répéter les deux IDs témoins. Tout q4 dont `ab` est
l'arête maximale possède ses deux autres sommets dans cette fenêtre, donc un
unique routage candidat. Le nom `OwnedCK-WST4` n'est acquis qu'après
injectivité, owner parmi six arêtes, orientation non nulle et positivité des
quatre poids. Pour `0<eta<=1`, le nombre de blocs initiaux avant
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

Un tétraèdre q4 positif possède au moins un `Q4Seed3` aigu adjacent à son arête
maximale. À l'intérieur de `Lane4`, le plus petit `PointId` parmi ces préfixes
choisit la provenance primaire et l'autre point devient le quatrième rôle. Ce
record ternaire est produit par `Lane4` et n'a aucun statut q3.
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

La même règle se factorise sans former le carré des blocs. Pour un rectangle,
partitionner les cellules en `A`, celles dont `Hmin<0` permet encore un carrier
aigu, et `N`, celles dont `Hmin>=0` prouve `NONE_ACUTE`. Au niveau de cette
relaxation, les couples possibles sont exactement :

```text
Q4CandidateCellPairs = Sym2(A) disjoint_union Cross(A,N)
```

`Sym2(A)` conserve sa diagonale `binom(C,2)` ; `Cross(A,N)` est orienté de
façon unique par les deux classes. Un nœud `N` disparaît seulement comme
carrier, jamais comme apex. Ces deux termes restent des descripteurs de produit,
pas des buffers ni une source q4 déjà owner/positive.

La liste WST3 elle-même peut rester virtuelle. Un
`LensDescriptor(RectId,root)` descend le BVH témoin : hors lentille disparaît,
`NONE_ACUTE` rejoint seulement le pool apex, et un carrier possible appelle
d'abord `FaceAxisJungDepth8Block`. Si ce reçu ferme toutes ses complétions,
aucun apex n'est ouvert ; sinon une tâche dual-tree
`(CarrierNode,ApexRoot)` descend seulement le produit résiduel avant
owner/distinct-ID/positivité/Corner8. Le pire cas n'est pas borné par ce contrat,
mais l'ordonnance attaque le coût avant matérialisation.

La fixture de tie
`p0=(0,0,0)`, `p1=(0,1,1)`, `p2=(1,0,1)`, `p3=(1,1,0)`
a ses six distances au carré égales à deux, son centre en
`(1/2,1/2,1/2)` et quatre poids `1/4`. Avec les IDs `0<1<2<3`, l'owner q4
est exactement `EdgeKey(0,1)`, le `CellPair` porte `{2,3}` et le primary de la
sweep vaut `2`. Elle tue les owners non déterministes, l'émission depuis les
deux carriers et la coexistence d'un parent avec ses enfants.

Le preflight `M4_apex` n'a pas besoin de développer carrier × apex. Pour une
arête owner `e={a,b}`, exclure `a,b` et tout site avec
`(b-a) cross (z-a)=0` avant l'acuité. Une arête distincte `f` ne bat pas `e`
exactement si `D_f<D_e`, ou si `D_f=D_e` et `EdgeKey(e)<EdgeKey(f)`. Définir
`V_e` par les deux arêtes endpoint qui ne battent pas `e`, `A_e` par l'acuité
stricte et `N_e=V_e minus A_e`. Cette exclusion collinéaire est indispensable :
un site entre `a,b` peut vérifier l'acuité scalaire tout en ne portant aucune
face.

Si `E_e(S)` compte les paires de `S` dont l'arête ne bat pas `e`, alors
`E_e(V_e)-E_e(N_e)` compte exactement les paires incidentes à au moins un
carrier aigu. Grouper par la direction projective
`(b-a) cross (z-a)`, calculée en i64, divisée par le pgcd absolu de ses composantes puis signée
avec sa première composante non nulle positive, et soustraire le même count
dans chaque classe `pi` retire exactement l'orientation nulle. Poser
`V_{e,pi}=V_e intersect pi` et `N_{e,pi}=N_e intersect pi` :

```text
M4_e = E_e(V_e)-E_e(N_e)
       - sum_pi (E_e(V_e,pi)-E_e(N_e,pi))
```

Chaque `E_e` est un range-count dual-tree/cell-pair `ALL/NONE/MIXED`, avec
tie-break `EdgeKey` exact. L'identité donne un count et des bornes avant fill,
pas les barycentriques, le rang ou les BallKeys.

Comme checksum global, le premier terme
`Q_aff=sum_e[E_e(V_e)-sum_pi E_e(V_{e,pi})]` doit égaler le nombre de
4-sous-ensembles affine-indépendants : `e` est leur dernière arête dans l'ordre
owner. Puis
`M4=Q_aff-sum_e[E_e(N_e)-sum_pi E_e(N_{e,pi})]`. Calculer en parallèle les
vues `GOOD` directe et `AFFINE minus BOTH_NONACUTE` mord les erreurs de tie,
de `PlaneKey` et de diagonale.

Cette identité reste un **microkernel**, jamais un RLE global par arête. À
`n=50000`, il existe `1249975000` arêtes et
`62496250050000` incidences `(e,z)` potentielles : 10 Go à huit octets par
record pour les premières et environ 1 Po à seize octets pour les secondes. Le chemin industriel
compte donc directement les atomes WST4 et n'instancie `PlaneKey_e` que lorsque
le facteur endpoint est singleton ou explicitement capé.

La masse de quatre `PointId` distincts d'un atome se calcule sans expansion par
Möbius sur les quinze partitions de ses quatre rôles. Pour
`S0=A,S1=B,S2=C,S3=D` :

```text
inj(S0,S1,S2,S3)
 = sum_partitions pi product_blocks T in pi
     [(-1)^(|T|-1) * (|T|-1)! * |intersection_{i in T} Si|]
```

Si `C=D`, diviser cette masse injective par deux ; si `C<D`, ne pas diviser.
Sur des nœuds Morton laminaires, chaque intersection est `O(1)` ; sinon le
bloc se scinde jusqu'à une partition commune. Cette formule traite ensemble
toutes les diagonales `A/C,A/D,B/C,B/D`, que des corrections indépendantes
peuvent mal compter.

La forme développée directement implémentable, avec `n_T` la taille de
l'intersection des facteurs indexés par `T`, est :

```text
inj4 = n0*n1*n2*n3
       - sum_pairs(ij) n_ij*n_k*n_l
       + n_01*n_23+n_02*n_13+n_03*n_12
       + 2*sum_triples(ijk) n_ijk*n_l
       - 6*n_0123
```

Ne jamais soustraire dans un type non signé au fil de la formule. Accumuler en
u128 contrôlé
`pos=n0*n1*n2*n3+sum_pairings(n_ij*n_kl)+2*sum_triples(n_ijk*n_l)` et
`neg=sum_pairs(n_ij*n_k*n_l)+6*n_0123`, exiger `pos>=neg`, puis poser
`inj4=pos-neg`. Si `C=D`, exiger aussi `inj4%2==0` avant la division par deux.
Lorsque l'endpoint est singleton ou capé et que la
relation `xy` est uniformément admissible, le microkernel d'une arête fixe
compte directement l'OR aigu. Pour deux cellules distinctes,
`GOOD(C,D)=a_C*v_D+n_C*a_D`; sur la diagonale,
`GOOD(C,C)=choose2(v_C)-choose2(n_C)=a_C*n_C+choose2(a_C)`. Retrancher les
mêmes quantités **localement** par `PlaneKey`, jamais après agrégation globale.

Initialiser `M4_pending` à la somme des masses des atomes non décidés. Pour un
atome de masse `m`, `ALL_Q` effectue `M4_pending-=m; M4_L+=m`, `NONE_Q`
effectue `M4_pending-=m`, et `MIXED_Q` remplace atomiquement le parent par la
partition complète de ses enfants, donc conserve `M4_pending`. Poser
`M4_U=M4_L+M4_pending`. `M4_L>B_fill` rejette exactement le moteur ponctuel qui
matérialiserait M4, pas la route factorisée profondeur/shallow.
`M4_U<=B_fill` certifie seulement la capacité du fill ; count exact, offsets et
publication exigent encore `M4_pending=0`. Au cap, rendre une continuation ou
passer au shallow. Les
termes soustraits de l'identité par arête ne sont **jamais saturés
séparément** : `E(V)=B+2` et `E(N)=B+1`, tous deux rabattus à `B+1`,
donneraient zéro au lieu d'un. Fusionner la contribution positive
`acute(x) OR acute(y), det!=0` au niveau `CellPair`, ou calculer les termes
complets en u128 avant soustraction.

Garder deux ledgers : `M4_raw_[L,U]` décrit owner, OR aigu et orientation avant
profondeur ; `residual_output_[L,U]` décrit ce qui reste matériellement à
émettre après profondeur/positivité. Un bloc fermé par profondeur conserve sa
masse dans `M4_raw`, crédite `domain_mass_closed` et émet zéro sortie
résiduelle. Il ne vaut jamais `M4_raw=0`.

La transition GPU est un vrai `count--scan--fill`. Le `count` écrit un
`DecisionTape` immuable contenant `BlockId/ParentId`, quatre `NodeKey`,
lane/vue, masse, décision, reçu, règle de split ou clés enfants, nombres
d'enfants/sorties, offsets et digest. Le scan préflighte le type des offsets et
réserve exactement les deux flux ; le fill consomme le tape sans recalcul de
prédicat. Exiger `sum(child_mass)=parent_mass`, `planned=filled=consumed`,
disparition atomique de tout parent splitté et pending de chaque ledger nul
avant publication.

Le CPU/oracle garde u128. Sur device, utiliser soit `Mass128{lo,hi}` avec
addition/carry/scan déterministes, soit le contrat prouvé `n<=50000` :
`6*choose(n,4)<2^61` et `n^4<2^63`, avec preflight avant cast vers `uint64_t`.

La construction ne forme pas aveuglément tous les couples `(C,D)`. Elle
classe d'abord `CarrierBlock(A,B,C)` en `ALL_ACUTE/NONE_ACUTE/MIXED`, puis
conserve symboliquement `Sym2(A) disjoint_union Cross(A,N)` avec
`A={Hmin<0}` et `N={Hmin>=0}`. `N` est éliminé comme carrier mais reste dans le
pool apex. La famille u16 à deux droites doit ainsi produire zéro face aiguë et
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
174 bits : employer trois limbs/BigInt, pas i128 pour ce **filtre de bloc**. Les
égalités, `B_y=0`, `A_y=0` et `A_y=2B_y^2` sont des fixtures obligatoires.

Au support ponctuel complet, une forme homogène plus courte évite ce chemin
large. Poser `v1=b-a`, `v2=x-a`, `v3=y-a`, puis :

```text
T = det3(v1,v2,v3)
h1 = ||v1||^2, h2 = ||v2||^2, h3 = ||v3||^2
Q = h1*(v2 cross v3)+h2*(v3 cross v1)+h3*(v1 cross v2)
```

Le circumcentre vérifie `c-a=Q/(2T)`. Les numérateurs barycentriques sont :

```text
L1 = det3(Q,v2,v3)
L2 = det3(v1,Q,v3)
L3 = det3(v1,v2,Q)
L0 = 2*T^2-L1-L2-L3
```

Le support est q4 positif si et seulement si `T!=0` et les quatre `Li` sont
strictement positifs ; ses poids valent `Li/(2*T^2)`. Avec `A=abs(T)` et
`Qbar=sign(T)*Q`, la puissance normalisée est :

```text
P(z) = A*||z-a||^2-Qbar dot (z-a)
BallForm = (A, -(2*A*a+Qbar), A*||a||^2+a dot Qbar)
```

`P(z)<0` signifie intérieur strict. Sous u16, `|T|<2^51`, les composantes de
`Q` restent sous `2^69`, les `Li` sous `2^106` et les coefficients/pouvoirs sous
`2^87`; i128 signé ou deux limbs suffisent avec preflight avant toute opération.
Cette largeur doit être vérifiée par une autorité BigInt, pas seulement affirmée
par le sujet. Fixtures permanentes : tétraèdre régulier avec
`L=(2,2,2,2)`, support non positif
`(0,0,0),(4,0,0),(2,3,0),(2,0,1)` avec
`L=(320,320,80,-432)`, permutation/orientation inversée, shell et extrema u16.
Cette forme est l'autorité terminale candidate pour `0A`; elle ne transforme pas
un produit de boîtes `MIXED` en `ALL` et ne remplace pas le filtre large de bloc.

Avant de matérialiser les apex, exploiter la même droite comme certificateur de
profondeur. Son intersection avec `K_4(ab)` est un segment fermé `J_f`. Avec
`c_0=a+W/(2G)`, `m=(a+b)/2` et `h_e=(b-a)/2`, l'identité
`W-Gd=(E-F)*(D*u-F*d)` donne exactement, pour q4 :

```text
T2 = D*(G-2*(E-F)^2)
J_f = {tau : 2*tau^2<=T2}
```

Si `T2<0`, le domaine est vide. Si `T2=0`, le seul centre est dans le plan de
la face : le poids de tout apex vaut zéro, donc la lane q4 strictement positive
est vide elle aussi. Le verdict de mort q4 est ainsi `T2<=0`, pas seulement
`T2<0`. Pour q3, remplacer par
`3*tau^2<=D*(G-3*(E-F)^2)`. Pour chaque témoin, poser
`A_z=G||z-a||^2-W dot (z-a)` et
`B_z=n dot (z-a)` ; l'absence d'intérieur est le demi-intervalle
`A_z-tau*B_z>=0`. Un groupe couvre toute la face si :

```text
J_f intersect intersection_z {tau : A_z-tau*B_z>=0} = empty
```

Helly avec tolérance en dimension un donne ici un reçu constructif, plus simple
que le DAG leave-out. Pour un seuil cible `r`, classer d'abord les témoins qui
sont intérieurs sur tout `J_f` ; en conserver
`p=min(r,n_permanents)`. Chaque autre témoin utile est une demi-droite ouverte
`tau<alpha` ou `tau>beta`. Poser `k=r-p`, garder les `k`
plus grands `alpha` et les `k` plus petits `beta`, avec leurs `PointId`.

Pour tout `tau`, le nombre de demi-droites gauches retenues vaut
`min(k,n_gauches(tau))`, et de même à droite. Si la famille complète a
profondeur au moins `r`, leur somme vaut donc au moins `k` partout. La
réciproque est immédiate puisqu'il s'agit d'un sous-pool. Le contrat exact est
donc `Depth(kernel)>=r iff Depth(pool)>=r`, pas l'égalité de leurs profondeurs
numériques. Ainsi un `AxisToleranceKernel-r` contient au plus
`p+2k=2r-p` IDs : **16 pour q4**, **18 pour q3**, et souvent moins.

Un seul scan top-k suivi du replay des deux bouts de `J_f` et des seuils groupés
décide donc exactement le seuil de profondeur d'une face fixe en `O(n)` avec
`O(r)` mémoire. L'égalité est shell et ne crédite rien. Les bouts de `J_f`
étant généralement irrationnels, les comparaisons emploient signe puis carré
exact ; `B_z=0` conserve les trois cas constant intérieur/shell/extérieur.
Sous u16, l'ordre de deux seuils demande environ 155 bits, mais les bouts
exigent davantage : `T2<2^102`, `|A_z|<2^103`, `|B_z|<2^51`, et le test
`2*A_z^2<=T2*B_z^2` monte sous environ 207 bits. Employer i256/quatre limbs ;
la largeur du lift uniforme `A×B×C` reste une gate distincte. Cette forme est
la spécialisation constructive de `eta(2,r)=2r`.

L'implémentation peut éviter un replay ambigu en écrivant le test de gap
directement. Trier `alpha_1>=...` et `beta_1<=...`, compléter par les sentinelles
`alpha_t=-inf` et `beta_t=+inf`, puis, pour chaque `j=0,...,k-1`, exiger
`max(ell,alpha_{j+1})>min(u,beta_{k-j})`. Une inégalité non satisfaite exhibe un
`tau` de `J_f` avec au plus `k-1` intérieurs non permanents. La comparaison est
strictement `>` : à l'égalité, les événements sont shell. La fixture q4
strictement aiguë prend `a=(220,440,440)`, `b=(660,440,440)` et
`x=(440,682,440)`. L'arête owner `ab` a le carré de longueur `193600`, contre
`106964` pour les deux autres ; les trois produits angulaires valent
`96800,96800,10164`, tous strictement positifs. Ajouter les sept IDs permanents
`(275+55*j,440,440)`, `j=0,...,6`, puis `z+=(440,461,661)` et
`z-=(440,461,219)`. Le centre axial `c0=(440,461,440)` a
`R^2=48841=221^2` : les deux derniers témoins sont shell au même événement et
la profondeur minimale vaut exactement sept. Cette fixture tue `>=`, l'oubli
des événements égaux et un test limité aux extrémités sans sortir du domaine
face owner aiguë.

Un `FaceAxisJungDepth8Block` propose ce petit noyau sur un représentant, puis
vérifie orientations, signes, ordre des seuils et marges sur tout `A×B×C`.
`ALL` s'hérite. Une égalité uniformément prouvée est groupée et rejouée comme
shell ; seul un ordre dont l'intervalle peut s'inverser ou une borne indécise
scinde fail-open. Un LBVH/range-extrema peut proposer directement les top-k sans
matérialiser les faces ou les apex.

### 7.2 `Q4SeedAxisExtremalCompletion-r4` : noyau interne de `Lane4`

Ce théorème n'établit aucune dépendance envers q3 : son entrée est un
`Q4Seed3` produit dans `Lane4`. Poser `r4=smax-3`, le premier nombre
d'intérieurs qui rejette un support q4 ; à `smax=11`, `r4=8`.

Pour un `Q4Seed3` exact, masquer ses trois IDs et noter `p` le nombre de sites strictement
intérieurs sur **tout** `J_f`. Cela comprend `B_z=0,A_z<0`, mais aussi les
racines entrantes strictement avant le bout gauche et les racines sortantes
strictement après le bout droit. Le reçu conserve leurs vrais `PointId`. Les
sites `B_z=0,A_z=0` forment séparément le shell persistant. Si `p>=r4`, aucune
complétion q4 pertinente n'existe. Sinon poser `k=r4-p` et ne sélectionner que
parmi les racines non permanentes rencontrant `J_f`.

Pour `B_z!=0`, écrire `rho_z=A_z/B_z`. Un site de signe `B_z>0` entre dans la
boule lorsque `tau` franchit `rho_z` vers la droite ; un site de signe
`B_z<0` en sort lorsque `tau` franchit `rho_z` vers la droite. Conserver :

```text
First_k = k plus petites racines parmi B_z>0
Last_k  = k plus grandes racines parmi B_z<0
```

Tout quatrième point régulier dont la circumsphère contient moins de `r4`
intérieurs est
dans `First_k union Last_k`. En effet, un apex positif absent de `First_k`
possède déjà `k` entrants strictement antérieurs ; avec les `p` permanents, sa
profondeur est au moins `r4`. Le cas négatif est symétrique avec les sortants
strictement postérieurs. Ainsi :

```text
candidate_root_groups <= 2*(r4-p) <= 2*(smax-3)
```

Au profil courant, cette borne vaut `2*(8-p)<=16`. La généralisation ne change
donc ni le noyau top-8 ni ses fixtures ; elle explicite seulement le paramètre
de contrat et élimine l'ambiguïté avec le seuil q3 `smax-2`.

Pour une arête owner fixée, noter `m` le nombre de lignes géométriques définies
par **tous** les sites non coplanaires admissibles, pas seulement les faces
aiguës que le générateur orientera. La somme sur ces lignes est au plus `16m`
incidences shallow. Chaque centre d'intersection est incident à au moins deux
lignes, donc il existe au plus `8m` centres géométriques de profondeur au plus
sept. En revanche, si `m_acute` ne compte que les faces aiguës effectivement
parcourues, la borne directe des propositions est `16m_acute`; il serait faux
de la diviser par deux lorsqu'un q4 n'a qu'une face aiguë adjacente à l'owner.
Ces bornes sont déterministes, sans ordre aléatoire et sans Delaunay. Sous
concurrence de lignes, elles portent encore sur les centres et groupes ; la
masse de `SupportKey` peut être grande et suit la politique fail-closed de
dégénérescence.

Ce majorant porte sur des groupes d'égalité complets. À une racine shallow,
tous les intérieurs sont aussi dans le noyau : un intérieur omis impliquerait
déjà `k` extrêmes intérieurs et contredirait la profondeur inférieure à huit.
Le replay fournit donc exactement :

```text
I_B = permanents
      union First_k strictement avant la racine
      union Last_k strictement après la racine
U_B = face union groupe d'égalité de la racine union shell persistant
```

Sous `RelevantGP`, le groupe contient un seul apex et le shell persistant est
vide. Toute autre situation rend `unsupported_degeneracy`; elle n'autorise ni
troncature, ni choix arbitraire d'un représentant. Le range selection remplace
donc à la fois le produit carrier×apex et le second census q4. La `BallKey`
canonique et le RLE restent exigés pour l'identité et la provenance.

La capacité porte sur des IDs, pas sur les seuls groupes. Si chaque côté garde
au plus `kCapRacines` IDs et le shell persistant au plus `kCapShell`, le buffer
de replay doit contenir au moins `3+2*kCapRacines+kCapShell` IDs ou rendre avant
toute consommation un fate `PENDING_CAP` avec le compte requis. La fixture
owner aiguë `(96,108,100),(108,96,100),(92,96,100)`, complétée par 49 IDs en
`(100,100,110)` et 48 en `(100,100,92)`, exige exactement 100 IDs de shell et
tue tout buffer de 99 silencieux.

Le domaine du replay fait partie du théorème. Les trois IDs du `Q4Seed3` sont
distincts ; chaque autre `PointId` apparaît exactement une fois et hors du
seed ; les ledgers permanents, shell, entrants et sortants sont deux à deux
disjoints. La sélection doit être `OUVERT`, l'apex doit apparaître exactement
une fois parmi les racines retenues et son compte intérieur tronqué doit être
strictement inférieur à `r4`. S'il atteint `r4`, le fate est `DEEP` ou
`HORS_DOMAINE` et aucune liste n'est publiée. S'il est inférieur à `r4`, tout
intérieur et tout co-shell omis aurait déjà forcé `r4` extrêmes meilleurs : les
IDs reconstruits sont donc complets. Une répétition d'identité est
`HORS_DOMAINE`, tandis que plusieurs identités distinctes co-shell relèvent de
la politique déclarée `RelevantGP` ou `Plateau`.

Tout tétraèdre q4 positif choisit d'abord son arête owner, puis le plus petit
vrai `PointId` parmi ses deux `Q4Seed3` aigus comme provenance primaire. Cette
règle donne un seul préfixe générateur à l'intérieur de `Lane4`. La voie q4
active est donc :

```text
Q4Seed3Block owner/aigu
 -> Q4SeedAxisTopR4
 -> au plus 2*(smax-3) groupes de racines
 -> distinct-ID4/owner6/carrier primaire/positivité
 -> BallKey(I_B,U_B)
```

Aucun `CellPair`, `Sym2` ou record carrier×apex n'est autorisé sur cette voie.
Pour chaque lot de préfixes q4 ouverts, exiger
`candidate_root_groups<=sum_f 2*(r4-p_f)` et `apex_pair_records=0`.

Pour une face fixe, `A_z` est convexe séparable en `z` et `B_z` linéaire. Une
recherche best-first sur l'octree peut donc sélectionner les extrêmes par
bornes rationnelles sûres, en descendant tout nœud de signe `B` mixte ou dont
l'intervalle chevauche le cutoff. Les égalités sont toujours descendues ou
range-reportées jusqu'au groupe complet.

Cette exactitude ne se transfère pas automatiquement à un `FaceBlock` où
`a,b,x` varient. `G,W,n,T2` et les comparaisons croisées de racines sont des
polynômes de degré supérieur, ni multiaffines ni convexes dans tous les
facteurs. Tous les coins d'un produit ayant le même ordre ne prouvent donc pas
`ALL`. Une extension d'intervalles/Bernstein reçue, ou un microtile exhaustif,
doit vérifier le classement ; sinon le bloc splitte fail-open.

Une fixture u16 concrète tue `corners_order_implies_all`. Après translation par
`(10,10,10)`, prendre `a=(10,10,10)`, `b=(24,10,10)`, les quatre coins carrier
`(13,16,10),(13,18,10),(21,16,10),(21,18,10)`, le vrai carrier intérieur
`(16,17,10)` et les témoins `z1=(17,16,16)`, `z2=(13,5,15)`. Tous les carriers
sont aigus, `ab` reste owner et `T2>0`. L'ordre des deux racines est le même aux
quatre coins, mais s'inverse strictement au carrier intérieur.

La borne seize est sharp dans le profil u16. Prendre
`p0=(125,100,100)`, `p1=(93,124,100)`, `p2=(93,76,100)`, puis
`y_h=(100,100,100+h)` pour
`h=-33,...,-26,26,...,33`. Les seize tétraèdres ont tous l'owner unique
`p1p2`, le carrier primaire `p0`, des poids strictement positifs, aucun
extra-shell et des profondeurs `7,...,0,0,...,7`. Un cap quinze perd donc un
support régulier. Avec la même face et `h=-24,...,-17,17,...,24`, les seize
témoins rendent la profondeur minimale exactement huit et chacun est
nécessaire : la taille `2r=16` du certificat de mort est elle aussi sharp.

Le contrat indépendant des trois lanes et les précautions de vocabulaire sont
dans
[`audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).

### 7.3 Ancien consommateur WST4, diagnostic seulement

Les certificats ci-dessous restent sûrs pour juger le probe historique ou
comme preuves complémentaires sur un microtile `MIXED`. Ils ne réintroduisent
ni cellule apex, ni produit WST4 dans la route active de `Lane4`.

Dans le probe historique, après ajout d'une cellule apex,
`BlockBallDepth8(A,B,C,D)` restreint encore la famille de centres. Normaliser
d'abord les diagonales de `PointId` : CK garantit
`A/B`, ou les scinde ; la géométrie raffine ensuite toute intersection
`A/C,A/D,B/C,B/D,C/D` jusqu'à quatre spans disjoints. La formule de Möbius
corrige leur masse, pas leur domaine intervalle : une diagonale laissée dans un
bloc forcerait artificiellement `O` ou `J` à contenir zéro. Poser alors, pour un
support `S=(a,b,x,y)` et un témoin `z` :

```text
O(S)   = det3(b-a,x-a,y-a)
J(S,z) = det4((a-z,||a-z||^2),
              (b-z,||b-z||^2),
              (x-z,||x-z||^2),
              (y-z,||y-z||^2))
```

Avec cette convention, `z` est strictement intérieur si et seulement si
`O*J<0`. Ne jamais former ce produit en i128 ni multiplier deux intervalles
décorrélés. Sur le résiduel dont le signe d'orientation reste indécis, employer
la forme Gram corrélée `Phi=O*J` ci-dessus en i192/i256 ; ses huit coins restent
complets. Si un intervalle d'orientation sur
`A×B×C×D` exclut zéro, fixer `sigma=sign(O)`. Pour chaque support ponctuel,
le coefficient de `||z||^2` dans `sigma*J(S,z)` vaut `|O|>0` : cette fonction
est strictement convexe en `z`. Son maximum sur une AABB témoin `Z` est donc
atteint à l'un de ses huit coins.

Cela donne `Corner8BallDepth`, un vérificateur `ALL` entier et factorisé. Pour
chacun des huit coins `q` de `Z`, translater les quatre lignes support à `q` et
borner le déterminant 4×4 par ses 24 monômes de Leibniz sur
`A×B×C×D`. Si `sup J_q<0` lorsque `sigma=+1`, ou `inf J_q>0` lorsque
`sigma=-1`, aux huit coins, alors **tout** `Z` est intérieur de **tout** support
du bloc. Une borne indécise rend `MIXED`. Pour un support fixé, la condition aux
huit coins est nécessaire et suffisante sur l'enveloppe continue ; la perte du
classifieur de bloc vient seulement des intervalles sur les facteurs support.

Le certificat bisigne évite d'exiger un signe uniforme de `O`. Garder un ledger
`P` de spans dont les huit coins prouvent `J<0` et un ledger `N` de spans dont
les huit coins prouvent `J>0`. Après exclusion de `O=0`, fermer exactement la
voie concernée lorsque :

```text
O>0 uniforme  et |P|>=8
O<0 uniforme  et |N|>=8
O de signes possibles des deux côtés et |P|>=8 et |N|>=8
```

Chaque ledger est une antichaîne de vrais `PointId`, sans doublon et disjointe
des quatre facteurs support. Les deux ledgers peuvent se recouper entre eux,
car ils ne sont jamais additionnés pour un même support. Le parcours transporte
des tâches `(WitnessNode,active_sign_mask)` : il crédite seulement
`certified_mask & active_sign_mask`, puis descend le même nœud pour
`active_sign_mask & ~certified_mask`. Un succès pour le mauvais signe ne permet
donc jamais le `continue` global du nœud. Une lane arrivée à huit se désactive ;
cap ou pile saturée donne `PENDING`. Cette règle est nécessaire pour que le
bisigne reste mordant, même si son omission ne crée ici que des faux négatifs.

Pour une ligne issue de la boîte `p_i in [l_i,u_i]`, borner sa dernière
composante sans regarder seulement les coins :

```text
Q_L = sum_i (0 si q_i in [l_i,u_i], sinon min((l_i-q_i)^2,(u_i-q_i)^2))
Q_U = sum_i max((l_i-q_i)^2,(u_i-q_i)^2)
```

Ces bornes alimentent les 24 produits intervalles. Le minimum de norme peut
être intérieur à l'AABB ; l'évaluer aux seuls coins rendrait `ALL` non sûr.

La réciproque n'est pas disponible pour `NONE` : huit coins extérieurs peuvent
cacher un centre intérieur. Un `NONE_INTERIOR` optionnel utilise une BallForm
représentante normalisée `P0(r)=A0||r||^2+B0 dot r+C0`, `A0>0`, et des bornes
d'erreur `epsA,epsB_i,epsC`. Avec
`E=epsA*Qmax+sum_i epsB_i*maxabs(r_i)+epsC`, la tangente entière à `P0` en un
point `r0` donne un minorant affine séparable ; `min_Z tangent-E>=0` certifie
`NONE`, sinon le résultat reste `MIXED`. La contre-fixture « coins extérieurs,
centre intérieur » devient donc un mutant `corners-outside-implies-none`, pas
une interdiction du classifieur `ALL` par huit coins.

Sous u16, `|O|<6*65535^3<2^51`. Chaque monôme de `J` est au plus
`3*65535^5`, et la somme des valeurs absolues au plus
`72*65535^5<2^87`. Pour le `NONE` optionnel, choisir l'origine `o` dans le cube
u16 et des cofacteurs 4×4 directs, sans dénominateur : alors
`E<720*65535^5<2^90` et les opérations tangente/erreur restent sous `2^91`.
i128 suffit lorsque `O` et `J` sont jugés séparément.
Un test `(F4Block,WitnessNode)` paie au plus 192 monômes de déterminant, plus
l'orientation, avec early exit ; il n'énumère aucun quadruplet.

Le standalone de `89774d0`, inchangé au snapshot courant, reçoit ce lemme `ALL`
mais pas son raccord événementiel. Son ABI ne préflighte ni domaine u16, ni
boîtes valides, ni quatre IDs distincts, ni positivité, owner, shell ou
`BallKey`. Son oracle exhaustif réemploie les prédicats ponctuels du sujet ;
trois portes nominales sont à regex et `--selftest=1` imprime `accord=OUI` avant
un code `3`. Les mutants `drop-corner` et `norme-aux-coins` restent hors porte,
le mutant produit `O*J` est déclaré mais absent, et celui nommé
`corners-outside-implies-none` produit en réalité un faux `ALL`. Les `6/6`
verts ciblés reçoivent donc une campagne bornée, pas le support q4 complet.

Avant les recertifications owner/positivité, un succès publie
`domain_mass_closed`. La fixture cartésienne prouve par contre-calcul externe
que ses 4096 supports sont positifs, mais le probe ne porte pas ces poids ni les
vrais IDs. Une promotion industrielle se juge contre une autorité
BigInt/rationnelle indépendante et compte les opérations : huit coins demandent
des déterminants intervalles, pas huit tests scalaires.

Sur G4, un CTA traite un couple `(F4Block,WitnessNode)` : huit groupes de 24
monômes produisent les intervalles des coins, puis une réduction de signe rend
`ALL/MIXED`. Le count conserve le reçu et crédite la population ; scan/fill ne
réexécute pas ces déterminants. La gate physique publie couples testés, monômes
larges, early exits, visites de nœuds témoins, octets et HWM.

Le parcours BVH utilise une antichaîne de `WitnessNode` disjoints des quatre
spans support. Un nœud `ALL_INTERIOR` crédite toute sa population ; dès huit
vrais `PointId` distincts, le bloc q4 ferme et le reçu conserve huit IDs
canoniques. Un parent et ses enfants ne peuvent jamais être crédités ensemble.
Le cas symétrique `C=D` se raffine paresseusement par
`choose2(X)=choose2(L) disjoint_union (L×R) disjoint_union choose2(R)` avant
le test d'orientation ; les quatre intersections endpoint--carrier/apex suivent
la même normalisation laminaire.

La hiérarchie suivante décrivait l'ancienne proposition WST4, désormais
diagnostique : Jung edge 2D, porte aiguë, Jung axe 1D,
`OwnedCK-WST4` broad-phase symbolique, normalisation distinct-ID et signe
d'orientation, puis `Corner8BallDepth/BlockBallDepth8` sur son produit.
Owner et acuité sont assez bon marché pour précéder normalement une traversée
témoin complète ; un essai `Corner8` sur quelques gros nœuds peut en revanche
précéder le prédicat barycentrique large. Si owner/positivité ne sont pas encore
décidés, nommer la télémétrie `domain_mass_closed`, pas `M4_closed`. Cette
ordonnance attaque la masse avant toute expansion ; une borne linéaire sur le
nombre de blocs ne suffirait pas sans ce consommateur factorisé.

L'autorité q4 reste directe : quatre IDs distincts, orientation non nulle,
owner parmi six arêtes et quatre numérateurs barycentriques du circumcentre
strictement positifs. « Quatre faces aiguës » n'est ni nécessaire ni suffisant.
Après `JungDiskDepth8`, seules les paires singleton ou microtiles effectivement
fermés évitent leurs couples de cellules. Un rectangle CK ne les évite que si
un futur classifieur uniforme le prouve. Le `count` et le preflight précèdent
impérativement le `fill`.

Le successeur candidat est propre à `Lane4` : son moteur local du plan
médiateur interroge les intersections shallow de lignes de ses `Q4Seed3`. Il ne construit aucun
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
symbolique dans ce diagnostic. Cela n'est plus une ordonnance active.

Le sampler v2 du HEAD ne fournit pas encore une estimation reçue : sa borne
Hoeffding est correcte sous i.i.d. uniforme, mais le mapping multiply-high reste
sans rejet, les streams n'ont pas de contrat d'indépendance, `W4` n'a pas
d'intervalle et le contrôle ne juge pas le décodeur rang--`PairId` dans un mode
exhaustif déterministe. La vue SOC reste absente.
Son option `--rang` peut réussir sans lancer le sampler, ignore les extra-shells
et conditionne le tirage sans les poids nécessaires. Le brute-force q4 reçoit
une énumération bornée, mais recopie les prédicats du sujet et appelle à tort
`H4` le seul test `I<=7`. Aucun de ces chiffres ne justifie une rampe G4 50k.

Le mur avant rang est néanmoins réel sur une famille ouverte : le support
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)`, `y=(31,19,31)` a owner `ab`
unique, deux faces aiguës et poids q4 `(47,3,55,55)/160`. Ses inégalités
strictes persistent sur un voisinage réel, ce qui prouve qu'une masse logique
quartique peut exister avant rang. L'ancienne phrase « quatre petits
sous-cubes » ne constituait toutefois pas une fixture u16 : sur les cubes
unitaires non mis à l'échelle, seuls `2093/4096` supports passent et un cas est
shell.

La fixture de réception entière utilise au contraire les cinq nœuds de huit
points suivants :

```text
A = (20000,20000,20000)+{0,1}^3
B = (30000,30000,30000)+{0,1}^3
C = (19000,31000,31000)+{0,1}^3
D = (31000,19000,31000)+{0,1}^3
Z = (20000,20000,30000)+{0,1}^3
```

Donner à ces cinq spans quarante `PointId` distincts dans un même
`cloud_epoch`. L'oracle exact sur les `8^4=4096` supports donne owner `AB`, une
marge minimale des distances carrées de `11892000` et une plus petite
barycentrique `13217143/721310286>0`. Séparément, le vérificateur intervalle
sur les cinq AABB continues donne l'orientation
`[1438694087994,1441306088006]` et une pire borne supérieure de `J` aux huit
coins de `Z` égale à `-79011820908103787995`. Les huit IDs de `Z` sont donc
intérieurs de chaque support ; le futur classifieur doit fermer ce bloc avant
fill. Cette fixture tue l'expansion `8^4`, l'erreur de signe et la perte d'un
coin témoin. La contre-fixture `[1,3]^3` ci-dessus tue séparément le faux
`corners-outside-implies-none`.

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
3. Recevoir un manifeste `NoDelaunay` : split-tree Morton, range queries et
   `NeutralPairPartition` exacte, immuable, avec vrais IDs, positions
   dupliquées et caps. Cette partition ne porte aucun statut de lane.
4. Lancer trois producteurs indépendants, chacun avec sa queue, son ledger et
   sa preuve de complétude : `Lane2` ferme sa boule diamétrale à dix intérieurs ;
   `Lane3` construit ses propres triples, applique distinct-ID3, owner3,
   acuité et rejette à neuf intérieurs ; `Lane4` construit ses propres
   `Q4Seed3Block`, puis applique `Q4SeedAxisTopR4`, owner6 et positivité q4,
   avec rejet à huit intérieurs. Aucune sortie d'une lane n'entre dans une
   autre.
5. Dans `Lane4` seulement, exiger zéro `CellPair/Sym2`, la borne
   `candidate_root_groups<=sum_f 2*(r4-p_f)` et le tie report complet. Le
   replay fournit directement `I_B/U_B` aux racines retenues.
6. Recevoir le lift `Q4Seed3Block` par intervalles/Bernstein ou microtiles contre
   l'oracle ponctuel. Les coins seuls sont un proposer. Employer Jung, SOC, LP,
   cages et Corner8 seulement comme preuves complémentaires sur les tâches
   `MIXED`, jamais comme condition de complétude de la source.
7. Porter les trois producteurs par tâches persistantes, SoA, best-first top-k,
   arithmétique i256 reçue, count--scan--fill et radix/RLE. Le fill consomme un
   `DecisionTape` sans recalculer les prédicats.
8. Exécuter les microgates `1500/3000/6000`; ne passer à
   `12500/25000/50000` que si tâches, groupes de racines, octets, HWM, sorties
   et pentes physiques passent, sans record carrier×apex.
9. Qualifier deux warmups puis trente répétitions chaudes à `50000` avec le
    payload officiel complet et une seule copie hôte incluse.

La piste de coût peut avancer en parallèle comme diagnostic `counter-only` sur
petit `n` : `NeutralPairPartition -> Lane4(Q4Seed3Block ->
Q4SeedAxisTopR4)`, toujours contre vérité exhaustive. Elle mesure les morts
`T2/permanent/gap`, les sélections extrémales,
les groupes d'égalité, les splits, les octets et la HWM. Elle n'autorise aucun
claim 0A/0B ou produit avant les portes 1--2 ci-dessus.

La recette G4 q4 reste défectueuse et ne doit pas être relancée en l'état. Son
parser collecte seulement les lignes de sweep mais compte aussi les neuf codes
exact-once, donc son cardinal d'accord est impossible ; il cherche des champs
du probe historique et décide avant de rapatrier les sorties rouges. Le reste
du timeout n'est pas recalculé avant chaque run, exact-once reste hors deadline
globale et le P0 d'identités du replay est encore ouvert.
La tentative précédente n'a exécuté aucun build : elle certifie seulement
l'arrêt ciblé `TERMINATED`.

Les campagnes, échecs, hashes et états GCP sont autoritaires uniquement dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) et les reçus.
Une campagne ne passe qu'avec tous les codes zéro, `pending=0`, payload
officiel, coûts conservés et arrêt ciblé certifié. Le no-go historique de la
configuration centrale sur huit amas ne réfute aucun certificateur corrélé.
Le SLO ne peut pas être qualifié sur `uniform` seule : le plan exige aussi le
mélange équilibré de huit amas.

La porte composée publie au minimum :

```text
E3/E4, max par ancre, CLOSED/OPEN/PENDING
F2, FaceBlocks q3, q3 shallow, T4_site/acute faces, primary accepts
dead_T2/dead_perm8/dead_gap, candidate_root_groups, root ties
W4_positive, H4_rank, N4_event, tâches, splits, visites axiales
BallKeys brutes/uniques, supports, census, shells
sortie H, opérations larges, octets/HWM
temps par phase et warm_e2e
commandes, seeds, HEAD, diff, hashes, codes de sortie
```

Une pente seule, un taux de fermeture, un `OK` CPU ou une extrapolation de
bande passante ne qualifie aucun SLO.

## 10. Fixtures permanentes prioritaires

- owner équilatéral/isocèle sous permutation indépendante stockage/PointId ;
- support minimal contre shell : quatre sites cosphériques dont la miniboule
  reste portée par une paire diamétrale ;
- q3 ambiant : témoin hors du plan mais intérieur à la boule canonique ;
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
  pairwise sans poids commun au rectangle ; pour `BlockJungDual64`, mutants
  `drop-corner`, `vary-weights-per-corner`, `accept-equality` et
  `narrow-before-widen` ;
- count factorisé `M4_e` contre expansion : tie `EdgeKey`, site collinéaire,
  deux carriers aigus, même `PlaneKey`, plans distincts et checksum
  `Q_aff=#4-ensembles affine-indépendants` ;
- Möbius `inj4` sur tous les motifs de recouvrement, parité de `C=D`,
  conservation parent--enfants, mutant `saturate-before-subtract` et ledgers
  `M4_raw/residual_output` indépendants ;
- Jung axe : événements opposés égaux, trois constantes `B=0`, noyau top-k
  contre sweep exhaustive, `T2<0/0/>0`, bouts 207 bits et mutants
  `wrong_extrema/drop_equal_shell` ;
- `Q4SeedAxisTopR4` génératif : fixture u16 de dix-neuf points donnant exactement
  seize q4 réguliers, deux à chaque profondeur zéro à sept, owner `EdgeKey(1,2)`
  et carrier primaire zéro ; mutants `cap15`, `first7/last7`, mauvais signe
  `B<0`, division tronquée et double émission du primary ;
- certificat axial sharp : même face avec seize témoins tous nécessaires pour
  une profondeur minimale huit ; mutant `endpoint_counts_imply_depth` contre
  la fixture seize-apex qui a les mêmes comptes aux bouts mais un gap central ;
- `FaceBlock` coins insuffisants : ordre des deux racines identique aux quatre
  coins carrier mais inversé au vrai point intérieur ; mutant
  `corners_order_implies_all` ;
- cinq nœuds u16 mis à l'échelle ci-dessus : `4096` supports q4, huit témoins
  uniformes et fermeture `Corner8BallDepth` avant fill ;
- Corner8 : huitième coin seul extérieur, minimum de norme intérieur à un
  facteur support, permutation, extrêmes u16, shell et oracle BigInt indépendant ;
- WST3/WST4 : inventaire de toutes les ancres non-owner, diagonales de spans,
  positions dupliquées avec multiplicité et tie-break par vrai `PointId` ;
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

Deux fixtures q4 exactes empêchent de confondre q2 et q4 ou de promouvoir un
cutoff kNN non prouvé. Pour la première :

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

La seconde réfute à la taille contractuelle une source kNN ancrée de petit
préfixe. Poser `c=(30000,30000,30000)`, `L=10000` et prendre les quatre signes
`S={(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)}` :

```text
v_s=c+L*s, s in S
z_(s,j)=c+(L+j)*s, s in S, j=1,...,12499
```

Cela donne exactement `4+4*12499=50000` IDs u16. Les quatre `v_s` forment un
tétraèdre régulier positif de centre `c`, de poids `1/4`, de rayon carré
`300000000` et d'arêtes carrées `800000000`. Chaque `z_(s,j)` est strictement
extérieur puisque sa distance carrée à `c` vaut `3*(L+j)^2`, mais il est à
distance carrée `3*j^2<800000000` de `v_s`. Chacun des quatre sommets a donc au
moins `12499` distracteurs strictement plus proches que chacun de ses trois
partenaires. Le support q4 reste vide, sans extra-shell, mais toute source
ancrée au préfixe `k<=12499` le perd. Cette fixture finie ne prétend pas une
impossibilité asymptotique sous le domaine u16 ; elle interdit de transformer
un petit cutoff observé en théorème. Un préfixe kNN peut proposer des témoins ou
une continuation, jamais fermer par son échec.

## 11. Non-claims

Il existe trois producteurs candidats GPU-factorisables en blocs, sans aucune
Delaunay : `{Lane2(Q2MidballDepth10), Lane3(Q3MiniballDepth9),
Lane4(Q4SeedAxisExtremalCompletion-r4)} -> BallKey/RLE`. Les accolades
désignent un fork indépendant, jamais une composition séquentielle. « Sparse » qualifie seulement les
enregistrements physiques restant factorisés. Il n'existe aucune garantie
universelle sous-quadratique pour la sortie explicite ou pour le raffinement
des frontières `MIXED`. La borne de seize apex par face retire le carré q4 ;
elle ne borne ni le nombre de faces primaires, ni une sortie q2/q3 lourde.

Le préfiltre combiné de la section 6bis ne fait pas exception. Ses trois
comptes sont prouvés disjoints et ses deux bornes sont exactes, mais il ferme
des **ancres**, pas des supports : `184` ancres q4 par point à
`n=8 000, s=8, K=10` ne sont pas `428` supports par point, et chaque ancre
survivante reste entièrement à instruire. Il ne rend donc pas le producteur
*output-sensitive*, et aucune de ses mesures ne vaut pour une famille dont la
campagne n'a pas été rendue — `eight_clusters` en particulier.

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
