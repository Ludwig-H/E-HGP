#!/usr/bin/env python3
"""Create-only real-census judge overlay; no product or source receipt edits."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
BASE = REPO / 'build/v7_terminal_batch_20260911/o2_r5'
TESTS = REPO / 'build/v7_census_tower_permanent_20260911/candidate/morsehgp3D_v7/tests'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    receipt = json.loads((BASE / 'receipt.json').read_text())
    if receipt['status'] != 'passed' or not receipt['snapshot_stable'] or not receipt['sources_stable']:
        raise RuntimeError('unclosed base')
    for name, pin in receipt['sources_before'].items():
        if sha(BASE / 'source_snapshot' / name) != pin:
            raise RuntimeError('base drift')
    shutil.copytree(BASE / 'source_snapshot/prototype', ROOT / 'prototype')
    imported = {}
    for name in ('census_tower_gate.cpp', 'census_tower_oracle.hpp'):
        path = ROOT / 'prototype/source/morsehgp3D_v7/tests' / name
        with path.open('xb') as out:
            out.write((TESTS / name).read_bytes())
        imported[name] = sha(path)
    with (ROOT / 'import.json').open('x') as out:
        json.dump(dict(base_receipt_sha256=sha(BASE / 'receipt.json'),
                       base_sources=receipt['sources_before'], relocated_test_pins=imported,
                       inherited_results=False, gcp_used=False), out, indent=2, sort_keys=True)
        out.write('\n')


if __name__ == '__main__':
    main()
