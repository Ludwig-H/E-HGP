#!/usr/bin/env python3
"""Exact optimization campaign: simulated children and real parsers, no native execution."""
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from bench_parallel_collector_test import native_events, ready as historical_ready, environment, setup
from bench_semantic_test import fixture
import catalogue_optimizations as driver

CHECKS = 0


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def events(bits, workers, mode):
    values = native_events(bits,workers)
    cache = dict(evaluations=3,hits=4,fallbacks=1) if mode & 1 else dict(evaluations=7,hits=0,fallbacks=0)
    values[1].update(optimizations=mode,cache_work=cache)
    values[1]['timings']['sort_comparisons'] = 2 if mode & 2 else 1
    return values


def ready(request):
    result = historical_ready(request)
    result.update(stderr='',errors=[])
    result['events'] = events(request['coord_bits'],request['workers'],request['optimizations'])
    return result


def attempts(root):
    args, _, _ = environment(root,'attempts'); args.work.mkdir(); args.budget_seconds = 700
    case = dict(name='fake',coordinates='xyz',point_ids='ids',count=4)
    modes = ('mode0','mode1','mode2','mode3','other_profile','opt_wrong','opt_bool','opt_missing',
             'cache_missing','cache_extra','cache_notdict','cache_bool','cache_negative','cache_overflow',
             'requests','fallbacks','off_hits','off_fallbacks','workers','timings','stderr',
             'bad_json','damaged','missing','timeout','launch','refused','failed','signal','cleanup')
    calls = 0
    for mode in modes:
        optimization = int(mode[-1]) if mode.startswith('mode') else 2 if mode.startswith('off_') else 3
        bits, workers = (24,8) if mode == 'other_profile' else (21,48)
        request = dict(case='fake',coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=optimization)
        checkpoints = []

        def child(argv, **kwargs):
            nonlocal calls
            calls += 1
            check(len(argv) == (12 if optimization else 11) and argv[10] == str(workers), 'native option arity')
            check((argv[-1] == str(optimization)) if optimization else argv[-1] == str(workers), 'native option value')
            check(kwargs['timeout'] == 15 and kwargs['check'] is False, 'bounded native attempt')
            if mode == 'launch':
                raise OSError('fake launch')
            if mode == 'timeout':
                raise subprocess.TimeoutExpired(argv,15,output=b'{"phase":',stderr=b'partial\xff')
            values = events(bits,workers,optimization); event = values[1]
            if mode == 'opt_wrong': event['optimizations'] = 0
            if mode == 'opt_bool': event['optimizations'] = True
            if mode == 'opt_missing': del event['optimizations']
            if mode == 'cache_missing': del event['cache_work']['evaluations']
            if mode == 'cache_extra': event['cache_work']['extra'] = 0
            if mode == 'cache_notdict': event['cache_work'] = []
            if mode in ('cache_bool','cache_negative','cache_overflow'):
                event['cache_work']['evaluations'] = {'cache_bool':True,'cache_negative':-1,'cache_overflow':2**64}[mode]
            if mode == 'requests': event['cache_work']['hits'] += 1
            if mode == 'fallbacks': event['cache_work']['fallbacks'] = 4
            if mode == 'off_hits': event['cache_work'].update(evaluations=6,hits=1)
            if mode == 'off_fallbacks': event['cache_work']['fallbacks'] = 1
            if mode == 'workers': event['workers'] = 1
            if mode == 'timings': event['timings']['sort_comparisons'] = 1000
            payload = '\n'.join(json.dumps(e) for e in values).encode()
            if mode in ('bad_json','refused','failed','signal'):
                payload += b'\n{"phase":'
            if mode != 'missing':
                raw,_ = fixture(bits)
                Path(argv[3]).write_bytes(raw[:-1] if mode == 'damaged' else raw)
            return subprocess.CompletedProcess(argv,{'refused':2,'failed':3,'signal':-15}.get(mode,0),payload,
                                               b'warning' if mode == 'stderr' else b'')

        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(driver.profiles.subprocess,'run',side_effect=child))
            if mode == 'cleanup':
                stack.enter_context(patch.object(Path,'unlink',side_effect=OSError('cleanup failed')))
            row = driver.profiles.measure(root/'fake',case,bits,5,args,
                lambda r: checkpoints.append(copy.deepcopy(r)),workers=workers,timeout=15,optimizations=optimization)
        driver.check_optimization(row,request)
        wanted = ('ok' if mode.startswith('mode') or mode == 'other_profile' else
                  {'damaged':'artifact_error','missing':'artifact_error','cleanup':'artifact_error',
                   'timeout':'timeout','launch':'launch_error','refused':'refused','failed':'failed','signal':'failed'
                  }.get(mode,'invalid_output'))
        check(row['status'] == wanted, mode+': verdict '+row['status'])
        check(driver.identity(row) == driver.identity(request), 'full identity retained')
        check(len(checkpoints) == 1 and row['stdout'] == checkpoints[0]['stdout'], 'single process checkpoint')
        if wanted == 'ok':
            check(row['cache_work'] == row['events'][1]['cache_work'] and row['semantic']['balls'] == 11, 'cache and real artifact')
            check(checkpoints[0]['status'] == 'pending_semantic', 'checkpoint precedes decoder')
        else:
            check(row['errors'], 'failure details retained')
        if wanted in ('refused','failed','timeout'):
            check(row['stdout'] and row['errors'][-1]['stage'] == 'events', 'malformed failure stream remains failure')
        if mode == 'cleanup': Path(row['argv'][3]).unlink()
        else: check(not Path(row['argv'][3]).exists(), 'artifact cleanup')
    check(calls == len(modes), 'attempt inventory')
    return calls


def compare_controls():
    requested = driver.schedule(); rows = [ready(r) for r in requested]
    check(len(requested) == len(set(map(driver.identity,requested))) == 36, '36 identities including mode')
    check([r['optimizations'] for r in requested[:4]] == [0,2,1,3], 'mode order')
    check(len([r for r in requested if r['workers'] == 8]) == 6 and
          all(r['optimizations'] == 3 for r in requested[24:]), 'W8 and growth mode3')
    check(all(r['kmax'] == 5 and r['repetition'] == 0 for r in requested), 'K5 one invocation')
    check(all(c['status'] == 'equal' for c in driver.comparisons(rows,requested)), 'positive comparison groups')
    corruptions = 0
    for kind in ('semantic','raw','logical','q4','duplicate','unknown_mode','bool_mode'):
        changed = copy.deepcopy(rows)
        if kind == 'semantic': changed[1]['semantic']['sha256'] = 'b'*64
        if kind == 'raw': changed[1]['canonical_sha256'] = 'b'*64
        if kind == 'logical': changed[1]['events'][1]['logical']['nodes'] += 1
        if kind == 'q4': changed[1]['events'][1]['work']['q4_candidates'] += 1
        if kind == 'duplicate': changed.append(changed[0])
        if kind == 'unknown_mode': changed[0]['optimizations'] = 4
        if kind == 'bool_mode': changed[0]['optimizations'] = False
        if kind in ('duplicate','unknown_mode','bool_mode'):
            try: driver.comparisons(changed,requested)
            except ValueError: corruptions += 1
            else: raise ValueError('invalid comparison inventory accepted')
        else:
            check(driver.comparisons(changed,requested)[0]['status'] == 'different', 'observed divergence')
            changed[0]['status'] = 'timeout'
            check(driver.comparisons(changed,requested)[0]['status'] == 'different', 'incomplete does not hide divergence')
            corruptions += 1
    bad = copy.deepcopy(rows[0]); bad['optimizations'] = 1
    try: driver.check_optimization(bad,requested[0])
    except ValueError: corruptions += 1
    else: raise ValueError('attempt mode substitution accepted')
    return corruptions


def campaigns(root):
    modes = ('ok','baseline_failed','cache_failed','all_failed','different','incomplete_different','cache_stats','deadline','no_budget')
    for mode in modes:
        args, manifest, builds = environment(root,mode); args.budget_seconds = 700
        elapsed, ticks = 0, 0

        def clock():
            nonlocal ticks
            ticks += 1
            return 700 if mode == 'no_budget' and ticks > 1 else elapsed

        def measure(_exe,case,bits,kmax,_args,checkpoint,*,workers,repetition,timeout,optimizations):
            nonlocal elapsed
            check(timeout == 15, 'campaign child deadline')
            request = dict(case=case['name'],coord_bits=bits,kmax=kmax,workers=workers,repetition=repetition,
                           optimizations=optimizations)
            row = ready(request)
            pending = copy.deepcopy(row); pending['status'] = 'pending_semantic'; checkpoint(pending)
            target = case['name'] == 'lidar_ng00' and bits == 21 and workers == 48
            if mode == 'all_failed' or (target and ((mode in ('baseline_failed','incomplete_different') and optimizations == 0)
                                                  or (mode == 'cache_failed' and optimizations == 1))):
                row['status'] = 'timeout'
            if target and optimizations == 2 and mode in ('different','incomplete_different'):
                row['semantic']['sha256'] = 'b'*64
            if mode == 'cache_stats' and optimizations == 3:
                row['events'][1]['cache_work'].update(evaluations=4,hits=3)
            if mode == 'deadline': elapsed += 100
            return row

        with setup(manifest,builds), patch.object(driver.profiles,'measure',side_effect=measure), \
                patch.object(driver.time,'monotonic',side_effect=clock):
            code = driver.run(args)
        report = json.loads((args.out/'optimizations.json').read_text())
        success = mode in ('ok','cache_stats')
        check(code == (0 if success else 1) and report['conforming'] is success, 'campaign verdict '+mode)
        actual = [driver.identity(r) for r in report['runs']+report['not_run']]
        check(len(actual) == len(set(actual)) == 36 and set(actual) == set(map(driver.identity,driver.schedule())), 'complete unit inventory')
        check(list(map(driver.identity,report['launch_intents'])) == list(map(driver.identity,report['runs'])), 'launch intent identity')
        check(report['complete'] and report['schema'] == driver.SCHEMA and report['native_schedule_bound_seconds'] == 540, 'report contract')
        if mode in ('deadline','no_budget'):
            check(len(report['runs']) == (7 if mode == 'deadline' else 0), 'budget before launch')
            check(all(r['reason'] == 'campaign_budget_before_launch' for r in report['not_run']), 'explicit omission cause')
        else:
            check(len(report['runs']) == 36 and not report['not_run'], 'failed mode never suppresses another')
        if mode in ('different','incomplete_different'):
            check(report['comparisons'][0]['status'] == 'different', 'reported disagreement')
    return len(modes)


def interruptions(root):
    for when in ('launch','decoder'):
        args, manifest, builds = environment(root,'interrupted_'+when); args.budget_seconds = 700
        def child(argv, **_kwargs):
            report = json.loads((args.out/'optimizations.json').read_text())
            check(not report['runs'] and len(report['launch_intents']) == 1, 'intent before process')
            check(report['launch_intents'][0]['optimizations'] == 0 and report['launch_intents'][0]['argv'] == argv,
                  'intent exact optimization argv')
            if when == 'launch': raise KeyboardInterrupt
            Path(argv[3]).write_bytes(fixture(21)[0])
            payload = '\n'.join(json.dumps(e) for e in events(21,48,0)).encode()
            return subprocess.CompletedProcess(argv,0,payload,b'')
        with setup(manifest,builds), patch.object(driver.profiles.subprocess,'run',side_effect=child), \
                patch.object(driver.profiles.semantic,'inspect',side_effect=KeyboardInterrupt):
            try: driver.run(args)
            except KeyboardInterrupt: pass
            else: raise ValueError('interruption swallowed')
        report = json.loads((args.out/'optimizations.json').read_text())
        check(not report['complete'] and not report['conforming'], 'interruption never green')
        check(len(report['runs']) == (1 if when == 'decoder' else 0), 'checkpoint stage inventory')
        if when == 'decoder':
            row = report['runs'][0]
            check(row['status'] == 'pending_semantic' and row['exit_code'] == 0 and row['optimizations'] == 0,
                  'native result retained before semantic')
            check('semantic' not in row and Path(row['argv'][3]).exists(), 'unfinished artifact retained')
    return 2


def main():
    with tempfile.TemporaryDirectory(prefix='mhgp11-optimizations-') as directory:
        root = Path(directory)
        count = attempts(root)
        corruptions = compare_controls()
        schedules = campaigns(root)
        interrupted = interruptions(root)
    check((count,corruptions,schedules,interrupted) == (30,8,9,2) and CHECKS >= 500, 'collector floors')
    print('optimizations_verdict conforme attempts30 comparisons8 schedules9 interrupted2 checks%d native0' % CHECKS)


if __name__ == '__main__':
    main()
