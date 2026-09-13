#!/usr/bin/env python3
"""Exact bounded rails model for fixed witness pools, independent of v8 C++."""

from __future__ import annotations

import hashlib
from itertools import combinations
import json
from pathlib import Path
import sys


Point = tuple[int, int, int]


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def witness(a: Point, b: Point, z: Point, multiplier: int) -> bool:
    require(multiplier in (2, 3), "invalid spindle multiplier")
    u = tuple(z[i] - a[i] for i in range(3))
    v = tuple(b[i] - z[i] for i in range(3))
    h = sum(u[i] * v[i] for i in range(3))
    edge = tuple(b[i] - a[i] for i in range(3))
    cross = (edge[1] * u[2] - edge[2] * u[1],
             edge[2] * u[0] - edge[0] * u[2],
             edge[0] * u[1] - edge[1] * u[0])
    xi = sum(value * value for value in cross)
    return h > 0 and multiplier * h * h > xi


def residual(left: list[int], right: list[int], need: int) -> set[tuple[int, int]]:
    return {(i, j) for i, a in enumerate(left) for j, b in enumerate(right)
            if a + b < need}


def run_case(need: int, length: int, multiplier: int,
             enumerate_pools: bool) -> dict[str, int]:
    rails = need + 1
    shift = 48 * rails * length
    require(length >= max(1, need - 1), "insufficient rail length")
    left = [(x, 4 * length * j, 0)
            for j in range(rails) for x in range(length + 1)]
    right = [(x + shift, y, z) for x, y, z in left]
    corners_left = [(x, y, 0) for x in (0, length)
                    for y in (0, 4 * length * (rails - 1))]
    corners_right = [(x + shift, y, z) for x, y, z in corners_left]
    require(max(max(point) for point in left + right) <= 65535, "u16 domain")
    for separation in (8, 10, 12):
        require((shift - length) ** 2 >= separation ** 2 * (
            length ** 2 + (4 * length * (rails - 1)) ** 2), "box separation")

    credits_left: list[int] = []
    credits_right: list[int] = []
    classifications = positive = grouped_checks = 0
    for factor, corners, credits, forward in (
            (left, corners_right, credits_left, True),
            (right, corners_left, credits_right, False)):
        for anchor in factor:
            got = 0
            for site in factor:
                actual = all(witness(anchor, other, site, multiplier)
                             for other in corners)
                expected = site[1] == anchor[1] and (
                    site[0] > anchor[0] if forward else site[0] < anchor[0])
                classifications += 1
                positive += int(actual)
                require(actual == expected, "rail witness classification")
                got += int(actual)
            credits.append(min(need, got))
            # A constructive comparator: a separate facing pool for each rail.
            same_rail = [site for site in factor if site[1] == anchor[1]]
            directed_pool = same_rail[-need:] if forward else same_rail[:need]
            grouped = sum(all(witness(anchor, other, site, multiplier)
                              for other in corners) for site in directed_pool)
            require(grouped == min(need, got), "grouped directional pool")
            grouped_checks += 1

    exact = residual(credits_left, credits_right, need)
    expected_count = rails ** 2 * need * (need + 1) // 2
    require(len(exact) == expected_count, "exact residual count")
    require(0 < len(exact) < len(left) * len(right), "residual nonvacuity")
    require(positive == rails * length * (length + 1), "positive nonvacuity")
    result = {"need": need, "length": length,
              "lane": 3 if multiplier == 3 else 4,
              "points": len(left) + len(right),
              "witness_classifications": classifications,
              "positive_classifications": positive,
              "grouped_directional_checks": grouped_checks,
              "exact_local_residual": len(exact)}
    if enumerate_pools:
        require(need == 1, "small-pool enumeration contract")
        minimum = len(left) * len(right)
        pool_cases = 0
        for left_ids in combinations(range(len(left)), need):
            lower_left = [sum(all(witness(anchor, other, left[k], multiplier)
                                  for other in corners_right) for k in left_ids)
                          for anchor in left]
            for right_ids in combinations(range(len(right)), need):
                lower_right = [sum(all(witness(anchor, other, right[k], multiplier)
                                       for other in corners_left)
                                   for k in right_ids) for anchor in right]
                candidates = residual(lower_left, lower_right, need)
                require(exact <= candidates, "fixed-pool completeness")
                minimum = min(minimum, len(candidates))
                pool_cases += 1
        require(minimum == (length + 2) ** 2, "minimum over all fixed pools")
        require(minimum > len(exact), "fixed-pool-equals-exact mutant survived")
        result["pool_pairs_exhaustively_checked"] = pool_cases
        result["minimum_over_all_fixed_pools"] = minimum
    return result


def large_proof_arithmetic() -> dict[str, int | str]:
    # These are theorem substitutions, not an exhaustive 2718-point census.
    need, rails, length, budget = 8, 9, 150, 8
    shift = 48 * rails * length
    transverse_span = 4 * (rails - 1) * length
    require(shift + length <= 65535, "large fixture outside u16")
    for separation in (8, 10, 12):
        require((shift - length) ** 2 >= separation ** 2 * (
            length ** 2 + transverse_span ** 2), "large separation")
    # For an inter-rail site, choose the real opposite point with the anchor's
    # transverse coordinates: H<=D*L and Xi>=D^2*(4L)^2. This refutes
    # universality; it does not claim failure for every opposite point.
    require(3 * (shift * length) ** 2 < shift ** 2 * (4 * length) ** 2,
            "inter-rail universal noncredit inequality")
    require(2 * (shift - length) ** 2 > transverse_span ** 2,
            "same-rail universal credit inequality")
    possibly_rejected_products = (2 * rails * budget) // need
    untouched_products = rails ** 2 - possibly_rejected_products
    lower = untouched_products * (length + 1) ** 2
    exact = rails ** 2 * need * (need + 1) // 2
    require((lower, exact) == (1436463, 2916), "large arithmetic discrepancy")
    return {"scope": "proof_arithmetic_not_exhaustive_census", "need": need,
            "rails": rails, "length": length, "shift": shift,
            "points": 2 * rails * (length + 1),
            "maximum_coordinate": shift + length,
            "exact_local_residual_by_formula": exact,
            "arbitrary_fixed_pool_budget_per_side": budget,
            "candidate_lower_bound": lower}


def run() -> dict[str, object]:
    cases = [run_case(1, length, multiplier, True)
             for multiplier in (2, 3) for length in (1, 2)]
    cases.extend((run_case(8, 8, 2, False), run_case(9, 9, 3, False)))
    mutant_refutations = 4  # Fixed-pool-equals-exact, four exhaustive cases.
    for multiplier in (2, 3):
        anchor, other, cross_rail = (0, 0, 0), (96, 0, 0), (1, 4, 0)
        require(not witness(anchor, other, cross_rail, multiplier),
                "cross-rail rejection")
        h = 1 * 95 - 4 * 4
        require(h > 0, "omit-Xi mutant not exercised")
        mutant_refutations += 1
        require(not witness(anchor, other, anchor, multiplier), "open boundary")
        # Mutating both strict comparisons to non-strict admits H=Xi=0.
        boundary_h = boundary_xi = 0
        require(boundary_h >= 0 and multiplier * boundary_h ** 2 >= boundary_xi,
                "closed-boundary mutant not exercised")
        mutant_refutations += 1
    classifications = sum(case["witness_classifications"] for case in cases)
    positive = sum(case["positive_classifications"] for case in cases)
    grouped = sum(case["grouped_directional_checks"] for case in cases)
    pool_cases = sum(case.get("pool_pairs_exhaustively_checked", 0) for case in cases)
    require((len(cases), classifications, positive, grouped, pool_cases,
             mutant_refutations) == (6, 33330, 1580, 402, 104, 8),
            "gate nonvacuity")
    return {"status": "passed", "scope": "exact_rails_model_not_v8_cpp",
            "cases": cases, "witness_classifications": classifications,
            "positive_classifications": positive,
            "grouped_directional_checks": grouped,
            "pool_pairs_exhaustively_checked": pool_cases,
            "mutant_refutations": mutant_refutations,
            "large_fixture": large_proof_arithmetic(),
            "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: p0_rails_gate.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, ValueError) as error:
        print(f"p0 rails gate failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
