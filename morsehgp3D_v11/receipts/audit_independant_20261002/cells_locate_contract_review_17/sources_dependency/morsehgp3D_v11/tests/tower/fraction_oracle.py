"""MEB par Gram/Gauss/Fraction : minimum de TOUTES les circonspheres englobantes, support positif ensuite."""
import hashlib
import itertools
import json
import re
import subprocess
import sys
from dataclasses import dataclass, replace
from fractions import Fraction as F
from functools import lru_cache

from fixtures import fixtures


def require(condition, message):
    if not condition:
        raise ValueError(message)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(matrix, rhs):
    rows = [[F(x) for x in row] + [F(y)] for row, y in zip(matrix, rhs)]
    for col in range(len(rows)):
        pivot = next((i for i in range(col, len(rows)) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [v/scale for v in rows[col]]
        for i in range(len(rows)):
            if i != col:
                scale = rows[i][col]
                rows[i] = [a-scale*b for a, b in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def circumsphere(points):
    anchor = points[0]
    edges = [sub(p, anchor) for p in points[1:]]
    weights = solve([[dot(a, b) for b in edges] for a in edges], [F(dot(a, a), 2) for a in edges])
    if weights is None:
        return None
    center = tuple(F(anchor[j])+sum(w*e[j] for w, e in zip(weights, edges)) for j in range(3))
    return center, dot(sub(center, anchor), sub(center, anchor)), (1-sum(weights),) + weights


def morton(point):
    return sum(((value >> bit) & 1) << (3*bit+axis)
               for axis, value in enumerate(point) for bit in range(value.bit_length()))


def cloud_of(records):
    groups = {}
    for x, y, z, point_id in records:
        groups.setdefault((x, y, z), []).append(point_id)
    sites = sorted(groups, key=morton)
    return sites, [sorted(groups[p]) for p in sites]


@lru_cache(maxsize=None)
def local_meb(points):
    """La minimisation geometrique n'utilise PAS les signes barycentriques du produit."""
    n = len(points)
    ledger = dict(presentations=0, nondegenerate=0, positive=0, containing=0, comparisons=0, point_tests=0)
    covering = []
    positive_covering = []
    for q in range(1, min(4, n)+1):
        for support in itertools.combinations(range(n), q):
            ledger['presentations'] += 1
            ball = circumsphere(tuple(points[i] for i in support))
            if ball is None:
                continue
            ledger['nondegenerate'] += 1
            center, radius, weights = ball
            powers = [dot(sub(p, center), sub(p, center))-radius for p in points]
            contains = all(v <= 0 for v in powers)
            if contains:
                covering.append((radius, center, support))
            if all(w > 0 for w in weights):
                ledger['positive'] += 1
                ledger['point_tests'] += next((i+1 for i, power in enumerate(powers) if power > 0), n)
                if contains:
                    ledger['containing'] += 1
                    positive_covering.append((radius, center, support))
    require(covering, 'aucune boule englobante dans le modele')
    radius = min(ball[0] for ball in covering)
    winners = [ball for ball in covering if ball[0] == radius]
    centers = {ball[1] for ball in winners}
    require(len(centers) == 1, 'unicite de la MEB')
    center = next(iter(centers))
    strict = [support for r, c, support in positive_covering if r == radius and c == center]
    require(strict, 'support local strict absent')
    support = min(strict, key=lambda s: (len(s), s))
    ledger['comparisons'] = ledger['containing']-1
    return dict(center=center, radius=radius, support=support, ledger=ledger)


@lru_cache(maxsize=None)
def expected(records, part):
    sites, ids = cloud_of(records)
    selected = sorted(part)
    result = local_meb(tuple(sites[i] for i in selected))
    center, radius = result['center'], result['radius']
    powers = [dot(sub(p, center), sub(p, center))-radius for p in sites]
    return dict(sites=[list(p) for p in sites], site_ids=ids, center=center, radius=radius,
                support=[selected[i] for i in result['support']], ledger=result['ledger'],
                inner=[i for i, value in enumerate(powers) if value < 0],
                shell=[i for i, value in enumerate(powers) if value == 0])


@dataclass(frozen=True)
class Request:
    name: str
    records: tuple
    part: tuple
    mode: int = 0
    leaf: int = 8
    threshold: int = 1
    index_budget: int = 1 << 20
    query_budget: int = 1 << 20
    refusal: str = ''

    def encode(self):
        header = (self.mode, self.leaf, self.threshold, self.index_budget, self.query_budget, len(self.records), len(self.part))
        return ' '.join(map(str, header))+'\n'+''.join(' '.join(map(str, p))+'\n' for p in self.records)+\
               ' '.join(map(str, self.part))+'\n'


def requests(bits):
    out, pairs = [], []
    for fixture in fixtures(bits):
        sites, _ = cloud_of(fixture.records)
        part = tuple(sites.index(p) for p in fixture.part_points)
        base = Request(fixture.name+'_meb', fixture.records, part, index_budget=0, query_budget=0)
        out.extend((base, replace(base, name=base.name+'_reverse', part=part[::-1], records=base.records[::-1])))
        pairs.append((len(out)-2, len(out)-1))
        count = len(expected(base.records, base.part)['inner'])
        for threshold in sorted({1, 2, 5, 10, max(1, count), count+1}):
            for leaf in (1, 8):
                out.append(replace(base, name='%s_L%d_K%d' % (fixture.name, leaf, threshold), mode=1,
                                   leaf=leaf, threshold=threshold, index_budget=1 << 20, query_budget=1 << 20))
    base = next(r for r in out if r.name == 'ligne_descente_meb')
    for mode in (0, 1):
        valid = replace(base, mode=mode, index_budget=1 << 20, query_budget=1 << 20)
        out += [replace(valid, name='empty_part_%d' % mode, part=(), refusal='empty_input'),
                replace(valid, name='large_part_%d' % mode, part=(0,)*13, refusal='parameter_out_of_range'),
                replace(valid, name='bad_site_%d' % mode, part=(4,), refusal='parameter_out_of_range'),
                replace(valid, name='none_site_%d' % mode, part=(2**32-1,), refusal='parameter_out_of_range'),
                replace(valid, name='repeat_site_%d' % mode, part=(0, 0), refusal='parameter_out_of_range')]
    out += [replace(base, name='empty_cloud', records=(), part=(0,), refusal='empty_input'),
            replace(base, name='coordinate_domain', records=((1 << bits, 0, 0, 7),), part=(0,), refusal='coordinate_out_of_domain'),
            replace(base, name='duplicate_point_id', records=((0, 0, 0, 7), (1, 0, 0, 7)), part=(0,), refusal='duplicate_point_id')]
    combined = replace(base, mode=1, index_budget=1 << 20, query_budget=1 << 20)
    out += [replace(combined, name='threshold_zero', threshold=0, refusal='parameter_out_of_range'),
            replace(combined, name='leaf_zero', leaf=0, refusal='parameter_out_of_range'),
            replace(combined, name='index_budget_zero', index_budget=0, refusal='memory_budget'),
            replace(combined, name='query_saturated_budget_zero', query_budget=0, refusal='memory_budget'),
            replace(combined, name='query_complete_budget_zero', threshold=3, query_budget=0, refusal='memory_budget')]
    return out, pairs


def parse(line):
    def object_of(pairs):
        result = {}
        for name, value in pairs:
            require(name not in result, 'cle JSON repetee '+name)
            result[name] = value
        return result

    def invalid(value):
        raise ValueError('constante JSON interdite '+value)

    result = json.loads(line, object_pairs_hook=object_of, parse_constant=invalid)
    require(type(result) is dict, 'objet JSON requis')
    return result


def integer_hex(word):
    require(type(word) is str and re.fullmatch(r'-?[0-9a-fA-F]+', word), 'entier hexadecimal requis')
    return int(word, 16)


def check_response(req, answer, bits):
    count = 0

    def check(condition, message):
        nonlocal count
        count += 1
        require(condition, req.name+': '+message)

    for key, value in (('coord_bits', bits), ('mode', req.mode), ('threshold', req.threshold)):
        check(type(answer.get(key)) is int and answer[key] == value, key)
    for stage, limit in (('index', req.index_budget), ('query', req.query_budget)):
        memory = answer.get(stage+'_memory')
        check(type(memory) is dict and all(type(memory.get(k)) is int for k in ('before', 'after', 'peak')), 'memoire '+stage)
        check(memory['before'] == memory['after'] == 0 and 0 <= memory['peak'] <= limit, 'restitution '+stage)
        if req.mode == 0:
            check(memory['peak'] == 0, 'MEB sans reservation '+stage)
    if req.refusal:
        check(answer.get('reason') == req.refusal, 'raison')
        check(answer.get('status') == ('resource_exhausted' if req.refusal == 'memory_budget' else 'invalid_input'), 'statut')
        check(answer.get('meb') is None and answer.get('census') is None, 'refus sans resultat partiel')
        return count
    check(answer.get('status') == 'ok' and answer.get('reason') == 'none', 'succes')
    truth = expected(req.records, req.part)
    for key in ('sites', 'site_ids'):
        value = answer.get(key)
        check(type(value) is list and all(type(row) is list and all(type(v) is int for v in row) for row in value), 'table '+key)
        check(value == truth[key], 'identite '+key)
    meb = answer.get('meb')
    check(type(meb) is dict, 'MEB requise')
    support = meb.get('support')
    check(type(support) is list and all(type(v) is int for v in support), 'support SiteIdx')
    check(support == truth['support'], 'support LOCAL strict minimal puis lexicographique')
    check(type(meb.get('arity')) is int and meb['arity'] == len(support), 'arite')
    anchor = meb.get('anchor')
    check(type(anchor) is list and all(type(v) is int for v in anchor) and anchor == truth['sites'][support[0]], 'ancre du support canonique local')
    check(type(meb.get('N')) is list and len(meb['N']) == 3, 'numerateur centre')
    numerator = [integer_hex(v) for v in meb['N']]
    denominator = integer_hex(meb.get('D'))
    check(denominator > 0, 'denominateur positif')
    check(tuple(F(n, denominator)+a for n, a in zip(numerator, anchor)) == truth['center'], 'centre Gram exact')
    level = meb.get('level')
    check(type(level) is list and len(level) == 2, 'paire niveau')
    num, den = map(integer_hex, level)
    check(num >= 0 and den > 0 and F(num, den) == truth['radius'], 'niveau MEB minimal exact')
    ledger = meb.get('ledger')
    check(type(ledger) is dict, 'compteurs MEB')
    for key, value in truth['ledger'].items():
        check(type(ledger.get(key)) is int and ledger[key] == value, 'compteur exact '+key)
    if req.mode == 0:
        check(answer.get('census') is None, 'MEB seule sans census implicite')
    else:
        census = answer.get('census')
        check(type(census) is dict, 'census requis')
        for key in ('inner', 'shell'):
            values = census.get(key)
            check(type(values) is list and all(type(v) is int for v in values), 'table census '+key)
            check(values == sorted(set(values)) and all(0 <= v < len(truth['sites']) for v in values), 'SiteIdx distincts croissants')
        saturated = len(truth['inner']) >= req.threshold
        check(census.get('kind') == ('saturated' if saturated else 'complete'), 'nature census')
        if saturated:
            check(len(census['inner']) == req.threshold and set(census['inner']) <= set(truth['inner']), 'temoins stricts globaux')
            check(census['shell'] == [], 'pas de coquille partielle')
        else:
            check(census['inner'] == truth['inner'], 'I global complet')
            check(census['shell'] == truth['shell'], 'U global complet sans plafond partie/K')
        ledger = census.get('ledger')
        check(type(ledger) is dict, 'compteurs census')
        for key in ('nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes'):
            check(type(ledger.get(key)) is int and 0 <= ledger[key] < 2**64, 'compteur census '+key)
        check(ledger['passes'] == 2 and ledger['point_tests'] <= 2*len(truth['sites']), 'parcours census compte')
        check(answer['query_memory']['peak'] == 4*(len(census['inner'])+len(census['shell'])), 'allocation exacte de la population')
    return count


def stable(answer):
    return {key: answer[key] for key in ('coord_bits', 'mode', 'threshold', 'status', 'reason', 'sites', 'site_ids', 'meb', 'census')}


def run(probe):
    result = subprocess.run([probe, '--profile'], capture_output=True, text=True, timeout=15)
    require(result.returncode == 0 and not result.stderr, 'metadata pilote')
    bits = parse(result.stdout).get('coord_bits')
    require(type(bits) is int and bits in (18, 21, 24), 'profil')
    queries, pairs = requests(bits)
    payload = ''.join(req.encode() for req in queries)
    result = subprocess.run([probe], input=payload, capture_output=True, text=True, timeout=90)
    require(result.returncode == 0 and not result.stderr, 'pilote en echec : '+result.stderr)
    lines = result.stdout.splitlines()
    require(len(lines) == len(queries), 'nombre de reponses')
    answers = [parse(line) for line in lines]
    checks = sum(check_response(req, answer, bits) for req, answer in zip(queries, answers))
    for a, b in pairs:
        require(stable(answers[a]) == stable(answers[b]), 'permutation change MEB/support/ledger')
        checks += 1
    modes = {str(mode): sum(req.mode == mode and not req.refusal for req in queries) for mode in (0, 1)}
    require(len(queries) >= 330 and len(pairs) == 31 and sum(bool(req.refusal) for req in queries) == 18 and
            checks >= 14000, 'plancher MEB')
    print(json.dumps(dict(bits=bits, fixtures=31, requests=len(queries), modes=modes, refusals=18,
                          permutations=len(pairs), checks=checks,
                          input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage : fraction_oracle.py pilote', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print('ECHEC tower Fraction : '+str(error), file=sys.stderr)
        sys.exit(1)
