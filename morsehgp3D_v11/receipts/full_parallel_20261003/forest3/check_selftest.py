"""Pure checks of recorded evidence and a synthetic archived FULL singleton; no native execution."""
import copy
import json
from pathlib import Path
import re
import struct
import tempfile
from unittest.mock import patch

import check as r

COUNTS = dict(positives=0, corruptions=0)


def good(value, label):
    r.need(value,label); COUNTS['positives'] += 1


def reject(action):
    try: action()
    except r.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):
        COUNTS['corruptions'] += 1; return
    raise ValueError('corruption accepted')


def stdout(row):
    row['stdout'] = ''.join(json.dumps(e)+'\n' for e in row['events'])


def decoded(row):
    if 'semantic_reuse' in row:
        row['semantic_reuse'].update(mode='decoded',source_attempt=list(r.identity(row)))


def refresh(value):
    value['comparisons'] = r.driver.comparisons(value['runs'],value['requested'])
    done = len(value['runs'])==19 and not value['not_run']
    value['full_schedule_completed'] = done if value['complete'] else False
    value['conforming'] = value['complete'] and done and all(row['status']=='ok' for row in value['runs']) and all(
        c['status']=='equal' for c in value['comparisons'])


def failed(row,state):
    for key in ('semantic','semantic_reuse','semantic_wall_seconds',*r.DERIVED): row.pop(key,None)
    row.update(status=state,events=[],stdout='',stderr='',errors=[])
    if state=='timeout': row.update(exit_code=None,errors=[dict(stage='process',type='TimeoutExpired',message='fixture')])
    elif state=='launch_error': row.update(exit_code=None,errors=[dict(stage='launch',type='OSError',message='fixture')])
    elif state=='failed': row['exit_code'] = -9
    elif state=='refused': row['exit_code'] = 2
    elif state=='pending_semantic': row['exit_code'] = 0
    else: raise ValueError('unknown fixture')


def schedules(report,judge):
    good(judge(copy.deepcopy(report))['successes']==18,'closed omission retained')
    for state in ('timeout','launch_error','failed','refused'):
        value=copy.deepcopy(report)
        for row in value['runs']: decoded(row)
        failed(value['runs'][0],state); refresh(value)
        result=judge(value)
        good(result['successes']==17 and not result['conforming'],'failed attempt retained')
        value['runs'][1]['semantic_reuse'].update(mode='reused',source_attempt=list(r.identity(value['runs'][0])),
                                               decode_wall_seconds=0)
        reject(lambda:judge(value))
    for pending in (False,True):
        value=copy.deepcopy(report); value.update(complete=False,not_run=[])
        value['runs']=value['runs'][:1]; value['launch_intents']=value['launch_intents'][:1 if pending else 2]
        if pending: failed(value['runs'][0],'pending_semantic')
        refresh(value); good(judge(value)['unpersisted']==18,'interruption checkpoint retained')
        value['complete']=True; reject(lambda:judge(value))
    mutations=[lambda v:v.update(conforming=True),lambda v:v.update(full_schedule_completed=True),
        lambda v:v.update(requested_runs=18),lambda v:v.update(requested_runs=True),
        lambda v:v.update(budget_seconds=550),lambda v:v.update(campaign_wall_seconds=369),
        lambda v:v.update(campaign_wall_seconds=float('nan')),lambda v:v['not_run'].clear(),
        lambda v:v['not_run'][0].update(reason='another_mode_failed'),
        lambda v:v['not_run'][0].update(case='lidar_ng00'),lambda v:v['runs'].reverse(),
        lambda v:v['runs'].append(copy.deepcopy(v['runs'][0])),lambda v:v['launch_intents'].pop(),
        lambda v:v['launch_intents'][0].update(input_sha256='0'*64),
        lambda v:v['requested'][0].update(workers=True),lambda v:v['builds'][1].update(bytes=True),
        lambda v:v['builds'][1].update(sha256='0'*64),lambda v:v.update(qualification_sha256='0'*64),
        lambda v:v.update(supplement_sha256='0'*64),lambda v:v.update(parallel_schema='future'),
        lambda v:v.update(work_schema='future'),lambda v:v.update(regular_batch_capacity=0),
        lambda v:v.update(descent_lanes=8),lambda v:v.update(lane_memo_capacity=True),
        lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),
        lambda v:v['optimization_modes'].update({'15':'other'}),lambda v:v['comparisons'][0].update(status='different')]
    for mutate in mutations:
        value=copy.deepcopy(report); mutate(value); reject(lambda:judge(value))


def diagnostics(report,judge):
    def order(e): return e[2]['orders'][0]
    mutations=[lambda e:e[2]['parallel'].update(descent_lanes=8),
        lambda e:e[2]['parallel'].update(regular_batch_capacity=True),
        lambda e:e[2]['parallel'].update(lane_memo_reserved_bytes=0),
        lambda e:e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']),
        lambda e:order(e)['parallel'].update(regular_traces=0),
        lambda e:order(e)['parallel'].update(regular_cells=0),
        lambda e:order(e)['parallel'].update(regular_batches=0),
        lambda e:order(e)['parallel'].update(max_regular_batch=4097),
        lambda e:order(e)['parallel'].update(regular_task_sum_ns=2**64),
        lambda e:order(e)['parallel'].update(regular_task_max_ns=order(e)['parallel']['regular_dispatch_ns']+1),
        lambda e:order(e)['parallel'].update(regular_task_sum_ns=48*order(e)['parallel']['regular_dispatch_ns']+1),
        lambda e:order(e)['parallel'].update(regular_publish_ns=order(e)['timings']['plateaus_ns']+1),
        lambda e:order(e)['work'].update(memo_queries=0),
        lambda e:order(e)['work'].update(memo_lookups=0),
        lambda e:order(e)['work'].update(part_diameter_pairs=1),
        lambda e:order(e)['timings'].update(verticals_ns=1),
        lambda e:e[2].update(workers=8),lambda e:e[2].update(optimizations=7),
        lambda e:e[2].update(cpu_seconds=float('nan'))]
    for mutate in mutations:
        value=copy.deepcopy(report); row=value['runs'][1]; mutate(row['events']); stdout(row); refresh(value)
        reject(lambda:judge(value))
    # Reach work-comparison guards with internally valid current event streams.
    for mode in ('paid','invariant','raw','semantic'):
        value=copy.deepcopy(report)
        for row in value['runs']: decoded(row)
        row=next(x for x in value['runs'] if x['optimizations']==15 and x['workers']==8)
        if mode=='paid': row['events'][2]['orders'][0]['work']['census_point_tests']+=1
        elif mode=='invariant': row['events'][2]['orders'][0]['work']['cells']+=1
        else:
            key='raw_sha256' if mode=='raw' else 'sha256'; row['semantic'][key]='0'*64
            if mode=='raw': row['semantic_reuse']['raw_sha256']='0'*64
        stdout(row); refresh(value)
        good(judge(value)['different']==1,'observed difference stays nonconforming')
        value['comparisons'][0]['status']='equal'; reject(lambda:judge(value))


def reuse(report,judge):
    mutations=[lambda x:x.pop('semantic_reuse'),lambda x:x.update(exit_code=True),lambda x:x.update(stderr='extra'),
        lambda x:x.update(full_ms=1),lambda x:x.update(stdout=x['stdout']+'{}\n'),
        lambda x:x['semantic'].update(nodes=1),lambda x:x['semantic'].update(bytes=1),
        lambda x:x['semantic_reuse'].update(source_attempt=list(r.identity(report['runs'][3]))),
        lambda x:x['semantic_reuse'].update(source_attempt=list(r.identity(x))),
        lambda x:x['semantic_reuse'].update(decode_wall_seconds=1),
        lambda x:x['semantic_reuse']['context'].update(coord_bits=24),
        lambda x:x['semantic_reuse']['context'].update(decoder_sha256='0'*64),
        lambda x:x['semantic_reuse']['context'].update(ids_sha256='0'*64),
        lambda x:x['semantic_reuse'].update(raw_sha256='0'*64),
        lambda x:x['semantic_reuse'].update(bytes=1),
        lambda x:x['semantic_reuse'].update(current_attempt=list(r.identity(report['runs'][0]))),
        lambda x:x['semantic_reuse'].update(hash_wall_seconds=-1)]
    for mutate in mutations:
        value=copy.deepcopy(report); mutate(value['runs'][1]); reject(lambda:judge(value))


def tiny(report,manifest,builds):
    words=[21,1,1,1,0,0,0,1,7,1,1,1,0,0,2**32-1,0,0]
    for integer in (0,1,0,0,0,1): words += [0,1,integer]
    raw=b'MHGP11FUL1'+struct.pack('<'+'Q'*len(words),*words)
    summary=r.full.semantic.decode(raw,21,1,1)
    row=copy.deepcopy(report['runs'][0]); row.update(kmax=1,count=1,optimizations=3,semantic=summary)
    row.pop('semantic_reuse'); row['argv'][4]='1';row['argv'][11]='3'
    row['argv'][3]=str(Path(row['argv'][3]).with_name('%s_b21_k1_w48_r0_o3.bin' % row['case']))
    case=dict(manifest['cases'][0],count=1); row['events'][0].update(sites=1,points=1)
    e=row['events'][2]; e.update(kmax=1,optimizations=3,memo_capacity=0,memo_reserved_bytes=0)
    e['parallel']=dict.fromkeys(r.full.parallel.META,0)
    e['orders']=[dict(k=1,births=1,nodes=1,edges=0,verticals=0,node_capacity=1,edge_capacity=0,
        work=dict.fromkeys(r.full.WORK,0),timings=dict.fromkeys(r.full.ORDER_TIMINGS,0),
        parallel=dict.fromkeys(r.full.parallel.FIELDS,0))]
    with patch.object(r.full.semantic,'inspect',lambda *_:summary): r.full.collect(row,case,None,21)
    stdout(row); build=next(b for b in report['builds'] if b['coord_bits']==21)
    path='results/cmd/002_parallel_full/files/'+Path(row['argv'][3]).name
    good(r.attempt(row,case,build,True,True,{path:raw},{})==1,'actual archived tiny payload decoded')
    reject(lambda:r.attempt(row,case,build,True,True,{path:raw+b'!'},{}))
    reject(lambda:r.attempt(row,case,build,True,True,{path:raw,path.replace('/files/','/other/'):raw},{}))


def qualification(data):
    good(r.mutants(data)==254,'causal mutant logs')
    r.required_gates(data); good(True,'parallel native gates present')
    counts,_,_=r.q4.judge_matrix(data); extra,_=r.optim.supplement(data)
    r.qualification_counts(counts,extra)
    changed=dict(counts); changed['gcc_release']=(503,503,0,0)
    reject(lambda:r.qualification_counts(changed,extra))
    reject(lambda:r.qualification_counts(counts,(200,200,0,0)))
    for name in ('mhgp11_tower_forest_parallel_plateaus','mhgp11_tower_forest_parallel_memo_fraction'):
        altered=dict(data); path=r.BASE+'bits21/tests.json'; tests=r.js(altered[path])
        altered[path]=json.dumps([x for x in tests if x['name']!=name]).encode(); reject(lambda:r.required_gates(altered))
    altered=dict(data); ident=next(iter(r.PINS['mutants']['tower']))
    pattern=re.compile(re.escape(ident).encode()+rb'(\s+TUE\s+)(?:code|ligne)'); hits=0
    for name,raw in list(altered.items()):
        if name.startswith(r.BASE+'mutants/'):
            updated,count=pattern.subn(lambda m:ident.encode()+m[1]+b'signal',raw);altered[name]=updated;hits+=count
    r.need(hits>0,'mutation line exists');reject(lambda:r.mutants(altered))
    for mutate in (lambda v:v.update(budget_seconds=700),lambda v:v.update(thread_budget=32),
                   lambda v:v['configurations'][1].update(threads=12),lambda v:v['configurations'].pop()):
        altered=dict(data); path=r.BASE+'summary.json'; m=r.js(altered[path]);mutate(m)
        altered[path]=json.dumps(m).encode();reject(lambda:r.matrix_parameters(altered))


def transport(receipt):
    with tempfile.TemporaryDirectory(prefix='forest3-proof-') as td:
        folder=Path(td)
        for name in ('results.tar.gz','matrix.json','asan18.json','full_parallel.json'):
            (folder/name).symlink_to(r.HERE/name)
        (folder/'DONE').write_text('3\n'); original=r.js(Path(receipt['raw_receipt_local']).read_bytes())
        def trial(raw_change,compact_change):
            raw=copy.deepcopy(original); compact=copy.deepcopy(receipt);raw_change(raw);compact_change(compact)
            encoded=json.dumps(raw,sort_keys=True).encode();(folder/'raw.json').write_bytes(encoded)
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=r.sha(encoded))
            (folder/'receipt.json').write_text(json.dumps(compact,sort_keys=True));return r.transport(folder)
        good(bool(trial(lambda _:None,lambda _:None)),'transport fixture')
        for field in ('targeted_shutdown_certified','oslogin_key_removed'):
            reject(lambda:trial(lambda v:v.pop(field),lambda _:None))
        for key,value in [('oslogin_key_removed',False),('results_sha256','0'*64),('warnings',['unexpected']),
                          ('errors',['unexpected']),('results_skipped_members',['missing']),('commit','0'*40)]:
            reject(lambda:trial(lambda v:v.update({key:value}),lambda v:v.update({key:value})))
        trial(lambda _:None,lambda _:None);(folder/'DONE').write_text('0\n');reject(lambda:r.transport(folder))
        (folder/'DONE').write_text('3\n');(folder/'matrix.json').unlink();(folder/'matrix.json').write_bytes(b'{}')
        reject(lambda:r.transport(folder))


def main():
    receipt,worker,data,_=r.transport(r.HERE); manifest,mhash=r.old.inputs(r.HERE,receipt)
    _,builds,_=r.q4.judge_matrix(data); report=r.js(data[r.BENCH])
    def judge(value): return r.benchmark(value,manifest,mhash,builds,data)
    schedules(report,judge);diagnostics(report,judge);reuse(report,judge);tiny(report,manifest,builds)
    qualification(data);transport(receipt)
    verdict=r.check();good(verdict['coherent'] and not verdict['conforming'] and verdict['successes']==18 and
                          verdict['omissions']==1 and verdict['matrix_passed']==2673,'LIVE final')
    metrics=r.analysis(report);good(len(metrics['complete_mode_pairs'])==7,'six memo pairs and one nonmemo pair')
    print(json.dumps(dict(**COUNTS,native=0),sort_keys=True))


if __name__=='__main__': main()
