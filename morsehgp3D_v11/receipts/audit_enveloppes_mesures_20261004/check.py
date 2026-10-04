#!/usr/bin/env python3
"""Read-only closed receipt and bounded Python replays; no native or cloud."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
manifest = root / 'SHA256SUMS'
expected = {}
for line in manifest.read_text().splitlines():
    digest, name = line.split('  ', 1)
    if name in expected or name == 'SHA256SUMS':
        raise SystemExit('duplicate or self hash')
    expected[name] = digest
actual = {str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p != manifest}
if actual != set(expected):
    raise SystemExit('inventory differs')
for name, digest in expected.items():
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('hash mismatch: ' + name)

metadata = json.loads((root / 'REPLAYS.json').read_text())
if metadata['native_runs'] or metadata['fits'] or metadata['gcp_actions']:
    raise SystemExit('unexpected execution scope')
checks = 0
for row in metadata['replays']:
    directory = root / row['directory']
    saved = (directory / row['output']).read_bytes()
    if saved != (directory / row['optimized']).read_bytes():
        raise SystemExit('normal and optimized differ: ' + row['directory'])
    command = [sys.executable, '-B', '-S']
    if sys.flags.optimize:
        command.append('-O')
    run = subprocess.run(command + [str(directory / row['script'])], capture_output=True)
    if run.returncode or run.stderr or run.stdout != saved:
        raise SystemExit('bounded replay differs: ' + row['directory'])
    count = json.loads(saved)['checks']
    if count != row['checks']:
        raise SystemExit('check count differs: ' + row['directory'])
    checks += count
if checks != metadata['checks_each_mode']:
    raise SystemExit('total differs')
print(json.dumps({'status': 'PASS', 'payloads': len(actual), 'bounded_checks_each_mode': checks,
                  'replays': len(metadata['replays']), 'normal_optimized_identical': True,
                  'native_runs': 0, 'fits': 0, 'gcp_actions': 0}, sort_keys=True))
