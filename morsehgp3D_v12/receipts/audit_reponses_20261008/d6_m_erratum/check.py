#!/usr/bin/env python3
"""Vérifie les sources et le patch documentaire en copie ; aucun moteur."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(body):
    return hashlib.sha256(body).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    def blob(pin, path):
        return subprocess.check_output(['git', '-C', str(args.repo), 'show', pin + ':' + path])
    before = blob(cap['documentation_pin'], cap['documentation_path'])
    need(sha(before) == cap['before_sha256'], 'préimage documentaire')
    need(sha(blob(cap['executed_source'], cap['pilot_path'])) == cap['pilot_sha256'], 'source pilote')
    patch = HERE / 'proposition.patch'
    need(sha(patch.read_bytes()) == cap['patch_sha256'], 'patch')
    with tempfile.TemporaryDirectory(prefix='audit-d6-m-doc-') as tmp:
        path = Path(tmp) / cap['documentation_path']
        path.parent.mkdir(parents=True)
        path.write_bytes(before)
        for options in (['--check'], []):
            subprocess.run(['git', 'apply', *options, str(patch)], cwd=tmp,
                           check=True, capture_output=True)
        need(sha(path.read_bytes()) == cap['after_sha256'], 'postimage documentaire')
    m = ((1 << 21) - 1) // 8
    need(cap['domain_bound'] == dict(max_original=m, max_scaled_2048=m * 2048,
                                    strict_upper_scaled=1 << 29), 'borne entière')
    need(m * 2048 < 1 << 29, 'borne stricte')
    print(json.dumps(dict(source=True, patch=True, domain_bound=cap['domain_bound'], native_executed=False),
                     sort_keys=True))


if __name__ == '__main__':
    main()
