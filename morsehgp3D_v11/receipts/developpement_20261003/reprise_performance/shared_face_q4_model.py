from fractions import Fraction as F
from itertools import combinations
from collections import Counter
import json


def need(x,m):
    if not x:raise ValueError(m)


def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def d2(a,b):return dot(sub(a,b),sub(a,b))


def gauss(a,b):
    n=len(b);r=[[F(x) for x in a[i]]+[F(b[i])] for i in range(n)]
    for j in range(n):
        k=next((i for i in range(j,n) if r[i][j]),None)
        if k is None:return None
        r[j],r[k]=r[k],r[j];v=r[j][j];r[j]=[x/v for x in r[j]]
        for i in range(n):
            if i!=j:
                v=r[i][j];r[i]=[x-v*y for x,y in zip(r[i],r[j])]
    return tuple(x[-1] for x in r)


def circum(P):
    a=P[0];d=[sub(p,a) for p in P[1:]]
    w=gauss([[dot(x,y) for y in d] for x in d],[F(dot(x,x),2) for x in d])
    if w is None:return None
    c=tuple(F(a[j])+sum(t*x[j] for t,x in zip(w,d)) for j in range(3))
    return c,d2(c,a),(1-sum(w),)+w


def morton(p):return sum(((p[j]>>i)&1)<<(3*i+j) for i in range(8) for j in range(3))


def one(points,face):
    count=Counter();tested=[]
    pairs=tuple(combinations(face,2));count['diameter_distances']=len(pairs)
    diameter=max(d2(points[a],points[b]) for a,b in pairs)
    pair=next(s for s in pairs if d2(points[s[0]],points[s[1]])==diameter)
    for q in (2,3,4):
        for s in ((pair,) if q==2 else combinations(face,q)):
            count['presentations']+=1;tested.append(s)
            b=circum(tuple(points[i] for i in s))
            if b is None:continue
            count['q4_nondegenerate_centres']+=q==4
            if not all(v>0 for v in b[2]):continue
            count['q3_strict_centres']+=q==3
            count['strict_spheres']+=1
            count['centre_supports_q3_q4']+=q>=3
            for i in face:
                count['power_tests']+=1
                if d2(points[i],b[0])>b[1]:break
            else:return s,b,count,tested
    raise ValueError('no containing strict support')


def joint(points,faces,U):
    unresolved=set(U);count=Counter();result={}
    count['diameter_distances']=len(tuple(combinations(range(len(points)),2)))
    # Per-face canonical diameter is still evaluated first; joint distances are shared.
    for u in U:
        face=faces[u];pairs=tuple(combinations(face,2))
        diameter=max(d2(points[a],points[b]) for a,b in pairs)
        s=next(s for s in pairs if d2(points[s[0]],points[s[1]])==diameter)
        b=circum(tuple(points[i] for i in s));count['q2_presentations']+=1
        for i in face:
            count['power_tests']+=1
            if d2(points[i],b[0])>b[1]:break
        else:result[u]=(s,b);unresolved.remove(u)
    for q in (3,4):
        for s in combinations(range(len(points)),q):
            eligible=unresolved-set(s)
            if not eligible:continue
            count['q3_q4_presentations']+=1
            b=circum(tuple(points[i] for i in s))
            if b is None:continue
            count['q4_nondegenerate_centres']+=q==4
            if not all(v>0 for v in b[2]):continue
            count['q3_strict_centres']+=q==3
            count['centre_supports_q3_q4']+=1
            possible=set(eligible)
            for i in range(len(points)):
                count['power_tests']+=1
                if d2(points[i],b[0])>b[1]:possible.intersection_update({i})
                if not possible:break
            for u in possible:
                result[u]=(s,b);unresolved.remove(u)
            if not unresolved:return result,count
    need(not unresolved,'joint failed')
    return result,count


Uraw=((0,0,0),(8,8,0),(8,0,8),(0,8,8));Iraw=((1,1,1),(7,7,1))
points=tuple(sorted(Uraw+Iraw,key=morton));U=tuple(i for i,p in enumerate(points) if p in Uraw)
I=tuple(i for i,p in enumerate(points) if p in Iraw)
need(all(d2(p,(4,4,4))==48 for p in Uraw),'shell')
need(all(d2(p,(4,4,4))<48 for p in Iraw),'inner')
need(circum(Uraw)[2]==(F(1,4),)*4,'regular critical support')
faces={u:tuple(i for i in range(len(points)) if i!=u) for u in U}
old={u:one(points,face) for u,face in faces.items()}
new,counts=joint(points,faces,U)
need(all(new[u][:2]==old[u][:2] for u in U),'canonical face MEB changed')
need(all(new[u][1][1]<48 for u in U),'face not strict')
old_counts=Counter()
for v in old.values():old_counts.update(v[2])
need(counts['centre_supports_q3_q4']<=old_counts['centre_supports_q3_q4'],'no geometric sharing')
print(json.dumps({'scope':'standalone exact scalar model; no product/reference imports or native timing',
 'points_morton':points,'I':I,'U':U,'critical_beta':'48','order':5,
 'faces':[{'omitted':u,'support':new[u][0],'beta':str(new[u][1][1]),'centre':[str(x) for x in new[u][1][0]]} for u in U],
 'independent_counts':dict(old_counts),'shared_counts':dict(counts),
 'limitations':'candidate model omits factory bit work and q3 classification costs; no measured speed gain'},sort_keys=True))
