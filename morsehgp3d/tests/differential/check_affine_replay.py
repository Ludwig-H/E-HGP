#!/usr/bin/env python3
"""Audit the v3 affine replay contract against an independent Fraction oracle."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path
from typing import Any


INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
V3_DOMAIN = b"MorseHGP3D/predicate-replay-v3/"
PREDICATE_FAMILIES = {
    "plane_through_points",
    "power_bisector_affine_form",
    "orientation_2d_in_plane",
    "intersect_three_planes",
    "fourth_plane_incidence",
}
SIGNS = {"negative", "zero", "positive"}
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}
RATIONAL3_FIELDS = {
    "denominator",
    "schema_version",
    "unit",
    "x_numerator",
    "y_numerator",
    "z_numerator",
}
PLANE_FIELDS = {"a", "b", "c", "d", "schema_version"}
AFFINE_FORM_FIELDS = {"a", "b", "c", "d", "schema_version"}
WRAPPER_TIMEOUT_SECONDS = 40

Point = tuple[Fraction, Fraction, Fraction]
Coefficients = tuple[Fraction, Fraction, Fraction, Fraction]


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def point_from_words(words: list[str]) -> Point:
    if len(words) != 3:
        raise AssertionError("an affine replay point does not have three coordinates")
    return tuple(
        Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])
        for word in words
    )  # type: ignore[return-value]


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def opposite_sign(value: str) -> str:
    return {"negative": "positive", "zero": "zero", "positive": "negative"}[value]


def primitive_coefficients(coefficients: Coefficients) -> tuple[int, int, int, int]:
    denominator = 1
    for coefficient in coefficients:
        denominator = math.lcm(denominator, coefficient.denominator)
    integers = [
        coefficient.numerator * (denominator // coefficient.denominator)
        for coefficient in coefficients
    ]
    divisor = 0
    for coefficient in integers:
        divisor = math.gcd(divisor, abs(coefficient))
    if divisor:
        integers = [coefficient // divisor for coefficient in integers]
    return tuple(integers)  # type: ignore[return-value]


def plane_record(coefficients: Coefficients) -> dict[str, str]:
    primitive = primitive_coefficients(coefficients)
    if primitive[:3] == (0, 0, 0):
        raise ValueError("zero normal")
    return {
        "a": str(primitive[0]),
        "b": str(primitive[1]),
        "c": str(primitive[2]),
        "d": str(primitive[3]),
        "schema_version": "2.0.0",
    }


def plane_coefficients(plane: dict[str, str]) -> Coefficients:
    if not isinstance(plane, dict) or set(plane) != PLANE_FIELDS:
        raise AssertionError("an observed plane has an open schema")
    if plane["schema_version"] != "2.0.0":
        raise AssertionError("an observed plane has the wrong contract version")
    coefficients: list[int] = []
    for field in ("a", "b", "c", "d"):
        serialized = plane[field]
        parsed = int(serialized)
        if serialized != str(parsed):
            raise AssertionError("an observed plane coefficient is not canonical")
        coefficients.append(parsed)
    divisor = 0
    for coefficient in coefficients:
        divisor = math.gcd(divisor, abs(coefficient))
    if coefficients[:3] == [0, 0, 0] or divisor != 1:
        raise AssertionError("an observed plane is not primitive with a nonzero normal")
    return tuple(Fraction(value) for value in coefficients)  # type: ignore[return-value]


def opposite_plane(plane: dict[str, str]) -> dict[str, str]:
    return {
        field: str(-int(plane[field])) if field != "schema_version" else "2.0.0"
        for field in ("a", "b", "c", "d", "schema_version")
    }


def subtract(left: Point, right: Point) -> Point:
    return tuple(a - b for a, b in zip(left, right))  # type: ignore[return-value]


def cross(left: Point, right: Point) -> Point:
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def dot(left: tuple[Fraction, ...], right: tuple[Fraction, ...]) -> Fraction:
    return sum((a * b for a, b in zip(left, right)), Fraction())


def through_points(points: list[Point]) -> dict[str, str]:
    normal = cross(subtract(points[1], points[0]), subtract(points[2], points[0]))
    offset = -dot(normal, points[0])
    return plane_record((normal[0], normal[1], normal[2], offset))


def evaluate(plane: dict[str, str], point: Point) -> Fraction:
    coefficients = plane_coefficients(plane)
    return dot(coefficients[:3], point) + coefficients[3]


def rref(matrix: list[list[Fraction]]) -> tuple[list[list[Fraction]], list[int]]:
    reduced = [list(row) for row in matrix]
    pivots: list[int] = []
    pivot_row = 0
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


def solve_three_planes(
    planes: list[dict[str, str]],
) -> tuple[str, Point | None, int, int, int | None]:
    normal_matrix: list[list[Fraction]] = []
    augmented_matrix: list[list[Fraction]] = []
    for plane in planes:
        a, b, c, d = plane_coefficients(plane)
        normal_matrix.append([a, b, c])
        augmented_matrix.append([a, b, c, -d])
    _, normal_pivots = rref(normal_matrix)
    reduced, augmented_pivots = rref(augmented_matrix)
    normal_rank = len(normal_pivots)
    augmented_rank = len(augmented_pivots)
    if normal_rank < augmented_rank:
        return "empty", None, normal_rank, augmented_rank, None
    if normal_rank < 3:
        return "affine_family", None, normal_rank, augmented_rank, 3 - normal_rank
    point = [Fraction(), Fraction(), Fraction()]
    for row, column in enumerate(augmented_pivots):
        if column < 3:
            point[column] = reduced[row][3]
    return "unique", tuple(point), normal_rank, augmented_rank, 0  # type: ignore[return-value]


def rational_record(value: Fraction) -> dict[str, str]:
    return {"denominator": str(value.denominator), "numerator": str(value.numerator)}


def rational_from_record(value: Any, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != {"denominator", "numerator"}:
        raise AssertionError(f"{label} has an open rational schema")
    numerator = int(value["numerator"])
    denominator = int(value["denominator"])
    if (
        value["numerator"] != str(numerator)
        or value["denominator"] != str(denominator)
        or denominator <= 0
        or math.gcd(abs(numerator), denominator) != 1
    ):
        raise AssertionError(f"{label} is not a canonical reduced rational")
    return Fraction(numerator, denominator)


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


def rational3_from_record(value: Any, label: str) -> Point:
    if not isinstance(value, dict) or set(value) != RATIONAL3_FIELDS:
        raise AssertionError(f"{label} has an open ExactRational3 schema")
    if value["schema_version"] != "2.0.0" or value["unit"] != "input_coordinate_unit":
        raise AssertionError(f"{label} has the wrong ExactRational3 contract")
    denominator = int(value["denominator"])
    numerators = [int(value[f"{axis}_numerator"]) for axis in "xyz"]
    if value != rational3_record(
        tuple(Fraction(numerator, denominator) for numerator in numerators)  # type: ignore[arg-type]
    ):
        raise AssertionError(f"{label} is not canonical and jointly reduced")
    return tuple(Fraction(numerator, denominator) for numerator in numerators)  # type: ignore[return-value]


def affine_form_record(coefficients: Coefficients) -> dict[str, Any]:
    return {
        "a": rational_record(coefficients[0]),
        "b": rational_record(coefficients[1]),
        "c": rational_record(coefficients[2]),
        "d": rational_record(coefficients[3]),
        "schema_version": "2.0.0",
    }


def affine_form_from_record(value: Any, label: str) -> Coefficients:
    if not isinstance(value, dict) or set(value) != AFFINE_FORM_FIELDS:
        raise AssertionError(f"{label} has an open ExactAffineForm3 schema")
    if value["schema_version"] != "2.0.0":
        raise AssertionError(f"{label} has the wrong ExactAffineForm3 contract")
    return tuple(
        rational_from_record(value[field], f"{label} coefficient {field}")
        for field in ("a", "b", "c", "d")
    )  # type: ignore[return-value]


def expected_counters(value: Fraction) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if value == 0 else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def check_exact_decision(result: dict[str, Any], value: Fraction, value_field: str) -> None:
    expected_fields = {
        "certification_stage",
        "counters",
        "predicate",
        "sign",
        value_field,
    }
    if set(result) != expected_fields:
        raise AssertionError("an exact affine decision has an open result schema")
    if result["certification_stage"] != "cpu_multiprecision":
        raise AssertionError("an affine predicate used an unimplemented filter stage")
    if not isinstance(result["counters"], dict) or set(result["counters"]) != COUNTER_FIELDS:
        raise AssertionError("an affine predicate has an open counter schema")
    if result["counters"] != expected_counters(value):
        raise AssertionError("an affine predicate counter disagrees with its exact decision")
    if result["sign"] not in SIGNS or result["sign"] != sign(value):
        raise AssertionError("an affine predicate sign disagrees with the Fraction oracle")
    if rational_from_record(result[value_field], value_field) != value:
        raise AssertionError("an affine predicate witness disagrees with the Fraction oracle")


def expected_power_form(
    replay_input: dict[str, Any],
) -> tuple[str, dict[str, str] | None, Coefficients]:
    table = [point_from_words(words) for words in replay_input["point_table"]]
    r_points = [table[index] for index in replay_input["r_ids"]]
    q_points = [table[index] for index in replay_input["q_ids"]]
    delta = tuple(
        sum((point[axis] for point in r_points), Fraction())
        - sum((point[axis] for point in q_points), Fraction())
        for axis in range(3)
    )
    norm_delta = sum((dot(point, point) for point in r_points), Fraction()) - sum(
        (dot(point, point) for point in q_points), Fraction()
    )
    coefficients = (-2 * delta[0], -2 * delta[1], -2 * delta[2], norm_delta)
    if coefficients[:3] != (Fraction(), Fraction(), Fraction()):
        return "proper_plane", plane_record(coefficients), coefficients
    if coefficients[3] < 0:
        return "constant_negative", None, coefficients
    if coefficients[3] > 0:
        return "constant_positive", None, coefficients
    return "identically_zero", None, coefficients


def audit_scientific_result(replay: dict[str, Any]) -> tuple[str, str | None]:
    replay_input = replay["input"]
    result = replay["result"]
    predicate = replay_input["predicate"]
    if not isinstance(result, dict) or result.get("predicate") != predicate:
        raise AssertionError("the native result does not identify its exact predicate")

    if predicate == "plane_through_points":
        if set(result) != {"plane", "predicate"}:
            raise AssertionError("the plane-construction result schema is open")
        expected = through_points(
            [point_from_words(words) for words in replay_input["points"]]
        )
        plane_coefficients(result["plane"])
        if result["plane"] != expected:
            raise AssertionError("plane construction differs from the Fraction cross product")
        return predicate, None

    if predicate == "power_bisector_affine_form":
        classification, expected_plane, expected_coefficients = expected_power_form(replay_input)
        expected_fields = {"affine_form", "classification", "predicate"}
        if expected_plane is not None:
            expected_fields.add("plane")
        if set(result) != expected_fields or result["classification"] != classification:
            raise AssertionError("the affine-form classification differs from the Fraction oracle")
        observed_coefficients = affine_form_from_record(
            result["affine_form"], "power-bisector affine form"
        )
        if observed_coefficients != expected_coefficients:
            raise AssertionError("the exact H_RQ scale differs from the Fraction oracle")
        if expected_plane is not None:
            plane_coefficients(result["plane"])
            if result["plane"] != expected_plane:
                raise AssertionError("the power-bisector plane differs from the Fraction oracle")
        return predicate, classification

    if predicate == "orientation_2d_in_plane":
        plane = replay_input["plane"]
        points = [point_from_words(words) for words in replay_input["points"]]
        if any(evaluate(plane, point) for point in points):
            raise AssertionError("an accepted orientation fixture is outside its support plane")
        normal = plane_coefficients(plane)[:3]
        value = dot(normal, cross(subtract(points[1], points[0]), subtract(points[2], points[0])))
        check_exact_decision(result, value, "orientation_value_exact")
        return predicate, sign(value)

    if predicate == "intersect_three_planes":
        intersection_kind, point, normal_rank, augmented_rank, dimension = (
            solve_three_planes(replay_input["planes"])
        )
        expected_fields = {
            "affine_dimension",
            "augmented_rank",
            "intersection_exact",
            "intersection_kind",
            "normal_rank",
            "predicate",
        }
        if set(result) != expected_fields or result["intersection_kind"] != intersection_kind:
            raise AssertionError("the plane-system class differs from independent elimination")
        if (
            type(result["normal_rank"]) is not int
            or type(result["augmented_rank"]) is not int
            or result["normal_rank"] != normal_rank
            or result["augmented_rank"] != augmented_rank
            or (dimension is not None and type(result["affine_dimension"]) is not int)
            or result["affine_dimension"] != dimension
        ):
            raise AssertionError("the plane-system ranks or dimension differ from RREF")
        if point is not None:
            observed = rational3_from_record(result["intersection_exact"], "intersection")
            if observed != point:
                raise AssertionError("the unique intersection differs from independent elimination")
            if any(evaluate(plane, observed) for plane in replay_input["planes"]):
                raise AssertionError("the returned intersection is not incident to every plane")
        elif result["intersection_exact"] is not None:
            raise AssertionError("a nonunique plane system returned an exact point")
        detail = intersection_kind if dimension is None else f"{intersection_kind}:{dimension}"
        return predicate, detail

    if predicate == "fourth_plane_incidence":
        intersection_kind, point, _, _, _ = solve_three_planes(replay_input["planes"][:3])
        if intersection_kind != "unique" or point is None:
            raise AssertionError("an accepted fourth-plane fixture has no unique intersection")
        if set(result) != {
            "certification_stage",
            "counters",
            "intersection_exact",
            "predicate",
            "sign",
            "signed_value_exact",
        }:
            raise AssertionError("the fourth-plane result schema is open")
        observed_point = rational3_from_record(result["intersection_exact"], "fourth intersection")
        if observed_point != point:
            raise AssertionError("the fourth-plane intersection differs from elimination")
        value = evaluate(replay_input["planes"][3], point)
        decision = dict(result)
        del decision["intersection_exact"]
        check_exact_decision(decision, value, "signed_value_exact")
        return predicate, sign(value)

    raise AssertionError(f"unexpected affine predicate: {predicate}")


def run_wrapper(wrapper: Path, executable: Path, fixture: Path) -> tuple[str, dict[str, Any]]:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    replay = json.loads(completed.stdout)
    if completed.stdout != canonical_json(replay) + "\n":
        raise AssertionError(f"{fixture.name} does not produce canonical replay JSON")
    return completed.stdout, replay


def audit_envelope(replay: dict[str, Any], fixture_value: dict[str, Any]) -> None:
    if set(replay) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v3 replay envelope schema is open")
    if (
        replay["kind"] != RESULT_KIND
        or replay["schema_version"] != 3
        or replay["input"] != fixture_value
        or replay["input"].get("kind") != INPUT_KIND
        or replay["input"].get("schema_version") != 3
    ):
        raise AssertionError("the wrapper changed or mis-versioned its v3 replay input")
    serialized = canonical_json(replay["input"]).encode("utf-8")
    expected = hashlib.sha256(V3_DOMAIN + serialized).hexdigest()
    if replay["replay_id"] != expected:
        raise AssertionError("the v3 replay identifier uses the wrong bytes or domain")
    for old_domain in (
        b"MorseHGP3D/predicate-replay-v1/",
        b"MorseHGP3D/predicate-replay-v2/",
    ):
        if replay["replay_id"] == hashlib.sha256(old_domain + serialized).hexdigest():
            raise AssertionError("the v3 replay identifier aliases an older domain")


def write_case(directory: Path, name: str, value: dict[str, Any], *, pretty: bool = False) -> Path:
    path = directory / name
    serialized = json.dumps(value, ensure_ascii=False, indent=2) if pretty else canonical_json(value)
    path.write_text(serialized + "\n", encoding="utf-8")
    return path


def run_generated(
    wrapper: Path, executable: Path, directory: Path, name: str, value: dict[str, Any]
) -> dict[str, Any]:
    fixture = write_case(directory, name, value)
    _, replay = run_wrapper(wrapper, executable, fixture)
    audit_envelope(replay, value)
    audit_scientific_result(replay)
    return replay


def check_metamorphisms(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> None:
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-affine-meta-") as temporary:
        directory = Path(temporary)

        plane_base = fixtures["affine_plane_translated.json"]
        base_replay = run_generated(wrapper, executable, directory, "plane-base.json", plane_base)
        cyclic = json.loads(json.dumps(plane_base))
        cyclic["points"] = [plane_base["points"][index] for index in (1, 2, 0)]
        cyclic_replay = run_generated(wrapper, executable, directory, "plane-cyclic.json", cyclic)
        if cyclic_replay["result"] != base_replay["result"]:
            raise AssertionError("cyclic point permutation changed the oriented plane")
        swapped = json.loads(json.dumps(plane_base))
        swapped["points"] = [plane_base["points"][index] for index in (0, 2, 1)]
        swapped_replay = run_generated(wrapper, executable, directory, "plane-swapped.json", swapped)
        if swapped_replay["result"]["plane"] != opposite_plane(base_replay["result"]["plane"]):
            raise AssertionError("odd point permutation did not reverse the oriented plane")

        power_base = fixtures["affine_power_proper.json"]
        power_replay = run_generated(wrapper, executable, directory, "power-base.json", power_base)
        reversed_power = json.loads(json.dumps(power_base))
        reversed_power["r_ids"], reversed_power["q_ids"] = power_base["q_ids"], power_base["r_ids"]
        reversed_replay = run_generated(
            wrapper, executable, directory, "power-reversed.json", reversed_power
        )
        if (
            reversed_replay["result"].get("classification") != "proper_plane"
            or reversed_replay["result"].get("plane")
            != opposite_plane(power_replay["result"]["plane"])
            or affine_form_from_record(
                reversed_replay["result"]["affine_form"], "reversed power form"
            )
            != tuple(
                -coefficient
                for coefficient in affine_form_from_record(
                    power_replay["result"]["affine_form"], "base power form"
                )
            )
        ):
            raise AssertionError("exchanging R and Q did not negate the power-bisector form")
        positive_power = fixtures["affine_power_constant_positive.json"]
        reversed_constant = json.loads(json.dumps(positive_power))
        reversed_constant["r_ids"], reversed_constant["q_ids"] = (
            positive_power["q_ids"],
            positive_power["r_ids"],
        )
        if run_generated(
            wrapper, executable, directory, "power-constant-reversed.json", reversed_constant
        )["result"]["classification"] != "constant_negative":
            raise AssertionError("exchanging R and Q did not reverse the constant form sign")

        orientation_base = fixtures["affine_orientation_positive.json"]
        orientation_replay = run_generated(
            wrapper, executable, directory, "orientation-base.json", orientation_base
        )
        orientation_cyclic = json.loads(json.dumps(orientation_base))
        orientation_cyclic["points"] = [orientation_base["points"][index] for index in (1, 2, 0)]
        if run_generated(
            wrapper, executable, directory, "orientation-cyclic.json", orientation_cyclic
        )["result"] != orientation_replay["result"]:
            raise AssertionError("cyclic point permutation changed orientation 2D")
        for name, transformed in (
            ("orientation-swapped.json", json.loads(json.dumps(orientation_base))),
            ("orientation-opposite-plane.json", json.loads(json.dumps(orientation_base))),
        ):
            if "swapped" in name:
                transformed["points"] = [orientation_base["points"][index] for index in (0, 2, 1)]
            else:
                transformed["plane"] = opposite_plane(orientation_base["plane"])
            transformed_result = run_generated(
                wrapper, executable, directory, name, transformed
            )["result"]
            if (
                transformed_result["sign"] != opposite_sign(orientation_replay["result"]["sign"])
                or rational_from_record(
                    transformed_result["orientation_value_exact"], "transformed orientation"
                )
                != -rational_from_record(
                    orientation_replay["result"]["orientation_value_exact"], "base orientation"
                )
            ):
                raise AssertionError("an orientation-reversing transform did not negate orientation 2D")

        intersection_base = fixtures["affine_intersection_unique.json"]
        intersection_replay = run_generated(
            wrapper, executable, directory, "intersection-base.json", intersection_base
        )
        intersection_permuted = json.loads(json.dumps(intersection_base))
        intersection_permuted["planes"] = [intersection_base["planes"][index] for index in (2, 0, 1)]
        intersection_flipped = json.loads(json.dumps(intersection_base))
        intersection_flipped["planes"][1] = opposite_plane(intersection_flipped["planes"][1])
        for name, transformed in (
            ("intersection-permuted.json", intersection_permuted),
            ("intersection-flipped.json", intersection_flipped),
        ):
            if run_generated(wrapper, executable, directory, name, transformed)["result"] != intersection_replay["result"]:
                raise AssertionError("plane reordering/orientation changed a unique intersection")

        fourth_base = fixtures["affine_fourth_positive.json"]
        fourth_replay = run_generated(wrapper, executable, directory, "fourth-base.json", fourth_base)
        fourth_permuted = json.loads(json.dumps(fourth_base))
        fourth_permuted["planes"][:3] = [fourth_base["planes"][index] for index in (2, 0, 1)]
        fourth_flipped_binding = json.loads(json.dumps(fourth_base))
        fourth_flipped_binding["planes"][0] = opposite_plane(fourth_flipped_binding["planes"][0])
        for name, transformed in (
            ("fourth-permuted.json", fourth_permuted),
            ("fourth-binding-flipped.json", fourth_flipped_binding),
        ):
            if run_generated(wrapper, executable, directory, name, transformed)["result"] != fourth_replay["result"]:
                raise AssertionError("binding-plane reordering/orientation changed fourth incidence")
        fourth_flipped_test = json.loads(json.dumps(fourth_base))
        fourth_flipped_test["planes"][3] = opposite_plane(fourth_flipped_test["planes"][3])
        flipped_result = run_generated(
            wrapper, executable, directory, "fourth-test-flipped.json", fourth_flipped_test
        )["result"]
        if (
            flipped_result["intersection_exact"] != fourth_replay["result"]["intersection_exact"]
            or flipped_result["sign"] != opposite_sign(fourth_replay["result"]["sign"])
            or rational_from_record(flipped_result["signed_value_exact"], "flipped fourth value")
            != -rational_from_record(fourth_replay["result"]["signed_value_exact"], "base fourth value")
        ):
            raise AssertionError("reversing the test plane did not negate fourth incidence")


def expect_closed_failure(wrapper: Path, executable: Path, fixture: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or "failed closed" not in completed.stderr:
        raise AssertionError(
            f"{fixture.name} did not fail closed: rc={completed.returncode}, "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )


def check_invalid_inputs(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> None:
    orientation = fixtures["affine_orientation_positive.json"]
    plane = fixtures["affine_plane_translated.json"]
    fourth = fixtures["affine_fourth_positive.json"]
    line = fixtures["affine_intersection_line.json"]
    variants: list[tuple[str, dict[str, Any]]] = []

    nonprimitive = json.loads(json.dumps(orientation))
    nonprimitive["plane"] = {
        "a": "0", "b": "0", "c": "2", "d": "0", "schema_version": "2.0.0"
    }
    variants.append(("nonprimitive-plane.json", nonprimitive))
    zero_normal = json.loads(json.dumps(orientation))
    zero_normal["plane"] = {
        "a": "0", "b": "0", "c": "0", "d": "1", "schema_version": "2.0.0"
    }
    variants.append(("zero-normal.json", zero_normal))
    negative_zero = json.loads(json.dumps(orientation))
    negative_zero["plane"]["a"] = "-0"
    variants.append(("negative-zero.json", negative_zero))
    unknown_field = json.loads(json.dumps(plane))
    unknown_field["unexpected"] = "closed schemas only"
    variants.append(("unknown-field.json", unknown_field))
    outside_support = json.loads(json.dumps(orientation))
    outside_support["points"][0][2] = "3ff0000000000000"
    variants.append(("outside-support.json", outside_support))
    nonunique_fourth = json.loads(json.dumps(fourth))
    nonunique_fourth["planes"][:3] = line["planes"]
    variants.append(("nonunique-fourth.json", nonunique_fourth))
    collinear = json.loads(json.dumps(plane))
    collinear["points"] = [
        ["0000000000000000", "0000000000000000", "0000000000000000"],
        ["3ff0000000000000", "0000000000000000", "0000000000000000"],
        ["4000000000000000", "0000000000000000", "0000000000000000"],
    ]
    variants.append(("collinear-plane.json", collinear))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-affine-invalid-") as temporary:
        directory = Path(temporary)
        for name, value in variants:
            expect_closed_failure(wrapper, executable, write_case(directory, name, value))


def make_fake_native(
    path: Path, output: dict[str, Any], *, diagnostic: str | None = None
) -> None:
    payload = canonical_json(output)
    diagnostic_statement = (
        f"sys.stderr.write({diagnostic!r} + '\\n')\n" if diagnostic is not None else ""
    )
    path.write_text(
        "#!/usr/bin/env python3\n"
        "import sys\n"
        + diagnostic_statement
        + f"sys.stdout.write({payload!r} + '\\n')\n",
        encoding="utf-8",
    )
    path.chmod(0o755)


def check_coherent_false_native_outputs(
    wrapper: Path,
    executable: Path,
    fixture_paths: dict[str, Path],
) -> None:
    selected = {
        "plane": "affine_plane_translated.json",
        "power": "affine_power_proper.json",
        "orientation": "affine_orientation_positive.json",
        "intersection": "affine_intersection_unique.json",
        "fourth": "affine_fourth_positive.json",
    }
    valid_results = {
        family: run_wrapper(wrapper, executable, fixture_paths[name])[1]["result"]
        for family, name in selected.items()
    }
    false_results = json.loads(json.dumps(valid_results))
    false_results["plane"]["plane"]["d"] = "-4"
    for field in ("a", "b", "c", "d"):
        coefficient = rational_from_record(
            false_results["power"]["affine_form"][field], f"false power {field}"
        )
        false_results["power"]["affine_form"][field] = rational_record(2 * coefficient)
    false_results["orientation"]["orientation_value_exact"] = {
        "denominator": "1", "numerator": "2"
    }
    false_results["intersection"]["intersection_exact"] = {
        "denominator": "3",
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit",
        "x_numerator": "2",
        "y_numerator": "1",
        "z_numerator": "1",
    }
    false_results["fourth"]["signed_value_exact"] = {
        "denominator": "1", "numerator": "2"
    }

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-affine-fake-") as temporary:
        directory = Path(temporary)
        for family, fixture_name in selected.items():
            fake = directory / f"fake-{family}"
            make_fake_native(fake, false_results[family])
            expect_closed_failure(wrapper, fake, fixture_paths[fixture_name])
        warning = directory / "fake-warning"
        make_fake_native(
            warning,
            valid_results["plane"],
            diagnostic="unexpected native warning",
        )
        expect_closed_failure(
            wrapper, warning, fixture_paths[selected["plane"]]
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wrapper", type=Path)
    parser.add_argument("executable", type=Path)
    parser.add_argument("fixture_directory", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    wrapper = arguments.wrapper.resolve(strict=True)
    executable = arguments.executable.resolve(strict=True)
    fixture_paths = {
        path.name: path for path in sorted(arguments.fixture_directory.glob("affine_*.json"))
    }
    if not fixture_paths:
        raise AssertionError("no affine replay fixtures were found")

    fixtures: dict[str, dict[str, Any]] = {}
    families: set[str] = set()
    power_classes: set[str] = set()
    intersection_cases: set[str] = set()
    fourth_signs: set[str] = set()
    for name, path in fixture_paths.items():
        fixture = load_json(path)
        fixtures[name] = fixture
        first_stdout, first_replay = run_wrapper(wrapper, executable, path)
        second_stdout, second_replay = run_wrapper(wrapper, executable, path)
        if first_stdout != second_stdout or first_replay != second_replay:
            raise AssertionError(f"{name} is not byte-for-byte replay-stable")
        audit_envelope(first_replay, fixture)
        family, detail = audit_scientific_result(first_replay)
        families.add(family)
        if family == "power_bisector_affine_form" and detail is not None:
            power_classes.add(detail)
        elif family == "intersect_three_planes" and detail is not None:
            intersection_cases.add(detail)
        elif family == "fourth_plane_incidence" and detail is not None:
            fourth_signs.add(detail)

    if families != PREDICATE_FAMILIES:
        raise AssertionError(f"the affine fixture corpus misses families: {PREDICATE_FAMILIES - families}")
    if power_classes != {
        "proper_plane", "constant_negative", "constant_positive", "identically_zero"
    }:
        raise AssertionError("the affine fixture corpus misses an affine-form classification")
    if intersection_cases != {"unique:0", "empty", "affine_family:1", "affine_family:2"}:
        raise AssertionError(f"the affine fixture corpus misses an intersection class: {intersection_cases}")
    if fourth_signs != SIGNS:
        raise AssertionError("the affine fixture corpus misses a fourth-plane sign")

    first_fixture = next(iter(fixtures.values()))
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-affine-order-") as temporary:
        reordered = dict(reversed(list(first_fixture.items())))
        reordered_path = write_case(Path(temporary), "reordered.json", reordered, pretty=True)
        _, reordered_replay = run_wrapper(wrapper, executable, reordered_path)
        original_path = fixture_paths[next(iter(fixtures))]
        _, original_replay = run_wrapper(wrapper, executable, original_path)
        if (
            reordered_replay["replay_id"] != original_replay["replay_id"]
            or reordered_replay["result"] != original_replay["result"]
        ):
            raise AssertionError("JSON member order or whitespace changed replay identity")

    check_metamorphisms(wrapper, executable, fixtures)
    check_invalid_inputs(wrapper, executable, fixtures)
    check_coherent_false_native_outputs(wrapper, executable, fixture_paths)
    print(
        "affine replay differential checks passed: "
        f"fixtures={len(fixtures)}, families={len(families)}, "
        f"power_classes={len(power_classes)}, intersection_cases={len(intersection_cases)}, "
        f"fourth_signs={len(fourth_signs)}, metamorphisms=12, invalid_inputs=7, "
        "coherent_false_outputs=5, unexpected_stderr=1"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
