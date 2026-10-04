#!/usr/bin/env python3
"""Controle du theoreme de monotonie en z (rapport, § 4.3) : pour lambda = r^(-z), la selection EOM a z2 > z1 raffine
celle a z1 (tout amas selectionne a z2 est descendant ou egal d'un amas selectionne a z1). Le theoreme est prouve ;
ce script en grave des cas et cherche un contre-exemple (il ne le verifie pas exhaustivement).

Deux sources d'arbres condenses :
  - HDBSCAN (scikit-learn) sur nuages gaussiens de 100 a 400 points, EOM flottante (pas d'ex aequo de stabilite
    attendu ; une violation serait examinee a part) ;
  - H^r_{k+1} exacte de l'oracle (petits nuages entiers), EOM certifiee par intervalles (z entier, log, feuilles).

    python3 z_monotonie.py GRAINE NUAGES_SKLEARN NUAGES_ORACLE
"""
import json
import math
import sys

import numpy as np

import modele_lib as ml

ZS = [0.25, 0.5, 1, 1.5, 2, 3, 4, 6, 10, 20]


def eom_float(tg, clusters, z):
    radii = [ml.rfloat(v) for v in tg.radii]

    def S(c):
        d = c['death']
        tot = 0.0
        for i, j in c['join'].items():
            pd = 0.0 if d is None else radii[d] ** (-z)
            tot += radii[j] ** (-z) - pd
        return tot
    memo = {}

    def St(c):
        if c['id'] in memo:
            return memo[c['id']]
        s = S(c)
        if not c['children']:
            out = (s, [c['id']])
        else:
            tot, sel = 0.0, []
            for ch in c['children']:
                a, b = St(clusters[ch])
                tot += a
                sel += b
            out = (s, [c['id']]) if s >= tot else (tot, sel)
        memo[c['id']] = out
        return out
    sel = []
    for c in clusters:
        if c['parent'] is None:
            for ch in c['children']:
                sel += St(clusters[ch])[1]
    return sorted(sel)


def ancestors(clusters, cid):
    out = set()
    while cid is not None:
        out.add(cid)
        cid = clusters[cid]['parent']
    return out


def refines(clusters, fine, coarse):
    """Vrai si chaque amas de `fine` a un ancetre (ou lui-meme) dans `coarse`, et si l'inverse ne viole pas
    l'antichaine (aucun amas de coarse strictement sous un amas de fine)."""
    cs = set(coarse)
    for f in fine:
        if not (ancestors(clusters, f) & cs):
            return False
    fs = set(fine)
    for c in coarse:
        if (ancestors(clusters, c) - {c}) & fs:
            return False
    return True


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261004
    nsk = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    nor = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    rng = np.random.default_rng(seed)
    out = dict(seed=seed, zs=ZS, sklearn=dict(trees=0, pairs=0, violations=[], distinct_selections=0),
               oracle=dict(trees=0, pairs=0, violations=[], distinct_selections=0))
    for c in range(nsk):
        n = int(rng.integers(100, 400))
        g = int(rng.integers(3, 8))
        centers = rng.uniform(0, 12, size=(g, 3))
        lab = rng.integers(0, g, size=n)
        scales = rng.uniform(0.2, 1.5, size=g)
        X = centers[lab] + rng.normal(0, 1, size=(n, 3)) * scales[lab][:, None]
        k = int(rng.integers(1, 8))
        U = ml.hdbscan_ultrametric(X, k)
        for mcs in (3, 5, 10, 20):
            tg = ml.Treegram(n, U, [U[i][i] for i in range(n)])
            clusters, _ = ml.condensed_tree(tg, mcs)
            sels = [eom_float(tg, clusters, z) for z in ZS]
            out['sklearn']['trees'] += 1
            out['sklearn']['distinct_selections'] += len(set(tuple(s) for s in sels))
            for a in range(len(ZS)):
                for b in range(a + 1, len(ZS)):
                    out['sklearn']['pairs'] += 1
                    if not refines(clusters, sels[b], sels[a]):
                        out['sklearn']['violations'].append(dict(cloud=c, mcs=mcs, z1=ZS[a], z2=ZS[b]))
    # oracle exact : petits nuages entiers, k = 2 et 3, criteres A et C
    zs_exact = ['log', 1, 2, 3, 6, 'leaves']
    for c in range(nor):
        n = int(rng.integers(8, 12))
        k = 2 if c % 2 == 0 else 3
        pts = [tuple(int(v) for v in rng.integers(0, 60, size=3)) for _ in range(n)]
        if len(set(pts)) < n:
            continue
        ph = ml.PointHierarchy(pts, k)
        for cname, dates in (('A', list(ph.e)), ('C', list(ph.rho))):
            for mcs in (2, 3, 4):
                tg = ph.treegram(dates, cname)
                clusters, viol = ml.condensed_tree(tg, mcs)
                if viol:
                    raise AssertionError('critere non monotone')
                sels = [ml.eom_select(tg, clusters, z)[0] for z in zs_exact]
                out['oracle']['trees'] += 1
                out['oracle']['distinct_selections'] += len(set(tuple(s) for s in sels))
                for a in range(len(zs_exact)):
                    for b in range(a + 1, len(zs_exact)):
                        out['oracle']['pairs'] += 1
                        if not refines(clusters, sels[b], sels[a]):
                            out['oracle']['violations'].append(dict(cloud=c, crit=cname, mcs=mcs,
                                                                    z1=str(zs_exact[a]), z2=str(zs_exact[b])))
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
