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


DOMAINS = {
    1: b"MorseHGP3D/predicate-replay-v1/",
    2: b"MorseHGP3D/predicate-replay-v2/",
}


def canonical_json(value: object) -> str:
    return json.dumps(
        value, allow_nan=False, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )


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


def expected_counters(stage: str, exact_zero: bool) -> dict[str, int]:
    if stage not in {"fp64_filtered", "cpu_multiprecision"}:
        raise AssertionError(f"unsupported certification stage {stage}")
    if exact_zero and stage == "fp64_filtered":
        raise AssertionError("an FP64 filter certified an exact zero")
    return {
        "cpu_multiprecision_certified": 1 if stage == "cpu_multiprecision" else 0,
        "exact_zeros": 1 if exact_zero else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 1 if stage == "fp64_filtered" else 0,
        "remaining_unknown": 0,
    }


def check_distance(
    wrapper: Path,
    executable: Path,
    fixture: Path,
    expected_stage: str | None = None,
    expected_replay_id: str | None = None,
) -> None:
    first_output, replay = run_replay(wrapper, executable, fixture)
    second_output, repeated = run_replay(wrapper, executable, fixture)
    if first_output != second_output or replay["replay_id"] != repeated["replay_id"]:
        raise AssertionError("predicate replay is not byte-stable")
    if re.fullmatch(r"[0-9a-f]{64}", replay["replay_id"]) is None:
        raise AssertionError("predicate replay_id is not a SHA-256 hex digest")
    expected_id = hashlib.sha256(
        DOMAINS[replay["input"]["schema_version"]]
        + canonical_json(replay["input"]).encode("utf-8")
    ).hexdigest()
    if replay["replay_id"] != expected_id:
        raise AssertionError("predicate replay_id does not cover its normalized input")
    if expected_replay_id is not None and replay["replay_id"] != expected_replay_id:
        raise AssertionError("the historical v1 replay identity changed")

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
    stage = result["certification_stage"]
    if expected_stage is not None and stage != expected_stage:
        raise AssertionError(f"distance used {stage} instead of {expected_stage}")
    if result["counters"] != expected_counters(stage, left == right):
        raise AssertionError("distance stage accounting is not closed")


def check_orientation(
    wrapper: Path, executable: Path, fixture: Path, expected_stage: str | None = None
) -> None:
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
    stage = result["certification_stage"]
    if expected_stage is not None and stage != expected_stage:
        raise AssertionError(f"orientation used {stage} instead of {expected_stage}")
    if result["counters"] != expected_counters(stage, expected == 0):
        raise AssertionError("orientation stage accounting is not closed")


def rational3(record: dict[str, str]) -> tuple[Fraction, Fraction, Fraction]:
    denominator = int(record["denominator"])
    return tuple(
        Fraction(int(record[f"{axis}_numerator"]), denominator) for axis in "xyz"
    )  # type: ignore[return-value]


def check_power_bisector(wrapper: Path, executable: Path, fixture: Path) -> None:
    first_output, replay = run_replay(wrapper, executable, fixture)
    second_output, repeated = run_replay(wrapper, executable, fixture)
    if first_output != second_output or replay["replay_id"] != repeated["replay_id"]:
        raise AssertionError("power-bisector replay is not byte-stable")
    expected_id = hashlib.sha256(
        DOMAINS[2] + canonical_json(replay["input"]).encode("utf-8")
    ).hexdigest()
    if replay["replay_id"] != expected_id or replay["schema_version"] != 2:
        raise AssertionError("power-bisector replay does not preserve its v2 identity")
    if replay["replay_id"] != "b90d0731fa0f77f867c44532b08e70cf28ac02417ed5b8a9cc01e581be504da3":
        raise AssertionError("the historical v2 power-bisector identity changed")

    replay_input = replay["input"]
    point_table = [point(words) for words in replay_input["point_table"]]
    r_points = [point_table[index] for index in replay_input["r_ids"]]
    q_points = [point_table[index] for index in replay_input["q_ids"]]
    delta = tuple(
        sum((value[axis] for value in r_points), Fraction())
        - sum((value[axis] for value in q_points), Fraction())
        for axis in range(3)
    )
    delta_norm = sum(
        (sum((coordinate * coordinate for coordinate in value), Fraction())
         for value in r_points),
        Fraction(),
    ) - sum(
        (sum((coordinate * coordinate for coordinate in value), Fraction())
         for value in q_points),
        Fraction(),
    )
    witness = rational3(replay_input["witness"])
    expected = -2 * sum(
        (coordinate * difference for coordinate, difference in zip(witness, delta)),
        Fraction(),
    ) + delta_norm
    result = replay["result"]
    if result["sign"] != sign(expected):
        raise AssertionError("native power-bisector sign differs from Fraction")
    if rational3(result["delta_coordinate_sum_exact"]) != delta:
        raise AssertionError("native power-bisector coordinate delta differs from Fraction")
    if result["delta_squared_norm_sum_exact"] != {
        "denominator": str(delta_norm.denominator),
        "numerator": str(delta_norm.numerator),
    } or result["affine_value_exact"] != {
        "denominator": str(expected.denominator),
        "numerator": str(expected.numerator),
    }:
        raise AssertionError("native power-bisector exact witness differs from Fraction")
    if result["certification_stage"] != "cpu_multiprecision" or result["counters"] != {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": 1 if expected == 0 else 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }:
        raise AssertionError("power-bisector stage accounting is not exact-only and closed")


def check_invalid_power_input(wrapper: Path, executable: Path, fixture: Path) -> None:
    base = json.loads(fixture.read_text(encoding="utf-8"))
    variants: list[dict] = []
    unequal = json.loads(json.dumps(base))
    unequal["q_ids"] = []
    variants.append(unequal)
    both_empty = json.loads(json.dumps(base))
    both_empty["q_ids"] = []
    both_empty["r_ids"] = []
    variants.append(both_empty)
    duplicate = json.loads(json.dumps(base))
    duplicate["point_table"].append(duplicate["point_table"][0])
    duplicate["q_ids"] = [0, 0]
    duplicate["r_ids"] = [0, 1]
    variants.append(duplicate)
    out_of_range = json.loads(json.dumps(base))
    out_of_range["r_ids"] = [2]
    variants.append(out_of_range)
    unsorted = json.loads(json.dumps(base))
    unsorted["q_ids"] = [1, 0]
    unsorted["r_ids"] = [0, 1]
    variants.append(unsorted)
    boolean_id = json.loads(json.dumps(base))
    boolean_id["q_ids"] = [True]
    variants.append(boolean_id)
    negative_id = json.loads(json.dumps(base))
    negative_id["r_ids"] = [-1]
    variants.append(negative_id)
    too_large = json.loads(json.dumps(base))
    too_large["point_table"] = [base["point_table"][0] for _ in range(11)]
    too_large["q_ids"] = list(range(11))
    too_large["r_ids"] = list(range(11))
    variants.append(too_large)
    noncanonical_witness = json.loads(json.dumps(base))
    noncanonical_witness["witness"]["x_numerator"] = "6"
    noncanonical_witness["witness"]["denominator"] = "4"
    variants.append(noncanonical_witness)
    negative_zero = json.loads(json.dumps(base))
    negative_zero["witness"]["y_numerator"] = "-0"
    variants.append(negative_zero)
    zero_denominator = json.loads(json.dumps(base))
    zero_denominator["witness"]["denominator"] = "0"
    variants.append(zero_denominator)
    wrong_witness_unit = json.loads(json.dumps(base))
    wrong_witness_unit["witness"]["unit"] = "input_coordinate_unit_squared"
    variants.append(wrong_witness_unit)
    uppercase_point = json.loads(json.dumps(base))
    uppercase_point["point_table"][0][0] = "3FF0000000000000"
    variants.append(uppercase_point)
    nonfinite_point = json.loads(json.dumps(base))
    nonfinite_point["point_table"][0][0] = "7ff0000000000000"
    variants.append(nonfinite_point)
    unknown = json.loads(json.dumps(base))
    unknown["unexpected"] = 1
    variants.append(unknown)
    wrong_schema = json.loads(json.dumps(base))
    wrong_schema["schema_version"] = 1
    variants.append(wrong_schema)

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-power-input-") as directory:
        for index, value in enumerate(variants):
            invalid = Path(directory) / f"invalid-power-{index}.json"
            invalid.write_text(canonical_json(value) + "\n", encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, str(wrapper), str(invalid), "--executable", str(executable)],
                capture_output=True,
                encoding="utf-8",
                timeout=40,
            )
            if completed.returncode != 2 or "failed closed" not in completed.stderr:
                raise AssertionError(f"invalid power replay input {index} did not fail closed")


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
    mismatched_stage = json.loads(json.dumps(valid))
    mismatched_stage["certification_stage"] = "fp64_filtered"
    variants.append(canonical_json(mismatched_stage))
    filtered_zero = json.loads(json.dumps(valid))
    filtered_zero["certification_stage"] = "fp64_filtered"
    filtered_zero["sign"] = "zero"
    filtered_zero["counters"]["cpu_multiprecision_certified"] = 0
    filtered_zero["counters"]["exact_zeros"] = 1
    filtered_zero["counters"]["fp64_filtered_certified"] = 1
    variants.append(canonical_json(filtered_zero))
    contradicted = json.loads(json.dumps(valid))
    contradicted["sign"] = "positive"
    variants.append(canonical_json(contradicted))
    noncanonical = json.loads(json.dumps(valid))
    noncanonical["left_squared_distance"]["numerator"] = "01"
    variants.append(canonical_json(noncanonical))
    coherent_but_false = json.loads(json.dumps(valid))
    coherent_but_false["left_squared_distance"]["numerator"] = "0"
    coherent_but_false["left_squared_distance"]["denominator"] = "1"
    coherent_but_false["right_squared_distance"]["numerator"] = "0"
    coherent_but_false["right_squared_distance"]["denominator"] = "1"
    coherent_but_false["sign"] = "zero"
    coherent_but_false["counters"]["exact_zeros"] = 1
    variants.append(canonical_json(coherent_but_false))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-native-output-") as directory:
        for index, output in enumerate(variants):
            fake = Path(directory) / f"fake-{index}"
            make_output_script(fake, output)
            expect_closed_failure(wrapper, fake, fixture)


def check_invalid_orientation_native_output(
    wrapper: Path, executable: Path, fixture: Path
) -> None:
    _, replay = run_replay(wrapper, executable, fixture)
    coherent_but_false = json.loads(json.dumps(replay["result"]))
    coherent_but_false["determinant_exact"] = {
        "denominator": "1",
        "numerator": "0",
    }
    coherent_but_false["sign"] = "zero"
    coherent_but_false["counters"]["exact_zeros"] = 1
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-orientation-output-") as directory:
        fake = Path(directory) / "fake-orientation"
        make_output_script(fake, canonical_json(coherent_but_false))
        expect_closed_failure(wrapper, fake, fixture)


def check_invalid_power_native_output(
    wrapper: Path, executable: Path, fixture: Path
) -> None:
    _, replay = run_replay(wrapper, executable, fixture)
    valid = replay["result"]
    variants: list[dict] = []
    contradicted = json.loads(json.dumps(valid))
    contradicted["sign"] = "positive"
    variants.append(contradicted)
    wrong_delta = json.loads(json.dumps(valid))
    wrong_delta["delta_coordinate_sum_exact"]["x_numerator"] = "2"
    variants.append(wrong_delta)
    wrong_norm = json.loads(json.dumps(valid))
    wrong_norm["delta_squared_norm_sum_exact"]["numerator"] = "4"
    variants.append(wrong_norm)
    wrong_affine = json.loads(json.dumps(valid))
    wrong_affine["affine_value_exact"]["numerator"] = "1"
    variants.append(wrong_affine)
    noncanonical_delta = json.loads(json.dumps(valid))
    noncanonical_delta["delta_coordinate_sum_exact"]["x_numerator"] = "2"
    noncanonical_delta["delta_coordinate_sum_exact"]["denominator"] = "2"
    variants.append(noncanonical_delta)

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-power-output-") as directory:
        for index, value in enumerate(variants):
            fake = Path(directory) / f"fake-power-{index}"
            make_output_script(fake, canonical_json(value))
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


def check_native_batch_fail_closed(executable: Path) -> None:
    zero_point = "0000000000000000 0000000000000000 0000000000000000"
    eleven_ids = " ".join(str(index) for index in range(11))
    invalid_inputs = [
        "\n",
        "unknown_predicate\n",
        "power_bisector_side 3 0 0 2 2 "
        "3ff0000000000000 0000000000000000 0000000000000000 "
        "4000000000000000 0000000000000000 0000000000000000 "
        "2 0 0 2 0 1\n",
        "power_bisector_side 0 0 0 1 11 "
        + " ".join(zero_point for _ in range(11))
        + f" 11 {eleven_ids} 11 {eleven_ids}\n",
    ]
    for index, batch_input in enumerate(invalid_inputs):
        completed = subprocess.run(
            [str(executable), "--batch"],
            input=batch_input,
            capture_output=True,
            encoding="utf-8",
            timeout=40,
        )
        if (
            completed.returncode != 2
            or "failed closed" not in completed.stderr
            or "batch replay line 1" not in completed.stderr
        ):
            raise AssertionError(f"invalid native batch {index} did not fail closed")


def check_native_output_failure(executable: Path, fixture: Path) -> None:
    if os.name == "nt" or not Path("/dev/full").exists():
        return
    points = json.loads(fixture.read_text(encoding="utf-8"))["points"]
    command = [
        str(executable),
        "compare_squared_distances",
        *(word for point_words in points for word in point_words),
    ]
    with Path("/dev/full").open("wb") as sink:
        completed = subprocess.run(
            command,
            stdout=sink,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            timeout=40,
        )
    if completed.returncode != 2 or "failed closed" not in completed.stderr:
        raise AssertionError("a native replay output failure did not fail closed")


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit(
            "usage: check_predicate_replay.py WRAPPER NATIVE_REPLAY FIXTURE_DIRECTORY"
        )
    wrapper = Path(sys.argv[1])
    executable = Path(sys.argv[2])
    fixtures = Path(sys.argv[3])
    check_distance(
        wrapper,
        executable,
        fixtures / "distance_one_ulp.json",
        expected_replay_id="db04d3582bb0a813cd60cf48ca5f0ce630b1f57ed209e63f363f8088a0bdd56c",
    )
    check_distance(
        wrapper,
        executable,
        fixtures / "distance_equal.json",
        expected_stage="cpu_multiprecision",
    )
    check_distance(
        wrapper,
        executable,
        fixtures / "distance_filtered.json",
        expected_stage="fp64_filtered",
    )
    check_orientation(wrapper, executable, fixtures / "orientation_subnormal.json")
    check_orientation(
        wrapper,
        executable,
        fixtures / "orientation_daz_regression.json",
    )
    check_orientation(
        wrapper,
        executable,
        fixtures / "orientation_filtered.json",
        expected_stage="fp64_filtered",
    )
    check_power_bisector(
        wrapper, executable, fixtures / "power_bisector_rational_zero.json"
    )
    check_invalid_input(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_invalid_power_input(
        wrapper, executable, fixtures / "power_bisector_rational_zero.json"
    )
    check_invalid_native_output(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_invalid_orientation_native_output(
        wrapper, executable, fixtures / "orientation_subnormal.json"
    )
    check_invalid_power_native_output(
        wrapper, executable, fixtures / "power_bisector_rational_zero.json"
    )
    check_executable_resolution(wrapper, executable, fixtures / "distance_one_ulp.json")
    check_native_batch_fail_closed(executable)
    check_native_output_failure(executable, fixtures / "distance_one_ulp.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
