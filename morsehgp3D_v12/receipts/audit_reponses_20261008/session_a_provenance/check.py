#!/usr/bin/env python3
"""Provenance A locale, sans moteur ni donnees de scene ; deux archives de sources distinctes."""
import argparse
import csv
import importlib.util
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--session', type=Path, required=True)
    ap.add_argument('--before-sources', type=Path, required=True)
    a = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    helper = next(n for n in cap['helpers'] if '/session_l1_recuperation/' in n)
    spec = importlib.util.spec_from_file_location('archive', a.repo / helper)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    for name, pin in cap['helpers'].items():
        lib.same_hash((a.repo / name).read_bytes(), pin)
    for name, pin in cap['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), pin)
    before, after = (cap['source_archives'][arm] for arm in ('avant', 'apres'))
    lib.same_hash(a.before_sources.read_bytes(), dict(bytes=before['package_bytes'], sha256=before['package_sha256']))
    fs = lib.files((a.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, cap['manifest_count'])
    for name, pin in cap['archive_files'].items():
        lib.same_hash(fs[name], pin)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == cap['commands'], 'commandes differentes')
    for command in commands:
        prefix = 'results/cmd/%03d_%s/' % (int(command['index']), command['name'])
        meta = dict(line.split('=', 1) for line in fs[prefix + 'meta.txt'].decode().splitlines() if '=' in line)
        lib.need(meta['exit_code'] == command['exit_code'] and meta['status'] == command['status'], 'code/meta')
        if command['name'] in cap['gate_summaries']:
            text = fs[prefix + 'stdout'].decode()
            rows = re.findall(r'^\s*(\d+)/(\d+) Test\s+#\d+: (\S+).*? '
                              r'(Passed|\*\*\*Skipped|\*\*\*Failed|\*\*\*Timeout)\s', text, re.M)
            done = {r[2] for r in rows}
            summary = dict(selected=int(rows[0][1]), finished=len(rows),
                           passed=sum(r[3] == 'Passed' for r in rows), skipped=sum(r[3] == '***Skipped' for r in rows),
                           started_without_finish=[n for n in re.findall(r'Start\s+\d+: (\S+)', text) if n not in done])
            lib.need(summary == cap['gate_summaries'][command['name']], 'selection/resultats CTest')
    plan = json.loads((a.session / 'package/plan.json').read_text())
    args = plan['commands'][1]['argv']
    lib.need(args[args.index('--avant-sha256') + 1] == before['package_sha256'], 'source avant du plan')
    lib.need(Path(args[args.index('--avant-archive') + 1]).name == 'v12_src_27eca166b.tar.gz', 'nom source avant')
    report = json.loads(fs['results/cmd/001_t2da_pilote/files/t2da/rapport_t2d_a.json'])
    lib.need(report['construction']['archive_avant_sha256'] == before['package_sha256'] ==
             cap['before_archive_reported'], 'source avant du rapport')
    binaries = {n: v['sha256'] for n, v in report['construction']['binaires'].items()}
    lib.need(binaries == cap['binary_hashes_declared'] == report['campagne_k5']['binaires_apres'], 'binaires declares')
    lib.need(report['jugement']['verdict'] == cap['pilot_report_verdict'], 'verdict declare')
    package = lib.files((a.session / 'package/package.tar.gz').read_bytes())
    pilot = cap['pilot_source']
    lib.same_hash(package[pilot['path']], pilot)
    git_pilot = subprocess.check_output(['git', '-C', str(a.repo), 'show', after['source_git'] + ':' + pilot['path']])
    lib.need(package[pilot['path']] == git_pilot, 'pilote source')
    source_reader = next(n for n in cap['helpers'] if n.endswith('/source_check.py'))
    cmd = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-a-source-') as folder:
        for arm, archive in [('avant', a.before_sources), ('apres', a.session / 'package/package.tar.gz')]:
            tmp = Path(folder) / (arm + '.json')
            tmp.write_text(json.dumps(cap['source_archives'][arm]))
            subprocess.run(cmd + [str(a.repo / source_reader), '--repo', str(a.repo), '--package', str(archive),
                                  '--plan', str(a.session / 'package/plan.json'), '--capture', str(tmp)], check=True)
    def blob(pin, path):
        return subprocess.check_output(['git', '-C', str(a.repo), 'show', pin + ':' + path])
    delta = subprocess.check_output(['git', '-C', str(a.repo), 'diff', '--name-only', before['source_git'],
                                     after['source_git'], '--', 'morsehgp3D_v12/src/']).decode().splitlines()
    lib.need(delta == cap['native_delta'] and not any('/catalogue/' in n for n in delta), 'delta natif')
    pool = 'morsehgp3D_v12/src/sched/pool.cpp'
    def without_comments(raw):
        return b''.join(l for l in raw.splitlines(keepends=True) if not l.lstrip().startswith(b'//'))
    lib.need(without_comments(blob(before['source_git'], pool)) == without_comments(blob(after['source_git'], pool)),
             'corps pool differents')
    receipt = json.loads((a.session / 'receipt.json').read_text())
    for key, value in cap['closure'].items():
        if key in ('errors_count', 'done', 'before_status', 'after_status'):
            continue
        lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'champ cloture')
    lib.need(receipt['commit'] == after['source_git'] and receipt['commands'] == commands, 'enveloppe')
    lib.need(len(receipt['errors']) == cap['closure']['errors_count'] == 0, 'incident controleur')
    lib.need(int((a.session / 'DONE').read_text()) == cap['closure']['done'] == 3, 'DONE')
    lib.need(receipt['observed_before_stop']['status'] == 'RUNNING' and
             receipt['observed_after']['status'] == 'TERMINATED', 'arret')
    lib.need([{k: v[k] for k in ('name', 'size', 'sha256')} for v in receipt['data_files']] ==
             cap['data_declared'] and receipt['data_manifest_sha256'] == cap['data_manifest_sha256'], 'metadonnees')
    for name, pin in cap['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), pin)
    print('A provenance verified: 351/356 sources, 128 manifest entries; worker1, LiDAR124, pilot0, certified stop')


if __name__ == '__main__':
    main()
