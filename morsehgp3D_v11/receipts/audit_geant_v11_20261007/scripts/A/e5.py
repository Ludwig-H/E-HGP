import sys
sys.path.insert(0, sys.argv[1])
from meb import *
from itertools import combinations
from fractions import Fraction as Fr
X={'A':(0,0,7),'B':(0,9,6),'C':(1,4,0),'D':(0,0,1),'E':(4,1,2)}
names=sorted(X)
K=2
beta={}
for s in range(1,6):
    for F in combinations(names,s):
        beta[F]=meb([X[x] for x in F])
# general position (Def 26): for every subset |s|>=2, no other point on the boundary of its MEB
gp_viol=[]
for F,(c,r2) in beta.items():
    if len(F)>=2:
        for y in names:
            if y not in F and d2(c,X[y])==r2: gp_viol.append((F,y))
print('Def26 violations:',gp_viol)
def gabriel(G):
    c,r2=beta[G]
    return all(not(d2(c,X[y])<r2) for y in names if y not in G)
def comps(vertices,edges):
    par={v:v for v in vertices}
    def f(v):
        while par[v]!=v: par[v]=par[par[v]]; v=par[v]
        return v
    for (u,v) in edges: par[f(u)]=f(v)
    cl={}
    for v in vertices: cl.setdefault(f(v),[]).append(v)
    return list(cl.values())
levels=sorted(set(beta[F][1] for F in beta if len(F) in (K,K+1)))
print('pairs:',{F:str(beta[F][1]) for F in combinations(names,2)})
print('triples:',{F:(str(beta[F][1]),'Gab' if gabriel(F) else 'nonGab') for F in combinations(names,3)})
mism=[]
for a in levels:
    V=[F for F in combinations(names,K) if beta[F][1]<=a]
    Etri=[G for G in combinations(names,K+1) if beta[G][1]<=a]
    E=[]
    for G in Etri:
        fs=list(combinations(G,K)); 
        for i in range(len(fs)-1): E.append((fs[i],fs[i+1]))
    gam=comps(V,E)
    poly=sorted(sorted(set(x for F in C for x in F)) for C in gam if len(C)>1 or any(F in [f for G in Etri for f in combinations(G,K)] for F in C))
    # Gabriel graph: vertices = facets of some Gabriel K-simplex (any level); edges = Gabriel simplices with level <= a
    GabS=[G for G in combinations(names,K+1) if gabriel(G)]
    VG=sorted(set(f for G in GabS for f in combinations(G,K)))
    EG=[]
    for G in GabS:
        if beta[G][1]<=a:
            fs=list(combinations(G,K))
            for i in range(len(fs)-1): EG.append((fs[i],fs[i+1]))
    gc=comps(VG,EG)
    gnon=sorted(sorted(set(x for F in C for x in F)) for C in gc if len(C)>1)
    pnon=sorted(sorted(set(x for F in C for x in F)) for C in gam if len(C)>1)
    flag = '' if gnon==pnon else '   <== MISMATCH'
    print('a=',a, 'Gamma nontriv:',pnon,' Gabriel nontriv:',gnon, flag)
