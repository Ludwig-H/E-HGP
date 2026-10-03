#!/usr/bin/env python3
"""Verification adverse (verif_marge_cibles) : FULL_K et regles de pendaison ecrits de zero.

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
public_status=not_claimed. GCP non utilise. Aucune commande git. Ecritures sous verif_marge_cibles/ seulement.

Aucun code du rapport marge_cibles ni de l'oracle v11 n'est importe ici. Construction :
  - L_K(a) = union des K-lentilles P_F(r) = intersection des boules fermees B(y, r), y dans F, |F| = K, r^2 = a ;
  - P_F et P_G se coupent ssi MEB(F u G) <= r (intersection de boules de meme rayon) : nerf exact ;
  - MEB exacte par enumeration des supports affinement independants (2 a 4 points), Fraction ;
  - arbre de fusion par balayage des niveaux distincts (coupes fermees) : 0 ancien noeud -> feuille,
    1 -> continuation, >= 2 -> noeud de fusion ;
  - couverture : x dans E_C(a) ssi il existe F dans C avec MEB(F u {x})^2 <= a ;
    c_x(nu) = max(h(nu), min_{F dans sous-arbre(nu)} MEB(F u {x})^2), valide si < mort(nu).
Regles : core (P1), cover / first (P2, LCA des ex aequo), H_m de HIERARCHIE_POINTS.md section 3 (famille H4 de pente
kappa), en niveau carre ou en rayon (decisions exactes par vrad.R de la v10, simple arithmetique de radicaux).
Aucun assert : les controles levent VerifErreur.
"""
import sys
sys.dont_write_bytecode = True
from fractions import Fraction as Fr  # noqa: E402
from itertools import combinations  # noqa: E402

VRAD = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'
if VRAD not in sys.path:
    sys.path.insert(0, VRAD)
from vrad import R  # noqa: E402  (arithmetique exacte des sommes de racines, sans logique de regle)


class VerifErreur(RuntimeError):
    pass


def exiger(c, m):
    if not c:
        raise VerifErreur(m)


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _det3(r0, r1, r2):
    return (r0[0] * (r1[1] * r2[2] - r1[2] * r2[1]) - r0[1] * (r1[0] * r2[2] - r1[2] * r2[0])
            + r0[2] * (r1[0] * r2[1] - r1[1] * r2[0]))


def circumball(S):
    """Boule circonscrite de S (1 a 4 points entiers) de centre dans l'enveloppe affine ; None si degenere."""
    a = S[0]
    if len(S) == 1:
        return (Fr(a[0]), Fr(a[1]), Fr(a[2])), Fr(0)
    if len(S) == 2:
        b = S[1]
        u = _sub(b, a)
        return tuple(Fr(a[i] + b[i], 2) for i in range(3)), Fr(_dot(u, u), 4)
    if len(S) == 3:
        u, v = _sub(S[1], a), _sub(S[2], a)
        uu, uv, vv = _dot(u, u), _dot(u, v), _dot(v, v)
        det = uu * vv - uv * uv
        if det == 0:
            return None
        al = Fr(uu * vv - uv * vv, 2 * det)
        be = Fr(uu * vv - uv * uu, 2 * det)
        w = tuple(al * u[i] + be * v[i] for i in range(3))
        return tuple(a[i] + w[i] for i in range(3)), _dot(w, w)
    u, v, z = _sub(S[1], a), _sub(S[2], a), _sub(S[3], a)
    det = _det3(u, v, z)
    if det == 0:
        return None
    rhs = (Fr(_dot(u, u), 2), Fr(_dot(v, v), 2), Fr(_dot(z, z), 2))
    rows = (u, v, z)
    w = []
    for k in range(3):
        # Cramer : lignes u, v, z ; colonne k remplacee par rhs
        M = [list(rows[r]) for r in range(3)]
        for r in range(3):
            M[r][k] = rhs[r]
        w.append(Fr(_det3(M[0], M[1], M[2])) / det)
    w = tuple(w)
    return tuple(a[i] + w[i] for i in range(3)), _dot(w, w)


def meb2(pts):
    """Rayon carre exact de la plus petite boule fermee contenant pts (points entiers 3D distincts)."""
    pts = list(pts)
    if len(pts) == 1:
        return Fr(0)
    best = None
    for k in (2, 3, 4):
        if k > len(pts):
            break
        for S in combinations(pts, k):
            cb = circumball(S)
            if cb is None:
                continue
            c, r2 = cb
            if best is not None and r2 >= best:
                continue
            ok = True
            for p in pts:
                d = (p[0] - c[0]) ** 2 + (p[1] - c[1]) ** 2 + (p[2] - c[2]) ** 2
                if d > r2:
                    ok = False
                    break
            if ok:
                best = r2
    exiger(best is not None, 'MEB introuvable')
    return best


class Full(object):
    """FULL_K d'un nuage P (liste de triplets entiers distincts)."""

    def __init__(self, P, K):
        self.P, self.K, self.n = [tuple(p) for p in P], K, len(P)
        exiger(len(set(self.P)) == self.n, 'sites non distincts')
        self._meb = {}
        n = self.n
        self.Ks = list(combinations(range(n), K))
        nk = len(self.Ks)
        self.b = [self.meb(F) for F in self.Ks]
        edges = []
        for i in range(nk):
            Fi = set(self.Ks[i])
            for j in range(i + 1, nk):
                U = tuple(sorted(Fi | set(self.Ks[j])))
                edges.append((self.meb(U), i, j))
        edges.sort()
        births = sorted(range(nk), key=lambda f: self.b[f])
        levels = sorted(set(self.b) | set(e[0] for e in edges))
        uf = list(range(nk))

        def find(x):
            while uf[x] != x:
                uf[x] = uf[uf[x]]
                x = uf[x]
            return x
        alive = [False] * nk
        cur = [None] * nk
        self.first_node = [None] * nk
        self.h, self.children = [], []
        bi = ei = 0
        for L in levels:
            touched = set()
            while bi < nk and self.b[births[bi]] == L:
                alive[births[bi]] = True
                touched.add(births[bi])
                bi += 1
            while ei < len(edges) and edges[ei][0] == L:
                _w, i, j = edges[ei]
                ri, rj = find(i), find(j)
                if ri != rj:
                    uf[rj] = ri
                touched.add(i)
                touched.add(j)
                ei += 1
            roots = set(find(f) for f in touched)
            groups = {}
            for f in range(nk):
                if alive[f]:
                    r = find(f)
                    if r in roots:
                        groups.setdefault(r, []).append(f)
            for r, g in groups.items():
                olds = set(cur[f] for f in g if cur[f] is not None)
                if not olds:
                    nd = len(self.h)
                    self.h.append(L)
                    self.children.append([])
                elif len(olds) == 1:
                    nd = olds.pop()
                else:
                    nd = len(self.h)
                    self.h.append(L)
                    self.children.append(sorted(olds))
                for f in g:
                    cur[f] = nd
                    if self.first_node[f] is None:
                        self.first_node[f] = nd
        nn = len(self.h)
        self.parent = [-1] * nn
        for v in range(nn):
            for c in self.children[v]:
                self.parent[c] = v
        roots = [v for v in range(nn) if self.parent[v] < 0]
        exiger(len(roots) == 1, 'foret non connexe')
        self.root = roots[0]
        self.death = [self.h[self.parent[v]] if self.parent[v] >= 0 else None for v in range(nn)]
        # sous-arbres des K-parties
        self.sub = [[] for _ in range(nn)]
        for f in range(nk):
            v = self.first_node[f]
            while v >= 0:
                self.sub[v].append(f)
                v = self.parent[v]
        # couverture c_x(nu)
        self.cov = [[None] * nn for _ in range(n)]
        for x in range(n):
            Mx = [self.meb(tuple(sorted(set(F) | {x}))) for F in self.Ks]
            for v in range(nn):
                if not self.sub[v]:
                    continue
                c = max(self.h[v], min(Mx[f] for f in self.sub[v]))
                if self.death[v] is None or c < self.death[v]:
                    self.cov[x][v] = c
        self._qual = {}

    def meb(self, idx):
        idx = tuple(idx)
        if idx not in self._meb:
            self._meb[idx] = meb2([self.P[i] for i in idx])
        return self._meb[idx]

    # ---------------------------------------------------------------- arbre
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
        raise VerifErreur('lca')

    def alive_anc(self, v, level):
        """Ancetre de v vivant a level (niveau carre) : h <= level < mort."""
        exiger(self.h[v] <= level, 'noeud non ne')
        while self.parent[v] >= 0 and self.h[self.parent[v]] <= level:
            v = self.parent[v]
        return v

    def alive_anc_r(self, v, r):
        exiger(R.rac(self.h[v]).cmp(r) <= 0, 'noeud non ne (rayon)')
        while self.parent[v] >= 0 and R.rac(self.h[self.parent[v]]).cmp(r) <= 0:
            v = self.parent[v]
        return v

    def meet(self, a, la, b, lb):
        w = self.lca(a, b)
        if w in (a, b):
            return max(la, lb)
        return max(la, lb, self.h[w])

    def qual(self, m):
        """q_m(nu) : niveau ou nu couvre au moins m sites (None si jamais avant sa mort)."""
        if m not in self._qual:
            out = []
            for v in range(len(self.h)):
                cs = sorted(self.cov[x][v] for x in range(self.n) if self.cov[x][v] is not None)
                out.append(cs[m - 1] if len(cs) >= m else None)
            self._qual[m] = out
        return self._qual[m]

    def covered(self, v, level):
        return frozenset(x for x in range(self.n) if self.cov[x][v] is not None and self.cov[x][v] <= level)


# -------------------------------------------------------------------- regles

def qualified_points(T, x, m):
    """{nu: s_nu} : premier niveau ou nu couvre x en etant qualifie (au moins m sites), dans sa vie."""
    q = T.qual(m)
    S = {}
    for v in range(len(T.h)):
        c = T.cov[x][v]
        if c is None or q[v] is None:
            continue
        s = max(c, q[v])
        if T.death[v] is not None and s >= T.death[v]:
            continue
        S[v] = s
    return S


def rule_first(T, m):
    """P2 / first : premiere couverture (qualifiee a m), LCA des ex aequo, entree a max(t, h(LCA))."""
    out = []
    for x in range(T.n):
        S = qualified_points(T, x, m)
        t = min(S.values())
        ties = sorted(v for v, s in S.items() if s == t)
        w = ties[0]
        for v in ties[1:]:
            w = T.lca(w, v)
        out.append((max(t, T.h[w]), w))
    return out


def rule_core(T):
    out = []
    for x in range(T.n):
        d2 = sorted(_dot(_sub(T.P[x], T.P[y]), _sub(T.P[x], T.P[y])) for y in range(T.n))
        Dk = Fr(d2[T.K - 1])
        near = sorted(range(T.n), key=lambda y: _dot(_sub(T.P[x], T.P[y]), _sub(T.P[x], T.P[y])))[:T.K]
        F = tuple(sorted(near))
        f = T.Ks.index(F)
        exiger(T.b[f] <= Dk, 'core : K-partie non nee')
        out.append((Dk, T.alive_anc(T.first_node[f], Dk)))
    return out


def rule_margin_sq(T, m, kappa=1):
    """H_m (kappa = 1) et famille H4 : e = sup_q m(p, q) - kappa (h(q) - t), niveaux carres."""
    out = []
    for x in range(T.n):
        S = qualified_points(T, x, m)
        t = min(S.values())
        o = min(v for v, s in S.items() if s == t)
        e = t
        for v, s in S.items():
            val = T.meet(o, t, v, s) - kappa * (s - t)
            if val > e:
                e = val
        out.append((e, T.alive_anc(o, e)))
    return out


def rule_margin_r(T, m, kappa=1):
    """Meme famille, marge en rayon : e = sup_q sqrt(m(p, q)) - kappa (sqrt(h(q)) - sqrt(t)) (objets R exacts)."""
    out = []
    for x in range(T.n):
        S = qualified_points(T, x, m)
        t = min(S.values())
        o = min(v for v, s in S.items() if s == t)
        rt = R.rac(t)
        e = rt
        for v, s in S.items():
            val = R.rac(T.meet(o, t, v, s)) - (R.rac(s) - rt).mul(Fr(kappa))
            if val.cmp(e) > 0:
                e = val
        out.append((e, T.alive_anc_r(o, e)))
    return out


def ultra_sq(T, entries):
    n = len(entries)
    U = [[None] * n for _ in range(n)]
    for i in range(n):
        ei, oi = entries[i]
        U[i][i] = ei
        for j in range(i + 1, n):
            ej, oj = entries[j]
            U[i][j] = U[j][i] = T.meet(oi, ei, oj, ej)
    return U


def ultra_r(T, entries):
    n = len(entries)
    U = [[None] * n for _ in range(n)]
    for i in range(n):
        ei, oi = entries[i]
        U[i][i] = ei
        for j in range(i + 1, n):
            ej, oj = entries[j]
            w = T.lca(oi, oj)
            val = ei if ei.cmp(ej) >= 0 else ej
            if w not in (oi, oj):
                bw = R.rac(T.h[w])
                if bw.cmp(val) > 0:
                    val = bw
            U[i][j] = U[j][i] = val
    return U


class Hier(object):
    """Hierarchie de points lue sur une ultrametrique ; valeurs en R (rayons exacts)."""

    def __init__(self, U, kind):
        n = len(U)
        self.n = n
        if kind == 'sq':
            self.U = [[R.rac(U[i][j]) for j in range(n)] for i in range(n)]
        else:
            self.U = U
        self.dates = [self.U[i][i] for i in range(n)]
        vals = []
        for i in range(n):
            for j in range(i, n):
                v = self.U[i][j]
                if not any(v.cmp(w) == 0 for w in vals):
                    vals.append(v)
        # tri exact par insertion
        out = []
        for v in vals:
            k = len(out)
            while k > 0 and out[k - 1].cmp(v) > 0:
                k -= 1
            out.insert(k, v)
        self._chg = out

    def rayons_changement(self):
        return self._chg

    def partition(self, r):
        n = self.n
        act = [self.U[i][i].cmp(r) <= 0 for i in range(n)]
        lab = list(range(n))
        for i in range(n):
            if not act[i]:
                continue
            for j in range(i + 1, n):
                if act[j] and self.U[i][j].cmp(r) <= 0:
                    a, b = lab[i], lab[j]
                    if a != b:
                        lab = [a if z == b else z for z in lab]
        g = {}
        for i in range(n):
            g.setdefault(('c', lab[i]) if act[i] else ('s', i), []).append(i)
        return tuple(sorted(tuple(sorted(v)) for v in g.values()))
