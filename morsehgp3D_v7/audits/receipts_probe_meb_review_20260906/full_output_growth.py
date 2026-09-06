#!/usr/bin/env python3
"""Exact finite witnesses for quadratic FULL births; no C++ or timing claim."""
from __future__ import annotations

from fractions import Fraction as Q
import hashlib
import json
from pathlib import Path
import re

AUDIT = Path(__file__).resolve().parents[1]
ROOT = AUDIT.parent.parent
GATE = ROOT / "morsehgp3D_v7/tests/linked_arcs_gate.cpp"
GATE_SHA = "cb11c5e16ba613ee87b2c27848adaa25d5722373f4d16ab5aa151cacfc047614"


def require(value: bool, reason: str) -> None:
    if not value:
        raise RuntimeError(reason)


def diametral_power(a, b, z):
    return sum((z[d] - a[d]) * (z[d] - b[d]) for d in range(3))


def test_cloud(m, k):
    h, epsilon = Q(1, 4 * m * m), Q(1, 4 * m)
    ts = [(i + 1) * h for i in range(m)]
    a = [(1 - t * t / 2, t, Q(0)) for t in ts]
    b = [(s * s / 2, Q(0), s) for s in ts]
    anchors = [(Q(1, 2), Q(i, 100 * k), Q(0)) for i in range(1, k - 1)]
    points = a + b + anchors
    require(len(set(points)) == len(points), "distinct point positions")
    require(h / 2 > epsilon ** 3, "strict symbolic power margin")
    foreign_count = interior_count = 0
    minimum_foreign = maximum_interior = None
    labels = set()
    for i, left in enumerate(a):
        for j, right in enumerate(b):
            selected = frozenset({i, m + j, *range(2 * m, len(points))})
            require(len(selected) == k, "FULL minimum cardinal")
            require(selected not in labels, "distinct FULL labels")
            labels.add(selected)
            center = tuple((left[d] + right[d]) / 2 for d in range(3))
            radius = sum((left[d] - right[d]) ** 2 for d in range(3)) / 4
            require(radius > 0, "positive radius")
            for p, point in enumerate(points):
                value = diametral_power(left, right, point)
                require(value == sum((point[d] - center[d]) ** 2 for d in range(3)) - radius,
                        "independent center-radius power equality")
                if p in (i, m + j):
                    require(value == 0, "support boundary")
                elif p in selected:
                    require(value < 0, "anchor strictly interior")
                    interior_count += 1
                    maximum_interior = value if maximum_interior is None else max(maximum_interior, value)
                else:
                    require(value > 0, "foreign site strictly outside")
                    foreign_count += 1
                    minimum_foreign = value if minimum_foreign is None else min(minimum_foreign, value)
                    if p < m:
                        u, t, s = ts[p], ts[i], ts[j]
                    else:
                        u, t, s = ts[p - m], ts[j], ts[i]
                    expanded = (u - t) ** 2 / 2 + (u * u - t * t) * (u * u + s * s) / 4
                    lower = abs(u - t) * (abs(u - t) / 2 - epsilon ** 3)
                    require(value == expanded and expanded >= lower > 0,
                            "exact expansion and uniform lower bound")
    require(len(labels) == m * m, "quadratic birth-label count")
    require(foreign_count == m * m * (2 * m - 2), "foreign test non-vacuity")
    require(interior_count == m * m * (k - 2), "interior test non-vacuity")
    return dict(m=m, k=k, n=len(points), distinct_birth_labels=len(labels),
                foreign_power_checks=foreign_count, interior_power_checks=interior_count,
                minimum_foreign_power=str(minimum_foreign),
                maximum_interior_power=None if maximum_interior is None else str(maximum_interior),
                coordinates=[[str(x) for x in point] for point in points],
                labels=[sorted(label) for label in sorted(labels, key=lambda row: sorted(row))],
                strict_coface_basis="MEB uniqueness for the two antipodal support sites; "
                                    "every foreign point is strictly outside that unique ball",
                regularity="Only these labelled balls certified; entire cloud regularity not claimed")


def check_integer_gate():
    require(hashlib.sha256(GATE.read_bytes()).hexdigest() == GATE_SHA, "v7 literal gate pin")
    text = GATE.read_text()
    x = [int(v) for v in re.search(r"kFixX\[17\] = \{([^}]+)\}", text).group(1).split(",")]
    u = [int(v) for v in re.search(r"kFixU\[17\] = \{([^}]+)\}", text).group(1).split(",")]
    require(len(x) == len(u) == 17, "literal coordinate lengths")
    rows = []
    for m, expected, expected_margin in ((2, 9, 2877505), (4, 25, 718129),
                                          (8, 81, 178009), (16, 289, 29464)):
        selected = list(range(0, 17, 16 // m))
        a = [(x[i], u[i], 30000) for i in selected]
        b = [(60000 - x[i], 30000, u[i]) for i in selected]
        points = a + b
        require(all(0 <= c <= 65535 for p in points for c in p), "u16 domain")
        count, minimum = 0, None
        for i, left in enumerate(a):
            for j, right in enumerate(b):
                for p, z in enumerate(points):
                    if p in (i, len(a) + j):
                        continue
                    value = diametral_power(left, right, z)
                    require(value > 0, "u16 cross-pair strict Gabriel")
                    count += 1
                    minimum = value if minimum is None else min(minimum, value)
        require(len(a) * len(b) == expected and minimum == expected_margin,
                "u16 independent literal floors")
        rows.append(dict(m=m, n=len(points), cross_pair_births_k2=expected,
                         foreign_power_checks=count, minimum_foreign_power=minimum))
    return rows


def main():
    rows = [test_cloud(m, k) for m in (2, 4, 8, 16) for k in (2, 3, 4, 10)]
    result = dict(status="passed", scope="exact rational and integer arithmetic only; no FULL engine",
                  public_status="not_claimed", gcp="not_used", rational=rows,
                  integer_u16=check_integer_gate(),
                  rational_tested_labels=sum(row["distinct_birth_labels"] for row in rows),
                  rational_foreign_power_checks=sum(row["foreign_power_checks"] for row in rows),
                  rational_anchor_power_checks=sum(row["interior_power_checks"] for row in rows),
                  theorem_scope="For each fixed K>=2, growing rational precision permits Omega(N^2) "
                                "FULL birth labels on generically perturbed regular 3D clouds",
                  limitations=["No asymptotic sequence in fixed u16 universe",
                               "The explicit rational clouds are not claimed globally regular",
                               "No inherited v6 execution or q3/q4 C++ result",
                               "No performance or complete finite-cloud FULL qualification"],
                  pins={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                        for path in (Path(__file__), GATE)})
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
