"""Sondes petites de frontiere ; conserve les sorties meme pour les crashes attendus."""
import hashlib
import json
import os
from pathlib import Path
import resource
import struct
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = Path('/workspaces/E-HGP/build/v10-audit-independent-20260929')
BUILD = ROOT / 'release'
SOURCE = ROOT / 'source/morsehgp3D_v10'
DATA = ROOT / 'frontier'
DATA.mkdir(exist_ok=True)


def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run(name, args, timeout=60):
    start = time.monotonic()
    try:
        r = subprocess.run(list(map(str, args)), capture_output=True, text=True,
                           timeout=timeout, preexec_fn=no_core)
        record = dict(name=name, command=list(map(str, args)), returncode=r.returncode,
                      stdout=r.stdout, stderr=r.stderr, elapsed_s=time.monotonic() - start)
    except subprocess.TimeoutExpired as e:
        def decoded(s):
            return s.decode(errors='replace') if isinstance(s, bytes) else s
        record = dict(name=name, command=list(map(str, args)), timeout=timeout,
                      stdout=decoded(e.stdout), stderr=decoded(e.stderr), elapsed_s=time.monotonic() - start)
    (HERE / (name + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(dict(name=name, returncode=record.get('returncode'),
                         elapsed_s=record['elapsed_s'], timeout=record.get('timeout'))), flush=True)
    return record


valid = DATA / 'valid.u32le'
raw = b''.join(struct.pack('<III', *p) for p in [(0, 0, 0), (8, 0, 0), (0, 8, 0), (0, 0, 8)])
valid.write_bytes(raw)
records = []
for suffix, trailer in [('one_byte', b'\x01'), ('one_word', struct.pack('<I', 262144)),
                        ('two_words', struct.pack('<II', 262144, 1))]:
    src = DATA / (suffix + '.u32le')
    src.write_bytes(raw + trailer)
    for tool in ['catalogue', 'tower', 'cluster']:
        args = [BUILD / ('mhgp10_' + tool), src]
        if tool == 'cluster':
            args += [DATA / (tool + suffix + '.i32le'), '--mcs=2']
        args += ['--k=2', '--threads=1']
        records.append(run('frontier_' + tool + '_' + suffix, args))

records.append(run('frontier_tower_repeat0', [BUILD / 'mhgp10_tower', valid, '--k=2', '--threads=1', '--repeat=0']))
records.append(run('frontier_tower_no_points_dump', [BUILD / 'mhgp10_tower', valid, '--k=2', '--threads=1',
                                                   '--no-points', '--dump=' + str(DATA / 'no_points.dump')]))
exe = DATA / 'pool_failure_probe'
compiled = run('pool_failure_compile', ['c++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                      '-I' + str(SOURCE / 'src'), HERE / 'pool_failure_probe.cpp',
                                      BUILD / 'libmhgp10_core.a', '-pthread', '-o', exe])
if compiled.get('returncode') == 0:
    records.append(run('pool_failure_worker', [exe], timeout=10))

(HERE / 'FRONTIER_CHECKS.json').write_text(json.dumps({
    'commit': '6206d1d118794c9e1cabb6faeaec2aaa77d37e5b',
    'source': str(SOURCE), 'cases': records,
    'binary_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in BUILD.glob('mhgp10_*') if p.is_file()},
}, indent=2) + '\n')
