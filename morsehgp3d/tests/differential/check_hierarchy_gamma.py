#!/usr/bin/env python3
"""Exact open-cut differential for the bounded C++ Gamma catalogue."""

from __future__ import annotations

import argparse
import json
import math
import struct
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import TypeAlias


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from reference.morsehgp3d_oracle.gamma import build_gamma_filtration


DEFAULT_TIMEOUT_SECONDS = 30
UINT64_MASK = (1 << 64) - 1
SIGN_MASK = 1 << 63
EXPONENT_MASK = 0x7FF << 52

ExactPoint: TypeAlias = tuple[Fraction, Fraction, Fraction]
InputPoint: TypeAlias = tuple[int, int, int]


@dataclass(frozen=True)
class FixedCase:
    name: str
    input_points: tuple[InputPoint, ...]
    input_labels: tuple[str, ...]
    order: int
    strict_cut_squared_level: Fraction
    source_input_indices: tuple[tuple[int, ...], ...]


FIXED_CASES = (
    FixedCase(
        "binary_ac",
        ((-2, 0, 0), (2, 0, 0)),
        ("A", "C"),
        1,
        Fraction(4),
        ((0,), (1,)),
    ),
    FixedCase(
        "mediated_apc",
        ((-2, 0, 0), (0, 3, 0), (2, 0, 0)),
        ("A", "P", "C"),
        1,
        Fraction(4),
        ((0,), (2,)),
    ),
    FixedCase(
        "ternary_abcpq",
        ((-2, 0, 0), (0, 3, 0), (2, 0, 0), (2, 2, 0), (-2, 2, 0)),
        ("A", "B", "C", "P", "Q"),
        2,
        Fraction(169, 36),
        ((1, 2), (0, 2), (0, 1)),
    ),
    FixedCase(
        "gabriel_silent_incidences",
        ((0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)),
        ("A", "B", "C", "D", "E"),
        2,
        Fraction(83886, 3563),
        ((0, 2), (3, 4), (0, 1), (1, 2)),
    ),
    FixedCase(
        "order_ten_eleven_point_coface",
        tuple((coordinate, 0, 0) for coordinate in range(11)),
        tuple(f"P{index}" for index in range(11)),
        10,
        Fraction(26),
        (tuple(range(10)),),
    ),
)


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def strict_json_object(line: str) -> dict[str, object]:
    def reject_constant(value: str) -> object:
        raise ValueError(f"non-finite JSON constant {value!r}")

    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key {key!r}")
            result[key] = value
        return result

    value = json.loads(
        line,
        parse_constant=reject_constant,
        object_pairs_hook=reject_duplicates,
    )
    if not isinstance(value, dict):
        raise ValueError("each dump line must be one JSON object")
    if canonical_json(value) != line:
        raise ValueError("a dump line is not canonical JSON")
    return value


def first_difference(expected: object, actual: object, path: str = "$") -> str | None:
    if type(expected) is not type(actual):
        return (
            f"{path}: expected type {type(expected).__name__}, "
            f"observed {type(actual).__name__}"
        )
    if isinstance(expected, dict):
        expected_keys = set(expected)
        actual_keys = set(actual)  # type: ignore[arg-type]
        if expected_keys != actual_keys:
            return (
                f"{path}: missing keys {sorted(expected_keys - actual_keys)!r}, "
                f"extra keys {sorted(actual_keys - expected_keys)!r}"
            )
        for key in sorted(expected):
            difference = first_difference(
                expected[key], actual[key], f"{path}.{key}"  # type: ignore[index]
            )
            if difference is not None:
                return difference
        return None
    if isinstance(expected, list):
        if len(expected) != len(actual):  # type: ignore[arg-type]
            return (
                f"{path}: expected length {len(expected)}, "
                f"observed {len(actual)}"  # type: ignore[arg-type]
            )
        for index, (expected_item, actual_item) in enumerate(
            zip(expected, actual)  # type: ignore[arg-type]
        ):
            difference = first_difference(
                expected_item, actual_item, f"{path}[{index}]"
            )
            if difference is not None:
                return difference
        return None
    if expected != actual:
        return f"{path}: expected {expected!r}, observed {actual!r}"
    return None


def binary64_word(value: int) -> str:
    encoded = float(value)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != value:
        raise AssertionError(f"fixture coordinate {value!r} is not exact binary64")
    return struct.pack(">d", encoded).hex()


def bits_from_word(word: str) -> int:
    if len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 word {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("the Gamma fixture contains a non-finite coordinate")
    return bits


def canonical_word(word: str) -> str:
    bits = bits_from_word(word)
    if bits == SIGN_MASK:
        bits = 0
    return f"{bits:016x}"


def total_order_key(word: str) -> int:
    bits = bits_from_word(canonical_word(word))
    if bits & SIGN_MASK:
        return (~bits) & UINT64_MASK
    return bits ^ SIGN_MASK


def fraction_from_word(word: str) -> Fraction:
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def ratio_record(value: Fraction | int) -> dict[str, str]:
    exact = Fraction(value)
    return {
        "denominator": str(exact.denominator),
        "numerator": str(exact.numerator),
    }


def expected_canonical_points(fixed_case: FixedCase) -> list[dict[str, object]]:
    candidates = []
    for source_index, point in enumerate(fixed_case.input_points):
        words = tuple(binary64_word(coordinate) for coordinate in point)
        key = tuple(total_order_key(word) for word in words)
        candidates.append((key, words, source_index))
    candidates.sort(key=lambda candidate: candidate[0])
    return [
        {
            "id": point_id,
            "input_bits": list(words),
            "label": fixed_case.input_labels[source_index],
            "source_index": source_index,
        }
        for point_id, (_, words, source_index) in enumerate(candidates)
    ]


def emitted_point_order(
    fixed_case: FixedCase, actual: dict[str, object]
) -> tuple[list[dict[str, object]], tuple[ExactPoint, ...], dict[int, int]]:
    expected_points = expected_canonical_points(fixed_case)
    actual_points = actual.get("canonical_points")
    difference = first_difference(expected_points, actual_points, "$.canonical_points")
    if difference is not None:
        raise AssertionError(difference)

    points = []
    point_id_by_source_index = {}
    for record in actual_points:  # type: ignore[union-attr]
        if not isinstance(record, dict):
            raise AssertionError("a canonical point record is not an object")
        point_id = record["id"]
        source_index = record["source_index"]
        words = record["input_bits"]
        if not isinstance(point_id, int) or not isinstance(source_index, int):
            raise AssertionError("canonical point identifiers must be integers")
        if not isinstance(words, list) or len(words) != 3:
            raise AssertionError("canonical point input_bits must have length three")
        exact_point = tuple(fraction_from_word(str(word)) for word in words)
        points.append(exact_point)
        point_id_by_source_index[source_index] = point_id
    return expected_points, tuple(points), point_id_by_source_index  # type: ignore[return-value]


def expected_document(
    fixed_case: FixedCase, actual: dict[str, object]
) -> dict[str, object]:
    canonical_points, points, point_id_by_source_index = emitted_point_order(
        fixed_case, actual
    )
    filtration = build_gamma_filtration(points, fixed_case.order)
    cut = filtration.cut(fixed_case.strict_cut_squared_level, closed=False)

    facet_levels = {facet.point_ids: facet.squared_level for facet in filtration.facets}
    active_facets = [
        {
            "point_ids": list(facet),
            "squared_level": ratio_record(facet_levels[facet]),
        }
        for facet in cut.active_facet_point_ids
    ]
    active_cofaces = [
        {
            "facet_point_ids": [list(facet) for facet in coface.facet_point_ids],
            "point_ids": list(coface.point_ids),
            "squared_level": ratio_record(coface.squared_level),
        }
        for coface in filtration.cofaces
        if coface.squared_level < fixed_case.strict_cut_squared_level
    ]
    components = [
        {
            "canonical_representative_facet_point_ids": list(
                component.facet_point_ids[0]
            ),
            "facet_point_ids": [list(facet) for facet in component.facet_point_ids],
        }
        for component in cut.components
    ]
    component_index_by_facet = {
        facet: component_index
        for component_index, component in enumerate(cut.components)
        for facet in component.facet_point_ids
    }

    sources = []
    for source_input_indices in fixed_case.source_input_indices:
        source = tuple(
            sorted(point_id_by_source_index[index] for index in source_input_indices)
        )
        squared_level = facet_levels[source]
        active = squared_level < fixed_case.strict_cut_squared_level
        sources.append(
            {
                "active_strictly_below_cut": active,
                "component_index": component_index_by_facet[source] if active else None,
                "source_facet_point_ids": list(source),
                "squared_level": ratio_record(squared_level),
            }
        )

    return {
        "active_cofaces": active_cofaces,
        "active_facets": active_facets,
        "canonical_points": canonical_points,
        "case": fixed_case.name,
        "closed": False,
        "components": components,
        "order": fixed_case.order,
        "source_classifications": sources,
        "strict_cut_squared_level": ratio_record(
            fixed_case.strict_cut_squared_level
        ),
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="path to hierarchy_gamma_dump")
    parser.add_argument(
        "--timeout-seconds",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="subprocess timeout (default: %(default)s)",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout_seconds <= 0:
        raise ValueError("--timeout-seconds must be positive")
    executable = arguments.executable.resolve(strict=True)
    completed = subprocess.run(
        [str(executable)],
        check=False,
        capture_output=True,
        text=True,
        timeout=arguments.timeout_seconds,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"Gamma dump exited with {completed.returncode}: {completed.stderr.strip()}"
        )
    lines = completed.stdout.splitlines()
    if len(lines) != len(FIXED_CASES) or any(not line for line in lines):
        raise AssertionError(
            f"expected {len(FIXED_CASES)} nonempty dump lines, observed {len(lines)}"
        )

    documents = [strict_json_object(line) for line in lines]
    observed_names = [document.get("case") for document in documents]
    expected_names = [fixed_case.name for fixed_case in FIXED_CASES]
    if observed_names != expected_names:
        raise AssertionError(
            f"expected fixed cases {expected_names!r}, observed {observed_names!r}"
        )
    for fixed_case, actual in zip(FIXED_CASES, documents):
        expected = expected_document(fixed_case, actual)
        difference = first_difference(expected, actual)
        if difference is not None:
            raise AssertionError(f"{fixed_case.name}: {difference}")

    print("hierarchy Gamma differential passed for 5 exact open-cut cases")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, RuntimeError, ValueError) as error:
        print(f"hierarchy Gamma differential failed: {error}", file=sys.stderr)
        raise SystemExit(1) from error
