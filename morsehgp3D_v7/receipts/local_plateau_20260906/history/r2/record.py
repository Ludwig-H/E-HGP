#!/usr/bin/env python3
"""Small local qualification; direct wait, no added resource/time quotas."""
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
LOG = BASE / 'logs'
BOOST = ROOT / 'build/v7_boost_gate/extracted/usr/include'
ROWS = []


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as out:
        json.dump(value, out, sort_keys=True, indent=2)
        out.write('\n')


def require(ok, why):
    if not ok:
        raise ValueError(why)


def command(label, args, expected=0):
    argv = ['taskset', '--cpu-list', '0', *args]
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    row = dict(argv=argv, cwd=str(ROOT), started=datetime.now(timezone.utc).isoformat(), cpu=0,
               expected_exit=expected, exit_code=None, pid=None, closed=False, added_quotas='none',
               environment={k: env[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')})
    save(LOG / (label + '.intent.json'), row)
    try:
        with (LOG / (label + '.stdout')).open('xb') as out, (LOG / (label + '.stderr')).open('xb') as err:
            p = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=out, stderr=err, start_new_session=True)
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


def dependencies(path):
    names = path.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
    return {str(Path(p).resolve()): sha(p) for p in names}


def main():
    result = dict(schema='mhgp7-private-local-plateau-qualification-v1', status='failed', public_status='not_claimed', binaries={})
    try:
        original = (BASE / 'nominal/local_plateau.hpp').read_text()
        mutant = (BASE / 'mutant/local_plateau.hpp').read_text()
        require(original.count('if (a != b) parent[') == 1 and mutant == original.replace(
            'if (a != b) parent[', 'if (a != b && mask == 0) parent['), 'single_physical_star_mutant')
        for mode in ('O2', 'san', 'mutant'):
            source = BASE / ('mutant' if mode == 'mutant' else 'nominal') / 'local_gate.cpp'
            flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if mode == 'san' else ['-O2']
            common = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', *flags,
                      '-I' + str(ROOT / 'morsehgp3D_v7'), '-I' + str(BOOST)]
            # Preprocess only: pin the actual product AND Boost dependencies before compilation.
            command(mode + '_dependencies', [*common, '-MM', str(source)])
            before = dependencies(LOG / (mode + '_dependencies.stdout'))
            save(LOG / (mode + '.sources_before.json'), before)
            binary = BASE / 'bin' / mode
            dep = LOG / (mode + '.d')
            command(mode + '_compile', [*common, '-MMD', '-MF', str(dep), str(source), '-o', str(binary)])
            after = dependencies(dep)
            save(LOG / (mode + '.sources_after_compile.json'), after)
            require(before == after, 'compiled_dependencies_stable')
            result['binaries'][mode] = sha(binary)
            command(mode + '_selftest', [str(binary), '--selftest'], 1 if mode == 'mutant' else 0)
            if mode == 'mutant':
                require((LOG / 'mutant_selftest.stderr').read_text() ==
                    'local plateau rejected: quotient.strict_component_count\n', 'causal_star_mutant_message')
            else:
                command(mode + '_unknown', [str(binary), '--unknown'], 2)
                gate = json.loads((LOG / (mode + '_selftest.stdout')).read_text())
                require(gate['status'] == 'passed' and gate['tables'] == 18 and gate['ranks'] == 96,
                        'gate_nonvacuity')
            require(sha(binary) == result['binaries'][mode] and dependencies(dep) == before, 'sources_and_binary_stable')
        require((LOG / 'O2_selftest.stdout').read_bytes() == (LOG / 'san_selftest.stdout').read_bytes(), 'O2_SAN_literal_equality')
        result['status'] = 'completed'
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        result.update(commands=len(ROWS), cpp_closed=all(row['closed'] for row in ROWS))
        save(BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
