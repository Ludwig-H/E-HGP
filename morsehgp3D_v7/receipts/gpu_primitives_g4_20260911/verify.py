#!/usr/bin/env python3
"""Read-only portable verifier: finite GPU primitive gates and targeted closure."""
import hashlib
import io
import json
import math
from datetime import datetime
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile

BASE = Path(__file__).resolve().parent
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
PINS = dict(worker='5a1c85320b8a075af3e713314a5c837e72a15c2b3c8b9e74b9d908566ce47f60',
            controller='177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8',
            prepare='0e437f4860bf8420309da3a1a9b821a09aff6c1502473b52ca79f2c5e47bcf46',
            start='73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0',
            stop='ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate_json_key')
        result[key] = value
    return result


def strict(raw):
    def finite(value):
        number = float(value)
        need(math.isfinite(number), 'nonfinite_json')
        return number
    def invalid(_value):
        raise ValueError('nonfinite_json')
    return json.loads(raw, object_pairs_hook=unique, parse_float=finite, parse_constant=invalid)


def safe(name):
    p = PurePosixPath(name)
    need(type(name) is str and not p.is_absolute() and '..' not in p.parts
         and str(p) == name and name not in ('', '.'), 'canonical_relative_path')
    return name


def fields(raw):
    result = {}
    for line in raw.decode().splitlines():
        key, sep, value = line.partition('=')
        need(sep == '=' and key not in result, 'guard_fields')
        result[key] = value
    return result


def epoch(value):
    return datetime.fromisoformat(value.replace('Z', '+00:00')).timestamp()


def no_secret_or_elf(name, raw):
    need(not raw.startswith(b'\x7fELF'), 'ELF_excluded:' + name)
    prefix = b'-----BEGIN ' + b'OPENSSH' + b' PRIVATE KEY-----'
    rsa = b'-----BEGIN ' + b'RSA' + b' PRIVATE KEY-----'
    need(prefix not in raw and rsa not in raw, 'private_key_excluded:' + name)
    need(not any(p.startswith('oslogin_') or p in ('session_key', 'session_key.pub')
                 for p in PurePosixPath(name).parts), 'connection_material_excluded')


def archive(raw):
    found = {}
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:gz') as stream:
        seen = set()
        for member in stream.getmembers():
            name = member.name.rstrip('/') if member.isdir() else member.name
            safe(name)
            need(name not in seen, 'duplicate_archive_member')
            seen.add(name)
            need(member.isfile() or member.isdir(), 'archive_regular_only')
            if member.isfile():
                data = stream.extractfile(member).read()
                no_secret_or_elf(name, data)
                found[name] = data
    return found


def verify(base=BASE):
    manifest = strict((base / 'MANIFEST.json').read_bytes())
    files = {p.relative_to(base).as_posix() for p in base.rglob('*') if p.is_file()
             and p.name != 'MANIFEST.json'}
    need(files == set(manifest['files']), 'physical_file_set')
    for name, digest in manifest['files'].items():
        p = base / safe(name)
        need(not p.is_symlink(), 'no_symlink')
        data = p.read_bytes()
        need(sha(data) == digest, 'physical_hash:' + name)
        no_secret_or_elf(name, data)
    mapping = strict((base / 'storage_map.json').read_bytes())
    def raw(name):
        safe(name)
        row = mapping[name]
        data = (base / safe(row['physical'])).read_bytes()
        need(sha(data) == row['sha256'] and len(data) == row['bytes'], 'logical_hash:' + name)
        no_secret_or_elf(name, data)
        return data
    def js(name):
        return strict(raw(name))
    for name in mapping:
        raw(name)
    stable = strict((base / 'capture_stability.json').read_bytes())
    need(stable['before'] == stable['after'] == {name: row['sha256'] for name, row in mapping.items()},
         'all_originals_stable_during_copy')
    sources = js('local/input_r1/source_manifest.json')
    pins = js('local/input_r1/pins.json')
    need(sources == js('local/input_r1/sources_before.json') == js('local/input_r1/sources_after.json'),
         'frozen_source_stability')
    source_archive = archive(raw('local/input_r1/snapshot.tar.gz'))
    need(set(source_archive) == set(sources), 'snapshot_exact_inventory')
    for name, digest in sources.items():
        need(sha(source_archive[name]) == digest and
             raw('local/input_r1/source/' + name) == source_archive[name], 'source_archive_pin')
    for name, expected in PINS.items():
        need(pins[name]['sha256'] == expected, 'reviewed_pin:' + name)
    pin_paths = dict(worker='scripts/gcp-migration/anchor_meb_worker_v7.py',
                     controller='scripts/gcp-migration/full_probe_session_v7.py',
                     start='scripts/gcp-migration/start_and_verify.sh',
                     stop='scripts/gcp-migration/stop_and_verify.sh',
                     prepare='scripts/prepare.py')
    for name, path in pin_paths.items():
        need(sha(raw(path)) == PINS[name], 'copied_reviewed_script')
    need(sha(raw('local/input_r1/source_manifest.json')) == pins['manifest']['sha256'] and
         sha(raw('local/input_r1/snapshot.tar.gz')) == pins['snapshot']['sha256'], 'prepared_inputs')
    historical = []
    for version in ('checks_r1', 'checks_r2', 'checks_r3'):
        pre = 'local/' + version + '/'
        receipt = js(pre + 'receipt.json')
        need(receipt['status'] == 'passed' and receipt['GCP_used'] is False and
             receipt['before'] == receipt['after'] and len(receipt['commands']) == 4, 'selftests_closed')
        for row in receipt['commands']:
            need(row['exit_code'] == 0 and row['ended_epoch'] >= row['started_epoch'], 'selftest_exit')
            value = strict(raw(pre + row['name'] + '.stdout'))
            need(value['status'] == 'passed' and value['GCP_used'] is False and
                 not raw(pre + row['name'] + '.stderr').strip(), 'selftest_report')
        if version == 'checks_r3':
            for name, digest in receipt['before'].items():
                need(sha(raw('scripts/gcp-migration/' + Path(name).name)) == digest, 'r3_current_sources')
        else:
            historical.append(version)
    local = js('local/local_o3_r1/receipt.json')
    need(local['status'] == 'passed' and local['device_executed'] is False and local['GCP_used'] is False
         and local['before'] == local['after'] == sources and len(local['commands']) == 2,
         'local_compile_only')
    need(local['worker_sha256'] == PINS['worker'] and
         local['source_manifest_sha256'] == pins['manifest']['sha256'], 'local_compile_pins')
    for row in local['commands']:
        need(row['exit_code'] == 0 and row['ended_epoch'] >= row['started_epoch'], 'local_compile_exit')
        for stream in ('stdout', 'stderr'):
            need(sha(raw('local/local_o3_r1/' + row['kind'] + '.' + stream)) == row[stream + '_sha256'],
                 'local_compile_stream')
        need(sha(raw('local/local_o3_r1/' + row['kind'] + '.d')) == row['depfile_sha256'], 'local_depfile')
        need(row['project'] and all(sources.get(k) == v for k, v in row['project'].items()), 'local_project_dependencies')
    host = js('host/receipt.json')
    need(host['status'] == 'completed' and host['target'] == TARGET and host['public_status'] == 'not_claimed'
         and host['targeted_shutdown_certified'] is True and host['worker_exit_code'] == 0
         and host['capture_received'] is True and host['worker_receipt_present'] is True
         and host['capture_pack_exit_code'] == 0, 'session_and_targeted_closure')
    for name in ('worker', 'controller', 'snapshot', 'manifest'):
        need(host[name + '_sha256'] == pins[name]['sha256'], 'session_input_pins')
    need(raw('host/snapshot.tar.gz') == raw('local/input_r1/snapshot.tar.gz') and
         raw('host/source_manifest.json') == raw('local/input_r1/source_manifest.json') and
         sha(raw('host/worker.py')) == PINS['worker'], 'transport_source_identity')
    for name in ('start', 'stop'):
        need(sha(raw('host/' + name + '_and_verify.sh')) == PINS[name], 'transport_guard_pin')
    handoff = js('host/handoff.json')
    mark = fields(raw('host/guardmarks/double_guard_verified'))
    lifecycle = fields(raw('host/lifecycle.txt'))
    need(all(record.get(k) == v for record in (handoff, mark, lifecycle) for k, v in TARGET.items()),
         'all_guard_targets')
    need(handoff['last_start_timestamp'] == mark['generation'] == lifecycle['generation'] == host['generation'],
         'one_generation')
    need(mark['mark'] == 'double_guard_verified' and mark['guest_shutdown_minutes'] == '30'
         and 2700 <= int(mark['max_run_seconds']) <= 28800, 'double_guard_bounds')
    for phase in ('before_upload', 'before_worker', 'before_retrieve'):
        observed = js('host/' + phase + '.stdout')
        need(observed['name'] == TARGET['instance'] and observed['status'] == 'RUNNING' and
             observed['lastStartTimestamp'] == host['generation'] and observed['labels']['project'] == 'e-hgp'
             and observed['zone'].endswith('/' + TARGET['zone']) and
             observed['machineType'].endswith('/g4-standard-48'), 'GCE_target_type_generation')
        scheduling = observed['scheduling']
        need(scheduling['provisioningModel'] == 'SPOT' and scheduling['instanceTerminationAction'] == 'STOP'
             and int(scheduling['maxRunDuration']['seconds']) == int(mark['max_run_seconds']) == 3600,
             'SPOT_STOP_3600_seconds')
    commands = {row['name']: row for row in host['commands']}
    need(len(commands) == len(host['commands']), 'unique_host_commands')
    for name in ('guarded_start', 'guarded_stop', 'worker', 'download', 'pack_capture'):
        row = commands[name]
        need(row['exit_code'] == 0 and row.get('group_closed') is True, 'closed_host_command:' + name)
        need(js('host/' + name + '.command.json') == row, 'host_command_record')
    for name, row in commands.items():
        if name == 'oslogin_add':
            continue  # Its output/profile deliberately is not copied.
        need(row['ended_epoch'] >= row['started_epoch'] and row['group_closed'] is True and
             not row.get('last_resort_group_killed'), 'host_command_closed')
        for stream in ('stdout', 'stderr'):
            need(sha(raw('host/' + name + '.' + stream)) == row[stream + '_sha256'], 'host_stream_pin')
    stop = commands['guarded_stop']['argv']
    need(stop[stop.index('--expected-last-start-timestamp') + 1] == host['generation'], 'stop_exact_generation')
    stop_text = raw('host/guarded_stop.stdout').decode()
    need(any(line.startswith('[OK] Cible ' + TARGET['instance'] + ' ') and
             'état GCE TERMINATED' in line for line in stop_text.splitlines()), 'raw_target_TERMINATED')
    captured = raw('host/capture.tar.gz')
    need(sha(captured) == host['capture_sha256'], 'capture_transport_pin')
    archived = archive(captured)
    need({'host/received/' + name for name in archived} ==
         {name for name in mapping if name.startswith('host/received/')}, 'capture_whole_inventory')
    for name, data in archived.items():
        need(name.startswith('output/') and raw('host/received/' + name) == data, 'capture_exact_bytes')
    pre = 'host/received/output/'
    guest = js(pre + 'receipt.json')
    need(guest['status'] == 'completed' and guest['public_status'] == 'not_claimed' and
         guest['contract_qualified'] is False and guest['FULL_GPU_available'] is False and
         guest['GCP_used'] is True and guest['backend_scope'] == 'local_meb_batch_only' and
         guest['target'] == TARGET and guest['generation'] == host['generation'] and
         guest['selected_gates'] == ['gate', 'key_gate'] and guest['sources_stable'] is True,
         'worker_bounded_authority')
    need(guest['worker_sha256'] == PINS['worker'] and
         guest['source_manifest_sha256'] == pins['manifest']['sha256'] and
         sources == js(pre + 'sources_before.json') == js(pre + 'sources_after.json'), 'guest_source_stability')
    need(guest['CUDA_installation_attempted'] is False and len(guest['available_cpus']) == 48,
         'existing_toolkit_48_vcpu')
    evidence = js(pre + 'guard_evidence.json')
    need(evidence['mark'] == mark and evidence['metadata'] == guest['guest_metadata'] and
         evidence['schedule']['MODE'] == 'poweroff' and evidence['bounds'] == guest['guards'], 'guest_double_guard')
    bounds = guest['guards']
    guest_deadline = int(evidence['schedule']['USEC']) / 1000000
    need(bounds['guest_deadline_epoch'] == guest_deadline and bounds['closing_margin_seconds'] == 300 and
         bounds['safe_gce_deadline_epoch'] == epoch(host['generation']) + 3600 - 300 and
         host['session_deadline_epoch'] == int(guest_deadline) and
         bounds['work_deadline_epoch'] == int(guest_deadline) - 300 and
         guest_deadline <= bounds['safe_gce_deadline_epoch'] and
         commands['guarded_stop']['ended_epoch'] < guest_deadline, 'shutdown_deadlines_and_closing_margin')
    rows = {PurePosixPath(name).name.removesuffix('.command.json'): js(name) for name in mapping
            if name.startswith(pre) and name.endswith('.command.json')}
    need(len(rows) == len(guest['commands']), 'unique_guest_commands')
    need(all(sum(row == recorded for recorded in guest['commands']) == 1 for row in rows.values()),
         'whole_guest_command_inventory')
    for name, row in rows.items():
        need(row['group_closed'] is True and not row.get('residual_or_interrupted_group_killed') and
             row['elapsed_seconds'] >= 0 and row['session_work_deadline_epoch'] == guest['guards']['work_deadline_epoch'],
             'guest_closed_with_session_deadline')
        need(epoch(host['generation']) <= epoch(row['started_utc']) <= epoch(row['ended_utc']) <=
             guest['guards']['work_deadline_epoch'], 'guest_command_chronology')
        for stream in ('stdout', 'stderr'):
            need(sha(raw(pre + name + '.' + stream)) == row[stream + '_sha256'], 'guest_stream_pin')
    expected = dict(device_gate=0, key_gate=0, key_gate_skip_write=1, key_gate_corrupt_word=1,
                    device_gate_bad_argument=2, compile_gate=0, compile_key_gate=0)
    for name, code in expected.items():
        row = rows[name]
        need(row['exit_code'] == code, 'guest_exit:' + name)
        need(js(pre + name + '.command.json') == row, 'guest_command_record')
    for kind in ('gate', 'key_gate'):
        binary = guest['binaries'][kind]
        need(binary['sha256'] == binary['sha256_after'] and re.fullmatch('[0-9a-f]{64}', binary['sha256']),
             'guest_binary_stability')
        need(sha(raw(pre + kind + '.d')) == binary['depfile_sha256'], 'guest_depfile')
        dependency = js(pre + kind + '_compiled_dependencies.json')
        need(dependency['project'] and all(sources.get(k) == v for k, v in dependency['project'].items()),
             'guest_project_dependencies')
        argv = rows['compile_' + kind]['argv']
        for flag in ('-O3', '-DNDEBUG', '-std=c++20', '-arch=sm_120', '-fmad=false',
                     '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread', '-MMD'):
            need(flag in argv, 'strict_device_compile_flag')
        need(argv[-2:] == ['-o', binary['path']] and argv[argv.index('-ccbin') + 1].endswith('/nvcc_strict_host.py'),
             'compiled_binary_and_adapter')
    need(sha(raw(pre + 'nvcc_strict_host.py')) == sources['morsehgp3D_v7/bench/nvcc_strict_host.py'],
         'guest_adapter_snapshot')
    need(rows['device_gate']['argv'] == [guest['binaries']['gate']['path'], '--selftest'] and
         rows['device_gate_bad_argument']['argv'] == [guest['binaries']['gate']['path'], '--unknown'],
         'MEB_actual_execution_arguments')
    key_argv = rows['key_gate']['argv']
    need(len(key_argv) == 4 and key_argv[:2] == [guest['binaries']['key_gate']['path'], '--selftest'] and
         key_argv[2].endswith('/morsehgp3D_v7/bench/ball_key_device_private/vectors.txt') and
         key_argv[3] == '--expected-sha=' + sources['morsehgp3D_v7/bench/ball_key_device_private/vectors.txt'],
         'arithmetic_actual_execution_arguments')
    for name, injection in [('key_gate_skip_write', 'skip-write'), ('key_gate_corrupt_word', 'corrupt-word')]:
        need(rows[name]['argv'] == key_argv + ['--inject=' + injection], 'causal_injection_argv')
    def summary(name):
        data = raw(pre + name + '.stdout')
        need(len(data.splitlines()) == 1 and not raw(pre + name + '.stderr').strip(), 'one_clean_scientific_summary')
        value = strict(data)
        need(value == js(pre + name + '.summary.json') == guest[name], 'summary_raw_receipt_identity')
        return value
    meb = summary('device_gate')
    need(meb['status'] == 'passed' and meb['backend'] == 'CUDA' and meb['device_executed'] is True
         and meb['gcp_used'] is False and meb['scope'] == 'local_meb_batch_only' and meb['sm'] == '12.0',
         'actual_MEB_device_scope')
    for key in ('compared', 'checks', 'q1', 'q2', 'q3', 'q4', 'extra_shells', 'rejections', 'failures',
                'abi_version', 'request_bytes', 'selection_bytes', 'causal_flags', 'positions',
                'resident_h2d_bytes', 'batch_h2d_bytes', 'batch_d2h_bytes', 'batch_launches',
                'host_materializations', 'host_validation_powers', 'reported_selection_powers'):
        need(type(meb[key]) is int and meb[key] >= 0, 'MEB_integer_counter')
    need(meb['compared'] == 605 and [meb[k] for k in ('q1', 'q2', 'q3', 'q4')] == [82, 393, 110, 20]
         and meb['checks'] >= 6 * 605 and meb['extra_shells'] == 197 and meb['rejections'] >= 20
         and meb['failures'] == 0, 'MEB_exact_fixture_census')
    need((meb['abi_version'], meb['request_bytes'], meb['selection_bytes']) == (1, 72, 112)
         and meb['causal_flags'] >= 4 and meb['positions'] > 0 and meb['batch_launches'] == 1
         and meb['resident_h2d_bytes'] == 24 * meb['positions'] and meb['batch_h2d_bytes'] == 184 * 605
         and meb['batch_d2h_bytes'] == 112 * 605 and meb['host_materializations'] == 605
         and meb['host_validation_powers'] > 0 and meb['reported_selection_powers'] > 0, 'MEB_ABI_batch_accounting')
    key = summary('key_gate')
    need(key['status'] == 'passed' and key['backend'] == 'CUDA' and key['device_executed'] is True
         and key['public_status'] == 'not_claimed' and key['vectors_sha256'] == sources[
             'morsehgp3D_v7/bench/ball_key_device_private/vectors.txt'], 'actual_arithmetic_device_scope')
    for name in ('cases', 'checked_words', 'raw_cases', 'gcd_cases', 'division_cases', 'support_cases',
                 'rejected_cases', 'input_bytes', 'output_bytes', 'alignment', 'device_arch'):
        need(type(key[name]) is int and key[name] >= 0, 'key_integer_counter')
    need(key['raw_cases'] >= 4000 and key['gcd_cases'] >= 2000 and key['division_cases'] >= 4000
         and key['support_cases'] >= 3000 and key['rejected_cases'] >= 20 and
         key['cases'] == sum(key[k] for k in ('raw_cases', 'gcd_cases', 'division_cases', 'support_cases'))
         and key['checked_words'] == 24 * key['cases'], 'arithmetic_whole_corpus_twice')
    need((key['input_bytes'], key['output_bytes'], key['alignment'], key['device_arch']) == (112, 96, 8, 1200),
         'arithmetic_device_ABI')
    for name in ('key_gate_skip_write', 'key_gate_corrupt_word'):
        need(not raw(pre + name + '.stdout') and raw(pre + name + '.stderr').strip() == b'FAIL backend.expected_word',
             'device_causal_mutant')
    need(not raw(pre + 'device_gate_bad_argument.stdout'), 'bad_argument_no_publication')
    return dict(status='verified_finite_GPU_primitives_and_targeted_shutdown', MEB_cases=meb['compared'],
                arithmetic_cases=key['cases'], arithmetic_checked_words=key['checked_words'],
                causal_device_mutants=2, host_commands=len(commands), worker_commands=len(rows),
                logical_files=len(mapping), target=TARGET, generation=host['generation'],
                targeted_shutdown_certified=True, FULL_GPU_available=False, contract_qualified=False,
                public_status='not_claimed', reader_GCP_used=False, captured_session_GCP_used=True,
                historical_selftest_sources_not_reconstructed=historical)


if __name__ == '__main__':
    need(len(sys.argv) == 1, 'no_arguments')
    print(json.dumps(verify(), sort_keys=True))
