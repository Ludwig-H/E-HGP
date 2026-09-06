#!/usr/bin/env python3
"""Fresh private filtered MEB qualification; no subprocess without --execute.

--judge replays captured bytes only, including under python -O. The copied
rational modules are imported for pure functions, never their old runners.
No product integration, performance claim, Git mutation or GCP operation.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import shutil
import signal
import subprocess
import sys
import time
import types

BASE = Path(__file__).resolve().parent
ROOT = Path('/workspaces/E-HGP')
REL = 'build/v7_meb_filter_qualification_20260906'
FILTER = 'build/v7_meb_filtered_preparation_20260905/pivot.hpp'
LEGACY = 'build/v7_meb_pivot_prototype/pivot.hpp'
GATES = ('budget_gate', 'trajectory_gate', 'geometry_gate', 'rational_bridge')
PINS = {
    FILTER: '484a89bc2dbd472cc0571ed31d59631d5f31f9b0a425118040c916fc16e5abcf',
    LEGACY: 'd6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5',
    'build/v7_meb_dual_budget_prototype/pivot.hpp':
        '0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d',
    'morsehgp3D_v7/src/forest/silent_incidence.hpp':
        'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76',
    'morsehgp3D_v7/audits/meb_dual_oracle.py':
        'f5c277e24e077d02b3426ce7973954503d6b00c536cb329e63d368e73046716a',
    'morsehgp3D_v7/audits/meb_rational_oracle_20260905.py':
        'ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b',
}
MUTATIONS = {
    'skip_shell': (FILTER, 'if (shell != candidate.q) return false;',
                   'if (false && shell != candidate.q) return false;', 'oracle.terminal.'),
    'ordinal_plus_one': (FILTER, 'const u64 count = ordinal(n, candidate);',
                        'const u64 count = ordinal(n, candidate) + 1;', 'oracle.terminal.'),
    'q4_rescale_level': (LEGACY, 'ball.level = q4_level_raw(candidate.four);',
                        'ball.level = q4_level_raw(candidate.four);\n'
                        '    for (size_t j = 2; j > 0; --j) ball.level.num[j] = '
                        '(ball.level.num[j] << 1) | (ball.level.num[j - 1] >> 63);\n'
                        '    ball.level.num[0] <<= 1; ball.level.den *= 2;',
                        'oracle.q4_raw_level.'),
}


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def strict_json(raw):
    def pairs(items):
        value = {}
        for key, item in items:
            require(key not in value, 'json.duplicate_key')
            value[key] = item
        return value
    return json.loads(raw, object_pairs_hook=pairs,
                      parse_constant=lambda _: require(False, 'json.nonfinite'))


def rational_types(raw):
    for line in raw.splitlines():
        row = strict_json(line)
        require(set(row) == {'reference', 'proposed', 'same_as_F', 'work', 'observer'} and
                type(row['same_as_F']) is bool, 'rational.record_shape')
        for key, length in (('work', 4), ('observer', 3)):
            require(type(row[key]) is list and len(row[key]) == length and
                    all(type(v) is int and v >= 0 for v in row[key]), 'rational.work_types')
        for key in ('reference', 'proposed'):
            ball = row[key]
            require(set(ball) == {'ok', 'status', 'reason', 'stats', 'q', 'key', 'num', 'den',
                                  'support_slots', 'events_size'} and type(ball['ok']) is bool and
                    type(ball['reason']) is str, 'rational.ball_shape')
            require(all(type(ball[k]) is int for k in ('status', 'q', 'den', 'events_size')), 'rational.scalar_types')
            for field, length in (('stats', 13), ('key', 5), ('num', 3), ('support_slots', 4)):
                require(type(ball[field]) is list and len(ball[field]) == length and
                        all(type(v) is int for v in ball[field]), 'rational.array_types')


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(data)


def read(path):
    require(path.is_file() and not path.is_symlink(), 'source.not_regular:' + str(path))
    a = path.stat()
    data = path.read_bytes()
    b = path.stat()
    require((a.st_ino, a.st_size, a.st_mtime_ns, a.st_ctime_ns) ==
            (b.st_ino, b.st_size, b.st_mtime_ns, b.st_ctime_ns), 'source.changed_during_read')
    return data


def snapshot():
    names = set(PINS) | {REL + '/' + g + '.cpp' for g in GATES}
    names |= {REL + '/additional_scenes.inc', REL + '/README.md', REL + '/capture.py',
              'morsehgp3D_v7/oracle/obig.hpp',
              'build/v7_meb_filtered_preparation_20260905/README.md'}
    names |= {str(p.relative_to(ROOT)) for p in (ROOT / 'morsehgp3D_v7/src').rglob('*') if p.is_file()}
    values = {name: read(ROOT / name) for name in sorted(names)}
    for name, pin in PINS.items():
        require(sha(values[name]) == pin, 'authority.changed:' + name)
    return values


def oracle(snapshot_root):
    # Load approved bytes without importlib's bytecode writes or an existing
    # sys.modules authority. Only the dependency's name is bound temporarily;
    # neither historical main is invoked and sys.path stays untouched.
    def frozen(filename, name):
        relative = 'morsehgp3D_v7/audits/' + filename
        path = snapshot_root / relative
        raw = read(path)
        require(sha(raw) == PINS[relative], 'oracle.protocol_pin:' + filename)
        module = types.ModuleType(name)
        module.__file__ = str(path)
        exec(compile(raw, str(path), 'exec'), module.__dict__)
        return module

    dependency_name = 'meb_rational_oracle_20260905'
    dependency = frozen(dependency_name + '.py', 'filtered_rational_dependency')
    missing = object()
    prior = sys.modules.get(dependency_name, missing)
    sys.modules[dependency_name] = dependency
    try:
        return frozen('meb_dual_oracle.py', 'filtered_rational_judge')
    finally:
        if prior is missing:
            del sys.modules[dependency_name]
        else:
            sys.modules[dependency_name] = prior


def bounded_child():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_CPU, (120, 120))
    resource.setrlimit(resource.RLIMIT_FSIZE, (64 << 20, 64 << 20))


def execute(out, expected_source):
    values = snapshot()
    pins = {name: sha(raw) for name, raw in values.items()}
    require(sha(encoded(pins)) == expected_source, 'snapshot.not_admitted')
    snap = out / 'snapshot'
    compiler = Path('/usr/bin/c++').resolve()
    report = dict(schema='mhgp7-private-filtered-meb-run-v1', status='running',
                  public_status='not_claimed', started_utc=datetime.now(timezone.utc).isoformat(),
                  source_map_sha256=sha(encoded(pins)), compiler_path=str(compiler),
                  compiler_sha256=sha(read(compiler)), gcp='not_used', commands={}, binaries={})
    env = {name: value for name, value in os.environ.items() if name not in (
        'LD_PRELOAD', 'LD_AUDIT', 'LD_LIBRARY_PATH', 'CPATH', 'CPLUS_INCLUDE_PATH',
        'C_INCLUDE_PATH', 'GCC_EXEC_PREFIX', 'COMPILER_PATH', 'LIBRARY_PATH',
        'CPPFLAGS', 'CXXFLAGS', 'ASAN_OPTIONS', 'LSAN_OPTIONS', 'UBSAN_OPTIONS', 'PYTHONPATH')}
    env.update(LC_ALL='C', PYTHONDONTWRITEBYTECODE='1',
               ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    deadline = time.monotonic() + 900
    out.mkdir(parents=True, exist_ok=False)

    def command(label, argv, code=0, stdin=b'', expected_stderr=b''):
        require(time.monotonic() < deadline, 'campaign.deadline')
        begin = time.monotonic()
        started = datetime.now(timezone.utc).isoformat()
        save(out / 'commands' / (label + '.intent.json'), encoded(dict(
            argv=argv, cwd=str(snap), started_utc=started, expected_exit_code=code,
            stdin_sha256=sha(stdin), timeout_seconds=120)))
        input_path = out / 'commands' / (label + '.stdin')
        save(input_path, stdin)
        stdout_path = out / 'commands' / (label + '.stdout')
        stderr_path = out / 'commands' / (label + '.stderr')
        proc = None
        error = None
        try:
            with input_path.open('rb') as source, stdout_path.open('xb') as output, stderr_path.open('xb') as errors:
                proc = subprocess.Popen(argv, cwd=snap, env=env, stdin=source,
                                        stdout=output, stderr=errors,
                                        start_new_session=True, preexec_fn=bounded_child)
                proc.wait(timeout=min(120, deadline - time.monotonic()))
        except BaseException as exc:
            error = str(exc)
        finally:
            if proc is not None:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                proc.wait()
        stdout = read(stdout_path) if stdout_path.exists() else b''
        stderr = read(stderr_path) if stderr_path.exists() else b''
        row = dict(argv=argv, cwd=str(snap), started_utc=started,
                   elapsed_seconds=time.monotonic() - begin, exit_code=None if proc is None else proc.returncode,
                   expected_exit_code=code, interrupted=error,
                   stdin_sha256=sha(stdin), stdout_sha256=sha(stdout), stderr_sha256=sha(stderr))
        save(out / 'commands' / (label + '.json'), encoded(row))
        report['commands'][label] = row
        print(json.dumps(dict(command=label, code=row['exit_code'], seconds=row['elapsed_seconds'])), flush=True)
        require(error is None and row['exit_code'] == code and stderr == expected_stderr, 'command.failed:' + label)
        return stdout

    def compile_one(root, mode, gate):
        binary = out / 'bin' / (mode + '_' + gate)
        binary.parent.mkdir(exist_ok=True)
        dep = out / 'dependencies' / (mode + '_' + gate + '.d')
        dep.parent.mkdir(exist_ok=True)
        flags = ['-O2'] if mode != 'san' else ['-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        argv = [str(compiler), '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', *flags, '-I', str(root / 'morsehgp3D_v7'), '-MMD', '-MF', str(dep),
                str(root / REL / (gate + '.cpp')), '-o', str(binary)]
        command(mode + '_' + gate + '_compile', argv)
        text = read(dep).decode().replace('\\\n', ' ')
        target, body = text.split(':', 1)
        require(shlex.split(target) == [str(binary)], 'dependency.target')
        deps = [Path(p).resolve() for p in shlex.split(body)]
        require(deps and all(p.is_relative_to(root) for p in deps), 'dependency.outside_snapshot')
        actual = {str(p.relative_to(root)): sha(read(p)) for p in deps}
        require(str(Path(REL) / (gate + '.cpp')) in actual and FILTER in actual and LEGACY in actual,
                'dependency.required_helper_missing')
        expected = dict(pins)
        if mode in MUTATIONS:
            target_name, old, new, _ = MUTATIONS[mode]
            expected[target_name] = sha(values[target_name].decode().replace(old, new).encode())
        require(set(actual) <= set(expected) and all(actual[n] == expected[n] for n in actual),
                'dependency.unpinned_or_changed')
        save(out / 'dependencies' / (mode + '_' + gate + '.json'), encoded(actual))
        report['binaries'][mode + '_' + gate] = sha(read(binary))
        return str(binary)

    try:
        for name, data in values.items():
            save(snap / name, data)
        save(out / 'source_pins.json', encoded(pins))
        command('compiler', [str(compiler), '--version'])
        command('head', ['/usr/bin/git', '-C', str(ROOT), 'rev-parse', 'HEAD'])
        command('worktree', ['/usr/bin/git', '-C', str(ROOT), 'status', '--porcelain'])
        module = oracle(snap)
        _, _, geometry_input = module.make_cases()
        _, ordinal_input = module.ordinal_commands()
        save(out / 'geometry.stdin', geometry_input.encode())
        save(out / 'ordinals.stdin', ordinal_input.encode())
        for mode in ('o2', 'san'):
            for gate in GATES:
                binary = compile_one(snap, mode, gate)
                if gate == 'rational_bridge':
                    command(mode + '_rational', [binary], stdin=geometry_input.encode())
                    command(mode + '_ordinals', [binary], stdin=ordinal_input.encode())
                else:
                    command(mode + '_' + gate, [binary] + (['--selftest'] if gate == 'trajectory_gate' else []))
                    if gate in ('budget_gate', 'geometry_gate'):
                        command(mode + '_' + gate + '_charge_after', [binary, '--mutant=charge-after'], 4)
                    if gate == 'trajectory_gate':
                        command(mode + '_trajectory_order_mutant', [binary, '--reverse-order-mutant'], 4,
                                expected_stderr=b'trajectory rejected: order_mutant.first_support_changed\n')
                        command(mode + '_trajectory_admissible_order_mutant', [binary, '--admissible-order-mutant'], 4,
                                expected_stderr=b'trajectory rejected: order_budget.calendar_changed\n')
                    command(mode + '_' + gate + '_bad_argument', [binary, '--unknown'], 2)
        for name, (target, old, new, _) in MUTATIONS.items():
            variant = out / 'mutations' / name
            shutil.copytree(snap, variant)
            path = variant / target
            original = read(path).decode()
            require(original.count(old) == 1, 'mutation.site_count:' + name)
            path.write_text(original.replace(old, new))
            save(out / 'mutations' / (name + '.json'), encoded(dict(
                path=target, old=old, new=new, before=sha(original.encode()), after=sha(read(path)))))
            binary = compile_one(variant, name, 'rational_bridge')
            command(name, [binary], stdin=geometry_input.encode())
        require(snapshot() == values, 'live_source.changed')
        require(all(sha(read(snap / n)) == p for n, p in pins.items()), 'snapshot.changed')
        require(sha(read(compiler)) == report['compiler_sha256'], 'compiler.changed')
        for name, pin in report['binaries'].items():
            require(sha(read(out / 'bin' / name)) == pin, 'binary.changed')
        report['status'] = 'completed'
    except BaseException as exc:
        report['status'] = 'failed'
        report['error'] = str(exc)
        raise
    finally:
        report['ended_utc'] = datetime.now(timezone.utc).isoformat()
        save(out / 'run.json', encoded(report))


def judge(out, expected_source, expected_run):
    require(sha(read(out / 'run.json')) == expected_run, 'run.not_admitted')
    report = json.loads(read(out / 'run.json'))
    require(report['status'] == 'completed', 'run.not_completed')
    pins = json.loads(read(out / 'source_pins.json'))
    require(sha(encoded(pins)) == report['source_map_sha256'] == expected_source, 'source.map_changed')
    for name, pin in pins.items():
        require(sha(read(out / 'snapshot' / name)) == pin, 'snapshot.changed:' + name)
    for label, record in report['commands'].items():
        require(record == json.loads(read(out / 'commands' / (label + '.json'))), 'command.metadata_changed')
        require(record['exit_code'] == record['expected_exit_code'] and record['interrupted'] is None,
                'command.bad_terminal')
        for suffix in ('stdin', 'stdout', 'stderr'):
            require(sha(read(out / 'commands' / (label + '.' + suffix))) == record[suffix + '_sha256'],
                    'capture.changed')
        wanted = b'trajectory rejected: order_mutant.first_support_changed\n' if label.endswith('_trajectory_order_mutant') else (
            b'trajectory rejected: order_budget.calendar_changed\n' if label.endswith('_trajectory_admissible_order_mutant') else b'')
        require(read(out / 'commands' / (label + '.stderr')) == wanted, 'command.stderr')
    module = oracle(out / 'snapshot')
    oracles, rows, commands = module.make_cases()
    ordinals, ordinal_commands = module.ordinal_commands()
    result = dict(schema='mhgp7-private-filtered-meb-judgment-v1', status='passed',
                  public_status='not_claimed', gcp='not_used', geometry={}, gates={}, mutants={})
    for mode in ('o2', 'san'):
        raw = read(out / 'commands' / (mode + '_rational.stdout')).decode()
        rational_types(raw)
        require(report['commands'][mode + '_rational']['stdin_sha256'] == sha(commands.encode()), 'input.geometry')
        result['geometry'][mode] = module.judge(raw, oracles, rows)
        require(report['commands'][mode + '_ordinals']['stdin_sha256'] == sha(ordinal_commands.encode()), 'input.ordinals')
        ordinal_rows = [strict_json(s) for s in read(out / 'commands' / (mode + '_ordinals.stdout')).decode().splitlines()]
        require(all(set(r) == {'ordinal'} and type(r['ordinal']) is int for r in ordinal_rows), 'ordinal.types')
        actual = [r['ordinal'] for r in ordinal_rows]
        require(actual == ordinals, 'ordinal.independent_enumeration')
        for gate in GATES[:-1]:
            lines = read(out / 'commands' / (mode + '_' + gate + '.stdout')).decode().splitlines()
            record = strict_json(lines[-1])
            require(record['status'] == 'passed' and record['public_status'] == 'not_claimed', 'gate.status')
            if gate == 'budget_gate':
                require(len(lines) == 60 and all(s.startswith('case=') for s in lines[:-1]), 'budget.case_lines')
                require(record['schema'] == 'mhgp7-private-filtered-meb-budget-v1' and
                        record['cases'] == 59 and record['proposal_forms'] == 50 and
                        record['prospective_violations'] == 0, 'budget.fixed_counts')
            if gate == 'geometry_gate':
                fixed = dict(ordinals=1507, scenes=176, orders=384, main_comparisons=9216,
                             boundary_comparisons=128, reference_rank_calls=384,
                             direct_form_checks=6, direct_form_rejected=4,
                             named_fast_q2=8, named_fast_q3=16, named_fast_q4=52,
                             prospective_violations=0)
                require(len(lines) == 1 and record['schema'] == 'mhgp7-private-filtered-meb-geometry-v1'
                        and all(record[k] == v for k, v in fixed.items()), 'geometry.fixed_counts')
                require(record['complete'] + record['degenerate'] + record['capped'] == 9344 and
                        all(record[k] > 0 for k in ('fast_q2', 'fast_q3', 'fast_q4',
                            'q4_two_pivots', 'q4_high_limb', 'exhausted_fallback', 'shell_fallback')),
                        'geometry.nonvacuity')
            if gate == 'trajectory_gate':
                fixed = dict(local_permutations=62, local_diameter=32, local_23=2,
                             local_34=6, local_43=24, local_44=24, local_extra_shell=6,
                             native_calls=180, false_domains=3, test_side_order_mutants=1,
                             admissible_order_local_calls=8, admissible_order_native_calls=6,
                             admissible_order_global_replays=1, admissible_order_budget_differences=3,
                             admissible_order_same_support=3)
                require(record['schema'] == 'mhgp7-private-filtered-meb-trajectory-v1' and
                        all(type(record[k]) is int and record[k] == v for k, v in fixed.items()) and
                        record['native_success'] + record['native_refusal'] == 180 and
                        record['native_ambiguity_claim'] is False and record['engine_integration'] is False,
                        'trajectory.fixed_counts_and_scope')
            result['gates'][mode + '_' + gate] = record
        result['mutants'][mode + '_trajectory_order_mutant'] = 'order_mutant.first_support_changed'
        result['mutants'][mode + '_trajectory_admissible_order_mutant'] = 'order_budget.calendar_changed'
        for gate in ('budget_gate', 'geometry_gate'):
            record = json.loads(read(out / 'commands' / (mode + '_' + gate + '_charge_after.stdout')).decode().splitlines()[-1])
            require(record['status'] == 'causal_violation' and record['cause'] == 'charge_not_prospective' and
                    record['prospective_violations'] > 0, 'mutant.prospective_cause')
            result['mutants'][mode + '_' + gate] = record['prospective_violations']
    for gate in GATES[:-1]:
        require(result['gates']['o2_' + gate] == result['gates']['san_' + gate], 'gate.optimization_mismatch')
    require(result['geometry']['o2'] == result['geometry']['san'], 'rational.optimization_mismatch')
    for name, (_, _, _, wanted) in MUTATIONS.items():
        require(report['commands'][name]['stdin_sha256'] == sha(commands.encode()), 'mutant.input')
        try:
            raw = read(out / 'commands' / (name + '.stdout')).decode()
            rational_types(raw)
            module.judge(raw, oracles, rows)
        except ValueError as exc:
            require(str(exc).startswith(wanted), 'mutant.wrong_cause:' + str(exc))
            result['mutants'][name] = str(exc)
        else:
            raise ValueError('mutant.survived:' + name)
    result['ordinals_per_build'] = len(ordinals)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--snapshot', action='store_true')
    modes.add_argument('--execute', type=Path)
    modes.add_argument('--judge', type=Path)
    parser.add_argument('--source-sha')
    parser.add_argument('--run-sha')
    args = parser.parse_args()
    if args.snapshot:
        pins = {n: sha(raw) for n, raw in snapshot().items()}
        print(json.dumps(dict(files=len(pins), source_sha256=sha(encoded(pins)))))
    elif args.execute:
        execute(args.execute.resolve(), args.source_sha)
    else:
        print(json.dumps(judge(args.judge.resolve(), args.source_sha, args.run_sha), sort_keys=True))


if __name__ == '__main__':
    def interrupted(signum, _frame):
        raise RuntimeError('controller.signal:' + str(signum))

    for watched in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(watched, interrupted)
    try:
        main()
    except (ValueError, OSError, RuntimeError) as error:
        print(json.dumps(dict(status='failed', error=str(error))), file=sys.stderr)
        sys.exit(1)
