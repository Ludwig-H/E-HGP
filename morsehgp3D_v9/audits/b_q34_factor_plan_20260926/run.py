#!/usr/bin/env python3
"""Fresh LOCAL audit captures and LIVE readback; never opens a cloud session.

Counts descriptor mass, not FULL output. All commands, including failures,
are retained. The borrowed process collector is pinned and never invokes
its cloud workflow. No KITTI bytes are copied into this folder.
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
import inputs

HERE = Path(__file__).resolve().parent
ROOT = inputs.ROOT
SCHEMA = 'mhgp9_q34_factor_capture_v1'
PROBE = 'mhgp9_q34_factor_plan_v1'
HELPER = ROOT / 'gcp-migration/full_probe_session_v7.py'
HELPER_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
LIBS = {
    'release': Path('/workspaces/E-HGP/build/v9-q3-payload-integration-20260926/libmhgp9_gen.a'),
    'sanitize': Path('/workspaces/E-HGP/build/v9-q3-payload-sanitize-20260926/libmhgp9_gen.a'),
}
TARGET = 'mhgp9_audit_q34_factor_plan'
need, sha, read = inputs.need, inputs.sha, inputs.read


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def command_class():
    need(sha(HELPER) == HELPER_SHA, 'process collector pin')
    spec = importlib.util.spec_from_file_location('factor_plan_process_capture', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.Commands


def source_pins():
    paths = [p for p in (ROOT / 'morsehgp3D_v9/src').rglob('*')
             if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / name for name in ('plan.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py',
                                       'inputs.py', 'math_oracle.py')]
    paths += [ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp', HELPER, *LIBS.values()]
    return {str(p): sha(p) for p in sorted(set(paths))}


def cases():
    result, pins, partitions = [], {}, []
    for family in ('uniform', 'terrain', 'clusters'):
        for n in (8000, 16000, 32000):
            export = ROOT / f'morsehgp3D_v9/receipts/q3_payload_local_20260926/{family}_{n}_export.stdout'
            prior = read(export)
            need(prior['seed'] == 3 and prior['sites'] == n and prior['family'] == family, 'synthetic recipe identity')
            pins[str(export)] = sha(export)
            result.append(dict(name=f'{family}_{n}', args=['--synthetic', family, str(n), '5', '8'],
                               n=n, k=5, s=8, min_factor=2, family=family, point_hash=prior['fixture_hash']))
    for scene in ('00', '01', '02'):
        data, current_pins, partition = inputs.grounded(scene)
        pins.update(current_pins)
        partitions.append(partition)
        for part in (inputs.PARTS if scene == '00' else ('full',)):
            entry = data[part]
            result.append(dict(name=f'ng_{scene}_{part}', args=['--frame', entry['path'], '5', '8'],
                               n=entry['n'], k=5, s=8, min_factor=2, data=entry))
        if scene == '00':
            for name, k, s, minimum in (('k10', 10, 8, 2), ('s10', 5, 10, 2),
                                        ('s12', 5, 12, 2), ('min1', 5, 8, 1)):
                entry = data['full']
                result.append(dict(name=f'ng_00_{name}', args=['--frame', entry['path'], str(k), str(s)],
                                   n=entry['n'], k=k, s=s, min_factor=minimum, data=entry))
    entry, current_pins = inputs.raw_full()
    pins.update(current_pins)
    for k in (5, 10):
        result.append(dict(name=f'raw_00_k{k}', args=['--frame', entry['path'], str(k), '8'],
                           n=entry['n'], k=k, s=8, min_factor=2, data=entry))
    for row in result:
        row['args'].append('--min-factor=' + str(row['min_factor']))
    return result, pins, partitions


def counters(section):
    need(type(section) is dict and section and
         all(type(v) is int and v >= 0 for v in section.values()), 'nonnegative integer counters')


def validate(value, case):
    need(value.get('schema') == PROBE and value.get('status') == 'pass', 'successful probe schema')
    for name in ('n', 'k', 's', 'min_factor'):
        need(type(value.get(name)) is int and value[name] == case[name], 'case binding: ' + name)
    need(value['input_hash'] == (case['data']['point_hash'] if 'data' in case else case['point_hash']),
         'point identity differs from frame mapping or previous synthetic FULL test')
    need(value['mode'] == ('frame' if 'data' in case else 'synthetic') and value['seed'] == 3 and
         value['family'] == case.get('family', 'none'), 'input mode/recipe binding')
    for name in ('front', 'rectangle_filter', 'plans', 'memory'):
        counters(value[name])
    times = value['times_ms']
    need(type(times) is dict and all(type(x) in (float, int) and math.isfinite(x) and x >= 0
                                   for x in times.values()), 'finite stage timing')
    f, r, p = (value[x] for x in ('front', 'rectangle_filter', 'plans'))
    need(f['total_unordered_pairs'] == case['n'] * (case['n']-1)//2, 'full cloud pair domain')
    need(max(f['pair_mass_q3'], f['pair_mass_q4']) <= f['pair_mass_union'] <=
         min(f['pair_mass_q3'] + f['pair_mass_q4'], f['total_unordered_pairs']), 'front union mass')
    need(max(r['P3'], r['P4']) <= r['P'] <= min(r['P3'] + r['P4'], f['pair_mass_union']), 'rectangle mass')
    need(max(p['E3'], p['E4']) <= p['E_union'] <= min(p['E3'] + p['E4'], r['P']) and
         p['E3'] <= r['P3'] and p['E4'] <= r['P4'], 'residual lane mass')
    need(p['rectangles'] + p['skipped_min_factor_rectangles'] + p['skipped_capacity_rectangles'] ==
         r['rectangles_surviving'], 'all surviving rectangles planned or passed through')
    need(p['skipped_min_factor_mass'] + p['skipped_capacity_mass'] <= p['E_union'], 'fallback preserved')
    need(p['F'] <= f['F'] and p['selection_visits'] == p['F'] == p['anchor_visits'], 'factor scans paid')
    need(p['grouping_visits'] == 2 * p['F'], 'class counting and scatter paid')
    need(p['retained_bytes_peak'] <= p['retained_bytes_sum'], 'sequential peak versus retained sum')
    need(p['descriptors'] > 0 and p['cells_tested'] > 0 and r['P'] > 0, 'non-vacuous residual test')
    need(times['total'] >= times['front_filter_plan'] and
         times['front_filter_plan'] + .1 >= times['plans'] + times['rectangle_filter'], 'stage accounting')


def metric(value):
    f, r, p = (value[x] for x in ('front', 'rectangle_filter', 'plans'))
    return dict(n=value['n'], P=r['P'], E=p['E_union'], E3=p['E3'], E4=p['E4'],
                front_F=f['F'], planned_F=p['F'], corner_tests=p['corner_tests'],
                descriptors=p['descriptors'], cells=p['cells_tested'],
                fallback_descriptors=p['fallback_descriptors'],
                retained_bytes_sum=p['retained_bytes_sum'], retained_bytes_peak=p['retained_bytes_peak'],
                fraction_surviving=p['E_union']/r['P'], times_ms=value['times_ms'])


def growth(small, large):
    size_ratio = large['n'] / small['n']
    rows = {}
    for key in ('P', 'E', 'front_F', 'planned_F', 'corner_tests', 'descriptors', 'cells'):
        ratio = large[key]/small[key] if small[key] else None
        rows[key] = dict(ratio=ratio, log_slope=math.log(ratio)/math.log(size_ratio) if ratio else None)
    return dict(n_ratio=size_ratio, metrics=rows)


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False,
         'closed local capture')
    need(state['pins_before'] == state['pins_after'], 'source/input closure')
    for path, digest in (state['pins_after'] | state['build_pins']).items():
        need(sha(path) == digest, 'LIVE hash changed: ' + path)
    expected_cases, expected_pins, partitions = cases()
    need(state['cases'] == expected_cases and state['partitions'] == partitions and
         all(state['pins_after'].get(p) == h for p, h in expected_pins.items()), 'fresh input/plan mapping')
    need(state['pins_before'] == source_pins() | expected_pins, 'complete source/input pin inventory')
    build_pins = {}
    for binary in state['binaries'].values():
        path = Path(binary)
        for item in (path, path.parent / 'CMakeCache.txt', path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'flags.make',
                     path.parent / 'CMakeFiles' / (TARGET + '.dir') / 'link.txt'):
            build_pins[str(item)] = sha(item)
    need(build_pins == state['build_pins'] and set(state['binaries']) == {'release', 'sanitize'}, 'complete build pins')
    rows = {}
    for case in state['cases']:
        value = read(directory / ('measure_' + case['name'] + '.stdout'))
        validate(value, case)
        rows[case['name']] = metric(value)
    names = [command['name'] for command in state['commands']]
    need(len(names) == len(set(names)) and set(names) == set(state['expected_codes']), 'command inventory')
    required = {'system', 'cpu', 'math_normal', 'math_optimized'}
    required.update('measure_' + case['name'] for case in expected_cases)
    for kind in ('release', 'sanitize'):
        required.update(phase + '_' + kind for phase in ('compiler', 'configure', 'build', 'gate'))
        required.update('mutant_' + kind + '_' + name for name in ('equality', 'overlap', 'ids'))
    need(set(names) == required, 'complete fixed campaign command set')
    gates = []
    for command in state['commands']:
        name = command['name']
        need(read(directory / (name + '.command.json')) == command, 'command binding')
        intent = read(directory / (name + '.intent.json'))
        need(all(command.get(k) == v for k, v in intent.items()), 'command intent binding')
        need(command['exit_code'] == state['expected_codes'][name] and command['group_closed'] and
             command['ended_epoch'] >= command['started_epoch'], 'command closure/code')
        need(command['exit_code'] == (1 if name.startswith('mutant_') else 0), 'fixed expected code')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'captured stream pin')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == PROBE and value['mode'] == 'gate' and value['status'] == 'pass', 'native gate')
            need(all(type(x) is int and x > 0 for x in value['coverage'].values()), 'gate coverage non-vacuity')
            need(command['argv'] == [state['binaries'][name.split('_')[1]], '--gate'], 'gate argv')
            gates.append(value)
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            causes = dict(equality='strict_credit_differs', overlap='joint_mask_differs', ids='original_ids_differ')
            need(command['exit_code'] == 1 and value['status'] == 'failed' and
                 value['cause'] == 'factor_plan.' + causes[name.rsplit('_', 1)[1]], 'causal mutant')
            _, kind, mutant = name.split('_')
            need(command['argv'] == [state['binaries'][kind], '--mutant', mutant], 'mutant argv')
        if name.startswith('math_'):
            value = read(directory / (name + '.stdout'))
            need(value['status'] == 'PASS' and value['counts']['strict_witnesses'] > 0 and
                 value['product_imported'] is False, 'independent mathematical oracle')
            flags = ['-B', '-O'] if name.endswith('optimized') else ['-B']
            need(command['argv'] == [state['python'], *flags, str(HERE / 'math_oracle.py')], 'math argv')
        if name.startswith('measure_'):
            case = next(row for row in expected_cases if 'measure_' + row['name'] == name)
            need(command['argv'] == [state['binaries']['release'], *case['args']] and
                 command['exit_code'] == 0, 'measurement argv binding')
    need(gates[0] == gates[1], 'Release/sanitizer gates differ')
    series = {}
    for family in ('uniform', 'terrain', 'clusters'):
        series[family] = [growth(rows[f'{family}_{a}'], rows[f'{family}_{b}'])
                          for a, b in ((8000, 16000), (16000, 32000))]
    spatial = {parent + '/' + child: growth(rows['ng_00_' + child], rows['ng_00_' + parent])
               for parent, child in inputs.RELATIONS}
    return dict(status='PASS', scope='factor_plan_no_census_no_FULL_no_GPU',
                measurements=rows, synthetic_growth=series, spatial_growth=spatial,
                whole_frame_P_matches_prior=rows['ng_00_full']['P'] == 23686751,
                commands=len(state['commands']), GCP_used=False)


def capture(args):
    directory = args.capture.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    plan, input_pins, partitions = cases()
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    commands = command_class()(directory, env)
    state = dict(schema=SCHEMA, status='failed', GCP_used=False, cases=plan, partitions=partitions,
                 pins_before=source_pins() | input_pins, expected_codes={}, build_pins={}, python=sys.executable)

    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)

    def run(name, argv, code=0):
        state['expected_codes'][name] = code
        rc, _, _ = commands.run(name, list(map(str, argv)), timeout=None)
        need(rc == code, name + ': unexpected exit ' + str(rc))

    try:
        run('system', ['uname', '-a'])
        run('cpu', ['lscpu'])
        binaries = {}
        state['binaries'] = {}
        for kind in ('release', 'sanitize'):
            build = Path(str(args.build_prefix.resolve()) + '_' + kind)
            need(not build.exists(), 'fresh build required: ' + str(build))
            flags = '-Wall -Wextra -Wpedantic -Werror'
            compiler = 'c++' if kind == 'release' else 'clang++'
            if kind == 'sanitize':
                flags += ' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
            run('compiler_' + kind, [compiler, '--version'])
            run('configure_' + kind, ['cmake', '-S', HERE, '-B', build, '-DCMAKE_BUILD_TYPE=Release',
                                      '-DCMAKE_CXX_COMPILER=' + compiler, '-DCMAKE_CXX_FLAGS=' + flags,
                                      '-DMHGP9_SOURCE_ROOT=' + str(ROOT / 'morsehgp3D_v9'),
                                      '-DMHGP9_GEN_LIBRARY=' + str(LIBS[kind])])
            run('build_' + kind, ['cmake', '--build', build, '--target', TARGET, '-j', '2'])
            binary = build / TARGET
            binaries[kind] = binary
            state['binaries'][kind] = str(binary)
            for path in (binary, build / 'CMakeCache.txt', build / 'CMakeFiles' / (TARGET + '.dir') / 'flags.make',
                         build / 'CMakeFiles' / (TARGET + '.dir') / 'link.txt'):
                state['build_pins'][str(path)] = sha(path)
            run('gate_' + kind, [binary, '--gate'])
            for mutant in ('equality', 'overlap', 'ids'):
                run('mutant_' + kind + '_' + mutant, [binary, '--mutant', mutant], 1)
        run('math_normal', [sys.executable, '-B', HERE / 'math_oracle.py'])
        run('math_optimized', [sys.executable, '-B', '-O', HERE / 'math_oracle.py'])
        for case in plan:
            name = 'measure_' + case['name']
            run(name, [binaries['release'], *case['args']])
            validate(read(directory / (name + '.stdout')), case)
            print(json.dumps(dict(case=case['name'], metrics=metric(read(directory / (name + '.stdout'))))), flush=True)
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
    print(json.dumps(summary, sort_keys=True), flush=True)


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
        need(args.build_prefix is not None, 'build-prefix required')
        capture(args)


if __name__ == '__main__':
    main()
