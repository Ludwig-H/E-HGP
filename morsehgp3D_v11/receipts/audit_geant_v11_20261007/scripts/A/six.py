import sys
sys.path.insert(0, sys.argv[1])
from meb import *
from itertools import combinations
from fractions import Fraction as Fr
X={'A':(268,3000,0),'B':(268,1000,0),'C':(2000,2000,0),'D':(4000,2000,0),'E':(5732,3000,0),'F':(5732,1000,0)}
names=sorted(X)
def dd(a,b): return sum((p-q)**2 for p,q in zip(X[a],X[b]))
# RSL / HDBSCAN with min_samples=K (self included): core distance^2 = (K-1)-th nearest other point
def rsl(K):
    core={x:sorted(dd(x,y) for y in names if y!=x)[K-2] if K>=2 else 0 for x in names}
    edges=sorted((max(core[x],core[y],dd(x,y)),x,y) for x,y in combinations(names,2))
    par={x:x for x in names}
    def f(v):
        while par[v]!=v: v=par[v]
        return v
    merges=[]
    for w,x,y in edges:
        if f(x)!=f(y): par[f(x)]=f(y); merges.append(Fr(w,4))  # thesis convention: level = (d/2)^2
    return merges
print('RSL/HDBSCAN K=2 merge squared levels (thesis convention, (d/2)^2):',[float(m) for m in rsl(2)])
beta={}
for s in (2,3):
    for F in combinations(names,s): beta[F]=meb([X[x] for x in F])[1]
def gamma2(a):
    V=[F for F in combinations(names,2) if beta[F]<=a]
    par={v:v for v in V}
    def f(v):
        while par[v]!=v: v=par[v]
        return v
    for G in combinations(names,3):
        if beta[G]<=a:
            fs=list(combinations(G,2)); 
            for i in range(2): par[f(fs[i])]=f(fs[i+1])
    cl={}
    for v in V: cl.setdefault(f(v),[]).append(''.join(v))
    return sorted(cl.values())
for a in sorted(set(beta.values())):
    print('a=%.0f r=%.1f'%(float(a),float(a)**0.5), gamma2(a)); 
    if len(gamma2(a))==1: break
