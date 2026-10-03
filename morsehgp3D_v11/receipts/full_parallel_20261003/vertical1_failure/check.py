"""LIVE vertical1 failure reader: archived first failure, no native execution or WIP import.

Code0 means coherent failed evidence. The original local receipt and pinned Git
objects remain mandatory; no geometric/performance qualification is inferred.
"""
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = '5e39d2726041684e9499bf1ba05f37f31f2f39a5'
CONTRACT = '399f7062823409cfa80dd56ae481f30d5c29e2281ebf70666a51b224ad493fc8'


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
FAILED = 'mhgp11_tower_census_reuse_descents'
COMMANDS = ('000_matrice','001_asan18','002_vertical_full')
PLAN = 'bench/plans/full_vertical_g4.json'
DIAGNOSTIC = 'ECHEC census_reuse_test.cpp:35: saturated > 0'


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob)==CONTRACT,'contract_pin')
    value = js(blob); need(value['source']==SOURCE,'source_pin')
    for path,pin in value['files'].items():
        result = subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/'+path],cwd=HERE.parents[3],
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(result.returncode==0 and sha(result.stdout)==pin['sha256'] and len(result.stdout)==pin['bytes'],'git_source_pin')
        for key,file in [('plan',PLAN),('matrix','tools/g4_matrix.json'),('supplement','bench/meb_asan18_matrix.json')]:
            if path==file: same(js(result.stdout),value[key],'source_contract_copy')
        if path=='tests/tower/census_reuse_test.cpp':
            need(b'CHECK(saturated > 0);' in result.stdout.splitlines()[34],'failing_source_line')
    return value


def gate_failure(config,data,declared):
    name = config['name']; prefix = BASE+name+'/'
    result = old.foundation.judge_config(config,data); proof = first.build(config,data,declared)
    for key in ('threads','compiler','ctest_args','cmake_options'): same(config[key],declared[key],'config_'+key)
    need(config['optional'] is declared.get('optional',False),'config_optional')
    if name in ('style','clang_release'):
        need(config['status']==('ok' if name=='style' else 'absent'),'optional_or_style'); return result
    need(config['status']=='failed' and config['conforming'] is False and config['not_run']==[],'native_failure_status')
    provenance = js(data[prefix+'build_provenance.json'])
    need(provenance['complete'] is True and provenance['errors']==[] and
         {'libmhgp11.a','mhgp11_full_bench','mhgp11_tower_forest_parallel','mhgp11_tower_forest_parallel_fault',
          'mhgp11_tower_forest_parallel_probe'}<=set(proof),'compiled_targets')
    steps = old.foundation.unique((s['name'],s) for s in config['steps'])
    need(set(steps)=={'configure','build','list','test'},'step_inventory')
    for key,status,code in [('configure','ok',0),('build','ok',0),('list','ok',0),('test','failed',8)]:
        need(steps[key]['status']==status and type(steps[key]['exit_code']) is int and steps[key]['exit_code']==code and
             steps[key].get('timed_out',False) is False,'executed_steps')
    need('error:' not in data[prefix+'build.log'].decode(),'no_compiler_error')
    cases = list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    expected = 'mhgp11_mutants_tower' if name=='mutants' else FAILED
    need({c.get('name') for c in cases if c.find('failure') is not None}=={expected} and
         len(config['failures'])==1 and config['failures'][0]['test']==expected,'failed_gate_inventory')
    for case in cases:
        failed = case.get('name')==expected
        need(case.get('status')==('fail' if failed else 'run') and case.find('skipped') is None,'gate_state')
        if not failed: continue
        output = case.findtext('system-out') or ''
        need('run_expect_verdict code' in output.splitlines() and 'lancement_impossible' not in output,'executed_not_launch_failure')
        if name=='mutants':
            need('temoin [] : porte '+FAILED+' echec sans mutation (executee et passee attendue)' in output and
                 'TEMOIN ROUGE module=tower : aucun mutant juge' in output and DIAGNOSTIC in output and
                 re.search(r'^\S+\s+TUE\b',output,re.MULTILINE) is None,'red_witness_no_tower_mutants')
        else:
            need(output.splitlines().count(DIAGNOSTIC)==1 and
                 'test descents controles=1338 echecs=1 plancher=500' in output.splitlines() and
                 'ECHECS 1' in output.splitlines() and 'mhgp11_test_ok' not in output,'exact_failed_assertion')
    need(result[2:]==(1,0),'failure_counts')
    return result


def matrices(data,pins):
    counts = {}
    for prefix,key,budget,threads in ((BASE,'matrix',800,48),(SUPP,'supplement',160,12)):
        summary = js(data[prefix+'summary.json']); configs = summary['configurations']
        declared = {c['name']:c for c in pins[key]['configurations']}; names = [c['name'] for c in configs]
        need(len(names)==len(set(names)) and set(names)==set(declared) and summary['requested']==names and
             summary['schema']=='ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
             summary['conforming'] is False and type(summary['exit_code']) is int and summary['exit_code']==1 and
             summary['signals']==[],'matrix_status')
        same(summary['budget_seconds'],budget,'matrix_budget'); same(summary['thread_budget'],threads,'matrix_threads')
        same(summary['statuses'],{c['name']:c['status'] for c in configs},'matrix_statuses')
        need({p[len(prefix):].split('/')[0] for p in data if p.startswith(prefix) and '/' in p[len(prefix):]}==set(names),
             'matrix_directories')
        mapped = {BASE+p[len(prefix):]:v for p,v in data.items() if p.startswith(prefix)}
        observed = {c['name']:gate_failure(c,mapped,declared[c['name']]) for c in configs}
        if key=='matrix':
            need(all(observed[n][0]==pins['matrix_counts'][n] for n in names),'main_counts'); counts = observed
        else: need(observed=={'gcc_asan_ubsan18':(255,254,1,0)},'supplement_counts')
    return counts


def other_mutants(data,pins):
    prefix = BASE+'mutants/'; cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    total,construction = 0,0
    for module,entries in pins['mutants'].items():
        if module=='tower': continue
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutation_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        n = len(entries); built = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,built,n)) in lines,'mutation_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind=='construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutation_cause')
        total += n; construction += built
    return total-construction,construction


def commands(receipt,worker,data,pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'command_inventory')
    for name,code,declared in zip(COMMANDS,(1,1,2),pins['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = old.fields(data[prefix+'meta.txt']); q4.command(meta,code)
        need(meta['requested_timeout_seconds']==str(declared['timeout_seconds']) and
             meta['streams_truncated']=='0' and meta['residual_group_killed']=='0','command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode()); root = argv[1].split('/src/morsehgp3D_v11/',1)[0]
        replacements = dict(src=root+'/src',build=root+'/build',data=root+'/data',out=root+'/results/cmd/'+name+'/files')
        same(argv,[a.format(**replacements) for a in declared['argv']],'executed_plan')
    prefix = 'results/cmd/002_vertical_full/'
    need(data[prefix+'stdout']==b'full_parallel_refused: ValueError\n' and data[prefix+'stderr']==b'' and
         not any(p.startswith(prefix+'files/') for p in data),'no_full_benchmark')
    need(worker['commands_total']=='3' and worker['commands_ok']=='0' and worker['status']=='failed' and
         worker['interrupted']=='0' and receipt['status']=='failed_remote' and receipt['worker_exit_code']==1,'session_failure')


def transport(folder,pins):
    receipt,worker,data = old.read_capture(folder); raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local','original_receipt_sha256','preserved_failure'}<=set(raw),'receipt_raw_fields')
    need(receipt['commit']==SOURCE and receipt['results_sha256']==pins['archive_sha256'] and
         receipt['plan_sha256']==pins['files'][PLAN]['sha256'] and
         receipt['worker_plan_sha256']==worker['plan_sha256'],'capture_identity')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()=='3' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')) and
         receipt['warnings']==[] and receipt['preserved_failure'] is True and receipt['results_skipped_members']==[],
         'closed_failure')
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
    for file,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json')]:
        need((folder/file).read_bytes()==data[path],'compact_copy')
    old.inputs(folder,receipt)
    return receipt,worker,data


def preflight_and_first(folder,pins):
    for file in ('preflight.json','first_failure.txt'):
        pin = pins[file]; blob = (folder/file).read_bytes()
        need(len(blob)==pin['bytes'] and sha(blob)==pin['sha256'] and
             Path(pin['raw_path']).read_bytes()==blob,'raw_auxiliary_copy')
    preflight = js((folder/'preflight.json').read_bytes())
    need(preflight['commit']==SOURCE and preflight['gcp_mutations']=='none' and
         preflight['plan_sha256']==pins['files'][PLAN]['sha256'],'preflight_identity')
    budget = preflight['budget']
    same(budget,dict(build_and_setup_seconds=120,closing_reserve_seconds=660,
         command_timeouts_sum_seconds=1680,guest_seconds=2700,oversubscribed=True,
         ssh_cutoff_before_guest_seconds=360,upload_estimate_seconds=183,worker_window_seconds=1737),'original_budget')
    same(preflight['warnings'],["somme des delais > fenetre utile : les dernieres commandes seront coupees ou sautees a l'echeance"],
         'preflight_warning_preserved')
    same(preflight['plan']['commands'],pins['plan']['commands'],'preflight_commands')
    need(sum(c['timeout_seconds'] for c in pins['plan']['commands'])==1680 and 1680+120>1737,
         'oversubscribed_with_setup')
    pin = pins['future_plan']; blob = (folder/'future_plan_95d.json').read_bytes()
    future = subprocess.run(['git','show',pin['source']+':morsehgp3D_v11/'+pin['path']],cwd=HERE.parents[3],
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
    need(future.returncode==0 and future.stdout==blob and len(blob)==pin['bytes'] and sha(blob)==pin['sha256'],
         'future_plan_source')
    value = js(blob); commands = value['commands']
    need([c['timeout_seconds'] for c in commands]==[850,180,570] and sum(c['timeout_seconds'] for c in commands)+120==1720 and
         1720<=1737 and commands[2]['argv'][commands[2]['argv'].index('--budget-seconds')+1]=='500','future_plan_budget')
    changed = dict(pins['plan'],note=value['note'],commands=[dict(c) for c in pins['plan']['commands']])
    changed['commands'][2]=dict(changed['commands'][2],timeout_seconds=570,argv=list(changed['commands'][2]['argv']))
    argv = changed['commands'][2]['argv']; argv[argv.index('--budget-seconds')+1]='500'
    same(value,changed,'future_plan_only_budget_and_note')


def check(folder=HERE):
    pins = source_contract(); receipt,worker,data = transport(folder,pins)
    counts = matrices(data,pins); commands(receipt,worker,data,pins); executed,built = other_mutants(data,pins)
    preflight_and_first(folder,pins)
    return dict(coherent=True,conforming=False,source=SOURCE,
        matrix_selected=sum(c[0] for c in counts.values()),matrix_passed=sum(c[1] for c in counts.values()),
        matrix_failed=sum(c[2] for c in counts.values()),matrix_no_closed_result=0,
        asan18_selected=255,asan18_passed=254,asan18_failed=1,
        failing_native_gate=FAILED,native_gate_failures=7,wrong_assertions_per_native_gate=1,
        successful_builds=8,completed_mutation_campaign_gates=6,non_tower_mutants_code_or_line=executed,
        non_tower_mutants_expected_construction=built,tower_mutants_judged=0,
        full_attempts=0,semantic_reuse_attempts=0,full_measurements=0,unpersisted=0,unstarted_declared_units=20,
        original_preflight_oversubscribed=True,original_declared_seconds_with_setup=1800,
        original_worker_window_seconds=1737,future_plan_declared_seconds_with_setup=1720)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,old.foundation.ET.ParseError,tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
