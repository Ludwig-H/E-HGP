#!/usr/bin/env python3
"""Census FULL ablation: real tiny artifacts and mocked processes, never native execution."""
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
    # Synthetic paid-work counts: the same requests have one traversal or two.
    for order in result[2]['orders']:
        order['work']['census_point_tests'] *= 1 if req['optimizations'] & 256 else 2
    return result


def corruptions():
    result = {
        'mode_missing': lambda e: e[2].pop('reuse_census_workspace'),
        'mode_false': lambda e: e[2].update(reuse_census_workspace=False),
        'mode_integer': lambda e: e[2].update(reuse_census_workspace=1),
        'mode_null': lambda e: e[2].update(reuse_census_workspace=None),
        'count_zero': lambda e: e[2].update(census_workspaces=0),
        'count_49': lambda e: e[2].update(census_workspaces=49),
        'bytes_short': lambda e: e[2].update(census_workspace_reserved_bytes=959),
        'bytes_large': lambda e: e[2].update(census_workspace_reserved_bytes=961),
        'owned_instead': lambda e: e[2].update(census_workspaces=0,census_workspace_reserved_bytes=0),
        'peak_excludes_scratch': lambda e: e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']+
            e[2]['memo_reserved_bytes']+e[2]['parallel']['lane_memo_reserved_bytes']),
        'peak_one_short': lambda e: e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']+
            e[2]['memo_reserved_bytes']+e[2]['parallel']['lane_memo_reserved_bytes']+
            e[2]['census_workspace_reserved_bytes']-1),
    }
    for field in ('census_workspaces','census_workspace_reserved_bytes'):
        for name, value in (('bool',True),('negative',-1),('overflow',2**64),('float',48.0)):
            result[name+'_'+field] = lambda e, f=field,v=value: e[2].update({f:v})
        result['missing_'+field] = lambda e, f=field: e[2].pop(f)
    return result


def physical_slots():
    # Direct arithmetic boundary tests, not invented geometric FULL results.
    for n in (13,41,2**32-2):
        for k in (1,7,12):
            for mode in (0,4,8,12,256,260,264,268):
                for workers in (1,8,48,64,256):
                    value = stream(fixtures.request(mode=mode,workers=workers))[2]
                    value['kmax'] = k
                    count = 0 if mode < 256 else 1 if not mode & 8 else min(workers,48)
                    value.update(census_workspaces=count,census_workspace_reserved_bytes=4*n*count)
                    value['peak_reserved_bytes'] = value['reserved_after_bytes']+value['memo_reserved_bytes']+\
                        value['parallel']['lane_memo_reserved_bytes']+4*n*count
                    full.workspace.validate(value,n,need,full.unsigned)
                    changed = copy.deepcopy(value)
                    changed['census_workspace_reserved_bytes'] += 1
                    try:
                        full.workspace.validate(changed,n,full.need,full.unsigned)
                    except ValueError:
                        COUNTS['corruptions'] += 1
                    else:
                        raise ValueError('incorrect physical reservation accepted')
    need(len([i for i in range(512) if not i & 128 or i & 8]) == 384,'full mode inventory')


def attempts(root):
    harness = fixtures.Attempts(root)
    def run(mode=511, workers=48, bits=21, **options):
        return harness.run(fixtures.request(mode=mode,workers=workers,bits=bits),**options)
    for mode in (0,7,15,127,255,256,260,264,268,392,511):
        for bits in (18,21,24):
            for workers in (1,8,48):
                row = run(mode,workers,bits)
                need(row['status']=='ok' and row['semantic']['kmax']==5,'real FULL payload')
                event = row['events'][2]
                count = 0 if mode < 256 else 1 if not mode & 8 else min(workers,48)
                need(event['census_workspaces']==count and event['census_workspace_reserved_bytes']==20*count,
                     'spaces depend on physical concurrency, not K or all lanes')
                if not mode & 4:
                    need(event['memo_reserved_bytes']==event['parallel']['lane_memo_reserved_bytes']==0 and
                         all(o['work']['memo_queries']==0 for o in event['orders']),'workspace without memo')
    row = run(mutation=lambda e:e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']+
        e[2]['memo_reserved_bytes']+e[2]['parallel']['lane_memo_reserved_bytes']+e[2]['census_workspace_reserved_bytes']))
    need(row['status']=='ok','coexistence at exact integer boundary')
    for name,mutate in corruptions().items():
        row = run(mutation=mutate)
        need(row['status']=='invalid_output' and 'semantic' not in row and row['stdout'],name)
        COUNTS['corruptions'] += 1
    for mode,mutate in ((255,lambda e:e[2].update(reuse_census_workspace=True)),
                       (255,lambda e:e[2].update(census_workspaces=1)),
                       (255,lambda e:e[2].update(census_workspace_reserved_bytes=20)),
                       (256,lambda e:e[2].update(census_workspaces=48,census_workspace_reserved_bytes=960))):
        row = run(mode,mutation=mutate)
        need(row['status']=='invalid_output','disabled or serial workspace route enforced')
        COUNTS['corruptions'] += 1
    row = run(workers=1,mutation=lambda e:e[2].update(census_workspaces=48,census_workspace_reserved_bytes=960))
    need(row['status']=='invalid_output','W1 needs one space despite48 logical lanes'); COUNTS['corruptions'] += 1
    for process in ('timeout','launch','refused','failed'):
        empty = full.reuse.SummaryCache(); row = run(cache=empty,process=process)
        need(row['status']==('launch_error' if process=='launch' else process) and len(empty)==0 and
             'semantic' not in row,'process failure never seeds cache')
    cache = full.reuse.SummaryCache()
    first = run(255,cache=cache); previous = copy.deepcopy(first)
    before = fixtures.COUNTS['decodes']; second = run(511,cache=cache)
    need(second['status']=='ok' and second['semantic_reuse']['mode']=='reused' and
         fixtures.COUNTS['decodes']==before,'raw geometry identity reused across census routes')
    for mutate in (corruptions()['count_zero'],corruptions()['peak_one_short']):
        row = run(cache=cache,mutation=mutate)
        need(row['status']=='invalid_output' and 'semantic_reuse' not in row and len(cache)==1,
             'workspace fields judged before any cache hit')
        empty = full.reuse.SummaryCache(); row = run(cache=empty,mutation=mutate)
        need(row['status']=='invalid_output' and len(empty)==0,'failed origin never published')
    wrong = run(cache=cache,mutation=lambda e:e[2]['orders'][0].update(nodes=8))
    need(wrong['status']=='invalid_output' and wrong['semantic_reuse']['mode']=='reused',
         'current structural counts checked after reuse')
    changed = run(cache=cache,raw_suffix=b'changed')
    need(changed['status']=='invalid_output' and len(cache)==1 and first==previous,'new raw bytes decoded, no alias')


def calendar():
    def row(name,bits,mode,workers=48):
        return dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected = [row(name,bits,mode) for name in ('lidar_ng00','lidar_ng01','lidar_ng02')
                for bits in (21,24) for mode in (127,255,511)]
    expected += [row('lidar_ng00',21,511,w) for w in (1,8)]
    expected += [row(name,21,mode) for name in ('uniform_u18_n8000','uniform_u18_n16000','uniform_u18_n32000')
                 for mode in (127,255,511)]
    need(driver.schedule(reuse_census=True)==expected and len(set(map(full.identity,expected)))==29,'calendar29')
    need(len(driver.schedule())==19 and len(driver.schedule(True))==27 and
         len(driver.schedule(parallel_verticals=True))==20,'historical calendars retained')
    for values in ((True,False,True),(False,True,True),(True,True,False),(False,False,1),(False,False,None)):
        try: driver.schedule(*values)
        except ValueError: need(True,'exclusive strict calendar options')
        else: raise ValueError('invalid calendar accepted')
    plan = json.loads((Path(__file__).resolve().parents[2]/'bench/plans/full_census_g4.json').read_text())
    need([r['timeout_seconds'] for r in plan['commands']]==[850,180,570] and
         sum(r['timeout_seconds'] for r in plan['commands'])+120==1720,'declared commands plus setup120')
    command = plan['commands'][2]
    need(command['name']=='census_full' and command['argv'][-4:]==['--budget-seconds','500','--reuse-semantic','--reuse-census']
         and plan['results_cap_bytes']==16*1024**2,'guarded fixed plan')


def campaign(root, scenario):
    COUNTS['schedules'] += 1
    args = arguments(root,'campaign_'+scenario)
    args.budget_seconds=500; args.reuse_semantic=True; args.reuse_census=True
    manifest = dict(cases=[dict(fixtures.CASE,name=name) for name in driver.profiles.COUNTS])
    original_measure,original_decode = full.measure,full.semantic.decode
    launched = []; path = args.out/'full_parallel.json'
    def measure(exe,case,req,call_args,checkpoint,**options):
        saved = json.loads(path.read_text())
        need(len(saved['launch_intents'])==len(launched)+1 and len(saved['runs'])==len(launched),'intent before process')
        launched.append(req)
        if scenario=='missing_checkpoint': return dict(req,status='failed')
        def child(argv,**kwargs):
            COUNTS['attempts'] += 1
            need(kwargs['timeout']==60 and argv[10:]==[str(req['workers']),str(req['optimizations'])],'actual argv')
            if scenario=='interrupt_before': raise KeyboardInterrupt
            if scenario=='failure' and len(launched)==1:
                raise subprocess.TimeoutExpired(argv,60,output=b'preserved partial process')
            Path(argv[3]).write_bytes(encode(fixtures.VALUE,req['coord_bits'])[0])
            return subprocess.CompletedProcess(argv,0,fixtures.wire(stream(req)),b'')
        with patch.object(full.subprocess,'run',side_effect=child):
            return original_measure(exe,case,req,call_args,checkpoint,**options)
    def decode(*parameters):
        COUNTS['decodes'] += 1
        if scenario=='interrupt_after': raise KeyboardInterrupt
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
                return 100000 if (scenario=='budget_all' and ticks[0]>1 or len(launched)>=3) else 0
            stack.enter_context(patch.object(driver.time,'monotonic',side_effect=clock))
        try: code = driver.run(args)
        except KeyboardInterrupt:
            need(scenario.startswith('interrupt'),'deliberate interruption'); COUNTS['interruptions'] += 1; code=None
        except ValueError:
            need(scenario=='missing_checkpoint','deliberate lost checkpoint'); code=None
    report = json.loads(path.read_text())
    need(report['schema']=='ehgp.v11.full_parallel_campaign.v4' and report['reuse_census'] is True and
         report['parallel_verticals'] is False and report['optimized_catalogue'] is False and
         report['descent_work_mask']==399 and report['requested_runs']==29 and
         report['requested']==driver.schedule(reuse_census=True),'persisted route and calendar')
    if code is None:
        after = scenario=='interrupt_after'
        need(not report['complete'] and not report['conforming'] and len(report['launch_intents'])==1 and
             len(report['runs'])==int(after) and not report['not_run'],'no fabricated completion or attempt')
        if after:
            row = report['runs'][0]
            need(row['status']=='pending_semantic' and row['stdout'] and 'semantic' not in row and
                 Path(row['argv'][3]).is_file(),'process evidence retained before decoder interruption')
        return report
    need(report['complete'] and report['conforming'] is (scenario=='ok') and code==int(scenario!='ok'),'verdict')
    actual = [full.identity(r) for field in ('runs','not_run') for r in report[field]]
    need(len(actual)==len(set(actual))==29 and set(actual)==set(map(full.identity,report['requested'])),'partition')
    if scenario.startswith('budget'):
        n = 0 if scenario=='budget_all' else 3
        need(len(report['runs'])==n and len(report['not_run'])==29-n and
             all(r['reason']=='campaign_budget_before_launch' for r in report['not_run']),'deadline-only omissions')
    else: need(len(report['runs'])==29 and not report['not_run'],'no mode suppressed')
    if scenario=='failure':
        need(report['runs'][0]['status']=='timeout' and report['runs'][1]['status']==report['runs'][2]['status']=='ok','255 and511 after127 failure')
    return report


def comparisons(report):
    for scenario in ('ok','paid511','paid255','paid127','invariant','semantic','raw','verticals','lanes','incomplete','incomplete_different'):
        rows = copy.deepcopy(report['runs'])
        target = next(r for r in rows if r['case']=='lidar_ng00' and r['coord_bits']==21 and r['workers']==48 and
                      r['optimizations']==(int(scenario[4:]) if scenario.startswith('paid') else 511))
        if scenario.startswith('paid'): target['events'][2]['orders'][1]['work']['census_point_tests'] += 1
        if scenario=='invariant': target['events'][2]['orders'][0]['work']['cells'] += 1
        if scenario in ('semantic','incomplete_different'): target['semantic']['sha256']='b'*64
        if scenario=='raw': target['semantic']['raw_sha256']='c'*64
        if scenario=='verticals': target['events'][2]['orders'][1]['vertical_parallel']['vertical_batches'] += 1
        if scenario=='lanes': target['events'][2]['orders'][0]['parallel']['regular_batches'] += 1
        if scenario.startswith('incomplete'): rows[0]['status']='timeout'
        group = next(g for g in driver.comparisons(rows,report['requested']) if g['case']=='lidar_ng00')
        need(group['status']==('equal' if scenario=='ok' else 'incomplete' if scenario=='incomplete' else 'different'),scenario)
        if scenario=='ok':
            a,b=report['runs'][1:3]
            need(a['events'][2]['orders'][1]['work']['census_point_tests']==
                 2*b['events'][2]['orders'][1]['work']['census_point_tests'] and group['lane_work_equal'],
                 'actual counts differ by route, equal within route')
        if scenario.startswith('paid'):
            need(group['invariant_work_equal'] and not group['lane_work_equal'],'paid comparison includes256')
        COUNTS['comparisons'] += 1


def main():
    calendar(); physical_slots()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-census-') as directory:
        root = Path(directory)
        with patch.object(fixtures,'stream',side_effect=stream): attempts(root)
        comparisons(campaign(root,'ok'))
        for scenario in ('failure','budget_all','budget_tail','interrupt_before','interrupt_after','missing_checkpoint'):
            campaign(root,scenario)
    COUNTS['attempts'] += fixtures.COUNTS['attempts']; COUNTS['decodes'] += fixtures.COUNTS['decodes']
    need(COUNTS['attempts']>=150 and COUNTS['corruptions']>=380 and COUNTS['decodes']>=90 and
         COUNTS['schedules']==7 and COUNTS['interruptions']==2 and COUNTS['comparisons']==11,'coverage floors')
    print('full_census_collector_verdict conforme '+' '.join('%s%d'%item for item in COUNTS.items())+
          ' checks%d native0'%CHECKS)


if __name__=='__main__': main()
