#!/usr/bin/env python3
"""Rejeu stdlib du seul témoin de décision, sans exécution native ni mesure."""
import ast
import hashlib
import json
from pathlib import Path


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    raw = Path(__file__).with_name('proof.json').read_bytes()
    need(hashlib.sha256(raw).hexdigest() ==
         'e4e8e54fcfc4096b4f04637ef3ed5c6201555ce3f4d6866477b4f5a917a0f0d3',
         'empreinte de preuve différente')
    proof = json.loads(raw)
    model = proof['bounded_model']
    need(model['constants']['OUTPUTS'] ==
         [['full', 'full.mhgp11ful1', 16379], ['supports', 'supports.mhgp11sp', 16379]],
         'sorties ou masques différents')
    need(not proof['actual_source_commit'].startswith(proof['declared_bench_commit_argument']),
         'le témoin exige la divergence de commit déclaré')
    function = model['function_source']
    need(hashlib.sha256(function.encode()).hexdigest() == model['function_sha256'],
         'fonction différente')
    tree = ast.parse(function)
    need(len(tree.body) == 1 and isinstance(tree.body[0], ast.FunctionDef) and
         tree.body[0].name == 'decide', 'une seule fonction decide attendue')
    scope = dict(model['constants'])
    exec(compile(tree, '<decide épinglé>', 'exec'), scope)
    decisions = []
    for fixture in model['fixtures']:
        result = scope['decide'](fixture['ratios'], fixture['defects'],
                                 fixture['prises'], fixture['k'])
        need(result == fixture['observed'], 'branche différente')
        decisions.append(result['decision'])
    need(decisions == ['build_order_par_defaut', 'livrer_L2b'], 'décisions différentes')
    print(json.dumps({'verdict': 'conforme', 'modele': 'source_only',
                      'masques': {'full': 16379, 'supports': 16379},
                      'decisions_historiques_encore_emises': decisions,
                      'native_or_cloud_actions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
