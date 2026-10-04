#!/usr/bin/env python3
"""Read-only receipt closure and Python replay; no native tool."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
expected = {}
for line in (root / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    if name in expected or name == 'SHA256SUMS':
        raise SystemExit('duplicate or self hash')
    expected[name] = digest
actual = {str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p.name != 'SHA256SUMS'}
if actual != set(expected):
    raise SystemExit('inventory differs')
for name, digest in expected.items():
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('hash mismatch: ' + name)
a = (root / 'checks.normal.json').read_bytes()
if a != (root / 'checks.optimized.json').read_bytes():
    raise SystemExit('normal optimized differ')
if (root / 'checks.normal.stderr').read_bytes() or (root / 'checks.optimized.stderr').read_bytes():
    raise SystemExit('stderr not empty')
args = [sys.executable, '-B', '-S']
if sys.flags.optimize:
    args.append('-O')
run = subprocess.run(args + [str(root / 'check_pipeline_timing.py')], capture_output=True, check=False)
if run.returncode or run.stderr or run.stdout != a:
    raise SystemExit('replay differs')
answer = json.loads(a)
print(json.dumps({'status': 'PASS', 'payloads': len(actual), 'checks': answer['checks'],
                  'normal_optimized_identical': True, 'replay_identical': True}, sort_keys=True))
