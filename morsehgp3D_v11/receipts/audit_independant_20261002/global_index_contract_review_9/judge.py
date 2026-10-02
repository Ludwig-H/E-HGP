from pathlib import Path
import hashlib,json
from derive import derive
ROOT=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 if not ok: raise ValueError(msg)
 checks+=1
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
before=json.loads((ROOT/'SOURCE_BEFORE.json').read_text())
need(before['pin']=='d0dc9cd8b8551e559ef8baa9fbcb1d8f2050dcbe','pin changed')
need(before['index_source_paths']==[],'absent-index claim changed')
for row in before['sources']:
 need(sha(ROOT/'sources'/row['path'])==row['sha256'],'snapshot changed '+row['path'])
 need(row['live_matches_pin'],'initial LIVE differed '+row['path'])
model=derive(); need(model==json.loads((ROOT/'MODEL.json').read_text()),'model replay differs')
need((ROOT/'MODEL.json').read_bytes()==(ROOT/'MODEL_opt.json').read_bytes(),'normal/optimized differs')
for name in ('derive.stderr','derive_opt.stderr'):need((ROOT/name).read_bytes()==b'','unexpected model error')
fail=json.loads((ROOT/'READ_COMMAND_FAILURE.json').read_text());need(fail['exit_code']==2 and len(fail['error_messages'])==2,'lost read lookup failure')
env=json.loads((ROOT/'ENVIRONMENT.json').read_text());need(not env['native_executed'] and not env['GCP_used'] and not env['massive_allocation'],'scope changed')
a=ROOT/'SOURCE_AFTER.json'
if a.exists():
 after=json.loads(a.read_text());need(len(after['checks'])==len(before['sources']),'dependency closure incomplete')
 for row in after['checks']:need(row['captured_sha256']==sha(ROOT/'sources'/row['path']),'closure snapshot hash')
 need(after['drift']==[row['path'] for row in after['checks'] if row['changed']],'drift summary not causal')
m=ROOT/'SHA256SUMS'
if m.exists():
 for line in m.read_text().splitlines():
  digest,path=line.split('  ',1);need(sha(ROOT/path)==digest,'closed payload changed '+path)
print(json.dumps({'status':'PASS','receipt_checks':checks,'model_checks':model['checks'],'scope':'Pinned plan/API review + scalar protocol/capacity models; no index/native/massive qualification'},sort_keys=True))
