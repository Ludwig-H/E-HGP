#!/usr/bin/env python3
"""Audit exact circumcenters with a deterministic independent Fraction oracle."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Sequence


SEED = 0x43454E5445523356
DEFAULT_UNIQUE_BASES = 64
DEFAULT_DEPENDENT_BASES = 32
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "7277882dcdf66ba798087c19284283600821d56a4e5b60c723558a73224ea0cf"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "3dfdd2b9727b5db7b7eccd1110bbdd1367deb0abaaf713c9c1b7e09c720d214f"
)
UINT64_MASK = (1 << 64) - 1

Point = tuple[Fraction, Fraction, Fraction]


class StableGenerator:
    """Versioned SplitMix64 generator with platform-independent selection."""

    def __init__(self, seed: int) -> None:
        self.state = seed & UINT64_MASK

    def next_u64(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & UINT64_MASK
        value = self.state
        value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & UINT64_MASK
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & UINT64_MASK
        return (value ^ (value >> 31)) & UINT64_MASK

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


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(point: Point, factor: Fraction | int) -> Point:
    return tuple(value * factor for value in point)  # type: ignore[return-value]


def dot(left: Sequence[Fraction], right: Sequence[Fraction]) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def cross(left: Point, right: Point) -> Point:
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def determinant3(rows: Sequence[Sequence[Fraction]]) -> Fraction:
    return (
        rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1])
        - rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0])
        + rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0])
    )


def rref(matrix: list[list[Fraction]]) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [list(row) for row in matrix]
    pivots: list[int] = []
    pivot_row = 0
    column_count = len(reduced[0]) if reduced else 0
    for column in range(column_count):
        selected = next(
            (row for row in range(pivot_row, len(reduced)) if reduced[row][column]),
            None,
        )
        if selected is None:
            continue
        reduced[pivot_row], reduced[selected] = reduced[selected], reduced[pivot_row]
        pivot = reduced[pivot_row][column]
        reduced[pivot_row] = [entry / pivot for entry in reduced[pivot_row]]
        for row in range(len(reduced)):
            if row == pivot_row or not reduced[row][column]:
                continue
            factor = reduced[row][column]
            reduced[row] = [
                entry - factor * pivot_entry
                for entry, pivot_entry in zip(reduced[row], reduced[pivot_row])
            ]
        pivots.append(column)
        pivot_row += 1
        if pivot_row == len(reduced):
            break
    return reduced, pivots


def matrix_rank(matrix: list[list[Fraction]]) -> int:
    return len(rref(matrix)[1])


def solve_square(matrix: list[list[Fraction]], right: list[Fraction]) -> list[Fraction]:
    augmented = [row + [value] for row, value in zip(matrix, right)]
    reduced, pivots = rref(augmented)
    size = len(matrix)
    if pivots[:size] != list(range(size)):
        raise AssertionError("the independent Gram system is unexpectedly singular")
    return [reduced[row][size] for row in range(size)]


def squared_distance(left: Point, right: Point) -> Fraction:
    return sum(((a - b) ** 2 for a, b in zip(left, right)), Fraction())


def rational3_record(point: Point) -> dict[str, str]:
    denominator = math.lcm(*(coordinate.denominator for coordinate in point))
    numerators = [
        coordinate.numerator * (denominator // coordinate.denominator)
        for coordinate in point
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


def level_record(level: Fraction) -> dict[str, str]:
    if level < 0:
        raise AssertionError("a squared level cannot be negative")
    return {
        "denominator": str(level.denominator),
        "numerator": str(level.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def support_oracle(points: Sequence[Point]) -> dict[str, object]:
    if not 2 <= len(points) <= 4:
        raise ValueError("a center support must contain two, three or four points")
    vectors = [subtract(point, points[0]) for point in points[1:]]
    affine_dimension = matrix_rank([list(vector) for vector in vectors])
    if affine_dimension != len(points) - 1:
        return {
            "affine_dimension": affine_dimension,
            "center_exact": None,
            "predicate": "circumcenter_support",
            "squared_level_exact": None,
            "support_kind": "affinely_dependent",
            "support_size": len(points),
        }

    gram = [[dot(left, right) for right in vectors] for left in vectors]
    coefficients = solve_square(
        gram, [dot(vector, vector) / 2 for vector in vectors]
    )
    center = points[0]
    for coefficient, vector in zip(coefficients, vectors):
        center = add(center, multiply(vector, coefficient))
    level = squared_distance(center, points[0])
    if any(squared_distance(center, point) != level for point in points[1:]):
        raise AssertionError("the independent Gram oracle produced unequal distances")
    offset = subtract(center, points[0])
    if matrix_rank([list(vector) for vector in vectors] + [list(offset)]) != affine_dimension:
        raise AssertionError("the Gram center left the exact affine hull")
    return {
        "affine_dimension": affine_dimension,
        "center_exact": rational3_record(center),
        "predicate": "circumcenter_support",
        "squared_level_exact": level_record(level),
        "support_kind": "affinely_independent",
        "support_size": len(points),
    }


def word_from_fraction(value: Fraction) -> str:
    encoded = float(value)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != value:
        raise ValueError("a generated coordinate is not exactly representable as binary64")
    return struct.pack(">d", encoded).hex()


def command_for(points: Sequence[Point]) -> str:
    words = [word_from_fraction(value) for point in points for value in point]
    return " ".join(["circumcenter_support", str(len(points)), *words])


@dataclass(frozen=True)
class Case:
    points: tuple[Point, ...]
    command: str
    expected: dict[str, object]
    family: str
    metamorphism: str


def make_case(points: Sequence[Point], family: str, metamorphism: str) -> Case:
    immutable = tuple(points)
    return Case(
        immutable,
        command_for(immutable),
        support_oracle(immutable),
        family,
        metamorphism,
    )


def random_point(generator: StableGenerator, radius: int = 16) -> Point:
    return tuple(
        Fraction(generator.randint(-radius, radius)) for _ in range(3)
    )  # type: ignore[return-value]


def random_nonzero_vector(generator: StableGenerator, radius: int = 7) -> Point:
    while True:
        vector = random_point(generator, radius)
        if vector != (0, 0, 0):
            return vector


def independent_triangle(generator: StableGenerator) -> tuple[Point, Point, Point]:
    origin = random_point(generator)
    first = random_nonzero_vector(generator)
    while True:
        second = random_nonzero_vector(generator)
        if cross(first, second) != (0, 0, 0):
            return origin, add(origin, first), add(origin, second)


def independent_tetrahedron(
    generator: StableGenerator,
) -> tuple[Point, Point, Point, Point]:
    origin = random_point(generator)
    while True:
        first = random_nonzero_vector(generator)
        second = random_nonzero_vector(generator)
        third = random_nonzero_vector(generator)
        if determinant3((first, second, third)):
            return (
                origin,
                add(origin, first),
                add(origin, second),
                add(origin, third),
            )


def translate(points: Sequence[Point], delta: Point) -> tuple[Point, ...]:
    return tuple(add(point, delta) for point in points)


def signed_axes(points: Sequence[Point]) -> tuple[Point, ...]:
    return tuple((point[2], -point[0], point[1]) for point in points)


def scale(points: Sequence[Point], factor: Fraction | int) -> tuple[Point, ...]:
    return tuple(multiply(point, factor) for point in points)


def permute(points: Sequence[Point], order: Sequence[int]) -> tuple[Point, ...]:
    return tuple(points[index] for index in order)


def unique_cases(base_count: int) -> list[Case]:
    cases: list[Case] = []
    pair_generator = StableGenerator(SEED ^ 0x50414952)
    triangle_generator = StableGenerator(SEED ^ 0x545249414E474C45)
    tetra_generator = StableGenerator(SEED ^ 0x5445545241)
    translation = (Fraction(8), Fraction(-4), Fraction(2))
    for _ in range(base_count):
        origin = random_point(pair_generator)
        pair = (origin, add(origin, random_nonzero_vector(pair_generator)))
        pair_variants = (
            ("identity", pair),
            ("point_permutation", permute(pair, (1, 0))),
            ("translation", translate(pair, translation)),
            ("signed_axis_permutation", signed_axes(pair)),
            ("power_of_two_scale", scale(pair, 2)),
        )
        cases.extend(make_case(points, "unique_pair", name) for name, points in pair_variants)

        triangle = independent_triangle(triangle_generator)
        triangle_variants = (
            ("identity", triangle),
            ("cyclic_point_permutation", permute(triangle, (1, 2, 0))),
            ("odd_point_permutation", permute(triangle, (1, 0, 2))),
            ("translation", translate(triangle, translation)),
            ("signed_axis_permutation", signed_axes(triangle)),
            ("negative_power_of_two_scale", scale(triangle, Fraction(1, 2))),
        )
        cases.extend(
            make_case(points, "unique_triangle", name)
            for name, points in triangle_variants
        )

        tetrahedron = independent_tetrahedron(tetra_generator)
        tetra_variants = (
            ("identity", tetrahedron),
            ("point_swap_01", permute(tetrahedron, (1, 0, 2, 3))),
            ("point_cycle", permute(tetrahedron, (1, 2, 3, 0))),
            ("point_reversal", permute(tetrahedron, (3, 2, 1, 0))),
            ("translation", translate(tetrahedron, translation)),
            ("signed_axis_permutation", signed_axes(tetrahedron)),
            ("power_of_two_scale", scale(tetrahedron, 2)),
        )
        cases.extend(
            make_case(points, "unique_tetrahedron", name)
            for name, points in tetra_variants
        )
    return cases


def dependent_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x444550454E44454E54)
    cases: list[Case] = []
    translation = (Fraction(-6), Fraction(3), Fraction(9))
    for _ in range(base_count):
        origin = random_point(generator)
        direction = random_nonzero_vector(generator)
        plane_origin, plane_first, plane_second = independent_triangle(generator)
        plane_u = subtract(plane_first, plane_origin)
        plane_v = subtract(plane_second, plane_origin)
        families: tuple[tuple[str, tuple[Point, ...]], ...] = (
            ("dependent_pair_rank0", (origin, origin)),
            ("dependent_triangle_rank0", (origin, origin, origin)),
            (
                "dependent_triangle_rank1",
                (origin, add(origin, direction), add(origin, multiply(direction, 3))),
            ),
            ("dependent_tetrahedron_rank0", (origin, origin, origin, origin)),
            (
                "dependent_tetrahedron_rank1",
                (
                    origin,
                    add(origin, direction),
                    add(origin, multiply(direction, 2)),
                    add(origin, multiply(direction, -1)),
                ),
            ),
            (
                "dependent_tetrahedron_rank2",
                (
                    plane_origin,
                    add(plane_origin, plane_u),
                    add(plane_origin, plane_v),
                    add(add(plane_origin, plane_u), plane_v),
                ),
            ),
        )
        for family, points in families:
            variants = (
                ("identity", points),
                ("translation", translate(points, translation)),
                ("signed_axis_permutation", signed_axes(points)),
            )
            cases.extend(make_case(value, family, name) for name, value in variants)
    return cases


def build_cases(unique_bases: int, dependent_bases: int) -> list[Case]:
    return [*unique_cases(unique_bases), *dependent_cases(dependent_bases)]


def run_batch(executable: Path, corpus: str, arguments: Sequence[str], timeout: int) -> str:
    completed = subprocess.run(
        [str(executable), *arguments, "--batch"],
        input=corpus,
        capture_output=True,
        encoding="utf-8",
        timeout=timeout,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"native center batch {' '.join(arguments) or 'normal'} failed closed: "
            f"{completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("native center batch unexpectedly wrote to stderr")
    return completed.stdout


def audit_output(output: str, cases: Sequence[Case]) -> None:
    lines = output.splitlines(keepends=True)
    if len(lines) != len(cases):
        raise AssertionError(
            f"native center batch returned {len(lines)} lines for {len(cases)} cases"
        )
    for index, (line, case) in enumerate(zip(lines, cases)):
        expected = canonical_json(case.expected) + "\n"
        if line != expected:
            try:
                observed = json.loads(line)
            except json.JSONDecodeError as error:
                raise AssertionError(
                    f"center case {index} returned invalid JSON: {error}"
                ) from error
            raise AssertionError(
                f"center case {index} ({case.family}/{case.metamorphism}) differs "
                f"from the Fraction Gram oracle: expected={expected.strip()}, "
                f"observed={canonical_json(observed)}"
            )


def nonnegative_count(value: str) -> int:
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("case counts must be nonnegative")
    return parsed


def positive_count(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument(
        "--unique-bases", type=nonnegative_count, default=DEFAULT_UNIQUE_BASES
    )
    parser.add_argument(
        "--dependent-bases", type=nonnegative_count, default=DEFAULT_DEPENDENT_BASES
    )
    parser.add_argument("--timeout-seconds", type=positive_count, default=180)
    arguments = parser.parse_args()

    cases = build_cases(arguments.unique_bases, arguments.dependent_bases)
    corpus = "".join(case.command + "\n" for case in cases)
    oracle = "".join(canonical_json(case.expected) + "\n" for case in cases)
    corpus_hash = hashlib.sha256(corpus.encode("ascii")).hexdigest()
    oracle_hash = hashlib.sha256(oracle.encode("utf-8")).hexdigest()
    default_counts = (
        arguments.unique_bases == DEFAULT_UNIQUE_BASES
        and arguments.dependent_bases == DEFAULT_DEPENDENT_BASES
    )
    if (
        default_counts
        and EXPECTED_DEFAULT_CORPUS_SHA256
        and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256
    ):
        raise AssertionError(
            "the default center corpus changed without a generator-version update"
        )
    if (
        default_counts
        and EXPECTED_DEFAULT_ORACLE_SHA256
        and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256
    ):
        raise AssertionError(
            "the default center oracle changed without a generator-version update"
        )

    normal_output = run_batch(arguments.native_replay, corpus, (), arguments.timeout_seconds)
    multiprecision_output = run_batch(
        arguments.native_replay,
        corpus,
        ("--multiprecision-only",),
        arguments.timeout_seconds,
    )
    if normal_output != multiprecision_output:
        raise AssertionError(
            "exact-only center outputs differ under --multiprecision-only"
        )
    audit_output(normal_output, cases)

    size_histogram = {"2": 0, "3": 0, "4": 0}
    kind_histogram = {"affinely_dependent": 0, "affinely_independent": 0}
    dimension_histogram = {"0": 0, "1": 0, "2": 0, "3": 0}
    family_histogram: dict[str, int] = {}
    metamorphism_histogram: dict[str, int] = {}
    for case in cases:
        size_histogram[str(len(case.points))] += 1
        kind = str(case.expected["support_kind"])
        kind_histogram[kind] += 1
        dimension_histogram[str(case.expected["affine_dimension"])] += 1
        family_histogram[case.family] = family_histogram.get(case.family, 0) + 1
        metamorphism_histogram[case.metamorphism] = (
            metamorphism_histogram.get(case.metamorphism, 0) + 1
        )

    print(
        canonical_json(
            {
                "base_case_counts": {
                    "dependent": arguments.dependent_bases,
                    "unique_per_cardinality": arguments.unique_bases,
                },
                "case_count": len(cases),
                "command_corpus_sha256": corpus_hash,
                "family_histogram": family_histogram,
                "generator": "center-dyadic-splitmix64-v1",
                "kind_histogram": kind_histogram,
                "metamorphism_histogram": metamorphism_histogram,
                "multiprecision_only_byte_identical": True,
                "oracle_sha256": oracle_hash,
                "seed": f"0x{SEED:016x}",
                "support_dimension_histogram": dimension_histogram,
                "support_size_histogram": size_histogram,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
