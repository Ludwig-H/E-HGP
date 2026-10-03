"""LIVE graph2 failure reader: archived first failure, no native execution or WIP import.

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
SOURCE = '245eee1aec7ee9e3e8fd50d227d20ab7554c75db'
CONTRACT = '8a1c69ca278a34438a9058b0558f950e7774f75377945554839568fe443bfdfe'


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
DESCENTS = {'mhgp11_tower_descent_'+n for n in ('interiors','outside','traces','boundaries','refusals',
    'capacity','ownership','concurrency','singleton','singleton_refusals','inventaire')}
VERTICAL = 'mhgp11_tower_vertical_parallel_fault_vertical_census_failure'
FAILED = DESCENTS | {VERTICAL}
COMMANDS = ('000_matrice','001_asan18','002_pair_graph_leaf16','003_pair_graph_leaf8')
PLAN = 'bench/plans/full_pair_graph_g4.json'




def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob)==CONTRACT,'contract_pin')
    value = js(blob); need(value['source']==SOURCE,'source_pin')
    for path,pin in value['files'].items():
        result = subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/'+path],cwd=HERE.parents[3],
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(result.returncode==0 and sha(result.stdout)==pin['sha256'] and len(result.stdout)==pin['bytes'],'git_source_pin')
        for key,file in [('plan',PLAN),('matrix','tools/g4_matrix.json'),('supplement','bench/meb_asan18_matrix.json')]:
            if path==file: same(js(result.stdout),value[key],'source_contract_copy')
        if path.startswith('tests/mutants/') and path.endswith('.json'):
            manifest = js(result.stdout); module = Path(path).stem
            same({m['id']:m.get('attendu','execution') for m in manifest['mutants']},
                 value['mutants'][module],'mutant_manifest_copy')
    return value

def failed_output(data,prefix,name,case):
    source = data[prefix+'LastTest.log'].decode()
    headings = list(re.finditer(r'^\d+/\d+ Testing: ([^\n]+)\n',source,re.MULTILINE))
    hits = [i for i,h in enumerate(headings) if h.group(1)==name]
    need(len(hits)==1,'failed_log_unique'); i = hits[0]
    section = source[headings[i].end():headings[i+1].start() if i+1<len(headings) else len(source)]
    need(len(re.findall(r'^\d+/\d+ Test: '+re.escape(name)+'$',section,re.MULTILINE))==1 and
         section.count('\nTest Failed.\n')==1 and '\nTest Passed.\n' not in section,'failed_log_status')
    marker = '\nOutput:\n----------------------------------------------------------\n'
    need(section.count(marker)==1 and section.count('\n<end of output>\n')==1,'failed_output_boundaries')
    output = section.split(marker,1)[1].split('\n<end of output>\n',1)[0]
    need(output.strip()==(case.findtext('system-out') or '').strip(),'failed_junit_lasttest')
    return output.splitlines()

def transport(folder,pins):
    receipt,worker,data = old.read_capture(folder); raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local','original_receipt_sha256','preserved_failure'}<=set(raw),'receipt_raw_fields')
    need(receipt['commit']==SOURCE and receipt['results_sha256']==pins['archive_sha256'] and
         receipt['plan_sha256']==pins['files'][PLAN]['sha256'] and
         receipt['worker_plan_sha256']==worker['plan_sha256']==sha(data['results/plan.sh']),'capture_identity')
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

def live_sources(folder,receipt):
    session = Path(receipt['raw_receipt_local']).parent
    preflight = js((folder/'preflight.json').read_bytes())
    package = session/'package/package.tar.gz'; archive = session/'results/results.tar.gz'
    need(preflight['package_sha256']==receipt['package_sha256'] and
         package.stat().st_size==preflight['package_bytes'] and old.digest(package)==receipt['package_sha256'],
         'live_source_package')
    need(archive.stat().st_size==receipt['results_bytes'] and old.digest(archive)==receipt['results_sha256'],
         'live_results_archive')
    # The pinned Git pieces must also be the bytes actually carried by the archived source package.
    wanted={'morsehgp3D_v11/'+path:pin for path,pin in _import_pins['files'].items()};seen=set()
    with tarfile.open(package,'r:gz') as packed:
        for member in packed:
            if member.name not in wanted:continue
            need(member.name not in seen and member.isfile(),'package_source_member')
            pin=wanted[member.name];need(member.size==pin['bytes'],'package_source_size')
            need(sha(packed.extractfile(member).read())==pin['sha256'],'package_source_hash');seen.add(member.name)
    need(seen==set(wanted),'package_source_inventory')

def compiler_errors(config,data,prefix):
    options=config['cmake_options'];bits=next((int(x.split('=')[1]) for x in options if x.startswith('-DMHGP11_COORD_BITS=')),21)
    def error(line,name,words):
        value=f"const {name}' {{aka 'const mhgp11::num::Wide<{words}>'}}"
        return f"{line}: error: no match for 'operator==' (operand types are '{value} and '{value})"
    expected=[error('158:44','Numerator',4 if bits==24 else 3)]
    if bits!=18:expected.append(error('159:46','Denominator',3))
    errors=[line for line in data[prefix+'build.log'].decode().splitlines() if 'error:' in line]
    need(len(errors)==len(expected) and all('/tests/tower/descent_test.cpp:' in line for line in errors),
         'compiler_error_locations')
    same([line.split('descent_test.cpp:',1)[1] for line in errors],expected,'exact_compiler_errors')


def gate(config,data,declared):
    name=config['name'];prefix=BASE+name+'/'
    result=old.foundation.judge_config(config,data);proof=first.build(config,data,declared)
    for key in ('threads','compiler','ctest_args','cmake_options'):same(config[key],declared[key],'config_'+key)
    need(config['optional'] is declared.get('optional',False),'config_optional')
    if name in ('style','clang_release'):
        need(config['status']==('ok' if name=='style' else 'absent'),'optional_or_style');return result
    need(config['status']=='build_failed' and config['conforming'] is False and config['not_run']==[],
         'native_build_failure_status')
    provenance=js(data[prefix+'build_provenance.json'])
    need(provenance['complete'] is True and provenance['errors']==[] and
         {'libmhgp11.a','mhgp11_full_bench','mhgp11_tower_vertical_parallel_fault'}<=set(proof) and
         'mhgp11_tower_descent' not in proof,'partial_build_targets')
    steps=old.foundation.unique((s['name'],s) for s in config['steps'])
    need(set(steps)=={'configure','build','list','test'},'step_inventory')
    for key,status,code in [('configure','ok',0),('build','failed',2),('list','ok',0),('test','failed',8)]:
        need(steps[key]['status']==status and type(steps[key]['exit_code']) is int and
             steps[key]['exit_code']==code and steps[key].get('timed_out',False) is False,'executed_steps')
    compiler_errors(config,data,prefix)
    cases=list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    wanted={'mhgp11_mutants_tower'} if name=='mutants' else FAILED
    need({c.get('name') for c in cases if c.find('failure') is not None}==wanted and
         len(config['failures'])==len(wanted) and {f['test'] for f in config['failures']}==wanted,'failed_gate_inventory')
    for case in cases:
        failed=case.get('name') in wanted
        need(case.get('status')==('fail' if failed else 'run') and case.find('skipped') is None,'gate_state')
        if not failed:continue
        lines=failed_output(data,prefix,case.get('name'),case)
        if name=='mutants':
            need('temoin [] : construction en echec' in lines and
                 'TEMOIN ROUGE module=tower : aucun mutant juge' in lines and
                 not any(re.match(r'^\S+\s+(TUE|INVALIDE|SURVIVANT)\b',s) for s in lines),
                 'red_witness_no_tower_mutants')
        elif case.get('name') in DESCENTS:
            need(lines.count('run_expect_verdict lancement_impossible')==1 and
                 any('exec: ' in line and '/build/mhgp11_tower_descent: not found' in line for line in lines) and
                 '  programme impossible a lancer (code 127 du shell : absent, non executable' in lines and
                 'run_expect_verdict code' not in lines,'missing_descent_binary')
        else:
            errors=[s for s in lines if s.startswith('ECHEC ')]
            expected=['ECHEC forest_vertical_parallel_fault.cpp:99: refused.reason == Reason::memory_budget',
                      'ECHEC forest_vertical_parallel_fault.cpp:99: injections.load() == hits+1',
                      'ECHEC forest_vertical_parallel_fault.cpp:100: times == before',
                      'ECHEC forest_vertical_parallel_fault.cpp:100: upper.value().ledger().vertical_descents == 0u']*2
            same(errors,expected,'vertical_exact_failures')
            need(lines.count('      obtenu 0, attendu 6')==2 and lines.count('      obtenu 0, attendu 1')==2 and
                 lines.count('      obtenu 3, attendu 0')==2 and
                 lines.count('test vertical_census_failure controles=38 echecs=8 plancher=30')==1 and
                 lines.count('ECHECS 8')==1 and lines.count('run_expect_verdict code')==1 and
                 '  code de sortie 1, attendu 0' in lines,'vertical_exact_values')
    need(result[2:]==(len(wanted),0),'failure_counts')
    return result


def matrices(data,pins):
    counts={}
    for prefix,key,budget,threads in ((BASE,'matrix',800,48),(SUPP,'supplement',160,12)):
        summary=js(data[prefix+'summary.json']);configs=summary['configurations']
        declared={c['name']:c for c in pins[key]['configurations']};names=[c['name'] for c in configs]
        need(len(names)==len(set(names)) and set(names)==set(declared) and summary['requested']==names and
             summary['schema']=='ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
             summary['conforming'] is False and type(summary['exit_code']) is int and summary['exit_code']==1 and
             summary['signals']==[],'matrix_status')
        same(summary['budget_seconds'],budget,'matrix_budget');same(summary['thread_budget'],threads,'matrix_threads')
        same(summary['statuses'],{c['name']:c['status'] for c in configs},'matrix_statuses')
        need({p[len(prefix):].split('/')[0] for p in data if p.startswith(prefix) and '/' in p[len(prefix):]}==set(names),
             'matrix_directories')
        mapped={BASE+p[len(prefix):]:v for p,v in data.items() if p.startswith(prefix)}
        observed={c['name']:gate(c,mapped,declared[c['name']]) for c in configs}
        if key=='matrix':
            need(all(observed[n][0]==pins['matrix_counts'][n] for n in names),'main_counts');counts=observed
        else:need(observed=={'gcc_asan_ubsan18':(309,297,12,0)},'supplement_counts')
    return counts


def mutants(data,pins):
    prefix=BASE+'mutants/'
    cases={c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    total=construction=0
    for module,entries in pins['mutants'].items():
        name='mhgp11_mutants_'+module;case=cases[name]
        if module=='tower':
            need(case.get('status')=='fail' and case.find('failure') is not None,'red_tower_gate')
            lines=failed_output(data,prefix,name,case)
            need('TEMOIN ROUGE module=tower : aucun mutant juge' in lines and
                 not any(re.match(r'^\S+\s+(TUE|INVALIDE|SURVIVANT)\b',s) for s in lines),'no_tower_mutant_verdict')
            continue
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutant_gate')
        lines=index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        verdicts=[re.fullmatch(r'(\w+)\s+(TUE|INVALIDE|SURVIVANT)\s+(.+)',s) for s in lines]
        verdicts=[v for v in verdicts if v is not None]
        need(len(verdicts)==len(entries) and {v[1] for v in verdicts}==set(entries),'exact_mutant_inventory')
        n=len(entries);built=sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,built,n)) in lines,'mutant_totals')
        for ident,kind in entries.items():
            cause='construction' if kind=='construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutant_cause')
        total+=n;construction+=built
    return total-construction,construction


def commands(receipt,worker,data,pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'command_inventory')
    for name,code,declared in zip(COMMANDS,(1,1,2,2),pins['plan']['commands']):
        prefix='results/cmd/'+name+'/'
        meta=old.fields(data[prefix+'meta.txt']);q4.command(meta,code)
        need(meta['requested_timeout_seconds']==str(declared['timeout_seconds']) and
             meta['streams_truncated']=='0' and meta['residual_group_killed']=='0','command_closure')
        argv=shlex.split(data[prefix+'argv.txt'].decode());root=argv[1].split('/src/morsehgp3D_v11/',1)[0]
        replacements=dict(src=root+'/src',build=root+'/build',data=root+'/data',out=root+'/results/cmd/'+name+'/files')
        same(argv,[a.format(**replacements) for a in declared['argv']],'executed_plan')
    for name in COMMANDS[2:]:
        prefix='results/cmd/'+name+'/'
        need(data[prefix+'stdout']==b'full_parallel_refused: ValueError\n' and data[prefix+'stderr']==b'' and
             not any(p.startswith(prefix+'files/') for p in data),'no_full_benchmark')
    need(worker['commands_total']=='4' and worker['commands_ok']=='0' and worker['status']=='failed' and
         worker['interrupted']=='0' and receipt['status']=='failed_remote' and receipt['worker_exit_code']==1,'session_failure')


def preflight_and_first(folder,pins):
    for file in ('preflight.json','first_compile_failure.txt','first_vertical_failure.txt'):
        pin=pins[file];blob=(folder/file).read_bytes()
        need(len(blob)==pin['bytes'] and sha(blob)==pin['sha256'] and
             Path(pin['raw_path']).read_bytes()==blob,'raw_auxiliary_copy')
    preflight=js((folder/'preflight.json').read_bytes())
    need(preflight['commit']==SOURCE and preflight['gcp_mutations']=='none' and
         preflight['plan_sha256']==pins['files'][PLAN]['sha256'],'preflight_identity')
    same(preflight['budget'],dict(build_and_setup_seconds=120,closing_reserve_seconds=660,
         command_timeouts_sum_seconds=2170,guest_seconds=3300,oversubscribed=False,
         ssh_cutoff_before_guest_seconds=360,upload_estimate_seconds=184,worker_window_seconds=2336),'original_budget')
    need('warnings' not in preflight,'no_preflight_warning')
    normalized={k:v for k,v in pins['plan'].items() if k not in ('note','schema')}
    normalized.update(build_targets=[],build_timeout_seconds=600);same(preflight['plan'],normalized,'preflight_commands')
    cmds=pins['plan']['commands']
    need([c['timeout_seconds'] for c in cmds]==[850,180,570,570] and 2170+120<=2336,'declared_budget')
    for cmd,leaf in zip(cmds[2:],('16','8')):
        argv=cmd['argv'];need('--pair-graph' in argv and argv[argv.index('--leaf-size')+1]==leaf and
                             argv[argv.index('--budget-seconds')+1]=='500','two_declared_calendars')
    need("error: no match for 'operator=='" in (folder/'first_compile_failure.txt').read_text() and
         'test vertical_census_failure controles=38 echecs=8 plancher=30' in
         (folder/'first_vertical_failure.txt').read_text(),'first_failures_preserved')


def declared_schedule(pins):
    import ast
    from types import SimpleNamespace
    source=subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/bench/full_parallel.py'],cwd=HERE.parents[3],
                          stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
    pin=pins['files']['bench/full_parallel.py']
    need(source.returncode==0 and sha(source.stdout)==pin['sha256'] and len(source.stdout)==pin['bytes'],'schedule_source')
    nodes=[n for n in ast.parse(source.stdout).body if isinstance(n,ast.FunctionDef) and n.name=='schedule']
    need(len(nodes)==1,'schedule_function')
    context=dict(need=need,profiles=SimpleNamespace(COUNTS=old.CASE_COUNTS))
    exec(compile(ast.Module(body=nodes,type_ignores=[]),'<pinned schedule>','exec'),context)
    actual=context['schedule'](pair_graph=True)
    def row(case,bits,mode,workers=48):
        return dict(case=case,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected=[row(case,bits,mode) for case in ('lidar_ng00','lidar_ng01','lidar_ng02')
              for bits in (21,24) for mode in (2047,4095)]
    expected += [row('lidar_ng00',21,4095,w) for w in (1,8)]
    expected += [row('uniform_u18_n'+str(n),21,mode) for n in (8000,16000,32000) for mode in (2047,4095)]
    same(actual,expected,'unstarted_schedule');need(len(actual)==20,'calendar_units')
    return 2*len(actual)


def source_causes(pins):
    texts={}
    for path in ('tests/tower/descent_test.cpp','tests/tower/forest_vertical_parallel_fault.cpp','src/tower/descent.cpp'):
        child=subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/'+path],cwd=HERE.parents[3],capture_output=True)
        need(child.returncode==0 and sha(child.stdout)==pins['files'][path]['sha256'],'causal_source_pin')
        texts[path]=child.stdout.decode()
    lines=texts['tests/tower/descent_test.cpp'].splitlines()
    need('level().numerator() == reference.value().sphere().level().numerator()' in lines[157] and
         'level().denominator() == reference.value().sphere().level().denominator()' in lines[158],'wide_equality_source')
    fault=texts['tests/tower/forest_vertical_parallel_fault.cpp'];product=texts['src/tower/descent.cpp']
    need('auto lower = build_forest(domain.value(),1,work)' in fault and
         'auto upper = build_forest(domain.value(),2,work)' in fault and
         'fail_at.store(calls.load()+4)' in fault,'old_q1_allocation_fixture')
    start=product.index('if (k == 1) {');end=product.index('  StepQuery query',start)
    singleton=product[start:end]
    need('auto meb = bounded_meb(domain.index().cloud(), part);' in singleton and
         'return DescentBuilder::singleton(meb.value());' in singleton and
         'visit_located_part' not in singleton.split('\n',2)[-1] and 'census(' not in singleton,'allocation_free_singleton_source')


def check(folder=HERE):
    pins=source_contract();receipt,worker,data=transport(folder,pins)
    counts=matrices(data,pins);commands(receipt,worker,data,pins);executed,built=mutants(data,pins)
    preflight_and_first(folder,pins);live_sources(folder,receipt);source_causes(pins)
    units=declared_schedule(pins)
    return dict(coherent=True,conforming=False,source=SOURCE,
        matrix_selected=sum(c[0] for c in counts.values()),matrix_passed=sum(c[1] for c in counts.values()),
        matrix_failed=sum(c[2] for c in counts.values()),matrix_no_closed_result=0,
        asan18_selected=309,asan18_passed=297,asan18_failed=12,build_failures=8,
        unavailable_descent_gate_executions=77,vertical_fault_gate_executions_failed=7,
        non_tower_mutants_code_or_line=executed,non_tower_mutants_expected_construction=built,
        tower_mutants_judged=0,full_attempts=0,semantic_reuse_attempts=0,full_measurements=0,
        unpersisted=0,unstarted_declared_units=units,preflight_oversubscribed=False,
        declared_seconds_with_setup=2290,worker_window_seconds=2336)


if __name__=='__main__':
    try:print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
