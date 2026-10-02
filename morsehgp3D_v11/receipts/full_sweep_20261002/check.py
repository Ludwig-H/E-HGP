"""LIVE sweep2 evidence: code0 means coherence of its preserved calendar failure.

One original archive and the original local receipt remain mandatory. Deleted
FULL payloads contribute recorded hashes only; present payloads are decoded.
The thirteen matching FULL3 runs are separately validated by their LIVE reader.
"""
import copy
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = '12f49d0cae6488f0076ef295a9f107920d7567db'
CONTRACT = '4942d10dca515b4b536d082d165802836f227285d63337f8c7bb67cf2083b72b'
COMMANDS = ('000_matrice','001_asan18','002_full')
SUPP = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_full/files/full.json'
STABLE_WORK = ('cells','replayed_cells','plateaus','traces','unions','continuations',
               'descent_steps','vertical_descents','vertical_checks')


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


baseline = load('sweep_baseline',HERE.parent/'full_20261002/check_full.py')
optim = load('sweep_optimization_helpers',HERE.parent/'catalogue_optimizations_20261002/check.py')
q4,old = baseline.q4,baseline.old
need,js,sha,fields,BASE = old.need,old.js,old.sha,old.fields,old.BASE
same,finite = baseline.same,baseline.finite
REFUSALS = (old.foundation.Refusal,)+optim.REFUSALS


def frozen():
    raw = (HERE/'source_contract.json').read_bytes()
    need(sha(raw) == CONTRACT,'contract_pin'); pins = js(raw)
    need(pins['source'] == SOURCE,'source_pin')
    names = ('catalogue_g4','catalogue_semantic','catalogue_profiles','full_semantic','full_campaign')
    saved = {name:sys.modules.get(name) for name in names}
    try:
        for name in names:
            path = HERE/'source_12f49'/(name+'.py')
            need(sha(path.read_bytes()) == pins['scripts'][name+'.py']['sha256'],'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name] = module; spec.loader.exec_module(module)
        return pins,sys.modules['full_campaign']
    finally:
        for name,value in saved.items():
            if value is None: sys.modules.pop(name,None)
            else: sys.modules[name] = value


PINS,driver = frozen()


def attempt(row,intent,request,case,build,data):
    same({k:row[k] for k in request},request,'attempt_identity')
    same({k:intent[k] for k in request},request,'intent_identity')
    need(row['status'] == 'ok' and type(row['exit_code']) is int and row['exit_code'] == 0 and
         row['errors'] == [] and row['stderr'] == '' and row['whole_input'] is True and
         type(row['count']) is int and row['count'] == case['count'] and
         type(row['timeout_seconds']) is int and row['timeout_seconds'] == 60,'attempt_success')
    bits,k,r = request['coord_bits'],request['kmax'],request['repetition']; argv = row['argv']
    need(len(argv) == 12 and argv[0] == build['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == '%s_b%d_k%d_w48_r%d_o3.bin' % (case['name'],bits,k,r) and
         argv[4:] == [str(k),'16','256','0',str(2**32-1),str(8*1024**3),'48','3'],'attempt_argv')
    need(intent['argv'] == argv and intent['whole_input'] is True and
         type(intent['count']) is int and intent['count'] == case['count'] and
         intent['timeout_seconds'] == 60 and intent['input_sha256'] == case['sha256'] and
         intent['ids_sha256'] == case['ids_sha256'],'intent_payload')
    need(finite(row['process_wall_seconds']) and finite(row['semantic_wall_seconds']),'attempt_times')
    same([js(line) for line in row['stdout'].splitlines() if line.strip()],row['events'],'attempt_events')
    baseline.semantic_record(row['semantic'],case,bits,k)
    replay = copy.deepcopy(row); inspect = driver.semantic.inspect
    try:
        # Rejudge recorded diagnostics, without claiming to decode a deleted file.
        driver.semantic.inspect = lambda *_:copy.deepcopy(row['semantic'])
        driver.collect(replay,case,None,bits)
    finally:
        driver.semantic.inspect = inspect
    need(replay['status'] == 'ok' and replay['errors'] == [],'native_diagnostics')
    for key in ('full_ms','whole_peak_reserved_bytes','cloud_ms','read_ms','pool_ms','stage_ms',
                'order_stage_ms','full_within_200ms','cloud_pool_full_ms'):
        same(row[key],replay[key],'derived_'+key)
    need(row['full_ms']/1000 <= row['process_wall_seconds'],'native_process_wall')
    matches = [raw for path,raw in data.items() if path.startswith('results/cmd/002_full/') and
               Path(path).name == Path(argv[3]).name]
    need(len(matches) <= 1,'payload_duplicate')
    if not matches: return 0
    raw = matches[0]
    need(sha(raw) == row['semantic']['raw_sha256'] and len(raw) == row['semantic']['bytes'],'payload_bytes')
    same(driver.semantic.decode(raw,bits,k,case['count']),row['semantic'],'payload_semantic')
    return 1


def report(value,manifest,mhash,builds,data):
    need(value['schema'] == driver.SCHEMA and value['work_schema'] == 'ehgp.v11.full_work.v2' and
         value['complete'] is True and value['full_schedule_completed'] is False and
         value['conforming'] is False,'preserved_calendar_failure')
    need(value['manifest_sha256'] == mhash and value['qualification_sha256'] == sha(data[BASE+'summary.json']) and
         value['supplement_sha256'] == sha(data[SUPP+'summary.json']),'report_links')
    same(value['manifest'],manifest,'report_manifest')
    requested = driver.schedule(3); same(value['requested'],requested,'schedule_exact')
    for key,wanted in dict(requested_runs=24,budget_seconds=600,timeout_seconds=60,leaf_size=16,
                           max_leaf=256,optimizations=3).items(): same(value[key],wanted,'parameter_'+key)
    need(len(value['runs']) == len(value['launch_intents']) == 13 and len(value['not_run']) == 11,'inventory')
    same(value['not_run'],[dict(r,reason='campaign_budget_before_launch') for r in requested[13:]],'omissions_exact')
    names = {18:'gcc_release',21:'bits21',24:'bits24'}
    same([b['coord_bits'] for b in value['builds']],list(names),'build_inventory'); by_bits = {}
    for build in value['builds']:
        bits = build['coord_bits']
        baseline.build_record(build,names[bits],'mhgp11_full_bench',bits,builds,data); by_bits[bits] = build
    cases = {c['name']:c for c in manifest['cases']}; archived = 0
    for row,intent,request in zip(value['runs'],value['launch_intents'],requested[:13]):
        archived += attempt(row,intent,request,cases[request['case']],by_bits[request['coord_bits']],data)
    same(value['comparisons'],driver.comparisons(value['runs'],requested),'comparisons_recomputed')
    elapsed = value['campaign_wall_seconds']
    paid = sum(r['process_wall_seconds']+r['semantic_wall_seconds'] for r in value['runs'])
    need(finite(elapsed) and 520 <= paid <= elapsed < 600,'budget_omission_elapsed')
    return dict(attempts=13,successes=13,omissions=11,conforming=False,
                different=sum(c['status']=='different' for c in value['comparisons']),
                payloads_rehashed=archived,recorded_payloads=13-archived,campaign_wall_seconds=elapsed)


def compare_baseline(value,previous):
    same(value['manifest'],previous['manifest'],'baseline_inputs')
    need(value['manifest_sha256'] == previous['manifest_sha256'],'baseline_input_hash')
    unit = lambda r:(r['case'],r['coord_bits'],r['kmax'],r['workers'],r['repetition'])
    old_rows = {unit(r):r for r in previous['runs'] if r['status']=='ok'}
    need(len(old_rows) == len(previous['runs']) == 13 and len(value['runs']) == 13,'baseline_inventory')
    need({unit(r) for r in value['runs']} == set(old_rows),'baseline_matching_units')
    for row in value['runs']:
        before = old_rows[unit(row)]
        # Full record includes RAW same-profile hash, semantic hash, byte size and all topology counts.
        same(row['semantic'],before['semantic'],'baseline_output_identity')
        need(row['events'][1]['catalogue_balls'] == before['events'][1]['catalogue_balls'],'baseline_catalogue')
        for now,old_order in zip(row['events'][2]['orders'],before['events'][2]['orders']):
            same({k:now['work'][k] for k in STABLE_WORK},{k:old_order['work'][k] for k in STABLE_WORK},
                 'baseline_conserved_work')
    return 13


def required_gates(data):
    required = {'mhgp11_tower_classification_'+k for k in ('analytic','prefix','refusals','ownership_concurrency')}
    required |= {'mhgp11_tower_forest_sweep_'+k for k in ('reference','depth_and_memory','timings','concurrency')}
    required |= {'mhgp11_tower_classification_fault_no_allocation','mhgp11_tower_forest_fault_direct_timings',
                 'mhgp11_tower_forest_fault_starvation'}
    required |= {'mhgp11_tower_'+name+suffix for name in ('classification_fraction','classification_oracle_model',
                 'classification_model','forest_fraction','full_campaign','full_bench_semantic','full_bench_io')
                 for suffix in ('','_opt')}
    for prefix in (BASE,SUPP):
        for c in js(data[prefix+'summary.json'])['configurations']:
            if c['status']=='ok' and c['name'] not in ('style','mutants'):
                need(required <= {r['name'] for r in js(data[prefix+c['name']+'/tests.json'])},'required_gates')


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


def check(folder=HERE/'sweep2'):
    receipt,worker,data = old.read_capture(folder)
    need(receipt['commit']==SOURCE and receipt['results_sha256']==PINS['archive'] and
         receipt['plan_sha256']==PINS['plan_sha256'],'capture_pin')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()=='3' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')),'cleanup')
    inventory = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,path = line.split('  ',1); path = 'results/'+path.removeprefix('./')
        need(path not in inventory and path in data and sha(data[path])==digest,'archive_manifest'); inventory.add(path)
    need(inventory == data.keys()-{'results/MANIFEST.sha256'},'archive_inventory')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),('full.json',BENCH)]:
        need((folder/name).read_bytes()==data[path],'compact_copy')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'command_inventory')
    for name,code,timeout in zip(COMMANDS,(0,0,1),(600,180,680)):
        meta = fields(data['results/cmd/'+name+'/meta.txt']); q4.command(meta,code)
        need(meta['requested_timeout_seconds']==str(timeout) and meta.get('streams_truncated')=='0' and
             meta.get('residual_group_killed')=='0','command_closure')
    counts,builds,code = q4.judge_matrix(data)
    need(code==0 and tuple(sum(c[i] for c in counts.values()) for i in range(4))==(2229,2229,0,0),'matrix_passes')
    for name,proof in builds.items():
        if 'CMakeCache.txt' in proof:
            optim.cache(proof,dict(MHGP11_SANITIZE='ON' if name=='gcc_asan_ubsan' else 'OFF',
                                  MHGP11_TSAN='ON' if name=='gcc_tsan' else 'OFF',MHGP11_POISON='ON' if name=='poison' else 'OFF'))
    extra,supp_code = optim.supplement(data)
    need(supp_code==0 and extra==(158,158,0,0),'supplement_passes')
    baseline.supplement_provenance(data); required_gates(data); killed = mutants(data)
    manifest,mhash = old.inputs(folder,receipt); value = js(data[BENCH])
    verdict = report(value,manifest,mhash,builds,data)
    need(worker['commands_total']=='3' and worker['commands_ok']=='2' and worker['status']=='failed' and
         receipt['status']=='failed_remote' and type(receipt['worker_exit_code']) is int and
         receipt['worker_exit_code']==1 and receipt['preserved_failure'] is True,'worker_failure')
    raw = (baseline.HERE/'full3/full.json').read_bytes()
    need(sha(raw)==PINS['baseline_full_sha256'],'baseline_pin')
    baseline.check(); matched = compare_baseline(value,js(raw))
    return dict(coherent=True,source=SOURCE,matrix_passed=2229,asan18_passed=158,causal_mutants=killed,
                baseline_matched=matched,**verdict)


if __name__ == '__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,
                     old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
