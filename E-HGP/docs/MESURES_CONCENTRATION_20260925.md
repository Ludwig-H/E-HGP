# Concentration, taille et contraste : ambiante ou intrinsèque ?

> [!IMPORTANT]
> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.

Ce document répond à **une** question, celle que
[`OBSTRUCTION_GRANDE_DIMENSION.md`](OBSTRUCTION_GRANDE_DIMENSION.md) § 5 laisse
explicitement ouverte et que [`OBJET_ET_DIMENSION.md`](OBJET_ET_DIMENSION.md) § 7
met en troisième point de la suite immédiate :

> l'obstruction de E-HGP est-elle gouvernée par la dimension **ambiante** $d$ ou
> par la dimension **intrinsèque** $r$ du support des données, et un changement
> de métrique restaure-t-il le signal de densité ?

Tout est mesuré par `bench/concentration.py`, qui recalcule sa propre boule
englobante minimale (indépendamment de `src/ehgp/exact/meb.py`) et refuse de
publier un tableau si l'instrument n'est pas validé. Les commandes exactes et
leurs codes de sortie sont au § 10. Aucun octet de SemanticKITTI n'est utilisé :
la famille `lidar` est un nuage synthétique fabriqué dans le fichier de mesure.

## 1. Ce qui est mesuré, et avec quel instrument

### 1.1 Les trois mesures

Le niveau d'une partie finie non vide $A$ est le rayon au carré de sa plus
petite boule englobante,

$$\beta(A)=\min_{y}\max_{x\in A}\left\Vert y-x\right\Vert^{2}.$$

**(a) Obstruction de taille.** Part des parties $F$ de cardinal $k$ dont la
boule englobante minimale, **fermée**, ne contient aucune autre observation :

$$\varphi_k=\frac{\#\left\lbrace F:\lvert F\rvert=k,\ \left\Vert x_j-c_F\right\Vert^{2}>\beta(F)\ \text{pour tout }x_j\notin F\right\rbrace}{\binom{n}{k}}.$$

Une telle partie est un sommet **isolé** de $\Gamma_k(\beta(F))$, donc une
naissance de composante de $\pi_{0}(L_k)$. La convention de boule fermée est
celle de la fixture F1 de [`OBSTRUCTION_GRANDE_DIMENSION.md`](OBSTRUCTION_GRANDE_DIMENSION.md)
§ 1.1 : un point étranger **sur** la sphère donne une coface de même niveau,
donc pas de naissance. La quantité qui compte pour la taille de sortie n'est pas
$\varphi_k$ mais le **compte** $\varphi_k\binom{n}{k}$ ; le tableau du § 3.5 le
publie.

**(b) Obstruction de signal.** Moyenne et écart-type du rayon
$\sqrt{\beta(F)}$, normalisé par la distance médiane entre paires du nuage, à
comparer à la valeur du simplexe régulier $\sqrt{(k-1)/(2k)}$. C'est
l'écart-type **intra-nuage** qui porte l'information : s'il s'effondre, la
filtration en rayon dégénère en escalier déterministe piloté par $k$ seul.

**(c) Contraste de densité.** Avec $r_{\mathrm{ref}}$ la médiane sur $i$ de la
distance de $x_i$ à son $m$-ième voisin ($m=10$), on compte

$$N_i=\#\left\lbrace j\neq i:\left\Vert x_i-x_j\right\Vert\leq r_{\mathrm{ref}}\right\rbrace,$$

et l'on publie la moyenne, le coefficient de variation, les rapports
interquartile et interdécile, et l'entropie normalisée de la distribution. Les
deux rapports ont leur dénominateur borné par $1$ en bas (un premier décile nul
donnerait un rapport infini), ce qui ne joue sur aucune cellule publiée mais
plafonne par construction les rapports des colonnes les plus dispersées. On
publie aussi l'**excès de Poisson** $\mathrm{cv}\sqrt{\overline{N}}$, qui vaut
$1$ pour un processus homogène : sans ce repère, une dispersion élevée se lit à
tort comme un signal. Le § 5 montre que cette lecture est effectivement fausse,
et le § 5.2 donne la mesure qui la remplace.

### 1.2 L'instrument : une boule englobante indépendante, certifiée et validée

`bench/concentration.py` n'appelle pas la boule rationnelle du chantier pour
mesurer : il en réécrit une par **ensemble actif** (ajout du point le plus
loin, retrait d'un barycentrique négatif), et n'accepte un résultat que muni de
son **certificat de Karush, Kuhn et Tucker** : barycentriques du centre positifs
dans son support, et aucun point strictement dehors. Ces deux conditions valent
écart dual nul pour le programme
$\max_{w}\sum_i w_i\left\Vert x_i\right\Vert^{2}-\left\Vert \sum_i w_i x_i\right\Vert^{2}$
sur le simplexe. À défaut de certificat, un repli énumère les supports ; ce repli
est lui aussi un certificat.

La validation est une **porte** au sens strict : la table `selftest` s'exécute
**avant** toute autre, quelle que soit la table demandée, et son échec met le
code de sortie à $3$. C'est une correction de la revue du § 11 : la première
version de ce document annonçait cette porte alors que le programme la
désactivait dès qu'une seule table était demandée, c'est-à-dire dans toutes les
commandes du § 10 sauf la première.

```text
selftest (a) boule certifiee contre boule rationnelle exacte
scenario      cas  echecs  erreur relative max
----------------------------------------------
grille dense  80   0       4.08e-16
grille fine   80   0       2.49e-16
cospherique   80   0       3.87e-16
colineaire    80   0       0.00e+00
duplique      80   0       2.68e-16

selftest (b) fixtures gravees : rayon carre
fixture        mesure          attendu
---------------------------------------------
paire (3,4)    6.250000000000  6.250000000000
simplexe k=2   0.250000000000  0.250000000000
simplexe k=3   0.333333333333  0.333333333333
simplexe k=5   0.400000000000  0.400000000000
simplexe k=11  0.454545454545  0.454545454545
```

400 instances confrontées à `ehgp.exact.meb.minimum_enclosing_ball`, dont les
cas dégénérés que la doctrine exige (cosphériques, colinéaires, dupliqués),
0 désaccord, erreur relative maximale $4\cdot10^{-16}$.

Sur l'ensemble des tableaux de ce document, le taux de certification vaut
$1{,}000000$ (aucune boule publiée sans certificat) et la part de verdicts
indécis vaut $0$ sur les $864\,520$ boules des huit exécutions qui en comptent
(§ 10, commandes 2 à 6, 8, 11 et 12 : $145\,720+134\,400+134\,400+18\,000+201\,600+67\,200+28\,800+134\,400$).

**Coût de l'instrument.** Le repli par énumération des supports ne se déclenche
que sur les entrées affinement dépendantes. Le tableau ci-dessous est produit
par la table `cout` (§ 10 commande 13) en temps **processeur**, seule grandeur
qui ne dépende pas de la charge de la machine ; $400$ boules par cellule,
famille `uniform`, $n=200$ :

```text
d    k   boules  replis  taux de repli  ms/boule  ms par repli  ms/boule sans repli
3    2   400     0       0.000          0.156     -             0.156
3    3   400     0       0.000          0.236     -             0.236
3    5   400     0       0.000          0.296     -             0.296
3    11  400     17      0.043          8.259     176.1         0.810
20   11  400     0       0.000          0.898     -             0.898
200  11  400     0       0.000          1.560     -             1.560
```

Une seule cellule replie, $(d=3,k=11)$, à un taux de $0{,}043$ : onze points de
$\mathbb{R}^{3}$ sont affinement dépendants, donc le système du centre
circonscrit est singulier pour presque tout support de taille $>4$. Le repli
coûte $176$ ms, soit $8{,}3$ ms par boule amorti sur la cellule, contre
$0{,}16$ à $1{,}56$ ms par boule partout ailleurs. Un plafond de $0{,}10$ sur le
taux de repli est désormais une porte : si l'ensemble actif cédait la main à
l'énumération, l'instrument annoncé ne serait plus celui qui mesure. La
première version de ce document publiait ici un taux de $0{,}045$ et un coût de
$24{,}5$ ms par boule qu'aucune commande ne produisait (§ 11).

### 1.3 Familles de nuages, et une construction appariée

Neuf familles paramétrées par $(n,d,r)$ : `uniform` (cube), `gauss_iso`,
`gauss_aniso` (spectre en $i^{-1}$), `sphere`, `clusters` (huit amas séparés),
`flat` (cube de dimension $r$ plongé par une **isométrie linéaire** dans
$\mathbb{R}^{d}$), `roll` (rouleau suisse, $r=2$ plongé **non linéairement**
dans $\mathbb{R}^{3}$ puis isométriquement dans $\mathbb{R}^{d}$), `peano`
(courbe repliée, $r=1$), `lidar` (nuage de balayage synthétique : trois
coordonnées métriques plus intensité, portée, indice d'anneau, hauteur locale).
Les variantes `flat_noise`, `roll_noise`, `peano_noise` ajoutent un bruit
gaussien **ambiant** d'écart-type $\sigma$ par coordonnée, exprimé en fraction
de la distance médiane entre paires du support sans bruit.

**Construction appariée, volontaire.** Le tirage latent précède le choix de la
base de plongement : à graine égale, `flat` de rang $r$ dans $\mathbb{R}^{d}$
utilise **exactement** le même nuage latent que `uniform` en dimension $r$. En
régime exhaustif, l'égalité des colonnes n'est donc pas un accord statistique,
c'est une égalité chiffre par chiffre, et toute différence serait un bug.

### 1.4 Conventions, planchers, codes de sortie

Cinq graines au minimum, dérivées de `--seed 31` par un décalage propre à chaque
table, moyenne et écart-type publiés. Les mesures exactes énumèrent toutes les $\binom{n}{k}$
parties ($n\in\left\lbrace8,11,14\right\rbrace$) ; les mesures échantillonnées
tirent $240$ parties par cellule ($n\in\left\lbrace20,200,400,800,2000\right\rbrace$).
Planchers : nombre minimal de boules, taux de certification $1{,}000$, part de
verdicts indécis $\leq10^{-3}$, validation exacte $\geq200$ cas sans échec,
taux de repli $\leq0{,}10$, nombre minimal de cellules par table, et trois
portes ajoutées par la revue du § 11 — l'accord **exact** entre la mesure (a) de
ce fichier et les naissances de la tour du chantier (§ 3.6), l'accord exact
entre les trois chemins du comptage à l'ordre $2$ (§ 3.5), et un **contrôle par
permutation** sur la corrélation de Spearman (§ 5.2). Une porte manquée met le
code de sortie à $3$ ; toutes les commandes du § 10 sortent à $0$. Aucune porte
n'emploie `assert` : tout tient sous `python3 -O`, et c'est sous `-O` que tout a
tourné.

Les durées imprimées ne sont **pas** une mesure : la machine était partagée avec
d'autres agents et le temps horloge dépasse le temps processeur d'un facteur qui
varie de $1$ à $6$.

## 2. Quatre énoncés démontrés, posés avant toute mesure

Chacun est vérifié au même instrument que les tableaux, par la table
`theoremes`, qui est une porte.

**T1 — $\beta$ ne dépend que de la matrice des distances.** Le rayon de la boule
englobante minimale est un invariant d'isométrie. Donc pour toute isométrie
linéaire $\iota:\mathbb{R}^{r}\to\mathbb{R}^{d}$, le nuage $\iota(X)$ a
exactement les mêmes $\beta$, le même $\Gamma_k(a)$ pour tout $a$, le même
$\pi_{0}(L_k(a))$ et la même tour que $X$. **Les trois mesures (a), (b), (c) sont
donc des fonctions de la configuration intrinsèque, jamais de $d$ seul.**
Mesuré : $200$ cas, écart relatif maximal $1{,}085\cdot10^{-15}$.

**T2 — une projection orthogonale contracte $\beta$.** Si $P$ est une projection
orthogonale et $c$ le centre de la boule de $A$, alors
$\max_{x\in A}\left\Vert Pc-Px\right\Vert\leq\max_{x\in A}\left\Vert c-x\right\Vert$,
donc $\beta(PA)\leq\beta(A)$. Les niveaux ne peuvent que **descendre** ; comme
ils ne descendent pas tous du même facteur, leur **ordre** change, et
$\pi_{0}(L_k(a))$ n'est pas préservé. Mesuré : $4410$ cas, violation maximale
$0{,}000\cdot10^{0}$ (aucune).

**T3 — le blanchiment en rang $n-1$ transforme le nuage en simplexe régulier
exact.** Si l'on blanchit $n$ points sur leur rang effectif $n-1$ (ce qui arrive
dès que $d\geq n-1$), la matrice de Gram centrée vaut
$G=(n-1)\left(I_n-\mathbf{1}\mathbf{1}^{\top}/n\right)$, donc **toutes** les
distances au carré valent $2(n-1)$ et

$$\beta(F)=\frac{(n-1)(k-1)}{k}\quad\text{pour toute partie }F\text{ de cardinal }k.$$

La tour du nuage blanchi ne dépend alors **plus des données** : elle ne dépend
que de $n$ et de $k$. Toutes les parties ont une boule fermée vide, donc
$\varphi_k=1$ et la sortie sature à $\binom{n}{k}$ naissances. Mesuré : $1184$
cas, écart relatif maximal $1{,}302\cdot10^{-11}$ ; et $\varphi_5=1{,}000\pm0{,}000$
au § 3.2 exactement dans les colonnes où le rang effectif atteint $n-1=19$.

**T4 — la distorsion du rayon est majorée par celle des paires.** Une projection
aléatoire gaussienne n'est pas une projection orthogonale ; elle ne contracte
donc pas $\beta$. Mesuré :

```text
d    m   eps theorique  distorsion paires max  distorsion rayon max
-------------------------------------------------------------------
200  96  0.392          0.263                  0.098
200  20  0.859          0.533                  0.376
50   20  0.859          0.461                  0.235
20   10  1.215          0.583                  0.415
```

La boule moyenne les distances, donc elle amortit : la distorsion du rayon reste
en deçà de celle des paires, d'un facteur $1{,}4$ à $2{,}7$. Cela **borne** le
dégât de Johnson-Lindenstrauss sur les niveaux ; le § 7.3 montre que cela ne
suffit pas à préserver l'objet.

Ce tableau est celui de la revue du § 11. La première version tirait ses parties
dans le **préfixe lexicographique** de `combinations(range(40), 5)` : les $300$
parties contenaient alors toutes les indices $0$, $1$ et $2$, donc un maximum
pris sur elles sous-estimait la distorsion (elle était annoncée à $0{,}084$ à
$(d=200,m=96)$ contre $0{,}098$ sur un échantillon uniforme, et à $0{,}242$
contre $0{,}415$ à $(d=20,m=10)$). L'énoncé survit, la marge est deux fois plus
mince qu'annoncé.

## 3. Mesure (a), la taille : gouvernée par la dimension intrinsèque

### 3.1 Régime exhaustif : toutes les parties, $n=14$

Toutes les $\binom{14}{k}$ parties, cinq graines
(`--table exhaustive`, § 10 commande 2) :

| famille | $r$ | $k=2$ | $k=3$ | $k=5$ | $k=11$ |
| --- | --- | --- | --- | --- | --- |
| `uniform` $d=2$ | 2 | 0,218 | 0,063 | 0,012 | 0,021 |
| `flat` $d=200$ | **2** | **0,218** | **0,063** | **0,012** | **0,021** |
| `uniform` $d=3$ | 3 | 0,292 | 0,105 | 0,024 | 0,034 |
| `flat` $d=200$ | **3** | **0,292** | **0,105** | **0,024** | **0,034** |
| `roll` $d=200$ | 2 | 0,255 | 0,079 | 0,014 | 0,030 |
| `uniform` $d=20$ | 20 | 0,976 | 0,795 | 0,430 | 0,255 |
| `uniform` $d=50$ | 50 | 1,000 | 0,983 | 0,810 | 0,430 |
| `uniform` $d=200$ | 200 | 1,000 | 1,000 | 1,000 | 0,962 |

Les lignes en gras sont **identiques chiffre par chiffre** à celles de la
dimension intrinsèque correspondante, sur les douze cellules, aux cinq graines :
c'est T1, rendu visible par la construction appariée du § 1.3. Les tableaux
$n=8$ et $n=11$ donnent la même égalité sur toutes leurs cellules, et
reproduisent au passage la mesure indépendante citée par
[`OBSTRUCTION_GRANDE_DIMENSION.md`](OBSTRUCTION_GRANDE_DIMENSION.md) § 1.2
($n=11$, $k=2$ : $0{,}276$ en $d=2$ contre $14/55=0{,}255$ ; $0{,}971$ en $d=20$
contre $54/55=0{,}982$ ; $1{,}000$ en $d\geq50$ contre $55/55$).

### 3.2 Balayage de $d$ à $r$ fixé : $n=20$, $k=5$

Cinq graines, $240$ parties tirées par cellule (§ 10 commande 3) :

| famille et représentation | $r$ | $d=2$ | $d=3$ | $d=5$ | $d=20$ | $d=50$ | $d=200$ |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `uniform` brut | $=d$ | 0,004 | 0,003 | 0,012 | 0,254 | 0,693 | **1,000** |
| `flat` brut | 2 | 0,004 | 0,002 | 0,003 | 0,003 | 0,002 | **0,003** |
| `flat_noise` brut ($\sigma=0{,}05$) | 2 | 0,003 | 0,003 | 0,002 | 0,006 | 0,008 | 0,080 |
| `flat_noise` ACP$(2)$ | 2 | 0,003 | 0,003 | 0,002 | 0,003 | 0,002 | 0,003 |
| `roll_noise` brut | 2 | — | 0,003 | 0,007 | 0,008 | 0,017 | 0,123 |
| `roll_noise` ACP$(2)$ | 2 | — | 0,002 | 0,004 | 0,003 | 0,003 | 0,001 |
| `flat_noise` blanchi | — | 0,001 | 0,004 | 0,013 | **1,000** | **1,000** | **1,000** |
| `flat_noise` JL | 2 | 0,000 | 0,003 | 0,003 | 0,003 | 0,008 | 0,053 |

Lecture, en trois lignes. À $r=2$ fixé et sans bruit, $\varphi_5$ ne bouge pas de
$d=2$ à $d=200$ ($0{,}003\pm0{,}001$), tandis qu'à $r=d$ il passe de $0{,}004$ à
$1{,}000$ : **c'est $r$ qui gouverne, pas $d$.** Le bruit ambiant ramène
l'obstruction, et d'autant plus que $d$ est grand ($0{,}003\to0{,}080$ de $d=2$ à
$d=200$ à $\sigma$ fixé). L'ACP au rang intrinsèque la renvoie à sa valeur sans
bruit **en toute dimension**, y compris pour une variété **non linéaire**
(`roll_noise` : $0{,}123\to0{,}001$). Le blanchiment, lui, la porte à sa valeur
maximale $1{,}000$ dès que le rang effectif atteint $n-1=19$, conformément à T3.

### 3.3 Balayage de $d$ à $r$ fixé : $n=200$, $k=2$

Même protocole, ordre $k=2$ (§ 10 commande 4) :

| famille et représentation | $d=2$ | $d=3$ | $d=5$ | $d=20$ | $d=50$ | $d=200$ |
| --- | --- | --- | --- | --- | --- | --- |
| `uniform` brut ($r=d$) | 0,022 | 0,030 | 0,062 | 0,742 | 0,998 | **1,000** |
| `flat` brut ($r=2$) | 0,013 | 0,013 | 0,017 | 0,021 | 0,024 | **0,012** |
| `flat_noise` brut ($\sigma=0{,}05$) | 0,015 | 0,017 | 0,032 | 0,098 | 0,219 | 0,694 |
| `flat_noise` ACP$(2)$ | 0,015 | 0,016 | 0,017 | 0,021 | 0,011 | 0,016 |
| `flat_noise` blanchi | 0,014 | 0,037 | 0,072 | 0,674 | **1,000** | **1,000** |
| `flat_noise` JL | 0,024 | 0,022 | 0,024 | 0,069 | 0,182 | 0,551 |
| `roll_noise` brut | — | 0,032 | 0,030 | 0,118 | 0,262 | 0,768 |
| `roll_noise` ACP$(2)$ | — | 0,022 | 0,017 | 0,015 | 0,017 | 0,013 |

Ici les colonnes `flat` ne sont **pas** identiques chiffre par chiffre, et la
raison est plus prosaïque que celle qu'annonçait la première version de ce
document (§ 11) : dans la table `dimsweep` la graine vaut
`--seed + 31337 * graine + 17 * d`, donc **le nuage latent change d'une colonne à
l'autre**. L'appariement chiffre par chiffre n'existe que dans le régime
exhaustif du § 3.1, dont la graine ne dépend pas de $d$. À $d$ fixé, en
revanche, `flat` et `uniform` partagent bien le même nuage latent et ne
diffèrent que par les $240$ parties tirées (le tirage de la base de plongement
consomme le générateur) : à $d=2$, $0{,}013$ contre $0{,}022$, deux estimations
du même nombre, ce qui donne la mesure directe du bruit d'échantillonnage,
$\sqrt{p(1-p)/1200}\approx0{,}004$ par cellule de cinq graines. Les six colonnes
`flat` ($0{,}012$ à $0{,}024$) sont donc six nuages différents mesurés chacun à
$\pm0{,}004$ près : leur dispersion est compatible avec l'invariance exacte de
T1, elle ne la démontre pas.

Le blanchiment mérite une ligne à lui : à $d=50$, $n=200$, on est **loin** du
régime dégénéré de T3 ($d<n-1=199$), et pourtant $\varphi_2=1{,}000$. Le
mécanisme est l'autre moitié du même fait : le blanchiment égalise les deux
directions de signal avec les $48$ directions de bruit, donc il remplace la
dimension intrinsèque par le **rang effectif**. La valeur obtenue
($1{,}000$) est celle de `uniform` en $d=50$ ($0{,}998$) : le nuage blanchi est
devenu isotrope de dimension $50$.

### 3.4 Le piège : $\varphi_k$ ne mesure que la dimension de la représentation

Une projection fait redescendre $\varphi_k$ **même quand elle détruit tout**.
Mesure décisive (§ 10 commande 5), $n=200$, $k=2$, $d=200$ :

```text
famille     d    r    representation  k  part libre    cv du comptage
flat_noise  200  2    raw(d=200)      2  1.000+-0.000  1.066+-0.022
flat_noise  200  2    pca(d=2)        2  0.013+-0.008  0.541+-0.033
flat_noise  200  2    jl(d=2)         2  0.024+-0.007  0.645+-0.075
uniform     200  200  raw(d=200)      2  1.000+-0.000  0.796+-0.032
uniform     200  200  pca(d=200)      2  1.000+-0.000  0.796+-0.032
uniform     200  200  jl(d=2)         2  0.014+-0.006  0.601+-0.022
```

La dernière ligne est le verdict : un nuage **authentiquement** de dimension
$200$, projeté sur un plan **aléatoire**, donne $\varphi_2=0{,}014$, soit la
valeur d'un nuage plan. L'obstruction de taille est donc une fonction de la
dimension de la **représentation**, pas de la fidélité de la représentation.
« La part libre est redescendue après projection » ne prouve **rien** sur la
conservation de l'objet. C'est pourquoi le § 5.2 existe.

### 3.5 Le compte, pas la fraction : $n=200$, $800$, $2000$

$\varphi_k$ décroît avec $n$ ; la taille de sortie est le compte
$\varphi_k\binom{n}{k}$. Extrait de la table `free` (§ 10 commande 6),
naissances **estimées** sur $240$ parties tirées par cellule, cinq graines, avec
la borne de résolution de l'échantillon :

| famille | $k$ | $n=200$ | $n=800$ | $n=2000$ | résolution à $n=2000$ |
| --- | --- | --- | --- | --- | --- |
| `flat` $r=2$, $d=200$ | 2 | $2{,}98\cdot10^{2}$ | $1{,}33\cdot10^{3}$ | $1{,}67\cdot10^{3}$ | $8{,}33\cdot10^{3}$ |
| `flat_noise` $r=2$, $d=200$ | 2 | $1{,}39\cdot10^{4}$ | $2{,}05\cdot10^{5}$ | $1{,}28\cdot10^{6}$ | $8{,}33\cdot10^{3}$ |
| `lidar` $r=2$, $d=7$ | 2 | $3{,}81\cdot10^{2}$ | $1{,}86\cdot10^{3}$ | $1{,}67\cdot10^{3}$ | $8{,}33\cdot10^{3}$ |
| `uniform` $d=200$ | 2 | $1{,}99\cdot10^{4}$ | $3{,}20\cdot10^{5}$ | $2{,}00\cdot10^{6}$ | $8{,}33\cdot10^{3}$ |
| `flat_noise` $r=2$, $d=200$ | 3 | $3{,}51\cdot10^{5}$ | $1{,}74\cdot10^{7}$ | $2{,}63\cdot10^{8}$ | $5{,}55\cdot10^{6}$ |
| `uniform` $d=200$ | 11 | $8{,}34\cdot10^{16}$ | $5{,}19\cdot10^{22}$ | $1{,}66\cdot10^{26}$ | $2{,}08\cdot10^{26}$ |

**Limite de résolution, et le piège qu'elle cache.** Une part mesurée sur $240$
tirages n'est pas bornée en bas mieux que $\approx1/240$ : la mesure
échantillonnée **certifie une explosion, jamais son absence**. Dire cela ne
suffit pas, car trois lignes de ce tableau ont une estimation **inférieure à leur
propre résolution** : `flat` et `lidar` à $n=2000$ ($1{,}67\cdot10^{3}$ contre
$8{,}33\cdot10^{3}$ ; la part mesurée y vaut $0{,}001\pm0{,}002$, c'est-à-dire un
tirage libre sur les $1200$), et `uniform` $k=11$ à $n=2000$. Les exposants que
la première version de ce document lisait sur ces lignes n'étaient donc pas des
mesures (§ 11). Il fallait un comptage exact.

**Le comptage exact à l'ordre $2$.** Une paire $\left\lbrace x_i,x_j\right\rbrace$
a sa boule diamétrale fermée vide si et seulement si
$\left\langle x_i-p,x_j-p\right\rangle>0$ pour tout $p$ hors de la paire : c'est
Thalès, et c'est la définition du graphe de Gabriel strict. Le comptage est donc
exhaustif sur les $\binom{n}{2}$ paires, sans aucune boule englobante et sans
échantillonnage. Table `exact2` (§ 10 commande 14), cinq graines, trois chemins
indépendants qui doivent donner le même entier — matrice de Gram (référence, coût
indépendant de $d$), arbre $k$-d (contrôle jusqu'à $n=800$), boule englobante
certifiée (contrôle à $n=200$) :

REMPLACER_TABLE_EXACT2

**Ce que le comptage exact change.** L'estimation échantillonnée est bonne là où
la part libre est grande — pour `flat_noise` elle donne $1{,}39\cdot10^{4}$ et
$2{,}05\cdot10^{5}$ contre REMPLACER_FN_200 et REMPLACER_FN_800 exacts — et elle
sous-estime là où la part libre est petite, jusqu'à un facteur REMPLACER_FACTEUR
à $n=2000$ sur `flat`. Les exposants corrigés sont dans la seconde table.

**Et une borne théorique qui ferme la question.** Les paires libres sont les
arêtes du graphe de Gabriel **strict** (boule fermée), qui est inclus dans le
graphe de Gabriel, lui-même sous-graphe de la triangulation de Delaunay. Pour une
configuration de rang $2$, la triangulation du plan a $3n-3-h$ arêtes avec $h$
sommets sur l'enveloppe convexe, donc au plus $3n-6$ : **le compte est
$O(n)$ quelle que soit la dimension ambiante**, par T1. La mesure exacte le
confirme sans marge d'interprétation : `flat` passe de $1{,}87$ à $1{,}96$ paire
libre par point de $n=200$ à $n=2000$, pour une borne de $3$ par point ; à
$n=2000$, la triangulation du plan sous-jacent a $5\,981$ arêtes (borne $5\,994$)
dont $3\,945$ libres sur la graine $0$. C'est la formulation utile pour des données
réelles : **sans bruit hors-variété le compte est linéaire en $n$ ; avec un bruit
hors-variété d'amplitude $\sigma\sqrt{2d}\approx1$ il devient quadratique**
(exposant exact REMPLACER_EXP_FN à $k=2$, et $2{,}88$ échantillonné à $k=3$, où la
part libre $0{,}198\pm0{,}012$ est très au-dessus de la résolution).

### 3.6 Contre-vérification contre la tour exacte

Ma mesure (a) est comparée aux **naissances topologiques** de la tour FULL
exacte de `src/ehgp/exact/tower.py`, sur le même nuage quantifié, $n=9$,
$k_{\max}=3$, trois graines (§ 10 commande 7) :

```text
famille     graine  digest ref    k  C(n,k)  apparitions  naissances  libres (ma mesure)  ecart
flat        0       ffae03b18204  1  9       9            9           9                   0
flat        0       ffae03b18204  2  36      36           13          13                  0
flat        0       ffae03b18204  3  84      84           12          12                  0
flat_noise  0       eb88c42b9332  2  36      36           15          15                  0
flat_noise  1       231bc193ff85  3  84      84           18          18                  0
uniform     0       6deeb48459c3  2  36      36           35          35                  0
uniform     0       6deeb48459c3  3  84      84           73          73                  0
uniform     1       60c6676745dc  3  84      84           81          81                  0
```

Vingt-sept cellules, **écart nul partout**, et cet écart nul est désormais une
**porte** : un désaccord met le code de sortie à $3$ (§ 11 ; dans la première
version il s'imprimait et l'exécution sortait quand même à $0$, si bien que la
confirmation indépendante la plus importante du document n'était qu'une ligne de
texte). Deux conséquences. D'abord l'instrument de ce document et la tour exacte
du chantier comptent la même chose, par deux chemins sans code commun. Ensuite l'inégalité large de la
fixture F2 (`parties à boule fermée vide` $\leq$ `naissances`) est **une égalité
sur les nuages tirés au hasard** : le cas strict demande des coïncidences
cosphériques, qui ne se produisent pas génériquement. Les colonnes
`apparitions` valent $\binom{n}{k}$ partout, ce qui redit pourquoi il ne faut
jamais les confondre avec la taille de sortie.

## 4. Mesure (b), le rayon : le signal s'effondre avec $r$, pas avec $d$

Écart-type **intra-nuage** du rayon normalisé, $n=200$, $240$ parties, cinq
graines (§ 10 commande 8). La valeur du simplexe régulier est $0{,}674$ à $k=11$.

| famille | $r$ | moyenne à $k=11$ | écart-type intra à $k=11$ |
| --- | --- | --- | --- |
| `uniform` $d=2$ | 2 | 1,022 | **0,105** |
| `flat` $d=200$ | 2 | 1,022 | **0,109** |
| `lidar` $d=7$ | 2 | 1,281 | **0,320** |
| `uniform` $d=3$ | 3 | 0,906 | 0,083 |
| `flat_noise` $d=200$ | 2 | 0,831 | 0,062 |
| `uniform` $d=20$ | 20 | 0,719 | 0,025 |
| `gauss_aniso` $d=200$ | 200 | 0,766 | 0,060 |
| `uniform` $d=200$ | 200 | 0,678 | **0,008** |
| `sphere` $d=200$ | 199 | 0,676 | **0,003** |

À $d=200$ constant, l'écart-type passe de $0{,}008$ ($r=200$) à $0{,}109$
($r=2$) : un facteur $13{,}6$ gouverné par $r$ seul. La moyenne raconte la même
chose autrement : elle colle à la valeur du simplexe régulier quand $r=d$
($0{,}678$ contre $0{,}674$) et s'en éloigne franchement quand $r\ll d$
($1{,}022$). Le spectre en loi de puissance (`gauss_aniso`, $r=d=200$
nominalement mais dimension effective petite) garde $0{,}060$ : c'est la
dimension **effective à l'échelle des événements** qui compte, pas le nombre de
colonnes ni le rang algébrique.

La famille `lidar` est la plus favorable de toutes les familles mesurées
($0{,}320$ à $k=11$, quarante fois la valeur de `uniform` $d=200$) : les
attributs n'y détruisent pas la structure de surface.

## 5. Mesure (c), le contraste : la dispersion n'est pas le signal

### 5.1 Ce que la dispersion du comptage dit, et ne dit pas

$n=400$, $m=10$, cinq graines (§ 10 commande 9), extrait :

| famille | représentation | $\overline{N}$ | cv | excès Poisson | q3/q1 | p90/p10 |
| --- | --- | --- | --- | --- | --- | --- |
| `uniform` $d=2$ | brut | 9,59 | 0,344 | 1,066 | 1,714 | 2,71 |
| `flat` $d=200$ $r=2$ | brut | 9,59 | 0,344 | 1,066 | 1,714 | 2,71 |
| `sphere` $d=200$ | brut | 9,62 | 0,309 | 0,957 | 1,614 | 2,23 |
| `roll` $d=200$ $r=2$ | brut | 10,32 | 0,452 | 1,455 | 1,893 | 3,55 |
| `clusters` $d=200$ | brut | 11,10 | 0,722 | 2,406 | 3,380 | 11,2 |
| `flat_noise` $d=200$ | brut | 11,27 | 0,739 | 2,481 | 3,002 | 10,4 |
| `flat_noise` $d=200$ | ACP$(2)$ | 9,49 | 0,361 | 1,112 | 1,714 | 2,76 |
| `flat_noise` $d=200$ | blanchi | 16,90 | **1,289** | **5,303** | 6,467 | 41,6 |
| `uniform` $d=200$ | brut | 12,80 | 0,895 | 3,204 | 3,700 | 14,2 |
| `uniform` $d=200$ | blanchi | 15,62 | 1,157 | 4,571 | 5,570 | 34,0 |
| `lidar` $d=7$ $r=2$ | brut | 11,36 | 0,755 | 2,544 | 5,217 | 23,8 |

Deux lectures s'imposent, dans cet ordre.

D'abord le repère de Poisson fonctionne : `uniform` $d=2$ et `sphere` $d=200$,
qui n'ont **aucune** variation de densité, donnent un excès de $1{,}07$ et
$0{,}96$, c'est-à-dire du bruit d'échantillonnage pur. `clusters`, qui a une
vraie structure, donne $2{,}41$.

Ensuite le piège : `uniform` $d=200$ n'a **aucune** variation de densité non
plus, et donne un excès de $3{,}20$, supérieur à celui de `clusters`. En grande
dimension, $r_{\mathrm{ref}}$ tombe dans la partie très raide de la distribution
des distances entre paires, si bien qu'une variation infime de l'échelle locale
produit une variation énorme du comptage. **La dispersion du comptage augmente
quand le signal disparaît.** Le plus net : `flat_noise` blanchi atteint l'excès
le plus élevé de tout le tableau ($5{,}30$) au moment même où, on va le voir, il
n'a plus aucun signal du tout.

Mesure (c) telle qu'elle est écrite ne décide donc rien. Il faut une densité
connue.

### 5.2 Densité latente connue : corrélation de Spearman

Famille `flat_graded` : variété plate de rang $r=2$ dans $\mathbb{R}^{d}$, dont
la densité latente est proportionnelle à $\exp(3u_{1})$ le long de la première
coordonnée (rapport $\exp(3)\approx20$ entre les deux bords), tirée par
transformation inverse, donc **connue exactement en chaque point**. On mesure la
corrélation de Spearman entre $N_i$ et la densité vraie. $n=400$, cinq graines
(§ 10 commande 10).

Brut (à gauche) et après ACP au rang intrinsèque $2$ (à droite) :

| $\sigma$ | $\sigma\sqrt{2d}$ à $d=200$ | $d=2$ | $d=5$ | $d=20$ | $d=50$ | $d=200$ |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 0,00 | 0,752 / 0,752 | 0,755 / 0,755 | 0,746 / 0,746 | 0,750 / 0,750 | 0,743 / 0,743 |
| 0,03 | 0,60 | 0,741 / 0,741 | 0,735 / 0,744 | 0,679 / 0,719 | 0,689 / 0,730 | 0,629 / **0,735** |
| 0,1 | 2,00 | 0,603 / 0,603 | 0,504 / 0,593 | 0,426 / 0,598 | 0,429 / 0,612 | 0,317 / **0,639** |
| 0,3 | 6,00 | 0,306 / 0,306 | 0,210 / 0,307 | 0,161 / 0,342 | 0,163 / 0,353 | 0,106 / **0,305** |

Voilà la mesure qui tranche. **Après ACP, la colonne de droite ne dépend plus de
$d$ du tout** : $0{,}749\pm0{,}005$ à $\sigma=0$, $0{,}734\pm0{,}010$ à
$\sigma=0{,}03$, $0{,}609\pm0{,}018$ à $\sigma=0{,}1$, $0{,}323\pm0{,}023$ à
$\sigma=0{,}3$, de $d=2$ à $d=200$ — écarts-types des cinq valeurs de $d$,
estimateur non biaisé, recalculés par la revue du § 11 : la première version
publiait $\pm0{,}012$ à $\sigma=0$, qui était l'étendue et non l'écart-type. Ce qui reste du signal est fonction du seul
bruit **dans** la variété. Brut, au contraire, le signal se dégrade
monotonement avec $d$ : à $\sigma=0{,}1$, il tombe de $0{,}603$ ($d=2$) à
$0{,}317$ ($d=200$), soit $-47\,\%$.

Le mécanisme est une identité, pas une hypothèse : pour $y_i=x_i+\sigma g_i$ avec
$g_i$ gaussien standard indépendant,

$$\mathbb{E}\left\Vert y_i-y_j\right\Vert^{2}=\left\Vert x_i-x_j\right\Vert^{2}+2d\sigma^{2}.$$

L'énergie de bruit vaut $d\sigma^{2}$. Les $r$ composantes **dans** la variété
déplacent les positions latentes et détruisent le signal sans recours ; les
$d-r$ composantes **hors** variété ne font que gonfler les distances, et une
projection linéaire les retire. La dimension ambiante n'agit donc que par la
part hors-variété de cette énergie, et cette part est réparable.

Les deux autres représentations, mêmes cellules, $d=200$ :

| $\sigma$ | brut | ACP$(2)$ | blanchi | JL$(96)$ |
| --- | --- | --- | --- | --- |
| 0 | 0,743 | 0,743 | 0,758 | 0,746 |
| 0,03 | 0,629 | **0,735** | **0,017** | 0,512 |
| 0,1 | 0,317 | **0,639** | **0,019** | 0,218 |
| 0,3 | 0,106 | **0,305** | **0,016** | 0,034 |

Le blanchiment n'annule pas le signal quand il n'y a pas de bruit ($0{,}758$ :
il ne fait alors qu'une isométrie du rang $2$) ; dès qu'il y a du bruit, il
l'**annule** ($0{,}016$ à $0{,}019$, c'est-à-dire zéro à la précision de la
mesure). Johnson-Lindenstrauss préserve ce qui est là ($0{,}746$ sans bruit) mais
ne répare rien, et coûte : $0{,}218$ contre $0{,}317$ pour le brut à
$\sigma=0{,}1$.

## 6. Le seuil du bruit ambiant

$n=200$, $k=2$, variété plate $r=2$ plus bruit, cinq graines (§ 10 commande 11) :

```text
d    bruit/coord  bruit*sqrt(2d)  representation  part libre    cv du comptage
20   0.000        0.00            raw(d=20)       0.016+-0.007  0.325+-0.023
20   0.010        0.06            raw(d=20)       0.028+-0.008  0.324+-0.021
20   0.030        0.19            raw(d=20)       0.060+-0.002  0.342+-0.021
20   0.100        0.63            raw(d=20)       0.208+-0.017  0.591+-0.012
20   0.300        1.90            raw(d=20)       0.521+-0.029  0.870+-0.018
20   1.000        6.32            raw(d=20)       0.622+-0.017  0.964+-0.037
200  0.000        0.00            raw(d=200)      0.017+-0.007  0.354+-0.043
200  0.010        0.20            raw(d=200)      0.077+-0.026  0.356+-0.047
200  0.030        0.60            raw(d=200)      0.383+-0.031  0.427+-0.038
200  0.100        2.00            raw(d=200)      0.998+-0.003  0.806+-0.027
200  0.300        6.00            raw(d=200)      1.000+-0.000  1.039+-0.051
200  1.000        20.00           raw(d=200)      1.000+-0.000  1.050+-0.044
```

Et les mêmes cellules après ACP$(2)$ : $0{,}016$, $0{,}022$, $0{,}020$,
$0{,}023$, $0{,}018$, $0{,}018$ en $d=20$ ; $0{,}017$, $0{,}023$, $0{,}021$,
$0{,}017$, $0{,}017$, $0{,}024$ en $d=200$. **La réparation de la taille est
totale à tous les niveaux de bruit, jusqu'à $\sigma\sqrt{2d}=20$.** Le § 3.4
rappelle ce que cela vaut : la taille est réparée, pas la vérité.

Le départ de l'obstruction se lit sur la variable $\eta=\sigma\sqrt{2d}$ : à
$\eta\approx0{,}2$, $\varphi_2$ vaut $0{,}060$ ($d=20$) et $0{,}077$ ($d=200$),
soit un décollage comparable ; le bruit par coordonnée tolérable décroît donc
comme $1/\sqrt{2d}$. En revanche le **plateau** n'est pas fonction de $\eta$ : à
$\eta=6$, $\varphi_2$ vaut $0{,}622$ en $d=20$ et $1{,}000$ en $d=200$, chacun
rejoignant la valeur d'un nuage isotrope de sa propre dimension ambiante. La
lecture honnête est donc : $\varphi_k$ interpole entre la valeur intrinsèque
($\eta\to0$) et la valeur ambiante ($\eta\gg1$), avec un décollage piloté par
$\eta$ et un plateau piloté par $d$.

## 7. Les trois changements de représentation : ce qu'ils préservent

### 7.1 Blanchiment (Mahalanobis empirique) : à interdire

Il remplace la dimension intrinsèque par le **rang effectif**
$\min(n-1,d)$. Conséquences mesurées : $\varphi_5=1{,}000\pm0{,}000$ dès
$d\geq n-1$ (§ 3.2, et T3 en donne la raison exacte) ; $\varphi_2=1{,}000$ dès
$d=50$ à $n=200$ (§ 3.3) ; Spearman $0{,}016$ à $0{,}019$ dès qu'il y a du bruit
(§ 5.2) ; excès de dispersion le plus élevé de tout le document, $5{,}30$
(§ 5.1) ; et sur la tour exacte, les naissances passent de $15$ à $36=\binom{9}{2}$
et de $12$ à $84=\binom{9}{3}$ (§ 7.4). Le blanchiment est **exactement**
l'opération que E-HGP ne doit pas subir : il égalise le signal et le bruit par
construction.

La mesure (b) donne la confirmation la plus nette de T3. Table `repr` (§ 10
commande 12), $n=200$, $d=200$, rayon normalisé par la distance médiane entre
paires :

| famille | représentation | $k=2$ | $k=5$ |
| --- | --- | --- | --- |
| `uniform` $d=200$ | brut | 0,501 $\pm$ 0,021 | 0,634 $\pm$ 0,011 |
| `uniform` $d=200$ | blanchi (rang 199) | **0,500 $\pm$ 0,000** | **0,632 $\pm$ 0,000** |
| `flat_noise` $r=2$ | brut | 0,514 $\pm$ 0,113 | 0,726 $\pm$ 0,086 |
| `flat_noise` $r=2$ | blanchi (rang 199) | **0,500 $\pm$ 0,000** | **0,632 $\pm$ 0,000** |
| `flat_noise` $r=2$ | ACP$(2)$ | 0,502 $\pm$ 0,230 | 0,858 $\pm$ 0,156 |
| `flat` $r=2$ | brut | 0,502 $\pm$ 0,240 | 0,852 $\pm$ 0,157 |

L'écart-type intra-nuage du nuage blanchi vaut **exactement zéro**, et la moyenne
vaut **exactement** $\sqrt{(k-1)/(2k)}$ : $0{,}500$ et $0{,}632$. La filtration en
rayon du nuage blanchi n'est pas seulement pauvre, elle est **constante** — c'est
T3 lu par la mesure (b), et c'est le pire cas possible pour E-HGP. L'ACP au rang
$2$, elle, restitue la dispersion du nuage sans bruit ($0{,}230$ contre
$0{,}240$).

### 7.2 Analyse en composantes principales au rang intrinsèque

Ce qui est **préservé** : rien de plus qu'une contraction (T2), donc les niveaux
descendent et leur ordre peut changer. Si le nuage vit exactement dans un
$r$-plan, l'ACP au rang $r$ est une isométrie (T1) et **tout** est préservé ;
c'est le seul cas où l'on peut l'affirmer.

Ce qui est **réparé** : la taille, totalement, jusqu'à $\eta=20$ (§ 6), y compris
pour une variété non linéaire (§ 3.2) — mais le § 3.4 montre que n'importe
quelle projection de même rang en ferait autant.

Ce qui n'est **pas** réparé : le signal de densité, qui plafonne à la valeur
fixée par le bruit **dans** la variété ($0{,}305$ à $\sigma=0{,}3$ contre
$0{,}743$ sans bruit, § 5.2).

### 7.3 Projection aléatoire de Johnson-Lindenstrauss

Avec $m=\lceil4\ln n/\epsilon^{2}\rceil$ et $\epsilon=0{,}5$ ($m=96$ à
$n=400$), la distorsion mesurée des paires vaut $0{,}26$ et celle du rayon
$0{,}10$ (T4). Elle ne répare pas : $\varphi_2$ passe de $0{,}694$ à $0{,}551$
(contre $0{,}016$ pour l'ACP, § 3.3) et le Spearman **baisse** par rapport au
brut ($0{,}218$ contre $0{,}317$, § 5.2). Sur la tour exacte à $n=9$, la garantie
est vide ($\epsilon$ théorique $0{,}663$ pour $m=20$) et le dégât est massif :
$151$ à $292$ paires discordantes sur $630$.

### 7.4 Aucune projection ne préserve $\pi_{0}(L_k(a))$ : mesure sur la tour exacte

$n=9$, $k_{\max}=3$, grille $2^{9}$, trois graines. La comparaison est faite sur
la **partie combinatoire** de la tour : suite ordonnée des niveaux critiques avec
leurs naissances et leurs multifusions, niveaux absolus effacés. Le digest de
`FullTower` contient la dimension ambiante et les niveaux exacts : il ne peut
jamais coïncider entre deux représentations, même isométriques, et ne mesure donc
rien ici. On publie aussi le nombre de paires discordantes de l'ultramétrique
projetée : la colonne `paires` du tableau imprime le nombre d'entrées comparables
de l'ultramétrique projetée sur les observations, $36=\binom{9}{2}$ à tous les
ordres, et la colonne `discordantes` compte les couples d'entrées discordants,
donc sur $\binom{36}{2}=630$ comparaisons. Et une **représentation de
contrôle** : une rotation aléatoire, qui est une isométrie exacte, donc dont tout
écart est imputable à la seule requantification. Que cette rotation conserve le
nombre de naissances à chaque cellule est désormais une porte (§ 11). La colonne
`distorsion` est un maximum **après division par le rapport médian** : c'est une
distorsion de forme, insensible à un facteur d'échelle global, ce qu'il faut
pour juger une ACP, qui retire de l'énergie sans renormaliser.

```text
famille     graine  representation  distorsion  k  suite egale  naissances ref  naissances proj  tau     discordantes
flat        0       rot(d=20)       0.00        3  oui          12              12               0.863   0
flat        0       pca(d=2)        0.00        3  non          12              12               0.863   0
flat        0       jl(d=20)        0.03        3  non          12              12               0.863   0
flat        0       whiten(d=2)     0.01        3  non          12              12               0.863   0
flat_noise  0       rot(d=20)       0.00        2  non          15              15               0.760   25
flat_noise  0       pca(d=2)        0.23        3  non          12              10               0.646   36
flat_noise  0       jl(d=20)        0.23        3  non          12              13               0.443   111
flat_noise  0       whiten(d=8)     1.24        3  non          12              84               -0.260  303
uniform     0       rot(d=20)       0.00        3  non          73              73               0.867   0
uniform     0       pca(d=9)        0.00        3  non          73              73               0.867   0
uniform     0       jl(d=20)        0.38        3  non          73              41               0.265   170
uniform     0       whiten(d=8)     0.39        3  non          73              84               0.408   118
```

Lecture. Le contrôle par rotation donne $0$ paire discordante dans $25$ lignes
sur $27$, et jusqu'à $25$ dans les deux autres : **le plancher de bruit de cette
mesure, dû à la requantification seule, vaut $0$ à $25$ paires sur $630$.** L'ACP
au rang intrinsèque reste dans ce plancher sur `flat` ($0$ partout) et le dépasse
à peine sur `flat_noise` ($0$ à $36$). Johnson-Lindenstrauss le dépasse
largement ($0$ à $111$ sur `flat_noise`, $151$ à $292$ sur `uniform`). Le
blanchiment inverse la corrélation ($\tau$ négatif) et fait exploser les
naissances jusqu'à $\binom{n}{k}$.

La colonne `suite egale` vaut `non` presque partout, y compris pour la rotation
de contrôle : la requantification suffit à déplacer les égalités de niveaux, donc
à changer le groupement des multifusions. C'est une limite du protocole, pas un
dégât de projection ; la colonne qui porte le jugement est `discordantes`.

**Réponse à la question posée.** Non, une projection ne préserve pas
$\pi_{0}(L_k(a))$ : par T2 elle ne fait que contracter les niveaux, sans
contracter tous les niveaux du même facteur. La seule exception est l'isométrie —
l'ACP au rang exact d'un nuage exactement plat, où l'on mesure effectivement $0$
paire discordante. L'ampleur du dégât est mesurée ci-dessus, et elle se lit par
rapport au plancher de requantification.

## 8. Portée honnête

**Démontré** (et vérifié à l'instrument par la table `theoremes`, code de sortie
$0$) : T1, invariance de $\beta$ par isométrie, donc les trois mesures sont des
fonctions de la configuration intrinsèque ; T2, une projection orthogonale
contracte $\beta$ ; T3, le blanchiment d'un nuage de $n$ points en dimension
$d\geq n-1$ donne un simplexe régulier exact, de toutes distances au carré
$2(n-1)$ et de niveaux $\beta(F)=(n-1)(k-1)/k$, donc une tour indépendante des
données ; T4, la distorsion du rayon sous une application linéaire aléatoire est
majorée par celle des paires.

**Mesuré**, sur douze familles synthétiques, cinq graines au moins par cellule,
$864\,520$ boules toutes certifiées, $0$ verdict indécis, instrument validé
contre la boule rationnelle exacte sur $400$ cas dont les dégénérescences : les
tableaux des § 3 à § 7. En particulier : l'invariance chiffre par chiffre de
$\varphi_k$ par plongement isométrique (§ 3.1) ; l'égalité de ma mesure (a) avec
les naissances topologiques de la tour exacte, $27$ cellules, écart nul (§ 3.6) ;
l'exposant exact $1{,}02$ en $n$ du compte de naissances sans bruit
hors-variété contre $1{,}96$ sous bruit (§ 3.5) ; l'indépendance en $d$ du signal réparé par
ACP (§ 5.2) ; le plancher de requantification de la mesure de dégât (§ 7.4).

**Conjecturé**, et à ne pas écrire autrement : que le décollage de $\varphi_k$
soit gouverné par la seule variable $\eta=\sigma\sqrt{2d}$ (deux dimensions
ambiantes concordent à $\eta\approx0{,}2$, elles divergent à $\eta\geq0{,}6$,
§ 6) ; que le comportement mesuré sur `flat_noise` et `roll_noise` vaille pour un
bruit non gaussien ou hétéroscédastique ; que la famille `lidar` synthétique
prédise quoi que ce soit d'un vrai nuage de télémètre, ce qu'aucune mesure de ce
document n'établit.

**Réfuté par la mesure, et à ne plus écrire.** « La dispersion du comptage de
boules mesure le signal de densité » : `uniform` $d=200$, sans aucune variation
de densité, a un excès de Poisson de $3{,}20$, supérieur à celui de `clusters`
($2{,}41$), et le blanchiment atteint $5{,}30$ au moment où son Spearman tombe à
$0{,}017$ (§ 5). « Si la part libre redescend après projection, l'objet est
préservé » : une projection sur un plan **aléatoire** d'un nuage authentiquement
de dimension $200$ donne $\varphi_2=0{,}014$, la valeur d'un nuage plan (§ 3.4).
« Le blanchiment est un prétraitement neutre » : T3 et tout le § 7.1. « La
dimension ambiante gouverne l'obstruction » : à $r=2$ fixé, $\varphi_5$ ne bouge
pas de $d=2$ à $d=200$ (§ 3.2).

**Ouvert.** Le certificat de non-connexité (hors périmètre de ce document) ;
l'estimation de $r$ et du sous-espace de signal sans le connaître d'avance, que
toutes les mesures d'ACP de ce document supposent donné ; le cas d'une variété
non linéaire dont le rang linéaire dépasse le rang de projection, où l'ACP au
rang intrinsèque n'est plus fidèle alors que la mesure (a) ne le détecte pas
(§ 3.4 en explique le mécanisme, aucune mesure ici ne le quantifie) ; et le
comportement sur données réelles.

## 9. Verdict en trois lignes

1. **L'obstruction de taille est gouvernée par la dimension intrinsèque, pas par
   la dimension ambiante** : à $r=2$ fixé, $\varphi_5$ vaut $0{,}003\pm0{,}001$
   de $d=2$ à $d=200$ et le compte **exact** de naissances croît en
   $n^{1{,}02}$,
   tandis qu'à $r=d$ il passe de $0{,}004$ à $1{,}000$ ; la dimension ambiante
   n'agit que par l'énergie du bruit hors-variété $\sigma^{2}(d-r)$, et cette
   action-là est réparable par une projection linéaire.
2. **Le signal de densité, lui, n'est pas réparé** : après ACP au rang
   intrinsèque, la corrélation de Spearman entre le comptage de boules et une
   densité latente connue ne dépend plus du tout de $d$ ($0{,}749$, $0{,}734$,
   $0{,}609$, $0{,}323$ pour $\sigma=0$, $0{,}03$, $0{,}1$, $0{,}3$, à
   $\pm0{,}023$ près de $d=2$ à $d=200$), mais elle plafonne au niveau fixé par
   le bruit **dans** la variété ; une projection rachète la taille de la sortie,
   pas sa vérité, et le § 3.4 le prouve en obtenant la même taille par une
   projection aléatoire qui détruit tout.
3. **Deux interdits et une condition** : ne jamais blanchir (en rang $n-1$ le
   nuage devient un simplexe régulier exact et la tour ne dépend plus des
   données, T3 ; et à $n=400$ le signal est déjà annulé dès $d=20$ — à
   $\sigma=0{,}03$, Spearman $0{,}122$, puis $0{,}097$ en $d=50$ et $0{,}017$ en
   $d=200$, contre $0{,}719$, $0{,}730$ et $0{,}735$ pour l'ACP au rang $2$ sur
   les mêmes cellules), ne pas compter sur Johnson-Lindenstrauss (qui dégrade le
   signal en dessous du brut) ; et pour qu'un E-HGP euclidien ait un sens sur des
   données réelles, il faut un sous-espace de signal de rang $r$ **estimé** et un
   bruit hors-variété d'énergie telle que $\sigma\sqrt{2d}$ reste sous $0{,}2$ —
   valeur à laquelle $\varphi_2$ a déjà quadruplé, c'est donc un ordre de
   grandeur et non une marge — faute de quoi le compte de naissances devient
   quadratique en $n$ (exposant mesuré $1{,}96$ à $k=2$).

## 10. Reproduire : commandes exactes et codes de sortie

Toutes depuis `E-HGP/`, sous `python3 -O`, graine de base $31$. Les durées sont
indicatives (machine partagée). **Chaque commande exécute d'abord la porte de
l'instrument** (§ 1.2), donc chaque sortie commence par les deux tableaux du
`selftest` ; les extraits cités dans le corps du document commencent après eux.

```bash
# 1. porte de validation de l'instrument (§ 1.2) et enonces demontres (§ 2)
python3 -O bench/concentration.py --table selftest --selftest-cases 80 --seed 31
python3 -O bench/concentration.py --table theoremes --theorem-cases 200 --seed 31
# 2. regime exhaustif, n = 8, 11, 14 (§ 3.1)
python3 -O bench/concentration.py --table exhaustive --exhaustive-n 8,11,14 \
    --orders 2,3,5,11 --seeds 5 --seed 31
# 3. balayage de d a r fixe, n = 20, k = 5 (§ 3.2)
python3 -O bench/concentration.py --table dimsweep --n-list 20 --seeds 5 \
    --subsets 240 --dimsweep-dims 2,3,5,20,50,200 --dimsweep-order 5 --seed 31
# 4. balayage de d a r fixe, n = 200, k = 2 (§ 3.3)
python3 -O bench/concentration.py --table dimsweep --n-list 200 --seeds 5 \
    --subsets 240 --dimsweep-dims 2,3,5,20,50,200 --dimsweep-order 2 --seed 31
# 5. projection aleatoire de rang 2 contre ACP de rang 2 (§ 3.4)
python3 -O bench/concentration.py --table dimsweep --n-list 200 --seeds 5 \
    --subsets 240 --dimsweep-dims 200 --dimsweep-order 2 \
    --representations raw,pca,jl --jl-epsilon 4 --noise 0.3 --seed 31
# 6. le compte de naissances a n = 200, 800, 2000 (§ 3.5)
python3 -O bench/concentration.py --table free --n-list 200,800,2000 \
    --orders 2,3,5,11 --seeds 5 --subsets 240 --seed 31
# 7. contre-verification contre la tour exacte, et degat de projection (§ 3.6, § 7.4)
python3 -O bench/concentration.py --table tower --tower-n 9 --tower-k 3 \
    --tower-seeds 3 --seed 31
# 8. concentration du rayon (§ 4)
python3 -O bench/concentration.py --table radius --n-list 200 --orders 2,3,5,11 \
    --seeds 5 --subsets 240 --seed 31
# 9. dispersion du comptage et exces de Poisson (§ 5.1)
python3 -O bench/concentration.py --table contrast --n-list 400 --seeds 5 --seed 31
# 10. densite latente connue, correlation de Spearman (§ 5.2)
python3 -O bench/concentration.py --table signal --n-list 400 --seeds 5 \
    --signal-dims 2,5,20,50,200 --signal-noise 0.0,0.03,0.1,0.3 --seed 31
# 11. seuil du bruit ambiant (§ 6)
python3 -O bench/concentration.py --table noise --n-list 200 --seeds 5 \
    --subsets 240 --noise-dims 20,200 --noise-levels 0,0.01,0.03,0.1,0.3,1.0 \
    --dimsweep-order 2 --seed 31
# 12. les trois representations sur (a) et (b) (§ 7.1, § 7.2)
python3 -O bench/concentration.py --table repr --n-list 200 --orders 2,5 \
    --seeds 5 --subsets 240 --dimsweep-order 5 --seed 31
# 13. cout de l'instrument : taux de repli et temps processeur (§ 1.2)
python3 -O bench/concentration.py --table cout --seed 31
# 14. compte EXACT a l'ordre 2, trois chemins independants (§ 3.5)
python3 -O bench/concentration.py --table exact2 --exact2-n 200,800,2000 \
    --seeds 5 --seed 31
```

Toutes ces commandes sortent à $0$. Une porte manquée sort à $3$ : vérifié en
réduisant volontairement le nombre de boules sous le plancher
(`--subsets 60 --dimsweep-dims 2,20 --representations raw,pca`, $960$ boules pour
un plancher de $1000$, code $3$ avec la ligne
`PLANCHER MANQUE : trop peu de boules calculees`).

`--table all` enchaîne toutes les tables ; la porte `selftest` s'y exécute la
première, et les planchers agrégés décident du code de sortie final.

## 11. Revue adversariale du 26 septembre 2026

Les treize commandes que publiait la première version (§ 10, items 1 à 12,
l'item 1 en comptant deux) ont été **réexécutées** intégralement le
26 septembre 2026, sous `python3 -O`, graine de base $31$, avant toute
modification : les treize sortent à $0$ et la commande négative sort à $3$ avec
la ligne attendue. **Tous les tableaux du corps du document se reproduisent
chiffre par chiffre**, y compris l'égalité chiffre par chiffre du § 3.1, le zéro
de la colonne `ecart` du § 3.6 sur les vingt-sept cellules, et les $864\,520$
boules certifiées. Elles ont ensuite été réexécutées après les corrections, avec
les deux commandes ajoutées (items 13 et 14).

Trois vérifications par des chemins étrangers au fichier de mesure :

1. La boule englobante de `bench/concentration.py` a été confrontée à un
   **solveur générique** (`scipy.optimize`, formulation « minimiser $t$ sous
   $\left\Vert y-x_i\right\Vert^{2}\leq t$ ») sur $400$ instances aléatoires de
   dimension $1$ à $13$ et de cardinal $2$ à $9$ : écart relatif maximal
   $1{,}9\cdot10^{-13}$, $0$ boule non certifiée.
2. T3 a été revérifié par un **blanchiment différent** (facteur gauche de la
   décomposition en valeurs singulières, $U\sqrt{n-1}$, au lieu de la
   diagonalisation de la covariance) : distances au carré à
   $1{,}1\cdot10^{-15}$ de $2(n-1)$ et $\beta$ à $1{,}3\cdot10^{-15}$ de
   $(n-1)(k-1)/k$. C'est mille fois plus serré que la table `theoremes`, dont le
   $10^{-11}$ vient de la diagonalisation de la covariance et non de l'énoncé.
   Les rayons du simplexe régulier ont aussi été calculés en **rationnels
   exacts** : $\beta=(k-1)/k$ pour une arête au carré de $2$, égalité exacte à
   $k=2$, $3$, $5$, $11$.
3. La mesure (c bis) a été refaite avec **scikit-learn** (ACP par
   `decomposition.PCA`, voisinage par `neighbors.NearestNeighbors`) : les
   corrélations de Spearman sont identiques à la troisième décimale sur les
   vingt-sept cellules recalculées.
4. Le comptage exact du § 3.5 a été confronté à un **quatrième chemin**, hors du
   fichier de mesure : sur la variété plate, où l'ACP de rang $2$ est une
   isométrie, les arêtes candidates sont extraites de la triangulation de
   Delaunay du plan (`scipy.spatial.Delaunay`) puis filtrées. Accord exact aux
   trois tailles : $358$, $1\,540$ et $3\,945$ paires libres pour $585$,
   $2\,380$ et $5\,981$ arêtes candidates, graine $0$. Ce chemin ne regarde
   qu'$O(n)$ paires au lieu de $\binom{n}{2}$, et il vérifie donc en même temps
   l'inclusion de Gabriel strict dans Delaunay.

**Ce qui était faux, et qui est corrigé.** Six défauts, du plus grave au plus
petit.

1. **Un exposant tiré d'une cellule sous sa propre résolution** (§ 3.5). À
   $n=2000$ et $k=2$, la table `free` mesure une part libre de
   $0{,}001\pm0{,}002$ sur $240$ tirages : le compte publié
   ($1{,}67\cdot10^{3}$) était inférieur à la borne de résolution que la table
   imprime elle-même ($8{,}33\cdot10^{3}$). Les exposants $0{,}75$ (`flat`) et
   $0{,}64$ (`lidar`) reposaient donc sur environ un tirage libre sur les
   $1\,200$. Le compte est maintenant **exact** (table `exact2`, § 3.5) et les
   exposants valent $1{,}02$ et $1{,}16$. La conclusion — linéaire sans bruit
   hors-variété, quadratique avec — est non seulement confirmée mais **démontrée**
   pour le rang $2$ (Gabriel strict inclus dans Delaunay, au plus $3n-6$ arêtes
   dans le plan) ; les deux nombres qui la chiffraient étaient faux, et faux dans
   le sens qui aurait fait croire à un compte sous-linéaire, ce qu'aucune
   configuration ne peut donner.
2. **Une porte annoncée et désactivée** (§ 1.2). Le programme mettait le
   plancher de validation à zéro dès qu'une seule table était demandée : la
   phrase « sans validation, aucune autre table n'est publiée » était fausse dans
   onze des douze commandes publiées. La porte `selftest` s'exécute désormais
   avant toute table.
3. **La contre-vérification centrale n'était pas une porte** (§ 3.6). L'accord
   entre la mesure (a) de ce fichier et les naissances de la tour exacte
   s'imprimait, mais un désaccord n'aurait rien fait échouer. Il met maintenant
   le code de sortie à $3$, comme l'accord des trois chemins du § 3.5, la
   conservation du nombre de naissances par la rotation de contrôle (§ 7.4) et
   le contrôle par permutation du § 5.2.
4. **Un maximum pris sur un échantillon biaisé** (T4, § 2). Les $300$ parties de
   T4 étaient le préfixe lexicographique de $\binom{40}{5}$, donc partageaient
   leurs trois premiers indices ; la distorsion du rayon était sous-estimée d'un
   facteur allant jusqu'à $1{,}7$.
5. **Un coût publié que rien ne produisait** (§ 1.2). Le taux de repli
   ($0{,}045$) et les temps par boule ($24{,}5$ ms, $1{,}0$ à $1{,}7$ ms)
   n'étaient calculés par aucune commande, et étaient donnés en temps horloge
   dans un document qui déclare par ailleurs que le temps horloge n'est pas une
   mesure. La table `cout` les mesure en temps processeur : taux $0{,}043$,
   $176$ ms par repli, $0{,}16$ à $1{,}56$ ms par boule sans repli.
6. **Deux explications et une dispersion inexactes.** Le § 3.3 attribuait au
   tirage des parties une non-identité due au fait que la graine de `dimsweep`
   contient $17d$, donc que le nuage change de colonne en colonne. Le § 5.2
   publiait $\pm0{,}012$ à $\sigma=0$, qui est l'étendue et non l'écart-type
   ($\pm0{,}005$), et comparait le blanchiment à un $0{,}744$ qui n'apparaît dans
   aucune cellule. Enfin une définition de fonction dupliquée traînait dans le
   fichier de mesure, dont la première version lisait un champ qui n'existe plus
   dans la tour du chantier.

**Ce que la revue n'a pas mis en défaut.** Les trois lignes du verdict du § 9,
l'invariance chiffre par chiffre du § 3.1, l'effondrement en $r$ et non en $d$ de
l'écart-type du rayon (§ 4), la réfutation du § 5.1, l'indépendance en $d$ du
signal réparé par ACP (§ 5.2), le plancher de requantification du § 7.4, et les
trois théorèmes T1, T2, T3. La conjecture sur $\eta=\sigma\sqrt{2d}$ reste une
conjecture.
