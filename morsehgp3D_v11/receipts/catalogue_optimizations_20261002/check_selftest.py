"""Pure receipt controls: memory corruptions, preserved failures, one tiny archived payload."""
import copy
import hashlib
import json
import re
from pathlib import Path
import struct
from unittest.mock import patch

import check as reader


def refresh(report):
    report['comparisons'] = reader.js(json.dumps(reader.driver.comparisons(report['runs'],report['requested'])))
    done = len(report['runs']) == 36 and not report['not_run']
    report['full_schedule_completed'] = done if report['complete'] else False
    report['conforming'] = report['complete'] and done and all(r['status']=='ok' for r in report['runs']) and all(
        r['status']=='equal' for r in report['comparisons'])


def failed(row,state):
    for key in ('semantic','qmin_counts','canonical_bytes','canonical_sha256','semantic_wall_seconds',
                'stage_ms','pool_ms','catalogue_ms','cloud_ms','read_ms','cache_work','catalogue_within_100ms',
                'catalogue_within_200ms','cloud_pool_catalogue_ms'):
        row.pop(key,None)
    row.update(status=state,events=[],stdout='',stderr='',errors=[])
    if state == 'timeout': row.update(exit_code=None,errors=[dict(stage='process',type='TimeoutExpired',message='fixture')])
    elif state == 'failed': row['exit_code'] = -15
    elif state == 'launch_error': row.update(exit_code=None,errors=[dict(stage='launch',type='OSError',message='fixture')])
    elif state == 'pending_semantic': row['exit_code'] = 0
    else: raise ValueError('fixture state')


def tiny():
    bits = 21
    def words(*values): return struct.pack('<%dQ' % len(values),*values)
    def integer(value,budget):
        limbs = (budget+63)//64 if budget > 127 else 2
        return words(0,limbs,value,*([0]*(limbs-1)))
    raw = b'MHGP11CAT1'+words(bits,5,2)+words(0,0,0,1,7,2,0,0,1,9)+words(2)
    for n in (0,1): raw += integer(n,8*bits+12)+integer(1,6*bits+8)
    raw += words(1,2,0,2,1,0,1,2**32-1,2**32-1,0,2,0,1)
    semantic = reader.driver.profiles.semantic.decode(raw,bits,5,2,arity_counts=True)
    qmin = semantic.pop('qmin_counts')
    case = dict(name='fixture',coordinates='xyz',point_ids='ids',count=2)
    build = dict(path='/build/mhgp11_catalogue_bench')
    row = dict(case='fixture',coord_bits=bits,kmax=5,workers=48,repetition=0,optimizations=0,
               count=2,whole_input=True,timeout_seconds=15,exit_code=0,status='ok',errors=[],stderr='',
               process_wall_seconds=1.,semantic_wall_seconds=0.,semantic=semantic,qmin_counts=qmin,
               canonical_bytes=len(raw),canonical_sha256=hashlib.sha256(raw).hexdigest(),
               catalogue_ms=0.0001,cloud_ms=0.00002,read_ms=0.00001,catalogue_within_100ms=True)
    row['argv'] = [build['path'],'/data/xyz','/data/ids','/work/fixture_b21_k5_w48_r0.bin',
                   '5','16','256','0',str(2**32-1),str(8*1024**3),'48']
    cloud = dict(phase='cloud',status='ok',reason='none',points=2,sites=2,read_ns=10,cloud_ns=20,cloud_peak_bytes=200)
    logical = dict.fromkeys(reader.driver.profiles.LOGICAL,0)
    logical.update(nodes=1,leaves=1,prefixes=1,judged=1,census_tests=2,max_leaf=2)
    timing = dict.fromkeys(reader.driver.parallel.TIMING_FIELDS,1); timing['sort_comparisons'] = 0
    cat = dict(phase='catalogue',status='ok',reason='none',coord_bits=bits,kmax=5,workers=48,pool_ns=10,
               generation_passes=2,balls=1,levels=2,incidences=2,wall_ns=100,cpu_seconds=0.,
               peak_reserved_bytes=200,reserved_after_bytes=100,logical=logical,timings=timing,
               optimizations=0,cache_work=dict(evaluations=0,hits=0,fallbacks=0),work=dict(q4_candidates=0,q4_levels=0))
    row['events'] = [cloud,cat,dict(phase='exit',status='ok')]
    row['stdout'] = '\n'.join(json.dumps(v) for v in row['events'])+'\n'
    reader.driver.check_optimization(row,row)
    data = {'results/cmd/002_optimizations/files/fixture_b21_k5_w48_r0.bin':raw}
    return row,case,build,data


def main():
    folder = reader.HERE/'optimizations2'
    receipt,worker,data = reader.old.read_capture(folder)
    manifest,mhash = reader.old.inputs(folder,receipt)
    _,builds,_ = reader.q4.judge_matrix(data)
    report = reader.js(data[reader.BENCH])
    positives,corruptions = 0,0
    def judge(value):
        return reader.benchmark(value,manifest,mhash,reader.sha(data[reader.BASE+'summary.json']),
                                reader.sha(data[reader.SUPP+'summary.json']),builds,data)
    def reject(fn):
        nonlocal corruptions
        try: fn()
        except reader.REFUSALS+(ValueError,KeyError,TypeError,IndexError):
            corruptions += 1; return
        raise ValueError('corruption not refused')
    result = judge(copy.deepcopy(report)); reader.need(result['conforming'] and result['successes']==36,'positive'); positives += 1
    for state in ('timeout','failed','launch_error'):
        value = copy.deepcopy(report); failed(value['runs'][0],state); refresh(value)
        verdict = judge(value)
        reader.need(not verdict['conforming'] and verdict['attempts']==36 and verdict['successes']==35,
                    'failed baseline does not suppress another mode'); positives += 1
    value = copy.deepcopy(report); value['runs'] = value['runs'][:3]; value['launch_intents'] = value['launch_intents'][:3]
    value['complete'] = False; failed(value['runs'][-1],'pending_semantic'); refresh(value)
    reader.need(judge(value)['unpersisted']==33,'pending checkpoint'); positives += 1
    value = copy.deepcopy(report); value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:3]
    value['complete'] = False; refresh(value)
    reader.need(judge(value)['unpersisted']==34,'pending launch intent'); positives += 1
    value = copy.deepcopy(report); value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:2]
    value['not_run'] = [dict(r,reason='campaign_budget_before_launch') for r in value['requested'][2:]]
    value['campaign_wall_seconds'] = 680.; refresh(value)
    reader.need(judge(value)['omissions']==34,'budget tail'); positives += 1
    value = copy.deepcopy(report); failed(value['runs'][0],'timeout')
    value['runs'][1]['semantic']['sha256'] = 'a'*64; refresh(value)
    reader.need(judge(value)['different']==1,'divergence wins over missing baseline'); positives += 1
    value = copy.deepcopy(report); row = value['runs'][0]
    row.update(status='invalid_output',errors=[dict(stage='optimization',type='ValueError',message='fixture')])
    refresh(value); reader.need(not judge(value)['conforming'],'postdecode failure retained'); positives += 1
    mutations = [
        lambda v:v.update(schema='unknown'),lambda v:v.update(conforming=False),
        lambda v:v.update(complete=1),lambda v:v.update(full_schedule_completed=False),
        lambda v:v['requested'].reverse(),lambda v:v['runs'].pop(),lambda v:v['runs'].append(v['runs'][0]),
        lambda v:v['runs'].reverse(),lambda v:v['runs'][0].update(optimizations=True),
        lambda v:v['runs'][0].update(coord_bits=True),lambda v:v['runs'][0].update(workers=8),
        lambda v:v['runs'][0].update(repetition=1),lambda v:v['launch_intents'].pop(),
        lambda v:v['launch_intents'][0].update(optimizations=3),lambda v:v['launch_intents'][0].update(ids_sha256='0'*64),
        lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),lambda v:v['runs'][1]['argv'].__setitem__(11,'3'),
        lambda v:v['runs'][0].update(exit_code=True),lambda v:v['runs'][0].update(stderr='diagnostic'),
        lambda v:v['runs'][0].update(process_wall_seconds=float('nan')),
        lambda v:v['runs'][0].update(semantic_wall_seconds=-1),lambda v:v.update(campaign_wall_seconds=1),
        lambda v:v.update(budget_seconds=701),lambda v:v['runs'][0].update(canonical_bytes=35),
        lambda v:v['runs'][0]['semantic'].update(balls=True),lambda v:v['runs'][0]['semantic'].update(sha256='0'*64),
        lambda v:v['runs'][0].update(canonical_sha256='0'*64),lambda v:v['runs'][0].update(catalogue_within_200ms=True),
        lambda v:v['runs'][0]['cache_work'].update(hits=1),lambda v:v['runs'][0]['stage_ms'].update(sort=0),
        lambda v:v['runs'][0]['events'][1]['cache_work'].update(hits=1),
        lambda v:v['runs'][0]['events'][1]['work'].update(q4_levels=0),
        lambda v:v['runs'][0]['events'][1]['timings'].update(sort_comparisons=2**64),
        lambda v:v['runs'][0].update(stdout=v['runs'][0]['stdout']+'{}\n'),
        lambda v:v.update(manifest_sha256='0'*64),lambda v:v['builds'][1].update(sha256='0'*64),
        lambda v:v['builds'][1].update(bytes=True),lambda v:v.update(supplement_sha256='0'*64),
        lambda v:v['comparisons'][0].update(requested=9),
    ]
    for mutate in mutations:
        value = copy.deepcopy(report); mutate(value); reject(lambda:judge(value))
    # Keep stdout/events concordant here, so actual diagnostic guards must reject.
    for kind in ('mode_bool','mode_wrong','cache_request','cache_off','cache_overflow','q4','concurrency'):
        value = copy.deepcopy(report); row = value['runs'][0]; event = row['events'][1]
        if kind == 'mode_bool': event['optimizations'] = False
        elif kind == 'mode_wrong': event['optimizations'] = 3
        elif kind == 'cache_request': event['cache_work']['evaluations'] += 1
        elif kind == 'cache_off': event['cache_work']['fallbacks'] = 1
        elif kind == 'cache_overflow': event['cache_work']['evaluations'] = 2**64
        elif kind == 'q4': event['work']['q4_levels'] += 1
        else:
            t = event['timings']; t['count_task_max_ns'] = t['count_ns']
            t['count_task_sum_ns'] = event['workers']*t['count_ns']+1
        row['stdout'] = '\n'.join(json.dumps(v) for v in row['events'])+'\n'
        row['cache_work'] = copy.deepcopy(event['cache_work'])
        if kind != 'q4': refresh(value)
        reject(lambda:judge(value))
    for kind in ('reason','after_skip','pending_nonfinal','intent_unknown'):
        value = copy.deepcopy(report)
        if kind in ('reason','after_skip'):
            value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:2]
            value['not_run'] = [dict(r,reason='campaign_budget_before_launch') for r in value['requested'][2:]]
            value['campaign_wall_seconds'] = 680.
            if kind == 'reason': value['not_run'][0]['reason'] = 'same_profile_K5_failed'
            else: value['runs'].append(copy.deepcopy(report['runs'][3]));value['launch_intents'].append(copy.deepcopy(report['launch_intents'][3]));value['not_run'].pop(1)
        elif kind == 'pending_nonfinal': value['complete']=False;failed(value['runs'][0],'pending_semantic')
        else: value['launch_intents'][-1]['case'] = 'unknown'
        refresh(value); reject(lambda:judge(value))
    row,case,build,small = tiny()
    reader.need(reader.attempt(row,case,build,True,True,small)==1,'real archive decode'); positives += 1
    changed = dict(small); key = next(iter(changed)); changed[key] += b'!'
    reject(lambda:reader.attempt(row,case,build,True,True,changed))
    changed = dict(small); changed['results/cmd/002_optimizations/files/duplicate/'+Path(key).name] = changed[key]
    reject(lambda:reader.attempt(row,case,build,True,True,changed))
    bad = copy.deepcopy(row); bad['semantic']['sha256'] = '0'*64
    reject(lambda:reader.attempt(bad,case,build,True,True,small))
    changed = dict(data); key = reader.BASE+'bits21/tests.json'; selected = reader.js(changed[key])
    selected = [r for r in selected if r['name']!='mhgp11_catalogue_sort_fraction']
    changed[key] = json.dumps(selected).encode()
    reject(lambda:reader.required_gates(changed))
    read_bytes = Path.read_bytes
    for name in ('source_contract.json','source_df/catalogue_optimizations.py'):
        target = reader.HERE/name
        with patch.object(Path,'read_bytes',lambda self:read_bytes(self)+(b'\n' if self==target else b'')):
            reject(reader.frozen)
    # Assembly controls bypass the hash transport ONLY to isolate these later guards.
    for kind in ('cleanup','source','copy','meta','mutant'):
        r,w,d = copy.deepcopy(receipt),copy.deepcopy(worker),dict(data)
        if kind == 'cleanup': r['oslogin_key_removed'] = False
        if kind == 'source': r['commit'] = '0'*40
        if kind == 'copy': d[reader.BENCH] += b'\n'
        if kind == 'meta':
            key = 'results/cmd/002_optimizations/meta.txt';d[key] = d[key].replace(b'group_closed=1',b'group_closed=0')
        if kind == 'mutant':
            key = next(p for p in d if p.endswith('/LastTest.log') and '/mutants/' in p)
            before = d[key]; d[key] = re.sub(rb'TUE\s+code',b'TUE signal',before,count=1)
            reader.need(d[key] != before,'mutant corruption effective')
        if kind in ('copy','meta','mutant'):
            d['results/MANIFEST.sha256'] = ''.join(reader.sha(raw)+'  ./'+p.removeprefix('results/')+'\n'
                for p,raw in d.items() if p!='results/MANIFEST.sha256').encode()
        with patch.object(reader.old,'read_capture',return_value=(r,w,d)):
            reject(reader.check)
    reader.need(positives==10 and corruptions>=50,'selftest_floor')
    print(json.dumps(dict(verdict='conforme',positives=positives,corruptions=corruptions,native=0),sort_keys=True))


if __name__ == '__main__': main()
