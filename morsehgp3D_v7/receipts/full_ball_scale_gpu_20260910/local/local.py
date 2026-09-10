#!/usr/bin/env python3
"""Create-only monotone scaling capture on the exact frozen G4 source snapshot."""
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tarfile
import time

ROOT = Path('/workspaces/E-HGP')
HERE = Path(__file__).resolve().parent
RUN = HERE / 'local_r1'
RUN.mkdir(exist_ok=False)
shutil.copyfile(__file__, RUN / 'local.py')
manifest = json.loads((HERE / 'input_r1/source_manifest.json').read_text())
TREE = RUN / 'snapshot'
TREE.mkdir()
with tarfile.open(HERE / 'input_r1/snapshot.tar.gz', 'r:gz') as archive:
    for member in archive:
        if not member.isfile() or member.name not in manifest:
            raise ValueError('unexpected archive member')
        raw = archive.extractfile(member).read()
        if hashlib.sha256(raw).hexdigest() != manifest[member.name]:
            raise ValueError('snapshot pin')
        path = TREE / member.name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(raw)
rows = []
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')
def command(name, argv):
    row = dict(name=name, argv=list(map(str, argv)), started_ns=time.time_ns())
    save(RUN / (name + '.intent.json'), row)
    with (RUN / (name + '.stdout')).open('xb') as out, (RUN / (name + '.stderr')).open('xb') as err:
        process = subprocess.Popen(row['argv'], cwd=ROOT, stdout=out, stderr=err)
        row['pid'] = process.pid
        print('started', name, process.pid, flush=True)
        row['exit_code'] = process.wait()
    row.update(closed=True, ended_ns=time.time_ns(), stdout_sha256=sha(RUN / (name+'.stdout')),
               stderr_sha256=sha(RUN / (name+'.stderr')))
    rows.append(row)
    save(RUN / (name + '.command.json'), row)
    print('closed', name, row['exit_code'], flush=True)
    if row['exit_code']:
        raise RuntimeError(name + ': nonzero exit')

result = dict(status='failed', public_status='not_claimed', GCP_used=False,
              performance_scope='local_single_diagnostic_shared_host', source_snapshot_retained=True)
try:
    save(RUN / 'sources_before.json', manifest)
    command('compiler', ['g++', '--version'])
    binary = RUN / 'full_ball_probe'
    source = TREE / 'morsehgp3D_v7/bench/full_ball_tower_probe.cpp'
    command('compile', ['g++', '-std=c++20', '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                        '-pthread', '-MMD', '-MF', RUN / 'compile.d', source, '-o', binary])
    result['binary_sha256'] = sha(binary)
    for n, s in ((400, 8), (8000, 8), (16000, 8), (32000, 8), (8000, 10), (8000, 12)):
        name = f'n{n}_s{s}'
        save(RUN / (name + '.host.json'), {p: Path(p).read_text() for p in
             ['/proc/loadavg', '/proc/meminfo', '/sys/fs/cgroup/cpu.max']})
        command(name, ['/usr/bin/time', '-v', binary, f'--n={n}', f'--s={s}', '--kmax=10', '--threads=1'])
        value = json.loads((RUN / (name + '.stdout')).read_text())
        if value['orders'] != 10 or value['status'] != 'completed_relative' or value['contract_qualified'] is not False:
            raise RuntimeError(name + ': incomplete or misqualified tower')
        if sha(binary) != result['binary_sha256']:
            raise RuntimeError('binary drift')
    result['status'] = 'completed'
except BaseException as error:
    result['error'] = type(error).__name__ + ': ' + str(error)
finally:
    after = {name: sha(TREE / name) for name in manifest}
    save(RUN / 'sources_after.json', after)
    result['sources_stable'] = manifest == after
    if manifest != after:
        result['status'] = 'failed'
    result['commands'] = rows
    save(RUN / 'receipt.json', result)
    print(json.dumps({k: v for k, v in result.items() if k != 'commands'}), flush=True)
raise SystemExit(0 if result['status'] == 'completed' else 1)
