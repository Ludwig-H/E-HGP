# Audit du lemme de miniboule unique : support complet et cœur affine

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!IMPORTANT]
> Ce rapport a été intégré par le commit `694920a`, dont le titre affirme une
> « fenêtre exacte » en `n^1,09`. Le contre-audit ci-dessous resserre ce claim :
> q2 est exact par paire sous le domaine régulier ; q3/q4 produisent un majorant
> `U<h` échantillonné, pas la fenêtre des événements. Les additions postérieures
> au commit appartiennent uniquement à la documentation d'audit.

## 0. Snapshot et verdict

Cet audit a commencé au pin
`5809bd2c054c02c4c77119d979a6be796032ca15`, qui a introduit le diagnostic
`--fenetre-exacte`. Le snapshot courant relu est désormais
`HEAD=694920ac59d58afdd639bd5156a481223e8d37d8`, commit
`la fenetre exacte est en n^1,09 la ou le certificat en publie n^1,95`, avec
worktree documentaire seulement pour le présent audit. L'auditeur ne modifie
aucun logiciel.

Ce nouveau pin absorbe aussi les réparations BJD postérieures : fixture
`collinear_seven`, statut `COMPLET/PARTIEL`, refus des modes vacuaires et huit
CTest d'intégration ciblées. Elles sont reçues comme réparations causales de
ces fautes précises, pas comme réception de `tau(F)`, de la source finie, du
sampler statistique, du chemin device ou du contrat G4.

L'intuition proposée est **exacte au niveau du support minimal complet** :
une fois un support positif affinement indépendant fixé, son événement est sa
miniboule intrinsèque unique. En particulier, un support q2 `{a,b}` ne possède
qu'un centre, le milieu, et qu'une boule, la boule de diamètre `ab`.

Deux conséquences doivent rester séparées :

- la source exacte ne doit jamais parcourir un continuum de sphères pour q2 ;
- la profondeur de la boule diamétrale ne permet pas d'éliminer les q3/q4 qui
  contiennent la même arête. Leurs centres uniques sont différents.

La version algorithmique forte est finie : pour une ancre `ab`, q2 interroge
l'origine du plan médiateur, q3 le pied auto-centré d'une ligne par troisième
site, et q4 les intersections shallow de deux lignes. Le disque de Jung et
`BlockJungDual64` sont des prunes collectifs suffisants avant cette source ; ils
ne définissent pas les événements.

## 1. Lemme exact de miniboule

Soit `S={p_i}`, `2<=|S|<=4`, affinement indépendant, et soit `c` équidistant
des membres de `S`, avec `c=sum_i lambda_i*p_i`, `lambda_i>0` et
`sum_i lambda_i=1`. Pour tout point `y`, l'identité de variance donne :

```text
sum_i lambda_i * ||p_i-y||^2 = r^2 + ||c-y||^2
```

Toute boule contenant `S` a donc un rayon au moins `r`, strictement plus grand
si son centre diffère de `c`. La boule `(c,r)` est l'unique miniboule de `S`.

Spécialisations en dimension trois :

- q2 : `c=(a+b)/2`, `r^2=||a-b||^2/4` ;
- q3 : le circumcentre intrinsèque du triangle, dans son plan ; le support est
  positif exactement pour un triangle strictement aigu ;
- q4 : le circumscentre du tétraèdre ; le support est positif exactement si
  les quatre barycentriques du centre sont strictement positives.

Un point supplémentaire sur le shell n'est pas automatiquement un membre du
support minimal. Un triangle droit a trois points sur le shell mais sa paire
hypoténuse reste le support q2. Les cosphères exigent toujours le census complet,
le groupement par `BallKey` et la politique `RelevantGP` ; elles ne permettent
pas de promouvoir silencieusement q2 en q3 ou q4.

### 1.1 Famille de toutes les sphères incidentes

La distinction entre support complet et support partiel se voit directement.
Soit `o` le circumcentre de `S` **dans** son espace affine, `R` son circumrayon
et `N=(aff(S)-o)^perp`, l'orthogonal de son espace directeur. Toutes les sphères
passant par `S`, et seulement elles, ont la forme :

$$c=o+w,\qquad \rho^2=R^2+\left\Vert w\right\Vert^2,\qquad w\in N.$$

Pour un témoin `z`, sa puissance à cette sphère est :

$$\mathrm{Pow}_z(w)=\left\Vert z-o\right\Vert^2-R^2-2\left\langle \mathrm{proj}_N(z-o),w\right\rangle.$$

La famille des centres a donc dimension `4-|S|` : un plan pour une paire, une
droite pour un triangle et un point pour un tétraèdre affine indépendant. La
condition de support minimal positif sélectionne `w=0`, car elle impose aussi
le centre dans `relint conv(S)`, donc dans `aff(S)`. C'est exactement pourquoi
la **classification de l'événement complet** ne visite qu'une boule, tandis
qu'une **élimination de toutes les complétions d'une ancre partielle** garde un
quantificateur sur `w`.

Cette distinction corrige le mot « associé » : une sphère arbitraire incidente
à `a,b` n'est pas un événement q2. Elle peut devenir la sphère canonique d'un
support q3/q4 complet qui contient cette paire.

### 1.2 Correction légère qui rend le prune partiel exact

La formule précédente donne aussi la version exacte la plus proche de
l'intuition initiale :

$$\bigcap_{\mathcal{S}\supset S}\mathrm{int}(\mathcal{S})=\mathrm{aff}(S)\cap\mathrm{int}\,B(o,R),$$

où l'intersection porte sur toutes les boules dont la sphère passe par `S`.
En effet, si `z` appartient à `aff(S)`, le terme linéaire en `w` est nul et son
statut est celui de la boule canonique. Hors de `aff(S)`, choisir `w` assez loin
dans la direction opposée à la projection normale rend sa puissance positive.

Ainsi le certificat universel non borné `UnboundedAffineCoreCount` compte :

- pour q2, seulement les `PointId` situés strictement sur le segment `]a,b[` ;
- pour une face q3, seulement les `PointId` de son plan situés strictement dans
  son circumdisque ;
- pour q4, toute la boule ouverte, puisque `aff(S)` est l'espace ambiant et la
  sphère est unique.

Pour une complétion cible d'arité `t`, `h` vrais IDs distincts de ce cœur
garantissent un rang fermé d'au moins `h+t`. Sous `smax=11`, les seuils de prune
suffisants restent donc dix pour q2, neuf pour q3 et huit pour q4. Ce certificat
est souvent vide en position générique, mais il est exact, très bon marché et
peut être décisif sur des scanlines ou des plans quantifiés. Sur le domaine
**borné** des centres Morse imposé par Jung, des témoins hors de l'espace affine
peuvent aussi être universels ; les reconnaître exige précisément le terme de
fonction support porté par `SOC/Jung/Axis`, pas le seul census canonique.

Plus généralement, pour un domaine de centres `K` contenant `w=0`, noter `C`
le nombre d'intérieurs de la boule canonique, `U_K` le nombre de témoins
individuellement intérieurs pour tout `w` de `K`, et `D_K` la profondeur
minimale collective sur `K`. On a le sandwich exact :

```text
U_K <= D_K <= C
```

Il donne une triage peu coûteuse pour chaque paire ou microtuile reçue :

- `C<h` prouve que **ce certificateur universel** ne peut pas fermer ; il faut
  passer directement aux complétions, sans conclure qu'elles n'existent pas ;
- `U_K>=h` ferme sûrement ;
- `U_K<h<=C` est précisément le résiduel collectif pour `tau(F)`, la sweep
  d'axe ou un split.

Le nombre canonique `C` n'est donc pas seulement insuffisant comme verdict
positif : il fournit aussi un early exit négatif exact pour éviter de lancer un
Jung/BJD voué à échouer sur ce domaine. Cette implication reste pointwise ; un
représentant ne la transmet pas à un rectangle CK sans borne uniforme.

Si `E` désigne seulement les centres effectivement produits par des complétions
du nuage, alors `E` est inclus dans le domaine continu `K` et
`D_K<=min_(w in E) Depth(w)`. En revanche, `w=0` n'appartient généralement pas
à `E` : pour une paire, il correspondrait à un troisième point formant un
triangle droit. Il n'existe donc aucune comparaison générale entre `C` et la
profondeur minimale des seuls événements finis. `C<h` court-circuite la preuve
relaxée Jung/BJD ; il ne déclare aucune complétion ouverte ou fermée.

## 2. Pourquoi la seule boule diamétrale ne ferme pas q3/q4

Prendre d'abord le nuage q3 u16 suivant :

```text
a=(52,114,100)       b=(148,114,100)
c3=(100,50,100)
z_j=(100,151+j,100), 0<=j<10
```

La boule de diamètre `ab` a pour centre `(100,114,100)` et rayon carré `2304`.
Les dix `z_j` y sont strictement intérieurs, avec distances carrées de `1369`
à `2116`. Son q2 possède donc dix intérieurs et dépasse la fenêtre régulière
`smax=11`.

Pourtant le triangle `a,b,c3` a pour circumcentre `(100,100,100)`, rayon carré
`2500` et barycentriques `(25/64,25/64,7/32)`. Il est strictement positif ;
`ab^2=9216` est sa plus longue arête, contre `6400` pour les deux autres. Les
dix témoins sont tous strictement extérieurs, de puissances `101` à `1100`.
Ce q3 de rang trois subsiste donc malgré le q2 profond.

Prendre séparément le nuage q4 suivant, sans `c3` :

```text
a=(52,114,100)       b=(148,114,100)
c4=(100,86,148)      d4=(100,86,52)
z_j=(100,151+j,100), 0<=j<10
```

Alors `a,b,c4,d4` a le centre `(100,100,100)`, le rayon carré `2500` et les
poids `(1/4,1/4,1/4,1/4)`. Les arêtes `ab` et `c4d4` ont longueur carrée
`9216`, les quatre autres `5392`; l'`EdgeKey` choisit `ab` si ses IDs sont les
plus petits. Les dix témoins restent extérieurs et ce q4 de rang quatre
subsiste.

Les deux nuages doivent rester deux fixtures distinctes. Si `c3,c4,d4` sont
placés ensemble, les cinq sites sont cosphériques autour de `(100,100,100)` :
le q3 a deux extra-shell et le q4 en a un. La non-hérédité resterait visible,
mais les rangs réguliers trois et quatre annoncés seraient faux.

Cette fixture interdit trois raccourcis :

- propager une fermeture q2 vers q3/q4 ;
- supposer le rang héréditaire d'une face vers une coface ;
- appeler « profondeur exacte q3/q4 » un compte fait seulement dans le cœur
  universel des sphères ancrées par `ab`.

### 2.1 La même non-hérédité de q3 vers q4

La phrase « idem pour q3 » exige une seconde fixture. Prendre :

```text
a=(10,10,10)       b=(20,10,10)
x=(12,6,6)         y=(12,6,14)
```

La face aiguë `abx` a pour circumcentre planaire `(15,9,9)`, rayon carré `27`
et barycentriques `(3/10,9/20,1/4)`. Les neuf témoins

```text
(10,9,8), (10,10,9), (11,8,6), (11,9,6), (11,10,6),
(11,11,7), (11,12,8), (11,12,9), (11,12,10)
```

ont les puissances `-1,-1,-1,-2,-1,-3,-1,-2,-1`. Le q3 possède donc un rang
fermé douze et sort de `smax=11`. Pourtant le tétraèdre `abxy` est bien centré :
sa circumsphère a pour centre `(15,8,10)`, rayon carré `29`, barycentriques
`(1/10,2/5,1/4,1/4)` et `ab` est son owner unique. Les mêmes puissances valent
`1,1,3,4,7,5,7,4,3` : les neuf témoins sont extérieurs et le q4 a rang quatre.

Fermer l'**événement** q3 n'autorise donc jamais à supprimer sa face comme
carrier géométrique pré-rang de q4. Seuls les témoins du circumdisque **et du
plan de la face**, ou un certificat universel sur son axe borné, se propagent à
toutes les circumsphères qui passent par cette face.

## 3. Forme finie exacte dans le plan médiateur

Fixer une paire propre `a,b`, poser `d=b-a`, `D=d dot d`,
`w=2*c-a-b` pour un centre équidistant de `a,b`, et
`U_z=2*z-a-b` pour chaque autre site. Le plan médiateur est `w dot d=0` et la
marge entière du site vaut :

```text
F_z(w) = D - ||U_z||^2 + 2*U_z dot w
```

Le signe `F_z>0`, `=0` ou `<0` signifie exactement intérieur, shell ou
extérieur à la sphère de centre `c`. Les événements de support contenant
`a,b` sont alors :

Avec la convention usuelle `Pow<0` pour l'intérieur et `t=c-(a+b)/2`, les
notations des deux sections vérifient exactement `F_z(2t)=-4*Pow_z(t)`. Cette
identité de signe doit être une fixture, car inverser `F` retournerait toutes
les demi-régions shallow.

1. q2 : `w=0`, donc la seule boule de diamètre `ab` ;
2. q3 : pour chaque site `x` non collinéaire, la ligne `F_x(w)=0`, puis son
   point de norme minimale dans le plan médiateur. C'est le pied unique et le
   circumcentre intrinsèque de `abx` ;
3. q4 : pour deux sites `x,y` aux normales indépendantes, l'intersection
   unique de `F_x=0` et `F_y=0`.

Chaque candidat recertifie ensuite indépendance affine, positivité stricte,
arête maximale/`EdgeKey`, intérieurs, shell complet et disposition. Une ligne
parallèle, une concurrence ou une égalité n'est jamais abandonnée : elle passe
au bundle exact ou au statut de dégénérescence prévu.

À `K_max=10`, un q3 retenu possède au plus huit intérieurs et son carrier est
donc sur l'un des neuf premiers niveaux au centre auto-centré. Un q4 retenu
possède au plus sept intérieurs et apparaît à une intersection des niveaux
`0..7`. Cette réduction porte sur les vrais niveaux orientés edge-local ; elle
ne transforme ni une face shallow arbitraire en événement q2/q3, ni un
arrangement global en architecture produit.

## 4. Conséquence blockwise q2

Pour q2, le prédicat exact de Thalès est :

```text
H(a,b,z) = (z-a) dot (b-z) > 0
```

Pour trois AABB `A,B,C`, le minimum continu de `H` se calcule exactement axe
par axe : sur chaque axe, la fonction est bilinéaire en `a,b` et concave en
`z`, donc son minimum sur la boîte est atteint parmi les huit triplets de
bornes. Si la somme des trois minima est strictement positive, tout vrai
`PointId` d'un nœud témoin `C` est dans la boule diamétrale de toute paire de
`A×B`. Sa population distincte peut être créditée q2 avant tout fill.

L'échec de ce **minimum** reste `MIXED`, jamais `NONE`. Un calcul distinct de
`max H<=0` peut certifier `NONE_INTERIOR` pour la relation nœud--produit ; son
maximum en `z`, concave, doit inclure le point stationnaire rabattu et ne se
réduit pas aux huit triplets de bornes. L'égalité reste shell. Des témoins
différents peuvent couvrir des paires différentes même si aucun nœud n'est
uniformément `ALL`; le raffinement ou un count exact doit alors continuer.

Ce `MidballBlockDepth` ferme uniquement la lane événementielle q2. Pour fermer
des complétions q3/q4 depuis le même rectangle, il faut soit certifier le cœur
affine précédent, soit quantifier sur le domaine Jung, soit poursuivre jusqu'au
support complet. Additionner la population tridimensionnelle de la boule
diamétrale à une lane supérieure est le mutant `midball-prune-q3q4`.

## 5. Audit de `--fenetre-exacte` au commit

Le nouveau diagnostic capture la bonne distinction dans ses commentaires,
mais son nom et son intervalle dépassent ce qu'il juge :

- pour q2, il décide exactement le seuil d'intérieurs stricts de la miniboule
  sur chaque paire tirée, sous domaine régulier ;
- pour q3/q4, il compte seulement des témoins singleton intérieurs à **toute**
  sphère du disque de Jung. Ce nombre minore la profondeur réelle ; la masse
  imprimée comme `ouverte` est donc un majorant des ancres pouvant encore
  porter un événement, pas la fenêtre exacte des miniboules finies ;
- il n'énumère ni les pieds q3, ni les intersections q4, ni leur positivité,
  leur owner, leur shell ou leur `BallKey` ;
- aucune CTest ne cible actuellement `--fenetre-exacte` ou `--fenetre-seed` ;
- la réduction modulo est non biaisée si ses mots u64 sont indépendants et
  uniformes, mais le flux SplitMix scellé est déterministe. Sans modèle
  probabiliste reçu sur la graine et l'indépendance des tirages, Hoeffding ne
  fournit pas un intervalle de confiance certifié ;
- le coût pire cas est `O(S*n)` pour `S` tirages, avec un cap accepté jusqu'à
  `2^24`, sans preflight d'opérations ni HWM.

Rejeu déterministe informatif sur le delta inchangé pour ce diagnostic :
`eight_clusters,n=200,S=1000,seed=1` imprime une masse ouverte `0,198` en q2,
`0,520` en q3 et `0,559` en q4, après `198000` scans. Ces valeurs sont
rejouables ; les crochets Hoeffding imprimés ne sont pas reçus comme intervalles
de confiance pour les raisons précédentes.

Le statut recevable est `PairUniversalCoreSample-diagnostic`. Renommer les
sorties en `q2_midball_exact_sample` et `q3/q4_universal_upper_window`, ou typer
explicitement `EXACT_Q2/UPPER_BOUND_Q3Q4`, évite une promotion accidentelle.
Pour une borne déterministe, classifier des blocs CK entiers en
`CLOSED/OPEN/PENDING` et attribuer toute la masse pendante à l'intervalle exact.
Pour un échantillon statistique, déclarer séparément la source de hasard et une
borne adaptée au tirage sans remise ; une seed fixe seule ne suffit pas.

## 6. Route d'implémentation à falsifier

```text
CKPairTape coarse exact-once, proche de s=2
  -> q2 MidballBlockDepth par H(a,b,z)
  -> C<h court-circuite Jung ; U_K>=h ferme ; sinon tau/split si amorti
  -> carrier aigu et WST factorisés sur le résiduel
  -> support q3 complet : positivité puis une seule boule canonique P_S
  -> carrier q3 pré-rang conservé pour q4
  -> support q4 complet : barycentriques puis une seule sphère canonique P_S
  -> résiduel alternatif edge-local : pieds q3 et intersections shallow q4
  -> BallKey/RLE, census, fold et payload
```

Le point de performance important est un **déplacement de quantificateur** :
après complétion du support, le polynôme homogène canonique `P_S` permet de
prouver `for all support tuple, census(B_S)` par blocs et splits, sans garder
une variable libre de centre. Une boule par tuple ne signifie toutefois pas
une boule commune à tout `A×B×C×D` ; un verdict de bloc exige toujours une
borne uniforme ou un split. Jung/BJD reste un pré-prune facultatif. Dans
l'ordonnance mesurée au pin, il vient après la descente, ne réduit aucune
recertification et augmente déjà le temps CPU : il ne doit pas retarder la
route canonique.

Le moteur edge-local ne construit pas `binom(m,2)` intersections. Il maintient
les niveaux orientés utiles et ne matérialise que leurs événements shallow,
avec une continuation si la liste de conflits dépasse le cap. Son coût doit
être publié par ancre et globalement : formes, pieds q3, intersections q4,
bundles, concurrences, conflits, scans de census, octets, HWM et temps. Une
borne locale en `O(k*m)` n'autorise aucune extrapolation tant que la somme des
`m` sur toutes les ancres n'est pas mesurée.

Un scan canonique par paire reste tout aussi interdit : le front historique
`s=2` contient environ `1,392,028` blocs sur `uniform`, et un seul scan de
50 000 témoins par bloc dépasserait `69` milliards de tests. Les classifications
`H/P_S`, les comptes de cœur et les census doivent donc rester dual-tree et
blockwise ; seuls les vrais résiduels capés atteignent une feuille.

## 7. Contre-audit croisé des textes antérieurs

[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
avait correctement séparé les trois frontières de supports et interdit la
cascade de rang. Sa source `OwnedCK-WST3/WST4` est compatible avec le présent
lemme : une fois le tuple complet, le polynôme de puissance canonique suffit.
Ses bornes conditionnelles de blocs ne prouvent toujours ni une masse logique
sparse, ni le coût des splits.

[`AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`](AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md)
avait correctement réparé le double crédit et redérivé `Depth=tau(F)` pour une
paire fixe. Son ordre Jung/BJD avant carrier reste un pré-prune possible, pas
une obligation de complétude. Son propre replay CPU — lectures identiques et
temps accru — justifie de tester d'abord la route support complet/canonique ou
de déplacer réellement le certificat avant la descente.

[`NOTE_CLAUDE_FENETRE_PAR_PAIRE_ET_UNICITE_20260814.md`](NOTE_CLAUDE_FENETRE_PAR_PAIRE_ET_UNICITE_20260814.md)
mesure une information utile : à taille et seed données, la proportion de
paires ayant assez de singletons universels. Elle a raison de séparer masse
logique et rectangles physiques. Elle appelle toutefois ce majorant « fenêtre
exacte », puis en déduit un squelette et une inclusion Delaunay. Cette déduction
est invalide : `U<h` n'exhibe aucun centre avec moins de `h` intérieurs. La
fixture à huit groupes de
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
ferme tout le disque q4 avec `U=0`, donc mord exactement ce non-converse. Les
tables restent des estimations issues d'un préfixe SplitMix sans sorties brutes
archivées ni modèle probabiliste reçu ; leurs pentes sont diagnostiques.

Aucun artefact Git ne démontre que ces deux flux documentaires proviennent de
deux identités indépendantes. Ils sont contre-audités ici par leurs énoncés,
leurs pins et leurs fixtures, jamais par autorité personnelle.

## 8. Contre-audit de la note de Claude au `HEAD=694920a`

La note
[`NOTE_CLAUDE_FENETRE_PAR_PAIRE_ET_UNICITE_20260814.md`](NOTE_CLAUDE_FENETRE_PAR_PAIRE_ET_UNICITE_20260814.md)
reçoit correctement le lemme d'unicité et interdit elle-même la cascade q2 vers
q3/q4. Elle mélange néanmoins encore une fenêtre d'**ancres candidates** avec
la fenêtre exacte des événements finis.

### 8.1 État logiciel rejoué

La sous-suite `ctest -R '^mhgp3v_bjd_'` passe `8/8` en `0,36 s` sur cette
machine. La fixture saine `collinear_seven,n=9` produit exactement
`couvrants=0`, `rejetes_credite=55`, aucune fermeture q4 et le code zéro. Le
nombre `55` compte des **visites de feuilles témoins rejetées** parce que leurs
IDs sont déjà crédités ; il ne compte ni bases ni IDs distincts. Le nuage ne
contient que sept témoins, jamais « cinquante-cinq témoins libres ».

Deux trous de porte restent indépendants de cette réparation :

- `--exige-q4-ouvert` sans `--juge-bjd`, sans BJD et sans vague rend encore
  `OK`/code zéro, donc l'exigence est vacuaire hors de son bloc de juge ;
- `--family=collinear_seven --points=200` exécute silencieusement `n=9`. Une
  fixture fixe doit refuser toute cardinalité différente ou ne pas exposer
  `--points` comme paramètre ;
- `--fenetre-seed` sans `--fenetre-exacte` est également accepté sans effet.

Un delta logiciel non repinné observé après ce rejeu corrige déjà les deux
premiers trous : `--exige-q4-ouvert` exige maintenant le juge, et
`collinear_seven` refuse toute taille autre que neuf. Le build ciblé passe et
les deux refus rendent le code `2`. Ce delta ajoute aussi des libellés
`q2_midball_exact` / `q3/q4_universal_upper_window` et un mode
`--fenetre-exhaustive` ; ces corrections sont sémantiquement bien orientées,
mais restent mobiles.

Le nouveau mode exhaustif doit encore être borné avant réception :

- son `return 0` précède les gates BJD et finales : combiné à un plancher BJD
  impossible ou à un mutant de réutilisation, il rend code zéro avant que la
  gate code `3` ou le mutant code `4` ne puissent mordre. Il faut refuser ces
  combinaisons ou n'avoir aucun retour anticipé avant les portes communes ;
- il est cubique en pire cas et accepte jusqu'à cent millions de points sans
  preflight du nombre de tests ni de l'overflow du compteur `scans` ;
- avec `--points=8,9`, son `return 0` après la première taille publie seulement
  les 28 paires de `n=8` et saute silencieusement `n=9` ; le mode doit exiger
  une taille unique ou poursuivre toutes les tailles ;
- `u_moyen` ne moyenne que les mille premières paires lexicographiques, donc
  doit être nommé `u_moyen_prefixe_1000`, pas moyenne exhaustive ;
- `--fenetre-exacte` et `--fenetre-seed` doivent être refusés avec le mode
  exhaustif au lieu d'être acceptés puis ignorés ;
- aucune CTest n'exerce encore les trois comptes, l'exactitude du total de
  paires, les libellés, le multi-taille refusé, les retours anticipés et le cap
  moins un.

À `eight_clusters,n=200`, le delta parcourt exactement `19900` paires et rend
les fenêtres `U<h` égales à `3790/10059/10937`, avec `3184359` tests ponctuels.
Ces entiers reçoivent le compte exhaustif de **ce majorant U** ; ils ne changent
pas l'inégalité `U<=D` ni le statut q3/q4.

Le diagnostic `--fenetre-exacte=1000` à `eight_clusters,n=200,seed=1` rejoue
`198000` scans et les fractions ouvertes `0,198/0,520/0,559`. Aucun CTest ne
cible encore cette option, ses libellés ou son mutant statistique.

### 8.2 La bonne inclusion

Pour q2, le domaine n'a qu'un centre, donc `u=d` : sous hypothèse régulière,
le sampler décide exactement, pour chaque paire tirée, si la boule diamétrale
contient moins de dix autres IDs. Il s'agit du graphe seuil
`I(B_ab)<10`. L'appeler « Gabriel d'ordre dix » est ambigu et généralement
décalé d'une unité : avec la convention usuelle « au plus k sites intérieurs »,
c'est l'ordre neuf. En présence d'extra-shell, il faut de plus le census fermé
et `BallKey`, absents du sampler.

Pour q3/q4, noter `u(ab)` le nombre de sites intérieurs à **tous** les centres
du domaine continu et `d(ab)` la profondeur minimale sur ce domaine. On a
seulement `u(ab)<=d(ab)`. Par conséquent :

```text
ancre d'un événement q4 retenu  =>  u(ab)<8,
mais u(ab)<8  n'implique ni d(ab)<8 ni l'existence d'un événement q4.
```

La fenêtre q4 publiée est donc un **sur-ensemble** des arêtes-owner utiles. Elle
n'est pas incluse dans un graphe de Delaunay d'ordre onze. C'est l'ensemble des
vrais événements q4 retenus qui, selon la convention de rang et après gestion
du shell, induit des arêtes dans un graphe de Delaunay borné. Des témoins
différents peuvent couvrir chaque centre sans qu'aucun témoin soit universel ;
alors `u=0` tandis que `d` est arbitrairement grand. Cette distinction est
exactement le verrou collectif que `tau(F)` traite.

Une fixture u16 permanente tue directement l'inclusion Delaunay revendiquée.
Prendre `a=(0,1000,1000)`, `b=(1000,1000,1000)` et, pour chaque entier
`420<=x<=431`, les trois points :

```text
p_x=(x,1260,1000)
q_x=(x,870,1220)
r_x=(x,870,780)
```

Après translation par `a`, écrire le centre doublé d'une sphère par `a,b` sous
la forme `(1000,Y,Z)`. Pour `s=(x,u,v)`, l'intérieur strict équivaut à
`Y*u+Z*v > x^2-1000*x+u^2+v^2`. À chaque `x`, les trois normales
`(260,0),(-130,220),(-130,-220)` somment à zéro, tandis que leurs trois seconds
membres somment à un nombre strictement négatif. Au moins un des trois sites
est donc intérieur à toute sphère ; avec douze valeurs de `x`, la profondeur
est toujours au moins douze. La paire n'appartient même pas au Delaunay
d'ordre onze standard.

Pourtant chaque singleton est évitable dans le disque q4
`Y^2+Z^2<=500000` : `(-684,0)` évite tous les `p_x`, `(362,-606)` tous les
`q_x`, et `(362,606)` tous les `r_x`. Ainsi `u_q4=0<8` et le sampler classe
l'ancre ouverte. Cette fixture doit tuer
`universal-core-window-subset-delaunay11`. Elle montre aussi que ce
sur-squelette peut rester dense : la finitude de la source n'est jamais une
preuve de sparsité.

Les pentes mesurées portent donc sur `PairUniversalCoreSample`, un majorant
d'ancres, pas sur une « fenêtre exacte quasi linéaire ». Elles sont un excellent
signal de route, mais ne prouvent ni la taille du squelette exact, ni son coût.
Le flux SplitMix scellé rend le replay déterministe ; sans modèle aléatoire
reçu sur la graine et les dépendances entre sorties, Hoeffding n'est pas un
intervalle de confiance certifié.

### 8.3 Réponses aux cinq questions

**Q1 — gate de coût.** `sum_E4` ne doit pas être confondu avec le nombre de
records effectivement traités, mais il ne doit pas être supprimé. La porte SLO
doit gater ensemble records physiques, octets/HWM, lectures et
recertifications, splits, sorties `BallEvent`/incidences et temps bout en bout.
La masse logique reste un préflight de toute expansion et un diagnostic de
charge/équilibrage. Une WSPD quasi linéaire ne reçoit pas un consommateur dont
le coût ou la sortie dépend encore de sa masse.

**Q2 — architecture.** Un tape global d'ancres candidates, puis des événements
edge-local shallow, respecte l'invariant s'il ne matérialise ni cellules,
cofaces ni incidences de la mosaïque de Delaunay d'ordre supérieur et s'il est
reçu exact-once. Construire d'abord cette mosaïque pour en extraire ses arêtes
reste interdit. Le sampler actuel ne prouve de toute façon pas que son
sur-ensemble est le graphe de Delaunay d'ordre onze.

**Q3 — ordonnance.** Oui : employer les certificats de rectangle bon marché
avant la descente, puis construire directement le résiduel fini. q2 dispose du
minimum blockwise exact `MidballBlockDepth`; q3/q4 utilisent le cœur universel,
`BlockJungDual64/tau(F)` si amorti, puis pieds q3 et intersections shallow q4.
Le BJD placé après la descente ne retire aucune recertification. Cette dernière
source est toutefois **pair-level** : lorsque `(a,b)` varie dans un rectangle
CK, son plan médiateur, ses lignes, pieds et intersections changent. Aucun
arrangement shallow commun au rectangle n'est encore prouvé ; sans nouveau
classifieur paramétré, la couture développe les `PairId` et reperd la
factorisation.

**Q4 — contrat.** Ni la cible principale `100 ms`, ni la cible secondaire
`1 s` à `50000` ne change. Une extrapolation « cent nanosecondes par record »
n'est pas une réception : il faut d'abord mesurer opérations, octets, HWM,
sorties et temps sur le chemin complet, puis seulement lancer une G4 avec la
recette fail-closed réparée. Les quelque `5,3` millions de records obtenus en
prolongeant la dernière pente locale exigent
soit davantage de fermeture/fusion, soit un kernel massivement parallèle ; ils
ne justifient pas de relâcher la cible.

**Q5 — central avant descente.** Pour q2, oui exactement : le minimum de
`H(a,b,z)=(z-a) dot (b-z)` sur trois AABB est la somme des trois minima parmi
les huit triplets de bornes par axe. Il doit être évalué avant fill. Pour une
base de témoins **fixe** et des poids fixes, les 64 coins BJD donnent aussi le
vrai verdict uniforme sur les deux boîtes endpoint. Si la boîte témoin varie,
`Q=||z||^2` et la norme du cône couplent les variables : la même conclusion ne
suit pas du seul minimum de `A0`. Il faut un wrapper exact reçu, un scan capé
des vrais IDs ou un split ; sinon le verdict reste `MIXED`.

## 9. Fixtures et portes permanentes

- q2 : milieu et rayon exacts, `D=0`, shell strict, dix IDs distincts ;
- triangle droit : troisième point shell mais support minimal q2 ;
- fixtures u16 q3/q4 du §2 : q2 fermé, cofaces vides et retenues ;
- fixture u16 du §2.1 : q3 fermé, face carrier conservée et q4 retenu ;
- cœur affine : segment/plan exacts, point hors affine et égalité shell ;
- parité `F_z(w)` contre `BallForm` rationnelle sur tous les supports à petit
  `n` ;
- q3 : pied de ligne, ligne parallèle/dégénérée, positivité aiguë/obtuse ;
- q4 : intersections `P-P/N-N/P-N`, owner parmi six arêtes, orientation,
  concurrence de trois lignes et extra-shell ;
- IDs non denses, coordonnées dupliquées avec multiplicité, permutation
  Morton et exact-once ;
- comparaison du catalogue edge-local complet à l'oracle exhaustif q2/q3/q4 ;
- mutant `midball-prune-q3q4`, tué par les deux configurations du §2 ;
- diagnostic pairwise : libellés exact/majorant séparés, seed rejouable,
  plancher non vacuaire et aucune prétention Hoeffding sans modèle reçu.

La propriété de miniboule unique ouvre une source finie plus directe ; elle ne
reçoit encore ni le coût global, ni le chemin device, ni `BallEvent -> 0B`, ni
le contrat 50 000/G4.

## 10. Rejeux ponctuels du contre-audit

```text
build ciblé wspd_wavefront_probe : vert
CTests mhgp3v_bjd_* au HEAD 694920a : 8/8 verts en 0,26 s
eight_clusters n=200, S=1000, seed=1 :
  q2 ouverte=0,198 ; q3=0,520 ; q4=0,559 ; scans=198000
delta exhaustif n=200 : 19900 paires ; U<h=3790/10059/10937 ; 3184359 tests
delta exhaustif --points=8,9 : seulement 28 paires de n=8, n=9 sauté
delta : exige-q4-ouvert sans juge et collinear_seven n=200 refusés code 2
contre-calcul rationnel des deux fixtures : puissances et barycentriques exactes
```

Le rejeu du sampler confirme sa sortie déterministe, pas l'interprétation
probabiliste des crochets ni les extrapolations de pente. GCP non utilisé par
le présent auditeur.
