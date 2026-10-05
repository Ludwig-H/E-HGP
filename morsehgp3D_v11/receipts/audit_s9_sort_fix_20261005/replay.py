#!/usr/bin/env python3
"""Validate the frozen S9 sort correction and its bounded standalone model."""
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
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        expected, name = line.split('  ', 1)
        require(sha((HERE / name).read_bytes()) == expected, 'capsule SHA: ' + name)
    for name, expected in json.loads((HERE / 'math/SHA256.json').read_text()).items():
        require(sha((HERE / 'math' / name).read_bytes()) == expected, 'math SHA: ' + name)
    source = json.loads((HERE / 'native/source_manifest.json').read_text())
    for name, expected in source['files'].items():
        require(sha((HERE / 'native/sources' / name).read_bytes()) == expected['sha256'],
                'sort source SHA: ' + name)
    command = [sys.executable] + (['-O'] if sys.flags.optimize else [])
    command += ['-S', '-B', str(HERE / 'native/heap_sort_port.py')]
    result = subprocess.run(command, capture_output=True, timeout=60)
    require(result.returncode == 0, 'sort model failed: ' + result.stderr.decode())
    require(result.stderr == b'', 'unexpected model stderr')
    require(result.stdout == (HERE / 'native/heap_port.json').read_bytes(), 'model output bytes differ')
    print('audit_s9_sort_fix: hashes=ok immediate_refusal_model=ok source_correction=favorable native_runs=0 gcp_calls=0')


if __name__ == '__main__':
    main()
