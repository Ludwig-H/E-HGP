#!/usr/bin/env python3
"""Clôture locale FULL M : enveloppe, sources, archive, commandes ; pas de banc."""
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--session', type=Path, required=True)
    a = ap.parse_args()
    c = json.loads(Path(__file__).with_name('capture.json').read_text())
    archive_helper = next(n for n in c['helpers'] if '/session_l1_recuperation/' in n)
    spec = importlib.util.spec_from_file_location('archive', a.repo/archive_helper)
    lib = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(lib)
    for n, expected in c['helpers'].items():
        lib.same_hash((a.repo/n).read_bytes(), expected)
    for n, expected in c['local_files'].items():
        lib.same_hash((a.session/n).read_bytes(), expected)
    protocol = next(n for n in c['helpers'] if n.endswith('session_m_protocole/check.py'))
    protocol_cap = json.loads((a.repo/Path(protocol).with_name('capture.json')).read_text())
    lib.need(protocol_cap['data_manifest_sha256'] == c['data_manifest_sha256'], 'entrées hors préparation')
    python = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
    subprocess.run(python+[str(a.repo/protocol), '--repo', str(a.repo), '--session', str(a.session)],
                   check=True, stdout=subprocess.PIPE)
    fs = lib.files((a.session/'results/results.tar.gz').read_bytes())
    lib.manifest(fs, c['manifest_entries'])
    for n, expected in c['archive_files'].items():
        lib.same_hash(fs[n], expected)
    commands = list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()), delimiter='\t'))
    lib.need(commands == c['commands'], 'commandes différentes')
    for command in commands:
        name = command['name']
        prefix = 'results/cmd/%03d_%s/' % (int(command['index']), name)
        meta = dict(v.split('=', 1) for v in fs[prefix+'meta.txt'].decode().splitlines() if '=' in v)
        lib.need({k:meta[k] for k in c['command_meta'][name]} == c['command_meta'][name], 'métadonnées')
        lib.need(meta['status'] == 'ok' and meta['exit_code'] == '0' and meta['group_closed'] == '1'
                 and meta['residual_group_killed'] == '0' and meta['streams_truncated'] == '0', 'groupe')
        if name not in c['gate_summaries']:
            continue
        text = fs[prefix+'stdout'].decode()
        rows = re.findall(r'^\s*(\d+)/(\d+) Test\s+#\d+: (\S+).*?'
                          r'(Passed|\*\*\*Skipped|\*\*\*Failed|\*\*\*Timeout)\s', text, re.M)
        done = {v[2] for v in rows}
        summary = dict(selected=int(rows[0][1]), finished=len(rows),
                       passed=sum(v[3]=='Passed' for v in rows), skipped=sum(v[3]=='***Skipped' for v in rows),
                       started_without_finish=[n for n in re.findall(r'Start\s+\d+: (\S+)', text) if n not in done])
        lib.need(summary == c['gate_summaries'][name] and summary['passed'] == summary['selected']
                 and not summary['started_without_finish'], 'portes')
    for label, expected in c['reports'].items():
        report = json.loads(fs[expected['path']])
        provenance = report['provenance']
        lib.need({k:v for k,v in provenance.items() if k.endswith('sha256')} == expected['hashes'],
                 'provenance du rapport')
        lib.need(all(v in provenance['cmake'] for v in expected['cmake_public']), 'CMake')
    for label, count in c['journal_counts'].items():
        lib.need(sum(n.endswith('.jsonl') and '/files/'+label+'/' in n for n in fs) == count, 'journaux')
    receipt = json.loads((a.session/'receipt.json').read_text())
    preflight = json.loads((a.session/'preflight.json').read_text())
    lib.need(receipt['data_files'] == preflight['data_files'], 'entrées modifiées depuis préparation')
    for k, v in c['closure'].items():
        if k not in ('errors_count', 'done', 'before_status', 'after_status'):
            lib.need(type(receipt[k]) is type(v) and receipt[k] == v, 'clôture')
    lib.need(receipt['commit'] == c['source_git'] and receipt['commands'] == commands, 'enveloppe')
    lib.need(len(receipt['errors']) == c['closure']['errors_count'] == 0 and
             int((a.session/'DONE').read_text()) == c['closure']['done'] == 0, 'issue')
    lib.need(receipt['observed_before_stop']['status'] == c['closure']['before_status'] == 'RUNNING' and
             receipt['observed_after']['status'] == c['closure']['after_status'] == 'TERMINATED', 'arrêt')
    lib.need([{k:v[k] for k in ('name', 'size', 'sha256')} for v in receipt['data_files']] == c['data_declared']
             and receipt['data_manifest_sha256'] == c['data_manifest_sha256'], 'métadonnées données')
    lib.need((a.session/'results/SHA256SUMS').read_text().split()[0] == receipt['results_sha256'], 'SHA local')
    for n, expected in c['local_files'].items():
        lib.same_hash((a.session/n).read_bytes(), expected)
    print('FULL M provenance verified: Git957, 356 sources + 7 targeted files; 518 manifest entries; '
          '5 commands0; 722+1 CTests Passed; 38/140/126 logs; worker0/DONE0; certified stop; no native audit')


if __name__ == '__main__':
    main()
