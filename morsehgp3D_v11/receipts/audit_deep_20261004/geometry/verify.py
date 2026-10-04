#!/usr/bin/env python3
"""Closed receipt reader; no native/product import, no live dependency."""
from pathlib import Path
import hashlib
import json

root=Path(__file__).resolve().parent
checks=0

def need(value, reason):
    global checks
    checks+=1
    if not value:
        raise RuntimeError(reason)

expected={}
for line in (root/'SHA256SUMS').read_text().splitlines():
    digest,path=line.split('  ',1)
    need(path not in expected,'duplicate ledger path')
    need(not Path(path).is_absolute() and '..' not in Path(path).parts,'unsafe ledger path')
    expected[path]=digest
actual={p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file() and p.relative_to(root).as_posix()!='SHA256SUMS'}
need(actual==set(expected),'ledger file inventory differs')
for path,digest in expected.items():
    need(hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,'ledger hash differs '+path)
for manifest in ('SOURCE_BEFORE.json','SOURCE_V10_BEFORE.json'):
    for path,info in json.loads((root/manifest).read_text())['files'].items():
        need(hashlib.sha256((root/info['copy']).read_bytes()).hexdigest()==info['sha256'],'snapshot differs '+path)
a=(root/'staging_normal.json').read_bytes()
b=(root/'staging_optimized.json').read_bytes()
need(a==b,'optimized output differs')
result=json.loads(a)
need(result['status']=='PASS' and result['checks']==1733,'wrong proof result')
need(result['native_runs']==result['fits']==result['gcp_actions']==0,'scope differs')
after=json.loads((root/'SOURCE_AFTER.json').read_text())
need(after['copied_sources_unchanged'] and after['git_pins_unchanged'],'source closure failed')
need(after['product_source_live_changed']==[],'product drift concealed')
print(json.dumps({'status':'PASS','proof_checks':result['checks'],'source_dependencies':249,'native_runs':0,'fits':0,'gcp_actions':0},sort_keys=True))
