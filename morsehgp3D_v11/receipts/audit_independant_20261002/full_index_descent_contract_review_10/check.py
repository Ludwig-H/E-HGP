#!/usr/bin/env python3
"""Deux portes de raccord FULL, oracle Gram/Gauss/Fraction sans imports produit."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import combinations
from math import comb
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent
checks=0


def require(condition, why):
    global checks
    if not condition:
        raise RuntimeError(why)
    checks+=1


def dot(a,b):
    return sum(x*y for x,y in zip(a,b))


def sub(a,b):
    return tuple(x-y for x,y in zip(a,b))


def solve(matrix,rhs):
    n=len(rhs)
    rows=[[F(x) for x in row]+[F(rhs[i])] for i,row in enumerate(matrix)]
    for col in range(n):
        pivot=next((i for i in range(col,n) if rows[i][col]),None)
        if pivot is None:
            return None
        rows[col],rows[pivot]=rows[pivot],rows[col]
        divisor=rows[col][col]
        rows[col]=[x/divisor for x in rows[col]]
        for i in range(n):
            if i!=col:
                factor=rows[i][col]
                rows[i]=[x-factor*y for x,y in zip(rows[i],rows[col])]
    return tuple(row[-1] for row in rows)


def circumsphere(points):
    a=points[0]
    vectors=tuple(sub(x,a) for x in points[1:])
    gram=[[dot(u,v) for v in vectors] for u in vectors]
    weights=solve(gram,[F(dot(u,u),2) for u in vectors])
    if weights is None:
        return None
    center=tuple(F(a[j])+sum(w*u[j] for w,u in zip(weights,vectors)) for j in range(3))
    return center,dot(sub(a,center),sub(a,center)),(1-sum(weights),)+weights


class Geometry:
    def __init__(self,points):
        self.points=tuple(points)
        self.n=len(points)
        self.balls={}
        for size in range(1,self.n+1):
            for part in combinations(range(self.n),size):
                candidates=[]
                for q in range(1,min(4,size)+1):
                    for support in combinations(part,q):
                        sphere=circumsphere([points[i] for i in support])
                        if sphere is None:
                            continue
                        center,radius2,weights=sphere
                        if all(w>0 for w in weights) and all(dot(sub(points[i],center),sub(points[i],center))<=radius2 for i in part):
                            candidates.append((radius2,q,center,support))
                require(bool(candidates),'MEB exacte existe')
                radius2,q,center,support=min(candidates)
                self.balls[part]=(center,radius2,q,support)

    def census(self,ball):
        center,radius2,_,_=ball
        interior=tuple(i for i,x in enumerate(self.points) if dot(sub(x,center),sub(x,center))<radius2)
        shell=tuple(i for i,x in enumerate(self.points) if dot(sub(x,center),sub(x,center))==radius2)
        return interior,shell

    def gamma(self,k,level,opened=False):
        compare=(lambda b:b<level) if opened else (lambda b:b<=level)
        vertices=tuple(part for part in combinations(range(self.n),k) if compare(self.balls[part][1]))
        root={part:part for part in vertices}

        def find(part):
            while root[part]!=part:
                root[part]=root[root[part]]
                part=root[part]
            return part

        for coface in combinations(range(self.n),k+1):
            if not compare(self.balls[coface][1]):
                continue
            faces=tuple(combinations(coface,k))
            for face in faces[1:]:
                a,b=find(faces[0]),find(face)
                root[max(a,b)]=min(a,b)
        groups={}
        for part in vertices:
            groups.setdefault(find(part),set()).add(part)
        return tuple(sorted((frozenset(group) for group in groups.values()),key=lambda group:min(group)))

    def component(self,k,part,level,opened=False):
        groups=self.gamma(k,level,opened)
        return next(group for group in groups if tuple(part) in group)


# Porte 1 : boule de descente hors catalogue K2, terminal à relever à la date.
line=Geometry(((0,0,0),(4,0,0),(5,0,0),(11,0,0)))
origin=(0,3)
ball=line.balls[origin]
interior,shell=line.census(ball)
require(ball[1]==F(121,4) and interior==(1,2) and shell==(0,3),'census extrême')
require(len(interior)>=2 and len(interior)+ball[2]>3,'saturé K2 et hors admission Cat2')
next_part=interior[:2]
terminal=line.balls[next_part]
require(terminal[1]==F(1,4)<ball[1],'descente stricte sans K plus proches requis')
require(line.census(terminal)==((),(1,2)),'naissance : coque ne devient pas intérieur')
require(line.balls[tuple(sorted(line.census(terminal)[1]))][1]==terminal[1],
        'faux saut par population fermée ne diminue pas beta')
at_birth=line.component(2,next_part,terminal[1])
at_query=line.component(2,next_part,ball[1])
require(len(at_birth)==1 and len(at_query)==6 and origin in at_query,'terminal != composante actuelle')
require(len(line.gamma(2,F(25,4),True))==2 and len(line.gamma(2,F(25,4)))==1,'première fusion ouverte/fermée')
require(len(line.gamma(2,F(49,4),True))==2 and len(line.gamma(2,F(49,4)))==1,'fusion racine ouverte/fermée')
at_nine=line.gamma(2,F(9))
require(len(at_nine)==2,'deux composantes à beta9')
populations=[frozenset(i for part in group for i in part) for group in at_nine]
require(set(populations)=={frozenset((0,1,2)),frozenset((2,3))},'populations distinctes partageant un site')
require(populations[0]&populations[1]=={2},'point commun ne prouve pas fusion')
require(len(interior)<3 and len(interior)+ball[2]<=4,'même boule complete et admise Cat3')
require(len(line.gamma(3,F(121,4),True))==2 and len(line.gamma(3,F(121,4)))==1,'jonction K3 même boule')

# Porte 2 : carré+centre, cofaces coplanaires et multifusion forte qmin2/K3.
square=Geometry(((0,0,0),(4,0,0),(4,4,0),(0,4,0),(2,2,0)))
full=square.balls[tuple(range(5))]
inner,boundary=square.census(full)
require(full[:3]==((F(2),F(2),F(0)),F(8),2),'boule globale critique')
require(inner==(4,) and boundary==(0,1,2,3),'p1,m4,coquille complète')
require(len(inner)<3<len(boundary),'census K3 complet, U>K')
require(len(inner)+full[2]<=3<=len(inner)+len(boundary),'cellule forte K3')
births=square.gamma(3,F(4))
opened=square.gamma(3,F(8),True)
closed=square.gamma(3,F(8))
expected_births={frozenset((part,)) for part in ((0,1,4),(1,2,4),(2,3,4),(0,3,4))}
require(set(births)==expected_births and set(opened)==expected_births,'quatre naissances beta4 avant8')
require(len(closed)==1 and len(closed[0])==10,'plateau beta8 joint quatre parents et dix sommets')
require(full[2]==2 and len(opened)==4,'qmin ne borne pas arité de fusion')
require(all(4 in next(iter(group)) for group in births),'point frontière partagé ne fusionne pas les naissances')
require(len({i for i in full[3]})==2 and 4 not in full[3],'S* ne contient pas toute population/incidence')
cofaces=tuple(combinations(range(5),4))
for part in cofaces:
    require(circumsphere([square.points[i] for i in part]) is None,'q4 dépendant')
    require(square.balls[part][1]==8 and square.balls[part][2]==2,'MEB affine valide via diagonale')
lower_open=square.gamma(2,F(4),True)
lower_closed=square.gamma(2,F(4))
require(len(lower_open)==4 and len(lower_closed)==1,'plateau inférieur complet avant verticales')
for group in births:
    part=next(iter(group))
    images={square.component(2,face,F(4)) for face in combinations(part,2)}
    require(len(images)==1 and next(iter(images))==lower_closed[0],'toutes faces même image verticale fermée')

# Supplément publié pendant la revue : présentation minimisante non stricte,
# support MEB local strict et support canonique global distincts.
local_points=((1,2,0),(0,5,0),(8,1,0),(8,9,0))
local=Geometry(local_points)
local_ball=local.balls[(0,1,2,3)]
first=circumsphere(local_points[:3])
strict=circumsphere([local_points[i] for i in (0,2,3)])
require(first[0:2]==strict[0:2]==local_ball[0:2]==((F(5),F(5),F(0)),F(25)),
        'première présentation et vraie MEB même géométrie')
require(first[2]==(F(-1),F(5,4),F(3,4)),'poids première présentation non stricts')
require(strict[2]==(F(3,7),F(1,8),F(25,56)) and all(w>0 for w in strict[2]),
        'support strict local après égalité de niveau')
require(local_ball[2:]==(3,(0,2,3)),'qmin local3, support lex strict023')
global_geometry=Geometry(local_points+((9,8,0),))
global_ball=global_geometry.balls[tuple(range(5))]
require(global_ball[0:2]==local_ball[0:2] and global_ball[2:]==(2,(0,4)),
        'identité inchangée, qmin global2 après paire antipodale')
require(global_geometry.census(global_ball)==((),tuple(range(5))),'coquille globale complète distincte de F')
require(sum(comb(12,q) for q in range(1,5))==793,'coût local annoncé793 présentations')

before=json.loads((HERE/'SOURCE_BEFORE.json').read_text())
after=json.loads((HERE/'SOURCE_AFTER.json').read_text())
require(before['frozen_commit']==after['frozen_commit'],'commit figé')
require(len(before['files'])==len(after['files'])==26,'vingt-six sources')
for old,new in zip(before['files'],after['files']):
    require(old['path']==new['path'],'mêmes chemins')
    require(sha256((HERE/'source'/old['path']).read_bytes()).hexdigest()==old['sha256']==new['copy_sha256']==new['git_sha256'],'copies intactes')
supplement_before=json.loads((HERE/'SUPPLEMENT_BEFORE.json').read_text())
supplement_after=json.loads((HERE/'SUPPLEMENT_AFTER.json').read_text())
require(supplement_before['commit']==supplement_after['commit'],'pin supplément séparé')
for old,new in zip(supplement_before['files'],supplement_after['files']):
    require(old['path']==new['path'],'même supplément')
    require(sha256((HERE/'supplement_source'/old['path']).read_bytes()).hexdigest()==old['sha256']==new['copy_sha256']==new['git_sha256'],'supplément intact')
manifest=HERE/'SHA256SUMS'
if manifest.exists():
    for item in manifest.read_text().splitlines():
        digest,name=item.split('  ',1)
        require(sha256((HERE/name).read_bytes()).hexdigest()==digest,'fermeture '+name)
print(json.dumps({'status':'PASS','sources':26,'supplement_sources':1,'fixtures':3,
                  'line':{'start_beta':str(ball[1]),'terminal_beta':str(terminal[1]),'merges':['25/4','49/4'],'birth_component_vertices':len(at_birth),'query_component_vertices':len(at_query),'shared_site_at_beta9':2},
                  'square':{'beta':str(full[1]),'p':len(inner),'m':len(boundary),'qmin':full[2],'birth_beta':'4','strict_parents':len(opened),'closed_vertices':len(closed[0]),'affine_cofaces':len(cofaces),'lower_open_components':len(lower_open),'lower_closed_components':len(lower_closed)},
                  'support_scope':{'beta':str(local_ball[1]),'first_weights':[str(w) for w in first[2]],'strict_weights':[str(w) for w in strict[2]],'local_qmin':local_ball[2],'global_qmin':global_ball[2]}},sort_keys=True))
