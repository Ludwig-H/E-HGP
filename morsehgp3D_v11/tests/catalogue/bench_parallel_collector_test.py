#!/usr/bin/env python3
"""Collecteur parallele : processus factices, vrais measure/check_parallel/comparisons/run ; aucun natif."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_semantic_test import fixture, need
from bench_profiles_collector_test import events, row as profile_row
import catalogue_parallel as driver


def native_events(bits, workers):
    values = events(bits)
    values[1].update(workers=workers, pool_ns=40, timings=dict.fromkeys(driver.TIMING_FIELDS, 1))
    return values


def ready(request):
    result = profile_row(request['case'], request['coord_bits'], request['kmax'])
    result.update(request, canonical_sha256=str(request['coord_bits']) * 32,
                  cloud_ms=0.000020, catalogue_ms=0.000030)
    result['events'] = native_events(request['coord_bits'], request['workers'])
    result['events'][1]['kmax'] = request['kmax']
    return result


def attempts(root):
    args = argparse.Namespace(work=root, data=root)
    case = dict(name='fake', coordinates='xyz', point_ids='ids', count=4)
    modes = ('ok', 'other_profile', 'wall_boundary', 'workers_wrong', 'workers_bool', 'workers_missing',
             'pool_negative', 'pool_bool', 'pool_string', 'pool_missing', 'pool_overflow',
             'timings_missing', 'timings_bool', 'timings_negative', 'timings_extra', 'timings_field_missing',
             'timings_tasks_zero', 'timings_tasks_big', 'timings_sum', 'timings_max', 'timings_wall',
             'timings_sort_zero', 'timings_sort_big', 'balls',
             'incidences', 'bad_json', 'bad_canonical', 'timeout', 'launch', 'refused', 'failed')
    calls = 0
    for mode in modes:
        bits, workers = (24, 8) if mode == 'other_profile' else (21, 48)
        request = dict(case='fake', coord_bits=bits, kmax=5, workers=workers, repetition=2)

        def child(argv, **kwargs):
            nonlocal calls
            calls += 1
            need(len(argv) == 11 and argv[4:6] == ['5', '16'] and argv[-1] == str(workers), 'worker argv')
            need(argv[3].endswith('_b%d_k5_w%d_r2.bin' % (bits, workers)), 'repetition output path')
            need(kwargs['timeout'] == 15 and kwargs['check'] is False and kwargs['stdin'] == subprocess.DEVNULL,
                 'bounded process invocation')
            if mode == 'timeout':
                raise subprocess.TimeoutExpired(argv, 15, output=b'{"phase":', stderr=b'partial')
            if mode == 'launch':
                raise OSError('fake launch error')
            values = native_events(bits, workers)
            event = values[1]
            if mode == 'wall_boundary':
                event['wall_ns'] = 200_000_000
            if mode.startswith('workers_'):
                if mode == 'workers_missing': del event['workers']
                else: event['workers'] = True if mode == 'workers_bool' else 1
            if mode.startswith('pool_'):
                if mode == 'pool_missing': del event['pool_ns']
                else: event['pool_ns'] = {'pool_negative': -1, 'pool_bool': True,
                                          'pool_string': '40', 'pool_overflow': 2**64}[mode]
            if mode.startswith('timings_'):
                t = event['timings']
                if mode == 'timings_missing': del event['timings']
                elif mode == 'timings_extra': t['extra'] = 0
                elif mode == 'timings_field_missing': del t['sort_ns']
                else:
                    field, value = {'timings_bool': ('fill_ns', True), 'timings_negative': ('count_ns', -1),
                                    'timings_tasks_zero': ('tasks', 0), 'timings_tasks_big': ('tasks', 257),
                                    'timings_sum': ('count_task_sum_ns', 2), 'timings_max': ('fill_task_max_ns', 2),
                                    'timings_wall': ('sort_ns', 30), 'timings_sort_zero': ('sort_comparisons', 0),
                                    'timings_sort_big': ('sort_comparisons', 1000)}[mode]
                    t[field] = value
            if mode in ('balls', 'incidences'):
                event[mode] += 1
            payload = '\n'.join(json.dumps(v) for v in values).encode()
            if mode == 'bad_json': payload += b'\n{"x":1,"x":2}'
            if mode not in ('refused', 'failed'):
                data, _ = fixture(bits)
                Path(argv[3]).write_bytes(data[:-1] if mode == 'bad_canonical' else data)
            return subprocess.CompletedProcess(argv, {'refused': 2, 'failed': 3}.get(mode, 0), payload, b'')

        checkpoints = []
        with patch.object(driver.profiles.subprocess, 'run', side_effect=child):
            actual = driver.profiles.measure(root/'fake_binary', case, bits, 5, args,
                lambda value: checkpoints.append(copy.deepcopy(value)), workers=workers, repetition=2, timeout=15)
        driver.check_parallel(actual, request)
        wanted = ('invalid_output' if mode.startswith(('pool_', 'workers_', 'timings_')) else
                  {'bad_json': 'invalid_output', 'bad_canonical': 'artifact_error', 'balls': 'artifact_error',
                   'incidences': 'artifact_error', 'timeout': 'timeout', 'launch': 'launch_error',
                   'refused': 'refused', 'failed': 'failed'}.get(mode, 'ok'))
        need(actual['status'] == wanted, mode+': status')
        need(driver.identity(actual) == driver.identity(request), 'request identity preserved')
        need(len(checkpoints) == 1 and not Path(actual['argv'][3]).exists(), 'single checkpoint and cleanup')
        if wanted == 'ok':
            need(checkpoints[0]['status'] == 'pending_semantic', 'checkpoint before decoder')
            need(actual['catalogue_within_200ms'] is (mode != 'wall_boundary'), 'strict 200ms flag')
            need(actual['pool_ms'] == 40/1e6 and actual['cloud_pool_catalogue_ms'] ==
                 actual['cloud_ms']+actual['pool_ms']+actual['catalogue_ms'], 'timing scopes')
        if wanted in ('invalid_output', 'artifact_error', 'timeout', 'launch_error'):
            need(actual['errors'], 'structured failure kept')
        if mode.startswith(('pool_', 'workers_', 'timings_')):
            need('semantic' in actual and 'canonical_sha256' in actual, 'prior evidence kept on native metadata failure')
    need(calls == len(modes), 'attempt floor')
    return calls


def comparisons():
    requested = driver.schedule()
    rows = [ready(r) for r in requested]
    baseline = driver.comparisons(rows, requested)
    need(len(baseline) == 9 and all(r['status'] == 'equal' for r in baseline), 'positive cross-profile groups')
    mutations = ['semantic', 'raw', *sorted(driver.profiles.LOGICAL), 'q4_candidates', 'q4_levels']
    target = next(i for i, r in enumerate(rows) if r['case'] == 'lidar_ng00' and r['coord_bits'] == 21
                  and r['repetition'] == 1)
    for field in mutations:
        changed = copy.deepcopy(rows)
        row = changed[target]
        if field == 'semantic': row['semantic']['sha256'] = 'b'*64
        elif field == 'raw': row['canonical_sha256'] = 'b'*64
        elif field in driver.profiles.LOGICAL: row['events'][1]['logical'][field] += 1
        elif field == 'q4_candidates': row['events'][1]['work'][field] += 1
        else:
            row['events'][1]['work'].update(q4_candidates=2, q4_levels=2)
            row['qmin_counts']['4'] = 2; row['semantic']['balls'] = 12
        result = next(r for r in driver.comparisons(changed, requested) if (r['case'], r['kmax']) == ('lidar_ng00', 5))
        need(result['status'] == 'different', field+': divergence detected')
        changed = [r for r in changed if not (r['case'] == 'lidar_ng00' and r['coord_bits'] == 24)]
        result = next(r for r in driver.comparisons(changed, requested) if (r['case'], r['kmax']) == ('lidar_ng00', 5))
        need(result['status'] == 'different', field+': incompleteness cannot hide divergence')
    missing = driver.comparisons(rows[1:], requested)
    need(next(r for r in missing if (r['case'], r['kmax']) == ('lidar_ng00', 5))['status'] == 'incomplete',
         'missing first attempt stays incomplete')
    return len(mutations)


def environment(root, name):
    args = argparse.Namespace(out=root/name, work=root/(name+'_work'), data=root, builds=root/'builds',
                              qualification=root/'unused', supplement=root/'supplement', budget_seconds=750)
    manifest = dict(cases=[dict(name=n, count=4, coordinates='xyz', point_ids='ids', sha256='d'*64, ids_sha256='e'*64) for n in driver.profiles.COUNTS])
    builds = {bits: dict(path='fake%d' % bits) for bits in driver.profiles.PROFILES}
    return args, manifest, builds


@contextlib.contextmanager
def setup(manifest, builds):
    with patch.object(driver.profiles, 'checked_builds', return_value=builds), \
         patch.object(driver.profiles, 'checked_supplement', return_value='c'*64), \
         patch.object(driver.profiles, 'inputs', return_value=(manifest, 'a'*64)), \
         patch.object(driver.base, 'digest', return_value='b'*64), contextlib.redirect_stdout(io.StringIO()):
        yield


def schedule_case(root, mode):
    args, manifest, builds = environment(root, mode)
    elapsed, ticks = 0, 0

    def clock():
        nonlocal ticks
        ticks += 1
        return 800 if mode == 'early_deadline' and ticks > 1 else elapsed

    def measure(_exe, case, bits, kmax, _args, checkpoint, *, workers, repetition, timeout):
        nonlocal elapsed
        request = dict(case=case['name'], coord_bits=bits, kmax=kmax, workers=workers, repetition=repetition)
        result = ready(request)
        need(timeout == 15, 'per-attempt timeout')
        pending = copy.deepcopy(result); pending['status'] = 'pending_semantic'; checkpoint(pending)
        target = case['name'] == 'lidar_ng00' and bits == 21 and kmax == 5
        fail = ((mode in ('first_failed', 'incomplete_difference') and target and workers == 48 and repetition == 0) or
                (mode == 'repeat_failed' and target and repetition == 1) or
                (mode == 'w8_failed' and target and workers == 8) or
                (mode == 'synthetic_failed' and case['name'] == 'uniform_u18_n8000' and bits == 21))
        if fail: result['status'] = 'timeout'
        if case['name'] == 'lidar_ng00' and bits == 24 and kmax == 5 and repetition == 1:
            if mode in ('semantic_different', 'incomplete_difference'): result['semantic']['sha256'] = 'b'*64
            if mode == 'work_different': result['events'][1]['logical']['nodes'] += 1
            if mode == 'raw_different': result['canonical_sha256'] = 'b'*64
        if mode == 'deadline': elapsed += 100
        return result

    with setup(manifest, builds), patch.object(driver.profiles, 'measure', side_effect=measure), \
         patch.object(driver.time, 'monotonic', side_effect=clock):
        code = driver.run(args)
    report = json.loads((args.out/'parallel.json').read_text())
    need(code == (0 if mode == 'ok' else 1), mode+': conformity')
    inventory = [driver.identity(r) for r in report['runs']+report['not_run']]
    need(len(inventory) == len(set(inventory)) == 36 and set(inventory) == set(map(driver.identity, driver.schedule())),
         'every requested identity exactly once')
    need(list(map(driver.identity, report['launch_intents'])) == list(map(driver.identity, report['runs'])),
         'launch intentions cover completed attempts')
    need(report['native_schedule_bound_seconds'] == 540 and report['timeout_seconds'] == 15, 'honest native bound')
    need(report['complete'] is True and report['conforming'] is (mode == 'ok'), 'closed collection verdict')
    if mode in ('first_failed', 'incomplete_difference'):
        need(len(report['runs']) == 32 and len(report['not_run']) == 4, 'only causal four omissions')
        need(all(r['case'] == 'lidar_ng00' and r['coord_bits'] == 21 and
                 r['reason'] == 'same_profile_K5_W48_first_attempt_failed' for r in report['not_run']), 'causal identity')
    elif mode in ('deadline', 'early_deadline'):
        need(len(report['runs']) == (8 if mode == 'deadline' else 0), 'deadline launch limit')
        need(all(r['reason'] == 'campaign_budget_before_launch' for r in report['not_run']), 'deadline omissions recorded')
    else:
        need(len(report['runs']) == 36 and not report['not_run'], 'later failure cannot suppress future work')
    if mode in ('semantic_different', 'work_different', 'raw_different', 'incomplete_difference'):
        group = next(c for c in report['comparisons'] if (c['case'], c['kmax']) == ('lidar_ng00', 5))
        need(group['status'] == 'different', 'causal group disagreement')


def schedules(root):
    requested = driver.schedule()
    need(len(requested) == 36 and all(r['coord_bits'] in (21,24) for r in requested), '36-profile schedule')
    need(all(r['case'].startswith('lidar') for r in requested[:6]), 'whole LiDAR comes first')
    need(len([r for r in requested if r['workers'] == 8]) == 6, 'W8 inventory')
    need(len([r for r in requested if r['kmax'] == 10]) == 6, 'K10 inventory')
    need(len([r for r in requested if r['repetition'] > 0]) == 12, 'fresh repetition inventory')
    modes = ('ok', 'first_failed', 'repeat_failed', 'w8_failed', 'synthetic_failed', 'deadline', 'early_deadline',
             'semantic_different', 'work_different', 'raw_different', 'incomplete_difference')
    for mode in modes:
        schedule_case(root, mode)
    return len(modes)


def interrupted_decoder(root):
    args, manifest, builds = environment(root, 'interrupted')

    def child(argv, **kwargs):
        need(argv[-1] == '48' and kwargs['timeout'] == 15, 'first launch identity')
        payload, _ = fixture(21)
        Path(argv[3]).write_bytes(payload)
        return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in native_events(21,48)).encode(), b'')

    with setup(manifest, builds), patch.object(driver.profiles.subprocess, 'run', side_effect=child), \
         patch.object(driver.semantic, 'inspect', side_effect=KeyboardInterrupt):
        try:
            driver.run(args)
        except KeyboardInterrupt:
            pass
        else:
            raise ValueError('decoder interruption ignored')
    report = json.loads((args.out/'parallel.json').read_text())
    need(report['complete'] is False and report['conforming'] is False, 'interrupted run cannot be conforming')
    need(len(report['runs']) == 1 and len(report['requested']) == 36, 'native attempt retained')
    row = report['runs'][0]
    need(row['status'] == 'pending_semantic' and row['exit_code'] == 0 and len(row['events']) == 3, 'pending proof retained')
    need('semantic' not in row and Path(row['argv'][3]).exists(), 'unfinished decoder keeps source artifact')
    need(driver.identity(row) == driver.identity(driver.schedule()[0]), 'checkpoint exact request')
    return 1


def interrupted_launch(root):
    args, manifest, builds = environment(root, 'interrupted_launch')

    def child(argv, **kwargs):
        report = json.loads((args.out/'parallel.json').read_text())
        need(not report['runs'] and len(report['launch_intents']) == 1, 'intent precedes spawn')
        intent = report['launch_intents'][0]
        need(intent['argv'] == argv and intent['input_sha256'] == 'd'*64 and intent['ids_sha256'] == 'e'*64,
             'intent command and input hashes')
        need(intent['coord_bits'] == 21 and intent['workers'] == 48 and intent['whole_input'] is True,
             'intent whole input and profile')
        raise KeyboardInterrupt

    with setup(manifest, builds), patch.object(driver.profiles.subprocess, 'run', side_effect=child):
        try:
            driver.run(args)
        except KeyboardInterrupt:
            pass
        else:
            raise ValueError('launch interruption ignored')
    report = json.loads((args.out/'parallel.json').read_text())
    need(not report['complete'] and not report['runs'] and len(report['launch_intents']) == 1,
         'unknown spawn preserved as intention only')
    return 1


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11_parallel_collector_') as directory:
        root = Path(directory)
        count = attempts(root)
        changed = comparisons()
        calendar = schedules(root)
        checkpoints = interrupted_decoder(root) + interrupted_launch(root)
    print('catalogue_parallel_collector_verdict conforme attempts%d comparisons%d schedules%d checkpoints%d native0' %
          (count, changed, calendar, checkpoints))


if __name__ == '__main__':
    main()
