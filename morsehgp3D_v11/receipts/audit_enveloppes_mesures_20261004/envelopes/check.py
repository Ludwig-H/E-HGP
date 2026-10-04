#!/usr/bin/env python3
"""M3/E4 : centres Gram/Fraction, enveloppes et compteurs, sans imports produit.

M3. Les trois milieux forment un triangle semblable au triangle initial.
Le centre circonscrit O est son orthocentre : pour chaque côté BC,
(O-(B+C)/2)·(B-C)=0, par égalité des deux distances. Un triangle
strictement aigu a son orthocentre strictement intérieur. Donc O est
dans l'enveloppe fermée des trois milieux, y compris quand elle est
plate sur un axe. Les poids médiaux exacts sont 1-2*lambda_i,
lambda_i étant les poids barycentriques de O dans le triangle initial.

E4. Un centre strictement intérieur à un tétraèdre est une combinaison
convexe strictement positive de ses QUATRE sommets ; l'enveloppe des
quatre sommets le contient. Sans strict intérieur, cette implication
n'existe pas, mais l'ancien filtre de positivité rejette déjà le tuple.

Une enveloppe fermée [l,h] rencontre l'intervalle propriétaire [lo,hi)
ssi h>=lo et l<hi. Doubler exactement toutes les coordonnées conserve
ces signes et les contacts. Ces rejets ne gouvernent pas la récursion
du préfixe q3 vers q4 ; on compare ici les décisions locales, pas un
catalogue natif ni ses chronos. Le compteur q4_candidates reste un
compte de présentations non dégénérées AVANT le rejet E4.
"""
from fractions import Fraction as F
from itertools import permutations, combinations, product
from math import comb
import json

CHECKS=0

def need(v,m):
    global CHECKS
    CHECKS+=1
    if not v:raise ValueError(m)

def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])

def gram(points):
    a=points[0];us=[sub(p,a) for p in points[1:]];n=len(us)
    rows=[[F(dot(u,v)) for v in us]+[F(dot(u,u),2)] for u in us]
    for i in range(n):
        pivot=next((j for j in range(i,n) if rows[j][i]),None)
        if pivot is None:return None
        rows[i],rows[pivot]=rows[pivot],rows[i];r=rows[i][i]
        rows[i]=[v/r for v in rows[i]]
        for j in range(n):
            if j!=i:
                r=rows[j][i];rows[j]=[x-r*y for x,y in zip(rows[j],rows[i])]
    w=[row[-1] for row in rows]
    c=tuple(F(a[d])+sum(x*u[d] for x,u in zip(w,us)) for d in range(3))
    return c,[1-sum(w)]+w

def acute(p):
    return all(dot(sub(p[(i+1)%3],p[i]),sub(p[(i+2)%3],p[i]))>0 for i in range(3))

def vertices(p):return [tuple(2*x for x in a) for a in p]
def median(p):return [tuple(a+b for a,b in zip(p[i],p[j])) for i,j in combinations(range(3),2)]

def meets(env,box):
    lo,hi=box
    return all(max(p[d] for p in env)>=2*lo[d] and min(p[d] for p in env)<2*hi[d] for d in range(3))

def owned(c,box):return all(l<=x<h for l,x,h in zip(box[0],c,box[1]))

def boxes_for(c):
    # Exact local boxes around, below, and above each coordinate; includes lo/hi contacts.
    low=tuple(x.numerator//x.denominator for x in c)
    boxes=[(low,tuple(x+1 for x in low))]
    for d in range(3):
        for offset in (-2,-1,1,2):
            lo=list(low);lo[d]+=offset
            boxes.append((tuple(lo),tuple(x+1 for x in lo)))
    return boxes

def test_tuple(p,boxes):
    q=len(p);solution=gram(p)
    if q==4:
        a,b,c,d=p;u,v,w=sub(b,a),sub(c,a),sub(d,a)
        det1=dot(cross(u,v),w);det2=dot(u,cross(v,w))
        need(det1==det2,'orientation equals q4 factory scalar triple product')
        need((solution is None)==(det1==0),'nondegenerate Gram iff orientation nonzero')
        oldcount=1 if solution else 0;newcount=1 if det1 else 0
        need(oldcount==newcount,'q4_candidates unchanged before E4 rejection')
    if solution is None:return {'decisions':0,'envelope_rejects':0,'accepted':0}
    c,weights=solution
    strict=acute(p) if q==3 else all(x>0 for x in weights)
    env=median(p) if q==3 else vertices(p)
    if strict:
        if q==3:
            mu=[1-2*x for x in weights]
            need(all(x>0 for x in mu),'acute center strictly inside medial triangle')
            need(sum(mu)==1,'medial barycentric sum')
            for i in range(3):
                a2=dot(sub(p[(i+1)%3],p[(i+2)%3]),sub(p[(i+1)%3],p[(i+2)%3]))
                b2=dot(sub(p[i],p[(i+2)%3]),sub(p[i],p[(i+2)%3]))
                c2=dot(sub(p[i],p[(i+1)%3]),sub(p[i],p[(i+1)%3]))
                den=2*(a2*b2+b2*c2+c2*a2)-a2*a2-b2*b2-c2*c2
                need(mu[i]==F((a2-b2+c2)*(a2+b2-c2),den),'positive medial weight factorization')
            mids=[tuple(F(p[j][d]+p[k][d],2) for d in range(3)) for j,k in ((1,2),(0,2),(0,1))]
            need(tuple(sum(w*m[d] for w,m in zip(mu,mids)) for d in range(3))==c,'medial barycentric center')
            for i,j in combinations(range(3),2):
                midpoint=tuple(F(p[i][d]+p[j][d],2) for d in range(3))
                need(dot(sub(c,midpoint),sub(p[i],p[j]))==0,'medial altitude orthogonality')
        need(all(min(x[d] for x in env)<=2*c[d]<=max(x[d] for x in env) for d in range(3)),'strict accepted center lies in appropriate envelope')
    rejects=accepts=decisions=0
    for box in boxes:
        if any(l<0 for l in box[0]):continue # T0 exact nonnegative owner boxes only.
        decisions+=1
        old=bool(strict and owned(c,box));new=bool(strict and meets(env,box) and owned(c,box))
        need(old==new,'old versus envelope local admission unchanged')
        if not meets(env,box):
            rejects+=1
            need(not old,'rejected envelope cannot contain an old admitted center')
        if old:accepts+=1
    return {'decisions':decisions,'envelope_rejects':rejects,'accepted':accepts}

def main():
    flat=[(2,0,0),(2,4,0),(2,1,3)]
    flat_tetra=[(20,20,20),(30,20,20),(20,30,20),(21,21,21)]
    tall=[(0,0,0),(30,0,0),(15,25,0),(15,8,30)]
    obtuse_prefix=[(1,2,6),(8,4,8),(2,1,3),(7,8,5)]
    initial=[flat,[(0,0,0),(4,0,0),(0,4,0)],[(0,0,0),(4,0,0),(1,1,0)],[(0,0,0),(2,0,0),(4,0,0)],flat_tetra,tall,obtuse_prefix,[(0,0,0),(2,2,0),(2,0,2),(0,2,2)],[(0,0,0),(2,0,0),(0,2,0),(2,2,0)]]
    totals={'tuples':0,'decisions':0,'envelope_rejects':0,'accepted':0}
    for p in initial:
        solution=gram(p)
        boxes=boxes_for(solution[0]) if solution else [((0,0,0),(4,4,4))]
        for order in permutations(p):
            r=test_tuple(order,boxes);totals['tuples']+=1
            for key in r:totals[key]+=r[key]
    # Bounded additional triangles in 3D; no stochastic search or native dependency.
    sites=list(product((1,3,5),(1,4),(2,6)))
    for p in combinations(sites,3):
        solution=gram(p)
        if solution is None:continue
        r=test_tuple(p,boxes_for(solution[0]));totals['tuples']+=1
        for key in r:totals[key]+=r[key]
    cf,wf=gram(flat);boxf=((2,2,1),(3,3,2))
    need(cf==(F(2),F(2),F(1)) and acute(flat),'source flat acute center exact')
    need(owned(cf,boxf) and meets(median(flat),boxf),'included lower face remains admissible')
    need(max(p[0] for p in median(flat))==2*boxf[0][0],'lower-face mutant causal equality')
    need(not all(max(p[d] for p in median(flat))>2*boxf[0][d] and min(p[d] for p in median(flat))<2*boxf[1][d] for d in range(3)),'mutant high<=2lo would lose admitted q3')
    need(not meets(median(flat),((3,2,1),(4,3,2))),'M3 nonvacuous rejection beside planar triangle')
    need(not owned(cf,((1,2,1),(2,3,2))) and not meets(median(flat),((1,2,1),(2,3,2))),'upper face correctly open')
    ct,wt=gram(tall);boxt=((15,8,10),(16,9,11))
    need(ct==(F(15),F(8),F(611,60)) and all(x>0 for x in wt),'source tall strict center exact')
    need(owned(ct,boxt) and meets(vertices(tall),boxt),'four-vertex E4 keeps positive candidate')
    need(not meets(vertices(tall[:3]),boxt),'three-vertex E4 mutant causal')
    cx,wx=gram(flat_tetra);boxx=((25,25,11),(26,26,12))
    need(cx==(F(25),F(25),F(23,2)) and owned(cx,boxx),'source flat nondegenerate center exact')
    need(not all(x>0 for x in wx) and not meets(vertices(flat_tetra),boxx),'E4 safe for owned but non-strict center')
    u,v,w=(sub(flat_tetra[i],flat_tetra[0]) for i in (1,2,3))
    oldcount=int(gram(flat_tetra) is not None)
    newcount=int(dot(cross(u,v),w)!=0)
    latecount=newcount*int(meets(vertices(flat_tetra),boxx))
    need(oldcount==newcount==1 and latecount==0,'E4-before-count mutant would lose a nondegenerate q4 count')
    co,wo=gram(obtuse_prefix)
    need(all(x>0 for x in wo) and owned(co,((4,4,4),(5,5,5))),'source q4 behind obtuse prefixes positive and owned')
    need(any(not acute(list(p)[:3]) for p in permutations(obtuse_prefix)),'acute test must not prune q4 extension')
    profiles=[]
    for bits in (18,21,24):
        M=1<<bits;scale=(M-1)//30
        for p in (flat,tall,flat_tetra):
            pts=[tuple(scale*x for x in v) for v in p]
            solution=gram(pts);r=test_tuple(pts,boxes_for(solution[0]))
            totals['tuples']+=1
            for key in r:totals[key]+=r[key]
        need(2*M < 1<<63,'doubled endpoints fit i64')
        need(6*M**3 < 1<<127,'orientation bound fits i128')
        profiles.append({'bits':bits,'doubled_endpoint_bits':(2*M).bit_length(),'triple_bound_bits':(6*M**3).bit_length()})
    P=sum(comb(1024,q) for q in range(1,5))
    need(P<1<<37 and 1024*3*P<1<<49,'new q4 count within existing leaf count bound')
    print(json.dumps({'status':'ok','checks':CHECKS,'cases':totals,'source_fixtures':{'flat_center':list(map(str,cf)),'tall_center':list(map(str,ct)),'flat_tetra_center':list(map(str,cx)),'flat_tetra_weights':list(map(str,wx)),'obtuse_prefix_center':list(map(str,co))},'profiles':profiles,'native':False,'chrono_claimed':False},sort_keys=True,indent=2))

if __name__=='__main__':main()
