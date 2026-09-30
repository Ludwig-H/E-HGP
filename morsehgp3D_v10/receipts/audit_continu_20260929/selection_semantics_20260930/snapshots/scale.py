"""Echelle phi(beta) = beta^(-q) (= r^(-2q)), decroissante, phi(infini) = 0.

q entier : Fraction exacte, aucune erreur. q demi-entier (z = 2q impair) : Decimal a PREC chiffres, et toute
comparaison exige une marge relative > TOL, sinon `Ambiguous` est leve (decision certifiee par marge, jamais
tranchee au hasard). Les deux regimes ne sont jamais melanges dans un meme calcul.
"""
from decimal import Decimal, getcontext, localcontext
from fractions import Fraction

PREC = 110
TOL = Decimal(10) ** -80
# toutes les operations Decimal du processus (sommes, produits hors des contextes locaux) a PREC chiffres
getcontext().prec = PREC


class Ambiguous(RuntimeError):
    """Comparaison non certifiable a la precision declaree."""


class Phi:
    def __init__(self, q, force_decimal=False):
        q = Fraction(q)
        if q <= 0 or (2 * q).denominator != 1:
            raise ValueError('q doit etre un entier ou un demi-entier positif')
        self.q = q
        self.exact = q.denominator == 1 and not force_decimal

    def __repr__(self):
        return 'Phi(q=%s, z=%s)' % (self.q, 2 * self.q)

    def num(self, v):
        """Conversion d'une valeur exacte dans le regime de calcul."""
        if self.exact:
            return Fraction(v)
        with localcontext() as ctx:
            ctx.prec = PREC
            v = Fraction(v)
            return Decimal(v.numerator) / Decimal(v.denominator)

    def zero(self):
        return Fraction(0) if self.exact else Decimal(0)

    def __call__(self, beta):
        """phi(beta) ; beta = None signifie +infini (phi = 0). Valeurs memorisees par niveau exact."""
        if beta is None:
            return self.zero()
        cache = self.__dict__.setdefault('_cache', {})
        hit = cache.get(beta)
        if hit is not None:
            return hit
        val = self._compute(beta)
        cache[beta] = val
        return val

    def _compute(self, beta):
        beta = Fraction(beta)
        if beta <= 0:
            raise ValueError('phi(0) infini : hors domaine (K >= 2 donne des niveaux > 0)')
        if self.exact:
            return (1 / beta) ** int(self.q)
        with localcontext() as ctx:
            ctx.prec = PREC
            inv = Decimal(beta.denominator) / Decimal(beta.numerator)
            if self.q.denominator == 1:
                return inv ** int(self.q)
            k = int(self.q - Fraction(1, 2))
            return (inv ** k) * inv.sqrt()

    def inverse(self, value):
        """beta tel que phi(beta) = value (value > 0) ; rendu en float pour l'affichage seulement."""
        return float(value) ** (-1.0 / float(self.q))


SOFT_TIES = [0]
SMALL = Decimal(10) ** -60


def cmp(a, b, soft=False):
    """Signe de a - b ; exact pour Fraction, certifie par marge pour Decimal.

    soft=True : une difference sous la tolerance (Decimal) est rendue comme une egalite (0) et comptee dans
    SOFT_TIES ; a n'utiliser que la ou les deux issues donnent la meme date (egalite a la mort d'une branche)."""
    if isinstance(a, Fraction) and isinstance(b, Fraction):
        return (a > b) - (a < b)
    if isinstance(a, Decimal) and isinstance(b, Decimal):
        d = a - b
        if abs(d) <= TOL * (abs(a) + abs(b) + SMALL):
            if soft:
                SOFT_TIES[0] += 1
                return 0
            raise Ambiguous('comparaison non certifiable : %s vs %s' % (a, b))
        return 1 if d > 0 else -1
    with localcontext() as ctx:
        ctx.prec = PREC
        a = a if isinstance(a, Decimal) else Decimal(a.numerator) / Decimal(a.denominator)
        b = b if isinstance(b, Decimal) else Decimal(b.numerator) / Decimal(b.denominator)
        d = a - b
        scale = abs(a) + abs(b) + Decimal(1) * (Decimal(10) ** -60)
        if abs(d) <= TOL * scale:
            if soft:
                SOFT_TIES[0] += 1
                return 0
            raise Ambiguous('comparaison non certifiable : %s vs %s' % (a, b))
        return 1 if d > 0 else -1


def fl(v):
    """Affichage flottant (jamais une decision)."""
    return None if v is None else float(v)
