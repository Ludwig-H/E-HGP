#!/usr/bin/env python3
"""Read-only validation of the pinned S8 audit and its bounded Python proof."""
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
    math = HERE / 'math'
    for name, expected in json.loads((math / 'SHA256.json').read_text()).items():
        require(sha((math / name).read_bytes()) == expected, 'math capsule SHA: ' + name)
    source = json.loads((HERE / 'native/source_manifest.json').read_text())
    for name, expected in source['files'].items():
        if expected.get('authority') == 'rapport_local_non_versionne_adfcdc692':
            raw = (math / 'developer_reports' / Path(name).name).read_bytes()
        else:
            raw = subprocess.check_output(['git', '-C', str(repository), 'show', source['pin'] + ':' + name])
        require(sha(raw) == expected['sha256'], 'native source SHA: ' + name)
    command = [sys.executable] + (['-O'] if sys.flags.optimize else [])
    command += [str(math / 'replay.py'), str(repository)]
    result = subprocess.run(command, capture_output=True, timeout=60)
    require(result.returncode == 0, 'math replay failed: ' + result.stderr.decode())
    require(result.stderr == b'', 'unexpected math replay stderr')
    require(result.stdout == (math / 'normal.json').read_bytes(), 'math replay bytes differ')
    print('audit_s8: capsule_sha=ok native_sources=12 mathematical_replay=ok native_runs=0 gcp_calls=0')


if __name__ == '__main__':
    main()
