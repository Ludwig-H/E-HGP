#!/usr/bin/env python3
"""Fermeture locale T2d-C et sources avant/après ; pas de moteur ni de cloud."""
import argparse
import csv
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--before-archive', type=Path, required=True, help='archive de SOURCES 902, jamais données XYZ')
    p.add_argument('--repo', type=Path, required=True)
    args = p.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    dep = HERE.parent / 'session_l1_recuperation/check.py'
    spec = importlib.util.spec_from_file_location('prior_archive_reader', dep)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    lib.same_hash(dep.read_bytes(), cap['reused_archive_reader'])
    checker = HERE.parent / 'session_mes_c_provenance/source_check.py'
    lib.same_hash(checker.read_bytes(), cap['reused_source_checker'])
    for name, pin in cap['local_files'].items():
        lib.same_hash((args.session / name).read_bytes(), pin)
    fs = lib.files((args.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, cap['manifest_count'])
    for name, pin in cap['archive_files'].items():
        lib.same_hash(fs[name], pin)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == cap['commands'], 'commandes differentes')
    package = lib.files((args.session / 'package/package.tar.gz').read_bytes())
    for name, pin in cap['pilot_sources'].items():
        lib.same_hash(package[name], pin)
        raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['source_commit'] + ':' + name])
        lib.need(raw == package[name], 'source du pilote differente')
    with tempfile.TemporaryDirectory(prefix='audit-t2dc-sources-') as tmp:
        for label, archive in [('before', args.before_archive), ('after', args.session / 'package/package.tar.gz')]:
            source_cap = Path(tmp) / (label + '.json')
            source_cap.write_text(json.dumps(cap['source_archives'][label]))
            command = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
            subprocess.run(command + [str(checker), '--repo', str(args.repo), '--package', str(archive),
                           '--plan', str(args.session / 'package/plan.json'), '--capture', str(source_cap)], check=True)
    report = json.loads(fs['results/cmd/000_t2dc_flux/files/t2dc/report.json'])
    lib.need(report['options'] == cap['reported_options'], 'options du rapport')
    lib.need(report['options']['avant_sha256'] == cap['source_archives']['before']['package_sha256'], 'reference avant')
    lib.need(report['steps']['builds'] == cap['reported_builds'], 'constructions annoncees')
    lib.need(report['steps']['binaries'] == report['steps']['binaries_after'] == cap['declared_binary_hashes'], 'hashes binaires declares')
    lib.need(report['verdict']['verdict'] == cap['reported_outcome']['verdict'], 'verdict declare')
    receipt = json.loads((args.session / 'receipt.json').read_text())
    for key, value in cap['closure'].items():
        if key in ('done', 'errors_count', 'before_status', 'after_status'):
            continue
        lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'fermeture differente')
    lib.need(receipt['commit'] == cap['source_commit'] and receipt['commands'] == commands, 'source/commande')
    lib.need(len(receipt['errors']) == cap['closure']['errors_count'] == 0, 'erreurs de session')
    lib.need(int((args.session / 'DONE').read_text()) == cap['closure']['done'] == 0, 'DONE')
    lib.need(receipt['observed_before_stop']['status'] == cap['closure']['before_status'] == 'RUNNING', 'avant arret')
    lib.need(receipt['observed_after']['status'] == cap['closure']['after_status'] == 'TERMINATED', 'apres arret')
    lib.need(receipt['package_sha256'] == cap['source_archives']['after']['package_sha256'], 'paquet')
    for name, pin in cap['local_files'].items():
        lib.same_hash((args.session / name).read_bytes(), pin)
    print('T2d-C: 454 entrees, sources 902/02b (342/351 fichiers), ancien Pool commun, commande0 et arret certifie')


if __name__ == '__main__':
    main()
