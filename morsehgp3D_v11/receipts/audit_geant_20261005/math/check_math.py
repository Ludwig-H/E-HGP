#!/usr/bin/env python3
"""Bounded audit: full intersection nerve versus Johnson definition; exact point dates."""
import hashlib
import json
import os
from fractions import Fraction
from itertools import combinations, product
import sys

TREE = os.environ.get('MHGP11_AUDIT_TREE', '/workspaces/E-HGP/build/v11-audit-geant-math-20261005/morsehgp3D_v11')
sys.path[:0] = [os.path.join(TREE, 'reference'), os.path.join(TREE, 'bench')]
from hgp11_ref.definition import Definition
from hgp11_ref.judge import compare_cloud
from hgp11_ref.supports import Supports
import points_reference as pref
import points_radius as pr

FIXTURES = {
    'cube_minus_opposite': [p for p in product((0, 4), repeat=3) if p not in ((0, 0, 0), (4, 4, 4))],
    'cube_plus_boundary': list(product((0, 4), repeat=3))[:7] + [(2, 2, 0)],
    'octahedron_twisted': [(0, 4, 4), (8, 4, 4), (4, 0, 4), (4, 8, 4), (4, 4, 0), (5, 4, 8)],
    'interior_line_shell': [(0, 6, 6), (12, 6, 6), (6, 0, 6), (6, 12, 6), (6, 6, 0), (6, 6, 12), (5, 6, 6), (7, 6, 6)],
    'two_plateaus_tilt': [(0, 0, 0), (2, 2, 0), (2, 0, 2), (9, 0, 0), (11, 2, 0), (11, 0, 2), (6, 1, 1)],
    'near_coplanar_21': [(0, 0, 0), (2097151, 0, 0), (0, 2097151, 0), (2097151, 2097151, 1), (1048575, 1048576, 0), (1048576, 1048575, 1)],
    'near_collinear_21': [(0, 0, 0), (2097151, 1, 0), (1048575, 0, 1), (1048576, 1, 1), (1, 1, 1), (2097150, 0, 1)],
    'corner_mixed_21': [(0, 0, 0), (2097151, 2097151, 0), (2097151, 0, 2097151), (0, 2097151, 2097151), (1048575, 1048576, 1048575), (1048576, 1048575, 1048576)],
}
# u24 exercises Definition/Supports alone: constructive explicitly remains u21.
FIXTURES['near_coplanar_24'] = [(x * 8, y * 8, z) for x, y, z in FIXTURES['near_coplanar_21']]
FIXTURES['corner_mixed_24'] = [(x * 8, y * 8, z * 8) for x, y, z in FIXTURES['corner_mixed_21']]

counts = dict(clouds=0, orders=0, nerve_cuts=0, vertex_partitions=0, arbitrary_union_queries=0,
              supports_balls=0, supports=0, point_dates=0, point_owners=0, terminal_orders=0, vertical_faces=0)
per_cloud = []
for name, pts in FIXTURES.items():
    n = len(pts)
    D = Definition(pts)
    all_parts = [f for r in range(1, n + 1) for f in combinations(range(n), r)]
    beta = {f: D.beta(f) for f in all_parts}
    levels = sorted(set(beta.values()))
    S = Supports(pts)
    if max(max(p) for p in pts) < (1 << 21):
        errors, _truth, _ref = compare_cloud(pts, n)
        if errors:
            raise RuntimeError((name, errors))
    for k in range(1, n + 1):
        result = D.order(k)
        parts = list(combinations(range(n), k))
        joints = [(a, b, beta[tuple(sorted(set(parts[a]) | set(parts[b])))])
                  for a in range(len(parts)) for b in range(a + 1, len(parts))]
        counts['arbitrary_union_queries'] += len(joints)
        for j, level in enumerate(levels):
            for strict in (False, True):
                active = [i for i, f in enumerate(parts) if beta[f] < level or (not strict and beta[f] == level)]
                if strict and j == 0:
                    if active:
                        raise RuntimeError('strict lowest level not empty')
                    continue
                parent = {i: i for i in active}
                def root(i):
                    while parent[i] != i:
                        i = parent[i]
                    return i
                for a, b, threshold in joints:
                    if a in parent and b in parent and (threshold < level or (not strict and threshold == level)):
                        parent[root(b)] = root(a)
                nerve, johnson = {}, {}
                at = (levels[j - 1] + level) / 2 if strict else level
                for i in active:
                    nerve.setdefault(root(i), []).append(parts[i])
                    node = D.node_at(k, parts[i], at)
                    johnson.setdefault(node, []).append(parts[i])
                canon = lambda blocks: sorted(tuple(sorted(b)) for b in blocks.values())
                if canon(nerve) != canon(johnson):
                    raise RuntimeError((name, k, level, strict, canon(nerve), canon(johnson)))
                if k > 1:
                    low = D.order(k - 1)
                    low_parent = [-1] * len(low.nodes)
                    for v, node in enumerate(low.nodes):
                        for child in node.children:
                            low_parent[child] = v
                    for upper_node, vertex_group in johnson.items():
                        expected = set()
                        for vertex in vertex_group:
                            for face in combinations(vertex, k - 1):
                                expected.add(D.node_at(k - 1, face, at))
                                counts['vertical_faces'] += 1
                        image = result.lower[upper_node]
                        while low_parent[image] >= 0 and low.nodes[low_parent[image]].level <= at:
                            image = low_parent[image]
                        if expected != {image}:
                            raise RuntimeError((name, k, level, strict, 'vertical', expected, image))
                counts['nerve_cuts'] += 1
                counts['vertex_partitions'] += len(active)
        sp = S.order(k)
        counts['supports_balls'] += len(sp.balls)
        counts['supports'] += sum(len(b.supports) for b in sp.balls)
        # Exactly one terminal FULL component at K=n, supports remains defined.
        if k == n:
            if len(result.nodes) != 1 or sp.balls[0].role != 'naissance':
                raise RuntimeError((name, 'terminal order', len(result.nodes)))
            counts['terminal_orders'] += 1
        elif max(max(p) for p in pts) < (1 << 21):
            m = 1 if k == 1 else k + 1
            order = pref.order_from_definition(result, n, k)
            hanging = pr.hang_margin_radius(order, m)
            reference, _tree = pref.reference_radius_rules(result, n, m)
            for i, (_read, owner, (t, meet, start)) in enumerate(reference['margin_r']):
                if hanging.value(i).cmp(pr.RValue(t, meet, start)) != 0:
                    raise RuntimeError((name, k, i, 'point date'))
                if int(hanging.owner[i]) != owner:
                    raise RuntimeError((name, k, i, 'point owner'))
                counts['point_dates'] += 1
                counts['point_owners'] += 1
        counts['orders'] += 1
    counts['clouds'] += 1
    per_cloud.append(dict(name=name, n=n, levels=len(levels)))

# The forthcoming native points contract admits K=n, but qualification m=n+1 has no valid site.
terminal = Definition([(0, 0, 0), (2, 0, 0)]).order(2)
terminal_order = pref.order_from_definition(terminal, 2, 2)
try:
    pr.hang_margin_radius(terminal_order, 3)
except Exception as exc:
    terminal_refusal = dict(type=type(exc).__name__, reason=str(exc))
else:
    raise RuntimeError('K=n unexpectedly produces qualified hanging')

output = dict(pin='238734f1d03ab32e2a722bf036eb8fc5626dfd44', counts=counts,
              fixtures=FIXTURES, per_cloud=per_cloud, terminal_points=terminal_refusal,
              scope='Python bounded independent adjacency; no native execution, no timing qualification')
print(json.dumps(output, sort_keys=True, separators=(',', ':')))
