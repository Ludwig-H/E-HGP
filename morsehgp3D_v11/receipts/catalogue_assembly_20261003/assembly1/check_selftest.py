"""Pure reader checks over the closed capture and tiny in-memory fixtures; no native execution."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile

import check as reader


def refresh(value):
    value['comparisons'] = reader.js(json.dumps(reader.driver.comparisons(value['runs'],value['requested'])))
    done = len(value['runs'])==36 and not value['not_run']
    value['full_schedule_completed'] = done if value['complete'] else False
    value['conforming'] = value['complete'] and done and all(r['status']=='ok' for r in value['runs']) and all(
        c['status']=='equal' for c in value['comparisons'])


def decoded(row):
    if 'semantic_reuse' in row:
        row['semantic_reuse'].update(mode='decoded',source_attempt=list(reader.identity(row))+[False])


def failed(row,state):
    for key in ('semantic','qmin_counts','canonical_bytes','canonical_sha256','semantic_wall_seconds','semantic_reuse',
                'stage_ms','pool_ms','catalogue_ms','cloud_ms','read_ms','cache_work','catalogue_within_100ms',
                'catalogue_within_200ms','cloud_pool_catalogue_ms'): row.pop(key,None)
    row.update(status=state,events=[],stdout='',stderr='',errors=[])
    if state=='timeout': row.update(exit_code=None,errors=[dict(stage='process',type='TimeoutExpired',message='fixture')])
    elif state=='failed': row['exit_code'] = -15
    elif state=='launch_error': row.update(exit_code=None,errors=[dict(stage='launch',type='OSError',message='fixture')])
    elif state=='refused': row['exit_code'] = 2
    elif state=='pending_semantic': row['exit_code'] = 0
    else: raise ValueError('fixture state')


def tiny():
    def words(*values): return struct.pack('<%dQ' % len(values),*values)
    def integer(value,budget):
        limbs = (budget+63)//64 if budget>127 else 2
        return words(0,limbs,value,*([0]*(limbs-1)))
    bits = 21
    raw = b'MHGP11CAT1'+words(bits,5,2)+words(0,0,0,1,7,2,0,0,1,9)+words(2)
    for n in (0,1): raw += integer(n,8*bits+12)+integer(1,6*bits+8)
    raw += words(1,2,0,2,1,0,1,2**32-1,2**32-1,0,2,0,1)
    value = reader.profiles.semantic.decode(raw,bits,5,2,arity_counts=True); qmin = value.pop('qmin_counts')
    case = dict(name='fixture',coordinates='xyz',point_ids='ids',count=2,sha256='1'*64,ids_sha256='2'*64)
    build = dict(path='/build/mhgp11_catalogue_bench')
    row = dict(case='fixture',coord_bits=21,kmax=5,workers=48,repetition=0,optimizations=3,diagnostics=False,
               count=2,whole_input=True,timeout_seconds=15,exit_code=0,status='ok',errors=[],stderr='',
               process_wall_seconds=1.,semantic_wall_seconds=0.,semantic=value,qmin_counts=qmin,
               canonical_bytes=len(raw),canonical_sha256=hashlib.sha256(raw).hexdigest(),
               catalogue_ms=.0001,cloud_ms=.00002,read_ms=.00001,catalogue_within_100ms=True,semantic_reuse_requested=True)
    row['argv'] = [build['path'],'/data/xyz','/data/ids','/work/fixture_b21_k5_w48_r0_o3.bin',
                   '5','16','256','0',str(2**32-1),str(8*1024**3),'48','3']
    cloud = dict(phase='cloud',status='ok',reason='none',points=2,sites=2,read_ns=10,cloud_ns=20,cloud_peak_bytes=200)
    logical = dict.fromkeys(reader.profiles.LOGICAL,0)
    logical.update(nodes=1,leaves=1,prefixes=1,judged=1,census_tests=2,max_leaf=2)
    timing = dict.fromkeys(reader.driver.parallel.TIMING_FIELDS,1); timing['sort_comparisons'] = 0
    cat = dict(phase='catalogue',status='ok',reason='none',coord_bits=21,kmax=5,workers=48,pool_ns=10,
               generation_passes=2,balls=1,levels=2,incidences=2,wall_ns=100,cpu_seconds=0.,
               peak_reserved_bytes=200,reserved_after_bytes=100,logical=logical,timings=timing,
               optimizations=3,cache_work=dict(evaluations=0,hits=0,fallbacks=0),work=dict(q4_candidates=0,q4_levels=0))
    row['events'] = [cloud,cat,dict(phase='exit',status='ok')]
    row['stdout'] = '\n'.join(json.dumps(v) for v in row['events'])+'\n'
    reader.driver.check_assembly(row,row)
    context = reader.driver.reuse.Context('MHGP11CAT1',reader.profiles.semantic.SCHEMA+';arity_counts=true',
        reader.driver.reuse.decoder_digest([Path(reader.profiles.semantic.__file__)]),21,5,2,case['sha256'],case['ids_sha256'])
    row['semantic_reuse'] = dict(schema=reader.driver.reuse.SCHEMA,mode='decoded',context=reader.asdict(context),
        current_attempt=list(reader.identity(row))+[False],source_attempt=list(reader.identity(row))+[False],
        raw_sha256=row['canonical_sha256'],bytes=len(raw),hash_wall_seconds=0.,decode_wall_seconds=0.)
    return row,case,build,{'results/cmd/002_assembly/files/fixture_b21_k5_w48_r0_o3.bin':raw}


def main():
    receipt,worker,data,_ = reader.transport(reader.HERE)
    manifest,mhash = reader.old.inputs(reader.HERE,receipt); _,builds,_ = reader.q4.judge_matrix(data)
    report = reader.js(data[reader.BENCH]); positives,corruptions = 0,0
    def judge(value): return reader.benchmark(value,manifest,mhash,builds,data)
    def reject(fn):
        nonlocal corruptions
        try: fn()
        except reader.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):
            corruptions += 1; return
        raise ValueError('corruption not refused')
    reader.need(judge(copy.deepcopy(report))['conforming'],'closed positive'); positives += 1
    for state in ('timeout','failed','launch_error','refused'):
        value = copy.deepcopy(report)
        for row in value['runs']: decoded(row)
        failed(value['runs'][0],state); refresh(value); result = judge(value)
        reader.need(not result['conforming'] and result['successes']==35 and result['attempts']==36,'failure retained')
        positives += 1
    for pending in (False,True):
        value = copy.deepcopy(report); value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:3 if pending else 2]
        value['complete'] = False
        if not pending: failed(value['runs'][-1],'pending_semantic')
        refresh(value); reader.need(judge(value)['unpersisted']==34,'partial preserved'); positives += 1
    value = copy.deepcopy(report); value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:2]
    value['not_run'] = [dict(r,reason='campaign_budget_before_launch') for r in value['requested'][2:]]
    value['campaign_wall_seconds'] = 430.; refresh(value)
    reader.need(judge(value)['omissions']==34 and not value['conforming'],'budget tail'); positives += 1
    value = copy.deepcopy(report)
    for row in value['runs']: decoded(row)
    failed(value['runs'][0],'timeout'); value['runs'][1]['semantic']['sha256'] = '0'*64; refresh(value)
    reader.need(judge(value)['different']==1,'partial divergence retained'); positives += 1
    value = copy.deepcopy(report)
    for row in value['runs']: decoded(row)
    value['runs'][0].update(status='invalid_output',errors=[dict(stage='assembly',type='ValueError',message='fixture')])
    refresh(value); reader.need(not judge(value)['conforming'],'postdecode failure'); positives += 1
    mutations = [
        lambda v:v.update(schema='unknown'),lambda v:v.update(conforming=False),lambda v:v.update(complete=1),
        lambda v:v['requested'].reverse(),lambda v:v['runs'].pop(),lambda v:v['runs'].append(v['runs'][0]),
        lambda v:v['runs'].reverse(),lambda v:v['runs'][0].update(optimizations=True),
        lambda v:v['runs'][0].update(coord_bits=True),lambda v:v['runs'][0].update(workers=8),
        lambda v:v['runs'][0].update(diagnostics=True),lambda v:v['runs'][0].update(repetition=1),
        lambda v:v['launch_intents'].pop(),lambda v:v['launch_intents'][0].update(optimizations=11),
        lambda v:v['launch_intents'][0].update(ids_sha256='0'*64),lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),
        lambda v:v['runs'][1]['argv'].__setitem__(11,'3'),lambda v:v['runs'][0].update(exit_code=True),
        lambda v:v['runs'][0].update(stderr='diagnostic'),lambda v:v['runs'][0].update(process_wall_seconds=float('nan')),
        lambda v:v['runs'][0].update(semantic_wall_seconds=-1),lambda v:v.update(campaign_wall_seconds=1),
        lambda v:v.update(budget_seconds=700),lambda v:v['runs'][0].update(canonical_bytes=35),
        lambda v:v['runs'][0]['semantic'].update(balls=True),lambda v:v['runs'][0]['semantic'].update(sha256='0'*64),
        lambda v:v['runs'][0].update(canonical_sha256='0'*64),lambda v:v['runs'][0].update(catalogue_within_200ms=True),
        lambda v:v['runs'][0]['cache_work'].update(hits=1),lambda v:v['runs'][0]['stage_ms'].update(sort=0),
        lambda v:v['runs'][0].update(stdout=v['runs'][0]['stdout']+'{}\n'),lambda v:v.update(manifest_sha256='0'*64),
        lambda v:v['builds'][1].update(sha256='0'*64),lambda v:v['builds'][1].update(bytes=True),
        lambda v:v.update(supplement_sha256='0'*64),lambda v:v['comparisons'][0].update(requested=9),
        lambda v:v['semantic_reuse'].update(capacity=32),lambda v:v['runs'][0].pop('semantic_reuse'),
        lambda v:v['runs'][0]['semantic_reuse']['context'].update(decoder_sha256='0'*64),
        lambda v:v['runs'][0]['semantic_reuse']['context'].update(ids_sha256='0'*64),
        lambda v:v['runs'][1]['semantic_reuse'].update(source_attempt=v['runs'][1]['semantic_reuse']['current_attempt']),
        lambda v:v['runs'][0]['semantic_reuse'].update(mode='reused'),
        lambda v:v['runs'][1]['semantic_reuse'].update(decode_wall_seconds=.1),
        lambda v:v['runs'][1]['semantic_reuse'].update(raw_sha256='0'*64),
        lambda v:v['runs'][1]['semantic_reuse'].update(bytes=1),
        lambda v:v['runs'][1]['semantic_reuse'].update(hash_wall_seconds=100.),
        lambda v:v['runs'][1]['semantic'].update(sha256='0'*64),
        lambda v:v['launch_intents'][0].update(semantic_reuse_requested=False),
    ]
    for mutate in mutations:
        value = copy.deepcopy(report); mutate(value); reject(lambda:judge(value))
    for kind in ('mode','tasks','concurrency','wall','q4','cache','reserved','bool','reason'):
        value = copy.deepcopy(report); row = value['runs'][0]; event = row['events'][1]
        if kind=='mode': event['optimizations'] = 11
        elif kind=='tasks': event['timings']['tasks'] = 1025
        elif kind=='concurrency':
            event['timings']['count_task_max_ns'] = event['timings']['count_ns']
            event['timings']['count_task_sum_ns'] = 48*event['timings']['count_ns']+1
        elif kind=='wall': event['timings']['assembly_ns'] = event['wall_ns']+1
        elif kind=='q4': event['work']['q4_levels'] += 1
        elif kind=='cache': event['cache_work']['evaluations'] += 1
        elif kind=='reserved': event['reserved_after_bytes'] = event['peak_reserved_bytes']+1
        elif kind=='bool': event['timings']['tasks'] = True
        else: event['reason'] = 'memory_budget'
        row['stdout'] = '\n'.join(json.dumps(v) for v in row['events'])+'\n'
        reject(lambda:judge(value))
    for kind in ('wrong_reason','late_after_skip','pending_nonfinal','source_failed'):
        value = copy.deepcopy(report)
        if kind=='source_failed': failed(value['runs'][0],'timeout')
        elif kind=='pending_nonfinal': value['complete']=False; failed(value['runs'][0],'pending_semantic')
        else:
            value['runs'] = value['runs'][:2]; value['launch_intents'] = value['launch_intents'][:2]
            value['not_run'] = [dict(r,reason='campaign_budget_before_launch') for r in value['requested'][2:]]
            value['campaign_wall_seconds'] = 430.
            if kind=='wrong_reason': value['not_run'][0]['reason'] = 'baseline_failed'
            else:
                value['runs'].append(copy.deepcopy(report['runs'][3])); value['launch_intents'].append(copy.deepcopy(report['launch_intents'][3])); value['not_run'].pop(1)
        refresh(value); reject(lambda:judge(value))
    row,case,build,small = tiny()
    reader.need(reader.attempt(row,case,build,True,True,small,{})==1,'tiny archived decode'); positives += 1
    for raw in (next(iter(small.values()))[:-1],next(iter(small.values()))+b'\0'):
        reject(lambda:reader.attempt(row,case,build,True,True,{next(iter(small)):raw},{}))
    duplicate = dict(small); duplicate['results/cmd/002_assembly/other/'+Path(next(iter(small))).name] = next(iter(small.values()))
    reject(lambda:reader.attempt(row,case,build,True,True,duplicate,{}))
    corrupt = copy.deepcopy(row); corrupt['semantic']['sha256'] = '0'*64
    reject(lambda:reader.attempt(corrupt,case,build,True,True,small,{}))
    for mutation in ('source','generation','cleanup','missing_raw','manifest','copy','archive_hash'):
        with tempfile.TemporaryDirectory(prefix='assembly-reader-') as tmp:
            folder = Path(tmp); compact = copy.deepcopy(receipt)
            raw = reader.js(Path(receipt['raw_receipt_local']).read_bytes())
            for name in ('results.tar.gz','matrix.json','asan18.json','assembly.json','inputs.json'):
                (folder/name).symlink_to(reader.HERE/name)
            if mutation=='source': raw['commit']='0'*40; compact['commit']='0'*40
            elif mutation=='generation': raw['closing_generation']='wrong'; compact['closing_generation']='wrong'
            elif mutation=='cleanup': raw['oslogin_key_removed']=False; compact['oslogin_key_removed']=False
            elif mutation=='missing_raw': raw.pop('targeted_shutdown_certified')
            elif mutation=='manifest': raw['data_files'][0]['sha256']='0'*64; compact['data_files']=copy.deepcopy(raw['data_files'])
            elif mutation=='copy': (folder/'assembly.json').unlink(); (folder/'assembly.json').write_text('{}\n')
            else: raw['results_sha256']='0'*64; compact['results_sha256']='0'*64
            raw_bytes = (json.dumps(raw,sort_keys=True)+'\n').encode(); (folder/'raw.json').write_bytes(raw_bytes)
            (folder/'DONE').write_text('0\n'); compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=reader.sha(raw_bytes))
            (folder/'receipt.json').write_text(json.dumps(compact)+'\n')
            reject(lambda:reader.check(folder))
    mutated = dict(data); summary = reader.js(data[reader.BASE+'summary.json'])
    next(c for c in summary['configurations'] if c['name']=='mutants')['threads'] = 12
    mutated[reader.BASE+'summary.json'] = json.dumps(summary).encode(); reject(lambda:reader.matrix_parameters(mutated))
    print(json.dumps(dict(positives=positives,corruptions=corruptions,native=0),sort_keys=True))


if __name__=='__main__': main()
