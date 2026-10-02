from pathlib import Path
import hashlib,json
p=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 checks+=1
 if not ok:raise RuntimeError(msg)
for mode in ('normal','ubsan'):
 run=json.loads((p/('RUN_'+mode+'.json')).read_text())
 need(len(run['commands'])==7,'native coverage count')
 for c in run['commands']:need(c['returncode']==c['expected_returncode'],'native return '+c['stdout'])
 need(not (p/('compile_'+mode+'.stderr')).read_bytes(),'compile stderr')
 for name in ('budget','baseline','mutable','reset'):
  need(not (p/(mode+'_'+name+'.stderr')).read_bytes(),'native stderr')
 budget=json.loads((p/(mode+'_budget.stdout')).read_text())
 need(budget=={'status':'PASS','cross_budget_move':True,'destructive_refusal_documented':True,'survives_budget_owner':True},'bounded ownership controls')
 baseline=json.loads((p/(mode+'_baseline.stdout')).read_text())
 need(baseline['destructor_allocations']==0 and baseline['abc_present'] and not baseline['dbc_present'],'stable timer control')
 mutable=json.loads((p/(mode+'_mutable.stdout')).read_text())
 need(mutable['destructor_allocations']==1 and mutable['abc_present'] and mutable['dbc_present'],'mutable name new key')
 reset=json.loads((p/(mode+'_reset.stdout')).read_text())
 need(reset['destructor_allocations']==1 and reset['abc_present'] and not reset['dbc_present'],'reset ledger new node')
 for name in ('mutable_fault','reset_fault'):
  need((p/(mode+'_'+name+'.stdout')).read_bytes()==b'','fault stdout')
  need((p/(mode+'_'+name+'.stderr')).read_bytes()==b'stage_timer_destructor_called_terminate_after_injected_bad_alloc\n','destructor terminate observed')
for name in ('budget','baseline','mutable','reset','mutable_fault','reset_fault'):
 for suffix in ('stdout','stderr'):
  need((p/('normal_'+name+'.'+suffix)).read_bytes()==(p/('ubsan_'+name+'.'+suffix)).read_bytes(),'normal UBSan output equality')
before=json.loads((p/'SOURCE_BEFORE.json').read_text());after=json.loads((p/'SOURCE_AFTER.json').read_text())
need(len(before['files'])==19,'snapshot coverage')
need(before['git_status_named_scope'].find('?? morsehgp3D_v11/src/core/')>=0,'initial uncommitted provenance')
for e in before['files']:
 need(hashlib.sha256((p/'sources'/e['path']).read_bytes()).hexdigest()==e['sha256'],'frozen source hash')
need(after['ledger_body_live_matches_snapshot'],'target ledger body still same at closing read')
need(not after['stable'],'live drift explicitly preserved')
print(json.dumps({'status':'PASS','checks':checks,'native_cases_per_mode':6,'scope':'Frozen WIP core; actual StageTimer allocation/terminate cases, no current moving-source integration claim'}))
