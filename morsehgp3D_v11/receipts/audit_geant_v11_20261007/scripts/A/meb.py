# Independent exact check (Fraction) of small HGP facts. Written by agent A, not imported from the repo.
from fractions import Fraction as Fr
from itertools import combinations

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))

def circum(S):
    """center (in affine hull) and squared radius of the circumsphere of affinely independent S, else None"""
    a=S[0]; V=[sub(p,a) for p in S[1:]]
    m=len(V)
    if m==0: return (tuple(Fr(x) for x in a), Fr(0))
    G=[[Fr(2*dot(V[i],V[j])) for j in range(m)]+[Fr(dot(V[i],V[i]))] for i in range(m)]
    # gaussian elimination
    for c in range(m):
        piv=None
        for r in range(c,m):
            if G[r][c]!=0: piv=r;break
        if piv is None: return None
        G[c],G[piv]=G[piv],G[c]
        for r in range(m):
            if r!=c and G[r][c]!=0:
                f=G[r][c]/G[c][c]
                G[r]=[x-f*y for x,y in zip(G[r],G[c])]
    lam=[G[i][m]/G[i][i] for i in range(m)]
    c=tuple(Fr(a[k])+sum(lam[j]*V[j][k] for j in range(m)) for k in range(len(a)))
    r2=sum((c[k]-a[k])**2 for k in range(len(a)))
    return (c,r2)

def meb(P):
    """exact minimum enclosing ball of point list P (dimension <= 3): (center, r2)"""
    best=None
    d=len(P[0])
    for s in range(1,min(len(P),d+1)+1):
        for S in combinations(P,s):
            cc=circum(list(S))
            if cc is None: continue
            c,r2=cc
            if all(sum((c[k]-p[k])**2 for k in range(d))<=r2 for p in P):
                if best is None or r2<best[1]: best=(c,r2)
    return best

def d2(c,p): return sum((c[k]-p[k])**2 for k in range(len(p)))
