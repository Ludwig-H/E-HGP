"""Tiny pure-reader fixtures: no native child, build, data payload, or cloud access."""
import copy
from dataclasses import asdict
import json
from pathlib import Path
import re
import struct
import tempfile
from unittest.mock import patch

import check as r


def semantic(case,bits,k=5):
    n = case['count']; orders = []
    for order in range(1,k+1):
        nodes = 2*n-1
        orders.append(dict(order=order,births=n,merges=n-1,nodes=nodes,edges=nodes-1,
                           root=nodes-1,verticals=nodes if order>1 else 0))
    value = dict(schema=r.full.semantic.SCHEMA,sha256=r.sha(case['name'].encode()),
                 raw_sha256=r.sha(('%s:%d' % (case['name'],bits)).encode()),coord_bits=bits,kmax=k,
                 sites=n,points=n,orders=orders)
    for key in ('births','merges','nodes','edges','verticals'): value[key] = sum(o[key] for o in orders)
    value['bytes'] = 1066+40*n+40*k+72*value['nodes']+96*value['births']+8*(value['verticals']+value['edges'])
    return value


def record(request,case,build):
    bits,mode = request['coord_bits'],request['optimizations']; n = case['count']
    sem = semantic(case,bits); orders = []
    for decoded in sem['orders']:
        k = decoded['order']; work = dict.fromkeys(r.full.WORK,0)
        if k>1:
            work.update(vertical_descents=n,vertical_checks=2*n-2,ancestor_queries=3*n-2,
                        descent_steps=n,part_meb_presentations=n,part_diameter_pairs=n)
        if mode==7:
            queries = work['vertical_descents']; hits = 1 if k==3 else 0
            work.update(memo_queries=queries,memo_lookups=queries,memo_hits=hits,
                        memo_misses=queries-hits,memo_insertions=queries-hits,
                        descent_steps=queries-hits,part_meb_presentations=queries-hits,
                        part_diameter_pairs=queries-hits)
        orders.append(dict(k=k,**{key:decoded[key] for key in ('births','nodes','edges','verticals')},
                           node_capacity=2*n-1,edge_capacity=2*n-2,work=work,
                           timings=dict(classify_ns=10,births_ns=10,plateaus_ns=10,verticals_ns=10 if k>1 else 0)))
    capacity = 65536 if mode==7 else 0; slot = 192 if bits==21 else 208
    retained = sum(64*n-32 if o['k']==1 else 72*n-36 for o in orders)+1000
    ev = [dict(phase='cloud',read_ns=10,cloud_ns=10,cloud_peak_bytes=100,sites=n,points=n),
          dict(phase='domain',index_ns=10,domain_ns=100,catalogue_balls=n,pool_ns=10,sort_ns=10,count_ns=10,fill_ns=10),
          dict(phase='full',status='ok',reason='none',coord_bits=bits,kmax=5,workers=48,optimizations=mode,
               wall_ns=1000,index_ns=10,domain_ns=100,forest_ns=800,cpu_seconds=.0001,
               peak_reserved_bytes=retained+10000+capacity*slot,reserved_after_bytes=retained,
               memo_capacity=capacity,memo_slot_bytes=slot,memo_reserved_bytes=capacity*slot,orders=orders),
          dict(phase='exit',status='ok',reason='none')]
    argv = [build['path'],'/data/'+case['coordinates'],'/data/'+case['point_ids'],
            '/work/%s_b%d_k5_w48_r0_o%d.bin' % (case['name'],bits,mode),
            '5','16','256','0',str(2**32-1),str(8*1024**3),'48',str(mode)]
    row = dict(request,argv=argv,timeout_seconds=60,whole_input=True,count=n,exit_code=0,
               stdout='',stderr='',events=ev,errors=[],status='exited',process_wall_seconds=.1)
    with patch.object(r.full.semantic,'inspect',lambda *_:copy.deepcopy(sem)):
        r.full.collect(row,case,None,bits)
    r.need(row['status']=='ok','fixture_collect')
    row['semantic_wall_seconds'] = .2
    context = r.full.reuse.Context('MHGP11FUL1',r.full.semantic.SCHEMA,
        r.full.reuse.decoder_digest([Path(r.full.semantic.__file__),Path(r.full.profiles.semantic.__file__)]),
        bits,5,n,case['sha256'],case['ids_sha256'])
    origin = list(r.identity(row)); origin[-1] = 3
    row['semantic_reuse'] = dict(schema=r.full.reuse.SCHEMA,mode='decoded' if mode==3 else 'reused',
        context=asdict(context),current_attempt=list(r.identity(row)),source_attempt=origin,
        raw_sha256=sem['raw_sha256'],bytes=sem['bytes'],hash_wall_seconds=.01,decode_wall_seconds=.1 if mode==3 else 0)
    row['stdout'] = ''.join(json.dumps(e)+'\n' for e in ev)
    intent = dict(request,argv=argv,timeout_seconds=60,whole_input=True,count=n,
                  input_sha256=case['sha256'],ids_sha256=case['ids_sha256'])
    return row,intent


def fixture():
    manifest = dict(cases=[dict(name=name,count=count,coordinates=name+'.u32le',point_ids=name+'.ids.u32le',
        sha256=r.sha((name+'XYZ').encode()),ids_sha256=r.sha((name+'ID').encode())) for name,count in r.old.CASE_COUNTS.items()])
    data = {r.BASE+'summary.json':b'{}',r.SUPP+'summary.json':b'{}'}; builds = {}; records = []
    for bits,name in ((18,'gcc_release'),(21,'bits21'),(24,'bits24')):
        h = r.sha(name.encode()); data[r.BASE+name+'/build_provenance.json'] = name.encode()
        builds[name] = {'mhgp11_full_bench':dict(sha256=h,size=123),'CMakeCache.txt':dict(sha256=h)}
        records.append(dict(configuration=name,coord_bits=bits,path='/matrix/'+name+'/build/mhgp11_full_bench',
                            sha256=h,bytes=123,cache_sha256=h,provenance_sha256=h))
    bybits = {b['coord_bits']:b for b in records}; cases = {c['name']:c for c in manifest['cases']}
    requested = r.driver.schedule(); rows = []; intents = []
    for request in requested:
        row,intent = record(request,cases[request['case']],bybits[request['coord_bits']]); rows.append(row); intents.append(intent)
    value = dict(schema=r.driver.SCHEMA,work_schema='ehgp.v11.full_work.v4',manifest=manifest,
        manifest_sha256=r.sha(json.dumps(manifest).encode()),qualification_sha256=r.sha(b'{}'),supplement_sha256=r.sha(b'{}'),
        builds=records,requested=requested,requested_runs=18,timeout_seconds=60,budget_seconds=550,leaf_size=16,
        max_leaf=256,memo_capacity=65536,semantic_reuse_enabled=True,invariant_work=sorted(r.driver.INVARIANT_WORK),
        variable_work=sorted(r.driver.VARIABLE_WORK),optimization_modes={'3':'cache_J2_indirect_sort','7':'cache_J2_indirect_sort_tuple_memo'},
        complete=True,conforming=True,full_schedule_completed=True,runs=rows,launch_intents=intents,not_run=[],
        comparisons=r.driver.comparisons(rows,requested),campaign_wall_seconds=10.)
    return value,manifest,builds,data


def main():
    value,manifest,builds,data = fixture(); positives = refused = 0
    def judge(v): return r.benchmark(v,manifest,value['manifest_sha256'],builds,data)
    def reject(action):
        nonlocal refused
        try: action()
        except r.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError): refused += 1; return
        raise ValueError('corruption accepted')
    r.need(judge(value)['conforming'],'complete_fixture'); positives += 1
    mutations = [
        lambda v:v['runs'].pop(),lambda v:v['runs'].append(copy.deepcopy(v['runs'][0])),
        lambda v:v['runs'].reverse(),lambda v:v['requested'][0].update(coord_bits=True),
        lambda v:v['launch_intents'].pop(),lambda v:v['launch_intents'][0].update(input_sha256='0'*64),
        lambda v:v['runs'][0]['argv'].__setitem__(11,'7'),lambda v:v.update(work_schema='old'),
        lambda v:v.update(qualification_sha256='0'*64),lambda v:v.update(supplement_sha256='0'*64),
        lambda v:v['builds'][1].update(bytes=True),lambda v:v.update(memo_capacity=True),
        lambda v:v.update(campaign_wall_seconds=float('nan')),lambda v:v.update(campaign_wall_seconds=1),
        lambda v:v['runs'][0].update(semantic_wall_seconds=True),lambda v:v['runs'][0].update(exit_code=True),
        lambda v:v['runs'][0].update(stdout=v['runs'][0]['stdout']+'{}\n'),
        lambda v:v['runs'][0].update(full_ms=1),lambda v:v['runs'][0].update(stderr='diagnostic'),
        lambda v:v['runs'][0]['semantic'].update(bytes=1),lambda v:v['runs'][0]['semantic'].update(nodes=1),
        lambda v:v['runs'][0].pop('semantic_reuse'),lambda v:v['runs'][1]['semantic_reuse'].update(source_attempt=r.identity(v['runs'][3])),
        lambda v:v['runs'][1]['semantic_reuse'].update(source_attempt=r.identity(v['runs'][1])),
        lambda v:v['runs'][1]['semantic_reuse'].update(decode_wall_seconds=1),
        lambda v:v['runs'][1]['semantic_reuse']['context'].update(decoder_sha256='0'*64),
        lambda v:v['runs'][1]['semantic_reuse'].update(raw_sha256='0'*64),
        lambda v:v['runs'][1]['semantic_reuse'].update(bytes=1),
        lambda v:v['runs'][1]['semantic_reuse'].update(current_attempt=r.identity(v['runs'][0])),
        lambda v:v['runs'][1]['semantic_reuse'].update(hash_wall_seconds=-1),
        lambda v:v['runs'][0]['semantic_reuse'].update(source_attempt=r.identity(v['runs'][1])),
        lambda v:v['runs'][0]['semantic_reuse'].update(mode='reused'),
        lambda v:v['comparisons'][0].update(status='incomplete'),lambda v:v['invariant_work'].pop(),
        lambda v:v['variable_work'].append('cells'),lambda v:v['optimization_modes'].update({'7':'adaptive'}),
        lambda v:v.update(semantic_reuse_enabled=False)]
    for mutate in mutations:
        changed = copy.deepcopy(value); mutate(changed); reject(lambda:judge(changed))
    event_mutations = [
        lambda e:e[2].update(cpu_seconds=float('nan')),lambda e:e[2].update(memo_slot_bytes=256),
        lambda e:e[2].update(memo_reserved_bytes=0),lambda e:e[2].update(memo_capacity=0),
        lambda e:e[2].update(peak_reserved_bytes=e[2]['memo_reserved_bytes']),
        lambda e:e[2].update(reserved_after_bytes=1),
        lambda e:e[2]['orders'][2]['work'].update(memo_queries=0),
        lambda e:e[2]['orders'][2]['work'].update(memo_lookups=0),
        lambda e:e[2]['orders'][2]['work'].update(memo_hits=2**63),
        lambda e:e[2]['orders'][2]['work'].update(memo_suffix_hits=2),
        lambda e:e[2]['orders'][2]['work'].update(memo_insertions=0),
        lambda e:e[2]['orders'][2]['work'].update(memo_collisions=2**63),
        lambda e:e[2]['orders'][2]['work'].update(memo_evictions=2**63),
        lambda e:e[2]['orders'][2]['work'].update(part_diameter_pairs=2**63),
        lambda e:e[2]['orders'][2]['work'].update(classification_examined=1),
        lambda e:e[2]['orders'][2]['work'].update(ancestor_hops=1),
        lambda e:e[2]['orders'][2]['work'].update(vertical_checks=0),
        lambda e:e[2]['orders'][0]['timings'].update(verticals_ns=1),
        lambda e:e[2]['orders'][0]['timings'].update(classify_ns=2**63),
        lambda e:e[2]['orders'][0].update(node_capacity=0)]
    for mutate in event_mutations:
        changed = copy.deepcopy(value); row = changed['runs'][1]; mutate(row['events'])
        row['stdout'] = ''.join(json.dumps(e)+'\n' for e in row['events'])
        changed['comparisons'] = r.driver.comparisons(changed['runs'],changed['requested'])
        reject(lambda:judge(changed))
    partial = copy.deepcopy(value); partial.update(complete=False,conforming=False,full_schedule_completed=False,
        runs=partial['runs'][:1],launch_intents=partial['launch_intents'][:1])
    row = partial['runs'][-1]
    for k in ('semantic','semantic_reuse','semantic_wall_seconds',*r.DERIVED): row.pop(k,None)
    row['status'] = 'pending_semantic'; partial['comparisons'] = r.driver.comparisons(partial['runs'],partial['requested'])
    r.need(not judge(partial)['conforming'],'pending_positive'); positives += 1
    changed = copy.deepcopy(partial); changed['complete'] = True; reject(lambda:judge(changed))
    omitted = copy.deepcopy(value); omitted['runs'] = omitted['runs'][:2]; omitted['launch_intents'] = omitted['launch_intents'][:2]
    omitted.update(conforming=False,full_schedule_completed=False,campaign_wall_seconds=470.,
        not_run=[dict(row,reason='campaign_budget_before_launch') for row in omitted['requested'][2:]])
    omitted['comparisons'] = r.driver.comparisons(omitted['runs'],omitted['requested'])
    r.need(not judge(omitted)['conforming'],'omission_positive'); positives += 1
    changed = copy.deepcopy(omitted); changed['not_run'][0]['reason'] = 'other_mode_failed'; reject(lambda:judge(changed))
    for status,code,errors,stdout in (
        ('timeout',None,[dict(stage='process',type='TimeoutExpired',message='')],'partial'),
        ('refused',2,[],'{}\n'),('failed',-9,[],''),
        ('launch_error',None,[dict(stage='launch',type='OSError',message='')],''),
        ('invalid_output',0,[dict(stage='events',type='ValueError',message='')],'partial')):
        changed = copy.deepcopy(value); row = changed['runs'][0]
        for k in ('semantic','semantic_reuse','semantic_wall_seconds',*r.DERIVED): row.pop(k,None)
        if stdout=='partial' and status=='timeout': errors += [dict(stage='events',type='ValueError',message='')]
        row.update(status=status,exit_code=code,stdout=stdout,errors=errors,events=[{}] if stdout=='{}\n' else [])
        # A failed attempt never publishes a reusable entry. Its paired success must decode itself.
        pair = changed['runs'][1]; pair['semantic_reuse'].update(mode='decoded',source_attempt=list(r.identity(pair)),decode_wall_seconds=.1)
        changed.update(conforming=False,comparisons=r.driver.comparisons(changed['runs'],changed['requested']))
        r.need(not judge(changed)['conforming'],'failure_positive'); positives += 1
        pair['semantic_reuse'].update(mode='reused',source_attempt=list(r.identity(row)),decode_wall_seconds=0)
        reject(lambda:judge(changed))
    # Corruption of all recorded bytes/summary of a reused entry cannot borrow a different source.
    changed = copy.deepcopy(value); row = changed['runs'][1]
    row['semantic']['raw_sha256'] = row['semantic_reuse']['raw_sha256'] = '0'*64
    changed['comparisons'] = r.driver.comparisons(changed['runs'],changed['requested']); changed['conforming'] = False
    reject(lambda:judge(changed))
    # Recorded-configuration controls do not invoke CMake or CTest.
    matrices = {}
    for prefix,source,budget,threads in ((r.BASE,r.PINS['matrix'],700,48),(r.SUPP,r.PINS['supplement'],160,12)):
        configs = []
        for item in source['configurations']:
            c = {k:copy.deepcopy(item[k]) for k in ('name','threads','compiler','cmake_options','ctest_args')}
            c.update(optional=item.get('optional',False),steps=[dict(name='configure',argv=['cmake']+
                [s.replace('{threads}',str(c['threads'])) for s in item['cmake_options']])]); configs.append(c)
        matrices[prefix+'summary.json'] = json.dumps(dict(configurations=configs,budget_seconds=budget,thread_budget=threads)).encode()
    r.matrix_parameters(matrices); positives += 1
    for mutate in (
        lambda v:v.update(budget_seconds=750),lambda v:v.update(thread_budget=32),
        lambda v:v['configurations'][1].update(threads=12),lambda v:v['configurations'][1].update(optional=True),
        lambda v:v['configurations'][1]['cmake_options'].pop(),
        lambda v:v['configurations'][1].update(ctest_args=['-R','nothing']),
        lambda v:v['configurations'][1]['steps'][0]['argv'].__setitem__(-2,'-DMHGP11_MUTANT_JOBS=12'),
        lambda v:v['configurations'].pop()):
        changed = copy.deepcopy(matrices); m = r.js(changed[r.BASE+'summary.json']); mutate(m)
        changed[r.BASE+'summary.json'] = json.dumps(m).encode(); reject(lambda:r.matrix_parameters(changed))
    # An actual tiny codec payload covers the optional archived-byte path.
    words = [21,1,1,1,0,0,0,1,7,1,1,1,0,0,2**32-1,0,0]
    for integer in (0,1,0,0,0,1): words += [0,1,integer]
    raw = b'MHGP11FUL1'+struct.pack('<'+'Q'*len(words),*words)
    summary = r.full.semantic.decode(raw,21,1,1)
    row = copy.deepcopy(value['runs'][0]); row.update(kmax=1,count=1,semantic=summary)
    row.pop('semantic_reuse'); row['argv'][4] = '1'
    row['argv'][3] = str(Path(row['argv'][3]).with_name('%s_b21_k1_w48_r0_o3.bin' % row['case']))
    case = dict(manifest['cases'][0],count=1)
    row['events'][0].update(sites=1,points=1); e = row['events'][2]; e.update(kmax=1)
    e['orders'] = [dict(k=1,births=1,nodes=1,edges=0,verticals=0,node_capacity=1,edge_capacity=0,
                       work=dict.fromkeys(r.full.WORK,0),timings=dict.fromkeys(r.full.ORDER_TIMINGS,0))]
    with patch.object(r.full.semantic,'inspect',lambda *_:summary): r.full.collect(row,case,None,21)
    row['stdout'] = ''.join(json.dumps(e)+'\n' for e in row['events'])
    build = next(b for b in value['builds'] if b['coord_bits']==21)
    path = 'results/cmd/002_memo/files/'+Path(row['argv'][3]).name
    r.need(r.attempt(row,case,build,True,True,{path:raw},{})==1,'archived_payload'); positives += 1
    reject(lambda:r.attempt(row,case,build,True,True,{path:raw+b'!'},{}))
    reject(lambda:r.attempt(row,case,build,True,True,{path:raw,path.replace('/files/','/other/'):raw},{}))
    r.need(len(r.analysis(value)['complete_pairs'])==9,'analysis_pairs'); positives += 1
    changed = copy.deepcopy(value); changed['runs'] = changed['runs'][:1]
    r.need(not r.analysis(changed)['complete_pairs'],'analysis_missing_pair'); positives += 1
    previous = dict(manifest=manifest,manifest_sha256=value['manifest_sha256'],runs=[copy.deepcopy(row)
        for row in value['runs'] if row['case'].startswith('lidar') and row['optimizations']==3])
    r.need(len(r.compare_sweep(value,previous))==12,'sweep_matching_outputs'); positives += 1
    changed = copy.deepcopy(previous)
    for row in changed['runs']:
        for order in row['events'][2]['orders']: order['work']['part_meb_presentations'] += 17
    r.need(len(r.compare_sweep(value,changed))==12,'changed_MEB_work_not_inherited'); positives += 1
    for mutate in (
        lambda v:v['runs'][0]['semantic'].update(raw_sha256='0'*64),
        lambda v:v['runs'][0]['semantic'].update(sha256='0'*64),
        lambda v:v['runs'][0]['events'][2]['orders'][1]['work'].update(vertical_checks=0),
        lambda v:v['runs'][0]['events'][2]['orders'][1]['work'].update(descent_steps=0),
        lambda v:v.update(manifest_sha256='0'*64)):
        changed = copy.deepcopy(previous); mutate(changed); reject(lambda:r.compare_sweep(value,changed))
    if (r.HERE/'receipt.json').exists():
        verdict = r.check()
        r.need(verdict['coherent'] and verdict['attempts']==17 and not verdict['conforming'] and
               verdict['mutants_code_or_line']==238 and verdict['mutants_expected_construction']==2,
               'LIVE_failure_preserved')
        positives += 1
        receipt,worker,archived = r.old.read_capture(r.HERE)
        r.need(r.mutants(archived)==240,'LIVE_causal_mutants'); positives += 1
        r.required_gates(archived); positives += 1
        changed = dict(archived)
        path = r.BASE+'bits21/tests.json'; rows = r.js(changed[path])
        changed[path] = json.dumps([x for x in rows if x['name']!='mhgp11_tower_memo_fault_starvation']).encode()
        reject(lambda:r.required_gates(changed))
        changed = dict(archived)
        # Match whitespace without relying on the runner's column padding.
        ident = next(iter(r.PINS['mutants']['tower']))
        pattern = re.compile(re.escape(ident).encode()+rb'(\s+TUE\s+)(?:code|ligne)')
        hits = 0
        for name,raw in list(changed.items()):
            if name.startswith(r.BASE+'mutants/'):
                updated,count = pattern.subn(lambda m:ident.encode()+m[1]+b'signal',raw)
                changed[name] = updated; hits += count
        r.need(hits>0,'causal_mutation_present'); reject(lambda:r.mutants(changed))
        # Raw-field absence with a coherent new hash reaches our mandatory-key guard;
        # neither an old hash mismatch nor compact/raw overlap explains this refusal.
        with tempfile.TemporaryDirectory(prefix='memo-receipt-check-') as td:
            folder = Path(td)
            for name in ('results.tar.gz','matrix.json','asan18.json','full_memo.json'):
                (folder/name).symlink_to(r.HERE/name)
            (folder/'DONE').write_text('3\n')
            original = r.js(Path(receipt['raw_receipt_local']).read_bytes())
            def trial(change_raw,change_compact):
                raw = copy.deepcopy(original); compact = copy.deepcopy(receipt)
                change_raw(raw); change_compact(compact)
                encoded = json.dumps(raw,sort_keys=True).encode(); (folder/'raw.json').write_bytes(encoded)
                compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=r.sha(encoded))
                (folder/'receipt.json').write_text(json.dumps(compact,sort_keys=True))
                return r.transport(folder)
            trial(lambda _:None,lambda _:None); positives += 1
            reject(lambda:trial(lambda v:v.pop('targeted_shutdown_certified'),lambda _:None))
            reject(lambda:trial(lambda v:v.update(oslogin_key_removed=False),lambda v:v.update(oslogin_key_removed=False)))
            reject(lambda:trial(lambda v:v.update(results_sha256='0'*64),lambda v:v.update(results_sha256='0'*64)))
            reject(lambda:trial(lambda v:v.update(warnings=['unexpected']),lambda v:v.update(warnings=['unexpected'])))
            reject(lambda:trial(lambda v:v.update(results_skipped_members=['missing']),lambda v:v.update(results_skipped_members=['missing'])))
    print(json.dumps(dict(positives=positives,corruptions=refused,native=0),sort_keys=True))


if __name__=='__main__': main()
