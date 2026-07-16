#!/usr/bin/env python3
"""Validate the closed decision-only predicate batch against Fraction oracles."""

from __future__ import annotations

import argparse
import json
import struct
import subprocess
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Sequence


PointWords = tuple[str, str, str]
ExactPoint = tuple[Fraction, Fraction, Fraction]

ZERO = "0000000000000000"
ONE = "3ff0000000000000"
TWO = "4000000000000000"
NEGATIVE_ONE = "bff0000000000000"
ONE_ULP_BELOW = "3fefffffffffffff"
MINIMUM_SUBNORMAL = "0000000000000001"
NAN = "7ff8000000000000"

ORIGIN: PointWords = (ZERO, ZERO, ZERO)
E1: PointWords = (ONE, ZERO, ZERO)
E2: PointWords = (ZERO, ONE, ZERO)
E3: PointWords = (ZERO, ZERO, ONE)

OUTPUT_FIELDS = {"certification_stage", "counters", "predicate", "sign"}
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise AssertionError(f"decision-only output contains duplicate key {key!r}")
        result[key] = value
    return result


def binary64(word: str) -> Fraction:
    value = struct.unpack(">d", bytes.fromhex(word))[0]
    return Fraction.from_float(value)


def exact_point(words: PointWords) -> ExactPoint:
    return tuple(binary64(word) for word in words)  # type: ignore[return-value]


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


@dataclass(frozen=True)
class Case:
    name: str
    predicate: str
    points: tuple[PointWords, ...]
    adaptive_stage: str
    witness: tuple[int, int, int, int] | None = None
    r_ids: tuple[int, ...] = ()
    q_ids: tuple[int, ...] = ()

    def command(self) -> str:
        if self.predicate != "power_bisector_side":
            return " ".join(
                [self.predicate, *(word for point in self.points for word in point)]
            )
        if self.witness is None:
            raise AssertionError(f"power case {self.name} has no witness")
        return " ".join(
            [
                self.predicate,
                *(str(value) for value in self.witness),
                str(len(self.points)),
                *(word for point in self.points for word in point),
                str(len(self.r_ids)),
                *(str(identifier) for identifier in self.r_ids),
                str(len(self.q_ids)),
                *(str(identifier) for identifier in self.q_ids),
            ]
        )

    def oracle(self) -> Fraction:
        points = tuple(exact_point(point) for point in self.points)
        if self.predicate == "compare_squared_distances":
            witness, left, right = points
            left_distance = sum(
                ((a - b) ** 2 for a, b in zip(witness, left)), Fraction()
            )
            right_distance = sum(
                ((a - b) ** 2 for a, b in zip(witness, right)), Fraction()
            )
            return left_distance - right_distance
        if self.predicate == "orientation_3d":
            a, b, c, d = points
            u = tuple(bi - ai for ai, bi in zip(a, b))
            v = tuple(ci - ai for ai, ci in zip(a, c))
            w = tuple(di - ai for ai, di in zip(a, d))
            return (
                u[0] * (v[1] * w[2] - v[2] * w[1])
                - u[1] * (v[0] * w[2] - v[2] * w[0])
                + u[2] * (v[0] * w[1] - v[1] * w[0])
            )
        if self.predicate != "power_bisector_side" or self.witness is None:
            raise AssertionError(f"case {self.name} has an unsupported predicate")
        r_points = tuple(points[index] for index in self.r_ids)
        q_points = tuple(points[index] for index in self.q_ids)
        delta_coordinates = tuple(
            sum((point[axis] for point in r_points), Fraction())
            - sum((point[axis] for point in q_points), Fraction())
            for axis in range(3)
        )
        delta_squared_norms = sum(
            (
                sum((coordinate * coordinate for coordinate in point), Fraction())
                for point in r_points
            ),
            Fraction(),
        ) - sum(
            (
                sum((coordinate * coordinate for coordinate in point), Fraction())
                for point in q_points
            ),
            Fraction(),
        )
        witness = tuple(
            Fraction(numerator, self.witness[3]) for numerator in self.witness[:3]
        )
        return -2 * sum(
            (
                coordinate * difference
                for coordinate, difference in zip(witness, delta_coordinates)
            ),
            Fraction(),
        ) + delta_squared_norms


CASES = (
    Case(
        "distance_fp64",
        "compare_squared_distances",
        (ORIGIN, E1, (TWO, ZERO, ZERO)),
        "fp64_filtered",
    ),
    Case(
        "distance_expansion",
        "compare_squared_distances",
        (ORIGIN, (ONE_ULP_BELOW, ZERO, ZERO), E1),
        "expansion",
    ),
    Case(
        "distance_zero",
        "compare_squared_distances",
        (ORIGIN, (NEGATIVE_ONE, ZERO, ZERO), E1),
        "expansion",
    ),
    Case(
        "distance_fallback",
        "compare_squared_distances",
        (ORIGIN, (MINIMUM_SUBNORMAL, ZERO, ZERO), ORIGIN),
        "cpu_multiprecision",
    ),
    Case("orientation_fp64", "orientation_3d", (ORIGIN, E1, E2, E3), "fp64_filtered"),
    Case(
        "orientation_fp64_negative",
        "orientation_3d",
        (ORIGIN, E2, E1, E3),
        "fp64_filtered",
    ),
    Case(
        "orientation_expansion",
        "orientation_3d",
        (ORIGIN, E1, E2, (ONE, ONE, MINIMUM_SUBNORMAL)),
        "expansion",
    ),
    Case(
        "orientation_zero",
        "orientation_3d",
        (ORIGIN, E1, E2, (ONE, ONE, ZERO)),
        "expansion",
    ),
    Case(
        "orientation_fallback",
        "orientation_3d",
        (
            ORIGIN,
            (MINIMUM_SUBNORMAL, ZERO, ZERO),
            (ZERO, MINIMUM_SUBNORMAL, ZERO),
            (ZERO, ZERO, MINIMUM_SUBNORMAL),
        ),
        "cpu_multiprecision",
    ),
    Case(
        "power_fp64",
        "power_bisector_side",
        (E1, (TWO, ZERO, ZERO)),
        "fp64_filtered",
        (0, 0, 0, 1),
        (1,),
        (0,),
    ),
    Case(
        "power_fp64_negative",
        "power_bisector_side",
        (E1, (TWO, ZERO, ZERO)),
        "fp64_filtered",
        (0, 0, 0, 1),
        (0,),
        (1,),
    ),
    Case(
        "power_expansion_zero",
        "power_bisector_side",
        (E1, (TWO, ZERO, ZERO)),
        "expansion",
        (3, 0, 0, 2),
        (1,),
        (0,),
    ),
    Case(
        "power_rational_fallback",
        "power_bisector_side",
        (E1, (TWO, ZERO, ZERO)),
        "cpu_multiprecision",
        (1, 0, 0, 3),
        (1,),
        (0,),
    ),
    Case(
        "power_underflow_fallback",
        "power_bisector_side",
        ((MINIMUM_SUBNORMAL, ZERO, ZERO), ORIGIN),
        "cpu_multiprecision",
        (0, 0, 0, 1),
        (0,),
        (1,),
    ),
)


def expected_counters(stage: str, exact_sign: str) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 1 if stage == "cpu_multiprecision" else 0,
        "exact_zeros": 1 if exact_sign == "zero" else 0,
        "expansion_certified": 1 if stage == "expansion" else 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 1 if stage == "fp64_filtered" else 0,
        "remaining_unknown": 0,
    }


def run(
    executable: Path,
    arguments: Sequence[str],
    corpus: str,
    *,
    timeout: int = 30,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(executable), *arguments],
        input=corpus,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        text=True,
        timeout=timeout,
    )


def parse_results(completed: subprocess.CompletedProcess[str]) -> list[dict[str, object]]:
    if completed.returncode != 0 or completed.stderr:
        raise AssertionError(
            "decision-only batch failed: "
            f"returncode={completed.returncode}, stderr={completed.stderr!r}"
        )
    lines = completed.stdout.splitlines()
    if len(lines) != len(CASES):
        raise AssertionError(
            f"decision-only batch returned {len(lines)} rows for {len(CASES)} cases"
        )
    results: list[dict[str, object]] = []
    for line in lines:
        value = json.loads(line, object_pairs_hook=reject_duplicate_keys)
        if not isinstance(value, dict) or canonical_json(value) != line:
            raise AssertionError("decision-only output is not canonical JSON")
        results.append(value)
    return results


def audit_batch(
    executable: Path, multiprecision_only: bool, timeout: int
) -> dict[str, int]:
    corpus = "".join(f"{case.command()}\n" for case in CASES)
    arguments = ["--decision-only", "--batch"]
    if multiprecision_only:
        arguments.insert(0, "--multiprecision-only")
    results = parse_results(run(executable, arguments, corpus, timeout=timeout))
    stages = {"cpu_multiprecision": 0, "expansion": 0, "fp64_filtered": 0}
    for case, observed in zip(CASES, results):
        if set(observed) != OUTPUT_FIELDS:
            raise AssertionError(
                f"{case.name} leaked fields outside the closed decision schema: {sorted(observed)}"
            )
        expected_sign = sign(case.oracle())
        expected_stage = (
            "cpu_multiprecision" if multiprecision_only else case.adaptive_stage
        )
        if observed["predicate"] != case.predicate:
            raise AssertionError(f"{case.name} changed its predicate identity")
        if observed["sign"] != expected_sign:
            raise AssertionError(
                f"{case.name} differs from Fraction: expected {expected_sign}, "
                f"observed {observed['sign']}"
            )
        if observed["certification_stage"] != expected_stage:
            raise AssertionError(
                f"{case.name} used {observed['certification_stage']!r}, "
                f"expected {expected_stage!r}"
            )
        counters = observed["counters"]
        if not isinstance(counters, dict) or set(counters) != COUNTER_FIELDS:
            raise AssertionError(f"{case.name} returned an invalid counter schema")
        if any(type(value) is not int or value < 0 for value in counters.values()):
            raise AssertionError(f"{case.name} returned noninteger or negative counters")
        if counters != expected_counters(expected_stage, expected_sign):
            raise AssertionError(f"{case.name} returned counters inconsistent with its decision")
        stages[expected_stage] += 1
    return stages


def audit_historical_batch(executable: Path, timeout: int) -> None:
    sentinels = tuple(
        next(case for case in CASES if case.predicate == predicate)
        for predicate in (
            "compare_squared_distances",
            "orientation_3d",
            "power_bisector_side",
        )
    )
    completed = run(
        executable,
        ["--batch"],
        "".join(f"{case.command()}\n" for case in sentinels),
        timeout=timeout,
    )
    if completed.returncode != 0 or completed.stderr:
        raise AssertionError("historical rich batch no longer succeeds")
    expected_fields = {
        "compare_squared_distances": {
            "certification_stage",
            "counters",
            "left_squared_distance",
            "predicate",
            "right_squared_distance",
            "sign",
        },
        "orientation_3d": {
            "certification_stage",
            "counters",
            "determinant_exact",
            "predicate",
            "sign",
        },
        "power_bisector_side": {
            "affine_value_exact",
            "certification_stage",
            "counters",
            "delta_coordinate_sum_exact",
            "delta_squared_norm_sum_exact",
            "predicate",
            "sign",
        },
    }
    lines = completed.stdout.splitlines()
    if len(lines) != len(sentinels):
        raise AssertionError("historical rich batch changed its row count")
    for case, line in zip(sentinels, lines):
        observed = json.loads(line, object_pairs_hook=reject_duplicate_keys)
        if set(observed) != expected_fields[case.predicate]:
            raise AssertionError(f"historical rich output changed for {case.predicate}")
        individual = run(
            executable,
            case.command().split(),
            "",
            timeout=timeout,
        )
        if (
            individual.returncode != 0
            or individual.stderr
            or individual.stdout != f"{line}\n"
        ):
            raise AssertionError(
                "historical rich batch differs from the individual replay for "
                f"{case.predicate}"
            )


def expect_closed_failure(
    executable: Path,
    arguments: Sequence[str],
    corpus: str,
    required_diagnostic: str,
    timeout: int,
) -> None:
    completed = run(executable, arguments, corpus, timeout=timeout)
    if (
        completed.returncode != 2
        or completed.stdout
        or not completed.stderr.startswith("predicate replay failed closed:")
        or required_diagnostic not in completed.stderr
    ):
        raise AssertionError(
            "decision-only failure did not fail closed: "
            f"args={arguments!r}, stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )


def audit_failures(executable: Path, timeout: int) -> None:
    expect_closed_failure(
        executable,
        ["--decision-only", *CASES[0].command().split()],
        "",
        "--decision-only requires --batch",
        timeout,
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch", "unexpected"],
        "",
        "--batch does not accept trailing arguments",
        timeout,
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch"],
        "compare_exact_levels 1 1 1 1\n",
        "unsupported in decision-only batch mode",
        timeout,
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch"],
        "compare_squared_distances 0000000000000000\n",
        "unsupported in decision-only batch mode",
        timeout,
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch"],
        "\n",
        "batch replay line 1 is empty",
        timeout,
    )
    nonfinite = Case(
        "nonfinite",
        "compare_squared_distances",
        ((NAN, ZERO, ZERO), E1, (TWO, ZERO, ZERO)),
        "cpu_multiprecision",
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch"],
        f"{nonfinite.command()}\n",
        "must be finite",
        timeout,
    )
    expect_closed_failure(
        executable,
        ["--decision-only", "--batch"],
        f"{CASES[0].command()}\ncompare_exact_levels 1 1 1 1\n",
        "batch replay line 2",
        timeout,
    )


def audit_usage(executable: Path, timeout: int) -> None:
    completed = run(executable, [], "", timeout=timeout)
    if (
        completed.returncode != 2
        or completed.stdout
        or "--decision-only --batch < predicate-lines.txt" not in completed.stderr
    ):
        raise AssertionError("native usage omits the decision-only batch interface")


def positive_timeout(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("--timeout-seconds", type=positive_timeout, default=30)
    arguments = parser.parse_args()

    if not arguments.native_replay.is_file():
        raise SystemExit(f"native replay does not exist: {arguments.native_replay}")

    adaptive_stages = audit_batch(
        arguments.native_replay, False, arguments.timeout_seconds
    )
    exact_stages = audit_batch(
        arguments.native_replay, True, arguments.timeout_seconds
    )
    audit_historical_batch(arguments.native_replay, arguments.timeout_seconds)
    audit_failures(arguments.native_replay, arguments.timeout_seconds)
    audit_usage(arguments.native_replay, arguments.timeout_seconds)
    print(
        canonical_json(
            {
                "adaptive_stage_counts": adaptive_stages,
                "case_count": len(CASES),
                "closed_failure_count": 7,
                "historical_sentinel_count": 3,
                "multiprecision_stage_counts": exact_stages,
                "predicate_case_counts": {
                    predicate: sum(case.predicate == predicate for case in CASES)
                    for predicate in (
                        "compare_squared_distances",
                        "orientation_3d",
                        "power_bisector_side",
                    )
                },
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
