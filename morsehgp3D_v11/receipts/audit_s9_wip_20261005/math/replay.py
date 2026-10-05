#!/usr/bin/env python3
"""Reconstruct frozen S9 WIP from Git pin and SHA-verified overlays, without the developer worktree."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE=pathlib.Path(__file__).resolve().parent
manifest=json.loads((HERE/'source_manifest.json').read_text())
repository=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path('/workspaces/E-HGP')
with tempfile.TemporaryDirectory(prefix='v11-audit-math-s9-') as directory:
    root=pathlib.Path(directory)
    for name,fact in manifest['files'].items():
        raw=(HERE/'overlay'/name).read_bytes() if fact['origin']=='overlay' else subprocess.check_output(
            ['git','-C',str(repository),'show',manifest['pin']+':'+name])
        digest=hashlib.sha256(raw).hexdigest()
        if digest!=fact['sha256_before'] or digest!=fact['sha256_after']:
            raise RuntimeError('source SHA256 differs: '+name)
        target=root/'snapshot'/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
    shutil.copyfile(HERE/'check_points.py',root/'check_points.py')
    command=[sys.executable]+(['-O'] if sys.flags.optimize else [])+[str(root/'check_points.py')]
    sys.exit(subprocess.run(command,timeout=90).returncode)
