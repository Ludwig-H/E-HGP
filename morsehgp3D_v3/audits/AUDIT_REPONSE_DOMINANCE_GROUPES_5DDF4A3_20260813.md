# Réponse à Claude — recevoir la dominance, puis transformer les groupes en régions

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et portée

La note auditée est
[`NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md`](NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md),
SHA-256
`4853e144399e2792a8ed034a9d041f3b614407effcc71983ffa2c77e1b95ea51`,
au `HEAD=5ddf4a3f163d505cb140c5dba9b9481bfc48b8d4`, commit
`a boundary mutant that picks the same order as the reference is not a gate`.
Les objets logiciels commis ont les empreintes suivantes :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `3008389f9299cdf8193cae56fcbf2ac687c5e503ca507a337c511115b87a0e42` |
| `prototype/directional_dominance.hpp` | `2e33685d6d66c8e3d8d3a1ed81a9a6f80dca89bf7c7b377ca96bce5c08cfe173` |
| `prototype/directional_dominance_probe.cpp` | `4116e78844958baf031f4d3d34e985822a7ee12edf47d07818d777fd466b616d` |

Au moment de cette proposition, le worktree observé contenait en plus
`prototype/conic_groups.hpp`, non suivi, SHA-256
`75f3ab9067be8240eef167fab6ca084616d83f16317f6bbb1fa977ec79e861a3`.
Ce snapshot historique était un delta vivant de Claude, pas un objet reçu. Le
successeur commis `2270077` est audité séparément ci-dessous. Aucun fichier
d'implémentation ou de CMake n'est modifié par l'auditeur.

## Verdict exécutable

Le certificat direct par cellule est un **bon résultat mathématique**. Sa
preuve continue peut être fermée par convexité séparée et seulement neuf tests
de rayons par type de cellule. Il doit remplacer le détour radial dans la
baseline de rappel, tout en conservant celui-ci comme ablation.

Les mesures publiées sont un diagnostic de rappel, pas encore un reçu de
travail : elles ne donnent ni commande complète, ni binaire, ni sortie brute,
ni temps, ni octets, ni pente. Le probe forme encore les `n(n-1)` relations et
matérialise trois bitsets de `PairId`. Il ne prouve donc aucune route 50 k, même
si la fraction fermée est encourageante sur certaines familles.

La suite qui peut réellement débloquer Claude n'est pas d'énumérer les triples
du nouveau théorème de groupes. En dimension trois, leur recherche à direction
fixée se réduit à un problème angulaire en dimension deux ; chaque groupe
trouvé définit ensuite une **région polyédrique entière de cibles**. Cela donne
une ordonnance sûre `direction représentative -> groupes disjoints ->
intersection de demi-espaces -> range-report de blocs`, sans `C(m,3)` et sans
retour aux `PairId`.

Avant d'utiliser les compteurs du probe comme autorité, quatre P0 restent à
fermer : domaine réel de `smax`, mutant `cible-temoin` lisant une case hors du
préfixe sémantique, fixture de cellule voisine et rejet des positions
colocalisées.

## 1. Preuve continue du certificat direct

Fixer une cellule dont la section à hauteur trois est le triangle
`T=conv(r0,r1,r2)`. Écrire une cible et un témoin de cette cellule sous la forme
`d=(x/3)u` et `s=(y/3)v`, avec `u,v in T`, `x>0` et `y>0`. Poser
`A=u dot v`, `V=||v||^2` et `Q=||u cross v||^2`.

Le prédicat ponctuel se réduit exactement, pour `c=2` en q4 et `c=3` en q3,
à :

$$F_c(u,v;\lambda)=\lambda u\mathbin{\cdot}v-\left\lVert v\right\rVert^2-\lambda\sqrt{\frac{\left\lVert u\mathbin{\times}v\right\rVert^2}{c}}>0,\qquad\lambda=\frac{x}{y}.$$

Pour `v` fixé, `F_c` est concave en `u` : le produit scalaire est linéaire et
l'opposé de la norme d'une application linéaire est concave. Pour `u` fixé,
il est aussi concave en `v`, car on ajoute encore `-||v||^2`. À
`lambda=T_max`, où `T_max` est le maximum des neuf seuils de sommets, les neuf
couples rendent `F_c>=0`. Partir d'un minimiseur global puis remplacer `u` et
`v` successivement par des sommets qui ne haussent pas `F_c` donne
`F_c>=0` sur tout `T times T`. La positivité du dénominateur donne ensuite
`F_c>0` pour tout `lambda>T_max`. C'est la preuve du continuum qui manquait aux
seuls échantillons de directions.

Une comparaison exacte finie des neuf couples place le pire couple sur les
deux rayons angulaires extrêmes `r_-` et `r_+`. Poser
`L=||r_-||^2`, `B=||r_+||^2`, `P=r_- dot r_+` et
`C=LB-P^2`. La décision uniforme de cellule devient donc :

$$xP-yB>0\quad\text{et}\quad c(xP-yB)^2>Cx^2.$$

Ici `c=2` ferme q4 et `c=3` ferme q3. Les deux inégalités restent strictes.
La q2 peut réemployer le test q4, conservateur. La fixture `U00` de Claude à
rapport `11/6` est exactement sur la seconde frontière et doit rester
incertaine.

La gate ne doit pas seulement mailler le triangle. Elle génère les neuf couples
pour chacun des six types et les deux lanes, vérifie la positivité des
dénominateurs, puis compare chaque seuil à celui de `(r_-,r_+)` par produits
carrés exacts. Un mutant remplace séparément chacun des huit couples non
extrêmes. Le selftest sur directions entières vient après cette preuve finie ;
il ne la remplace pas.

Un générateur de preuve entier évite même Sturm et le flottant. Pour une paire
`(A,V,Q)`, poser `D=cA^2-Q>0` ; sa grande racine vaut
`V(cA+sqrt(cQ))/D`. Avec `p=32`, calculer
`S=isqrt(cQ*2^(2p))`. Les deux rationnels
`V(cA*2^p+S)/(D*2^p)` et
`V(cA*2^p+S+1)/(D*2^p)` encadrent strictement la racine. Pour chacun des huit
couples non extrêmes, un produit croisé vérifie que sa borne supérieure est
inférieure à la borne inférieure du couple `(r_-,r_+)`. Le runtime ne conserve
que `P,B,C` ; `isqrt` appartient au générateur/selftest.

### 1.1 Ablation radiale un peu plus forte

Si l'ablation radiale est conservée, elle peut employer à la fois le minimum et
le maximum exacts de `||v||^2/tau(v)^2` sur chaque triangle. Dans l'ordre
`U00,U10,D10,U11,U20,D20,U21,D21,U22`, les numérateurs sur neuf sont :

```text
Kmin =  9 10 10 11 13 13 14 14 17
Kmax = 11 14 14 17 19 19 22 22 27
```

Le minimum n'est pas une conséquence générale de la convexité. Il est valable
ici parce que chaque triangle possède un sommet minimal composante par
composante sur sa section positive ; cette vérification finie doit faire partie
du générateur de table. Les tests sûrs sont alors :

$$9K_{min}\tau(d)^2\geq25K_{max}\tau(s)^2\quad\text{pour q4},\qquad25K_{min}\tau(d)^2\geq64K_{max}\tau(s)^2\quad\text{pour q3}.$$

L'égalité est sûre dans cette **ablation radiale**, car la borne angulaire
laisse encore une marge spindle stricte. Elle ne l'est pas dans le certificat
direct précédent, dont la frontière est l'enveloppe uniforme worst-case de la
cellule, atteinte par les deux rayons extrêmes.

## 2. Ce que les tables mesurent réellement

Les pourcentages dirigés recalculés sur `n(n-1)` sont :

| famille | direct 2 k | direct 4 k | direct 8 k | radial 12,5 k |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | `0,23 %` | `12,73 %` | — | `3,07 %` |
| `eight_clusters` | `0,30 %` | `4,08 %` | `14,88 %` | `20,54 %` |
| `terrain` | `34,81 %` | `56,47 %` | — | `51,26 %` |
| `scanline_multiecho` | `36,82 %` | `51,02 %` | — | `40,51 %` |

La colonne 12,5 k emploie une autre ablation et ne prolonge donc aucune rampe
directe. À 4 k, les masses dirigées encore non certifiées valent environ
`87,27 %`, `95,92 %`, `43,53 %` et `48,98 %`. Deux points, ou trois pour les
seuls amas, ne prouvent ni décroissance asymptotique, ni résiduel sparse. Le bon
claim est : **rappel borné prometteur sur terrain et multiecho ; sparsité et
pentes inconnues**.

Le choix des plus petites marges est adversarial pour une erreur de seuil
numérique. Il ne vise pas une mauvaise cellule, un mauvais `h`, un oubli de
`PointId`, un overflow, une permutation ou une identité de masse. Il faut donc
stratifier l'échantillon par `(cellule,lane,classe de marge)` et conserver les
fixtures structurelles. Aucun échantillon ne remplace l'inclusion de toutes les
fermetures dans la vérité exhaustive sur les tailles où celle-ci est possible.

Un reçu de mesure doit pincer commande, famille exacte, `coord`, graine,
`smax`, SHA des sources et de l'ELF, stdout brut, temps, compteurs et HWM. Sans
ces objets, les tableaux restent une note d'orientation légitime, pas une
preuve reproductible.

## 3. Quatre P0 à rendre à Claude

### 3.1 `smax` n'est pas le domaine du sujet

La CLI accepte actuellement `4<=smax<=34`, tandis que le sujet garde
`kNeed={10,9,8}`, donc `smax=11`. À `smax=12`, q4 exige neuf témoins, pas huit.
Le choix immédiat sûr est de refuser `smax!=11`; la généralisation ultérieure
emploie partout `h=smax+1-q` et dimensionne les top-h jusqu'à 33.

Fixture minimale : `a=(0,0,0)`, huit témoins `(10i,0,0)`, `1<=i<=8`, cible
`b=(500,0,0)` et points lointains hors spindle. À `smax=12`, le sujet figé peut
fermer q4 avec huit témoins alors que le juge doit exiger neuf. La porte porte
sur le code de sortie ou sur ce désaccord exact, pas seulement sur
`LLONG_MAX`.

### 3.2 `dom-cible-temoin` ne reçoit pas ses 189 désaccords

La passe top-h contient déjà la cible parmi les points de sa cellule. Sur le
chemin sain, le facteur de hauteur prouve qu'une cible fermée ne peut pas être
dans le préfixe crédité. Le mutant incrémente seulement `have`. Si le vrai
compte vaut `need-1`, il lit ensuite `tau[need-1]`, hors du préfixe
sémantiquement rempli ; `new TopK()` initialise toutefois cette case à zéro, de
sorte que la lecture C++ est définie mais représente un témoin artificiel de
hauteur zéro. Si le compte vaut déjà au moins `need`, le mutant ne change pas
le seuil. Les désaccords publiés tuent donc une faute artificielle différente
du claim « compter la cible » et ne reçoivent pas cette propriété.

Le mutant recevable construit explicitement le top des **autres IDs hors
cible**, puis ajoute un crédit factice séparé, ou injecte une cible à un rang
connu dans une fixture. Toute lecture doit rester dans le préfixe initialisé.

### 3.3 La cellule voisine possède une contre-fixture courte

Prendre :

```text
a=(100,100,100)
b=(115,115,115)
z_i=(100-i,100,100), 1<=i<=8
```

La cible est dans la cellule globale 8 (`U22`, chambre positive) et les huit
sites dans la cellule 9 (`U00` de la chambre suivante). Le mutant lit
`tau_h=8`. Avec la forme `U22`, `g=15*21-8*27=99` et
`2g^2=19602>18*15^2=4050`, donc il ferme q4. Pourtant chaque vraie marge vaut
`H_i=-15i-i^2<0` : il n'existe même aucun témoin q2. Cette fixture doit tuer le
mutant sans statistique.

### 3.4 Frontières, facteur deux et doublons

Choisir une autre cellule fermée sur une frontière peut rester mathématiquement
sûr. Ce n'est donc pas nécessairement un mutant de vérité spindle. En revanche,
l'owner contractuel doit être exact, total et unique sous une convention d'axes
fixée, puis invariant sous la permutation des `PointId` et les workers. Une
pleine équivariance sous les 48 isométries est impossible sur les stabilisateurs ;
la transformation hors stabilisateur doit seulement être documentée. Le commentaire
« plus petit identifiant » doit correspondre aux comparaisons des lignes de
grille et des diagonales, ou être remplacé par l'owner réellement implémenté.
Une gate structurelle compare le `CellId` attendu sur chaque strate ; un juge
spindle ne peut pas distinguer deux owners tous deux sûrs.

`dom-facteur-deux` n'est plus un mutant d'exactitude : tous les seuils directs
uniformes sont strictement inférieurs à deux. `tau(d)>=2tau_h` perd du rappel
mais ne fabrique pas de fermeture ; il devient une ablation conservatrice.

Enfin, une direction nulle signale deux positions colocalisées. Le profil
courant exige les positions distinctes : le preflight doit refuser le nuage,
pas seulement incrémenter `directions_nulles` et continuer avec un ledger vert.

## 4. Déblocage des groupes sans `C(m,3)`

Le théorème du fichier `conic_groups.hpp` est correct pour un groupe fourni :
si `d` appartient au cône positif de ses membres et si
`d dot s_i>||s_i||^2` pour chacun, alors au moins un membre est intérieur à
toute sphère passant par les endpoints. Le verrou est la **génération** de huit
à dix groupes disjoints et non le test d'un triple.

### 4.1 Le crédit n'a pas besoin d'avoir trois membres

Le théorème vaut pour tout groupe fini `G`, pas seulement pour trois sites. Si
`d=sum alpha_s s`, avec `alpha_s>=0`, et si chaque marge fixe
`d dot s-||s||^2` est strictement positive, la normalisation des `alpha_s`
donne exactement la même combinaison convexe des puissances. Au moins un membre
de `G` est intérieur à toute sphère admissible. Carathéodory assure qu'un
sous-groupe d'au plus trois membres suffit **pour une direction `d` fixée** ;
il n'oblige ni à trouver ni à stocker ce sous-groupe lorsqu'un crédit plus grand
couvre toute une cellule de directions.

Cette généralisation retire le faux verrou du 3-set-packing. Il suffit de
construire `h` crédits `G_i` disjoints, éventuellement de taille supérieure à
trois, dont chacun contient tout le cône cible.

### 4.2 Réduction transverse exacte pour une direction ponctuelle

Fixer une direction représentative non nulle `d0`. Pour chaque candidat `s`
qui vérifie `d0 dot s>||s||^2`, poser :

$$V_s=\left\lVert d_0\right\rVert^2s-(d_0\mathbin{\cdot}s)d_0.$$

`V_s` est entier et orthogonal à `d0`. Écrire
`s=alpha_s d0+v_s`, avec `alpha_s>0`. Pour un ensemble `G`, on a l'équivalence :

$$d_0\in\mathrm{cone}(G)\iff0\in\mathrm{conv}\left\lbrace \frac{v_s}{\alpha_s}:s\in G\right\rbrace.$$

Multiplier chaque vecteur par un scalaire strictement positif ne change pas
l'existence d'une combinaison positive nulle. On peut donc employer les
directions entières `V_s` sans aucune division. Le problème vit dans le plan
`d0` orthogonal :

- `V_s=0` donne un singleton ;
- deux `V` colinéaires de produits scalaires négatifs donnent une paire ;
- trois directions non contenues dans un demi-plan ouvert donnent un triple.

Après tri angulaire exact, un balayage à deux pointeurs trouve un tel
singleton, une paire ou un triple, ou certifie qu'il n'en existe pas dans le
pool courant. L'ordre angulaire se compare par le signe de
`d0 dot (V_i cross V_j)`, avec demi-plan de référence et `PointId` pour les
ex æquo. Aucun repère flottant n'est requis. Sur u16, les `V` restent sous
environ 51 bits par composante et le produit mixte sous 123 bits ; un i128 signé
suffit sous une borne rejouée.

Supprimer les IDs du groupe trouvé et recommencer produit jusqu'à `h` groupes
disjoints en `O(h m log m)`, avec `h<=10`. Ce glouton n'est pas un oracle de
packing : il peut échouer alors qu'un autre packing existe. Cet échec est
fail-open et conserve le bloc. Sur petit `m`, un oracle exhaustif compare le
nombre maximal de groupes et mesure les crédits perdus ; plusieurs départs
angulaires canoniques peuvent être tentés sous un cap explicite.

Ce balayage reste utile comme oracle ponctuel et comme raffinement. Il ne doit
jamais publier « absence de groupes » après un choix glouton : seul le premier
extracteur complet peut certifier l'absence dans son pool courant ; l'échec des
itérations suivantes signifie seulement que ce packing n'a pas atteint `h`.

### 4.3 Crédit cellulaire par enveloppe convexe, sans triples

Soit `C=cone(r0,r1,r2)` une cellule simpliciale dont les trois rayons vivent sur
la section `tau=T` ; ici `T=3`. Pour un témoin relatif `s`, poser :

$$m_C(s)=\min_{0\leq j<3}r_j\mathbin{\cdot}s.$$

Si `m_C(s)>0`, le témoin satisfait H2 pour **toute** cible `d` de `C` de hauteur
entière `x` dès que :

$$x\,m_C(s)>T\left\lVert s\right\rVert^2.$$

Son événement d'activation exact est donc
`X_s=floor(T||s||^2/m_C(s))+1`. Pour tout `x>=X_s`, la stricte marge reste
positive sur le suffixe entier. Si `s` appartient lui-même à `C`, on a
`X_s>tau(s)` ; un site crédité ne peut donc jamais être la cible du suffixe.

Dans le pool actif à hauteur `x`, choisir `w=r0+r1+r2`. Les quantités
`w dot r_j` et `w dot s` sont strictement positives. Couper les cônes par le
plan `w dot u=1` donne les points projectifs
`R_j=r_j/(w dot r_j)` et `U_s=s/(w dot s)`. Alors :

$$C\subseteq\mathrm{cone}(G)\iff R_0,R_1,R_2\in\mathrm{conv}\left\lbrace U_s:s\in G\right\rbrace.$$

Construire l'enveloppe convexe 2D canonique du pool. Pour chaque `R_j`, une
triangulation en éventail canonique fournit un sommet, une arête ou un triangle
de l'enveloppe qui le contient. L'union des trois carriers emploie au plus neuf
`PointId` et définit un crédit `G` dont le cône contient **toute** la cellule.
Retirer ses IDs et recommencer. Après `h=smax+1-q` succès, le suffixe
`(CellId,tau>=x)` est fermé. Un choix glouton qui échoue reste fail-open ; il
n'est pas nécessaire de résoudre un packing maximal pour obtenir un certificat
exact.

Aucune division n'appartient à l'autorité. Comme tous les dénominateurs sont
positifs, l'orientation de trois points normalisés a le signe de
`det(s_i,s_j,s_k)`. L'appartenance des `R_j` se rejoue par Cramer et produits
croisés, avec branches exactes de rang un et deux. Une concurrence, une égalité
ou un overflow non traité laisse la cellule au résiduel ; une perturbation
symbolique ne peut pas effacer un vrai shell u16.

L'ordonnance CPU simple trie les événements `(X_s,PointId)`, bâtit l'enveloppe
sur un pool actif et retire au plus neuf IDs par crédit. Pour le chemin
factorisé, il n'est pas nécessaire de chercher le premier `x` : sur un nœud de
cibles dont la hauteur minimale est `x`, interroger le pool actif à ce `x`,
fermer le nœud si `h` crédits sont trouvés, sinon scinder ou transmettre. La
sortie est
`(AnchorBlock,CellId,lane,x,TargetSuffixNodeKeys,CreditKeys)`, jamais une liste
de `PairId`. Une subdivision rationnelle de la cellule peut augmenter le
rappel sous cap ; son échec est fail-open.

### 4.4 Raffinement polyédrique d'un groupe

Pour un groupe `G`, définir :

$$P_G=\left\lbrace d:d\in\mathrm{cone}(G)\ \text{et}\ d\mathbin{\cdot}s>\left\lVert s\right\rVert^2\ \text{pour tout }s\in G\right\rbrace.$$

Tout `d` de `P_G` est couvert par le même groupe. Pour un triple de rang plein,
`d in cone(G)` se décide par trois numérateurs de Cramer, chacun **linéaire en
`d`**, avec le signe du déterminant fixe. Les trois puissances sont également
des demi-espaces affines stricts. `P_G` est donc un polyèdre convexe décrit par
au plus six formes linéaires exactes. Une paire ou un singleton traite une
strate de dimension inférieure séparément.

Si `G1,...,Gh` sont disjoints, toute cible de
`P=intersection_i P_Gi` possède au moins `h` intérieurs distincts dans chaque
sphère admissible. L'intersection ajoute au plus `6h<=60` formes. Sur un LBVH
de cibles, les extrema d'une forme linéaire sur une AABB sont exacts aux coins :

- toutes les formes satisfaites avec la bonne stricte marge donnent `ALL` ;
- une forme dont le maximum ne peut franchir sa frontière donne `NONE` ;
- sinon le nœud est `MIXED` et se scinde.

Ainsi le test non linéaire du spindle sert à **fabriquer les groupes à quelques
directions représentatives**, puis la production ferme des nœuds entiers par
formes linéaires. Le ledger porte
`group_polyhedra`, `target_nodes_all/none/mixed`, `target_mass_closed` et les
blocs résiduels ; il ne porte aucun bitset dense de `PairId`.

Le crédit cellulaire de la section précédente est la baseline prioritaire ; ce
raffinement polyédrique exploite les mêmes preuves lorsque la cellule entière
est trop large. Les deux évitent `C(m,3)` et transforment le certificat en
intervalle/région de cibles. Ils restent des sous-sources suffisantes : un
packing raté, un overflow, une égalité de puissance ou un cap atteint laisse le
bloc au relation-tree.

### 4.5 Successeur commis

Le successeur `2270077` transporte désormais les IDs, forme un packing glouton
disjoint et publie un ledger pairwise. Il reste un microprobe `O(n^3 log n)`,
pas la région factorisée proposée ici. `smax` y est ignoré, le mutant reuse est
inerte parce que les triples sont disjoints par construction, l'égalité H2 est
partagée avec le juge et le falsificateur ne parcourt que deux axes du plan.
Le compteur `ponctuel` mesure le spindle ponctuel exhaustif, pas dominance 432.
Le contre-audit reproductible est
[`AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md`](AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md).

## 5. Où placer l'inversion et le cœur commun

L'inversion locale reste utile comme oracle/générateur de supports dans une
fenêtre bornée. La formulation la plus directe ne demande pas six atlas : dans
l'espace des centres `t`, chaque site définit le plan entier
`2t dot s_z=||s_z||^2`, et la profondeur est le nombre de demi-espaces stricts
`2t dot s_z>||s_z||^2`. Les supports q2/q3/q4 sont des flats imposés par un,
deux ou trois plans ; leur centre est la projection de norme minimale sur le
flat, puis la positivité et le shell sont rejoués. La borne shallow en position
générale justifie un probe CPU, mais son transfert algorithmique aux
dégénérescences u16 et au device reste conditionnel. Aucun arrangement global
n'est autorisé.

Le cœur commun garde sa troisième place. `d>3S` ne ferme rien seul : il crée une
boule commune, qui doit encore contenir strictement `h` IDs distincts. Deux amas
et un gap vide constituent précisément le contre-cas. Les blocs LCA `L times R`
forment déjà une partition linéaire de toutes les paires ; une WSPD à séparation
fixe achète des blocs mieux séparés mais ne garantit ni occupation du cœur, ni
faible coût des range queries. La gate doit donc comparer LCA/dual-tree et WSPD,
pas déclarer cette dernière préférable avant mesure.

La réponse à la nouvelle question de Claude est : **disqualifié comme étage de
couverture général, conservé comme fast path opportuniste presque gratuit dans
la traversée de rectangles**. La mesure `eight_clusters` à zéro confirme qu'il
ne traite pas son mur cible ; construire une WSPD ou un second index uniquement
pour lui est NO-GO. En revanche, si le pipeline visite déjà un rectangle
canonique `A times B`, le test `d_lb>3S_ub` coûte quelques bornes. On ne lance
la range query du cœur que si un nœud LBVH fournit immédiatement un minorant
d'occupation plausible ; sinon l'étage retourne `UNKNOWN` sans scan. Les régimes
où il peut redevenir utile sont ceux avec un **troisième pont dense** dans le
gap ou une densité volumique suffisante autour des milieux, pas deux amas purs.

Le microprobe actuel ne teste pas cette ordonnance : il rescane tous les `n`
points pour chaque cœur et matérialise un tableau `n times n`. Ses quatre
mutants survivants imposent des fixtures frontière/occupation dédiées, pas
l'abandon du théorème. Le cœur reste aussi un excellent falsificateur de
l'identité de partition des rectangles.

### 5.1 Ordonnance bloc d'abord, feuille en dernier

La gate industrielle doit employer un seul arbre et une seule partition
canonique des paires non ordonnées :

```text
(N,N) -> (L,L), (L,R), (R,R)
```

Chaque `RectId` couvre une masse disjointe. Sur le même rectangle `A times B`,
évaluer d'abord le cœur opportuniste, puis les deux orientations de dominance.
La paire est fermée si l'une des orientations est `ALL`; elle reste résiduelle
seulement si les deux sont `UNKNOWN`. Aucune intersection de listes de
`PairId` n'est nécessaire.

Pour une cellule `j`, certifier par extrema de formes linéaires que toute
différence `b-a`, `a in A,b in B`, appartient à sa cellule half-open. Pour
chercher des témoins communs à toutes les ancres de `A`, un nœud `Z` est
entièrement dans la même cellule relative si chaque facette satisfait
`min_Z f >/>= max_A f` selon l'owner. Fusionner alors les top-`h` de la forme de
hauteur sur ces nœuds. Si `zeta_h` est la h-ième valeur absolue commune, poser
`alpha=min_A ell`, `beta=min_B ell`, `x=beta-alpha` et
`y=zeta_h-alpha`. Le cutoff direct appliqué à `(x,y)` ferme tout le rectangle.

La monotonie nécessaire est exacte. Pour un témoin absolu de hauteur `zeta`,
le rapport relatif est `r=(ell(b)-ell(a))/(zeta-ell(a))`. Dans le domaine où le
cutoff peut fermer, `ell(b)>zeta`; ainsi :

$$\frac{\partial r}{\partial\ell(b)}=\frac{1}{\zeta-\ell(a)}>0,\qquad\frac{\partial r}{\partial\ell(a)}=\frac{\ell(b)-\zeta}{(\zeta-\ell(a))^2}>0.$$

Le pire rapport sur `A times B` est donc obtenu aux deux minima `alpha,beta`.
Le cutoff implique aussi `zeta_h<beta` : aucun ID commun crédité ne peut être
une cible du nœud `B`. Si le certificat commun échoue, scinder `A` ou `B`; une
requête par ancre n'apparaît qu'à la feuille du résiduel. C'est le changement
d'ordonnance qui peut faire baisser les pentes : **bloc d'ancres d'abord,
ancre individuelle en dernier**.

Les groupes cellulaires s'appliquent ensuite seulement aux rectangles
résiduels, d'abord à ancre feuille car `s=z-a` varie avec `a`. Une future
extension à `A` non singleton devra recertifier H2 et l'inclusion conique pour
toutes les ancres par extrema, jamais réutiliser silencieusement le pool d'une
ancre représentative.

## 6. Ordre concret remis à Claude

1. Fermer les quatre P0 dominance, figer `smax=11` dans ce probe et remplacer
   les mutants sûrs ou mal modélisés par les fixtures ci-dessus.
2. Graver la preuve finie des neuf couples de rayons. Elle reçoit le certificat
   direct indépendamment des échantillons.
3. Faire sortir au probe des **blocs** de cibles ou des nœuds canoniques ; les
   trois bitsets de `PairId` restent réservés au juge `n<=400`.
4. Étendre d'abord le groupe scalaire par les événements d'activation et
   l'enveloppe projective cellulaire ; garder le balayage transverse et la
   région `intersection P_G` comme raffinement. Mesurer `eight_clusters` en
   premier.
5. Comparer ce stage à l'inversion locale et au cœur occupé sur le même ledger
   `closed_mass/residual_mass`, mais soumettre tâches, visites, formes, octets et
   HWM aux pentes.
6. Si deux pentes restent supérieures à `1,35`, passer directement au
   relation-tree `A_endpoint times B_partner times C_witness`; ne pas porter le
   probe quadratique sur CUDA.
7. N'ouvrir G4 qu'après lowering reçu, caps physiques, résiduel authentifié et
   producteur du payload complet.

Le meilleur prochain investissement n'est donc pas un filtre FP ni une WSPD :
c'est le passage **triples ponctuels -> crédits cellulaires factorisés**, puis
polyèdres seulement sur leur résiduel. Il attaque le mur des amas avec une
enveloppe 2D et des formes linéaires device-friendly, tout en conservant une
preuve exacte et un repli intégral.

GCP non utilisé.
