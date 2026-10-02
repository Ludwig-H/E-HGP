"""Faits exacts graves et sensibilite du juge aux reponses corrompues, sans binaire produit."""
import copy
import json
import sys
from fractions import Fraction as F

from fixtures import fixtures, records
from fraction_model import all_balls, catalogue, circumsphere, expected, prepared, require
from fraction_oracle import requests
from judge import Request, check_response, parse


def unique(balls, center, level):
    selected = [ball for ball in balls if ball.center == center and ball.level == level]
    require(len(selected) == 1, 'boule gravee absente ou double')
    return selected[0]


def facts(bits):
    cases = {fixture.name: prepared(records(fixture.points))[0] for fixture in fixtures(bits)}
    count = 0
    pair = catalogue(cases['pair'], 1)
    require(len(pair) == 1 and pair[0].level == F(9, 4) and pair[0].qmin == 2, 'paire gravee')
    count += 1
    require(catalogue(cases['singleton'], 12) == (), 'aucune boule q1 positive')
    count += 1
    triangle = unique(all_balls(cases['right_triangle']), (F(1), F(1), F(0)), F(2))
    require(triangle.qmin == 2 and len(triangle.shell) == 3, 'triangle droit -> coquille q2')
    count += 1
    face = unique(all_balls(cases['zero_weight']), (F(2), F(5, 6), F(0)), F(169, 36))
    require(face.qmin == 3 and len(face.shell) == 4, 'poids nul -> q3, coquille entiere')
    count += 1
    tetra = unique(catalogue(cases['obtuse_prefix'], 3), (F(5),) * 3, F(25))
    require(tetra.qmin == 4 and tetra.support == (0, 1, 2, 3), 'tetra q4 a prefixe obtus')
    require(any(weight < 0 for weight in circumsphere(cases['obtuse_prefix'][:3])[2]), 'prefixe vraiment obtus')
    require(tetra not in catalogue(cases['obtuse_prefix'], 2), 'q4 refuse a K2')
    count += 3
    extended = unique(catalogue(cases['extended_q4'], 3), (F(5),) * 3, F(25))
    require(extended.p == 0 and extended.qmin == 4 and len(extended.shell) == 5 and extended.support == (0, 1, 3, 4),
            'coquille5 qmin4 : support canonique apres deux quadruplets non stricts')
    count += 1
    require(extended.presentations == ((0, 1, 3, 4), (0, 2, 3, 4)) and extended not in catalogue(cases['extended_q4'], 2),
            'les deux supports stricts de la coquille q4 et admission seulement des K3')
    count += 1
    cube = unique(catalogue(cases['cube'], 1), (F(2),) * 3, F(12))
    require(cube.qmin == 2 and len(cube.shell) == 8 and cube.support == (0, 7), 'cube support canonique minimal')
    count += 1
    sphere = unique(all_balls(cases['extended_q3']), (F(5),) * 3, F(25))
    require(sphere.qmin == 3 and len(sphere.shell) == 7 and any(len(s) == 4 for s in sphere.presentations),
            'coquille q3 avec presentation q4 strictement interieure')
    count += 1
    octa = unique(catalogue(cases['octa_center'], 2), (F(2),) * 3, F(4))
    require(octa.p == 1 and octa.qmin == 2 and octa not in catalogue(cases['octa_center'], 1),
            'admission au seuil p+q=K+1')
    count += 1
    a = ((1 << bits) - 1) // 2
    small, large = F(a * a), F(a * a) + F(1, 4 * (a * a + 1))
    levels = {ball.level for ball in catalogue(cases['close_levels'], 2)}
    require(small in levels and large in levels and small < large and float(small) == float(large),
            'deux niveaux exacts distincts confondus en binary64')
    count += 1
    batch, pairs, _lookup = requests(bits)
    require(len(batch) == 378 and len(pairs) == 43, 'lot natif grave')
    count += 1
    return count


def truthful_response(req, bits):
    answer = expected(req.records, req.kmax)
    answer.update({'coord_bits': bits, 'kmax': req.kmax, 'status': 'ok', 'reason': 'none',
                   'used_before': 0, 'used_after': 0, 'peak': 1})
    answer['ledger'] = dict.fromkeys(('nodes', 'leaves', 'filter_tests', 'dominance_tests', 'prefixes', 'judged', 'census_tests',
                                    'emitted', 'incidences', 'q4_candidates', 'q4_levels', 'max_leaf', 'max_depth'), 0)
    answer['ledger']['emitted'] = len(answer['balls'])
    answer['ledger']['incidences'] = sum(ball['p'] + ball['m'] for ball in answer['balls'])
    answer['ledger']['q4_levels'] = sum(ball['qmin'] == 4 for ball in answer['balls'])
    answer['ledger']['q4_candidates'] = answer['ledger']['q4_levels']
    return answer


def judge_mutants():
    families = {fixture.name: fixture for fixture in fixtures(18)}
    requests_by_name = {name: Request(name, records(fixture.points), 3) for name, fixture in families.items()}
    killed = 0

    def kill(name, change):
        nonlocal killed
        req = requests_by_name[name]
        answer = truthful_response(req, 18)
        check_response(req, answer, 18)
        change(answer)
        try:
            check_response(req, answer, 18)
        except (ValueError, KeyError, TypeError):
            killed += 1
        else:
            raise ValueError('mutation de reponse survivante : ' + name)

    kill('pair', lambda answer: answer['balls'].clear())
    kill('pair', lambda answer: answer['balls'].append(copy.deepcopy(answer['balls'][0])))
    kill('pair', lambda answer: answer['balls'][0].update(qmin=3))
    kill('pair', lambda answer: answer['balls'][0].update(p=1))
    kill('pair', lambda answer: answer['balls'][0].update(rank=0))
    kill('pair', lambda answer: answer['balls'][0].update(inner=[0]))
    kill('pair', lambda answer: answer['balls'][0]['shell'].pop())
    kill('pair', lambda answer: answer['balls'][0].update(level=['1', '1']))
    kill('pair', lambda answer: answer.update(used_after=1))
    kill('pair', lambda answer: answer['site_ids'][0].append(4))
    kill('cube', lambda answer: next(ball for ball in answer['balls'] if ball['m'] == 8).update(support=[1, 6]))
    kill('extended_q4', lambda answer: next(ball for ball in answer['balls'] if ball['qmin'] == 4 and ball['m'] == 5)
         .update(support=[0, 2, 3, 4]))
    kill('obtuse_prefix', lambda answer: answer['balls'].remove(next(ball for ball in answer['balls'] if ball['qmin'] == 4)))
    kill('close_levels', lambda answer: answer['levels'].reverse())
    kill('regular_tetra', lambda answer: answer['ledger'].update(q4_levels=0))
    kill('regular_tetra', lambda answer: answer['ledger'].update(q4_candidates=0))
    kill('pair', lambda answer: answer['ledger'].update(q4_levels=1))
    for malformed in ('{"x":1,"x":2}', '{"x":NaN}', '[1,2]'):
        try:
            parse(malformed)
        except ValueError:
            killed += 1
        else:
            raise ValueError('JSON invalide admis')
    require(killed == 20, 'plancher mutants du juge')
    return killed


def main():
    checks = sum(facts(bits) for bits in (18, 21, 24))
    killed = judge_mutants()
    require(checks == 42 and killed == 20, 'plancher des faits du modele')
    print(json.dumps({'facts': checks, 'judge_mutants_killed': killed, 'profiles': [18, 21, 24]}, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError) as error:
        print('ECHEC catalogue modele : ' + str(error), file=sys.stderr)
        sys.exit(1)
