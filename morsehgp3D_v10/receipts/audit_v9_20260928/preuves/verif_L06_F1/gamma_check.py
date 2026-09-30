"""Verification independante (verif_L06_F1) : pi_0 de L_K par oracle Gamma exact (Fraction),
contre (a) la foret native FULL exportee, (b) l'arbre du consommateur HEAD, conventions gabriel/boundary.
Comptes lus directement sur les arbres (pas de re-union-find cote consommateur)."""
import itertools, json, subprocess, sys, os
from fractions import Fraction as F
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'head'))
import cluster as C, measure as M
B = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'

def det3(m):
    return (m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
            + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]))

def gauss(A, b):
    n = len(b); M_ = [list(A[i]) + [b[i]] for i in range(n)]
    for c in range(n):
        p = next((r for r in range(c, n) if M_[r][c] != 0), None)
        if p is None: return None
        M_[c], M_[p] = M_[p], M_[c]
        piv = M_[c][c]
        M_[c] = [v / piv for v in M_[c]]
        for r in range(n):
            if r != c and M_[r][c] != 0:
                f = M_[r][c]; M_[r] = [x - f*y for x, y in zip(M_[r], M_[c])]
    return [M_[i][n] for i in range(n)]

def circ(P, T):
    """centre du cercle/sphere circonscrit dans l'enveloppe affine de T, poids barycentriques."""
    base = [F(v) for v in P[T[0]]]
    if len(T) == 1: return tuple(base), F(0), [F(1)]
    E = [[F(P[t][j]) - base[j] for j in range(3)] for t in T[1:]]
    G = [[sum(E[i][j]*E[k][j] for j in range(3)) for k in range(len(E))] for i in range(len(E))]
    rhs = [sum(E[i][j]**2 for j in range(3)) / 2 for i in range(len(E))]
    w = gauss(G, rhs)
    if w is None: return None
    c = tuple(base[j] + sum(w[i]*E[i][j] for i in range(len(E))) for j in range(3))
    bary = [1 - sum(w)] + list(w)
    return c, sum((c[j]-base[j])**2 for j in range(3)), bary

def meb_all(P, maxsize):
    n = len(P)
    supports = []
    for q in range(1, 5):
        for T in itertools.combinations(range(n), q):
            r = circ(P, T)
            if r is None: continue
            c, rr, bary = r
            if all(x > 0 for x in bary): supports.append((T, c, rr))
    meb = {}
    for q in range(1, maxsize+1):
        for S in itertools.combinations(range(n), q):
            best = None
            sset = set(S)
            for T, c, rr in supports:
                if not set(T) <= sset: continue
                if best is not None and rr >= best: continue
                if all(sum((F(P[s][j]) - c[j])**2 for j in range(3)) <= rr for s in S):
                    best = rr
            meb[S] = best
    return meb

def gamma_count(meb, n, K, a):
    fac = [T for T in itertools.combinations(range(n), K) if meb[T] <= a]
    idx = {T: i for i, T in enumerate(fac)}; uf = list(range(len(fac)))
    def fd(x):
        while uf[x] != x: uf[x] = uf[uf[x]]; x = uf[x]
        return x
    if K < n:
        for U in itertools.combinations(range(n), K+1):
            if meb[U] <= a:
                s = [idx[tuple(v for v in U if v != d)] for d in U]
                for t in s[1:]: uf[fd(t)] = fd(s[0])
    return len({fd(i) for i in range(len(fac))})

def fr(e): return F(int(e['num']), int(e['den']))

def native_count(nat, a):
    lev = {nd['id']: fr(nd['level']) for nd in nat['nodes']}
    succ = {nd['id']: nd['successor'] for nd in nat['nodes']}
    return sum(1 for i in lev if lev[i] <= a and (succ[i] is None or lev[succ[i]] > a))

def consumer_tree(rep, conv):
    cof, gab, size = M.read_export(rep)
    births = M.facet_births(cof, gab, conv); keep = None if conv == 'boundary' else gab
    facets, pl = C.facet_levels(cof, keep); nodes, roots = C.merge_tree(facets, pl, births)
    level = {f: births[f] for f in facets}
    parent = {}
    for name, nd in nodes.items():
        level[name] = nd['level']
        for ch in nd['children']: parent[ch] = name
    return level, parent, roots

def consumer_count(tree, a):
    level, parent, roots = tree
    return sum(1 for it, l in level.items() if l <= a and (it not in parent or level[parent[it]] > a))

def run_export(P, K, path):
    np.array(P, dtype='<u4').tofile(path)
    out = subprocess.run(['nice', '-n', '19', B, '--input', path, '--k', str(K), '--workers', '1'],
                         capture_output=True, text=True)
    if out.returncode: return None
    return json.loads(out.stdout)

def main():
    rng = np.random.default_rng(int(sys.argv[1]) if len(sys.argv) > 1 else 777)
    trials = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gc.u32le')
    stats = {}
    first = {}
    for t in range(trials):
        n = int(rng.integers(5, 10)); box = int(rng.choice([6, 12, 5000]))
        s = set()
        while len(s) < n: s.add(tuple(int(v) for v in rng.integers(0, box, 3)))
        P = sorted(s); rng.shuffle(P); P = [tuple(p) for p in P]
        meb = meb_all(P, min(n, 5))
        for K in (2, 3):
            if K+1 > n: continue
            rep = run_export(P, K, path)
            if rep is None:
                stats.setdefault((K, 'refused'), 0); stats[(K, 'refused')] += 1; continue
            trees = {conv: consumer_tree(rep, conv) for conv in ('gabriel', 'boundary')}
            levels = sorted(set(v for T, v in meb.items() if len(T) in (K, K+1))) + [max(meb.values()) + 1]
            for a in levels:
                g = gamma_count(meb, n, K, a)
                key = (K, 'native'); stats.setdefault(key, [0, 0]); stats[key][0] += 1; stats[key][1] += (native_count(rep['native'], a) != g)
                for conv, tr in trees.items():
                    c = consumer_count(tr, a)
                    key = (K, conv); stats.setdefault(key, [0, 0]); stats[key][0] += 1
                    if c != g:
                        stats[key][1] += 1
                        first.setdefault(key, (t, n, box, P, str(a), g, c))
            for conv, tr in trees.items():
                key = (K, conv, 'roots_ne_1'); stats.setdefault(key, 0); stats[key] += (len(tr[2]) != 1)
    for k, v in sorted(stats.items(), key=str): print(k, v)
    for k, v in first.items(): print('first', k, v)

main()
