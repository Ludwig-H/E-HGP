#!/usr/bin/env python3
"""Portable read-only proof reader; no compilation, vendor files, CUDA or GCP."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text())


def main():
    manifest = read(ROOT/'MANIFEST.json')
    need(manifest['schema'] == 'portable_static_batch_seam_v1' and manifest['public_status'] == 'not_claimed' and
         manifest['GCP_used'] is False and manifest['device_executed'] is False, 'bounded_scope')
    for name, expected in manifest['files'].items():
        relative = Path(name)
        need(not relative.is_absolute() and '..' not in relative.parts, 'safe_packet_path')
        need(sha(ROOT/relative) == expected, 'packet_pin:'+name)
    mapping = read(ROOT/'storage_map.json')
    need(len(mapping) == manifest['logical_files'] and len(set(mapping.values())) == manifest['objects'],
         'deduplication_counts')
    source_manifest = 'closed_o2_san_r1/MANIFEST.json'
    need(mapping[source_manifest] == manifest['original_manifest_sha256'], 'original_closure_identity')
    with tempfile.TemporaryDirectory(prefix='mhgp7-batch-proof-') as directory:
        restored = Path(directory)
        for name, pin in mapping.items():
            relative = Path(name)
            need(not relative.is_absolute() and '..' not in relative.parts, 'safe_logical_path')
            payload = ROOT/'objects'/pin
            need(sha(payload) == pin, 'content_pin')
            data = payload.read_bytes()
            need(data[:4] != b'\x7fELF', 'no_ELF_distributed')
            target = restored/relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        original = read(restored/source_manifest)
        need(original['source_baseline_sha256'] == manifest['baseline_header_sha256'] and
             original['source_candidate_sha256'] == manifest['candidate_header_sha256'], 'source_versions')
        need(original['omitted_ELF'] == manifest['omitted_ELF'], 'omitted_binary_provenance')
        for name, row in original['files'].items():
            need(mapping[name] == row['sha256'] and (restored/name).stat().st_size == row['bytes'],
                 'reversible_original_file:'+name)
        paired = read(restored/'closed_o2_san_r1/O2_SAN_comparison.json')
        need(paired['status'] == 'passed' and paired['same_sources'] is True and len(paired['physical']) == 7,
             'paired_scope')
        for row in paired['physical']:
            need(sha(restored/'o2_r2'/row['name']) == row['O2_sha256'] == row['SAN_sha256'] ==
                 sha(restored/'san_root_r1'/row['name']), 'paired_byte_identity')
        for capture in ('o2_r1', 'o2_r2', 'san_root_r1'):
            command = ['python3', '-B', *(['-O'] if sys.flags.optimize else []),
                       str(restored/'closed_o2_san_r1/verify.py'), str(restored/capture)]
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
            need(result.returncode == 0 and result.stderr == b'', 'captured_reader:'+capture)
            print(result.stdout.decode().strip())
    print(json.dumps(dict(status='passed_portable', captures=3, engine_commands=33,
        logical_files=manifest['logical_files'], objects=manifest['objects'],
        no_ELF=True, no_vendor=True, device_executed=False, GCP_used=False,
        private_prototype=True, public_status='not_claimed')))


if __name__ == '__main__':
    main()
