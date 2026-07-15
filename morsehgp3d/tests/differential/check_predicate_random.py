#!/usr/bin/env python3
"""Run a deterministic batched differential corpus against exact predicates."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path
from typing import TextIO


SEED = 0x4D4F525345484750
EXPONENTS = (0, 1, 2, 1022, 1023, 1024, 2045, 2046)
DEFAULT_DISTANCE_CASES = 1024
DEFAULT_ORIENTATION_CASES = 512
DEFAULT_POWER_CASES = 512
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "066f9ee577fc6c6d64ed930df4eaeca88e96895fa801a661980ba22fa98b4be3"
)
UINT64_MASK = (1 << 64) - 1


class StableGenerator:
    """Versioned SplitMix64 generator with stable selection primitives."""

    def __init__(self, seed: int) -> None:
        self.state = seed & UINT64_MASK

    def next_u64(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & UINT64_MASK
        value = self.state
        value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & UINT64_MASK
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & UINT64_MASK
        return (value ^ (value >> 31)) & UINT64_MASK

    def getrandbits(self, width: int) -> int:
        if not 0 <= width <= 64:
            raise ValueError("StableGenerator supports widths from zero through 64")
        return self.next_u64() & ((1 << width) - 1) if width else 0

    def randbelow(self, bound: int) -> int:
        if bound <= 0:
            raise ValueError("StableGenerator bound must be positive")
        limit = (1 << 64) - ((1 << 64) % bound)
        while True:
            value = self.next_u64()
            if value < limit:
                return value % bound

    def randint(self, lower: int, upper: int) -> int:
        if lower > upper:
            raise ValueError("StableGenerator integer range is empty")
        return lower + self.randbelow(upper - lower + 1)

    def choice(self, values: tuple[int, ...]) -> int:
        return values[self.randbelow(len(values))]

    def sample(self, population: range, count: int) -> list[int]:
        values = list(population)
        if not 0 <= count <= len(values):
            raise ValueError("StableGenerator sample size is invalid")
        for index in range(count):
            selected = index + self.randbelow(len(values) - index)
            values[index], values[selected] = values[selected], values[index]
        return values[:count]


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def random_word(generator: StableGenerator) -> str:
    sign_bit = generator.getrandbits(1)
    exponent = generator.choice(EXPONENTS)
    fraction = generator.getrandbits(52)
    bits = (sign_bit << 63) | (exponent << 52) | fraction
    return f"{bits:016x}"


def rational(word: str) -> Fraction:
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def point(words: list[str]) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(rational(word) for word in words)  # type: ignore[return-value]


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def distance_difference(points: list[tuple[Fraction, Fraction, Fraction]]) -> Fraction:
    witness, left, right = points
    left_distance = sum(((a - b) ** 2 for a, b in zip(witness, left)), Fraction())
    right_distance = sum(((a - b) ** 2 for a, b in zip(witness, right)), Fraction())
    return left_distance - right_distance


def orientation(points: list[tuple[Fraction, Fraction, Fraction]]) -> Fraction:
    a, b, c, d = points
    u = tuple(bi - ai for ai, bi in zip(a, b))
    v = tuple(ci - ai for ai, ci in zip(a, c))
    w = tuple(di - ai for ai, di in zip(a, d))
    return (
        u[0] * (v[1] * w[2] - v[2] * w[1])
        - u[1] * (v[0] * w[2] - v[2] * w[0])
        + u[2] * (v[0] * w[1] - v[1] * w[0])
    )


def generated_points(generator: StableGenerator, count: int) -> list[list[str]]:
    return [[random_word(generator) for _ in range(3)] for _ in range(count)]


ZERO = "0000000000000000"
ONE = "3ff0000000000000"
ONE_ULP_BELOW = "3fefffffffffffff"
MINIMUM_SUBNORMAL = "0000000000000001"
NEGATIVE_MINIMUM_SUBNORMAL = "8000000000000001"


def expansion_fixed_case(predicate: str, case_index: int) -> list[list[str]] | None:
    origin = [ZERO, ZERO, ZERO]
    if predicate == "compare_squared_distances":
        if case_index == 0:
            return [origin, [ONE_ULP_BELOW, ZERO, ZERO], [ONE, ZERO, ZERO]]
        if case_index == 1:
            return [origin, [ONE, ZERO, ZERO], [ONE_ULP_BELOW, ZERO, ZERO]]
        if case_index == 2:
            return [origin, ["bff0000000000000", ZERO, ZERO], [ONE, ZERO, ZERO]]
    if predicate == "orientation_3d":
        first_axis = [ONE, ZERO, ZERO]
        second_axis = [ZERO, ONE, ZERO]
        if case_index == 0:
            return [origin, first_axis, second_axis, [ZERO, ZERO, MINIMUM_SUBNORMAL]]
        if case_index == 1:
            return [
                origin,
                first_axis,
                second_axis,
                [ZERO, ZERO, NEGATIVE_MINIMUM_SUBNORMAL],
            ]
        if case_index == 2:
            return [origin, first_axis, second_axis, [ONE, ONE, ZERO]]
    return None


def generated_fixed_case(
    generator: StableGenerator, predicate: str, case_index: int, point_count: int
) -> list[list[str]]:
    targeted = expansion_fixed_case(predicate, case_index)
    return targeted if targeted is not None else generated_points(generator, point_count)


def generated_power_case(
    generator: StableGenerator, case_index: int,
) -> tuple[list[list[str]], list[int], list[int], tuple[int, int, int, int]]:
    if case_index == 0:
        return (
            [[ONE, ZERO, ZERO], ["4000000000000000", ZERO, ZERO]],
            [1],
            [0],
            (3, 0, 0, 2),
        )
    if case_index == 1:
        return (
            [[ONE_ULP_BELOW, ZERO, ZERO], [ONE, ZERO, ZERO]],
            [0],
            [1],
            (0, 0, 0, 1),
        )
    if case_index == 2:
        return (
            [[ONE_ULP_BELOW, ZERO, ZERO], [ONE, ZERO, ZERO]],
            [1],
            [0],
            (0, 0, 0, 1),
        )
    point_count = generator.randint(2, 8)
    point_words = generated_points(generator, point_count)
    cardinality = generator.randint(1, min(4, point_count))
    r_ids = sorted(generator.sample(range(point_count), cardinality))
    q_ids = sorted(generator.sample(range(point_count), cardinality))
    denominator = generator.choice((1, 3, 5, 7))
    numerators = [generator.randint(-17, 17) for _ in range(3)]
    divisor = denominator
    for numerator in numerators:
        divisor = math.gcd(divisor, abs(numerator))
    numerators = [numerator // divisor for numerator in numerators]
    denominator //= divisor
    return point_words, r_ids, q_ids, (
        numerators[0], numerators[1], numerators[2], denominator
    )


def fixed_command(predicate: str, point_words: list[list[str]]) -> str:
    return " ".join(
        [predicate, *(word for point_words_item in point_words for word in point_words_item)]
    )


def power_command(
    point_words: list[list[str]],
    r_ids: list[int],
    q_ids: list[int],
    witness: tuple[int, int, int, int],
) -> str:
    x_numerator, y_numerator, z_numerator, denominator = witness
    return " ".join(
        [
            "power_bisector_side",
            str(x_numerator),
            str(y_numerator),
            str(z_numerator),
            str(denominator),
            str(len(point_words)),
            *(word for point_words_item in point_words for word in point_words_item),
            str(len(r_ids)),
            *(str(identifier) for identifier in r_ids),
            str(len(q_ids)),
            *(str(identifier) for identifier in q_ids),
        ]
    )


def expected_counters(expected: Fraction, stage: str) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 1 if stage == "cpu_multiprecision" else 0,
        "exact_zeros": 1 if expected == 0 else 0,
        "expansion_certified": 1 if stage == "expansion" else 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 1 if stage == "fp64_filtered" else 0,
        "remaining_unknown": 0,
    }


NATIVE_FIELDS = {
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


def exact_rational3_record(
    coordinates: tuple[Fraction, Fraction, Fraction],
) -> dict[str, str]:
    denominator = math.lcm(*(coordinate.denominator for coordinate in coordinates))
    numerators = [
        coordinate.numerator * (denominator // coordinate.denominator)
        for coordinate in coordinates
    ]
    divisor = denominator
    for numerator in numerators:
        divisor = math.gcd(divisor, abs(numerator))
    denominator //= divisor
    numerators = [numerator // divisor for numerator in numerators]
    return {
        "denominator": str(denominator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit",
        "x_numerator": str(numerators[0]),
        "y_numerator": str(numerators[1]),
        "z_numerator": str(numerators[2]),
    }


def read_result(output: TextIO, predicate: str, case_index: int) -> dict:
    line = output.readline()
    if not line:
        raise AssertionError(f"batch output ended before {predicate} case {case_index}")
    observed = json.loads(line)
    if line != canonical_json(observed) + "\n":
        raise AssertionError(f"{predicate} case {case_index} is not canonical JSON")
    if not isinstance(observed, dict) or set(observed) != NATIVE_FIELDS[predicate]:
        raise AssertionError(f"{predicate} case {case_index} has an open native schema")
    if observed.get("predicate") != predicate:
        raise AssertionError(f"{predicate} case {case_index} returned another predicate")
    return observed


def validate_stage(
    observed: dict,
    expected: Fraction,
    predicate: str,
    case_index: int,
    *,
    multiprecision_only: bool,
) -> str:
    stage = observed["certification_stage"]
    allowed = (
        {"cpu_multiprecision"}
        if multiprecision_only
        else {"fp64_filtered", "expansion", "cpu_multiprecision"}
    )
    if (
        stage not in allowed
        or (expected == 0 and stage == "fp64_filtered")
        or observed["counters"] != expected_counters(expected, stage)
    ):
        raise AssertionError(f"{predicate} case {case_index} has invalid counters")
    return stage


def scientific_result(observed: dict) -> dict:
    return {
        key: value
        for key, value in observed.items()
        if key not in {"certification_stage", "counters"}
    }


def audit_fixed(
    output: TextIO,
    multiprecision_output: TextIO,
    predicate: str,
    point_count: int,
    cases: int,
    sign_histogram: dict[str, int],
    stage_histogram: dict[str, int],
) -> None:
    generator = StableGenerator(SEED ^ point_count)
    oracle = distance_difference if predicate == "compare_squared_distances" else orientation
    for case_index in range(cases):
        words = generated_fixed_case(generator, predicate, case_index, point_count)
        exact_points = [point(point_words) for point_words in words]
        expected = oracle(exact_points)
        observed = read_result(output, predicate, case_index)
        multiprecision_observed = read_result(
            multiprecision_output, predicate, case_index
        )
        if observed["sign"] != sign(expected):
            raise AssertionError(
                f"{predicate} case {case_index} differs: expected {sign(expected)}, "
                f"observed {observed['sign']}, words={words}"
            )
        stage = validate_stage(
            observed,
            expected,
            predicate,
            case_index,
            multiprecision_only=False,
        )
        validate_stage(
            multiprecision_observed,
            expected,
            predicate,
            case_index,
            multiprecision_only=True,
        )
        if scientific_result(observed) != scientific_result(multiprecision_observed):
            raise AssertionError(
                f"{predicate} case {case_index} differs with the FP64 filter disabled"
            )
        sign_histogram[sign(expected)] += 1
        stage_histogram[stage] += 1
        if predicate == "compare_squared_distances":
            witness, left, right = exact_points
            left_level = sum(((a - b) ** 2 for a, b in zip(witness, left)), Fraction())
            right_level = sum(((a - b) ** 2 for a, b in zip(witness, right)), Fraction())
            if observed["left_squared_distance"] != {
                "denominator": str(left_level.denominator),
                "numerator": str(left_level.numerator),
                "schema_version": "2.0.0",
                "unit": "input_coordinate_unit_squared",
            } or observed["right_squared_distance"] != {
                "denominator": str(right_level.denominator),
                "numerator": str(right_level.numerator),
                "schema_version": "2.0.0",
                "unit": "input_coordinate_unit_squared",
            }:
                raise AssertionError(f"{predicate} case {case_index} has invalid exact levels")
        elif observed["determinant_exact"] != {
            "denominator": str(expected.denominator),
            "numerator": str(expected.numerator),
        }:
            raise AssertionError(f"{predicate} case {case_index} has an invalid determinant")


def audit_power(
    output: TextIO,
    multiprecision_output: TextIO,
    cases: int,
    sign_histogram: dict[str, int],
    stage_histogram: dict[str, int],
) -> None:
    predicate = "power_bisector_side"
    generator = StableGenerator(SEED ^ 0x485251)
    for case_index in range(cases):
        words, r_ids, q_ids, witness_record = generated_power_case(
            generator, case_index
        )
        exact_points = [point(point_words) for point_words in words]
        r_points = [exact_points[index] for index in r_ids]
        q_points = [exact_points[index] for index in q_ids]
        delta = tuple(
            sum((value[axis] for value in r_points), Fraction())
            - sum((value[axis] for value in q_points), Fraction())
            for axis in range(3)
        )
        delta_norm = sum(
            (sum((coordinate * coordinate for coordinate in value), Fraction())
             for value in r_points),
            Fraction(),
        ) - sum(
            (sum((coordinate * coordinate for coordinate in value), Fraction())
             for value in q_points),
            Fraction(),
        )
        witness = tuple(
            Fraction(numerator, witness_record[3]) for numerator in witness_record[:3]
        )
        expected = -2 * sum(
            (coordinate * difference for coordinate, difference in zip(witness, delta)),
            Fraction(),
        ) + delta_norm
        observed = read_result(output, predicate, case_index)
        multiprecision_observed = read_result(
            multiprecision_output, predicate, case_index
        )
        if observed["sign"] != sign(expected):
            raise AssertionError(
                f"{predicate} case {case_index} differs: expected {sign(expected)}, "
                f"observed {observed['sign']}, words={words}, R={r_ids}, Q={q_ids}, "
                f"witness={witness_record}"
            )
        stage = validate_stage(
            observed,
            expected,
            predicate,
            case_index,
            multiprecision_only=False,
        )
        validate_stage(
            multiprecision_observed,
            expected,
            predicate,
            case_index,
            multiprecision_only=True,
        )
        if scientific_result(observed) != scientific_result(multiprecision_observed):
            raise AssertionError(
                f"{predicate} case {case_index} differs with the FP64 filter disabled"
            )
        sign_histogram[sign(expected)] += 1
        stage_histogram[stage] += 1
        if observed["delta_coordinate_sum_exact"] != exact_rational3_record(delta) or observed[
            "delta_squared_norm_sum_exact"
        ] != {
            "denominator": str(delta_norm.denominator),
            "numerator": str(delta_norm.numerator),
        } or observed["affine_value_exact"] != {
            "denominator": str(expected.denominator),
            "numerator": str(expected.numerator),
        }:
            raise AssertionError(f"{predicate} case {case_index} has an invalid exact witness")


def write_corpus(
    stream: TextIO, distance_cases: int, orientation_cases: int, power_cases: int
) -> str:
    digest = hashlib.sha256()

    def write(command: str) -> None:
        encoded = (command + "\n").encode("ascii")
        digest.update(encoded)
        stream.write(encoded.decode("ascii"))

    generator = StableGenerator(SEED ^ 3)
    for case_index in range(distance_cases):
        write(
            fixed_command(
                "compare_squared_distances",
                generated_fixed_case(
                    generator, "compare_squared_distances", case_index, 3
                ),
            )
        )
    generator = StableGenerator(SEED ^ 4)
    for case_index in range(orientation_cases):
        write(
            fixed_command(
                "orientation_3d",
                generated_fixed_case(generator, "orientation_3d", case_index, 4),
            )
        )
    generator = StableGenerator(SEED ^ 0x485251)
    for case_index in range(power_cases):
        write(power_command(*generated_power_case(generator, case_index)))
    stream.flush()
    stream.seek(0)
    return digest.hexdigest()


def nonnegative_count(value: str) -> int:
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("case counts must be nonnegative")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument(
        "--distance-cases", type=nonnegative_count, default=DEFAULT_DISTANCE_CASES
    )
    parser.add_argument(
        "--orientation-cases", type=nonnegative_count, default=DEFAULT_ORIENTATION_CASES
    )
    parser.add_argument("--power-cases", type=nonnegative_count, default=DEFAULT_POWER_CASES)
    parser.add_argument("--timeout-seconds", type=nonnegative_count, default=180)
    arguments = parser.parse_args()

    with tempfile.TemporaryFile(mode="w+", encoding="ascii") as batch_input, tempfile.TemporaryFile(
        mode="w+", encoding="utf-8"
    ) as batch_output, tempfile.TemporaryFile(
        mode="w+", encoding="utf-8"
    ) as multiprecision_output:
        corpus_hash = write_corpus(
            batch_input,
            arguments.distance_cases,
            arguments.orientation_cases,
            arguments.power_cases,
        )
        if (
            arguments.distance_cases == DEFAULT_DISTANCE_CASES
            and arguments.orientation_cases == DEFAULT_ORIENTATION_CASES
            and arguments.power_cases == DEFAULT_POWER_CASES
            and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256
        ):
            raise AssertionError(
                "the default predicate corpus changed without a generator version update"
            )
        subprocess.run(
            [str(arguments.native_replay), "--batch"],
            check=True,
            stdin=batch_input,
            stdout=batch_output,
            encoding="utf-8",
            timeout=arguments.timeout_seconds,
        )
        batch_input.seek(0)
        subprocess.run(
            [str(arguments.native_replay), "--multiprecision-only", "--batch"],
            check=True,
            stdin=batch_input,
            stdout=multiprecision_output,
            encoding="utf-8",
            timeout=arguments.timeout_seconds,
        )
        batch_output.seek(0)
        multiprecision_output.seek(0)
        histogram = {"negative": 0, "positive": 0, "zero": 0}
        stage_histogram = {
            "cpu_multiprecision": 0,
            "expansion": 0,
            "fp64_filtered": 0,
        }
        stage_histogram_by_predicate: dict[str, dict[str, int]] = {}
        stage_before = dict(stage_histogram)
        audit_fixed(
            batch_output,
            multiprecision_output,
            "compare_squared_distances",
            3,
            arguments.distance_cases,
            histogram,
            stage_histogram,
        )
        stage_histogram_by_predicate["compare_squared_distances"] = {
            stage: stage_histogram[stage] - stage_before[stage]
            for stage in stage_histogram
        }
        stage_before = dict(stage_histogram)
        audit_fixed(
            batch_output,
            multiprecision_output,
            "orientation_3d",
            4,
            arguments.orientation_cases,
            histogram,
            stage_histogram,
        )
        stage_histogram_by_predicate["orientation_3d"] = {
            stage: stage_histogram[stage] - stage_before[stage]
            for stage in stage_histogram
        }
        filterable_stage_histogram = dict(stage_histogram)
        stage_before = dict(stage_histogram)
        audit_power(
            batch_output,
            multiprecision_output,
            arguments.power_cases,
            histogram,
            stage_histogram,
        )
        stage_histogram_by_predicate["power_bisector_side"] = {
            stage: stage_histogram[stage] - stage_before[stage]
            for stage in stage_histogram
        }
        if batch_output.readline():
            raise AssertionError("batch replay returned more results than requested")
        if multiprecision_output.readline():
            raise AssertionError(
                "multiprecision batch replay returned more results than requested"
            )
        for predicate, cases in (
            ("compare_squared_distances", arguments.distance_cases),
            ("orientation_3d", arguments.orientation_cases),
            ("power_bisector_side", arguments.power_cases),
        ):
            predicate_stages = stage_histogram_by_predicate[predicate]
            default_cases = {
                "compare_squared_distances": DEFAULT_DISTANCE_CASES,
                "orientation_3d": DEFAULT_ORIENTATION_CASES,
                "power_bisector_side": DEFAULT_POWER_CASES,
            }[predicate]
            if cases == default_cases and (
                predicate_stages["fp64_filtered"] == 0
                or predicate_stages["expansion"] == 0
                or predicate_stages["cpu_multiprecision"] == 0
            ):
                raise AssertionError(
                    f"the {predicate} corpus did not exercise all adaptive stages"
                )

    print(
        canonical_json(
            {
                "case_count": (
                    arguments.distance_cases
                    + arguments.orientation_cases
                    + arguments.power_cases
                ),
                "corpus_sha256": corpus_hash,
                "generator": "mixed-binary64-splitmix64-v2",
                "seed": f"0x{SEED:016x}",
                "sign_histogram": histogram,
                "stage_histogram": stage_histogram,
                "filterable_stage_histogram": filterable_stage_histogram,
                "stage_histogram_by_predicate": stage_histogram_by_predicate,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
