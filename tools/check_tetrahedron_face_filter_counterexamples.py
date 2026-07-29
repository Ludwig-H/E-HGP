#!/usr/bin/env python3
"""Recertify that tetrahedron face acuteness cannot drive support-four search."""

from __future__ import annotations

import argparse
import json
import sys
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any, NoReturn


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.geometry import (
    barycentric_coordinates,
    circumball,
    minimum_enclosing_ball,
)


DEFAULT_FIXTURE = (
    REPOSITORY_ROOT
    / "tests"
    / "fixtures"
    / "regressions"
    / "tetrahedron_face_filter_counterexamples.json"
)


class ValidationError(RuntimeError):
    """Raised when the exact fixture or one of its claims is inconsistent."""


def fail(message: str) -> NoReturn:
    raise ValidationError(message)


def exact_object(value: Any, keys: set[str], path: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        fail(f"{path} does not respect its closed schema")
    return value


def integer(value: Any, path: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        fail(f"{path} must be an integer")
    return value


def fraction(value: Any, path: str) -> Fraction:
    if isinstance(value, int) and not isinstance(value, bool):
        return Fraction(value)
    record = exact_object(value, {"denominator", "numerator"}, path)
    numerator = integer(record["numerator"], f"{path}.numerator")
    denominator = integer(record["denominator"], f"{path}.denominator")
    if denominator <= 0:
        fail(f"{path}.denominator must be positive")
    result = Fraction(numerator, denominator)
    if (result.numerator, result.denominator) != (numerator, denominator):
        fail(f"{path} must be a reduced canonical fraction")
    return result


def fraction_tuple(value: Any, size: int, path: str) -> tuple[Fraction, ...]:
    if not isinstance(value, list) or len(value) != size:
        fail(f"{path} must contain exactly {size} scalars")
    return tuple(fraction(item, f"{path}[{index}]") for index, item in enumerate(value))


def point_ids(value: Any, size: int, path: str) -> tuple[int, ...]:
    if not isinstance(value, list) or len(value) != size:
        fail(f"{path} must contain exactly {size} point IDs")
    result = tuple(integer(item, f"{path}[{index}]") for index, item in enumerate(value))
    if result != tuple(sorted(set(result))) or any(item < 0 or item >= 4 for item in result):
        fail(f"{path} must be sorted, unique and inside 0..3")
    return result


def face_dot_products(
    points: tuple[tuple[Fraction, Fraction, Fraction], ...],
    face: tuple[int, int, int],
) -> tuple[Fraction, Fraction, Fraction]:
    products: list[Fraction] = []
    for vertex in face:
        others = tuple(point_id for point_id in face if point_id != vertex)
        first = tuple(points[others[0]][axis] - points[vertex][axis] for axis in range(3))
        second = tuple(points[others[1]][axis] - points[vertex][axis] for axis in range(3))
        products.append(sum((a * b for a, b in zip(first, second)), Fraction()))
    return tuple(products)  # type: ignore[return-value]


def angle_class(products: tuple[Fraction, Fraction, Fraction]) -> str:
    if all(value > 0 for value in products):
        return "acute"
    if any(value < 0 for value in products):
        return "obtuse"
    return "right"


def validate_case(raw_case: Any, path: str) -> tuple[bool, bool]:
    case = exact_object(
        raw_case,
        {
            "all_faces_acute",
            "case_id",
            "circumball",
            "face_audit",
            "minimum_enclosing_ball",
            "point_labels",
            "points",
            "support4_admissible",
        },
        path,
    )
    if not isinstance(case["case_id"], str) or not case["case_id"]:
        fail(f"{path}.case_id must be a nonempty string")
    labels = case["point_labels"]
    if not isinstance(labels, list) or len(labels) != 4 or len(set(labels)) != 4 or not all(isinstance(label, str) and label for label in labels):
        fail(f"{path}.point_labels must contain four distinct nonempty labels")
    raw_points = case["points"]
    if not isinstance(raw_points, list) or len(raw_points) != 4:
        fail(f"{path}.points must contain four points")
    points = tuple(
        fraction_tuple(point, 3, f"{path}.points[{index}]")
        for index, point in enumerate(raw_points)
    )
    if len(set(points)) != 4:
        fail(f"{path}.points must be distinct")

    ball = circumball(points, range(4))
    circumball_record = exact_object(
        case["circumball"],
        {"barycentric_coordinates", "center", "squared_level"},
        f"{path}.circumball",
    )
    expected_center = fraction_tuple(circumball_record["center"], 3, f"{path}.circumball.center")
    expected_level = fraction(circumball_record["squared_level"], f"{path}.circumball.squared_level")
    expected_barycentric = fraction_tuple(circumball_record["barycentric_coordinates"], 4, f"{path}.circumball.barycentric_coordinates")
    barycentric = barycentric_coordinates(ball.center, points)
    if ball.center != expected_center or ball.squared_radius != expected_level:
        fail(f"{path} circumball mismatch")
    if barycentric != expected_barycentric or sum(barycentric, Fraction()) != 1:
        fail(f"{path} barycentric mismatch")
    support4_admissible = all(value > 0 for value in barycentric)
    if type(case["support4_admissible"]) is not bool or case["support4_admissible"] is not support4_admissible:
        fail(f"{path} support-four verdict mismatch")

    face_records = case["face_audit"]
    expected_faces = tuple(combinations(range(4), 3))
    if not isinstance(face_records, list) or len(face_records) != len(expected_faces):
        fail(f"{path}.face_audit must contain all four faces")
    observed_classes: list[str] = []
    for index, (raw_face, expected_face) in enumerate(zip(face_records, expected_faces)):
        face_path = f"{path}.face_audit[{index}]"
        face_record = exact_object(raw_face, {"angle_class", "point_ids", "vertex_dot_products"}, face_path)
        face = point_ids(face_record["point_ids"], 3, f"{face_path}.point_ids")
        if face != expected_face:
            fail(f"{face_path} is not in canonical face order")
        products = face_dot_products(points, face)
        expected_products = fraction_tuple(face_record["vertex_dot_products"], 3, f"{face_path}.vertex_dot_products")
        classification = angle_class(products)
        if products != expected_products or face_record["angle_class"] != classification:
            fail(f"{face_path} angle audit mismatch")
        observed_classes.append(classification)
    all_faces_acute = all(value == "acute" for value in observed_classes)
    if type(case["all_faces_acute"]) is not bool or case["all_faces_acute"] is not all_faces_acute:
        fail(f"{path} all-faces-acute verdict mismatch")

    minimum = minimum_enclosing_ball(points)
    minimum_record = exact_object(
        case["minimum_enclosing_ball"],
        {"center", "squared_level", "support_point_ids"},
        f"{path}.minimum_enclosing_ball",
    )
    expected_support = point_ids(
        minimum_record["support_point_ids"],
        len(minimum_record["support_point_ids"]) if isinstance(minimum_record["support_point_ids"], list) else 0,
        f"{path}.minimum_enclosing_ball.support_point_ids",
    )
    expected_minimum_center = fraction_tuple(minimum_record["center"], 3, f"{path}.minimum_enclosing_ball.center")
    expected_minimum_level = fraction(minimum_record["squared_level"], f"{path}.minimum_enclosing_ball.squared_level")
    if (minimum.support_ids, minimum.center, minimum.squared_radius) != (
        expected_support,
        expected_minimum_center,
        expected_minimum_level,
    ):
        fail(f"{path} minimum enclosing ball mismatch")
    if support4_admissible != (minimum.support_ids == (0, 1, 2, 3)):
        fail(f"{path} support-four and miniball support disagree")
    return all_faces_acute, support4_admissible


def validate_fixture(payload: Any) -> None:
    root = exact_object(payload, {"cases", "description", "kind", "schema_version"}, "$")
    if root["schema_version"] != 1 or root["kind"] != "morsehgp3d_exact_tetrahedron_face_filter_counterexamples":
        fail("fixture identity mismatch")
    if not isinstance(root["description"], str) or not root["description"]:
        fail("fixture description must be nonempty")
    cases = root["cases"]
    if not isinstance(cases, list) or len(cases) != 2:
        fail("fixture must contain exactly two counterexamples")
    verdicts = tuple(validate_case(case, f"$.cases[{index}]") for index, case in enumerate(cases))
    if verdicts != ((False, True), (True, False)):
        fail("fixture does not prove both directions of face-filter failure")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", nargs="?", type=Path, default=DEFAULT_FIXTURE)
    args = parser.parse_args()
    try:
        with args.fixture.open(encoding="utf-8") as stream:
            payload = json.load(stream)
        validate_fixture(payload)
    except (OSError, UnicodeError, json.JSONDecodeError, ValidationError) as error:
        print(f"tetrahedron face-filter fixture rejected: {error}", file=sys.stderr)
        return 1
    print(f"Validated exact tetrahedron face-filter counterexamples: {args.fixture}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
