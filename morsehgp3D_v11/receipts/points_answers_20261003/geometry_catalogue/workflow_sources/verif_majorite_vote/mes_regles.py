#!/usr/bin/env python3
"""Verification adverse (label verif_majorite_vote) : regles reecrites ICI depuis leurs definitions ecrites.

Aucun code de regle du rapport majorite_vote n'est importe. Arbre : vfull.Arbre de la v10 (birth, parent, death,
children, cov[x] = {v : c_x(v)}, n) ou l'adaptateur depuis l'oracle v11 (arbre_v11). Rayons exacts : vrad.R (v10).

Regles :
  er0h(T, x, eta, kappa, relatif=False) : definition du VERDICT_FINAL v10, section 2.2, points 1 a 6, par balayage
      direct des niveaux (pas de forme close, pas de selection a deux medianes) ; relatif=True : remise
      kappa' * mu * (sqrt(e) - sqrt(A)) (ER0hr du rapport, section 3.4).
  hm_niveau(T, x, m) : H_m a marge en NIVEAU (premiere version du developpeur, = 'margin' de points_reference).
  hm_rayon(T, x, m) : H^r_m a marge en RAYON (version retenue par le developpeur au 3 oct.).
Aucun assert : comportement identique sous python3 -O.
"""
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
VER = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'
if VER not in sys.path:
    sys.path.insert(0, VER)
from vrad import R  # noqa: E402
import vfull  # noqa: E402


class Erreur(RuntimeError):
    pass


def exiger(c, m):
    if not c:
        raise Erreur(m)


def anc(T, v, s):
    """Ancetre de v vivant au niveau s (coupe fermee)."""
    exiger(T.birth[v] <= s, 'noeud non ne')
    while T.parent[v] >= 0 and T.birth[T.parent[v]] <= s:
        v = T.parent[v]
    return v


def anc_r(T, v, t):
    """Ancetre de v vivant au rayon t (R)."""
    exiger(R.rac(T.birth[v]).cmp(t) <= 0, 'noeud non ne au rayon')
    while T.parent[v] >= 0 and R.rac(T.birth[T.parent[v]]).cmp(t) <= 0:
        v = T.parent[v]
    return v


def lca(T, a, b):
    sa = set()
    u = a
    while u >= 0:
        sa.add(u)
        u = T.parent[u]
    u = b
    while u not in sa:
        u = T.parent[u]
    return u


def rmax(a, b):
    return a if a.cmp(b) >= 0 else b


# ------------------------------------------------------------------ ER0h / ER0hr

def er0h(T, x, eta, kappa, relatif=False, details=False, sans_heritage=False):
    eta, kappa = Fraction(eta), Fraction(kappa)
    cov = T.cov[x]
    A = min(cov.values())
    E2 = (1 + eta) * A
    votes = {}
    for v, c in cov.items():
        if c >= E2:
            continue
        d = T.death[v]
        hi = E2 if d is None else min(d, E2)
        w = hi - c
        if w > 0:
            votes[v] = (c, w)
    exiger(votes, 'aucun vote')
    W = sum(w for _c, w in votes.values())
    enf = {v: [] for v in votes}
    racines = []
    for v in votes:
        p = T.parent[v]
        if p >= 0 and p in votes:
            enf[p].append(v)
        else:
            racines.append(v)
    ordre = sorted(votes, key=lambda v: T.birth[v])
    S = {}
    for v in ordre:
        S[v] = votes[v][1] + sum((S[c] for c in enf[v]), Fraction(0))
    recu = {v: Fraction(0) for v in votes}
    credit = {}
    if sans_heritage:
        ordre = []
        credit = {v: votes[v][1] for v in votes}
    for v in reversed(ordre):
        tot = votes[v][1] + recu[v]
        if not enf[v]:
            credit[v] = tot
            continue
        den = S[v] - votes[v][1]
        for c in enf[v]:
            recu[c] += tot * S[c] / den
    exiger(sum(credit.values()) == W, 'conservation')
    # niveaux candidats : debuts des feuilles et naissances de leurs ancetres
    niv = set()
    for l in credit:
        niv.add(votes[l][0])
        u = l
        while u >= 0:
            niv.add(T.birth[u])
            u = T.parent[u]
    niv = sorted(s for s in niv if s >= A)

    def masses(s):
        m = {}
        for l, cr in credit.items():
            if votes[l][0] <= s:
                C = anc(T, l, s)
                m[C] = m.get(C, Fraction(0)) + cr
        return m
    T_half = O = None
    for s in niv:
        m = masses(s)
        g = [C for C, mm in m.items() if 2 * mm > W]
        if g:
            exiger(len(g) == 1, 'deux majorites')
            T_half, O = s, g[0]
            break
    exiger(T_half is not None, 'aucune majorite')
    a = R.rac(A)
    date = R.rac(T_half)
    G_prev = masses(T_half)[O]
    termes = []
    for s in niv:
        if s <= T_half:
            continue
        G = masses(s).get(anc(T, O, s), Fraction(0))
        exiger(G >= G_prev, 'lignee decroissante')
        if G > G_prev and G_prev < W:
            mu = 2 * G_prev / W - 1
            exiger(mu > 0, 'mu <= 0')
            if relatif:
                val = R.rac(s) - (R.rac(s) - a).mul(kappa * mu)
            else:
                val = R.rac(s) - a.mul(kappa * mu)
            termes.append((s, mu))
            date = rmax(date, val)
        G_prev = G
    owner = anc_r(T, O, date)
    if details:
        return date, owner, {'A': A, 'W': W, 'T_half': T_half, 'O': O, 'termes': termes, 'credits': credit,
                             'votes': votes}
    return date, owner


# ------------------------------------------------------------------ H_m (niveau) et H^r_m (rayon)

def _qualifies(T, x, m):
    """{v : q_x(v)} avec q_x(v) = max(c_x(v), m-ieme plus petit c_y(v)) < d_v."""
    out = {}
    for v, c in T.cov[x].items():
        cs = sorted(T.cov[y][v] for y in range(T.n) if v in T.cov[y])
        if len(cs) < m:
            continue
        q = max(c, cs[m - 1])
        d = T.death[v]
        if d is None or q < d:
            out[v] = q
    return out


def _meet(T, v1, t, v, a):
    w = lca(T, v1, v)
    if w in (v1, v):
        return max(t, a)
    return max(t, a, T.birth[w])


def hm_niveau(T, x, m):
    Q = _qualifies(T, x, m)
    exiger(Q, 'aucun point qualifie')
    t = min(Q.values())
    v1 = min(v for v, q in Q.items() if q == t)
    D = max(_meet(T, v1, t, v, q) - q for v, q in Q.items())
    e = t + D
    return R.rac(e), anc(T, v1, e)


def hm_rayon(T, x, m):
    Q = _qualifies(T, x, m)
    exiger(Q, 'aucun point qualifie')
    t = min(Q.values())
    v1 = min(v for v, q in Q.items() if q == t)
    D = R()
    for v, q in Q.items():
        D = rmax(D, R.rac(_meet(T, v1, t, v, q)) - R.rac(q))
    e = R.rac(t) + D
    return e, anc_r(T, v1, e)


# ------------------------------------------------------------------ arbres

def full(points, K):
    T, info = vfull.full_gamma([tuple(p) for p in points], K)
    return T, info


def arbre_v11(points, K):
    """Arbre de l'oracle v11 (hgp11_ref.Definition), meme interface que vfull.Arbre."""
    base = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
    if base not in sys.path:
        sys.path.insert(0, base)
    from hgp11_ref import Definition
    res = Definition([tuple(p) for p in points]).order(K)
    n = len(points)
    birth = [nd.level for nd in res.nodes]
    parent = [-1] * len(birth)
    children = [list(nd.children) for nd in res.nodes]
    for v, nd in enumerate(res.nodes):
        for c in nd.children:
            parent[c] = v
    cov = [dict() for _ in range(n)]
    for cut in res.cuts:
        for v, mask, _core in cut.closed:
            for i in range(n):
                if mask >> i & 1 and v not in cov[i]:
                    cov[i][v] = cut.level
    return vfull.Arbre(birth, parent, children, cov, n)


def u_niveau(T, hang, i, j):
    """Ultrametrique en NIVEAU (flottant haute precision via R -> on rend un R au carre impossible : on rend le
    rayon R) : meet des pendaisons (rayon)."""
    ti, oi = hang[i]
    tj, oj = hang[j]
    w = lca(T, oi, oj)
    val = rmax(ti, tj)
    if w not in (oi, oj):
        val = rmax(val, R.rac(T.birth[w]))
    return val


if __name__ == '__main__':
    # autotest minimal : {0,2,4} a K = 2 (P5 du contrat)
    for pts in ([(0, 0, 0), (2, 0, 0), (4, 0, 0)], [(0, 0, 0), (2000, 0, 0), (4001, 0, 0)]):
        T, _ = full(pts, 2)
        T11 = arbre_v11(pts, 2)
        print(pts, [str(b) for b in T.birth], [str(b) for b in T11.birth])
        for x in range(3):
            print(' x', x, 'ER0h', float(er0h(T, x, 1, 12)[0]), 'ER0hr', float(er0h(T, x, 1, 10, True)[0]),
                  'H1', float(hm_niveau(T, x, 1)[0]), 'Hr1', float(hm_rayon(T, x, 1)[0]))
