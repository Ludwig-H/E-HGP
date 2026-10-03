"""LIVE reuse1 success reader: archived exact campaign, no native execution or WIP import.

Code0 means the closed success evidence is coherent. Raw receipt, source and results
remain mandatory. Deleted FULL payloads contribute recorded hashes, never a claimed rehash.
"""
import copy
from dataclasses import asdict
import math
import importlib.util
import json
from pathlib import Path
import re
import shlex
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = 'ae817d09ec66e8024c5947f178cf2b1b3ab766e7'
CONTRACT = '417325830ccd650ddf8c516b14433ce96d11f9c131b96b97b76445963df2661c'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


# Frozen copies are checked BEFORE import; no dependency on omitted historical checkout files.
import hashlib
_contract_blob = (HERE/'source_contract.json').read_bytes()
if hashlib.sha256(_contract_blob).hexdigest()!=CONTRACT:
    raise ValueError('contract_pin')
_import_pins = json.loads(_contract_blob)
for _path in _import_pins['helpers']:
    _blob = (HERE/'reader_sources'/_path).read_bytes(); _pin = _import_pins['files'][_path]
    if len(_blob)!=_pin['bytes'] or hashlib.sha256(_blob).hexdigest()!=_pin['sha256']:
        raise ValueError('historical_reader_pin')


first = load('forest_failure_transport',HERE/'reader_sources/receipts/catalogue_adaptive_20261002/adaptive1_failure/check.py')
index = load('forest_failure_logs',HERE/'reader_sources/receipts/index_20261002/check.py')
q4,old = first.q4,first.old
need,js,sha,same,BASE,SUPP = first.need,first.js,first.sha,first.same,first.BASE,first.SUPP
REFUSALS = (old.foundation.Refusal,index.old.foundation.Refusal)
COMMANDS = ('000_matrice','001_asan18','002_reuse_verticals_full')
PLAN = 'bench/plans/full_vertical_reuse_g4.json'
BENCH = 'results/cmd/002_reuse_verticals_full/files/full_parallel.json'
fields = old.fields


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob)==CONTRACT,'contract_pin')
    value = js(blob); need(value['source']==SOURCE,'source_pin')
    package_pin = value['source_package']; package = Path(package_pin['raw_path'])
    need(package.stat().st_size==package_pin['bytes'] and old.digest(package)==package_pin['sha256'],
         'live_source_pin')
    with tarfile.open(package) as archive:
        for path,pin in value['files'].items():
            member = archive.getmember('morsehgp3D_v11/'+path)
            need(member.isfile(),'source_member_type')
            raw = archive.extractfile(member).read()
            need(sha(raw)==pin['sha256'] and len(raw)==pin['bytes'],'package_source_pin')
            for key,file in [('plan',PLAN),('matrix','tools/g4_matrix.json'),('supplement','bench/meb_asan18_matrix.json')]:
                if path==file: same(js(raw),value[key],'source_contract_copy')
            if path.startswith('tests/mutants/') and path.endswith('.json'):
                same({m['id']:m.get('attendu','execution') for m in js(raw)['mutants']},
                     value['mutants'][Path(path).stem],'mutant_manifest_copy')
    return value

def frozen():
    pins = source_contract(); names = pins['scripts']; saved = {n:sys.modules.get(n) for n in names}
    try:
        for name in names:
            path = HERE/'source_ae'/(name+'.py'); blob = path.read_bytes(); pin = pins['files']['bench/'+name+'.py']
            need(len(blob)==pin['bytes'] and sha(blob)==pin['sha256'],'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name]=module; spec.loader.exec_module(module)
        return pins,sys.modules['full_parallel']
    finally:
        for name,value in saved.items():
            if value is None: sys.modules.pop(name,None)
            else: sys.modules[name]=value


PINS,driver = frozen()
full = driver.full
DERIVED = ('full_ms','whole_peak_reserved_bytes','cloud_ms','read_ms','pool_ms','stage_ms',
           'order_stage_ms','full_within_200ms','cloud_pool_full_ms','catalogue_stage_ms','catalogue_execution')


def finite(value): return type(value) in (int,float) and math.isfinite(value) and value>=0


def build_record(record, name, executable, bits, builds, data):
    files = builds[name]
    need(record['configuration'] == name and type(record['coord_bits']) is int and record['coord_bits'] == bits and
         Path(record['path']).name == executable, 'build_identity')
    for key,value in [('sha256',files[executable]['sha256']),('bytes',files[executable]['size']),
                      ('cache_sha256',files['CMakeCache.txt']['sha256']),
                      ('provenance_sha256',old.sha(data[BASE+name+'/build_provenance.json']))]:
        same(record[key],value,'build_'+key)

def semantic_record(value, case, bits, kmax):
    need(value['schema'] == 'ehgp.v11.full_semantic.v1' and old.HEX.fullmatch(value['sha256']) and
         old.HEX.fullmatch(value['raw_sha256']), 'recorded_hashes')
    full.unsigned(value,('births','bytes','coord_bits','edges','kmax','merges','nodes','points','sites','verticals'))
    need(value['coord_bits'] == bits and value['kmax'] == kmax and
         value['sites'] == value['points'] == case['count'] and len(value['orders']) == kmax,'semantic_identity')
    for k,order in enumerate(value['orders'],1):
        full.unsigned(order,('order','births','edges','merges','nodes','root','verticals'))
        need(order['order'] == k and order['births'] > 0 and
             order['nodes'] == order['births']+order['merges'] == order['edges']+1 and
             order['root'] < order['nodes'] and order['verticals'] == (order['nodes'] if k > 1 else 0),
             'semantic_order_counts')
    for key in ('births','edges','merges','nodes','verticals'):
        need(sum(o[key] for o in value['orders']) == value[key], 'semantic_total_'+key)
    minimum = 42+40*case['count']+40*kmax+72*value['nodes']+96*value['births']+8*(value['verticals']+value['edges'])
    need(value['bytes'] >= minimum, 'semantic_minimum_bytes')

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
    semantic_record(row['semantic'],case,row['coord_bits'],row['kmax'])
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
    retained = sum((56*o['births']-32 if o['k']==1 else 64*o['births']-36)+o['lookup_reserved_bytes']
                   for o in event['orders'])
    need(retained<=event['reserved_after_bytes'],'retained_forest_lower_bound')
    return archived_payload(row,case,data)


def archived_payload(row,case,data):
    matches = [raw for path,raw in data.items() if path.startswith('results/cmd/002_reuse_verticals_full/') and
               Path(path).name==Path(row['argv'][3]).name]
    need(len(matches)<=1,'payload_duplicate')
    if not matches: return 0
    raw = matches[0]
    need(sha(raw)==row['semantic']['raw_sha256'] and len(raw)==row['semantic']['bytes'],'payload_bytes')
    same(full.semantic.decode(raw,row['coord_bits'],row['kmax'],case['count']),row['semantic'],'payload_decoded')
    return 1

def schedule(value):
    requested = driver.schedule(reuse_verticals=True); same(value['requested'],requested,'schedule_exact')
    for key,wanted in dict(requested_runs=29,timeout_seconds=60,budget_seconds=500,leaf_size=16,max_leaf=256,
                           memo_capacity=65536,semantic_reuse_enabled=True,regular_batch_capacity=4096,
                           descent_lanes=48,lane_memo_capacity=4096).items(): same(value[key],wanted,'parameter_'+key)
    same(value['invariant_work'],sorted(driver.INVARIANT_WORK),'invariant_work')
    same(value['variable_work'],sorted(driver.VARIABLE_WORK),'variable_work')
    same(value['optimization_modes'],{'511':'mode255_reused_census_workspace','1023':'mode511_dense_birth_lookup',
         '2047':'mode1023_reused_regular_vertical_seeds'},'modes')
    for key,expected in dict(optimized_catalogue=False,parallel_verticals=False,reuse_census=False,dense_births=False,reuse_verticals=True,
            parallel_schema='ehgp.v11.full_parallel.v1',vertical_schema=full.vertical.SCHEMA,
            census_workspace_schema=full.workspace.SCHEMA,census_comparison_schema=full.workspace.COMPARISON_SCHEMA,
            dense_lookup_schema=full.dense.SCHEMA,regular_vertical_schema=full.regular.SCHEMA,
            census_comparison_mask=1167,descent_work_mask=1423,catalogue_execution_schema='ehgp.v11.catalogue_execution.v1').items():
        same(value[key],expected,'route_'+key)
    need(all(type(value[k]) is bool for k in ('complete','conforming','full_schedule_completed')),'report_bools')
    rows,skips,intents = value['runs'],value['not_run'],value['launch_intents']
    keys = list(map(identity,rows)); missing = list(map(identity,skips)); wanted = list(map(identity,requested))
    need(len(keys)<=29 and keys==wanted[:len(keys)],'attempt_prefix')
    need(len(keys)+len(missing)<=29 and missing==wanted[len(keys):len(keys)+len(missing)] and
         all(r['reason']=='campaign_budget_before_launch' for r in skips),'omission_prefix')
    if value['complete']: need(len(keys)+len(missing)==29,'complete_inventory')
    need(len(intents)==len(rows) or (not value['complete'] and not skips and len(intents)==len(rows)+1),
         'intent_inventory')
    need(list(map(identity,intents))==wanted[:len(intents)],'intent_order')
    for i,row in enumerate(rows):
        need(row['argv']==intents[i]['argv'],'intent_argv')
        if row['status']=='pending_semantic': need(not value['complete'] and not skips and i==len(rows)-1,'pending_last')
    comparisons = driver.comparisons(rows,requested)
    same(value['comparisons'],comparisons,'comparisons_recomputed')
    done = not skips and len(rows)==29
    good = done and all(r['status']=='ok' for r in rows) and all(c['status']=='equal' for c in comparisons)
    need(value['full_schedule_completed'] is (done if value['complete'] else False) and
         value['conforming'] is (good if value['complete'] else False),'report_verdict')
    if value['complete']:
        need(finite(value['campaign_wall_seconds']),'campaign_time')
        paid = sum(r['process_wall_seconds']+r.get('semantic_wall_seconds',0) for r in rows)
        need(paid<=value['campaign_wall_seconds'],'paid_time')
        if skips: need(value['campaign_wall_seconds']>=420,'budget_omission_elapsed')
    return dict(attempts=len(rows),successes=sum(r['status']=='ok' for r in rows),omissions=len(skips),
                unpersisted=29-len(rows)-len(skips),conforming=value['conforming'],
                different=sum(c['status']=='different' for c in comparisons),
                equal=sum(c['status']=='equal' for c in comparisons))

def benchmark(value,manifest,mhash,builds,data):
    need(value['schema']==driver.SCHEMA and value['work_schema']=='ehgp.v11.full_work.v5' and
         value['manifest_sha256']==mhash and value['qualification_sha256']==sha(data[BASE+'summary.json']) and
         value['supplement_sha256']==sha(data[SUPP+'summary.json']),'report_identity')
    same(value['manifest'],manifest,'manifest_copy')
    verdict = schedule(value); by_bits = {}; names = {18:'gcc_release',21:'bits21',24:'bits24'}
    same([b['coord_bits'] for b in value['builds']],list(names),'build_inventory')
    for build in value['builds']:
        bits = build['coord_bits']; build_record(build,names[bits],'mhgp11_full_bench',bits,builds,data)
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

    return dict(verdict,payloads_rehashed=archived,recorded_payloads=verdict['successes']-archived,
                semantic_reused=sum(r.get('semantic_reuse',{}).get('mode')=='reused' for r in value['runs']))

def gate(config,data,declared):
    name = config['name']; prefix = BASE+name+'/'
    result = old.foundation.judge_config(config,data); proof = first.build(config,data,declared)
    for key in ('threads','compiler','ctest_args','cmake_options'): same(config[key],declared[key],'config_'+key)
    need(config['optional'] is declared.get('optional',False),'config_optional')
    expected = 'absent' if name=='clang_release' else 'ok'
    need(config['status']==expected,'configuration_status')
    if name in ('style','clang_release'): return result
    need(config['not_run']==[],'native_all_executed')
    provenance = js(data[prefix+'build_provenance.json'])
    need(provenance['complete'] is True and provenance['errors']==[] and
         {'libmhgp11.a','mhgp11_full_bench','mhgp11_tower_forest_parallel','mhgp11_tower_forest_parallel_fault',
          'mhgp11_tower_forest_parallel_probe'}<=set(proof),'compiled_targets')
    steps = old.foundation.unique((s['name'],s) for s in config['steps'])
    need(set(steps)=={'configure','build','list','test'},'step_inventory')
    for key in steps:
        failed = False
        need(steps[key]['status']==('failed' if failed else 'ok') and type(steps[key]['exit_code']) is int and
             steps[key]['exit_code']==(8 if failed else 0) and steps[key].get('timed_out',False) is False,'executed_steps')
    need('error:' not in data[prefix+'build.log'].decode(),'no_base_compiler_error')
    cases = list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    wanted = set()
    need({c.get('name') for c in cases if c.find('failure') is not None}==wanted and
         len(config['failures'])==len(wanted) and {f['test'] for f in config['failures']}==wanted,'failed_gate_inventory')
    for case in cases:
        failed = case.get('name') in wanted
        need(case.get('status')==('fail' if failed else 'run') and case.find('skipped') is None,'gate_state')
    need(result[2:]==(len(wanted),0),'failure_counts')
    return result

def matrices(data,pins):
    counts = {}
    for prefix,key,budget,threads in ((BASE,'matrix',800,48),(SUPP,'supplement',160,12)):
        summary = js(data[prefix+'summary.json']); configs = summary['configurations']
        declared = {c['name']:c for c in pins[key]['configurations']}; names = [c['name'] for c in configs]
        need(len(names)==len(set(names)) and set(names)==set(declared) and summary['requested']==names and
             summary['schema']=='ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
             summary['conforming'] is True and type(summary['exit_code']) is int and
             summary['exit_code']==0 and
             summary['signals']==[],'matrix_status')
        same(summary['budget_seconds'],budget,'matrix_budget'); same(summary['thread_budget'],threads,'matrix_threads')
        same(summary['statuses'],{c['name']:c['status'] for c in configs},'matrix_statuses')
        need({p[len(prefix):].split('/')[0] for p in data if p.startswith(prefix) and '/' in p[len(prefix):]}==set(names),
             'matrix_directories')
        mapped = {BASE+p[len(prefix):]:v for p,v in data.items() if p.startswith(prefix)}
        observed = {c['name']:gate(c,mapped,declared[c['name']]) for c in configs}
        if key=='matrix':
            need(all(observed[n][0]==pins['matrix_counts'][n] for n in names),'main_counts'); counts = observed
        else: need(observed=={'gcc_asan_ubsan18':(299,299,0,0)},'supplement_counts')
    return counts

def mutants(data):
    prefix = BASE+'mutants/'
    cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    total = 0
    for module,entries in PINS['mutants'].items():
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutant_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        verdicts = [re.fullmatch(r'(\w+)\s+(TUE|INVALIDE|SURVIVANT)\s+(.+)',s) for s in lines]
        verdicts = [v for v in verdicts if v is not None]
        need(len(verdicts)==len(entries) and {v[1] for v in verdicts}==set(entries),'exact_mutant_inventory')
        n = len(entries); construction = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,construction,n)) in lines,'mutant_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind=='construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutant_cause')
        total += n
    return total

def transport(folder,pins):
    receipt,worker,data = old.read_capture(folder); raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local','original_receipt_sha256','preserved_failure'}<=set(raw),'receipt_raw_fields')
    need(receipt['commit']==SOURCE and receipt['results_sha256']==pins['archive_sha256'] and
         receipt['plan_sha256']==pins['files'][PLAN]['sha256'] and
         receipt['worker_plan_sha256']==worker['plan_sha256']==sha(data['results/plan.sh']),'capture_identity')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()=='0' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')) and
         receipt['warnings']==[] and receipt['preserved_failure'] is False and receipt['results_skipped_members']==[],
         'closed_success')
    need(receipt['target']==dict(project='devpod-gpu-exploration',zone='us-central1-c',
         instance='ehgp-v7-3b1d496aed430749ea7e049f') and
         receipt['observed_after']['name']==receipt['target']['instance'] and
         receipt['start_certified'] is True and receipt['worker_outcome']=='exited','target_identity')
    need(raw['guest_guard_intact'] is True and raw['overflow']==dict(evicted=[],truncated_streams=[]) and
         raw['retrieval']=='downloaded','guard_and_collection')
    for command in ('guarded_stop','oslogin_remove'):
        calls = [row for row in raw['host_commands'] if row['name']==command]
        need(len(calls)==1 and type(calls[0]['exit_code']) is int and calls[0]['exit_code']==0,
             'cleanup_command')
    members = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,name = line.split('  ',1); name = 'results/'+name.removeprefix('./')
        need(name not in members and name in data and sha(data[name])==digest,'manifest_hash'); members.add(name)
    need(members==set(data)-{'results/MANIFEST.sha256'},'manifest_inventory')
    for file,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),('full_parallel.json',BENCH)]:
        need((folder/file).read_bytes()==data[path],'compact_copy')
    old.inputs(folder,receipt)
    return receipt,worker,data

def live_sources(folder,receipt):
    session = Path(receipt['raw_receipt_local']).parent
    preflight = js((folder/'preflight.json').read_bytes())
    package = session/'package/package.tar.gz'; archive = session/'results/results.tar.gz'
    need(preflight['package_sha256']==receipt['package_sha256'] and
         package.stat().st_size==preflight['package_bytes'] and old.digest(package)==receipt['package_sha256'],
         'live_source_package')
    need(archive.stat().st_size==receipt['results_bytes'] and old.digest(archive)==receipt['results_sha256'],
         'live_results_archive')

def commands(receipt,worker,data,pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'command_inventory')
    for name,code,declared in zip(COMMANDS,(0,0,0),pins['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = old.fields(data[prefix+'meta.txt']); q4.command(meta,code)
        need(meta['requested_timeout_seconds']==str(declared['timeout_seconds']) and
             meta['streams_truncated']=='0' and meta['residual_group_killed']=='0','command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode()); root = argv[1].split('/src/morsehgp3D_v11/',1)[0]
        replacements = dict(src=root+'/src',build=root+'/build',data=root+'/data',out=root+'/results/cmd/'+name+'/files')
        same(argv,[a.format(**replacements) for a in declared['argv']],'executed_plan')
    rows = js(data[BENCH])['runs']
    expected = ['%s B%d K%d W%d r%d mode%d %s' % (*identity(row),row['status']) for row in rows]
    same(data['results/cmd/002_reuse_verticals_full/stdout'].decode().splitlines(),expected,'command_report_rows')
    need(data['results/cmd/002_reuse_verticals_full/stderr']==b'','command_stderr')
    need(worker['commands_total']=='3' and worker['commands_ok']=='3' and worker['status']=='completed' and
         worker['interrupted']=='0' and receipt['status']=='completed' and receipt['worker_exit_code']==0,'session_success')


def preflight(folder):
    pin = PINS['preflight']; blob = (folder/'preflight.json').read_bytes()
    need(len(blob)==pin['bytes'] and sha(blob)==pin['sha256'] and Path(pin['raw_path']).read_bytes()==blob,'preflight_live')
    value = js(blob)
    need(value['commit']==SOURCE and value['gcp_mutations']=='none' and
         value['plan_sha256']==PINS['files'][PLAN]['sha256'] and 'warnings' not in value,'preflight_identity')
    same(value['budget'],dict(build_and_setup_seconds=120,closing_reserve_seconds=660,
         command_timeouts_sum_seconds=1600,guest_seconds=2700,oversubscribed=False,
         ssh_cutoff_before_guest_seconds=360,upload_estimate_seconds=183,worker_window_seconds=1737),'budget')
    normalized = {k:v for k,v in PINS['plan'].items() if k not in ('note','schema')}
    normalized.update(build_targets=[],build_timeout_seconds=600); same(value['plan'],normalized,'normalized_plan')


def required_gates(data):
    required = {'mhgp11_tower_census_reuse_'+s for s in ('descents','memo_identity','full','admission','inventaire',
                'fault_allocation_free','fault_starvation','fault_inventaire')}
    required |= {'mhgp11_tower_'+n+s for n in ('census_reuse_fraction','census_reuse_model','full_census_collector',
                  'forest_parallel_fraction','forest_parallel_memo_fraction','full_bench_io') for s in ('','_opt')}
    required |= {'mhgp11_num_unit_certificate_'+s for s in ('limits','public','owners')}
    required |= {'mhgp11_num_power_certificate_'+n+s for n in ('fraction','model') for s in ('','_opt')}
    required |= {'mhgp11_tower_regular_vertical_reuse_'+s for s in ('equivalence','closed_dates','extended',
                 'cache_contract','inventaire','fault_factory','fault_all_k_memory','fault_starvation',
                 'fault_inactive','fault_inventaire')}
    required |= {'mhgp11_tower_'+n+s for n in ('regular_vertical_reuse_model','full_regular_vertical_collector',
                 'full_dense_collector') for s in ('','_opt')}
    required |= {'mhgp11_num_orientation_certificate_'+n+s for n in ('fraction','model') for s in ('','_opt')}
    for prefix in (BASE,SUPP):
        for c in js(data[prefix+'summary.json'])['configurations']:
            if c['status']=='ok' and c['name'] not in ('style','mutants'):
                need(required<={r['name'] for r in js(data[prefix+c['name']+'/tests.json'])},'required_census_gates')


def analysis(value):
    rows = []
    for row in value['runs']:
        if row['status']!='ok': continue
        event = row['events'][2]
        rows.append(dict(case=row['case'],coord_bits=row['coord_bits'],mode=row['optimizations'],workers=row['workers'],
            full_ms=row['full_ms'],stage_ms=row['stage_ms'],catalogue_stage_ms=row['catalogue_stage_ms'],
            cpu_seconds=event['cpu_seconds'],process_seconds=row['process_wall_seconds'],
            cloud_ms=row['cloud_ms'],read_ms=row['read_ms'],pool_ms=row['pool_ms'],
            semantic_seconds=row['semantic_wall_seconds'],semantic_mode=row['semantic_reuse']['mode'],
            peak_reserved_bytes=event['peak_reserved_bytes'],reserved_after_bytes=event['reserved_after_bytes'],
            lookup_reserved_bytes=event['lookup_reserved_bytes'],regular_vertical_reserved_bytes=event['regular_vertical_reserved_bytes'],
            census_workspaces=event['census_workspaces'],census_workspace_reserved_bytes=event['census_workspace_reserved_bytes'],
            orders=[dict(k=o['k'],births=o['births'],timings=o['timings'],vertical_parallel=o['vertical_parallel'],
                work={key:o['work'][key] for key in ('census_point_tests','part_meb_presentations','part_diameter_pairs',
                     'descent_steps','memo_hits','memo_queries','memo_suffix_hits','vertical_descents','vertical_reuses',
                     'ancestor_queries','ancestor_find_steps')}) for o in event['orders']]))
    return dict(source=SOURCE,attempts=rows,comparisons=value['comparisons'],
        process_seconds=sum(r['process_wall_seconds'] for r in value['runs']),
        semantic_seconds=sum(r.get('semantic_wall_seconds',0) for r in value['runs']),
        full_seconds=sum(r['full_ms']/1000 for r in value['runs'] if r['status']=='ok'),
        campaign_seconds=value.get('campaign_wall_seconds'),
        scope='One fresh process per unit, CPU FULL K1..5; read/Cloud/Pool/decoding excluded from full_ms',
        memory_scope='Buffer reservations, not stack/RSS/Python; payload hashes remain recorded')


def check(folder=HERE):
    receipt,worker,data = transport(folder,PINS); preflight(folder); live_sources(folder,receipt)
    counts = matrices(data,PINS); commands(receipt,worker,data,PINS); killed = mutants(data)
    builds = {name:old.provenance(c,data) for c in js(data[BASE+'summary.json'])['configurations'] for name in [c['name']]}
    required_gates(data)
    manifest,mhash = old.inputs(folder,receipt); value = js(data[BENCH])
    verdict = benchmark(value,manifest,mhash,builds,data)
    need(verdict['conforming'] and verdict['successes']==29 and verdict['omissions']==verdict['unpersisted']==0,
         'closed_benchmark_success')
    if (folder/'metrics.json').exists(): same(js((folder/'metrics.json').read_bytes()),analysis(value),'derived_metrics')
    construction = sum(kind=='construction' for entries in PINS['mutants'].values() for kind in entries.values())
    return dict(coherent=True,source=SOURCE,matrix_passed=sum(v[1] for v in counts.values()),
                matrix_total=sum(v[0] for v in counts.values()),asan18_passed=299,asan18_total=299,
                causal_mutants=killed,mutants_code_or_line=killed-construction,mutants_expected_construction=construction,
                cross_route=full.workspace.comparisons([r for r in value['runs'] if r['status']=='ok'],full.WORK,need),**verdict)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
