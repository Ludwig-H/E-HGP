"""Oracle borne (n <= 16) : quel catalogue de cofaces preserve pi0 de Gamma_K (Cech) ?

Catalogues compares, a K fixe :
  cech     : toutes les (K+1)-parties (verite, Def. 21 + Prop. 5 du manuscrit)
  gabriel  : (K+1)-parties dont la boule minimale ouverte est vide d'autres points (Def. 28, Alg. 1)
  vor      : (K+1)-parties a cellule de Voronoi d'ordre K+1 non vide (catalogue de HGP-old/HGP-C3D)
Graphe : sommets = facettes (K-parties) du catalogue, aretes = cofaces au niveau MEB(sigma).
Critere : a chaque niveau critique r, la partition des K-parties de MEB <= r induite par le
catalogue (restreinte a ses sommets) egale celle de Cech, et toute composante non triviale de
Cech contient un sommet du catalogue. Flottant double sur points generiques : sonde, pas preuve.
"""
import itertools, sys, json
import numpy as np
from scipy.optimize import linprog

def ball_through(P):
    P = np.asarray(P, float)
    if len(P) == 1:
        return P[0], 0.0
    A = P[1:] - P[0]
    G = A @ A.T
    rhs = 0.5 * np.einsum('ij,ij->i', A, A)
    try:
        lam = np.linalg.solve(G, rhs)
    except np.linalg.LinAlgError:
        return None, None
    c = P[0] + lam @ A
    return c, float(np.sum((P[0] - c) ** 2))

def meb2(P):
    P = np.asarray(P, float)
    best = None
    for s in range(1, min(4, len(P)) + 1):
        for S in itertools.combinations(range(len(P)), s):
            c, r2 = ball_through(P[list(S)])
            if c is None:
                continue
            if np.all(np.sum((P - c) ** 2, axis=1) <= r2 * (1 + 1e-12) + 1e-15):
                if best is None or r2 < best[1]:
                    best = (c, r2)
    return best

def vor_nonempty(X, sigma):
    inside = list(sigma); outside = [i for i in range(len(X)) if i not in sigma]
    if not outside:
        return True
    A, b = [], []
    for q in inside:
        for x in outside:
            A.append(list(2 * (X[x] - X[q])) + [1.0])
            b.append(float(X[x] @ X[x] - X[q] @ X[q]))
    res = linprog([0, 0, 0, -1.0], A_ub=A, b_ub=b, bounds=[(-50, 50)] * 3 + [(None, 1.0)], method='highs')
    return res.status == 0 and -res.fun > 1e-9

class UF:
    def __init__(s, items): s.p = {i: i for i in items}
    def f(s, a):
        while s.p[a] != a:
            s.p[a] = s.p[s.p[a]]; a = s.p[a]
        return a
    def u(s, a, b): s.p[s.f(a)] = s.f(b)

def run(n, K, seed):
    rng = np.random.default_rng(seed)
    X = rng.uniform(0, 1, size=(n, 3))
    ksets = list(itertools.combinations(range(n), K))
    kb = {t: meb2(X[list(t)]) for t in ksets}
    cof = {}
    for s in itertools.combinations(range(n), K + 1):
        c, r2 = meb2(X[list(s)])
        others = [i for i in range(n) if i not in s]
        gab = all(np.sum((X[i] - c) ** 2) > r2 * (1 + 1e-9) for i in others)
        cof[s] = dict(r2=r2, gab=gab, vor=vor_nonempty(X, s))
    levels = sorted({v[1] for v in kb.values()} | {v['r2'] for v in cof.values()})
    out = {}
    for name in ('gabriel', 'vor'):
        cat = [s for s, v in cof.items() if v[name[:3] if name == 'vor' else 'gab']]
        F = {f for s in cat for f in itertools.combinations(s, K)}
        bad_levels, uncovered_levels, first_bad, pts_bad = 0, 0, None, 0
        for r in levels:
            alive = [t for t in ksets if kb[t][1] <= r]
            ufc = UF(alive)
            for s, v in cof.items():
                if v['r2'] <= r:
                    fs = list(itertools.combinations(s, K))
                    for a, b in zip(fs, fs[1:]):
                        ufc.u(a, b)
            ufg = UF(F)
            for s in cat:
                if cof[s]['r2'] <= r:
                    fs = list(itertools.combinations(s, K))
                    for a, b in zip(fs, fs[1:]):
                        ufg.u(a, b)
            verts = [t for t in F if kb[t][1] <= r]
            # egalite des partitions sur les sommets vivants du catalogue
            pc = {}; pg = {}
            for t in verts:
                pc.setdefault(ufc.f(t), set()).add(t); pg.setdefault(ufg.f(t), set()).add(t)
            same = sorted(map(sorted, pc.values())) == sorted(map(sorted, pg.values()))
            # couverture des composantes non triviales de Cech
            comps = {}
            for t in alive:
                comps.setdefault(ufc.f(t), set()).add(t)
            covered = all(any(t in F for t in c) for c in comps.values() if len(c) > 1)
            # critere du theoreme 5 : ensembles de POINTS des composantes non triviales
            cech_sets = sorted(sorted(set().union(*c)) for c in comps.values() if len(c) > 1)
            gcomp = {}
            for s in cat:
                if cof[s]['r2'] <= r:
                    fs = list(itertools.combinations(s, K))
                    gcomp.setdefault(ufg.f(fs[0]), set()).update(s)
            cat_sets = sorted(sorted(c) for c in gcomp.values())
            if cech_sets != cat_sets:
                pts_bad += 1
            if not same:
                bad_levels += 1
                first_bad = first_bad or r
            if not covered:
                uncovered_levels += 1
        out[name] = dict(catalogue=len(cat), facets=len(F), levels=len(levels),
                         partition_mismatch_levels=bad_levels, pointset_mismatch_levels=pts_bad, uncovered_levels=uncovered_levels)
    return out

if __name__ == '__main__':
    for K in (2, 3):
        for seed in range(int(sys.argv[1]) if len(sys.argv) > 1 else 4):
            print(json.dumps(dict(n=int(sys.argv[2]) if len(sys.argv) > 2 else 12, K=K, seed=seed,
                                  **run(int(sys.argv[2]) if len(sys.argv) > 2 else 12, K, seed))), flush=True)
