#!/usr/bin/env python3
"""Compile/link CUDA CMake targets only. Never execute a CUDA binary."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
BUILD = Path(__file__).resolve().parent
OUT = BUILD / 'capture_r1'
OUT.mkdir(exist_ok=False)
shutil.copyfile(__file__, OUT / 'capture.py')
files = [ROOT / 'morsehgp3D_v7/CMakeLists.txt', ROOT / 'morsehgp3D_v7/bench/nvcc_strict_host.py']
for group in ('src', 'bench', 'tests'):
    files += [p for p in (ROOT / 'morsehgp3D_v7' / group).rglob('*') if p.is_file() and p.suffix in ('.hpp','.h','.cuh','.cpp','.cu')]
def pins():
    return {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
def save(name, value):
    with (OUT / name).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')
before = pins()
save('sources_before.json', before)
commands=[]
for name, argv in [
    ('configure', ['cmake','-S',str(ROOT / 'morsehgp3D_v7'),'-B',str(BUILD),'-DCMAKE_BUILD_TYPE=Release',
       '-DMHGP7_ENABLE_CUDA=ON','-DCMAKE_CUDA_COMPILER='+str(ROOT/'build/v7_nvcc_pedantic_20260910/toolkit/bin/nvcc'),
       '-DCMAKE_CUDA_FLAGS=-L'+str(ROOT/'build/v7_nvcc_pedantic_20260910/toolkit/lib'),
       '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR='+str(ROOT/'build/v7_boost_gate/extracted/usr/include')]),
    ('build', ['cmake','--build',str(BUILD),'--parallel','1','--verbose','--target',
               'mhgp7_census_route_device_gate','mhgp7_full_ball_tower_cuda_probe'])]:
    start = time.time_ns()
    with (OUT/(name+'.stdout')).open('xb') as stdout, (OUT/(name+'.stderr')).open('xb') as stderr:
        result = subprocess.run(argv, cwd=ROOT, stdout=stdout, stderr=stderr)
    commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(), exit_code=result.returncode))
    print(name, result.returncode, flush=True)
    if result.returncode:
        break
after=pins()
save('sources_after.json', after)
save('commands.json', commands)
binary_pins={name:hashlib.sha256((BUILD/name).read_bytes()).hexdigest() for name in
             ('mhgp7_census_route_device_gate','mhgp7_full_ball_tower_cuda_probe') if (BUILD/name).is_file()}
ok=before==after and len(binary_pins)==2 and all(c['exit_code']==0 for c in commands)
save('receipt.json',dict(status='passed' if ok else 'failed', binaries=binary_pins, sources_stable=before==after,
                         CUDA_executed=False, GCP_used=False, fresh_targets_on_local_redist=True))
for name in ('CMakeCache.txt','nvcc_strict_host.py'):
    shutil.copyfile(BUILD/name, OUT/name)
raise SystemExit(0 if ok else 1)
