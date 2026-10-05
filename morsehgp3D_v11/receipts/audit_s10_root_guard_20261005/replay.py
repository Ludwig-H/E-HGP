#!/usr/bin/env python3
"""Replay the frozen WIP patch atop its Git pin, verify hashes, run the tiny check."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--repo', required=True)
a = p.parse_args()
m = json.loads((HERE / 'SOURCE.json').read_text())
with tempfile.TemporaryDirectory(prefix='mhgp11-s10-guard-') as tmp:
    root = Path(tmp)
    snapshot = root / 'snapshot'
    snapshot.mkdir()
    for path in m['files']:
        blob = subprocess.run(['git','-C',a.repo,'show',m['pin']+':'+path],
                              stdout=subprocess.PIPE,check=True).stdout
        target = snapshot / path
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(blob)
    subprocess.run(['git','apply',str(HERE/'wip.patch')],cwd=snapshot,check=True)
    for name in ('SOURCE.json','check.py'):
        shutil.copyfile(HERE/name,root/name)
    command = [sys.executable,'-B'] + (['-O'] if sys.flags.optimize else []) + [str(root/'check.py')]
    raise SystemExit(subprocess.run(command,check=False).returncode)
