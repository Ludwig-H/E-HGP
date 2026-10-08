#!/usr/bin/env python3
"""Fermeture MES-C2 locale : helpers MES-C reutilises, aucune donnee ni action distante."""
import argparse
import csv
import importlib.util
import io
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--session', type=Path, required=True)
    ap.add_argument('--repo', type=Path, required=True)
    a = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    helper_names = list(cap['helpers'])
    archive_reader = next(n for n in helper_names if '/session_l1_recuperation/' in n)
    source_reader = next(n for n in helper_names if n.endswith('/source_check.py'))
    spec = importlib.util.spec_from_file_location('archive_reader', a.repo / archive_reader)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    for name, pin in cap['helpers'].items():
        lib.same_hash((a.repo / name).read_bytes(), pin)
    for name, pin in cap['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), pin)
    fs = lib.files((a.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, cap['manifest_count'])
    for name, pin in cap['archive_files'].items():
        lib.same_hash(fs[name], pin)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == cap['commands'], 'commandes differentes')
    meta = dict(line.split('=', 1) for line in fs['results/cmd/000_mes_c/meta.txt'].decode().splitlines()
                if '=' in line)
    lib.need(all(meta[k] == v for k, v in cap['command_meta'].items()), 'metadonnees commande')
    report = json.loads(fs['results/cmd/000_mes_c/files/c/rapport_c.json'])
    lib.need({k: v for k, v in report['provenance'].items() if k.endswith('_sha256')} ==
             cap['report_provenance'], 'provenance du rapport')
    construction = fs['results/cmd/000_mes_c/files/c/construction.log'].decode()
    for key, expected in cap['construction_fields'].items():
        values = set(re.findall(r'(?:-D)?' + key + r'(?::\w+)?=([^\s]+)', construction))
        lib.need(values == {expected}, 'configuration construction')
    args = report['parametres']['argv']
    opts = dict(zip(args[::2], args[1::2]))
    lib.need(all(opts[k] == v for k, v in cap['configuration'].items()), 'configuration rapport')
    # Commande du plan : chemins prives jamais affiches ; seules les options declarees sont comparees.
    plan = json.loads((a.session / 'package/plan.json').read_text())
    planned = plan['commands'][0]['argv']
    for key, value in cap['configuration'].items():
        lib.need(planned.count(key) == 1 and planned.index(key) + 1 < len(planned) and
                 planned[planned.index(key) + 1] == value, 'configuration du plan')
    package = lib.files((a.session / 'package/package.tar.gz').read_bytes())
    for name, pin in cap['source_files'].items():
        lib.same_hash(package[name], pin)
        git = subprocess.check_output(['git', '-C', str(a.repo), 'show', cap['source_git'] + ':' + name])
        lib.need(package[name] == git, 'source du paquet differente')
    cmd = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    subprocess.run(cmd + [str(a.repo / source_reader), '--repo', str(a.repo),
                         '--package', str(a.session / 'package/package.tar.gz'),
                         '--plan', str(a.session / 'package/plan.json'),
                         '--capture', str(HERE / 'capture.json')], check=True)
    delta = subprocess.check_output(['git', '-C', str(a.repo), 'diff', '--name-only', cap['C1_source_git'],
                                     cap['source_git'], '--', 'morsehgp3D_v12/src/']).decode().splitlines()
    lib.need(delta == cap['native_delta_vs_C1'], 'delta produit modifie')
    for name in ('bench/full_probe.cpp', 'microbancs/outils/lecteur_full.py'):
        path = 'morsehgp3D_v12/' + name
        old = subprocess.check_output(['git', '-C', str(a.repo), 'show', cap['C1_source_git'] + ':' + path])
        lib.need(old == package[path], 'frontiere ou lecteur FULL different')
    receipt = json.loads((a.session / 'receipt.json').read_text())
    close = cap['closure']
    for key, value in close.items():
        if key not in ('errors_count', 'done', 'before_status', 'after_status'):
            lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'champ cloture')
    lib.need(receipt['commit'] == cap['source_git'] and receipt['commands'] == commands, 'source ou commande')
    lib.need(len(receipt['errors']) == close['errors_count'] == 0 and
             int((a.session / 'DONE').read_text()) == close['done'] == 0, 'erreur ou DONE')
    lib.need(receipt['observed_before_stop']['status'] == close['before_status'] == 'RUNNING' and
             receipt['observed_after']['status'] == close['after_status'] == 'TERMINATED', 'arret')
    lib.need(receipt['package_sha256'] == cap['package_sha256'] and receipt['plan_sha256'] == cap['plan_sha256'],
             'paquet/plan declare')
    data = [{k: v[k] for k in ('name', 'size', 'sha256')} for v in receipt['data_files']]
    lib.need(data == cap['data_declared'] and receipt['data_manifest_sha256'] == cap['data_manifest_sha256']
             and receipt['data_verified_remote'] is True, 'declaration des donnees')
    lib.need(cap['data_manifest_sha256'] == cap['local_files']['package/data/SHA256SUMS']['sha256'],
             'manifeste des donnees different')
    for name, pin in cap['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), pin)
    print('MES-C2 verified: 351 source files, 92 manifest entries, command 0, certified stop; admission separate')


if __name__ == '__main__':
    main()
