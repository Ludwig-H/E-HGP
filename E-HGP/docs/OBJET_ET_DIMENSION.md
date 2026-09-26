# L'objet HGP et son audit de dimension

> [!IMPORTANT]
> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.

Ce document fixe **quel objet E-HGP calcule** et **ce qui, dans la théorie de
MorseHGP3D, dépend de la dimension 3**. Il ne redéfinit rien : l'autorité
reste le manuscrit (`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, Parties I
et II) et `docs/SPECIFICATION_MORSEHGP3D.md` § 3–4.

## 1. L'objet, tel qu'il est écrit

Pour $X=\left\lbrace x_1,\ldots,x_n\right\rbrace\subset\mathbb{R}^{d}$ et
$y\in\mathbb{R}^{d}$, soient $a_1(y)\leq\cdots\leq a_n(y)$ les distances au
carré aux observations, $D_k(y)=a_k(y)$, et

$$L_k(a)=\left\lbrace y\in\mathbb{R}^{d}\;:\;D_k(y)\leq a\right\rbrace ,$$

la région couverte par au moins $k$ boules fermées de rayon $\sqrt{a}$
centrées sur $X$. La sortie complète est le foncteur de composantes

$$\mathcal{H}_X(k,a)=\pi_0\!\left(L_k(a)\right),$$

muni des applications induites par les deux inclusions
$L_k(a)\subseteq L_k(b)$ pour $a\leq b$ (flèches **horizontales**) et
$L_{\ell}(a)\subseteq L_k(a)$ pour $k<\ell$ (flèches **verticales**). La
« tour FULL » est une représentation finie de ce foncteur : par ordre, une
forêt de naissances et de multifusions à niveaux exacts, plus les
applications verticales. Rien de tout cela ne mentionne $d$.

Le modèle discret exact est le graphe filtré $\Gamma_k(a)$ : sommets les
parties $F\subseteq X$ de cardinal $k$ telles que $\beta(F)\leq a$, où
$\beta(F)$ est le rayon au carré de la boule englobante minimale de $F$ ;
arête entre $F$ et $F'$ lorsque $\lvert F\cup F'\rvert=k+1$ et
$\beta(F\cup F')\leq a$.

## 2. Audit énoncé par énoncé

L'audit suivant a été établi par deux lectures indépendantes du manuscrit et
de la spécification, puis contre-vérifié. Il porte sur les énoncés que
MorseHGP3D utilise.

| énoncé | contenu | dépendance en $d$ |
| --- | --- | --- |
| Défs 20–22 | $D_k$, $L_k(a)$, la bifiltration | **libre** : purement métrique |
| Théorème 2 | $\pi_0(L_k(a))\cong\pi_0(\Gamma_k(a))$ | **libre** : n'utilise que la convexité des intersections finies de boules et un argument de nerf pour $\pi_0$ ; vaut dans tout espace normé, donc dans un RKHS |
| Proposition 5 | restriction aux adjacences élémentaires $\lvert F\cup F'\rvert=k+1$ | **libre** : combinatoire, monotonie de $\beta$ |
| Théorème 4 | tout simplexe $K$-séparant est de Gabriel | **libre** sous position générale : n'utilise du Fait 12 que la décroissance stricte du rayon quand on retire un point du support |
| Fait 12 | le support minimal d'une boule englobante a au plus $d+1$ points (Carathéodory) | **dimensionnel**, mais la borne liante est $\min(k+1,d+1)$ : à $K_{\max}=10$, le descripteur d'événement reste de taille constante même en $d=1000$ |
| Théorèmes 6–7 | reconstruction par la mosaïque de Delaunay d'ordre $K$ | preuves libres, **cible inutilisable** : taille $O(n^{d+1})$ |
| Propositions 8–9 | $K=2$ par Delaunay planaire en $O(n\log n)$ | **spécifique au plan** |
| § 9.1 | l'arbre est une partition des $(K-1)$-simplexes ; partition de l'unité $w_{x\tau}=S_{\tau}/T_x$ | **libre** : la laminarité et la partition de l'unité ne dépendent pas de $d$ |

Deux avertissements héritent du dépôt et restent valables ici : la
proposition 6 et le théorème 5 sont classés `false_in_general` dans
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (fixture exacte à cinq
points) ; et le « fold en dix forêts » de la v4 n'est pas une tour.

**Conclusion de l'audit.** L'objet et sa caractérisation combinatoire sont
libres en dimension. Ce qui est dimensionnel est **le chemin de calcul** :
supports bornés par $d+1$, catalogue critique énuméré sur ces supports,
clés de Morton, WSPD de constante $2^{\Theta(d)}$, Delaunay d'ordre $k$.
E-HGP garde donc l'objet et change entièrement le chemin.

## 3. Deux lectures utiles de $\Gamma_k$

**(a) Le squelette de Čech.** Un sommet de $\Gamma_k(a)$ est une partie de
cardinal $k$ dont les boules de rayon $\sqrt{a}$ ont un point commun : c'est
exactement un $(k-1)$-simplexe du complexe de Čech $\check{C}(X,\sqrt{a})$.
Une arête est un $k$-simplexe. Donc

$$\pi_0\!\left(L_k(a)\right)\ \cong\ \pi_0\ \text{du graphe d'adjacence des }(k-1)\text{-simplexes de }\check{C}(X,\sqrt{a}).$$

C'est la formulation du § 9.1 du manuscrit (l'arbre est une partition des
$(K-1)$-simplexes, donc laminaire sur les facettes). On retrouve aussi la
bifiltration de multicouverture $\mathrm{Cov}_{r,k}$ de
Corbet–Kerber–Lesnick–Osang, restreinte à $H_0$.

**(b) Le cas $k=1$ est un arbre couvrant minimal.** Pour $k=1$, $L_1(a)$ est
une union de boules ; deux boules de rayon $\sqrt{a}$ se rencontrent si et
seulement si leurs centres sont à distance au plus $2\sqrt{a}$, et la
connexité d'une union de convexes se lit sur les paires. Donc

$$\pi_0\!\left(L_1(a)\right)\ \cong\ \text{composantes du graphe }\left\lbrace \left\Vert x_i-x_j\right\Vert^2\leq4a\right\rbrace ,$$

et la tour d'ordre $1$ est **exactement le dendrogramme de liaison simple**,
de niveaux $\left\Vert x_i-x_j\right\Vert^2/4$, c'est-à-dire l'arbre
couvrant minimal euclidien. Coût $O(n^2d)$, exact, libre en dimension.

**Mesuré.** L'égalité est vérifiée exactement (multiensembles de niveaux
rationnels identiques) en $d=2,3,5,20$ dans
`tests/test_moteur_segment.py` et par `bench/`. C'est la seule partie de la
tour dont E-HGP livre la **valeur exacte complète** en grande dimension au
budget demandé.

## 4. Ce qui reste exactement calculable en grande dimension

| pièce | statut en dimension $d$ quelconque | coût |
| --- | --- | --- |
| $\beta(F)$ pour $\lvert F\rvert\leq K+1$ | exact, rationnel | $O(2^{\lvert F\rvert}(\lvert F\rvert^3+\lvert F\rvert d))$ |
| tour d'ordre $k=1$ | **exacte et complète** | $O(n^2d)$ |
| niveaux d'entrée $a_k(x_i)$ | exacts | $O(n^2d)$ |
| maximum de $a_k$ sur un segment | **exact** (§ 5) | $O(n^2)$ candidats, $O(n\log n)$ par candidat |
| sphères critiques individuelles | exactes, certifiées une par une | $O(nd)$ par pas de descente |
| catalogue critique **complet** | hors d'atteinte dès que $d$ croît | $\Theta\!\left(\binom{n}{k}\right)$ |
| tour FULL **complète** | hors d'atteinte : la sortie elle-même explose | voir [l'obstruction](OBSTRUCTION_GRANDE_DIMENSION.md) |

## 5. Le fait de segment

C'est la brique qui remplace, en grande dimension, la géométrie des sphères
critiques à $d+1$ points. Le long du segment $y(t)=p+t(q-p)$,

$$e_l(t)=\left\Vert y(t)-x_l\right\Vert^2=At^2+B_lt+C_l,\qquad A=\left\Vert q-p\right\Vert^2,$$

et **le coefficient dominant $A$ est le même pour tous les $l$**. L'ordre
des $e_l(t)$ est donc celui des fonctions affines $B_lt+C_l$, d'où

$$a_k\!\left(y(t)\right)=At^2+g_k(t),$$

où $g_k$ est la $k$-ième plus petite de $n$ fonctions affines, donc affine
par morceaux, à ruptures aux croisements de droites. Sur chaque morceau,
$At^2+(st+c)$ est convexe, donc son maximum sur l'intervalle est atteint à
une extrémité. Par conséquent :

> le maximum de $a_k$ sur un segment est atteint en $t=0$, en $t=1$, ou à un
> croisement de droites ; il suffit d'évaluer $a_k$ sur cet ensemble fini de
> temps candidats.

C'est exact, rationnel, et **ne suppose rien sur $d$** : un chemin droit
entre deux observations se certifie par un calcul à une variable. La
conséquence opérationnelle est un **majorant certifié** du niveau de fusion
de deux observations : si le segment est contenu dans $L_k(a)$, les deux
extrémités sont dans la même composante, donc le niveau vrai est au plus
$a$.

## 6. Ce que E-HGP calcule, et sous quel nom

Trois objets distincts, à ne jamais confondre :

1. **La tour FULL exacte** (`src/ehgp/exact/tower.py`) : l'objet du § 1, sur
   les $C(n,k)$ parties. Oracle borné ($n\leq14$), jamais un backend. C'est
   la vérité qui falsifie tout le reste.
2. **La projection sur les observations** (`src/ehgp/engine/point_tower.py`) :
   pour chaque ordre, l'arbre de fusion des observations, avec naissance de
   $x_i$ au niveau $a_k(x_i)$ et fusion certifiée par segment. Exacte pour
   $k=1$ ; **majorant certifié** pour $k\geq2$, l'écart étant mesuré contre
   l'oracle. Coût cubique.
3. **La tour régularisée** (`src/ehgp/soft/`, `src/ehgp/spectral/`) : la tour
   de niveau d'un comptage doux, ou d'un modèle de densité estimé, reliée à
   l'objet exact par l'encadrement du théorème A de
   [`REGULARISATION_ENTROPIQUE.md`](REGULARISATION_ENTROPIQUE.md).

Le contrat de sortie public du dépôt
(`schemas/morsehgp3d-contract-v2.schema.json`) est verrouillé à $d=3$ et
$K\leq10$ : E-HGP **ne publie pas** dans ce schéma et ne le modifie pas. Ses
propres enregistrements portent l'étiquette `ehgp.full_tower.v1` et
`ehgp.point_tower.v1`, avec un digest sha256 canonique.
