"""Descente : minimum englobant exhaustif et composantes du graphe FERME Gamma_k.

Le juge ne choisit pas le terminal du produit. Toutes les circonspheres affines q<=4,
y compris celles non strictes, donnent beta par minimisation ; leurs populations
fermees engendrent exactement les hyperaretes admissibles. Aucun import de R2.
"""
import copy
import importlib.util
import itertools as it
import json
import math
from functools import lru_cache
from fractions import Fraction as F
from pathlib import Path
import subprocess
import sys

import fraction_oracle as arithmetic


def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).parent.parent/'catalogue'/filename)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


model = load('descent_catalogue_model', 'fraction_model.py')
fixtures = load('descent_catalogue_fixtures', 'fixtures.py')
require = arithmetic.require
NONE = (1 << 32)-1
MEB = ('presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests')
CENSUS = ('nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes')
COUNTS = ('steps', 'interior_steps', 'trace_steps', 'candidate_traces', 'trace_meb_calls',
          'census_calls', 'catalogue_hits')


def integer(value, maximum=(1 << 64)-1):
    require(type(value) is int and 0 <= value <= maximum, 'entier non signe')
    return value


def rational(raw):
    require(type(raw) is list and len(raw) == 2 and all(type(x) is str for x in raw), 'niveau format')
    a, b = map(arithmetic.integer_hex, raw)
    require(a >= 0 and b > 0, 'niveau domaine')
    return F(a, b)


def encoded(value):
    return [format(value.numerator, 'x'), format(value.denominator, 'x')]


def equal(a, b):
    require(type(a) is type(b), 'type exact')
    if isinstance(b, dict):
        require(a.keys() == b.keys(), 'champs exacts')
        return 1+sum(equal(a[k], v) for k, v in b.items())
    if isinstance(b, list):
        require(len(a) == len(b), 'taille exacte')
        return 1+sum(equal(x, y) for x, y in zip(a, b))
    require(a == b, 'valeur exacte')
    return 1


@lru_cache(maxsize=128)
def spheres(points):
    balls = {}
    for q in range(1, min(4, len(points))+1):
        for support in it.combinations(range(len(points)), q):
            sphere = model.circumsphere([points[i] for i in support])
            if sphere is None:
                continue
            center, level, _ = sphere
            distances = [model.distance2(p, center) for p in points]
            inner = tuple(i for i, d in enumerate(distances) if d < level)
            shell = tuple(i for i, d in enumerate(distances) if d == level)
            balls[center, level] = (level, center, inner, shell, frozenset(inner+shell))
    return tuple(sorted(balls.values()))


@lru_cache(maxsize=32768)
def meb(points, part):
    candidates = [s for s in spheres(points) if set(part) <= s[4]]
    require(candidates, 'MEB absente')
    least = candidates[0][0]
    winners = [s for s in candidates if s[0] == least]
    require(len(winners) == 1, 'unicite MEB')
    return winners[0]


def components(points, k, level, closed=True, direct=False):
    """Hyperaretes via populations ; direct=True est un second controle combinatoire."""
    parents = {}
    def root(v):
        parents.setdefault(v, v)
        while parents[v] != v:
            v = parents[v]
        return v
    def join(group):
        rows = list(group)
        if not rows:
            return
        a = root(rows[0])
        for b in rows[1:]:
            parents[root(b)] = a
    admissible = lambda value: value <= level if closed else value < level
    if direct:
        for part in it.combinations(range(len(points)), k):
            if admissible(meb(points, part)[0]):
                root(part)
        for edge in it.combinations(range(len(points)), k+1):
            if admissible(meb(points, edge)[0]):
                join(it.combinations(edge, k))
    else:
        populations = {s[4] for s in spheres(points) if admissible(s[0]) and len(s[4]) >= k}
        maximal = [p for p in populations if not any(p < q for q in populations)]
        for population in maximal:
            join(it.combinations(sorted(population), k))
    return {part: frozenset(p for p in parents if root(p) == root(part)) for part in parents}


def transitions(points, part):
    level, _, inner, shell, _ = meb(points, part)
    k = len(part)
    candidates = it.combinations(inner, k) if len(inner) >= k else (
        tuple(sorted(inner+a)) for a in it.combinations(shell, k-len(inner)))
    return tuple(p for p in candidates if meb(points, tuple(p))[0] < level)


@lru_cache(maxsize=128)
def geometry(records, kmax):
    points, ids = model.prepared(records)
    balls = model.catalogue(points, kmax)
    metadata = [dict(support=list(b.support), qmin=b.qmin, level=encoded(b.level),
                     inner=list(b.inner), shell=list(b.shell)) for b in balls]
    return points, ids, balls, metadata


def refusal(req, bits):
    records = req['records']
    if not records:
        return 'empty_input'
    if any(any(v < 0 or v >= 1 << bits for v in r[:3]) for r in records):
        return 'coordinate_out_of_domain'
    if len({r[3] for r in records}) != len(records):
        return 'duplicate_point_id'
    if not 1 <= req['kmax'] <= 12:
        return 'kmax_out_of_range'
    if len({r[:3] for r in records}) != len(records):
        return 'multiplicity_unsupported'
    k, part = req['order'], req['part']
    if not 1 <= k <= req['kmax']:
        return 'kmax_out_of_range'
    if len(part) != k or len(set(part)) != k or any(i >= len(records) for i in part):
        return 'parameter_out_of_range'
    return req.get('refusal')


def part_of(raw, k, n, ordered=True):
    require(type(raw) is list and len(raw) == k, 'cardinal partie')
    require(all(integer(i, n-1) == i for i in raw) and len(set(raw)) == k, 'sites partie')
    if ordered:
        require(raw == sorted(raw), 'ordre partie')
    return tuple(sorted(raw))


def ledger(raw):
    require(type(raw) is dict and raw.keys() == set(COUNTS+('part_meb', 'trace_meb', 'census')), 'ledger champs')
    flat = {k: integer(raw[k]) for k in COUNTS}
    for group, keys in (('part_meb', MEB), ('trace_meb', MEB), ('census', CENSUS)):
        require(type(raw[group]) is dict and raw[group].keys() == set(keys), 'sous ledger champs')
        flat.update((group+'.'+key, integer(raw[group][key])) for key in keys)
    for group in ('part_meb', 'trace_meb'):
        w = raw[group]
        require(w['containing'] <= w['positive'] <= w['nondegenerate'] <= w['presentations'], 'MEB sous comptes')
    return flat


def seed_for(points, balls, part, k):
    level, center, _, _, _ = meb(points, part)
    if level == 0:
        require(k == 1, 'naissance nulle')
        return dict(site=part[0], ball=None, order=k)
    found = [i for i, b in enumerate(balls) if b.center == center and b.level == level]
    require(len(found) == 1, 'naissance positive absente du catalogue')
    return dict(site=None, ball=found[0], order=k)


def judge(row, req, bits):
    require(type(row) is dict, 'reponse objet')
    require(row.keys() == {'status', 'reason', 'coord_bits', 'kmax', 'order', 'owner_after',
                           'query_memory', 'sites', 'site_ids', 'balls', 'steps', 'result'}, 'reponse champs')
    for key, value in (('coord_bits', bits), ('kmax', req['kmax']), ('order', req['order']), ('owner_after', 0)):
        equal(row[key], value)
    memory = row['query_memory']
    require(type(memory) is dict and memory.keys() == {'after', 'peak'}, 'memoire champs')
    equal(memory['after'], 0)
    integer(memory['peak'], req['budget'])
    reason = refusal(req, bits)
    if reason:
        status = ('resource_exhausted' if reason == 'memory_budget' else 'unsupported_degeneracy'
                  if reason == 'multiplicity_unsupported' else 'invalid_input')
        return sum(equal(row[key], value) for key, value in (
            ('status', status), ('reason', reason), ('steps', []), ('result', None),
            ('sites', []), ('site_ids', []), ('balls', None)))+5
    equal(row['status'], 'ok'); equal(row['reason'], 'none')
    points, ids, balls, metadata = geometry(tuple(req['records']), req['kmax'])
    checks = equal(row['sites'], [list(p) for p in points])+equal(row['site_ids'], [list(i) for i in ids])
    actual_balls = copy.deepcopy(row['balls'])
    for b in actual_balls:
        b['level'] = encoded(rational(b['level']))
    checks += equal(actual_balls, metadata)
    steps, k = row['steps'], req['order']
    require(type(steps) is list and 1 <= len(steps) <= math.comb(len(points), k), 'longueur chemin')
    current = tuple(sorted(req['part']))
    initial_level = meb(points, current)[0]
    component = components(points, k, initial_level)[current]
    summed, peak = None, 0
    for index, step in enumerate(steps):
        require(step.keys() == {'part', 'level', 'next', 'seed', 'ledger'}, 'pas champs')
        equal(list(part_of(step['part'], k, len(points), False)), list(current))
        value, _, inner, shell, _ = meb(points, current)
        require(rational(step['level']) == value, 'beta du pas')
        require(current in component, 'composante fermee initiale')
        flat = ledger(step['ledger']); w = step['ledger']
        equal(w['steps'], 1)
        require(w['census_calls']+w['catalogue_hits'] == 1, 'source population')
        require(w['part_meb']['containing'] >= 1, 'MEB partie non vacante')
        require(w['trace_meb_calls'] <= w['candidate_traces'], 'traces comptees')
        require(w['trace_meb']['containing'] >= w['trace_meb_calls'], 'MEB traces non vacantes')
        if w['catalogue_hits']:
            require(all(v == 0 for v in w['census'].values()), 'hit avec census')
            require(any(b.center == meb(points, current)[1] and b.level == value for b in balls), 'faux hit')
        else:
            require(w['census']['passes'] == 2 and w['census']['nodes'] > 0, 'census deux passes')
            peak = max(peak, 4*(k if len(inner) >= k else len(inner)+len(shell)))
        summed = flat if summed is None else {key: summed[key]+v for key, v in flat.items()}
        if index+1 == len(steps):
            equal(step['next'], [])
            require(not transitions(points, current), 'faux terminal : trace stricte existe')
            checks += equal(step['seed'], seed_for(points, balls, current, k))
            equal(w['interior_steps'], 0); equal(w['trace_steps'], 0)
        else:
            require(step['seed'] is None, 'naissance avant fin')
            nxt = part_of(step['next'], k, len(points))
            require(nxt in transitions(points, current), 'transition non stricte ou hors population')
            equal(w['interior_steps'], int(len(inner) >= k))
            equal(w['trace_steps'], int(len(inner) < k))
            require(w['candidate_traces'] >= w['trace_steps'], 'transition trace non vacante')
            current = nxt
        checks += 20+len(flat)
    result = row['result']
    require(result.keys() == {'initial_level', 'terminal_level', 'seed', 'ledger'}, 'resultat champs')
    require(rational(result['initial_level']) == initial_level, 'date initiale du terminal')
    require(rational(result['terminal_level']) == meb(points, current)[0], 'niveau terminal')
    checks += equal(result['seed'], steps[-1]['seed'])+equal(ledger(result['ledger']), summed)
    checks += equal(memory['peak'], peak)
    return checks+10


def requests(bits):
    rows = []
    def add(name, points, k, part=None, kmax=12, budget=1 << 24):
        records = fixtures.records(tuple(points))
        prepared, _ = model.prepared(records)
        chosen = list(range(k)) if part is None else [prepared.index(tuple(points[i])) for i in part]
        rows.append(dict(name=name, records=records, order=k, part=chosen, kmax=kmax, budget=budget))
    add('singleton', [(1,2,3)], 1)
    add('site_among_three', [(0,0,0),(2,0,0),(4,0,0)], 1, [1])
    add('pair_birth', [(0,0,0),(4,0,0)], 2)
    add('closed_line', [(0,0,0),(2,0,0),(4,0,0)], 2, [0,2], 2)
    line = [(0,0,0),(4,0,0),(5,0,0),(11,0,0)]
    add('interior_miss', line, 2, [0,3], 2)
    add('interior_hit', line, 2, [0,3], 12)
    add('no_query_memory', line, 2, [0,3], 2, 0); rows[-1]['refusal'] = 'memory_budget'
    triangle = [(0,0,0),(12,0,0),(6,9,0),(5,3,0),(7,3,0)]
    add('trace_outside_catalogue', triangle, 3, [0,1,2], 3)
    square = [(0,0,0),(4,0,0),(0,4,0),(4,4,0),(2,2,0)]
    add('extended_birth', square, 4, [0,1,2,3])
    add('extended_trace', square, 3, [0,3,4])
    add('local_q4_global_q2', list(it.product((0,4), repeat=3)), 4, [0,3,5,6])
    for fixture in fixtures.fixtures(bits):
        if fixture.name in ('regular_tetra', 'obtuse_prefix', 'extended_q4', 'cube', 'octa_center',
                             'maximum_tetra', 'close_levels', 'random0', 'random1'):
            for k in sorted({2, min(4, len(fixture.points)), len(fixture.points)}):
                add(fixture.name+'_k'+str(k), fixture.points, k)
            if fixture.name == 'random1':
                add('three_steps', fixture.points, 2)
                rows[-1]['part'] = [3,6]
    add('line12', [(i,0,0) for i in range(12)], 12)
    add('line13', [(i,0,0) for i in range(13)], 12, list(range(11))+[12])
    add('shell14_k12', [(0,5,5),(1,2,5),(1,8,5),(2,1,5),(2,9,5),(5,0,5),(5,10,5),
                        (8,1,5),(8,9,5),(9,2,5),(9,8,5),(10,5,5),(5,5,0),(5,5,10)], 12)
    for name in ('closed_line', 'extended_birth', 'maximum_tetra_k4'):
        original = next(r for r in rows if r['name'] == name)
        reverse = copy.deepcopy(original); reverse['name'] += '_reverse'
        reverse['records'] = reverse['records'][::-1]; reverse['part'].reverse(); rows.append(reverse)
    base = rows[2]
    for name, change in (
        ('empty', dict(records=())), ('coordinate', dict(records=((1 << bits,0,0,7),))),
        ('duplicate_id', dict(records=((0,0,0,7),(2,0,0,7)))),
        ('weight', dict(records=((0,0,0,7),(0,0,0,8)))),
        ('kmax_zero', dict(kmax=0)), ('kmax_large', dict(kmax=13)), ('order_zero', dict(order=0)),
        ('order_large', dict(order=13)), ('empty_part', dict(part=[])), ('short_part', dict(part=[0])),
        ('duplicate_site', dict(part=[0,0])), ('unknown_site', dict(part=[0,2])),
        ('sentinel_site', dict(part=[0,NONE])), ('large_part', dict(part=list(range(13))))):
        request = copy.deepcopy(base); request.update(change); request['name'] = name; rows.append(request)
    return rows


def parse(line):
    require(len(line) <= 32*1024*1024, 'reponse trop grande')
    return arithmetic.parse(line)


def run(executable):
    profile = subprocess.run([executable, '--profile'], capture_output=True, text=True, timeout=10)
    require(profile.returncode == 0 and not profile.stderr, 'profil processus')
    bits = parse(profile.stdout)['coord_bits']; integer(bits, 24); require(bits in (18,21,24), 'profil')
    reqs = requests(bits)
    encoded_requests = ''.join('%d %d %d %d %d\n' %
        (r['kmax'], r['order'], r['budget'], len(r['records']), len(r['part']))+
        ''.join(' '.join(map(str, p))+'\n' for p in r['records'])+' '.join(map(str, r['part']))+'\n' for r in reqs)
    process = subprocess.run([executable], input=encoded_requests, capture_output=True, text=True, timeout=180)
    require(process.returncode == 0 and not process.stderr, 'processus natif')
    lines = process.stdout.splitlines(); require(len(lines) == len(reqs), 'nombre reponses')
    rows = [parse(line) for line in lines]
    checks = sum(judge(row, req, bits) for row, req in zip(rows, reqs))
    steps = sum(len(row['steps']) for row in rows)
    require(len(reqs) >= 50 and steps >= 40 and checks >= 4000, 'planchers oracle')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=len(reqs), steps=steps,
                         refusals=sum(row['status'] != 'ok' for row in rows), checks=checks), sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 2, 'usage descent_oracle.py executable')
        run(sys.argv[1])
    except (ValueError, KeyError, TypeError, IndexError, subprocess.SubprocessError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
