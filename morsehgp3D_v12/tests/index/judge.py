"""Juge des deux certificats exclusifs : K interieurs distincts OU census global complet."""
import json
from dataclasses import dataclass

from fraction_model import morton, population, require


def radix_shape(sites, leaf):
    """Noeuds et profondeur de l'arbre radix de Morton (racine a profondeur 1), recalcules ici sans parcours natif.

    Les sites sont tries par cle et distincts ; chaque plage de plus de leaf sites est coupee au plus haut bit qui
    differe entre ses cles extremes. La recursion reste bornee par le nombre de bits des cles."""
    keys = sorted(set(morton(tuple(site)) for site in sites))

    def shape(begin, end):
        if end - begin <= leaf:
            return 1, 1
        top = 1 << ((keys[begin] ^ keys[end - 1]).bit_length() - 1)
        split = next(i for i in range(begin, end) if keys[i] & top)
        left, right = shape(begin, split), shape(split, end)
        return 1 + left[0] + right[0], 1 + max(left[1], right[1])
    return shape(0, len(keys))


@dataclass(frozen=True)
class Request:
    name: str
    records: tuple
    support: tuple
    threshold: int
    leaf: int = 4
    index_budget: int = 1 << 20
    query_budget: int = 1 << 20
    refusal: str = ''

    def encode(self):
        header = (self.leaf, self.threshold, self.index_budget, self.query_budget, len(self.records), len(self.support))
        return ' '.join(map(str, header)) + '\n' + ''.join(' '.join(map(str, p)) + '\n'
                                                        for p in self.records + self.support)


def parse(line):
    def pairs(values):
        out = {}
        for key, value in values:
            require(key not in out, 'cle JSON repetee : ' + key)
            out[key] = value
        return out

    def invalid(value):
        raise ValueError('constante JSON interdite : ' + value)

    result = json.loads(line, object_pairs_hook=pairs, parse_constant=invalid)
    require(type(result) is dict, 'objet JSON attendu')
    return result


def check_response(req, answer, bits):
    checks = 0

    def check(condition, message):
        nonlocal checks
        checks += 1
        require(condition, req.name + ': ' + message)

    check(answer.get('coord_bits') == bits and type(answer.get('coord_bits')) is int, 'profil')
    check(answer.get('threshold') == req.threshold and type(answer.get('threshold')) is int, 'seuil')
    for stage, limit in (('index', req.index_budget), ('query', req.query_budget)):
        memory = answer.get(stage + '_memory')
        check(type(memory) is dict, 'memoire ' + stage)
        check(all(type(memory.get(k)) is int for k in ('before', 'after', 'peak')), 'memoire entiere')
        check(memory['before'] == memory['after'] == 0, 'aucune reservation retenue apres reponse')
        check(0 <= memory['peak'] <= limit, 'pic dans budget')
    if req.refusal:
        check(answer.get('reason') == req.refusal, 'raison du refus')
        status = 'resource_exhausted' if req.refusal == 'memory_budget' else (
            'unsupported_degeneracy' if req.refusal == 'multiplicity_unsupported' else 'invalid_input')
        check(answer.get('status') == status, 'statut du refus')
        check(answer.get('kind') == 'refused', 'refus ne vaut aucun certificat')
        check(answer.get('inner') == answer.get('shell') == [], 'aucune population partielle')
        return checks
    check(answer.get('status') == 'ok' and answer.get('reason') == 'none', 'succes')
    truth = population(req.records, req.support)
    for key in ('sites', 'site_ids'):
        value = answer.get(key)
        check(type(value) is list and all(type(row) is list and all(type(v) is int for v in row) for row in value),
              'table entiere ' + key)
        check(value == truth[key], 'identite canonique ' + key)
    for key in ('inner', 'shell'):
        values = answer.get(key)
        check(type(values) is list and all(type(v) is int for v in values), 'table SiteIdx ' + key)
        check(values == sorted(set(values)), 'distincts et croissants ' + key)
        check(all(0 <= v < len(truth['sites']) for v in values), 'SiteIdx dans domaine ' + key)
    if len(truth['inner']) >= req.threshold:
        check(answer.get('kind') == 'saturated', 'K atteint, certificat saturant requis')
        check(len(answer['inner']) == req.threshold, 'exactement K temoins')
        check(set(answer['inner']) <= set(truth['inner']), 'temoins interieurs STRICTS globaux')
        check(answer['shell'] == [], 'aucune coquille partielle presentee comme complete')
    else:
        check(answer.get('kind') == 'complete', 'moins de K interieurs, census complet requis')
        check(answer['inner'] == truth['inner'], 'tous les interieurs')
        check(answer['shell'] == truth['shell'], 'toute la coquille, sans plafond K')
    ledger = answer.get('ledger')
    check(type(ledger) is dict, 'compteurs cumules requis')
    for field in ('nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes'):
        check(type(ledger.get(field)) is int and 0 <= ledger[field] < 2**64, 'compteur u64 : ' + field)
    check(ledger['passes'] == 2, 'deux passes reellement comptees')
    check(ledger['point_tests'] <= 2 * len(truth['sites']), 'aucun site reteste dans une passe')
    check(type(answer.get('index_nodes')) is int and 1 <= answer['index_nodes'] <= 2 * len(truth['sites']) - 1,
          'nombre de noeuds de l arbre binaire')
    expected_nodes, expected_depth = radix_shape(truth['sites'], req.leaf)
    check(answer['index_nodes'] == expected_nodes, 'noeuds de l arbre radix de Morton')
    check(type(answer.get('index_depth')) is int and answer['index_depth'] == expected_depth and
          expected_depth <= 3 * bits + 1, 'profondeur de l arbre radix, au plus 3B+1')
    return checks


def canonical(answer):
    return {key: answer[key] for key in ('coord_bits', 'threshold', 'status', 'reason', 'kind',
                                       'sites', 'site_ids', 'inner', 'shell')}
