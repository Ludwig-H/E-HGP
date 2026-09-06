#!/usr/bin/env python3
"""Explicit one-command capture; --compile never launches a measurement."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os
import resource
import signal
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
PROJECT = ROOT / 'morsehgp3D_v7'
PRIVATE = ROOT / 'build/v7_wspd_histogram_blocks_20260906'
BINARY = BASE / 'histogram_bench'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as out:
        json.dump(value, out, indent=2, sort_keys=True)
        out.write('\n')


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ('--compile', '--unknown', '--s=8', '--s=10', '--s=12'):
        return 2
    mode = sys.argv[1]
    compile_step = mode == '--compile'
    measurement = mode.startswith('--s=')
    label = 'compile' if compile_step else mode[2:].replace('=', '')
    cpu = 0 if compile_step or mode == '--unknown' else 6
    sources = sorted((PROJECT / 'src').rglob('*.hpp')) + [PRIVATE / 'histogram_bench.cpp',
        PRIVATE / 'histogram_blocks.hpp', Path(__file__).resolve()]
    before = {str(p.relative_to(ROOT)): sha(p) for p in sources}
    save(BASE / (label + '.sources_before.json'), before)
    argv = ['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
            '-I' + str(PROJECT), '-MMD', '-MF', str(BASE / 'compile.d'),
            str(PRIVATE / 'histogram_bench.cpp'), '-o', str(BINARY)] if compile_step else (
            ['taskset', '--cpu-list', '6', str(BINARY), mode] if measurement else [str(BINARY), mode])
    row = dict(argv=argv, cwd=str(ROOT), cpu=cpu, started=datetime.now(timezone.utc).isoformat(),
               wall_seconds=None if measurement else 300, cpu_seconds=None if measurement else 320,
               fsize_bytes=None if measurement else (512 if compile_step else 64) << 20,
               address_space_bytes=26 << 30, expected_exit=2 if mode == '--unknown' else 0,
               exit_code=None, pid=None, closed=False, binary_before=None if compile_step else sha(BINARY))
    save(BASE / (label + '.intent.json'), row)
    def limits():
        if not measurement:
            os.sched_setaffinity(0, {cpu})
            resource.setrlimit(resource.RLIMIT_CPU, (320, 320))
            resource.setrlimit(resource.RLIMIT_FSIZE, (row['fsize_bytes'], row['fsize_bytes']))
        resource.setrlimit(resource.RLIMIT_AS, (26 << 30, 26 << 30))
    try:
        with (BASE / (label + '.stdout')).open('xb') as out, (BASE / (label + '.stderr')).open('xb') as err:
            p = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err, start_new_session=True, preexec_fn=limits)
            row['pid'] = p.pid
            try:
                row['exit_code'] = p.wait() if measurement else p.wait(timeout=300)
            except KeyboardInterrupt:
                row['interrupted'] = True
                os.killpg(p.pid, signal.SIGKILL)
                row['exit_code'] = p.wait()
            except subprocess.TimeoutExpired:
                row['timeout'] = True
                os.killpg(p.pid, signal.SIGKILL)
                row['exit_code'] = p.wait()
            try:
                os.killpg(p.pid, 0)
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                row['closed'] = True
    finally:
        row['ended'] = datetime.now(timezone.utc).isoformat()
        row['streams'] = {s: sha(BASE / (label + '.' + s)) for s in ('stdout', 'stderr')}
        row['binary_after'] = sha(BINARY) if BINARY.exists() else None
        after = {str(p.relative_to(ROOT)): sha(p) for p in sources}
        save(BASE / (label + '.sources_after.json'), after)
        row['sources_stable'] = before == after
        if compile_step and row['exit_code'] == 0:
            deps = (BASE / 'compile.d').read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            observed = {str(Path(p).resolve().relative_to(ROOT)): sha(p) for p in deps}
            row['dependencies_bound'] = bool(observed) and all(before.get(k) == v for k, v in observed.items())
            save(BASE / 'compile.dependencies.json', observed)
        row['status'] = 'completed' if (row['closed'] and row['exit_code'] == row['expected_exit']
            and row['sources_stable'] and not row.get('timeout') and not row.get('interrupted')
            and row.get('dependencies_bound', True)
            and (compile_step or row['binary_before'] == row['binary_after'])) else 'failed'
        save(BASE / (label + '.command.json'), row)
        print(json.dumps(row, sort_keys=True), flush=True)
    return 0 if row['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
