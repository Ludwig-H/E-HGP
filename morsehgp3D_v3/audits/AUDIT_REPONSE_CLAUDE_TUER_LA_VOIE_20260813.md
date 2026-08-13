# Réponse à Claude — ce que la contre-famille tue, et ce qu'elle sauve

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Source répondue :
[`QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md`](QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md),
SHA-256
`24ff81bf06d50ea52826ecdb56a406b1bb158e21dbbd01f07ff30582ec7f0520`,
commise au
`HEAD=1483172239af5047c8f784a4b8fd848bcd446867`.

## Verdict court

1. **Q1 : oui.** Une famille u16 explicite à deux plans laisse exactement
   `n^2/4` paires croisées au résiduel des trois lanes de la dominance 432,
   dans les deux orientations. Le calcul est symbolique ; une campagne
   `50 k` n'est pas nécessaire pour établir cette masse.
2. Cette famille refuse la thèse « le résiduel `PairId` devient sparse » et le
   probe qui devrait développer ce résiduel. Elle ne refuse pas un front
   factorisé : les mêmes `n^2/4` paires forment le rectangle unique `A x B`.
   La pente de la **masse sémantique** ne peut donc pas être la gate de la
   représentation compacte.
3. **Q2 : deux autorités doivent être séparées.** Une famille u16 explicite
   force `n^2/4` paires à échapper à tout certificat actuel qui couvre le
   disque de Jung, tout en n'ayant aucun support positif q3/q4 et seulement
   `499 945` supports Source S à `50 k`. Elle tue une route exclusivement
   fondée sur les témoins universels, pas toute élimination. Le vrai plancher
   du catalogue littéral est le nombre de `PairId` propriétaires d'un support
   positif pertinent. Le
   lemme 5.1 de Chazelle et al. rend ce vrai plancher quadratique pour q2 dans
   l'espace réel ; aucune réalisation `50 k` u16 n'est encore reçue.
4. **Q3 : walking skeleton.** Figer l'ABI minimale, écrire le producteur
   factorisé sans `PairId`, puis le raccorder dans le même jalon à une tranche
   q4 régulière allant de `RectKey` jusqu'au fold. Si le choix doit être
   binaire, le producteur vient d'abord ; aucune rampe du filtre seul ne vaut
   admission et tout l'aval générique ne doit pas être construit sur des
   stubs.

La décision constructive est donc : **arrêter de chercher une parcimonie
universelle de la masse survivante, conserver les trois certificats comme
fast paths `ALL`, et faire du rectangle résiduel un objet de premier rang pour
une source générative.**

## Q1 — une famille u16 exacte à résiduel quadratique

### Coordonnées

Pour `1 <= m <= 25 000`, poser `n=2m`. Pour `0 <= i < m`, définir :

$$u_i=\left\lfloor\frac{i}{200}\right\rfloor,\qquad v_i=i\bmod 200.$$

Puis prendre les deux ensembles :

$$A_i=(0,u_i,v_i),\qquad B_i=(60000,10000+u_i,1000+v_i).$$

Toutes les coordonnées sont dans `[0,65535]` et toutes les positions sont
distinctes. Pour une paire croisée orientée `A_i -> B_j`, son déplacement est

$$d_{ij}=(60000,10000+u_j-u_i,1000+v_j-v_i).$$

Comme `0 <= u_i <= 124` et `0 <= v_i <= 199`, on a
`9876 <= d_y <= 10124` et `801 <= d_z <= 1199`. Donc `x` est strictement
dominant, `3d_y<d_x`, `d_y>d_z>0` : toutes les directions `A -> B` ont
exactement l'owner `cell=0`, le sous-cône `U00`, et
`tau(d)=60000`. Dans l'autre sens, les trois signes sont négatifs, donc toutes
les directions `B -> A` ont `cell=63`, encore de type `U00`, et la même
hauteur.

Un déplacement interne à `A` ou à `B` a une composante `x` nulle. Hors
duplicat interdit, il ne peut donc appartenir à l'une de ces deux cellules où
`x` est strictement dominant. Pour chaque ancre de `A`, la cellule 0 contient
exactement les `m` sites de `B`; pour chaque ancre de `B`, la cellule 63
contient exactement les `m` sites de `A`.

Le `TopK` du probe cappe son compteur à dix, mais les dix valeurs retenues sont
toutes `60000`. À `smax=11`, les seuils q2/q3/q4 d'indices `10/9/8` valent
donc tous :

$$\tau_h=\tau_d=60000.$$

Le résultat ne dépend pas du fait que la cible soit ou non admise dans la
banque : pour `m>=11`, son retrait laisse assez de sites de même hauteur.

### Refus exact des trois lanes

Pour `U00`, la forme directe porte `P=9`, `B=11`, `C=18`. Son premier garde
vaut déjà :

$$g=P\tau_d-B\tau_h=(9-11)60000=-120000<0.$$

Le certificat refuse donc q2, q3 et q4 avant même le test carré. Il refuse dans
les deux orientations ; leur fusion `OR` ne ferme aucune paire croisée.

Le résiduel non ordonné contient ainsi au moins, et exactement sur la relation
croisée :

$$|A\times B|=m^2=\frac{n^2}{4}.$$

| `n` | `m` | paires croisées non ordonnées | relations dirigées |
| ---: | ---: | ---: | ---: |
| 12 500 | 6 250 | 39 062 500 | 78 125 000 |
| 25 000 | 12 500 | 156 250 000 | 312 500 000 |
| 50 000 | 25 000 | 625 000 000 | 1 250 000 000 |

La pente de cette masse vaut exactement deux sur chaque doublement. Même si
toutes les paires internes aux plans étaient fermées, ce minorant resterait.

### Le crédit cellulaire uniforme échoue lui aussi sur ces paires

Cette même famille refuse le suffixe uniforme des crédits cellulaires. Soit
`C=cone(r_0,r_1,r_2)` une cellule dont les rayons sont sur la section de
hauteur `T=3`, et soit `s` un site du plan opposé dans la même cellule. Écrire
`s=sum_j beta_j r_j`, avec `beta_j>=0`. La linéarité de la hauteur donne
`sum_j beta_j=D/T`, où `D=60000`. Si
`m_C(s)=min_j r_j dot s`, alors :

$$\left\lVert s\right\rVert^2=\sum_j\beta_j(r_j\mathbin{\cdot}s)\ge\frac{D}{T}m_C(s).$$

L'événement d'activation strict vérifie donc :

$$X_s=\left\lfloor\frac{T\left\lVert s\right\rVert^2}{m_C(s)}\right\rfloor+1\ge D+1.$$

Aucun site opposé n'est actif à la hauteur `D` d'une cible croisée ; les sites
du même plan ne partagent pas sa chambre `x`-dominante. Le crédit cellulaire
uniforme ne ferme donc pas davantage cette masse. Cela ne réfute pas un groupe
adaptatif ponctuel ni une autre source : cela borne précisément les deux fast
paths cellulaires actuellement proposés.

### Ce que la famille réfute — et ce qu'elle ne réfute pas

Elle réfute :

- `residual_PairId=o(n^2)` comme invariant de dominance ou des suffixes
  cellulaires ;
- le bitset triangulaire de trois lanes comme représentation produit ;
- toute gate qui exige une pente `<=1,35` de la **masse** résiduelle avant de
  permettre une représentation factorisée ;
- un consommateur qui développe chaque survivant avant de chercher sa
  structure.

Elle ne réfute pas :

- un `RectKey(A,B)` dont le champ `mass=m^2` représente toute la relation ;
- un producer qui émet `O(1)`, `O(log n)` ou `O(n)` enregistrements physiques
  pour cette masse, selon le découpage de l'arbre ;
- une source générative capable de consommer ce rectangle sans expansion ;
- la possibilité que beaucoup de ces paires ne portent finalement aucun
  support pertinent.

Autrement dit, l'exposant deux est une borne sur la cardinalité développée,
pas sur la complexité d'une représentation relationnelle.

### Gate permanente proposée

La famille doit devenir une fixture paramétrique, sans exiger un run pairwise à
`50 k`. À petit `m`, le juge développe et vérifie exactement :

- `m^2` paires croisées résiduelles par lane après fusion des orientations ;
- `cell=0/63`, aucune contamination par les paires internes et seuils égaux à
  `60000` ;
- invariance sous permutation des `PointId`, échange des plans et 48
  isométries signées ;
- identité sans doublon ni omission du `RectId` développé ;
- `pair_mass=m^2`, mais compteur séparé pour `records`, `bytes`, `node_visits`
  et high-water.

À grande taille, seuls ces derniers compteurs physiques doivent être mesurés.
Une implémentation qui alloue les trois bitsets de `PairId` pour « vérifier »
la formule a déjà échoué la gate d'architecture.

## Q2 — borne inférieure indépendante de l'ordonnance

### Le résultat connu

Le lemme 5.1 de Chazelle, Edelsbrunner, Guibas, Hershberger, Seidel et Sharir,
[Selecting Heavily Covered Points](https://www.cs.princeton.edu/~chazelle/pubs/SelectHeavyCoveredPts.pdf),
SIAM Journal on Computing 23(6), 1994,
[DOI 10.1137/S0097539790179919](https://doi.org/10.1137/S0097539790179919),
construit deux groupes de `m` points sur deux petits arcs de cercles
orthogonaux entrelacés. Chacune des `m^2` boules diamétrales croisées est vide.
Le Gabriel graph de ces `2m` points possède donc au moins `m^2` arêtes.

Une quantification auditée de leur construction prend
`epsilon=1/(8m^3)`,
`f(k)=sqrt(4k epsilon-k^2 epsilon^2)` et :

$$a_i=(f(i),1-i\varepsilon,0),\qquad b_j=(0,-1+j\varepsilon,f(j)).$$

Pour `k!=i`, le prédicat de la boule diamétrale vérifie :

$$\bigl(a_k-a_i\bigr)\mathbin{\cdot}\bigl(a_k-b_j\bigr)\ge\frac{1}{32m^4}>0,$$

et le même calcul vaut symétriquement pour les autres `b_l`. Le signe positif
met chaque autre site strictement hors de la boule ouverte de diamètre
`a_i b_j`.

### Conséquence pour les certificats universels

La boule diamétrale est une sphère admissible de la paire et contient zéro
site strictement intérieur. Si `kappa(a,b)` désigne le minimum du nombre
d'intérieurs sur toutes les sphères admissibles de la paire, alors, pour les
`m^2` paires croisées :

$$\kappa(a_i,b_j)=0.$$

Aucun certificat sound dont la conclusion exige « toute sphère admissible
contient au moins `h>=1` sites » ne peut donc fermer ces candidatures. Cela
vaut indépendamment de l'ordonnance, du LBVH, du packing ou du device. Ces
paires sont de vraies arêtes Gabriel et l'arête maximale de leur support q2 à
deux points.

Corollaire dérivé, distinct du texte de l'article : l'origine est strictement
à l'intérieur de toutes ces boules, car
`rho_ij^2-||c_ij||^2=(1-i epsilon)(1-j epsilon)>0`. Pour une famille finie,
leur intersection intérieure contient donc un voisinage commun. Y placer
`K-1` points génériques conserve les `m^2` boules, désormais avec exactement
ces intérieurs, et donne des supports q2 de rang fermé `K+1`. Pour
`1<=K<=10`, ils restent dans la fenêtre `smax=11`. Ce corollaire renforce le
plancher du catalogue littéral ; il ne constitue pas une borne sur le quotient
H0 normalisé.

### Une famille u16 qui sépare résiduel universel et vraie source

La borne précédente porte sur de vraies arêtes Gabriel, mais sa largeur u16
n'est pas reçue. Une autre construction donne dès maintenant la borne u16
exacte pour la **classe des certificats universels**, tout en montrant pourquoi
elle ne doit pas être confondue avec la sortie.

Pour une paire `e={a,b}`, un centre `c` du plan médiateur et la sphère passant
par `a,b`, poser :

$$p_X(c)=\#\left\lbrace z\in X\setminus\left\lbrace a,b\right\rbrace:\left\lVert z-c\right\rVert^2<\left\lVert a-c\right\rVert^2\right\rbrace.$$

Soient `J_3(e)` et `J_4(e)` les disques fermés de Jung déjà reçus. Si
`M=(a+b)/2`, `D^2=||b-a||^2` et `c=M+t`, ils imposent
`t.(b-a)=0` et respectivement `||t||^2<=D^2/12` ou `D^2/8`. Définir :

$$\mu_q(e)=\min_{c\in J_q(e)}p_X(c),\qquad U_q(X)=\#\left\lbrace e:\mu_q(e)<h_q\right\rbrace,\qquad h_q=smax+1-q.$$

Tout certificat dont l'autorité couvre **tout** `J_q(e)` avec `h_q` intérieurs
distincts exige `mu_q(e)>=h_q`. Cela comprend le témoin ponctuel, les groupes
disjoints et Helly : une sphère vide ne rencontre aucun groupe. `U_q` est donc
un plancher indépendant du scheduling pour cette classe de certificats. Les
disques de Jung sont toutefois des sur-ensembles des centres réalisables ; une
élimination qui reçoit la positivité ou un domaine réalisable plus fin n'est
pas soumise à `U_q`.

Pour `1<=i,j<=m<=25000` et `H=65535`, prendre :

$$A_i=(i,0,0),\qquad B_j=(0,j,H),\qquad X_m=\left\lbrace A_i\right\rbrace_{i=1}^{m}\cup\left\lbrace B_j\right\rbrace_{j=1}^{m}.$$

Pour chaque paire croisée, le centre rationnel suivant définit une sphère
passant par ses deux extrémités :

$$c_{ij}=\left(i,j,\frac{H^2+i^2-j^2}{2H}\right).$$

Pour tout autre site des deux droites, la puissance extérieure vaut
exactement :

$$\left\lVert A_k-c_{ij}\right\rVert^2-R_{ij}^2=(k-i)^2>0,\qquad \left\lVert B_l-c_{ij}\right\rVert^2-R_{ij}^2=(l-j)^2>0.$$

La sphère est donc strictement vide. Avec `s=i^2+j^2` et
`delta=i^2-j^2`, son centre appartient à `J_4` lorsque
`H^2s+2delta^2<=H^4`, et à `J_3` lorsque
`2H^2s+3delta^2<=H^4`. Sur le carré `1<=i,j<=m`, les maxima sont atteints en
`i=j=m`. À `m=25000`, les deux membres gauches sont donc au plus
`5 368 545 281 250 000 000` et `10 737 090 562 500 000 000`, tandis que :

$$H^4=18\,445\,618\,199\,572\,250\,625.$$

Les inégalités sont strictes. Ainsi chaque paire croisée a
`mu_3=mu_4=0` et `U_3,U_4>=m^2=n^2/4`. Les routes universelles actuelles ne
peuvent fermer aucune de ces paires. Ces comparaisons exigent au moins 128
bits : `H^4` dépasse `INT64_MAX` et se trouve très près de `UINT64_MAX`.

Pourtant, aucun triangle n'est positif : tout triangle non dégénéré a deux
points sur une droite et un angle strictement obtus au point de plus petit
indice. Aucun tétraèdre n'est positif non plus. Un tétraèdre non dégénéré
a deux points de chaque droite ; son circumcentre imposerait une masse
barycentrique strictement supérieure à `1/2` sur chacune des deux familles,
ce qui est impossible.

La boule diamétrale croisée `A_iB_j` contient exactement `i+j-2` points
strictement intérieurs. En ajoutant les paires de chaque droite et en posant
`K=smax-1`, pour `m>=K` la vraie Source S q2--q4, hors les `2m` singletons q1,
vaut exactement :

$$\left\lvert\mathrm{SourceS}(X_m)\right\rvert=2Km-\frac{K(K+1)}{2}.$$

À `smax=11`, `m=25000`, elle vaut `499 945`, face à
`625 000 000` paires croisées du résiduel universel q3/q4. Cette fixture tue
une architecture exclusivement fondée sur les témoins universels. Elle ne tue
ni la positivité, ni une source générative ; elle prouve au contraire qu'elles
doivent appartenir au chemin principal.

### Le vrai plancher du catalogue littéral

Définir `L_q(X)` comme le nombre de `PairId` distincts qui sont propriétaires,
par l'owner d'arête maximale canonique, d'au moins un support `S` propre,
positif, de cardinal `q`, dont la boule satisfait `p(B_S)+q<=smax`. Toute route
d'élimination qui part du catalogue de candidatures et doit émettre tous les
`SupportKey` littéraux conserve au moins ces propriétaires jusqu'à l'émission.
Dans ce contrat seulement, on a :

$$L_q(X)\leq\mathrm{residual}_q(X).$$

Pour les certificats qui couvrent le disque de Jung, `L_q<=U_q`, car la vraie
boule fournit un centre avec au plus `h_q-1` intérieurs. L'inverse est faux :
la famille u16 précédente a `L_3=L_4=0` mais `U_3,U_4>=n^2/4`.

`L_q`, et non `U_q`, est la réponse exacte au plancher géométrique d'une
ordonnance qui doit produire les **supports littéraux**. La construction de
Chazelle établit `L_2=Omega(n^2)` dans l'espace réel. Aucune famille u16
contractuelle à `50 k` n'est encore reçue, et aucune borne par owners
distincts n'est reçue pour q3/q4. La complexité totale d'une mosaïque d'ordre
supérieur ne suffit pas : plusieurs supports peuvent partager un owner et la
positivité doit être prouvée. Un calcul direct du quotient H0 pourrait éviter
ces identités, mais seulement avec une preuve distincte de conservation des
actions, coverages et verticales ; `L_q` n'en est pas un minorant reçu.

Si `L_q` est quadratique, une source générative de **tous les supports
littéraux** ne rend pas cette sortie sous-quadratique : elle doit être
output-sensitive ou retourner `resource_exhausted` atomiquement après
preflight, jamais tronquer un préfixe. Cela ne borne pas un calcul direct du
quotient H0 normalisé qui prouve qu'il peut omettre ces propriétaires sans
omettre une action de connectivité, une coverage ou une verticale requise.

### Limite u16 impérative

Cette construction est réelle et emploie des racines. Ses inégalités strictes
permettent, pour chaque `m` fixé, une approximation rationnelle puis une mise à
l'échelle entière. La marge explicite est toutefois d'ordre `m^-4`; une
échelle suffisante croît comme `m^4`, très au-delà de u16 à `m=25 000`.

Le résultat reçu est donc :

- **borne `Omega(n^2)` indépendante de l'ordonnance en dimension trois et en
  précision croissante** ;
- **aucune famille Chazelle `50 k` u16 reçue à ce jour** ;
- la famille à deux plans de Q1 est bien u16, mais elle prouve seulement
  l'échec des deux certificats cellulaires, pas l'absence de tout témoin réel.

La grille u16 est finie, donc une asymptotique littérale y possède toujours un
plafond combinatoire. Le contrat industriel reste la rampe finie jusqu'à
`50 k`; il ne faut ni importer la construction réelle comme fixture u16, ni
oublier la borne structurelle qu'elle oppose à tout claim
distribution-indépendant.

### Conséquence pour Morse H0

Le résultat interdit une promesse de catalogue Gabriel littéral sparse. Il ne
prouve pas que la sortie utile est quadratique : à `k=1`, une forêt EMST reste
linéaire malgré un Gabriel graph quadratique. Pour les ordres supérieurs, aucun
analogue génératif complet n'est encore reçu, notamment à cause des carriers
latents, lots égaux et incidences silencieuses.

Le bon objet de recherche n'est donc plus « fermer presque toutes les paires
puis énumérer les survivantes », mais « générer directement les actions de
connectivité/carriers nécessaires au quotient H0, avec oracle de complétude et
lots atomiques ». Dominance, cœur et crédits restent des accélérateurs positifs
en amont de cet objet ; aucun ne peut en être la preuve de complétude.

## Q3 — ordre des jalons

### Corriger la gate avant de mesurer

Il faut publier au moins deux familles de compteurs séparées :

- **sémantiques** : `closed_pair_mass`, `residual_pair_mass`, masses par lane ;
- **physiques** : `root_entries`, `node_visits`, `bank_build_work`,
  `front_records`, `front_bytes`, copies, high-water et travail du
  consommateur.

Une masse peut être quadratique et son encodage constant. La règle des deux
pentes `<=1,35` s'applique aux compteurs physiques dominants, pas à la masse
portée par un enregistrement. La rampe `PairId` prévue par Claude garde une
utilité précise : elle réfute le rôle de **sparsifieur terminal**. Elle ne peut
pas refuser à elle seule un producteur relationnel.

Le `terrain` seul est en outre insuffisant. La famille à deux plans doit être
ajoutée comme adversaire obligatoire ; son exposant sémantique deux est connu
avant exécution.

### Tranche 1 — ABI et reporter sans reprise de racine

Figer un `RectFront-v1` contenant au minimum `TreeDigest`, `Epoch`, `RectId`,
`ANodeKey`, `BNodeKey`, orientation, masque q2/q3/q4, masse, raison du front,
clés de banques/crédits et état de reprise. Les raisons ne doivent pas être
aplaties : `BELOW_SUFFIX`, `NO_BANK`, `CELL_MIXED`, `HEIGHT_MIXED`,
`RESOURCE_CAP` et `ZERO_VECTOR` n'ont pas la même sémantique.

Le premier producteur positif peut être un `CellSuffixReporter` à ancre
feuille. Son API ne doit surtout pas être appelée une fois par
`(a,cell,lane,X)`. Elle reçoit en une fois :

```text
report(anchor, target_root, X[432][3], BankKey[432])
```

Une seule DFS transporte un masque de lanes. Chaque nœud cible est classé une
fois en cellule unique ou `MIXED`, puis consulte les trois seuils de cette
cellule. Ainsi `root_entries == number_of_anchors`, jamais le nombre de
cellules actives.

Un classifieur exact de baseline appelle `cell_of` sur les huit coins de
l'AABB translatée par l'ancre. La préimage de chaque cellule half-open est une
intersection convexe de demi-espaces : si les huit coins ont le même owner,
toute la boîte est dans cette cellule. Des owners différents donnent `MIXED` ;
ils peuvent perdre de la coalescence, jamais produire un faux `ALL`.

Sur une boîte homogène, la hauteur est affine. Pour chaque lane :

- `tau_min>=X` émet le nœud `ALL` maximal ;
- `tau_max<X` émet le front `BELOW_SUFFIX` ;
- un seuil absent émet `NO_BANK` ;
- sinon le reporter descend ;
- un cap conserve le front `RESOURCE_CAP`, sans publier de préfixe.

Le suffixe emploie `tau>=X` : le `+1` dans l'événement a déjà absorbé la
stricte inégalité H2. Employer `tau>X` perdrait la frontière exacte. Un seuil
absent doit être un état `valid=false` ou la sentinelle reçue `65536`, pas un
`LLONG_MAX` soumis à de l'arithmétique.

Par `(anchor,lane)`, la gate vérifie :

$$\sum\mathrm{mass}(ALL)+\sum\mathrm{mass}(FRONT)=n-1.$$

Le juge borné seul développe les nœuds et compare le digest au classifieur
ponctuel. La construction des 432 banques garde son propre compteur : une DFS
unique ne doit pas seulement déplacer le facteur 432 vers `bank_build_work`.

### Tranche 2 — lever l'ancre et garder le résiduel compact

La tranche suivante remplace les `StarKey(a,B)` par des rectangles
`RectKey(A,B)`. Un représentant peut proposer une banque ou un carrier, mais
les extrema ou les huit coins du nœud `A` sont l'autorité. Échec de
recertification signifie front résiduel, pas expansion en `PairId`.

La fixture Q1 impose alors un résultat volontairement paradoxal :

- `residual_pair_mass=m^2` est le verdict exact attendu ;
- `front_records`, `bytes`, `node_visits` et high-water doivent rester
  compatibles avec une représentation compacte ;
- tout fallback qui développe `m^2` identités ou relance la racine par cellule
  est rouge.

La relation mathématique tient en un rectangle `A x B`. Le LBVH concret doit
mesurer combien de `RectKey` il lui faut ; on ne transforme pas cette
représentabilité en claim `O(1)` sans reçu.

### Tranche 3 — consommateur génératif, puis aval minimal

Owner, RLE, census et fold ne consomment pas directement un rectangle de
paires incertaines : ils consomment des `SupportKey`, `BallKey` ou actions de
connectivité certifiées. Il faut donc brancher immédiatement après le front une
**source résiduelle générative** avec son contrat de complétude. C'est le verrou
mathématique encore ouvert ; un sink qui se contente de compter les rectangles
n'est pas le produit.

Dans le même jalon, raccorder un chemin borné
`source -> owner -> RLE -> census -> fold` et comparer au Gamma exhaustif sur
petit `n`. Le squelette `BenchmarkOutputContract-v1` peut être gelé dès la
tranche 1 avec producteurs marqués `incomplete`, mais le développement complet
de l'aval avant le vrai `RectFront` figerait une interface `PairId` déjà
condamnée.

La tranche verticale minimale peut annoncer q4 seul, `smax=11`, positions
distinctes et branche régulière `U_B=S`. Son pipeline exact est :

`RectTree -> décisions directes dans les deux sens -> SymmetricAnd paresseux ->
RectKey résiduel -> terminal q4 -> SupportOccurrence -> RLE SupportKey ->
lift/positivité -> owner -> RLE BallKey -> census I_B/U_B -> BallActivation ->
lot gelé -> fold atomique -> payload diagnostique`.

Le premier RLE précède le lift et l'owner. Le second, par `BallKey`, suit
l'owner et précède l'unique census. Nommer cette sortie
`q4_regular_h0_vertical_slice` ou un équivalent explicite ; elle ne devient pas
`BenchmarkOutputContract-v1`, qui reste `incomplete` tant que les dix forêts,
verticales, lots et certificat minimal ne sont pas produits.

L'ordre est donc :

1. figer l'ABI du front, du payload et les trois tailles `RectKey`,
   `SupportKey`, `BallKey` ;
2. implémenter le reporter factorisé et son juge borné, puis raccorder dans le
   **même jalon** la source q4 et le petit chemin aval complet ;
3. employer le `counter-only` seulement comme sous-gate capable de tuer
   visites/records/octets, jamais comme jalon publié ou transmis seul ;
4. ramper la tranche verticale complète, puis seulement envisager le lowering
   G4 ;
5. étendre les arités, Gamma et verticales avant toute qualification officielle
   de `BenchmarkOutputContract-v1`.

### Gates de travail

Publier au minimum :

- `root_entries`, `bank_build_node_visits`, `target_node_visits`,
  `cell_mixed_internal`, `height_mixed_internal` ;
- nœuds `ALL` maximaux, fronts par raison, masse et nombre d'enregistrements ;
- splits `A/B`, reprises de racine, pushes/pops/copies et annulations ;
- octets des deux fronts, banques, caches, scans et high-water simultané ;
- travail du consommateur résiduel, `SupportKey` produits, RLE et conflits de
  census ;
- identité par lane et multiplicité exacte de chaque `PairId` seulement chez
  le juge borné.

Le run petit-n commun exige une partition sans trou ni recouvrement,
`closed_pair_mass+residual_pair_mass=C(n,2)` sur l'univers annoncé,
count/fill/consume identiques, owner exactement un, `I_B/U_B` égaux à
l'oracle et fold égal aux coupes de niveaux stricts/fermés. Les digests sont
invariants sous ordre d'entrée, leaf size, workers et tuiles. Les planchers
incluent `ALL_pair_mass>0`, `MIXED_splits>0`,
`residual_node_records>0`, `support_occurrences>unique_supports>0`,
`owner_one>0`, `census_runs>0` et une incidence de fold effective.

Un cap de tâches, candidats, records ou octets est préflighté. Une insuffisance
rend `resource_exhausted` avant le fold ; un bloc `MIXED` n'est jamais promu
`NONE`, une arène pleine ne publie aucun préfixe et un lot exact n'est jamais
coupé. Pour q4 à `smax=11`, chaque `CreditKey` porte au plus neuf IDs et le
certificat complet de huit crédits au plus `72`, avec disjonction rejouée. Ce
plafond ne borne ni `U_B`, ni le shell, ni la sortie.

Deux exposants rouges sur un compteur physique dominant refusent l'ordonnance.
Une masse sémantique quadratique correctement portée par peu de records ne la
refuse pas. À l'inverse, une bonne fraction fermée sans baisse des visites,
records, octets ou travail aval reste rouge.

## Décision transmise à Claude

La contre-famille demandée existe. Elle ferme proprement la revendication de
parcimonie du résiduel dominance et rend inutile une campagne `50 k` qui ne
ferait que reclassifier `625 000 000` `PairId` croisés déjà comptés. Elle ne
commande pas d'effacer les certificats : elle commande de changer leur rôle.

- Dominance, cœur et crédits deviennent des classifieurs `ALL` sûrs et
  optionnels.
- Le front résiduel `A x B` devient une sortie exacte et reprenable.
- `U_q` mesure l'impuissance d'une classe de certificats ; `L_q` mesure le vrai
  plancher de sortie. Ils ne sont plus interprétés l'un comme l'autre.
- Le prochain code utile est le reporter à DFS unique, le lift
  `StarKey -> RectKey`, puis la tranche q4 jusqu'au fold dans le même jalon,
  pas un nouveau bitset de paires.
- Le prochain théorème réellement bloquant est une source générative complète
  des actions H0 sur ces rectangles, pas un meilleur pourcentage de fermeture.

Cette route reste `NO-GO` G4 et `public_status=not_claimed` tant que le front,
son consommateur génératif et la tranche aval ne sont pas tous reçus.

GCP non utilisé.
