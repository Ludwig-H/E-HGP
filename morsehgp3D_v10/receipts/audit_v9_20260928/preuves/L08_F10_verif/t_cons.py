import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cluster_cda as CD, cluster_head as C
nodes = {'n0': dict(children=['a','b'], level=1, members={'a','b'}),
         'n1': dict(children=['c','d'], level=1, members={'c','d'}),
         'n2': dict(children=['n0','n1'], level=16, members={'a','b','c','d'})}
masses = dict(a=1.0, b=1.0, c=1.0, d=1.0); births = dict(a=.25,b=.25,c=.25,d=.25)
for tag, mod in (('cda636b5e', CD), ('ce8a649dd', C)):
    cl, order = mod.condense(nodes, ['n2'], masses, births, 1.5, 'radius', 1)
    root = [c for c in order if cl[c]['parent'] is None][0]
    print(tag, 'root cluster falls mass', sum(masses[f] for f, _ in cl[root]['falls']), 'subtree mass 4.0', 'stability', cl[root]['stability'], 'birth', cl[root]['birth'])
