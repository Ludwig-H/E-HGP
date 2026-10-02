from pathlib import Path
import json,hashlib
from trace import model
R=Path(__file__).resolve().parent
checks=0

def require(ok,message):
 global checks
 checks+=1
 if not ok:raise RuntimeError(message)

before=json.loads((R/'SOURCE_BEFORE.json').read_text())
require(before['commit']=='9df77494732b03ddf11dbcf1dcb11d96bef54a3b','source pin')
require(before['source_status']==[],'clean source at freeze')
for e in before['entries']:
 base='sources' if e['group']=='product_commit' else 'wip_docs'
 b=(R/base/e['path']).read_bytes()
 require(hashlib.sha256(b).hexdigest()==e['sha256'],e['group']+' '+e['path']+' hash')
 require(len(b)==e['size'],e['path']+' size')
 require(e['double_read_live_stable'],e['path']+' stable capture')
 if e['path'].startswith(('morsehgp3D_v11/src/num/','morsehgp3D_v11/src/catalogue/')):require(e['live_matches_snapshot_before'],e['path']+' source pin versus live')
actual=json.loads(json.dumps(model()));saved=json.loads((R/'TRACE.json').read_text())
require(actual==saved,'abstract analytic trace replay')
bad,good=actual['fixtures']
require(bad['fixture']['affine_determinant']!=0 and bad['fixture']['level']=='12','outside fixture')
require(not bad['fixture']['strictly_inside'] and bad['fixture']['owner_root'],'outside owner/hull')
require(bad['first_pass']['emitted']==0 and bad['first_pass']['judged']==0 and bad['logical_q4_levels']==0,'inside reject before judged/level')
require(actual['missing_inside_mutant']['emitted']==1,'omitted inside mutant witness')
require(good['fixture']['strictly_inside'] and good['first_pass']['emitted']==1,'positive fixture')
for row in actual['fixtures']:
 require(row['first_pass']==row['second_pass'],'same two abstract passes')
 require(row['actual_two_pass_q4_levels']==2*row['logical_q4_levels'],'per-pass level metric')
plan=json.loads((R/'API_PLAN.json').read_text())
require(all(not c['needs_level'] for c in plan['consumers'][:4]),'prepublication consumers need no Level')
require(plan['ordered_catalogue_stages'].index('strictly_inside')<plan['ordered_catalogue_stages'].index('judged increment'),'inside stage order')
require(plan['ordered_catalogue_stages'].index('q4 Level materialization')+1==plan['ordered_catalogue_stages'].index('Collector.accept guards/copy/commit'),'level before accept')
if (R/'SOURCE_AFTER.json').exists():
 after=json.loads((R/'SOURCE_AFTER.json').read_text());require(after['snapshot_intact'],'source snapshot after')
 require(len(after['entries'])==len(before['entries']),'dependency closure')
if (R/'SHA256SUMS').exists():
 for line in (R/'SHA256SUMS').read_text().splitlines():
  digest,path=line.split('  ',1);require(hashlib.sha256((R/path).read_bytes()).hexdigest()==digest,'closed '+path)
print(json.dumps({'status':'PASS','checks':checks,'scope':'snapshots and two exact tiny fixtures/abstract traces only; no candidate port or native qualification'},sort_keys=True))
