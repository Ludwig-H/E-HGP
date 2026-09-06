#!/usr/bin/env python3
"""One-shot private differential capture. No benchmark or public admission."""
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
LOG = BASE / 'logs'
CPU = min(os.sched_getaffinity(0))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sources():
    return {str(p.relative_to(BASE)): sha(p) for p in sorted(BASE.rglob('*'))
            if p.is_file() and p.relative_to(BASE).parts[0] not in ('logs', 'bin')
            and p.name not in ('sources_before.json', 'sources_after.json', 'receipt.json')}


COMMANDS = []
def run(label, argv, expected=0, compile_step=False):
    row = dict(label=label, argv=argv, cwd=str(ROOT), cpu=CPU, expected_exit=expected,
               started=datetime.now(timezone.utc).isoformat(), wall_seconds=180 if compile_step else 60,
               cpu_seconds=120 if compile_step else 45, fsize_bytes=(512 if compile_step else 64) << 20,
               address_space_limit='not_set_ASan_shadow', pid=None, exit_code=None, closed=False)
    save(LOG / (label + '.intent.json'), row)
    def limits():
        os.sched_setaffinity(0, {CPU})
        resource.setrlimit(resource.RLIMIT_CPU, (row['cpu_seconds'], row['cpu_seconds']))
        resource.setrlimit(resource.RLIMIT_FSIZE, (row['fsize_bytes'], row['fsize_bytes']))
    p = None
    try:
        with (LOG / (label + '.stdout')).open('xb') as out, (LOG / (label + '.stderr')).open('xb') as err:
            p = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err, start_new_session=True,
                                 preexec_fn=limits)
            row['pid'] = p.pid
            try:
                row['exit_code'] = p.wait(timeout=row['wall_seconds'])
            except subprocess.TimeoutExpired:
                row['timeout'] = True
                os.killpg(p.pid, signal.SIGKILL)
                row['exit_code'] = p.wait()
            try:
                os.killpg(p.pid, 0)
                os.killpg(p.pid, signal.SIGKILL)
                row['residual_group_killed'] = True
            except ProcessLookupError:
                row['closed'] = True
    finally:
        row['ended'] = datetime.now(timezone.utc).isoformat()
        row['streams'] = {suffix: dict(sha256=sha(LOG / (label + '.' + suffix)),
                                      bytes=(LOG / (label + '.' + suffix)).stat().st_size)
                          for suffix in ('stdout', 'stderr') if (LOG / (label + '.' + suffix)).exists()}
        save(LOG / (label + '.command.json'), row)
        COMMANDS.append(row)
    print(label, row['exit_code'], 'closed=' + str(row['closed']), flush=True)
    need(row['exit_code'] == expected and row['closed'] and not row.get('timeout'), 'command:' + label)


def main():
    before = sources()
    save(BASE / 'sources_before.json', before)
    old = {k.removeprefix('reference/'): v for k, v in before.items() if k.startswith('reference/')}
    new = {k.removeprefix('candidate/'): v for k, v in before.items() if k.startswith('candidate/')}
    need(old.keys() == new.keys() and [k for k in old if old[k] != new[k]] == ['src/pipeline/generate.hpp'],
         'only_generate_delta')
    need(old['src/pipeline/generate.hpp'] == 'ee2a4a1f96875c7db1fbd054700a22db6eabb8f62379c71c0ed6728f1b18de59',
         'reference_pin')
    result = dict(schema='mhgp7-private-wspd-q2-capture-v1', status='failed', public_status='not_claimed',
                  cpu=CPU, reference_source='receipts/full_probe_no_quotas_20260906/source_snapshot', binaries={})
    try:
        run('compiler_version', ['g++', '--version'])
        for mode in ('O2', 'san'):
            flags = ['-O2'] if mode == 'O2' else ['-O1', '-g', '-fsanitize=address,undefined',
                                                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            for arm in ('reference', 'candidate'):
                label = mode + '_' + arm
                binary = BASE / 'bin' / label
                dep = LOG / (label + '.d')
                run(label + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-pthread', *flags, '-I' + str(BASE / arm), '-MMD', '-MF', str(dep),
                    str(BASE / 'front_gate.cpp'), '-o', str(binary)], compile_step=True)
                result['binaries'][label] = sha(binary)
                dependencies = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
                need(str(BASE / arm / 'src/pipeline/generate.hpp') in dependencies, 'actual_generate_dependency')
                need(all(Path(p).is_relative_to(BASE / arm) or Path(p) == BASE / 'front_gate.cpp'
                         for p in dependencies), 'dependency_snapshot_escape')
                save(LOG / (label + '.dependencies.json'), {p: sha(p) for p in dependencies})
                run(label + '_selftest', [str(binary), '--selftest'])
                run(label + '_unknown', [str(binary), '--unknown'], 2)
                need(sha(binary) == result['binaries'][label], 'binary_changed')
            pair = [str(LOG / (mode + '_' + arm + '_selftest.stdout')) for arm in ('reference', 'candidate')]
            for opt in ('normal', 'optimized'):
                run(mode + '_compare_' + opt, ['python3', '-B', *(['-O'] if opt == 'optimized' else []),
                    str(BASE / 'compare.py'), *pair])
            need((LOG / (mode + '_compare_normal.stdout')).read_bytes() ==
                 (LOG / (mode + '_compare_optimized.stdout')).read_bytes(), 'normal_optimized_difference')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = sources()
        save(BASE / 'sources_after.json', after)
        result.update(sources_stable=before == after, commands=len(COMMANDS),
                      all_process_groups_closed=all(row['closed'] for row in COMMANDS))
        if before != after:
            result['status'] = 'failed'
        save(BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
