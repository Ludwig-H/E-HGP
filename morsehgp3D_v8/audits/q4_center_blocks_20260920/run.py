#!/usr/bin/env python3
"""Closed captures of the audit-only map; keeps failing child executions."""
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

from fixtures import BASE, ROOT, cases, save_input


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    binary, folder = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
    if folder.exists():
        raise RuntimeError('Refusing to overwrite a capture')
    folder.mkdir(parents=True)
    source_names = ('center_blocks.cpp','fixtures.py','domain_gate.py','oracle_gate.py','run.py','read.py')
    sources = {name:sha(BASE/name) for name in source_names}
    manifest = dict(schema='audit_q4_center_blocks_capture_v1', status='started',
                    scope='independent certificate only; no family sweep, edge generator or output qualification',
                    sources=sources, binary=str(binary), binary_sha256=sha(binary),
                    product_reference='8d0a0f0f', public_status='not_claimed', kmax=10,
                    node_budget=4096, records=[])
    (folder/'MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
    for name, points, provenance, depths in cases():
        input_file, digest = save_input(name, points)
        for depth in depths:
            for domain in (0,1):
                command = [str(binary),str(input_file),'10',str(depth),'4096',str(domain)]
                record = dict(case=name, n=len(points), input_sha256=digest, provenance=provenance,
                              depth=depth, domain=domain, command=command)
                start = time.monotonic()
                try:
                    child = subprocess.run(command, capture_output=True, text=True, timeout=180)
                    record.update(returncode=child.returncode, stdout=child.stdout, stderr=child.stderr)
                except BaseException as error:
                    record.update(returncode=None, error=repr(error),
                                  stdout=str(getattr(error,'stdout','')), stderr=str(getattr(error,'stderr','')))
                record['process_wall_seconds_including_io'] = time.monotonic()-start
                record_name = f'{len(manifest["records"]):03d}.json.gz'
                record_file = folder/record_name
                record_file.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
                manifest['records'].append(dict(file=record_name, sha256=sha(record_file)))
                (folder/'MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
                if record.get('returncode') != 0:
                    raise RuntimeError('Certificate experiment failed; capture retained')
                print(f'{name} depth={depth} domain={domain} completed', flush=True)
    if sources != {name:sha(BASE/name) for name in source_names} or manifest['binary_sha256'] != sha(binary):
        raise RuntimeError('Source or binary changed during measurement')
    manifest['status'] = 'completed'
    (folder/'MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
    (folder/'COMPLETION.json').write_text(json.dumps(dict(status='completed', records=len(manifest['records']),
        manifest_sha256=sha(folder/'MANIFEST.json')), sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
