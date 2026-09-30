import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cluster_head as C, cluster_wt as W
import weighted_model as WM
nodes = {'n0': dict(children=['a','b'], level=1, members={'a','b'}),
         'n1': dict(children=['c','d'], level=1, members={'c','d'}),
         'n2': dict(children=['n0','n1'], level=16, members={'a','b','c','d'})}
masses = dict(a=1.0, b=1.0, c=1.0, d=1.0, e=0.3)
births = dict(a=0.25, b=0.25, c=0.25, d=0.25, e=0.25)
cl, order = C.condense(nodes, ['e', 'n2'], masses, births, 1.5, 'radius', 1)
print('HEAD selected', [(s, cl[s]['node'], cl[s]['mass']) for s in C.select_excess_of_mass(cl, order)])
cl, order = W.condense(nodes, ['e', 'n2'], masses, births, 1.5, 'radius', 1)
print('WT eom selected', [(s, cl[s]['node'], cl[s]['mass']) for s in W.select(cl, order, 'eom')])
print('WT leaf selected', [(s, cl[s]['node'], cl[s]['mass']) for s in W.select(cl, order, 'leaf')])
# weighted_model on a disconnected catalogue
try:
    WM.cluster_cofaces(6, 2, [dict(vertices=[0,1,2], beta=dict(num=1,den=1)), dict(vertices=[3,4,5], beta=dict(num=1,den=1))], min_cluster_size=2)
    print('weighted_model accepted forest')
except ValueError as e:
    print('weighted_model refused:', e)
