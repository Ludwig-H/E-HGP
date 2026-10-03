#!/usr/bin/env python3
"""Independent exact leaf model. No product import, execution or performance claim."""
from fractions import Fraction as F
from itertools import combinations, product, permutations
import json

CHECKS=0
def check(value, message):
    global CHECKS
    CHECKS+=1
    if not value: raise RuntimeError(message)
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def gauss(a,b):
    n=len(b); rows=[[F(x) for x in a[i]]+[F(b[i])] for i in range(n)]
    for k in range(n):
        piv=next((i for i in range(k,n) if rows[i][k]),None)
        if piv is None: return None
        rows[k],rows[piv]=rows[piv],rows[k]
        d=rows[k][k]; rows[k]=[x/d for x in rows[k]]
        for i in range(n):
            if i!=k:
                d=rows[i][k]; rows[i]=[x-d*y for x,y in zip(rows[i],rows[k])]
    return tuple(row[-1] for row in rows)
def sphere(points, ids):
    a=points[ids[0]]; u=[sub(points[i],a) for i in ids[1:]]
    lam=gauss([[dot(x,y) for y in u] for x in u],[F(dot(x,x),2) for x in u])
    if lam is None:return None
    c=tuple(F(a[j])+sum(lam[i]*u[i][j] for i in range(len(u))) for j in range(3))
    return c,dot(sub(c,a),sub(c,a)),(1-sum(lam),)+lam

def line_meets(points, ids, qbox):
    s=sphere(points,ids)
    if s is None:return False
    c=s[0]; u=sub(points[ids[1]],points[ids[0]]);v=sub(points[ids[2]],points[ids[0]])
    cross=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    low=high=None
    for j,d in enumerate(cross):
        if not d:
            if not(qbox[0][j]<=c[j]<=qbox[1][j]):return False
        else:
            a=F(qbox[0][j]-c[j],d);b=F(qbox[1][j]-c[j],d)
            a,b=min(a,b),max(a,b)
            low=a if low is None else max(low,a);high=b if high is None else min(high,b)
    return low is None or low<=high

def run(points,qbox,K,early=False,cut=False,pair_graph=False):
    n=len(points); corners=list(product(*zip(*qbox))); dom=[set() for _ in points]
    for i in range(n):
        for j in range(n):
            if i!=j and max(dot(sub(points[j],c),sub(points[j],c))-dot(sub(points[i],c),sub(points[i],c)) for c in corners)<0:dom[i].add(j)
    compatible=lambda a,b:b not in dom[a] and a not in dom[b]
    out=[]; led=dict(prefixes=0,pair_tests=0,pair_rejects=0,line_tests=0,line_rejects=0,g3_rejects=0,descendant_cuts=0,judged=0)
    def possible(t, bound=None):
        q=len(t)
        if not pair_graph:
            for j in t[:-1]:
                led['pair_tests']+=1
                if not compatible(j,t[-1]):led['pair_rejects']+=1;return False
        # Preserve pair-first accounting; commute only G3 and expensive J2 lines.
        if bound is not None and bound>K+1-q:
            led['g3_rejects']+=1;return False
        for a,b in combinations(t[:-1],2):
            led['line_tests']+=1
            if not line_meets(points,(a,b,t[-1]),qbox):led['line_rejects']+=1;return False
        return True
    def census(t):
        s=sphere(points,t)
        if s is None or min(s[2])<=0:return
        c,beta,_=s
        if not all(qbox[0][j]<=c[j]<qbox[1][j] for j in range(3)):return
        led['judged']+=1
        I=tuple(i for i,p in enumerate(points) if dot(sub(p,c),sub(p,c))<beta)
        U=tuple(i for i,p in enumerate(points) if dot(sub(p,c),sub(p,c))==beta)
        candidates=[]
        for q in range(2,5):
            for a in combinations(U,q):
                z=sphere(points,a)
                if z and min(z[2])>0 and z[0]==c and z[1]==beta:candidates.append(a)
            if candidates:break
        check(bool(candidates),'strict generating support must yield canonical support')
        canonical=min(candidates)
        if canonical==t and len(I)+len(canonical)<=K+1:
            out.append((canonical,c,beta,I,U))
    def dfs(prefix,mask):
        q=len(prefix)+1;threshold=K+1-q
        if threshold<0:return
        start=prefix[-1]+1 if prefix else 0
        for i in range(start,n):
            if pair_graph and any(not compatible(j,i) for j in prefix):continue
            t=prefix+(i,);led['prefixes']+=1;merged=mask|dom[i]
            if early and q==1 and len(merged)>threshold:led['g3_rejects']+=1;continue
            if q>=2 and not possible(t,len(merged) if early else None):continue
            if not early and len(merged)>threshold:led['g3_rejects']+=1;continue
            if q>=2:census(t)
            if q<4:
                if cut and len(merged)>K-q:led['descendant_cuts']+=1
                else:dfs(t,merged)
    dfs((),set())
    return sorted(out),led,dom

A=(0,0,0);B=(4,4,0);C=(4,0,4);D=(0,4,4);I=[(2,1,1),(3,1,1),(2,2,1)]
P=[A,B,C,*I,D];Q=((2,1,1),(3,3,3));Qc=((2,1,1),(3,2,2))
tri=sphere(P,(0,1,2));tet=sphere(P,(0,1,2,6))
check(tri[0]==(F(8,3),F(4,3),F(4,3)) and tri[1]==F(32,3),'triangle exact')
check(min(tri[2])>0 and min(tet[2])>0,'both strict supports')
check(tet[0]==(F(2),F(2),F(2)) and tet[1]==12,'tetra exact')
for qbox in [Q,Qc]:
    check(all(line_meets(P,t,qbox) for t in combinations((0,1,2,6),3)),'all four faces touch closed box')
rows=[]
for qbox,name in [(Q,'interior'),(Qc,'closed_contact')]:
    for graph in [False,True]:
        baseline,ledger,dom=run(P,qbox,5,pair_graph=graph)
        check(set.union(*(dom[i] for i in (0,1,2)))=={3,4,5},'exact triple dominance witness count')
        check(any(o[0]==(0,1,2) and o[3]==(3,4,5) for o in baseline),'admitted q3 must be retained')
        for early,cut,label in [(True,False,'early_g3'),(False,True,'descendant_threshold'),(True,True,'combined')]:
            output,newledger,_=run(P,qbox,5,early,cut,graph)
            check(output==baseline,'canonical support/centre/beta/full I/U unchanged')
            rows.append(dict(box=name,pair_graph=graph,variant=label,baseline=ledger,optimized=newledger,balls=len(output)))
# Limited metamorphic controls. This is a legal K-certified local leaf because it contains ALL X.
# It is not a claim that this Q/ordinal occurs in a public generator execution.
configs=0
for order in list(permutations([A,B,C,D]))[:6]:
    points=list(order)+I
    for K in [4,5,6]:
        for qbox in [Q,Qc]:
            baseline,_,_=run(points,qbox,K)
            for early,cut in [(True,False),(False,True),(True,True)]:
                result,_,_=run(points,qbox,K,early,cut)
                check(result==baseline,'ID permutations/K/contact invariance of full output')
            configs+=1
result={'status':'PASS','scope':'independent Fraction model of a legal all-X local leaf; no product execution, no timing or generator reachability claim','checks':CHECKS,'metamorphic_configurations':configs,'fixture':{'points':P,'Q':Q,'Q_contact':Qc,'K':5,'triangle':{'center':[str(x) for x in tri[0]],'beta':str(tri[1]),'weights':[str(x) for x in tri[2]]},'tetrahedron':{'center':[str(x) for x in tet[0]],'beta':str(tet[1]),'weights':[str(x) for x in tet[2]]}},'proof':'Dom(T) subset I(B) for support T and centre in closed Q. Union is monotone under extension. Any required canonical strict support of arity h extending prefix T requires p<=K+1-h. For h>=q+1, cut only descendants when current count>K-q; retain q emission and complete contacts. Noncanonical presentations can have smaller qmin: their ball remains visited through S*, so this does NOT assert that every geometric sphere of that presentation is inadmissible. Early G3 and J2 are independent necessary filters.','variants':rows}
print(json.dumps(result,sort_keys=True,indent=2))
