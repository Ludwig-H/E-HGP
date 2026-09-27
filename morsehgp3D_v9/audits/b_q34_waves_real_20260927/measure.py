#!/usr/bin/env python3
"""Read-only CPU measurements, only after the fresh qualification closes."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import signal
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('qualified_real_harness', HERE / 'run.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
need, read, sha, save = gate.need, gate.read, gate.sha, gate.save
base = gate.base


def cases(group):
    frames, pins, partition = base.inputs.grounded('00')
    rows = []
    pieces = ('full',) if group == 'initial' else base.inputs.PARTS[1:]
    for part in pieces:
        entry = frames[part]
        rows.append(dict(name='ng00_' + part, argv=['--frame', entry['path'], '5', '8', '4096'],
                         n=entry['n'], k=5, s=8, Q=4096, mode='frame', family='none',
                         input_hash=entry['point_hash'], part=part))
    if group == 'initial':
        for n in (8000, 16000, 32000):
            prior_path = base.ROOT / f'morsehgp3D_v9/receipts/q34_factor_plan_20260926/measure_uniform_{n}.stdout'
            prior = read(prior_path);pins[str(prior_path)] = sha(prior_path)
            rows.append(dict(name=f'uniform_{n}', argv=['--synthetic', 'uniform', str(n), '5', '8', '4096'],
                             n=n, k=5, s=8, Q=4096, mode='synthetic', family='uniform', input_hash=prior['input_hash']))
    return rows, pins, partition


def qualification(directory):
    summary = gate.readback(directory)
    need(summary['status'] == 'PASS', 'qualification must pass first')
    state = read(directory / 'capture.json')
    pins = {str(directory / 'capture.json'): sha(directory / 'capture.json')}
    pins.update(state['build_pins'])
    return state['binaries']['release'], pins


def planned_commands(state):
    return [('system', ['uname', '-a'], 0)] + [('measure_' + c['name'], [state['binary'], *c['argv']], 0) for c in state['cases']]


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == 'mhgp9_q34_waves_real_measurement_v1' and state['status'] == 'completed' and
         state['GCP_used'] is False, 'closed CPU real measurements')
    need(state['group'] in ('initial', 'cuts'), 'known measurement group')
    rows, input_pins, partition = cases(state['group'])
    binary, qualification_pins = qualification(Path(state['qualification']))
    need(state['cases'] == rows and state['partition'] == partition and state['binary'] == binary, 'input/build binding')
    pins = gate.sources() | input_pins | qualification_pins
    need(state['pins_before'] == state['pins_after'] == pins, 'LIVE complete dependency closure')
    commands = planned_commands(state);gate.check_commands(directory, state, commands)
    results = {}
    for case in rows:
        value = read(directory / ('measure_' + case['name'] + '.stdout'));gate.validate(value, case)
        results[case['name']] = value
    return dict(status='PASS', commands=len(commands), scope='CPU_S2_real_one_ordered_observation_no_FULL_no_GPU',
                group=state['group'], partition=partition, measurements=results, GCP_used=False)


def capture(args):
    directory = args.capture.resolve();qpath = args.qualification.resolve()
    need(not directory.exists(), 'fresh measurement capture required')
    binary, qualification_pins = qualification(qpath)
    rows, input_pins, partition = cases(args.group)
    directory.mkdir(parents=True)
    state = dict(schema='mhgp9_q34_waves_real_measurement_v1', status='failed', GCP_used=False,
                 group=args.group, qualification=str(qpath), binary=binary, cases=rows, partition=partition,
                 pins_before=gate.sources() | input_pins | qualification_pins)
    need(sha(base.HELPER) == base.HELPER_SHA, 'local process collector pin')
    spec = importlib.util.spec_from_file_location('real_measure_process_collector', base.HELPER)
    helper = importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    commands = helper.Commands(directory, dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    try:
        for name, argv, code in planned_commands(state):
            rc, _, _ = commands.run(name, argv, timeout=None)
            need(rc == code, name + ': unexpected exit ' + str(rc))
            if name.startswith('measure_'):
                case = next(c for c in rows if name == 'measure_' + c['name'])
                value = read(directory / (name + '.stdout'));gate.validate(value, case)
                print(json.dumps(dict(case=case['name'], metrics=value['metrics'], times_ms=value['times_ms'],
                                      rss_bytes=value['rss_bytes'])), flush=True)
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
    summary = readback(directory);save(directory / 'summary.json', summary)
    print(json.dumps(summary), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--capture', type=Path)
    action.add_argument('--readback', type=Path)
    parser.add_argument('--qualification', type=Path)
    parser.add_argument('--group', choices=('initial', 'cuts'), default='initial')
    args = parser.parse_args()
    if args.readback:
        print(json.dumps(readback(args.readback.resolve()), sort_keys=True))
    else:
        need(args.qualification is not None, 'qualification directory required')
        capture(args)


if __name__ == '__main__':
    main()
