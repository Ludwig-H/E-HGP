#!/usr/bin/env python3
"""Explicit reuse of the frozen local collector, with a new recipe and judge.

All old source/build authorities remain unchanged. This captures a fresh
collective binary and checks its own protocol, never an inherited result.
"""
import importlib.util
import math
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE_FILE = HERE.parent / 'b_q34_direct_bands_20260927/run.py'
spec = importlib.util.spec_from_file_location('collective_local_orchestrator', BASE_FILE)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
need, sha, read = base.need, base.sha, base.read
need(sha(BASE_FILE) == '3165f540438340a23792b0c26217b20bc0eec07635c6f661a7acdffa8ff249e5', 'frozen orchestration pin')
OLD_CASES, OLD_RECIPE = base.cases, base.recipe
base.HERE = HERE
base.TARGET = 'mhgp9_audit_q34_collective'
base.SCHEMA = 'mhgp9_q34_collective_capture_v1'
base.PROBE = 'mhgp9_q34_collective_v1'
SCHEMA, PROBE = base.SCHEMA, base.PROBE


def cases():
    rows, pins, partitions = OLD_CASES()
    # Existing input helper verifies all three manifests; only these ten
    # cases are measured. No claim on unmeasured scenes K10/s10/s12 follows.
    return [r for r in rows if r['name'] == 'ng_00_full' or r['mode'] == 'synthetic'], pins, partitions


def sources():
    paths = [p for p in (base.ROOT / 'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / name for name in ('arena.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py')]
    paths += [base.OLD / name for name in ('plan.hpp', 'probe.cpp', 'inputs.py')]
    paths += [BASE_FILE, BASE_FILE.parent / 'direct.hpp', base.HELPER, *base.LIBS.values(),
              base.ROOT / 'morsehgp3D_v9/tests/gen/front_fixtures.hpp']
    return {str(p): sha(p) for p in sorted(set(paths))}


def recipe(state):
    rows = []
    for name, argv, code in OLD_RECIPE(state):
        if name.startswith('mutant_') and name.endswith('_ids'):
            continue
        if name.startswith('mutant_') and name.endswith('_overlap'):
            name = name[:-7] + 'scatter'
            argv[-1] = 'scatter'
        rows.append((name, argv, code))
    return rows


def validate(value, case):
    need(value.get('schema') == PROBE and value.get('status') == 'pass', 'successful collective probe')
    for name in ('n', 'k', 's', 'family', 'mode', 'input_hash'):
        need(value.get(name) == case[name], 'input recipe: ' + name)
    need(value['seed'] == 3 and value['min_factor'] == 2 and value['grain'] == 128 and
         value['scope'] == 'collective_CPU_no_S2_no_FULL_no_GPU', 'collective scope')
    m = value['metrics']
    need(all(type(x) is int and x >= 0 for x in m.values()), 'nonnegative integer counters')
    need(m['anchor_visits'] == m['F'] and m['grouping_reads'] == 3*m['F'] and m['scatter_writes'] == m['F'],
         'geometry once, credit rescans paid')
    need(m['histogram_slots'] == 644*m['planned'] and m['band_passes'] == 2*m['planned'] and
         m['prefix_entries'] == 5*m['planned'], 'count/prefix/scatter work')
    need(m['final_buffers'] == 6 and m['owned_array_peak_one'] >= m['retained_bytes'] and
         m['owned_array_peak_four'] >= m['retained_bytes'], 'collective capacity accounting')
    prior = read(case['prior'])
    for key, expected in dict(P=prior['rectangle_filter']['P'], E=prior['plans']['E_union'],
                             F=prior['plans']['F'], planned=prior['plans']['rectangles'],
                             corner_tests=prior['plans']['corner_tests'], selected_sites=prior['plans']['selected_sites']).items():
        need(m[key] == expected, 'same geometry and residual: ' + key)
    t = value['times_ms']
    need(all(type(x) in (int, float) and math.isfinite(x) and x >= 0 for x in t.values()), 'finite timing')
    need(t['total'] + .1 >= sum(t[key] for key in ('input', 'index', 'front_filter', 'one_ab', 'four_ab',
         'four_ba', 'one_ba', 'comparison', 'direct_reference_check')), 'all measured stages paid')


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    current_cases, pins, partitions = cases()
    need(state['cases'] == current_cases and state['partitions'] == partitions, 'recipe/input mapping')
    need(state['pins_before'] == state['pins_after'] == sources() | pins, 'LIVE dependency closure')
    need(state['build_pins'] == base.build_pins(state), 'LIVE build identity')
    commands = recipe(state)
    need(len(state['commands']) == len(commands), 'complete command inventory')
    gates, rows = [], {}
    for command, (name, argv, code) in zip(state['commands'], commands):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command recipe/closure')
        need(read(directory / (name + '.command.json')) == command, 'stored command binding')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'command intent')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == PROBE and value['status'] == 'pass' and value['mode'] == 'gate', 'gate verdict')
            c = value['coverage']
            need(c['batches'] == 96 and c['rectangles'] > 0 and c['ranks'] > 0 and c['pairs'] > 1000 and
                 c['pool_ids'] > 0 and c['refusals'] == 72 and c['worker_throws'] == 8, 'non-vacuous collective gate')
            gates.append(value)
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            expected = 'factor_plan.arena_' + ('scatter_rank' if name.endswith('scatter') else 'pair_mask')
            need(value['schema'] == PROBE and value['status'] == 'failed' and value['cause'] == expected, 'causal mutant')
        if name.startswith('measure_'):
            case = next(c for c in current_cases if name == 'measure_' + c['name'])
            value = read(directory / (name + '.stdout'))
            validate(value, case)
            t = value['times_ms']
            one, four = (t['one_ab']+t['one_ba'])/2, (t['four_ab']+t['four_ba'])/2
            rows[case['name']] = dict(metrics=value['metrics'], times_ms=t, one_mean_ms=one, four_mean_ms=four,
                                     speedup=one/four, ratios_by_order=[t['one_ab']/t['four_ab'], t['one_ba']/t['four_ba']])
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer gate equality')
    return dict(status='PASS', commands=len(commands), gate=gates[0], measurements=rows,
                scope='collective_CPU_no_S2_no_FULL_no_GPU', GCP_used=False)


base.cases, base.sources, base.recipe = cases, sources, recipe
base.validate, base.readback = validate, readback
save = base.save

if __name__ == '__main__':
    base.main()
