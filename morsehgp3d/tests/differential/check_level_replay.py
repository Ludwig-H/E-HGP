#!/usr/bin/env python3
"""Audit the closed v6 exact-level ordering replay with Fraction oracles."""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import os
import subprocess
import sys
import tempfile
from copy import deepcopy
from fractions import Fraction
from pathlib import Path
from typing import Any

# The v6 ExactLevel contract is unbounded and the native side uses
# boost::multiprecision::cpp_int. Keep the independent Fraction oracle aligned
# with that contract on Python 3.11 and later.
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

INPUT_KIND = "morsehgp3d_predicate_replay_input"
RESULT_KIND = "morsehgp3d_predicate_replay_result"
COMPARE_PREDICATE = "compare_exact_levels"
BATCH_PREDICATE = "canonical_level_batches"
V6_DOMAIN = b"MorseHGP3D/predicate-replay-v6/"
MAX_SAFE_JSON_INTEGER = 9_007_199_254_740_991
MAX_LEVEL_BATCH_ITEMS = 64
MAX_JSON_INTEGER_DIGITS = len(str(MAX_SAFE_JSON_INTEGER))
MAX_EXHAUSTIVE_ITEM_PERMUTATION_COUNT = 5
MAX_SAMPLED_ITEM_PERMUTATION_COUNT = 8
HISTORICAL_SENTINELS = {
    "distance_one_ulp.json": (
        1,
        "db04d3582bb0a813cd60cf48ca5f0ce630b1f57ed209e63f363f8088a0bdd56c",
    ),
    "power_bisector_rational_zero.json": (
        2,
        "b90d0731fa0f77f867c44532b08e70cf28ac02417ed5b8a9cc01e581be504da3",
    ),
    "affine_plane_translated.json": (
        3,
        "f676bacc74860b38647592c2ea525444b7f588824c8d06eae141602d77524762",
    ),
    "center_triangle_nondyadic.json": (
        4,
        "e25d8274fb4331a630afd8995b38fa427c0b2bb72aa644b1d1943c1e974c73ec",
    ),
    "sphere_rational_boundary.json": (
        5,
        "b2fc1c9710f09984ac90c56fc522a43eae52d90606e0f5b5f1d6ade4c1b74c3d",
    ),
}
EXPECTED_COMPARE_FIXTURES = frozenset(
    {
        "level_compare_cross_minus_one.json",
        "level_compare_cross_plus_one.json",
        "level_compare_huge_cancellation.json",
        "level_compare_rational_equal.json",
        "level_compare_zero_equal.json",
    }
)
EXPECTED_BATCH_FIXTURES = frozenset(
    {
        "level_batch_adjacent_levels.json",
        "level_batch_equal_distinct_supports.json",
        "level_batch_mixed_levels.json",
        "level_batch_reduced_provenance.json",
        "level_batch_singleton_zero.json",
        "level_batch_support_size_tiebreak.json",
    }
)
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}
WRAPPER_TIMEOUT_SECONDS = 40


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def strict_json_loads(text: str) -> Any:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> None:
        raise ValueError(f"non-finite JSON number: {value}")

    def parse_bounded_integer(value: str) -> int:
        digits = value[1:] if value.startswith("-") else value
        if len(digits) > MAX_JSON_INTEGER_DIGITS:
            raise ValueError("a JSON integer exceeds the exact numeric token domain")
        return int(value)

    return json.loads(
        text,
        object_pairs_hook=reject_duplicates,
        parse_int=parse_bounded_integer,
        parse_constant=reject_nonfinite,
    )


def load_json(path: Path) -> dict[str, Any]:
    value = strict_json_loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name} is not a JSON object")
    return value


def level_from_record(value: Any) -> Fraction:
    if not isinstance(value, dict) or set(value) != {
        "denominator",
        "numerator",
        "schema_version",
        "unit",
    }:
        raise AssertionError("an ExactLevel record is not closed")
    if (
        value["schema_version"] != "2.0.0"
        or value["unit"] != "input_coordinate_unit_squared"
    ):
        raise AssertionError("an ExactLevel record changed contract")
    numerator = value["numerator"]
    denominator = value["denominator"]
    if type(numerator) is not str or type(denominator) is not str:
        raise AssertionError("ExactLevel integers are not strings")
    if str(int(numerator)) != numerator or str(int(denominator)) != denominator:
        raise AssertionError("ExactLevel integers are not canonical decimal strings")
    if int(numerator) < 0 or int(denominator) <= 0:
        raise AssertionError(
            "an ExactLevel is negative or has a nonpositive denominator"
        )
    if math.gcd(int(numerator), int(denominator)) != 1:
        raise AssertionError("an ExactLevel is not reduced")
    return Fraction(int(numerator), int(denominator))


def level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise AssertionError("an exact squared level cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def sign(value: int) -> str:
    return "negative" if value < 0 else "zero" if value == 0 else "positive"


def empty_counters() -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 0,
        "exact_zeros": 0,
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def compare_oracle(value: dict[str, Any]) -> dict[str, Any]:
    left_record = value["left_squared_level_exact"]
    right_record = value["right_squared_level_exact"]
    left = level_from_record(left_record)
    right = level_from_record(right_record)
    difference = int(left_record["numerator"]) * int(right_record["denominator"]) - int(
        right_record["numerator"]
    ) * int(left_record["denominator"])
    expected_sign = sign(difference)
    if expected_sign != sign(-1 if left < right else 0 if left == right else 1):
        raise AssertionError("Fraction and cross-product level comparisons disagree")
    counters = empty_counters()
    counters["cpu_multiprecision_certified"] = 1
    counters["exact_zeros"] = 1 if difference == 0 else 0
    return {
        "certification_stage": "cpu_multiprecision",
        "counters": counters,
        "cross_product_difference_exact": str(difference),
        "equal": difference == 0,
        "ordering": {
            "negative": "less",
            "zero": "equal",
            "positive": "greater",
        }[expected_sign],
        "predicate": COMPARE_PREDICATE,
        "sign": expected_sign,
    }


def canonical_ids(value: Any, label: str) -> tuple[int, ...]:
    if not isinstance(value, list) or not 1 <= len(value) <= 4:
        raise AssertionError(f"{label} must contain one to four identifiers")
    if any(type(identifier) is not int for identifier in value):
        raise AssertionError(f"{label} contains a non-integer identifier")
    if any(
        identifier < 0 or identifier > MAX_SAFE_JSON_INTEGER for identifier in value
    ):
        raise AssertionError(f"{label} contains an out-of-range identifier")
    if len(set(value)) != len(value):
        raise AssertionError(f"{label} contains duplicate identifiers")
    return tuple(sorted(value))


def batch_oracle(value: dict[str, Any]) -> dict[str, Any]:
    grouped: dict[tuple[int, ...], tuple[Fraction, dict[tuple[int, ...], int]]] = {}
    for index, emission in enumerate(value["items"]):
        minimal = canonical_ids(
            emission["minimal_support_ids"], f"minimal support {index}"
        )
        source = canonical_ids(
            emission["source_support_ids"], f"source support {index}"
        )
        if not set(minimal).issubset(source):
            raise AssertionError("a minimal support is absent from its source support")
        level = level_from_record(emission["squared_level_exact"])
        if minimal not in grouped:
            grouped[minimal] = (level, {})
        previous_level, sources = grouped[minimal]
        if previous_level != level:
            raise AssertionError("one minimal support carries contradictory levels")
        sources[source] = sources.get(source, 0) + 1

    ordered = sorted(
        ((level, minimal, sources) for minimal, (level, sources) in grouped.items()),
        key=lambda entry: (entry[0], len(entry[1]), entry[1]),
    )
    batches: list[dict[str, Any]] = []
    unique_count = 0
    for level, minimal, sources in ordered:
        if (
            not batches
            or level_from_record(batches[-1]["squared_level_exact"]) != level
        ):
            batches.append(
                {
                    "emission_count": 0,
                    "squared_level_exact": level_record(level),
                    "supports": [],
                }
            )
        support_count = sum(sources.values())
        batches[-1]["emission_count"] += support_count
        batches[-1]["supports"].append(
            {
                "emission_count": support_count,
                "minimal_support_ids": list(minimal),
                "source_provenance": [
                    {
                        "emission_count": sources[source],
                        "source_support_ids": list(source),
                    }
                    for source in sorted(sources, key=lambda ids: (len(ids), ids))
                ],
                "squared_level_exact": level_record(level),
            }
        )
        unique_count += len(sources)
    emission_count = len(value["items"])
    return {
        "duplicate_emission_count": emission_count - unique_count,
        "emission_count": emission_count,
        "equal_level_batches": batches,
        "predicate": BATCH_PREDICATE,
        "unique_emission_count": unique_count,
    }


def expected_result(value: dict[str, Any]) -> dict[str, Any]:
    return (
        compare_oracle(value)
        if value["predicate"] == COMPARE_PREDICATE
        else batch_oracle(value)
    )


def expected_replay_id(value: dict[str, Any]) -> str:
    return hashlib.sha256(V6_DOMAIN + canonical_json(value).encode("utf-8")).hexdigest()


def run_wrapper(
    wrapper: Path,
    executable: Path,
    input_path: Path,
    executable_prefix_arguments: tuple[str, ...] = (),
) -> tuple[str, dict[str, Any]]:
    command = [
        sys.executable,
        str(wrapper),
        str(input_path),
        "--executable",
        str(executable),
    ]
    command.extend(
        f"--executable-prefix-argument={argument}"
        for argument in executable_prefix_arguments
    )
    completed = subprocess.run(
        command,
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.stderr:
        raise AssertionError("a successful replay wrote diagnostics to stderr")
    replay = strict_json_loads(completed.stdout)
    if not isinstance(replay, dict):
        raise AssertionError("the wrapper result is not a JSON object")
    if completed.stdout != canonical_json(replay) + "\n":
        raise AssertionError("the wrapper result is not canonical JSON")
    return completed.stdout, replay


def audit_replay(replay: dict[str, Any], fixture: dict[str, Any]) -> None:
    if set(replay) != {"input", "kind", "replay_id", "result", "schema_version"}:
        raise AssertionError("the v6 replay envelope is not closed")
    if (
        replay["kind"] != RESULT_KIND
        or replay["schema_version"] != 6
        or replay["input"] != fixture
        or replay["replay_id"] != expected_replay_id(fixture)
    ):
        raise AssertionError("the v6 replay envelope or identifier is invalid")
    if replay["result"] != expected_result(fixture):
        raise AssertionError(
            "the native replay differs from the independent Fraction oracle"
        )


def write_case(
    directory: Path,
    name: str,
    value: dict[str, Any] | None = None,
    *,
    raw: str | None = None,
) -> Path:
    path = directory / name
    path.write_text(
        raw if raw is not None else json.dumps(value, indent=2), encoding="utf-8"
    )
    return path


def check_historical_sentinels(
    wrapper: Path, executable: Path, fixture_directory: Path
) -> None:
    for filename, (version, replay_id) in HISTORICAL_SENTINELS.items():
        _, replay = run_wrapper(wrapper, executable, fixture_directory / filename)
        if (
            replay.get("schema_version") != version
            or replay.get("replay_id") != replay_id
        ):
            raise AssertionError(f"historical replay identity changed for {filename}")


def check_compare_symmetry(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> int:
    checked = 0
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-symmetry-") as temporary:
        directory = Path(temporary)
        for name, fixture in fixtures.items():
            swapped = deepcopy(fixture)
            (
                swapped["left_squared_level_exact"],
                swapped["right_squared_level_exact"],
            ) = (
                swapped["right_squared_level_exact"],
                swapped["left_squared_level_exact"],
            )
            path = write_case(directory, f"swapped-{name}", swapped)
            _, replay = run_wrapper(wrapper, executable, path)
            audit_replay(replay, swapped)
            original = compare_oracle(fixture)
            observed = replay["result"]
            if (
                int(observed["cross_product_difference_exact"])
                != -int(original["cross_product_difference_exact"])
                or observed["equal"] != original["equal"]
                or observed["sign"]
                != {"negative": "positive", "zero": "zero", "positive": "negative"}[
                    original["sign"]
                ]
            ):
                raise AssertionError(f"level comparison symmetry failed for {name}")
            checked += 1
    return checked


def scheduled_item_permutations(item_count: int) -> tuple[tuple[int, ...], ...]:
    if not 1 <= item_count <= MAX_LEVEL_BATCH_ITEMS:
        raise AssertionError("a permutation schedule requires one through 64 items")
    indices = tuple(range(item_count))
    if item_count <= MAX_EXHAUSTIVE_ITEM_PERMUTATION_COUNT:
        return tuple(itertools.permutations(indices))

    midpoint = (item_count + 1) // 2
    first_half = indices[:midpoint]
    second_half = indices[midpoint:]
    interleaved: list[int] = []
    for index, value in enumerate(first_half):
        interleaved.append(value)
        if index < len(second_half):
            interleaved.append(second_half[index])
    candidates = (
        indices,
        tuple(reversed(indices)),
        indices[1:] + indices[:1],
        indices[-1:] + indices[:-1],
        indices[midpoint:] + indices[:midpoint],
        indices[::2] + indices[1::2],
        indices[1::2] + indices[::2],
        tuple(interleaved),
    )
    schedule = tuple(dict.fromkeys(candidates))
    if len(schedule) > MAX_SAMPLED_ITEM_PERMUTATION_COUNT:
        raise AssertionError("the bounded item-permutation schedule grew unexpectedly")
    if any(tuple(sorted(permutation)) != indices for permutation in schedule):
        raise AssertionError("the bounded item-permutation schedule is malformed")
    return schedule


def check_batch_permutations(
    wrapper: Path,
    executable: Path,
    fixtures: dict[str, dict[str, Any]],
) -> tuple[int, int]:
    item_permutations = 0
    identifier_permutations = 0
    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-level-permutations-"
    ) as temporary:
        directory = Path(temporary)
        for name, fixture in fixtures.items():
            expected = batch_oracle(fixture)
            for permutation in scheduled_item_permutations(len(fixture["items"])):
                permuted = deepcopy(fixture)
                permuted["items"] = [
                    deepcopy(fixture["items"][index]) for index in permutation
                ]
                path = write_case(
                    directory,
                    f"items-{item_permutations}.json",
                    permuted,
                )
                _, replay = run_wrapper(wrapper, executable, path)
                audit_replay(replay, permuted)
                if replay["result"] != expected:
                    raise AssertionError(f"item arrival order changed {name}")
                item_permutations += 1

            reversed_ids = deepcopy(fixture)
            for item in reversed_ids["items"]:
                item["minimal_support_ids"].reverse()
                item["source_support_ids"].reverse()
            path = write_case(
                directory,
                f"ids-{identifier_permutations}.json",
                reversed_ids,
            )
            _, replay = run_wrapper(wrapper, executable, path)
            audit_replay(replay, reversed_ids)
            if replay["result"] != expected:
                raise AssertionError(f"support identifier order changed {name}")
            identifier_permutations += 1
    return item_permutations, identifier_permutations


def check_large_batch_permutations(wrapper: Path, executable: Path) -> int:
    fixture = {
        "items": [
            {
                "minimal_support_ids": [MAX_SAFE_JSON_INTEGER - index],
                "source_support_ids": [MAX_SAFE_JSON_INTEGER - index],
                "squared_level_exact": level_record(Fraction((17 * index) % 13, 7)),
            }
            for index in range(MAX_LEVEL_BATCH_ITEMS)
        ],
        "kind": INPUT_KIND,
        "predicate": BATCH_PREDICATE,
        "schema_version": 6,
    }
    expected = batch_oracle(fixture)
    schedule = scheduled_item_permutations(MAX_LEVEL_BATCH_ITEMS)
    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-level-large-permutations-"
    ) as temporary:
        directory = Path(temporary)
        for sample_index, permutation in enumerate(schedule):
            permuted = deepcopy(fixture)
            permuted["items"] = [
                deepcopy(fixture["items"][index]) for index in permutation
            ]
            path = write_case(
                directory,
                f"large-items-{sample_index}.json",
                permuted,
            )
            _, replay = run_wrapper(wrapper, executable, path)
            audit_replay(replay, permuted)
            if replay["result"] != expected:
                raise AssertionError(
                    "a sampled 64-item arrival order changed canonical batching"
                )
    return len(schedule)


def check_common_scaling(
    wrapper: Path,
    executable: Path,
    compare_fixture: dict[str, Any],
    batch_fixture: dict[str, Any],
) -> int:
    factor = Fraction(8, 3)
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-scaling-") as temporary:
        directory = Path(temporary)
        scaled_compare = deepcopy(compare_fixture)
        for field in ("left_squared_level_exact", "right_squared_level_exact"):
            scaled_compare[field] = level_record(
                level_from_record(scaled_compare[field]) * factor
            )
        compare_path = write_case(directory, "scaled-compare.json", scaled_compare)
        _, compare_replay = run_wrapper(wrapper, executable, compare_path)
        audit_replay(compare_replay, scaled_compare)
        if compare_replay["result"]["sign"] != compare_oracle(compare_fixture)["sign"]:
            raise AssertionError("positive common scaling changed level order")

        scaled_batch = deepcopy(batch_fixture)
        for item in scaled_batch["items"]:
            item["squared_level_exact"] = level_record(
                level_from_record(item["squared_level_exact"]) * factor
            )
        batch_path = write_case(directory, "scaled-batch.json", scaled_batch)
        _, batch_replay = run_wrapper(wrapper, executable, batch_path)
        audit_replay(batch_replay, scaled_batch)
        original = batch_oracle(batch_fixture)["equal_level_batches"]
        scaled = batch_replay["result"]["equal_level_batches"]
        if len(original) != len(scaled):
            raise AssertionError("positive common scaling changed batch count")
        for original_batch, scaled_batch_result in zip(original, scaled):
            if original_batch["supports"] != scaled_batch_result["supports"]:
                original_supports = [
                    {
                        key: value
                        for key, value in support.items()
                        if key != "squared_level_exact"
                    }
                    for support in original_batch["supports"]
                ]
                scaled_supports = [
                    {
                        key: value
                        for key, value in support.items()
                        if key != "squared_level_exact"
                    }
                    for support in scaled_batch_result["supports"]
                ]
                if original_supports != scaled_supports:
                    raise AssertionError(
                        "positive common scaling changed batch supports"
                    )
        return 2


def check_json_normalization(
    wrapper: Path,
    executable: Path,
    fixture: dict[str, Any],
    fixture_path: Path,
) -> None:
    with tempfile.TemporaryDirectory(
        prefix="morsehgp3d-level-json-order-"
    ) as temporary:
        reordered = dict(reversed(list(fixture.items())))
        path = write_case(
            Path(temporary),
            "reordered.json",
            raw=json.dumps(reordered, indent=4),
        )
        _, observed = run_wrapper(wrapper, executable, path)
        _, original = run_wrapper(wrapper, executable, fixture_path)
        if (
            observed["replay_id"] != original["replay_id"]
            or observed["result"] != original["result"]
        ):
            raise AssertionError("JSON whitespace or key order changed v6 identity")


def check_large_decimal_level(
    wrapper: Path,
    executable: Path,
    compare_base: dict[str, Any],
) -> int:
    decimal_digit_count = 5_000
    huge_numerator = "1" + "0" * (decimal_digit_count - 1)
    fixture = deepcopy(compare_base)
    fixture["left_squared_level_exact"] = {
        "denominator": "1",
        "numerator": huge_numerator,
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }
    fixture["right_squared_level_exact"] = level_record(Fraction())
    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-decimal-") as temporary:
        path = write_case(Path(temporary), "decimal-5000.json", fixture)
        normal_stdout, normal_replay = run_wrapper(wrapper, executable, path)
        mp_stdout, mp_replay = run_wrapper(
            wrapper,
            executable,
            path,
            ("--multiprecision-only",),
        )
    audit_replay(normal_replay, fixture)
    if normal_stdout != mp_stdout or normal_replay != mp_replay:
        raise AssertionError(
            "a large decimal level changed in multiprecision-only mode"
        )
    result = normal_replay["result"]
    if (
        result["cross_product_difference_exact"] != huge_numerator
        or result["sign"] != "positive"
    ):
        raise AssertionError("the 5,000-digit ExactLevel lost its exact witness")
    return decimal_digit_count


def expect_closed_failure(
    wrapper: Path,
    executable: Path,
    input_path: Path,
    executable_prefix_arguments: tuple[str, ...] = (),
) -> None:
    command = [
        sys.executable,
        str(wrapper),
        str(input_path),
        "--executable",
        str(executable),
    ]
    command.extend(
        f"--executable-prefix-argument={argument}"
        for argument in executable_prefix_arguments
    )
    completed = subprocess.run(
        command,
        check=False,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or completed.stdout:
        raise AssertionError(f"{input_path.name} did not fail closed")
    if not completed.stderr.startswith("predicate replay failed closed:"):
        raise AssertionError(f"{input_path.name} omitted the closed-failure diagnostic")


def check_invalid_inputs(
    wrapper: Path,
    executable: Path,
    compare_base: dict[str, Any],
    batch_base: dict[str, Any],
) -> int:
    variants: list[tuple[str, dict[str, Any] | None, str | None]] = []

    def add(name: str, value: dict[str, Any]) -> None:
        variants.append((name, value, None))

    value = deepcopy(compare_base)
    value["extra"] = True
    add("compare-extra-field.json", value)
    value = deepcopy(compare_base)
    del value["left_squared_level_exact"]
    add("compare-missing-level.json", value)
    value = deepcopy(compare_base)
    value["schema_version"] = 5
    add("compare-old-version.json", value)
    value = deepcopy(compare_base)
    value["schema_version"] = True
    add("compare-boolean-version.json", value)
    value = deepcopy(compare_base)
    value["schema_version"] = 6.0
    add("compare-floating-version.json", value)
    value = deepcopy(compare_base)
    value["predicate"] = "unknown_level_predicate"
    add("compare-unknown-predicate.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["numerator"] = "-1"
    add("compare-negative-level.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["numerator"] = "01"
    add("compare-leading-zero.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["denominator"] = "0"
    add("compare-zero-denominator.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["numerator"] = "2"
    value["left_squared_level_exact"]["denominator"] = "4"
    add("compare-unreduced-level.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["unit"] = "wrong"
    add("compare-wrong-unit.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["schema_version"] = "1.0.0"
    add("compare-wrong-level-version.json", value)
    value = deepcopy(compare_base)
    value["left_squared_level_exact"]["extra"] = "closed"
    add("compare-level-extra.json", value)

    value = deepcopy(batch_base)
    value["items"] = []
    add("batch-empty.json", value)
    value = deepcopy(batch_base)
    value["items"] = [deepcopy(value["items"][0]) for _ in range(65)]
    add("batch-too-many.json", value)
    value = deepcopy(batch_base)
    value["items"] = "not-an-array"
    add("batch-items-string.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["extra"] = True
    add("batch-item-extra.json", value)
    value = deepcopy(batch_base)
    del value["items"][0]["source_support_ids"]
    add("batch-item-missing.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = []
    add("batch-empty-minimal.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [0, 1, 2, 3, 4]
    value["items"][0]["source_support_ids"] = [0, 1, 2, 3]
    add("batch-large-minimal.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["source_support_ids"] = [0, 1, 2, 3, 4]
    add("batch-large-source.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [0, 0]
    value["items"][0]["source_support_ids"] = [0]
    add("batch-duplicate-id.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [True]
    value["items"][0]["source_support_ids"] = [True]
    add("batch-boolean-id.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [-1]
    value["items"][0]["source_support_ids"] = [-1]
    add("batch-negative-id.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [MAX_SAFE_JSON_INTEGER + 1]
    value["items"][0]["source_support_ids"] = [MAX_SAFE_JSON_INTEGER + 1]
    add("batch-large-id.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["minimal_support_ids"] = [99]
    add("batch-minimal-not-source.json", value)
    value = deepcopy(batch_base)
    contradictory = deepcopy(value["items"][0])
    contradictory["squared_level_exact"] = level_record(
        level_from_record(contradictory["squared_level_exact"]) + 1
    )
    contradictory["source_support_ids"] = list(
        reversed(contradictory["source_support_ids"])
    )
    value["items"].append(contradictory)
    add("batch-conflicting-level.json", value)
    value = deepcopy(batch_base)
    value["items"][0]["squared_level_exact"]["numerator"] = "2"
    value["items"][0]["squared_level_exact"]["denominator"] = "2"
    add("batch-unreduced-level.json", value)
    value = deepcopy(batch_base)
    value["schema_version"] = 5
    add("batch-old-version.json", value)
    value = deepcopy(batch_base)
    value["extra"] = True
    add("batch-extra-field.json", value)

    duplicate_key = canonical_json(compare_base).replace(
        '"schema_version":6', '"schema_version":6,"schema_version":6'
    )
    variants.append(("duplicate-json-key.json", None, duplicate_key))
    nonfinite = canonical_json(batch_base).replace(
        '"schema_version":6', '"nonfinite":NaN,"schema_version":6', 1
    )
    variants.append(("nonfinite-json.json", None, nonfinite))
    first_minimal_id = batch_base["items"][0]["minimal_support_ids"][0]
    oversized_numeric = canonical_json(batch_base).replace(
        f'"minimal_support_ids":[{first_minimal_id}',
        f'"minimal_support_ids":[{"9" * 5_000}',
        1,
    )
    variants.append(("oversized-json-integer.json", None, oversized_numeric))
    deeply_nested = "[" * 5_000 + "0" + "]" * 5_000
    variants.append(("deeply-nested-json.json", None, deeply_nested))

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-invalid-") as temporary:
        directory = Path(temporary)
        for name, value, raw in variants:
            expect_closed_failure(
                wrapper,
                executable,
                write_case(directory, name, value, raw=raw),
            )
    return len(variants)


def expect_native_failure(executable: Path, arguments: list[str]) -> None:
    completed = subprocess.run(
        [str(executable), *arguments],
        check=False,
        capture_output=True,
        encoding="utf-8",
        timeout=WRAPPER_TIMEOUT_SECONDS,
    )
    if completed.returncode != 2 or completed.stdout:
        raise AssertionError("an invalid native level command did not fail closed")
    if not completed.stderr.startswith("predicate replay failed closed:"):
        raise AssertionError("an invalid native level command omitted its diagnostic")


def check_invalid_native_commands(executable: Path) -> int:
    commands = [
        [COMPARE_PREDICATE, "0", "1", "0"],
        [COMPARE_PREDICATE, "0", "01", "0", "1"],
        [COMPARE_PREDICATE, "-1", "1", "0", "1"],
        [BATCH_PREDICATE, "0"],
        [BATCH_PREDICATE, "65"],
        [BATCH_PREDICATE, "1", "0", "1", "1"],
        [BATCH_PREDICATE, "1", "0", "1", "1", "0", "1", "1"],
        [BATCH_PREDICATE, "1", "0", "1", "1", "0", "1", "1", "0", "extra"],
        [
            BATCH_PREDICATE,
            "1",
            "0",
            "1",
            "1",
            str(MAX_SAFE_JSON_INTEGER + 1),
            "1",
            str(MAX_SAFE_JSON_INTEGER + 1),
        ],
        [
            BATCH_PREDICATE,
            "2",
            "0",
            "1",
            "1",
            "0",
            "1",
            "0",
            "1",
            "1",
            "1",
            "0",
            "1",
            "0",
        ],
    ]
    for command in commands:
        expect_native_failure(executable, command)
    return len(commands)


def check_native_batch_boundaries(executable: Path) -> int:
    zero_level = level_record(Fraction())

    def native_arguments(items: list[dict[str, Any]]) -> list[str]:
        arguments = [BATCH_PREDICATE, str(len(items))]
        for item in items:
            level = item["squared_level_exact"]
            minimal = item["minimal_support_ids"]
            source = item["source_support_ids"]
            arguments.extend(
                [level["numerator"], level["denominator"], str(len(minimal))]
            )
            arguments.extend(str(identifier) for identifier in minimal)
            arguments.append(str(len(source)))
            arguments.extend(str(identifier) for identifier in source)
        return arguments

    maximum_item = {
        "minimal_support_ids": [MAX_SAFE_JSON_INTEGER],
        "source_support_ids": [MAX_SAFE_JSON_INTEGER],
        "squared_level_exact": zero_level,
    }
    fixtures = (
        {
            "items": [maximum_item],
            "kind": INPUT_KIND,
            "predicate": BATCH_PREDICATE,
            "schema_version": 6,
        },
        {
            "items": [deepcopy(maximum_item) for _ in range(MAX_LEVEL_BATCH_ITEMS)],
            "kind": INPUT_KIND,
            "predicate": BATCH_PREDICATE,
            "schema_version": 6,
        },
    )
    for fixture in fixtures:
        completed = subprocess.run(
            [str(executable), *native_arguments(fixture["items"])],
            check=True,
            capture_output=True,
            encoding="utf-8",
            timeout=WRAPPER_TIMEOUT_SECONDS,
        )
        if completed.stderr:
            raise AssertionError("a valid native boundary batch wrote diagnostics")
        observed = strict_json_loads(completed.stdout)
        if completed.stdout != canonical_json(observed) + "\n":
            raise AssertionError("a valid native boundary batch was not canonical JSON")
        if observed != batch_oracle(fixture):
            raise AssertionError(
                "a valid native boundary batch differs from the oracle"
            )
    return len(fixtures)


def make_fake_native(
    path: Path,
    output: dict[str, Any],
    *,
    diagnostic: str | None = None,
    canonical: bool = True,
) -> None:
    payload = canonical_json(output) if canonical else json.dumps(output, indent=2)
    diagnostic_statement = (
        f"sys.stderr.write({diagnostic!r} + '\\n')\n" if diagnostic else ""
    )
    path.write_text(
        "import sys\n"
        + diagnostic_statement
        + f"sys.stdout.write({payload!r} + '\\n')\n",
        encoding="utf-8",
    )


def changed(base: dict[str, Any], field: str, value: Any) -> dict[str, Any]:
    result = deepcopy(base)
    result[field] = value
    return result


def check_false_native_outputs(
    wrapper: Path,
    executable: Path,
    fixture_paths: dict[str, Path],
) -> tuple[int, int, int]:
    def result(name: str) -> dict[str, Any]:
        return run_wrapper(wrapper, executable, fixture_paths[name])[1]["result"]

    less_name = "level_compare_cross_minus_one.json"
    equal_name = "level_compare_zero_equal.json"
    mixed_name = "level_batch_mixed_levels.json"
    equal_batch_name = "level_batch_equal_distinct_supports.json"
    provenance_name = "level_batch_reduced_provenance.json"
    adjacent_name = "level_batch_adjacent_levels.json"
    less = result(less_name)
    equal = result(equal_name)
    mixed = result(mixed_name)
    equal_batch = result(equal_batch_name)
    provenance = result(provenance_name)
    adjacent = result(adjacent_name)

    variants: list[tuple[str, str, dict[str, Any]]] = [
        ("compare-wrong-sign", less_name, changed(less, "sign", "positive")),
        ("compare-wrong-order", less_name, changed(less, "ordering", "greater")),
        ("compare-wrong-equal", less_name, changed(less, "equal", True)),
        (
            "compare-wrong-witness",
            less_name,
            changed(less, "cross_product_difference_exact", "-2"),
        ),
        (
            "compare-noncanonical-witness",
            less_name,
            changed(less, "cross_product_difference_exact", "-01"),
        ),
        (
            "compare-negative-zero-witness",
            equal_name,
            changed(equal, "cross_product_difference_exact", "-0"),
        ),
        (
            "compare-wrong-stage",
            less_name,
            changed(less, "certification_stage", "fp64_filtered"),
        ),
        (
            "compare-wrong-counter",
            less_name,
            changed(
                less,
                "counters",
                {**less["counters"], "cpu_multiprecision_certified": 0},
            ),
        ),
        (
            "compare-wrong-zero-counter",
            equal_name,
            changed(equal, "counters", {**equal["counters"], "exact_zeros": 0}),
        ),
        (
            "compare-wrong-predicate",
            less_name,
            changed(less, "predicate", BATCH_PREDICATE),
        ),
        ("compare-extra", less_name, {**less, "extra": True}),
    ]

    reversed_batches = deepcopy(mixed)
    reversed_batches["equal_level_batches"].reverse()
    variants.append(("batch-reversed-levels", mixed_name, reversed_batches))

    split_equal = deepcopy(equal_batch)
    batch = split_equal["equal_level_batches"][0]
    second_support = batch["supports"].pop()
    batch["emission_count"] -= second_support["emission_count"]
    split_equal["equal_level_batches"].append(
        {
            "emission_count": second_support["emission_count"],
            "squared_level_exact": deepcopy(batch["squared_level_exact"]),
            "supports": [second_support],
        }
    )
    variants.append(("batch-split-equal-level", equal_batch_name, split_equal))

    merged_adjacent = deepcopy(adjacent)
    second = merged_adjacent["equal_level_batches"].pop()
    first = merged_adjacent["equal_level_batches"][0]
    first["supports"].extend(second["supports"])
    first["emission_count"] += second["emission_count"]
    for support in first["supports"]:
        support["squared_level_exact"] = deepcopy(first["squared_level_exact"])
    variants.append(("batch-merge-distinct-levels", adjacent_name, merged_adjacent))

    missing_support = deepcopy(equal_batch)
    removed = missing_support["equal_level_batches"][0]["supports"].pop()
    missing_support["equal_level_batches"][0]["emission_count"] -= removed[
        "emission_count"
    ]
    missing_support["emission_count"] -= removed["emission_count"]
    missing_support["unique_emission_count"] -= 1
    variants.append(("batch-lost-support", equal_batch_name, missing_support))

    reversed_supports = deepcopy(equal_batch)
    reversed_supports["equal_level_batches"][0]["supports"].reverse()
    variants.append(("batch-reversed-supports", equal_batch_name, reversed_supports))

    unsorted_minimal = deepcopy(equal_batch)
    unsorted_minimal["equal_level_batches"][0]["supports"][0][
        "minimal_support_ids"
    ].reverse()
    variants.append(("batch-unsorted-minimal", equal_batch_name, unsorted_minimal))

    reversed_provenance = deepcopy(provenance)
    reversed_provenance["equal_level_batches"][0]["supports"][0][
        "source_provenance"
    ].reverse()
    variants.append(("batch-reversed-provenance", provenance_name, reversed_provenance))

    unsorted_source = deepcopy(provenance)
    unsorted_source["equal_level_batches"][0]["supports"][0]["source_provenance"][0][
        "source_support_ids"
    ].reverse()
    variants.append(("batch-unsorted-source", provenance_name, unsorted_source))

    zero_count = deepcopy(provenance)
    zero_count["equal_level_batches"][0]["supports"][0]["source_provenance"][0][
        "emission_count"
    ] = 0
    variants.append(("batch-zero-emission", provenance_name, zero_count))

    wrong_source_count = deepcopy(provenance)
    wrong_source_count["equal_level_batches"][0]["supports"][0]["source_provenance"][0][
        "emission_count"
    ] = 1
    variants.append(("batch-wrong-source-count", provenance_name, wrong_source_count))

    wrong_support_count = deepcopy(provenance)
    wrong_support_count["equal_level_batches"][0]["supports"][0]["emission_count"] = 2
    variants.append(("batch-wrong-support-count", provenance_name, wrong_support_count))

    wrong_batch_count = deepcopy(provenance)
    wrong_batch_count["equal_level_batches"][0]["emission_count"] = 2
    variants.append(("batch-wrong-batch-count", provenance_name, wrong_batch_count))

    wrong_global_count = deepcopy(provenance)
    wrong_global_count["emission_count"] = 2
    variants.append(("batch-wrong-global-count", provenance_name, wrong_global_count))

    wrong_unique_count = deepcopy(provenance)
    wrong_unique_count["unique_emission_count"] = 3
    wrong_unique_count["duplicate_emission_count"] = 0
    variants.append(("batch-wrong-unique-count", provenance_name, wrong_unique_count))

    wrong_duplicate_count = deepcopy(provenance)
    wrong_duplicate_count["duplicate_emission_count"] = 0
    variants.append(
        ("batch-wrong-duplicate-count", provenance_name, wrong_duplicate_count)
    )

    wrong_support_level = deepcopy(provenance)
    wrong_support_level["equal_level_batches"][0]["supports"][0][
        "squared_level_exact"
    ] = level_record(Fraction(2))
    variants.append(("batch-wrong-support-level", provenance_name, wrong_support_level))

    noncanonical_level = deepcopy(provenance)
    noncanonical_level["equal_level_batches"][0]["squared_level_exact"][
        "numerator"
    ] = "2"
    noncanonical_level["equal_level_batches"][0]["squared_level_exact"][
        "denominator"
    ] = "2"
    variants.append(("batch-noncanonical-level", provenance_name, noncanonical_level))

    variants.extend(
        [
            (
                "batch-wrong-predicate",
                provenance_name,
                changed(provenance, "predicate", COMPARE_PREDICATE),
            ),
            ("batch-extra", provenance_name, {**provenance, "extra": True}),
        ]
    )

    with tempfile.TemporaryDirectory(prefix="morsehgp3d-level-fake-") as temporary:
        directory = Path(temporary)
        python_executable = Path(sys.executable).resolve(strict=True)
        for name, fixture_name, output in variants:
            fake = directory / f"fake-{name}.py"
            make_fake_native(fake, output)
            expect_closed_failure(
                wrapper,
                python_executable,
                fixture_paths[fixture_name],
                (str(fake),),
            )
        noncanonical = directory / "fake-noncanonical.py"
        make_fake_native(noncanonical, less, canonical=False)
        expect_closed_failure(
            wrapper,
            python_executable,
            fixture_paths[less_name],
            (str(noncanonical),),
        )
        diagnostic = directory / "fake-diagnostic.py"
        make_fake_native(
            diagnostic,
            less,
            diagnostic="unexpected native exact-level warning",
        )
        expect_closed_failure(
            wrapper,
            python_executable,
            fixture_paths[less_name],
            (str(diagnostic),),
        )
    return len(variants), 1, 1


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
    fixture_directory = arguments.fixture_directory.resolve(strict=True)
    compare_paths = {
        path.name: path
        for path in sorted(fixture_directory.glob("level_compare_*.json"))
    }
    batch_paths = {
        path.name: path for path in sorted(fixture_directory.glob("level_batch_*.json"))
    }
    if frozenset(compare_paths) != EXPECTED_COMPARE_FIXTURES:
        raise AssertionError(
            "the frozen exact-level comparison fixture set changed: "
            f"missing={sorted(EXPECTED_COMPARE_FIXTURES - frozenset(compare_paths))}, "
            f"unexpected={sorted(frozenset(compare_paths) - EXPECTED_COMPARE_FIXTURES)}"
        )
    if frozenset(batch_paths) != EXPECTED_BATCH_FIXTURES:
        raise AssertionError(
            "the frozen exact-level batch fixture set changed: "
            f"missing={sorted(EXPECTED_BATCH_FIXTURES - frozenset(batch_paths))}, "
            f"unexpected={sorted(frozenset(batch_paths) - EXPECTED_BATCH_FIXTURES)}"
        )

    compare_fixtures = {name: load_json(path) for name, path in compare_paths.items()}
    batch_fixtures = {name: load_json(path) for name, path in batch_paths.items()}
    fixture_paths = {**compare_paths, **batch_paths}
    fixtures = {**compare_fixtures, **batch_fixtures}

    signs: set[str] = set()
    support_sizes: set[int] = set()
    batch_count = 0
    unique_emission_count = 0
    duplicate_emission_count = 0
    for name, fixture in fixtures.items():
        first_stdout, first_replay = run_wrapper(
            wrapper, executable, fixture_paths[name]
        )
        second_stdout, second_replay = run_wrapper(
            wrapper, executable, fixture_paths[name]
        )
        mp_stdout, mp_replay = run_wrapper(
            wrapper,
            executable,
            fixture_paths[name],
            ("--multiprecision-only",),
        )
        if (
            first_stdout != second_stdout
            or first_replay != second_replay
            or first_stdout != mp_stdout
            or first_replay != mp_replay
        ):
            raise AssertionError(f"{name} is not byte-stable in normal and MP modes")
        audit_replay(first_replay, fixture)
        result = first_replay["result"]
        if fixture["predicate"] == COMPARE_PREDICATE:
            signs.add(result["sign"])
        else:
            batch_count += len(result["equal_level_batches"])
            unique_emission_count += result["unique_emission_count"]
            duplicate_emission_count += result["duplicate_emission_count"]
            for batch in result["equal_level_batches"]:
                for support in batch["supports"]:
                    support_sizes.add(len(support["minimal_support_ids"]))

    if signs != {"negative", "zero", "positive"}:
        raise AssertionError(f"the v6 fixtures miss exact comparison signs: {signs}")
    if support_sizes != {1, 2, 3, 4}:
        raise AssertionError(f"the v6 fixtures miss support sizes: {support_sizes}")
    minus_one = compare_oracle(compare_fixtures["level_compare_cross_minus_one.json"])
    plus_one = compare_oracle(compare_fixtures["level_compare_cross_plus_one.json"])
    if (
        minus_one["cross_product_difference_exact"] != "-1"
        or plus_one["cross_product_difference_exact"] != "1"
    ):
        raise AssertionError("the adjacent exact-level fixtures lost their unit gap")
    reduced = batch_oracle(batch_fixtures["level_batch_reduced_provenance.json"])
    if (
        reduced["emission_count"] != 3
        or reduced["unique_emission_count"] != 2
        or reduced["duplicate_emission_count"] != 1
        or len(reduced["equal_level_batches"][0]["supports"][0]["source_provenance"])
        != 2
    ):
        raise AssertionError(
            "the reduced-provenance fixture lost its aggregation contract"
        )

    check_historical_sentinels(wrapper, executable, fixture_directory)
    symmetry_count = check_compare_symmetry(wrapper, executable, compare_fixtures)
    item_permutation_count, identifier_permutation_count = check_batch_permutations(
        wrapper, executable, batch_fixtures
    )
    scaling_count = check_common_scaling(
        wrapper,
        executable,
        compare_fixtures["level_compare_cross_minus_one.json"],
        batch_fixtures["level_batch_mixed_levels.json"],
    )
    check_json_normalization(
        wrapper,
        executable,
        batch_fixtures["level_batch_reduced_provenance.json"],
        batch_paths["level_batch_reduced_provenance.json"],
    )
    large_decimal_digit_count = check_large_decimal_level(
        wrapper,
        executable,
        compare_fixtures["level_compare_zero_equal.json"],
    )
    large_batch_permutation_sample_count = check_large_batch_permutations(
        wrapper,
        executable,
    )
    invalid_input_count = check_invalid_inputs(
        wrapper,
        executable,
        compare_fixtures["level_compare_zero_equal.json"],
        batch_fixtures["level_batch_reduced_provenance.json"],
    )
    invalid_native_count = check_invalid_native_commands(executable)
    native_boundary_count = check_native_batch_boundaries(executable)
    false_native_count, noncanonical_count, stderr_count = check_false_native_outputs(
        wrapper, executable, fixture_paths
    )
    print(
        "exact-level replay differential checks passed: "
        f"fixtures={len(fixtures)}, compare_fixtures={len(compare_fixtures)}, "
        f"batch_fixtures={len(batch_fixtures)}, signs={len(signs)}, "
        f"support_sizes={len(support_sizes)}, batches={batch_count}, "
        f"unique_emissions={unique_emission_count}, "
        f"duplicate_emissions={duplicate_emission_count}, "
        f"compare_symmetries={symmetry_count}, "
        f"item_permutations={item_permutation_count}, "
        f"identifier_permutations={identifier_permutation_count}, "
        f"scaling_metamorphisms={scaling_count}, invalid_inputs={invalid_input_count}, "
        f"large_decimal_digits={large_decimal_digit_count}, "
        f"large_batch_permutation_samples={large_batch_permutation_sample_count}, "
        f"invalid_native_commands={invalid_native_count}, "
        f"native_batch_boundaries={native_boundary_count}, "
        f"false_native_outputs={false_native_count}, "
        f"noncanonical_stdout={noncanonical_count}, unexpected_stderr={stderr_count}, "
        f"historical_sentinels={len(HISTORICAL_SENTINELS)}, "
        "normal_mp_byte_identical=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
