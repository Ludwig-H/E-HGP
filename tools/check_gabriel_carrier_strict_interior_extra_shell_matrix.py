#!/usr/bin/env python3
"""Recertify the exact q=3 Gabriel-carrier strict-interior/shell matrix."""

from __future__ import annotations

import argparse
import json
import sys
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any, NoReturn


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = (
    REPOSITORY_ROOT
    / "tests"
    / "fixtures"
    / "regressions"
    / "gabriel_carrier_strict_interior_extra_shell_matrix.json"
)

Point3 = tuple[Fraction, Fraction, Fraction]


class ValidationError(RuntimeError):
    """Raised when the fixture or one of its exact claims is inconsistent."""


def fail(message: str) -> NoReturn:
    raise ValidationError(message)


def exact_object(value: Any, keys: set[str], path: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        fail(f"{path} does not respect its closed schema")
    return value


def integer(value: Any, path: str, *, minimum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        fail(f"{path} must be an integer")
    if minimum is not None and value < minimum:
        fail(f"{path} must be at least {minimum}")
    return value


def fraction(value: Any, path: str) -> Fraction:
    if isinstance(value, int) and not isinstance(value, bool):
        return Fraction(value)
    record = exact_object(value, {"denominator", "numerator"}, path)
    numerator = integer(record["numerator"], f"{path}.numerator")
    denominator = integer(
        record["denominator"], f"{path}.denominator", minimum=1
    )
    result = Fraction(numerator, denominator)
    if (result.numerator, result.denominator) != (numerator, denominator):
        fail(f"{path} must be a reduced canonical fraction")
    return result


def point(value: Any, path: str) -> Point3:
    if not isinstance(value, list) or len(value) != 3:
        fail(f"{path} must contain exactly three coordinates")
    return tuple(
        fraction(coordinate, f"{path}[{axis}]")
        for axis, coordinate in enumerate(value)
    )  # type: ignore[return-value]


def point_ids(
    value: Any,
    path: str,
    *,
    point_count: int,
    size: int | None = None,
) -> tuple[int, ...]:
    if not isinstance(value, list):
        fail(f"{path} must be an array")
    result = tuple(
        integer(point_id, f"{path}[{index}]", minimum=0)
        for index, point_id in enumerate(value)
    )
    if size is not None and len(result) != size:
        fail(f"{path} must contain exactly {size} point IDs")
    if result != tuple(sorted(set(result))):
        fail(f"{path} must be sorted and duplicate-free")
    if any(point_id >= point_count for point_id in result):
        fail(f"{path} contains an out-of-range point ID")
    return result


def simplex_ids(
    value: Any,
    path: str,
    *,
    point_count: int,
    target_cardinality: int,
) -> tuple[tuple[int, ...], ...]:
    if not isinstance(value, list):
        fail(f"{path} must be an array")
    result = tuple(
        point_ids(
            simplex,
            f"{path}[{index}]",
            point_count=point_count,
            size=target_cardinality,
        )
        for index, simplex in enumerate(value)
    )
    if result != tuple(sorted(set(result))):
        fail(f"{path} must be lexicographically sorted and duplicate-free")
    return result


def subtract(left: Point3, right: Point3) -> Point3:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def dot(left: Point3, right: Point3) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def squared_distance(left: Point3, right: Point3) -> Fraction:
    delta = subtract(left, right)
    return dot(delta, delta)


def pair_ball(points: tuple[Point3, ...], support: tuple[int, ...]) -> tuple[Point3, Fraction]:
    first, second = (points[point_id] for point_id in support)
    center = tuple((a + b) / 2 for a, b in zip(first, second))
    return center, squared_distance(first, second) / 4  # type: ignore[return-value]


def triangle_ball(
    points: tuple[Point3, ...], support: tuple[int, ...], path: str
) -> tuple[Point3, Fraction]:
    origin, first, second = (points[point_id] for point_id in support)
    edge1 = subtract(first, origin)
    edge2 = subtract(second, origin)
    gram11 = dot(edge1, edge1)
    gram12 = dot(edge1, edge2)
    gram22 = dot(edge2, edge2)
    determinant = gram11 * gram22 - gram12 * gram12
    if determinant <= 0:
        fail(f"{path} triangle support must be affinely independent")
    height1 = gram11
    height2 = gram22
    cramer1 = height1 * gram22 - gram12 * height2
    cramer2 = gram11 * height2 - height1 * gram12
    coefficient1 = cramer1 / (2 * determinant)
    coefficient2 = cramer2 / (2 * determinant)
    barycentric = (1 - coefficient1 - coefficient2, coefficient1, coefficient2)
    if not all(weight > 0 for weight in barycentric):
        fail(f"{path} triangle support must be strictly acute")
    center = tuple(
        origin[axis]
        + coefficient1 * edge1[axis]
        + coefficient2 * edge2[axis]
        for axis in range(3)
    )
    return center, squared_distance(center, origin)  # type: ignore[return-value]


def carried_simplices(
    support: tuple[int, ...],
    strict_interior: tuple[int, ...],
    extra_shell: tuple[int, ...],
    target_cardinality: int,
) -> tuple[tuple[int, ...], ...]:
    selected_shell_count = (
        target_cardinality - len(support) - len(strict_interior)
    )
    if selected_shell_count < 0 or selected_shell_count > len(extra_shell):
        return ()
    return tuple(
        sorted(
            tuple(sorted(support + strict_interior + selected_shell))
            for selected_shell in combinations(extra_shell, selected_shell_count)
        )
    )


def validate_case(
    raw_case: Any,
    path: str,
    target_cardinality: int,
) -> tuple[str, str, int, int, int, int]:
    case = exact_object(
        raw_case,
        {
            "ball",
            "case_id",
            "description",
            "expected",
            "point_labels",
            "points",
            "support_kind",
            "support_point_ids",
        },
        path,
    )
    case_id = case["case_id"]
    if not isinstance(case_id, str) or not case_id:
        fail(f"{path}.case_id must be a nonempty string")
    if not isinstance(case["description"], str) or not case["description"]:
        fail(f"{path}.description must be a nonempty string")
    labels = case["point_labels"]
    raw_points = case["points"]
    if (
        not isinstance(labels, list)
        or not labels
        or not all(isinstance(label, str) and label for label in labels)
        or len(labels) != len(set(labels))
    ):
        fail(f"{path}.point_labels must contain distinct nonempty labels")
    if not isinstance(raw_points, list) or len(raw_points) != len(labels):
        fail(f"{path}.points and point_labels must have the same nonzero size")
    points = tuple(
        point(raw_point, f"{path}.points[{index}]")
        for index, raw_point in enumerate(raw_points)
    )
    if len(points) != len(set(points)):
        fail(f"{path}.points must contain distinct coordinates")

    support_kind = case["support_kind"]
    if support_kind not in {"pair", "triangle"}:
        fail(f"{path}.support_kind must be pair or triangle")
    support_size = 2 if support_kind == "pair" else 3
    support = point_ids(
        case["support_point_ids"],
        f"{path}.support_point_ids",
        point_count=len(points),
        size=support_size,
    )
    if support_kind == "pair":
        computed_center, computed_level = pair_ball(points, support)
    else:
        computed_center, computed_level = triangle_ball(points, support, path)

    ball = exact_object(
        case["ball"], {"center", "squared_level"}, f"{path}.ball"
    )
    declared_center = point(ball["center"], f"{path}.ball.center")
    declared_level = fraction(
        ball["squared_level"], f"{path}.ball.squared_level"
    )
    if declared_level <= 0:
        fail(f"{path}.ball.squared_level must be positive")
    if (declared_center, declared_level) != (computed_center, computed_level):
        fail(f"{path} ball mismatch")

    powers = tuple(
        squared_distance(candidate, computed_center) - computed_level
        for candidate in points
    )
    strict_interior = tuple(
        point_id for point_id, power in enumerate(powers) if power < 0
    )
    shell = tuple(point_id for point_id, power in enumerate(powers) if power == 0)
    exterior = tuple(point_id for point_id, power in enumerate(powers) if power > 0)
    if not set(support).issubset(shell):
        fail(f"{path} support is not contained in its exact shell")
    extra_shell = tuple(point_id for point_id in shell if point_id not in support)
    closed_rank = len(strict_interior) + len(shell)
    closed_rank_at_most_target = closed_rank <= target_cardinality

    expected = exact_object(
        case["expected"],
        {
            "carried_simplex_count",
            "carried_simplex_ids",
            "closed_rank",
            "closed_rank_at_most_target_cardinality",
            "exterior_ids",
            "extra_shell_ids",
            "shell_ids",
            "strict_interior_ids",
        },
        f"{path}.expected",
    )
    expected_strict = point_ids(
        expected["strict_interior_ids"],
        f"{path}.expected.strict_interior_ids",
        point_count=len(points),
    )
    expected_shell = point_ids(
        expected["shell_ids"],
        f"{path}.expected.shell_ids",
        point_count=len(points),
    )
    expected_extra_shell = point_ids(
        expected["extra_shell_ids"],
        f"{path}.expected.extra_shell_ids",
        point_count=len(points),
    )
    expected_exterior = point_ids(
        expected["exterior_ids"],
        f"{path}.expected.exterior_ids",
        point_count=len(points),
    )
    if strict_interior != expected_strict:
        fail(f"{path} strict interior mismatch")
    if shell != expected_shell:
        fail(f"{path} shell mismatch")
    if extra_shell != expected_extra_shell:
        fail(f"{path} extra shell mismatch")
    if exterior != expected_exterior:
        fail(f"{path} exterior mismatch")
    if integer(expected["closed_rank"], f"{path}.expected.closed_rank") != closed_rank:
        fail(f"{path} closed rank mismatch")
    expected_rank_window = expected["closed_rank_at_most_target_cardinality"]
    if type(expected_rank_window) is not bool or expected_rank_window is not closed_rank_at_most_target:
        fail(f"{path} closed-rank window verdict mismatch")

    formula_simplices = carried_simplices(
        support, strict_interior, extra_shell, target_cardinality
    )
    closed_non_support = tuple(sorted(strict_interior + extra_shell))
    remaining_size = target_cardinality - len(support)
    brute_simplices: list[tuple[int, ...]] = []
    if 0 <= remaining_size <= len(closed_non_support):
        for selected in combinations(closed_non_support, remaining_size):
            simplex = tuple(sorted(support + selected))
            if not set(strict_interior).issubset(simplex):
                continue
            if any(
                power < 0 and point_id not in simplex
                for point_id, power in enumerate(powers)
            ):
                continue
            brute_simplices.append(simplex)
    if formula_simplices != tuple(brute_simplices):
        fail(f"{path} I-subset-Q formula disagrees with exact Gabriel scan")

    expected_count = integer(
        expected["carried_simplex_count"],
        f"{path}.expected.carried_simplex_count",
        minimum=0,
    )
    expected_simplices = simplex_ids(
        expected["carried_simplex_ids"],
        f"{path}.expected.carried_simplex_ids",
        point_count=len(points),
        target_cardinality=target_cardinality,
    )
    if expected_count != len(formula_simplices):
        fail(f"{path} carried simplex count mismatch")
    if expected_simplices != formula_simplices:
        fail(f"{path} carried simplex mismatch")
    return (
        case_id,
        support_kind,
        len(strict_interior),
        len(extra_shell),
        closed_rank,
        len(formula_simplices),
    )


def validate_fixture(payload: Any) -> None:
    root = exact_object(
        payload,
        {
            "cases",
            "description",
            "fixture_id",
            "kind",
            "schema_version",
            "target_cardinality",
        },
        "$",
    )
    if (
        root["schema_version"] != 1
        or root["kind"] != "morsehgp3d_exact_k2_gabriel_carrier_matrix"
        or root["fixture_id"]
        != "gabriel-carrier-strict-interior-extra-shell-matrix-v1"
    ):
        fail("fixture identity mismatch")
    if not isinstance(root["description"], str) or not root["description"]:
        fail("fixture description must be nonempty")
    target_cardinality = integer(
        root["target_cardinality"], "$.target_cardinality", minimum=1
    )
    if target_cardinality != 3:
        fail("fixture target cardinality must remain three")
    cases = root["cases"]
    if not isinstance(cases, list) or len(cases) != 4:
        fail("fixture must contain exactly four carrier cases")
    summaries = tuple(
        validate_case(case, f"$.cases[{index}]", target_cardinality)
        for index, case in enumerate(cases)
    )
    expected_summaries = (
        ("pair-zero-strict-two-shell", "pair", 0, 2, 4, 2),
        ("pair-one-strict-two-shell", "pair", 1, 2, 5, 1),
        ("pair-two-strict-one-shell", "pair", 2, 1, 5, 0),
        ("support3-zero-strict-one-shell", "triangle", 0, 1, 4, 1),
    )
    if summaries != expected_summaries:
        fail("fixture no longer covers the canonical strict-interior/shell matrix")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", nargs="?", type=Path, default=DEFAULT_FIXTURE)
    arguments = parser.parse_args()
    try:
        with arguments.fixture.open(encoding="utf-8") as stream:
            payload = json.load(stream)
        validate_fixture(payload)
    except (OSError, UnicodeError, json.JSONDecodeError, ValidationError) as error:
        print(f"Gabriel-carrier fixture validation failed: {error}", file=sys.stderr)
        return 1
    print("Validated four exact q=3 Gabriel-carrier strict-interior/shell cases.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
