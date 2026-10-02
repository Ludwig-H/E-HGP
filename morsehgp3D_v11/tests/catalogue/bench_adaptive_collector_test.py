#!/usr/bin/env python3
"""Adaptive telemetry, real collection and schedules with mocked children only; no native execution."""
import contextlib
import copy
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_parallel_collector_test import environment, setup
from bench_semantic_test import fixture
import catalogue_adaptive as driver

D = driver.diagnostics
CHECKS = 0


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def events(bits=21, workers=48, mode=7, count=4, jobs=1, requested=True):
    """Structurally coherent telemetry; not asserted to be a geometric execution of a synthetic Cloud."""
    depth = (jobs-1).bit_length()
    ledger = dict.fromkeys(D.LEDGER_FIELDS, 0)
    ledger.update(leaves=1, prefixes=11, judged=11, census_tests=44, emitted=11, incidences=28,
                  q4_candidates=1, q4_levels=1, region_pair_tests=12, region_line_tests=7,
                  region_line_evaluations=3 if mode & 1 else 7, region_line_cache_hits=4 if mode & 1 else 0,
                  max_leaf=4, max_depth=depth)
    tasks = []
    for i in range(jobs):
        part = count//jobs + int(i < count % jobs)
        raw = i << (128-depth) if depth else 0
        t = dict(ordinal=i, path=[raw >> 64, raw % 2**64] if mode & 4 else [0, 0],
                 path_known=bool(mode & 4), inside_known=bool(mode & 4), lo=[0, 0, 0], hi=[8, 8, 8],
                 depth=depth, count=part, capacity=count, inside=part if mode & 4 else 0,
                 count_ns=1, fill_ns=1, ledger=dict(ledger) if i == 0 else dict.fromkeys(D.LEDGER_FIELDS, 0))
        tasks.append(t)
    total = dict(ledger, nodes=2*jobs-1, filter_tests=16)
    logical = {k: total[k] for k in driver.profiles.LOGICAL}
    timing = dict.fromkeys(driver.parallel.TIMING_FIELDS, 1)
    phase = (jobs+workers-1)//workers
    timing.update(tasks=jobs, count_ns=phase, fill_ns=phase, count_task_sum_ns=jobs, fill_task_sum_ns=jobs)
    p = dict(adaptive=bool(mode & 4), memory_fallback=False, plan_nodes=2*jobs-1, plan_leaves=jobs,
             empty_leaves=0, rounds=depth if mode & 4 else 0,
             priority_tests=count if mode & 4 and jobs > 1 else 0, replay_bytes=(8 if mode & 4 else 40)*count)
    diag = dict(schema=D.SCHEMA, record_bytes=D.RECORD_BYTES, reserved_bytes=jobs*D.RECORD_BYTES,
                planning=p, tasks=tasks)
    retained = 28*count+8+2048+(diag['reserved_bytes'] if requested else 0)
    event = dict(phase='catalogue', status='ok', coord_bits=bits, kmax=5, balls=11, levels=4, incidences=28,
                 workers=workers, pool_ns=4, wall_ns=2*phase+20, peak_reserved_bytes=retained+1024,
                 reserved_after_bytes=retained, generation_passes=2, optimizations=mode,
                 diagnostics_requested=requested, timings=timing, logical=logical,
                 work={'q4_candidates': 1, 'q4_levels': 1},
                 cache_work=dict(evaluations=total['region_line_evaluations'], hits=total['region_line_cache_hits'], fallbacks=0))
    if requested:
        event['diagnostics'] = diag
    return [dict(phase='cloud', points=count, sites=count, cloud_ns=20, read_ns=10), event,
            dict(phase='exit', status='ok')]


def ready(request, count=None):
    count = driver.profiles.COUNTS[request['case']] if count is None else count
    row = dict(request, status='ok', count=count, stderr='', stdout='', errors=[], cloud_ms=0.00002,
               catalogue_ms=0.000022, semantic=dict(sha256='a'*64, balls=11),
               qmin_counts={'2': 6, '3': 4, '4': 1}, canonical_sha256=str(request['coord_bits'])*32)
    row['events'] = events(request['coord_bits'], request['workers'], request['optimizations'], count)
    return row


def structural():
    positives = 0
    for bits in (18, 21, 24):
        for mode in (3, 7):
            for jobs in (1, 256 if mode == 3 else 1024):
                value = events(bits, 8, mode, max(4, jobs), jobs)[1]
                D.check(value, max(4, jobs), bits); positives += 1
    ghost = events()[1]
    ghost['diagnostics']['planning'].update(plan_nodes=3, plan_leaves=2, empty_leaves=1, memory_fallback=True)
    ghost['logical']['nodes'] = 3
    D.check(ghost, 4, 21); positives += 1
    source = events()[1]
    changes = []
    def edit(name, action):
        v = copy.deepcopy(source); action(v); changes.append((name, v))
    edit('schema', lambda e: e['diagnostics'].update(schema='other'))
    edit('unknown_field', lambda e: e['diagnostics'].update(extra=0))
    edit('ABI', lambda e: e['diagnostics'].update(record_bytes=263))
    edit('reservation', lambda e: e['diagnostics'].update(reserved_bytes=0))
    edit('coexistence', lambda e: e.update(reserved_after_bytes=383))
    edit('peak', lambda e: e.update(peak_reserved_bytes=0))
    edit('memory_bool', lambda e: e.update(peak_reserved_bytes=True))
    edit('cache_inventory', lambda e: e['cache_work'].update(evaluations=4))
    edit('mode_bool', lambda e: e.update(optimizations=True))
    edit('workers_bool', lambda e: e.update(workers=True))
    edit('adaptive_flag', lambda e: e['diagnostics']['planning'].update(adaptive=False))
    edit('fallback_type', lambda e: e['diagnostics']['planning'].update(memory_fallback=0))
    edit('binary_plan', lambda e: e['diagnostics']['planning'].update(plan_nodes=2))
    edit('ghost_inventory', lambda e: e['diagnostics']['planning'].update(empty_leaves=1))
    edit('round_bound', lambda e: e['diagnostics']['planning'].update(rounds=64))
    edit('replay_low', lambda e: e['diagnostics']['planning'].update(replay_bytes=31))
    edit('replay_high', lambda e: e['diagnostics']['planning'].update(replay_bytes=32769))
    edit('priority_bool', lambda e: e['diagnostics']['planning'].update(priority_tests=True))
    def task(e): return e['diagnostics']['tasks'][0]
    edit('ordinal', lambda e: task(e).update(ordinal=1))
    edit('path_known', lambda e: task(e).update(path_known=False))
    edit('inside_known', lambda e: task(e).update(inside_known=False))
    edit('depth', lambda e: task(e).update(depth=64))
    edit('capacity', lambda e: task(e).update(capacity=3))
    edit('count_zero', lambda e: task(e).update(count=0))
    edit('population', lambda e: task(e).update(inside=3))
    edit('negative_box', lambda e: task(e).update(lo=[-1, 0, 0]))
    edit('wide_box', lambda e: task(e).update(hi=[2**21+1, 8, 8]))
    edit('inverted_box', lambda e: task(e).update(lo=[8, 0, 0]))
    edit('path_bits', lambda e: task(e).update(path=[0, 1]))
    edit('task_ledger_bool', lambda e: task(e)['ledger'].update(leaves=True))
    edit('task_ledger_missing', lambda e: task(e)['ledger'].pop('leaves'))
    edit('node_total', lambda e: task(e)['ledger'].update(nodes=1))
    edit('suffix_total', lambda e: task(e)['ledger'].update(prefixes=10))
    edit('filter_total', lambda e: task(e)['ledger'].update(filter_tests=17))
    edit('depth_total', lambda e: task(e)['ledger'].update(max_depth=1))
    edit('interval_total', lambda e: task(e).update(count_ns=0))
    edit('wall_total', lambda e: e.update(wall_ns=0))
    # Admitted by old max<=wall and sum<=J*max, but impossible with one simultaneous worker.
    v = events(workers=1, count=8, jobs=8)[1]
    v['timings'].update(count_ns=1); changes.append(('concurrency', v))
    v = events(count=4, jobs=2)[1]; v['diagnostics']['tasks'][1]['path'] = [0, 0]
    changes.append(('duplicate_path', v))
    v = events(mode=3)[1]; v['diagnostics']['planning']['priority_tests'] = 1
    changes.append(('fixed_priority', v))
    v = events(mode=3)[1]; v['diagnostics']['tasks'][0]['inside'] = 4
    changes.append(('fixed_unknown', v))
    v = events(mode=3, count=257, jobs=257)[1]; changes.append(('fixed_257', v))
    v = events(count=1025, jobs=1025)[1]; changes.append(('adaptive_1025', v))
    for name, value in changes:
        try:
            D.check(value, value['diagnostics']['tasks'][0]['capacity'], 21)
        except (ValueError, KeyError, TypeError):
            pass
        else:
            raise ValueError('corruption accepted: '+name)
    need(len(changes) >= 40, 'structural corruption floor')
    return positives, len(changes)


def attempts(root):
    args, _, _ = environment(root, 'attempts'); args.work.mkdir()
    case = dict(name='fake', coordinates='xyz', point_ids='ids', count=4)
    kinds = ('ok3', 'ok7', 'off', 'other_bits', 'missing_diag', 'unsolicited', 'echo', 'mask', 'workers',
             'bad_diag', 'stderr', 'bad_json', 'damaged', 'missing', 'timeout', 'launch', 'refused', 'failed', 'signal')
    for kind in kinds:
        mode, requested = (3 if kind == 'ok3' else 7), kind not in ('off', 'unsolicited')
        bits, workers = (24, 8) if kind == 'other_bits' else (21, 48)
        checks = []
        def child(argv, **kwargs):
            need(argv[10:] == [str(workers), str(mode)]+(['1'] if requested else []), 'full native options')
            need(kwargs['timeout'] == 15 and not kwargs['check'], 'bounded native invocation')
            if kind == 'timeout':
                raise subprocess.TimeoutExpired(argv, 15, output=b'{', stderr=b'partial\xff')
            if kind == 'launch': raise OSError('fake launch')
            values = events(bits, workers, mode, requested=requested); e = values[1]
            if kind == 'missing_diag': del e['diagnostics']
            if kind == 'unsolicited': e['diagnostics'] = events()[1]['diagnostics']
            if kind == 'echo': e['diagnostics_requested'] = False
            if kind == 'mask': e['optimizations'] = 3
            if kind == 'workers': e['workers'] = 1
            if kind == 'bad_diag': e['diagnostics']['tasks'][0]['inside'] = 3
            raw = '\n'.join(json.dumps(v) for v in values).encode()
            if kind in ('bad_json', 'refused', 'failed', 'signal'): raw += b'\n{"phase":'
            if kind != 'missing':
                data = fixture(bits)[0]; Path(argv[3]).write_bytes(data[:-1] if kind == 'damaged' else data)
            return subprocess.CompletedProcess(argv, {'refused': 2, 'failed': 3, 'signal': -15}.get(kind, 0),
                                               raw, b'warning' if kind == 'stderr' else b'')
        with patch.object(driver.profiles.subprocess, 'run', side_effect=child):
            row = driver.profiles.measure(root/'fake', case, bits, 5, args, lambda r: checks.append(copy.deepcopy(r)),
                                         workers=workers, timeout=15, optimizations=mode, diagnostics=requested)
        if requested:
            driver.check_adaptive(row, dict(case='fake', coord_bits=bits, kmax=5, workers=workers, repetition=0,
                                           optimizations=mode, diagnostics=True))
        expected = ('ok' if kind in ('ok3', 'ok7', 'off', 'other_bits') else
                    dict(damaged='artifact_error', missing='artifact_error', timeout='timeout', launch='launch_error',
                         refused='refused', failed='failed', signal='failed').get(kind, 'invalid_output'))
        need(row['status'] == expected, kind+': '+row['status'])
        need(len(checks) == 1 and checks[0]['stdout'] == row['stdout'], 'process checkpoint preserved')
        need(not Path(row['argv'][3]).exists(), 'canonical artifact cleaned')
        if expected == 'ok':
            need(row['semantic']['balls'] == 11 and checks[0]['status'] == 'pending_semantic', 'actual decode after checkpoint')
        else:
            need(row['errors'], 'error evidence retained')
    return len(kinds)


def options(root):
    args, _, _ = environment(root, 'options')
    case = dict(name='fake', coordinates='xyz', point_ids='ids', count=4)
    positives = 0
    for mode in range(8):
        for diagnostic in (False, True):
            _, argv = driver.profiles.invocation(root/'fake', case, 21, 5, args, 4, 0, mode, diagnostic)
            expected = ['4'] + ([str(mode)] if mode or diagnostic else []) + (['1'] if diagnostic else [])
            need(argv[10:] == expected, 'option mask/request encoded independently')
            positives += 1
    _, argv = driver.profiles.invocation(root/'fake', case, 21, 5, args)
    need(len(argv) == 10, 'old serial argv unchanged'); positives += 1
    bad = ((0, 1, False), (0, 0, True), (257, 0, False), (-1, 0, False), (True, 0, False),
           (1, -1, False), (1, 8, False), (1, True, False), (1, 0, 1), (1, 0, None), (1, 0, '0'))
    for workers, mode, diagnostic in bad:
        try: driver.profiles.invocation(root/'fake', case, 21, 5, args, workers, 0, mode, diagnostic)
        except ValueError: pass
        else: raise ValueError('invalid option accepted before subprocess')
    return positives+len(bad)


def compare_controls():
    requested = driver.schedule(); rows = [ready(r) for r in requested]
    need(len(requested) == len(set(map(driver.identity, requested))) == 36, 'complete schedule identities')
    need(sum(r['workers'] == 8 for r in requested) == 12 and
         [r['optimizations'] for r in requested[:2]] == [3, 7], 'W8 and mode pairs')
    need(all(c['status'] == 'equal' for c in driver.comparisons(rows, requested)), 'valid comparison groups')
    kinds = ('semantic', 'raw', 'logical', 'q4', 'cache', 'plan', 'duplicate', 'unknown', 'bool', 'missing_diag')
    for kind in kinds:
        changed = copy.deepcopy(rows)
        if kind == 'semantic': changed[1]['semantic']['sha256'] = 'b'*64
        if kind == 'raw': changed[1]['canonical_sha256'] = 'b'*64
        if kind == 'logical': changed[1]['events'][1]['logical']['nodes'] += 1
        if kind == 'q4': changed[1]['events'][1]['work']['q4_candidates'] += 1
        if kind == 'cache': changed[1]['events'][1]['cache_work'].update(evaluations=4, hits=3)
        if kind == 'plan': changed[1]['events'][1]['diagnostics']['planning']['priority_tests'] += 1
        if kind == 'duplicate': changed.append(changed[0])
        if kind == 'unknown': changed[0]['optimizations'] = 4
        if kind == 'bool': changed[0]['optimizations'] = True
        if kind == 'missing_diag': changed[0]['diagnostics'] = False
        if kind in ('duplicate', 'unknown', 'bool', 'missing_diag'):
            try: driver.comparisons(changed, requested)
            except ValueError: pass
            else: raise ValueError('invalid comparison inventory accepted')
        else:
            need(driver.comparisons(changed, requested)[0]['status'] == 'different', kind+' disagreement')
            changed[0]['status'] = 'timeout'
            need(driver.comparisons(changed, requested)[0]['status'] == 'different', 'incomplete cannot hide disagreement')
    changed = copy.deepcopy(rows)
    for row in changed:
        row['events'][1]['diagnostics']['tasks'][0].update(count_ns=999, fill_ns=222)
    need(all(c['status'] == 'equal' for c in driver.comparisons(changed, requested)), 'clocks are not geometry')
    return len(kinds)


def schedules(root):
    modes = ('ok', 'baseline_failed', 'adaptive_failed', 'all_failed', 'different', 'incomplete_different',
             'plan_different', 'deadline', 'no_budget')
    for mode in modes:
        args, manifest, builds = environment(root, mode); args.budget_seconds = 700
        elapsed, ticks = 0, 0
        def clock():
            nonlocal ticks
            ticks += 1
            return 700 if mode == 'no_budget' and ticks > 1 else elapsed
        def measure(_exe, case, bits, kmax, _args, checkpoint, *, workers, repetition, timeout, optimizations, diagnostics):
            nonlocal elapsed
            need(timeout == 15 and diagnostics is True, 'bounded diagnostic measure')
            row = ready(dict(case=case['name'], coord_bits=bits, kmax=kmax, workers=workers, repetition=repetition,
                             optimizations=optimizations, diagnostics=True))
            pending = copy.deepcopy(row); pending['status'] = 'pending_semantic'; checkpoint(pending)
            target = case['name'] == 'lidar_ng00' and bits == 21 and workers == 48
            if mode == 'all_failed' or (target and ((optimizations == 3 and mode in ('baseline_failed', 'incomplete_different')) or
                                                   (optimizations == 7 and mode == 'adaptive_failed'))):
                row['status'] = 'timeout'
            if target and optimizations == 7:
                if mode in ('different', 'incomplete_different'): row['semantic']['sha256'] = 'b'*64
                if mode == 'plan_different': row['events'][1]['diagnostics']['planning']['priority_tests'] += 1
            if mode == 'deadline': elapsed += 100
            return row
        with setup(manifest, builds), patch.object(driver.profiles, 'measure', side_effect=measure), \
                patch.object(driver.time, 'monotonic', side_effect=clock):
            code = driver.run(args)
        report = driver.load_report(args.out/'adaptive.json.gz')
        need(code == (0 if mode == 'ok' else 1) and report['conforming'] is (mode == 'ok'), 'campaign '+mode)
        inventory = [driver.identity(r) for r in report['runs']+report['not_run']]
        need(len(inventory) == len(set(inventory)) == 36 and set(inventory) == set(map(driver.identity, driver.schedule())),
             'every requested unit preserved')
        need(list(map(driver.identity, report['launch_intents'])) == list(map(driver.identity, report['runs'])), 'intent inventory')
        need(report['complete'] and report['native_schedule_bound_seconds'] == 540, 'campaign bound')
        if mode in ('deadline', 'no_budget'):
            need(len(report['runs']) == (7 if mode == 'deadline' else 0) and
                 all(r['reason'] == 'campaign_budget_before_launch' for r in report['not_run']), 'budget omissions')
        else:
            need(len(report['runs']) == 36 and not report['not_run'], 'failure does not suppress later attempt')
    return len(modes)


def interruptions(root):
    for when in ('launch', 'decoder'):
        args, manifest, builds = environment(root, 'interrupted_'+when); args.budget_seconds = 700
        def child(argv, **_kwargs):
            report = driver.load_report(args.out/'adaptive.json.gz')
            need(not report['runs'] and len(report['launch_intents']) == 1 and
                 report['launch_intents'][0]['argv'] == argv, 'intent persisted before subprocess')
            if when == 'launch': raise KeyboardInterrupt
            Path(argv[3]).write_bytes(fixture(21)[0])
            # The manifest is mocked as four sites here so collection reaches the real decoder boundary.
            return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in events(mode=3)).encode(), b'')
        for case in manifest['cases']: case['count'] = 4
        with setup(manifest, builds), patch.object(driver.profiles.subprocess, 'run', side_effect=child), \
                patch.object(driver.profiles.semantic, 'inspect', side_effect=KeyboardInterrupt):
            try: driver.run(args)
            except KeyboardInterrupt: pass
            else: raise ValueError('interruption swallowed')
        report = driver.load_report(args.out/'adaptive.json.gz')
        need(not report['complete'] and not report['conforming'] and len(report['launch_intents']) == 1, 'interrupted report')
        need(len(report['runs']) == (0 if when == 'launch' else 1), 'interruption stage retained')
        if when == 'decoder':
            need(report['runs'][0]['status'] == 'pending_semantic' and report['runs'][0]['exit_code'] == 0,
                 'unjudged successful process not promoted')
            need(Path(report['runs'][0]['argv'][3]).exists(), 'unfinished artifact retained with pending evidence')
    return 2


def compressed_checkpoints(root):
    path = root/'checkpoint.json.gz'
    value = dict(schema=driver.SCHEMA, complete=False,
                 runs=[dict(status='pending_semantic', stdout='first\ufffd\n{"phase":"exit"}\n',
                            stderr='partial\udcff', events=events(), errors=['premier échec'])],
                 launch_intents=[dict(argv=['probe', 'input xyz'])], not_run=[])
    driver.save_report(path, value)
    original = path.read_bytes()
    need(driver.load_report(path) == value, 'gzip exact object roundtrip including escaped invalid bytes')
    need(gzip.decompress(original).decode() == json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+'\n',
         'all uncompressed JSON bytes retained')
    need(original[:4] == b'\x1f\x8b\x08\x00' and original[4:8] == b'\x00'*4 and original[8] == 4,
         'fast gzip header contains neither filename nor timestamp')
    driver.save_report(path, dict(reversed(list(value.items()))))
    need(path.read_bytes() == original, 'gzip deterministic across key insertion order and repeated saves')
    temporary = path.with_name(path.name+'.tmp')
    need(not temporary.exists(), 'complete checkpoint published by replace')
    def interrupted(_value, stream, **_kwargs):
        stream.write('{"partial":')
        raise KeyboardInterrupt
    with patch.object(driver.json, 'dump', side_effect=interrupted):
        try: driver.save_report(path, dict(value, complete=True))
        except KeyboardInterrupt: pass
        else: raise ValueError('checkpoint interruption swallowed')
    need(path.read_bytes() == original and driver.load_report(path) == value and temporary.exists(),
         'interrupted compression retains first complete checkpoint and partial temporary')
    replacement = dict(value, complete=True)
    with patch.object(Path, 'replace', side_effect=OSError('publication refused')):
        try: driver.save_report(path, replacement)
        except OSError: pass
        else: raise ValueError('checkpoint publication failure swallowed')
    need(path.read_bytes() == original and driver.load_report(path) == value and
         driver.load_report(temporary) == replacement, 'failed replace retains previous and complete temporary')
    damaged = (('header', b'not gzip'), ('truncated', original[:-1]),
               ('duplicate', gzip.compress(b'{"a":1,"a":2}', mtime=0)),
               ('nonfinite', gzip.compress(b'{"a":NaN}', mtime=0)),
               ('nonobject', gzip.compress(b'[]', mtime=0)))
    for name, data in damaged:
        bad = root/(name+'.json.gz'); bad.write_bytes(data)
        try: driver.load_report(bad)
        except (OSError, EOFError, ValueError): pass
        else: raise ValueError('damaged gzip/JSON accepted: '+name)
    driver.save_report(path, replacement)
    need(driver.load_report(path) == replacement and not temporary.exists(), 'subsequent complete save after failures')
    return 8+len(damaged)


def reused_campaign(root):
    """Real helper/decoder/measure across the full schedule; only native children and staging are mocked."""
    args, manifest, builds = environment(root, 'reuse'); args.budget_seconds = 550; args.reuse_semantic = True
    for i, case in enumerate(manifest['cases']):
        case.update(count=4, sha256=hashlib.sha256(('xyz%d' % i).encode()).hexdigest(),
                    ids_sha256=hashlib.sha256(('ids%d' % i).encode()).hexdigest())
    actual_decode = driver.profiles.semantic.decode
    actual_digest = driver.base.digest
    decode_calls = 0
    def decode(*arguments, **kwargs):
        nonlocal decode_calls
        decode_calls += 1
        return actual_decode(*arguments, **kwargs)
    def child(argv, **_kwargs):
        bits = next(b for b, build in builds.items() if str(build['path']) == argv[0])
        workers, mode = int(argv[10]), int(argv[11])
        need(argv[12] == '1', 'reuse retains requested diagnostics')
        Path(argv[3]).write_bytes(fixture(bits)[0])
        return subprocess.CompletedProcess(argv, 0,
            '\n'.join(json.dumps(e) for e in events(bits, workers, mode)).encode(), b'')
    with setup(manifest, builds), patch.object(driver.profiles.subprocess, 'run', side_effect=child), \
            patch.object(driver.profiles.semantic, 'decode', side_effect=decode), \
            patch.object(driver.base, 'digest', side_effect=lambda path: actual_digest(path) if path.suffix == '.bin' else 'b'*64):
        code = driver.run(args)
    report = driver.load_report(args.out/'adaptive.json.gz')
    need(code == 0 and report['conforming'] and len(report['runs']) == 36, 'whole reuse campaign conforming')
    counts = {mode: sum(row['semantic_reuse']['mode'] == mode for row in report['runs']) for mode in ('decoded', 'reused')}
    need(counts == {'decoded': 12, 'reused': 24} and decode_calls == 12, 'twelve unique complete decodes, not thirty-six')
    validated = set()
    for row in report['runs']:
        evidence = row['semantic_reuse']
        if evidence['mode'] == 'reused':
            need(tuple(evidence['source_attempt']) in validated and evidence['decode_wall_seconds'] == 0,
                 'reuse source is an earlier validated decode')
        else:
            need(evidence['source_attempt'] == evidence['current_attempt'], 'decoded origin is current attempt')
            validated.add(tuple(evidence['source_attempt']))
    need(report['semantic_reuse']['capacity'] == 64 and
         all(intent['semantic_reuse_requested'] for intent in report['launch_intents']), 'reuse declared before every launch')
    # A self-consistent payload/diagnostic stream still fails if the current worker echo differs.
    # This check belongs to the adaptive wrapper and must precede cache publication.
    args, _, _ = environment(root, 'reuse_wrong_worker'); args.work.mkdir()
    case = dict(name='badworker', coordinates='xyz', point_ids='ids', count=4, sha256='a'*64, ids_sha256='b'*64)
    cache = driver.reuse.SummaryCache()
    def wrong_child(argv, **_kwargs):
        Path(argv[3]).write_bytes(fixture(21)[0])
        return subprocess.CompletedProcess(argv, 0, '\n'.join(json.dumps(e) for e in events(21, 8, 7)).encode(), b'')
    request = dict(case='badworker', coord_bits=21, kmax=5, workers=48, repetition=0, optimizations=7, diagnostics=True)
    with patch.object(driver.profiles.subprocess, 'run', side_effect=wrong_child):
        row = driver.profiles.measure(root/'fake', case, 21, 5, args, workers=48, optimizations=7, diagnostics=True,
            semantic_cache=cache, validate_attempt=lambda value: driver.check_adaptive(value, request))
    need(row['status'] == 'invalid_output' and len(cache) == 0, 'wrapper rejection cannot publish cache origin')
    return len(report['runs']), counts['decoded'], counts['reused']


def main():
    positives, corruptions = structural()
    with tempfile.TemporaryDirectory(prefix='mhgp11_adaptive_collector_') as folder, contextlib.redirect_stdout(io.StringIO()):
        root = Path(folder)
        a, c, s, i, o = attempts(root), compare_controls(), schedules(root), interruptions(root), options(root)
        z = compressed_checkpoints(root)
        reused, decoded, hits = reused_campaign(root)
    print(json.dumps(dict(positives=positives, corruptions=corruptions, attempts=a, comparisons=c,
                          schedules=s, interrupted=i, options=o, gzip=z, reuse=reused, decoded=decoded, reused=hits,
                          checks=CHECKS, native=0), sort_keys=True))
    print('catalogue_adaptive_collector_verdict conforme positives%d corruptions%d attempts%d comparisons%d schedules%d interrupted%d options%d gzip%d reuse%d decoded%d reused%d native0' %
          (positives, corruptions, a, c, s, i, o, z, reused, decoded, hits))


if __name__ == '__main__':
    main()
