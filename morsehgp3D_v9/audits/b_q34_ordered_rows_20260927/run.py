#!/usr/bin/env python3
"""Ordered-row representation gates and measured W/T, no GPU or engine edit."""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREVIOUS = HERE.parent / 'b_q34_direct_bands_20260927/run.py'
spec = importlib.util.spec_from_file_location('ordered_frozen_recipes', PREVIOUS)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, read, save, sha = base.need, base.read, base.save, base.sha
TARGET = 'mhgp9_ordered_rows'
SCHEMA = 'mhgp9_q34_ordered_rows_v1'


def pins():
    result = base.sources()
    result[str(PREVIOUS)] = sha(PREVIOUS)
    for name in ('rows.hpp', 'probe.cpp', 'run.py', 'CMakeLists.txt'):
        result[str(HERE/name)] = sha(HERE/name)
    return result


def recipe(state):
    out = []
    for kind, compiler in [('release', 'g++'), ('sanitize', 'clang++')]:
        build = state['builds'][kind]
        flags = '-Wall -Wextra -Wpedantic -Werror'
        if kind == 'sanitize':
            flags += ' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
        out += [(kind+'_configure', ['cmake', '-S', str(HERE), '-B', build,
                 '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER='+compiler,
                 '-DCMAKE_CXX_FLAGS='+flags, '-DMHGP9_SOURCE_ROOT='+str(base.ROOT/'morsehgp3D_v9'),
                 '-DMHGP9_GEN_LIBRARY='+str(base.LIBS[kind])], 0),
                (kind+'_build', ['cmake', '--build', build, '--parallel', '2'], 0),
                (kind+'_gate', [state['binaries'][kind], '--gate'], 0)]
        out += [(kind+'_'+mutant, [state['binaries'][kind], '--mutant', mutant], 1)
                for mutant in ('reverse-b', 'mask6')]
    out += [('measure_'+c['name'], [state['binaries']['release'], *c['args']], 0) for c in state['cases']]
    return out


def build_pins(state):
    result = {}
    for binary in state['binaries'].values():
        p = Path(binary)
        for path in (p, p.parent/'CMakeCache.txt', p.parent/'CMakeFiles'/f'{TARGET}.dir/flags.make',
                     p.parent/'CMakeFiles'/f'{TARGET}.dir/link.txt'):
            result[str(path)] = sha(path)
    return result


def validate(value, case):
    need(value['schema'] == SCHEMA and value['status'] == 'pass' and
         value['scope'] == 'ordered_representation_old_pool_paid_no_S2_FULL_GPU', 'measurement scope')
    for key in ('n', 'k', 's', 'mode', 'family', 'input_hash'):
        need(value[key] == case[key], 'case identity '+key)
    m = value['metrics']
    need(all(type(x) is int and x >= 0 for x in m.values()), 'integer counters')
    need(m['F'] == m['FA']+m['FB'] and m['T'] <= min(m['E'], m['W']) and
         m['W'] <= case['k']*(case['k']-1)*m['FB'], 'work/storage bounds')
    need(m['nonempty'] <= m['classes'] and m['histogram_slots'] == 100*m['planned'], 'class work')
    need(m['peak_plan_bytes'] <= m['capacity_bytes'] and m['logical_bytes'] <= m['capacity_bytes'], 'memory ledger')
    prior = read(case['prior'])
    for key, expected in dict(P=prior['rectangle_filter']['P'], E=prior['plans']['E_union'],
                             E3=prior['plans']['E3'], E4=prior['plans']['E4'], F=prior['plans']['F']).items():
        need(m[key] == expected, 'same Pool residue '+key)
    need(all(type(x) in (int, float) and math.isfinite(x) and x >= 0 for x in value['times_ms'].values()), 'finite times')
    need(sum(value['times_ms'][k] for k in ('pool', 'ordered_rows', 'verification')) <= value['times_ms']['total']+.1,
         'measured phases paid')


def check(directory):
    state = read(directory/'capture.json')
    cases, input_pins, partitions = base.cases()
    need(state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    need(state['cases'] == cases and state['partitions'] == partitions, 'input manifest')
    need(state['before'] == state['after'] == pins() | input_pins, 'LIVE source/input closure')
    need(state['build_pins'] == build_pins(state), 'exact LIVE build inventory')
    need(len(state['commands']) == len(recipe(state)), 'complete recipe')
    values, gates = {}, []
    for row, (name, argv, code) in zip(state['commands'], recipe(state)):
        need(row['name'] == name and row['argv'] == argv and row['exit_code'] == code and row['group_closed'], 'command binding')
        need(read(directory/(name+'.command.json')) == row, 'stored command')
        need(all(row.get(k) == v for k, v in read(directory/(name+'.intent.json')).items()), 'intent')
        for stream in ('stdout', 'stderr'):
            need(sha(directory/(name+'.'+stream)) == row[stream+'_sha256'], 'stream identity')
        if name.endswith('_gate'):
            value = read(directory/(name+'.stdout'))
            need(value['schema'] == SCHEMA and value['status'] == 'pass' and value['fronts'] == 96 and
                 value['cases'] > 10000 and value['pairs'] > 10000 and value['implicit'] > 0 and
                 value['order_changes'] > 0 and value['refusals'] == 4, 'gate nonvacuity')
            gates.append(value)
        if name.endswith('_reverse-b'):
            need((directory/(name+'.stderr')).read_text() == 'factor_plan.ordered_original_B\n', 'causal order mutant')
        if name.endswith('_mask6'):
            need((directory/(name+'.stderr')).read_text() == 'factor_plan.ordered_mass\n', 'causal mask mutant')
        if name.startswith('measure_'):
            value = read(directory/(name+'.stdout'))
            validate(value, next(c for c in cases if name == 'measure_'+c['name']))
            values[name[8:]] = value
    need(len(gates) == 2 and gates[0] == gates[1], 'gate agreement')
    return dict(status='PASS', commands=len(state['commands']), gate=gates[0], measurements=values, GCP_used=False)


def capture(directory, prefix):
    need(not directory.exists(), 'fresh receipt')
    cases, inputs, partitions = base.cases()
    builds = {kind: str(prefix)+'_'+kind for kind in base.LIBS}
    need(all(not Path(path).exists() for path in builds.values()), 'fresh builds')
    directory.mkdir(parents=True)
    need(sha(base.HELPER) == base.HELPER_SHA, 'collector pin')
    spec = importlib.util.spec_from_file_location('ordered_commands', base.HELPER)
    collector = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(collector)
    commands = collector.Commands(directory, dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                                  UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    state = dict(status='running', GCP_used=False, cases=cases, partitions=partitions,
                 before=pins() | inputs, commands=[], builds=builds,
                 binaries={kind: str(Path(path)/TARGET) for kind, path in builds.items()})
    save(directory/'capture.json', state)
    try:
        for name, argv, code in recipe(state):
            rc, _, _ = commands.run(name, argv)
            state['commands'] = commands.rows
            save(directory/'capture.json', state)
            need(rc == code, 'unexpected exit '+name)
            if name.startswith('measure_'):
                value = read(directory/(name+'.stdout'))
                validate(value, next(c for c in cases if name == 'measure_'+c['name']))
                print(json.dumps(dict(case=name, metrics=value['metrics'], times=value['times_ms'])), flush=True)
        state['after'] = pins() | base.cases()[1]
        need(state['before'] == state['after'], 'source/input drift')
        state['build_pins'] = build_pins(state)
        state['status'] = 'completed'
        save(directory/'capture.json', state)
        result = check(directory)
        save(directory/'summary.json', result)
        print(json.dumps(dict(status=result['status'], gate=result['gate'], measurements=len(result['measurements']))))
    except BaseException as error:
        state.update(status='failed', error=type(error).__name__+': '+str(error), commands=commands.rows)
        save(directory/'capture.json', state)
        raise


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=('capture', 'check'))
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--build-prefix', type=Path)
    args = parser.parse_args()
    if args.action == 'capture':
        need(args.build_prefix is not None, 'build prefix required')
        capture(args.receipt.resolve(), args.build_prefix.resolve())
    else:
        result = check(args.receipt.resolve())
        print(json.dumps(dict(status=result['status'], gate=result['gate'], measurements=len(result['measurements']))))
