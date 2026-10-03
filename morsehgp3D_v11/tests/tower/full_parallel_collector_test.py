#!/usr/bin/env python3
"""Regular-lane FULL collector: real tiny artifacts and decoding, simulated child processes only."""
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
from full_campaign_test import arguments, events
from full_bench_semantic_test import encode, fixture

full = driver.full
CHECKS = 0
COUNTS = dict(attempts=0, corruptions=0, decodes=0, schedules=0, interruptions=0, comparisons=0)
VALUE = fixture([(0, 0, 0), (2, 0, 0), (4, 0, 0), (6, 0, 0), (8, 0, 0)], 5)
CASE = dict(name='test', count=5, coordinates='xyz', point_ids='ids', sha256='d'*64, ids_sha256='e'*64)


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def paid_work(work, k, mode):
    queries = work['traces'] + work['vertical_descents']
    hits = queries // 2 if mode == 7 else 0
    steps = queries - hits
    work.update(descent_steps=steps, part_meb_presentations=steps,
                part_diameter_pairs=steps * k * (k - 1) // 2, census_point_tests=steps)
    if mode & 4:
        work.update(memo_queries=queries, memo_lookups=queries, memo_misses=steps, memo_hits=hits,
                    memo_insertions=steps)


def stream(request):
    bits, mode, workers = (request[key] for key in ('coord_bits', 'optimizations', 'workers'))
    result = events(bits=bits, kmax=5, workers=workers, optimizations=mode)
    result[0].update(sites=5, points=5)
    result[2].update(wall_ns=1200, forest_ns=1000, orders=[])
    for k, value in enumerate(VALUE['orders'], 1):
        births, nodes = value['births'], len(value['nodes'])
        work = dict.fromkeys(full.WORK, 0)
        if k > 1:
            work.update(vertical_descents=births, vertical_checks=nodes-1, ancestor_queries=births+nodes-1)
        parallel = dict.fromkeys(full.parallel.FIELDS, 0)
        if k == 1:
            # Synthetic paid-work fixture; geometry comes independently from the encoded Definition tree.
            work.update(cells=4, replayed_cells=3, plateaus=1, traces=6)
            if mode & 8:
                parallel.update(regular_batches=1, regular_cells=2, regular_traces=4, extended_cells=1,
                                max_regular_batch=2, regular_dispatch_ns=30, regular_task_sum_ns=20*min(workers, 2),
                                regular_task_max_ns=20, regular_publish_ns=10, extended_ns=20)
        paid_work(work, k, mode)
        result[2]['orders'].append(dict(k=k, births=births, nodes=nodes, edges=nodes-1,
            verticals=nodes if k > 1 else 0, node_capacity=2*births-1, edge_capacity=2*births-2, work=work,
            timings=dict(classify_ns=1, births_ns=1, plateaus_ns=100, verticals_ns=10 if k > 1 else 0),
            parallel=parallel))
    return result


def request(mode=15, workers=48, bits=21, repetition=0):
    return dict(case='test', coord_bits=bits, kmax=5, workers=workers, repetition=repetition, optimizations=mode)


def wire(value):
    return '\n'.join(json.dumps(event) for event in value).encode()


class Attempts:
    def __init__(self, root):
        self.root = root
        self.args = arguments(root, 'attempts')
        self.args.work.mkdir()
        self.decode = full.semantic.decode

    def counted_decode(self, *args):
        COUNTS['decodes'] += 1
        return self.decode(*args)

    def run(self, req, mutation=None, *, cache=None, process='ok', raw_suffix=b'', case=None):
        COUNTS['attempts'] += 1
        current = dict(req, repetition=COUNTS['attempts'])
        args = argparse.Namespace(**vars(self.args))
        args.optimizations = current['optimizations']
        checkpoints = []

        def child(argv, **kwargs):
            need(kwargs['timeout'] == full.TIMEOUT and kwargs['check'] is False, 'bounded native child')
            need(argv[4:] == ['5', '16', '256', '0', str(2**32-1), str(full.BUDGET),
                             str(current['workers']), str(current['optimizations'])], 'worker/mode/whole input argv')
            if process == 'launch':
                raise OSError('fake launch unavailable')
            if process == 'timeout':
                raise subprocess.TimeoutExpired(argv, full.TIMEOUT, output=b'partial native output', stderr=b'late')
            value = stream(current)
            if mutation is not None:
                mutation(value)
            Path(argv[3]).write_bytes(encode(VALUE, current['coord_bits'])[0] + raw_suffix)
            code = 2 if process == 'refused' else 1 if process == 'failed' else 0
            return subprocess.CompletedProcess(argv, code, wire(value), b'')

        with patch.object(full.subprocess, 'run', side_effect=child), \
                patch.object(full.semantic, 'decode', side_effect=self.counted_decode):
            row = full.measure(self.root/'fake', case or CASE, current, args,
                               lambda value: checkpoints.append(copy.deepcopy(value)), semantic_cache=cache)
        need(len(checkpoints) == 1 and full.identity(checkpoints[0]) == full.identity(current), 'one exact checkpoint')
        need(checkpoints[0]['status'] == ('pending_semantic' if process == 'ok' else
             'launch_error' if process == 'launch' else process), 'first process verdict persisted')
        need('semantic' not in checkpoints[0], 'checkpoint before geometric decode')
        need(not Path(row['argv'][3]).exists(), 'attempt artifact released after collection')
        need(row['stdout'] == '' if process == 'launch' else bool(row['stdout']), 'process output retained')
        return row


def order(value):
    return value[2]['orders'][0]


def trace_inventory(value, traces, regular_traces):
    order(value)['work']['traces'] = traces
    order(value)['parallel']['regular_traces'] = regular_traces
    paid_work(order(value)['work'], 1, value[2]['optimizations'])


def batch_fixture(value, cells, batches, maximum):
    work, parallel = order(value)['work'], order(value)['parallel']
    work.update(replayed_cells=cells, traces=2*cells)
    parallel.update(regular_cells=cells, regular_traces=2*cells, regular_batches=batches,
                    max_regular_batch=maximum, extended_cells=0, extended_ns=0)
    paid_work(work, 1, value[2]['optimizations'])


def unassigned_trace(value):
    batch_fixture(value, 1, 1, 1)
    trace_inventory(value, 4, 3)


def corruptions():
    return {
        'metadata_missing': lambda e: e[2].pop('parallel'),
        'metadata_field_missing': lambda e: e[2]['parallel'].pop('descent_lanes'),
        'metadata_extra': lambda e: e[2]['parallel'].update(alien=0),
        'metadata_bool': lambda e: e[2]['parallel'].update(descent_lanes=True),
        'metadata_negative': lambda e: e[2]['parallel'].update(lane_memo_capacity=-1),
        'metadata_overflow': lambda e: e[2]['parallel'].update(lane_memo_reserved_bytes=2**64),
        'batch_requested': lambda e: e[2]['parallel'].update(regular_batch_capacity=4095),
        'lanes_requested': lambda e: e[2]['parallel'].update(descent_lanes=47),
        'memo_requested': lambda e: e[2]['parallel'].update(lane_memo_capacity=4095),
        'memo_bytes': lambda e: e[2]['parallel'].update(lane_memo_reserved_bytes=1),
        'coexistence': lambda e: e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes'] +
            e[2]['memo_reserved_bytes'] + e[2]['parallel']['lane_memo_reserved_bytes'] - 1),
        'order_missing': lambda e: order(e).pop('parallel'),
        'order_field_missing': lambda e: order(e)['parallel'].pop('regular_cells'),
        'order_extra': lambda e: order(e)['parallel'].update(alien=0),
        'order_bool': lambda e: order(e)['parallel'].update(regular_batches=True),
        'order_negative': lambda e: order(e)['parallel'].update(extended_ns=-1),
        'order_overflow': lambda e: order(e)['parallel'].update(regular_task_sum_ns=2**64),
        'coverage': lambda e: order(e)['parallel'].update(extended_cells=2),
        'faces_lower': lambda e: order(e)['parallel'].update(regular_traces=3),
        'faces_upper': lambda e: trace_inventory(e, 9, 9),
        'faces_inventory': lambda e: trace_inventory(e, 3, 4),
        'trace_unassigned_without_extended': unassigned_trace,
        'extended_cell_without_trace': lambda e: trace_inventory(e, 4, 4),
        'batches_missing': lambda e: order(e)['parallel'].update(regular_batches=0),
        'batches_excess': lambda e: order(e)['parallel'].update(regular_batches=3),
        'batch_max_zero': lambda e: order(e)['parallel'].update(max_regular_batch=0),
        'batch_max_large': lambda e: order(e)['parallel'].update(max_regular_batch=4097),
        'batch_max_vs_cells': lambda e: order(e)['parallel'].update(max_regular_batch=3),
        'batch_product': lambda e: order(e)['parallel'].update(max_regular_batch=1),
        'task_max_vs_sum': lambda e: order(e)['parallel'].update(regular_task_sum_ns=19),
        'task_max_vs_wall': lambda e: order(e)['parallel'].update(regular_task_max_ns=31),
        'task_sum_vs_workers': lambda e: order(e)['parallel'].update(regular_task_sum_ns=1441),
        'disjoint_times': lambda e: order(e)['parallel'].update(regular_publish_ns=51),
        'empty_work_time': lambda e: e[2]['orders'][1]['parallel'].update(regular_dispatch_ns=1),
    }


def attempt_gates(root):
    harness = Attempts(root)
    for bits in (21, 24):
        for mode in (3, 7, 11, 15):
            for workers in (1, 8, 48):
                row = harness.run(request(mode, workers, bits))
                need(row['status'] == 'ok' and row['semantic']['sites'] == 5, 'real FULL decoded at each mode/profile/worker')
                need(row['full_within_200ms'] and len(row['order_stage_ms']) == 5, 'current stage diagnostics retained')
    for cells, batches, maximum in ((4096, 1, 4096), (4097, 2, 4096)):
        row = harness.run(request(), lambda e: batch_fixture(e, cells, batches, maximum))
        need(row['status'] == 'ok', 'full and partial final batch boundary')
    row = harness.run(request(), lambda e: batch_fixture(e, 4097, 1, 4096))
    need(row['status'] == 'invalid_output', 'one batch cannot cover more than its fixed capacity')
    COUNTS['corruptions'] += 1
    row = harness.run(request(), lambda e: order(e)['parallel'].update(
        dict.fromkeys(full.parallel.FIELDS, 0), extended_cells=3, extended_ns=20))
    need(row['status'] == 'ok', 'extended-only order has no regular dispatch work')
    for name, change in corruptions().items():
        row = harness.run(request(), change)
        need(row['status'] == 'invalid_output' and row['errors'][-1]['stage'] == 'FULL_semantic', 'lane corruption '+name)
        COUNTS['corruptions'] += 1
    for workers in (1, 8):
        row = harness.run(request(workers=workers), lambda e: order(e)['parallel'].update(
            regular_task_sum_ns=30*workers+1))
        need(row['status'] == 'invalid_output', 'elapsed sum uses actual worker count')
        COUNTS['corruptions'] += 1
    for change in (lambda e: e[2]['parallel'].update(descent_lanes=48),
                   lambda e: order(e)['parallel'].update(regular_cells=1)):
        need(harness.run(request(mode=3), change)['status'] == 'invalid_output', 'disabled lanes have zero diagnostics')
        COUNTS['corruptions'] += 1
    for process in ('refused', 'failed', 'timeout', 'launch'):
        row = harness.run(request(), process=process)
        need(row['status'] == ('launch_error' if process == 'launch' else process), 'nonzero attempt never promoted')
        need('semantic' not in row, 'failure cannot enter equality comparisons')
    return harness


def cache_gates(harness):
    cache = full.reuse.SummaryCache()
    first = harness.run(request(mode=7), cache=cache)
    before = COUNTS['decodes']
    second = harness.run(request(mode=15, workers=8), cache=cache)
    need(first['status'] == second['status'] == 'ok' and len(cache) == 1 and COUNTS['decodes'] == before,
         'cross-mode/lane summary reuse still reads current artifact')
    evidence = second['semantic_reuse']
    need(evidence['mode'] == 'reused' and evidence['raw_sha256'] == first['semantic']['raw_sha256'] and
         evidence['bytes'] == first['semantic']['bytes'] and evidence['decode_wall_seconds'] == 0 and
         evidence['hash_wall_seconds'] >= 0 and evidence['source_attempt'] != evidence['current_attempt'],
         'reuse records source, current identity and complete raw hash')
    bad = harness.run(request(), lambda e: order(e).update(nodes=order(e)['nodes']-1), cache=cache)
    need(bad['status'] == 'invalid_output' and bad['semantic_reuse']['mode'] == 'reused' and
         COUNTS['decodes'] == before and len(cache) == 1, 'current native counts rejected after a cache hit')
    bad = harness.run(request(), lambda e: order(e)['parallel'].update(regular_task_sum_ns=1441), cache=cache)
    need(bad['status'] == 'invalid_output' and COUNTS['decodes'] == before and len(cache) == 1,
         'current lane durations rejected even with a cached summary')
    changed = harness.run(request(), cache=cache, raw_suffix=b'changed')
    need(changed['status'] == 'invalid_output' and COUNTS['decodes'] == before+1 and len(cache) == 1,
         'changed bytes cannot reuse the previous successful summary')
    other = harness.run(request(), cache=cache, case=dict(CASE, sha256='f'*64))
    need(other['status'] == 'ok' and other['semantic_reuse']['mode'] == 'decoded' and len(cache) == 2,
         'identical output under another input context must be decoded again')


def setup(root, name):
    args = arguments(root, name)
    args.budget_seconds = 600
    args.reuse_semantic = True
    manifest = dict(cases=[dict(CASE, name=case) for case in driver.profiles.COUNTS])
    stack = contextlib.ExitStack()
    stack.enter_context(patch.object(driver.profiles, 'checked_builds', return_value={
        bits: dict(path='fake%d' % bits) for bits in (21, 24)}))
    stack.enter_context(patch.object(driver.profiles, 'checked_supplement', return_value='c'*64))
    stack.enter_context(patch.object(driver.profiles, 'inputs', return_value=(manifest, 'a'*64)))
    stack.enter_context(patch.object(driver.base, 'digest', return_value='b'*64))
    stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
    return args, stack


def campaign(root, mode):
    COUNTS['schedules'] += 1
    args, stack = setup(root, 'campaign_'+mode)
    actual_measure, actual_decode = full.measure, full.semantic.decode
    launched = []

    def measured(exe, case, req, call_args, checkpoint, **options):
        need(call_args.optimizations == req['optimizations'], 'campaign passes each requested mode')
        live = json.loads((args.out/'full_parallel.json').read_text())
        need(len(live['launch_intents']) == len(launched)+1 and len(live['runs']) == len(launched), 'intent precedes process')
        launched.append(req)
        if mode == 'missing_checkpoint':
            return dict(req, status='failed')

        def child(argv, **kwargs):
            COUNTS['attempts'] += 1
            need(kwargs['timeout'] == 60 and argv[10:] == [str(req['workers']), str(req['optimizations'])],
                 'campaign worker/mode/timeout argv')
            if mode == 'interrupt_before':
                raise KeyboardInterrupt
            if mode == 'failure' and len(launched) == 1:
                raise subprocess.TimeoutExpired(argv, 60, output=b'partial')
            Path(argv[3]).write_bytes(encode(VALUE, req['coord_bits'])[0])
            return subprocess.CompletedProcess(argv, 0, wire(stream(req)), b'')

        with patch.object(full.subprocess, 'run', side_effect=child):
            return actual_measure(exe, case, req, call_args, checkpoint, **options)

    def decoded(*parameters):
        COUNTS['decodes'] += 1
        if mode == 'interrupt_after':
            raise KeyboardInterrupt
        return actual_decode(*parameters)

    with stack, patch.object(full, 'measure', side_effect=measured), patch.object(full.semantic, 'decode', side_effect=decoded):
        if mode.startswith('budget'):
            stack.enter_context(patch.object(driver.time, 'monotonic', side_effect=lambda:
                100000 if (mode == 'budget_all' or len(launched) >= 3) else 0))
            if mode == 'budget_all':
                ticks = iter([0]+[100000]*100)
                stack.enter_context(patch.object(driver.time, 'monotonic', side_effect=lambda: next(ticks)))
        try:
            code = driver.run(args)
        except KeyboardInterrupt:
            need(mode.startswith('interrupt'), 'only intentional interruption')
            COUNTS['interruptions'] += 1
            code = None
        except ValueError:
            need(mode == 'missing_checkpoint', 'only intentional checkpoint refusal')
            code = None
    report = json.loads((args.out/'full_parallel.json').read_text())
    need(report['schema'] == driver.SCHEMA and report['requested_runs'] == 19 and report['semantic_reuse_enabled'],
         'parallel campaign contract persisted')
    if code is None:
        after = mode == 'interrupt_after'
        need(not report['complete'] and not report['conforming'] and len(report['launch_intents']) == 1,
             'interrupted/incomplete campaign never succeeds')
        need(len(report['runs']) == int(after) and not report['not_run'], 'no invented result or budget omission')
        if after:
            row = report['runs'][0]
            need(row['status'] == 'pending_semantic' and len(row['events']) == 4 and 'semantic' not in row and
                 Path(row['argv'][3]).is_file(), 'completed native evidence survives decoder interruption')
        return report
    success = mode == 'ok'
    need(code == (0 if success else 1) and report['conforming'] is success and report['complete'], 'campaign final verdict')
    observed = [full.identity(row) for key in ('runs', 'not_run') for row in report[key]]
    wanted = [full.identity(row) for row in driver.schedule()]
    need(len(observed) == len(set(observed)) == 19 and set(observed) == set(wanted), 'exact attempt/omission inventory')
    need(len(report['launch_intents']) == len(report['runs']), 'one intent per attempted result')
    if mode.startswith('budget'):
        count = 0 if mode == 'budget_all' else 3
        need(len(report['runs']) == count and len(report['not_run']) == 19-count and
             all(row['reason'] == 'campaign_budget_before_launch' for row in report['not_run']), 'only budget omissions')
    else:
        need(len(report['runs']) == 19 and not report['not_run'] and report['full_schedule_completed'], 'failures do not suppress another mode')
    if mode == 'failure':
        need(report['runs'][0]['status'] == 'timeout' and report['runs'][1]['status'] == 'ok', 'mode15 still attempted after mode7 failure')
    return report


def compare_gates(report):
    rows, wanted = report['runs'], report['requested']
    first = wanted[0]['case']
    variations = ('ok', 'variable', 'semantic', 'raw', 'invariant', 'paid', 'lanes', 'incomplete', 'incomplete_different')
    for mode in variations:
        changed = copy.deepcopy(rows)
        target = next(row for row in changed if row['case'] == first and row['workers'] == 8)
        if mode in ('semantic', 'incomplete_different'):
            target['semantic']['sha256'] = 'b'*64
        if mode == 'raw': target['semantic']['raw_sha256'] = 'c'*64
        if mode in ('invariant', 'variable'):
            for row in changed:
                if row['case'] == first and row['optimizations'] == 15:
                    key = 'cells' if mode == 'invariant' else 'census_point_tests'
                    row['events'][2]['orders'][0]['work'][key] += 1
        if mode == 'paid': target['events'][2]['orders'][0]['work']['census_point_tests'] += 1
        if mode == 'lanes': target['events'][2]['orders'][0]['parallel'].update(regular_batches=2, max_regular_batch=1)
        if mode.startswith('incomplete'): changed[0]['status'] = 'timeout'
        for row in changed:
            row['stdout'] = wire(row['events']).decode()
        result = next(value for value in driver.comparisons(changed, wanted) if value['case'] == first)
        expected = 'equal' if mode in ('ok', 'variable') else 'incomplete' if mode == 'incomplete' else 'different'
        need(result['status'] == expected, 'comparison '+mode)
        if mode == 'invariant':
            need(not result['invariant_work_equal'] and result['lane_work_equal'], 'structural guard isolated across modes')
        if mode == 'paid':
            need(result['invariant_work_equal'] and not result['lane_work_equal'], 'actual work guard isolated at fixed mode')
        if mode == 'lanes':
            need(result['lane_work_equal'] and not result['lane_counts_equal'], 'lane coverage guard isolated from work')
        COUNTS['comparisons'] += 1
    for actual, requested in ((rows+[rows[0]], wanted), (rows, wanted+[wanted[0]]),
                              ([dict(rows[0], case='alien')], wanted)):
        try:
            driver.comparisons(actual, requested)
        except ValueError:
            need(True, 'duplicate/foreign comparison refused')
        else:
            raise ValueError('invalid comparison inventory accepted')
        COUNTS['comparisons'] += 1
    groups = driver.comparisons(rows, wanted)
    need(all(group['status'] == 'equal' and group['lane_work_equal'] and group['lane_counts_equal'] for group in groups),
         'all paid work equal at fixed mode across W1/W8/W48 and both profiles')
    need(set(driver.INVARIANT_WORK) | set(driver.VARIABLE_WORK) == full.WORK and
         not (set(driver.INVARIANT_WORK) & set(driver.VARIABLE_WORK)), 'work partition complete and disjoint')


def calendar():
    wanted = driver.schedule()
    need(len(wanted) == 19 and len({full.identity(row) for row in wanted}) == 19, 'unique calendar19')
    need(all(row['kmax'] == 5 and row['repetition'] == 0 for row in wanted), 'FULL K5 fresh processes')
    need([row['optimizations'] for row in wanted[:12]] == [7, 15]*6, 'paired memo then memo+lanes')
    need([row['workers'] for row in wanted[12:14]] == [1, 8] and
         all(row['optimizations'] == 15 and row['coord_bits'] == 21 for row in wanted[12:14]), 'fixed lanes worker ablation')
    need([row['optimizations'] for row in wanted[14:16]] == [3, 11], 'nonmemo lane ablation')
    need(all(not row['case'].startswith('lidar') and row['optimizations'] == 15 and row['coord_bits'] == 21
             for row in wanted[16:]), 'three complete synthetic inputs')


def main():
    calendar()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-parallel-') as folder:
        root = Path(folder)
        cache_gates(attempt_gates(root))
        good = campaign(root, 'ok')
        compare_gates(good)
        for mode in ('failure', 'budget_all', 'budget_tail', 'interrupt_before', 'interrupt_after', 'missing_checkpoint'):
            campaign(root, mode)
    need(COUNTS['attempts'] >= 100 and COUNTS['corruptions'] >= 37 and COUNTS['decodes'] >= 20 and
         COUNTS['schedules'] == 7 and COUNTS['interruptions'] == 2 and COUNTS['comparisons'] == 12,
         'collector coverage floors')
    print('full_parallel_collector_verdict conforme ' + ' '.join('%s%d' % item for item in COUNTS.items()) +
          ' checks%d native0' % CHECKS)


if __name__ == '__main__':
    main()
