#!/usr/bin/env python3
"""OPEN RAM audit of consecutive-occurrence LCA distinct-subtree counts."""
import ast
import hashlib
import json
from bisect import bisect_left, bisect_right
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace

ROOT=Path(__file__).absolute().parent
PINS={'r1_snapshot.py':'debe1ec3eff15ccbb5965b4e29ccc2f648c5fb26f777605c89301c25b9e5f00f',
      'er_snapshot.py':'99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d'}
def need(c,m):
 if not c: raise RuntimeError(m)
def source(name):
 b=(ROOT/name).read_bytes(); need(hashlib.sha256(b).hexdigest()==PINS[name],'snapshot pin '+name); return b
# Complete original snapshots stay unchanged. Only the explicitly inventoried
# AST definitions are compiled: no LIVE shared path/module/main is executed.
R1_DEFINITIONS=('Tree','owned_seeds','full_profile_oracle','intervals','distinct_counts','fixtures')
r1_nodes=[v for v in ast.parse(source('r1_snapshot.py')).body
          if isinstance(v,(ast.ClassDef,ast.FunctionDef)) and v.name in R1_DEFINITIONS]
need(tuple(v.name for v in r1_nodes)==R1_DEFINITIONS,'exact R1 AST definition inventory')
er_nodes=[v for v in ast.parse(source('er_snapshot.py')).body if isinstance(v,ast.ClassDef) and v.name=='StructureER']
need(len(er_nodes)==1,'exact ER StructureER class AST inventory')
er_env={'Fraction':F,'bisect_right':bisect_right,'exiger':need}
exec(compile(ast.Module(body=er_nodes,type_ignores=[]),'er_snapshot.py','exec'),er_env)
r1={'F':F,'need':need,'env':er_env}
exec(compile(ast.Module(body=r1_nodes,type_ignores=[]),'r1_snapshot.py','exec'),r1)

def event_trace(T,seeds):
 early=[[] for _ in T.parent]; late=[[] for _ in T.parent]
 for seed in seeds:
  p,v,c=seed; (early if c==T.birth[v] else late)[v].append(seed)
 trace=[]; stack=[(T.root,False)]
 while stack:
  v,closing=stack.pop()
  if not closing:
   trace.extend(early[v]); stack.append((v,True)); stack.extend((ch,False) for ch in reversed(T.children[v]))
  else: trace.extend(late[v])
 return trace,early,late

def walk_lca(parent,a,b):
 ancestors=set(); steps=0
 while a>=0: ancestors.add(a); a=parent[a]; steps+=1
 while b not in ancestors: b=parent[b]; steps+=1
 return b,steps

def original_euler(T):
 order=[]; tin=[0]*len(T); tout=[0]*len(T); stack=[(T.root,False)]
 while stack:
  v,closing=stack.pop()
  if not closing:
   tin[v]=len(order); order.append(v); stack.append((v,True)); stack.extend((ch,False) for ch in reversed(T.children[v]))
  else: tout[v]=len(order)
 return order,tin,tout

def original_counts(T,dB,dF):
 order,tin,tout=original_euler(T); prefix=[0]
 for v in order: prefix.append(prefix[-1]+dB[v]+dF[v])
 deaths=[prefix[tout[v]]-prefix[tin[v]] for v in range(len(T))]
 births=[deaths[v]-dF[v] for v in range(len(T))]
 return births,deaths,prefix

def lca_counts(T,trace,early,late,n,mode='correct'):
 H=len(T); dB=[len(x) for x in early]; dF=[len(x) for x in late]
 last=[None]*n; corrections=[]; original_steps=0
 # Augmented tree only for a SMALL independent LCA/accumulation oracle:
 # F_v=2v, B_v=2v+1; every occurrence is a separate attached leaf.
 ap=[-1]*(2*H+len(trace))
 for v,p in enumerate(T.parent):
  ap[2*v]=-1 if p<0 else 2*p+1; ap[2*v+1]=2*v
 for i,(_p,v,c) in enumerate(trace): ap[2*H+i]=2*v+(1 if c==T.birth[v] else 0)
 aug_weights=[0]*(2*H+len(trace))
 for i in range(len(trace)): aug_weights[2*H+i]=1
 for i,(p,b,cb) in enumerate(trace):
  if last[p] is not None:
   j=last[p]; _p,a,ca=trace[j]
   w,steps=walk_lca(T.parent,a,b); original_steps+=steps
   targetF=(a==w and ca>T.birth[w]) or (b==w and cb>T.birth[w])
   exact_target=2*w+(0 if targetF else 1)
   aug_lca,_unused=walk_lca(ap,2*H+j,2*H+i)
   need(aug_lca==exact_target,'original owner/late formula agrees with explicit augmented LCA')
   if mode=='all_B': targetF=False
   if mode=='all_F': targetF=True
   (dF if targetF else dB)[w]-=1
   aug_weights[aug_lca]-=1
   corrections.append({'point':p,'left_occurrence':j,'right_occurrence':i,'owner_lca':w,
                       'correct_target':'F' if exact_target%2==0 else 'B'})
  last[p]=i
 # Independent augmented postorder accumulation (no original Euler formula).
 ach=[[] for _ in ap]
 for u,p in enumerate(ap):
  if p>=0: ach[p].append(u)
 stack=[2*T.root]; order=[]
 while stack:
  u=stack.pop(); order.append(u); stack.extend(ach[u])
 totals=aug_weights[:]
 for u in reversed(order):
  if ap[u]>=0: totals[ap[u]]+=totals[u]
 oracleB=[totals[2*v+1] for v in range(H)]; oracleF=[totals[2*v] for v in range(H)]
 births,deaths,prefix=original_counts(T,dB,dF)
 if mode=='correct': need(births==oracleB and deaths==oracleF,'collapsed B/F formula agrees with augmented accumulation')
 return {'births':births,'deaths':deaths,'dB':dB,'dF':dF,'prefix':prefix,
         'corrections':corrections,'original_lca_queries':len(corrections),'original_parent_walk_steps':original_steps,
         'augmented_oracle_births':oracleB,'augmented_oracle_deaths':oracleF}

def main():
 cases=[]; comparisons=all_B_kills=all_F_kills=sum_kills=cap_kills=clamp_kills=0
 same_owner_early_late=0; correctionsB=correctionsF=0
 for name,mode,T,raw0 in r1['fixtures']():
  raw=list(raw0)
  # Redundant extra seed preserves R1's true profile while forcing a label
  # with OWN early and late occurrences on the SAME node (mode1, four trees).
  if mode==1:
   v=next(v for v,ch in enumerate(T.children) if not ch)
   raw.append((0,v,(T.birth[v]+T.death[v])/2))
  seeds,moved,normalization_steps=r1['owned_seeds'](T,raw)
  cov=r1['full_profile_oracle'](T,seeds,6)
  sc=SimpleNamespace(n=6,noms=list(map(str,range(6))),ctx=SimpleNamespace(point_id=list(range(6))),arbre_masses=lambda:(T,cov,{}))
  st=r1['env']['StructureER'](sc)
  trace,early,late=event_trace(T,seeds)
  events,birth_q,death_q,_early=r1['intervals'](T,seeds)
  need([p for p,_v,_c in trace]==events,'identical R1 event order')
  fw,_updates,_reads=r1['distinct_counts'](events,birth_q+death_q,6)
  expectedB=[]; expectedF=[]
  for v in range(len(T)):
   bs={p for p,row in enumerate(cov) if v in row and row[v]<=T.birth[v]}
   fs={p for p,row in enumerate(cov) if v in row and (T.death[v] is None or row[v]<T.death[v])}
   cs=st._cs.get(v,[])
   b=bisect_right(cs,T.birth[v]); f=len(cs) if T.death[v] is None else bisect_left(cs,T.death[v])
   need(b==len(bs)==fw[v] and f==len(fs)==fw[len(T)+v],'Fenwick / set oracle / actual AST counts')
   expectedB.append(b); expectedF.append(f)
  correct=lca_counts(T,trace,early,late,6)
  need(correct['births']==expectedB and correct['deaths']==expectedF,'new algorithm exact counts')
  need(all(0<=b<=f<=6 for b,f in zip(correct['births'],correct['deaths'])),'true distinct counts bounds')
  need(all(0<=d<=6 for d in correct['dF']),'dF is late-only new-point count')
  need(all(-len(seeds)<=p<=len(seeds) for p in correct['prefix']),'signed prefix bounded by paid seed count')
  comparisons+=2*len(T)
  for row in correct['corrections']:
   correctionsF+=row['correct_target']=='F'; correctionsB+=row['correct_target']=='B'
  for v in range(len(T)):
   ep={p for p,_w,_c in early[v]}; lp={p for p,_w,_c in late[v]}
   same_owner_early_late+=len(ep & lp)
  badB=lca_counts(T,trace,early,late,6,'all_B'); badF=lca_counts(T,trace,early,late,6,'all_F')
  killedB=(badB['births'],badB['deaths'])!=(expectedB,expectedF)
  killedF=(badF['births'],badF['deaths'])!=(expectedB,expectedF)
  all_B_kills+=killedB; all_F_kills+=killedF
  naiveB=[len({p for p,_w,_c in early[v]})+sum(expectedF[ch] for ch in T.children[v]) for v in range(len(T))]
  naiveF=[naiveB[v]+len({p for p,_w,_c in late[v]}) for v in range(len(T))]
  sum_killed=(naiveB,naiveF)!=(expectedB,expectedF); sum_kills+=sum_killed
  cap_errors=[[c,p] for c,p in enumerate(T.parent) if p>=0 and expectedF[c]!=expectedB[p]
              and min(expectedF[c],2)==min(expectedB[p],2)]
  cap_kills+=bool(cap_errors)
  # Signed corrections cannot be clamped or accumulated in unsigned words.
  clampB=[max(0,x) for x in correct['dB']]
  clamp_birth,clamp_death,_p=original_counts(T,clampB,correct['dF'])
  clamp_killed=(clamp_birth,clamp_death)!=(expectedB,expectedF); clamp_kills+=clamp_killed
  cases.append({'tree':name,'mode':mode,'H':len(T),'T':len(seeds),'n':6,
                'redundant_same_owner_early_late_seed_added':mode==1,'normalized_moves':moved,
                'normalization_parent_steps':normalization_steps,'owned_seeds':[[p,v,str(c)] for p,v,c in seeds],
                'new_counts':correct,'Fenwick_counts_birth':fw[:len(T)],'Fenwick_counts_death':fw[len(T):],
                'AST_counts_birth':expectedB,'AST_counts_death':expectedF,
                'all_B_mutant_rejected':killedB,'all_F_mutant_rejected':killedF,
                'sum_children_mutant_rejected':sum_killed,'mcs2_cap_false_equalities':cap_errors,
                'signed_clamp_mutant_rejected':clamp_killed})
 need(len(cases)==20 and comparisons==180,'20 fixtures /180 endpoint counts')
 need(same_owner_early_late>=4 and correctionsB>0 and correctionsF>0,'B/F causal branches both exercised')
 need(min(all_B_kills,all_F_kills,sum_kills,cap_kills,clamp_kills)>0,'all five causal controls nonvacuous')
 for name in PINS: source(name)
 return {'scope':'exact RAM LCA cancellation proposal; immutable ER99e8 and R1 AST snapshots only; no native/Scene/GCP/GPU or geometrical realizability claim',
         'snapshot_pins':PINS,'cases':cases,'endpoint_comparisons':comparisons,'same_owner_early_late_labels':same_owner_early_late,
         'correction_targets_B':correctionsB,'correction_targets_F':correctionsF,'all_B_mutants_rejected':all_B_kills,
         'all_F_mutants_rejected':all_F_kills,'sum_children_mutants_rejected':sum_kills,
         'mcs2_cap_mutants_rejected':cap_kills,'signed_clamp_mutants_rejected':clamp_kills,'native_calls':0,'GCP':'unused'}
if __name__=='__main__': print(json.dumps(main(),sort_keys=True,indent=1))
