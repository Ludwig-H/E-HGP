#!/usr/bin/env python3
"""Vertical FULL collection: real tiny artifacts, mocked children, no native execution."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'bench'))
import full_parallel as driver
import full_parallel_collector_test as fixtures
from full_campaign_test import arguments
from full_bench_semantic_test import encode

full = driver.full
CHECKS = 0
COUNTS = dict(attempts=0, corruptions=0, decodes=0, schedules=0, interruptions=0, comparisons=0)
ORIGINAL_STREAM = fixtures.stream


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def stream(req):
    result = ORIGINAL_STREAM(req)
    # Exact disjoint boundary in integer ns. Different paid work between routes is legitimate.
    if req['optimizations'] & 128 and req['optimizations'] & 4:
        for k, order in enumerate(result[2]['orders'], 1):
            if k == 1:
                continue
            work = order['work']; queries = work['memo_queries']; steps = queries - 1
            work.update(memo_hits=1, memo_misses=steps, memo_insertions=steps, descent_steps=steps,
                        part_meb_presentations=steps, part_diameter_pairs=steps*k*(k-1)//2)
            fixtures.sources(work,k,req['optimizations'])
    return result


def vertical(events, k=2):
    return events[2]['orders'][k-1]['vertical_parallel']


def corruptions():
    result = {
        'metadata_missing': lambda e: e[2].pop('parallel_verticals'),
        'metadata_integer': lambda e: e[2].update(parallel_verticals=1),
        'metadata_false': lambda e: e[2].update(parallel_verticals=False),
        'metadata_null': lambda e: e[2].update(parallel_verticals=None),
        'order_missing': lambda e: e[2]['orders'][1].pop('vertical_parallel'),
        'extra_field': lambda e: vertical(e).update(alien=0),
        'resolutions_short': lambda e: vertical(e).update(vertical_resolutions=3),
        'batches_missing': lambda e: vertical(e).update(vertical_batches=0),
        'batches_excess': lambda e: vertical(e).update(vertical_batches=2),
        'maximum_short': lambda e: vertical(e).update(max_vertical_batch=3),
        'maximum_excess': lambda e: vertical(e).update(max_vertical_batch=5),
        'maximum_above_dispatch': lambda e: vertical(e).update(vertical_task_max_ns=7),
        'maximum_above_sum': lambda e: vertical(e).update(vertical_task_sum_ns=3),
        'lane_capacity': lambda e: vertical(e).update(vertical_task_sum_ns=48*6+1),
        'wall_one_ns': lambda e: vertical(e).update(vertical_sweep_ns=5),
        'k1_work': lambda e: vertical(e, 1).update(vertical_resolutions=1),
        'k1_time': lambda e: vertical(e, 1).update(vertical_sweep_ns=1),
    }
    for field in sorted(full.vertical.FIELDS):
        result['missing_'+field] = lambda e, f=field: vertical(e).pop(f)
        result['bool_'+field] = lambda e, f=field: vertical(e).update({f: True})
        result['negative_'+field] = lambda e, f=field: vertical(e).update({f: -1})
        result['overflow_'+field] = lambda e, f=field: vertical(e).update({f: 2**64})
    return result


def bounded_windows():
    # Pure arithmetic fixtures for a boundary spanning two windows; not a geometric payload.
    for births in (1, 4095, 4096, 4097, 8192, 8193):
        for workers in (1, 8, 48):
            value = stream(fixtures.request(mode=255, workers=workers))[2]
            order = value['orders'][1]; order['births'] = births
            rows = order['vertical_parallel']; batches = (births+4095)//4096
            rows.update(vertical_resolutions=births, vertical_batches=batches, max_vertical_batch=min(births,4096),
                        vertical_dispatch_ns=10*batches, vertical_task_sum_ns=10*batches*workers,
                        vertical_task_max_ns=10, vertical_sweep_ns=7)
            order['timings']['verticals_ns'] = 10*batches+7
            full.vertical.validate(value, need, full.unsigned)
            broken = copy.deepcopy(value)
            broken['orders'][1]['vertical_parallel']['vertical_batches'] -= 1
            try:
                full.vertical.validate(broken, full.need, full.unsigned)
            except ValueError:
                COUNTS['corruptions'] += 1
            else:
                raise ValueError('missing window accepted')


def attempts(root):
    harness = fixtures.Attempts(root)
    def run(mode=255, workers=48, bits=21, **options):
        return harness.run(fixtures.request(mode=mode, workers=workers, bits=bits), **options)
    for mode in (127, 255, 136):
        for bits in (21, 24):
            for workers in (1, 8, 48):
                row = run(mode, workers, bits)
                need(row['status'] == 'ok' and row['semantic']['kmax'] == 5, 'real FULL payload')
                need(row['events'][2]['parallel_verticals'] is bool(mode & 128), 'exact requested route')
                if mode & 128:
                    order = row['events'][2]['orders'][1]; value = order['vertical_parallel']
                    need(value['vertical_dispatch_ns'] + value['vertical_sweep_ns'] == order['timings']['verticals_ns'],
                         'integer boundary admitted, no float sum gate')
    for workers in (1, 8, 48):
        row = run(workers=workers, mutation=lambda e, w=workers: vertical(e).update(
            vertical_task_max_ns=6, vertical_task_sum_ns=6*min(w,4)))
        need(row['status'] == 'ok', 'maximum equals total dispatch without subtracting task sum')
    row = run(mutation=lambda e: vertical(e).update(dict.fromkeys(full.vertical.TIMES,0)))
    need(row['status'] == 'ok', 'zero clock intervals allowed with nonzero work')
    for process in ('timeout', 'launch'):
        row = run(process=process)
        need(row['status'] == ('launch_error' if process == 'launch' else process) and 'semantic' not in row,
             'first process failure retained')
    row = run(127, mutation=lambda e: e[2].update(parallel_verticals=True))
    need(row['status'] == 'invalid_output', 'unrequested vertical route refused')
    row = run(mutation=lambda e: e[2]['parallel'].update(lane_memo_reserved_bytes=1))
    need(row['status'] == 'invalid_output', 'vertical route reuses exact existing lane reservations')
    for name, mutate in corruptions().items():
        row = run(mutation=mutate)
        need(row['status'] == 'invalid_output' and 'semantic' not in row and row['stdout'], name)
        COUNTS['corruptions'] += 1
    for field in sorted(full.vertical.FIELDS):
        row = run(127, mutation=lambda e, f=field: vertical(e).update({f: 1}))
        need(row['status'] == 'invalid_output', 'disabled field '+field)
        COUNTS['corruptions'] += 1
    row = run(workers=1, mutation=lambda e: vertical(e).update(vertical_task_sum_ns=7))
    need(row['status'] == 'invalid_output', 'W1 temporal capacity stricter than L48')
    COUNTS['corruptions'] += 1
    cache = full.reuse.SummaryCache()
    first = run(127, cache=cache); previous = copy.deepcopy(first)
    before = fixtures.COUNTS['decodes']; second = run(255, cache=cache)
    need(second['status'] == 'ok' and second['semantic_reuse']['mode'] == 'reused' and
         fixtures.COUNTS['decodes'] == before, 'raw identity reused across vertical routes')
    for change in (corruptions()['wall_one_ns'], corruptions()['resolutions_short']):
        row = run(cache=cache, mutation=change)
        need(row['status'] == 'invalid_output' and 'semantic_reuse' not in row and len(cache) == 1,
             'current vertical diagnostics checked before cached summary')
    wrong = run(cache=cache, mutation=lambda e: e[2]['orders'][0].update(nodes=8))
    need(wrong['status'] == 'invalid_output' and wrong['semantic_reuse']['mode'] == 'reused',
         'current structural counts checked after reuse')
    refused = run(cache=cache, process='refused')
    need(refused['status'] == 'refused' and 'semantic' not in refused and first == previous and len(cache) == 1,
         'failed child does not inherit prior result')
    changed = run(cache=cache, raw_suffix=b'changed')
    need(changed['status'] == 'invalid_output' and len(cache) == 1, 'changed raw bytes rejudged')


def calendar():
    names = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
    def row(name, bits, mode, workers=48):
        return dict(case=name, coord_bits=bits, kmax=5, workers=workers, repetition=0, optimizations=mode)
    expected = [row(name,bits,mode) for name in names for bits in (21,24) for mode in (127,255)]
    expected += [row(names[0],21,255,w) for w in (1,8)]
    expected += [row(name,21,mode) for name in ('uniform_u18_n8000','uniform_u18_n16000','uniform_u18_n32000')
                 for mode in (127,255)]
    need(driver.schedule(parallel_verticals=True) == expected and len(set(map(full.identity,expected))) == 20,
         'independent requested calendar20')
    need(len(driver.schedule()) == 19 and len(driver.schedule(True)) == 27, 'previous calendars retained')
    for a, b in ((True,True),(False,1),(False,None),(False,'true')):
        try:
            driver.schedule(a,b)
        except ValueError:
            need(True, 'mutually exclusive strict calendar options')
        else:
            raise ValueError('invalid calendar accepted')
    return expected


def campaign(root, scenario):
    COUNTS['schedules'] += 1
    args = arguments(root, 'campaign_'+scenario)
    args.budget_seconds=570; args.reuse_semantic=True; args.parallel_verticals=True
    manifest = dict(cases=[dict(fixtures.CASE,name=name) for name in driver.profiles.COUNTS])
    original_measure, original_decode = full.measure, full.semantic.decode
    launched = []; path = args.out/'full_parallel.json'
    def measure(exe, case, req, call_args, checkpoint, **options):
        saved = json.loads(path.read_text())
        need(len(saved['launch_intents']) == len(launched)+1 and len(saved['runs']) == len(launched), 'intent first')
        launched.append(req)
        if scenario == 'missing_checkpoint':
            return dict(req,status='failed')
        def child(argv, **kwargs):
            COUNTS['attempts'] += 1
            need(kwargs['timeout'] == 60 and argv[10:] == [str(req['workers']),str(req['optimizations'])], 'actual argv')
            if scenario == 'interrupt_before':
                raise KeyboardInterrupt
            if scenario == 'failure' and len(launched) == 1:
                raise subprocess.TimeoutExpired(argv,60,output=b'preserved partial process')
            Path(argv[3]).write_bytes(encode(fixtures.VALUE,req['coord_bits'])[0])
            return subprocess.CompletedProcess(argv,0,fixtures.wire(stream(req)),b'')
        with patch.object(full.subprocess,'run',side_effect=child):
            return original_measure(exe,case,req,call_args,checkpoint,**options)
    def decode(*parameters):
        COUNTS['decodes'] += 1
        if scenario == 'interrupt_after':
            raise KeyboardInterrupt
        return original_decode(*parameters)
    with contextlib.ExitStack() as stack:
        stack.enter_context(patch.object(driver.profiles,'checked_builds',return_value={
            bits:dict(path='fake%d'%bits) for bits in (21,24)}))
        stack.enter_context(patch.object(driver.profiles,'checked_supplement',return_value='c'*64))
        stack.enter_context(patch.object(driver.profiles,'inputs',return_value=(manifest,'a'*64)))
        stack.enter_context(patch.object(driver.base,'digest',return_value='b'*64))
        stack.enter_context(patch.object(full,'measure',side_effect=measure))
        stack.enter_context(patch.object(full.semantic,'decode',side_effect=decode))
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        if scenario.startswith('budget'):
            ticks = [0]
            def clock():
                ticks[0] += 1
                return 100000 if (scenario == 'budget_all' and ticks[0] > 1 or len(launched) >= 3) else 0
            stack.enter_context(patch.object(driver.time,'monotonic',side_effect=clock))
        try:
            code = driver.run(args)
        except KeyboardInterrupt:
            need(scenario.startswith('interrupt'), 'deliberate interruption')
            COUNTS['interruptions'] += 1; code = None
        except ValueError:
            need(scenario == 'missing_checkpoint', 'deliberate lost checkpoint'); code = None
    report = json.loads(path.read_text())
    need(report['schema'] == driver.SCHEMA and report['parallel_verticals'] is True and
         report['optimized_catalogue'] is False and report['descent_work_mask'] == 1423 and report['requested_runs'] == 20 and
         report['requested'] == driver.schedule(parallel_verticals=True), 'persisted route and calendar')
    if code is None:
        after = scenario == 'interrupt_after'
        need(not report['complete'] and not report['conforming'] and len(report['launch_intents']) == 1 and
             len(report['runs']) == int(after) and not report['not_run'], 'no fabricated completion or attempt')
        if after:
            row = report['runs'][0]
            need(row['status'] == 'pending_semantic' and row['stdout'] and 'semantic' not in row and
                 Path(row['argv'][3]).is_file(), 'process evidence retained before decode interruption')
        return report
    need(report['complete'] and report['conforming'] is (scenario == 'ok') and code == int(scenario != 'ok'), 'verdict')
    actual = [full.identity(r) for field in ('runs','not_run') for r in report[field]]
    need(len(actual) == len(set(actual)) == 20 and set(actual) == set(map(full.identity,report['requested'])), 'partition')
    if scenario.startswith('budget'):
        n = 0 if scenario == 'budget_all' else 3
        need(len(report['runs']) == n and len(report['not_run']) == 20-n and
             all(r['reason'] == 'campaign_budget_before_launch' for r in report['not_run']), 'deadline-only omissions')
    else:
        need(len(report['runs']) == 20 and not report['not_run'], 'no mode suppressed')
    if scenario == 'failure':
        need(report['runs'][0]['status'] == 'timeout' and report['runs'][1]['status'] == 'ok', '255 attempted after127failure')
    return report


def comparison_gates(report):
    scenarios = ('ok','paid255','paid127','invariant','semantic','raw','verticals','lanes',
                 'incomplete','incomplete_different')
    for scenario in scenarios:
        rows = copy.deepcopy(report['runs'])
        target = next(r for r in rows if r['case']=='lidar_ng00' and r['coord_bits']==21 and r['workers']==48 and
                      r['optimizations']==(127 if scenario=='paid127' else 255))
        if scenario.startswith('paid'): target['events'][2]['orders'][1]['work']['census_point_tests'] += 1
        if scenario=='invariant': target['events'][2]['orders'][0]['work']['cells'] += 1
        if scenario in ('semantic','incomplete_different'): target['semantic']['sha256']='b'*64
        if scenario=='raw': target['semantic']['raw_sha256']='c'*64
        if scenario=='verticals': target['events'][2]['orders'][1]['vertical_parallel']['vertical_batches'] += 1
        if scenario=='lanes': target['events'][2]['orders'][0]['parallel']['regular_batches'] += 1
        if scenario.startswith('incomplete'): rows[0]['status']='timeout'
        for row in rows: row['stdout']=fixtures.wire(row['events']).decode()
        group = next(g for g in driver.comparisons(rows,report['requested']) if g['case']=='lidar_ng00')
        expected = 'equal' if scenario=='ok' else 'incomplete' if scenario=='incomplete' else 'different'
        need(group['status']==expected, 'comparison '+scenario)
        if scenario=='ok':
            a,b=report['runs'][:2]
            need(a['events'][2]['orders'][1]['work'] != b['events'][2]['orders'][1]['work'] and
                 group['lane_work_equal'], 'paid work differs across routes but equals within each route')
        if scenario.startswith('paid'):
            need(group['invariant_work_equal'] and not group['lane_work_equal'], 'paid comparison includes128')
        COUNTS['comparisons'] += 1


def main():
    calendar(); bounded_windows()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-vertical-') as folder:
        root = Path(folder)
        with patch.object(fixtures,'stream',side_effect=stream):
            attempts(root)
        comparison_gates(campaign(root,'ok'))
        for scenario in ('failure','budget_all','budget_tail','interrupt_before','interrupt_after','missing_checkpoint'):
            campaign(root,scenario)
    COUNTS['attempts'] += fixtures.COUNTS['attempts']; COUNTS['decodes'] += fixtures.COUNTS['decodes']
    need(COUNTS['attempts'] >= 130 and COUNTS['corruptions'] >= 65 and COUNTS['decodes'] >= 30 and
         COUNTS['schedules']==7 and COUNTS['interruptions']==2 and COUNTS['comparisons']==10, 'coverage floors')
    print('full_vertical_collector_verdict conforme '+' '.join('%s%d'%item for item in COUNTS.items())+
          ' checks%d native0'%CHECKS)


if __name__ == '__main__':
    main()
