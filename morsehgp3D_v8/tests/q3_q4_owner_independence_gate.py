#!/usr/bin/env python3
"""Exact, test-only counterexamples: rejected q2 owners can support q3/q4.

No product geometry is imported. Fraction arithmetic verifies the proposed
centers, strict positive barycentric coordinates, shells and empty interiors.
This is not a q3/q4 generator, nor a completeness or performance qualification.
"""

from fractions import Fraction as F
import json
import sys


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def distance2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b, strict=True))


def determinant3(columns):
    a, b, c = columns
    return (a[0] * (b[1] * c[2] - b[2] * c[1])
            - b[0] * (a[1] * c[2] - a[2] * c[1])
            + c[0] * (a[1] * b[2] - a[2] * b[1]))


def check_fixture(k, q):
    a, b = (900, 1000, 1000), (1100, 1000, 1000)
    witnesses = [(1000 + j, 910, 1000) for j in range(k)]
    if q == 3:
        support = [a, b, (1000, 1120, 1000)]
        center, r2 = (F(1000), F(3055, 3), F(1000)), F(93025, 9)
        weights = [F(61, 144), F(61, 144), F(11, 72)]
        # Nonzero xy determinant certifies affine independence in its plane.
        require((b[0] - a[0]) * (support[2][1] - a[1]) != 0, "collinear q3")
    else:
        support = [a, b, (1000, 1120, 1040), (1000, 1120, 960)]
        center, r2 = (F(1000), F(1025), F(1000)), F(10625)
        weights = [F(19, 48), F(19, 48), F(5, 48), F(5, 48)]
        differences = [tuple(x - y for x, y in zip(p, a, strict=True)) for p in support[1:]]
        require(determinant3(differences) != 0, "coplanar q4")
    points = support + witnesses
    require(len(set(points)) == len(points), "duplicate site")
    require(all(0 <= x <= 65535 for p in points for x in p), "outside u16")
    require(all(w > 0 for w in weights) and sum(weights) == 1, "non-positive support")
    require(tuple(sum(w * p[axis] for w, p in zip(weights, support, strict=True))
                  for axis in range(3)) == center, "barycentric center mismatch")
    require(all(distance2(p, center) == r2 for p in support), "support not on sphere")
    # Positive barycentric center + affine independence gives minimal positive
    # support; no proper subset has this center in its convex hull.
    require(all(distance2(support[i], support[j]) < distance2(a, b)
                for i in range(q) for j in range(i + 1, q) if (i, j) != (0, 1)),
            "ab is not the unique maximal edge")
    require(all(sum((z[t] - a[t]) * (b[t] - z[t]) for t in range(3)) == 1900 - j*j > 0
                for j, z in enumerate(witnesses)), "q2 does not saturate at K")
    powers = [r2 - distance2(z, center) for z in witnesses]
    offset = 1400 if q == 3 else 2600
    require(powers == [-offset - j*j for j in range(k)] and all(p < 0 for p in powers),
            "q3/q4 witness entered the sphere")
    shell = [i for i, p in enumerate(points) if distance2(p, center) == r2]
    require(shell == list(range(q)), "shell is not exactly the support")
    require(all(distance2(p, center) >= r2 for p in points), "nonempty strict interior")
    hq = k + 2 - q
    require(hq > 0, "inactive lane in fixture")
    return {"q": q, "kmax": k, "sites": len(points), "q2_depth_at_least": k,
            "support_depth": 0, "lane_rejection_threshold": hq,
            "center": [str(x) for x in center], "radius_squared": str(r2),
            "unique_maximal_edge": [0, 1], "shell": shell,
            "witness_powers": [str(x) for x in powers]}


def check_deep_seed(k):
    support = [(900, 1000, 1000), (1100, 1000, 1000),
               (1000, 1120, 1040), (1000, 1120, 960)]
    seed_center, seed_r2 = (F(1000), F(2045, 2), F(2015, 2)), F(21125, 2)
    seed_weights = [F(13, 32), F(13, 32), F(3, 16)]
    require(tuple(sum(w * p[axis] for w, p in zip(seed_weights, support[:3], strict=True))
                  for axis in range(3)) == seed_center, "seed barycentric center mismatch")
    require(sum(seed_weights) == 1 and all(w > 0 for w in seed_weights), "seed is not positive")
    require(all(distance2(p, seed_center) == seed_r2 for p in support[:3]), "seed sphere mismatch")
    witnesses = [(1000 + j, 1020, 1105) for j in range(k - 1)]
    require(len(set(support + witnesses)) == len(support) + len(witnesses), "duplicate seed witness")
    seed_powers = [distance2(p, seed_center) - seed_r2 for p in witnesses]
    tetra_powers = [distance2(p, (1000, 1025, 1000)) - 10625 for p in witnesses]
    require(seed_powers == [-1050 + j*j for j in range(k - 1)] and all(v < 0 for v in seed_powers),
            "q3 seed does not saturate at K-1")
    require(tetra_powers == [425 + j*j for j in range(k - 1)] and all(v > 0 for v in tetra_powers),
            "witness entered the q4 sphere")
    require(distance2(support[3], seed_center) - seed_r2 == 1200, "d is not outside the seed sphere")
    # The tetrahedron's affine independence/positive support and ownership
    # were independently checked above. Its complete shell is unchanged here.
    return {"kmax": k, "seed_ids": [0, 1, 2], "q3_seed_depth": k - 1,
            "q4_depth": 0, "seed_power_distance_minus_radius": [str(x) for x in seed_powers],
            "q4_power_distance_minus_radius": [str(x) for x in tetra_powers]}


def check_row_tangency():
    """Exact boundary of the two-row argument, not a product q3 test.

    On the second row, t=2u makes the two circle intersections coincide.
    There are then zero strict interiors there, not minus one. This one
    point difference changes acceptance exactly at h3=9 (Kmax10).
    """
    delta, d, i, h3 = 4, 200, 5, 9
    cloud = [(x, delta * j, 0) for x in (0, d) for j in range(11)]
    rows = []
    for j in (9, 10):
        u, t = delta * i, delta * j
        a, b, x = (0, 0, 0), (d, u, 0), (0, t, 0)
        center = (F(d*d + u*u - u*t, 2*d), F(t, 2), F(0))
        r2 = distance2(a, center)
        wb = center[0] / d
        wx = (center[1] - u * wb) / t
        require(wb > 0 and wx > 0 and 1 - wb - wx > 0, "row triangle not positive")
        require(all(distance2(v, center) == r2 for v in (a, b, x)), "row sphere mismatch")
        require(distance2(a, b) >= max(distance2(a, x), distance2(b, x)), "row owner not maximal")
        inside = [p for p in cloud if distance2(p, center) < r2]
        shell = [p for p in cloud if distance2(p, center) == r2]
        depth = j - 1 + max(2*i - j - 1, 0)
        require(len(inside) == depth, "row strict chord count mismatch")
        require(len(shell) == (4 if j < 2*i else 3), "row tangent shell mismatch")
        require((depth < h3) == (j == 9), "tangency did not change threshold decision")
        rows.append({"i": i, "j": j, "delta": delta, "D": d,
                     "strict_depth": depth, "shell_size": len(shell),
                     "kmax": 10, "h3": h3, "alive": depth < h3})
    require(rows[1]["strict_depth"] != 2*i - 2, "tangent-as-open-chord mutant survived")
    return rows


def main():
    if sys.argv[1:] != ["--selftest"]:
        print("usage: q3_q4_owner_independence_gate.py --selftest", file=sys.stderr)
        return 2
    rows = [check_fixture(k, q) for k in (5, 10) for q in (3, 4)]
    print(json.dumps({"status": "passed", "fixtures": rows,
                      "deep_seed_fixtures": [check_deep_seed(k) for k in (5, 10)],
                      "row_tangency_fixtures": check_row_tangency(),
                      "claim": "q2_survival_is_not_a_q3_q4_owner_filter",
                      "full_contract_qualified": False}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
