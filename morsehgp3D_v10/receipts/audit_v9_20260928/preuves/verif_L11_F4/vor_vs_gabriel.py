"""Mesure legere (petit n, oracle seulement) : catalogue ordre-Voronoi K+1
(construction iteree HGP-old/C3D, re-ecrite ici independamment) contre
catalogue Gabriel (MEB sans point strictement interieur hors sigma).
Aucune conclusion de pente : taille unique, role d'existence/ordre de grandeur."""
import itertools
import sys
import numpy as np
from scipy.spatial import Delaunay, ConvexHull
from scipy.optimize import linprog


def meb(P):
    # Welzl naif pour <= 4 points en 3D (petits ensembles)
    import itertools as it
    best = None
    m = len(P)
    for r in range(1, min(4, m) + 1):
        for S in it.combinations(range(m), r):
            Q = P[list(S)]
            if r == 1:
                c = Q[0]; rad2 = 0.0
            else:
                A = 2 * (Q[1:] - Q[0])
                b = (Q[1:] ** 2).sum(1) - (Q[0] ** 2).sum()
                # centre dans l'enveloppe affine : c = Q0 + sum a_i (Qi-Q0)
                D = Q[1:] - Q[0]
                G = D @ D.T
                rhs = 0.5 * (D ** 2).sum(1)
                try:
                    a = np.linalg.solve(G, rhs)
                except np.linalg.LinAlgError:
                    continue
                c = Q[0] + a @ D
                rad2 = ((Q[0] - c) ** 2).sum()
            if ((P - c) ** 2).sum(1).max() <= rad2 * (1 + 1e-12) + 1e-15:
                if best is None or rad2 < best[1]:
                    best = (c, rad2)
    return best


def next_order(X, sets):
    k = len(sets[0])
    C = X[np.array(sets)].mean(1)
    w = C.__pow__(2).sum(1) - (X[np.array(sets)] ** 2).sum(2).mean(1)
    lift = np.c_[C, (C ** 2).sum(1) - w]
    hull = ConvexHull(lift, qhull_options="Qt Qx")
    out = set()
    for simp, eq in zip(hull.simplices, hull.equations):
        if eq[3] >= 0:
            continue
        for a, b in itertools.combinations(simp, 2):
            u = tuple(sorted(set(sets[a]) | set(sets[b])))
            if len(u) == k + 1:
                out.add(u)
    return sorted(out)


def lp_open_cell(X, s):
    s = list(s); others = [i for i in range(len(X)) if i not in s]
    A = []; b = []
    for p in s:
        for q in others:
            A.append(np.r_[2 * (X[q] - X[p]), 1.0])
            b.append((X[q] ** 2).sum() - (X[p] ** 2).sum())
    res = linprog(-np.r_[0, 0, 0, 1.0], A_ub=np.array(A), b_ub=np.array(b),
                  bounds=[(None, None)] * 3 + [(None, 1.0)], method="highs")
    return res.status == 0 and -res.fun > 1e-12


def main(n, K, seed):
    rng = np.random.default_rng(seed)
    X = rng.random((n, 3))
    tri = Delaunay(X)
    edges = set()
    for simp in tri.simplices:
        for a, b in itertools.combinations(sorted(simp), 2):
            edges.add((a, b))
    sets = sorted(edges)
    for _ in range(K - 1):
        sets = next_order(X, sets)
    vor = sets
    rows = []
    for s in vor:
        c, r2 = meb(X[list(s)])
        d2 = ((X - c) ** 2).sum(1)
        mask = np.ones(n, bool); mask[list(s)] = False
        gab = not (d2[mask] < r2 * (1 - 1e-12)).any()
        rows.append((s, r2, gab))
    ng = sum(1 for r in rows if not r[2])
    print(f"n={n} K={K} seed={seed} vor_sets={len(vor)} non_gabriel={ng} frac={ng/len(vor):.3f}")
    # verification LP sur echantillon
    idx = rng.choice(len(vor), size=min(60, len(vor)), replace=False)
    ok = sum(lp_open_cell(X, vor[i]) for i in idx)
    print(f"  LP open-cell check on sample: {ok}/{len(idx)} feasible")
    # scores z=2
    def scores(filtered):
        S = {}
        for s, r2, g in filtered:
            for t in itertools.combinations(s, K):
                S[t] = S.get(t, 0.0) + 1.0 / r2
        T = np.zeros(n)
        for t, v in S.items():
            for x in t:
                T[x] += v
        m = {t: v * sum(1 / T[x] for x in t) for t, v in S.items()}
        return S, m
    Sv, mv = scores(rows)
    Sg, mg = scores([r for r in rows if r[2]])
    common = [t for t in Sg]
    ratS = np.array([Sv[t] / Sg[t] for t in common])
    ratm = np.array([mv[t] / mg[t] for t in common])
    print(f"  facets vor={len(Sv)} gabriel={len(Sg)} only_vor={len(set(Sv)-set(Sg))}")
    print(f"  S_vor/S_gab on common: min={ratS.min():.3f} med={np.median(ratS):.3f} max={ratS.max():.3f}")
    print(f"  m_vor/m_gab on common: min={ratm.min():.3f} med={np.median(ratm):.3f} max={ratm.max():.3f}")
    print(f"  sum m vor={sum(mv.values()):.3f} gab={sum(mg.values()):.3f}")
    if n <= 60:
        # inclusion Gabriel(brute force) subset of Vor
        gab_bf = []
        for s in itertools.combinations(range(n), K + 1):
            c, r2 = meb(X[list(s)])
            d2 = ((X - c) ** 2).sum(1)
            mask = np.ones(n, bool); mask[list(s)] = False
            if not (d2[mask] < r2 * (1 - 1e-12)).any():
                gab_bf.append(s)
        vs = set(vor)
        print(f"  brute-force gabriel={len(gab_bf)} all_in_vor={all(g in vs for g in gab_bf)}"
              f" gab_in_vor_flagged={sum(1 for r in rows if r[2])}")


if __name__ == "__main__":
    main(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]))
