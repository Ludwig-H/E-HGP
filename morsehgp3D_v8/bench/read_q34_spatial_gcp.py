#!/usr/bin/env python3
"""Read/archive a closed spatial34 G4 CPU capture locally; never contact GCP.

Explicit reuse of frozen spatial worker/session predicates and the older
read_cpu_probe_v8 raw-command/archive checks, not its scientific reader.
Scientific schema4 validation is loaded from the transported source snapshot.
Partial execution stays partial. Neither this reader nor the CPU producer
qualifies GPU execution, the full HGP tower, or a global complexity bound.
"""
import argparse
from contextlib import contextmanager
from copy import deepcopy
import hashlib
import importlib
import io
import json
import math
from pathlib import Path
import re
import shlex
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import q34_spatial_worker_v8 as payload
import q34_spatial_session_v8 as control
import read_cpu_probe_v8 as old

PINS = {
    'gcp-migration/q34_spatial_worker_v8.py': '39596c7607370e27d9b2e397e75dc4b7271469db58b26023406b1f7fa99d2b52',
    'gcp-migration/q34_spatial_session_v8.py': 'd830651eb34f2b970e28549b49b3fe48b61897f6b0b684d08e7c44464b8504ae',
    'gcp-migration/q34_spatial_snapshot_v8.py': '73764da1d14c94ec95f2d13585c90ee00770092e8155a8e30d429be023c07cd5',
    'gcp-migration/q34_spatial_selftest_v8.py': '70e9bf1a17ef32c3111e9d51db86c1dfd25093565b408a9960c9012ecbf1e482',
    'morsehgp3D_v8/bench/run_q34_spatial.py': '3a7a141cfdde2fd8baa0c197cdfab1419c16f181537f0230811b6e4d4d02c35c',
    'morsehgp3D_v8/bench/prepare_lidar_spatial.py': 'd5bc8af10dad6c14f304f5737b52f2c0a6e452ac9a674957bd909f062c32c898',
    'morsehgp3D_v8/tests/q34_spatial_gate.py': '3b5f39496f0f899ee4b5f4ff3f532ec3f37bcfd2ccf1439426f1f9d4cf0d57ca',
    'gcp-migration/read_cpu_probe_v8.py': '5d6f79e06a7a539d656cfa83c7fded4c631bae44efe48b2095eae1396ede554a',
    'gcp-migration/full_probe_session_v7.py': control.LEGACY_SHA,
    'gcp-migration/cpu_probe_session_v8.py': old.CONTROLLER_SHA,
    'gcp-migration/cpu_probe_worker_v8.py': old.WORKER_SHA,
}
need, sha, read_json = payload.need, payload.sha, old.read_json


def finite_number(text, positive=False):
    value = float(text)
    need(math.isfinite(value) and (value > 0 if positive else value >= 0), 'invalid nonnegative finite quantity')
    return value


def elapsed_seconds(text):
    fields = text.split(':')
    need(1 <= len(fields) <= 3 and all(re.fullmatch(r'\d+(?:\.\d+)?', f) for f in fields), 'GNU time elapsed syntax')
    values = [finite_number(f) for f in fields]
    need(all(v < 60 for v in values[1:]), 'GNU time elapsed component out of range')
    result = 0.0
    for value in values:
        result = 60*result + value
    return result


def gnu_time(raw):
    fields = {}
    for line in raw.splitlines():
        if ': ' in line:
            key, value = line.strip().rsplit(': ', 1)
            need(key not in fields, 'duplicate GNU time field')
            fields[key] = value
    user, system = (finite_number(fields[key]) for key in ('User time (seconds)', 'System time (seconds)'))
    wall = elapsed_seconds(fields['Elapsed (wall clock) time (h:mm:ss or m:ss)'])
    percent, memory = fields['Percent of CPU this job got'], fields['Maximum resident set size (kbytes)']
    need(percent.endswith('%') and re.fullmatch(r'\d+', memory) and fields['Exit status'] == '0', 'GNU time successful fields')
    return dict(user_seconds=user, system_seconds=system, wall_seconds=wall,
        reported_cpu_percent=finite_number(percent[:-1]), maximum_resident_set_KiB=int(memory),
        average_busy_logical_CPUs=(user+system)/wall if wall else None,
        scope='GNU_time_whole_native_process_all_threads_not_per_worker_time')


def expected_commands(remote, compiler, cases):
    root, build = Path(remote) / 'source', Path(remote) / 'output/build'
    flags = ['-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-pthread', '-I', str(root / 'morsehgp3D_v8/src')]
    objects = [str(build / (str(i)+'.o')) for i in range(len(payload.LIBRARY))]
    result = dict(compiler=[compiler, '--version'], cpu=['lscpu'], time=['/usr/bin/time', '--version'])
    for i, name in enumerate(payload.LIBRARY):
        result['compile_'+str(i)] = [compiler, *flags, '-MD', '-MF', str(build/(str(i)+'.d')),
                                    '-c', str(root/name), '-o', objects[i]]
    for name, source in payload.PROGRAMS.items():
        result['link_'+name] = [compiler, *flags, '-MD', '-MF', str(build/(name+'.d')),
                              str(root/source), *objects, '-o', str(build/name)]
    for i, name in enumerate(payload.GATES):
        result['gate_'+str(i)] = payload.gate_command(build, name)
    for i, case in enumerate(cases):
        if case['n']:
            result['probe_'+str(i)] = ['/usr/bin/time', '-v', *payload.probe_command(build, root, case)]
    return result


def case_states(cases, commands, completed_indices, empty_indices, worker_status):
    """A successful native exit is not by itself a producer validation receipt."""
    need(type(completed_indices) is list and type(empty_indices) is list and
         all(type(i) is int for i in completed_indices+empty_indices), 'case index types')
    completed, empty, result, stopped = [], [], [], False
    for i, case in enumerate(cases):
        name = 'probe_'+str(i)
        command = commands.get(name)
        if case['n'] == 0:
            need(command is None, 'empty dataset executed')
            if i in empty_indices:
                need(not stopped, 'empty case processed after earlier interruption')
                empty.append(i)
                status = 'empty'
            else:
                status, stopped = 'not_started', True
        elif command is None:
            status, stopped = 'not_started', True
        else:
            need(not stopped, 'case executed after a failed/missing earlier case')
            if command['exit_code'] == 0 and not command.get('residual_or_interrupted_group_killed'):
                need(i in completed_indices, 'native exit0 lacks producer geometry validation')
                completed.append(i)
                status = 'completed'
            else:
                status, stopped = 'failed', True
        result.append(dict(index=i, command=name if command is not None else None, case=case, status=status))
    need(completed_indices == completed and empty_indices == empty, 'case completion/empty ledger differs')
    need(worker_status in ('completed', 'failed') and
         (worker_status != 'completed' or all(r['status'] in ('completed','empty') for r in result)), 'false worker completion')
    return result


@contextmanager
def snapshot_validator(snapshot, manifest):
    payload.validate_manifest(manifest)
    # Exact qualified r1 protocol, not merely a self-declared archive hash.
    trusted = {*payload.PROTOCOL_NAMES,payload.SPATIAL_READER,payload.PREPARER,
               'morsehgp3D_v8/tests/q34_spatial_gate.py'}
    need(all(manifest.get(name)==PINS[name] for name in trusted), 'unqualified archived Python protocol')
    modules = {Path(n).stem for n in manifest if n.startswith('morsehgp3D_v8/bench/') and n.endswith('.py')}
    saved = {name: sys.modules.pop(name) for name in modules if name in sys.modules}
    try:
        with tempfile.TemporaryDirectory(prefix='q34-spatial-reader-') as temporary, tarfile.open(snapshot, 'r:*') as archive:
            observed, raw = {}, {}
            for member in archive.getmembers():
                need(member.isfile() and payload.safe_name(member.name) and member.name not in observed,
                     'unsafe or duplicate source archive member')
                value = archive.extractfile(member).read()
                observed[member.name] = hashlib.sha256(value).hexdigest()
                raw[member.name] = value
            need(observed == manifest, 'source snapshot hash/inventory differs')
            payload.validate_authority(manifest, raw.__getitem__)
            cases = payload.validate_plan(payload.strict_json(raw[payload.PLAN]), manifest)
            payload.validate_data(cases, raw.__getitem__)
            root, bench = Path(temporary), Path(temporary) / 'morsehgp3D_v8/bench'
            bench.mkdir(parents=True)
            for name, value in raw.items():
                if name.startswith('morsehgp3D_v8/bench/') and name.endswith('.py'):
                    (root/name).write_bytes(value)
            sys.path.insert(0, str(bench))
            try:
                reference = importlib.import_module('run_q34_spatial')
                payload.validate_preparations(manifest, cases, raw.__getitem__, reference)
                yield reference, cases, raw
            finally:
                sys.path.remove(str(bench))
    finally:
        for name in modules:
            sys.modules.pop(name, None)
        sys.modules.update(saved)


def worker_command_names(commands, expected):
    names = []
    for command in commands:
        matches = [name for name,argv in expected.items() if name not in names and command.get('argv')==argv]
        need(matches, 'unrecognized or excess worker command while selecting archive')
        # Explicit repetitions have identical argv and distinct numbered
        # probes; the producer appends these serially in plan order.
        names.append(matches[0])
    return names


def safe_host_files(host):
    need(host.is_dir() and not host.is_symlink() and host.name == 'q34_spatial_v8_host', 'exact host receipt directory required')
    fixed = {'snapshot.tar.gz','source_manifest.json','worker.py','receipt.json','handoff.json','lifecycle.txt','capture.tar.gz',
             *control.GUARDS,'guardmarks/guest_guard_pending','guardmarks/double_guard_verified'}
    host_names = {'before_start','oslogin_add','guarded_start','before_upload','guest_schedule','remote_mkdir',
                  'upload','unpack','before_worker','worker','before_retrieve','pack_capture','download','guarded_stop'}
    receipt = read_json(host/'receipt.json')
    need(type(receipt.get('commands')) is list, 'host command inventory absent')
    declared_host = []
    for command in receipt['commands']:
        need(type(command) is dict and command.get('name') in host_names, 'unknown declared host command')
        declared_host.append(command['name'])
    need(len(set(declared_host))==len(declared_host), 'duplicate declared host command')
    suffixes = ('.intent.json','.command.json','.stdout','.stderr')
    allowed = fixed | {name+suffix for name in declared_host for suffix in suffixes}
    worker = read_json(host/'received/output/receipt.json')
    need(type(worker.get('commands')) is list and type(worker.get('cases')) is list and
         type(worker.get('completed_case_indices')) is list, 'worker command/case inventory absent')
    compiler_rows = [row for row in worker['commands'] if type(row) is dict and type(row.get('argv')) is list and
                     len(row['argv'])==2 and row['argv'][1]=='--version' and Path(row['argv'][0]).name=='g++']
    need(len(compiler_rows)<=1, 'duplicate compiler command')
    compiler = compiler_rows[0]['argv'][0] if compiler_rows else '/usr/bin/g++'
    expected = expected_commands(receipt['remote_directory'],compiler,worker['cases'])
    declared_worker = worker_command_names(worker['commands'],expected)
    received = {'receipt.json','guard_evidence.json','sources_before.json','sources_after.json'}
    if 'compiled_dependency_manifest_sha256' in worker:
        received.add('compiled_dependencies.json')
    for index in worker['completed_case_indices']:
        need(type(index) is int and 0<=index<len(worker['cases']) and 'probe_'+str(index) in declared_worker,
             'summary without declared executed case')
        received.add('probe_'+str(index)+'.summary.json')
    received |= {name+suffix for name in declared_worker for suffix in suffixes}
    allowed |= {'received/output/'+name for name in received}
    files = []
    directories = set()
    for path in host.rglob('*'):
        need(not path.is_symlink(), 'host evidence link forbidden')
        relative = str(path.relative_to(host))
        if path.is_dir():
            directories.add(relative)
            continue
        need(path.is_file() and relative in allowed,
             'unapproved host artifact; never archive session parent/private key')
        files.append(path)
    need(directories=={'guardmarks','received','received/output'} and
         {str(path.relative_to(host)) for path in files}==allowed, 'host archive inventory missing/extra paths')
    return sorted(files)


def local_comparisons(path, rows, remote_manifest):
    # Local reader runs against its own genuine original locations/authority;
    # it is never made to pretend GCP commands ran on this host.
    reference = payload.load_validator(manifest=remote_manifest)
    local = reference.read(path, check_live=False)
    m = read_json(path/'MANIFEST.json')
    need(all(remote_manifest.get(n) == h for n,h in m['source_sha256'].items()), 'local/G4 native source differs')
    result = []
    for entry in local['records']:
        if entry['status'] != 'completed':
            continue
        a = entry['row']
        source = next(d for d in m['datasets'] if d['name'] == entry['dataset'])
        for remote in rows:
            b, case = remote['row'], remote['case']
            if all(a[k] == b[k] for k in ('n','input_hash','kmax','s','mask','q4_backend')):
                need(remote_manifest[case['file']] == m['input_sha256'][source['source']], 'local/G4 physical input differs')
                need(payload.logical_result(a) == payload.logical_result(b), 'local/G4 geometry or payload differs')
                ta,tb = (r['timings_ms']['pipeline_including_shared_preparation'] for r in (a,b))
                result.append(dict(dataset=entry['dataset'], scene=case['scene'], local_repeat=entry['repeat'],
                    remote_repeat=case['repeat'], n=a['n'], local_workers=a['workers'], G4_workers=b['workers'],
                    logical_work_equal=True, local_pipeline_ms=ta, G4_pipeline_ms=tb,
                    local_over_G4=ta/tb if tb else None,
                    normalized_only=['work.peak_edge_buffer_bytes','work.q3.peak_shell_bytes'],
                    scope='different_hosts_and_worker_counts_not_isolated_parallel_speedup'))
    return dict(path=str(path), capture_status='passed', comparisons=result, local_read=local)


def analyse(host, localcapture=None):
    host = host.resolve()
    need(all(sha(ROOT/name) == pin for name,pin in PINS.items()), 'frozen reader dependency changed')
    files = safe_host_files(host)
    dependency_paths = {ROOT/name for name in PINS} | {Path(__file__).resolve()}
    if localcapture is not None:
        dependency_paths.update(p for p in localcapture.rglob('*') if p.is_file())
        dependency_paths.update(ROOT/name for name in read_json(host/'source_manifest.json') if
            payload.safe_name(name) and name.startswith('morsehgp3D_v8/') and name.endswith('.py'))
    before = {str(path): sha(path) for path in [*files, *sorted(dependency_paths)]}
    host_receipt = read_json(host/'receipt.json')
    need(host_receipt.get('status') in ('completed','worker_failed','failed') and
         host_receipt.get('targeted_shutdown_certified') is True and host_receipt.get('capture_received') is True and
         host_receipt.get('capture_pack_exit_code') == 0 and host_receipt.get('worker_receipt_present') is True and
         host_receipt.get('target') == payload.TARGET and host_receipt.get('GPU_executed') is False and
         host_receipt.get('FULL_executed') is False and host_receipt.get('public_status') == 'not_claimed',
         'host not closed/stopped with a received worker receipt')
    need(host_receipt['controller_sha256'] == PINS['gcp-migration/q34_spatial_session_v8.py'] and
         host_receipt['worker_sha256'] == PINS['gcp-migration/q34_spatial_worker_v8.py'], 'executed protocol pins')
    commands = old.raw_commands(host, host_receipt['commands'], host=True)
    generation, remote = host_receipt['generation'], host_receipt['remote_directory']
    control.epoch(generation)
    need(re.fullmatch(r'/tmp/ehgp-q34-spatial-v8-[0-9a-f]{16}\.[A-Za-z0-9]{10}', remote), 'remote directory identity')
    stop = commands['guarded_stop']
    start = commands['guarded_start']
    origin = Path(start['argv'][0]).parent
    need(origin.is_absolute() and origin.name=='q34_spatial_v8_host' and '..' not in origin.parts and
         start['exit_code']==0 and start['argv']==[str(origin/'start_and_verify.sh'),'--yes',
         '--guest-shutdown-minutes','30','--handoff-file',str(origin/'handoff.json'),
         '--lifecycle-state-file',str(origin/'lifecycle.txt'),'--guard-mark-dir',str(origin/'guardmarks')],
         'exact guarded start command')
    need(stop['exit_code'] == 0 and stop['argv'][0] == str(origin/'stop_and_verify.sh') and
         stop['argv'][1:] == ['--yes','--expected-last-start-timestamp',generation], 'targeted exact-generation stop')
    for name,pin in control.GUARDS.items():
        need(sha(host/name) == pin, 'guard source pin')
    need(control.generation_from_records(read_json(host/'handoff.json'),
         control.fields((host/'lifecycle.txt').read_text())) == generation, 'handoff generation')
    mark = control.fields((host/'guardmarks/double_guard_verified').read_text())
    need(mark.get('generation')==generation and mark.get('max_run_seconds')=='3600' and
         mark.get('guest_shutdown_minutes')=='30' and all(mark.get(k)==v for k,v in payload.TARGET.items()),
         'double-guard generation/duration/target')
    for name in ('before_upload','before_worker','before_retrieve'):
        need(commands[name]['exit_code'] == 0, 'generation recertification failed')
        control.validate_target(read_json(host/(name+'.stdout')), 'RUNNING', generation)
    manifest = read_json(host/'source_manifest.json')
    need(sha(host/'source_manifest.json') == host_receipt['manifest_sha256'] and
         sha(host/'snapshot.tar.gz') == host_receipt['snapshot_sha256'] and
         sha(host/'worker.py') == host_receipt['worker_sha256'] and
         all(manifest[n] == PINS[n] for n in ('gcp-migration/q34_spatial_worker_v8.py',
                                            'gcp-migration/q34_spatial_session_v8.py')), 'transport source pins')
    output = old.match_capture_archive(host, host_receipt)
    worker = read_json(output/'receipt.json')
    need(worker.get('status') in ('completed','failed') and worker.get('target') == payload.TARGET and
         worker.get('generation') == generation and worker.get('worker_sha256') == host_receipt['worker_sha256'] and
         worker.get('source_manifest_sha256') == host_receipt['manifest_sha256'] and worker.get('sources_stable') is True and
         worker.get('GPU_executed') is False and worker.get('FULL_executed') is False and
         worker.get('contract_certified') is False and worker.get('useful_budget_seconds') == 900 and
         len(worker.get('available_cpus',[])) == 48, 'worker scope/source closure')
    need((worker['status'] == 'completed') == (host_receipt['status'] == 'completed'), 'host/worker success disagrees')
    worker_call = commands['worker']
    need(worker_call['exit_code']==host_receipt['worker_exit_code'], 'worker host exit binding')
    scripts = [value[len('--command='):] for value in worker_call['argv'] if value.startswith('--command=')]
    expected_worker = ['exec','python3',remote+'/worker.py','--source-root',remote+'/source',
        '--source-manifest',remote+'/source_manifest.json','--source-manifest-sha256',host_receipt['manifest_sha256'],
        '--guard-mark',remote+'/double_guard_verified','--guard-mark-sha256',sha(host/'guardmarks/double_guard_verified'),
        '--generation',generation,'--session-deadline-epoch',str(host_receipt['session_deadline_epoch']),
        '--closing-margin-seconds','300','--output',remote+'/output']
    for key,value in payload.TARGET.items(): expected_worker += ['--'+key,value]
    expected_worker += ['--execute','--useful-budget-seconds','900']
    need(len(scripts)==1 and shlex.split(scripts[0])==expected_worker and worker['worker_argv']==expected_worker[2:],
         'actual worker invocation/receipt differs')
    for name in ('sources_before.json','sources_after.json'):
        need(read_json(output/name) == manifest, 'worker source before/after differs')
    work_commands = old.raw_commands(output, worker['commands'])
    failed_commands = [dict(name=n, exit_code=r['exit_code'], session_deadline_reached=r.get('session_deadline_reached',False))
                       for n,r in work_commands.items() if r['exit_code'] != 0 or r.get('residual_or_interrupted_group_killed')]
    dependency_complete = worker.get('compiled_dependencies_stable') is True
    if dependency_complete:
        need(sha(output/'compiled_dependencies.json') == worker['compiled_dependency_manifest_sha256'], 'compiled dependency hash')
        for name,pin in read_json(output/'compiled_dependencies.json').items():
            need(type(pin) is str and re.fullmatch('[0-9a-f]{64}',pin) and Path(name).is_absolute(), 'compiled dependency type')
            if name.startswith(remote+'/source/'):
                need(manifest.get(name[len(remote+'/source/'):]) == pin, 'compiled unmanifested source')
            else:
                need(name.startswith('/usr/'), 'unexpected external header dependency')
    rows, gates = [], []
    with snapshot_validator(host/'snapshot.tar.gz', manifest) as (reference,cases,_raw):
        need(worker['cases'] == cases and worker['plan_schema'] == 'mhgp8_q34_spatial_plan_v1', 'worker plan differs')
        compiler = work_commands.get('compiler',{}).get('argv',['/usr/bin/g++'])[0]
        need(Path(compiler).is_absolute() and Path(compiler).name == 'g++', 'compiler path')
        expected = expected_commands(remote,compiler,cases)
        need(set(work_commands) <= set(expected) and all(r['argv'] == expected[n] and r['cwd'] == remote+'/output' and
             r['session_work_deadline_epoch'] == worker['useful_deadline_epoch'] and r['per_command_watchdog'] is None
             for n,r in work_commands.items()), 'native commands/deadlines differ from plan')
        preparation = {n for n in expected if not n.startswith(('gate_','probe_'))}
        prepared = preparation <= set(work_commands) and all(work_commands[n]['exit_code']==0 for n in preparation)
        for i,name in enumerate(payload.GATES):
            command = work_commands.get('gate_'+str(i))
            if command is not None and command['exit_code'] == 0:
                reference.checks.validate_gate(read_json(output/('gate_'+str(i)+'.stdout')),name)
                gates.append(name)
        states = case_states(cases,work_commands,worker['completed_case_indices'],worker['empty_cases'],worker['status'])
        for entry in states:
            if entry['status'] != 'completed':
                continue
            need(prepared and dependency_complete and len(gates)==4 and set(worker['binaries'])==set(payload.PROGRAMS),
                 'scientific result without complete build/gates/dependencies')
            name,case = entry['command'],entry['case']
            row = read_json(output/(name+'.stdout'))
            payload.validate_probe(row,case,work_commands[name]['argv'][2:],reference)
            need(read_json(output/(name+'.summary.json')) == dict(case=case,status='completed',
                 input_file_sha256=manifest[case['file']],output=row['output']), 'producer summary differs')
            rows.append(dict(**entry,row=row,gnu_time=gnu_time((output/(name+'.stderr')).read_text())))
        need({p.name for p in output.glob('probe_*.summary.json')} == {r['command']+'.summary.json' for r in rows},
             'summary inventory differs')
    local = local_comparisons(localcapture,rows,manifest) if localcapture is not None else None
    need(safe_host_files(host) == files, 'host artifact inventory changed')
    after = {name:sha(name) for name in before}
    need(before == after, 'evidence changed while reading')
    return dict(schema='mhgp8_q34_spatial_gcp_read_v1', validation_status='passed',
        status='validated_complete' if worker['status']=='completed' else 'validated_partial',
        host_status=host_receipt['status'], worker_status=worker['status'], worker_error=worker.get('error'),
        host_error=host_receipt.get('error'), target=payload.TARGET,generation=generation,
        targeted_shutdown_certified=True, gate_names_passed=gates,compiled_dependencies_closed=dependency_complete,
        case_states=states,measurements=rows,failed_commands=failed_commands,completed=len(rows),planned=len(cases),
        local_comparison=local,hashes_before=before,hashes_after=after,
        scope='closed_CPU_q3_q4_candidate_stream_not_FULL_tower',GPU_executed=False,FULL_executed=False,
        full_contract_qualified=False,universal_subquadratic_claim=False), files


class PureTests(unittest.TestCase):
    def test_unqualified_archived_code_rejected_before_loading(self):
        required = set(payload.LIBRARY)|set(payload.PROGRAMS.values())|payload.SUPPORT_SOURCES|payload.PROTOCOL_NAMES|{
            payload.HELPER,payload.PLAN,payload.PREPARATIONS,payload.SPATIAL_READER,payload.PREPARER,*payload.AUTHORITY_PINS,
            'morsehgp3D_v8/tests/q34_spatial_gate.py'}
        manifest = {name:'0'*64 for name in required}
        manifest.update(payload.AUTHORITY_PINS)
        manifest[payload.HELPER]=payload.HELPER_SHA
        trusted = {*payload.PROTOCOL_NAMES,payload.SPATIAL_READER,payload.PREPARER,
                   'morsehgp3D_v8/tests/q34_spatial_gate.py'}
        manifest.update({name:PINS[name] for name in trusted})
        payload.validate_manifest(manifest)
        for name in trusted:
            bad = dict(manifest,**{name:'0'*64})
            with self.subTest(name=name), patch.object(tarfile,'open',side_effect=AssertionError('archive opened too early')):
                with self.assertRaises(ValueError):
                    with snapshot_validator(Path('/never-open'),bad): pass

    def test_no_private_key_archive(self):
        with tempfile.TemporaryDirectory() as temporary:
            host = Path(temporary)/'q34_spatial_v8_host'
            fixed = {'snapshot.tar.gz','source_manifest.json','worker.py','receipt.json','handoff.json','lifecycle.txt',
                'capture.tar.gz',*control.GUARDS,'guardmarks/guest_guard_pending','guardmarks/double_guard_verified',
                'received/output/receipt.json','received/output/guard_evidence.json',
                'received/output/sources_before.json','received/output/sources_after.json'}
            for name in fixed:
                path = host/name
                path.parent.mkdir(parents=True,exist_ok=True)
                path.write_text('{}')
            (host/'receipt.json').write_text(json.dumps(dict(commands=[],remote_directory='/tmp/fixture')))
            (host/'received/output/receipt.json').write_text(json.dumps(dict(commands=[],cases=[],completed_case_indices=[])))
            self.assertEqual(len(safe_host_files(host)),len(fixed))
            for name in ('session_key','secret.stdout','guardmarks/session_key','received/output/session_key',
                         'received/output/secret.stdout','received/output/rogue.command.json',
                         'received/output/nested/secret.stdout'):
                path = host/name
                path.parent.mkdir(parents=True,exist_ok=True)
                path.write_text('fixture-not-a-real-key')
                with self.subTest(name=name),self.assertRaises(ValueError): safe_host_files(host)
                path.unlink()
                if path.parent.name=='nested': path.parent.rmdir()
            (host/'unexpected_empty_directory').mkdir()
            with self.assertRaises(ValueError): safe_host_files(host)

    def test_time(self):
        raw = '\n'.join(('User time (seconds): 220.58','System time (seconds): 0.00',
            'Elapsed (wall clock) time (h:mm:ss or m:ss): 0:59.27','Percent of CPU this job got: 372%',
            'Maximum resident set size (kbytes): 4608','Exit status: 0'))
        self.assertAlmostEqual(gnu_time(raw)['average_busy_logical_CPUs'],220.58/59.27)
        self.assertEqual(elapsed_seconds('1:02:03.5'),3723.5)
        for bad in (raw+'\nExit status: 0',raw.replace('220.58','NaN'),raw.replace('Exit status: 0','Exit status: 1'),
                    raw.replace('372%','inf%'),raw.replace('0:59.27','0:61.27')):
            with self.assertRaises(ValueError): gnu_time(bad)

    def test_partial_cases(self):
        cases = [{'n':3},{'n':3},{'n':3}]
        commands = {'probe_0':{'exit_code':0},'probe_1':{'exit_code':-9,'residual_or_interrupted_group_killed':True}}
        self.assertEqual([r['status'] for r in case_states(cases,commands,[0],[],'failed')],
                         ['completed','failed','not_started'])
        for completed,status in (([], 'failed'),([0],'completed'),([True],'failed')):
            with self.assertRaises(ValueError): case_states(cases,commands,completed,[],status)
        with self.assertRaises(ValueError): case_states(cases,{'probe_1':{'exit_code':0}},[1],[],'failed')
        self.assertEqual(case_states([{'n':0}],{},[],[0],'completed')[0]['status'],'empty')
        self.assertEqual(case_states([{'n':0}],{},[],[],'failed')[0]['status'],'not_started')

    def test_recipe_and_projection(self):
        case = dict(scene='s',dataset='full',file='data/s/full.u16le',n=3,input_hash=1,k=5,s=8,workers=48,repeat=0)
        commands = expected_commands('/tmp/example','/usr/bin/g++',[case])
        self.assertEqual(sum(n.startswith('compile_') for n in commands),24)
        self.assertEqual(sum(n.startswith('gate_') for n in commands),4)
        self.assertEqual(commands['probe_0'][-5:],['rectangle-pair','boxes','affine','live','64'])
        repeated = expected_commands('/tmp/example','/usr/bin/g++',[case,dict(case,repeat=1)])
        self.assertEqual(worker_command_names([{'argv':repeated['probe_0']},{'argv':repeated['probe_1']}],repeated),
                         ['probe_0','probe_1'])
        with self.assertRaises(ValueError):
            worker_command_names([{'argv':repeated['probe_0']}]*3,repeated)
        a = dict(input_hash=1,output={},front={},cloud_work={},index_work={},
                 work={'peak_edge_buffer_bytes':1,'q3':{'peak_shell_bytes':1,'seeds':2}})
        b = deepcopy(a)
        b['work']['peak_edge_buffer_bytes']=9
        b['work']['q3']['peak_shell_bytes']=10
        self.assertEqual(payload.logical_result(a),payload.logical_result(b))
        b['work']['q3']['seeds']+=1
        self.assertNotEqual(payload.logical_result(a),payload.logical_result(b))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--localcapture',type=Path)
    parser.add_argument('--selftest',action='store_true')
    args = parser.parse_args()
    if args.selftest:
        return 0 if unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(PureTests)).wasSuccessful() else 1
    need(args.host is not None and args.output is not None,'--host and --output required')
    output = args.output.absolute()
    need(not output.exists() and not output.is_symlink() and not output.resolve().is_relative_to(args.host.resolve()),'fresh output outside host')
    output.mkdir(parents=True)
    report = dict(validation_status='failed',error=None)
    try:
        report,files = analyse(args.host,args.localcapture.resolve() if args.localcapture else None)
        archive = output/'host_evidence.tar.gz'
        with tarfile.open(archive,'x:gz') as target:
            for path in files:
                raw = path.read_bytes()
                need(hashlib.sha256(raw).hexdigest()==report['hashes_before'][str(path)],'evidence changed before archive')
                member = tarfile.TarInfo('q34_spatial_v8_host/'+str(path.relative_to(args.host.resolve())))
                member.size,member.mode,member.mtime=len(raw),0o444,0
                target.addfile(member,io.BytesIO(raw))
        report['archive_sha256']=sha(archive)
        with tarfile.open(archive,'r:*') as archived:
            actual = {member.name:hashlib.sha256(archived.extractfile(member).read()).hexdigest()
                      for member in archived.getmembers() if member.isfile()}
        expected_archive = {'q34_spatial_v8_host/'+str(path.relative_to(args.host.resolve())):
                            report['hashes_before'][str(path)] for path in files}
        need(actual==expected_archive,'archive readback differs')
        report['archive_scope']='whitelisted_host_only_never_parent_or_SSH_private_key'
        need(all(sha(name)==pin for name,pin in report['hashes_before'].items()),'evidence changed at final closure')
    except BaseException as cause:
        report.update(validation_status='failed',error=f'{type(cause).__name__}: {cause}')
    with (output/'READBACK.json').open('x') as stream:
        json.dump(report,stream,sort_keys=True,indent=2,allow_nan=False)
        stream.write('\n')
    print(json.dumps(dict(output=str(output),validation_status=report['validation_status'],
                         status=report.get('status'),completed=report.get('completed'),error=report.get('error'))))
    return 0 if report['validation_status']=='passed' else 1


if __name__=='__main__':
    raise SystemExit(main())
