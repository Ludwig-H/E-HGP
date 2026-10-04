#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parent
seen=set()
for line in (root/'SHA256SUMS').read_text().splitlines():
    digest,name=line.split('  ',1)
    path=root/name
    if name in seen or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise ValueError('ledger mismatch '+name)
    seen.add(name)
actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=root/'SHA256SUMS'}
if actual!=seen: raise ValueError('incomplete ledger')
normal=(root/'normal.json').read_bytes()
if normal!=(root/'optimized.json').read_bytes(): raise ValueError('stored outputs differ')
for flags in [[],['-O']]:
    replay=subprocess.run([sys.executable]+flags+['-B','-S',str(root/'check.py')],capture_output=True)
    if replay.returncode or replay.stderr or replay.stdout!=normal: raise ValueError('portable replay differs')
result=json.loads(normal)
if result['checks']!=3404 or result['verdict']!='pass': raise ValueError('result floor')
print(json.dumps(dict(verdict='pass',checks=3404,payloads=len(seen),native=False),sort_keys=True))
