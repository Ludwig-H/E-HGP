import sys
sys.path.insert(0, sys.argv[1])
from meb import *
from itertools import combinations
def run(X, K, label):
    names=sorted(X)
    beta={}
    for s in range(1,len(names)+1):
        for F in combinations(names,s):
            beta[F]=meb([X[x] for x in F])
    viol=[(F,y) for F,(c,r2) in beta.items() if len(F)>=2 for y in names if y not in F and d2(c,X[y])==r2]
    def gabriel(G):
        c,r2=beta[G]; return all(not(d2(c,X[y])<r2) for y in names if y not in G)
    def comps(V,E):
        par={v:v for v in V}
        def f(v):
            while par[v]!=v: par[v]=par[par[v]]; v=par[v]
            return v
        for u,v in E: par[f(u)]=f(v)
        cl={}
        for v in V: cl.setdefault(f(v),[]).append(v)
        return list(cl.values())
    levels=sorted(set(beta[F][1] for F in beta if len(F) in (K,K+1)))
    GabS=[G for G in combinations(names,K+1) if gabriel(G)]
    VG=sorted(set(f for G in GabS for f in combinations(G,K)))
    bad=[]
    for a in levels:
        V=[F for F in combinations(names,K) if beta[F][1]<=a]
        E=[(fs[i],fs[i+1]) for G in combinations(names,K+1) if beta[G][1]<=a for fs in [list(combinations(G,K))] for i in range(len(fs)-1)]
        EG=[(fs[i],fs[i+1]) for G in GabS if beta[G][1]<=a for fs in [list(combinations(G,K))] for i in range(len(fs)-1)]
        p=sorted(sorted(set(x for F in C for x in F)) for C in comps(V,E) if len(C)>1)
        g=sorted(sorted(set(x for F in C for x in F)) for C in comps(VG,EG) if len(C)>1)
        if p!=g: bad.append((a,p,g))
    print(label,'K=',K,'Def26 violations:',len(viol),'Gabriel simplices:',GabS)
    for a,p,g in bad[:3]+bad[-2:]: print('   a=',a,' Cech:',p,' Gabriel:',g)
    print('   mismatching levels:',len(bad),'of',len(levels))
run({'A':(-100,0,0),'C':(100,0,0),'z':(1,-90,0),'y':(30,-85,0),'w':(3,300,0)},2,'L02')
