#!/usr/bin/env python3
"""Contrat MEB borné : Gram/Gauss/Fraction indépendant du produit et des refs."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import combinations, permutations, product
from math import comb
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent
checks=presentations=input_orders=0


def require(condition,why):
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
        rows[col]=[v/divisor for v in rows[col]]
        for i in range(n):
            if i!=col:
                factor=rows[i][col]
                rows[i]=[v-factor*w for v,w in zip(rows[i],rows[col])]
    return tuple(row[-1] for row in rows)


def through(points):
    a=points[0]
    vectors=tuple(sub(x,a) for x in points[1:])
    weights=solve([[dot(u,v) for v in vectors] for u in vectors],
                  [F(dot(u,u),2) for u in vectors])
    if weights is None:
        return None
    center=tuple(F(a[j])+sum(w*u[j] for w,u in zip(weights,vectors)) for j in range(3))
    radius2=dot(sub(center,a),sub(center,a))
    return center,radius2,(1-sum(weights),)+weights


def contains(sphere,points):
    center,radius2,_=sphere
    return all(dot(sub(x,center),sub(x,center))<=radius2 for x in points)


def bounded_minimum(points):
    """Présentation gagnante non filtrée par positivité, puis certificat local."""
    global presentations
    n=len(points)
    if not 1<=n<=12:
        raise ValueError('domaine non vide, au plus douze sites')
    require(len(set(points))==n,'partie de sites distincts')
    candidates=[]
    attempts=degenerate=0
    for q in range(1,min(4,n)+1):
        for indices in combinations(range(n),q):
            attempts+=1
            sphere=through([points[i] for i in indices])
            if sphere is None:
                degenerate+=1
            elif contains(sphere,points):
                candidates.append((sphere,indices))
    presentations+=attempts
    require(attempts==sum(comb(n,q) for q in range(1,min(4,n)+1))<=793,
            'compte des présentations non ordonnées')
    require(bool(candidates),'au moins un candidat contient toute F')
    # L'algorithme proposé garde le premier niveau minimal sans comparer les centres.
    winner=min(candidates,key=lambda pair:pair[0][1])
    minimum=winner[0][1]
    tied=tuple(pair for pair in candidates if pair[0][1]==minimum)
    require(all(pair[0][:2]==winner[0][:2] for pair in tied),'unicité au minimum, centres égaux a posteriori')
    # La stricte positivité est retrouvée parmi les supports de la frontière locale.
    strict=tuple(pair for pair in tied if all(w>0 for w in pair[0][2]))
    require(bool(strict),'support strict local existe au minimum')
    certificate=min(strict,key=lambda pair:(len(pair[1]),pair[1]))
    require(all(dot(sub(points[i],winner[0][0]),sub(points[i],winner[0][0]))==minimum
                for i in certificate[1]),'support sur frontière locale')
    return winner,tied,certificate,attempts,degenerate


corners=tuple(product((0,4),(0,6),(0,8)))
maximal=corners+((1,1,1),(1,2,3),(2,3,4),(3,4,5))
lens=((3,3,0),(3,1,0),(2,4,0),(1,3,0),(1,1,0),(2,0,0))
fixtures=(
    ('singleton',((3,2,1),),(F(3),F(2),F(1)),F(0),1),
    ('affine_collinear',((0,0,0),(2,0,0),(7,0,0)),(F(7,2),F(0),F(0)),F(49,4),2),
    ('obtuse',((0,0,0),(6,0,0),(1,1,0)),(F(3),F(0),F(0)),F(9),2),
    ('acute',((0,0,0),(6,0,0),(3,4,0)),(F(3),F(7,8),F(0)),F(625,64),3),
    ('tetra_strict',((0,0,0),(0,4,4),(4,0,4),(4,4,0)),(F(2),F(2),F(2)),F(12),4),
    ('domain12_repeated',maximal,(F(2),F(3),F(4)),F(29),2),
    ('equal_above_min',lens,(F(2),F(2),F(0)),F(4),2),
)
stats=[]
for name,points,center,radius2,qmin in fixtures:
    if len(points)<=4:
        orders=tuple(permutations(range(len(points))))
    else:
        orders=(tuple(range(len(points))),tuple(reversed(range(len(points)))),tuple(range(1,len(points)))+(0,))
    for order in orders:
        ordered=tuple(points[i] for i in order)
        winner,tied,certificate,attempts,degenerate=bounded_minimum(ordered)
        require(winner[0][:2]==(center,radius2),'géométrie attendue '+name)
        require(len(certificate[1])==qmin,'qmin local attendu '+name)
        require(all(0<=x<2**18 for point in ordered for x in point),'valide aux trois profils entiers')
        input_orders+=1
    winner,tied,certificate,attempts,degenerate=bounded_minimum(points)
    stats.append({'fixture':name,'sites':len(points),'attempts':attempts,'degenerate':degenerate,
                  'minimum_presentations':len(tied),'minimum_arities':sorted({len(pair[1]) for pair in tied}),
                  'nonpositive_minimum_presentations':sum(not all(w>0 for w in pair[0][2]) for pair in tied),
                  'beta':str(radius2),'local_qmin':qmin})

require(through(fixtures[1][1]) is None,'support complet collinéaire absent, MEB valide')
obtuse_sphere=through(fixtures[2][1])
require(min(obtuse_sphere[2])<0 and obtuse_sphere[1]>9,'circonsphère obtuse distincte de MEB')
large=next(item for item in stats if item['fixture']=='domain12_repeated')
require(large['attempts']==793 and large['minimum_arities']==[2,3,4]
        and large['nonpositive_minimum_presentations']>0,
        'même MEB de douze sites : arités variées, minimiseur non strict possible')

# Au-dessus du minimum : même F, inclusion complète et niveau égal ne suffisent pas.
left=through((lens[0],lens[1],lens[2]))
right=through((lens[3],lens[4],lens[2]))
require(contains(left,lens) and contains(right,lens),'deux boules contiennent même F')
require(left[:2]==((F(1),F(2),F(0)),F(5))
        and right[:2]==((F(3),F(2),F(0)),F(5)),'niveau5 égal, centres distincts')
require(left[1]>4 and right[1]>4,'égalité non minimale exclue du corollaire')

# Deux MEB de parties différentes peuvent aussi avoir le même niveau et des centres distincts.
one=through(((0,0,0),(2,0,0)))
two=through(((4,0,0),(6,0,0)))
require(one[1]==two[1]==1 and one[0]!=two[0] and all(w>0 for w in one[2]+two[2]),
        'Level seul n’est jamais identité globale')

# Le budget local douze ne couvre pas une éventuelle coface treize.
require(sum(comb(12,q) for q in range(1,5))==793,'793')
require(sum(comb(13,q) for q in range(1,5))==1092,'1092')
try:
    bounded_minimum(tuple((i,0,0) for i in range(12))+((24,0,0),))
except ValueError:
    pass
else:
    raise RuntimeError('modèle borné accepte treize sites')
prefix_beta=F(11*11,4)
whole_beta=F(24*24,4)
require(prefix_beta!=whole_beta==144,'préfixe douze ne remplace pas une coface13')

before=json.loads((HERE/'SOURCE_BEFORE.json').read_text())
after=json.loads((HERE/'SOURCE_AFTER.json').read_text())
require(before['frozen_commit']==after['frozen_commit'],'pin356 inchangé')
require(len(before['files'])==len(after['files'])==19,'dix-neuf sources')
for old,new in zip(before['files'],after['files']):
    require(old['path']==new['path'],'mêmes chemins')
    require(sha256((HERE/'source'/old['path']).read_bytes()).hexdigest()==old['sha256']==new['copy_sha256']==new['git_sha256'],'copies intactes')
manifest=HERE/'SHA256SUMS'
if manifest.exists():
    for row in manifest.read_text().splitlines():
        digest,name=row.split('  ',1)
        require(sha256((HERE/name).read_bytes()).hexdigest()==digest,'fermeture '+name)
print(json.dumps({'status':'PASS','sources':19,'fixture_sets':len(fixtures),'input_orders':input_orders,
                  'support_presentations_evaluated':presentations,'stats':stats,
                  'above_min_equal_level':'5','above_min_centers':[[1,2,0],[3,2,0]],
                  'domain12_presentations':793,'coface13_presentations':1092},sort_keys=True))
