#!/usr/bin/env python3
"""Freeze private baseline and candidate source trees without touching product."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import shutil

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PACKET = ROOT / 'morsehgp3D_v7/receipts/incremental_journal_prototype_20260911'

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    if (HERE / 'baseline').exists() or (HERE / 'candidate').exists():
        raise RuntimeError('new snapshot required')
    files = []
    for directory in ('src', 'oracle'):
        files.extend(p for p in (ROOT / 'morsehgp3D_v7' / directory).rglob('*') if p.is_file())
    for name in ('tests/full_ball_tower_gate.cpp', 'bench/full_ball_tower_probe.cpp',
                 'bench/full_gabriel_semantic_digest.hpp'):
        files.append(ROOT / 'morsehgp3D_v7' / name)
    before = {p.relative_to(ROOT).as_posix(): sha(p) for p in files}
    for kind in ('baseline', 'candidate'):
        for path in files:
            target = HERE / kind / path.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
    expected = {
        'full_coverage_certificate.hpp': 'b526b895238d454c7b5df6b8d40a7af8682551e71fb2582bcc8e016f0018650b',
        'full_coverage_incremental.hpp': '76885ecd317eaa6a4277a3545e18efd739209ca53cdf9f7c08f9007b33b57a12'}
    for name, digest in expected.items():
        path = PACKET / 'sources' / name
        if sha(path) != digest:
            raise RuntimeError('prototype pin:' + name)
        shutil.copyfile(path, HERE / 'candidate/morsehgp3D_v7/src/forest' / name)
    for name in ('README.md', 'prefix_gate.cpp', 'source_pins.json'):
        target = HERE / 'prefix_reference' / (name + ('.source' if name.endswith('.md') else ''))
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / 'morsehgp3D_v7/audits/receipts_incremental_prefix_20260911' / name, target)
    after = {p.relative_to(ROOT).as_posix(): sha(p) for p in files}
    if before != after:
        raise RuntimeError('active source changed while copying')
    (HERE / 'baseline_sources.json').write_text(json.dumps(before, indent=2, sort_keys=True) + '\n')
    (HERE / 'prototype_pins.json').write_text(json.dumps(expected, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'status': 'snapshots_prepared', 'source_files': len(files),
        'starting_commit': 'ce842a3f1c0d55786250b1deba85e7bd92c8807a', 'GCP_used': False}))

if __name__ == '__main__':
    main()
