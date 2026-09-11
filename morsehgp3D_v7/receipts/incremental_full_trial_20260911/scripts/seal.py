#!/usr/bin/env python3
"""Seal this completed private task, preserving failures and source snapshots."""
from __future__ import annotations
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    if (HERE / 'MANIFEST.json').exists(): raise RuntimeError('already sealed')
    checks = HERE / 'readback'; checks.mkdir(exist_ok=False)
    commands = []
    for name, flags in (('normal', []), ('optimized', ['-O'])):
        argv = [sys.executable, '-B', *flags, str(HERE / 'verify.py')]
        start = time.time_ns()
        completed = subprocess.run(argv, text=True, capture_output=True, check=False)
        (checks / (name + '.stdout')).write_text(completed.stdout)
        (checks / (name + '.stderr')).write_text(completed.stderr)
        commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(),
            exit_code=completed.returncode, stdout_sha256=sha(checks / (name + '.stdout')),
            stderr_sha256=sha(checks / (name + '.stderr'))))
        if completed.returncode or completed.stderr: raise RuntimeError('reader failed:' + name)
    (checks / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    deltas = []
    for name in ('full_ball_tower.hpp', 'full_coverage_certificate.hpp', 'full_coverage_incremental.hpp'):
        old = HERE / 'baseline/morsehgp3D_v7/src/forest' / name
        new = HERE / 'candidate/morsehgp3D_v7/src/forest' / name
        deltas.extend(difflib.unified_diff(old.read_text().splitlines(keepends=True) if old.exists() else [],
            new.read_text().splitlines(keepends=True), fromfile='a/morsehgp3D_v7/src/forest/' + name,
            tofile='b/morsehgp3D_v7/src/forest/' + name))
    (HERE / 'delta.patch').write_text(''.join(deltas))
    binaries, files = {}, {}
    for path in sorted(HERE.rglob('*')):
        if not path.is_file(): continue
        name = path.relative_to(HERE).as_posix()
        if path.read_bytes().startswith(b'\x7fELF'): binaries[name] = sha(path)
        else: files[name] = sha(path)
    (HERE / 'binaries.json').write_text(json.dumps(binaries, indent=2, sort_keys=True) + '\n')
    files['binaries.json'] = sha(HERE / 'binaries.json')
    (HERE / 'MANIFEST.json').write_text(json.dumps(dict(files=files, public_status='not_claimed',
        no_active_source_change=True, GCP_used=False), indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(status='sealed_private_incremental_full', captured_files=len(files),
        pinned_ELFs=len(binaries), manifest_sha256=sha(HERE / 'MANIFEST.json'))))

if __name__ == '__main__': main()
