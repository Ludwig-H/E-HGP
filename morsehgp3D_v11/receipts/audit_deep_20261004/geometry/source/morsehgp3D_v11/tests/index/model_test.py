"""Qualification legere du juge : faits analytiques, trois profils et corruptions de reponses."""
import copy
import json

from fixtures import fixtures
from fraction_model import population, require
from judge import check_response, parse
from requests import requests


def model_answer(req, bits):
    answer = dict(coord_bits=bits, threshold=req.threshold,
                  index_memory=dict(before=0, after=0, peak=0), query_memory=dict(before=0, after=0, peak=0))
    if req.refusal:
        return dict(answer, status='resource_exhausted' if req.refusal == 'memory_budget' else 'invalid_input',
                    reason=req.refusal, sites=[], site_ids=[], kind='refused', inner=[], shell=[], ledger={})
    truth = population(req.records, req.support)
    saturated = len(truth['inner']) >= req.threshold
    return dict(answer, status='ok', reason='none', kind='saturated' if saturated else 'complete',
                sites=truth['sites'], site_ids=truth['site_ids'], inner=truth['inner'][:req.threshold],
                shell=[] if saturated else truth['shell'], index_nodes=1, index_depth=1,
                ledger=dict(nodes=2, bounds=2, point_tests=0, inside_blocks=0, outside_blocks=0, passes=2))


def run():
    checks = 0
    for bits in (18, 21, 24):
        fixtures_by_name = {f.name: f for f in fixtures(bits)}
        for name, counts in [('point_contact', (0, 1)), ('seuil_trois', (3, 6)), ('coquille_douze', (1, 12)),
                             ('cube_fractionnaire', (0, 8)), ('triangle_rationnel', (20, 3)),
                             ('tetra_droit', (19, 8)), ('poids_nul', (21, 4)), ('bloc_interieur', (3, 0))]:
            f = fixtures_by_name[name]
            truth = population(f.records, f.support)
            require((len(truth['inner']), len(truth['shell'])) == counts, 'fait fixe ' + name)
        outer = fixtures_by_name['triangle_exterieur']
        require(population(outer.records, outer.support)['center'][1] < 0, 'centre exterieur au cube source')
        qs, pairs = requests(bits)
        require(len(qs) == 1010 and len(pairs) == 47, 'comptes de la matrice')
        checks += sum(check_response(q, model_answer(q, bits), bits) for q in qs)
    qs, _ = requests(24)
    full = next(q for q in qs if q.name == 'seuil_trois_L4_K4')
    saturated = next(q for q in qs if q.name == 'seuil_trois_L4_K2')
    no_shell = next(q for q in qs if q.name == 'point_absent_L4_K1')
    refusal = next(q for q in qs if q.name == 'budget_census_nul')
    corruptions = []
    def corrupt(req, key, value):
        answer = copy.deepcopy(model_answer(req, 24)); answer[key] = value
        corruptions.append((req, answer))
    truth = model_answer(full, 24)
    corrupt(full, 'inner', truth['inner'][:-1])
    corrupt(full, 'shell', truth['shell'][:-1])
    corrupt(full, 'inner', truth['inner'][::-1])
    corrupt(full, 'inner', truth['inner'] + truth['inner'][:1])
    corrupt(full, 'inner', truth['inner'] + [len(truth['sites'])])
    corrupt(full, 'sites', [[True, 0, 0]] + truth['sites'][1:])
    corrupt(full, 'kind', 'saturated')
    corrupt(saturated, 'kind', 'complete')
    corrupt(saturated, 'inner', truth['shell'][:2])
    corrupt(saturated, 'inner', truth['inner'][:1])
    corrupt(saturated, 'shell', truth['shell'][:1])
    corrupt(refusal, 'kind', 'complete')
    corrupt(refusal, 'inner', [0])
    corrupt(no_shell, 'shell', [0])
    corrupt(full, 'query_memory', dict(before=0, after=4, peak=4))
    corrupt(full, 'ledger', dict(nodes=1, bounds=1, point_tests=0, inside_blocks=0, outside_blocks=0, passes=1))
    for req, answer in corruptions:
        try:
            check_response(req, answer, 24)
        except ValueError:
            pass
        else:
            raise ValueError('corruption non tuee : ' + req.name)
    for invalid in ('{"a":1,"a":2}', '{"a":NaN}', '[]'):
        try:
            parse(invalid)
        except ValueError:
            pass
        else:
            raise ValueError('JSON malforme accepte')
    print(json.dumps(dict(profiles=3, requests_per_profile=1010, fixed_facts_per_profile=9,
                          checks=checks, corruptions=len(corruptions), malformed=3, native=0), sort_keys=True))


if __name__ == '__main__':
    run()
