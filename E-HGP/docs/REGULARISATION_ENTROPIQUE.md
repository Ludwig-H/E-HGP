# Régularisation entropique de la tour HGP

> [!IMPORTANT]
> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.
> Aucun énoncé de ce document ne promeut un statut public. Les théorèmes
> marqués **démontré ici** sont démontrés dans ce document ; les faits
> marqués **mesuré** renvoient à une commande reproductible ; ce qui est
> conjectural est dit conjectural.

Ce document établit la chaîne mathématique qui relie la tour HGP exacte à
une famille régularisée, et explique pourquoi cette famille est le point
d'entrée d'un calcul en grande dimension. Il se lit après
[l'objet et son audit de dimension](OBJET_ET_DIMENSION.md) et avant
[l'obstruction mesurée](OBSTRUCTION_GRANDE_DIMENSION.md).

## 1. La $k$-ième distance est un multiplicateur de masse

Pour $y\in\mathbb{R}^{d}$ et $X=\left\lbrace x_1,\ldots,x_n\right\rbrace$, on
note $e_i(y)=\left\Vert y-x_i\right\Vert^2$ les énergies et
$a_1(y)\leq\cdots\leq a_n(y)$ leur réordonnement croissant. La définition
normative de la tour utilise $a_k$ et les régions
$L_k(a)=\left\lbrace y: a_k(y)\leq a\right\rbrace$.

**Fait 1 (programme de masse, démontré ici).** Pour tout entier
$1\leq k\leq n$,

$$\sum_{i=1}^{k}a_i(y)=\min\left\lbrace \sum_{i=1}^{n}w_ie_i(y)\;:\;0\leq w_i\leq1,\;\sum_{i=1}^{n}w_i=k\right\rbrace .$$

*Preuve.* Le domaine est le polytope de capacité
$\left\lbrace w\in[0,1]^n:\sum_iw_i=k\right\rbrace$, dont les sommets sont
exactement les vecteurs indicatrices de parties de cardinal $k$ ; le minimum
d'une forme linéaire y est atteint en un sommet, et parmi les sommets celui
qui minimise $\sum_iw_ie_i$ choisit les $k$ plus petites énergies. $\square$

La valeur $\frac{1}{k}\sum_{i\leq k}a_i(y)$ est le carré de la
distance-à-la-mesure de Chazal–Cohen-Steiner–Mérigot ; la tour HGP utilise
non pas cette moyenne mais **le multiplicateur de la contrainte de masse**.

**Fait 2 bis (la DTM n'est pas $D_k$, démontré ailleurs, à graver).** On a
seulement l'encadrement multiplicatif
$d_{\mu,k/n}^2(y)\leq a_k(y)\leq 2\,d_{\mu,2k/n}^2(y)$, et ces inégalités
sont serrées. Les deux tours ne sont donc entrelacées que
multiplicativement, jamais égales : **tout l'outillage DTM — distance
puissance, Delaunay pondéré, $k$-distance témoignée — ne donne pas la tour
HGP**. C'est une piste fermée, pas un raccourci.

**Fait 2 (multiplicateur, démontré ici).** Soit
$V(\kappa)=\min\left\lbrace \sum_iw_ie_i:0\leq w_i\leq1,\;\sum_iw_i=\kappa\right\rbrace$
pour $\kappa\in[0,n]$ réel. Alors $V$ est convexe, affine par morceaux, et
pour $\kappa\in(k-1,k)$ sa dérivée vaut $a_k(y)$ ; le sous-différentiel en
$\kappa=k$ est $[a_k(y),a_{k+1}(y)]$.

*Preuve.* En relâchant la contrainte de masse par un multiplicateur $\mu$, le
problème se sépare : $\min_{0\leq w_i\leq1}w_i(e_i-\mu)$ vaut $e_i-\mu$ si
$e_i<\mu$ et $0$ sinon, donc $w_i=1$ pour $e_i<\mu$, $w_i=0$ pour $e_i>\mu$,
et $w_i$ libre dans $[0,1]$ pour $e_i=\mu$. La masse $\kappa$ est atteinte
pour $\mu\in[a_k,a_{k+1}]$ dès que $\kappa\in(k-1,k]$. $\square$

Autrement dit : **$a_k$ est le niveau de Fermi d'un système de $n$ états
d'énergies $e_i$ rempli par $k$ fermions**, et $L_k(a)$ est la région où ce
niveau ne dépasse pas $a$. Toute la suite exploite cette lecture.

**Antécédents, à citer et à ne pas réinventer.** L'occupation
$w_i=\sigma\!\left((\mu-e_i)/\varepsilon\right)$ avec $\mu$ fixé par la
masse est exactement la couche LML (*Limited Multi-Label projection*) de
Amos, Koltun et Kolter (2019) ; le top-$k$ doux et le tri doux par transport
entropique sont dus à Cuturi, Teboul et Vert (2019) et à Xie et al. (2020).
La nouveauté de ce chantier n'est donc **pas** le top-$k$ doux : c'est son
branchement sur la tour HGP (la criticité du § 4, l'encadrement du § 3 et la
descente exacte du § 5).

## 2. L'entropie choisit la forme du noyau

Régularisons le programme de masse par une entropie séparable : pour $\phi$
strictement convexe sur $[0,1]$ et $\varepsilon>0$,

$$\min\left\lbrace \sum_{i=1}^{n}w_ie_i+\varepsilon\sum_{i=1}^{n}\phi(w_i)\;:\;0\leq w_i\leq1,\;\sum_{i=1}^{n}w_i=k\right\rbrace .$$

**Fait 3 (forme de l'optimum, démontré ici).** L'optimum est unique et vaut

$$w_i=g\!\left(\frac{\mu-e_i}{\varepsilon}\right),\qquad g=\left(\phi'\right)^{-1}\ \text{tronquée à }[0,1],$$

où $\mu$ est l'unique réel tel que
$\sum_{i}g\!\left((\mu-e_i)/\varepsilon\right)=k$, dès que $0<k<n$.

*Preuve.* Le Lagrangien de la contrainte de masse est séparable ; la
condition de stationnarité sur $[0,1]$ s'écrit
$e_i+\varepsilon\phi'(w_i)-\mu\in-\partial\iota_{[0,1]}(w_i)$, soit
$w_i=\left(\phi'\right)^{-1}\!\left((\mu-e_i)/\varepsilon\right)$ à
l'intérieur, $w_i=0$ si $(\mu-e_i)/\varepsilon\leq\phi'(0^+)$ et $w_i=1$ si
$(\mu-e_i)/\varepsilon\geq\phi'(1^-)$ : c'est exactement la troncature.
L'unicité vient de la stricte convexité, et l'existence de $\mu$ de ce que
$\mu\mapsto\sum_ig((\mu-e_i)/\varepsilon)$ est continue, croissante, de
limites $0$ et $n$. $\square$

La conséquence est la correspondance qui donne son nom au chantier : le choix
de l'entropie, donc de la $f$-divergence associée, **fixe la forme du noyau**
$g$, par transformée de Legendre–Fenchel. Trois cas, tous vérifiés
numériquement contre un solveur générique (`tests/test_familles_noyaux.py`) :

| entropie $\phi(w)$ | $\phi'(w)$ | noyau $g(u)$ | support |
| --- | --- | --- | --- |
| $w\log w+(1-w)\log(1-w)$ (Fermi–Dirac) | $\log\frac{w}{1-w}$ | $\frac{1}{1+e^{-u}}$ (logistique) | infini, $C^{\infty}$ |
| $w^2/2$ (khi-deux) | $w$ | $\min(\max(u,0),1)$ (**rampe**) | compact, $C^0$ |
| $\frac{w^{\alpha}-w}{\alpha-1}$ (Tsallis, $\alpha>1$) | $\frac{\alpha w^{\alpha-1}-1}{\alpha-1}$ | puissance tronquée | compact |

Le comptage doux associé,

$$C_{\varepsilon}(y,a)=\sum_{i=1}^{n}g\!\left(\frac{a-e_i(y)}{\varepsilon}\right),$$

est **exactement une estimation à noyau** de la mesure empirique : $n$ fois
un estimateur de densité de noyau $g$ et de fenêtre $\varepsilon$, écrit en
distance au carré. Le comptage dur $\#\left\lbrace i:e_i\leq a\right\rbrace$
est le cas dégénéré $g=\mathbf{1}_{u\geq0}$. La tour HGP est donc la tour de
niveau d'un estimateur à noyau **indicatrice de boule**, et la régularisation
entropique est le changement de noyau qui la rend lisse.

Le choix du noyau rampe n'est pas cosmétique : il est **linéaire par
morceaux à ruptures rationnelles**, donc tout le comptage doux et toutes ses
décisions restent dans $\mathbb{Q}$. La doctrine d'exactitude du dépôt
survit à la régularisation, ce qui n'est pas le cas du noyau logistique.

### Lien avec la famille $f_\rho$ de Bach

Le billet de Francis Bach sur l'estimation spectrale de la log-densité part
de la famille de khi-deux pondérés
$f_{\rho}(t)=\frac{1}{2}\frac{(t-1)^2}{\rho t+1-\rho}$, dont la
représentation variationnelle est **quadratique**, donc résoluble en forme
close, et de l'identité intégrale qui décompose la divergence de
Kullback–Leibler en une famille de tels khi-deux. Le lien avec ce document
est double, et il faut le dire exactement :

1. **acquis** : $\rho=0$ donne la divergence du khi-deux, dont l'entropie
   associée est $w^2/2$, donc le noyau rampe. La forme close de Bach et la
   forme close du niveau de Fermi rampe (§ 4) sont deux faces de la même
   quadraticité ;
2. **conjectural** : que le noyau logistique (entropie de Shannon) soit
   l'intégrale en $\rho$ des noyaux rampe pondérés, comme la divergence de
   Kullback–Leibler est l'intégrale des $f_{\rho}$, est une identité
   plausible mais **non démontrée ici**. Elle est marquée comme question
   ouverte, pas comme résultat.

Ce que la famille $f_\rho$ apporte en propre est ailleurs, et c'est décisif
pour la grande dimension : elle donne un estimateur **spectral** du
log-rapport de densités, de coût $O(m^2n+m^3)$ pour $m$ descripteurs, qui ne
souffre pas de l'explosion de variance du terme $\log\sum e^{v}$. Cette
couche est traitée dans [`MOTEUR_ET_COUTS.md`](MOTEUR_ET_COUTS.md) § 5.

## 3. Encadrement exact du noyau rampe

Posons, pour $\varepsilon>0$,

$$L_k^{\varepsilon}(a)=\left\lbrace y\;:\;\sum_{i=1}^{n}\min\!\left(\max\!\left(\frac{a-e_i(y)}{\varepsilon},0\right),1\right)\geq k\right\rbrace .$$

**Théorème A (encadrement, démontré ici).** Pour tous $\varepsilon>0$,
$1\leq k\leq n$ et $a\in\mathbb{R}$,

$$L_k(a-\varepsilon)\ \subseteq\ L_k^{\varepsilon}(a)\ \subseteq\ L_k(a).$$

*Preuve.* Inclusion de droite : chaque terme est majoré par $1$ et n'est
non nul que si $e_i(y)<a$ ; si la somme atteint $k$, au moins $k$ énergies
vérifient $e_i(y)<a$, donc $a_k(y)<a$ et $y\in L_k(a)$. Inclusion de
gauche : si $a_k(y)\leq a-\varepsilon$, au moins $k$ énergies vérifient
$e_i(y)\leq a-\varepsilon$, donc $(a-e_i)/\varepsilon\geq1$ et chacun de ces
termes vaut exactement $1$ ; la somme atteint $k$. $\square$

Trois remarques, toutes vérifiées en rationnels exacts :

* le décalage est $\varepsilon$, **sans facteur $\log n$ et sans hypothèse
  sur $d$ ni sur $X$** ; la variante logistique exige, elle, un seuil
  $k-\tfrac12$ et un décalage $\varepsilon\log(2n)$ (§ 6) ;
* $\varepsilon\mapsto L_k^{\varepsilon}(a)$ est décroissante (à $a$ fixé,
  $\min(\max((a-e)/\varepsilon,0),1)$ décroît quand $\varepsilon$ croît), et
  $\bigcup_{\varepsilon>0}L_k^{\varepsilon}(a)=\left\lbrace y:a_k(y)<a\right\rbrace$ :
  la limite est le sous-niveau **ouvert**, la différence portant exactement
  sur les observations à distance $a$ exactement ;
* le décalage $\varepsilon$ est optimal : prendre $k$ observations à
  distance au carré $a-\varepsilon/2$ d'un point $y$ (confondues, ou sur une
  sphère minuscule pour rester en position distincte) donne
  $C_{\varepsilon}(y,a)=k/2<k$, donc
  $L_k(a-\varepsilon/2)\not\subseteq L_k^{\varepsilon}(a)$.

Attention à une monotonie qui n'existe pas : **le niveau de Fermi lui-même
n'est pas monotone en $\varepsilon$**. Contre-exemple explicite pour le noyau
logistique avec $e=(0;\,0{,}2;\,1;\,1;\,1;\,5)$ et $k=3$ : $\mu$ vaut
$0{,}99931$ à $\varepsilon=10^{-3}$, décroît jusqu'à $0{,}85907$, puis
remonte. Seule l'inclusion d'ensembles ci-dessus est monotone, et seulement
pour la rampe.

**Corollaire A1 (entrelacement).** Les deux familles de régions étant
emboîtées à décalage $\varepsilon$ près, les modules de persistance de
$\pi_0$ associés sont $\varepsilon$-entrelacés, donc la distance
d'entrelacement de leurs arbres de fusion est au plus $\varepsilon$
(stabilité algébrique de Chazal–Cohen-Steiner–Glisse–Guibas–Oudot pour les
modules ; Morozov–Beketayev–Weber pour la distance d'entrelacement des
arbres de fusion).

**Ce que le corollaire ne donne pas, et c'est démontré.** Un entrelacement
ne donne **jamais** l'égalité combinatoire. Contre-exemple valable pour tout
$\delta>0$ : $f(x)=x^2$ sur $[-1,1]$ a un arbre de fusion à une feuille,
tandis que $g(x)=x^2+\delta\max\!\left(0,1-\lvert x\rvert/\eta\right)$ avec
$\eta^2<\delta$ en a deux, et $\left\Vert f-g\right\Vert_{\infty}=\delta$.
L'argument de compression
$\pi_0(L_k(a-\varepsilon))\to\pi_0(L_k^{\varepsilon}(a))\to\pi_0(L_k(a))$
donne une injection puis une surjection : il borne les cardinaux par le bas,
pas par le haut.

**Théorème A2 (exactitude conditionnelle certifiable).** Soient
$A\subseteq B\subseteq C$ des espaces topologiques. Si $\pi_0(A\to C)$ est
bijective **et** $\pi_0(A\to B)$ est surjective, alors $\pi_0(A\to B)$ et
$\pi_0(B\to C)$ sont bijectives. Appliqué à
$A=L_k(a-\delta)$, $B=L_k^{\varepsilon}(a)$, $C=L_k(a+\delta)$, cela donne :
si **(H1)** la tour dure n'a aucun événement dans $(a-\delta,a+\delta]$ et
**(H2)** chaque composante connexe de $L_k^{\varepsilon}(a)$ rencontre
$L_k(a-\delta)$, alors $\pi_0(L_k^{\varepsilon}(a))\cong\pi_0(L_k(a))$
canoniquement.

L'intérêt est que (H2) est **vérifiable** : c'est exactement l'absence de
composante parasite, que l'on certifie en exhibant, pour chaque composante
trouvée, un chemin certifié (§ 5 de [`OBJET_ET_DIMENSION.md`](OBJET_ET_DIMENSION.md))
vers une observation ou un centre critique de $L_k(a-\delta)$. C'est la
seule voie, dans ce chantier, qui puisse un jour porter autre chose que
`not_claimed` : un certificat, jamais un entrelacement, jamais un accord
numérique.

## 4. Le niveau de Fermi en forme close, et son gradient

Notons $\mu_{\kappa}^{\varepsilon}(y)$ l'unique réel tel que
$C_{\varepsilon}(y,\mu)=\kappa$. La masse $\kappa$ est maintenant un
**réel** : l'axe d'ordre entier $k$ de HGP devient continu, ce qui est une
extension stricte de l'objet, pas une approximation.

**Fait 4 (forme close rampe, démontré ici).** Pour le noyau rampe,
$a\mapsto C_{\varepsilon}(y,a)$ est affine par morceaux croissante, de
ruptures $\left\lbrace e_i\right\rbrace\cup\left\lbrace e_i+\varepsilon\right\rbrace$ ;
$\mu_{\kappa}^{\varepsilon}(y)$ s'obtient donc en triant $2n$ ruptures et en
inversant une fonction affine, en $O(n\log n)$ **sans itération**, et la
formule se transpose telle quelle en arithmétique rationnelle
(`src/ehgp/soft/fermi.py`, `ramp_level`).

**Théorème B (gradient, démontré ici).** Là où $\sum_ig'>0$,

$$\nabla\mu_{\kappa}^{\varepsilon}(y)=2\left(y-m(y)\right),\qquad m(y)=\frac{\sum_is_ix_i}{\sum_is_i},\qquad s_i=g'\!\left(\frac{\mu-e_i}{\varepsilon}\right).$$

*Preuve.* Dériver l'équation implicite
$\sum_ig\!\left((\mu-e_i)/\varepsilon\right)=\kappa$ par rapport à $y$ donne
$\sum_is_i\left(\nabla\mu-\nabla e_i\right)=0$, d'où
$\nabla\mu=\sum_is_i\nabla e_i/\sum_is_i$, et $\nabla e_i=2(y-x_i)$. $\square$

**Corollaire B1 (criticité).** Les points critiques du niveau de Fermi sont
les $y$ égaux au barycentre pondéré $m(y)$ de leur propre coquille. Quand
$\varepsilon\to0$, les poids $s_i$ se concentrent sur les observations
situées à distance $\mu$ de $y$, et la condition devient : **$y$ appartient
à l'enveloppe convexe des observations de sa sphère**. C'est exactement la
condition de criticité de `docs/SPECIFICATION_MORSEHGP3D.md` § 5,
$c\in\mathrm{relint}\,\mathrm{conv}(U(c,a))$, obtenue ici **sans aucune
énumération combinatoire et sans dépendance en $d$**.

C'est le pont central du chantier : MorseHGP3D énumère les sphères
critiques par la combinatoire des supports de cardinal au plus $d+1$ ; E-HGP
les atteint par une **descente**, dont le coût est $O(nd)$ par pas.

## 5. La descente exacte, limite $\varepsilon\to0$ de la descente entropique

Le corollaire B1 se discrétise en une descente exacte, sans flottant, qui
est la brique de production du chantier (`src/ehgp/engine/critical.py`).

> **Itération MEB-Lloyd.** Pour une masse entière $m$ : prendre les $m$
> observations les plus proches de $y$, remplacer $y$ par le centre de leur
> boule englobante minimale exacte, recommencer.

**Théorème C (décroissance et criticité, démontré ici).** Notons $N(y)$ les
$m$ plus proches et $c$ le centre de la boule englobante minimale de $N(y)$,
de rayon au carré $r^2$. Alors $a_m(c)\leq r^2\leq a_m(y)$, la suite des
niveaux décroît et la descente s'arrête ; et en un point fixe, la sphère de
centre $y$ et de rayon au carré $a_m(y)$ vérifie $y\in\mathrm{conv}(U)$,
toute observation hors de $N(y)$ est à distance au moins $r$, et le rang
fermé vaut $s=\lvert I\rvert+\lvert U\rvert=m$ hors dégénérescence
cosphérique. Avec la caractérisation de la spécification ($y$ critique pour
$D_k$ ssi $y\in\mathrm{relint}\,\mathrm{conv}(U)$ et
$\lvert I\rvert<k\leq s$, d'indice $s-k$) :

* $m=k$ donne un point critique d'**indice 0**, soit une **naissance** de
  composante à l'ordre $k$ ;
* $m=k+1$ donne un point critique d'**indice 1**, soit un événement de
  **fusion** à l'ordre $k$.

*Preuve.* Si $N(y)$ est l'ensemble des $m$ plus proches, alors
$a_m(y)=\max_{x\in N(y)}\left\Vert y-x\right\Vert^2$ ; le centre englobant
$c$ minimise ce maximum, donc $\max_{x\in N(y)}\left\Vert c-x\right\Vert^2=r^2\leq a_m(y)$ ;
et comme $N(y)$ fournit $m$ observations à distance au plus $r$ de $c$, on a
$a_m(c)\leq r^2$. Le nombre de parties $N$ est fini et la décroissance est
stricte hors point fixe. En un point fixe, $y$ est un centre englobant, donc
appartient à l'enveloppe convexe de la coquille ; et toute observation hors
de $N(y)$ est à distance au moins la $m$-ième, c'est-à-dire au moins
$r$. $\square$

**Mesuré** (`bench/`, graine 31, 11 observations, départs = observations et
milieux de paires) : la descente ne produit **aucun faux positif** — tout
point fixe atteint est bien une sphère critique du catalogue exact — et sa
couverture du catalogue est complète en $d=2$ et $d=3$ (10/10, 11/11,
19/19), puis partielle au-delà (22/27 en $d=5$, 37/113 en $d=20$, 39/321 en
$d=50$), la limite étant le **nombre de départs** et non la descente. La
complétude reste donc une question de couverture mesurée :
`public_status=not_claimed`.

## 6. La variante logistique : une réfutation utile

Il serait naturel de croire que le niveau de Fermi logistique de masse
entière $k$ tend vers $a_k$ quand $\varepsilon\to0$. **C'est faux.**

**Fait 5 (réfutation, démontré ailleurs, vérifié à 14 chiffres).** Soient
$\gamma=a_{k+1}-a_k>0$, $p$ le nombre d'énergies égales à $a_k$ et $q$ le
nombre d'énergies égales à $a_{k+1}$. Alors

$$\mu_k^{\varepsilon}=\frac{a_k+a_{k+1}}{2}+\frac{\varepsilon}{2}\log\frac{p}{q}+O\!\left(\varepsilon e^{-c/\varepsilon}\right).$$

La limite est le **milieu du trou**, pas $a_k$ : le lissage entropique d'un
coude prend la moyenne des deux pentes. L'erreur ne tend donc pas vers zéro,
elle tend vers $\gamma/2$.

**Fait 6 (l'énoncé correct, avec sa constante optimale).** Avec la cible
**décalée** $\kappa=k-\tfrac12$, pour tout $y$, tout $1\leq k\leq n$ et tout
$\varepsilon>0$,

$$\left\lvert\mu_{k-1/2}^{\varepsilon}(y)-a_k(y)\right\rvert\leq\varepsilon\log\!\left(2\max(k,n-k+1)-1\right)\leq\varepsilon\log(2n-1),$$

et la constante est atteinte. Cet encadrement uniforme est **équivalent** au
double encadrement d'ensembles
$L_k(a-\delta)\subseteq\widetilde{L}_k^{\varepsilon}(a)\subseteq L_k(a+\delta)$
pour tout $a$, avec $\widetilde{L}_k^{\varepsilon}(a)=\left\lbrace y:C_{\varepsilon}(y,a)\geq k-\tfrac12\right\rbrace$ ;
et l'axe des ordres, lui, reste **exact**.

Deux leçons pour la conception :

1. la rampe est **structurellement meilleure** ici : sa saturation exacte
   supprime le défaut du milieu du trou, son encadrement est unilatéral et
   sans facteur $\log n$, et il vaut pour la cible entière $k$ sans décalage
   (théorème A). C'est un argument de fond pour le choix de la
   $f$-divergence du khi-deux, et non un détail d'implémentation ;
2. le logistique garde deux avantages : il est $C^{\infty}$, donc la descente
   et la recherche de cols y sont mieux conditionnées, et il est le noyau
   naturel de l'entropie de Shannon, donc du cadre de Bach. Le chantier
   utilise **le logistique pour chercher et la rampe ou l'exact pour
   certifier**.

## 7. Ce que la régularisation ne répare pas

**Mesuré, et c'est un résultat négatif à conserver.** Lisser le niveau ne
réduit pas le nombre de points critiques en grande dimension. Le nombre de
minima distincts trouvés par descente passe de 24 à 7 quand
$\varepsilon$ croît de $10^{-4}$ à $1$ fois la variance en $d=2$, et de 23 à
13 en $d=3$ ; mais seulement de 53 à 50 en $d=20$, alors que le catalogue
exact en compte 156 (`bench/scale_space.py --n 11 --k 3 --dims 2,3,20 --seed 31`).
Les minima ne fusionnent pas : ils sont séparés par des barrières grandes
devant $\varepsilon$.

Conséquence de conception, qui commande la suite du chantier : en grande
dimension il faut régulariser **la classe de fonctions** — donc le noyau,
donc la métrique — et pas seulement l'axe des niveaux. C'est l'objet de
[`MOTEUR_ET_COUTS.md`](MOTEUR_ET_COUTS.md) § 5 et de
[`OBSTRUCTION_GRANDE_DIMENSION.md`](OBSTRUCTION_GRANDE_DIMENSION.md).

## 8. Questions ouvertes

1. L'identité intégrale $\rho$ entre noyau logistique et noyaux rampe
   pondérés (§ 2) : vraie ou fausse ?
2. Le théorème A2 rend l'exactitude certifiable, mais son hypothèse (H2)
   demande un certificat de chemin par composante trouvée : quel est le coût
   réel de ce certificat, et existe-t-il un test suffisant plus faible ?
3. La descente MEB-Lloyd a-t-elle une garantie de couverture sous hypothèse
   de séparation, ou faut-il se contenter d'une couverture mesurée ?
4. Quelle est la bonne notion de masse continue $\kappa$ du point de vue du
   manuscrit : le § 9.1 fournit déjà une partition de l'unité
   $w_{x\tau}=S_{\tau}/T_x$, qui est elle-même une occupation fractionnaire.
   Les deux coïncident-elles ?
