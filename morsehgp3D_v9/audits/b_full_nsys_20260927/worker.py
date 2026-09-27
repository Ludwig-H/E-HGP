#!/usr/bin/env python3
"""One diagnostic of the already qualified FULL binary; no build or VM management."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import sqlite3
import sys
import time

sys.dont_write_bytecode = True
SCHEMA = 'mhgp9_full_nsys_diagnostic_v1'
USEFUL_SECONDS = 300
ENGINE_COMMIT = 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a'
OLD_ROOT = Path('/tmp/ehgp-tower-v9-4e3fa578d9c95e0b.jLEtLeGGhi')
BINARY = OLD_ROOT/'output/build/mhgp9_tower_probe'
INPUT = OLD_ROOT/'source/data/scene_00.u32le'
ARTIFACT_PINS = {
    str(BINARY): '3a3798623cf3fa867a1da191580381394bdb48231098155f9c1f3610aee68ab4',
    str(INPUT): '0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf',
}
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


def pinned_artifacts(pins=ARTIFACT_PINS):
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
        native_validator_sha256=PAYLOAD_PIN,collector_sha256=HELPER_PIN,generation=args.generation,commands=[])
    collector, payload, before, libraries, nsys, tools_dir, previous = None, None, None, None, None, None, {}
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
        save(output/'guard_evidence.json',dict(mark=mark,schedule=schedule))
        collector = helper.Worker(output,min(started_epoch+USEFUL_SECONDS,guards['work_deadline_epoch']),schedule)
        def interrupted(signum, _frame):
            raise InterruptedError('diagnostic worker signal '+str(signum))
        for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):
            previous[sig] = signal.signal(sig,interrupted)
        before = payload.source_map(root,manifest)
        save(output/'sources_before.json',before)
        state['artifacts_before'] = pinned_artifacts()
        need(INPUT.stat().st_size == 39885*12 and os.access(BINARY,os.X_OK), 'executable and complete ng00')
        need(sha(root/payload.PLAN) == PLAN_PIN, 'original six-case plan')
        case = payload.validate_plan(payload.strict_json((root/payload.PLAN).read_bytes()),manifest)[0]
        need(case['frames'] == 4 and case['levers']['q34_dead_core'], 'original ON frames4 case')
        argv = payload.probe_command(BINARY.parent,OLD_ROOT/'source',case)
        need(argv[:2] == [str(BINARY),str(INPUT)], 'direct qualified executable and input')
        state['native_argv'] = argv
        def command(name, command_argv):
            row = collector.command(name,command_argv)
            need(row['exit_code'] == 0, 'command failed: '+name)
            return (output/(name+'.stdout')).read_text()
        libraries = library_pins(command('binary_libraries',['ldd',str(BINARY)]))
        state['libraries_before'] = libraries
        command('gpu_inventory',['nvidia-smi'])
        need(NSYS_URL.startswith('https://developer.download.nvidia.com/') and
             re.fullmatch('[0-9a-f]{64}',NSYS_SHA256) and 0 < NSYS_SIZE <= 300*1024*1024 and
             NSYS_RELATIVE_CLI and not Path(NSYS_RELATIVE_CLI).is_absolute() and
             '..' not in Path(NSYS_RELATIVE_CLI).parts and NSYS_VERSION, 'verified official Nsight package metadata')
        tools_dir = root.parent/'tools';tools_dir.mkdir(mode=0o700)
        for name in ('tmp','config','cache','cuda-cache'):
            (tools_dir/name).mkdir(mode=0o700)
        os.environ.update(TMPDIR=str(tools_dir/'tmp'),XDG_CONFIG_HOME=str(tools_dir/'config'),
                          XDG_CACHE_HOME=str(tools_dir/'cache'),CUDA_CACHE_PATH=str(tools_dir/'cuda-cache'))
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
        baseline = native_probe(command('unprofiled',argv),payload,case)
        save(output/'unprofiled_probe.json',baseline)
        state['FULL_executed'] = True
        profile_argv = profile_command(nsys,output/'full_trace',argv)
        profiled = native_probe(command('profile',profile_argv),payload,case)
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
                state['artifacts_after'] = pinned_artifacts()
                need(state['artifacts_before'] == state['artifacts_after'], 'qualified artifact closure')
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
