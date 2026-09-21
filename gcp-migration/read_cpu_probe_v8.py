#!/usr/bin/env python3
"""Read a CLOSED guarded v8 CPU capture, including an interrupted prefix.

No cloud call, worker execution or capture mutation. Scientific dependencies
come from the SHA-checked source snapshot. The global validator was NOT shipped
in that 196-source snapshot: its exact separately archived revision is pinned
below and reported as a posthoc validation dependency, never producer evidence.
CPU candidate streams are neither GPU execution nor the FULL tower contract.
"""
import argparse
import base64
from contextlib import contextmanager
from copy import deepcopy
import hashlib
import importlib
import json
from pathlib import Path
import re
import shlex
import struct
import sys
import tarfile
import tempfile
import unittest

import cpu_probe_session_v8 as control
import cpu_probe_worker_v8 as payload

need, sha, read_json = payload.need, payload.sha, lambda path: payload.strict_json(Path(path).read_text())
CONTROLLER_SHA = 'e402e931da80104af94ef8dce25168160d35ef5c2792b8e18e00106456421dfc'
WORKER_SHA = 'b8385feb20c82485fc4347c45157e58abff786060cbd57a1a73355e3c6f073cd'
VALIDATOR_SHA = 'ad76a103dac4baeb16cf9be1d9a6d9ddea6770692e803d6720fb97312521676c'
VALIDATOR_ARCHIVE = Path(__file__).resolve().parent / 'receipts/cpu_probe_v8_20260921/posthoc_sources/run_wspd_q34_lidar.py'


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def raw_commands(directory, declared, host=False):
    need(type(declared) is list and declared, 'command list absent')
    observed = {}
    for path in sorted(directory.glob('*.command.json')):
        name = path.name[:-len('.command.json')]
        row, intent = read_json(path), read_json(directory / (name+'.intent.json'))
        need(type(row.get('argv')) is list and row['argv'] and all(type(x) is str for x in row['argv']), 'argv type')
        immutable = ('argv', 'started_epoch', 'timeout_seconds') if host else (
            'argv', 'cwd', 'started_utc', 'session_work_deadline_epoch', 'per_command_watchdog')
        need(all(key in row and intent.get(key) == row[key] for key in immutable), 'intent/final command mismatch')
        if host:
            need(row.get('name') == name and intent.get('name') == name, 'host command name mismatch')
        need(type(row.get('exit_code')) is int and row.get('group_closed') is True and
             not row.get('last_resort_group_killed'), 'command process group not closed')
        for kind in ('stdout', 'stderr'):
            need(sha(directory / (name+'.'+kind)) == row.get(kind+'_sha256'), 'raw command hash mismatch')
        observed[name] = row
    need(sorted(map(canonical, observed.values())) == sorted(map(canonical, declared)), 'raw command inventory differs')
    return observed


def input_fingerprint(raw, n):
    need(type(n) is int and 0 < n <= len(raw)//6 and len(raw) % 6 == 0, 'input prefix size')
    points = list(struct.iter_unpack('<HHH', raw[:6*n]))
    need(len(set(points)) == n, 'duplicate quantized prefix site')
    result = 14695981039346656037
    for value in (n, *(coordinate for point in points for coordinate in point)):
        for byte in value.to_bytes(8, 'little'):
            result = ((result ^ byte) * 1099511628211) & ((1 << 64)-1)
    return result


def bind_probe(row, argv, case, remote, raw):
    expected = ['/usr/bin/time', '-v', remote+'/output/build/mhgp8_wspd_q34_probe',
                remote+'/source/'+case['file'],
                *[str(case[key]) for key in ('n', 'k', 's', 'mask', 'backend', 'workers')], 'samples', 'digest']
    need(argv == expected, 'raw probe command differs from exact plan')
    need(row.get('source_n') == len(raw)//6 and row.get('input_hash') == input_fingerprint(raw, case['n']),
         'source size or independent FNV prefix hash differs')
    # Existing strict validator expects the original n8000.u16le basename.
    # Only after binding REAL argv, file bytes/hash and source_n, adapt that
    # documentary alias. Never pretend this synthetic path was executed.
    scientific_argv = list(argv[2:])
    scientific_argv[1] = str(Path(scientific_argv[1]).with_name('n'+str(len(raw)//6)+'.u16le'))
    return scientific_argv


@contextmanager
def validator_from_snapshot(snapshot, manifest):
    names = {Path(name).stem for name in manifest if
             name.startswith('morsehgp3D_v8/bench/') and name.endswith('.py')}
    names.add('run_wspd_q34_lidar')
    need(sha(VALIDATOR_ARCHIVE) == VALIDATOR_SHA, 'external posthoc validator pin')
    saved = {name: sys.modules.pop(name) for name in names if name in sys.modules}
    with tempfile.TemporaryDirectory(prefix='mhgp8-closed-reader-') as temporary:
        root = Path(temporary)
        bench = root / 'morsehgp3D_v8/bench'
        bench.mkdir(parents=True)
        with tarfile.open(snapshot, 'r:*') as archive:
            for name in manifest:
                if name.startswith('morsehgp3D_v8/bench/') and name.endswith('.py'):
                    raw = archive.extractfile(name).read()
                    need(hashlib.sha256(raw).hexdigest() == manifest[name], 'validator source pin')
                    (root / name).write_bytes(raw)
        external = bench / 'run_wspd_q34_lidar.py'
        need(not external.exists() or sha(external) == VALIDATOR_SHA, 'snapshot/external validator ambiguity')
        external.write_bytes(VALIDATOR_ARCHIVE.read_bytes())
        sys.path.insert(0, str(bench))
        try:
            yield importlib.import_module('run_wspd_q34_lidar')
        finally:
            sys.path.remove(str(bench))
            for name in names:
                sys.modules.pop(name, None)
            sys.modules.update(saved)


def expected_worker_commands(remote, compiler, cases):
    source, build = remote+'/source/', remote+'/output/build/'
    flags = ['-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-pthread', '-I', source+'morsehgp3D_v8/src']
    objects = [build+str(i)+'.o' for i in range(len(payload.LIBRARY))]
    commands = {'compiler': [compiler, '--version'], 'cpu': ['lscpu'], 'time': ['/usr/bin/time', '--version']}
    for i, name in enumerate(payload.LIBRARY):
        commands['compile_'+str(i)] = [compiler, *flags, '-MD', '-MF', build+str(i)+'.d',
                                       '-c', source+name, '-o', objects[i]]
    for name, filename in payload.PROGRAMS.items():
        commands['link_'+name] = [compiler, *flags, '-MD', '-MF', build+name+'.d', source+filename,
                                 *objects, '-o', build+name]
    commands['gate92'] = [build+'mhgp8_wspd_q34_gate', '--selftest']
    for i, case in enumerate(cases):
        commands['probe_'+str(i)] = ['/usr/bin/time', '-v', build+'mhgp8_wspd_q34_probe', source+case['file'],
            *[str(case[key]) for key in ('n', 'k', 's', 'mask', 'backend', 'workers')], 'samples', 'digest']
    return commands


def match_capture_archive(host, receipt):
    archive_path = host / 'capture.tar.gz'
    need(sha(archive_path) == receipt.get('capture_sha256'), 'received archive hash')
    output, observed = host / 'received/output', set()
    with tarfile.open(archive_path, 'r:*') as archive:
        for member, name in control.legacy.archive_members(archive, 'output'):
            if member.isfile():
                raw = archive.extractfile(member).read()
                relative = str(name.relative_to('output'))
                local = output / relative
                need(local.is_file() and not local.is_symlink() and local.resolve().is_relative_to(output.resolve()) and
                     hashlib.sha256(raw).hexdigest() == sha(local), 'received file differs from archive')
                observed.add(relative)
    need({str(path.relative_to(output)) for path in output.rglob('*') if path.is_file()} == observed,
         'received output has missing/extra files')
    return output


def local_completed_record(path, reference, remote_manifest, remote_inputs):
    """One completed command from a closed possibly interrupted local campaign.

    This does not promote the containing campaign to PASS. Its exact overall
    status, failed records and close hashes remain unchanged and are reported.
    """
    path = path.resolve()
    directory = path.parent
    manifest, completion = read_json(directory/'MANIFEST.json'), read_json(directory/'COMPLETION.json')
    need(manifest.get('schema') == reference.SCHEMA and manifest.get('gcp_used') is False and
         manifest.get('public_status') == 'not_claimed' and manifest.get('full_contract_qualified') is False and
         completion.get('status') in ('passed', 'failed') and completion.get('closing_errors') == [] and
         sha(directory/'MANIFEST.json') == completion.get('manifest_sha256'), 'local campaign close identity')
    for key in ('source_sha256', 'artifact_sha256', 'input_sha256'):
        need(manifest.get(key) == completion.get(key+'_after') and type(manifest[key]) is dict and
             all(type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin) for pin in manifest[key].values()),
             'local captured hash closure')
    need(set(manifest['source_sha256']) == reference.SOURCES and
         manifest['source_sha256']['morsehgp3D_v8/bench/run_wspd_q34_lidar.py'] == VALIDATOR_SHA,
         'local source inventory/validator')
    for name, pin in manifest['source_sha256'].items():
        if name.startswith('morsehgp3D_v8/src/') or (name.startswith('morsehgp3D_v8/bench/') and name.endswith(('.hpp', '.cpp'))):
            need(remote_manifest.get(name) == pin, 'local/remote compiled product source differs')
    items = completion.get('records')
    need(type(items) is list and items and len({item['path'] for item in items}) == len(items), 'local record inventory')
    need({p.name for p in directory.glob('record_*.json')} == {item['path'] for item in items}, 'local record extras/missing')
    found = None
    for index, item in enumerate(items):
        need(item['path'] == f'record_{index:04}.json' and sha(directory/item['path']) == item['sha256'], 'local record pin')
        if item['path'] == path.name:
            found = index
    need(found is not None, 'local selected record not in closure')
    record = read_json(path)
    command = manifest['commands'][found]
    build = Path(manifest['build'])
    need(build.is_absolute() and build.parent.name == 'build' and '..' not in build.parts and
         command[0] == str(build/'mhgp8_wspd_q34_probe'), 'local build/command binding')
    need(record.get('command') == command and record.get('status') == 'passed' and record.get('exit_code') == 0 and
         type(record['exit_code']) is int and record.get('stderr') == '' and record.get('cwd') == str(build.parent.parent) and
         record.get('environment') == manifest['environment'], 'local selected command failed or unbound')
    for stream in ('stdout', 'stderr'):
        need(base64.b64decode(record[stream+'_base64'], validate=True).decode('utf-8', errors='replace') == record[stream],
             'local raw/decoded result differs')
    row = payload.strict_json(record['stdout'])
    reference.validate(row, command)
    dataset = next((d for d in manifest['datasets'] if d['source'] == command[1] and d['n'] == row['n']), None)
    need(dataset is not None and dataset['scan'] == 0, 'local dataset binding')
    input_name = str(Path(command[1]).relative_to(build.parent.parent))
    matching = [(name, raw) for name, raw in remote_inputs.items() if
                remote_manifest[name] == manifest['input_sha256'].get(input_name)]
    need(matching and row['input_hash'] == dataset['input_hash'] == input_fingerprint(matching[0][1], row['n']),
         'local/remote input SHA and exact prefix differ')
    row['scan'] = dataset['scan']
    need(record['result'] == row, 'local parsed/result differs')
    return row, dict(record=str(path), record_sha256=sha(path),
        completion_sha256=sha(directory/'COMPLETION.json'), containing_campaign_status=completion['status'],
        containing_campaign_error=completion.get('error'), selected_command_status='passed',
        source_relation='identical_engine_and_CPP_bench_pins_tests_may_differ',
        input_file_sha256=manifest['input_sha256'][input_name])


def analyse(host, local_record=None):
    host = host.resolve()
    evidence_paths = {Path(__file__).resolve(), Path(control.__file__).resolve(), Path(payload.__file__).resolve(),
                      VALIDATOR_ARCHIVE, *(path for path in host.rglob('*') if path.is_file())}
    if local_record is not None:
        evidence_paths.update(local_record.resolve().parent.glob('*.json'))
    need(all(not path.is_symlink() for path in evidence_paths), 'reader evidence symlink')
    evidence_before = {str(path): sha(path) for path in sorted(evidence_paths)}
    need(sha(control.__file__) == CONTROLLER_SHA and sha(payload.__file__) == WORKER_SHA, 'reader adapter dependencies changed')
    receipt_pin = sha(host / 'receipt.json')
    receipt = read_json(host / 'receipt.json')
    need(receipt.get('target') == payload.TARGET and receipt.get('targeted_shutdown_certified') is True and
         receipt.get('capture_received') is True and receipt.get('capture_pack_exit_code') == 0 and
         receipt.get('worker_receipt_present') is True and receipt.get('status') in ('completed', 'worker_failed') and
         receipt.get('controller_sha256') == CONTROLLER_SHA and receipt.get('worker_sha256') == WORKER_SHA,
         'closed target/capture/controller identity')
    need(receipt.get('GPU_executed') is False and receipt.get('FULL_executed') is False and
         receipt.get('public_status') == 'not_claimed' and receipt.get('backend') == 'cpu_reference', 'host scope')
    host_commands = raw_commands(host, receipt['commands'], host=True)
    generation, remote = receipt['generation'], receipt['remote_directory']
    control.epoch(generation)
    need(re.fullmatch(r'/tmp/ehgp-cpu-v8-[0-9a-f]{16}\.[A-Za-z0-9]{10}', remote), 'remote root format')
    for name, pin in control.GUARDS.items():
        need(sha(host / name) == pin, 'guard source pin')
    start = host_commands['guarded_start']
    origin = Path(start['argv'][0]).parent
    need(origin.is_absolute() and origin.name == 'cpu_v8_host' and '..' not in origin.parts and start['exit_code'] == 0 and
         start['argv'] == [str(origin/'start_and_verify.sh'), '--yes', '--guest-shutdown-minutes', '30',
            '--handoff-file', str(origin/'handoff.json'), '--lifecycle-state-file', str(origin/'lifecycle.txt'),
            '--guard-mark-dir', str(origin/'guardmarks')], 'exact guarded start/handoff paths')
    stop = host_commands['guarded_stop']
    need(stop['exit_code'] == 0 and stop['argv'][0] == str(origin/'stop_and_verify.sh') and
         stop['argv'][1:] == ['--yes', '--expected-last-start-timestamp', generation], 'exact-generation stop command')
    for name in ('before_upload', 'before_worker', 'before_retrieve'):
        need(host_commands[name]['exit_code'] == 0, 'generation recertification command')
        control.validate_target(read_json(host / (name+'.stdout')), 'RUNNING', generation)
    mark = control.fields((host / 'guardmarks/double_guard_verified').read_text())
    need(mark.get('generation') == generation and all(mark.get(key) == value for key, value in payload.TARGET.items()) and
         control.generation_from_records(read_json(host/'handoff.json'), control.fields((host/'lifecycle.txt').read_text())) == generation,
         'guard/handoff generation and target')
    source_manifest = host / 'source_manifest.json'
    manifest = read_json(source_manifest)
    need(sha(source_manifest) == receipt['manifest_sha256'] and sha(host/'snapshot.tar.gz') == receipt['snapshot_sha256'] and
         sha(host/'worker.py') == WORKER_SHA, 'payload transport pins')
    control.validate_snapshot(host/'snapshot.tar.gz', manifest)
    with tarfile.open(host/'snapshot.tar.gz', 'r:*') as archive:
        cases = payload.validate_plan(payload.strict_json(archive.extractfile(payload.PLAN).read()), manifest)
        inputs = {case['file']: archive.extractfile(case['file']).read() for case in cases}
    for name, raw in inputs.items():
        need(hashlib.sha256(raw).hexdigest() == manifest[name], 'input payload SHA256')
    output = match_capture_archive(host, receipt)
    worker = read_json(output/'receipt.json')
    need(worker.get('target') == payload.TARGET and worker.get('generation') == generation and
         worker.get('worker_sha256') == WORKER_SHA and worker.get('source_manifest_sha256') == receipt['manifest_sha256'] and
         worker.get('status') in ('completed', 'failed') and worker.get('sources_stable') is True and
         worker.get('compiled_dependencies_stable') is True and worker.get('GPU_executed') is False and
         worker.get('FULL_executed') is False and worker.get('contract_certified') is False and
         worker.get('useful_budget_seconds') == 900 and len(worker.get('available_cpus', [])) == 48, 'worker closure/scope')
    need((worker['status'] == 'completed') == (receipt['status'] == 'completed'), 'host/worker completion differs')
    for name in ('sources_before.json', 'sources_after.json'):
        need(read_json(output/name) == manifest, 'source/input closure differs')
    need(sha(output/'compiled_dependencies.json') == worker['compiled_dependency_manifest_sha256'], 'dependency manifest pin')
    dependencies = read_json(output/'compiled_dependencies.json')
    need(type(dependencies) is dict and dependencies and all(type(name) is str and Path(name).is_absolute() and
         type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin) for name, pin in dependencies.items()),
         'compiled dependency inventory')
    for name, pin in dependencies.items():
        if name.startswith(remote+'/source/'):
            need(manifest.get(name[len(remote+'/source/'):]) == pin, 'compiled local dependency not in source snapshot')
        else:
            need(name.startswith('/usr/'), 'unexpected external compiled dependency location')
    need(set(worker['binaries']) == set(payload.PROGRAMS) and all(re.fullmatch('[0-9a-f]{64}', pin)
         for pin in worker['binaries'].values()), 'compiled binary pins')
    worker_call = host_commands['worker']
    need(worker_call['exit_code'] == receipt['worker_exit_code'], 'worker remote exit binding')
    scripts = [x[len('--command='):] for x in worker_call['argv'] if x.startswith('--command=')]
    need(len(scripts) == 1, 'remote worker command absent')
    remote_argv = shlex.split(scripts[0])
    expected_argv = ['exec', 'python3', remote+'/worker.py', '--source-root', remote+'/source',
        '--source-manifest', remote+'/source_manifest.json', '--source-manifest-sha256', receipt['manifest_sha256'],
        '--guard-mark', remote+'/double_guard_verified', '--guard-mark-sha256', sha(host/'guardmarks/double_guard_verified'),
        '--generation', generation, '--session-deadline-epoch', str(receipt['session_deadline_epoch']),
        '--closing-margin-seconds', '300', '--output', remote+'/output']
    for key, value in payload.TARGET.items():
        expected_argv += ['--'+key, value]
    expected_argv += ['--execute', '--useful-budget-seconds', '900']
    need(remote_argv == expected_argv and worker['worker_argv'] == expected_argv[2:], 'worker command/receipt differs')
    commands = raw_commands(output, worker['commands'])
    compiler = commands['compiler']['argv'][0]
    need(Path(compiler).is_absolute() and Path(compiler).name == 'g++', 'compiler identity')
    expected = expected_worker_commands(remote, compiler, cases)
    need(set(commands) <= set(expected) and all(row['argv'] == expected[name] and row['cwd'] == remote+'/output' and
         row['session_work_deadline_epoch'] == worker['useful_deadline_epoch'] and row['per_command_watchdog'] is None
         for name, row in commands.items()), 'worker raw command/cwd/deadline binding')
    preparation = {name for name in expected if name != 'gate92' and not name.startswith('probe_')}
    need(preparation <= set(commands) and all(commands[name]['exit_code'] == 0 for name in preparation),
         'remote strict preparation incomplete')
    need('gate92' in commands, 'gate not attempted')
    rows, finished, failed, local_pair = [], [], [], None
    with validator_from_snapshot(host/'snapshot.tar.gz', manifest) as reference:
        gate_passed = commands['gate92']['exit_code'] == 0
        if gate_passed:
            reference.edge.validate_gate(read_json(output/'gate92.stdout'), 'mhgp8_wspd_q34_gate')
        else:
            failed.append(dict(command='gate92', exit_code=commands['gate92']['exit_code']))
        stopped = not gate_passed
        for i, case in enumerate(cases):
            name = 'probe_'+str(i)
            if name not in commands:
                stopped = True
                continue
            need(not stopped, 'probe ran after failed/missing gate or earlier probe')
            row_command = commands[name]
            if row_command['exit_code'] != 0:
                need(not (output/(name+'.summary.json')).exists(), 'interrupted probe promoted to summary')
                failed.append(dict(command=name, case=case, exit_code=row_command['exit_code'],
                    session_deadline_reached=row_command.get('session_deadline_reached', False)))
                stopped = True
                continue
            need(not row_command.get('residual_or_interrupted_group_killed'), 'successful command was interrupted')
            row = read_json(output/(name+'.stdout'))
            scientific_argv = bind_probe(row, row_command['argv'], case, remote, inputs[case['file']])
            reference.validate(row, scientific_argv)
            summary = read_json(output/(name+'.summary.json'))
            need(summary == dict(case=case, status='completed', input_file_sha256=manifest[case['file']],
                                 output=row['output']), 'published summary/raw result differs')
            row['scan'] = 0
            rows.append(row)
            finished.append(dict(command=name, case=case, input_hash=row['input_hash'], output=row['output'],
                timings_ms=row['timings_ms'], expanded_pairs=row['work']['expanded_pairs'],
                cover_sites=row['work']['cover_sites'], q3_census_sites=row['work']['q3']['census_point_tests'],
                q4_active_sites=row['work']['local28']['sweep']['active_sites']))
        need({path.name for path in output.glob('probe_*.summary.json')} ==
             {entry['command']+'.summary.json' for entry in finished}, 'completed summary inventory differs')
        need(worker['status'] != 'completed' or (gate_passed and len(rows) == len(cases)), 'false complete session')
        analysis = reference.summary(rows)
        if local_record is not None:
            local, provenance = local_completed_record(local_record, reference, manifest, inputs)
            remote_row = next((row for row in rows if row['workers'] == 48 and all(row[key] == local[key]
                for key in ('n', 'input_hash', 'kmax', 's', 'mask', 'q4_backend'))), None)
            local_pair = dict(provenance, compared=False, reason='matching_remote_command_not_completed')
            if remote_row is not None:
                need(payload.logical_result(local) == payload.logical_result(remote_row), 'local/G4 exact logical work differs')
                local_time = local['timings_ms']['pipeline_including_shared_preparation']
                remote_time = remote_row['timings_ms']['pipeline_including_shared_preparation']
                local_pair.update(compared=True, reason=None, n=local['n'], local_workers=local['workers'],
                    G4_workers=remote_row['workers'], verified_equal=True, local_pipeline_ms=local_time,
                    G4_pipeline_ms=remote_time, local_over_G4_time=local_time/remote_time if remote_time else None,
                    speedup_scope='different_hosts_and_worker_counts_not_isolated_parallel_scaling')
    pairs = []
    for a in rows:
        for b in rows:
            if a['workers'] == 1 and b['workers'] == 48 and all(a[key] == b[key] for key in
                    ('n', 'input_hash', 'kmax', 's', 'mask', 'q4_backend')):
                need(payload.logical_result(a) == payload.logical_result(b), 'W1/W48 exact logical work differs')
                denominator = b['timings_ms']['pipeline_including_shared_preparation']
                pairs.append(dict(n=a['n'], kmax=a['kmax'], verified_equal=True,
                    speedup=a['timings_ms']['pipeline_including_shared_preparation']/denominator if denominator else None))
    need(sha(host/'receipt.json') == receipt_pin and sha(host/'source_manifest.json') == receipt['manifest_sha256'] and
         sha(host/'snapshot.tar.gz') == receipt['snapshot_sha256'] and sha(host/'capture.tar.gz') == receipt['capture_sha256'] and
         sha(VALIDATOR_ARCHIVE) == VALIDATOR_SHA, 'closed evidence changed while reading')
    match_capture_archive(host, receipt)
    evidence_after = {str(path): sha(path) for path in sorted(evidence_paths)}
    need(evidence_before == evidence_after, 'read evidence/dependency closure changed')
    return dict(schema='mhgp8_cpu_posthoc_v1', status='validated_complete' if worker['status']=='completed' else 'validated_partial',
        host_receipt_sha256=receipt_pin, target=payload.TARGET, generation=generation,
        targeted_shutdown_certified=True, gate_passed=gate_passed, measurements=finished,
        attempted_failures=failed, planned=len(cases), completed=len(finished), unexecuted=len(cases)-len(finished)-
        sum('case' in item for item in failed), worker_error=worker.get('error'), cross_worker=pairs,
        local_completed_comparison=local_pair,
        growth=analysis['growth'], validator_sha256=VALIDATOR_SHA,
        validator_origin='separately_archived_posthoc_reader_not_executed_in_guest',
        reader_sha256=sha(__file__), read_evidence_sha256_before=evidence_before, read_evidence_sha256_after=evidence_after,
        reader_scope='closed_CPU_candidate_capture_not_exhaustive_large_oracle',
        full_contract_qualified=False, GPU_executed=False, universal_subquadratic_claim=False)


class PureTests(unittest.TestCase):
    def test_commands(self):
        case = dict(file='data/scan0_n8000.u16le', n=1000, k=5, s=8, mask=6, backend=28, workers=48)
        commands = expected_worker_commands('/tmp/example', '/usr/bin/g++', [case])
        self.assertEqual(commands['gate92'], ['/tmp/example/output/build/mhgp8_wspd_q34_gate', '--selftest'])
        self.assertEqual(len([name for name in commands if name.startswith('compile_')]), 22)
        self.assertEqual(len([name for name in commands if name.startswith('link_')]), 2)
        self.assertIn('-Werror', commands['compile_0'])

    def test_input_binding(self):
        raw = struct.pack('<HHHHHH', 1, 2, 3, 4, 5, 6)
        self.assertEqual(input_fingerprint(raw, 2), 16295428579255750976)
        case = dict(file='data/test.u16le', n=2, k=5, s=8, mask=6, backend=28, workers=48)
        row = dict(source_n=2, input_hash=input_fingerprint(raw, 2))
        command = expected_worker_commands('/tmp/example', '/usr/bin/g++', [case])['probe_0']
        self.assertEqual(Path(bind_probe(row, command, case, '/tmp/example', raw)[1]).name, 'n2.u16le')
        for bad_row, bad_command, bad_raw in [({**row, 'input_hash': row['input_hash'] ^ 1}, command, raw),
            (row, [*command[:-1], 'records'], raw), (row, command, raw[:-1]),
            (row, command, struct.pack('<HHHHHH', 1, 2, 3, 1, 2, 3))]:
            with self.assertRaises(ValueError):
                bind_probe(bad_row, bad_command, case, '/tmp/example', bad_raw)

    def test_raw_binding(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            base = dict(argv=['probe'], cwd='/tmp/fixed', started_utc='fixed', session_work_deadline_epoch=900,
                        per_command_watchdog=None)
            row = dict(base, exit_code=0, group_closed=True, stdout_sha256=hashlib.sha256(b'{}').hexdigest(),
                       stderr_sha256=hashlib.sha256(b'').hexdigest())
            for name, value in [('p.intent.json', base), ('p.command.json', row)]:
                (path/name).write_text(json.dumps(value))
            (path/'p.stdout').write_bytes(b'{}')
            (path/'p.stderr').write_bytes(b'')
            self.assertEqual(raw_commands(path, [row]), {'p': row})
            (path/'p.stdout').write_bytes(b'{"tampered":true}')
            with self.assertRaises(ValueError):
                raw_commands(path, [row])

    def test_only_two_capacity_maxima_normalized(self):
        a = dict(input_hash=123, output={'sum': '1'}, front={'count': 2}, cloud_work={'count': 3},
                 index_work={'count': 4}, work={'peak_edge_buffer_bytes': 100,
                     'q3': {'peak_shell_bytes': 200, 'census_point_tests': 300}})
        b = deepcopy(a)
        b['work']['peak_edge_buffer_bytes'] += 1
        b['work']['q3']['peak_shell_bytes'] += 1
        self.assertEqual(payload.logical_result(a), payload.logical_result(b))
        for field in ('input_hash', 'output', 'front', 'cloud_work', 'index_work', 'work'):
            wrong = deepcopy(b)
            if field == 'input_hash': wrong[field] += 1
            elif field == 'output': wrong[field]['sum'] = '2'
            elif field == 'work': wrong[field]['q3']['census_point_tests'] += 1
            else: wrong[field]['count'] += 1
            self.assertNotEqual(payload.logical_result(a), payload.logical_result(wrong))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path)
    parser.add_argument('--local-record', type=Path, help='optional completed local record, even if containing campaign was interrupted')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        return 0 if unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(PureTests)).wasSuccessful() else 1
    need(args.host is not None, '--host is required')
    print(json.dumps(analyse(args.host, args.local_record), sort_keys=True, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
