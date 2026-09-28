"""Sonde en lecture seule : v9 cluster.condense quand un cluster se fragmente en morceaux tous sous le seuil."""
import sys
from fractions import Fraction as F
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C
fac = [('a',), ('b',), ('c',), ('d',), ('e',), ('f',), ('g',), ('h',)]
births = {f: F(1, 4) for f in fac}
masses = {f: 1.0 for f in fac}
# cofaces (groupes) : paires a beta=1, quadruplets a beta=4, racine a beta=16
cof = [((('a',), ('b',)), F(1)), ((('c',), ('d',)), F(1)), ((('e',), ('f',)), F(1)), ((('g',), ('h',)), F(1)),
       ((('a',), ('c',)), F(4)), ((('e',), ('g',)), F(4)), ((('a',), ('e',)), F(16))]
import collections
plateaus = collections.defaultdict(list)
for group, beta in cof:
    plateaus[beta].append(list(group))
nodes, roots = C.merge_tree(set(fac), sorted(plateaus.items()), births)
clusters, order = C.condense(nodes, roots, masses, births, 3.0, 'radius', 1)
for name in order:
    c = clusters[name]
    print(name, 'parent', c['parent'], 'birth_lambda', c['birth'], 'stability', c['stability'],
          'falls', sorted((f[0], lam) for f, lam in c['falls']))
print('HDBSCAN/HGP-old attendu : chaque enfant X,Y ne 4*(0.5-0.25)=1.0 (tout tombe a lambda(beta=4)=0.5)')
