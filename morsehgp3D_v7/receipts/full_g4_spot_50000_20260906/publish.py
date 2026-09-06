#!/usr/bin/env python3
"""Create-only selected publication. No cloud calls, secret/profile/ELF/tar reads."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SESSION = Path('/home/codespace/.local/state/ehgp/full-v7-20260906.B6cePziJ')
HOST = SESSION / 'full_host'
GUEST = HOST / 'received/output'
RECEIPTS = ROOT / 'morsehgp3D_v7/receipts'
DEST = RECEIPTS / 'full_g4_spot_50000_20260906'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n')


def main():
    need(len(sys.argv) == 2 and sys.argv[1] == '--execute', 'inert without --execute')
    need(not DEST.exists(), 'destination already exists')
    need(sha(HOST / 'receipt.json') == '6f02b9ff6e0e99a098a07c5a0555a6bf92c49179b3e877b5bdc7ce9b21aed3bd', 'closed host receipt')
    need(sha(GUEST / 'receipt.json') == '10ab69dd048e257de39a8c6824f2eda87da0d48638b2d3ebf0bd5c5b2b0562bb', 'closed guest receipt')
    selected = {}
    for name in ('compiler_version', 'time_version', 'compile', 'smoke_n8_k10', 'n50000_k10', 'n50000_k5'):
        for suffix in ('intent.json', 'command.json', 'stdout', 'stderr'):
            selected['guest/' + name + '.' + suffix] = GUEST / (name + '.' + suffix)
    for name in ('receipt.json', 'sources_before.json', 'sources_after.json', 'compiled_dependencies.json', 'full_probe.d',
                 'guard_evidence.json', 'meminfo.txt', 'cgroup.txt', 'os-release.txt',
                 'smoke_n8_k10.summary.json', 'n50000_k10.summary.json', 'n50000_k5.summary.json'):
        selected['guest/' + name] = GUEST / name
    for name in ('guarded_start', 'guarded_stop', 'guest_schedule', 'worker', 'pack_capture', 'download'):
        for suffix in ('intent.json', 'command.json', 'stdout', 'stderr'):
            selected['host/' + name + '.' + suffix] = HOST / (name + '.' + suffix)
    for name in ('receipt.json', 'handoff.json', 'lifecycle.txt', 'guardmarks/guest_guard_pending', 'guardmarks/double_guard_verified'):
        selected['host/' + name] = HOST / name
    selected.update({'launch_context.json': SESSION / 'launch_context.json', 'closure_readonly.json': SESSION / 'closure_readonly.json',
                     'source_manifest.json': HOST / 'source_manifest.json', 'README.md': BASE / 'README.md',
                     'verify.py': BASE / 'verify.py', 'publish.py': Path(__file__)})
    manifest = json.loads(selected['source_manifest.json'].read_text())
    old_packet = RECEIPTS / 'full_census_payload_20260906'
    old_refs = json.loads((old_packet / 'publication.json').read_text())['source_references']
    refs = {}
    for name, pin in manifest.items():
        direct = old_packet / 'run_r1/sources' / name
        previous = old_refs.get('run_r1/sources/' + name)
        path = direct if direct.is_file() else (old_packet / previous['relative_path']).resolve()
        need(sha(path) == pin, 'sealed source ' + name)
        refs[name] = dict(relative_path='../' + str(path.relative_to(RECEIPTS)), sha256=pin)
    launch = json.loads((SESSION / 'launch_context.json').read_text())
    host = json.loads((HOST / 'receipt.json').read_text())
    scripts = {}
    for role, filename, pin in (('worker', 'full_probe_worker_v7.py', host['worker_sha256']),
                               ('controller', 'full_probe_session_v7.py', host['controller_sha256']),
                               ('start_guard', 'start_and_verify.sh', '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0'),
                               ('stop_guard', 'stop_and_verify.sh', 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c')):
        need(sha(ROOT / 'gcp-migration' / filename) == pin, 'recorded script pin')
        scripts[role] = dict(repository_path='gcp-migration/' + filename, sha256=pin, commit=launch['git_commit'])
    before = {name: sha(path) for name, path in selected.items()}
    DEST.mkdir()
    for name, path in selected.items():
        destination = DEST / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        need(sha(destination) == before[name], 'copied bytes')
    save(DEST / 'publication.json', dict(schema='mhgp7-full-g4-negative-capture-v1', status='published_closed_capture',
         public_status='not_claimed', global_FULL_successful=False, files=before, source_references=refs, scripts=scripts,
         exclusions=['private/public keys', 'oslogin_add.stdout', 'full GCE descriptions', 'ELF', 'tar archives', 'duplicate snapshots']))
    results = []
    for mode, options in (('normal', []), ('optimized', ['-O'])):
        argv = [sys.executable, '-B', *options, str(DEST / 'verify.py'), str(DEST)]
        result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        (DEST / ('verify_' + mode + '.stdout')).write_bytes(result.stdout)
        (DEST / ('verify_' + mode + '.stderr')).write_bytes(result.stderr)
        save(DEST / ('verify_' + mode + '.command.json'), dict(argv=argv, exit_code=result.returncode,
             scope='offline captured evidence only; no engine/GCP', stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
             stderr_sha256=hashlib.sha256(result.stderr).hexdigest()))
        need(result.returncode == 0, mode + ': ' + result.stderr.decode())
        results.append(result.stdout)
    need(results[0] == results[1], 'normal/optimized parity')
    need({name: sha(path) for name, path in selected.items()} == before, 'input bytes changed during publication')
    files = sorted(path for path in DEST.rglob('*') if path.is_file())
    (DEST / 'SHA256SUMS').write_text(''.join(f'{sha(path)}  {path.relative_to(DEST)}\n' for path in files))
    print(json.dumps(dict(status='published', files=len(files)+1, source_references=len(refs),
                         SHA256SUMS=sha(DEST / 'SHA256SUMS')), sort_keys=True))


if __name__ == '__main__':
    main()
