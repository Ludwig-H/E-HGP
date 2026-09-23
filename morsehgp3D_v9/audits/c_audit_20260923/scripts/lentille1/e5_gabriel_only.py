#!/usr/bin/env python3
"""E5 (tests/fixtures/regressions/gabriel_point_set_counterexample.json) :
Gamma_2 complet contre graphe des seules cofaces Gabriel, au niveau 83886/3563."""
import itertools, sys
from fractions import Fraction as F
sys.path.insert(0, '.')
from hgp_block_oracle import meb, power
P = {'A': (0,0,7), 'B': (0,9,6), 'C': (1,4,0), 'D': (0,0,1), 'E': (4,1,2)}
names = sorted(P)
def lvl(S): return meb([P[x] for x in S])[1]
def gabriel(S):
    c, R2 = meb([P[x] for x in S]); return all(power(c, R2, P[y]) >= 0 for y in names if y not in S)
def comps(cut, gab_only):
    par = {}
    def f(x):
        par.setdefault(x, x)
        while par[x] != x: x = par[x]
        return x
    facets = [S for S in itertools.combinations(names, 2) if lvl(S) <= cut]
    for S in facets: f(S)
    for Q in itertools.combinations(names, 3):
        if lvl(Q) <= cut and (not gab_only or gabriel(Q)):
            fs = [S for S in itertools.combinations(Q, 2) if (not gab_only) or lvl(S) <= cut]
            for g in fs[1:]: par[f(g)] = f(fs[0])
    out = {}
    for S in facets: out.setdefault(f(S), set()).update(S)
    # composantes non triviales (au moins une coface) seulement, comme Prop. 6
    return sorted(''.join(sorted(v)) for v in out.values())
cut = F(83886, 3563)
full, gab = comps(cut, False), comps(cut, True)
print('Gamma_2 complet   :', full)
print('Gabriel seulement :', gab)
ok = ('ABCDE' in full) and ('ABC' in gab or any(set('ABC') <= set(x) and 'D' not in x for x in gab))
print('status', 'PASS (E5 reproduit)' if ok else 'FAIL'); sys.exit(0 if ok else 1)
