#!/usr/bin/env python3
"""Capture only offline protocol tests; no VM, credentials or package launch."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import common as c


def source_paths():
    paths = [c.HERE/name for name in ('common.py', 'package.py', 'worker.py', 'session.py',
                                    'selftest.py', 'run_local.py', 'compile_contract.py', 'README.md')]
    paths += [c.ROOT/name for name in (
        c.HELPER, 'gcp-migration/selftest_full_probe_session_v7.py', c.DATA_SOURCE,
        'morsehgp3D_v9/audits/b_q34_cuda_session_20260927/session.py',
        c.PROTOTYPE+'/CMakeLists.txt', c.PROTOTYPE+'/probe.cpp', c.SURVIVORS+'/device_cuda.cu',
        c.DIRECT_GATE+'/device_gate.cu', 'morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/device_cuda.cu',
        'morsehgp3D_v9/src/gpu/witness_filter.hpp')]
    return paths


def sources():
    return {str(path.relative_to(c.ROOT)): c.sha(path) for path in source_paths()}


def recipes():
    return [(name, [sys.executable, '-B', *optimized, str(script)])
            for name, script, optimized in (
                ('selftest_normal', c.HERE/'selftest.py', []), ('selftest_optimized', c.HERE/'selftest.py', ['-O']),
                ('legacy_normal', c.ROOT/'gcp-migration/selftest_full_probe_session_v7.py', []),
                ('legacy_optimized', c.ROOT/'gcp-migration/selftest_full_probe_session_v7.py', ['-O']),
                ('inert_normal', c.HERE/'session.py', []), ('inert_optimized', c.HERE/'session.py', ['-O']))]


def judged(directory, record):
    c.need(record.get('status') == 'passed' and record.get('GCP_used') is False and
           record.get('scope') == 'offline_protocol_only' and record.get('measured_data') is False,
           'offline receipt scope')
    before, after = c.read(directory/'sources_before.json'), c.read(directory/'sources_after.json')
    c.need(before == after == sources(), 'live source closure')
    c.need(record['python_sha256'] == c.sha(sys.executable), 'Python executable pin')
    rows = record['commands']
    commands = recipes()
    c.need(len(rows) == len(commands), 'complete offline commands')
    values = {}
    for row, (name, argv) in zip(rows, commands):
        c.need(row['name'] == name and row['argv'] == argv and row['exit_code'] == 0 and
               row == c.read(directory/(name+'.command.json')), 'exact closed offline recipe')
        for stream in ('stdout', 'stderr'):
            c.need(c.sha(directory/(name+'.'+stream)) == row[stream+'_sha256'], 'offline stream pin')
        c.need(not (directory/(name+'.stderr')).read_bytes(), 'clean offline stderr')
        values[name] = c.read(directory/(name+'.stdout'))
    c.need(values['selftest_normal'] == values['selftest_optimized'], 'same normal and optimized protocol judgment')
    check = values['selftest_normal']
    c.need(check['status'] == 'passed' and check['real_subprocesses'] == 0 and check['GCP_used'] is False and
           check['measured_data'] is False and check['controller_wait_scenarios'] == 5 and
           type(check['positive']) is int and check['positive'] >= 72 and
           type(check['rejected']) is int and check['rejected'] >= 202, 'nonvacuous pure protocol test')
    c.need(all(before[str(Path(path).relative_to(c.ROOT))] == pin for path, pin in check['source_sha256'].items()),
           'selftest and capture source identity')
    legacy = values['legacy_normal']
    c.need(legacy == values['legacy_optimized'] == check['legacy'] and legacy['status'] == 'passed' and
           legacy['real_subprocesses'] == 0 and legacy['mocked_commands'] == 3 and legacy['GCP_used'] is False and
           legacy['positive_predicates'] == 11 and legacy['rejected_predicates'] == 22, 'same independent helper tests')
    inert = values['inert_normal']
    c.need(inert == values['inert_optimized'] and inert['status'] == 'inert' and
           inert['GCP_used'] is False and inert['FULL_executed'] is False and inert['target'] == c.TARGET and
           inert['useful_seconds'] == 450 and inert['guest_minutes'] == 30, 'inert default')
    return dict(status='passed', scope=record['scope'], commands=len(rows),
        positive=check['positive'], rejected=check['rejected'], GCP_used=False, measured_data=False)


def capture(directory):
    c.need(directory.is_relative_to(c.HERE/'checks') and directory != c.HERE/'checks',
           'capture belongs inside this protocol checks directory')
    directory.mkdir(parents=True, exist_ok=False)
    before = sources()
    c.save(directory/'sources_before.json', before)
    result = dict(schema=c.SCHEMA, status='failed', scope='offline_protocol_only',
        GCP_used=False, measured_data=False, python_sha256=c.sha(sys.executable), commands=[])
    try:
        for name, argv in recipes():
            start = time.monotonic()
            process = subprocess.run(argv, capture_output=True, timeout=120, check=False)
            row = dict(name=name, argv=argv, exit_code=process.returncode, elapsed_seconds=time.monotonic()-start)
            for stream, raw in (('stdout', process.stdout), ('stderr', process.stderr)):
                with (directory/(name+'.'+stream)).open('xb') as output:
                    output.write(raw)
                row[stream+'_sha256'] = hashlib.sha256(raw).hexdigest()
            c.save(directory/(name+'.command.json'), row)
            result['commands'].append(row)
            c.need(process.returncode == 0, 'offline command failed: '+name)
        result['status'] = 'passed'
    except BaseException as error:
        result['error'] = type(error).__name__+': '+str(error)
    finally:
        after = sources()
        c.save(directory/'sources_after.json', after)
        if before != after:
            result['status'] = 'failed'
            result['closure_error'] = 'sources changed during capture'
        if result['status'] == 'passed':
            try:
                judged(directory, result)
            except BaseException as error:
                result['status'] = 'failed'
                result['judgment_error'] = type(error).__name__+': '+str(error)
        c.save(directory/'receipt.json', result)
    return judged(directory, result)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--readback', action='store_true')
    args = parser.parse_args()
    directory = args.directory.resolve()
    result = judged(directory, c.read(directory/'receipt.json')) if args.readback else capture(directory)
    print(json.dumps(result, sort_keys=True))
