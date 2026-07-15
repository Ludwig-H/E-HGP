#!/usr/bin/env python3
"""Audit the four v8 adaptive affine predicates with independent Fraction oracles."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
from dataclasses import dataclass, replace
from fractions import Fraction
from pathlib import Path
from typing import Sequence

SEED = 0x3241384341464649
LEGACY_ADAPTIVE_AFFINE_CASE_COUNT = 4096
EXPECTED_DEFAULT_CASE_COUNT = 5120
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "63d22e4daf8417de218b98101b49fcedf501c0557eaff7a2b251a17c07bed5eb"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "03dfcacbd777b2debaebd802a86f356fa7e34948ba04ef43e3d748bbdc2fabdc"
)
UINT64_MASK = (1 << 64) - 1
STAGES = ("fp64_filtered", "expansion", "cpu_multiprecision")
SIGNS = ("negative", "zero", "positive")
ORIGINS = ("coeff", "through", "power", "exact")
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}

Point = tuple[Fraction, Fraction, Fraction]
WordPoint = tuple[str, str, str]
Plane = tuple[int, int, int, int]


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

    def choice(self, values: Sequence[int]) -> int:
        return values[self.randbelow(len(values))]

    def shuffle(self, values: list[object]) -> None:
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


def sign(value: Fraction | int) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def add(left: Point, right: Point) -> Point:
    return tuple(a + b for a, b in zip(left, right))  # type: ignore[return-value]


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def multiply(point: Point, factor: int) -> Point:
    return tuple(value * factor for value in point)  # type: ignore[return-value]


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


def canonical_plane(coefficients: Sequence[Fraction | int]) -> Plane:
    rationals = [Fraction(value) for value in coefficients]
    if len(rationals) != 4:
        raise ValueError("a plane requires four coefficients")
    denominator = math.lcm(*(value.denominator for value in rationals))
    integers = [
        value.numerator * (denominator // value.denominator) for value in rationals
    ]
    divisor = 0
    for value in integers:
        divisor = math.gcd(divisor, abs(value))
    if divisor:
        integers = [value // divisor for value in integers]
    if integers[:3] == [0, 0, 0]:
        raise ValueError("a generated plane requires a nonzero normal")
    return tuple(integers)  # type: ignore[return-value]


def evaluate_plane(plane: Plane, point: Point) -> Fraction:
    return dot(plane[:3], point) + plane[3]


def oriented_key(plane: Plane) -> str:
    return ":".join(str(value) for value in plane)


def word_from_fraction(value: Fraction | int) -> str:
    exact = Fraction(value)
    encoded = float(exact)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != exact:
        raise ValueError(f"{exact} is not exactly representable as binary64")
    return struct.pack(">d", encoded).hex()


def point_tokens(point: Point) -> tuple[str, str, str]:
    return tuple(word_from_fraction(value) for value in point)  # type: ignore[return-value]


def flatten_points(points: Sequence[Point]) -> list[str]:
    return [token for point in points for token in point_tokens(point)]


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


def matrix_rref(
    matrix: Sequence[Sequence[Fraction | int]],
) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [[Fraction(value) for value in row] for row in matrix]
    if not reduced:
        return reduced, []
    pivot_row = 0
    pivots: list[int] = []
    for column in range(len(reduced[0])):
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
class PlaneSource:
    origin: str
    tokens: tuple[str, ...]
    plane: Plane


def coefficient_source(coefficients: Sequence[Fraction | int]) -> PlaneSource:
    plane = canonical_plane(coefficients)
    return PlaneSource(
        "coeff",
        ("coeff", *(word_from_fraction(value) for value in coefficients)),
        plane,
    )


def exact_source(coefficients: Sequence[Fraction | int]) -> PlaneSource:
    plane = canonical_plane(coefficients)
    return PlaneSource("exact", ("exact", *(str(value) for value in plane)), plane)


def through_source(points: Sequence[Point]) -> PlaneSource:
    if len(points) != 3:
        raise ValueError("a through source requires three points")
    normal = cross(subtract(points[1], points[0]), subtract(points[2], points[0]))
    plane = canonical_plane((*normal, -dot(normal, points[0])))
    return PlaneSource("through", ("through", *flatten_points(points)), plane)


def power_source(r_points: Sequence[Point], q_points: Sequence[Point]) -> PlaneSource:
    if not r_points or len(r_points) != len(q_points) or len(r_points) > 10:
        raise ValueError(
            "power sources require equal cardinalities between one and ten"
        )
    delta = tuple(
        sum((point[axis] for point in r_points), Fraction())
        - sum((point[axis] for point in q_points), Fraction())
        for axis in range(3)
    )
    constant = sum((dot(point, point) for point in r_points), Fraction()) - sum(
        (dot(point, point) for point in q_points), Fraction()
    )
    coefficients = (-2 * delta[0], -2 * delta[1], -2 * delta[2], constant)
    plane = canonical_plane(coefficients)
    return PlaneSource(
        "power",
        (
            "power",
            str(len(r_points)),
            *flatten_points(r_points),
            *flatten_points(q_points),
        ),
        plane,
    )


def unit(axis: int) -> Point:
    return tuple(Fraction(1 if index == axis else 0) for index in range(3))  # type: ignore[return-value]


def axis_geometry(axis: int, root: int) -> tuple[Point, Point, Point]:
    anchor = multiply(unit(axis), root)
    first_direction = unit((axis + 1) % 3)
    second_direction = unit((axis + 2) % 3)
    return anchor, first_direction, second_direction


def axis_source(origin: str, axis: int, root: int, orientation: int) -> PlaneSource:
    if origin not in ORIGINS or axis not in range(3) or orientation not in (-1, 1):
        raise ValueError("invalid axis-source parameters")
    coefficients = [Fraction(), Fraction(), Fraction(), Fraction(-orientation * root)]
    coefficients[axis] = Fraction(orientation)
    expected = canonical_plane(coefficients)
    anchor, first_direction, second_direction = axis_geometry(axis, root)
    if origin == "coeff":
        source = coefficient_source(coefficients)
    elif origin == "exact":
        source = exact_source(coefficients)
    elif origin == "through":
        first = add(anchor, first_direction)
        second = add(anchor, second_direction)
        points = (anchor, first, second) if orientation > 0 else (anchor, second, first)
        source = through_source(points)
    else:
        before = subtract(anchor, unit(axis))
        after = add(anchor, unit(axis))
        source = (
            power_source((before,), (after,))
            if orientation > 0
            else power_source((after,), (before,))
        )
    if source.plane != expected:
        raise AssertionError(
            f"{origin} axis source produced {source.plane}, expected {expected}"
        )
    return source


@dataclass(frozen=True)
class IntersectionOracle:
    kind: str
    point: Point | None
    normal_rank: int
    augmented_rank: int
    affine_dimension: int | None


def solve_three_planes(sources: Sequence[PlaneSource]) -> IntersectionOracle:
    if len(sources) != 3:
        raise ValueError("the intersection oracle requires three planes")
    normals = [list(source.plane[:3]) for source in sources]
    augmented = [[*source.plane[:3], -source.plane[3]] for source in sources]
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
    if any(evaluate_plane(source.plane, result) for source in sources):
        raise AssertionError(
            "the independent RREF oracle returned a non-incident point"
        )
    return IntersectionOracle("unique", result, normal_rank, augmented_rank, 0)


def canonical_normal_determinant_sign(sources: Sequence[PlaneSource]) -> str:
    ordered = sorted(sources, key=lambda source: oriented_key(source.plane))
    return sign(determinant3([source.plane[:3] for source in ordered]))


@dataclass(frozen=True)
class Case:
    predicate: str
    command: str
    expected: dict[str, object]
    origins: tuple[str, ...]
    forced_stage: str | None = None
    forced_sign: str | None = None
    metamorphic_group: str | None = None


def case_sign(case: Case) -> str:
    field = (
        "normal_determinant_sign"
        if case.predicate == "adaptive_intersect_three_planes"
        else "sign"
    )
    return str(case.expected[field])


def force_case(case: Case, stage: str) -> Case:
    return replace(case, forced_stage=stage, forced_sign=case_sign(case))


def orientation_case(
    source: PlaneSource,
    points: Sequence[Point],
    *,
    metamorphic_group: str | None = None,
) -> Case:
    if len(points) != 3:
        raise ValueError("an orientation query requires three points")
    if any(evaluate_plane(source.plane, point) for point in points):
        raise ValueError("an orientation query escaped its plane")
    area = dot(
        source.plane[:3],
        cross(subtract(points[1], points[0]), subtract(points[2], points[0])),
    )
    predicate = "adaptive_orientation_2d_in_plane"
    return Case(
        predicate,
        " ".join((predicate, *source.tokens, *flatten_points(points))),
        {
            "orientation_value_exact": rational_record(area),
            "predicate": predicate,
            "sign": sign(area),
        },
        (source.origin,),
        metamorphic_group=metamorphic_group,
    )


def plane_side_case(
    source: PlaneSource,
    point: Point,
    *,
    metamorphic_group: str | None = None,
) -> Case:
    signed_value = evaluate_plane(source.plane, point)
    predicate = "adaptive_plane_side"
    return Case(
        predicate,
        " ".join((predicate, *source.tokens, *point_tokens(point))),
        {
            "predicate": predicate,
            "sign": sign(signed_value),
            "signed_value_exact": rational_record(signed_value),
        },
        (source.origin,),
        metamorphic_group=metamorphic_group,
    )


def intersection_case(
    sources: Sequence[PlaneSource],
    *,
    metamorphic_group: str | None = None,
) -> Case:
    oracle = solve_three_planes(sources)
    predicate = "adaptive_intersect_three_planes"
    return Case(
        predicate,
        " ".join(
            (predicate, *(token for source in sources for token in source.tokens))
        ),
        {
            "affine_dimension": oracle.affine_dimension,
            "augmented_rank": oracle.augmented_rank,
            "intersection_exact": (
                None if oracle.point is None else rational3_record(oracle.point)
            ),
            "intersection_kind": oracle.kind,
            "normal_determinant_sign": canonical_normal_determinant_sign(sources),
            "normal_rank": oracle.normal_rank,
            "predicate": predicate,
        },
        tuple(source.origin for source in sources),
        metamorphic_group=metamorphic_group,
    )


def fourth_case(
    binding: Sequence[PlaneSource],
    fourth: PlaneSource,
    *,
    metamorphic_group: str | None = None,
) -> Case:
    oracle = solve_three_planes(binding)
    if oracle.kind != "unique" or oracle.point is None:
        raise ValueError(
            "fourth-plane incidence requires a unique binding intersection"
        )
    signed_value = evaluate_plane(fourth.plane, oracle.point)
    predicate = "adaptive_fourth_plane_incidence"
    sources = (*binding, fourth)
    return Case(
        predicate,
        " ".join(
            (predicate, *(token for source in sources for token in source.tokens))
        ),
        {
            "intersection_exact": rational3_record(oracle.point),
            "predicate": predicate,
            "sign": sign(signed_value),
            "signed_value_exact": rational_record(signed_value),
        },
        tuple(source.origin for source in sources),
        metamorphic_group=metamorphic_group,
    )


def select_sign_cases(cases: Sequence[Case]) -> tuple[Case, Case]:
    by_sign = {case_sign(case): case for case in cases}
    if "negative" not in by_sign or "positive" not in by_sign:
        raise AssertionError(
            "the constructed determinant variants do not cover both signs"
        )
    return by_sign["negative"], by_sign["positive"]


def add_targeted_cases(cases: list[Case]) -> None:
    origin = (Fraction(), Fraction(), Fraction())
    x = unit(0)
    y = unit(1)
    z_plane = coefficient_source((0, 0, 1, 0))

    cases.extend(
        (
            force_case(orientation_case(z_plane, (origin, x, y)), "fp64_filtered"),
            force_case(orientation_case(z_plane, (origin, y, x)), "fp64_filtered"),
        )
    )

    magnitude = 1 << 26
    near_left = (Fraction(magnitude), Fraction(magnitude - 1), Fraction())
    near_right = (Fraction(magnitude + 1), Fraction(magnitude), Fraction())
    collinear = multiply(near_left, 2)
    cases.extend(
        force_case(orientation_case(z_plane, points), "expansion")
        for points in (
            (origin, near_left, near_right),
            (origin, near_right, near_left),
            (origin, near_left, collinear),
        )
    )

    subnormal = Fraction(1, 1 << 1074)
    subnormal_z = through_source(
        (
            origin,
            (subnormal, Fraction(), Fraction()),
            (Fraction(), subnormal, Fraction()),
        )
    )
    cases.extend(
        force_case(orientation_case(subnormal_z, points), "cpu_multiprecision")
        for points in (
            (origin, x, y),
            (origin, y, x),
            (origin, x, multiply(x, 2)),
        )
    )

    fp_bindings = [axis_source("coeff", axis, 0, 1) for axis in range(3)]
    fp_variants = [
        intersection_case(fp_bindings),
        intersection_case([axis_source("coeff", 0, 0, -1), *fp_bindings[1:]]),
    ]
    negative, positive = select_sign_cases(fp_variants)
    cases.extend(
        (
            force_case(negative, "fp64_filtered"),
            force_case(positive, "fp64_filtered"),
        )
    )

    expansion_bindings = [
        coefficient_source((0, 0, 1, 0)),
        coefficient_source((magnitude, magnitude - 1, 0, 0)),
        coefficient_source((magnitude + 1, magnitude, 0, 0)),
    ]
    expansion_variants = [
        intersection_case(expansion_bindings),
        intersection_case([coefficient_source((0, 0, -1, 0)), *expansion_bindings[1:]]),
    ]
    negative, positive = select_sign_cases(expansion_variants)
    cases.extend(
        (
            force_case(negative, "expansion"),
            force_case(positive, "expansion"),
        )
    )
    family = intersection_case(
        [
            coefficient_source((1, 0, 0, 0)),
            coefficient_source((0, 1, 0, 0)),
            coefficient_source((1, 1, 0, 0)),
        ]
    )
    empty = intersection_case(
        [
            coefficient_source((1, 0, 0, 0)),
            coefficient_source((0, 1, 0, 0)),
            coefficient_source((1, 1, 0, -1)),
        ]
    )
    cases.extend((force_case(family, "expansion"), force_case(empty, "expansion")))

    mp_bindings = [
        coefficient_source((subnormal, 0, 0, 0)),
        coefficient_source((0, subnormal, 0, 0)),
        coefficient_source((0, 0, 1, 0)),
    ]
    mp_variants = [
        intersection_case(mp_bindings),
        intersection_case(
            [coefficient_source((-subnormal, 0, 0, 0)), *mp_bindings[1:]]
        ),
    ]
    negative, positive = select_sign_cases(mp_variants)
    cases.extend(
        (
            force_case(negative, "cpu_multiprecision"),
            force_case(positive, "cpu_multiprecision"),
        )
    )
    mp_family = intersection_case(
        [
            *mp_bindings[:2],
            coefficient_source((subnormal, subnormal, 0, 0)),
        ]
    )
    mp_empty = intersection_case(
        [
            *mp_bindings[:2],
            coefficient_source((subnormal, subnormal, 0, -subnormal)),
        ]
    )
    cases.extend(
        (
            force_case(mp_family, "cpu_multiprecision"),
            force_case(mp_empty, "cpu_multiprecision"),
        )
    )

    cases.extend(
        force_case(
            fourth_case(fp_bindings, coefficient_source((1, 1, 1, constant))),
            "fp64_filtered",
        )
        for constant in (-4, 4)
    )
    cases.extend(
        force_case(
            fourth_case(expansion_bindings, coefficient_source((1, 1, 1, constant))),
            "expansion",
        )
        for constant in (-1, 0, 1)
    )
    cases.extend(
        force_case(
            fourth_case(mp_bindings, coefficient_source((1, 1, 1, constant))),
            "cpu_multiprecision",
        )
        for constant in (-1, 0, 1)
    )

    mixed_rational_bindings = [
        exact_source((1, 1, 1, -1)),
        coefficient_source((1, -1, 0, 0)),
        coefficient_source((0, 1, -1, 0)),
    ]
    mixed_rational_intersection = force_case(
        intersection_case(mixed_rational_bindings),
        "cpu_multiprecision",
    )
    mixed_rational_incidence = force_case(
        fourth_case(
            mixed_rational_bindings,
            coefficient_source((1, 0, 0, 0)),
        ),
        "cpu_multiprecision",
    )
    if (
        mixed_rational_intersection.expected["intersection_exact"]["denominator"]
        != "3"
        or mixed_rational_incidence.expected["intersection_exact"]["denominator"]
        != "3"
        or mixed_rational_incidence.expected["signed_value_exact"]["denominator"]
        != "3"
    ):
        raise AssertionError(
            "the mixed-provenance Cramer cases must retain their noninteger witnesses"
        )
    cases.extend((mixed_rational_intersection, mixed_rational_incidence))


def add_metamorphic_cases(cases: list[Case], generator: StableGenerator) -> None:
    for index in range(12):
        axis = index % 3
        root = generator.randint(-5, 5)
        orientation = -1 if generator.randbelow(2) else 1
        anchor, first_direction, second_direction = axis_geometry(axis, root)
        first = add(anchor, first_direction)
        second = add(anchor, second_direction)
        if orientation < 0:
            first, second = second, first
        if index == 0:
            first_label_point = (Fraction(1), Fraction(2), Fraction(3))
            second_label_point = (Fraction(-2), Fraction(1), Fraction(4))
            common_label_point = (Fraction(5), Fraction(-3), Fraction(2))
            anchor = (Fraction(), Fraction(), Fraction(7, 2))
            first = (Fraction(1), Fraction(), Fraction(13, 2))
            second = (Fraction(), Fraction(1), Fraction(9, 2))
            sources = (
                coefficient_source((-6, -2, 2, -7)),
                through_source((anchor, first, second)),
                power_source(
                    (first_label_point, common_label_point),
                    (second_label_point, common_label_point),
                ),
                exact_source((-6, -2, 2, -7)),
            )
        else:
            sources = tuple(
                axis_source(origin, axis, root, orientation) for origin in ORIGINS
            )
        group = f"orientation-source-substitution-{index}"
        for source in sources:
            cases.append(
                orientation_case(
                    source,
                    (anchor, first, second),
                    metamorphic_group=group,
                )
            )

    for index in range(12):
        roots = [generator.randint(-6, 6) for _ in range(3)]
        origin = ORIGINS[index % len(ORIGINS)]
        sources = [
            axis_source(origin, axis, roots[axis], -1 if index & (1 << axis) else 1)
            for axis in range(3)
        ]
        group = f"intersection-binding-permutation-{index}"
        cases.append(intersection_case(sources, metamorphic_group=group))
        cases.append(
            intersection_case(
                (sources[1], sources[2], sources[0]), metamorphic_group=group
            )
        )
        cases.append(
            intersection_case(
                (sources[2], sources[0], sources[1]), metamorphic_group=group
            )
        )

    for index in range(12):
        roots = [generator.randint(-6, 6) for _ in range(3)]
        origin = ORIGINS[index % len(ORIGINS)]
        binding = [axis_source(origin, axis, roots[axis], 1) for axis in range(3)]
        fourth = axis_source(
            origin,
            generator.randbelow(3),
            generator.randint(-6, 6),
            -1 if generator.randbelow(2) else 1,
        )
        group = f"fourth-binding-permutation-{index}"
        cases.append(fourth_case(binding, fourth, metamorphic_group=group))
        cases.append(
            fourth_case(
                (binding[1], binding[2], binding[0]),
                fourth,
                metamorphic_group=group,
            )
        )
        cases.append(
            fourth_case(
                (binding[2], binding[0], binding[1]),
                fourth,
                metamorphic_group=group,
            )
        )


def add_plane_side_cases(cases: list[Case], generator: StableGenerator) -> None:
    origin = (Fraction(), Fraction(), Fraction())
    z_plane = coefficient_source((0, 0, 1, 0))
    cases.extend(
        (
            force_case(
                plane_side_case(z_plane, (Fraction(), Fraction(), Fraction(-1))),
                "fp64_filtered",
            ),
            force_case(
                plane_side_case(z_plane, (Fraction(), Fraction(), Fraction(1))),
                "fp64_filtered",
            ),
        )
    )

    magnitude = 1 << 26
    cancellation_plane = coefficient_source((1, 0, 0, -magnitude))
    cases.extend(
        force_case(
            plane_side_case(cancellation_plane, point), "expansion"
        )
        for point in (
            (Fraction(magnitude) - Fraction(1, 1 << 27), Fraction(), Fraction()),
            (Fraction(magnitude), Fraction(), Fraction()),
            (Fraction(magnitude) + Fraction(1, 1 << 26), Fraction(), Fraction()),
        )
    )

    subnormal = Fraction(1, 1 << 1074)
    subnormal_plane = through_source(
        (
            origin,
            (subnormal, Fraction(), Fraction()),
            (Fraction(), subnormal, Fraction()),
        )
    )
    cases.extend(
        force_case(
            plane_side_case(
                subnormal_plane, (Fraction(), Fraction(), Fraction(z_value))
            ),
            "cpu_multiprecision",
        )
        for z_value in (-1, 0, 1)
    )
    cases.append(
        force_case(
            plane_side_case(
                exact_source((0, 0, 1, 0)),
                (Fraction(), Fraction(), Fraction(1)),
            ),
            "cpu_multiprecision",
        )
    )

    for index in range(12):
        if index == 0:
            first_label_point = (Fraction(1), Fraction(2), Fraction(3))
            second_label_point = (Fraction(-2), Fraction(1), Fraction(4))
            common_label_point = (Fraction(5), Fraction(-3), Fraction(2))
            anchor = (Fraction(), Fraction(), Fraction(7, 2))
            sources = (
                coefficient_source((-6, -2, 2, -7)),
                through_source(
                    (
                        anchor,
                        (Fraction(1), Fraction(), Fraction(13, 2)),
                        (Fraction(), Fraction(1), Fraction(9, 2)),
                    )
                ),
                power_source(
                    (first_label_point, common_label_point),
                    (second_label_point, common_label_point),
                ),
                exact_source((-6, -2, 2, -7)),
            )
            query = origin
        else:
            axis = index % 3
            root = generator.randint(-5, 5)
            orientation = -1 if generator.randbelow(2) else 1
            sources = tuple(
                axis_source(source_origin, axis, root, orientation)
                for source_origin in ORIGINS
            )
            anchor, _, _ = axis_geometry(axis, root)
            query = add(anchor, unit(axis))
        group = f"plane-side-source-substitution-{index}"
        cases.extend(
            plane_side_case(source, query, metamorphic_group=group)
            for source in sources
        )


def random_plane_side_case(generator: StableGenerator, index: int) -> Case:
    source_origin = ORIGINS[index % len(ORIGINS)]
    axis = generator.randbelow(3)
    root = generator.randint(-16, 16)
    orientation = -1 if generator.randbelow(2) else 1
    source = axis_source(source_origin, axis, root, orientation)
    anchor, first_direction, second_direction = axis_geometry(axis, root)
    point = add(
        anchor,
        add(
            multiply(unit(axis), generator.randint(-3, 3)),
            add(
                multiply(first_direction, generator.randint(-4, 4)),
                multiply(second_direction, generator.randint(-4, 4)),
            ),
        ),
    )
    return plane_side_case(source, point)


def random_orientation_case(generator: StableGenerator, index: int) -> Case:
    origin = ORIGINS[index % len(ORIGINS)]
    axis = generator.randbelow(3)
    root = generator.randint(-16, 16)
    orientation = -1 if generator.randbelow(2) else 1
    source = axis_source(origin, axis, root, orientation)
    anchor, first_direction, second_direction = axis_geometry(axis, root)
    anchor = add(
        anchor,
        add(
            multiply(first_direction, generator.randint(-4, 4)),
            multiply(second_direction, generator.randint(-4, 4)),
        ),
    )
    first_scale = generator.choice((-3, -2, -1, 1, 2, 3))
    second_scale = generator.randint(-3, 3)
    shear = generator.randint(-3, 3)
    first = add(anchor, multiply(first_direction, first_scale))
    second = add(
        anchor,
        add(
            multiply(first_direction, shear),
            multiply(second_direction, second_scale),
        ),
    )
    return orientation_case(source, (anchor, first, second))


def random_intersection_case(generator: StableGenerator, index: int) -> Case:
    origin = ORIGINS[index % len(ORIGINS)]
    orientations = [-1 if generator.randbelow(2) else 1 for _ in range(3)]
    roots = [generator.randint(-16, 16) for _ in range(3)]
    if index % 53 == 0:
        axes = [0, 0, 0]
        if index % 106:
            roots[1] = roots[0] + (1 if roots[0] < 16 else -1)
        else:
            roots[1] = roots[0]
        roots[2] = roots[0]
    elif index % 17 == 0:
        axes = [0, 1, 0]
        roots[2] = (
            roots[0] if index % 34 == 0 else roots[0] + (1 if roots[0] < 16 else -1)
        )
    else:
        axes = [0, 1, 2]
    sources = [
        axis_source(origin, axis, root, orientation)
        for axis, root, orientation in zip(axes, roots, orientations)
    ]
    shuffled: list[object] = list(sources)
    generator.shuffle(shuffled)
    return intersection_case(shuffled)  # type: ignore[arg-type]


def random_fourth_case(generator: StableGenerator, index: int) -> Case:
    origin = ORIGINS[index % len(ORIGINS)]
    roots = [generator.randint(-16, 16) for _ in range(3)]
    binding = [
        axis_source(origin, axis, roots[axis], -1 if generator.randbelow(2) else 1)
        for axis in range(3)
    ]
    axis = generator.randbelow(3)
    if index % 7 == 0:
        fourth_root = roots[axis]
    else:
        fourth_root = generator.randint(-16, 16)
    fourth = axis_source(origin, axis, fourth_root, -1 if generator.randbelow(2) else 1)
    shuffled: list[object] = list(binding)
    generator.shuffle(shuffled)
    return fourth_case(shuffled, fourth)  # type: ignore[arg-type]


def build_cases() -> list[Case]:
    generator = StableGenerator(SEED)
    cases: list[Case] = []
    add_targeted_cases(cases)
    add_metamorphic_cases(cases, generator)
    random_index = 0
    while len(cases) < LEGACY_ADAPTIVE_AFFINE_CASE_COUNT:
        selector = random_index % 3
        if selector == 0:
            cases.append(random_orientation_case(generator, random_index // 3))
        elif selector == 1:
            cases.append(random_intersection_case(generator, random_index // 3))
        else:
            cases.append(random_fourth_case(generator, random_index // 3))
        random_index += 1
    if len(cases) != LEGACY_ADAPTIVE_AFFINE_CASE_COUNT:
        raise AssertionError(
            "the default v8 corpus does not have exactly 4096 commands"
        )
    add_plane_side_cases(cases, generator)
    plane_side_index = 0
    while len(cases) < EXPECTED_DEFAULT_CASE_COUNT:
        cases.append(random_plane_side_case(generator, plane_side_index))
        plane_side_index += 1
    if len(cases) != EXPECTED_DEFAULT_CASE_COUNT:
        raise AssertionError(
            "the extended v8 corpus does not have exactly 5120 commands"
        )
    return cases


SCHEMAS = {
    "adaptive_plane_side": {
        "certification_stage",
        "counters",
        "predicate",
        "sign",
        "signed_value_exact",
    },
    "adaptive_orientation_2d_in_plane": {
        "certification_stage",
        "counters",
        "orientation_value_exact",
        "predicate",
        "sign",
    },
    "adaptive_intersect_three_planes": {
        "affine_dimension",
        "augmented_rank",
        "certification_stage",
        "counters",
        "intersection_exact",
        "intersection_kind",
        "normal_determinant_sign",
        "normal_rank",
        "predicate",
    },
    "adaptive_fourth_plane_incidence": {
        "certification_stage",
        "counters",
        "intersection_exact",
        "predicate",
        "sign",
        "signed_value_exact",
    },
}


def scientific_projection(value: dict[str, object]) -> dict[str, object]:
    return {
        key: field
        for key, field in value.items()
        if key not in {"certification_stage", "counters"}
    }


def validate_stage_and_counters(
    value: dict[str, object], case: Case, *, multiprecision_only: bool
) -> str:
    stage = value.get("certification_stage")
    counters = value.get("counters")
    if not isinstance(stage, str) or stage not in STAGES:
        raise AssertionError(f"{case.predicate} returned an unavailable stage")
    if multiprecision_only and stage != "cpu_multiprecision":
        raise AssertionError(
            f"{case.predicate} escaped --multiprecision-only through {stage}"
        )
    if (
        not multiprecision_only
        and "exact" in case.origins
        and stage != "cpu_multiprecision"
    ):
        raise AssertionError(
            f"{case.predicate} used {stage} without complete binary64 provenance"
        )
    if not isinstance(counters, dict) or set(counters) != COUNTER_FIELDS:
        raise AssertionError(f"{case.predicate} returned malformed PredicateCounts v2")
    if any(type(count) is not int or count < 0 for count in counters.values()):
        raise AssertionError(
            f"{case.predicate} returned noninteger or negative predicate counters"
        )
    decision = case_sign(case)
    if decision == "zero" and stage == "fp64_filtered":
        raise AssertionError("an FP64 interval certified an exact affine zero")
    expected = {
        "cpu_multiprecision_certified": 1 if stage == "cpu_multiprecision" else 0,
        "exact_zeros": 1 if decision == "zero" else 0,
        "expansion_certified": 1 if stage == "expansion" else 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 1 if stage == "fp64_filtered" else 0,
        "remaining_unknown": 0,
    }
    if counters != expected:
        raise AssertionError(
            f"{case.predicate} counters disagree with {stage}: expected {expected}, "
            f"observed {counters}"
        )
    return stage


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
            f"native v8 batch failed with status {result.returncode}: "
            f"{result.stderr.strip()}"
        )
    if result.stderr:
        raise AssertionError(f"native v8 batch wrote stderr: {result.stderr.strip()}")
    return result.stdout


def parse_output(
    output: str, cases: Sequence[Case], label: str
) -> list[dict[str, object]]:
    lines = output.splitlines()
    if len(lines) != len(cases):
        raise AssertionError(
            f"{label} produced {len(lines)} records for {len(cases)} commands"
        )
    parsed: list[dict[str, object]] = []
    for index, (line, case) in enumerate(zip(lines, cases)):
        try:
            value = json.loads(line)
        except json.JSONDecodeError as error:
            raise AssertionError(f"{label} line {index + 1} is not JSON") from error
        if not isinstance(value, dict) or set(value) != SCHEMAS[case.predicate]:
            raise AssertionError(
                f"{label} line {index + 1} does not match the {case.predicate} schema"
            )
        if case.predicate == "adaptive_intersect_three_planes":
            if (
                type(value["normal_rank"]) is not int
                or type(value["augmented_rank"]) is not int
                or (
                    value["affine_dimension"] is not None
                    and type(value["affine_dimension"]) is not int
                )
            ):
                raise AssertionError(
                    f"{label} line {index + 1} returned noninteger affine ranks"
                )
        if line != canonical_json(value):
            raise AssertionError(f"{label} line {index + 1} is not canonical JSON")
        parsed.append(value)
    return parsed


def audit_metamorphic_relations(
    cases: Sequence[Case], observed: Sequence[dict[str, object]]
) -> tuple[int, int]:
    expected_groups: dict[str, list[dict[str, object]]] = {}
    observed_groups: dict[str, list[dict[str, object]]] = {}
    for case, value in zip(cases, observed):
        if case.metamorphic_group is None:
            continue
        expected_groups.setdefault(case.metamorphic_group, []).append(case.expected)
        observed_groups.setdefault(case.metamorphic_group, []).append(
            scientific_projection(value)
        )
    relation_count = 0
    for group, expected in expected_groups.items():
        observed_values = observed_groups[group]
        if len(expected) < 2:
            raise AssertionError(f"metamorphic group {group} has no relation")
        if any(value != expected[0] for value in expected[1:]):
            raise AssertionError(f"the independent oracle invalidated {group}")
        if any(value != observed_values[0] for value in observed_values[1:]):
            raise AssertionError(f"the native scientific result invalidated {group}")
        relation_count += len(expected) - 1
    return len(expected_groups), relation_count


def nested_stage_sign_histogram() -> dict[str, dict[str, int]]:
    return {stage: {decision: 0 for decision in SIGNS} for stage in STAGES}


def audit(
    cases: Sequence[Case],
    normal: Sequence[dict[str, object]],
    multiprecision: Sequence[dict[str, object]],
) -> dict[str, object]:
    predicates = sorted(SCHEMAS)
    predicate_counts = {predicate: 0 for predicate in predicates}
    origin_histogram = {
        predicate: {origin: 0 for origin in ORIGINS} for predicate in predicates
    }
    stage_histogram = {
        predicate: {stage: 0 for stage in STAGES} for predicate in predicates
    }
    stage_sign_histogram = {
        predicate: nested_stage_sign_histogram() for predicate in predicates
    }
    fast_provenance_stage_sign_histogram = {
        predicate: nested_stage_sign_histogram() for predicate in predicates
    }
    intersection_kind_histogram = {
        "affine_family": 0,
        "empty": 0,
        "unique": 0,
    }
    rank_histogram: dict[str, int] = {}
    exact_zero_count = 0
    forced_count = 0

    for index, (case, observed, mp_observed) in enumerate(
        zip(cases, normal, multiprecision)
    ):
        for label, value in (("adaptive", observed), ("multiprecision", mp_observed)):
            if value.get("predicate") != case.predicate:
                raise AssertionError(
                    f"{label} case {index} returned {value.get('predicate')!r}"
                )
            projection = scientific_projection(value)
            if projection != case.expected:
                raise AssertionError(
                    f"{label} {case.predicate} case {index} differs from the "
                    f"Fraction/RREF oracle: expected {canonical_json(case.expected)}, "
                    f"observed {canonical_json(projection)}"
                )
        if scientific_projection(observed) != scientific_projection(mp_observed):
            raise AssertionError(
                f"{case.predicate} case {index} changed scientifically under "
                "--multiprecision-only"
            )

        stage = validate_stage_and_counters(observed, case, multiprecision_only=False)
        validate_stage_and_counters(mp_observed, case, multiprecision_only=True)
        decision = case_sign(case)
        predicate_counts[case.predicate] += 1
        stage_histogram[case.predicate][stage] += 1
        stage_sign_histogram[case.predicate][stage][decision] += 1
        if all(origin != "exact" for origin in case.origins):
            fast_provenance_stage_sign_histogram[case.predicate][stage][decision] += 1
        exact_zero_count += 1 if decision == "zero" else 0
        for origin in case.origins:
            origin_histogram[case.predicate][origin] += 1
        if case.predicate == "adaptive_intersect_three_planes":
            kind = str(case.expected["intersection_kind"])
            intersection_kind_histogram[kind] += 1
            rank_key = f"{case.expected['normal_rank']}/{case.expected['augmented_rank']}/{kind}"
            rank_histogram[rank_key] = rank_histogram.get(rank_key, 0) + 1
        if case.forced_stage is not None:
            forced_count += 1
            if stage != case.forced_stage or decision != case.forced_sign:
                raise AssertionError(
                    f"forced {case.predicate} case {index} expected "
                    f"{case.forced_stage}/{case.forced_sign}, observed {stage}/{decision}"
                )

    required = {
        ("fp64_filtered", "negative"),
        ("fp64_filtered", "positive"),
        ("expansion", "negative"),
        ("expansion", "zero"),
        ("expansion", "positive"),
        ("cpu_multiprecision", "negative"),
        ("cpu_multiprecision", "zero"),
        ("cpu_multiprecision", "positive"),
    }
    missing = [
        f"{predicate}:{stage}:{decision}"
        for predicate in predicates
        for stage, decision in sorted(required)
        if stage_sign_histogram[predicate][stage][decision] == 0
    ]
    if missing:
        raise AssertionError(
            "the corpus misses required adaptive affine classes: " + ", ".join(missing)
        )
    missing_fast_multiprecision = [
        f"{predicate}:cpu_multiprecision:{decision}"
        for predicate in predicates
        for decision in SIGNS
        if fast_provenance_stage_sign_histogram[predicate]["cpu_multiprecision"][
            decision
        ]
        == 0
    ]
    if missing_fast_multiprecision:
        raise AssertionError(
            "the corpus misses binary64-provenance fallbacks: "
            + ", ".join(missing_fast_multiprecision)
        )
    missing_origins = [
        f"{predicate}:{origin}"
        for predicate in predicates
        for origin in ORIGINS
        if origin_histogram[predicate][origin] == 0
    ]
    if missing_origins:
        raise AssertionError(
            "the corpus misses source origins: " + ", ".join(missing_origins)
        )
    if any(count == 0 for count in intersection_kind_histogram.values()):
        raise AssertionError(
            "the corpus does not cover unique, empty and affine-family ranks"
        )

    return {
        "adaptive_stage_histogram": stage_histogram,
        "adaptive_stage_sign_histogram": stage_sign_histogram,
        "binary64_provenance_stage_sign_histogram": (
            fast_provenance_stage_sign_histogram
        ),
        "exact_zero_decision_count": exact_zero_count,
        "forced_case_count": forced_count,
        "intersection_kind_histogram": intersection_kind_histogram,
        "intersection_rank_histogram": dict(sorted(rank_histogram.items())),
        "predicate_case_counts": predicate_counts,
        "source_origin_histogram": origin_histogram,
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
            "the default v8 affine command corpus changed without a generator update"
        )
    if EXPECTED_DEFAULT_ORACLE_SHA256 and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256:
        raise AssertionError(
            "the default v8 affine oracle changed without a generator update"
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
    metamorphic_group_count, metamorphic_relation_count = audit_metamorphic_relations(
        cases, normal
    )
    print(
        canonical_json(
            {
                "adaptive_multiprecision_scientific_projection_equal": True,
                "case_count": len(cases),
                "command_corpus_sha256": corpus_hash,
                "generator": "affine-adaptive-fraction-rref-splitmix64-v2",
                "metamorphic_group_count": metamorphic_group_count,
                "metamorphic_relation_count": metamorphic_relation_count,
                "oracle_sha256": oracle_hash,
                "seed": f"0x{SEED:016x}",
                **statistics,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
