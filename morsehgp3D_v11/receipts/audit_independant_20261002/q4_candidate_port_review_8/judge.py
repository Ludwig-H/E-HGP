"""Read-only closed receipt reader. No product/native subprocess."""
from pathlib import Path
import hashlib,json
from derive import derive
ROOT=Path(__file__).resolve().parent
checks=0

def need(condition,msg):
 global checks
 if not condition: raise ValueError(msg)
 checks+=1

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
before=json.loads((ROOT/'SOURCE_BEFORE.json').read_text())
need(before['head']=='ffc2ff95f0ae7296bdc522df81df34c58c3fdf47','incorrect source pin')
for row in before['sources']:
 need(row['capture_stable'],'unstable initial snapshot '+row['path'])
 need(sha(ROOT/'sources'/row['path'])==row['sha256'],'snapshot changed '+row['path'])
extra=json.loads((ROOT/'SOURCE_EXTRA_BEFORE.json').read_text())
for row in extra['sources']: need(sha(ROOT/'sources'/row['path'])==row['sha256'],'extra snapshot changed')
baseline=json.loads((ROOT/'BASELINE.json').read_text())
need(baseline['commit']=='9df77494732b03ddf11dbcf1dcb11d96bef54a3b','baseline pin')
for row in baseline['files']: need(sha(ROOT/'baseline9df'/row['path'])==row['sha256'],'baseline snapshot changed')
pins=json.loads((ROOT/'PIN_CHECK.json').read_text())
need(pins['commit']==before['head'] and len(pins['checks'])==len(before['sources']),'incorrect pin checks')
for row in pins['checks']: need(row['captured_matches_pin'],'snapshot did not match committed source')
review=derive()
need(review==json.loads((ROOT/'REVIEW.json').read_text()),'rational/source replay changed')
need((ROOT/'REVIEW.json').read_bytes()==(ROOT/'REVIEW_opt.json').read_bytes(),'normal/optimized output differs')
for name in ('derive.stderr','derive_opt.stderr'): need((ROOT/name).read_bytes()==b'','unexpected own error')
env=json.loads((ROOT/'ENVIRONMENT.json').read_text())
need(env['native_executed'] is False and env['GCP_used'] is False,'scope changed')
after=ROOT/'SOURCE_AFTER.json'
if after.exists():
 a=json.loads(after.read_text()); need(len(a['checks'])==len(before['sources'])+len(extra['sources']),'closure dependency count')
 for row in a['checks']: need(row['captured_sha256']==sha(ROOT/'sources'/row['path']),'closure captured hash changed')
 need(a['product_drift']==[row['path'] for row in a['checks'] if row['changed'] and '/src/' in row['path']],'incorrect source drift summary')
manifest=ROOT/'SHA256SUMS'
if manifest.exists():
 for line in manifest.read_text().splitlines():
  digest,rel=line.split('  ',1)
  need(sha(ROOT/rel)==digest,'closed payload changed '+rel)
print(json.dumps({'status':'PASS','receipt_checks':checks,'model_checks':review['checks'],'scope':'static snapshot/body comparison and bounded Fraction models; no native qualification'},sort_keys=True))
