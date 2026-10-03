"""Synthetic reader checks only; no native execution or geometric qualification."""
import copy
import gzip
import json
from pathlib import Path
import sys
import tempfile

import check as C

CHECKS = 0


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def rejected(action, message):
    try:
        action()
    except C.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,EOFError):
        need(True, message)
    else:
        raise ValueError('accepted corruption: '+message)


def telemetry(bits, workers, mode, count):
    D = C.driver.diagnostics
    ledger = dict.fromkeys(D.LEDGER_FIELDS, 0)
    ledger.update(leaves=1,prefixes=11,judged=11,census_tests=44,emitted=11,incidences=28,
                  q4_candidates=1,q4_levels=1,region_pair_tests=12,region_line_tests=7,
                  region_line_evaluations=3,region_line_cache_hits=4,max_leaf=4)
    task = dict(ordinal=0,path=[0,0],path_known=bool(mode&4),inside_known=bool(mode&4),lo=[0,0,0],hi=[8,8,8],
                depth=0,count=count,capacity=count,inside=count if mode&4 else 0,count_ns=1,fill_ns=1,
                ledger=ledger)
    total = dict(ledger,nodes=1,filter_tests=16)
    planning = dict(adaptive=bool(mode&4),memory_fallback=False,plan_nodes=1,plan_leaves=1,empty_leaves=0,
                    rounds=0,priority_tests=0,replay_bytes=(8 if mode&4 else 40)*count)
    event = dict(phase='catalogue',status='ok',reason='none',coord_bits=bits,kmax=5,balls=11,levels=4,
                 incidences=28,workers=workers,pool_ns=4,wall_ns=22,peak_reserved_bytes=28*count+8192,
                 reserved_after_bytes=28*count+4096,generation_passes=2,optimizations=mode,
                 diagnostics_requested=True,cpu_seconds=0.000001,
                 timings=dict.fromkeys(C.driver.parallel.TIMING_FIELDS,1),
                 logical={k:total[k] for k in C.driver.profiles.LOGICAL},
                 work=dict(q4_candidates=1,q4_levels=1),cache_work=dict(evaluations=3,hits=4,fallbacks=0),
                 diagnostics=dict(schema=D.SCHEMA,record_bytes=264,reserved_bytes=264,
                                  planning=planning,tasks=[task]))
    return [dict(phase='cloud',points=count,sites=count,cloud_ns=20,read_ns=10,cloud_peak_bytes=28*count+8),
            event,dict(phase='exit',status='ok',reason='none')]


def models():
    requests = C.driver.schedule()
    cases = {name:dict(name=name,count=count,coordinates=name+'.u32',point_ids=name+'.ids',
                      sha256=C.sha(name.encode()),ids_sha256=C.sha((name+'ids').encode()))
             for name,count in C.driver.profiles.COUNTS.items()}
    builds = {bits:dict(path='/fake/'+C.legacy.PROFILES[bits]+'/build/mhgp11_catalogue_bench')
              for bits in (18,21,24)}
    rows, sources = [], {}
    decoder = C.driver.reuse.decoder_digest([Path(C.driver.profiles.semantic.__file__)])
    for request in requests:
        name,bits,k,workers,repeat,mode = C.identity(request)
        case = cases[name]; count = case['count']
        events = telemetry(bits,workers,mode,count)
        semantic = dict(schema=C.driver.profiles.semantic.SCHEMA,sha256=C.sha(('semantic'+name).encode()),
                        coord_bits=bits,kmax=k,sites=count,levels=4,balls=11,incidences=28)
        limbs = [(b+63)//64 if b>127 else 2 for b in (8*bits+12,6*bits+8)]
        size = 58+40*count+8*(4+sum(limbs))*4+72*11+8*28
        identity = [name,bits,k,workers,repeat,mode,True]
        origin = sources.setdefault((name,bits),identity)
        evidence = dict(schema=C.driver.reuse.SCHEMA,mode='decoded' if identity==origin else 'reused',
                        current_attempt=identity,source_attempt=origin,
                        context=dict(format='MHGP11CAT1',decoder_version=C.driver.profiles.semantic.SCHEMA+';arity_counts=true',
                         decoder_sha256=decoder,coord_bits=bits,kmax=k,count=count,
                         xyz_sha256=case['sha256'],ids_sha256=case['ids_sha256']),
                        raw_sha256=C.sha((name+str(bits)).encode()),bytes=size,hash_wall_seconds=0.002,
                        decode_wall_seconds=0.003 if identity==origin else 0.0)
        row = dict(request,status='ok',count=count,stderr='',stdout='\n'.join(map(json.dumps,events)),errors=[],
                   exit_code=0,events=events,cloud_ms=20/1e6,read_ms=10/1e6,catalogue_ms=22/1e6,
                   catalogue_within_100ms=True,semantic=semantic,qmin_counts={'2':6,'3':4,'4':1},
                   canonical_sha256=evidence['raw_sha256'],canonical_bytes=size,semantic_wall_seconds=0.01,
                   process_wall_seconds=0.1,whole_input=True,timeout_seconds=15,semantic_reuse_requested=True,
                   semantic_reuse=evidence,
                   argv=[builds[bits]['path'],'/fake/'+case['coordinates'],'/fake/'+case['point_ids'],
                         '/fake/%s_b%d_k%d_w%d_r%d_o%d_d1.bin' % (name,bits,k,workers,repeat,mode),
                         str(k),'16','256','0',str(2**32-1),str(8*1024**3),str(workers),str(mode),'1'])
        saved = sys.modules.get('catalogue_parallel')
        sys.modules['catalogue_parallel'] = C.driver.parallel
        try:
            C.driver.check_adaptive(row,request)
        finally:
            if saved is None:
                sys.modules.pop('catalogue_parallel',None)
            else:
                sys.modules['catalogue_parallel'] = saved
        need(row['status']=='ok','synthetic telemetry')
        rows.append(row)
    intents = [dict(r,input_sha256=cases[r['case']]['sha256'],ids_sha256=cases[r['case']]['ids_sha256']) for r in rows]
    report = dict(requested=requests,requested_runs=36,timeout_seconds=15,native_schedule_bound_seconds=540,
                  budget_seconds=550,leaf_size=16,max_leaf=256,complete=True,conforming=True,
                  full_schedule_completed=True,runs=rows,not_run=[],launch_intents=intents,campaign_wall_seconds=5,
                  comparisons=C.js(json.dumps(C.driver.comparisons(rows,requests))),
                  semantic_reuse=dict(schema=C.driver.reuse.SCHEMA,capacity=64,summary_limit_bytes=65536,
                    assumption='SHA256 collision resistance; immutable campaign artifacts',
                    scope='current payload fully rehashed; only validated summaries reused; all current-event checks repeated'))
    return report,cases,builds


def main():
    report,cases,builds = models()
    for i,row in enumerate(report['runs']):
        need(C.attempt(row,cases[row['case']],builds[row['coord_bits']],True,i==35,{})==0,'recorded-only artifact')
    need(C.schedule(report)['successes']==36,'inventory36')
    need(C.reuse_chain(report['runs'],cases)==dict(decoded=12,reused=24,published=12),'reuse12/24')
    # Every complete context member is tied to the actual attempted case, not only the raw digest.
    changes = []
    for key in report['runs'][1]['semantic_reuse']['context']:
        def action(v,key=key):
            old = v[1]['semantic_reuse']['context'][key]
            v[1]['semantic_reuse']['context'][key] = old+1 if type(old) is int else old+'x'
        changes.append(('context_'+key,action))
    changes += [('raw',lambda v:v[1]['semantic_reuse'].update(raw_sha256='0'*64)),
                ('size',lambda v:v[1]['semantic_reuse'].update(bytes=1)),
                ('origin',lambda v:v[1]['semantic_reuse'].update(source_attempt=v[1]['semantic_reuse']['current_attempt'])),
                ('current',lambda v:v[1]['semantic_reuse'].update(current_attempt=['other'])),
                ('miss_hit',lambda v:v[0]['semantic_reuse'].update(mode='reused')),
                ('hit_decoded',lambda v:v[1]['semantic_reuse'].update(mode='decoded')),
                ('hit_clock',lambda v:v[1]['semantic_reuse'].update(decode_wall_seconds=0.001)),
                ('negative_clock',lambda v:v[1]['semantic_reuse'].update(hash_wall_seconds=-1)),
                ('large_clock',lambda v:v[1]['semantic_reuse'].update(hash_wall_seconds=1)),
                ('nan_clock',lambda v:v[1]['semantic_reuse'].update(hash_wall_seconds=float('nan'))),
                ('summary',lambda v:v[1]['semantic'].update(sha256='0'*64)),
                ('arity',lambda v:v[1]['qmin_counts'].update({'4':0})),
                ('failed_origin',lambda v:v[0].update(status='invalid_output')),
                ('missing',lambda v:v[1].pop('semantic_reuse')),
                ('unsolicited',lambda v:v[1].update(semantic_reuse_requested=False))]
    for name,action in changes:
        value = copy.deepcopy(report['runs']); action(value)
        rejected(lambda:C.reuse_chain(value,cases),name)
    changed = copy.deepcopy(report['runs'])
    changed[0]['status'] = 'artifact_error'
    changed[1]['semantic_reuse'].update(mode='decoded',source_attempt=changed[1]['semantic_reuse']['current_attempt'],
                                      decode_wall_seconds=0.003)
    # All subsequent hits must now point to the new validated origin.
    origin = changed[1]['semantic_reuse']['current_attempt']
    for row in changed[2:]:
        if (row['case'],row['coord_bits']) == (changed[1]['case'],changed[1]['coord_bits']):
            row['semantic_reuse']['source_attempt'] = origin
    need(C.reuse_chain(changed,cases)==dict(decoded=13,reused=23,published=12),'failure never published')
    for label,edit in [('missing',lambda v:v['runs'].pop()),
                       ('order',lambda v:v['runs'].reverse()),
                       ('budget',lambda v:v.update(budget_seconds=700)),
                       ('comparison',lambda v:v['comparisons'][0].update(status='different')),
                       ('false_complete',lambda v:v.update(full_schedule_completed=False)),
                       ('paid_time',lambda v:v.update(campaign_wall_seconds=0)),
                       ('extra_intent',lambda v:v['launch_intents'].append(v['launch_intents'][0]))]:
        value = copy.deepcopy(report); edit(value)
        rejected(lambda:C.schedule(value),'schedule_'+label)
    for label,edit in [('worker',lambda r:r['events'][1].update(workers=1)),
                       ('ledger',lambda r:r['events'][1]['logical'].update(prefixes=10)),
                       ('population',lambda r:r['events'][1]['diagnostics']['tasks'][0].update(inside=0)),
                       ('wall',lambda r:r['events'][1].update(wall_ns=1)),
                       ('semantic',lambda r:r['semantic'].update(balls=12)),
                       ('ABI',lambda r:r['events'][1]['diagnostics'].update(record_bytes=263))]:
        value = copy.deepcopy(report['runs'][1]); edit(value)
        value['stdout'] = '\n'.join(map(json.dumps,value['events']))
        rejected(lambda:C.attempt(value,cases[value['case']],builds[value['coord_bits']],True,False,{}),
                 'current_event_despite_hit_'+label)
    pending = copy.deepcopy(report); pending.update(complete=False,conforming=False,full_schedule_completed=False)
    pending['runs'] = pending['runs'][:1]; pending['launch_intents'] = pending['launch_intents'][:1]
    pending['runs'][0]['status'] = 'pending_semantic'
    pending['comparisons'] = C.js(json.dumps(C.driver.comparisons(pending['runs'],pending['requested'])))
    need(C.schedule(pending)['attempts']==1,'pending preserved')
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)/'model.json.gz'
        C.driver.save_report(path,report)
        raw = path.read_bytes()
        need(C.gzip_report(raw)==report,'gzip exact roundtrip')
        C.driver.save_report(path,report)
        need(path.read_bytes()==raw,'gzip deterministic')
        rejected(lambda:C.gzip_report(raw[:-5]),'truncated gzip')
        rejected(lambda:C.gzip_report(raw,100),'bounded gzip')
        for value in (b'{"x":1,"x":2}',b'{"x":NaN}',b'[]',b'{'):
            rejected(lambda value=value:C.gzip_report(gzip.compress(value)), 'strict gzip JSON')
    # Actual closed evidence, read only. Counterexamples alter in-memory copies, never the capture.
    result = C.check()
    need(result['coherent'] and not result['conforming'] and result['attempts']==0 and
         result['matrix_passed']==2474 and result['matrix_total']==2475 and result['asan18_passed']==178 and
         result['causal_mutants'] is None and result['individual_mutant_verdicts']=='not_certified' and
         result['unpersisted']==0 and result['unstarted_declared_units']==36,
         'closed failed capture')
    _,_,data = C.old.read_capture(C.HERE)
    config = C.js(data[C.BASE+'mutants/result.json'])
    for name,action in [('threads',lambda c:c.update(threads=32)),
                        ('state',lambda c:c.update(status='ok')),
                        ('conforming',lambda c:c.update(conforming=True)),
                        ('missing',lambda c:c.update(not_run=[])),
                        ('passed',lambda c:c['tests'].update(passed=21)),
                        ('deadline',lambda c:c['steps'][-1].update(timed_out=False)),
                        ('exit',lambda c:c['steps'][-1].update(exit_code=0)),
                        ('gate',lambda c:c['not_run'][0].update(test='other'))]:
        value = copy.deepcopy(config); action(value)
        changed = dict(data); changed[C.BASE+'mutants/result.json'] = json.dumps(value).encode()
        rejected(lambda:C.timeout_mutants(value,changed),'captured_timeout_'+name)
    for name,raw in [('truncated',data[C.BASE+'mutants/ctest.log'].rsplit(b'\n',2)[0]),
                     ('forged_pass',data[C.BASE+'mutants/ctest.log']+b'21/21 Test #492: mhgp11_mutants_tower ... Passed 0.01 sec\n')]:
        changed = dict(data); changed[C.BASE+'mutants/ctest.log'] = raw
        rejected(lambda:C.timeout_mutants(config,changed),'captured_log_'+name)
    print(json.dumps(dict(reader_selftest='conforme',checks=CHECKS,synthetic_attempts=36,
                          decoded=12,reused=24,captured_failure=True,native=0),sort_keys=True))


if __name__ == '__main__':
    main()
