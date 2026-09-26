#!/usr/bin/env python3
"""One-shot guarded retrieval of a closed v30 session. Inert unless --execute.

Never launches the worker, a build, or a benchmark. Original evidence is
read-only. A new generation gets its own ten-minute guest shutdown and
targeted stop; the recovered worker is judged against its ORIGINAL guard.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shlex
import signal
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import tower_session_v9 as session

HOST = Path('/workspaces/E-HGP/build/v9-q3-payload-session-20260926-BatCh2/tower_v9_host')
DEST = Path('/workspaces/E-HGP/build/v9-q3-payload-recovery-r2-f9f273bb0')
KEY = Path('/workspaces/E-HGP/build/v9-q3-payload-recovery-key-f9f273bb0/session_key')
GCLOUD = Path('/home/codespace/google-cloud-sdk/bin/gcloud')
ORIGINAL = '2026-09-26T13:34:29.327-07:00'
REMOTE = '/tmp/ehgp-tower-v9-8023dfb7ea5a8ef9.Ok4TF329uj'
need, sha, save, fields, epoch = session.need, session.sha, session.save, session.fields, session.epoch


def recovery_deadline(mark, schedule, generation, now):
    session.target(mark)
    need(mark.get('schema') == 'e-hgp.guard-mark.v1' and
         mark.get('mark') == 'double_guard_verified' and mark.get('generation') == generation and
         mark.get('guest_shutdown_minutes') == '10' and mark.get('max_run_seconds') == '3600',
         'recovery guard identity/durations')
    need(epoch(generation) <= epoch(mark['date_utc']) <= now + 5, 'guard chronology')
    need(schedule.get('MODE') == 'poweroff' and
         re.fullmatch('[0-9]{1,18}', schedule.get('USEC', '')), 'guest shutdown')
    deadline = int(schedule['USEC']) // 1000000
    need(now + 300 < deadline <= min(epoch(generation) + 3600 - 300,
                                    epoch(mark['date_utc']) + 720, now + 720), 'recovery window')
    return deadline


def run():
    original_bytes = (HOST / 'receipt.json').read_bytes()
    original = json.loads(original_bytes)
    need(original.get('status') == 'capture_failed' and original.get('worker_exit_code') == 0 and
         original.get('targeted_shutdown_certified') is True and original.get('generation') == ORIGINAL and
         original.get('remote_directory') == REMOTE, 'exact closed original session required')
    need(not DEST.exists() and not DEST.is_symlink(), 'fresh private recovery directory required')
    need(KEY.is_file() and not KEY.is_symlink() and KEY.stat().st_mode & 0o077 == 0, 'private SSH key mode')
    for name, pin in session.GUARDS.items():
        need(sha(session.HERE / name) == pin, 'pinned guard ' + name)
    manifest = json.loads((HOST / 'source_manifest.json').read_text())
    need(sha(HOST / 'source_manifest.json') == original['manifest_sha256'] and
         sha(HOST / 'snapshot.tar.gz') == original['snapshot_sha256'], 'original input pins')
    cases, provenance = session.validate_snapshot(HOST / 'snapshot.tar.gz', manifest)
    session.validate_protocol_runtime(manifest)
    need(provenance == original['provenance'], 'original provenance')
    DEST.mkdir(mode=0o700)
    DEST.chmod(0o700)
    marks = DEST / 'guardmarks'
    marks.mkdir(mode=0o700)
    env = dict(os.environ, GCP_PROJECT_ID=session.TARGET['project'], GCP_ZONE=session.TARGET['zone'],
               GCP_INSTANCE_NAME=session.TARGET['instance'], GCP_SSH_KEY_FILE=str(KEY),
               PATH=str(GCLOUD.parent) + os.pathsep + os.environ.get('PATH', ''))
    commands = session.Commands(DEST, env)
    state = dict(schema='mhgp9_capture_recovery_v1', status='failed', target=session.TARGET,
                 original_generation=ORIGINAL, original_host_receipt_sha256=sha(HOST / 'receipt.json'),
                 benchmark_executed=False, targeted_shutdown_certified=False,
                 original_worker_receipt_hash_pinned_before_stop=False)
    generation = common = deadline = monotonic_deadline = None
    previous = {}

    def records():
        handoff = json.loads((DEST / 'handoff.json').read_text()) if (DEST / 'handoff.json').exists() else None
        lifecycle = fields((DEST / 'lifecycle.txt').read_text()) if (DEST / 'lifecycle.txt').exists() else None
        return session.generation_from_records(handoff, lifecycle)

    def describe(name, status, expected):
        rc, raw, _ = commands.run(name, [GCLOUD, 'compute', 'instances', 'describe', session.TARGET['instance'],
            '--project=' + session.TARGET['project'], '--zone=' + session.TARGET['zone'], '--format=json'], timeout=60)
        need(rc == 0, 'target unreadable: ' + name)
        value = json.loads(raw)
        session.validate_target(value, status, expected)
        return value

    def ssh(name, script, timeout=60):
        return commands.run(name, [GCLOUD, 'compute', 'ssh', session.TARGET['instance'], *common,
            '--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15',
            '--command=' + script], timeout=timeout)

    def remaining(cap):
        need(deadline is not None, 'no recovery deadline')
        value = min(deadline - 180 - time.time(), monotonic_deadline - 180 - time.monotonic(), cap)
        need(value > 0, 'closing reserve reached')
        return value

    def interrupted(signum, _frame):
        raise InterruptedError('recovery signal ' + str(signum))

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous[sig] = signal.signal(sig, interrupted)
        before = describe('original_after_stop', 'TERMINATED', ORIGINAL)
        save(DEST / 'original_after_stop.json', before)
        state.update(original_after_stop_sha256=sha(DEST / 'original_after_stop.json'),
                     original_after_stop_observed_at=datetime.datetime.now(datetime.timezone.utc).isoformat())
        rc, _, _ = commands.run('oslogin_add', [GCLOUD, 'compute', 'os-login', 'ssh-keys', 'add',
            '--project=' + session.TARGET['project'], '--key-file=' + str(KEY) + '.pub',
            '--ttl=70m', '--format=json', '--quiet'])
        need(rc == 0, 'OS Login registration')
        rc, raw, _ = commands.run('guarded_start', [session.HERE / 'start_and_verify.sh', '--yes',
            '--guest-shutdown-minutes', '10', '--handoff-file', DEST / 'handoff.json',
            '--lifecycle-state-file', DEST / 'lifecycle.txt', '--guard-mark-dir', marks], timeout=None, guard=True)
        generation = records()
        need(rc == 0 and generation is not None, 'guarded recovery start')
        expiration = re.findall(r'expiration fixe=(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{6}Z)', raw)
        need(len(expiration) == 1 and epoch(expiration[0]) > time.time(), 'SSH expiration')
        common = ['--project=' + session.TARGET['project'], '--zone=' + session.TARGET['zone'], '--quiet',
                  '--ssh-key-file=' + str(KEY), '--ssh-key-expiration=' + expiration[0]]
        mark = fields((marks / 'double_guard_verified').read_text())
        describe('before_recovery', 'RUNNING', generation)
        rc, raw, _ = ssh('guest_schedule', 'sudo -n cat /run/systemd/shutdown/scheduled')
        need(rc == 0, 'guest schedule unreadable')
        schedule = fields(raw)
        deadline = recovery_deadline(mark, schedule, generation, time.time())
        monotonic_deadline = time.monotonic() + deadline - time.time()
        state.update(generation=generation, verified_guard=dict(mark=mark, schedule=schedule),
                     session_deadline_epoch=deadline)
        archive = REMOTE + '/capture_recovered_f9f273bb0.tar.gz'
        script = ('set -eu; umask 077; test -d ' + shlex.quote(REMOTE) + '; test ! -L ' + shlex.quote(REMOTE) +
                  '; test -O ' + shlex.quote(REMOTE) +
                  '; test "$(stat -c %a ' + shlex.quote(REMOTE) + ')" = 700; test -d ' + shlex.quote(REMOTE + '/output') +
                  '; test ! -L ' + shlex.quote(REMOTE + '/output') + '; test ! -e ' + shlex.quote(archive) +
                  '; test ! -L ' + shlex.quote(archive) +
                  '; tar -czf ' + shlex.quote(archive) + ' --exclude=output/build -C ' + shlex.quote(REMOTE) +
                  ' output; sha256sum ' + shlex.quote(archive))
        rc, raw, _ = ssh('pack_capture', script, remaining(90))
        matches = re.findall(r'^([0-9a-f]{64})  ' + re.escape(archive) + r'$', raw, re.M)
        need(rc == 0 and len(matches) == 1, 'recovery pack/hash')
        remaining(60)
        describe('before_download', 'RUNNING', generation)
        rc, _, _ = commands.run('download', [GCLOUD, 'compute', 'scp', *common,
            session.TARGET['instance'] + ':' + archive, DEST / 'capture.tar.gz'], timeout=remaining(120))
        need(rc == 0 and sha(DEST / 'capture.tar.gz') == matches[0], 'recovery download/hash')
        remaining(60)
        describe('after_download', 'RUNNING', generation)
        state['capture_sha256'] = matches[0]
        state['status'] = 'capture_downloaded'
    except BaseException as error:
        state.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        try:
            try:
                closing = session.closure_generation(generation, records())
            except (OSError, UnicodeError, json.JSONDecodeError):
                closing = session.closure_generation(generation, None, unreadable=True)
            if closing is not None:
                rc, _, _ = commands.run('guarded_stop', [session.HERE / 'stop_and_verify.sh', '--yes',
                    '--expected-last-start-timestamp', closing], timeout=None, critical=True, guard=True)
                need(rc == 0, 'targeted stop uncertified')
                after = describe('after_stop', 'TERMINATED', closing)
                save(DEST / 'after_stop.json', after)
                state.update(generation=closing, targeted_shutdown_certified=True,
                             after_stop_sha256=sha(DEST / 'after_stop.json'))
            else:
                state['no_start_lifecycle_created'] = True
        except BaseException as error:
            state.update(status='shutdown_uncertified', shutdown_error=type(error).__name__ + ': ' + str(error))
        state['commands'] = commands.rows
        save(DEST / 'receipt.json', state)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    # All costly local validation AFTER the VM is certified stopped.
    if state['status'] == 'capture_downloaded' and state['targeted_shutdown_certified']:
        try:
            session.extract_capture(DEST / 'capture.tar.gz', DEST / 'received')
            vm = DEST / 'received/output'
            verdict = session.validate_received(vm, manifest, original['worker_sha256'], cases,
                                                 ORIGINAL, provenance, original['verified_guard'])
            need((HOST / 'receipt.json').read_bytes() == original_bytes, 'original host evidence changed')
            state.update(status='recovered_capture', replayed_original_status=verdict,
                         original_worker_receipt_sha256=sha(vm / 'receipt.json'))
        except BaseException as error:
            state.update(status='failed', replay_error=type(error).__name__ + ': ' + str(error))
        save(DEST / 'receipt.json', state)
    print(json.dumps({key: state.get(key) for key in ('status', 'generation', 'targeted_shutdown_certified',
                     'replayed_original_status', 'error', 'shutdown_error', 'replay_error')}, sort_keys=True))
    return 0 if state['status'] == 'recovered_capture' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    if args.execute:
        raise SystemExit(run())
    print(json.dumps(dict(status='inert', benchmark_executed=False, GCP_used=False,
                         original_generation=ORIGINAL, guest_minutes=10, gce_seconds=3600)))
