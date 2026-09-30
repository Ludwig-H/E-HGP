"""Verification adverse : l'oracle Line de la porte contre l'oracle exhaustif independant du depot
(reference/hgp10_ref.py : miniboules exactes, Gamma_k par K-parties et (K+1)-parties), sur les nuages des fixtures et
sur de petits nuages alignes aleatoires (graines dev). Compare : partitions core a tous les niveaux critiques,
hauteurs de fusion core, couvertures discretes (coupes fermees de Gamma_k), alpha_K. Aucune modification du correcteur."""
import importlib.util, itertools, random, sys
from fractions import Fraction as Fr
gate_path, ref_dir = sys.argv[1], sys.argv[2]
spec = importlib.util.spec_from_file_location('g', gate_path); g = importlib.util.module_from_spec(spec); spec.loader.exec_module(g)
sys.path.insert(0, ref_dir); import hgp10_ref as H


def check_cloud(xs, k):
    s = sorted(xs); P = [(x, 0, 0) for x in s]; n = len(s)
    line = g.Line(s, k)
    beta = {F: H.meb(P, F)[0] for F in itertools.combinations(range(n), k)}
    cof = {G: H.meb(P, G)[0] for G in itertools.combinations(range(n), k + 1)} if k < n else {}
    entry = [H.entry_level(P, i, k) for i in range(n)]
    knn = [H.knn_vertex(P, i, k) for i in range(n)]
    levels = sorted(set(beta.values()) | set(cof.values()) | set(entry))
    checks = bad = 0
    u = {}
    for a in levels:
        dsu = H.DSU()
        for F, b in beta.items():
            if b <= a:
                dsu.find(F)
        for G, b in cof.items():
            if b <= a:
                fs = [tuple(y for y in G if y != w) for w in G]
                for f in fs[1:]:
                    dsu.union(fs[0], f)
        groups, covers = {}, {}
        for i in range(n):
            if entry[i] <= a:
                groups.setdefault(dsu.find(knn[i]), []).append(s[i])
        for F, b in beta.items():
            if b <= a:
                covers.setdefault(dsu.find(F), set()).update(s[i] for i in F)
        want = sorted(sorted(v) for v in groups.values())
        r = g.root(a)
        checks += 1
        if line.core_partition(r) != want:
            bad += 1; print('ECART core', s, k, a, line.core_partition(r), want)
        wc = sorted(sorted(c) for c in covers.values())
        gc = sorted(line.discrete_cover(c, r) for c in line.components(r))
        checks += 1
        if wc != gc:
            bad += 1; print('ECART couverture', s, k, a, gc, wc)
        for blk in want:
            for x, y in itertools.combinations(blk, 2):
                u.setdefault((x, y), a)
    for x, y in itertools.combinations(s, 2):
        checks += 1
        if (x, y) not in u or line.u_core(x, y) ** 2 != u[(x, y)]:
            bad += 1; print('ECART u_core', s, k, x, y, line.u_core(x, y) ** 2, u.get((x, y)))
    for i, x in enumerate(s):
        best = min(b for F, b in beta.items() if i in F)
        checks += 1
        if line.alpha(x)[0] ** 2 != best:
            bad += 1; print('ECART alpha', s, k, x, line.alpha(x)[0] ** 2, best)
    return checks, bad


tot = ecarts = 0
fixtures = [([0, 2, 4], 2), ([0, 999, 2000], 2), ([0, 1001, 2000], 2), ([0, 20, 22, 50, 52], 1), ([0, 20, 22, 50, 52], 2),
            ([0, 1, 4, 7], 1), ([0, 1, 4, 7], 4), ([1, 2], 2), ([0, 3], 2), ([1, 1001], 2), ([0, 1002], 2), ([7, 1007], 2),
            ([0, 1014], 2), ([0, 100000 - 1, 200000], 2), ([0, 100000 + 1, 200000], 2)]
for xs, k in fixtures:
    c, b = check_cloud(xs, k); tot += c; ecarts += b
print('fixtures : controles %d, ecarts %d' % (tot, ecarts), flush=True)
rng = random.Random(int(sys.argv[3]))
t0, nck = tot, 0
for t in range(int(sys.argv[4])):
    n = rng.randint(2, 6)
    xs = sorted(rng.sample(range(0, 30), n))
    for k in range(1, n + 1):
        c, b = check_cloud(xs, k); tot += c; ecarts += b; nck += 1
print('aleatoires : %d couples (nuage, K), controles %d ; ecarts au total %d' % (nck, tot - t0, ecarts), flush=True)
sys.exit(1 if ecarts else 0)
