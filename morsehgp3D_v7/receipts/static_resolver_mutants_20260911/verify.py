#!/usr/bin/env python3
"""Verify portable deduplicated captures, then run only the pinned reader."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parent
PRIVATE_SHA = '47bad2042100d4545faa1f5bcd57bf77482cc126266d2ef6b612a69a9f16c15e'
HEADER_SHA = '33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209'

def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def relative(name: str) -> Path:
    path = Path(name)
    need(not path.is_absolute() and '..' not in path.parts and name == path.as_posix(), 'relative_path')
    need((BASE / path).resolve().is_relative_to(BASE), 'packet_path_escape')
    return path

def main() -> None:
    need(len(sys.argv) == 1, 'no_arguments')
    public = json.loads((BASE / 'MANIFEST.json').read_text())
    found = {p.relative_to(BASE).as_posix() for p in BASE.rglob('*')
             if p.is_file() and p != BASE / 'MANIFEST.json'}
    need(set(public['files']) == found, 'publication_file_set')
    for name, digest in public['files'].items():
        data = (BASE / relative(name)).read_bytes()
        need(sha(data) == digest, 'publication_hash:' + name)
        need(not data.startswith(b'\x7fELF'), 'no_ELF:' + name)
    original_bytes = (BASE / 'capture/MANIFEST.json').read_bytes()
    need(sha(original_bytes) == PRIVATE_SHA, 'original_manifest_unchanged')
    original = json.loads(original_bytes)
    need(len(original['files']) == 544, 'original_file_count')
    mapping = json.loads((BASE / 'storage_map.json').read_text())
    need(set(mapping) == set(original['files']), 'logical_storage_mapping_domain')
    for logical, physical in mapping.items():
        relative(logical)
        need(sha((BASE / relative(physical)).read_bytes()) == original['files'][logical],
             'logical_capture_byte_identity:' + logical)
    pins = json.loads((BASE / 'source_pins.json').read_text())
    need(pins['private_manifest_sha256'] == PRIVATE_SHA and pins['prototype_header_sha256'] == HEADER_SHA,
         'provenance_pins')
    need(pins['logical_capture_files'] == 544 and pins['commands'] == 18 and pins['physical_mutants'] == 6,
         'provenance_scope')
    for name, digest in pins['review_files'].items():
        need(sha((BASE / relative(name)).read_bytes()) == digest, 'review_original_bytes')
    need(original['files']['source/morsehgp3D_v7/src/forest/full_ball_tower.hpp'] == HEADER_SHA,
         'private_header_identity')
    # Reconstitute ONLY verified source/receipt bytes in a fresh temporary
    # directory. The invoked script reads manifests; it never runs a compiler,
    # an ELF, an oracle, a benchmark or a network/GCP command.
    with tempfile.TemporaryDirectory(prefix='mhgp7-static-mutants-read-') as temp:
        root = Path(temp)
        for logical, physical in mapping.items():
            target = root / logical
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(BASE / physical, target)
        (root / 'MANIFEST.json').write_bytes(original_bytes)
        argv = [sys.executable, '-B']
        if sys.flags.optimize:
            argv.append('-O')
        argv.append(str(root / 'verify.py'))
        completed = subprocess.run(argv, text=True, capture_output=True, check=False)
        need(completed.returncode == 0, 'private_reader_rejected:' + completed.stderr.strip())
        need(not completed.stderr.strip(), 'private_reader_stderr')
        result = json.loads(completed.stdout)
        need(result['status'] == 'verified_private_static_mutants' and result['commands'] == 18 and
             result['mutants'] == 6 and result['geometry_Gamma_rows'] == 2524, 'private_reader_verdict')
    print(json.dumps(dict(status='verified_portable_static_resolver_mutants', commands=18, mutants=6,
        unchanged_logical_files=544, physical_files=len(found), independent_Gamma_rows=2524,
        public_status='not_claimed', GCP_used=False)))

if __name__ == '__main__':
    main()
