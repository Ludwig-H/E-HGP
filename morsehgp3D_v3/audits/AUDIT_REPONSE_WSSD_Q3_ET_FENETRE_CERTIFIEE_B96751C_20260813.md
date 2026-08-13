# Réponse q3 : une WSSD aiguë compresse un broad phase, pas la source exacte

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Fraîcheur et verdict direct

Le `HEAD` observé est
`b96751c3d2342c2ca62b3005c3d3cd3e6e2988b3`. Le worktree est modifié
concurremment par Claude dans le probe WSPD, CMake, la proposition et plusieurs
notes. Aucun fichier logiciel n'a été modifié par l'auditeur. Les constats
logiciels ci-dessous sont donc relatifs au snapshot indiqué par l'audit mutable,
jamais un statut du `HEAD`.

Réponse à la question posée :

- **oui**, une généralisation de type WSSD peut représenter des blocs de
  triplets aigus et fournit un broad phase régulier, parallélisable et
  potentiellement utile ;
- **non**, le nombre linéaire de blocs ne rend pas l'énumération q3 exacte
  sparse. Un bloc `ALL_ACUTE(A,B,C)` peut représenter
  `|A|*|B|*|C|` triplets, et l'acuité ne décide ni le rang, ni le census, ni la
  multiplicité de `SupportKey` d'un même `BallKey` ;
- le premier moteur q3 à falsifier n'est donc pas une WSSD de triplets, mais
  `Q3FootPowerRange-v0` sur les carriers des **seules arêtes maximales
  certifiées ouvertes**, précédé par `CertifiedCageWindow-v0` et le reporter
  projectif pour les ancres sous-remplies.

La route recommandée évite toute mosaïque de Delaunay d'ordre supérieur, tout
arrangement global et toute expansion des produits WSSD. Elle ne possède encore
aucune borne reçue compatible avec le contrat G4.

## 1. Ce que garantit réellement une WSSD

La notion pertinente existe dans la littérature : une *well-separated
simplicial decomposition* couvre les simplexes par des tuples de cellules et
possède un nombre de tuples linéaire en `n` lorsque dimension, arité et précision
sont fixées. La source primaire vérifiée,
[Kerber--Sharathkumar, *Approximate Čech Complexes in Low and High Dimensions*](https://arxiv.org/abs/1307.3272),
emploie cette structure pour une filtration de Čech approchée. Elle ne fournit
pas un décideur exact d'acuité, de profondeur ou de `SupportKey`.

Trois quantités ne doivent jamais être confondues :

```text
nombre de tuples de blocs
masse logique somme |A|*|B|*|C|
travail exact après raffinement des MIXED, rang et census
```

La séparation WSSD porte sur le diamètre des cellules relativement à une boule
englobante. Elle n'interdit donc pas les triangles compacts. Dire que la WSSD
serait inapplicable parce que les trois sommets sont proches est trop fort. En
revanche, sa borne de représentation ne survit pas automatiquement au
raffinement exact de la frontière angle droit, ni à l'expansion des blocs
`ALL`.

### 1.1 Contre-famille cubique d'acuité

Placer trois petites boules disjointes autour des sommets d'un triangle
équilatéral et `m` points distincts dans chacune. Par continuité, si leur rayon
est assez petit, tout choix d'un point dans chaque boule forme un triangle
strictement aigu. Un nombre constant de tuples peut donc représenter
`m^3=Theta(n^3)` triplets aigus.

Cette famille ne prouve pas que tous ces triplets passent le rang q3 : elle
prouve précisément que **l'acuité seule n'apporte aucune parcimonie de rang**.
Un bloc `ALL_ACUTE` doit rester symbolique jusqu'à une preuve collective de
profondeur, ou publier sa masse et être refusé si elle est rouge.

### 1.2 Une seule boule peut porter plus de deux millions de supports q3

Poser
`N=826408505=5*13*17^2*29*37*41` et prendre tous les points u16

```text
(32768+x,32768+y,32768), avec x^2+y^2=N.
```

La formule des sommes de deux carrés donne exactement `384` positions, toutes
dans le cube u16. Les `C(384,3)=9 363 584` triples se répartissent exactement en
`6 967 680` obtus, `73 344` rectangles et `2 322 560` aigus. Chaque triple aigu
a le même cercle circonscrit, aucun intérieur strict et un shell de `384` IDs.

Ainsi, avant quotient ou branche de plateau, une seule `BallKey` porte plus de
deux millions de `SupportKey` q3. La route produit doit soit conserver cette
multiplicité sans perte, soit appliquer le quotient reçu, soit rendre
`unsupported_degeneracy`. Elle ne peut pas revendiquer une énumération sparse
sur tout le profil u16 à partir du seul nombre de blocs WSSD.

Cette fixture doit devenir permanente dans le code de Claude ; l'auditeur ne
touche pas au logiciel.

## 2. Canonicaliser q3 par l'arête maximale, pas par un tuple symétrique

Un support q3 positif possède un triangle aigu. Choisir son arête maximale
canonique `ab` transforme le problème en une relation binaire :

```text
arête maximale ouverte ab  ×  carrier aigu x.
```

Pour un triple ponctuel, si `D=||b-a||^2`, `E=||x-a||^2` et
`X=||x-b||^2`, les marges déjà présentes dans `rect_front.hpp` sont :

```text
E + X - D > 0
D - E >= 0
D - X >= 0
```

Elles expriment respectivement l'angle opposé à `ab` strictement aigu et `ab`
maximale faible. Leur version AABB `ALL/NONE/MIXED` est une bonne primitive de
`EdgeActiveFormCounter-v0`. Un futur `AcuteCarrierCellFront-v0` peut agréger
ces carriers si `carrier_mass` domine ; il ne remplace ni la fenêtre d'arêtes,
ni le rang, ni la source q4.

Le bloc canonique doit porter `EdgeSpan`, `CarrierNode`, masse logique,
provenance, owner et continuation. Un `ALL` ne doit jamais être développé sans
gate.

## 3. `Q3FootPowerRange-v0` : la source q3 exacte à falsifier d'abord

Fixer une arête `ab`, poser `d=b-a`, et pour un carrier `x`, poser `u=x-a`.
Définir :

$$D=d\cdot d,\quad E=u\cdot u,\quad F=d\cdot u,\quad G=DE-F^2,\quad W=E(D-F)d+D(E-F)u.$$

Pour un carrier non collinéaire, `G>0` et le circumcentre vaut :

$$c=a+\frac{W}{2G}.$$

Pour un site `z` et `v=z-a`, le prédicat de puissance entier est :

$$P_x(z)=G\left\Vert v\right\Vert^2-v\cdot W.$$

Le site est strictement intérieur si `P_x(z)<0`, sur la coquille si
`P_x(z)=0`, et extérieur sinon. Il n'y a aucun flottant ni division dans le
prédicat.

Sous u16, `D,E,F` restent sous environ `2^34`, `G` sous `2^68`, `W` sous
environ `2^86` et `P_x` sous `2^105`. Le CPU peut employer un signé 128 bits ;
le device doit employer une ABI explicite à deux limbs, avec multiplication et
comparaison signées jugées.

### 3.1 Parcours LBVH saturé à neuf

Sur une boîte entière `v_i in [L_i,U_i]`, chaque terme
`G*v_i^2-W_i*v_i` est convexe. Son minimum exact est atteint à une des deux
valeurs entières voisines de `W_i/(2G)`, clipées à l'intervalle ; son maximum
est atteint à une extrémité. Les bornes de `P_x` sont donc séparables et
exactes.

Un parcours persistant peut alors :

- ajouter toute la population si `Pmax<0` ;
- élaguer le comptage strict si `Pmin>=0` ;
- scinder sinon ;
- arrêter le compte à neuf intérieurs, car q3 sous `smax=11` n'accepte que
  `p<=8` ;
- sérialiser toute tâche non consommée, jamais transformer un cap en absence.

Pour les survivants, le shell et les vrais IDs intérieurs restent obligatoires.
Le meilleur ordre est `BallKey` d'abord : former les clés des pieds, radix/RLE,
faire un seul range-count saturé par boule unique, puis un seul census complet
`I_B/U_B` pour chaque boule acceptée. Cela absorbe les carriers cosphériques
avant le census, au lieu de rescanner le nuage pour chacun.

Cette voie remplace directement le rescan `O(m_ab^2)` du q3 de
`edge_shallow.hpp`. Si ses visites LBVH deviennent rouges, les mêmes pieds
pourront ensuite être localisés dans les bas niveaux du `LineFormTape`. Il ne
faut pas commencer par cette structure plus complexe sans avoir mesuré le
range-count.

## 4. `CertifiedCageWindow-v0` : le cutoff géométrique qui remplace kNN

Le cutoff de rang est inexact, mais les cages de Voronoï donnent un cutoff
radial certifié. Le lemme et les fleurs sont détaillés dans
[`AUDIT_REPONSE_DEUX_PERTES_CAGES_FLEURS_B96751C_20260813.md`](AUDIT_REPONSE_DEUX_PERTES_CAGES_FLEURS_B96751C_20260813.md).

Pour une cage et un sommet de sa cellule locale, écrire
`t_v=p_v/(2r_v)`, avec `r_v>0`. La cible `d=b-a` échappe à la preuve de cette
cage seulement dans la boule :

$$F_v(d)=r_v\left\Vert d\right\Vert^2-p_v\cdot d\leq0.$$

Cette boule est centrée en `t_v`, passe par l'origine et est contenue dans
`||d||<=2||t_v||=||p_v||/r_v`. Pour une banque de cages, poser le maximum
rationnel de `||p_v||^2/r_v^2`. Un `BNode` est fermé par un seul test radial si
son `Dmin` dépasse strictement ce maximum. L'égalité reste ouverte.

Huit cages à unions d'IDs disjointes ferment q4 ; neuf ferment q3. Une ancre
`FULL9` obtient donc une boule LBVH certifiée contenant tous ses partenaires q3
possibles. Si `ab` est l'arête maximale d'un support, tous ses carriers vérifient
aussi `||x-a||<=||b-a||` : la même fenêtre radiale contient la génération
entière du support, pas seulement son endpoint maximal.

L'ordonnance candidate est :

```text
banque de cages proposée puis validée exactement
  -> range-report LBVH dans la fenêtre radiale certifiée
  -> test directionnel des 32 formes q4 ou 36 formes q3 sur le résiduel proche
  -> déduplication EdgeKey
  -> PWC0-A seulement pour les paires dont les deux endpoints sont UNDERFULL
```

La preuve d'un endpoint peut fermer une paire même si l'autre endpoint est
`UNDERFULL`; il faut donc tester les deux orientations avant de déléguer. Le
proposer de cages peut être borné, mais toute cage non trouvée, dégénérée ou
incomplète délègue. Les compteurs bloquants sont `FULL8/FULL9`, causes
`UNDERFULL`, masse des fenêtres radiales, masse `UNDERFULL×UNDERFULL`, formes
testées, splits, tâches, octets et HWM.

Ce jalon est plus directement compatible avec une LBVH résidente et un
`count--scan--fill` qu'une WSSD de triplets. Il reste une proposition : une
fenêtre radiale peut être dense et les ancres de bord ou quasi planaires peuvent
être sous-remplies.

## 5. Réponses aux trois questions de la note « deux pertes »

### 5.1 Le taux `84 %` n'était pas un plafond

Le chiffre `84,05 %` provenait de la lane q2 d'une configuration centrale. Il
a depuis été rétracté par Claude. Même pour la bonne lane, un taux à une
séparation, une taille et une famille ne borne ni toute factorisation
rectangulaire ni les crédits projectifs, où le témoin peut varier avec le
centre de sphère.

Les mesures récentes du spindle anisotrope sont un signal de rappel, pas un
plafond ni un reçu : elles sont par paire, sur une graine, sans producteur de
supports, et ne prouvent aucune factorisation sur `A×B×C`.

### 5.2 Aucun rang partenaire fixe n'est exact

Prendre :

```text
a=(10000,10000,10000)
b=(20000,10000,10000)
x=(15000,18000,10000)
z_j=(10000-j,10000,10000), 1<=j<=129
```

On a `AB^2=100000000` et `AX^2=BX^2=89000000` : le triangle est strictement
aigu et `AB` est son unique arête maximale. Son centre est
`(15000,24875/2,10000)` et son rayon carré `123765625/4`. Chaque `z_j` est
strictement hors de la circumboule, avec excès exact `10000*j+j^2`, tout en
précédant `b` dans l'ordre des voisins de `a`. Le support q3 vide survit donc
au-delà du rang `128`.

Un rang peut ordonner un proposer. Il ne ferme jamais le suffixe sans certificat
géométrique. La fenêtre de cages ci-dessus est un tel certificat ; elle n'est
plus un cutoff kNN.

### 5.3 Le shallow reste nécessaire, mais d'abord pour q4

Après une vraie fenêtre, q3 possède un pied unique par carrier et doit d'abord
tester `Q3FootPowerRange-v0`. L'arrangement complet est inutile. Les bas niveaux
du `LineFormTape` deviennent une optimisation seulement si les visites de
range-count sont rouges.

q4 conserve au contraire les intersections shallow `P-P/N-N/P-N`. Même une
fenêtre d'arêtes sparse peut laisser un grand `m_ab`; les portes
`E_4`, puis `M=sum m_ab`, puis `J/H` restent obligatoires. Une petite double
boucle peut être une ablation mesurée quand `m_ab` est certifié petit, jamais un
fallback non borné.

## 6. Réponse à la nouvelle question sur le terme directionnel du spindle

Pour `d=b-a` et `v=2z-a-b`, le terme demandé se simplifie exactement :

$$T=d\cdot v=\left\Vert z-a\right\Vert^2-\left\Vert z-b\right\Vert^2.$$

Il est séparable par axe sur `A×B×C`. Pour une coordonnée et un `z` fixé :

```text
minimum = dist(z,A)^2 - far(z,B)^2
maximum = far(z,A)^2 - dist(z,B)^2.
```

Les extrema entiers sur l'intervalle de `z` se trouvent en évaluant un ensemble
constant : endpoints clipés, ruptures des distances aux intervalles et les deux
entiers voisins des milieux qui séparent les endpoints les plus lointains.
Additionner les trois minima et maxima donne l'intervalle exact de `T`.

Tester uniquement les coins est faux. En dimension un, avec
`A=[0,1]`, `B=[0,3]`, `C=[0,2]`, le maximum exact vaut `4` en
`(a,b,z)=(0,2,2)`, alors que les huit choix d'endpoints ne donnent au plus que
`3`. Cette contradiction doit devenir une fixture permanente.

Si `[Tlo,Thi]` est exact, poser `T2lo=0` lorsque l'intervalle traverse zéro, et
sinon `T2lo=min(Tlo^2,Thi^2)`. Une fermeture rectangle q4 sûre est :

```text
D2lo > V2hi
(D2lo-V2hi)^2 > 2*(D2hi*V2hi-T2lo)
```

La variante q3 remplace la seconde ligne par
`3*(D2lo-V2hi)^2 > 4*(D2hi*V2hi-T2lo)`. Le `Hlo` exact déjà rendu par
`rect_h_interval` donne aussi la forme sûre, souvent plus forte :

```text
Qhi = D2hi*V2hi-T2lo
q4 ALL si Hlo>0 et 8*Hlo^2>Qhi
q3 ALL si Hlo>0 et 12*Hlo^2>Qhi
```

Toutes les égalités restent ouvertes et les produits sont promus en `i128`.
Le membre droit emploie impérativement `D2hi`, jamais `D2lo` : avec
`A=[2,4]x[1,2]`, `B=[0,1]x{7}` et `C={(1,4)}`, la substitution de `D2lo`
ferme faussement q4 pour `a=(4,2)`, `b=(1,7)`, `z=(1,4)`.

L'intervalle de `T` est exact ; le classifieur composé est seulement **sûr et
fail-open**. Il n'est pas nécessaire et suffisant sur le produit : `D2`, `V2`
et `T` sont corrélés, et leurs extrema séparés perdent du rappel. Cette branche
doit étendre `rect_classify` et réutiliser le spindle déjà gardé dans
`spindle_cone.hpp`, pas créer une seconde autorité. Supprimer la perte demande
un raffinement du rectangle ou une optimisation jointe, pas un claim « exact
sans perte ».

Les deux autres réponses de Claude restent celles du contre-audit de seuil :
raffiner jusqu'à un crédit ou une continuation, jamais seulement jusqu'à
`rho>0`; partager un `lane_mask` est recevable, mais choisir `s` juste au-dessus
du seuil ne minimise aucun coût composé.

## 7. Directive d'implémentation remise à Claude

Ordre proposé :

1. recevoir `BallFormToBallEvent-v0` sur petit `n`, avec
   `(BallKey,SupportKey,I_B,U_B,owner)` indépendants ;
2. construire `CertifiedCageWindow-v0` counter-only, exact-verify et fail-open ;
3. déléguer le sous-univers `UNDERFULL×UNDERFULL` à PWC0-A et recevoir les
   fates exclusifs `CLOSED/OPEN/PENDING` ;
4. mesurer séparément `E_3`, `E_4`, `M_3`, `M_4`, tâches et HWM ;
5. si q3 passe, générer les `BallKey` de pieds, RLE, puis lancer le range-count
   saturé à neuf et le census unique ;
6. si q4 passe, construire seulement les niveaux shallow locaux ;
7. conserver `AcuteCarrierCellFront-v0` ou une WSSD aiguë comme ablation
   comparative sur le même ledger transitif, jamais comme source présumée ;
8. porter sur device seulement après accord petit oracle et portes de coût,
   puis mesurer la chaîne complète jusqu'au fold et au payload officiel.

Fixtures/mutants minimaux : rang `128`, cosphère `384`, angle droit, carrier
colinéaire `G=0`, égalité de puissance, seuil huit/neuf, signe de `W`, overflow,
égalité du cutoff radial, réutilisation d'un ID de cage, cage dégénérée, omission
d'un sommet de fleur, coin-only pour `T`, permutation/tiling et continuation non
consommée.

Le ledger q3 bloque sur `carrier_blocks`, `carrier_mass`, `foot_queries`,
`unique_BallKeys`, visites LBVH, populations créditées, tests feuille,
`rejects_at_9`, survivants, shell, opérations larges, octets/HWM et pending.
Une gate verte sur le seul nombre de blocs ou sur la seule fraction fermée ne
reçoit rien vers la seconde G4.

GCP non utilisé.
