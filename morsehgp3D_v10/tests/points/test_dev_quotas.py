"""Standalone quota gate: no engine, coordinates, float arithmetic or GCP.

Run with ``python3 [-O] test_dev_quotas.py``. Explicit checks remain active
under -O. The independent oracle uses Fraction target weights and ranks
the fractional parts. Rejections require the exact exception class.
"""

import json
import sys
from fractions import Fraction
from pathlib import Path
from typing import Callable

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "bench/frontier"))

from dev_quotas import allocate_dev_quotas  # noqa: E402


def reference_counts(n: int, groups: int) -> list[int]:
    """Hamilton oracle expressed in rational target masses, not divmod."""
    targets = [
        Fraction(n) * weight
        for _ in range(groups)
        for weight in (Fraction(84, 100 * groups) * Fraction(3, 4),
                       Fraction(84, 100 * groups) * Fraction(1, 4))
    ]
    targets.append(Fraction(n) * Fraction(16, 100))
    floors = [target.numerator // target.denominator for target in targets]
    fractions = [target - floor for target, floor in zip(targets, floors)]
    order = sorted(range(len(targets)), key=lambda i: (-fractions[i], i))
    missing = n - sum(floors)
    return [floor + int(i in order[:missing]) for i, floor in enumerate(floors)]


def main() -> int:
    """Validate non-vacuous small, scaling, exact-ratio and rejection panels."""
    checks = 0
    allocations = 0
    rejections = 0

    def check(condition: bool, message: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            raise RuntimeError(message)

    def verify(n: int, groups: int) -> list[int]:
        nonlocal allocations
        allocation = allocate_dev_quotas(n, groups)
        quotas = allocation["quotas"]
        counts = [quota["returns"] for quota in quotas]
        allocations += 1
        check(allocation["requested_returns"] == n, "requested count")
        check(allocation["generated_returns"] == n, "generated count")
        check(sum(counts) == n, "allocation total")
        check(allocation["groups"] == groups, "community count")
        check(allocation["counting_unit"] ==
              "generated_returns_before_quantization", "return/site scope")
        check(allocation["allocation_method"] == "stable_largest_remainders",
              "allocation method")
        check(len(quotas) == 2 * groups + 1, "all roles retained")
        check(counts == reference_counts(n, groups), "Fraction oracle")
        check(json.loads(json.dumps(allocation)) == allocation, "JSON manifest")
        for i, quota in enumerate(quotas):
            background = i == 2 * groups
            expected_role = (
                "background" if background else ("core", "halo")[i % 2]
            )
            expected_community = None if background else i // 2
            numerator = 16 * groups if background else (63, 21)[i % 2]
            denominator = 100 * groups
            check(quota["role"] == expected_role, "stable role order")
            check(quota["community"] == expected_community, "community identity")
            check(quota["weight_numerator"] == numerator, "weight numerator")
            check(quota["weight_denominator"] == denominator, "weight denominator")
            check(type(quota["returns"]) is int and quota["returns"] >= 0,
                  "non-negative integer quota")
            floor, remainder = divmod(n * numerator, denominator)
            check(quota["returns"] in (floor, floor + int(remainder != 0)),
                  "target floor or ceiling")
        return counts

    for groups in (1, 2, 3, 4, 7, 11):
        for n in range(101):
            verify(n, groups)
    for n in (0, 1, 2, 5, 100):
        verify(n, 113)
    for n in (8000, 16000, 32000):
        for groups in (1, 2, 3, 7, 11):
            verify(n, groups)
        counts = verify(n, 3)
        check(counts == [n * 21 // 100, n * 7 // 100] * 3 + [n * 16 // 100],
              "three-community scaling fractions")
    for groups in (1, 2, 3, 7, 11):
        check(verify(100 * groups, groups) == [63, 21] * groups + [16 * groups],
              "unrounded community ratio")
    verify(2 ** 100 + 17, 3)
    check(verify(1, 3) == [1, 0, 0, 0, 0, 0, 0], "core tie winner")
    check(verify(2, 3) == [1, 0, 1, 0, 0, 0, 0], "second core tie winner")
    check(verify(3, 3) == [1, 0, 1, 0, 1, 0, 0], "third core tie winner")
    check(verify(1500, 3) == [315, 105] * 3 + [240], "1500-return regression")
    check(verify(100, 2) == [32, 11, 31, 10, 16], "flat cross-role tie order")
    check(verify(50, 1) == [32, 10, 8], "core wins exact core/halo tie")
    check(verify(9007199254740993, 2) ==
          [2837267765243413, 945755921747804] * 2 + [1441151880758559],
          "integer count above the binary64 exact domain")
    check(allocate_dev_quotas(100) == allocate_dev_quotas(100, 3),
          "default is three communities")

    original = allocate_dev_quotas(100, 3)
    original["quotas"][0]["returns"] = 999
    original["quotas"].pop()
    check(allocate_dev_quotas(100, 3)["quotas"][0]["returns"] == 21,
          "no state shared between calls")
    check(len(allocate_dev_quotas(100, 3)["quotas"]) == 7, "no shared role list")

    def rejected(call: Callable[[], object], expected: type[Exception]) -> None:
        nonlocal rejections
        rejections += 1
        try:
            call()
        except Exception as exc:
            check(type(exc) is expected, "exact rejection class")
        else:
            check(False, "invalid input accepted")

    for invalid in (True, False, 1.0, "1", None, [], Fraction(1)):
        rejected(lambda value=invalid: allocate_dev_quotas(value, 3), TypeError)
        rejected(lambda value=invalid: allocate_dev_quotas(1, value), TypeError)
    for invalid in (-1, -2 ** 100):
        rejected(lambda value=invalid: allocate_dev_quotas(value, 3), ValueError)
    for invalid in (0, -1, -2 ** 100):
        rejected(lambda value=invalid: allocate_dev_quotas(1, value), ValueError)

    check(allocations >= 639, "allocation non-vacuity floor")
    check(rejections == 19, "rejection non-vacuity floor")
    check(checks > 40000, "explicit-check non-vacuity floor")
    print(json.dumps({"allocations": allocations, "checks": checks,
                      "rejections": rejections, "status": "PASS"}, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
