#!/usr/bin/env python3
"""Python synthétique, sans moteur. Usage: check.py DEPOT PILOTE_EPINGLE"""
import ast
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C = json.loads((HERE / 'capture.json').read_text())
REPO, PILOT = map(Path, sys.argv[1:3])


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


source = PILOT.read_bytes()
need(hashlib.sha256(source).hexdigest() == C['files']['pilote_t2d_a.py']['sha256'], 'pilote différent')
fixture = subprocess.check_output(['git', '-C', str(REPO), 'show', C['head'] + ':' + C['fixture_path']])
need(hashlib.sha256(fixture).hexdigest() == C['fixture_sha256'], 'fixture publiée différente')
functions = [n for n in ast.parse(fixture).body if isinstance(n, ast.FunctionDef) and n.name in ('rows', 'take', 'report')]
need(len(functions) == 3, 'port explicite des trois fabriques')


def exercise(path, corrected):
    spec = importlib.util.spec_from_file_location('pilot', path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    scope = {'m': m, 'json': json}
    exec(compile(ast.Module(body=functions, type_ignores=[]), 'fixture-publiee', 'exec'), scope)
    result = {}
    with tempfile.TemporaryDirectory(prefix='t2da-json-') as folder:
        root = Path(folder)
        nominal = scope['report'](root)
        nominal['environnement'] = {k: {'gpu': 'GPU synthétique', 'gpu_apps': ''} for k in ('avant', 'apres')}
        nominal['campagne_k5']['fils'] = 48  # champ effectivement émis par etape_campagne

        def judge(name, report, valid=False):
            out = m.juger(report, str(root), verifier=True)
            expected = 'adopte' if valid or not corrected else 'refuse'
            need(out['verdict'] == expected, name + ': verdict inattendu')
            need(bool(out['refus']) == (expected == 'refuse'), name + ': contrôle non bloquant')
            result[name] = {'verdict': out['verdict'], 'refus': out['refus']}

        judge('nominal', nominal, True)
        other = copy.deepcopy(nominal)
        other['environnement']['avant']['gpu_apps'] = None
        judge('gpu_apps_inconnu', other)
        other = copy.deepcopy(nominal)
        other['campagne_k5']['trames']['autre'] = [{b: scope['take'](root, 'extra_' + b, b, ['autre'] * 10)
                                                   for b in m.BRAS}]
        judge('trame_etrangere_complete', other)
        other = copy.deepcopy(nominal)
        for frame, tours in other['campagne_k5']['trames'].items():
            for i, tour in enumerate(tours):
                take = scope['take'](root, 'seq_%s_%s' % (frame, i), 'apres', [frame] * 10, schema='sequentiel')
                path = root / take['journal']
                rows = [json.loads(line) for line in path.read_text().splitlines()]
                for row in rows:
                    if row['phase'] == 'full':
                        row['wall_ns'] = 70000
                        row['etapes_ns'].update(G=25000, TMVR=15000, T=5000, M=1000, V=1000, R=1000)
                path.write_text(''.join(json.dumps(x) + '\n' for x in rows))
                take.update(m.lire_prise(str(path), 0, take['attendu']))
                take['journal_sha256'] = m.sha256(path)
                tour['apres'] = take
        judge('apres_sequentiel_sans_recouvrement', other)
    return result


result = {'capture': exercise(PILOT, False)}
with tempfile.TemporaryDirectory(prefix='t2da-patch-') as folder:
    root = Path(folder)
    p = root / 'morsehgp3D_v12/microbancs/mes_t2d_a/pilote_t2d_a.py'
    p.parent.mkdir(parents=True)
    p.write_bytes(source)
    for flags in (['--check'], []):
        subprocess.run(['git', 'apply', *flags, str(HERE / 'campagne.patch')], cwd=root, check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    result['proposition'] = exercise(p, True)
print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))
