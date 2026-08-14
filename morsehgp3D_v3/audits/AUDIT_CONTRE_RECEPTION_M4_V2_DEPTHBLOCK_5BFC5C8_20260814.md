# Contre-réception M4 v2 et certificat de profondeur par bloc

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot

Le pin relu est
`HEAD=5bfc5c88c3457a6dc989192e325f1801e50aef0e`, commit
`je retire mon propre titre : M4 est quartique par construction`.

Empreintes SHA-256 au HEAD :

- `CMakeLists.txt` :
  `b31ef598c1df69eaeb50ae9e645716fd0ba9e10628e11411fce479ebfc3c1078` ;
- `prototype/soc64_rect.hpp` :
  `bbd1de16f4884d98ed2033f6c072ef6245cff6a8e90d95d5283f6e2bbe9ad902` ;
- `prototype/soc64_probe.cpp` :
  `d442b59279f345d11337b86993b8b620774eb236815a8f77a227cbf8edc4944f` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `fe146f28d962750facc92f0597246f995c044662188d99a5b687a72bf70486ce` ;
- `prototype/q4_brute_oracle.cpp` :
  `ff66bd5afc49420957e7344135fbe86e8877404de40ed7e045326a0160d582fd` ;
- `prototype/cloud_families.hpp` :
  `1f9089ba5972bf76aece6d899bacd8682341f394833c5d06e46ea2a921efad57`.

Le worktree est redevenu mouvant après ce commit. Claude modifie le brute-force
q4 et précise la preuve de `2B_R` dans l'audit précédent. Ces deltas live ne
sont pas attribués à l'auditeur et ne deviennent pas le statut du HEAD.
L'auditeur ne modifie aucun fichier logiciel. GCP non utilisé.

## Verdict direct

Le verrou conceptuel M4 est levé, mais les deux nouveaux instruments ne sont
pas encore reçus comme mesures :

1. `CarrierApexEstimator-v2` corrige l'owner, le mélange de `PENDING` et la
   censure des grosses lentilles. Son estimateur emboîté est une bonne
   proposition. Sa sortie `+/- 2 sigma` n'est toutefois pas un intervalle de
   confiance et peut avoir une largeur nulle autour d'une valeur fausse ;
2. `q4_brute_oracle` énumère utilement tous les quadruplets à petit `n`, mais
   recopie les mêmes identités Gram--Cramer/in-sphere et ne traite ni shell ni
   politique de dégénérescence. Son claim « `M4=Theta(n^4)` pour tout nuage »
   est faux et contredit `two_lines`, où `M4=0` ;
3. la masse logique pré-rang peut réellement être quartique sur les familles
   mesurées et sur une famille ouverte explicite. Elle ne doit donc jamais être
   remplie. Le bon objet est `BlockBallDepth8` sur les blocs WST4, puis un
   arrangement **shallow** edge-local sur le seul résiduel ;
4. le SLO G4 reste ouvert. Aucune rampe 50 000 du sampler M4 n'est utile avant
   réception des compteurs physiques `F4`, splits, visites de témoins, centres
   shallow, octets et HWM.

Le résultat utile n'est donc pas « mesurer plus précisément un nombre
quartique ». Il est : **prouver la profondeur avant le fill et ne jamais créer
ce nombre**.

## 1. Noms contractuels

Pour une vue finale `v`, conserver :

```text
C4_carrier(v) = incidences owner-edge x face aiguë
M4_apex(v)    = quadruplets canoniques pré-barycentriques
W4_positive  = quadruplets dont le circumcentre est strictement intérieur
H4_rank      = supports reçus après census, shell et politique de dégénérescence
```

`M4_total` du brute-force porte sur toutes les arêtes du nuage.
`M4_apex(v)` du sampler porte seulement sur les arêtes `OPEN_FINAL` de sa vue.
Ils ne sont pas interchangeables. Un compteur de seuls intérieurs
`I<=smax-4` n'est pas encore `H4_rank` tant que shell, `BallKey`, plateaux et
statut transactionnel ne sont pas traités.

## 2. Contre-audit de `CarrierApexEstimator-v2`

### 2.1 Corrections reçues comme code de diagnostic

Le v2 :

- exclut `PENDING` de la population et publie sa masse séparément ;
- rejoue l'owner longueur/`EdgeKey` sur les vrais `PointId` ;
- tire des arêtes avec remise et des paires internes dans chaque lentille ;
- ne retire plus une arête lorsque sa lentille dépasse un cap ;
- sépare `C4_carrier_quadrature`, `M4_apex_quadrature` et
  `W4_positive_quadrature` ;
- refuse désormais un contrôle demandé au-delà de sa taille bornée ;
- fournit un parcours direct exhaustif de la population à petit `n`.

Les quatre barycentriques par Gram--Cramer sont algébriquement correctes sous
les hypothèses non dégénérées. Le schéma Hansen--Hurwitz emboîté serait non
biaisé si les deux étages étaient effectivement uniformes et indépendants.

### 2.2 Contre-fixture : une barre nulle autour d'une valeur fausse

Commande rejouée :

```text
./build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=40 \
  --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger \
  --porteurs=1 --apex=1 --porteurs-seed=1 --porteurs-oracle=100
```

Le processus sort `0`, `pending=0`, puis publie en q4 :

| quantité | quadrature v2 | exhaustif direct |
|---|---:|---:|
| `C4_carrier` | `1560 +/- 0` | `4652` |
| `M4_apex` | `4680 +/- 0` | `60280` |

Avec un seul tirage, la variance empirique vaut mécaniquement zéro. La chaîne
`+/- 2*sqrt(var/K)` ne peut donc être ni une couverture probabiliste, ni une
porte. Cette contradiction doit devenir une fixture : l'exact doit appartenir
à l'intervalle annoncé ou le statut doit être `UNKNOWN`, jamais `OK`.

### 2.3 Les claims « exhaustif » et « doublons » sont faux

La boucle extérieure reste avec remise même lorsque `K>N`. À `n=120`, q4 a
`N=6917`, mais `K=20000` produit encore une quadrature différente de
l'exhaustif. Pour la seed 1 :

```text
C4_carrier_quadrature = 119520.2264 +/- 1189 ; exact = 119669
M4_apex_quadrature    = 4677946.989 +/- 79700 ; exact = 4676447
```

Le commentaire CMake « le tirage devient exhaustif » est donc incorrect. Le
contrôle imprime les totaux directs, mais ne les compare pas au sujet dans un
mode exhaustif commun ; son mutant compare `C4` et `M4` dans le contrôle
lui-même.

Les CTests reflètent cette lacune : `mhgp3v_porteurs_uniform` n'active même pas
`--apex`, les portes avec apex n'exigent par regex que la ligne du contrôle
exhaustif, et le mutant `porteurs-c4-comme-m4` ne corrompt que cette impression
du contrôle. Aucun test ne mord actuellement une mauvaise
`M4_apex_quadrature` ou une mauvaise `W4_positive_quadrature`.

Le même run annonce `doublons=3`. Avec 20 000 tirages dans 6917 cases, au
moins 13 083 tirages répètent nécessairement une case déjà vue. Le compteur ne
détecte que deux rangs égaux **consécutifs**. Le renommer
`repetitions_consecutives` ou publier le nombre de rangs uniques.

Une vraie porte d'indexation ajoute un mode distinct :

```text
--porteurs-exhaustif
  -> visiter exactement les rangs 0..N-1, sans RNG
  -> visiter exactement les paires internes 0..binom(l_e,2)-1
  -> comparer dans le processus C4/M4/W4 au juge direct
```

### 2.4 Loi et intervalle manquants

Le rang `floor(h*N/2^64)` par multiply-high est presque uniforme, mais pas
uniforme exact lorsque `N` ne divise pas `2^64`. Une réduction par rejet doit
éliminer ce biais aux deux étages. Une suite SplitMix déterministe à seed fixe
ne constitue pas à elle seule une preuve d'indépendance ; documenter une
famille de tirages et préannoncer plusieurs seeds indépendantes.

Pour une porte conservatrice simple, chaque observation extérieure de
`C4_carrier` appartient à `[0,n-2]` et chaque observation emboîtée de
`M4_apex` à `[0,binom(n-2,2)]`. Sous de vrais tirages indépendants, une
demi-largeur Hoeffding bilatérale sûre est :

```text
width_C4 = N*(n-2)*sqrt(log(2/delta)/(2*K))
width_M4 = N*binom(n-2,2)*sqrt(log(2/delta)/(2*K))
```

Diviser `delta` par le nombre total de lanes, familles, tailles et décisions
simultanées. Si cette borne est trop large, le verdict est `UNKNOWN`; une
borne empirical-Bernstein préannoncée ou un tirage PPS stratifié peut la
resserrer, mais `2 sigma` sans niveau `delta` ne la remplace pas. Publier aussi
un intervalle propre à `W4_positive`.

Le tirage de rang actuel est encore conditionné sur les premiers candidats
bien centrés rencontrés et n'a pas les poids de sélection nécessaires. Son
ratio `retenus` n'est donc ni `H4/W4`, ni un estimateur de sortie. La vue SOC
appariée reste absente.

### 2.5 Delta live : Hoeffding répare la barre nulle, pas encore la loi

Le worktree postérieur au pin remplace `2 sigma` par la demi-largeur Hoeffding
conditionnelle correcte. La contre-fixture `K=1` imprime désormais
`C4=1560` avec une demi-largeur `48240`, et `M4=4680` avec une demi-largeur
`892500` : elle ne prétend plus une précision nulle. Cette réparation est
nécessaire et reçue comme formule.

La couverture probabiliste ne l'est pas encore. Les rangs restent obtenus par
multiply-high presque uniforme, sans rejet exact, depuis des compteurs SplitMix
à seed fixe dont la loi et l'indépendance conditionnelle des deux étages ne sont
pas contractées. Le `delta` n'est pas réparti sur toutes les lanes, familles,
tailles et décisions simultanées. `W4_positive_quadrature` reste sans intervalle
propre. Enfin `porteurs_controle` parcourt directement la population, mais ne
compare toujours pas dans le processus les valeurs de la quadrature à un mode
exhaustif déterministe `0..N-1` ; son commentaire promet plus que son code.

Le delta améliore donc un diagnostic, sans rendre le sampler apte à décider une
pente M4 ou une campagne G4.

## 3. Contre-audit de `q4_brute_oracle`

Chaque quadruplet possède exactement une arête owner, mais `Q_e` exige encore
une orientation non nulle et au moins une face aiguë incidente à cette arête.
L'owner prouve donc seulement `M4_total<=binom(n,4)`. Il ne prouve aucune borne
inférieure. La famille `two_lines`, pour laquelle le binaire imprime
`M4_total=0`, réfute dans la même unité de traduction le claim universel
`Theta(n^4)`.

Les valeurs exhaustives restent un signal très fort sur les deux régimes :

| famille | `n` | `M4/C(n,4)` | `W4/M4` | test `I<=7` par point |
|---|---:|---:|---:|---:|
| `uniform` | 60 | 0,6429 | 0,1206 | 39,8 |
| `eight_clusters` | 60 | 0,6610 | 0,1508 | 26,8 |
| `eight_clusters` | 90 | 0,6598 | 0,1453 | 27,3 |
| `eight_clusters` | 120 | 0,6577 | 0,1441 | 31,4 |
| `two_lines` | 60 | 0 | — | 0 |

La conclusion recevable est : `M4_total` et `W4_positive` ont une fraction
quartique observée sur ces tailles, familles et cette seed. Une construction
ouverte sur quatre sous-cubes, déjà donnée dans `PROPOSITION.md`, établit en
outre une vraie borne quartique avant rang sur une famille explicite. Aucun de
ces faits ne rend `M4` quartique pour tout nuage.

Le ratio du test `I<=7` est petit, mais trois tailles ne prouvent pas
`H4=Theta(n)`. En dimension trois, la théorie générale des mosaïques de
Delaunay d'ordre fixé n'offre pas une borne linéaire universelle ; même la
complexité Delaunay ordinaire peut être quadratique. La restriction
well-centered pourrait être plus favorable sur ces régimes, mais le présent
oracle ne le démontre pas. L'extrapolation `H4≈30n` à 50 000 est donc un modèle
exploratoire, pas une borne produit.

L'unité dite indépendante recopie les mêmes formules Gram--Cramer et les mêmes
cofacteurs in-sphere. Une vraie autorité indépendante emploiera par exemple un
solveur rationnel GMP pour le circumcentre et un déterminant 5×5 direct, avec
mutants de signe, stricte, colonne de Cramer et owner à égalité. Le brute-force
actuel oublie l'extra-shell et la politique `unsupported_degeneracy`; sur
`two_lines`, ses ratios vides impriment `NaN`. Ses CTests figent des valeurs
utiles, mais ne reçoivent pas encore `H4_rank`.

La fixture des huit sommets d'un cube montre la différence : les deux
tétraèdres réguliers complémentaires ont le même `BallKey`, zéro intérieur et
les quatre autres sommets sur le shell. Un compteur de quadruplets `I<=7`
peut donc compter plusieurs supports pour une même boule sans reconstruire le
rang fermé ni la sortie RLE.

Le brute-force et le probe ne génèrent pas non plus le même nuage par défaut :
emprise calculée par famille et seed `12345` d'un côté, paramètres du probe de
l'autre. Aucun CTest ne scelle un hash de nuage commun puis ne croise les deux
catalogues. Une unité de traduction séparée ne suffit donc pas à faire un juge
différentiel.

Enfin, le delta live mesure une profondeur moyenne de `3,2x` à `6,6x` le seuil
sur `n=60..120`, pas « deux ordres de grandeur ». La croissance observée rend
un certificat de bloc plausible ; son extrapolation à 50 000 reste une
inférence.

L'échec d'un témoin spindle singleton ne prouve pas qu'une sphère admissible
reste vide. Plusieurs demi-plans peuvent couvrir collectivement le disque
`K_4` : c'est précisément ce que `BlockJungDualTile` cherche à certifier. Les
phrases « le spindle est maximal » ou « une longue arête possède toujours une
sphère presque vide » ne sont vraies que sous des hypothèses supplémentaires
ou pour une contre-famille construite ; elles ne suivent pas des mesures SOC.

## 4. Réponse à la Question 7 : `BlockBallDepth8` entier

Oui : la profondeur doit être certifiée sur `F4` avant tout fill de
`M4_apex`. Le prédicat ponctuel possède une forme entière directe.

Pour `u=b-a`, `v=x-a`, `w=y-a` et `r=z-a`, définir :

```text
O(a,b,x,y) = det3(u,v,w)
J(a,b,x,y,z) = det4((u,||u||^2),
                    (v,||v||^2),
                    (w,||w||^2),
                    (r,||r||^2))
```

Avec cette convention de lignes, `z` est strictement intérieur si et seulement
si `O*J<0`. Au lieu de former le produit, un classifieur de bloc borne les deux
signes séparément :

```text
O_L > 0 et J_U < 0  -> ALL_INTERIOR
O_U < 0 et J_L > 0  -> ALL_INTERIOR
0 dans [O_L,O_U] ou 0 dans [J_L,J_U] -> MIXED
```

Les bornes portent sur tout
`A×B×C×D×Z`, pas sur les seuls coins. `J` contient des normes quadratiques :
une interpolation de coins AABB n'est pas exacte. Employer une forme centrée
avec restes, des coefficients Bernstein rationnels vérifiés, ou scinder. Le
proposer peut travailler en flottant ; seul le vérificateur entier crédite
`ALL`.

Une première implémentation n'a pas besoin d'un tenseur Bernstein à quinze
variables. Utiliser le déterminant lifté 5×5 original, dont chaque ligne vaut
`(p_x,p_y,p_z,||p||^2,1)`, et borner ses 120 termes par intervalles entiers ;
les minima/maxima de `||p||^2` sur une AABB sont séparables. Une version plus
serrée pré-calcule pour le support les cinq cofacteurs de la forme sphérique
`g||z||^2-q dot z+r`, puis borne cette quadratique sur `Z`. Les dépendances
perdues ne créent que `MIXED`, jamais un faux `ALL`.

Pour resserrer sans énorme tenseur, choisir une origine entière locale `o` et
un tuple support représentant. Sa forme ponctuelle est
`P0(r)=A0||r||^2+B0 dot r+C0`, avec `r=z-o`. Borner sur le bloc support les
écarts de cofacteurs par `dA,dB_i,dC`, puis poser :

```text
E = maxabs(dA)*Qmax + sum_i maxabs(dB_i)*maxabs(r_i) + maxabs(dC)
J(B,Z) subset [min_Z P0-E, max_Z P0+E]
```

`min_Z P0` et `max_Z P0` sont séparables : pour chaque axe, tester les deux
endpoints et les entiers `floor/ceil(-B0_i/(2*A0))` clipés lorsque `A0` est non
nul. Splitter ensuite le facteur qui contribue le plus à `E`. L'orientation
`O` est multiaffine et peut, elle, être enveloppée par les coins de ses quatre
facteurs ; cette propriété ne se transfère pas à `J`.

Une contre-fixture minimale au raccourci `corners-only` prend le tétraèdre
`(3,2,2),(1,2,2),(2,3,2),(2,2,3)`, de centre circonscrit `(2,2,2)` et de
rayon un, puis le witness box continu `[1,3]^3`. Ses huit coins sont extérieurs
à la sphère, tandis que son centre est strictement intérieur. Les extrema du
lift quadratique ne sont donc pas déterminés par les coins de la boîte.

À l'échelle ponctuelle u16, `O` tient sous environ 51 bits et `J` sous environ
87 bits. Les transformations Bernstein et leurs dénominateurs exigent une
borne propre ; commencer avec un vérificateur i256, puis réduire seulement
après preuve de largeur.

Parcourir un BVH de témoins en nœuds disjoints. Un nœud `Z` classé
`ALL_INTERIOR` crédite uniquement les IDs garantis distincts de **tous** les
facteurs support `A/B/C/D`. Les nœuds crédités sont mutuellement disjoints ;
dès que leur population minimale cumulée atteint huit, tout le bloc WST4 est
fermé en q4. Une égalité est shell et ne crédite rien. Un cap rend `MIXED`,
jamais `OPEN` ni `CLOSED`.

Pseudo-ordonnance :

```text
classify_depth(F4Block B):
  certifier orientation non nulle et de signe fixe, sinon split
  credits = 0
  parcourir les WitnessNodes Z disjoints des quatre facteurs:
    borner J sur B x Z
    si signe strictement intérieur sur tout le produit:
      credits += population_min(Z)
      si credits >= 8: CLOSED_Q4
    sinon si Z est MIXED et non terminal: descendre Z
  rendre MIXED
```

Publier `F4_depth_tested`, `orientation_mixed`, `witness_nodes`,
`determinant_bounds`, `wide_ops`, `credits`, `closed_mass`, `splits`, octets et
HWM. Une fixture positive prend les quatre sommets de la construction ouverte
de `PROPOSITION.md` et les huit témoins
`(20+i,20+j,30+k)`, `i,j,k` dans `{0,1}` ; un mutant `corners-only`, un mutant
de signe et un mutant qui crédite un ID support doivent mourir.

Ce prune ne dépend ni de l'owner des six arêtes, ni des barycentriques : une
sphère qui possède huit intérieurs est hors q4 même si son support sera ensuite
rejeté pour une autre raison. Sur un `F4` broad-phase déjà distinct-ID, exécuter
donc la profondeur **avant** owner et positivité ; le résiduel seul paie ces
prédicats.

## 5. Fermer encore plus tôt : `BlockJungDualTile` sans fractions

Avant même WST4, un groupe de trois témoins au plus peut fermer toutes les
sphères q4 admissibles par une paire. Pour rendre la proposition directement
implémentable, écrire les poids rationnels sous forme d'entiers positifs
`w_z`, avec `W=sum w_z`, puis poser :

```text
d = b-a
D = ||d||^2
A = W*D - sum_z w_z*||a+b-2z||^2
P = W*(a+b) - 2*sum_z w_z*z
R = D*||P||^2 - (P dot d)^2

q3 : A>0 et 3*A^2>4*R
q4 : A>0 et   A^2>2*R
```

Ce sont exactement les inégalités minimax de `JungDiskDepth`, débarrassées de
toutes les divisions. Pour `W<=65535` sous u16, les deux côtés restent sous
environ 105 bits ; i128 signé suffit au prédicat ponctuel. Au niveau
`A×B`, borner les polynômes quartiques sur tout le produit par arithmétique
entière/Bernstein ou scinder. Les coins seuls restent interdits.

Un proposer trouve une base et des poids sur un représentant, le vérificateur
uniforme décide `ALL/MIXED`, puis huit groupes d'IDs disjoints ferment q4. Un
échec ne réfute rien. Cette porte est moins générale que `BlockBallDepth8`,
mais elle agit avant la multiplication carrier et vise directement les longues
arêtes inter-amas.

Attention aux quantificateurs : chaque paire peut admettre ses propres poids,
sans qu'un même vecteur de poids fonctionne sur tout `A×B`. Garder des poids
communs et vérifier les inégalités uniformément est un certificat **suffisant**
`ALL`, jamais une équivalence de bloc. Un échec impose un split ou `MIXED`.

La primitive ponctuelle doit elle-même garder cette portée : elle vérifie une
pondération fournie, elle ne décide pas l'existence des bons poids. Son ABI
exige `D>0`, `1<=k<=3`, coordonnées u16, poids strictement positifs et
`sum(weights)<=65535` vérifié avant les additions. Les `PointId` authentifiés
et la disjonction entre groupes appartiennent au wrapper de profondeur, jamais
à une simple matrice de coordonnées. Un retour négatif, un cap ou une banque
de poids épuisée vaut `UNKNOWN`, pas « groupe non couvrant ».

Le juge `k>1` doit résoudre exactement
`J intersect intersection_z{Phi_z<=0}` ou recalculer la marge duale avec une
arithmétique rationnelle indépendante. Tirer quelques centres ne prouve pas la
couverture du disque continu. Les mutants de largeur n'emploient jamais un
overflow signé volontaire : ce comportement indéfini invaliderait le kill.

### 5.1 Contre-audit du prototype live `JungDual`

Claude a matérialisé pendant l'audit `prototype/jung_dual.hpp` et
`prototype/jung_dual_probe.cpp`, aux empreintes live respectives
`1b9dffa1767988b812e1da360775858d023383112fcdde6e905d8ac3b2b46001` et
`05c6199a16bcfe1399aa600b4b7089b15b2b53efcb9707ea4e3c6368d9e71386`.
La forme `A/P/R`, l'ordre des coefficients q3/q4 et les largeurs ponctuelles
sont corrects sous le contrat annoncé.

Le commentaire inverse toutefois le minimax. La couverture est :

```text
min_w max_z Phi_z(w) > 0
  = max_lambda min_w sum_z lambda_z*Phi_z(w) > 0
```

et non « maximum sur le disque du minimum sur le groupe ». La formule codée
correspond au bon ordre, mais la preuve documentaire doit être corrigée.

Le selftest compare toujours seulement `k=1` à `(g,Q)`. Contrairement à son
en-tête, aucun chemin ne construit des centres du disque de Jung pour juger
indépendamment un groupe `k=2/3`. La nouvelle boucle à deux jeux de poids tue
le mutant `dual-ignore-weights`, mais elle teste seulement que deux appels au
même prédicat diffèrent ; ce n'est pas un oracle de couverture. L'ablation
appelle directement le prédicat dual et tire toutes les paires du nuage ; elle
ne lit ni fate ni tape des paires q4 `OPEN_FINAL`. Son gain est donc un
diagnostic all-pairs du proposer, pas le gain sur le verrou M4.

Fixture collective minimale q4 :

```text
a=(0,0,0), b=(100,0,0)
z1=(42,26,0), z2=(42,0,26), poids=(1,1)
A=14080, R=54080000, A^2=198246400 > 2R=108160000
```

Aucun singleton ne passe q4, tandis que le groupe passe. Un juge indépendant
intersecte exactement le disque de Jung avec les demi-plans mauvais ; cette
fixture doit exercer le collectif. Pour tuer `dual-ignore-weights`, prendre
`a=(0,0,0)`, `b=(6,0,0)`, `z1=(1,0,1)`, `z2=(2,2,0)`. Les poids `(3,1)`
donnent `A=64`, `R=1872` et ferment q4, tandis que `(1,1)` donne `A=32`,
`R=720` et ne dépasse que q3. Enfin `z=(2,1,1)` donne l'égalité q4
`A=24`, `R=288`, `A^2=2R` : elle doit rester non-q4.

L'autorité primale est constante sur une base. Poser `u_z=a+b-2z` et `s=2w`.
Les mauvais centres vérifient `s dot (b-a)=0` et
`2*s dot u_z>=D-||u_z||^2`. Dans ce plan, minimiser `||s||^2` sur
l'intersection des demi-plans. Le minimiseur est l'origine, une projection sur
un bord ou l'intersection de deux bords. Pour trois IDs au plus, il suffit donc
d'énumérer un nombre constant de candidats exacts. Le groupe couvre q4 si
l'intersection est vide ou si `2*r^2>D`, q3 si elle est vide ou si `3*r^2>D` ;
l'égalité est shell.

Pour éviter une base orthonormée, prendre
`v_z=2*(D*u_z-(u_z dot d)*d)` et `c_z=D*(D-||u_z||^2)` : le demi-plan devient
`s dot v_z>=c_z`. Les projections demandent environ 142 bits et les
intersections de deux bords environ 250 bits sous u16. Ce juge primal doit donc
rester GMP/i256 ; le dual `A/P/R` i128 est le vérificateur device compact.

Cette base donne aussi une profondeur sans exiger huit groupes globalement
disjoints. Avec `Depth(P,0)=true` et une base couvrante `G`, on a exactement :

```text
Depth(P,h) = AND_{z in G} Depth(P minus {z},h-1)
```

Au centre considéré, choisir le membre de `G` qui est intérieur puis le fils
qui l'a supprimé garantit des IDs distincts. Le pire nombre de recherches de
base vaut `(3^h-1)/2`, soit `3280` pour q4 et `9841` pour q3. Le hot path garde
huit/neuf groupes disjoints comme fast path ; le DAG de suppressions est le
fallback capé ou l'oracle. Sur un rectangle, une base proposée n'est héritée
qu'après vérification uniforme de sa marge ; sinon on scinde `A/B`.

Le fast path n'est pas complet, même exactement au seuil. La fixture q4
`a=(0,100,100)`, `b=(40,100,100)` possède six témoins universels
`(j,100,100)`, `j=1..6`, et trois gadgets `(20,111,100)`, `(20,92,108)`,
`(20,92,92)`. Leurs régions mauvaises dans `Y^2+Z^2<=200` sont respectivement
`Y<=-279/22`, `Y-Z>=17` et `Y+Z>=17` : elles sont non vides et deux à deux
disjointes. La profondeur vaut donc huit, mais au plus sept groupes couvrants
peuvent être disjoints. L'analogue q3 avec sept universels et les gadgets
`(20,113,100)`, `(20,91,109)`, `(20,91,91)` a profondeur neuf et packing huit.
Une porte qui assimile échec du packing et profondeur insuffisante perd des
fermetures vraies.

P0 d'API et de portes :

- `dual_lane` ne borne pas `sum w_z<=65535`, alors que toute sa preuve de
  largeur l'exige ; préflighter la somme sans overflow avant le calcul ;
- `--groupes=0 --echantillon=0` n'est pas refusé et rend une ablation vide en
  code zéro ; borner explicitement toutes les tailles et `rounds` ;
- `dual-narrow-i64` exécute des overflows signés, donc un comportement indéfini
  ne peut porter une porte ; émuler une troncature non signée définie ;
- la convention code `4 = mutant tué` et le mutant `dual-ignore-weights` ont
  été réparés dans le delta live ; conserver néanmoins la fixture collective
  fixe ci-dessus, car le balayage aléatoire ne juge pas la sémantique ;
- huit groupes doivent conserver des listes de `PointId` disjointes. Une
  simple profondeur numérique sans reçu d'IDs ne ferme rien.

Le marqueur `MHGP_HD` autour de `__int128` ne reçoit pas un chemin CUDA : il
faut soit deux limbs device avec opérations gardées, soit une qualification de
compilation et de largeur propre avant tout claim G4.

Le prototype est donc une primitive mathématique prometteuse, pas encore une
porte collective reçue ni un `BlockJungDualTile`.

Le raccord CMake live, SHA-256
`87b3e5d30c25b432d2631ed3572c6e462438c007de0326b5ba926378c819879b`,
construit le probe ; ses onze CTests passent en `0,24 s`. Ils reçoivent la
transcription singleton, cinq mutants et une ablation non vide sur une seed,
pas le juge géométrique collectif annoncé ni le domaine `OPEN_FINAL`.

### 5.2 La nouvelle « dissection de perte » ne compare pas la même population

Le CTest live `mhgp3v_dissection_perte_amas` passe. Son replay
`eight_clusters,n=1500` publie :

```text
profondeur_rect : 200 rectangles, masse moyenne 7,74,
  26,5 % avec huit témoins singleton communs à tout A×B
profondeur_exacte q4 : 200 PairId OPEN_FINAL tirés par masse,
  89,5 % avec huit témoins singleton universels de la paire
```

Le contraste est intéressant, mais il ne sépare pas quantitativement « boîte »
et « budget ». Le premier échantillon porte uniformément par enregistrement
accepté après filtre de hash et s'arrête à 200 ; le second porte sur la masse
des PairId ouverts. Un gros rectangle et un singleton ont donc le même poids
d'un côté et des poids `|A||B|` différents de l'autre. Les deux taux n'ont ni
la même unité, ni intervalle, ni appariement rectangle vers ses paires.

En outre `profondeur_rect` compte seulement des témoins **singleton** communs à
toutes les paires. Il ne juge pas les groupes collectifs de `JungDual`, encore
moins un `BlockJungDualTile` uniforme. La conclusion sûre est directionnelle :
le certificat exact pairwise possède un réservoir de fermetures que la boîte et
le budget actuels perdent. La réparation reste microtile/bloc avec preuve
uniforme ; matérialiser les PairId pour récolter les `89,5 %` recréerait le mur
M4. Publier ensuite le gain apparié en masse `M4_L/M4_U`, pas comparer ces deux
pourcentages bruts.

### 5.3 Delta live `--ordre-proche` : meilleur budget, même résiduel final

Le worktree postérieur à `8f47835` change seulement l'ordre DFS des deux enfants
MIXED : le nœud dont l'AABB est le plus proche du milieu du rectangle est
dépilé d'abord. Les largeurs u16 gardent le carré de cette distance dans i64 et
le prédicat certifié reste inchangé. À budget infini, l'ensemble visité et le
fate sont donc les mêmes ; sous budget fini, seul le partage `CLOSED/PENDING`
peut changer.

Un A/B déterministe `eight_clusters,n=1500` donne :

| fenêtre | ordre Morton : pending | ordre proche : pending | résiduel final |
|---:|---:|---:|---:|
| 32 | 124012 | 109546 | non final |
| 64 | 80680 | 43607 | non final |
| 128 | 2587 | 121 | non final |
| 256 | 0 | 0 | `E4=1071162` dans les deux cas |
| 512 | 0 | 0 | `E4=1071162` dans les deux cas |

Le levier est donc réel comme compression du budget du certificateur central,
mais nul sur son résiduel géométrique final dans ce replay. Il ne répond ni à
M4, ni à la profondeur collective. La phrase « les nœuds créditeurs sont tous
autour du milieu » justifie une heuristique, pas une optimalité : une gate doit
comparer plusieurs familles/seeds et exiger l'égalité des fates lorsque les deux
ordres sont finaux. À budget tronqué, toute comparaison de masse reste un
surensemble avec `PENDING`, jamais une victoire de source.

### 5.4 `--diag-feuille` : cohorte utile, pas rappel global de SOC64

Le delta suivant compare sur 200 000 **incidences de feuilles visitées** le
certificat central inscrit, SOC64 et le juge ponctuel `(g,Q)`. Les deux nouveaux
CTests passent et publient environ `27,5/42,7/44,4 %` sur huit amas et
`26,6/40,6/42,1 %` sur uniforme. L'ordre attendu est cohérent avec les preuves
de suffisance.

L'unité est toutefois fortement conditionnée : premiers parcours jusqu'au cap,
seulement `tk.node` feuille, endpoint rectangle de masse `|A||B|<=64`, lane
encore vivante sous ses ancêtres, et répétitions possibles du même témoin sous
plusieurs rectangles. Ce n'est ni un échantillon uniforme des PointId, ni la
masse des rectangles, ni `M4_open`. Dire que SOC « capte 96 % de l'atteignable »
est recevable uniquement sur cette cohorte, comme ratio `feuilles_soc /
feuilles_exact`; il ne prédit pas huit IDs distincts par rectangle et encore
moins la masse q4 fermée.

Les portes regex figent trois totaux agrégés. Elles ne vérifient pas les
implications **par incidence** `central_ALL => exact` et `SOC_ALL => exact`, ni
la disjonction des crédits. Le test `ordre_proche_sans_effet` lance seulement
l'ordre proche et cherche un ancien nombre `E4`; il ne compare pas les deux
ordres dans le même processus et n'exige pas explicitement `pending=0`. Enfin,
les commentaires « tronqués=0 partout » et « douze configurations identiques »
ne sont gravés par aucun de ces trois CTests.

La gate utile publie une matrice appariée par incidence et par terminal, compte
les faux positifs, les huit-uplets d'IDs distincts et leur masse, puis compare
baseline/SOC/exact sur le même univers terminal. Sans cette jointure, le
diagnostic explique un écart de prédicat mais ne tranche toujours pas M4.

### 5.5 P0 du delta `--none-descend` : la vue combinée est éteinte

Le commit `c271c84` conclut trop vite que `pending=0` signifie que toutes les
feuilles pertinentes ont été atteintes. Le parcours élague un nœud
`central-NONE` : ce verdict est exact pour la boule inscrite, pas pour SOC64 ou
le spindle exact. La cohorte de `--diag-feuille` est donc conditionnée par cet
élagage et ne mesure aucun plafond global.

Un replay exhaustif borné `eight_clusters,n=200,window=512` donne, sur les
incidences effectivement visitées :

```text
parcours courant :  C/S/E = 2621/4870/5033,    SOC/exact = 96,761 %
none-descend     :  C/S/E = 2621/16718/19416, SOC/exact = 86,104 %
```

Le prune central cachait donc `14383` témoins exacts et `11848` témoins SOC.
À `n=1500`, la descente rend à nouveau le budget actif :

```text
baseline        : pending=0,      E4=1071162, fenêtre finale OUI
none-descend    : pending=127858, E4=1117700, fenêtre finale NON
+ ordre-proche  : pending=117915, E4=1071199, fenêtre finale NON
```

Le delta live ne répare pourtant pas la vue combinée. Sur `v=NONE`, il ajoute
la lane au masque baseline `mixed`, mais laisse `w=v`; SOC n'est appelé que si
`w==MIXED`, puis `cmask` reste intersecté avec le masque central. Les branches
nouvellement parcourues consomment donc le quantum sans pouvoir créditer SOC.
Sur le petit replay, les statistiques du shadow sont strictement identiques
avec et sans `--none-descend`, malgré les `11848` témoins SOC cachés.

La réparation exacte exige deux traversées logiques indépendantes :

```text
baseline : central-ALL crédite, central-MIXED descend, central-NONE élague
combined : central-ALL crédite ; sinon SOC-ALL crédite ;
           sinon SOC-UNKNOWN descend, même si central est NONE
physique  : poursuivre un enfant si baseline-MIXED OR combined-MIXED
```

Chaque vue possède son masque, son ledger, sa saturation, son cap et son
`PENDING`. Une porte bornée compare la vue combinée à l'union exacte des
`PointId` et contient explicitement un chemin
`central-NONE -> descendant SOC-ALL`. `--none-descend` tel quel reste un
diagnostic réfuté.

Enfin, seul le témoin est singleton dans `--diag-feuille`; `A×B` conserve
jusqu'à 64 paires. Le ratio mélange donc puissance du prédicat et relaxation
des boîtes. Lorsque `A` et `B` sont singleton, SOC64 se réduit au prédicat
`(g,Q)` et leur accord est exact. Le titre « SOC64 à 96 % de son plafond »
ne vaut que sur la cohorte de petites boîtes déjà atteintes, jamais comme
plafond ponctuel ni comme gain M4.

### 5.6 Réponse à la Question 9 : le troisième levier est collectif

Le ratio `SOC64/exact` porte sur une exigence bien plus forte que le contrat :
le **même** témoin doit être intérieur pour tout centre du domaine. Le rang
demande seulement qu'à chaque centre il existe huit témoins intérieurs, qui
peuvent changer avec le centre. Formellement :

```text
u = nombre de témoins intérieurs pour tous les centres
d = min_c nombre de témoins intérieurs au centre c
u <= d
```

Améliorer encore le prédicat singleton ne touche pas le cas structurel où
aucun témoin supplémentaire n'est universel mais plusieurs demi-plans couvrent
collectivement le disque. La fixture des six universels et trois gadgets a
`u=6`, profondeur q4 `d=8`, et packing disjoint maximal sept. Elle réfute donc
simultanément « le singleton est presque optimal pour le contrat » et « huit
groupes disjoints suffisent comme décision complète ».

Le troisième levier conserve la factorisation :

1. garder le `RectId` CK coarse comme owner immuable ;
2. attacher un `ProofSpanDAG` lane-local qui propose des bases Helly de trois
   IDs, vérifie leur marge uniformément et scinde seulement le proof-tile
   `MIXED`, sans émettre les `PairId` ;
3. appliquer la récurrence leave-out jusqu'à profondeur huit/neuf sous cap ;
4. après carrier/apex, recommencer sur le segment `FaceAxisJung` puis sur la
   famille plus petite `BlockBallDepth`, où le domaine de centres est restreint ;
5. envoyer seulement le résiduel vers les niveaux shallow.

Le seuil `need=8` ne change pas et aucune mosaïque d'ordre supérieur n'est
construite. La gate publie, sur la même cohorte, `u`, le packing `p`, la
profondeur exacte `d`, puis les masses `u<8<=p` et `p<8<=d`, les proof-tiles,
splits, octets et `M4_apex` fermé. Une moyenne de `9,30` ne prouve pas à elle
seule que la perte au seuil explique les flips ; l'histogramme apparié le
prouve ou le réfute.

## 6. M4 sans échantillonnage : intervalles de blocs

Le delta live propose d'abord une identité exacte pour une arête fixée
`e={a,b}`. Retirer les sites collinéaires à `e`, puis noter `V_e` les sites
dont les deux arêtes endpoint ne battent pas `e`, `A_e` les carriers strictement
aigus et `N_e=V_e minus A_e`. Si `E_e(S)` compte les paires de `S` dont
l'arête ne bat pas `e`, alors :

```text
M4_e = E_e(V_e)-E_e(N_e)
       - sum_pi (E_e(V_e,pi)-E_e(N_e,pi))
```

Ici `pi` est la direction projective primitive canonique de
`(b-a) cross (z-a)`. La première différence conserve exactement les paires
avec au moins un carrier ; la somme retire exactement les paires coplanaires.
Sous un `EdgeKey` total et une normalisation projective qui identifie les signes
opposés, l'identité est correcte et exact-once. Elle mérite les fixtures
collinearité, direction opposée, tie de longueur et deux carriers aigus.

La relation incidente est un `OR`, pas un `AND`. Le q4 positif
`p0=(8,2,12)`, `p1=(1,3,9)`, `p2=(4,0,0)`, `p3=(10,5,1)` a l'arête owner
unique `p0p2`, une seule face adjacente aiguë et quatre poids positifs. Le
carrier aigu est primaire ; l'autre sommet reste un apex admissible même si sa
face owner n'est pas aiguë. Cette fixture tue une jointure de deux
`AcuteCarrierBlock` et, symétriquement, le carrier primaire tue le doublon
lorsque les deux faces sont aiguës.

Cette identité débloque un **preflight**, pas encore son coût. `V_e`, `A_e`, la
relation `xy` et `PlaneKey_e` dépendent toutes de la même arête. Un dual-tree
sur un rectangle CK doit donc conserver cette corrélation dans ses prédicats
`ALL/NONE/MIXED`; multiplier des masses agrégées sur `A×B` serait faux. Les
range-counts de distance et les RLE projectifs ont leurs propres compteurs,
splits, octets et HWM. Le résultat ne fournit ni barycentriques, ni profondeur,
ni `BallKey`.

Lorsque le tape CK et WST4 partitionne exactement les tuples, M4 possède un
compteur déterministe sans fill. Pour chaque bloc disjoint, calculer sa masse
de quatre IDs distincts après les diagonales `A/C`, `A/D`, `B/C`, `B/D`, puis
classifier owner, acuité et orientation :

```text
ALL_Q   -> ajouter mass(block) à M4_L et M4_U
NONE_Q  -> ajouter zéro
MIXED_Q -> ajouter mass(block) à M4_U et pending_mass
```

Ainsi `M4_L<=M4_apex<=M4_U`. Si `M4_L` suffit déjà à rendre rouge un moteur
ponctuel, l'ablation s'arrête sans estimation. Si `M4_U` est sous le budget, la
porte est verte. Sinon on scinde les blocs de plus grande masse jusqu'au cap,
où le verdict reste `UNKNOWN`. Employer au moins u128 pour masses et produits.

Le filtre positif du résiduel a lui aussi une forme fermée. Pour une face
owner aiguë `(a,b,x)`, reprendre `D,E,F,G,n,W` de `PROPOSITION.md`; pour
`s=y-a`, poser `A_y=G||s||^2-W dot s` et `B_y=n dot s`. Le poids de l'apex
vaut `A_y/(2B_y^2)`. Les trois poids de face sont positifs exactement lorsque,
en plus de `B_y!=0` et `0<A_y<2B_y^2`, on a :

```text
F*X*B_y^2 > A_y*(G+(F-E)*(d dot s)+(F-D)*(u dot s))
E*(D-F)*B_y^2 > A_y*(E*(d dot s)-F*(u dot s))
D*(E-F)*B_y^2 > A_y*(D*(u dot s)-F*(d dot s))
```

Ces comparaisons donnent `ApexWellCenteredBlock` sans solveur générique et
atteignent environ 174 bits sous u16. Elles ne réduisent pas seules l'exposant :
la fixture quartique positive impose de les combiner avec `BlockBallDepth8`
avant tout fill.

La borne `O(s^3*eta^-6*n)` ne porte que sur les blocs WST4 **initiaux**, sous
les hypothèses CK et à `s,eta` fixés. Elle ne borne ni les splits `MIXED`, ni
les nœuds témoins visités, ni les tests de cofacteurs, ni les blocs terminaux.
Publier séparément `F4_initial/F4_terminal`, splits par facteur, visites Z,
wide ops, masse fermée/résiduelle, fill, octets et HWM.

Les formules par masses aiguës de cellules ne sont exactes que pour une arête
fixe ou un bloc `ALL` corrélé. Des comptes `c_C` agrégés sur tout `A×B` ne se
multiplient pas : edge et carrier sont corrélés. Un juge exhaustif petit-n doit
comparer l'intervalle, les diagonales et l'exact-once à chaque split.

## 7. Fallback qui évite M4 : niveaux shallow edge-local

Pour une paire résiduelle fixe `a,b`, les centres vivent dans le plan
médiateur. Chaque site `z` définit la ligne :

```text
F_z(c) = 2*c dot (z-a) - ||z-a||^2 = 0
```

Le site est intérieur lorsque `F_z(c)>0`. Un support q4 est l'intersection des
deux lignes de ses autres sommets et, au seuil courant, sa profondeur stricte
est au plus sept. Il n'est donc pas nécessaire d'énumérer les
`binom(|L_e|,2)` apex : construire uniquement les niveaux `0..7` des lignes
orientées `P/N` et leurs événements `P-P`, `N-N`, `P-N`.

L'audit historique prouve, pour `m` formes et profondeur `k`, moins de
`e*(k+1)*m` centres géométriques shallow distincts et moins de
`2*e*(k+1)*m` incidences shell, avant les concurrences :
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).

Cette borne ne couvre ni les couples cross-bundle `J`, ni les supports
concurrents `H`, ni la somme des formes sur toutes les paires. Elle interdit
néanmoins le mauvais moteur `une face × tous les sites`. Le fallback correct
après un microtile edge non fermé est le constructeur shallow, pas une sweep
complète pour chaque face.

## 8. Ordre remis à Claude

```text
CKPairTape exact-once
  -> SOC64 optionnel + BlockJungDualTile9/8, sans expansion PairId
  -> CarrierBlocks dans la fenêtre 2B_R--lentille
  -> WST4 broad-phase symbolique et diagonales distinct-ID
  -> BlockBallDepth8 avant owner, barycentriques et tout fill de M4/W4
  -> owner/positivité seulement sur le résiduel
  -> pour le seul résiduel edge/microtile : niveaux shallow 0..7
  -> BallKey/RLE, census complet, shell, fold et payload
```

Ne pas lancer de rampe M4 à 50 000. L'ordre des microgates est :

1. exactitude de `BlockJungDualTile` sur petits produits et mutant
   `corners-only` ;
2. exactitude de l'intervalle `[M4_L,M4_U]` et de toutes les diagonales ;
3. `BlockBallDepth8` contre brute-force indépendant, huit IDs fixes et égalité
   shell ;
4. niveaux shallow contre catalogue exhaustif, bundles et concurrences ;
5. rampes `1500/3000/6000` sur `F3/F4`, splits, witness visits, centres,
   `J/H`, octets, HWM et temps ;
6. seulement si deux pentes physiques et les caps absolus passent, portage
   count--scan--fill puis campagne G4 50 000 avec payload officiel.

## 9. Rejeux et contre-audit de l'autre auditeur

Après reconfiguration du worktree :

```text
SOC64 isolé + WSPD--SOC + porteurs/apex/two_lines : 33/33 verts
q4 brute-force : 4/4 verts
JungDual live : 11/11 verts, juge collectif indépendant absent
contre-fixture K=1 estimateur : code 0, intervalle nul faux
q4 brute eight_clusters n=120 : M4=5402516, W4=778626, test I<=7=3764
```

Ces verts reçoivent des chemins et des valeurs bornées, pas les claims
statistiques ou asymptotiques.

L'autre auditeur a correctement identifié le biais des quadratures v0/v1,
l'owner manquant, la censure des grosses lentilles, la fenêtre sharp `2B_R`,
les diagonales WST et le besoin d'un tirage Hansen--Hurwitz apparié. Son schéma
v2 était mathématiquement recevable sous une loi uniforme indépendante ; le
live ne remplit pas encore ces hypothèses et sa barre `2 sigma` ne suit pas sa
recommandation empirical-Bernstein. Sa formule de join par masses est exacte
pour une arête fixe, pas depuis des masses carrier agrégées sur un rectangle.

Son intuition principale est confirmée : conserver l'owner arête maximale et
changer l'ordre physique, plutôt que chercher un owner `BallKey` circulaire.
Le présent audit la renforce par deux sorties exactes : profondeur
déterminantielle sur WST4, puis niveaux shallow sur le résiduel.

## 10. Addendum live au `HEAD=8268753`

Le worktree postérieur au pin historique contient un header et un probe
`JungDual` non suivis. L'identité `A/P/R` est correcte sous
`sum(weights)<=65535`, mais l'implémentation ne recherche aucun optimum : elle
certifie seulement un des sept vecteurs de poids essayés. Son commentaire écrit
à tort `max_w min_z`; la couverture est `min_w max_z Phi_z(w)>0`, puis le dual
`max_lambda min_w`. Le probe ne construit aucun centre pour juger `k>1`, malgré
son en-tête ; il compare seulement le singleton à `(g,Q)`.

Les blocages live sont causaux : le cap sur la somme des poids n'est pas
appliqué dans le header, le mutant étroit déclenche un overflow signé UBSan,
`dual-accept-equality` survit au random, `dual-ignore-weights` est invisible au
juge singleton, `--echantillon=0` et `--groupes=0` sortent zéro. L'inclusion
groupes/singletons n'est testée qu'en agrégé et peut masquer une perte par
paire. Les ablations `23/256` huit-amas, `18/256` uniforme, `5/256` terrain et
`0/256` deux-droites portent sur des paires arbitraires, pas sur le résiduel
owner CK--WST.

Le delta WSPD ajoute aussi le juge direct des flips demandé. À cap complet, le
mutant de somme est tué avec `168` flips jugés et `25` faux. À cap `1000`, le
nominal publie pourtant `accord=OUI juges=47 sautes=3 faux=0` : un juge partiel
doit publier `PARTIEL/UNKNOWN`. Aucun de ces deux nouveaux chemins n'est encore
câblé dans CMake.

Après rebuild live, la sous-suite ciblée passe `21/22` en `31,61 s`. L'unique
rouge est `mhgp3v_porteurs_uniform` : CMake exige encore le champ `doublons`
alors que le probe l'a correctement renommé `repetitions_consecutives`. Ce rouge
de cohérence ne répare aucune des lacunes statistiques décrites plus haut.

GCP non utilisé.
