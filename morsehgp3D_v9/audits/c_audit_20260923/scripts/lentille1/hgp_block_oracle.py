#!/usr/bin/env python3
"""Oracle exact (Fractions) de la tour HGP par definition (Def. 20-22, Prop. 5)
et controle du lemme d'effet de bloc utilise par la tour FULL v7/v9.

Pour chaque ordre K et chaque niveau L (rayon carre exact), on compare :
  - la verite : composantes de Gamma_K(<L) (pre-lot) et Gamma_K(<=L) (post-lot)
    sur TOUS les K-sous-ensembles et adjacences elementaires (K+1) ;
  - la prediction par boules : pour chaque boule B de niveau L (I strict, U
    coquille, q_min), si p+q_min >= K+2 : bloc inerte (Th. 4.2) ; sinon fusion
    des racines pre-lot des K-facettes strictes de I u U, naissance couvrant
    I u U si aucune.
Aucune assertion Python : les echecs sont comptes et le code de sortie est non
nul (valide sous python3 -O)."""
import itertools, os, random, sys
MUTANT = os.environ.get('MUTANT', '')
from fractions import Fraction as F

def sub(a, b): return tuple(x - y for x, y in zip(a, b))
def dot(a, b): return sum(x * y for x, y in zip(a, b))

def solve(G, b):
    n = len(G); M = [list(G[i]) + [b[i]] for i in range(n)]
    for col in range(n):
        piv = next((r for r in range(col, n) if M[r][col] != 0), None)
        if piv is None: return None
        M[col], M[piv] = M[piv], M[col]
        for r in range(n):
            if r != col and M[r][col] != 0:
                f = M[r][col] / M[col][col]
                M[r] = [x - f * y for x, y in zip(M[r], M[col])]
    return [M[i][n] / M[i][i] for i in range(n)]

def circum(S):
    """Centre et R^2 de la plus petite sphere passant par S dans aff(S),
    avec coordonnees barycentriques ; None si S affinement dependant."""
    s0 = S[0]
    if len(S) == 1: return (tuple(F(x) for x in s0), F(0), [F(1)])
    V = [sub(s, s0) for s in S[1:]]
    G = [[F(dot(a, b)) for b in V] for a in V]
    t = solve(G, [F(dot(a, a), 2) for a in V])
    if t is None: return None
    c = tuple(F(s0[k]) + sum(t[i] * V[i][k] for i in range(len(V))) for k in range(3))
    lam = [1 - sum(t)] + t
    return (c, sum((c[k] - s0[k]) ** 2 for k in range(3)), lam)

def power(c, R2, x): return sum((F(x[k]) - c[k]) ** 2 for k in range(3)) - R2

MEB_CACHE = {}
def meb(P):
    key = tuple(sorted(P))
    if key in MEB_CACHE: return MEB_CACHE[key]
    best = None
    for m in range(1, min(4, len(P)) + 1):
        for S in itertools.combinations(key, m):
            r = circum(S)
            if r is None: continue
            c, R2, lam = r
            if best is not None and R2 >= best[1]: continue
            if all(power(c, R2, x) <= 0 for x in key): best = (c, R2)
    MEB_CACHE[key] = best
    return best

def ball_census(pts, c, R2):
    I = [i for i, x in enumerate(pts) if power(c, R2, x) < 0]
    U = [i for i, x in enumerate(pts) if power(c, R2, x) == 0]
    return I, U

def qmin(pts, c, R2, U):
    if R2 == 0: return 1
    for m in range(2, 5):
        for S in itertools.combinations(U, m):
            r = circum([pts[i] for i in S])
            if r is None: continue
            cc, RR, lam = r
            if cc == c and all(l > 0 for l in lam): return m
    return None

class DSU:
    def __init__(s): s.p = {}
    def find(s, x):
        s.p.setdefault(x, x)
        while s.p[x] != x:
            s.p[x] = s.p[s.p[x]]; x = s.p[x]
        return x
    def union(s, a, b):
        a, b = s.find(a), s.find(b)
        if a != b: s.p[a] = b

def check_cloud(pts, kmax, stats, report):
    n = len(pts); idx = range(n)
    level = {}
    for m in range(1, kmax + 2):
        for S in itertools.combinations(idx, m):
            level[S] = meb([pts[i] for i in S])[1]
    for K in range(1, kmax + 1):
        facets = [S for S in itertools.combinations(idx, K)]
        cofaces = [S for S in itertools.combinations(idx, K + 1)]
        levels = sorted(set(level[S] for S in facets) | set(level[S] for S in cofaces))
        def components(cut, strict):
            d = DSU(); present = [S for S in facets if (level[S] < cut if strict else level[S] <= cut)]
            for S in present: d.find(S)
            for Q in cofaces:
                if (level[Q] < cut) if strict else (level[Q] <= cut):
                    fs = list(itertools.combinations(Q, K))
                    for f in fs[1:]: d.union(fs[0], f)
            return d, present
        for L in levels:
            pre, pre_present = components(L, True)
            post, post_present = components(L, False)
            pre_roots = set(pre.find(S) for S in pre_present)
            # verite : partition des racines pre-lot et naissances au niveau L
            true_groups = {}
            for r in pre_roots: true_groups.setdefault(post.find(r), set()).add(r)
            true_births = set(post.find(S) for S in post_present) - set(true_groups)
            true_merges = sorted(len(g) for g in true_groups.values() if len(g) >= 2)
            # boules de niveau L portant >= K sites
            balls = {}
            for S in list(facets) + list(cofaces):
                if level[S] == L:
                    c, R2 = meb([pts[i] for i in S])
                    balls[(c, R2)] = 1
            pred = DSU(); births = []; attached = []
            for (c, R2) in balls:
                I, U = ball_census(pts, c, R2)
                p, u, q = len(I), len(U), qmin(pts, c, R2, U)
                if q is None: report.append(('no_support', pts, K, L)); continue
                if K > p + u: continue
                block = I + U
                strict = [S for S in itertools.combinations(sorted(block), K) if level[S] < L]
                roots = set(pre.find(S) for S in strict)
                cover = set(i for S in strict for i in S)
                qq = u if MUTANT == 'shell_size' else q
                lim = K + 1 if MUTANT == 'window_minus_one' else K + 2
                if p + qq >= lim:
                    stats['inert_checked'] += 1
                    if len(roots) != 1 or cover != set(block):
                        report.append(('th42_violated', pts, K, L, p, u, q, len(roots))); 
                    continue
                stats['admitted'] += 1
                if u > q: stats['extra_shell_admitted'] += 1
                if not roots:
                    births.append((c, R2, p + u, set(block)))
                    stats['births'] += 1
                    if p + u > K: stats['births_cover_gt_K'] += 1
                    if K < p + q:  # naissance sous la fenetre : jamais attendue
                        report.append(('birth_below_window', pts, K, L, p, u, q))
                    if K > 1 and K - 1 < p + q - 1:  # ancre verticale absente a K-1
                        report.append(('no_lower_anchor', pts, K, L, p, u, q))
                else:
                    rs = list(roots)
                    for r in rs[1:]: pred.union(rs[0], r)
                    attached.append((rs[0], set(block)))
            pred_groups = {}
            for r in pre_roots: pred_groups.setdefault(pred.find(r), set()).add(r)
            pm = sorted(len(g) for g in pred_groups.values() if len(g) >= 2)
            # couvertures (Th. 2 : amas discret = union des sommets des facettes)
            precover = {}
            for S in pre_present: precover.setdefault(pre.find(S), set()).update(S)
            postcover = {}
            for S in post_present: postcover.setdefault(post.find(S), set()).update(S)
            predcover = {}
            for r in pre_roots: predcover.setdefault(pred.find(r), set()).update(precover[r])
            for r, pts_block in attached: predcover.setdefault(pred.find(r), set()).update(pts_block)
            for g in pred_groups_placeholder(pre_roots, pred):
                old = set().union(*(precover[r] for r in g)); new = predcover[pred.find(next(iter(g)))]
                if new != old:
                    stats['continuation_gain' if len(g) == 1 else 'merge_gain'] += 1
            if sorted(map(sorted, predcover.values())) != sorted(sorted(postcover[post.find(next(iter(g)))]) for g in pred_groups_placeholder(pre_roots, pred)):
                report.append(('coverage_mismatch', pts, K, L))
            if sorted(sorted(b[3]) for b in births) != sorted(sorted(postcover[x]) for x in true_births):
                report.append(('birth_cover_mismatch', pts, K, L))
            if pm != true_merges or len(births) != len(true_births):
                report.append(('block_lemma_mismatch', pts, K, L, pm, true_merges, len(births), len(true_births)))
            stats['levels'] += 1
            # composantes distinctes a ensembles de points egaux (theta ensembliste)
            comp_pts = {}
            for S in post_present: comp_pts.setdefault(post.find(S), set()).update(S)
            sets = [frozenset(v) for v in comp_pts.values()]
            if len(sets) != len(set(sets)): report.append(('equal_point_sets', pts, K, L))

def pred_groups_placeholder(pre_roots, pred):
    g = {}
    for r in pre_roots: g.setdefault(pred.find(r), set()).add(r)
    return list(g.values())

def e5_check():
    A, B, C, D, E = (0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)
    P = {'A': A, 'B': B, 'C': C, 'D': D, 'E': E}
    out = {}
    for name in ['CDE', 'ADE', 'ACD', 'ACE', 'ABC', 'BCE', 'AC']:
        c, R2 = meb([P[x] for x in name])
        foreign = [y for y in P if y not in name and power(c, R2, P[y]) < 0]
        out[name] = (R2, 'Gabriel' if not foreign else 'intrus=' + ''.join(foreign))
    return out

def main():
    rc = 0
    e5 = e5_check()
    expect = {'CDE': F(162, 25), 'ADE': F(189, 17), 'ACD': F(33, 2), 'ACE': F(33, 2), 'ABC': F(83886, 3563), 'BCE': F(24)}
    for k, v in expect.items():
        if e5[k][0] != v: print('E5 mismatch', k, e5[k]); rc = 1
    print('E5', {k: (str(v[0]), v[1]) for k, v in e5.items()})
    rng = random.Random(20260923)
    stats = dict(levels=0, admitted=0, inert_checked=0, extra_shell_admitted=0, births=0, births_cover_gt_K=0, continuation_gain=0, merge_gain=0)
    report = []
    clouds = []
    # familles degenerees : grille entiere 0..2 (cospheriques frequents), carre+arc, n=6..7
    for t in range(int(sys.argv[1]) if len(sys.argv) > 1 else 40):
        n = rng.choice([6, 7])
        pts = set()
        side = rng.choice([2, 3])
        while len(pts) < n: pts.add(tuple(rng.randint(0, side) for _ in range(3)))
        clouds.append(sorted(pts))
    clouds.append([(0,0,0),(2,0,0),(1,1,0)])                    # triangle rectangle
    clouds.append([(0,0,0),(2,0,0),(2,2,0),(0,2,0),(1,1,1)])      # carre + apex
    clouds.append([(0,0,0),(2,0,0),(2,2,0),(0,2,0),(1,1,0)])      # carre + centre
    for pts in clouds:
        MEB_CACHE.clear()
        check_cloud(pts, min(4, len(pts) - 1), stats, report)
    print('stats', stats)
    kinds = {}
    for r in report: kinds[r[0]] = kinds.get(r[0], 0) + 1
    print('report', kinds)
    for r in report[:5]: print('  ', r)
    bad = {k: v for k, v in kinds.items() if k != 'equal_point_sets'}
    if bad: rc = 2
    print('status', 'PASS' if rc == 0 else 'FAIL')
    return rc

if __name__ == '__main__':
    sys.exit(main())
