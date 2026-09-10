#!/usr/bin/env python3
"""Preserve the initial reader schema error before correcting its field lookup."""
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT=Path('/workspaces/E-HGP')
PACKAGE=ROOT/'morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910'
OUT=Path(__file__).resolve().parent/'reader_review_r1'
OUT.mkdir(exist_ok=False)
shutil.copyfile(PACKAGE/'verify.py',OUT/'verify_initial.py.source')
shutil.copyfile(PACKAGE/'manifest.json',OUT/'manifest_initial.json')
commands=[]
for mode in ('normal','optimized'):
    argv=['python3','-B',*(['-O'] if mode=='optimized' else []),str(PACKAGE/'verify.py')]
    start=time.time_ns()
    run=subprocess.run(argv,capture_output=True)
    (OUT/(mode+'.stdout')).write_bytes(run.stdout)
    (OUT/(mode+'.stderr')).write_bytes(run.stderr)
    commands.append(dict(argv=argv,exit_code=run.returncode,started_ns=start,ended_ns=time.time_ns()))
    if run.returncode!=1 or b"KeyError: 'name'" not in run.stderr:
        raise ValueError('unexpected reader failure')
(OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
print('preserved initial reader KeyError, no benchmark rerun')
