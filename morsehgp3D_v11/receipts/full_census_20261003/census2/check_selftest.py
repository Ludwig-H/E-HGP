"""Pure mutations of the recorded success; no native/cloud call or real payload recreation."""
import copy
import json
from pathlib import Path
import re
import struct
import tempfile

import check as r

COUNTS=dict(positives=0,corruptions=0)


def good(value,label):
    r.need(value,label); COUNTS['positives']+=1


def reject(action):
    try: action()
    except r.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):
        COUNTS['corruptions']+=1; return
    raise ValueError('corruption accepted')


def stdout(row): row['stdout']=''.join(json.dumps(e)+'\n' for e in row['events'])


def decoded(row):
    row['semantic_reuse'].update(mode='decoded',source_attempt=list(r.identity(row)))


def refresh(value):
    value['comparisons']=r.driver.comparisons(value['runs'],value['requested'])
    done=len(value['runs'])==29 and not value['not_run']
    value['full_schedule_completed']=done if value['complete'] else False
    value['conforming']=value['complete'] and done and all(x['status']=='ok' for x in value['runs']) and all(
        c['status']=='equal' for c in value['comparisons'])


def schedules(report,judge):
    good(judge(copy.deepcopy(report))['successes']==29,'all successful runs retained')
    for pending in (False,True):
        value=copy.deepcopy(report);value.update(complete=False,not_run=[])
        value['runs']=value['runs'][:1];value['launch_intents']=value['launch_intents'][:1 if pending else 2]
        if pending:
            row=value['runs'][0]
            for key in ('semantic','semantic_reuse','semantic_wall_seconds',*r.DERIVED): row.pop(key,None)
            row['status']='pending_semantic'
        refresh(value);good(judge(value)['unpersisted']==28,'checkpoint preserved')
        value['complete']=True;reject(lambda:judge(value))
    value=copy.deepcopy(report); omitted=value['requested'][-1]
    value['runs'].pop();value['launch_intents'].pop();value['not_run']=[dict(omitted,reason='campaign_budget_before_launch')]
    value['campaign_wall_seconds']=450;refresh(value)
    good(judge(value)['omissions']==1 and not judge(value)['conforming'],'omission remains nonconforming')
    value['conforming']=True;reject(lambda:judge(value))
    mutations=[lambda v:v.update(conforming=False),lambda v:v.update(full_schedule_completed=False),
        lambda v:v.update(requested_runs=28),lambda v:v.update(requested_runs=True),
        lambda v:v.update(budget_seconds=450),lambda v:v.update(campaign_wall_seconds=1),
        lambda v:v.update(campaign_wall_seconds=float('nan')),lambda v:v['runs'].reverse(),
        lambda v:v['runs'].append(copy.deepcopy(v['runs'][0])),lambda v:v['launch_intents'].pop(),
        lambda v:v['launch_intents'][0].update(input_sha256='0'*64),
        lambda v:v['requested'][0].update(workers=True),lambda v:v['builds'][1].update(bytes=True),
        lambda v:v['builds'][1].update(sha256='0'*64),lambda v:v.update(qualification_sha256='0'*64),
        lambda v:v.update(supplement_sha256='0'*64),lambda v:v.update(parallel_schema='future'),
        lambda v:v.update(work_schema='future'),lambda v:v.update(regular_batch_capacity=0),
        lambda v:v.update(descent_lanes=8),lambda v:v.update(lane_memo_capacity=True),
        lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),lambda v:v['optimization_modes'].update({'511':'other'}),
        lambda v:v['comparisons'][0]['census_workspace'].update(positive_order_pairs=0),
        lambda v:v.update(census_comparison_mask=399),lambda v:v.update(census_comparison_schema='v0'),
        lambda v:v.update(reuse_census=1),lambda v:v.update(descent_work_mask=143)]
    for mutate in mutations:
        value=copy.deepcopy(report);mutate(value);reject(lambda:judge(value))


def diagnostics(report,judge):
    def order(e): return e[2]['orders'][1]
    mutations=[lambda e:e[2].update(census_workspaces=1),lambda e:e[2].update(census_workspace_reserved_bytes=0),
        lambda e:e[2].update(reuse_census_workspace=False),lambda e:e[2].update(reuse_census_workspace=1),
        lambda e:e[2].update(census_workspace_reserved_bytes=2**64),
        lambda e:e[2].update(peak_reserved_bytes=e[2]['reserved_after_bytes']+e[2]['memo_reserved_bytes']+e[2]['parallel']['lane_memo_reserved_bytes']),
        lambda e:e[2]['parallel'].update(descent_lanes=8),lambda e:e[2]['parallel'].update(lane_memo_reserved_bytes=0),
        lambda e:order(e)['parallel'].update(regular_traces=0),lambda e:order(e)['parallel'].update(max_regular_batch=4097),
        lambda e:order(e)['parallel'].update(regular_task_sum_ns=2**64),
        lambda e:order(e)['parallel'].update(regular_task_max_ns=order(e)['parallel']['regular_dispatch_ns']+1),
        lambda e:order(e)['parallel'].update(regular_task_sum_ns=48*order(e)['parallel']['regular_dispatch_ns']+1),
        lambda e:order(e)['parallel'].update(regular_publish_ns=order(e)['timings']['plateaus_ns']+1),
        lambda e:order(e)['vertical_parallel'].update(vertical_resolutions=order(e)['births']-1),
        lambda e:order(e)['vertical_parallel'].update(vertical_batches=0),
        lambda e:order(e)['vertical_parallel'].update(vertical_task_sum_ns=48*order(e)['vertical_parallel']['vertical_dispatch_ns']+1),
        lambda e:order(e)['vertical_parallel'].update(vertical_sweep_ns=order(e)['timings']['verticals_ns']+1),
        lambda e:e[2]['orders'][0]['vertical_parallel'].update(vertical_resolutions=1),
        lambda e:e[2].update(parallel_verticals=False),
        lambda e:order(e)['work'].update(memo_queries=0),lambda e:order(e)['work'].update(memo_lookups=0),
        lambda e:order(e)['work'].update(census_point_tests=True),lambda e:order(e)['timings'].update(verticals_ns=1),
        lambda e:e[2].update(workers=8),lambda e:e[2].update(optimizations=255),lambda e:e[2].update(cpu_seconds=float('nan'))]
    for mutate in mutations:
        value=copy.deepcopy(report);row=value['runs'][2];mutate(row['events']);stdout(row)
        def changed():
            refresh(value); return judge(value)
        reject(changed)
    # W1 limits use one concurrent lane, regardless of a 48-lane capacity.
    value=copy.deepcopy(report);row=next(x for x in value['runs'] if x['workers']==1)
    v=row['events'][2]['orders'][1]['vertical_parallel'];v['vertical_task_sum_ns']=v['vertical_dispatch_ns']+1
    stdout(row);refresh(value);reject(lambda:judge(value))
    # Change both stdout and event fields to reach v5 cross-route checks, keeping within-route equality.
    for kind in ('point','other','raw','semantic'):
        value=copy.deepcopy(report)
        for row in value['runs']: decoded(row)
        for row in value['runs']:
            if row['case']!='lidar_ng00' or row['optimizations']!=511: continue
            if kind=='point': row['events'][2]['orders'][0]['work']['census_point_tests']+=1
            elif kind=='other': row['events'][2]['orders'][0]['work']['part_meb_presentations']+=1
            else:
                key='raw_sha256' if kind=='raw' else 'sha256';row['semantic'][key]='0'*64
                if kind=='raw': row['semantic_reuse']['raw_sha256']='0'*64
            stdout(row)
        refresh(value);good(judge(value)['different']==1,'difference remains visible')
        if kind in ('point','other'):
            need='point_tests_equal' if kind=='point' else 'other_work_equal'
            good(value['comparisons'][0]['census_workspace'][need] is False,'cross-route v5 difference')
            value['comparisons'][0]['census_workspace'][need]=True
        value['comparisons'][0]['status']='equal';reject(lambda:judge(value))


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
        lambda x:x['semantic_reuse'].update(raw_sha256='0'*64),lambda x:x['semantic_reuse'].update(bytes=1),
        lambda x:x['semantic_reuse'].update(current_attempt=list(r.identity(report['runs'][0]))),
        lambda x:x['semantic_reuse'].update(hash_wall_seconds=-1)]
    for mutate in mutations:
        value=copy.deepcopy(report);mutate(value['runs'][1]);reject(lambda:judge(value))


def tiny():
    words=[21,1,1,1,0,0,0,1,7,1,1,1,0,0,2**32-1,0,0]
    for integer in (0,1,0,0,0,1): words += [0,1,integer]
    raw=b'MHGP11FUL1'+struct.pack('<'+'Q'*len(words),*words);summary=r.full.semantic.decode(raw,21,1,1)
    row=dict(argv=['exe','points','ids','tiny.bin'],coord_bits=21,kmax=1,semantic=summary)
    case=dict(count=1);path='results/cmd/002_census_full/files/tiny.bin'
    good(r.archived_payload(row,case,{path:raw})==1,'tiny synthetic archived payload decoded')
    reject(lambda:r.archived_payload(row,case,{path:raw+b'!'}))
    reject(lambda:r.archived_payload(row,case,{path:raw,path.replace('/files/','/duplicate/'):raw}))


def qualification(data):
    good(r.mutants(data)==281,'causal manifest inventory');r.required_gates(data)
    good(sum(x[0] for x in r.matrices(data,r.PINS).values())==3141,'matrix inventory')
    for name in ('mhgp11_tower_census_reuse_full','mhgp11_tower_full_census_collector_opt','mhgp11_num_unit_certificate_owners'):
        changed=data.copy();path=r.BASE+'bits21/tests.json';tests=r.js(changed[path])
        changed[path]=json.dumps([x for x in tests if x['name']!=name]).encode();reject(lambda:r.required_gates(changed))
    for module,ident in [('tower','forest_cohort_nonbirth_reset'),('core','refus_construit_une_valeur')]:
        altered=data.copy();pattern=re.compile(re.escape(ident).encode()+rb'(\s+TUE\s+)(?:code|ligne|construction)');hits=0
        for name,raw in list(altered.items()):
            if name.startswith(r.BASE+'mutants/'):
                updated,n=pattern.subn(lambda m:ident.encode()+m[1]+b'signal',raw);altered[name]=updated;hits+=n
        r.need(hits>0,'mutation applied');reject(lambda:r.mutants(altered))
    for mutate in (lambda v:v.update(budget_seconds=700),lambda v:v.update(thread_budget=32),
                   lambda v:v.update(conforming=False),lambda v:v['configurations'].pop()):
        altered=data.copy();path=r.BASE+'summary.json';m=r.js(altered[path]);mutate(m)
        altered[path]=json.dumps(m).encode();reject(lambda:r.matrices(altered,r.PINS))


def transport(receipt):
    with tempfile.TemporaryDirectory(prefix='census2-proof-') as td:
        folder=Path(td)
        for name in ('results.tar.gz','matrix.json','asan18.json','full_parallel.json','inputs.json'):
            (folder/name).symlink_to(r.HERE/name)
        (folder/'DONE').write_text('0\n');original=r.js(Path(receipt['raw_receipt_local']).read_bytes())
        def trial(raw_change,compact_change):
            raw=copy.deepcopy(original);compact=copy.deepcopy(receipt);raw_change(raw);compact_change(compact)
            encoded=json.dumps(raw,sort_keys=True).encode();(folder/'raw.json').write_bytes(encoded)
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=r.sha(encoded))
            (folder/'receipt.json').write_text(json.dumps(compact,sort_keys=True));return r.transport(folder,r.PINS)
        good(bool(trial(lambda _:None,lambda _:None)),'transport fixture')
        for field in ('targeted_shutdown_certified','oslogin_key_removed'):
            reject(lambda:trial(lambda v:v.pop(field),lambda _:None))
        for key,value in [('oslogin_key_removed',False),('private_key_deleted',False),('reserve_released',False),
                          ('results_sha256','0'*64),('warnings',['unexpected']),('errors',['unexpected']),
                          ('results_skipped_members',['missing']),('commit','0'*40),('closing_generation','wrong')]:
            reject(lambda:trial(lambda v:v.update({key:value}),lambda v:v.update({key:value})))
        reject(lambda:trial(lambda v:v.update(guest_guard_intact=False),lambda v:v.update(guest_guard_intact=False)))
        trial(lambda _:None,lambda _:None);(folder/'DONE').write_text('3\n');reject(lambda:r.transport(folder,r.PINS))
        (folder/'DONE').write_text('0\n');(folder/'matrix.json').unlink();(folder/'matrix.json').write_bytes(b'{}')
        reject(lambda:r.transport(folder,r.PINS))


def main():
    receipt,worker,data=r.transport(r.HERE,r.PINS);manifest,mhash=r.old.inputs(r.HERE,receipt)
    builds={c['name']:r.old.provenance(c,data) for c in r.js(data[r.BASE+'summary.json'])['configurations']}
    report=r.js(data[r.BENCH]);judge=lambda v:r.benchmark(v,manifest,mhash,builds,data)
    for field,contents in [('stdout',b'fake ok\n'),('stderr',b'warning')]:
        altered=data.copy();altered['results/cmd/002_census_full/'+field]=contents
        reject(lambda:r.commands(receipt,worker,altered,r.PINS))
    schedules(report,judge);diagnostics(report,judge);reuse(report,judge);tiny();qualification(data);transport(receipt)
    verdict=r.check();good(verdict['conforming'] and verdict['successes']==29 and verdict['matrix_passed']==3141,'LIVE')
    good(verdict['cross_route']['positive_order_pairs']==95,'positive paired orders')
    print(json.dumps(dict(**COUNTS,native=0),sort_keys=True))


if __name__=='__main__': main()
