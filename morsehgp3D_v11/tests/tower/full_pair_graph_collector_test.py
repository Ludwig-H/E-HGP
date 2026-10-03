"""FULL pair graph and singleton accounting: real tiny dumps, mocked processes, no native calls."""
import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'bench'))
import full_parallel as driver
import full_parallel_collector_test as fixtures
import full_dense_collector_test as campaigns
full=driver.full
CHECKS=0
COUNTS=dict(attempts=0,corruptions=0,cli=0,comparisons=0,schedules=0,interruptions=0)


def need(value,message):
    global CHECKS
    CHECKS+=1
    if not value:
        raise ValueError(message)


def calendar():
    def row(name,bits,mode,workers=48):
        return dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected=[row(name,bits,mode) for name in ('lidar_ng00','lidar_ng01','lidar_ng02')
              for bits in (21,24) for mode in (2047,4095)]
    expected += [row('lidar_ng00',21,4095,w) for w in (1,8)]
    expected += [row(name,21,mode) for name in ('uniform_u18_n8000','uniform_u18_n16000','uniform_u18_n32000')
                 for mode in (2047,4095)]
    need(driver.schedule(pair_graph=True)==expected and len(set(map(full.identity,expected)))==20,'exact calendar20')
    need([len(driver.schedule()),len(driver.schedule(True)),len(driver.schedule(parallel_verticals=True)),
          len(driver.schedule(reuse_census=True)),len(driver.schedule(dense_births=True)),
          len(driver.schedule(reuse_verticals=True))]==[19,27,20,29,20,29],'older calendars unchanged')
    for key in ('optimized_catalogue','parallel_verticals','reuse_census','dense_births','reuse_verticals'):
        try: driver.schedule(pair_graph=True,**{key:True})
        except ValueError: COUNTS['corruptions']+=1
        else: raise ValueError('two calendars accepted')
    for value in (1,None,'true'):
        try: driver.schedule(pair_graph=value)
        except ValueError: COUNTS['corruptions']+=1
        else: raise ValueError('non-bool option accepted')
    plan=json.loads((Path(__file__).resolve().parents[2]/'bench/plans/full_pair_graph_g4.json').read_text())
    need([c['timeout_seconds'] for c in plan['commands']]==[850,180,570,570] and
         sum(c['timeout_seconds'] for c in plan['commands'])+120==2290,'whole guarded budget')
    commands=plan['commands'][2:]
    for command,leaf in zip(commands,(16,8)):
        need(command['argv'][-6:]==['--budget-seconds','500','--reuse-semantic','--pair-graph','--leaf-size',str(leaf)],
             'two independent bounded plans')
    need(commands[0]['name']!=commands[1]['name'] and
         commands[0]['argv'][commands[0]['argv'].index('--work')+1]!=
         commands[1]['argv'][commands[1]['argv'].index('--work')+1],'distinct output roots and workspaces')


def cli():
    argv=['test']
    for key in ('builds','data','out','work','qualification','supplement'):
        argv+=['--'+key,'/not-read']
    for mode in (0,2047,2048,2051,2055,2063,4095,-1,16384,2176):
        valid=0<=mode<=16383 and bool(not mode & 128 or mode & 8) and bool(not mode & 8192 or mode & 8)
        with patch.object(sys,'argv',argv+['--optimizations',str(mode)]),patch.object(full,'run',return_value=0) as run, \
                contextlib.redirect_stderr(io.StringIO()):
            try: code=full.main()
            except SystemExit as error: code=error.code
        need(code==(0 if valid else 2) and run.called==valid,'actual FULL argparse')
        if valid:need(run.call_args.args[0].optimizations==mode,'mode preserved')
        COUNTS['cli']+=1
    for flags in (['--pair-graph'],['--pair-graph','--reuse-verticals']):
        with patch.object(sys,'argv',argv+flags),patch.object(driver,'run',return_value=0) as run, \
                contextlib.redirect_stderr(io.StringIO()):
            try: code=driver.main()
            except SystemExit as error: code=error.code
        need(code==(0 if len(flags)==1 else 2) and run.called==(len(flags)==1),'actual calendar argparse')
        COUNTS['cli']+=1
    for leaf in (7,8,12,16,256,257):
        with patch.object(sys,'argv',argv+['--pair-graph','--leaf-size',str(leaf)]), \
                patch.object(driver,'run',return_value=0) as run,contextlib.redirect_stderr(io.StringIO()):
            try:code=driver.main()
            except SystemExit as error:code=error.code
        valid=8<=leaf<=256
        need(code==(0 if valid else 2) and run.called==valid,'actual leaf parser')
        if valid:need(run.call_args.args[0].leaf_size==leaf,'leaf argument preserved')
        COUNTS['cli']+=1


def attempts(root):
    harness=fixtures.Attempts(root)
    def run(mode=4095,bits=21,workers=48,**kwargs):
        return harness.run(fixtures.request(mode,workers,bits),**kwargs)
    for bits in (18,21,24):
        for mode in (2048,2051,2055,2063,2047,4095):
            row=run(mode,bits)
            need(row['status']=='ok','whole artifact and current counters')
            work=row['events'][2]['orders'][0]['work']
            need(work['singleton_hits']==work['descent_steps'] and work['census_point_tests']==0,'K1 shortcut paid')
    for workers in (1,8,48):
        need(run(workers=workers)['status']=='ok','physical workers do not change graph work')
    cache=full.reuse.SummaryCache()
    old=run(2047,cache=cache); snapshot=copy.deepcopy(old)
    new=run(cache=cache)
    need(old['status']==new['status']=='ok' and new['semantic_reuse']['mode']=='reused' and
         new['semantic']==old['semantic'],'current complete bytes before cross-option summary reuse')
    mutations={
        'graph_missing':lambda e:e[1].pop('pair_graph'),
        'graph_bool':lambda e:e[1].update(pair_graph=1),
        'graph_wrong':lambda e:e[1].update(pair_graph=False),
        'mapped_option':lambda e:e[1].update(catalogue_optimizations=31),
        'ledger_missing':lambda e:e[1].pop('catalogue_work'),
        'ledger_extra':lambda e:e[1]['catalogue_work'].update(alien=0),
        'ledger_missing_field':lambda e:e[1]['catalogue_work'].pop('prefixes'),
        'ledger_bool':lambda e:e[1]['catalogue_work'].update(prefixes=True),
        'ledger_negative':lambda e:e[1]['catalogue_work'].update(region_pair_tests=-1),
        'ledger_overflow':lambda e:e[1]['catalogue_work'].update(census_tests=2**64),
        'emissions':lambda e:e[1]['catalogue_work'].update(emitted=5),
        'populations':lambda e:e[1]['catalogue_work'].update(incidences=11),
        'graph_pair_test':lambda e:e[1]['catalogue_work'].update(region_pair_tests=1),
        'graph_pair_reject':lambda e:e[1]['catalogue_work'].update(region_pair_tests=1,region_pair_rejects=1),
        'line_cache':lambda e:e[1]['catalogue_work'].update(region_line_tests=4),
        'leaf_missing':lambda e:e[1].pop('leaf_size'),
        'leaf_wrong':lambda e:e[1].update(leaf_size=8),
        'leaf_bool':lambda e:e[1].update(leaf_size=True),
        'singleton_missing':lambda e:e[2]['orders'][0]['work'].pop('singleton_hits'),
        'singleton_bool':lambda e:e[2]['orders'][0]['work'].update(singleton_hits=True),
        'singleton_negative':lambda e:e[2]['orders'][0]['work'].update(singleton_hits=-1),
        'singleton_overflow':lambda e:e[2]['orders'][0]['work'].update(singleton_hits=2**64),
        'partition':lambda e:e[2]['orders'][0]['work'].update(catalogue_hits=1),
        'K1_census':lambda e:e[2]['orders'][0]['work'].update(singleton_hits=5,census_calls=1,census_point_tests=1),
        'K2_no_descent':lambda e:e[2]['orders'][1]['work'].update(singleton_hits=1,descent_steps=1),
        'K3_singleton':lambda e:e[2]['orders'][2]['work'].update(singleton_hits=1,descent_steps=1),
        'points_without_call':lambda e:e[2]['orders'][0]['work'].update(census_point_tests=1),
    }
    for name,mutate in mutations.items():
        for active_cache in (cache,full.reuse.SummaryCache()):
            before=fixtures.COUNTS['decodes']; size=len(active_cache)
            row=run(mutation=mutate,cache=active_cache)
            need(row['status']=='invalid_output' and 'semantic_reuse' not in row and len(active_cache)==size and
                 fixtures.COUNTS['decodes']==before,'guard before cached summary: '+name)
            COUNTS['corruptions']+=1
    need(old==snapshot,'prior receipt not mutated')
    # A memo hit pays no singleton step. A K2 vertical miss really can be a singleton.
    row=run(7)
    need(row['events'][2]['orders'][0]['work']['memo_hits']>0 and
         row['events'][2]['orders'][0]['work']['singleton_hits']<row['events'][2]['orders'][0]['work']['memo_queries'],
         'memo initial hits do not fabricate singleton work')
    need(row['events'][2]['orders'][1]['work']['singleton_hits']>0,'K2 singleton counted')
    for process in ('timeout','failed','refused','launch'):
        row=run(process=process)
        need(row['status']==('launch_error' if process=='launch' else process) and 'semantic' not in row,
             'failed attempt remains failed')


def comparisons(report):
    for scenario in ('ok','prefix','pair_tests','pair_rejects','line','q4','census','forest','raw','semantic',
                     'incomplete','incomplete_different'):
        rows=copy.deepcopy(report['runs'])
        target=next(r for r in rows if r['case']=='lidar_ng00' and r['coord_bits']==21 and r['workers']==48 and
                    r['optimizations']==4095)
        work=target['events'][1]['catalogue_work']
        if scenario=='prefix':work['prefixes']+=1
        if scenario=='pair_tests':work['region_pair_tests']=100
        if scenario=='pair_rejects':work['region_pair_rejects']=20;work['prefixes']=30
        if scenario=='line':work['region_line_tests']+=1;work['region_line_evaluations']+=1
        if scenario=='q4':work['q4_candidates']+=1
        if scenario=='census':work['census_tests']+=1
        if scenario=='forest':target['events'][2]['orders'][0]['work']['singleton_hits']+=1
        if scenario=='raw':target['semantic']['raw_sha256']='c'*64
        if scenario in ('semantic','incomplete_different'):target['semantic']['sha256']='b'*64
        if scenario.startswith('incomplete'):rows[0]['status']='timeout'
        group=next(g for g in driver.comparisons(rows,report['requested']) if g['case']=='lidar_ng00')
        need(group['status']==('equal' if scenario=='ok' else 'incomplete' if scenario=='incomplete' else 'different'),scenario)
        COUNTS['comparisons']+=1
    totals=[g['catalogue_pair_graph'] for g in report['comparisons']]
    need(sum(g['run_pairs'] for g in totals)==sum(g['reduced_pairs'] for g in totals)==19,'19 available graph pairs')
    # Mixed >32 fallback leaves keep pair rejections. This is a comparison fixture, not a fake geometric run.
    rows=copy.deepcopy(report['runs'])
    for row in rows:
        if row['optimizations'] & 2048:
            row['events'][1]['catalogue_work'].update(max_leaf=33,region_pair_tests=7,region_pair_rejects=4,prefixes=14)
        else:row['events'][1]['catalogue_work']['max_leaf']=33
    proof=full.pair_graph.comparisons(rows)
    need(proof['prefixes_equal'] and proof['pair_work_monotone'] and proof['mixed_fallback_pairs']==19,'mixed formula')
    COUNTS['comparisons']+=1


def main():
    calendar();cli()
    with tempfile.TemporaryDirectory(prefix='mhgp11-full-pair-graph-') as tmp:
        root=Path(tmp);attempts(root)
        report=campaigns.campaign(root,'ok',option='pair_graph');comparisons(report)
        leaf8=campaigns.campaign(root,'ok',option='pair_graph',leaf_size=8)
        need(leaf8['requested']==report['requested'] and leaf8['leaf_size']==8 and report['leaf_size']==16 and
             all(left['semantic']==right['semantic'] for left,right in zip(report['runs'],leaf8['runs'])),
             'independent leaf calendars compare geometry without mixing work')
        for scenario in ('failure','budget_all','budget_tail','interrupt_before','interrupt_after','missing_checkpoint'):
            campaigns.campaign(root,scenario,option='pair_graph')
    COUNTS['attempts']=fixtures.COUNTS['attempts']+campaigns.COUNTS['attempts']
    COUNTS['schedules']=campaigns.COUNTS['schedules'];COUNTS['interruptions']=campaigns.COUNTS['interruptions']
    need(COUNTS['attempts']>=100 and COUNTS['corruptions']==62 and COUNTS['cli']==18 and
         COUNTS['comparisons']==13 and COUNTS['schedules']==8 and COUNTS['interruptions']==2,'coverage')
    print('full_pair_graph_collector_verdict conforme '+json.dumps(dict(COUNTS,checks=CHECKS,native=0),sort_keys=True))


if __name__=='__main__':
    main()
