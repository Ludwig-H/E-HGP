#!/usr/bin/env python3
"""Fresh strict CPU build and selected regression checks, no cloud/device execution."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
HERE = Path(__file__).resolve().parent
BUILD = HERE / 'cmake_r1'
OUT = HERE / 'capture_r1'
OUT.mkdir(exist_ok=False)
shutil.copyfile(__file__, OUT / 'capture.py')
sources = [ROOT / 'morsehgp3D_v7/CMakeLists.txt', ROOT / 'morsehgp3D_v7/bench/nvcc_strict_host.py',
           ROOT / 'gcp-migration/full_ball_worker_v7.py', ROOT / 'gcp-migration/selftest_full_ball_worker_v7.py']
for directory in ('src', 'tests', 'oracle', 'bench'):
    sources.extend(p for p in (ROOT / 'morsehgp3D_v7' / directory).rglob('*')
                   if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cuh', '.cu', '.h'))
def pins():
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
def save(name, value):
    with (OUT / name).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')
before = pins()
save('sources_before.json', before)
commands = []
plan = [
    ('configure', ['cmake', '-S', 'morsehgp3D_v7', '-B', str(BUILD), '-DCMAKE_BUILD_TYPE=Release',
                   '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=' + str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]),
    ('build', ['cmake', '--build', str(BUILD), '--parallel', '2', '--target',
               'mhgp7_anchor_meb_gate', 'mhgp7_full_ball_tower_gate', 'mhgp7_full_ball_work_gate',
               'mhgp7_full_ball_tower_probe', 'mhgp7_full_coverage_certificate_gate',
               'mhgp7_local_plateau_gate', 'mhgp7_census_route_stub_gate',
               'mhgp7_full_gabriel_digest_gate', 'mhgp7_facet_resolver_cache_gate', 'mhgp7_witness_front_gate']),
    ('ctest', ['ctest', '--test-dir', str(BUILD), '--output-on-failure', '-R',
               '^mhgp7_(anchor_meb|full_ball_tower|full_ball_work|census_route_stub|local_plateau|full_coverage_certificate|facet_resolver_cache|witness_front|full_gabriel_digest)']),
    ('worker_normal', ['python3', '-B', 'gcp-migration/selftest_full_ball_worker_v7.py']),
    ('worker_optimized', ['python3', '-B', '-O', 'gcp-migration/selftest_full_ball_worker_v7.py'])]
for name, argv in plan:
    started = time.time_ns()
    with (OUT / (name+'.stdout')).open('xb') as stdout, (OUT / (name+'.stderr')).open('xb') as stderr:
        run = subprocess.run(argv, cwd=ROOT, stdout=stdout, stderr=stderr)
    commands.append(dict(name=name, argv=argv, started_ns=started, ended_ns=time.time_ns(), exit_code=run.returncode))
    print(name, run.returncode, flush=True)
    if run.returncode:
        break
after = pins()
save('sources_after.json', after)
save('commands.json', commands)
last = BUILD / 'Testing/Temporary/LastTest.log'
if last.is_file():
    shutil.copyfile(last, OUT / 'LastTest.log')
ok = before == after and len(commands) == len(plan) and all(c['exit_code'] == 0 for c in commands)
receipt = dict(status='passed' if ok else 'failed', sources_stable=before == after,
               GCP_used=False, GPU_simulated_only=True, build_kind='fresh_private_release')
save('receipt.json', receipt)
print(json.dumps(receipt), flush=True)
raise SystemExit(0 if ok else 1)
