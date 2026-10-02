"""Faits fixes et sensibilite du juge MEB : aucun executable natif n'est lance."""
import copy
import json
import math
from fractions import Fraction as F

from fixtures import fixtures
from fraction_oracle import check_response, circumsphere, cloud_of, expected, local_meb, parse, requests, require


def model_answer(req, bits):
    result = dict(coord_bits=bits, mode=req.mode, threshold=req.threshold,
                  index_memory=dict(before=0, after=0, peak=0), query_memory=dict(before=0, after=0, peak=0))
    if req.refusal:
        return dict(result, status='resource_exhausted' if req.refusal == 'memory_budget' else 'invalid_input',
                    reason=req.refusal, sites=[], site_ids=[], meb=None, census=None)
    truth = expected(req.records, req.part)
    anchor = truth['sites'][truth['support'][0]]
    offsets = [v-a for v, a in zip(truth['center'], anchor)]
    denominator = math.lcm(*(v.denominator for v in offsets))
    numerator = [int(v*denominator) for v in offsets]
    radius = truth['radius']
    meb = dict(support=truth['support'], arity=len(truth['support']), anchor=anchor,
               N=[format(v, 'x') for v in numerator], D=format(denominator, 'x'),
               level=[format(radius.numerator, 'x'), format(radius.denominator, 'x')], ledger=dict(truth['ledger']))
    census = None
    if req.mode == 1:
        saturated = len(truth['inner']) >= req.threshold
        census = dict(kind='saturated' if saturated else 'complete', inner=truth['inner'][:req.threshold],
                      shell=[] if saturated else truth['shell'],
                      ledger=dict(nodes=2, bounds=2, point_tests=0, inside_blocks=0, outside_blocks=0, passes=2))
        result['query_memory']['peak'] = 4*(len(census['inner'])+len(census['shell']))
    return dict(result, status='ok', reason='none', sites=truth['sites'], site_ids=truth['site_ids'], meb=meb, census=census)


def fixed_facts(bits):
    qs, _ = requests(bits)
    by_name = {q.name: q for q in qs}
    facts = [('singleton', F(0), 1), ('demi_entier', F(3, 4), 2), ('ligne_descente', F(121, 4), 2),
             ('triangle_droit', F(8), 2), ('triangle_obtus', F(4), 2), ('triangle_aigu', F(169, 36), 3),
             ('carre_centre', F(8), 2), ('support_negatif', F(25), 3), ('support_local_global', F(25), 3),
             ('meme_boule_globale', F(25), 2), ('prefixe_obtus_q4', F(25), 4), ('poids_nul_q4', F(169, 36), 3),
             ('coquille_douze', F(25), 2)]
    maximum = (1 << bits)-1
    facts += [('tetra_extreme', F(3*maximum*maximum, 4), 4),
              ('triangle_extreme', F(2*maximum*maximum, 3), 3)]
    for name, radius, arity in facts:
        req = by_name[name+'_meb']
        truth = expected(req.records, req.part)
        require((truth['radius'], len(truth['support'])) == (radius, arity), 'fait fixe '+name)
    neg = by_name['support_negatif_meb']
    truth = expected(neg.records, neg.part)
    require(truth['support'] == [0, 2, 3], 'support negatif non canonique')
    first = circumsphere(tuple(tuple(p) for p in truth['sites'][:3]))
    require(first[2] == (F(-1), F(5, 4), F(3, 4)), 'poids negatifs exacts')
    local = by_name['support_local_global_meb']
    require(len(expected(local.records, local.part)['shell']) == 5, 'coquille globale hors partie')
    circle = by_name['coquille_douze_meb']
    require(len(expected(circle.records, circle.part)['shell']) == 12, 'coquille etendue')
    line = by_name['ligne_descente_meb']
    line_truth = expected(line.records, line.part)
    require(line_truth['inner'] == [1, 2] and line_truth['shell'] == [0, 3], 'saut strict ligne')
    fields = ('presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests', 'diameter_pairs')
    stopped = {'singleton': (1, 1, 1, 1, 0, 1, 0), 'cube_huit': (1, 1, 1, 1, 0, 8, 28),
               'ligne_douze_extreme': (1, 1, 1, 1, 0, 12, 66),
               'support_negatif': (4, 4, 2, 1, 0, 7, 6), 'prefixe_obtus_q4': (6, 6, 5, 1, 0, 11, 6)}
    for name, values in stopped.items():
        req = by_name[name+'_meb']
        ledger = expected(req.records, req.part)['ledger']
        require(tuple(ledger[key] for key in fields) == values, 'travail arrete fixe '+name)
    for name in ('cube_huit', 'carre_coface', 'triangle_aigu', 'tetra_extreme'):
        req = by_name[name+'_meb']; sites, _ = cloud_of(req.records)
        points = tuple(sites[i] for i in sorted(req.part)); result = local_meb(points)
        pair = result['diameter_support']
        candidate = circumsphere(tuple(points[i] for i in pair))
        contains = all(sum((F(x)-c)**2 for x, c in zip(p, candidate[0])) <= candidate[1] for p in points)
        require(contains is (len(result['support']) == 2), 'diametre classe q2 '+name)
        if contains:
            require(pair == result['support'], 'diametre departage lex canonique '+name)
    return len(facts)+9+len(stopped)


def run():
    total = 0
    for bits in (18, 21, 24):
        facts = fixed_facts(bits)
        qs, pairs = requests(bits)
        require(len(fixtures(bits)) == 31 and len(qs) == 350 and len(pairs) == 31, 'comptes matrice')
        total += sum(check_response(q, model_answer(q, bits), bits) for q in qs)
    qs, _ = requests(24)
    named = {q.name: q for q in qs}
    neg = named['support_negatif_meb']
    local = named['support_local_global_meb']
    full = named['coquille_douze_L1_K2']
    saturated = named['ligne_descente_L1_K2']
    mutations = []

    def changed(req, section, key, value):
        answer = copy.deepcopy(model_answer(req, 24))
        target = answer if section is None else answer[section]
        target[key] = value
        mutations.append((req, answer))

    changed(neg, 'meb', 'support', [0, 1, 2])
    changed(neg, 'meb', 'support', [1, 2, 3])
    changed(neg, 'meb', 'support', [3, 2, 0])
    changed(neg, 'meb', 'arity', 4)
    changed(neg, 'meb', 'anchor', [0, 0, 0])
    changed(neg, 'meb', 'N', ['0', '0', '0'])
    changed(neg, 'meb', 'D', '0')
    changed(neg, 'meb', 'level', ['0', '1'])
    changed(neg, 'meb', 'level', ['19', '0'])
    changed(neg, 'meb', 'N', [1, '0', '0'])
    # Le support global antipodal n'appartient pas a F : il ne remplace pas le certificat local.
    sites, _ = cloud_of(local.records)
    changed(local, 'meb', 'support', [sites.index((1, 2, 0)), sites.index((9, 8, 0))])
    changed(full, 'census', 'shell', model_answer(full, 24)['census']['shell'][:2])
    changed(full, 'census', 'inner', [])
    changed(saturated, 'census', 'kind', 'complete')
    changed(saturated, 'census', 'inner', [0, 3])
    changed(saturated, 'census', 'inner', [1, 1])
    changed(saturated, 'census', 'shell', [0])
    changed(full, None, 'query_memory', dict(before=0, after=4, peak=52))
    changed(full, None, 'query_memory', dict(before=0, after=0, peak=4))
    changed(neg, None, 'index_memory', dict(before=0, after=0, peak=1))
    refused = named['query_complete_budget_zero']
    changed(refused, None, 'meb', model_answer(neg, 24)['meb'])
    for key in ('presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests', 'diameter_pairs'):
        ledger = dict(model_answer(neg, 24)['meb']['ledger']); ledger[key] += 1
        changed(neg, 'meb', 'ledger', ledger)
    for req in (neg, named['cube_huit_meb']):
        sites, _ = cloud_of(req.records)
        history = local_meb(tuple(sites[i] for i in sorted(req.part)))
        for key in ('exhaustive_ledger', 'historical_prefix_ledger'):
            changed(req, 'meb', 'ledger', history[key])
    for req, answer in mutations:
        try:
            check_response(req, answer, 24)
        except ValueError:
            pass
        else:
            raise ValueError('corruption non tuee : '+req.name)
    for invalid in ('{"a":1,"a":2}', '{"a":NaN}', '[]'):
        try:
            parse(invalid)
        except ValueError:
            pass
        else:
            raise ValueError('JSON invalide accepte')
    print(json.dumps(dict(profiles=3, fixtures=31, requests_per_profile=350, fixed_facts_per_profile=facts,
                          checks=total, corruptions=len(mutations), malformed=3, native=0), sort_keys=True))


if __name__ == '__main__':
    run()
