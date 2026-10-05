#!/usr/bin/env python3
"""Reconstruct fixed Git sources and run the Python-only bounded S6b counter-check."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
repo = next((p for p in HERE.parents if (p/'.git').exists()), None)
if repo is None:
    raise SystemExit('Run inside the repository with the captured Git pin available')
if sys.argv[1:] not in ([], ['--optimized']):
    raise SystemExit('usage: python3 replay.py [--optimized]')
manifest = json.loads((HERE/'source_manifest.json').read_text())
with tempfile.TemporaryDirectory(prefix='mhgp11-s6b-math-') as folder:
    base = pathlib.Path(folder)
    for rel, want in sorted(manifest['files'].items()):
        data = subprocess.check_output(['git','show',manifest['pin']+':'+rel],cwd=repo)
        if hashlib.sha256(data).hexdigest() != want:
            raise SystemExit('Captured source SHA256 mismatch: '+rel)
        target = base/'snapshot'/rel
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(data)
    shutil.copyfile(HERE/'check_assembly.py',base/'check_assembly.py')
    command = [sys.executable]+(['-O'] if sys.argv[1:] else [])+[str(base/'check_assembly.py')]
    result = subprocess.run(command,cwd=base)
    raise SystemExit(result.returncode)
