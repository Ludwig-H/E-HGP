#!/usr/bin/env python3
"""Local paired ABBA preparation benchmark, geometry unchanged, no GPU/FULL."""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import signal
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OLD = HERE.parent / 'b_q34_factor_plan_20260926'
spec = importlib.util.spec_from_file_location('direct_input_maps', OLD / 'inputs.py')
inputs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inputs)
inputs.GROUND = Path('/workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6')
need, sha, read = inputs.need, inputs.sha, inputs.read
HELPER = ROOT / 'gcp-migration/full_probe_session_v7.py'
HELPER_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
LIBS = {
    'release': Path('/workspaces/E-HGP/build/v9-q3-payload-integration-20260926/libmhgp9_gen.a'),
    'sanitize': Path('/workspaces/E-HGP/build/v9-q3-payload-sanitize-20260926/libmhgp9_gen.a'),
}
TARGET = 'mhgp9_audit_q34_direct_bands'
SCHEMA = 'mhgp9_q34_direct_bands_capture_v1'
PROBE = 'mhgp9_q34_direct_bands_v1'


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def sources():
    paths = [p for p in (ROOT / 'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / name for name in ('direct.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py')]
    paths += [OLD / name for name in ('plan.hpp', 'probe.cpp', 'inputs.py')]
    paths += [ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp', HELPER, *LIBS.values()]
    return {str(p): sha(p) for p in sorted(set(paths))}


def cases():
    rows, pins, partitions = [], {}, []

    def add(name, argv, n, k, s, family, mode, point_hash):
        prior = ROOT / f'morsehgp3D_v9/receipts/q34_factor_plan_20260926/measure_{name}.stdout'
        pins[str(prior)] = sha(prior)
        rows.append(dict(name=name, args=argv, n=n, k=k, s=s, family=family, mode=mode,
                         input_hash=point_hash, prior=str(prior)))

    frames = {}
    for scene in ('00', '01', '02'):
        data, frame_pins, partition = inputs.grounded(scene)
        pins.update(frame_pins)
        partitions.append(partition)
        frames[scene] = data['full']
    entry = frames['00']
    add('ng_00_full', ['--frame', entry['path'], '5', '8'], entry['n'], 5, 8, 'none', 'frame', entry['point_hash'])
    for family in ('uniform', 'terrain', 'clusters'):
        for n in (8000, 16000, 32000):
            name = f'{family}_{n}'
            prior = read(ROOT / f'morsehgp3D_v9/receipts/q34_factor_plan_20260926/measure_{name}.stdout')
            add(name, ['--synthetic', family, str(n), '5', '8'], n, 5, 8, family, 'synthetic', prior['input_hash'])
    for scene in ('01', '02'):
        entry = frames[scene]
        add(f'ng_{scene}_full', ['--frame', entry['path'], '5', '8'], entry['n'], 5, 8, 'none', 'frame', entry['point_hash'])
    entry = frames['00']
    for suffix, k, s in (('k10', 10, 8), ('s10', 5, 10), ('s12', 5, 12)):
        add('ng_00_' + suffix, ['--frame', entry['path'], str(k), str(s)], entry['n'], k, s, 'none', 'frame', entry['point_hash'])
    return rows, pins, partitions


def recipe(state):
    rows = [('system', ['uname', '-a'], 0)]
    for kind in ('release', 'sanitize'):
        compiler = 'c++' if kind == 'release' else 'clang++'
        flags = '-Wall -Wextra -Wpedantic -Werror'
        if kind == 'sanitize':
            flags += ' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
        build = Path(state['builds'][kind])
        binary = state['binaries'][kind]
        rows += [('compiler_' + kind, [compiler, '--version'], 0),
                 ('configure_' + kind, ['cmake', '-S', str(HERE), '-B', str(build),
                    '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=' + compiler,
                    '-DCMAKE_CXX_FLAGS=' + flags, '-DMHGP9_SOURCE_ROOT=' + str(ROOT / 'morsehgp3D_v9'),
                    '-DMHGP9_GEN_LIBRARY=' + str(LIBS[kind])], 0),
                 ('build_' + kind, ['cmake', '--build', str(build), '--target', TARGET, '-j', '2'], 0),
                 ('gate_' + kind, [binary, '--gate'], 0)]
        rows += [('mutant_' + kind + '_' + name, [binary, '--mutant', name], 1)
                 for name in ('overlap', 'mask6', 'ids')]
    rows += [('measure_' + case['name'], [state['binaries']['release'], *case['args']], 0)
             for case in state['cases']]
    return rows


def validate(value, case):
    need(value.get('schema') == PROBE and value.get('status') == 'pass', 'successful probe')
    for name in ('n', 'k', 's', 'family', 'mode', 'input_hash'):
        need(value.get(name) == case[name], 'case identity: ' + name)
    need(value['seed'] == 3 and value['min_factor'] == 2 and value['paired_order'] == 'ABBA_per_rectangle' and
         value['scope'] == 'direct_preparation_paired_no_S2_no_FULL_no_GPU', 'measurement scope')
    m = value['metrics']
    need(all(type(x) is int and x >= 0 for x in m.values()), 'nonnegative integer counters')
    need(m['new_cells_tested'] == 0 and m['geometry_fields_verified'] == 40*m['planned'], 'no old cells, both geometry checks')
    need(max(m['E3'], m['E4']) <= m['E'] <= min(m['P'], m['E3']+m['E4']), 'union mass')
    need(m['band_bytes'] >= 12*m['bands'] and m['cell_bytes'] >= 40*m['cells'], 'descriptor capacities')
    need(m['old_peak'] <= m['old_bytes'] and m['new_peak'] <= m['new_bytes'], 'sequential peak versus sum')
    need(m['old_retained_buffers'] >= 8*m['planned'] and m['new_retained_buffers'] >= 8*m['planned'], 'factors retained')
    prior = read(case['prior'])
    for key, expected in dict(P=prior['rectangle_filter']['P'], E=prior['plans']['E_union'],
                             E3=prior['plans']['E3'], E4=prior['plans']['E4'], F=prior['plans']['F'],
                             cells=prior['plans']['descriptors'], old_cells_tested=prior['plans']['cells_tested'],
                             corner_tests=prior['plans']['corner_tests'], selected_sites=prior['plans']['selected_sites']).items():
        need(m[key] == expected, 'same geometry and residual: ' + key)
    need(prior['input_hash'] == value['input_hash'], 'historical input binding')
    times = value['times_ms']
    need(all(type(x) in (int, float) and math.isfinite(x) and x >= 0 for x in times.values()), 'finite times')
    need(times['total'] >= times['front_all_pairs'] and
         times['front_all_pairs'] + .1 >= sum(times[x] for x in ('old_ab', 'new_ab', 'new_ba', 'old_ba', 'filter', 'verification')),
         'paired stages paid separately')


def build_pins(state):
    out = {}
    for binary in state['binaries'].values():
        path = Path(binary)
        for p in (path, path.parent / 'CMakeCache.txt', path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'flags.make',
                  path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'link.txt'):
            out[str(p)] = sha(p)
    return out


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    current_cases, input_pins, partitions = cases()
    need(state['cases'] == current_cases and state['partitions'] == partitions, 'recipe/input mapping')
    need(state['pins_before'] == state['pins_after'] == sources() | input_pins, 'LIVE dependency closure')
    need(state['build_pins'] == build_pins(state), 'LIVE build identity')
    planned = recipe(state)
    need(len(state['commands']) == len(planned), 'complete command inventory')
    gates, measured = [], {}
    for command, (name, argv, code) in zip(state['commands'], planned):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command recipe/closure')
        need(read(directory / (name + '.command.json')) == command, 'stored command binding')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'command intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == PROBE and value['status'] == 'pass' and value['mode'] == 'gate', 'gate verdict')
            need(value['fronts'] == 96 and value['rectangles'] > 0 and value['refusals'] == 56 and
                 value['cases'] >= 1440 and value['pairs'] > 100000 and value['geometry_fields'] == 20*value['rectangles'],
                 'non-vacuous gate')
            gates.append(value)
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            causes = dict(overlap='direct_overlap_duplicate', mask6='direct_pair_mask', ids='direct_factor_proposals')
            need(value['schema'] == PROBE and value['status'] == 'failed' and
                 value['cause'] == 'factor_plan.' + causes[name.rsplit('_', 1)[1]], 'causal native mutant')
        if name.startswith('measure_'):
            case = next(c for c in current_cases if name == 'measure_' + c['name'])
            value = read(directory / (name + '.stdout'))
            validate(value, case)
            t = value['times_ms']
            old_ms, new_ms = (t['old_ab']+t['old_ba'])/2, (t['new_ab']+t['new_ba'])/2
            measured[case['name']] = dict(metrics=value['metrics'], times_ms=t, old_mean_ms=old_ms,
                                         new_mean_ms=new_ms, speedup=old_ms/new_ms,
                                         ratios_by_order=[t['old_ab']/t['new_ab'], t['old_ba']/t['new_ba']])
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer gate equality')
    return dict(status='PASS', commands=len(planned), gate=gates[0], measurements=measured,
                scope='direct_preparation_paired_no_S2_no_FULL_no_GPU', GCP_used=False)


def capture(args):
    directory = args.capture.resolve()
    need(not directory.exists(), 'fresh capture required')
    current_cases, input_pins, partitions = cases()
    builds = {kind: str(args.build_prefix.resolve()) + '_' + kind for kind in LIBS}
    need(all(not Path(p).exists() for p in builds.values()), 'fresh builds required')
    directory.mkdir(parents=True)
    state = dict(schema=SCHEMA, status='failed', GCP_used=False, cases=current_cases, partitions=partitions,
                 pins_before=sources() | input_pins, builds=builds,
                 binaries={kind: str(Path(p) / TARGET) for kind, p in builds.items()}, build_pins={})
    need(sha(HELPER) == HELPER_SHA, 'local collector pin')
    spec = importlib.util.spec_from_file_location('direct_process_collector', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    commands = module.Commands(directory, env)

    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    try:
        for name, argv, code in recipe(state):
            rc, _, _ = commands.run(name, argv, timeout=None)
            need(rc == code, name + ': unexpected exit ' + str(rc))
            if name.startswith('measure_'):
                case = next(c for c in current_cases if name == 'measure_' + c['name'])
                value = read(directory / (name + '.stdout'))
                validate(value, case)
                print(json.dumps(dict(case=case['name'], times=value['times_ms'], metrics=value['metrics'])), flush=True)
        state['build_pins'] = build_pins(state)
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        state['commands'] = commands.rows
        state['pins_after'] = {path: sha(path) for path in state['pins_before']}
        if state['pins_before'] != state['pins_after']:
            state['status'] = 'failed'
        save(directory / 'capture.json', state)
    summary = readback(directory)
    save(directory / 'summary.json', summary)
    print(json.dumps(summary), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--capture', type=Path)
    action.add_argument('--readback', type=Path)
    parser.add_argument('--build-prefix', type=Path)
    args = parser.parse_args()
    if args.readback:
        print(json.dumps(readback(args.readback.resolve()), sort_keys=True))
    else:
        need(args.build_prefix is not None, 'build prefix required')
        capture(args)


if __name__ == '__main__':
    main()
