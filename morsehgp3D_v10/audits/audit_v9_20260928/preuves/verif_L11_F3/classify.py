"""Rejoue la sonde L11 et classe chaque niveau de desaccord de partition des facettes.
Type A (retard d'attache) : seules des facettes ISOLEES dans le catalogue (aucune coface du
catalogue vivante) sont groupees par Cech ; en retirant ces singletons, les partitions
coincident. Type B : desaccord entre classes non triviales du catalogue (scission type E5).
"""
import itertools, sys, json
import numpy as np
sys.path.insert(0, '.')
from probe import meb2, vor_nonempty, UF

def run(n, K, seed, X=None):
    if X is None:
        rng = np.random.default_rng(seed)
        X = rng.uniform(0, 1, size=(n, 3))
    n = len(X)
    ksets = list(itertools.combinations(range(n), K))
    kb = {t: meb2(X[list(t)]) for t in ksets}
    cof = {}
    for s in itertools.combinations(range(n), K + 1):
        c, r2 = meb2(X[list(s)])
        others = [i for i in range(n) if i not in s]
        gab = all(np.sum((X[i] - c) ** 2) > r2 * (1 + 1e-9) for i in others)
        cof[s] = dict(r2=r2, gab=gab, vor=vor_nonempty(X, s))
    # facettes "gabriel" au sens ordre K (boule min de t vide d'autres points)
    kgab = {}
    for t in ksets:
        c, r2 = kb[t]
        kgab[t] = all(np.sum((X[i] - c) ** 2) > r2 * (1 + 1e-9) for i in range(n) if i not in t)
    levels = sorted({v[1] for v in kb.values()} | {v['r2'] for v in cof.values()})
    out = {}
    for name in ('gab', 'vor'):
        cat = [s for s, v in cof.items() if v[name]]
        F = {f for s in cat for f in itertools.combinations(s, K)}
        A = B = 0
        late_nongab = late_gab = 0
        pts_bad = 0
        late_facets = set()
        for r in levels:
            alive = [t for t in ksets if kb[t][1] <= r]
            ufc = UF(alive)
            for s, v in cof.items():
                if v['r2'] <= r:
                    fs = list(itertools.combinations(s, K))
                    for a, b in zip(fs, fs[1:]):
                        ufc.u(a, b)
            ufg = UF(F)
            touched = set()
            for s in cat:
                if cof[s]['r2'] <= r:
                    fs = list(itertools.combinations(s, K))
                    touched.update(fs)
                    for a, b in zip(fs, fs[1:]):
                        ufg.u(a, b)
            verts = [t for t in F if kb[t][1] <= r]
            def parts(uf, vs):
                d = {}
                for t in vs:
                    d.setdefault(uf.f(t), set()).add(t)
                return sorted(map(sorted, d.values()))
            if parts(ufc, verts) == parts(ufg, verts):
                continue
            # retirer les facettes isolees du catalogue
            vt = [t for t in verts if t in touched]
            if parts(ufc, vt) == parts(ufg, vt):
                A += 1
                # facettes isolees dans le catalogue mais non isolees dans Cech (restreint)
                pc = {}
                for t in verts:
                    pc.setdefault(ufc.f(t), set()).add(t)
                for t in verts:
                    if t not in touched and len(pc[ufc.f(t)]) > 1:
                        late_facets.add(t)
            else:
                B += 1
        out[name] = dict(levels=len(levels), typeA_levels=A, typeB_levels=B,
                         late_facets=len(late_facets),
                         late_facets_nonGabriel_orderK=sum(1 for t in late_facets if not kgab[t]))
    return out

if __name__ == '__main__':
    if sys.argv[1] == 'e5':
        X = np.array([[0,0,7],[0,9,6],[1,4,0],[0,0,1],[4,1,2]], float)
        for K in (2, 3):
            print(json.dumps(dict(fixture='E5', K=K, **run(5, K, 0, X))), flush=True)
    else:
        n = int(sys.argv[2]); seeds = int(sys.argv[1])
        for K in (2, 3):
            for seed in range(seeds):
                print(json.dumps(dict(n=n, K=K, seed=seed, **run(n, K, seed))), flush=True)
