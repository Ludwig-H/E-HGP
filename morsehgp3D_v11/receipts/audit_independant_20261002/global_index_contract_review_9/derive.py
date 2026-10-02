"""Scalar protocol/capacity model; no index implementation or native execution."""
from pathlib import Path
import json,itertools
ROOT=Path(__file__).resolve().parent

def need(ok,msg):
 if not ok: raise ValueError(msg)
def morton(p):
 return sum(((p[a]>>b)&1)<<(3*b+a) for b in range(24) for a in range(3))
def side(p,c,r2): return sum((a-b)**2 for a,b in zip(p,c))-r2
def query(points,weights,c,r2,k,order,available=2**64-1):
 if not 1<=k<=12: return {'kind':'refused','reason':'kmax_out_of_range'}
 if any(w!=1 for w in weights): return {'kind':'refused','reason':'multiplicity_unsupported'}
 witness=[]; p=m=visits=0
 for i in order:
  visits+=1; s=side(points[i],c,r2)
  if s<0:
   p+=1; witness.append(i)
   if p==k: return {'kind':'saturated','k':k,'witness':sorted(witness),'first_visits':visits,'witness_payload_bytes':4*k,'second_visits':0}
  elif s==0: m+=1
 # Full discovery to EOF, exact cardinalities, then one exact allocation and a second fill.
 size=p+m; bytes_needed=4*size
 if bytes_needed>available: return {'kind':'refused','reason':'memory_budget','requested_bytes':bytes_needed}
 interior=[]; shell=[]
 for i in order:
  s=side(points[i],c,r2)
  if s<0: interior.append(i)
  elif s==0: shell.append(i)
 need(len(interior)==p and len(shell)==m,'count/fill disagreement')
 return {'kind':'complete','interior':sorted(interior),'shell':sorted(shell),'first_visits':visits,'second_visits':len(order),'query_ids_bytes':bytes_needed}
def valid(result,points,c,r2,k):
 if result['kind']=='saturated':
  w=result['witness']; return result['k']==k and len(w)==k and len(set(w))==k and w==sorted(w) and all(0<=i<len(points) and side(points[i],c,r2)<0 for i in w)
 if result['kind']=='complete':
  return result['interior']==[i for i,p in enumerate(points) if side(p,c,r2)<0] and result['shell']==[i for i,p in enumerate(points) if side(p,c,r2)==0]
 return result['kind']=='refused' and 'interior' not in result and 'shell' not in result and 'witness' not in result

def derive():
 checks=0
 def check(ok,msg):
  nonlocal checks
  need(ok,msg); checks+=1
 c=(5,5,5); shell=sorted([tuple(x+5 for x in p) for p in itertools.product(range(-5,6),repeat=3) if sum(x*x for x in p)==25],key=morton)
 check(len(shell)==30,'R5 exact shell')
 case_rows=[]
 for points,name in [(shell,'shell30'),(sorted(shell+[c],key=morton),'shell30_plus_one_interior')]:
  weights=[1]*len(points)
  interior=[i for i,p in enumerate(points) if side(p,c,25)<0]
  orders=[list(range(len(points))),list(reversed(range(len(points)))),[i for i in range(len(points)) if i not in interior]+interior]
  for k in (1,2,5,10,12):
   for ordinal,order in enumerate(orders):
    got=query(points,weights,c,25,k,order);check(valid(got,points,c,25,k),'typed protocol result')
    if got['kind']=='complete':
     check(got['query_ids_bytes']==4*(len(got['interior'])+len(got['shell'])),'exact owned capacity')
     refused=query(points,weights,c,25,k,order,got['query_ids_bytes']-1);check(valid(refused,points,c,25,k) and refused['kind']=='refused','clean memory refusal without prefix')
     altered=dict(got);altered['shell']=got['shell'][:-1];check(not valid(altered,points,c,25,k),'shell truncation detected')
    case_rows.append({'fixture':name,'k':k,'traversal':ordinal,'result':got})
 check(all(r['result']['kind']=='complete' and len(r['result']['shell'])==30 for r in case_rows if r['fixture']=='shell30'),'shell is not bounded by K')
 last=next(r['result'] for r in case_rows if r['fixture']=='shell30_plus_one_interior' and r['k']==1 and r['traversal']==2)
 check(last['first_visits']==31 and last['kind']=='saturated','late strict witness after 30 shell contacts')
 # Two strict sites: duplicating a witness is not a valid K=2 certificate.
 points=[(5,5,5),(5,5,6),(10,5,5)];got=query(points,[1,1,1],c,25,2,[0,1,2]);check(valid(got,points,c,25,2),'two distinct strict witnesses')
 bad=dict(got);bad['witness']=[0,0];check(not valid(bad,points,c,25,2),'duplicate witness rejected')
 check(query(points,[2,1,1],c,25,2,[0,1,2])=={'kind':'refused','reason':'multiplicity_unsupported'},'unit mode rejects weighted Cloud')
 # Output-empty census is valid, distinct from refusal and saturation.
 empty=query([(20,20,20)],[1],c,25,2,[0]);check(empty['kind']=='complete' and empty['query_ids_bytes']==0,'valid empty census')
 # Admission is not a reservation. Independent pilots can both pass but only one later allocation fits.
 limit,used,request=1000,400,500
 admissions=[request<=limit-used,request<=limit-used];first=used+request<=limit
 used_after=used+request if first else used
 second=used_after+request<=limit
 check(admissions==[True,True] and first and not second,'driver/quiescence obligation, not budget invariant failure')
 table=[]
 for n in (40_000,10_000_000,30_000_000,50_000_000):
  for t in (1,8):
   cloud=28*n+8; query_upper=4*t*n
   table.append({'sites':n,'workers_or_live_complete_outputs':t,'unit_cloud_bytes':cloud,'complete_query_ids_safe_upper_bytes':query_upper,'known_subtotal_bytes':cloud+query_upper,'known_subtotal_GB':(cloud+query_upper)/10**9,'known_subtotal_GiB':(cloud+query_upper)/2**30,'excludes':'index, catalogue, FULL, traversal/sort scratch, input, allocator metadata, RSS; safe reservation bound not attained fixture/performance'})
 before=json.loads((ROOT/'SOURCE_BEFORE.json').read_text());check(before['index_source_paths']==[],'no pinned index implementation')
 h=(ROOT/'sources/morsehgp3D_v11/src/cloud/cloud.hpp').read_text();buf=(ROOT/'sources/morsehgp3D_v11/src/core/buffer.hpp').read_text();cat=(ROOT/'sources/morsehgp3D_v11/src/catalogue/catalogue.cpp').read_text()
 check('Cloud(Cloud&& other) noexcept' in h and 'weight_(std::exchange(other.weight_, 0))' in h,'Cloud move empties source object')
 check('if (weight != 1) return fail(Reason::multiplicity_unsupported);' in cat,'published catalogue requires unit sites')
 check('Ce n\'est PAS une reservation' in buf and 'un pilote' in buf,'admit driver contract')
 return {'status':'PASS','scope':'Scalar protocol/capacity proposal, not an index implementation, native performance or massive qualification','checks':checks,'exact_shell_sites':len(shell),'cases':case_rows,'duplicate_witness_mutation_rejected':True,'weighted_cloud_refusal':True,'late_witness_after_shell':last,'empty_complete':empty,'admission_model':{'limit':limit,'used_initial':used,'request_each':request,'admissions':admissions,'first_allocation':first,'second_allocation':second},'capacity_table':table,'formulas':{'Cloud_persistent':'24*n + 4*N + 8; unit case N=n','Catalogue_persistent':'sizeof(CatalogueBall)*B + sizeof(Level)*L + 8*(B+1) + 4*P, L includes zero','Stage_peak':'max(previous_peak, max_t(U(t)+J(t)+Catalogue(t)+FULL(t)+Q_retained(t)+sum_j_live(S_j(t)+Q_j(t))))','Complete_query':'Q_j=4*(p_j+m_j)<=4*n, plus owned metadata and separately counted traversal/sort scratch','Saturated_query':'exactly K distinct strict SiteIdx; not exact p nor KNN; K-sized witness and traversal scratch','Capacity_bound_for_T_outputs':'4*T*n IDs bytes; safe upper bound, not an attained large geometric example'}}
if __name__=='__main__':print(json.dumps(derive(),sort_keys=True,indent=2))
