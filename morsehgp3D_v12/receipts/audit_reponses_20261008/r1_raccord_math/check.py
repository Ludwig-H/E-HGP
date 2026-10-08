#!/usr/bin/env python3
"""Contrôle des épingles uniquement ; aucun test du moteur."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    here = Path(__file__).resolve().parent
    repo = Path(sys.argv[1])
    capture = json.loads((here / 'capture.json').read_text())
    pin = capture['pin']
    def git(*args):
        return subprocess.check_output(['git', '-C', str(repo), *args])
    require(git('rev-parse', pin + '^').decode().strip() == capture['parent'], 'parent')
    for path, expected in capture['sources'].items():
        content = git('show', pin + ':' + path)
        require(len(content) == expected['bytes'], 'taille ' + path)
        require(hashlib.sha256(content).hexdigest() == expected['sha256'], 'hash ' + path)
    delta = git('diff-tree', '--no-commit-id', '--name-only', '-r', pin,
                '--', 'morsehgp3D_v12/src/').decode().splitlines()
    require(delta == capture['native_delta'], 'delta produit')
    checked = 0
    for line in (here / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        require('/' not in name and name != 'SHA256SUMS', 'nom manifeste')
        require(hashlib.sha256((here / name).read_bytes()).hexdigest() == digest, name)
        checked += 1
    print(json.dumps({'pin': pin, 'sources_exactes': len(capture['sources']),
                      'fichiers_recus_verifies': checked, 'delta_produit': delta,
                      'qualification_native': False}, sort_keys=True, ensure_ascii=False))


if __name__ == '__main__':
    main()
