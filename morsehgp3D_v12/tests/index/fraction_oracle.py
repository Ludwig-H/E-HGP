"""Pilote du juge global Fraction : processus natif batch, aucun resultat partiel accepte sur refus."""
import hashlib
import json
import subprocess
import sys

from fraction_model import require
from judge import canonical, check_response, parse
from requests import requests


def run(probe):
    info = subprocess.run([probe, '--profile'], capture_output=True, text=True, timeout=15)
    require(info.returncode == 0 and not info.stderr, 'profil natif en echec')
    bits = parse(info.stdout).get('coord_bits')
    require(type(bits) is int and bits in (21, 24, 32), 'profil absent')
    queries, pairs = requests(bits)
    payload = ''.join(query.encode() for query in queries)
    result = subprocess.run([probe], input=payload, capture_output=True, text=True, timeout=90)
    require(result.returncode == 0 and not result.stderr, 'pilote natif en echec : ' + result.stderr)
    lines = result.stdout.splitlines()
    require(len(lines) == len(queries), 'nombre de reponses')
    answers = [parse(line) for line in lines]
    checks = sum(check_response(req, answer, bits) for req, answer in zip(queries, answers))
    for i, j in pairs:
        require(canonical(answers[i]) == canonical(answers[j]), 'permutation entree change resultat')
        require(answers[i]['ledger'] == answers[j]['ledger'], 'permutation entree change travail')
        checks += 2
    kinds = {kind: sum(answer['kind'] == kind for answer in answers) for kind in ('complete', 'saturated', 'refused')}
    require(len(queries) >= 800 and checks >= 20000 and min(kinds['complete'], kinds['saturated']) >= 200 and
            kinds['refused'] == 6 and len(pairs) == 47, 'plancher de strate non atteint')
    print(json.dumps(dict(bits=bits, requests=len(queries), checks=checks, kinds=kinds, permutations=len(pairs),
                          input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage : fraction_oracle.py sonde', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print('ECHEC index Fraction : ' + str(error), file=sys.stderr)
        sys.exit(1)
