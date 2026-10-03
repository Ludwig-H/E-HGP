"""LIVE forest1 failure reader: archived first failure, no native execution or WIP import.

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
SOURCE = 'e5f6a5683237005117a6b9f2c23bdc7eda6e3caa'
CONTRACT = 'a5e0fead105cc8eb192177c5e3607929060318571150beafaf3609b7d21653d9'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


first = load('forest_failure_transport',HERE.parent.parent/'catalogue_adaptive_20261002/adaptive1_failure/check.py')
index = load('forest_failure_logs',HERE.parent.parent/'index_20261002/check.py')
q4,old = first.q4,first.old
need,js,sha,same,BASE,SUPP = first.need,first.js,first.sha,first.same,first.BASE,first.SUPP
REFUSALS = (old.foundation.Refusal,index.old.foundation.Refusal)
FAILED = 'mhgp11_tower_forest_parallel_plateaus'
COMMANDS = ('000_matrice','001_asan18','002_parallel_full')
PLAN = 'bench/plans/full_parallel_g4.json'
DIAGNOSTIC = 'ECHEC forest_parallel_test.cpp:74: times.orders[1].extended_cells == 1u'


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob)==CONTRACT,'contract_pin')
    value = js(blob); need(value['source']==SOURCE,'source_pin')
    for path,pin in value['files'].items():
        result = subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/'+path],cwd=HERE.parents[3],
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(result.returncode==0 and sha(result.stdout)==pin['sha256'] and len(result.stdout)==pin['bytes'],'git_source_pin')
        for key,file in [('plan',PLAN),('matrix','tools/g4_matrix.json'),('supplement','bench/meb_asan18_matrix.json')]:
            if path==file: same(js(result.stdout),value[key],'source_contract_copy')
        if path=='tests/tower/forest_parallel_test.cpp':
            need(result.stdout.splitlines()[73].strip()==
                 b'CHECK_EQ(second.nodes().size(), 3u); CHECK_EQ(times.orders[1].extended_cells, 1u);','failing_source_line')
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
            need(output.splitlines().count(DIAGNOSTIC)==3 and output.splitlines().count('      obtenu 0, attendu 1')==3 and
                 'test plateaus controles=49 echecs=3 plancher=45' in output.splitlines() and
                 'ECHECS 3' in output.splitlines() and 'mhgp11_test_ok' not in output,'exact_failed_assertions')
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
        else: need(observed=={'gcc_asan_ubsan18':(200,199,1,0)},'supplement_counts')
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
    prefix = 'results/cmd/002_parallel_full/'
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
    members = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,name = line.split('  ',1); name = 'results/'+name.removeprefix('./')
        need(name not in members and name in data and sha(data[name])==digest,'manifest_hash'); members.add(name)
    need(members==set(data)-{'results/MANIFEST.sha256'},'manifest_inventory')
    for file,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json')]:
        need((folder/file).read_bytes()==data[path],'compact_copy')
    old.inputs(folder,receipt)
    return receipt,worker,data


def check(folder=HERE):
    pins = source_contract(); receipt,worker,data = transport(folder,pins)
    counts = matrices(data,pins); commands(receipt,worker,data,pins); executed,built = other_mutants(data,pins)
    return dict(coherent=True,conforming=False,source=SOURCE,
        matrix_selected=sum(c[0] for c in counts.values()),matrix_passed=sum(c[1] for c in counts.values()),
        matrix_failed=sum(c[2] for c in counts.values()),matrix_no_closed_result=0,
        asan18_selected=200,asan18_passed=199,asan18_failed=1,
        failing_native_gate=FAILED,native_gate_failures=7,wrong_assertions_per_native_gate=3,
        completed_mutation_campaign_gates=6,non_tower_mutants_code_or_line=executed,
        non_tower_mutants_expected_construction=built,tower_mutants_judged=0,
        full_attempts=0,semantic_reuse_attempts=0,full_measurements=0)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,old.foundation.ET.ParseError,tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
