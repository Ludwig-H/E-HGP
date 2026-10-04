#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parent
seen=set()
for line in (root/'SHA256SUMS').read_text().splitlines():
    sha,name=line.split('  ',1)
    path=root/name
    if name in seen or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=sha:
        raise ValueError('ledger mismatch '+name)
    seen.add(name)
actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=root/'SHA256SUMS'}
if actual!=seen: raise ValueError('ledger not exhaustive')
raw=(root/'normal.json').read_bytes()
if raw!=(root/'optimized.json').read_bytes(): raise ValueError('stored replay differs')
for flags in [[],['-O']]:
    child=subprocess.run([sys.executable]+flags+['-B','-S',str(root/'check.py')],capture_output=True)
    if child.returncode or child.stderr or child.stdout!=raw: raise ValueError('portable replay differs')
result=json.loads(raw)
if result['checks']!=6852 or result['verdict']!='pass': raise ValueError('result floor')
print(json.dumps(dict(verdict='pass',payloads=len(seen),checks=6852,native=False),sort_keys=True))
