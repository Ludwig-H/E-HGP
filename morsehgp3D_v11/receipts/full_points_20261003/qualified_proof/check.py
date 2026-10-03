#!/usr/bin/env python3
"""Autonomous exact Gamma/qualified-cover model. No product/reference imports."""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent

def need(ok,why):
    if not ok: raise ValueError(why)

def dot(a,b): return sum((x*y for x,y in zip(a,b)),Q(0))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dist2(a,b): return dot(sub(a,b),sub(a,b))

def solve(matrix,rhs):
    n=len(rhs); A=[[Q(x) for x in row]+[Q(b)] for row,b in zip(matrix,rhs)]
    for j in range(n):
        p=next((i for i in range(j,n) if A[i][j]),None)
        if p is None:return None
        A[j],A[p]=A[p],A[j]
        d=A[j][j];A[j]=[v/d for v in A[j]]
        for i in range(n):
            if i!=j:
                d=A[i][j];A[i]=[x-d*y for x,y in zip(A[i],A[j])]
    return tuple(A[i][-1] for i in range(n))

def circ(points,ids):
    a=points[ids[0]]
    if len(ids)==1:return a,Q(0)
    v=[sub(points[i],a) for i in ids[1:]]
    t=solve([[2*dot(x,y) for y in v] for x in v],[dot(x,x) for x in v])
    if t is None:return None
    c=tuple(a[j]+sum(ti*vi[j] for ti,vi in zip(t,v)) for j in range(3))
    return c,dist2(c,a)

class Geometry:
    def __init__(self,points):
        self.p=tuple(tuple(Q(x) for x in p) for p in points);self.n=len(points);self.meb={}
        # Independent minimum enclosure definition: no positive-support/early-exit product route.
        for size in range(1,min(self.n,4)+1):
            for F in combinations(range(self.n),size):
                candidates=[]
                for q in range(1,min(size,4)+1):
                    for S in combinations(F,q):
                        b=circ(self.p,S)
                        if b is not None and all(dist2(x,b[0])<=b[1] for x in (self.p[i] for i in F)):
                            candidates.append(b)
                need(candidates,'MEB exists')
                radius=min(b[1] for b in candidates)
                centers={b[0] for b in candidates if b[1]==radius}
                need(len(centers)==1,'MEB unique')
                self.meb[F]=(next(iter(centers)),radius)

    def levels(self,k):
        return sorted({b[1] for F,b in self.meb.items() if len(F) in (k,k+1)})

    def cover_components(self,k,a):
        vertices=[F for F,b in self.meb.items() if len(F)==k and b[1]<=a]
        parent={F:F for F in vertices}
        def find(F):
            while parent[F]!=F:F=parent[F]
            return F
        for H,b in self.meb.items():
            if len(H)!=k+1 or b[1]>a:continue
            faces=list(combinations(H,k));r=find(faces[0])
            for F in faces[1:]:parent[find(F)]=r
        cov={}
        for F in vertices:cov.setdefault(find(F),set()).update(F)
        return tuple(sorted(tuple(sorted(C)) for C in cov.values()))

    def projected(self,k,m,a):
        covers=self.cover_components(k,a)
        parent=list(range(self.n));active=set()
        def find(i):
            while parent[i]!=i:i=parent[i]
            return i
        for C in covers:
            if len(C)<m:continue
            active.update(C);r=find(C[0])
            for i in C[1:]:parent[find(i)]=r
        blocks={}
        for i in range(self.n):blocks.setdefault(find(i),set()).add(i)
        return tuple(sorted(tuple(sorted(B)) for B in blocks.values())),tuple(sorted(active))

    def hierarchy(self,k,m):
        n=self.n;heights=[[None]*n for _ in range(n)];entries=[None]*n
        cuts=[]
        for a in self.levels(k):
            P,active=self.projected(k,m,a);cuts.append((a,P,active))
            for i in active:
                if entries[i] is None:entries[i]=a
            for B in P:
                for i in B:
                    for j in B:
                        if heights[i][j] is None:heights[i][j]=a if i!=j else Q(0)
        need(all(e is not None for e in entries),'all sites eventually enter')
        need(all(x is not None for row in heights for x in row),'one terminal root')
        for i in range(n):
            for j in range(n):
                for l in range(n):need(heights[i][j]<=max(heights[i][l],heights[l][j]),'ultrametric')
        for (_,P,_),(_,R,_) in zip(cuts,cuts[1:]):need(refines(P,R),'laminar cuts')
        return cuts,entries,heights

def refines(P,R):return all(any(set(B)<=set(C) for C in R) for B in P)
def single_linkage(G,a):
    parent=list(range(G.n))
    def find(i):
        while parent[i]!=i:i=parent[i]
        return i
    for i,j in combinations(range(G.n),2):
        if dist2(G.p[i],G.p[j])<=4*a:parent[find(j)]=find(i)
    out={}
    for i in range(G.n):out.setdefault(find(i),[]).append(i)
    return tuple(sorted(tuple(v) for v in out.values()))

def direct_cocover(G,k,m):
    w=[[None]*G.n for _ in range(G.n)]
    for i in range(G.n):w[i][i]=Q(0)
    for a in G.levels(k):
        for C in G.cover_components(k,a):
            if len(C)<m:continue
            for i in C:
                for j in C:
                    if w[i][j] is None:w[i][j]=a
    need(all(v is not None for row in w for v in row),'finite co-cover')
    u=[row[:] for row in w]
    for z in range(G.n):
        for i in range(G.n):
            for j in range(G.n):u[i][j]=min(u[i][j],max(u[i][z],u[z][j]))
    return w,u
def serial(x):
    if isinstance(x,Q):return str(x)
    if isinstance(x,dict):return {k:serial(v) for k,v in x.items()}
    if isinstance(x,(tuple,list)):return [serial(v) for v in x]
    return x

def triangles(d):
    return [(268,3000,0),(268,1000,0),(2000,2000,0),(2000+d,2000,0),(3732+d,3000,0),(3732+d,1000,0)]

results=[];guards=0
for bridge in (2000,1998,1700):
    X=triangles(bridge);G=Geometry(X)
    a=G.meb[(0,1,2)][1]
    need(a==G.meb[(3,4,5)][1],'symmetric within-triangle date')
    cov=G.cover_components(2,a)
    need(cov==((0,1,2),(2,3),(3,4,5)),'three FULL covers')
    P,active=G.projected(2,3,a)
    need(P==((0,1,2),(3,4,5)) and len(active)==6,'m3 retains both triangles')
    P2,_=G.projected(2,2,a)
    need(P2==(tuple(range(6)),),'m2 percolates through CD')
    mixed=min(b[1] for F,b in G.meb.items() if len(F)==3 and not(set(F)<={0,1,2} or set(F)<={3,4,5}))
    need(mixed>a,'positive separation interval')
    need(G.projected(2,3,(a+mixed)/2)[0]==P,'triangles persist strictly before cross triple')
    # For m3, the six all-points pair heights belong to the intended two equal blocks before fusion.
    cuts,entries,H=G.hierarchy(2,3)
    need(entries==[a]*6,'all entries at triangle qualification')
    need(all(H[i][j]==a for B in ((0,1,2),(3,4,5)) for i in B for j in B if i!=j),'within triangle heights')
    # Check complete same-cut verticality at fixed m, including levels from all three orders.
    levels=sorted(set().union(*(G.levels(k) for k in (1,2,3))))
    for level in levels:
        for k in (2,3):
            need(refines(G.projected(k,3,level)[0],G.projected(k-1,3,level)[0]),'same-cut verticality')
            guards+=1
    # Relabeling and a nontrivial grid isometry leave geometric groups unchanged after undoing the labels.
    perm=(5,3,1,4,0,2);GP=Geometry([X[i] for i in perm]);PG=GP.projected(2,3,a)[0]
    need(tuple(sorted(tuple(sorted(perm[i] for i in B)) for B in PG))==P,'permutation equivariance')
    GI=Geometry([(10000-y,10000+x,z+7) for x,y,z in X])
    need(GI.projected(2,3,a)[0]==P,'grid isometry equivariance')
    results.append({'case':'two_triangles_'+str(bridge),'within_triangle_beta':a,'first_mixed_triple_beta':mixed,'full_cover_at_triangle_date':cov,'m3_blocks':P,'m2_blocks':P2,'m2_root_beta':max(x for row in G.hierarchy(2,2)[2] for x in row),'m3_entries':entries,'m3_pair_heights':H,'cuts':len(cuts)})

G=Geometry([(x,0,0) for x in (0,2,100,102)])
need(G.projected(2,3,Q(1))[1]==(),'m3 vetoes the two small pairs')
need(G.projected(2,3,Q(2401))[1]==(),'m3 still inactive before FULL fusion')
need(G.projected(2,3,Q(2500))==( ((0,1,2,3),),(0,1,2,3)),'m3 enters only as four-site root')
need(G.projected(2,2,Q(1))[0]==((0,1),(2,3)),'m2 retains small pairs')
need(G.projected(2,2,Q(2401))[0]==((0,1,2,3),),'m2 connects at bridge before FULL fusion')
results.append({'case':'two_pairs','m3_first_root_beta':2500,'m2_root_beta':2401,'full_root_beta':2500,'m3_has_two_point_branches':False})

# Translation of {(0,0),(-6,-2),(-6,2),(6,-2),(6,2)} into the profile domain.
X=[(6,2,0),(0,0,0),(0,4,0),(12,0,0),(12,4,0)]
G=Geometry(X);a=Q(100,9)
need(G.meb[(0,1,2)][1]==a and G.meb[(0,3,4)][1]==a,'qualified cover triangles')
need(G.cover_components(2,a)==((0,1,2),(0,3,4)),'two distinct overlapping FULL covers')
need(G.projected(2,3,a)==(((0,1,2,3,4),),(0,1,2,3,4)),'m3 shared boundary percolates')
need(G.cover_components(2,Q(36)-Q(1,10000))==((0,1,2),(0,3,4)),'FULL separate strictly before 36')
need(G.cover_components(2,Q(36))==((0,1,2,3,4),),'FULL fusion beta36')
need(sorted(dist2(G.p[0],p) for p in G.p)[1]==40,'shared point core date40')
results.append({'case':'shared_boundary','points':X,'qualified_cover_beta':a,'full_cover':G.cover_components(2,a),'exclusive_counts_per_component':[2,2],'m3_projected_root_beta':a,'full_root_beta':36,'shared_core_beta':40})

sl_guards=0;closure_guards=0
for X in [triangles(d) for d in (2000,1998,1700)]+[[(x,0,0) for x in (0,2,100,102)],[(6,2,0),(0,0,0),(0,4,0),(12,0,0),(12,4,0)]]:
    G=Geometry(X)
    for a in G.levels(2):
        need(G.projected(2,2,a)[0]==single_linkage(G,a),'K2/m2 = SL at2r')
        sl_guards+=1
    for m in (2,3):
        w,u=direct_cocover(G,2,m)
        H=G.hierarchy(2,m)[2]
        need(u==H,'partition closure = minmax co-cover closure')
        need(all(u[i][j]<=w[i][j] for i in range(G.n) for j in range(G.n)),'closure dominated by deadlines')
        closure_guards+=1

output={'schema':'ehgp.v11.qualified_cover_exact_check.v1','model':'independent Gamma components + qualified hyperedge transitive closure','product_or_reference_imports':False,'native_runs':0,'vertical_cut_checks':guards,'single_linkage_cut_checks':sl_guards,'minmax_closure_checks':closure_guards,'cases':results}
text=json.dumps(serial(output),ensure_ascii=False,sort_keys=True,separators=(',',':'))+'\n'
print(text,end='')
