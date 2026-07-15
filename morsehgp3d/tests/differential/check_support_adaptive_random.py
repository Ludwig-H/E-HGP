#!/usr/bin/env python3
"""Exercise the four v7 support predicates against independent Fraction oracles."""

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
from typing import Iterable, Sequence

SEED = 0x3241384253555050
RANDOM_BUNDLES_PER_SIZE = 224
METAMORPHIC_BASES_PER_SIZE = 12
EXPECTED_DEFAULT_CASE_COUNT = 4050
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "229c59e99a1d7d2404ec90cd8d30e430a0650f4972d8ce07f4dc5b2f0eb6019f"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "3f4094fe7768dcf14e3ad81d3e96ff529eb38cb0e8f1daceeb15a56c5b6d0ca1"
)
UINT64_MASK = (1 << 64) - 1
STAGES = ("fp64_filtered", "expansion", "cpu_multiprecision")
SIGNS = ("negative", "zero", "positive")
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}

WordPoint = tuple[str, str, str]
ExactPoint = tuple[Fraction, Fraction, Fraction]


class StableGenerator:
    """Versioned SplitMix64 generator independent of Python's random module."""

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

    def shuffle(self, values: list[WordPoint]) -> None:
        for index in range(len(values) - 1, 0, -1):
            selected = self.randbelow(index + 1)
            values[index], values[selected] = values[selected], values[index]


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def word(value: float) -> str:
    if not math.isfinite(value):
        raise ValueError("the differential corpus only accepts finite binary64")
    return struct.pack(">d", value).hex()


def point_words(x: float, y: float = 0.0, z: float = 0.0) -> WordPoint:
    return word(x), word(y), word(z)


def integer_point(values: Sequence[int]) -> WordPoint:
    if len(values) != 3:
        raise ValueError("a point requires three coordinates")
    return point_words(float(values[0]), float(values[1]), float(values[2]))


def decode_word(value: str) -> Fraction:
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(value))[0])


def decode_point(value: WordPoint) -> ExactPoint:
    return tuple(decode_word(coordinate) for coordinate in value)  # type: ignore[return-value]


def flatten_points(points: Sequence[WordPoint]) -> list[str]:
    return [coordinate for point in points for coordinate in point]


def vector_subtract(left: ExactPoint, right: ExactPoint) -> ExactPoint:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def dot(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    difference = vector_subtract(left, right)
    return dot(difference, difference)


def matrix_rank(matrix: Sequence[Sequence[Fraction]]) -> int:
    reduced = [list(row) for row in matrix]
    if not reduced:
        return 0
    row_count = len(reduced)
    column_count = len(reduced[0])
    pivot_row = 0
    for column in range(column_count):
        pivot = next(
            (row for row in range(pivot_row, row_count) if reduced[row][column]),
            None,
        )
        if pivot is None:
            continue
        reduced[pivot_row], reduced[pivot] = reduced[pivot], reduced[pivot_row]
        pivot_value = reduced[pivot_row][column]
        reduced[pivot_row] = [value / pivot_value for value in reduced[pivot_row]]
        for row in range(row_count):
            if row == pivot_row or not reduced[row][column]:
                continue
            factor = reduced[row][column]
            reduced[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(reduced[row], reduced[pivot_row])
            ]
        pivot_row += 1
        if pivot_row == row_count:
            break
    return pivot_row


def solve(
    matrix: Sequence[Sequence[Fraction]], rhs: Sequence[Fraction]
) -> list[Fraction]:
    size = len(matrix)
    if len(rhs) != size or any(len(row) != size for row in matrix):
        raise ValueError("the exact linear system is not square")
    augmented = [list(row) + [rhs[index]] for index, row in enumerate(matrix)]
    for column in range(size):
        pivot = next(
            (row for row in range(column, size) if augmented[row][column]), None
        )
        if pivot is None:
            raise ValueError("the exact linear system is singular")
        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]
        pivot_value = augmented[column][column]
        augmented[column] = [value / pivot_value for value in augmented[column]]
        for row in range(size):
            if row == column or not augmented[row][column]:
                continue
            factor = augmented[row][column]
            augmented[row] = [
                value - factor * pivot_entry
                for value, pivot_entry in zip(augmented[row], augmented[column])
            ]
    return [augmented[index][-1] for index in range(size)]


def affine_dimension(points: Sequence[ExactPoint]) -> int:
    if not points:
        raise ValueError("an affine support cannot be empty")
    directions = [vector_subtract(point, points[0]) for point in points[1:]]
    return matrix_rank(directions)


def gram_data(
    points: Sequence[ExactPoint],
) -> tuple[list[ExactPoint], list[list[Fraction]]]:
    directions = [vector_subtract(point, points[0]) for point in points[1:]]
    gram = [[dot(left, right) for right in directions] for left in directions]
    if matrix_rank(gram) != len(directions):
        raise ValueError("the support is affinely dependent")
    return directions, gram


def barycentric_coordinates(
    query: ExactPoint, points: Sequence[ExactPoint]
) -> list[Fraction]:
    if len(points) == 1:
        if query != points[0]:
            raise ValueError("a singleton barycentric query must equal its support")
        return [Fraction(1)]
    directions, gram = gram_data(points)
    relative_query = vector_subtract(query, points[0])
    coefficients = solve(
        gram, [dot(direction, relative_query) for direction in directions]
    )
    reconstructed = tuple(
        points[0][axis]
        + sum(
            (
                coefficient * direction[axis]
                for coefficient, direction in zip(coefficients, directions)
            ),
            Fraction(),
        )
        for axis in range(3)
    )
    if reconstructed != query:
        raise ValueError("the generated query escaped the exact affine hull")
    return [Fraction(1) - sum(coefficients, Fraction()), *coefficients]


def circumcenter_witness(
    points: Sequence[ExactPoint],
) -> tuple[ExactPoint, Fraction, list[Fraction]]:
    if len(points) == 1:
        return points[0], Fraction(), [Fraction(1)]
    directions, gram = gram_data(points)
    coefficients = solve(
        gram, [dot(direction, direction) / 2 for direction in directions]
    )
    center = tuple(
        points[0][axis]
        + sum(
            (
                coefficient * direction[axis]
                for coefficient, direction in zip(coefficients, directions)
            ),
            Fraction(),
        )
        for axis in range(3)
    )
    barycentric = [Fraction(1) - sum(coefficients, Fraction()), *coefficients]
    level = squared_distance(center, points[0])
    if any(squared_distance(center, point) != level for point in points):
        raise AssertionError("the independent Fraction circumcenter is not equidistant")
    if sum(barycentric, Fraction()) != 1:
        raise AssertionError("the independent Fraction barycentrics do not sum to one")
    return center, level, barycentric  # type: ignore[return-value]


def sign_name(value: Fraction) -> str:
    return "negative" if value < 0 else "zero" if value == 0 else "positive"


def location(signs: Sequence[str]) -> str:
    if "negative" in signs:
        return "exterior"
    return "relative_boundary" if "zero" in signs else "relative_interior"


def rational_record(value: Fraction) -> dict[str, str]:
    return {"denominator": str(value.denominator), "numerator": str(value.numerator)}


def level_record(value: Fraction) -> dict[str, str]:
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def rational3_record(point: ExactPoint) -> dict[str, str]:
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


def barycentric_expected(
    support_words: Sequence[WordPoint], query_words: WordPoint
) -> dict[str, object]:
    support = [decode_point(point) for point in support_words]
    coordinates = barycentric_coordinates(decode_point(query_words), support)
    signs = [sign_name(coordinate) for coordinate in coordinates]
    return {
        "barycentric_coordinates_exact": [
            rational_record(coordinate) for coordinate in coordinates
        ],
        "barycentric_signs": signs,
        "convex_hull_location": location(signs),
        "predicate": "binary64_barycentric_coordinates",
        "support_size": len(support),
    }


def support_analysis_expected(support_words: Sequence[WordPoint]) -> dict[str, object]:
    support = [decode_point(point) for point in support_words]
    dimension = affine_dimension(support)
    if dimension != len(support) - 1:
        return {
            "affine_dimension": dimension,
            "barycentric_coordinates_exact": None,
            "barycentric_signs": None,
            "center_exact": None,
            "convex_hull_location": None,
            "predicate": "binary64_circumcenter_support_analysis",
            "reduced_support_indices": None,
            "squared_level_exact": None,
            "support_kind": "affinely_dependent",
            "support_size": len(support),
            "support_status": "affinely_dependent",
        }
    center, level, coordinates = circumcenter_witness(support)
    signs = [sign_name(coordinate) for coordinate in coordinates]
    if "negative" in signs:
        status = "exterior_circumcenter"
        reduced: list[int] | None = None
    elif "zero" in signs:
        status = "boundary_reduced"
        reduced = [
            index for index, coordinate in enumerate(coordinates) if coordinate > 0
        ]
        reduced_center, reduced_level, reduced_coordinates = circumcenter_witness(
            [support[index] for index in reduced]
        )
        if (
            reduced_center != center
            or reduced_level != level
            or any(coordinate <= 0 for coordinate in reduced_coordinates)
        ):
            raise AssertionError("the Fraction boundary reduction is inconsistent")
    else:
        status = "minimal"
        reduced = list(range(len(support)))
    return {
        "affine_dimension": dimension,
        "barycentric_coordinates_exact": [
            rational_record(coordinate) for coordinate in coordinates
        ],
        "barycentric_signs": signs,
        "center_exact": rational3_record(center),
        "convex_hull_location": location(signs),
        "predicate": "binary64_circumcenter_support_analysis",
        "reduced_support_indices": reduced,
        "squared_level_exact": level_record(level),
        "support_kind": "affinely_independent",
        "support_size": len(support),
        "support_status": status,
    }


def sphere_expected(
    support_words: Sequence[WordPoint], query_words: WordPoint
) -> dict[str, object]:
    support = [decode_point(point) for point in support_words]
    center, level, _ = circumcenter_witness(support)
    distance = squared_distance(center, decode_point(query_words))
    offset = distance - level
    sign = sign_name(offset)
    return {
        "center_exact": rational3_record(center),
        "classification": {
            "negative": "strictly_inside",
            "zero": "boundary",
            "positive": "outside",
        }[sign],
        "predicate": "support_sphere_side",
        "sign": sign,
        "signed_offset_exact": rational_record(offset),
        "squared_distance_exact": level_record(distance),
        "squared_level_exact": level_record(level),
    }


def level_comparison_expected(
    left_words: Sequence[WordPoint], right_words: Sequence[WordPoint]
) -> dict[str, object]:
    _, left_level, _ = circumcenter_witness(
        [decode_point(point) for point in left_words]
    )
    _, right_level, _ = circumcenter_witness(
        [decode_point(point) for point in right_words]
    )
    difference = (
        left_level.numerator * right_level.denominator
        - right_level.numerator * left_level.denominator
    )
    sign = sign_name(Fraction(difference))
    return {
        "cross_product_difference_exact": str(difference),
        "equal": difference == 0,
        "left_squared_level_exact": level_record(left_level),
        "ordering": {"negative": "less", "zero": "equal", "positive": "greater"}[sign],
        "predicate": "compare_support_levels",
        "right_squared_level_exact": level_record(right_level),
        "sign": sign,
    }


def barycentric_command(support: Sequence[WordPoint], query: WordPoint) -> str:
    return " ".join(
        [
            "binary64_barycentric_coordinates",
            str(len(support)),
            *flatten_points(support),
            *query,
        ]
    )


def analysis_command(support: Sequence[WordPoint]) -> str:
    return " ".join(
        [
            "binary64_circumcenter_support_analysis",
            str(len(support)),
            *flatten_points(support),
        ]
    )


def sphere_command(support: Sequence[WordPoint], query: WordPoint) -> str:
    return " ".join(
        ["support_sphere_side", str(len(support)), *flatten_points(support), *query]
    )


def level_command(left: Sequence[WordPoint], right: Sequence[WordPoint]) -> str:
    return " ".join(
        [
            "compare_support_levels",
            str(len(left)),
            *flatten_points(left),
            str(len(right)),
            *flatten_points(right),
        ]
    )


@dataclass(frozen=True)
class Case:
    command: str
    predicate: str
    expected: dict[str, object]
    decision_count: int
    support_sizes: tuple[int, ...]
    forced_stage: str | None = None
    forced_sign: str | None = None
    metamorphic_group: str | None = None


def make_barycentric_case(
    support: Sequence[WordPoint],
    query: WordPoint,
    *,
    forced_stage: str | None = None,
    forced_sign: str | None = None,
    group: str | None = None,
) -> Case:
    return Case(
        barycentric_command(support, query),
        "binary64_barycentric_coordinates",
        barycentric_expected(support, query),
        len(support),
        (len(support),),
        forced_stage,
        forced_sign,
        group,
    )


def make_analysis_case(
    support: Sequence[WordPoint],
    *,
    forced_stage: str | None = None,
    forced_sign: str | None = None,
    group: str | None = None,
) -> Case:
    expected = support_analysis_expected(support)
    dependent = expected["support_kind"] == "affinely_dependent"
    return Case(
        analysis_command(support),
        "binary64_circumcenter_support_analysis",
        expected,
        0 if dependent else len(support),
        (len(support),),
        forced_stage,
        forced_sign,
        group,
    )


def make_sphere_case(
    support: Sequence[WordPoint],
    query: WordPoint,
    *,
    forced_stage: str | None = None,
    forced_sign: str | None = None,
    group: str | None = None,
) -> Case:
    return Case(
        sphere_command(support, query),
        "support_sphere_side",
        sphere_expected(support, query),
        1,
        (len(support),),
        forced_stage,
        forced_sign,
        group,
    )


def make_level_case(
    left: Sequence[WordPoint],
    right: Sequence[WordPoint],
    *,
    forced_stage: str | None = None,
    forced_sign: str | None = None,
    group: str | None = None,
) -> Case:
    return Case(
        level_command(left, right),
        "compare_support_levels",
        level_comparison_expected(left, right),
        1,
        (len(left), len(right)),
        forced_stage,
        forced_sign,
        group,
    )


def targeted_cases() -> list[Case]:
    zero = point_words(0.0)
    one = point_words(1.0)
    two = point_words(2.0)
    three = point_words(3.0)
    minimum_subnormal = point_words(float.fromhex("0x0.0000000000001p-1022"))
    negative_minimum_subnormal = point_words(-float.fromhex("0x0.0000000000001p-1022"))
    pair_zero_two = [zero, two]
    tiny_pair = [zero, minimum_subnormal]
    cases = [
        make_barycentric_case(
            pair_zero_two,
            one,
            forced_stage="fp64_filtered",
            forced_sign="positive",
        ),
        make_barycentric_case(
            pair_zero_two,
            point_words(-1.0),
            forced_stage="fp64_filtered",
            forced_sign="negative",
        ),
        make_barycentric_case(
            pair_zero_two,
            minimum_subnormal,
            forced_stage="expansion",
            forced_sign="positive",
        ),
        make_barycentric_case(
            pair_zero_two,
            negative_minimum_subnormal,
            forced_stage="expansion",
            forced_sign="negative",
        ),
        make_barycentric_case(
            pair_zero_two,
            zero,
            forced_stage="expansion",
            forced_sign="zero",
        ),
        make_barycentric_case(
            tiny_pair,
            zero,
            forced_stage="cpu_multiprecision",
            forced_sign="zero",
        ),
        make_barycentric_case(
            tiny_pair,
            negative_minimum_subnormal,
            forced_stage="cpu_multiprecision",
            forced_sign="negative",
        ),
    ]

    acute = [zero, two, point_words(1.0, 2.0)]
    obtuse = [zero, two, point_words(1.0, 0.5)]
    right = [zero, two, point_words(0.0, 2.0)]
    epsilon = float.fromhex("0x1p-100")
    nearly_acute = [zero, two, point_words(epsilon, 2.0)]
    nearly_obtuse = [zero, two, point_words(-epsilon, 2.0)]
    cases.extend(
        [
            make_analysis_case(
                acute, forced_stage="fp64_filtered", forced_sign="positive"
            ),
            make_analysis_case(
                obtuse, forced_stage="fp64_filtered", forced_sign="negative"
            ),
            make_analysis_case(
                nearly_acute, forced_stage="expansion", forced_sign="positive"
            ),
            make_analysis_case(
                nearly_obtuse, forced_stage="expansion", forced_sign="negative"
            ),
            make_analysis_case(right, forced_stage="expansion", forced_sign="zero"),
            make_analysis_case(
                tiny_pair,
                forced_stage="cpu_multiprecision",
                forced_sign="positive",
            ),
            make_analysis_case(
                [
                    zero,
                    point_words(4.0 * float.fromhex("0x0.0000000000001p-1022")),
                    point_words(
                        2.0 * float.fromhex("0x0.0000000000001p-1022"),
                        float.fromhex("0x0.0000000000001p-1022"),
                    ),
                ],
                forced_stage="cpu_multiprecision",
                forced_sign="negative",
            ),
            make_analysis_case(
                [
                    zero,
                    minimum_subnormal,
                    point_words(0.0, float.fromhex("0x0.0000000000001p-1022")),
                ],
                forced_stage="cpu_multiprecision",
                forced_sign="zero",
            ),
            make_analysis_case([zero, zero]),
            make_analysis_case([zero, one, two]),
            make_analysis_case(
                [
                    zero,
                    point_words(1.0, 0.0),
                    point_words(0.0, 1.0),
                    point_words(1.0, 1.0),
                ]
            ),
        ]
    )

    pair_one_three = [one, three]
    below_three = point_words(math.nextafter(3.0, -math.inf))
    above_three = point_words(math.nextafter(3.0, math.inf))
    cases.extend(
        [
            make_sphere_case(
                pair_one_three,
                point_words(2.0),
                forced_stage="fp64_filtered",
                forced_sign="negative",
            ),
            make_sphere_case(
                pair_one_three,
                point_words(5.0),
                forced_stage="fp64_filtered",
                forced_sign="positive",
            ),
            make_sphere_case(
                pair_one_three,
                below_three,
                forced_stage="expansion",
                forced_sign="negative",
            ),
            make_sphere_case(
                pair_one_three,
                three,
                forced_stage="expansion",
                forced_sign="zero",
            ),
            make_sphere_case(
                pair_one_three,
                above_three,
                forced_stage="expansion",
                forced_sign="positive",
            ),
            make_sphere_case(
                tiny_pair,
                zero,
                forced_stage="cpu_multiprecision",
                forced_sign="zero",
            ),
            make_sphere_case(
                [
                    zero,
                    point_words(4.0 * float.fromhex("0x0.0000000000001p-1022")),
                ],
                point_words(2.0 * float.fromhex("0x0.0000000000001p-1022")),
                forced_stage="cpu_multiprecision",
                forced_sign="negative",
            ),
            make_sphere_case(
                [
                    zero,
                    point_words(4.0 * float.fromhex("0x0.0000000000001p-1022")),
                ],
                negative_minimum_subnormal,
                forced_stage="cpu_multiprecision",
                forced_sign="positive",
            ),
        ]
    )

    pair_zero_three = [zero, three]
    pair_zero_two_up = [zero, point_words(math.nextafter(2.0, math.inf))]
    cases.extend(
        [
            make_level_case(
                pair_zero_two,
                pair_zero_three,
                forced_stage="fp64_filtered",
                forced_sign="negative",
            ),
            make_level_case(
                pair_zero_three,
                pair_zero_two,
                forced_stage="fp64_filtered",
                forced_sign="positive",
            ),
            make_level_case(
                pair_zero_two,
                pair_zero_two_up,
                forced_stage="expansion",
                forced_sign="negative",
            ),
            make_level_case(
                pair_zero_two,
                pair_zero_two,
                forced_stage="expansion",
                forced_sign="zero",
            ),
            make_level_case(
                pair_zero_two_up,
                pair_zero_two,
                forced_stage="expansion",
                forced_sign="positive",
            ),
            make_level_case(
                tiny_pair,
                tiny_pair,
                forced_stage="cpu_multiprecision",
                forced_sign="zero",
            ),
            make_level_case(
                [zero],
                tiny_pair,
                forced_stage="cpu_multiprecision",
                forced_sign="negative",
            ),
            make_level_case(
                tiny_pair,
                [zero],
                forced_stage="cpu_multiprecision",
                forced_sign="positive",
            ),
        ]
    )
    return cases


def random_independent_support(
    generator: StableGenerator, size: int
) -> list[WordPoint]:
    if not 1 <= size <= 4:
        raise ValueError("support size must be between one and four")
    for _ in range(10_000):
        points = [
            integer_point([generator.randint(-32, 32) for _axis in range(3)])
            for _point in range(size)
        ]
        exact = [decode_point(point) for point in points]
        if affine_dimension(exact) == size - 1:
            return points
    raise RuntimeError("the stable generator could not produce an independent support")


def affine_query(
    generator: StableGenerator, support_words: Sequence[WordPoint]
) -> WordPoint:
    support = [decode_point(point) for point in support_words]
    if len(support) == 1:
        return support_words[0]
    coefficients = [generator.randint(-2, 2) for _ in support[1:]]
    query = tuple(
        support[0][axis]
        + sum(
            (
                coefficient * (support[index + 1][axis] - support[0][axis])
                for index, coefficient in enumerate(coefficients)
            ),
            Fraction(),
        )
        for axis in range(3)
    )
    if any(coordinate.denominator != 1 for coordinate in query):
        raise AssertionError("an integer affine query unexpectedly became fractional")
    return integer_point([int(coordinate) for coordinate in query])


def random_query(generator: StableGenerator) -> WordPoint:
    return integer_point([generator.randint(-40, 40) for _axis in range(3)])


def negate_word(value: str) -> str:
    return f"{int(value, 16) ^ (1 << 63):016x}"


def signed_axis_point(value: WordPoint) -> WordPoint:
    return negate_word(value[2]), value[0], negate_word(value[1])


def signed_axis_points(values: Sequence[WordPoint]) -> list[WordPoint]:
    return [signed_axis_point(value) for value in values]


def add_bundle(
    cases: list[Case],
    support: Sequence[WordPoint],
    query: WordPoint,
    sphere_query: WordPoint,
    right_support: Sequence[WordPoint],
    *,
    group_prefix: str | None = None,
) -> None:
    cases.extend(
        [
            make_barycentric_case(
                support,
                query,
                group=None if group_prefix is None else group_prefix + ":barycentric",
            ),
            make_analysis_case(
                support,
                group=None if group_prefix is None else group_prefix + ":analysis",
            ),
            make_sphere_case(
                support,
                sphere_query,
                group=None if group_prefix is None else group_prefix + ":sphere",
            ),
            make_level_case(
                support,
                right_support,
                group=None if group_prefix is None else group_prefix + ":level",
            ),
        ]
    )


def build_cases() -> list[Case]:
    cases = targeted_cases()
    generator = StableGenerator(SEED)
    for support_size in range(1, 5):
        for _case_index in range(RANDOM_BUNDLES_PER_SIZE):
            support = random_independent_support(generator, support_size)
            query = affine_query(generator, support)
            sphere_query = random_query(generator)
            right_size = 1 + generator.randbelow(4)
            right = random_independent_support(generator, right_size)
            add_bundle(cases, support, query, sphere_query, right)

    for support_size in range(2, 5):
        for group_index in range(METAMORPHIC_BASES_PER_SIZE):
            support = random_independent_support(generator, support_size)
            query = affine_query(generator, support)
            sphere_query = random_query(generator)
            right = random_independent_support(generator, 1 + generator.randbelow(4))
            group = f"m{support_size}-{group_index}"
            add_bundle(cases, support, query, sphere_query, right, group_prefix=group)

            permuted_support = list(support)
            generator.shuffle(permuted_support)
            permuted_right = list(right)
            generator.shuffle(permuted_right)
            add_bundle(
                cases,
                permuted_support,
                query,
                sphere_query,
                permuted_right,
                group_prefix=group,
            )

            add_bundle(
                cases,
                signed_axis_points(support),
                signed_axis_point(query),
                signed_axis_point(sphere_query),
                signed_axis_points(right),
                group_prefix=group,
            )
    if len(cases) != EXPECTED_DEFAULT_CASE_COUNT:
        raise AssertionError(
            f"the default v7 corpus has {len(cases)} cases, expected "
            f"{EXPECTED_DEFAULT_CASE_COUNT}"
        )
    return cases


def scientific_projection(value: dict[str, object]) -> dict[str, object]:
    return {
        key: field
        for key, field in value.items()
        if key not in {"certification_stage", "counters"}
    }


def decision_signs(case: Case) -> list[str]:
    signs = case.expected.get("barycentric_signs")
    if isinstance(signs, list):
        return [str(sign) for sign in signs]
    sign = case.expected.get("sign")
    return [] if sign is None else [str(sign)]


def validate_stage_and_counters(
    value: dict[str, object], case: Case, *, multiprecision_only: bool
) -> str:
    stage = value.get("certification_stage")
    counters = value.get("counters")
    if not isinstance(counters, dict) or set(counters) != COUNTER_FIELDS:
        raise AssertionError(f"{case.predicate} returned malformed PredicateCounts v2")
    if any(type(count) is not int or count < 0 for count in counters.values()):
        raise AssertionError(
            f"{case.predicate} returned a negative or noninteger counter"
        )
    signs = decision_signs(case)
    if case.decision_count == 0:
        if stage is not None or any(counters.values()):
            raise AssertionError("a dependent support analysis exposed a decision")
        return "none"
    if not isinstance(stage, str) or stage not in STAGES:
        raise AssertionError(f"{case.predicate} returned an unavailable stage")
    if multiprecision_only and stage != "cpu_multiprecision":
        raise AssertionError(
            f"{case.predicate} escaped --multiprecision-only through {stage}"
        )
    if "zero" in signs and stage == "fp64_filtered":
        raise AssertionError("an FP64 interval filter certified an exact zero")
    expected = {
        "cpu_multiprecision_certified": (
            case.decision_count if stage == "cpu_multiprecision" else 0
        ),
        "exact_zeros": signs.count("zero"),
        "expansion_certified": case.decision_count if stage == "expansion" else 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": (
            case.decision_count if stage == "fp64_filtered" else 0
        ),
        "remaining_unknown": 0,
    }
    if counters != expected:
        raise AssertionError(
            f"{case.predicate} counters disagree with {stage}: "
            f"expected {expected}, observed {counters}"
        )
    return stage


def parse_output(
    output: str, cases: Sequence[Case], label: str
) -> list[dict[str, object]]:
    lines = output.splitlines()
    if len(lines) != len(cases):
        raise AssertionError(
            f"{label} produced {len(lines)} records for {len(cases)} commands"
        )
    parsed: list[dict[str, object]] = []
    for index, line in enumerate(lines):
        try:
            value = json.loads(line)
        except json.JSONDecodeError as error:
            raise AssertionError(f"{label} line {index + 1} is not JSON") from error
        if not isinstance(value, dict):
            raise AssertionError(f"{label} line {index + 1} is not a JSON object")
        parsed.append(value)
    return parsed


def run_batch(executable: Path, corpus: str, extra: Sequence[str], timeout: int) -> str:
    result = subprocess.run(
        [str(executable), *extra, "--batch"],
        input=corpus,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8",
        check=False,
        timeout=timeout,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"native v7 batch failed with status {result.returncode}: {result.stderr.strip()}"
        )
    if result.stderr:
        raise AssertionError(f"native v7 batch wrote stderr: {result.stderr.strip()}")
    return result.stdout


def rational_key(record: object) -> tuple[int, int]:
    if not isinstance(record, dict):
        raise AssertionError("a metamorphic rational witness is malformed")
    return int(str(record["numerator"])), int(str(record["denominator"]))


def metamorphic_signature(value: dict[str, object]) -> object:
    predicate = value["predicate"]
    if predicate == "binary64_barycentric_coordinates":
        return (
            predicate,
            value["support_size"],
            value["convex_hull_location"],
            tuple(sorted(rational_key(record) for record in value["barycentric_coordinates_exact"])),  # type: ignore[arg-type]
            tuple(sorted(value["barycentric_signs"])),  # type: ignore[arg-type]
        )
    if predicate == "binary64_circumcenter_support_analysis":
        coordinates = value["barycentric_coordinates_exact"]
        signs = value["barycentric_signs"]
        return (
            predicate,
            value["affine_dimension"],
            value["support_size"],
            value["support_kind"],
            value["support_status"],
            value["convex_hull_location"],
            canonical_json(value["squared_level_exact"]),
            (
                ()
                if coordinates is None
                else tuple(sorted(rational_key(record) for record in coordinates))
            ),
            () if signs is None else tuple(sorted(signs)),
            (
                None
                if value["reduced_support_indices"] is None
                else len(value["reduced_support_indices"])
            ),  # type: ignore[arg-type]
        )
    if predicate == "support_sphere_side":
        return (
            predicate,
            value["classification"],
            value["sign"],
            canonical_json(value["signed_offset_exact"]),
            canonical_json(value["squared_distance_exact"]),
            canonical_json(value["squared_level_exact"]),
        )
    if predicate == "compare_support_levels":
        return canonical_json(value)
    raise AssertionError(f"unexpected metamorphic predicate {predicate}")


def audit_metamorphic_relations(
    cases: Sequence[Case], observed: Sequence[dict[str, object]]
) -> int:
    groups: dict[str, list[object]] = {}
    for case, value in zip(cases, observed):
        if case.metamorphic_group is not None:
            groups.setdefault(case.metamorphic_group, []).append(
                metamorphic_signature(scientific_projection(value))
            )
    relation_count = 0
    for group, signatures in groups.items():
        if len(signatures) != 3:
            raise AssertionError(
                f"metamorphic group {group} has {len(signatures)} variants instead of three"
            )
        if signatures[1] != signatures[0] or signatures[2] != signatures[0]:
            raise AssertionError(
                f"support permutation or signed-axis transform changed {group}"
            )
        relation_count += 2
    return relation_count


def nested_histogram() -> dict[str, dict[str, int]]:
    return {stage: {sign: 0 for sign in SIGNS} for stage in STAGES}


def audit(
    cases: Sequence[Case],
    normal: Sequence[dict[str, object]],
    multiprecision: Sequence[dict[str, object]],
) -> dict[str, object]:
    predicates = sorted({case.predicate for case in cases})
    stage_histogram = {
        predicate: {**{stage: 0 for stage in STAGES}, "none": 0}
        for predicate in predicates
    }
    stage_sign_histogram = {predicate: nested_histogram() for predicate in predicates}
    support_size_histogram = {
        predicate: {str(size): 0 for size in range(1, 5)} for predicate in predicates
    }
    predicate_case_counts = {predicate: 0 for predicate in predicates}
    exact_zero_total = 0
    dependent_analysis_count = 0
    forced_count = 0

    for index, (case, observed, mp_observed) in enumerate(
        zip(cases, normal, multiprecision)
    ):
        for label, value in (("adaptive", observed), ("multiprecision", mp_observed)):
            if value.get("predicate") != case.predicate:
                raise AssertionError(
                    f"{label} case {index} returned predicate {value.get('predicate')!r}"
                )
            projection = scientific_projection(value)
            if projection != case.expected:
                raise AssertionError(
                    f"{label} {case.predicate} case {index} differs from the "
                    f"independent Fraction oracle: expected {canonical_json(case.expected)}, "
                    f"observed {canonical_json(projection)}"
                )
        if scientific_projection(observed) != scientific_projection(mp_observed):
            raise AssertionError(
                f"{case.predicate} case {index} changed scientifically under "
                "--multiprecision-only"
            )
        stage = validate_stage_and_counters(observed, case, multiprecision_only=False)
        validate_stage_and_counters(mp_observed, case, multiprecision_only=True)
        stage_histogram[case.predicate][stage] += 1
        signs = decision_signs(case)
        if stage != "none":
            for sign in signs:
                stage_sign_histogram[case.predicate][stage][sign] += 1
        exact_zero_total += signs.count("zero")
        predicate_case_counts[case.predicate] += 1
        for size in case.support_sizes:
            support_size_histogram[case.predicate][str(size)] += 1
        if case.decision_count == 0:
            dependent_analysis_count += 1
        if case.forced_stage is not None:
            forced_count += 1
            if stage != case.forced_stage:
                raise AssertionError(
                    f"constructed {case.predicate} case {index} was meant to force "
                    f"{case.forced_stage}, but obtained {stage}"
                )
            if case.forced_sign is not None and case.forced_sign not in signs:
                raise AssertionError(
                    f"constructed {case.predicate} case {index} forced {stage} but "
                    f"did not contain the required {case.forced_sign} decision"
                )

    required = {
        predicate: {
            ("fp64_filtered", "negative"),
            ("fp64_filtered", "positive"),
            ("expansion", "negative"),
            ("expansion", "zero"),
            ("expansion", "positive"),
            ("cpu_multiprecision", "negative"),
            ("cpu_multiprecision", "zero"),
            ("cpu_multiprecision", "positive"),
        }
        for predicate in predicates
    }
    missing: list[str] = []
    for predicate, combinations in required.items():
        for stage, sign in combinations:
            if stage_sign_histogram[predicate][stage][sign] == 0:
                missing.append(f"{predicate}:{stage}:{sign}")
    if missing:
        raise AssertionError(
            "the constructed corpus could not force required classes: "
            + ", ".join(missing)
        )

    return {
        "adaptive_stage_histogram": stage_histogram,
        "adaptive_stage_sign_histogram": stage_sign_histogram,
        "dependent_analysis_case_count": dependent_analysis_count,
        "exact_zero_decision_count": exact_zero_total,
        "forced_case_count": forced_count,
        "predicate_case_counts": predicate_case_counts,
        "support_size_histogram": support_size_histogram,
    }


def positive_integer(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("--timeout-seconds", type=positive_integer, default=180)
    arguments = parser.parse_args()

    cases = build_cases()
    corpus = "".join(case.command + "\n" for case in cases)
    oracle = "".join(canonical_json(case.expected) + "\n" for case in cases)
    corpus_hash = hashlib.sha256(corpus.encode("ascii")).hexdigest()
    oracle_hash = hashlib.sha256(oracle.encode("utf-8")).hexdigest()
    if EXPECTED_DEFAULT_CORPUS_SHA256 and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256:
        raise AssertionError(
            "the default v7 command corpus changed without a generator-version update"
        )
    if EXPECTED_DEFAULT_ORACLE_SHA256 and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256:
        raise AssertionError(
            "the default v7 Fraction oracle changed without a generator-version update"
        )

    normal = parse_output(
        run_batch(arguments.native_replay, corpus, (), arguments.timeout_seconds),
        cases,
        "adaptive batch",
    )
    multiprecision = parse_output(
        run_batch(
            arguments.native_replay,
            corpus,
            ("--multiprecision-only",),
            arguments.timeout_seconds,
        ),
        cases,
        "multiprecision batch",
    )
    statistics = audit(cases, normal, multiprecision)
    metamorphic_relations = audit_metamorphic_relations(cases, normal)
    print(
        canonical_json(
            {
                "adaptive_multiprecision_scientific_projection_equal": True,
                "case_count": len(cases),
                "command_corpus_sha256": corpus_hash,
                "generator": "support-adaptive-fraction-splitmix64-v1",
                "metamorphic_group_count": 4 * 3 * METAMORPHIC_BASES_PER_SIZE,
                "metamorphic_relation_count": metamorphic_relations,
                "oracle_sha256": oracle_hash,
                "random_bundle_count": 4 * RANDOM_BUNDLES_PER_SIZE,
                "seed": f"0x{SEED:016x}",
                "signed_axis_and_support_permutation_variants": True,
                **statistics,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
