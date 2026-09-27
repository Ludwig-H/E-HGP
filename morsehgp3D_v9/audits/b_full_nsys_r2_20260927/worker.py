#!/usr/bin/env python3
"""Explicit R1 port: build only the original FULL probe, then profile it; no cloud."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import signal
import shutil
import sqlite3
import sys
import time

sys.dont_write_bytecode = True
SCHEMA = 'mhgp9_full_nsys_diagnostic_v2'
USEFUL_SECONDS = 600
ENGINE_COMMIT = 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a'
MANIFEST_PIN = '71463c8b200d484437e758749899f6f5a2f5f1f560e8a5d48c63a01f4892c03d'
PAYLOAD_PIN = '49b096b3a7ef630f122b3577dda1878e69c3e9d2378a6a219c979a906cfb0f2b'
HELPER_PIN = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
PLAN_PIN = 'ef459d6abe21c866a423eaa6c6b2859287670c1a654e9e4631c08a6b735afded'
# Official NVIDIA Ubuntu 22.04 amd64 repository, verified before publication.
NSYS_URL = 'https://developer.download.nvidia.com/devtools/repos/ubuntu2204/amd64/NsightSystems-linux-cli-public-2025.3.1.90-3582212.deb'
NSYS_SHA256 = 'd2484ad0faf6831b11fa0bf73c54232d9ea8beafb50414019e6ba299c4ed5718'
NSYS_SIZE = 175210028
NSYS_RELATIVE_CLI = 'opt/nvidia/nsight-systems-cli/2025.3.1/target-linux-x64/nsys'
NSYS_VERSION = '2025.3.1.90'
EXPECTED_LOGICAL = dict(hash='5c785760053d17ce', sites=39885, K_effective=5,
    unique_keys=1306696, balls=1306696,
    tower_digest='67450c64611075b1', catalogue_digest='5ad1fe09354411ba',
    presentation_digest='a2aa4b20ca392dfe',
    presentations=[456919,691284,158496,456919,691284,158496],
    euler=dict(by_k=[1,1,1,151415,-560284],checkable_max_k=3,status='holds'),
    orders=[dict(K=k,births=b,contributions=b,merges=m,nodes=n,parents=n-1)
            for k,b,m,n in ((1,39885,39796,79681),(2,101089,77038,178127),
                (3,166228,119682,285910),(4,249493,172168,421661),(5,341081,235290,576371))])


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def pinned_artifacts(pins):
    observed = {}
    for name, expected in pins.items():
        path = Path(name)
        need(path.is_file() and not path.is_symlink(), 'existing regular qualified artifact: '+name)
        observed[name] = sha(path)
        need(observed[name] == expected, 'qualified artifact hash: '+name)
    return observed


def load_payload(root, manifest):
    name = 'gcp-migration/tower_worker_v9.py'
    need(manifest.get(name) == PAYLOAD_PIN and sha(root/name) == PAYLOAD_PIN, 'original FULL validator pin')
    need(manifest.get('gcp-migration/full_probe_worker_v7.py') == HELPER_PIN, 'original collector pin')
    spec = importlib.util.spec_from_file_location('full_nsys_pinned_payload', root/name)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def native_probe(raw, payload, case):
    values = []
    for line in raw.splitlines():
        if line.lstrip().startswith('{'):
            value = payload.strict_json(line)
            if value.get('schema') == payload.PROBE_SCHEMA:
                values.append(value)
    need(len(values) == 1, 'one complete native FULL JSON in command stdout')
    value = values[0]
    need(payload.validate_probe(value, case, 0) == 'complete_relative', 'native FULL validation')
    logical = json.loads(json.dumps(payload.logical_result(value)))
    need(logical == EXPECTED_LOGICAL, 'same FULL object as qualified historical capture')
    return value


def library_pins(text):
    need('not found' not in text, 'missing dynamic dependency')
    paths = set(re.findall(r'(?:=>\s*|^\s*)(/[^\s]+)\s+\(', text, re.MULTILINE))
    need(paths, 'dynamic dependency inventory')
    # libcuda is loaded dynamically and need not appear in ldd's list.
    driver = Path('/usr/lib/x86_64-linux-gnu/libcuda.so.1')
    if driver.is_file():
        paths.add(str(driver))
    return {str(Path(path).resolve()): sha(path) for path in sorted(paths)}


def profile_command(cli, report, argv):
    return [str(cli), 'profile', '--trace=cuda,osrt', '--sample=none', '--cpuctxsw=none',
            '--export=sqlite', '--stats=false', '--output='+str(report), *argv]


def sqlite_summary(path):
    need(path.is_file() and not path.is_symlink(), 'SQLite report exists')
    with sqlite3.connect(path.resolve().as_uri()+'?mode=ro', uri=True) as db:
        tables = {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
        kernels = sorted(tables & {'CUPTI_ACTIVITY_KIND_KERNEL','CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL'})
        counts = {name: db.execute('SELECT COUNT(*) FROM "'+name+'"').fetchone()[0] for name in kernels}
    need(sum(counts.values()) > 0, 'nonempty recorded CUDA kernel activity')
    return dict(kernel_tables=counts)


def build_recipe_pins(build):
    """Generated recipe + explicit external link files, before build; rsp optional."""
    paths = {build/'CMakeCache.txt'}
    for target in ('mhgp9_tower_probe','mhgp9_chain','mhgp9_gen','mhgp9_gpu'):
        folder = build/'CMakeFiles'/(target+'.dir')
        paths.update(folder/name for name in ('flags.make','build.make','link.txt','DependInfo.cmake'))
        paths.update(folder.rglob('*.rsp'))
        for token in shlex.split((folder/'link.txt').read_text()):
            path = Path(token)
            if path.is_absolute() and not path.is_relative_to(build) and path.is_file():
                paths.add(path)
    return {str(path):sha(path) for path in sorted(paths)}


def build_product_pins(build):
    paths = set(build.glob('CMakeFiles/*.dir/**/*.o')) | set(build.glob('CMakeFiles/*.dir/**/*.o.d'))
    paths.update(build.glob('libmhgp9_*.a'))
    need(paths and {build/('lib'+name+'.a') for name in ('mhgp9_chain','mhgp9_gen','mhgp9_gpu')} <= paths,
         'compiled objects and three native archives')
    return {str(path):sha(path) for path in sorted(paths)}


def run_preflights(command,output,binary,payload,cases):
    """The three original native preflights; same frames4 inherited from case0."""
    raw = payload.preflight_cloud()
    with (output/payload.PREFLIGHT_FILE).open('xb') as stream:stream.write(raw)
    case = payload.preflight_case(cases,raw)
    engine = dict(case,levers=payload.engine_levers(case['levers']))
    capacity,lanes_capacity = payload.deferral_capacities(case['levers'])
    values = {}
    for name,current,cap,judge,lanes in (
            ('preflight',case,0,payload.judged_preflight(case['levers']),0),
            ('preflight_engine',engine,0,False,0),
            ('preflight_deferral',case,capacity,True,lanes_capacity)):
        argv = [payload.TIME,'-v',str(binary),str(output/payload.PREFLIGHT_FILE),
                *payload.expected_probe_tail(current,cap,judge,lanes)]
        value = payload.strict_json(command(name,argv))
        need(payload.validate_probe(value,current,0,inputs=payload.preflight_inputs(raw),
             capacity=cap,judge=judge,lanes_capacity=lanes) == 'complete_relative',name+' complete')
        row = payload.strict_json((output/(name+'.command.json')).read_bytes())
        payload.validate_external_wall(value,row['elapsed_seconds'])
        payload.validate_gnu_time((output/(name+'.stderr')).read_text(),0)
        if name != 'preflight_deferral':payload.validate_preflight_work(value,current['levers'])
        values[name] = value
    reference = values['preflight']
    need(payload.device_ran(case,reference), 'GPU really ran in native preflight')
    need(all(payload.logical_result(value) == payload.logical_result(reference) and
             payload.certificate_work(value) == payload.certificate_work(reference) for value in values.values()),
         'preflight GPU/engine/deferral exact same object and certificate work')
    return dict(sites=case['n'],frames=case['frames'],tower_digest=reference['tower_digest'],
                GPU_executed=True,engine_equal=True,deferral_equal=True,
                deferred=values['preflight_deferral']['q34_batch']['deferred'],
                lanes_deferred=values['preflight_deferral']['q34_batch']['lanes_deferred'])


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    need(args.source_manifest.resolve() == root.parent/'source_manifest.json' and
         output == root.parent/'output' and not output.exists() and not output.is_symlink(), 'fresh fixed layout')
    output.mkdir(mode=0o700)
    started, started_epoch = time.monotonic(), time.time()
    state = dict(schema=SCHEMA,status='failed',scope='FULL_profile_diagnostic_only',engine_commit=ENGINE_COMMIT,
        contract_certified=False,public_status='not_claimed',FULL_executed=False,CUDA_profile_executed=False,
        CUDA_installation_attempted=False,system_installation_attempted=False,useful_budget_seconds=USEFUL_SECONDS,
        worker_sha256=sha(__file__),source_manifest_sha256=args.source_manifest_sha256,
        native_validator_sha256=PAYLOAD_PIN,collector_sha256=HELPER_PIN,generation=args.generation,commands=[],
        binary_origin='rebuilt_from_original_snapshot',GPU_preflight_executed=False,
        GPU_preflight_attempted=False,FULL_attempted=False,CUDA_profile_attempted=False,
        dependency_scope='native_postbuild_depfiles_pinned_before_execution_then_closed')
    collector, payload, before, libraries, nsys, tools_dir, previous = None, None, None, None, None, None, {}
    recipe,consumed,products,tools,tool_pins = {},{},{},{},{}
    try:
        need(args.source_manifest_sha256 == MANIFEST_PIN and sha(args.source_manifest) == MANIFEST_PIN,
             'original FULL snapshot manifest pin')
        manifest = json.loads(args.source_manifest.read_bytes())
        payload = load_payload(root, manifest)
        helper = payload.load_helper(root)
        state['target'] = dict(project=args.project,zone=args.zone,instance=args.instance)
        need(state['target'] == payload.TARGET and args.closing_margin_seconds >= 300, 'fixed target/margin')
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'guard mark pin')
        mark = helper.fields(args.guard_mark.read_text())
        need(mark['guest_shutdown_minutes'] == '30' and mark['max_run_seconds'] == '3600', 'fixed double guards')
        schedule = helper.fields(helper.scheduled_text())
        guards = helper.guard_values(mark,schedule,payload.TARGET,args.generation,args.session_deadline_epoch,
                                     args.closing_margin_seconds,time.time())
        collector = helper.Worker(output,min(started_epoch+USEFUL_SECONDS,guards['work_deadline_epoch']),schedule)
        def interrupted(signum, _frame):
            raise InterruptedError('diagnostic worker signal '+str(signum))
        for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):
            previous[sig] = signal.signal(sig,interrupted)
        observed = helper.metadata()
        need(all(observed[key] == value for key,value in payload.TARGET.items()) and
             observed['machine'] == 'g4-standard-48' and len(payload.available_cpus()) == 48 and
             abs(payload.boot_epoch()-helper.epoch(args.generation)) <= 300,'native G4 guest identity/CPU/generation')
        save(output/'guard_evidence.json',dict(mark=mark,schedule=schedule,metadata=observed))
        before = payload.source_map(root,manifest)
        save(output/'sources_before.json',before)
        need(sha(root/payload.PLAN) == PLAN_PIN, 'original six-case plan')
        payload.validate_sources(lambda name:(root/name).read_bytes())
        cases = payload.validate_plan(payload.strict_json((root/payload.PLAN).read_bytes()),manifest)
        case = cases[0]
        provenance = payload.validate_provenance(payload.strict_json((root/payload.PROVENANCE).read_bytes()),manifest)
        need(provenance['commit'] == ENGINE_COMMIT and provenance['protocol_source'] == 'commit', 'original committed FULL source')
        need(case['frames'] == 4 and case['levers']['q34_dead_core'] and
             not case['levers']['q3_interior_payload'] and not case['levers']['q34_lanes_fused'], 'original ON frames4 case')
        data = (root/case['file']).read_bytes()
        need(len(data) == 39885*12 and payload.input_fnv(data) == EXPECTED_LOGICAL['hash'], 'complete original ng00 input')
        tools_dir = root.parent/'tools';tools_dir.mkdir(mode=0o700)
        for name in ('tmp','config','cache','cuda-cache'):
            (tools_dir/name).mkdir(mode=0o700)
        os.environ.update(TMPDIR=str(tools_dir/'tmp'),XDG_CONFIG_HOME=str(tools_dir/'config'),
                          XDG_CACHE_HOME=str(tools_dir/'cache'),CUDA_CACHE_PATH=str(tools_dir/'cuda-cache'))
        def command(name, command_argv):
            row = collector.command(name,command_argv)
            need(row['exit_code'] == 0, 'command failed: '+name)
            return (output/(name+'.stdout')).read_text()
        nvcc = next((path for path in payload.CUDA_PATHS if Path(path).is_file() and os.access(path,os.X_OK)),None)
        tools = {'g++':shutil.which('g++'),'cmake':shutil.which('cmake'),'nvcc':nvcc,
                 'nvidia-smi':shutil.which('nvidia-smi')}
        need(all(tools.values()) and Path(payload.BOOST_HEADER).is_file() and Path(payload.TIME).is_file(),
             'existing native build tools, CUDA and Boost; no system installation')
        tool_pins = {str(Path(path).resolve()):sha(path) for path in (*tools.values(),payload.TIME)}
        state['tool_paths'] = tools;state['tool_hashes_before'] = tool_pins
        for name,argv in (('compiler',[tools['g++'],'--version']),('cmake_version',[tools['cmake'],'--version']),
                          ('nvcc_version',[tools['nvcc'],'--version']),
                          ('gpu_inventory',[tools['nvidia-smi'],'--query-gpu=name,driver_version,memory.total,compute_cap',
                                            '--format=csv,noheader'])):
            command(name,argv)
        need(payload.DEVICE_NAME in (output/'gpu_inventory.stdout').read_text(),'native G4 GPU inventory')
        build = root.parent/'build'
        need(not build.exists() and not build.is_symlink(),'fresh native build outside collected output')
        command('configure',payload.configure_command(tools,root,build))
        recipe = build_recipe_pins(build)
        save(output/'build_recipe_before.json',recipe)
        command('build',payload.build_command(tools,build,False))
        need(recipe == build_recipe_pins(build),'generated recipe/link inputs stable through build')
        binary = build/payload.PROBE_TARGET
        need(binary.is_file() and not binary.is_symlink() and os.access(binary,os.X_OK),'new executable after native build')
        consumed = payload.compiled_dependencies(build,root,before,False)
        save(output/'compiled_dependencies_before.json',consumed)
        products = build_product_pins(build)
        save(output/'build_products_before.json',products)
        shutil.copy2(binary,output/'qualified_probe')
        binary_sha = sha(binary)
        state['artifacts_before'] = pinned_artifacts({str(binary):binary_sha,str(output/'qualified_probe'):binary_sha,
                                                     str(root/case['file']):manifest[case['file']]})
        state['binary_sha256'] = binary_sha
        argv = payload.probe_command(build,root,case)
        state['native_argv'] = argv
        libraries = library_pins(command('binary_libraries',['ldd',str(binary)]))
        state['libraries_before'] = libraries
        state['GPU_preflight_attempted'] = True
        state['preflight'] = run_preflights(command,output,binary,payload,cases)
        state['GPU_preflight_executed'] = True
        need(NSYS_URL.startswith('https://developer.download.nvidia.com/') and
             re.fullmatch('[0-9a-f]{64}',NSYS_SHA256) and 0 < NSYS_SIZE <= 300*1024*1024 and
             NSYS_RELATIVE_CLI and not Path(NSYS_RELATIVE_CLI).is_absolute() and
             '..' not in Path(NSYS_RELATIVE_CLI).parts and NSYS_VERSION, 'verified official Nsight package metadata')
        deb = tools_dir/'nsight-systems-cli.deb'
        command('nsys_download',['curl','--fail','--location','--silent','--show-error','--proto','=https',
            '--max-time','120','--max-filesize',str(NSYS_SIZE),'--output',str(deb),NSYS_URL])
        need(deb.stat().st_size == NSYS_SIZE and sha(deb) == NSYS_SHA256, 'official package exact size/hash')
        command('nsys_extract',['dpkg-deb','-x',str(deb),str(tools_dir/'nsys')])
        nsys = tools_dir/'nsys'/NSYS_RELATIVE_CLI
        need(nsys.is_file() and nsys.resolve().is_relative_to(tools_dir/'nsys') and os.access(nsys,os.X_OK),
             'private Nsight executable')
        state['nsys'] = dict(url=NSYS_URL,package_sha256=NSYS_SHA256,package_bytes=NSYS_SIZE,
                            path=str(nsys),sha256=sha(nsys))
        version = command('nsys_version',[str(nsys),'--version'])
        need(NSYS_VERSION in version, 'expected Nsight version')
        help_text = command('nsys_profile_help',[str(nsys),'profile','--help'])
        need(all(option in help_text for option in ('--trace','--sample','--cpuctxsw','--export','--stats','--output')),
             'installed profile options')
        # Report names were verified against this exact package offline.
        # Its stats --help-reports normally returns 1; it is not a VM gate.
        state['FULL_attempted'] = True
        baseline = native_probe(command('unprofiled',argv),payload,case)
        payload.validate_external_wall(baseline,collector.commands[-1]['elapsed_seconds'])
        save(output/'unprofiled_probe.json',baseline)
        state['FULL_executed'] = True
        profile_argv = profile_command(nsys,output/'full_trace',argv)
        state['CUDA_profile_attempted'] = True
        profiled = native_probe(command('profile',profile_argv),payload,case)
        payload.validate_external_wall(profiled,collector.commands[-1]['elapsed_seconds'])
        save(output/'profile_probe.json',profiled)
        need(payload.logical_result(profiled) == payload.logical_result(baseline), 'profile preserves native FULL object')
        report, database = output/'full_trace.nsys-rep',output/'full_trace.sqlite'
        need(report.is_file() and not report.is_symlink() and report.stat().st_size > 0, 'nonempty Nsight report')
        state['activity'] = sqlite_summary(database)
        state['CUDA_profile_executed'] = True
        state['reports'] = {path.name:dict(sha256=sha(path),bytes=path.stat().st_size) for path in (report,database)}
        command('nsys_stats',[str(nsys),'stats','--report','cuda_gpu_kern_sum,cuda_gpu_mem_time_sum,cuda_api_sum,osrt_sum',
                '--format','csv',str(database)])
        state['reports'] = {path.name:dict(sha256=sha(path),bytes=path.stat().st_size) for path in (report,database)}
        collector.remaining()
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__+': '+str(error)
    finally:
        for sig in previous:
            signal.signal(sig,signal.SIG_IGN)
        try:
            if before is not None:
                after = payload.source_map(root,manifest)
                save(output/'sources_after.json',after)
                need(before == after, 'snapshot source closure')
                state['sources_stable'] = True
            if 'artifacts_before' in state:
                state['artifacts_after'] = pinned_artifacts(state['artifacts_before'])
                need(state['artifacts_before'] == state['artifacts_after'], 'qualified artifact closure')
            if recipe:
                need(recipe == build_recipe_pins(build),'generated recipe/link inputs closure')
                save(output/'build_recipe_after.json',recipe)
                state['build_recipe_stable'] = True
            if consumed:
                after_dependencies = payload.compiled_dependencies(build,root,before,False)
                need(consumed == after_dependencies,'native actual compiled dependency closure')
                save(output/'compiled_dependencies_after.json',after_dependencies)
                state['compiled_dependencies_stable'] = True
            if products:
                need(products == build_product_pins(build),'native compiled product closure')
                save(output/'build_products_after.json',products)
                state['build_products_stable'] = True
            if tool_pins:
                state['tool_hashes_after'] = {path:sha(path) for path in tool_pins}
                need(tool_pins == state['tool_hashes_after'],'native tool closure')
            if libraries is not None:
                state['libraries_after'] = {path:sha(path) for path in libraries}
                need(state['libraries_after'] == libraries, 'runtime library closure')
            if 'nsys' in state:
                need(sha(nsys) == state['nsys']['sha256'] and sha(tools_dir/'nsight-systems-cli.deb') == NSYS_SHA256,
                     'private Nsight tool/package closure')
                state['nsys_stable'] = True
        except BaseException as error:
            state.update(status='failed',closure_error=type(error).__name__+': '+str(error))
        if collector is not None:
            state['commands'] = collector.commands
        state['elapsed_seconds'] = time.monotonic()-started
        save(output/'receipt.json',state)
        for sig,handler in previous.items():
            signal.signal(sig,handler)
    print(json.dumps(dict(schema=SCHEMA,status=state['status'],scope=state['scope']),sort_keys=True))
    return 0 if state['status'] == 'completed' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root','source-manifest','guard-mark','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    for name in ('source-manifest-sha256','guard-mark-sha256','generation','project','zone','instance'):
        parser.add_argument('--'+name,required=True)
    parser.add_argument('--session-deadline-epoch',type=float,required=True)
    parser.add_argument('--closing-margin-seconds',type=int,required=True)
    raise SystemExit(execute(parser.parse_args()))
