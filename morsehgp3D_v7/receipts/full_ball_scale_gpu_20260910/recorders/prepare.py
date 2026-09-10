#!/usr/bin/env python3
"""Freeze reviewed sources for the unchanged guarded controller; no cloud write."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path('/workspaces/E-HGP')
OUT = Path(__file__).resolve().parent / 'input_r1'
OUT.mkdir(exist_ok=False)
files = {}
for directory in ('src', 'bench'):
    for path in (ROOT / 'morsehgp3D_v7' / directory).rglob('*'):
        if path.is_file() and path.suffix in ('.hpp', '.h', '.cuh', '.cpp', '.cu'):
            files[path.relative_to(ROOT).as_posix()] = path
for name in ('morsehgp3D_v7/tests/census_route_device_gate.cu', 'morsehgp3D_v7/bench/nvcc_strict_host.py'):
    files[name] = ROOT / name
files['morsehgp3D_v7/bench/session_support/full_probe_worker_v7.py'] = ROOT / 'gcp-migration/full_probe_worker_v7.py'
manifest = {}
with tarfile.open(OUT / 'snapshot.tar.gz', 'x:gz') as archive:
    for name, path in sorted(files.items()):
        raw = path.read_bytes()
        manifest[name] = hashlib.sha256(raw).hexdigest()
        member = tarfile.TarInfo(name)
        member.size = len(raw)
        member.mode = 0o600
        archive.addfile(member, io.BytesIO(raw))
(OUT / 'source_manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
pins = {}
for key, path in {'snapshot': OUT / 'snapshot.tar.gz', 'manifest': OUT / 'source_manifest.json',
                  'worker': ROOT / 'gcp-migration/full_ball_worker_v7.py',
                  'controller': ROOT / 'gcp-migration/full_probe_session_v7.py',
                  'start': ROOT / 'gcp-migration/start_and_verify.sh',
                  'stop': ROOT / 'gcp-migration/stop_and_verify.sh'}.items():
    pins[key] = {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
(OUT / 'pins.json').write_text(json.dumps(pins, indent=2, sort_keys=True) + '\n')
for name, argv in [('head', ['git', 'rev-parse', 'HEAD']), ('status', ['git', 'status', '--porcelain=v1'])]:
    (OUT / (name + '.txt')).write_bytes(subprocess.check_output(argv, cwd=ROOT))
spec = importlib.util.spec_from_file_location('controller', ROOT / 'gcp-migration/full_probe_session_v7.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.validate_snapshot(OUT / 'snapshot.tar.gz', manifest)
print(json.dumps({'sources': len(manifest), 'pins': pins}, indent=2))
