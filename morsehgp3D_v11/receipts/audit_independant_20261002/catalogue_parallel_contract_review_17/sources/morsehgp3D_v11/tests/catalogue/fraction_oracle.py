"""Confronte un pilote catalogue natif par lots au modele Gram/Fraction, dans le profil effectivement compile."""
import hashlib
import json
import subprocess
import sys
from collections import Counter
from dataclasses import replace
from itertools import product

from fixtures import fixtures, records
from fraction_model import require
from judge import Request, canonical_answer, check_response, parse, rational


def requests(bits):
    families = fixtures(bits)
    out, pairs = [], []
    lookup = {}
    for fixture in families:
        points = records(fixture.points)
        for kmax in tuple(range(1, 11)) + (12,):
            lookup[(fixture.name, kmax)] = len(out)
            out.append(Request('%s/K%d' % (fixture.name, kmax), points, kmax))
        permuted = replace(out[lookup[(fixture.name, 3)]], name=fixture.name + '/permuted', records=points[::-1])
        pairs.append((lookup[(fixture.name, 3)], len(out)))
        out.append(permuted)
        if fixture.split:
            for kmax in (1, 2, 3):
                split = replace(out[lookup[(fixture.name, kmax)]], name='%s/split%d' % (fixture.name, kmax), leaf=kmax + 3)
                pairs.append((lookup[(fixture.name, kmax)], len(out)))
                out.append(split)
    extreme = next(fixture for fixture in families if fixture.name == 'maximum_face')
    for axis in (1, 2):
        points = records(tuple(tuple(point[(j + axis) % 3] for j in range(3)) for point in extreme.points))
        for kmax in (1, 2, 3):
            out.append(Request('maximum_face/axis%d/K%d' % (axis, kmax), points, kmax))
    ordinary = Request('ordinary', records(((0, 0, 0), (3, 0, 0))), 1)
    refusals = [
        replace(ordinary, name='bad_k_negative', kmax=-1, refusal='kmax_out_of_range'),
        replace(ordinary, name='bad_k_zero', kmax=0, refusal='kmax_out_of_range'),
        replace(ordinary, name='bad_k_high', kmax=13, refusal='kmax_out_of_range'),
        replace(ordinary, name='bad_leaf_zero', leaf=0, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_leaf_k', kmax=5, leaf=7, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_maxleaf_zero', max_leaf=0, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_maxleaf_high', max_leaf=1025, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_leaf_order', leaf=64, max_leaf=32, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_balllimit_zero', ball_limit=0, refusal='parameter_out_of_range'),
        replace(ordinary, name='bad_balllimit_high', ball_limit=2**32, refusal='parameter_out_of_range'),
        replace(ordinary, name='memory_zero', budget=0, refusal='memory_budget'),
        replace(ordinary, name='memory_one', budget=1, refusal='memory_budget'),
        replace(ordinary, name='empty', records=(), refusal='empty_input'),
        replace(ordinary, name='coordinate', records=((1 << bits, 0, 0, 2),), refusal='coordinate_out_of_domain'),
        replace(ordinary, name='duplicate_id', records=((0, 0, 0, 7), (1, 0, 0, 7)), refusal='duplicate_point_id'),
        replace(ordinary, name='duplicate_position', records=((0, 0, 0, 7), (0, 0, 0, 9)), refusal='multiplicity_unsupported'),
        replace(ordinary, name='node_budget', records=records(tuple(product((0, 4), repeat=3))), leaf=4,
                max_nodes=1, refusal='node_budget'),
        replace(ordinary, name='wide_leaf', records=records(tuple(product((0, 1), repeat=3))), leaf=4,
                max_leaf=4, refusal='wide_leaf'),
        replace(ordinary, name='balllimit_exclusive', ball_limit=1, refusal='index_overflow_u32'),
        replace(ordinary, name='balllimit_after_prefix', records=records(tuple(product((0, 4), repeat=3))),
                kmax=3, ball_limit=5, refusal='index_overflow_u32'),
    ]
    out.extend(refusals)
    out.append(replace(ordinary, name='after_refusals', ball_limit=2))
    require(len(out) == 378 and len(pairs) == 43 and len(refusals) == 20, 'planchers du lot modifies')
    return out, pairs, lookup


def ball_signature(ball):
    return (tuple(ball['support']), rational(ball['level']), ball['p'], ball['m'], ball['qmin'],
            tuple(ball['inner']), tuple(ball['shell']))


def metamorphic(answers, pairs, lookup):
    count = 0
    for a, b in pairs:
        require(canonical_answer(answers[a]) == canonical_answer(answers[b]), 'permutation/subdivision non deterministe')
        count += 1
    for family in (fixture.name for fixture in fixtures(18)):
        for low, high in ((1, 3), (3, 5), (5, 10)):
            small, large = answers[lookup[(family, low)]], answers[lookup[(family, high)]]
            restriction = [ball_signature(ball) for ball in large['balls'] if ball['p'] + ball['qmin'] <= low + 1]
            require(restriction == [ball_signature(ball) for ball in small['balls']], 'restriction CatK differente')
            count += 1
    return count


def run(probe, parallel=False):
    options = {'capture_output': True, 'text': True, 'encoding': 'utf-8', 'errors': 'backslashreplace'}
    profile = subprocess.run([probe, '--profile'], timeout=15, **options)
    require(profile.returncode == 0 and not profile.stderr, 'profil : pilote en echec')
    bits = parse(profile.stdout).get('coord_bits')
    require(type(bits) is int and bits in (18, 21, 24), 'profil numerique non declare')
    batch, pairs, lookup = requests(bits)
    payload = ''.join(req.encode() for req in batch)
    completed = subprocess.run([probe], input=payload, timeout=120, **options)
    require(completed.returncode == 0 and not completed.stderr, 'pilote en echec : ' + completed.stderr)
    lines = completed.stdout.splitlines()
    require(len(lines) == len(batch), 'un JSON par requete attendu')
    answers, checks, accepted, rejected = [], 0, 0, 0
    for req, line in zip(batch, lines):
        answer = parse(line)
        checks += check_response(req, answer, bits)
        accepted += not bool(req.refusal)
        rejected += bool(req.refusal)
        answers.append(answer)
    checks += metamorphic(answers, pairs, lookup)
    strata = Counter(stratum for fixture in fixtures(bits) for stratum in fixture.strata)
    require(checks >= 15000 and accepted == 358 and rejected == 20, 'plancher des verdicts non atteint')
    report = {'bits': bits, 'requests': len(batch), 'accepted': accepted, 'refused': rejected,
                      'checks': checks, 'metamorphic': len(pairs) + 84, 'strata': dict(sorted(strata.items())),
                      'input_sha256': hashlib.sha256(payload.encode()).hexdigest()}
    if parallel:
        for workers in (1, 2, 4, 8):
            completed = subprocess.run([probe, '--workers', str(workers)], input=payload, timeout=120, **options)
            require(completed.returncode == 0 and not completed.stderr, 'pilote parallele en echec : ' + completed.stderr)
            lines = completed.stdout.splitlines()
            require(len(lines) == len(batch), 'un JSON par requete parallele attendu')
            for req, line, reference in zip(batch, lines, answers):
                answer = parse(line)
                checks += check_response(req, answer, bits)
                require(type(answer.get('workers')) is int and answer['workers'] == workers, 'Pool W non observe')
                require(canonical_answer(answer) == canonical_answer(reference), 'octets canoniques differents de W0')
                require(answer['ledger'] == reference['ledger'], '17 compteurs differents de W0')
                checks += 3
        report.update(workers=[0, 1, 2, 4, 8], requests=5 * len(batch), accepted=5 * accepted, refused=5 * rejected,
                      checks=checks, paired=4 * len(batch))
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != '--parallel'):
        print('usage : fraction_oracle.py pilote [--parallel]', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1], len(sys.argv) == 3)
    except (ValueError, KeyError, TypeError, OSError, subprocess.TimeoutExpired) as error:
        print('ECHEC catalogue Fraction : ' + str(error), file=sys.stderr)
        sys.exit(1)
