"""Correspondance f-divergence / forme de noyau pour le programme de masse.

Ce module etablit la correspondance entre le POTENTIEL de regularisation
choisi et la FORME du noyau d'occupation, puis l'implemente et la fait
verifier par un solveur generique independant (`scipy`).

Notations. Pour $y$ dans $\\mathbb{R}^{d}$ et un nuage
$X = \\lbrace x_{1}, \\ldots, x_{n} \\rbrace$ on pose
$e_{i}(y) = \\Vert y - x_{i} \\Vert^{2}$,
et $a_{k}(y)$ est la k-ieme plus petite des energies $e_{i}(y)$.
Les energies sont notees $e$ sans argument quand $y$ est fixe, et
$e_{(1)} \\leq \\cdots \\leq e_{(n)}$ est leur reordonnancement croissant.

== 1. LE PROGRAMME DE MASSE ET SON MULTIPLICATEUR ==

Pour $k$ entier, $1 \\leq k \\leq n$, on considere le programme lineaire

    $P(k) : \\min \\sum_{i} w_{i} e_{i}$ sur
    $\\lbrace w : 0 \\leq w_{i} \\leq 1 , \\sum_{i} w_{i} = k \\rbrace$ .

PROPOSITION 1. La valeur de $P(k)$ est $\\sum_{j \\leq k} e_{(j)}$.
L'ensemble des multiplicateurs optimaux de la contrainte de masse est
l'intervalle $[ e_{(k)} , e_{(k+1)} ]$ avec la convention
$e_{(n+1)} = + \\infty$ ; son extremite gauche est exactement $a_{k}(y)$.

Preuve. Le vecteur qui vaut $1$ sur les $k$ plus petites energies et $0$
ailleurs est admissible et atteint $\\sum_{j \\leq k} e_{(j)}$. Soit $w$
admissible et $\\mu = e_{(k)}$. Comme $\\sum_{i} w_{i} = k$,

    $\\sum_{i} w_{i} e_{i} - \\sum_{j \\leq k} e_{(j)}
     = \\sum_{i} w_{i} ( e_{i} - \\mu ) - \\sum_{j \\leq k} ( e_{(j)} - \\mu )$ .

Les indices $j \\leq k$ verifient $e_{(j)} \\leq \\mu$ et les indices
$j > k$ verifient $e_{(j)} \\geq \\mu$, donc
$\\sum_{j \\leq k} ( e_{(j)} - \\mu ) = - \\sum_{i} ( \\mu - e_{i} )^{+}$ .
Par ailleurs, terme a terme, $w_{i} ( e_{i} - \\mu ) \\geq - ( \\mu - e_{i} )^{+}$
(si $e_{i} \\geq \\mu$ le membre de gauche est positif ; sinon on utilise
$w_{i} \\leq 1$). La difference est donc positive : le minimum est atteint.
Pour le dual, la fonction duale vaut

    $q(\\mu) = k \\mu - \\sum_{i} ( \\mu - e_{i} )^{+}$ ,

concave, affine par morceaux, de derivee a droite
$k - \\#\\lbrace i : e_{i} \\leq \\mu \\rbrace$ et de derivee a gauche
$k - \\#\\lbrace i : e_{i} < \\mu \\rbrace$ : l'argmax est
$[ e_{(k)} , e_{(k+1)} ]$. Le saut de dualite est nul (programme lineaire
faisable borne), donc $a_{k}(y) = e_{(k)}$ est le plus petit multiplicateur
optimal. CQFD.

Remarque de vocabulaire. Dire que "le multiplicateur de masse est $a_{k}$"
est donc exact a condition de choisir la CONVENTION DE GAUCHE ; c'est la
meme convention que celle qui definit $a_{k}$ comme la k-ieme plus petite
energie et non comme un point quelconque de l'intervalle critique.

== 2. REGULARISATION SEPARABLE : FORME FERMEE DES POIDS ==

Soit $\\varphi$ strictement convexe sur $[0, 1]$, derivable sur $(0, 1)$,
de derivee $\\varphi'$ strictement croissante, de limites
$\\varphi'(0^{+})$ dans $[ - \\infty , + \\infty )$ et
$\\varphi'(1^{-})$ dans $( - \\infty , + \\infty ]$. Pour $\\epsilon > 0$ on
considere

    $P_{\\epsilon}(k) : \\min \\sum_{i} w_{i} e_{i}
     + \\epsilon \\sum_{i} \\varphi(w_{i})$ sur le meme polytope .

PROPOSITION 2. $P_{\\epsilon}(k)$ a une solution unique $w$, et il existe
$\\mu$ reel tel que, pour tout $i$,

    $w_{i} = g \\left( \\frac{ \\mu - e_{i} }{ \\epsilon } \\right)$ ,
    $g(u) = 0$ si $u \\leq \\varphi'(0^{+})$ ,
    $g(u) = 1$ si $u \\geq \\varphi'(1^{-})$ ,
    $g(u) = ( \\varphi' )^{-1}(u)$ sinon .

Le niveau $\\mu$ (dit NIVEAU DE FERMI) est n'importe quelle solution de
$\\sum_{i} g( ( \\mu - e_{i} ) / \\epsilon ) = k$ ; l'ensemble des solutions
est un intervalle compact non vide, reduit a un point des que $g$ est
strictement croissante au voisinage de la solution.

Preuve. L'objectif est strictement convexe et continu sur un polytope
compact non vide (il contient $w_{i} = k / n$), d'ou existence et unicite
de $w$. Ecrivons le lagrangien

    $L(w, \\mu, \\lambda, \\nu) = \\sum_{i} w_{i} e_{i}
     + \\epsilon \\sum_{i} \\varphi(w_{i}) - \\mu ( \\sum_{i} w_{i} - k )
     - \\sum_{i} \\lambda_{i} w_{i} + \\sum_{i} \\nu_{i} ( w_{i} - 1 )$ ,

avec $\\lambda \\geq 0$ et $\\nu \\geq 0$. La contrainte d'egalite est affine
et le polytope a un point admissible, donc la qualification de Slater
affine est satisfaite et les conditions KKT sont necessaires et
suffisantes. La stationnarite s'ecrit
$e_{i} + \\epsilon \\varphi'(w_{i}) - \\mu - \\lambda_{i} + \\nu_{i} = 0$
aux points ou $\\varphi$ est derivable, et les complementarites sont
$\\lambda_{i} w_{i} = 0$ et $\\nu_{i} ( w_{i} - 1 ) = 0$. Trois cas :

  * $0 < w_{i} < 1$ : alors $\\lambda_{i} = \\nu_{i} = 0$ et
    $\\varphi'(w_{i}) = ( \\mu - e_{i} ) / \\epsilon$, d'ou
    $w_{i} = ( \\varphi' )^{-1}( ( \\mu - e_{i} ) / \\epsilon )$ ;
  * $w_{i} = 0$ : alors $\\nu_{i} = 0$ et $\\lambda_{i} \\geq 0$ donne
    $\\epsilon \\varphi'(0^{+}) \\geq \\mu - e_{i}$, soit
    $( \\mu - e_{i} ) / \\epsilon \\leq \\varphi'(0^{+})$ ;
  * $w_{i} = 1$ : alors $\\lambda_{i} = 0$ et $\\nu_{i} \\geq 0$ donne
    $\\epsilon \\varphi'(1^{-}) \\leq \\mu - e_{i}$, soit
    $( \\mu - e_{i} ) / \\epsilon \\geq \\varphi'(1^{-})$ .

Ces trois cas sont exactement la definition de $g$, qui est bien definie
et croissante puisque $\\varphi'$ est strictement croissante. Enfin
$\\mu \\mapsto \\sum_{i} g( ( \\mu - e_{i} ) / \\epsilon )$ est continue,
croissante, tend vers $0$ en $- \\infty$ et vers $n$ en $+ \\infty$ ; comme
$0 < k < n$ ou $k = n$ (borne atteinte), l'equation de masse a une
solution, et l'ensemble des solutions d'une fonction croissante continue
est un intervalle ferme. CQFD.

Consequence pratique. $w$ est TOUJOURS unique, $\\mu$ ne l'est pas :
pour un noyau a support compact, si $k$ energies sont strictement sous la
fenetre et les autres strictement au-dessus, tout $\\mu$ d'un intervalle
convient. La convention retenue ici, comme au point 1, est l'EXTREMITE
GAUCHE, c'est-a-dire le plus petit $\\mu$ realisant la masse.

Invariance affine. Remplacer $\\varphi$ par $\\varphi + \\alpha w + \\beta$
ne change pas $w$ et translate $\\mu$ de $\\epsilon \\alpha$ (la
stationnarite s'ecrit
$e_{i} + \\epsilon \\varphi'(w_{i}) + \\epsilon \\alpha = \\mu$) : le noyau
n'est donc fonction que de la CLASSE de $\\varphi$ modulo les fonctions
affines. Ce point sert au paragraphe 4.

== 3. CORRESPONDANCE DE LEGENDRE : QUATRE FAMILLES ==

PROPOSITION 3. Avec les notations de la proposition 2 :

  a. entropie de Fermi-Dirac,
     $\\varphi(w) = w \\log w + (1 - w) \\log (1 - w)$ ,
     $\\varphi'(w) = \\log ( w / (1 - w) )$ ,
     $g(u) = 1 / (1 + e^{-u})$ : noyau LOGISTIQUE, fenetre
     $( - \\infty , + \\infty )$, support non compact ;

  b. chi-deux de Pearson, $\\varphi(w) = w^{2} / 2$ ,
     $\\varphi'(w) = w$ ,
     $g(u) = \\min ( \\max ( u , 0 ) , 1 )$ : noyau RAMPE, fenetre
     $[0, 1]$, support compact, profil AFFINE ;

  c. Tsallis, $\\varphi_{\\alpha}(w) = ( w^{\\alpha} - w ) / ( \\alpha - 1 )$
     avec $\\alpha > 1$ ,
     $\\varphi_{\\alpha}'(w) = ( \\alpha w^{\\alpha - 1} - 1 ) / ( \\alpha - 1 )$ ,
     $g(u) = ( ( ( \\alpha - 1 ) u + 1 ) / \\alpha )^{1 / ( \\alpha - 1 )}$
     tronque a $[0, 1]$ : noyau EN PUISSANCE, fenetre
     $[ - 1 / ( \\alpha - 1 ) , 1 ]$, donc support compact de largeur
     $\\epsilon \\alpha / ( \\alpha - 1 )$. Le cas $\\alpha = 2$ redonne la
     rampe de largeur $2 \\epsilon$ ; $\\alpha \\to 1^{+}$ redonne
     l'entropie de Shannon (la fenetre s'elargit indefiniment) ;
     $\\alpha \\to + \\infty$ laisse la fenetre tendre vers $[0, 1]$ mais
     fait degenerer le PROFIL vers l'indicatrice de $u > 0$, c'est-a-dire
     vers le comptage dur : c'est une seconde route vers la limite dure,
     a $\\epsilon$ fixe (verifie numeriquement dans le fichier de tests) ;

  d. famille de Bach, $\\varphi(w) = f_{\\rho}(w)$ avec
     $f_{\\rho}(t) = \\frac{1}{2} \\frac{ ( t - 1 )^{2} }{ \\rho t + 1 - \\rho }$
     et $\\rho$ dans $[0, 1]$ : voir le paragraphe 4.

Preuve. Dans chaque cas $\\varphi''> 0$ sur $(0, 1)$ (respectivement
$1 / w + 1 / (1 - w)$, $1$, $\\alpha w^{\\alpha - 2}$), donc $\\varphi'$ est
strictement croissante et la proposition 2 s'applique ; l'expression de
$g$ est l'inversion directe de $\\varphi'$, et les bornes de fenetre sont
$\\varphi'(0^{+})$ et $\\varphi'(1^{-})$. CQFD.

== 4. LIEN AVEC LA FAMILLE $f_{\\rho}$ DE FRANCIS BACH ==

Le billet "spectral log-density estimation" de Francis Bach
(https://francisbach.com/spectral_log_density_estimation/, consulte le
25 septembre 2026) utilise la famille de chi-deux ponderes

    $f_{\\rho}(t) = \\frac{1}{2} \\frac{ ( t - 1 )^{2} }{ \\rho t + 1 - \\rho }$ ,

avec $\\rho = 0$ Pearson, $\\rho = 1/2$ Le Cam, $\\rho = 1$ Neyman, et la
representation integrale
$t \\log t - t + 1 = \\int_{0}^{1} 2 f_{\\rho}(t) ( 1 - \\rho ) d\\rho$ .

PROPOSITION 4. En posant $v = w - 1$ on a
$f_{\\rho}(w) = \\frac{1}{2} \\frac{ v^{2} }{ \\rho v + 1 }$ , puis

    $f_{\\rho}'(w) = \\frac{ v ( \\rho v + 2 ) }{ 2 ( \\rho v + 1 )^{2} }$ ,
    $f_{\\rho}''(w) = \\frac{ 1 }{ ( \\rho v + 1 )^{3} }
     = \\frac{ 1 }{ ( \\rho w + 1 - \\rho )^{3} }$ .

Donc, pour $\\rho$ dans $[0, 1)$, $f_{\\rho}$ est strictement convexe sur
$[0, 1]$, sa fenetre est $[ - c(\\rho) , 0 ]$ avec

    $c(\\rho) = \\frac{ 2 - \\rho }{ 2 ( 1 - \\rho )^{2} }$ ,

et $g_{\\rho}$ vaut $1$ des que $e_{i} \\leq \\mu$ et $0$ des que
$e_{i} \\geq \\mu + \\epsilon c(\\rho)$ : SATURATION EXACTE des deux cotes.
On a $c(0) = 1$, $c(1/2) = 3$, $c(3/4) = 10$, et
$c(\\rho) \\to + \\infty$ quand $\\rho \\to 1^{-}$ ; le cas $\\rho = 1$
(Neyman) donne $g_{1}(u) = ( 1 - 2 u )^{-1/2}$ pour $u \\leq 0$ et
$g_{1}(u) = 1$ pour $u \\geq 0$, a support infini et a queue polynomiale
en $\\vert u \\vert^{-1/2}$. La forme abregee
$\\min ( 1 , ( 1 - 2 u )^{-1/2} )$ que l'on rencontre souvent n'a de sens
que pour $u < 1/2$ ; au-dela le radical est negatif, et c'est la branche
$g_{1} = 1$ qui vaut. Enfin $g_{0}(u) = \\min ( \\max ( u + 1 , 0 ) , 1 )$ :
c'est la RAMPE, a la translation de multiplicateur pres, car
$f_{0}(w) = w^{2}/2 - w + 1/2$ differe de $w^{2}/2$ d'une fonction affine
de pente $\\alpha = - 1$ (invariance affine du paragraphe 2), ce qui
translate le multiplicateur de $\\epsilon \\alpha = - \\epsilon$ sans
toucher $w$ : le niveau de Fermi de $f_{0}$ vaut celui de la rampe MOINS
$\\epsilon$. Le signe est grave dans une fixture du fichier de tests.

Reponse a la question posee : le noyau rampe correspond a $\\rho = 0$,
c'est-a-dire au chi-deux de PEARSON, et $\\rho = 0$ est le seul membre de
la famille dont $g$ est AFFINE sur sa fenetre (pour $\\rho > 0$ l'inversion
de $f_{\\rho}'$ est une equation du second degre en $w$).

Ce que la famille $f_{\\rho}$ apporte VRAIMENT ici, honnetement.

  * Chez Bach, le gain est reel et profond : la representation
    variationnelle
    $2 f_{\\rho}(t) = \\sup_{u} ( 2 ( t - 1 ) u - ( \\rho t + 1 - \\rho ) u^{2} )$
    est QUADRATIQUE en $u$, donc avec un modele lineaire
    $u(x) = \\theta^{\\top} \\phi(x)$ l'optimum est
    $\\theta(\\rho) = ( \\rho \\Sigma_{p} + ( 1 - \\rho ) \\Sigma_{q} )^{-1}
     ( m_{p} - m_{q} )$ ,
    et l'integration en $\\rho$ de la representation integrale de la
    divergence de Kullback-Leibler se resout par UNE SEULE decomposition
    en valeurs propres generalisees.

  * Chez nous, rien de cela ne se transporte : il n'y a pas de classe de
    fonctions ni de covariance, la variable duale est le SCALAIRE $\\mu$
    attache au point $y$, et il n'y a donc rien a diagonaliser. De plus le
    melange en $\\rho$ ne commute pas avec l'inversion : la derivee d'un
    melange est le melange des derivees, mais
    $( \\int f_{\\rho}' d\\nu )^{-1} \\neq \\int ( f_{\\rho}' )^{-1} d\\nu$ ,
    donc melanger les potentiels ne melange pas les noyaux.

  * Ce qui se transporte reellement : (i) $\\rho = 0$ donne exactement la
    rampe, donc l'arithmetique rationnelle exacte de `ramp_exact.py` ;
    (ii) au point $\\rho = 0$ la representation variationnelle se lit comme
    un moindre carre, et l'optimum est la PROJECTION EUCLIDIENNE
    $w = \\Pi_{[0,1]^{n}} ( ( \\mu - e ) / \\epsilon )$, ce qui est la raison
    structurelle pour laquelle tout reste rationnel ; (iii) pour
    l'encadrement demontre dans `ramp_exact.py`, seul compte le couple
    d'extremites de la fenetre, donc $\\rho$ n'agit que par la largeur
    $\\epsilon c(\\rho)$ : dans notre usage, $\\rho$ est une TEMPERATURE
    DEGUISEE, plus un changement de profil qui modifie la dynamique du
    gradient (les poids $g'$ du barycentre de coquille) mais pas
    l'encadrement.

Le lien est donc reel mais plus faible qu'il n'y parait : il tient au
membre $\\rho = 0$ et a la lecture "moindres carres / projection", pas a
la machinerie spectrale du billet.

Arithmetique. Ce module travaille en FLOTTANT : c'est un proposeur et un
banc de verification numerique. Les decisions certifiantes sont prises en
rationnels exacts par `ramp_exact.py`.
"""

import numpy as np
from scipy.optimize import linprog, minimize

FAMILY_NAMES = ("logistic", "ramp", "tsallis", "bach")

_BISECTION_STEPS = 100


class Family:
    """Famille de regularisation : potentiel `phi`, noyau `g` et derivees."""

    name = "abstract"

    def __init__(self, parameter=None):
        self.parameter = parameter

    def potential(self, weights):
        """Potentiel `phi` evalue sur un vecteur de poids de `[0, 1]`."""
        raise NotImplementedError

    def potential_slope(self, weights):
        """Derivee `phi'` sur `(0, 1)`."""
        raise NotImplementedError

    def potential_curvature(self, weights):
        """Derivee seconde `phi''` sur `(0, 1)`, strictement positive."""
        raise NotImplementedError

    def slope_bounds(self):
        """Fenetre `(phi'(0+), phi'(1-))` du noyau, bornes eventuelles infinies."""
        raise NotImplementedError

    def window_width(self):
        """Largeur de la fenetre, en unites de `epsilon` ; `inf` si non compacte."""
        low, high = self.slope_bounds()
        return high - low

    def occupancy(self, values):
        """Noyau `g` : inverse de `phi'` tronque a `[0, 1]`."""
        values = np.asarray(values, dtype=float)
        low, high = self.slope_bounds()
        result = np.where(values >= high, 1.0, 0.0)
        interior = (values > low) & (values < high)
        if np.any(interior):
            result[interior] = self._invert_slope(values[interior])
        return result

    def occupancy_slope(self, values):
        """Derivee `g'`, nulle hors de la fenetre, `1 / phi''(g(u))` dedans."""
        values = np.asarray(values, dtype=float)
        low, high = self.slope_bounds()
        result = np.zeros_like(values)
        interior = (values > low) & (values < high)
        if np.any(interior):
            inside = self._invert_slope(values[interior])
            result[interior] = 1.0 / self.potential_curvature(inside)
        return result

    def _invert_slope(self, values):
        """Inversion monotone de `phi'` par dichotomie vectorisee sur `(0, 1)`."""
        lower = np.zeros_like(values)
        upper = np.ones_like(values)
        for _step in range(_BISECTION_STEPS):
            middle = 0.5 * (lower + upper)
            below = self.potential_slope(middle) < values
            lower = np.where(below, middle, lower)
            upper = np.where(below, upper, middle)
        return 0.5 * (lower + upper)


class Logistic(Family):
    """Entropie de Fermi-Dirac : noyau logistique, fenetre infinie."""

    name = "logistic"

    def potential(self, weights):
        weights = np.asarray(weights, dtype=float)
        safe = np.clip(weights, 1e-300, 1.0 - 1e-16)
        value = safe * np.log(safe) + (1.0 - safe) * np.log(1.0 - safe)
        return np.where((weights <= 0.0) | (weights >= 1.0), 0.0, value)

    def potential_slope(self, weights):
        weights = np.asarray(weights, dtype=float)
        safe = np.clip(weights, 1e-300, 1.0 - 1e-16)
        return np.log(safe) - np.log1p(-safe)

    def potential_curvature(self, weights):
        weights = np.asarray(weights, dtype=float)
        safe = np.clip(weights, 1e-300, 1.0 - 1e-16)
        return 1.0 / safe + 1.0 / (1.0 - safe)

    def slope_bounds(self):
        return (-np.inf, np.inf)

    def occupancy(self, values):
        values = np.asarray(values, dtype=float)
        return 1.0 / (1.0 + np.exp(-np.clip(values, -700.0, 700.0)))

    def occupancy_slope(self, values):
        weights = self.occupancy(values)
        return weights * (1.0 - weights)


class Ramp(Family):
    """Chi-deux de Pearson : noyau rampe, fenetre `[0, 1]`, profil affine."""

    name = "ramp"

    def potential(self, weights):
        weights = np.asarray(weights, dtype=float)
        return 0.5 * weights * weights

    def potential_slope(self, weights):
        return np.asarray(weights, dtype=float)

    def potential_curvature(self, weights):
        return np.ones_like(np.asarray(weights, dtype=float))

    def slope_bounds(self):
        return (0.0, 1.0)

    def occupancy(self, values):
        return np.clip(np.asarray(values, dtype=float), 0.0, 1.0)

    def occupancy_slope(self, values):
        values = np.asarray(values, dtype=float)
        return ((values > 0.0) & (values < 1.0)).astype(float)


class Tsallis(Family):
    """Tsallis d'exposant `alpha > 1` : noyau en puissance, fenetre compacte."""

    name = "tsallis"

    def __init__(self, parameter=2.0):
        if parameter is None or float(parameter) <= 1.0:
            raise ValueError("Tsallis exige alpha > 1")
        Family.__init__(self, float(parameter))

    def potential(self, weights):
        weights = np.asarray(weights, dtype=float)
        alpha = self.parameter
        return (np.power(np.clip(weights, 0.0, 1.0), alpha) - weights) / (alpha - 1.0)

    def potential_slope(self, weights):
        weights = np.asarray(weights, dtype=float)
        alpha = self.parameter
        power = np.power(np.clip(weights, 0.0, 1.0), alpha - 1.0)
        return (alpha * power - 1.0) / (alpha - 1.0)

    def potential_curvature(self, weights):
        weights = np.asarray(weights, dtype=float)
        alpha = self.parameter
        safe = np.clip(weights, 1e-300, 1.0)
        return alpha * np.power(safe, alpha - 2.0)

    def slope_bounds(self):
        alpha = self.parameter
        return (-1.0 / (alpha - 1.0), 1.0)

    def occupancy(self, values):
        values = np.asarray(values, dtype=float)
        alpha = self.parameter
        base = ((alpha - 1.0) * values + 1.0) / alpha
        base = np.clip(base, 0.0, None)
        return np.clip(np.power(base, 1.0 / (alpha - 1.0)), 0.0, 1.0)


class Bach(Family):
    """Chi-deux pondere `f_rho` de Bach ; `rho = 0` redonne la rampe translatee."""

    name = "bach"

    def __init__(self, parameter=0.0):
        value = 0.0 if parameter is None else float(parameter)
        if not 0.0 <= value < 1.0:
            raise ValueError("Bach exige 0 <= rho < 1 (rho = 1 a fenetre infinie)")
        Family.__init__(self, value)

    def _denominator(self, weights):
        return self.parameter * np.asarray(weights, dtype=float) + 1.0 - self.parameter

    def potential(self, weights):
        weights = np.asarray(weights, dtype=float)
        return 0.5 * (weights - 1.0) ** 2 / self._denominator(weights)

    def potential_slope(self, weights):
        weights = np.asarray(weights, dtype=float)
        shifted = weights - 1.0
        return shifted * (self.parameter * shifted + 2.0) / (2.0 * self._denominator(weights) ** 2)

    def potential_curvature(self, weights):
        return 1.0 / self._denominator(weights) ** 3

    def slope_bounds(self):
        rho = self.parameter
        return (-(2.0 - rho) / (2.0 * (1.0 - rho) ** 2), 0.0)

    def occupancy(self, values):
        if self.parameter == 0.0:
            return np.clip(np.asarray(values, dtype=float) + 1.0, 0.0, 1.0)
        return Family.occupancy(self, values)


def family(name, parameter=None):
    """Construit la famille demandee : `logistic`, `ramp`, `tsallis`, `bach`."""
    if name == "logistic":
        return Logistic()
    if name == "ramp":
        return Ramp()
    if name == "tsallis":
        return Tsallis(2.0 if parameter is None else parameter)
    if name == "bach":
        return Bach(0.0 if parameter is None else parameter)
    raise ValueError("famille inconnue : " + str(name))


def support_width(name, parameter=None):
    """Largeur `c` de la fenetre : la transition occupe `epsilon * c` en energie."""
    return family(name, parameter).window_width()


def occupancy(values, name="ramp", parameter=None):
    """Noyau `g` de la famille demandee, vectorise."""
    return family(name, parameter).occupancy(values)


def occupancy_slope(values, name="ramp", parameter=None):
    """Derivee `g'` de la famille demandee, vectorisee."""
    return family(name, parameter).occupancy_slope(values)


def soft_count(energies, level, epsilon, name="ramp", parameter=None):
    """Comptage doux `somme_i g((level - e_i) / epsilon)`."""
    energies = np.asarray(energies, dtype=float)
    return float(np.sum(occupancy((level - energies) / epsilon, name, parameter)))


def fermi_level(energies, mass, epsilon, name="ramp", parameter=None, steps=200):
    """Niveau de Fermi : plus petit `mu` dont le comptage doux atteint `mass`.

    La convention est l'EXTREMITE GAUCHE de l'intervalle des multiplicateurs
    (proposition 2), obtenue par dichotomie sur le predicat monotone
    `comptage >= mass`.
    """
    energies = np.asarray(energies, dtype=float)
    count = energies.size
    mass = float(mass)
    if not 0.0 < mass <= count:
        raise ValueError("masse hors de (0, n]")
    low = float(energies.min()) - abs(epsilon)
    high = float(energies.max()) + abs(epsilon)
    span = max(1.0, high - low)
    while soft_count(energies, low, epsilon, name, parameter) >= mass:
        low -= span
        span *= 2.0
    span = max(1.0, high - low)
    while soft_count(energies, high, epsilon, name, parameter) < mass:
        high += span
        span *= 2.0
    for _step in range(steps):
        middle = 0.5 * (low + high)
        if soft_count(energies, middle, epsilon, name, parameter) < mass:
            low = middle
        else:
            high = middle
        if high - low <= 1e-15 * max(1.0, abs(high)):
            break
    return 0.5 * (low + high)


def regularised_weights(energies, mass, epsilon, name="ramp", parameter=None):
    """Forme fermee de la proposition 2 : renvoie `(poids, niveau)`."""
    energies = np.asarray(energies, dtype=float)
    level = fermi_level(energies, mass, epsilon, name, parameter)
    weights = occupancy((level - energies) / epsilon, name, parameter)
    return weights, level


def objective_value(weights, energies, epsilon, name="ramp", parameter=None):
    """Valeur `somme_i w_i e_i + epsilon somme_i phi(w_i)`."""
    weights = np.asarray(weights, dtype=float)
    energies = np.asarray(energies, dtype=float)
    return float(weights @ energies + epsilon * np.sum(family(name, parameter).potential(weights)))


def reference_weights(energies, mass, epsilon, name="ramp", parameter=None, margin=1e-9):
    """Juge independant : optimum du programme regularise par `scipy`.

    Aucune forme fermee n'est utilisee ici : seuls l'objectif et son
    gradient sont fournis, et `scipy` resout le programme sous contraintes.
    """
    energies = np.asarray(energies, dtype=float)
    count = energies.size
    handle = family(name, parameter)
    low, high = handle.slope_bounds()
    edge = margin if not np.isfinite(low) or not np.isfinite(high) else 0.0

    def cost(weights):
        return float(weights @ energies + epsilon * np.sum(handle.potential(weights)))

    def gradient(weights):
        return energies + epsilon * handle.potential_slope(weights)

    start = np.full(count, float(mass) / count)
    constraint = {
        "type": "eq",
        "fun": lambda weights: float(np.sum(weights) - mass),
        "jac": lambda weights: np.ones(count),
    }
    outcome = minimize(
        cost,
        start,
        jac=gradient,
        bounds=[(edge, 1.0 - edge)] * count,
        constraints=[constraint],
        method="SLSQP",
        options={"maxiter": 2000, "ftol": 1e-14},
    )
    return np.clip(outcome.x, 0.0, 1.0), float(outcome.fun), bool(outcome.success)


def linear_program_value(energies, mass):
    """Juge independant du point 1 : valeur du programme lineaire par `scipy`."""
    energies = np.asarray(energies, dtype=float)
    count = energies.size
    outcome = linprog(
        energies,
        A_eq=np.ones((1, count)),
        b_eq=[float(mass)],
        bounds=[(0.0, 1.0)] * count,
        method="highs",
    )
    return float(outcome.fun), np.asarray(outcome.x, dtype=float), bool(outcome.success)


def smallest_sum(energies, order):
    """Somme des `order` plus petites energies : valeur annoncee du point 1."""
    energies = np.sort(np.asarray(energies, dtype=float))
    return float(np.sum(energies[:order]))
