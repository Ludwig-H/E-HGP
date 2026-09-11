#!/usr/bin/env python3
"""Create-only publication, only after ROOT explicitly confirms targeted closure."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
LOCAL = ROOT / 'build/v7_gpu_execution_20260911'
HOST = Path('/tmp/ehgp-v7-meb-g4-20260911.PxVVJkOLaW/full_host')
DEST = ROOT / 'morsehgp3D_v7/receipts/gpu_primitives_g4_20260911'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def save(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)


def js(raw):
    return (json.dumps(raw, indent=2, sort_keys=True) + '\n').encode()


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root-confirmed-targeted-closure', action='store_true')
parser.add_argument('--preview', help='optional fresh identifier below the private preparation directory')
args = parser.parse_args()
if args.preview is not None:
    need(args.preview.isidentifier(), 'private preview identifier')
    DEST = BASE / args.preview
need(args.root_confirmed_targeted_closure, 'ROOT notification required before collecting logs')
receipt = json.loads((HOST / 'receipt.json').read_text())
need(receipt['targeted_shutdown_certified'] is True and receipt['status'] == 'completed', 'closed successful session required')
need(not DEST.exists(), 'create-only destination')
spec = importlib.util.spec_from_file_location('publication_verifier', BASE / 'verify.py')
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)
selected = {}
excluded = []
for directory in ('input_r1', 'local_o3_r1', 'checks_r1', 'checks_r2', 'checks_r3'):
    for path in sorted((LOCAL / directory).rglob('*')):
        if not path.is_file():
            continue
        need(not path.is_symlink(), 'source symlink')
        name = 'local/' + path.relative_to(LOCAL).as_posix()
        data = path.read_bytes()
        if data.startswith(b'\x7fELF'):
            excluded.append(dict(logical=name, sha256=sha(data), reason='ELF omitted; hash retained'))
        else:
            selected[name] = path
for path in sorted(HOST.rglob('*')):
    if not path.is_file():
        continue
    need(not path.is_symlink(), 'host symlink')
    name = 'host/' + path.relative_to(HOST).as_posix()
    if any(part.startswith('oslogin_') for part in path.relative_to(HOST).parts):
        excluded.append(dict(logical=name, reason='OSLogin material omitted without reading or hashing'))
        continue
    data = path.read_bytes()
    verifier.no_secret_or_elf(name, data)
    if name.endswith('.tar.gz'):
        verifier.archive(data)
    selected[name] = path
for name in ('prepare.py', 'compile_snapshot.py', 'checks.py'):
    selected['scripts/' + name] = LOCAL / name
for name in ('anchor_meb_worker_v7.py', 'selftest_anchor_meb_worker_v7.py', 'full_probe_worker_v7.py',
             'full_probe_session_v7.py', 'selftest_full_probe_session_v7.py', 'selftest_session_v7.py',
             'start_and_verify.sh', 'stop_and_verify.sh'):
    selected['scripts/gcp-migration/' + name] = ROOT / 'gcp-migration' / name
before = {name: sha(path.read_bytes()) for name, path in sorted(selected.items())}
DEST.mkdir()
mapping = {}
for name, path in sorted(selected.items()):
    data = path.read_bytes()
    need(sha(data) == before[name], 'source changed while copying')
    verifier.no_secret_or_elf(name, data)
    physical = 'objects/' + before[name]
    if not (DEST / physical).exists():
        save(DEST / physical, data)
    mapping[name] = dict(physical=physical, sha256=before[name], bytes=len(data))
after = {name: sha(path.read_bytes()) for name, path in sorted(selected.items())}
need(before == after, 'source changed during publication')
save(DEST / 'storage_map.json', js(mapping))
save(DEST / 'capture_stability.json', js(dict(before=before, after=after)))
save(DEST / 'exclusions.json', js(excluded))
for name in ('verify.py', 'publish.py', 'README.md'):
    save(DEST / name, (BASE / name).read_bytes())
files = {p.relative_to(DEST).as_posix(): sha(p.read_bytes()) for p in sorted(DEST.rglob('*')) if p.is_file()}
save(DEST / 'MANIFEST.json', js(dict(schema='mhgp7.finite-gpu-primitives.closed-session.v1', files=files)))
result = verifier.verify(DEST)
print(json.dumps(dict(status='published', path=str(DEST), manifest_sha256=sha((DEST / 'MANIFEST.json').read_bytes()),
                      verification=result), sort_keys=True))
