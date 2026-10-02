from pathlib import Path
import hashlib,json
from capacity import capacity
R=Path(__file__).resolve().parent
checks=0
def need(ok,msg):
 global checks
 if not ok:raise ValueError(msg)
 checks+=1
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((R/'SOURCE_BEFORE.json').read_text())
need(b['developer_head']=='25792084eb4e672c5222d62f5b2ae87bd2ee4948','developer HEAD, distinct from WIP')
need(b['own_head']=='ab04bc7b1ef61b9996eb7ec15db4fd4e5f2d511a','own before HEAD')
need(len(b['sources'])==67 and len({x['path'] for x in b['sources']})==67,'snapshot inventory67')
for row in b['sources']:
 need(sha(R/'sources'/row['path'])==row['sha256'],'snapshot '+row['path'])
 need(row['matches_head_blob']==(row['tracked_at_head'] and row['head_blob_sha256']==row['sha256']),'before classification '+row['path'])
wip={x['path'] for x in b['sources'] if not x['matches_head_blob']}
need(len(wip)==11,'WIP inventory11')
for rel in ('src/tower/full_domain.cpp','src/tower/full_domain.hpp','src/catalogue/leaf.cpp','src/catalogue/catalogue.hpp','src/num/center_region.cpp','src/num/center_region.hpp'):need('morsehgp3D_v11/'+rel in wip,'explicit WIP '+rel)
model=capacity();need(model==json.loads((R/'CAPACITY.json').read_text()),'independent capacity replay')
need((R/'CAPACITY.json').read_bytes()==(R/'CAPACITY_opt.json').read_bytes(),'capacity normal/-O identical')
need(model['max_lookup_slots']==1<<33 and model['max_lookup_bytes']==1<<35,'upper bound')
for row in model['samples']:
 need(row['lookup_fits_below_released_emission_bytes'],'upstream admission proof sample')
 if row['catalogue_balls']>0:need(row['lookup_bytes']<row['temporary_emissions_proven_lower_bytes'],'strict released bound')
for name in ('capture.stderr','capacity.stderr','capacity_opt.stderr'):need((R/name).read_bytes()==b'','stderr '+name)
env=json.loads((R/'ENVIRONMENT.json').read_text())
for key in ('native_executed','product_built','GCP_used','massive_allocation','product_modules_imported'):need(env[key] is False,'scope '+key)
a=json.loads((R/'SOURCE_AFTER.json').read_text());need(len(a['sources'])==67,'after initial inventory')
for row in a['sources']:
 need(sha(R/'sources'/row['path'])==row['snapshot_sha256'],'closed snapshot '+row['path'])
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
 if set(entries)!=actual:raise ValueError('exhaustive root-only closure')
 for rel,digest in entries.items():
  if sha(R/rel)!=digest:raise ValueError('closed payload '+rel)
print(json.dumps({'status':'PASS','checks':checks,'sources':67,'initial_WIP_paths':11,'scalar_capacity_cases':model['scalar_cases'],'scope':'WIP ownership/capacity source review and autonomous scalar proof only; no native/FULL/pruning qualification'},sort_keys=True))
