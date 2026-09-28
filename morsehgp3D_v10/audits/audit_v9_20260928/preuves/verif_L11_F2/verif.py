"""Verification adverse L11-F2 : v9 condense vs HDBSCAN (sklearn) vs HGP-old (re-implementation fidele)."""
import sys, collections
sys.dont_write_bytecode = True
import numpy as np
from sklearn.cluster._hdbscan._tree import _condense_tree, HIERARCHY_dtype
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C

# --- 1. HDBSCAN (sklearn 1.9.1) sur l'arbre single-linkage equivalent (distance = rayon = sqrt(beta))
H = np.array([(0,1,1.0,2),(2,3,1.0,2),(4,5,1.0,2),(6,7,1.0,2),(8,9,2.0,4),(10,11,2.0,4),(12,13,4.0,8)], dtype=HIERARCHY_dtype)
ct = _condense_tree(H, 3)
print('sklearn condensed tree:')
for r in ct: print('  ', tuple(r))
births = {8: 0.0}
for r in ct:
    if r['cluster_size'] > 1 or r['child'] >= 8:
        if r['child'] >= 8: births[int(r['child'])] = float(r['value'])
stab = collections.defaultdict(float)
for r in ct:
    p = int(r['parent']); stab[p] += (float(r['value']) - births[p]) * int(r['cluster_size'])
print('sklearn stabilities (root birth 0):', dict(stab))

# --- 2. HGP-old condense (re-implementation fidele de _cython.pyx:456-638, lot par lot)
def hgp_old(W_nodes, U, V, W, mcs, EPS=1e-12):
    N=len(W_nodes); parent=list(range(N)); cw=list(W_nodes); cid=[-1]*N; nodes=[[i] for i in range(N)]
    tracked=[[] for _ in range(N)]; ch=[]; br=[]; dr=[]; st=[]; nin=[]; sjl=[]; last=[-1]*N
    def find(x):
        while parent[x]!=x: parent[x]=parent[parent[x]]; x=parent[x]
        return x
    i=0; M=len(W)
    while i<M:
        j=i
        while j<M and W[j]-W[i]<=0.0: j+=1
        r=W[j-1]; lam=1.0/(r+EPS); roots=[]
        for k in range(i,j):
            ru,rv=find(U[k]),find(V[k])
            if ru==rv: continue
            if cw[ru]<cw[rv]: ru,rv=rv,ru
            for x in (ru,rv):
                if cid[x]!=-1: tracked[x].append(cid[x]); cid[x]=-1
            tracked[ru]+=tracked[rv]; tracked[rv]=[]
            parent[rv]=ru; cw[ru]+=cw[rv]; nodes[ru]+=nodes[rv]; nodes[rv]=[]
            if last[ru]!=i: roots.append(ru); last[ru]=i
        for ru in roots:
            if parent[ru]!=ru: continue
            if cw[ru]>=mcs:
                if not tracked[ru]:
                    c=len(ch); ch.append([]); br.append(r); dr.append(None); st.append(0.0); nin.append(cw[ru]); sjl.append(cw[ru]*lam); nodes[ru]=[]; cid[ru]=c
                elif len(tracked[ru])==1:
                    c=tracked[ru][0]; add=sum(W_nodes[n] for n in nodes[ru]); nodes[ru]=[]; nin[c]+=add; sjl[c]+=add*lam; cid[ru]=c
                else:
                    npar=0.0
                    for c in tracked[ru]:
                        if dr[c] is None: dr[c]=r; st[c]+=sjl[c]-nin[c]*lam
                        npar+=nin[c]
                    add=sum(W_nodes[n] for n in nodes[ru]); nodes[ru]=[]; npar+=add
                    c=len(ch); ch.append(list(tracked[ru])); br.append(r); dr.append(None); st.append(0.0); nin.append(npar); sjl.append(npar*lam); cid[ru]=c
            tracked[ru]=[]
        i=j
    for c in range(len(ch)):
        if dr[c] is None: st[c]+=sjl[c]
    return ch, st
ch, st = hgp_old([1.0]*8, [0,2,4,6,0,4,0], [1,3,5,7,2,6,4], [1.0,1.0,1.0,1.0,2.0,2.0,4.0], 3)
print('HGP-old children', ch, 'stabilities', st)

# --- 3. v9 condense (arbre de travail, cluster.py courant)
fac=[(x,) for x in 'abcdefgh']
from fractions import Fraction as F
births_v9={f:F(1,4) for f in fac}; masses={f:1.0 for f in fac}
pl=collections.defaultdict(list)
for g,b in [((('a',),('b',)),1),((('c',),('d',)),1),((('e',),('f',)),1),((('g',),('h',)),1),((('a',),('c',)),4),((('e',),('g',)),4),((('a',),('e',)),16)]:
    pl[F(b)].append(list(g))
nodes, roots = C.merge_tree(set(fac), sorted(pl.items()), births_v9)
for bf in (F(1,4), F(1,64)):
    births_v9={f:bf for f in fac}
    cl, order = C.condense(nodes, roots, masses, births_v9, 3.0, 'radius', 1)
    print('v9 facet birth beta=%s:'%bf, {n:(cl[n]['parent'], cl[n]['stability']) for n in order})
    print('   eom', C.select_excess_of_mass(cl, order, allow_single_cluster=True), 'leaf', C.select_leaf(cl, order))
