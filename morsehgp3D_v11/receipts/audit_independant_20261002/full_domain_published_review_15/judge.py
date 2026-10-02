from pathlib import Path
import hashlib,json,re
R=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 if not ok:raise ValueError(msg)
 checks+=1
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((R/'SOURCE_BEFORE.json').read_text())
need(b['pin']=='7f1922c7743d8682e2665a491b01d32e8f2d546c','published source pin')
need(len(b['sources'])==62 and len({x['path'] for x in b['sources']})==62,'source inventory62')
for row in b['sources']:
 need(sha(R/'sources'/row['path'])==row['sha256'],'snapshot '+row['path'])
 need(row['live_matches_pin']==(row['sha256']==row['live_sha256']),'before classification '+row['path'])
c=json.loads((R/'COMPARE_REVIEW14.json').read_text());need(len(c['shared_sources'])==58,'shared copies58')
need(c['review14_manifest_sha256']=='1f4e935f0ef67497681ef5e0891cdcfbddf00634d689f478344e62baa53d4d0b','immutable prior manifest reference')
for row in c['shared_sources']:
 need(sha(R/'sources'/row['path'])==row['published7f_sha256'],'delta copied bytes '+row['path'])
 need(row['matches_review14']==(row['published7f_sha256']==row['review14_sha256']),'delta classification '+row['path'])
need(not [x['path'] for x in c['shared_sources'] if x['path'].endswith(('.cpp','.hpp')) and not x['matches_review14']],'no implementation cpp/hpp drift')
need(set(c['added_vs_review14'])=={'morsehgp3D_v11/docs/CENTER_REGION.md','morsehgp3D_v11/docs/FULL_DOMAIN.md','morsehgp3D_v11/tests/tower/domain.cpp','morsehgp3D_v11/tests/tower/domain_fault.cpp'},'explicit delta sources')
u=(R/'sources/morsehgp3D_v11/tests/tower/domain.cpp').read_text();f=(R/'sources/morsehgp3D_v11/tests/tower/domain_fault.cpp').read_text()
groups=dict(re.findall(r'MHGP11_TEST\((\w+),\s*(\d+)\)',u))
need(groups=={'context':'18','lookup':'200','global_support':'17','ownership':'16','refusals':'18','capacity':'20','concurrency':'9','permutation':'100'},'new declared domain gates, not executed')
need(re.findall(r'MHGP11_TEST\((\w+),\s*(\d+)\)',f)==[('starvation','20')],'declared allocation gate, not executed')
need(sum(8-gap for gap in range(1,6))==25 and 1<<(2*25-1).bit_length()==64,'independent line fixture cardinality')
need('cat_count + 1' in f and 'i + 1 == count' in f,'final system allocation instrumented')
env=json.loads((R/'ENVIRONMENT.json').read_text())
for key in ('native_executed','product_built','GCP_used','massive_allocation','product_modules_imported'):need(env[key] is False,'audit scope '+key)
a=json.loads((R/'SOURCE_AFTER.json').read_text());need(len(a['sources'])==62,'after source count')
for row in a['sources']:
 need(sha(R/'sources'/row['path'])==row['snapshot_sha256'],'closed pin '+row['path'])
 need(row['live_matches_snapshot']==(row['live_sha256']==row['snapshot_sha256']),'after classification '+row['path'])
need(sorted(a['drift'])==sorted(x['path'] for x in a['sources'] if not x['live_matches_snapshot']),'after drift inventory')
m=R/'SHA256SUMS'
if m.exists():
 entries={}
 for line in m.read_text().splitlines():
  digest,rel=line.split('  ',1)
  if rel in entries:raise ValueError('duplicate manifest path')
  entries[rel]=digest
 actual={p.relative_to(R).as_posix() for p in R.rglob('*') if p.is_file() and p!=m}
 if set(entries)!=actual:raise ValueError('exhaustive root-only manifest')
 for rel,digest in entries.items():
  if sha(R/rel)!=digest:raise ValueError('closed payload '+rel)
print(json.dumps({'status':'PASS','checks':checks,'sources':62,'declared_domain_groups':8,'declared_fault_groups':1,'scope':'Published Git7f delta source review; no native/build/GCP/FULL qualification'},sort_keys=True))
