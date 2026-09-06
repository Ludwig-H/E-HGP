#!/usr/bin/env python3
"""Inert, fresh local product-port recheck; C++ runs only with --execute.

Three binaries per O2/SAN build: budget, geometry, rational bridge. Physical
mutants and FULL/Boost qualification belong to separate controllers. Snapshot
layout preserves repository-relative includes byte for byte (zero rewrites).
--judge reads captured bytes only; use both normal Python and python -O, with
judgment stdout kept OUTSIDE the sealed capture directory. No public claim.
This private judge requires the original absolute capture path and its ELF
files. A published judgment alone is evidence of that captured check, NOT a
portable independently replayable source-only qualification package.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import resource
import shlex
import signal
import subprocess
import sys
import time
import types

ROOT = Path('/workspaces/E-HGP')
SELF = Path(__file__).resolve()
REL = 'build/v7_meb_product_port_recheck_20260906'
COMMON = 'build/v7_meb_filter_qualification_20260906/capture.py'
GATES = ('budget_gate', 'geometry_gate', 'rational_bridge')
MODES = ('o2', 'san')
PINS = {
    COMMON: '0f5f0f6c9cb5b86117cb92334d764e16baee060089f177e1e688a42eae6a874c',
    REL + '/budget_gate.cpp': '298c44bc9fbf148a644bc020d1b4ddb74e5420d854bcada1b7ab4b11edd311b8',
    REL + '/geometry_gate.cpp': '79720a35c6a19fe61f551c8bf4c4fecb14dd181743d4c9c0d94edd0ef4bb795c',
    REL + '/rational_bridge.cpp': '4156a9d5b1cc30be1b594c376a7415802967091c52471c88093d6cda6ffcc8ec',
    REL + '/additional_scenes.inc': '6ced272e70bb3527a8b53728442b774508c1a9e7e49413bf2739e2774e6c0d51',
    'morsehgp3D_v7/src/forest/meb_proposal.hpp':
        'f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3',
    'morsehgp3D_v7/src/forest/silent_incidence.hpp':
        'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76',
    'morsehgp3D_v7/audits/meb_dual_oracle.py':
        'f5c277e24e077d02b3426ce7973954503d6b00c536cb329e63d368e73046716a',
    'morsehgp3D_v7/audits/meb_rational_oracle_20260905.py':
        'ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b',
}
SCHEMA = 'mhgp7-product-meb-extra-capture-v1'
ENV = dict(LC_ALL='C', PYTHONDONTWRITEBYTECODE='1',
           ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
BASE_FLAGS = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']
SAN_FLAGS = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    require(path.is_file() and not path.is_symlink(), 'file.not_regular:' + str(path))
    a = path.stat()
    raw = path.read_bytes()
    b = path.stat()
    require((a.st_ino, a.st_size, a.st_mtime_ns, a.st_ctime_ns) ==
            (b.st_ino, b.st_size, b.st_mtime_ns, b.st_ctime_ns), 'file.concurrent_change')
    return raw


def encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n').encode()


def valid_name(name):
    return type(name) is str and bool(name) and not Path(name).is_absolute() and '..' not in Path(name).parts


def save(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)


def stamp():
    return datetime.now(timezone.utc).isoformat()


def common(root):
    path = root / COMMON
    raw = read(path)
    require(sha(raw) == PINS[COMMON], 'common.pin')
    module = types.ModuleType('product_extra_pure_helpers')
    module.__file__ = str(path)
    # Reuse only strict_json, rational_types and the pure, pinned oracle loader.
    # Neither historical main, snapshot, execute nor judge is called.
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module


def sources():
    names = set(PINS) | {str(SELF.relative_to(ROOT))}
    names |= {str(p.relative_to(ROOT)) for p in (ROOT / 'morsehgp3D_v7/src').rglob('*') if p.is_file()}
    values = {name: read(ROOT / name) for name in sorted(names)}
    require(all(sha(values[n]) == p for n, p in PINS.items()), 'source.fixed_pin')
    return values


def filemap(base):
    return {str(p.relative_to(base)): sha(read(p)) for p in sorted(base.rglob('*'))
            if p.is_file() and p != base / 'run.json'}


def inputs(module):
    oracles, rows, geometry = module.make_cases()
    ordinals, ordinal_input = module.ordinal_commands()
    require(len(oracles) == 178 and len(rows) == 3430 and len(ordinals) == 1507,
            'oracle.fixed_input_counts')
    return oracles, rows, ordinals, geometry.encode(), ordinal_input.encode()


def plan(out, compiler, geometry, ordinals):
    snap = out / 'snapshot'
    rows = {'compiler': ([compiler, '--version'], 0, b'', 120)}
    for mode in MODES:
        for gate in GATES:
            name = mode + '_' + gate
            binary = str(out / 'bin' / name)
            flags = ['-O2'] if mode == 'o2' else SAN_FLAGS
            rows[name + '_compile'] = ([compiler, *BASE_FLAGS, *flags,
                '-I', str(snap / 'morsehgp3D_v7'), '-MMD', '-MF',
                str(out / 'dependencies' / (name + '.d')),
                str(snap / REL / (gate + '.cpp')), '-o', binary], 0, b'', 300)
            if gate == 'rational_bridge':
                rows[mode + '_rational'] = ([binary], 0, geometry, 120)
                rows[mode + '_ordinals'] = ([binary], 0, ordinals, 120)
            else:
                rows[name] = ([binary], 0, b'', 120)
            rows[name + '_unknown'] = ([binary, '--unknown'], 2, b'', 120)
    require(len(rows) == 21, 'plan.fixed_21_commands')
    return rows


def dependency_map(out, name):
    raw = read(out / 'dependencies' / (name + '.d')).decode().replace('\\\n', ' ')
    target, body = raw.split(':', 1)
    require(shlex.split(target) == [str(out / 'bin' / name)], 'dependency.target')
    snap = out / 'snapshot'
    paths = [Path(p).resolve() for p in shlex.split(body)]
    require(paths and len(set(paths)) == len(paths) and
            all(p.is_relative_to(snap) for p in paths), 'dependency.outside_snapshot_or_duplicate')
    result = {str(p.relative_to(snap)): sha(read(p)) for p in paths}
    gate = name.split('_', 1)[1]
    require({REL + '/' + gate + '.cpp', 'morsehgp3D_v7/src/forest/meb_proposal.hpp',
             'morsehgp3D_v7/src/forest/silent_incidence.hpp'} <= set(result), 'dependency.missing_product')
    return result


def judgments(out, helper, module, data):
    oracles, rows, ordinals, _, _ = data
    result = dict(gates={}, rational={}, ordinals_per_build=1507, rational_rows_per_build=3430,
                  physical_A_in_rational_protocol=False, physical_A_checked_in_adapted_gates=True)
    for mode in MODES:
        raw = read(out / 'commands' / (mode + '_rational.stdout')).decode()
        helper.rational_types(raw)
        result['rational'][mode] = module.judge(raw, oracles, rows)
        actual = [helper.strict_json(line) for line in
                  read(out / 'commands' / (mode + '_ordinals.stdout')).splitlines()]
        require(all(type(r) is dict and set(r) == {'ordinal'} and type(r['ordinal']) is int
                    for r in actual), 'ordinal.strict_types')
        require([r['ordinal'] for r in actual] == ordinals, 'ordinal.independent_enumeration')
        for gate in GATES[:2]:
            lines = read(out / 'commands' / (mode + '_' + gate + '.stdout')).splitlines()
            require(bool(lines), 'gate.empty_stdout')
            row = helper.strict_json(lines[-1])
            schema = 'mhgp7-product-port-meb-' + ('budget' if gate == 'budget_gate' else 'geometry') + '-v1'
            require(type(row) is dict and row.get('schema') == schema and row.get('status') == 'passed'
                    and row.get('public_status') == 'not_claimed' and row.get('cause') == 'none', 'gate.scope')
            strings = {'schema', 'status', 'public_status', 'cause'}
            if gate == 'budget_gate':
                strings.add('accounting')
                fixed = dict(cases=59, terminal_comparisons=118, NoObserver_comparisons=59,
                    successes=25, budget_refusals=28, shell_refusals=6, proposal_forms=50,
                    q2=34, q3=16, q4=0, pair_selections=34, pivots=23, fallback=25,
                    certified=20, legacy_increments=118, F_fallback_candidates=65,
                    prospective_violations=0)
                require(len(lines) == 60 and all(s.startswith(b'case=') for s in lines[:-1]), 'budget.case_lines')
                require(row.get('accounting') == 'reference_ordinal_plus_native_z_q3_q4_proposal_v2', 'budget.accounting')
                require(set(row) == strings | set(fixed), 'budget.exact_fields')
            else:
                fixed = dict(ordinals=1507, scenes=176, orders=384, main_comparisons=9216,
                    boundary_comparisons=128, reference_rank_calls=384, direct_form_checks=6,
                    direct_form_rejected=4, named_fast_q2=8, named_fast_q3=16, named_fast_q4=52,
                    prospective_violations=0)
                extra = {'proposal_forms', 'pair_selections', 'legacy_charges', 'actual_fallback_candidates',
                    'certified', 'fallback', 'complete', 'degenerate', 'capped', 'fast_q2', 'fast_q3', 'fast_q4',
                    'q4_two_pivots', 'q4_high_limb', 'exhausted_fallback', 'initial_p_fallback',
                    'shell_fallback', 'forced_fallback'}
                require(len(lines) == 1 and set(row) == strings | set(fixed) | extra, 'geometry.exact_fields')
                require(row['complete'] + row['degenerate'] + row['capped'] == 9344 and
                        all(row[k] > 0 for k in extra), 'geometry.nonvacuum')
            require(all(type(v) is str if k in strings else type(v) is int and v >= 0
                        for k, v in row.items()), 'gate.strict_types')
            require(all(row[k] == v for k, v in fixed.items()), 'gate.fixed_counts')
            result['gates'][mode + '_' + gate] = row
    require(encoded(result['rational']['o2']) == encoded(result['rational']['san']), 'rational.optimization_match')
    for gate in GATES[:2]:
        require(encoded(result['gates']['o2_' + gate]) == encoded(result['gates']['san_' + gate]),
                'gate.optimization_match')
    return result


def execute(out, expected_source, self_pin):
    require(sha(read(SELF)) == self_pin, 'controller.not_admitted')
    values = sources()
    pins = {n: sha(raw) for n, raw in values.items()}
    require(sha(encoded(pins)) == expected_source, 'sources.not_admitted')
    require(not out.exists() and out.is_relative_to(ROOT / 'build'), 'capture.fresh_build_path_required')
    out.mkdir(parents=True, exist_ok=False)
    snap = out / 'snapshot'
    compiler = Path('/usr/bin/c++').resolve()
    report = dict(schema=SCHEMA, status='running', public_status='not_claimed',
        phase='exploration_v7_hors_registre', backend='cpu_reference',
        profile='quantized_u16_input_only', mode='audit_independant_math_and_architecture',
        performance_capture=False, gcp='not_used', mutants='separate_controller_not_run_here',
        controller_sha256=self_pin, source_sha256=expected_source, started_utc=stamp(),
        compiler=str(compiler), compiler_sha256=sha(read(compiler)), env_overrides=ENV,
        global_seconds=1800, commands={}, binaries={}, dependencies={})
    env = {k: v for k, v in os.environ.items() if k not in {
        'LD_PRELOAD', 'LD_AUDIT', 'LD_LIBRARY_PATH', 'CPATH', 'CPLUS_INCLUDE_PATH',
        'C_INCLUDE_PATH', 'GCC_EXEC_PREFIX', 'COMPILER_PATH', 'LIBRARY_PATH',
        'CPPFLAGS', 'CXXFLAGS', 'LDFLAGS', 'ASAN_OPTIONS', 'LSAN_OPTIONS', 'UBSAN_OPTIONS', 'PYTHONPATH'}}
    env.update(ENV)
    deadline = time.monotonic() + 1800

    def command(label, item):
        argv, expected, stdin, limit = item
        file_size_bytes = (512 if label.endswith('_compile') else 64) << 20
        require(time.monotonic() < deadline, 'campaign.deadline')
        stem = out / 'commands' / label
        rec = dict(argv=argv, cwd=str(snap), expected_exit_code=expected, started_utc=stamp(),
                   timeout_seconds=limit, cpu_seconds=limit, stdin_sha256=sha(stdin),
                   file_size_bytes=file_size_bytes,
                   executable_sha256_before=sha(read(Path(argv[0]))),
                   source_sha256_before=sha(encoded({n: sha(read(snap / n)) for n in pins})))
        save(stem.with_suffix('.intent.json'), encoded(rec))
        save(stem.with_suffix('.stdin'), stdin)
        proc, error = None, None
        begin = time.monotonic()

        def child_limits():
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            resource.setrlimit(resource.RLIMIT_CPU, (limit, limit))
            resource.setrlimit(resource.RLIMIT_FSIZE, (file_size_bytes, file_size_bytes))

        try:
            with stem.with_suffix('.stdin').open('rb') as source, \
                    stem.with_suffix('.stdout').open('xb') as output, \
                    stem.with_suffix('.stderr').open('xb') as errors:
                proc = subprocess.Popen(argv, cwd=snap, env=env, stdin=source, stdout=output,
                    stderr=errors, start_new_session=True, preexec_fn=child_limits)
                proc.wait(timeout=min(limit, deadline - time.monotonic()))
        except BaseException as exc:
            error = type(exc).__name__ + ': ' + str(exc)
        finally:
            residual = False
            if proc is not None:
                try:
                    os.killpg(proc.pid, 0)
                    residual = True
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                proc.wait()
            closed = proc is not None
            if proc is not None:
                try:
                    os.killpg(proc.pid, 0)
                    closed = False
                except ProcessLookupError:
                    pass
            rec.update(pid=None if proc is None else proc.pid, process_group_closed=closed,
                residual_group_detected=residual, interrupted=error,
                exit_code=None if proc is None else proc.returncode, ended_utc=stamp(),
                elapsed_seconds=time.monotonic() - begin)
            try:
                rec['executable_sha256_after'] = sha(read(Path(argv[0])))
                rec['source_sha256_after'] = sha(encoded({n: sha(read(snap / n)) for n in pins}))
            except (ValueError, OSError):
                rec['executable_sha256_after'] = rec['source_sha256_after'] = None
            for suffix in ('stdout', 'stderr'):
                path = stem.with_suffix('.' + suffix)
                if not path.exists():
                    save(path, b'')
                rec[suffix + '_sha256'] = sha(read(path))
            save(stem.with_suffix('.json'), encoded(rec))
            report['commands'][label] = rec
        print(json.dumps(dict(command=label, exit_code=rec['exit_code'], closed=closed)), flush=True)
        require(error is None and not residual and closed and rec['exit_code'] == expected,
                'command.not_cleanly_closed:' + label)
        require(rec['executable_sha256_before'] == rec['executable_sha256_after'] and
                rec['source_sha256_before'] == rec['source_sha256_after'] == expected_source,
                'command.executable_or_source_changed:' + label)
        require(read(stem.with_suffix('.stderr')) == b'', 'command.stderr:' + label)
        if label.endswith('_unknown'):
            require(read(stem.with_suffix('.stdout')) == b'', 'argument.unexpected_stdout')

    try:
        for name, raw in values.items():
            save(snap / name, raw)
        save(out / 'sources_before.json', encoded(pins))
        (out / 'bin').mkdir()
        (out / 'dependencies').mkdir()
        helper = common(snap)
        module = helper.oracle(snap)
        data = inputs(module)
        save(out / 'geometry.stdin', data[3])
        save(out / 'ordinals.stdin', data[4])
        for label, item in plan(out, str(compiler), data[3], data[4]).items():
            command(label, item)
            if label.endswith('_compile'):
                name = label[:-8]
                deps = dependency_map(out, name)
                require(all(n in pins and pins[n] == p for n, p in deps.items()), 'dependency.not_pinned')
                save(out / 'dependencies' / (name + '.json'), encoded(deps))
                report['dependencies'][name] = deps
                report['binaries'][name] = sha(read(out / 'bin' / name))
        report['judgments'] = judgments(out, helper, module, data)
        require(time.monotonic() <= deadline, 'campaign.deadline')
        require(sha(read(compiler)) == report['compiler_sha256'], 'compiler.changed')
        require(all(sha(read(out / 'bin' / n)) == p for n, p in report['binaries'].items()), 'binary.changed')
        report['status'] = 'completed'
    except BaseException as exc:
        report.update(status='failed', failure=type(exc).__name__ + ': ' + str(exc))
    finally:
        try:
            after = {n: sha(raw) for n, raw in sources().items()}
            save(out / 'sources_after.json', encoded(after))
            report['sources_stable'] = after == pins and all(sha(read(snap / n)) == p for n, p in pins.items())
        except BaseException as exc:
            report['sources_stable'] = False
            report['source_failure'] = type(exc).__name__ + ': ' + str(exc)
        if not report['sources_stable']:
            report['status'] = 'failed'
        report['ended_utc'] = stamp()
        report['artifacts'] = filemap(out)
        save(out / 'run.json', encoded(report))
    print(json.dumps(dict(status=report['status'], run_sha256=sha(read(out / 'run.json')))), flush=True)
    return 0 if report['status'] == 'completed' else 1


def judge(out, expected_source, expected_run, self_pin):
    require(sha(read(SELF)) == self_pin, 'controller.not_admitted')
    require(sha(read(out / 'run.json')) == expected_run, 'run.not_admitted')
    helper = common(out / 'snapshot')
    parse = helper.strict_json
    report = parse(read(out / 'run.json'))
    require(report['schema'] == SCHEMA and report['status'] == 'completed' and
            report['public_status'] == 'not_claimed' and report['sources_stable'] is True and
            report['performance_capture'] is False and report['gcp'] == 'not_used' and
            report['controller_sha256'] == self_pin and type(report['global_seconds']) is int and
            report['global_seconds'] == 1800,
            'run.scope_or_completion')
    require(report['phase'] == 'exploration_v7_hors_registre' and report['backend'] == 'cpu_reference' and
            report['profile'] == 'quantized_u16_input_only' and
            report['mode'] == 'audit_independant_math_and_architecture' and
            report['mutants'] == 'separate_controller_not_run_here', 'run.normative_scope')
    require(encoded(filemap(out)) == encoded(report['artifacts']), 'artifacts.exact_closed_map')
    pins = parse(read(out / 'sources_before.json'))
    require(type(pins) is dict and all(valid_name(n) and type(p) is str and len(p) == 64
            and all(c in '0123456789abcdef' for c in p) for n, p in pins.items()), 'source.strict_map')
    require(sha(encoded(pins)) == expected_source == report['source_sha256'] and
            encoded(pins) == encoded(parse(read(out / 'sources_after.json'))), 'source.admission_and_stability')
    require(all(n in pins and pins[n] == p for n, p in PINS.items()), 'source.fixed_pins')
    own_name = str(SELF.relative_to(ROOT))
    require(pins[own_name] == self_pin and encoded(filemap(out / 'snapshot')) == encoded(pins),
            'snapshot.changed_or_wrong_controller')
    module = helper.oracle(out / 'snapshot')
    data = inputs(module)
    require(read(out / 'geometry.stdin') == data[3] and read(out / 'ordinals.stdin') == data[4], 'input.pure_regeneration')
    wanted = plan(out, report['compiler'], data[3], data[4])
    require(set(report['commands']) == set(wanted), 'commands.exact_matrix')
    require(encoded(report['env_overrides']) == encoded(ENV), 'environment.admitted')
    start = datetime.fromisoformat(report['started_utc'])
    finish = datetime.fromisoformat(report['ended_utc'])
    require(start.tzinfo is not None and finish.tzinfo is not None and
            0 <= (finish - start).total_seconds() <= 1805, 'campaign.utc_bound')
    previous_end = start
    for label, (argv, code, stdin, limit) in wanted.items():
        stem = out / 'commands' / label
        row = parse(read(stem.with_suffix('.json')))
        require(encoded(row) == encoded(report['commands'][label]), 'command.mirror')
        require(encoded(row['argv']) == encoded(argv) and row['cwd'] == str(out / 'snapshot') and
                type(row['exit_code']) is int and row['exit_code'] == code and
                type(row['expected_exit_code']) is int and row['expected_exit_code'] == code and
                row['interrupted'] is None and row['process_group_closed'] is True and
                row['residual_group_detected'] is False, 'command.terminal_binding')
        require(type(row['pid']) is int and row['pid'] > 0 and type(row['timeout_seconds']) is int and
                row['timeout_seconds'] == limit and type(row['cpu_seconds']) is int and row['cpu_seconds'] == limit,
                'command.limits_and_pid')
        require(type(row['file_size_bytes']) is int and
                row['file_size_bytes'] == ((512 if label.endswith('_compile') else 64) << 20),
                'command.file_size_limit')
        elapsed = row['elapsed_seconds']
        require(type(elapsed) in (int, float) and math.isfinite(elapsed) and 0 <= elapsed <= limit + 5,
                'command.elapsed')
        started = datetime.fromisoformat(row['started_utc'])
        ended = datetime.fromisoformat(row['ended_utc'])
        require(started.tzinfo is not None and ended.tzinfo is not None and
                previous_end <= started <= ended <= finish, 'command.utc_interval')
        previous_end = ended
        intent = {k: row[k] for k in ('argv', 'cwd', 'expected_exit_code', 'started_utc',
                  'timeout_seconds', 'cpu_seconds', 'file_size_bytes', 'stdin_sha256',
                  'executable_sha256_before', 'source_sha256_before')}
        require(encoded(intent) == encoded(parse(read(stem.with_suffix('.intent.json')))), 'command.intent_binding')
        executable_pin = (report['compiler_sha256'] if label == 'compiler' or label.endswith('_compile')
                          else report['binaries'][Path(argv[0]).name])
        require(row['executable_sha256_before'] == row['executable_sha256_after'] == executable_pin and
                row['source_sha256_before'] == row['source_sha256_after'] == expected_source,
                'command.executable_and_source_bindings')
        require(read(stem.with_suffix('.stdin')) == stdin, 'command.stdin')
        for suffix in ('stdin', 'stdout', 'stderr'):
            require(sha(read(stem.with_suffix('.' + suffix))) == row[suffix + '_sha256'], 'stream.changed')
        require(read(stem.with_suffix('.stderr')) == b'', 'command.stderr')
        if label.endswith('_unknown'):
            require(read(stem.with_suffix('.stdout')) == b'', 'argument.stdout')
    binaries = {mode + '_' + gate for mode in MODES for gate in GATES}
    require(set(report['binaries']) == set(report['dependencies']) == binaries, 'builds.exact_matrix')
    for name in binaries:
        require(sha(read(out / 'bin' / name)) == report['binaries'][name], 'binary.changed')
        deps = dependency_map(out, name)
        require(encoded(deps) == encoded(report['dependencies'][name]) ==
                encoded(parse(read(out / 'dependencies' / (name + '.json')))) and
                all(n in pins and p == pins[n] for n, p in deps.items()), 'dependencies.binding')
    results = judgments(out, helper, module, data)
    require(encoded(results) == encoded(report['judgments']), 'judgments.recomputed')
    return dict(schema='mhgp7-product-meb-extra-judgment-v1', status='passed', public_status='not_claimed',
                run_sha256=expected_run, source_sha256=expected_source, commands=21, builds=6,
                judgments=results, cpp_reexecuted=False, mutations_judged_here=0, gcp='not_used')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--snapshot', action='store_true')
    modes.add_argument('--execute', type=Path)
    modes.add_argument('--judge', type=Path)
    parser.add_argument('--source-sha')
    parser.add_argument('--run-sha')
    parser.add_argument('--self-sha')
    args = parser.parse_args()
    if args.snapshot:
        pins = {n: sha(raw) for n, raw in sources().items()}
        print(json.dumps(dict(files=len(pins), source_sha256=sha(encoded(pins)), controller_sha256=sha(read(SELF)))))
        return 0
    require(args.source_sha and args.self_sha, 'admission.source_and_controller_required')
    if args.execute:
        return execute(args.execute.resolve(), args.source_sha, args.self_sha)
    require(args.run_sha, 'admission.run_required')
    print(json.dumps(judge(args.judge.resolve(), args.source_sha, args.run_sha, args.self_sha), sort_keys=True))
    return 0


if __name__ == '__main__':
    def interrupted(signum, _frame):
        raise RuntimeError('controller.signal:' + str(signum))
    for watched in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(watched, interrupted)
    try:
        raise SystemExit(main())
    except (ValueError, OSError, RuntimeError, KeyError, TypeError) as error:
        print(json.dumps(dict(status='failed', error=str(error))), file=sys.stderr)
        raise SystemExit(1)
