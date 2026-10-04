#!/usr/bin/env python3
"""Independent bounded exact model; no engine import, native call or cloud call."""
from fractions import Fraction as F
from itertools import combinations
from math import comb
import json

CHECKS = 0


def require(condition, label):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(label)


def norm2(p, c):
    return sum((a - b) ** 2 for a, b in zip(p, c))


def circle(part, points):
    if len(part) == 1:
        return points[part[0]], F(0)
    a, b = (points[i] for i in part[:2])
    if len(part) == 2:
        c = tuple((x + y) / 2 for x, y in zip(a, b))
        return c, norm2(a, c)
    d = points[part[2]]
    u, v = tuple(x - y for x, y in zip(b, a)), tuple(x - y for x, y in zip(d, a))
    determinant = u[0] * v[1] - u[1] * v[0]
    if determinant == 0:
        return None
    uu, vv = sum(x * x for x in u), sum(x * x for x in v)
    shift = ((uu * v[1] - vv * u[1]) / (2 * determinant),
             (u[0] * vv - v[0] * uu) / (2 * determinant))
    c = tuple(x + y for x, y in zip(a, shift))
    return c, norm2(a, c)


def meb(part, points):
    candidates = []
    for size in range(1, min(3, len(part)) + 1):
        for support in combinations(part, size):
            candidate = circle(support, points)
            if candidate is not None and all(norm2(points[i], candidate[0]) <= candidate[1] for i in part):
                candidates.append(candidate)
    require(bool(candidates), 'every finite fixture has a containing candidate')
    return min(candidates, key=lambda value: value[1])


def positive_support(part, center, points):
    if len(part) == 2:
        return circle(part, points)[0] == center
    a, b, d = (points[i] for i in part)
    u, v = tuple(x-y for x, y in zip(b, a)), tuple(x-y for x, y in zip(d, a))
    determinant = u[0] * v[1] - u[1] * v[0]
    if determinant == 0:
        return False
    w = tuple(x-y for x, y in zip(center, a))
    t = (w[0] * v[1] - w[1] * v[0]) / determinant
    s = (u[0] * w[1] - u[1] * w[0]) / determinant
    return min(t, s, 1-t-s) > 0


def components(vertices, edges):
    parent = {x: x for x in vertices}

    def find(x):
        while parent[x] != x:
            x = parent[x]
        return x

    for a, b in edges:
        parent[find(a)] = find(b)
    groups = {}
    for vertex in vertices:
        groups.setdefault(find(vertex), []).append(vertex)
    return list(groups.values())


def fixture(name, raw, center, radius, k):
    points = [tuple(F(x) for x in point) for point in raw]
    ids = tuple(range(len(points)))
    p = tuple(i for i in ids if norm2(points[i], center) < radius)
    u = tuple(i for i in ids if norm2(points[i], center) == radius)
    require(len(p) + len(u) == len(ids), name + ': all sites in the ball')
    pairs = list(combinations(ids, k))
    beta = {part: meb(part, points)[1] for part in pairs}
    strict = [part for part in pairs if beta[part] < radius]
    closed = [part for part in pairs if beta[part] <= radius]
    compressed = [tuple(sorted(p + a)) for a in combinations(u, k-len(p))]
    traces = [part for part in compressed if beta[part] < radius]
    require(len(compressed) == comb(len(u), k-len(p)), name + ': compressed binomial')
    require(set(compressed) <= set(closed), name + ': compressed subset of closed parts')
    cofaces = [part for part in combinations(ids, k+1) if meb(part, points)[1] <= radius]
    graph_edges = []
    johnson_edges = []
    before_edges = []
    for a, b in combinations(closed, 2):
        union = tuple(sorted(set(a) | set(b)))
        level = meb(union, points)[1]
        if level <= radius:
            graph_edges.append((a, b))
            if len(union) == k+1:
                johnson_edges.append((a, b))
        if a in strict and b in strict and level < radius:
            before_edges.append((a, b))
    before = components(strict, before_edges)
    after = components(closed, graph_edges)
    old_unions = 0
    for group in after:
        touched = sum(bool(set(group) & set(old)) for old in before)
        old_unions += max(0, touched-1)
    supports = [part for size in (2, 3) for part in combinations(u, size)
                if positive_support(part, center, points)
                and circle(part, points) == (center, radius)]
    per_support = []
    for support in supports:
        containing = [part for part in cofaces if set(support) <= set(part)]
        formula = comb(len(p)+len(u)-len(support), k+1-len(support))
        require(len(containing) == formula, name + ': cofaces containing a support')
        per_support.append({'support': support, 'cofaces_containing_support': len(containing)})
    return {
        'name': name, 'points': raw, 'k': k, 'center': center, 'beta': radius,
        'interior': p, 'shell': u, 'positive_minimal_supports': supports,
        'compressed_part_count': len(compressed), 'strict_trace_count': len(traces),
        'strict_all_part_count': len(strict), 'closed_part_count': len(closed),
        'new_all_part_count': len(closed)-len(strict),
        'new_compressed_part_count': len(compressed)-len(traces),
        'coface_count': len(cofaces), 'intersection_pairs': len(graph_edges),
        'Johnson_pairs': len(johnson_edges),
        'before_components': len(before), 'closed_components': len(after),
        'spanning_links_on_closed_vertices': len(closed)-len(after),
        'root_unions_of_previous_components': old_unions,
        'per_support_cofaces': per_support,
    }


triangle = fixture('acute_triangle_K2', [(0, 0), (2, 0), (1, 2)], (F(1), F(3, 4)), F(25, 16), 2)
line = fixture('line_three_sites_K2', [(0, 0), (1, 0), (2, 0)], (F(1), F(0)), F(1), 2)
expected = {
    'acute_triangle_K2': {
        'compressed_part_count': 3, 'strict_trace_count': 3, 'closed_part_count': 3,
        'new_all_part_count': 0, 'new_compressed_part_count': 0, 'coface_count': 1,
        'intersection_pairs': 3, 'Johnson_pairs': 3, 'before_components': 3,
        'closed_components': 1, 'spanning_links_on_closed_vertices': 2,
        'root_unions_of_previous_components': 2,
    },
    'line_three_sites_K2': {
        'compressed_part_count': 2, 'strict_trace_count': 2, 'closed_part_count': 3,
        'new_all_part_count': 1, 'new_compressed_part_count': 0, 'coface_count': 1,
        'intersection_pairs': 3, 'Johnson_pairs': 3, 'before_components': 2,
        'closed_components': 1, 'spanning_links_on_closed_vertices': 2,
        'root_unions_of_previous_components': 1,
    },
}
for result in (triangle, line):
    for field, value in expected[result['name']].items():
        require(result[field] == value, result['name'] + ':' + field)
require(triangle['positive_minimal_supports'] == [(0, 1, 2)], 'triangle unique positive q3')
require(line['positive_minimal_supports'] == [(0, 2)], 'line unique positive q2')
require(triangle['strict_all_part_count'] == 3, 'triangle all strict vertices')
require(line['strict_all_part_count'] == 2, 'line all strict vertices')

# An algebraic date/ownership example, not a claimed native FULL fixture.
event_levels = [F(3), F(5), F(8)]
birth, parent_birth = F(3), F(10)
snapshots = {}
for cut, count in [(F(4), 1), (F(6), 2), (F(8), 3)]:
    require(birth <= cut < parent_birth, 'cut lies in node lifetime')
    retained = [level for level in event_levels if level <= cut]
    require(len(retained) == count, 'closed dated support snapshot')
    snapshots[str(cut)] = retained
require(snapshots['4'] != event_levels, 'whole-lifetime supports do not describe every earlier cut')
levels, parents = [F(1), F(2), F(3), F(5)], [2, 2, 3, None]


def ancestor_closed(seed, cut):
    node = seed
    while parents[node] is not None and levels[parents[node]] <= cut:
        node = parents[node]
    return node


require(ancestor_closed(0, F(3)) == 2, 'first trace attaches after whole equal-level plateau')
require(ancestor_closed(1, F(3)) == 2, 'second trace same closed owner')
require(ancestor_closed(0, F(5)) == 3, 'parent equality uses closed cut')
require(ancestor_closed(0, F(5)-F(1, 100)) == 2, 'strictly earlier cut keeps child')


def serial(value):
    if isinstance(value, F):
        return {'numerator': value.numerator, 'denominator': value.denominator}
    if isinstance(value, dict):
        return {str(k): serial(v) for k, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [serial(v) for v in value]
    return value


output = {'schema': 'audit.support_incidence_model.v1', 'checks': CHECKS,
          'scope': 'two <=3-site exact rational fixtures; independent stdlib model, no native qualification',
          'fixtures': [triangle, line],
          'temporal_example': {'kind': 'algebraic_example_of_an_already_proposed_design_guard',
                               'birth': birth, 'parent_birth': parent_birth,
                               'whole_lifetime_event_levels': event_levels, 'closed_snapshots': snapshots},
          'verdict': 'conforme_aux_comptes_explicites'}
print(json.dumps(serial(output), indent=2, sort_keys=True))
