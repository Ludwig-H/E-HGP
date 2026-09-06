#!/usr/bin/env python3
"""Portable read-only replay of pinned extra captures; no C++ or ELF is opened.

Only copied COMMON.strict_json/rational_types/oracle and rational pure functions
are called. Historical paths/timelines are records, never execution targets.
"""
import ast
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import posixpath
import shlex
import sys
import types

RUN = 'a709d26382b55820a6ee268e0aec49098f659869ed75419ba82241ee2f456ac2'
SOURCE = 'fd840afc5f134cdfb5052a358f3019323fc941bb53840b2e4658271a4dc567af'
RECORDER = 'aa8ef1651d7a3f829c88706893ed59d02cb42eeb604f038cf1d1137dcb85396b'
COMMON = '0f5f0f6c9cb5b86117cb92334d764e16baee060089f177e1e688a42eae6a874c'
RECORD_PATH = 'build/v7_meb_product_extra_qualification_20260906/record.py'
GATES, MODES = ('budget_gate', 'geometry_gate', 'rational_bridge'), ('o2', 'san')


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(obj):
    return (json.dumps(obj, sort_keys=True, indent=2, allow_nan=False) + '\n').encode()


def safe(name):
    return type(name) is str and bool(name) and str(PurePosixPath(name)) == name and not PurePosixPath(name).is_absolute() and '..' not in PurePosixPath(name).parts


def read(path):
    require(path.is_file() and not path.is_symlink(), 'regular_file:' + str(path))
    before = path.stat(); raw = path.read_bytes(); after = path.stat()
    require((before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns) ==
            (after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns), 'concurrent_change')
    return raw


def filemap(root):
    result = {}
    for path in sorted(root.rglob('*')):
        require(not path.is_symlink(), 'symlink_in_capture')
        name = path.relative_to(root).as_posix()
        if name == 'bin' or name.startswith('bin/') or name == 'run.json':
            continue  # Six captured ELF hashes stay in run.json; no ELF bytes are read.
        if path.is_file():
            result[name] = sha(read(path))
    return result


def plan(historical, compiler, rel, geometry, ordinals):
    snap = historical / 'snapshot'; rows = {'compiler': ([compiler, '--version'], 0, b'', 120)}
    for mode in MODES:
        for gate in GATES:
            name = mode + '_' + gate; binary = str(historical / 'bin' / name)
            flags = ['-O2'] if mode == 'o2' else ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            rows[name + '_compile'] = ([compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', *flags,
                '-I', str(snap / 'morsehgp3D_v7'), '-MMD', '-MF', str(historical / 'dependencies' / (name + '.d')),
                str(snap / rel / (gate + '.cpp')), '-o', binary], 0, b'', 300)
            if gate == 'rational_bridge':
                rows[mode + '_rational'] = ([binary], 0, geometry, 120)
                rows[mode + '_ordinals'] = ([binary], 0, ordinals, 120)
            else:
                rows[name] = ([binary], 0, b'', 120)
            rows[name + '_unknown'] = ([binary, '--unknown'], 2, b'', 120)
    return rows


def gates(root, helper, oracle, oracles, rows, ordinals):
    result = dict(gates={}, rational={}, ordinals_per_build=1507, rational_rows_per_build=3430,
                  physical_A_in_rational_protocol=False, physical_A_checked_in_adapted_gates=True)
    for mode in MODES:
        raw = read(root / 'commands' / (mode + '_rational.stdout')).decode()
        helper.rational_types(raw); result['rational'][mode] = oracle.judge(raw, oracles, rows)
        actual = [helper.strict_json(s) for s in read(root / 'commands' / (mode + '_ordinals.stdout')).splitlines()]
        require(all(type(r) is dict and set(r) == {'ordinal'} and type(r['ordinal']) is int for r in actual) and
                [r['ordinal'] for r in actual] == ordinals, 'independent_1507_ordinals')
        for gate in GATES[:2]:
            lines = read(root / 'commands' / (mode + '_' + gate + '.stdout')).splitlines(); row = helper.strict_json(lines[-1])
            strings = {'schema', 'status', 'public_status', 'cause'}
            require(row['schema'] == 'mhgp7-product-port-meb-' + gate.removesuffix('_gate') + '-v1' and
                    row['status'] == 'passed' and row['public_status'] == 'not_claimed' and row['cause'] == 'none', 'gate_scope')
            if gate == 'budget_gate':
                strings.add('accounting')
                fixed = dict(cases=59, terminal_comparisons=118, NoObserver_comparisons=59, successes=25, budget_refusals=28,
                    shell_refusals=6, proposal_forms=50, q2=34, q3=16, q4=0, pair_selections=34, pivots=23, fallback=25,
                    certified=20, legacy_increments=118, F_fallback_candidates=65, prospective_violations=0)
                extra = set()
                require(len(lines) == 60 and all(s.startswith(b'case=') for s in lines[:-1]) and
                        row['accounting'] == 'reference_ordinal_plus_native_z_q3_q4_proposal_v2', 'budget_59_cases')
            else:
                fixed = dict(ordinals=1507, scenes=176, orders=384, main_comparisons=9216, boundary_comparisons=128,
                    reference_rank_calls=384, direct_form_checks=6, direct_form_rejected=4, named_fast_q2=8,
                    named_fast_q3=16, named_fast_q4=52, prospective_violations=0)
                extra = set('proposal_forms pair_selections legacy_charges actual_fallback_candidates certified fallback complete degenerate capped fast_q2 fast_q3 fast_q4 q4_two_pivots q4_high_limb exhausted_fallback initial_p_fallback shell_fallback forced_fallback'.split())
                require(len(lines) == 1 and row['complete'] + row['degenerate'] + row['capped'] == 9344 and
                        all(row[k] > 0 for k in extra), 'geometry_9344_nonvacuum')
            require(set(row) == strings | set(fixed) | extra and all(type(v) is str if k in strings else type(v) is int and v >= 0
                    for k, v in row.items()) and all(row[k] == v for k, v in fixed.items()), 'gate_strict_fields_counts')
            result['gates'][mode + '_' + gate] = row
    require(encoded(result['rational']['o2']) == encoded(result['rational']['san']) and all(
        encoded(result['gates']['o2_' + g]) == encoded(result['gates']['san_' + g]) for g in GATES[:2]), 'O2_SAN_parity')
    return result


def main():
    require(len(sys.argv) == 2, 'usage: verify_extra.py <capture>')
    root = Path(sys.argv[1]).resolve(); snap = root / 'snapshot'
    raw = read(root / 'run.json'); require(sha(raw) == RUN, 'run_pin'); report = json.loads(raw)
    require(report['schema'] == 'mhgp7-product-meb-extra-capture-v1' and report['status'] == 'completed' and
            report['sources_stable'] is True and report['performance_capture'] is False and report['gcp'] == 'not_used' and
            report['public_status'] == 'not_claimed' and report['controller_sha256'] == RECORDER, 'capture_scope')
    require(report['phase'] == 'exploration_v7_hors_registre' and report['backend'] == 'cpu_reference' and
            report['profile'] == 'quantized_u16_input_only' and report['mode'] == 'audit_independant_math_and_architecture' and
            report['mutants'] == 'separate_controller_not_run_here' and type(report['global_seconds']) is int and report['global_seconds'] == 1800, 'normative_scope')
    expected = {n: p for n, p in report['artifacts'].items() if not n.startswith('bin/')}
    require(all(safe(n) for n in report['artifacts']) and filemap(root) == expected, 'exact_artifacts_except_ELF')
    record_raw = read(snap / RECORD_PATH); require(sha(record_raw) == RECORDER, 'copied_recorder_pin')
    literals = {n.targets[0].id: ast.literal_eval(n.value) for n in ast.parse(record_raw).body
                if isinstance(n, ast.Assign) and len(n.targets) == 1 and isinstance(n.targets[0], ast.Name) and n.targets[0].id in ('COMMON', 'REL')}
    require(set(literals) == {'COMMON', 'REL'} and all(safe(v) for v in literals.values()), 'copied_relative_routing')
    common_path = snap / literals['COMMON']; common_raw = read(common_path); require(sha(common_raw) == COMMON, 'COMMON_pin')
    helper = types.ModuleType('portable_extra_pure_common'); helper.__file__ = str(common_path)
    exec(compile(common_raw, str(common_path), 'exec'), helper.__dict__)  # No main/execute/judge call.
    parse = helper.strict_json
    pins = parse(read(root / 'sources_before.json'))
    require(len(pins) == 58 and sha(encoded(pins)) == SOURCE == report['source_sha256'] and
            encoded(pins) == encoded(parse(read(root / 'sources_after.json'))) and filemap(snap) == pins, '58_source_maps')
    oracle = helper.oracle(snap); oracles, rows, geometry = oracle.make_cases(); ordinals, ordinal_input = oracle.ordinal_commands()
    require(len(oracles) == 178 and len(rows) == 3430 and len(ordinals) == 1507, 'oracle_input_counts')
    require(read(root / 'geometry.stdin') == geometry.encode() and read(root / 'ordinals.stdin') == ordinal_input.encode(), 'pure_input_regeneration')
    historical_snap = PurePosixPath(report['commands']['compiler']['cwd']); historical = historical_snap.parent
    require(historical_snap.is_absolute() and historical_snap.name == 'snapshot', 'historical_layout')
    wanted = plan(historical, report['compiler'], literals['REL'], geometry.encode(), ordinal_input.encode())
    require(len(wanted) == 21 and set(wanted) == set(report['commands']), '21_commands')
    start, finish = (datetime.fromisoformat(report[k]) for k in ('started_utc', 'ended_utc')); previous = start
    require(start.tzinfo is not None and finish.tzinfo is not None and 0 <= (finish - start).total_seconds() <= 1805, 'captured_campaign_interval')
    for label, (argv, code, stdin, limit) in wanted.items():
        stem = root / 'commands' / label; row = parse(read(stem.with_suffix('.json')))
        require(encoded(row) == encoded(report['commands'][label]) and encoded(row['argv']) == encoded(argv) and
                row['cwd'] == str(historical_snap) and type(row['exit_code']) is int and type(row['expected_exit_code']) is int and row['exit_code'] == code == row['expected_exit_code'] and
                row['interrupted'] is None and row['process_group_closed'] is True and row['residual_group_detected'] is False, 'command_terminal')
        require(all(type(row[k]) is int for k in ('pid', 'timeout_seconds', 'cpu_seconds', 'file_size_bytes')) and row['pid'] > 0 and row['timeout_seconds'] == row['cpu_seconds'] == limit and
                row['file_size_bytes'] == ((512 if label.endswith('_compile') else 64) << 20), 'captured_pid_limits')
        a, b = (datetime.fromisoformat(row[k]) for k in ('started_utc', 'ended_utc'))
        require(a.tzinfo is not None and b.tzinfo is not None and previous <= a <= b <= finish and
                type(row['elapsed_seconds']) in (int, float) and math.isfinite(row['elapsed_seconds']) and 0 <= row['elapsed_seconds'] <= limit + 5, 'captured_command_interval'); previous = b
        intent = parse(read(stem.with_suffix('.intent.json')))
        require(encoded(intent) == encoded({k: row[k] for k in ('argv', 'cwd', 'expected_exit_code', 'started_utc', 'timeout_seconds',
                'cpu_seconds', 'file_size_bytes', 'stdin_sha256', 'executable_sha256_before', 'source_sha256_before')}), 'intent_binding')
        executable = report['compiler_sha256'] if label == 'compiler' or label.endswith('_compile') else report['binaries'][PurePosixPath(argv[0]).name]
        require(row['executable_sha256_before'] == row['executable_sha256_after'] == executable and
                row['source_sha256_before'] == row['source_sha256_after'] == SOURCE, 'captured_executable_source_pins')
        require(read(stem.with_suffix('.stdin')) == stdin and all(sha(read(stem.with_suffix('.' + s))) == row[s + '_sha256']
                for s in ('stdin', 'stdout', 'stderr')) and read(stem.with_suffix('.stderr')) == b'', 'stream_binding')
        if label.endswith('_unknown'):
            require(read(stem.with_suffix('.stdout')) == b'', 'unknown_output')
    binaries = {m + '_' + g for m in MODES for g in GATES}
    require(set(report['binaries']) == set(report['dependencies']) == binaries and
            {n for n in report['artifacts'] if n.startswith('bin/')} == {'bin/' + n for n in binaries}, 'six_captured_ELF_identities')
    for name in sorted(binaries):
        require(report['artifacts']['bin/' + name] == report['binaries'][name], 'ELF_captured_hash_only')
        target, body = read(root / 'dependencies' / (name + '.d')).decode().replace('\\\n', ' ').split(':', 1)
        names = [str(PurePosixPath(posixpath.normpath(p)).relative_to(historical_snap)) for p in shlex.split(body)]
        require(shlex.split(target) == [str(historical / 'bin' / name)] and len(names) == len(set(names)) and names, 'captured_dependency_paths')
        deps = {n: pins[n] for n in names}
        require(encoded(deps) == encoded(report['dependencies'][name]) == encoded(parse(read(root / 'dependencies' / (name + '.json')))), 'source_dependency_binding')
    results = gates(root, helper, oracle, oracles, rows, ordinals)
    require(encoded(results) == encoded(report['judgments']) and filemap(root) == expected and sha(read(root / 'run.json')) == RUN, 'recomputed_judgments_and_final_bytes')
    print(json.dumps(dict(schema='mhgp7-portable-product-meb-extra-verification-v1', status='passed', public_status='not_claimed',
        run_sha256=RUN, source_sha256=SOURCE, artifacts=len(expected), sources=58, commands=21, captured_binary_hashes=report['binaries'],
        rational_rows_per_build=3430, ordinals_per_build=1507, budget_cases_per_build=59, geometry_cases_per_build=9344,
        cpp_reexecuted=False, ELF_read_or_requalified=False, timelines='captured_only', performance_claim=False), sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, KeyError, TypeError, OSError, IndexError, RuntimeError) as error:
        print('rejected: ' + str(error), file=sys.stderr); sys.exit(1)
