"""Closed LIVE catalogue parallel evidence; code0 means coherence, not campaign success.

Reads the original local receipt and one archived result. Published canonical
hashes are compared; deleted payloads cannot be rehashed. Source c104 scripts are
private frozen copies, never the later WIP collector or a FULL qualification.
"""
import copy
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent

def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module); return module

q4 = load('parallel_q4_reader',HERE.parent/'catalogue_q4_20261002/check.py')
index = load('parallel_index_reader',HERE.parent/'index_20261002/check.py')
old, profiles = q4.old,q4.profiles
need,js,sha,fields,BASE = old.need,old.js,old.sha,old.fields,old.BASE
SOURCE = 'c1046dfc7689ce07faaee255671f13df96378b2d'
SUPPLEMENT = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_parallel/files/parallel.json'
COMMANDS = ('000_matrice','001_asan18','002_parallel')
EXPECTED = dict(gcc_release=355,mutants=21,gcc_asan_ubsan=280,gcc_tsan=280,clang_release=0,
                bits21=280,bits24=280,poison=281,style=2,gcc_asan_ubsan18=107)


def source_contract():
    contract = js((HERE/'contract.json').read_bytes())
    need(contract['source_commit'] == SOURCE,'source_contract')
    for name,pin in contract['scripts'].items():
        need(sha((HERE/'source_c104'/name).read_bytes()) == pin['sha256'],'snapshot_script_hash')
    return contract

PINS = source_contract()
sys.path.insert(0,str(HERE/'source_c104'))
import catalogue_parallel as driver
profiles.LOGICAL = driver.profiles.LOGICAL.copy()


def attempt(row,case,pin,complete,last):
    ident = driver.identity(row); name,bits,k,workers,repetition = ident
    argv = row['argv']; expected = '%s_b%d_k%d_w%d_r%d.bin' % ident
    need(row['timeout_seconds'] == 15 and len(argv) == 11 and argv[0] == pin['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == expected and argv[4:] ==
         [str(k),'16','256','0',str(2**32-1),str(8*1024**3),str(workers)],'native_command')
    # Reuse unchanged profile checks after validating the original parallel argv in full.
    legacy = copy.deepcopy(row); legacy['repetition'] = 0; legacy['timeout_seconds'] = 30
    legacy['argv'] = argv[:10]; legacy['argv'][3] = str(Path(argv[3]).with_name('%s_b%d_k%d.bin' % (name,bits,k)))
    for error in legacy['errors']:
        if error['stage'] == 'parallel': error['stage'] = 'success'
    profiles.attempt(legacy,case,pin,complete,last)
    if row['status'] != 'ok':
        need(row['status'] == 'artifact_error' or 'qmin_counts' not in row,'qmin_before_decode')
        return
    need(row['stderr'] == '' and row['events'][1]['workers'] == workers,'native_workers')
    q4.q4_signature(row); driver.profiles.work_signature(row)
    checked = copy.deepcopy(row); driver.check_parallel(checked,row)
    need(checked['status'] == 'ok','frozen_parallel_validation')
    for key in ('stage_ms','pool_ms','catalogue_within_200ms','cloud_pool_catalogue_ms'):
        need(row[key] == checked[key],'derived_'+key)


def schedule(report):
    requested = driver.schedule(); need(report['requested'] == requested,'schedule36')
    need(report['requested_runs'] == 36 and report['timeout_seconds'] == 15 and
         report['native_schedule_bound_seconds'] == 540 and report['leaf_size'] == 16 and report['max_leaf'] == 256
         and type(report['budget_seconds']) is int and report['budget_seconds'] == 750,'protocol')
    need(type(report['complete']) is bool and type(report['full_schedule_completed']) is bool and
         type(report['conforming']) is bool,'report_bool')
    runs,omissions = report['runs'],report['not_run']
    by_id = {driver.identity(r):r for r in runs}; skip = {driver.identity(r):r for r in omissions}
    need(len(by_id) == len(runs) and len(skip) == len(omissions) and not by_id.keys() & skip.keys(),'duplicate_units')
    allowed = {driver.identity(r) for r in requested}; need((by_id.keys() | skip.keys()) <= allowed,'extra_units')
    failures,deadline,observed,gap = set(),False,[],False
    for unit in requested:
        key = driver.identity(unit); pair = key[:2]
        if key not in by_id and key not in skip:
            gap = True; continue
        need(not gap,'nonprefix_checkpoint')
        if key in skip:
            reason = skip[key]['reason']
            if pair in failures: need(reason == 'same_profile_K5_W48_first_attempt_failed','causal_reason')
            else: need(reason == 'campaign_budget_before_launch','unjustified_omission'); deadline = True
        else:
            need(not deadline and pair not in failures,'launch_after_omission')
            row = by_id[key]; observed.append(key)
            if row['status'] not in ('ok','pending_semantic') and key[2:] == (5,48,0): failures.add(pair)
    need(observed == [driver.identity(r) for r in runs],'run_order')
    if report['complete']: need(not gap,'complete_missing_unit')
    for i,row in enumerate(runs):
        if row['status'] == 'pending_semantic': need(not report['complete'] and i == len(runs)-1,'pending_position')
    intents = report['launch_intents']
    need(len(intents) == len(runs) or not report['complete'] and len(intents) == len(runs)+1,'launch_intent_count')
    for intent,row in zip(intents,runs):
        need(driver.identity(intent) == driver.identity(row) and intent['argv'] == row['argv'] and
             intent['timeout_seconds'] == 15 and intent['whole_input'] is True and
             intent['count'] == row['count'],'launch_intent')
    if len(intents) > len(runs):
        next_key = next(driver.identity(r) for r in requested if driver.identity(r) not in by_id and driver.identity(r) not in skip)
        need(driver.identity(intents[-1]) == next_key,'pending_intent_order')
    comparison = js(json.dumps(driver.comparisons(runs,requested)))
    need(report['comparisons'] == comparison or not runs and report['comparisons'] == [],'comparisons')
    done = not omissions and len(runs) == 36
    conforming = done and all(r['status'] == 'ok' for r in runs) and all(c['status'] == 'equal' for c in comparison)
    need(report['full_schedule_completed'] is (done if report['complete'] else False) and
         report['conforming'] is (conforming if report['complete'] else False),'campaign_verdict')
    return dict(attempts=len(runs),omissions=len(omissions),unpersisted=36-len(runs)-len(omissions),
                equal=sum(c['status']=='equal' for c in comparison),different=sum(c['status']=='different' for c in comparison))


def benchmark(report,manifest,mhash,qhash,shash,builds,data):
    need(report['schema'] == driver.SCHEMA and report['attempt_schema'] == 'ehgp.v11.catalogue_attempt.v2' and
         report['manifest'] == manifest and report['manifest_sha256'] == mhash and
         report['qualification_sha256'] == qhash and report['supplement_sha256'] == shash,'benchmark_identity')
    pins = {r['coord_bits']:r for r in report['builds']}
    need(len(report['builds']) == len(pins) == 3 and set(pins) == {18,21,24},'build_inventory')
    for bits,name in profiles.PROFILES.items():
        p = pins[bits]; exe = builds[name]['mhgp11_catalogue_bench']
        need(p['configuration'] == name and p['sha256'] == exe['sha256'] and p['bytes'] == exe['size'] and
             p['provenance_sha256'] == sha(data[BASE+name+'/build_provenance.json']) and
             p['cache_sha256'] == builds[name]['CMakeCache.txt']['sha256'] and
             Path(p['path']).parts[-3:] == (name,'build','mhgp11_catalogue_bench'),'qualified_executable')
    result = schedule(report); cases = {c['name']:c for c in manifest['cases']}
    for i,row in enumerate(report['runs']): attempt(row,cases[row['case']],pins[row['coord_bits']],report['complete'],i==len(report['runs'])-1)
    for intent in report['launch_intents']:
        case = cases[intent['case']]
        need(intent['input_sha256'] == case['sha256'] and intent['ids_sha256'] == case['ids_sha256'],'intent_inputs')
    # Additional mathematical reading, NOT a guard run by the frozen c104 collector.
    bound = 0
    for r in report['runs']:
        if r['status'] == 'ok':
            t = r['events'][1]['timings']
            for phase in ('count','fill'):
                need(t[phase+'_task_sum_ns'] <= min(r['workers'],t['tasks'])*t[phase+'_ns'],'additional_worker_wall_bound')
                bound += 1
    return dict(result,additional_worker_wall_checks=bound)


def mutants(config,data):
    prefix = BASE+'mutants/'
    cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    count = 0
    for module,ids in PINS['mutants'].items():
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status') == 'run' and case.find('failure') is None and case.find('skipped') is None,'mutant_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        n,construction = len(ids),sum(v=='construction' for v in ids.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,construction,n)) in lines,'mutant_totals')
        for ident,kind in ids.items():
            verdict = 'construction' if kind == 'construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+verdict,s)) for s in lines)==1,'mutant_cause_'+ident)
        count += n
    return count


def check(folder):
    receipt,worker,data = old.read_capture(folder)
    need(receipt['commit'] == SOURCE and folder.name == 'parallel5','capture_source')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() == '3' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')),'session_closed')
    inventory = {}
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,name = line.split('  ',1); name = 'results/'+name.removeprefix('./')
        need(name not in inventory and name in data and sha(data[name]) == digest,'archive_manifest'); inventory[name] = digest
    need(set(inventory) == set(data)-{'results/MANIFEST.sha256'},'archive_inventory')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS),'commands')
    metas = [fields(data['results/cmd/'+name+'/meta.txt']) for name in COMMANDS]
    for meta,timeout in zip(metas,(600,180,820)):
        need(meta['requested_timeout_seconds'] == str(timeout) and meta['group_closed'] == '1' and
             meta['streams_truncated'] == '0' and meta['residual_group_killed'] == '0','command_closure')
    for filename,path in (('matrix.json',BASE+'summary.json'),('asan18.json',SUPPLEMENT+'summary.json'),('parallel.json',BENCH)):
        need((folder/filename).read_bytes() == data[path],'compact_copy_'+filename)
    counts,builds,code = q4.judge_matrix(data)
    for cfg in js(data[BASE+'summary.json'])['configurations']:
        need(counts[cfg['name']][0] == EXPECTED[cfg['name']],'matrix_count')
        if cfg['status'] == 'ok' and cfg['name'] != 'style':
            cache = builds[cfg['name']]['CMakeCache.txt']['text'].splitlines()
            for key,active in [('MHGP11_SANITIZE',cfg['name']=='gcc_asan_ubsan'),
                               ('MHGP11_TSAN',cfg['name']=='gcc_tsan'),('MHGP11_POISON',cfg['name']=='poison')]:
                need([line.split('=',1)[1] for line in cache if line.startswith(key+':')] ==
                     ['ON' if active else 'OFF'],'matrix_instrumentation')
    total_mutants = mutants(next(c for c in js(data[BASE+'summary.json'])['configurations'] if c['name']=='mutants'),data)
    summary = js(data[SUPPLEMENT+'summary.json']); configs = summary['configurations']
    need(summary['requested'] == ['gcc_asan_ubsan18'] and len(configs)==1 and
         configs[0]['name']=='gcc_asan_ubsan18' and summary['complete'] is True,'supplement_inventory')
    mapped = {BASE+p[len(SUPPLEMENT):]:v for p,v in data.items() if p.startswith(SUPPLEMENT)}
    extra = old.foundation.judge_config(configs[0],mapped)
    # The supplement intentionally has no catalogue executable. Reuse hash/text validation,
    # then require its actual num/index/tower targets instead of the historical catalogue target.
    provenance = old.provenance(dict(configs[0],status='failed'),mapped)
    proof = js(mapped[BASE+'gcc_asan_ubsan18/build_provenance.json'])
    need(proof['complete'] is True and not proof['errors'] and
         {'libmhgp11.a','mhgp11_num_probe','mhgp11_index_probe','mhgp11_tower_probe',
          'mhgp11_tower_cells_probe','mhgp11_tower_descent_probe'} <= set(provenance),'supplement_builds')
    need(extra[0] == 107 and summary['conforming'] is True and summary['exit_code'] == 0 and
         configs[0]['status']=='ok' and not summary.get('signals'),'supplement_verdict')
    cache = provenance['CMakeCache.txt']['text'].splitlines()
    for key,value in dict(MHGP11_COORD_BITS='18',MHGP11_MODULES='num;index;tower',MHGP11_SANITIZE='ON',MHGP11_TSAN='OFF',MHGP11_POISON='OFF').items():
        need([l.split('=',1)[1] for l in cache if l.startswith(key+':')] == [value],'supplement_'+key)
    q4.command(metas[0],code); q4.command(metas[1],0); need(code == 0,'benchmark_without_matrix')
    manifest,mhash = old.inputs(folder,receipt); report = js(data[BENCH])
    result = benchmark(report,manifest,mhash,sha(data[BASE+'summary.json']),sha(data[SUPPLEMENT+'summary.json']),builds,data)
    q4.command(metas[2],0 if report['conforming'] else 1 if report['complete'] else None)
    success = q4.session(receipt,worker,metas)
    need(success is report['conforming'] and receipt['preserved_failure'] is (not success),'session_verdict')
    return dict(source=SOURCE,coherent=True,campaign_conforming=success,matrix=sum(v[1] for v in counts.values()),
                matrix_selected=sum(v[0] for v in counts.values()),asan18=extra[1],mutants=total_mutants,
                states={s:sum(r['status']==s for r in report['runs']) for s in sorted({r['status'] for r in report['runs']})},**result)


if __name__ == '__main__':
    try: print(json.dumps(check(Path(sys.argv[1]) if len(sys.argv)>1 else HERE/'parallel5'),sort_keys=True))
    except (old.foundation.Refusal,ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError) as error:
        print('REFUS '+str(error)); raise SystemExit(1)
