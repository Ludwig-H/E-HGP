#!/usr/bin/env python3
"""Portable hash-before-execution proof reader, no private imports."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
FILES = {'README.md', 'PROTOCOL.txt', 'check.py', 'SOURCE_PINS.json',
         'normal.output.txt', 'optimized.output.txt', 'receipt.json', 'verify.py'}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def hashes():
    result = {}
    for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
        h, name = line.split('  ', 1)
        require(name in FILES and name not in result, 'manifest path/set')
        require(len(h) == 64 and digest(ROOT / name) == h, 'manifest hash: ' + name)
        result[name] = h
    require(set(result) == FILES, 'manifest completeness')
    require({p.name for p in ROOT.iterdir() if p.is_file()} == FILES | {'SHA256SUMS'},
            'unexpected packet file')
    return result


def main():
    before = hashes()
    pins = json.loads((ROOT / 'SOURCE_PINS.json').read_text())
    receipt = json.loads((ROOT / 'receipt.json').read_text())
    require(pins['prepared_before_execution'] is True, 'pre-execution pins')
    for name, h in pins['before_runs'].items():
        require(before[name] == h and receipt['source_after_runs'][name] == h, 'source changed')
    expected = (ROOT / 'normal.output.txt').read_bytes()
    require((ROOT / 'optimized.output.txt').read_bytes() == expected, 'capture normal/O')
    data = json.loads(expected)
    require(data['status'] == 'PASS' and data['scope'] == 'abstract_monotone_tree_only_rational_radii',
            'capture scope/status')
    require(data['engine_calls'] == 0 and data['gcp'] is False, 'scope mutation')
    require(data['counts'] == receipt['counts'], 'receipt counts')
    require(receipt['commands_exit_codes'] == [0, 0], 'capture commands')
    for flags in (['-B'], ['-B', '-O']):
        run = subprocess.run([sys.executable, *flags, str(ROOT / 'check.py')],
                             capture_output=True, timeout=60, check=False)
        require(run.returncode == 0 and run.stderr == b'', 'proof replay exit/stderr')
        require(run.stdout == expected, 'proof replay exact output')
    require(hashes() == before, 'packet changed during replay')
    print(json.dumps({'status': 'ARCHIVE_PASS', 'manifest_entries': len(before),
                      'proof_script_invocations_now': 2, 'counts': data['counts'],
                      'engine_calls_now': 0, 'gcp': False}, sort_keys=True))


if __name__ == '__main__':
    main()
