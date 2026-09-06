#!/usr/bin/env python3
"""Publish immutable captures with common sources referenced, never ELF files."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
RECEIPTS = ROOT / 'morsehgp3D_v7/receipts'
DEST = RECEIPTS / 'full_coverage_20260906'
LOCAL = RECEIPTS / 'local_plateau_diameter_20260906'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        raise RuntimeError('refuse overwrite: ' + str(target))
    shutil.copyfile(source, target)


shutil.copytree(ROOT / 'build/v7_local_plateau_diameter_20260906/packet', LOCAL)
DEST.mkdir(exist_ok=False)
for name in ['record.py', 'publish.py', 'verify.py', 'README.md']:
    copy(BASE / name, DEST / name)
for source in (BASE / 'r1').rglob('*'):
    if not source.is_file() or 'sources' in source.relative_to(BASE / 'r1').parts:
        continue
    if source.read_bytes()[:4] == b'\x7fELF':
        continue
    copy(source, DEST / 'direct' / source.relative_to(BASE / 'r1'))
cmake = ROOT / 'build/v7_coverage_cmake_20260906'
copy(cmake / 'capture.py', DEST / 'cmake/capture.py')
copy(cmake / 'README.md', DEST / 'cmake/README.md')
for source in (cmake / 'run_r1').rglob('*'):
    if source.is_file():
        copy(source, DEST / 'cmake/run_r1' / source.relative_to(cmake / 'run_r1'))
sources = json.loads((cmake / 'run_r1/project_sources.json').read_text())
refs = {}
for name, digest in sources.items():
    found = None
    if name in ['morsehgp3D_v7/src/forest/full_coverage_certificate.hpp',
                'morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp', 'morsehgp3D_v7/CMakeLists.txt']:
        found = DEST / 'sources' / name
        copy(BASE / 'r1/sources' / name, found)
    else:
        for candidate in RECEIPTS.rglob(Path(name).name):
            if candidate.is_file() and sha(candidate) == digest:
                found = candidate
                break
    if found is None or sha(found) != digest:
        raise RuntimeError('missing sealed project source: ' + name)
    refs[name] = dict(repo_path=str(found.relative_to(ROOT)), sha256=digest)
(DEST / 'source_refs.json').write_text(json.dumps(refs, indent=2, sort_keys=True) + '\n')
manifest = []
for source in sorted(DEST.rglob('*')):
    if source.is_file():
        if source.read_bytes()[:4] == b'\x7fELF':
            raise RuntimeError('unexpected ELF')
        manifest.append(sha(source) + '  ' + str(source.relative_to(DEST)))
(DEST / 'SHA256SUMS').write_text('\n'.join(manifest) + '\n')
print(json.dumps(dict(files=len(manifest), manifest_sha256=sha(DEST / 'SHA256SUMS'),
                     source_references=len(refs), local_manifest_sha256=sha(LOCAL / 'SHA256SUMS')), sort_keys=True))
