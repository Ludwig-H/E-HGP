#!/usr/bin/env python3
"""Fresh qualification before any real-wave benchmark."""
import importlib.util
import math
from pathlib import Path
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE_FILE = HERE.parent / 'b_q34_arena_waves_20260927/run.py'
spec = importlib.util.spec_from_file_location('real_frozen_wave_collector', BASE_FILE)
waves = importlib.util.module_from_spec(spec)
spec.loader.exec_module(waves)
base = waves.base
need, read, sha, save = base.need, base.read, base.sha, base.save
need(sha(BASE_FILE) == '61640b164bec212b1b8dd8277687206dc45332861334c76bec17790594283b4d', 'frozen wave runner')
OLD_SOURCES, OLD_RECIPE = base.sources, base.recipe
base.HERE = HERE
base.TARGET = 'mhgp9_q34_waves_real'
base.SCHEMA = 'mhgp9_q34_waves_real_qualification_v1'
QUICK = [('uniform', '7'), ('terrain', '1'), ('clusters', '64')]


def sources():
    paths = [HERE / name for name in ('probe.cpp', 'CMakeLists.txt', 'run.py', 'measure.py')]
    return OLD_SOURCES() | {str(p): sha(p) for p in paths}


def recipe(state):
    rows = []
    for name, argv, code in OLD_RECIPE(state):
        rows.append((name, argv, code))
        if name.startswith('gate_'):
            kind = name.split('_', 1)[1]
            rows += [('quick_' + kind + '_' + family,
                      [state['binaries'][kind], '--synthetic', family, '64', '5', '8', q], 0) for family, q in QUICK]
    return rows


def validate(value, case):
    need(value['schema'] == 'mhgp9_q34_waves_real_v1' and value['status'] == 'pass' and
         value['scope'] == 'CPU_S2_no_FULL_no_GPU', 'real probe verdict/scope')
    for key in ('n', 'k', 's', 'Q', 'mode', 'family'):
        need(value[key] == case[key], 'case identity: ' + key)
    if 'input_hash' in case:
        need(value['input_hash'] == case['input_hash'], 'represented input hash')
    need(value['seed'] == 3 and value['prepare_workers'] == value['cursor_workers'] == value['native_workers'] == 1 and
         value['order'] == 'candidate_then_native', 'mono ordered observation')
    m, t, rss = value['metrics'], value['times_ms'], value['rss_bytes']
    need(all(type(x) is int and x >= 0 for x in m.values()), 'nonnegative integer metrics')
    need(all(type(x) is int and x > 0 for x in rss.values()), 'Linux process RSS records')
    need(m['Praw'] >= m['P'] >= m['E'] >= m['S'], 'nested union masses')
    need(m['P3'] >= m['E3'] >= m['S3'] and m['P4'] >= m['E4'] >= m['S4'], 'nested lane masses')
    need(m['queries'] == m['E'] and m['q3_queries'] == m['E3'] and m['q4_queries'] == m['E4'], 'physical query counts')
    for lane in ('3', '4'):
        need(m['S' + lane] == m['P' + lane] - m['native_rejected' + lane] ==
             m['E' + lane] - m['point_rejected' + lane], 'lane rejection partition')
    need(max(m['S3'], m['S4']) <= m['S'] <= m['S3'] + m['S4'], 'survivor lane union')
    need(m['slot_capacity_peak'] <= value['Q'] and m['segments'] == m['bands'] + m['fallbacks'], 'bounded slots, complete fallback')
    need(m['planned'] + m['fallbacks'] + m['closed_rectangles'] == m['rectangles'], 'rectangle partition')
    need(m['prepared_retained_bytes'] >= m['arena_retained_bytes'] and
         m['cursor_array_peak_bytes'] >= m['native_output_capacity_bytes'], 'separate capacity scopes')
    need(all(type(x) in (int, float) and math.isfinite(x) and x >= 0 for x in t.values()), 'finite stage times')
    stages = sum(t[key] for key in ('candidate_prepare', 'candidate_consume', 'candidate_sort_convert', 'candidate_internal_cleanup'))
    need(abs(t['candidate_total'] - stages - t['candidate_observation_overhead']) < .01, 'instrumented candidate chronology')
    need(t['candidate_input_to_S2'] + .01 >= t['shared_elapsed_to_candidate'] + t['candidate_total'], 'input-to-S2 wall interval')
    need(t['experiment_total'] + .01 >= t['candidate_input_to_S2'] + t['native_total'] + t['comparison'] +
         t['final_output_input_cleanup'], 'oracle and destruction paid outside candidate')
    need(rss['hwm_after_input'] <= rss['hwm_before_candidate'] <= rss['hwm_after_candidate'] <=
         rss['hwm_after_native'] <= rss['hwm_final'], 'process high-water monotonicity, not phase peaks')


def check_commands(directory, state, commands):
    need(len(state['commands']) == len(commands), 'complete command inventory')
    for command, (name, argv, code) in zip(state['commands'], commands):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command binding')
        need(read(directory / (name + '.command.json')) == command, 'stored command')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == base.SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed qualification')
    need(state['cases'] == [] and state['partitions'] == [], 'qualification only')
    need(state['pins_before'] == state['pins_after'] == sources(), 'LIVE source closure')
    need(state['build_pins'] == base.build_pins(state), 'LIVE binary closure')
    commands = recipe(state);need(len(commands) == 21, 'qualification command count')
    check_commands(directory, state, commands)
    gates, quick, mutants = [], {}, []
    for name, _, _ in commands:
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == 'mhgp9_q34_arena_waves_v1' and value['status'] == 'pass' and
                 value['coverage']['batches'] == 175 and value['coverage']['consumptions'] == 1050, 'fresh frozen gate execution')
            gates.append(value)
        if name.startswith('quick_'):
            kind, family = name.removeprefix('quick_').split('_', 1)
            case = dict(n=64, k=5, s=8, Q=int(dict(QUICK)[family]), mode='synthetic', family=family)
            value = read(directory / (name + '.stdout'));validate(value, case)
            quick[name] = value
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            cause = 'waves.duplicate_ordinal' if name.endswith('_ordinal') else 'waves.physical_queries'
            need(value['status'] == 'failed' and value['cause'] == cause, 'causal frozen mutant')
            mutants.append(dict(name=name, cause=cause))
    need(len(gates) == 2 and gates[0] == gates[1], 'fresh Release/sanitize gate agreement')
    for family, _ in QUICK:
        a, b = quick['quick_release_' + family], quick['quick_sanitize_' + family]
        need(a['metrics'] == b['metrics'] and a['input_hash'] == b['input_hash'] and a['rectangle_hash'] == b['rectangle_hash'],
             'quick measurement discrete identity')
    return dict(status='PASS', commands=21, gate=gates[0], quick=quick, mutants=mutants, GCP_used=False,
                scope='fresh_harness_qualification_not_real_benchmark')


base.sources, base.recipe, base.readback = sources, recipe, readback
base.cases = lambda: ([], {}, [])
if __name__ == '__main__':
    base.main()
