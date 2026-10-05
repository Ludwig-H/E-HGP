#!/usr/bin/env python3
"""Reconstruct the pinned sources with Git, check SHA256, replay one synthetic mutation."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo', required=True, help='Git repository containing the pinned commit')
args = parser.parse_args()
manifest = json.loads((here / 'source_manifest.json').read_text())
with tempfile.TemporaryDirectory(prefix='mhgp11-floor-coverage-') as tmp:
    base = Path(tmp)
    for path, expected in manifest['files'].items():
        blob = subprocess.run(['git', '-C', args.repo, 'show', manifest['pin'] + ':' + path],
                              check=True, stdout=subprocess.PIPE).stdout
        if hashlib.sha256(blob).hexdigest() != expected:
            raise RuntimeError('Source hash mismatch: ' + path)
        target = base / 'snapshot' / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(blob)
    shutil.copyfile(here / 'check_gap.py', base / 'check_gap.py')
    cmd = [sys.executable] + (['-O'] if sys.flags.optimize else []) + [str(base / 'check_gap.py')]
    raise SystemExit(subprocess.run(cmd, check=False).returncode)
