#!/usr/bin/env python3
"""Exhaustive closed receipt validator; no live/product dependency."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parent

def need(ok,why):
 if not ok:raise RuntimeError(why)

ledger={}
for line in (r/'SHA256SUMS').read_text().splitlines():
 digest,path=line.split('  ',1)
 need(path not in ledger and not Path(path).is_absolute() and '..' not in Path(path).parts,'bad ledger path')
 ledger[path]=digest
actual={p.relative_to(r).as_posix() for p in r.rglob('*') if p.is_file() and p.relative_to(r).as_posix()!='SHA256SUMS'}
need(actual==set(ledger),'inventory differs')
for p,h in ledger.items():need(hashlib.sha256((r/p).read_bytes()).hexdigest()==h,'payload differs '+p)
for name in ('SOURCE_BEFORE.json','BASE_SOURCE.json'):
 for p,e in json.loads((r/name).read_text())['files'].items():need(hashlib.sha256((r/e['copy']).read_bytes()).hexdigest()==e['sha256'],'source differs '+p)
a=json.loads((r/'SOURCE_AFTER.json').read_text())
need(a['copies_unchanged'] and a['target_mutant_unchanged'] and a['target_gate_declaration_unchanged'],'target closure differs')
for p,e in a['files'].items():need(hashlib.sha256((r/e['live_copy']).read_bytes()).hexdigest()==e['live_sha256_after'],'AFTER differs '+p)
x=(r/'abandon_normal.json').read_bytes();y=(r/'abandon_optimized.json').read_bytes()
need(x==y,'optimized result differs')
proof=json.loads(x)
need(proof['status']=='PASS' and proof['checks']==45,'model result differs')
need(proof['native_runs']==proof['fits']==proof['gcp_actions']==0,'scope differs')
print(json.dumps({'status':'PASS','proof_checks':45,'native_runs':0,'fits':0,'gcp_actions':0},sort_keys=True))
