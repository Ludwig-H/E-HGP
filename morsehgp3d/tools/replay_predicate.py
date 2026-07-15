#!/usr/bin/env python3
"""Replay one certified CPU predicate from exact binary64 input words."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import struct
import subprocess
import sys
from fractions import Fraction
from pathlib import Path
from typing import Any


INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
SCHEMA_VERSIONS = {1, 2, 3, 4, 5}
DOMAINS = {
    1: b"MorseHGP3D/predicate-replay-v1/",
    2: b"MorseHGP3D/predicate-replay-v2/",
    3: b"MorseHGP3D/predicate-replay-v3/",
    4: b"MorseHGP3D/predicate-replay-v4/",
    5: b"MorseHGP3D/predicate-replay-v5/",
}
POINT_COUNTS = {
    "compare_squared_distances": 3,
    "orientation_3d": 4,
}
HEX_WORD = re.compile(r"^[0-9a-f]{16}$")
CANONICAL_INTEGER = re.compile(r"^(?:0|-?[1-9][0-9]*)$")
CANONICAL_POSITIVE_INTEGER = re.compile(r"^[1-9][0-9]*$")
SIGNS = {"negative", "zero", "positive"}
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}
NATIVE_EXECUTABLE = "morsehgp3d_replay_predicate" + (".exe" if os.name == "nt" else "")
NATIVE_TIMEOUT_SECONDS = 30


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def strict_json_loads(text: str) -> Any:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite_constant(value: str) -> None:
        raise ValueError(f"non-finite JSON number is forbidden: {value}")

    return json.loads(
        text,
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite_constant,
    )


def validate_points(points: Any, label: str) -> list[list[str]]:
    if not isinstance(points, list):
        raise ValueError(f"the replay {label} must be a point array")
    normalized_points: list[list[str]] = []
    for point in points:
        if not isinstance(point, list) or len(point) != 3:
            raise ValueError("every replay point must contain three binary64 words")
        normalized_point: list[str] = []
        for word in point:
            if not isinstance(word, str) or HEX_WORD.fullmatch(word) is None:
                raise ValueError("binary64 words must use 16 lowercase hexadecimal digits")
            bits = int(word, 16)
            if ((bits >> 52) & 0x7FF) == 0x7FF:
                raise ValueError("binary64 replay coordinates must be finite")
            normalized_point.append(word)
        normalized_points.append(normalized_point)
    return normalized_points


def validate_exact_rational3(value: Any, label: str) -> dict[str, str]:
    fields = {
        "denominator",
        "schema_version",
        "unit",
        "x_numerator",
        "y_numerator",
        "z_numerator",
    }
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"the replay {label} is not a closed ExactRational3 object")
    if value["schema_version"] != "2.0.0" or value["unit"] != "input_coordinate_unit":
        raise ValueError(f"the replay {label} has an unsupported ExactRational3 contract")
    denominator = value["denominator"]
    numerators = [value[f"{axis}_numerator"] for axis in "xyz"]
    if (
        not isinstance(denominator, str)
        or CANONICAL_POSITIVE_INTEGER.fullmatch(denominator) is None
        or any(
            not isinstance(numerator, str)
            or CANONICAL_INTEGER.fullmatch(numerator) is None
            for numerator in numerators
        )
    ):
        raise ValueError(f"the replay {label} has noncanonical exact integers")
    divisor = int(denominator)
    for numerator in numerators:
        divisor = math.gcd(divisor, abs(int(numerator)))
    if divisor != 1:
        raise ValueError(f"the replay {label} is not reduced canonically")
    return {key: value[key] for key in sorted(fields)}


def validate_exact_level_record(value: Any, label: str) -> dict[str, str]:
    fields = {"denominator", "numerator", "schema_version", "unit"}
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"the replay {label} is not a closed ExactLevel object")
    if (
        value["schema_version"] != "2.0.0"
        or value["unit"] != "input_coordinate_unit_squared"
    ):
        raise ValueError(f"the replay {label} has an unsupported ExactLevel contract")
    numerator = value["numerator"]
    denominator = value["denominator"]
    if (
        not isinstance(numerator, str)
        or CANONICAL_INTEGER.fullmatch(numerator) is None
        or int(numerator) < 0
        or not isinstance(denominator, str)
        or CANONICAL_POSITIVE_INTEGER.fullmatch(denominator) is None
        or math.gcd(int(numerator), int(denominator)) != 1
    ):
        raise ValueError(f"the replay {label} is not a canonical nonnegative level")
    return {key: value[key] for key in sorted(fields)}


def validate_plane(value: Any, label: str) -> dict[str, str]:
    fields = {"a", "b", "c", "d", "schema_version"}
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"the replay {label} is not a closed ExactPlane3 object")
    if value["schema_version"] != "2.0.0":
        raise ValueError(f"the replay {label} has an unsupported ExactPlane3 contract")
    coefficients: list[int] = []
    for field in ("a", "b", "c", "d"):
        coefficient = value[field]
        if (
            not isinstance(coefficient, str)
            or CANONICAL_INTEGER.fullmatch(coefficient) is None
        ):
            raise ValueError(f"the replay {label} has noncanonical exact integers")
        coefficients.append(int(coefficient))
    if coefficients[:3] == [0, 0, 0]:
        raise ValueError(f"the replay {label} has a zero normal")
    divisor = 0
    for coefficient in coefficients:
        divisor = math.gcd(divisor, abs(coefficient))
    if divisor != 1:
        raise ValueError(f"the replay {label} is not primitive canonically")
    return {key: value[key] for key in sorted(fields)}


def validate_label_ids(value: Any, point_count: int, label: str) -> list[int]:
    if not isinstance(value, list) or not 1 <= len(value) <= 10:
        raise ValueError(
            f"the replay {label} must contain between one and ten identifiers"
        )
    if any(type(identifier) is not int for identifier in value):
        raise ValueError(f"the replay {label} identifiers must be integers")
    if any(identifier < 0 or identifier >= point_count for identifier in value):
        raise ValueError(f"the replay {label} contains an out-of-range identifier")
    if any(left >= right for left, right in zip(value, value[1:])):
        raise ValueError(f"the replay {label} identifiers must be sorted and unique")
    return list(value)


def validate_input(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError("the replay input must be a JSON object")
    if (
        value.get("kind") != INPUT_KIND
        or type(value.get("schema_version")) is not int
        or value["schema_version"] not in SCHEMA_VERSIONS
        or not isinstance(value.get("predicate"), str)
    ):
        raise ValueError("the replay input kind, schema_version, or predicate is unsupported")

    schema_version = value["schema_version"]
    predicate = value["predicate"]
    if schema_version == 1:
        required = {"kind", "schema_version", "predicate", "points"}
        if set(value) != required or predicate not in POINT_COUNTS:
            raise ValueError("the v1 replay input has missing or unknown fields")
        points = validate_points(value["points"], "points")
        if len(points) != POINT_COUNTS[predicate]:
            raise ValueError("the replay input has the wrong point count")
        return {
            "kind": INPUT_KIND,
            "schema_version": 1,
            "predicate": predicate,
            "points": points,
        }

    if schema_version == 2:
        required = {
            "kind",
            "schema_version",
            "predicate",
            "point_table",
            "q_ids",
            "r_ids",
            "witness",
        }
        if set(value) != required or predicate != "power_bisector_side":
            raise ValueError("the v2 replay input has missing, unknown, or unsupported fields")
        point_table = validate_points(value["point_table"], "point_table")
        q_ids = validate_label_ids(value["q_ids"], len(point_table), "Q label")
        r_ids = validate_label_ids(value["r_ids"], len(point_table), "R label")
        if len(q_ids) != len(r_ids):
            raise ValueError("the replay power labels must have the same cardinality")
        return {
            "kind": INPUT_KIND,
            "schema_version": 2,
            "predicate": predicate,
            "point_table": point_table,
            "q_ids": q_ids,
            "r_ids": r_ids,
            "witness": validate_exact_rational3(value["witness"], "witness"),
        }

    if schema_version == 4:
        required = {"kind", "schema_version", "predicate", "points"}
        if set(value) != required or predicate != "circumcenter_support":
            raise ValueError("the v4 replay input has missing, unknown, or unsupported fields")
        points = validate_points(value["points"], "points")
        if not 2 <= len(points) <= 4:
            raise ValueError("circumcenter construction requires between two and four points")
        return {
            "kind": INPUT_KIND,
            "schema_version": 4,
            "predicate": predicate,
            "points": points,
        }

    if schema_version == 5:
        if predicate == "circumcenter_support_analysis":
            required = {"kind", "schema_version", "predicate", "points"}
            if set(value) != required:
                raise ValueError(
                    "the v5 support-analysis input has missing or unknown fields"
                )
            points = validate_points(value["points"], "points")
            if not 1 <= len(points) <= 4:
                raise ValueError(
                    "circumcenter support analysis requires between one and four points"
                )
            return {
                "kind": INPUT_KIND,
                "schema_version": 5,
                "predicate": predicate,
                "points": points,
            }
        if predicate == "sphere_side":
            required = {
                "center_exact",
                "kind",
                "point",
                "predicate",
                "schema_version",
                "squared_level_exact",
            }
            if set(value) != required:
                raise ValueError("the v5 sphere-side input has missing or unknown fields")
            point = validate_points([value["point"]], "point")[0]
            return {
                "center_exact": validate_exact_rational3(
                    value["center_exact"], "sphere center"
                ),
                "kind": INPUT_KIND,
                "point": point,
                "predicate": predicate,
                "schema_version": 5,
                "squared_level_exact": validate_exact_level_record(
                    value["squared_level_exact"], "sphere squared level"
                ),
            }
        raise ValueError("the v5 replay predicate is unsupported")

    if predicate == "plane_through_points":
        required = {"kind", "schema_version", "predicate", "points"}
        if set(value) != required:
            raise ValueError("the v3 plane input has missing or unknown fields")
        points = validate_points(value["points"], "points")
        if len(points) != 3:
            raise ValueError("plane construction requires exactly three points")
        return {
            "kind": INPUT_KIND,
            "schema_version": 3,
            "predicate": predicate,
            "points": points,
        }

    if predicate == "power_bisector_affine_form":
        required = {
            "kind",
            "schema_version",
            "predicate",
            "point_table",
            "q_ids",
            "r_ids",
        }
        if set(value) != required:
            raise ValueError("the v3 affine-form input has missing or unknown fields")
        point_table = validate_points(value["point_table"], "point_table")
        q_ids = validate_label_ids(value["q_ids"], len(point_table), "Q label")
        r_ids = validate_label_ids(value["r_ids"], len(point_table), "R label")
        if len(q_ids) != len(r_ids):
            raise ValueError("the replay power labels must have the same cardinality")
        return {
            "kind": INPUT_KIND,
            "schema_version": 3,
            "predicate": predicate,
            "point_table": point_table,
            "q_ids": q_ids,
            "r_ids": r_ids,
        }

    if predicate == "orientation_2d_in_plane":
        required = {"kind", "schema_version", "predicate", "plane", "points"}
        if set(value) != required:
            raise ValueError("the v3 orientation input has missing or unknown fields")
        points = validate_points(value["points"], "points")
        if len(points) != 3:
            raise ValueError("orientation 2D requires exactly three points")
        return {
            "kind": INPUT_KIND,
            "schema_version": 3,
            "predicate": predicate,
            "plane": validate_plane(value["plane"], "support plane"),
            "points": points,
        }

    plane_counts = {"intersect_three_planes": 3, "fourth_plane_incidence": 4}
    if predicate in plane_counts:
        required = {"kind", "schema_version", "predicate", "planes"}
        if set(value) != required or not isinstance(value["planes"], list):
            raise ValueError("the v3 plane-system input has missing or unknown fields")
        if len(value["planes"]) != plane_counts[predicate]:
            raise ValueError("the v3 plane system has the wrong plane count")
        return {
            "kind": INPUT_KIND,
            "schema_version": 3,
            "predicate": predicate,
            "planes": [
                validate_plane(plane, f"plane {index}")
                for index, plane in enumerate(value["planes"])
            ],
        }

    raise ValueError("the v3 replay predicate is unsupported")


def replay_id(value: dict[str, Any]) -> str:
    digest = hashlib.sha256()
    digest.update(DOMAINS[value["schema_version"]])
    digest.update(canonical_json(value).encode("utf-8"))
    return digest.hexdigest()


def checked_executable(candidate: Path, source: str) -> Path:
    try:
        resolved = candidate.expanduser().resolve(strict=True)
    except OSError as error:
        raise FileNotFoundError(f"{source} replay executable does not exist: {candidate}") from error
    if not resolved.is_file() or not os.access(resolved, os.X_OK):
        raise PermissionError(f"{source} replay executable is not an executable file: {resolved}")
    return resolved


def locate_executable(explicit: Path | None) -> Path:
    if explicit is not None:
        return checked_executable(explicit, "explicit")
    environment = os.environ.get("MORSEHGP3D_REPLAY_BIN")
    if environment:
        return checked_executable(Path(environment), "MORSEHGP3D_REPLAY_BIN")

    candidates = [Path(sys.argv[0]).resolve().with_name(NATIVE_EXECUTABLE)]
    repository = Path(__file__).resolve().parents[2]
    candidates.extend(
        [
            repository / "build/morsehgp3d" / NATIVE_EXECUTABLE,
            repository / "build/morsehgp3d-package" / NATIVE_EXECUTABLE,
        ]
    )
    for candidate in candidates:
        try:
            return checked_executable(candidate, "auto-discovered")
        except (FileNotFoundError, PermissionError):
            continue
    raise FileNotFoundError("morsehgp3d_replay_predicate executable was not found")


def canonical_rational(value: Any, label: str, *, nonnegative: bool) -> Fraction:
    if not isinstance(value, dict) or set(value) != {"denominator", "numerator"}:
        raise ValueError(f"the native {label} is not a closed rational object")
    numerator = value["numerator"]
    denominator = value["denominator"]
    if not isinstance(numerator, str) or CANONICAL_INTEGER.fullmatch(numerator) is None:
        raise ValueError(f"the native {label} numerator is not canonical")
    if (
        not isinstance(denominator, str)
        or CANONICAL_POSITIVE_INTEGER.fullmatch(denominator) is None
    ):
        raise ValueError(f"the native {label} denominator is not canonical positive")
    numerator_value = int(numerator)
    denominator_value = int(denominator)
    if math.gcd(abs(numerator_value), denominator_value) != 1:
        raise ValueError(f"the native {label} rational is not reduced")
    if nonnegative and numerator_value < 0:
        raise ValueError(f"the native {label} must be nonnegative")
    return Fraction(numerator_value, denominator_value)


def exact_affine_form(
    value: Any, label: str
) -> tuple[Fraction, Fraction, Fraction, Fraction]:
    fields = {"a", "b", "c", "d", "schema_version"}
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"the native {label} is not a closed exact affine form")
    if value["schema_version"] != "2.0.0":
        raise ValueError(f"the native {label} has an unsupported affine-form contract")
    return tuple(
        canonical_rational(value[field], f"{label} coefficient {field}", nonnegative=False)
        for field in ("a", "b", "c", "d")
    )  # type: ignore[return-value]


def canonical_level(value: Any, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != {
        "denominator",
        "numerator",
        "schema_version",
        "unit",
    }:
        raise ValueError(f"the native {label} is not a closed ExactLevel object")
    if value["schema_version"] != "2.0.0" or value["unit"] != "input_coordinate_unit_squared":
        raise ValueError(f"the native {label} has an unsupported ExactLevel contract")
    return canonical_rational(
        {"denominator": value["denominator"], "numerator": value["numerator"]},
        label,
        nonnegative=True,
    )


def rational3(value: Any, label: str) -> tuple[Fraction, Fraction, Fraction]:
    record = validate_exact_rational3(value, label)
    denominator = int(record["denominator"])
    return tuple(
        Fraction(int(record[f"{axis}_numerator"]), denominator) for axis in "xyz"
    )  # type: ignore[return-value]


def point_from_words(words: list[str]) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(
        Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0]) for word in words
    )  # type: ignore[return-value]


def primitive_integer_coefficients(
    coefficients: tuple[Fraction, Fraction, Fraction, Fraction],
) -> tuple[int, int, int, int]:
    common_denominator = 1
    for coefficient in coefficients:
        common_denominator = math.lcm(common_denominator, coefficient.denominator)
    integers = [
        coefficient.numerator * (common_denominator // coefficient.denominator)
        for coefficient in coefficients
    ]
    divisor = 0
    for coefficient in integers:
        divisor = math.gcd(divisor, abs(coefficient))
    if divisor != 0:
        integers = [coefficient // divisor for coefficient in integers]
    return tuple(integers)  # type: ignore[return-value]


def plane_record(
    coefficients: tuple[Fraction, Fraction, Fraction, Fraction],
) -> dict[str, str]:
    primitive = primitive_integer_coefficients(coefficients)
    if primitive[:3] == (0, 0, 0):
        raise ValueError("a zero normal does not define an exact plane")
    return {
        "a": str(primitive[0]),
        "b": str(primitive[1]),
        "c": str(primitive[2]),
        "d": str(primitive[3]),
        "schema_version": "2.0.0",
    }


def plane_coefficients(record: dict[str, str]) -> tuple[Fraction, Fraction, Fraction, Fraction]:
    return tuple(Fraction(int(record[field])) for field in ("a", "b", "c", "d"))  # type: ignore[return-value]


def cross_product(
    left: tuple[Fraction, Fraction, Fraction],
    right: tuple[Fraction, Fraction, Fraction],
) -> tuple[Fraction, Fraction, Fraction]:
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def plane_through_points(
    points: list[tuple[Fraction, Fraction, Fraction]],
) -> dict[str, str]:
    a, b, c = points
    first = tuple(bi - ai for ai, bi in zip(a, b))
    second = tuple(ci - ai for ai, ci in zip(a, c))
    normal = cross_product(first, second)
    offset = -sum((coefficient * coordinate for coefficient, coordinate in zip(normal, a)), Fraction())
    return plane_record((normal[0], normal[1], normal[2], offset))


def evaluate_plane(
    plane: dict[str, str], point: tuple[Fraction, Fraction, Fraction]
) -> Fraction:
    a, b, c, d = plane_coefficients(plane)
    return a * point[0] + b * point[1] + c * point[2] + d


def power_bisector_classification(
    replay_input: dict[str, Any],
) -> tuple[
    str,
    dict[str, str] | None,
    tuple[Fraction, Fraction, Fraction, Fraction],
]:
    point_table = [point_from_words(words) for words in replay_input["point_table"]]
    r_points = [point_table[identifier] for identifier in replay_input["r_ids"]]
    q_points = [point_table[identifier] for identifier in replay_input["q_ids"]]
    delta = tuple(
        sum((value[axis] for value in r_points), Fraction())
        - sum((value[axis] for value in q_points), Fraction())
        for axis in range(3)
    )
    delta_norm = sum(
        (sum((coordinate * coordinate for coordinate in value), Fraction()) for value in r_points),
        Fraction(),
    ) - sum(
        (sum((coordinate * coordinate for coordinate in value), Fraction()) for value in q_points),
        Fraction(),
    )
    coefficients = (-2 * delta[0], -2 * delta[1], -2 * delta[2], delta_norm)
    if coefficients[:3] != (Fraction(), Fraction(), Fraction()):
        return "proper_plane", plane_record(coefficients), coefficients
    if coefficients[3] < 0:
        return "constant_negative", None, coefficients
    if coefficients[3] > 0:
        return "constant_positive", None, coefficients
    return "identically_zero", None, coefficients


def matrix_rref(matrix: list[list[Fraction]]) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [list(row) for row in matrix]
    pivot_columns: list[int] = []
    pivot_row = 0
    column_count = len(reduced[0]) if reduced else 0
    for column in range(column_count):
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
        pivot_columns.append(column)
        pivot_row += 1
        if pivot_row == len(reduced):
            break
    return reduced, pivot_columns


def solve_three_planes(
    planes: list[dict[str, str]],
) -> tuple[
    str,
    tuple[Fraction, Fraction, Fraction] | None,
    int,
    int,
    int | None,
]:
    normals: list[list[Fraction]] = []
    augmented: list[list[Fraction]] = []
    for plane in planes:
        a, b, c, d = plane_coefficients(plane)
        normals.append([a, b, c])
        augmented.append([a, b, c, -d])
    _, normal_pivots = matrix_rref(normals)
    reduced, augmented_pivots = matrix_rref(augmented)
    normal_rank = len(normal_pivots)
    augmented_rank = len(augmented_pivots)
    if normal_rank < augmented_rank:
        return "empty", None, normal_rank, augmented_rank, None
    if normal_rank < 3:
        return "affine_family", None, normal_rank, augmented_rank, 3 - normal_rank
    coordinates = [Fraction(), Fraction(), Fraction()]
    for row, column in enumerate(augmented_pivots):
        if column < 3:
            coordinates[column] = reduced[row][3]
    return "unique", tuple(coordinates), normal_rank, augmented_rank, 0  # type: ignore[return-value]


def exact_rational3_record(
    point: tuple[Fraction, Fraction, Fraction]
) -> dict[str, str]:
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


def squared_distance(
    left: tuple[Fraction, Fraction, Fraction],
    right: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(((a - b) ** 2 for a, b in zip(left, right)), Fraction())


def exact_rational_record(value: Fraction) -> dict[str, str]:
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
    }


def exact_level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise ValueError("an exact squared level cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def independent_support_witness(
    points: list[tuple[Fraction, Fraction, Fraction]],
) -> tuple[
    tuple[Fraction, Fraction, Fraction],
    Fraction,
    list[Fraction],
]:
    """Solve the bordered barycentric system independently of the C++ path."""

    size = len(points)
    gram = [
        [
            sum((a * b for a, b in zip(left, right)), Fraction())
            for right in points
        ]
        for left in points
    ]
    system = [
        [2 * entry for entry in row]
        + [Fraction(-1), gram[index][index]]
        for index, row in enumerate(gram)
    ]
    system.append([Fraction(1) for _ in range(size)] + [Fraction(), Fraction(1)])
    reduced, pivots = matrix_rref(system)
    if pivots != list(range(size + 1)):
        raise ValueError(
            "an affinely independent support produced a singular bordered system"
        )
    barycentric = [reduced[index][-1] for index in range(size)]
    if sum(barycentric, Fraction()) != 1:
        raise ValueError("the exact bordered system did not produce affine weights")
    center = tuple(
        sum(
            (barycentric[index] * points[index][axis] for index in range(size)),
            Fraction(),
        )
        for axis in range(3)
    )
    squared_level = squared_distance(center, points[0])
    if any(squared_distance(center, point) != squared_level for point in points):
        raise ValueError("the exact bordered-system center is not equidistant")
    return center, squared_level, barycentric  # type: ignore[return-value]


def empty_predicate_counters() -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 0,
        "exact_zeros": 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def support_analysis_oracle(replay_input: dict[str, Any]) -> dict[str, Any]:
    points = [point_from_words(words) for words in replay_input["points"]]
    directions = [
        [point[axis] - points[0][axis] for axis in range(3)]
        for point in points[1:]
    ]
    _, affine_pivots = matrix_rref(directions)
    affine_dimension = len(affine_pivots)
    independent = affine_dimension + 1 == len(points)
    if not independent:
        return {
            "affine_dimension": affine_dimension,
            "barycentric_coordinates_exact": None,
            "barycentric_signs": None,
            "center_exact": None,
            "certification_stage": None,
            "convex_hull_location": None,
            "counters": empty_predicate_counters(),
            "predicate": "circumcenter_support_analysis",
            "reduced_support_indices": None,
            "squared_level_exact": None,
            "support_kind": "affinely_dependent",
            "support_size": len(points),
            "support_status": "affinely_dependent",
        }

    center, squared_level, barycentric = independent_support_witness(points)
    signs = [expected_sign(coordinate) for coordinate in barycentric]
    has_negative = "negative" in signs
    has_zero = "zero" in signs
    if has_negative:
        location = "exterior"
        status = "exterior_circumcenter"
        reduced_indices = None
    elif has_zero:
        location = "relative_boundary"
        status = "boundary_reduced"
        reduced_indices = [
            index for index, sign in enumerate(signs) if sign == "positive"
        ]
        if not reduced_indices:
            raise ValueError("an exact boundary support has no positive weight")
        reduced_points = [points[index] for index in reduced_indices]
        reduced_center, reduced_level, reduced_barycentric = (
            independent_support_witness(reduced_points)
        )
        if (
            reduced_center != center
            or reduced_level != squared_level
            or any(coordinate <= 0 for coordinate in reduced_barycentric)
        ):
            raise ValueError("the exact boundary reduction changed its sphere")
    else:
        location = "relative_interior"
        status = "minimal"
        reduced_indices = list(range(len(points)))

    counters = empty_predicate_counters()
    counters["cpu_multiprecision_certified"] = len(points)
    counters["exact_zeros"] = signs.count("zero")
    return {
        "affine_dimension": affine_dimension,
        "barycentric_coordinates_exact": [
            exact_rational_record(coordinate) for coordinate in barycentric
        ],
        "barycentric_signs": signs,
        "center_exact": exact_rational3_record(center),
        "certification_stage": "cpu_multiprecision",
        "convex_hull_location": location,
        "counters": counters,
        "predicate": "circumcenter_support_analysis",
        "reduced_support_indices": reduced_indices,
        "squared_level_exact": exact_level_record(squared_level),
        "support_kind": "affinely_independent",
        "support_size": len(points),
        "support_status": status,
    }


def sphere_side_oracle(replay_input: dict[str, Any]) -> dict[str, Any]:
    center = rational3(replay_input["center_exact"], "sphere center")
    point = point_from_words(replay_input["point"])
    squared_level = canonical_level(
        replay_input["squared_level_exact"], "sphere squared level"
    )
    distance = squared_distance(center, point)
    offset = distance - squared_level
    sign = expected_sign(offset)
    classification = {
        "negative": "strictly_inside",
        "zero": "boundary",
        "positive": "outside",
    }[sign]
    counters = empty_predicate_counters()
    counters["cpu_multiprecision_certified"] = 1
    counters["exact_zeros"] = 1 if sign == "zero" else 0
    return {
        "certification_stage": "cpu_multiprecision",
        "classification": classification,
        "counters": counters,
        "predicate": "sphere_side",
        "sign": sign,
        "signed_offset_exact": exact_rational_record(offset),
        "squared_distance_exact": exact_level_record(distance),
    }


def circumcenter_support_oracle(
    replay_input: dict[str, Any],
) -> tuple[
    str,
    int,
    tuple[Fraction, Fraction, Fraction] | None,
    Fraction | None,
]:
    points = [point_from_words(words) for words in replay_input["points"]]
    origin = points[0]
    differences = [
        [coordinate - origin[axis] for axis, coordinate in enumerate(point)]
        for point in points[1:]
    ]
    _, affine_pivots = matrix_rref(differences)
    affine_dimension = len(affine_pivots)
    expected_dimension = len(points) - 1
    if affine_dimension != expected_dimension:
        return "affinely_dependent", affine_dimension, None, None

    gram = [
        [
            sum(
                (left * right for left, right in zip(first, second)),
                Fraction(),
            )
            for second in differences
        ]
        for first in differences
    ]
    system = [
        [2 * entry for entry in row] + [gram[index][index]]
        for index, row in enumerate(gram)
    ]
    reduced, system_pivots = matrix_rref(system)
    if system_pivots != list(range(expected_dimension)):
        raise ValueError("an affinely independent support produced a singular Gram system")
    coefficients = [reduced[index][-1] for index in range(expected_dimension)]
    center = tuple(
        origin[axis]
        + sum(
            (
                coefficients[index] * differences[index][axis]
                for index in range(expected_dimension)
            ),
            Fraction(),
        )
        for axis in range(3)
    )
    squared_level = squared_distance(center, origin)
    gram_level = sum(
        (
            gram[index][index] * coefficients[index]
            for index in range(expected_dimension)
        ),
        Fraction(),
    ) / 2
    if squared_level != gram_level or squared_level <= 0:
        raise ValueError("the exact Gram center produced an invalid squared level")
    if any(squared_distance(center, point) != squared_level for point in points[1:]):
        raise ValueError("the exact Gram center is not equidistant from its support")
    return "affinely_independent", affine_dimension, center, squared_level


def orientation_determinant(
    points: list[tuple[Fraction, Fraction, Fraction]],
) -> Fraction:
    a, b, c, d = points
    u = tuple(bi - ai for ai, bi in zip(a, b))
    v = tuple(ci - ai for ai, ci in zip(a, c))
    w = tuple(di - ai for ai, di in zip(a, d))
    return (
        u[0] * (v[1] * w[2] - v[2] * w[1])
        - u[1] * (v[0] * w[2] - v[2] * w[0])
        + u[2] * (v[0] * w[1] - v[1] * w[0])
    )


def expected_sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def validate_counters(value: Any, sign: str, stage: str, predicate: str) -> None:
    if not isinstance(value, dict) or set(value) != COUNTER_FIELDS:
        raise ValueError("the native predicate counters do not match PredicateCounts v2")
    if any(type(count) is not int or count < 0 for count in value.values()):
        raise ValueError("the native predicate counters must be nonnegative integers")
    exact_only = {
        "power_bisector_side",
        "orientation_2d_in_plane",
        "fourth_plane_incidence",
        "sphere_side",
    }
    allowed_stages = (
        {"cpu_multiprecision"}
        if predicate in exact_only
        else {"fp64_filtered", "cpu_multiprecision"}
    )
    if stage not in allowed_stages:
        raise ValueError("the native predicate used an unavailable certification stage")
    if sign == "zero" and stage == "fp64_filtered":
        raise ValueError("an FP64 filter cannot certify an exact zero")
    expected = {
        "cpu_multiprecision_certified": 1 if stage == "cpu_multiprecision" else 0,
        "exact_zeros": 1 if sign == "zero" else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 1 if stage == "fp64_filtered" else 0,
        "remaining_unknown": 0,
    }
    if value != expected:
        raise ValueError("the native predicate counters disagree with its certification stage")


def validate_native_result(value: Any, replay_input: dict[str, Any]) -> dict[str, Any]:
    predicate = replay_input["predicate"]
    if predicate == "circumcenter_support_analysis":
        expected_fields = {
            "affine_dimension",
            "barycentric_coordinates_exact",
            "barycentric_signs",
            "center_exact",
            "certification_stage",
            "convex_hull_location",
            "counters",
            "predicate",
            "reduced_support_indices",
            "squared_level_exact",
            "support_kind",
            "support_size",
            "support_status",
        }
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native support analysis returned unknown fields")
        if (
            type(value["affine_dimension"]) is not int
            or type(value["support_size"]) is not int
            or value["certification_stage"] not in (None, "cpu_multiprecision")
            or value["convex_hull_location"]
            not in (None, "relative_interior", "relative_boundary", "exterior")
            or value["support_kind"]
            not in ("affinely_independent", "affinely_dependent")
            or value["support_status"]
            not in (
                "minimal",
                "boundary_reduced",
                "exterior_circumcenter",
                "affinely_dependent",
            )
        ):
            raise ValueError("the native support analysis has invalid classifications")
        barycentric = value["barycentric_coordinates_exact"]
        signs = value["barycentric_signs"]
        if barycentric is not None:
            if not isinstance(barycentric, list) or len(barycentric) != value["support_size"]:
                raise ValueError("the native support analysis has invalid barycentric size")
            for index, coordinate in enumerate(barycentric):
                canonical_rational(
                    coordinate, f"barycentric coordinate {index}", nonnegative=False
                )
        if signs is not None and (
            not isinstance(signs, list)
            or len(signs) != value["support_size"]
            or any(not isinstance(sign, str) or sign not in SIGNS for sign in signs)
        ):
            raise ValueError("the native support analysis has invalid barycentric signs")
        reduced = value["reduced_support_indices"]
        if reduced is not None and (
            not isinstance(reduced, list)
            or any(type(index) is not int for index in reduced)
            or any(index < 0 or index >= value["support_size"] for index in reduced)
            or any(left >= right for left, right in zip(reduced, reduced[1:]))
        ):
            raise ValueError("the native reduced support indices are not canonical")
        if value["center_exact"] is not None:
            validate_exact_rational3(value["center_exact"], "native analyzed center")
        if value["squared_level_exact"] is not None:
            canonical_level(value["squared_level_exact"], "native analyzed level")
        counters = value["counters"]
        if (
            not isinstance(counters, dict)
            or set(counters) != COUNTER_FIELDS
            or any(type(count) is not int or count < 0 for count in counters.values())
        ):
            raise ValueError("the native support counters are not PredicateCounts v2")
        expected = support_analysis_oracle(replay_input)
        if value != expected:
            raise ValueError(
                "the native support analysis differs from the exact bordered-system oracle"
            )
        return value

    if predicate == "sphere_side":
        expected_fields = {
            "certification_stage",
            "classification",
            "counters",
            "predicate",
            "sign",
            "signed_offset_exact",
            "squared_distance_exact",
        }
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native sphere-side result returned unknown fields")
        if value["classification"] not in (
            "strictly_inside",
            "boundary",
            "outside",
        ):
            raise ValueError("the native sphere-side classification is invalid")
        if not isinstance(value["sign"], str) or value["sign"] not in SIGNS:
            raise ValueError("the native sphere-side sign is invalid")
        stage = value["certification_stage"]
        if not isinstance(stage, str):
            raise ValueError("the native sphere-side stage is invalid")
        validate_counters(value["counters"], value["sign"], stage, predicate)
        canonical_rational(
            value["signed_offset_exact"], "sphere signed offset", nonnegative=False
        )
        canonical_level(value["squared_distance_exact"], "sphere squared distance")
        expected = sphere_side_oracle(replay_input)
        if value != expected:
            raise ValueError("the native sphere-side result differs from the exact oracle")
        return value

    if predicate == "circumcenter_support":
        support_kind, affine_dimension, expected_center, expected_level = (
            circumcenter_support_oracle(replay_input)
        )
        expected_fields = {
            "affine_dimension",
            "center_exact",
            "predicate",
            "squared_level_exact",
            "support_kind",
            "support_size",
        }
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native circumcenter construction returned unknown fields")
        if (
            value["predicate"] != predicate
            or value["support_kind"] != support_kind
            or type(value["support_size"]) is not int
            or value["support_size"] != len(replay_input["points"])
            or type(value["affine_dimension"]) is not int
            or value["affine_dimension"] != affine_dimension
        ):
            raise ValueError("the native circumcenter invariants differ from the replay input")
        if expected_center is None or expected_level is None:
            if value["center_exact"] is not None or value["squared_level_exact"] is not None:
                raise ValueError("an affinely dependent support exposed a circumcenter witness")
            return value
        native_center = validate_exact_rational3(
            value["center_exact"], "native circumcenter"
        )
        if native_center != exact_rational3_record(expected_center):
            raise ValueError("the native circumcenter differs from the exact Gram oracle")
        native_level = canonical_level(
            value["squared_level_exact"], "native circumcenter squared level"
        )
        if native_level != expected_level:
            raise ValueError("the native circumcenter level differs from the exact Gram oracle")
        return value

    if predicate == "plane_through_points":
        expected_fields = {"plane", "predicate"}
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native plane construction returned unknown fields")
        if value["predicate"] != predicate:
            raise ValueError("the native replay returned a different predicate")
        native_plane = validate_plane(value["plane"], "native constructed plane")
        points = [point_from_words(words) for words in replay_input["points"]]
        expected_plane = plane_through_points(points)
        if native_plane != expected_plane:
            raise ValueError("the native constructed plane differs from the replay input")
        return value

    if predicate == "power_bisector_affine_form":
        classification, expected_plane, expected_coefficients = power_bisector_classification(
            replay_input
        )
        expected_fields = {"affine_form", "classification", "predicate"}
        if expected_plane is not None:
            expected_fields.add("plane")
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native affine-form classification returned unknown fields")
        if value["predicate"] != predicate or value["classification"] != classification:
            raise ValueError("the native affine-form classification differs from the replay input")
        native_coefficients = exact_affine_form(
            value["affine_form"], "power-bisector affine form"
        )
        if native_coefficients != expected_coefficients:
            raise ValueError("the native affine-form coefficients differ from the replay input")
        if expected_plane is not None:
            native_plane = validate_plane(value["plane"], "native power-bisector plane")
            if native_plane != expected_plane:
                raise ValueError("the native power-bisector plane differs from the replay input")
        return value

    if predicate == "intersect_three_planes":
        (
            intersection_kind,
            expected_point,
            normal_rank,
            augmented_rank,
            affine_dimension,
        ) = solve_three_planes(replay_input["planes"])
        expected_fields = {
            "affine_dimension",
            "augmented_rank",
            "intersection_exact",
            "intersection_kind",
            "normal_rank",
            "predicate",
        }
        if not isinstance(value, dict) or set(value) != expected_fields:
            raise ValueError("the native plane intersection returned unknown fields")
        if (
            value["predicate"] != predicate
            or value["intersection_kind"] != intersection_kind
            or type(value["normal_rank"]) is not int
            or value["normal_rank"] != normal_rank
            or type(value["augmented_rank"]) is not int
            or value["augmented_rank"] != augmented_rank
            or (
                value["affine_dimension"] is not None
                and type(value["affine_dimension"]) is not int
            )
            or value["affine_dimension"] != affine_dimension
        ):
            raise ValueError("the native plane-intersection invariants differ from the replay input")
        if expected_point is not None:
            native_point = validate_exact_rational3(
                value["intersection_exact"], "native plane intersection"
            )
            if native_point != exact_rational3_record(expected_point):
                raise ValueError("the native plane intersection differs from Gaussian elimination")
        elif value["intersection_exact"] is not None:
            raise ValueError("a nonunique native plane intersection exposed a point")
        return value

    distance_fields = {
        "certification_stage",
        "counters",
        "left_squared_distance",
        "predicate",
        "right_squared_distance",
        "sign",
    }
    orientation_fields = {
        "certification_stage",
        "counters",
        "determinant_exact",
        "predicate",
        "sign",
    }
    power_fields = {
        "affine_value_exact",
        "certification_stage",
        "counters",
        "delta_coordinate_sum_exact",
        "delta_squared_norm_sum_exact",
        "predicate",
        "sign",
    }
    orientation_2d_fields = {
        "certification_stage",
        "counters",
        "orientation_value_exact",
        "predicate",
        "sign",
    }
    fourth_plane_fields = {
        "certification_stage",
        "counters",
        "intersection_exact",
        "predicate",
        "sign",
        "signed_value_exact",
    }
    expected_fields = {
        "compare_squared_distances": distance_fields,
        "orientation_3d": orientation_fields,
        "power_bisector_side": power_fields,
        "orientation_2d_in_plane": orientation_2d_fields,
        "fourth_plane_incidence": fourth_plane_fields,
    }[predicate]
    if not isinstance(value, dict) or set(value) != expected_fields:
        raise ValueError("the native replay returned missing or unknown predicate fields")
    if value["predicate"] != predicate:
        raise ValueError("the native replay returned a different predicate")
    stage = value["certification_stage"]
    if not isinstance(stage, str):
        raise ValueError("the native replay returned an invalid certification stage")
    sign = value["sign"]
    if not isinstance(sign, str) or sign not in SIGNS:
        raise ValueError("the native replay returned an invalid scientific sign")
    validate_counters(value["counters"], sign, stage, predicate)

    if predicate == "compare_squared_distances":
        left = canonical_level(value["left_squared_distance"], "left squared distance")
        right = canonical_level(value["right_squared_distance"], "right squared distance")
        points = [point_from_words(words) for words in replay_input["points"]]
        expected_left = squared_distance(points[0], points[1])
        expected_right = squared_distance(points[0], points[2])
        if left != expected_left or right != expected_right:
            raise ValueError("the native distance witness differs from the replay input")
        witness_sign = expected_sign(expected_left - expected_right)
    elif predicate == "orientation_3d":
        determinant = canonical_rational(
            value["determinant_exact"], "orientation determinant", nonnegative=False
        )
        points = [point_from_words(words) for words in replay_input["points"]]
        expected_determinant = orientation_determinant(points)
        if determinant != expected_determinant:
            raise ValueError("the native orientation witness differs from the replay input")
        witness_sign = expected_sign(expected_determinant)
    elif predicate == "power_bisector_side":
        point_table = [point_from_words(words) for words in replay_input["point_table"]]
        r_points = [point_table[identifier] for identifier in replay_input["r_ids"]]
        q_points = [point_table[identifier] for identifier in replay_input["q_ids"]]
        delta_coordinate_sum = tuple(
            sum((point[axis] for point in r_points), Fraction())
            - sum((point[axis] for point in q_points), Fraction())
            for axis in range(3)
        )
        delta_squared_norm_sum = sum(
            (sum((coordinate * coordinate for coordinate in point), Fraction())
             for point in r_points),
            Fraction(),
        ) - sum(
            (sum((coordinate * coordinate for coordinate in point), Fraction())
             for point in q_points),
            Fraction(),
        )
        witness = rational3(replay_input["witness"], "input witness")
        affine_value = -2 * sum(
            (coordinate * delta for coordinate, delta in zip(witness, delta_coordinate_sum)),
            Fraction(),
        ) + delta_squared_norm_sum
        native_delta = rational3(
            value["delta_coordinate_sum_exact"], "native delta coordinate sum"
        )
        native_norm = canonical_rational(
            value["delta_squared_norm_sum_exact"],
            "native delta squared norm sum",
            nonnegative=False,
        )
        native_affine = canonical_rational(
            value["affine_value_exact"], "native affine value", nonnegative=False
        )
        if (
            native_delta != delta_coordinate_sum
            or native_norm != delta_squared_norm_sum
            or native_affine != affine_value
        ):
            raise ValueError("the native power-bisector witness differs from the replay input")
        witness_sign = expected_sign(affine_value)
    elif predicate == "orientation_2d_in_plane":
        plane = replay_input["plane"]
        points = [point_from_words(words) for words in replay_input["points"]]
        if any(evaluate_plane(plane, point) != 0 for point in points):
            raise ValueError("the orientation replay points are outside the exact support")
        first = tuple(value - origin for origin, value in zip(points[0], points[1]))
        second = tuple(value - origin for origin, value in zip(points[0], points[2]))
        normal = plane_coefficients(plane)[:3]
        cross = cross_product(first, second)
        expected_value = sum(
            (coefficient * component for coefficient, component in zip(normal, cross)),
            Fraction(),
        )
        native_value = canonical_rational(
            value["orientation_value_exact"],
            "orientation 2D value",
            nonnegative=False,
        )
        if native_value != expected_value:
            raise ValueError("the native orientation 2D witness differs from the replay input")
        witness_sign = expected_sign(expected_value)
    else:
        intersection_kind, expected_point, _, _, _ = solve_three_planes(
            replay_input["planes"][:3]
        )
        if intersection_kind != "unique" or expected_point is None:
            raise ValueError("fourth-plane incidence requires a unique exact intersection")
        native_point = validate_exact_rational3(
            value["intersection_exact"], "native fourth-plane intersection"
        )
        if native_point != exact_rational3_record(expected_point):
            raise ValueError("the native fourth-plane intersection differs from Gaussian elimination")
        expected_value = evaluate_plane(replay_input["planes"][3], expected_point)
        native_value = canonical_rational(
            value["signed_value_exact"], "fourth-plane signed value", nonnegative=False
        )
        if native_value != expected_value:
            raise ValueError("the native fourth-plane value differs from the replay input")
        witness_sign = expected_sign(expected_value)
    if sign != witness_sign:
        raise ValueError("the native replay sign contradicts its exact witness")
    return value


def replay(value: dict[str, Any], executable: Path) -> dict[str, Any]:
    arguments = [str(executable), value["predicate"]]
    if value["schema_version"] == 1:
        arguments.extend(word for point in value["points"] for word in point)
    elif value["schema_version"] == 2:
        witness = value["witness"]
        arguments.extend(
            [
                witness["x_numerator"],
                witness["y_numerator"],
                witness["z_numerator"],
                witness["denominator"],
                str(len(value["point_table"])),
            ]
        )
        arguments.extend(word for point in value["point_table"] for word in point)
        arguments.append(str(len(value["r_ids"])))
        arguments.extend(str(identifier) for identifier in value["r_ids"])
        arguments.append(str(len(value["q_ids"])))
        arguments.extend(str(identifier) for identifier in value["q_ids"])
    elif value["schema_version"] == 4:
        arguments.append(str(len(value["points"])))
        arguments.extend(word for point in value["points"] for word in point)
    elif value["schema_version"] == 5:
        if value["predicate"] == "circumcenter_support_analysis":
            arguments.append(str(len(value["points"])))
            arguments.extend(word for point in value["points"] for word in point)
        else:
            center = value["center_exact"]
            level = value["squared_level_exact"]
            arguments.extend(
                center[f"{axis}_numerator"] for axis in "xyz"
            )
            arguments.append(center["denominator"])
            arguments.extend([level["numerator"], level["denominator"]])
            arguments.extend(value["point"])
    elif value["predicate"] == "plane_through_points":
        arguments.extend(word for point in value["points"] for word in point)
    elif value["predicate"] == "power_bisector_affine_form":
        arguments.append(str(len(value["point_table"])))
        arguments.extend(word for point in value["point_table"] for word in point)
        arguments.append(str(len(value["r_ids"])))
        arguments.extend(str(identifier) for identifier in value["r_ids"])
        arguments.append(str(len(value["q_ids"])))
        arguments.extend(str(identifier) for identifier in value["q_ids"])
    elif value["predicate"] == "orientation_2d_in_plane":
        arguments.extend(value["plane"][field] for field in ("a", "b", "c", "d"))
        arguments.extend(word for point in value["points"] for word in point)
    else:
        for plane in value["planes"]:
            arguments.extend(plane[field] for field in ("a", "b", "c", "d"))
    completed = subprocess.run(
        arguments,
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=NATIVE_TIMEOUT_SECONDS,
    )
    if completed.stderr:
        raise ValueError("the native replay wrote unexpected diagnostics to stderr")
    result = strict_json_loads(completed.stdout)
    if completed.stdout != canonical_json(result) + "\n":
        raise ValueError("the native replay stdout is not canonical JSON")
    result = validate_native_result(result, value)
    return {
        "input": value,
        "kind": RESULT_KIND,
        "replay_id": replay_id(value),
        "result": result,
        "schema_version": value["schema_version"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="versioned predicate replay JSON")
    parser.add_argument("--executable", type=Path, help="native replay executable")
    arguments = parser.parse_args()
    try:
        raw = strict_json_loads(arguments.input.read_text(encoding="utf-8"))
        value = validate_input(raw)
        result = replay(value, locate_executable(arguments.executable))
    except (OSError, ValueError, json.JSONDecodeError, subprocess.SubprocessError) as error:
        print(f"predicate replay failed closed: {error}", file=sys.stderr)
        return 2
    print(canonical_json(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
