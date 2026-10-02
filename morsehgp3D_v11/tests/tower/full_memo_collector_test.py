#!/usr/bin/env python3
"""FULL memo ablation schedules and invariants with simulated processes only."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import sys
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'bench'))
import full_memo as driver
from full_campaign_test import arguments, events

CHECKS = 0


def need(value, why):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(why)


def setup(root, name):
    args = arguments(root,name)
    manifest = {'cases':[dict(name=case,count=count,coordinates='xyz',point_ids='ids',sha256='d'*64,ids_sha256='e'*64)
                         for case,count in driver.profiles.COUNTS.items()]}
    stack = contextlib.ExitStack()
    stack.enter_context(patch.object(driver.profiles,'checked_builds',return_value={
        bits:dict(path='fake%d' % bits) for bits in (18,21,24)}))
    stack.enter_context(patch.object(driver.profiles,'checked_supplement',return_value='c'*64))
    stack.enter_context(patch.object(driver.profiles,'inputs',return_value=(manifest,'a'*64)))
    stack.enter_context(patch.object(driver.base,'digest',return_value='b'*64))
    stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
    return args, stack


def row(request):
    return dict(request,status='ok',semantic=dict(sha256='a'*64,raw_sha256=str(request['coord_bits'])*32),
                events=events(optimizations=request['optimizations']))


def campaign(root, mode):
    args, stack = setup(root,mode)
    def measure(_exe,_case,request,call_args,checkpoint):
        need(call_args.optimizations == request['optimizations'], 'mode delivered unchanged')
        value = row(request)
        if mode == 'failed' and request['optimizations'] == 3:
            value['status'] = 'timeout'
        if request['optimizations'] == 7:
            if mode == 'different': value['semantic']['sha256'] = 'b'*64
            if mode == 'raw_different': value['semantic']['raw_sha256'] = 'c'*64
            if mode == 'work_different': value['events'][2]['orders'][0]['work']['traces'] += 1
            if mode == 'variable_work': value['events'][2]['orders'][0]['work']['census_point_tests'] += 1
        checkpoint(value)
        return value
    with stack:
        stack.enter_context(patch.object(driver.full,'measure',side_effect=measure))
        if mode == 'budget':
            clock = iter([0]+[100000]*100)
            stack.enter_context(patch.object(driver.time,'monotonic',side_effect=lambda: next(clock)))
        code = driver.run(args)
    report = json.loads((args.out/'full_memo.json').read_text())
    success = mode in ('ok','variable_work')
    need(code == (0 if success else 1) and report['conforming'] is success, 'verdict '+mode)
    need(report['complete'] and len(report['requested']) == 18, 'whole calendar')
    wanted = [driver.full.identity(r) for r in driver.schedule()]
    observed = [driver.full.identity(r) for name in ('runs','not_run') for r in report[name]]
    need(len(wanted) == len(set(wanted)) == 18 and sorted(wanted) == sorted(observed), 'inventory exact')
    need(len(report['launch_intents']) == len(report['runs']), 'intent per attempted process')
    if mode == 'budget':
        need(len(report['not_run']) == 18 and not report['runs'] and not report['full_schedule_completed'], 'budget only omissions')
    else:
        need(len(report['runs']) == 18 and not report['not_run'] and report['full_schedule_completed'], 'failure does not skip another mode')
    wanted_status = 'different' if mode in ('different','raw_different','work_different') else 'equal' if success else 'incomplete'
    need(all(c['status'] == wanted_status for c in report['comparisons']), 'comparison verdict')


def interrupted(root):
    args, stack = setup(root,'interrupted')
    def measure(_exe,_case,request,_args,checkpoint):
        report = json.loads((args.out/'full_memo.json').read_text())
        need(len(report['launch_intents']) == 1 and not report['runs'], 'intent before child')
        value = dict(request,status='pending_semantic',stdout='completed native output',events=[],errors=[])
        checkpoint(value)
        raise KeyboardInterrupt
    with stack, patch.object(driver.full,'measure',side_effect=measure):
        try: driver.run(args)
        except KeyboardInterrupt: pass
        else: raise ValueError('interruption swallowed')
    report = json.loads((args.out/'full_memo.json').read_text())
    need(not report['complete'] and not report['conforming'] and len(report['runs']) == 1, 'interruption checkpoint')
    need(report['runs'][0]['status'] == 'pending_semantic' and 'semantic' not in report['runs'][0], 'decoder not claimed')


def main():
    modes = ('ok','failed','different','raw_different','work_different','variable_work','budget')
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-memo-collector-') as folder:
        root = Path(folder)
        for mode in modes: campaign(root,mode)
        interrupted(root)
    wanted = driver.schedule()
    keys = {(r['case'],r['coord_bits']) for r in wanted}
    need(len(keys) == 9 and sum(r['case'].startswith('lidar') for r in wanted) == 12,'nine paired inputs')
    need(all(r['kmax'] == 5 and r['workers'] == 48 and r['repetition'] == 0 for r in wanted),'fixed scope')
    for key in keys:
        need([r['optimizations'] for r in wanted if (r['case'],r['coord_bits']) == key] == [3,7], 'paired option order')
    for rows in ([row(wanted[0]),row(wanted[0])], [dict(row(wanted[0]),case='alien')]):
        try: driver.comparisons(rows,wanted)
        except ValueError: need(True,'invalid inventory refused')
        else: raise ValueError('invalid inventory accepted')
    need(len(modes) == 7 and CHECKS >= 160,'coverage floor')
    print('full_memo_collector_verdict conforme schedules7 interruptions1 inventories2 checks%d native0' % CHECKS)


if __name__ == '__main__':
    main()
