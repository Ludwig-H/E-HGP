#!/usr/bin/env python3
"""Bornes proposées : Gram/Fraction et petites boîtes exhaustives, hors produit."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import product
from math import prod
from pathlib import Path
import json

HERE = Path(__file__).resolve().parent
checks = boxes = points = 0


def require(condition, context):
    global checks
    if not condition:
        raise RuntimeError(context)
    checks += 1


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(matrix, rhs):
    n = len(rhs)
    rows = [[F(v) for v in row]+[F(rhs[i])] for i, row in enumerate(matrix)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [v/divisor for v in rows[col]]
        for i in range(n):
            if i != col:
                factor = rows[i][col]
                rows[i] = [v-factor*w for v, w in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def sphere(support):
    a = support[0]
    vectors = tuple(sub(p, a) for p in support[1:])
    gram = [[dot(u,v) for v in vectors] for u in vectors]
    weights = solve(gram, [F(dot(u,u),2) for u in vectors])
    if weights is None:
        return None
    center = tuple(F(a[j])+sum(w*u[j] for w,u in zip(weights,vectors)) for j in range(3))
    q = len(support)
    if q == 1:
        denominator = 1
    elif q == 2:
        denominator = 2
    elif q == 3:
        denominator = 2*(gram[0][0]*gram[1][1]-gram[0][1]*gram[1][0])
    else:
        u,v,w = vectors
        determinant = u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0])
        denominator = 2*abs(determinant)
    numerator = tuple((center[j]-a[j])*denominator for j in range(3))
    require(denominator > 0 and all(v.denominator == 1 for v in numerator), "coefficients entiers")
    return a, tuple(int(v) for v in numerator), denominator, center, dot(sub(center,a),sub(center,a))


def power(ball, x):
    a,n,d,_,_ = ball
    delta = sub(x,a)
    return d*dot(delta,delta)-2*dot(n,delta)


def bounds(ball, intervals):
    a,n,d,_,_ = ball
    endpoints = [(lo-a[j],hi-a[j]) for j,(lo,hi) in enumerate(intervals)]
    square_min = [0 if lo<=0<=hi else min(lo*lo,hi*hi) for lo,hi in endpoints]
    square_max = [max(lo*lo,hi*hi) for lo,hi in endpoints]
    linear_min = [min(n[j]*lo,n[j]*hi) for j,(lo,hi) in enumerate(endpoints)]
    linear_max = [max(n[j]*lo,n[j]*hi) for j,(lo,hi) in enumerate(endpoints)]
    lower = d*sum(square_min)-2*sum(linear_max)
    upper_coarse = d*sum(square_max)-2*sum(linear_min)
    upper_sharp = sum(max(d*lo*lo-2*n[j]*lo,d*hi*hi-2*n[j]*hi)
                      for j,(lo,hi) in enumerate(endpoints))
    return lower,upper_coarse,upper_sharp


fixtures = (
    ("q1",((1,1,1),)),
    ("q2",((0,1,1),(2,1,1))),
    ("q3_aigu",((0,0,0),(2,0,0),(1,2,0))),
    ("q3_obtus",((0,0,0),(4,0,0),(1,1,0))),
    ("q4_strict",((0,0,0),(0,2,2),(2,0,2),(2,2,0))),
    ("q4_exterieur_hull",((0,0,0),(4,0,0),(0,4,0),(0,0,4))),
    ("q4_noncritique",((0,1,1),(2,1,1),(1,2,1),(1,1,2))),
)
intervals = tuple((lo,hi) for lo in range(3) for hi in range(lo,3))
classifications = {"outside":0,"inside":0,"ambiguous":0}
loose_outside = None
for name,support in fixtures:
    ball = sphere(support)
    require(ball is not None, "support indépendant "+name)
    a,n,d,center,radius2 = ball
    for axes in product(intervals,repeat=3):
        lower,upper_coarse,upper = bounds(ball,axes)
        nearest = tuple(max(F(lo),min(F(hi),center[j])) for j,(lo,hi) in enumerate(axes))
        true_min = d*(dot(sub(nearest,center),sub(nearest,center))-radius2)
        corners = tuple(product(*((lo,hi) for lo,hi in axes)))
        true_max = max(d*(dot(sub(x,center),sub(x,center))-radius2) for x in corners)
        require(lower<=true_min<=true_max==upper<=upper_coarse, "bornes continues "+name)
        for x in product(*(range(lo,hi+1) for lo,hi in axes)):
            exact = d*(dot(sub(x,center),sub(x,center))-radius2)
            require(power(ball,x)==exact and lower<=exact<=upper, "points intérieurs/coins "+name)
            points += 1
        if lower>0:
            require(true_min>0, "rejet extérieur sûr")
            classifications["outside"] += 1
        elif upper<0:
            require(true_max<0, "intérieur strict sûr")
            classifications["inside"] += 1
        else:
            classifications["ambiguous"] += 1
            if true_min>0 and loose_outside is None:
                loose_outside = {"fixture":name,"box":axes,"lower":lower,"true_min":str(true_min)}
        boxes += 1

ball=sphere(fixtures[1][1])
whole=((0,2),)*3
corner_min=min(power(ball,x) for x in product((0,2),repeat=3))
require(corner_min>0 and power(ball,(1,1,1))<0, "minimum aux coins NON minorant")
touch=((0,0),(1,1),(1,1))
require(bounds(ball,touch)==(0,0,0), "contact exact retenu")
require(not(bounds(ball,touch)[0]>0) and not(bounds(ball,touch)[2]<0), "signes stricts")
require(loose_outside is not None, "borne prudente peut laisser un extérieur ambigu")

# Contrat du census : une coquille complète peut dépasser K même si |I|<K.
shell=((0,1,1),(2,1,1),(1,0,1),(1,2,1),(1,1,0),(1,1,2))
cloud=((1,1,1),)+shell
interior=tuple(i for i,x in enumerate(cloud) if power(ball,x)<0)
boundary=tuple(i for i,x in enumerate(cloud) if power(ball,x)==0)
require(interior==(0,) and len(boundary)==6, "un intérieur, six contacts")
require(len(interior)<2<len(boundary), "I<K n'autorise pas de plafonner U")
for k in (1,2,3):
    if len(interior)>=k:
        witnesses=interior[:k]
        require(len(set(witnesses))==k and all(power(ball,cloud[i])<0 for i in witnesses), "K témoins distincts stricts")
    else:
        require(interior==tuple(i for i,x in enumerate(cloud) if power(ball,x)<0)
                and boundary==tuple(i for i,x in enumerate(cloud) if power(ball,x)==0), "I/U complets")

for bits in (18,21,24):
    m=2**bits
    require(72*m**5<2**127, "q4 : chaque produit/somme native")
    require(216*m**6<2**(6*bits+8), "q3 : budget SideInt")
    require((6*bits+8<=127)==(bits==18), "q3 natif uniquement B18")
    require(m-1<m and not(m<m), "extrémité 2^B exclue du Point public")

before=json.loads((HERE/'SOURCE_BEFORE.json').read_text())
after=json.loads((HERE/'SOURCE_AFTER.json').read_text())
require(before['commit']==after['commit'], "commit figé")
require(len(before['files'])==len(after['files'])==12, "douze sources")
for old,new in zip(before['files'],after['files']):
    require(old['path']==new['path'], "même liste source")
    require(sha256((HERE/'source'/old['path']).read_bytes()).hexdigest()==old['sha256']==new['copy_sha256']==new['git_sha256'], "capture intacte")
wip_before=json.loads((HERE/'SOURCE_WIP_BEFORE.json').read_text())
wip_after=json.loads((HERE/'SOURCE_WIP_AFTER.json').read_text())
require(len(wip_before['files'])==len(wip_after['files'])==3, "trois sources WIP séparées")
for old,new in zip(wip_before['files'],wip_after['files']):
    require(old['path']==new['path'], "même liste WIP")
    require(sha256((HERE/'wip_source'/old['path']).read_bytes()).hexdigest()==old['sha256']==new['copy_sha256'], "copie WIP intacte")
manifest=HERE/'SHA256SUMS'
if manifest.exists():
    for line in manifest.read_text().splitlines():
        digest,name=line.split('  ',1)
        require(sha256((HERE/name).read_bytes()).hexdigest()==digest, "fermeture "+name)
print(json.dumps({'status':'PASS','fixtures':len(fixtures),'boxes':boxes,'integer_points':points,
                  'classifications':classifications,'corner_min_counterexample':{'corner_min':corner_min,'interior_power':power(ball,(1,1,1))},
                  'complete_shell_size':len(boundary),'conservative_ambiguity':loose_outside},sort_keys=True))
