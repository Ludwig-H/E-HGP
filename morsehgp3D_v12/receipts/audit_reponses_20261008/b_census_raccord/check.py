#!/usr/bin/env python3
"""Liaison source/bras déjà admis et patch documentaire ; aucun moteur."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--snapshot', type=Path, required=True)
    ap.add_argument('--source-pin', help='optionnel : rattacher les dix sources à un commit livré')
    a = ap.parse_args()
    c = json.loads((HERE / 'capture.json').read_text())
    def blob(pin, path):
        return subprocess.check_output(['git', '-C', str(a.repo), 'show', pin + ':' + path])
    m = c['manifest']
    raw = blob(m['git'], m['path'])
    need(sha(raw) == m['sha256'], 'manifeste différent')
    arm = json.loads(raw)['bras']['census']['fichiers']
    for rel, expected in c['source_files'].items():
        path = 'morsehgp3D_v12/' + rel
        raw = blob(a.source_pin, path) if a.source_pin else (a.snapshot / path).read_bytes()
        need(len(raw) == expected['bytes'] and sha(raw) == expected['sha256'], 'source différente')
        target = arm[rel]['sha256_apres'] if rel in arm else sha(blob(c['before_git'], path))
        need(sha(raw) == target, 'source différente du bras census')
    result = c['admission_results']
    raw = (a.repo / result['path']).read_bytes()
    need(sha(raw) == result['sha256'], 'admission différente')
    results = json.loads(raw)
    for frame, medians in c['selected_median_process_ms'].items():
        take = results['decisive'][frame]
        need(all(take['g_ms_median_process'][arm] == v for arm, v in medians.items()), 'chiffres différents')
        need(take['work_identical'] and take['object_rows_identical'], 'portée K5 différente')
    need(not results['information_G']['k10_w48']['ng00']['work_identical'] and
         not results['information_G']['k10_w48']['ng01']['work_identical'], 'portée K10 différente')
    with tempfile.TemporaryDirectory(prefix='census-docs-') as tmp:
        folder = Path(tmp)
        for rel, expected in c['docs'].items():
            raw = (a.snapshot / rel).read_bytes()
            need(len(raw) == expected['bytes'] and sha(raw) == expected['sha256'], 'document différent')
            p = folder / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(raw)
        subprocess.run(['git', 'apply', '--unidiff-zero', '--check', str(HERE / 'docs.patch')], cwd=folder, check=True)
        subprocess.run(['git', 'apply', '--unidiff-zero', str(HERE / 'docs.patch')], cwd=folder, check=True)
        for rel, expected in c['docs'].items():
            need(sha((folder / rel).read_bytes()) == expected['proposed_sha256'], 'postimage documentaire')
    t = c['termination']
    need(sha(blob(c['context_git'], t['path'])) == t['sha256'], 'pin de terminaison différent')
    print('10 sources exactes au bras census; 49.0369205/39.198647/45.323494 ms; '
          'K5 compteurs égaux, K10 distinct; patch docs applicable en copie; aucun moteur')


if __name__ == '__main__':
    main()
