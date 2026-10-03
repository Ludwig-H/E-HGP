#!/usr/bin/env python3
"""FULL catalogue options: actual tiny FULL decoding and collector, simulated children only."""
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
from full_campaign_test import arguments
from full_parallel_collector_test import CASE, VALUE, request, stream, wire
from full_bench_semantic_test import encode

full = driver.full
CHECKS = 0
COUNTS = dict(attempts=0, corruptions=0, decodes=0, schedules=0, interruptions=0, comparisons=0)
MAPPING = {15: 3, 63: 15, 127: 31, 64: 16}


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def current_events(req):
    value = stream(req)
    domain = value[1]
    # Distinct nonzero intervals reach the wall exactly, without predicting native timings.
    domain.update(prefix_ns=3, sort_ns=10, level_scan_ns=2, allocation_ns=4, assembly_ns=6)
    if req['optimizations'] & 64:
        domain.update(single_pass_ns=40, compact_ns=15)
    else:
        domain.update(replay_ns=5)
    return value


class Attempts:
    def __init__(self, root):
        self.root = root
        self.args = arguments(root, 'attempts')
        self.args.work.mkdir()
        self.decoder = full.semantic.decode

    def decoded(self, *parameters):
        COUNTS['decodes'] += 1
        return self.decoder(*parameters)

    def run(self, mode=127, bits=21, change=None, cache=None, process='ok', raw_suffix=b'', cleanup=False):
        COUNTS['attempts'] += 1
        req = request(mode=mode, bits=bits, repetition=COUNTS['attempts'])
        args = argparse.Namespace(**vars(self.args)); args.optimizations = mode
        checkpoints = []

        def child(argv, **kwargs):
            need(kwargs['timeout'] == full.TIMEOUT and kwargs['check'] is False, 'bounded process')
            need(argv[4:] == ['5', '16', '256', '0', str(2**32-1), str(full.BUDGET), '48', str(mode)],
                 'whole FULL options reach child')
            values = current_events(req)
            if change is not None:
                change(values)
            Path(argv[3]).write_bytes(encode(VALUE, bits)[0] + raw_suffix)
            return subprocess.CompletedProcess(argv, 2 if process == 'refused' else 0, wire(values), b'')

        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(full.subprocess, 'run', side_effect=child))
            stack.enter_context(patch.object(full.semantic, 'decode', side_effect=self.decoded))
            if cleanup:
                stack.enter_context(patch.object(Path, 'unlink', side_effect=OSError('fixture cleanup refused')))
            row = full.measure(self.root/'fake', CASE, req, args,
                               lambda r: checkpoints.append(copy.deepcopy(r)), semantic_cache=cache)
        need(len(checkpoints) == 1 and checkpoints[0]['status'] ==
             ('refused' if process == 'refused' else 'pending_semantic'), 'checkpoint preserves first process verdict')
        need(checkpoints[0]['stdout'] == row['stdout'] and 'semantic' not in checkpoints[0], 'undecoded evidence retained')
        need(row['optimizations'] == mode and full.identity(row) == full.identity(req), 'identity retained')
        need(Path(row['argv'][3]).exists() is cleanup, 'cleanup state recorded')
        if cleanup:
            Path(row['argv'][3]).unlink()
        return row


def changes():
    return {
        'options': lambda e: e[1].update(catalogue_optimizations=15),
        'options_bool': lambda e: e[1].update(catalogue_optimizations=True),
        'options_overflow': lambda e: e[1].update(catalogue_optimizations=2**64),
        'incidences_bool': lambda e: e[1].update(catalogue_incidences=True),
        'incidences_negative': lambda e: e[1].update(catalogue_incidences=-1),
        'incidences_overflow': lambda e: e[1].update(catalogue_incidences=2**64),
        'execution_missing': lambda e: e[1].pop('execution'),
        'execution_field_missing': lambda e: e[1]['execution'].pop('compact_records'),
        'execution_extra': lambda e: e[1]['execution'].update(alien=0),
        'passes': lambda e: e[1]['execution'].update(geometry_passes=2),
        'passes_bool': lambda e: e[1]['execution'].update(geometry_passes=True),
        'passes_overflow': lambda e: e[1]['execution'].update(geometry_passes=2**64),
        'blocks_empty': lambda e: e[1]['execution'].update(arena_blocks=0),
        'blocks_one': lambda e: e[1]['execution'].update(arena_blocks=1),
        'blocks_bool': lambda e: e[1]['execution'].update(arena_blocks=True),
        'blocks_overflow': lambda e: e[1]['execution'].update(arena_blocks=2**64),
        'capacity_empty': lambda e: e[1]['execution'].update(arena_capacity_bytes=0),
        'metadata_empty': lambda e: e[1]['execution'].update(arena_metadata_bytes=0),
        'metadata_negative': lambda e: e[1]['execution'].update(arena_metadata_bytes=-1),
        'metadata_bool': lambda e: e[1]['execution'].update(arena_metadata_bytes=True),
        'capacity_exceeds_peak': lambda e: e[1]['execution'].update(arena_capacity_bytes=e[2]['peak_reserved_bytes']),
        'metadata_exceeds_peak': lambda e: e[1]['execution'].update(arena_metadata_bytes=e[2]['peak_reserved_bytes']),
        'sum_exceeds_u64': lambda e: e[1]['execution'].update(arena_capacity_bytes=2**63, arena_metadata_bytes=2**63),
        'records_short': lambda e: e[1]['execution'].update(compact_records=e[1]['catalogue_balls']-1),
        'records_excess': lambda e: e[1]['execution'].update(compact_records=e[1]['catalogue_balls']+1),
        'population_short': lambda e: e[1]['execution'].update(compact_population=e[1]['catalogue_incidences']-1),
        'population_excess': lambda e: e[1]['execution'].update(compact_population=e[1]['catalogue_incidences']+1),
        'population_bool': lambda e: e[1]['execution'].update(compact_population=True),
    }


def attempt_gates(root):
    harness = Attempts(root)
    for mode, catalogue in MAPPING.items():
        for bits in (21, 24):
            row = harness.run(mode, bits)
            need(row['status'] == 'ok' and row['events'][1]['catalogue_optimizations'] == catalogue,
                 'explicit catalogue option mapping')
            need(row['semantic']['sites'] == 5 and row['semantic']['kmax'] == 5, 'actual FULL payload judged')
            need(set(row['catalogue_stage_ms']) == {key[:-3] for key in full.DOMAIN_TIMINGS}, 'all ten intervals exposed')
            need(sum(row['catalogue_stage_ms'].values()) == row['stage_ms']['domain'], 'disjoint boundary admitted')
            need(row['catalogue_execution']['geometry_passes'] == (1 if mode in (64, 127) else 2),
                 'actual geometry passes independent of memo/lanes')
            if mode == 64:
                need(row['events'][2]['memo_capacity'] == 0 and
                     all(o['work']['memo_queries'] == 0 for o in row['events'][2]['orders']), 'single pass without memo')
    mutations = changes()
    for field in sorted(full.DOMAIN_TIMINGS):
        mutations['missing_'+field] = lambda e, f=field: e[1].pop(f)
        mutations['bool_'+field] = lambda e, f=field: e[1].update({f: True})
        mutations['overflow_'+field] = lambda e, f=field: e[1].update({f: 2**64})
    mutations['stage_sum'] = lambda e: e[1].update(assembly_ns=e[1]['assembly_ns']+1)
    for field in ('count_ns', 'fill_ns', 'replay_ns'):
        # Preserve the total wall to isolate the forbidden replay, not the time-sum guard.
        mutations['replay_'+field] = lambda e, f=field: e[1].update({f: 1, 'compact_ns': 14})
    for name, change in mutations.items():
        row = harness.run(change=change)
        need(row['status'] == 'invalid_output' and row['errors'][-1]['stage'] == 'FULL_semantic', name)
        need('semantic' not in row and row['stdout'], 'invalid current events never decode a previous result')
        COUNTS['corruptions'] += 1
    for field in full.EXECUTION - {'geometry_passes'}:
        row = harness.run(63, change=lambda e, f=field: e[1]['execution'].update({f: 1}))
        need(row['status'] == 'invalid_output', 'disabled single pass cannot report '+field)
        COUNTS['corruptions'] += 1
    for field in ('single_pass_ns', 'compact_ns'):
        row = harness.run(15, change=lambda e, f=field: e[1].update({f: 1, 'fill_ns': 29}))
        need(row['status'] == 'invalid_output', 'disabled single pass duration '+field)
        COUNTS['corruptions'] += 1
    return harness


def reuse_gates(harness):
    cache = full.reuse.SummaryCache()
    first = harness.run(15, cache=cache); preserved = copy.deepcopy(first)
    before = COUNTS['decodes']
    second = harness.run(63, cache=cache); third = harness.run(127, cache=cache)
    need(first['status'] == second['status'] == third['status'] == 'ok' and COUNTS['decodes'] == before,
         'identical current bytes permit reuse across catalogue options')
    need(second['semantic_reuse']['mode'] == third['semantic_reuse']['mode'] == 'reused' and
         third['semantic_reuse']['source_attempt'] == list(full.identity(first)), 'reuse provenance retained')
    for change in (changes()['records_short'], changes()['options'], changes()['capacity_exceeds_peak']):
        wrong = harness.run(127, change=change, cache=cache)
        need(wrong['status'] == 'invalid_output' and 'semantic_reuse' not in wrong and len(cache) == 1,
             'current catalogue diagnostics checked before summary reuse')
    wrong = harness.run(127, cache=cache,
                        change=lambda e: e[2]['orders'][0].update(nodes=e[2]['orders'][0]['nodes']-1))
    need(wrong['status'] == 'invalid_output' and wrong['semantic_reuse']['mode'] == 'reused',
         'current order counts still checked after a cache hit')
    refused = harness.run(127, cache=cache, process='refused')
    need(refused['status'] == 'refused' and 'semantic' not in refused and len(cache) == 1 and first == preserved,
         'refusal cannot publish or mutate the previous successful result')
    changed = harness.run(127, cache=cache, raw_suffix=b'changed')
    need(changed['status'] == 'invalid_output' and COUNTS['decodes'] == before+1 and len(cache) == 1,
         'changed bytes are fully rehashed and rejected')
    pending = full.reuse.SummaryCache()
    failed = harness.run(127, cache=pending, cleanup=True)
    need(failed['status'] == 'artifact_error' and len(pending) == 0, 'failed cleanup does not publish summary')
    again = harness.run(127, cache=pending)
    need(again['status'] == 'ok' and again['semantic_reuse']['mode'] == 'decoded', 'failed publication is not reusable')


def calendar():
    expected = []
    lidar = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')
    synthetic = ('uniform_u18_n8000', 'uniform_u18_n16000', 'uniform_u18_n32000')
    for names, bits_set in ((lidar, (21, 24)), (synthetic, (21,))):
        for name in names:
            for bits in bits_set:
                for mode in (15, 63, 127):
                    expected.append(dict(case=name, coord_bits=bits, kmax=5, workers=48, repetition=0, optimizations=mode))
    need(driver.schedule(True) == expected and len(set(map(full.identity, expected))) == 27, 'explicit calendar27')
    need(driver.schedule() == driver.schedule(False) and len(driver.schedule()) == 19, 'historical default preserved')
    for value in (0, 1, None, 'true'):
        try:
            driver.schedule(value)
        except ValueError:
            need(True, 'strict campaign option')
        else:
            raise ValueError('non-boolean campaign option accepted')
    return expected


def campaign(root, scenario):
    COUNTS['schedules'] += 1
    args = arguments(root, 'campaign_'+scenario)
    args.budget_seconds = 600; args.reuse_semantic = True; args.optimized_catalogue = True
    manifest = dict(cases=[dict(CASE, name=name) for name in driver.profiles.COUNTS])
    original_measure, original_decode = full.measure, full.semantic.decode
    launched = []
    path = args.out/'full_parallel.json'

    def measure(exe, case, req, call_args, checkpoint, **options):
        live = json.loads(path.read_text())
        need(len(live['launch_intents']) == len(launched)+1 and len(live['runs']) == len(launched), 'intent before child')
        need(call_args.optimizations == req['optimizations'], 'requested catalogue options passed')
        launched.append(req)
        if scenario == 'missing_checkpoint':
            return dict(req, status='failed')

        def child(argv, **kwargs):
            COUNTS['attempts'] += 1
            need(kwargs['timeout'] == 60 and argv[10:] == ['48', str(req['optimizations'])], 'campaign argv and timeout')
            if scenario == 'interrupt_before':
                raise KeyboardInterrupt
            if scenario == 'failure' and len(launched) == 1:
                raise subprocess.TimeoutExpired(argv, 60, output=b'preserved incomplete output')
            Path(argv[3]).write_bytes(encode(VALUE, req['coord_bits'])[0])
            return subprocess.CompletedProcess(argv, 0, wire(current_events(req)), b'')

        with patch.object(full.subprocess, 'run', side_effect=child):
            return original_measure(exe, case, req, call_args, checkpoint, **options)

    def decode(*parameters):
        COUNTS['decodes'] += 1
        if scenario == 'interrupt_after':
            raise KeyboardInterrupt
        return original_decode(*parameters)

    with contextlib.ExitStack() as stack:
        stack.enter_context(patch.object(driver.profiles, 'checked_builds', return_value={
            bits: dict(path='fake%d' % bits) for bits in (21, 24)}))
        stack.enter_context(patch.object(driver.profiles, 'checked_supplement', return_value='c'*64))
        stack.enter_context(patch.object(driver.profiles, 'inputs', return_value=(manifest, 'a'*64)))
        stack.enter_context(patch.object(driver.base, 'digest', return_value='b'*64))
        stack.enter_context(patch.object(full, 'measure', side_effect=measure))
        stack.enter_context(patch.object(full.semantic, 'decode', side_effect=decode))
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        if scenario in ('budget_all', 'budget_tail'):
            ticks = [0]
            def clock():
                ticks[0] += 1
                return 100000 if (scenario == 'budget_all' and ticks[0] > 1 or len(launched) >= 3) else 0
            stack.enter_context(patch.object(driver.time, 'monotonic', side_effect=clock))
        try:
            code = driver.run(args)
        except KeyboardInterrupt:
            need(scenario.startswith('interrupt'), 'only deliberate interrupt')
            COUNTS['interruptions'] += 1; code = None
        except ValueError:
            need(scenario == 'missing_checkpoint', 'only deliberate missing checkpoint')
            code = None
    report = json.loads(path.read_text())
    need(report['optimized_catalogue'] is True and report['requested_runs'] == 27 and
         report['requested'] == driver.schedule(True), 'catalogue ablation contract persisted')
    if code is None:
        after = scenario == 'interrupt_after'
        need(not report['complete'] and not report['conforming'] and len(report['launch_intents']) == 1,
             'interruption cannot become conforming')
        need(len(report['runs']) == int(after) and not report['not_run'], 'no fabricated attempts or omissions')
        if after:
            row = report['runs'][0]
            need(row['status'] == 'pending_semantic' and len(row['events']) == 4 and
                 Path(row['argv'][3]).is_file() and 'semantic' not in row, 'native evidence survives decoder interruption')
        return report
    need(report['complete'] and report['conforming'] is (scenario == 'ok') and code == int(scenario != 'ok'),
         'complete campaign verdict')
    actual = [full.identity(r) for field in ('runs', 'not_run') for r in report[field]]
    need(len(actual) == len(set(actual)) == 27 and set(actual) == set(map(full.identity, report['requested'])),
         'attempts and omissions exactly partition the requested calendar')
    if scenario.startswith('budget'):
        n = 0 if scenario == 'budget_all' else 3
        need(len(report['runs']) == n and len(report['not_run']) == 27-n and
             all(r['reason'] == 'campaign_budget_before_launch' for r in report['not_run']), 'budget-only omissions')
    else:
        need(len(report['runs']) == 27 and not report['not_run'] and report['full_schedule_completed'], 'all modes attempted')
    if scenario == 'failure':
        need(report['runs'][0]['status'] == 'timeout' and
             [r['status'] for r in report['runs'][1:3]] == ['ok', 'ok'], 'mode15 failure never suppresses63 or127')
    return report


def comparison_gates(report):
    rows, requested = report['runs'], report['requested']
    for mode in ('ok', 'paid63', 'paid127', 'invariant', 'semantic', 'raw', 'lanes', 'incomplete', 'incomplete_different'):
        changed = copy.deepcopy(rows)
        target = next(r for r in changed if r['case'] == 'lidar_ng00' and r['coord_bits'] == 21 and
                      r['optimizations'] == (63 if mode == 'paid63' else 127))
        if mode.startswith('paid'):
            target['events'][2]['orders'][0]['work']['census_point_tests'] += 1
        if mode == 'invariant':
            target['events'][2]['orders'][0]['work']['cells'] += 1
        if mode in ('semantic', 'incomplete_different'):
            target['semantic']['sha256'] = 'b'*64
        if mode == 'raw':
            target['semantic']['raw_sha256'] = 'c'*64
        if mode == 'lanes':
            target['events'][2]['orders'][0]['parallel']['regular_batches'] += 1
        if mode.startswith('incomplete'):
            changed[0]['status'] = 'timeout'
        for row in changed:
            row['stdout'] = wire(row['events']).decode()
        group = next(g for g in driver.comparisons(changed, requested) if g['case'] == 'lidar_ng00')
        expected = 'equal' if mode == 'ok' else 'incomplete' if mode == 'incomplete' else 'different'
        need(group['status'] == expected, 'comparison '+mode)
        if mode.startswith('paid'):
            need(group['invariant_work_equal'] and not group['lane_work_equal'], 'paid work compared across catalogue options')
        COUNTS['comparisons'] += 1


def main():
    calendar()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-catalogue-') as folder:
        root = Path(folder)
        reuse_gates(attempt_gates(root))
        comparison_gates(campaign(root, 'ok'))
        for scenario in ('failure', 'budget_all', 'budget_tail', 'interrupt_before', 'interrupt_after', 'missing_checkpoint'):
            campaign(root, scenario)
    need(COUNTS['attempts'] >= 140 and COUNTS['corruptions'] >= 60 and COUNTS['decodes'] >= 15 and
         COUNTS['schedules'] == 7 and COUNTS['interruptions'] == 2 and COUNTS['comparisons'] == 9, 'coverage floors')
    print('full_catalogue_collector_verdict conforme ' + ' '.join('%s%d' % item for item in COUNTS.items()) +
          ' checks%d native0' % CHECKS)


if __name__ == '__main__':
    main()
