#!/usr/bin/env python3
"""Hash-first STATIC archive reader; never executes archived sources."""
import argparse
import datetime
import hashlib
import json
import math
import os
import re
import stat
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT=Path(__file__).absolute().parent
SOURCES=('README.txt','er_snapshot.py','r1_snapshot.py','probe.py','read.py','record.py','origin.json','protocol.json')
FILES=set(SOURCES)
CAPFILES={'run_receipt.json'}|{mode+suffix for mode in ('normal','optimized') for suffix in ('.command.json','.stdout.json','.stderr.bin','.receipt.json')}
PINS={'er_snapshot.py':'99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d',
      'r1_snapshot.py':'debe1ec3eff15ccbb5965b4e29ccc2f648c5fb26f777605c89301c25b9e5f00f'}
SCOPE='exact RAM LCA cancellation proposal; immutable ER99e8 and R1 AST snapshots only; no native/Scene/GCP/GPU or geometrical realizability claim'
PROBE_PREDECESSOR='8416513270022cdd0c5bb23cc2dc6f4bb9bd2bd5f4c4e396dad7b4cb6e7297a7'

def require(c,m):
 if not c: raise ValueError(m)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def digest(v): return type(v) is str and re.fullmatch('[0-9a-f]{64}',v) is not None
def pairs(items):
 out={}
 for k,v in items:
  require(k not in out,'duplicate JSON key'); out[k]=v
 return out
def bad_constant(v): raise ValueError('nonfinite JSON constant '+v)
def parse(raw):
 obj=json.loads(raw,object_pairs_hook=pairs,parse_constant=bad_constant)
 def walk(v):
  if type(v) is float: require(math.isfinite(v),'nonfinite parsed float')
  elif type(v) is list:
   for child in v: walk(child)
  elif type(v) is dict:
   for child in v.values(): walk(child)
 walk(obj); return obj
def keys(v,expected,where): require(type(v) is dict and set(v)==set(expected),'exact keys: '+where)
def same(actual,expected,where):
 require(type(actual) is type(expected),'exact type: '+where)
 if type(expected) is dict:
  keys(actual,expected,where)
  for k,v in expected.items(): same(actual[k],v,where+'.'+k)
 elif type(expected) is list:
  require(len(actual)==len(expected),'exact length: '+where)
  for i,v in enumerate(expected): same(actual[i],v,where+'['+str(i)+']')
 else: require(actual==expected,'exact value: '+where)

def inspect_root(base,expected):
 require('..' not in base.parts and os.path.normpath(str(base))==str(base),'root lexical path')
 for p in (base,)+tuple(base.parents):
  s=p.lstat(); require(stat.S_ISDIR(s.st_mode),'root/ancestor must be nonsymlink directory')
 entries=list(base.iterdir())
 require({p.name for p in entries}==set(expected),'closed REAL inventory')
 for p in entries: require(stat.S_ISREG(p.lstat().st_mode),'regular nonsymlink payload: '+p.name)

def original_lca(parent,a,b):
 # Independent finite parent walks. Counts deliberately match the declared
 # diagnostic cost, not an O(1) LCA implementation.
 ancestors=[]; steps=0
 while a>=0: ancestors.append(a); a=parent[a]; steps+=1
 while b not in ancestors:
  require(b>=0,'LCA disconnected'); b=parent[b]; steps+=1
 return b,steps

def fixture_rows():
 trees=(('chain',(1,2,3),(1,2,-1)),('fork',(1,1,2,3),(2,2,3,-1)),
        ('fractional_fork',(1,1,F(3,2),F(5,2)),(2,2,3,-1)),
        ('two_forks',(1,1,1,1,2,2,3),(4,4,5,5,6,6,-1)))
 for name,b0,p0 in trees:
  birth=list(map(F,b0)); parent=list(p0); H=len(parent); root=parent.index(-1)
  children=[[v for v,p in enumerate(parent) if p==u] for u in range(H)]
  leaves=[v for v,ch in enumerate(children) if not ch]
  internal=next(v for v,ch in enumerate(children) if ch and v!=root)
  death=[None if p<0 else birth[p] for p in parent]
  for mode in range(5):
   raw=[(i,leaves[i%len(leaves)],birth[leaves[i%len(leaves)]]) for i in range(6)]
   if mode==0: raw=[(i,v,(birth[v]+death[v])/2) for i,v,_c in raw]
   if mode in (1,3,4): raw.extend((0,v,birth[v]) for v in leaves)
   if mode==2:
    raw=[s for s in raw if s[0] not in (4,5)]
    raw.extend(((4,internal,(birth[internal]+death[internal])/2),(5,root,birth[root]+F(1,3))))
   if mode==3:
    raw=[s for s in raw if s[0] not in (4,5)]
    raw.extend(((4,leaves[0],death[leaves[0]]),(5,internal,death[internal])))
   if mode==4:
    raw=[s for s in raw if s[0] not in (1,4,5)]
    raw.extend(((1,internal,(birth[internal]+death[internal])/2),
                (4,leaves[0],death[leaves[0]]),(5,root,birth[root]+F(1,3))))
   if mode==1: raw.append((0,leaves[0],(birth[leaves[0]]+death[leaves[0]])/2))
   yield name,mode,birth,parent,children,root,death,raw

def expected_output():
 cases=[]; comparisons=allB=allF=sums=caps=clamps=0; same_owner=targetB=targetF=0
 for name,mode,birth,parent,children,root,death,raw in fixture_rows():
  H=len(parent); seeds=[]; moved=steps=0
  for p,v,c in raw:
   old=v; require(c>=birth[v],'raw seed before birth')
   while death[v] is not None and c>=death[v]: v=parent[v]; steps+=1
   require(c>=birth[v] and (death[v] is None or c<death[v]),'true normalized life')
   moved+=v!=old; seeds.append((p,v,c))
  early=[[] for _ in parent]; late=[[] for _ in parent]
  for seed in seeds:
   p,v,c=seed; (early if c==birth[v] else late)[v].append(seed)
  trace=[]; preorder=[]
  def visit(v):
   preorder.append(v); trace.extend(early[v])
   for ch in children[v]: visit(ch)
   trace.extend(late[v])
  visit(root)
  # Reference sets are expanded independently along persistent ancestor lives.
  coverage=[{} for _ in range(6)]
  for p,v,c in seeds:
   while True:
    coverage[p][v]=min(coverage[p].get(v,c),c)
    if parent[v]<0: break
    v=parent[v]; c=birth[v]
  require(all(coverage),'all point seeds complete')
  Bsets=[{p for p,row in enumerate(coverage) if v in row and row[v]<=birth[v]} for v in range(H)]
  Fsets=[{p for p,row in enumerate(coverage) if v in row and (death[v] is None or row[v]<death[v])} for v in range(H)]
  B=list(map(len,Bsets)); D=list(map(len,Fsets)); comparisons+=2*H
  # LCA targets are determined from an EXPLICIT augmented tree rather than
  # relying on the compressed owner/late test used by the proposal.
  ap=[-1]*(2*H+len(trace))
  for v,p in enumerate(parent): ap[2*v]=-1 if p<0 else 2*p+1; ap[2*v+1]=2*v
  for i,(_p,v,c) in enumerate(trace): ap[2*H+i]=2*v+(1 if c==birth[v] else 0)
  dB=list(map(len,early)); dF=list(map(len,late)); corrections=[]; original_steps=0
  last=[None]*6
  for i,(p,b,cb) in enumerate(trace):
   if last[p] is not None:
    j=last[p]; _p,a,ca=trace[j]
    aug,_unused=original_lca(ap,2*H+j,2*H+i)
    w,s=original_lca(parent,a,b); original_steps+=s
    require(aug//2==w,'augmented original owner')
    isF=aug%2==0
    require(isF==((a==w and ca>birth[w]) or (b==w and cb>birth[w])),'compressed target proof')
    (dF if isF else dB)[w]-=1
    corrections.append({'point':p,'left_occurrence':j,'right_occurrence':i,'owner_lca':w,'correct_target':'F' if isF else 'B'})
    targetF+=isF; targetB+=not isF
   last[p]=i
  require(len(corrections)==len(seeds)-6,'T-n exact correction count')
  require(dF==[len(Fsets[v]-Bsets[v]) for v in range(H)],'dF late-only labels')
  prefix=[0]
  for v in preorder: prefix.append(prefix[-1]+dB[v]+dF[v])
  require(all(abs(x)<=len(seeds) for x in prefix),'signed prefix bound')
  descendants=[]
  for v in range(H):
   row=set(); stack=[v]
   while stack:
    u=stack.pop(); row.add(u); stack.extend(children[u])
   descendants.append(row)
  def collapsed(db,df):
   deaths=[sum(db[u]+df[u] for u in descendants[v]) for v in range(H)]
   return [deaths[v]-df[v] for v in range(H)],deaths
  require(collapsed(dB,dF)==(B,D),'Euler/subtree exact count')
  def bad(which):
   db=list(map(len,early)); df=list(map(len,late))
   for row in corrections: (db if which=='B' else df)[row['owner_lca']]-=1
   return collapsed(db,df)!=(B,D)
  killedB=bad('B'); killedF=bad('F'); allB+=killedB; allF+=killedF
  naiveB=[len({p for p,_w,_c in early[v]})+sum(D[ch] for ch in children[v]) for v in range(H)]
  naiveD=[naiveB[v]+len({p for p,_w,_c in late[v]}) for v in range(H)]
  killedSum=(naiveB,naiveD)!=(B,D); sums+=killedSum
  cap_errors=[[v,p] for v,p in enumerate(parent) if p>=0 and D[v]!=B[p] and min(D[v],2)==min(B[p],2)]
  caps+=bool(cap_errors)
  killedClamp=collapsed([max(0,x) for x in dB],dF)!=(B,D); clamps+=killedClamp
  for v in range(H): same_owner+=len({p for p,_w,_c in early[v]} & {p for p,_w,_c in late[v]})
  counts={'births':B,'deaths':D,'dB':dB,'dF':dF,'prefix':prefix,'corrections':corrections,
          'original_lca_queries':len(corrections),'original_parent_walk_steps':original_steps,
          'augmented_oracle_births':B,'augmented_oracle_deaths':D}
  cases.append({'tree':name,'mode':mode,'H':H,'T':len(seeds),'n':6,
                'redundant_same_owner_early_late_seed_added':mode==1,'normalized_moves':moved,
                'normalization_parent_steps':steps,'owned_seeds':[[p,v,str(c)] for p,v,c in seeds],
                'new_counts':counts,'Fenwick_counts_birth':B,'Fenwick_counts_death':D,
                'AST_counts_birth':B,'AST_counts_death':D,'all_B_mutant_rejected':killedB,
                'all_F_mutant_rejected':killedF,'sum_children_mutant_rejected':killedSum,
                'mcs2_cap_false_equalities':cap_errors,'signed_clamp_mutant_rejected':killedClamp})
 require((len(cases),comparisons,targetB,targetF,allB,allF,sums,caps,clamps)==(20,180,27,4,4,12,10,17,5),'causal/nonvacuity census')
 require(same_owner>=4,'same-owner earlylate nonvacuity')
 return {'scope':SCOPE,'snapshot_pins':PINS,'cases':cases,'endpoint_comparisons':comparisons,
         'same_owner_early_late_labels':same_owner,'correction_targets_B':targetB,'correction_targets_F':targetF,
         'all_B_mutants_rejected':allB,'all_F_mutants_rejected':allF,'sum_children_mutants_rejected':sums,
         'mcs2_cap_mutants_rejected':caps,'signed_clamp_mutants_rejected':clamps,'native_calls':0,'GCP':'unused'}

def canonical_absolute(v,where):
 require(type(v) is str and Path(v).is_absolute() and str(Path(v))==v and os.path.normpath(v)==v,'absolute lexical path: '+where)
 return Path(v)

def utc(v):
 require(type(v) is str and re.fullmatch(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,6})?Z',v),'real UTC syntax')
 d=datetime.datetime.fromisoformat(v[:-1]+'+00:00')
 require(d.utcoffset()==datetime.timedelta(0),'UTC offset'); return d

def verify_tree(base,pin,expected):
 require(digest(pin),'external manifest SHA syntax')
 inspect_root(base,set(expected)|{'SHA256SUMS'})
 raw=(base/'SHA256SUMS').read_bytes()
 require(hashlib.sha256(raw).hexdigest()==pin,'external manifest SHA BEFORE parsing payloads')
 listed={}
 for line in raw.decode('ascii').splitlines():
  h,name=line.split('  ',1)
  require(digest(h) and name in expected and name not in listed,'manifest syntax/name/duplicate')
  listed[name]=h
 require(set(listed)==set(expected),'manifest inventory')
 for name,h in listed.items(): require(sha(base/name)==h,'payload SHA '+name)
 return listed

def verify_package(pin):
 listed=verify_tree(ROOT,pin,FILES)
 for name,h in PINS.items(): require(listed[name]==h,'snapshot pin '+name)
 origin=parse((ROOT/'origin.json').read_bytes())
 keys(origin,('schema','prepared_at_utc','predecessor','sources','previous_candidate'),'origin')
 same(origin['schema'],'er_distinct_lca_preparation_v2','origin schema'); utc(origin['prepared_at_utc'])
 keys(origin['predecessor'],('root','probe_sha256','status'),'predecessor')
 canonical_absolute(origin['predecessor']['root'],'predecessor root')
 same(origin['predecessor']['probe_sha256'],PROBE_PREDECESSOR,'predecessor probe')
 same(origin['predecessor']['status'],'OPEN diagnostic; independently replayed by root, not this prepared source','predecessor status')
 keys(origin['previous_candidate'],('root','status','probe_sha256'),'previous candidate')
 same(origin['previous_candidate']['root'],'/tmp/er-distinct-lca-recordable.KuqGt1hx','R1 candidate origin')
 same(origin['previous_candidate']['status'],'prepared OPEN, not closed or executed; left unchanged','R1 candidate status')
 same(origin['previous_candidate']['probe_sha256'],'3f57df80ecdbaff923b56a2eea18031892a6544c9a639429fa73c5ce31b6f27e','R1 candidate probe pin')
 keys(origin['sources'],PINS,'source provenance')
 for name,h in PINS.items():
  row=origin['sources'][name]; keys(row,('copied_from','sha256'),'snapshot provenance')
  canonical_absolute(row['copied_from'],'snapshot origin'); same(row['sha256'],h,'snapshot provenance pin')
 protocol=parse((ROOT/'protocol.json').read_bytes())
 expected_protocol={'schema':'er_distinct_lca_protocol_v2','scope':SCOPE,'snapshot_pins':PINS,
                    'R1_AST_definitions':['Tree','owned_seeds','full_profile_oracle','intervals','distinct_counts','fixtures'],
                    'ER_AST_classes':['StructureER'],'cases':20,'endpoint_cardinalities':180,'n_per_case':6,
                    'mode1_extension':'one redundant own late seed of point0 on its birth-covered leaf',
                    'timeout_seconds':15,'managed_signals':['SIGINT','SIGTERM','SIGHUP','SIGQUIT'],
                    'collector_protocol':'immutable checkpoint and per-attempt streams/receipt; atomic aggregate; any managed interruption refuses qualification',
                    'limits':['abstract persistent RAM profiles, not a geometric realization',
                              'slow parent-walk original LCA, not a linear preprocessing implementation',
                              'normalized COMPLETE owners are a premise, not a qualified FULL export',
                              'no native/Scene/GCP/GPU/100ms claim','reader static, no replay, no LIVE dependency']}
 same(protocol,expected_protocol,'protocol')
 return listed,sha(ROOT/'protocol.json')

def validate_python(e):
 keys(e,('path','sha256','version','version_info','implementation'),'Python provenance')
 canonical_absolute(e['path'],'Python binary')
 require(digest(e['sha256']) and type(e['version']) is str and e['version'] and type(e['implementation']) is str and e['implementation'],'Python metadata')
 vi=e['version_info']
 require(type(vi) is list and len(vi)==5 and all(type(vi[i]) is int for i in (0,1,2,4)) and
         type(vi[3]) is str and vi[3] in ('alpha','beta','candidate','final'),'Python version_info')

def verify_capture(cap,cap_pin,source_pin,listed,protocol_sha):
 verify_tree(cap,cap_pin,CAPFILES)
 r=parse((cap/'run_receipt.json').read_bytes())
 keys(r,('schema','source_manifest_sha','origin','protocol_sha256','timeout_seconds','runs','interrupted','first_signal','collection_complete'),'receipt')
 same(r['schema'],'er_distinct_lca_run_receipt_v2','receipt schema'); same(r['source_manifest_sha'],source_pin,'source authority')
 same(r['protocol_sha256'],protocol_sha,'protocol authority'); same(r['timeout_seconds'],15,'receipt timeout')
 same(r['interrupted'],False,'NO managed interruption'); same(r['first_signal'],None,'NO signal')
 same(r['collection_complete'],True,'complete collection')
 o=r['origin']; keys(o,('package_root','source_files','manifest','executable'),'capture origin')
 source_root=canonical_absolute(o['package_root'],'source origin')
 same(o['source_files'],{n:{'path':str(source_root/n),'sha256':listed[n]} for n in FILES},'origin source inventory')
 same(o['manifest'],{'path':str(source_root/'SHA256SUMS'),'sha256':source_pin},'manifest origin')
 py=o['executable']; validate_python(py)
 require(type(r['runs']) is list and len(r['runs'])==2,'two runs')
 expected=expected_output(); raws=[]
 for i,name in enumerate(('normal','optimized')):
  cp_raw=(cap/(name+'.command.json')).read_bytes(); cp=parse(cp_raw)
  argv=[py['path'],'-B']+(['-O'] if i else [])+[str(source_root/'probe.py')]
  keys(cp,('schema','index','mode','argv','cwd','timeout_seconds','checkpoint_utc','source_before','python_before','first_signal'),'checkpoint')
  same(cp['schema'],'er_distinct_lca_command_v2','checkpoint schema'); same(cp['index'],i,'checkpoint index')
  same(cp['mode'],name,'checkpoint mode'); same(cp['argv'],argv,'checkpoint EXACT argv'); same(cp['cwd'],str(source_root),'checkpoint cwd')
  same(cp['timeout_seconds'],15,'checkpoint timeout'); same(cp['source_before'],listed,'checkpoint source pins')
  same(cp['python_before'],py['sha256'],'checkpoint Python'); same(cp['first_signal'],None,'checkpoint no signal')
  checkpoint_time=utc(cp['checkpoint_utc'])
  run_raw=(cap/(name+'.receipt.json')).read_bytes(); run=parse(run_raw)
  same(r['runs'][i],run,'immutable attempt == aggregate entry')
  keys(run,('schema','index','mode','checkpoint_file','checkpoint_sha256','argv','cwd','timeout_seconds',
            'launched','timed_out','exit_code','launch_error','integrity_error','judge_performed','judge_error',
            'start_utc','end_utc','duration_seconds','stdout_file','stdout_sha256','stderr_file','stderr_sha256',
            'source_before','source_after','python_before','python_after','first_signal_at_receipt_creation'),'attempt')
  same(run['schema'],'er_distinct_lca_attempt_v2','attempt schema'); same(run['index'],i,'attempt index')
  same(run['checkpoint_file'],name+'.command.json','checkpoint file')
  same(run['checkpoint_sha256'],hashlib.sha256(cp_raw).hexdigest(),'checkpoint bytes pin')
  same(run['mode'],name,'mode'); same(run['cwd'],str(source_root),'cwd'); same(run['argv'],argv,'EXACT argv')
  same(run['timeout_seconds'],15,'timeout'); same(run['launched'],True,'actual launch')
  same(run['timed_out'],False,'no timeout'); same(run['exit_code'],0,'terminal code0')
  same(run['launch_error'],None,'no launch error'); same(run['integrity_error'],None,'no integrity error')
  same(run['judge_performed'],True,'independent post-run judge performed'); same(run['judge_error'],None,'no semantic judge error')
  same(run['first_signal_at_receipt_creation'],None,'no attempt signal')
  same(run['source_before'],listed,'source before'); same(run['source_after'],listed,'source after')
  same(run['python_before'],py['sha256'],'Python before'); same(run['python_after'],py['sha256'],'Python after')
  t=run['duration_seconds']; require(type(t) in (int,float) and math.isfinite(t) and 0<=t<=15.25,'finite duration within timeout')
  start,end=utc(run['start_utc']),utc(run['end_utc'])
  require(checkpoint_time<=start<=end and abs((end-start).total_seconds()-t)<=0.25,'checkpoint before actual collection/real UTC duration')
  same(run['stdout_file'],name+'.stdout.json','stdout file'); same(run['stderr_file'],name+'.stderr.bin','stderr file')
  raw=(cap/(name+'.stdout.json')).read_bytes(); stderr=(cap/(name+'.stderr.bin')).read_bytes()
  same(run['stdout_sha256'],hashlib.sha256(raw).hexdigest(),'stdout hash')
  same(run['stderr_sha256'],hashlib.sha256(stderr).hexdigest(),'stderr hash'); require(stderr==b'','empty RAW stderr')
  same(parse(raw),expected,'independent counts/fixtures/causal outcomes'); raws.append(raw)
 require(raws[0]==raws[1],'normal/-O stdout byte identity')
 verify_package(source_pin); verify_tree(cap,cap_pin,CAPFILES)

def main():
 p=argparse.ArgumentParser(); p.add_argument('--source-manifest-sha',required=True)
 p.add_argument('--capture'); p.add_argument('--capture-manifest-sha'); a=p.parse_args()
 listed,protocol_sha=verify_package(a.source_manifest_sha)
 if a.capture is None:
  require(a.capture_manifest_sha is None,'capture SHA without capture')
  verify_package(a.source_manifest_sha)
  print('source_package_verified; NO probe/record executed; no capture evidence')
 else:
  require(a.capture_manifest_sha is not None,'external capture manifest required')
  verify_capture(canonical_absolute(a.capture,'capture'),a.capture_manifest_sha,a.source_manifest_sha,listed,protocol_sha)
  print('capture_verified; 20 RAM profiles, 180 cardinalities, 31 corrections, five causal controls; static, no native/GCP/replay')

if __name__=='__main__':
 try: main()
 except (ValueError,OSError,UnicodeError,TypeError,KeyError,IndexError,StopIteration,RecursionError) as e:
  print('REFUSE: '+str(e),file=sys.stderr); sys.exit(2)
