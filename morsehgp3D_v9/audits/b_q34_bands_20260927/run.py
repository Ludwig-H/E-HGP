#!/usr/bin/env python3
"""Fresh local representation capture; no GCP call or engine modification.

The previous Pool driver/recipes are explicitly reused and pinned. Its old
cells are still constructed and paid; these are NOT replacement-S2 timings.
"""
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
spec = importlib.util.spec_from_file_location('band_input_maps', OLD / 'inputs.py')
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
TARGET = 'mhgp9_audit_q34_bands'
SCHEMA = 'mhgp9_q34_bands_capture_v2'
PROBE = 'mhgp9_q34_bands_v2'


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def sources():
    paths = [p for p in (ROOT / 'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / name for name in ('bands.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py')]
    paths += [OLD / name for name in ('plan.hpp', 'probe.cpp', 'inputs.py')]
    paths += [ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp', HELPER, *LIBS.values()]
    return {str(p): sha(p) for p in sorted(set(paths))}


def cases():
    result, pins = [], {}
    for family in ('uniform', 'terrain', 'clusters'):
        for n in (8000, 16000, 32000):
            name = f'{family}_{n}'
            prior_path = ROOT / f'morsehgp3D_v9/receipts/q34_factor_plan_20260926/measure_{name}.stdout'
            prior = read(prior_path)
            pins[str(prior_path)] = sha(prior_path)
            result.append(dict(name=name, args=['--synthetic', family, str(n), '5', '8'],
                               n=n, k=5, s=8, family=family, mode='synthetic',
                               input_hash=prior['input_hash'], prior=str(prior_path)))
    data, frame_pins, partition = inputs.grounded('00')
    pins.update(frame_pins)
    entry = data['full']
    prior_path = ROOT / 'morsehgp3D_v9/receipts/q34_factor_plan_20260926/measure_ng_00_full.stdout'
    pins[str(prior_path)] = sha(prior_path)
    result.append(dict(name='ng_00_full', args=['--frame', entry['path'], '5', '8'],
                       n=entry['n'], k=5, s=8, family='none', mode='frame',
                       input_hash=entry['point_hash'], prior=str(prior_path)))
    return result, pins, partition


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
                 for name in ('overlap', 'mask6')]
    rows += [('measure_' + case['name'], [state['binaries']['release'], *case['args']], 0)
             for case in state['cases']]
    return rows


def validate(value, case):
    need(value.get('schema') == PROBE and value.get('status') == 'pass', 'successful probe')
    for name in ('n', 'k', 's', 'family', 'mode', 'input_hash'):
        need(value.get(name) == case[name], 'case identity: ' + name)
    need(value['seed'] == 3 and value['min_factor'] == 2 and
         value['scope'] == 'representation_only_old_cells_still_paid', 'measurement scope')
    m = value['metrics']
    need(all(type(x) is int and x >= 0 for x in m.values()), 'nonnegative integer counters')
    need(m['rank_entries_added'] == m['credit_entries_added'] == 0, 'borrowed factors')
    need(m['validation_ranks'] == m['F'] and m['histogram_slots'] == 341*m['planned'] and
         m['histogram_prefix_steps'] == 100*m['planned'], 'preparation work paid')
    need(max(m['E3'], m['E4']) <= m['E'] <= min(m['P'], m['E3']+m['E4']), 'union mass')
    need(m['band_descriptor_bytes'] >= 12*m['bands'] and m['cell_descriptor_bytes'] >= 40*m['cells'],
         'allocated descriptor capacity')
    prior = read(case['prior'])
    for key, prior_value in dict(P=prior['rectangle_filter']['P'], E=prior['plans']['E_union'],
                                E3=prior['plans']['E3'], E4=prior['plans']['E4'],
                                F=prior['plans']['F'], cells=prior['plans']['descriptors']).items():
        need(m[key] == prior_value, 'same Pool residue: ' + key)
    need(prior['input_hash'] == value['input_hash'], 'prior input binding')
    need(all(type(x) in (int, float) and math.isfinite(x) and x >= 0 for x in value['times_ms'].values()),
         'finite nonnegative timing')


def build_pins(state):
    out = {}
    for binary in state['binaries'].values():
        path = Path(binary)
        for p in (path, path.parent / 'CMakeCache.txt',
                  path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'flags.make',
                  path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'link.txt'):
            out[str(p)] = sha(p)
    return out


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False,
         'closed local capture')
    current_cases, input_pins, partition = cases()
    need(state['cases'] == current_cases and state['partition'] == partition, 'recipe and input mapping')
    need(state['pins_before'] == state['pins_after'] == sources() | input_pins, 'LIVE dependency closure')
    need(state['build_pins'] == build_pins(state), 'LIVE build identity')
    planned = recipe(state)
    need(len(state['commands']) == len(planned), 'full command inventory')
    gates, measured = [], {}
    for command, (name, argv, code) in zip(state['commands'], planned):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command recipe/closure')
        need(read(directory / (name + '.command.json')) == command, 'stored command binding')
        intent = read(directory / (name + '.intent.json'))
        need(all(command.get(k) == v for k, v in intent.items()), 'original command intent')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == PROBE and value['status'] == 'pass' and value['mode'] == 'gate', 'gate verdict')
            need(value['native_fronts'] == 96 and value['native_rectangles'] > 0 and value['refusals'] == 8 and
                 value['cases'] >= 1440 and value['pairs'] > 100000, 'non-vacuous coverage')
            gates.append(value)
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            expected = 'factor_plan.bands_' + ('overlap_duplicate' if name.endswith('overlap') else 'mask_differs')
            need(value['schema'] == PROBE and value['status'] == 'failed' and value['cause'] == expected,
                 'causal mutant rejection')
        if name.startswith('measure_'):
            case = next(c for c in current_cases if name == 'measure_' + c['name'])
            value = read(directory / (name + '.stdout'))
            validate(value, case)
            measured[case['name']] = dict(metrics=value['metrics'], times_ms=value['times_ms'])
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer differential gate')
    return dict(status='PASS', commands=len(planned), gate=gates[0], measurements=measured,
                scope='representation_only_old_cells_still_paid', GCP_used=False)


def capture(args):
    directory = args.capture.resolve()
    need(not directory.exists(), 'fresh capture directory required')
    current_cases, input_pins, partition = cases()
    builds = {kind: str(args.build_prefix.resolve()) + '_' + kind for kind in LIBS}
    need(all(not Path(p).exists() for p in builds.values()), 'fresh build directories required')
    directory.mkdir(parents=True)
    state = dict(schema=SCHEMA, status='failed', GCP_used=False, cases=current_cases, partition=partition,
                 pins_before=sources() | input_pins, builds=builds,
                 binaries={kind: str(Path(p) / TARGET) for kind, p in builds.items()}, build_pins={})
    need(sha(HELPER) == HELPER_SHA, 'local collector source pin')
    spec = importlib.util.spec_from_file_location('band_process_collector', HELPER)
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
                print(json.dumps(dict(case=case['name'], **value['metrics'])), flush=True)
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
