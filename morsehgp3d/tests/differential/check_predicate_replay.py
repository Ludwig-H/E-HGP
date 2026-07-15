#!/usr/bin/env python3
"""Differentially replay native predicates against Python Fraction arithmetic."""

from __future__ import annotations

import json
import hashlib
import os
import re
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path


def canonical_json(value: object) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), sort_keys=True)


def rational_from_bits(word: str) -> Fraction:
    value = struct.unpack(">d", bytes.fromhex(word))[0]
    return Fraction.from_float(value)


def point(words: list[str]) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(rational_from_bits(word) for word in words)  # type: ignore[return-value]


def squared_distance(
    left: tuple[Fraction, Fraction, Fraction],
    right: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(((a - b) ** 2 for a, b in zip(left, right)), Fraction())


def determinant(
    a: tuple[Fraction, Fraction, Fraction],
    b: tuple[Fraction, Fraction, Fraction],
    c: tuple[Fraction, Fraction, Fraction],
    d: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    u = tuple(bi - ai for ai, bi in zip(a, b))
    v = tuple(ci - ai for ai, ci in zip(a, c))
    w = tuple(di - ai for ai, di in zip(a, d))
    return (
        u[0] * (v[1] * w[2] - v[2] * w[1])
        - u[1] * (v[0] * w[2] - v[2] * w[0])
        + u[2] * (v[0] * w[1] - v[1] * w[0])
    )


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def run_replay(wrapper: Path, executable: Path, fixture: Path) -> tuple[str, dict]:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=40,
    )
    parsed = json.loads(completed.stdout)
    if completed.stdout != canonical_json(parsed) + "\n":
        raise AssertionError("the replay wrapper output is not canonical JSON")
    return completed.stdout, parsed


def exact_level(value: Fraction) -> dict[str, str]:
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def check_distance(wrapper: Path, executable: Path, fixture: Path) -> None:
    first_output, replay = run_replay(wrapper, executable, fixture)
    second_output, repeated = run_replay(wrapper, executable, fixture)
    if first_output != second_output or replay["replay_id"] != repeated["replay_id"]:
        raise AssertionError("predicate replay is not byte-stable")
    if re.fullmatch(r"[0-9a-f]{64}", replay["replay_id"]) is None:
        raise AssertionError("predicate replay_id is not a SHA-256 hex digest")
    expected_id = hashlib.sha256(
        b"MorseHGP3D/predicate-replay-v1/"
        + canonical_json(replay["input"]).encode("utf-8")
    ).hexdigest()
    if replay["replay_id"] != expected_id:
        raise AssertionError("predicate replay_id does not cover its normalized input")

    points = [point(words) for words in replay["input"]["points"]]
    left = squared_distance(points[0], points[1])
    right = squared_distance(points[0], points[2])
    result = replay["result"]
    if result["sign"] != sign(left - right):
        raise AssertionError("native distance sign differs from Fraction")
    if result["left_squared_distance"] != exact_level(left):
        raise AssertionError("native left ExactLevel differs from Fraction")
    if result["right_squared_distance"] != exact_level(right):
        raise AssertionError("native right ExactLevel differs from Fraction")
    if result["certification_stage"] != "cpu_multiprecision" or result["counters"] != {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if left == right else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }:
        raise AssertionError("distance stage accounting is not exact-only and closed")


def check_orientation(wrapper: Path, executable: Path, fixture: Path) -> None:
    _, replay = run_replay(wrapper, executable, fixture)
    points = [point(words) for words in replay["input"]["points"]]
    expected = determinant(*points)
    result = replay["result"]
    if result["sign"] != sign(expected):
        raise AssertionError("native orientation sign differs from Fraction")
    if result["determinant_exact"] != {
        "denominator": str(expected.denominator),
        "numerator": str(expected.numerator),
    }:
        raise AssertionError("native orientation determinant differs from Fraction")
    if result["certification_stage"] != "cpu_multiprecision" or result["counters"] != {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if expected == 0 else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }:
        raise AssertionError("orientation stage accounting is not exact-only and closed")


def check_invalid_input(wrapper: Path, executable: Path, fixture: Path) -> None:
    base = json.loads(fixture.read_text(encoding="utf-8"))
    cases: list[str] = []
    nonfinite = json.loads(json.dumps(base))
    nonfinite["points"][0][0] = "7ff0000000000000"
    cases.append(canonical_json(nonfinite))
    boolean_version = json.loads(json.dumps(base))
    boolean_version["schema_version"] = True
    cases.append(canonical_json(boolean_version))
    floating_version = json.loads(json.dumps(base))
    floating_version["schema_version"] = 1.0
    cases.append(canonical_json(floating_version))
    non_string_predicate = json.loads(json.dumps(base))
    non_string_predicate["predicate"] = []
    cases.append(canonical_json(non_string_predicate))
    uppercase_bits = json.loads(json.dumps(base))
    uppercase_bits["points"][0][0] = "3FF0000000000000"
    cases.append(canonical_json(uppercase_bits))
    unknown_field = json.loads(json.dumps(base))
    unknown_field["unexpected"] = 1
    cases.append(canonical_json(unknown_field))
    cases.append(canonical_json(base)[:-1] + ',"schema_version":1}')

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-replay-") as directory:
        for index, text in enumerate(cases):
            invalid = Path(directory) / f"invalid-{index}.json"
            invalid.write_text(text + "\n", encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, str(wrapper), str(invalid), "--executable", str(executable)],
                capture_output=True,
                encoding="utf-8",
                timeout=40,
            )
            if completed.returncode != 2 or "failed closed" not in completed.stderr:
                raise AssertionError(f"invalid replay input {index} did not fail closed")


def make_output_script(path: Path, output: str) -> None:
    path.write_text("#!/bin/sh\nprintf '%s\\n' '" + output + "'\n", encoding="utf-8")
    path.chmod(0o755)


def expect_closed_failure(
    wrapper: Path,
    executable: Path,
    fixture: Path,
    *,
    cwd: Path | None = None,
    environment: dict[str, str] | None = None,
) -> None:
    completed = subprocess.run(
        [sys.executable, str(wrapper), str(fixture), "--executable", str(executable)],
        capture_output=True,
        encoding="utf-8",
        timeout=40,
        cwd=cwd,
        env=environment,
    )
    if completed.returncode != 2 or "failed closed" not in completed.stderr:
        raise AssertionError(f"invalid native replay did not fail closed: {completed.stdout}")


def check_invalid_native_output(wrapper: Path, executable: Path, fixture: Path) -> None:
    native = subprocess.run(
        [
            str(executable),
            "compare_squared_distances",
            *(word for point_words in json.loads(fixture.read_text())["points"] for word in point_words),
        ],
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=10,
    )
    valid = json.loads(native.stdout)
    variants: list[str] = [
        '{"predicate":"compare_squared_distances"}',
        '{"predicate":"compare_squared_distances","predicate":"compare_squared_distances"}',
    ]
    unknown = json.loads(json.dumps(valid))
    unknown["counters"]["remaining_unknown"] = 1
    variants.append(canonical_json(unknown))
    boolean_counter = json.loads(json.dumps(valid))
    boolean_counter["counters"]["exact_zeros"] = False
    variants.append(canonical_json(boolean_counter))
    contradicted = json.loads(json.dumps(valid))
    contradicted["sign"] = "positive"
    variants.append(canonical_json(contradicted))
    noncanonical = json.loads(json.dumps(valid))
    noncanonical["left_squared_distance"]["numerator"] = "01"
    variants.append(canonical_json(noncanonical))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-native-output-") as directory:
        for index, output in enumerate(variants):
            fake = Path(directory) / f"fake-{index}"
            make_output_script(fake, output)
            expect_closed_failure(wrapper, fake, fixture)


def check_executable_resolution(wrapper: Path, executable: Path, fixture: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-path-") as directory:
        root = Path(directory)
        environment = os.environ.copy()
        environment["MORSEHGP3D_REPLAY_BIN"] = str(executable.resolve())
        expect_closed_failure(
            wrapper,
            Path("missing-explicit-binary"),
            fixture,
            cwd=root,
            environment=environment,
        )

        if os.name != "nt":
            candidate = root / "chosen-binary"
            candidate.symlink_to(executable.resolve())
            malicious_directory = root / "path"
            malicious_directory.mkdir()
            make_output_script(
                malicious_directory / "chosen-binary",
                '{"predicate":"compare_squared_distances"}',
            )
            environment.pop("MORSEHGP3D_REPLAY_BIN")
            environment["PATH"] = str(malicious_directory) + os.pathsep + environment["PATH"]
            completed = subprocess.run(
                [
                    sys.executable,
                    str(wrapper),
                    str(fixture),
                    "--executable",
                    "chosen-binary",
                ],
                check=True,
                capture_output=True,
                encoding="utf-8",
                timeout=40,
                cwd=root,
                env=environment,
            )
            json.loads(completed.stdout)


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit(
            "usage: check_predicate_replay.py WRAPPER NATIVE_REPLAY FIXTURE_DIRECTORY"
        )
    wrapper = Path(sys.argv[1])
    executable = Path(sys.argv[2])
    fixtures = Path(sys.argv[3])
    check_distance(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_distance(wrapper, executable, fixtures / "distance_equal.json")
    check_orientation(wrapper, executable, fixtures / "orientation_subnormal.json")
    check_invalid_input(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_invalid_native_output(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_executable_resolution(wrapper, executable, fixtures / "distance_one_ulp.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
