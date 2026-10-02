from pathlib import Path
import hashlib,json
from derive import derive
R=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 if not ok:raise ValueError(msg)
 checks+=1
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
rows=[]
for name in ('SOURCE_BEFORE.json','SOURCE_EXTRA_BEFORE.json','SOURCE_EXTRA_2_BEFORE.json'):
 obj=json.loads((R/name).read_text());rows+=obj['sources']
 for row in obj['sources']:
  p=R/'sources'/row['path'];need(sha(p)==row['sha256'],'snapshot source '+row['path']);need(row['live_matches_pin'],'initial LIVE '+row['path'])
need(len(rows)==77 and len({r['path'] for r in rows})==77,'dependency closure')
need(json.loads((R/'SOURCE_BEFORE.json').read_text())['pin']=='e8520481d1745627e156723ad995ac5175a8163f','pin')
b=json.loads((R/'BASELINE_CONTRACTS.json').read_text())
for row in b['sources']:
 need(sha(R/'baseline_contracts'/row['path'])==row['sha256']==row['declared_sha256'] and row['matches'],'baseline contract '+row['path'])
model=derive();need(model==json.loads((R/'MODEL.json').read_text()),'model replay');need((R/'MODEL.json').read_bytes()==(R/'MODEL_opt.json').read_bytes(),'normal optimized model')
for name in ('derive.stderr','derive_opt.stderr'):need((R/name).read_bytes()==b'','model stderr')
env=json.loads((R/'ENVIRONMENT.json').read_text());need(not env['native_executed'] and not env['GCP_used'] and not env['massive_allocation'],'scope')
a=R/'SOURCE_AFTER.json'
if a.exists():
 obj=json.loads(a.read_text());need(len(obj['sources'])==77,'after count')
 for row in obj['sources']:
  need(sha(R/'sources'/row['path'])==row['snapshot_sha256'],'closed snapshot '+row['path']);need(row['live_matches_snapshot']==(row['live_sha256']==row['snapshot_sha256']),'after classification')
m=R/'SHA256SUMS'
if m.exists():
 for line in m.read_text().splitlines():
  digest,rel=line.split('  ',1)
  if sha(R/rel)!=digest:raise ValueError('closure '+rel)
print(json.dumps({'status':'PASS','receipt_checks':checks,'model_checks':model['checks'],'source_count':len(rows),'scope':'Pinned e852 source audit and autonomous scalar models; no native/FULL/G4 qualification'},sort_keys=True))
