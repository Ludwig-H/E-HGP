"""Census garde (NUM-GARDE) contre census generique et contre l'oracle Fraction, sur la matrice des requetes de l'index.

Pour chaque requete : le support est certifiable (centre dans l'enveloppe convexe, signes barycentriques en Fraction,
comme num::CertifiedBall::certify) ou non. Non certifiable : la sonde gardee doit rendre "uncertified" sans census.
Certifiable : la reponse gardee doit avoir exactement les populations, le genre et les refus du census generique, et
passer le juge Fraction. Les compteurs de la garde (boites disjointes, partielles, sites hors du pave) doivent etre
exerces. Python 3.10 nu, aucun assert.
"""
import hashlib
import json
import subprocess
import sys
from fractions import Fraction

from fraction_model import dot, require, solve, sub
from judge import canonical, check_response, parse
from requests import requests


def certifiable(support):
    """Centre dans l'enveloppe convexe ouverte du support (q3 aigu, q4 poids strictement positifs) ; q1, q2 toujours."""
    q = len(support)
    if q == 1:
        return True
    if q == 2:
        return support[0] != support[1]
    if q == 3:
        a, b, c = support
        return dot(sub(b, a), sub(c, a)) > 0 and dot(sub(a, b), sub(c, b)) > 0 and dot(sub(a, c), sub(b, c)) > 0
    anchor = support[0]
    edges = [sub(point, anchor) for point in support[1:]]
    weights = solve([[dot(u, v) for v in edges] for u in edges], [Fraction(dot(u, u), 2) for u in edges])
    if weights is None:
        return False
    return all(w > 0 for w in weights) and sum(weights) < 1


def answers(probe, payload, flag):
    command = [probe] + ([flag] if flag else [])
    result = subprocess.run(command, input=payload, capture_output=True, text=True, timeout=120)
    require(result.returncode == 0 and not result.stderr, 'pilote natif en echec : ' + result.stderr)
    return [parse(line) for line in result.stdout.splitlines()]


def run(probe):
    info = subprocess.run([probe, '--profile'], capture_output=True, text=True, timeout=15)
    require(info.returncode == 0 and not info.stderr, 'profil natif en echec')
    bits = parse(info.stdout).get('coord_bits')
    require(type(bits) is int and bits in (21, 24, 32), 'profil absent')
    queries, _ = requests(bits)
    payload = ''.join(query.encode() for query in queries)
    generic, guarded = answers(probe, payload, ''), answers(probe, payload, '--guarded')
    require(len(generic) == len(guarded) == len(queries), 'nombre de reponses')
    checks, counts = 0, dict(certified=0, uncertified=0, disjoint=0, partial=0, outside=0, wide=0, refused=0)
    for req, plain, guard in zip(queries, generic, guarded):
        if not certifiable(req.support):
            require(guard == dict(status='uncertified', coord_bits=bits, threshold=req.threshold),
                    req.name + ': support non certifiable mais census garde')
            counts['uncertified'] += 1
            checks += 1
            continue
        require(guard.get('status') != 'uncertified', req.name + ': support certifiable refuse')
        require(canonical(guard) == canonical(plain), req.name + ': census garde different du census generique')
        checks += 1 + check_response(req, guard, bits)
        counts['certified'] += 1
        if guard.get('kind') == 'refused':
            counts['refused'] += 1
            continue
        for key in ('disjoint', 'partial', 'outside'):
            counts[key] += guard['guard'][key]
        counts['wide'] += guard['lanes']['wide']
        require(all(plain['guard'][key] == 0 for key in ('disjoint', 'partial', 'outside')),
                req.name + ': garde comptee dans le census generique')
    require(counts['certified'] >= 600 and counts['uncertified'] >= 100 and counts['refused'] == 6 and
            counts['disjoint'] > 0 and counts['partial'] > 0 and counts['outside'] > 0 and checks >= 15000,
            'plancher de strate non atteint : %s' % counts)
    print(json.dumps(dict(bits=bits, requests=len(queries), checks=checks, counts=counts,
                          input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage : guarded_oracle.py sonde', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired, KeyError, TypeError) as error:
        print('ECHEC census garde : ' + str(error), file=sys.stderr)
        sys.exit(1)
