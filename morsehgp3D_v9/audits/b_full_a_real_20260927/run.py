#!/usr/bin/env python3
"""Real A harness; explicit port of frozen min-label runner 78816029.
Fresh GEN objects/archive per profile, source/system deps discovered BEFORE
compilation, LIVE readback without recompilation or geometric execution.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import shlex
import signal
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
V9 = ROOT / 'morsehgp3D_v9'
SOURCE = V9 / 'src'
MANIFEST = HERE.parent / 'b_full_a_manifest_20260927'
EVENT = HERE.parent / 'b_full_a_events_20260927'
MINIMUM = HERE.parent / 'b_full_a_min_label_20260927'
PORT = MINIMUM / 'run.py'
PORT_SHA = '78816029de56200d8b09526d1afeb931708e71154aad25206ee39983ff2313d1'
INPUTS = HERE.parent / 'b_q34_factor_plan_20260926/inputs.py'
INPUTS_SHA = '3626e3bb34beb0c209d92ccae6907d2040219da1a550333d921417253aca308c'
HELPER = ROOT / 'gcp-migration/full_probe_session_v7.py'
HELPER_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
TARGET = 'mhgp9_full_a_real'
LIBRARY = 'mhgp9_a_real_gen'
KINDS = ('release', 'sanitize')
SCHEMA = 'mhgp9_full_a_real_capture_v1'
GATE = dict(schema='mhgp9_full_a_real_gate_v1', status='passed', cases=6, orders=26,
            candidate_comparisons=156, nonidentity_index_positions=35, continuations=2, extra_blocks=16,
            GCP_used=False, FULL_executed_by_candidate=False)


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    def pairs(items):
        value = {}
        for key, item in items:
            need(key not in value, 'duplicate JSON key')
            value[key] = item
        return value
    return json.loads(Path(path).read_text(), object_pairs_hook=pairs)


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def module(path, expected, name):
    need(sha(path) == expected, 'module identity before import: ' + name)
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def units():
    text = (HERE / 'CMakeLists.txt').read_text()
    body = re.search(r'set\(GEN_UNITS (.*?)\)', text, re.S)
    need(body is not None, 'GEN inventory')
    names = body.group(1).split()
    native = re.search(r'add_library\(mhgp9_gen STATIC\s+(.*?)\)', (V9 / 'CMakeLists.txt').read_text(), re.S)
    need(native is not None and native.group(1).split() == ['src/gen/' + n for n in names], 'native GEN inventory identity')
    need(len(names) == len(set(names)) == 25, '25 distinct GEN translation units')
    return [SOURCE / 'gen' / name for name in names] + [HERE / 'probe.cpp']


def sources():
    need(sha(PORT) == PORT_SHA and sha(INPUTS) == INPUTS_SHA, 'explicit port pins')
    paths = [p for p in SOURCE.rglob('*') if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / n for n in ('probe.cpp', 'CMakeLists.txt', 'run.py', 'selftest.py')]
    paths += [MANIFEST / n for n in ('capture.hpp', 'manifest.hpp', 'native_a.hpp', 'instrument.py', 'instrument.diff', 'instrument.json')]
    paths += [EVENT / 'events.hpp', MINIMUM / 'min_label.hpp', V9 / 'CMakeLists.txt',
              V9 / 'tests/gen/front_fixtures.hpp', HELPER, INPUTS, PORT,
              HERE.parent / 'b_full_real_drafts_20260927/probe.cpp']
    units()
    return {str(p.resolve()): sha(p) for p in sorted(set(paths))}


def dependencies(text, directory=ROOT):
    logical = text.replace('\\\n', ' ')
    need(': ' in logical, 'dependency target')
    names = shlex.split(logical.split(': ', 1)[1])
    need(names and len(names) == len(set(names)), 'dependency inventory')
    out = set()
    for name in names:
        path = Path(name)
        path = path.resolve() if path.is_absolute() else (directory / path).resolve()
        need(path.is_file(), 'dependency exists')
        out.add(str(path))
    return out


def case_data(name):
    if name == 'ng00':
        source = module(INPUTS, INPUTS_SHA, 'a_real_input_reader')
        cases, pins, partitions = source.grounded('00')
        case = cases['full']
        need(case['n'] == 39885 and int(case['point_hash'], 16) == 9245360528374966039, 'whole ng00 identity')
        return dict(name=name, mode='frame', family='none', n=case['n'], input_hash=int(case['point_hash'], 16),
                    input=case, partitions=partitions), pins
    need(name in ('uniform_64', 'uniform_8000', 'uniform_16000', 'uniform_32000'), 'planned real/synthetic case')
    n = int(name.rsplit('_', 1)[1])
    if n == 64:
        return dict(name=name, mode='synthetic', family='uniform', n=n, input_hash=14940886961791931765), {}
    prior_path = V9 / f'receipts/q3_payload_local_20260926/{name}_export.stdout'
    prior = read(prior_path)
    need(prior['family'] == 'uniform' and prior['sites'] == n and prior['seed'] == 3, 'synthetic reference identity')
    return dict(name=name, mode='synthetic', family='uniform', n=n, input_hash=int(prior['fixture_hash'], 16)), {str(prior_path): sha(prior_path)}


def argv_case(binary, case):
    return [binary, '--frame', case['input']['path']] if case['mode'] == 'frame' else [binary, '--synthetic', 'uniform', str(case['n'])]


def compiled_plan(build):
    build = Path(build)
    path = build / 'compile_commands.json'
    pins, rows = {str(path): sha(path)}, {}

    def expand(argv, active=()):
        result = []
        for argument in argv:
            if not argument.startswith('@'):
                result.append(argument)
                continue
            response = (build / argument[1:]).resolve()
            need(str(response) not in active, 'recursive compiler response file')
            pins[str(response)] = sha(response)
            result += expand(shlex.split(response.read_text()), (*active, str(response)))
        return result

    for entry in read(path):
        need(Path(entry['directory']).resolve() == build, 'compiler working directory')
        unit = str(Path(entry['file']).resolve())
        need(unit not in rows and unit in set(map(str, units())), 'compiled unit unique/native')
        argv = expand(entry['arguments'] if 'arguments' in entry else shlex.split(entry['command']))
        transformed, outputs, compile_flags = [], 0, 0
        j = 0
        while j < len(argv):
            if argv[j] == '-o':
                need(j+1 < len(argv), 'compiler output argument')
                outputs += 1;j += 2;continue
            if argv[j] == '-c':
                compile_flags += 1;j += 1;continue
            transformed.append(argv[j]);j += 1
        need(outputs == compile_flags == 1 and transformed.count(unit) == 1, 'exact compilation-to-dependency substitution')
        rows[unit] = ['cmake', '-E', 'chdir', str(build), *transformed, '-M']
    need(set(rows) == set(map(str, units())), 'complete native compile plan')
    return [rows[str(unit)] for unit in units()], pins


def recipe(state):
    if state['case']['name'] != 'qualification':
        yield ('measure', argv_case(state['binaries']['release'], state['case']), 0)
        return
    yield from [('system', ['uname', '-a'], 0),
            ('instrument', ['python3', '-B', str(MANIFEST / 'instrument.py'), '--check'], 0),
            ('instrument_optimized', ['python3', '-B', '-O', str(MANIFEST / 'instrument.py'), '--check'], 0)]
    for kind in KINDS:
        compiler = 'c++' if kind == 'release' else 'clang++'
        yield ('compiler_' + kind, [compiler, '--version'], 0)
        yield ('configure_' + kind, ['cmake', '-S', str(HERE), '-B', state['builds'][kind], '-DCMAKE_BUILD_TYPE=Release',
                   '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_CXX_COMPILER=' + compiler,
                   '-DMHGP9_A_REAL_SANITIZE=' + ('ON' if kind == 'sanitize' else 'OFF')], 0)
        # The generator resumes only AFTER configure has completed. Thus -M
        # inherits the actual compiler, ordered flags/definitions/includes,
        # directory and expanded response files, not a manually guessed recipe.
        commands, _ = compiled_plan(state['builds'][kind])
        for number, argv in enumerate(commands):
            yield (f'dependencies_{kind}_{number:02d}', argv, 0)
    for kind in KINDS:
        build, binary = state['builds'][kind], state['binaries'][kind]
        yield from [('build_' + kind, ['cmake', '--build', build, '--target', TARGET, '-j', '2'], 0),
                 ('gate_' + kind, [binary, '--gate'], 0),
                 ('smoke_' + kind, [binary, '--synthetic', 'uniform', '64'], 0),
                 ('cli_no_arg_' + kind, [binary], 2),
                 ('cli_unknown_' + kind, [binary, '--unknown'], 2)]


def build_pins(state):
    out = {}
    for kind in KINDS:
        build = Path(state['builds'][kind])
        paths = [build / TARGET, build / ('lib' + LIBRARY + '.a'), build / 'CMakeCache.txt', build / 'compile_commands.json']
        for target in (TARGET, LIBRARY):
            folder = build / 'CMakeFiles' / (target + '.dir')
            paths += [folder / 'flags.make', folder / 'link.txt']
            objects, dep = sorted(folder.rglob('*.o')), sorted(folder.rglob('*.o.d'))
            count = 1 if target == TARGET else 25
            need(len(objects) == len(dep) == count and set(str(p) + '.d' for p in objects) == set(map(str, dep)), 'complete fresh objects/dependencies')
            paths += objects + dep
        out.update({str(p): sha(p) for p in paths})
    return out


def u64(x):
    return type(x) is int and 0 <= x <= (1 << 64)-1


def finite(x):
    return type(x) in (int, float) and math.isfinite(x) and x >= 0


def validate_gate(value):
    need(value == GATE and set(value) == set(GATE), 'exact small gate')
    for key in ('cases', 'orders', 'candidate_comparisons', 'nonidentity_index_positions', 'continuations', 'extra_blocks'):
        need(u64(value[key]) and value[key] > 0, 'strict gate counts')
    need(value['GCP_used'] is False and value['FULL_executed_by_candidate'] is False, 'gate scope')


def validate_result(value, case):
    need(value.get('schema') == 'mhgp9_full_a_real_v1' and value.get('status') == 'passed' and
         value.get('scope') == 'real_catalogue_serial_A_comparison', 'real result scope')
    for key in ('GCP_used', 'candidate_parallel', 'FULL_executed_by_candidate', 'sealed_catalogue'):
        need(value.get(key) is False, 'false scope: ' + key)
    for key, expected in (('mode', case['mode']), ('family', case['family']), ('n', case['n']),
                          ('input_hash_u64', case['input_hash']), ('k', 5), ('workers', 4), ('static_threads', 4), ('seed', 3), ('s', 8)):
        need(value.get(key) == expected and (type(expected) is not int or u64(value[key])), 'case identity: ' + key)
    need(value['hash_grouping'] is True, 'native grouping option')
    scalar = ('catalogue_balls', 'catalogue_digest', 'presentation_digest', 'observed_native_tower_digest',
              'index_capacity_bytes', 'catalogue_capacity_bytes', 'capture_capacity_bytes_initial', 'peak_rss_kib')
    header = ('schema status scope mode family seed s n input_hash_u64 k workers static_threads hash_grouping '
              'sealed_catalogue GCP_used candidate_parallel FULL_executed_by_candidate times_ms rows').split()
    need(set(value) == set(header) | set(scalar), 'exact result fields')
    need(all(u64(value[k]) and value[k] > 0 for k in scalar), 'real counts and memory')
    t = value['times_ms']
    time_keys = ('input chain_wall chain_reported chain_catalogue_digest chain_q2 chain_q34 chain_census reindex identity '
                 'capture_full_wall admission tower_digest capture_copy_sum upstream_cleanup capture_tail_cleanup external '
                 'points_cleanup process native_validate native_static native_lots native_populations native_images native_bank native_encode').split()
    need(set(t) == set(time_keys) and all(finite(x) for x in t.values()), 'finite measured scope times')
    need(t['chain_reported'] <= t['chain_wall'] + 1e-6 and
         t['input'] + t['external'] + t['points_cleanup'] <= t['process'] + 1e-6, 'outer timing inclusion')
    need([row['k'] for row in value['rows']] == list(range(1, 6)), 'all K exactly once')
    capacities, copy_ms = 0, 0
    for row in value['rows']:
        count_fields = ('k balls blocks regular_blocks extra_blocks targets nodes actions contributions output_digest '
                        'capture_input_capacity_bytes capture_output_capacity_bytes manifest_capacity_bytes').split()
        time_fields = ('native_a_instrumented_ms capture_copy_ms manifest_ms binding_ms checks_ms_sum manifest_cleanup_ms slot_cleanup_ms').split()
        extras = {prefix + '_' + suffix for prefix in ('minimum', 'event') for suffix in ('work', 'wall_ms', 'cleanup_ms', 'times')}
        need(set(row) == set(count_fields) | set(time_fields) | extras, 'exact row fields')
        need(all(u64(row[k]) for k in ('k', 'balls', 'blocks', 'regular_blocks', 'extra_blocks', 'targets', 'nodes', 'actions', 'contributions', 'output_digest',
             'capture_input_capacity_bytes', 'capture_output_capacity_bytes', 'manifest_capacity_bytes')), 'strict row counts')
        need(row['balls'] == value['catalogue_balls'] and row['nodes'] > 0 and row['actions'] >= row['nodes'] and
             row['regular_blocks'] + row['extra_blocks'] == row['blocks'], 'real row identity')
        capacities += row['capture_input_capacity_bytes'] + row['capture_output_capacity_bytes']
        copy_ms += row['capture_copy_ms']
        for field in ('native_a_instrumented_ms', 'capture_copy_ms', 'manifest_ms', 'binding_ms', 'checks_ms_sum', 'manifest_cleanup_ms', 'slot_cleanup_ms'):
            need(finite(row[field]), 'row phase time')
        a, b = row['minimum_work'], row['event_work']
        common_work = ('V E G C forest_edges components P_event P_draft P_forest dsu_steps ancestor_entries '
                       'ancestor_queries ancestor_steps predecessor_steps silent_groups temporary_capacity_observed_max '
                       'combined_capacity_observed_max output_capacity').split()
        need(set(a) == set(common_work) | {'loss_entries', 'history_entries', 'omitted_history_groups', 'predecessor_queries'} and
             set(b) == set(common_work) | {'continuation_rounds', 'continuation_tests'}, 'exact Work fields')
        for w in (a, b):
            need(all(u64(x) for x in w.values()), 'u64 Work fields')
            need(w['V'] == row['blocks'] + (value['n'] if row['k'] == 1 else 0) and w['E'] == row['targets'] and
                 w['C'] == row['contributions'] and w['forest_edges'] + w['components'] == w['V'] and
                 w['P_forest'] <= w['P_draft'] <= w['P_event'] <= w['E'] and
                 w['P_forest'] + 1 == row['nodes'] and w['G'] <= w['V'] and
                 w['ancestor_queries'] == w['V'] + w['E'] and
                 w['output_capacity'] <= w['combined_capacity_observed_max'] and
                 w['temporary_capacity_observed_max'] <= w['combined_capacity_observed_max'], 'Work domains')
        need(all(a[f] == b[f] for f in ('V', 'E', 'G', 'C', 'P_event', 'P_draft', 'P_forest', 'silent_groups')), 'candidate work equality')
        need(a['history_entries'] == row['nodes'] and a['omitted_history_groups'] + a['history_entries'] == a['G'] and
             a['loss_entries'] == a['V'] and a['ancestor_entries'] == a['V'] * a['V'].bit_length() and
             b['ancestor_entries'] == 2 * a['ancestor_entries'] and
             a['ancestor_steps'] == a['ancestor_queries'] * (a['V'].bit_length() + 1), 'minimum data structure identities')
        for prefix in ('minimum', 'event'):
            for suffix in ('wall_ms', 'cleanup_ms'):
                xs = row[prefix + '_' + suffix]
                need(type(xs) is list and len(xs) == 3 and all(finite(x) for x in xs), 'three finite paired calls')
            phases = row[prefix + '_times']
            expected = {'validate_ms', 'forest_ms', 'ancestors_ms', 'groups_ms', 'parents_ms', 'output_ms', 'total_ms',
                        'histories_ms' if prefix == 'minimum' else 'redirects_ms'}
            need(type(phases) is list and len(phases) == 3, 'three phase ledgers')
            for j, p in enumerate(phases):
                need(set(p) == expected and all(finite(x) for x in p.values()) and
                     p['total_ms'] <= row[prefix + '_wall_ms'][j] + 1e-6 and
                     sum(x for key, x in p.items() if key != 'total_ms') <= p['total_ms'] + 1e-6, 'candidate timing inclusion')
    need(capacities == value['capture_capacity_bytes_initial'] and abs(copy_ms-t['capture_copy_sum']) <= 1e-6, 'capture retained/copy sums')


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed capture')
    need(set(state['builds']) == set(state['binaries']) == set(KINDS) and
         all(state['binaries'][k] == str(Path(state['builds'][k]) / TARGET) for k in KINDS), 'binary/build binding')
    name = state['case']['name']
    case, extra = (dict(name='qualification'), {}) if name == 'qualification' else case_data(name)
    need(state['case'] == case and state['source_pins_before'] == state['source_pins_after'] == sources() | extra, 'LIVE source/input closure')
    need(state['build_pins'] == build_pins(state), 'LIVE archive/object/binary closure')
    need(state['dependency_pins_before'] == state['dependency_pins_after'] and
         all(sha(p) == digest for p, digest in state['dependency_pins_before'].items()), 'LIVE compiled dependencies')
    actual_plan_pins = {}
    for kind in KINDS:
        _, pins = compiled_plan(state['builds'][kind]);actual_plan_pins.update(pins)
    need(state['compile_plan_pins_before'] == state['compile_plan_pins_after'] == actual_plan_pins, 'LIVE actual compiler plans/response files')
    if name != 'qualification':
        qualification = Path(state['qualification'])
        q = readback(qualification)
        qs = read(qualification / 'capture.json')
        need(q['scope'] == 'qualification' and qs['build_pins'] == state['build_pins'] and
             qs['dependency_pins_before'] == state['dependency_pins_before'], 'qualification linkage')
    planned, inventory, gates = list(recipe(state)), set(), []
    need(len(state['commands']) == len(planned), 'complete recipe')
    last_end = 0
    for command, (label, argv, code) in zip(state['commands'], planned):
        need(command['name'] == label and command['argv'] == argv and command['exit_code'] == code and command['group_closed'] and
             command['started_epoch'] >= last_end and command['ended_epoch'] >= command['started_epoch'], 'command recipe/join/chronology')
        last_end = command['ended_epoch']
        need(command == read(directory / (label + '.command.json')) and
             all(command.get(k) == v for k, v in read(directory / (label + '.intent.json')).items()), 'command intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (label + '.' + stream)) == command[stream + '_sha256'], 'stream binding')
        if label.startswith('dependencies_'):
            kind = label.split('_')[1]
            inventory |= dependencies((directory / (label + '.stdout')).read_text(), Path(state['builds'][kind]))
        if label.startswith('gate_'):
            gate = read(directory / (label + '.stdout'));validate_gate(gate);gates.append(gate)
        if label.startswith('smoke_') or label == 'measure':
            output = read(directory / (label + '.stdout'))
            validate_result(output, case_data('uniform_64')[0] if label.startswith('smoke_') else case)
        if label.startswith(('gate_', 'smoke_')) or label == 'measure':
            need(not (directory / (label + '.stderr')).read_text(), 'clean execution stderr')
        if label.startswith('cli_'):
            need(not (directory / (label + '.stdout')).read_text() and
                 (directory / (label + '.stderr')).read_text() == 'real.usage\n', 'exact CLI refusal')
    if name == 'qualification':
        need(set(state['dependency_pins_before']) == inventory and len(gates) == 2 and gates[0] == gates[1], 'dependency prebuild inventory and profile equality')
        for kind in KINDS:
            built_units = set()
            for target in (TARGET, LIBRARY):
                folder = Path(state['builds'][kind]) / 'CMakeFiles' / (target + '.dir')
                for path in folder.rglob('*.o.d'):
                    compiled = dependencies(path.read_text(), Path(state['builds'][kind]))
                    need(compiled <= inventory, 'actual dependencies discovered before compilation')
                    built_units |= compiled & set(map(str, units()))
            need(built_units == set(map(str, units())), 'all compiled translation units linked')
    return dict(status='PASS', scope=name, commands=len(planned), dependencies=len(state['dependency_pins_before']),
                candidate_parallel=False, FULL_executed_by_candidate=False, GCP_used=False)


def capture(args):
    dest = args.capture.resolve()
    need(not dest.exists(), 'fresh capture directory')
    if args.case == 'qualification':
        need(args.build_prefix is not None, 'build prefix required')
        builds = {k: str(args.build_prefix.resolve()) + '_' + k for k in KINDS}
        need(all(not Path(p).exists() for p in builds.values()), 'fresh GEN/probe builds')
        case, extra, deps, plan_pins, qualification = dict(name='qualification'), {}, {}, {}, None
    else:
        need(args.qualification is not None, 'qualification required')
        qualification = str(args.qualification.resolve());readback(Path(qualification))
        old = read(Path(qualification) / 'capture.json');builds = old['builds'];deps = old['dependency_pins_before'];plan_pins = old['compile_plan_pins_before']
        case, extra = case_data(args.case)
    helper = module(HELPER, HELPER_SHA, 'a_real_process_collector')
    dest.mkdir(parents=True)
    state = dict(schema=SCHEMA, status='failed', GCP_used=False, case=case, qualification=qualification, builds=builds,
                 binaries={k: str(Path(p) / TARGET) for k, p in builds.items()}, source_pins_before=sources() | extra,
                 dependency_pins_before=dict(deps), compile_plan_pins_before=dict(plan_pins), build_pins={})
    commands = helper.Commands(dest, dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))
    handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        for label, argv, expected in recipe(state):
            code, _, _ = commands.run(label, argv, timeout=None)
            need(code == expected, label + ': unexpected exit ' + str(code))
            if label.startswith('configure_'):
                _, plan = compiled_plan(state['builds'][label.split('_')[1]])
                state['compile_plan_pins_before'].update(plan)
            if label.startswith('dependencies_'):
                kind = label.split('_')[1]
                for path in dependencies((dest / (label + '.stdout')).read_text(), Path(state['builds'][kind])):
                    digest = sha(path)
                    need(path not in state['dependency_pins_before'] or state['dependency_pins_before'][path] == digest, 'dependency drift during discovery')
                    state['dependency_pins_before'][path] = digest
            if label.startswith('gate_'):
                validate_gate(read(dest / (label + '.stdout')))
            if label.startswith('smoke_') or label == 'measure':
                validate_result(read(dest / (label + '.stdout')), case_data('uniform_64')[0] if label.startswith('smoke_') else case)
            print(label + ': closed ' + str(code), flush=True)
        state['build_pins'] = build_pins(state);state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        state['commands'] = commands.rows
        state['source_pins_after'] = {p: sha(p) for p in state['source_pins_before']}
        state['dependency_pins_after'] = {p: sha(p) for p in state['dependency_pins_before']}
        state['compile_plan_pins_after'] = {p: sha(p) for p in state['compile_plan_pins_before']}
        if (state['source_pins_before'] != state['source_pins_after'] or state['dependency_pins_before'] != state['dependency_pins_after'] or
                state['compile_plan_pins_before'] != state['compile_plan_pins_after']):
            state['status'] = 'failed'
        save(dest / 'capture.json', state)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    try:
        summary = readback(dest)
    except BaseException as error:
        save(dest / 'readback_failure.json', dict(status='failed', error=type(error).__name__ + ': ' + str(error),
                                                commands_completed=state['status'] == 'completed'))
        raise
    save(dest / 'summary.json', summary);print(json.dumps(summary, sort_keys=True), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--capture', type=Path);mode.add_argument('--readback', type=Path)
    parser.add_argument('--case', default='qualification');parser.add_argument('--build-prefix', type=Path)
    parser.add_argument('--qualification', type=Path)
    args = parser.parse_args()
    if args.readback:
        print(json.dumps(readback(args.readback.resolve()), sort_keys=True))
    else:
        capture(args)


if __name__ == '__main__':
    main()
