"""Verificateur independant (approche ancrage_marges) : oracle Gamma_K exact, propre MEB, propre arithmetique.

Aucun import du code de l'approche (ownership.py, qsqrt.py, oracle_gamma.py) ni de la reference hgp10_ref.py.
Decisions exactes : niveaux (rayons carres) en Fraction ; dates/hauteurs = sommes de racines de rationnels,
signe decide par regroupement en classes de carres (Besicovitch 1940) puis encadrement entier (isqrt).
Aucune verification par assert (identique sous python3 -O).
"""
from fractions import Fraction as Fr
from itertools import combinations
from math import isqrt
import functools


class VErr(RuntimeError):
    pass


def need(c, m):
    if not c:
        raise VErr(m)


# ------------------------------------------------------------------ arithmetique exacte : sommes de racines

def _is_sq(n):
    if n < 0:
        return False
    r = isqrt(n)
    return r * r == n


class R:
    """Somme exacte sum c_m sqrt(m), m entier > 0 (m = 1 : partie rationnelle), c_m rationnels."""
    __slots__ = ('t',)

    def __init__(self, t=None):
        self.t = {}
        if t:
            for m, c in t.items():
                if c != 0:
                    self.t[m] = self.t.get(m, Fr(0)) + c
            self.t = {m: c for m, c in self.t.items() if c != 0}

    @staticmethod
    def rat(q):
        q = Fr(q)
        return R({1: q}) if q != 0 else R()

    @staticmethod
    def sqrt(q):
        q = Fr(q)
        need(q >= 0, 'racine negative')
        if q == 0:
            return R()
        n, d = q.numerator, q.denominator
        m = n * d  # sqrt(n/d) = sqrt(n d) / d
        if _is_sq(m):
            return R({1: Fr(isqrt(m), d)})
        return R({m: Fr(1, d)})

    def __add__(self, o):
        o = o if isinstance(o, R) else R.rat(o)
        t = dict(self.t)
        for m, c in o.t.items():
            t[m] = t.get(m, Fr(0)) + c
        return R(t)

    __radd__ = __add__

    def __neg__(self):
        return R({m: -c for m, c in self.t.items()})

    def __sub__(self, o):
        o = o if isinstance(o, R) else R.rat(o)
        return self + (-o)

    def __rsub__(self, o):
        return R.rat(o) - self

    def __mul__(self, q):
        q = Fr(q)
        return R({m: c * q for m, c in self.t.items()})

    __rmul__ = __mul__

    def _groups(self):
        """Regroupe les radicandes de meme classe de carres : sqrt(m) = sqrt(m m0)/m0 * sqrt(m0)."""
        gs = []
        for m, c in self.t.items():
            for g in gs:
                p = m * g[0]
                if _is_sq(p):
                    g[1] += c * Fr(isqrt(p), g[0])
                    break
            else:
                gs.append([m, c])
        return [(m, c) for m, c in gs if c != 0]

    def sign(self):
        gs = self._groups()
        if not gs:
            return 0
        if len(gs) == 1:
            return 1 if gs[0][1] > 0 else -1
        k = 40
        while True:
            S = 1 << k
            lo = hi = Fr(0)
            for m, c in gs:
                s = isqrt(m * S * S)  # s/S <= sqrt(m) < (s+1)/S
                a, b = Fr(s, S), Fr(s + 1, S)
                if c > 0:
                    lo += c * a
                    hi += c * b
                else:
                    lo += c * b
                    hi += c * a
            if lo > 0:
                return 1
            if hi < 0:
                return -1
            k *= 2
            need(k <= 1 << 15, 'precision')

    def cmp(self, o):
        return (self - o).sign()

    def __lt__(self, o):
        return self.cmp(o) < 0

    def __le__(self, o):
        return self.cmp(o) <= 0

    def __gt__(self, o):
        return self.cmp(o) > 0

    def __ge__(self, o):
        return self.cmp(o) >= 0

    def eq(self, o):
        return self.cmp(o) == 0

    def __float__(self):
        return float(sum(float(c) * (m ** 0.5) for m, c in self.t.items()))

    def __repr__(self):
        return 'R(%s)' % ' + '.join('%s*sqrt(%d)' % (c, m) for m, c in sorted(self.t.items()))


def rmax(*vals):
    b = None
    for v in vals:
        if b is None or v > b:
            b = v
    return b


def rabs(v):
    return -v if v.sign() < 0 else v


# ------------------------------------------------------------------ MEB exacte (supports de taille <= 4)

def _sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _circ(pts):
    """Centre et rayon carre de la plus petite boule passant par pts (centre dans l'enveloppe affine).
    None si pts affinement dependants."""
    p0 = pts[0]
    V = [_sub(p, p0) for p in pts[1:]]
    m = len(V)
    if m == 0:
        return tuple(Fr(c) for c in p0), Fr(0)
    A = [[Fr(_dot(V[i], V[j])) for j in range(m)] + [Fr(_dot(V[i], V[i]), 2)] for i in range(m)]
    for col in range(m):
        piv = None
        for r in range(col, m):
            if A[r][col] != 0:
                piv = r
                break
        if piv is None:
            return None
        A[col], A[piv] = A[piv], A[col]
        for r in range(m):
            if r != col and A[r][col] != 0:
                f = A[r][col] / A[col][col]
                A[r] = [x - f * y for x, y in zip(A[r], A[col])]
    lam = [A[i][m] / A[i][i] for i in range(m)]
    c = tuple(Fr(p0[k]) + sum(lam[j] * V[j][k] for j in range(m)) for k in range(3))
    r2 = sum((c[k] - p0[k]) ** 2 for k in range(3))
    return c, r2


_MEB = {}


def meb2(P, idx):
    """Rayon carre exact de la plus petite boule fermee contenant {P[i], i in idx}."""
    key = tuple(sorted(P[i] for i in idx))
    hit = _MEB.get(key)
    if hit is not None:
        return hit
    pts = list(key)
    best = None
    for s in range(1, min(4, len(pts)) + 1):
        for T in combinations(pts, s):
            cr = _circ(list(T))
            if cr is None:
                continue
            c, r2 = cr
            if best is not None and r2 >= best:
                continue
            if all(sum((c[k] - p[k]) ** 2 for k in range(3)) <= r2 for p in pts):
                best = r2
    need(best is not None, 'MEB introuvable')
    _MEB[key] = best
    return best


# ------------------------------------------------------------------ Gamma_K et structure couvrante

class DSU:
    def __init__(self):
        self.p = {}

    def add(self, x):
        if x not in self.p:
            self.p[x] = x

    def find(self, x):
        r = x
        while self.p[r] != r:
            r = self.p[r]
        while self.p[x] != r:
            self.p[x], x = r, self.p[x]
        return r

    def union(self, a, b):
        a, b = self.find(a), self.find(b)
        if a != b:
            if b < a:
                a, b = b, a
            self.p[b] = a


class Tower:
    """FULL_K exact par Gamma_K (toutes les K-parties et (K+1)-parties), coupe fermee."""

    def __init__(self, P, K):
        P = [tuple(int(c) for c in p) for p in P]
        need(len(set(P)) == len(P), 'sites dupliques')
        need(1 <= K <= len(P), 'K')
        self.P, self.K, self.n = P, K, len(P)
        n = self.n
        self.V = {F: meb2(P, F) for F in combinations(range(n), K)}
        self.E = {G: meb2(P, G) for G in combinations(range(n), K + 1)} if K < n else {}
        self.levels = sorted(set(self.V.values()) | set(self.E.values()))
        dsu = DSU()
        vs = sorted(self.V.items(), key=lambda t: t[1])
        es = sorted(self.E.items(), key=lambda t: t[1])
        iv = ie = 0
        present = []
        self.snap = []
        for lv in self.levels:
            while iv < len(vs) and vs[iv][1] == lv:
                dsu.add(vs[iv][0])
                present.append(vs[iv][0])
                iv += 1
            while ie < len(es) and es[ie][1] == lv:
                G = es[ie][0]
                faces = [tuple(y for y in G if y != u) for u in G]
                for f in faces:
                    need(f in dsu.p, 'face absente')
                for f in faces[1:]:
                    dsu.union(faces[0], f)
                ie += 1
            self.snap.append({F: dsu.find(F) for F in present})
        self._br = {}
        self._join = {}

    def idx(self, beta):
        lo, hi = -1, len(self.levels) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if self.levels[mid] <= beta:
                lo = mid
            else:
                hi = mid - 1
        return lo

    def idx_r(self, r, closed=True):
        """Plus grand indice i avec sqrt(levels[i]) <= r (fermee) / < r (ouverte) ; r : R."""
        lo, hi = -1, len(self.levels) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            c = R.sqrt(self.levels[mid]).cmp(r)
            if c < 0 or (closed and c == 0):
                lo = mid
            else:
                hi = mid - 1
        return lo

    def cov(self, x, i):
        """Classes couvrant x a l'indice de niveau i."""
        if i < 0:
            return set()
        s = self.snap[i]
        return {s[F] for F in s if x in F}

    def branches(self, x):
        """Feuilles (branches couvrantes nouvelles) : [(c_beta, rep)] par date croissante."""
        if x in self._br:
            return self._br[x]
        seen = set()
        out = []
        for i, lv in enumerate(self.levels):
            s = self.snap[i]
            mine = [F for F in s if x in F]
            cl = {}
            for F in mine:
                cl.setdefault(s[F], []).append(F)
            for root in sorted(cl):
                Fs = cl[root]
                if not any(F in seen for F in Fs):
                    out.append((lv, min(Fs)))
            seen.update(mine)
        need(out, 'point jamais couvert')
        self._br[x] = out
        return out

    def join_level(self, reps):
        """Premier niveau ou tous les representants sont presents et dans une meme classe."""
        key = tuple(sorted(set(reps)))
        hit = self._join.get(key)
        if hit is not None:
            return hit
        for i, lv in enumerate(self.levels):
            s = self.snap[i]
            if all(F in s for F in key) and len({s[F] for F in key}) == 1:
                self._join[key] = lv
                return lv
        raise VErr('jamais reunis')

    def alpha2(self, x):
        return self.branches(x)[0][0]

    def dk2(self, x):
        return sorted(sum((a - b) ** 2 for a, b in zip(self.P[x], self.P[y])) for y in range(self.n))[self.K - 1]

    def stairs(self, x):
        """[(c_j, N_j, reps<=c_j)] ; N_j = niveau de reunion des branches nees <= c_j (N = c si une seule)."""
        br = self.branches(x)
        out, reps, i = [], [], 0
        while i < len(br):
            c = br[i][0]
            while i < len(br) and br[i][0] == c:
                reps.append(br[i][1])
                i += 1
            N = c if len(reps) == 1 else self.join_level(reps)
            out.append((c, N, list(reps)))
        return out

    def bars(self, x):
        """Code-barres H0 (regle de l'aine) des branches couvrantes : [(c, m)] barres finies, en niveaux."""
        br = self.branches(x)
        out = []
        for j in range(1, len(br)):
            c, rep = br[j]
            older = [b[1] for b in br[:j] if b[0] < c or (b[0] == c and b[1] < rep)]
            # mort = premier niveau ou rep rejoint la classe d'une branche plus ancienne
            m = None
            for i, lv in enumerate(self.levels):
                if lv < c:
                    continue
                s = self.snap[i]
                if any(o in s and s[o] == s[rep] for o in older):
                    m = lv
                    break
            need(m is not None, 'barre sans mort')
            out.append((c, m))
        return out


# ------------------------------------------------------------------ regles (independantes)

class Rule:
    def __init__(self, T, name, dates, reps):
        self.T, self.name, self.dates, self.reps = T, name, dates, reps

    def height(self, x, z):
        if x == z:
            return R()
        j = self.T.join_level([self.reps[x], self.reps[z]])
        return rmax(self.dates[x], self.dates[z], R.sqrt(j))

    def partition(self, r, closed=True):
        i = self.T.idx_r(r, closed)
        s = self.T.snap[i] if i >= 0 else {}
        g = {}
        for x in range(self.T.n):
            c = self.dates[x].cmp(r)
            if c > 0 or (c == 0 and not closed):
                g[('s', x)] = [x]
            else:
                need(self.reps[x] in s, 'proprietaire absent a la coupe')
                g.setdefault(('c', s[self.reps[x]]), []).append(x)
        return sorted(sorted(v) for v in g.values())


def rule_pk(T, kappa, profile=None):
    """P_kappa : t = max(alpha, max_j sqrt(N_j) - kappa (sqrt(c_j) - alpha)) ; profile(s: R) -> R remplace kappa*s."""
    dates, reps = [], []
    for x in range(T.n):
        st = T.stairs(x)
        a = R.sqrt(st[0][0])
        t = a
        for c, N, _r in st:
            s = R.sqrt(c) - a
            g = profile(s) if profile is not None else s * Fr(kappa)
            v = R.sqrt(N) - g
            if v > t:
                t = v
        dates.append(t)
        reps.append(st[0][2][0])
    return Rule(T, 'P_%s' % kappa, dates, reps)


def rule_pk_bars(T, kappa):
    """P_kappa par le code-barres (proposition 3.1, seconde forme) : max(alpha, max_j m_j - kappa (c_j - alpha))."""
    dates = []
    for x in range(T.n):
        a = R.sqrt(T.alpha2(x))
        t = a
        for c, m in T.bars(x):
            v = R.sqrt(m) - (R.sqrt(c) - a) * Fr(kappa)
            if v > t:
                t = v
        dates.append(t)
    return dates


def rule_a1_lca(T):
    """A5 : max(alpha, N_1)."""
    dates, reps = [], []
    for x in range(T.n):
        st = T.stairs(x)
        dates.append(rmax(R.sqrt(st[0][0]), R.sqrt(st[0][1])))
        reps.append(st[0][2][0])
    return Rule(T, 'A5', dates, reps)


def rule_band(T, eta, sat=False):
    dates, reps = [], []
    for x in range(T.n):
        st = T.stairs(x)
        a = R.sqrt(st[0][0])
        B = a * (1 + Fr(eta))
        N = None
        for c, Nj, _r in st:
            if R.sqrt(c) <= B:
                N = Nj
            else:
                break
        dates.append(rmax(B if sat else a, R.sqrt(N)))
        reps.append(st[0][2][0])
    return Rule(T, ('bande_sat[%s]' if sat else 'bande_f[%s]') % eta, dates, reps)


def rule_core(T):
    dates, reps = [], []
    for x in range(T.n):
        d2 = T.dk2(x)
        i = T.idx(d2)
        s = T.snap[i]
        F = [G for G in s if x in G]
        need(F, 'core : x non present a d_K')
        dates.append(R.sqrt(d2))
        reps.append(min(F))
    return Rule(T, 'core', dates, reps)


def check_nested(rule):
    """Emboitement exhaustif aux coupes d'evenements (fermees et ouvertes)."""
    T = rule.T
    ev = [R.sqrt(lv) for lv in T.levels] + list(rule.dates) + [R()]
    ev = sorted(ev, key=functools.cmp_to_key(lambda a, b: a.cmp(b)))
    uniq = []
    for r in ev:
        if not uniq or not uniq[-1].eq(r):
            uniq.append(r)
    prev = None
    n = 0
    for r in uniq:
        for closed in (False, True):
            cur = rule.partition(r, closed)
            if prev is not None:
                where = {}
                for i, b in enumerate(cur):
                    for x in b:
                        where[x] = i
                for b in prev:
                    need(len({where[x] for x in b}) == 1, '%s : bloc scinde' % rule.name)
                    n += 1
            prev = cur
    return n
