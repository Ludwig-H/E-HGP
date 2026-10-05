#!/usr/bin/env python3
"""Reconstruct S8 sources by Git pin + SHA256; execute Python proof without the snapshot or native binaries."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
manifest = json.loads((HERE/'source_manifest.json').read_text())
repository = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path('/workspaces/E-HGP')
with tempfile.TemporaryDirectory(prefix='v11-audit-math-s8-') as directory:
    root = pathlib.Path(directory)
    for name, expected in manifest['files'].items():
        raw = subprocess.check_output(['git','-C',str(repository),'show',manifest['pin']+':'+name])
        if hashlib.sha256(raw).hexdigest() != expected:
            raise RuntimeError('source SHA256 differs: '+name)
        target = root/'snapshot'/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(raw)
    shutil.copyfile(HERE/'check_radicals.py',root/'check_radicals.py')
    command = [sys.executable]+(['-O'] if sys.flags.optimize else [])+[str(root/'check_radicals.py')]
    sys.exit(subprocess.run(command,timeout=90).returncode)
