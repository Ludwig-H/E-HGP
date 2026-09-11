#!/usr/bin/env python3
"""Strict portable historical reader. No cloud call, compilation or execution."""
import argparse
from datetime import datetime
import hashlib
import io
import json
from pathlib import Path
import tarfile

ROOT = Path(__file__).resolve().parent
PROJECT = 'devpod-gpu-exploration'
US = dict(project=PROJECT, zone='us-central1-b', instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
EU = dict(project=PROJECT, zone='europe-west4-a', instance='ehgp-blackwell-spot')
GENERATIONS = {'session_r1': '2026-09-11T06:51:01.668-07:00',
               'session_r2': '2026-09-11T06:53:44.713-07:00',
               'session_eu_r1': '2026-09-11T06:59:41.712-07:00'}
WORKER = '043197e4c73d92ff8845ddb5836c11fbb9ed9bdbd9a609edf7ce4a946d2c0fb3'
CONTROLLER = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
WRAPPER = '3ec16f9b8777d6fc5527f65b7080d207f7dee0d8e89a7f9180789083ce15dab7'
START = '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0'
STOP = 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'
TAR = '15f0abbd41bc77d3c3c6e0bb0bce3bb373f5d41ea525b53d02e8879ea90d2c4b'
SOURCE_MANIFEST = '803f1fcfdd9369f1d13900a5793a446d455caf1b938fb2c36aa7f9c83d850f76'
WORKER_PACKET = '8758776c41abbbe08009a55c6e97bc10f7d040ccaadb4d5bcc5a6d93d5bf16b2'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(pairs):
    row = {}
    for key, value in pairs:
        need(key not in row, 'duplicate JSON field')
        row[key] = value
    return row


def decode(raw):
    return json.loads(raw, object_pairs_hook=unique)


def relative(name):
    need(type(name) is str, 'relative path type')
    path = Path(name)
    need(not path.is_absolute() and '..' not in path.parts and str(path) == name, 'safe relative path')
    return path


def epoch(value):
    parsed = datetime.fromisoformat(value.replace('Z', '+00:00'))
    need(parsed.tzinfo is not None, 'timestamp timezone')
    return parsed.timestamp()


def fields(raw):
    pairs = [line.split('=', 1) for line in raw.decode().splitlines()]
    need(all(len(pair) == 2 for pair in pairs), 'guard field grammar')
    return unique(pairs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extract', type=Path, help='new destination, preserved publishable bytes only')
    args = parser.parse_args()
    manifest = decode((ROOT / 'manifest.json').read_bytes())
    need(manifest['schema'] == 'mhgp7-terminal-batch-g4-attempts-v1', 'packet schema')
    need({p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file()} ==
         set(manifest['files']) | {'manifest.json'}, 'physical closure')
    for name, pin in manifest['files'].items():
        path = ROOT / relative(name)
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(ROOT), 'physical regular file')
        raw = path.read_bytes()
        need(sha(raw) == pin['sha256'] and len(raw) == pin['bytes'] and not raw.startswith(b'\x7fELF'), 'physical bytes/no ELF')
    mapping = decode((ROOT / 'storage_map.json').read_bytes())
    logical = decode((ROOT / 'logical_manifest.json').read_bytes())
    need(set(logical) == set(mapping) and set(mapping.values()) <= set(manifest['files']), 'logical mapping')
    data = {}
    for name, pin in logical.items():
        relative(name)
        raw = (ROOT / relative(mapping[name])).read_bytes()
        need(sha(raw) == pin['sha256'] and len(raw) == pin['bytes']
             and b'\n-----BEGIN OPENSSH PRIVATE KEY-----\n' not in raw, 'logical bytes/no credential')
        data[name] = raw

    def get(name):
        need(name in data, 'required logical file: ' + name)
        return data[name]

    def obj(name):
        return decode(get(name))

    omissions = obj('provenance/omissions.json')
    inventory = obj('provenance/source_inventory.json')
    projections = obj('provenance/projections.json')
    need(len(omissions) == 12 and not set(omissions) & set(data), 'explicit omissions only')
    need(set(inventory) == (set(data) - set(projections) - {
        'provenance/source_inventory.json', 'provenance/omissions.json',
        'provenance/projections.json', 'provenance/publish.py'}) | set(omissions), 'capture inventory closure')
    for name, pin in inventory.items():
        if name in omissions:
            need(omissions[name]['sha256'] == pin['sha256'] and omissions[name]['bytes'] == pin['bytes'], 'omission hash/size')
        else:
            need(logical[name] == pin, 'unchanged original captured bytes')
    for name, projection in projections.items():
        need(projection['source'] in omissions and omissions[projection['source']]['sha256'] == projection['source_sha256'],
             'declared derivative, never a silently redacted original')
    need(sha(get('imports/terminal_batch_worker_manifest.json')) == WORKER_PACKET, 'published worker snapshot link')

    def stream_pin(name):
        return sha(data[name]) if name in data else omissions[name]['sha256']

    def command(prefix, row, group=False):
        name = row['name']
        need(row == obj(prefix + name + '.command.json'), 'exact command receipt')
        if group:
            need(row['group_closed'] is True, 'process group joined')
        for suffix in ('stdout', 'stderr'):
            need(stream_pin(prefix + name + '.' + suffix) == row[suffix + '_sha256'], 'raw or explicitly omitted stream pin')

    session_receipts = {}
    for session, generation in GENERATIONS.items():
        prefix = 'captures/' + session + '/full_host/'
        target = EU if session == 'session_eu_r1' else US
        receipt = obj(prefix + 'receipt.json')
        session_receipts[session] = receipt
        need(receipt['target'] == target and receipt['generation'] == generation
             and receipt['targeted_shutdown_certified'] is True and receipt['public_status'] == 'not_claimed', 'exact closed target/generation')
        need(receipt['controller_sha256'] == CONTROLLER and receipt['worker_sha256'] == WORKER
             and receipt['snapshot_sha256'] == TAR and receipt['manifest_sha256'] == SOURCE_MANIFEST, 'session source authority')
        for name, pin in (('worker.py', WORKER), ('start_and_verify.sh', START), ('stop_and_verify.sh', STOP),
                          ('source_manifest.json', SOURCE_MANIFEST)):
            need(sha(get(prefix + name)) == pin, 'copied executable/source pin')
        need(omissions[prefix + 'snapshot.tar.gz']['sha256'] == TAR
             and omissions[prefix + 'snapshot.tar.gz']['reason'] == 'published_source_archive_linked_not_duplicated', 'no duplicated source archive')
        rows = receipt['commands']
        for row in rows:
            command(prefix, row, True)
        stop = rows[-1]
        need(stop['name'] == 'guarded_stop' and stop['exit_code'] == 0 and
             stop['argv'][1:] == ['--yes', '--expected-last-start-timestamp', generation], 'versioned guarded stop')
        need(target['instance'] in get(prefix + 'guarded_stop.stdout').decode()
             and 'état GCE TERMINATED' in get(prefix + 'guarded_stop.stdout').decode(), 'exact target TERMINATED stream')
        lifecycle = fields(get(prefix + 'lifecycle.txt'))
        need(lifecycle['schema'] == 'e-hgp.lifecycle-state.v1' and lifecycle['generation'] == generation
             and all(lifecycle[key] == value for key, value in target.items()), 'lifecycle exact identity')
        if session != 'session_eu_r1':
            need(receipt['status'] == 'failed' and receipt['error'] == 'ValueError: start not certified'
                 and [row['name'] for row in rows] == ['oslogin_add', 'guarded_start', 'guarded_stop']
                 and [row['exit_code'] for row in rows] == [0, 1, 0] and lifecycle['state'] == 'targeted_stopped',
                 'preempted before any worker')
            stderr = get(prefix + 'guarded_start.stderr').decode()
            need('préemptée pendant le démarrage' in stderr and generation in stderr, 'reported exact-generation preemption')
            need(not any(name.startswith(prefix + 'guardmarks/') or name.startswith(prefix + 'received/') for name in data),
                 'no US doubleguard/guest capture invented')
        else:
            need(receipt['status'] == 'worker_failed' and receipt['worker_exit_code'] == 1
                 and receipt['capture_received'] is True and receipt['capture_pack_exit_code'] == 0
                 and receipt['worker_receipt_present'] is True and lifecycle['state'] == 'targeted_running',
                 'EU historical start state is not rewritten by final stop')
            expected = ['oslogin_add', 'guarded_start', 'before_upload', 'guest_schedule', 'remote_mkdir', 'upload',
                        'unpack', 'before_worker', 'worker', 'before_retrieve', 'pack_capture', 'download', 'guarded_stop']
            need([row['name'] for row in rows] == expected
                 and all(row['exit_code'] == (1 if row['name'] == 'worker' else 0) for row in rows), 'EU exact process sequence')
            mark = fields(get(prefix + 'guardmarks/double_guard_verified'))
            need(mark == dict(schema='e-hgp.guard-mark.v1', mark='double_guard_verified', **target,
                generation=generation, max_run_seconds='3600', guest_shutdown_minutes='30', date_utc='2026-09-11T14:00:39Z'), 'doubleguard exact mark')
            handoff = obj(prefix + 'handoff.json')
            need(handoff['schema'] == 'e-hgp.start-handoff.v3' and handoff['status'] == 'targeted_running'
                 and handoff['last_start_timestamp'] == generation and handoff['guest_shutdown_minutes'] == 30
                 and all(handoff[key] == value for key, value in target.items()), 'versioned handoff')
            archive_bytes = get(prefix + 'capture.tar.gz')
            need(sha(archive_bytes) == receipt['capture_sha256'], 'received capture hash')
            with tarfile.open(fileobj=io.BytesIO(archive_bytes), mode='r:gz') as archive:
                members = archive.getmembers()
                expected_files = {'output/receipt.json', 'output/sources_before.json', 'output/sources_after.json',
                                  'output/guard_evidence.json', 'output/external_dependencies_after.json'}
                need({m.name for m in members if m.isfile()} == expected_files and len(members) == 6,
                     'only early worker receipt/source/guard files, no executable or benchmark')
                for member in members:
                    relative(member.name)
                    need(member.isfile() or member.isdir(), 'archive regular type')
                    if member.isfile():
                        stream = archive.extractfile(member)
                        need(stream is not None and stream.read() == get(prefix + 'received/' + member.name), 'raw archive bytes equal extracted receipt')
            guest_prefix = prefix + 'received/output/'
            guest = obj(guest_prefix + 'receipt.json')
            need(guest['status'] == 'failed' and guest['error'] == 'ValueError: existing tools required, no installation'
                 and guest['commands'] == guest['runs'] == [] and guest['binaries'] == {}
                 and guest['GCP_used'] is True and guest['FULL_GPU_available'] is False
                 and guest['contract_qualified'] is False and guest['VM_shutdown_certified_by_worker'] is False
                 and guest['target'] == EU and guest['generation'] == generation and guest['worker_sha256'] == WORKER,
                 'no command/compilation/kernel/benchmark, missing tool not individually identified')
            need(guest['available_cpus'] == list(range(48)) and guest['sources_stable'] is True, 'guest CPU affinity/source stability')
            sources = obj(prefix + 'source_manifest.json')
            need(len(sources) == 59 and obj(guest_prefix + 'sources_before.json') == sources
                 and obj(guest_prefix + 'sources_after.json') == sources
                 and obj(guest_prefix + 'external_dependencies_after.json') == {}, 'actual unchanged uploaded source closure')
            evidence = obj(guest_prefix + 'guard_evidence.json')
            need(evidence['mark'] == mark and evidence['metadata'] == guest['guest_metadata'] == dict(EU, machine='g4-standard-48')
                 and evidence['bounds'] == guest['guards'], 'guest guard evidence linked to exact target')
            schedule = fields(get(prefix + 'guest_schedule.stdout'))
            need(evidence['schedule'] == schedule and schedule['MODE'] == 'poweroff', 'unchanged guest shutdown')
            started, verified = epoch(generation), epoch(mark['date_utc'])
            deadline = int(schedule['USEC']) / 1000000
            need(started < verified < rows[8]['started_epoch'] < stop['ended_epoch'] < deadline
                 and deadline <= started + 3600 - 300 and 30 * 60 + 900 <= 3600, 'guard chronology and preserved stop margin')
            need(guest['guards']['closing_margin_seconds'] == 300
                 and guest['guards']['safe_gce_deadline_epoch'] == started + 3300
                 and guest['guards']['guest_deadline_epoch'] == deadline
                 and guest['guards']['work_deadline_epoch'] == receipt['session_deadline_epoch'] - 300
                 and guest['effective_work_deadline_epoch'] <= guest['guards']['work_deadline_epoch'], 'bounds exact')
            for stage in ('before_upload', 'before_worker', 'before_retrieve'):
                view = obj(prefix + stage + '.stdout.safety_projection.json')
                need(view['name'] == EU['instance'] and view['zone'].endswith('/' + EU['zone'])
                     and '/projects/' + PROJECT + '/' in view['selfLink'] and view['lastStartTimestamp'] == generation
                     and view['status'] == 'RUNNING' and view['machineType'].endswith('/g4-standard-48')
                     and view['labels']['project'] == 'e-hgp', 'explicit metadata projection target/type')
                scheduling = view['scheduling']
                need(scheduling['provisioningModel'] == 'SPOT' and scheduling['preemptible'] is True
                     and scheduling['automaticRestart'] is False and scheduling['instanceTerminationAction'] == 'STOP'
                     and scheduling['maxRunDuration'] == {'seconds': '3600', 'nanos': 0}
                     and abs(epoch(view['terminationTimestamp']) - started - 3600) <= 300, 'recorded GCE safeguard projection')

    binding = obj('captures/session_eu_r1/wrapper_binding.json')
    qualified_binding = obj('qualification/session_wrapper_binding.json')
    need(binding == qualified_binding and binding['wrapper_sha256'] == WRAPPER
         and binding['controller_sha256'] == CONTROLLER and binding['original_target'] == US and binding['target'] == EU
         and binding['changed_controller_globals'] == ['TARGET'] and binding['controller_receipt_identifies_wrapper'] is False,
         'separate wrapper binding, same values despite formatting')
    need(get('captures/session_eu_r1/wrapper_binding.json') != get('qualification/session_wrapper_binding.json'),
         'preserved binding reformatting, not falsely byte-identical')
    qualification = obj('qualification/receipt.json')
    need(qualification['status'] == 'passed' and qualification['checks'] == qualification['rejections'] == 13
         and qualification['engine_tests'] == 0 and qualification['GCP_used'] is False
         and qualification['wrapper_sha256'] == WRAPPER and qualification['controller_sha256'] == CONTROLLER,
         'wrapper qualification pure scope')
    qmanifest = obj('qualification/manifest.json')
    for name, pin in qmanifest.items():
        need(sha(get('qualification/' + name)) == pin, 'original qualification manifest')
    before = obj('qualification/sources_before.json')
    need(before == obj('qualification/sources_after.json') and len(before) == 7, 'wrapper sources stable')
    for name, pin in before.items():
        need(sha(get('qualification/' + Path(name).name)) == pin, 'qualified source bytes')
    need(sha(get('qualification/terminal_batch_session_eu_v7.py')) == WRAPPER
         and sha(get('qualification/full_probe_session_v7.py')) == CONTROLLER
         and sha(get('qualification/start_and_verify.sh')) == START
         and sha(get('qualification/stop_and_verify.sh')) == STOP, 'unchanged safety chain source pins')
    need([row['name'] for row in qualification['commands']] == ['normal', 'optimized', 'inert'], 'wrapper test sequence')
    for row in qualification['commands']:
        command('qualification/', row)
        need(row['exit_code'] == row['expected_exit'] == 0, 'wrapper pure process pass')
        if row['name'] == 'inert':
            need('--execute' not in row['argv'], 'inert path never cloud execute')
        else:
            summary = obj('qualification/' + row['name'] + '.stdout')
            need(summary['checks'] == summary['rejections'] == 13 and summary['real_subprocesses'] == 0
                 and summary['GCP_used'] is False and summary['target'] == EU, 'fixed pure wrapper nonvacuity')
            need(('-O' in row['argv']) == (row['name'] == 'optimized'), 'normal/-O wrapper mode')

    diagnostic = obj('captures/preemption_diagnostics_r1/receipt.json')
    need(diagnostic['read_only'] is True and len(diagnostic['commands']) == 2, 'read-only preemption observation')
    for row in diagnostic['commands']:
        need(row['exit_code'] == 0, 'diagnostic command exit')
        for suffix in ('stdout', 'stderr'):
            need(sha(get('captures/preemption_diagnostics_r1/' + row['name'] + '.' + suffix)) == row[suffix + '_sha256'], 'diagnostic stream pin')
    logging = obj('captures/preemption_diagnostics_r1/logging.stdout')
    need(any(row['protoPayload']['methodName'] == 'compute.instances.preempted' and
             row['protoPayload']['resourceName'].endswith('/' + US['instance']) for row in logging), 'first-window independent preemption event')

    closure = obj('captures/session_key_closure_r1/receipt.json')
    need(closure['status'] == 'completed' and closure['exact_oslogin_key_revoked'] is True
         and closure['exact_private_public_key_files_removed'] is True and closure['private_key_body_read'] is False
         and closure['other_VM_mutations'] is False and closure['other_active_instances'] == [], 'targeted key/resource closure')
    for session, row in closure['sessions'].items():
        original = session_receipts[session]
        need(row['receipt_sha256'] == sha(get('captures/' + session + '/full_host/receipt.json'))
             and row['generation'] == original['generation'] and row['target'] == original['target'], 'closure binds exact three session receipts')
    need(set(closure['sessions']) == set(GENERATIONS), 'exact session closure domain')
    need(closure['commands'] == obj('captures/session_key_closure_r1/commands.json')
         and [row['name'] for row in closure['commands']] == ['revoke_exact_key', 'labelled_inventory'], 'closure commands')
    for row in closure['commands']:
        need(row['exit_code'] == 0 and row['started_epoch'] > max(r['commands'][-1]['ended_epoch'] for r in session_receipts.values()),
             'closure follows every targeted shutdown')
        for suffix in ('stdout', 'stderr'):
            need(sha(get('captures/session_key_closure_r1/' + row['name'] + '.' + suffix)) == row[suffix + '_sha256'], 'closure streams')
    revoke = closure['commands'][0]['argv']
    need(revoke[1:5] == ['compute', 'os-login', 'ssh-keys', 'remove'], 'exact key revocation action')
    key_argument = next(item for item in revoke if item.startswith('--key-file='))
    need(all(key_argument in r['commands'][0]['argv'] for r in session_receipts.values()), 'same exact registered public key revoked')
    final_inventory = obj('captures/session_key_closure_r1/labelled_inventory.stdout')
    need(len(final_inventory) == 3 and all(row['status'] == 'TERMINATED' for row in final_inventory), 'historical all-labelled inventory stopped')
    for target, generation in ((US, GENERATIONS['session_r2']), (EU, GENERATIONS['session_eu_r1'])):
        rows = [row for row in final_inventory if row['name'] == target['instance'] and row['zone'].endswith('/' + target['zone'])]
        need(len(rows) == 1 and rows[0]['lastStartTimestamp'] == generation, 'final inventory latest exact generation')
    if args.extract:
        destination = args.extract.resolve()
        need(not destination.exists() and not destination.is_relative_to(ROOT), 'fresh extraction outside packet')
        destination.mkdir(parents=True)
        for name, raw in data.items():
            target = destination / relative(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(raw)
        need(all((destination / name).read_bytes() == raw for name, raw in data.items()), 'exact published bytes extracted')
    print(json.dumps(dict(status='passed', sessions=3, targeted_shutdowns_certified=3, US_preemptions_reported=2,
                          EU_doubleguard_verified=True, guest_commands=0, compiled=False, device_executed=False,
                          benchmarks=0, exact_oslogin_key_revoked=True, personal_or_duplicate_omissions=len(omissions),
                          wrapper_binding_values_equal=True, wrapper_binding_bytes_equal=False,
                          extracted=bool(args.extract), reader_GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
