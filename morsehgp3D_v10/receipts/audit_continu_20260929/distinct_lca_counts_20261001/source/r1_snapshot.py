#!/usr/bin/env python3
"""20 exact RAM fixtures: owned seeds, two intervals, DISTINCT-label Fenwick."""
import ast
import hashlib
import json
from bisect import bisect_left, bisect_right
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT=Path(__file__).absolute().parent
SHARED=Path('/workspaces/E-HGP/build/v10-verrou-points/juge_final/echelle_relative/er.py')
PIN='99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d'
def need(c,m):
 if not c: raise RuntimeError(m)
def hashfile(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def source():
 b=(ROOT/'er_snapshot.py').read_bytes()
 need(hashlib.sha256(b).hexdigest()==PIN and hashfile(SHARED)==PIN,'source pin')
 return b
nodes=[v for v in ast.parse(source()).body if isinstance(v,ast.ClassDef) and v.name=='StructureER']
need(len(nodes)==1,'exact class AST inventory')
env={'Fraction':F,'bisect_right':bisect_right,'exiger':need}
exec(compile(ast.Module(body=nodes,type_ignores=[]),'er_snapshot.py','exec'),env)

class Tree:
 def __init__(self,birth,parent):
  self.birth=list(map(F,birth)); self.parent=parent[:]; self.children=[[] for _ in parent]
  for v,p in enumerate(parent):
   if p>=0: self.children[p].append(v)
  self.root=parent.index(-1)
  self.death=[None if p<0 else self.birth[p] for p in parent]
  need(parent.count(-1)==1 and all(p<0 or self.birth[v]<self.birth[p] for v,p in enumerate(parent)),'positive atomic tree')
 def __len__(self): return len(self.parent)

def owned_seeds(T,raw):
 out=[]; moved=0; steps=0
 for p,v,c in raw:
  c=F(c); need(T.birth[v]<=c,'seed before proposed node birth')
  # Closed cuts: equality at death belongs to the parent, never dying child.
  original=v
  while T.death[v] is not None and c>=T.death[v]:
   v=T.parent[v]; steps+=1
  need(T.birth[v]<=c and (T.death[v] is None or c<T.death[v]),'normalized owner alive')
  moved+=v!=original; out.append((p,v,c))
 return out,moved,steps

def full_profile_oracle(T,seeds,n):
 cov=[{} for _ in range(n)]
 # This SMALL independent oracle explicitly expands ancestor paths.
 # The proposed interval algorithm does not call it in production.
 for p,v,c in seeds:
  while True:
   cov[p][v]=min(cov[p].get(v,c),c)
   if T.parent[v]<0: break
   v=T.parent[v]; c=T.birth[v]
 for row in cov:
  need(row,'every point has at least one seed')
  for v,c in row.items():
   need(T.birth[v]<=c and (T.death[v] is None or c<T.death[v]),'oracle coverage inside life')
   if T.parent[v]>=0: need(row[T.parent[v]]==T.birth[T.parent[v]],'oracle persistence')
 return cov

def intervals(T,seeds):
 early=[[] for _ in T.parent]; late=[[] for _ in T.parent]
 for p,v,c in seeds:
  (early if c==T.birth[v] else late)[v].append(p)
 events=[]; enter=[0]*len(T); birth_end=[0]*len(T); full_end=[0]*len(T)
 # Iterative DFS: no recursion depth assumption; each node and seed once.
 stack=[(T.root,False)]
 while stack:
  v,closing=stack.pop()
  if not closing:
   enter[v]=len(events)+1; events.extend(early[v])
   stack.append((v,True))
   stack.extend((ch,False) for ch in reversed(T.children[v]))
  else:
   birth_end[v]=len(events); events.extend(late[v]); full_end[v]=len(events)
 need(len(events)==len(seeds),'all events once')
 return events,[(enter[v],birth_end[v]) for v in range(len(T))],[(enter[v],full_end[v]) for v in range(len(T))],early

def distinct_counts(events,queries,n):
 m=len(events); bit=[0]*(m+1); last=[0]*n; result=[0]*len(queries); by_end=[[] for _ in range(m+1)]
 updates=reads=0
 for q,(l,r) in enumerate(queries):
  need(1<=l<=m+1 and 0<=r<=m,'query bounds')
  if r>=l: by_end[r].append((q,l))
 def update(i,d):
  nonlocal updates
  while i<=m: bit[i]+=d; updates+=1; i+=i & -i
 def prefix(i):
  nonlocal reads
  s=0
  while i>0: s+=bit[i]; reads+=1; i-=i & -i
  return s
 for r,p in enumerate(events,1):
  if last[p]: update(last[p],-1)
  update(r,1); last[p]=r
  for q,l in by_end[r]: result[q]=prefix(r)-prefix(l-1)
 return result,updates,reads

def fixtures():
 trees=[('chain',[1,2,3],[1,2,-1]),('fork',[1,1,2,3],[2,2,3,-1]),
        ('fractional_fork',[1,1,F(3,2),F(5,2)],[2,2,3,-1]),
        ('two_forks',[1,1,1,1,2,2,3],[4,4,5,5,6,6,-1])]
 for name,b,p in trees:
  for mode in range(5):
   T=Tree(b,p); leaves=[v for v,ch in enumerate(T.children) if not ch]
   internal=next(v for v,ch in enumerate(T.children) if ch and v!=T.root)
   raw=[(i,leaves[i%len(leaves)],T.birth[leaves[i%len(leaves)]]) for i in range(6)]
   if mode==0:
    raw=[(i,v,(T.birth[v]+T.death[v])/2) for i,v,_c in raw]
   if mode in (1,3,4):
    raw.extend((0,v,T.birth[v]) for v in leaves)
   if mode==2:
    raw=[s for s in raw if s[0] not in (4,5)]
    raw.extend([(4,internal,(T.birth[internal]+T.death[internal])/2),(5,T.root,T.birth[T.root]+F(1,3))])
   if mode==3:
    raw=[s for s in raw if s[0] not in (4,5)]
    raw.extend([(4,leaves[0],T.death[leaves[0]]),(5,internal,T.death[internal])])
   if mode==4:
    raw=[s for s in raw if s[0] not in (1,4,5)]
    raw.extend([(1,internal,(T.birth[internal]+T.death[internal])/2),
                (4,leaves[0],T.death[leaves[0]]),(5,T.root,T.birth[T.root]+F(1,3))])
   yield name,mode,T,raw

def main():
 cases=[]; comparisons=normalized=overlap_mutants=birth_mutants=0
 for name,mode,T,raw in fixtures():
  seeds,moved,normalization_steps=owned_seeds(T,raw); normalized+=moved
  cov=full_profile_oracle(T,seeds,6)
  sc=SimpleNamespace(n=6,noms=list(map(str,range(6))),ctx=SimpleNamespace(point_id=list(range(6))),arbre_masses=lambda:(T,cov,{}))
  st=env['StructureER'](sc)
  events,birth_q,death_q,early=intervals(T,seeds)
  all_counts,updates,reads=distinct_counts(events,birth_q+death_q,6)
  bcounts=all_counts[:len(T)]; dcounts=all_counts[len(T):]
  expected_b=[]; expected_d=[]
  for v in range(len(T)):
   bs={p for p,row in enumerate(cov) if v in row and row[v]<=T.birth[v]}
   ds={p for p,row in enumerate(cov) if v in row and (T.death[v] is None or row[v]<T.death[v])}
   row=st._cs.get(v,[])
   b=bisect_right(row,T.birth[v]); d=len(row) if T.death[v] is None else bisect_left(row,T.death[v])
   need(b==len(bs)==bcounts[v] and d==len(ds)==dcounts[v],'AST prefix / independent sets / Fenwick agree')
   expected_b.append(b); expected_d.append(d); comparisons+=2
  # Causal arithmetic mutant: count seed occurrences, not distinct point IDs.
  bad_occurrences=[max(0,r-l+1) for l,r in birth_q+death_q]
  occurrence_killed=bad_occurrences!=all_counts; overlap_mutants+=occurrence_killed
  # Causal endpoint mutant: omit OWN direct seeds exactly at node birth.
  bad_birth=[len(set(events[l-1+len(early[v]):r])) for v,(l,r) in enumerate(birth_q)]
  birth_killed=bad_birth!=bcounts; birth_mutants+=birth_killed
  cases.append({'tree':name,'mode':mode,'n':6,'H':len(T),'T':len(seeds),'raw_seeds':[[p,v,str(c)] for p,v,c in raw],
                'owned_seeds':[[p,v,str(c)] for p,v,c in seeds],'normalized_seed_moves':moved,'normalization_parent_steps':normalization_steps,
                'events':events,'birth_intervals':birth_q,'before_death_intervals':death_q,
                'counts_birth':bcounts,'counts_before_death':dcounts,'AST_counts_birth':expected_b,'AST_counts_before_death':expected_d,
                'fenwick_update_cell_visits':updates,'fenwick_query_cell_visits':reads,
                'occurrence_mutant_rejected':occurrence_killed,'birth_omission_mutant_rejected':birth_killed})
 need(len(cases)==20 and normalized>0 and overlap_mutants>0 and birth_mutants>0,'fixture/nonvacuity inventory')
 source()
 return {'scope':'20 abstract exact persistent RAM profiles; real StructureER AST only; no Scene/native/geometric/GCP claim',
         'source_pin':PIN,'cases':cases,'endpoint_comparisons':comparisons,'normalized_seed_moves':normalized,
         'occurrence_mutants_rejected':overlap_mutants,'birth_omission_mutants_rejected':birth_mutants,'native_calls':0,'GCP':'unused'}
if __name__=='__main__': print(json.dumps(main(),sort_keys=True,indent=1))
