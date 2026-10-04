#!/usr/bin/env python3
"""Verify immutable local ledger and replay its pure Python helper."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root = Path(__file__).resolve().parent
seen = set()
for line in (root/'SHA256SUMS').read_text().splitlines():
    digest, rel = line.split('  ', 1)
    path = root/rel
    if rel in seen or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise ValueError('ledger mismatch: '+rel)
    seen.add(rel)
actual = {str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p != root/'SHA256SUMS'}
if actual != seen:
    raise ValueError('ledger coverage mismatch')
normal=(root/'normal.json').read_bytes()
if normal != (root/'optimized.json').read_bytes():
    raise ValueError('stored outputs differ')
for options in [[], ['-O']]:
    replay=subprocess.run([sys.executable]+options+['-B','-S',str(root/'check.py')],capture_output=True)
    if replay.returncode != 0 or replay.stdout != normal or replay.stderr:
        raise ValueError('portable pure replay mismatch')
result=json.loads(normal)
if result['checks'] != 237 or result['verdict'] != 'pass':
    raise ValueError('result floor')
print(json.dumps({'verdict':'pass','payloads':len(seen),'checks':237,'native_children':0},sort_keys=True))
