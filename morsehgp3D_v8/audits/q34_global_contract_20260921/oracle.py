#!/usr/bin/env python3
"""Independent Cartesian Fraction oracle for the global q3/q4 support stream.

expected(points, k, mask, counts) returns a sorted list of JSON-compatible records
{arity, support, coefficients, depth, shell}; coefficients are primitive decimal
strings in the order (quadratic, x, y, z, constant). mask uses q3=2 and q4=4.
All positive triangles are retained. Tetrahedra are grouped by (owner edge,
smallest acute incident-face ID of that tetrahedron, primitive ball key), retaining
the smallest partner ID. This is a presentation stream, not a ball catalogue.

Geometry and complete census are cached in memory by coordinates, arity and ball
key. counters count actual work; cache hits do not re-charge enumeration/census.
No product geometry, WSPD, cover, hull, clipping or sweep implementation is used.
The historical rational Cartesian solve/ball functions are explicitly reused;
the caller must pin oracle_gate.py and its imported fixtures.py as dependencies.
"""
from fractions import Fraction as F
from itertools import combinations
from math import gcd, lcm
from pathlib import Path
import importlib.util
import json
import random
import sys

BASE = Path(__file__).resolve().parent
REFERENCE = BASE.parent / 'q4_center_blocks_20260920'


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


# The historical oracle imports its sibling under the generic name "fixtures".
# Restore the caller's namespace after loading, including an existing module.
_saved_fixture = sys.modules.get('fixtures')
sys.modules['fixtures'] = _load('_global_q34_reference_fixtures', REFERENCE / 'fixtures.py')
try:
    _reference = _load('_global_q34_reference_oracle', REFERENCE / 'oracle_gate.py')
finally:
    if _saved_fixture is None:
        del sys.modules['fixtures']
    else:
        sys.modules['fixtures'] = _saved_fixture
solve = _reference.solve
ball4 = _reference.ball


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def bump(counts, key, value=1):
    counts[key] = counts.get(key, 0) + value


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def distance(a, b):
    d = sub(a, b)
    return dot(d, d)


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def key(center, radius):
    raw = [F(1)] + [-2*x for x in center] + [dot(center, center)-radius]
    scale = lcm(*(x.denominator for x in raw))
    integers = [int(x*scale) for x in raw]
    divisor = gcd(*integers)
    return tuple(x//divisor for x in integers)


def ball3(triangle):
    """Solve the affine-plane 2x2 Gram system; require all three weights > 0."""
    a, b, c = triangle
    u, v = sub(b, a), sub(c, a)
    uu, uv, vv = dot(u, u), dot(u, v), dot(v, v)
    determinant = uu*vv-uv*uv
    if determinant == 0:
        return None
    require(determinant > 0, 'A Gram determinant cannot be negative')
    beta = F(vv*(uu-uv), 2*determinant)
    gamma = F(uu*(vv-uv), 2*determinant)
    if min(1-beta-gamma, beta, gamma) <= 0:
        return None
    center = tuple(F(a[i])+beta*u[i]+gamma*v[i] for i in range(3))
    return center, distance(center, a)


def acute(a, b, x):
    ab, ax, bx = distance(a, b), distance(a, x), distance(b, x)
    return ab+ax > bx and ab+bx > ax and ax+bx > ab


def owner(points, ids):
    return min(combinations(ids, 2), key=lambda ij: (-distance(points[ij[0]], points[ij[1]]), ij))


def collinear(points, counts):
    """Exact rank <= 1 certificate, including the 260-site non-enumeration case."""
    bump(counts, 'collinearity_calls')
    if len(points) < 3:
        return True
    direction = sub(points[1], points[0])
    for p in points[2:]:
        bump(counts, 'collinearity_cross_tests')
        if cross(direction, sub(p, points[0])) != (0, 0, 0):
            return False
    return True


_SUPPORTS = {}
_CENSUS = {}
_CATALOGUES = {}


def _points(points):
    result = tuple(tuple(p) for p in points)
    require(all(len(p) == 3 and all(type(x) is int and 0 <= x <= 65535 for x in p)
                for p in result), 'Expected exact u16 Cartesian input')
    require(len(result) == len(set(result)), 'Distinct site coordinates are required')
    return result


def _supports(points, q, counts):
    identity = points, q
    if identity in _SUPPORTS:
        bump(counts, 'positive_support_cache_hits')
        return _SUPPORTS[identity]
    result = []
    if len(points) < q or collinear(points, counts):
        bump(counts, 'rank_certified_empty_arities')
        _SUPPORTS[identity] = result
        return result
    for ids in combinations(range(len(points)), q):
        bump(counts, 'triangles_enumerated' if q == 3 else 'tetrahedra_enumerated')
        value = (ball3 if q == 3 else ball4)([points[i] for i in ids])
        if value is None:
            continue
        center, radius = value
        require(all(distance(points[i], center) == radius for i in ids), 'Bad Cartesian sphere')
        result.append((ids, tuple(center), radius, key(center, radius)))
        bump(counts, 'positive_triangles' if q == 3 else 'positive_tetrahedra')
    _SUPPORTS[identity] = result
    return result


def _census(points, center, radius, coefficients, counts):
    identity = points, coefficients
    if identity in _CENSUS:
        bump(counts, 'census_cache_hits')
        return _CENSUS[identity]
    depth, shell = 0, []
    for i, p in enumerate(points):
        delta = distance(p, center)-radius
        depth += delta < 0
        if delta == 0:
            shell.append(i)
    bump(counts, 'global_census_balls')
    bump(counts, 'global_census_point_tests', len(points))
    result = depth, tuple(shell)
    _CENSUS[identity] = result
    return result


def _catalogue(points, q, counts):
    identity = points, q
    if identity in _CATALOGUES:
        bump(counts, 'catalogue_cache_hits')
        return _CATALOGUES[identity]
    chosen = {}
    for ids, center, radius, coefficients in _supports(points, q, counts):
        depth, shell = _census(points, center, radius, coefficients, counts)
        record = q, ids, coefficients, depth, shell
        if q == 3:
            chosen[ids] = (0, record)
            continue
        edge = owner(points, ids)
        others = tuple(i for i in ids if i not in edge)
        seeds = [i for i in others if acute(points[edge[0]], points[edge[1]], points[i])]
        require(seeds, 'A positive tetrahedron must have an acute face on its owner edge')
        seed = min(seeds)
        partner = next(i for i in others if i != seed)
        presentation = edge, seed, coefficients
        if presentation not in chosen or partner < chosen[presentation][0]:
            chosen[presentation] = partner, record
    result = sorted(value[1] for value in chosen.values())
    _CATALOGUES[identity] = result
    return result


def normalize(records):
    return sorted((int(r['arity']), tuple(r['support']), tuple(map(int, r['coefficients'])),
                   int(r['depth']), tuple(r['shell'])) for r in records)


def expected(points, k, mask, counts):
    require(type(k) is int and k > 0, 'K must be positive')
    require(type(mask) is int and mask > 0 and mask & ~6 == 0, 'Mask must contain q3=2 and/or q4=4')
    points = _points(points)
    records = []
    bump(counts, 'expected_calls')
    for q, bit in ((3, 2), (4, 4)):
        if not mask & bit:
            continue
        for arity, ids, coefficients, depth, shell in _catalogue(points, q, counts):
            if depth < k+2-q:
                records.append(dict(arity=arity, support=list(ids),
                                    coefficients=[str(x) for x in coefficients],
                                    depth=depth, shell=list(shell)))
    records.sort(key=lambda r: (r['arity'], r['support'], tuple(map(int, r['coefficients'])), r['depth'], r['shell']))
    bump(counts, 'expected_records', len(records))
    return records


def fixtures():
    yield 'singleton', [(17, 19, 23)]
    yield 'line7', [(100+2*i, 100+3*i, 100+5*i) for i in range(7)]
    yield 'triangle_contact_w3', [(30, 30, 30), (36, 36, 30), (36, 30, 36), (32, 34, 28)]
    regular = [(30, 30, 30), (36, 36, 30), (30, 36, 24), (36, 30, 24)]
    yield 'regular_contact_w4', regular+[(32, 32, 32)]
    yield 'regular_q4_cover4', regular+[(32, 32, 32), (32, 32, 22)]
    yield 'isolated_shallow_vertex', regular+[(30, 36, 30), (36, 30, 30), (30, 30, 24), (32, 32, 32)]
    yield 'cube8', [(x, y, z) for x in (0, 8) for y in (0, 8) for z in (0, 8)]
    yield 'u16_extreme', [(0, 0, 65535), (65535, 65535, 65535), (0, 65535, 0), (65535, 0, 0)]
    rng = random.Random(20260921)
    for trial in range(3):
        points = []
        while len(points) < 9:
            p = tuple(rng.randrange(41) for _ in range(3))
            if p not in points:
                points.append(p)
        if trial == 1:
            # Rationally exact integral similarity from quaternion (1,2,3,4).
            matrix = ((-20, 4, 22), (20, -10, 20), (10, 28, 4))
            points = [tuple(2000+sum(row[j]*p[j] for j in range(3)) for row in matrix) for p in points]
        if trial == 2:
            points = [tuple(1000+1500*x for x in p) for p in points]
        yield f'random_nonaxial_{trial}', points
    sphere = [(x+20, y+20, z+20) for x in range(-5, 6) for y in range(-5, 6)
              for z in range(-5, 6) if x*x+y*y+z*z == 25]
    require(len(sphere) == 30, 'The integer radius-five sphere must have 30 sites')
    yield 'shell30', sphere
    yield 'line260', [(100+2*i, 100+3*i, 100+5*i) for i in range(260)]


def weights(points, ids, center):
    a = points[ids[0]]
    directions = [sub(points[i], a) for i in ids[1:]]
    gram = [[dot(u, v) for v in directions] for u in directions]
    rhs = [dot(u, sub(center, a)) for u in directions]
    values = solve(gram, rhs)
    require(values is not None, 'Positive support must have full affine rank')
    result = [1-sum(values)]+values
    require(min(result) > 0 and sum(result) == 1, 'Strict barycentric positivity')
    return result


def witness(a, b, z, q, closed=False):
    h = dot(sub(z, a), sub(b, z))
    v = cross(sub(b, a), sub(z, a))
    lhs, rhs = (3 if q == 3 else 2)*h*h, dot(v, v)
    return h > 0 and (lhs >= rhs if closed else lhs > rhs)


def model():
    """Finite exact checks supplement proofs; no asymptotic or product claim."""
    counts = {}
    mutants = {}
    fixture_map = dict(fixtures())
    for name, values in fixture_map.items():
        if name in ('shell30', 'line260'):
            continue
        points = _points(values)
        for q in (3, 4):
            for ids, center, radius, coefficients in _supports(points, q, counts):
                bary = weights(points, ids, center)
                edge = owner(points, ids)
                a, b = (points[i] for i in edge)
                diameter = distance(a, b)
                variance = sum(bary[i]*bary[j]*distance(points[ids[i]], points[ids[j]])
                               for i in range(q) for j in range(q))/2
                require(variance == radius, 'Exact variance identity')
                require(radius <= F(q-1, 2*q)*diameter, 'Variance diameter bound')
                midpoint = tuple(F(x+y, 2) for x, y in zip(a, b))
                require(distance(center, midpoint) == radius-F(diameter, 4), 'Edge midpoint identity')
                bump(counts, 'variance_positive_supports')
                for site in ids:
                    require(distance(points[site], a) <= diameter and distance(points[site], b) <= diameter,
                            'Support vertex outside owner lens')
                    bump(counts, 'owner_lens_checks')
                for z in points:
                    delta = distance(z, center)-radius
                    shifted = tuple(2*z[i]-a[i]-b[i] for i in range(3))
                    if delta <= 0:
                        require(dot(shifted, shifted) <= (3 if q == 3 else 4)*diameter,
                                'Closed ball outside arity-specific radial cover')
                        bump(counts, 'closed_cover_checks')
                    if witness(a, b, z, q):
                        require(delta < 0, 'A strict universal witness is not interior')
                        bump(counts, 'strict_witness_implications')
                    bump(counts, 'support_site_checks')
    for q, name in ((3, 'triangle_contact_w3'), (4, 'regular_contact_w4')):
        points = fixture_map[name]
        center, radius = (ball3 if q == 3 else ball4)(points[:q])
        a, b, z = points[0], points[1], points[q]
        require(distance(z, center) == radius, 'Witness contact fixture must be on the shell')
        require(not witness(a, b, z, q) and witness(a, b, z, q, closed=True), 'Strict-to-closed mutant not killed')
        records = expected(points, q-1, 2 if q == 3 else 4, counts)
        matched = [r for r in records if r['support'] == list(range(q))]
        require(len(matched) == 1 and matched[0]['depth'] == 0 and len(matched[0]['shell']) == q+1,
                'Contact presentation or complete shell lost')
        mutants[f'non_strict_w{q}'] = dict(killed=True, fixture=name, strict_depth=0,
                                         false_depth=1, shell_size=q+1)
        if q == 4:
            require(witness(a, b, z, 3) and not witness(a, b, z, 4),
                    'Using the W3 coefficient in W4 must count this shell contact')
            mutants['q4_uses_w3_alpha'] = dict(killed=True, fixture=name, strict_power=0,
                                               h=12, xi=288, false_depth=1)
    points = fixture_map['regular_q4_cover4']
    center, radius = ball4(points[:4])
    a, b, z = points[0], points[1], points[-1]
    shifted = tuple(2*z[i]-a[i]-b[i] for i in range(3))
    radial, diameter = dot(shifted, shifted), distance(a, b)
    require(distance(z, center) == radius and 3*diameter < radial <= 4*diameter,
            'Using alpha3 on q4 must lose a shell site')
    mutants['q4_uses_q3_cover'] = dict(killed=True, fixture='regular_q4_cover4',
                                   radial_squared=radial, alpha3_limit=3*diameter, alpha4_limit=4*diameter)
    # These are counterexamples to dropping the theorem's hypotheses, not
    # candidates to admit into the strictly-positive, maximal-edge product lane.
    a, b, x, z = (30, 30, 30), (42, 30, 30), (36, 48, 30), (36, 27, 30)
    center, radius = ball3([a, b, x])
    require(center == (F(36), F(38), F(30)) and radius == 100, 'Nonmaximal-edge ball')
    require(distance(a, x) > distance(a, b) and witness(a, b, z, 3)
            and distance(z, center)-radius == 21, 'Dropping maximality must fail')
    mutants['drop_owner_maximality'] = dict(killed=True, strict_power=21, h=27, xi=1296)
    a, b, x, z = (30, 30, 30), (46, 30, 30), (38, 32, 30), (38, 34, 30)
    center, radius = (F(38), F(15), F(30)), F(289)
    require(all(distance(p, center) == radius for p in (a, b, x)) and ball3([a, b, x]) is None,
            'Nonpositive circumcircle must fail the positive oracle')
    require(distance(a, b) > max(distance(a, x), distance(b, x)) and witness(a, b, z, 3)
            and distance(z, center)-radius == 72, 'Dropping positivity must fail')
    mutants['drop_support_positivity'] = dict(killed=True, strict_power=72, h=48, xi=4096)
    # A single explicit positive tetrahedron suffices to check the 30-site shell;
    # expected() in the executable comparison enumerates every support separately.
    sphere = fixture_map['shell30']
    chosen = [(17, 16, 20), (17, 24, 20), (23, 20, 16), (23, 20, 24)]
    require(all(p in sphere for p in chosen), 'Explicit shell tetrahedron sites missing')
    center, radius = ball4(chosen)
    require(center == [F(20)]*3 and radius == 25, 'Explicit shell30 ball')
    shell = [i for i, p in enumerate(sphere) if distance(p, center) == radius]
    require(len(shell) == 30, 'Collecting only support IDs must lose the large shell')
    mutants['shell_equals_support'] = dict(killed=True, fixture='shell30', full_shell=30, support_size=4)
    line_counts = {}
    require(expected(fixture_map['line260'], 10, 6, line_counts) == [], 'Collinear cloud has positive q3/q4 supports')
    require(line_counts.get('triangles_enumerated', 0) == 0 and line_counts.get('tetrahedra_enumerated', 0) == 0,
            'The line260 rank proof must avoid combination enumeration')
    require(line_counts['collinearity_cross_tests'] == 516, 'Both arity rank certificates must inspect all remaining sites')
    require(counts['variance_positive_supports'] > 0 and counts['strict_witness_implications'] > 0,
            'Geometric model must exercise positive supports and strict witnesses')
    return dict(status='passed', scope='finite independent rational geometry; not a product qualification',
                fixtures=len(fixture_map), counts=counts, line260=line_counts, mutants=mutants)


if __name__ == '__main__':
    require(len(sys.argv) == 1 or sys.argv[1:] == ['model'], 'Usage: oracle.py [model]')
    print(json.dumps(model(), sort_keys=True))
