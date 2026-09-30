"""Exact Q(sqrt(3)) six-point Gamma_2 oracle, not the native grid engine.

No float decisions, no developer geometry/ownership helper, no assert gates.
All pairs are nerve vertices; adjacency uses the MEB of their union.
"""
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import combinations
import json


def require(ok, why):
    if not ok:
        raise ValueError(why)


@dataclass(frozen=True)
class Q:
    a: F = F(0)
    b: F = F(0)

    def __post_init__(self):
        object.__setattr__(self,'a',F(self.a))
        object.__setattr__(self,'b',F(self.b))

    @staticmethod
    def cast(v):
        return v if isinstance(v,Q) else Q(v)

    def __add__(self,v):
        v=self.cast(v);return Q(self.a+v.a,self.b+v.b)
    __radd__=__add__
    def __neg__(self):return Q(-self.a,-self.b)
    def __sub__(self,v):return self+-self.cast(v)
    def __rsub__(self,v):return self.cast(v)+-self
    def __mul__(self,v):
        v=self.cast(v);return Q(self.a*v.a+3*self.b*v.b,self.a*v.b+self.b*v.a)
    __rmul__=__mul__
    def __truediv__(self,v):
        v=self.cast(v);d=v.a*v.a-3*v.b*v.b
        require(d!=0,'zero divisor')
        return self*Q(v.a/d,-v.b/d)
    def __rtruediv__(self,v):return self.cast(v)/self
    def __pow__(self,n):
        require(type(n) is int and n>=0,'nonnegative integer power')
        out=Q(1)
        for _ in range(n):out=out*self
        return out
    def sign(self):
        a,b=self.a,self.b
        if not b:return (a>0)-(a<0)
        if not a:return (b>0)-(b<0)
        if (a>0)==(b>0):return 1 if a>0 else -1
        d=a*a-3*b*b
        return ((d>0)-(d<0))*(1 if a>0 else -1)
    def __lt__(self,v):return (self-self.cast(v)).sign()<0
    def __le__(self,v):return (self-self.cast(v)).sign()<=0
    def encoded(self):return [str(self.a),str(self.b)]


def dot(p,q):return sum((a*b for a,b in zip(p,q)),Q())
def dist2(p,q):return dot(tuple(a-b for a,b in zip(p,q)),tuple(a-b for a,b in zip(p,q)))


def candidates(points):
    rows=[]
    for i,p in enumerate(points):rows.append(((i,),p,Q()))
    for i,j in combinations(range(len(points)),2):
        c=tuple((a+b)/2 for a,b in zip(points[i],points[j]))
        rows.append(((i,j),c,dist2(c,points[i])))
    for ids in combinations(range(len(points)),3):
        p,q,s=(points[i] for i in ids)
        h=2*(p[0]*(q[1]-s[1])+q[0]*(s[1]-p[1])+s[0]*(p[1]-q[1]))
        if h.sign()==0:continue
        pp,qq,ss=dot(p,p),dot(q,q),dot(s,s)
        c=((pp*(q[1]-s[1])+qq*(s[1]-p[1])+ss*(p[1]-q[1]))/h,
           (pp*(s[0]-q[0])+qq*(p[0]-s[0])+ss*(q[0]-p[0]))/h)
        rows.append((ids,c,dist2(c,p)))
    return rows


def all_meb(points):
    rows=candidates(points);result={}
    for m in range(1,len(points)+1):
        for ids in combinations(range(len(points)),m):
            valid=[(beta,support) for support,c,beta in rows if set(support)<=set(ids) and
                   all(dist2(c,points[i])<=beta for i in ids)]
            require(valid,'no enclosing candidate')
            result[ids]=min(v[0] for v in valid)
    return result


def gamma(meb,beta):
    pairs=[pair for pair in combinations(range(6),2) if meb[pair]<=beta]
    parent=list(range(len(pairs)))
    def root(i):
        while parent[i]!=i:i=parent[i]
        return i
    for i,j in combinations(range(len(pairs)),2):
        if meb[tuple(sorted(set(pairs[i])|set(pairs[j])))]<=beta:
            a,b=root(i),root(j)
            if a!=b:parent[b]=a
    groups={}
    for i,pair in enumerate(pairs):groups.setdefault(root(i),[]).append(pair)
    return sorted((tuple(sorted(ps)) for ps in groups.values()))


def covers(components):
    return sorted(tuple(sorted(set(x for pair in comp for x in pair))) for comp in components)


def majority(meb,beta,eta,weighted):
    components=gamma(meb,beta)
    owner={pair:i for i,comp in enumerate(components) for pair in comp}
    blocks={}
    for x in range(6):
        incident=[pair for pair in combinations(range(6),2) if x in pair]
        alpha2=min(meb[pair] for pair in incident)
        universe=[pair for pair in incident if meb[pair]<=alpha2*(1+eta)**2]
        weight={pair:(Q(1)/meb[pair] if weighted else Q(1)) for pair in universe}
        total=sum(weight.values(),Q());vote={}
        for pair,w in weight.items():
            if pair in owner:vote[owner[pair]]=vote.get(owner[pair],Q())+w
        win=[c for c,w in vote.items() if total/2<w]
        require(len(win)<=1,'majority collision')
        key=('component',win[0]) if win else ('singleton',x)
        blocks.setdefault(key,[]).append(x)
    return sorted(tuple(xs) for xs in blocks.values())


def run():
    s=Q(0,1)
    pts=[(-s,Q(1)),(-s,Q(-1)),(Q(),Q()),(Q(2),Q()),(Q(2)+s,Q(1)),(Q(2)+s,Q(-1))]
    edges=[(0,1),(0,2),(1,2),(2,3),(3,4),(3,5),(4,5)]
    require(all(dist2(pts[i],pts[j])==Q(4) for i,j in edges),'equilateral/bridge equality')
    meb=all_meb(pts)
    tri=Q(F(4,3));join=Q(2,1)
    require(meb[(0,1,2)]==meb[(3,4,5)]==tri,'triangle MEB')
    require(meb[(0,2,3)]==meb[(2,3,4)]==join,'bridge attachment MEB')
    before=tri-Q(F(1,1000000));after=join-Q(F(1,1000000))
    require(len(gamma(meb,before))==7,'seven independent first branches')
    expected=[(0,1,2),(2,3),(3,4,5)]
    require(covers(gamma(meb,tri))==covers(gamma(meb,after))==expected,'three overlapping FULL components')
    require(covers(gamma(meb,join))==[tuple(range(6))],'global closed multifusion')
    # Step activation, uniform fixed-band weights, before hard majority:
    # each point distributes one unit over its first incident edges.
    mass_rows=[]
    for comp in gamma(meb,tri):
        value=F(0)
        for x in range(6):
            incident=[pair for pair in edges if x in pair]
            value+=F(sum(pair in comp for pair in incident),len(incident))
        mass_rows.append((tuple(sorted(set(x for pair in comp for x in pair))),value))
    require(sorted(mass_rows)==[((0,1,2),F(8,3)),((2,3),F(2,3)),((3,4,5),F(8,3))] and
            sum(v for _,v in mass_rows)==6,'fractional step-band mass')
    levels=sorted(set(meb[pair] for pair in combinations(range(6),2))|
                  {meb[tuple(sorted(set(a)|set(b)))] for a,b in combinations(list(combinations(range(6),2)),2)})
    cuts=sorted(set(levels+[Q()]+[(a+b)/2 for a,b in zip(levels,levels[1:])]+[Q(F(169,100)),Q(F(289,100))]))
    target=[(0,1,2),(3,4,5)]
    pair_checks=target_checks=0
    for eta in (F(0),F(1,8)):
        for weighted in (False,True):
            partitions=[majority(meb,beta,eta,weighted) for beta in cuts]
            for old,new in zip(partitions,partitions[1:]):
                require(all(any(set(block)<=set(big) for big in new) for block in old),'majority nonlaminar')
                pair_checks+=1
            for beta,p in zip(cuts,partitions):
                if tri<=beta and beta<join:
                    require(p==target,'point hierarchy target missed')
                    target_checks+=1
    # First coverage may break ties as soon as coordinate equality changes;
    # no jitter or native-grid stability is inferred from this exact model.
    return dict(status='IDEAL_TWO_TRIANGLES_PASS',field='Q(sqrt(3))',K=2,r=1,
                point_order=list('ABCDEF'),coordinates=[[v.encoded() for v in p] for p in pts],
                subsets=len(meb),critical_pair_nerve_levels=len(levels),cuts=len(cuts),
                beta_triangle=tri.encoded(),beta_global=join.encoded(),
                full_cover_between=[list(xs) for xs in expected],point_target=[list(xs) for xs in target],
                majority_uniform_C_at_triangle=['2','3'],majority_C_margin_over_half=['1','6'],
                fractional_step_band_masses=[dict(points=list(xs),mass=str(v)) for xs,v in sorted(mass_rows)],
                hard_projected_masses=[3,3],
                target_checks=target_checks,laminar_adjacent_checks=pair_checks,
                first_cohort_C=['AC','BC','CD'],first_cohort_D=['CD','DE','DF'],
                native_executions=0,scope='ideal thesis geometry; band universe consists of all incident pairs; not native grid or larger-K weighting')


if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True,indent=2))
