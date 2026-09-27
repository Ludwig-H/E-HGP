#!/usr/bin/env python3
"""Fresh bounded-wave CPU gate, no benchmark or GCP claim."""
import importlib.util
from pathlib import Path
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE_FILE = HERE.parent / 'b_q34_collective_arena_20260927/run.py'
spec = importlib.util.spec_from_file_location('wave_collective_collector', BASE_FILE)
collective = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collective)
base = collective.base
need, read, sha, save = base.need, base.read, base.sha, base.save
need(sha(BASE_FILE) == '3a0e7b684b7118e7657c8a810661f5b655d6e2e764978060b4973e7bc4f8b119', 'frozen collective orchestration')
OLD_SOURCES, OLD_RECIPE = base.sources, base.recipe
base.HERE = HERE
base.TARGET = 'mhgp9_q34_arena_waves'
base.SCHEMA = 'mhgp9_q34_arena_waves_capture_v1'
base.PROBE = 'mhgp9_q34_arena_waves_v1'


def sources():
    paths = [HERE / name for name in ('waves.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py')]
    return OLD_SOURCES() | {str(p): sha(p) for p in paths}


def recipe(state):
    rows = []
    for name, argv, code in OLD_RECIPE(state):
        if name.startswith('mutant_'):
            continue
        rows.append((name, argv, code))
        if name.startswith('gate_'):
            kind = name.split('_', 1)[1]
            rows += [('mutant_' + kind + '_' + m, [state['binaries'][kind], '--mutant', m], 1)
                     for m in ('mask6', 'fallback', 'ordinal')]
    return rows


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == base.SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False,
         'closed local wave capture')
    need(state['cases'] == [] and state['partitions'] == [], 'gate scope only')
    need(state['pins_before'] == state['pins_after'] == sources(), 'LIVE source closure')
    need(state['build_pins'] == base.build_pins(state), 'LIVE binary closure')
    commands = recipe(state)
    need(len(commands) == 15 and len(state['commands']) == 15, 'complete command inventory')
    gates, mutants = [], []
    for command, (name, argv, code) in zip(state['commands'], commands):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command binding')
        need(read(directory / (name + '.command.json')) == command, 'stored command')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == base.PROBE and value['status'] == 'pass', 'gate verdict')
            c = value['coverage']
            need(c['batches'] == 175 and c['consumptions'] == 1050 and c['refusals'] == 20, 'gate inventory')
            for key in ('pairs', 'survivors', 'pool_rejected', 'pool_lane_rejected', 'fallback_survivors',
                        'band_splits', 'rectangle_crossings', 'reordered', 'empty', 'no_survivors', 'closed', 'holes', 'retries'):
                need(type(c[key]) is int and c[key] > 0, 'non-vacuous coverage: ' + key)
            gates.append(value)
        if name.startswith('mutant_'):
            value = read(directory / (name + '.stdout'))
            kind = name.rsplit('_', 1)[1]
            causes = {'mask6': ('waves.physical_queries',), 'fallback': ('waves.physical_queries',),
                      'ordinal': ('waves.duplicate_ordinal', 'gate.survivors_order_mask')}
            need(value['schema'] == base.PROBE and value['status'] == 'failed' and value['cause'] in causes[kind],
                 'causal native mutant: ' + name)
            mutants.append(dict(name=name, cause=value['cause']))
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitize agreement')
    return dict(status='PASS', commands=15, gate=gates[0], mutants=mutants, GCP_used=False,
                scope='bounded_waves_CPU_S2_gate_only_no_FULL')


base.sources, base.recipe, base.readback = sources, recipe, readback
base.cases = lambda: ([], {}, [])
if __name__ == '__main__':
    base.main()
