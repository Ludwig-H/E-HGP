#!/usr/bin/env python3
"""Launch only the frozen six-case experiment, using the committed guards.

No raw VM start/stop. Private keys and raw control logs stay outside v9.
The controller owns targeted closure; this wrapper verifies its result.
"""
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PACKAGE = Path('/workspaces/E-HGP/build/v9-g4-core-snapshot-20260927')
SESSION = Path('/workspaces/E-HGP/build/v9-g4-core-session-r2-20260927')
GCLOUD = Path('/home/codespace/google-cloud-sdk/bin/gcloud')
PIN = '30291a6a99707c0d35ef4d23798af2a335425865e1f9fd1de9f98406adff4def'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    need(sys.argv[1:] == ['--execute'], 'inert: explicit --execute required')
    package = json.loads((PACKAGE / 'PACKAGE.json').read_text())
    need(package['commit'] == 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a', 'source commit')
    need(len(package['cases']) == 6 and package['real_session_allowed'], 'frozen six cases')
    need(package['cases'] == json.loads((HERE / 'plan.json').read_text())['cases'], 'plan identity')
    controller = ROOT / 'gcp-migration/tower_session_v9.py'
    worker = ROOT / 'gcp-migration/tower_worker_v9.py'
    need(sha(controller) == PIN, 'controller pin')
    need(sha(worker) == package['worker_sha256'], 'worker pin')
    need(sha(PACKAGE / 'snapshot.tar.gz') == package['snapshot_sha256'], 'snapshot pin')
    need(sha(PACKAGE / 'source_manifest.json') == package['manifest_sha256'], 'manifest pin')
    SESSION.mkdir(mode=0o700, exist_ok=False)
    key = SESSION / 'id_ed25519'
    subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'mhgp9-core-20260927',
                    '-f', str(key)], check=True, capture_output=True)
    key.chmod(0o600)
    need(key.stat().st_mode & 0o777 == 0o600, 'private key mode')
    argv = [sys.executable, '-B', str(controller), '--snapshot', str(PACKAGE / 'snapshot.tar.gz'),
            '--manifest', str(PACKAGE / 'source_manifest.json'), '--worker', str(worker),
            '--snapshot-sha256', package['snapshot_sha256'],
            '--manifest-sha256', package['manifest_sha256'], '--worker-sha256', package['worker_sha256'],
            '--expected-controller-sha256', PIN, '--session-dir', str(SESSION),
            '--ssh-key', str(key), '--gcloud', str(GCLOUD), '--execute']
    save(SESSION / 'launch.json', dict(argv=argv, cwd=str(ROOT), wrapper_sha256=sha(Path(__file__)),
                                     max_wait_before_interrupt_seconds=600))
    with (SESSION / 'controller.stdout').open('wb') as out, (SESSION / 'controller.stderr').open('wb') as err:
        process = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err)
        save(SESSION / 'controller.pid.json', dict(pid=process.pid))
        try:
            code = process.wait(timeout=600)
        except subprocess.TimeoutExpired:
            # Ask the owner to collect and close; never kill its finally block.
            process.send_signal(signal.SIGINT)
            code = process.wait()
    host = SESSION / 'tower_v9_host'
    if not (host / 'receipt.json').is_file():
        need(not host.exists(), 'controller failed after creating state; inspect lifecycle before retry')
        print(json.dumps(dict(status='refused_before_host_state', controller_exit=code,
                              GCP_used=False)))
        return code or 1
    receipt = json.loads((host / 'receipt.json').read_text())
    target = receipt['target']
    argv = [str(GCLOUD), 'compute', 'instances', 'describe', target['instance'],
            '--project=' + target['project'], '--zone=' + target['zone'], '--format=json']
    with (SESSION / 'after_stop.json').open('wb') as out, (SESSION / 'after_stop.stderr').open('wb') as err:
        check = subprocess.run(argv, stdout=out, stderr=err, timeout=60, check=False)
    need(check.returncode == 0, 'post-session state unreadable: inspect exact target immediately')
    after = json.loads((SESSION / 'after_stop.json').read_text())
    need(receipt.get('targeted_shutdown_certified') is True and after['status'] == 'TERMINATED' and
         after['lastStartTimestamp'] == receipt['generation'], 'targeted generation closure not certified')
    print(json.dumps(dict(status=receipt['status'], controller_exit=code,
                          generation=receipt['generation'], targeted_shutdown_certified=True)))
    return code


if __name__ == '__main__':
    raise SystemExit(main())
