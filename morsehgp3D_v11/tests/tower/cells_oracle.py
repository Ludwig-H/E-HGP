"""T2 independant : faisabilite barycentrique fermee, jamais MEB(A) pour classer les traces.

Le catalogue est celui du juge Gram/Fraction exhaustif borne. Les compteurs MEB sont verifies
separement par le modele MEB existant ; ils ne decident ni separabilite ni exhaustivite des traces.
"""
import copy
import importlib.util
import itertools
import json
import math
import subprocess
import sys
from fractions import Fraction as F
from functools import lru_cache
from pathlib import Path

import fraction_oracle as meb


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


BASE = Path(__file__).resolve().parent
model = load('cells_catalogue_model', BASE.parent / 'catalogue/fraction_model.py')
fixtures = load('cells_catalogue_fixtures', BASE.parent / 'catalogue/fixtures.py')
NONE = 2**32 - 1
TRACE_BYTES = 52
FIELDS = ('presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests', 'diameter_pairs')


def require(ok, message):
    if not ok:
        raise ValueError(message)


@lru_cache(maxsize=2048)
def convex_witnesses(points, center):
    """Caratheodory : toute faisabilite a un temoin affinement independant de cardinal <=4."""
    witnesses = []
    for q in range(1, min(4, len(points)) + 1):
        for subset in itertools.combinations(range(len(points)), q):
            anchor = points[subset[0]]
            edges = [meb.sub(points[i], anchor) for i in subset[1:]]
            target = meb.sub(center, anchor)
            weights = model.solve([[meb.dot(a, b) for b in edges] for a in edges],
                                  [meb.dot(edge, target) for edge in edges])
            if weights is None or any(w < 0 for w in (1-sum(weights),) + weights):
                continue
            if any(sum(w*edge[j] for w, edge in zip(weights, edges)) != target[j] for j in range(3)):
                continue
            witnesses.append(frozenset(subset))
    return tuple(witnesses)


def cell_of(points, ball, order):
    t, m, q = order-ball.p, len(ball.shell), ball.qmin
    regular, analytical = m == q, m == q or t == m
    witnesses = convex_witnesses(tuple(points[i] for i in ball.shell), ball.center)
    ledger = dict(combinations=math.comb(m, t), passes=0 if analytical else 2,
                  trace_tests=0 if analytical else 2*math.comb(m, t), meb_calls=0,
                  meb=dict.fromkeys(FIELDS, 0))
    traces = []
    for subset in itertools.combinations(range(m), t):
        chosen = frozenset(subset)
        shell_ids = tuple(ball.shell[i] for i in subset)
        separable = not any(witness <= chosen for witness in witnesses)
        if separable:
            part = sorted(ball.inner + shell_ids)
            traces.append(dict(arity=order, sites=part + [NONE]*(12-order)))
        if not analytical and t >= q:
            ledger['meb_calls'] += 2
            counts = meb.local_meb(tuple(points[i] for i in shell_ids))['ledger']
            for name in FIELDS:
                ledger['meb'][name] += 2*counts[name]
    return dict(order=order, regular=regular, kind='strict_traces' if traces else 'birth',
                traces=traces, ledger=ledger)


def requests(bits):
    names = {'singleton', 'pair', 'line12', 'right_triangle', 'acute_triangle', 'regular_tetra',
             'zero_weight', 'obtuse_prefix', 'extended_q4', 'octa_center', 'cube', 'circle',
             'extended_q3', 'maximum_tetra', 'close_levels', 'random0'}
    rows = [(f.name, f.points) for f in fixtures.fixtures(bits) if f.name in names]
    rows += [('square', ((0,0,0), (4,0,0), (0,4,0), (4,4,0))),
             ('square_center', ((0,0,0), (4,0,0), (0,4,0), (4,4,0), (2,2,0))),
             ('line_capsule', ((0,0,0), (4,0,0), (5,0,0), (11,0,0)))]
    result = []
    for name, points in rows:
        records = fixtures.records(points)
        for k in (5, 12):
            result.append(dict(name=name+'_K'+str(k), records=records, kmax=k, budget=1 << 24))
        if name in ('square_center', 'extended_q4', 'maximum_tetra'):
            result.append(dict(name=name+'_reverse', records=records[::-1], kmax=12, budget=1 << 24))
    wide = fixtures.records(((0,5,5), (1,2,5), (1,8,5), (2,1,5), (2,9,5), (5,0,5), (5,10,5),
                             (8,1,5), (8,9,5), (9,2,5), (9,8,5), (10,5,5), (5,5,0), (5,5,10)))
    result.extend(dict(name='wide_shell'+suffix, records=records, kmax=1, budget=1 << 24)
                  for suffix, records in (('', wide), ('_reverse', wide[::-1])))
    result.append(dict(name='line13_K12', records=fixtures.records(tuple((i,0,0) for i in range(13))),
                       kmax=12, budget=1 << 24))
    pair = fixtures.records(((0,0,0), (2,0,0)))
    for name, records, k, budget in (
        ('no_memory', pair, 5, 0), ('empty', (), 5, 0), ('k_zero', pair, 0, 0),
        ('k_large', pair, 13, 0), ('duplicate_id', ((0,0,0,7), (1,0,0,7)), 5, 0),
        ('weight', ((0,0,0,7), (0,0,0,8)), 5, 0),
        ('coordinate', ((1 << bits,0,0,7),), 5, 0)):
        result.append(dict(name=name, records=records, kmax=k, budget=budget))
    return result


def empty_answer(req, bits, reason, peak=0):
    status = 'resource_exhausted' if reason == 'memory_budget' else (
        'unsupported_degeneracy' if reason == 'multiplicity_unsupported' else 'invalid_input')
    return dict(status=status, reason=reason, coord_bits=bits, kmax=req['kmax'], trace_bytes=TRACE_BYTES,
                cell_memory=dict(after=0, peak=peak), owner_after=0, sites=[], site_ids=[], balls=None)


@lru_cache(maxsize=128)
def geometry(records, k):
    points, ids = model.prepared(records)
    balls = model.catalogue(points, k)
    return points, ids, tuple(dict(support=list(ball.support), qmin=ball.qmin,
        level=[str(ball.level.numerator), str(ball.level.denominator)], inner=list(ball.inner), shell=list(ball.shell),
        cells=[cell_of(points, ball, order) for order in range(ball.p+ball.qmin-1, min(ball.p+len(ball.shell), k)+1)])
        for ball in balls)


def expected(req, bits):
    records, k = req['records'], req['kmax']
    if not records:
        return empty_answer(req, bits, 'empty_input')
    if any(any(v < 0 or v >= 1 << bits for v in row[:3]) for row in records):
        return empty_answer(req, bits, 'coordinate_out_of_domain')
    if len({row[3] for row in records}) != len(records):
        return empty_answer(req, bits, 'duplicate_point_id')
    if k < 1 or k > 12:
        return empty_answer(req, bits, 'kmax_out_of_range')
    if len({row[:3] for row in records}) != len(records):
        return empty_answer(req, bits, 'multiplicity_unsupported')
    points, ids, balls = geometry(tuple(records), k)
    peak = 0
    for ball in balls:
        for cell in ball['cells']:
            size = TRACE_BYTES*len(cell['traces'])
            if size > req['budget']:
                return empty_answer(req, bits, 'memory_budget', peak)
            peak = max(peak, size)
    return dict(status='ok', reason='none', coord_bits=bits, kmax=k, trace_bytes=TRACE_BYTES,
                cell_memory=dict(after=0, peak=peak), owner_after=0, sites=[list(p) for p in points],
                site_ids=[list(i) for i in ids], balls=list(balls))


def equal_exact(actual, wanted, where='root'):
    require(type(actual) is type(wanted), where+': type')
    if isinstance(wanted, dict):
        require(actual.keys() == wanted.keys(), where+': champs')
        return 1+sum(equal_exact(actual[k], value, where+'.'+k) for k, value in wanted.items())
    if isinstance(wanted, list):
        require(len(actual) == len(wanted), where+': taille')
        return 1+sum(equal_exact(a, b, where+'[]') for a, b in zip(actual, wanted))
    require(actual == wanted, where+': valeur')
    return 1


def parse(line):
    require(len(line) <= 32*1024*1024, 'ligne trop grande')
    def object_of(pairs):
        out = {}
        for key, value in pairs:
            require(key not in out, 'champ repete')
            out[key] = value
        return out
    return json.loads(line, object_pairs_hook=object_of,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('constante non finie')))


def judge(row, req, bits):
    row = copy.deepcopy(row)
    if isinstance(row.get('balls'), list):
        for ball in row['balls']:
            raw = ball['level']
            require(isinstance(raw, list) and len(raw) == 2 and all(type(x) is str for x in raw), 'niveau format')
            a, b = (int(x, 16) for x in raw)
            require(a > 0 and b > 0, 'niveau domaine')
            level = F(a, b)
            ball['level'] = [str(level.numerator), str(level.denominator)]
    return equal_exact(row, expected(req, bits))


def native_model(req, bits):
    result = copy.deepcopy(expected(req, bits))
    for ball in result['balls'] or ():
        ball['level'] = [format(int(v), 'x') for v in ball['level']]
    return result


def run(executable):
    profile = subprocess.run([executable, '--profile'], capture_output=True, text=True, check=True, timeout=10)
    bits = parse(profile.stdout)['coord_bits']
    require(type(bits) is int and bits in (18,21,24), 'profil')
    reqs = requests(bits)
    encoded = ''.join('%d %d %d\n' % (r['kmax'], r['budget'], len(r['records'])) +
                      ''.join(' '.join(map(str, row))+'\n' for row in r['records']) for r in reqs)
    process = subprocess.run([executable], input=encoded, capture_output=True, text=True, timeout=180)
    require(process.returncode == 0, 'code natif '+str(process.returncode)+': '+process.stderr[:2000])
    lines = process.stdout.splitlines()
    require(len(lines) == len(reqs), 'nombre reponses')
    checks = sum(judge(parse(line), request, bits) for line, request in zip(lines, reqs))
    cells = sum(len(ball['cells']) for req in reqs for ball in expected(req,bits)['balls'] or ())
    traces = sum(len(cell['traces']) for req in reqs for ball in expected(req,bits)['balls'] or () for cell in ball['cells'])
    require(len(reqs) >= 50 and cells >= 1300 and traces >= 2200 and checks >= 67000, 'planchers')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=len(reqs), cells=cells, traces=traces, checks=checks), sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 2, 'usage cells_oracle.py executable')
        run(sys.argv[1])
    except (ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
