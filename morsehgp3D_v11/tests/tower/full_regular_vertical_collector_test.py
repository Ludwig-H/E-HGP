"""Regular vertical reuse benchmark: real tiny artifacts, mocked processes, exact bounded windows."""
import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'bench'))
import full_parallel as driver
import full_parallel_collector_test as fixtures
import full_dense_collector_test as campaigns
full = driver.full
COUNTS = dict(attempts=0, corruptions=0, decodes=0, schedules=0, interruptions=0, comparisons=0, windows=0, cli=0)
CHECKS = 0


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def calendar():
    legal = [i for i in range(2048) if not i & 128 or i & 8]
    need(len(legal) == 1536 and all(full.optimization(i) == i for i in legal), 'all legal masks')
    def row(name, bits, mode, workers=48):
        return dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected = [row(name,bits,mode) for name in ('lidar_ng00','lidar_ng01','lidar_ng02')
                for bits in (21,24) for mode in (511,1023,2047)]
    expected += [row('lidar_ng00',21,2047,w) for w in (1,8)]
    expected += [row(name,21,mode) for name in ('uniform_u18_n8000','uniform_u18_n16000','uniform_u18_n32000')
                 for mode in (511,1023,2047)]
    need(driver.schedule(reuse_verticals=True) == expected and len(set(map(full.identity,expected))) == 29,
         'ordered calendar29')
    need([len(driver.schedule()),len(driver.schedule(True)),len(driver.schedule(parallel_verticals=True)),
          len(driver.schedule(reuse_census=True)),len(driver.schedule(dense_births=True))] == [19,27,20,29,20],
         'older calendars unchanged')
    for options in [dict(reuse_verticals=True,**{name:True}) for name in
                    ('optimized_catalogue','parallel_verticals','reuse_census','dense_births')] + [dict(reuse_verticals=1)]:
        try:
            driver.schedule(**options)
        except ValueError:
            COUNTS['corruptions'] += 1
        else:
            raise ValueError('invalid calendar accepted')
    root = Path(__file__).resolve().parents[2]/'bench'
    plan = json.loads((root/'plans/full_vertical_reuse_g4.json').read_text())
    need([c['timeout_seconds'] for c in plan['commands']] == [850,180,570] and
         sum(c['timeout_seconds'] for c in plan['commands']) + 120 == 1720, 'plan and setup bounds')
    need(plan['commands'][2]['argv'][-4:] == ['--budget-seconds','500','--reuse-semantic','--reuse-verticals'] and
         plan['results_cap_bytes'] == 16*1024**2, 'bounded trace-preserving benchmark')


def cli():
    args = ['full_campaign.py']
    for name in ('builds','data','out','work','qualification','supplement'):
        args += ['--'+name, '/not-read']
    for mode in (0,511,1023,1024,2047,128,1152,4096,-1):
        valid = mode in (0,511,1023,1024,2047)
        with patch.object(sys,'argv',args+['--optimizations',str(mode)]), \
                patch.object(full,'run',return_value=0) as run, contextlib.redirect_stderr(io.StringIO()):
            try:
                result = full.main()
            except SystemExit as error:
                result = error.code
        need(result == (0 if valid else 2) and run.called == valid, 'actual campaign argparse mode')
        if valid:
            need(run.call_args.args[0].optimizations == mode, 'parser preserves requested mode')
        COUNTS['cli'] += 1
    for flags in (['--reuse-verticals'], ['--reuse-verticals','--dense-births']):
        with patch.object(sys,'argv',args+flags), patch.object(driver,'run',return_value=0) as run, \
                contextlib.redirect_stderr(io.StringIO()):
            try:
                result = driver.main()
            except SystemExit as error:
                result = error.code
        need(result == (0 if len(flags)==1 else 2) and run.called == (len(flags)==1), 'actual exclusive calendar CLI')
        COUNTS['cli'] += 1


def windows():
    # Independently enumerate hit/miss positions, retaining original fixed windows.
    for births in range(1,9):
        for capacity in range(1,6):
            for mask in range(1 << births):
                misses = [i for i in range(births) if mask & (1 << i)]
                spans = [min(capacity,births-begin) for begin in range(0,births,capacity)
                         if any(begin <= i < begin+capacity for i in misses)]
                value = dict.fromkeys(full.vertical.FIELDS,0)
                value.update(vertical_batches=len(spans),vertical_resolutions=len(misses),max_vertical_batch=max(spans,default=0))
                full.vertical.reused_windows(births,capacity,len(misses),value,need)
                COUNTS['windows'] += 1
    for births,capacity,descents,batches,span in ((8,3,1,0,0),(8,3,1,2,3),(8,3,4,1,3),
            (8,3,1,1,1),(8,3,2,2,2),(8,3,3,1,2),(3,9,1,1,1),(8,3,0,1,3)):
        value = dict.fromkeys(full.vertical.FIELDS,0)
        value.update(vertical_batches=batches,vertical_resolutions=descents,max_vertical_batch=span)
        try:
            full.vertical.reused_windows(births,capacity,descents,value,need)
        except ValueError:
            COUNTS['corruptions'] += 1
        else:
            raise ValueError('impossible original window accepted')


def mixed(events):
    event = events[2]; order = event['orders'][1]
    work, timing = order['work'], order['vertical_parallel']
    work.update(vertical_descents=1,vertical_reuses=order['births']-1,ancestor_find_steps=2)
    fixtures.paid_work(work,2,event['optimizations'])
    timing.update(vertical_batches=1,vertical_resolutions=1,max_vertical_batch=order['births'],
                  vertical_dispatch_ns=6,vertical_task_sum_ns=4,vertical_task_max_ns=4,vertical_sweep_ns=4)


def attempts(root):
    harness = fixtures.Attempts(root)
    def run(mode=2047, bits=21, **options):
        return harness.run(fixtures.request(mode,48,bits),**options)
    modes = (1024,1031,1035,1151,1279,1280,1535,1544,2043,2047)
    for bits in (18,21,24):
        for mode in modes:
            row = run(mode,bits)
            need(row['status'] == 'ok', 'reuse route including zero memo')
            event = row['events'][2]
            need(event['regular_vertical_reserved_bytes'] == 24 and
                 all(o['work']['vertical_descents'] == 0 for o in event['orders']), 'all regular births reused')
    mixed_row = run(mutation=mixed)
    need(mixed_row['status']=='ok' and mixed_row['events'][2]['orders'][1]['vertical_parallel']['max_vertical_batch'] >
         mixed_row['events'][2]['orders'][1]['vertical_parallel']['vertical_resolutions'], 'partial window retains its span')
    # K1 has no table even with reuse enabled; standalone metadata test, not a second native claim.
    event = fixtures.stream(fixtures.request(2047))[2]; event['kmax']=1; event['orders']=event['orders'][:1]
    event['regular_vertical_reserved_bytes']=0
    full.regular.validate(event,6,need,full.unsigned)
    event['regular_vertical_reserved_bytes']=24
    try:
        full.regular.validate(event,6,need,full.unsigned)
    except ValueError:
        COUNTS['corruptions']+=1
    else:
        raise ValueError('K1 allocated a seed table')
    mutations = {
        'missing_flag':lambda e:e[2].pop('reuse_regular_verticals'),
        'false_flag':lambda e:e[2].update(reuse_regular_verticals=False),
        'int_flag':lambda e:e[2].update(reuse_regular_verticals=1),
        'missing_bytes':lambda e:e[2].pop('regular_vertical_reserved_bytes'),
        'small_bytes':lambda e:e[2].update(regular_vertical_reserved_bytes=20),
        'per_order_bytes':lambda e:e[2].update(regular_vertical_reserved_bytes=120),
        'boolean_bytes':lambda e:e[2].update(regular_vertical_reserved_bytes=True),
        'negative_bytes':lambda e:e[2].update(regular_vertical_reserved_bytes=-1),
        'overflow_bytes':lambda e:e[2].update(regular_vertical_reserved_bytes=2**64),
        'coexistence':lambda e:e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']+e[2]['memo_reserved_bytes']+
            e[2]['parallel']['lane_memo_reserved_bytes']+e[2]['census_workspace_reserved_bytes']+23),
        'missing_reuses':lambda e:e[2]['orders'][1]['work'].pop('vertical_reuses'),
        'boolean_reuses':lambda e:e[2]['orders'][1]['work'].update(vertical_reuses=True),
        'negative_reuses':lambda e:e[2]['orders'][1]['work'].update(vertical_reuses=-1),
        'overflow_reuses':lambda e:e[2]['orders'][1]['work'].update(vertical_reuses=2**64),
        'missing_birth':lambda e:e[2]['orders'][1]['work'].update(vertical_reuses=3),
        'double_birth':lambda e:e[2]['orders'][1]['work'].update(vertical_descents=1),
        'query_uses_descents_only':lambda e:e[2]['orders'][1]['work'].update(ancestor_queries=3),
        'k1_reuse':lambda e:e[2]['orders'][0]['work'].update(vertical_reuses=1),
        'all_hit_dispatch':lambda e:e[2]['orders'][1]['vertical_parallel'].update(vertical_dispatch_ns=1),
        'all_hit_batch':lambda e:e[2]['orders'][1]['vertical_parallel'].update(vertical_batches=1,max_vertical_batch=4),
        'phantom_resolution':lambda e:e[2]['orders'][1]['vertical_parallel'].update(vertical_resolutions=1),
    }
    cache=full.reuse.SummaryCache();first=run(1023,cache=cache);preserved=copy.deepcopy(first)
    second=run(cache=cache)
    need(first['status']==second['status']=='ok' and second['semantic_reuse']['mode']=='reused' and
         first['semantic']==second['semantic'] and len(cache)==1,'artifact reuse across changed descent work')
    for name,mutation in mutations.items():
        for origin in (cache,full.reuse.SummaryCache()):
            before=fixtures.COUNTS['decodes'];size=len(origin)
            row=run(cache=origin,mutation=mutation)
            need(row['status']=='invalid_output' and 'semantic_reuse' not in row and len(origin)==size and
                 fixtures.COUNTS['decodes']==before,'diagnostics precede decode/reuse: '+name)
            COUNTS['corruptions']+=1
    for mutation in (lambda e:e[2].update(reuse_regular_verticals=True),
                     lambda e:e[2].update(regular_vertical_reserved_bytes=24),
                     lambda e:e[2]['orders'][1]['work'].update(vertical_reuses=1)):
        need(run(1023,mutation=mutation)['status']=='invalid_output','disabled path remains strict')
        COUNTS['corruptions']+=1
    need(first==preserved,'previous attempt and summary not aliased')


def comparisons(report):
    for scenario in ('ok','dense_paid','reuse_paid','invariant','semantic','raw','verticals','lanes',
                     'reuse_count','reference_find','incomplete','incomplete_different'):
        rows=copy.deepcopy(report['runs'])
        mode=1023 if scenario in ('dense_paid','reference_find') else 2047
        target=next(r for r in rows if r['case']=='lidar_ng00' and r['coord_bits']==21 and r['workers']==48 and
                    r['optimizations']==mode)
        work=target['events'][2]['orders'][1]['work']
        if scenario in ('dense_paid','reuse_paid'):work['census_point_tests']+=1
        if scenario=='invariant':work['cells']+=1
        if scenario=='reference_find':work['ancestor_find_steps']+=1
        if scenario=='reuse_count':work['vertical_reuses']-=1
        if scenario in ('semantic','incomplete_different'):target['semantic']['sha256']='b'*64
        if scenario=='raw':target['semantic']['raw_sha256']='c'*64
        if scenario=='verticals':target['events'][2]['orders'][1]['vertical_parallel']['vertical_batches']+=1
        if scenario=='lanes':target['events'][2]['orders'][0]['parallel']['regular_batches']+=1
        if scenario.startswith('incomplete'):rows[0]['status']='timeout'
        group=next(g for g in driver.comparisons(rows,report['requested']) if g['case']=='lidar_ng00')
        need(group['status']==('equal' if scenario=='ok' else 'incomplete' if scenario=='incomplete' else 'different'),scenario)
        COUNTS['comparisons']+=1
    # Paid work may differ between1023 and2047, but not between equivalent2047 attempts.
    rows=copy.deepcopy(report['runs'])
    for row in rows:
        if row['optimizations'] & 1024:
            for order in row['events'][2]['orders'][1:]:
                order['work']['ancestor_find_steps']+=2
    need(all(g['status']=='equal' for g in driver.comparisons(rows,report['requested'])), 'no false memo-work invariant across reuse')
    COUNTS['comparisons']+=1


def main():
    calendar(); cli(); windows()
    with tempfile.TemporaryDirectory(prefix='mhgp11-regular-vertical-') as temporary:
        root=Path(temporary);attempts(root)
        report=campaigns.campaign(root,'ok',option='reuse_verticals');comparisons(report)
        for scenario in ('failure','budget_all','budget_tail','interrupt_before','interrupt_after','missing_checkpoint'):
            campaigns.campaign(root,scenario,option='reuse_verticals')
    for field in ('attempts','decodes'):
        COUNTS[field]=fixtures.COUNTS[field]+campaigns.COUNTS[field]
    for field in ('schedules','interruptions'):
        COUNTS[field]=campaigns.COUNTS[field]
    need(COUNTS['attempts']>=100 and COUNTS['corruptions']>=55 and COUNTS['decodes']>=30 and
         COUNTS['windows']>=2500 and COUNTS['cli']==11 and COUNTS['schedules']==7 and
         COUNTS['interruptions']==2 and COUNTS['comparisons']==13,'coverage floors')
    print('full_regular_vertical_collector_verdict conforme '+' '.join('%s%d'%item for item in COUNTS.items())+
          ' checks%d native0'%CHECKS)


if __name__=='__main__':
    main()
