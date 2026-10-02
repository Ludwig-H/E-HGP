"""Independent scalar capacity/structure and two-pass-state models. No native allocation."""
from functools import lru_cache
from pathlib import Path
import itertools,json
ROOT=Path(__file__).resolve().parent
MAX_N=(1<<32)-2

def need(ok,msg):
 if not ok:raise ValueError(msg)
@lru_cache(None)
def recurrence(n,leaf):
 if n<=leaf:return 1,1
 a,da=recurrence(n//2,leaf);b,db=recurrence(n-n//2,leaf)
 return a+b+1,max(da,db)+1

def quotient(n,leaf):
 width=1
 while n//width>leaf:width*=2
 extra=n%width if n//width==leaf else 0
 return 2*(width+extra)-1

def nodes_small(n,leaf):
 result=[]
 def visit(begin,end):
  here=len(result);result.append(None)
  if end-begin>leaf:
   middle=begin+(end-begin)//2;visit(begin,middle);visit(middle,end)
  result[here]=(begin,end,len(result))
 visit(0,n);return result

def walk(signs,nodes,leaf,k,keep_shell,fill):
 p=m=0;inner=[];shell=[];cursor=0;ledger=dict(nodes=0,bounds=0,point_tests=0,inside_blocks=0,outside_blocks=0,passes=1)
 while cursor<len(nodes) and p<k:
  begin,end,escape=nodes[cursor];vals=signs[begin:end];ledger['nodes']+=1;ledger['bounds']+=1
  # Abstract sound certificates, not the numerical power_bounds expression.
  if all(v>0 for v in vals):ledger['outside_blocks']+=1;cursor=escape
  elif all(v<0 for v in vals):
   ledger['inside_blocks']+=1;added=min(end-begin,k-p)
   if fill:inner.extend(range(begin,begin+added))
   p+=added;cursor=escape
  elif end-begin<=leaf:
   for i in range(begin,end):
    if p==k:break
    ledger['point_tests']+=1
    if signs[i]<0:
     if fill:inner.append(i)
     p+=1
    elif signs[i]==0 and keep_shell:
     if fill:shell.append(i)
     m+=1
   cursor=escape
  else:cursor+=1
 return p,m,inner,shell,ledger

def reserve(used,limit,parts,deny=None):
 before=used;allocations=0
 if sum(parts)>limit-used:return False,before,allocations
 for part in parts:
  if not part:continue
  if allocations==deny:return False,before,allocations
  used+=part;allocations+=1
 return True,used,allocations

def derive():
 checks=0
 def check(ok,msg):
  nonlocal checks
  need(ok,msg);checks+=1
 rows=[];sizes=[]
 for leaf in range(1,257):
  for n in range(1,257):
   expected,depth=recurrence(n,leaf);actual=quotient(n,leaf)
   check(actual==expected,'quotient vs independent recurrence')
   check(depth==((n-1)//leaf).bit_length()+1,'closed depth formula')
 for leaf in (1,2,3,8,16,31,256):
  edges={1,MAX_N,(1<<31)-1,1<<31,(1<<31)+1,10_000_000,30_000_000,50_000_000}
  for power in range(33):
   for offset in (-1,0,1):
    n=leaf*(1<<power)+offset
    if 1<=n<=MAX_N:edges.add(n)
  for n in sorted(edges):
   count,depth=recurrence(n,leaf)
   check(quotient(n,leaf)==count,'u32 virtual boundary count')
   check(1<=count<=2*n-1<1<<33 and count*128<1<<40,'u64 nodes/bytes bounds')
   check(1<=depth<=33 and depth==((n-1)//leaf).bit_length()+1,'u32 virtual boundary depth')
   sizes.append((n,leaf,count,depth))
 for n in (10_000_000,30_000_000,50_000_000):
  for leaf in (1,8,256):
   count,depth=recurrence(n,leaf);cloud=28*n+8
   for t in (1,8):
    total=cloud+40*count+4*t*n
    rows.append({'sites':n,'leaf_size':leaf,'nodes':count,'depth':depth,'node_bytes_40_conditional':40*count,'node_bytes_128_published_guard_upper':128*count,'complete_outputs_alive':t,'complete_ids_safe_upper':4*t*n,'Cloud_unit_bytes':cloud,'subtotal_at_required_bench_layout40':total,'subtotal_GB':total/10**9,'subtotal_GiB':total/2**30,'scope':'Conditional on node sizeof40 required by bench judge; no native sizeof measurement. Excludes catalogue/FULL/input/allocator metadata/RSS. Upper capacities are not attained large-shell fixtures.'})
 query_cases=0
 for n in range(1,6):
  for leaf in (1,3,8):
   nodes=nodes_small(n,leaf);check(len(nodes)==recurrence(n,leaf)[0],'small preorder node capacity')
   for begin,end,escape in nodes:check(0<=begin<end<=n and 1<=escape<=len(nodes),'small begin/end/escape bounds')
   for signs in itertools.product((-1,0,1),repeat=n):
    I=[i for i,s in enumerate(signs) if s<0];U=[i for i,s in enumerate(signs) if s==0]
    for k in sorted({1,2,n,n+1,(1<<32)-1}):
     first=walk(signs,nodes,leaf,k,True,False);p,m=first[:2];sat=p==k
     shell_size=0 if sat else m;second=walk(signs,nodes,leaf,k,not sat,True);p2,m2,inner,shell,l2=second
     check(p==p2 and shell_size==m2 and len(inner)==p and len(shell)==shell_size,'count/fill exact owned capacities')
     check(inner==sorted(set(inner)) and shell==sorted(set(shell)) and not set(inner)&set(shell),'sorted distinct disjoint IDs')
     check((sat and p==k and set(inner)<=set(I) and not shell) or (not sat and p<k and inner==I and shell==U),'exclusive strict witness or complete census')
     check(p+shell_size<=n and 4*(p+shell_size)<1<<34,'population and bytes bounds, even u32 max threshold')
     check(first[4]['passes']+l2['passes']==2 and first[4]['point_tests']+l2['point_tests']<=2*n,'cumulative two-pass ledger')
     check(all(first[4][key]+l2[key]<=2*len(nodes) for key in ('nodes','bounds','inside_blocks','outside_blocks')),'per-query bounded counters')
     query_cases+=1
 # Common budget, first full census retained, next exact response refused without losing it.
 base=400;limit=471;good,held,calls=reserve(base,limit,(12,24));check(good and held==436 and calls==2,'first complete exact reservation')
 refused,after,calls=reserve(held,limit,(12,24));check(not refused and after==held,'retained result makes second response refuse cleanly')
 sat,held_two,calls=reserve(held,limit,(4,0));check(sat and held_two==440 and calls==1,'retained full plus saturated coexist')
 check(held_two-36-4==base,'release order restores preexisting reservations')
 failures=[]
 for deny in (0,1):
  good,after,calls=reserve(held,1000,(12,24),deny);check(not good and after==held,'failure restores only call delta');failures.append({'deny_allocation':deny,'preexisting_retained_bytes':held,'after_refusal':after,'successful_new_allocations_before_refusal':calls})
 src=ROOT/'sources/morsehgp3D_v11';h=(src/'src/index/index.hpp').read_text();build=(src/'src/index/build.cpp').read_text();census=(src/'src/index/census.cpp').read_text()
 check('Cloud cloud_;' in h and 'Buffer<index_detail::Node> nodes_;' in h,'GlobalIndex owns Cloud and nodes')
 check(build.rfind('std::move(cloud)')>build.index('if (!built.ok()) return built.outcome();'),'Cloud transfer only after Builder success')
 check('u64 escape;' in h and 'u64 nodes()' in h,'global node indices are wide')
 check('const u64 shell_size = saturated ? 0 : count.m;' in census,'saturated shell is never allocated')
 check('result.interior_.allocate(count.p, budget)' in census and 'result.shell_.allocate(shell_size, budget)' in census,'exact two population buffers')
 check('Pass fill{threshold, !saturated' in census,'fill suppresses abandoned shell')
 check('MemoryBudget query_budget(4096);' in (src/'tests/index/unit.cpp').read_text(),'native concurrency declaration uses private budgets')
 check('node_bytes\']) is int and index[\'node_bytes\'] == 40' in (src/'bench/index_semantic.py').read_text(),'40-byte Node is bench requirement, not measured here')
 return {'status':'PASS','checks':checks,'node_small_pairs':256*256,'virtual_boundary_pairs':len(sizes),'query_model_cases':query_cases,'max_nodes_virtual':max(x[2] for x in sizes),'max_depth_virtual':max(x[3] for x in sizes),'memory_model':{'base':base,'limit':limit,'one_full_held':held,'two_results_held':held_two,'injected_failures':failures},'capacity_table':rows,'scope':'Independent scalar recurrence/cardinality/abstract certified-sign and budget states, not product execution, universal power-bound proof or native/G4/massive qualification'}
if __name__=='__main__':print(json.dumps(derive(),sort_keys=True,indent=2))
