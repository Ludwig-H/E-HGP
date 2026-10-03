#!/usr/bin/env python3
"""Vote de la these (section 9.1) rendu hierarchique, recode ICI (label verif_majorite_vote).

S_tau = somme sur les cofaces sigma (|sigma| = K + 1) de psi(rho(sigma)), psi(t) = t^-p, rho = rayon de naissance
(rayon de la boule minimale) : avec p = 2, psi = 1 / beta (beta = rayon carre). T_x = somme des S_tau, tau contient x.
Faces : 'toutes' (toutes les K-parties, toutes les cofaces) ou 'gabriel' (cofaces dont la boule minimale ouverte ne
contient aucun autre site ; faces = facettes d'au moins une coface de Gabriel).
Mesure de x : atomes S_tau / T_x au point (v(tau), beta(tau)), v(tau) = noeud de FULL de la composante de tau a sa
naissance (vfull, info['sommet_noeud']). V_1/2 : premier niveau ou une composante porte plus de 1/2 (majorite
stricte), date sqrt(T_1/2) ; V_1/2^kappa : cone d'ER0h avec A = min beta(tau), W = 1.
"""
from fractions import Fraction
from itertools import combinations
import sys

sys.dont_write_bytecode = True
import mes_regles as M  # noqa: E402
from vrad import R  # noqa: E402

V11 = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
if V11 not in sys.path:
    sys.path.insert(0, V11)
from hgp11_ref import Definition  # noqa: E402


def faces(points, K, p, mode):
    d = Definition([tuple(q) for q in points])
    n = len(points)
    beta = {}
    gab = {}
    for sig in combinations(range(n), K + 1):
        lv, c, _closed = d.meb(sig)
        beta[sig] = lv
        if mode == 'gabriel':
            inside = False
            for i in range(n):
                if i in sig:
                    continue
                if sum((Fraction(points[i][j]) - c[j]) ** 2 for j in range(3)) < lv:
                    inside = True
                    break
            gab[sig] = not inside
        else:
            gab[sig] = True
    S = {}
    for tau in combinations(range(n), K):
        s = Fraction(0)
        used = False
        for sig in combinations(range(n), K + 1):
            if set(tau) <= set(sig) and gab[sig]:
                used = True
                s += Fraction(1) / beta[sig] ** (p // 2) if p > 0 else Fraction(1)
        if used:
            S[tau] = s
    bt = {tau: d.meb(tau)[0] for tau in S}
    return S, bt


def vote_point(T, info, S, bt, x, kappa=None, details=False):
    Fx = [tau for tau in S if x in tau]
    Tx = sum((S[t] for t in Fx), Fraction(0))
    if Tx > 0:
        atoms = [(bt[t], info['sommet_noeud'][t], S[t] / Tx) for t in Fx]
    else:  # repli (convention 1/T_x = 0 de la these : x sans face) : poids uniformes sur toutes ses K-parties
        Fx = [t for t in info['sommet_noeud'] if x in t]
        atoms = [(info['beta_sommets'][t], info['sommet_noeud'][t], Fraction(1, len(Fx))) for t in Fx]
    niv = set()
    for b, v, _w in atoms:
        niv.add(b)
        u = v
        while u >= 0:
            niv.add(T.birth[u])
            u = T.parent[u]
    A = min(b for b, _v, _w in atoms)
    niv = sorted(s for s in niv if s >= A)

    def masses(s):
        m = {}
        for b, v, w in atoms:
            if b <= s:
                C = M.anc(T, v, s)
                m[C] = m.get(C, Fraction(0)) + w
        return m
    T_half = O = None
    for s in niv:
        m = masses(s)
        g = [C for C, mm in m.items() if 2 * mm > 1]
        if g:
            T_half, O = s, g[0]
            break
    M.exiger(T_half is not None, 'aucune majorite')
    date = R.rac(T_half)
    if kappa is not None:
        a = R.rac(A)
        G_prev = masses(T_half)[O]
        for s in niv:
            if s <= T_half:
                continue
            G = masses(s).get(M.anc(T, O, s), Fraction(0))
            if G > G_prev and G_prev < 1:
                mu = 2 * G_prev - 1
                date = M.rmax(date, R.rac(s) - a.mul(Fraction(kappa) * mu))
            G_prev = G
    owner = M.anc_r(T, O, date)
    if details:
        return date, owner, {'T_half': T_half, 'O': O, 'masses_avant': {str(k): str(v) for k, v in
                                                                         masses(T_half - Fraction(1, 10 ** 12)).items()}}
    return date, owner


def hier(points, K, p, mode, kappa=None):
    T, info = M.full(points, K)
    S, bt = faces(points, K, p, mode)
    return T, [vote_point(T, info, S, bt, x, kappa) for x in range(len(points))], (S, bt, info)
