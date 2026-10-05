"""Contre-garde exacte de la preuve D2 du futur raccord supports -> forêt K.

Profil : cinq sites unitaires distincts, entiers u21. K=2.
A=(2,10,0), B=(18,10,0), C=(10,20,0), Z=(9,3,0), W=(11,3,0).
La boule b=MEB(ABC) a c=(10,59/5,0), β=1681/25,p=0,U=ABC,qmin=3.
Sa trace F=AB est stricte, β(F)=64<β(b). Mais MEB(F) contient strictement
Z,W : p=2,qmin=2, donc p+qmin=4>K+1=3. Elle n'est PAS dans Cat2.
Le plus grand niveau positif de Cat2 avant b vaut41. Ainsi la phrase
β(F) <= niveau(r_b-1) de SPECIFICATION_FINALE lemmeD2 est fausse.

L'algorithme proposé reste valide : descendre AB vers ZW donne une graine
née à1, dans la même composante à64 (T5). Le nœud de cette classe est aussi
celui de coupe ouverte juste avant b, et se représente à41 : aucun événement
de naissance/fusion FULL n'intervient entre deux niveaux consécutifs Cat2.
Il faut parler de continuation de cette CLASSE ; F lui-même n'existe pas
encore dans Γ2(41). Le balayage interroge g=ZW, pas F=AB.
La seule garde nécessaire du trace initial est β(F)<β_b, pas β(F)<=41.

Solveur Gram/Fraction écrit ici, aucun import produit/oracle/natif. On
énumère tous les supports q2..4 du nuage pour Cat2 (25 présentations),
puis toutes ses dix paires et dix triples pour Γ2 (20 MEB bornées).
Ce programme ne qualifie pas l'API S3 future, qui n'est pas implantée au pin.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
from math import comb
import hashlib
import json

CHECKS=0
MEB_CALLS=0
SUPPORT_PRESENTATIONS=0

def require(test,message):
    global CHECKS
    CHECKS+=1
    if not test:
        raise ValueError(message)

def sub(a,b):
    return tuple(x-y for x,y in zip(a,b))

def dot(a,b):
    return sum((x*y for x,y in zip(a,b)),F(0))

def solve(matrix,rhs):
    rows=[list(row)+[value] for row,value in zip(matrix,rhs)]
    n=len(rows)
    for j in range(n):
        p=next((i for i in range(j,n) if rows[i][j]),None)
        if p is None:
            return None
        rows[j],rows[p]=rows[p],rows[j]
        divisor=rows[j][j]
        rows[j]=[x/divisor for x in rows[j]]
        for i in range(n):
            if i!=j:
                v=rows[i][j]
                rows[i]=[x-v*y for x,y in zip(rows[i],rows[j])]
    return tuple(row[-1] for row in rows)

def sphere(ids,cloud):
    points=[cloud[i] for i in ids]
    a=points[0]
    vectors=[sub(p,a) for p in points[1:]]
    weights=solve([[dot(v,w) for w in vectors] for v in vectors],
                  [dot(v,v)/2 for v in vectors])
    if weights is None:
        return None
    center=tuple(a[j]+sum((w*v[j] for w,v in zip(weights,vectors)),F(0)) for j in range(3))
    bary=(1-sum(weights,F(0)),)+weights
    radius=dot(sub(a,center),sub(a,center))
    return center,radius,bary

def meb(ids,cloud):
    global MEB_CALLS
    MEB_CALLS+=1
    winners=[]
    for q in range(1,min(4,len(ids))+1):
        for support in combinations(ids,q):
            s=sphere(support,cloud)
            if s is None or not all(w>0 for w in s[2]):
                continue
            center,radius,bary=s
            if all(dot(sub(cloud[i],center),sub(cloud[i],center))<=radius for i in ids):
                winners.append((radius,center))
    require(bool(winners),'bounded MEB has a strict containing support')
    level=min(r for r,c in winners)
    require(len({c for r,c in winners if r==level})==1,'minimum enclosing ball is unique')
    return level

def census(center,radius,cloud):
    signs=[dot(sub(p,center),sub(p,center))-radius for p in cloud]
    return tuple(i for i,v in enumerate(signs) if v<0),tuple(i for i,v in enumerate(signs) if v==0)

def catalogue(cloud,k):
    global SUPPORT_PRESENTATIONS
    balls={}
    for q in range(2,5):
        for support in combinations(range(len(cloud)),q):
            SUPPORT_PRESENTATIONS+=1
            s=sphere(support,cloud)
            if s is None or not all(w>0 for w in s[2]):
                continue
            center,radius,_=s
            interior,shell=census(center,radius,cloud)
            entry=balls.setdefault((center,radius),{'qmin':q,'supports':[],'I':interior,'U':shell})
            entry['qmin']=min(q,entry['qmin'])
            entry['supports'].append(support)
    cat=[]
    for (center,level),b in balls.items():
        if len(b['I'])+b['qmin']<=k+1:
            cat.append({'center':center,'level':level,**b})
    return sorted(cat,key=lambda b:(b['level'],b['supports']))

def gamma(pairs,triples,levels,cut):
    vertices=[v for v in pairs if levels[v]<=cut]
    index={v:i for i,v in enumerate(vertices)}
    parent=list(range(len(vertices)))
    def root(x):
        while parent[x]!=x:
            x=parent[x]
        return x
    for coface in triples:
        if levels[coface]>cut:
            continue
        faces=list(combinations(coface,2))
        require(all(v in index for v in faces),'active coface has all active pair faces')
        first=root(index[faces[0]])
        for face in faces[1:]:
            parent[root(index[face])]=first
    groups={}
    for v,i in index.items():
        groups.setdefault(root(i),set()).add(v)
    return tuple(sorted(tuple(sorted(g)) for g in groups.values()))

def owner(groups,vertex):
    found=[g for g in groups if vertex in g]
    require(len(found)==1,'one owner of active vertex')
    return found[0]

def main():
    cloud=tuple(tuple(F(v) for v in p) for p in ((2,10,0),(18,10,0),(10,20,0),(9,3,0),(11,3,0)))
    require(len(set(cloud))==5,'five distinct sites')
    require(all(v.denominator==1 and 0<=v<2**21 for p in cloud for v in p),'all sites in u21 input domain')
    cat=catalogue(cloud,2)
    require(SUPPORT_PRESENTATIONS==sum(comb(5,q) for q in range(2,5))==25,'full q2/q3/q4 support enumeration')
    cut=F(1681,25)
    ball=[b for b in cat if b['center']==(F(10),F(59,5),F(0)) and b['level']==cut]
    require(len(ball)==1,'target ball in Cat2')
    b=ball[0]
    require(b['I']==() and b['U']==(0,1,2) and b['qmin']==3,'target p0m3q3 complete census')
    require(0+3-1<=2<=0+3,'target is in event window W2')
    previous=max(entry['level'] for entry in cat if entry['level']<cut)
    require(previous==41,'last preceding Cat2 level is exactly41')
    center,trace_level,weights=sphere((0,1),cloud)
    interior,shell=census(center,trace_level,cloud)
    require(trace_level==64 and interior==(3,4) and shell==(0,1),'trace MEB contains two global strict interiors')
    require(len(interior)+2==4>3 and not any(entry['center']==center and entry['level']==trace_level for entry in cat),'trace MEB excluded from Cat2')
    require(previous<trace_level<cut,'false D2 inequality rejected while trace remains strict')
    pairs=tuple(combinations(range(5),2));triples=tuple(combinations(range(5),3))
    levels={v:meb(v,cloud) for v in pairs+triples}
    require(MEB_CALLS==20,'ten pairs and ten triples')
    require(levels[(0,1)]==64 and levels[(3,4)]==1,'initial trace and terminal seed levels')
    before=gamma(pairs,triples,levels,previous)
    initial=gamma(pairs,triples,levels,trace_level)
    near=gamma(pairs,triples,levels,F(67))
    closed=gamma(pairs,triples,levels,cut)
    require(len(before)==len(initial)==len(near)==3,'no birth/fusion of components between previous and target')
    require((0,1) not in {v for g in before for v in g},'trace itself absent at preceding catalogue cut')
    seed=(3,4)
    old_owner=owner(before,seed)
    initial_owner=owner(initial,(0,1))
    require(seed in initial_owner,'descent seed ZW and AB have same component at initial64')
    require(set(old_owner)==set(initial_owner)-{(0,1)},'class continues; only an inessential vertex is added')
    require(owner(near,seed)==initial_owner,'owner unchanged until just before target')
    require(len(closed)==1 and len(closed[0])==8,'one closed fusion of three old components')
    require(all(v in closed[0] for v in ((0,1),(0,2),(1,2))), 'all target traces have same final closed owner')
    report={'status':'PASS','checks':CHECKS,'scope':'exact_Fraction_model_no_native_no_S3_implementation',
            'cloud':cloud,'k':2,'support_presentations':SUPPORT_PRESENTATIONS,'meb_calls':MEB_CALLS,
            'target_level':cut,'previous_catalogue_level':previous,'initial_trace_level':trace_level,
            'terminal_seed_level':1,'target_census':{'p':0,'m':3,'qmin':3},
            'trace_meb_census':{'p':2,'m':2,'qmin':2,'admitted':False},
            'cat2':cat,'components_previous':before,'components_initial':initial,
            'components_near':near,'components_closed':closed,
            'false_inequality_initial_le_previous_rejected':True,
            'proposed_attachment_algorithm_refuted':False,
            'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    print(json.dumps(encode(report),sort_keys=True,ensure_ascii=False,indent=2))

def encode(value):
    if isinstance(value,F):
        return str(value.numerator)+'/'+str(value.denominator)
    if isinstance(value,dict):
        return {k:encode(v) for k,v in value.items()}
    if isinstance(value,(tuple,list)):
        return [encode(v) for v in value]
    return value

if __name__=='__main__':
    main()
