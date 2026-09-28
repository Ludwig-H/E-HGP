from fractions import Fraction as Fr
import itertools
P=[(0,0,7),(0,9,6),(1,4,0),(0,0,1),(4,1,2)]; L='ABCDE'
def solve(M,b):
    n=len(M); A=[list(map(Fr,r))+[Fr(v)] for r,v in zip(M,b)]
    for i in range(n):
        p=next((j for j in range(i,n) if A[j][i]!=0),None)
        if p is None: return None
        A[i],A[p]=A[p],A[i]
        for j in range(n):
            if j!=i and A[j][i]!=0:
                f=A[j][i]/A[i][i]; A[j]=[a-f*c for a,c in zip(A[j],A[i])]
    return [A[i][n]/A[i][i] for i in range(n)]
def ball(S):
    Q=[P[i] for i in S]
    if len(Q)==1: return tuple(map(Fr,Q[0])),Fr(0)
    a=[tuple(Fr(x-y) for x,y in zip(q,Q[0])) for q in Q[1:]]
    G=[[sum(u*v for u,v in zip(ai,aj)) for aj in a] for ai in a]
    rhs=[sum(u*u for u in ai)/2 for ai in a]
    lam=solve(G,rhs)
    if lam is None: return None
    c=tuple(Fr(Q[0][d])+sum(l*ai[d] for l,ai in zip(lam,a)) for d in range(3))
    return c,sum((Fr(Q[0][d])-c[d])**2 for d in range(3))
def d2(i,c): return sum((Fr(P[i][d])-c[d])**2 for d in range(3))
def meb(S):
    best=None
    for s in range(1,min(4,len(S))+1):
        for T in itertools.combinations(S,s):
            b=ball(T)
            if b is None: continue
            c,r=b
            if all(d2(i,c)<=r for i in S) and (best is None or r<best[1]): best=(c,r)
    return best
K=2; lev=Fr(83886,3563)
cof={s:meb(s) for s in itertools.combinations(range(5),K+1)}
gab={s for s,(c,r) in cof.items() if all(d2(i,c)>r for i in range(5) if i not in s)}
print('Gabriel cofaces:',sorted(''.join(L[i] for i in s) for s in gab))
print('rho2(AC)=',meb((0,2))[1],' ACD',cof[(0,2,3)][1],' ACE',cof[(0,2,4)][1],' ABC',cof[(0,1,2)][1])
def comps(cats):
    par={}
    def f(a):
        par.setdefault(a,a)
        while par[a]!=a: a=par[a]
        return a
    for s in cats:
        if cof[s][1]<=lev:
            fs=list(itertools.combinations(s,K))
            for a,b in zip(fs,fs[1:]): par[f(a)]=f(b)
    out={}
    for t in list(par): out.setdefault(f(t),set()).update(t)
    return sorted(''.join(L[i] for i in sorted(v)) for v in out.values())
print('Cech point-sets at 83886/3563:',comps(list(cof)))
print('Gabriel point-sets at 83886/3563:',comps(list(gab)))
