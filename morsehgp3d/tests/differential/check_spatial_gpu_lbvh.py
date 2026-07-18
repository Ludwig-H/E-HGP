#!/usr/bin/env python3
"""Bounded Fraction audit of the resident CUDA Morton-LBVH queries."""

from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from types import ModuleType


PROTOCOL_HEADER = "morsehgp3d-spatial-gpu-lbvh-v1"
RESULT_SCHEMA = "morsehgp3d.spatial_gpu_lbvh.v2"
SUMMARY_SCHEMA = "morsehgp3d.phase4.spatial_gpu_lbvh_differential.v2"
DEFAULT_TIMEOUT_SECONDS = 300
CHUNK_CASE_LIMIT = 128
PROTOCOL_MAXIMUM_POINT_COUNT = 4096
PROTOCOL_MAXIMUM_LINE_BYTES = 1 << 20
FULL_CAMPAIGN_SIZES = tuple(range(1, 1001))
QUICK_CAMPAIGN_SIZES = (1, 2, 3, 4, 17, 257, 1000)
TARGETED_CASE_COUNT = 13
TARGETED_GPU_LAUNCH_COUNT = 19
TARGETED_INPUT_POINT_COUNT = 50
SIGN_BIT = 1 << 63
EXPONENT_MASK = 0x7FF << 52
MAXIMUM_FINITE_WORD = "7fefffffffffffff"
NEGATIVE_MAXIMUM_FINITE_WORD = "ffefffffffffffff"
MINIMUM_SUBNORMAL_WORD = "0000000000000001"
NEGATIVE_MINIMUM_SUBNORMAL_WORD = "8000000000000001"
ZERO_WORD = "0000000000000000"
AUDIT_KEYS = {
    "buffer_epoch",
    "candidate_point_count",
    "certified_pruned_eligible_point_count",
    "certified_pruned_point_count",
    "certified_pruned_subtree_count",
    "cover_antichain_complete",
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "cpu_exact_prune_recertification_count",
    "cpu_exact_recertification_complete",
    "cpu_exact_seed_distance_evaluation_count",
    "cutoff_enclosure",
    "cutoff_lower_bits",
    "cutoff_upper_bits",
    "decision_semantics",
    "exact_partition_complete",
    "excluded_candidate_point_count",
    "gpu_candidate_leaf_count",
    "gpu_kernel_launch_count",
    "gpu_launch_count",
    "gpu_output_capacity",
    "gpu_output_cover_record_count",
    "gpu_parallel_round_count",
    "gpu_peak_frontier_count",
    "gpu_prune_proposal_count",
    "gpu_processed_node_count",
    "gpu_traversal_round_count",
    "internal_node_expansion_count",
    "minimum_certified_strict_margin",
    "point_partition_complete",
    "proposal_digest_fnv1a",
    "proposal_semantics",
    "query_enclosure",
    "query_lower_bits",
    "query_upper_bits",
    "resident_node_count",
    "resident_point_count",
    "top_k_seed_squared_cutoff",
    "traversed_node_count",
    "unsupported_range_fallback_count",
}
REQUIRED_COVERAGE = {
    "addition_only_overflow",
    "cutoff_non_binary64",
    "cutoff_outside_binary64",
    "exact_tie",
    "exclusions",
    "maximum_finite",
    "negative_query_outside_binary64",
    "permuted_input",
    "query_non_binary64",
    "query_outside_binary64",
    "signed_subnormal",
    "singleton",
    "six_way_shell",
    "tri_partition",
}

PointWords = tuple[str, str, str]
QueryComponents = tuple[int, int, int, int]


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def load_lbvh_oracle() -> ModuleType:
    path = Path(__file__).with_name("check_spatial_lbvh.py")
    specification = importlib.util.spec_from_file_location(
        "morsehgp3d_spatial_lbvh_oracle", path
    )
    if specification is None or specification.loader is None:
        raise AssertionError(f"cannot load the Fraction oracle from {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def finite_fraction_from_word(word: str) -> Fraction:
    if len(word) != 16 or word.lower() != word:
        raise AssertionError(f"noncanonical binary64 word: {word!r}")
    try:
        bits = int(word, 16)
    except ValueError as error:
        raise AssertionError(f"invalid binary64 word: {word!r}") from error
    if bits & EXPONENT_MASK == EXPONENT_MASK:
        raise AssertionError("a generated point coordinate is non-finite")
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def make_case(
    oracle: ModuleType,
    *,
    case_id: int,
    source_points: tuple[PointWords, ...],
    query: QueryComponents,
    squared_radius: Fraction | int,
    requested_rank: int,
    run_m_star: int = 0,
    exclusion_ids: tuple[int, ...] = (),
    label: str,
    coverage: set[str],
) -> dict[str, object]:
    case = oracle.make_case(
        case_id=case_id,
        source_points=source_points,
        query_components=query,
        requested_rank=requested_rank,
        run_m_star=run_m_star,
        exclusion_ids=exclusion_ids,
        label=label,
    )
    radius = Fraction(squared_radius)
    if radius < 0:
        raise AssertionError("a generated squared radius cannot be negative")
    case["coverage"] = frozenset(coverage)
    case["squared_radius"] = radius
    return case


def all_cases(oracle: ModuleType) -> tuple[dict[str, object], ...]:
    point_words = oracle.point_words
    maximum_finite = finite_fraction_from_word(MAXIMUM_FINITE_WORD)
    minimum_subnormal = finite_fraction_from_word(MINIMUM_SUBNORMAL_WORD)
    overflow_coordinate_word = "5fe7dddf6b095ff1"
    overflow_coordinate = finite_fraction_from_word(overflow_coordinate_word)

    singleton_query = (1, -2, 3, 7)
    singleton_point = (Fraction(3), Fraction(-2), Fraction(5))
    singleton_query_point = tuple(
        Fraction(value, singleton_query[3]) for value in singleton_query[:3]
    )
    singleton_radius = sum(
        (
            (singleton_point[axis] - singleton_query_point[axis]) ** 2
            for axis in range(3)
        ),
        Fraction(),
    )
    singleton = make_case(
        oracle,
        case_id=50_001,
        source_points=(point_words(*singleton_point),),
        query=singleton_query,
        squared_radius=singleton_radius,
        requested_rank=1,
        label="singleton-rational-query-exact-shell",
        coverage={"singleton", "query_non_binary64"},
    )

    tri_points = (
        point_words(0, 0, 0),
        point_words(1, 0, 0),
        point_words(2, 0, 0),
    )
    tri_partition = make_case(
        oracle,
        case_id=50_002,
        source_points=tri_points,
        query=(0, 0, 0, 1),
        squared_radius=1,
        requested_rank=2,
        label="interior-shell-exterior-partition",
        coverage={"tri_partition"},
    )

    six_way_shell = make_case(
        oracle,
        case_id=50_003,
        source_points=tuple(
            point_words(x, y, z)
            for x, y, z in (
                (0, 0, 0),
                (-1, 0, 0),
                (1, 0, 0),
                (0, -1, 0),
                (0, 1, 0),
                (0, 0, -1),
                (0, 0, 1),
                (3, 3, 3),
            )
        ),
        query=(0, 0, 0, 1),
        squared_radius=1,
        requested_rank=4,
        label="six-way-equal-distance-shell",
        coverage={"exact_tie", "six_way_shell"},
    )

    tie_denominator = 1 << 53
    tie_radius = Fraction(1, 1 << 106)
    projected_tie = make_case(
        oracle,
        case_id=50_004,
        source_points=(
            ("3ff0000000000000", ZERO_WORD, ZERO_WORD),
            ("3ff0000000000001", ZERO_WORD, ZERO_WORD),
        ),
        query=(tie_denominator + 1, 0, 0, tie_denominator),
        squared_radius=tie_radius,
        requested_rank=1,
        label="exact-tie-split-by-binary64-projection",
        coverage={"exact_tie", "query_non_binary64"},
    )

    rational = make_case(
        oracle,
        case_id=50_005,
        source_points=tuple(
            point_words(x, y, z)
            for x, y, z in (
                (-2, -1, 0),
                (-1, 1, 2),
                (0, 0, 0),
                (1, -1, 1),
                (2, 2, -1),
                (4, -3, 2),
            )
        ),
        query=(1, -2, 3, 7),
        squared_radius=Fraction(1, 3),
        requested_rank=3,
        label="non-binary64-query-and-radius",
        coverage={"cutoff_non_binary64", "query_non_binary64"},
    )

    extrema = make_case(
        oracle,
        case_id=50_006,
        source_points=(
            (NEGATIVE_MAXIMUM_FINITE_WORD, ZERO_WORD, ZERO_WORD),
            (NEGATIVE_MINIMUM_SUBNORMAL_WORD, ZERO_WORD, ZERO_WORD),
            (ZERO_WORD, "8000000000000000", ZERO_WORD),
            (MINIMUM_SUBNORMAL_WORD, ZERO_WORD, ZERO_WORD),
            (MAXIMUM_FINITE_WORD, ZERO_WORD, ZERO_WORD),
        ),
        query=(0, 0, 0, 1),
        squared_radius=minimum_subnormal * minimum_subnormal,
        requested_rank=2,
        label="signed-subnormal-zero-and-maximum-finite",
        coverage={"maximum_finite", "signed_subnormal"},
    )

    overflow_points = (
        point_words(0, 0, 0),
        (overflow_coordinate_word, overflow_coordinate_word, ZERO_WORD),
        (MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD, MAXIMUM_FINITE_WORD),
    )
    addition_only_overflow = make_case(
        oracle,
        case_id=50_007,
        source_points=overflow_points,
        query=(0, 0, 0, 1),
        squared_radius=1,
        requested_rank=1,
        label="finite-squares-overflow-only-on-addition",
        coverage={"addition_only_overflow", "maximum_finite"},
    )
    unsupported_cutoff = make_case(
        oracle,
        case_id=50_008,
        source_points=overflow_points,
        query=(0, 0, 0, 1),
        squared_radius=2 * overflow_coordinate * overflow_coordinate,
        requested_rank=1,
        label="squared-radius-outside-finite-binary64",
        coverage={"cutoff_outside_binary64", "maximum_finite"},
    )

    outside_points = tuple(point_words(x, 0, 0) for x in (-1, 0, 1))
    outside_query = make_case(
        oracle,
        case_id=50_009,
        source_points=outside_points,
        query=(1 << 1024, 0, 0, 1),
        squared_radius=0,
        requested_rank=1,
        label="positive-query-outside-finite-binary64",
        coverage={"query_outside_binary64"},
    )
    negative_outside_query = make_case(
        oracle,
        case_id=50_010,
        source_points=outside_points,
        query=(-(1 << 1024), 0, 0, 1),
        squared_radius=0,
        requested_rank=1,
        label="negative-query-outside-finite-binary64",
        coverage={
            "negative_query_outside_binary64",
            "query_outside_binary64",
        },
    )

    maximum_query = make_case(
        oracle,
        case_id=50_011,
        source_points=(
            point_words(0, 0, 0),
            (MAXIMUM_FINITE_WORD, ZERO_WORD, ZERO_WORD),
            (MAXIMUM_FINITE_WORD, NEGATIVE_MAXIMUM_FINITE_WORD, ZERO_WORD),
        ),
        query=(
            maximum_finite.numerator,
            -maximum_finite.numerator,
            0,
            maximum_finite.denominator,
        ),
        squared_radius=0,
        requested_rank=1,
        label="maximum-finite-exact-query",
        coverage={"maximum_finite"},
    )

    excluded = make_case(
        oracle,
        case_id=50_012,
        source_points=tuple(
            point_words(x, (x * x) % 5 - 2, (3 * x) % 7 - 3)
            for x in range(-3, 4)
        ),
        query=(2, -1, 3, 5),
        squared_radius=Fraction(9, 4),
        requested_rank=3,
        run_m_star=3,
        exclusion_ids=(1, 5),
        label="top-k-exclusions-and-global-closed-ball",
        coverage={"exclusions", "query_non_binary64"},
    )

    permuted = make_case(
        oracle,
        case_id=50_013,
        source_points=tuple(reversed(tri_points)),
        query=(0, 0, 0, 1),
        squared_radius=1,
        requested_rank=2,
        label="permuted-replay-of-tri-partition",
        coverage={"permuted_input", "tri_partition"},
    )
    return (
        singleton,
        tri_partition,
        six_way_shell,
        projected_tie,
        rational,
        extrema,
        addition_only_overflow,
        unsupported_cutoff,
        outside_query,
        negative_outside_query,
        maximum_query,
        excluded,
        permuted,
    )


def campaign_case(oracle: ModuleType, size: int) -> dict[str, object]:
    if size not in FULL_CAMPAIGN_SIZES:
        raise AssertionError("a GPU-LBVH campaign size is outside 1..1000")
    case = oracle.campaign_case(size)
    expected = oracle.expected_scientific_result(case)
    top_k = expected.get("top_k")
    if not isinstance(top_k, dict):
        raise AssertionError("the LBVH oracle omitted the campaign top-k result")
    cutoff_record = top_k.get("cutoff_squared_distance")
    if not isinstance(cutoff_record, dict) or set(cutoff_record) != {
        "denominator",
        "numerator",
    }:
        raise AssertionError("the LBVH oracle emitted an invalid campaign cutoff")
    numerator = cutoff_record["numerator"]
    denominator = cutoff_record["denominator"]
    if not isinstance(numerator, str) or not isinstance(denominator, str):
        raise AssertionError("the campaign cutoff fields must be decimal strings")
    squared_radius = Fraction(int(numerator), int(denominator))
    if squared_radius < 0 or {
        "denominator": str(squared_radius.denominator),
        "numerator": str(squared_radius.numerator),
    } != cutoff_record:
        raise AssertionError("the campaign cutoff is not a canonical exact level")
    case["campaign_size"] = size
    case["coverage"] = frozenset()
    case["squared_radius"] = squared_radius
    case["expected_top_k"] = top_k
    return case


def scoped_cases(
    oracle: ModuleType, *, quick: bool
):
    yield from all_cases(oracle)
    sizes = QUICK_CAMPAIGN_SIZES if quick else FULL_CAMPAIGN_SIZES
    for size in sizes:
        yield campaign_case(oracle, size)


def case_protocol_line(case: dict[str, object]) -> str:
    source_points = case["source_points"]
    exclusion_ids = case["exclusion_ids"]
    query = case["query"]
    radius = case["squared_radius"]
    if not isinstance(source_points, tuple) or not isinstance(exclusion_ids, tuple):
        raise AssertionError("invalid generated point or exclusion payload")
    if not isinstance(query, tuple) or not isinstance(radius, Fraction):
        raise AssertionError("invalid generated rational payload")
    tokens = [
        "case",
        str(case["case_id"]),
        str(len(source_points)),
        str(case["requested_rank"]),
        str(case["run_m_star"]),
        str(len(exclusion_ids)),
        *(str(component) for component in query),
        str(radius.numerator),
        str(radius.denominator),
        *(str(point_id) for point_id in exclusion_ids),
    ]
    for words in source_points:
        tokens.extend(words)
    return " ".join(tokens)


def expected_scientific_result(
    case: dict[str, object], oracle: ModuleType
) -> dict[str, object]:
    top_k = case.get("expected_top_k")
    if top_k is None:
        top_k = oracle.expected_scientific_result(case)["top_k"]
    if not isinstance(top_k, dict):
        raise AssertionError("invalid cached top-k oracle result")
    points = oracle.canonicalize_points(case["source_points"])
    query = oracle.query_point(case["query"])
    radius = case["squared_radius"]
    if not isinstance(radius, Fraction):
        raise AssertionError("invalid internal squared radius")
    distances = [oracle.squared_distance(point, query) for point in points]
    interior_ids = [
        point_id for point_id, distance in enumerate(distances) if distance < radius
    ]
    shell_ids = [
        point_id for point_id, distance in enumerate(distances) if distance == radius
    ]
    exterior_ids = [
        point_id for point_id, distance in enumerate(distances) if distance > radius
    ]
    return {
        "closed_ball": {
            "closed_rank": len(interior_ids) + len(shell_ids),
            "evaluation_count": len(points),
            "exterior_ids": exterior_ids,
            "interior_ids": interior_ids,
            "partition_complete": True,
            "shell_ids": shell_ids,
            "squared_radius": oracle.level_record(radius),
        },
        "top_k": top_k,
    }


def top_k_seed_cutoff(case: dict[str, object], oracle: ModuleType) -> Fraction:
    points = oracle.canonicalize_points(case["source_points"])
    query = oracle.query_point(case["query"])
    exclusions = set(case["exclusion_ids"])
    requested_rank = case["requested_rank"]
    if not isinstance(requested_rank, int):
        raise AssertionError("invalid internal requested rank")
    seed_distances: list[Fraction] = []
    for point_id, point in enumerate(points):
        if point_id in exclusions:
            continue
        seed_distances.append(oracle.squared_distance(point, query))
        if len(seed_distances) == requested_rank:
            return max(seed_distances)
    raise AssertionError("the generated case has too few eligible seed points")


def bits_word(value: object, label: str) -> str:
    if not isinstance(value, str) or len(value) != 16 or value.lower() != value:
        raise AssertionError(f"{label} must be a lowercase 16-digit hex word")
    try:
        int(value, 16)
    except ValueError as error:
        raise AssertionError(f"{label} is not hexadecimal") from error
    return value


def expected_enclosure(value: Fraction) -> tuple[str, str, str]:
    maximum_finite = finite_fraction_from_word(MAXIMUM_FINITE_WORD)
    if abs(value) > maximum_finite:
        clamped = (
            NEGATIVE_MAXIMUM_FINITE_WORD if value < 0 else MAXIMUM_FINITE_WORD
        )
        return ("unsupported_range", clamped, clamped)
    rounded = float(value)
    rounded_fraction = Fraction.from_float(rounded)

    def canonical_word(encoded: float) -> str:
        if encoded == 0.0:
            return ZERO_WORD
        return struct.pack(">d", encoded).hex()

    if rounded_fraction == value:
        word = canonical_word(rounded)
        return ("exact", word, word)
    if rounded_fraction < value:
        lower = rounded
        upper = math.nextafter(rounded, math.inf)
    else:
        lower = math.nextafter(rounded, -math.inf)
        upper = rounded
    return ("enclosed", canonical_word(lower), canonical_word(upper))


def require_nonnegative_integer(value: object, label: str) -> int:
    if type(value) is not int or value < 0:
        raise AssertionError(f"{label} must be a nonnegative JSON integer")
    return value


def require_boolean(value: object, label: str) -> bool:
    if type(value) is not bool:
        raise AssertionError(f"{label} must be a JSON boolean")
    return value


def exact_level(value: object, label: str) -> Fraction:
    if not isinstance(value, dict) or set(value) != {"denominator", "numerator"}:
        raise AssertionError(f"{label} is not an exact-level record")
    numerator = value["numerator"]
    denominator = value["denominator"]
    if not isinstance(numerator, str) or not isinstance(denominator, str):
        raise AssertionError(f"{label} fields must be strings")
    if not numerator.isdecimal() or not denominator.isdecimal() or denominator == "0":
        raise AssertionError(f"{label} has invalid nonnegative integer fields")
    if (len(numerator) > 1 and numerator[0] == "0") or (
        len(denominator) > 1 and denominator[0] == "0"
    ):
        raise AssertionError(f"{label} is not canonical")
    result = Fraction(int(numerator), int(denominator))
    canonical = {
        "denominator": str(result.denominator),
        "numerator": str(result.numerator),
    }
    if value != canonical:
        raise AssertionError(f"{label} is not reduced")
    return result


def validate_scalar_enclosure(
    *,
    exact: Fraction,
    status: object,
    lower: object,
    upper: object,
    label: str,
) -> str:
    expected_status, expected_lower, expected_upper = expected_enclosure(exact)
    lower_word = bits_word(lower, f"{label}.lower_bits")
    upper_word = bits_word(upper, f"{label}.upper_bits")
    if status != expected_status:
        raise AssertionError(
            f"{label} status is {status!r}, expected {expected_status!r}"
        )
    if lower_word != expected_lower or upper_word != expected_upper:
        raise AssertionError(f"{label} directed binary64 enclosure is wrong")
    return expected_status


def validate_audit(
    value: object,
    *,
    case: dict[str, object],
    oracle: ModuleType,
    cutoff: Fraction,
    kind: str,
    expected_epoch: int,
) -> set[str]:
    if not isinstance(value, dict) or set(value) != AUDIT_KEYS:
        raise AssertionError(f"{kind} audit has unexpected fields")
    audit = value
    source_points = case["source_points"]
    exclusions = case["exclusion_ids"]
    query_components = case["query"]
    if not isinstance(source_points, tuple) or not isinstance(exclusions, tuple):
        raise AssertionError("invalid internal case cardinalities")
    if not isinstance(query_components, tuple):
        raise AssertionError("invalid internal query")
    point_count = len(source_points)
    node_count = 2 * point_count - 1
    eligible_count = point_count - len(exclusions)
    query = oracle.query_point(query_components)

    if audit["proposal_semantics"] != (
        "gpu_resident_parallel_frontier_lbvh_strict_exterior_cover"
    ):
        raise AssertionError(f"{kind} proposal semantics changed")
    if audit["decision_semantics"] != "cpu_exact_cover_and_leaf_recertification":
        raise AssertionError(f"{kind} decision semantics changed")
    for key in (
        "cover_antichain_complete",
        "point_partition_complete",
        "cpu_exact_recertification_complete",
        "exact_partition_complete",
    ):
        if not require_boolean(audit[key], f"{kind}.{key}"):
            raise AssertionError(f"{kind}.{key} is incomplete")

    non_count_keys = {
        "cover_antichain_complete",
        "cpu_exact_recertification_complete",
        "cutoff_enclosure",
        "cutoff_lower_bits",
        "cutoff_upper_bits",
        "decision_semantics",
        "exact_partition_complete",
        "minimum_certified_strict_margin",
        "point_partition_complete",
        "proposal_digest_fnv1a",
        "proposal_semantics",
        "query_enclosure",
        "query_lower_bits",
        "query_upper_bits",
        "top_k_seed_squared_cutoff",
    }
    counts = {
        key: require_nonnegative_integer(audit[key], f"{kind}.{key}")
        for key in AUDIT_KEYS - non_count_keys
    }
    if counts["resident_point_count"] != point_count:
        raise AssertionError(f"{kind} resident point count is wrong")
    if counts["resident_node_count"] != node_count:
        raise AssertionError(f"{kind} resident tree is not full binary")
    if counts["gpu_output_capacity"] != node_count:
        raise AssertionError(f"{kind} GPU cover capacity is not node-bounded")
    if (
        counts["candidate_point_count"]
        + counts["certified_pruned_point_count"]
        != point_count
    ):
        raise AssertionError(f"{kind} cover does not partition every point")

    query_statuses = audit["query_enclosure"]
    query_lowers = audit["query_lower_bits"]
    query_uppers = audit["query_upper_bits"]
    if not all(
        isinstance(array, list) and len(array) == 3
        for array in (query_statuses, query_lowers, query_uppers)
    ):
        raise AssertionError(f"{kind} query enclosures must have three entries")
    enclosure_coverage: set[str] = set()
    for axis in range(3):
        enclosure_coverage.add(
            validate_scalar_enclosure(
                exact=query[axis],
                status=query_statuses[axis],
                lower=query_lowers[axis],
                upper=query_uppers[axis],
                label=f"{kind}.query[{axis}]",
            )
        )
    enclosure_coverage.add(
        validate_scalar_enclosure(
            exact=cutoff,
            status=audit["cutoff_enclosure"],
            lower=audit["cutoff_lower_bits"],
            upper=audit["cutoff_upper_bits"],
            label=f"{kind}.cutoff",
        )
    )
    supported = "unsupported_range" not in enclosure_coverage
    expected_launch_count = 1 if supported else 0
    if counts["gpu_launch_count"] != expected_launch_count:
        raise AssertionError(f"{kind} GPU launch accounting is wrong")
    if counts["buffer_epoch"] != expected_epoch:
        raise AssertionError(f"{kind} buffer epoch is wrong")
    expected_fallback_count = 0 if supported else point_count
    if counts["unsupported_range_fallback_count"] != expected_fallback_count:
        raise AssertionError(f"{kind} unsupported fallback accounting is wrong")

    digest = bits_word(audit["proposal_digest_fnv1a"], f"{kind}.digest")
    margin = audit["minimum_certified_strict_margin"]
    if supported:
        if counts["gpu_kernel_launch_count"] == 0:
            raise AssertionError(f"{kind} launched no traversal kernel")
        if (
            counts["gpu_kernel_launch_count"]
            != counts["gpu_traversal_round_count"]
        ):
            raise AssertionError(f"{kind} kernel/round accounting is wrong")
        if counts["gpu_processed_node_count"] != counts["traversed_node_count"]:
            raise AssertionError(f"{kind} processed/traversed nodes differ")
        if not (
            1
            <= counts["gpu_traversal_round_count"]
            <= counts["gpu_processed_node_count"]
        ):
            raise AssertionError(f"{kind} traversal round count is impossible")
        if not (
            1
            <= counts["gpu_peak_frontier_count"]
            <= counts["gpu_processed_node_count"]
        ):
            raise AssertionError(f"{kind} peak frontier count is impossible")
        if (
            counts["gpu_parallel_round_count"]
            > counts["gpu_traversal_round_count"]
        ):
            raise AssertionError(f"{kind} parallel round count is impossible")
        if (counts["gpu_peak_frontier_count"] > 1) != (
            counts["gpu_parallel_round_count"] > 0
        ):
            raise AssertionError(f"{kind} parallel frontier accounting is wrong")
        if counts["gpu_output_cover_record_count"] == 0:
            raise AssertionError(f"{kind} launched an empty cover")
        if counts["gpu_candidate_leaf_count"] != counts["candidate_point_count"]:
            raise AssertionError(f"{kind} candidate leaf accounting is wrong")
        if (
            counts["gpu_candidate_leaf_count"]
            + counts["gpu_prune_proposal_count"]
            != counts["gpu_output_cover_record_count"]
        ):
            raise AssertionError(f"{kind} terminal cover counters do not close")
        if not (
            counts["gpu_prune_proposal_count"]
            == counts["cpu_exact_prune_recertification_count"]
            == counts["certified_pruned_subtree_count"]
        ):
            raise AssertionError(f"{kind} prune recertification does not close")
        if (
            counts["traversed_node_count"]
            != 1 + 2 * counts["internal_node_expansion_count"]
        ):
            raise AssertionError(f"{kind} traversal is not a full-tree prefix")
        if (
            counts["cpu_exact_aabb_bound_evaluation_count"]
            != counts["traversed_node_count"]
        ):
            raise AssertionError(f"{kind} exact AABB accounting is wrong")
        if digest == "0000000000000000":
            raise AssertionError(f"{kind} launched cover has a null digest")
        if counts["certified_pruned_subtree_count"]:
            if margin is None or exact_level(margin, f"{kind}.margin") <= 0:
                raise AssertionError(f"{kind} certified prune lacks a strict margin")
        elif margin is not None:
            raise AssertionError(f"{kind} has a margin without a certified prune")
    else:
        zero_keys = {
            "buffer_epoch",
            "certified_pruned_point_count",
            "certified_pruned_subtree_count",
            "cpu_exact_aabb_bound_evaluation_count",
            "cpu_exact_prune_recertification_count",
            "gpu_candidate_leaf_count",
            "gpu_kernel_launch_count",
            "gpu_launch_count",
            "gpu_output_cover_record_count",
            "gpu_parallel_round_count",
            "gpu_peak_frontier_count",
            "gpu_prune_proposal_count",
            "gpu_processed_node_count",
            "gpu_traversal_round_count",
            "internal_node_expansion_count",
            "traversed_node_count",
        }
        if any(counts[key] != 0 for key in zero_keys):
            raise AssertionError(f"{kind} unsupported fallback published GPU work")
        if counts["candidate_point_count"] != point_count:
            raise AssertionError(f"{kind} unsupported fallback lost candidates")
        if digest != ZERO_WORD or margin is not None:
            raise AssertionError(f"{kind} unsupported fallback published a certificate")

    if kind == "closed_ball":
        if counts["cpu_exact_candidate_distance_evaluation_count"] != counts[
            "candidate_point_count"
        ]:
            raise AssertionError("closed_ball candidate evaluation count is wrong")
        for key in (
            "certified_pruned_eligible_point_count",
            "cpu_exact_seed_distance_evaluation_count",
            "excluded_candidate_point_count",
        ):
            if counts[key] != 0:
                raise AssertionError(f"closed_ball unexpectedly populated {key}")
        if audit["top_k_seed_squared_cutoff"] is not None:
            raise AssertionError("closed_ball published a top-k seed cutoff")
    elif kind == "top_k":
        requested_rank = case["requested_rank"]
        if not isinstance(requested_rank, int):
            raise AssertionError("invalid internal requested rank")
        if counts["cpu_exact_seed_distance_evaluation_count"] != requested_rank:
            raise AssertionError("top_k seed evaluation count is wrong")
        if (
            counts["cpu_exact_candidate_distance_evaluation_count"]
            != counts["candidate_point_count"]
            - counts["excluded_candidate_point_count"]
        ):
            raise AssertionError("top_k candidate evaluation count is wrong")
        if (
            counts["cpu_exact_candidate_distance_evaluation_count"]
            + counts["certified_pruned_eligible_point_count"]
            != eligible_count
        ):
            raise AssertionError("top_k eligible point accounting is wrong")
        if (
            counts["certified_pruned_eligible_point_count"]
            + len(exclusions)
            - counts["excluded_candidate_point_count"]
            != counts["certified_pruned_point_count"]
        ):
            raise AssertionError("top_k pruned exclusion accounting is wrong")
        seed_value = audit["top_k_seed_squared_cutoff"]
        if seed_value is None or exact_level(seed_value, "top_k.seed") != cutoff:
            raise AssertionError("top_k exact seed cutoff is wrong")
    else:
        raise AssertionError(f"unknown audit kind {kind!r}")
    return enclosure_coverage


def validate_case_result(
    actual: dict[str, object], case: dict[str, object], oracle: ModuleType
) -> tuple[set[str], int]:
    expected_keys = {
        "case_id",
        "closed_ball",
        "closed_ball_audit",
        "schema",
        "top_k",
        "top_k_audit",
    }
    if set(actual) != expected_keys:
        raise AssertionError("replay result has unexpected fields")
    if actual["schema"] != RESULT_SCHEMA or actual["case_id"] != case["case_id"]:
        raise AssertionError("replay result identity changed")
    expected = expected_scientific_result(case, oracle)
    scientific = {
        "closed_ball": actual["closed_ball"],
        "top_k": actual["top_k"],
    }
    if scientific != expected:
        difference = oracle.first_difference(expected, scientific)
        raise AssertionError(f"Fraction scientific mismatch: {difference}")

    seed_cutoff = top_k_seed_cutoff(case, oracle)
    query = oracle.query_point(case["query"])
    top_supported = all(
        expected_enclosure(value)[0] != "unsupported_range"
        for value in (*query, seed_cutoff)
    )
    radius = case["squared_radius"]
    if not isinstance(radius, Fraction):
        raise AssertionError("invalid internal radius")
    ball_supported = all(
        expected_enclosure(value)[0] != "unsupported_range"
        for value in (*query, radius)
    )
    top_epoch = 1 if top_supported else 0
    ball_epoch = (1 if top_supported else 0) + 1 if ball_supported else 0
    enclosure_coverage = validate_audit(
        actual["top_k_audit"],
        case=case,
        oracle=oracle,
        cutoff=seed_cutoff,
        kind="top_k",
        expected_epoch=top_epoch,
    )
    enclosure_coverage.update(
        validate_audit(
            actual["closed_ball_audit"],
            case=case,
            oracle=oracle,
            cutoff=radius,
            kind="closed_ball",
            expected_epoch=ball_epoch,
        )
    )
    prune_count = 0
    for audit_key in ("top_k_audit", "closed_ball_audit"):
        audit = actual[audit_key]
        if not isinstance(audit, dict):
            raise AssertionError(f"{audit_key} is not an object")
        count = audit["certified_pruned_subtree_count"]
        if type(count) is not int:
            raise AssertionError(f"{audit_key} prune count is not integral")
        prune_count += count
    return enclosure_coverage, prune_count


def write_summary(path: Path, value: dict[str, object]) -> None:
    if not path.is_absolute():
        raise ValueError("--summary-json must be an absolute path")
    parent = path.parent.resolve(strict=True)
    target = parent / path.name
    if not path.name or path.name in {".", ".."}:
        raise ValueError("--summary-json has an invalid file name")
    if target.exists() or target.is_symlink():
        raise ValueError(f"refusing to replace existing summary: {target}")

    encoded = (canonical_json(value) + "\n").encode("utf-8")
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{target.name}.", suffix=".partial", dir=parent
    )
    temporary = Path(temporary_name)
    try:
        offset = 0
        while offset < len(encoded):
            offset += os.write(descriptor, encoded[offset:])
        os.fsync(descriptor)
        os.close(descriptor)
        descriptor = -1
        os.link(temporary, target, follow_symlinks=False)
        os.unlink(temporary)
        directory = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        temporary.unlink(missing_ok=True)


def run_differential(
    executable: Path, timeout_seconds: int, *, quick: bool = False
) -> dict[str, object]:
    deadline = time.monotonic() + timeout_seconds

    def remaining_seconds() -> float:
        remaining = deadline - time.monotonic()
        if remaining <= 0.0:
            raise subprocess.TimeoutExpired(str(executable), timeout_seconds)
        return remaining

    oracle = load_lbvh_oracle()
    expected_sizes = QUICK_CAMPAIGN_SIZES if quick else FULL_CAMPAIGN_SIZES
    expected_case_count = TARGETED_CASE_COUNT + len(expected_sizes)
    expected_gpu_launch_count = (
        TARGETED_GPU_LAUNCH_COUNT + 2 * len(expected_sizes)
    )
    expected_chunk_count = (
        expected_case_count + CHUNK_CASE_LIMIT - 1
    ) // CHUNK_CASE_LIMIT
    expected_input_point_count = TARGETED_INPUT_POINT_COUNT + sum(expected_sizes)

    coverage: set[str] = set()
    enclosure_coverage: set[str] = set()
    checked_sizes: set[int] = set()
    certified_prune_count = 0
    gpu_kernel_launch_count = 0
    gpu_launch_count = 0
    gpu_parallel_round_count = 0
    gpu_peak_frontier_count = 0
    gpu_processed_node_count = 0
    gpu_traversal_round_count = 0
    case_count = 0
    chunk_count = 0
    input_point_count = 0
    maximum_point_count = 0
    deterministic_pair: dict[int, dict[str, object]] = {}
    input_hasher = hashlib.sha256(
        b"morsehgp3d.phase4.spatial_gpu_lbvh.ordered_cases.v1\n"
    )
    result_hasher = hashlib.sha256(
        b"morsehgp3d.phase4.spatial_gpu_lbvh.ordered_results.v1\n"
    )

    cases = iter(scoped_cases(oracle, quick=quick))
    while True:
        chunk: list[dict[str, object]] = []
        while len(chunk) < CHUNK_CASE_LIMIT:
            remaining_seconds()
            try:
                case = next(cases)
            except StopIteration:
                break
            remaining_seconds()
            chunk.append(case)
        if not chunk:
            break
        if len(chunk) > CHUNK_CASE_LIMIT:
            raise AssertionError("a GPU-LBVH chunk exceeds its case bound")
        chunk_count += 1

        protocol_lines: list[str] = []
        for case in chunk:
            source_points = case["source_points"]
            if not isinstance(source_points, tuple):
                raise AssertionError("a generated case has invalid source points")
            point_count = len(source_points)
            if not 1 <= point_count <= PROTOCOL_MAXIMUM_POINT_COUNT:
                raise AssertionError("a case exceeds the bounded point protocol")
            protocol_line = case_protocol_line(case)
            if len(protocol_line.encode("utf-8")) > PROTOCOL_MAXIMUM_LINE_BYTES:
                raise AssertionError("a case exceeds the bounded line protocol")
            protocol_lines.append(protocol_line)
            input_hasher.update(protocol_line.encode("utf-8"))
            input_hasher.update(b"\n")
            input_point_count += point_count
            maximum_point_count = max(maximum_point_count, point_count)

        payload = "\n".join(
            [PROTOCOL_HEADER, *protocol_lines, "end", ""]
        )
        completed = subprocess.run(
            [str(executable)],
            input=payload,
            text=True,
            capture_output=True,
            timeout=remaining_seconds(),
            check=False,
        )
        if completed.returncode != 0:
            raise AssertionError(
                f"GPU-LBVH replay chunk {chunk_count} failed with "
                f"{completed.returncode}: {completed.stderr.strip()}"
            )
        if completed.stderr:
            raise AssertionError(
                f"GPU-LBVH replay chunk {chunk_count} wrote stderr: "
                f"{completed.stderr!r}"
            )
        output_lines = completed.stdout.splitlines()
        if len(output_lines) != len(chunk):
            raise AssertionError(
                f"expected {len(chunk)} GPU-LBVH results in chunk "
                f"{chunk_count}, got {len(output_lines)}"
            )

        for line, case in zip(output_lines, chunk, strict=True):
            remaining_seconds()
            actual = oracle.strict_json_object(line)
            if line != oracle.canonical_json(actual):
                raise AssertionError(
                    f"case {case['case_id']} output is not canonical JSON"
                )
            try:
                case_enclosures, case_prunes = validate_case_result(
                    actual, case, oracle
                )
            except AssertionError as error:
                raise AssertionError(
                    f"case {case['case_id']} ({case['label']}): {error}"
                ) from error
            remaining_seconds()

            result_hasher.update(line.encode("utf-8"))
            result_hasher.update(b"\n")
            coverage.update(case["coverage"])
            enclosure_coverage.update(case_enclosures)
            certified_prune_count += case_prunes
            case_count += 1

            campaign_size = case.get("campaign_size")
            if campaign_size is not None:
                if type(campaign_size) is not int or campaign_size not in expected_sizes:
                    raise AssertionError("a campaign case has an unexpected size")
                if campaign_size in checked_sizes:
                    raise AssertionError("a campaign size was replayed twice")
                checked_sizes.add(campaign_size)

            case_id = case["case_id"]
            if case_id in (50_002, 50_013):
                if not isinstance(case_id, int) or case_id in deterministic_pair:
                    raise AssertionError("the deterministic pair is not unique")
                deterministic_pair[case_id] = actual

            for audit_key in ("top_k_audit", "closed_ball_audit"):
                audit = actual[audit_key]
                if not isinstance(audit, dict):
                    raise AssertionError(f"{audit_key} is not an object")
                launches = audit["gpu_launch_count"]
                if type(launches) is not int or launches not in (0, 1):
                    raise AssertionError(
                        f"{audit_key} has an invalid logical launch count"
                    )
                gpu_launch_count += launches
                gpu_kernel_launch_count += require_nonnegative_integer(
                    audit["gpu_kernel_launch_count"],
                    f"{audit_key}.gpu_kernel_launch_count",
                )
                gpu_parallel_round_count += require_nonnegative_integer(
                    audit["gpu_parallel_round_count"],
                    f"{audit_key}.gpu_parallel_round_count",
                )
                gpu_peak_frontier_count = max(
                    gpu_peak_frontier_count,
                    require_nonnegative_integer(
                        audit["gpu_peak_frontier_count"],
                        f"{audit_key}.gpu_peak_frontier_count",
                    ),
                )
                gpu_processed_node_count += require_nonnegative_integer(
                    audit["gpu_processed_node_count"],
                    f"{audit_key}.gpu_processed_node_count",
                )
                gpu_traversal_round_count += require_nonnegative_integer(
                    audit["gpu_traversal_round_count"],
                    f"{audit_key}.gpu_traversal_round_count",
                )

    expected_size_set = set(expected_sizes)
    if case_count != expected_case_count:
        raise AssertionError(
            f"expected {expected_case_count} cases, got {case_count}"
        )
    if checked_sizes != expected_size_set:
        missing = sorted(expected_size_set - checked_sizes)
        unexpected = sorted(checked_sizes - expected_size_set)
        raise AssertionError(
            f"campaign sizes differ: missing={missing}, unexpected={unexpected}"
        )
    if chunk_count != expected_chunk_count:
        raise AssertionError(
            f"expected {expected_chunk_count} chunks, got {chunk_count}"
        )
    if input_point_count != expected_input_point_count:
        raise AssertionError(
            f"expected {expected_input_point_count} cumulative input points, "
            f"got {input_point_count}"
        )
    if maximum_point_count != max(expected_sizes):
        raise AssertionError("the campaign maximum point count is wrong")
    if coverage != REQUIRED_COVERAGE:
        raise AssertionError(f"targeted coverage differs: {sorted(coverage)}")
    if enclosure_coverage != {"enclosed", "exact", "unsupported_range"}:
        raise AssertionError(
            f"directed-enclosure coverage differs: {sorted(enclosure_coverage)}"
        )
    if certified_prune_count == 0:
        raise AssertionError("the bounded campaign certified no strict subtree prune")
    if gpu_launch_count != expected_gpu_launch_count:
        raise AssertionError(
            f"expected {expected_gpu_launch_count} logical GPU traversals, "
            f"got {gpu_launch_count}"
        )
    if gpu_kernel_launch_count != gpu_traversal_round_count:
        raise AssertionError("aggregate kernel/round accounting differs")

    first = deterministic_pair.get(50_002)
    repeated = deterministic_pair.get(50_013)
    if first is None or repeated is None:
        raise AssertionError("the deterministic permutation pair is incomplete")
    for key in ("closed_ball", "closed_ball_audit", "top_k", "top_k_audit"):
        if first[key] != repeated[key]:
            raise AssertionError(f"permuted input changed deterministic {key}")

    scope = "quick" if quick else "full"
    print(
        "spatial CUDA resident LBVH differential: "
        f"{case_count} bounded Fraction cases passed; scope={scope}, "
        f"chunks={chunk_count}, sizes={len(checked_sizes)}, "
        f"logical_traversals={gpu_launch_count}, "
        f"certified_subtree_prunes={certified_prune_count}"
    )
    return {
        "all_cases_passed": True,
        "bounded_protocol": True,
        "campaign_complete": not quick,
        "campaign_input_sha256": input_hasher.hexdigest(),
        "campaign_result_sha256": result_hasher.hexdigest(),
        "campaign_sizes_checked": sorted(checked_sizes),
        "case_count": case_count,
        "certified_pruned_subtree_count": certified_prune_count,
        "chunk_case_limit": CHUNK_CASE_LIMIT,
        "chunk_count": chunk_count,
        "closed_ball_query_count": case_count,
        "cover_antichain_complete": True,
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_cover_and_leaf_recertification",
        "directed_enclosure_coverage": sorted(enclosure_coverage),
        "exact_partition_complete": True,
        "gpu_kernel_launch_count": gpu_kernel_launch_count,
        "gpu_launch_count": gpu_launch_count,
        "gpu_parallel_round_count": gpu_parallel_round_count,
        "gpu_peak_frontier_count": gpu_peak_frontier_count,
        "gpu_processed_node_count": gpu_processed_node_count,
        "gpu_traversal_round_count": gpu_traversal_round_count,
        "input_point_count": input_point_count,
        "maximum_point_count": maximum_point_count,
        "point_partition_complete": True,
        "proposal_semantics": (
            "gpu_resident_parallel_frontier_lbvh_strict_exterior_cover"
        ),
        "schema": SUMMARY_SCHEMA,
        "scope": scope,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "targeted_case_count": TARGETED_CASE_COUNT,
        "targeted_coverage": sorted(coverage),
        "top_k_query_count": case_count,
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT_SECONDS)
    parser.add_argument("--summary-json", type=Path)
    parser.add_argument(
        "--quick",
        action="store_true",
        help="run the bounded sanitizer subset instead of every size 1..1000",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.timeout <= 0:
        raise ValueError("--timeout must be positive")
    summary = run_differential(
        arguments.executable, arguments.timeout, quick=arguments.quick
    )
    if arguments.summary_json is not None:
        write_summary(arguments.summary_json, summary)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check_spatial_gpu_lbvh: {error}", file=sys.stderr)
        raise SystemExit(2) from error
