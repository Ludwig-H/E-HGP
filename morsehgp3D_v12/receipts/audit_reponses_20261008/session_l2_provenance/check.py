#!/usr/bin/env python3
"""Local provenance only; imports the published archive checker, no cloud/engine."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--repo-root', type=Path, default=HERE.parents[3])
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    dep = HERE.parent / 'session_l1_recuperation/check.py'
    spec = importlib.util.spec_from_file_location('prior_archive_reader', dep)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    lib.same_hash(dep.read_bytes(), cap['reused_reader'])
    for name, pin in cap['local_files'].items():
        lib.same_hash((args.session / name).read_bytes(), pin)
    fs = lib.files((args.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, cap['manifest_count'])
    for name, pin in cap['archive_files'].items():
        lib.same_hash(fs[name], pin)
    package = lib.files((args.session / 'package/package.tar.gz').read_bytes())
    for name, pin in cap['source_files'].items():
        lib.same_hash(package[name], pin)
        git = subprocess.check_output(['git', 'show', cap['source_commit'] + ':' + name], cwd=args.repo_root)
        lib.need(git == package[name], 'packaged source differs from Git')
    receipt = json.loads((args.session / 'receipt.json').read_text())
    close = cap['closure']
    for key, value in close.items():
        if key in ('done', 'errors_count', 'before_status', 'after_status'):
            continue
        lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'closure field differs')
    lib.need(receipt['commit'] == cap['source_commit'] and receipt['commands'] == cap['commands'], 'source/command')
    lib.need(len(receipt['errors']) == close['errors_count'] == 0, 'session errors')
    lib.need(int((args.session / 'DONE').read_text()) == close['done'] == 0, 'DONE')
    lib.need(receipt['observed_before_stop']['status'] == close['before_status'] == 'RUNNING', 'before stop')
    lib.need(receipt['observed_after']['status'] == close['after_status'] == 'TERMINATED', 'after stop')
    print('verified L2: source a2c2fccfd, archive 52f1dded, manifest 46, command code 0, certified stop; admission separate')


if __name__ == '__main__':
    main()
