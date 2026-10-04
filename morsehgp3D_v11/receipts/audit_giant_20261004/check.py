#!/usr/bin/env python3
"""Exhaustive closure verification; stdlib only, no native or cloud."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
checks = 0


def need(condition, reason):
    global checks
    checks += 1
    if not condition:
        raise ValueError(reason)


def manifest(folder):
    expected = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        p = Path(name)
        need(not p.is_absolute() and '..' not in p.parts and name not in expected, 'manifest name')
        expected[name] = digest
    actual = {str(p.relative_to(folder)) for p in folder.rglob('*')
              if p.is_file() and p != folder / 'SHA256SUMS'}
    need(actual == set(expected), 'exhaustive manifest: ' + folder.name)
    for name, digest in expected.items():
        need(hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest, 'hash: ' + name)
    return len(expected)


total = manifest(HERE)
groups = {}
for name in ('foundations', 'geometry', 'mathematics', 'tower_evidence', 'g4_protocol'):
    groups[name] = manifest(HERE / name)
ledger = json.loads((HERE / 'LEDGER.json').read_text())
payloads = {r['path']: r for r in ledger['files']}
actual = {str(p.relative_to(HERE)) for p in HERE.rglob('*')
          if p.is_file() and p not in (HERE / 'LEDGER.json', HERE / 'SHA256SUMS')}
need(set(payloads) == actual, 'parent ledger exhaustive including nested manifests')
for name, row in payloads.items():
    p = HERE / name
    need(p.stat().st_size == row['bytes'] and hashlib.sha256(p.read_bytes()).hexdigest() == row['sha256'],
         'ledger bytes/hash: ' + name)
need(ledger['checks_per_mode'] == 16481 and ledger['native_runs'] == ledger['fits'] == ledger['gcp_actions'] == 0,
     'scope counters')
print(json.dumps(dict(verdict='conforme', checks=checks, parent_payloads=total, groups=groups), sort_keys=True))
