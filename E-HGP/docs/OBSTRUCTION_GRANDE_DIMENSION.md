# L'obstruction de la grande dimension, mesurée

> [!IMPORTANT]
> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.

Ce document établit **pourquoi la tour FULL de MorseHGP3D ne peut pas être
transportée en grande dimension**, et pourquoi il faut changer d'objet et non
d'algorithme. Les deux obstructions ci-dessous sont mesurées, chacune par
deux voies indépendantes.

## 1. Obstruction de taille : la sortie explose

### 1.1 Le bon critère est la boule fermée, et la bonne quantité est un compte

Deux pièges, tous deux relevés par un audit indépendant et gravés ici en
fixtures permanentes.

**Piège 1 : apparition de sommet contre naissance de composante.** Le nombre
de sommets de $\Gamma_k$ vaut $\binom{n}{k}$ en **toute** dimension : ce
n'est pas la taille de la sortie. La taille de la sortie est le nombre de
composantes de $\pi_0$ **créées**, c'est-à-dire les composantes dont aucun
sommet n'était actif au niveau précédent. Un sommet peut naître déjà relié à
un voisin par une coface de même niveau.

**Piège 2 : intérieur strict contre boule fermée.** Une partie $F$ de
cardinal $k$ est un sommet isolé de $\Gamma_k(\beta(F))$ — donc une feuille
de l'arbre de fusion, née au niveau $\beta(F)$ — si et seulement si la boule
**fermée** de sa boule englobante minimale ne contient aucune autre
observation. Le critère par l'intérieur strict **surcompte** et ne minore
donc pas la taille de sortie.

> **Fixture permanente F1.** $A=(0,0)$, $B=(2,0)$, $C=(1,1)$ dans le plan,
> ordre $k=2$. La boule englobante de $\left\lbrace A,B\right\rbrace$ a pour
> centre $(1,0)$ et rayon au carré $1$ ; $C$ est à distance au carré $1$ de ce
> centre, donc **sur** la sphère. L'intérieur strict est vide, la boule fermée
> ne l'est pas : il y a **deux** naissances topologiques et non trois. Vérifié
> par `bench/births_vs_dimension.py` (contrôle de fixture au démarrage) et par
> `src/ehgp/exact/tower.py` (3 apparitions de sommets, 2 naissances).

> **Fixture permanente F2.** Les quatre sommets du carré $(\pm1,\pm1)$,
> ordre $k=3$. Les quatre triplets ont la même boule englobante : le compte
> de parties à boule fermée vide est alors **strictement inférieur** au
> nombre de naissances. L'inégalité correcte est donc
> $\#\left\lbrace F:\text{boule fermée vide}\right\rbrace\leq\#\text{naissances}$,
> et elle peut être stricte. C'est bien le sens utile : le compte de parties à
> boule vide **minore** la taille de sortie.

### 1.2 La mesure, prise sur l'objet

`bench/births_vs_dimension.py --n 8 --k-max 4 --dims 2,3,5,10,20,50,100 --seeds 5,17,31`
compte les naissances topologiques de la tour exacte, moyenne et intervalle
sur trois graines, $n=8$ :

| $d$ | $k=1$ (sur 8) | $k=2$ (sur 28) | $k=3$ (sur 56) | $k=4$ (sur 70) |
| --- | --- | --- | --- | --- |
| 2 | 8,0 | 10,7 (9–12) | 8,7 (8–9) | 8,3 (8–9) |
| 3 | 8,0 | 14,0 (13–15) | 15,7 (15–17) | 15,3 (14–16) |
| 5 | 8,0 | 16,7 (14–19) | 19,7 (14–26) | 16,0 (11–21) |
| 10 | 8,0 | 21,7 (19–25) | 31,0 (21–36) | 32,0 (25–40) |
| 20 | 8,0 | 26,7 (26–27) | 45,3 (40–50) | 47,0 (40–52) |
| 50 | 8,0 | 28,0 | 55,3 (54–56) | 65,0 (61–70) |
| 100 | 8,0 | **28,0 = $\binom{8}{2}$** | **56,0 = $\binom{8}{3}$** | **70,0 = $\binom{8}{4}$** |

La taille de sortie passe donc de $O(n)$ en dimension 2 à **exactement
$\binom{n}{k}$** en dimension 100. Mesure auxiliaire cohérente
(`bench/empty_ball_fraction.py`, $n=11$, graine 5) : la part des parties à
boule fermée vide passe de 14/55 ($d=2$, $k=2$) à 55/55 ($d\geq50$), et de
11/462 ($d=2$, $k=5$) à 462/462 ($d\geq50$).

Mesure indépendante par une troisième voie (prédicat entier pur pour $k=2$,
nuages uniformes) : à $n=1000$, la fraction de paires de Gabriel vaut
$0{,}511$ en $d=20$, $0{,}992$ en $d=50$ et **exactement $1{,}000$ en
$d=100$** — 499 500 paires sur 499 500, le graphe de Gabriel est complet.

### 1.3 L'énoncé asymptotique, démontré

**Théorème (démontré par l'audit, sans hypothèse de loi).** Si toutes les
distances au carré entre observations vérifient
$\left\lvert \mathrm{dist}^2/D-1\right\rvert\leq\epsilon$ pour un $D>0$,
alors pour toute partie $F$ de cardinal $k\geq2$ et toute observation
$x_j\notin F$,

$$\left\Vert x_j-c_F\right\Vert^2-\beta(F)\geq\frac{D}{2}\left(\frac{2}{k}-6\epsilon\right).$$

En particulier $\epsilon<1/(3k)$ suffit pour que **toutes** les parties de
cardinal $k$ aient une boule fermée vide. Comme la concentration des
distances donne $\epsilon\to0$ quand $d\to\infty$ à $n$ et $k$ fixés, la
probabilité que toutes les parties de cardinal $k$ soient des naissances tend
vers $1$.

**Et il faut dire que cette preuve est loin d'être optimale** : la condition
suffisante $\epsilon<1/(3k)$ n'est atteinte qu'à $d$ de l'ordre de $10^3$
($k=2$) à $10^4$ ($k=10$) pour $n=11$, alors que la fraction mesurée vaut
déjà $1{,}000$ dès $d=50$. La preuve établit la limite, pas le seuil.

### 1.4 La portée exacte : un compte, pas une fraction

Trois précautions, sans lesquelles l'obstruction serait surévaluée.

1. **À $d$ fixé, la fraction décroît avec $n$.** Mesuré ($k=2$, prédicat
   entier exact) : en $d=10$ la fraction passe de $0{,}776$ ($n=11$) à
   $0{,}197$ ($n=352$) ; en $d=20$ de $0{,}988$ à $0{,}652$. L'obstruction
   doit donc être écrite comme un **compte** : à $d=100$, $n=1000$, $k=2$, le
   compte est $499\,500$ naissances, et c'est cela qui est rédhibitoire.
2. **L'énoncé « tend vers $\binom{n}{k}$ » est à $n$ et $k$ fixés quand
   $d\to\infty$.** Il ne vaut **pas** uniformément au point cible
   $n=1000$, $K=10$ : à $k=10$ la fraction tombe à $0/120$ tirages dès
   $n=176$, aussi bien en $d=50$ qu'en $d=100$. Les comptes estimés restent
   cependant astronomiques aux ordres intermédiaires : à $k=5$,
   $n=704$, la fraction mesurée vaut $0{,}058$ en $d=50$ et $0{,}655$ en
   $d=100$, soit environ $8{,}2\cdot10^{10}$ et $9{,}3\cdot10^{11}$
   naissances.
3. **C'est la dimension INTRINSÈQUE qui gouverne, pas l'ambiante**, et la
   dimension ambiante n'agit que par l'énergie du bruit hors-variété
   $\sigma^2(d-r)$. Mesuré sur douze familles synthétiques, cinq graines au
   moins, $864\,520$ boules toutes certifiées
   ([`MESURES_CONCENTRATION_20260925.md`](MESURES_CONCENTRATION_20260925.md)) :
   à rang intrinsèque $r=2$ fixé, la part libre $\varphi_5$ vaut
   $0{,}003\pm0{,}001$ de $d=2$ à $d=200$ et le compte de naissances reste
   quasi linéaire en $n$ (exposant $\approx0{,}75$), tandis qu'à $r=d$ elle
   passe de $0{,}004$ à $1{,}000$. Un plongement isométrique exact laisse la
   mesure inchangée **chiffre par chiffre**.
4. **Mais le bruit ambiant ramène l'obstruction**, et c'est le régime des
   données réelles : sur une variété de dimension 2 dans $\mathbb{R}^{200}$
   avec bruit gaussien ambiant, l'exposant du compte de naissances en $n$
   passe de $0{,}75$ à $\mathbf{1{,}96}$, soit un régime quadratique
   ($1{,}39\cdot10^{4}$ naissances à $n=200$, $1{,}28\cdot10^{6}$ à
   $n=2000$).
5. **Deux interdits mesurés, et un piège.** Le blanchiment est à proscrire :
   en rang $d\geq n-1$ il transforme le nuage en **simplexe régulier exact**,
   toutes distances au carré égales à $2(n-1)$ et niveaux
   $\beta(F)=(n-1)(k-1)/k$, donc une tour qui **ne dépend plus des
   données** ; et dès $d/n\sim1/4$ le signal est annulé. Johnson-Lindenstrauss
   dégrade le signal en dessous du brut. Le piège : **la part libre qui
   redescend après projection ne prouve rien** — une projection sur un plan
   *aléatoire* d'un nuage authentiquement de dimension 200 donne
   $\varphi_2=0{,}014$, la valeur d'un nuage plan. La taille de sortie se
   rachète par une projection, la vérité de l'objet non.

**Pont avec la littérature.** Les feuilles de l'arbre de fusion d'ordre $k$
s'injectent dans les sommets de la mosaïque de Delaunay d'ordre $k$ : si la
boule fermée de $F$ est vide, son centre a pour $k$ plus proches exactement
$F$, donc la cellule de Voronoï d'ordre $k$ de $F$ est non vide.
L'obstruction mesurée est donc la face $H_0$ d'un fait connu : la Delaunay
d'ordre $k$ dans $\mathbb{R}^d$ a une taille
$O\!\left(k^{\lceil(d+1)/2\rceil}n^{\lfloor(d+1)/2\rfloor}\right)$ avec une
constante doublement exponentielle en $d$ (Corbet–Kerber–Lesnick–Osang pour
la bifiltration de multicouverture, Edelsbrunner–Osang pour le pavage
rhomboïdal).

## 2. Obstruction de signal : la filtration se vide

La seconde obstruction est plus grave, parce qu'elle survivrait à un
algorithme parfait.

**Mesure.** Rayon de boule englobante d'une partie de cardinal $K$ tirée au
hasard, normalisé par la distance typique entre paires, en $d=200$ :

| $K$ | moyenne $\pm$ écart-type | valeur du simplexe régulier $\sqrt{(K-1)/(2K)}$ |
| --- | --- | --- |
| 2 | $0{,}499\pm0{,}022$ | $0{,}500$ |
| 3 | $0{,}580\pm0{,}018$ | $0{,}577$ |
| 11 | $0{,}682\pm0{,}012$ | $0{,}674$ |

Toutes les parties de même cardinal ont donc **le même niveau**, à un
écart-type qui s'effondre quand $d$ croît. La filtration en rayon dégénère
en un escalier déterministe piloté par $K$ : elle **ne porte plus de signal
de densité**. C'est la concentration des distances appliquée à l'objet
lui-même, et non à son calcul.

**Conséquence de conception.** Même si l'on savait énumérer la tour, elle
serait vide d'information en métrique euclidienne ambiante et en grande
dimension. Toute version utile de E-HGP doit donc changer la **mesure de
proximité**, pas seulement l'algorithme.

## 3. Ce que la régularisation du niveau ne répare pas

Il serait tentant d'espérer que le lissage entropique fusionne les
$\binom{n}{k}$ minima en quelques modes. **Mesuré : non.**

`bench/scale_space.py --n 11 --k 3 --dims 2,3,20 --seed 31` donne le nombre
de minima distincts trouvés par descente quand $\varepsilon$ croît de
$10^{-4}$ à $1$ fois la variance :

| $d$ | catalogue exact | $\varepsilon/\mathrm{var}=10^{-4}$ | $10^{-2}$ | $10^{-1}$ | $1$ |
| --- | --- | --- | --- | --- | --- |
| 2 | 13 | 24 | 20 | 19 | 7 |
| 3 | 23 | 23 | 21 | 17 | 13 |
| 20 | 156 | 53 | 52 | 52 | 50 |

En $d=2$ le lissage fusionne (24 vers 7). En $d=20$ il ne fusionne
pratiquement rien (53 vers 50) : les minima sont séparés par des barrières
grandes devant $\varepsilon$. Le niveau minimal trouvé reste, lui, à
distance au plus $\varepsilon$ du niveau exact, conformément au théorème A.

**Donc : il faut régulariser la classe de fonctions, pas l'axe des
niveaux.** C'est ce que fait la couche statistique
([`MOTEUR_ET_COUTS.md`](MOTEUR_ET_COUTS.md) § 5), et c'est là que
l'estimateur spectral de log-densité entre en jeu — non comme accélérateur,
mais comme **changement d'objet assumé**.

## 4. Ce qui survit, exactement

| pièce | statut | coût |
| --- | --- | --- |
| tour d'ordre $k=1$ (liaison simple, arbre couvrant minimal) | **exacte et complète, en toute dimension** | $O(n^2d)$ |
| niveaux d'entrée $a_k(x_i)$ | exacts | $O(n^2d)$ |
| certification d'un chemin polygonal dans $L_k(a)$ | **exacte** | $O(n^2)$ candidats par segment |
| une sphère critique donnée | exacte, certifiée individuellement | $O(nd)$ par pas |
| projection de la tour sur les observations | majorant certifié, égalité **mesurée** et non démontrée | cubique |
| tour FULL complète, $k\geq2$ | **hors d'atteinte** (§ 1) | $\Theta\!\left(\binom{n}{k}\right)$ |
| signal de densité en métrique ambiante | **inexistant** (§ 2) | sans objet |

## 5. Portée honnête de l'obstruction

Ce qui est **démontré** : le critère exact de naissance (boule fermée vide,
§ 1.1) ; l'inégalité `parties à boule vide` $\leq$ `naissances` ; l'énoncé
asymptotique du § 1.3 avec ses constantes explicites ; l'injection des
feuilles dans les sommets de la Delaunay d'ordre $k$.

Ce qui est **mesuré**, sur des lois synthétiques (uniforme dans le cube,
gaussienne, sphère, deux amas, variété de dimension intrinsèque 2 plongée),
par **trois** implémentations indépendantes (la tour exacte du chantier, un
juge à arithmétique autre, un prédicat entier pur pour $k=2$) : les tableaux
des § 1.2, § 2 et § 3.

Ce qui est **explicitement faux** et ne doit pas être écrit : « le nombre de
naissances vaut $\binom{n}{k}$ en grande dimension » sans préciser « à $n$ et
$k$ fixés » ; « la fraction croît avec $n$ » (elle décroît) ; « l'intérieur
strict suffit comme critère de naissance » (fixture F1).

Ce qui reste **ouvert** : le seuil réel en $d$ (la preuve du § 1.3 donne
$10^3$ à $10^4$, la mesure donne 50) ; l'estimation du rang de signal $r$ et
du sous-espace sans les connaître d'avance, que toutes les mesures d'analyse
en composantes principales supposent donnés ; et le comportement sur données
réelles, qu'aucune mesure de ce chantier n'établit.

**Réfuté par la mesure, et à ne plus écrire** : « la dispersion du comptage de
boules mesure le signal de densité » (un nuage uniforme en $d=200$, sans
aucune variation de densité, a un excès de Poisson supérieur à celui d'un
nuage à huit amas) ; « si la part libre redescend après projection, l'objet
est préservé » ; « le blanchiment est un prétraitement neutre » ; « la
dimension ambiante gouverne l'obstruction ».

**La condition d'utilité, énoncée une fois pour toutes.** Pour qu'un E-HGP
euclidien ait un sens sur des données réelles, il faut un sous-espace de
signal de rang $r$ **estimé** et une énergie de bruit hors-variété sous le
seuil du § 6 de
[`MESURES_CONCENTRATION_20260925.md`](MESURES_CONCENTRATION_20260925.md). Une
projection rachète la taille de la sortie, jamais sa vérité : après analyse en
composantes principales au rang intrinsèque, la corrélation de rang entre le
comptage de boules et une densité latente connue ne dépend plus de $d$, mais
elle plafonne au niveau fixé par le bruit **dans** la variété ($0{,}749$ à
$\sigma=0$, $0{,}323$ à $\sigma=0{,}3$).

## 6. Conséquence pour le chantier

La tour FULL de MorseHGP3D **ne se transporte pas** en grande dimension : ni
par un meilleur algorithme (la sortie explose), ni par un meilleur lissage de
niveau (§ 3), ni en métrique euclidienne ambiante (§ 2). E-HGP calcule donc
trois objets distincts et nommés distinctement
([`OBJET_ET_DIMENSION.md`](OBJET_ET_DIMENSION.md) § 6) : la tour FULL exacte
comme **oracle borné**, sa **projection certifiée** sur les observations, et
la **tour régularisée** d'un modèle de densité. Aucun des trois ne revendique
le contrat public de la ligne enregistrée.
