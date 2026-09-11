#!/usr/bin/env python3
"""Create a compact source-backed publication; never copy ELF or vendor files."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if len(sys.argv) != 2:
        return 2
    target = Path(sys.argv[1]).resolve()
    target.mkdir()
    for mode, directory in (('o2', 'o2_final'), ('san', 'san_final')):
        run = HERE / directory
        summary = json.loads((run / 'summary.json').read_text())
        if summary['status'] != 'passed':
            raise ValueError('unclosed run:' + directory)
        if mode == 'o2':
            shutil.copytree(run / 'source', target / 'source')
        destination = target / 'runs' / mode
        destination.mkdir(parents=True)
        for path in sorted(run.iterdir()):
            if path.is_file() and path.name != 'gate':
                shutil.copyfile(path, destination / path.name)
    history = target / 'history' / 'o2_r1'
    history.mkdir(parents=True)
    for path in sorted((HERE / 'o2_r1').iterdir()):
        if path.is_file() and path.name != 'gate':
            shutil.copyfile(path, history / path.name)
    for name in ('t2_gate.cpp', 't2_oracle.hpp'):
        shutil.copyfile(HERE / 'o2_r1' / 'source' / name, history / name)
    for name in ('verify.py', 'record.py', 'publish.py', 'PLAN.md'):
        shutil.copyfile(HERE / name, target / name)
    manifest = {
        'schema': 'mhgp7-bounded-t2-census-full-v1',
        'source_commit': 'c03f6be8488453486b112811071827a96303ec86',
        'public_status': 'not_claimed', 'GCP_used': False,
        'authority': 'bounded_independent_Gram_Gamma_real_census_not_universal_completeness',
        'files': {str(p.relative_to(target)): sha(p) for p in sorted(target.rglob('*')) if p.is_file()}
    }
    (target / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'status': 'published', 'target': str(target), 'files': len(manifest['files'])}))
    return 0


if __name__ == '__main__':
    sys.exit(main())
