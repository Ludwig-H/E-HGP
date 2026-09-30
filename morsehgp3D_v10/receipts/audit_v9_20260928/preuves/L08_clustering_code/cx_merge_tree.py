"""Contre-exemples sur cluster.py (worktree v9, lecture seule)."""
import sys
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

# --- 1. merge_tree : cle perimee dans `merged` au sein d'un plateau
c, d, a, b = (0,), (1,), (2,), (3,)
facets = {a, b, c, d}
plateaus = [(F(4), [[c, d], [a, b], [a, c]])]
births = {f: F(1) for f in facets}
nodes, roots = C.merge_tree(facets, plateaus, births)
print('CX1 nodes', {k: (v['children'], sorted(v['members'])) for k, v in nodes.items()})
print('CX1 roots', roots)
reach = reachable(nodes, roots)
lost = facets - reach
print('CX1 facets unreachable from roots:', lost, ' nodes unreachable:', set(nodes) - reach)

# meme contre-exemple par des cofaces reelles (K=2 : facettes = paires)
# cofaces triangles au meme niveau : {0,1,2} {3,4,5} {0,1,3}? construire des groupes
# On cherche une configuration de cofaces qui produit l'ordre de groupes voulu.
import itertools
def run(cofaces):
    facets, plateaus = C.facet_levels(cofaces, None)
    births = {}
    for v, beta in cofaces:
        for drop in v:
            f = tuple(x for x in v if x != drop)
            births[f] = min(births.get(f, beta), beta)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    reach = reachable(nodes, roots)
    return facets - reach, nodes, roots
found = None
pts = range(7)
tris = list(itertools.combinations(pts, 3))
import random
random.seed(1)
for trial in range(20000):
    cof = random.sample(tris, random.randint(2, 5))
    cofaces = [(t, F(4)) for t in cof]
    lost, nodes, roots = run(cofaces)
    if lost:
        found = (cofaces, lost, len(nodes), roots)
        break
print('CX1b real cofaces, single plateau:', found)
