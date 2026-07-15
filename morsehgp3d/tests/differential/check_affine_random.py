#!/usr/bin/env python3
"""Audit the exact affine batch replay with a deterministic Fraction oracle."""

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


SEED = 0x414646494E453356
DEFAULT_PLANE_BASES = 48
DEFAULT_POWER_FORM_BASES = 48
DEFAULT_ORIENTATION_BASES = 48
DEFAULT_INTERSECTION_BASES = 32
DEFAULT_FOURTH_BASES = 48
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "1dc9bce8051ba64ddc67d7943122190be36bed884fb35a9a9a6f78e111418457"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "47929b8b1fed34e3ee38a77da13bf3d994b66962fb3b35e773ce49e0a785a0a9"
)
UINT64_MASK = (1 << 64) - 1
PLANE_SCHEMA_VERSION = "2.0.0"

Point = tuple[Fraction, Fraction, Fraction]
Plane = tuple[int, int, int, int]


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

    def choice(self, values: Sequence[int]) -> int:
        return values[self.randbelow(len(values))]


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(value: Point, factor: int) -> Point:
    return tuple(coordinate * factor for coordinate in value)  # type: ignore[return-value]


def dot(left: Sequence[Fraction | int], right: Sequence[Fraction | int]) -> Fraction:
    return sum((Fraction(a) * Fraction(b) for a, b in zip(left, right)), Fraction())


def cross(left: Point, right: Point) -> Point:
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def determinant3(rows: Sequence[Sequence[Fraction | int]]) -> Fraction:
    return (
        Fraction(rows[0][0])
        * (
            Fraction(rows[1][1]) * Fraction(rows[2][2])
            - Fraction(rows[1][2]) * Fraction(rows[2][1])
        )
        - Fraction(rows[0][1])
        * (
            Fraction(rows[1][0]) * Fraction(rows[2][2])
            - Fraction(rows[1][2]) * Fraction(rows[2][0])
        )
        + Fraction(rows[0][2])
        * (
            Fraction(rows[1][0]) * Fraction(rows[2][1])
            - Fraction(rows[1][1]) * Fraction(rows[2][0])
        )
    )


def canonical_homogeneous(coefficients: Sequence[Fraction | int]) -> tuple[int, ...]:
    rationals = [Fraction(value) for value in coefficients]
    denominator = math.lcm(*(value.denominator for value in rationals))
    integers = [
        value.numerator * (denominator // value.denominator) for value in rationals
    ]
    divisor = 0
    for value in integers:
        divisor = math.gcd(divisor, abs(value))
    if divisor == 0:
        return tuple(integers)
    return tuple(value // divisor for value in integers)


def canonical_plane(coefficients: Sequence[Fraction | int]) -> Plane:
    canonical = canonical_homogeneous(coefficients)
    if len(canonical) != 4 or canonical[:3] == (0, 0, 0):
        raise ValueError("a generated support plane must have a nonzero normal")
    return canonical  # type: ignore[return-value]


def plane_through_points(a: Point, b: Point, c: Point) -> Plane:
    normal = cross(subtract(b, a), subtract(c, a))
    if normal == (0, 0, 0):
        raise ValueError("a generated plane triangle is affinely dependent")
    return canonical_plane((*normal, -dot(normal, a)))


def opposite(plane: Plane) -> Plane:
    return tuple(-value for value in plane)  # type: ignore[return-value]


def translate_plane(plane: Plane, translation: Point) -> Plane:
    return canonical_plane((*plane[:3], Fraction(plane[3]) - dot(plane[:3], translation)))


def evaluate_plane(plane: Plane, point: Point) -> Fraction:
    return dot(plane[:3], point) + plane[3]


def plane_record(plane: Plane) -> dict[str, str]:
    return {
        "a": str(plane[0]),
        "b": str(plane[1]),
        "c": str(plane[2]),
        "d": str(plane[3]),
        "schema_version": PLANE_SCHEMA_VERSION,
    }


def rational_record(value: Fraction) -> dict[str, str]:
    return {"denominator": str(value.denominator), "numerator": str(value.numerator)}


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


def exact_counters(value: Fraction) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if value == 0 else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def word_from_fraction(value: Fraction) -> str:
    encoded = float(value)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != value:
        raise ValueError("a generated point coordinate is not exactly binary64")
    return struct.pack(">d", encoded).hex()


def point_tokens(point: Point) -> list[str]:
    return [word_from_fraction(coordinate) for coordinate in point]


def plane_tokens(plane: Plane) -> list[str]:
    return [str(coefficient) for coefficient in plane]


def matrix_rref(
    matrix: Sequence[Sequence[Fraction | int]],
) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [[Fraction(entry) for entry in row] for row in matrix]
    pivots: list[int] = []
    pivot_row = 0
    if not reduced:
        return reduced, pivots
    for column in range(len(reduced[0])):
        selected = next(
            (row for row in range(pivot_row, len(reduced)) if reduced[row][column] != 0),
            None,
        )
        if selected is None:
            continue
        reduced[pivot_row], reduced[selected] = reduced[selected], reduced[pivot_row]
        pivot = reduced[pivot_row][column]
        reduced[pivot_row] = [entry / pivot for entry in reduced[pivot_row]]
        for row in range(len(reduced)):
            if row == pivot_row or reduced[row][column] == 0:
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


@dataclass(frozen=True)
class IntersectionOracle:
    kind: str
    point: Point | None
    normal_rank: int
    augmented_rank: int
    affine_dimension: int | None


def solve_three_planes(planes: Sequence[Plane]) -> IntersectionOracle:
    normals = [list(plane[:3]) for plane in planes]
    augmented = [[*plane[:3], -plane[3]] for plane in planes]
    _, normal_pivots = matrix_rref(normals)
    reduced, augmented_pivots = matrix_rref(augmented)
    normal_rank = len(normal_pivots)
    augmented_rank = len(augmented_pivots)
    if normal_rank < augmented_rank:
        return IntersectionOracle("empty", None, normal_rank, augmented_rank, None)
    if normal_rank < 3:
        return IntersectionOracle(
            "affine_family", None, normal_rank, augmented_rank, 3 - normal_rank
        )
    point = [Fraction(), Fraction(), Fraction()]
    for row, column in enumerate(augmented_pivots):
        if column < 3:
            point[column] = reduced[row][3]
    result = tuple(point)  # type: ignore[assignment]
    if any(evaluate_plane(plane, result) != 0 for plane in planes):
        raise AssertionError("the independent Gaussian oracle produced a non-incident point")
    return IntersectionOracle("unique", result, normal_rank, augmented_rank, 0)


@dataclass(frozen=True)
class Case:
    predicate: str
    command: str
    expected: dict[str, object]
    metamorphism: str
    label_cardinality: int | None = None


def plane_case(points: Sequence[Point], metamorphism: str) -> Case:
    plane = plane_through_points(*points)
    command = " ".join(
        ["plane_through_points", *(token for point in points for token in point_tokens(point))]
    )
    return Case(
        "plane_through_points",
        command,
        {"plane": plane_record(plane), "predicate": "plane_through_points"},
        metamorphism,
    )


def power_form_coefficients(
    point_table: Sequence[Point], r_ids: Sequence[int], q_ids: Sequence[int]
) -> tuple[Fraction, Fraction, Fraction, Fraction]:
    r_points = [point_table[index] for index in r_ids]
    q_points = [point_table[index] for index in q_ids]
    delta = tuple(
        sum((point[axis] for point in r_points), Fraction())
        - sum((point[axis] for point in q_points), Fraction())
        for axis in range(3)
    )
    delta_norm = sum(
        (dot(point, point) for point in r_points), Fraction()
    ) - sum((dot(point, point) for point in q_points), Fraction())
    return -2 * delta[0], -2 * delta[1], -2 * delta[2], delta_norm


def power_form_case(
    point_table: Sequence[Point],
    r_ids: Sequence[int],
    q_ids: Sequence[int],
    metamorphism: str,
) -> Case:
    if (
        not r_ids
        or len(r_ids) != len(q_ids)
        or list(r_ids) != sorted(set(r_ids))
        or list(q_ids) != sorted(set(q_ids))
    ):
        raise AssertionError("generated power labels are not equal-cardinality canonical sets")
    coefficients = power_form_coefficients(point_table, r_ids, q_ids)
    normal_is_zero = coefficients[:3] == (Fraction(), Fraction(), Fraction())
    if not normal_is_zero:
        classification = "proper_plane"
    elif coefficients[3] < 0:
        classification = "constant_negative"
    elif coefficients[3] > 0:
        classification = "constant_positive"
    else:
        classification = "identically_zero"
    result: dict[str, object] = {
        "affine_form": {
            "a": rational_record(coefficients[0]),
            "b": rational_record(coefficients[1]),
            "c": rational_record(coefficients[2]),
            "d": rational_record(coefficients[3]),
            "schema_version": "2.0.0",
        },
        "classification": classification,
        "predicate": "power_bisector_affine_form",
    }
    if classification == "proper_plane":
        result["plane"] = plane_record(canonical_plane(coefficients))
    command = " ".join(
        [
            "power_bisector_affine_form",
            str(len(point_table)),
            *(token for point in point_table for token in point_tokens(point)),
            str(len(r_ids)),
            *(str(index) for index in r_ids),
            str(len(q_ids)),
            *(str(index) for index in q_ids),
        ]
    )
    return Case(
        "power_bisector_affine_form",
        command,
        result,
        metamorphism,
        len(r_ids),
    )


def orientation_case(plane: Plane, points: Sequence[Point], metamorphism: str) -> Case:
    if any(evaluate_plane(plane, point) != 0 for point in points):
        raise AssertionError("the generator placed an orientation point off its support plane")
    area = dot(plane[:3], cross(subtract(points[1], points[0]), subtract(points[2], points[0])))
    command = " ".join(
        [
            "orientation_2d_in_plane",
            *plane_tokens(plane),
            *(token for point in points for token in point_tokens(point)),
        ]
    )
    return Case(
        "orientation_2d_in_plane",
        command,
        {
            "certification_stage": "cpu_multiprecision",
            "counters": exact_counters(area),
            "orientation_value_exact": rational_record(area),
            "predicate": "orientation_2d_in_plane",
            "sign": sign(area),
        },
        metamorphism,
    )


def intersection_case(planes: Sequence[Plane], metamorphism: str) -> Case:
    oracle = solve_three_planes(planes)
    result: dict[str, object] = {
        "affine_dimension": oracle.affine_dimension,
        "augmented_rank": oracle.augmented_rank,
        "intersection_exact": (
            rational3_record(oracle.point) if oracle.point is not None else None
        ),
        "intersection_kind": oracle.kind,
        "normal_rank": oracle.normal_rank,
        "predicate": "intersect_three_planes",
    }
    return Case(
        "intersect_three_planes",
        " ".join(
            [
                "intersect_three_planes",
                *(token for plane in planes for token in plane_tokens(plane)),
            ]
        ),
        result,
        metamorphism,
    )


def fourth_case(planes: Sequence[Plane], metamorphism: str) -> Case:
    oracle = solve_three_planes(planes[:3])
    if oracle.kind != "unique" or oracle.point is None:
        raise AssertionError("a generated fourth-plane case has no unique base intersection")
    value = evaluate_plane(planes[3], oracle.point)
    return Case(
        "fourth_plane_incidence",
        " ".join(
            [
                "fourth_plane_incidence",
                *(token for plane in planes for token in plane_tokens(plane)),
            ]
        ),
        {
            "certification_stage": "cpu_multiprecision",
            "counters": exact_counters(value),
            "intersection_exact": rational3_record(oracle.point),
            "predicate": "fourth_plane_incidence",
            "sign": sign(value),
            "signed_value_exact": rational_record(value),
        },
        metamorphism,
    )


def random_point(generator: StableGenerator, denominator_power: int = 0) -> Point:
    denominator = 1 << denominator_power
    return tuple(
        Fraction(generator.randint(-12, 12), denominator) for _ in range(3)
    )  # type: ignore[return-value]


def random_vector(generator: StableGenerator, radius: int = 6) -> Point:
    while True:
        value = tuple(Fraction(generator.randint(-radius, radius)) for _ in range(3))
        if value != (0, 0, 0):
            return value  # type: ignore[return-value]


def random_triangle(generator: StableGenerator) -> tuple[Point, Point, Point]:
    denominator_power = generator.choice((0, 1, 2, 3))
    denominator = 1 << denominator_power
    a = random_point(generator, denominator_power)
    while True:
        u_integer = random_vector(generator)
        v_integer = random_vector(generator)
        if cross(u_integer, v_integer) == (0, 0, 0):
            continue
        u = tuple(value / denominator for value in u_integer)
        v = tuple(value / denominator for value in v_integer)
        return a, add(a, u), add(a, v)  # type: ignore[arg-type]


def random_plane(generator: StableGenerator) -> Plane:
    normal = random_vector(generator, 7)
    return canonical_plane((*normal, generator.randint(-15, 15)))


def plane_at_point(generator: StableGenerator, point: Point) -> Plane:
    normal = random_vector(generator, 7)
    return canonical_plane((*normal, -dot(normal, point)))


def random_unique_planes(generator: StableGenerator) -> tuple[Plane, Plane, Plane]:
    while True:
        planes = (random_plane(generator), random_plane(generator), random_plane(generator))
        if determinant3([plane[:3] for plane in planes]) != 0:
            return planes


def plane_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x504C414E45)
    cases: list[Case] = []
    for _ in range(base_count):
        a, b, c = random_triangle(generator)
        translation = random_point(generator, generator.choice((0, 1, 2)))
        cases.extend(
            [
                plane_case((a, b, c), "identity"),
                plane_case((b, c, a), "cyclic_permutation"),
                plane_case((a, c, b), "orientation_reversal"),
                plane_case(
                    (
                        a,
                        add(a, multiply(subtract(b, a), 2)),
                        add(a, multiply(subtract(c, a), 3)),
                    ),
                    "positive_basis_rescaling",
                ),
                plane_case(
                    tuple(add(point, translation) for point in (a, b, c)),
                    "translation",
                ),
            ]
        )
        if cases[-5].expected["plane"] != cases[-4].expected["plane"]:
            raise AssertionError("a cyclic point permutation changed the generated oriented plane")
        if cases[-3].expected["plane"] != plane_record(opposite(plane_through_points(a, b, c))):
            raise AssertionError("a point transposition did not reverse the generated plane")
        if cases[-2].expected["plane"] != cases[-5].expected["plane"]:
            raise AssertionError("positive in-plane basis rescaling changed the generated plane")
    return cases


def random_dyadic_point(generator: StableGenerator) -> Point:
    return random_point(generator, generator.choice((0, 1, 2, 3)))


def proper_power_form_cases(generator: StableGenerator, cardinality: int) -> list[Case]:
    while True:
        point_table = [random_dyadic_point(generator) for _ in range(2 * cardinality)]
        r_ids = list(range(cardinality))
        q_ids = list(range(cardinality, 2 * cardinality))
        coefficients = power_form_coefficients(point_table, r_ids, q_ids)
        if coefficients[:3] != (Fraction(), Fraction(), Fraction()):
            break
    original = power_form_case(point_table, r_ids, q_ids, "h_proper")
    swapped = power_form_case(point_table, q_ids, r_ids, "h_r_q_swap")
    original_form = original.expected["affine_form"]
    swapped_form = swapped.expected["affine_form"]
    if not isinstance(original_form, dict) or not isinstance(swapped_form, dict):
        raise AssertionError("the generated H witness is not a closed affine form")
    for field in "abcd":
        original_value = Fraction(
            int(original_form[field]["numerator"]),  # type: ignore[index]
            int(original_form[field]["denominator"]),  # type: ignore[index]
        )
        swapped_value = Fraction(
            int(swapped_form[field]["numerator"]),  # type: ignore[index]
            int(swapped_form[field]["denominator"]),  # type: ignore[index]
        )
        if swapped_value != -original_value:
            raise AssertionError("R/Q exchange did not negate an exact H coefficient")
    return [original, swapped]


def constant_power_form_cases(
    generator: StableGenerator, cardinality: int
) -> list[Case]:
    if cardinality < 2:
        raise ValueError("a nonzero constant H construction requires cardinality at least two")
    center = random_dyadic_point(generator)
    while True:
        first_offset = random_vector(generator, 5)
        second_offset = random_vector(generator, 5)
        first_norm = dot(first_offset, first_offset)
        second_norm = dot(second_offset, second_offset)
        if first_norm != second_norm:
            break
    if first_norm < second_norm:
        first_offset, second_offset = second_offset, first_offset
    point_table = [
        add(center, first_offset),
        subtract(center, first_offset),
        add(center, second_offset),
        subtract(center, second_offset),
    ]
    shared_ids: list[int] = []
    for _ in range(cardinality - 2):
        shared_ids.append(len(point_table))
        point_table.append(random_dyadic_point(generator))
    r_ids = [0, 1, *shared_ids]
    q_ids = [2, 3, *shared_ids]
    positive = power_form_case(point_table, r_ids, q_ids, "h_constant_positive")
    negative = power_form_case(point_table, q_ids, r_ids, "h_constant_negative_r_q_swap")
    if (
        positive.expected["classification"] != "constant_positive"
        or negative.expected["classification"] != "constant_negative"
    ):
        raise AssertionError("the paired constant-H construction has the wrong sign")
    return [positive, negative]


def power_form_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x485F464F524D)
    cases: list[Case] = []
    for index in range(base_count):
        proper_cardinality = 1 + index % 4
        constant_cardinality = 2 + index % 3
        identical_cardinality = 1 + (index + 2) % 4
        cases.extend(proper_power_form_cases(generator, proper_cardinality))
        cases.extend(constant_power_form_cases(generator, constant_cardinality))
        point_table = [
            random_dyadic_point(generator) for _ in range(identical_cardinality)
        ]
        ids = list(range(identical_cardinality))
        identical = power_form_case(
            point_table, ids, ids, "h_identically_zero_r_q_identity"
        )
        if identical.expected["classification"] != "identically_zero":
            raise AssertionError("identical R/Q labels did not produce zero H")
        cases.append(identical)
    return cases


def orientation_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x4F5249454E54)
    cases: list[Case] = []
    for _ in range(base_count):
        a, b, c = random_triangle(generator)
        plane = plane_through_points(a, b, c)
        translation = random_point(generator, generator.choice((0, 1, 2)))
        translated = tuple(add(point, translation) for point in (a, b, c))
        translated_plane = plane_through_points(*translated)
        collinear = add(a, multiply(subtract(b, a), 2))
        cases.extend(
            [
                orientation_case(plane, (a, b, c), "identity"),
                orientation_case(plane, (b, c, a), "cyclic_permutation"),
                orientation_case(plane, (a, c, b), "point_orientation_reversal"),
                orientation_case(opposite(plane), (a, b, c), "support_orientation_reversal"),
                orientation_case(
                    plane,
                    (
                        a,
                        add(a, multiply(subtract(b, a), 2)),
                        add(a, multiply(subtract(c, a), 3)),
                    ),
                    "positive_basis_rescaling",
                ),
                orientation_case(plane, (a, b, collinear), "collinear_zero"),
                orientation_case(translated_plane, translated, "translation"),
            ]
        )
    return cases


def unique_intersection_cases(generator: StableGenerator) -> list[Case]:
    planes = random_unique_planes(generator)
    translation = random_point(generator)
    translated = tuple(translate_plane(plane, translation) for plane in planes)
    return [
        intersection_case(planes, "unique_identity"),
        intersection_case((planes[2], planes[0], planes[1]), "unique_plane_permutation"),
        intersection_case(
            (opposite(planes[0]), planes[1], planes[2]),
            "unique_single_orientation_reversal",
        ),
        intersection_case(
            tuple(opposite(plane) for plane in planes),
            "unique_all_orientations_reversed",
        ),
        intersection_case(translated, "unique_translation"),
    ]


def empty_intersection_cases(generator: StableGenerator) -> list[Case]:
    while True:
        normal = canonical_homogeneous(random_vector(generator, 7))
        if len(normal) == 3:
            break
    first = canonical_plane((*normal, generator.randint(-12, 12)))
    second = canonical_plane((*normal, first[3] + generator.choice((-3, -2, -1, 1, 2, 3))))
    third = random_plane(generator)
    planes = (first, second, third)
    translation = random_point(generator)
    translated = tuple(translate_plane(plane, translation) for plane in planes)
    return [
        intersection_case(planes, "empty_identity"),
        intersection_case((third, second, first), "empty_plane_permutation"),
        intersection_case((opposite(first), second, third), "empty_single_orientation_reversal"),
        intersection_case(
            tuple(opposite(plane) for plane in planes),
            "empty_all_orientations_reversed",
        ),
        intersection_case(translated, "empty_translation"),
    ]


def line_family_cases(generator: StableGenerator) -> list[Case]:
    point = random_point(generator)
    while True:
        first = plane_at_point(generator, point)
        second = plane_at_point(generator, point)
        first_normal = tuple(Fraction(value) for value in first[:3])
        second_normal = tuple(Fraction(value) for value in second[:3])
        if cross(first_normal, second_normal) != (0, 0, 0):  # type: ignore[arg-type]
            break
    third = canonical_plane(tuple(first[index] + second[index] for index in range(4)))
    planes = (first, second, third)
    translation = random_point(generator)
    translated = tuple(translate_plane(plane, translation) for plane in planes)
    return [
        intersection_case(planes, "line_family_identity"),
        intersection_case((third, first, second), "line_family_plane_permutation"),
        intersection_case(
            (opposite(first), second, third),
            "line_family_single_orientation_reversal",
        ),
        intersection_case(
            tuple(opposite(plane) for plane in planes),
            "line_family_all_orientations_reversed",
        ),
        intersection_case(translated, "line_family_translation"),
    ]


def plane_family_cases(generator: StableGenerator) -> list[Case]:
    plane = random_plane(generator)
    planes = (plane, opposite(plane), plane)
    translation = random_point(generator)
    translated = tuple(translate_plane(value, translation) for value in planes)
    return [
        intersection_case(planes, "plane_family_identity"),
        intersection_case((planes[1], planes[2], planes[0]), "plane_family_permutation"),
        intersection_case(
            tuple(opposite(value) for value in planes),
            "plane_family_orientations_reversed",
        ),
        intersection_case(translated, "plane_family_translation"),
    ]


def intersection_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x494E544552)
    cases: list[Case] = []
    for _ in range(base_count):
        cases.extend(unique_intersection_cases(generator))
        cases.extend(empty_intersection_cases(generator))
        cases.extend(line_family_cases(generator))
        cases.extend(plane_family_cases(generator))
    return cases


def fourth_cases(base_count: int) -> list[Case]:
    generator = StableGenerator(SEED ^ 0x464F55525448)
    cases: list[Case] = []
    for _ in range(base_count):
        first_three = random_unique_planes(generator)
        oracle = solve_three_planes(first_three)
        if oracle.kind != "unique" or oracle.point is None:
            raise AssertionError("the unique-plane generator contradicted Gaussian elimination")
        normal = random_vector(generator, 7)
        binding = canonical_plane((*normal, -dot(normal, oracle.point)))
        positive = canonical_plane((*normal, -dot(normal, oracle.point) + 1))
        negative = canonical_plane((*normal, -dot(normal, oracle.point) - 1))
        translation = random_point(generator)
        translated = tuple(
            translate_plane(plane, translation) for plane in (*first_three, positive)
        )
        cases.extend(
            [
                fourth_case((*first_three, binding), "incident_zero"),
                fourth_case((*first_three, positive), "positive_offset"),
                fourth_case((*first_three, negative), "negative_offset"),
                fourth_case(
                    (first_three[2], first_three[0], first_three[1], positive),
                    "binding_plane_permutation",
                ),
                fourth_case(
                    (
                        opposite(first_three[0]),
                        first_three[1],
                        first_three[2],
                        positive,
                    ),
                    "binding_orientation_reversal",
                ),
                fourth_case((*first_three, opposite(positive)), "fourth_orientation_reversal"),
                fourth_case(translated, "translation"),
            ]
        )
    return cases


def build_cases(
    plane_bases: int,
    power_form_bases: int,
    orientation_bases: int,
    intersection_bases: int,
    fourth_bases: int,
) -> list[Case]:
    return [
        *plane_cases(plane_bases),
        *power_form_cases(power_form_bases),
        *orientation_cases(orientation_bases),
        *intersection_cases(intersection_bases),
        *fourth_cases(fourth_bases),
    ]


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
            f"native affine batch {' '.join(arguments) or 'normal'} failed closed: "
            f"{completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("native affine batch unexpectedly wrote to stderr")
    return completed.stdout


def audit_output(output: str, cases: Sequence[Case]) -> None:
    lines = output.splitlines(keepends=True)
    if len(lines) != len(cases):
        raise AssertionError(
            f"native affine batch returned {len(lines)} lines for {len(cases)} cases"
        )
    for index, (line, case) in enumerate(zip(lines, cases)):
        expected = canonical_json(case.expected) + "\n"
        if line != expected:
            try:
                observed = json.loads(line)
            except json.JSONDecodeError as error:
                raise AssertionError(
                    f"{case.predicate} case {index} returned invalid JSON: {error}"
                ) from error
            raise AssertionError(
                f"{case.predicate} case {index} ({case.metamorphism}) differs from "
                f"the Fraction oracle: expected={expected.strip()}, "
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
    parser.add_argument("--plane-bases", type=nonnegative_count, default=DEFAULT_PLANE_BASES)
    parser.add_argument(
        "--power-form-bases", type=nonnegative_count, default=DEFAULT_POWER_FORM_BASES
    )
    parser.add_argument(
        "--orientation-bases", type=nonnegative_count, default=DEFAULT_ORIENTATION_BASES
    )
    parser.add_argument(
        "--intersection-bases", type=nonnegative_count, default=DEFAULT_INTERSECTION_BASES
    )
    parser.add_argument("--fourth-bases", type=nonnegative_count, default=DEFAULT_FOURTH_BASES)
    parser.add_argument("--timeout-seconds", type=positive_count, default=180)
    arguments = parser.parse_args()

    cases = build_cases(
        arguments.plane_bases,
        arguments.power_form_bases,
        arguments.orientation_bases,
        arguments.intersection_bases,
        arguments.fourth_bases,
    )
    corpus = "".join(case.command + "\n" for case in cases)
    corpus_hash = hashlib.sha256(corpus.encode("ascii")).hexdigest()
    oracle = "".join(canonical_json(case.expected) + "\n" for case in cases)
    oracle_hash = hashlib.sha256(oracle.encode("utf-8")).hexdigest()
    default_counts = (
        arguments.plane_bases == DEFAULT_PLANE_BASES
        and arguments.power_form_bases == DEFAULT_POWER_FORM_BASES
        and arguments.orientation_bases == DEFAULT_ORIENTATION_BASES
        and arguments.intersection_bases == DEFAULT_INTERSECTION_BASES
        and arguments.fourth_bases == DEFAULT_FOURTH_BASES
    )
    if (
        default_counts
        and EXPECTED_DEFAULT_CORPUS_SHA256
        and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256
    ):
        raise AssertionError(
            "the default affine corpus changed without a generator-version update"
        )
    if (
        default_counts
        and EXPECTED_DEFAULT_ORACLE_SHA256
        and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256
    ):
        raise AssertionError(
            "the default affine oracle changed without a generator-version update"
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
        normal_lines = normal_output.splitlines()
        multiprecision_lines = multiprecision_output.splitlines()
        mismatch = next(
            (
                index
                for index, values in enumerate(zip(normal_lines, multiprecision_lines))
                if values[0] != values[1]
            ),
            min(len(normal_lines), len(multiprecision_lines)),
        )
        raise AssertionError(
            f"exact-only affine outputs differ under --multiprecision-only at case {mismatch}"
        )
    audit_output(normal_output, cases)

    predicate_counts: dict[str, int] = {}
    metamorphism_counts: dict[str, int] = {}
    sign_histogram = {"negative": 0, "positive": 0, "zero": 0}
    intersection_histogram = {"affine_family": 0, "empty": 0, "unique": 0}
    affine_form_histogram = {
        "constant_negative": 0,
        "constant_positive": 0,
        "identically_zero": 0,
        "proper_plane": 0,
    }
    label_cardinality_histogram = {str(cardinality): 0 for cardinality in range(1, 5)}
    for case in cases:
        predicate_counts[case.predicate] = predicate_counts.get(case.predicate, 0) + 1
        metamorphism_counts[case.metamorphism] = (
            metamorphism_counts.get(case.metamorphism, 0) + 1
        )
        if "sign" in case.expected:
            sign_histogram[str(case.expected["sign"])] += 1
        if "intersection_kind" in case.expected:
            intersection_histogram[str(case.expected["intersection_kind"])] += 1
        if "classification" in case.expected:
            affine_form_histogram[str(case.expected["classification"])] += 1
        if case.label_cardinality is not None:
            label_cardinality_histogram[str(case.label_cardinality)] += 1

    print(
        canonical_json(
            {
                "affine_form_classification_histogram": affine_form_histogram,
                "base_case_counts": {
                    "fourth_plane_incidence": arguments.fourth_bases,
                    "intersect_three_planes_per_family": arguments.intersection_bases,
                    "orientation_2d_in_plane": arguments.orientation_bases,
                    "plane_through_points": arguments.plane_bases,
                    "power_bisector_affine_form": arguments.power_form_bases,
                },
                "case_count": len(cases),
                "case_count_by_predicate": predicate_counts,
                "corpus_sha256": corpus_hash,
                "generator": "affine-dyadic-splitmix64-v1",
                "intersection_kind_histogram": intersection_histogram,
                "label_cardinality_histogram": label_cardinality_histogram,
                "metamorphism_histogram": metamorphism_counts,
                "multiprecision_only_byte_identical": True,
                "oracle_sha256": oracle_hash,
                "seed": f"0x{SEED:016x}",
                "sign_histogram": sign_histogram,
                "substream_seeds": {
                    "fourth_plane_incidence": f"0x{SEED ^ 0x464F55525448:016x}",
                    "intersect_three_planes": f"0x{SEED ^ 0x494E544552:016x}",
                    "orientation_2d_in_plane": f"0x{SEED ^ 0x4F5249454E54:016x}",
                    "plane_through_points": f"0x{SEED ^ 0x504C414E45:016x}",
                    "power_bisector_affine_form": f"0x{SEED ^ 0x485F464F524D:016x}",
                },
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
