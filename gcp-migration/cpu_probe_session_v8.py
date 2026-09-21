#!/usr/bin/env python3
"""Fixed SPOT v8 CPU pilot; inert without --execute. No GPU/FULL claim.

Explicit lifecycle port from full_probe_session_v7.py at SHA177b25a0...
Only its hash-checked parsing/process/lifecycle helpers are reused. Neither
its run_session nor its v7 snapshot/FULL validation is called or modified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import secrets
import shlex
import shutil
import signal
import sys
import tarfile
import time
import types
import cpu_probe_worker_v8 as payload

HERE = Path(__file__).resolve().parent
TARGET = dict(payload.TARGET)
LEGACY_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
raw = (HERE / 'full_probe_session_v7.py').read_bytes()
if hashlib.sha256(raw).hexdigest() != LEGACY_SHA:
    raise ValueError('pinned lifecycle helper changed')
legacy = types.ModuleType('mhgp8_pinned_lifecycle_primitives')
legacy.__file__ = str(HERE / 'full_probe_session_v7.py')
exec(compile(raw, legacy.__file__, 'exec'), legacy.__dict__)
GUARDS = dict(legacy.GUARDS)
need, sha, save, unique, fields, epoch, target = (
    legacy.need, legacy.sha, legacy.save, legacy.unique, legacy.fields, legacy.epoch, legacy.target)
generation_from_records, closure_generation = legacy.generation_from_records, legacy.closure_generation
Commands, extract_capture = legacy.Commands, legacy.extract_capture


def validate_target(value, status, generation=None):
    need(type(value) is dict and value.get('name') == TARGET['instance'] and
         value.get('zone', '').rsplit('/', 1)[-1] == TARGET['zone'] and
         value.get('selfLink', '').endswith('/projects/' + TARGET['project'] + '/zones/' +
             TARGET['zone'] + '/instances/' + TARGET['instance']) and
         value.get('status') == status and value.get('labels', {}).get('project') == 'e-hgp' and
         value.get('machineType', '').rsplit('/', 1)[-1] == 'g4-standard-48', 'fixed G4 target/status')
    scheduling = value.get('scheduling', {})
    duration = scheduling.get('maxRunDuration', {})
    need(scheduling.get('provisioningModel') == 'SPOT' and scheduling.get('instanceTerminationAction') == 'STOP' and
         scheduling.get('onHostMaintenance') == 'TERMINATE' and scheduling.get('automaticRestart') is False and
         str(duration.get('seconds')) == '3600' and duration.get('nanos', 0) == 0, 'fixed SPOT/STOP/3600s')
    if generation is not None:
        need(value.get('lastStartTimestamp') == generation, 'current target/generation mismatch')


def guard_deadline(mark, schedule, generation, now):
    need(mark.get('max_run_seconds') == '3600' and mark.get('guest_shutdown_minutes') == '30', 'v8 guard durations')
    return legacy.guard_deadline(mark, schedule, generation, now)


def validate_snapshot(path, manifest):
    payload.validate_manifest(manifest)
    observed, names = {}, set()
    with tarfile.open(path, 'r:*') as archive:
        for member in archive.getmembers():
            name = member.name.rstrip('/') if member.isdir() else member.name
            parts = PurePosixPath(name)
            allowed_dir = member.isdir() and (name in ('morsehgp3D_v8', 'data', 'gcp-migration') or
                                               name.startswith('morsehgp3D_v8/') or name.startswith('data/'))
            need(name and len(name) <= 4096 and str(parts) == name and not parts.is_absolute() and
                 all(p not in ('.', '..') for p in parts.parts) and name not in names and
                 (allowed_dir or (member.isfile() and payload.safe_name(name))), 'unsafe v8 archive member')
            names.add(name)
            if member.isfile():
                need(name in manifest, 'unmanifested payload')
                observed[name] = hashlib.sha256(archive.extractfile(member).read()).hexdigest()
        need(observed == manifest, 'snapshot differs from manifest')
        plan = payload.strict_json(archive.extractfile(payload.PLAN).read())
        cases = payload.validate_plan(plan, manifest)
        for case in cases:
            size = archive.getmember(case['file']).size
            need(size % 6 == 0 and case['n'] <= size // 6, 'u16le input/prefix size')


def validate_received(output, manifest, worker_pin):
    value = payload.strict_json((output / 'receipt.json').read_text())
    need(value.get('status') == 'completed' and value.get('sources_stable') is True and
         value.get('compiled_dependencies_stable') is True and
         value.get('worker_sha256') == worker_pin and value.get('GPU_executed') is False and
         value.get('FULL_executed') is False and value.get('contract_certified') is False, 'worker completion/scope')
    dependencies_path = output / 'compiled_dependencies.json'
    need(sha(dependencies_path) == value.get('compiled_dependency_manifest_sha256'), 'compiled dependencies manifest pin')
    dependencies = payload.strict_json(dependencies_path.read_text())
    need(type(dependencies) is dict and dependencies and all(type(name) is str and Path(name).is_absolute() and
         type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin) for name, pin in dependencies.items()),
         'compiled dependency inventory')
    for name in ('sources_before.json', 'sources_after.json'):
        need(payload.strict_json((output / name).read_text()) == manifest, 'worker payload closure')
    need(value.get('commands') and all(row.get('exit_code') == 0 and row.get('group_closed') is True and
         not row.get('residual_or_interrupted_group_killed') for row in value['commands']), 'worker command closure')
    records = []
    for path in sorted(output.glob('*.command.json')):
        row = payload.strict_json(path.read_text())
        stem = path.name[:-len('.command.json')]
        need(sha(output / (stem+'.stdout')) == row['stdout_sha256'] and
             sha(output / (stem+'.stderr')) == row['stderr_sha256'], 'worker raw log hash')
        need(type(row.get('argv')) is list and row['argv'], 'worker invocation')
        records.append(row)
    canonical = lambda rows: sorted(json.dumps(row, sort_keys=True) for row in rows)
    need(canonical(records) == canonical(value['commands']), 'worker command list/raw receipts mismatch')


def run_session(args):
    session = args.session_dir.absolute()
    key = args.ssh_key.absolute()
    need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700, 'existing private session 0700')
    need(key.is_file() and not key.is_symlink() and key.stat().st_mode & 0o777 == 0o600 and
         Path(str(key) + '.pub').is_file() and not Path(str(key) + '.pub').is_symlink(), 'existing private/public key files')
    need(sha(__file__) == args.expected_controller_sha256, 'controller pin')
    need(args.worker_sha256 == sha(payload.__file__), 'worker must match this v8 adapter')
    for name, pin in GUARDS.items():
        need(sha(HERE / name) == pin, 'guard pin ' + name)
    inputs = [(args.snapshot, args.snapshot_sha256, 'snapshot.tar.gz'),
              (args.manifest, args.manifest_sha256, 'source_manifest.json'), (args.worker, args.worker_sha256, 'worker.py')]
    for path, pin, _ in inputs:
        need(path.is_file() and not path.is_symlink() and re.fullmatch('[0-9a-f]{64}', pin) and sha(path) == pin, 'input pin')
    manifest = json.loads(args.manifest.read_text(), object_pairs_hook=unique)
    validate_snapshot(args.snapshot, manifest)
    host = session / 'cpu_v8_host'
    host.mkdir(mode=0o700, exist_ok=False)
    marks = host / 'guardmarks'
    marks.mkdir(mode=0o700)
    for path, pin, name in inputs:
        shutil.copyfile(path, host / name)
        need(sha(host / name) == pin, 'copied input pin')
    for name in GUARDS:
        shutil.copyfile(HERE / name, host / name)
        need(sha(host / name) == GUARDS[name], 'copied guard pin')
        (host / name).chmod(0o700)
    env = dict(os.environ, GCP_PROJECT_ID=TARGET['project'], GCP_ZONE=TARGET['zone'],
               GCP_INSTANCE_NAME=TARGET['instance'], GCP_SSH_KEY_FILE=str(key),
               PATH=str(args.gcloud.parent) + os.pathsep + os.environ.get('PATH', ''))
    commands = Commands(host, env)
    state = dict(status='failed', target=TARGET, public_status='not_claimed', targeted_shutdown_certified=False,
                 backend='cpu_reference', GPU_executed=False, FULL_executed=False, useful_budget_seconds=900,
                 snapshot_sha256=args.snapshot_sha256, manifest_sha256=args.manifest_sha256,
                 worker_sha256=args.worker_sha256, controller_sha256=args.expected_controller_sha256)
    common = None
    remote = None
    worker_attempted = False
    generation = None
    deadline = monotonic_deadline = None
    previous_handlers = {}

    def interrupted(signum, _frame):
        raise InterruptedError('host signal ' + str(signum))

    def read_generation():
        handoff = json.loads((host / 'handoff.json').read_text(), object_pairs_hook=unique) if (host / 'handoff.json').exists() else None
        lifecycle = fields((host / 'lifecycle.txt').read_text()) if (host / 'lifecycle.txt').exists() else None
        return generation_from_records(handoff, lifecycle)

    def ssh(name, script, timeout=90):
        return commands.run(name, [args.gcloud, 'compute', 'ssh', TARGET['instance'], *common,
                                  '--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15',
                                  '--command=' + script], timeout=timeout)

    def recertify(name, timeout=60):
        rc, raw, _ = commands.run(name, [args.gcloud, 'compute', 'instances', 'describe', TARGET['instance'],
                                '--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--format=json'], timeout=timeout)
        need(rc == 0, 'target unreadable')
        value = json.loads(raw, object_pairs_hook=unique)
        validate_target(value, 'RUNNING', generation)

    def remaining(reserve, cap):
        need(deadline is not None and monotonic_deadline is not None, 'no certified session deadline')
        value = min(deadline-reserve-time.time(), monotonic_deadline-reserve-time.monotonic(), cap)
        need(value > 0, 'session window ended; stop without further recovery')
        return value

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous_handlers[sig] = signal.signal(sig, interrupted)
        rc, raw, _ = commands.run('before_start', [args.gcloud, 'compute', 'instances', 'describe',
            TARGET['instance'], '--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--format=json'])
        need(rc == 0, 'pre-start target unreadable')
        validate_target(json.loads(raw, object_pairs_hook=unique), 'TERMINATED')
        # Only this public key is registered; never export/read the private key.
        rc, _, _ = commands.run('oslogin_add', [args.gcloud, 'compute', 'os-login', 'ssh-keys', 'add',
            '--project=' + TARGET['project'], '--key-file=' + str(key) + '.pub', '--ttl=70m', '--format=json', '--quiet'])
        need(rc == 0, 'OS Login registration failed')
        rc, start_output, _ = commands.run('guarded_start', [host / 'start_and_verify.sh', '--yes',
            '--guest-shutdown-minutes', '30', '--handoff-file', host / 'handoff.json',
            '--lifecycle-state-file', host / 'lifecycle.txt', '--guard-mark-dir', marks], timeout=None, guard=True)
        generation = read_generation()
        need(rc == 0 and generation is not None, 'start not certified')
        expiration = re.findall(r'expiration fixe=(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{6}Z)', start_output)
        need(len(expiration) == 1 and epoch(expiration[0]) > time.time(), 'exact SSH expiration not certified')
        common = ['--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--quiet',
                  '--ssh-key-file=' + str(key), '--ssh-key-expiration=' + expiration[0]]
        mark_path = marks / 'double_guard_verified'
        mark = fields(mark_path.read_text())
        target(mark)
        need(mark.get('mark') == 'double_guard_verified' and mark.get('generation') == generation, 'double guard before upload')
        recertify('before_upload')
        rc, schedule_raw, _ = ssh('guest_schedule', 'sudo -n cat /run/systemd/shutdown/scheduled')
        need(rc == 0, 'guest schedule unreadable')
        deadline = guard_deadline(mark, fields(schedule_raw), generation, time.time())
        monotonic_deadline = time.monotonic() + deadline-time.time()
        state.update(generation=generation, session_deadline_epoch=deadline, ssh_expiration=expiration[0])
        nonce = secrets.token_hex(8)
        prefix = '/tmp/ehgp-cpu-v8-' + nonce + '.'
        rc, raw, _ = ssh('remote_mkdir', 'umask 077; mktemp -d ' + shlex.quote(prefix + 'XXXXXXXXXX'))
        need(rc == 0 and re.fullmatch(re.escape(prefix) + r'[A-Za-z0-9]{10}\n?', raw), 'fresh remote path')
        remote = raw.strip()
        state['remote_directory'] = remote
        rc, _, _ = commands.run('upload', [args.gcloud, 'compute', 'scp', *common, host / 'snapshot.tar.gz',
            host / 'source_manifest.json', host / 'worker.py', mark_path, TARGET['instance'] + ':' + remote + '/'], timeout=180)
        need(rc == 0, 'upload failed')
        pins = [(args.snapshot_sha256, 'snapshot.tar.gz'), (args.manifest_sha256, 'source_manifest.json'),
                (args.worker_sha256, 'worker.py'), (sha(mark_path), 'double_guard_verified')]
        checks = ' && '.join('test "$(sha256sum ' + shlex.quote(remote + '/' + name) +
                            ' | cut -d " " -f 1)" = ' + shlex.quote(pin) for pin, name in pins)
        # The tar was fully inspected locally and its transported bytes are
        # rehashed before extraction into a fresh, private source directory.
        rc, _, _ = ssh('unpack', checks + ' && mkdir -m 700 ' + shlex.quote(remote + '/source') +
            ' && tar --no-same-owner --no-same-permissions -xzf ' + shlex.quote(remote + '/snapshot.tar.gz') +
            ' -C ' + shlex.quote(remote + '/source'))
        need(rc == 0, 'remote pins/extraction failed')
        recertify('before_worker')
        argv = ['python3', remote + '/worker.py', '--source-root', remote + '/source',
                '--source-manifest', remote + '/source_manifest.json', '--source-manifest-sha256', args.manifest_sha256,
                '--guard-mark', remote + '/double_guard_verified', '--guard-mark-sha256', sha(mark_path),
                '--generation', generation, '--session-deadline-epoch', str(deadline), '--closing-margin-seconds', '300',
                '--output', remote + '/output']
        for key_name, value in TARGET.items():
            argv += ['--' + key_name, value]
        argv += ['--execute', '--useful-budget-seconds', '900']
        worker_attempted = True
        rc, _, _ = ssh('worker', 'exec ' + shlex.join(argv), timeout=remaining(240, 930))
        state['worker_exit_code'] = rc
        state['status'] = 'completed' if rc == 0 else 'worker_failed'
    except BaseException as error:
        state.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        # Repeated signals cannot interrupt recovery or the targeted stop.
        for sig in previous_handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            if worker_attempted and remote is not None and common is not None:
                try:
                    recertify('before_retrieve', timeout=remaining(60, 60))
                    script = ('tar -czf ' + shlex.quote(remote + '/capture.tar.gz') + ' --exclude=output/build -C ' +
                              shlex.quote(remote) + ' output; capture_code=$?; sha256sum ' +
                              shlex.quote(remote + '/capture.tar.gz') + '; exit "$capture_code"')
                    pack_rc, raw, _ = ssh('pack_capture', script, timeout=remaining(60, 90))
                    matches = re.findall(r'^([0-9a-f]{64})  ' + re.escape(remote + '/capture.tar.gz') + r'$', raw, re.M)
                    need(len(matches) == 1, 'capture tar hash absent')
                    state['capture_pack_exit_code'] = pack_rc
                    rc, _, _ = commands.run('download', [args.gcloud, 'compute', 'scp', *common,
                        TARGET['instance'] + ':' + remote + '/capture.tar.gz', host / 'capture.tar.gz'], timeout=remaining(60, 180))
                    need(rc == 0 and sha(host / 'capture.tar.gz') == matches[0], 'capture download/hash')
                    extract_capture(host / 'capture.tar.gz', host / 'received')
                    state.update(capture_sha256=matches[0], capture_received=True,
                                 worker_receipt_present=(host / 'received/output/receipt.json').is_file())
                    if pack_rc != 0 or not state['worker_receipt_present']:
                        state['status'] = 'capture_incomplete'
                    elif state['status'] == 'completed':
                        validate_received(host / 'received/output', manifest, args.worker_sha256)
                except BaseException as error:
                    state.update(status='capture_failed', capture_error=type(error).__name__ + ': ' + str(error))
        finally:
            # Never infer a generation from a new GCE read. Unknown/ambiguous
            # lifecycle requires human recovery, not an unversioned stop.
            try:
                try:
                    closing_generation = closure_generation(generation, read_generation())
                except (OSError, UnicodeError, json.JSONDecodeError):
                    closing_generation = closure_generation(generation, None, unreadable=True)
                if closing_generation is not None:
                    rc, _, _ = commands.run('guarded_stop', [host / 'stop_and_verify.sh', '--yes',
                        '--expected-last-start-timestamp', closing_generation], timeout=None, critical=True, guard=True)
                    need(rc == 0, 'targeted stop not certified')
                    state['targeted_shutdown_certified'] = True
                    state['generation'] = closing_generation
                else:
                    state['no_start_lifecycle_created'] = True
            except BaseException as error:
                state.update(status='shutdown_uncertified', shutdown_error=type(error).__name__ + ': ' + str(error))
            state['commands'] = commands.rows
            try:
                save(host / 'receipt.json', state)
            except OSError as error:
                print('Host receipt write failed: ' + str(error), file=sys.stderr)
            for sig, handler in previous_handlers.items():
                signal.signal(sig, handler)
    print(json.dumps(state, sort_keys=True), flush=True)
    return 74 if state['status'] == 'shutdown_uncertified' else (0 if state['status'] == 'completed' else 1)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot', 'manifest', 'worker', 'session-dir', 'ssh-key'):
        parser.add_argument('--' + name, type=Path)
    for name in ('snapshot-sha256', 'manifest-sha256', 'worker-sha256', 'expected-controller-sha256'):
        parser.add_argument('--' + name)
    parser.add_argument('--gcloud', type=Path, default=Path('/home/codespace/google-cloud-sdk/bin/gcloud'))
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, guest_minutes=30, gce_seconds=3600,
                              useful_seconds=900, GPU_executed=False, FULL_executed=False), sort_keys=True))
        return 0
    need(all(getattr(args, name.replace('-', '_')) is not None for name in (
        'snapshot', 'manifest', 'worker', 'session-dir', 'ssh-key', 'snapshot-sha256',
        'manifest-sha256', 'worker-sha256', 'expected-controller-sha256')), 'missing execution argument')
    return run_session(args)


if __name__ == '__main__':
    raise SystemExit(main())
