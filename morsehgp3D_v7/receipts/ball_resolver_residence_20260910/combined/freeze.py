#!/usr/bin/env python3
"""Physical source closures for the active combined delta."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_combined_resolver_20260910'
PROJECT = ROOT / 'morsehgp3D_v7'
kind = sys.argv[1]
dest = BASE / kind
dest.mkdir(exist_ok=False)
for name in ['src', 'oracle']:
    shutil.copytree(PROJECT / name, dest / name)
for name in ['tests', 'bench']:
    (dest / name).mkdir()
gates = ['full_ball_tower_gate.cpp', 'full_ball_work_gate.cpp', 'full_coverage_certificate_gate.cpp']
if kind != 'baseline':
    gates += ['facet_resolver_cache_gate.cpp']
for name in gates:
    shutil.copy2(PROJECT / 'tests' / name, dest / 'tests' / name)
for name in ['full_ball_tower_probe.cpp', 'full_gabriel_semantic_digest.hpp']:
    shutil.copy2(PROJECT / 'bench' / name, dest / 'bench' / name)
pins = {str(p.relative_to(dest)): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted(dest.rglob('*')) if p.is_file()}
(BASE / (kind + '_original_pins.json')).write_text(json.dumps(pins, indent=2) + '\n')
print(json.dumps({key: val for key, val in pins.items() if key.endswith('full_ball_tower.hpp') or key.endswith('full_coverage_certificate.hpp')}))
