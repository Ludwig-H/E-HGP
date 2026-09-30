"""Exact four-site counterexample to the claimed MM membership in D(gamma,1)."""
from fractions import Fraction as F
from itertools import combinations
import json


def require(ok, why):
    if not ok:
        raise ValueError(why)


def dot(a,b): return sum((x*y for x,y in zip(a,b)),F(0))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def d2(a,b): return dot(sub(a,b),sub(a,b))


def solve(a,b):
    m=[list(row)+[x] for row,x in zip(a,b)]
    for j in range(len(b)):
        i=next((i for i in range(j,len(b)) if m[i][j]),None)
        if i is None: return None
        m[j],m[i]=m[i],m[j]
        q=m[j][j];m[j]=[x/q for x in m[j]]
        for i in range(len(b)):
            if i!=j:
                q=m[i][j];m[i]=[x-q*y for x,y in zip(m[i],m[j])]
    return [row[-1] for row in m]


def sphere(P,S):
    a=P[S[0]];ds=[sub(P[i],a) for i in S[1:]]
    if not ds: return a,F(0)
    ts=solve([[dot(x,y) for y in ds] for x in ds],[dot(x,x)/2 for x in ds])
    if ts is None or min([1-sum(ts),*ts])<=0: return None
    c=tuple(a[j]+sum((t*x[j] for t,x in zip(ts,ds)),F(0)) for j in range(3))
    return c,d2(c,a)


def meb(P,S):
    valid=[]
    for q in range(1,min(4,len(S))+1):
        for support in combinations(S,q):
            ball=sphere(P,support)
            if ball is not None and all(d2(P[i],ball[0])<=ball[1] for i in S): valid.append(ball)
    require(valid,'no MEB')
    return min(valid,key=lambda b:b[1])[1]


def main():
    rho=F(6,5);c=F(99,101);s=F(20,101)
    gamma=F(1,50);kappa=F(25);eta=F(1)
    P=[(F(0),F(0),F(0)),(F(-2),F(0),F(0)),(2*rho*c,2*rho*s,F(0)),(2*rho*c,-2*rho*s,F(0))]
    require(c*c+s*s==1 and s>0 and len(set(P))==4,'distinct unit directions')
    facets=list(combinations(range(4),2));edges=list(combinations(range(4),3))
    birth={S:meb(P,S) for S in facets};events={S:meb(P,S) for S in edges}
    A=min(v for S,v in birth.items() if 0 in S)
    weights={S:max(F(0),1+(1-v/A)/eta) for S,v in birth.items() if 0 in S}
    W=sum(weights.values(),F(0));T=None;prev=F(0);terms=[];trace=[]
    for beta in sorted(set(birth.values())|set(events.values())):
        active=[S for S in facets if birth[S]<=beta];parent={S:S for S in active}
        def find(S):
            while parent[S]!=S:S=parent[S]
            return S
        for E,v in events.items():
            if v<=beta:
                faces=list(combinations(E,2))
                require(all(S in parent for S in faces),'unborn event endpoint')
                for S in faces[1:]:parent[find(S)]=find(faces[0])
        masses={}
        for S,w in weights.items():
            if birth[S]<=beta:masses[find(S)]=masses.get(find(S),F(0))+w
        G=max(masses.values()) if masses else F(0)
        if T is None and 2*G>W:T=beta
        elif T is not None and G>prev:terms.append((beta,2*prev/W-1))
        trace.append(dict(beta=str(beta),G=str(G),W=str(W)))
        prev=G
    f2=F(12101,2525);TR=F(202,165)
    require(A==1 and W==F(53,25) and weights=={(0,1):F(1),(0,2):F(14,25),(0,3):F(14,25)},'soft weights')
    require(T==TR*TR and terms==[(f2,F(3,53))],'Gamma majority and post-majority event')
    # Exact squared comparisons, both sides nonnegative.
    require(f2<(TR+kappa*F(3,53))**2,'MM cone term can exceed T_half')
    require(F(28,53)>F(1,2)+gamma and 2*kappa*gamma==1,'D premise')
    require(f2<(1+rho)**2 and TR>rho,'direction error')
    require((TR+1)**2>f2,'MM date is not later than f-1')
    print(json.dumps(dict(status='MM_D_GAMMA_G1_MEMBERSHIP_COUNTEREXAMPLE',K=2,
        points=[[str(v) for v in q] for q in P],rho=str(rho),gamma=str(gamma),kappa=str(kappa),eta=str(eta),
        alpha2=str(A),W=str(W),right_share='28/53',right_margin='3/53',
        first_majority_radius=str(TR),MM_date_radius=str(TR),fusion_radius2=str(f2),
        claimed_deadline='sqrt(12101/2525)-1',actual_date_later_than_deadline=True,
        all_pair_births={str(S):str(v) for S,v in birth.items()},all_triple_events={str(S):str(v) for S,v in events.items()},
        trace=trace,native_calls=0,GCP_used=False,
        scope='exact rational real-model counterexample to g=1 class membership; not a grid/native or uniform-stability refutation'),sort_keys=True,indent=2))


if __name__=='__main__':main()
