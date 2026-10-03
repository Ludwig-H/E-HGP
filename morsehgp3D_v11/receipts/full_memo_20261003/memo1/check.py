"""LIVE memo1 evidence reader. Code0 means coherent evidence, not necessarily a passed campaign.

Only frozen c2 helpers are imported. Large deleted payloads remain recorded hashes;
any archived payload is rehashed and decoded. No native execution or tar extraction.
"""
import copy
from dataclasses import asdict
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
RECEIPTS = HERE.parent.parent
SOURCE = 'c2c3e0323afe25feadf9ed2c2ab85887ab1be3db'
CONTRACT = 'd93b458da0d05ab8e4c0ddac8bf34e91522376d14e95a02f52c440f744b45cf1'
COMMANDS = ('000_matrice','001_asan18','002_memo')
SUPP = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_memo/files/full_memo.json'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


sweep = load('memo_sweep_helpers',RECEIPTS/'full_sweep_20261002/check.py')
baseline,optim,q4,old = sweep.baseline,sweep.optim,sweep.q4,sweep.old
need,js,sha,fields,BASE = old.need,old.js,old.sha,old.fields,old.BASE
same,finite,REFUSALS = baseline.same,baseline.finite,sweep.REFUSALS


def frozen():
    raw = (HERE/'source_contract.json').read_bytes()
    need(sha(raw)==CONTRACT,'contract_pin'); pins = js(raw)
    need(pins['source']==SOURCE,'source_pin')
    names = ('catalogue_semantic','catalogue_diagnostics','catalogue_g4','semantic_cache','catalogue_profiles',
             'full_semantic','full_campaign','full_memo')
    saved = {name:sys.modules.get(name) for name in names}
    try:
        for name in names:
            path = HERE/'source_c2'/(name+'.py')
            need(sha(path.read_bytes())==pins['scripts'][name+'.py']['sha256'],'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name] = module; spec.loader.exec_module(module)
        return pins,sys.modules['full_memo']
    finally:
        for name,value in saved.items():
            if value is None: sys.modules.pop(name,None)
            else: sys.modules[name] = value


PINS,driver = frozen()
full = driver.full
DERIVED = ('full_ms','whole_peak_reserved_bytes','cloud_ms','read_ms','pool_ms','stage_ms',
           'order_stage_ms','full_within_200ms','cloud_pool_full_ms')


def identity(row):
    need(type(row['case']) is str and all(type(row[k]) is int for k in
         ('coord_bits','kmax','workers','repetition','optimizations')),'unit_types')
    return full.identity(row)


def argv(row,case,build):
    name,bits,k,w,r,mode = identity(row)
    command = row['argv']
    need(type(command) is list and len(command)==12 and command[0]==build['path'] and
         Path(command[1]).name==case['coordinates'] and Path(command[2]).name==case['point_ids'] and
         Path(command[3]).name=='%s_b%d_k%d_w%d_r%d_o%d.bin' % (name,bits,k,w,r,mode) and
         command[4:]==[str(k),'16','256','0',str(2**32-1),str(8*1024**3),str(w),str(mode)],'native_argv')
    need(type(row['timeout_seconds']) is int and row['timeout_seconds']==60 and row['whole_input'] is True and
         type(row['count']) is int and row['count']==case['count'],'native_input')


def events(row,complete,last):
    need(type(row['stdout']) is str and type(row['stderr']) is str and
         (row['exit_code'] is None or type(row['exit_code']) is int),'streams_code')
    need(finite(row['process_wall_seconds']),'process_time')
    errors = row['errors']; allowed = {'launch','process','events','FULL_semantic','cleanup'}
    need(type(errors) is list and all(type(e) is dict and set(e)=={'stage','type','message'} and
         all(type(v) is str for v in e.values()) and e['stage'] in allowed for e in errors),'errors')
    parsed,invalid = [],0
    for line in row['stdout'].splitlines():
        if not line.strip(): continue
        try:
            value = js(line); need(type(value) is dict,'event_object'); parsed.append(value)
        except REFUSALS+(ValueError,TypeError): invalid += 1
    same(parsed,row['events'],'stdout_events')
    stages = [e['stage'] for e in errors]
    need(stages.count('events')==invalid,'invalid_event_count')
    state,code = row['status'],row['exit_code']
    if state in ('ok','pending_semantic'):
        need(code==0 and not errors,'success_code_errors')
        if state=='pending_semantic':
            need(not complete and last and not ({'semantic','semantic_reuse'}|set(DERIVED)) & row.keys(),
                 'pending_checkpoint')
    elif state=='timeout': need(code is None and 'process' in stages,'timeout')
    elif state=='launch_error': need(code is None and 'launch' in stages,'launch')
    elif state=='refused': need(code==2,'refusal')
    elif state=='failed': need(type(code) is int and code not in (0,2),'failed')
    elif state=='invalid_output': need(code==0 and bool(set(stages)&{'events','FULL_semantic'}),'invalid_output')
    elif state=='artifact_error': need(code==0 and 'cleanup' in stages,'artifact_error')
    else: need(False,'unknown_attempt_status')
    if state not in ('ok','artifact_error'):
        need('semantic' not in row and not set(DERIVED)&row.keys(),'early_output')
    if 'semantic_wall_seconds' in row: need(finite(row['semantic_wall_seconds']),'semantic_time')


def reuse_evidence(row,case,prior):
    evidence = row['semantic_reuse']; value = row.get('semantic')
    need(set(evidence)=={'schema','mode','context','current_attempt','source_attempt','raw_sha256','bytes',
                         'hash_wall_seconds','decode_wall_seconds'},'reuse_fields')
    context = full.reuse.Context('MHGP11FUL1',full.semantic.SCHEMA,
        full.reuse.decoder_digest([Path(full.semantic.__file__),Path(full.profiles.semantic.__file__)]),
        row['coord_bits'],row['kmax'],case['count'],case['sha256'],case['ids_sha256'])
    same(evidence['context'],asdict(context),'reuse_context')
    same(evidence['current_attempt'],list(identity(row)),'reuse_current')
    need(evidence['schema']==full.reuse.SCHEMA and old.HEX.fullmatch(evidence['raw_sha256']) and
         type(evidence['bytes']) is int and evidence['bytes']>0 and
         finite(evidence['hash_wall_seconds']) and finite(evidence['decode_wall_seconds']) and
         evidence['hash_wall_seconds']+evidence['decode_wall_seconds']<=row['semantic_wall_seconds'],'reuse_times')
    if value is not None:
        need(value['raw_sha256']==evidence['raw_sha256'] and value['bytes']==evidence['bytes'],'reuse_bytes')
    if evidence['mode']=='decoded':
        same(evidence['source_attempt'],list(identity(row)),'decoded_origin')
    else:
        need(evidence['mode']=='reused' and evidence['decode_wall_seconds']==0,'reuse_mode')
        key = tuple(evidence['source_attempt'])
        need(key in prior and prior[key]['status']=='ok','reuse_prior_success')
        source = prior[key]
        same(evidence['source_attempt'],list(identity(source)),'reuse_source_identity')
        need(source['semantic_reuse']['mode']=='decoded','reuse_original_decode')
        for name in ('context','raw_sha256','bytes'):
            same(source['semantic_reuse'][name],evidence[name],'reuse_origin_'+name)
        if value is not None: same(source['semantic'],value,'reuse_summary_copy')


def attempt(row,case,build,complete,last,data,prior):
    argv(row,case,build); events(row,complete,last)
    if 'semantic_reuse' in row: reuse_evidence(row,case,prior)
    if row['status'] not in ('ok','artifact_error'): return 0
    if 'semantic' not in row:
        need(row['status']=='artifact_error','success_without_semantic'); return 0
    baseline.semantic_record(row['semantic'],case,row['coord_bits'],row['kmax'])
    need(row['semantic']['orders'][0]['births']==case['count'],'K1_birth_inventory')
    replay = copy.deepcopy(row); replay.update(status='exited',errors=[])
    inspect = full.semantic.inspect
    try:
        full.semantic.inspect = lambda *_:copy.deepcopy(row['semantic'])
        full.collect(replay,case,None,row['coord_bits'])
    finally: full.semantic.inspect = inspect
    need(replay['status']=='ok' and not replay['errors'],'native_diagnostics')
    for key in DERIVED: same(row[key],replay[key],'derived_'+key)
    need(row['full_ms']/1000<=row['process_wall_seconds'],'native_process_wall')
    event = row['events'][2]
    need(event['memo_slot_bytes']=={18:176,21:192,24:208}[row['coord_bits']],'memo_slot_ABI')
    retained = sum(64*o['births']-32 if o['k']==1 else 72*o['births']-36 for o in event['orders'])
    need(retained<=event['reserved_after_bytes'],'retained_forest_lower_bound')
    matches = [raw for path,raw in data.items() if path.startswith('results/cmd/002_memo/') and
               Path(path).name==Path(row['argv'][3]).name]
    need(len(matches)<=1,'payload_duplicate')
    if not matches: return 0
    raw = matches[0]
    need(sha(raw)==row['semantic']['raw_sha256'] and len(raw)==row['semantic']['bytes'],'payload_bytes')
    same(full.semantic.decode(raw,row['coord_bits'],row['kmax'],case['count']),row['semantic'],'payload_decoded')
    return 1


def schedule(value):
    requested = driver.schedule(); same(value['requested'],requested,'schedule_exact')
    for key,wanted in dict(requested_runs=18,timeout_seconds=60,budget_seconds=550,leaf_size=16,
                           max_leaf=256,memo_capacity=65536,semantic_reuse_enabled=True).items():
        same(value[key],wanted,'parameter_'+key)
    same(value['invariant_work'],sorted(driver.INVARIANT_WORK),'invariant_work')
    same(value['variable_work'],sorted(driver.VARIABLE_WORK),'variable_work')
    same(value['optimization_modes'],{'3':'cache_J2_indirect_sort','7':'cache_J2_indirect_sort_tuple_memo'},'modes')
    need(all(type(value[k]) is bool for k in ('complete','conforming','full_schedule_completed')),'report_bools')
    rows,skips,intents = value['runs'],value['not_run'],value['launch_intents']
    keys = list(map(identity,rows)); missing = list(map(identity,skips)); wanted = list(map(identity,requested))
    need(len(keys)<=18 and keys==wanted[:len(keys)],'attempt_prefix')
    need(len(keys)+len(missing)<=18 and missing==wanted[len(keys):len(keys)+len(missing)] and
         all(r['reason']=='campaign_budget_before_launch' for r in skips),'omission_prefix')
    if value['complete']: need(len(keys)+len(missing)==18,'complete_inventory')
    need(len(intents)==len(rows) or (not value['complete'] and not skips and len(intents)==len(rows)+1),
         'intent_inventory')
    need(list(map(identity,intents))==wanted[:len(intents)],'intent_order')
    for i,row in enumerate(rows):
        need(row['argv']==intents[i]['argv'],'intent_argv')
        if row['status']=='pending_semantic':
            need(not value['complete'] and not skips and i==len(rows)-1,'pending_last')
    comparisons = driver.comparisons(rows,requested)
    same(value['comparisons'],comparisons,'comparisons_recomputed')
    done = not skips and len(rows)==18
    good = done and all(r['status']=='ok' for r in rows) and all(c['status']=='equal' for c in comparisons)
    need(value['full_schedule_completed'] is (done if value['complete'] else False) and
         value['conforming'] is (good if value['complete'] else False),'report_verdict')
    if value['complete']:
        need(finite(value['campaign_wall_seconds']),'campaign_time')
        paid = sum(r['process_wall_seconds']+r.get('semantic_wall_seconds',0) for r in rows)
        need(paid<=value['campaign_wall_seconds'],'paid_time')
        if skips: need(value['campaign_wall_seconds']>=470,'budget_omission_elapsed')
    return dict(attempts=len(rows),successes=sum(r['status']=='ok' for r in rows),omissions=len(skips),
                unpersisted=18-len(rows)-len(skips),conforming=value['conforming'],
                different=sum(c['status']=='different' for c in comparisons))


def benchmark(value,manifest,mhash,builds,data):
    need(value['schema']==driver.SCHEMA and value['work_schema']=='ehgp.v11.full_work.v4' and
         value['manifest_sha256']==mhash and value['qualification_sha256']==sha(data[BASE+'summary.json']) and
         value['supplement_sha256']==sha(data[SUPP+'summary.json']),'report_identity')
    same(value['manifest'],manifest,'manifest_copy')
    verdict = schedule(value); by_bits = {}; names = {18:'gcc_release',21:'bits21',24:'bits24'}
    same([b['coord_bits'] for b in value['builds']],list(names),'build_inventory')
    for build in value['builds']:
        bits = build['coord_bits']; baseline.build_record(build,names[bits],'mhgp11_full_bench',bits,builds,data)
        need(Path(build['path']).parts[-3:]==(names[bits],'build','mhgp11_full_bench'),'build_path')
        by_bits[bits] = build
    cases = {c['name']:c for c in manifest['cases']}; archived = 0; prior = {}
    for i,row in enumerate(value['runs']):
        archived += attempt(row,cases[row['case']],by_bits[row['coord_bits']],value['complete'],
                            i==len(value['runs'])-1,data,prior)
        if row['status']=='ok': need('semantic_reuse' in row,'successful_reuse_evidence')
        prior[identity(row)] = row
    for intent in value['launch_intents']:
        case = cases[intent['case']]; argv(intent,case,by_bits[intent['coord_bits']])
        need(intent['input_sha256']==case['sha256'] and intent['ids_sha256']==case['ids_sha256'],'intent_input_hashes')
    # Additional analysis guard: same mode has the same exact work across B21/B24.
    for name in cases:
        for mode in (3,7):
            rows = [r for r in value['runs'] if r['case']==name and r['optimizations']==mode and r['status']=='ok']
            need(len({json.dumps([o['work'] for o in r['events'][2]['orders']],sort_keys=True) for r in rows})<=1,
                 'same_mode_profile_work')
    return dict(verdict,payloads_rehashed=archived,recorded_payloads=verdict['successes']-archived,
                semantic_reused=sum(r.get('semantic_reuse',{}).get('mode')=='reused' for r in value['runs']))


def required_gates(data):
    required = {'mhgp11_tower_memo_'+s for s in ('dates','collisions_refusals','differential','capacity')}
    required |= {'mhgp11_tower_memo_full_'+s for s in ('equivalence','refusals','concurrency')}
    required |= {'mhgp11_tower_memo_fault_starvation','mhgp11_tower_memo_fault_inventaire',
                 'mhgp11_tower_diameter_canonical_pairs','mhgp11_tower_diameter_fallback'}
    required |= {'mhgp11_tower_'+n+s for n in ('forest_memo_fraction','forest_fraction','full_campaign',
                 'full_memo_collector','full_reuse_collector','full_bench_io') for s in ('','_opt')}
    for prefix in (BASE,SUPP):
        for c in js(data[prefix+'summary.json'])['configurations']:
            if c['status']=='ok' and c['name'] not in ('style','mutants'):
                need(required<={r['name'] for r in js(data[prefix+c['name']+'/tests.json'])},'required_memo_gates')


def mutants(data):
    prefix = BASE+'mutants/'
    cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    total = 0
    for module,entries in PINS['mutants'].items():
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutant_gate')
        lines = optim.index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        n = len(entries); construction = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,construction,n)) in lines,'mutant_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind=='construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutant_cause')
        total += n
    return total


def matrix_parameters(data):
    for prefix,spec,budget,threads in ((BASE,PINS['matrix'],700,48),(SUPP,PINS['supplement'],160,12)):
        value = js(data[prefix+'summary.json'])
        same(value['budget_seconds'],budget,'matrix_budget')
        same(value['thread_budget'],threads,'matrix_thread_budget')
        expected = {c['name']:c for c in spec['configurations']}
        need(len(value['configurations'])==len(expected),'matrix_config_count')
        need({c['name'] for c in value['configurations']}==set(expected),'matrix_config_names')
        for observed in value['configurations']:
            source = expected[observed['name']]
            for key in ('threads','compiler','cmake_options','ctest_args'):
                same(observed[key],source[key],'matrix_'+key)
            need(observed['optional'] is source.get('optional',False),'matrix_optional')
            for step in observed['steps']:
                if step['name']=='configure':
                    options = [v.replace('{threads}',str(observed['threads'])) for v in source['cmake_options']]
                    need(step['argv'][-len(options):]==options,'configure_options')


def analysis(value):
    """Derived diagnostics only; successful pairs are required before any ratio is formed."""
    rows = [row for row in value['runs'] if row['status']=='ok']; cases = []
    for row in rows:
        event = row['events'][2]
        orders = []
        for order in event['orders']:
            work = order['work']
            orders.append(dict(order=order['k'],**order['timings'],queries=work['memo_queries'],
                origin_hits=work['memo_hits']-work['memo_suffix_hits'],suffix_hits=work['memo_suffix_hits'],
                resolver_calls=work['traces']+work['vertical_descents'],memo_lookups=work['memo_lookups'],
                memo_hits=work['memo_hits'],memo_misses=work['memo_misses'],memo_insertions=work['memo_insertions'],
                memo_collisions=work['memo_collisions'],memo_evictions=work['memo_evictions'],
                steps=work['descent_steps'],part_meb_presentations=work['part_meb_presentations'],
                part_diameter_pairs=work['part_diameter_pairs'],trace_meb_calls=work['trace_meb_calls'],
                trace_meb_presentations=work['trace_meb_presentations'],census_point_tests=work['census_point_tests'],
                retained_forest_bytes=(64*order['births']-32 if order['k']==1 else 72*order['births']-36)))
        cases.append(dict(case=row['case'],coord_bits=row['coord_bits'],mode=row['optimizations'],
            full_ms=row['full_ms'],**row['stage_ms'],cpu_seconds=event['cpu_seconds'],
            process_wall_seconds=row['process_wall_seconds'],semantic_wall_seconds=row['semantic_wall_seconds'],
            semantic_mode=row['semantic_reuse']['mode'],peak_reserved_bytes=event['peak_reserved_bytes'],
            reserved_after_bytes=event['reserved_after_bytes'],memo_reserved_bytes=event['memo_reserved_bytes'],orders=orders))
    pairs = []
    for name,bits in sorted({(row['case'],row['coord_bits']) for row in rows}):
        by_mode = {row['mode']:row for row in cases if row['case']==name and row['coord_bits']==bits}
        if set(by_mode)!={3,7}: continue
        a,b = by_mode[3],by_mode[7]
        need(a['reserved_after_bytes']==b['reserved_after_bytes'],'memo_table_released_after_return')
        pairs.append(dict(case=name,coord_bits=bits,full_ms_3=a['full_ms'],full_ms_7=b['full_ms'],
            full_ratio_3_over_7=a['full_ms']/b['full_ms'] if b['full_ms'] else None,
            forest_ms_3=a['forest'],forest_ms_7=b['forest'],peak_delta_bytes=b['peak_reserved_bytes']-a['peak_reserved_bytes']))
    return dict(attempts=cases,complete_pairs=pairs,per_order_peak='not_measured',
                retained_scope='ForestNode24 BirthEntry8 NodeIdx4, forest buffers only; not whole retained memory',
                native_memory_scope='Buffer reservations; excludes stacks, RSS and Python')


def compare_sweep(value,previous):
    same(value['manifest'],previous['manifest'],'sweep_inputs')
    need(value['manifest_sha256']==previous['manifest_sha256'],'sweep_input_hash')
    before = {(row['case'],row['coord_bits']):row for row in previous['runs']
              if row['status']=='ok' and row['kmax']==5 and row['repetition']==0}
    need(len(before)==6,'sweep_six_baselines'); matched = []
    for row in value['runs']:
        key = row['case'],row['coord_bits']
        if row['status']!='ok' or key not in before: continue
        old_row = before[key]
        same(row['semantic'],old_row['semantic'],'sweep_recorded_output_identity')
        need(row['events'][1]['catalogue_balls']==old_row['events'][1]['catalogue_balls'],'sweep_catalogue')
        for now,old_order in zip(row['events'][2]['orders'],old_row['events'][2]['orders']):
            stable = set(now['work']) & set(old_order['work'])
            stable = {key for key in stable if not key.endswith('_meb_presentations')}
            if row['optimizations']==7: stable -= driver.VARIABLE_WORK
            same({key:now['work'][key] for key in stable},{key:old_order['work'][key] for key in stable},
                 'sweep_conserved_work')
        matched.append(list(identity(row)))
    return matched


def metrics(value,previous,previous_hash):
    result = analysis(value)
    result.update(source=SOURCE,scope='Successful recorded attempts, single fresh process per mode; no new native execution',
                  baseline_source=sweep.SOURCE,baseline_full_sha256=previous_hash,diameter_vs_sweep=[])
    old_rows = {(r['case'],r['coord_bits']):r for r in previous['runs'] if r['repetition']==0}
    for row in value['runs']:
        key = row['case'],row['coord_bits']
        if row['status']!='ok' or row['optimizations']!=3 or key not in old_rows: continue
        old_row = old_rows[key]; orders = []
        for before,now in zip(old_row['events'][2]['orders'],row['events'][2]['orders']):
            a,b = before['work'],now['work']; x,y = a['part_meb_presentations'],b['part_meb_presentations']
            orders.append(dict(order=before['k'],old_part_presentations=x,new_part_presentations=y,
                ratio_old_over_new=x/y if y else None,diameter_pairs_paid=b['part_diameter_pairs'],
                old_steps=a['descent_steps'],new_steps=b['descent_steps'],
                old_plateaus_ns=before['timings']['plateaus_ns'],new_plateaus_ns=now['timings']['plateaus_ns'],
                old_verticals_ns=before['timings']['verticals_ns'],new_verticals_ns=now['timings']['verticals_ns']))
        x = sum(o['old_part_presentations'] for o in orders); y = sum(o['new_part_presentations'] for o in orders)
        result['diameter_vs_sweep'].append(dict(case=row['case'],coord_bits=row['coord_bits'],orders=orders,
            old_part_presentations=x,new_part_presentations=y,ratio_old_over_new=x/y if y else None,
            diameter_pairs_paid=sum(o['diameter_pairs_paid'] for o in orders),
            old_forest_ms=old_row['stage_ms']['forest'],new_forest_ms=row['stage_ms']['forest']))
    return result


def transport(folder):
    receipt,worker,data = old.read_capture(folder)
    raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    extra = {'raw_receipt_local','original_receipt_sha256','preserved_failure'}
    need(set(receipt)-extra<=set(raw),'raw_required_fields')
    need(receipt['commit']==SOURCE and receipt['plan_sha256']==PINS['plan_sha256'],'capture_source')
    need(receipt['results_skipped_members']==[] and receipt['results_bytes']<=16*1024**2,'complete_bounded_archive')
    need(all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')) and
         not receipt['warnings'],'cleanup')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()==
         ('0' if receipt['status']=='completed' else '3'),'controller_done')
    inventory = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,path = line.split('  ',1); path = 'results/'+path.removeprefix('./')
        need(path not in inventory and path in data and sha(data[path])==digest,'archive_manifest'); inventory.add(path)
    need(inventory==data.keys()-{'results/MANIFEST.sha256'},'archive_inventory')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'commands_inventory')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),('full_memo.json',BENCH)]:
        need((folder/name).read_bytes()==data[path] if path in data else not (folder/name).exists(),'compact_copy')
    metas = [fields(data['results/cmd/'+name+'/meta.txt']) for name in COMMANDS]
    for meta,timeout in zip(metas,(750,180,630)):
        need(meta['requested_timeout_seconds']==str(timeout) and meta.get('group_closed')=='1' and
             meta.get('streams_truncated')=='0' and meta.get('residual_group_killed')=='0','command_closure')
    return receipt,worker,data,metas


def check(folder=HERE):
    receipt,worker,data,metas = transport(folder)
    matrix_parameters(data)
    counts,builds,code = q4.judge_matrix(data)
    for name,proof in builds.items():
        if 'CMakeCache.txt' in proof:
            optim.cache(proof,dict(MHGP11_SANITIZE='ON' if name=='gcc_asan_ubsan' else 'OFF',
                                  MHGP11_TSAN='ON' if name=='gcc_tsan' else 'OFF',MHGP11_POISON='ON' if name=='poison' else 'OFF'))
    if 'CMakeCache.txt' in builds['mutants']: optim.cache(builds['mutants'],{'MHGP11_MUTANT_JOBS':'32'})
    extra,suppcode = optim.supplement(data)
    if suppcode==0: baseline.supplement_provenance(data)
    required_gates(data)
    killed = mutants(data) if js(data[BASE+'mutants/result.json'])['status']=='ok' else None
    q4.command(metas[0],code); q4.command(metas[1],suppcode)
    manifest,mhash = old.inputs(folder,receipt)
    verdict = dict(conforming=False,attempts=0,successes=0,omissions=0,unpersisted=18,different=0,
                   payloads_rehashed=0,recorded_payloads=0,semantic_reused=0)
    if BENCH in data:
        need(code==suppcode==0,'benchmark_requires_qualification')
        value = js(data[BENCH]); verdict = benchmark(value,manifest,mhash,builds,data)
        q4.command(metas[2],0 if value['conforming'] else 1 if value['complete'] else None)
        sweep.check()
        previous = js((sweep.HERE/'sweep2/full.json').read_bytes())
        verdict['sweep_recorded_outputs_matched'] = len(compare_sweep(value,previous))
        if (folder/'metrics.json').exists():
            previous_hash = sha((sweep.HERE/'sweep2/full.json').read_bytes())
            same(js((folder/'metrics.json').read_bytes()),metrics(value,previous,previous_hash),'derived_metrics')
    else: q4.command(metas[2])
    good = q4.session(receipt,worker,metas)
    need(good is verdict['conforming'] and receipt['preserved_failure'] is (not good),'session_verdict')
    construction = sum(kind=='construction' for entries in PINS['mutants'].values() for kind in entries.values())
    return dict(coherent=True,source=SOURCE,matrix_passed=sum(v[1] for v in counts.values()),
                matrix_total=sum(v[0] for v in counts.values()),asan18_passed=extra[1],asan18_total=extra[0],
                causal_mutants=killed,mutants_code_or_line=killed-construction if killed is not None else None,
                mutants_expected_construction=construction if killed is not None else None,**verdict)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,
                     old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
