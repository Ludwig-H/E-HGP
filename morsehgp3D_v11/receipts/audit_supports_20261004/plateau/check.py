#!/usr/bin/env python3
"""Garde indépendante pour les incidences de supports à un plateau FULL, K=5.

Les q2/q3/q4 sont des supports de MEB, pas des K-parties de Γ_K.
Avec p intérieurs et m sites de coquille, C(m,K-p) compte seulement
les parties comprimées I∪A. Le nombre de K-parties de P est C(p+m,K).
Le quotient par traces strictes suffit à la connectivité (T2), pas à
compter tous les nouveaux sommets, cofaces ou arêtes de Γ_K.

Quatre sommets d'un tétraèdre régulier, trois sites proches de son centre :
les six K-parties I3+paire naissent à β=200. Les quatre boules de faces,
β=800/3, portent chacune p=3,m=qmin=3. Au seuil fermé, chacune porte six
K-parties : trois anciennes traces comprimées et trois nouveaux sommets.
Elles relient les six anciennes composantes en UN plateau. Les quatre
cellules font 2,2,1,0 unions DSU, mais chaque face peut être celle qui fait
zéro ou deux unions selon l'ordre. Le nombre de composantes strictes
rencontrées est constamment trois. L'affectation après fermeture est unique.

Aucun import produit/référence ; solveur Gram/Fraction construit ici.
Les 21 MEB de K-parties et 7 MEB de cofaces sont exhaustives sur ce seul
nuage. Pas de qualification native, budget, performance ni theorem global.
"""
from fractions import Fraction as F
from itertools import combinations, permutations
from math import comb
from pathlib import Path
import hashlib
import json

CHECKS = 0

def require(test, what):
    global CHECKS
    CHECKS += 1
    if not test:
        raise ValueError(what)


def sub(a, b):
    return tuple(x-y for x,y in zip(a,b))


def dot(a, b):
    return sum((x*y for x,y in zip(a,b)), F(0))


def solve(matrix, rhs):
    a = [list(row)+[v] for row,v in zip(matrix,rhs)]
    n = len(a)
    for j in range(n):
        pivot = next((i for i in range(j,n) if a[i][j]), None)
        if pivot is None:
            return None
        a[j],a[pivot]=a[pivot],a[j]
        divisor=a[j][j]
        a[j]=[x/divisor for x in a[j]]
        for i in range(n):
            if i != j:
                coefficient=a[i][j]
                a[i]=[x-coefficient*y for x,y in zip(a[i],a[j])]
    return tuple(row[-1] for row in a)


def sphere(points):
    a=points[0]
    if len(points)==1:
        return a,F(0),(F(1),)
    vectors=[sub(x,a) for x in points[1:]]
    gram=[[dot(v,w) for w in vectors] for v in vectors]
    coefficients=solve(gram,[dot(v,v)/2 for v in vectors])
    if coefficients is None:
        return None
    center=tuple(a[j]+sum((q*v[j] for q,v in zip(coefficients,vectors)),F(0)) for j in range(3))
    weights=(1-sum(coefficients,F(0)),)+coefficients
    return center,dot(sub(a,center),sub(a,center)),weights


def meb(ids, cloud):
    candidates=[]
    for arity in range(1,min(4,len(ids))+1):
        for support in combinations(ids,arity):
            ball=sphere([cloud[i] for i in support])
            if ball is None or not all(w>0 for w in ball[2]):
                continue
            center,radius,weights=ball
            if all(dot(sub(cloud[i],center),sub(cloud[i],center))<=radius for i in ids):
                candidates.append((radius,center,support))
    require(bool(candidates),'MEB requires a strict containing support')
    level=min(c[0] for c in candidates)
    minimizers=[c for c in candidates if c[0]==level]
    require(len({c[1] for c in minimizers})==1,'minimum MEB center unique')
    return level,minimizers[0][1]


class DSU:
    def __init__(self, size):
        self.parent=list(range(size))
    def find(self, x):
        while self.parent[x]!=x:
            x=self.parent[x]
        return x
    def union(self,a,b):
        a,b=self.find(a),self.find(b)
        if a==b:
            return False
        lo,hi=sorted((a,b))
        self.parent[hi]=lo
        return True
    def groups(self):
        groups={}
        for i in range(len(self.parent)):
            groups.setdefault(self.find(i),[]).append(i)
        return tuple(sorted(tuple(v) for v in groups.values()))


def components(vertices, cofaces, levels, coface_levels, cut, strict):
    chosen=[v for v in vertices if levels[v]<cut or (levels[v]==cut and not strict)]
    positions={v:i for i,v in enumerate(chosen)}
    dsu=DSU(len(chosen))
    for coface in cofaces:
        level=coface_levels[coface]
        if level>cut or (level==cut and strict):
            continue
        faces=list(combinations(coface,5))
        require(all(face in positions for face in faces),'every active coface has active faces')
        for face in faces[1:]:
            dsu.union(positions[faces[0]],positions[face])
    return tuple(sorted(tuple(chosen[i] for i in group) for group in dsu.groups()))


def main():
    cloud=tuple(tuple(F(v) for v in p) for p in (
        (10,10,10),(10,-10,-10),(-10,10,-10),(-10,-10,10),
        (0,0,0),(1,0,0),(0,1,0)))
    vertices=tuple(combinations(range(7),5))
    cofaces=tuple(combinations(range(7),6))
    levels={v:meb(v,cloud)[0] for v in vertices}
    coface_levels={v:meb(v,cloud)[0] for v in cofaces}
    cut=F(800,3)
    require(len(vertices)==21 and len(cofaces)==7,'28 bounded MEB calls')
    require(sorted(levels.values())==[F(200)]*6+[cut]*12+[F(300)]*3,'all 21 vertex levels')
    require(sorted(coface_levels.values())==[cut]*4+[F(300)]*3,'all seven coface levels')
    before=components(vertices,cofaces,levels,coface_levels,cut,True)
    after=components(vertices,cofaces,levels,coface_levels,cut,False)
    require(len(before)==6 and all(len(g)==1 for g in before),'six strict components')
    require(len(after)==1 and len(after[0])==18,'one closed component with18 vertices')
    old=tuple(group[0] for group in before)
    faces=tuple(combinations(range(4),3))
    touched=[]
    payload=[]
    for face in faces:
        center,level,weights=sphere([cloud[i] for i in face])
        require(level==cut and weights==(F(1,3),)*3,'face ball positive')
        interior=tuple(i for i,p in enumerate(cloud) if dot(sub(p,center),sub(p,center))<level)
        shell=tuple(i for i,p in enumerate(cloud) if dot(sub(p,center),sub(p,center))==level)
        require(interior==(4,5,6) and shell==face,'complete I/U census')
        population=tuple(sorted(interior+shell))
        all_parts=tuple(combinations(population,5))
        compressed=tuple(tuple(sorted(interior+a)) for a in combinations(shell,2))
        old_parts=tuple(v for v in all_parts if levels[v]<cut)
        new_parts=tuple(v for v in all_parts if levels[v]==cut)
        require(comb(3,2)==3 and comb(6,5)==6,'compressed vs all parts')
        require(set(old_parts)==set(compressed) and len(new_parts)==3,'three old and three new')
        require(all(set(shell).issubset(v) for v in new_parts),'new vertices omit an interior')
        roots=tuple(old.index(v) for v in old_parts)
        require(len(set(roots))==3,'three strict global roots touched')
        touched.append(roots)
        payload.append({'support':face,'level':level,'p':3,'m':3,'qmin':3,
                        'compressed_parts':3,'compressed_strict':3,'all_kparts':6,
                        'new_vertices':3,'strict_components':3})
    per_face=[[] for _ in faces]
    traces=[]
    for order in permutations(range(4)):
        dsu=DSU(6)
        unions=[]
        for f in order:
            roots=touched[f]
            count=sum(int(dsu.union(roots[0],r)) for r in roots[1:])
            unions.append(count)
            per_face[f].append(count)
        require(unions==[2,2,1,0] and sum(unions)==5,'operational DSU unions2/2/1/0')
        require(dsu.groups()==(tuple(range(6)),),'same unique owner after closure')
        traces.append({'face_order':order,'unions':unions})
    require(all(min(v)==0 and max(v)==2 for v in per_face),'per-ball operational roles vary')
    # Edges of the full intersection nerve: the union of any two active K-parts.
    active=after[0]
    edge_count=0
    for a,b in combinations(active,2):
        union=tuple(sorted(set(a)|set(b)))
        level=levels[union] if len(union)==5 else (coface_levels[union] if len(union)==6 else F(300))
        edge_count+=int(level<=cut)
    require(edge_count==60,'four cliques of six K-parts;60 nerve edges')
    report={'schema':'supports.plateau.exact.v1','status':'PASS','checks':CHECKS,
            'scope':'one_integer_cloud_K5_only_no_native','k':5,'sites':cloud,
            'meb_calls':28,'strict_components':6,'closed_components':1,
            'closed_vertices':18,'new_vertices':12,'closed_nerve_edges':60,
            'critical_face_cofaces':4,'macro_unions':5,'payload':payload,
            'orders_checked':24,'orders':traces,
            'wrong_compressed_new_vertex_claim_rejected':True,
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
