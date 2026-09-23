#!/usr/bin/env python3
"""Exact integer fixture for a correlated bound on q3/q4 centre subcells."""

import json
from fractions import Fraction
from itertools import combinations, product

A = [(-500, -40, 0), (-500, 40, 0)]
B = [(500, -40, 0), (500, 40, 0)]
GUARDS = [(0, 501, 0), (1, 501, 0), (-1, 501, 0), (0, 501, 1)]
SUPPORTS = [(0, -500, -40), (0, -500, 40),
            (0, 500, -40), (0, 500, 40)]
POINTS = A + B + GUARDS + SUPPORTS
EDGES = list(product(A, B))
VERTICES_TWICE = list(product((-1, 1), repeat=3))


def check(condition, message):
    if not condition:
        raise ValueError(message)


def squared(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b, strict=True))


def squared_at_twice_vertex(point, vertex_twice):
    return sum((2 * x - w) ** 2 for x, w in zip(point, vertex_twice, strict=True))


def box_squared_at_twice_vertex(vertex_twice, lo, hi):
    return sum((w - min(max(w, 2 * a), 2 * b)) ** 2
               for w, a, b in zip(vertex_twice, lo, hi, strict=True))


def singleton_witness(edge, site):
    a, b = edge
    d = tuple(b[i] - a[i] for i in range(3))
    diameter = sum(x * x for x in d)
    y_twice = tuple(2 * site[i] - a[i] - b[i] for i in range(3))
    h4 = diameter - sum(x * x for x in y_twice)
    # Xi = |d x (site-midpoint)|^2; multiply by four for integral y_twice.
    xi4 = diameter * sum(x * x for x in y_twice) - sum(
        d[i] * y_twice[i] for i in range(3)) ** 2
    q3 = h4 > 0 and 3 * 4 * h4 * h4 > 16 * xi4
    q4 = h4 > 0 and 2 * 4 * h4 * h4 > 16 * xi4
    return q3, q4


def main():
    check(len(set(POINTS)) == 12, 'distinct points')
    check(len(EDGES) == 4, 'edge count')
    for edge in EDGES:
        witnesses = [singleton_witness(edge, p) for p in POINTS if p not in edge]
        check(sum(q3 for q3, _ in witnesses) == 0, 'q3 singleton witness')
        check(sum(q4 for _, q4 in witnesses) == 0, 'q4 singleton witness')

    # C = [-1/2, 1/2]^3. Twice-coordinate arithmetic keeps every test exact.
    minimum_margin_eighths = None
    for vertex in VERTICES_TWICE:
        pair_sum = min(squared_at_twice_vertex(a, vertex) +
                       squared_at_twice_vertex(b, vertex) for a, b in EDGES)
        for guard in GUARDS:
            margin = pair_sum - 2 * squared_at_twice_vertex(guard, vertex)
            check(margin > 0, 'correlated bound failed')
            minimum_margin_eighths = min(margin, minimum_margin_eighths or margin)

            # L1 from the two endpoint boxes and L2 from the midpoint box.
            da = box_squared_at_twice_vertex(vertex, (-500, -40, 0), (-500, 40, 0))
            db = box_squared_at_twice_vertex(vertex, (500, -40, 0), (500, 40, 0))
            dm = box_squared_at_twice_vertex(vertex, (0, -40, 0), (0, 40, 0))
            lower_bound_times_eight = max(da + db, 2 * dm + 2_000_000)
            check(2 * squared_at_twice_vertex(guard, vertex) >= lower_bound_times_eight,
                  'a box bound unexpectedly certified')
    check(minimum_margin_eighths == 448, 'minimum margin')  # 8 * 56.

    # A real positive q4 support at centre zero, owned by its unique longest edge.
    tetra = (A[1], B[1], SUPPORTS[0], SUPPORTS[1])
    check(all(squared(p, (0, 0, 0)) == 251_600 for p in tetra), 'support sphere')
    edge_lengths = sorted(squared(tetra[i], tetra[j]) for i, j in combinations(range(4), 2))
    check(edge_lengths == [6_400, 543_200, 543_200, 543_200, 543_200, 1_000_000],
          'unique longest edge')
    weights = (Fraction(25, 54), Fraction(25, 54), Fraction(1, 27), Fraction(1, 27))
    check(sum(weights) == 1 and all(w > 0 for w in weights), 'positive weights')
    check(all(sum(w * p[i] for w, p in zip(weights, tetra, strict=True)) == 0 for i in range(3)),
          'weighted centre')
    check(all(0 <= x + 500 < 2**18 for p in POINTS for x in p), 'u18 translation')
    print(json.dumps({'sites': len(POINTS), 'edges': len(EDGES),
                      's2_singleton_witnesses_q3_q4': [0, 0],
                      'subcell_vertices': len(VERTICES_TWICE),
                      'minimum_correlated_margin': [56, 1],
                      'box_bounds_certify_guards': False,
                      'positive_q4_unique_longest_edge': True}, sort_keys=True))


if __name__ == '__main__':
    main()
