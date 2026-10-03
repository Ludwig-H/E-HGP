#!/usr/bin/env python3
"""Oracle INDEPENDANT du verificateur adverse (label verif_fermeture, 3 octobre 2026).

Aucun import de reference/hgp11_ref, de bench/points_reference.py ni de fermeture/lib_fermeture.py.
Niveaux = rayons carres exacts (Fraction), coupes fermees.

- meb(F) : plus petite boule englobante exacte par le critere de M1 (centre dans l'enveloppe convexe d'un support
  affinement independant pose sur la sphere, tous les sites de F dedans). Circumcentres par les formules fermees
  du § 2 de MATHEMATIQUES.md (cardinaux 2, 3, 4), coordonnees barycentriques par Cramer. Ce n'est PAS la route de
  l'oracle de reference (qui prend le minimum des spheres circonscrites contenant F).
- Full(points, k) : balayage de Gamma_k par niveaux (sommets = k-parties, aretes = (k+1)-parties reliant toutes leurs
  k-faces), DSU, naissances / continuations / fusions N-aires, instantane (noeud, masque de couverture) a chaque
  niveau d'evenement. Couverture d'une composante = reunion de ses k-parties (theoreme 2 de la these).
- regles : fermeture qualifiee, first / cover (LCA des ex aequo), H_m (marge en niveau carre), EC, core.
"""
from fractions import Fraction
from itertools import combinations
import sys

sys.dont_write_bytecode = True


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def det3(a, b, c):
    return dot(a, cross(b, c))


def circumcenter(P):
    """Centre (Fractions) de la sphere circonscrite dans l'enveloppe affine, ou None si degenere."""
    a = P[0]
    if len(P) == 1:
        return tuple(Fraction(x) for x in a)
    if len(P) == 2:
        return tuple(Fraction(x + y, 2) for x, y in zip(P[0], P[1]))
    if len(P) == 3:
        u, v = sub(P[1], a), sub(P[2], a)
        w = cross(u, v)
        D = 2 * dot(w, w)
        if D == 0:
            return None
        uu, vv = dot(u, u), dot(v, v)
        t = tuple(uu * vi - vv * ui for ui, vi in zip(u, v))
        N = cross(t, w)
        return tuple(Fraction(ai) + Fraction(ni, D) for ai, ni in zip(a, N))
    u, v, z = sub(P[1], a), sub(P[2], a), sub(P[3], a)
    D = 2 * det3(u, v, z)
    if D == 0:
        return None
    uu, vv, zz = dot(u, u), dot(v, v), dot(z, z)
    vz, zu, uv = cross(v, z), cross(z, u), cross(u, v)
    N = tuple(uu * vz[i] + vv * zu[i] + zz * uv[i] for i in range(3))
    return tuple(Fraction(ai) + Fraction(ni, D) for ai, ni in zip(a, N))


def barycentric_nonneg(P, c):
    """c = sum l_i P_i, sum l_i = 1 : toutes les l_i >= 0 ? (P affinement independant, c dans aff(P))."""
    q = len(P)
    if q == 1:
        return True
    a = P[0]
    d = [sub(p, a) for p in P[1:]]
    cc = tuple(ci - ai for ci, ai in zip(c, a))
    # resoudre G l = r avec G_ij = d_i . d_j, r_i = d_i . cc (equations normales, systeme carre q-1)
    G = [[Fraction(dot(di, dj)) for dj in d] for di in d]
    r = [sum(Fraction(x) * y for x, y in zip(di, cc)) for di in d]
    m = q - 1
    M = [G[i][:] + [r[i]] for i in range(m)]
    for col in range(m):
        piv = None
        for row in range(col, m):
            if M[row][col] != 0:
                piv = row
                break
        if piv is None:
            return False
        M[col], M[piv] = M[piv], M[col]
        for row in range(m):
            if row != col and M[row][col] != 0:
                f = M[row][col] / M[col][col]
                M[row] = [x - f * y for x, y in zip(M[row], M[col])]
    lam = [M[i][m] / M[i][i] for i in range(m)]
    l0 = 1 - sum(lam)
    return l0 >= 0 and all(x >= 0 for x in lam)


class Cloud(object):
    def __init__(self, points):
        self.P = [tuple(int(c) for c in p) for p in points]
        if len(set(self.P)) != len(self.P):
            raise ValueError('sites distincts attendus')
        self.n = len(self.P)
        self._meb = {}
        self._cc = {}

    def d2(self, i, j):
        return dot(sub(self.P[i], self.P[j]), sub(self.P[i], self.P[j]))

    def _center(self, S):
        if S not in self._cc:
            pts = [self.P[i] for i in S]
            c = circumcenter(pts)
            if c is not None and not barycentric_nonneg(pts, c):
                c = None
            self._cc[S] = c
        return self._cc[S]

    def beta(self, F):
        F = tuple(sorted(F))
        if F in self._meb:
            return self._meb[F]
        if len(F) == 1:
            self._meb[F] = Fraction(0)
            return Fraction(0)
        found = None
        for q in range(2, min(4, len(F)) + 1):
            for S in combinations(F, q):
                c = self._center(S)
                if c is None:
                    continue
                R2 = sum((c[t] - self.P[S[0]][t]) ** 2 for t in range(3))
                ok = True
                for i in F:
                    if sum((c[t] - self.P[i][t]) ** 2 for t in range(3)) > R2:
                        ok = False
                        break
                if ok:
                    if found is not None and found != R2:
                        raise AssertionError('deux MEB distinctes %r' % (F,))
                    found = R2
                    break
            if found is not None:
                break
        if found is None:
            raise AssertionError('aucun support pour %r' % (F,))
        self._meb[F] = found
        return found

    def Dk(self, i, k):
        """k-ieme plus petite distance carree depuis i, le site compte (distance 0)."""
        return sorted(self.d2(i, j) for j in range(self.n))[k - 1]


def popcount(x):
    return bin(x).count('1')


def bits(mask):
    out, i = [], 0
    while mask:
        if mask & 1:
            out.append(i)
        mask >>= 1
        i += 1
    return out


class Full(object):
    """Arbre FULL d'ordre k par balayage de Gamma_k, avec instantanes de couverture aux coupes fermees."""

    def __init__(self, cloud, k):
        self.cloud, self.k, n = cloud, k, cloud.n
        verts = {}
        for F in combinations(range(n), k):
            verts[F] = cloud.beta(F)
        edges = []
        if k < n:
            for G in combinations(range(n), k + 1):
                edges.append((cloud.beta(G), G))
        events = {}
        for F, b in verts.items():
            events.setdefault(b, ([], []))[0].append(F)
        for b, G in edges:
            events.setdefault(b, ([], []))[1].append(G)
        parent = {}

        def find(x):
            r = x
            while parent[r] != r:
                r = parent[r]
            while parent[x] != r:
                parent[x], x = r, parent[x]
            return r
        node_of_root = {}
        cov_of_root = {}
        self.levels_of_node = []
        self.parent = []
        self.children = []
        self.snaps = []   # (niveau, [(noeud, couverture)])
        self.vnode = {}   # k-partie -> noeud vivant a son niveau
        for a in sorted(events):
            newv, newe = events[a]
            before = {}  # racine courante -> ensemble d'anciens noeuds absorbes
            for F in newv:
                parent[F] = F
                cov_of_root[F] = sum(1 << i for i in F)
                before[F] = set()
            touched = set(newv)
            for G in newe:
                faces = [tuple(x for x in G if x != y) for y in G]
                for f in faces:
                    if f not in parent:
                        raise AssertionError('face absente')
                r0 = find(faces[0])
                for f in faces[1:]:
                    r1 = find(f)
                    if r1 == r0:
                        continue
                    s0 = before.pop(r0, None)
                    if s0 is None:
                        s0 = {node_of_root[r0]}
                    s1 = before.pop(r1, None)
                    if s1 is None:
                        s1 = {node_of_root[r1]}
                    parent[r1] = r0
                    cov_of_root[r0] |= cov_of_root.pop(r1)
                    node_of_root.pop(r1, None)
                    before[r0] = s0 | s1
            for root, olds in before.items():
                if len(olds) == 0:
                    v = len(self.levels_of_node)
                    self.levels_of_node.append(a)
                    self.parent.append(-1)
                    self.children.append([])
                    node_of_root[root] = v
                elif len(olds) == 1:
                    node_of_root[root] = next(iter(olds))
                else:
                    v = len(self.levels_of_node)
                    self.levels_of_node.append(a)
                    self.parent.append(-1)
                    self.children.append(sorted(olds))
                    for c in olds:
                        self.parent[c] = v
                    node_of_root[root] = v
            for F in newv:
                self.vnode[F] = node_of_root[find(F)]
            snap = sorted((node_of_root[r], cov_of_root[r]) for r in node_of_root)
            self.snaps.append((a, snap))
        roots = [v for v in range(len(self.parent)) if self.parent[v] < 0]
        if len(roots) != 1:
            raise AssertionError('foret non connexe')
        self.root = roots[0]

    # -- arbre
    def chain(self, v):
        out = [v]
        while self.parent[out[-1]] >= 0:
            out.append(self.parent[out[-1]])
        return out

    def lca(self, a, b):
        s = set(self.chain(a))
        for w in self.chain(b):
            if w in s:
                return w
        raise AssertionError('lca')

    def alive(self, v, level):
        for w in self.chain(v):
            p = self.parent[w]
            if p < 0 or self.levels_of_node[p] > level:
                return w
        raise AssertionError('alive')

    def meet(self, v1, a1, v2, a2):
        w = self.lca(v1, v2)
        if w in (v1, v2):
            return max(a1, a2)
        return max(a1, a2, self.levels_of_node[w])

    def snap_at(self, level):
        """Instantane de la coupe fermee a un niveau quelconque (dernier evenement <= level)."""
        last = None
        for a, s in self.snaps:
            if a <= level:
                last = (a, s)
            else:
                break
        return last

    # -- regles
    def closure(self, m):
        n = self.cloud.n
        par = list(range(n))

        def f(x):
            while par[x] != x:
                par[x] = par[par[x]]
                x = par[x]
            return x
        active = [False] * n
        u = [[None] * n for _ in range(n)]
        w = [[None] * n for _ in range(n)]
        for a, snap in self.snaps:
            for v, cov in snap:
                if popcount(cov) < m:
                    continue
                s = bits(cov)
                for i in s:
                    active[i] = True
                for i in s[1:]:
                    ri, r0 = f(i), f(s[0])
                    if ri != r0:
                        par[ri] = r0
                for x in s:
                    for y in s:
                        if w[x][y] is None:
                            w[x][y] = a
            for i in range(n):
                if not active[i]:
                    continue
                for j in range(n):
                    if u[i][j] is None and active[j] and f(i) == f(j):
                        u[i][j] = a
        return u, w

    def cover_points(self, i, m):
        out = []
        for a, snap in self.snaps:
            for v, cov in snap:
                if cov >> i & 1 and popcount(cov) >= m:
                    out.append((a, v))
        return out

    def hang_first(self, m):
        ent = []
        for i in range(self.cloud.n):
            pts = self.cover_points(i, m)
            t = min(a for a, _ in pts)
            ties = sorted(set(v for a, v in pts if a == t))
            w = ties[0]
            for v in ties[1:]:
                w = self.lca(w, v)
            ent.append((max(t, self.levels_of_node[w]), w))
        return ent

    def hang_margin(self, m):
        ent = []
        for i in range(self.cloud.n):
            pts = self.cover_points(i, m)
            t = min(a for a, _ in pts)
            v1 = [v for a, v in pts if a == t][0]
            D = max(self.meet(v1, t, v, a) - a for a, v in pts)
            e = t + D
            ent.append((e, self.alive(v1, e)))
        return ent

    def hang_ec(self, m):
        ent = [None] * self.cloud.n
        for a, snap in self.snaps:
            for i in range(self.cloud.n):
                if ent[i] is not None:
                    continue
                cs = [v for v, cov in snap if cov >> i & 1 and popcount(cov) >= m]
                if len(cs) == 1:
                    ent[i] = (a, cs[0])
        return ent

    def hang_core(self):
        ent = []
        n, k = self.cloud.n, self.k
        for i in range(n):
            order = sorted(range(n), key=lambda j: (self.cloud.d2(i, j), j))
            F = tuple(sorted(order[:k]))
            Dk = self.cloud.d2(i, order[k - 1])
            v = self.alive(self.vnode[F], Dk)
            ent.append((Dk, v))
        return ent

    def ultra(self, ent):
        n = len(ent)
        u = [[None] * n for _ in range(n)]
        for i in range(n):
            for j in range(n):
                u[i][j] = self.meet(ent[i][1], ent[i][0], ent[j][1], ent[j][0]) if i != j else ent[i][0]
        return u


def blocks(u, level):
    n = len(u)
    act = [i for i in range(n) if u[i][i] <= level]
    seen, out = set(), []
    for i in act:
        if i in seen:
            continue
        b = frozenset(j for j in act if u[i][j] <= level)
        seen |= b
        out.append(b)
    return sorted(out, key=lambda b: sorted(b))


def minmax(w, n):
    u = [row[:] for row in w]
    for z in range(n):
        for i in range(n):
            for j in range(n):
                v = max(u[i][z], u[z][j])
                if v < u[i][j]:
                    u[i][j] = v
    return u


def w_direct(cloud, kp):
    n = cloud.n
    w = [[None] * n for _ in range(n)]
    for F in combinations(range(n), kp):
        b = cloud.beta(F)
        for x in F:
            for y in F:
                if w[x][y] is None or b < w[x][y]:
                    w[x][y] = b
    return w


def sqrt_gap_le(a, b, e2, factor2=1):
    """|sqrt(a) - sqrt(b)| <= sqrt(factor2 * e2) exactement (rationnels >= 0)."""
    E = Fraction(e2) * factor2
    hi, lo = (a, b) if a >= b else (b, a)
    # sqrt(hi) <= sqrt(lo) + sqrt(E)  <=>  hi - lo - E <= 2 sqrt(lo E)
    s = hi - lo - E
    if s <= 0:
        return True
    return s * s <= 4 * lo * E
