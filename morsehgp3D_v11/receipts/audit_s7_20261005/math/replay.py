#!/usr/bin/env python3
"""Reconstruct source at the pinned Git commit and run the bounded Python audit."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
manifest = json.loads((HERE / 'source_manifest.json').read_text())
repository = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path('/workspaces/E-HGP')
with tempfile.TemporaryDirectory(prefix='v11-audit-math-s7-') as directory:
    root = pathlib.Path(directory)
    for name, expected in manifest['files'].items():
        raw = subprocess.check_output(['git', '-C', str(repository), 'show', manifest['pin'] + ':' + name])
        if hashlib.sha256(raw).hexdigest() != expected:
            raise RuntimeError('source digest differs: ' + name)
        target = root / 'snapshot' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    shutil.copyfile(HERE / 'check_format.py', root / 'check_format.py')
    command = [sys.executable]
    if sys.flags.optimize:
        command.append('-O')
    command.append(str(root / 'check_format.py'))
    completed = subprocess.run(command, timeout=90)
    sys.exit(completed.returncode)
