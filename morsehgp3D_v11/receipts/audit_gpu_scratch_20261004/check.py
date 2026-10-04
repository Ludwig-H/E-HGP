#!/usr/bin/env python3
"""Closed inventories and portable source/snapshot readers; no native or cloud action."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parent

def inventory(base):
    seen = set()
    for line in (base / 'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        p = Path(name)
        if p.is_absolute() or '..' in p.parts or name in seen:
            raise ValueError('invalid inventory path')
        f = base / p
        if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest() != sha:
            raise ValueError('inventory mismatch: ' + str(p))
        seen.add(name)
    actual = {str(p.relative_to(base)) for p in base.rglob('*')
              if p.is_file() and p != base / 'SHA256SUMS'}
    if actual != seen:
        raise ValueError('inventory not exhaustive')
    return len(seen)

payloads = inventory(ROOT)
meta = json.loads((ROOT / 'SOURCE.json').read_text())
results = []
for child in meta['children']:
    base = ROOT / child['name']
    if hashlib.sha256((base / 'SHA256SUMS').read_bytes()).hexdigest() != child['inventory_sha256']:
        raise ValueError('closure mismatch')
    inventory(base)
    expected = (base / child['expected_output']).read_bytes()
    for flags in ([], ['-O']):
        run = subprocess.run([sys.executable, '-B', '-S'] + flags + [str(base / child['script'])], capture_output=True)
        if run.returncode or run.stderr or run.stdout != expected:
            raise ValueError('replay mismatch: ' + child['name'])
    derived = json.loads(expected)
    if child['scalar_checks'] is not None:
        if derived['checks'] != child['scalar_checks']:
            raise ValueError('check count mismatch')
    elif derived['metadata_files'] != meta['snapshot_metadata_files']:
        raise ValueError('snapshot count mismatch')
    results.append({'capsule': child['name'], 'scalar_checks': child['scalar_checks']})
print(json.dumps({'verdict': 'pass', 'payloads': payloads, 'scalar_checks': meta['scalar_checks'],
                  'snapshot_metadata_files': meta['snapshot_metadata_files'], 'children': results,
                  'native_runs': 0, 'gcp_actions': 0}, sort_keys=True))
