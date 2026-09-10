#!/usr/bin/env python3
"""Correct only a test-mutant type error, on the previous frozen baseline."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
HERE = Path(__file__).resolve().parent
OUT = HERE / 'run_r3_vertical'
OUT.mkdir(exist_ok=False)
shutil.copyfile(__file__, OUT / 'vertical_retry.py')
shutil.copytree(HERE / 'run_r2/baseline', OUT / 'baseline')
header = OUT / 'baseline/morsehgp3D_v7/src/forest/full_ball_tower.hpp'
old = 'upper.lower_nodes[root], cut, closed);'
text = header.read_text()
if text.count(old) != 1:
    raise RuntimeError('nonunique vertical mutation')
header.write_text(text.replace(old, 'upper.lower_nodes[root], ExactLevel{{0, 0, 0}, 1}, true);'))
def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
binary = OUT / 'wrong_vertical_cut.bin'
commands = []
for name, argv, expected in [
    ('compile', ['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
                 '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include'),
                 str(OUT / 'baseline/morsehgp3D_v7/tests/full_ball_tower_gate.cpp'), '-o', str(binary)], 0),
    ('run', [str(binary), '--selftest'], 1)]:
    started = time.time()
    with (OUT / (name + '.stdout')).open('xb') as stdout, (OUT / (name + '.stderr')).open('xb') as stderr:
        run = subprocess.run(argv, stdout=stdout, stderr=stderr, check=False)
    row = dict(argv=argv, exit_code=run.returncode, expected=expected, started_epoch=started,
               ended_epoch=time.time(), stdout_sha256=sha(OUT / (name + '.stdout')),
               stderr_sha256=sha(OUT / (name + '.stderr')))
    commands.append(row)
    print(name, run.returncode, flush=True)
    if run.returncode != expected:
        break
passed = len(commands) == 2 and all(r['exit_code'] == r['expected'] for r in commands)
receipt = dict(status='passed' if passed else 'failed', commands=commands,
               mutant_header_sha256=sha(header), binary_sha256=sha(binary) if binary.is_file() else None,
               frozen_source_inventory=sha(HERE / 'run_r2/sources_before.json'),
               live_sources_consumed=False, GCP_used=False)
with (OUT / 'receipt.json').open('x') as stream:
    json.dump(receipt, stream, indent=2)
raise SystemExit(0 if passed else 1)
