#!/usr/bin/env python3
"""Deux triangles de la these sous jitter entier : stabilite de FULL (Gamma_2 exact) contre HDBSCAN officiel.
Graine fixee avant execution ; 300 nuages ; jitter uniforme entier dans [-J, J] sur x et y de chaque point."""
import sys, random
from fractions import Fraction as F
import numpy as np
from sklearn.cluster import HDBSCAN
from gamma_oracle import Gamma

BASE = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)]
rng = random.Random(20261002)
rows = []
for J in (1, 2, 5, 20):
    full_ok = 0
    hd = {}
    N = 300
    for t in range(N):
        X = [(x + rng.randint(-J, J), y + rng.randint(-J, J), 0) for x, y, _ in BASE]
        g = Gamma(X, 2)
        mid = F(1500) ** 2
        full_ok += g.covers(mid) == [[0, 1, 2], [2, 3], [3, 4, 5]]
        P = np.array(X, dtype=float)
        for ms, mcs in ((1, 3), (2, 3), (2, 2)):
            lab = HDBSCAN(min_cluster_size=mcs, min_samples=ms, copy=True).fit_predict(P).tolist()
            ok = lab[0] == lab[1] == lab[2] != -1 and lab[3] == lab[4] == lab[5] != -1 and lab[0] != lab[3]
            hd[(ms, mcs)] = hd.get((ms, mcs), 0) + ok
    rows.append((J, N, full_ok, hd))
    print("J=%d N=%d FULL_K2_ABC_CD_DEF=%d  HDBSCAN_cible_ABC|DEF: %s" % (J, N, full_ok, ", ".join("ms%d/mcs%d=%d" % (k[0], k[1], v) for k, v in sorted(hd.items()))))
