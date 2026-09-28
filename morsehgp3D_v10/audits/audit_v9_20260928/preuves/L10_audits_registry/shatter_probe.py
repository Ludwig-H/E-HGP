import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C
from fractions import Fraction as F
# Arbre : racine R (niveau beta=4) = fusion de deux noeuds A (beta=1) et B (beta=1),
# chacun fusion de deux facettes de masse 1 nees a beta=1/4.
nodes = {
 'A': dict(children=['a1','a2'], level=F(1), members={'a1','a2'}),
 'B': dict(children=['b1','b2'], level=F(1), members={'b1','b2'}),
 'R': dict(children=['A','B'], level=F(4), members={'a1','a2','b1','b2'}),
}
masses = {f: 1.0 for f in ('a1','a2','b1','b2')}
births = {f: F(1,4) for f in masses}
# seuil 3 : ni A (masse 2) ni B (masse 2) n'atteint 3 -> en HDBSCAN, R s'eteint a lambda(R) et
# toutes ses facettes tombent a lambda(beta=4)=1/2 ; stabilite HDBSCAN = 4*(1/2-1/2)=0.
clusters, order = C.condense(nodes, ['R'], masses, births, 3.0, 'radius', 1)
for n in order:
    c = clusters[n]
    print(n, 'birth', c['birth'], 'falls', sorted((f, round(l,3)) for f,l in c['falls']), 'stability', c['stability'])
