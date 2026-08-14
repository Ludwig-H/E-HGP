# Audit-source CK/WST pour q2, q3 et q4

> **Contre-audit du 14 août 2026.** La couverture CK et l'owner sont
> recevables comme proposition factorisée. Trois points ne le sont pas encore
> comme ordonnance : `JungDiskDepth` est prouvé pour une paire ponctuelle, pas
> uniformément sur `A x B`; le prune `NONE_ACUTE` doit précéder la
> multiplication des cellules q4; et aucun argument n'impose
> `eta=Theta(1/s)`. L'ordre consolidé, la contre-famille u16 et la sweep 1D sont
> dans
> [`AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md).

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot et verdict

Le pin relu est `HEAD=35fcea884cb93eff24db1e7c5962f8be23d4cb04`.
Le code producteur du `HEAD` est celui du parent `3c11bc8`. Pendant cet audit,
Claude modifie concurremment `prototype/wspd_wavefront_probe.cpp` et ajoute les
fichiers non suivis `prototype/soc64_rect.hpp` et `prototype/soc64_probe.cpp`.
Ces deltas live sont audités séparément ; ils ne sont ni attribués à
l'auditeur, ni reçus par le présent rapport. L'auditeur n'a modifié aucun
fichier logiciel et n'a pas utilisé GCP.

Verdict : Callahan--Kosaraju fournit le bon **tape factorisé exact** de toutes
les paires. Après rejet ou quotient des positions dupliquées et filtre exact
`D>0`, il devient la source de tous les q2 propres. Une extension ternaire puis
quaternaire, nommée ici `OwnedCK-WST`, couvre exactement les supports q3 et q4
par blocs et possède une ordonnance
`count--scan--fill` naturelle. Elle doit rester paresseuse : sa parcimonie
porte sur les enregistrements physiques, jamais sur une masse candidate brute
qui peut être quadratique, cubique ou quartique. Sous la politique actuelle,
un plateau extra-shell lourd peut d'ailleurs finir en `unsupported_degeneracy`
plutôt qu'en sortie publiée.

La route candidate est :

```text
CKPairTape(A,B)
  -> prune de profondeur q2/q3/q4 au niveau paire
  -> OwnedCK-WST3(A,B,C) avec owner arête maximale et acuité q3
  -> OwnedCK-WST4(A,B,C,D) avec au moins un carrier aigu
  -> positivité q4 directe, BallKey, census et fold
```

Les sorties retenues des arités inférieures ne sont jamais une source pour
l'arité supérieure. Les lanes partagent le split-tree, les blocs et les
codecs, mais pas leurs verdicts de rang.

## 1. Trois sémantiques à ne pas confondre

Pour `smax=11`, la distinction minimale est :

| arité | support géométrique minimal positif | budget régulier hors support |
|---|---|---:|
| q2 | deux positions distinctes, donc `D=||b-a||^2>0` ; centre milieu, poids `1/2,1/2` | 9 |
| q3 | triangle strictement aigu et non collinéaire | 8 |
| q4 | tétraèdre non coplanaire dont les quatre poids barycentriques du circumcentre sont strictement positifs | 7 |

Le budget du tableau ne décide que le cas régulier où le shell est le support.
Dans le cas général, c'est la cardinalité fermée de `I_B union U_B` qui
commande la pertinence et le domaine courant peut rendre
`unsupported_degeneracy`.

L'expression « quadruplet aigu » est donc impropre comme autorité q4. Le bon
prédicat est `well_centered_q4`, c'est-à-dire quatre barycentriques strictement
positives. La fixture
[`tetrahedron_face_filter_counterexamples.json`](../../tests/fixtures/regressions/tetrahedron_face_filter_counterexamples.json)
contient à la fois un q4 positif avec exactement deux faces obtuses et un
tétraèdre à quatre faces aiguës dont une barycentrique vaut `-1/12`. « Quatre
faces aiguës » n'est ni nécessaire ni suffisant.

## 2. q2 : `CKPairTape` est déjà le générateur exact

Soit `T` un fair-split tree canonique, ou un équivalent Morton dont la propriété
fair/compressed-split, le packing, les partitions et tie-breaks sont prouvés.
Une WSPD de séparation fixe `s`
fournit des couples de nœuds disjoints `(A_i,B_i)` tels que :

$$\binom{X}{2}=\bigsqcup_i A_i\times B_i.$$

Chaque paire non ordonnée de `PointId` apparaît donc exactement une fois. En
dimension trois, le nombre physique de rectangles vaut `O(s^3 n)`. Une paire
n'est cependant un q2 propre que si `D=||b-a||^2>0`. L'entrée doit donc rejeter
ou quotienter les positions dupliquées, ou le tape doit filtrer `D=0`
exactement. Sous cette porte, `CKPairTape` est une génération q2 complète sans
matrice de paires.

L'ABI minimale est :

```text
CKPairBlock{
  TreeDigest, RectId, ANodeKey, BNodeKey,
  pair_mass, lane_mask, proof_handle, continuation
}
```

`RectId` dépend des deux `NodeKey` canoniques, jamais du chemin d'une vague.
Un split remplace un bloc par des produits d'enfants disjoints ; l'identité de
masse et un oracle `PairId -> RectId` à petit `n` prouvent l'exact-once.

### 2.1 Rang q2 sans expansion

Pour un témoin `z`, poser :

$$H(a,b,z)=(z-a)\mathbin{\cdot}(b-z).$$

`H>0`, `H=0` et `H<0` signifient respectivement intérieur strict, shell et
extérieur de la boule diamétrale. Chaque `CKPairBlock` possède une partition
persistante du witness tree avec masquage diagonal exact des deux endpoints.
Deux frontières de sortie sont indispensables : `NO_OPEN_INTERIOR` signifie
seulement `H<=0`, tandis que `STRICT_OUTSIDE_CLOSED` exige `H<0`.

`ALL_IN` est certifié par un minorant strict. Le maximum continu exact s'obtient
en clippant, pour chaque couple de coins de `A` et `B`, le milieu `(a+b)/2` dans
la boîte témoin `C`, puis en évaluant
`4H=||b-a||^2-||2z-a-b||^2`. Un maximum `<=0` exclut l'intérieur strict ; seul
un maximum `<0` permet de retirer le CNode de la borne fermée. L'égalité reste
possible-shell et n'est jamais retirée du census fermé.

Noter `L_open` la masse garantie strictement intérieure et `U_closed` la
cardinalité de l'union fixe des IDs non-support qui peuvent encore appartenir à
au moins une boule fermée du bloc. `U_closed` n'est jamais le maximum de
cardinalités calculé séparément par paire. Alors :

- `L_open>=10` ferme tout le bloc q2 sans émettre un seul `PairId` ;
- `U_closed<=9` produit un `PairWitnessPacket` d'au plus neuf IDs, suffisant
  pour classifier exactement intérieur et shell de chaque paire du bloc ;
- sinon seuls les nœuds ou facteurs encore nécessaires pour resserrer
  `L_open` ou `U_closed` sont raffinés.

Les spans `ALL_IN`, `NO_OPEN_INTERIOR` et `STRICT_OUTSIDE_CLOSED` sont hérités
par restriction. Repartir de la racine après chaque split détruirait
précisément le gain CK. Pour q3/q4, la puissance est négative à l'intérieur :
`min P>=0` prouve seulement `NO_OPEN_INTERIOR`, tandis que `min P>0` prouve
`STRICT_OUTSIDE_CLOSED`. Les égalités restent toujours dans `U_closed`.

### 2.2 Cœur commun constant

Si `A` et `B` sont contenus dans les boules `(c_A,r_A)` et `(c_B,r_B)`, poser
`d=||c_B-c_A||`, `S=r_A+r_B` et `m_0=(c_A+c_B)/2`. Lorsque `d>2S` :

$$B^\circ\left(m_0,\frac{d-2S}{2}\right)\subseteq\bigcap_{a\in A,b\in B}B^\circ_{ab}.$$

En effet, le centre d'une boule diamétrale se déplace d'au plus `S/2` autour
de `m_0`, tandis que son rayon vaut au moins `(d-S)/2`. Dix IDs distincts dans
ce cœur, recertifiés exactement, ferment le rectangle entier par une seule
requête de population.

### 2.3 Limite informationnelle

Un bloc q2 accepté de masse `|A||B|` n'est pas devenu une liste sparse. Le
graphe de Gabriel 3D peut déjà avoir une masse quadratique. Si l'API aval exige
chaque `SupportKey` ou chaque `BallKey`, écrire ces objets coûte
`Omega(|A||B|)`.

L'architecture doit donc choisir explicitement :

1. consommer un `PairBlock` paresseux dans un fold factorisé à prouver ; ou
2. préflight la masse, puis développer atomiquement, appliquer un quotient
   reçu, ou rendre `resource_exhausted` sans sortie partielle.

La WSPD borne le tape physique. Elle ne rend pas égaux les niveaux ou les
`BallKey` des paires d'un rectangle.

## 3. q3 : extension `OwnedCK-WST3`

### 3.1 Construction et borne physique

Pour un rectangle CK `R=(A,B)`, choisir une boule déterministe
`B_R=B(o_R,r_R)` contenant `A union B`, avec `r_R>0`. Choisir un niveau Morton
`h(R)` dont le diamètre de cellule est comparable à `eta*r_R`, puis énumérer
les cellules non vides `C` de ce niveau qui rencontrent `3B_R`.

Cette région est complète pour l'arête maximale. Si `ab` est l'arête maximale
d'un triangle `abx`, alors `||x-a||<=||a-b||<=2r_R`, donc :

$$||x-o_R||\leq||x-a||+||a-o_R||\leq3r_R.$$

Le point `x` appartient à une unique cellule half-open du niveau `h(R)`. Une
grille de pas comparable à `eta*r_R` possède `O((1+eta^{-1})^3)` cellules
rencontrant `3B_R`. Pour `0<eta<=1`, le nombre de tuples physiques initiaux
avant filtres vaut donc :

$$F_3=O(s^3\eta^{-3}n).$$

Avec le choix supplémentaire `eta` proportionnel à `1/s`, on retrouve le
majorant `O(s^6 n)`. Ni ce choix, ni sa rentabilité ne découlent de CK. C'est un
majorant de blocs initiaux sous grille uniforme, pas de triplets logiques, ni du
coût de localisation, des diagonales et de tous les raffinements `MIXED`.

### 3.2 Owner exact-once

Chaque triangle choisit son arête owner en maximisant la distance carrée puis,
à égalité, en prenant la plus petite `EdgeKey`. Cette arête appartient à un
unique rectangle CK et le troisième sommet à une unique cellule `C`. Le tuple
retenu est donc unique.

Au niveau bloc, un owner commun est sûr si le minorant de `||a-b||^2` dépasse
les majorants des deux autres côtés. Les égalités et recouvrements restent
`MIXED` jusqu'au tie-break exact par `PointId`. Un tuple qui contient des IDs
répétés utilise une branche diagonale et n'émet jamais un faux simplexe.

### 3.3 Acuité q3

Pour `d=b-a`, `u=x-a`, poser :

```text
D = d dot d
E = u dot u
F = d dot u
X = D+E-2F
G = D*E-F^2
```

Une fois `ab` owner, le triangle est strictement aigu si et seulement si
`E+X-D>0`; `G>0` recertifie l'indépendance affine. Les deux autres angles sont
alors automatiquement strictement aigus. L'égalité est un angle droit et reste
hors q3 positif.

Un `CarrierBlock(A,B,C)` reçoit `ALL_ACUTE`, `NONE_ACUTE` ou `MIXED` par bornes
entières sûres sur les trois longueurs et sur `E+X-D`. Seul `MIXED` est scindé.
Le bloc `ALL_ACUTE` reste symbolique : il peut représenter `|A||B||C|`
supports.

### 3.4 Profondeur q3

Avant de construire `WST3`, une porte au niveau paire cherche neuf témoins
universels dans toutes les sphères q3 admissibles par `ab`. Le cœur CK et
`SOC64/CORNER512` possèdent une sémantique uniforme sur un bloc. En revanche,
`JungDiskDepth9` n'est prouvé en section 5 que pour une paire ponctuelle ; il ne
ferme un rectangle CK qu'après une preuve robuste supplémentaire sur tout
`A×B`.

Pour le résiduel, la forme entière du support q3 est celle de la proposition :

$$P_x(z)=G||z-a||^2-(z-a)\mathbin{\cdot}W.$$

Un quatrième facteur witness `Z` applique le même certificat
`L_open/U_closed`. Neuf intérieurs garantis ferment le bloc ; au plus huit
sites possiblement fermés donnent un packet exact. Le census et le
`BallKey` restent séparés par support ou par run réellement égal.

La fixture
[`hartigan_triangle_all_side_ranks_above_k.json`](../../tests/fixtures/regressions/hartigan_triangle_all_side_ranks_above_k.json)
interdit une cascade au même seuil de rang : un q3 de rang trois y a ses trois
côtés q2 au rang quatre. Pour le contrat global `smax=11`, la fixture de
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md)
est plus forte : le triangle aigu est vide et pertinent, tandis que chacune de
ses trois arêtes possède dix intérieurs, deux points de shell et rang 12.

## 4. q4 : extension semblable `OwnedCK-WST4`

### 4.1 Construction directe depuis le même rectangle owner

Pour chaque `R=(A,B)`, réutiliser le même niveau `h(R)` et sa liste de cellules
carrier rencontrant `3B_R`. Un tuple q4 est :

```text
OwnerEdgeBlock(A,B) x CarrierCell(C) x CarrierCell(D)
```

Pour `C!=D`, il représente `A×B×C×D`. Pour `C=D`, il représente la diagonale
sans répétition `A×B×binom(C,2)`. Les paires de cellules sont non ordonnées et
la règle d'owner sur les six arêtes est rejouée exactement.

Si `ab` est l'arête maximale du tétraèdre `abxy`, les deux carriers `x,y`
appartiennent à `3B_R` par la preuve q3. Ils possèdent donc un unique couple de
cellules au niveau `h(R)`. Pour `0<eta<=1`, le nombre physique initial avant
filtres vaut :

$$F_4=O(s^3\eta^{-6}n),$$

soit `O(s^9 n)` sous le choix supplémentaire `eta` proportionnel à `1/s`. Ce
majorant ne couvre ni localisation ni raffinements `MIXED`. Il justifie une
génération semblable à q3, mais sa constante impose un `count` et un preflight
avant tout `fill`.

### 4.2 Réemploi sûr de la relation aiguë q3

Un q4 bien centré dont `ab` est l'arête maximale possède au moins une face
adjacente `abx` ou `aby` strictement aiguë. Pour le voir, placer le milieu de
`ab` à l'origine, écrire les composantes perpendiculaires des carriers `u,v`
et celle du circumcentre `w`. La sphère et la positivité donnent :

$$||x||^2-\frac{D}{4}=2u\mathbin{\cdot}w,\qquad ||y||^2-\frac{D}{4}=2v\mathbin{\cdot}w,\qquad w=\alpha u+\beta v,$$

avec `alpha,beta>0` et `w!=0`. Si les deux faces étaient non aiguës au carrier,
les deux produits seraient non positifs et
`||w||^2=alpha*(u dot w)+beta*(v dot w)<=0`, contradiction.

`WST4` peut donc choisir comme carrier primaire le plus petit `PointId` parmi
les carriers adjacents aigus, puis prendre l'autre sommet comme apex. Cette
règle tue le doublon lorsque les deux faces sont aiguës. Elle réutilise la
**relation géométrique pré-rang** de `WST3`, jamais ses événements retenus.

### 4.3 Autorité q4

Après les filtres de distance et d'acuité nécessaire, chaque quadruplet doit
encore passer :

- quatre IDs distincts et orientation 3D non nulle ;
- owner exact parmi les six arêtes, avec tie-break `EdgeKey` ;
- quatre numérateurs barycentriques du circumcentre strictement positifs ;
- `BallKey`, census fermé, rang, shell et statut transactionnel q4.

Un test de faces aiguës ne remplace jamais les barycentriques. Les blocs qui
ne possèdent pas de preuve universelle restent `MIXED` et descendent jusqu'au
prédicat exact.

### 4.4 q4 ne dérive d'aucun sous-événement retenu

La contre-fixture u16 suivante contient 64 points. Poser :

```text
T={(20,20,20),(60,60,20),(60,20,60),(20,60,60)}
G01={(21,55,65),(21,57,64),(21,58,63),(21,59,62),(22,53,67),
     (22,55,66),(22,56,65),(22,57,65),(22,58,64)}
G23={(21,21,18),(21,22,17),(21,23,16),(21,25,15),(22,21,17),
     (22,22,16),(22,23,15),(22,24,15),(22,25,14)}
W={(40,40,0),(40,40,80)}
pour i=-4,...,5 ajouter
  (40+i,0,40),(40+i,80,40),(0,40+i,40),(80,40+i,40)
```

Le recalcul rationnel indépendant donne :

| objet | centre/rayon | intérieurs | shell | rang fermé |
|---|---|---:|---:|---:|
| q4 sur `T` | `(40,40,40)`, `R2=1200`, poids `1/4` | 0 | 4 | 4 |
| chacune des 4 faces q3 | `R2=3200/3` | 9 | 3 | 12 |
| chacune des 6 arêtes q2 | `R2=800` | 10 | 2 | 12 |

À `smax=11`, le q4 régulier est retenu mais aucun de ses dix sous-supports q2
ou q3 ne l'est. Une cascade depuis `E2`, depuis les événements q3, ou depuis
une clique q2 est donc fausse. La relation géométrique `WST3` nécessaire à q4
doit survivre indépendamment du masque de rang q3.

## 5. `JungDiskDepth` avant les extensions ternaires/quaternaires

Fixer une paire ponctuelle, translater `a` en zéro, poser `d=b-a`, `D=||d||^2`
et paramétrer le centre d'une sphère par `y=2c`. Toute sphère par `a,b`
vérifie `y dot d=D`. Les domaines nécessaires des centres q3 et q4 sont :

$$K_3(d)=\left\lbrace y:y\mathbin{\cdot}d=D,\ ||y-d||^2\leq\frac{D}{3}\right\rbrace,$$

$$K_4(d)=\left\lbrace y:y\mathbin{\cdot}d=D,\ ||y-d||^2\leq\frac{D}{2}\right\rbrace.$$

Ils viennent respectivement de `R2<=D/3` pour un triangle aigu d'arête
maximale et de Jung `R2<=3D/8` pour un tétraèdre bien centré d'arête maximale.
Un témoin relatif `s=z-a` est strictement intérieur si et seulement si :

$$y\mathbin{\cdot}s>||s||^2.$$

Pour un groupe d'IDs `G`, l'intersection du disque `K_q(d)` avec les
demi-plans `y dot s<=||s||^2` est vide si et seulement si `G` garantit au moins
un intérieur pour tout centre admissible. Dans le plan affine de dimension
deux, Helly fournit toujours une sous-base d'au plus trois IDs. Neuf groupes
disjoints ferment q3 ; huit ferment q4.

Cette preuve fixe `a,b`, donc `d`, le plan, le disque et les demi-plans. Elle ne
se transfère pas automatiquement à un rectangle `A×B`. Un
`BlockJungDiskDepth(A,B,G)` devrait quantifier le même verdict pour tout
`a in A`, `b in B` et tout centre du disque correspondant, avec un classifieur
`ALL/MIXED` prouvé. En son absence, la porte est exacte seulement sur une paire
singleton ou un microtile dont toutes les paires sont explicitement rejouées ;
un échec reste fail-open et ne ferme jamais le rectangle parent.

La contre-fixture u16 minimale suivante interdit de valider seulement un
représentant du rectangle :

```text
A={(0,0,0),(0,100,0)}
B={(100,0,0),(100,100,0)}
z_j=(50,j,0), j=0,...,7
```

Pour la paire basse, `H=2500-j^2`, `E=X=2500+j^2` et, au pire `j=7`,
`H=2451>0` ainsi que `3H^2-EX=11524802>0`. Les huit singletons ferment donc
q4. Pour la paire haute, `H=2500-(100-j)^2<=-6149` : aucun de ces témoins n'est
même q2 intérieur. Une fermeture paire ne peut donc pas être promue en
fermeture de `A×B`.

Le hot path GPU n'exécute pas l'arbre complet de suppressions. Il propose de
petits groupes de deux ou trois IDs, valide exactement l'infaisabilité
disque--demi-plans, puis compte des groupes disjoints. L'échec reste fail-open.
L'arbre exact sert d'oracle causal.

Une fixture q4 montre pourquoi ce disque est plus pertinent que le LP sur tout
le plan. Prendre `a=(0,0,0)`, `b=(100,0,0)` et huit groupes disjoints :

```text
G_x={(x,u,0),(x,0,u)}
(x,u)=(42,26),...,(47,26),(48,27),(49,27)
```

Dans le plan `y_x=100`, chaque groupe impose `Y<=t` et `Z<=t`, avec
`t=(x^2-100x+u^2)/u` et `50<|t|<sqrt(5000)`. Chaque demi-plan seul rencontre
le disque q4 `Y^2+Z^2<=5000`, mais leur paire ne le rencontre pas car
`2t^2>5000`. Les huit groupes ferment donc q4. Le LP global échoue même à
profondeur un, puisque `Y,Z` peuvent tendre vers moins l'infini. Un échec du
LP global ne signifie donc pas une pénurie de témoins sur le domaine Morse.

## 6. Ordonnance GPU résidente

Les trois étages utilisent la même mécanique :

1. radix Morton u16 et hiérarchie canonique ;
2. wavefront CK `(AKey,BKey)` jusqu'à séparation ;
3. classification pair-level q2/q3/q4, héritage du `ProofSpanDAG` ;
4. `count` des cellules non vides de niveau `h(R)` rencontrant `3B_R` ;
5. scan et émission `WST3`, puis filtre owner/acuité ;
6. seulement pour les rectangles q4 ouverts, count triangulaire des couples
   de cellules, scan et émission `WST4` ;
7. prédicats exacts, `BallKey`, radix/RLE, range-count et census ;
8. preflight de la sortie, fill, validation et publication atomique.

Toutes les files sont SoA et persistantes. Aucun thread ne possède une DFS,
une `priority_queue` ou une allocation variable. Chaque tâche porte
`CloudDigest`, `TreeDigest`, `RectId`, clés de cellules, `lane_mask`, owner,
`proof_handle` et version de continuation. Les égalités descendent ; elles ne
sont ni prunées par flottant, ni transformées en position générale.

Avant toute allocation, publier séparément :

```text
F2, F3, F4                    blocs physiques
mass2, mass3, mass4          masse logique factorisée
L/U et masse fermée          par lane
mixed/refined/pending        par étage
M3 et M4                     incidences réellement ouvertes
raw/unique BallKeys          après génération
octets, HWM, wide_ops        puis temps par phase
```

Le q4 ne construit jamais ses carriers pour une **paire singleton** déjà fermée
par `JungDiskDepth8`; de même q3 avec `JungDiskDepth9`. Un rectangle CK entier
ne bénéficie de cette ordonnance que si un futur `BlockJungDiskDepth` prouve la
quantification uniforme, ou après split jusqu'à des microtiles explicitement
rejoués. Le cœur CK et les classifieurs rectangle restent les seuls certificats
block-uniformes déjà établis ici.

## 7. Contre-audit de l'autre auditeur

L'autre auditeur avait raison sur trois restrictions : une WSSD compacte ne
borne pas sa masse logique, `ALL_ACUTE` ne décide pas le rang, et q4 exige une
positivité directe. Ces restrictions sont conservées.

Deux corrections sont nécessaires :

1. une WSSD n'est pas seulement un broad phase si elle devient une partition
   possédée, porte des certificats `[L,U]` et reste un objet paresseux aval ;
   elle devient alors une source factorisée exacte, sans promettre une sortie
   sparse ;
2. le LP projectif sur tout le plan des centres est trop fort pour q3/q4. Le
   domaine pertinent est le disque de Jung ; la fixture à huit groupes montre
   que `LP global failure` ne diagnostique pas une profondeur Morse nulle.

Le théorème des faces aiguës est recevable seulement pour la relation
géométrique pré-rang. La fixture de 64 points interdit d'en déduire une cascade
depuis les événements q3 retenus.

## 8. Portes remises à Claude

Ordre d'implémentation proposé, sans toucher au code de Claude :

1. recevoir `CKPairTape-v0` : unicité `PairId`, masse, séparation entière,
   doublons de positions et déterminisme CPU/device ;
2. ajouter le certificateur q2 `[L_open,U_closed]`, le cœur commun et le mutant
   qui rejoue tous les witnesses après un split ;
3. implémenter `OwnedCK-WST3-v0` en mode counter-only : couverture exhaustive
   à petit `n`, owner, cellules de `3B_R`, `ALL/NONE/MIXED`, sans BallKey ;
4. ajouter `JungDiskDepth9` au niveau paire/microtile et graver la
   contre-fixture qui interdit sa promotion au rectangle, puis le census q3 ;
5. implémenter `OwnedCK-WST4-v0` depuis les carriers géométriques pré-rang,
   avec paires de cellules `CROSS/WITHIN`, carrier aigu canonique et quatre
   barycentriques exactes ;
6. ajouter `JungDiskDepth8` au même niveau borné, la fixture de 64 points et les mutants
   `q4_requires_retained_q3`, `q4_requires_q2_clique` et
   `q4_uses_q3_depth_mask` ;
7. câbler aussi `arity-cascade` sur la fixture `arite4` : le binaire courant
   tue déjà le mutant, mais CMake ne possède qu'une porte explicitement motivée
   par l'arité trois ;
8. seulement après les oracles, mesurer le ledger transitif jusqu'au fold et
   au payload, puis envisager le port G4.

Fixtures supplémentaires : même cellule `C=D`, quatre IDs dont deux égaux,
arête maximale ex aequo, deux faces aiguës avec carrier primaire ex aequo,
q4 positif à deux faces obtuses, quatre faces aiguës mais q4 négatif, angle
droit, coplanaire, barycentrique `-1/0/+1`, seuils 7/8 et shell supplémentaire.

## 9. Rejeux de cet audit

- build Release : réussi ;
- sous-suite `ball_event|wspd_wavefront|gamma_judge|postings_gate|saturated_pipeline` : `87/87` passée en `87,28 s` ;
- checker des faces q4 : `5/5` passé ;
- checker q3 dont les côtés dépassent le rang : `6/6` passé ;
- CTests `centre_cell_fixture_arite3`, `centre_cell_fixture_arite4` et
  `centre_cell_kill_arity_cascade` : `3/3` passés ;
- recalcul `Fraction` de la fixture u16 de 64 points : six rangs q2 égaux à
  12, quatre rangs q3 égaux à 12 et q4 égal à 4 ;
- suite complète : interrompue volontairement après `28/734` tests terminés,
  tous verts, afin de ne pas concurrencer le travail live de Claude ; ce n'est
  pas une suite complète passée.

Ces résultats reçoivent les contre-fixtures et les composants bornés cités,
pas le générateur CK/WST encore proposé, ni le contrat G4.

## 10. Contre-audit live du shadow `SOC64` de Claude

Le delta concurrent ajoute `soc64_rect.hpp`, `soc64_probe.cpp`, puis branche un
shadow q4 dans `wspd_wavefront_probe.cpp`. Le prédicat ponctuel et la preuve
`SOC64` sont cohérents avec la section 6.2 de la proposition. Les `16/16` portes
du probe isolé passent en `0,39 s` sur le dernier relevé.

La première version raccordée n'était pas sûre. Elle appelait `SOC64` avant
`g_spindle` et `g_fallback`, créditait un ancêtre SOC-`ALL`, poursuivait la
descente, puis pouvait recréditer le même nœud ou ses descendants dans
`cred[2]`. Le test `cred[2]+soc_cred>=need[2]` additionnait ainsi des ensembles
non disjoints. Séparer deux compteurs ne séparait pas les IDs comptés.

La réparation demandée est un replay virtuel combiné :

```text
baseline q4 ledger : parcours historique inchangé
combined q4 ledger :
  appliquer tous les fallbacks baseline au nœud
  si verdict final ALL, créditer une fois et arrêter cette branche
  si verdict final MIXED, essayer SOC64
  si SOC64 ALL, créditer une fois et arrêter cette branche
  sinon descendre
flip = baseline_open && combined_closed
```

Claude a ensuite appliqué ce replay. Dans la source live de hash
`1e90ac322b523a355d79a51100469a7fd9839b29619302197bafa8aa5c0b6336`, `cred`
reste le ledger baseline, `ccred` est le ledger combiné, `cmask` éteint une
branche combinée dès son premier `ALL`, et SOC n'est essayé qu'après spindle et
fallback. Le flip est désormais `cred[2]<need[2] && ccred[2]>=need[2]`; la
somme brute est seulement conservée comme témoin négatif. Le build strict des
deux probes réussit.

Le replay manuel suivant est non vide et reproductible :

```text
build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=120 --coord=512 --sep-euclid=6/1 --tight --vwave --window=512 --window-ledger --soc64-shadow --judge-soc64 --min-soc-taches=1
```

Il juge `624` rectangles SOC-`ALL`, soit `3873` triples réels, avec zéro faux,
et le ledger combiné ferme `41` terminaux de masse `95`. L'ancienne somme
fautive aurait déclaré `127` fermetures de masse `316`, donc un surcompte de
`86` fermetures et `221` unités de masse. Ce résultat reçoit la correction du
chevauchement pour ce replay borné ; il ne reçoit pas encore l'intégration.

En effet, CMake ne contient encore aucune porte utilisant `--soc64-shadow` ou
`--judge-soc64`. Les `43/43` tests existants `soc64|wspd_wavefront` passent,
mais ils couvrent seulement le probe SOC isolé et les portes WSPD historiques.
Il faut graver le replay ci-dessus, exiger `faux=0`, `pending=0`, invariant nul,
et un plancher strict `surcompte>0`, puis tuer un mutant
`soc-sum-instead-of-union`. Le juge courant recertifie chaque rectangle
SOC-`ALL`, pas encore la cardinalité distincte qui justifie chaque flip.

Enfin, le protocole proposé plafonnait l'ablation à `4096` tâches par famille,
mais aucun cap maximal n'est implémenté. À `n=1000`, le shadow a soumis environ
`988000` tâches et `3,69` millions de couples. Ce coût non borné interdit toute
extrapolation vers `50000/1 s` ; la campagne doit échantillonner
déterministement hors chrono ou rendre un statut tronqué explicite.

## 11. Non-claims

`OwnedCK-WST` évite tout catalogue global de paires, toute mosaïque de Delaunay
d'ordre supérieur et tout arrangement global. Il matérialise seulement un
tape hiérarchique transitoire et des petits packets de preuve.

Il ne prouve ni une sortie universellement sparse, ni une borne de temps pour
le raffinement exact des frontières, ni `50000/1 s`. La porte vers le SLO est
falsifiable : la masse doit être fermée avant expansion sur les familles du
contrat, tandis que toute vraie sortie lourde doit être preflightée et rendue
atomiquement ou refusée par une ressource physique réelle.

GCP non utilisé.
