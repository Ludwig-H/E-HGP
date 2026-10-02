"""Controles du lecteur de preuve seulement : JSON synthetique, aucun calcul natif, CTest ou GCP.

Les captures LIVE catalogue1/2 restent des echecs. Les temoins fabriques couvrent calendriers, erreurs de
collecte et deux versions du protocole ; ils ne constituent aucune campagne ni mesure produit.
"""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
from unittest import mock


def main():
    root = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('catalogue_reader', root / 'check.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    manifest = reader.js((root / 'catalogue1/inputs.json').read_bytes())
    manifest_hash = reader.sha((root / 'catalogue1/inputs.json').read_bytes())
    source = b'{"synthetic_qualification_fixture":true}\n'
    executable_hash = 'a' * 64
    builds = {'gcc_release': {'mhgp11_catalogue_bench': {'sha256': executable_hash}}}
    base = {'schema': 'ehgp.v11.catalogue_benchmark.v1', 'manifest': manifest, 'manifest_sha256': manifest_hash,
            'qualification_sha256': reader.sha(source), 'executable_sha256': executable_hash,
            'full_contract': 'not_testable_missing_tower', 'requested_runs': 36, 'repetitions_requested': 3,
            'runs': [], 'not_run': [], 'complete': True, 'all_attempted_ok': True, 'full_schedule_completed': True}
    cases = sorted(manifest['cases'], key=lambda c: (c['name'].startswith('lidar'), c['count']))
    for case in cases:
        for k in (5, 10):
            for repetition in range(3):
                events = [dict(phase='cloud', points=case['count'], sites=case['count'], read_ns=1, cloud_ns=2,
                               cloud_peak_bytes=64),
                          dict(phase='catalogue', status='ok', reason='none', coord_bits=18, kmax=k, balls=1,
                               generation_passes=2, wall_ns=1000, peak_reserved_bytes=128, reserved_after_bytes=80),
                          dict(phase='exit', status='ok', reason='none')]
                base['runs'].append(dict(case=case['name'], kmax=k, repetition=repetition, whole_input=True,
                    count=case['count'], argv=['/build/mhgp11_catalogue_bench', '/data/' + case['coordinates'],
                    '/data/' + case['point_ids'], '/out/result.bin', str(k), '32', '256', '0', str(2**32 - 1),
                    str(8 * 1024**3)], process_wall_seconds=1, stdout='\n'.join(map(json.dumps, events)), events=events,
                    status='ok', exit_code=0, canonical_sha256='b' * 64, canonical_bytes=200,
                    catalogue_ms=0.001, cloud_ms=0.000002, read_ms=0.000001, catalogue_within_100ms=True))

    def judge(report):
        return reader.benchmark({reader.BASE + 'summary.json': source}, root, report, manifest, manifest_hash,
                                builds, True)

    if judge(base) != dict(attempted=36, ok=36, omitted=0, complete=True, schedule_ok=True):
        raise RuntimeError('positive full schedule fixture')
    slow = copy.deepcopy(base)
    slow['runs'] = [row for row in slow['runs'] if row['repetition'] == 0]
    for row in slow['runs']:
        row['process_wall_seconds'] = 11
        slow['not_run'].extend(dict(case=row['case'], kmax=row['kmax'], repetition=r, reason='duration') for r in (1, 2))
    slow['full_schedule_completed'] = False
    if judge(slow) != dict(attempted=12, ok=12, omitted=24, complete=True, schedule_ok=False):
        raise RuntimeError('positive slow schedule fixture')
    failed = copy.deepcopy(base)
    failed['runs'] = [row for row in failed['runs'] if row['kmax'] == 5 and row['repetition'] == 0]
    for row in failed['runs']:
        for key in ('canonical_sha256', 'canonical_bytes'):
            del row[key]
        row.update(status='timeout', exit_code=None, stdout='', events=[])
        failed['not_run'].extend(dict(case=row['case'], kmax=k, repetition=r, reason='previous failure')
                                 for k in (5, 10) for r in range(3) if (k, r) != (5, 0))
    failed.update(all_attempted_ok=False, full_schedule_completed=False)
    if judge(failed) != dict(attempted=6, ok=0, omitted=30, complete=True, schedule_ok=False):
        raise RuntimeError('positive failed schedule fixture')
    refused = []

    def corrupt(name, mutate, template=base):
        report = copy.deepcopy(template)
        mutate(report)
        try:
            judge(report)
        except reader.foundation.Refusal:
            refused.append(name)
        else:
            raise RuntimeError('corruption accepted: ' + name)

    corrupt('missing_run', lambda r: r['runs'].pop())
    corrupt('duplicate_run', lambda r: r['runs'].append(copy.deepcopy(r['runs'][0])))
    corrupt('unknown_case', lambda r: r['runs'][0].__setitem__('case', 'unknown'))
    corrupt('wrong_executable_hash', lambda r: r.__setitem__('executable_sha256', 'c' * 64))
    corrupt('wrong_manifest_hash', lambda r: r.__setitem__('manifest_sha256', 'c' * 64))
    corrupt('wrong_qualification_hash', lambda r: r.__setitem__('qualification_sha256', 'c' * 64))
    corrupt('wrong_native_K', lambda r: r['runs'][0]['argv'].__setitem__(4, '1'))
    corrupt('wrong_events', lambda r: r['runs'][0]['events'][0].__setitem__('points', 1))
    corrupt('changed_repeat_hash', lambda r: r['runs'][1].__setitem__('canonical_sha256', 'c' * 64))
    corrupt('signal_green', lambda r: r['runs'][0].__setitem__('exit_code', -15))
    corrupt('nonfinite_duration', lambda r: r['runs'][0].__setitem__('process_wall_seconds', float('nan')))
    corrupt('false_summary', lambda r: r.__setitem__('all_attempted_ok', False))

    def unjustified(report):
        row = report['runs'].pop()
        report['not_run'].append(dict(case=row['case'], kmax=row['kmax'], repetition=row['repetition']))
        report['full_schedule_completed'] = False
    corrupt('unjustified_omission', unjustified)
    v2 = copy.deepcopy(base)
    v2.update(attempt_schema='ehgp.v11.catalogue_attempt.v2', leaf_size=16)
    for row in v2['runs']:
        row['argv'][5] = '16'
        row.update(errors=[], stderr='')
    if judge(v2)['schedule_ok'] is not True:
        raise RuntimeError('positive v2 leaf16 fixture')
    mixed = copy.deepcopy(failed)
    mixed.update(attempt_schema=v2['attempt_schema'], leaf_size=16)
    states = ['launch_error', 'invalid_output', 'artifact_error', 'timeout', 'refused', 'failed']
    for row, state in zip(mixed['runs'], states):
        original = next(r for r in v2['runs'] if reader.unit(r) == reader.unit(row))
        row.update(copy.deepcopy(original))
        row['status'] = state
        if state != 'artifact_error':
            for key in ('canonical_sha256', 'canonical_bytes', 'catalogue_ms', 'cloud_ms', 'read_ms',
                        'catalogue_within_100ms'):
                row.pop(key, None)
        if state == 'launch_error':
            row.update(exit_code=None, stdout='', events=[], errors=[dict(stage='launch', type='OSError', message='x')])
        elif state in ('invalid_output', 'timeout'):
            row['events'] = [row['events'][0]]
            row['stdout'] = json.dumps(row['events'][0]) + '\n{broken'
            row['exit_code'] = 0 if state == 'invalid_output' else None
            row['errors'] = [dict(stage='events', type='ValueError', message='x')]
            if state == 'timeout':
                row['errors'].insert(0, dict(stage='process', type='TimeoutExpired', message='x'))
        elif state == 'artifact_error':
            row['errors'] = [dict(stage='cleanup', type='OSError', message='x')]
        elif state == 'refused':
            row['events'] = [dict(phase='exit', status='resource_exhausted', reason='memory_budget')]
            row.update(exit_code=2, stdout=json.dumps(row['events'][0]))
        else:
            row.update(exit_code=-15, stdout='', events=[])
    if judge(mixed) != dict(attempted=6, ok=0, omitted=30, complete=True, schedule_ok=False):
        raise RuntimeError('positive v2 retained failures fixture')
    corrupt('v2_missing_marker', lambda r: r.pop('attempt_schema'), v2)
    corrupt('v2_missing_leaf', lambda r: r.pop('leaf_size'), v2)
    corrupt('v2_leaf_too_small', lambda r: r.__setitem__('leaf_size', 12), v2)
    corrupt('v2_leaf_too_large', lambda r: r.__setitem__('leaf_size', 257), v2)
    corrupt('v2_leaf_argv', lambda r: r['runs'][0]['argv'].__setitem__(5, '32'), v2)
    corrupt('v2_error_green', lambda r: r['runs'][0]['errors'].append(
        dict(stage='cleanup', type='OSError', message='x')), v2)
    corrupt('v2_error_event_count', lambda r: r['runs'][1].__setitem__('errors', []), mixed)
    corrupt('v2_launch_with_code0', lambda r: r['runs'][0].__setitem__('exit_code', 0), mixed)
    def partial_green(report):
        report.update(complete=False, runs=[], not_run=[], full_schedule_completed=True)
    corrupt('partial_schedule_green', partial_green, v2)
    partial = copy.deepcopy(v2)
    partial.update(complete=False, runs=[], not_run=[], full_schedule_completed=False)
    partial_bytes = json.dumps(partial).encode()
    data = {reader.BASE + 'summary.json': source, reader.BENCH + 'files/catalogue.json': partial_bytes,
            'results/cmd/000_matrice/meta.txt': b'exit_code=0\nstatus=ok\ngroup_closed=1\n',
            reader.BENCH + 'meta.txt': b'exit_code=1\nstatus=failed\ngroup_closed=1\n'}
    capture = ({'status': 'failed_remote', 'worker_exit_code': 1, 'commit': 'f' * 40},
               {'commands_total': '2', 'commands_ok': '1', 'status': 'failed'}, data)
    with mock.patch.object(reader, 'read_capture', return_value=capture), \
            mock.patch.object(reader, 'matrix', return_value=({'statuses': {}}, {}, builds, 0)), \
            mock.patch.object(reader, 'inputs', return_value=(manifest, manifest_hash)), \
            mock.patch.object(Path, 'read_bytes', return_value=partial_bytes), \
            contextlib.redirect_stdout(io.StringIO()):
        if reader.check(root / '__in_memory_partial') is not True:
            raise RuntimeError('partial failed capture witness')
        data[reader.BENCH + 'meta.txt'] = b'exit_code=0\nstatus=failed\ngroup_closed=1\n'
        try:
            reader.check(root / '__in_memory_partial')
        except reader.foundation.Refusal:
            refused.append('partial_meta_code0')
        else:
            raise RuntimeError('partial command code0 accepted')
    with contextlib.redirect_stdout(io.StringIO()):
        for name in ('catalogue1', 'catalogue2'):
            if reader.check(root / name) is not True:
                raise RuntimeError('historical campaign failure was promoted')
    print(json.dumps({'live_failures_preserved': 2, 'synthetic_positive_fixtures': 6,
                      'corruptions_refused': len(refused), 'cases': refused}, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
