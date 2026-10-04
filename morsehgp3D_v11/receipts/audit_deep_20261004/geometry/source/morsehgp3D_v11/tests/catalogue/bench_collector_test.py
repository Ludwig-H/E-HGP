#!/usr/bin/env python3
"""Collector-only failures with tiny Python children; no native build or geometric computation."""
import argparse
import contextlib
import hashlib
import io
import json
from pathlib import Path
import signal
import sys
import tempfile
import types
from unittest.mock import patch


CHECKS = 0
SOURCE = Path(__file__).resolve().parents[2] / 'bench' / 'catalogue_g4.py'
sys.path.insert(0, str(SOURCE.parent))
BENCH = types.ModuleType('catalogue_g4_collector_under_test')
exec(compile(SOURCE.read_text(), str(SOURCE), 'exec'), BENCH.__dict__)

CHILD = r'''
import json, os, signal, sys, time
from pathlib import Path
mode = Path(sys.argv[1]).name
if mode in ('malformed', 'duplicate', 'nonfinite', 'not_object', 'refused', 'failed', 'timeout', 'signal'):
    print('stderr:' + mode, file=sys.stderr, flush=True)
cloud = {'phase': 'cloud', 'points': 3, 'sites': 3, 'cloud_ns': 20, 'read_ns': 10}
catalogue = {'phase': 'catalogue', 'status': 'ok', 'balls': 1, 'wall_ns': 30}
if mode == 'wrong_count': cloud['points'] = 2
if mode == 'negative': catalogue['wall_ns'] = -1
if mode == 'string_time': catalogue['wall_ns'] = '30'
if mode == 'missing_field': del cloud['read_ns']
print(json.dumps(cloud), flush=True)
if mode in ('malformed', 'refused', 'failed', 'timeout', 'signal'):
    print('{"phase":', flush=True)
    if mode == 'timeout': time.sleep(30)
    if mode == 'signal': os.kill(os.getpid(), signal.SIGTERM)
    sys.exit({'refused': 2, 'failed': 7}.get(mode, 0))
if mode == 'duplicate': print('{"phase":"extra","phase":"extra"}')
if mode == 'nonfinite': print('{"phase":"extra","value":NaN}')
if mode == 'not_object': print('[]')
print(json.dumps(catalogue))
print(json.dumps({'phase': 'exit', 'status': 'ok'}))
if mode != 'missing':
    Path(sys.argv[3]).write_bytes(b'' if mode == 'empty' else b'canonical fixture bytes')
'''


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def make_args(root, executable, leaf_size=16):
    return argparse.Namespace(exe=executable, data=root / 'data', work=root / 'work',
                              out=root / 'out', qualification=root / 'qualification.json',
                              timeout=1, leaf_size=leaf_size)


def check_attempt(row, status, stage=None):
    require(row['status'] == status, 'attempt verdict: ' + str(row))
    require(type(row['process_wall_seconds']) is float and row['process_wall_seconds'] >= 0,
            'duration missing or invalid')
    require(isinstance(row['stdout'], str) and isinstance(row['stderr'], str), 'streams missing')
    require(isinstance(row['events'], list) and isinstance(row['errors'], list), 'event/error lists missing')
    for error in row['errors']:
        require(set(error) == {'stage', 'type', 'message'} and all(isinstance(v, str) for v in error.values()),
                'structured error schema')
    if stage:
        require(any(error['stage'] == stage for error in row['errors']), 'missing error stage ' + stage)
    require(row['argv'][5] == '16', 'leaf-size not passed to native argument')


def attempt_cases(root, executable):
    args = make_args(root, executable)
    args.work.mkdir()
    calls = 0

    def attempt(mode, exe=executable):
        nonlocal calls
        calls += 1
        case = {'name': mode, 'coordinates': mode, 'point_ids': 'ids', 'count': 3}
        return BENCH.measure(exe, case, 5, 0, args)

    row = attempt('ok')
    check_attempt(row, 'ok')
    require(row['errors'] == [] and row['exit_code'] == 0, 'successful child falsely rejected')
    require(row['canonical_sha256'] == hashlib.sha256(b'canonical fixture bytes').hexdigest(), 'artifact hash')
    require(row['catalogue_ms'] == 30 / 1e6 and row['catalogue_within_100ms'] is True, 'native duration')
    require(not (args.work / 'ok_k5_r0.bin').exists(), 'successful artifact not cleaned')
    for mode in ('malformed', 'duplicate', 'nonfinite', 'not_object'):
        row = attempt(mode)
        check_attempt(row, 'invalid_output', 'events')
        require(row['exit_code'] == 0 and len(row['events']) >= 1, 'valid event prefix lost')
        require('stderr:' + mode in row['stderr'] and 'canonical_sha256' not in row, 'invalid JSON promoted')
    for mode in ('wrong_count', 'negative', 'string_time', 'missing_field'):
        row = attempt(mode)
        check_attempt(row, 'invalid_output', 'success')
        require(row['exit_code'] == 0 and len(row['events']) == 3, 'invalid success evidence lost')
    for mode in ('missing', 'empty'):
        row = attempt(mode)
        check_attempt(row, 'artifact_error', 'artifact')
        require(row['exit_code'] == 0 and len(row['events']) == 3, 'artifact failure lost process result')
    for mode, code, status in (('refused', 2, 'refused'), ('failed', 7, 'failed'),
                                ('signal', -signal.SIGTERM, 'failed'), ('timeout', None, 'timeout')):
        row = attempt(mode)
        check_attempt(row, status, 'events')
        require(row['exit_code'] == code and len(row['events']) == 1, 'first process verdict/prefix lost')
        require('{"phase":\n' in row['stdout'] and 'stderr:' + mode in row['stderr'], 'partial streams lost')
        require('canonical_sha256' not in row, 'failed process received canonical hash')
        if status == 'timeout':
            require(any(error['stage'] == 'process' for error in row['errors']), 'timeout error missing')
    row = attempt('launch', root / 'missing_executable')
    check_attempt(row, 'launch_error', 'launch')
    require(row['exit_code'] is None and row['stdout'] == row['stderr'] == '', 'launch evidence invalid')
    require('canonical_sha256' not in row, 'launch failure received hash')
    with patch.object(BENCH, 'digest', side_effect=OSError('injected hash read failure')):
        row = attempt('ok')
    check_attempt(row, 'artifact_error', 'artifact')
    require('canonical_sha256' not in row, 'failed hash published')
    original_stat = Path.stat

    def broken_stat(path, *positional, **keywords):
        if path.name == 'ok_k5_r0.bin':
            raise OSError('injected artifact stat failure')
        return original_stat(path, *positional, **keywords)

    with patch.object(Path, 'stat', broken_stat):
        row = attempt('ok')
    check_attempt(row, 'artifact_error', 'artifact')
    require('canonical_sha256' in row and 'canonical_bytes' not in row, 'completed hash lost after stat failure')
    with patch.object(Path, 'unlink', side_effect=OSError('injected cleanup failure')):
        row = attempt('ok')
        broken = attempt('malformed')
    check_attempt(row, 'artifact_error', 'cleanup')
    require('canonical_sha256' in row, 'completed hash lost after cleanup failure')
    check_attempt(broken, 'invalid_output', 'cleanup')
    require([e['stage'] for e in broken['errors']] == ['events', 'cleanup'], 'first failure overwritten')
    return calls


def persisted_failures(root, executable):
    args = make_args(root, executable)
    args.data.mkdir()
    coordinates, ids = args.data / 'malformed', args.data / 'ids'
    coordinates.write_bytes(bytes(36))
    ids.write_bytes(bytes(12))
    cases = [{'name': 'case%d' % i, 'coordinates': coordinates.name, 'point_ids': ids.name, 'count': 3,
              'sha256': BENCH.digest(coordinates), 'ids_sha256': BENCH.digest(ids)} for i in range(6)]
    (args.data / 'manifest.json').write_text(json.dumps({'cases': cases}))
    args.qualification.write_text(json.dumps({'conforming': True, 'exit_code': 0}))
    provenance = args.qualification.parent / 'gcc_release'
    provenance.mkdir()
    (provenance / 'build_provenance.json').write_text(json.dumps({
        'complete': True, 'files': [{'path': executable.name, 'sha256': BENCH.digest(executable)}]}))
    with contextlib.redirect_stdout(io.StringIO()):
        result = BENCH.run(args)
    report = json.loads((args.out / 'catalogue.json').read_text())
    require(result == 1 and report['complete'] is True, 'failed schedule must finish with code1')
    require(report['attempt_schema'] == 'ehgp.v11.catalogue_attempt.v2' and report['leaf_size'] == 16,
            'attempt schema/leaf-size not persisted')
    require(report['all_attempted_ok'] is False and report['full_schedule_completed'] is False,
            'failed attempts promoted to conformity')
    require(len(report['runs']) == 6 and len(report['not_run']) == 30, 'failed attempt rerun or lost')
    for row in report['runs']:
        check_attempt(row, 'invalid_output', 'events')
        require(row['repetition'] == 0 and row['kmax'] == 5, 'failure was retried')
        require('stderr:malformed' in row['stderr'] and '{"phase":\n' in row['stdout'], 'persisted streams lost')


def cli_leaf_bounds():
    for leaf in (12, 257):
        argv = [str(SOURCE), '--exe', 'x', '--data', 'x', '--out', 'x', '--work', 'x',
                '--qualification', 'x', '--leaf-size', str(leaf)]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stderr(io.StringIO()):
            try:
                BENCH.main()
            except SystemExit as error:
                require(error.code == 2, 'invalid leaf-size did not return argparse code2')
            else:
                raise ValueError('invalid leaf-size accepted')
    for leaf in (None, 13, 256):
        argv = [str(SOURCE), '--exe', 'x', '--data', 'x', '--out', 'x', '--work', 'x', '--qualification', 'x']
        if leaf is not None:
            argv += ['--leaf-size', str(leaf)]
        with patch.object(sys, 'argv', argv), patch.object(BENCH, 'run', return_value=17) as run:
            require(BENCH.main() == 17, 'valid CLI options rejected')
            require(run.call_args.args[0].leaf_size == (32 if leaf is None else leaf), 'CLI default/bound changed')


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11_catalogue_collector_') as temporary:
        root = Path(temporary)
        executable = root / 'fake_bench'
        executable.write_text('#!' + sys.executable + '\n' + CHILD)
        executable.chmod(0o700)
        attempt_root, persist_root = root / 'attempts', root / 'persist'
        attempt_root.mkdir()
        persist_root.mkdir()
        fixtures = attempt_cases(attempt_root, executable)
        persisted_failures(persist_root, executable)
        cli_leaf_bounds()
    require(fixtures == 20 and CHECKS >= 190, 'collector non-vacuity floor')
    print(json.dumps({'status': 'ok', 'attempt_fixtures': fixtures, 'persisted_failures': 6,
                      'checks': CHECKS, 'native_calls': 0}, sort_keys=True))
    print('catalogue_collector_verdict conforme attempts20 persisted6 native0')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
