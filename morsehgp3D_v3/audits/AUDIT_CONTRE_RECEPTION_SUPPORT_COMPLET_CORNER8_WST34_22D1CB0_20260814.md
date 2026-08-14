# Contre-réception du support complet, de `Corner8` et de `WST3/WST4`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot audité :
`HEAD=22d1cb0326d6d2f7e640e14aa5cf276b8a93056b`, après les commits
`89774d0` (`Corner8BallDepth`), `e3f1925` (`WST3`) et `88a9ba8`
(`WST4`). Au début de cette contre-réception, le worktree était propre.
L'auditeur n'a modifié aucun logiciel.

Empreintes SHA-256 :

- `CMakeLists.txt` :
  `2687033ab827d04a08299c3105af850facef53246d13f2eda8c881ffcb84f807` ;
- `prototype/wst3_probe.cpp` :
  `362c921b8f1a41075951fd32498c4344e7ae7a06ce6577aacac265259944cb3b` ;
- `prototype/corner8_ball.hpp` :
  `35d140031f76cfba5394013e95a35c61643a22bb6ad89c2670535c9dc259e2d0` ;
- `prototype/corner8_probe.cpp` :
  `9f29100613543cdf1f246243ec7b8112b4c84826618c4a053b3332361c49115f`.

> [!CAUTION]
> **Révision `3703097` mathématiquement rouge, réparée par `a73161c`.** Le filtre d'acuité
> committé après ce snapshot affirme `E+X-D=2H` pour
> `H=(x-a) dot (b-x)`. L'identité exacte est `E+X-D=-2H`. Il élimine donc le
> côté aigu au lieu du côté non aigu. Toutes ses mesures
> `blocs4_aigu/gain`, dont le claim `1,62x`, sont invalides. Fixture minimale :
> le tétraèdre régulier
> `p0=(0,0,0),p1=(0,1,1),p2=(1,0,1),p3=(1,1,0)`, avec IDs
> `0<1<2<3`, a l'owner `EdgeKey(0,1)`, quatre poids `1/4` et
> `H(p0,p1,p2)=H(p0,p1,p3)=-1`. Le filtre committé retire précisément son
> couple de carriers. `a73161c` corrige le signe et ajoute la fixture scalaire ;
> toutes les mesures de la révision rouge restent invalides. La correction et
> la portée de la porte sont détaillées en Q6 et dans l'audit courant.

## Verdict exécutif

L'intuition de Louis est **exacte après une correction de vocabulaire** :
ce n'est pas le nombre de sites sur le shell qui fixe q2/q3/q4, mais le
**support minimal positif complet** de la miniboule.

```text
support minimal positif complet de taille 2 -> unique boule diamétrale q2
support minimal positif complet de taille 3 -> unique miniboule ambiante q3, centrée au circumcentre intrinsèque
support minimal positif complet de taille 4 -> unique circumsphère q4
```

Il faut donc tester une seule boule par support complet, jamais toutes les
sphères qui lui sont incidentes. Cette réduction est importante et reçue.
Elle ne dit toutefois pas que tester la boule diamétrale de `ab` élimine les
complétions q3/q4 de `ab`, ni que la liste des supports complets est déjà
sparse.

`Corner8BallDepth` implémente un certificat `ALL_INTERIOR` mathématiquement sûr
pour un **bloc de supports q4 déjà authentifiés**. Il ne construit ni ces
supports, ni leur owner, ni leur positivité, ni leur census fermé. Son ABI et
ses juges ne suffisent pas encore à une réception industrielle.

`WST3/WST4` reçoit la couverture d'un broad phase conditionnellement à l'owner,
mais pas le claim « source physique exact-once ». Le tape brut contient les
mêmes supports sous des ancres non-owner, des diagonales avec les endpoints et
des IDs départagés par rang Morton. Le juge ne regarde précisément que la
projection owner et partage ce tie-break. Le bon statut est donc :

```text
candidate-cover exact-once après projection owner sémantique
```

Cette projection n'est pas encore factorisée dans le producteur. Les masses
WST4 quartiques et les centaines de millions de blocs diagnostiquent un verrou
réel, mais ne sont ni `M4`, ni une source de sortie reçue.

## 1. Théorème exact de la miniboule canonique

Soit `S` un ensemble de deux à quatre sites, affinement indépendant, et soit
`o=sum(lambda_s*s)` son circumcentre intrinsèque, avec tous les
`lambda_s>0`. Alors la boule ambiante `B_S=B_R3(o,R)` est l'unique miniboule de
`S` et `S` en est le support minimal positif.

- pour `S={a,b}`, `o=(a+b)/2` et `B_S` est la boule de diamètre `ab` ;
- pour un triangle strictement aigu, `o` est calculé dans `aff(S)`, mais la
  boule événementielle et son census sont **ambiants en dimension trois** ;
- pour un tétraèdre bien centré, `B_S` est son unique circumsphère ;
- si un coefficient est nul ou négatif, la miniboule se réduit à un support
  minimal d'arité inférieure.

Toutes les sphères incidentes à `S` ont un centre `o+w`, où `w` est orthogonal
à l'espace directeur de `aff(S)`, et un rayon carré `R^2+||w||^2`. Pour
`w!=0`, elles ne sont pas des événements portés minimalement par `S`. Leur
intersection ouverte vaut exactement le cœur affine : segment ouvert pour q2,
circumdisque planaire pour q3, boule q4 entière.

La phrase « un troisième point sur le support donne q3 » doit donc se lire
« un troisième coefficient strictement positif dans le support minimal donne
q3 ». Un point intérieur augmente le rang sans entrer au support. Un site de
shell supplémentaire peut produire une autre `SupportKey` pour la même
`BallKey`, sans augmenter l'arité minimale.

Fixture de shell :

```text
a=(0,0,0), b=(4,0,0), c=(2,2,0), d=(2,0,2)
```

Les quatre sites sont cosphériques autour de `(2,0,0)` avec rayon `2`, mais
`ab` est un diamètre. Le triangle `abc` est droit et les poids de `c,d` dans le
tétraèdre sont nuls : l'événement reste q2.

Pour q3, « intrinsèque » qualifie le centre et le rayon, pas le domaine du
census. La fixture

```text
a=(0,0,0), b=(4,0,0), c=(2,3,0), z=(2,1,1)
```

a `o=(2,5/6,0)`, `R^2=169/36` et des poids positifs
`(13/36,13/36,5/18)`. Le site `z` est hors du plan du triangle, mais sa
puissance vaut `-11/3` : il est strictement intérieur à la boule q3 ambiante et
doit compter dans son census.

Sous le régime régulier `smax=11`, dix, neuf et huit intérieurs stricts
suffisent respectivement à éliminer q2, q3 et q4. Le shell et le `BallKey`
restent un census séparé.

## 2. Ce que l'unicité permet réellement de retirer

Pour une paire complète q2, le test de Thalès
`H(a,b,z)=(z-a) dot (b-z)>0` décide l'intérieur de l'unique boule diamétrale.
Un certificateur de bloc peut donc fermer q2 sans quantifier un continuum de
centres.

Pour une paire qui n'est encore qu'une **ancre partielle** de q3/q4, le centre
reste variable. Le census de la seule boule diamétrale ne se propage pas en
général à ses complétions. Seuls les IDs dans le cœur affine, notamment le
segment ouvert `]a,b[`, sont intérieurs à toutes les sphères incidentes.

La version exacte et finie pour une paire fixée est :

```text
q2 -> centre w=0
q3 -> pied de la droite du troisième site dans le plan médiateur
q4 -> intersection des deux droites des troisième et quatrième sites
```

Positivité, owner, shell et `BallKey` sont ensuite recertifiés. Les domaines
Jung/BJD restent des pré-prunes collectifs suffisants, jamais la définition des
événements.

Cette réduction n'est pas encore uniforme sur un rectangle CK : quand `a,b`
varient, le plan médiateur, les formes, les pieds et les intersections varient
ensemble. L'unicité rend la source finie ; elle ne prouve ni un arrangement
commun à `A×B`, ni une somme linéaire de supports.

## 3. Réception mathématique bornée de `Corner8`

Pour un support q4 fixé `S=(a,b,c,d)`, poser `O=orient3d(S)` et `J(S,z)` le
déterminant in-sphere translaté. L'intérieur strict équivaut à des signes
opposés de `O` et `J`; le produit `O*J` ne doit pas être formé.

Si le signe de `O` est uniforme sur le bloc support et `sigma=sign(O)`, alors
`sigma*J(S,z)` est strictement convexe en `z`, avec coefficient quadratique
`|O|>0`. Son maximum sur une AABB témoin est donc atteint à l'un de ses huit
coins. Prouver le bon signe strict aux huit coins certifie tout le bloc
témoin. La réciproque est fausse : des coins extérieurs peuvent entourer un
centre intérieur.

Les largeurs annoncées sont sûres sous u16 : `|O|<6*65535^3<2^51` et
`|J|<72*65535^5<2^87`. Juger les deux signes séparément tient dans `i128`.

### 3.1 Portée exacte du verdict

Le verdict signifie seulement :

```text
pour tout support du produit A×B×C×D et tout z de Z,
le prédicat in-sphere strict est intérieur
```

Il ne prouve pas que les quatre facteurs portent des IDs distincts, que les
boîtes sont u16 valides, que le support est affinement indépendant et positif,
qu'il possède l'owner canonique, ni que le shell et le rang fermé ont été
traités. Avant ces gates, le compteur se nomme `domain_mass_closed`, jamais
`M4_closed`.

Contre-support u16 :

```text
a=(10,10,10), b=(14,10,10), c=(11,11,10), d=(11,10,11)
z=(12,9,9)
```

`Corner8` peut correctement classer `z` intérieur à la circumsphère, mais les
barycentriques du circumcentre sont `(2,1,-1,-1)`. Ce n'est pas un support q4
positif et aucune fermeture M4 ne suit sans gate amont.

### 3.2 Trous de réception

- le « juge » exhaustif réemploie `orient3d`, `insphere_j` et
  `interieur_strict` du sujet ; il est indépendant des intervalles, pas du
  signe ni du déterminant ponctuel ;
- les trois portes nominales emploient `PASS_REGULAR_EXPRESSION` ;
  `--selftest=1` imprime `accord=OUI` avant de rendre le code `3` sur son
  plancher ;
- `corners-outside-implies-none` retourne en réalité un faux `ALL` au premier
  coin non prouvé ; la primitive ne possède aucun verdict `NONE` ;
- `kProduitOJ` est déclaré, mais ni parsé ni implémenté ;
- `c8-drop-corner` et `c8-norme-aux-coins` survivent sur la fixture 4096 et ne
  sont pas dans CTest ;
- la fixture 4096 recertifie l'orientation et les intérieurs avec les mêmes
  prédicats, mais pas les vrais IDs, l'owner ni les barycentriques positives ;
- « huit coins » signifie huit évaluations d'un déterminant intervalle, pas
  huit opérations scalaires. Le chemin courant paie environ 1508
  multiplications d'extrémités `i128` pour un bloc `ALL`; ce coût doit être
  compté avant tout portage G4.

Une fixture déterministe simple tue `c8-drop-corner` : prendre le support
régulier centré en `(100,100,100)` avec les quatre vecteurs
`5*(1,1,1)`, `5*(1,-1,-1)`, `5*(-1,1,-1)`, `5*(-1,-1,1)` et
`Z=[100,106]^3`. Sept coins ont une distance carrée au centre au plus `72`, le
huitième `108`, pour un rayon carré `75`.

Une porte industrielle ajoute un oracle BigInt/rationnel indépendant, les
permutations, les extrêmes u16, les égalités de shell et le preflight du vrai
support. Elle doit vérifier chaque promotion, pas seulement la ligne finale.

## 4. `WST3/WST4` : couverture reçue, source exact-once non reçue

Le lemme géométrique de broad phase est exact. Si `ab` est une arête maximale
du triangle ou du tétraèdre, tout autre sommet `x` vérifie
`||x-a||<=||b-a||` et `||x-b||<=||b-a||`. Une enveloppe par `Dmax(A,B)` et
distance aux boîtes ne perd donc aucun support owner.

Le juge vérifie correctement que chaque triplet ou quadruplet énuméré possède
une représentation dans les blocs du **rectangle de son owner choisi**. Les
campagnes `uniform/eight_clusters` donnent zéro manquant et zéro doublon dans
ce sous-domaine.

Il ne vérifie pas l'exact-once du tape physique global :

1. il ignore toutes les représentations du même support sous les deux ou cinq
   autres ancres non-owner ;
2. la traversée témoin part de la racine et ne normalise pas les intersections
   `A/B/C/D` : `x=a`, `x=b` et les autres diagonales sont dans la masse brute,
   mais absents des boucles `i<j<k<l` du juge ;
3. le tie-break emploie les rangs Morton triés `idx`, pas les vrais
   `PointId` conservés dans `pid`; sujet et juge partagent cette faute ;
4. les positions dupliquées sont refusées globalement, contrairement au
   profil : seule une paire endpoint `D=0` est filtrée, tandis que la
   multiplicité des `PointId` vers toute troisième position doit survivre ;
5. un arrêt à échelle grossière sur-couvre volontairement la lentille ; sans
   owner uniforme et normalisation factorisés, le filtre exact est simplement
   repoussé vers une expansion aval.

Fixture minimale du premier trou :

```text
p0=(0,0,0), p1=(1,1,0), p2=(1,0,1)
```

Les trois distances carrées valent `2`. Chaque arête est une arête maximale et
la lentille de chacune contient le troisième site. Un tie-break sélectionne un
owner, mais le broad phase représente le triangle sous les trois ancres ; le
juge ne cherche que celle qu'il a sélectionnée et annonce `vus=1`.

Pour WST4, prendre les couples non ordonnés de blocs témoins garantit une
représentation unique **à l'intérieur d'un rectangle owner déjà fixé**. Cela ne
répare aucune des quatre coutures globales ci-dessus.

Le producteur ne sort encore ni `SupportKey`, ni `PointId`, ni payload. La
formulation reçue est donc :

```text
CK candidate cover -> projection distinct-ID + owner -> support positif
                   -> Corner8/BlockBallDepth -> census -> BallKey/RLE
```

Le claim `OwnedCK-WST3/WST4` commence seulement après la projection, pas au
compteur brut actuel.

## 5. Coût : ce que les rampes prouvent et ne prouvent pas

Les replays suivants sont utiles :

```text
uniform n=120, s=2, inclusion complète :
  rectangles=1368, blocs WST3=19390, masse=589653, visites=88602
  triangles=280840, manquants=0, doublons=0

eight_clusters n=140, s=2, échelle=1 :
  rectangles=708, blocs WST3=2707, masse=1192590, visites=12140
  triangles=447580, manquants=0, doublons=0

WST4 uniform n=60, échelle=1 :
  quadruplets=487635, manquants=0, doublons=0
```

Les cinq portes WST3 du parent et la porte WST4 passent. Les trois portes de
couverture sont à regex et peuvent masquer un code non nul postérieur.

Les pentes `1000..16000` et la valeur d'environ 32 blocs WST3 par rectangle
sont des diagnostics finis, pas une borne. Le parcours s'arrête sur des nœuds
LBVH de tailles arbitrairement petites ; aucun théorème ne borne leur nombre
par rectangle sur un nuage adverse.

Le compte WST4 `sum_t k_t(k_t+1)/2` atteint déjà des centaines de millions de
records à `n=8000`. Sa masse logique proche de `n^4` confirme qu'il ne faut
jamais remplir le produit brut. Elle inclut toutefois la surcouverture, les
ancres non-owner et les diagonales : elle ne mesure ni `M4_apex`, ni les
supports positifs, ni les événements.

Un vrai niveau Morton fixé à une maille `Theta(eta*r_R)` donne, par packing,
`O(eta^-3)` cellules de grille dans une boule de rayon constant. Il faut pour
cela regrouper directement les IDs par `CellId` de ce niveau ; le simple arrêt
d'un LBVH quand sa diagonale passe sous le seuil n'impose aucune maille
minimale et ne reçoit pas cette borne.

## 6. Réponses aux questions Q6--Q9 de Claude

### Q6 — filtre aigu uniforme avant le produit

Sous owner maximal `ab`, les angles aux endpoints sont déjà non obtus. Le
troisième angle est strictement aigu si et seulement si
`K=(a-x) dot (b-x)>0`, équivalent à `E+X-D>0`.

Avec l'ABI Midball, attention au signe :

```text
H=(x-a) dot (b-x)=-K
Hmax<0  -> ALL_ACUTE
Hmin>=0 -> NONE_ACUTE
sinon   -> MIXED/split
```

Un simple test `midball verdict != NONE` est faux. `Midball NONE` agrège le
cas strict `Hmax<0`, qui est entièrement aigu, et l'égalité shell ; il faut
exposer les extrema stricts ou classifier directement `K`.

La borne AABB exacte et séparable existe. Par axe, pour chacun des quatre
couples d'endpoints `a_j,b_j`, la fonction
`(a_j-x_j)(b_j-x_j)` est convexe en `x_j`; son minimum est au point
`clip((a_j+b_j)/2,C_j)` et son maximum à une extrémité de `C_j`. Additionner
les trois extrema donne :

```text
K_min>0  -> ALL_ACUTE
K_max<=0 -> NONE_ACUTE
sinon    -> MIXED/split
```

Toute égalité descend vers le tie-break owner. Cette gate doit précéder le
produit WST4 et être jointe à une preuve owner uniforme ; seule, elle ne
déduplique pas les ancres.

### Q7 — croissance de `k_t`

La croissance observée n'est pas prouvée intrinsèque à la lentille. Elle est
compatible avec l'artefact du LBVH : le critère impose une taille maximale,
jamais une taille minimale, et peut donc émettre beaucoup de petits nœuds dans
un même volume. Une grille Morton à niveau réellement fixé par `r_R` possède
une borne de packing indépendante de la densité. La gate compare LBVH et vrais
`CellId`, publie `sum k_t`, `sum k_t^2`, maximum/quantiles, visites, octets et
HWM sur familles régulières et adverses.

### Q8 — `2B_R` ou lentille

Pour une paire ponctuelle et une boule `B_R` contenant ses endpoints, la
lentille exacte implique `x in 2B_R`; la constante deux est sharp. Sur un
rectangle, `Dmax` et les distances aux AABB décorrèlent les endpoints : les
deux relaxations ne se dominent plus nécessairement. Leur intersection est
sûre et ne peut qu'être plus petite. Mesurer séparément les rejets de `2B_R`,
des deux enveloppes endpoint et de leur intersection répond à la question sans
transformer un échantillon en théorème.

### Q9 — gate des variantes qui sur-couvrent

Une surcouverture ne doit pas être un mutant de correction. La racine unique
de `wst3-pas-de-descente` est bien une antichaîne et passe légitimement la
couverture ; elle réduit le nombre de blocs mais augmente la masse à filtrer.

La gate est une comparaison appariée multi-objectifs après projection owner :

```text
couverture et exact-once sémantiques identiques
masse candidate distinct-ID et extra-masse non-owner
sum k_t, sum k_t^2, max/quantiles k_t
blocs WST3/WST4, visites, opérations de filtre, octets et HWM
```

Un plafond absolu n'est recevable que s'il vient d'une capacité produit ou d'un
théorème de packing. `pas-de-descente` et `une-seule-boule` sont des ablations
de coût à placer sur une frontière de Pareto, jamais des mutants attendus code
`4` par le juge de couverture.

## 7. Route vers le SLO

L'unicité suggère la bonne ordonnance, mais pas le produit brut :

```text
CKPairTape exact avec vrais PointId et multiplicité
  -> cellule carrier coarse, sans produit C×D
  -> owner uniforme + distinct-ID + ALL/NONE_ACUTE
  -> certificat de profondeur sur le carrier
  -> seulement pour le résiduel, joindre un apex de façon paresseuse
  -> positivité q4 + Corner8 sur gros WitnessNode
  -> census/shell/BallKey/RLE avant tout fill
```

Le filtre aigu fournit une factorisation plus forte que le seul compteur. Pour
un rectangle, partitionner l'antichaîne témoin en `A`, blocs qui peuvent porter
une face aiguë incidente à `ab`, et `N`, blocs uniformément non aigus. Le lemme
du porteur donne alors la couverture symbolique exacte des couples encore
possibles :

```text
Q4CandidateCellPairs = Sym2(A) disjoint_union Cross(A,N)
```

`Sym2` désigne ici les couples non ordonnés `C<=D`, diagonale comprise ;
`Sym2(N)` disparaît. Il ne faut surtout pas développer les deux termes : ce sont
deux descripteurs de produit par rectangle. Dans `Sym2(A)`, le carrier primaire
ne peut être choisi qu'à la projection pointwise ou par un filtre factorisé sur
les vrais `PointId`; un tie-break entre blocs `MIXED` serait insuffisant.
`Cross(A,N)` est déjà orienté. Owner tétraédrique, distinct-ID et positivité
restent des filtres aval obligatoires.

Mieux encore, la liste WST3 elle-même n'a pas à être remplie avant le
consommateur. Un `LensDescriptor(RectId,root)` lance une traversée paresseuse du
BVH témoin. Un nœud hors lentille disparaît. Un nœud `NONE_ACUTE` disparaît du
flux **carrier**, mais reste disponible comme apex dans `Cross(A,N)` ; le jeter
des deux flux perdrait les tétraèdres qui n'ont qu'un seul carrier aigu. Un
carrier encore possible appelle d'abord `FaceAxisJungDepth8Block`. S'il ferme
toutes ses complétions, aucun apex n'est ouvert. Sinon seulement, une tâche
dual-tree `(CarrierNode,ApexRoot)` descend le produit résiduel et appelle
owner/positivité/Corner8. Cette ordonnance remplace les `44` millions de blocs
WST3 et le milliard de couples diagnostiqués à `n=32000` par des tâches guidées
par preuves ; son pire cas reste ouvert, mais elle attaque enfin le coût avant
matérialisation.

Pour q3, la version support-complet analogue à Corner8 peut éviter le centre.
Avec `u=b-a`, `v=c-a`, `U=u dot u`, `V=v dot v`, `F=u dot v` et
`Delta=U*V-F^2>0`, poser
`N=V*(U-F)*u+U*(V-F)*v`. Pour un témoin `z`, l'intérieur de la boule q3
ambiante équivaut à
`Delta*||z-a||^2-N dot (z-a)<0`. La forme est convexe en `z` ; huit coins
suffisent donc pour un support q3 fixé. Comme Corner8, un lift aux boîtes
support reste seulement un `ALL` conservateur et exige owner, positivité, IDs
et census séparés.

Le verrou immédiat n'est plus « trouver un centre » : c'est construire la
projection owner/positivité et appliquer ces certificats **avant** le carré
`sum k_t^2`. Sans cette couture, WST4 compte déjà trop de records pour le G4 et
ne rapproche pas du contrat une seconde à 50 000 points.

## 8. Contre-audit croisé

Les audits antérieurs avaient correctement placé `tau(F)`, le branch-and-cut
et les preuves de support complet avant le packing greedy. Leur angle mort
n'était pas le théorème, mais le raccord live des identités, des supports et des
tests non dégénérés. La présente contre-réception applique la même discipline à
WST3/WST4 : un owner mathématique dans le juge ne devient pas un owner physique
dans le tape.

La note de Claude au pin audité reconnaît correctement que WST4 brut est rouge
et pose les bonnes questions de filtrage. Elle suraffirme toutefois quatre
points : les compteurs ne sont pas encore `Owned`, la constante WST3 n'est pas
prouvée, `cred+reste` reste faux en composition SOC/BJD, et Midball n'est pas le
seul certificat viable reçu par une simple comparaison de temps sur machine
partagée. Les verdicts de coût se prennent après ordonnance marginale,
profilage et sortie appariée.

## 9. Portes permanentes demandées

1. `support_minimal_vs_shell` avec le carré/tétraèdre droit ci-dessus ;
2. `q3_ambient_witness` avec le témoin hors plan ;
3. Corner8 contre une autorité BigInt/rationnelle indépendante ;
4. Corner8 `drop-corner`, norme intérieure, permutation, u16 extrême et shell ;
5. WST3 égalité des trois arêtes, inventaire de **toutes** les ancres physiques ;
6. permutation des `PointId` à géométrie constante et vrai tie-break EdgeKey ;
7. positions dupliquées : paire `D=0` omise, multiplicité vers un tiers gardée ;
8. normalisation `A/B/C/D` et masse distinct-ID exacte ;
9. WST3/WST4 OFF/ON contre oracle complet, y compris extra-masse non-owner ;
10. plafonds count/preflight avant tout buffer, `double` interdit pour un compte
    exact et overflow vérifié sur `sum k_t^2`.

Rejeux ciblés : Corner8 `6/6`, WST3 parent `5/5`, WST4 `1/1`. Ces verts
reçoivent les campagnes bornées décrites, pas les coutures absentes.

## 10. Post-scriptum à `3703097`, réparation `a73161c`

Le commit `3703097` ajoute un compteur d'acuité avec le signe inversé. Avec
`H=(x-a) dot (b-x)`, on a exactement `E+X-D=-2H`; le troisième angle est aigu
si et seulement si `H<0`. Les taux et gains publiés par la révision `H>0` sont
donc invalides. Un tétraèdre régulier
`(0,0,0),(0,1,1),(1,0,1),(1,1,0)` a `H=-1` pour ses deux carriers et mord
directement cette faute.

Le commit `a73161c` corrige le routage potentiel : `Hmin>=0` donne
`NONE_ACUTE`, `Hmax<0` donne `ALL_ACUTE`, sinon `MIXED`; pour le produit q4,
conserver un couple dès qu'au moins un facteur vérifie `Hmin<0` est sûr. La
fixture scalaire du tétraèdre régulier et la sous-suite WST `8/8` passent. Elles
ne traversent toutefois pas le filtre de bloc puis le produit contre un juge q4
positif. Ce filtre reste un coût par bloc, pas « gratuit » : il appelle
l'extremum Midball. Il ne répare ni owner, ni diagonales, ni vrais `PointId`, ni
multiplicité ou positivité.

Trois défauts sont stables depuis `a73161c`, y compris au `HEAD=069d903` :

- le commentaire de `--echelle=num/den` exige
  `diag^2*den<=rayon^2*num`, mais la révision observée calcule
  `diag^2*num<=rayon^2*den`. L'option implémente l'inverse du ratio documenté ;
- la porte `--echelle=1/256` ne mord pas ce mutant. À `n=1000`, les comptes
  WST4 sont `6 159 060` pour `1/1`, `35 494` pour `1/64` et `65 167 274` pour
  `64/1`, exactement le sens inverse du contrat. À `1/4096,n=60`, chaque
  rectangle peut s'arrêter à la racine et le juge owner reste vert par pure
  surcouverture ; il ne reçoit ni sélectivité ni coût ;
- l'identité `sum choose2(C_i)+sum C_i*C_j=choose2(sum C_i)` remplace
  correctement le double loop et l'accumulation u128 est sûre sous le cap de
  points. La sortie reconvertit toutefois ce compte en `double` avec six
  chiffres ; ce n'est pas un reçu exact et cela ne réduit pas
  `candidate_blocks=sum k_t(k_t+1)/2`.

La rampe `n=1000..32000` réfute l'extrapolation de la pente locale `1,73`.
L'échelle grossière de `a73161c` réduit fortement le nombre fini de descripteurs,
mais ne prouve ni quasi-linéarité, ni convergence vers `41,6` cellules : les
nœuds LBVH ne sont pas les cubes d'un niveau Morton fixé requis par cet argument
de packing, et la masse logique est simplement repoussée vers l'aval. La route
reste : sémantique d'échelle non ambiguë, vraie grille ou preuve LBVH,
projection owner/distinct-ID, acuité et profondeur avant tout produit résiduel.

Le commit `069d903` ajoute un diagnostic `--corner8` **après** la
matérialisation de `par_terminal` et parcourt explicitement les couples `u<=v`.
Il ne retire donc encore aucune tâche du produit. Sur les diagonales `C=D`, le
produit de boîtes contient nécessairement `x=y`, donc l'orientation contient
zéro : à l'échelle très grossière où chaque rectangle n'a que sa racine, le
diagnostic est `100 % orient_indecise` et ne peut fermer aucun bloc. La voie
exacte est un raffinement paresseux de `binom(C,2)` en `LL/LR/RR` jusqu'à des
facteurs disjoints, jamais l'abandon de la diagonale.

Le nouveau prétest d'orientation centré est un majorant sûr, mais il n'est pas
transmis à `corner8_block`, qui recalcule l'ancien intervalle. La promesse
« orientation une seule fois » est donc fausse : lorsqu'ancien et nouveau
tests diffèrent, chaque nœud témoin peut refaire un verdict définitivement
`MIXED`. Sur `uniform,n=60,echelle=1,corner8=10000`, le diagnostic soumet
`10000` couples, en déclare `9704` indécis, ne ferme qu'un couple et paie
`35117` visites ; sur `eight_clusters,n=60,echelle=1/256`, les `379` couples
sont tous indécis. Aucun CTest, juge indépendant, statut `PARTIEL` du cap ou
plancher causal ne reçoit ce raccord. `masse_fermee` est la masse physique de
la candidate-cover observée, pas un compte d'événements q4 owner/positifs.

Le même commit saute tout couple dont `C,D` ne satisfait pas le
séparateur CK. C'est un filtre fail-open acceptable pour un diagnostic de
fermetures partielles, mais pas une source : des supports q4 valides ont des
carriers proches ou appartiennent à la diagonale `binom(C,2)`. Le commentaire
dit en outre « tous les couples de facteurs », alors que le code ne teste que
`C--D` ; `A--C`, `A--D`, `B--C` et `B--D` ne le sont pas. La condition utile
pour Corner8 est une orientation uniformément non nulle après raffinement, pas
une séparation CK nécessaire entre chaque paire de sommets.

## 11. Réponses Q10--Q13 à Claude

### Q10 — décision d'orientation et aplatissement

Aucune séparation deux-à-deux ne borne le volume normalisé d'un tétraèdre. La
fixture u16 centrée en `o=(2000,2000,2000)`,

```text
A=o+(1000,0,1)   B=o+(0,1000,-1)
C=o+(-1000,0,1)  D=o+(0,-1000,-1)
```

a quatre poids circumcentriques `1/4`, `R^2=1000001` et une orientation non
nulle très petite relativement aux longueurs. La famille paramétrique analogue
sur les réels, obtenue en remplaçant `1000` par `L` à hauteur un, a
`R/sup_edge_length` qui tend vers `1/2` tandis que son aplatissement normalisé
tend vers zéro. Dans le profil u16 fini, la fixture affichée réfute tout seuil
pratique supérieur à sa valeur ; elle ne prouve pas à elle seule l'absence
d'un plancher positif dépendant de la résolution. Seule la famille réelle porte
l'énoncé asymptotique indépendant de la résolution.

Il existe une partition exacte **adaptative**, pas une borne linéaire reçue.
Sur chaque produit, borner le déterminant centré : un intervalle strictement
positif ou négatif fixe le signe ; un produit atomique de déterminant nul est
rejeté à q4 ; sinon scinder le facteur dont le terme rayon--cofacteur domine.
Chaque parent est remplacé par tous ses enfants disjoints. Sur un nuage fini, le
processus termine au pire aux quadruplets ponctuels ; son nombre de blocs est
output-sensitive et peut rester quartique. Pour une arête ponctuelle, une
partition angulaire des projections autour de son axe peut fixer le signe de
`4O=det(d,2x-a-b,2y-a-b)`, mais elle ne fournit pas davantage de borne linéaire
sur les données adverses. Un cap publie `PENDING`, jamais
`NONE` ni « support ouvert ».

Le terme `e1` du helper centré est linéaire dans les rayons. La forme centrée
peut réduire les dépendances d'intervalle, mais elle ne change pas l'ordre de
l'erreur. Exiger `sep_ok(C,D)` est un choix de proposition, pas un théorème de
complétude.

### Q11 — petite orientation, rayon et certificat bisigne

Une petite orientation ne force pas un grand circumrayon relatif : la famille
précédente garde `R/sup_edge_length -> 1/2`. Même un rayon effectivement grand
ne garantit aucun témoin intérieur dans un nuage arbitraire. Une borne de rayon
ou une densité moyenne ne peut donc jamais fournir seule huit vrais `PointId`.
La formule entière exacte est plus instructive. Avec
`M=[b-a,x-a,y-a]`, `G=M^T*M`, `q=(||b-a||^2,||x-a||^2,||y-a||^2)` et
`Delta=det(G)=O^2`, on a :

```text
R^2 = (q^T*adj(G)*q)/(4*Delta)
```

Petit `Delta` ne force un grand rayon que si le numérateur possède en plus une
minoration indépendante, absente ici.

Une réparation sûre sans signe uniforme est un **certificat bisigne**, pas une
heuristique de rayon. Avec les conventions `O=det3(b-a,x-a,y-a)` et `J` de
Corner8, l'intérieur strict vérifie `O*J<0`. Un ledger de huit IDs authentifiés
universellement `J<0` ferme tous les supports `O>0`; un autre de huit IDs
universellement `J>0` ferme tous les supports `O<0`. Les supports `O=0` ne sont
pas q4. Chaque ledger doit être disjoint en lui-même des quatre IDs de support ;
les deux ledgers ne sont jamais additionnés pour un même support. Un cap ou un
ledger incomplet rend `MIXED/PENDING`.

Pour `O>0`, `J` est convexe en `z` et huit coins négatifs suffisent ; pour
`O<0`, `J` est concave et huit coins positifs suffisent. Ce n'est pas « la même
convexité » dans les deux cas. Cette voie exige encore que les bornes
`J<0/J>0` valent uniformément sur les facteurs support et la boîte témoin, un
oracle ponctuel indépendant des deux strates, puis une exécution avant
matérialisation.

Une seconde réparation, souvent mieux corrélée, supprime entièrement le signe
d'orientation. Poser `r=adj(G)*q` et, pour `s=z-a` :

```text
Phi(S,z) = Delta*||s||^2 - s^T*M*r = O*J(S,z)
```

Pour tout support non dégénéré, `z` est strictement intérieur si et seulement
si `Phi<0`. Le coefficient quadratique est `Delta=O^2>0` : `Phi` est convexe
quel que soit le signe de `O`, et ses huit coins suffisent sur une boîte témoin.
Sous u16, environ 138 bits signés suffisent ; le hot path i128 peut garder son
test rapide, puis un fallback i192/i256 traite le résiduel. Sur un produit de
boîtes support, borner directement `Phi` peut rester `MIXED` par décorrélation ;
un échec splitte ou publie `PENDING`, jamais un faux verdict.

La même forme de Gram recertifie la positivité sans orientation. Les poids de
`b,x,y` ont pour numérateurs `r_1,r_2,r_3` sur le dénominateur positif
`2*Delta`, et celui de `a` a pour numérateur
`2*Delta-r_1-r_2-r_3`. Les quatre doivent être strictement positifs. Owner et
shell restent séparés. Le bisigne committé à `f7ab7bb` est donc une voie sûre
conditionnelle, pas l'unique route sign-free ni une réception industrielle.

### Q12 — séparation globale et raffinement par lane

Conserver une seule `CKPairTape` coarse, proche du régime déjà favorable à q2,
puis raffiner localement les produits témoins qui restent `MIXED`. q3 et q4
peuvent avoir des arbres de continuation distincts, mais partagent le même
`RectId`, les mêmes vrais `PointId` et l'owner total. L'exact-once est préservé
si chaque raffinement remplace atomiquement un parent par une partition complète
de ses enfants, avec `sum(child_mass)=parent_mass` et aucun parent simultanément
actif.

Il n'est donc pas nécessaire de choisir un unique grand `s`. Utiliser `s` pour
la partition CK des paires, puis une échelle locale et un split guidé par le
prédicat pour owner, acuité, orientation ou profondeur. Deux WSPD indépendantes
par lane sans table de routage canonique recréeraient en revanche doublons et
trous.

### Q13 — la diagonale `C=D`

La diagonale n'est ni un cas Corner8 immédiat, ni un déchet. Pour un nœud de
deux enfants disjoints `L,R`, appliquer exactement :

```text
Sym2(C) = Sym2(L) disjoint_union (L cross R) disjoint_union Sym2(R)
```

Les diagonales d'une feuille à un seul `PointId` sont vides faute de deux IDs
distincts. Une feuille géométrique bucketisant plusieurs IDs dupliqués conserve
leurs paires, puis `Delta=0` les rejette à q4 ; la multiplicité n'est jamais
supprimée par convention. Les produits croisés peuvent ensuite être classifiés ;
les deux diagonales récursent. Cette identité termine sur l'arbre fini et
conserve chaque paire non ordonnée une fois.
Une sweep d'axe ou le shallow edge-local ne devient applicable qu'après fixation
d'un vrai carrier `(a,b,x)` ; ce n'est pas un substitut à la partition initiale
de `Sym2(C)`.

## 12. Contre-réception du commit `52585c5`

Snapshot relu : `HEAD=52585c57b81fd429a1b9974f02217655f8c7661a`,
`prototype/wst3_probe.cpp=e604069a38e9cd699968d41869769c57c85d360a87de4a3b9f2b5de2e65145bd`,
`prototype/corner8_ball.hpp=c09ef9f2f7b3c0710464cbb108bd0c6285b2d1c36b41e2771a1b9fa267508a1c`
et `CMakeLists.txt=363a4fdac07eed17ed98334511d216f39e821629c77b2ebb8076c281f3b903b4`.
Le worktree était propre avant les présentes écritures documentaires. Le commit
répare une omission : `sep_ok(C,D)` incrémente désormais un diagnostic mais ne
supprime plus le couple. Il ne reçoit pas encore la source q4, `Sym2` ou le
raccord bisigne.

### 12.1 La généralisation exacte n'exige pas toutes les séparations

Pour le rectangle pair-exact `R=(A,B)` et son antichaîne témoin disjointe
`C_1,...,C_k`, les atomes d'ordre `q` sont indexés par :

```text
alpha_i>=0, sum_i alpha_i=q-2, alpha_i<=|C_i|
Atom(alpha) = A x B x product_i binom(C_i,alpha_i)
```

La relation reçue est leur intersection avec distinct-ID, owner total et
positivité. À q3, un seul facteur fournit un ID. À q4, la partition est
exactement `C_i x C_j` pour `i<j`, plus `binom(C_i,2)`. Le nombre d'atomes non
vides est le coefficient
`[z^(q-2)] product_i (1+z+...+z^min(|C_i|,q-2))`, donc au plus
`binom(k+q-3,q-2)`. La borne conditionnelle
`O(s^3*n*eta^(-3*(q-2)))` demande encore une vraie preuve
fair/compressed de `k=O(eta^-3)` ; le LBVH courant ne la fournit pas.

La séparation deux à deux n'est pas une précondition de cette partition. Elle
peut aider un classifieur uniforme, mais son échec ne donne que split ou
`PENDING`. La fixture suivante fixe trois facteurs et fait varier le quatrième :

```text
A=(1000,1000,30000)
B=(60000,1000,30000)
C=(1000,60000,30000)
D={(30000,30000,29999),(30000,30000,30001)}
```

Les deux orientations valent respectivement `-3481000000` et `+3481000000`.
Des facteurs spatialement lointains peuvent donc porter les deux signes ; aucune
collection de seules distances pairwise ne fixe l'orientation.

### 12.2 `Sym2` est compté, pas routé

Le commit parcourt chaque sous-arbre `C` et incrémente un compteur à chacun de
ses nœuds internes. Cela retrouve numériquement `|C|-1`, mais ne construit ni ne
consomme la partition
`Sym2(C)=Sym2(L) disjoint_union (L x R) disjoint_union Sym2(R)`. La boucle
Corner8 reste `u<=v` sur les blocs WST3 plats et soumet toujours `{C,C}` comme
deux boîtes identiques. Ce produit contient `x=y`, donc `O=J=0`; il force
l'intervalle bisigne à renoncer et peut déclencher presque un parcours complet
de l'arbre témoin par bloc.

Le rejeu local `n=60` à échelle grossière publie `478/478` couples
`cd_pending`, `478` orientations indécises, zéro fermeture et `56882` visites,
après `27393` itérations consacrées au simple compte `Sym2`. Sa
`masse_soumise=6.34469e6` emploie encore `|C|^2` sur la diagonale, tandis que la
masse candidate non ordonnée exacte vaut `3.11968e6`. Un vrai raccord doit
porter les deux facteurs enfants disjoints, retirer atomiquement le parent et
calculer l'injectivité par vrais `PointId` ; le compteur seul n'est pas ce
raccord.

Ce compteur est lui-même un mur inutile : sur `uniform`, `n=1000/2000/4000`,
`--echelle=1/256`, il effectue respectivement
`25434157/103783757/459477476` itérations, de pentes `2,029/2,146`. À `n=4000`,
cela vaut `3248x` les `141468` couples plats. Pour la seule télémétrie,
`count_of(C)-1` donne le même nombre en `O(1)` par cellule ; pour le produit
réel, `Sym2` doit rester un descripteur paresseux et ne descendre qu'à la demande
d'un prédicat `MIXED`.

### 12.3 Le théorème bisigne est sûr, son caller perd le mauvais bit

Pour un support fixé `O>0`, `J` est convexe en `z` et huit coins `J<0`
certifient une boîte intérieure. Pour `O<0`, `J` est concave et huit coins
`J>0` donnent l'implication symétrique. Deux ledgers authentifiés ferment donc
sûrement les deux strates, sans jamais additionner leurs crédits. Cette preuve
du flux mathématique antérieur est reçue.

Le caller de `52585c5` crédite toutefois tout masque non nul puis exécute un
`continue` global. Il peut ainsi arrêter la descente parce qu'un nœud certifie
le **mauvais** signe. La fixture déjà présente dans le probe Corner8 prend :

```text
A=(3,2,2), B=(1,2,2), C=(2,3,2), D=(2,2,3), Z=[1,3]^3
```

Ici `O=-2` et les huit coins du parent ont `J=-4` : le parent ne crédite que la
voie `O>0`, inutile au support réel `O<0`. Son centre a pourtant `J=+2` et est
intérieur. Le `continue` empêche précisément de descendre vers cette preuve.
C'est un faux négatif, pas un faux `ALL`, mais il peut annuler le gain annoncé.

La fixture de ledger au seuil huit est plus directe :

```text
A=(14,10,10), B=(6,10,10), C=(10,14,10), D=(10,10,14), O=-128
parent Z=[5,15]^3       : J=-7552 aux huit coins, masque 1
huit enfants {9,11}^3  : J=+1664, masque 2
```

Le caller actuel crédite le masque 1 puis ne visite jamais les huit IDs du
masque 2, seuls capables de fermer le support négatif. Échanger deux sommets
donne la fixture symétrique.

La correction normative est une pile de `(WitnessNode,active_sign_mask)` :
créditer l'intersection du masque certifié et des lanes actives, puis descendre
le même nœud pour les bits restants. `O=0` est rejeté comme support q4. Un bloc
d'orientation mixte peut contenir de tels tuples dégénérés : les retirer avant
la masse/fermeture conserve la sûreté de la relation, mais la télémétrie brute
qui les inclut dans `masse_fermee` est fausse. Il faut une fixture de mauvais
bit, son swap d'orientation, un oracle causal par bloc, les vrais IDs des deux
antichaînes et un statut `PENDING` au cap.

Le rejeu ciblé du snapshot donne `15/15` portes Corner8/WST3/WST4 vertes en
`14,67 s`, mais aucune commande enregistrée ne passe `--corner8` et aucune
CTest n'appelle `corner8_bisigne`. Les verts ne couvrent donc ni 12.2 ni 12.3.
Plusieurs portes saines utilisent encore `PASS_REGULAR_EXPRESSION`, qui peut
masquer un code non nul ultérieur.

### 12.4 Portée performance

La rampe CPU locale `12500/25000/50000`, Corner8 désactivé, reste rouge : la
dernière pente vaut `2,050` pour les visites et `2,193` pour F4 sur huit amas.
À `n=50000`, la masse candidate est proche de `3*binom(n,3)` en q3 et de
`6*binom(n,4)` en q4. Le filtre d'acuité coarse conserve plus de `99,998 %` des
blocs q3 et q4 dans les deux familles mesurées. Le prochain jalon doit donc
projeter distinct-ID/owner/positivité ou fermer la profondeur sur des
descripteurs **avant** leur produit ; compter `Sym2`, parcourir un préfixe ou
recenser la masse brute ne rapproche pas du SLO.

### 12.5 Contre-audit croisé des deux flux

Le flux mathématique a proposé la partition par multiplicités et signalé le
`continue` du mauvais bit. Le présent rapport a redérivé la partition, vérifié
les lignes du caller et donné deux fixtures indépendantes. Le flux performance
a signalé que `Sym2` n'était qu'un compteur et que la masse diagonale était
ordonnée ; la lecture du consommateur `u<=v` et le rejeu `n=60` le confirment.
En revanche, toute formulation antérieure exigeant toutes les séparations
pairwise est rejetée par la fixture aux orientations opposées de 12.1.

Ces validations portent sur contenu, pin, commande et contre-fixture. Aucun
artefact durable ne prouve une identité organisationnelle indépendante des
différents « auditeurs » ; cette expression reste relationnelle au document et
ne vaut jamais argument d'autorité.

GCP non utilisé.
