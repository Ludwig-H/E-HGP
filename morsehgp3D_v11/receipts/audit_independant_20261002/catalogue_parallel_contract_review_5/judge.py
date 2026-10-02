from pathlib import Path
import hashlib,json
from model import models
R=Path(__file__).resolve().parent
checks=0

def require(condition,message):
 global checks
 checks+=1
 if not condition:raise RuntimeError(message)

before=json.loads((R/'SOURCE_BEFORE.json').read_text())
for entry in before['entries']:
 b=(R/'sources'/entry['group']/entry['path']).read_bytes()
 require(hashlib.sha256(b).hexdigest()==entry['sha256'],entry['path']+' source hash')
 require(len(b)==entry['size'],entry['path']+' size')
require(before['product_commit']=='e6fe34cb082f19d0041c829dfb38ea249319ab19','product pin')
require(before['f391_e6_src_diff']==[],'src diff')
expected=json.loads((R/'MODEL.json').read_text());actual=json.loads(json.dumps(models()))
require(actual==expected,'analytic model replay')
o=actual['overlap'];s=actual['scatter']
require(o['root']['sites']==[0,1,2,3,4],'root fixture')
require(o['left']['sites']==[0,1,2] and o['right']['sites']==[2,3,4],'children fixture')
require(o['logical_children']>o['n'] and o['children_capacity']==10,'nonpartition/capacities')
require(s['completion_orders']==24 and s['naive_rejected'],'scatter permutations/base mutant')
require(s['u64_boundary_exact'] and s['u64_overflow_refused'],'virtual u64 prefixes')
require([r[2] for r in s['canonical']]==[1,1,2,2,3],'global exact plateaus')
a=json.loads((R/'ABLATION.json').read_text())['rows']
require(len(a)==4,'ablation completed rows')
for row in a:
 require(row['generation_passes']==2,'two passes')
 require((row['balls'],row['levels'],row['incidences'],row['peak_reserved_bytes'])==(597998,597987,2895136,133416208),'same capacities/output counts')
 require(row['canonical_sha256_declared']=='2671f84acd61597300af06e7f164b0c8cd552b726bf7519711c8772d3febf74a','same declared canonical hash')
require(a[0]['logical_one_pass']['prefixes']==144086254,'leaf32 prefixes')
require(all(v['logical_one_pass']['prefixes']==85489590 for v in a[1:]),'leaf16 prefixes')
if (R/'SOURCE_AFTER.json').exists():
 after=json.loads((R/'SOURCE_AFTER.json').read_text())
 require(after['snapshot_intact'],'snapshot after')
 require(len(after['entries'])==len(before['entries']),'after dependency count')
if (R/'SHA256SUMS').exists():
 for line in (R/'SHA256SUMS').read_text().splitlines():
  digest,path=line.split('  ',1)
  if not (path.startswith('reader_postclosure.') or path.startswith('reader_opt_postclosure.')):
   require(hashlib.sha256((R/path).read_bytes()).hexdigest()==digest,'closure '+path)
print(json.dumps({'status':'PASS','checks':checks,'scope':'static snapshots and tiny analytic models only; no native qualification'},sort_keys=True))
