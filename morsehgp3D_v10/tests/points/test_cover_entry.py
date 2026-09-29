"""Porte de l'entree des points par premiere couverture (--entry=cover).

  1. alpha_K(x)^2 = min des rayons carres des plus petites boules englobantes des K-parties contenant x, calcule par
     force brute exacte (Fraction) sur de petits nuages (K = 2, 3, 4) ;
  2. alpha_K(x) <= d_K(x) : l'entree cover n'est jamais plus tardive que l'entree core (sur un nuage moyen) ;
  3. K = 1 : cover et core donnent le meme arbre de points ;
  4. 1 fil = 4 fils (etiquettes et vote).
Ce sont des tests du code (la definition calculee), pas un oracle du clustering.

  python3 test_cover_entry.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import hashlib
import itertools
import os
import subprocess
import sys
import tempfile
from fractions import Fraction

import numpy as np


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


def run(build, G, tmp, tag, args):
    src = os.path.join(tmp, tag + '.in')
    np.ascontiguousarray(G, dtype='<u4').tofile(src)
    out = os.path.join(tmp, tag)
    r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, out] + args, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('%s : code %d %s %s' % (tag, r.returncode, r.stdout, r.stderr))
    return out


def read_tree(path):
    lines = open(path).read().split('\n')
    L = int(lines[0].split()[1])
    levels = [float(x) for x in lines[1:1 + L]]
    N = int(lines[1 + L].split()[1])
    nodes = lines[2 + L:2 + L + N]
    off = 2 + L + N
    P = int(lines[off].split()[1])
    pts = {}
    for ln in lines[off + 1:off + 1 + P]:
        x, v, rk, w = map(int, ln.split())
        pts[x] = (v, levels[rk])
    return levels, nodes, pts


def main():
    build = sys.argv[1]
    bad = []
    rng = np.random.default_rng(11)
    checks = 0
    with tempfile.TemporaryDirectory() as tmp:
        # 1. alpha exact contre force brute
        for trial in range(2):
            G = np.unique(rng.integers(0, 200, size=(14, 3)), axis=0)
            for K in (2, 3, 4):
                run(build, G, tmp, 'a', ['--k=%d' % K, '--mcs=3', '--entry=cover', '--tree=' + os.path.join(tmp, 'ta')])
                _, _, pts = read_tree(os.path.join(tmp, 'ta'))
                for x in range(len(G)):
                    others = [i for i in range(len(G)) if i != x]
                    best = min(meb2([G[x]] + [G[i] for i in c]) for c in itertools.combinations(others, K - 1))
                    checks += 1
                    if abs(float(best) - pts[x][1]) > 1e-9 * max(1.0, float(best)):
                        bad.append('alpha K=%d x=%d : %r contre %r' % (K, x, float(best), pts[x][1]))
        # 2-4 sur un nuage moyen
        G = np.unique(rng.integers(0, 4000, size=(1500, 3)), axis=0)
        for K in (1, 3, 5):
            cfg = os.path.join(tmp, 'cfg')
            open(cfg, 'w').write('39 1.0 eom 0\n39 2.5 eom 0\n')
            run(build, G, tmp, 'core%d' % K, ['--k=%d' % K, '--configs=' + cfg, '--tree=' + os.path.join(tmp, 'tc')])
            _, nodes_c, pts_c = read_tree(os.path.join(tmp, 'tc.k%d' % K))  # --configs : sorties suffixees .k<K>
            hashes = []
            for thr in (1, 4):
                out = run(build, G, tmp, 'cov%d_%d' % (K, thr), ['--k=%d' % K, '--configs=' + cfg, '--entry=cover',
                                                                 '--label=vote', '--threads=%d' % thr,
                                                                 '--tree=' + os.path.join(tmp, 'tv%d' % thr)])
                h = hashlib.sha256(open(os.path.join(tmp, 'tv%d.k%d' % (thr, K)), 'rb').read())
                for i in range(2):
                    for suf in ('', '.vote'):
                        p = out + '.k%d.%d' % (K, i) + suf
                        if os.path.exists(p):
                            h.update(open(p, 'rb').read())
                hashes.append(h.hexdigest())
            if hashes[0] != hashes[1]:
                bad.append('K=%d : sorties differentes a 1 et 4 fils' % K)
            _, nodes_v, pts_v = read_tree(os.path.join(tmp, 'tv1.k%d' % K))
            late = sum(1 for x in pts_c if pts_v[x][1] > pts_c[x][1] * (1 + 1e-12))
            if late:
                bad.append('K=%d : %d points entrent plus tard en cover qu en core' % (K, late))
            if K == 1 and (nodes_c != nodes_v or pts_c != pts_v):
                bad.append('K=1 : cover differe de core')
            checks += len(pts_c)
    print('controles %d ecarts %d' % (checks, len(bad)))
    for b in bad[:20]:
        print('ECART', b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
