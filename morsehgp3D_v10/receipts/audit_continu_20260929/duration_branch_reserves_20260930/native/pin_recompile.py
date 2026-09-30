"""Reproduce only the existing wrapper; close all -MMD project dependencies.

No native execution. The first compilation pinned .hpp files but omitted
core/reasons.def; this second compilation explicitly closes that dependency.
It must reproduce the exact binary already used by the two saved exports.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parent
INCLUDE = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/src')
ARCHIVE = Path('/workspaces/E-HGP/build/v10-wt/libmhgp10_core.a')
dep = (ROOT/'probe.d').read_text().replace('\\\n',' ')
dependencies = [Path(x) for x in dep.split(':',1)[1].split()]
inputs = sorted(set(dependencies+[ARCHIVE,ROOT/'probe',Path(__file__).resolve()]))

def hashes():
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}

before = hashes()
argv = ['c++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-pthread',
        '-MMD','-MF',str(ROOT/'probe_closed.d'),'-I'+str(INCLUDE),str(ROOT/'probe.cpp'),
        str(ARCHIVE),'-o',str(ROOT/'probe_closed')]
start = time.monotonic()
result = subprocess.run(argv,capture_output=True,text=True,timeout=15)
wall = time.monotonic()-start
after = hashes()
binary_sha = hashlib.sha256((ROOT/'probe_closed').read_bytes()).hexdigest() if result.returncode==0 else None
original_sha = before[str(ROOT/'probe')]
print(json.dumps({'scope':'wrapper compilation only, no native execution or engine rebuild',
                  'reason':'close core/reasons.def omitted from first .hpp-only pin',
                  'argv':argv,'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr,
                  'wall_seconds':wall,'hashes_before':before,'hashes_after':after,
                  'source_closure_stable':before==after,'project_dependencies':[str(p) for p in dependencies],
                  'original_probe_sha256':original_sha,'reproduced_probe_sha256':binary_sha,
                  'binary_identical_to_two_saved_exports':binary_sha==original_sha,
                  'archive_source_commit_from_matching_historical_receipt':'6206d1d11',
                  'GCP_used':False,'native_calls':0},indent=2))
