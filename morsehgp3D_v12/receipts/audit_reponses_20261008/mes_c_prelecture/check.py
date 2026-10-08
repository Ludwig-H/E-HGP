#!/usr/bin/env python3
"""Rejoue seulement verdicts, extrait par AST ; aucune sonde ni campagne."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def require(ok, message):
    if not ok:
        raise ValueError(message)


def reader(text, origin):
    node = next(n for n in ast.parse(text).body if isinstance(n, ast.FunctionDef) and n.name == 'verdicts')
    env = {'FIXED_LIMIT_NS': 2e6, 'MAIN_REGIME_NS_PER_SITE': 241.3e6 / 64740}
    exec(compile(ast.Module(body=[node], type_ignores=[]), str(origin), 'exec'), env)
    return env['verdicts']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True, type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    pins = json.loads((here / 'pins.json').read_text())
    raw = args.source.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == pins['source_sha256'], 'source hors pin')
    before = reader(raw, args.source)
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / 'morsehgp3D_v12/microbancs/mes_c_petits/pilote_c.py'
        path.parent.mkdir(parents=True)
        path.write_bytes(raw)
        subprocess.run(['git', 'apply', str(here / 'cohorte_proposed.patch')], cwd=tmp, check=True)
        fixed = path.read_bytes()
        require(hashlib.sha256(fixed).hexdigest() == pins['postimage_sha256'], 'postimage hors pin')
        after = reader(fixed, path)
        subprocess.run(['git', 'apply', '-R', str(here / 'cohorte_proposed.patch')], cwd=tmp, check=True)
        require(path.read_bytes() == raw, 'inverse non identique')

    config = {'cpu:5:48': {'droites': {'reel': {'fixe_ns': 1e6, 'par_site_ns': 1000}}}}
    names = ['lattice_fixture', 'sphere_fixture']
    full = [dict(nom=n, voie=v, etat='ok', raison='', k=5, fils=48)
            for n in names for v in ('cpu', 'appareil')]
    cases = {
        'complete': (full, 'tenu'),
        'un_non_joue': (full[:-1] + [dict(full[-1], etat='non_joue')], 'non evalue'),
        'cpu_seul': ([r for r in full if r['voie'] == 'cpu'], 'non evalue'),
        'mauvais_fils': ([dict(r, fils=1) for r in full], 'non evalue'),
        'cas_absent': (full[:-1], 'non evalue'),
        'doublon': (full + [full[0]], 'non evalue'),
        'tous_non_joues': ([dict(r, etat='non_joue') for r in full], 'non evalue'),
        'refus_reel': (full[:-1] + [dict(full[-1], etat='refus', raison='wide_leaf')], 'non tenu'),
        'expiration': (full[:-1] + [dict(full[-1], etat='echec', raison='expire')], 'non tenu'),
        'k10_informatif': (full + [dict(full[0], k=10, etat='refus')], 'tenu'),
        'permutation': (list(reversed(full)), 'tenu'),
        'vide': ([], 'non evalue'),
    }
    result = {}
    for name, (rows, expected) in cases.items():
        old = before(config, rows)
        new = after(config, rows, names)
        require(new['C3']['etat'] == expected, name)
        require(old['C1'] == new['C1'] and old['C2'] == new['C2'], 'C1/C2 changes')
        result[name] = {'avant': old['C3']['etat'], 'propose': new['C3']['etat']}
    require(after(config, [], [])['C3']['etat'] == 'non evalue', 'aucune famille declaree')
    print(json.dumps({'native_execution': False, 'cases': result}, sort_keys=True))


if __name__ == '__main__':
    main()
