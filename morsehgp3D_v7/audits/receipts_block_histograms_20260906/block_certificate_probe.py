#!/usr/bin/env python3
"""Exact bounded block-witness models; no product imports or engine execution.

The continuous certificate follows from H >= Hmin > 0 and Xi <= Xi_max.
Corner sufficiency uses separate convexity in each endpoint and concavity
of sqrt(t)*H - norm((b-a) cross (z-a)) in z. The finite midpoint grid below
checks implementations of these statements; it is not their general proof.
All grid coordinates are doubled integers, so midpoints remain exact.
"""
from __future__ import annotations

import itertools
import json
import sys

Point = tuple[int, int, int]
Box = tuple[Point, Point]
Interval = tuple[int, int]
LANES = (3, 4)
MAX_GRID_TRIPLES = 45000
CHECKS = 0


def require(ok: bool, reason: str) -> None:
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(reason)


def point_box(point: Point) -> Box:
    return point, point


def determinant3(a: Point, b: Point, c: Point) -> int:
    return (a[0] * (b[1] * c[2] - b[2] * c[1])
            - a[1] * (b[0] * c[2] - b[2] * c[0])
            + a[2] * (b[0] * c[1] - b[1] * c[0]))


def morton48(point: Point) -> int:
    return sum(((point[axis] >> bit) & 1) << (3 * bit + axis)
               for bit in range(16) for axis in range(3))


def coordinates(box: Box, doubled_grid: bool = False) -> tuple[Point, ...]:
    axes = []
    for lo, hi in zip(*box):
        require(0 <= lo <= hi <= 65535, 'u16 ordered box')
        values = (2 * lo, lo + hi, 2 * hi) if doubled_grid else (lo, hi)
        axes.append(tuple(sorted(set(values))))
    return tuple(itertools.product(*axes))


def h_xi(a: Point, b: Point, z: Point) -> tuple[int, int]:
    d = tuple(b[i] - a[i] for i in range(3))
    w = tuple(z[i] - a[i] for i in range(3))
    h = sum(w[i] * (b[i] - z[i]) for i in range(3))
    cross = (d[1] * w[2] - d[2] * w[1],
             d[2] * w[0] - d[0] * w[2],
             d[0] * w[1] - d[1] * w[0])
    return h, sum(value * value for value in cross)


def inside(q: int, h: int, xi: int) -> bool:
    require(q in LANES, 'q3/q4 lane')
    return h > 0 and (6 - q) * h * h > xi


def interval_product(a: Interval, b: Interval) -> Interval:
    products = tuple(x * y for x in a for y in b)
    return min(products), max(products)


def interval_cross_bound(a: Box, b: Box, z: Box) -> int:
    # Shared a-dependencies are deliberately forgotten: enclosure, not equality.
    d = tuple((b[0][i] - a[1][i], b[1][i] - a[0][i]) for i in range(3))
    w = tuple((z[0][i] - a[1][i], z[1][i] - a[0][i]) for i in range(3))
    upper = 0
    for i, j in ((1, 2), (2, 0), (0, 1)):
        first = interval_product(d[i], w[j])
        second = interval_product(d[j], w[i])
        lo, hi = first[0] - second[1], first[1] - second[0]
        upper += max(abs(lo), abs(hi)) ** 2
    return upper


def h_min(a: Box, b: Box, z: Box) -> int:
    return sum(min((zz - aa) * (bb - zz)
                   for aa in (a[0][i], a[1][i])
                   for bb in (b[0][i], b[1][i])
                   for zz in (z[0][i], z[1][i])) for i in range(3))


def block_certificate(q: int, a: Box, b: Box, z: Box) -> bool:
    return inside(q, h_min(a, b, z), interval_cross_bound(a, b, z))


def all_corners(q: int, a: Box, b: Box, z: Box) -> bool:
    return all(inside(q, *h_xi(aa, bb, zz))
               for aa, bb, zz in itertools.product(
                   coordinates(a), coordinates(b), coordinates(z)))


def h_max4_minimax(a: Box, b: Box, z: Box) -> int:
    """min_ab max_z per axis: valid core rejection, not all-anchor rejection."""
    result = 0
    for i in range(3):
        values = []
        for aa, bb in itertools.product((a[0][i], a[1][i]),
                                       (b[0][i], b[1][i])):
            center2 = aa + bb
            zz2 = min(max(center2, 2 * z[0][i]), 2 * z[1][i])
            values.append((bb - aa) ** 2 - (zz2 - center2) ** 2)
        result += min(values)
    return result


def main() -> dict[str, object]:
    require(len(sys.argv) == 1, 'no arguments')
    positive = (((0, 0, 0), (1, 1, 0)), point_box((100, 0, 0)),
                ((4, 0, 1), (5, 1, 1)))
    require(h_min(*positive) == 286, 'positive exact Hmin')
    require(interval_cross_bound(*positive) == 21026, 'positive interval Xi bound')
    anchors, witnesses = ((0, 0, 0), (1, 1, 0)), ((4, 0, 1), (5, 1, 1))
    require(len(set((*anchors, *witnesses, (100, 0, 0)))) == 5,
            'distinct original factor and opposite point identities')
    # These vectors have determinant 100: positive model has affine dimension 3.
    require(h_xi((0, 0, 0), (1, 1, 0), (4, 0, 1))[1] == 18,
            'noncollinear factor directions')
    determinant = determinant3(anchors[1], witnesses[0], (100, 0, 0))
    require(determinant == 100, 'affine dimension three with opposite point')
    for q in LANES:
        require(block_certificate(q, *positive), 'positive block credits q3/q4')
        for a in anchors:
            require(sum(inside(q, *h_xi(a, (100, 0, 0), z))
                        for z in witnesses) == 2, 'two real witnesses per anchor')

    diagonal = (point_box((1, 1, 1)), point_box((20, 1, 1)), point_box((1, 1, 1)))
    negative = (point_box((0, 0, 0)), point_box((10, 0, 0)), point_box((11, 0, 0)))
    boundary3 = (point_box((0, 1, 1)), point_box((2, 0, 0)), point_box((1, 1, 0)))
    boundary4 = (point_box((0, 0, 0)), point_box((2, 2, 2)), point_box((1, 1, 0)))
    for q in LANES:
        require(not block_certificate(q, *diagonal), 'diagonal has H=Xi=0')
        require(not block_certificate(q, *negative), 'positive square cannot replace H>0')
    for q, boxes in ((3, boundary3), (4, boundary4)):
        hh, xi = h_xi(*(box[0] for box in boxes))
        require(hh > 0 and (6 - q) * hh * hh == xi, 'nontrivial exact lane boundary')
        require(not block_certificate(q, *boxes), 'strict lane boundary rejected')
        require((6 - q) * hh * hh >= xi, 'nonstrict mutant would falsely credit boundary')

    # U has Morton keys 0 and 27; Z has key 64. They can be distinct subtrees.
    # Their original factor box is [0,4]x[0,3]x{0}; B=(100,100,0).
    # The ordinary s=8 separation test gives 196^2+197^2 >= 100*(4^2+3^2).
    rejection = (((0, 0, 0), (3, 3, 0)), point_box((100, 100, 0)),
                 point_box((4, 0, 0)))
    require(tuple(morton48(point) for point in
                  ((0, 0, 0), (3, 3, 0), (4, 0, 0))) == (0, 27, 64)
            and morton48((100, 100, 0)) > 64, 'disjoint contiguous Morton populations')
    require(196 ** 2 + 197 ** 2 >= 100 * (4 ** 2 + 3 ** 2),
            'hmax counterexample compatible with s8 factor separation')
    require(h_max4_minimax(*rejection) == -816, 'variable-anchor hmax negative')
    for q in LANES:
        require(inside(q, *h_xi((0, 0, 0), (100, 100, 0), (4, 0, 0))),
                'hmax mutant loses an actual credited anchor')
        require(not inside(q, *h_xi((3, 3, 0), (100, 100, 0), (4, 0, 0))),
                'other anchor actually fails')

    corpus = (
        ('endpoint_positive', positive),
        ('three_varying_boxes', (((0, 0, 0), (1, 1, 1)),
                                 ((100, 0, 0), (101, 1, 1)),
                                 ((4, 0, 0), (5, 1, 1)))),
        ('central_positive', (((0, 0, 0), (1, 1, 1)),
                              ((20, 20, 20), (21, 21, 21)),
                              ((10, 10, 10), (11, 11, 11)))),
        ('diagonal', diagonal), ('negative_H', negative),
        ('boundary_q3', boundary3), ('boundary_q4', boundary4),
        ('variable_anchor_rejection', rejection),
    )
    results = []
    total = 0
    for name, boxes in corpus:
        low, high = h_min(*boxes), interval_cross_bound(*boxes)
        interval = {q: block_certificate(q, *boxes) for q in LANES}
        corners = {q: all_corners(q, *boxes) for q in LANES}
        grids = tuple(coordinates(box, True) for box in boxes)
        size = len(grids[0]) * len(grids[1]) * len(grids[2])
        require(total + size <= MAX_GRID_TRIPLES, 'bounded midpoint corpus')
        for aa, bb, zz in itertools.product(*grids):
            hh, xi = h_xi(aa, bb, zz)
            require(hh >= 4 * low and xi <= 16 * high, 'scaled integer interval enclosure')
            for q in LANES:
                member = inside(q, hh, xi)
                require(not interval[q] or member, 'interval certificate implication')
                require(not corners[q] or member, 'corner certificate implication on exact grid')
        total += size
        for q in LANES:
            require(not interval[q] or corners[q], 'interval certificate includes every corner')
        results.append(dict(name=name, grid_triples=size, interval_accepts=interval,
                            corner_accepts=corners, Hmin=low, Xi_upper=high))
    require(total > 39000, 'nonvacant product-box grid')
    require(all(block_certificate(q, *corpus[1][1]) for q in LANES),
            'both lanes credit a fully varying three-box product')
    return dict(schema='mhgp7-auditor-block-histogram-certificate-v1', status='passed',
                exact_integer_midpoint_grid_triples=total, explicit_checks=CHECKS,
                positive_real_anchors=2, positive_real_witnesses_per_anchor=2,
                strict_boundary_mutants=2, H_sign_and_diagonal_controls=4,
                variable_anchor_hmax_counterexamples=2, corpus=results,
                limits=dict(product_imported=False, engine_invoked=False,
                            general_continuous_proof_replaced_by_grid=False,
                            histogram_product_implementation_qualified=False,
                            asymptotic_or_measured_gain_claimed=False,
                            gcp_used=False, public_status='not_claimed'))


if __name__ == '__main__':
    try:
        print(json.dumps(main(), sort_keys=True, indent=2, allow_nan=False))
    except (ValueError, TypeError, OverflowError) as error:
        print(json.dumps(dict(status='failed', reason=str(error), engine_invoked=False)))
        raise SystemExit(1)
