#!/usr/bin/env python3
"""Differential audit of the exact Phase 4 brute-force spatial oracle."""

from __future__ import annotations

import argparse
import hashlib
import heapq
import json
import struct
import subprocess
import sys
from fractions import Fraction
from pathlib import Path


PROTOCOL_HEADER = "morsehgp3d-spatial-query-v1"
RESULT_SCHEMA = "morsehgp3d.spatial_query_dump.v1"
UINT64_MASK = (1 << 64) - 1
NEGATIVE_ZERO_BITS = 1 << 63
EXPONENT_MASK = 0x7FF << 52
DEFAULT_TIMEOUT_SECONDS = 240
CAMPAIGN_MIN_SIZE = 1
CAMPAIGN_MAX_SIZE = 1000
CAMPAIGN_CASE_ID_BASE = 10_000

PointWords = tuple[str, str, str]
ExactPoint = tuple[Fraction, Fraction, Fraction]


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def greatest_common_divisor(left: int, right: int) -> int:
    left = abs(left)
    right = abs(right)
    while right:
        left, right = right, left % right
    return left


def normalized_query_components(
    x_numerator: int,
    y_numerator: int,
    z_numerator: int,
    denominator: int,
) -> tuple[int, int, int, int]:
    if denominator <= 0:
        raise AssertionError("a generated query denominator must be positive")
    divisor = denominator
    for numerator in (x_numerator, y_numerator, z_numerator):
        divisor = greatest_common_divisor(divisor, numerator)
    return (
        x_numerator // divisor,
        y_numerator // divisor,
        z_numerator // divisor,
        denominator // divisor,
    )


def binary64_word(value: Fraction | int) -> str:
    exact = Fraction(value)
    encoded = float(exact)
    if Fraction.from_float(encoded) != exact:
        raise AssertionError(f"{exact} is not exactly representable as binary64")
    return struct.pack(">d", encoded).hex()


def bits_from_word(word: str) -> int:
    if len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 word: {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word: {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("the spatial campaign contains a non-finite coordinate")
    return bits


def canonical_word(word: str) -> str:
    bits = bits_from_word(word)
    if bits == NEGATIVE_ZERO_BITS:
        bits = 0
    return f"{bits:016x}"


def total_order_key(word: str) -> int:
    bits = bits_from_word(canonical_word(word))
    if bits & NEGATIVE_ZERO_BITS:
        return (~bits) & UINT64_MASK
    return bits ^ NEGATIVE_ZERO_BITS


def fraction_from_word(word: str) -> Fraction:
    canonical = canonical_word(word)
    value = struct.unpack(">d", bytes.fromhex(canonical))[0]
    return Fraction.from_float(value)


def point_words(x: Fraction | int, y: Fraction | int, z: Fraction | int) -> PointWords:
    return (binary64_word(x), binary64_word(y), binary64_word(z))


def exact_point(words: PointWords) -> ExactPoint:
    return (
        fraction_from_word(words[0]),
        fraction_from_word(words[1]),
        fraction_from_word(words[2]),
    )


def canonicalize_points(
    source_points: tuple[PointWords, ...],
) -> tuple[list[dict[str, object]], list[ExactPoint]]:
    candidates: list[tuple[tuple[int, int, int], PointWords, int]] = []
    for source_index, source_words in enumerate(source_points):
        words = (
            canonical_word(source_words[0]),
            canonical_word(source_words[1]),
            canonical_word(source_words[2]),
        )
        keys = (
            total_order_key(words[0]),
            total_order_key(words[1]),
            total_order_key(words[2]),
        )
        candidates.append((keys, words, source_index))
    candidates.sort(key=lambda candidate: candidate[0])
    for index in range(1, len(candidates)):
        if candidates[index - 1][1] == candidates[index][1]:
            raise AssertionError("the Python campaign generated a duplicate point")

    records: list[dict[str, object]] = []
    points: list[ExactPoint] = []
    for point_id, (_, words, source_index) in enumerate(candidates):
        records.append(
            {
                "id": point_id,
                "input_bits": list(words),
                "source_index": source_index,
            }
        )
        points.append(exact_point(words))
    return records, points


def squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum(
        ((left[axis] - right[axis]) ** 2 for axis in range(3)),
        Fraction(),
    )


def level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise AssertionError("a squared distance cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
    }


def query_record(query_components: tuple[int, int, int, int]) -> dict[str, str]:
    x_numerator, y_numerator, z_numerator, denominator = query_components
    return {
        "denominator": str(denominator),
        "x_numerator": str(x_numerator),
        "y_numerator": str(y_numerator),
        "z_numerator": str(z_numerator),
    }


def query_point(query_components: tuple[int, int, int, int]) -> ExactPoint:
    x_numerator, y_numerator, z_numerator, denominator = query_components
    return (
        Fraction(x_numerator, denominator),
        Fraction(y_numerator, denominator),
        Fraction(z_numerator, denominator),
    )


def make_case(
    case_id: int,
    source_points: tuple[PointWords, ...],
    query_components: tuple[int, int, int, int],
    requested_rank: int,
    run_m_star: int,
    exclusion_ids: tuple[int, ...],
    label: str,
) -> dict[str, object]:
    normalized_query = normalized_query_components(*query_components)
    if not source_points:
        raise AssertionError("a differential case must contain at least one point")
    if not 0 <= run_m_star <= 9:
        raise AssertionError("run_m_star must lie in zero through nine")
    if len(set(exclusion_ids)) != len(exclusion_ids):
        raise AssertionError("a generated exclusion set must not repeat IDs")
    if len(exclusion_ids) > run_m_star:
        raise AssertionError("a generated exclusion set exceeds run_m_star")
    if any(point_id < 0 or point_id >= len(source_points) for point_id in exclusion_ids):
        raise AssertionError("a generated exclusion ID is outside the cloud")
    eligible_count = len(source_points) - len(exclusion_ids)
    if not 1 <= requested_rank <= eligible_count:
        raise AssertionError("a generated rank is outside the eligible set")
    return {
        "case_id": case_id,
        "exclusion_ids": exclusion_ids,
        "label": label,
        "query": normalized_query,
        "requested_rank": requested_rank,
        "run_m_star": run_m_star,
        "source_points": source_points,
    }


def case_protocol_line(case: dict[str, object]) -> str:
    source_points = case["source_points"]
    exclusion_ids = case["exclusion_ids"]
    query = case["query"]
    if not isinstance(source_points, tuple):
        raise AssertionError("invalid case point payload")
    if not isinstance(exclusion_ids, tuple) or not isinstance(query, tuple):
        raise AssertionError("invalid case query payload")
    tokens = [
        "case",
        str(case["case_id"]),
        str(len(source_points)),
        str(case["requested_rank"]),
        str(case["run_m_star"]),
        str(len(exclusion_ids)),
        *(str(value) for value in query),
        *(str(point_id) for point_id in exclusion_ids),
    ]
    for words in source_points:
        tokens.extend(words)
    return " ".join(tokens)


def expected_result(case: dict[str, object]) -> dict[str, object]:
    source_points = case["source_points"]
    exclusion_ids_input = case["exclusion_ids"]
    query_components = case["query"]
    requested_rank = case["requested_rank"]
    run_m_star = case["run_m_star"]
    if not isinstance(source_points, tuple):
        raise AssertionError("invalid source points")
    if not isinstance(exclusion_ids_input, tuple):
        raise AssertionError("invalid exclusions")
    if not isinstance(query_components, tuple):
        raise AssertionError("invalid query")
    if not isinstance(requested_rank, int) or not isinstance(run_m_star, int):
        raise AssertionError("invalid rank contract")

    canonical_records, canonical_points = canonicalize_points(source_points)
    exclusions = sorted(exclusion_ids_input)
    exclusion_set = set(exclusions)
    query = query_point(query_components)
    all_distances = [
        squared_distance(point, query) for point in canonical_points
    ]
    eligible = [
        (distance, point_id)
        for point_id, distance in enumerate(all_distances)
        if point_id not in exclusion_set
    ]
    cutoff = heapq.nsmallest(requested_rank, eligible)[-1][0]
    strict = [entry for entry in eligible if entry[0] < cutoff]
    strict.sort()
    shell_ids = [point_id for distance, point_id in eligible if distance == cutoff]
    shell_choice_count = requested_rank - len(strict)
    canonical_choice = sorted(
        [point_id for _, point_id in strict] + shell_ids[:shell_choice_count]
    )

    interior_ids = [
        point_id
        for point_id, distance in enumerate(all_distances)
        if distance < cutoff
    ]
    global_shell_ids = [
        point_id
        for point_id, distance in enumerate(all_distances)
        if distance == cutoff
    ]
    exterior_ids = [
        point_id
        for point_id, distance in enumerate(all_distances)
        if distance > cutoff
    ]
    return {
        "canonical_points": canonical_records,
        "case_id": case["case_id"],
        "closed_ball": {
            "closed_rank": len(interior_ids) + len(global_shell_ids),
            "evaluation_count": len(canonical_points),
            "exterior_ids": exterior_ids,
            "interior_ids": interior_ids,
            "partition_complete": True,
            "shell_ids": global_shell_ids,
            "squared_radius": level_record(cutoff),
        },
        "exclusions": {
            "ids": exclusions,
            "point_count": len(canonical_points),
            "run_m_star": run_m_star,
        },
        "query": query_record(query_components),
        "schema": RESULT_SCHEMA,
        "top_k": {
            "canonical_choice_ids": canonical_choice,
            "cutoff_shell_ids": shell_ids,
            "cutoff_squared_distance": level_record(cutoff),
            "distance_evaluation_count": len(eligible),
            "eligible_point_count": len(eligible),
            "requested_rank": requested_rank,
            "shell_complete": True,
            "strict_below": [
                {
                    "point_id": point_id,
                    "squared_distance": level_record(distance),
                }
                for distance, point_id in strict
            ],
        },
    }


def strict_json_object(text: str) -> dict[str, object]:
    def reject_duplicate_keys(
        pairs: list[tuple[str, object]],
    ) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> None:
        raise ValueError(f"non-finite JSON number: {value}")

    parsed = json.loads(
        text,
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    if not isinstance(parsed, dict):
        raise AssertionError("the C++ dump emitted a non-object JSON value")
    return parsed


def first_difference(
    expected: object,
    actual: object,
    path: str = "$",
) -> str | None:
    if type(expected) is not type(actual):
        return f"{path}: expected type {type(expected).__name__}, got {type(actual).__name__}"
    if isinstance(expected, dict) and isinstance(actual, dict):
        if expected.keys() != actual.keys():
            return f"{path}: expected keys {sorted(expected)}, got {sorted(actual)}"
        for key in expected:
            difference = first_difference(expected[key], actual[key], f"{path}.{key}")
            if difference is not None:
                return difference
        return None
    if isinstance(expected, list) and isinstance(actual, list):
        if len(expected) != len(actual):
            return f"{path}: expected length {len(expected)}, got {len(actual)}"
        for index, (expected_item, actual_item) in enumerate(zip(expected, actual)):
            difference = first_difference(
                expected_item,
                actual_item,
                f"{path}[{index}]",
            )
            if difference is not None:
                return difference
        return None
    if expected != actual:
        return f"{path}: expected {expected!r}, got {actual!r}"
    return None


def load_relevant_gp_fixture(path: Path) -> tuple[dict[str, object], dict[str, object]]:
    raw = path.read_text(encoding="utf-8")
    value = strict_json_object(raw)
    if raw != canonical_json(value) + "\n":
        raise AssertionError(f"{path} is not canonical one-line JSON")
    if value.get("schema") != "morsehgp3d.spatial_regression.v1":
        raise AssertionError(f"{path} has an unexpected schema")
    if value.get("fixture_id") != "relevant-gp-extra-shell-above-smax-v1":
        raise AssertionError(f"{path} has an unexpected fixture_id")
    point_payload = value.get("points_binary64")
    center = value.get("center")
    expected = value.get("expected")
    manifest = value.get("manifest")
    squared_radius = value.get("squared_radius")
    candidate_ids = value.get("candidate_ids")
    s_max = value.get("s_max")
    if not isinstance(point_payload, list) or not isinstance(center, dict):
        raise AssertionError(f"{path} has an invalid point/center payload")
    if (
        not isinstance(expected, dict)
        or not isinstance(manifest, dict)
        or not isinstance(squared_radius, dict)
    ):
        raise AssertionError(f"{path} has an invalid expected/radius payload")
    if not isinstance(candidate_ids, list) or not isinstance(s_max, int):
        raise AssertionError(f"{path} has an invalid RelevantGP payload")
    points: list[PointWords] = []
    for words in point_payload:
        if not isinstance(words, list) or len(words) != 3:
            raise AssertionError(f"{path} contains an invalid point")
        points.append((str(words[0]), str(words[1]), str(words[2])))
    coordinate_digest = hashlib.sha256(
        canonical_json(point_payload).encode("utf-8")
    ).hexdigest()
    expected_manifest = {
        "coordinate_serialization": "canonical-json-points_binary64-v1",
        "coordinates_sha256": coordinate_digest,
        "duplication_policy": "reject-geometric-duplicates",
        "expected_status": "unsupported_degeneracy",
        "generator": {
            "name": "manual-minimization",
            "parameters": {
                "point_count": 3,
                "purpose": "global-shell-rank-exceeds-s-max",
            },
            "seed": None,
            "version": "1",
        },
        "genericity_assumptions": "none-boundary-degeneracy-is-intentional",
        "k_max": 2,
        "license": "repository-license",
        "objective": "reject-rank-truncated-global-shell-certificates",
        "profile": "hgp_reduced",
        "provenance": "internal-minimal-regression",
        "units": "input_coordinate_unit",
    }
    if manifest != expected_manifest:
        raise AssertionError(f"{path} has an incomplete or stale fixture manifest")
    query_components = (
        int(center["x_numerator"]),
        int(center["y_numerator"]),
        int(center["z_numerator"]),
        int(center["denominator"]),
    )
    fixture_case = make_case(
        case_id=3,
        source_points=tuple(points),
        query_components=query_components,
        requested_rank=s_max,
        run_m_star=0,
        exclusion_ids=(),
        label="fixture-relevant-gp-extra-shell-above-smax",
    )
    oracle = expected_result(fixture_case)
    closed_ball = oracle["closed_ball"]
    if not isinstance(closed_ball, dict):
        raise AssertionError("invalid fixture oracle")
    fixture_radius = Fraction(
        int(squared_radius["numerator"]),
        int(squared_radius["denominator"]),
    )
    if closed_ball["squared_radius"] != level_record(fixture_radius):
        raise AssertionError(f"{path} radius is not the fixture top-k cutoff")
    for field in ("closed_rank", "exterior_ids", "interior_ids", "shell_ids"):
        if closed_ball[field] != expected[field]:
            raise AssertionError(f"{path} expected.{field} disagrees with Fraction")
    outside_shell = sorted(set(closed_ball["shell_ids"]) - set(candidate_ids))
    if outside_shell != expected.get("outside_candidate_shell_ids"):
        raise AssertionError(f"{path} does not preserve its outside-candidate shell")
    if expected.get("verdict") != "unsupported_degeneracy":
        raise AssertionError(f"{path} weakens the RelevantGP verdict")
    if not isinstance(closed_ball["closed_rank"], int) or closed_ball["closed_rank"] <= s_max:
        raise AssertionError(f"{path} no longer exercises closed rank above s_max")
    return fixture_case, value


def targeted_cases(fixture_case: dict[str, object]) -> tuple[dict[str, object], ...]:
    rational_shell = make_case(
        case_id=1,
        source_points=(
            point_words(0, 0, 0),
            point_words(1, 0, 0),
            point_words(Fraction(1, 2), 0, 0),
            point_words(0, 1, 0),
        ),
        query_components=(3, 2, 0, 6),
        requested_rank=2,
        run_m_star=0,
        exclusion_ids=(),
        label="nondyadic-rational-shell",
    )
    six_axis_tie = make_case(
        case_id=2,
        source_points=(
            point_words(-1, 0, 0),
            point_words(1, 0, 0),
            point_words(0, -1, 0),
            point_words(0, 1, 0),
            point_words(0, 0, -1),
            point_words(0, 0, 1),
        ),
        query_components=(0, 0, 0, 1),
        requested_rank=3,
        run_m_star=0,
        exclusion_ids=(),
        label="six-way-cutoff-tie",
    )
    extreme_words = make_case(
        case_id=4,
        source_points=(
            ("ffefffffffffffff", "0000000000000000", "0000000000000000"),
            ("8000000000000001", "0000000000000000", "0000000000000000"),
            ("8000000000000000", "3ff0000000000000", "0000000000000000"),
            ("0000000000000001", "0000000000000000", "0000000000000000"),
            ("7fefffffffffffff", "0000000000000000", "0000000000000000"),
        ),
        query_components=(5, -3, 0, 15),
        requested_rank=3,
        run_m_star=0,
        exclusion_ids=(),
        label="subnormal-signed-zero-max-finite",
    )
    excluded_global_interior = make_case(
        case_id=5,
        source_points=tuple(point_words(x, 0, 0) for x in (-2, -1, 0, 1, 2)),
        query_components=(0, 0, 0, 1),
        requested_rank=1,
        run_m_star=3,
        exclusion_ids=(2,),
        label="excluded-point-remains-in-global-ball",
    )
    maximum_rank_shell = make_case(
        case_id=6,
        source_points=(
            point_words(0, 0, 0),
            point_words(-1, 0, 0),
            point_words(1, 0, 0),
            point_words(-2, 0, 0),
            point_words(2, 0, 0),
            point_words(-3, 0, 0),
            point_words(3, 0, 0),
            point_words(-4, 0, 0),
            point_words(4, 0, 0),
            point_words(-5, 0, 0),
            point_words(5, 0, 0),
            point_words(0, 5, 0),
        ),
        query_components=(0, 0, 0, 1),
        requested_rank=11,
        run_m_star=0,
        exclusion_ids=(),
        label="s-max-eleven-nontrivial-cutoff-shell",
    )
    return (
        rational_shell,
        six_axis_tie,
        fixture_case,
        extreme_words,
        excluded_global_interior,
        maximum_rank_shell,
    )


def campaign_source_points(size: int) -> tuple[PointWords, ...]:
    canonical = []
    for index in range(size):
        x = index - size // 2
        words = point_words(x, 0, 0)
        if x == 0 and size % 17 == 0:
            words = ("8000000000000000", words[1], words[2])
        canonical.append(words)
    offset = (size * 37 + 11) % size
    source = canonical[offset:] + canonical[:offset]
    if size % 2:
        source.reverse()
    return tuple(source)


def campaign_exclusions(size: int, count: int) -> tuple[int, ...]:
    selected: list[int] = []
    candidate = (size * 17 + 5) % size
    while len(selected) < count:
        if candidate not in selected:
            selected.append(candidate)
        candidate = (candidate + 31) % size
        if candidate in selected:
            candidate = (candidate + 1) % size
    if size % 2:
        selected.reverse()
    return tuple(selected)


def campaign_case(size: int) -> dict[str, object]:
    maximum_run_m_star = min(9, max(0, size - 2))
    exclusion_count = min(maximum_run_m_star, size % 10)
    exclusions = campaign_exclusions(size, exclusion_count)
    eligible_count = size - exclusion_count
    requested_rank = min(eligible_count, ((size * 7) % 10) + 1)
    run_m_star = exclusion_count + (
        (size // 3) % (maximum_run_m_star - exclusion_count + 1)
    )
    denominators = (2, 3, 5, 7, 11)
    denominator = denominators[size % len(denominators)]
    query = (
        (size % 7) - 3,
        0,
        0,
        denominator,
    )
    return make_case(
        case_id=CAMPAIGN_CASE_ID_BASE + size,
        source_points=campaign_source_points(size),
        query_components=query,
        requested_rank=requested_rank,
        run_m_star=run_m_star,
        exclusion_ids=exclusions,
        label=f"deterministic-size-{size}",
    )


def all_cases(fixture_case: dict[str, object]):
    yield from targeted_cases(fixture_case)
    for size in range(CAMPAIGN_MIN_SIZE, CAMPAIGN_MAX_SIZE + 1):
        yield campaign_case(size)


def run_differential(
    executable: Path,
    fixture_case: dict[str, object],
    timeout_seconds: int,
) -> None:
    payload_lines = [PROTOCOL_HEADER]
    payload_lines.extend(case_protocol_line(case) for case in all_cases(fixture_case))
    payload_lines.append("end")
    payload = "\n".join(payload_lines) + "\n"
    completed = subprocess.run(
        [str(executable)],
        input=payload,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout_seconds,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "the C++ spatial dump failed with status "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("the C++ spatial dump wrote unexpected diagnostics")

    output_lines = completed.stdout.splitlines()
    cases = all_cases(fixture_case)
    checked_sizes: set[int] = set()
    case_count = 0
    for case_count, case in enumerate(cases, start=1):
        if case_count > len(output_lines):
            raise AssertionError(
                f"the C++ dump omitted case {case['case_id']} ({case['label']})"
            )
        line = output_lines[case_count - 1]
        actual = strict_json_object(line)
        if line != canonical_json(actual):
            raise AssertionError(
                f"case {case['case_id']} output is not canonical JSON"
            )
        expected = expected_result(case)
        difference = first_difference(expected, actual)
        if difference is not None:
            raise AssertionError(
                f"case {case['case_id']} ({case['label']}) differs: {difference}"
            )
        case_id = case["case_id"]
        if isinstance(case_id, int) and case_id > CAMPAIGN_CASE_ID_BASE:
            checked_sizes.add(case_id - CAMPAIGN_CASE_ID_BASE)

    if len(output_lines) != case_count:
        raise AssertionError(
            f"the C++ dump emitted {len(output_lines)} lines for {case_count} cases"
        )
    expected_sizes = set(range(CAMPAIGN_MIN_SIZE, CAMPAIGN_MAX_SIZE + 1))
    if checked_sizes != expected_sizes:
        missing = sorted(expected_sizes - checked_sizes)
        raise AssertionError(f"the deterministic campaign missed sizes: {missing}")
    expected_case_count = len(targeted_cases(fixture_case)) + len(expected_sizes)
    if case_count != expected_case_count:
        raise AssertionError(
            f"expected {expected_case_count} differential cases, got {case_count}"
        )

    print(
        "spatial brute-force differential: "
        f"{case_count} exact cases passed; all sizes "
        f"{CAMPAIGN_MIN_SIZE}..{CAMPAIGN_MAX_SIZE} covered"
    )


def parse_arguments() -> argparse.Namespace:
    default_fixture = (
        Path(__file__).resolve().parents[1]
        / "fixtures"
        / "spatial"
        / "relevant_gp_extra_shell_above_smax.json"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="spatial_query_dump executable")
    parser.add_argument(
        "--fixture",
        type=Path,
        default=default_fixture,
        help="RelevantGP extra-shell regression fixture",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="subprocess timeout in seconds",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout <= 0:
        raise ValueError("--timeout must be positive")
    fixture_case, _ = load_relevant_gp_fixture(arguments.fixture)
    run_differential(arguments.executable, fixture_case, arguments.timeout)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check_spatial_brute_force: {error}", file=sys.stderr)
        raise SystemExit(2) from error
