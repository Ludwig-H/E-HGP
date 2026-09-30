"""Tiny R2 output-collision audit. Prints receipt; does not alter sources/builds."""
import hashlib
import json
import pathlib
import subprocess
import time

ROOT = pathlib.Path('/tmp/mhgp10-audit-r2-cli-0930.0qaMilRs')
SOURCE = pathlib.Path('/tmp/mhgp10-r2/entrees_cli/src/morsehgp3D_v10')
BINARY = pathlib.Path('/tmp/mhgp10-r2/entrees_cli/build/mhgp10_cluster')
TARGETS = {
    'binary': BINARY,
    'cluster_source': SOURCE / 'cli/mhgp10_cluster.cpp',
    'output_header': SOURCE / 'src/core/cli_output.hpp',
    'input': ROOT / 'five.u32le',
}

def hashes():
    return {k: hashlib.sha256(p.read_bytes()).hexdigest() for k, p in TARGETS.items()}

before = hashes()
rows = []
for name, collide in [('control3', False), ('collision3', True)]:
    output = ROOT / (name + '.i32le')
    argv = [str(BINARY), str(TARGETS['input']), str(output), '--k=2', '--mcs=2', '--threads=1']
    if collide:
        argv.append('--tree=' + str(output))
    start = time.monotonic()
    result = subprocess.run(argv, capture_output=True, text=True, timeout=5)
    payload = output.read_bytes() if output.exists() else None
    rows.append({'case': name, 'argv': argv, 'returncode': result.returncode,
                 'stdout': result.stdout, 'stderr': result.stderr,
                 'wall_seconds': time.monotonic() - start,
                 'output_bytes': None if payload is None else len(payload),
                 'output_sha256': None if payload is None else hashlib.sha256(payload).hexdigest(),
                 'output_prefix_hex': None if payload is None else payload[:32].hex()})
after = hashes()
print(json.dumps({'scope': 'R2 CLI existing binary; two tiny native calls; no engine build; GCP0',
                  'expected_label_bytes': 20, 'targets': {k: str(p) for k, p in TARGETS.items()},
                  'sha256_before': before, 'sha256_after': after,
                  'hashes_stable': before == after, 'results': rows}, indent=2))
