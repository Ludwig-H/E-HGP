#!/usr/bin/env python3
"""Closed portable receipt validator, no product import or live dependency."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parent

def need(ok,message):
 if not ok:raise RuntimeError(message)

files={}
for line in (r/'SHA256SUMS').read_text().splitlines():
 digest,path=line.split('  ',1)
 need(path not in files,'duplicate ledger path')
 need(not Path(path).is_absolute() and '..' not in Path(path).parts,'unsafe path')
 files[path]=digest
actual={p.relative_to(r).as_posix() for p in r.rglob('*') if p.is_file() and p.relative_to(r).as_posix()!='SHA256SUMS'}
need(set(files)==actual,'inventory differs')
for path,digest in files.items():need(hashlib.sha256((r/path).read_bytes()).hexdigest()==digest,'hash differs '+path)
for name in ('SOURCE_BEFORE.json','SOURCE_API_BEFORE.json'):
 for p,entry in json.loads((r/name).read_text())['files'].items():need(hashlib.sha256((r/entry['copy']).read_bytes()).hexdigest()==entry['sha256'],'source differs '+p)
a=(r/'q2_normal.json').read_bytes();b=(r/'q2_optimized.json').read_bytes()
need(a==b,'optimized output differs')
x=json.loads(a)
need(x['status']=='PASS' and x['checks']==633,'proof result differs')
need(x['native_runs']==x['fits']==x['gcp_actions']==0,'scope differs')
after=json.loads((r/'SOURCE_AFTER.json').read_text())
need(after['copies_and_git_unchanged'],'source closure failed')
need(after['v11_source_live_changed']==[],'v11 source drift')
print(json.dumps({'status':'PASS','proof_checks':633,'source_dependencies':11,'native_runs':0,'fits':0,'gcp_actions':0},sort_keys=True))
