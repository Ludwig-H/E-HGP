#!/usr/bin/env python3
"""Confrontation exacte (Fraction) du Theoreme 5 / de la Proposition 6 du manuscrit (K-arbre couvrant minimal du
K-graphe de Gabriel) aux K-polyedres de Cech, sur de petits nuages.

Pour chaque rayon critique r (au carre) :
  - K-polyedres non triviaux de Cech : ensembles de points des composantes de Gamma_K(r) ayant au moins 2 sommets ;
  - composantes non triviales du K-graphe de Gabriel elague a r (Def. 29) : sommets = K-parties facettes d'au moins un
    (K+1)-simplexe de Gabriel (Def. 28 : boule minimale ouverte vide de tout point hors du simplexe), aretes = clique
    des facettes de chaque simplexe de Gabriel de rayon <= r.
Position generale (Def. 26) verifiee : aucun point hors de sigma sur le bord de la boule minimale de sigma, |sigma| >= 2.
"""
import itertools
import sys
import random
sys.path.insert(0, __file__.rsplit('/', 1)[0])
from l02_judge import Geo, _d2  # MEB exacte du juge
from fractions import Fraction as Fr

comb = itertools.combinations


def general_position(P, geo, maxsize):
    n = len(P)
    bad = []
    for s in range(2, min(maxsize, n) + 1):
        for S in comb(range(n), s):
            rad, c = geo.meb(S)
            for x in range(n):
                if x not in S and _d2(c, P[x]) == rad:
                    bad.append((S, x))
    return bad


def components(vertices, edges):
    par = {v: v for v in vertices}

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    for fs in edges:
        r0 = find(fs[0])
        for f in fs[1:]:
            r = find(f)
            if r != r0:
                par[r] = r0
    comps = {}
    for v in vertices:
        comps.setdefault(find(v), []).append(v)
    return list(comps.values())


def nontrivial_pointsets(vertices, edges):
    out = []
    for comp in components(vertices, edges):
        if len(comp) >= 2:
            pts = set()
            for F in comp:
                pts.update(F)
            out.append(frozenset(pts))
    return sorted(out, key=lambda s: sorted(s))


def compare(P, K, verbose=True, names=None):
    n = len(P)
    geo = Geo(P)
    nm = (lambda S: ''.join(names[i] for i in S)) if names else (lambda S: str(tuple(S)))
    beta = {F: geo.meb(F)[0] for F in comb(range(n), K)}
    cof = {}
    gab = {}
    for G in comb(range(n), K + 1):
        rad, c = geo.meb(G)
        cof[G] = rad
        gab[G] = all(_d2(c, P[x]) >= rad for x in range(n) if x not in G)
    gab_vertices = set()
    for G, is_g in gab.items():
        if is_g:
            for i in range(K + 1):
                gab_vertices.add(G[:i] + G[i + 1:])
    levels = sorted(set(beta.values()) | set(cof.values()))
    diffs = []
    for r in levels:
        cech_v = [F for F in beta if beta[F] <= r]
        cech_e = [[G[:i] + G[i + 1:] for i in range(K + 1)] for G in cof if cof[G] <= r]
        want = nontrivial_pointsets(cech_v, cech_e)
        gv = [F for F in gab_vertices if beta[F] <= r]
        ge = [[G[:i] + G[i + 1:] for i in range(K + 1)] for G in cof if gab[G] and cof[G] <= r]
        got = nontrivial_pointsets(gv, ge)
        if want != got:
            diffs.append((r, want, got))
    if verbose:
        print('K=%d n=%d : %d niveaux critiques, %d simplexes de Gabriel sur %d, %d niveaux en desaccord' % (
            K, n, len(levels), sum(gab.values()), len(gab), len(diffs)))
        for r, want, got in diffs[:6]:
            print('  r2=%s (=%.4f)\n     Cech    : %s\n     Gabriel : %s' % (
                r, float(r), [nm(sorted(s)) for s in want], [nm(sorted(s)) for s in got]))
        if diffs:
            r, want, got = diffs[-1]
            print('  dernier niveau en desaccord r2=%s : Cech %s ; Gabriel %s' % (
                r, [nm(sorted(s)) for s in want], [nm(sorted(s)) for s in got]))
    return diffs


def main():
    E5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    print('== E5', E5)
    geo = Geo(E5)
    print('   position generale (Def. 26), violations :', general_position(E5, geo, 5))
    for K in (1, 2, 3, 4):
        compare(E5, K, names='ABCDE')
    # exemple plan construit pour l'audit : la facette AC a deux intrus z, z' dans sa boule diametrale, puis
    # participe au triangle de Gabriel ACw
    X = [(-100, 0, 0), (100, 0, 0), (1, -90, 0), (30, -85, 0), (3, 300, 0)]
    print('== exemple plan a 5 points (A, C, z, y, w)', X)
    geo = Geo(X)
    print('   position generale (Def. 26), violations :', general_position(X, geo, 5))
    compare(X, 2, names='ACzyw')
    # recherche aleatoire : frequence des desaccords en position generale
    rnd = random.Random(20261002)
    for (n, K, dim2) in ((5, 2, True), (6, 2, True), (6, 2, False), (7, 3, False), (7, 2, False)):
        tot = bad = final_bad = skipped = 0
        for _ in range(300):
            P = set()
            while len(P) < n:
                P.add((rnd.randint(0, 2000), rnd.randint(0, 2000), 0 if dim2 else rnd.randint(0, 2000)))
            P = sorted(P)
            geo = Geo(P)
            if general_position(P, geo, K + 2):
                skipped += 1
                continue
            d = compare(P, K, verbose=False)
            tot += 1
            if d:
                bad += 1
                # desaccord encore present au dernier niveau critique ?
                lv = sorted(set(geo.meb(F)[0] for F in comb(range(n), K)) | set(geo.meb(G)[0] for G in comb(range(n), K + 1)))
                if d[-1][0] == lv[-1]:
                    final_bad += 1
        print('aleatoire n=%d K=%d %s : %d nuages en position generale (%d ecartes), %d avec desaccord, %d encore en '
              'desaccord au dernier niveau' % (n, K, 'plan' if dim2 else '3D', tot, skipped, bad, final_bad))


if __name__ == '__main__':
    main()
