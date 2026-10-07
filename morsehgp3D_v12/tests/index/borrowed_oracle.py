"""Census emprunte : juge Fraction existant, premier K exact et travail compare a la voie possedee.

Le facteur deux adapte seulement le schema historique du juge ; il n'est jamais
publie comme du travail execute par la nouvelle primitive. Aucun import produit.
"""
import argparse
import copy
import json
import subprocess

from fraction_model import population, require
from judge import canonical, check_response, parse
from model_test import model_answer
from requests import requests

FIELDS = ('nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes')


def validate(req, answer, bits, reference=None):
    adapted = copy.deepcopy(answer)
    if not req.refusal:
        work = answer.get('ledger')
        require(type(work) is dict and set(work) == set(FIELDS), 'inventaire travail emprunte')
        require(all(type(v) is int and 0 <= v < 2**63 for v in work.values()), 'travail entier borne')
        require(work['passes'] == 1, 'une traversee reelle')
        truth = population(req.records, req.support)
        require(answer['inner'] == truth['inner'][:req.threshold], 'premiers K interieurs Morton')
        require(answer['query_memory']['peak'] == 4 * len(truth['sites']), 'capacite n exactement reservee')
        adapted['ledger'] = {k: 2 * v for k, v in work.items()}
    checks = check_response(req, adapted, bits)
    if reference is not None:
        require(canonical(answer) == canonical(reference), 'population differe du census possede')
        if not req.refusal:
            require(adapted['ledger'] == reference['ledger'], 'parcours differents au-dela du facteur deux')
        checks += 2
    return checks + (0 if req.refusal else 5)


def truthful(req, bits):
    answer = model_answer(req, bits)
    if not req.refusal:
        answer['ledger'] = {k: v // 2 for k, v in answer['ledger'].items()}
        answer['query_memory']['peak'] = 4 * len(answer['sites'])
    return answer


def selftest():
    checks = positives = corruptions = 0
    for bits in (21, 24):
        queries, _ = requests(bits)
        for req in queries:
            checks += validate(req, truthful(req, bits), bits)
            positives += 1
        full = next(r for r in queries if r.name == 'seuil_trois_L4_K4')
        saturated = next(r for r in queries if r.name == 'seuil_trois_L4_K2')
        for req, change in (
            (full, lambda a: a['ledger'].update(passes=2)),
            (full, lambda a: a['ledger'].update(passes=True)),
            (full, lambda a: a['ledger'].update(nodes=-1)),
            (full, lambda a: a['ledger'].update(extra=0)),
            (full, lambda a: a['query_memory'].update(peak=a['query_memory']['peak']-4)),
            (full, lambda a: a['query_memory'].update(after=4)),
            (full, lambda a: a.update(shell=a['shell'][::-1])),
            (full, lambda a: a.update(shell=a['shell'][:-1])),
            (full, lambda a: a.update(inner=a['inner'][::-1])),
            (saturated, lambda a: a.update(shell=[0])),
            (saturated, lambda a: a.update(kind='complete')),
            (saturated, lambda a: a.update(inner=population(saturated.records, saturated.support)['inner'][1:3])),
        ):
            bad = truthful(req, bits)
            change(bad)
            try:
                validate(req, bad, bits)
            except (ValueError, KeyError, TypeError):
                corruptions += 1
            else:
                raise ValueError('corruption census emprunte survivante')
    # Deux profils (21 et 24) : 1010 positifs, 12 corruptions et 41950 controles chacun ; la v11 en jugeait trois
    # (18 compris) avec le plancher 100000, ramene ici aux deux tiers.
    require(positives == 2020 and corruptions == 24 and checks > 66666, 'planchers modele')
    print('borrowed_model_verdict conforme positives2020 corruptions24 native0')


def execute(probe, payload):
    result = subprocess.run([probe], input=payload, capture_output=True, text=True, timeout=120)
    require(result.returncode == 0 and not result.stderr, 'pilote natif en echec')
    return [parse(line) for line in result.stdout.splitlines()]


def run(probe, owned):
    info = subprocess.run([probe, '--profile'], capture_output=True, text=True, timeout=15)
    other = subprocess.run([owned, '--profile'], capture_output=True, text=True, timeout=15)
    require(info.returncode == other.returncode == 0 and not info.stderr and not other.stderr, 'profils refuses')
    bits = parse(info.stdout).get('coord_bits')
    require(type(bits) is int and bits in (21, 24, 32) and parse(other.stdout).get('coord_bits') == bits,
            'profils apparies requis')
    queries, pairs = requests(bits)
    payload = ''.join(req.encode() for req in queries)
    answers, references = execute(probe, payload), execute(owned, payload)
    # Inventaire de la matrice : 1010 requetes aux profils 21 et 24 ; au profil 32, les tirages sur [0,2^32) donnent
    # trois seuils distincts de plus (requests.py), 1013.
    inventory = {21: 1010, 24: 1010, 32: 1013}[bits]
    require(len(answers) == len(references) == len(queries) == inventory, 'inventaire requetes')
    checks = sum(validate(req, a, bits, r) for req, a, r in zip(queries, answers, references))
    for i, j in pairs:
        require(canonical(answers[i]) == canonical(answers[j]), 'permutation entree change certificat')
        require(answers[i]['ledger'] == answers[j]['ledger'], 'permutation entree change travail')
    require(checks > 35000 and len(pairs) == 47, 'plancher Fraction')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=inventory, paired=inventory,
                          permutations=47, checks=checks, refusals=6), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe', nargs='?')
    parser.add_argument('owned', nargs='?')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest and not args.probe and not args.owned:
        selftest()
    elif args.probe and args.owned and not args.selftest:
        run(args.probe, args.owned)
    else:
        parser.error('deux pilotes ou --selftest')
