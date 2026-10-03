#!/usr/bin/env python3
"""Lecteur du recu points_g4 : empreintes, arrets certifies, portes, tableaux recalcules des archives.

    python3 -B morsehgp3D_v11/receipts/developpement_20261003/points_g4/check.py

Code 0 si les empreintes concordent, si chaque session a un arret cible certifie (TERMINATED relu), si chaque
porte rend « conforme » sans desaccord, si chaque scene publiee est « ok » et si les sessions claudepts3 et
claudepts4 (meme regle a l'arithmetique pres, memes donnees) publient des scenes identiques hors chronometrage ;
1 sinon. Une etape coupee par l'echeance de la VM (deadline_cut) est declaree, pas refusee : ses scenes absentes
manquent simplement aux tableaux. Python 3.10 nu, aucun assert.

Les tableaux sont relus SESSION PAR SESSION (jamais de melange de versions de code) par bench/points_summary.py ;
le synthetique est en outre separe par taille de nuage (ecart apparie par scene, bootstrap a graine fixe).
"""
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent
MODULE = HERE.parents[2]
SESSIONS = ('claudepts1', 'claudepts2', 'claudepts3', 'claudepts4', 'claudepts5')
SAME = ('claudepts3', 'claudepts4')
TIMING = ('export_ns', 'full_ns', 'seconds', 'wall_seconds')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def members(archive, prefix):
    with tarfile.open(archive) as t:
        for m in t.getmembers():
            if m.isfile() and m.name.startswith(prefix):
                yield m.name, t.extractfile(m).read()


def strip_timing(x):
    if isinstance(x, dict):
        return {k: strip_timing(v) for k, v in x.items() if k not in TIMING}
    if isinstance(x, list):
        return [strip_timing(v) for v in x]
    return x


def by_size(scenes, rules=('margin_r', 'margin', 'first', 'cover', 'core')):
    """Ecart apparie par scene (moyenne des meilleurs IoU par objet), regle - HDBSCAN, par taille et par ordre."""
    rows = {}
    for d in scenes:
        n = d['meta']['spec']['n']
        for k, o in d['orders'].items():
            if 'hdbscan' not in o:
                continue
            h = sum(o['hdbscan']['best']) / len(o['hdbscan']['best'])
            for rule in rules:
                if rule in o:
                    r = sum(o[rule]['best']) / len(o[rule]['best'])
                    rows.setdefault((n, int(k), rule), []).append(r - h)
    rng = random.Random(20261003)
    lines = []
    for (n, k, rule) in sorted(rows):
        diffs = rows[(n, k, rule)]
        mean = sum(diffs) / len(diffs)
        boots = sorted(sum(rng.choice(diffs) for _ in diffs) / len(diffs) for _ in range(2000))
        lines.append('  n=%-5d k=%-2d %-8s scenes %3d ecart %+.4f [%+.4f, %+.4f] scenes>=HDBSCAN %d' % (
            n, k, rule, len(diffs), mean, boots[49], boots[1949], sum(x >= 0 for x in diffs)))
    return lines


def main():
    problems = []
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        path = HERE / name
        if not path.is_file() or sha256(path) != digest:
            problems.append('empreinte : ' + name)
    work = Path(tempfile.mkdtemp(prefix='points_g4_'))
    published = {}
    for name in SESSIONS:
        base = HERE / 'sessions' / name
        if not base.is_dir():
            print('==', name, 'absente')
            continue
        receipt = json.loads((base / 'receipt.json').read_text())
        after = receipt.get('observed_after') or {}
        if receipt.get('targeted_shutdown_certified') is not True or after.get('status') != 'TERMINATED':
            problems.append('arret cible non certifie : ' + name)
        commands = {c['name']: c for c in receipt.get('commands', [])}
        print('==', name, receipt.get('status'), receipt.get('evidence_grade'), receipt.get('source_kind'),
              {k: (c['status'], c['exit_code']) for k, c in commands.items()})
        for step, c in commands.items():
            if c['status'] not in ('ok', 'deadline_cut'):
                problems.append('etape en echec : %s/%s' % (name, step))
        synthetic, lidar, scenes = set(), set(), []
        published[name] = {}
        for path, data in members(base / 'results.tar.gz', 'results/cmd/'):
            parts = path.split('/')
            if path.endswith('/gate.json'):
                gate = json.loads(data)
                print('  porte', gate['verdict'], 'nuages', gate['clouds'], 'comparaisons', gate['comparisons'],
                      'sites rayon', gate.get('radius_sites', '-'),
                      'fixtures', sum(f['ok'] for f in gate['fixtures']), '/', len(gate['fixtures']),
                      'mutants', {k: v['killed'] for k, v in gate.get('mutants', {}).items()} or '-')
                if gate['verdict'] != 'conforme' or gate['disagreements']:
                    problems.append('porte non conforme : ' + name)
            elif path.endswith('/prepare.json'):
                prep = json.loads(data)
                print('  preparation : statut', prep['status'], 'sonde', prep['probe']['ok'], 'scenes', prep['scenes'],
                      'ecarts de rejeu', sum(1 for f in prep['frames'] if not f['replay']['identical']))
                if not prep['probe']['ok']:
                    problems.append('sonde de sol non conforme : ' + name)
            elif len(parts) >= 6 and parts[-2] in ('synthetic', 'lidar') and path.endswith('.json'):
                scene = json.loads(data)
                if scene.get('status') != 'ok':
                    problems.append('scene en echec : %s/%s' % (name, scene.get('name')))
                published[name][(parts[-2], parts[-1])] = strip_timing(scene)
                target = work / name / parts[-2]
                target.mkdir(parents=True, exist_ok=True)
                (target / parts[-1]).write_bytes(data)
                (synthetic if parts[-2] == 'synthetic' else lidar).add(str(target))
                if parts[-2] == 'synthetic':
                    scenes.append(scene)
        print('  scenes publiees : synthetiques %d, LiDAR %d' % (
            sum(1 for k in published[name] if k[0] == 'synthetic'), sum(1 for k in published[name] if k[0] == 'lidar')))
        argv = [sys.executable, '-B', str(MODULE / 'bench' / 'points_summary.py')]
        for directory in sorted(synthetic):
            argv += ['--synthetic', directory]
        for directory in sorted(lidar):
            argv += ['--lidar', directory]
        for manifest in sorted(HERE.glob('data_manifest_*.json')):
            argv += ['--manifest', str(manifest)]
        if synthetic or lidar:
            done = subprocess.run(argv, capture_output=True, text=True)
            print(done.stdout)
            if done.returncode != 0:
                problems.append('lecture des tableaux : %s %s' % (name, done.stderr[-300:]))
        if scenes:
            print('  synthetique par taille de nuage (regle - HDBSCAN, ecart apparie par scene) :')
            print('\n'.join(by_size(scenes)))
    if all(s in published for s in SAME):
        a, b = published[SAME[0]], published[SAME[1]]
        common = sorted(set(a) & set(b))
        same = sum(a[k] == b[k] for k in common)
        print('== identite %s / %s : %d scenes communes, %d identiques hors chronometrage' % (
            SAME[0], SAME[1], len(common), same))
        if same != len(common) or not common:
            problems.append('scenes differentes entre %s et %s' % SAME)
    print('points_g4_verdict', 'conforme' if not problems else 'refus', problems)
    return 0 if not problems else 1


if __name__ == '__main__':
    raise SystemExit(main())
