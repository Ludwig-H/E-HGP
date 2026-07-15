#!/usr/bin/env python3
"""Replay one certified CPU predicate from exact binary64 input words."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import subprocess
import sys
from fractions import Fraction
from pathlib import Path
from typing import Any


INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
SCHEMA_VERSION = 1
DOMAIN = b"MorseHGP3D/predicate-replay-v1/"
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
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), sort_keys=True)


def strict_json_loads(text: str) -> Any:
    def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    return json.loads(text, object_pairs_hook=reject_duplicate_keys)


def validate_input(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError("the replay input must be a JSON object")
    required = {"kind", "schema_version", "predicate", "points"}
    if set(value) != required:
        raise ValueError("the replay input has missing or unknown fields")
    if (
        value["kind"] != INPUT_KIND
        or type(value["schema_version"]) is not int
        or value["schema_version"] != SCHEMA_VERSION
    ):
        raise ValueError("the replay input kind or schema_version is unsupported")
    predicate = value["predicate"]
    if not isinstance(predicate, str) or predicate not in POINT_COUNTS:
        raise ValueError("the replay predicate is unsupported")
    points = value["points"]
    if not isinstance(points, list) or len(points) != POINT_COUNTS[predicate]:
        raise ValueError("the replay input has the wrong point count")

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
    return {
        "kind": INPUT_KIND,
        "schema_version": SCHEMA_VERSION,
        "predicate": predicate,
        "points": normalized_points,
    }


def replay_id(value: dict[str, Any]) -> str:
    digest = hashlib.sha256()
    digest.update(DOMAIN)
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


def validate_native_result(value: Any, predicate: str) -> dict[str, Any]:
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
    expected_fields = (
        distance_fields if predicate == "compare_squared_distances" else orientation_fields
    )
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
        witness_sign = expected_sign(left - right)
    else:
        determinant = canonical_rational(
            value["determinant_exact"], "orientation determinant", nonnegative=False
        )
        witness_sign = expected_sign(determinant)
    if sign != witness_sign:
        raise ValueError("the native replay sign contradicts its exact witness")
    return value


def replay(value: dict[str, Any], executable: Path) -> dict[str, Any]:
    arguments = [str(executable), value["predicate"]]
    arguments.extend(word for point in value["points"] for word in point)
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
    result = validate_native_result(result, value["predicate"])
    return {
        "input": value,
        "kind": RESULT_KIND,
        "replay_id": replay_id(value),
        "result": result,
        "schema_version": SCHEMA_VERSION,
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
