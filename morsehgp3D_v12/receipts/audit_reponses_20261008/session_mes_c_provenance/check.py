#!/usr/bin/env python3
"""Fermeture locale MES-C ; sources, métadonnées et hashes, jamais de moteur/cloud."""
import argparse
import csv
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    dep = HERE.parent / 'session_l1_recuperation/check.py'
    spec = importlib.util.spec_from_file_location('prior_archive_reader', dep)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    lib.same_hash(dep.read_bytes(), cap['reused_archive_reader'])
    lib.same_hash((HERE / 'source_check.py').read_bytes(), cap['source_checker'])
    for name, pin in cap['local_files'].items():
        lib.same_hash((args.session / name).read_bytes(), pin)
    fs = lib.files((args.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, cap['manifest_count'])
    for name, pin in cap['archive_files'].items():
        lib.same_hash(fs[name], pin)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == cap['commands'], 'commands differ')
    package = lib.files((args.session / 'package/package.tar.gz').read_bytes())
    for name, pin in cap['source_files'].items():
        lib.same_hash(package[name], pin)
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['source_git'] + ':' + name])
        lib.need(body == package[name], 'packaged source differs from Git')
    cmd = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    subprocess.run(cmd + [str(HERE / 'source_check.py'), '--repo', str(args.repo),
                         '--package', str(args.session / 'package/package.tar.gz'),
                         '--plan', str(args.session / 'package/plan.json'),
                         '--capture', str(HERE / 'capture.json')], check=True)
    report = json.loads(fs['results/cmd/000_mes_c/files/c/rapport_c.json'])
    for name, pin in cap['report_provenance'].items():
        lib.need(report['provenance'][name] == pin, 'reported provenance differs')
    receipt = json.loads((args.session / 'receipt.json').read_text())
    for key, value in cap['closure'].items():
        if key in ('done', 'errors_count', 'before_status', 'after_status'):
            continue
        lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'closure field differs')
    lib.need(receipt['commit'] == cap['source_git'] and receipt['commands'] == commands, 'source/commands')
    lib.need(len(receipt['errors']) == cap['closure']['errors_count'] == 0, 'session errors')
    lib.need(int((args.session / 'DONE').read_text()) == cap['closure']['done'] == 0, 'DONE')
    lib.need(receipt['observed_before_stop']['status'] == cap['closure']['before_status'] == 'RUNNING', 'before stop')
    lib.need(receipt['observed_after']['status'] == cap['closure']['after_status'] == 'TERMINATED', 'after stop')
    lib.need(receipt['package_sha256'] == cap['package_sha256'] and receipt['plan_sha256'] == cap['plan_sha256'], 'package/plan')
    for name, pin in cap['local_files'].items():
        lib.same_hash((args.session / name).read_bytes(), pin)
    print('MES-C verified: 342 source files, 70 manifest entries, command 0, certified stop; admission separate')


if __name__ == '__main__':
    main()
