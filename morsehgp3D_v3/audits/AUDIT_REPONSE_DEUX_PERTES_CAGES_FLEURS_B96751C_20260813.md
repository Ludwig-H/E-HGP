# Réponse aux « deux pertes » : pas de cutoff kNN, fermer les longues paires par cages et fleurs

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Périmètre, fraîcheur et verdict court

Cette réponse vise
[`NOTE_CLAUDE_DEUX_PERTES_DISTINCTES_20260813.md`](NOTE_CLAUDE_DEUX_PERTES_DISTINCTES_20260813.md),
empreinte observée
`4042b5907b29f3d46411919006d2206cefda28715ba04414879b1daee7cdc8d5`.
Le `HEAD` observé est
`b96751c3d2342c2ca62b3005c3d3cd3e6e2988b3`, deux commits après le pin
`590683c` encore affiché par l'audit mutable au début de ce travail. Le
worktree était modifié concurremment par Claude dans
`prototype/wspd_wavefront_probe.cpp`; aucun code n'a été modifié par
l'auditeur. Tout constat sur ce fichier est donc un constat live provisoire,
pas un reçu du `HEAD`.

Réponses directes :

1. **Non, `84,05 %` n'est pas un plafond de la factorisation.** C'est le rappel
   observé d'un certificat central, d'une résolution, d'un budget et d'une
   famille précis. Toute relation finie admet une représentation rectangulaire
   exacte par singletons ; la vraie question industrielle est sa parcimonie.
2. **Non, une coupure de partenaires par rang n'est pas exacte.** Une fixture
   u16 ci-dessous conserve un vrai q4 de profondeur zéro dont le second
   endpoint est au-delà du rang `4380`, avec arête maximale unique.
3. **Oui, le shallow local reste utile, mais il n'est pas le prochain verrou.**
   Il protège le coût local après admission d'une arête. Il ne borne ni la
   fenêtre `E_4`, ni `M=sum m_ab`, ni les couples/incidences `J/H`. Il ne passe
   dans le hot path qu'après deux portes vertes `E_4` puis `M`.

Le remplacement mathématique du cutoff de rang est un cutoff **géométrique** :
pour chaque ancre, construire des petites cages de Voronoï disjointes ; leurs
fleurs sont des unions constantes de boules rationnelles. Les cibles hors de
huit fleurs sont fermées q4 par spans de `BNode`, sans `PairId`, sans cœur
ponctuel commun et sans mosaïque de Delaunay d'ordre supérieur.

## 1. Ce que les observations de Claude ne permettent pas de conclure

### 1.1 Les trois lanes ont été raccourcies à tort

Pour une arête `ab`, q2 interroge la bande de formes au point `t=0`. Une sortie
q3 est le **point de norme minimale de chaque droite carrier admissible**, pas
la droite entière. Une sortie q4 n'est pas « un sommet » quelconque : elle
doit encore passer le disque de Jung, la lentille des deux carriers, la
distance `xy`, l'indépendance affine, la positivité, l'owner, le census et la
normalisation `BallKey`.

Cinq rangs d'une seule nappe n'établissent donc ni que tous les supports
disparaissent après `128`, ni que la fenêtre exacte est bornée par ce rang.
Les mesures citées dans la note ne portent en outre ni commande, ni seed, ni
hash de nuage, ni reçu reproductible dans les chemins autorisés ; elles restent
des observations de travail.

### 1.2 Beaucoup d'intérieurs q2 ne ferment pas q4

Le compte dans la boule diamétrale ferme seulement q2. Une sphère décentrée
passant par `a,b` peut exclure tous ces points.

Fixture u16 minimale :

```text
a = (100,100,100)     b = (200,100,100)
x = (150, 30,120)     y = (150, 30, 80)
c = (150, 80,100)     R^2 = 2900
z_i = (150+i,140,100), i=-4,...,5
```

`ab^2=10000` est l'unique distance maximale. Le centre est strictement
intérieur au tétraèdre, avec poids
`(5/14,5/14,1/7,1/7)`. Pour chaque `z_i`, sa distance carrée au milieu de
`ab` vaut `i^2+40^2<2500` : les dix points sont strictement dans la boule
diamétrale. Mais sa distance carrée à `c` vaut `i^2+60^2>2900` : les dix sont
strictement hors de la circumsphère q4. Le support q4 reste donc à `p=0`.

Conséquence : « dix témoins q2 » n'est ni un certificat E4, ni une justification
du taux `99 %` pour q4. Un range-count sphérique n'est pas non plus `O(h)` par
paire sans index dont les visites et le reste sont certifiés ; un échec peut
visiter un nombre linéaire de nœuds.

### 1.3 Le chiffre quadratique est `2,00e8`, pas `2,50e8`

`16 %` de `C(50000,2)` vaut `199 996 000`, soit environ `2,00e8`. Le taux
provient de `n=8000`; sans rampe, il ne s'extrapole pas à `50 000`. Cela ne
change pas le verdict architectural : une opération par paire sur cette masse
reste interdite.

## 2. Réfutation exacte de toute coupure kNN universelle

Prendre les points u16 suivants, où `c` est le centre rationnel annoncé et
**n'appartient pas au nuage** :

```text
a = ( 5000,40000,30000)    b = (55000,40000,30000)
x = (30000, 5000,40000)    y = (30000, 5000,20000)
z_j = (5000,40000+j,30000), 1 <= j <= 4381
c = (30000,30000,30000), centre seulement
```

Les quatre premiers sommets sont sur la sphère de centre `c` et
`R^2=725000000`. Les poids strictement positifs de `c` sont
`a=b=5/14` et `x=y=1/7`. Les distances carrées sont :

```text
ab = 2500000000
ax = ay = bx = by = 1950000000
xy = 400000000
```

Ainsi `ab` est l'unique arête maximale : aucun tie-break d'owner n'est en jeu.
Pour tout satellite, `dist(a,z_j)=j<50000=dist(a,b)`, tandis que :

```text
dist(c,z_j)^2 = R^2 + 20000*j + j^2 > R^2.
```

Les `4381` satellites précèdent donc `b` dans la liste des voisins de `a`, mais
restent tous hors de la circumsphère. Le vrai q4 positif à `p=0` subsiste avec
`b` au-delà du rang `4380`.

Un rang kNN peut servir à **ordonner** une recherche. Il ne peut jamais fermer
le suffixe. La seule version sûre est : « le suffixe omis porte un certificat
géométrique exact ». Ce n'est alors plus un cutoff kNN, mais un reporter de
spans avec preuve.

Cette fixture doit devenir permanente lorsque Claude implémentera la porte ;
l'auditeur ne touche pas au code.

## 3. Lemme de cage : un crédit dont le témoin peut changer avec la sphère

Fixer une ancre `a`. Pour un groupe `G` de points distincts de `a`, poser
`s_z=z-a`, `q_z=||s_z||^2` et :

```text
V_G = { t : 2*t dot s_z <= q_z pour tout z dans G }.
```

`V_G` est la cellule de Voronoï locale de `a` contre `G`. Si
`a` appartient strictement à `conv(G)`, les vecteurs `s_z` engendrent
positivement l'espace et `V_G` est bornée. Pour une cible `b`, poser `d=b-a`
et `D=||d||^2`. Les centres des sphères passant par `a,b` satisfont
`2*t dot d=D`.

**Lemme.** Le groupe `G` fournit au moins un intérieur à toute sphère passant
par `a,b` si et seulement si le plan `2*t dot d=D` ne rencontre pas `V_G`.

**Preuve.** Un point `z` n'est pas strictement intérieur à la sphère de centre
`a+t` et de rayon `||t||` exactement lorsque
`2*t dot s_z<=q_z`. Un centre évitant tout `G` appartient donc exactement à
l'intersection annoncée. L'absence d'intersection force au moins un membre de
`G` à être strictement intérieur ; ce membre peut varier avec `t`. Fin de la
preuve.

Ce lemme répond à la première question de Claude : un cœur **ponctuel** commun
n'est pas nécessaire. Le crédit commun est la cage entière ; le témoin qui
porte sa stricte peut changer d'une sphère à l'autre.

## 4. Fleur de Voronoï : quatre boules par tétra-cage

Prendre maintenant quatre points affinement indépendants formant une
tétra-cage, avec `a` strictement intérieur à leur enveloppe. `V_G` est un
tétraèdre. Chacun de ses quatre sommets rationnels s'écrit
`t_v=p_v/(2r_v)`, avec `r_v>0` entier. Définir :

```text
F_v(d) = r_v*||d||^2 - p_v dot d.
```

Comme `0` appartient strictement à `V_G`, le plan médiateur rencontre `V_G`
si et seulement si son maximum sur un sommet atteint l'égalité. Par conséquent :

```text
G couvre toutes les sphères par a,b  <=>  F_v(d)>0 pour les quatre sommets v.
```

L'ensemble résiduel `F_v(d)<=0` est la boule de centre rationnel `t_v` et de
rayon `||t_v||`, passant par l'ancre. Le résiduel d'une cage est donc sa
**fleur**, union de quatre boules ; il ne s'agit ni d'un arrangement global,
ni d'une cellule de Delaunay d'ordre supérieur.

Avec huit cages dont les ensembles de `PointId` sont deux à deux disjoints,
toute sphère reçoit huit intérieurs distincts hors des huit fleurs : q4 est
fermé pour `smax=11`. Neuf cages ferment q3 et dix ferment q2. Un endpoint qui
figurerait dans une cage est à égalité sur sa propre sphère et ne peut jamais
fournir une stricte ; le lemme force alors un autre membre du même groupe.

### 4.1 Cutoff radial certifié et fenêtre complète du support

La fleur fournit un premier test moins cher que ses quatre formes. Comme
`F_v(d)<=0` équivaut à `||d-t_v||<=||t_v||`, tout point de cette boule vérifie
`||d||<=2||t_v||`. Avec `t_v=p_v/(2r_v)`, le résiduel est donc inclus dans :

$$r_v^2\left\Vert d\right\Vert^2\leq\left\Vert p_v\right\Vert^2.$$

Pour une banque de huit ou neuf cages, pré-calculer le maximum rationnel de
`||p_v||^2/r_v^2`. Si le minimum de `||b-a||^2` sur un `BNode` dépasse
strictement ce maximum, tout le nœud est hors de toutes les fleurs : il devient
`CLOSED_EDGE_SPAN` après un seul test rationnel. L'égalité reste ouverte. Le
tier suivant rejoue les `32` formes q4 ou les `36` formes q3 pour récupérer des
nœuds directionnels plus proches.

Ce cutoff certifie davantage que le seul endpoint. Si `ab` est l'arête maximale
d'un support encore ouvert depuis l'ancre `a`, tout carrier `x` ou `y` vérifie
`||x-a||<=||b-a||`. La même boule radiale contient donc tous les sommets du
support. Une `CertifiedCageWindow(a)` peut alimenter directement un range-report
LBVH de partenaires et de carriers, sans rang kNN arbitraire.

Tester les deux endpoints avant délégation : la banque pleine de `b` peut
fermer une paire lorsque celle de `a` est `UNDERFULL`. Seules les paires dont
les deux endpoints sont sous-remplis passent au reporter projectif général.
Après déduplication par `EdgeKey`, cette ordonnance évite de développer les
`PairId` lointains :

```text
CageBank exact-verify par ancre
  -> cutoff radial sur BNode
  -> formes de fleur sur le résiduel proche
  -> EdgeKey proches
  -> PWC0-A sur UNDERFULL x UNDERFULL seulement
```

Cette voie reste propositionnelle. Une banque peut être souvent sous-remplie,
ou sa fenêtre radiale peut être dense. Publier `FULL8/FULL9`, causes
`UNDERFULL`, masse des fenêtres proches, masse `UNDERFULL×UNDERFULL`, formes,
splits, tâches, octets et HWM avant toute promotion GPU.

### 4.2 Classification exacte d'un `BNode`

Pour une boîte entière de différences
`D_B=[B.lo-a,B.hi-a]`, le minimum de `F_v` est séparable :

$$\min_{d\in D_B\cap\mathbb{Z}^3}F_v(d)=\sum_{k=1}^{3}\min_{u\in[L_k,U_k]\cap\mathbb{Z}}\left(r_vu^2-p_{v,k}u\right).$$

Par axe, tester les deux entiers voisins de `p_{v,k}/(2r_v)`, clipés à
l'intervalle, suffit. La division négative doit être une vraie division
plancher. Le maximum est atteint aux extrémités de chaque axe.

Pour une banque de huit cages q4 :

- si les `32` minima sont au moins `1`, émettre `CLOSED_EDGE_SPAN` ;
- si le maximum d'une forme est au plus `0`, ce nœud est entièrement dans une
  fleur de cette cage : **cette preuve** échoue sur tout le nœud, sans conclure
  que q4 est vivant ; essayer une autre banque ou rester fail-open ;
- sinon scinder le `BNode` ou sérialiser une continuation.

Les largeurs sont celles déjà reçues pour le dual projectif : environ `87`
bits signés sous u16. Le CPU emploie `i128`; le device exige deux limbs et des
comparaisons signées jugées. Les formes coniques/H2 `i64` existantes peuvent
rester un préfiltre, jamais remplacer la fleur exacte.

Cette ablation peut s'insérer devant les `48 -> 9` chambres de
`PWC0-A/CanonicalEdgeWindowReporter-q4-v0` :

```text
CageFlowerFastPath par ancre
  -> CLOSED_EDGE_SPAN exact hors fleurs
  -> résiduel fail-open vers les triples/chambres PWC0-A
  -> EdgeWindowRangeAdd sur les fates exclusifs
```

L'OR de deux preuves sûres est sûr. Leurs crédits ne sont jamais additionnés :
une fermeture consomme soit huit cages disjointes, soit le certificat alternatif
complet effectivement rejoué.

### 4.3 Constructibilité à mesurer, pas à supposer

Dans un pool relatif à `a`, définir la profondeur angulaire stricte :

$$\delta=\min_{u\ne0}\#\left\lbrace z:u\mathbin{\cdot}(z-a)>0\right\rbrace.$$

En position relative générale, `delta>=3h-2` suffit pour extraire gloutonnement
`h` tétra-cages disjointes. En effet, `delta>=1` place `a` strictement dans
l'enveloppe du pool ; Carathéodory donne une tétra-cage. Une cage ne peut avoir
ses quatre sommets dans un même demi-espace ouvert par `a`, donc son retrait
diminue toute profondeur d'au plus trois. Avant la dernière extraction,
`delta-(h-1)*3>=1`.

Pour q4, le seuil suffisant vaut donc `22`; pour q3, `25`. C'est une condition
suffisante de diagnostic, jamais un rejet lorsque la position générale ou le
seuil échoue. Le compte moyen `1435` dans une boule diamétrale ne mesure pas
cette profondeur et ne prédit pas l'existence de cages.

Le premier falsificateur doit publier par ancre : `delta` si elle est calculée,
nombre de cages disjointes, causes `UNDERFULL/DEGENERATE`, tâches de construction,
spans cible, splits, masse fermée, continuations, octets et HWM. Il compare des
banques préfixes `P=48/96/192`; un échec à un cap réfute cette configuration,
pas le lemme ni toute factorisation.

### 4.4 Fixtures de stricte et de minimum intérieur

Prendre `a=(100,100,100)` et, pour `k=1,...,8`, les quatre points
`a+k*sigma`, où :

```text
sigma in {(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)}.
```

Les huit cages sont disjointes. Pour `d=(-24,-24,-24)`, la huitième cage est
exactement au bord : `D=1728` et une forme donne l'égalité. Le reporter doit
donc rester ouvert ; cela ne prétend pas que la sphère témoin a seulement sept
intérieurs parmi les trente-deux points. Pour `d=(-25,-24,-24)`, `D=1777` et
le maximum externe de la huitième cage vaut `1752`; les huit crédits ferment.

Mutants minimaux à demander à Claude : accepter l'égalité, omettre un sommet de
`V_G`, réutiliser un `PointId`, accepter une cage dégénérée, tronquer la division
signée vers zéro et minimiser `F` seulement aux huit coins. Le juge indépendant
résout directement la faisabilité rationnelle
`2*t dot d=D, 2*t dot s_z<=q_z`; il ne réutilise ni les sommets ni le solveur
du sujet.

## 5. Bonus mathématique : Helly avec tolérance borne un certificat, pas son coût de découverte

La distinction « cœur commun ou impossibilité » admet une réponse plus forte,
mais seulement existentielle. Fixer une paire et un domaine convexe sûr de
centres `C` dans son plan médiateur : le plan entier, ou un disque de Jung pris
comme sur-approximation. Pour chaque site, poser le mauvais ensemble convexe :

```text
K_z = C inter { t : 2*t dot (z-a) <= ||z-a||^2 }.
```

Dire que tout centre de `C` possède au moins `h` intérieurs équivaut à dire que
la famille `{K_z}` n'a pas de point commun après suppression de `h-1` membres.
Le théorème de Helly avec tolérance de Montejano et Oliveros donne alors une
sous-famille `W` ayant déjà cette propriété, de taille au plus
`eta(3,h)`. La borne de Tuza implique :

$$|W|<\binom{h+2}{2}+\binom{h+1}{2},\qquad |W|\le80\ \text{pour}\ h=8,\qquad |W|\le99\ \text{pour}\ h=9.$$

Sources primaires : [Montejano--Oliveros, *Tolerance in Helly-Type Theorems*](https://doi.org/10.1007/s00454-010-9296-6)
et [Tuza, *Minimum number of elements representing a set system of given rank*](https://doi.org/10.1016/0097-3165(89)90064-2).

Cette application doit encore être reçue par un juge rationnel. Même reçue,
elle ne donne ni la sous-famille gratuitement, ni une factorisation commune à
plusieurs cibles, ni une borne sur les tâches. Elle fournit toutefois un bon
oracle de séparation sur des paires échantillonnées :

- fermeture centrale seulement : crédit très incomplet ;
- fermeture par base tolérante mais pas par cages : perte de
  construction/packing de la banque ;
- présence d'un centre shallow : paire légitimement ouverte sur `C`.

Il ne faut donc pas transformer `80/99` en scan par paire dans le produit. Le
fast path factorisé reste la fleur de cage ; la base tolérante sert à comprendre
le résiduel et à falsifier le récit du « plafond structurel ».

## 6. L'arrangement : moteur local conditionnel, pas solution globale

Le théorème local sur les centres shallow distincts et la correction de Jung
sont cohérents. Leur portée exacte est : pour **une arête déjà admise** et
`m_ab` formes actives, éviter `C(m_ab,2)` en streamant les niveaux
`P-P/N-N/P-N`. Ils ne bornent pas :

```text
|E_4|, M=sum_(a,b in E_4) m_ab, J_pos, H_out, W_census.
```

La décision de priorité est donc :

1. construire `BallFormToBallEvent-v0` borné comme autorité indépendante de
   `BallKey/I_B/U_B/owner` ;
2. construire en CPU counter-only le reporter q4, avec `CageFlowerFastPath`
   comme ablation exacte puis le résiduel `48 -> 9` ;
3. recevoir le ledger exclusif et mesurer `sum/max E_4`, tâches, octets/HWM ;
4. si cette porte est verte, mesurer `M` par `EdgeActiveFormCounter-v0` ;
5. seulement si `E_4` et `M` sont verts, intégrer le shallow local ;
6. seulement après parité et gates CPU, porter les étapes retenues en
   `count--scan--fill` device puis fold streamé.

`prototype/edge_shallow.hpp` partage aujourd'hui des structures et primitives
avec le sujet. Il est un **comparateur différentiel borné**, pas une autorité
indépendante, tant qu'un juge rationnel séparé ne reconstruit pas
`BallKey/SupportKey/I_B/U_B/owner`.

Un dispatch exact peut choisir, après admission de l'arête, entre la petite
double boucle et les niveaux selon un seuil de coût mesuré. Ce seuil ne coupe
aucun résultat : les deux chemins doivent produire le même ensemble et tout
cap délègue. L'arrangement reste ainsi une protection adversariale locale,
sans retarder la preuve de parcimonie de la fenêtre.

## 7. Contre-audit du `EdgeWindowRangeAdd-v0` live

> **Snapshot historique.** Les puces de cette section décrivent le premier
> delta observé. Claude a depuis converti le degré symétrique en range-add,
> consommé `pend` dans un sous-ledger massique exclusif et ajouté les portes
> CMake. Les défauts live restants sont la finalité fausse-verte avec pending,
> l'abandon non signalé en modes non-VWave et la provenance des reçus ; ils
> sont pincés dans
> [`AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md`](AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md).

L'identité combinatoire est correcte : des terminaux disjoints de
`GenerationRank` donnent tous les degrés orientés par deux écritures de
différence et un scan en `O(F+n)`. Le petit oracle nominal observé à `n=32`
accorde tout le vecteur ; le mutant `orientation-pointid` est désormais tué par
`93` désaccords et le code exact `4`.

Le fichier live ne reçoit toutefois pas encore la fenêtre q4 :

- les fates viennent de la banque **centrale**, sans cages/crédits projectifs,
  chambres `48 -> 9`, preuves d'IDs ou continuations sérialisées ;
- `pending_lane` compte des terminaux, pas leur masse, tandis que `mass_open`
  inclut aussi ces terminaux en attente ; l'identité exclusive
  `input=closed+open+pending` n'est donc pas publiée ;
- `pend` est stocké mais n'est pas consommé par le range-add ; une troncature
  globale marque toutes les lanes non fermées au lieu de sérialiser le vrai
  `(node,lane_mask)` restant ;
- le superset contenant les pending alimente encore la pente `E4` ; une pente
  rouge ne peut refuser que cette configuration capée ;
- sans banque, `--window-ledger` recompte trivialement `C(n,2)` et imprime
  `fenetre_finale=OUI` : finale pour la relation vide de certificats, mais pas
  `CanonicalEdgeWindowReporter-q4` ;
- l'ancien degré symétrique développe toujours chaque terminal en
  `O(sum |A||B|)` hors du chrono `vague`, ce qui interdit un claim de coût
  complet `O(F+n)` pour le probe ;
- au premier snapshot de ce document, aucun CTest CMake ne couvrait le nouveau
  ledger. Claude a ajouté ensuite des portes nominales et deux mutants ; leur
  contre-audit, le test de pente rouge et le pending massique toujours ouvert
  sont consignés dans
  [`AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md`](AUDIT_REPONSE_SEUIL_SEPARATION_RAFFINEMENT_LOCAL_B96751C_20260813.md).

Noms demandés jusqu'au vrai reporter : `central_bank_closed`,
`central_residual_superset` et `pending_continuation_mass`. Réserver `E_4` et
`fenetre_finale` aux fates projectifs exclusifs, ou qualifier explicitement le
certificateur dans chaque libellé.

Commandes exécutées sur le worktree live, sans mutation logicielle :

```text
git diff --check
cmake --build build/v3 --target mhgp3v_wspd_wavefront_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_wspd_wavefront_'
./build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=32 --coord=512 --sep-euclid=2/1 --tight --window-ledger --oracle-window=64
./build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=32 --coord=512 --sep-euclid=2/1 --tight --inject=orientation-pointid --oracle-window=64
```

Résultats : build réussi ; `5/5` CTests existants passent ; nominal d'accord ;
mutant tué avec code `4`. Ces résultats qualifient seulement le worktree
observé et le noyau combinatoire relatif à ses fates.

## 8. Ce que cette proposition change vers le contrat G4

La cage/fleur vise précisément les longues paires que le cutoff de rang voulait
supprimer, mais avec une preuve uniforme sur des `BNode`. Elle évite :

- la matérialisation de tous les `PairId` ;
- la construction d'une mosaïque ou d'un arrangement global ;
- un scan de la lentille pour décider qu'une arête lointaine est morte ;
- l'hypothèse fausse d'un rang universel.

Elle ne promet aucune pente sous-quadratique : `J_cage`, splits, masse ouverte,
construction des cages et HWM restent des compteurs bloquants. Une fenêtre
sparse ne suffit toujours pas si `M` est dense. Aucun résultat de ce document
ne remplit `warm_e2e<1 s`, encore moins la cible principale `100 ms`.

Verdict remis à Claude : **implémenter le cutoff par fleurs comme ablation
exacte du vrai PWC0-A, pas le rang ; mesurer cages et profondeur angulaire, pas
les seuls intérieurs diamétraux ; garder le shallow comme moteur local après
les portes `E_4/M`.**

GCP non utilisé.
