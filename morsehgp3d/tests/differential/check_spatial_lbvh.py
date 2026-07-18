#!/usr/bin/env python3
"""Independent Fraction audit of exact Morton-LBVH spatial queries."""

from __future__ import annotations

import argparse
import heapq
import json
import struct
import subprocess
import sys
from fractions import Fraction
from pathlib import Path


PROTOCOL_HEADER = "morsehgp3d-spatial-lbvh-query-v1"
RESULT_SCHEMA = "morsehgp3d.spatial_lbvh_query_dump.v1"
UINT64_MASK = (1 << 64) - 1
NEGATIVE_ZERO_BITS = 1 << 63
EXPONENT_MASK = 0x7FF << 52
DEFAULT_TIMEOUT_SECONDS = 300
CAMPAIGN_MIN_SIZE = 1
CAMPAIGN_MAX_SIZE = 1000
CAMPAIGN_CASE_ID_BASE = 20_000

PointWords = tuple[str, str, str]
ExactPoint = tuple[Fraction, Fraction, Fraction]
QueryComponents = tuple[int, int, int, int]

BUILD_COUNTER_KEYS = {
    "maximum_depth",
    "maximum_morton_collision_size",
    "morton_collision_group_count",
    "node_count",
    "point_count",
}
COUNTER_KEYS = {
    "bulk_interior_point_count",
    "bulk_interior_subtree_count",
    "exact_aabb_bound_evaluation_count",
    "exact_point_distance_evaluation_count",
    "excluded_point_count",
    "internal_node_expansion_count",
    "method",
    "minimum_strict_pruning_margin",
    "node_visit_count",
    "pruned_eligible_point_count",
    "pruned_subtree_count",
}
TOP_K_KEYS = {
    "canonical_choice_ids",
    "counters",
    "cutoff_shell_ids",
    "cutoff_squared_distance",
    "distance_evaluation_count",
    "eligible_point_count",
    "requested_rank",
    "shell_complete",
    "strict_below",
}
CLOSED_BALL_KEYS = {
    "closed_rank",
    "counters",
    "distance_evaluation_count",
    "evaluation_count",
    "exterior_ids",
    "interior_ids",
    "partition_complete",
    "shell_ids",
    "squared_radius",
}


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
) -> QueryComponents:
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
        raise AssertionError("the LBVH campaign contains a non-finite coordinate")
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
    value = struct.unpack(">d", bytes.fromhex(canonical_word(word)))[0]
    return Fraction.from_float(value)


def point_words(x: Fraction | int, y: Fraction | int, z: Fraction | int) -> PointWords:
    return (binary64_word(x), binary64_word(y), binary64_word(z))


def exact_point(words: PointWords) -> ExactPoint:
    return (
        fraction_from_word(words[0]),
        fraction_from_word(words[1]),
        fraction_from_word(words[2]),
    )


def canonicalize_points(source_points: tuple[PointWords, ...]) -> list[ExactPoint]:
    candidates: list[tuple[tuple[int, int, int], PointWords]] = []
    for source_words in source_points:
        words = (
            canonical_word(source_words[0]),
            canonical_word(source_words[1]),
            canonical_word(source_words[2]),
        )
        candidates.append(
            (
                (
                    total_order_key(words[0]),
                    total_order_key(words[1]),
                    total_order_key(words[2]),
                ),
                words,
            )
        )
    candidates.sort(key=lambda candidate: candidate[0])
    for index in range(1, len(candidates)):
        if candidates[index - 1][1] == candidates[index][1]:
            raise AssertionError("the Python campaign generated a duplicate point")
    return [exact_point(words) for _, words in candidates]


def squared_distance(left: ExactPoint, right: ExactPoint) -> Fraction:
    return sum(
        ((left[axis] - right[axis]) ** 2 for axis in range(3)),
        Fraction(),
    )


def level_record(value: Fraction) -> dict[str, str]:
    if value < 0:
        raise AssertionError("an exact squared quantity cannot be negative")
    return {
        "denominator": str(value.denominator),
        "numerator": str(value.numerator),
    }


def query_point(query_components: QueryComponents) -> ExactPoint:
    x_numerator, y_numerator, z_numerator, denominator = query_components
    return (
        Fraction(x_numerator, denominator),
        Fraction(y_numerator, denominator),
        Fraction(z_numerator, denominator),
    )


def make_case(
    case_id: int,
    source_points: tuple[PointWords, ...],
    query_components: QueryComponents,
    requested_rank: int,
    run_m_star: int,
    exclusion_ids: tuple[int, ...],
    label: str,
) -> dict[str, object]:
    query = normalized_query_components(*query_components)
    if not source_points:
        raise AssertionError("a differential case must contain at least one point")
    canonicalize_points(source_points)
    point_count_bound = min(9, max(0, len(source_points) - 2))
    if not 0 <= run_m_star <= point_count_bound:
        raise AssertionError(
            "run_m_star exceeds the depth possible for the generated cloud"
        )
    if len(set(exclusion_ids)) != len(exclusion_ids):
        raise AssertionError("a generated exclusion set repeats a PointId")
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
        "query": query,
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


def expected_scientific_result(case: dict[str, object]) -> dict[str, object]:
    source_points = case["source_points"]
    exclusion_ids_value = case["exclusion_ids"]
    query_components = case["query"]
    requested_rank = case["requested_rank"]
    if not isinstance(source_points, tuple):
        raise AssertionError("invalid source points")
    if not isinstance(exclusion_ids_value, tuple):
        raise AssertionError("invalid exclusions")
    if not isinstance(query_components, tuple):
        raise AssertionError("invalid query")
    if not isinstance(requested_rank, int):
        raise AssertionError("invalid rank")

    points = canonicalize_points(source_points)
    query = query_point(query_components)
    exclusion_set = set(exclusion_ids_value)
    distances = [squared_distance(point, query) for point in points]
    eligible = [
        (distance, point_id)
        for point_id, distance in enumerate(distances)
        if point_id not in exclusion_set
    ]
    cutoff = heapq.nsmallest(requested_rank, eligible)[-1][0]
    strict = sorted(entry for entry in eligible if entry[0] < cutoff)
    cutoff_shell_ids = [
        point_id for distance, point_id in eligible if distance == cutoff
    ]
    shell_choice_count = requested_rank - len(strict)
    canonical_choice_ids = sorted(
        [point_id for _, point_id in strict]
        + cutoff_shell_ids[:shell_choice_count]
    )

    interior_ids = [
        point_id
        for point_id, distance in enumerate(distances)
        if distance < cutoff
    ]
    shell_ids = [
        point_id
        for point_id, distance in enumerate(distances)
        if distance == cutoff
    ]
    exterior_ids = [
        point_id
        for point_id, distance in enumerate(distances)
        if distance > cutoff
    ]
    return {
        "closed_ball": {
            "closed_rank": len(interior_ids) + len(shell_ids),
            "evaluation_count": len(points),
            "exterior_ids": exterior_ids,
            "interior_ids": interior_ids,
            "partition_complete": True,
            "shell_ids": shell_ids,
            "squared_radius": level_record(cutoff),
        },
        "top_k": {
            "canonical_choice_ids": canonical_choice_ids,
            "cutoff_shell_ids": cutoff_shell_ids,
            "cutoff_squared_distance": level_record(cutoff),
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


def first_difference(expected: object, actual: object, path: str = "$") -> str | None:
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


def target_cases() -> tuple[dict[str, object], ...]:
    singleton = make_case(
        case_id=1,
        source_points=(point_words(3, -2, 5),),
        query_components=(1, -2, 3, 7),
        requested_rank=1,
        run_m_star=0,
        exclusion_ids=(),
        label="singleton",
    )
    three_dimensional_shell = make_case(
        case_id=2,
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
                (-2, -2, -2),
                (2, 2, 2),
            )
        ),
        query_components=(0, 0, 0, 1),
        requested_rank=4,
        run_m_star=0,
        exclusion_ids=(),
        label="three-dimensional-six-way-shell",
    )
    excluded_global_ball = make_case(
        case_id=3,
        source_points=tuple(
            point_words(x, y, z)
            for x, y, z in (
                (-3, -1, 2),
                (-2, 2, -1),
                (-1, 0, 1),
                (0, 0, 0),
                (1, 1, 0),
                (2, -2, 1),
                (3, 1, -2),
            )
        ),
        query_components=(1, -1, 2, 5),
        requested_rank=3,
        run_m_star=4,
        exclusion_ids=(1, 3),
        label="excluded-points-remain-in-global-ball",
    )
    extreme_binary64 = make_case(
        case_id=4,
        source_points=(
            ("ffefffffffffffff", "0000000000000001", "3ff0000000000000"),
            ("8000000000000001", "7fefffffffffffff", "bff0000000000000"),
            ("8000000000000000", "0000000000000000", "0000000000000001"),
            ("0000000000000001", "8000000000000001", "7fefffffffffffff"),
            ("7fefffffffffffff", "ffefffffffffffff", "0000000000000000"),
        ),
        query_components=(5, -3, 7, 15),
        requested_rank=3,
        run_m_star=1,
        exclusion_ids=(2,),
        label="subnormal-signed-zero-and-max-finite-3d",
    )
    cluster_points = tuple(
        point_words(
            -400 + 17 * index,
            ((index * 29) % 97) - 48,
            ((index * index * 11 + index) % 89) - 44,
        )
        for index in range(48)
    )
    separated_clusters = make_case(
        case_id=5,
        source_points=cluster_points,
        query_components=(-401, 7, -5, 3),
        requested_rank=5,
        run_m_star=3,
        exclusion_ids=(0, 7, 31),
        label="separated-3d-clusters-force-pruning",
    )
    scale = 1 << 30
    tiny = Fraction(1, 1 << 40)
    morton_collision = make_case(
        case_id=6,
        source_points=(
            point_words(-scale, -scale, -scale),
            point_words(scale, scale, scale),
            *(point_words(index * tiny, (index * index) * tiny, -index * tiny)
              for index in range(1, 17)),
        ),
        query_components=(1, -2, 3, 11),
        requested_rank=7,
        run_m_star=0,
        exclusion_ids=(),
        label="large-span-morton-collision-cluster",
    )
    return (
        singleton,
        three_dimensional_shell,
        excluded_global_ball,
        extreme_binary64,
        separated_clusters,
        morton_collision,
    )


def campaign_source_points(size: int) -> tuple[PointWords, ...]:
    canonical: list[PointWords] = []
    tetrahedral_yz = ((0, 0), (1, 1), (-1, 2), (2, -1))
    for index in range(size):
        x = index - size // 2
        if index < len(tetrahedral_yz):
            y, z = tetrahedral_yz[index]
        else:
            y = ((index * 37 + size * 11) % (2 * size + 17)) - size
            z = (
                (index * index * 13 + index * 7 + size * 5)
                % (3 * size + 23)
            ) - size
        words = point_words(x, y, z)
        if index == 0 and size % 17 == 0:
            words = (words[0], "8000000000000000", words[2])
        canonical.append(words)

    if size >= 4:
        points = [exact_point(words) for words in canonical[:4]]
        vectors = [
            tuple(points[row][axis] - points[0][axis] for axis in range(3))
            for row in range(1, 4)
        ]
        determinant = (
            vectors[0][0]
            * (vectors[1][1] * vectors[2][2] - vectors[1][2] * vectors[2][1])
            - vectors[0][1]
            * (vectors[1][0] * vectors[2][2] - vectors[1][2] * vectors[2][0])
            + vectors[0][2]
            * (vectors[1][0] * vectors[2][1] - vectors[1][1] * vectors[2][0])
        )
        if determinant == 0:
            raise AssertionError("the deterministic campaign lost its 3D tetrahedron")

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
    requested_rank = min(eligible_count, ((size * 7) % 11) + 1)
    run_m_star = exclusion_count + (
        (size // 3) % (maximum_run_m_star - exclusion_count + 1)
    )
    denominators = (2, 3, 5, 7, 11, 13)
    denominator = denominators[size % len(denominators)]
    query = (
        (size % 11) - 5,
        ((size * 3) % 13) - 6,
        ((size * 5) % 17) - 8,
        denominator,
    )
    return make_case(
        case_id=CAMPAIGN_CASE_ID_BASE + size,
        source_points=campaign_source_points(size),
        query_components=query,
        requested_rank=requested_rank,
        run_m_star=run_m_star,
        exclusion_ids=exclusions,
        label=f"deterministic-3d-size-{size}",
    )


def all_cases():
    yield from target_cases()
    for size in range(CAMPAIGN_MIN_SIZE, CAMPAIGN_MAX_SIZE + 1):
        yield campaign_case(size)


def require_object(value: object, label: str) -> dict[str, object]:
    if not isinstance(value, dict):
        raise AssertionError(f"{label} must be a JSON object")
    return value


def require_nonnegative_integer(value: object, label: str) -> int:
    if type(value) is not int or value < 0:
        raise AssertionError(f"{label} must be a nonnegative JSON integer")
    return value


def exact_level_from_record(value: object, label: str) -> Fraction:
    record = require_object(value, label)
    if record.keys() != {"denominator", "numerator"}:
        raise AssertionError(f"{label} has invalid exact-level keys")
    denominator = record["denominator"]
    numerator = record["numerator"]
    if not isinstance(denominator, str) or not isinstance(numerator, str):
        raise AssertionError(f"{label} exact-level fields must be strings")
    if not denominator.isdecimal() or denominator == "0":
        raise AssertionError(f"{label} has an invalid denominator")
    if len(denominator) > 1 and denominator[0] == "0":
        raise AssertionError(f"{label} has a noncanonical denominator")
    numerator_digits = numerator[1:] if numerator.startswith("-") else numerator
    if not numerator_digits.isdecimal():
        raise AssertionError(f"{label} has an invalid numerator")
    if len(numerator_digits) > 1 and numerator_digits[0] == "0":
        raise AssertionError(f"{label} has a noncanonical numerator")
    if numerator == "-0":
        raise AssertionError(f"{label} has a negative zero numerator")
    fraction = Fraction(int(numerator), int(denominator))
    if level_record(fraction) != record:
        raise AssertionError(f"{label} exact level is not reduced")
    return fraction


def validate_build_counters(value: object, point_count: int) -> int:
    counters = require_object(value, "build_counters")
    if counters.keys() != BUILD_COUNTER_KEYS:
        raise AssertionError("build_counters has unexpected fields")
    parsed = {
        key: require_nonnegative_integer(value, f"build_counters.{key}")
        for key, value in counters.items()
    }
    if parsed["point_count"] != point_count:
        raise AssertionError("LBVH build point_count disagrees with its case")
    expected_node_count = 2 * point_count - 1
    if parsed["node_count"] != expected_node_count:
        raise AssertionError("LBVH build is not a full binary tree")
    maximum_depth = parsed["maximum_depth"]
    if point_count == 1:
        if maximum_depth != 0:
            raise AssertionError("a singleton LBVH must have depth zero")
    elif not 1 <= maximum_depth <= point_count - 1:
        raise AssertionError("LBVH maximum_depth is outside full-tree bounds")
    group_count = parsed["morton_collision_group_count"]
    maximum_collision = parsed["maximum_morton_collision_size"]
    if group_count == 0:
        if maximum_collision != 0:
            raise AssertionError("collision maximum exists without a collision group")
    elif not 2 <= maximum_collision <= point_count:
        raise AssertionError("invalid maximum Morton collision size")
    if group_count > point_count // 2:
        raise AssertionError("too many disjoint Morton collision groups")
    return expected_node_count


def parsed_query_counters(value: object, label: str) -> dict[str, object]:
    counters = require_object(value, label)
    if counters.keys() != COUNTER_KEYS:
        raise AssertionError(f"{label} has unexpected fields")
    if counters["method"] != "morton_lbvh":
        raise AssertionError(f"{label}.method does not identify Morton LBVH")
    for key in COUNTER_KEYS - {"method", "minimum_strict_pruning_margin"}:
        require_nonnegative_integer(counters[key], f"{label}.{key}")
    margin = counters["minimum_strict_pruning_margin"]
    if margin is not None and exact_level_from_record(
        margin, f"{label}.minimum_strict_pruning_margin"
    ) <= 0:
        raise AssertionError(f"{label} has a non-strict pruning margin")
    return counters


def validate_top_k_counters(
    top_k: dict[str, object],
    eligible_count: int,
    exclusion_count: int,
    node_count: int,
    requested_rank: int,
    label: str,
) -> None:
    counters = parsed_query_counters(top_k["counters"], f"{label}.counters")
    visits = require_nonnegative_integer(counters["node_visit_count"], label)
    expansions = require_nonnegative_integer(
        counters["internal_node_expansion_count"], label
    )
    aabb_evaluations = require_nonnegative_integer(
        counters["exact_aabb_bound_evaluation_count"], label
    )
    exact_evaluations = require_nonnegative_integer(
        counters["exact_point_distance_evaluation_count"], label
    )
    pruned_points = require_nonnegative_integer(
        counters["pruned_eligible_point_count"], label
    )
    pruned_subtrees = require_nonnegative_integer(
        counters["pruned_subtree_count"], label
    )
    if not 1 <= visits <= node_count or visits != 1 + 2 * expansions:
        raise AssertionError(f"{label} violates full-tree traversal accounting")
    if aabb_evaluations != visits:
        raise AssertionError(f"{label} must evaluate one exact lower bound per visit")
    if exact_evaluations + pruned_points != eligible_count:
        raise AssertionError(f"{label} does not account for every eligible point")
    if not requested_rank <= exact_evaluations <= eligible_count:
        raise AssertionError(f"{label} cannot certify its requested cutoff")
    if counters["excluded_point_count"] != exclusion_count:
        raise AssertionError(f"{label} exclusion counter is wrong")
    if counters["bulk_interior_point_count"] != 0:
        raise AssertionError(f"{label} unexpectedly bulk-classified top-k points")
    if counters["bulk_interior_subtree_count"] != 0:
        raise AssertionError(f"{label} unexpectedly bulk-classified top-k nodes")
    if pruned_subtrees > visits:
        raise AssertionError(f"{label} prunes more subtrees than it visits")
    margin = counters["minimum_strict_pruning_margin"]
    if (pruned_subtrees == 0) != (margin is None):
        raise AssertionError(f"{label} pruning margin presence is inconsistent")
    if top_k["distance_evaluation_count"] != exact_evaluations:
        raise AssertionError(f"{label} public distance counter is inconsistent")


def validate_closed_ball_counters(
    closed_ball: dict[str, object],
    point_count: int,
    node_count: int,
    label: str,
) -> None:
    counters = parsed_query_counters(
        closed_ball["counters"], f"{label}.counters"
    )
    visits = require_nonnegative_integer(counters["node_visit_count"], label)
    expansions = require_nonnegative_integer(
        counters["internal_node_expansion_count"], label
    )
    exterior_subtrees = require_nonnegative_integer(
        counters["pruned_subtree_count"], label
    )
    interior_subtrees = require_nonnegative_integer(
        counters["bulk_interior_subtree_count"], label
    )
    aabb_evaluations = require_nonnegative_integer(
        counters["exact_aabb_bound_evaluation_count"], label
    )
    exact_evaluations = require_nonnegative_integer(
        counters["exact_point_distance_evaluation_count"], label
    )
    exterior_points = require_nonnegative_integer(
        counters["pruned_eligible_point_count"], label
    )
    interior_points = require_nonnegative_integer(
        counters["bulk_interior_point_count"], label
    )
    if not 1 <= visits <= node_count or visits != 1 + 2 * expansions:
        raise AssertionError(f"{label} violates full-tree traversal accounting")
    if aabb_evaluations != 2 * visits - exterior_subtrees:
        raise AssertionError(f"{label} exact AABB evaluations are inconsistent")
    if exact_evaluations + exterior_points + interior_points != point_count:
        raise AssertionError(f"{label} does not account for every cloud point")
    if counters["excluded_point_count"] != 0:
        raise AssertionError(f"{label} incorrectly applies query exclusions")
    if exterior_subtrees > visits or interior_subtrees > visits:
        raise AssertionError(f"{label} bulk-classifies more nodes than it visits")
    if (exterior_subtrees == 0) != (exterior_points == 0):
        raise AssertionError(f"{label} exterior subtree/point counters disagree")
    if (interior_subtrees == 0) != (interior_points == 0):
        raise AssertionError(f"{label} interior subtree/point counters disagree")
    margin = counters["minimum_strict_pruning_margin"]
    has_bulk_decision = exterior_subtrees + interior_subtrees != 0
    if has_bulk_decision != (margin is not None):
        raise AssertionError(f"{label} strict margin presence is inconsistent")
    if closed_ball["distance_evaluation_count"] != exact_evaluations:
        raise AssertionError(f"{label} public distance counter is inconsistent")


def scientific_projection(traversal: dict[str, object]) -> dict[str, object]:
    top_k = require_object(traversal["top_k"], "top_k")
    closed_ball = require_object(traversal["closed_ball"], "closed_ball")
    return {
        "closed_ball": {
            key: value
            for key, value in closed_ball.items()
            if key not in {"counters", "distance_evaluation_count"}
        },
        "top_k": {
            key: value
            for key, value in top_k.items()
            if key not in {"counters", "distance_evaluation_count"}
        },
    }


def validate_traversal(
    value: object,
    expected: dict[str, object],
    point_count: int,
    exclusion_count: int,
    requested_rank: int,
    node_count: int,
    label: str,
) -> dict[str, object]:
    traversal = require_object(value, label)
    if traversal.keys() != {"closed_ball", "top_k"}:
        raise AssertionError(f"{label} has unexpected fields")
    top_k = require_object(traversal["top_k"], f"{label}.top_k")
    closed_ball = require_object(
        traversal["closed_ball"], f"{label}.closed_ball"
    )
    if top_k.keys() != TOP_K_KEYS:
        raise AssertionError(f"{label}.top_k has unexpected fields")
    if closed_ball.keys() != CLOSED_BALL_KEYS:
        raise AssertionError(f"{label}.closed_ball has unexpected fields")
    projection = scientific_projection(traversal)
    difference = first_difference(expected, projection)
    if difference is not None:
        raise AssertionError(f"{label} scientific partition differs: {difference}")
    eligible_count = point_count - exclusion_count
    validate_top_k_counters(
        top_k,
        eligible_count,
        exclusion_count,
        node_count,
        requested_rank,
        f"{label}.top_k",
    )
    validate_closed_ball_counters(
        closed_ball,
        point_count,
        node_count,
        f"{label}.closed_ball",
    )
    return projection


def validate_case_result(actual: dict[str, object], case: dict[str, object]) -> None:
    if actual.keys() != {
        "build_counters",
        "case_id",
        "far_first",
        "near_first",
        "schema",
    }:
        raise AssertionError("the LBVH dump has unexpected top-level fields")
    if actual["schema"] != RESULT_SCHEMA:
        raise AssertionError("the LBVH dump has an unexpected schema")
    if actual["case_id"] != case["case_id"]:
        raise AssertionError("the LBVH dump changed the case_id")
    source_points = case["source_points"]
    exclusions = case["exclusion_ids"]
    requested_rank = case["requested_rank"]
    if not isinstance(source_points, tuple) or not isinstance(exclusions, tuple):
        raise AssertionError("invalid case payload")
    if not isinstance(requested_rank, int):
        raise AssertionError("invalid case rank")
    point_count = len(source_points)
    node_count = validate_build_counters(actual["build_counters"], point_count)
    expected = expected_scientific_result(case)
    near = validate_traversal(
        actual["near_first"],
        expected,
        point_count,
        len(exclusions),
        requested_rank,
        node_count,
        "near_first",
    )
    far = validate_traversal(
        actual["far_first"],
        expected,
        point_count,
        len(exclusions),
        requested_rank,
        node_count,
        "far_first",
    )
    difference = first_difference(near, far)
    if difference is not None:
        raise AssertionError(
            f"near/far traversal changes the scientific result: {difference}"
        )


def run_differential(executable: Path, timeout_seconds: int) -> None:
    payload_lines = [PROTOCOL_HEADER]
    payload_lines.extend(case_protocol_line(case) for case in all_cases())
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
            "the C++ LBVH dump failed with status "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("the C++ LBVH dump wrote unexpected diagnostics")

    output_lines = completed.stdout.splitlines()
    cases = all_cases()
    checked_sizes: set[int] = set()
    coverage = {
        "ball_bulk_interior": False,
        "ball_bulk_exterior": False,
        "morton_collision": False,
        "traversal_counter_difference": False,
        "top_k_pruning": False,
    }
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
        try:
            validate_case_result(actual, case)
        except AssertionError as error:
            raise AssertionError(
                f"case {case['case_id']} ({case['label']}): {error}"
            ) from error
        build_counters = require_object(actual["build_counters"], "build_counters")
        coverage["morton_collision"] |= (
            build_counters["morton_collision_group_count"] != 0
        )
        near = require_object(actual["near_first"], "near_first")
        far = require_object(actual["far_first"], "far_first")
        near_top = require_object(near["top_k"], "near_first.top_k")
        far_top = require_object(far["top_k"], "far_first.top_k")
        near_ball = require_object(near["closed_ball"], "near_first.closed_ball")
        near_top_counters = require_object(
            near_top["counters"], "near_first.top_k.counters"
        )
        far_top_counters = require_object(
            far_top["counters"], "far_first.top_k.counters"
        )
        near_ball_counters = require_object(
            near_ball["counters"], "near_first.closed_ball.counters"
        )
        coverage["top_k_pruning"] |= (
            near_top_counters["pruned_subtree_count"] != 0
            or far_top_counters["pruned_subtree_count"] != 0
        )
        coverage["ball_bulk_exterior"] |= (
            near_ball_counters["pruned_subtree_count"] != 0
        )
        coverage["ball_bulk_interior"] |= (
            near_ball_counters["bulk_interior_subtree_count"] != 0
        )
        coverage["traversal_counter_difference"] |= (
            near_top_counters != far_top_counters
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
    expected_case_count = len(target_cases()) + len(expected_sizes)
    if case_count != expected_case_count:
        raise AssertionError(
            f"expected {expected_case_count} differential cases, got {case_count}"
        )
    missing_coverage = sorted(
        name for name, observed in coverage.items() if not observed
    )
    if missing_coverage:
        raise AssertionError(
            f"the LBVH campaign missed execution paths: {missing_coverage}"
        )

    print(
        "spatial Morton-LBVH differential: "
        f"{case_count} exact near/far cases passed; deterministic 3D sizes "
        f"{CAMPAIGN_MIN_SIZE}..{CAMPAIGN_MAX_SIZE} covered"
    )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "executable",
        type=Path,
        help="spatial_lbvh_query_dump executable",
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
    run_differential(arguments.executable, arguments.timeout)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check_spatial_lbvh: {error}", file=sys.stderr)
        raise SystemExit(2) from error
