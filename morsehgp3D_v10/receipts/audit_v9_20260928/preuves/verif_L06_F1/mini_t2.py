"""Mini-campagne T2 aleatoire, independante : oracle Gamma en Fraction (Python, ecrit ici) contre la tour FULL
publiee par la chaine reelle (export natif, kmax=K), K = 1..5, nuages degeneres (boites serrees) et generiques.
Compare le NOMBRE de composantes de L_K a chaque niveau critique, coupe fermee et ouverte. Sans assert."""
import itertools, json, subprocess, sys
from fractions import Fraction as F
import numpy as np
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
def solve(A, b):
    n=len(b); M=[row[:]+[b[i]] for i,row in enumerate(A)]
    for c in range(n):
        p=next((r for r in range(c,n) if M[r][c]!=0),None)
        if p is None: return None
        M[c],M[p]=M[p],M[c]; d=M[c][c]; M[c]=[v/d for v in M[c]]
        for r in range(n):
            if r!=c and M[r][c]!=0:
                f=M[r][c]; M[r]=[a-f*bb for a,bb in zip(M[r],M[c])]
    return [M[i][n] for i in range(n)]
def support_ball(P, S):
    p0=P[S[0]]
    if len(S)==1: return (tuple(map(F,p0)),F(0))
    E=[[F(P[s][j]-p0[j]) for j in range(3)] for s in S[1:]]
    G=[[sum(a*b for a,b in zip(E[i],E[k])) for k in range(len(E))] for i in range(len(E))]
    w=solve(G,[sum(a*a for a in E[i])/2 for i in range(len(E))])
    if w is None or any(x<=0 for x in w) or 1-sum(w)<=0: return None
    c=[F(p0[j])+sum(w[i]*E[i][j] for i in range(len(E))) for j in range(3)]
    return (tuple(c), sum((c[j]-p0[j])**2 for j in range(3)))
def meb_table(P, maxsize):
    n=len(P); balls=[]
    for q in range(1,5):
        for S in itertools.combinations(range(n),q):
            b=support_ball(P,S)
            if b: balls.append(b)
    balls.sort(key=lambda b:b[1]); r2={}
    for q in range(1,maxsize+1):
        for T in itertools.combinations(range(n),q):
            for c,rr in balls:
                if all(sum((F(P[t][j])-c[j])**2 for j in range(3))<=rr for t in T): r2[T]=rr; break
    return r2
def gamma_count(r2, n, K, a, closed):
    ok=(lambda v: v<=a) if closed else (lambda v: v<a)
    facets=[T for T in itertools.combinations(range(n),K) if ok(r2[T])]
    idx={T:i for i,T in enumerate(facets)}; uf=list(range(len(facets)))
    def f(x):
        while uf[x]!=x: uf[x]=uf[uf[x]]; x=uf[x]
        return x
    if K<n:
        for U in itertools.combinations(range(n),K+1):
            if ok(r2[U]):
                sub=[idx[tuple(v for v in U if v!=d)] for d in U]
                for s in sub[1:]: uf[f(s)]=f(sub[0])
    return len({f(i) for i in range(len(facets))})
def full_count(nodes, a, closed):
    lev={nd['id']:F(int(nd['level']['num']),int(nd['level']['den'])) for nd in nodes}
    succ={nd['id']:nd['successor'] for nd in nodes}
    if closed: return sum(1 for i in lev if lev[i]<=a and (succ[i] is None or lev[succ[i]]>a))
    return sum(1 for i in lev if lev[i]<a and (succ[i] is None or lev[succ[i]]>=a))
def main():
    rng=np.random.default_rng(20260928); clouds=0; checks=0; bad=[]; refused=0
    for trial in range(70):
        n=int(rng.integers(5,10)); box=int(rng.choice([4,6,9,5000]))
        s=set()
        while len(s)<n: s.add(tuple(int(v) for v in rng.integers(0,box,3)))
        P=sorted(s); rng.shuffle(P); P=[tuple(p) for p in P]
        open('mt2.u32le','wb').write(np.array(P,dtype='<u4').tobytes())
        r2=meb_table(P, min(n,6)); clouds+=1
        for K in range(1,min(5,n)+1):
            out=subprocess.run([B,'--input','mt2.u32le','--k',str(K),'--workers','1'],capture_output=True,text=True)
            if out.returncode: refused+=1; continue
            nodes=json.loads(out.stdout)['native']['nodes']
            levels=sorted(set(v for T,v in r2.items() if len(T) in (K,K+1)))+[max(r2.values())+1]
            for a in levels:
                for closed in (False,True):
                    g=gamma_count(r2,n,K,a,closed); fc=full_count(nodes,a,closed); checks+=1
                    if g!=fc: bad.append((trial,n,box,K,str(a),closed,g,fc,P))
    print(f'mini_t2 clouds={clouds} checks={checks} refused={refused} disagreements={len(bad)}')
    for b in bad[:5]: print(b)
    return 1 if bad else 0
sys.exit(main())
