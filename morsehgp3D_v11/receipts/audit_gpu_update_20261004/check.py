#!/usr/bin/env python3
"""Closed inventory and source-pinned stdlib replays; no native or GPU call."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent

def verify_inventory(base):
    names = set()
    for line in (base / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        rel = Path(name)
        if rel.is_absolute() or '..' in rel.parts or name in names:
            raise ValueError('invalid inventory name')
        p = base / name
        if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest() != digest:
            raise ValueError('inventory mismatch: ' + name)
        names.add(name)
    actual = {str(p.relative_to(base)) for p in base.rglob('*')
              if p.is_file() and p != base / 'SHA256SUMS'}
    if actual != names:
        raise ValueError('inventory not exhaustive')
    return len(names)

payloads = verify_inventory(ROOT)
meta = json.loads((ROOT / 'SOURCE.json').read_text())
results = []
for child in meta['children']:
    base = ROOT / child['name']
    if hashlib.sha256((base / 'SHA256SUMS').read_bytes()).hexdigest() != child['inventory_sha256']:
        raise ValueError('child closure mismatch')
    verify_inventory(base)
    expected = (base / child['expected_output']).read_bytes()
    for flags in ([], ['-O']):
        run = subprocess.run([sys.executable, '-B', '-S'] + flags + [str(base / child['script'])], capture_output=True)
        if run.returncode or run.stderr or run.stdout != expected:
            raise ValueError('portable replay failed: ' + child['name'])
    result = json.loads(expected)
    if result['checks'] != child['checks']:
        raise ValueError('check count mismatch')
    results.append({'capsule': child['name'], 'checks': child['checks']})
print(json.dumps({'verdict': 'pass', 'payloads': payloads, 'children': results,
                  'checks': sum(c['checks'] for c in results), 'native_runs': 0, 'gcp_actions': 0}, sort_keys=True))
