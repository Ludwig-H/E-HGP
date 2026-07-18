#!/usr/bin/env python3
"""Bounded Fraction audit of the certified CUDA AABB prune filter."""

from __future__ import annotations

import argparse
from fractions import Fraction
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

PROTOCOL_HEADER = "morsehgp3d-spatial-gpu-bounds-v1"
RESULT_SCHEMA = "morsehgp3d.spatial_gpu_bounds.v1"
SUMMARY_SCHEMA = "morsehgp3d.phase4.spatial_gpu_bounds_differential.v1"
DEFAULT_TIMEOUT_SECONDS = 120
SIGN_BIT = 1 << 63
EXPONENT_MASK = 0x7FF << 52
MAXIMUM_FINITE_WORD = "7fefffffffffffff"
NEGATIVE_MAXIMUM_FINITE_WORD = "ffefffffffffffff"
MINIMUM_SUBNORMAL_WORD = "0000000000000001"
NEGATIVE_MINIMUM_SUBNORMAL_WORD = "8000000000000001"
AUDIT_KEYS = {
    "all_boxes_classified",
    "buffer_epoch",
    "certified_prune_count",
    "cpu_exact_prune_recertification_count",
    "cpu_exact_recertification_complete",
    "cutoff_enclosure",
    "cutoff_lower_bits",
    "cutoff_upper_bits",
    "decision_semantics",
    "gpu_input_box_count",
    "gpu_launch_count",
    "gpu_output_record_count",
    "gpu_prune_proposal_count",
    "gpu_unique_box_index_count",
    "gpu_unknown_proposal_count",
    "gpu_visit_proposal_count",
    "minimum_certified_strict_margin",
    "proposal_digest_fnv1a",
    "proposal_permutation_complete",
    "proposal_semantics",
    "query_enclosure",
    "query_lower_bits",
    "query_upper_bits",
    "unsupported_range_fallback_count",
}
REQUIRED_COVERAGE = {
    "cutoff_non_binary64",
    "cutoff_outside_binary64",
    "degenerate_aabb",
    "exact_equality",
    "maximum_finite",
    "mixed_batch",
    "negative_outside_binary64",
    "query_non_binary64",
    "query_outside_binary64",
    "signed_subnormal",
    "strict_prune",
    "subnormal_square",
}

Words3 = tuple[str, str, str]
BoxWords = tuple[Words3, Words3]
QueryComponents = tuple[int, int, int, int]


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def strict_json_object(text: str) -> dict[str, object]:
    def reject_constant(value: str) -> object:
        raise ValueError(f"non-finite JSON constant {value!r}")

    def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key {key!r}")
            result[key] = value
        return result

    value = json.loads(
        text,
        object_pairs_hook=unique_object,
        parse_constant=reject_constant,
    )
    if not isinstance(value, dict):
        raise ValueError("a replay result must be a JSON object")
    return value


def greatest_common_divisor(left: int, right: int) -> int:
    left = abs(left)
    right = abs(right)
    while right:
        left, right = right, left % right
    return left


def normalized_query(
    x_numerator: int,
    y_numerator: int,
    z_numerator: int,
    denominator: int,
) -> QueryComponents:
    if denominator <= 0:
        raise AssertionError("a query denominator must be positive")
    divisor = denominator
    for numerator in (x_numerator, y_numerator, z_numerator):
        divisor = greatest_common_divisor(divisor, numerator)
    return (
        x_numerator // divisor,
        y_numerator // divisor,
        z_numerator // divisor,
        denominator // divisor,
    )


def finite_fraction_from_word(word: str) -> Fraction:
    if len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 word: {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word: {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("a bounds fixture contains a non-finite coordinate")
    value = struct.unpack(">d", bytes.fromhex(word))[0]
    return Fraction.from_float(value)


def exact_binary64_word(value: Fraction | int) -> str:
    exact = Fraction(value)
    encoded = float(exact)
    if not math.isfinite(encoded) or Fraction.from_float(encoded) != exact:
        raise AssertionError(f"{exact} is not exactly representable as binary64")
    return struct.pack(">d", encoded).hex()


def point_words(x: Fraction | int, y: Fraction | int, z: Fraction | int) -> Words3:
    return (
        exact_binary64_word(x),
        exact_binary64_word(y),
        exact_binary64_word(z),
    )


def point_box(
    x: str | Fraction | int, y: str | Fraction | int, z: str | Fraction | int
) -> BoxWords:
    coordinates = tuple(
        value if isinstance(value, str) else exact_binary64_word(value)
        for value in (x, y, z)
    )
    if len(coordinates) != 3:
        raise AssertionError("a point box must have three coordinates")
    words = (coordinates[0], coordinates[1], coordinates[2])
    return (words, words)


def aabb(lower: Words3, upper: Words3) -> BoxWords:
    for axis in range(3):
        if finite_fraction_from_word(lower[axis]) > finite_fraction_from_word(
            upper[axis]
        ):
            raise AssertionError("an AABB lower endpoint exceeds its upper endpoint")
    return (lower, upper)


def query_point(components: QueryComponents) -> tuple[Fraction, Fraction, Fraction]:
    x_numerator, y_numerator, z_numerator, denominator = components
    return (
        Fraction(x_numerator, denominator),
        Fraction(y_numerator, denominator),
        Fraction(z_numerator, denominator),
    )


def exact_minimum_squared_distance(
    box: BoxWords, query: tuple[Fraction, Fraction, Fraction]
) -> Fraction:
    total = Fraction()
    lower, upper = box
    for axis in range(3):
        lower_value = finite_fraction_from_word(lower[axis])
        upper_value = finite_fraction_from_word(upper[axis])
        if query[axis] < lower_value:
            delta = lower_value - query[axis]
        elif query[axis] > upper_value:
            delta = query[axis] - upper_value
        else:
            delta = Fraction()
        total += delta * delta
    return total


def make_case(
    *,
    case_id: int,
    query: QueryComponents,
    cutoff: Fraction | int,
    boxes: tuple[BoxWords, ...],
    label: str,
    coverage: set[str],
) -> dict[str, object]:
    normalized = normalized_query(*query)
    exact_cutoff = Fraction(cutoff)
    if exact_cutoff < 0:
        raise AssertionError("a squared cutoff cannot be negative")
    if not boxes:
        raise AssertionError("a bounds case must contain at least one AABB")
    for box in boxes:
        aabb(*box)
    return {
        "boxes": boxes,
        "case_id": case_id,
        "coverage": frozenset(coverage),
        "cutoff": exact_cutoff,
        "label": label,
        "query": normalized,
    }


def all_cases() -> tuple[dict[str, object], ...]:
    zero = point_words(0, 0, 0)
    maximum_finite = finite_fraction_from_word(MAXIMUM_FINITE_WORD)
    minimum_subnormal = finite_fraction_from_word(MINIMUM_SUBNORMAL_WORD)
    square_root_of_minimum_subnormal_word = "1e60000000000000"
    mixed = make_case(
        case_id=40_001,
        query=(2, 0, 0, 1),
        cutoff=1,
        boxes=(
            point_box(0, 0, 0),
            point_box(1, 0, 0),
            point_box(2, 0, 0),
            aabb(point_words(-1, 0, 0), point_words(1, 0, 0)),
        ),
        label="strict-prune-equality-and-visit",
        coverage={"strict_prune", "exact_equality", "mixed_batch", "degenerate_aabb"},
    )
    return (
        mixed,
        make_case(
            case_id=40_002,
            query=(10, -20, 3, 30),
            cutoff=Fraction(1, 9),
            boxes=(
                point_box(0, 0, 0),
                aabb(point_words(-1, -1, -1), point_words(1, 1, 1)),
                point_box(Fraction(1, 2), Fraction(-1, 2), 0),
            ),
            label="non-binary64-rational-query-and-cutoff",
            coverage={"query_non_binary64", "cutoff_non_binary64", "mixed_batch"},
        ),
        make_case(
            case_id=40_003,
            query=(0, 0, 0, 1),
            cutoff=0,
            boxes=(
                point_box(
                    MINIMUM_SUBNORMAL_WORD, "0000000000000000", "0000000000000000"
                ),
                point_box(
                    square_root_of_minimum_subnormal_word,
                    "0000000000000000",
                    "0000000000000000",
                ),
                (zero, zero),
            ),
            label="subnormal-distance-and-square-underflow",
            coverage={
                "signed_subnormal",
                "subnormal_square",
                "exact_equality",
                "degenerate_aabb",
            },
        ),
        make_case(
            case_id=40_004,
            query=(1, -1, 0, 1 << 1100),
            cutoff=0,
            boxes=(
                (zero, zero),
                point_box(
                    MINIMUM_SUBNORMAL_WORD,
                    NEGATIVE_MINIMUM_SUBNORMAL_WORD,
                    "0000000000000000",
                ),
            ),
            label="subnormal-query-directed-enclosure",
            coverage={"query_non_binary64", "signed_subnormal", "subnormal_square"},
        ),
        make_case(
            case_id=40_005,
            query=(0, 0, 0, 1),
            cutoff=maximum_finite,
            boxes=(
                point_box(MAXIMUM_FINITE_WORD, "0000000000000000", "0000000000000000"),
                point_box(1, 0, 0),
            ),
            label="maximum-finite-box-and-overflowing-square",
            coverage={"maximum_finite", "strict_prune", "mixed_batch"},
        ),
        make_case(
            case_id=40_006,
            query=(
                maximum_finite.numerator,
                -maximum_finite.numerator,
                0,
                maximum_finite.denominator,
            ),
            cutoff=0,
            boxes=(
                point_box(
                    MAXIMUM_FINITE_WORD,
                    NEGATIVE_MAXIMUM_FINITE_WORD,
                    "0000000000000000",
                ),
                (zero, zero),
            ),
            label="maximum-finite-exact-query",
            coverage={
                "maximum_finite",
                "strict_prune",
                "exact_equality",
                "mixed_batch",
            },
        ),
        make_case(
            case_id=40_007,
            query=(1 << 1024, 0, 0, 1),
            cutoff=0,
            boxes=(
                point_box(MAXIMUM_FINITE_WORD, "0000000000000000", "0000000000000000"),
                (zero, zero),
            ),
            label="positive-query-outside-finite-binary64",
            coverage={"query_outside_binary64", "strict_prune"},
        ),
        make_case(
            case_id=40_008,
            query=(-(1 << 1024), 0, 0, 1),
            cutoff=0,
            boxes=(
                point_box(
                    NEGATIVE_MAXIMUM_FINITE_WORD, "0000000000000000", "0000000000000000"
                ),
                (zero, zero),
            ),
            label="negative-query-outside-finite-binary64",
            coverage={
                "negative_outside_binary64",
                "query_outside_binary64",
                "strict_prune",
            },
        ),
        make_case(
            case_id=40_009,
            query=(0, 0, 0, 1),
            cutoff=Fraction(1 << 2050, 1),
            boxes=(
                point_box(
                    MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD
                ),
                (zero, zero),
            ),
            label="cutoff-outside-finite-binary64",
            coverage={"cutoff_outside_binary64", "maximum_finite", "exact_equality"},
        ),
        make_case(
            case_id=40_010,
            query=(0, 0, 0, 1),
            cutoff=Fraction(1, 3),
            boxes=(
                point_box(1, 0, 0),
                point_box(Fraction(1, 2), 0, 0),
                aabb(
                    point_words(Fraction(-1, 2), Fraction(-1, 2), Fraction(-1, 2)),
                    point_words(Fraction(1, 2), Fraction(1, 2), Fraction(1, 2)),
                ),
            ),
            label="non-binary64-cutoff-enclosure",
            coverage={"cutoff_non_binary64", "strict_prune", "mixed_batch"},
        ),
        make_case(
            case_id=40_011,
            query=(0, 0, 0, 1),
            cutoff=maximum_finite,
            boxes=(
                point_box(
                    MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD
                ),
                point_box(1, 1, 1),
            ),
            label="three-axis-addition-overflow",
            coverage={"maximum_finite", "strict_prune", "mixed_batch"},
        ),
        make_case(
            case_id=40_012,
            query=(0, 0, 0, 1),
            cutoff=4,
            boxes=(
                point_box(-3, 0, 0),
                aabb(point_words(-2, -2, -2), point_words(2, 2, 2)),
                point_box(2, 0, 0),
            ),
            label="negative-degenerate-and-equality-boxes",
            coverage={
                "strict_prune",
                "exact_equality",
                "degenerate_aabb",
                "mixed_batch",
            },
        ),
        make_case(
            case_id=40_013,
            query=mixed["query"],
            cutoff=mixed["cutoff"],
            boxes=mixed["boxes"],
            label="deterministic-replay-of-first-case",
            coverage={
                "strict_prune",
                "exact_equality",
                "mixed_batch",
                "degenerate_aabb",
            },
        ),
    )


def case_protocol_line(case: dict[str, object]) -> str:
    query = case["query"]
    cutoff = case["cutoff"]
    boxes = case["boxes"]
    if not isinstance(query, tuple) or not isinstance(cutoff, Fraction):
        raise AssertionError("internal bounds case types are inconsistent")
    if not isinstance(boxes, tuple):
        raise AssertionError("internal bounds boxes are not a tuple")
    tokens = [
        "case",
        str(case["case_id"]),
        str(len(boxes)),
        *(str(component) for component in query),
        str(cutoff.numerator),
        str(cutoff.denominator),
    ]
    for lower, upper in boxes:
        tokens.extend(lower)
        tokens.extend(upper)
    return " ".join(tokens)


def bits_word(value: object, label: str) -> str:
    if not isinstance(value, str) or len(value) != 16 or value.lower() != value:
        raise AssertionError(f"{label} must be a lowercase 16-digit hex word")
    try:
        int(value, 16)
    except ValueError as error:
        raise AssertionError(f"{label} is not hexadecimal") from error
    return value


def finite_value_from_audit_word(value: object, label: str) -> Fraction:
    word = bits_word(value, label)
    bits = int(word, 16)
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError(f"{label} is non-finite")
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def expected_enclosure(value: Fraction) -> tuple[str, str | None, str | None]:
    maximum_finite = finite_fraction_from_word(MAXIMUM_FINITE_WORD)
    if abs(value) > maximum_finite:
        return ("unsupported_range", None, None)
    rounded = float(value)
    rounded_fraction = Fraction.from_float(rounded)
    if rounded_fraction == value:
        word = struct.pack(">d", rounded).hex()
        if rounded == 0.0:
            word = "0000000000000000"
        return ("exact", word, word)
    if rounded_fraction < value:
        lower = rounded
        upper = math.nextafter(rounded, math.inf)
    else:
        lower = math.nextafter(rounded, -math.inf)
        upper = rounded

    def canonical_word(encoded: float) -> str:
        if encoded == 0.0:
            return "0000000000000000"
        return struct.pack(">d", encoded).hex()

    return (
        "enclosed",
        canonical_word(lower),
        canonical_word(upper),
    )


def require_nonnegative_integer(value: object, label: str) -> int:
    if type(value) is not int or value < 0:
        raise AssertionError(f"{label} must be a nonnegative JSON integer")
    return value


def exact_level(value: object, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != {"denominator", "numerator"}:
        raise AssertionError(f"{label} is not an exact-level record")
    numerator = value["numerator"]
    denominator = value["denominator"]
    if not isinstance(numerator, str) or not isinstance(denominator, str):
        raise AssertionError(f"{label} fields must be strings")
    if not numerator.isdecimal() or not denominator.isdecimal() or denominator == "0":
        raise AssertionError(f"{label} has invalid integer fields")
    if (len(numerator) > 1 and numerator[0] == "0") or (
        len(denominator) > 1 and denominator[0] == "0"
    ):
        raise AssertionError(f"{label} is not canonical")
    result = Fraction(int(numerator), int(denominator))
    if {
        "denominator": str(result.denominator),
        "numerator": str(result.numerator),
    } != value:
        raise AssertionError(f"{label} is not reduced")
    return result


def require_boolean(value: object, label: str) -> bool:
    if type(value) is not bool:
        raise AssertionError(f"{label} must be a JSON boolean")
    return value


def validate_scalar_enclosure(
    *,
    exact: Fraction,
    status_value: object,
    lower_value: object,
    upper_value: object,
    label: str,
) -> str:
    expected_status, expected_lower, expected_upper = expected_enclosure(exact)
    if status_value != expected_status:
        raise AssertionError(
            f"{label} enclosure status is {status_value!r}, expected {expected_status!r}"
        )
    lower_word = bits_word(lower_value, f"{label}.lower_bits")
    upper_word = bits_word(upper_value, f"{label}.upper_bits")
    if expected_status != "unsupported_range":
        if lower_word != expected_lower or upper_word != expected_upper:
            raise AssertionError(f"{label} directed binary64 enclosure is wrong")
        lower = finite_value_from_audit_word(lower_word, f"{label}.lower_bits")
        upper = finite_value_from_audit_word(upper_word, f"{label}.upper_bits")
        if not lower <= exact <= upper:
            raise AssertionError(f"{label} enclosure does not contain its exact value")
    return expected_status


def validate_audit(
    value: object,
    case: dict[str, object],
    decisions: list[str],
    exact_distances: list[Fraction],
) -> set[str]:
    if not isinstance(value, dict) or set(value) != AUDIT_KEYS:
        raise AssertionError("the spatial-bounds audit has unexpected fields")
    audit = value
    box_count = len(exact_distances)
    query = query_point(case["query"])
    cutoff = case["cutoff"]
    if not isinstance(cutoff, Fraction):
        raise AssertionError("the internal cutoff type changed")
    input_has_unsupported_range = (
        any(
            expected_enclosure(coordinate)[0] == "unsupported_range"
            for coordinate in query
        )
        or expected_enclosure(cutoff)[0] == "unsupported_range"
    )
    if audit["proposal_semantics"] != "gpu_outward_interval_aabb":
        raise AssertionError("the proposal semantics changed")
    if audit["decision_semantics"] != "cpu_exact_recertified_strict_prune":
        raise AssertionError("the decision semantics changed")
    if not require_boolean(audit["all_boxes_classified"], "all_boxes_classified"):
        raise AssertionError("the result did not classify every input AABB")
    if not require_boolean(
        audit["cpu_exact_recertification_complete"],
        "cpu_exact_recertification_complete",
    ):
        raise AssertionError("the exact prune recertification is incomplete")

    count_keys = AUDIT_KEYS - {
        "all_boxes_classified",
        "cpu_exact_recertification_complete",
        "cutoff_enclosure",
        "cutoff_lower_bits",
        "cutoff_upper_bits",
        "decision_semantics",
        "minimum_certified_strict_margin",
        "proposal_digest_fnv1a",
        "proposal_permutation_complete",
        "proposal_semantics",
        "query_enclosure",
        "query_lower_bits",
        "query_upper_bits",
    }
    counts = {key: require_nonnegative_integer(audit[key], key) for key in count_keys}
    if counts["gpu_input_box_count"] != box_count:
        raise AssertionError("gpu_input_box_count disagrees with the case")
    expected_launch_count = 0 if input_has_unsupported_range else 1
    if counts["gpu_launch_count"] != expected_launch_count:
        raise AssertionError("GPU launch accounting disagrees with input support")
    proposal_total = (
        counts["gpu_prune_proposal_count"]
        + counts["gpu_visit_proposal_count"]
        + counts["gpu_unknown_proposal_count"]
    )
    if proposal_total != counts["gpu_output_record_count"]:
        raise AssertionError("GPU proposal counters do not close")
    if counts["gpu_unique_box_index_count"] != counts["gpu_output_record_count"]:
        raise AssertionError("the GPU proposal did not return unique box indices")
    permutation_complete = require_boolean(
        audit["proposal_permutation_complete"], "proposal_permutation_complete"
    )
    if counts["gpu_launch_count"] == 1:
        if counts["gpu_output_record_count"] != box_count or not permutation_complete:
            raise AssertionError("the GPU proposal permutation is incomplete")
        if counts["buffer_epoch"] != 1:
            raise AssertionError("a fresh replay context must publish epoch one")
    elif counts["gpu_output_record_count"] != 0 or permutation_complete:
        raise AssertionError("an unlaunched proposal published GPU records")

    prune_count = decisions.count("prune")
    visit_count = decisions.count("visit")
    unknown_count = decisions.count("unknown")
    if counts["certified_prune_count"] != prune_count:
        raise AssertionError("certified_prune_count disagrees with decisions")
    if (
        counts["cpu_exact_prune_recertification_count"]
        != counts["gpu_prune_proposal_count"]
    ):
        raise AssertionError("GPU prune proposals did not close exact recertification")
    if not input_has_unsupported_range:
        if (
            prune_count != counts["gpu_prune_proposal_count"]
            or visit_count != counts["gpu_visit_proposal_count"]
            or unknown_count != counts["gpu_unknown_proposal_count"]
        ):
            raise AssertionError("published decisions disagree with GPU proposals")
    elif decisions != ["unknown"] * box_count:
        raise AssertionError("an unsupported range published a terminal hint")
    strict_margins = [
        distance - cutoff
        for decision, distance in zip(decisions, exact_distances, strict=True)
        if decision == "prune"
    ]
    margin_value = audit["minimum_certified_strict_margin"]
    if strict_margins:
        if margin_value is None:
            raise AssertionError("certified prunes lack their strict margin")
        if exact_level(margin_value, "minimum_certified_strict_margin") != min(
            strict_margins
        ):
            raise AssertionError("the minimum certified strict margin is wrong")
    elif margin_value is not None:
        raise AssertionError("a strict margin exists without a certified prune")

    query_statuses = audit["query_enclosure"]
    query_lowers = audit["query_lower_bits"]
    query_uppers = audit["query_upper_bits"]
    if not all(
        isinstance(array, list) and len(array) == 3
        for array in (query_statuses, query_lowers, query_uppers)
    ):
        raise AssertionError("query enclosure arrays must have three entries")
    enclosure_coverage: set[str] = set()
    for axis in range(3):
        enclosure_coverage.add(
            validate_scalar_enclosure(
                exact=query[axis],
                status_value=query_statuses[axis],
                lower_value=query_lowers[axis],
                upper_value=query_uppers[axis],
                label=f"query[{axis}]",
            )
        )
    enclosure_coverage.add(
        validate_scalar_enclosure(
            exact=cutoff,
            status_value=audit["cutoff_enclosure"],
            lower_value=audit["cutoff_lower_bits"],
            upper_value=audit["cutoff_upper_bits"],
            label="cutoff",
        )
    )
    unsupported = "unsupported_range" in enclosure_coverage
    if unsupported != input_has_unsupported_range:
        raise AssertionError("unsupported-range enclosure detection drifted")
    expected_fallback_count = box_count if unsupported else 0
    if counts["unsupported_range_fallback_count"] != expected_fallback_count:
        raise AssertionError("unsupported-range fallback accounting is inconsistent")

    digest = bits_word(audit["proposal_digest_fnv1a"], "proposal_digest_fnv1a")
    if unsupported and digest != "0000000000000000":
        raise AssertionError("an unlaunched unsupported query published a digest")
    return enclosure_coverage


def validate_case_result(
    actual: dict[str, object], case: dict[str, object]
) -> tuple[set[str], int]:
    if set(actual) != {"audit", "case_id", "decisions", "schema"}:
        raise AssertionError("the replay result has unexpected fields")
    if actual["schema"] != RESULT_SCHEMA:
        raise AssertionError("the replay result has an unexpected schema")
    if actual["case_id"] != case["case_id"]:
        raise AssertionError("the replay changed case_id")
    decisions = actual["decisions"]
    boxes = case["boxes"]
    if not isinstance(decisions, list) or not isinstance(boxes, tuple):
        raise AssertionError("the replay decisions are not an array")
    if len(decisions) != len(boxes):
        raise AssertionError("the replay did not return one decision per AABB")
    allowed = {"prune", "unknown", "visit"}
    if any(
        type(decision) is not str or decision not in allowed for decision in decisions
    ):
        raise AssertionError("the replay returned an invalid decision")

    query = query_point(case["query"])
    cutoff = case["cutoff"]
    if not isinstance(cutoff, Fraction):
        raise AssertionError("the internal cutoff type changed")
    exact_distances = [exact_minimum_squared_distance(box, query) for box in boxes]
    for index, (decision, distance) in enumerate(
        zip(decisions, exact_distances, strict=True)
    ):
        if decision == "prune" and not distance > cutoff:
            raise AssertionError(
                f"box {index} is a false prune: D_min={distance}, cutoff={cutoff}"
            )
    enclosure_coverage = validate_audit(
        actual["audit"], case, decisions, exact_distances
    )
    return enclosure_coverage, decisions.count("prune")


def write_summary(path: Path, value: dict[str, object]) -> None:
    target = path.resolve(strict=False)
    parent = target.parent
    parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{target.name}.", suffix=".tmp", dir=parent
    )
    temporary = Path(temporary_name)
    try:
        payload = (canonical_json(value) + "\n").encode("utf-8")
        with os.fdopen(descriptor, "wb", closefd=True) as output:
            descriptor = -1
            output.write(payload)
            output.flush()
            os.fsync(output.fileno())
        os.link(temporary, target, follow_symlinks=False)
        temporary.unlink()
        directory = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        temporary.unlink(missing_ok=True)


def run_differential(executable: Path, timeout_seconds: int) -> dict[str, object]:
    cases = all_cases()
    if len(cases) > 256 or any(len(case["boxes"]) > 4096 for case in cases):
        raise AssertionError("the differential exceeds the bounded replay protocol")
    payload = "\n".join(
        [PROTOCOL_HEADER, *(case_protocol_line(case) for case in cases), "end", ""]
    )
    completed = subprocess.run(
        [str(executable)],
        input=payload,
        text=True,
        capture_output=True,
        timeout=timeout_seconds,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"GPU-bounds replay failed with {completed.returncode}: "
            f"{completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError(f"GPU-bounds replay wrote stderr: {completed.stderr!r}")
    output_lines = completed.stdout.splitlines()
    if len(output_lines) != len(cases):
        raise AssertionError(
            f"expected {len(cases)} GPU-bounds results, got {len(output_lines)}"
        )

    coverage: set[str] = set()
    enclosure_coverage: set[str] = set()
    certified_prune_count = 0
    parsed_results: list[dict[str, object]] = []
    for line, case in zip(output_lines, cases, strict=True):
        actual = strict_json_object(line)
        if line != canonical_json(actual):
            raise AssertionError(f"case {case['case_id']} output is not canonical JSON")
        try:
            case_enclosures, case_prunes = validate_case_result(actual, case)
        except AssertionError as error:
            raise AssertionError(
                f"case {case['case_id']} ({case['label']}): {error}"
            ) from error
        parsed_results.append(actual)
        coverage.update(case["coverage"])
        enclosure_coverage.update(case_enclosures)
        certified_prune_count += case_prunes

    if coverage != REQUIRED_COVERAGE:
        raise AssertionError(f"targeted coverage differs: {sorted(coverage)}")
    if enclosure_coverage != {"enclosed", "exact", "unsupported_range"}:
        raise AssertionError(
            f"directed-enclosure coverage differs: {sorted(enclosure_coverage)}"
        )
    if certified_prune_count == 0:
        raise AssertionError("the bounded campaign certified no strict prune")
    first = parsed_results[0]
    repeated = parsed_results[-1]
    if first["decisions"] != repeated["decisions"]:
        raise AssertionError("identical bounded cases changed their decisions")
    first_audit = first["audit"]
    repeated_audit = repeated["audit"]
    if not isinstance(first_audit, dict) or not isinstance(repeated_audit, dict):
        raise AssertionError("determinism audits are not objects")
    if first_audit["proposal_digest_fnv1a"] != repeated_audit["proposal_digest_fnv1a"]:
        raise AssertionError("identical bounded cases changed their proposal digest")

    print(
        "spatial CUDA AABB bounds differential: "
        f"{len(cases)} bounded Fraction cases passed; "
        f"certified_prunes={certified_prune_count}, zero false prunes, "
        "exact/enclosed/unsupported-range audits covered"
    )
    return {
        "all_cases_passed": True,
        "bounded_protocol": True,
        "case_count": len(cases),
        "certified_prune_count": certified_prune_count,
        "decision_semantics": "cpu_exact_recertified_strict_prune",
        "directed_enclosure_coverage": sorted(enclosure_coverage),
        "false_prune_count": 0,
        "proposal_semantics": "gpu_outward_interval_aabb",
        "schema": SUMMARY_SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "targeted_coverage": sorted(coverage),
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT_SECONDS)
    parser.add_argument("--summary-json", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout <= 0:
        raise ValueError("--timeout must be positive")
    summary = run_differential(arguments.executable, arguments.timeout)
    if arguments.summary_json is not None:
        write_summary(arguments.summary_json, summary)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check_spatial_gpu_bounds: {error}", file=sys.stderr)
        raise SystemExit(2) from error
