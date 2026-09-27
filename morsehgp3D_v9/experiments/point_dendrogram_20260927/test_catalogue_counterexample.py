#!/usr/bin/env python3
"""Independent rational certificates: connectivity catalogues need not keep S.

Four planar points in R^3, K=2, z=2. No historical module, geometry backend,
native process, clustering, random perturbation, file IO or non-stdlib import.
The 'order-cell' catalogue is the ideal mathematical contract, not a claim
to reproduce HGP-old/Clusterer3D's floating, perturbed implementations.
"""
from fractions import Fraction as Q
from itertools import combinations
import json

POINTS = ((0, 1, 0), (4, 1, 0), (1, 2, 0), (1, 0, 0))
K = 2


def need(ok, message):
    if not ok:
        raise ValueError(message)


def dist2(a, b):
    return sum((Q(x)-Q(y))**2 for x, y in zip(a, b))


def certify(vertices, witness, center, beta, support, weights):
    """Upper inclusion + exact quadratic lower-bound identity for every y.

    Positive weights on support, sum 1, weighted center c, support radius² b
    imply sum_i w_i |p_i-y|² = b + |c-y|². Thus every enclosing ball has
    radius² >= b; containment of all vertices in B(c,b) attains that bound.
    """
    center, beta = tuple(map(Q, center)), Q(beta)
    weights = tuple(map(Q, weights))
    outside = set(range(len(POINTS)))-set(vertices)
    values = [dist2(witness, p) for p in POINTS]
    gap = min(values[i] for i in outside)-max(values[i] for i in vertices)
    need(gap > 0, 'strict order-3 Voronoi witness')
    need(set(support) <= set(vertices) and len(set(support)) == len(support), 'support belongs to coface')
    need(len(support) == len(weights) and min(weights) > 0 and sum(weights) == 1, 'positive convex weights')
    need(all(sum(w*POINTS[i][j] for i, w in zip(support, weights)) == center[j] for j in range(3)),
         'quadratic identity linear coefficient')
    need(all(dist2(center, POINTS[i]) == beta for i in support), 'support boundary')
    need(sum(w*dist2(POINTS[i], (0, 0, 0)) for i, w in zip(support, weights)) == beta+dist2(center, (0, 0, 0)),
         'quadratic identity constant coefficient')
    need(beta > 0 and all(dist2(center, POINTS[i]) <= beta for i in vertices), 'MEB upper-bound containment')
    interior = tuple(i for i, p in enumerate(POINTS) if dist2(center, p) < beta)
    shell = tuple(i for i, p in enumerate(POINTS) if dist2(center, p) == beta)
    # A positive-radius ball has no one-site center support. A two-site
    # boundary support contains its center iff the pair is antipodal.
    antipodal = [pair for pair in combinations(shell, 2)
                 if all(Q(POINTS[pair[0]][j]+POINTS[pair[1]][j], 2) == center[j] for j in range(3))]
    q_min = 2 if antipodal else len(support)
    need(q_min in (2, 3) and (q_min == 2 or len(shell) == len(support) == 3), 'exact tiny minimum support cardinality')
    gabriel = set(interior) <= set(vertices)
    return dict(vertices=vertices, witness=witness, witness_distances_squared=values,
        strict_order_cell_gap=gap, center=center, beta=beta, support=support, weights=weights,
        lower_bound_identity=True, upper_bound_attained=True, interior=interior, shell=shell,
        q_min=q_min, rank_min=len(interior)+q_min,
        full_catalogue_window_K2=len(interior)+q_min <= K+1, gabriel=gabriel)


def measure(cofaces):
    scores = {}
    for vertices, beta in cofaces.items():
        for face in combinations(vertices, K):
            scores[face] = scores.get(face, Q(0))+1/beta
    totals = [sum((score for face, score in scores.items() if point in face), Q(0))
              for point in range(len(POINTS))]
    need(totals == [K*sum((1/beta for face, beta in cofaces.items() if point in face), Q(0))
                    for point in range(len(POINTS))], 'complete-boundary T identity')
    masses = {face: score*sum((1/totals[x] for x in face), Q(0)) for face, score in scores.items()}
    need(sum(masses.values()) == len(POINTS) and all(0 < x <= 1 for x in masses.values()),
         'total point mass and facet mass bound')
    return dict(scores=scores, totals=totals, masses=masses)


def encoded(value):
    if isinstance(value, Q):
        return dict(num=str(value.numerator), den=str(value.denominator))
    if isinstance(value, dict):
        return {(','.join(map(str, key)) if isinstance(key, tuple) else str(key)): encoded(item)
                for key, item in value.items()}
    if isinstance(value, (tuple, list)):
        return [encoded(item) for item in value]
    return value


def main():
    certificates = [
        certify((0, 1, 2), (1, 11, 0), (2, 1, 0), Q(4), (0, 1), (Q(1, 2), Q(1, 2))),
        certify((0, 1, 3), (1, -9, 0), (2, 1, 0), Q(4), (0, 1), (Q(1, 2), Q(1, 2))),
        certify((0, 2, 3), (-1, 1, 0), (1, 1, 0), Q(1), (2, 3), (Q(1, 2), Q(1, 2))),
        certify((1, 2, 3), (5, 1, 0), (Q(7, 3), 1, 0), Q(25, 9),
                (1, 2, 3), (Q(4, 9), Q(5, 18), Q(5, 18))),
    ]
    need([row['vertices'] for row in certificates] == list(combinations(range(4), 3)), 'all four possible cofaces certified')
    need([row['strict_order_cell_gap'] for row in certificates] == [12, 12, 20, 8], 'strict witness margins')
    need([row['interior'] for row in certificates] == [(2, 3), (2, 3), (), ()], 'complete strict interiors')
    need([row['shell'] for row in certificates] == [(0, 1), (0, 1), (0, 2, 3), (1, 2, 3)], 'complete shells')
    need([row['q_min'] for row in certificates] == [2, 2, 2, 3], 'support minima')
    need([row['full_catalogue_window_K2'] for row in certificates] == [False, False, True, True], 'rank-window omission')
    order_cells = {row['vertices']: row['beta'] for row in certificates}
    gabriel = {row['vertices']: row['beta'] for row in certificates if row['gabriel']}
    need(set(gabriel) == {(0, 2, 3), (1, 2, 3)}, 'exact Gabriel subcatalogue')
    full, reduced = measure(order_cells), measure(gabriel)
    expected_full = {(0, 1): Q(1, 2), (0, 2): Q(5, 4), (0, 3): Q(5, 4),
                     (1, 2): Q(61, 100), (1, 3): Q(61, 100), (2, 3): Q(34, 25)}
    expected_reduced = {(0, 2): Q(1), (0, 3): Q(1), (1, 2): Q(9, 25),
                        (1, 3): Q(9, 25), (2, 3): Q(34, 25)}
    need(full['scores'] == expected_full and reduced['scores'] == expected_reduced, 'exact incidence scores')
    need(set(full['scores'])-set(reduced['scores']) == {(0, 1)}, 'facet universe changes')
    need(len({full['scores'][face]/score for face, score in reduced['scores'].items()}) > 1,
         'difference not one global positive normalization')
    need(full['totals'] == [Q(3), Q(43, 25), Q(161, 50), Q(161, 50)], 'order-cell T values')
    need(reduced['totals'] == [Q(2), Q(18, 25), Q(68, 25), Q(68, 25)], 'Gabriel T values')
    output = dict(schema='mhgp9_non_gabriel_incidence_counterexample_v1', status='passed',
        points=POINTS, k=K, exp_z=2, certificates=certificates,
        order_cell_cofaces=order_cells, gabriel_cofaces=gabriel,
        order_cell_measure=full, gabriel_measure=reduced, additional_facets=[(0, 1)],
        summary=dict(strict_order_cell_witnesses=4, exact_MEB_certificates=4,
            distinct_MEB_balls=3, balls_missing_from_rank_K2_window=1,
            order_cell_cofaces=4, gabriel_cofaces=2, order_cell_facets=6, gabriel_facets=5),
        scope='ideal_order3_cell_contract_not_old_floating_implementation_or_quality',
        geometry_backend_imported=False, native_executed=False, clustering_executed=False)
    print(json.dumps(encoded(output), sort_keys=True))


if __name__ == '__main__':
    main()
