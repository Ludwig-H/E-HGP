#!/usr/bin/env python3
"""Exact bounded model of collective witnesses; no producer or timing claim."""
from __future__ import annotations

import argparse
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def dot(a: tuple, b: tuple) -> F:
    return sum((F(x) * F(y) for x, y in zip(a, b)), F(0))


def subtract(a: tuple, b: tuple) -> tuple:
    return tuple(F(x) - F(y) for x, y in zip(a, b))


def power(point: tuple, center: tuple, radius2: F) -> F:
    delta = subtract(point, center)
    return dot(delta, delta) - radius2


def certificate(a: tuple, b: tuple, sites: list, weights: list,
                lam: F, *, allow_equal: bool = False,
                ignore_mean: bool = False) -> tuple[bool, F]:
    require(len(sites) == len(weights) and bool(sites), "empty/mismatched group")
    require(all(weight >= 0 for weight in weights) and sum(weights) == 1,
            "invalid convex weights")
    require(0 < lam < 1, "interior chord parameter required")
    mean = tuple(sum((weight * F(z[axis]) for weight, z in zip(weights, sites)), F(0))
                 for axis in range(3))
    chord = tuple((1 - lam) * F(a[axis]) + lam * F(b[axis]) for axis in range(3))
    gap = sum((weight * dot(z, z) for weight, z in zip(weights, sites)), F(0))
    gap -= (1 - lam) * dot(a, a) + lam * dot(b, b)
    return ((ignore_mean or mean == chord) and
            (gap <= 0 if allow_equal else gap < 0)), gap


def row_ball(distance: int, i: int, j: int, cy: F, cz: F) -> tuple:
    cx = F(distance * distance + j * j - i * i - 2 * cy * (j - i), 2 * distance)
    center = (cx, cy, cz)
    radius2 = dot(subtract((0, i, 0), center), subtract((0, i, 0), center))
    require(power((distance, j, 0), center, radius2) == 0, "endpoint not on sphere")
    return center, radius2


def run() -> dict:
    groups = spheres = inside = boundary_members = 0
    distance = 59000
    # u16 fixture, separation s12, exhaustive triples of ranks on small rows.
    for m in (4, 7, 11):
        require(distance >= 12 * (m - 1), "separation")
        for i in range(m):
            for j in range(i + 2, m):
                for k in range(i + 1, j):
                    s, t = k - i, j - k
                    a, b = (0, i, 0), (distance, j, 0)
                    sites = [(0, k, 0), (distance, k, 0)]
                    weights = [F(t, s + t), F(s, s + t)]
                    ok, gap = certificate(a, b, sites, weights, F(s, s + t))
                    require(ok and gap == -s * t, "collective row identity")
                    groups += 1
                    seen_outside = [False, False]
                    for cy in (F(-m), F(i + k, 2), F(i + j, 2), F(j + k, 2), F(2 * m)):
                        for cz in (F(0), F(1, 3)):
                            center, radius2 = row_ball(distance, i, j, cy, cz)
                            values = [power(z, center, radius2) for z in sites]
                            require(sum(w * p for w, p in zip(weights, values)) == gap,
                                    "independent sphere power disagrees")
                            require(min(values) < 0, "group misses sphere")
                            spheres += 1
                            inside += sum(value < 0 for value in values)
                            boundary_members += sum(value == 0 for value in values)
                            seen_outside = [old or value > 0 for old, value in zip(seen_outside, values)]
                    require(all(seen_outside), "individual nonuniversality not exercised")

    # Noncoplanar positive tetrahedral support; one group member is defining.
    a, b, c, d = (0, 10, 10), (10, 10, 10), (5, 16, 10), (5, 10, 16)
    center = (F(5), F(131, 12), F(131, 12))
    radius2 = dot(subtract(a, center), subtract(a, center))
    support = [a, b, c, d]
    bary = [F(25, 72), F(25, 72), F(11, 72), F(11, 72)]
    require(all(power(z, center, radius2) == 0 for z in support), "q4 support boundary")
    require(all(w > 0 for w in bary) and sum(bary) == 1, "q4 support positivity")
    require(all(sum(w * z[axis] for w, z in zip(bary, support)) == center[axis]
                for axis in range(3)), "q4 center not convex barycenter")
    # det(b-a,c-a,d-a)=10*6*6, hence affine rank three.
    require((b[0] - a[0]) * (c[1] - a[1]) * (d[2] - a[2]) == 360, "q4 rank")
    sites = [c, (5, 9, 11), (5, 9, 9)]
    weights = [F(1, 7), F(3, 7), F(3, 7)]
    ok, gap = certificate(a, b, sites, weights, F(1, 2))
    require(ok and gap == F(-127, 7), "q4 group relation")
    values = [power(z, center, radius2) for z in sites]
    require(values[0] == 0 and values[1] < 0 and values[2] < 0, "q4 defining member exclusion")
    require(sum(w * p for w, p in zip(weights, values)) == gap, "q4 identity")

    # A second positive tetrahedron has a unique longest edge a-b and a
    # collective pair whose two sites BOTH fail the individual W3/W4 tests.
    a, b, c, d = (0, 4, 4), (8, 4, 4), (4, 7, 9), (4, 1, 9)
    center, radius2 = (F(4), F(4), F(29, 5)), F(481, 25)
    support, bary = [a, b, c, d], [F(8, 25), F(8, 25), F(9, 50), F(9, 50)]
    require(all(w > 0 for w in bary) and sum(bary) == 1, "second q4 support positivity")
    require(all(power(z, center, radius2) == 0 for z in support), "second q4 sphere")
    require(all(sum(w * z[axis] for w, z in zip(bary, support)) == center[axis]
                for axis in range(3)), "second q4 positive barycenter")
    require((c[1] - a[1]) * (d[2] - a[2]) - (c[2] - a[2]) * (d[1] - a[1]) == 30,
            "second q4 rank")
    require(dot(subtract(a, b), subtract(a, b)) == 64 and
            all(dot(subtract(support[i], support[j]), subtract(support[i], support[j])) < 64
                for i in range(4) for j in range(i + 1, 4) if (i, j) != (0, 1)),
            "second q4 owner longest edge")
    sites, weights = [(4, 7, 4), (4, 1, 4)], [F(1, 2), F(1, 2)]
    ok, gap = certificate(a, b, sites, weights, F(1, 2))
    require(ok and gap == -7, "second q4 collective certificate")
    require(all(power(z, center, radius2) < 0 for z in sites), "second q4 depth")
    for z in sites:
        u, v = subtract(z, a), subtract(b, z)
        h = dot(u, v)
        xi = dot(u, u) * dot(v, v) - h * h
        require(h == 7 and xi == 576 and 3 * h * h < xi and 2 * h * h < xi,
                "individual W3/W4 obstruction not exercised")

    # Three explicit erroneous rules, each accepted by the mutant and refuted
    # by sphere geometry; these are model mutants, not product source mutants.
    a, b = (0, 1, 0), (2, 1, 0)
    sites, weights = [(1, 2, 0), (1, 0, 0)], [F(1, 2), F(1, 2)]
    require(not certificate(a, b, sites, weights, F(1, 2))[0], "equality strict gate")
    require(certificate(a, b, sites, weights, F(1, 2), allow_equal=True)[0], "weak mutant inactive")
    require(all(power(z, (1, 1, 0), F(1)) == 0 for z in sites), "weak mutant counterexample")

    a, b, sites, weights = (0, 0, 0), (10, 0, 0), [(5, 1, 0)], [F(1)]
    require(not certificate(a, b, sites, weights, F(1, 2))[0], "mean identity gate")
    require(certificate(a, b, sites, weights, F(1, 2), ignore_mean=True)[0], "mean mutant inactive")
    require(power(sites[0], (5, -100, 0), F(10025)) > 0, "mean mutant counterexample")

    a, b = (0, 10, 10), (10, 10, 10)
    u, v, w = (5, 11, 10), (5, 9, 10), (5, 12, 10)
    require(certificate(a, b, [u, v], [F(1, 2), F(1, 2)], F(1, 2))[0],
            "first overlapping group")
    require(certificate(a, b, [v, w], [F(2, 3), F(1, 3)], F(1, 2))[0],
            "second distinct overlapping group")
    sites = [u, v, w]  # Even the deduplicated union has only one interior.
    center, radius2 = (5, -90, 10), F(10025)
    exact_interior = sum(power(z, center, radius2) < 0 for z in sites)
    require(exact_interior == 1 and 2 > exact_interior, "double group credit counterexample")

    require(groups >= 200 and spheres >= 2000 and boundary_members >= 800,
            "nonvacuity floor")
    return {"status": "passed_bounded_collective_model", "rank_groups": groups,
            "sphere_checks": spheres, "strict_inside_members": inside,
            "boundary_members": boundary_members, "positive_noncoplanar_q4": 2,
            "individual_w3_w4_failures_in_positive_q4": 2,
            "refuted_model_mutants": ["closed_margin", "missing_barycenter", "overlapping_groups"],
            "full_qualification": False, "performance_claim": False, "gcp_used": False,
            "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true", required=True)
    parser.parse_args()
    print(json.dumps(run(), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
