#!/usr/bin/env python3
"""One bounded FULL profiling diagnostic; no engine build or raw VM start.

Reuse the original FULL package and pinned lifecycle unchanged. The separate
diagnostic worker is committed and hashed, not confused with that package's
historical worker. No cloud operation without --execute.
"""
import argparse
from datetime import datetime
import hashlib
import importlib
import json
from pathlib import Path
import re
import subprocess
import sys
import types

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PACKAGE = Path('/workspaces/E-HGP/build/v9-g4-core-snapshot-20260927')
NATIVE_COMMIT = 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a'
SNAPSHOT_SHA = '2ccddb90e0e88bb072f7eef4f4b84ae25f3a148850d57bb03914f2c619221e8c'
MANIFEST_SHA = '71463c8b200d484437e758749899f6f5a2f5f1f560e8a5d48c63a01f4892c03d'
HELPER_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
OLD = HERE.parent / 'b_q34_survivors_session_20260927'
PINS = {
    'gcp-migration/full_probe_session_v7.py': HELPER_SHA,
    'gcp-migration/tower_session_v9.py': '30291a6a99707c0d35ef4d23798af2a335425865e1f9fd1de9f98406adff4def',
    'gcp-migration/tower_worker_v9.py': '49b096b3a7ef630f122b3577dda1878e69c3e9d2378a6a219c979a906cfb0f2b',
}
WAIT_PINS = dict(
    session='55edc13c88fbe484da269f9308413c9b52ae329cb23aebf6cb40c5c29cc6e863',
    common='3880781ea4c4ace07806cf00cc7d83c0f500e226a2b20a3a0c9c8d6e5b7fbf55',
    package='1f6d0522d0ead6d79d1140a62b87592384b8a80e3d6539f4164cf939367fac9e',
    worker='65923c1f3b72e1b986dec82394b7ff108574703de337b9ad931e825c13c37e4a',
    compile_contract='68e334b2abcbd1e63284d8f9ed0681d36ec31fa50472e4c80d1168424b9d0f11')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')


def runtime():
    for name, pin in PINS.items():
        need(sha(ROOT/name) == pin, 'changed native helper: '+name)
    for name, pin in WAIT_PINS.items():
        need(sha(OLD/(name+'.py')) == pin, 'changed wait helper: '+name)
    sys.path.insert(0, str(ROOT/'gcp-migration'))
    native = importlib.import_module('tower_session_v9')
    sys.path.insert(0, str(OLD))
    wait = importlib.import_module('session').wait_owned
    # No lifecycle primitive, stop, command or recovery method is replaced.
    native.legacy.validate_snapshot = native.validate_snapshot
    return native, wait


def package_check(native):
    need(sha(PACKAGE/'snapshot.tar.gz') == SNAPSHOT_SHA and
         sha(PACKAGE/'source_manifest.json') == MANIFEST_SHA, 'original FULL package')
    description = read(PACKAGE/'PACKAGE.json')
    need(description['commit'] == NATIVE_COMMIT and
         description['snapshot_sha256'] == SNAPSHOT_SHA and
         description['manifest_sha256'] == MANIFEST_SHA, 'FULL package provenance')
    cases, provenance = native.validate_snapshot(PACKAGE/'snapshot.tar.gz', read(PACKAGE/'source_manifest.json'))
    need(len(cases) == 6 and cases[0]['k'] == 5 and cases[0]['workers'] == 48 and
         cases[0]['frames'] == 4, 'original FULL case')
    return provenance


def committed(commit):
    need(type(commit) is str and re.fullmatch('[0-9a-f]{40}', commit), 'full diagnostic commit')
    for name in ('capture.py', 'worker.py'):
        path = HERE/name
        result = subprocess.run(['git', '-C', str(ROOT), '--no-replace-objects', 'show',
                                 commit+':'+str(path.relative_to(ROOT))], capture_output=True, check=True)
        need(result.stdout == path.read_bytes(), 'executing diagnostic differs from commit: '+name)


def controller(args):
    native, _ = runtime()
    committed(args.commit)
    package_check(native)
    values = types.SimpleNamespace(session_dir=args.session_dir, ssh_key=args.session_dir/'id_ed25519',
        snapshot=PACKAGE/'snapshot.tar.gz', manifest=PACKAGE/'source_manifest.json', worker=HERE/'worker.py',
        snapshot_sha256=SNAPSHOT_SHA, manifest_sha256=MANIFEST_SHA, worker_sha256=sha(HERE/'worker.py'),
        expected_controller_sha256=HELPER_SHA, gcloud=args.gcloud, bootstrap=False)
    return native.legacy.run_session(values)


def execute(args):
    native, wait = runtime()
    committed(args.commit)
    provenance = package_check(native)
    directory = args.session_dir.absolute()
    directory.mkdir(mode=0o700, parents=False, exist_ok=False)
    key = directory/'id_ed25519'
    subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'mhgp9-nsys-diagnostic',
                    '-f', str(key)], check=True, capture_output=True)
    key.chmod(0o600)
    argv = [sys.executable, '-B', str(HERE/'capture.py'), '--owned-controller', '--commit', args.commit,
            '--session-dir', str(directory), '--gcloud', str(args.gcloud)]
    save(directory/'launch.json', dict(argv=argv, diagnostic_commit=args.commit,
        worker_sha256=sha(HERE/'worker.py'), wrapper_sha256=sha(__file__),
        native_package_provenance=provenance, helper_sha256=HELPER_SHA,
        scope='FULL_profile_diagnostic_not_contract', max_wait_before_interrupt_seconds=600))
    with (directory/'controller.stdout').open('xb') as out, (directory/'controller.stderr').open('xb') as err:
        process = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=out, stderr=err)
        code = wait(process, directory)
    host = directory/'full_host'
    if not host.exists():
        save(directory/'verdict.json', dict(status='refused_before_host_state', controller_exit=code))
        return code or 1
    receipt = read(host/'receipt.json')
    if receipt.get('no_start_lifecycle_created'):
        need(not receipt.get('generation'), 'no generation in no-start')
        save(directory/'verdict.json', dict(status='no_start', controller_exit=code))
        return code or 1
    target = native.TARGET
    with (directory/'after_stop.json').open('xb') as out, (directory/'after_stop.stderr').open('xb') as err:
        result = subprocess.run([str(args.gcloud), 'compute', 'instances', 'describe', target['instance'],
            '--project='+target['project'], '--zone='+target['zone'], '--format=json'],
            stdout=out, stderr=err, timeout=60, check=False)
    need(result.returncode == 0, 'target state unreadable after stop')
    after = read(directory/'after_stop.json')
    need(receipt.get('targeted_shutdown_certified') is True, 'targeted stop not certified')
    native.validate_target(after, 'TERMINATED', receipt['generation'])
    start = datetime.fromisoformat(receipt['generation'].replace('Z', '+00:00'))
    stop = datetime.fromisoformat(after['lastStopTimestamp'].replace('Z', '+00:00'))
    need(start.tzinfo is not None and stop.tzinfo is not None and stop >= start, 'closure chronology')
    value = dict(status=receipt['status'], controller_exit=code, generation=receipt['generation'],
        targeted_shutdown_certified=True, vm_elapsed_seconds=(stop-start).total_seconds(),
        scope='FULL_profile_diagnostic_not_contract', contract_certified=False,
        receipt_needs_semantic_readback=True)
    save(directory/'verdict.json', value)
    print(json.dumps(value, sort_keys=True))
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--execute', action='store_true')
    modes.add_argument('--owned-controller', action='store_true')
    parser.add_argument('--commit')
    parser.add_argument('--session-dir', type=Path)
    parser.add_argument('--gcloud', type=Path, default=Path('/home/codespace/google-cloud-sdk/bin/gcloud'))
    args = parser.parse_args()
    if not args.execute and not args.owned_controller:
        print(json.dumps(dict(status='inert', GCP_used=False, scope='FULL_profile_diagnostic_not_contract')))
        return 0
    need(args.commit is not None and args.session_dir is not None, 'commit and session required')
    return controller(args) if args.owned_controller else execute(args)


if __name__ == '__main__':
    raise SystemExit(main())
