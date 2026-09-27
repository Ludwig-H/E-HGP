#!/usr/bin/env python3
# Explicit protocol port of b_q34_resident_session_20260927 at 03decc16c.
# The pinned lifecycle helper, guards, fixed target and cooperative join are unchanged.
"""Explicit adapter of the pinned guarded lifecycle; inert without --execute.

Only the snapshot validator is replaced. run_session, its commands, dual
guards, recovery, and generation-bound finally/stop are used unchanged.
The old directory name full_host is transport-only, never a FULL claim.
"""
import argparse
import json
from pathlib import Path
import re
import signal
import subprocess
import sys
import types
import common as c
import package
import worker
import compile_contract as cc


def validate_received(host, manifest, generation):
    """A successful shell command alone is not a successful S2 experiment."""
    directory = host / 'received/output'
    result = c.read(directory / 'receipt.json')
    c.need(result.get('schema') == c.SCHEMA and result.get('status') == 'completed' and
           result.get('scope') == 'S2_only_no_FULL' and result.get('FULL_executed') is False and
           result.get('contract_certified') is False and result.get('public_status') == 'not_claimed' and
           result.get('target') == c.TARGET and result.get('generation') == generation and
           result.get('useful_budget_seconds') == worker.USEFUL_SECONDS and
           result.get('CUDA_installation_attempted') is False, 'typed S2 receipt')
    c.need(result.get('worker_sha256') == manifest[c.PREFIX + '/worker.py'] and
           result.get('source_manifest_sha256') == c.sha(host / 'source_manifest.json') and
           all(result.get(key) is True for key in ('sources_stable', 'compilation_stable')),
           'source/binary closure')
    c.need(c.read(directory / 'sources_before.json') == c.read(directory / 'sources_after.json') == manifest,
           'closed source inventory')
    _, provenance = c.unpack_readonly(host / 'snapshot.tar.gz', manifest)
    c.need(result.get('provenance') == provenance, 'received committed provenance')
    host_receipt = c.read(host / 'receipt.json')
    remote = host_receipt.get('remote_directory', '')
    c.need(type(remote) is str and re.fullmatch('/tmp/ehgp-full-v7-[0-9a-f]{16}\\.[A-Za-z0-9]{10}', remote),
           'host-owned remote directory')
    root, build = Path(remote) / 'source', Path(remote) / 'survivors_build'
    recipes = worker.recipes(root, build, c)
    for field in ('compile_before', 'compile_after'):
        c.need(c.sha(directory / (field+'.json')) == result.get(field+'_sha256'), 'compile record pin')
    compiled_before, compiled_after = c.read(directory/'compile_before.json'), c.read(directory/'compile_after.json')
    cc.validate_closed(compiled_before, compiled_after, root, build, manifest)
    commands = result.get('commands')
    expected_names = list(recipes)
    c.need(type(commands) is list and [row.get('name') for row in commands] == expected_names,
           'exact command order, gate before measure')
    previous_end = None
    for row in commands:
        name = row['name']
        c.need(row == c.read(directory / (name + '.command.json')) and type(row['exit_code']) is int and row['exit_code'] == 0 and
               row['group_closed'] is True and row['ended_epoch'] >= row['started_epoch'] and
               row['argv'] == recipes[name], 'closed command and exact recipe')
        c.need(previous_end is None or previous_end <= row['started_epoch'], 'sequential command chronology')
        previous_end = row['ended_epoch']
        c.need(all(row.get(key) == value for key, value in c.read(directory / (name + '.intent.json')).items()),
               'command intent')
        for stream in ('stdout', 'stderr'):
            c.need(c.sha(directory / (name + '.' + stream)) == row[stream + '_sha256'], 'command stream pin')
    raw_gate = c.read(directory / 'device_gate.stdout')
    c.need(worker.validate_probe(raw_gate, 'gate') == result['gate'] and
           not (directory / 'device_gate.stderr').read_bytes(), 'clean validated device gate')
    c.need(worker.validate_high_gate(c.read(directory/'high_keys_gate.stdout')) == result.get('high_gate') and
           not (directory/'high_keys_gate.stderr').read_bytes(), 'clean high-key device gate')
    prepin = c.read(directory/'prepin.stdout')
    c.need(prepin == dict(schema=cc.SCHEMA, status='pinned_before_build', jobs=len(compiled_before['commands']),
        headers=len(compiled_before['headers']), output_sha256=c.sha(directory/'compile_before.json')), 'bound prepin command output')
    c.need(type(result.get('measures')) is dict and set(result['measures']) == set(worker.FRAME_KEYS), 'four paired frame processes')
    identity = None
    for width in (4, 48):
        for first in ('baseline', 'candidate'):
            key = 'w'+str(width)+'_'+first
            name = 'ng00_'+key
            raw = c.read(directory/(name+'.stdout'))
            c.need(worker.validate_probe(raw, 'frame', width, first) == result['measures'][key] and
                   not (directory/(name+'.stderr')).read_bytes(), 'clean bound paired frame')
            current = {field:raw['runs'][0][field] for field in ('masses','counters','geometry','arena','device')}
            c.need(identity is None or current == identity, 'same exact work across processes and widths')
            identity = current
    legacy = c.load_legacy()
    mark = legacy.fields((host / 'guardmarks/double_guard_verified').read_text())
    schedule = legacy.fields((host / 'guest_schedule.stdout').read_text())
    c.need(c.read(directory / 'guard_evidence.json') == dict(mark=mark, schedule=schedule), 'same certified guards')
    # The worker validated this same deadline before any compiler or GPU call.
    legacy.guard_deadline(mark, schedule, generation, legacy.epoch(mark['date_utc']))
    return result


def wait_owned(process, session_dir):
    """Every post-Popen path joins the controller, leaving its finally intact."""
    previous = {}
    def forward(_signum, _frame):
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous[sig] = signal.signal(sig, forward)
        c.save(session_dir / 'controller.pid.json', dict(pid=process.pid))
        try:
            return process.wait(timeout=600)
        except subprocess.TimeoutExpired:
            forward(None, None)  # never kill the lifecycle's finally
            return process.wait()
    finally:
        # Also covers a failed PID write or handler installation, not just wait.
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
            process.wait()
        for sig, handler in previous.items():
            signal.signal(sig, handler)


def run_owned(args):
    package_dir = args.package.resolve()
    manifest = c.read(package_dir / 'source_manifest.json')
    description = c.read(package_dir / 'PACKAGE.json')
    snapshot = package_dir / 'snapshot.tar.gz'
    provenance = package.verify_committed(snapshot, manifest)
    c.need(description['snapshot_sha256'] == c.sha(snapshot) and
           description['manifest_sha256'] == c.sha(package_dir / 'source_manifest.json') and
           description['provenance'] == provenance and description['worker_sha256'] == c.sha(c.HERE / 'worker.py'),
           'private package pins')
    session_dir = args.session_dir.absolute()
    session_dir.mkdir(mode=0o700, parents=False, exist_ok=False)
    key = session_dir / 'id_ed25519'
    subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'mhgp9-cuda-waves', '-f', str(key)],
                   check=True, capture_output=True)
    key.chmod(0o600)
    argv = [sys.executable, '-B', str(Path(__file__).resolve()), '--owned-controller',
            '--package', str(package_dir), '--session-dir', str(session_dir), '--gcloud', str(args.gcloud)]
    c.save(session_dir / 'launch.json', dict(schema=c.SCHEMA, argv=argv, wrapper_sha256=c.sha(__file__),
                                            provenance=provenance, max_wait_before_interrupt_seconds=600))
    with (session_dir / 'controller.stdout').open('xb') as out, (session_dir / 'controller.stderr').open('xb') as err:
        process = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=out, stderr=err)
        code = wait_owned(process, session_dir)
    host = session_dir / 'full_host'
    if not host.exists():
        c.save(session_dir / 'verdict.json', dict(schema=c.SCHEMA, status='refused_before_host_state',
                                                controller_exit=code, FULL_executed=False))
        return code or 1
    receipt = c.read(host / 'receipt.json')
    if receipt.get('no_start_lifecycle_created'):
        c.need(not receipt.get('generation'), 'no generation for no-start')
        c.save(session_dir / 'verdict.json', dict(schema=c.SCHEMA, status='no_start', controller_exit=code))
        return code or 1
    with (session_dir / 'after_stop.json').open('xb') as out, (session_dir / 'after_stop.stderr').open('xb') as err:
        closed = subprocess.run([str(args.gcloud), 'compute', 'instances', 'describe', c.TARGET['instance'],
            '--project=' + c.TARGET['project'], '--zone=' + c.TARGET['zone'], '--format=json'],
            stdout=out, stderr=err, timeout=60, check=False)
    c.need(closed.returncode == 0, 'target state unreadable after session; inspect exact target')
    after = c.read(session_dir / 'after_stop.json')
    c.need(receipt.get('targeted_shutdown_certified') is True and after.get('status') == 'TERMINATED' and
           after.get('lastStartTimestamp') == receipt.get('generation'), 'exact generation shutdown uncertified')
    verdict = dict(schema=c.SCHEMA, status='failed', controller_exit=code, scope='S2_only_no_FULL',
                   FULL_executed=False, generation=receipt['generation'], targeted_shutdown_certified=True)
    try:
        c.need(code == 0 and receipt.get('status') == 'completed', 'controller did not complete')
        result = validate_received(host, manifest, receipt['generation'])
        verdict.update(status='completed', gate=result['gate'], high_gate=result['high_gate'], measures=result['measures'])
    except (ValueError, KeyError, OSError) as error:
        verdict['error'] = type(error).__name__ + ': ' + str(error)
    c.save(session_dir / 'verdict.json', verdict)
    print(json.dumps(verdict, sort_keys=True))
    return 0 if verdict['status'] == 'completed' else 1


def owned_controller(args):
    directory = args.package.resolve()
    manifest = c.read(directory / 'source_manifest.json')
    package.verify_committed(directory / 'snapshot.tar.gz', manifest)
    legacy = c.load_legacy()
    # Explicit adapter: semantic snapshot contract changes, lifecycle does not.
    legacy.validate_snapshot = package.verify_committed
    values = types.SimpleNamespace(session_dir=args.session_dir, ssh_key=args.session_dir / 'id_ed25519',
        snapshot=directory / 'snapshot.tar.gz', manifest=directory / 'source_manifest.json',
        worker=c.HERE / 'worker.py', snapshot_sha256=c.sha(directory / 'snapshot.tar.gz'),
        manifest_sha256=c.sha(directory / 'source_manifest.json'), worker_sha256=c.sha(c.HERE / 'worker.py'),
        expected_controller_sha256=c.HELPER_PIN, gcloud=args.gcloud, bootstrap=False)
    return legacy.run_session(values)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--session-dir', type=Path)
    parser.add_argument('--gcloud', type=Path, default=Path('/home/codespace/google-cloud-sdk/bin/gcloud'))
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--execute', action='store_true')
    modes.add_argument('--owned-controller', action='store_true')
    args = parser.parse_args()
    if not args.execute and not args.owned_controller:
        print(json.dumps(dict(schema=c.SCHEMA, status='inert', GCP_used=False, FULL_executed=False,
                              target=c.TARGET, useful_seconds=worker.USEFUL_SECONDS, guest_minutes=30)))
        raise SystemExit(0)
    c.need(args.package is not None and args.session_dir is not None, 'package/session required')
    raise SystemExit(owned_controller(args) if args.owned_controller else run_owned(args))
