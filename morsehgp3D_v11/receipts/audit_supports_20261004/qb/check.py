#!/usr/bin/env python3
"""Exact, bounded Q_b/critical-window model. Does not execute native source."""
from fractions import Fraction as F
from itertools import combinations, product
from pathlib import Path
import hashlib
import json
import random
import re

ROOT = Path(__file__).resolve().parent
CHECKS = 0

def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)

def barycentric(points, centre):
    """Unique barycentrics if affinely independent; None otherwise/outside span."""
    n = len(points)
    a = [[F(p[j]) for p in points] + [F(centre[j])] for j in range(3)]
    a.append([F(1)] * n + [F(1)])
    pivot = 0
    columns = []
    for col in range(n):
        found = next((r for r in range(pivot, 4) if a[r][col] != 0), None)
        if found is None:
            return None
        a[pivot], a[found] = a[found], a[pivot]
        scale = a[pivot][col]
        a[pivot] = [x / scale for x in a[pivot]]
        for r in range(4):
            if r != pivot:
                scale = a[r][col]
                a[r] = [a[r][j] - scale * a[pivot][j] for j in range(n + 1)]
        columns.append(pivot)
        pivot += 1
    if any(all(row[c] == 0 for c in range(n)) and row[n] != 0 for row in a):
        return None
    return tuple(a[row][n] for row in columns)

def q_family(shell, centre):
    result = {}
    for q in (2, 3, 4):
        result[q] = []
        for indices in combinations(range(len(shell)), q):
            points = tuple(shell[i] for i in indices)
            w = barycentric(points, centre)
            if w is None or not all(x > 0 for x in w):
                continue
            need(sum(w) == 1, 'barycentric sum')
            need(all(sum(w[i] * points[i][j] for i in range(q)) == centre[j] for j in range(3)), 'centre relation')
            for smaller in range(1, q):
                for sub in combinations(points, smaller):
                    u = barycentric(sub, centre)
                    need(u is None or not all(x >= 0 for x in u), 'not minimal by inclusion')
            result[q].append((indices, w))
    return result

def geometry_set(shell, family):
    return {q: sorted(tuple(sorted(shell[i] for i in indices)) for indices, _ in family[q]) for q in family}

def distance2(a, b):
    return sum((F(x) - F(y)) ** 2 for x, y in zip(a, b))

def critical(p, qmin, m, k):
    return p + qmin - 1 <= k <= p + m

def strong(p, qmin, m, k):
    return p + qmin <= k <= p + m

def main():
    source = (ROOT / 'sources/bench/points_export.cpp').read_text()
    need(re.search(r'bool strong\(const CatalogueBall& ball, u32 k\) noexcept\s*\{\s*return u64\{ball\.p\} \+ ball\.qmin <= k && k <= u64\{ball\.p\} \+ ball\.m;', source) is not None, 'source strong predicate changed')
    need('if (!strong(ball, k))' in source, 'source gating missing')
    need('if (k == 1) {' in source and 'offsets[s + 1] = 1' in source, 'K1 points-incidence special case changed')
    spec = (ROOT / 'sources/src/tower/cells.hpp').read_text()
    need('k dans [p+qmin-1,min(p+m,K)]' in spec, 'source critical window changed')
    canon = (ROOT / 'sources/src/catalogue/support.cpp').read_text()
    need(canon.index('qmin = 2;') < canon.index('triangle_support(cloud, shell, sphere)') < canon.index('tetra_support(cloud, shell, sphere)'), 'source canonical cardinality order changed')

    cube = tuple(product((0, 2), repeat=3))
    centre = (F(1), F(1), F(1))
    need(all(distance2(x, centre) == 3 for x in cube), 'cube shell is not cospherical')
    fam = q_family(cube, centre)
    need({q: len(fam[q]) for q in fam} == {2: 4, 3: 0, 4: 2}, 'cube complete family count')
    need(all(all(wi == F(1, q) for wi in w) for q in fam for _, w in fam[q]), 'cube weights')
    geometries = geometry_set(cube, fam)
    need(min(q for q in fam if fam[q]) == 2 and fam[4], 'mixed qmin/presentation arity')
    # Every q4 support is strictly positive and has no proper subset carrying the centre.
    # It is therefore inclusion-minimal even though the full shell has diameter supports.
    rng = random.Random(20261004)
    for iteration in range(16):
        permutation = list(range(8))
        rng.shuffle(permutation)
        order = tuple(cube[i] for i in permutation)
        other = q_family(order, centre)
        need(geometry_set(order, other) == geometries, f'permutation {iteration} changes Q_b geometry')
    translated_scaled = tuple(tuple(7 + 3*x for x in p) for p in cube)
    transformed = q_family(translated_scaled, (F(10), F(10), F(10)))
    need({q: len(transformed[q]) for q in transformed} == {2: 4, 3: 0, 4: 2}, 'similarity changes family')
    need(all(distance2(x, (10, 10, 10)) == 27 for x in translated_scaled), 'similarity level')

    triangle = ((0, 0, 0), (2, 2, 0), (2, 0, 2))
    triangle_c = (F(4, 3), F(2, 3), F(2, 3))
    need(all(distance2(a, b) == 8 for a, b in combinations(triangle, 2)), 'integer triangle not exactly equilateral')
    need(all(distance2(p, triangle_c) == F(8, 3) for p in triangle), 'triangle circumradius')
    tri_family = q_family(triangle, triangle_c)
    need({q: len(tri_family[q]) for q in tri_family} == {2: 0, 3: 1, 4: 0}, 'triangle qmin')
    for pair in combinations(triangle, 2):
        mid = tuple((F(pair[0][j]) + F(pair[1][j]))/2 for j in range(3))
        need(distance2(pair[0], mid) == 2 < F(8, 3), 'triangle pair not strict preplateau')
        third = next(p for p in triangle if p not in pair)
        need(distance2(third, mid) > 2, 'pair census not p0,m2')
    need(critical(0, 3, 3, 2) and not strong(0, 3, 3, 2), 'triangle lower-order event should not be strong')
    need(critical(0, 2, 2, 1) and not strong(0, 2, 2, 1), 'K1 edge event should not be strong')
    for q in (2, 3, 4):
        for p in range(5):
            for m in range(q, 9):
                active = [k for k in range(1, 13) if critical(p, q, m, k)]
                held = [k for k in range(1, 13) if strong(p, q, m, k)]
                need(set(held) < set(active), 'strict selector is not proper critical subset')
                need(set(active) - set(held) == {p + q - 1}, 'window mismatch should be exactly first weak order')

    result = {
        'status': 'PASS', 'checks': CHECKS,
        'native_executed': False,
        'source_pin': 'ee2b48b4c306f7da403657fb5291aa78569e6296',
        'cube': {'sites': 8, 'centre': ['1','1','1'], 'beta': '3', 'qmin': 2,
                 'Q_counts': {str(q):len(fam[q]) for q in fam},
                 'Q_by_coordinates': {str(q): geometries[q] for q in geometries},
                 'permutations_checked': 16},
        'critical_not_strong': {
            'integer_equilateral': {'points': triangle, 'pair_beta': '2', 'triangle_beta': '8/3',
                                    'p':0, 'm':3, 'qmin':3, 'order':2, 'strict_pair_traces':3,
                                    'critical':True, 'strong':False},
            'pair_at_order1': {'p':0,'m':2,'qmin':2,'critical':True,'strong':False}},
        'source_sha256': {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'sources').rglob('*')) if p.is_file()},
        'scope': 'Exact Fraction/barycentric scalar geometry and source-contract guards; no forest/native/export execution or performance claim.'
    }
    print(json.dumps(result, sort_keys=True, indent=2))

if __name__ == '__main__':
    main()
