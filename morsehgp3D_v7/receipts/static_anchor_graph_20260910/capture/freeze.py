#!/usr/bin/env python3
"""Freeze independent rational audit helpers without editing their owner files."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_static_anchor_graph_20260910'
AUDIT = ROOT / 'morsehgp3D_v7/audits'
(BASE / 'geometry').mkdir(exist_ok=False)
pins = {}
for name in ['plateau_model.py', 'ball_anchor_model.py']:
    source = AUDIT / 'receipts_plateaux_full_20260906' / name
    pins[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    shutil.copy2(source, BASE / 'geometry' / name)
source = AUDIT / 'meb_rational_oracle_20260905.py'
pins[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
shutil.copy2(source, BASE / source.name)
model = BASE / 'geometry/plateau_model.py'
before = model.read_text()
old = '2 <= len(points) <= 7'
if before.count(old) != 1:
    raise RuntimeError('bounded model guard location changed')
# Only the finite test input bound is extended, not any product representation.
model.write_text(before.replace(old, '2 <= len(points) <= 10'))
(BASE / 'source_pins.json').write_text(json.dumps({'originals': pins,
    'private_model_sha256': hashlib.sha256(model.read_bytes()).hexdigest(),
    'mechanical_change': 'private Model bound 7 -> 10 only; main is never called'}, indent=2) + '\n')
