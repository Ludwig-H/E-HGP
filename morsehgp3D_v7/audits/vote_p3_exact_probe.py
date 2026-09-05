"""Bounded exact p3 comparisons from rational-level histograms; audit only."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass, replace
from datetime import datetime, timezone
from fractions import Fraction as Q
import hashlib
from itertools import permutations
import json
from math import gcd, isqrt, sqrt
from pathlib import Path
import sys


HERE = Path(__file__).resolve().parent
OUTPUT = HERE / "receipts_resolver_20260905/weights"
Row = tuple[int, int, int]  # unreduced numerator, positive denominator, multiplicity


@dataclass(frozen=True)
class Limits:
    input_rows: int = 256
    numerator_bits: int = 192
    denominator_bits: int = 128
    count_bits: int = 64
    coefficient_bits: int = 4096
    classes: int = 64
    class_pair_tests: int = 4096
    precision_bits: int = 128
    interval_rounds: int = 8
    accumulator_bits: int = 16384


class Rejected(Exception):
    def __init__(self, status: str, reason: str):
        super().__init__(reason)
        self.status, self.reason = status, reason


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sign(number) -> int:
    return (number > 0) - (number < 0)


def fraction_bits(value: Q) -> int:
    return max(abs(value.numerator).bit_length(), value.denominator.bit_length())


def compare_p3(left: list[Row], right: list[Row], limits: Limits = Limits(), fault: str = "") -> dict:
    """Compare sums count * (numerator/denominator)^(-3/2) without floating point.

    Fault arguments are named audit corruptions, absent from the valid contract.
    A budget refusal never provides a guessed ordering or equality.
    """
    stats = {"input_rows": len(left) + len(right), "rational_levels": 0,
             "radicands": 0, "classes": 0, "class_pair_tests": 0,
             "square_ratio_merges": 0, "isqrt_calls": 0, "interval_rounds": 0,
             "precision_bits": 0, "max_coefficient_bits": 0,
             "max_interval_integer_bits": 0, "max_accumulator_bits": 0}
    grouped = {}
    interval = None

    def bounded(ok: bool, reason: str) -> None:
        if not ok:
            raise Rejected("indeterminate", reason)

    def coefficient(value: Q) -> Q:
        size = fraction_bits(value)
        stats["max_coefficient_bits"] = max(stats["max_coefficient_bits"], size)
        bounded(size <= limits.coefficient_bits, "coefficient_bits")
        return value

    def integer_sqrt(number: int) -> int:
        stats["isqrt_calls"] += 1
        return isqrt(number)

    try:
        if any(type(value) is not int or value < 0 for value in asdict(limits).values()):
            raise Rejected("invalid_input", "limits")
        bounded(stats["input_rows"] <= limits.input_rows, "input_rows")
        levels = {}
        for direction, rows in ((1, left), (-1, right)):
            for row in rows:
                if not isinstance(row, (tuple, list)) or len(row) != 3 or any(type(value) is not int for value in row):
                    raise Rejected("invalid_input", "row_schema")
                numerator, denominator, count = row
                if numerator <= 0 or denominator <= 0 or count < 0:
                    raise Rejected("invalid_input", "positive_level_nonnegative_count")
                bounded(numerator.bit_length() <= limits.numerator_bits and
                        denominator.bit_length() <= limits.denominator_bits and
                        count.bit_length() <= limits.count_bits, "input_bits")
                level = Q(numerator, denominator)
                if not Q(1, 4) <= level <= Q(3 * 65535**2, 4):
                    raise Rejected("invalid_input", "u16_level_range")
                levels[level] = levels.get(level, 0) + direction * count
        levels = {level: count for level, count in levels.items() if count}
        stats["rational_levels"] = len(levels)
        radicands = {}
        for level, count in levels.items():
            a, b = level.numerator, level.denominator
            # (a/b)^(-3/2) = (b/a^2) sqrt(a*b).
            n = a * b
            c = coefficient(Q(count * b, a * a))
            root = integer_sqrt(n)
            if root * root == n:
                n, c = 1, coefficient(c * root)
            radicands[n] = coefficient(radicands.get(n, Q(0)) + c)
        radicands = {n: c for n, c in radicands.items() if c}
        stats["radicands"] = len(radicands)
        # Ascending representatives make permutations of the input deterministic.
        # No factorization or universal square-free normal form is required.
        for n, c in sorted(radicands.items()):
            merged = False
            for representative in list(grouped):
                bounded(stats["class_pair_tests"] < limits.class_pair_tests, "class_pair_tests")
                stats["class_pair_tests"] += 1
                divisor = gcd(n, representative)
                numerator, denominator = n // divisor, representative // divisor
                s, t = integer_sqrt(numerator), integer_sqrt(denominator)
                same = s * s == numerator and t * t == denominator
                if same and fault != "disable_square_ratio":
                    grouped[representative] = coefficient(grouped[representative] + c * Q(s, t))
                    stats["square_ratio_merges"] += 1
                    merged = True
                    break
            if not merged:
                bounded(len(grouped) < limits.classes, "classes")
                grouped[n] = c
        stats["classes"] = len(grouped)
        grouped = {n: c for n, c in grouped.items() if c}
        if not grouped:
            return {"status": "equal", "sign": 0, "reason": "all_square_class_coefficients_zero",
                    "normal_form": {}, "interval": None, "stats": stats}

        precision = 0
        while True:
            bounded(stats["interval_rounds"] < limits.interval_rounds, "interval_rounds")
            stats["interval_rounds"] += 1
            stats["precision_bits"] = precision
            lower, upper = Q(0), Q(0)
            denominator = 1 << precision
            for n, c in grouped.items():
                scaled = n << (2 * precision)
                stats["max_interval_integer_bits"] = max(stats["max_interval_integer_bits"], scaled.bit_length())
                root = integer_sqrt(scaled)
                lo = Q(root, denominator)
                exact = root * root == scaled
                hi = lo if exact or fault == "floor_upper" else Q(root + 1, denominator)
                if c >= 0:
                    lower, upper = lower + c * lo, upper + c * hi
                else:
                    lower, upper = lower + c * hi, upper + c * lo
                bits = max(fraction_bits(lower), fraction_bits(upper))
                stats["max_accumulator_bits"] = max(stats["max_accumulator_bits"], bits)
                bounded(bits <= limits.accumulator_bits, "accumulator_bits")
            interval = [str(lower), str(upper)]
            if lower > 0 or upper < 0:
                value = 1 if lower > 0 else -1
                return {"status": "greater" if value == 1 else "less", "sign": value,
                        "reason": "certified_rational_interval", "interval": interval,
                        "normal_form": {str(n): str(c) for n, c in grouped.items()}, "stats": stats}
            bounded(precision < limits.precision_bits, "precision_bits")
            precision = min(limits.precision_bits, 4 if precision == 0 else 2 * precision)
    except Rejected as error:
        return {"status": error.status, "reason": error.reason, "sign": None,
                "normal_form": {str(n): str(c) for n, c in grouped.items()},
                "interval": interval, "stats": stats}


def pell(exponent: int) -> tuple[int, int]:
    a, b = 1, 0
    for _ in range(exponent):
        a, b = a + 2 * b, a + b
    require(a * a - 2 * b * b == (-1)**exponent, "reference.pell_identity")
    return a, b


def run_cases() -> dict:
    cases = []

    def check(name: str, left: list[Row], right: list[Row], expected: str,
              limits: Limits = Limits()) -> dict:
        result = compare_p3(left, right, limits)
        require(result["status"] == expected, f"case.{name}.{result['status']}")
        cases.append({"name": name, "left": left, "right": right, "expected_status": expected,
                      "limits": asdict(limits), "result": result})
        return result

    check("empty", [], [], "equal")
    check("rational_square", [(1, 1, 1)], [(4, 1, 8)], "equal")
    check("irrational_square_ratio", [(2, 1, 1)], [(8, 1, 8)], "equal")
    check("nondividing_radicands", [(9, 2, 27)], [(25, 2, 125)], "equal")
    mixed_left = [(2, 1, 3), (3, 1, 2)]
    mixed_right = [(8, 1, 24), (12, 1, 16)]
    zero = check("mixed_irrational_equality_no_refinement", mixed_left, mixed_right, "equal",
                 replace(Limits(), precision_bits=0, interval_rounds=0))
    require(zero["stats"]["interval_rounds"] == 0 and zero["stats"]["square_ratio_merges"] == 2,
            "equality.algebraic_not_interval")
    check("duplicates_and_equivalent_fractions", [(4, 2, 1), (2, 1, 1), (3, 1, 0)], [(16, 2, 16)], "equal")
    check("positive_monotonicity", [(1, 1, 1)], [(4, 1, 1)], "greater")
    check("negative_monotonicity", [(4, 1, 1)], [(1, 1, 1)], "less")
    # sqrt(2)+sqrt(3)>sqrt(6): squaring reduces this to 2*sqrt(6)>1.
    check("three_distinct_classes_rank_two", [(2, 1, 4), (3, 1, 9)], [(6, 1, 36)], "greater")
    pell_inputs = {}
    for exponent in (22, 23, 31, 32):
        a, b = pell(exponent)
        expected = "greater" if a * a - 2 * b * b > 0 else "less"
        left, right = [(1, 1, a)], [(2, 1, 4 * b)]
        if exponent in (22, 23):
            require(a + 4 * b < 2**31, "pell.current_incidence_cap_scale")
        result = check(f"pell_{exponent}", left, right, expected)
        require(result["stats"]["precision_bits"] >= 64, "pell.nonvacuous_refinement")
        limited = check(f"pell_{exponent}_bounded", left, right, "indeterminate",
                        replace(Limits(), precision_bits=16))
        require(limited["reason"] == "precision_bits" and limited["sign"] is None, "budget.no_guess")
        pell_inputs[exponent] = (left, right, expected)
    check("term_budget", mixed_left, mixed_right, "indeterminate", replace(Limits(), input_rows=1))
    check("class_budget", [(2, 1, 4), (3, 1, 9)], [(6, 1, 36)], "indeterminate",
          replace(Limits(), classes=1))
    check("pair_budget", [(9, 2, 27)], [(25, 2, 125)], "indeterminate",
          replace(Limits(), class_pair_tests=0))
    check("coefficient_budget", [(1, 1, 100)], [], "indeterminate", replace(Limits(), coefficient_bits=3))
    check("zero_level", [(0, 1, 1)], [], "invalid_input")
    check("zero_denominator", [(1, 0, 1)], [], "invalid_input")
    check("negative_multiplicity", [(1, 1, -1)], [], "invalid_input")
    check("level_outside_u16_domain", [(1, 8, 1)], [], "invalid_input")
    check("malformed_row", [(1, 1)], [], "invalid_input")
    check("noninteger_multiplicity", [(1, 1, 1.0)], [], "invalid_input")

    variants = 0
    reference = compare_p3(mixed_left, mixed_right)
    for lhs in permutations(mixed_left):
        for rhs in permutations(mixed_right):
            result = compare_p3(list(lhs), list(rhs))
            require(result["status"] == reference["status"] and result["normal_form"] == reference["normal_form"],
                    "permutation.invariance")
            variants += 1
    faults = {}
    bad = compare_p3([(2, 1, 1)], [(8, 1, 8)], fault="disable_square_ratio")
    require(bad["status"] == "indeterminate", "fault.square_ratio_not_exercised")
    faults["audit_disable_square_ratio"] = {"wanted": "equal", "observed": bad["status"]}
    left, right, expected = pell_inputs[31]
    bad = compare_p3(left, right, fault="floor_upper")
    require(expected == "less" and bad["status"] == "greater", "fault.unsound_upper_not_exercised")
    faults["audit_floor_used_as_upper_bound"] = {"wanted": expected, "observed": bad["status"]}
    # Exact wrong-profile value for the rational-square equality above.
    wrong_exponent_difference = Q(1) - 8 * Q(1, 4**3)
    require(wrong_exponent_difference > 0, "fault.wrong_exponent_not_exercised")
    faults["audit_beta_minus_three"] = {"wanted": "equal", "observed": "greater",
                                        "wrong_difference": str(wrong_exponent_difference)}
    a, b = pell(32)
    approximate_difference = float(a) - float(b) * sqrt(2.0)
    guessed = "equal" if abs(approximate_difference) < 1e-6 else "greater" if approximate_difference > 0 else "less"
    require(guessed != "greater", "fault.epsilon_not_exercised")
    faults["audit_float_epsilon_equality"] = {"wanted": "greater", "observed": guessed,
                                            "binary64_difference": approximate_difference}
    statuses = {status: sum(case["result"]["status"] == status for case in cases)
                for status in ("equal", "greater", "less", "indeterminate", "invalid_input")}
    require(statuses["equal"] >= 5 and statuses["greater"] >= 3 and statuses["less"] >= 2 and
            statuses["indeterminate"] >= 5 and statuses["invalid_input"] >= 4 and len(faults) == 4,
            "nonvacuity")
    return {"numeric_profile": "p3_exact_algebraic_equality_and_rational_interval_sign",
            "cases": cases, "status_counts": statuses, "permutation_checks": variants,
            "audit_faults_rejected": faults,
            "scope": "audit_only_no_product_execution_no_universal_latency_claim"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--receipt", choices=("normal", "optimized"), required=True)
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve()
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    report = {"status": "running", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference",
              "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture",
              "public_status": "not_claimed", "gcp": "not_used", "utc": datetime.now(timezone.utc).isoformat(),
              "python": sys.version, "optimization": sys.flags.optimize, "argv": sys.argv,
              "source": str(source.relative_to(HERE)), "source_sha256_before": before}
    try:
        report["result"] = run_cases()
        after = hashlib.sha256(source.read_bytes()).hexdigest()
        require(before == after, "source.changed")
        report["source_sha256_after"] = after
        report["status"] = "completed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
    (OUTPUT / f"{args.receipt}.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    print(f"vote p3 status={report['status']} optimization={sys.flags.optimize}")
    if "error" in report:
        print(report["error"])
    return int(report["status"] != "completed")


if __name__ == "__main__":
    raise SystemExit(main())
