"""L'ecart change-t-il la selection EOM ? Arbre : racine -> P, Q ; P -> A, B (sous-amas de 4) ; A, B se desagregent."""
import sys, collections
sys.dont_write_bytecode = True
import numpy as np
from fractions import Fraction as F
from sklearn.cluster._hdbscan._tree import tree_to_labels, HIERARCHY_dtype
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C
# 16 points ; rayons : paires 1, quadruplets 2, P/Q 2.2, racine 100
rows=[]; nid=16
def link(a,b,d,s):
    global nid; rows.append((a,b,d,s)); nid+=1; return nid-1
pairs=[link(2*i,2*i+1,1.0,2) for i in range(8)]
quads=[link(pairs[2*i],pairs[2*i+1],2.0,4) for i in range(4)]
P=link(quads[0],quads[1],2.2,8); Q=link(quads[2],quads[3],2.2,8); R=link(P,Q,100.0,16)
H=np.array(rows,dtype=HIERARCHY_dtype)
labels, probs, *_ = tree_to_labels(H, min_cluster_size=3, cluster_selection_method='eom', allow_single_cluster=False)
print('sklearn HDBSCAN eom labels', labels.tolist())
# meme arbre en v9 : beta = rayon^2, facettes = points, naissance beta=1/4 (lambda=2)
fac=[(i,) for i in range(16)]
pl=collections.defaultdict(list)
for i in range(8): pl[F(1)].append([(2*i,),(2*i+1,)])
for i in range(4): pl[F(4)].append([(4*i,),(4*i+2,)])
pl[F(484,100)].append([(0,),(4,)]); pl[F(484,100)].append([(8,),(12,)])
pl[F(10000)].append([(0,),(8,)])
nodes, roots = C.merge_tree(set(fac), sorted(pl.items()), {})
births={f:F(1,4) for f in fac}; masses={f:1.0 for f in fac}
cl, order = C.condense(nodes, roots, masses, births, 3.0, 'radius', 1)
for n in order: print(' v9', n, 'parent', cl[n]['parent'], 'stab %.3f'%cl[n]['stability'])
sel = C.select_excess_of_mass(cl, order)
lab = C.label_facets(cl, sel)
print('v9 eom selected', sel, 'labels', [lab.get((i,),-1) for i in range(16)])
print('v9 leaf selected', C.select_leaf(cl, order))
