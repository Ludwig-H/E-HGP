"""Noyau rampe en rationnels exacts, et encadrement certifie de `L_k(a)`.

Ce module est la partie CERTIFIANTE de la voie entropique : il decide en
arithmetique rationnelle exacte (`fractions.Fraction`) l'appartenance au
sous-niveau doux, et il porte la preuve de l'encadrement qui relie ce
sous-niveau doux au sous-niveau dur de la specification.

Notations. $X = \\lbrace x_{1}, \\ldots, x_{n} \\rbrace$ dans
$\\mathbb{R}^{d}$, $e_{i}(y) = \\Vert y - x_{i} \\Vert^{2}$,
$a_{k}(y)$ la k-ieme plus petite energie,
$N(y, b) = \\#\\lbrace i : e_{i}(y) \\leq b \\rbrace$ le comptage dur,
$L_{k}(a) = \\lbrace y : a_{k}(y) \\leq a \\rbrace
 = \\lbrace y : N(y, a) \\geq k \\rbrace$ le sous-niveau dur
(`docs/SPECIFICATION_MORSEHGP3D.md` paragraphes 3 et 4). Pour
$\\epsilon > 0$ on pose

    $C_{\\epsilon}(y, a) = \\sum_{i}
     \\min ( \\max ( \\frac{ a - e_{i}(y) }{ \\epsilon } , 0 ) , 1 )$ ,
    $L_{k}^{r}(a) = \\lbrace y : C_{\\epsilon}(y, a) \\geq k \\rbrace$ .

== THEOREME 1 (encadrement, profil quelconque) ==

Soit $g$ croissante de $\\mathbb{R}$ dans $[0, 1]$ avec $g(u) = 0$ pour
tout $u \\leq 0$ et $g(u) = 1$ pour tout $u \\geq 1$. Posons
$C^{g}_{\\epsilon}(y, a) = \\sum_{i} g( ( a - e_{i}(y) ) / \\epsilon )$.
Alors, pour tous $y$, $a$, $\\epsilon > 0$ :

    $N(y, a - \\epsilon) \\leq C^{g}_{\\epsilon}(y, a) \\leq N(y, a)$ ,

et donc, pour tout $k$ (entier ou reel),

    $L_{k}(a - \\epsilon) \\subseteq
     \\lbrace y : C^{g}_{\\epsilon}(y, a) \\geq k \\rbrace
     \\subseteq L_{k}(a)$ .

Preuve. Terme a terme. Si $e_{i} \\leq a - \\epsilon$ alors
$( a - e_{i} ) / \\epsilon \\geq 1$ et le terme vaut $1$ ; en sommant sur
ces indices on obtient la minoration. Si $e_{i} > a$ alors
$( a - e_{i} ) / \\epsilon < 0$ et le terme vaut $0$ ; sinon il est majore
par $1$ ; en sommant on obtient la majoration. Les inclusions suivent en
comparant a $k$. CQFD.

Aucun facteur $\\log n$, aucune hypothese sur $d$, sur $n$, sur $X$ ni sur
la position generale : la preuve est point par point. Le PROFIL de $g$ est
libre ; seules comptent les deux extremites de la fenetre. C'est
exactement pourquoi, dans la famille $f_{\\rho}$ de Bach, le parametre
$\\rho$ n'agit sur l'encadrement que par la largeur $\\epsilon c(\\rho)$
(voir `families.py`, proposition 4) : pour l'encadrement, $\\rho$ est une
temperature deguisee.

COROLLAIRE (rampe). $g(u) = \\min ( \\max ( u , 0 ) , 1 )$ satisfait les
hypotheses, donc
$L_{k}(a - \\epsilon) \\subseteq L_{k}^{r}(a) \\subseteq L_{k}(a)$ .

Pourquoi la rampe parmi tous les profils admissibles : elle est AFFINE sur
sa fenetre, donc $a \\mapsto C_{\\epsilon}(y, a)$ est affine par morceaux a
noeuds rationnels, et son INVERSE (le niveau ci-dessous) se calcule par
une interpolation lineaire exacte. Un profil rationnel non affine (par
exemple $u^{2} ( 3 - 2 u )$) garde un comptage rationnel mais rend le
niveau algebrique de degre $> 1$ : la decision resterait exacte, le niveau
ne serait plus une fraction.

== THEOREME 2 (forme fonctionnelle) ==

Posons $A^{k}_{\\epsilon}(y) = \\min \\lbrace a : C_{\\epsilon}(y, a) \\geq k
 \\rbrace$ (le minimum est atteint car $C_{\\epsilon}(y, \\cdot)$ est
continue et croissante). C'est exactement le niveau de Fermi de la famille
rampe, avec la convention d'extremite gauche de `families.py`. Alors

    $a_{k}(y) < A^{k}_{\\epsilon}(y) \\leq a_{k}(y) + \\epsilon$ ,

et la borne superieure est ATTEINTE. De plus

    $| A^{k}_{\\epsilon}(y) - A^{k}_{\\epsilon}(y') |
     \\leq \\max_{i} | e_{i}(y) - e_{i}(y') |$ ,

donc $A^{k}_{\\epsilon}$ est continue sur $\\mathbb{R}^{d}$.

Preuve. Les deux inclusions du theoreme 1 se relisent : $L_{k}^{r}(a)
\\subseteq L_{k}(a)$ donne $a_{k} \\leq A^{k}_{\\epsilon}$, et
$L_{k}(a - \\epsilon) \\subseteq L_{k}^{r}(a)$ donne
$A^{k}_{\\epsilon} \\leq a_{k} + \\epsilon$. Pour la stricte inegalite de
gauche : en $a = a_{k}(y)$, les indices tels que $e_{i} = a_{k}$
contribuent $0$ et les autres contributions non nulles ont
$e_{i} < a_{k}$, donc au plus $k - 1$, d'ou
$C_{\\epsilon}(y, a_{k}) \\leq k - 1 < k$. La borne superieure est atteinte
des que $k$ energies valent $a_{k}$ et les autres sont plus grandes que
$a_{k} + \\epsilon$ : le comptage n'atteint $k$ qu'en $a_{k} + \\epsilon$.
Pour la lipschitzianite, soit $\\delta = \\max_{i} | e_{i}(y) - e_{i}(y') |$ ;
alors $e_{i}(y') \\leq e_{i}(y) + \\delta$ pour tout $i$, donc
$C_{\\epsilon}(y', a + \\delta) \\geq C_{\\epsilon}(y, a)$ ; en prenant
$a = A^{k}_{\\epsilon}(y)$ il vient
$A^{k}_{\\epsilon}(y') \\leq A^{k}_{\\epsilon}(y) + \\delta$, et l'on conclut
par symetrie. CQFD.

== THEOREME 3 (monotonie en epsilon et limite) ==

Pour $0 < \\epsilon \\leq \\epsilon'$ et tous $y$, $a$ :

    $C_{\\epsilon'}(y, a) \\leq C_{\\epsilon}(y, a)$ ,
    $L_{k}^{r, \\epsilon'}(a) \\subseteq L_{k}^{r, \\epsilon}(a)$ ,
    $A^{k}_{\\epsilon}(y) \\leq A^{k}_{\\epsilon'}(y)$ .

La famille $\\epsilon \\mapsto L_{k}^{r, \\epsilon}(a)$ CROIT donc quand
$\\epsilon$ decroit, et

    $\\bigcup_{\\epsilon > 0} L_{k}^{r, \\epsilon}(a)
     = \\lbrace y : a_{k}(y) < a \\rbrace$ ,

c'est-a-dire le sous-niveau OUVERT, strictement plus petit que $L_{k}(a)$
des qu'un point est a distance exactement $a$. Autrement dit, le bord
$\\lbrace a_{k} = a \\rbrace$ n'est atteint par aucun $\\epsilon > 0$ ;
c'est la contrepartie exacte de la stricte inegalite
$a_{k} < A^{k}_{\\epsilon}$ du theoreme 2. Enfin
$A^{k}_{\\epsilon}(y) \\to a_{k}(y)$ quand $\\epsilon \\to 0^{+}$, de facon
monotone et uniforme en $y$ (theoreme 2).

Preuve. Pour $u \\geq 0$, $\\epsilon \\mapsto \\min ( u / \\epsilon , 1 )$
est decroissante, et pour $u < 0$ le terme est nul : d'ou la premiere
ligne, puis les deux suivantes. Pour l'union : si $a_{k}(y) < a$, il y a au
moins $k$ indices avec $e_{i} < a$ ; en prenant
$\\epsilon \\leq a - a_{k}(y)$ chacun d'eux contribue $1$, donc
$y \\in L_{k}^{r, \\epsilon}(a)$. Reciproquement chaque
$L_{k}^{r, \\epsilon}(a)$ est contenu dans
$\\lbrace a_{k} < a \\rbrace$ par le theoreme 2. CQFD.

== THEOREME 4 (le decalage epsilon est optimal) ==

Pour tout $\\theta$ dans $(0, 1)$, tout $k \\geq 1$ et tout $\\epsilon > 0$,
il existe un nuage $X$ et un point $y$ tels que

    $y \\in L_{k}(a - \\theta \\epsilon)$ et
    $y \\notin L_{k}^{r, \\epsilon}(a)$ .

Le decalage $\\epsilon$ du theoreme 1 ne peut donc pas etre remplace par
$\\epsilon / 2$, ni par $\\theta \\epsilon$ pour aucun $\\theta < 1$.

Preuve. Placer exactement $k$ observations a distance au carre
$a - \\theta \\epsilon$ de $y$ et toutes les autres a distance au carre
superieure a $a$. Alors $a_{k}(y) = a - \\theta \\epsilon$, donc
$y \\in L_{k}(a - \\theta \\epsilon)$, tandis que
$C_{\\epsilon}(y, a) = k \\theta < k$. CQFD.

Temoin grave (coordonnees exactes, $d = 2$, $k = 1$,
$\\epsilon = 2$, $a = 2$, $\\theta = 1/2$) :
$X = \\lbrace (0, 0) , (100, 0) \\rbrace$ et $y = (1, 0)$ donnent
$a_{1}(y) = 1 = a - \\epsilon / 2$ et $C_{2}(y, 2) = 1/2 < 1$.

== THEOREME 5 (variante logistique : la constante exacte) ==

Posons $\\sigma(u) = 1 / ( 1 + e^{-u} )$ et
$S_{\\epsilon}(y, a) = \\sum_{i} \\sigma( ( a - e_{i}(y) ) / \\epsilon )$.
Soit $s = \\epsilon \\log ( 2 n - 1 )$ (et a fortiori
$s = \\epsilon \\log ( 2 n )$). Alors, pour $1 \\leq k \\leq n$ :

    $L_{k}(a - s) \\subseteq
     \\lbrace y : S_{\\epsilon}(y, a) \\geq k - 1/2 \\rbrace
     \\subseteq L_{k}(a + s)$ .

Preuve. Inclusion de gauche : si au moins $k$ indices ont
$e_{i} \\leq a - s$, chacun contribue au moins
$\\sigma( s / \\epsilon ) = 1 - 1 / ( 1 + e^{s / \\epsilon} ) = 1 - 1/(2n)$,
donc $S_{\\epsilon} \\geq k ( 1 - 1/(2n) ) \\geq k - 1/2$ puisque
$k \\leq n$. Inclusion de droite : si au plus $k - 1$ indices ont
$e_{i} \\leq a + s$, alors
$S_{\\epsilon} \\leq ( k - 1 ) + ( n - k + 1 ) \\sigma( - s / \\epsilon )
 = ( k - 1 ) + ( n - k + 1 ) / ( 2 n ) \\leq k - 1/2$,
et l'inegalite est stricte : pour $k \\geq 2$ parce que
$\\sigma < 1$ strictement, et pour $k = 1$ parce que l'egalite exigerait
$e_{i} = a + s$ pour tout $i$, ce qui contredirait l'hypothese. CQFD.

La constante est TENDUE des deux cotes, et les deux configurations
critiques sont differentes.

  * A gauche, le cas critique est $k = n$ : si les $n$ observations sont
    exactement a $e_{i} = a - s'$ alors
    $S_{\\epsilon} = n \\sigma( s' / \\epsilon )$, qui atteint
    $n - 1/2$ si et seulement si
    $\\sigma( s' / \\epsilon ) \\geq 1 - 1 / ( 2 n )$, c'est-a-dire
    $s' \\geq \\epsilon \\log ( 2 n - 1 )$. Tout $s' < s$ casse donc
    l'inclusion de gauche.
  * A droite, le cas critique est $k = 1$ : si les $n$ observations sont
    juste au-dela de $a + s'$ alors $S_{\\epsilon}$ vaut presque
    $n \\sigma( - s' / \\epsilon ) = n / ( 1 + e^{s' / \\epsilon} )$, qui
    depasse $1/2$ des que $s' < \\epsilon \\log ( 2 n - 1 )$, alors que
    $L_{1}(a + s')$ ne contient pas $y$. Tout $s' < s$ casse donc
    l'inclusion de droite.

Les deux temoins sont graves dans `tests/test_familles_noyaux.py`
(`test_constante_optimale` pour les deux cotes,
`test_encadrement_logistique` pour un corpus ou l'inclusion de droite
exige effectivement le decalage). Le seuil $k - 1/2$ ne
peut pas etre remplace par $k$ : si $n = k$, alors
$S_{\\epsilon}(y, a) < n = k$ pour tous $y$ et $a$, donc
$\\lbrace S_{\\epsilon} \\geq k \\rbrace$ est vide alors que $L_{k}(b)$ ne
l'est pas pour $b$ assez grand. Comparaison : la rampe donne un
encadrement SANS dependance en $n$, avec le seuil entier $k$ et une seule
inclusion decalee ; la logistique paie son support infini par un decalage
$\\epsilon \\log ( 2 n - 1 )$ des deux cotes et par un seuil demi-entier.

== THEOREME 6 (consequence topologique, et ce qu'elle ne donne pas) ==

Du theoreme 2 on tire
$\\Vert A^{k}_{\\epsilon} - a_{k} \\Vert_{\\infty} \\leq \\epsilon$ sur
$\\mathbb{R}^{d}$, les deux fonctions etant continues et TEMPEREES au sens
de la definition 2 de [1] (les sous-niveaux sont des reunions finies
d'intersections de boules fermees, donc ont un nombre fini de composantes
connexes). Par consequent :

  * les suites $H_{0}(L_{k}(a))$ et $H_{0}(L_{k}^{r, \\epsilon}(a))$ sont
    $\\epsilon$-entrelacees au sens du theoreme 1 de [1] (enonce simplifie
    de [2]), donc les diagrammes de persistance de dimension $0$ sont a
    distance bottleneck au plus $\\epsilon$ ;
  * les arbres de fusion verifient
    $d_{I}( T_{a_{k}} , T_{A^{k}_{\\epsilon}} ) \\leq \\epsilon$ par le
    theoreme 2 (stabilite) de [1], ou $d_{I}$ est la distance
    d'entrelacement de la definition 4 de [1] ;
  * l'entrelacement est NATUREL EN $k$ : toutes les fleches en jeu
    ($L_{k}^{r}(a) \\subseteq L_{k}(a) \\subseteq L_{k}^{r}(a + \\epsilon)$
    et $L_{k}(a) \\subseteq L_{k-1}(a)$,
    $L_{k}^{r}(a) \\subseteq L_{k-1}^{r}(a)$) sont des INCLUSIONS de
    parties de $\\mathbb{R}^{d}$, et tout diagramme d'inclusions commute.
    Les applications verticales de la tour sont donc respectees a
    $\\epsilon$ pres, au sens de l'entrelacement du bi-filtre en
    $(k, a)$.

Ce que cet encadrement NE DONNE PAS, et qu'il ne faut pas lui faire dire :

  * il ne donne AUCUNE egalite combinatoire de la tour : le nombre de
    naissances et de multifusions peut differer, toute paire naissance-mort
    de persistance inferieure a $2 \\epsilon$ peut apparaitre ou
    disparaitre, et les niveaux critiques eux-memes peuvent bouger de
    $\\epsilon$. Le digest canonique de `exact/tower.py` n'est donc pas
    reproductible par cette voie, et aucun `public_status=exact` ne peut
    en sortir ;
  * a un niveau $a$ FIXE, il ne donne pas la partition : deux composantes
    peuvent fusionner du cote doux et pas du cote dur ;
  * il ne dit rien du cote combinatoire $\\Gamma_{k}$ : l'identification
    $\\pi_{0}(L_{k}(a)) = \\pi_{0}(\\Gamma_{k}(a))$ est un theoreme sur
    l'objet dur, et le comptage doux n'a pas de modele combinatoire ;
  * la borne est en $\\epsilon$ ABSOLU sur le niveau, c'est-a-dire sur un
    RAYON AU CARRE : elle n'est pas invariante d'echelle.

Le comptage doux reste donc un PROPOSEUR : il localise les niveaux
critiques a $\\epsilon$ pres, en toute dimension et sans combinatoire, et
la decision finale revient aux modules exacts.

References verifiees.
[1] D. Morozov, K. Beketayev, G. H. Weber, "Interleaving Distance between
    Merge Trees", TopoInVis 2013 (definition 3 : applications
    $\\epsilon$-compatibles ; definition 4 : $d_{I}$ ; theoreme 1 :
    $\\epsilon$-entrelacement implique bottleneck au plus $\\epsilon$ ;
    theoreme 2 : $d_{I}( T_{f} , T_{g} ) \\leq \\sup_{x} | f(x) - g(x) |$).
[2] F. Chazal, D. Cohen-Steiner, M. Glisse, L. Guibas, S. Oudot,
    "Proximity of Persistence Modules and their Diagrams", SoCG 2009.
[3] F. Bach, billet "spectral log-density estimation",
    https://francisbach.com/spectral_log_density_estimation/ (consulte le
    25 septembre 2026).

Arithmetique. Tout ce module travaille en `Fraction` : aucune decision ne
passe par un flottant. Les mutants ci-dessous servent aux portes de
`tests/test_familles_noyaux.py` : ils doivent tous etre tues.
"""

from fractions import Fraction

from ..exact.meb import squared_distance, to_rational_cloud, to_rational_point

MUTANTS = ("no_epsilon", "tailed", "shifted")


def rational_energies(cloud, point):
    """Vecteur exact des `e_i(y)`, en `Fraction`."""
    points = to_rational_cloud(cloud)
    target = to_rational_point(point)
    if len(target) != len(points[0]):
        raise ValueError("dimension du point incompatible avec le nuage")
    return tuple(squared_distance(target, other) for other in points)


def hard_count(energies, level):
    """Comptage dur `#{i : e_i <= level}`."""
    level = Fraction(level)
    return sum(1 for energy in energies if energy <= level)


def hard_level(energies, order):
    """`a_order(y)` exact : la `order`-ieme plus petite energie."""
    order = int(order)
    if not 1 <= order <= len(energies):
        raise ValueError("ordre hors de [1, n]")
    return sorted(energies)[order - 1]


def ramp_weight(energy, level, epsilon, mutant=None):
    """Poids d'occupation exact d'une observation, avec mutant optionnel.

    Sans mutant : `min(max((level - energy) / epsilon, 0), 1)`.
    Les mutants sont des fautes d'implementation plausibles, gravees ici
    pour que les portes puissent les tuer :

    * `no_epsilon` : le pas `epsilon` est oublie au denominateur ;
    * `tailed` : profil rationnel a queues, jamais nul, jamais egal a 1
      (surrogate de la sigmoide) ;
    * `shifted` : normalisation de `f_0` (fenetre `[-1, 0]`) appliquee par
      erreur a la convention de la rampe (fenetre `[0, 1]`).
    """
    level = Fraction(level)
    epsilon = Fraction(epsilon)
    if epsilon <= 0:
        raise ValueError("epsilon doit etre strictement positif")
    if mutant is None:
        value = (level - energy) / epsilon
    elif mutant == "no_epsilon":
        value = level - energy
    elif mutant == "shifted":
        value = (level - energy) / epsilon + 1
    elif mutant == "tailed":
        ratio = (level - energy) / epsilon
        return (Fraction(1, 2)) * (1 + ratio / (1 + abs(ratio)))
    else:
        raise ValueError("mutant inconnu : " + str(mutant))
    if value <= 0:
        return Fraction(0)
    if value >= 1:
        return Fraction(1)
    return value


def soft_count_exact(energies, level, epsilon, mutant=None):
    """Comptage doux rampe exact `C_epsilon(y, level)`, en `Fraction`."""
    total = Fraction(0)
    for energy in energies:
        total += ramp_weight(energy, level, epsilon, mutant)
    return total


def soft_level_exact(energies, mass, epsilon):
    """`A^k_epsilon(y)` exact : plus petit `a` tel que `C_epsilon(y, a) >= mass`.

    `C_epsilon(y, .)` est affine par morceaux, continue et croissante, de
    noeuds `e_i` et `e_i + epsilon` : on localise l'intervalle de passage
    puis on interpole lineairement. Le resultat est une `Fraction` exacte.
    La masse peut etre fractionnaire (axe d'ordre continu).
    """
    epsilon = Fraction(epsilon)
    if epsilon <= 0:
        raise ValueError("epsilon doit etre strictement positif")
    mass = Fraction(mass)
    if not 0 < mass <= len(energies):
        raise ValueError("masse hors de (0, n]")
    knots = sorted(set(list(energies) + [energy + epsilon for energy in energies]))
    previous = None
    previous_count = Fraction(0)
    for knot in knots:
        count = soft_count_exact(energies, knot, epsilon)
        if count >= mass:
            if previous is None:
                return knot
            slope = count - previous_count
            if slope == 0:
                return knot
            return previous + (knot - previous) * (mass - previous_count) / slope
        previous = knot
        previous_count = count
    raise RuntimeError("masse inatteignable : invariant de balayage viole")


def belongs_hard(energies, order, level):
    """Decide exactement `y dans L_order(level)`."""
    return hard_count(energies, level) >= int(order)


def belongs_soft(energies, order, level, epsilon, mutant=None):
    """Decide exactement `y dans L_order^rampe(level)`. Aucun flottant."""
    return soft_count_exact(energies, level, epsilon, mutant) >= Fraction(order)


def check_bracket(energies, order, level, epsilon, mutant=None):
    """Verdict exact de l'encadrement en un point, pour les portes.

    Renvoie un dictionnaire :

    * `inner` : `y` dans `L_order(level - epsilon)` ;
    * `soft` : `y` dans `L_order^rampe(level)` ;
    * `outer` : `y` dans `L_order(level)` ;
    * `inner_ok` : l'inclusion de gauche tient en ce point ;
    * `outer_ok` : l'inclusion de droite tient en ce point ;
    * `strict` : `y` separe le sous-niveau doux du sous-niveau dur, ce qui
      atteste que l'encadrement n'est pas une egalite (anti-vacuite).
    """
    level = Fraction(level)
    epsilon = Fraction(epsilon)
    inner = belongs_hard(energies, order, level - epsilon)
    soft = belongs_soft(energies, order, level, epsilon, mutant)
    outer = belongs_hard(energies, order, level)
    return {
        "inner": inner,
        "soft": soft,
        "outer": outer,
        "inner_ok": (not inner) or soft,
        "outer_ok": (not soft) or outer,
        "strict": outer and not soft,
    }
