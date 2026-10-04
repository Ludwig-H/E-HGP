#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parent
expected={}
for line in (root/'SHA256SUMS').read_text().splitlines():
    digest,name=line.split('  ',1)
    if name in expected or name=='SHA256SUMS':raise SystemExit('invalid inventory')
    expected[name]=digest
actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=root/'SHA256SUMS'}
if actual!=set(expected):raise SystemExit('inventory differs')
for name,digest in expected.items():
    if hashlib.sha256((root/name).read_bytes()).hexdigest()!=digest:raise SystemExit('hash differs '+name)
a=(root/'checks.normal.json').read_bytes()
if a!=(root/'checks.optimized.json').read_bytes() or (root/'checks.normal.stderr').read_bytes() or (root/'checks.optimized.stderr').read_bytes():raise SystemExit('outputs differ')
args=[sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
r=subprocess.run(args+[str(root/'check_bindings.py')],capture_output=True,check=False)
if r.returncode or r.stderr or r.stdout!=a:raise SystemExit('replay differs')
print(json.dumps({'status':'PASS','payloads':len(actual),'checks':json.loads(a)['checks']},sort_keys=True))
