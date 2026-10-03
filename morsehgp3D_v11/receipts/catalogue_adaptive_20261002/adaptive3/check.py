"""LIVE evidence reader, no native/cloud execution. Exit zero means coherent evidence.

The original local receipt is mandatory. Frozen helpers are checked before import.
Deleted canonical payloads contribute recorded digests, never a claimed new rehash.
"""
from collections import OrderedDict
import copy
import gzip
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import shlex
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = 'f718f53aa9f76ee19f6be0dc50c1897801f32c1f'
CONTRACT = 'eac85e06ceaef986d04244d37169d5fd636dbd6e415d6a64c7690cee0ec3aa75'
CAPTURE = '0c8fbacd21e72f5095076a6ec3df1fd55367b7564efc662350b6c7f8443fc204'
COMMANDS = ('000_matrice', '001_asan18', '002_adaptive')
SUPP = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_adaptive/files/adaptive.json.gz'
ARCHIVE_CAP = 64 * 1024**2
REPORT_CAP = 128 * 1024**2


def initial_need(value, reason):
    if not value:
        raise ValueError(reason)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def frozen():
    raw = (HERE/'source_contract.json').read_bytes()
    initial_need(hashlib.sha256(raw).hexdigest() == CONTRACT, 'contract_pin')
    pins = json.loads(raw)
    initial_need(pins['source'] == SOURCE, 'source_pin')
    for name, record in pins['frozen'].items():
        data = (HERE/name).read_bytes()
        initial_need(len(data) == record['bytes'] and hashlib.sha256(data).hexdigest() == record['sha256'],
                     'frozen_pin:'+name)
    # Transitive imports keep the historical relative reader paths, inside this capsule only.
    previous = HERE/'frozen/receipts'
    q4 = load('adaptive3_q4', previous/'catalogue_q4_20261002/check.py')
    index = load('adaptive3_index', previous/'index_20261002/check.py')
    names = ('catalogue_semantic', 'catalogue_diagnostics', 'catalogue_g4', 'semantic_cache',
             'catalogue_profiles', 'catalogue_parallel', 'catalogue_adaptive')
    saved = {name: sys.modules.get(name) for name in names}
    try:
        for name in names:
            load(name, HERE/'frozen/bench'/(name+'.py'))
        driver = sys.modules['catalogue_adaptive']
    finally:
        for name, value in saved.items():
            if value is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = value
    return pins, q4, index, driver


PINS, q4, index, driver = frozen()
old, legacy = q4.old, q4.profiles
need, js, sha, fields, BASE = old.need, old.js, old.sha, old.fields, old.BASE
legacy.LOGICAL = driver.profiles.LOGICAL.copy()
old.CAP = ARCHIVE_CAP
REFUSALS = (old.foundation.Refusal, index.old.foundation.Refusal)


def same(actual, expected, label):
    need(json.dumps(actual, sort_keys=True, allow_nan=False) ==
         json.dumps(expected, sort_keys=True, allow_nan=False), label)


def gzip_report(raw, limit=REPORT_CAP):
    need(type(limit) is int and 0 < limit <= REPORT_CAP, 'decompression_limit')
    need(len(raw) <= ARCHIVE_CAP, 'compressed_report_limit')
    # Read at most limit+1 decompressed bytes. CRC/truncation and concatenated-member errors remain errors.
    with gzip.GzipFile(fileobj=io.BytesIO(raw), mode='rb') as stream:
        data = stream.read(limit+1)
        need(len(data) <= limit, 'decompressed_report_limit')
        need(stream.read(1) == b'', 'decompressed_report_trailing')
    result = js(data)
    need(type(result) is dict, 'report_object')
    return result


def identity(row):
    need(type(row['case']) is str and all(type(row[k]) is int for k in
         ('coord_bits', 'kmax', 'workers', 'repetition', 'optimizations')), 'unit_types')
    return driver.identity(row)


def command_argv(row, case, build):
    name, bits, k, workers, repeat, mode = identity(row)
    argv = row['argv']
    need(len(argv) == 13 and argv[0] == build['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == '%s_b%d_k%d_w%d_r%d_o%d_d1.bin' % (name,bits,k,workers,repeat,mode) and
         argv[4:] == [str(k), '16', '256', '0', str(2**32-1), str(8*1024**3), str(workers), str(mode), '1'],
         'native_argv')
    need(type(row['timeout_seconds']) is int and row['timeout_seconds'] == 15 and
         row['whole_input'] is True and type(row['count']) is int and row['count'] == case['count'], 'native_input')
    need(row['semantic_reuse_requested'] is True, 'reuse_requested')


def attempt(row, case, build, complete, last, data):
    command_argv(row, case, build)
    need(row['exit_code'] is None or type(row['exit_code']) is int, 'exit_code_type')
    compatible = copy.deepcopy(row)
    compatible['timeout_seconds'] = 30
    compatible['argv'] = row['argv'][:10]
    compatible['argv'][3] = str(Path(row['argv'][3]).with_name('%s_b%d_k%d.bin' %
                              (row['case'],row['coord_bits'],row['kmax'])))
    post = {'parallel', 'adaptive', 'attempt_validation'}
    if row['status'] == 'invalid_output' and any(e['stage'] in post for e in row['errors']):
        need(row['exit_code'] == 0, 'postdecode_failure_code')
        compatible['status'] = 'artifact_error'
    for error in compatible['errors']:
        if error['stage'] in post:
            error['stage'] = 'artifact'
    legacy.attempt(compatible, case, build, complete, last)
    if row['status'] != 'ok':
        return 0
    need(row['stderr'] == '' and all(e.get('reason', 'none') == 'none' for e in row['events']),
         'success_diagnostics')
    driver.profiles.q4_signature(row)
    driver.profiles.work_signature(row)
    checked = copy.deepcopy(row)
    # The frozen diagnostics function has a late import: restore only its frozen dependency during this call.
    saved = sys.modules.get('catalogue_parallel')
    sys.modules['catalogue_parallel'] = driver.parallel
    try:
        driver.check_adaptive(checked, row)
    finally:
        if saved is None:
            sys.modules.pop('catalogue_parallel', None)
        else:
            sys.modules['catalogue_parallel'] = saved
    need(checked['status'] == 'ok' and checked['errors'] == [], 'frozen_adaptive_verdict')
    for key in ('stage_ms', 'pool_ms', 'catalogue_within_200ms', 'cloud_pool_catalogue_ms',
                'cache_work', 'diagnostic_summary'):
        same(row[key], checked[key], 'derived_'+key)
    value, bits = row['semantic'], row['coord_bits']
    need(all(type(value[k]) is int and 0 <= value[k] < 2**32 for k in
             ('coord_bits', 'kmax', 'sites', 'levels', 'balls')) and
         type(value['incidences']) is int and 0 <= value['incidences'] < 2**64, 'semantic_counts')
    limbs = [(b+63)//64 if b > 127 else 2 for b in (8*bits+12, 6*bits+8)]
    size = 58+40*case['count']+8*(4+sum(limbs))*value['levels']+72*value['balls']+8*value['incidences']
    need(row['canonical_bytes'] == size <= driver.profiles.semantic.LIMIT, 'canonical_exact_size')
    need(row['catalogue_ms']/1000 <= row['process_wall_seconds'], 'api_process_wall')
    matches = [v for p,v in data.items() if p.startswith('results/cmd/002_adaptive/') and
               Path(p).name == Path(row['argv'][3]).name]
    need(len(matches) <= 1, 'canonical_archive_duplicate')
    if not matches:
        return 0
    raw = matches[0]
    need(len(raw) == size and sha(raw) == row['canonical_sha256'], 'canonical_archived_bytes')
    decoded = driver.profiles.semantic.decode(raw, bits, row['kmax'], case['count'], arity_counts=True)
    same(decoded.pop('qmin_counts'), row['qmin_counts'], 'archived_qmin')
    same(decoded, value, 'archived_semantic')
    return 1


def reuse_chain(rows, cases):
    """Replay bounded publication/FIFO authority, never infer the deleted bytes from their SHA."""
    entries = OrderedDict()
    counts = dict(decoded=0, reused=0, published=0)
    decoder = driver.reuse.decoder_digest([Path(driver.profiles.semantic.__file__)])
    for row in rows:
        need(row['semantic_reuse_requested'] is True, 'reuse_attempt_request')
        if 'semantic_reuse' not in row:
            need(row['status'] != 'ok', 'success_without_reuse_evidence')
            continue
        ev, case = row['semantic_reuse'], cases[row['case']]
        need(type(ev) is dict and set(ev) == {'schema', 'mode', 'context', 'current_attempt', 'source_attempt',
             'raw_sha256', 'bytes', 'hash_wall_seconds', 'decode_wall_seconds'}, 'reuse_fields')
        context = dict(format='MHGP11CAT1', decoder_version=driver.profiles.semantic.SCHEMA+';arity_counts=true',
            decoder_sha256=decoder, coord_bits=row['coord_bits'], kmax=row['kmax'], count=case['count'],
            xyz_sha256=case['sha256'], ids_sha256=case['ids_sha256'])
        driver.reuse.Context(**context)
        same(ev['context'], context, 'reuse_context')
        current = [row['case'], row['coord_bits'], row['kmax'], row['workers'], row['repetition'],
                   row['optimizations'], True]
        same(ev['current_attempt'], current, 'reuse_current_attempt')
        need(ev['schema'] == driver.reuse.SCHEMA and ev['mode'] in ('decoded', 'reused') and
             ev['raw_sha256'] == row['canonical_sha256'] and ev['bytes'] == row['canonical_bytes'] and
             type(ev['bytes']) is int and ev['bytes'] > 0 and old.HEX.fullmatch(ev['raw_sha256']), 'reuse_raw')
        need(legacy.finite(ev['hash_wall_seconds']) and legacy.finite(ev['decode_wall_seconds']) and
             legacy.finite(row['semantic_wall_seconds']) and
             ev['hash_wall_seconds']+ev['decode_wall_seconds'] <= row['semantic_wall_seconds'], 'reuse_clocks')
        key = json.dumps([context, ev['raw_sha256'], ev['bytes']], sort_keys=True)
        counts[ev['mode']] += 1
        if key in entries:
            origin, summary = entries[key]
            need(ev['mode'] == 'reused' and ev['decode_wall_seconds'] == 0, 'reuse_expected_hit')
            same(ev['source_attempt'], origin, 'reuse_origin')
            if 'semantic' in row:
                same(dict(row['semantic'], qmin_counts=row['qmin_counts']), summary, 'reuse_summary')
        else:
            need(ev['mode'] == 'decoded', 'reuse_without_valid_origin')
            same(ev['source_attempt'], current, 'decoded_origin')
        if row['status'] == 'ok' and ev['mode'] == 'decoded':
            need(key not in entries, 'duplicate_cache_publication')
            entries[key] = (copy.deepcopy(current), copy.deepcopy(dict(row['semantic'], qmin_counts=row['qmin_counts'])))
            if len(entries) > 64:
                entries.popitem(last=False)
            counts['published'] += 1
    return counts


def schedule(report):
    requested = driver.schedule()
    same(report['requested'], requested, 'schedule_exact')
    for row in requested:
        identity(row)
    for key,value in dict(requested_runs=36, timeout_seconds=15, native_schedule_bound_seconds=540,
                          budget_seconds=550, leaf_size=16, max_leaf=256).items():
        same(report[key], value, 'parameter_'+key)
    same(report['semantic_reuse'], dict(schema=driver.reuse.SCHEMA, capacity=64, summary_limit_bytes=65536,
         assumption='SHA256 collision resistance; immutable campaign artifacts',
         scope='current payload fully rehashed; only validated summaries reused; all current-event checks repeated'),
         'reuse_policy')
    need(all(type(report[k]) is bool for k in ('complete', 'conforming', 'full_schedule_completed')), 'report_bool')
    runs, omitted, intents = report['runs'], report['not_run'], report['launch_intents']
    keys, skips, wanted = list(map(identity,runs)), list(map(identity,omitted)), list(map(identity,requested))
    need(keys == wanted[:len(keys)] and len(keys) <= 36, 'run_prefix')
    need(skips == wanted[len(keys):len(keys)+len(skips)] and len(keys)+len(skips) <= 36 and
         all(r['reason'] == 'campaign_budget_before_launch' for r in omitted), 'budget_omissions')
    if report['complete']:
        need(len(keys)+len(skips) == 36, 'complete_inventory')
    for i,row in enumerate(runs):
        if row['status'] == 'pending_semantic':
            need(not report['complete'] and not omitted and i == len(runs)-1, 'pending_last')
    need(len(intents) == len(runs) or (not report['complete'] and not omitted and len(intents) == len(runs)+1),
         'intent_inventory')
    need(list(map(identity,intents)) == wanted[:len(intents)], 'intent_order')
    for row,intent in zip(runs,intents):
        need(row['argv'] == intent['argv'], 'intent_argv')
    comparisons = js(json.dumps(driver.comparisons(runs, requested)))
    same(report['comparisons'], comparisons, 'comparisons_recomputed')
    done = len(keys) == 36 and not omitted
    good = done and all(r['status'] == 'ok' for r in runs) and all(c['status'] == 'equal' for c in comparisons)
    need(report['full_schedule_completed'] is (done if report['complete'] else False) and
         report['conforming'] is (good if report['complete'] else False), 'campaign_verdict')
    if report['complete']:
        need(legacy.finite(report['campaign_wall_seconds']), 'campaign_duration')
        paid = sum(r['process_wall_seconds']+r.get('semantic_wall_seconds',0) for r in runs)
        need(paid <= report['campaign_wall_seconds'], 'paid_duration')
        if omitted:
            need(report['campaign_wall_seconds'] >= 515, 'budget_elapsed')
    return dict(attempts=len(runs), successes=sum(r['status']=='ok' for r in runs), omissions=len(omitted),
                unpersisted=36-len(keys)-len(skips), conforming=report['conforming'],
                different=sum(c['status']=='different' for c in comparisons),
                equal=sum(c['status']=='equal' for c in comparisons))


def benchmark(report, manifest, mhash, qhash, shash, builds, data):
    need(report['schema'] == driver.SCHEMA and report['attempt_schema'] == 'ehgp.v11.catalogue_attempt.v2' and
         report['manifest_sha256'] == mhash and report['qualification_sha256'] == qhash and
         report['supplement_sha256'] == shash, 'report_identity')
    same(report['manifest'], manifest, 'manifest_copy')
    records = report['builds']
    need(len(records) == 3 and [p['coord_bits'] for p in records] == [18,21,24], 'build_inventory')
    by_bits = {}
    for pin in records:
        bits = pin['coord_bits']; name = legacy.PROFILES[bits]; files = builds[name]
        need(type(bits) is int and pin['configuration'] == name and
             Path(pin['path']).parts[-3:] == (name,'build','mhgp11_catalogue_bench'), 'build_identity')
        for key,value in [('sha256',files['mhgp11_catalogue_bench']['sha256']),
                          ('bytes',files['mhgp11_catalogue_bench']['size']),
                          ('cache_sha256',files['CMakeCache.txt']['sha256']),
                          ('provenance_sha256',sha(data[BASE+name+'/build_provenance.json']))]:
            same(pin[key], value, 'build_'+key)
        by_bits[bits] = pin
    result = schedule(report)
    cases = {c['name']:c for c in manifest['cases']}
    archived = 0
    for i,row in enumerate(report['runs']):
        archived += attempt(row,cases[row['case']],by_bits[row['coord_bits']],report['complete'],
                            i==len(report['runs'])-1,data)
    chain = reuse_chain(report['runs'], cases)
    for row in report['launch_intents']:
        case = cases[row['case']]
        command_argv(row,case,by_bits[row['coord_bits']])
        need(row['input_sha256'] == case['sha256'] and row['ids_sha256'] == case['ids_sha256'], 'intent_inputs')
    return dict(result, reuse=chain, payloads_rehashed=archived, recorded_payloads=result['successes']-archived)


def configuration(config, data, declared):
    for key in ('cmake_options', 'ctest_args'):
        same(config[key], declared[key], 'declared_'+key)
    files = old.provenance(dict(config,status='failed'), data)
    if config['status'] == 'ok':
        value = js(data[BASE+config['name']+'/build_provenance.json'])
        need(value['complete'] is True and not value['errors'], 'complete_provenance')
        if config['name'] != 'style':
            required = {'libmhgp11.a','CMakeCache.txt'}
            required |= {'mhgp11_tower_forest_probe','mhgp11_num_probe','mhgp11_index_probe'} if (
                config['name']=='gcc_asan_ubsan18') else {'mhgp11_catalogue_bench'}
            need(required <= files.keys(), 'required_binaries')
    if 'CMakeCache.txt' in files:
        lines = files['CMakeCache.txt']['text'].splitlines()
        for flag in declared['cmake_options']:
            key,value = flag[2:].split('=',1)
            actual = [s.split('=',1)[1] for s in lines if s.startswith(key+':')]
            need(actual == [value.replace('{threads}',str(config['threads']))], 'cache_'+key)
    return files


def supplement(data):
    mapped = {BASE+p[len(SUPP):]:v for p,v in data.items() if p.startswith(SUPP)}
    summary = js(mapped[BASE+'summary.json']); configs = summary['configurations']
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['requested'] == ['gcc_asan_ubsan18'] and len(configs) == 1 and
         configs[0]['name'] == 'gcc_asan_ubsan18' and
         summary['statuses'] == {'gcc_asan_ubsan18':configs[0]['status']}, 'supplement_inventory')
    c = configs[0]
    if 'tests' not in c and c['status'] in old.EARLY | {'build_failed'}:
        need(js(mapped[BASE+c['name']+'/result.json']) == c and c['conforming'] is False, 'early_supplement')
        counts = (0,0,0,0)
    else:
        counts = old.foundation.judge_config(c, mapped)
    configuration(c, mapped, PINS['supplement']['configurations'][0])
    code = 0 if c['status'] == 'ok' and not summary.get('signals') else 3 if c['status'] in (
        'vacuous','incomplete','floor_violated') and not summary.get('signals') else 1
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and summary['conforming'] is (code==0),
         'supplement_verdict')
    return counts,code


def mutant_evidence(data):
    prefix = BASE+'mutants/'
    cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    need(set(cases) == {'mhgp11_mutants_'+m+s for m in PINS['mutants']
                       for s in ('','_manifest','_manifest_opt')}, 'mutant_inventory')
    count = 0
    for module,entries in PINS['mutants'].items():
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status') == 'run' and case.find('failure') is None and case.find('skipped') is None,
             'mutant_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        n = len(entries); construction = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,construction,n)) in lines, 'mutant_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind == 'construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1, 'mutant_cause')
        count += n
    return count


def timeout_mutants(config, data):
    """A started final gate with no result remains unqualified, never a passing mutant campaign."""
    prefix = BASE+'mutants/'
    same(js(data[prefix+'result.json']),config,'mutant_result_copy')
    need(config['name']=='mutants' and config['status']=='timeout' and config['conforming'] is False and
         config['reason']=='ctest : timeout' and prefix+'junit.xml' not in data,'mutant_timeout')
    modules = ('core','num','sched','cloud','index','catalogue','tower')
    expected = [dict(name='mhgp11_mutants_'+m+s,disabled=False,labels=['long' if s=='' else 'fast','mutant'])
                for m in modules for s in ('_manifest','_manifest_opt','')]
    same(js(data[prefix+'tests.json']),expected,'mutant_selection')
    names = [t['name'] for t in expected]
    lines = data[prefix+'ctest.log'].decode().splitlines()
    need(len(lines)==42 and lines[0].startswith('Test project '),'mutant_log_inventory')
    starts,passed = [],[]
    for line in lines[1:]:
        start = re.fullmatch(r'\s*Start (\d+): (\w+)',line)
        if start:
            need(len(starts)==len(passed),'mutant_unfinished_before_next')
            starts.append((int(start[1]),start[2]))
            continue
        result = re.fullmatch(r'\s*(\d+)/21 Test #(\d+): (\w+) \.+\s+Passed\s+(\d+\.\d+) sec',line)
        need(result is not None and len(starts)==len(passed)+1 and
             int(result[1])==len(starts) and (int(result[2]),result[3])==starts[-1],'mutant_ctest_verdict')
        passed.append(result[3])
    need([name for _,name in starts]==names and passed==names[:-1] and len({i for i,_ in starts})==21,
         'mutant_log_selection')
    same(config['tests'],dict(selected=21,passed=20,failed=0,not_run=1,ctest_total=None,ctest_failed=None),
         'mutant_counts')
    same(config['passed_labels'],dict(fast=14,long=6,mutant=20),'mutant_labels')
    same(config['not_run'],[dict(test=names[-1],state='no_result',labels=['long','mutant'],seconds=0.0,
         detail='aucun resultat (ctest coupe, ou porte jamais lancee)',excerpt=[])],'mutant_open_result')
    need(config['failures']==[] and type(config['threads']) is int and config['threads']==12,'mutant_failure_threads')
    steps = old.foundation.unique((s['name'],s) for s in config['steps'])
    need(set(steps)=={'configure','build','list','test'} and
         all(steps[k]['status']=='ok' and steps[k]['exit_code']==0 for k in ('configure','build','list')),
         'mutant_preparation')
    test = steps['test']
    need(test['status']=='timeout' and test['timed_out'] is True and test['exit_code']==-9 and
         legacy.finite(test['seconds']) and legacy.finite(test['timeout_seconds']) and
         0 < test['timeout_seconds'] <= test['seconds'] < 700 <= config['seconds'] < 750,'mutant_deadline')
    return (21,20,0,1)


def matrix(data):
    summary = js(data[BASE+'summary.json']); configs = summary['configurations']
    names = [c['name'] for c in configs]
    need(summary['schema']=='ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         len(names)==len(set(names)) and set(names)==old.foundation.NAMES and summary['requested']==names,
         'matrix_inventory')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]}==set(names),
         'matrix_directories')
    same(summary['statuses'],{c['name']:c['status'] for c in configs},'matrix_statuses')
    counts,builds = {},{}
    for c in configs:
        name = c['name']
        counts[name] = timeout_mutants(c,data) if name=='mutants' else old.foundation.judge_config(c,data)
        builds[name] = old.provenance(c,data)
        if 'CMakeCache.txt' in builds[name]:
            found = [s.split('=',1)[1] for s in builds[name]['CMakeCache.txt']['text'].splitlines()
                     if s.startswith('MHGP11_COORD_BITS:')]
            need(found==[str(legacy.CONFIG_BITS[name])],'profile_configuration')
    need(summary['conforming'] is False and type(summary['exit_code']) is int and
         summary['exit_code']==1 and summary['signals']==[],'matrix_timeout_verdict')
    return counts,builds,1


def required_gates(data):
    required = {'mhgp11_catalogue_'+s for s in ('adaptive_model','adaptive_collector','adaptive_bench_io',
                                             'adaptive_fraction','adaptive_combined_fraction','adaptive_judge',
                                             'semantic_cache')}
    required |= {s+'_opt' for s in list(required)}
    required |= {'mhgp11_catalogue_adaptive_'+s for s in
                 ('equivalence','round_memory','plan_limits','diagnostics_transaction','fault_allocations')}
    for c in js(data[BASE+'summary.json'])['configurations']:
        if c['status'] == 'ok' and c['name'] not in ('style','mutants'):
            names = {t['name'] for t in js(data[BASE+c['name']+'/tests.json'])}
            need(required <= names, 'adaptive_gates_missing')


def check(folder=HERE):
    proof_raw = (folder/'capture_contract.json').read_bytes()
    need(sha(proof_raw)==CAPTURE,'capture_contract_pin')
    proof = js(proof_raw)
    for name,digest in proof['files'].items():
        need(sha((folder/name).read_bytes())==digest,'compact_pin')
    receipt,worker,data = old.read_capture(folder)
    raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local','original_receipt_sha256','preserved_failure'} <= set(raw),
         'receipt_raw_fields')
    need(receipt['commit'] == SOURCE and
         receipt['plan_sha256'] == PINS['source_files']['bench/plans/catalogue_adaptive_g4.json']['sha256'] and
         receipt['worker_plan_sha256'] == worker['plan_sha256'], 'executed_source_plan')
    need(receipt['original_receipt_sha256']==proof['original_receipt_sha256'] and
         receipt['results_sha256']==proof['archive_sha256'] and receipt['results_bytes']==proof['archive_bytes'] and
         receipt['generation']==proof['generation'] and proof['source']==SOURCE,'capture_identity')
    same(receipt['target'], dict(instance='ehgp-v7-3b1d496aed430749ea7e049f',
         project='devpod-gpu-exploration', zone='us-central1-c'), 'target')
    done = (Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()
    need(done == ('0' if receipt['status']=='completed' else '3') and all(receipt[k] is True for k in
         ('private_key_deleted','oslogin_key_removed','reserve_released')), 'session_cleanup')
    inventory = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,path = line.split('  ',1); path = 'results/'+path.removeprefix('./')
        need(path not in inventory and path in data and sha(data[path]) == digest, 'archive_manifest')
        inventory.add(path)
    need(inventory == data.keys()-{'results/MANIFEST.sha256'}, 'archive_inventory')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS), 'command_inventory')
    metas = []
    for name,declared in zip(COMMANDS,PINS['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = fields(data[prefix+'meta.txt']); metas.append(meta)
        need(meta['requested_timeout_seconds'] == str(declared['timeout_seconds']) and
             meta.get('group_closed') == '1' and meta.get('streams_truncated') == '0' and
             meta.get('residual_group_killed') == '0', 'command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode())
        root = argv[1].split('/src/morsehgp3D_v11/',1)[0]
        replacements = dict(src=root+'/src',build=root+'/build',data=root+'/data',
                            out=root+'/results/cmd/'+name+'/files')
        same(argv,[a.format(**replacements) for a in declared['argv']], 'executed_plan')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),
                      ('adaptive.json.gz',BENCH)]:
        need((folder/name).read_bytes() == data[path] if path in data else not (folder/name).exists(), 'compact_copy')
    counts,builds,matrix_code = matrix(data)
    same({name:values[0] for name,values in counts.items()},proof['matrix_counts'],'captured_selection_counts')
    required_gates(data)
    declared = {c['name']:c for c in PINS['matrix']['configurations']}
    for c in js(data[BASE+'summary.json'])['configurations']:
        configuration(c,data,declared[c['name']])
    mutants = mutant_evidence(data) if js(data[BASE+'mutants/result.json'])['status'] == 'ok' else None
    extra,supp_code = supplement(data)
    need(extra==(proof['asan18_count'],proof['asan18_count'],0,0),'captured_supplement_counts')
    q4.command(metas[0],matrix_code); q4.command(metas[1],supp_code)
    manifest,mhash = old.inputs(folder,receipt)
    verdict = dict(conforming=False,attempts=0,successes=0,omissions=0,unpersisted=0,unstarted_declared_units=36,
                   equal=0,different=0,
                   payloads_rehashed=0,recorded_payloads=0,reuse=dict(decoded=0,reused=0,published=0))
    if BENCH in data:
        need(matrix_code == supp_code == 0, 'unqualified_benchmark')
        report = gzip_report(data[BENCH])
        verdict = benchmark(report,manifest,mhash,sha(data[BASE+'summary.json']),sha(data[SUPP+'summary.json']),builds,data)
        q4.command(metas[2],0 if report['conforming'] else 1 if report['complete'] else None)
    else:
        q4.command(metas[2],2)
        prefix = 'results/cmd/002_adaptive/'
        need(data[prefix+'stdout']==b'catalogue_adaptive_refused: ValueError\n' and data[prefix+'stderr']==b'' and
             not any(p.startswith(prefix+'files/') for p in data),'no_adaptive_benchmark')
    success = q4.session(receipt,worker,metas)
    need(success is verdict['conforming'] and receipt['preserved_failure'] is (not success), 'session_verdict')
    return dict(coherent=True,source=SOURCE,matrix_passed=sum(v[1] for v in counts.values()),
                matrix_total=sum(v[0] for v in counts.values()),asan18_passed=extra[1],asan18_total=extra[0],
                causal_mutants=mutants,
                individual_mutant_verdicts='not_certified' if mutants is None else 'certified',**verdict)


if __name__ == '__main__':
    try:
        print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,EOFError,tarfile.TarError,
                     old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
