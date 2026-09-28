import sys, itertools
from fractions import Fraction as F
sys.path.insert(0, sys.argv[1])
import cluster_head as C
def reach(nodes, roots):
    seen, st = set(), list(roots)
    while st:
        c = st.pop()
        if c in seen: continue
        seen.add(c)
        if c in nodes: st.extend(nodes[c]['children'])
    return seen
def births_of(cof):
    b = {}
    for v, beta in cof:
        for d in v:
            f = tuple(x for x in v if x != d); b[f] = min(b.get(f, beta), beta)
    return b
# 1) my hand-made chain on edges (K=1 style: cofaces are pairs? no: cofaces of size 3, facets size 2)
# enumerate small single-plateau catalogues; report lost facets and nested same-level nodes
stats = dict(total=0, lost=0, nested=0)
first_lost = first_nested = None
for npts in range(4, 7):
    tris = list(itertools.combinations(range(npts), 3))
    for m in range(2, 4):
        for combo in itertools.combinations(tris, m):
            cof = [(t, F(4)) for t in combo]
            facets, plateaus = C.facet_levels(cof, None)
            nodes, roots = C.merge_tree(facets, plateaus, births_of(cof))
            stats['total'] += 1
            lost = facets - reach(nodes, roots)
            # nested: an internal node child of an internal node at same level
            nested = any(ch in nodes for n in nodes.values() for ch in n['children'])
            # connectivity truth: number of components of facet graph
            comp = {}
            def f(x):
                while comp.setdefault(x, x) != x: x = comp[x]
                return x
            for v, _ in cof:
                fs = [tuple(y for y in v if y != d) for d in v]
                for a in fs[1:]: comp[f(a)] = f(fs[0])
            ncomp = len({f(x) for x in facets})
            if lost:
                stats['lost'] += 1
                if first_lost is None: first_lost = (combo, sorted(lost), {k: v['children'] for k, v in nodes.items()}, roots, ncomp)
            if nested:
                stats['nested'] += 1
                if first_nested is None: first_nested = (combo, {k: v['children'] for k, v in nodes.items()}, roots, ncomp)
print(stats)
print('first_lost', first_lost)
print('first_nested', first_nested)
