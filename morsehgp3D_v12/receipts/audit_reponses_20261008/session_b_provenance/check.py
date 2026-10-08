#!/usr/bin/env python3
"""Provenance B close, sans moteur, cloud ni lecture de payload de scène."""
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
    c = json.loads((HERE / 'capture.json').read_text())
    helper = next(n for n in c['helpers'] if '/session_l1_recuperation/' in n)
    spec = importlib.util.spec_from_file_location('archive', a.repo / helper)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    for name, expected in c['helpers'].items():
        lib.same_hash((a.repo / name).read_bytes(), expected)
    for name, expected in c['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), expected)
    pc = json.loads((a.repo / c['protocol_capture_path']).read_text())
    lib.same_hash(a.before_sources.read_bytes(),
                  dict(bytes=pc['source_archives']['avant']['package_bytes'],
                       sha256=pc['source_archives']['avant']['package_sha256']))
    source_reader = a.repo / pc['helper_path']
    cmd = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-b-source-') as tmp:
        for arm, archive in [('avant', a.before_sources), ('paquet', a.session / 'package/package.tar.gz')]:
            path = Path(tmp) / (arm + '.json')
            path.write_text(json.dumps(pc['source_archives'][arm]))
            subprocess.run(cmd + [str(source_reader), '--repo', str(a.repo), '--package', str(archive),
                                  '--plan', str(a.session / 'package/plan.json'), '--capture', str(path)],
                           check=True, stdout=subprocess.PIPE)
    fs = lib.files((a.session / 'results/results.tar.gz').read_bytes())
    lib.manifest(fs, c['manifest_entries'])
    for name, expected in c['archive_files'].items():
        lib.same_hash(fs[name], expected)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == c['commands'], 'commandes différentes')
    for command in commands:
        name = command['name']
        prefix = 'results/cmd/%03d_%s/' % (int(command['index']), name)
        meta = dict(line.split('=', 1) for line in fs[prefix + 'meta.txt'].decode().splitlines() if '=' in line)
        lib.need({k:meta[k] for k in c['command_meta'][name]} == c['command_meta'][name], 'meta différente')
        lib.need(meta['status'] == 'ok' and meta['exit_code'] == '0' and meta['group_closed'] == '1' and
                 meta['residual_group_killed'] == '0' and meta['streams_truncated'] == '0', 'groupe non clos')
        if name in c['gate_summaries']:
            text = fs[prefix + 'stdout'].decode()
            rows = re.findall(r'^\s*(\d+)/(\d+) Test\s+#\d+: (\S+).*?'
                              r'(Passed|\*\*\*Skipped|\*\*\*Failed|\*\*\*Timeout)\s', text, re.M)
            done = {r[2] for r in rows}
            summary = dict(selected=int(rows[0][1]), finished=len(rows), passed=sum(r[3] == 'Passed' for r in rows),
                           skipped=sum(r[3] == '***Skipped' for r in rows),
                           started_without_finish=[n for n in re.findall(r'Start\s+\d+: (\S+)', text) if n not in done])
            lib.need(summary == c['gate_summaries'][name], 'portes différentes')
            lib.need(summary['selected'] == summary['passed'] and not summary['started_without_finish'], 'porte non close')
    plan = json.loads((a.session / 'package/plan.json').read_text())
    args = plan['commands'][1]['argv']
    lib.need(args[args.index('--avant-sha256') + 1] == c['before_archive_reported'] ==
             pc['source_archives']['avant']['package_sha256'], 'archive avant du plan/rapport')
    lib.need(Path(args[args.index('--avant-archive') + 1]).name == pc['before_name'], 'nom archive avant')
    report = json.loads(fs[c['report_path']])
    build = report['construction']
    lib.need(build['archive_avant_sha256'] == c['before_archive_reported'] and
             build['bras_sha256'] == c['arms_manifest_reported'], 'déclaration de construction')
    lib.need(build['src_conforme_au_lot'] == c['source_conformity_reported'] == {'ecarts': [], 'fichiers': 10},
             'conformité des fichiers B')
    bins = {n:v['sha256'] for n,v in build['binaires'].items()}
    lib.need(bins == c['binary_hashes_declared'] == report['campagne_k5']['binaires_apres'] and
             bins['avant'] == bins['avant_bis'], 'empreintes des binaires déclarées')
    lib.need({n:None if v is None else v['sha256'] for n,v in report['informations']['constructions']['binaires'].items()}
             == c['informational_binary_hashes_declared'], 'binaires informatifs déclarés')
    lib.need(report['jugement']['verdicts'] == c['reported_verdicts'] and
             report['jugement']['refus'] == c['reported_refusals'] == [], 'verdict déclaré')
    lib.need(sum(n.endswith('.jsonl') and '/files/t2d_b/' in n for n in fs) == c['recorded_jsonl'], 'journaux')
    package = lib.files((a.session / 'package/package.tar.gz').read_bytes())
    for name, expected in pc['pilot_sources'].items():
        lib.same_hash(package[name], expected)
        git = subprocess.check_output(['git', '-C', str(a.repo), 'show', c['source_git'] + ':' + name])
        lib.need(git == package[name], 'pilote du paquet différent de Git')
    receipt = json.loads((a.session / 'receipt.json').read_text())
    for key, value in c['closure'].items():
        if key not in ('errors_count', 'done', 'before_status', 'after_status'):
            lib.need(type(receipt[key]) is type(value) and receipt[key] == value, 'clôture différente')
    lib.need(receipt['commit'] == c['source_git'] and receipt['commands'] == commands, 'enveloppe différente')
    lib.need(len(receipt['errors']) == c['closure']['errors_count'] == 0 and
             int((a.session / 'DONE').read_text()) == c['closure']['done'] == 0, 'issue différente')
    lib.need(receipt['observed_before_stop']['status'] == 'RUNNING' and
             receipt['observed_after']['status'] == 'TERMINATED', 'arrêt différent')
    lib.need([{k:v[k] for k in ('name', 'size', 'sha256')} for v in receipt['data_files']] == c['data_declared'] and
             receipt['data_manifest_sha256'] == c['data_manifest_sha256'], 'métadonnées des données différentes')
    for name, expected in c['local_files'].items():
        lib.same_hash((a.session / name).read_bytes(), expected)
    print('B provenance verified: sources902/41d4 342/357; 357 manifest entries; '
          '724+7+3 CTests Passed; worker0; 272 measurement journals; certified stop; no native audit call')


if __name__ == '__main__':
    main()
