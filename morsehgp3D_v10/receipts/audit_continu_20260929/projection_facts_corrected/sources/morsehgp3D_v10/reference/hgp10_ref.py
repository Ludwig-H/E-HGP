"""Reference exacte de la v10 : oracle Gamma_k, catalogue critique, tour FULL, hierarchie de points C n X.

Arithmetique : Fraction partout (aucun flottant). Domaine : petits nuages (n <= 14 pour l'oracle Gamma,
quelques dizaines pour le catalogue et la tour), points distincts a coordonnees entieres, degenerescences comprises.

Objet. Pour k = 1..K, L_k(a) = { y : D_k(y) <= a } (D_k = k-ieme distance carree, multiplicite comprise).
Theoreme 2 du manuscrit : les composantes de L_k(a) correspondent a celles du graphe Gamma_k(a) (sommets : k-parties F
avec beta(F) <= a ; aretes : F, F' avec |F u F'| = k+1 et beta(F u F') <= a). La COUVERTURE d'une composante est
l'union des points de ses sommets. Une coupe est (a, ferme) : beta <= a, ou (a, ouvert) : beta < a.

La tour (voie de la v10) n'enumere jamais les k-parties : elle lit les spheres critiques (support S affinement
independant, |S| <= 4, centre c dans l'interieur relatif de conv(S), interieur strict I, coquille U) et, pour chaque
ordre k avec p < k <= p + m, la structure locale des sous-ensembles separables de U (theoreme de Gordan : A est
separable ssi c n'est pas dans conv(A)) :
  - aucun A separable de taille k - p : NAISSANCE d'une composante couvrant I u (union des A non separables) ;
  - sinon les MORCEAUX locaux (composantes du graphe A ~ A' ssi c hors de conv(A u A')) sont joints, et la
    composante recoit toute la boule fermee I u U (contribution datee), fusion si plusieurs racines.
Les morceaux sont rattaches a des minima par DESCENTE (beta decroit strictement), independante de l'ordre de traitement.
"""
from fractions import Fraction as Fr
from itertools import combinations
import bisect


# ---------------------------------------------------------------- geometrie exacte

def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve(G, b):
    """Resolution exacte (Gauss sur Fraction) ; None si singulier."""
    n = len(G)
    M = [list(map(Fr, G[i])) + [Fr(b[i])] for i in range(n)]
    for c in range(n):
        piv = next((r for r in range(c, n) if M[r][c] != 0), None)
        if piv is None:
            return None
        M[c], M[piv] = M[piv], M[c]
        for r in range(n):
            if r != c and M[r][c] != 0:
                f = M[r][c] / M[c][c]
                M[r] = [x - f * y for x, y in zip(M[r], M[c])]
    return [M[i][n] / M[i][i] for i in range(n)]


def rank(vectors):
    M = [list(map(Fr, v)) for v in vectors]
    r = 0
    for col in range(3):
        piv = next((i for i in range(r, len(M)) if M[i][col] != 0), None)
        if piv is None:
            continue
        M[r], M[piv] = M[piv], M[r]
        for i in range(len(M)):
            if i != r and M[i][col] != 0:
                f = M[i][col] / M[r][col]
                M[i] = [x - f * y for x, y in zip(M[i], M[r])]
        r += 1
    return r


def affinely_independent(pts):
    return len(pts) <= 1 or rank([sub(p, pts[0]) for p in pts[1:]]) == len(pts) - 1


def circumcenter(pts):
    """Centre du cercle/de la sphere circonscrit(e) dans aff(pts) et barycentriques ; None si degenere."""
    p0 = pts[0]
    if len(pts) == 1:
        return tuple(map(Fr, p0)), [Fr(1)]
    D = [sub(p, p0) for p in pts[1:]]
    lam = solve([[dot(a, b) for b in D] for a in D], [Fr(dot(a, a), 2) for a in D])
    if lam is None:
        return None
    c = tuple(Fr(p0[j]) + sum(l * d[j] for l, d in zip(lam, D)) for j in range(3))
    return c, [1 - sum(lam)] + lam


def d2(c, p):
    return sum((Fr(x) - y) ** 2 for x, y in zip(p, c))


def in_closed_hull(c, pts):
    """c dans conv(pts) ferme (Caratheodory : un sous-ensemble affinement independant le contient)."""
    for q in range(1, min(4, len(pts)) + 1):
        for S in combinations(pts, q):
            if not affinely_independent(list(S)):
                continue
            if q == 1:
                if tuple(map(Fr, S[0])) == c:
                    return True
                continue
            p0 = S[0]
            D = [sub(p, p0) for p in S[1:]]
            lam = solve([[dot(a, b) for b in D] for a in D], [dot(a, sub(c, p0)) for a in D])
            if lam is None:
                continue
            proj = tuple(Fr(p0[j]) + sum(l * d[j] for l, d in zip(lam, D)) for j in range(3))
            if proj == c and all(l >= 0 for l in lam) and 1 - sum(lam) >= 0:
                return True
    return False


def meb(P, idx):
    """Plus petite boule englobante exacte : (beta, centre) par force brute sur les supports."""
    best = None
    for q in range(1, min(4, len(idx)) + 1):
        for U in combinations(idx, q):
            pts = [P[i] for i in U]
            if not affinely_independent(pts):
                continue
            r = circumcenter(pts)
            if r is None:
                continue
            c, lam = r
            if any(l < 0 for l in lam):
                continue
            rad = d2(c, P[U[0]])
            if (best is None or rad < best[0]) and all(d2(c, P[i]) <= rad for i in idx):
                best = (rad, c)
    return best


class DSU:
    def __init__(self):
        self.p = {}

    def find(self, x):
        self.p.setdefault(x, x)
        root = x
        while self.p[root] != root:
            root = self.p[root]
        while self.p[x] != root:
            self.p[x], x = root, self.p[x]
        return root

    def union(self, a, b):
        a, b = self.find(a), self.find(b)
        if a == b:
            return False
        if b < a:
            a, b = b, a
        self.p[b] = a
        return True


# ---------------------------------------------------------------- oracle Gamma_k

def gamma_cuts(P, k):
    """Oracle exhaustif : pour chaque niveau critique a, les couvertures des composantes aux coupes ouverte et fermee.

    Retourne (levels, closed, opened) : closed[i] / opened[i] = liste triee de frozensets de points (couvertures)."""
    n = len(P)
    beta = {F: meb(P, F)[0] for F in combinations(range(n), k)}
    cof = {G: meb(P, G)[0] for G in combinations(range(n), k + 1)} if k < n else {}
    levels = sorted(set(beta.values()) | set(cof.values()))
    closed, opened = [], []
    dsu = DSU()
    alive = []
    vs = sorted(beta.items(), key=lambda t: t[1])
    es = sorted(cof.items(), key=lambda t: t[1])
    iv = ie = 0

    def snapshot():
        comps = {}
        for F in alive:
            comps.setdefault(dsu.find(F), set()).update(F)
        return sorted((frozenset(c) for c in comps.values()), key=lambda s: sorted(s))

    for a in levels:
        opened.append(snapshot())
        while iv < len(vs) and vs[iv][1] == a:
            alive.append(vs[iv][0])
            dsu.find(vs[iv][0])
            iv += 1
        while ie < len(es) and es[ie][1] == a:
            G = es[ie][0]
            fs = [tuple(x for x in G if x != u) for u in G]
            for f in fs[1:]:
                dsu.union(fs[0], f)
            ie += 1
        closed.append(snapshot())
    return levels, closed, opened


# ---------------------------------------------------------------- catalogue critique

class Ball:
    __slots__ = ('level', 'center', 'I', 'U', 'qmin')

    def __init__(self, level, center, I, U, qmin):
        self.level, self.center, self.I, self.U, self.qmin = level, center, I, U, qmin

    @property
    def p(self):
        return len(self.I)

    @property
    def m(self):
        return len(self.U)

    def key(self):
        return (self.center, self.level)


def critical_balls(P):
    """Toutes les spheres critiques (force brute sur les supports, dedoublonnees par (centre, rayon carre))."""
    n = len(P)
    seen = {}
    for q in range(1, 5):
        for S in combinations(range(n), q):
            pts = [P[i] for i in S]
            if not affinely_independent(pts):
                continue
            r = circumcenter(pts)
            if r is None:
                continue
            c, lam = r
            if any(l <= 0 for l in lam):
                continue
            rad = d2(c, P[S[0]])
            key = (c, rad)
            if key in seen:
                seen[key][4] = min(seen[key][4], q)
                continue
            I = tuple(i for i in range(n) if d2(c, P[i]) < rad)
            U = tuple(i for i in range(n) if d2(c, P[i]) == rad)
            seen[key] = [rad, c, I, U, q]
    return [Ball(*v) for v in seen.values()]


def catalogue(P, kmax):
    """Admission : p + q_min <= min(kmax + 1, n)."""
    n = len(P)
    return [b for b in critical_balls(P) if b.p + b.qmin <= min(kmax + 1, n)]


# ---------------------------------------------------------------- structure locale d'une boule a l'ordre k

def local_structure(P, ball, k):
    """Pour p < k <= p + m : ('birth', couverture) ou ('join', representants des morceaux) ; None hors fenetre."""
    p, m = ball.p, ball.m
    if not (p < k <= p + m):
        return None
    t = k - p
    c = ball.center
    subs = list(combinations(ball.U, t))
    sep = [A for A in subs if not in_closed_hull(c, [P[i] for i in A])]
    if not sep:
        covered = set(ball.I)
        for A in subs:
            covered.update(A)
        return ('birth', frozenset(covered))
    dsu = DSU()
    for A in sep:
        dsu.find(A)
    for A, B in combinations(sep, 2):
        AB = sorted(set(A) | set(B))
        if not in_closed_hull(c, [P[i] for i in AB]):
            dsu.union(A, B)
    groups = {}
    for A in sep:
        groups.setdefault(dsu.find(A), []).append(A)
    reps = sorted(tuple(sorted(ball.I + g[0])) for g in (sorted(v) for v in groups.values()))
    return ('join', reps)


def canonical_birth_vertex(ball, k):
    return tuple(sorted(ball.I + tuple(sorted(ball.U)[:k - ball.p])))


# ---------------------------------------------------------------- descente vers un minimum

def descend(P, F, k, balls_by_key):
    """k-partie -> sommet canonique d'une naissance (minimum), connexe a F au niveau beta(F)."""
    F = tuple(sorted(F))
    n = len(P)
    while True:
        rad, c = meb(P, F)
        I = tuple(i for i in range(n) if d2(c, P[i]) < rad)
        U = tuple(i for i in range(n) if d2(c, P[i]) == rad)
        if len(I) >= k:
            G = tuple(sorted(sorted(range(n), key=lambda i: (d2(c, P[i]), i))[:k]))
        else:
            ball = balls_by_key.get((c, rad))
            if ball is None:
                ball = Ball(rad, c, I, U, 4)
            st = local_structure(P, ball, k)
            if st[0] == 'birth':
                return canonical_birth_vertex(ball, k)
            best = min((meb(P, G0)[0], G0) for G0 in st[1])
            G = best[1]
        if not meb(P, G)[0] < rad:
            raise AssertionError('descent did not decrease beta')
        F = G


# ---------------------------------------------------------------- tour d'un ordre

class OrderForest:
    """Foret d'un ordre k : naissances, contributions datees, multifusions (lots par niveau exact)."""

    def __init__(self, k):
        self.k = k
        self.events = []      # (niveau, 'birth'|'join', donnees)
        self.batches = []     # (niveau, naissances [(id, couverture)], jonctions [(ids, contribution)])

    def cut(self, a, closed=True):
        """Couvertures des composantes a la coupe (a, ferme|ouvert)."""
        dsu = DSU()
        cover = {}
        for lvl, births, joins in self.batches:
            if lvl > a or (lvl == a and not closed):
                break
            for vid, cov in births:
                dsu.find(vid)
                cover.setdefault(vid, set()).update(cov)
            for ids, contrib in joins:
                r0 = dsu.find(ids[0])
                for other in ids[1:]:
                    ro, r0 = dsu.find(other), dsu.find(r0)
                    if ro != r0:
                        dsu.union(r0, ro)
                        nr = dsu.find(r0)
                        merged = cover.pop(r0, set()) | cover.pop(ro, set())
                        cover[nr] = merged
                        r0 = nr
                r0 = dsu.find(r0)
                cover.setdefault(r0, set()).update(contrib)
        comps = {}
        for vid in list(cover):
            comps.setdefault(dsu.find(vid), set()).update(cover[vid])
        return sorted((frozenset(c) for c in comps.values()), key=lambda s: sorted(s))

    def levels(self):
        return [b[0] for b in self.batches]


def build_order(P, k, cat):
    """Tour d'ordre k depuis le catalogue : lots par niveau exact, naissances avant jonctions."""
    by_key = {b.key(): b for b in cat}
    per_level = {}
    for b in cat:
        st = local_structure(P, b, k)
        if st is None:
            continue
        births, joins = per_level.setdefault(b.level, ([], []))
        if st[0] == 'birth':
            births.append((canonical_birth_vertex(b, k), st[1]))
        else:
            ids = sorted({descend(P, F, k, by_key) for F in st[1]})
            joins.append((ids, frozenset(b.I + b.U)))
    forest = OrderForest(k)
    for lvl in sorted(per_level):
        births, joins = per_level[lvl]
        forest.batches.append((lvl, sorted(births), sorted(joins)))
    return forest


def build_tower(P, kmax):
    cat = catalogue(P, kmax)
    return cat, {k: build_order(P, k, cat) for k in range(1, min(kmax, len(P)) + 1)}


# ---------------------------------------------------------------- hierarchie de points C n X

def entry_level(P, x, k):
    """D_k(x) : k-ieme plus petite distance carree de x a X (x compris, a distance 0)."""
    return sorted(dot(sub(P[x], P[y]), sub(P[x], P[y])) for y in range(len(P)))[k - 1]


def knn_vertex(P, x, k):
    """Sommet realise en x : ses k plus proches (x compris), departage par indice."""
    return tuple(sorted(sorted(range(len(P)), key=lambda y: (dot(sub(P[x], P[y]), sub(P[x], P[y])), y))[:k]))


def forest_root(forest, vid, a, closed=True):
    """Racine (identifiant canonique = plus petit sommet de naissance) de vid a la coupe."""
    dsu = DSU()
    for lvl, births, joins in forest.batches:
        if lvl > a or (lvl == a and not closed):
            break
        for b, _ in births:
            dsu.find(b)
        for ids, _ in joins:
            for other in ids[1:]:
                dsu.union(ids[0], other)
    return dsu.find(vid)


def point_partition_tower(P, forest, a, cat, closed=True):
    """C n X depuis la tour : x entre a D_k(x) ; sa composante est celle de la descente de ses k voisins."""
    k = forest.k
    by_key = {b.key(): b for b in cat}
    groups = {}
    for x in range(len(P)):
        e = entry_level(P, x, k)
        if e > a or (e == a and not closed):
            continue
        m = descend(P, knn_vertex(P, x, k), k, by_key)
        groups.setdefault(forest_root(forest, m, a, closed), set()).add(x)
    return sorted((frozenset(g) for g in groups.values()), key=lambda s: sorted(s))


def point_partition_gamma(P, k, a, closed=True):
    """C n X par l'oracle : composante de Gamma_k(a) du sommet knn(x)."""
    n = len(P)
    beta = {F: meb(P, F)[0] for F in combinations(range(n), k)}
    ok = (lambda b: b <= a) if closed else (lambda b: b < a)
    dsu = DSU()
    for F, b in beta.items():
        if ok(b):
            dsu.find(F)
    if k < n:
        for G in combinations(range(n), k + 1):
            if ok(meb(P, G)[0]):
                fs = [tuple(y for y in G if y != u) for u in G]
                for f in fs[1:]:
                    dsu.union(fs[0], f)
    groups = {}
    for x in range(n):
        e = entry_level(P, x, k)
        if not ok(e):
            continue
        groups.setdefault(dsu.find(knn_vertex(P, x, k)), set()).add(x)
    return sorted((frozenset(g) for g in groups.values()), key=lambda s: sorted(s))
