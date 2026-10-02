from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent
checks=0

def require(condition,message):
 global checks
 checks+=1
 if not condition:raise RuntimeError(message)

before=json.loads((R/'SOURCE_BEFORE.json').read_text())
require(before['commit']=='9d639e1460e7a75b9af5340bc1fc436ea00c1d07','source pin')
require(before['working_changes']==[],'clean num at capture')
for e in before['entries']:
 b=(R/'sources'/e['path']).read_bytes()
 require(hashlib.sha256(b).hexdigest()==e['sha256'],e['path']+' hash')
 require(len(b)==e['size'],e['path']+' size')
 require(e['live_matches_commit_before'] and e['double_read_live_stable'],e['path']+' capture stability')
pins=json.loads((R/'PINS_VERIFICATION.json').read_text())
require(pins['baseline_commit']=='3e7b52b430deaa115464672d27e3b81cbc8d371e','baseline pin')
for e in pins['entries']:
 b=(R/'baseline_sources'/e['path']).read_bytes()
 require(hashlib.sha256(b).hexdigest()==e['actual_sha256']==e['declared_sha256'],e['path']+' baseline hash')
 require(len(b)==e['size'] and e['matches'],e['path']+' baseline size')
s=json.loads((R/'STATIC_INSPECTION.json').read_text());num=R/'sources/morsehgp3D_v11'
m=json.loads((num/'tests/mutants/num.json').read_text())
require(len(m['mutants'])==s['mutant_floor']==13,'mutant floor')
require(len(s['mutants'])==13,'mutant inventory')
for item in m['mutants']:
 require((num/item['fichier']).read_text().count(item['cherche'])==1,item['id']+' unique source pattern')
 declared=next(e for e in s['mutants'] if e['id']==item['id'])
 require(declared['options']==item.get('options',[]) and declared['gate']==item['porte'],item['id']+' gate/options')
for b,native,wide in [(18,[1,2,3,4],[]),(21,[1,2,4],[3]),(24,[1,2,4],[3])]:
 row=next(v for v in s['branches'] if v['coord_bits']==b)
 require(row['side_bits']==6*b+8 and row['native_presentation_arities']==native and row['wide_presentation_arities']==wide,'branch contract '+str(b))
configs={c['name']:c['cmake_options'] for c in s['configs']}
for name,bits in [('gcc_release',18),('mutants',18),('gcc_asan_ubsan',24),('gcc_tsan',21),('bits21',21),('bits24',24),('poison',21)]:
 require('-DMHGP11_COORD_BITS='+str(bits) in configs[name],'explicit matrix profile '+name)
if (R/'SOURCE_AFTER.json').exists():
 after=json.loads((R/'SOURCE_AFTER.json').read_text())
 require(after['snapshot_intact'],'snapshot after')
 require(len(after['entries'])==len(before['entries']),'dependency closure')
if (R/'SHA256SUMS').exists():
 for line in (R/'SHA256SUMS').read_text().splitlines():
  digest,path=line.split('  ',1)
  require(hashlib.sha256((R/path).read_bytes()).hexdigest()==digest,'closed '+path)
print(json.dumps({'status':'PASS','checks':checks,'scope':'hashes, baseline pins and static inventories; no native qualification'},sort_keys=True))
