#!/usr/bin/env python3
"""Reconstruct audited sources from a Git pin + frozen patch, then run Python-only counter-check."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
repo = next((p for p in HERE.parents if (p / '.git').exists()), None)
if repo is None:
    raise SystemExit('Run this capsule inside the repository, with the captured Git pin available')
if sys.argv[1:] not in ([], ['--optimized']):
    raise SystemExit('usage: python3 replay.py [--optimized]')
manifest = json.loads((HERE / 'source_manifest.json').read_text())
with tempfile.TemporaryDirectory(prefix='mhgp11-attach-audit-') as work:
    base = pathlib.Path(work)
    snapshot = base / 'snapshot'
    snapshot.mkdir()
    for rel in sorted(manifest['files']):
        old = subprocess.run(['git', 'show', manifest['head'] + ':' + rel], cwd=repo, capture_output=True)
        if old.returncode == 0:
            target = snapshot / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(old.stdout)
    subprocess.run(['git', 'apply', '--check', str(HERE / 'source_delta.patch')], cwd=snapshot, check=True)
    subprocess.run(['git', 'apply', str(HERE / 'source_delta.patch')], cwd=snapshot, check=True)
    for rel, want in sorted(manifest['files'].items()):
        got = hashlib.sha256((snapshot / rel).read_bytes()).hexdigest()
        if got != want:
            raise SystemExit('Reconstruction SHA256 mismatch: ' + rel)
    shutil.copyfile(HERE / 'check_gate.py', base / 'check_gate.py')
    command = [sys.executable] + (['-O'] if sys.argv[1:] else []) + [str(base / 'check_gate.py')]
    completed = subprocess.run(command, cwd=base)
    raise SystemExit(completed.returncode)
