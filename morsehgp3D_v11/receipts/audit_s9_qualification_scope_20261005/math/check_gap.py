#!/usr/bin/env python3
"""Synthetic F8 probe mutation establishes a qualification boundary; no native execution."""
import copy
import functools
import hashlib
import json
import pathlib
import sys
from fractions import Fraction as F
from itertools import combinations
sys.dont_write_bytecode=True
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE/'snapshot/morsehgp3D_v11'
sys.path.insert(0,str(ROOT/'tests/points'))
import points_oracle_stdlib as O
from hgp11_ref.supports import barycentric,split
from hgp11_ref.definition import circumsphere
sign_dates=O.formats.compare_dates

def need(ok,message):
    if not ok:raise RuntimeError(message)

def levels_of(points,k):
    balls = {}
    for q in (2,3,4):
        for part in combinations(points,q):
            cs = circumsphere(part)
            w = None if cs is None else barycentric(part,cs[0])
            if w is not None and min(w)>0: balls[cs] = min(q,balls.get(cs,5))
    levels = {F(0)}
    for (center,level),q in balls.items():
        inside,_shell = split(points,center,level)
        if len(inside)+q<=k+1: levels.add(level)
    return sorted(levels)

def point_tree(doc,levels,dates,owner,floor,strict):
    nodes=doc['nodes']; children=[n['children'] for n in nodes]
    cmp=lambda a,b: sign_dates(dates[a],dates[b]) or (a>b)-(a<b)
    entries=[]
    for rank in sorted(set(floor)):
        entries += sorted((s for s in range(len(dates)) if floor[s]==rank and not strict[s]))
        entries += sorted((s for s in range(len(dates)) if floor[s]==rank and strict[s]),key=functools.cmp_to_key(cmp))
    births=sum(not c for c in children)
    mi,ei=births,0; block_of={}; plateaus=[]; bp=[]; parent=[]; sb=[-1]*len(dates); sp=[-1]*len(dates)
    def add_block(p): bp.append(p); parent.append(O.NONE); return len(bp)-1
    def enter(s,p):
        o=owner[s]
        if o not in block_of: block_of[o]=add_block(p)
        sb[s],sp[s]=block_of[o],p
    ranks={l:r for r,l in enumerate(levels)}
    while mi<len(nodes) or ei<len(entries):
        r=min(ranks[F(nodes[mi]['level'])] if mi<len(nodes) else O.NONE,
              floor[entries[ei]] if ei<len(entries) else O.NONE)
        p=len(plateaus);plateaus.append((levels[r],F(0),F(0)))
        while mi<len(nodes) and ranks[F(nodes[mi]['level'])]==r:
            parts=[block_of.pop(c) for c in children[mi] if c in block_of]
            if len(parts)>=2:
                b=add_block(p)
                for old in parts: parent[old]=b
                block_of[mi]=b
            elif parts: block_of[mi]=parts[0]
            mi+=1
        while ei<len(entries) and floor[entries[ei]]==r and not strict[entries[ei]]:
            enter(entries[ei],p);ei+=1
        while ei<len(entries) and floor[entries[ei]]==r:
            first=entries[ei];p=len(plateaus);plateaus.append(dates[first])
            while ei<len(entries) and floor[entries[ei]]==r and sign_dates(dates[entries[ei]],dates[first])==0:
                enter(entries[ei],p);ei+=1
    used=set(bp)|set(sp);remap={old:new for new,old in enumerate(sorted(used))}
    plateaus=[v for p,v in enumerate(plateaus) if p in used]
    bp=[remap[p] for p in bp];sp=[remap[p] for p in sp]
    need(len(bp)<=2*len(dates)-1,'block bound')
    need(all(sign_dates(plateaus[p-1],plateaus[p])<0 for p in range(1,len(plateaus))),'non-atomic plateaus')
    for s in range(len(dates)):
        need(bp[sb[s]]<=sp[s] and (parent[sb[s]]==O.NONE or sp[s]<bp[parent[sb[s]]]),'entry in non-living block')
    return plateaus,dict(n=len(dates),ids=list(range(len(dates))),site_block=sb,site_plateau=sp,block_plateau=bp,block_parent=parent)

points=O.EIGHT; k=2; m=3; n=len(points)
res=O.Definition(points).order(k)
entries,tree=O.reference_radius(res,n,m)
levels=levels_of(points,k); ranks={v:r for r,v in enumerate(levels)}
doc=dict(nodes=[dict(level=str(node.level),children=list(node.children)) for node in res.nodes])
dates=[date for owner,date in entries]; owners=[owner for owner,date in entries]
predicate=lambda s,r:O.two_roots_sign(dates[s][0],dates[s][1],dates[s][2],levels[r])
floors=[max(r for r in range(len(levels)) if predicate(s,r)>=0) for s in range(n)]
strict=[int(predicate(s,floors[s])>0) for s in range(n)]
plateaus,dump=point_tree(doc,levels,dates,owners,floors,strict)
dump.update(levels={str(r):[format(v.numerator,'x'),format(v.denominator,'x')] for r,v in enumerate(levels)},
            t=[ranks[d[0]] for d in dates],M=[ranks[d[1]] for d in dates],Q=[ranks[d[2]] for d in dates],
            owner=owners,rank=[ranks[node.level] for node in res.nodes],floor=floors,strict=strict,
            cover=[sorted(O.owner_signature(res,n,v)[1]) for v in range(len(res.nodes))],
            plateau_t=[ranks[d[0]] for d in plateaus],plateau_m=[ranks[d[1]] for d in plateaus],
            plateau_q=[ranks[d[2]] for d in plateaus],probe=dict(exact_decisions=0))

def compare(value):
    stats=dict(exact=0,comparisons=0,delayed=0,plateaus=0)
    O.compare(value,res,n,m,stats)
    return stats

baseline=compare(dump)
s=0
need(strict[s]==1,'F8 strict witness absent')
low=max(dump['t'][s],dump['rank'][owners[s]])
need(low<floors[s],'F8 has no lower floor consistent with owner')
changed=copy.deepcopy(dump)
changed['floor'][s]=low
accepted=compare(changed)
need(accepted==baseline,'stdlib comparator observes mutated floor')

def reader_checks(value):
    f=O.formats.PointsFile()
    f.n=n;f.N=len(res.nodes);f.P=len(plateaus);f.Bk=len(value['block_parent'])
    f.parent=[O.NONE if p<0 else p for p in tree.parent]
    f.levels={r:v for r,v in enumerate(levels)}
    for column in ('t','M','Q','owner','floor','strict','rank','block_plateau','block_parent','site_block','site_plateau'):
        setattr(f,column,value[column])
    f.plateau_t=value['plateau_t'];f.plateau_M=value['plateau_m'];f.plateau_Q=value['plateau_q']
    O.formats._check_hanging(f,True)
    O.formats._check_point_tree(f,True)

reader_checks(dump)
reader_checks(changed)
need(changed['floor'][s]!=floors[s] and predicate(s,changed['floor'][s]+1)>=0,'mutation is not a non-maximal floor')
print(json.dumps(dict(pin='c97776ea8b8730bc41c8da4b137879cb8b544c2c',native_executed=False,
                     scope='synthetic output mutation; stdlib comparator and hanging/tree reader checks only',
                     witness='F8 K2 m3 site0',baseline_accepted=True,mutated_stdlib_accepted=True,
                     mutated_hanging_tree_reader_checks_accepted=True,site=s,date=[str(v) for v in dates[s]],
                     correct_floor=floors[s],correct_floor_level=str(levels[floors[s]]),
                     false_floor=low,false_floor_level=str(levels[low]),strict=strict[s],
                     following_rank_still_below_date=True,baseline_stats=baseline),sort_keys=True,separators=(',',':')))
