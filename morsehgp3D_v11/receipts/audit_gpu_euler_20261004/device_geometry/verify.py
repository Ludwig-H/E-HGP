#!/usr/bin/env python3
"""Closure plus independent bounded pure replay; no C++/CUDA tool."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parent
expected={}
for line in (root/'SHA256SUMS').read_text().splitlines():
    digest,name=line.split('  ',1)
    if name in expected or name=='SHA256SUMS':
        raise SystemExit('duplicate or self hash')
    expected[name]=digest
actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=root/'SHA256SUMS'}
if actual!=set(expected):
    raise SystemExit('inventory differs')
for name,digest in expected.items():
    if hashlib.sha256((root/name).read_bytes()).hexdigest()!=digest:
        raise SystemExit('hash mismatch '+name)
a=(root/'checks.normal.json').read_bytes()
if a!=(root/'checks.optimized.json').read_bytes() or (root/'checks.normal.stderr').read_bytes() or (root/'checks.optimized.stderr').read_bytes():
    raise SystemExit('outputs differ or stderr nonempty')
args=[sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
r=subprocess.run(args+[str(root/'check_device_geometry.py')],capture_output=True,check=False)
if r.returncode or r.stderr or r.stdout!=a:
    raise SystemExit('replay differs')
print(json.dumps({'status':'PASS','payloads':len(actual),'checks':json.loads(a)['checks'],
                  'normal_optimized_identical':True,'replay_identical':True},sort_keys=True))
