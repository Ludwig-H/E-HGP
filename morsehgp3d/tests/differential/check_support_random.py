#!/usr/bin/env python3
"""Audit v5 support analysis and sphere sides with an independent exact oracle."""

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


SEED = 0x535550504F525435
DEFAULT_SUPPORT_BASES = 24
DEFAULT_SPHERE_BASES = 64
DEFAULT_VARIED_SUPPORTS_PER_BUCKET = 32
DEFAULT_VARIED_SPHERE_BASES = 48
EXPECTED_DEFAULT_CASE_COUNT = 2128
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "0d5f30c7067c253b6e3c2cc36a530fad9a3f83e526b7f35fe23543ad1508a68c"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "7d10f2cb87398847abcfae505c1365072d48786212b25159d8154f92d65f88d9"
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
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(point: Point, factor: Fraction | int) -> Point:
    return tuple(value * factor for value in point)  # type: ignore[return-value]


def dot(left: Sequence[Fraction], right: Sequence[Fraction]) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def squared_distance(left: Point, right: Point) -> Fraction:
    return dot(subtract(left, right), subtract(left, right))


def rref(matrix: Sequence[Sequence[Fraction]]) -> tuple[list[list[Fraction]], list[int]]:
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


def matrix_rank(matrix: Sequence[Sequence[Fraction]]) -> int:
    return len(rref(matrix)[1])


def solve_square(
    matrix: Sequence[Sequence[Fraction]], right_hand_side: Sequence[Fraction]
) -> list[Fraction]:
    size = len(matrix)
    if size != len(right_hand_side) or any(len(row) != size for row in matrix):
        raise ValueError("the Fraction oracle requires a square system")
    augmented = [
        [*row, right_hand_side[index]] for index, row in enumerate(matrix)
    ]
    reduced, pivots = rref(augmented)
    if pivots[:size] != list(range(size)):
        raise AssertionError("an independent support produced a singular Gram system")
    return [reduced[row][size] for row in range(size)]


def sign_name(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def counters(decision_count: int, zero_count: int) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": decision_count,
        "exact_zeros": zero_count,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def rational_record(value: Fraction) -> dict[str, str]:
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
    }


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
        raise AssertionError("a squared sphere level cannot be negative")
    return {
        "denominator": str(level.denominator),
        "numerator": str(level.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def circumcenter_and_barycentrics(
    points: Sequence[Point],
) -> tuple[Point, Fraction, tuple[Fraction, ...]]:
    if not 1 <= len(points) <= 4:
        raise ValueError("support size must lie in one through four")
    if len(points) == 1:
        return points[0], Fraction(), (Fraction(1),)

    directions = [subtract(point, points[0]) for point in points[1:]]
    expected_dimension = len(points) - 1
    if matrix_rank(directions) != expected_dimension:
        raise ValueError("an affinely dependent support has no unique circumcenter")
    gram = [[dot(left, right) for right in directions] for left in directions]
    coefficients = solve_square(
        gram, [dot(direction, direction) / 2 for direction in directions]
    )
    center = points[0]
    for coefficient, direction in zip(coefficients, directions):
        center = add(center, multiply(direction, coefficient))
    barycentric = (Fraction(1) - sum(coefficients, Fraction()), *coefficients)
    level = squared_distance(center, points[0])
    if level <= 0:
        raise AssertionError("an independent nonsingleton support has nonpositive radius")
    if any(squared_distance(center, point) != level for point in points[1:]):
        raise AssertionError("the Fraction Gram center is not equidistant")
    reconstructed = tuple(
        sum(
            (barycentric[index] * points[index][axis] for index in range(len(points))),
            Fraction(),
        )
        for axis in range(3)
    )
    if reconstructed != center:
        raise AssertionError("the Fraction barycentrics do not reconstruct the center")
    return center, level, barycentric


def support_analysis_oracle(points: Sequence[Point]) -> dict[str, object]:
    if not 1 <= len(points) <= 4:
        raise ValueError("a support analysis requires one through four points")
    directions = [subtract(point, points[0]) for point in points[1:]]
    affine_dimension = matrix_rank(directions)
    expected_dimension = len(points) - 1
    if affine_dimension != expected_dimension:
        return {
            "affine_dimension": affine_dimension,
            "barycentric_coordinates_exact": None,
            "barycentric_signs": None,
            "center_exact": None,
            "certification_stage": None,
            "convex_hull_location": None,
            "counters": counters(0, 0),
            "predicate": "circumcenter_support_analysis",
            "reduced_support_indices": None,
            "squared_level_exact": None,
            "support_kind": "affinely_dependent",
            "support_size": len(points),
            "support_status": "affinely_dependent",
        }

    center, level, barycentric = circumcenter_and_barycentrics(points)
    signs = [sign_name(coordinate) for coordinate in barycentric]
    zero_count = signs.count("zero")
    if "negative" in signs:
        location = "exterior"
        status = "exterior_circumcenter"
        reduced_support: list[int] | None = None
    elif zero_count:
        location = "relative_boundary"
        status = "boundary_reduced"
        reduced_support = [
            index for index, coordinate in enumerate(barycentric) if coordinate > 0
        ]
        reduced_points = [points[index] for index in reduced_support]
        reduced_center, reduced_level, reduced_barycentric = (
            circumcenter_and_barycentrics(reduced_points)
        )
        if (
            reduced_center != center
            or reduced_level != level
            or any(coordinate <= 0 for coordinate in reduced_barycentric)
        ):
            raise AssertionError("the boundary reduction changed the exact sphere")
    else:
        location = "relative_interior"
        status = "minimal"
        reduced_support = list(range(len(points)))

    return {
        "affine_dimension": affine_dimension,
        "barycentric_coordinates_exact": [
            rational_record(coordinate) for coordinate in barycentric
        ],
        "barycentric_signs": signs,
        "center_exact": rational3_record(center),
        "certification_stage": "cpu_multiprecision",
        "convex_hull_location": location,
        "counters": counters(len(points), zero_count),
        "predicate": "circumcenter_support_analysis",
        "reduced_support_indices": reduced_support,
        "squared_level_exact": level_record(level),
        "support_kind": "affinely_independent",
        "support_size": len(points),
        "support_status": status,
    }


def sphere_side_oracle(center: Point, level: Fraction, point: Point) -> dict[str, object]:
    distance = squared_distance(center, point)
    signed_offset = distance - level
    sign = sign_name(signed_offset)
    classification = {
        "negative": "strictly_inside",
        "zero": "boundary",
        "positive": "outside",
    }[sign]
    return {
        "certification_stage": "cpu_multiprecision",
        "classification": classification,
        "counters": counters(1, 1 if sign == "zero" else 0),
        "predicate": "sphere_side",
        "sign": sign,
        "signed_offset_exact": rational_record(signed_offset),
        "squared_distance_exact": level_record(distance),
    }


def word_from_fraction(value: Fraction) -> str:
    binary64 = float(value)
    if not math.isfinite(binary64) or Fraction.from_float(binary64) != value:
        raise ValueError("a generated coordinate is not exactly binary64 representable")
    return struct.pack(">d", binary64).hex()


def support_command(points: Sequence[Point]) -> str:
    words = [word_from_fraction(value) for point in points for value in point]
    return " ".join(["circumcenter_support_analysis", str(len(points)), *words])


def sphere_command(center: Point, level: Fraction, point: Point) -> str:
    record = rational3_record(center)
    words = [word_from_fraction(value) for value in point]
    return " ".join(
        [
            "sphere_side",
            record["x_numerator"],
            record["y_numerator"],
            record["z_numerator"],
            record["denominator"],
            str(level.numerator),
            str(level.denominator),
            *words,
        ]
    )


@dataclass(frozen=True)
class Similarity:
    axes: tuple[int, int, int]
    signs: tuple[int, int, int]
    factor: Fraction
    translation: Point

    def point(self, point: Point) -> Point:
        return tuple(
            self.translation[axis]
            + self.factor * self.signs[axis] * point[self.axes[axis]]
            for axis in range(3)
        )  # type: ignore[return-value]

    def level(self, level: Fraction) -> Fraction:
        return level * self.factor * self.factor


IDENTITY_SIMILARITY = Similarity(
    (0, 1, 2),
    (1, 1, 1),
    Fraction(1),
    (Fraction(), Fraction(), Fraction()),
)


def shuffled(generator: StableGenerator, values: Sequence[int]) -> tuple[int, ...]:
    result = list(values)
    for index in range(len(result) - 1, 0, -1):
        selected = generator.randbelow(index + 1)
        result[index], result[selected] = result[selected], result[index]
    return tuple(result)


def nontrivial_permutation(generator: StableGenerator, size: int) -> tuple[int, ...]:
    if size == 1:
        return (0,)
    identity = tuple(range(size))
    permutation = shuffled(generator, identity)
    if permutation == identity:
        permutation = (identity[1], identity[0], *identity[2:])
    return permutation


def random_dyadic(generator: StableGenerator, radius: int = 64) -> Fraction:
    """Return a bounded binary64-representable dyadic rational."""

    return Fraction(generator.randint(-radius, radius), 1 << generator.randbelow(5))


def random_dyadic_point(generator: StableGenerator) -> Point:
    return tuple(random_dyadic(generator) for _ in range(3))  # type: ignore[return-value]


def random_nonzero_dyadic_vector(generator: StableGenerator) -> Point:
    while True:
        vector = random_dyadic_point(generator)
        if vector != (0, 0, 0):
            return vector


def random_independent_support(
    generator: StableGenerator, support_size: int
) -> tuple[Point, ...]:
    if support_size not in (3, 4):
        raise ValueError("varied independent supports must have size three or four")
    while True:
        origin = random_dyadic_point(generator)
        directions = tuple(
            random_nonzero_dyadic_vector(generator)
            for _ in range(support_size - 1)
        )
        if matrix_rank(directions) != support_size - 1:
            continue
        return (origin, *(add(origin, direction) for direction in directions))


def random_similarity(generator: StableGenerator) -> Similarity:
    exponent = generator.randint(-2, 2)
    factor = Fraction(1 << exponent) if exponent >= 0 else Fraction(1, 1 << -exponent)
    return Similarity(
        shuffled(generator, (0, 1, 2)),
        tuple(1 if generator.randbelow(2) == 0 else -1 for _ in range(3)),  # type: ignore[arg-type]
        factor,
        tuple(Fraction(generator.randint(-48, 48), 2) for _ in range(3)),  # type: ignore[arg-type]
    )


def transform_points(points: Sequence[Point], similarity: Similarity) -> tuple[Point, ...]:
    return tuple(similarity.point(point) for point in points)


def permute_points(points: Sequence[Point], permutation: Sequence[int]) -> tuple[Point, ...]:
    return tuple(points[index] for index in permutation)


@dataclass(frozen=True)
class Case:
    command: str
    expected: dict[str, object]
    predicate: str
    family: str
    metamorphism: str
    support_size: int | None = None


def make_support_case(
    points: Sequence[Point], family: str, metamorphism: str
) -> Case:
    immutable = tuple(points)
    return Case(
        support_command(immutable),
        support_analysis_oracle(immutable),
        "circumcenter_support_analysis",
        family,
        metamorphism,
        len(immutable),
    )


def make_sphere_case(
    center: Point,
    level: Fraction,
    point: Point,
    family: str,
    metamorphism: str,
) -> Case:
    return Case(
        sphere_command(center, level, point),
        sphere_side_oracle(center, level, point),
        "sphere_side",
        family,
        metamorphism,
    )


SUPPORT_TEMPLATES: tuple[tuple[str, tuple[Point, ...]], ...] = (
    ("minimal_singleton", ((Fraction(2), Fraction(-3), Fraction(5)),)),
    (
        "minimal_pair",
        (
            (Fraction(-1), Fraction(2), Fraction()),
            (Fraction(3), Fraction(-2), Fraction(2)),
        ),
    ),
    (
        "dependent_pair_rank0",
        (
            (Fraction(1), Fraction(-2), Fraction(3)),
            (Fraction(1), Fraction(-2), Fraction(3)),
        ),
    ),
    (
        "minimal_triangle",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(4), Fraction(), Fraction()),
            (Fraction(1), Fraction(3), Fraction()),
        ),
    ),
    (
        "boundary_triangle",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(2), Fraction(), Fraction()),
            (Fraction(), Fraction(2), Fraction()),
        ),
    ),
    (
        "exterior_triangle",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(4), Fraction(), Fraction()),
            (Fraction(1), Fraction(1), Fraction()),
        ),
    ),
    (
        "dependent_triangle_rank1",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(1), Fraction(2), Fraction(1)),
            (Fraction(3), Fraction(6), Fraction(3)),
        ),
    ),
    (
        "minimal_tetrahedron",
        (
            (Fraction(1), Fraction(1), Fraction(1)),
            (Fraction(1), Fraction(-1), Fraction(-1)),
            (Fraction(-1), Fraction(1), Fraction(-1)),
            (Fraction(-1), Fraction(-1), Fraction(1)),
        ),
    ),
    (
        "boundary_edge_tetrahedron",
        (
            (Fraction(1), Fraction(), Fraction()),
            (Fraction(-1), Fraction(), Fraction()),
            (Fraction(), Fraction(1), Fraction()),
            (Fraction(), Fraction(), Fraction(1)),
        ),
    ),
    (
        "boundary_face_tetrahedron",
        (
            (Fraction(5), Fraction(), Fraction()),
            (Fraction(-3), Fraction(4), Fraction()),
            (Fraction(-3), Fraction(-4), Fraction()),
            (Fraction(), Fraction(), Fraction(5)),
        ),
    ),
    (
        "exterior_tetrahedron",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(1), Fraction(), Fraction()),
            (Fraction(), Fraction(1), Fraction()),
            (Fraction(), Fraction(), Fraction(1)),
        ),
    ),
    (
        "dependent_tetrahedron_rank2",
        (
            (Fraction(), Fraction(), Fraction()),
            (Fraction(2), Fraction(), Fraction()),
            (Fraction(), Fraction(2), Fraction()),
            (Fraction(1), Fraction(1), Fraction()),
        ),
    ),
)


def support_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x535550504F5254)
    cases: list[Case] = []
    for family, template in SUPPORT_TEMPLATES:
        for _ in range(base_count):
            base = transform_points(template, random_similarity(generator))
            first_permutation = nontrivial_permutation(generator, len(base))
            transformed = transform_points(base, random_similarity(generator))
            second_permutation = nontrivial_permutation(generator, len(base))
            variants = (
                ("identity", base),
                ("point_permutation", permute_points(base, first_permutation)),
                ("exact_similarity", transformed),
                (
                    "permuted_exact_similarity",
                    permute_points(transformed, second_permutation),
                ),
            )
            cases.extend(
                make_support_case(points, family, metamorphism)
                for metamorphism, points in variants
            )
    return cases


def barycentric_multiset(expected: dict[str, object]) -> tuple[Fraction, ...]:
    records = expected["barycentric_coordinates_exact"]
    if not isinstance(records, list):
        raise AssertionError("an independent varied support omitted barycentrics")
    coordinates: list[Fraction] = []
    for record in records:
        if not isinstance(record, dict):
            raise AssertionError("a barycentric witness is not a rational record")
        coordinates.append(
            Fraction(int(record["numerator"]), int(record["denominator"]))
        )
    return tuple(sorted(coordinates))


def varied_support_bucket(
    support_size: int,
    status: str,
    count: int,
    seed_tag: int,
) -> list[Case]:
    generator = StableGenerator(SEED ^ seed_tag)
    accepted: list[tuple[Point, ...]] = []
    signatures: set[tuple[Fraction, ...]] = set()
    attempt_limit = max(4096, count * 2048)
    attempts = 0
    while len(accepted) < count and attempts < attempt_limit:
        attempts += 1
        points = random_independent_support(generator, support_size)
        expected = support_analysis_oracle(points)
        if expected["support_status"] != status:
            continue
        signature = barycentric_multiset(expected)
        if signature in signatures:
            continue
        signatures.add(signature)
        accepted.append(points)
    if len(accepted) != count:
        raise AssertionError(
            f"varied support bucket size={support_size}/status={status} "
            f"filled {len(accepted)} of {count} cases after {attempts} attempts"
        )

    family = (
        f"varied_triangle_{'minimal' if status == 'minimal' else 'exterior'}"
        if support_size == 3
        else f"varied_tetrahedron_{'minimal' if status == 'minimal' else 'exterior'}"
    )
    cases: list[Case] = []
    for points in accepted:
        permutation = nontrivial_permutation(generator, support_size)
        cases.append(make_support_case(points, family, "random_dyadic"))
        cases.append(
            make_support_case(
                permute_points(points, permutation),
                family,
                "random_dyadic_point_permutation",
            )
        )
    return cases


def varied_support_cases(per_bucket: int) -> list[Case]:
    buckets = (
        (3, "minimal", 0x5452494D494E),
        (3, "exterior_circumcenter", 0x545249455854),
        (4, "minimal", 0x5445544D494E),
        (4, "exterior_circumcenter", 0x544554455854),
    )
    cases: list[Case] = []
    for support_size, status, seed_tag in buckets:
        cases.extend(
            varied_support_bucket(
                support_size, status, per_bucket, seed_tag
            )
        )
    return cases


POSITIVE_SPHERE_CENTER: Point = (
    Fraction(1),
    Fraction(4, 3),
    Fraction(1),
)
POSITIVE_SPHERE_LEVEL = Fraction(25, 9)
POSITIVE_SPHERE_QUERIES: tuple[tuple[str, Point], ...] = (
    ("strictly_inside", (Fraction(1), Fraction(1), Fraction(1))),
    ("boundary", (Fraction(), Fraction(), Fraction(1))),
    ("outside", (Fraction(1), Fraction(4), Fraction(1))),
)
ZERO_SPHERE_CENTER: Point = (Fraction(2), Fraction(-3), Fraction(5))
ZERO_SPHERE_QUERIES: tuple[tuple[str, Point], ...] = (
    ("boundary", ZERO_SPHERE_CENTER),
    ("outside_positive", (Fraction(3), Fraction(-3), Fraction(5))),
    ("outside_negative", (Fraction(2), Fraction(-5), Fraction(5))),
)


def sphere_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x535048455245)
    cases: list[Case] = []
    for base_index in range(base_count):
        zero_radius = base_index % 8 == 0
        if zero_radius:
            center = ZERO_SPHERE_CENTER
            level = Fraction()
            queries = ZERO_SPHERE_QUERIES
            family_prefix = "zero_radius_sphere"
        else:
            center = POSITIVE_SPHERE_CENTER
            level = POSITIVE_SPHERE_LEVEL
            queries = POSITIVE_SPHERE_QUERIES
            family_prefix = "positive_rational_sphere"

        base_similarity = random_similarity(generator)
        base_center = base_similarity.point(center)
        base_level = base_similarity.level(level)
        base_queries = tuple(
            (relation, base_similarity.point(point)) for relation, point in queries
        )
        similarities = (
            ("identity", IDENTITY_SIMILARITY),
            ("exact_similarity", random_similarity(generator)),
            ("second_exact_similarity", random_similarity(generator)),
        )
        for metamorphism, similarity in similarities:
            transformed_center = similarity.point(base_center)
            transformed_level = similarity.level(base_level)
            for relation, query in base_queries:
                transformed_query = similarity.point(query)
                cases.append(
                    make_sphere_case(
                        transformed_center,
                        transformed_level,
                        transformed_query,
                        f"{family_prefix}_{relation}",
                        metamorphism,
                    )
                )
    return cases


def varied_rational_center(generator: StableGenerator) -> Point:
    odd_denominators = (3, 5, 7, 9, 11)
    return tuple(
        Fraction(
            generator.randint(-80, 80),
            odd_denominators[generator.randbelow(len(odd_denominators))],
        )
        for _ in range(3)
    )  # type: ignore[return-value]


def varied_sphere_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x564152535048)
    outside_factors = (Fraction(), Fraction(1, 3), Fraction(1, 2), Fraction(3, 4))
    cases: list[Case] = []
    for _ in range(base_count):
        while True:
            center = varied_rational_center(generator)
            query = random_dyadic_point(generator)
            distance = squared_distance(center, query)
            if distance > 0:
                break
        delta = Fraction(
            generator.randint(1, 31),
            (1 << generator.randbelow(5)) * (3 + 2 * generator.randbelow(5)),
        )
        outside_factor = outside_factors[generator.randbelow(len(outside_factors))]
        variants = (
            ("strictly_inside", distance + delta),
            ("boundary", distance),
            ("outside", distance * outside_factor),
        )
        for relation, level in variants:
            cases.append(
                make_sphere_case(
                    center,
                    level,
                    query,
                    f"varied_rational_sphere_{relation}",
                    "random_rational_parameters",
                )
            )
    return cases


def build_cases(
    support_base_count: int,
    sphere_base_count: int,
    varied_supports_per_bucket: int,
    varied_sphere_base_count: int,
) -> list[Case]:
    return [
        *support_cases(support_base_count),
        *varied_support_cases(varied_supports_per_bucket),
        *sphere_cases(sphere_base_count),
        *varied_sphere_cases(varied_sphere_base_count),
    ]


def run_batch(
    executable: Path, corpus: str, arguments: Sequence[str], timeout: int
) -> str:
    completed = subprocess.run(
        [str(executable), *arguments, "--batch"],
        input=corpus,
        capture_output=True,
        encoding="utf-8",
        timeout=timeout,
    )
    if completed.returncode != 0:
        mode = " ".join(arguments) or "normal"
        raise AssertionError(
            f"native support/sphere batch {mode} failed closed: "
            f"{completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("native support/sphere batch unexpectedly wrote to stderr")
    return completed.stdout


def audit_output(output: str, cases: Sequence[Case]) -> None:
    lines = output.splitlines(keepends=True)
    if len(lines) != len(cases):
        raise AssertionError(
            f"native support/sphere batch returned {len(lines)} lines "
            f"for {len(cases)} cases"
        )
    for index, (line, case) in enumerate(zip(lines, cases)):
        expected = canonical_json(case.expected) + "\n"
        if line == expected:
            continue
        try:
            observed = json.loads(line)
        except json.JSONDecodeError as error:
            raise AssertionError(
                f"v5 random case {index} returned invalid JSON: {error}"
            ) from error
        raise AssertionError(
            f"v5 random case {index} ({case.predicate}/{case.family}/"
            f"{case.metamorphism}) differs from the Fraction/RREF oracle: "
            f"expected={expected.strip()}, observed={canonical_json(observed)}"
        )


def nonnegative_count(value: str) -> int:
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("base counts must be nonnegative")
    return parsed


def positive_count(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def increment(histogram: dict[str, int], key: str) -> None:
    histogram[key] = histogram.get(key, 0) + 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument(
        "--support-bases", type=nonnegative_count, default=DEFAULT_SUPPORT_BASES
    )
    parser.add_argument(
        "--sphere-bases", type=nonnegative_count, default=DEFAULT_SPHERE_BASES
    )
    parser.add_argument(
        "--varied-supports-per-bucket",
        type=nonnegative_count,
        default=DEFAULT_VARIED_SUPPORTS_PER_BUCKET,
    )
    parser.add_argument(
        "--varied-sphere-bases",
        type=nonnegative_count,
        default=DEFAULT_VARIED_SPHERE_BASES,
    )
    parser.add_argument("--timeout-seconds", type=positive_count, default=180)
    arguments = parser.parse_args()

    cases = build_cases(
        arguments.support_bases,
        arguments.sphere_bases,
        arguments.varied_supports_per_bucket,
        arguments.varied_sphere_bases,
    )
    corpus = "".join(case.command + "\n" for case in cases)
    oracle = "".join(canonical_json(case.expected) + "\n" for case in cases)
    corpus_hash = hashlib.sha256(corpus.encode("ascii")).hexdigest()
    oracle_hash = hashlib.sha256(oracle.encode("utf-8")).hexdigest()
    default_counts = (
        arguments.support_bases == DEFAULT_SUPPORT_BASES
        and arguments.sphere_bases == DEFAULT_SPHERE_BASES
        and arguments.varied_supports_per_bucket
        == DEFAULT_VARIED_SUPPORTS_PER_BUCKET
        and arguments.varied_sphere_bases == DEFAULT_VARIED_SPHERE_BASES
    )
    if default_counts and len(cases) != EXPECTED_DEFAULT_CASE_COUNT:
        raise AssertionError(
            "the default v5 case count changed without a generator-version update"
        )
    if (
        default_counts
        and EXPECTED_DEFAULT_CORPUS_SHA256
        and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256
    ):
        raise AssertionError(
            "the default v5 corpus changed without a generator-version update"
        )
    if (
        default_counts
        and EXPECTED_DEFAULT_ORACLE_SHA256
        and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256
    ):
        raise AssertionError(
            "the default v5 oracle changed without a generator-version update"
        )

    normal_output = run_batch(
        arguments.native_replay, corpus, (), arguments.timeout_seconds
    )
    multiprecision_output = run_batch(
        arguments.native_replay,
        corpus,
        ("--multiprecision-only",),
        arguments.timeout_seconds,
    )
    if normal_output != multiprecision_output:
        raise AssertionError(
            "v5 exact outputs differ under --multiprecision-only"
        )
    audit_output(normal_output, cases)

    predicate_histogram: dict[str, int] = {}
    family_histogram: dict[str, int] = {}
    metamorphism_histogram: dict[str, int] = {}
    support_size_histogram = {str(size): 0 for size in range(1, 5)}
    support_dimension_histogram = {str(dimension): 0 for dimension in range(4)}
    support_status_histogram = {
        "affinely_dependent": 0,
        "boundary_reduced": 0,
        "exterior_circumcenter": 0,
        "minimal": 0,
    }
    convex_hull_histogram = {
        "affinely_dependent": 0,
        "exterior": 0,
        "relative_boundary": 0,
        "relative_interior": 0,
    }
    sphere_classification_histogram = {
        "boundary": 0,
        "outside": 0,
        "strictly_inside": 0,
    }
    for case in cases:
        increment(predicate_histogram, case.predicate)
        increment(family_histogram, case.family)
        increment(metamorphism_histogram, case.metamorphism)
        if case.predicate == "circumcenter_support_analysis":
            assert case.support_size is not None
            support_size_histogram[str(case.support_size)] += 1
            dimension = str(case.expected["affine_dimension"])
            support_dimension_histogram[dimension] += 1
            status = str(case.expected["support_status"])
            support_status_histogram[status] += 1
            location = case.expected["convex_hull_location"]
            convex_hull_histogram[
                "affinely_dependent" if location is None else str(location)
            ] += 1
        else:
            classification = str(case.expected["classification"])
            sphere_classification_histogram[classification] += 1

    print(
        canonical_json(
            {
                "base_case_counts": {
                    "sphere": arguments.sphere_bases,
                    "support_per_family": arguments.support_bases,
                    "varied_sphere": arguments.varied_sphere_bases,
                    "varied_support_per_bucket": arguments.varied_supports_per_bucket,
                },
                "case_count": len(cases),
                "command_corpus_sha256": corpus_hash,
                "convex_hull_histogram": convex_hull_histogram,
                "family_histogram": family_histogram,
                "generator": "support-sphere-dyadic-splitmix64-v2",
                "metamorphism_histogram": metamorphism_histogram,
                "multiprecision_only_byte_identical": True,
                "oracle_sha256": oracle_hash,
                "predicate_histogram": predicate_histogram,
                "seed": f"0x{SEED:016x}",
                "sphere_classification_histogram": sphere_classification_histogram,
                "support_dimension_histogram": support_dimension_histogram,
                "support_size_histogram": support_size_histogram,
                "support_status_histogram": support_status_histogram,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
