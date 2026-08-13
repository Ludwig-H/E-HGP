# Réponse au seuil de séparation : raffiner jusqu'à une preuve, pas jusqu'à `rho>0`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin observé et réponses aux trois questions

Cette réponse vise
[`NOTE_CLAUDE_SEUIL_EXACT_DE_SEPARATION_20260813.md`](NOTE_CLAUDE_SEUIL_EXACT_DE_SEPARATION_20260813.md),
empreinte observée
`600b049c823572241178ed4e71d4bb2a308ee5109c7d1941da72ba68e86b1c61`.
Le `HEAD` reste
`b96751c3d2342c2ca62b3005c3d3cd3e6e2988b3`. Le worktree contient des
mutations concurrentes de Claude dans `CMakeLists.txt`,
`prototype/wspd_wavefront_probe.cpp`, la note et un nouveau dossier de reçus.
Au snapshot du `13 août 2026 20:51:52 UTC`, les empreintes live du probe et de
CMake étaient respectivement
`2d4a4bfb46313b18973dc714fd4e4b1a3fa8a96dd6558fc34f8cbc2173ec62a7`
et `065f9f32af2da97467e410d04cbfc94853861366803eea8041d0241a5465c241`.
L'auditeur n'a modifié aucun fichier logiciel.

Réponses courtes :

1. **Oui à l'intervalle exact de `P=(b-a) dot (2z-a-b)` ; non à une
   équivalence rectangle après décorrélation de `D2`, `V2` et `P`.** `P`
   se borne exactement en `O(1)` par axe, sans se limiter aux coins. Le verdict
   qui combine ses extrema aux extrema de `D2/V2` est un certificat `ALL`
   suffisant et fail-open. Il doit prolonger `rect_classify`, pas dupliquer le
   spindle déjà présent dans `prototype/spindle_cone.hpp`.
2. **Oui au raffinement local des seuls terminaux ouverts ; non à `rho>0`
   comme critère d'arrêt.** `rho>0` prouve seulement l'existence d'une boule
   continue garantie où chercher des témoins. La fermeture exige encore huit
   `PointId` distincts. `rho<=0` ne prouve pas que le cœur réel est vide.
   L'arrêt exact est `credit4>=8`; un cap donne
   `PENDING_CONTINUATION`, jamais `OPEN` final.
3. **Oui à une tâche partagée avec `lane_mask` et critères locaux par lane ;
   non au choix automatique « juste au-dessus du seuil ».** Les seuils sont des
   bornes de non-vacuité garantie, pas les minima du travail total. q4 doit
   rester la première lane à falsifier avant de complexifier la traversée.

La tendance `13,9 %` est traitée plus bas comme question scientifique
supplémentaire, mais ce n'était pas la question numérotée 2 de Claude.

## 1. Ce que la proposition de `rho` prouve exactement

Fixer deux ensembles de cibles inclus dans les boules
`B(c_A,r_A)` et `B(c_B,r_B)`. Poser `d=||c_B-c_A||` et
`R=r_A+r_B`. Pour une paire `a,b`, son milieu varie d'au plus `R/2` autour de
`m_0=(c_A+c_B)/2`, tandis que `||b-a||>=d-R`.

Si le certificat ponctuel d'une lane contient pour chaque paire la boule de
centre `(a+b)/2` et de rayon `tau*||b-a||/2`, alors l'intersection de tous ces
cœurs contient la boule centrée en `m_0` de rayon minoré :

$$\rho_{\mathrm{lb}}=\frac{\tau}{2}(d-R)-\frac{R}{2}.$$

La preuve de Claude reçoit cette **inclusion**. Elle ne reçoit pas l'égalité
« le cœur commun est cette boule ». Les AABB portent un ensemble discret de
points, pas toutes les boules circonscrites ; les inégalités triangulaires
peuvent être strictes et l'intersection peut être non sphérique. Deux
conséquences sont impératives :

- `rho_lb>0` ne fournit aucun témoin : la boule peut ne contenir aucun point du
  nuage, encore moins huit IDs distincts ;
- `rho_lb<=0` ne prouve aucune vacuité : la minoration a seulement cessé de
  parler.

La phrase « en dessous du seuil, le cœur est vide pour tout nuage » doit donc
être remplacée par : **« en dessous du seuil, le seul invariant de séparation
ne garantit plus une boule inscrite de rayon strictement positif dans le pire
cas ».** La nuance de la section Non-claims de Claude ne répare pas le théorème
énoncé plus haut ; elle en est précisément la correction.

### 1.1 Seuil idéal et seuil réellement codé

Sous la séparation suffisante `d>=(s+2)r` et
`r_A+r_B<=2r`, la minoration devient
`rho_lb>=r*(tau*s/2-1)`. Un rayon garanti positif exige `s>2/tau`.

Pour la boule centrale idéale :

```text
q2 : tau = 1                         -> s > 2
q3 : tau = 1/sqrt(3)                 -> s > 2*sqrt(3)
q4 : tau = sqrt(2-sqrt(3))           -> s > sqrt(6)+sqrt(2)
```

Mais le probe ne code pas la frontière algébrique q4. Il emploie le certificat
rationnel sûr `209*V2<=56*D2`, donc
`tau_code=sqrt(56/209)`. Son seuil garanti exact est :

$$s>\sqrt{\frac{209}{14}}=3{,}863750953228\ldots,$$

et non `sqrt(6)+sqrt(2)=3,863703305156...`. L'écart est petit mais un audit
exact ne peut les identifier. Pour `s=p/q`, les trois tests entiers de rayon
strictement positif sont :

```text
q2 : p > 2*q
q3 : p*p > 12*q*q
q4 codé : 14*p*p > 209*q*q
```

Ces tests sont des **planchers de résolution**. Ils ne sont ni une fermeture,
ni un motif de rejet d'un rectangle, ni un choix de performance optimal.

### 1.2 Contre-fixture : `rho_lb<0` mais huit crédits q4 centraux

Le seuil n'est même pas nécessaire au certificat factorisé courant. Prendre :

```text
A = {(100,90,100),(100,110,100)}
B = {(150,90,100),(150,110,100)}
C = {124,126} x {99,101} x {99,101}
```

Les deux boules englobantes ont `r_A=r_B=10`, leurs centres sont à `d=50`,
donc elles réalisent exactement `d=(s+2)r` avec `s=3`. Pourtant
`rho_lb=15*sqrt(2-sqrt(3))-10<0`. Sur le produit discret, les extrema exacts
sont `Dlo=2500` et `Vhi=492`, d'où :

```text
209*Vhi = 102828 < 140000 = 56*Dlo.
```

Les huit `PointId` de `C` sont donc tous crédités et le rectangle ferme q4.
Cette fixture réfute simultanément « sous le seuil le moteur ne peut rien
fermer » et tout arrêt de raffinement fondé sur le seul signe de `rho_lb`.

La frontière rationnelle codée n'est pas non plus la frontière idéale, même
sur u16. Avec `a=(1000,1000,0)`, `b=(2588,1000,0)` et
`z=(1794,1411,0)`, on a `D2=2521744`, `V2=675684` et `P=0`. Le rapport
`V2/D2` est strictement inférieur à `2-sqrt(3)`, mais
`209*V2=141217956>141217664=56*D2`. L'idéal accepte ce point et
`CentralBall209-v0` le rejette : l'approximation est conservative, pas exacte.

## 2. Réponse Q2 : la bonne boucle de raffinement local

Le raffinement local est sound si chaque lane transporte un sort exclusif et
si tout split remplace son parent par une partition exacte de ses enfants. Il
ne doit pas être piloté jusqu'à `rho>0`, mais jusqu'à un certificat ou une
délégation.

Pour le certificateur central déjà codé, `Dlo` est le minimum exact de
`||b-a||^2` sur `A×B` et `Smax` le maximum exact du score sur un nœud témoin
`C`. Les marges de crédit sont directement entières :

```text
q2 : Dlo - Smax > 0
q3 : Dlo - 3*Smax > 0
q4 : 56*Dlo - 209*Smax >= 0, avec Dlo>0
```

La boucle exacte recommandée à Claude est :

1. une tâche persistante porte `(ANode,BNode,CNode,lane_mask)` ou la
   factorisation équivalente déjà reçue ;
2. un `CNode ALL` crédite son antichaîne de vrais `PointId` une seule fois ;
3. une lane termine `CLOSED_EDGE_SPAN` dès qu'elle a `h_q` IDs distincts ;
4. si toutes ses tâches sont épuisées sans le seuil, elle devient
   `OPEN_EDGE_SPAN` **pour ce certificateur** ;
5. si un quantum, une pile ou un buffer finit avant les tâches, elle devient
   `PENDING_CONTINUATION`, avec curseur et masse sérialisés ;
6. un split de cible ou d'ancre est priorisé par l'amélioration de la marge
   exacte ou par `Vbest`, mais son nombre de tâches, ses octets et sa HWM sont
   gatés.

`rho_lb>0` peut servir à choisir une tâche prometteuse. `rho_lb<=0` peut
justifier de scinder parce que le minorant est devenu muet, jamais parce qu'une
boule aurait été prouvée vide. Aucun des deux ne décide le fate. Un rectangle positif peut rester sans
point ; un rectangle négatif peut fermer après split, ou même avant via les
extrema exacts et une géométrie favorable.

Le danger industriel est clair : raffiner jusqu'aux feuilles récupère le rappel
par paire en payant jusqu'à `C(n,2)` terminaux. La porte doit donc porter sur
`target_splits`, `witness_tasks`, `classifications`, masse pending, octets et
HWM, pas seulement sur la fraction fermée. Une récupération de `71 %` après
travail quadratique reste un NO-GO.

## 3. La boule centrale n'est pas le cœur q4 maximal, même par paire

La condition `209*V2<=56*D2` compte exactement les points de la **boule
centrale rationnelle codée**. Elle est volontairement plus petite que la boule
idéale et beaucoup plus petite que le cœur anisotrope obtenu avec le terme
directionnel favorable.

Sous l'hypothèse que `ab` est l'arête maximale canonique, tout centre q4
pertinent appartient au disque de Jung du plan médiateur. Poser
`d=b-a`, `L2=||d||^2`, `v=2z-a-b`, `V2=||v||^2` et
`P=d dot v`. Un site est intérieur à toutes les sphères du **disque de Jung
sur-approché** si et seulement si :

$$L2>V2\quad\text{et}\quad (L2-V2)^2>2(V2L2-P^2).$$

Cette identité vient du maximum de
`||u||^2-2*t dot u` sur
`t perpendicular d`, `||t||<=||d||/(2*sqrt(2))`. Elle est exacte pour ce
domaine convexe de centres ; elle reste un certificat suffisant pour les vrais
supports, car le domaine réel peut être plus petit. Sur l'axe de `ab`, elle
atteint la boule diamétrale entière ; dans le plan orthogonal, elle redonne la
frontière `V2/L2<2-sqrt(3)`.

Ce n'est pas une nouvelle géométrie. Avec
`H=(b-z) dot (z-a)`, `R=||(b-z) cross (z-a)||^2`, on a exactement
`L2-V2=4H` et `L2*V2-P^2=4R`. Le test ci-dessus est donc le q4 déjà présent
dans `prototype/spindle_cone.hpp` :

```text
q4 : H>0 et 2*H^2>R, soit 3*H^2>E2*X2
q3 : H>0 et 3*H^2>R, soit 4*H^2>E2*X2
```

L'implémentation doit réutiliser ce prédicat et son oracle borné, ou expliquer
une ABI commune ; recopier l'algèbre dans le probe crée deux sujets susceptibles
de diverger.

Trois niveaux doivent donc être nommés séparément :

```text
CentralBall209-v0 : petite boule rationnelle, rapide et isotrope
JungSpindleSingleton-v0 : cœur anisotrope exact sur le disque de Jung
CageFlower-v0 : crédit de groupe couvrant toutes les sphères par a,b
```

Le live probe a ajouté un compteur appelé `coeur EXACT` et une fixture
`fixtures_spindle`. La fixture exerce un rejet et une non-vacuité ; son
balayage discret peut falsifier un crédit, mais son absence de centre excluant
ne prouve pas une appartenance continue. Il manque encore la parité avec
l'oracle spindle séparé, les mutants de frontière/terme `P^2`/largeur et un
reçu causal. Le mot exact doit rester qualifié « exact sur le disque de Jung et
sous owner maximal », jamais « exact pour toute sphère par la paire ».

### 3.1 Réponse Q1 : intervalle exact, certificat rectangle seulement suffisant

Le terme directionnel se simplifie avant toute borne. Avec `d=b-a` et
`v=2z-a-b` :

$$T=d\cdot v=\left\Vert z-a\right\Vert^2-\left\Vert z-b\right\Vert^2.$$

Il est donc séparable par coordonnée sur `A×B×C`. Pour une coordonnée et un
`z` fixé, les extrema sur `a,b` valent exactement :

```text
minimum = dist(z,A)^2 - far(z,B)^2
maximum = far(z,A)^2 - dist(z,B)^2.
```

Ces deux fonctions sont quadratiques par morceaux. Leurs extrema entiers sur
`C` se trouvent en un nombre constant d'évaluations : endpoints clipés,
ruptures des distances aux intervalles, plancher/plafond des milieux qui
séparent les deux endpoints les plus lointains, et voisins entiers de tout
sommet d'un morceau. La somme des trois intervalles unidimensionnels donne
l'intervalle exact `[Tlo,Thi]` parce que les trois axes sont indépendants.

Les seuls coins ne suffisent pas. En dimension un, pour
`A=[0,1]`, `B=[0,3]`, `C=[0,2]`, le maximum exact vaut `4` en
`(a,b,z)=(0,2,2)`, alors que les huit choix d'endpoints donnent au plus `3`.
Cette fixture doit tuer tout `dot_interval` réduit aux coins.

Poser `P2lo=0` si `[Tlo,Thi]` contient zéro, et sinon
`P2lo=min(Tlo^2,Thi^2)`. Avec des bornes sûres de `D2` et `V2`, le verdict
rectangle q4 suivant est sûr :

```text
D2lo > V2hi
(D2lo-V2hi)^2 > 2*(D2hi*V2hi-P2lo)
```

La variante q3 emploie
`3*(D2lo-V2hi)^2 > 4*(D2hi*V2hi-P2lo)`. Une forme généralement plus forte
réutilise le `Hlo` exact déjà calculé par `rect_h_interval` :

```text
Qhi = D2hi*V2hi-P2lo
q4 ALL si Hlo>0 et 8*Hlo^2>Qhi
q3 ALL si Hlo>0 et 12*Hlo^2>Qhi
```

Les égalités restent ouvertes et toutes les multiplications sont promues en
`i128` avant produit. Cette branche prolonge naturellement le fallback
`Hmin/E2max/X2max` de `rect_classify`; elle ne justifie pas un second
classifieur isolé.

Le membre droit doit employer `D2hi`, jamais `D2lo`. Fixture mutante en 2D,
avec troisième coordonnée nulle :

```text
A=[2,4]x[1,2], B=[0,1]x{7}, C={(1,4)}
Hlo=6, D2lo=26, D2hi=52, V2hi=10, P in [-5,9], P2lo=0.
```

Le faux membre droit `D2lo*V2hi=260` serait battu par `8*Hlo^2=288` et
fermerait q4. Pourtant `a=(4,2)`, `b=(1,7)`, `z=(1,4)` donne
`D2=34`, `V2=10`, `P=4` et
`(D2-V2)^2-2*(D2*V2-P^2)=-72` : la fermeture serait fausse.

L'intervalle de `P` est exact ; le classifieur composé est **sûr fail-open mais
incomplet**, car les extrema séparés de `D2`, `V2` et `P` perdent leur
corrélation. Dire que le passage au rectangle est « exact » ou « sans perte »
serait donc faux. Le résiduel peut être raffiné, ou confié aux cages/projectif ;
il ne peut pas être fermé par l'échec de cette inégalité.

## 4. Complément : ce que peut signifier la tendance `13,9 %`

Le taux annoncé ne borne pas le résiduel q4 :

- « boule `209` vide » n'implique pas « spindle de Jung vide » ;
- « spindle singleton vide » n'implique pas « aucune couverture de
  multiplicité » ; huit cages disjointes peuvent garantir huit intérieurs dont
  l'identité change avec le centre ;
- absence de certificat n'implique jamais présence d'un support.

Il peut même y avoir zéro témoin singleton universel et une fermeture q4 par
groupes. Prendre `a=(100,100,100)`, `b=a+(-25,-24,-24)` et, pour
`k=1,...,8`, la cage `G_k={a+k*sigma}` avec
`sigma` dans `{+++, +--, -+-, --+}`. Les huit cellules locales ont pour
sommets `t=-(3k/2)*sigma` et vérifient
`max 2*t dot (b-a)=219k<=1752<1777=||b-a||^2`; les huit cages disjointes
ferment donc toute sphère par `a,b`. Pourtant aucun des `32` sites ne satisfait
le prédicat singleton q4 de Jung. Une mesure `spindle_empty` peut ainsi valoir
`100 %` sur cette paire tandis que `cage_rank=8`. Les diagnostics doivent
publier séparément `CentralBall209`, `JungSpindleSingleton` et le rang de cages.

La route cages/fleurs et la fixture kNN sont détaillées dans
[`AUDIT_REPONSE_DEUX_PERTES_CAGES_FLEURS_B96751C_20260813.md`](AUDIT_REPONSE_DEUX_PERTES_CAGES_FLEURS_B96751C_20260813.md).

La question de tendance devient mathématiquement bien posée après choix d'un
modèle de famille. Pour une paire fixe dans un échantillon i.i.d. et une région
de certificat `W_ab` de masse de probabilité `mu_ab`, le nombre d'autres points
dans cette région suit conditionnellement une loi binomiale. Pour
`h=8` :

$$\Pr[N_{ab}<h\mid a,b]=\sum_{j=0}^{h-1}\binom{n-2}{j}\mu_{ab}^{j}(1-\mu_{ab})^{n-2-j}.$$

Si `mu_ab>0` est fixe, cette probabilité tend exponentiellement vers zéro. Si
`mu_ab=0`, elle vaut un. Pour des paires choisies **conditionnellement dans le
résiduel**, la distribution de `mu_ab` change avec `n` : une masse atomique en
zéro peut laisser une fraction constante, et beaucoup de très petits `mu_ab`
peuvent donner une décroissance lente. Sans minorant uniforme positif, aucun
théorème de disparition ne suit.

Le fichier `coeur_par_paire_amas.txt` montre seulement, pour la boule `209`,
`15,2 %`, `13,8 %`, `12,7 %` de vides aux trois tailles, et
`62,5 %`, `71,3 %`, `78,0 %` avec au moins huit points. Trois tailles et une
sélection changeante ne distinguent pas limite positive et décroissance. Pour
une inférence exploratoire : trois seeds, commande/hash complets, stratification
par type de paire/amas et mesure de la distribution du compte normalisé sont
nécessaires. Pour un contrat exact adversarial u16, la réponse reste simple :
la fraction peut être constante ou totale ; aucune distribution ne protège le
produit.

## 5. Réponse Q3 : front multi-lane masqué, seuil local, coût global

Les trois seuils justifient une traversée partagée **masquée**, pas trois
constructions d'arbres :

```text
Task(ANode,BNode,lane_mask)
  q2 devenue terminale -> émettre son RectId, retirer bit q2
  q3 devenue terminale -> émettre son RectId, retirer bit q3
  q4 devenue terminale -> émettre son RectId, retirer bit q4
  bits restants -> split commun déterministe
```

Pour chaque lane, ses terminaux doivent former une partition exacte de tous les
`PairId`; les partitions peuvent différer entre lanes. Le range-add s'applique
séparément à chacune. Une lane déjà fermée ne redescend jamais et ses crédits
ne sont pas recomptés.

Le choix « juste au-dessus » n'est pas une conséquence mathématique. Quand
`s` dépasse à peine `2/tau`, la boule garantie a un rayon presque nul et contient
souvent zéro point, alors que le front a déjà grossi. Un `s` plus grand peut
réduire la masse ouverte mais augmenter front, tâches et mémoire. L'objectif à
minimiser est un coût composé reçu, par exemple :

```text
W_lane = front_tasks + witness_tasks + pending_tasks + downstream_open_work
```

Le rejeu live montre pourquoi. Sur `uniform,n=4000`, passer de `s=3` à `s=8`
fait chuter `sum_E4` d'environ `7,91 M` à `1,43 M`, mais fait monter les
recertifications d'environ `28,11 M` à `171,67 M` et le temps de vague de
`9,20 s` à `70,27 s` sur la machine partagée. La sortie est plus parcimonieuse
et son producteur environ `7,6` fois plus lent. Ce diagnostic mono-run n'est pas
un benchmark reçu ; il suffit toutefois à réfuter « choisir le premier `s`
admissible » et toute gate fondée sur `sum_E4` seule.

Les seuils éliminent les valeurs structurellement peu informatives ; ils ne
choisissent pas l'optimum. Pour le certificat codé, les premières valeurs
rationnelles simples au-dessus sont seulement des ablations (`q3: 7/2`,
`q4: 4`). Le P0 recommandé reste q4 seul avec critère local exact. Partager q3
et q2 ne vient qu'après une pente q4 et une HWM vertes, afin de ne pas payer une
architecture multi-lane pour une source q4 déjà réfutée.

## 6. Contre-audit live des nouvelles portes et des reçus

Le noyau de range-add est reçu **relativement aux fates qu'on lui fournit** par
l'oracle de vecteur. Depuis le premier snapshot rouge de cet audit, Claude a
corrigé le masquage des pentes, converti l'ancien degré symétrique en
range-add, ajouté la partition massique exclusive, un oracle avec banque et les
fixtures spindle/rang. Le verdict `7/8` du snapshot intermédiaire est donc
rétracté comme état live.

Commandes rejouées :

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --target mhgp3v_wspd_wavefront_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '<huit portes courtes>'
```

Au snapshot antérieur `probe=e5c8d4eee9...`, `CMake=8dce6d8570...`, les dix
portes `fenetre_*` et `fixtures_spindle` passaient en `136,42 s`. Au snapshot
courant pincé en section 0, reconfiguration et build passent, puis les huit
portes courtes `699--704`, `fixtures_spindle` et `fixtures_rang` passent en
`0,24 s`. Les longues rampes n'ont pas été rejouées après le dernier delta ;
aucun « binaire final » ne peut être annoncé dans ce worktree mouvant.

Les réparations sont réelles : toutes les pentes sont imprimées avant verdict,
le refus mordant nomme maintenant `sum_E4`, et le sous-ledger vérifie
`closed+pending+strict_open=C(n,2)`. Les blocages restants sont précis :

1. **Le nominal avec banque est vert sans fenêtre finale.** La commande du
   CTest à `window=256` sort `0` et imprime `oracle_fenetre accord=OUI`, mais
   aussi `fenetre_finale=NON`, avec `18` paires pending en q3 et q4. La regex ne
   reçoit que l'accord du range-add sur le **surensemble**. Le nominal doit
   exiger `pending=0` et un sentinel post-verdict `fenetre_finale=OUI`, ou tester
   séparément une continuation qui consomme exactement ces `18` paires.
2. **Le superset garde un nom ambigu.** `mass_open`, `open_terms`, `sum_E4` et
   le plancher `min-ouverts` incluent encore les pending ; seule la ligne
   `masses : fermee/pendante/ouverte` est exclusive. C'est sûr en fail-open,
   mais seulement sous les noms `residual_superset` et `pending_mass`, et aucune
   pente finale ne doit être gatée tant que `pending_mass>0`.
3. **La finalité n'est pas multi-mode.** Le reproducer
   `--descent --window=2 --bank-l=1` abandonne un heap vivant mais publie
   `recert=0`, `pending=0`, `fenetre_finale=OUI`. Seules les troncatures VWave
   alimentent aujourd'hui `pend`. Chaque mode doit produire la continuation
   exacte `(node,lane_mask,cursor)` ou refuser le label final.
4. **Le nominal de pente peut être faux-vert.** Son
   `PASS_REGULAR_EXPRESSION` accepte dès qu'une ligne ressemble à
   `pente sum_E4=1.[0-3]`; il ne verrouille ni les deux pentes, ni le code `0`,
   ni un verdict final. Employer un wrapper code+sortie ou un sentinel imprimé
   seulement après toutes les gates. Garder en parallèle une expected-failure
   physique sur `front_records` : les tests unitaires E4 désarment actuellement
   cette gate avec `--max-slope=9`.
5. **Le claim scientifique reste borné.** `E_4` désigne le résiduel du
   certificateur central sous owner maximal, pas encore
   `PWC0-A/CanonicalEdgeWindowReporter-q4-v0`. L'oracle juge le ledger relatif
   aux fates, pas la complétude vis-à-vis des vrais `BallKey/SupportKey`.

Le dossier `receipts/fenetre_e4_20260813/` possède maintenant README, manifeste
et script, mais leur présence ne répare pas la causalité :

- `uniform_s8.txt` est vide alors que README et note lui attribuent quatre
  tailles et des pentes ;
- les autres sorties portent l'ancien format, tandis que le manifeste leur
  associe un source/binaire plus récent ; le source live a déjà encore changé ;
- le manifeste annonce son propre hash `84d2...`, mais son hash observé vaut
  `25d07...` ; une auto-empreinte dans le fichier qu'elle hache est impossible ;
- `refaire.sh` masque les codes par `|| true`, ne les sérialise pas et emploie
  des backticks dans un `echo`, donc tente une substitution de commande ;
- aucune sortie ne reçoit encore les tables spindle annoncées, et le cadre du
  README du reçu n'emploie pas les cinq champs v3 imposés.

Le prochain reçu doit geler source, CMake et ELF **avant** les runs, enregistrer
commande/code/timestamps par sortie, ne pas auto-hasher son manifeste, puis
hacher le manifeste de l'extérieur. En l'état, ces fichiers restent des logs
legacy non pinnés.

## 7. Directive consolidée remise à Claude

Ordre recommandé :

1. corriger dans la note les claims encore absolus « ne pouvait rien fermer »,
   « linéaire/borné » et « Fermé » ; distinguer toujours idéal,
   `CentralBall209`, spindle et cages ;
2. rendre la finalité mordante dans **chaque** mode, avec
   `closed/strict_open/pending` exclusifs, continuation par `lane_mask`, puis
   refaire un reçu causal ;
3. étendre le `rect_classify` existant par l'intervalle directionnel exact de
   `P`, le `Qhi` sûr et les mutants coins/`D2lo`/signe/égalité/overflow ; comparer
   au juge exhaustif sur petites AABB ;
4. mesurer cette branche sur `eight_clusters` par **classifications ALL
   supplémentaires, tâches, temps et HWM**. Le fallback actuel
   `Hmin/E2max/X2max` n'est pas un substitut : dans un diagnostic live à
   `n=1000`, il ne réduit `E4` que d'environ `0,43 %` sur les amas et multiplie
   le temps par `2,79` ;
5. ajouter en OR le `CageFlowerFastPath`, qui traite les paires où aucun témoin
   singleton n'est universel, puis laisser le résiduel au vrai reporter PWC0-A ;
6. gater un coût composé `front+certificateur+aval`, HWM/octets et temps E2E,
   avec une porte physique du front distincte de la porte unitaire `sum_E4` ;
7. seulement après une fenêtre q4 finale et sparse, mesurer `M=sum m_ab`, puis
   intégrer les niveaux shallow locaux.

La découverte du seuil explique une transition du **minorant** ; elle ne rend
pas `s=3` invalide et ne choisit pas `s=8`. Le reporter live prend déjà environ
`70 s` à `n=4000,s=8` sur le diagnostic CPU, sans produire de `BallKey`, de
census ni de fold. Aucun élément de ce snapshot ne se rapproche encore du
contrat `1 s` à `50 000` sur G4 ; le levier requis reste un certificat par
spans dont **la construction** est subquadratique, pas seulement une sortie
`E4` parcimonieuse.

GCP non utilisé.
