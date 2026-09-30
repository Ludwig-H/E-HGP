import sys, itertools
from fractions import Fraction as F
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C
def reachable(nodes, roots):
    seen, stack = set(), list(roots)
    while stack:
        cur = stack.pop()
        if cur in seen: continue
        seen.add(cur)
        if cur in nodes: stack.extend(nodes[cur]['children'])
    return seen
def run(cofaces):
    facets, plateaus = C.facet_levels(cofaces, None)
    births = {}
    for v, beta in cofaces:
        for d in v:
            f = tuple(x for x in v if x != d); births[f] = min(births.get(f, beta), beta)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    return facets - reachable(nodes, roots), nodes, roots
best = None
for npts in range(4, 8):
    tris = list(itertools.combinations(range(npts), 3))
    for m in range(2, 5):
        for combo in itertools.combinations(tris, m):
            cof = [(t, F(4)) for t in combo]
            lost, nodes, roots = run(cof)
            if lost:
                best = (npts, combo, sorted(lost), {k: v['children'] for k, v in nodes.items()}, roots); break
        if best: break
    if best: break
print(best)
if best:
    # the same cofaces with the two-level births does not matter: all at one level
    cof = [(t, F(4)) for t in best[1]]
    facets, plateaus = C.facet_levels(cof, None)
    print('facets', len(facets), 'groups on the single plateau', len(plateaus[0][1]))
