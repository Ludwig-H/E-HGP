#!/usr/bin/env python3
"""Bounded exact counter-fixture for shared versus per-anchor credits.

This standalone model does not import or qualify the v8 implementation.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def witness(a: int, b: int, z: int, q: int, closed: bool = False) -> bool:
    require(q in (2, 3, 4), "invalid lane")
    # All coordinates are (x, 0, 0): Xi=0, so every lane reduces to H>0.
    h = (z - a) * (b - z)
    return h >= 0 if closed else h > 0


def max_h4(a: int, b: int, lo: int, hi: int) -> int:
    # Four times the exact maximum over the continuous interval Z.
    twice_z = min(max(a + b, 2 * lo), 2 * hi)
    return (b - a) ** 2 - (twice_z - a - b) ** 2


def run() -> dict[str, object]:
    cases: list[dict[str, int]] = []
    lane_checks = 0
    mutant_refutations = 0
    for m in (3, 16, 64):
        gap = 4096
        left = list(range(m))
        right = list(range(gap, gap + m))
        # Even s=12 satisfies centre distance >= (s+2)*maximum radius.
        require(2 * gap >= 14 * (m - 1), "separation nonvacuity")
        shared_left = min(
            max_h4(a, b, left[0], left[-1])
            for a in (left[0], left[-1])
            for b in (right[0], right[-1])
        )
        shared_right = min(
            max_h4(a, b, right[0], right[-1])
            for a in (left[0], left[-1])
            for b in (right[0], right[-1])
        )
        require(shared_left == shared_right == 0, "shared bound changed")
        for q in (2, 3, 4):
            # Enumerate the discrete opposite factor; do not reuse the bound.
            credit_left = [
                sum(z != a and all(witness(a, b, z, q) for b in right)
                    for z in left)
                for a in left
            ]
            credit_right = [
                sum(z != b and all(witness(a, b, z, q) for a in left)
                    for z in right)
                for b in right
            ]
            require(credit_left == list(reversed(range(m))), "left oracle")
            require(credit_right == list(range(m)), "right oracle")
            require(sum(credit_left) > 0, "lost-credit nonvacuity")
            # Refute interpreting non-positive shared bound as zero row credits.
            require(credit_left != [0] * m, "quantifier mutant survived")
            mutant_refutations += 1
            for need in (1, 2, min(m, 10)):
                pairs = [
                    (i, j) for i in range(m) for j in range(m)
                    if credit_left[i] + credit_right[j] < need
                ]
                require(len(pairs) == need * (need + 1) // 2, "residual count")
                # h witnesses facing the other factor suffice on this family.
                sample_left = left[-need:]
                sample_right = right[:need]
                lower_left = [sum(z != a and witness(a, right[0], z, q)
                                  for z in sample_left) for a in left]
                lower_right = [sum(z != b and witness(left[-1], b, z, q)
                                   for z in sample_right) for b in right]
                require(lower_left == [min(need, c) for c in credit_left],
                        "directional left credits")
                require(lower_right == [min(need, c) for c in credit_right],
                        "directional right credits")
                selected = [(i, j) for i in range(m) for j in range(m)
                            if lower_left[i] + lower_right[j] < need]
                require(selected == pairs, "directional residual changed")
                require(0 < len(pairs) < m * m, "residual nonvacuity")
                lane_checks += 1
                if q == 2:
                    cases.append({"factor_size": m, "need": need,
                                  "shared_bound4": shared_left,
                                  "zero_credit_residual": m * m,
                                  "exact_credit_residual": len(pairs)})
            require(not witness(left[0], right[0], left[0], q), "open boundary")
            require(witness(left[0], right[0], left[0], q, closed=True),
                    "boundary mutant not exercised")
            mutant_refutations += 1
    require(lane_checks == 27 and mutant_refutations == 18, "gate nonvacuity")
    return {"status": "passed", "scope": "exact_collinear_model_not_v8_cpp",
            "lane_checks": lane_checks, "mutant_refutations": mutant_refutations,
            "cases": cases,
            "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: p0_quantifiers_gate.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, ValueError) as error:
        print(f"p0 quantifiers gate failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
