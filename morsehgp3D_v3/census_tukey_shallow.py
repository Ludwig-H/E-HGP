# Minorant exact de l'ensemble { p : tau(p) = +inf }, c.-a-d. de profondeur de
# Tukey < s_max : p y est des qu'il est parmi les s_max premiers dans UNE
# direction. On echantillonne D directions ; le resultat est donc un MINORANT.
import numpy as np, sys, json
def shallow_fraction(P, s_max, D=4096, seed=1):
    n = len(P)
    rng = np.random.default_rng(seed)
    g = rng.normal(size=(D,3)); g /= np.linalg.norm(g,axis=1,keepdims=True)
    mark = np.zeros(n, dtype=bool)
    for i in range(0, D, 128):
        proj = P @ g[i:i+128].T              # n x d
        k = min(s_max, n)
        idx = np.argpartition(-proj, k-1, axis=0)[:k]   # k x d
        mark[np.unique(idx)] = True
    return mark
def decimate(P, m, seed=7):
    if len(P) <= m: return P
    rng = np.random.default_rng(seed)
    return P[rng.choice(len(P), m, replace=False)]
def uniform_cube(n, seed=3):
    return np.random.default_rng(seed).random((n,3))
out=[]
for name, P in [("bunny_zipper", np.load("clouds/bunny_zipper.npy")),
                ("bunny_multi_10_captations", np.load("clouds/bunny_multi.npy"))]:
    for m in [5000, 20000, 50000]:
        Q = decimate(P, m)
        if len(Q) < m: continue
        for s_max in [3, 11]:
            mk = shallow_fraction(Q, s_max)
            out.append(dict(cloud=name, n=len(Q), s_max=s_max,
                            shallow=int(mk.sum()), pct=round(100*mk.mean(),3)))
            print(out[-1], flush=True)
for m in [5000, 20000, 50000]:
    Q = uniform_cube(m)
    for s_max in [3, 11]:
        mk = shallow_fraction(Q, s_max)
        out.append(dict(cloud="uniform_cube_volumetrique", n=m, s_max=s_max,
                        shallow=int(mk.sum()), pct=round(100*mk.mean(),3)))
        print(out[-1], flush=True)
json.dump(out, open("shallow.json","w"), indent=1)
