from pathlib import Path
import hashlib,json,re
from capacity import capacity
R=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 if not ok:raise ValueError(msg)
 checks+=1
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((R/'SOURCE_BEFORE.json').read_text())
need(b['developer_head']=='24e4e04bc5adfcbdcd1bc7f6883ffa2773665bb7','developer pin')
need(b['own_head']=='4a3c91d42609612ce58342d30a2c467f08a3b4de','own pin')
need(len(b['sources'])==49 and len({x['path'] for x in b['sources']})==49,'source closure49')
for row in b['sources']:
 need(sha(R/'sources'/row['path'])==row['sha256'],'source '+row['path'])
 need(row['matches_head_blob'] and row['tracked_at_head'] and row['sha256']==row['head_blob_sha256'],'published pin '+row['path'])
pins=json.loads((R/'BASELINE_DEPENDENCIES.json').read_text());need(len(pins['sources'])==27,'reused dependency count')
for row in pins['sources']:need(sha(R/'sources'/row['path'])==row['snapshot_sha256']==row['baseline_sha256'] and row['matches_baseline'],'reused source '+row['path'])
c=capacity();need(c==json.loads((R/'CAPACITY.json').read_text()),'capacity replay')
need((R/'CAPACITY.json').read_bytes()==(R/'CAPACITY_opt.json').read_bytes(),'capacity normal/-O')
for row in c['local']:
 need(1<=row['presentations']<=793 and row['point_tests_upper']<=9516 and row['comparisons_upper']<=792,'bounded cardinality')
need(c['maximum_local']=={'local_sites':12,'presentations':793,'point_tests_upper':9516,'comparisons_upper':792},'maximum capacity')
need(c['one_global_response_ids_bytes_upper']==17179869176<1<<34,'global scalar capacity')
for name in ('capacity.stderr','capacity_opt.stderr','capture.stderr'):need((R/name).read_bytes()==b'','stderr '+name)
env=json.loads((R/'ENVIRONMENT.json').read_text())
for key in ('native_executed','product_built','GCP_used','massive_allocation','product_modules_imported'):need(env[key] is False,'scope '+key)
unit=(R/'sources/morsehgp3D_v11/tests/tower/unit.cpp').read_text()
groups=re.findall(r'MHGP11_TEST\((\w+),\s*(\d+)\)',unit)
need(dict(groups)=={'geometry':'175','local_support':'20','refusals':'24','ownership':'22','wrapper':'30','capacity':'18','shell':'10','concurrency':'10'},'declared unit gates (not executed)')
a=json.loads((R/'SOURCE_AFTER.json').read_text());need(len(a['sources'])==49,'after count')
for row in a['sources']:
 need(sha(R/'sources'/row['path'])==row['snapshot_sha256'],'after snapshot '+row['path'])
 need(row['live_matches_snapshot']==(row['live_sha256']==row['snapshot_sha256']),'after classification '+row['path'])
need(sorted(a['drift'])==sorted(x['path'] for x in a['sources'] if not x['live_matches_snapshot']),'drift inventory')
m=R/'SHA256SUMS'
if m.exists():
 entries={}
 for line in m.read_text().splitlines():
  digest,rel=line.split('  ',1)
  if rel in entries:raise ValueError('duplicate manifest path')
  entries[rel]=digest
 actual={p.relative_to(R).as_posix() for p in R.rglob('*') if p.is_file() and p!=m}
 if set(entries)!=actual:raise ValueError('exhaustive root-only manifest inventory')
 for rel,digest in entries.items():
  if sha(R/rel)!=digest:raise ValueError('manifest '+rel)
print(json.dumps({'status':'PASS','checks':checks,'sources':49,'reused_e852_dependencies':27,'scope':'Source review and analytical cardinalities only; no native/build/GCP/FULL qualification'},sort_keys=True))
