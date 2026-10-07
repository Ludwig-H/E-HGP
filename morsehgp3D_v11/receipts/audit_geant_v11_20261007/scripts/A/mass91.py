import sys
sys.path.insert(0, sys.argv[1])
from meb import *
from itertools import combinations
from fractions import Fraction as Fr
X={'A':(268,3000,0),'B':(268,1000,0),'C':(2000,2000,0),'D':(4000,2000,0),'E':(5732,3000,0),'F':(5732,1000,0)}
names=sorted(X); K=2
beta={F:meb([X[x] for x in F]) for s in (2,3) for F in combinations(names,s)}
def gab(G):
    c,r2=beta[G]; return all(not(d2(c,X[y])<r2) for y in names if y not in G)
GabS=[G for G in combinations(names,3) if gab(G)]
print('Gabriel triangles:',[''.join(g) for g in GabS])
faces=sorted(set(f for G in GabS for f in combinations(G,2)))
for name,psi in (('psi=1',lambda r2:1.0),('psi=1/r^3',lambda r2:float(r2)**-1.5),('psi=1/r',lambda r2:float(r2)**-0.5)):
    S={t:sum(psi(beta[G][1]) for G in GabS if set(t)<=set(G)) for t in faces}
    T={x:sum(S[t] for t in faces if x in t) for x in names}
    m={t:S[t]*sum(1/T[x] for x in t) for t in faces}
    tri=[('A','B'),('A','C'),('B','C')]
    print(name,' mass(ABC faces)=%.4f'%sum(m[t] for t in tri),' total=%.4f'%sum(m.values()))
