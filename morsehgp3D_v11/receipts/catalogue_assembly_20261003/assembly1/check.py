"""LIVE assembly1 reader. Code0 means coherent evidence, including an explicit failed campaign.

Raw local receipt and one archive are mandatory. Frozen 4b helpers only, no native call.
Deleted canonical files remain recorded hashes; archived files are rehashed and decoded.
"""
import copy
from dataclasses import asdict
import importlib.util
import json
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent
RECEIPTS = HERE.parent.parent
SOURCE = '4b8e04be6107d27cb91fea3746d944a3c9beadb4'
CONTRACT = 'ba48abc4684b66b1f02d7918fba94a2d389d96db016869d5042616a798a1d0a7'
COMMANDS = ('000_matrice','001_asan18','002_assembly')
SUPP = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_assembly/files/assembly.json'


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


optim = load('assembly_previous',RECEIPTS/'catalogue_optimizations_20261002/check.py')
baseline = load('assembly_full_transport',RECEIPTS/'full_20261002/check_full.py')
q4,old,legacy = optim.q4,optim.old,optim.legacy
need,js,sha,fields,BASE = old.need,old.js,old.sha,old.fields,old.BASE
same,finite = optim.same,legacy.finite
REFUSALS = optim.REFUSALS+(baseline.old.foundation.Refusal,)


def frozen():
    raw = (HERE/'source_contract.json').read_bytes()
    need(sha(raw)==CONTRACT,'contract_pin'); pins = js(raw)
    need(pins['source']==SOURCE,'source_pin')
    names = ('catalogue_semantic','catalogue_diagnostics','catalogue_g4','semantic_cache',
             'catalogue_profiles','catalogue_parallel','catalogue_assembly')
    saved = {name:sys.modules.get(name) for name in names}
    try:
        for name in names:
            path = HERE/'source_4b'/(name+'.py')
            need(sha(path.read_bytes())==pins['scripts'][name+'.py']['sha256'],'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name] = module; spec.loader.exec_module(module)
        return pins,sys.modules['catalogue_assembly']
    finally:
        for name,value in saved.items():
            if value is None: sys.modules.pop(name,None)
            else: sys.modules[name] = value


PINS,driver = frozen()
profiles = driver.profiles
DERIVED = ('stage_ms','pool_ms','catalogue_within_200ms','cloud_pool_catalogue_ms','cache_work')


def identity(row):
    need(type(row['case']) is str and all(type(row[k]) is int for k in
         ('coord_bits','kmax','workers','repetition','optimizations')) and row['diagnostics'] is False,'unit_types')
    return driver.identity(row)


def argv(row,case,build):
    name,bits,k,w,r,mode = identity(row); command = row['argv']
    need(type(command) is list and len(command)==12 and command[0]==build['path'] and
         Path(command[1]).name==case['coordinates'] and Path(command[2]).name==case['point_ids'] and
         Path(command[3]).name=='%s_b%d_k%d_w%d_r%d_o%d.bin' % (name,bits,k,w,r,mode) and
         command[4:]==[str(k),'16','256','0',str(2**32-1),str(8*1024**3),str(w),str(mode)],'native_argv')
    need(type(row['timeout_seconds']) is int and row['timeout_seconds']==15 and row['whole_input'] is True and
         type(row['count']) is int and row['count']==case['count'],'native_input')


def reuse_evidence(row,case,prior):
    evidence = row['semantic_reuse']
    need(set(evidence)=={'schema','mode','context','current_attempt','source_attempt','raw_sha256','bytes',
                         'hash_wall_seconds','decode_wall_seconds'},'reuse_fields')
    context = driver.reuse.Context('MHGP11CAT1',profiles.semantic.SCHEMA+';arity_counts=true',
        driver.reuse.decoder_digest([Path(profiles.semantic.__file__)]),row['coord_bits'],row['kmax'],
        case['count'],case['sha256'],case['ids_sha256'])
    same(evidence['context'],asdict(context),'reuse_context')
    same(evidence['current_attempt'],list(identity(row))+[False],'reuse_current')
    need(evidence['schema']==driver.reuse.SCHEMA and old.HEX.fullmatch(evidence['raw_sha256']) and
         type(evidence['bytes']) is int and evidence['bytes']>0 and
         finite(evidence['hash_wall_seconds']) and finite(evidence['decode_wall_seconds']) and
         finite(row['semantic_wall_seconds']) and
         evidence['hash_wall_seconds']+evidence['decode_wall_seconds']<=row['semantic_wall_seconds'],'reuse_time')
    need(evidence['raw_sha256']==row['canonical_sha256'] and evidence['bytes']==row['canonical_bytes'],'reuse_bytes')
    if evidence['mode']=='decoded':
        same(evidence['source_attempt'],list(identity(row))+[False],'decode_origin')
    else:
        need(evidence['mode']=='reused' and evidence['decode_wall_seconds']==0,'reuse_mode')
        key = tuple(evidence['source_attempt']); need(key in prior and prior[key]['status']=='ok','reuse_prior_success')
        source = prior[key]
        same(evidence['source_attempt'],list(identity(source))+[False],'reuse_source_identity')
        need(source['semantic_reuse']['mode']=='decoded','reuse_original_decode')
        for name in ('context','raw_sha256','bytes'):
            same(evidence[name],source['semantic_reuse'][name],'reuse_origin_'+name)
        if 'semantic' in row:
            same(row['semantic'],source['semantic'],'reuse_summary'); same(row['qmin_counts'],source['qmin_counts'],'reuse_qmin')


def attempt(row,case,build,complete,last,data,prior):
    argv(row,case,build)
    need(row['exit_code'] is None or type(row['exit_code']) is int,'exit_code_type')
    need(row.get('semantic_reuse_requested') is True,'reuse_requested')
    if 'semantic_wall_seconds' in row: need(finite(row['semantic_wall_seconds']),'semantic_time')
    if row['status']=='pending_semantic':
        need(not ({'semantic_reuse','qmin_counts'}|set(DERIVED)) & row.keys(),'pending_derived_output')
    compatible = copy.deepcopy(row); compatible['timeout_seconds'] = 30; compatible['argv'] = row['argv'][:10]
    compatible['argv'][3] = str(Path(row['argv'][3]).with_name('%s_b%d_k%d.bin' %
                               (row['case'],row['coord_bits'],row['kmax'])))
    post = any(e['stage'] in ('parallel','assembly','attempt_validation') for e in row['errors'])
    if row['status']=='invalid_output' and post:
        need(row['exit_code']==0,'postdecode_failure_code'); compatible['status'] = 'artifact_error'
    for error in compatible['errors']:
        if error['stage'] in ('parallel','assembly','attempt_validation'): error['stage'] = 'artifact'
    legacy.attempt(compatible,case,build,complete,last)
    if 'semantic_reuse' in row: reuse_evidence(row,case,prior)
    if row['status']!='ok':
        need(row['status'] in ('artifact_error','invalid_output') or 'qmin_counts' not in row,'early_qmin')
        return 0
    need(row['stderr']=='' and all(e.get('reason','none')=='none' for e in row['events']),'success_diagnostics')
    need('semantic_reuse' in row,'successful_reuse_evidence')
    q4.q4_signature(row); profiles.work_signature(row)
    checked = copy.deepcopy(row); driver.check_assembly(checked,row)
    need(checked['status']=='ok' and not checked['errors'],'frozen_assembly_verdict')
    for key in DERIVED: same(row[key],checked[key],'derived_'+key)
    value = row['semantic']; bits = row['coord_bits']
    need(all(type(value[k]) is int and 0<=value[k]<2**32 for k in ('coord_bits','kmax','sites','levels','balls')) and
         type(value['incidences']) is int and 0<=value['incidences']<2**64,'semantic_counts')
    limbs = [(b+63)//64 if b>127 else 2 for b in (8*bits+12,6*bits+8)]
    size = 58+40*case['count']+8*(4+sum(limbs))*value['levels']+72*value['balls']+8*value['incidences']
    need(row['canonical_bytes']==size<=profiles.semantic.LIMIT,'canonical_exact_size')
    need(row['catalogue_ms']/1000<=row['process_wall_seconds'],'api_process_wall')
    matches = [raw for path,raw in data.items() if path.startswith('results/cmd/002_assembly/') and
               Path(path).name==Path(row['argv'][3]).name]
    need(len(matches)<=1,'payload_duplicate')
    if not matches: return 0
    raw = matches[0]
    need(sha(raw)==row['canonical_sha256'] and len(raw)==size,'payload_bytes')
    decoded = profiles.semantic.decode(raw,bits,row['kmax'],case['count'],arity_counts=True)
    same(decoded.pop('qmin_counts'),row['qmin_counts'],'payload_qmin'); same(decoded,value,'payload_semantic')
    return 1


def schedule(value):
    requested = driver.schedule(); same(value['requested'],requested,'schedule_exact')
    for key,wanted in dict(requested_runs=36,timeout_seconds=15,native_schedule_bound_seconds=540,
                           budget_seconds=450,leaf_size=16,max_leaf=256).items(): same(value[key],wanted,'parameter_'+key)
    same(value['optimization_modes'],{'3':'fixed_serial_assembly','11':'fixed_parallel_assembly',
         '7':'adaptive_serial_assembly','15':'adaptive_parallel_assembly'},'modes')
    reuse = value['semantic_reuse']
    need(reuse['schema']==driver.reuse.SCHEMA and type(reuse['capacity']) is int and reuse['capacity']==64 and
         type(reuse['summary_limit_bytes']) is int and reuse['summary_limit_bytes']==65536,'reuse_report')
    need(all(type(value[k]) is bool for k in ('complete','conforming','full_schedule_completed')),'report_bools')
    rows,skips,intents = value['runs'],value['not_run'],value['launch_intents']
    keys = list(map(identity,rows)); missing = list(map(identity,skips)); wanted = list(map(identity,requested))
    need(len(keys)<=36 and keys==wanted[:len(keys)],'attempt_prefix')
    need(len(keys)+len(missing)<=36 and missing==wanted[len(keys):len(keys)+len(missing)] and
         all(r['reason']=='campaign_budget_before_launch' for r in skips),'omission_prefix')
    if value['complete']: need(len(keys)+len(missing)==36,'complete_inventory')
    need(len(intents)==len(rows) or (not value['complete'] and not skips and len(intents)==len(rows)+1),'intent_inventory')
    need(list(map(identity,intents))==wanted[:len(intents)],'intent_order')
    for i,row in enumerate(rows):
        need(row['argv']==intents[i]['argv'],'intent_argv')
        if row['status']=='pending_semantic': need(not value['complete'] and not skips and i==len(rows)-1,'pending_last')
    comparisons = js(json.dumps(driver.comparisons(rows,requested)))
    same(value['comparisons'],comparisons,'comparisons_recomputed')
    done = not skips and len(rows)==36
    good = done and all(r['status']=='ok' for r in rows) and all(c['status']=='equal' for c in comparisons)
    need(value['full_schedule_completed'] is (done if value['complete'] else False) and
         value['conforming'] is (good if value['complete'] else False),'report_verdict')
    if value['complete']:
        need(finite(value['campaign_wall_seconds']),'campaign_time')
        paid = sum(r['process_wall_seconds']+r.get('semantic_wall_seconds',0) for r in rows)
        need(paid<=value['campaign_wall_seconds'],'paid_time')
        if skips: need(value['campaign_wall_seconds']>=415,'budget_omission_elapsed')
    return dict(attempts=len(rows),successes=sum(r['status']=='ok' for r in rows),omissions=len(skips),
                unpersisted=36-len(rows)-len(skips),conforming=value['conforming'],
                different=sum(c['status']=='different' for c in comparisons),equal=sum(c['status']=='equal' for c in comparisons))


def benchmark(value,manifest,mhash,builds,data):
    need(value['schema']==driver.SCHEMA and value['attempt_schema']=='ehgp.v11.catalogue_attempt.v2' and
         value['manifest_sha256']==mhash and value['qualification_sha256']==sha(data[BASE+'summary.json']) and
         value['supplement_sha256']==sha(data[SUPP+'summary.json']),'report_identity')
    same(value['manifest'],manifest,'manifest_copy')
    verdict = schedule(value); by_bits = {}; names = {18:'gcc_release',21:'bits21',24:'bits24'}
    same([b['coord_bits'] for b in value['builds']],list(names),'build_inventory')
    for build in value['builds']:
        bits = build['coord_bits']; baseline.build_record(build,names[bits],'mhgp11_catalogue_bench',bits,builds,data)
        need(Path(build['path']).parts[-3:]==(names[bits],'build','mhgp11_catalogue_bench'),'build_path'); by_bits[bits] = build
    cases = {c['name']:c for c in manifest['cases']}; archived = 0; prior = {}
    for i,row in enumerate(value['runs']):
        archived += attempt(row,cases[row['case']],by_bits[row['coord_bits']],value['complete'],i==len(value['runs'])-1,data,prior)
        prior[tuple(list(identity(row))+[False])] = row
    for intent in value['launch_intents']:
        case = cases[intent['case']]; argv(intent,case,by_bits[intent['coord_bits']])
        need(intent['input_sha256']==case['sha256'] and intent['ids_sha256']==case['ids_sha256'] and
             intent.get('semantic_reuse_requested') is True,'intent_input_hashes')
    return dict(verdict,payloads_rehashed=archived,recorded_payloads=verdict['successes']-archived,
                semantic_reused=sum(r.get('semantic_reuse',{}).get('mode')=='reused' for r in value['runs']))


def matrix_parameters(data):
    for prefix,spec,budget,threads in ((BASE,PINS['matrix'],800,48),(SUPP,PINS['supplement'],160,12)):
        value = js(data[prefix+'summary.json']); same(value['budget_seconds'],budget,'matrix_budget')
        same(value['thread_budget'],threads,'matrix_threads'); expected = {c['name']:c for c in spec['configurations']}
        need(len(value['configurations'])==len(expected) and
             {c['name'] for c in value['configurations']}==set(expected),'matrix_configs')
        for observed in value['configurations']:
            source = expected[observed['name']]
            for key in ('threads','compiler','cmake_options','ctest_args'): same(observed[key],source[key],'matrix_'+key)
            need(observed['optional'] is source.get('optional',False),'matrix_optional')
            for step in observed['steps']:
                if step['name']=='configure':
                    options = [v.replace('{threads}',str(observed['threads'])) for v in source['cmake_options']]
                    need(step['argv'][-len(options):]==options,'configure_options')


def required_gates(data):
    required = {'mhgp11_catalogue_assembly_'+s for s in
                ('boundaries','wide_levels','refusals','memory_and_equivalence','pool_and_limits')}
    required.add('mhgp11_catalogue_assembly_fault_allocations')
    required |= {'mhgp11_catalogue_'+n+s for n in ('assembly_collector','adaptive_fraction',
                 'adaptive_combined_fraction','adaptive_bench_io','semantic_cache','sort_cache_fraction') for s in ('','_opt')}
    for c in js(data[BASE+'summary.json'])['configurations']:
        if c['status']=='ok' and c['name'] not in ('style','mutants'):
            need(required<={r['name'] for r in js(data[BASE+c['name']+'/tests.json'])},'required_assembly_gates')


def mutants(data):
    prefix = BASE+'mutants/'; cases = {c.get('name'):c for c in old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase')}
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


def analysis(value):
    rows = []
    for row in value['runs']:
        if row['status']!='ok': continue
        event = row['events'][1]
        rows.append(dict(case=row['case'],coord_bits=row['coord_bits'],mode=row['optimizations'],
            catalogue_ms=row['catalogue_ms'],process_wall_seconds=row['process_wall_seconds'],
            semantic_wall_seconds=row['semantic_wall_seconds'],semantic_mode=row['semantic_reuse']['mode'],
            cloud_ms=row['cloud_ms'],pool_ms=row['pool_ms'],stage_ms=row['stage_ms'],
            peak_reserved_bytes=event['peak_reserved_bytes'],reserved_after_bytes=event['reserved_after_bytes'],
            balls=event['balls'],levels=event['levels'],incidences=event['incidences'],tasks=event['timings']['tasks'],
            cache_work=event['cache_work']))
    pairs = []
    for name,bits in sorted({(row['case'],row['coord_bits']) for row in rows}):
        modes = {row['mode']:row for row in rows if row['case']==name and row['coord_bits']==bits}
        need(len({r['reserved_after_bytes'] for r in modes.values()})<=1,'same_profile_retained_output')
        for a,b in ((3,11),(7,15)):
            if a not in modes or b not in modes: continue
            x,y = modes[a],modes[b]
            pairs.append(dict(case=name,coord_bits=bits,modes=[a,b],catalogue_ms=[x['catalogue_ms'],y['catalogue_ms']],
                ratio_serial_over_parallel=x['catalogue_ms']/y['catalogue_ms'] if y['catalogue_ms'] else None,
                assembly_ms=[x['stage_ms']['assembly'],y['stage_ms']['assembly']],
                peak_delta_bytes=y['peak_reserved_bytes']-x['peak_reserved_bytes']))
    return dict(source=SOURCE,attempts=rows,assembly_pairs=pairs,
        wall_seconds=value.get('campaign_wall_seconds'),
        process_seconds=sum(r['process_wall_seconds'] for r in value['runs']),
        semantic_seconds=sum(r.get('semantic_wall_seconds',0) for r in value['runs']),
        memory_scope='Native Buffer reservations including live Cloud; excludes stacks, RSS and Python',
        timing_scope='Catalogue API only; eight disjoint but non-exhaustive stage intervals; one process per unit')


def transport(folder):
    receipt,worker,data = old.read_capture(folder); raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local','original_receipt_sha256','preserved_failure'}<=set(raw),'raw_required_fields')
    need(receipt['commit']==SOURCE and receipt['plan_sha256']==PINS['plan_sha256'],'capture_source')
    need(receipt['results_skipped_members']==[] and receipt['results_bytes']<=16*1024**2,'complete_bounded_archive')
    need(all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')) and
         not receipt['warnings'],'cleanup')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip()==
         ('0' if receipt['status']=='completed' else '3'),'controller_done')
    inventory = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,path = line.split('  ',1); path = 'results/'+path.removeprefix('./')
        need(path not in inventory and path in data and sha(data[path])==digest,'archive_manifest'); inventory.add(path)
    need(inventory==data.keys()-{'results/MANIFEST.sha256'},'archive_inventory')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')}==set(COMMANDS),'commands_inventory')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),('assembly.json',BENCH)]:
        need((folder/name).read_bytes()==data[path] if path in data else not (folder/name).exists(),'compact_copy')
    metas = [fields(data['results/cmd/'+name+'/meta.txt']) for name in COMMANDS]
    for meta,timeout in zip(metas,(850,180,530)):
        need(meta['requested_timeout_seconds']==str(timeout) and meta.get('group_closed')=='1' and
             meta.get('streams_truncated')=='0' and meta.get('residual_group_killed')=='0','command_closure')
    return receipt,worker,data,metas


def check(folder=HERE):
    receipt,worker,data,metas = transport(folder); matrix_parameters(data)
    counts,builds,code = q4.judge_matrix(data)
    for name,proof in builds.items():
        if 'CMakeCache.txt' in proof:
            optim.cache(proof,dict(MHGP11_SANITIZE='ON' if name=='gcc_asan_ubsan' else 'OFF',
                                  MHGP11_TSAN='ON' if name=='gcc_tsan' else 'OFF',MHGP11_POISON='ON' if name=='poison' else 'OFF'))
    if 'CMakeCache.txt' in builds['mutants']: optim.cache(builds['mutants'],{'MHGP11_MUTANT_JOBS':'32'})
    extra,suppcode = optim.supplement(data)
    if suppcode==0: baseline.supplement_provenance(data)
    required_gates(data); killed = mutants(data) if js(data[BASE+'mutants/result.json'])['status']=='ok' else None
    q4.command(metas[0],code); q4.command(metas[1],suppcode)
    manifest,mhash = old.inputs(folder,receipt)
    verdict = dict(conforming=False,attempts=0,successes=0,omissions=0,unpersisted=36,different=0,
                   payloads_rehashed=0,recorded_payloads=0,semantic_reused=0)
    if BENCH in data:
        need(code==suppcode==0,'benchmark_requires_qualification'); value = js(data[BENCH])
        verdict = benchmark(value,manifest,mhash,builds,data)
        q4.command(metas[2],0 if value['conforming'] else 1 if value['complete'] else None)
        if (folder/'metrics.json').exists(): same(js((folder/'metrics.json').read_bytes()),analysis(value),'derived_metrics')
    else: q4.command(metas[2])
    good = q4.session(receipt,worker,metas)
    need(good is verdict['conforming'] and receipt['preserved_failure'] is (not good),'session_verdict')
    construction = sum(kind=='construction' for entries in PINS['mutants'].values() for kind in entries.values())
    return dict(coherent=True,source=SOURCE,matrix_passed=sum(v[1] for v in counts.values()),
                matrix_total=sum(v[0] for v in counts.values()),asan18_passed=extra[1],asan18_total=extra[0],
                causal_mutants=killed,mutants_code_or_line=killed-construction if killed is not None else None,
                mutants_expected_construction=construction if killed is not None else None,**verdict)


if __name__=='__main__':
    try: print(json.dumps(check(),sort_keys=True))
    except REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,REFUSALS) else type(error).__name__))
        raise SystemExit(1)
