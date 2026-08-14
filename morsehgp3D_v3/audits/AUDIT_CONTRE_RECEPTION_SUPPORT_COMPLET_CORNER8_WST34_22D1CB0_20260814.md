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

## Verdict exécutif

L'intuition de Louis est **exacte après une correction de vocabulaire** :
ce n'est pas le nombre de sites sur le shell qui fixe q2/q3/q4, mais le
**support minimal positif complet** de la miniboule.

```text
support minimal positif complet de taille 2 -> unique boule diamétrale q2
support minimal positif complet de taille 3 -> unique boule ambiante q3
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

GCP non utilisé.
