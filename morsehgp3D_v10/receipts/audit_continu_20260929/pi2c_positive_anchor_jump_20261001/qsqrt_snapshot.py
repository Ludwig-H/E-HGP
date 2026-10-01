"""Nombres exacts de la forme  sum_i q_i * sqrt(a_i),  q_i et a_i rationnels, a_i >= 0.

Sert aux dates d'entree et aux hauteurs de reunion EN RAYON (le rayon est la racine d'un niveau exact, rayon carre
rationnel). Aucune decision n'est prise en flottant.

Decision de signe :
  1. regroupement par classe de carres : sqrt(a) et sqrt(b) sont Q-proportionnelles ssi a/b est le carre d'un
     rationnel (test entier exact : numerateur et denominateur reduits carres parfaits) ;
  2. apres regroupement, les racines restantes ont des parties sans facteur carre distinctes ; elles sont lineairement
     independantes sur Q (Besicovitch, J. London Math. Soc. 15 (1940) 3-6). Le nombre est donc nul ssi tous les
     coefficients de groupe sont nuls ;
  3. sinon le signe est decide par encadrement entier (isqrt) a precision croissante, qui termine puisque le nombre
     est non nul.

Bibliotheque standard seulement ; aucune verification par assert (comportement identique sous python3 -O).
"""
from fractions import Fraction
from math import isqrt


class QSError(ValueError):
    pass


def _frac(v):
    if isinstance(v, bool):
        raise QSError('booleen refuse')
    if isinstance(v, int):
        return Fraction(v)
    if isinstance(v, Fraction):
        return v
    raise QSError('rationnel exact attendu, recu %r' % (v,))


def _is_square_int(n):
    if n < 0:
        return False
    r = isqrt(n)
    return r * r == n


_RSQRT_CACHE = {}
_RATIO_CACHE = {}


def rational_sqrt(a):
    """sqrt(a) si a est le carre d'un rationnel, sinon None (memoise ; decision entiere exacte)."""
    a = _frac(a)
    hit = _RSQRT_CACHE.get(a, 0)
    if hit != 0:
        return hit
    if a < 0:
        raise QSError('racine d un negatif')
    p, q = a.numerator, a.denominator
    if _is_square_int(p) and _is_square_int(q):
        out = Fraction(isqrt(p), isqrt(q))
    else:
        out = None
    if len(_RSQRT_CACHE) > 2000000:
        _RSQRT_CACHE.clear()
    _RSQRT_CACHE[a] = out
    return out


def _ratio_sqrt(a, g):
    """sqrt(a / g) si rationnel, sinon None (memoise par couple)."""
    key = (a, g)
    hit = _RATIO_CACHE.get(key, 0)
    if hit != 0:
        return hit
    out = rational_sqrt(a / g)
    if len(_RATIO_CACHE) > 2000000:
        _RATIO_CACHE.clear()
    _RATIO_CACHE[key] = out
    return out


class QS:
    """Somme exacte de racines carrees de rationnels. Immuable. `terms` : dict radicande -> coefficient."""
    __slots__ = ('terms',)

    def __init__(self, terms=None):
        t = {}
        if terms:
            for a, q in terms.items():
                a, q = _frac(a), _frac(q)
                if a < 0:
                    raise QSError('radicande negatif')
                if q == 0 or a == 0:
                    continue
                r = rational_sqrt(a)
                if r is not None:
                    a, q = Fraction(1), q * r
                t[a] = t.get(a, Fraction(0)) + q
        self.terms = {a: q for a, q in t.items() if q != 0}

    # -- constructeurs
    @classmethod
    def _raw(cls, terms):
        """Termes deja normalises (radicandes > 0 non carres ou 1, coefficients non nuls)."""
        obj = cls.__new__(cls)
        obj.terms = terms
        return obj

    @staticmethod
    def rat(v):
        v = _frac(v)
        return QS._raw({Fraction(1): v}) if v != 0 else QS._raw({})

    @staticmethod
    def sqrt(a):
        a = _frac(a)
        if a < 0:
            raise QSError('racine d un negatif')
        return QS({a: Fraction(1)})

    # -- arithmetique
    def __add__(self, other):
        other = _coerce(other)
        t = dict(self.terms)
        for a, q in other.terms.items():
            v = t.get(a, 0) + q
            if v == 0:
                t.pop(a, None)
            else:
                t[a] = v
        return QS._raw(t)

    __radd__ = __add__

    def __neg__(self):
        return QS._raw({a: -q for a, q in self.terms.items()})

    def __sub__(self, other):
        other = _coerce(other)
        t = dict(self.terms)
        for a, q in other.terms.items():
            v = t.get(a, 0) - q
            if v == 0:
                t.pop(a, None)
            else:
                t[a] = v
        return QS._raw(t)

    def __rsub__(self, other):
        return _coerce(other) - self

    def __mul__(self, c):
        c = _frac(c)
        if c == 0:
            return QS._raw({})
        return QS._raw({a: q * c for a, q in self.terms.items()})

    __rmul__ = __mul__

    # -- signe exact
    def _groups(self):
        groups = []  # [representant, coefficient rationnel]
        for a, q in self.terms.items():
            for g in groups:
                r = _ratio_sqrt(a, g[0])
                if r is not None:
                    g[1] += q * r
                    break
            else:
                groups.append([a, q])
        return [(g, c) for g, c in groups if c != 0]

    def sign(self):
        groups = self._groups()
        if not groups:
            return 0
        if len(groups) == 1:
            return 1 if groups[0][1] > 0 else -1
        k = 64
        while True:
            # encadrement entier de 2^k * valeur : chaque terme c sqrt(n/d) = (cn/cd) sqrt(n d) / d
            lo = 0
            hi = 0
            scale = 1 << k
            for a, c in groups:
                n, d = a.numerator, a.denominator
                cn, cd = c.numerator, c.denominator
                s = isqrt(n * d * scale * scale)  # s <= 2^k sqrt(n d) < s + 1
                den = cd * d
                if cn > 0:
                    lo += (cn * s) // den                      # plancher de la borne basse
                    hi += -((-(cn * (s + 1))) // den)          # plafond de la borne haute
                else:
                    lo += (cn * (s + 1)) // den                # cn < 0 : borne basse avec s + 1
                    hi += -((-(cn * s)) // den)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            k *= 2
            if k > 1 << 16:
                raise QSError('precision non convergente (nombre non nul attendu)')

    def is_zero(self):
        return not self._groups()

    # -- comparaisons
    def cmp(self, other):
        return (self - _coerce(other)).sign()

    def __lt__(self, other):
        return self.cmp(other) < 0

    def __le__(self, other):
        return self.cmp(other) <= 0

    def __gt__(self, other):
        return self.cmp(other) > 0

    def __ge__(self, other):
        return self.cmp(other) >= 0

    def __eq__(self, other):
        try:
            return self.cmp(other) == 0
        except QSError:
            return False

    def __hash__(self):
        raise TypeError('QS non hachable (egalite algebrique) ; utiliser key()')

    def __float__(self):
        return float(sum(float(q) * float(a) ** 0.5 for a, q in self.terms.items()))

    def __repr__(self):
        if not self.terms:
            return 'QS(0)'
        parts = []
        for a, q in sorted(self.terms.items()):
            parts.append('%s' % q if a == 1 else '%s*sqrt(%s)' % (q, a))
        return 'QS(' + ' + '.join(parts) + ')'

    def text(self):
        """Forme exacte lisible, stable (tri des radicandes)."""
        if not self.terms:
            return '0'
        parts = []
        for a, q in sorted(self.terms.items()):
            parts.append(str(q) if a == 1 else '%s*sqrt(%s)' % (q, a))
        return ' + '.join(parts)

    def as_rational(self):
        """Valeur rationnelle si le nombre est rationnel (apres regroupement), sinon None."""
        groups = self._groups()
        if not groups:
            return Fraction(0)
        if len(groups) == 1:
            a, c = groups[0]
            r = rational_sqrt(a)
            if r is not None:
                return c * r
        return None


def _coerce(v):
    if isinstance(v, QS):
        return v
    return QS.rat(v)


def qmax(*values):
    best = None
    for v in values:
        v = _coerce(v)
        if best is None or v > best:
            best = v
    return best


def qmin(*values):
    best = None
    for v in values:
        v = _coerce(v)
        if best is None or v < best:
            best = v
    return best


def radius(beta):
    """Rayon exact d'un niveau (rayon carre) rationnel."""
    return QS.sqrt(_frac(beta))


def selftest(rounds=400, seed=20260930):
    """Auto-controle : identites algebriques exactes et comparaison a Decimal 120 chiffres loin des egalites."""
    import random
    from decimal import Decimal, getcontext
    getcontext().prec = 120
    rng = random.Random(seed)
    checks = 0

    def need(cond, msg):
        if not cond:
            raise QSError('autotest : ' + msg)

    need(QS.sqrt(8) == 2 * QS.sqrt(2), 'sqrt8')
    need((QS.sqrt(Fraction(1, 2)) - QS.sqrt(2) * Fraction(1, 2)).is_zero(), 'sqrt1/2')
    need(QS.sqrt(2) + QS.sqrt(3) > QS.sqrt(Fraction(98, 10)), 'sqrt2+sqrt3>sqrt9.8')
    need((QS.sqrt(2) + QS.sqrt(3) - QS.sqrt(5 + 2 * 6 ** 0 * 0 + 0)).sign() == 1, 'trivial')
    need((QS.sqrt(12) - QS.sqrt(3) - QS.sqrt(3)).is_zero(), 'sqrt12')
    need(QS.sqrt(Fraction(9, 4)).as_rational() == Fraction(3, 2), 'rationnel')
    need(QS.sqrt(Fraction(3249, 89)) > 6, '3249/89')
    checks += 7
    for _ in range(rounds):
        k = rng.randint(1, 5)
        terms = {}
        for _j in range(k):
            a = Fraction(rng.randint(0, 50), rng.randint(1, 12))
            q = Fraction(rng.randint(-9, 9), rng.randint(1, 7))
            terms[a] = terms.get(a, Fraction(0)) + q
        x = QS(terms)
        dec = sum(Decimal(q.numerator) / Decimal(q.denominator) *
                  (Decimal(a.numerator) / Decimal(a.denominator)).sqrt() for a, q in x.terms.items())
        s = x.sign()
        if abs(dec) > Decimal(10) ** -60:
            need(s == (1 if dec > 0 else -1), 'signe different de Decimal pour %r' % (x,))
        else:
            need(s == 0, 'quasi-zero non nul exact %r' % (x,))
        # identite : x - x = 0 et (x + y) - y = x
        y = QS({Fraction(rng.randint(1, 30), rng.randint(1, 5)): Fraction(rng.randint(-5, 5))})
        need(((x + y) - y - x).is_zero(), 'associativite')
        checks += 2
    return checks


if __name__ == '__main__':
    import json
    import sys
    n = selftest()
    print(json.dumps({'status': 'PASS', 'checks': n, 'optimize': sys.flags.optimize}))
