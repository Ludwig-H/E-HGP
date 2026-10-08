#!/usr/bin/env python3
"""Contre-exemple algébrique de périmètre : aucune mesure ni invocation de sonde."""
import argparse
import ast
import copy
import hashlib
import json
import math
from pathlib import Path
import random
import subprocess

HERE = Path(__file__).resolve().parent
PILOT = 'morsehgp3D_v12/microbancs/mes_t2d_a/pilote_t2d_a.py'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--repo', type=Path, required=True); args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text()); bodies = {}
    for version, paths in cap['sources'].items():
        for name, digest in paths.items():
            raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['pins'][version] + ':' + name])
            need(hashlib.sha256(raw).hexdigest() == digest, 'source différente')
            bodies[(version, name)] = raw
    tree = ast.parse(bodies[('livre', PILOT)])
    functions = {'mediane', 'bootstrap_gm', 'murs_chauds', 'juger_identite', '_juger', 'juger', 'campagne_synthetique'}
    constants = {'TRAMES', 'BRAS', 'REGLE_T2D_A'}
    nodes = [n for n in tree.body if
             isinstance(n, ast.FunctionDef) and n.name in functions or
             isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in constants]
    need(len(nodes) == len(functions) + len(constants), 'extraction incomplète')
    namespace = dict(math=math, json=json, random=random)
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'fonctions_pures_capturees', 'exec'), namespace)
    make, judge = namespace['campagne_synthetique'], namespace['juger']
    cases = []
    for before, sequential, overlap, expected_history, expected_route in (
        (100, 70, 77, 'adopte', 'rejete'),
        (100, 120, 108, 'rejete', 'adopte'),
    ):
        report = make({t: overlap / before for t in namespace['TRAMES']}, bruit=0)
        for tours in report['campagne_k5']['trames'].values():
            for tour in tours:
                tour['avant']['murs_ns'] = [before] * 10
                tour['apres']['murs_ns'] = [overlap] * 10
                # Ce champ est volontairement ignoré par la comparaison BRAS=(avant,apres).
                tour['apres_sequentiel'] = dict(valide=True, murs_ns=[sequential] * 10)
        historical = judge(report, '', verifier=False)
        paired = copy.deepcopy(report)
        for tours in paired['campagne_k5']['trames'].values():
            for tour in tours:
                # Même formule statistique ; substitution du dénominateur, pas campagne réelle admise.
                tour['avant'] = tour['apres_sequentiel']
        causal = judge(paired, '', verifier=False)
        need(historical['verdict'] == expected_history and causal['verdict'] == expected_route, 'contre-exemple')
        h = historical['cas']['ng00']['moyenne_geometrique']
        a = causal['cas']['ng00']['moyenne_geometrique']
        need(abs(h - (sequential / before) * a) < 1e-12, 'factorisation')
        cases.append(dict(unites_arbitraires=dict(historique=before, sequentiel_actuel=sequential, recouvert=overlap),
                          rapport_historique=h, rapport_route=a,
                          verdict_historique=historical['verdict'], verdict_nouvelle_route=causal['verdict']))
    print(json.dumps(dict(cases=cases, regle_originale_inchangee=True, voie_du_juge='synthetique verifier=False',
                         admission_de_bruts=False, native_execution=False, remote_execution=False),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
