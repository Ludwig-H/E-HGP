"""LIVE combined1 failure reader: archived first failure, no native execution or WIP import.

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
SOURCE = '06d7c44b4d4a52a80a0c00f0fd926b48a92d463c'
CONTRACT = '3eb8a287ae7ebb4da5c0cfa416f24cc32e20df6002a68cd149b072bb42fb9370'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


import hashlib
_contract_blob = (HERE/'source_contract.json').read_bytes()
if hashlib.sha256(_contract_blob).hexdigest()!=CONTRACT:
    raise ValueError('contract_pin')
_import_pins = json.loads(_contract_blob)
for _path in _import_pins['helpers']:
    _blob = (HERE.parents[2]/_path).read_bytes()
    _pin = _import_pins['files'][_path]
    if len(_blob)!=_pin['bytes'] or hashlib.sha256(_blob).hexdigest()!=_pin['sha256']:
        raise ValueError('historical_reader_pin')


first = load('forest_failure_transport',HERE.parent.parent/'catalogue_adaptive_20261002/adaptive1_failure/check.py')
index = load('forest_failure_logs',HERE.parent.parent/'index_20261002/check.py')
q4,old = first.q4,first.old
need,js,sha,same,BASE,SUPP = first.need,first.js,first.sha,first.same,first.BASE,first.SUPP
REFUSALS = (old.foundation.Refusal,index.old.foundation.Refusal)
FAILED = {'mhgp11_tower_full_bench_io'+suffix for suffix in ('', '_opt')} | {
    'mhgp11_tower_full_catalogue_collector'+suffix for suffix in ('', '_opt')}
COMMANDS = ('000_matrice','001_asan18','002_combined_full')
PLAN = 'bench/plans/full_combined_g4.json'
DIAGNOSTIC = "full_probe.cpp:142:58: error: 'const class mhgp11::Catalogue' has no member named 'incidences'"


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob)==CONTRACT,'contract_pin')
    value = js(blob); need(value['source']==SOURCE,'source_pin')
    for path,pin in value['files'].items():
        result = subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/'+path],cwd=HERE.parents[3],
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(result.returncode==0 and sha(result.stdout)==pin['sha256'] and len(result.stdout)==pin['bytes'],'git_source_pin')
        for key,file in [('plan',PLAN),('matrix','tools/g4_matrix.json'),('supplement','bench/meb_asan18_matrix.json')]:
            if path==file: same(js(result.stdout),value[key],'source_contract_copy')
        if path=='bench/full_probe.cpp':
            need(b'catalogue.incidences()' in result.stdout.splitlines()[141],'failing_source_line')
        if path=='tests/tower/full_catalogue_collector_test.py':
            need(b"sum(row['catalogue_stage_ms'].values()) == row['stage_ms']['domain']" in
                 result.stdout.splitlines()[130],'float_sum_source_line')
    return value


def gate_failure(config,data,declared):
    name = config['name']; prefix = BASE+name+'/'
    result = old.foundation.judge_config(config,data); proof = first.build(config,data,declared)
    for key in ('threads','compiler','ctest_args','cmake_options'): same(config[key],declared[key],'config_'+key)
    need(config['optional'] is declared.get('optional',False),'config_optional')
    if name in ('style','clang_release'):
        need(config['status']==('ok' if name=='style' else 'absent'),'optional_or_style'); return result
    need(config['status']=='build_failed' and config['conforming'] is False and config['not_run']==[],
         'native_build_failure_status')
    provenance = js(data[prefix+'build_provenance.json'])
    need(provenance['complete'] is True and provenance['errors']==[] and
         {'libmhgp11.a','mhgp11_tower_forest_parallel','mhgp11_tower_forest_parallel_fault',
          'mhgp11_tower_forest_parallel_probe'}<=set(proof) and 'mhgp11_full_bench' not in proof,'partial_build_targets')
    steps = old.foundation.unique((s['name'],s) for s in config['steps'])
    need(set(steps)=={'configure','build','list','test'},'step_inventory')
    for key,status,code in [('configure','ok',0),('build','failed',2),('list','ok',0),('test','failed',8)]:
        need(steps[key]['status']==status and type(steps[key]['exit_code']) is int and steps[key]['exit_code']==code and
             steps[key].get('timed_out',False) is False,'executed_steps')
    errors = [line for line in data[prefix+'build.log'].decode().splitlines() if 'error:' in line]
    need(len(errors)==1 and errors[0].endswith(DIAGNOSTIC),'exact_compiler_error')
    cases = list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    expected = {'mhgp11_mutants_tower'} if name=='mutants' else FAILED
    need({c.get('name') for c in cases if c.find('failure') is not None}==expected and
         len(config['failures'])==len(expected) and {r['test'] for r in config['failures']}==expected,
         'failed_gate_inventory')
    for case in cases:
        failed = case.get('name') in expected
        need(case.get('status')==('fail' if failed else 'run') and case.find('skipped') is None,'gate_state')
        if not failed: continue
        output = case.findtext('system-out') or ''
        need('run_expect_verdict code' in output.splitlines(),'python_gate_executed')
        if name=='mutants':
            need('temoin [] : construction en echec' in output.splitlines() and
                 'TEMOIN ROUGE module=tower : aucun mutant juge' in output.splitlines() and
                 re.search(r'^\S+\s+TUE\b',output,re.MULTILINE) is None,'red_witness_no_tower_mutants')
        elif 'full_bench_io' in case.get('name'):
            need("FileNotFoundError: [Errno 2] No such file or directory:" in output and
                 "/build/mhgp11_full_bench'" in output and 'full_bench_io.py' in output,
                 'missing_benchmark_binary')
        else:
            need('ValueError: disjoint boundary admitted' in output.splitlines() and
                 "sum(row['catalogue_stage_ms'].values()) == row['stage_ms']['domain']" in output and
                 'full_catalogue_collector_test.py", line 131' in output,'float_sum_assertion')
    need(result[2:]==(len(expected),0),'failure_counts')
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
        else: need(observed=={'gcc_asan_ubsan18':(209,205,4,0)},'supplement_counts')
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
    prefix = 'results/cmd/002_combined_full/'
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


def local_preflight(folder,pins):
    blob = (folder/'combined2_local_preflight.json').read_bytes()
    need(sha(blob)==pins['later_local_preflight']['sha256'] and
         len(blob)==pins['later_local_preflight']['bytes'],'local_preflight_pin')
    row = js(blob)
    need(row['status']=='failed_before_start' and row['gcp_mutations']=='none' and
         row['commit']=='90dd48bd284a960ed687328e1fe2468947039069' and
         'espace local insuffisant' in row['reason'],'later_local_preflight_scope')


def check(folder=HERE):
    pins = source_contract(); receipt,worker,data = transport(folder,pins)
    counts = matrices(data,pins); commands(receipt,worker,data,pins); executed,built = other_mutants(data,pins)
    local_preflight(folder,pins)
    return dict(coherent=True,conforming=False,source=SOURCE,
        matrix_selected=sum(c[0] for c in counts.values()),matrix_passed=sum(c[1] for c in counts.values()),
        matrix_failed=sum(c[2] for c in counts.values()),matrix_no_closed_result=0,
        asan18_selected=209,asan18_passed=205,asan18_failed=4,
        build_failures=8,failing_python_gates=sorted(FAILED),failed_python_gate_executions=28,
        completed_mutation_campaign_gates=6,non_tower_mutants_code_or_line=executed,
        non_tower_mutants_expected_construction=built,tower_mutants_judged=0,
        full_attempts=0,semantic_reuse_attempts=0,full_measurements=0,
        unpersisted=0,unstarted_declared_units=27,later_combined2_status='failed_before_start')


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,old.foundation.ET.ParseError,tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
