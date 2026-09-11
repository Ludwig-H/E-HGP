#!/usr/bin/env python3
"""Verify portable scale evidence, invoking only two pinned read-only readers."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
HEADER_SHA = '33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209'
BINARY_SHA = 'f71f31190f5d50ac70f8332f969c6baa50549536bd08836702e6ba01ae7fcdce'
SCALE_READER_SHA = '2bc5596fc1d281e1ed99766ca0d3baaef710df71f1ff3c91794e3cedce55a368'
KEY_MANIFEST_SHA = '9167657c29c3ffb36292f2b830475d537d47072831740e5bb0b7bb937caa2c77'
REFERENCE_MANIFEST_SHA = 'c9913094b6479a61a0395be2dc0f09ef2d25672f170e19d3c6e01c584ba08b2c'
SIZES = (8000, 16000, 32000)


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def relative(name: str) -> Path:
    path = Path(name)
    need(not path.is_absolute() and '..' not in path.parts and name == path.as_posix(), 'relative_path')
    need((BASE / path).resolve().is_relative_to(BASE), 'packet_path_escape')
    return path


def read_json(name: str):
    return json.loads((BASE / relative(name)).read_text())


def reader(name: str, args: list[str]):
    argv = [sys.executable, '-B']
    if sys.flags.optimize:
        argv.append('-O')
    result = subprocess.run(argv + [str(BASE / name)] + args, text=True, capture_output=True, check=False)
    need(result.returncode == 0 and not result.stderr.strip(), 'pinned_reader:' + name + ':' + result.stderr)
    return json.loads(result.stdout)


def main() -> None:
    need(len(sys.argv) == 1, 'no_arguments')
    manifest = read_json('MANIFEST.json')
    found = {p.relative_to(BASE).as_posix() for p in BASE.rglob('*')
             if p.is_file() and p != BASE / 'MANIFEST.json'}
    need(set(manifest['files']) == found, 'publication_file_set')
    for name, digest in manifest['files'].items():
        path = BASE / relative(name)
        need(sha(path) == digest, 'publication_hash:' + name)
        need(not path.read_bytes().startswith(b'\x7fELF'), 'no_ELF:' + name)
    mapping = read_json('source_map.json')
    originals = read_json('capture/original_inventory.json')
    need(set(mapping) == set(originals), 'original_mapping_domain')
    for logical, physical in mapping.items():
        relative(logical)
        need(sha(BASE / relative(physical)) == originals[logical], 'original_bytes:' + logical)
    pins = read_json('source_pins.json')
    need(pins['binary_sha256'] == BINARY_SHA and pins['header_sha256'] == HEADER_SHA and
         pins['reference_manifest_sha256'] == REFERENCE_MANIFEST_SHA and
         pins['key_comparison_manifest_sha256'] == KEY_MANIFEST_SHA and
         pins['runs'] == 3 and pins['ELF_included'] is False and pins['GCP_used'] is False, 'scope_pins')
    need(sha(BASE / 'source/morsehgp3D_v7/src/forest/full_ball_tower.hpp') == HEADER_SHA, 'header_pin')
    need(sha(BASE / 'verify_scale.py') == SCALE_READER_SHA, 'scale_reader_pin')
    need(sha(BASE / 'capture/reference_manifest.json') == REFERENCE_MANIFEST_SHA, 'reference_manifest_pin')
    need(sha(BASE / 'initial_key_comparison/manifest.json') == KEY_MANIFEST_SHA, 'key_manifest_pin')
    key_manifest = read_json('initial_key_comparison/manifest.json')
    key_files = {p.relative_to(BASE / 'initial_key_comparison').as_posix()
                 for p in (BASE / 'initial_key_comparison').rglob('*') if p.is_file()}
    need(key_files == set(key_manifest['files']) | {'manifest.json'}, 'key_original_file_set')
    for name, digest in key_manifest['files'].items():
        need(sha(BASE / 'initial_key_comparison' / relative(name)) == digest, 'key_original_bytes:' + name)
    refs = read_json('capture/reference_manifest.json')
    prior_end = 0
    for n in SIZES:
        prefix = f'n{n}_s8_static4/'
        actual = read_json(prefix + 'stdout')
        need((actual['n'], actual['s'], actual['kmax'], actual['threads'], actual['static_threads'],
              actual['seed'], actual['coord']) == (n, 8, 10, 1, 4, 3, 65536), 'configuration')
        need(actual['private_static_prototype'] is True and actual['backend'] == 'cpu_reference', 'prototype_scope')
        command = read_json(prefix + 'command.json')
        need(command['started_ns'] >= prior_end, 'sequential_scale_runs')
        prior_end = command['ended_ns']
        metadata = read_json(prefix + 'reference.json')
        need(metadata['packet_manifest_sha256'] == REFERENCE_MANIFEST_SHA and
             sha(BASE / prefix / 'reference.stdout') == refs[f'local/n{n}_s8.stdout'], 'reference_bound_to_manifest')
        rows = actual['static_orders']
        need(rows[0] == dict(K=1, requests=0, unique=0, seeded_unique=0), 'nominal_K1')
        need(all(0 < r['seeded_unique'] <= r['unique'] < r['requests'] for r in rows[1:]), 'per_order_nonvacuity')
    need((BASE / 'initial_key_comparison/static.stdout').read_bytes() ==
         (BASE / 'n8000_s8_static4/stdout').read_bytes(), 'key_comparison_bound_to_exact_8k_capture')
    # These scripts are pinned and read local JSON/source bytes only. Neither
    # runs the engine, compiler, oracle, network, cloud or historical recorder.
    scale = reader('verify_scale.py', list(map(str, SIZES)))
    need(scale == read_json('results.json') and scale['status'] == 'verified_closed_local_static_scale', 'derived_scale')
    keys = reader('initial_key_comparison/verify.py', [])
    need(keys['status'] == 'passed' and keys['orders_compared'] == 9 and
         keys['new_engine_execution'] is False, 'derived_key_comparison')
    print(json.dumps(dict(status='verified_portable_static_resolution_scale', runs=3,
        sizes=list(SIZES), fields_compared_each=35, independent_key_orders=9,
        scale=scale, key_comparison=keys, timing_speedup_claim=False,
        public_status='not_claimed', GCP_used=False)))


if __name__ == '__main__':
    main()
