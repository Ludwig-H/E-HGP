#!/usr/bin/env python3
"""Small exact, standalone falsification of component-cover catalogue lemma.

All sites lie on a line in R^3. Orthogonal projection onto that line stays
inside L_K(R), so its interval components and distances from input sites
are sufficient for this geometric test. No product/reference imports.
"""

import argparse
from fractions import Fraction as F
from hashlib import sha256
from itertools import combinations, product
import json
from pathlib import Path


def components(points, weights, k, radius):
    # Arrangement of the input closed balls intersected with the axis,
    # with independent depth tests at endpoints and open-cell midpoints.
    ends = sorted({x + sign * radius for x in points for sign in (-1, 1)})

    def active(t):
        return sum(w for x, w in zip(points, weights) if abs(x - t) <= radius) >= k

    cells = [(a, a) for a in ends if active(a)]
    cells += [(a, b) for a, b in zip(ends, ends[1:]) if active((a + b) / 2)]
    result = []
    for a, b in sorted(cells):
        if result and a <= result[-1][1]:
            result[-1] = (result[-1][0], max(b, result[-1][1]))
        else:
            result.append((a, b))
    return result


def critical_balls(points, weights, k, strong):
    # Every positive critical ball for collinear sites has two diametral
    # support sites. Site balls are the necessary radius-zero extension.
    out = [(x, F(0)) for x, w in zip(points, weights) if w >= k]
    for a, b in combinations(points, 2):
        c, r = (a + b) / 2, (b - a) / 2
        inside = sum(w for x, w in zip(points, weights) if abs(x - c) < r)
        shell = [(x, w) for x, w in zip(points, weights) if abs(x - c) == r]
        population = inside + sum(w for _, w in shell)
        weighted_shell = any(w > 1 for _, w in shell)
        admissible = inside + 2 <= k if strong else (
            inside <= k - 1 if weighted_shell else inside + 2 <= k + 1
        )
        if admissible and population >= k:
            out.append((c, r))
    return out


def direct_list(x, comps, radius):
    return [i for i, (a, b) in enumerate(comps) if max(a - x, x - b, F(0)) <= radius]


def catalogue_list(x, comps, radius, balls):
    return [i for i, (a, b) in enumerate(comps) if any(
        r <= radius and abs(x - c) <= r and a <= c <= b for c, r in balls
    )]


def check_scene(points, weights, counts):
    radii = sorted({F(0)} | {abs(a - b) / 2 for a, b in combinations(points, 2)})
    radii = sorted(set(radii + [(a + b) / 2 for a, b in zip(radii, radii[1:])] + [radii[-1] + 1]))
    for k in range(1, min(3, sum(weights)) + 1):
        strong = critical_balls(points, weights, k, True)
        ordinary = critical_balls(points, weights, k, False)
        for radius in radii:
            comps = components(points, weights, k, radius)
            counts["cuts"] += 1
            for x in points:
                expected = direct_list(x, comps, radius)
                for kind, balls in (("strong", strong), ("ordinary", ordinary)):
                    actual = catalogue_list(x, comps, radius, balls)
                    if expected != actual:
                        raise RuntimeError(("list mismatch", points, weights, k, radius, x, kind,
                                            comps, expected, actual, balls))
                counts["point_queries"] += 1
                counts["multi_component_queries"] += len(expected) > 1
                counts["strict_radius_witnesses"] += sum(
                    any(r < radius and abs(x - c) <= r and a <= c <= b for c, r in strong)
                    for a, b in (comps[i] for i in expected)
                )
    counts["scenes"] += 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        raise RuntimeError("receipt already exists; refuse overwrite")
    counts = dict(scenes=0, cuts=0, point_queries=0, multi_component_queries=0,
                  strict_radius_witnesses=0)
    for n in range(1, 7):
        for coords in combinations(range(7), n):
            check_scene(tuple(map(F, coords)), (1,) * n, counts)
    for coords in ((0,), (0, 2), (0, 2, 4), (0, 1, 4), (0, 3, 7, 8)):
        for weights in product((1, 2, 3), repeat=len(coords)):
            check_scene(tuple(map(F, coords)), weights, counts)

    # A single earliest ball, even retaining all global minimum ties,
    # loses the later right component for x=0 at R=2.
    points, weights, k, radius, x = tuple(map(F, (-2, 0, 4))), (1, 1, 1), 2, F(2), F(0)
    comps = components(points, weights, k, radius)
    balls = critical_balls(points, weights, k, True)
    covering = [(c, r) for c, r in balls if abs(x - c) <= r]
    first = min(r for _, r in covering)
    first_only = [(c, r) for c, r in covering if r == first]
    expected, mutated = direct_list(x, comps, radius), catalogue_list(x, comps, radius, first_only)
    if expected != [0, 1] or mutated != [0]:
        raise RuntimeError(("minimum-only mutant not killed", expected, mutated))

    # Positive catalogue alone fails at a site of multiplicity K.
    wp, ww, wk, wr = (F(0), F(10)), (2, 1), 2, F(1)
    wc = components(wp, ww, wk, wr)
    positive_only = [(c, r) for c, r in critical_balls(wp, ww, wk, False) if r > 0]
    if direct_list(F(0), wc, wr) != [0] or catalogue_list(F(0), wc, wr, positive_only) != []:
        raise RuntimeError("zero-ball mutant not killed")

    receipt = {
        "status": "PASS", "scope": "exact rational collinear component-coverage oracle, not product qualification",
        "source_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
        "counts": counts, "orders": [1, 2, 3],
        "minimum_only_counterexample": {"points": [-2, 0, 4], "K": 2, "R": 2, "x": 0,
                                         "expected_components": expected, "minimum_only_components": mutated},
        "weighted_zero_counterexample": {"points": [0, 10], "weights": [2, 1], "K": 2, "R": 1,
                                         "expected_components": [0], "positive_only_components": []},
        "engine_imports": False, "GCP_used": False,
    }
    args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, sort_keys=True))


if __name__ == "__main__":
    main()
