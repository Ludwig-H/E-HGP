#!/usr/bin/env python3
"""Standalone exact Gamma checks and classifier cost model; no product/reference imports."""
from fractions import Fraction as F
from itertools import combinations
from functools import lru_cache
from math import comb
import json


def require(ok, why):
    if not ok:
        raise ValueError(why)


def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def d2(a,b): return dot(sub(a,b),sub(a,b))


def solve(matrix,rhs):
    n=len(rhs)
    rows=[[F(x) for x in matrix[i]]+[F(rhs[i])] for i in range(n)]
    for j in range(n):
        pivot=next((i for i in range(j,n) if rows[i][j]),None)
        if pivot is None:return None
        rows[j],rows[pivot]=rows[pivot],rows[j]
        q=rows[j][j];rows[j]=[x/q for x in rows[j]]
        for i in range(n):
            if i!=j:
                q=rows[i][j];rows[i]=[x-q*y for x,y in zip(rows[i],rows[j])]
    return tuple(row[-1] for row in rows)


def circum(points):
    a=points[0]
    edges=[sub(x,a) for x in points[1:]]
    w=solve([[dot(x,y) for y in edges] for x in edges],[F(dot(x,x),2) for x in edges])
    if w is None:return None
    c=tuple(F(a[j])+sum(t*x[j] for t,x in zip(w,edges)) for j in range(3))
    return c,d2(c,a)


@lru_cache(maxsize=None)
def meb(points):
    candidates=[]
    for q in range(1,min(4,len(points))+1):
        for support in combinations(points,q):
            b=circum(support)
            if b is not None and all(d2(b[0],p)<=b[1] for p in points):candidates.append(b)
    require(candidates,'no enclosing affine support')
    beta=min(b[1] for b in candidates)
    winners=set(b[0] for b in candidates if b[1]==beta)
    require(len(winners)==1,'MEB uniqueness')
    return next(iter(winners)),beta


class Gamma:
    """Definition graph from all k and k+1 subsets; independent of critical balls/cells/descent."""
    def __init__(self,points,k,linear=False):
        self.points=points;self.k=k
        self.vertices=tuple(combinations(range(len(points)),k))
        self.edges=tuple(combinations(range(len(points)),k+1))
        def beta(part):
            if linear:return F((max(part)-min(part))**2,4)
            return meb(tuple(points[i] for i in part))[1]
        self.vlevel={v:beta(v) for v in self.vertices}
        self.elevel={g:beta(g) for g in self.edges}

    def components(self,level,closed=True):
        active=lambda r:r<=level if closed else r<level
        vertices=[v for v in self.vertices if active(self.vlevel[v])]
        parent={v:v for v in vertices}
        def find(v):
            while parent[v]!=v:v=parent[v]
            return v
        for union in self.edges:
            if not active(self.elevel[union]):continue
            faces=tuple(combinations(union,self.k))
            require(all(f in parent for f in faces),'edge before vertex')
            a=find(faces[0])
            for face in faces[1:]:parent[find(face)]=a
        groups={}
        for v in vertices:groups.setdefault(find(v),set()).add(v)
        return sorted((frozenset(g) for g in groups.values()),key=lambda g:min(g))


def morton(p):
    return sum(((p[j]>>b)&1)<<(3*b+j) for b in range(24) for j in range(3))


centre=(2,2,2)
octa=tuple(sorted(((4,2,2),(0,2,2),(2,4,2),(2,0,2),(2,2,4),(2,2,0),centre),key=morton))
shell=tuple(p for p in octa if p!=centre)
require(shell[:2]==((2,2,0),(2,0,2)),'wrong first Morton pair')
require(all(d2(p,centre)==4 for p in shell),'wrong shell')
betas=[meb(pair)[1] for pair in combinations(shell,2)]
strict=sum(b<4 for b in betas)
require(strict==12 and betas.count(F(4))==3 and betas[0]==2,'wrong T2 witnesses')
g=Gamma(octa,3)
for level,closed,count,active in [(F(2),False,0,0),(F(2),True,12,12),
                                  (F(8,3),False,12,12),(F(8,3),True,1,20),
                                  (F(4),False,1,20),(F(4),True,1,35)]:
    groups=g.components(level,closed)
    require(len(groups)==count and sum(map(len,groups))==active,'wrong octa Gamma cut')
old=g.components(F(8,3),False);new=g.components(F(8,3),True)
require(all(group<=new[0] for group in old),'plateau does not merge all twelve')
# Only the critical central cell's classification/replay costs are counted, not the whole forest.
C=comb(6,2);payload=strict*52
current=dict(classify_passes=2,replay_passes=2,trace_tests=4*C,meb_A_calls=4*C,
             trace_payload_allocations=2,each_payload_bytes=payload)
proposed=dict(classify_candidates=1,classify_meb_A_calls=1,classify_trace_payload_allocations=0,
              replay_passes=2,replay_trace_tests=2*C,replay_meb_A_calls=2*C,
              total_trace_tests=1+2*C,total_meb_A_calls=1+2*C,trace_payload_allocations=1,
              each_payload_bytes=payload)
require(current['trace_tests']==60 and proposed['total_trace_tests']==31 and payload==624,'cost arithmetic')
# t<qmin and t=m retain their exact semantics; criticality implies MEB(U)=the critical ball.
require(meb(shell)[0]==tuple(map(F,centre)) and meb(shell)[1]==4,'whole shell level differs')
require(all(meb((p,))[1]==0 for p in shell),'singleton not strict')
# K12 on thirteen integer collinear sites: includes the critical coface of cardinal K+1.
line=tuple((i,0,0) for i in range(13));upper=Gamma(line,12,True);lower=Gamma(line,11,True)
u0=upper.components(F(121,4),True);u1=upper.components(F(36),False);u2=upper.components(F(36),True)
require(len(u0)==2 and len(u1)==2 and len(u2)==1 and len(u2[0])==13,'wrong K12 coface closure')
require(upper.elevel[tuple(range(13))]==36,'wrong coface13 level')
require({next(iter(group)) for group in u0}=={tuple(range(12)),tuple(range(1,13))},'wrong K12 births')
l0=lower.components(F(25),True);l1=lower.components(F(121,4),False);l2=lower.components(F(121,4),True)
require(len(l0)==3 and len(l1)==3 and len(l2)==1 and len(l2[0])==23,'wrong K11 closed plateau')
for v in (tuple(range(11)),tuple(range(1,12))):
    require(v in l2[0],'birth lower image not root after exact contact')
# Mathematical canonical forest: K11 births0..2/root3; K12 births0..1/root2.
verticals=(3,3,3)
print(json.dumps({'scope':'standalone Fraction/definition Gamma, no native/build/product-reference imports',
 'octa_sites_morton':octa,'octa_shell_morton_keys':[morton(p) for p in shell],
 'central_cell':{'order':3,'p':1,'m':6,'qmin':2,'beta':'4','t':2,'combinations':C,
                 'strict_traces':strict,'contacts':3,'pieces_before_beta4':1},
 'gamma3':{'births':12,'birth_beta':'2','merge_beta':'8/3','merge_children':12,'beta4':'continuation'},
 'current_central_cell':current,'proposed_classifier_then_exhaustive_replay':proposed,
 'classification_only_shortcuts':{'t_less_qmin':'nonbirth, no geometry/buffer','t_equal_m':'birth, no geometry/buffer'},
 'K12_line13':{'births':2,'birth_beta':'121/4','birth_centres':['11/2','13/2'],
               'merge_beta':'36','merge_children':2,'critical_p':11,'critical_qmin':2,
               'critical_coface_sites':13,'admission_p_plus_qmin':13,
               'trace_cardinality':12,'K11_births':3,'K11_merge_beta':'121/4','closed_lower':verticals},
 'cost_limits':'symbolic per-cell counts only; no wall-time/allocator-runtime/full-work bound'},sort_keys=True))
