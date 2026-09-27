#!/usr/bin/env python3
"""Compact LIVE readback of one closed R2 FULL trace diagnostic; never call GCP."""
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import capture as c
import worker as w

SCHEMA = 'mhgp9_full_nsys_readback_v2'
HERE = Path(__file__).resolve().parent
TEXT_FILES = ('receipt.json','guard_evidence.json','sources_before.json','sources_after.json',
              'build_recipe_before.json','build_recipe_after.json','compiled_dependencies_before.json',
              'compiled_dependencies_after.json','build_products_before.json','build_products_after.json',
              'unprofiled_probe.json','profile_probe.json')
SUFFIXES = ('.command.json','.intent.json','.stdout','.stderr')


def unique(pairs):
    result = {}
    for key,value in pairs:
        c.need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def read(path):
    return json.loads(Path(path).read_bytes(),object_pairs_hook=unique)


def encoded(value):
    return (json.dumps(value,indent=2,sort_keys=True,allow_nan=False)+'\n').encode()


def safe_text(path):
    raw = Path(path).read_bytes()
    raw.decode('utf-8')
    c.need(b'PRIVATE KEY' not in raw and not re.search(rb'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+',raw),
           'no key material in published text')
    return raw


def projection(after):
    # Same explicit projection used by the preceding S2 publication reader.
    return {key:after[key] for key in ('name','selfLink','status','lastStartTimestamp','lastStopTimestamp',
            'zone','machineType','labels','scheduling') if key in after}


def stop_check(native,host,verdict,after):
    generation = host.get('generation')
    c.need(host.get('targeted_shutdown_certified') is True and
           verdict.get('targeted_shutdown_certified') is True and verdict.get('generation') == generation,
           'certified same-generation stop')
    native.validate_target(after,'TERMINATED',generation)
    begin = datetime.fromisoformat(generation.replace('Z','+00:00'))
    end = datetime.fromisoformat(after['lastStopTimestamp'].replace('Z','+00:00'))
    c.need(begin.tzinfo is not None and end.tzinfo is not None and end >= begin, 'stop chronology')
    elapsed = (end-begin).total_seconds()
    c.need(verdict.get('vm_elapsed_seconds') == elapsed, 'stop duration binding')
    return dict(generation=generation,stopped=after['lastStopTimestamp'],vm_elapsed_seconds=elapsed,
                billed_cost_usd=None)


def check_command(directory,name,row,remember,expected=None):
    command,intent = directory/(name+'.command.json'),directory/(name+'.intent.json')
    remember(command);remember(intent)
    c.need(read(command) == row and row.get('group_closed') is True and
           type(row.get('exit_code')) is int, 'closed command record')
    planned = read(intent)
    stable = ('name','argv','started_epoch','timeout_seconds') if expected is None else (
        'argv','cwd','started_utc','session_work_deadline_epoch','per_command_watchdog')
    c.need(all(row.get(key) == planned.get(key) for key in stable), 'intent/command binding')
    if expected is None:start,end = row['started_epoch'],row['ended_epoch']
    else:
        c.need(row['argv'] == expected and planned.get('pid') is None and planned.get('exit_code') is None and
               planned.get('group_closed') is False,'exact original guest intent')
        start,end = (datetime.fromisoformat(row[key]).timestamp() for key in ('started_utc','ended_utc'))
    c.need(end >= start, 'command chronology')
    for stream in ('stdout','stderr'):
        path = directory/(name+'.'+stream);remember(path)
        c.need(w.sha(path) == row[stream+'_sha256'], 'command stream hash')
    return start,end


def recipes(remote,payload,cases,tools):
    root,build,output = remote/'source',remote/'build',remote/'output'
    cli,deb = remote/'tools/nsys'/w.NSYS_RELATIVE_CLI,remote/'tools/nsight-systems-cli.deb'
    native = payload.probe_command(build,root,cases[0])
    pre_case = payload.preflight_case(cases,payload.preflight_cloud())
    engine = dict(pre_case,levers=payload.engine_levers(pre_case['levers']))
    capacity,lanes = payload.deferral_capacities(pre_case['levers'])
    pre = lambda case,cap=0,judge=False,lane=0:[payload.TIME,'-v',str(build/payload.PROBE_TARGET),
        str(output/payload.PREFLIGHT_FILE),*payload.expected_probe_tail(case,cap,judge,lane)]
    return dict(compiler=[tools['g++'],'--version'],cmake_version=[tools['cmake'],'--version'],
        nvcc_version=[tools['nvcc'],'--version'],
        gpu_inventory=[tools['nvidia-smi'],'--query-gpu=name,driver_version,memory.total,compute_cap','--format=csv,noheader'],
        configure=payload.configure_command(tools,root,build),build=payload.build_command(tools,build,False),
        binary_libraries=['ldd',str(build/payload.PROBE_TARGET)],
        preflight=pre(pre_case,judge=payload.judged_preflight(pre_case['levers'])),preflight_engine=pre(engine),
        preflight_deferral=pre(pre_case,capacity,True,lanes),
        nsys_download=['curl','--fail','--location','--silent','--show-error','--proto','=https',
            '--max-time','120','--max-filesize',str(w.NSYS_SIZE),'--output',str(deb),w.NSYS_URL],
        nsys_extract=['dpkg-deb','-x',str(deb),str(remote/'tools/nsys')],
        nsys_version=[str(cli),'--version'],nsys_profile_help=[str(cli),'profile','--help'],
        unprofiled=native,profile=w.profile_command(cli,output/'full_trace',native),
        nsys_stats=[str(cli),'stats','--report','cuda_gpu_kern_sum,cuda_gpu_mem_time_sum,cuda_api_sum,osrt_sum',
                    '--format','csv',str(output/'full_trace.sqlite')])


def worker_identity(vm,host,verdict,target,pin):
    c.need(vm.get('schema') == w.SCHEMA and vm.get('scope') == 'FULL_profile_diagnostic_only' and
           vm.get('target') == target and vm.get('generation') == host['generation'] and
           vm.get('engine_commit') == w.ENGINE_COMMIT and vm.get('worker_sha256') == pin and
           vm.get('source_manifest_sha256') == c.MANIFEST_SHA and vm.get('native_validator_sha256') == w.PAYLOAD_PIN and
           vm.get('collector_sha256') == w.HELPER_PIN and vm.get('public_status') == 'not_claimed' and
           vm.get('contract_certified') is False and vm.get('useful_budget_seconds') == w.USEFUL_SECONDS and
           vm.get('binary_origin') == 'rebuilt_from_original_snapshot' and
           vm.get('dependency_scope') == 'native_postbuild_depfiles_pinned_before_execution_then_closed' and
           vm.get('CUDA_installation_attempted') is False and vm.get('system_installation_attempted') is False,
           'diagnostic worker identity/scope, also on failure')
    success = vm.get('status') == 'completed'
    c.need(vm.get('status') in ('failed','completed') and host.get('status') == verdict.get('status') ==
           ('completed' if success else 'worker_failed') and type(verdict.get('controller_exit')) is int and
           type(host.get('worker_exit_code')) is int and verdict['controller_exit'] == host['worker_exit_code'] ==
           (0 if success else 1), 'closed success or failure, never promotion')
    c.need(type(vm.get('elapsed_seconds')) in (int,float) and math.isfinite(vm['elapsed_seconds']) and
           vm['elapsed_seconds'] >= 0,'finite worker duration')
    return success


def hash_map(value):
    return type(value) is dict and bool(value) and all(type(path) is str and Path(path).is_absolute() and
        type(pin) is str and re.fullmatch('[0-9a-f]{64}',pin) for path,pin in value.items())


def trace_diagnostics(path):
    with w.sqlite3.connect(path.resolve().as_uri()+'?mode=ro',uri=True) as database:
        database.row_factory = w.sqlite3.Row
        rows = [dict(row) for row in database.execute(
            'SELECT timestamp,timestampType,source,severity,text,globalPid FROM DIAGNOSTIC_EVENT ORDER BY rowid')]
        severity = {row['id']:row['label'] for row in database.execute('SELECT id,label FROM ENUM_DIAGNOSTIC_SEVERITY_LEVEL')}
        clocks = {row['id']:row['name'] for row in database.execute('SELECT id,name FROM ENUM_DIAGNOSTIC_TIMESTAMP_SOURCE')}
    c.need(rows and all(row['severity'] in severity and row['timestampType'] in clocks for row in rows),
           'explicit profiler diagnostics and clock domains')
    return dict(events=rows,severity_labels=severity,timestamp_sources=clocks,trace_completeness_certified=False,
                warnings=[row['text'] for row in rows if severity[row['severity']] in ('Warning','Error')])


def closed_full(vm,output,remote,manifest,payload,cases,commands):
    c.need(vm['sources_stable'] is True and read(output/'sources_before.json') == read(output/'sources_after.json') == manifest,
           'original source closure')
    for field,flag in (('build_recipe','build_recipe_stable'),('compiled_dependencies','compiled_dependencies_stable'),
                       ('build_products','build_products_stable')):
        before,after = (read(output/(field+'_'+when+'.json')) for when in ('before','after'))
        c.need(vm[flag] is True and hash_map(before) and before == after,field+' closure')
        if field == 'build_recipe':
            expected = {str(remote/'build/CMakeCache.txt')} | {str(remote/'build/CMakeFiles'/(target+'.dir')/name)
                for target in ('mhgp9_tower_probe','mhgp9_chain','mhgp9_gen','mhgp9_gpu')
                for name in ('flags.make','build.make','link.txt','DependInfo.cmake')}
            c.need(expected <= set(before),'all four native target recipes')
        if field == 'build_products':
            c.need({str(remote/'build'/('lib'+target+'.a')) for target in ('mhgp9_chain','mhgp9_gen','mhgp9_gpu')} <= set(before)
                   and any(path.endswith('.o') for path in before) and any(path.endswith('.o.d') for path in before),
                   'native objects, compiler depfiles and three static libraries')
        if field == 'compiled_dependencies':
            prefix = str(remote/'source')+'/'
            c.need(all(manifest.get(path[len(prefix):]) == pin for path,pin in before.items() if path.startswith(prefix)) and
                   {str(remote/'source'/name) for name in (payload.PROBE_SOURCE,payload.CHAIN_SOURCE,
                       'morsehgp3D_v9/src/gpu/filter_runner.cu')} <= set(before),'compiled original FULL/GPU sources')
    for field in ('libraries','tool_hashes','artifacts'):
        before,after = vm[field+'_before'],vm[field+'_after']
        c.need(hash_map(before) and before == after,field+' closure')
    binary_pin = w.sha(output/'qualified_probe')
    c.need(vm['binary_sha256'] == binary_pin and vm['artifacts_before'] == {
        str(remote/'build'/payload.PROBE_TARGET):binary_pin,str(remote/'output/qualified_probe'):binary_pin,
        str(remote/'source'/cases[0]['file']):manifest[cases[0]['file']]},'rebuilt and transported binary/input identity')
    c.need(vm['native_argv'] == commands['unprofiled'] and vm['nsys_stable'] is True and vm['nsys'] == dict(
        url=w.NSYS_URL,package_sha256=w.NSYS_SHA256,package_bytes=w.NSYS_SIZE,path=commands['nsys_version'][0],
        sha256=vm['nsys']['sha256']) and re.fullmatch('[0-9a-f]{64}',vm['nsys']['sha256']),'native and pinned Nsight tools')
    c.need(w.NSYS_VERSION in (output/'nsys_version.stdout').read_text(),'observed Nsight version')
    raw = payload.preflight_cloud();case = payload.preflight_case(cases,raw)
    c.need((output/payload.PREFLIGHT_FILE).read_bytes() == raw,'original synthetic preflight input')
    engine = dict(case,levers=payload.engine_levers(case['levers']))
    capacity,lanes = payload.deferral_capacities(case['levers']);pre = {}
    for name,current,cap,judge,lane in (('preflight',case,0,payload.judged_preflight(case['levers']),0),
            ('preflight_engine',engine,0,False,0),('preflight_deferral',case,capacity,True,lanes)):
        value = payload.strict_json((output/(name+'.stdout')).read_bytes());pre[name] = value
        c.need(payload.validate_probe(value,current,0,inputs=payload.preflight_inputs(raw),capacity=cap,judge=judge,
               lanes_capacity=lane) == 'complete_relative','native preflight replay')
        payload.validate_external_wall(value,read(output/(name+'.command.json'))['elapsed_seconds'])
        payload.validate_gnu_time((output/(name+'.stderr')).read_text(),0)
        if name != 'preflight_deferral':payload.validate_preflight_work(value,current['levers'])
    c.need(all(payload.logical_result(value) == payload.logical_result(pre['preflight']) and
               payload.certificate_work(value) == payload.certificate_work(pre['preflight']) for value in pre.values()) and
           payload.device_ran(case,pre['preflight']),'native GPU/engine/deferral preflight equality')
    c.need(vm['preflight'] == dict(sites=case['n'],frames=case['frames'],tower_digest=pre['preflight']['tower_digest'],
        GPU_executed=True,engine_equal=True,deferral_equal=True,deferred=pre['preflight_deferral']['q34_batch']['deferred'],
        lanes_deferred=pre['preflight_deferral']['q34_batch']['lanes_deferred']),'preflight receipt equals native evidence')
    values = {}
    for name in ('unprofiled','profile'):
        value = w.native_probe((output/(name+'.stdout')).read_text(),payload,cases[0]);values[name] = value
        c.need(value == read(output/(name+'_probe.json')),'saved native JSON equals command output')
        payload.validate_external_wall(value,read(output/(name+'.command.json'))['elapsed_seconds'])
    c.need(payload.logical_result(values['unprofiled']) == payload.logical_result(values['profile']),'same FULL under profiler')
    c.need(set(vm['reports']) == {'full_trace.nsys-rep','full_trace.sqlite'},'two native profiler reports')
    for name,pin in vm['reports'].items():
        path = output/name
        c.need(pin == dict(sha256=w.sha(path),bytes=path.stat().st_size) and path.stat().st_size > 0,'private report closure')
    c.need(w.sqlite_summary(output/'full_trace.sqlite') == vm['activity'],'actual recorded CUDA kernels')
    diagnostics = trace_diagnostics(output/'full_trace.sqlite')
    return dict(activity=vm['activity'],reports=vm['reports'],binary_sha256=binary_pin,
                profiler_warnings=diagnostics['warnings'],trace_completeness_certified=False,
                chain_total_ms={name:value['frames']['chain_total_ms'] for name,value in values.items()})


def collect(session_dir):
    session_dir = session_dir.absolute();host = session_dir/'full_host';output = host/'received/output'
    public,private = {},{}
    def remember(path):
        path = Path(path)
        c.need(path.is_file() and not path.is_symlink(), 'regular private evidence: '+str(path))
        private[str(path)] = w.sha(path)
    def publish_text(path,name):
        remember(path);public[name] = safe_text(path)
    native,_ = c.runtime()
    provenance = c.package_check(native)
    launch = read(session_dir/'launch.json');verdict = read(session_dir/'verdict.json')
    receipt = read(host/'receipt.json');after = read(session_dir/'after_stop.json')
    for path in (session_dir/'launch.json',session_dir/'verdict.json',session_dir/'after_stop.json',
                 host/'receipt.json',host/'source_manifest.json',host/'snapshot.tar.gz',host/'worker.py',
                 c.HERE/'capture.py',c.HERE/'worker.py',HERE/'readback.py',HERE/'selftest_readback.py',c.PACKAGE/'PACKAGE.json',
                 c.PACKAGE/'snapshot.tar.gz',c.PACKAGE/'source_manifest.json'):
        remember(path)
    c.committed(launch['diagnostic_commit'])
    c.need(launch.get('worker_sha256') == w.sha(w.__file__) == receipt.get('worker_sha256') == w.sha(host/'worker.py') and
           launch.get('wrapper_sha256') == w.sha(c.__file__) and launch.get('helper_sha256') == c.HELPER_SHA and
           receipt.get('controller_sha256') == c.HELPER_SHA and launch.get('native_package_provenance') == provenance,
           'committed diagnostic and native package binding')
    c.need(receipt.get('target') == native.TARGET and receipt.get('snapshot_sha256') == w.sha(host/'snapshot.tar.gz') == c.SNAPSHOT_SHA and
           receipt.get('manifest_sha256') == w.sha(host/'source_manifest.json') == c.MANIFEST_SHA, 'transported original package')
    cost = stop_check(native,receipt,verdict,after)
    c.need(verdict.get('status') == receipt.get('status') and verdict.get('contract_certified') is False and
           verdict.get('scope') == 'FULL_profile_diagnostic_not_contract', 'closed host verdict')
    names = set()
    for row in receipt['commands']:
        name = row['name']
        c.need(re.fullmatch('[a-z0-9_]+',name) and name not in names,'unique host command')
        names.add(name);check_command(host,name,row,remember)
    c.need('guarded_stop' in names and receipt.get('capture_received') is True and
           receipt.get('capture_pack_exit_code') == 0, 'closed capture transport')
    remember(host/'capture.tar.gz')
    c.need(w.sha(host/'capture.tar.gz') == receipt.get('capture_sha256'),'capture archive hash')
    stop = next(row for row in receipt['commands'] if row['name'] == 'guarded_stop')
    c.need(stop['exit_code'] == 0 and stop['argv'] == [str(host/'stop_and_verify.sh'),'--yes',
           '--expected-last-start-timestamp',receipt['generation']], 'same-generation guarded stop command')
    manifest = read(host/'source_manifest.json')
    cases,_ = native.validate_snapshot(host/'snapshot.tar.gz',manifest)
    remote = Path(receipt['remote_directory'])
    c.need(re.fullmatch(r'/tmp/ehgp-full-v7-[0-9a-f]{16}\.[A-Za-z0-9]{10}',str(remote)),'owned remote directory')
    # Every received file remains private unless explicitly selected below.
    received = {path.name:path for path in output.iterdir()}
    for path in received.values():remember(path)
    with tarfile.open(host/'capture.tar.gz','r:gz') as archive:
        members = archive.getmembers()
        c.need(len(members) == len(received)+1 and
               {member.name for member in members} == {'output',*('output/'+name for name in received)},'received archive inventory')
        for member in members:
            if member.name == 'output':c.need(member.isdir(),'archive output directory');continue
            c.need(member.isfile(),'regular captured evidence')
            digest = hashlib.sha256()
            with archive.extractfile(member) as stream:
                for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
            c.need(digest.hexdigest() == private[str(host/'received'/member.name)],'received evidence bound to archive')
    vm = read(output/'receipt.json')
    candidate = worker_identity(vm,receipt,verdict,native.TARGET,launch['worker_sha256'])
    rows = vm.get('commands');c.need(type(rows) is list,'worker command list')
    commands = recipes(remote,native.payload,cases,vm['tool_paths']) if rows else {}
    c.need(len(rows) <= len(commands),'native command prefix')
    previous = None
    for row,(name,argv) in zip(rows,commands.items()):
        start,end = check_command(output,name,row,remember,argv)
        c.need(previous is None or start >= previous,'sequential diagnostic processes');previous = end
        for suffix in SUFFIXES:publish_text(output/(name+suffix),'vm/'+name+suffix)
    for name in TEXT_FILES:
        if name in received:publish_text(output/name,'vm/'+name)
    mark = native.fields((host/'guardmarks/double_guard_verified').read_text())
    schedule = native.fields((host/'guest_schedule.stdout').read_text())
    remember(host/'guardmarks/double_guard_verified');remember(host/'guest_schedule.stdout')
    native.legacy.guard_deadline(mark,schedule,receipt['generation'],native.epoch(mark['date_utc']))
    if 'guard_evidence.json' in received:
        guard = read(output/'guard_evidence.json')
        c.need(guard == dict(mark=mark,schedule=schedule,metadata=dict(native.TARGET,machine='g4-standard-48')),'same guest guard evidence')
    summary = dict(schema=SCHEMA,status='failed',semantic_replay=False,scope='FULL_profile_diagnostic_not_contract',
        diagnostic_commit=launch['diagnostic_commit'],engine_commit=w.ENGINE_COMMIT,contract_certified=False,
        cost=cost,host_status=receipt['status'],worker_status=vm['status'],worker_error=vm.get('error'),
        worker_elapsed_seconds=vm['elapsed_seconds'],command_count=len(rows),GPU_execution='not_qualified_by_reader',
        reported_flags={key:vm.get(key) for key in ('FULL_attempted','FULL_executed','GPU_preflight_attempted',
            'GPU_preflight_executed','CUDA_profile_attempted','CUDA_profile_executed')})
    if candidate:
        try:
            c.need(len(rows) == len(commands) and all(row['exit_code'] == 0 for row in rows) and
                   all(value is True for value in summary['reported_flags'].values()) and
                   'error' not in vm and 'closure_error' not in vm,'all bounded diagnostic commands completed')
            summary.update(closed_full(vm,output,remote,manifest,native.payload,cases,commands))
            summary.update(status='completed',semantic_replay=True,GPU_execution='FULL_profile_trace_validated')
            public['DIAGNOSTICS.json'] = encoded(trace_diagnostics(output/'full_trace.sqlite'))
        except (ValueError,KeyError,OSError,TypeError,w.sqlite3.Error) as error:
            summary['validation_error'] = type(error).__name__+': '+str(error)
    # Text-only publication: no raw GCE/OS Login, keys, input or binary report.
    public['after_stop.json'] = encoded(projection(after))
    public['launch.json'] = encoded(launch);public['verdict.json'] = encoded(verdict)
    public['SUMMARY.json'] = encoded(summary)
    public['PRIVATE_LINKS.json'] = encoded(dict(session=str(session_dir),pins=private))
    warning_note = ('La collecte n’est pas certifiée complète. Les diagnostics Nsight signalent le pilote CUDA 13.0 non pris en charge '
        'avec repli sur les bibliothèques 12.9, et l’absence d’informations d’ordonnancement : '
        'l’activité des threads inférée via OSRT est imprécise. Voir DIAGNOSTICS.json pour les messages bruts '
        'et leurs domaines d’horloge distincts. Ces réserves n’invalident pas les résultats exacts de la référence sans profiler. '
        if summary.get('profiler_warnings') else 'Aucune complétude de trace revendiquée. ')
    public['README.md'] = ('# Nsight FULL R2 — diagnostic clos\n\n'
        'Statut du lecteur : '+summary['status']+'. Voir SUMMARY.json pour validation native et éventuels échecs. '
        'Aucun contrat de temps acquis ; les durées sous profiler ne sont pas des mesures de performance contractuelles. '
        +warning_note+'Arrêt certifié de la même génération ; allocation '
        +str(cost['vm_elapsed_seconds'])+' s, sans estimation de prix.\n\n'
        'Lecture LIVE : `python3 -B morsehgp3D_v9/audits/b_full_nsys_r2_20260927/readback.py --readback DOSSIER`. '
        'Les preuves privées référencées doivent rester disponibles. '
        'Seuls les JSON/logs VM sélectionnés, le lancement, le verdict et la projection d’arrêt sont publiés ; '
        'aucune clé, donnée KITTI, réponse GCE/OS Login brute ou rapport binaire.\n').encode()
    return summary,public


def publish(session_dir,destination):
    summary,files = collect(session_dir)
    destination.mkdir(parents=True,exist_ok=False)
    for name,raw in files.items():
        path = destination/name;path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('xb') as stream:stream.write(raw)
    inventory = {name:hashlib.sha256(raw).hexdigest() for name,raw in files.items()}
    (destination/'SHA256SUMS').write_text(''.join(pin+'  '+name+'\n' for name,pin in sorted(inventory.items())))
    return summary


def replay(directory):
    inventory = {}
    for line in (directory/'SHA256SUMS').read_text().splitlines():
        pin,name = line.split('  ',1)
        c.need(name not in inventory and re.fullmatch('[0-9a-f]{64}',pin),'publication inventory')
        inventory[name] = pin
    c.need(not any(path.is_symlink() for path in directory.rglob('*')), 'regular publication without symlinks')
    actual = {str(path.relative_to(directory)):w.sha(path) for path in directory.rglob('*')
              if path.is_file() and path.name != 'SHA256SUMS'}
    c.need(actual == inventory,'published file closure')
    links = read(directory/'PRIVATE_LINKS.json')
    c.need(all(Path(path).is_file() and not Path(path).is_symlink() and w.sha(path) == pin
               for path,pin in links['pins'].items()),'private LIVE evidence closure')
    summary,files = collect(Path(links['session']))
    c.need(all((directory/name).read_bytes() == raw for name,raw in files.items()) and set(files) == set(inventory),
           'publication reproduces from closed private evidence')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--session',type=Path);modes.add_argument('--readback',type=Path)
    parser.add_argument('--publish',type=Path)
    args = parser.parse_args()
    c.need(args.publish is None or args.session is not None,'publication needs private session')
    value = replay(args.readback) if args.readback else (
        publish(args.session,args.publish) if args.publish else collect(args.session)[0])
    print(json.dumps(value,sort_keys=True))
