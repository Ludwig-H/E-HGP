#!/usr/bin/env python3
"""Seal closed local/G4 observations without keys, profiles, vendor sources or ELF."""
import gzip
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
HERE = Path(__file__).resolve().parent
OUT = ROOT / 'morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910'
LOCAL = HERE / 'local_r1'
SESSIONS = {
    'compile_failed': Path('/home/codespace/.local/state/ehgp/ball-v7-r2-20260910.ijuqhvqjYP/full_host'),
    'optimized': Path('/home/codespace/.local/state/ehgp/optimized-v7-20260910.Xqje4g82k2/full_host')}
def read(path):
    return json.loads(path.read_text())
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
local = read(LOCAL / 'receipt.json')
if local['status'] != 'completed' or not local['sources_stable']:
    raise ValueError('local observations not closed')
for name, host in SESSIONS.items():
    receipt = read(host / 'receipt.json')
    if not receipt['targeted_shutdown_certified'] or not receipt['capture_received']:
        raise ValueError('target shutdown/capture not certified: ' + name)
if OUT.exists():
    raise ValueError('publication already exists')
OUT.mkdir()
storage = {}
def copy(source, relative):
    raw = source.read_bytes()
    if raw.startswith((b'\x7fELF', b'-----BEGIN OPENSSH PRIVATE KEY-----')):
        raise ValueError('forbidden binary/key')
    target = OUT / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    # Archive a large compiler failure losslessly, not as a rewritten excerpt.
    compressed = source.suffix == '.stderr' and len(raw) > 100000
    if compressed:
        target = target.with_name(target.name + '.gz')
        target.write_bytes(gzip.compress(raw, mtime=0))
        storage[relative] = dict(stored=target.relative_to(OUT).as_posix(),
                                 raw_sha256=hashlib.sha256(raw).hexdigest(), raw_bytes=len(raw))
    else:
        target.write_bytes(raw)

for path in LOCAL.iterdir():
    if path.is_file() and path.name != 'full_ball_probe':
        copy(path, 'local/' + path.name)
for name in ('snapshot.tar.gz','source_manifest.json','pins.json','head.txt','status.txt'):
    copy(HERE / 'input_r1' / name, 'snapshot/' + name)
for label, host in SESSIONS.items():
    for name in ('receipt.json','handoff.json','lifecycle.txt','worker.py','start_and_verify.sh','stop_and_verify.sh',
                 'guarded_start.command.json','guarded_stop.command.json','guarded_stop.stdout','guarded_stop.stderr',
                 'source_manifest.json','snapshot.tar.gz'):
        if (host/name).is_file():
            copy(host/name, f'gcp/{label}/host/{name}')
    guest = host / 'received/output'
    for path in guest.iterdir():
        if path.is_file() and path.name not in ('device_gate','full_probe','full_probe_gpu'):
            copy(path, f'gcp/{label}/output/{path.name}')

captures = {
    'cmake_cpu/initial': ROOT/'build/v7_optimized_checks_20260910/capture_r1',
    'cmake_cpu/stable': ROOT/'build/v7_optimized_checks_20260910/capture_r2',
    'cmake_cuda': ROOT/'build/v7_cuda_cmake_20260910/capture_r1'}
for label, directory in captures.items():
    for path in directory.iterdir():
        if path.is_file(): copy(path, label+'/'+path.name)
source_pins = read(captures['cmake_cpu/stable']/'sources_before.json')
consumed = set()
for build in (ROOT/'build/v7_optimized_checks_20260910/cmake_r1', ROOT/'build/v7_cuda_cmake_20260910'):
    for depfile in build.rglob('*.o.d'):
        raw = depfile.read_text().replace('\\\n',' ').split(':',1)
        if len(raw) != 2: continue
        for spelling in shlex.split(raw[1]):
            path = Path(spelling).resolve()
            if path.is_relative_to(ROOT/'morsehgp3D_v7'):
                consumed.add(path.relative_to(ROOT).as_posix())
consumed.update(('morsehgp3D_v7/CMakeLists.txt','morsehgp3D_v7/bench/nvcc_strict_host.py'))
for name in sorted(consumed):
    if source_pins.get(name) != sha(ROOT/name):
        raise ValueError('CMake consumed source drift: '+name)
    copy(ROOT/name, 'checks_sources/'+name)
with (OUT/'checks_sources.json').open('x') as stream:
    json.dump({name:source_pins[name] for name in sorted(consumed)},stream,indent=2,sort_keys=True)

for name in ('full_probe_session_v7.py','full_probe_worker_v7.py','full_ball_worker_v7.py',
             'selftest_full_ball_worker_v7.py','inspect_full_ball_session_v7.py','selftest_inspect_full_ball_session_v7.py'):
    copy(ROOT/'gcp-migration'/name,'tools/'+name)
pure = []
for name in ('selftest_full_ball_worker_v7.py','selftest_inspect_full_ball_session_v7.py'):
    for optimize in (False,True):
        label = name.removesuffix('.py') + ('_optimized' if optimize else '_normal')
        argv = ['python3','-B',*(['-O'] if optimize else []),'gcp-migration/'+name]
        start = time.time_ns()
        run = subprocess.run(argv,cwd=ROOT,capture_output=True)
        (OUT/'tools'/f'{label}.stdout').write_bytes(run.stdout)
        (OUT/'tools'/f'{label}.stderr').write_bytes(run.stderr)
        pure.append(dict(argv=argv,started_ns=start,ended_ns=time.time_ns(),exit_code=run.returncode))
        if run.returncode: raise ValueError('pure worker/inspection gate')
with (OUT/'tools/pure_commands.json').open('x') as stream: json.dump(pure,stream,indent=2)
for name in ('progress_r1.json','progress_r2.json'):
    copy(HERE/name,'inspection/'+name)
for name in ('prepare.py','launch.py','local.py','publish.py'):
    copy(HERE/name,'recorders/'+name)
copy(HERE/'verify_scale.py','verify.py')
copy(HERE/'README.scale.md','README.md')
with (OUT/'storage_map.json').open('x') as stream: json.dump(storage,stream,indent=2,sort_keys=True)
manifest={p.relative_to(OUT).as_posix():sha(p) for p in sorted(OUT.rglob('*')) if p.is_file()}
with (OUT/'manifest.json').open('x') as stream: json.dump(manifest,stream,indent=2,sort_keys=True)
print(json.dumps(dict(files=len(manifest),bytes=sum(p.stat().st_size for p in OUT.rglob('*') if p.is_file()),
                      manifest_sha256=sha(OUT/'manifest.json')),sort_keys=True))
