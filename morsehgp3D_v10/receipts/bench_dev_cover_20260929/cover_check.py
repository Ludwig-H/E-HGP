"""Controle logiciel du prototype « premiere couverture » : alpha_K(x)^2 = min des rayons carres des plus petites
boules englobantes des K-parties contenant x, calcule par force brute exacte (Fraction) sur de petits nuages."""
import itertools, os, subprocess, sys, tempfile
from fractions import Fraction
import numpy as np
B = sys.argv[1]


def circumcenter(S):
    s0 = S[0]
    D = [tuple(a - b for a, b in zip(s, s0)) for s in S[1:]]
    m = len(D)
    M = [[2 * sum(a * b for a, b in zip(D[i], D[j])) for j in range(m)] + [sum(a * a for a in D[i])] for i in range(m)]
    for c in range(m):
        piv = next((i for i in range(c, m) if M[i][c] != 0), None)
        if piv is None:
            return None
        M[c], M[piv] = M[piv], M[c]
        for i in range(m):
            if i != c and M[i][c] != 0:
                f = M[i][c] / M[c][c]
                M[i] = [a - f * b for a, b in zip(M[i], M[c])]
    lam = [M[i][m] / M[i][i] for i in range(m)]
    return tuple(s0[t] + sum(lam[j] * D[j][t] for j in range(m)) for t in range(3))


def meb2(pts):
    pts = [tuple(Fraction(int(v)) for v in p) for p in pts]
    best = None
    for q in range(1, min(4, len(pts)) + 1):
        for S in itertools.combinations(pts, q):
            cen = S[0] if q == 1 else circumcenter(list(S))
            if cen is None:
                continue
            rr = max(sum((a - b) ** 2 for a, b in zip(cen, p)) for p in S)
            if all(sum((a - b) ** 2 for a, b in zip(cen, p)) <= rr for p in pts) and (best is None or rr < best):
                best = rr
    return best


rng = np.random.default_rng(5)
bad = checked = 0
for trial in range(4):
    G = np.unique(rng.integers(0, 200, size=(16, 3)), axis=0)
    for K in (2, 3, 4):
        with tempfile.TemporaryDirectory() as t:
            src = os.path.join(t, 'in')
            np.ascontiguousarray(G, dtype='<u4').tofile(src)
            r = subprocess.run([B + '/mhgp10_cluster', src, os.path.join(t, 'o'), '--k=%d' % K, '--mcs=3', '--entry=cover',
                                '--tree=' + os.path.join(t, 'tr')], capture_output=True, text=True)
            if r.returncode:
                print('FAIL', r.stdout, r.stderr)
                bad += 1
                continue
            lines = open(os.path.join(t, 'tr')).read().split('\n')
            L = int(lines[0].split()[1])
            lev = [float(x) for x in lines[1:1 + L]]
            N = int(lines[1 + L].split()[1])
            off = 2 + L + N
            P = int(lines[off].split()[1])
            pr = {}
            for ln in lines[off + 1:off + 1 + P]:
                x, v, rk, w = map(int, ln.split())
                pr[x] = lev[rk]
        n = len(G)
        for x in range(n):
            others = [i for i in range(n) if i != x]
            best = min(meb2([G[x]] + [G[i] for i in c]) for c in itertools.combinations(others, K - 1))
            checked += 1
            if abs(float(best) - pr[x]) > 1e-9 * max(1.0, float(best)):
                bad += 1
                print('ECART', trial, K, x, float(best), pr[x])
print('controles', checked, 'ecarts', bad)
sys.exit(1 if bad else 0)
