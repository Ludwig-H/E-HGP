"""Vote sur la tour condensee, chemin rapide v2 : arithmetique EXACTE de repli et bornes flottantes prouvees.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, mode=benchmark_only, public_status=not_claimed.
Bibliotheque standard seulement (plus numpy pour psi_flottant sur tableaux). Aucun assert (identique sous -O).

1. Racines : nombres exacts de la forme  somme_i c_i sqrt(a_i),  c_i rationnels, a_i rationnels > 0 (a = 1 : partie
   rationnelle). Clos par addition, produit par un rationnel, produit par sqrt(q) et produit entre eux
   (sqrt(a) sqrt(b) = sqrt(a b)). C'est la classe des poids r^(-z) (z impair : l^(-(z+1)/2) sqrt(l)), des sommes de
   votes, et des rayons differes multiplies par leur denominateur.
   Signe (Racines.signe) :
     a. encadrement ENTIER a P bits (isqrt) : pour a = p/q et S >= 0, L = isqrt(floor(p 4^S / q)) verifie
        L <= 2^S sqrt(a) <= L + 1 ; chaque terme c sqrt(a) est encadre par des divisions entieres arrondies vers le
        bas et vers le haut ; la somme des bornes encadre le nombre. Si l'intervalle exclut 0, le signe est prouve ;
     b. sinon regroupement par classes de carres : sqrt(a) et sqrt(g) sont proportionnelles sur Q ssi a / g est le
        carre d'un rationnel (test entier exact sur la fraction reduite). Les representants de classes distinctes
        ont des parties sans facteur carre distinctes : leurs racines sont lineairement independantes sur Q
        (Besicovitch, J. London Math. Soc. 15 (1940) 3-6). Le nombre est donc nul ssi tous les coefficients de
        classe sont nuls ;
     c. s'il est non nul, l'encadrement a precision doublee termine.
   Meme theoreme que v10-verrou-points/ancrage_marges/qsqrt.py (reference exacte) ; code reecrit ici pour que le
   chemin rapide n'importe pas la bibliotheque des regles.

2. psi_flottant(v, z) : v^(-z/2) par operations IEEE correctement arrondies seulement (division, racine, produit ;
   pas de pow). Si v = l (1 + d0), |d0| <= u = 2^-53 :
     z = 2k   : p = fl(1 / v) puis k - 1 produits : erreur relative <= (1 + u)^(2k) - 1 + ... <= 1,5 z u (1 + 2^-40) ;
     z = 2k+1 : t = fl(1 / fl(sqrt(v))) puis k produits par p : <= (z / 2 + 2 + z) u (1 + 2^-40).
   BORNE_PSI(z) = (4 z + 4) u majore les deux (facteur de securite >= 2).
"""
import decimal
from fractions import Fraction
from math import isqrt, sqrt

U = 2.0 ** -53
UN = Fraction(1)


class CertifErreur(RuntimeError):
    """Invariant viole (jamais un assert)."""


def exiger(cond, msg):
    if not cond:
        raise CertifErreur(msg)


def borne_psi(z):
    """Borne d'erreur RELATIVE de psi_flottant(fl(l), z) par rapport a l^(-z/2) exact."""
    return (4 * z + 4) * U


def psi_flottant(v, z, racine=sqrt):
    """v^(-z/2), v flottant > 0 (scalaire, ou tableau numpy avec racine=np.sqrt), z entier >= 1."""
    p = 1.0 / v
    k = z // 2
    if z % 2:
        w = 1.0 / racine(v)
        for _ in range(k):
            w = w * p
        return w
    w = p
    for _ in range(k - 1):
        w = w * p
    return w


_CARRES = {}


def racine_rationnelle(a):
    """sqrt(a) si a (Fraction >= 0) est le carre d'un rationnel, sinon None (test entier exact, memoise)."""
    r = _CARRES.get(a, 0)
    if r != 0:
        return r
    p, q = a.numerator, a.denominator
    rp, rq = isqrt(p), isqrt(q)
    r = Fraction(rp, rq) if rp * rp == p and rq * rq == q else None
    if len(_CARRES) > 500000:
        _CARRES.clear()
    _CARRES[a] = r
    return r


def _bits(x):
    return abs(x).bit_length()


class Racines:
    """somme c_i sqrt(a_i) exacte. t : {radicande (Fraction > 0, non carre, ou 1) : coefficient (Fraction non nul)}."""
    __slots__ = ('t',)

    def __init__(self, t=None):
        self.t = {} if t is None else t

    @staticmethod
    def rat(q):
        q = Fraction(q)
        return Racines({UN: q} if q else {})

    @staticmethod
    def racine(a, c=1):
        out = Racines()
        out.ajoute(Fraction(a), Fraction(c))
        return out

    def ajoute(self, a, c):
        """self += c sqrt(a) (en place)."""
        if not c:
            return
        exiger(a >= 0, 'radicande negatif')
        if a == 0:
            return
        if a != UN:
            r = racine_rationnelle(a)
            if r is not None:
                a, c = UN, c * r
        v = self.t.get(a, 0) + c
        if v:
            self.t[a] = v
        else:
            self.t.pop(a, None)

    def __add__(self, o):
        out = Racines(dict(self.t))
        for a, c in o.t.items():
            out.ajoute(a, c)
        return out

    def __sub__(self, o):
        out = Racines(dict(self.t))
        for a, c in o.t.items():
            out.ajoute(a, -c)
        return out

    def fois(self, q):
        q = Fraction(q)
        return Racines({a: c * q for a, c in self.t.items()} if q else {})

    def fois_racine(self, q):
        """self * sqrt(q), q rationnel >= 0."""
        q = Fraction(q)
        out = Racines()
        for a, c in self.t.items():
            out.ajoute(a * q, c)
        return out

    def produit(self, o):
        out = Racines()
        for a, c in self.t.items():
            for b, d in o.t.items():
                out.ajoute(a * b, c * d)
        return out

    def rationnel(self):
        """La valeur si elle est rationnelle par construction (seule la partie de radicande 1), sinon None."""
        if not self.t:
            return Fraction(0)
        if len(self.t) == 1 and UN in self.t:
            return self.t[UN]
        return None

    def __float__(self):
        return sum(float(c) * sqrt(float(a)) for a, c in self.t.items())

    def decimal(self, chiffres=60):
        """Valeur approchee a `chiffres` chiffres (affichage et arbitrage des mesures ; jamais une decision)."""
        with decimal.localcontext(decimal.Context(prec=chiffres)):
            tot = decimal.Decimal(0)
            for a, c in self.t.items():
                tot += (decimal.Decimal(c.numerator) / decimal.Decimal(c.denominator)) * \
                       (decimal.Decimal(a.numerator) / decimal.Decimal(a.denominator)).sqrt()
            return tot

    # -- signe exact
    @staticmethod
    def _encadre(termes, P):
        """Encadrement entier (lo, hi) de 2^E somme c sqrt(a), E choisi pour P bits apres le plus grand terme."""
        mags = []
        for a, c in termes:
            m = _bits(c.numerator) - _bits(c.denominator) + (_bits(a.numerator) - _bits(a.denominator)) // 2
            mags.append(m)
        mx = max(mags)
        E = P - mx + 4
        lo = hi = 0
        for (a, c), m in zip(termes, mags):
            n, d = c.numerator, c.denominator
            if a == UN:
                S, L0, L1 = 0, 1, 1
            else:
                p, q = a.numerator, a.denominator
                S = max(0, P + 8 + (_bits(q) - _bits(p)) // 2 - (mx - m))
                L0 = isqrt((p << (2 * S)) // q)
                L1 = L0 + 1
            sh = E - S
            # terme = (n / d) * [L0, L1] * 2^(E - S)
            if n > 0:
                a0, a1 = n * L0, n * L1
            else:
                a0, a1 = n * L1, n * L0
            if sh >= 0:
                lo += (a0 << sh) // d
                hi += -((-(a1 << sh)) // d)
            else:
                dd = d << (-sh)
                lo += a0 // dd
                hi += -((-a1) // dd)
        return lo, hi

    def signe(self):
        termes = [(a, c) for a, c in self.t.items() if c]
        if not termes:
            return 0
        if len(termes) == 1:
            return 1 if termes[0][1] > 0 else -1
        if all(a == UN for a, _c in termes):
            v = sum(c for _a, c in termes)
            return (v > 0) - (v < 0)
        for P in (96, 320):
            lo, hi = self._encadre(termes, P)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
        # classes de carres
        reps, coefs = [], []
        for a, c in termes:
            for i, g in enumerate(reps):
                r = racine_rationnelle(a / g)
                if r is not None:
                    coefs[i] += c * r
                    break
            else:
                reps.append(a)
                coefs.append(c)
        classes = [(g, c) for g, c in zip(reps, coefs) if c]
        if not classes:
            return 0
        if len(classes) == 1:
            return 1 if classes[0][1] > 0 else -1
        P = 640
        while True:
            lo, hi = self._encadre(classes, P)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            exiger(P < (1 << 22), 'signe exact : precision epuisee (contradiction avec l independance des racines)')
            P *= 2


class Differe:
    """Rayon differe exact  rho = sqrt(B) - kappa sqrt(A) (2 W - D) / D,  W, D : Racines, D > 0, kappa rationnel.
    Garde num = rho * den et den (Racines > 0, ou None pour 1)."""
    __slots__ = ('num', 'den')

    def __init__(self, B, A, kappa, W, D):
        w, d = W.rationnel(), D.rationnel()
        if w is not None and d is not None:
            exiger(d > 0, 'denominateur de vote non positif')
            mu = 2 * w / d - 1
            self.num = Racines.racine(B) - Racines.racine(A, kappa * mu)
            self.den = None
        else:
            exiger(D.signe() > 0, 'denominateur de vote non positif')
            self.num = D.fois_racine(B) - (W.fois(2) - D).fois_racine(A).fois(kappa)
            self.den = D

    def cmp_niveau(self, lv):
        """Signe exact de rho - sqrt(lv)."""
        if self.den is None:
            return (self.num - Racines.racine(lv)).signe()
        return (self.num - self.den.fois_racine(lv)).signe()

    def carre_decimal(self, chiffres=60):
        """rho^2 a `chiffres` chiffres (affichage et arbitrage des mesures ; jamais une decision)."""
        with decimal.localcontext(decimal.Context(prec=chiffres)):
            v = self.num.decimal(chiffres)
            if self.den is not None:
                v = v / self.den.decimal(chiffres)
            return v * v

    def cmp(self, o):
        """Signe exact de self - o."""
        a = self.num if o.den is None else self.num.produit(o.den)
        b = o.num if self.den is None else o.num.produit(self.den)
        return (a - b).signe()


def _selftest():
    """Controle rapide (appele par les tests) : rend le nombre de verifications ; leve CertifErreur sinon."""
    n = 0
    F = Fraction
    cas = [
        (Racines.racine(8) - Racines.racine(2, 2), 0),                      # sqrt(8) = 2 sqrt(2)
        (Racines.racine(F(1, 8), 2) - Racines.racine(F(1, 2)), 0),          # 2 / sqrt(8) = 1 / sqrt(2)
        (Racines.racine(2) + Racines.racine(3) - Racines.racine(5, 1) - Racines.rat(F(909, 1000)), 1),
        (Racines.racine(2) - Racines.rat(F(14142135623730951, 10 ** 16)), -1),
        (Racines.racine(18) + Racines.racine(8) - Racines.racine(50), 0),   # 3 + 2 - 5 fois sqrt(2)
        (Racines.racine(F(16) + F(1, 1 << 60)) - Racines.rat(4), 1),
        (Racines.racine(10 ** 40 + 1) - Racines.racine(10 ** 40), 1),
        (Racines.racine(6).produit(Racines.racine(F(2, 3))) - Racines.rat(2), 0),
    ]
    for x, attendu in cas:
        exiger(x.signe() == attendu, 'selftest Racines : signe %r attendu %d' % (x.t, attendu))
        n += 1
    d1 = Differe(F(64), F(1), F(12), Racines.rat(2), Racines.rat(3))        # 8 - 12 / 3 = 4
    exiger(d1.cmp_niveau(F(16)) == 0 and d1.cmp_niveau(16 + F(1, 1 << 60)) == -1 and
           d1.cmp_niveau(16 - F(1, 1 << 60)) == 1, 'selftest Differe rationnel')
    W, D = Racines.racine(F(1, 2)), Racines.racine(F(1, 2)) + Racines.racine(F(1, 8))   # part 2/3, poids irrationnels
    d2 = Differe(F(64), F(1), F(12), W, D)
    exiger(d2.cmp(d1) == 0 and d2.cmp_niveau(F(16)) == 0 and d1.cmp(d2) == 0, 'selftest Differe irrationnel')
    d3 = Differe(F(64), F(1), F(12), Racines.rat(2), Racines.rat(3) + Racines.rat(F(1, 10 ** 30)))
    exiger(d3.cmp(d1) == 1 and d1.cmp(d3) == -1 and d3.cmp(d2) == 1, 'selftest Differe ordre')
    n += 9
    for z in range(1, 9):
        for v in (0.25, 3.0, 1e11, 6.8e10, 12345.678):
            w = psi_flottant(v, z)
            ref = Fraction(v) ** z                       # w^2 * v^z = 1 exactement si w etait exact
            ecart = abs(Fraction(w) ** 2 * ref - 1)
            exiger(ecart <= 2 * Fraction(borne_psi(z)) * (1 + Fraction(1, 1000)), 'selftest psi_flottant')
            n += 1
    return n
