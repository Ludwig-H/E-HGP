# Oracle independant : MEB exacte (Fraction) et test de Gabriel (Def. 28) pour les
# 4-sous-ensembles contenant une facette donnee.
import sys, json, itertools
from fractions import Fraction as F
import numpy as np
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M

def solve(A, b):
    n = len(A); A = [row[:] + [bb] for row, bb in zip(A, b)]
    for c in range(n):
        p = next((r for r in range(c, n) if A[r][c] != 0), None)
        if p is None: return None
        A[c], A[p] = A[p], A[c]
        for r in range(n):
            if r != c and A[r][c] != 0:
                f = A[r][c] / A[c][c]
                A[r] = [x - f * y for x, y in zip(A[r], A[c])]
    return [A[i][n] / A[i][i] for i in range(n)]

def circ(S):
    # centre de la circumsphere de S dans aff(S) : c = p0 + sum l_i (p_i - p0)
    p0 = S[0]; V = [[pi[j] - p0[j] for j in range(3)] for pi in S[1:]]
    m = len(V)
    G = [[sum(V[i][t] * V[j][t] for t in range(3)) for j in range(m)] for i in range(m)]
    rhs = [G[i][i] / 2 for i in range(m)]
    lam = solve(G, rhs)
    if lam is None: return None
    c = [p0[t] + sum(lam[i] * V[i][t] for i in range(m)) for t in range(3)]
    r2 = sum((c[t] - p0[t]) ** 2 for t in range(3))
    return c, r2

def meb(P):
    best = None
    for s in range(1, len(P) + 1):
        for S in itertools.combinations(P, s):
            if s == 1:
                c, r2 = list(S[0]), F(0)
            else:
                res = circ(list(S))
                if res is None: continue
                c, r2 = res
            if all(sum((c[t] - q[t]) ** 2 for t in range(3)) <= r2 for q in P):
                if best is None or r2 < best[1]: best = (c, r2)
    return best

pts = np.fromfile(sys.argv[1], dtype='<u4').reshape(-1, 3)
X = [tuple(F(int(v)) for v in p) for p in pts]
facets = [tuple(int(v) for v in f.split(',')) for f in sys.argv[2:]]
rep = json.load(open(sys.argv[1] + '.json'))
cof, gab, K = M.read_export(rep)
cofset = {v: b for v, b in cof}
for tau in facets:
    found = []
    for z in range(len(X)):
        if z in tau: continue
        sig = tuple(sorted(tau + (z,)))
        c, r2 = meb([X[i] for i in sig])
        inside = [i for i in range(len(X)) if i not in sig and sum((c[t] - X[i][t]) ** 2 for t in range(3)) < r2]
        if not inside:
            found.append((sig, r2, sig in cofset))
    print('facette', tau, 'cofaces de Gabriel (oracle):', [(s, str(r), inexp) for s, r, inexp in found])
