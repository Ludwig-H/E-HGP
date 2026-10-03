#!/usr/bin/env python3
"""Sommes exactes de racines carrees de rationnels : x = sum c_i sqrt(q_i), c_i et q_i dans Q, q_i >= 0.

Code independant de vrad.py (v10), meme mathematique :
  - regroupement des termes dont les radicandes ont un rapport carre de rationnel ;
  - apres regroupement, les racines de classes distinctes sont lineairement independantes sur Q
    (theoreme classique : sqrt(s_1), ..., sqrt(s_m), s_i sans facteur carre distincts, sont Q-libres),
    donc la somme est nulle si et seulement si tous les coefficients regroupes sont nuls ;
  - sinon le signe est decide par encadrement entier (isqrt) a precision croissante : terminaison garantie.
Aucun flottant dans une decision ; aucun assert (identique sous python3 -O).
"""
from fractions import Fraction
from math import isqrt


class SurdError(RuntimeError):
    pass


def fr(v):
    if isinstance(v, Fraction):
        return v
    if isinstance(v, int) and not isinstance(v, bool):
        return Fraction(v)
    raise SurdError('rationnel exact attendu : %r' % (v,))


def rational_sqrt(q):
    """Racine rationnelle de q >= 0 si q est un carre de rationnel, sinon None."""
    q = fr(q)
    if q < 0:
        return None
    a, b = isqrt(q.numerator), isqrt(q.denominator)
    if a * a == q.numerator and b * b == q.denominator:
        return Fraction(a, b)
    return None


class Surd(object):
    __slots__ = ('t',)

    def __init__(self, terms=None):
        reps = []
        for q, c in (terms or {}).items():
            q, c = fr(q), fr(c)
            if q < 0:
                raise SurdError('radicande negatif')
            if q == 0 or c == 0:
                continue
            for i, (rep, cr) in enumerate(reps):
                s = rational_sqrt(q / rep)
                if s is not None:
                    reps[i] = (rep, cr + c * s)
                    break
            else:
                reps.append((q, c))
        self.t = dict((rep, c) for rep, c in reps if c != 0)

    @staticmethod
    def sqrt(q):
        q = fr(q)
        s = rational_sqrt(q)
        if s is not None:
            return Surd({Fraction(1): s})
        return Surd({q: Fraction(1)})

    @staticmethod
    def rat(x):
        return Surd({Fraction(1): fr(x)})

    def __add__(self, o):
        o = o if isinstance(o, Surd) else Surd.rat(o)
        t = dict(self.t)
        for q, c in o.t.items():
            t[q] = t.get(q, Fraction(0)) + c
        return Surd(t)

    def __neg__(self):
        return Surd(dict((q, -c) for q, c in self.t.items()))

    def __sub__(self, o):
        o = o if isinstance(o, Surd) else Surd.rat(o)
        return self + (-o)

    def scale(self, k):
        k = fr(k)
        return Surd(dict((q, c * k) for q, c in self.t.items()))

    def square(self):
        """Carre exact (rend un Surd ; les produits croises sqrt(q1 q2) restent exacts)."""
        out = {}
        items = list(self.t.items())
        for i, (q1, c1) in enumerate(items):
            for j, (q2, c2) in enumerate(items):
                q = q1 * q2
                s = rational_sqrt(q)
                if s is not None:
                    out[Fraction(1)] = out.get(Fraction(1), Fraction(0)) + c1 * c2 * s
                else:
                    out[q] = out.get(q, Fraction(0)) + c1 * c2
        return Surd(out)

    def sign(self):
        if not self.t:
            return 0
        if len(self.t) == 1:
            (_q, c), = self.t.items()
            return 1 if c > 0 else -1
        k = 80
        while k <= 1 << 22:
            lo = hi = Fraction(0)
            for q, c in self.t.items():
                # sqrt(q) = sqrt(num * den) / den ; encadrement de sqrt(num * den * 4^k) par isqrt
                X = q.numerator * q.denominator << (2 * k)
                s = isqrt(X)
                den = q.denominator << k
                a, b = Fraction(s, den) * c, Fraction(s + 1, den) * c
                if c > 0:
                    lo, hi = lo + a, hi + b
                else:
                    lo, hi = lo + b, hi + a
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            k *= 2
        raise SurdError('signe non decide')

    def cmp(self, o):
        return (self - o).sign()

    def bounds(self, k=200):
        """Encadrement rationnel certifie [lo, hi] a 2^-k pres par terme."""
        lo = hi = Fraction(0)
        for q, c in self.t.items():
            X = q.numerator * q.denominator << (2 * k)
            s = isqrt(X)
            den = q.denominator << k
            a, b = Fraction(s, den) * c, Fraction(s + 1, den) * c
            if c > 0:
                lo, hi = lo + a, hi + b
            else:
                lo, hi = lo + b, hi + a
        return lo, hi

    def __float__(self):
        lo, hi = self.bounds(80)
        return float((lo + hi) / 2)

    def rational(self):
        """Valeur rationnelle si le nombre est rationnel, sinon None."""
        if not self.t:
            return Fraction(0)
        if len(self.t) == 1 and Fraction(1) in self.t:
            return self.t[Fraction(1)]
        return None

    def text(self):
        if not self.t:
            return '0'
        return ' + '.join(('%s' % c) if q == 1 else ('%s*sqrt(%s)' % (c, q)) for q, c in sorted(self.t.items()))


def smax(xs):
    m = None
    for x in xs:
        if m is None or x.cmp(m) > 0:
            m = x
    return m


def selftest():
    a = Surd.sqrt(2) + Surd.sqrt(8)          # 3 sqrt 2
    b = Surd.sqrt(18)
    if a.cmp(b) != 0:
        raise SurdError('3 sqrt 2')
    c = Surd.sqrt(2) + Surd.sqrt(3) - Surd.sqrt(Fraction(49, 5))   # 3,146 - 3,130 > 0
    if c.sign() != 1:
        raise SurdError('signe c')
    d = (Surd.sqrt(3) - Surd.rat(1)).square() - (Surd.rat(4) - Surd.sqrt(12))
    if d.sign() != 0:
        raise SurdError('carre')
    return True


if __name__ == '__main__':
    print('selftest', selftest())
