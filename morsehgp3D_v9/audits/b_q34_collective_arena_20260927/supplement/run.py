#!/usr/bin/env python3
"""Fresh supplementary exact-refusal gate; no main-capture source changes."""
import importlib.util
from pathlib import Path
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('collective_supplement_source', HERE.parent / 'run.py')
main = importlib.util.module_from_spec(spec)
spec.loader.exec_module(main)
base = main.base
need, read, sha = base.need, base.read, base.sha
OLD_SOURCES, OLD_RECIPE = base.sources, base.recipe
base.HERE = HERE
base.TARGET = 'mhgp9_collective_supplement'
base.SCHEMA = 'mhgp9_collective_supplement_capture_v1'
base.PROBE = 'mhgp9_collective_supplement_v1'


def sources():
    paths = [HERE / name for name in ('gate.cpp', 'CMakeLists.txt', 'run.py')]
    return OLD_SOURCES() | {str(p): sha(p) for p in paths}


def recipe(state):
    return [(name, argv, code) for name, argv, code in OLD_RECIPE(state)
            if not name.startswith('mutant_')]


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == base.SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False,
         'closed local supplementary capture')
    need(state['cases'] == [] and state['partitions'] == [], 'no benchmark claims')
    need(state['pins_before'] == state['pins_after'] == sources(), 'LIVE source closure')
    need(state['build_pins'] == base.build_pins(state), 'LIVE binary closure')
    commands = recipe(state)
    need(len(commands) == 9 and len(state['commands']) == 9, 'complete command inventory')
    gates = []
    for command, (name, argv, code) in zip(state['commands'], commands):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command binding')
        need(read(directory / (name + '.command.json')) == command, 'stored command')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            need(value['schema'] == base.PROBE and value['status'] == 'pass' and value['positive_plans'] == 18 and
                 value['exact_refusals'] == 78 and value['query_refusals'] == 18 and value['alias_checks'] == 18 and
                 value['permuted_sites'] > 0 and value['nonidentity_original_ids'] > 0, 'causal supplementary coverage')
            gates.append(value)
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitize agreement')
    return dict(status='PASS', commands=9, gate=gates[0], GCP_used=False, scope='supplementary_API_gate_only')


base.sources, base.recipe, base.readback = sources, recipe, readback
base.cases = lambda: ([], {}, [])
if __name__ == '__main__':
    base.main()
