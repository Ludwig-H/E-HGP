#!/usr/bin/env python3
"""Verify frozen S9 WIP captures and replay bounded Python counterchecks."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    repository = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path('/workspaces/E-HGP')
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        expected, name = line.split('  ', 1)
        require(sha((HERE / name).read_bytes()) == expected, 'capsule SHA: ' + name)
    for name, expected in json.loads((HERE / 'math/SHA256.json').read_text()).items():
        require(sha((HERE / 'math' / name).read_bytes()) == expected, 'math SHA: ' + name)
    api = json.loads((HERE / 'api/source_manifest.json').read_text())
    for name, expected in api['files'].items():
        require(sha((HERE / 'api/snapshot' / name).read_bytes()) == expected, 'API source SHA: ' + name)
    native = json.loads((HERE / 'native/source_manifest.json').read_text())
    for name, expected in native['files'].items():
        require(sha((HERE / 'native/sources' / name).read_bytes()) == expected['sha256'],
                'native source SHA: ' + name)
    python = [sys.executable] + (['-O'] if sys.flags.optimize else [])
    jobs = [
        ('math', python + [str(HERE / 'math/replay.py'), str(repository)], HERE / 'math/normal.json'),
        ('sort', python + ['-S', '-B', str(HERE / 'native/sort_refusal_model.py')],
         HERE / 'native/sort_refusal.json'),
    ]
    for label, command, captured in jobs:
        result = subprocess.run(command, capture_output=True, timeout=60)
        require(result.returncode == 0, label + ' replay failed: ' + result.stderr.decode())
        require(result.stderr == b'', label + ' unexpected stderr')
        require(result.stdout == captured.read_bytes(), label + ' replay bytes differ')
    print('audit_s9_wip: hashes=ok mathematical_model=ok sort_refusal_counterexample=confirmed native_runs=0 gcp_calls=0')


if __name__ == '__main__':
    main()
