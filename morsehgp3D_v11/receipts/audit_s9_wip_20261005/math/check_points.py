#!/usr/bin/env python3
"""Bounded Python translation of S9, against closed cuts of Definition. No native execution."""
import ast
import functools
import hashlib
import json
import math
import pathlib
import sys
from fractions import Fraction as F
from itertools import combinations

sys.dont_write_bytecode = True
HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE/'snapshot/morsehgp3D_v11'
sys.path.insert(0,str(ROOT/'tests/points'))
import points_oracle_stdlib as O
from hgp11_ref.supports import Supports, barycentric, split
from hgp11_ref.definition import circumsphere

tree = ast.parse((ROOT/'bench/points_radius.py').read_text())
names = {'sign','sqrt_diff_cmp','sqrt_cmp2','sqrt_bounds','Refusal','square_ratio','radical_classes','sign_of_radicals'}
scope = {'Fraction':F,'math':math,'ZERO':F(0)}
exec(compile(ast.Module(body=[n for n in tree.body if getattr(n,'name',None) in names],type_ignores=[]),
             'frozen_points_radius.py','exec'),scope)
cmp2, sign_dates = scope['sqrt_cmp2'], O.formats.compare_dates


def need(ok, message):
    if not ok: raise RuntimeError(message)


def morton(p):
    return sum(((p[d]>>i)&1)<<(3*i+d) for i in range(24) for d in range(3))


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


class Ancestors:
    def __init__(self,parent):
        self.parent=parent; self.depth=[0]*len(parent); self.jump=[0]*len(parent)
        for v in reversed(range(len(parent))):
            p=parent[v]
            if p<0: self.jump[v]=v; continue
            jp=self.jump[p]; jjp=self.jump[jp]
            self.depth[v]=self.depth[p]+1
            self.jump[v]=jjp if self.depth[p]-self.depth[jp]==self.depth[jp]-self.depth[jjp] else p
    def at(self,v,d):
        while self.depth[v]>d: v=self.parent[v] if self.depth[self.jump[v]]<d else self.jump[v]
        return v
    def lca(self,a,b):
        if self.depth[a]<self.depth[b]: a,b=b,a
        a=self.at(a,self.depth[b])
        while a!=b:
            a,b=(self.parent[a],self.parent[b]) if self.jump[a]==self.jump[b] else (self.jump[a],self.jump[b])
        return a
    def highest(self,x,keep):
        while self.parent[x]>=0:
            up,far=self.parent[x],self.jump[x]
            if far!=up and keep(far): x=far; continue
            if not keep(up): break
            x=up
        return x


def floor_search(predicate,low,high,guess):
    """Translation of floor_of's exact repair, allowing every possible binary64 proposal."""
    a,lo,hi=guess,low,high
    if predicate(a)>=0:
        lo=a; step=1
        while lo<hi:
            c=lo+step if hi-lo>step else hi
            if predicate(c)<0: hi=c-1; break
            lo=c; step=2*step if step<1<<30 else step
    else:
        need(a!=low,'false P(low)')
        hi=a-1; step=1
        while lo<hi:
            c=hi-step if hi-lo>step else lo
            if predicate(c)>=0: lo=c; break
            hi=c-1; step=2*step if step<1<<30 else step
    while lo<hi:
        mid=lo+(hi-lo+1)//2
        if predicate(mid)>=0: lo=mid
        else: hi=mid-1
    need(predicate(lo)>=0,'false floor certificate')
    return lo


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


results=[];floor_proposals=0;lca_checks=0;qualification_checks=0
fixtures=[('singleton',[(0,0,0)],1),('plateau_k1',O.PLATEAU,1),('plateau_k2',O.PLATEAU,2),
          ('five_k2',O.FIVE,2),('eight_k2',O.EIGHT,2),('equilateral_k2',O.EQUILATERAL,2)]
for name,points,k in fixtures:
    points=sorted(points,key=morton); n=len(points);m=1 if k==1 else k+1
    oracle=Supports(points); doc=oracle.canonical(k,list(range(n)));res=oracle.definition.order(k)
    levels=levels_of(points,k);ranks={v:r for r,v in enumerate(levels)}
    nodes=doc['nodes'];parent=[-1 if n['parent'] is None else n['parent'] for n in nodes]
    ancestors=Ancestors(parent);slow=O.Tree(res.nodes)
    for a in range(len(nodes)):
        for b in range(len(nodes)):
            need(ancestors.lca(a,b)==slow.lca(a,b),'Myers LCA differs from parent walk');lca_checks+=1
    incidences=[[] for _ in points]
    if k==1:
        for v,node in enumerate(nodes):
            if node['kind']==0: incidences[points.index(tuple(int(F(c)) for c in node['birth_center']))].append((0,v))
    else:
        for ball in doc['balls']:
            if ball['p']+ball['qmin']>k:continue
            center=[F(c) for c in ball['center']];level=F(ball['level'])
            for s,p in enumerate(points):
                if sum((x-c)**2 for x,c in zip(p,center))<=level:incidences[s].append((ranks[level],ball['node']))
    for row in incidences:row.sort()
    qual=[ranks[F(node['level'])] for node in nodes]
    if m>k:
        for v,node in enumerate(nodes):
            if node['kind']==2:continue
            arrivals=[min(r for r,w in row if w==v) for row in incidences if any(w==v for _r,w in row)]
            qual[v]=sorted(arrivals)[m-1] if len(arrivals)>=m else None
    for v in range(len(nodes)):
        raw=next((cut.level for cut in res.cuts for w,cover,_core in cut.closed if w==v and cover.bit_count()>=m),None)
        need(raw==(None if qual[v] is None else levels[qual[v]]),'qualification differs from exact closed coverage')
        qualification_checks+=1
    dates=[];owner=[];floors=[];strict=[]
    for s,row in enumerate(incidences):
        starts=[]
        for r,v in row:
            if qual[v] is not None:starts.append((max(r,qual[v]),v));continue
            v=parent[v]
            while v>=0 and qual[v] is None:v=parent[v]
            need(v>=0,'never qualified');starts.append((qual[v],v))
        t=min(r for r,v in starts);p1=next(v for r,v in starts if r==t)
        M=Q=0;has=False
        for r,v in starts:
            w=ancestors.lca(p1,v);meet=ranks[F(nodes[w]['level'])]
            if w==v or meet<=r:continue
            if not has or (meet>=M and r<=Q):M,Q,has=meet,r,True;continue
            if meet<=M and r>=Q:continue
            if cmp2(levels[meet],levels[Q],levels[M],levels[r])>0:M,Q=meet,r
        date=(levels[t],levels[M],levels[Q]);P=lambda r:cmp2(date[0],date[1],levels[r],date[2])
        own=ancestors.highest(p1,lambda v:P(ranks[F(nodes[v]['level'])])>=0) if has else p1
        low=max(t,ranks[F(nodes[own]['level'])]);cap=len(levels)-1 if parent[own]<0 else ranks[F(nodes[parent[own]]['level'])]-1
        high=min(M,cap) if has else t
        expected=max(r for r in range(len(levels)) if P(r)>=0)
        for guess in range(low,high+1):
            need(floor_search(P,low,high,guess)==expected,'floor repair differs from maximal exact floor');floor_proposals+=1
        need(P(expected)>=0 and (expected+1==len(levels) or P(expected+1)<0),'non-maximal floor')
        dates.append(date);owner.append(own);floors.append(expected);strict.append(P(expected)>0)
    entries,reference_tree=O.reference_radius(res,n,m)
    for s in range(n):
        need(sign_dates(dates[s],entries[s][1])==0,'date differs from definition cuts')
        need(O.owner_signature(res,n,owner[s])==O.owner_signature(res,n,entries[s][0]),'owner differs from definition cuts')
    plateaus,dump=point_tree(doc,levels,dates,owner,floors,strict)
    for p,value in enumerate(plateaus):
        need(O.native_blocks(dump,{},p)==O.oracle_blocks(entries,reference_tree,value),'point blocks differ from exact ultrametric')
    results.append(dict(name=name,k=k,m=m,sites=n,nodes=len(nodes),delayed=sum(d[1]!=0 for d in dates),
                        strict=sum(strict),plateaus=len(plateaus),blocks=len(dump['block_parent'])))

print(json.dumps(dict(pin='53c027fe848b0d890f164eb87ebf347338c58d55',wip=True,native_executed=False,
                     scope='frozen S9 Python translation vs exact Definition cuts and ultrametric',cases=results,
                     floor_proposals=floor_proposals,lca_checks=lca_checks,qualification_checks=qualification_checks,
                     entry_block_liveness=True,maximal_floor=True,atomic_plateaus=True),sort_keys=True,separators=(',',':')))
