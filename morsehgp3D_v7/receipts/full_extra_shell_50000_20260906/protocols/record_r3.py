#!/usr/bin/env python3
"""Create-only local capture; no operation/time quotas, no cloud, no oracle path."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    parser.add_argument('--capture-50000', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    rows = []

    def command(name, argv, expected=0, sanitized=False):
        env = dict(os.environ)
        if sanitized:
            env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        row = dict(argv=list(map(str, argv)), cwd=str(ROOT), expected_exit=expected,
                   utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), timeout=None)
        save(output / (name + '.intent.json'), row)
        start = time.monotonic()
        failure = None
        with (output / (name + '.stdout')).open('xb') as out, (output / (name + '.stderr')).open('xb') as err:
            child = subprocess.Popen(row['argv'], cwd=ROOT, stdout=out, stderr=err, env=env, start_new_session=True)
            print(name, 'PID', child.pid, flush=True)
            row['pid'] = child.pid
            try:
                row['exit_code'] = child.wait()
            except BaseException as error:
                failure = error
            finally:
                if child.poll() is None:
                    os.killpg(child.pid, signal.SIGTERM)
                    try:
                        child.wait(timeout=10)  # cleanup only, not computation budget
                    except subprocess.TimeoutExpired:
                        os.killpg(child.pid, signal.SIGKILL)
                        child.wait()
                row['exit_code'] = child.returncode
        row.update(elapsed_seconds=time.monotonic()-start,
                   stdout_sha256=sha(output / (name + '.stdout')),
                   stderr_sha256=sha(output / (name + '.stderr')))
        rows.append(row)
        save(output / (name + '.command.json'), row)
        if failure is not None:
            raise failure
        if row['exit_code'] != expected:
            raise ValueError(name + ': unexpected exit ' + str(row['exit_code']))

    sources = ['morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp',
               'morsehgp3D_v7/bench/full_extra_shell_diagnostic.hpp',
               'morsehgp3D_v7/tests/full_extra_shell_diagnostic_gate.cpp']
    before = {name: sha(ROOT / name) for name in sources}
    save(output / 'edited_sources_before.json', before)
    status = 'failed'
    try:
        common = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']
        gate = ROOT / sources[2]
        for mode, flags in [('O2', ['-O2', '-DNDEBUG']),
                            ('SAN', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
            binary = output / ('gate_' + mode)
            command('compile_' + mode, [*common, *flags, gate, '-o', binary])
            command('selftest_' + mode, [binary, '--selftest'], sanitized=mode == 'SAN')
            command('square_' + mode, [binary, '--emit-square'], sanitized=mode == 'SAN')
        if (output / 'selftest_O2.stdout').read_bytes() != (output / 'selftest_SAN.stdout').read_bytes():
            raise ValueError('O2/SAN gate mismatch')
        if (output / 'square_O2.stdout').read_bytes() != (output / 'square_SAN.stdout').read_bytes():
            raise ValueError('O2/SAN square mismatch')
        command('unknown_gate', [output / 'gate_O2', '--unknown'], expected=2)
        probe = output / 'probe_O3'
        depfile = output / 'probe.d'
        command('compile_probe', [*common, '-O3', '-DNDEBUG', '-MMD', '-MF', depfile, ROOT / sources[0], '-o', probe])
        dependencies = sorted({str(Path(name).resolve().relative_to(ROOT)) for name in
            shlex.split(depfile.read_text().replace('\\\n', ' ').split(':', 1)[1])})
        pins = {name: sha(ROOT / name) for name in dependencies}
        save(output / 'dependencies.json', pins)
        for name in set(dependencies) | set(sources):
            path = output / 'sources' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, path)
        save(output / 'binaries.json', {path.name: sha(path) for path in (probe, output/'gate_O2', output/'gate_SAN')})
        options = ['--s=8', '--kmax=10', '--alias-policy=lazy', '--cache-entries=1000000',
                   '--meb-proposal-supports=unlimited', '--threads=8']
        command('micro_plain', [probe, '--n=8', *options])
        command('micro_diagnostic', [probe, '--n=8', *options, '--extra-shell-diagnostics'])
        command('duplicate_flag', [probe, '--n=8', *options, '--extra-shell-diagnostics', '--extra-shell-diagnostics'], expected=2)
        plain = [json.loads(line) for line in (output/'micro_plain.stdout').read_text().splitlines()]
        diagnosed = [json.loads(line) for line in (output/'micro_diagnostic.stdout').read_text().splitlines()]
        if ([r.get('certificate_digest') for r in plain] != [r.get('certificate_digest') for r in diagnosed] or
                diagnosed[-1]['extra_shell_diagnostic_records'] != 0 or (output/'micro_diagnostic.stderr').stat().st_size):
            raise ValueError('diagnostic changed micro forests or emitted false cases')
        if args.capture_50000:
            command('n50000_k10', ['/usr/bin/time', '-v', probe, '--n=50000', *options,
                                  '--extra-shell-diagnostics'], expected=2)
        if {name: sha(ROOT / name) for name in pins} != pins:
            raise ValueError('dependencies changed during capture')
        status = 'completed'
    finally:
        after = {name: sha(ROOT / name) for name in sources}
        save(output / 'edited_sources_after.json', after)
        save(output / 'receipt.json', dict(status=status, public_status='not_claimed',
             commands=rows, sources_stable=before == after, actual_GCP_used=False,
             no_new_time_or_operation_quotas=True, performance_contract_certified=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
