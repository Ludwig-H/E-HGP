"""Tiny exhaustive Fraction proof probes; Definition only, no native/product imports."""
from fractions import Fraction as F
from itertools import combinations, permutations
from pathlib import Path
import hashlib, importlib.util, json, sys, types
ROOT=Path(__file__).resolve().parents[2]
REF=ROOT/'reference/hgp11_ref'
pkg=types.ModuleType('vertical_reuse_definition');pkg.__path__=[str(REF)];sys.modules[pkg.__name__]=pkg
for leaf in ('model','definition'):
 spec=importlib.util.spec_from_file_location(pkg.__name__+'.'+leaf,REF/(leaf+'.py'))
 mod=importlib.util.module_from_spec(spec);sys.modules[spec.name]=mod;spec.loader.exec_module(mod)
D=sys.modules[pkg.__name__+'.definition']
def need(ok,msg):
 if not ok:raise ValueError(msg)
def morton(p):return sum(((v>>bit)&1)<<(3*bit+axis) for axis,v in enumerate(p) for bit in range(24))
def balls(points):
 found={}
 for q in range(2,5):
  for s in combinations(range(len(points)),q):
   pts=[points[i] for i in s]; a=pts[0];delta=[tuple(x-y for x,y in zip(p,a)) for p in pts[1:]]
   weights=D._solve([[2*sum(x*y for x,y in zip(u,v)) for v in delta] for u in delta], [sum(x*x for x in u) for u in delta])
   if weights is None or min(weights)<=0 or sum(weights)>=1:continue
   center,level=D.circumsphere(pts)
   key=(level,center)
   if key not in found:found[key]=(q,s)
 out=[]
 for (level,center),(q,s) in sorted(found.items()):
  distance=[sum((F(x)-y)**2 for x,y in zip(p,center)) for p in points]
  inner=tuple(i for i,d in enumerate(distance) if d<level);shell=tuple(i for i,d in enumerate(distance) if d==level)
  out.append(dict(level=level,center=center,q=q,support=s,inner=inner,shell=shell))
 return out
def parent_of(order):
 parent=[None]*len(order.nodes)
 for i,node in enumerate(order.nodes):
  for child in node.children:parent[child]=i
 return parent
def climb(order,parent,seed,level,closed=True):
 need(order.nodes[seed].level<=level,'seed not alive')
 while parent[seed] is not None and (order.nodes[parent[seed]].level<=level if closed else order.nodes[parent[seed]].level<level):seed=parent[seed]
 return seed
def births(order,node):
 if not order.nodes[node].children:return (node,)
 return tuple(x for child in order.nodes[node].children for x in births(order,child))
base=[
 ((3,0,0),(0,3,0),(2,2,0)),
 ((0,0,0),(2,0,0),(4,0,0)),
 ((0,0,0),(2,0,0),(0,2,0),(2,2,0)),
 ((0,0,0),(2,0,0),(0,2,0),(2,2,0),(1,1,0)),
 ((0,0,0),(4,0,0),(2,3,0),(2,1,0)),
 ((0,0,0),(2,2,0),(2,0,2),(0,2,2),(1,1,1)),
 ((1,2,6),(8,4,8),(2,1,3),(7,8,5),(4,4,4)),
 ((0,2,0),(1,0,0),(3,0,0),(4,2,0),(3,4,0),(1,4,0)),
]
# Every cloud of size2..5 from this fixed six-site planar grid is inspected.
grid=((0,0,0),(2,0,0),(0,2,0),(2,2,0),(1,1,0),(2,1,0))
clouds=[tuple(s) for n in range(2,6) for s in combinations(grid,n)]
clouds+=base
clouds += [tuple(tuple(3*p[a]+5 for a in axes) for p in base[0]) for axes in permutations(range(3))]
checks=regular=faces=terminals=interior_removed=at_birth=multi_terminal=extended=0
kills=dict(open_cut=0,no_lift=0,terminal_date=0,foreign_seed=0)
witness=None;extended_witness=None
for raw in clouds:
 points=tuple(sorted(raw,key=morton));truth=D.Definition(points);catalogue=balls(points)
 for b in catalogue:
  q,I,U,level=b['q'],b['inner'],b['shell'],b['level'];h=len(I)+q
  if len(U)!=q:
   extended+=1
   if extended_witness is None and len(U)>q:
    extended_witness=dict(points=points,q=q,m=len(U),wrong_birth_order=h,
       strict_faces_at_wrong_birth=sum(truth.beta(tuple(sorted(I+A)))<level for A in combinations(U,q)))
   continue
  if h>min(5,len(points)):continue
  regular+=1;P=tuple(sorted(I+U));need(len(P)==h,'regular full population');checks+=1
  lower=truth.order(h-1);upper=truth.order(h);parent=parent_of(lower)
  upper_node=truth.node_at(h,P,level)
  need(not upper.nodes[upper_node].children and upper.nodes[upper_node].level==level and upper.nodes[upper_node].center==b['center'],'regular upper birth');checks+=1
  historical=P[:-1];target=truth.node_at(h-1,historical,level);all_seeds=set()
  if P[-1] in I:interior_removed+=1;need(truth.beta(historical)==level,'interior omission keeps full strict support');checks+=1
  for omitted in reversed(U):
   R=tuple(i for i in P if i!=omitted);initial=truth.beta(R);faces+=1
   need(initial<level,'T2 strict regular face');checks+=1
   current=truth.node_at(h-1,R,initial)
   need(truth.node_at(h-1,R,level)==target,'T3 closed equality');checks+=1
   for seed in births(lower,current):
    all_seeds.add(seed);terminals+=1
    need(lower.nodes[seed].level<=initial,'T5 dated terminal');checks+=1
    need(climb(lower,parent,seed,level)==target,'T6 vertical image');checks+=1
    if climb(lower,parent,seed,level,False)!=target:kills['open_cut']+=1
    if seed!=target:kills['no_lift']+=1
    if climb(lower,parent,seed,lower.nodes[seed].level)!=target:kills['terminal_date']+=1
   for seed,node in enumerate(lower.nodes):
    if not node.children and node.level<=level and climb(lower,parent,seed,level)!=target:kills['foreign_seed']+=1;break
  multi_terminal+=len(all_seeds)>1
  if points==tuple(sorted(base[0],key=morton)) and q==2 and len(I)==1:
   need(level==F(9,2) and truth.beta(historical)==F(9,2),'witness closed date')
   need({lower.nodes[s].level for s in all_seeds}=={F(5,4)} and len(all_seeds)==2,'witness two terminals')
   witness=dict(points=points,inner=I,shell=U,level=str(level),historical=historical,
                historical_initial=str(truth.beta(historical)),terminal_levels=['5/4','5/4'],target=target)
   checks+=2
need(witness is not None and interior_removed>0 and all(kills.values()),'nonvacuity')
# The first extended witness is a right triangle: formula h=q=2 is not its
# birth order, since its two leg pairs are strict at the circumcircle level.
need(extended_witness['strict_faces_at_wrong_birth']>0,'extended counterexample')
# Persistent table contract: b is a geometric catalogue ordinal, never a raw
# NodeIdx or a DSU root. Erasure / missing slot, wrong order and equal date
# are rejected before using an alleged strict regular predecessor.
def hint(seed,expected_order,lower,level):
 need(seed is not None and 0<=seed<len(lower.nodes),'missing/outside hint')
 need(lower.k==expected_order and not lower.nodes[seed].children,'wrong order or nonbirth')
 need(lower.nodes[seed].level<level,'strict predecessor date')
 return seed
rejected=0
points=tuple(sorted(base[0],key=morton));truth=D.Definition(points);lower=truth.order(2)
seed=next(i for i,n in enumerate(lower.nodes) if not n.children);level=F(9,2)
hint(seed,2,lower,level)
for operation in (lambda:hint(None,2,lower,level),lambda:hint(len(lower.nodes),2,lower,level),
                  lambda:hint(seed,1,lower,level),lambda:hint(seed,2,lower,F(5,4))):
 try:operation()
 except ValueError:rejected+=1
 else:raise ValueError('bad table hint accepted')
need(rejected==4,'hint mutation count')
square=D.Definition(base[3][:-1]);circle=F(2)
square_birth_orders=[k for k in (2,3,4) if any(not n.children and n.level==circle for n in square.order(k).nodes)]
need(square_birth_orders==[3,4],'extended multiple births')
need((len(clouds),regular,faces,terminals,checks,interior_removed)==(70,239,501,516,2522,8),
     'fixed proof coverage inventory')
sourcepins={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [REF/'model.py',REF/'definition.py']}
print(json.dumps(dict(clouds=len(clouds),regular=regular,faces=faces,terminal_choices=terminals,checks=checks,
 interior_removed=interior_removed,multiple_terminal_cases=multi_terminal,extended_balls=extended,
 mutant_divergences=kills,witness=witness,extended_counterexample=extended_witness,sourcepins=sourcepins,
 guard_mutants_rejected=rejected,square_extended_birth_orders=square_birth_orders,native_calls=0),sort_keys=True))
