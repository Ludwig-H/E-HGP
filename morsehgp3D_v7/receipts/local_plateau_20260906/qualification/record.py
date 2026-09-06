#!/usr/bin/env python3
"""Small two-phase local qualification; --san is run by ROOT outside ptrace."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
PRODUCT = ROOT / 'morsehgp3D_v7'
BOOST = ROOT / 'build/v7_boost_gate/extracted/usr/include'
PRIMARY = ('src/forest/local_plateau.hpp', 'oracle/local_plateau_oracle.hpp', 'tests/local_plateau_gate.cpp')
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
    save(BASE / (label + '.intent.json'), row)
    try:
        with (BASE / (label + '.stdout')).open('xb') as out, (BASE / (label + '.stderr')).open('xb') as err:
            p = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=out, stderr=err, start_new_session=True)
            row['pid'] = p.pid
            try:
                row['exit_code'] = p.wait()
            except BaseException:
                row['interrupted'] = True
                os.killpg(p.pid, signal.SIGKILL)
                row['exit_code'] = p.wait()
                raise
            finally:
                try:
                    os.killpg(p.pid, 0)
                    os.killpg(p.pid, signal.SIGKILL)
                except ProcessLookupError:
                    row['closed'] = True
    finally:
        row['ended'] = datetime.now(timezone.utc).isoformat()
        row['streams'] = {s: sha(BASE / (label + '.' + s)) for s in ('stdout', 'stderr') if (BASE / (label + '.' + s)).exists()}
        save(BASE / (label + '.command.json'), row)
        ROWS.append(row)
    print(label, row['exit_code'], 'closed=' + str(row['closed']), flush=True)
    require(row['exit_code'] == expected and row['closed'] and not row.get('interrupted'), label)


def dependencies(path):
    names = path.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
    return {str(Path(p).resolve()): sha(p) for p in names}


def selftest(mode):
    binary = BASE / 'bin' / mode
    command(mode + '_selftest', [str(binary), '--selftest'], 1 if mode == 'mutant' else 0)
    if mode == 'mutant':
        require((BASE / 'mutant_selftest.stderr').read_text() == 'local plateau rejected: quotient.strict_component_count\n', 'causal_star_mutant_message')
    else:
        command(mode + '_unknown', [str(binary), '--unknown'], 2)
        gate = json.loads((BASE / (mode + '_selftest.stdout')).read_text())
        require(gate['schema'] == 'mhgp7-local-plateau-v1' and gate['status'] == 'passed'
                and gate['tables'] == 18 and gate['ranks'] == 96
                and gate['real_tables'] == 4 and gate['real_ranks'] == 40, 'gate_nonvacuity')


def main(phase):
    result = dict(schema='mhgp7-local-plateau-product-qualification-v1', phase=phase, status='failed', public_status='not_claimed', binaries={})
    try:
        if phase == 'prepare':
            (BASE / 'bin').mkdir()
            for mode in ('O2', 'san', 'mutant'):
                source = PRODUCT / 'tests/local_plateau_gate.cpp'
                flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if mode == 'san' else ['-O2']
                common = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', *flags, '-I' + str(BOOST)]
                if mode == 'mutant':
                    source = BASE / 'mutant/tests/local_plateau_gate.cpp'
                command(mode + '_dependencies', [*common, '-MM', str(source)])
                before = dependencies(BASE / (mode + '_dependencies.stdout'))
                save(BASE / (mode + '.sources_before.json'), before)
                if mode == 'O2':
                    # Snapshot only the three new sources. Shared product headers
                    # remain pinned dependencies, linked in the isolated mutant.
                    for name in before:
                        path = Path(name)
                        if not path.is_relative_to(PRODUCT):
                            continue
                        relative = path.relative_to(PRODUCT)
                        destination = BASE / 'mutant' / relative
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        if str(relative) in PRIMARY:
                            snap = BASE / 'source_snapshot' / relative
                            snap.parent.mkdir(parents=True, exist_ok=True)
                            shutil.copyfile(path, snap)
                            shutil.copyfile(path, destination)
                        else:
                            destination.symlink_to(path)
                    target = BASE / 'mutant/src/forest/local_plateau.hpp'
                    original = target.read_text()
                    require(original.count('if (a != b) parent[') == 1, 'single_physical_star_mutant')
                    target.write_text(original.replace('if (a != b) parent[', 'if (a != b && mask == 0) parent['))
                binary, dep = BASE / 'bin' / mode, BASE / (mode + '.d')
                command(mode + '_compile', [*common, '-MMD', '-MF', str(dep), str(source), '-o', str(binary)])
                require(dependencies(dep) == before, 'compiled_dependencies_stable')
                result['binaries'][mode] = sha(binary)
                if mode != 'san':
                    selftest(mode)
                require(sha(binary) == result['binaries'][mode] and dependencies(dep) == before, 'source_binary_stable')
        else:
            previous = json.loads((BASE / 'prepare.receipt.json').read_text())
            require(previous['status'] == 'completed' and previous['cpp_closed'], 'prepare_closed')
            result['prepare_receipt_sha256'] = sha(BASE / 'prepare.receipt.json')
            result['binaries'] = previous['binaries']
            require(sha(BASE / 'bin/san') == previous['binaries']['san'], 'SAN_binary_pin')
            require(dependencies(BASE / 'san.d') == json.loads((BASE / 'san.sources_before.json').read_text()), 'SAN_sources_pin')
            selftest('san')
            require((BASE / 'O2_selftest.stdout').read_bytes() == (BASE / 'san_selftest.stdout').read_bytes(), 'O2_SAN_literal_equality')
            require(sha(BASE / 'bin/san') == previous['binaries']['san'], 'SAN_binary_stable')
        result['status'] = 'completed'
    except BaseException as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        result.update(commands=len(ROWS), cpp_closed=all(row['closed'] for row in ROWS))
        save(BASE / (phase + '.receipt.json'), result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    if sys.argv[1:] not in (['--prepare'], ['--san']):
        sys.exit(2)
    sys.exit(main(sys.argv[1][2:]))
