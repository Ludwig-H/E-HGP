"""LIVE full3 only: code0 means coherent evidence, including its preserved budget failure.

Raw local receipt is mandatory. Tar members are read with extractfile, never extracted.
Large FULL artifacts were deleted remotely: recorded hashes/counts are checked, not their bytes.
The 42 small v10/v11 common artifacts ARE archived and compared again byte for byte.
"""
import copy
import importlib.util
import json
import math
from pathlib import Path
import sys
import tarfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('full3_failure_helpers', HERE/'check_failure.py')
failure = importlib.util.module_from_spec(spec); spec.loader.exec_module(failure)
q4, old, need, js, BASE = failure.q4, failure.old, failure.need, failure.js, failure.BASE
SOURCE = 'c6ca345e0239b70191526eb0d38e516298386cea'
ARCHIVE = 'cf7143d1a8250da70a642b7506ca8f1ec725089139c13e8b71172a187382c8ef'
V10_ARCHIVE = '79fa4c637d796a2b9ff0310842b7fe2e6419cd3d4d0d93b56b1084580028f3b2'
CONTRACT = 'cb27a1c977a40f4b325d12076ed08058bd106ed42e237490ba917d855aa538fe'
SUPP, DIFF, BENCH = failure.SUPP, failure.DIFF, 'results/cmd/003_full/'
CASES = [('singleton',1,1),('line024',3,3),('square_plain',4,4),('square_center',5,5),
         ('global_q3_nonfirst_shell',5,5),('two_components_same_plateau',5,6),('line13_K12',12,13),
         ('regular_tetra',4,4),('obtuse_prefix',4,4),('extended_q4',5,5),('maximum_tetra',4,4),
         ('close_levels',3,3),('random0',5,6),('random1',5,7)]


def same(actual, expected, label):
    need(json.dumps(actual,sort_keys=True,allow_nan=False) ==
         json.dumps(expected,sort_keys=True,allow_nan=False), label)


def frozen_driver():
    raw = (HERE/'full3_contract.json').read_bytes()
    need(old.sha(raw) == CONTRACT, 'source_contract')
    contract = js(raw)
    need(contract['source'] == SOURCE and contract['archive'] == ARCHIVE, 'source_pin')
    saved = {name:sys.modules.get(name) for name in
             ('catalogue_g4','catalogue_semantic','catalogue_profiles','full_semantic','full_campaign')}
    try:
        for name in saved:
            path = HERE/'source_c6'/(name+'.py')
            need(old.sha(path.read_bytes()) == contract['scripts'][name+'.py']['sha256'], 'script_pin')
            spec = importlib.util.spec_from_file_location(name,path)
            module = importlib.util.module_from_spec(spec); sys.modules[name] = module; spec.loader.exec_module(module)
        return sys.modules['full_campaign']
    finally:
        for name, module in saved.items():
            if module is None: sys.modules.pop(name,None)
            else: sys.modules[name] = module


driver = frozen_driver()


def finite(value):
    return type(value) in (int,float) and math.isfinite(value) and value >= 0


def build_record(record, name, executable, bits, builds, data):
    files = builds[name]
    need(record['configuration'] == name and type(record['coord_bits']) is int and record['coord_bits'] == bits and
         Path(record['path']).name == executable, 'build_identity')
    for key,value in [('sha256',files[executable]['sha256']),('bytes',files[executable]['size']),
                      ('cache_sha256',files['CMakeCache.txt']['sha256']),
                      ('provenance_sha256',old.sha(data[BASE+name+'/build_provenance.json']))]:
        same(record[key],value,'build_'+key)


def semantic_record(value, case, bits, kmax):
    need(value['schema'] == 'ehgp.v11.full_semantic.v1' and old.HEX.fullmatch(value['sha256']) and
         old.HEX.fullmatch(value['raw_sha256']), 'recorded_hashes')
    driver.unsigned(value,('births','bytes','coord_bits','edges','kmax','merges','nodes','points','sites','verticals'))
    need(value['coord_bits'] == bits and value['kmax'] == kmax and
         value['sites'] == value['points'] == case['count'] and len(value['orders']) == kmax,'semantic_identity')
    for k,order in enumerate(value['orders'],1):
        driver.unsigned(order,('order','births','edges','merges','nodes','root','verticals'))
        need(order['order'] == k and order['births'] > 0 and
             order['nodes'] == order['births']+order['merges'] == order['edges']+1 and
             order['root'] < order['nodes'] and order['verticals'] == (order['nodes'] if k > 1 else 0),
             'semantic_order_counts')
    for key in ('births','edges','merges','nodes','verticals'):
        need(sum(o[key] for o in value['orders']) == value[key], 'semantic_total_'+key)
    minimum = 42+40*case['count']+40*kmax+72*value['nodes']+96*value['births']+8*(value['verticals']+value['edges'])
    need(value['bytes'] >= minimum, 'semantic_minimum_bytes')


def attempt(row, intent, request, case, build):
    same({k:row[k] for k in request},request,'attempt_identity')
    same({k:intent[k] for k in request},request,'intent_identity')
    need(row['status'] == 'ok' and type(row['exit_code']) is int and row['exit_code'] == 0 and
         row['errors'] == [] and row['stderr'] == '' and row['whole_input'] is True and
         row['count'] == case['count'] and row['timeout_seconds'] == 60,'attempt_success')
    bits,k,r = request['coord_bits'],request['kmax'],request['repetition']
    argv = row['argv']
    need(len(argv) == 11 and argv[0] == build['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == '%s_b%d_k%d_w48_r%d.bin' % (case['name'],bits,k,r) and
         argv[4:] == [str(k),'16','256','0',str(2**32-1),str(8*1024**3),'48'],'attempt_argv')
    need(intent['argv'] == argv and intent['whole_input'] is True and intent['count'] == case['count'] and
         intent['timeout_seconds'] == 60 and intent['input_sha256'] == case['sha256'] and
         intent['ids_sha256'] == case['ids_sha256'],'intent_payload')
    need(finite(row['process_wall_seconds']) and finite(row['semantic_wall_seconds']),'attempt_times')
    parsed = [js(line) for line in row['stdout'].splitlines() if line.strip()]
    same(parsed,row['events'],'attempt_events')
    semantic_record(row['semantic'],case,bits,k)
    replay = copy.deepcopy(row)
    inspect = driver.semantic.inspect
    try:
        # Validation des diagnostics enregistres, PAS nouvelle lecture de l'artefact supprime.
        driver.semantic.inspect = lambda *_:copy.deepcopy(row['semantic'])
        driver.collect(replay,case,None,bits)
    finally:
        driver.semantic.inspect = inspect
    need(replay['status'] == 'ok' and not replay['errors'],'replayed_native_diagnostics')
    for key in ('full_ms','whole_peak_reserved_bytes','cloud_ms','read_ms','pool_ms','stage_ms',
                'full_within_200ms','cloud_pool_full_ms'):
        same(row[key],replay[key],'derived_'+key)
    need(row['full_ms']/1000 <= row['process_wall_seconds'],'native_process_wall')


def report(value, manifest, manifest_hash, builds, data):
    need(value['schema'] == driver.SCHEMA and value['complete'] is True and
         value['full_schedule_completed'] is False and value['conforming'] is False,'preserved_campaign_failure')
    need(value['manifest_sha256'] == manifest_hash and value['qualification_sha256'] == old.sha(data[BASE+'summary.json'])
         and value['supplement_sha256'] == old.sha(data[SUPP+'summary.json']),'report_links')
    same(value['manifest'],manifest,'report_manifest')
    requested = driver.schedule()
    same(value['requested'],requested,'schedule_exact')
    need(value['requested_runs'] == 24 and value['budget_seconds'] == 600 and value['timeout_seconds'] == 60 and
         value['leaf_size'] == 16 and value['max_leaf'] == 256,'campaign_parameters')
    need(len(value['runs']) == len(value['launch_intents']) == 13 and len(value['not_run']) == 11,'campaign_inventory')
    same(value['not_run'],[dict(r,reason='campaign_budget_before_launch') for r in requested[13:]],'omissions_exact')
    expected_builds = {18:'gcc_release',21:'bits21',24:'bits24'}
    need(len(value['builds']) == 3 and [b['coord_bits'] for b in value['builds']] == list(expected_builds),'builds_exact')
    by_bits = {}
    for build in value['builds']:
        bits = build['coord_bits']; build_record(build,expected_builds[bits],'mhgp11_full_bench',bits,builds,data)
        by_bits[bits] = build
    cases = {c['name']:c for c in manifest['cases']}
    for row,intent,request in zip(value['runs'],value['launch_intents'],requested[:13]):
        attempt(row,intent,request,cases[request['case']],by_bits[request['coord_bits']])
    same(value['comparisons'],driver.comparisons(value['runs'],requested),'comparisons_recomputed')
    elapsed = value['campaign_wall_seconds']
    paid = sum(r['process_wall_seconds']+r['semantic_wall_seconds'] for r in value['runs'])
    need(finite(elapsed) and 520 <= paid <= elapsed < 600,'budget_omission_lower_bound')
    return dict(attempts=13,successes=13,omitted=11,conforming=False,different=sum(
        c['status'] == 'different' for c in value['comparisons']),campaign_wall_seconds=elapsed)


def differential(value, data, builds):
    need(value['schema'] == 'ehgp.v11.full_v10_diff.v1' and value['source_v10'] ==
         'c764e121aa52f2e5dd9b85fbe308c9c3511ff55e' and value['archive_sha256'] == V10_ARCHIVE and
         value['complete'] is True and
         value['conforming'] is True and not value['errors'] and value['comparison'] ==
         'common_canonical_bytes_full_only' and value['qualification_sha256'] == old.sha(data[BASE+'summary.json']),
         'differential_status')
    same([(c['name'],c['kmax'],c['sites']) for c in value['cases']],CASES,'differential_inventory')
    command_names = ['configure_v10','build_v10']
    for i,case in enumerate(value['cases']):
        stem = '%02d_%s' % (i,case['name'])
        source = data[DIFF+stem+'.u32le']
        need(len(source) == 12*case['sites'] and old.sha(source) == case['input_sha256'],'differential_input')
        common = data[DIFF+stem+'.v10.common']
        need(common and old.sha(common) == case['v10_sha256'],'v10_common')
        need([r['bits'] for r in case['v11']] == [18,21,24],'differential_profiles')
        for row in case['v11']:
            raw = data[DIFF+stem+'.v11_%d.common' % row['bits']]
            need(row['equal'] is True and row['sha256'] == old.sha(raw) and raw == common,'common_byte_equality')
        command_names += [stem+s for s in ('_mhgp10_catalogue','_mhgp10_tower','_v11_18','_v11_21','_v11_24')]
    same([c['name'] for c in value['commands']],command_names,'differential_commands')
    for command in value['commands']:
        need(command['status'] == 'ok' and type(command['returncode']) is int and command['returncode'] == 0 and
             finite(command['seconds']),'differential_execution')
        for stream in ('stdout','stderr'):
            entry = command[stream]; raw = data[DIFF+entry['path']]
            need(entry['sha256'] == old.sha(raw) and entry['bytes'] == len(raw),'differential_stream')
    need(set(value['v11_binaries']) == {'18','21','24'},'differential_builds')
    for bits,name in ((18,'gcc_release'),(21,'bits21'),(24,'bits24')):
        build_record(value['v11_binaries'][str(bits)],name,'mhgp11_tower_forest_probe',bits,builds,data)
    return 42


def supplement_provenance(data):
    value = js(data[SUPP+'gcc_asan_ubsan18/build_provenance.json'])
    need(value['schema'] == 'ehgp.v11.build_provenance.v1' and value['complete'] is True and
         value['errors'] == [], 'supplement_provenance_status')
    files = {r['path']:r for r in value['files']}
    need(len(files) == len(value['files']), 'supplement_provenance_unique')
    for name,row in files.items():
        need(not Path(name).is_absolute() and '..' not in Path(name).parts and
             old.HEX.fullmatch(row['sha256']) and type(row['size']) is int and row['size'] >= 0,
             'supplement_provenance_field')
        if 'text' in row:
            raw = row['text'].encode()
            need(len(raw) == row['size'] and old.sha(raw) == row['sha256'], 'supplement_text')
    need({'CMakeCache.txt','libmhgp11.a'} <= set(files), 'supplement_build_files')
    for target in ('mhgp11_full_bench','mhgp11_num_probe','mhgp11_index_probe','mhgp11_tower_forest_probe'):
        need(target in files and files[target]['size'] > 0, 'supplement_binary')
        for suffix in ('flags.make','link.txt'):
            name = 'CMakeFiles/'+target+'.dir/'+suffix
            need(name in files and '-fsanitize=address,undefined' in files[name]['text'], 'supplement_instrumentation')
    return files


def check(folder=HERE/'full3'):
    receipt,worker,data = old.read_capture(folder)
    need(receipt['commit'] == SOURCE and receipt['results_sha256'] == ARCHIVE,'pinned_capture')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() == '3' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')),'cleanup')
    names = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,name = line.split('  ',1); name = 'results/'+name.removeprefix('./')
        need(name not in names and name in data and old.sha(data[name]) == digest,'archive_manifest'); names.add(name)
    need(names == set(data)-{'results/MANIFEST.sha256'},'archive_inventory')
    for name,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),
                      ('v10diff.json',DIFF+'summary.json'),('full.json',BENCH+'files/full.json')]:
        need((folder/name).read_bytes() == data[path],'compact_copy')
    manifest,manifest_hash = old.inputs(folder,receipt)
    uploaded = {r['name']:r for r in receipt['data_files']}
    need(uploaded['v10_frozen_c764.tar.gz']['sha256'] == V10_ARCHIVE,'v10_uploaded_source')
    counts,builds,code = q4.judge_matrix(data)
    need(code == 0 and tuple(sum(c[i] for c in counts.values()) for i in range(4)) == (1971,1971,0,0),'matrix_passes')
    extra = js(data[SUPP+'summary.json']); configs = extra['configurations']
    need(extra['requested'] == ['gcc_asan_ubsan18'] and len(configs) == 1 and extra['complete'] is True and
         extra['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and
         extra['statuses'] == {'gcc_asan_ubsan18':'ok'} and configs[0]['name'] == 'gcc_asan_ubsan18' and
         extra['conforming'] is True and type(extra['exit_code']) is int and extra['exit_code'] == 0 and
         not extra['signals'],'supplement_status')
    mapped = {BASE+p[len(SUPP):]:v for p,v in data.items() if p.startswith(SUPP)}
    need(old.foundation.judge_config(configs[0],mapped) == (139,139,0,0),'supplement_passes')
    proof = supplement_provenance(data)
    cache = dict(line.split('=',1) for line in proof['CMakeCache.txt']['text'].splitlines()
                 if line and not line.startswith(('#','//')) and '=' in line)
    for key,wanted in [('MHGP11_COORD_BITS:STRING','18'),('MHGP11_MODULES:STRING','num;index;tower'),
                       ('MHGP11_SANITIZE:BOOL','ON'),('MHGP11_TSAN:BOOL','OFF'),('MHGP11_POISON:BOOL','OFF')]:
        need(cache.get(key) == wanted,'supplement_cache')
    compared = differential(js(data[DIFF+'summary.json']),data,builds)
    verdict = report(js(data[BENCH+'files/full.json']),manifest,manifest_hash,builds,data)
    commands = ('000_matrice','001_asan18','002_v10diff','003_full')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(commands),'worker_commands')
    for name,exit_code,timeout in zip(commands,(0,0,0,1),(650,200,300,680)):
        meta = old.fields(data['results/cmd/'+name+'/meta.txt']); q4.command(meta,exit_code)
        need(meta['requested_timeout_seconds'] == str(timeout) and meta['streams_truncated'] == '0' and
             meta['residual_group_killed'] == '0','command_closure')
    need(worker['commands_total'] == '4' and worker['commands_ok'] == '3' and worker['status'] == 'failed' and
         receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1 and
         receipt['preserved_failure'] is True,'worker_failure')
    return dict(coherent=True,source=SOURCE,matrix_passed=1971,asan18_passed=139,v10_comparisons=compared,**verdict)


if __name__ == '__main__':
    try:
        print(json.dumps(check(),sort_keys=True))
    except (old.foundation.Refusal,ValueError,KeyError,TypeError,IndexError,OSError,tarfile.TarError,
            old.foundation.ET.ParseError) as error:
        print('REFUS '+(str(error) if isinstance(error,old.foundation.Refusal) else type(error).__name__))
        raise SystemExit(1)
