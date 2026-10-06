#!/usr/bin/env python3
"""Verify frozen documentation deltas in an isolated copy, stdlib only."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def need(value, reason):
    if not value:
        raise ValueError(reason)


parser = argparse.ArgumentParser()
parser.add_argument('--repo', default='/workspaces/E-HGP')
args = parser.parse_args()
base = Path(__file__).resolve().parent
meta = json.loads((base / 'sources.json').read_text())
with tempfile.TemporaryDirectory(prefix='mhgp11_contract_review_') as directory:
    target = Path(directory)
    for path, record in meta['files'].items():
        data = subprocess.check_output(['git', '-C', args.repo, 'show', meta['base_pin'] + ':' + path])
        need(hashlib.sha256(data).hexdigest() == record['base_sha256'], 'base hash: ' + path)
        (target / path).parent.mkdir(parents=True, exist_ok=True)
        (target / path).write_bytes(data)
    for patch, hash_key in [('wip.patch', 'wip_sha256'), ('proposed.patch', 'proposed_sha256')]:
        for options in [['--check'], []]:
            result = subprocess.run(['git', 'apply', *options, str(base / patch)], cwd=target,
                                    capture_output=True, text=True)
            need(result.returncode == 0, patch + ': ' + result.stderr)
        for path, record in meta['files'].items():
            need(hashlib.sha256((target / path).read_bytes()).hexdigest() == record[hash_key],
                 hash_key + ': ' + path)
print('contract proposal PASS: two WIP documents reconstructed and proposed patch verified; native0 cloud0')
