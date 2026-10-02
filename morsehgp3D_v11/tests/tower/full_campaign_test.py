#!/usr/bin/env python3
"""FULL collection and campaign controls, simulated children only, no native execution."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from full_bench_semantic_test import encode, fixture
import full_campaign as driver

CHECKS = 0
VALUE = fixture([(0,0,0),(2,0,0),(4,0,0)],3)


def check(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def events(bits=21, kmax=3, workers=48, optimizations=0):
    orders = []
    for k,order in enumerate(VALUE['orders'],1):
        births, nodes = order['births'], len(order['nodes'])
        orders.append(dict(k=k,births=births,nodes=nodes,edges=nodes-1,verticals=nodes if k > 1 else 0,
                           node_capacity=2*births-1,edge_capacity=2*births-2,work=dict.fromkeys(driver.WORK,0),
                           timings=dict.fromkeys(driver.ORDER_TIMINGS,0)))
        orders[-1]['timings'].update(classify_ns=1,births_ns=1,plateaus_ns=1,verticals_ns=1 if k > 1 else 0)
        if k > 1:
            orders[-1]['work'].update(vertical_descents=births,vertical_checks=nodes-1,ancestor_queries=births+nodes-1)
    capacity = driver.MEMO_CAPACITY if optimizations & 4 else 0
    if capacity:
        for order in orders:
            work = order['work']
            queries = work['traces'] + work['vertical_descents']
            work.update(memo_queries=queries, memo_lookups=queries, memo_misses=queries,
                        memo_insertions=queries, descent_steps=queries, part_meb_presentations=queries)
    return [dict(phase='cloud',sites=3,points=3,read_ns=10,cloud_ns=20,cloud_peak_bytes=200),
            dict(phase='domain',index_ns=30,domain_ns=80,catalogue_balls=6,pool_ns=5,
                 sort_ns=10,count_ns=20,fill_ns=30),
            dict(phase='full',status='ok',reason='none',coord_bits=bits,kmax=kmax,workers=workers,optimizations=optimizations,
                 wall_ns=200,index_ns=30,domain_ns=80,forest_ns=50,cpu_seconds=0.000001,
                 peak_reserved_bytes=400+capacity*256,reserved_after_bytes=300,orders=orders,
                 memo_capacity=capacity,memo_slot_bytes=256,memo_reserved_bytes=capacity*256),
            dict(phase='exit',status='ok',reason='none')]


def request(name='test', bits=21, kmax=3, optimizations=0):
    return dict(case=name,coord_bits=bits,kmax=kmax,workers=48,repetition=0,optimizations=optimizations)


def arguments(root, name):
    return argparse.Namespace(out=root/name,work=root/(name+'_work'),data=root,builds=root/'builds',
                              qualification=root/'qualification',supplement=root/'supplement',budget_seconds=100000,
                              optimizations=0)


def event_mutations():
    return {
        'part_diameter_bound': lambda e: e[2]['orders'][1]['work'].update(
            descent_steps=1,part_meb_presentations=1,part_diameter_pairs=2),
        'trace_diameter_bound': lambda e: e[2]['orders'][1]['work'].update(
            trace_meb_calls=1,trace_meb_presentations=1,trace_diameter_pairs=2),
        'classification_diameter_bound': lambda e: e[2]['orders'][1]['work'].update(
            classification_meb_calls=1,classification_examined=1,classification_combinations=1,
            classification_meb_presentations=1,classification_diameter_pairs=2),
        'replay_diameter_bound': lambda e: e[2]['orders'][1]['work'].update(
            replay_meb_calls=1,replay_trace_tests=1,replay_meb_presentations=1,replay_diameter_pairs=2),
        'singleton_diameter_bound': lambda e: e[2]['orders'][0]['work'].update(
            descent_steps=1,part_meb_presentations=1,part_diameter_pairs=1),
        'part_diameter_overflow': lambda e: e[2]['orders'][1]['work'].update(part_diameter_pairs=1),
        'trace_diameter_overflow': lambda e: e[2]['orders'][1]['work'].update(trace_diameter_pairs=1),
        'classification_diameter_overflow': lambda e: e[2]['orders'][1]['work'].update(classification_diameter_pairs=1),
        'replay_diameter_overflow': lambda e: e[2]['orders'][1]['work'].update(replay_diameter_pairs=1),
        'part_candidate_missing': lambda e: e[2]['orders'][1]['work'].update(descent_steps=1),
        'trace_candidate_missing': lambda e: e[2]['orders'][1]['work'].update(trace_meb_calls=1),
        'order_time_missing': lambda e: e[2]['orders'][0].pop('timings'),
        'order_time_bool': lambda e: e[2]['orders'][0]['timings'].update(classify_ns=True),
        'order_time_negative': lambda e: e[2]['orders'][0]['timings'].update(classify_ns=-1),
        'order_time_sum': lambda e: e[2]['orders'][0]['timings'].update(classify_ns=50),
        'k1_vertical_time': lambda e: e[2]['orders'][0]['timings'].update(verticals_ns=1),
        'classification_subset': lambda e: e[2]['orders'][0]['work'].update(classification_examined=1),
        'classification_calls': lambda e: e[2]['orders'][0]['work'].update(classification_meb_calls=1),
        'replay_calls': lambda e: e[2]['orders'][0]['work'].update(replay_meb_calls=1),
        'old_ancestor_walk': lambda e: e[2]['orders'][1]['work'].update(ancestor_hops=1),
        'ancestor_query_count': lambda e: e[2]['orders'][1]['work'].update(ancestor_queries=0),
        'vertical_child_omitted': lambda e: e[2]['orders'][1]['work'].update(vertical_checks=0,ancestor_queries=2),
        'ancestor_extra_activation': lambda e: e[2]['orders'][1]['work'].update(ancestor_activations=2),
        'ancestor_extra_union': lambda e: e[2]['orders'][1]['work'].update(ancestor_unions=4),
        'bool': lambda e: e[0].update(sites=True),
        'nan': lambda e: e[2].update(cpu_seconds=float('nan')),
        'inf': lambda e: e[2].update(cpu_seconds=float('inf')),
        'cpu_bool': lambda e: e[2].update(cpu_seconds=True),
        'negative': lambda e: e[2].update(forest_ns=-1),
        'overflow': lambda e: e[2].update(wall_ns=2**64),
        'stages': lambda e: e[2].update(forest_ns=201),
        'sub_stages': lambda e: e[1].update(sort_ns=81),
        'duplicate_time': lambda e: e[1].update(index_ns=31),
        'profile': lambda e: e[2].update(coord_bits=24),
        'order': lambda e: e[2].update(kmax=2),
        'workers': lambda e: e[2].update(workers=1),
        'optimization_missing': lambda e: e[2].pop('optimizations'),
        'optimization_wrong': lambda e: e[2].update(optimizations=1),
        'optimization_bool': lambda e: e[2].update(optimizations=True),
        'optimization_large': lambda e: e[2].update(optimizations=8),
        'optimization_float': lambda e: e[2].update(optimizations=0.0),
        'optimization_negative': lambda e: e[2].update(optimizations=-1),
        'count': lambda e: e[0].update(points=2),
        'reason': lambda e: e[3].update(reason='no_error'),
        'full_reason': lambda e: e[2].update(reason='tower_invariant'),
        'verdict': lambda e: e[2].update(status='invalid_input'),
        'reserved': lambda e: e[2].update(reserved_after_bytes=401),
        'budget': lambda e: e[2].update(peak_reserved_bytes=driver.BUDGET+1),
        'cloud_budget': lambda e: e[0].update(cloud_peak_bytes=driver.BUDGET+1),
        'empty_catalogue': lambda e: e[1].update(catalogue_balls=0),
        'missing_order': lambda e: e[2]['orders'].pop(),
        'order_counts': lambda e: e[2]['orders'][0].update(nodes=3),
        'capacity_small': lambda e: e[2]['orders'][0].update(node_capacity=1),
        'capacity_large': lambda e: e[2]['orders'][0].update(edge_capacity=99),
        'work_bool': lambda e: e[2]['orders'][0]['work'].update(cells=True),
        'work_missing': lambda e: e[2]['orders'][0]['work'].pop('cells'),
        'phase': lambda e: e.pop(1),
    }


def attempts(root):
    args = arguments(root,'attempts'); args.work.mkdir()
    case = dict(name='test',count=3,coordinates='xyz',point_ids='ids')
    mutations = event_mutations()
    modes = ('ok','diameter_positive','slow','stderr','bad_json','duplicate_json','binary_log','bad_artifact','artifact_profile',
             'missing_artifact','refused','failed','signal','timeout','launch','cleanup',
             'opt0','opt1','opt2','opt3','opt4','opt5','opt6','opt7')+tuple(mutations)
    calls = 0
    for mode in modes:
        optimization = int(mode[-1]) if mode.startswith('opt') and len(mode) == 4 else 0
        args.optimizations = optimization
        checkpoints = []
        def child(argv, **kwargs):
            nonlocal calls
            calls += 1
            check(kwargs['timeout'] == driver.TIMEOUT and kwargs['check'] is False, 'bounded child')
            check(argv[4:] == ['3','16','256','0',str(2**32-1),str(driver.BUDGET),'48']+
                  ([str(optimization)] if optimization else []), 'whole FULL invocation')
            if mode == 'launch':
                raise OSError('unavailable')
            if mode == 'timeout':
                raise subprocess.TimeoutExpired(argv,driver.TIMEOUT,output=b'{"phase":',stderr=b'partial\xff')
            values = events(optimizations=optimization)
            if mode in mutations:
                mutations[mode](values)
            if mode == 'diameter_positive':
                values[2]['orders'][1]['work'].update(
                    descent_steps=2,part_meb_presentations=2,part_diameter_pairs=2,
                    trace_meb_calls=1,trace_meb_presentations=1,trace_diameter_pairs=1,
                    classification_meb_calls=1,classification_examined=1,classification_combinations=1,
                    classification_meb_presentations=1,classification_diameter_pairs=1,
                    replay_meb_calls=1,replay_trace_tests=1,replay_meb_presentations=1,replay_diameter_pairs=1)
            if mode == 'slow':
                values[2]['wall_ns'] = 200_000_001
            payload = '\n'.join(json.dumps(e) for e in values).encode()
            if mode in ('bad_json','refused','failed','signal'):
                payload += b'\n{"phase":'
            if mode == 'duplicate_json':
                payload += b'\n{"phase":"exit","phase":"exit"}'
            if mode == 'binary_log':
                payload += b'\n\xff'
            raw,_ = encode(VALUE,24 if mode == 'artifact_profile' else 21)
            if mode != 'missing_artifact':
                Path(argv[3]).write_bytes(raw[:-1] if mode == 'bad_artifact' else raw)
            return subprocess.CompletedProcess(argv,{'refused':2,'failed':3,'signal':-15}.get(mode,0),
                                               payload,b'warning' if mode == 'stderr' else b'')
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(driver.subprocess,'run',side_effect=child))
            if mode == 'cleanup':
                stack.enter_context(patch.object(Path,'unlink',side_effect=OSError('cleanup failed')))
            result = driver.measure(root/'fake',case,request(optimizations=optimization),args,
                                    lambda row: checkpoints.append(copy.deepcopy(row)))
        wanted = {'ok':'ok','diameter_positive':'ok','slow':'ok','refused':'refused','failed':'failed','signal':'failed',
                  'timeout':'timeout','launch':'launch_error','cleanup':'artifact_error',
                  **{'opt'+str(n):'ok' for n in range(8)}}.get(mode,'invalid_output')
        check(result['status'] == wanted, mode+': wrong verdict '+result['status'])
        check(len(checkpoints) == 1 and checkpoints[0]['stdout'] == result['stdout'], 'single process checkpoint')
        check(result['case'] == 'test' and result['count'] == 3 and result['whole_input'], 'whole identity')
        check(result['optimizations'] == checkpoints[0]['optimizations'] == optimization,'row optimization preserved')
        check(result['process_wall_seconds'] >= 0, 'process duration present')
        if wanted == 'ok':
            check(result['semantic']['nodes'] == 8 and len(result['semantic']['sha256']) == 64, 'real artifact decoded')
            check(result['full_within_200ms'] is (mode != 'slow'), 'quality and target separated')
            check(result['whole_peak_reserved_bytes'] == events(optimizations=optimization)[2]['peak_reserved_bytes'] and result['cloud_pool_full_ms'] > result['full_ms'], 'scope arithmetic')
        else:
            check(result['errors'], 'first verdict and collection error retained')
        if wanted in ('refused','failed','timeout'):
            check(result['stdout'] and result['errors'][-1]['stage'] == 'events', 'malformed failure log retained')
        if mode != 'cleanup':
            check(not Path(result['argv'][3]).exists(), 'artifact removed')
        else:
            Path(result['argv'][3]).unlink()
    check(calls == len(modes), 'attempt count')
    return calls


def campaign(root, mode, optimization=0):
    args = arguments(root,'run_'+mode+'_o'+str(optimization)); args.optimizations = optimization
    manifest = {'cases':[dict(name=name,count=count,coordinates='xyz',point_ids='ids',sha256='d'*64,ids_sha256='e'*64)
                         for name,count in driver.profiles.COUNTS.items()]}
    builds = {bits:dict(path='fake%d' % bits) for bits in (18,21,24)}
    first = driver.schedule()[0]['case']
    def measure(_exe, case, req, _args, checkpoint):
        row = dict(req,status='ok',semantic=dict(sha256='a'*64,raw_sha256=str(req['coord_bits'])*32),
                   events=events(optimizations=optimization))
        if mode in ('one_failed','incomplete_different') and req['case'] == first and req['coord_bits'] == 21 and req['repetition'] == 0:
            row['status'] = 'timeout'
        if mode in ('different','incomplete_different') and req['coord_bits'] == 24 and req['repetition'] == 2:
            row['semantic']['sha256'] = 'b'*64
        if mode == 'raw_different' and req['coord_bits'] == 24 and req['repetition'] == 2:
            row['semantic']['raw_sha256'] = 'c'*64
        if mode == 'work_different' and req['coord_bits'] == 24:
            row['events'][2]['orders'][0]['work']['cells'] = 1
        if mode == 'repeat_failed' and req['case'] == first and req['coord_bits'] == 21 and req['repetition'] == 1:
            row['status'] = 'failed'
        checkpoint(row)
        return row
    clock = iter([0]+[100000]*100) if mode == 'budget' else None
    with contextlib.ExitStack() as stack:
        stack.enter_context(patch.object(driver.profiles,'checked_builds',return_value=builds))
        stack.enter_context(patch.object(driver.profiles,'checked_supplement',return_value='c'*64))
        stack.enter_context(patch.object(driver.profiles,'inputs',return_value=(manifest,'a'*64)))
        stack.enter_context(patch.object(driver.base,'digest',return_value='b'*64))
        stack.enter_context(patch.object(driver,'measure',side_effect=measure))
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        if clock is not None:
            stack.enter_context(patch.object(driver.time,'monotonic',side_effect=lambda: next(clock)))
        code = driver.run(args)
    report = json.loads((args.out/'full.json').read_text())
    check(code == (0 if mode == 'ok' else 1) and report['conforming'] is (mode == 'ok'), 'schedule verdict '+mode)
    requested = [driver.identity(r) for r in report['requested']]
    observed = [driver.identity(r) for key in ('runs','not_run') for r in report[key]]
    check(len(requested) == len(set(requested)) == 24 and sorted(requested) == sorted(observed), '24 exact units')
    check(report['complete'] and len(report['launch_intents']) == len(report['runs']), 'intent and final inventory')
    check(report['schema'] == 'ehgp.v11.full_campaign.v5' and report['optimizations'] == optimization and
          all(r['optimizations'] == optimization for key in ('requested','runs','not_run','launch_intents','comparisons')
              for r in report[key]),'campaign mode identity')
    check(all(len(r['argv']) == (12 if optimization else 11) and
              (r['argv'][-1] == str(optimization) if optimization else r['argv'][-1] == '48')
              for r in report['launch_intents']),'mode reaches native command')
    if mode in ('one_failed','incomplete_different'):
        check(len(report['runs']) == 21 and len(report['not_run']) == 3, 'causal omissions count')
        check(all(r['case'] == first and r['coord_bits'] == 21 and r['reason'] == 'same_profile_K5_first_attempt_failed'
                  for r in report['not_run']), 'same profile and scene only')
    elif mode == 'budget':
        check(not report['runs'] and len(report['not_run']) == 24 and not report['full_schedule_completed'], 'budget omissions')
    else:
        check(len(report['runs']) == 24 and not report['not_run'] and report['full_schedule_completed'], 'all units attempted')
    if mode in ('different','raw_different','work_different','incomplete_different'):
        relevant = [c for c in report['comparisons'] if c['kmax'] == 5]
        check(all(c['status'] == 'different' for c in relevant), 'disagreement stronger than incompleteness')


def interrupted(root):
    args = arguments(root,'interrupted')
    one = request('tiny')
    manifest = {'cases':[dict(name='tiny',count=3,coordinates='xyz',point_ids='ids',sha256='d'*64,ids_sha256='e'*64)]}
    def child(argv, **_kwargs):
        Path(argv[3]).write_bytes(encode(VALUE,21)[0])
        return subprocess.CompletedProcess(argv,0,'\n'.join(json.dumps(e) for e in events()).encode(),b'')
    with patch.object(driver.profiles,'checked_builds',return_value={21:dict(path='fake')}), \
            patch.object(driver.profiles,'checked_supplement',return_value='c'*64), \
            patch.object(driver.profiles,'inputs',return_value=(manifest,'a'*64)), \
            patch.object(driver.base,'digest',return_value='b'*64), \
            patch.object(driver,'schedule',return_value=[one]), \
            patch.object(driver.subprocess,'run',side_effect=child), \
            patch.object(driver.semantic,'inspect',side_effect=KeyboardInterrupt):
        try:
            driver.run(args)
        except KeyboardInterrupt:
            pass
        else:
            raise ValueError('semantic interruption swallowed')
    report = json.loads((args.out/'full.json').read_text())
    check(not report['complete'] and not report['conforming'] and not report['full_schedule_completed'], 'interruption not complete')
    check(len(report['runs']) == len(report['launch_intents']) == 1, 'interrupted attempt once')
    row = report['runs'][0]
    check(row['status'] == 'pending_semantic' and row['events'] == events() and row['exit_code'] == 0, 'process captured before semantic')
    check('semantic' not in row and row['stdout'] and Path(row['argv'][3]).exists(), 'unfinished artifact retained')


def invalid_modes(root):
    for value in (True,False,-1,8,0.0,None,'1'):
        args = arguments(root,'invalid'); args.optimizations = value
        try:
            driver.run(args)
        except ValueError:
            check(not args.out.exists() and not args.work.exists(),'invalid mode before IO')
        else:
            raise ValueError('invalid mode accepted')
    args = arguments(root,'mismatch')
    with patch.object(driver.subprocess,'run') as child:
        try:
            driver.measure(root/'fake',{},request(optimizations=1),args,lambda row: None)
        except ValueError:
            check(not child.called,'request/campaign mismatch before launch')
        else:
            raise ValueError('request mismatch accepted')



def memo_diagnostics(root):
    args = arguments(root, 'memo'); args.optimizations = 7; args.work.mkdir()
    case = dict(name='test',count=3,coordinates='xyz',point_ids='ids')
    mutations = {
        'memo reservations': lambda e: e[2].update(memo_reserved_bytes=0),
        'memo table coexists': lambda e: e[2].update(peak_reserved_bytes=e[2]['memo_reserved_bytes']),
        'memo query inventory': lambda e: e[2]['orders'][1]['work'].update(memo_queries=0),
        'memo lookups and actual steps': lambda e: e[2]['orders'][1]['work'].update(memo_lookups=0),
        'memo hit inventory': lambda e: e[2]['orders'][1]['work'].update(memo_suffix_hits=1),
        'memo publications': lambda e: e[2]['orders'][1]['work'].update(memo_insertions=0),
        'memo replacement work': lambda e: e[2]['orders'][1]['work'].update(memo_collisions=3),
    }
    # Positive initial and suffix hits pay only their actually executed steps.
    initial = events(optimizations=7)
    initial[2]['orders'][1]['work'].update(memo_lookups=2,memo_hits=2,memo_misses=0,
        memo_insertions=0,descent_steps=0,part_meb_presentations=0)
    suffix = events(optimizations=7)
    suffix[2]['orders'][1]['work'].update(memo_lookups=3,memo_hits=1,memo_suffix_hits=1)
    for value in (initial, suffix):
        driver.check_order_diagnostics(value[2]); check(True, 'valid memo hits')
    for reason, mutate in mutations.items():
        values = events(optimizations=7); mutate(values)
        raw, _ = encode(VALUE,21)
        def child(argv, **_kwargs):
            Path(argv[3]).write_bytes(raw)
            return subprocess.CompletedProcess(argv,0,'\n'.join(json.dumps(e) for e in values).encode(),b'')
        with patch.object(driver.subprocess,'run',side_effect=child):
            result = driver.measure(root/'fake',case,request(optimizations=7),args,lambda _r: None)
        check(result['status'] == 'invalid_output' and result['errors'], 'memo event corruption: '+reason)
    disabled = events()
    disabled[2]['orders'][0]['work']['memo_lookups'] = 1
    try: driver.check_order_diagnostics(disabled[2])
    except ValueError: check(True, 'disabled memo work rejected')
    else: raise ValueError('disabled memo accepted work')


def main():
    modes = ('ok','one_failed','different','raw_different','work_different','incomplete_different','repeat_failed','budget')
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-collector-') as directory:
        root = Path(directory)
        count = attempts(root)
        for mode in modes:
            campaign(root,mode)
        for optimization in range(1,8):
            campaign(root,'ok',optimization)
        interrupted(root)
        invalid_modes(root)
        memo_diagnostics(root)
    check(count == 81 and len(modes) == 8 and CHECKS >= 450, 'collector floors')
    print('full_campaign_verdict conforme attempts%d schedules%d interrupted1 checks%d native0' % (count,len(modes)+7,CHECKS))


if __name__ == '__main__':
    main()
