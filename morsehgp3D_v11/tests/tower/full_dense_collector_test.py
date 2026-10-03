"""Dense birth lookup collection: exact reservations and unchanged work, simulated children only."""
import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'bench'))
import full_parallel as driver
import full_parallel_collector_test as fixtures
from full_campaign_test import arguments
from full_bench_semantic_test import encode
full=driver.full
COUNTS=dict(attempts=0,corruptions=0,decodes=0,schedules=0,interruptions=0,comparisons=0)
CHECKS=0

def need(condition,message):
    global CHECKS
    CHECKS+=1
    if not condition:raise ValueError(message)

def calendar():
    legal=[i for i in range(1024) if not i & 128 or i & 8]
    need(len(legal)==768 and all(full.optimization(i)==i for i in legal),'all768 legal parser masks')
    for value in (True,False,-1,16384,None,1.0,'512',640):
        try:full.optimization(value)
        except ValueError:COUNTS['corruptions']+=1
        else:raise ValueError('illegal mode accepted')
    def row(name,bits,mode,workers=48):
        return dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected=[row(name,bits,mode) for name in ('lidar_ng00','lidar_ng01','lidar_ng02')
              for bits in (21,24) for mode in (511,1023)]
    expected += [row('lidar_ng00',21,1023,w) for w in (1,8)]
    expected += [row(name,21,mode) for name in ('uniform_u18_n8000','uniform_u18_n16000','uniform_u18_n32000')
                 for mode in (511,1023)]
    need(driver.schedule(dense_births=True)==expected and len(set(map(full.identity,expected)))==20,'calendar20')
    need([len(driver.schedule()),len(driver.schedule(True)),len(driver.schedule(parallel_verticals=True)),
          len(driver.schedule(reuse_census=True))]==[19,27,20,29],'historical calendars retained')
    for options in (dict(dense_births=True,reuse_census=True),dict(dense_births=True,parallel_verticals=True),
                    dict(dense_births=True,optimized_catalogue=True),dict(dense_births=1),dict(dense_births=None)):
        try:driver.schedule(**options)
        except ValueError:COUNTS['corruptions']+=1
        else:raise ValueError('invalid calendar accepted')
    plan=json.loads((Path(__file__).resolve().parents[2]/'bench/plans/full_dense_g4.json').read_text())
    need([c['timeout_seconds'] for c in plan['commands']]==[850,180,570] and
         sum(c['timeout_seconds'] for c in plan['commands'])+120==1720,'plan includes120setup')
    command=plan['commands'][2]
    need(command['name']=='dense_full' and command['argv'][-4:]==['--budget-seconds','500','--reuse-semantic','--dense-births']
         and plan['results_cap_bytes']==16*1024**2,'bounded dense plan')

def attempts(root):
    harness=fixtures.Attempts(root)
    def run(mode=1023,bits=21,workers=48,**options):
        return harness.run(fixtures.request(mode,workers,bits),**options)
    for bits in (18,21,24):
        for mode in (512,519,527,639,767,1023):
            row=run(mode,bits)
            need(row['status']=='ok','targeted dense combination')
            orders=row['events'][2]['orders']
            need([o['lookup_reserved_bytes'] for o in orders]==[20,24,24,24,24],'dense exact array reservations')
            need(row['events'][2]['lookup_reserved_bytes']==116,'all five dense lookups retained')
    for workers in (1,8,48):
        row=run(511,workers=workers)
        need(row['status']=='ok' and row['events'][2]['lookup_reserved_bytes']==120,'sparse exact8births retained')
    # Fresh cache for each profile/route pair: prove cross-512 reuse with memo
    # disabled, regular lanes, parallel verticals and borrowed census separately.
    for bits in (18,21,24):
        for target in (512,527,767,1023):
            pair_cache=full.reuse.SummaryCache()
            left=run(target & ~512,bits,cache=pair_cache);right=run(target,bits,cache=pair_cache)
            need(left['status']==right['status']=='ok' and left['semantic_reuse']['mode']=='decoded' and
                 right['semantic_reuse']['mode']=='reused' and left['semantic']==right['semantic'] and
                 [o['work'] for o in left['events'][2]['orders']]==[o['work'] for o in right['events'][2]['orders']],
                 'every route/profile pair keeps geometry and complete paid work')
    mutations={
        'full_missing':lambda e:e[2].pop('dense_birth_lookup'),
        'full_false':lambda e:e[2].update(dense_birth_lookup=False),
        'full_bool_integer':lambda e:e[2].update(dense_birth_lookup=1),
        'full_null':lambda e:e[2].update(dense_birth_lookup=None),
        'total_missing':lambda e:e[2].pop('lookup_reserved_bytes'),
        'total_bool':lambda e:e[2].update(lookup_reserved_bytes=True),
        'total_negative':lambda e:e[2].update(lookup_reserved_bytes=-1),
        'total_overflow':lambda e:e[2].update(lookup_reserved_bytes=2**64),
        'total_small':lambda e:e[2].update(lookup_reserved_bytes=115),
        'total_large':lambda e:e[2].update(lookup_reserved_bytes=117),
        'retained_small':lambda e:e[2].update(reserved_after_bytes=115),
        'order_missing':lambda e:e[2]['orders'][0].pop('dense_birth_lookup'),
        'order_false':lambda e:e[2]['orders'][1].update(dense_birth_lookup=False),
        'order_bool_integer':lambda e:e[2]['orders'][1].update(dense_birth_lookup=1),
        'order_bytes_missing':lambda e:e[2]['orders'][1].pop('lookup_reserved_bytes'),
        'order_bytes_bool':lambda e:e[2]['orders'][1].update(lookup_reserved_bytes=True),
        'order_bytes_negative':lambda e:e[2]['orders'][1].update(lookup_reserved_bytes=-1),
        'order_bytes_overflow':lambda e:e[2]['orders'][1].update(lookup_reserved_bytes=2**64),
        'k1_uses_balls':lambda e:e[2]['orders'][0].update(lookup_reserved_bytes=24),
        'k2_uses_sites':lambda e:e[2]['orders'][1].update(lookup_reserved_bytes=20),
        'dense_uses_births':lambda e:e[2]['orders'][1].update(lookup_reserved_bytes=32),
    }
    cache=full.reuse.SummaryCache();first=run(511,cache=cache);preserved=copy.deepcopy(first)
    second=run(cache=cache)
    need(first['status']==second['status']=='ok' and second['semantic_reuse']['mode']=='reused' and len(cache)==1,
         'same exact artifact reused across lookup representation')
    for name,mutation in mutations.items():
        for origin in (cache,full.reuse.SummaryCache()):
            before=fixtures.COUNTS['decodes'];size=len(origin)
            row=run(cache=origin,mutation=mutation)
            need(row['status']=='invalid_output' and 'semantic_reuse' not in row and len(origin)==size and
                 fixtures.COUNTS['decodes']==before,'dense diagnosis before decode/cache: '+name)
            COUNTS['corruptions']+=1
    for mutation in (lambda e:e[2].update(dense_birth_lookup=True),
                     lambda e:e[2]['orders'][0].update(lookup_reserved_bytes=20)):
        row=run(511,mutation=mutation)
        need(row['status']=='invalid_output','sparse route cannot silently become dense');COUNTS['corruptions']+=1
    need(first==preserved,'summary and original attempt never aliased')

def campaign(root, scenario, option='dense_births', leaf_size=16):
    desired = driver.schedule(**{option:True})
    count = len(desired)
    COUNTS['schedules'] += 1
    args = arguments(root,'campaign_'+scenario+'_leaf%d'%leaf_size)
    args.leaf_size=leaf_size
    args.budget_seconds=500; args.reuse_semantic=True; setattr(args,option,True)
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
            need(kwargs['timeout']==60 and argv[5]==str(leaf_size) and
                 argv[10:]==[str(req['workers']),str(req['optimizations'])],'actual argv')
            if scenario=='interrupt_before': raise KeyboardInterrupt
            if scenario=='failure' and len(launched)==1:
                raise subprocess.TimeoutExpired(argv,60,output=b'preserved partial process')
            Path(argv[3]).write_bytes(encode(fixtures.VALUE,req['coord_bits'])[0])
            stream=fixtures.stream(req);stream[1]['leaf_size']=leaf_size
            return subprocess.CompletedProcess(argv,0,fixtures.wire(stream),b'')
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
    need(report['leaf_size']==leaf_size and all(r['argv'][5]==str(leaf_size) for r in report['launch_intents']),
         'report and every process intent retain actual leaf size')
    need(report['schema']==driver.SCHEMA and all(report[name] is (name==option) for name in
         ('dense_births','reuse_census','parallel_verticals','optimized_catalogue','reuse_verticals','pair_graph')) and
         report['descent_work_mask']==13711 and report['requested_runs']==count and
         report['requested']==desired,'persisted route and calendar')
    need(report['census_comparison_schema']=='ehgp.v11.full_census_comparison.v2' and
         report['census_comparison_mask']==13455,'versioned cross-route comparison')
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
    need(len(actual)==len(set(actual))==count and set(actual)==set(map(full.identity,report['requested'])),'partition')
    if scenario.startswith('budget'):
        n = 0 if scenario=='budget_all' else 3
        need(len(report['runs'])==n and len(report['not_run'])==count-n and
             all(r['reason']=='campaign_budget_before_launch' for r in report['not_run']),'deadline-only omissions')
    else: need(len(report['runs'])==count and not report['not_run'],'no mode suppressed')
    if scenario=='failure':
        need(report['runs'][0]['status']=='timeout' and report['runs'][1]['status']=='ok','dense pair survives sparse timeout')
    return report


def comparisons(report):
    for scenario in ('ok','paid','invariant','semantic','raw','verticals','lanes','incomplete','incomplete_different'):
        rows=copy.deepcopy(report['runs'])
        target=next(r for r in rows if r['case']=='lidar_ng00' and r['coord_bits']==21 and
                    r['workers']==48 and r['optimizations']==1023)
        if scenario=='paid':target['events'][2]['orders'][1]['work']['census_point_tests']+=1
        if scenario=='invariant':target['events'][2]['orders'][0]['work']['cells']+=1
        if scenario in ('semantic','incomplete_different'):target['semantic']['sha256']='b'*64
        if scenario=='raw':target['semantic']['raw_sha256']='c'*64
        if scenario=='verticals':target['events'][2]['orders'][1]['vertical_parallel']['vertical_batches']+=1
        if scenario=='lanes':target['events'][2]['orders'][0]['parallel']['regular_batches']+=1
        if scenario.startswith('incomplete'):rows[0]['status']='timeout'
        group=next(g for g in driver.comparisons(rows,report['requested']) if g['case']=='lidar_ng00')
        need(group['status']==('equal' if scenario=='ok' else 'incomplete' if scenario=='incomplete' else 'different'),scenario)
        if scenario=='paid':need(group['invariant_work_equal'] and not group['lane_work_equal'],'512 cannot mask paid-work changes')
        COUNTS['comparisons']+=1

def main():
    calendar()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-dense-') as temporary:
        root=Path(temporary);attempts(root)
        report=campaign(root,'ok');comparisons(report)
        for scenario in ('failure','budget_all','budget_tail','interrupt_before','interrupt_after','missing_checkpoint'):
            campaign(root,scenario)
    COUNTS['attempts']+=fixtures.COUNTS['attempts'];COUNTS['decodes']+=fixtures.COUNTS['decodes']
    need(COUNTS['attempts']>=100 and COUNTS['corruptions']>=55 and COUNTS['decodes']>=40 and
         COUNTS['schedules']==7 and COUNTS['interruptions']==2 and COUNTS['comparisons']==9,'coverage floors')
    print('full_dense_collector_verdict conforme '+' '.join('%s%d'%item for item in COUNTS.items())+
          ' checks%d native0'%CHECKS)

if __name__=='__main__':main()
