"""E5 (gabriel_point_set_counterexample): compare Gamma_2 exact components with the
Gabriel-only fold used by experiments/tower_clustering_20260928/cluster.py (merge_tree).
Read-only import of cluster.py; exact rationals."""
import itertools, sys
from fractions import Fraction as F
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C

P = {'A': (0,0,7), 'B': (0,9,6), 'C': (1,4,0), 'D': (0,0,1), 'E': (4,1,2)}
names = sorted(P)

def sub(a,b): return tuple(F(x)-F(y) for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))

def solve(M, v):
    n=len(M); A=[list(map(F,row))+[F(v[i])] for i,row in enumerate(M)]
    for c in range(n):
        piv=next((r for r in range(c,n) if A[r][c]!=0),None)
        if piv is None: return None
        A[c],A[piv]=A[piv],A[c]
        for r in range(n):
            if r!=c and A[r][c]!=0:
                f=A[r][c]/A[c][c]; A[r]=[x-f*y for x,y in zip(A[r],A[c])]
    return [A[i][n]/A[i][i] for i in range(n)]

def circ(S):
    pts=[P[s] for s in S]; p0=pts[0]
    if len(pts)==1: return tuple(map(F,p0)), F(0)
    E=[sub(p,p0) for p in pts[1:]]
    G=[[dot(e,f) for f in E] for e in E]
    lam=solve(G,[dot(e,e)/2 for e in E])
    if lam is None: return None
    c=tuple(F(p0[i])+sum(l*e[i] for l,e in zip(lam,E)) for i in range(3))
    return c, dot(sub(p0,c),sub(p0,c))

def meb(Q):
    best=None
    for r in range(1,min(4,len(Q))+1):
        for S in itertools.combinations(Q,r):
            cc=circ(S)
            if cc is None: continue
            c,r2=cc
            if all(dot(sub(P[q],c),sub(P[q],c))<=r2 for q in Q):
                if best is None or r2<best[1]: best=(c,r2)
    return best

beta={}
for r in range(1,6):
    for Q in itertools.combinations(names,r): beta[Q]=meb(Q)[1]

def gamma_components(k,a):
    V=[Q for Q in itertools.combinations(names,k) if beta[Q]<=a]
    parent={v:v for v in V}
    def f(x):
        while parent[x]!=x: parent[x]=parent[parent[x]]; x=parent[x]
        return x
    for u,v in itertools.combinations(V,2):
        U=tuple(sorted(set(u)|set(v)))
        if len(U)==k+1 and beta[U]<=a: parent[f(u)]=f(v)
    comps={}
    for v in V: comps.setdefault(f(v),set()).add(v)
    return sorted(sorted(c) for c in comps.values())

def gabriel(Q):
    c,r2=meb(Q)
    return all(dot(sub(P[x],c),sub(P[x],c))>=r2 for x in names if x not in Q)

k=2
cofaces=[(Q,beta[Q]) for Q in itertools.combinations(names,k+1) if gabriel(Q)]
print('Gabriel cofaces K=2:', [(''.join(Q),str(b)) for Q,b in cofaces])
facets, plateaus = C.facet_levels(cofaces, None)
births = {}
for Q,b in cofaces:
    for fct in C.facets if False else [tuple(v for v in Q if v!=d) for d in Q]:
        births[fct]=min(births.get(fct,b), beta[fct] if gabriel(fct) else b)
nodes, roots = C.merge_tree(facets, plateaus, births)
levels=sorted({b for _,b in cofaces}|{beta[Q] for Q in itertools.combinations(names,3)})
def fold_components(a):
    parent={f:f for f in facets}
    def fd(x):
        while parent[x]!=x: parent[x]=parent[parent[x]]; x=parent[x]
        return x
    for Q,b in cofaces:
        if b<=a:
            fs=[tuple(v for v in Q if v!=d) for d in Q]
            for x in fs[1:]: parent[fd(x)]=fd(fs[0])
    comps={}
    for f in facets:
        if (f in births and births[f]<=a):
            comps.setdefault(fd(f),set()).add(f)
    return comps
for a in levels:
    g=gamma_components(2,a)
    gcov=sorted(''.join(sorted(set().union(*[set(x) for x in c]))) for c in g if len(c)>1)
    fc=fold_components(a)
    fcov=sorted(''.join(sorted(set().union(*[set(x) for x in c]))) for c in fc.values() if len(c)>1)
    print(f'a={str(a):>12} Gamma2 nontrivial covers={gcov}  GabrielFold covers={fcov}  {"DIFF" if gcov!=fcov else ""}')
print('merge_tree nodes (level, #children):', sorted((str(n['level']),len(n['children'])) for n in nodes.values()))
