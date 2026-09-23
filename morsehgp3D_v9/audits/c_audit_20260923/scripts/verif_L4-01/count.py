from fractions import Fraction as F
from itertools import combinations
def solve(A,b):
    n=len(A); M=[row[:]+[b[i]] for i,row in enumerate(A)]
    for c in range(n):
        p=next((r for r in range(c,n) if M[r][c]!=0),None)
        if p is None: return None
        M[c],M[p]=M[p],M[c]
        d=M[c][c]; M[c]=[x/d for x in M[c]]
        for r in range(n):
            if r!=c and M[r][c]!=0:
                m=M[r][c]; M[r]=[x-m*y for x,y in zip(M[r],M[c])]
    return [M[i][n] for i in range(n)]
def ball(P):
    b=[F(x) for x in P[0]]; E=[[F(p[i])-b[i] for i in range(3)] for p in P[1:]]
    dot=lambda u,v: sum(x*y for x,y in zip(u,v))
    A=[[dot(E[i],E[j]) for j in range(len(E))] for i in range(len(E))]
    w=solve(A,[dot(e,e)/2 for e in E])
    if w is None: return None
    if any(x<=0 for x in w) or 1-sum(w)<=0: return None
    c=[b[i]+sum(w[j]*E[j][i] for j in range(len(E))) for i in range(3)]
    return tuple(c), dot([c[i]-b[i] for i in range(3)],[c[i]-b[i] for i in range(3)])
fx={}
fx['line12']=[(2*j+(j%2),7,9) for j in range(12)]
fx['shell14']=[(15,10,10),(5,10,10),(10,15,10),(10,5,10),(10,10,15),(10,10,5),(13,14,10),(13,6,10),(7,14,10),(7,6,10),(10,13,14),(10,7,6),(10,10,10),(20,25,30)]
fx['spatial12']=[(7,42,83),(91,12,64),(33,88,9),(54,20,71),(18,61,39),(76,53,95),(42,7,24),(62,94,47),(3,29,58),(85,73,15),(29,36,97),(58,65,3)]
for name,pts in fx.items():
    n=len(pts); rows={}
    for q in (2,3,4):
        for S in combinations(range(n),q):
            B=ball([pts[i] for i in S])
            if B is None: continue
            rows[B]=min(rows.get(B,9),q)
    stats=[]
    for (c,r2),q in rows.items():
        inte=sum(1 for p in pts if sum((F(p[i])-c[i])**2 for i in range(3))<r2)
        stats.append((inte,q))
    out=[name,n,len(stats)]
    for K in (1,2,3,5,10):
        cap=min(K+1,n)
        kept=sum(1 for i,q in stats if i+q<=cap)
        edge=sum(1 for i,q in stats if i+q==cap)   # on the threshold
        above=sum(1 for i,q in stats if i+q==cap+1) # just rejected
        out.append(f"K{K}:kept={kept},rej={len(stats)-kept},eq={edge},eq+1={above}")
    print(*out)
