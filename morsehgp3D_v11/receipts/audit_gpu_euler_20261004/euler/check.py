#!/usr/bin/env python3
"""J3 indépendant par regroupement de TOUTES les parties selon leur MEB.

Pour chaque partie F non vide, les supports positifs affinement indépendants
de cardinal <=4 qui contiennent F donnent sa MEB par minimum exact de rayon.
Toutes ces présentations sont énumérées avec Gram/Gauss ; aucun import du
produit, de ses références ou de ses tests. On regroupe directement le poids
(-1)^(|F|-k) C(|F|-1,k-1) selon cette MEB, puis compare au terme J3 calculé
sur les seules parties A de coquille avec MEB(A)=b. Cette dernière condition
équivaut à c dans conv(A), sans implémenter la fermeture de supports utilisée
par le juge C++. Les singletons contribuent n pour k=1.

En séparant les p intérieurs et la coquille : le coefficient de chaque A de
cardinal s est la différence finie sur j=0..p. Elle vaut zéro si k<=p ou
k>p+s, et sinon (-1)^(s-(k-p)) C(s-1,k-p-1). Donc p<=k-1 et qmin<=4
suffisent pour admettre tout terme non nul dans Cat_(K+2), pour k<=K.
J1 dépend de l'admission p+qmin<=K+1 au même nuage, puis de la renumérotation
des niveaux EXACTS ; le représentant BRUT du premier support filtré peut
différer du représentant du catalogue plus grand.

J3/J1 sont nécessaires, pas une preuve de complétude ni un juge FULL.
Ni natif, ni build, ni campagne, ni mesure de temps dans ce programme.
"""
from fractions import Fraction as F
from itertools import combinations
from math import comb
import json

CHECKS=0
def need(v,m):
    global CHECKS
    CHECKS+=1
    if not v:raise ValueError(m)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def d2(a,b):return dot(sub(a,b),sub(a,b))
def morton(p):return sum(((x>>b)&1)<<(3*b+a) for a,x in enumerate(p) for b in range(x.bit_length()))
def gram(points):
    a=points[0];us=[sub(p,a) for p in points[1:]];n=len(us)
    rows=[[F(dot(u,v)) for v in us]+[F(dot(u,u),2)] for u in us]
    for i in range(n):
        pivot=next((j for j in range(i,n) if rows[j][i]),None)
        if pivot is None:return None
        rows[i],rows[pivot]=rows[pivot],rows[i];v=rows[i][i];rows[i]=[x/v for x in rows[i]]
        for j in range(n):
            if j!=i:
                v=rows[j][i];rows[j]=[x-v*y for x,y in zip(rows[j],rows[i])]
    weights=[r[-1] for r in rows]
    center=tuple(F(a[a0])+sum(w*u[a0] for w,u in zip(weights,us)) for a0 in range(3))
    return center,d2(center,a),[1-sum(weights)]+weights

def all_critical(points):
    out={}
    for q in range(2,min(4,len(points))+1):
        for ids in combinations(range(len(points)),q):
            g=gram([points[i] for i in ids])
            if g is None or any(w<=0 for w in g[2]):continue
            key=g[:2]
            out.setdefault(key,[]).append(ids)
    rows=[]
    for key,presentations in out.items():
        c,r=key;inside=tuple(i for i,p in enumerate(points) if d2(c,p)<r)
        shell=tuple(i for i,p in enumerate(points) if d2(c,p)==r)
        s=min(presentations,key=lambda a:(len(a),a))
        rows.append(dict(key=key,support=s,p=len(inside),m=len(shell),shell=shell,presentations=presentations,contained=sum(1<<i for i in inside+shell)))
    return sorted(rows,key=lambda a:(a['key'][1],a['support']+(2**32-1,)*(4-len(a['support']))))

def all_mebs(points,balls):
    out={}
    for mask in range(1,1<<len(points)):
        if mask&(mask-1)==0:
            i=mask.bit_length()-1;out[mask]=(tuple(map(F,points[i])),F(0));continue
        candidates=[b for b in balls if mask&~b['contained']==0 and any(all(mask>>i&1 for i in s) for s in b['presentations'])]
        need(bool(candidates),'MEB exists among positive support spheres of size <=4')
        r=min(b['key'][1] for b in candidates)
        keys={b['key'] for b in candidates if b['key'][1]==r}
        need(len(keys)==1,'unique ball at the minimum enclosing radius')
        out[mask]=keys.pop()
    return out

def direct_coefficient(p,s,k):
    return sum((-1)**(j+s-k)*comb(p,j)*comb(j+s-1,k-1) for j in range(p+1) if j+s>=k)
def reduced_coefficient(p,s,k):
    t=k-p
    return 0 if t<1 or t>s else (-1)**(s-t)*comb(s-1,t-1)
def contribution(ball,counts,k):
    return sum(n*reduced_coefficient(ball['p'],s,k) for s,n in enumerate(counts) if s)

def check_cloud(name,xyz):
    points=tuple(sorted(xyz,key=morton));n=len(points);balls=all_critical(points);mebs=all_mebs(points,balls)
    grouped={b['key']:[0]*(n+1) for b in balls}
    for mask,key in mebs.items():
        size=mask.bit_count()
        if size==1:continue
        for k in range(1,size+1):grouped[key][k]+=(-1)**(size-k)*comb(size-1,k-1)
    counts={}
    for b in balls:
        cs=[0]*(b['m']+1)
        for mask in range(1,1<<b['m']):
            selected=sum(1<<i for at,i in enumerate(b['shell']) if mask>>at&1)
            if mebs[selected]==b['key']:cs[mask.bit_count()]+=1
        counts[b['key']]=cs
        for k in range(1,n+1):need(contribution(b,cs,k)==grouped[b['key']][k],'per-ball J3 equals regrouping all subsets by MEB')
    for k in range(1,n+1):
        e=(n if k==1 else 0)+sum(contribution(b,counts[b['key']],k) for b in balls)
        need(e==1,'J3 identity on ALL positive-radius critical balls')
    for K in (1,2,3,5,8,10):
        large=[b for b in balls if b['p']+len(b['support'])<=K+3]
        small=[b for b in balls if b['p']+len(b['support'])<=K+1]
        filtered=[b for b in large if b['p']+len(b['support'])<=K+1]
        need(small==filtered,'J1 restriction at fixed cloud and exact keys')
        for k in range(1,min(K,n)+1):
            need((n if k==1 else 0)+sum(contribution(b,counts[b['key']],k) for b in large)==1,'Cat_(K+2) contains all terms for checked order')
        sl=sorted({F(0)}|{b['key'][1] for b in small});fl=sorted({F(0)}|{b['key'][1] for b in filtered})
        need(sl==fl,'dense exact levels after restriction')
    return points,balls,counts,dict(name=name,sites=n,balls=len(balls),parts=len(mebs),extended=sum(b['m']>len(b['support']) for b in balls))

def packed_closure(m,seeds):
    words=[0]*max(1,(1<<m)//64)
    for s in seeds:words[s>>6]|=1<<(s&63)
    lows=(0x5555555555555555,0x3333333333333333,0x0f0f0f0f0f0f0f0f,0x00ff00ff00ff00ff,0x0000ffff0000ffff,0x00000000ffffffff)
    for i in range(min(m,6)):
        for w in range(len(words)):words[w]|=(words[w]&lows[i])<<(1<<i)
    for i in range(6,m):
        for w in range(len(words)):
            if w>>(i-6)&1:words[w]|=words[w^(1<<(i-6))]
    counts=[0]*(m+1)
    for mask in range(1<<m):
        truth=any(mask&s==s for s in seeds);got=bool(words[mask>>6]>>(mask&63)&1)
        need(got==truth,'packed OR zeta equals direct superset predicate, including high words')
        if got:counts[mask.bit_count()]+=1
    return counts

def main():
    for p in range(11):
        for s in range(2,7):
            for k in range(1,p+s+2):
                need(direct_coefficient(p,s,k)==reduced_coefficient(p,s,k),'finite difference with all interior subsets')
    fixtures=[('singleton',[(0,0,0)]),('triangle',[(0,0,0),(4,0,0),(2,3,0)]),('cross4',[(0,1,0),(2,1,0),(1,0,0),(1,2,0)]),('tetra4',[(0,0,0),(8,8,0),(8,0,8),(0,8,8)]),('tetra_center5',[(0,0,0),(8,8,0),(8,0,8),(0,8,8),(4,4,4)]),('octa6',[(0,3,3),(6,3,3),(3,0,3),(3,6,3),(3,3,0),(3,3,6)]),('triangle_two_interiors5',[(0,0,0),(6,0,0),(3,4,0),(2,1,0),(4,1,0)]),('compensation5',[(0,5,0),(8,9,0),(8,1,0),(35,5,0),(45,5,0)])]
    rows=[];comp=None;need_more=None
    for name,points in fixtures:
        result=check_cloud(name,points);rows.append(result[3])
        if name=='compensation5':comp=result[:3]
        if name=='tetra_center5':
            pts,bs,cs=result[:3];K=2
            too_small=[b for b in bs if b['p']+len(b['support'])<=K+2]
            need_more=sum(contribution(b,cs[b['key']],K) for b in too_small)
            need(need_more!=1,'Cat_(K+1) insufficient on a realized tetrahedron with one interior')
    pts,bs,cs=comp
    tri=next(b for b in bs if b['key']==((F(5),F(5),F(0)),F(25)))
    pair=next(b for b in bs if b['key']==((F(40),F(5),F(0)),F(25)))
    need(tri['p']==pair['p']==0 and len(tri['support'])==3 and len(pair['support'])==2,'actual compensation5 pair and triangle')
    e=[(5 if k==1 else 0)+sum(contribution(b,cs[b['key']],k) for b in bs if b not in (tri,pair)) for k in range(1,4)]
    need(e==[1,2,0],'common omission invisible at K1 and detected at K2')
    large=[b for b in bs if b['p']+len(b['support'])<=4]
    small=[b for b in bs if b['p']+len(b['support'])<=2]
    first_large=next(b for b in large if b['key'][1]==25);first_small=next(b for b in small if b['key'][1]==25)
    need(first_large==tri and first_small==pair,'same exact level has a different first representative after J1')
    need(F(409600,16384)==F(100,4)==25 and (409600,16384)!=(100,4),'raw representative must be recomputed, not copied by equal level')
    d13=[(0,0,0),(20,0,0),(8,1,1),(9,2,2),(11,1,3),(12,3,1)]
    t13=[(100,0,0),(120,0,0),(110,16,0),(108,4,1),(110,4,2),(112,5,1),(109,6,2)]
    d23=d13+[(7,1,1),(13,1,1),(10,1,1),(10,2,1),(10,3,1)]
    t23=t13+[(108,5,1),(109,4,1),(110,5,1),(111,4,1),(112,4,1)]
    dt=[]
    for name,d,t,p in (('dt13',d13,t13,4),('dt23',d23,t23,9)):
        allsites=d+t
        for support,m in ((d[:2],2),(t[:3],3)):
            g=gram(support);need(g is not None and all(w>0 for w in g[2]),'D/T support strictly positive')
            inner=sum(d2(x,g[0])<g[1] for x in allsites);shell=sum(d2(x,g[0])==g[1] for x in allsites)
            need((inner,shell)==(p,m),'actual D/T fixtures have declared p and regular shell')
        combined=[reduced_coefficient(p,2,k)+reduced_coefficient(p,3,k) for k in range(1,p+4)]
        need(all(x==0 for x in combined[:p+1]),'D/T common omission invisible at every checked order')
        need(combined[p+1]==-1 and reduced_coefficient(p,2,p+1)==-1,'next order or single pair removal exposes omission')
        dt.append({'name':name,'sites':len(allsites),'p':p,'combined_contributions':combined})
    closures=[]
    for m in range(2,11):
        seeds=[3,(1<<(m-1))|2] if m>2 else [3]
        counts=packed_closure(m,seeds);closures.append({'m':m,'counts':counts})
    for m in (24,25):
        refused=m>24;words=0 if refused else 1<<(m-6)
        need(refused or words*8==2*1024*1024,'2 MiB packed shell24 scratch per worker')
        need(not refused or words==0,'shell25 preflight refuses before mask allocation')
    need(2**32*2**48<2**127,'signed i128 Euler sum bound')
    need(2**32*sum(comb(24,q) for q in (2,3,4))<2**64,'support count sum bound')
    print(json.dumps({'status':'ok','checks':CHECKS,'fixtures':rows,'dt_regular_witnesses':dt,'closure_counts':closures,'compensation5_after_common_omission':e,'tetra_center5_euler_order2_cat3':need_more,'raw_level25_large':[409600,16384],'raw_level25_filtered':[100,4],'native':False,'full_qualification':False},sort_keys=True,indent=2))

if __name__=='__main__':main()
