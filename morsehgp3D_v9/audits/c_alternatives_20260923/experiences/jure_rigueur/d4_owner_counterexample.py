# Juré rigueur : la règle « propriétaire = plus petit PointId de la COQUILLE » (texte D4)
# peut n'avoir aucune présentation centrée passant par le propriétaire.
# Coquille U = {a, x, y, z} sur la sphère |c|=5, c=0 ; xyz aigu sur le grand cercle z=0,
# a=(0,0,5) hors de ce plan ; ID(a) minimal. On énumère toutes les présentations centrées.
import itertools
from fractions import Fraction as F
pts = {0: (0, 0, 5), 1: (5, 0, 0), 2: (-3, 4, 0), 3: (-3, -4, 0)}  # a=0, x=1, y=2, z=3
def sub(p, q): return tuple(p[i] - q[i] for i in range(3))
def dot(p, q): return sum(p[i] * q[i] for i in range(3))
def solve(m, r):
    n = len(m); a = [list(map(F, row)) + [F(v)] for row, v in zip(m, r)]
    for c in range(n):
        piv = next((i for i in range(c, n) if a[i][c] != 0), None)
        if piv is None: return None
        a[c], a[piv] = a[piv], a[c]
        for i in range(n):
            if i != c and a[i][c] != 0:
                f = a[i][c] / a[c][c]; a[i] = [u - f * w for u, w in zip(a[i], a[c])]
    return [a[i][n] / a[i][i] for i in range(n)]
def meb(S):
    p0 = S[0]; vs = [sub(s, p0) for s in S[1:]]
    lam = solve([[dot(u, v) for v in vs] for u in vs], [F(dot(v, v), 2) for v in vs])
    if lam is None: return None, None
    c = tuple(F(p0[t]) + sum(l * v[t] for l, v in zip(lam, vs)) for t in range(3))
    return c, [1 - sum(lam)] + lam
for q in (2, 3, 4):
    for S in itertools.combinations(sorted(pts), q):
        c, bary = meb([pts[i] for i in S])
        centred = c is not None and all(b > 0 for b in bary)
        if centred:
            same = c == (0, 0, 0)
            print("support", S, "centre", tuple(str(v) for v in c), "même boule" if same else "autre boule")
