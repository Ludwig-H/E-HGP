"""Gamma exact a K=2 par formule close (MEB de paires et de triangles), pour n moyen."""
import sys, json, itertools
from fractions import Fraction as F
sys.argv=['x']
exec(open('gamma_check.py').read().replace('\nmain()\n','\n'))
def d2(p,q): return sum((a-b)**2 for a,b in zip(p,q))
def meb3(p,q,r):
    a2,b2,c2=d2(q,r),d2(p,r),d2(p,q)
    s=sorted([a2,b2,c2])
    if s[2]>=s[0]+s[1]: return F(s[2],4)
    u=[q[i]-p[i] for i in range(3)]; v=[r[i]-p[i] for i in range(3)]
    cr=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
    return F(a2*b2*c2, 4*sum(x*x for x in cr))
def gamma_counts(P, levels):
    n=len(P)
    pairs={T:F(d2(P[T[0]],P[T[1]]),4) for T in itertools.combinations(range(n),2)}
    tri=[(meb3(P[i],P[j],P[k]),(i,j,k)) for i,j,k in itertools.combinations(range(n),3)]
    events=sorted([(v,0,T) for T,v in pairs.items()]+[(v,1,T) for v,T in tri])
    uf={}
    def fd(x):
        while uf[x]!=x: uf[x]=uf[uf[x]]; x=uf[x]
        return x
    comps=0; out={}; ei=0
    for a in levels:
        while ei<len(events) and events[ei][0]<=a:
            v,kind,T=events[ei]; ei+=1
            if kind==0: uf[T]=T; comps+=1
            else:
                fs=[(T[0],T[1]),(T[0],T[2]),(T[1],T[2])]
                for f in fs:
                    if f not in uf: raise RuntimeError('facet born after coface?')
                for f in fs[1:]:
                    x,y=fd(f),fd(fs[0])
                    if x!=y: uf[x]=y; comps-=1
        out[a]=comps
    return out
if __name__=='__main__':
    src=sys.argv_real if hasattr(sys,'argv_real') else None
