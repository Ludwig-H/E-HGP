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
SCHEMA_VERSIONS = {1, 2}
DOMAINS = {
    1: b"MorseHGP3D/predicate-replay-v1/",
    2: b"MorseHGP3D/predicate-replay-v2/",
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


def squared_distance(
    left: tuple[Fraction, Fraction, Fraction],
    right: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(((a - b) ** 2 for a, b in zip(left, right)), Fraction())


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


def validate_counters(value: Any, sign: str) -> None:
    if not isinstance(value, dict) or set(value) != COUNTER_FIELDS:
        raise ValueError("the native predicate counters do not match PredicateCounts v2")
    if any(type(count) is not int or count < 0 for count in value.values()):
        raise ValueError("the native predicate counters must be nonnegative integers")
    expected = {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if sign == "zero" else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }
    if value != expected:
        raise ValueError("the native predicate counters are not closed for the exact CPU slice")


def validate_native_result(value: Any, replay_input: dict[str, Any]) -> dict[str, Any]:
    predicate = replay_input["predicate"]
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
    expected_fields = {
        "compare_squared_distances": distance_fields,
        "orientation_3d": orientation_fields,
        "power_bisector_side": power_fields,
    }[predicate]
    if not isinstance(value, dict) or set(value) != expected_fields:
        raise ValueError("the native replay returned missing or unknown predicate fields")
    if value["predicate"] != predicate:
        raise ValueError("the native replay returned a different predicate")
    if value["certification_stage"] != "cpu_multiprecision":
        raise ValueError("the native replay did not use the declared exact CPU stage")
    sign = value["sign"]
    if not isinstance(sign, str) or sign not in SIGNS:
        raise ValueError("the native replay returned an invalid scientific sign")
    validate_counters(value["counters"], sign)

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
    else:
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
    if sign != witness_sign:
        raise ValueError("the native replay sign contradicts its exact witness")
    return value


def replay(value: dict[str, Any], executable: Path) -> dict[str, Any]:
    arguments = [str(executable), value["predicate"]]
    if value["schema_version"] == 1:
        arguments.extend(word for point in value["points"] for word in point)
    else:
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
    completed = subprocess.run(
        arguments,
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=NATIVE_TIMEOUT_SECONDS,
    )
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
