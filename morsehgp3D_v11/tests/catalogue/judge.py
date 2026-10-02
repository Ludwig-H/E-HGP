"""Lecture stricte du protocole et juge catalogue, separables du lanceur pour les mutants de reponse."""
import json
import re
from dataclasses import dataclass
from fractions import Fraction as F

from fraction_model import expected, require


@dataclass(frozen=True)
class Request:
    name: str
    records: tuple
    kmax: int
    leaf: int = 32
    max_leaf: int = 256
    max_nodes: int = 0
    ball_limit: int = 2**32 - 1
    budget: int = 16 * 1024 * 1024
    refusal: str = ''

    def encode(self):
        header = (self.kmax, self.leaf, self.max_leaf, self.max_nodes, self.ball_limit, self.budget, len(self.records))
        return ' '.join(map(str, header)) + '\n' + ''.join(' '.join(map(str, point)) + '\n' for point in self.records)


REFUSAL_STATUS = {
    'kmax_out_of_range': 'invalid_input', 'parameter_out_of_range': 'invalid_input',
    'coordinate_out_of_domain': 'invalid_input', 'duplicate_point_id': 'invalid_input',
    'empty_input': 'invalid_input', 'multiplicity_unsupported': 'unsupported_degeneracy',
    'wide_leaf': 'unsupported_degeneracy', 'node_budget': 'resource_exhausted',
    'memory_budget': 'resource_exhausted', 'index_overflow_u32': 'resource_exhausted',
}


def parse(line):
    def object_of(pairs):
        out = {}
        for name, value in pairs:
            require(name not in out, 'JSON : cle repetee ' + name)
            out[name] = value
        return out

    def invalid_constant(value):
        raise ValueError('JSON : constante non finie ' + value)

    out = json.loads(line, object_pairs_hook=object_of, parse_constant=invalid_constant)
    require(type(out) is dict, 'JSON : objet attendu')
    return out


def rational(value):
    require(type(value) is list and len(value) == 2, 'rationnel : paire attendue')
    require(all(type(word) is str and re.fullmatch(r'-?[0-9]+', word) for word in value),
            'rationnel : chaines decimales entieres attendues')
    numerator, denominator = map(int, value)
    require(numerator >= 0 and denominator > 0, 'rationnel : domaine non negatif requis')
    return F(numerator, denominator)


class Checks:
    def __init__(self, name):
        self.name = name
        self.count = 0

    def check(self, condition, message):
        self.count += 1
        require(condition, '%s : %s' % (self.name, message))

    def equal(self, actual, wanted, message):
        self.check(actual == wanted, '%s : obtenu %r, attendu %r' % (message, actual, wanted))


def check_response(req, answer, bits):
    check = Checks(req.name)
    check.equal(answer.get('coord_bits'), bits, 'profil')
    check.equal(answer.get('kmax'), req.kmax, 'K')
    for key in ('coord_bits', 'kmax', 'used_before', 'used_after', 'peak'):
        check.check(type(answer.get(key)) is int, 'entier requis : ' + key)
    check.equal(answer['used_before'], 0, 'budget neuf')
    check.equal(answer['used_after'], 0, 'reservations liberees apres transaction')
    check.check(0 <= answer['peak'] <= req.budget, 'pic dans la limite declaree')
    if req.refusal:
        check.equal(answer.get('status'), REFUSAL_STATUS[req.refusal], 'statut de refus')
        check.equal(answer.get('reason'), req.refusal, 'raison de refus')
        for key in ('sites', 'site_ids', 'levels', 'balls'):
            check.equal(answer.get(key), [], 'aucun prefixe sur refus : ' + key)
        return check.count
    check.equal(answer.get('status'), 'ok', 'statut')
    check.equal(answer.get('reason'), 'none', 'raison du succes')
    truth = expected(req.records, req.kmax)
    for key in ('sites', 'site_ids'):
        check.check(type(answer.get(key)) is list, 'table attendue : ' + key)
        for row in answer[key]:
            check.check(type(row) is list and all(type(value) is int for value in row), 'ligne entiere : ' + key)
        check.equal(answer[key], truth[key], 'sites/identites : ' + key)
    check.check(type(answer.get('levels')) is list and type(answer.get('balls')) is list, 'tables de sortie')
    check.equal(list(map(rational, answer['levels'])), list(map(rational, truth['levels'])), 'niveaux exacts distincts')
    check.equal(len(answer['balls']), len(truth['balls']), 'nombre de boules COMPLET')
    for index, (actual, wanted) in enumerate(zip(answer['balls'], truth['balls'])):
        check.check(type(actual) is dict, 'boule objet')
        for key in ('qmin', 'p', 'm', 'rank'):
            check.check(type(actual.get(key)) is int, 'champ entier : ' + key)
            check.equal(actual[key], wanted[key], 'boule %d %s' % (index, key))
        for key in ('support', 'inner', 'shell'):
            check.check(type(actual.get(key)) is list and all(type(value) is int for value in actual[key]),
                        'liste d indices : ' + key)
            check.equal(actual[key], wanted[key], 'boule %d %s' % (index, key))
        check.equal(rational(actual.get('level')), rational(wanted['level']), 'boule %d niveau' % index)
    ledger = answer.get('ledger')
    check.check(type(ledger) is dict, 'compteurs logiques requis')
    for key in ('nodes', 'leaves', 'filter_tests', 'dominance_tests', 'prefixes', 'judged', 'census_tests', 'emitted',
                'incidences', 'q4_candidates', 'q4_levels',
                                    'region_pair_tests', 'region_pair_rejects', 'region_line_tests',
                                    'region_line_rejects', 'max_leaf', 'max_depth'):
        check.check(type(ledger.get(key)) is int and 0 <= ledger[key] < 2**64, 'compteur u64 : ' + key)
    check.equal(ledger['emitted'], len(truth['balls']), 'emissions logiques egales au catalogue')
    check.equal(ledger['incidences'], sum(ball['p'] + ball['m'] for ball in truth['balls']), 'incidences completes')
    check.equal(ledger['q4_levels'], sum(ball['qmin'] == 4 for ball in truth['balls']), 'niveaux q4 emis seulement')
    check.check(ledger['q4_candidates'] >= ledger['q4_levels'], 'candidats q4 couvrent emissions')
    check.check(ledger['max_leaf'] <= req.max_leaf and ledger['max_depth'] <= 3 * bits, 'bornes du parcours')
    return check.count


def canonical_answer(answer):
    """Champs stables, hors temps/pic/compteurs dependants de la subdivision."""
    return {key: answer[key] for key in ('status', 'reason', 'coord_bits', 'kmax', 'sites', 'site_ids', 'levels', 'balls')}
