#!/usr/bin/env python3
"""Eight direct local commands; no added time, CPU, file or address-space quota."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
PROJECT = ROOT / 'morsehgp3D_v7'
LOG = BASE / 'logs'
ROWS = []


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as out:
        json.dump(value, out, indent=2, sort_keys=True)
        out.write('\n')


def require(ok, why):
    if not ok:
        raise ValueError(why)


def inventory():
    files = list((PROJECT / 'src').rglob('*.hpp'))
    files += [p for p in BASE.rglob('*') if p.suffix in ('.hpp', '.cpp', '.py')]
    return {str(p.resolve()): sha(p) for p in sorted(files)}


def command(label, argv, expected=0):
    argv = ['taskset', '--cpu-list', '0', *argv]
    row = dict(label=label, argv=argv, cwd=str(ROOT), cpu=0, started=datetime.now(timezone.utc).isoformat(),
               added_resource_quotas='none', pid=None, expected_exit=expected, exit_code=None, closed=False)
    save(LOG / (label + '.intent.json'), row)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    row['sanitizer_environment'] = {k: env[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')}
    try:
        with (LOG / (label + '.stdout')).open('xb') as out, (LOG / (label + '.stderr')).open('xb') as err:
            p = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err,
                                 env=env, start_new_session=True)
            row['pid'] = p.pid
            try:
                row['exit_code'] = p.wait()
            except KeyboardInterrupt:
                row['interrupted'] = True
                os.killpg(p.pid, signal.SIGKILL)
                row['exit_code'] = p.wait()
            try:
                os.killpg(p.pid, 0)
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                row['closed'] = True
    finally:
        row['ended'] = datetime.now(timezone.utc).isoformat()
        row['streams'] = {s: sha(LOG / (label + '.' + s)) for s in ('stdout', 'stderr')}
        save(LOG / (label + '.command.json'), row)
        ROWS.append(row)
    print(label, row['exit_code'], 'closed=' + str(row['closed']), flush=True)
    require(row['exit_code'] == expected and row['closed'] and not row.get('interrupted'), label)


def main():
    before = inventory()
    save(BASE / 'sources_before.json', before)
    result = dict(schema='mhgp7-private-negative-histogram-qualification-v1', status='failed', public_status='not_claimed', binaries={})
    try:
        original = (BASE / 'nominal/histogram_negative.hpp').read_text()
        mutated = (BASE / 'mutant/histogram_negative.hpp').read_text()
        needle = '16 * cross_lower(a, b0, z)'
        require(original.count(needle) == 1 and mutated == original.replace(needle,
            '16 * positive::cross_upper(a, positive::point_box(b0), z)'), 'single_physical_mutant')
        for mode in ('O2', 'san', 'mutant'):
            variant = 'mutant' if mode == 'mutant' else 'nominal'
            flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if mode == 'san' else ['-O2']
            binary = BASE / 'bin' / mode
            dep = LOG / (mode + '.d')
            command(mode + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
                *flags, '-I' + str(PROJECT), '-MMD', '-MF', str(dep), str(BASE / variant / 'negative_gate.cpp'), '-o', str(binary)])
            result['binaries'][mode] = sha(binary)
            paths = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            observed = {str(Path(p).resolve()): sha(p) for p in paths}
            require(observed and all(before.get(k) == v for k, v in observed.items()), 'actual_dependencies')
            save(LOG / (mode + '.dependencies.json'), observed)
            command(mode + '_selftest', [str(binary), '--selftest'], 1 if mode == 'mutant' else 0)
            if mode == 'mutant':
                require((LOG / 'mutant_selftest.stderr').read_text() ==
                    'negative histogram rejected: negative.true_witness_preserved\n', 'causal_mutant_message')
            else:
                command(mode + '_unknown', [str(binary), '--unknown'], 2)
                gate = json.loads((LOG / (mode + '_selftest.stdout')).read_text())
                require(gate['status'] == 'passed' and type(gate['comparisons']) is int and gate['comparisons'] == 432
                        and gate['non_site_lanes'] == 4, 'gate_nonvacuity')
            require(sha(binary) == result['binaries'][mode], 'stable_binary')
        require((LOG / 'O2_selftest.stdout').read_bytes() == (LOG / 'san_selftest.stdout').read_bytes(), 'O2_SAN_equality')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = inventory()
        save(BASE / 'sources_after.json', after)
        result.update(sources_stable=before == after, commands=len(ROWS), cpp_closed=all(r['closed'] for r in ROWS))
        if before != after:
            result['status'] = 'failed'
        save(BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
