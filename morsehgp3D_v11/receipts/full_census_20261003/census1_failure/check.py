"""LIVE census1 failure reader: archived first failure, no native execution or WIP import.

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
SOURCE = 'c6954f231bf726d37f399fc9298f6a4309033e8e'
CONTRACT = 'edf72686c3f0b60dee9f268679c17ca2c0dbfe51fbe3c2e8bf618a6d023ba338'


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
FAILED = 'mhgp11_mutants_tower'
INVALID = 'forest_cohort_nonbirth_reset'
COMMANDS = ('000_matrice','001_asan18','002_census_full')
PLAN = 'bench/plans/full_census_g4.json'
DIAGNOSTIC = '[-Werror=maybe-uninitialized]'


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
            if module=='tower':
                same(next(m for m in manifest['mutants'] if m['id']==INVALID),value['invalid_mutant'],'invalid_source_copy')
                need('rank.reset(); count = 0;' in value['invalid_mutant']['remplace'] and
                     value['mutants']['tower'][INVALID]=='execution','invalid_is_not_expected_construction')
    return value


def gate(config,data,declared):
    name = config['name']; prefix = BASE+name+'/'
    result = old.foundation.judge_config(config,data); proof = first.build(config,data,declared)
    for key in ('threads','compiler','ctest_args','cmake_options'): same(config[key],declared[key],'config_'+key)
    need(config['optional'] is declared.get('optional',False),'config_optional')
    expected = 'failed' if name=='mutants' else 'absent' if name=='clang_release' else 'ok'
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
        failed = name=='mutants' and key=='test'
        need(steps[key]['status']==('failed' if failed else 'ok') and type(steps[key]['exit_code']) is int and
             steps[key]['exit_code']==(8 if failed else 0) and steps[key].get('timed_out',False) is False,'executed_steps')
    need('error:' not in data[prefix+'build.log'].decode(),'no_base_compiler_error')
    cases = list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    wanted = {FAILED} if name=='mutants' else set()
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
             summary['conforming'] is (key=='supplement') and type(summary['exit_code']) is int and
             summary['exit_code']==(0 if key=='supplement' else 1) and
             summary['signals']==[],'matrix_status')
        same(summary['budget_seconds'],budget,'matrix_budget'); same(summary['thread_budget'],threads,'matrix_threads')
        same(summary['statuses'],{c['name']:c['status'] for c in configs},'matrix_statuses')
        need({p[len(prefix):].split('/')[0] for p in data if p.startswith(prefix) and '/' in p[len(prefix):]}==set(names),
             'matrix_directories')
        mapped = {BASE+p[len(prefix):]:v for p,v in data.items() if p.startswith(prefix)}
        observed = {c['name']:gate(c,mapped,declared[c['name']]) for c in configs}
        if key=='matrix':
            need(all(observed[n][0]==pins['matrix_counts'][n] for n in names),'main_counts'); counts = observed
        else: need(observed=={'gcc_asan_ubsan18':(264,264,0,0)},'supplement_counts')
    return counts


def other_mutants(data,pins):
    prefix = BASE+'mutants/'; cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
    total,construction = 0,0
    for module,entries in pins['mutants'].items():
        if module=='tower': continue
        name = 'mhgp11_mutants_'+module; case = cases[name]
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutation_gate')
        lines = index.full_test_output(data,prefix,name,case.findtext('system-out') or '')
        verdicts = [re.fullmatch(r'(\w+)\s+(TUE|INVALIDE|SURVIVANT)\s+(.+)',s) for s in lines]
        verdicts = [v for v in verdicts if v is not None]
        need(len(verdicts)==len(entries) and {v[1] for v in verdicts}==set(entries),'other_exact_mutant_inventory')
        n = len(entries); built = sum(v=='construction' for v in entries.values())
        need(('mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' %
              (module,n,n,built,n)) in lines,'mutation_totals')
        for ident,kind in entries.items():
            cause = 'construction' if kind=='construction' else '(code|ligne)'
            need(sum(bool(re.fullmatch(re.escape(ident)+r'\s+TUE\s+'+cause,s)) for s in lines)==1,'mutation_cause')
        total += n; construction += built
    return total-construction,construction


def failed_output(data,case):
    prefix = BASE+'mutants/'; source = data[prefix+'LastTest.log'].decode()
    headings = list(re.finditer(r'^\d+/\d+ Testing: ([^\n]+)\n',source,re.MULTILINE))
    hits = [i for i,h in enumerate(headings) if h.group(1)==FAILED]
    need(len(hits)==1,'failed_log_unique'); i = hits[0]
    section = source[headings[i].end():headings[i+1].start() if i+1<len(headings) else len(source)]
    need(len(re.findall(r'^\d+/\d+ Test: '+FAILED+'$',section,re.MULTILINE))==1 and
         section.count('\nTest Failed.\n')==1 and '\nTest Passed.\n' not in section,'failed_log_status')
    marker = '\nOutput:\n----------------------------------------------------------\n'
    need(section.count(marker)==1 and section.count('\n<end of output>\n')==1,'failed_output_boundaries')
    output = section.split(marker,1)[1].split('\n<end of output>\n',1)[0]
    # This failed gate's JUnit contains the whole output; no truncation accommodation is needed.
    need(output.strip()==(case.findtext('system-out') or '').strip(),'failed_junit_lasttest')
    return output.splitlines()


def tower_mutants(data,pins):
    cases = [c for c in old.foundation.ET.fromstring(data[BASE+'mutants/junit.xml']).iter('testcase')
             if c.get('name')==FAILED]
    need(len(cases)==1 and cases[0].get('status')=='fail' and cases[0].find('failure') is not None and
         cases[0].find('skipped') is None,'tower_mutation_failed_gate')
    lines = failed_output(data,cases[0]); entries = pins['mutants']['tower']
    need(len(entries)==88 and all(v=='execution' for v in entries.values()),'tower_manifest_expectations')
    verdicts = []
    for line in lines:
        match = re.fullmatch(r'(\w+)\s+(TUE|INVALIDE|SURVIVANT)\s+(.+)',line)
        if match: verdicts.append(match.groups())
    need(len(verdicts)==88 and len({v[0] for v in verdicts})==88 and
         {v[0] for v in verdicts}==set(entries),'tower_exact_mutant_inventory')
    for ident,verdict,reason in verdicts:
        if ident==INVALID:
            need(verdict=='INVALIDE' and reason=='le mutant ne passe pas la construction : il doit etre tue par sa porte',
                 'invalid_not_a_causal_death')
        else: need(verdict=='TUE' and reason=='code','tower_causal_death')
    need(lines.count('INVALIDES module=tower : 1 sur 88')==1 and lines.count('run_expect_verdict code')==1 and
         any('code de sortie 3, attendu 0' in line for line in lines) and
         not any('TEMOIN ROUGE' in line or 'mutants_ok module=tower' in line for line in lines),'tower_failure_summary')
    invalid_pos = next(i for i,line in enumerate(lines) if re.match(INVALID+r'\s+INVALIDE',line))
    end = next(i for i in range(invalid_pos+1,len(lines)) if re.fullmatch(r'\w+\s+TUE\s+code',lines[i]))
    diagnostic = lines[invalid_pos+1:end]
    need(sum(INVALID+'/src/src/tower/forest_build.cpp:63:15: error:' in line and DIAGNOSTIC in line
             for line in diagnostic)==1 and any('59 |   std::optional<LevelRank> rank;' in line for line in diagnostic) and
         any('63 |     if (!rank || *rank != next)' in line for line in diagnostic) and
         any('cc1plus: all warnings being treated as errors' in line for line in diagnostic),'invalid_compile_diagnostic')
    return 87


def commands(receipt,worker,data,pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'command_inventory')
    for name,code,declared in zip(COMMANDS,(1,0,2),pins['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = old.fields(data[prefix+'meta.txt']); q4.command(meta,code)
        need(meta['requested_timeout_seconds']==str(declared['timeout_seconds']) and
             meta['streams_truncated']=='0' and meta['residual_group_killed']=='0','command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode()); root = argv[1].split('/src/morsehgp3D_v11/',1)[0]
        replacements = dict(src=root+'/src',build=root+'/build',data=root+'/data',out=root+'/results/cmd/'+name+'/files')
        same(argv,[a.format(**replacements) for a in declared['argv']],'executed_plan')
    prefix = 'results/cmd/002_census_full/'
    need(data[prefix+'stdout']==b'full_parallel_refused: ValueError\n' and data[prefix+'stderr']==b'' and
         not any(p.startswith(prefix+'files/') for p in data),'no_full_benchmark')
    need(worker['commands_total']=='3' and worker['commands_ok']=='1' and worker['status']=='failed' and
         worker['interrupted']=='0' and receipt['status']=='failed_remote' and receipt['worker_exit_code']==1,'session_failure')


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


def preflight_and_first(folder,pins):
    for file in ('preflight.json','first_failure.txt'):
        pin = pins[file]; blob = (folder/file).read_bytes()
        need(len(blob)==pin['bytes'] and sha(blob)==pin['sha256'] and
             Path(pin['raw_path']).read_bytes()==blob,'raw_auxiliary_copy')
    preflight = js((folder/'preflight.json').read_bytes())
    need(preflight['commit']==SOURCE and preflight['gcp_mutations']=='none' and
         preflight['plan_sha256']==pins['files'][PLAN]['sha256'],'preflight_identity')
    same(preflight['budget'],dict(build_and_setup_seconds=120,closing_reserve_seconds=660,
         command_timeouts_sum_seconds=1600,guest_seconds=2700,oversubscribed=False,
         ssh_cutoff_before_guest_seconds=360,upload_estimate_seconds=183,worker_window_seconds=1737),'original_budget')
    need('warnings' not in preflight,'no_preflight_warning')
    normalized = {k:v for k,v in pins['plan'].items() if k not in ('note','schema')}
    normalized.update(build_targets=[],build_timeout_seconds=600)
    same(preflight['plan'],normalized,'preflight_commands')
    commands = pins['plan']['commands']; argv = commands[2]['argv']
    need([c['timeout_seconds'] for c in commands]==[850,180,570] and 1600+120<=1737 and
         argv[argv.index('--budget-seconds')+1]=='500' and '--reuse-census' in argv,'declared_budget')
    first_log = (folder/'first_failure.txt').read_text()
    need(DIAGNOSTIC in first_log and INVALID in first_log and 'INVALIDES module=tower : 1 sur 88' in first_log,
         'first_failure_preserved')


def declared_schedule(pins):
    # Execute only the pinned pure schedule AST, not the historical driver or its imports.
    import ast
    from types import SimpleNamespace
    result = subprocess.run(['git','show',SOURCE+':morsehgp3D_v11/bench/full_parallel.py'],cwd=HERE.parents[3],
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
    pin = pins['files']['bench/full_parallel.py']
    need(result.returncode==0 and sha(result.stdout)==pin['sha256'] and len(result.stdout)==pin['bytes'],'schedule_source')
    nodes = [n for n in ast.parse(result.stdout).body if isinstance(n,ast.FunctionDef) and n.name=='schedule']
    need(len(nodes)==1,'schedule_function')
    context = dict(need=need,profiles=SimpleNamespace(COUNTS=old.CASE_COUNTS))
    exec(compile(ast.Module(body=nodes,type_ignores=[]),'<pinned schedule>','exec'),context)
    actual = context['schedule'](reuse_census=True)
    def row(case,bits,mode,workers=48):
        return dict(case=case,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    expected = [row(case,bits,mode) for case in ('lidar_ng00','lidar_ng01','lidar_ng02')
                for bits in (21,24) for mode in (127,255,511)]
    expected += [row('lidar_ng00',21,511,w) for w in (1,8)]
    expected += [row('uniform_u18_n'+str(n),21,mode) for n in (8000,16000,32000) for mode in (127,255,511)]
    same(actual,expected,'unstarted_schedule'); need(len(actual)==29,'declared_units')
    return len(actual)


def check(folder=HERE):
    pins = source_contract(); receipt,worker,data = transport(folder,pins)
    counts = matrices(data,pins); commands(receipt,worker,data,pins)
    executed,built = other_mutants(data,pins); tower = tower_mutants(data,pins)
    preflight_and_first(folder,pins); live_sources(folder,receipt); unstarted = declared_schedule(pins)
    return dict(coherent=True,conforming=False,source=SOURCE,
        matrix_selected=sum(c[0] for c in counts.values()),matrix_passed=sum(c[1] for c in counts.values()),
        matrix_failed=sum(c[2] for c in counts.values()),matrix_no_closed_result=0,
        asan18_selected=264,asan18_passed=264,asan18_failed=0,native_product_gate_failures=0,
        successful_base_builds=8,successful_mutation_campaign_gates=6,
        non_tower_mutants_code_or_line=executed,non_tower_mutants_expected_construction=built,
        tower_mutants_code=tower,tower_mutants_invalid_construction=1,invalid_mutant=INVALID,
        full_attempts=0,semantic_reuse_attempts=0,full_measurements=0,unpersisted=0,unstarted_declared_units=unstarted,
        preflight_oversubscribed=False,declared_seconds_with_setup=1720,worker_window_seconds=1737)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,old.foundation.ET.ParseError,tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
