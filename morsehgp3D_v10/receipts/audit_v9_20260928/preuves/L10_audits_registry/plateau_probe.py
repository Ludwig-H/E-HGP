import sys
from fractions import Fraction
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster
# K=1 : facettes = points (tuples de taille 1), cofaces = aretes (taille 2).
# Trois cofaces au MEME niveau beta=1 : (a,b), (c,d), puis (a,c) qui joint les deux.
# Pour forcer le cas, on met d'abord deux aretes a un niveau inferieur ? Non : il faut
# que, DANS un meme plateau, un groupe ulterieur reunisse deux racines deja fusionnees
# de rang egal. On prend (a,b),(c,d),(b,d) au meme niveau.
a,b,c,d = (0,),(1,),(2,),(3,)
cofaces = [((0,1),Fraction(1)), ((2,3),Fraction(1)), ((1,3),Fraction(1))]
seen, plateaus = cluster.facet_levels(cofaces, None)
births = {f: Fraction(0) + Fraction(1,4) for f in seen}
nodes, roots = cluster.merge_tree(seen, plateaus, births)
for name, node in sorted(nodes.items()):
    print(name, 'level', node['level'], 'children', node['children'], 'members', sorted(node['members']))
print('roots', roots)
print('nodes at level 1:', sum(1 for n in nodes.values() if n['level']==1))
