#!/usr/bin/env python3
"""Controle de la proposition 1 (rapport, § 1.4) : la condensation a mcs est l'atteignabilite mutuelle de
l'ultrametrique u a min_samples = mcs. Pour chaque site i, le premier rayon ou i est compte dans un gros bloc
(arbre condense, evenements exacts) egale max(R_i, s_i), R_i = mcs-ieme plus petite valeur de { max(u(i, j), s_j) }_j
(R_i pour A) ; et deux sites
sont dans le meme gros bloc au rayon r si et seulement si max(u(i, j), R_i, R_j) <= r. Exact (rangs), petits nuages.

    python3 verif_condensation.py GRAINE NUAGES > ../sorties/verif_condensation.json
"""
from functools import cmp_to_key
import json
import sys

import numpy as np

import modele_lib as ml


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261004
    clouds = int(sys.argv[2]) if len(sys.argv) > 2 else 60
    rng = np.random.default_rng(seed)
    out = dict(seed=seed, clouds=0, sites=0, pairs=0, ecarts_R=0, ecarts_R_A=0, ecarts_paires=0)
    done = 0
    while done < clouds:
        n = int(rng.integers(6, 10))
        k = 2 if done % 2 == 0 else 3
        pts = [tuple(int(v) for v in rng.integers(0, 50, size=3)) for _ in range(n)]
        if len(set(pts)) < n:
            continue
        done += 1
        ph = ml.PointHierarchy(pts, k)
        for cname, dates in (('A', list(ph.e)), ('C', list(ph.rho)), ('B', list(ph.sB))):
            tg = ph.treegram(dates, cname)
            for mcs in (2, 3, 4):
                clusters, viol = ml.condensed_tree(tg, mcs)
                first = {}
                for c in clusters:
                    for i, r in c['join'].items():
                        first[i] = min(first.get(i, 1 << 30), r)
                for i in range(n):
                    vals = sorted(max(tg.urank[i][j], tg.srank[j]) for j in range(n))
                    R = vals[mcs - 1] if mcs <= n else None
                    got = first.get(i)
                    out['sites'] += 1
                    # premier rang ou i est COMPTE dans un gros bloc : max(R_i, s_i) (= R_i pour A, ou s = e <= R)
                    if max(R, tg.srank[i]) != got:
                        out['ecarts_R'] += 1
                    if cname == 'A' and R != got:
                        out['ecarts_R_A'] = out.get('ecarts_R_A', 0) + 1
                # paires : meme gros bloc a chaque rang <=> max(u, R_i, R_j) <= r
                Rk = {}
                for i in range(n):
                    vals = sorted(max(tg.urank[i][j], tg.srank[j]) for j in range(n))
                    Rk[i] = vals[mcs - 1]
                for r in range(len(tg.radii)):
                    big = {}
                    for b in tg.blocks_at(r):
                        if sum(1 for i in b if tg.srank[i] <= r) >= mcs:
                            for i in b:
                                big[i] = b
                    for i in range(n):
                        for j in range(i + 1, n):
                            same = i in big and j in big and big[i] is big[j]
                            pred = max(tg.urank[i][j], Rk[i], Rk[j]) <= r
                            out['pairs'] += 1
                            if same != pred:
                                out['ecarts_paires'] += 1
        out['clouds'] += 1
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
