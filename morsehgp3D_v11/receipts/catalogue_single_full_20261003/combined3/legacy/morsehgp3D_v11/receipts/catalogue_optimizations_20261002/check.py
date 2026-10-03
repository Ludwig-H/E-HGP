"""LIVE optimization evidence, never a new native run. Code0 means coherent evidence.

Raw local receipt is mandatory. Tar is read, never extracted. A present canonical
payload is decoded again; a deleted payload contributes only recorded hashes.
"""
import copy
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = 'df069960a0e109ea15c94115537576f540f4de1b'
CONTRACT = '7b99b59dc864fc99b3bf1e1c7b9f210f7b21ebc7ff286f036db5a9ce17929a23'
COMMANDS = ('000_matrice','001_asan18','002_optimizations')
SUPP = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_optimizations/files/optimizations.json'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


q4 = load('optim_q4',HERE.parent/'catalogue_q4_20261002/check.py')
index = load('optim_index',HERE.parent/'index_20261002/check.py')
old, legacy = q4.old,q4.profiles
need,js,sha,fields,BASE = old.need,old.js,old.sha,old.fields,old.BASE
REFUSALS = (old.foundation.Refusal,index.old.foundation.Refusal)


def same(actual,expected,label):
    need(json.dumps(actual,sort_keys=True,allow_nan=False) ==
         json.dumps(expected,sort_keys=True,allow_nan=False),label)


def frozen():
    raw = (HERE/'source_contract.json').read_bytes()
    need(sha(raw) == CONTRACT,'contract_pin'); pins = js(raw)
    need(pins['source'] == SOURCE,'source_pin')
    names = ('catalogue_g4','catalogue_semantic','catalogue_profiles','catalogue_parallel','catalogue_optimizations')
    saved = {name:sys.modules.get(name) for name in names}
    try:
        for name in names:
            path = HERE/'source_df'/(name+'.py')
            need(sha(path.read_bytes()) == pins['scripts'][name+'.py']['sha256'],'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name] = module; spec.loader.exec_module(module)
        return pins,sys.modules['catalogue_optimizations']
    finally:
        for name,value in saved.items():
            if value is None: sys.modules.pop(name,None)
            else: sys.modules[name] = value


PINS,driver = frozen()
legacy.LOGICAL = driver.profiles.LOGICAL.copy()


def identity(row):
    need(type(row['case']) is str and all(type(row[k]) is int for k in
         ('coord_bits','kmax','workers','repetition','optimizations')),'unit_types')
    return driver.identity(row)


def command_argv(row,case,build):
    name,bits,k,workers,repeat,mode = identity(row)
    suffix = '_o%d' % mode if mode else ''
    argv = row['argv']
    need(len(argv) == (12 if mode else 11) and argv[0] == build['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == '%s_b%d_k%d_w%d_r%d%s.bin' % (name,bits,k,workers,repeat,suffix) and
         argv[4:] == [str(k),'16','256','0',str(2**32-1),str(8*1024**3),str(workers)]+([str(mode)] if mode else []),
         'native_argv')
    need(row['timeout_seconds'] == 15 and row['whole_input'] is True and
         type(row['count']) is int and row['count'] == case['count'],'native_input')


def attempt(row,case,build,complete,last,data):
    command_argv(row,case,build)
    need(row['exit_code'] is None or type(row['exit_code']) is int,'exit_code_type')
    # Keep the historical validator for the unchanged process/semantic fields.
    # Optimization diagnostics are collected AFTER a valid canonical artifact:
    # invalid_output can therefore legitimately retain that artifact's metadata.
    compatible = copy.deepcopy(row); compatible['timeout_seconds'] = 30
    compatible['argv'] = row['argv'][:10]
    compatible['argv'][3] = str(Path(row['argv'][3]).with_name('%s_b%d_k%d.bin' %
                               (row['case'],row['coord_bits'],row['kmax'])))
    post = any(e['stage'] in ('parallel','optimization') for e in row['errors'])
    if row['status'] == 'invalid_output' and post:
        need(row['exit_code'] == 0,'postdecode_failure_code')
        compatible['status'] = 'artifact_error'
    for error in compatible['errors']:
        if error['stage'] in ('parallel','optimization'): error['stage'] = 'artifact'
    legacy.attempt(compatible,case,build,complete,last)
    if row['status'] != 'ok':
        need(row['status'] in ('artifact_error','invalid_output') or 'qmin_counts' not in row,'early_qmin')
        return 0
    need(row['stderr'] == '' and all(e.get('reason','none') == 'none' for e in row['events']),'success_diagnostics')
    q4.q4_signature(row); driver.profiles.work_signature(row)
    checked = copy.deepcopy(row); driver.check_optimization(checked,row)
    need(checked['status'] == 'ok' and checked['errors'] == [],'frozen_optimization_verdict')
    for key in ('stage_ms','pool_ms','catalogue_within_200ms','cloud_pool_catalogue_ms','cache_work'):
        same(row[key],checked[key],'derived_'+key)
    value = row['semantic']; bits = row['coord_bits']
    need(all(type(value[k]) is int and 0 <= value[k] < 2**32 for k in
             ('coord_bits','kmax','sites','levels','balls')) and type(value['incidences']) is int and
         0 <= value['incidences'] < 2**64,'semantic_counts')
    limbs = [(b+63)//64 if b > 127 else 2 for b in (8*bits+12,6*bits+8)]
    size = 58+40*case['count']+8*(4+sum(limbs))*value['levels']+72*value['balls']+8*value['incidences']
    need(row['canonical_bytes'] == size <= driver.profiles.semantic.LIMIT,'canonical_exact_size')
    need(row['catalogue_ms']/1000 <= row['process_wall_seconds'],'api_process_wall')
    matches = [v for p,v in data.items() if p.startswith('results/cmd/002_optimizations/') and
               Path(p).name == Path(row['argv'][3]).name]
    need(len(matches) <= 1,'canonical_archive_duplicate')
    if not matches: return 0
    raw = matches[0]
    need(len(raw) == size and sha(raw) == row['canonical_sha256'],'canonical_archived_bytes')
    decoded = driver.profiles.semantic.decode(raw,bits,row['kmax'],case['count'],arity_counts=True)
    same(decoded.pop('qmin_counts'),row['qmin_counts'],'archived_qmin')
    same(decoded,value,'archived_semantic')
    return 1


def schedule(report):
    requested = driver.schedule(); same(report['requested'],requested,'schedule_exact')
    for row in requested: identity(row)
    for key,value in dict(requested_runs=36,timeout_seconds=15,native_schedule_bound_seconds=540,
                          budget_seconds=700,leaf_size=16,max_leaf=256).items():
        same(report[key],value,'parameter_'+key)
    need(all(type(report[k]) is bool for k in ('complete','conforming','full_schedule_completed')),'report_bool')
    runs,omitted,intents = report['runs'],report['not_run'],report['launch_intents']
    keys = list(map(identity,runs)); skips = list(map(identity,omitted)); wanted = list(map(identity,requested))
    need(keys == wanted[:len(keys)] and len(keys) <= 36,'run_prefix')
    need(skips == wanted[len(keys):len(keys)+len(skips)] and len(keys)+len(skips) <= 36 and
         all(r['reason'] == 'campaign_budget_before_launch' for r in omitted),'budget_omissions')
    if report['complete']: need(len(keys)+len(skips) == 36,'complete_inventory')
    for i,row in enumerate(runs):
        if row['status'] == 'pending_semantic': need(not report['complete'] and not omitted and i == len(runs)-1,'pending_last')
    need(len(intents) == len(runs) or (not report['complete'] and not omitted and len(intents) == len(runs)+1),
         'intent_inventory')
    need(list(map(identity,intents)) == wanted[:len(intents)],'intent_order')
    for row,intent in zip(runs,intents):
        need(row['argv'] == intent['argv'],'intent_argv')
    comparisons = js(json.dumps(driver.comparisons(runs,requested)))
    same(report['comparisons'],comparisons,'comparisons_recomputed')
    done = len(keys) == 36 and not omitted
    good = done and all(r['status'] == 'ok' for r in runs) and all(c['status'] == 'equal' for c in comparisons)
    need(report['full_schedule_completed'] is (done if report['complete'] else False) and
         report['conforming'] is (good if report['complete'] else False),'campaign_verdict')
    if report['complete']:
        need(legacy.finite(report['campaign_wall_seconds']),'campaign_duration')
        paid = sum(r['process_wall_seconds']+r.get('semantic_wall_seconds',0) for r in runs)
        need(paid <= report['campaign_wall_seconds'],'paid_duration')
        if omitted: need(report['campaign_wall_seconds'] >= 665,'budget_elapsed')
    return dict(attempts=len(runs),successes=sum(r['status']=='ok' for r in runs),omissions=len(omitted),
                unpersisted=36-len(keys)-len(skips),conforming=report['conforming'],
                different=sum(c['status']=='different' for c in comparisons),equal=sum(c['status']=='equal' for c in comparisons))


def benchmark(report,manifest,mhash,qhash,shash,builds,data):
    need(report['schema'] == driver.SCHEMA and report['attempt_schema'] == 'ehgp.v11.catalogue_attempt.v2' and
         report['manifest_sha256'] == mhash and report['qualification_sha256'] == qhash and
         report['supplement_sha256'] == shash,'report_identity')
    same(report['manifest'],manifest,'manifest_copy')
    pins = report['builds']; need(len(pins) == 3 and [p['coord_bits'] for p in pins] == [18,21,24],'build_inventory')
    by_bits = {}
    for pin in pins:
        bits = pin['coord_bits']; name = legacy.PROFILES[bits]; files = builds[name]
        need(type(bits) is int and pin['configuration'] == name and
             Path(pin['path']).parts[-3:] == (name,'build','mhgp11_catalogue_bench'),'build_identity')
        for key,value in [('sha256',files['mhgp11_catalogue_bench']['sha256']),('bytes',files['mhgp11_catalogue_bench']['size']),
                          ('cache_sha256',files['CMakeCache.txt']['sha256']),
                          ('provenance_sha256',sha(data[BASE+name+'/build_provenance.json']))]:
            same(pin[key],value,'build_'+key)
        by_bits[bits] = pin
    result = schedule(report); cases = {c['name']:c for c in manifest['cases']}
    archived = 0
    for i,row in enumerate(report['runs']):
        archived += attempt(row,cases[row['case']],by_bits[row['coord_bits']],report['complete'],i==len(report['runs'])-1,data)
    for row in report['launch_intents']:
        case = cases[row['case']]; command_argv(row,case,by_bits[row['coord_bits']])
        need(row['input_sha256'] == case['sha256'] and row['ids_sha256'] == case['ids_sha256'],'intent_inputs')
    return dict(result,payloads_rehashed=archived,recorded_payloads=result['successes']-archived)


def cache(files,expected):
    lines = files['CMakeCache.txt']['text'].splitlines()
    for key,value in expected.items():
        need([s.split('=',1)[1] for s in lines if s.startswith(key+':')] == [value],'cache_'+key)


def required_gates(data):
    required = {'mhgp11_catalogue_cache_'+s for s in ('ranks','relations_reset','capacity_fallback','memory','equivalence')}
    required |= {'mhgp11_catalogue_sort_'+s for s in ('boundaries','wide_levels','refusals')}
    required |= {'mhgp11_catalogue_cache_fault_allocation','mhgp11_catalogue_sort_fault_allocations'}
    required |= {'mhgp11_catalogue_'+name+suffix for name in ('cache_fraction','sort_fraction','sort_cache_fraction',
                 'optimization_model','optimizations_collector') for suffix in ('','_opt')}
    for c in js(data[BASE+'summary.json'])['configurations']:
        if c['status'] == 'ok' and c['name'] not in ('style','mutants'):
            selected = js(data[BASE+c['name']+'/tests.json'])
            need(required <= {t['name'] for t in selected},'optimization_gates_missing')


def mutant_evidence(data):
    prefix = BASE+'mutants/'
    cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    count = 0
    for module,entries in PINS['mutants'].items():
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status') == 'run' and case.find('failure') is None and case.find('skipped') is None,'mutant_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        n = len(entries); construction = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,construction,n)) in lines,'mutant_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind == 'construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutant_cause')
        count += n
    return count


def supplement(data):
    mapped = {BASE+p[len(SUPP):]:v for p,v in data.items() if p.startswith(SUPP)}
    summary = js(mapped[BASE+'summary.json']); configs = summary['configurations']
    need(summary['complete'] is True and summary['requested'] == ['gcc_asan_ubsan18'] and len(configs) == 1 and
         configs[0]['name'] == 'gcc_asan_ubsan18' and
         summary['statuses'] == {'gcc_asan_ubsan18':configs[0]['status']},'supplement_inventory')
    c = configs[0]
    if 'tests' not in c and c['status'] in old.EARLY | {'build_failed'}:
        need(js(mapped[BASE+c['name']+'/result.json']) == c and c['conforming'] is False,'early_supplement')
        counts = (0,0,0,0)
    else: counts = old.foundation.judge_config(c,mapped)
    proof = old.provenance(dict(c,status='failed'),mapped)
    if c['status'] == 'ok':
        p = js(mapped[BASE+c['name']+'/build_provenance.json'])
        need(p['complete'] is True and not p['errors'] and
             {'libmhgp11.a','mhgp11_num_probe','mhgp11_index_probe','mhgp11_tower_forest_probe'} <= proof.keys(),
             'supplement_provenance')
        cache(proof,dict(MHGP11_COORD_BITS='18',MHGP11_MODULES='num;index;tower',MHGP11_SANITIZE='ON',
                         MHGP11_TSAN='OFF',MHGP11_POISON='OFF'))
    code = 0 if c['status'] == 'ok' and not summary.get('signals') else 3 if c['status'] in (
        'vacuous','incomplete','floor_violated') and not summary.get('signals') else 1
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and summary['conforming'] is (code==0),
         'supplement_verdict')
    return counts,code


def check(folder=HERE/'optimizations2'):
    receipt,worker,data = old.read_capture(folder)
    need(receipt['commit'] == SOURCE and receipt['plan_sha256'] == PINS['plan_sha256'],'executed_source_plan')
    done = (Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()
    need(done == ('0' if receipt['status']=='completed' else '3') and all(receipt[k] is True for k in
         ('private_key_deleted','oslogin_key_removed','reserve_released')),'session_cleanup')
    inventory = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,path = line.split('  ',1); path = 'results/'+path.removeprefix('./')
        need(path not in inventory and path in data and sha(data[path]) == digest,'archive_manifest'); inventory.add(path)
    need(inventory == data.keys()-{'results/MANIFEST.sha256'},'archive_inventory')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS),'command_inventory')
    metas = [fields(data['results/cmd/'+c+'/meta.txt']) for c in COMMANDS]
    for meta,timeout in zip(metas,(600,180,780)):
        need(meta['requested_timeout_seconds'] == str(timeout) and meta.get('group_closed') == '1' and
             meta.get('streams_truncated') == '0' and meta.get('residual_group_killed') == '0','command_closure')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),('optimizations.json',BENCH)]:
        need((folder/name).read_bytes() == data[path] if path in data else not (folder/name).exists(),'compact_copy')
    counts,builds,matrix_code = q4.judge_matrix(data)
    required_gates(data)
    for name,proof in builds.items():
        if 'CMakeCache.txt' in proof:
            cache(proof,dict(MHGP11_SANITIZE='ON' if name=='gcc_asan_ubsan' else 'OFF',
                             MHGP11_TSAN='ON' if name=='gcc_tsan' else 'OFF',MHGP11_POISON='ON' if name=='poison' else 'OFF'))
    mutants = mutant_evidence(data) if js(data[BASE+'mutants/result.json'])['status'] == 'ok' else 0
    extra,supp_code = supplement(data)
    q4.command(metas[0],matrix_code); q4.command(metas[1],supp_code)
    manifest,mhash = old.inputs(folder,receipt)
    verdict = dict(conforming=False,attempts=0,successes=0,omissions=0,unpersisted=36,equal=0,different=0,
                   payloads_rehashed=0,recorded_payloads=0)
    if BENCH in data:
        need(matrix_code == supp_code == 0,'unqualified_benchmark')
        report = js(data[BENCH])
        verdict = benchmark(report,manifest,mhash,sha(data[BASE+'summary.json']),sha(data[SUPP+'summary.json']),builds,data)
        q4.command(metas[2],0 if report['conforming'] else 1 if report['complete'] else None)
    else: q4.command(metas[2])
    success = q4.session(receipt,worker,metas)
    need(success is verdict['conforming'] and receipt['preserved_failure'] is (not success),'session_verdict')
    return dict(coherent=True,source=SOURCE,matrix_passed=sum(v[1] for v in counts.values()),
                matrix_total=sum(v[0] for v in counts.values()),asan18_passed=extra[1],asan18_total=extra[0],
                causal_mutants=mutants,**verdict)


if __name__ == '__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,
                     old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
