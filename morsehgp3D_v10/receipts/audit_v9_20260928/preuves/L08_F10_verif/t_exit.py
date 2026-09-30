import sys, os, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cluster_head as C, cluster_wt as W
import weighted_eom as E
# Hand tree on 4 facet leaves a,b,c,d (mass 1 each), threshold 1.5.
# n0 = {a,b} at beta=1, n1 = {c,d} at beta=1, root n2 = {n0,n1} at beta=16.
# Both n0,n1 have mass 2 >= 1.5 -> split at root; inside n0, children a,b have mass 1 < 1.5:
# HDBSCAN: a,b fall at lambda(n0 level). Leaf births: a..d born at beta=0.25 (earlier than any merge).
nodes = {'n0': dict(children=['a','b'], level=1, members={'a','b'}),
         'n1': dict(children=['c','d'], level=1, members={'c','d'}),
         'n2': dict(children=['n0','n1'], level=16, members={'a','b','c','d'})}
masses = dict(a=1.0, b=1.0, c=1.0, d=1.0)
births = dict(a=0.25, b=0.25, c=0.25, d=0.25)
cl, order = C.condense(nodes, ['n2'], masses, births, 1.5, 'radius', 1)
for name in order:
    print('cluster.py', name, 'birth', cl[name]['birth'], 'falls', cl[name]['falls'], 'stab', cl[name]['stability'])
# weighted_eom on the same tree: leaves 0..3, internal 4,5,6, heights = radius
res = E.weighted_condense_eom(4, {4:[0,1], 5:[2,3], 6:[4,5]}, {4:1.0, 5:1.0, 6:4.0}, [1.0]*4, min_cluster_size=1.5, exp_z=1)
print('weighted_eom stabilities', res['stabilities'], 'leaf_exit', res['leaf_exit_lambda'])
