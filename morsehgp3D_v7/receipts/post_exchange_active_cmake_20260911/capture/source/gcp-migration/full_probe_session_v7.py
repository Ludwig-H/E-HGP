#!/usr/bin/env python3
"""One fixed SPOT FULL CPU session. Inert without --execute; never a raw start.

The worker has only its mandatory session deadline, not the historical F 120s
watchdog. Lifecycle closure is independent of artifact/log success. No GPU run.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import secrets
import shlex
import shutil
import signal
import subprocess
import sys
import tarfile
import time

HERE = Path(__file__).resolve().parent
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b', instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
GUARDS = {'start_and_verify.sh': '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0',
          'stop_and_verify.sh': 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n')


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate field')
        result[key] = value
    return result


def fields(text):
    pairs = []
    for line in text.splitlines():
        key, separator, value = line.partition('=')
        need(separator == '=', 'malformed guard field')
        pairs.append((key, value))
    return unique(pairs)


def epoch(value):
    need(type(value) is str and re.fullmatch(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,6})?(?:Z|[+-]\d\d:\d\d)', value), 'RFC3339 generation')
    return datetime.fromisoformat(value.replace('Z', '+00:00')).timestamp()


def target(record):
    need(all(record.get(key) == value for key, value in TARGET.items()), 'different target')


def generation_from_records(handoff, lifecycle):
    generations = []
    if handoff is not None:
        target(handoff)
        need(handoff.get('schema') == 'e-hgp.start-handoff.v3', 'handoff schema')
        generations.append(handoff['last_start_timestamp'])
    if lifecycle is not None:
        target(lifecycle)
        need(lifecycle.get('schema') == 'e-hgp.lifecycle-state.v1', 'lifecycle schema')
        need(lifecycle.get('state') in ('start_may_have_been_requested', 'targeted_running', 'targeted_stopping',
                                       'targeted_stopped', 'targeted_stop_failed'), 'lifecycle state')
        if lifecycle.get('generation'):
            generations.append(lifecycle['generation'])
    if not generations:
        need(handoff is None and lifecycle is None, 'start may have happened but generation is unknown')
        return None
    need(len(set(generations)) == 1, 'ambiguous generation; never guess')
    epoch(generations[0])
    return generations[0]


def closure_generation(known, observed, unreadable=False):
    if known is not None:
        epoch(known)
    if observed is not None:
        epoch(observed)
    need(not unreadable or (known is not None and observed is None), 'unreadable generation without prior proof')
    need(known is None or observed is None or known == observed, 'conflicting generation; no fallback')
    return observed if observed is not None else known


def guard_deadline(mark, schedule, generation, now):
    target(mark)
    need(mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
         mark.get('generation') == generation and mark.get('guest_shutdown_minutes') == '30', 'double guard identity')
    duration = int(mark['max_run_seconds'])
    need(2700 <= duration <= 28800 and epoch(generation) <= epoch(mark['date_utc']) <= now + 5, 'guard duration/chronology')
    need(schedule.get('MODE') == 'poweroff' and re.fullmatch('[0-9]{1,18}', schedule.get('USEC', '')), 'guest shutdown')
    deadline = int(schedule['USEC']) // 1000000
    need(now + 300 < deadline <= epoch(generation) + duration - 300, 'session deadline')
    return deadline


def archive_members(archive, prefix):
    """No absolute/traversal/ambiguous paths, links, devices or duplicate names."""
    seen = set()
    for member in archive.getmembers():
        name = member.name.rstrip('/') if member.isdir() else member.name
        path = PurePosixPath(name)
        need(name and len(name) <= 4096 and str(path) == name and not path.is_absolute() and
             all(part not in ('..', '.') for part in path.parts) and
             (name == prefix or name.startswith(prefix + '/')) and
             (member.isdir() or member.isfile()) and name not in seen, 'unsafe archive member: ' + name)
        seen.add(name)
        yield member, path


def validate_snapshot(path, manifest):
    need(type(manifest) is dict and 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp' in manifest, 'source manifest')
    observed = {}
    with tarfile.open(path, 'r:*') as archive:
        for member, name in archive_members(archive, 'morsehgp3D_v7'):
            if member.isfile():
                need(str(name) in manifest and re.fullmatch('[0-9a-f]{64}', manifest[str(name)]), 'unexpected source')
                observed[str(name)] = hashlib.sha256(archive.extractfile(member).read()).hexdigest()
    need(observed == manifest, 'snapshot differs from manifest')


def extract_capture(path, destination):
    destination.mkdir(mode=0o700, exist_ok=False)
    with tarfile.open(path, 'r:*') as archive:
        members = list(archive_members(archive, 'output'))
        for member, name in members:
            output = destination / str(name)
            if member.isdir():
                output.mkdir(mode=0o700, parents=True, exist_ok=True)
            else:
                output.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
                with output.open('xb') as stream:
                    shutil.copyfileobj(archive.extractfile(member), stream)


class Commands:
    def __init__(self, directory, env):
        self.directory, self.env, self.rows = directory, env, []

    def run(self, name, argv, timeout=90, critical=False, guard=False):
        row = dict(name=name, argv=list(map(str, argv)), started_epoch=time.time(), timeout_seconds=timeout)
        try:
            save(self.directory / (name + '.intent.json'), row)
        except OSError:
            if not critical:
                raise
        process = None
        out, err = b'', b''
        try:
            process = subprocess.Popen(row['argv'], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, env=self.env, start_new_session=True)
            row['pid'] = process.pid
            try:
                print(name + ': PID ' + str(process.pid), flush=True)
            except OSError:
                if not critical:
                    raise
            out, err = process.communicate(timeout=timeout)
        finally:
            if process is not None:
                handlers = {sig: signal.signal(sig, signal.SIG_IGN) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
                try:
                    try:
                        os.killpg(process.pid, 0)
                        alive = True
                    except ProcessLookupError:
                        alive = False
                    if alive:
                        row['cleanup_needed'] = True
                        # First notify ONLY the guarded shell leader, never its
                        # stop child. The 900s allowance is trap/control cleanup,
                        # not a benchmark quota. Orphan groups are checked even
                        # when their leader has already exited.
                        try:
                            if guard:
                                if process.poll() is None:
                                    os.kill(process.pid, signal.SIGTERM)
                            else:
                                os.killpg(process.pid, signal.SIGTERM)
                        except ProcessLookupError:
                            pass
                        cleanup_end = time.monotonic() + (900 if guard else 10)
                        try:
                            out, err = process.communicate(timeout=max(0.01, cleanup_end-time.monotonic()))
                        except subprocess.TimeoutExpired:
                            pass
                        while True:
                            try:
                                os.killpg(process.pid, 0)
                            except ProcessLookupError:
                                break
                            if time.monotonic() >= cleanup_end:
                                row['last_resort_group_killed'] = True
                                os.killpg(process.pid, signal.SIGKILL)
                                out, err = process.communicate()
                                break
                            time.sleep(0.05)
                    process.wait()
                    try:
                        os.killpg(process.pid, 0)
                        row['group_closed'] = False
                    except ProcessLookupError:
                        row['group_closed'] = True
                finally:
                    for sig, handler in handlers.items():
                        signal.signal(sig, handler)
                row['exit_code'] = process.returncode
            row['ended_epoch'] = time.time()
            self.rows.append(row)
            try:
                (self.directory / (name + '.stdout')).write_bytes(out)
                (self.directory / (name + '.stderr')).write_bytes(err)
                row.update(stdout_sha256=hashlib.sha256(out).hexdigest(), stderr_sha256=hashlib.sha256(err).hexdigest())
                save(self.directory / (name + '.command.json'), row)
            except OSError as error:
                row['log_error'] = str(error)
                if not critical:
                    raise
        need(row.get('group_closed') and not row.get('last_resort_group_killed'), 'command group not cleanly closed')
        return row.get('exit_code'), out.decode(errors='replace'), err.decode(errors='replace')


def run_session(args):
    session = args.session_dir.absolute()
    key = args.ssh_key.absolute()
    need(session.is_dir() and not session.is_symlink() and session.stat().st_mode & 0o777 == 0o700, 'existing private session 0700')
    need(key.is_file() and not key.is_symlink() and key.stat().st_mode & 0o777 == 0o600 and
         Path(str(key) + '.pub').is_file() and not Path(str(key) + '.pub').is_symlink(), 'existing private/public key files')
    need(sha(__file__) == args.expected_controller_sha256, 'controller pin')
    for name, pin in GUARDS.items():
        need(sha(HERE / name) == pin, 'guard pin ' + name)
    inputs = [(args.snapshot, args.snapshot_sha256, 'snapshot.tar.gz'),
              (args.manifest, args.manifest_sha256, 'source_manifest.json'), (args.worker, args.worker_sha256, 'worker.py')]
    for path, pin, _ in inputs:
        need(path.is_file() and not path.is_symlink() and re.fullmatch('[0-9a-f]{64}', pin) and sha(path) == pin, 'input pin')
    manifest = json.loads(args.manifest.read_text(), object_pairs_hook=unique)
    validate_snapshot(args.snapshot, manifest)
    host = session / 'full_host'
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
        need(value['status'] == 'RUNNING' and value['lastStartTimestamp'] == generation and
             value['labels']['project'] == 'e-hgp', 'current target/generation mismatch')

    def remaining(reserve, cap):
        need(deadline is not None and monotonic_deadline is not None, 'no certified session deadline')
        value = min(deadline-reserve-time.time(), monotonic_deadline-reserve-time.monotonic(), cap)
        need(value > 0, 'session window ended; stop without further recovery')
        return value

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous_handlers[sig] = signal.signal(sig, interrupted)
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
        prefix = '/tmp/ehgp-full-v7-' + nonce + '.'
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
        if args.bootstrap:
            argv += ['--bootstrap']
        worker_attempted = True
        rc, _, _ = ssh('worker', 'exec ' + shlex.join(argv), timeout=remaining(240, float('inf')))
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
                    script = ('tar -czf ' + shlex.quote(remote + '/capture.tar.gz') + ' --exclude=output/full_probe -C ' +
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
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('snapshot-sha256', 'manifest-sha256', 'worker-sha256', 'expected-controller-sha256'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--gcloud', type=Path, default=Path('/home/codespace/google-cloud-sdk/bin/gcloud'))
    parser.add_argument('--bootstrap', action='store_true')
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, guest_minutes=30, OSLogin_TTL='70m'), sort_keys=True))
        return 0
    return run_session(args)


if __name__ == '__main__':
    raise SystemExit(main())
