#!/usr/bin/env python3
"""Capture/replay offline reader tests, never an actual G4 experiment."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time
import readback as r


def sources():
    paths = [r.HERE/name for name in ('readback.py', 'selftest.py', 'qualify.py')]
    paths += [r.PROTOCOL/name for name in ('common.py', 'package.py', 'session.py', 'worker.py', 'selftest.py')]
    paths += [r.ROOT/name for name in (r.c.HELPER,
        'morsehgp3D_v9/audits/b_q34_cuda_g4_readback_20260927/readback.py',
        'morsehgp3D_v9/audits/b_q34_cuda_g4_readback_20260927/selftest.py',
        r.c.DATA_SOURCE, r.c.PROTOTYPE+'/CMakeLists.txt', r.c.PROTOTYPE+'/probe.cpp',
        r.c.PROTOTYPE+'/device_cuda.cu', 'morsehgp3D_v9/src/gpu/witness_filter.hpp')]
    return {str(path.relative_to(r.ROOT)): r.sha(path) for path in paths}


def recipes():
    return [(name, [sys.executable, '-B', *options, str(r.HERE/'selftest.py')])
            for name, options in (('normal', []), ('optimized', ['-O']))]


def readback(directory, receipt=None):
    receipt = r.read(directory/'receipt.json') if receipt is None else receipt
    r.need(receipt.get('status') == 'passed' and receipt.get('scope') == 'offline_publication_reader_only' and
           receipt.get('GCP_used') is False and receipt.get('measured_data') is False, 'offline reader scope')
    r.need(sources() == r.read(directory/'sources_before.json') == r.read(directory/'sources_after.json'), 'live source closure')
    r.need(receipt['python_sha256'] == r.sha(sys.executable), 'Python executable pin')
    expected = recipes()
    rows = receipt['commands']
    r.need(len(rows) == len(expected), 'both normal and optimized commands')
    outputs = []
    for row, (name, argv) in zip(rows, expected):
        r.need(row['name'] == name and row['argv'] == argv and row['exit_code'] == 0 and
               row == r.read(directory/(name+'.command.json')), 'exact closed offline command')
        for stream in ('stdout', 'stderr'):
            r.need(r.sha(directory/(name+'.'+stream)) == row[stream+'_sha256'], 'offline stream pin')
        r.need(not (directory/(name+'.stderr')).read_bytes(), 'clean offline stderr')
        outputs.append(r.read(directory/(name+'.stdout')))
    r.need(outputs[0] == outputs[1], 'same normal/optimized judgment')
    value = outputs[0]
    r.need(value['schema'] == r.SCHEMA and value['status'] == 'passed' and value['GCP_used'] is False and
           value['measured_data'] is False and value['real_subprocesses'] == 0 and
           value['positive'] >= 17 and value['refused'] >= 21 and
           value['reused_fixture_positive'] >= 5 and value['reused_fixture_rejected'] >= 44, 'nonvacuous synthetic reader gates')
    return dict(status='passed', scope=receipt['scope'], commands=len(rows), positive=value['positive'],
        refused=value['refused'], fixture_positive=value['reused_fixture_positive'],
        fixture_refused=value['reused_fixture_rejected'], GCP_used=False, measured_data=False)


def capture(directory):
    r.need(directory.is_relative_to(r.HERE/'checks') and directory != r.HERE/'checks', 'capture scope inside owned checks')
    directory.mkdir(parents=True, exist_ok=False)
    before = sources()
    r.c.save(directory/'sources_before.json', before)
    receipt = dict(status='failed', scope='offline_publication_reader_only', GCP_used=False,
        measured_data=False, python_sha256=r.sha(sys.executable), commands=[])
    try:
        for name, argv in recipes():
            start = time.monotonic()
            value = subprocess.run(argv, capture_output=True, timeout=120, check=False)
            row = dict(name=name, argv=argv, exit_code=value.returncode, elapsed_seconds=time.monotonic()-start)
            for stream, raw in (('stdout', value.stdout), ('stderr', value.stderr)):
                with (directory/(name+'.'+stream)).open('xb') as target:
                    target.write(raw)
                row[stream+'_sha256'] = r.sha(directory/(name+'.'+stream))
            r.c.save(directory/(name+'.command.json'), row)
            receipt['commands'].append(row)
            r.need(value.returncode == 0, 'offline reader command failed: '+name)
        receipt['status'] = 'passed'
    except BaseException as error:
        receipt['error'] = type(error).__name__+': '+str(error)
    finally:
        after = sources()
        r.c.save(directory/'sources_after.json', after)
        if before != after:
            receipt['status'] = 'failed'
            receipt['closure_error'] = 'sources changed during qualification'
        if receipt['status'] == 'passed':
            try:
                readback(directory, receipt)
            except BaseException as error:
                receipt['status'] = 'failed'
                receipt['judgment_error'] = type(error).__name__+': '+str(error)
        r.c.save(directory/'receipt.json', receipt)
    return readback(directory)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--readback', action='store_true')
    args = parser.parse_args()
    result = readback(args.directory.resolve()) if args.readback else capture(args.directory.resolve())
    print(json.dumps(result, sort_keys=True))
