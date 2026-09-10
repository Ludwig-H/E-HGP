#!/usr/bin/env python3
"""Create-only local current-source capture; interrupted runs remain failures."""
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
RUN = Path(__file__).resolve().parent / sys.argv[1]
RUN.mkdir(exist_ok=False)
source = ROOT / 'morsehgp3D_v7/bench/full_ball_tower_probe.cpp'
rows, pins = [], {}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def command(name, argv):
    row = dict(name=name, argv=argv, started_ns=time.time_ns())
    save(RUN / (name + '.intent.json'), row)
    with (RUN / (name + '.stdout')).open('xb') as out, (RUN / (name + '.stderr')).open('xb') as err:
        process = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err)
        row['pid'] = process.pid
        print('started', name, process.pid, flush=True)
        row['exit_code'] = process.wait()
    row.update(closed=True, ended_ns=time.time_ns())
    rows.append(row)
    save(RUN / (name + '.command.json'), row)
    print('closed', name, row['exit_code'], flush=True)
    if row['exit_code'] != 0:
        raise RuntimeError(name + ': nonzero exit')


result = dict(status='failed', public_status='not_claimed', gcp_used=False,
              performance_scope='local_diagnostic_no_claim_of_exclusive_host')
try:
    command('git_head', ['git', 'rev-parse', 'HEAD'])
    command('git_status', ['git', 'status', '--porcelain=v1'])
    command('compiler_version', ['g++', '--version'])
    flags = ['g++', '-std=c++20', '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']
    command('dependencies', flags + ['-MM', str(source)])
    for spelling in shlex.split((RUN / 'dependencies.stdout').read_text().replace('\\\n', ' '))[1:]:
        path = Path(spelling).resolve()
        name = str(path.relative_to(ROOT))
        pins[name] = sha(path)
        destination = RUN / 'sources' / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
    save(RUN / 'sources_before.json', pins)
    binary = RUN / 'full_ball_probe'
    command('compile', flags + ['-MMD', '-MF', str(RUN / 'compile.d'), str(source), '-o', str(binary)])
    result['binary_sha256'] = sha(binary)
    for n in [400, 8000, 16000, 32000]:
        name = 'n' + str(n)
        save(RUN / (name + '.host.json'), {p: Path(p).read_text() for p in ['/proc/loadavg', '/proc/meminfo', '/sys/fs/cgroup/cpu.max']})
        command(name, ['/usr/bin/time', '-v', str(binary), '--n=' + str(n), '--s=8', '--kmax=10', '--threads=1'])
        value = json.loads((RUN / (name + '.stdout')).read_text())
        if value['orders'] != 10 or value['status'] != 'completed_relative':
            raise RuntimeError(name + ': incomplete tower')
    result['status'] = 'completed'
except BaseException as error:
    result['error'] = type(error).__name__ + ': ' + str(error)
finally:
    after = {name: sha(ROOT / name) for name in pins}
    save(RUN / 'sources_after.json', after)
    result['sources_stable'] = pins == after
    if pins != after:
        result['status'] = 'failed'
    result['commands'] = rows
    save(RUN / 'receipt.json', result)
    print(json.dumps({k: v for k, v in result.items() if k != 'commands'}), flush=True)
sys.exit(0 if result['status'] == 'completed' else 1)
