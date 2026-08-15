# Audit mathématique du préfiltre combiné WSPD q2/q3/q4

Date : 15 août 2026 UTC.

Sujet audité : `main`, commit `4cd1f82319f1aeacff15ef69684fd8527f7a104b`.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!IMPORTANT]
> **Statut au `HEAD=66b4f0c` : audit historique partiellement supersédé.**
> Le P0 de double crédit q2 décrit ci-dessous est réparé par `3bf1bf3` : masque
> de lanes par frame, fixture de cinq IDs, mutant causal, oracle par `PairId`
> et couverture WSPD sont reçus dans le domaine u16. Le diagnostic au pin
> `4cd1f82` et l'invalidation du reçu q2 historique restent vrais ; les lignes
> déjà produites doivent toujours être régénérées. Les conclusions abstraites
> sur les ensembles maximaux, le sens de l'implication, le coût total et la
> non-maximalité q3/q4 restent actives. Le verdict live, les solutions et
> l'audit du nouveau cœur-boule sont dans
> [`AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md).
> Le chemin apex optionnel ajouté ensuite n'est pas reçu : il ferme à tort à
> `separation=1` faute de tester `gamma_q>0` avant son carré.

## Verdict

Le schéma mathématique est bon, mais son implémentation et sa revendication de
maximalité ne le sont pas encore.

| objet | verdict au pin audité | conséquence |
| --- | --- | --- |
| fuseaux `W2/W3/W4` | **reçus** pour une arête maximale de support positif | les inégalités strictes `H>0`, `3H^2>Xi`, `2H^2>Xi` sont les bonnes |
| seuils `h_q=s_max-q+1` | **reçus** | `10/9/8` à `s_max=11` sont sûrs, même hors position générale |
| disjonction abstraite `h_coeur+h_a+h_b` | **reçue sous PointId** | la somme est un minorant si chaque ID est crédité au plus une fois |
| compteur q2 | **P0 rouge** | le même nœud est crédité en bloc puis recrédité par feuilles ; des ancres vivantes sont fermées |
| compteurs q3/q4 | **sûrs mais non maximaux** | les bornes actuelles perdent des témoins ; aucun faux positif géométrique q3/q4 n'a été trouvé |
| claim « bornes exactes / plus grand `h` » | **réfuté** | `Xi` est décorrélé par composante, puis de `H`; des prédicats exacts 8/64/512 coins existent déjà dans le dépôt |
| claim de coût | **à restreindre** | seul l'histogramme est en `O(|A|+|B|)` ; la formation actuelle de `h_a,h_b` est en `O(|A|^2+|B|^2)` |
| reçu `prefiltre_combine_20260815` | **q2 invalide** | tous les pourcentages q2 doivent être régénérés après réparation |

La réponse directe à la question « les ensembles sont-ils les plus grands
possibles ? » est donc :

- **oui au niveau de leur définition abstraite**, sous la contrainte demandée
  que le cœur vive hors `A union B`, `h_a` dans `A` et `h_b` dans `B` ;
- **non dans le code courant**, d'abord parce que q2 ne compte pas des IDs
  uniques, ensuite parce que les AABB et les extrema décorrélés ne décident pas
  ces ensembles abstraits ;
- **non pour le coût visé**, car éviter le produit croisé `A x B` ne suffit pas
  si les deux auto-jointures `A x A` et `B x B` restent explicites.

## 1. Périmètre et autorités relues

Ont été relus pour cet audit :

- les Parties I et II du manuscrit, pages PDF 35 à 134, avec contrôle visuel
  des pages PDF 110 à 119 ;
- l'intégralité de l'arbre `morsehgp3D_v3/`, en insistant sur `README.md`,
  `PROPOSITION.md`, `audits/`, `receipts/` et `prototype/` ;
- `prototype/combined_prefilter_probe.cpp`,
  `prototype/spindle_cone.hpp`, `prototype/soc64_rect.hpp` et les portes CMake
  associées ;
- le reçu `receipts/prefiltre_combine_20260815/` et son brut.

Dans le manuscrit, la miniboule unique est la définition 25, page imprimée 84
(PDF 110), son support minimal est décrit par le fait 12, page 85 (PDF 111),
et un simplexe de Gabriel interdit les points extérieurs au simplexe dans
l'**intérieur ouvert** de sa miniboule, définition 28, page 87 (PDF 113).
Le théorème 4, pages 88--89 (PDF 114--115), ne donne qu'une condition
nécessaire : un support survivant au filtre n'est pas pour autant une fusion.

Les symboles q2/q3/q4 ne viennent pas du manuscrit. En dimension trois, ils
désignent ici les trois cardinalités possibles d'une base minimale positive :

- q2 : paire antipodale et boule diamétrale ;
- q3 : triangle strictement aigu et cercle circonscrit dans son plan affine ;
- q4 : tétraèdre affinement indépendant et bien centré.

Pour q3 et q4, `(a,b)` n'est généralement **pas** une paire antipodale de la
miniboule. C'est une **arête maximale canonique du support**. L'expression
« paire diamétrale » doit être réservée à q2, ou explicitement qualifiée de
« paire réalisant le diamètre du support ».

Le mot « témoin » est lui aussi homonyme. Dans le manuscrit,
`T_r(sigma)=intersection B(x,r)` est une région de **centres**. Ici `z` est un
**site intérieur universel**. Cette seconde expression est employée dans les
preuves ci-dessous.

## 2. Les fuseaux exacts de l'ancre

Soit `D=||b-a||`, `m=(a+b)/2`, et soit `z=m+p`. Tout centre d'une miniboule
admissible contenant l'arête maximale `(a,b)` s'écrit `c=m+y`, avec
`y` orthogonal à `b-a` et `R^2=D^2/4+||y||^2`. La fermeture des centres
admissibles est le disque de rayon `D/sqrt(12)` pour q3 et `D/sqrt(8)` pour
q4. Le minimum de la marge de `z` sur ces disques vaut
`H-2 r_q ||p_perp||`, où :

$$H(a,b,z)=(b-z)\mathbin{\cdot}(z-a)=\frac{D^2}{4}-\lVert z-m\rVert^2.$$

En posant :

$$\Xi(a,b,z)=\lVert (b-a)\mathbin{\times}(z-a)\rVert^2=D^2\lVert p_{\perp}\rVert^2,$$

on obtient exactement :

$$W_2(a,b)=\left\lbrace z:H>0\right\rbrace,\qquad W_3(a,b)=\left\lbrace z:H>0,\ 3H^2>\Xi\right\rbrace,\qquad W_4(a,b)=\left\lbrace z:H>0,\ 2H^2>\Xi\right\rbrace.$$

Ainsi `W4` est inclus dans `W3`, lui-même inclus dans `W2`. L'égalité est hors
du crédit : elle appartient au shell, jamais à l'intérieur strict.

### Correction d'un quantificateur dans les documents courants

Si `mathcal B_q(a,b)` est la famille des miniboules des complétions positives
admissibles ayant `(a,b)` pour arête maximale owner, la définition est :

$$W_q(a,b)=\bigcap_{B\in\mathcal{B}_q(a,b)}\mathring{B}.$$

La bonne implication est donc :

$$z\in W_q(a,b)\quad\Longrightarrow\quad z\in\mathring{B}\ \text{pour toute complétion admissible }B.$$

`PROPOSITION.md` section 6bis.1 et le préambule de
`combined_prefilter_probe.cpp` écrivent l'implication inverse : « tout site
intérieur à sa miniboule appartient au fuseau ». Elle est fausse pour q3/q4.
Avec `a=(-1,0,0)`, `b=(1,0,0)`, les deux complétions équilatérales ont pour
centres `(0,+1/sqrt(3),0)` et `(0,-1/sqrt(3),0)`. Le site `(0,1,0)` est
intérieur à la première boule et extérieur à la seconde ; il n'appartient donc
pas à leur intersection.

Le filtre emploie heureusement la bonne direction : appartenir au fuseau est
une condition suffisante pour être intérieur à la vraie miniboule, d'où un
**minorant**, jamais un census complet.

## 3. Pourquoi les seuils `10/9/8` sont sûrs

Soit `S` une base minimale positive de cardinal `q` d'une miniboule `B`, et
soit `I_B=X intersection interior(B)` l'ensemble des PointId strictement
intérieurs. Si un simplexe de Gabriel `sigma` a `B` pour miniboule et `S` pour
support, alors `I_B` est inclus dans `sigma`. Sinon un ID de
`I_B setminus sigma` serait exactement un intrus de l'intérieur ouvert, en
contradiction avec la définition 28 du manuscrit. Par conséquent :

$$\lvert\sigma\rvert\ge q+\lvert I_B\rvert.$$

Tout candidat limité à `|sigma|<=s_max` est donc impossible dès que :

$$\lvert I_B\rvert\ge h_q:=s_{\max}-q+1.$$

À `s_max=11`, cela donne `h_2=10`, `h_3=9`, `h_4=8`. Cette implication ne
demande pas la position générale. Hors position générale, les PointId
supplémentaires sur le shell doivent rester séparés de `I_B`, mais ils ne
fragilisent pas le prune par intérieur strict. Sous la position générale de la
définition 26, le shell du simplexe est précisément son support minimal.

Il ne faut pas confondre `q`, cardinal du support minimal, avec `K`, ordre de
la hiérarchie. Pour une cible `K_max=10`, `s_max=K_max+1=11`.

## 4. Les trois plus grands ensembles sous la factorisation demandée

Soit `X` l'ensemble des PointId et `A,B` les deux côtés disjoints d'un
rectangle CK. Pour chaque lane `q`, définir :

$$C_q(A,B)=\left\lbrace z\in X\setminus(A\cup B):\ \forall a\in A,\ \forall b\in B,\ z\in W_q(a,b)\right\rbrace.$$

$$A_q(a;B)=\left\lbrace z\in A\setminus\left\lbrace a\right\rbrace:\ \forall b\in B,\ z\in W_q(a,b)\right\rbrace.$$

$$B_q(A;b)=\left\lbrace z\in B\setminus\left\lbrace b\right\rbrace:\ \forall a\in A,\ z\in W_q(a,b)\right\rbrace.$$

Ces ensembles sont les **plus grands possibles avec les dépendances et pools
imposés** : un cœur indépendant de `(a,b)` doit être inclus dans l'intersection
définissant `C_q`; un facteur ne dépendant que de `a` doit être inclus dans
`A_q(a;B)` ; même preuve pour `B_q(A;b)`.

Ils sont deux à deux disjoints par PointId. Pour toute ancre `(a,b)` :

$$C_q(A,B)\cup A_q(a;B)\cup B_q(A;b)\subseteq X\cap W_q(a,b).$$

Donc le verdict factorisé maximal dans cette classe est :

$$\lvert C_q(A,B)\rvert+\lvert A_q(a;B)\rvert+\lvert B_q(A;b)\rvert\ge h_q.$$

La disjonction « automatique » mentionnée dans la proposition est correcte :
si `z` vit dans `A`, le choix de l'endpoint de même PointId, ou même de même
position, donne `H=0`. Il ne peut donc pas entrer dans le cœur universel.

Cette maximalité est **relative au schéma demandé**. Si des sites hors
`A union B` peuvent aussi être affectés à des facteurs de ligne dépendant de
`a`, une décomposition additive plus riche est possible, avec une politique
d'affectation d'IDs. Ce n'est pas le contrat `cœur / A / B` audité ici.

## 5. P0 : q2 recrédite les mêmes PointId

Dans `combined_prefilter_probe.cpp` au pin audité :

1. lignes 711--718, si un nœud témoin interne `Z`, disjoint de `A` et `B`,
   vérifie `h_all_inside>0`, toute sa population est ajoutée à `hcore[0]` ;
2. ligne 719, le commentaire annonce que la descente continue pour q3/q4 ;
3. lignes 750--751, les deux enfants sont effectivement empilés sans
   désactiver q2 ;
4. lignes 730--732, chaque feuille de ce même `Z` incrémente à nouveau q2 tant
   que le seuil n'est pas atteint.

Si `pop(Z)=5` et `h_2=10`, cinq vrais sites suffisent ainsi à produire le faux
compte dix. Ce n'est pas une perte fail-open : c'est une fermeture fausse.

Le replay ciblé suivant a été compilé depuis le source exact du pin, puis un
diagnostic d'audit a seulement ajouté l'énumération vraie q2 avant la décision :

```text
--points=160 --family=uniform --seed=3 --separation=6 --juge=160
AUDIT_FALSE_Q2 ua=154 va=157 i=0 j=0 lower=10 exact=5 hcore=10 ha=0 hb=0
a=(34,43,41) b=(51,38,37)
```

La paire est fermée par le préfiltre avec `lower=10`, alors que l'unique boule
diamétrale ne contient que cinq PointId strictement intérieurs.

Sur les trois familles à `n=160`, même graine et même séparation, une
énumération q2 exhaustive donne :

| famille | survivantes annoncées | vraies ancres q2 vivantes | faux rejets au moins | survivantes sans le bulk q2 |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | 3 083 | 3 656 | 573 | 4 054 |
| `eight_clusters` | 2 545 | 2 898 | 353 | 3 599 |
| `terrain` | 1 882 | 2 595 | 713 | 2 939 |

La suppression d'audit de la seule voie bulk q2 fait passer, sur `uniform`,
les fermetures de `9 637` à `8 666`; q3 et q4 sont inchangées.

### Pourquoi les portes sont restées vertes

- `Ledger::recouvrements` est initialisé à zéro ligne 509, mais n'est jamais
  alimenté. La porte des lignes 905--907 est donc vacue.
- Le juge des lignes 841--869 recalcule le prédicat sur des boîtes singleton ;
  il ne reconstruit jamais `hcore+h_a+h_b`, l'histogramme ni les ancres
  réellement fermées.
- Le juge du cœur vérifie qu'un site créditable est géométriquement bon, pas
  que son PointId n'a été crédité qu'une fois.

### Réparation minimale sûre

La pile doit transporter `(node, active_lane_mask)`. Après un crédit bulk ALL
pour une lane, cette lane est effacée du masque transmis aux enfants ; les
autres lanes peuvent continuer à descendre. Une feuille n'incrémente que les
lanes encore actives. Désactiver entièrement la voie bulk q2 est un correctif
plus lent mais sûr.

Fixture permanente minimale : `A` et `B` singletons, un sous-arbre `Z`
disjoint de cinq PointId tous dans `W2`, aucun autre témoin, `s_max=11`.
Exiger `hcore2=5` et l'ancre survivante, jamais `hcore2=10`.

## 6. q3/q4 : les extrema actuels sont sûrs, mais pas exacts

`xi_max_over_box` (lignes 180--207) maximise séparément la valeur absolue de
chacune des trois composantes du produit vectoriel, puis somme leurs carrés.
Ces trois maxima ne sont en général pas atteints au même point de la boîte.
La fonction rend donc un **majorant**, pas le maximum exact de `Xi`.

Ensuite `universal_witness` et `universal_over_rect` comparent le minimum
global de `H` au maximum global de `Xi`. Même si ce second maximum était exact,
les deux extrema peuvent vivre à des coins différents. La condition
universelle porte sur le couple corrélé `(H,Xi)`.

Deux contre-exemples u16 simples prennent `a=(0,0,0)` et `z=(1,1,1)`. Tous les
coins, donc toute la boîte cible par convexité, appartiennent à la lane
indiquée ; le code rejette pourtant `z` :

| lane | boîte `B` | `min H` | vrai `max Xi` | majorant code | minimum vrai de `c H^2-Xi` | comparaison du code |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| q3, `c=3` | `[0,1] x [2,3] x [7,8]` | 6 | 104 | 109 | 30 | `108 <= 109`, rejet |
| q4, `c=2` | `[0,1] x [3,4] x [8,9]` | 8 | 126 | 133 | 30 | `128 <= 133`, rejet |

Même avec `Xi_max` réparé, la décorrélation avec `H_min` perd encore des sites :

| lane | boîte `B` | `min H` | vrai `max Xi` | minimum vrai de `c H^2-Xi` | test par extrema séparés |
| --- | --- | ---: | ---: | ---: | --- |
| q3 | `[0,1] x [1,2] x [9,10]` | 7 | 182 | 1 | `147 <= 182`, rejet |
| q4 | `[0,1] x [3,4] x [4,5]` | 4 | 42 | 6 | `32 <= 42`, rejet |

Ces exemples réfutent les phrases « les bornes sont exactes », « majorant le
plus serré » et « `h` le plus grand qui reste rigoureux » de `README.md`,
`prototype/README.md`, `PROPOSITION.md` section 6bis.3 et
`AUDIT_ETAT_COURANT.md`. Ils ne produisent pas de faux témoin q3/q4 : la perte
est conservatrice.

## 7. Le correctif exact AABB existe déjà dans le dépôt

Pour q3/q4, poser `c_3=3`, `c_4=2` et :

$$F_q(a,b,z)=\lVert(b-a)\mathbin{\times}(z-a)\rVert-\sqrt{c_q}\,H(a,b,z).$$

Le prédicat `z in W_q(a,b)` équivaut à `F_q<0`; cette inégalité implique déjà
`H>0`. À `a,z` fixés, `F_q` est convexe en `b` : norme d'une fonction affine
moins une fonction affine. Il est de même convexe en `a` à `b,z` fixés. À
`a,b` fixés, la norme reste convexe en `z` et `-H` est convexe en `z`.

Un maximum séparément convexe sur un produit de boîtes est atteint à un tuple
de sommets, par maximisations successives. Il en résulte :

- `h_a` ou `h_b`, endpoint et site fixés : **8 coins** de la boîte partenaire ;
- cœur avec site `z` fixé : **64 couples** de coins de `A x B` ;
- cœur avec un nœud témoin `Z` entier : **512 triples** de coins de
  `A x B x Z`.

Chaque coin est testé sans racine avec `H>0` et `c_q H^2>Xi`. Cette décision est
nécessaire et suffisante pour l'**enveloppe AABB continue**, et elle conserve le
coût constant par bloc.

Deux autorités déjà présentes l'implémentent :

- `spindle_cone.hpp::all_lane_of_box`, lignes 258--302, pour les huit coins ;
- `soc64_rect.hpp::corner512_all_lane`, lignes 341--387, pour les 512 triples,
  se réduisant à 64 lorsque `Z` est singleton.

`PROPOSITION.md` section 4.6 dit déjà correctement que `Hmin/Ximax` est un
préfiltre conservateur et que `Corner512` est l'autorité d'enveloppe. La
section 6bis plus récente contredit donc une preuve et un microkernel existants.
Il ne faut pas créer une troisième autorité géométrique.

Ordonnance recommandée pour le cœur : test bon marché suffisant, puis
`Corner512(A,B,Z)` ; si la lane est ALL, créditer toute la population de `Z`
et désactiver cette lane dans les descendants ; sinon splitter. Cela augmente
simultanément `h_coeur` q3/q4 et évite des visites de feuilles.

## 8. AABB maximale n'est pas maximalité sur les PointId stockés

Même le test 8/64/512 exact sur l'enveloppe continue reste conservateur par
rapport aux ensembles discrets de la section 4. Un coin fictif de l'AABB peut
échouer alors que tous les vrais points passent.

Deux contre-exemples q2, plongés dans le plan `z=0`, suffisent :

- cœur : `A={(0,4),(4,0)}`, `B={(6,6)}`, site `z=(3,3)`. Les deux vraies
  valeurs de `H` valent 6, mais le coin fictif `a=(4,4)` donne `H=-6` ;
- facteur `h_a` : `a=(0,0)`, site `z=(3,3)`,
  `B={(0,8),(8,0)}`. Les deux vraies valeurs valent 6, mais le coin fictif
  `b=(0,0)` donne `H=-18`.

La hiérarchie de maximalité à annoncer est donc :

1. **maximum abstrait discret** : les ensembles `C_q`, `A_q`, `B_q` de la
   section 4 ;
2. **maximum exact sur l'AABB continue** : 8/64/512 coins ;
3. **sous-certificat courant** : extrema de composantes puis extrema `H/Xi`
   décorrélés.

Pour approcher le niveau 1 sans produit complet :

- q2, facteur de ligne : répondre exactement à
  `min_{b in B} (z-a) dot b - (z-a) dot z` par requête de support sur enveloppe
  convexe ou BVH ;
- q3/q4 : tester les sommets de `conv(B)` est exact pour un endpoint fixé ; le
  cœur discret demande en général des couples de sommets de `conv(A)` et
  `conv(B)` ;
- en pratique : AABB 8/64 d'abord, puis raffinement BVH borné des blocs MIXED,
  avec toute fin de budget en `UNKNOWN` fail-open.

Il est légitime de publier « maximal sur l'enveloppe AABB » après le correctif
8/64/512. Il ne sera légitime de publier « `h` maximal » sans qualificatif que
si les trois intersections discrètes sont décidées exactement.

## 9. Le coût courant ne réalise pas encore l'intention factorisée

Les lignes 790--806 comptent les survivantes par histogramme en
`O(|A|+|B|)` **une fois les `h` connus**. En revanche, les lignes 762--788
forment actuellement :

$$O\!\left(3\left(\lvert A\rvert^2+\lvert B\rvert^2\right)\right)$$

tests, auxquels s'ajoute la descente du cœur. Si `|A|` et `|B|` sont du même
ordre, éviter `A x B` tout en conservant `A x A` et `B x B` ne change pas
l'ordre asymptotique par rectangle. Le cap 512 borne l'expérience, pas une
architecture à l'échelle.

Une factorisation cohérente avec les preuves existantes est une auto-jointure
hiérarchique disjointe. Pour `h_a`, un bloc `U x Z` de PointId de `A` est ALL
pour la lane `q` si `Corner512(U,B,Z)>=q`. On ajoute alors paresseusement
`|Z|` à chaque endpoint de `U`; un bloc contenant une diagonale ou des pools
d'IDs qui se chevauchent est splitté. Les blocs MIXED sont raffinés et une fin
de budget reste fail-open. La construction est symétrique dans `B` et une
seule lane maximale traite q2/q3/q4 ensemble.

Cette route peut être sous-quadratique sur les données qui agrègent bien, sans
promettre un pire cas irréaliste. Elle répond beaucoup plus fidèlement à
l'objectif « surtout pas de boucle quadratique » que les deux self-joins
actuels.

## 10. Autres obligations avant réception

### Exact-once de la WSPD

La construction par produits des deux enfants de chaque LCA, puis raffinements
qui partitionnent un côté, admet une preuve exacte-once. Mais la sortie
`masse=C(n,2)` ne suffit pas comme oracle : une omission et un doublon de même
masse peuvent se compenser. Le champ `recouvrements=0` n'est actuellement
jamais calculé. Une porte bornée doit matérialiser chaque PairId non ordonné et
exiger une occurrence exactement.

### Histogramme hors `s_max=11`

Le CLI accepte `s_max<=32`, tandis que l'histogramme a 16 cases et rabat
`h_b>15` à 15. Pour `h_q>=16`, cela surcompte des survivantes : c'est sûr, mais
non maximal. Employer `h_q+1` cases dynamiques, ou refuser explicitement tout
profil autre que la cible `s_max=11`.

### Endpoints de même position

Une paire de PointId distincts avec `D=0` ne peut porter aucun support positif.
Le probe la laisse survivre ; cela reste fail-open, mais gonfle le résiduel et
ne mesure plus exactement les ancres propres. La `NeutralPairPartition` doit
filtrer cette seule paire endpoint, sans supprimer les multiplicités de
PointId vers les autres positions.

### Mutant de seuil

Le mutant `seuil-decale` emploie `h_q+1`. Il tue moins d'ancres et reste sûr ;
son commentaire « ferme des ancres vivantes » est inversé. Le mutant dangereux
est `h_q-1`.

## 11. Portes d'acceptation demandées à Claude

### P0, avant toute nouvelle mesure

1. Corriger ou désactiver le bulk q2 avec un masque de lanes par frame.
2. Graver la fixture `pop(Z)=5`, `h_2=10`, `hcore2=5`.
3. À petit `n`, reconstruire pour chaque PairId les vrais comptes q2/q3/q4 et
   exiger `closed_prefilter` inclus dans `true_dead`.
4. Dans le même oracle, vérifier chaque PointId crédité au plus une fois dans
   le cœur et la disjonction effective avec les pools `A` et `B`.
5. Calculer réellement la couverture PairId WSPD et exiger une occurrence par
   paire non ordonnée.

### P1, pour recevoir les « plus grands `h` AABB »

6. Remplacer les extrema `Hmin/Ximax` corrélés à tort par
   `all_lane_of_box` et `corner512_all_lane`.
7. Graver les quatre contre-exemples q3/q4 de la section 6, plus échange
   `A/B`, shell strict et positions dupliquées.
8. Corriger le sens de l'implication définissant `W_q` et la terminologie
   « arête maximale owner ».
9. Régénérer le reçu complet : q2 parce qu'il est invalide, q3/q4 parce que le
   nouveau prédicat doit augmenter ou conserver les crédits.

### P2, pour l'objectif de coût

10. Remplacer les self-joins ponctuels par les auto-jointures hiérarchiques
    disjointes, publier `ALL/MIXED/UNKNOWN`, visites, splits et résiduel.
11. Rendre l'histogramme cohérent avec le domaine déclaré de `s_max`.
12. Ajouter un raffinement discret borné après l'AABB si le gain marginal le
    justifie ; ne jamais appeler son résultat « maximal » après abandon
    `UNKNOWN`.

## 12. Rejeux de l'audit

Le source du pin a été compilé directement, CMake étant absent du conteneur :

```text
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -Imorsehgp3D_v2/include -Imorsehgp3D_v3 \
  morsehgp3D_v3/prototype/combined_prefilter_probe.cpp \
  -o /tmp/mhgp3v_combined_prefilter_probe
```

Résultat sain affiché par le probe malgré la fausse fermeture :

```text
lane q2 survivantes=3083 fermees=9637 ferme_pct=75.763
juge paires=12720 vivantes=18988 desaccords=0 coeur_faux=0
OK : prefiltre combine mesure
```

Le diagnostic indépendant de la section 5 réfute précisément ce vert. Aucune
suite CTest complète n'est donc revendiquée dans cet environnement.

GCP non utilisé.

## Conclusion d'audit

Le théorème utile à conserver est simple : les trois intersections de la
section 4 sont les plus grands facteurs disjoints autorisés par le contrat, et
leur somme à `10/9/8` est un prune mathématiquement sûr. Le chemin logiciel à
conserver est tout aussi net : autorités 8/64/512 coins existantes, crédits par
PointId exact-once et lane masks.

Au pin audité, q2 viole cette dernière obligation et ferme réellement à tort ;
q3/q4 ne sont que des sous-certificats conservateurs, contrairement aux claims
de maximalité. Le reçu q2 est bloqué jusqu'aux cinq portes P0, et aucune
nouvelle campagne ne doit précéder leur vert.
