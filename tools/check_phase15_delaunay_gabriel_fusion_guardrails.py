#!/usr/bin/env python3
"""Validate the Phase 15 Delaunay/Gabriel cross-variant sidecar.

This checker deliberately separates three kinds of evidence:

* direct facet variants carry a source-event inclusion deadline upper bound;
  they do not claim to replay the full variant's first connection;
* surrogate orders carry the five-way ``before / at / late / never /
  unsupported`` partition and a corrected postcondition;
* massive reports without records expose producer commitments only.  They are
  structurally checkable, but cannot claim independent record replay.

When a bounded report emits its source, raw-tree, corrected-tree and overlay
records, the checker independently replays the binary64 closed plateaus and
recomputes every record-derived SHA-256 commitment.  This remains a
conditional binary64 guardrail, never an exact Gamma_2 or Morse certificate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
import sys
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Iterable, Mapping, Sequence

INPUT_SCHEMA = "morsehgp3d.phase15_delaunay_gabriel_fusion_guardrails.v1"
INNER_SCHEMA = "morsehgp3d.phase15_gabriel_fusion_guardrails.v1"
CHECK_SCHEMA = (
    "morsehgp3d.phase15_delaunay_gabriel_fusion_guardrails_check.v1"
)

OUTER_BACKEND = (
    "ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid_plus_binary64_LBVH_surrogate"
)
OUTER_MODE = "gabriel_necessary_batch_plus_variant_fusion_guardrails"
OUTER_COVERAGE_MODE = (
    "all_accepted_Gabriel_sources_explicit_no_global_facet_DSU"
)
DIRECT_VARIANTS = (
    "two_edge",
    "closed_star",
    "square_clique",
    "link_face_fan",
    "one_edge",
)
DIRECT_CERTIFICATE_KIND = "source_event_inclusion_deadline_upper_bound_v1"
DIRECT_INCLUSION_BASIS = (
    "two_Delaunay_edge_source_and_declared_variant_superset_v1"
)
POINT_LIFT_ADAPTER = "point_component_clique_lift_v1"
POINT_LIFT_SEMANTICS = (
    "one_lifted_component_per_point_component_with_all_internal_pair_facets"
)
SOURCE_LEVEL_CONVENTION = "binary64_squared_Gabriel_radius_recipe"
RAW_TREE_LEVEL_CONVENTION = (
    "binary64_mutual_reachability_squared_distance_recipe"
)
LEVEL_CONVERSION = "exact_dyadic_divide_by_4"
RAW_CORRECTED_LEVEL_ENCODING = (
    "raw_squared_weight_exact_dyadic_divide_by_4"
)
DIRECT_CORRECTED_LEVEL_ENCODING = "direct_Gabriel_squared_level"
_CORRECTED_LEVEL_ENCODING_CODE = {
    RAW_CORRECTED_LEVEL_ENCODING: 1,
    DIRECT_CORRECTED_LEVEL_ENCODING: 2,
}

GABRIEL_STATUS = "gabriel_binary64"
AMBIGUOUS_STATUS = "ambiguous_requires_cpu_recertification"
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
HEX64_PATTERN = re.compile(r"0x[0-9a-f]{16}")
GIT_SHA_PATTERN = re.compile(r"[0-9a-f]{40}")
MAX_U64 = (1 << 64) - 1
MAX_EMITTED_RECORD_POINTS = 256

_DECISION_CODE = {
    "before": 1,
    "at": 2,
    "late": 3,
    "never": 4,
    "unsupported": 5,
}


class DelaunayGuardrailInputError(ValueError):
    """The outer report cannot support the requested fail-closed check."""


@dataclass(frozen=True)
class TriangleRecord:
    a: int
    b: int
    c: int
    squared_level: float
    squared_level_bits: int
    status: str
    support_cardinality: int

    @property
    def point_ids(self) -> tuple[int, int, int]:
        return self.a, self.b, self.c

    @property
    def sort_key(self) -> tuple[float, int, int, int]:
        return self.squared_level, self.a, self.b, self.c

    def as_json(self) -> dict[str, object]:
        return {
            "a": self.a,
            "b": self.b,
            "c": self.c,
            "squared_level": self.squared_level,
            "status": self.status,
            "support_cardinality": self.support_cardinality,
        }


@dataclass(frozen=True)
class RawTreeEdge:
    u: int
    v: int
    raw_squared_weight: float
    weight_bits: int

    def as_json(self) -> dict[str, object]:
        return {
            "u": self.u,
            "v": self.v,
            "raw_squared_weight": self.raw_squared_weight,
        }


@dataclass(frozen=True)
class CorrectedTreeEdge:
    u: int
    v: int
    level_encoding: str
    encoded_binary64: float
    encoded_bits: int

    @property
    def exact_squared_level(self) -> Fraction:
        level = Fraction.from_float(self.encoded_binary64)
        if self.level_encoding == RAW_CORRECTED_LEVEL_ENCODING:
            return level / 4
        return level

    def as_json(self) -> dict[str, object]:
        return {
            "u": self.u,
            "v": self.v,
            "level_encoding": self.level_encoding,
            "encoded_binary64": self.encoded_binary64,
        }


@dataclass(frozen=True)
class _ReplayResult:
    counts: dict[str, int]
    corrected_counts: dict[str, int]
    decisions: tuple[int, ...]
    corrected_decisions: tuple[int, ...]
    correction_records: tuple[TriangleRecord, ...]
    corrected_tree: tuple[CorrectedTreeEdge, ...]
    useful_union_count: int
    corrected_postcondition_violation_count: int
    first_failure_witness: dict[str, object] | None
    first_late_witness: dict[str, object] | None
    first_unsupported_witness: dict[str, object] | None
    corrected_first_failure_witness: dict[str, object] | None
    corrected_first_late_witness: dict[str, object] | None
    corrected_first_unsupported_witness: dict[str, object] | None


class _DisjointSet:
    def __init__(self, size: int) -> None:
        self.parent = list(range(size))
        self.sizes = [1] * size
        self.component_count = size

    def find(self, value: int) -> int:
        root = value
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[value] != value:
            following = self.parent[value]
            self.parent[value] = root
            value = following
        return root

    def union(self, left: int, right: int) -> bool:
        left = self.find(left)
        right = self.find(right)
        if left == right:
            return False
        if self.sizes[left] < self.sizes[right] or (
            self.sizes[left] == self.sizes[right] and right < left
        ):
            left, right = right, left
        self.parent[right] = left
        self.sizes[left] += self.sizes[right]
        self.component_count -= 1
        return True


def _mapping(value: object, *, path: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        raise DelaunayGuardrailInputError(f"{path} must be an object")
    if any(not isinstance(key, str) for key in value):
        raise DelaunayGuardrailInputError(f"{path} keys must be strings")
    return value  # type: ignore[return-value]


def _array(value: object, *, path: str) -> list[object]:
    if not isinstance(value, list):
        raise DelaunayGuardrailInputError(f"{path} must be an array")
    return value


def _literal(
    record: Mapping[str, object], key: str, expected: object, *, path: str
) -> None:
    observed = record.get(key)
    if type(observed) is not type(expected) or observed != expected:
        raise DelaunayGuardrailInputError(
            f"{path}.{key} must be {expected!r}"
        )


def _require_exact_keys(
    record: Mapping[str, object], expected: Iterable[str], *, path: str
) -> None:
    expected_keys = set(expected)
    observed_keys = set(record)
    if observed_keys != expected_keys:
        missing = sorted(expected_keys - observed_keys)
        extra = sorted(observed_keys - expected_keys)
        raise DelaunayGuardrailInputError(
            f"{path} keys differ: missing={missing!r}, extra={extra!r}"
        )


def _uint(value: object, *, path: str, positive: bool = False) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise DelaunayGuardrailInputError(
            f"{path} must be a non-negative integer"
        )
    if value < 0 or (positive and value == 0):
        qualifier = "positive" if positive else "non-negative"
        raise DelaunayGuardrailInputError(f"{path} must be {qualifier}")
    if value > MAX_U64:
        raise DelaunayGuardrailInputError(
            f"{path} must fit in an unsigned 64-bit integer"
        )
    return value


def _boolean(value: object, *, path: str) -> bool:
    if not isinstance(value, bool):
        raise DelaunayGuardrailInputError(f"{path} must be a boolean")
    return value


def _sha256(value: object, *, path: str) -> str:
    if not isinstance(value, str) or SHA256_PATTERN.fullmatch(value) is None:
        raise DelaunayGuardrailInputError(
            f"{path} must be 64 lowercase hexadecimal digits"
        )
    return value


def _hex64(value: object, *, path: str) -> str:
    if not isinstance(value, str) or HEX64_PATTERN.fullmatch(value) is None:
        raise DelaunayGuardrailInputError(
            f"{path} must be 0x followed by 16 lowercase hexadecimal digits"
        )
    return value


def _binary64(
    value: object, *, path: str, non_negative: bool = True
) -> tuple[float, int]:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise DelaunayGuardrailInputError(f"{path} must be a binary64 number")
    try:
        result = float(value)
    except OverflowError as error:
        raise DelaunayGuardrailInputError(f"{path} must be finite") from error
    if not math.isfinite(result) or (non_negative and result < 0.0):
        raise DelaunayGuardrailInputError(
            f"{path} must be finite and non-negative"
        )
    return result, struct.unpack(">Q", struct.pack(">d", result))[0]


def _nonempty_string(value: object, *, path: str) -> str:
    if not isinstance(value, str) or not value or value != value.strip():
        raise DelaunayGuardrailInputError(
            f"{path} must be a non-empty trimmed string"
        )
    return value


def _sha256_word(builder: object, value: int) -> None:
    if value < 0 or value > MAX_U64:
        raise DelaunayGuardrailInputError("a digest word lies outside uint64")
    builder.update(struct.pack(">Q", value))  # type: ignore[attr-defined]


def _sha256_triangle(builder: object, record: TriangleRecord) -> None:
    _sha256_word(builder, record.squared_level_bits)
    _sha256_word(builder, record.a)
    _sha256_word(builder, record.b)
    _sha256_word(builder, record.c)
    _sha256_word(builder, record.support_cardinality)


def _point_id(value: object, *, path: str, point_count: int) -> int:
    point_id = _uint(value, path=path)
    if point_id >= point_count:
        raise DelaunayGuardrailInputError(
            f"{path} lies outside 0..{point_count - 1}"
        )
    return point_id


def _parse_triangle(
    value: object, *, path: str, point_count: int
) -> TriangleRecord:
    record = _mapping(value, path=path)
    a = _point_id(record.get("a"), path=f"{path}.a", point_count=point_count)
    b = _point_id(record.get("b"), path=f"{path}.b", point_count=point_count)
    c = _point_id(record.get("c"), path=f"{path}.c", point_count=point_count)
    if not a < b < c:
        raise DelaunayGuardrailInputError(f"{path} must satisfy a < b < c")
    level, level_bits = _binary64(
        record.get("squared_level"), path=f"{path}.squared_level"
    )
    status = record.get("status")
    if status not in {GABRIEL_STATUS, AMBIGUOUS_STATUS}:
        raise DelaunayGuardrailInputError(
            f"{path}.status must be a retained Gabriel or ambiguous status"
        )
    support = _uint(
        record.get("support_cardinality"),
        path=f"{path}.support_cardinality",
    )
    if support not in {2, 3}:
        raise DelaunayGuardrailInputError(
            f"{path}.support_cardinality must be 2 or 3"
        )
    return TriangleRecord(a, b, c, level, level_bits, str(status), support)


def _parse_triangle_array(
    value: object, *, path: str, point_count: int
) -> tuple[TriangleRecord, ...]:
    records = tuple(
        _parse_triangle(item, path=f"{path}[{index}]", point_count=point_count)
        for index, item in enumerate(_array(value, path=path))
    )
    if any(
        records[index].sort_key < records[index - 1].sort_key
        for index in range(1, len(records))
    ):
        raise DelaunayGuardrailInputError(
            f"{path} must be sorted by binary64 level and point IDs"
        )
    identities = [record.point_ids for record in records]
    if len(set(identities)) != len(identities):
        raise DelaunayGuardrailInputError(
            f"{path} contains duplicate triangle identities"
        )
    return records


def recompute_direct_variant_decision_sha256(
    variant: str,
    accepted_source_sha256: str,
    ambiguous_source_sha256: str,
    accepted_source_count: int,
    ambiguous_source_count: int,
) -> str:
    """Recompute the producer's direct source-inclusion commitment."""

    if variant not in DIRECT_VARIANTS:
        raise DelaunayGuardrailInputError("unknown direct variant")
    accepted = _sha256(
        accepted_source_sha256, path="accepted_source_sha256"
    )
    ambiguous = _sha256(
        ambiguous_source_sha256, path="ambiguous_source_sha256"
    )
    builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        b"direct-source-inclusion-deadline-upper-bound-v1/sha256/"
    )
    builder.update(variant.encode("ascii"))
    builder.update(bytes.fromhex(accepted))
    builder.update(bytes.fromhex(ambiguous))
    _sha256_word(builder, accepted_source_count)
    _sha256_word(builder, ambiguous_source_count)
    return builder.hexdigest()


def _raw_tree_sha256(
    point_count: int, order: int, tree: Sequence[RawTreeEdge]
) -> str:
    builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        b"surrogate-tree-v1/sha256/"
    )
    for value in (point_count, order, len(tree)):
        _sha256_word(builder, value)
    for edge in tree:
        for value in (edge.weight_bits, edge.u, edge.v):
            _sha256_word(builder, value)
    return builder.hexdigest()


def _corrected_tree_sha256(
    point_count: int, order: int, tree: Sequence[CorrectedTreeEdge]
) -> str:
    builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        b"corrected-surrogate-level-tree-v1/sha256/"
    )
    for value in (point_count, order, len(tree)):
        _sha256_word(builder, value)
    for edge in tree:
        for value in (
            _CORRECTED_LEVEL_ENCODING_CODE[edge.level_encoding],
            edge.encoded_bits,
            edge.u,
            edge.v,
        ):
            _sha256_word(builder, value)
    return builder.hexdigest()


def _decision_sha256(
    order: int,
    sources: Sequence[TriangleRecord],
    decisions: Sequence[int],
    *,
    corrected: bool,
) -> str:
    if len(sources) != len(decisions):
        raise DelaunayGuardrailInputError("decision extent mismatch")
    domain = (
        b"corrected-decisions-v1/sha256/"
        if corrected
        else b"raw-decisions-v1/sha256/"
    )
    builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-fusion-guardrail/" + domain
    )
    _sha256_word(builder, order)
    _sha256_word(builder, len(sources))
    for source, decision in zip(sources, decisions):
        for value in (
            source.squared_level_bits,
            source.a,
            source.b,
            source.c,
            decision,
        ):
            _sha256_word(builder, value)
    return builder.hexdigest()


def _overlay_sha256(
    order: int, records: Sequence[TriangleRecord]
) -> str:
    builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-fusion-guardrail/"
        b"canonical-greedy-overlay-v1/sha256/"
    )
    _sha256_word(builder, order)
    for record in records:
        for value in (
            record.squared_level_bits,
            record.a,
            record.b,
            record.c,
        ):
            _sha256_word(builder, value)
    builder.update(b"/count/")
    _sha256_word(builder, len(records))
    return builder.hexdigest()


def _parse_raw_tree_edges(
    value: object, *, path: str, point_count: int
) -> tuple[RawTreeEdge, ...]:
    edges: list[RawTreeEdge] = []
    for index, item in enumerate(_array(value, path=path)):
        edge_path = f"{path}[{index}]"
        record = _mapping(item, path=edge_path)
        u = _point_id(
            record.get("u"), path=f"{edge_path}.u", point_count=point_count
        )
        v = _point_id(
            record.get("v"), path=f"{edge_path}.v", point_count=point_count
        )
        if u >= v:
            raise DelaunayGuardrailInputError(f"{edge_path} must satisfy u < v")
        weight, bits = _binary64(
            record.get("raw_squared_weight"),
            path=f"{edge_path}.raw_squared_weight",
        )
        edges.append(RawTreeEdge(u, v, weight, bits))
    if any(
        edges[index].raw_squared_weight
        < edges[index - 1].raw_squared_weight
        for index in range(1, len(edges))
    ):
        raise DelaunayGuardrailInputError(
            f"{path} must be ordered by nondecreasing raw weight"
        )
    _verify_spanning_tree(edges, point_count=point_count, path=path)
    return tuple(edges)


def _parse_corrected_tree_edges(
    value: object, *, path: str, point_count: int
) -> tuple[CorrectedTreeEdge, ...]:
    edges: list[CorrectedTreeEdge] = []
    for index, item in enumerate(_array(value, path=path)):
        edge_path = f"{path}[{index}]"
        record = _mapping(item, path=edge_path)
        u = _point_id(
            record.get("u"), path=f"{edge_path}.u", point_count=point_count
        )
        v = _point_id(
            record.get("v"), path=f"{edge_path}.v", point_count=point_count
        )
        if u >= v:
            raise DelaunayGuardrailInputError(f"{edge_path} must satisfy u < v")
        encoding = record.get("level_encoding")
        if encoding not in _CORRECTED_LEVEL_ENCODING_CODE:
            raise DelaunayGuardrailInputError(
                f"{edge_path}.level_encoding is not recognized"
            )
        encoded, bits = _binary64(
            record.get("encoded_binary64"),
            path=f"{edge_path}.encoded_binary64",
        )
        edges.append(CorrectedTreeEdge(u, v, str(encoding), encoded, bits))
    if any(
        edges[index].exact_squared_level
        < edges[index - 1].exact_squared_level
        for index in range(1, len(edges))
    ):
        raise DelaunayGuardrailInputError(
            f"{path} must be ordered by nondecreasing squared level"
        )
    _verify_spanning_tree(edges, point_count=point_count, path=path)
    return tuple(edges)


def _verify_spanning_tree(
    edges: Sequence[RawTreeEdge] | Sequence[CorrectedTreeEdge],
    *,
    point_count: int,
    path: str,
) -> None:
    if len(edges) != point_count - 1:
        raise DelaunayGuardrailInputError(
            f"{path} must contain point_count-1 edges"
        )
    components = _DisjointSet(point_count)
    endpoints: set[tuple[int, int]] = set()
    for edge in edges:
        pair = edge.u, edge.v
        if pair in endpoints:
            raise DelaunayGuardrailInputError(f"{path} duplicates edge {pair}")
        endpoints.add(pair)
        if not components.union(edge.u, edge.v):
            raise DelaunayGuardrailInputError(f"{path} contains a cycle")
    if components.component_count != 1:
        raise DelaunayGuardrailInputError(f"{path} is not spanning")


def _connected(components: _DisjointSet, source: TriangleRecord) -> bool:
    root = components.find(source.a)
    return components.find(source.b) == root and components.find(source.c) == root


def _compare_raw_to_source(raw_weight: float, source_level: float) -> int:
    converted = Fraction.from_float(raw_weight) / 4
    source = Fraction.from_float(source_level)
    return (converted > source) - (converted < source)


def _late_connection_weight(
    tree: Sequence[RawTreeEdge], source: TriangleRecord
) -> float | None:
    components = _DisjointSet(len(tree) + 1)
    begin = 0
    while begin < len(tree):
        weight = tree[begin].raw_squared_weight
        end = begin + 1
        while end < len(tree) and tree[end].raw_squared_weight == weight:
            end += 1
        for edge in tree[begin:end]:
            components.union(edge.u, edge.v)
        if _connected(components, source):
            return weight
        begin = end
    return None


def _failure_witness(
    classification: str,
    source: TriangleRecord,
    *,
    late_raw_weight: float | None = None,
    include_late_observation: bool = True,
) -> dict[str, object]:
    result: dict[str, object] = {
        "classification": classification,
        "source": source.as_json(),
        "observed_first_connection": None,
    }
    if classification == "late" and include_late_observation:
        if late_raw_weight is None:
            raise DelaunayGuardrailInputError(
                "a late witness has no eventual raw-tree connection"
            )
        result["observed_first_connection"] = {
            "raw_squared_weight": late_raw_weight,
            "level_conversion": LEVEL_CONVERSION,
        }
        result["strictly_after_source_level"] = True
    return result


def _replay_surrogate(
    sources: Sequence[TriangleRecord],
    tree: Sequence[RawTreeEdge],
) -> _ReplayResult:
    decisions = [0] * len(sources)
    corrected_decisions = [0] * len(sources)
    components = _DisjointSet(len(tree) + 1)
    corrected_components = _DisjointSet(len(tree) + 1)
    corrected_tree: list[CorrectedTreeEdge] = []
    correction_records: list[TriangleRecord] = []
    useful_unions = 0
    tree_position = 0

    def apply_raw(edge: RawTreeEdge) -> None:
        if not components.union(edge.u, edge.v):
            raise DelaunayGuardrailInputError(
                "the emitted surrogate raw tree contains a cycle"
            )
        if corrected_components.union(edge.u, edge.v):
            corrected_tree.append(
                CorrectedTreeEdge(
                    edge.u,
                    edge.v,
                    RAW_CORRECTED_LEVEL_ENCODING,
                    edge.raw_squared_weight,
                    edge.weight_bits,
                )
            )

    plateau_begin = 0
    corrected_violations = 0
    while plateau_begin < len(sources):
        source_level = sources[plateau_begin].squared_level
        plateau_end = plateau_begin + 1
        while (
            plateau_end < len(sources)
            and sources[plateau_end].squared_level == source_level
        ):
            plateau_end += 1

        while (
            tree_position < len(tree)
            and _compare_raw_to_source(
                tree[tree_position].raw_squared_weight, source_level
            )
            < 0
        ):
            apply_raw(tree[tree_position])
            tree_position += 1

        for index in range(plateau_begin, plateau_end):
            source = sources[index]
            if source.status == AMBIGUOUS_STATUS:
                decisions[index] = _DECISION_CODE["unsupported"]
                corrected_decisions[index] = _DECISION_CODE["unsupported"]
            elif _connected(components, source):
                decisions[index] = _DECISION_CODE["before"]
            if source.status == GABRIEL_STATUS and _connected(
                corrected_components, source
            ):
                corrected_decisions[index] = _DECISION_CODE["before"]

        while (
            tree_position < len(tree)
            and _compare_raw_to_source(
                tree[tree_position].raw_squared_weight, source_level
            )
            == 0
        ):
            apply_raw(tree[tree_position])
            tree_position += 1

        for index in range(plateau_begin, plateau_end):
            if decisions[index] == 0:
                decisions[index] = _DECISION_CODE[
                    "at" if _connected(components, sources[index]) else "late"
                ]

        for index in range(plateau_begin, plateau_end):
            source = sources[index]
            if corrected_decisions[index] in {
                _DECISION_CODE["unsupported"],
                _DECISION_CODE["before"],
            }:
                continue
            if not _connected(corrected_components, source):
                correction_records.append(source)
                if corrected_components.union(source.a, source.b):
                    useful_unions += 1
                    corrected_tree.append(
                        CorrectedTreeEdge(
                            source.a,
                            source.b,
                            DIRECT_CORRECTED_LEVEL_ENCODING,
                            source.squared_level,
                            source.squared_level_bits,
                        )
                    )
                if corrected_components.union(source.a, source.c):
                    useful_unions += 1
                    corrected_tree.append(
                        CorrectedTreeEdge(
                            source.a,
                            source.c,
                            DIRECT_CORRECTED_LEVEL_ENCODING,
                            source.squared_level,
                            source.squared_level_bits,
                        )
                    )

        for index in range(plateau_begin, plateau_end):
            source = sources[index]
            if source.status != GABRIEL_STATUS:
                continue
            if not _connected(corrected_components, source):
                corrected_violations += 1
                corrected_decisions[index] = _DECISION_CODE["late"]
            elif corrected_decisions[index] == 0:
                corrected_decisions[index] = _DECISION_CODE["at"]
        plateau_begin = plateau_end

    while tree_position < len(tree):
        apply_raw(tree[tree_position])
        tree_position += 1

    if components.component_count != 1 or corrected_components.component_count != 1:
        raise DelaunayGuardrailInputError(
            "the raw or corrected emitted tree does not close"
        )
    if len(corrected_tree) != len(tree):
        raise DelaunayGuardrailInputError(
            "the corrected emitted tree has a wrong edge count"
        )
    if any(
        corrected_tree[index].exact_squared_level
        < corrected_tree[index - 1].exact_squared_level
        for index in range(1, len(corrected_tree))
    ):
        raise DelaunayGuardrailInputError(
            "the independently replayed corrected tree is not level ordered"
        )

    classes = tuple(_DECISION_CODE)
    counts = {
        "source_triangle_count": len(sources),
        "supported_triangle_count": sum(
            source.status == GABRIEL_STATUS for source in sources
        ),
        **{
            f"connected_{name}_count" if name in {"before", "at"} else f"{name}_count": sum(
                decision == _DECISION_CODE[name] for decision in decisions
            )
            for name in classes
        },
    }
    counts["satisfied_count"] = (
        counts["connected_before_count"] + counts["connected_at_count"]
    )
    counts["violation_count"] = (
        counts["late_count"]
        + counts["never_count"]
        + counts["unsupported_count"]
    )
    corrected_counts = {
        "source_triangle_count": len(sources),
        **{
            f"connected_{name}_count"
            if name in {"before", "at"}
            else f"{name}_count": sum(
                decision == _DECISION_CODE[name]
                for decision in corrected_decisions
            )
            for name in classes
        },
    }
    corrected_counts["satisfied_count"] = (
        corrected_counts["connected_before_count"]
        + corrected_counts["connected_at_count"]
    )
    corrected_counts["violation_count"] = (
        corrected_counts["late_count"]
        + corrected_counts["never_count"]
        + corrected_counts["unsupported_count"]
    )

    first_failure: dict[str, object] | None = None
    first_late: dict[str, object] | None = None
    first_unsupported: dict[str, object] | None = None
    corrected_first_failure: dict[str, object] | None = None
    corrected_first_late: dict[str, object] | None = None
    corrected_first_unsupported: dict[str, object] | None = None
    reverse_codes = {value: key for key, value in _DECISION_CODE.items()}
    for source, decision in zip(sources, decisions):
        classification = reverse_codes[decision]
        if classification in {"before", "at"}:
            continue
        late_weight = (
            _late_connection_weight(tree, source)
            if classification == "late"
            else None
        )
        witness = _failure_witness(
            classification, source, late_raw_weight=late_weight
        )
        if first_failure is None:
            first_failure = witness
        if classification == "late" and first_late is None:
            first_late = witness
        if classification == "unsupported" and first_unsupported is None:
            first_unsupported = witness

    for source, decision in zip(sources, corrected_decisions):
        classification = reverse_codes[decision]
        if classification in {"before", "at"}:
            continue
        witness = _failure_witness(
            classification,
            source,
            include_late_observation=False,
        )
        if corrected_first_failure is None:
            corrected_first_failure = witness
        if classification == "late" and corrected_first_late is None:
            corrected_first_late = witness
        if (
            classification == "unsupported"
            and corrected_first_unsupported is None
        ):
            corrected_first_unsupported = witness

    return _ReplayResult(
        counts=counts,
        corrected_counts=corrected_counts,
        decisions=tuple(decisions),
        corrected_decisions=tuple(corrected_decisions),
        correction_records=tuple(correction_records),
        corrected_tree=tuple(corrected_tree),
        useful_union_count=useful_unions,
        corrected_postcondition_violation_count=corrected_violations,
        first_failure_witness=first_failure,
        first_late_witness=first_late,
        first_unsupported_witness=first_unsupported,
        corrected_first_failure_witness=corrected_first_failure,
        corrected_first_late_witness=corrected_first_late,
        corrected_first_unsupported_witness=corrected_first_unsupported,
    )


def _five_way_counts(
    value: object, *, path: str
) -> tuple[dict[str, int], bool, bool]:
    record = _mapping(value, path=path)
    names = (
        "source_triangle_count",
        "supported_triangle_count",
        "connected_before_count",
        "connected_at_count",
        "late_count",
        "never_count",
        "unsupported_count",
        "satisfied_count",
        "violation_count",
    )
    _require_exact_keys(record, names, path=path)
    counts = {
        name: _uint(record.get(name), path=f"{path}.{name}") for name in names
    }
    before = counts["connected_before_count"]
    at = counts["connected_at_count"]
    late = counts["late_count"]
    never = counts["never_count"]
    unsupported = counts["unsupported_count"]
    if counts["source_triangle_count"] != before + at + late + never + unsupported:
        raise DelaunayGuardrailInputError(
            f"{path} does not close the five-way source partition"
        )
    if counts["supported_triangle_count"] != before + at + late + never:
        raise DelaunayGuardrailInputError(
            f"{path}.supported_triangle_count is inconsistent"
        )
    if counts["satisfied_count"] != before + at:
        raise DelaunayGuardrailInputError(
            f"{path}.satisfied_count must equal before + at"
        )
    if counts["violation_count"] != late + never + unsupported:
        raise DelaunayGuardrailInputError(
            f"{path}.violation_count must equal late + never + unsupported"
        )
    return counts, late == 0 and never == 0, late == 0 and never == 0 and unsupported == 0


def _corrected_five_way_counts(
    value: object, *, path: str
) -> tuple[dict[str, int], bool, bool]:
    record = _mapping(value, path=path)
    names = (
        "source_triangle_count",
        "connected_before_count",
        "connected_at_count",
        "late_count",
        "never_count",
        "unsupported_count",
        "satisfied_count",
        "violation_count",
    )
    _require_exact_keys(record, names, path=path)
    counts = {
        name: _uint(record.get(name), path=f"{path}.{name}")
        for name in names
    }
    before = counts["connected_before_count"]
    at = counts["connected_at_count"]
    late = counts["late_count"]
    never = counts["never_count"]
    unsupported = counts["unsupported_count"]
    if counts["source_triangle_count"] != before + at + late + never + unsupported:
        raise DelaunayGuardrailInputError(
            f"{path} does not close the corrected five-way source partition"
        )
    if counts["satisfied_count"] != before + at:
        raise DelaunayGuardrailInputError(
            f"{path}.satisfied_count must equal before + at"
        )
    if counts["violation_count"] != late + never + unsupported:
        raise DelaunayGuardrailInputError(
            f"{path}.violation_count must equal late + never + unsupported"
        )
    return (
        counts,
        late == 0 and never == 0,
        late == 0 and never == 0 and unsupported == 0,
    )


def _validate_witness_shape(
    value: object,
    *,
    path: str,
    point_count: int,
    allowed_classifications: set[str],
    require_late_observation: bool = True,
) -> dict[str, object] | None:
    if value is None:
        return None
    record = _mapping(value, path=path)
    classification = record.get("classification")
    if classification not in allowed_classifications:
        raise DelaunayGuardrailInputError(
            f"{path}.classification is inconsistent with its counter"
        )
    source = _parse_triangle(
        record.get("source"), path=f"{path}.source", point_count=point_count
    )
    if classification == "unsupported" and source.status != AMBIGUOUS_STATUS:
        raise DelaunayGuardrailInputError(
            f"{path} unsupported witness must carry an ambiguous source"
        )
    if classification != "unsupported" and source.status != GABRIEL_STATUS:
        raise DelaunayGuardrailInputError(
            f"{path} regular failure must carry an accepted source"
        )
    observed = record.get("observed_first_connection")
    normalized: dict[str, object] = {
        "classification": classification,
        "source": source.as_json(),
        "observed_first_connection": None,
    }
    if classification == "late" and require_late_observation:
        observed_record = _mapping(
            observed, path=f"{path}.observed_first_connection"
        )
        weight, _ = _binary64(
            observed_record.get("raw_squared_weight"),
            path=f"{path}.observed_first_connection.raw_squared_weight",
        )
        _literal(
            observed_record,
            "level_conversion",
            LEVEL_CONVERSION,
            path=f"{path}.observed_first_connection",
        )
        _literal(record, "strictly_after_source_level", True, path=path)
        if _compare_raw_to_source(weight, source.squared_level) <= 0:
            raise DelaunayGuardrailInputError(
                f"{path} late connection is not strictly after its source"
            )
        normalized["observed_first_connection"] = {
            "raw_squared_weight": weight,
            "level_conversion": LEVEL_CONVERSION,
        }
        normalized["strictly_after_source_level"] = True
    elif observed is not None:
        raise DelaunayGuardrailInputError(
            f"{path}.observed_first_connection must be null"
        )
    return normalized


def _validate_direct_variants(
    value: object,
    *,
    source_count: int,
    accepted_count: int,
    ambiguous_count: int,
    accepted_sha256: str,
    ambiguous_sha256: str,
    point_count: int,
    emitted_sources: tuple[TriangleRecord, ...] | None,
) -> tuple[list[dict[str, object]], bool]:
    raw = _array(value, path="variant_guardrails.direct_facet_variants")
    if len(raw) != len(DIRECT_VARIANTS):
        raise DelaunayGuardrailInputError(
            "variant_guardrails.direct_facet_variants must contain five variants"
        )
    summaries: list[dict[str, object]] = []
    all_fail_closed = True
    for index, expected_variant in enumerate(DIRECT_VARIANTS):
        path = f"variant_guardrails.direct_facet_variants[{index}]"
        record = _mapping(raw[index], path=path)
        for key, expected in (
            ("variant", expected_variant),
            ("representation", "k2_pair_facet_relation_stream"),
            ("certificate_kind", DIRECT_CERTIFICATE_KIND),
            ("full_variant_first_connection_replayed", False),
            ("source_inclusion_basis", DIRECT_INCLUSION_BASIS),
            ("source_triangle_included_by_definition", True),
            ("deadline_upper_bound_partition_closed", True),
            ("regular_binary64_guardrail_passed", True),
        ):
            _literal(record, key, expected, path=path)
        counts_record = _mapping(record.get("counts"), path=f"{path}.counts")
        direct_count_names = (
            "source_triangle_count",
            "accepted_source_triangle_count",
            "guaranteed_no_later_than_deadline_count",
            "violation_count",
            "unsupported_count",
        )
        _require_exact_keys(counts_record, direct_count_names, path=f"{path}.counts")
        counts = {
            name: _uint(
                counts_record.get(name), path=f"{path}.counts.{name}"
            )
            for name in direct_count_names
        }
        if counts != {
            "source_triangle_count": source_count,
            "accepted_source_triangle_count": accepted_count,
            "guaranteed_no_later_than_deadline_count": accepted_count,
            "violation_count": 0,
            "unsupported_count": ambiguous_count,
        }:
            raise DelaunayGuardrailInputError(
                f"{path}.counts disagrees with the shared source anchors"
            )
        if source_count != (
            counts["guaranteed_no_later_than_deadline_count"]
            + counts["violation_count"]
            + counts["unsupported_count"]
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.counts does not close the deadline upper-bound partition"
            )
        expected_fail_closed = ambiguous_count == 0
        _literal(
            record,
            "fail_closed_guardrail_passed",
            expected_fail_closed,
            path=path,
        )
        decision = _sha256(
            record.get("decision_sha256"), path=f"{path}.decision_sha256"
        )
        expected_decision = recompute_direct_variant_decision_sha256(
            expected_variant,
            accepted_sha256,
            ambiguous_sha256,
            accepted_count,
            ambiguous_count,
        )
        if decision != expected_decision:
            raise DelaunayGuardrailInputError(
                f"{path}.decision_sha256 does not match the shared anchors"
            )
        allowed = {"unsupported"} if ambiguous_count else set()
        first_failure = _validate_witness_shape(
            record.get("first_failure_witness"),
            path=f"{path}.first_failure_witness",
            point_count=point_count,
            allowed_classifications=allowed,
        )
        first_unsupported = _validate_witness_shape(
            record.get("first_unsupported_witness"),
            path=f"{path}.first_unsupported_witness",
            point_count=point_count,
            allowed_classifications=allowed,
        )
        if (first_failure is None) != (ambiguous_count == 0):
            raise DelaunayGuardrailInputError(
                f"{path}.first_failure_witness presence is inconsistent"
            )
        if (first_unsupported is None) != (ambiguous_count == 0):
            raise DelaunayGuardrailInputError(
                f"{path}.first_unsupported_witness presence is inconsistent"
            )
        if first_failure != first_unsupported:
            raise DelaunayGuardrailInputError(
                f"{path} direct unsupported witnesses must agree"
            )
        if emitted_sources is not None:
            expected_unsupported_source = next(
                (
                    source
                    for source in emitted_sources
                    if source.status == AMBIGUOUS_STATUS
                ),
                None,
            )
            expected_witness = (
                None
                if expected_unsupported_source is None
                else _failure_witness(
                    "unsupported",
                    expected_unsupported_source,
                    include_late_observation=False,
                )
            )
            if first_failure != expected_witness:
                raise DelaunayGuardrailInputError(
                    f"{path} direct first unsupported witness does not match records"
                )
        all_fail_closed = all_fail_closed and expected_fail_closed
        summaries.append(
            {
                "variant": expected_variant,
                "certificate_kind": DIRECT_CERTIFICATE_KIND,
                "full_variant_first_connection_replayed": False,
                "counts": counts,
                "decision_sha256_recomputed_from_shared_anchors": True,
                "fail_closed_guardrail_passed": expected_fail_closed,
            }
        )
    return summaries, all_fail_closed


def _validate_distinct_levels(
    values: Sequence[float], declared: int, *, path: str
) -> None:
    bits = [struct.unpack(">Q", struct.pack(">d", value))[0] for value in values]
    observed = sum(index == 0 or bits[index] != bits[index - 1] for index in range(len(bits)))
    if declared != observed:
        raise DelaunayGuardrailInputError(
            f"{path} disagrees with the emitted edge levels"
        )


def _validate_corrected_distinct_levels(
    edges: Sequence[CorrectedTreeEdge], declared: int, *, path: str
) -> None:
    observed = sum(
        index == 0
        or edges[index].exact_squared_level
        != edges[index - 1].exact_squared_level
        for index in range(len(edges))
    )
    if declared != observed:
        raise DelaunayGuardrailInputError(
            f"{path} disagrees with the emitted corrected edge levels"
        )


def _validate_surrogate_orders(
    value: object,
    *,
    maximum_order: int,
    point_count: int,
    source_count: int,
    accepted_count: int,
    ambiguous_count: int,
    emitted_sources: tuple[TriangleRecord, ...] | None,
) -> tuple[list[dict[str, object]], bool, bool, bool]:
    raw_orders = _array(value, path="variant_guardrails.surrogate.orders")
    if len(raw_orders) != maximum_order:
        raise DelaunayGuardrailInputError(
            "variant_guardrails.surrogate.orders must contain every contiguous order 1..maximum_order"
        )
    summaries: list[dict[str, object]] = []
    raw_regular_all = True
    corrected_regular_all = True
    corrected_fail_closed_all = True
    records_expected = emitted_sources is not None

    for index, item in enumerate(raw_orders):
        path = f"variant_guardrails.surrogate.orders[{index}]"
        record = _mapping(item, path=path)
        order = _uint(record.get("order"), path=f"{path}.order", positive=True)
        if order != index + 1:
            raise DelaunayGuardrailInputError(
                "surrogate orders must be the contiguous sequence 1..maximum_order"
            )
        tree = _mapping(record.get("tree"), path=f"{path}.tree")
        proposed = _uint(
            tree.get("proposed_edge_count"),
            path=f"{path}.tree.proposed_edge_count",
        )
        unique = _uint(
            tree.get("unique_edge_count"), path=f"{path}.tree.unique_edge_count"
        )
        selected = _uint(
            tree.get("selected_tree_edge_count"),
            path=f"{path}.tree.selected_tree_edge_count",
        )
        final_components = _uint(
            tree.get("final_component_count"),
            path=f"{path}.tree.final_component_count",
        )
        distinct = _uint(
            tree.get("distinct_level_count"),
            path=f"{path}.tree.distinct_level_count",
        )
        expected_proposed = point_count * 2 - 1
        if expected_proposed > MAX_U64 or proposed != expected_proposed:
            raise DelaunayGuardrailInputError(
                f"{path}.tree.proposed_edge_count must equal 2*n-1"
            )
        if selected != point_count - 1 or not selected <= unique <= proposed:
            raise DelaunayGuardrailInputError(
                f"{path}.tree edge counts do not describe a spanning reduction"
            )
        if final_components != 1 or not 1 <= distinct <= selected:
            raise DelaunayGuardrailInputError(
                f"{path}.tree component or level count is inconsistent"
            )
        root_weight, root_bits = _binary64(
            tree.get("root_raw_squared_weight"),
            path=f"{path}.tree.root_raw_squared_weight",
        )
        tree_sha = _sha256(
            tree.get("tree_sha256"), path=f"{path}.tree.tree_sha256"
        )
        _hex64(
            tree.get("surrogate_compatible_digest"),
            path=f"{path}.tree.surrogate_compatible_digest",
        )
        raw_tree_records: tuple[RawTreeEdge, ...] | None = None
        if records_expected:
            raw_tree_records = _parse_raw_tree_edges(
                tree.get("tree_edges"),
                path=f"{path}.tree.tree_edges",
                point_count=point_count,
            )
            if len(raw_tree_records) != selected:
                raise DelaunayGuardrailInputError(
                    f"{path}.tree.tree_edges extent mismatch"
                )
            if raw_tree_records[-1].weight_bits != root_bits:
                raise DelaunayGuardrailInputError(
                    f"{path}.tree.root_raw_squared_weight disagrees with records"
                )
            _validate_distinct_levels(
                [edge.raw_squared_weight for edge in raw_tree_records],
                distinct,
                path=f"{path}.tree.distinct_level_count",
            )
            if _raw_tree_sha256(point_count, order, raw_tree_records) != tree_sha:
                raise DelaunayGuardrailInputError(
                    f"{path}.tree.tree_sha256 does not match emitted records"
                )
        elif tree.get("tree_edges") is not None:
            raise DelaunayGuardrailInputError(
                f"{path}.tree.tree_edges requires emitted_fixture"
            )

        counts, raw_regular, raw_fail_closed = _five_way_counts(
            record.get("counts"), path=f"{path}.counts"
        )
        if (
            counts["source_triangle_count"] != source_count
            or counts["supported_triangle_count"] != accepted_count
            or counts["unsupported_count"] != ambiguous_count
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.counts disagrees with shared source anchors"
            )
        _literal(
            record,
            "classification_partition_closed",
            True,
            path=path,
        )
        _literal(
            record,
            "regular_binary64_guardrail_passed",
            raw_regular,
            path=path,
        )
        _literal(
            record,
            "fail_closed_guardrail_passed",
            raw_fail_closed,
            path=path,
        )
        decision_sha = _sha256(
            record.get("decision_sha256"), path=f"{path}.decision_sha256"
        )

        nonzero_failures = {
            classification
            for classification, key in (
                ("late", "late_count"),
                ("never", "never_count"),
                ("unsupported", "unsupported_count"),
            )
            if counts[key] != 0
        }
        first_failure = _validate_witness_shape(
            record.get("first_failure_witness"),
            path=f"{path}.first_failure_witness",
            point_count=point_count,
            allowed_classifications=nonzero_failures,
        )
        first_late = _validate_witness_shape(
            record.get("first_late_witness"),
            path=f"{path}.first_late_witness",
            point_count=point_count,
            allowed_classifications={"late"} if counts["late_count"] else set(),
        )
        first_unsupported = _validate_witness_shape(
            record.get("first_unsupported_witness"),
            path=f"{path}.first_unsupported_witness",
            point_count=point_count,
            allowed_classifications=(
                {"unsupported"} if counts["unsupported_count"] else set()
            ),
        )
        if (first_failure is None) != (counts["violation_count"] == 0):
            raise DelaunayGuardrailInputError(
                f"{path}.first_failure_witness presence is inconsistent"
            )
        if (first_late is None) != (counts["late_count"] == 0):
            raise DelaunayGuardrailInputError(
                f"{path}.first_late_witness presence is inconsistent"
            )
        if (first_unsupported is None) != (counts["unsupported_count"] == 0):
            raise DelaunayGuardrailInputError(
                f"{path}.first_unsupported_witness presence is inconsistent"
            )

        correction = _mapping(
            record.get("correction"), path=f"{path}.correction"
        )
        _literal(
            correction,
            "algorithm",
            "canonical_greedy_closed_plateau_Gabriel_triangle_overlay_v1",
            path=f"{path}.correction",
        )
        correction_count = _uint(
            correction.get("correction_triangle_count"),
            path=f"{path}.correction.correction_triangle_count",
        )
        useful_unions = _uint(
            correction.get("useful_union_count"),
            path=f"{path}.correction.useful_union_count",
        )
        corrected_violations = _uint(
            correction.get("corrected_postcondition_violation_count"),
            path=(
                f"{path}.correction.corrected_postcondition_violation_count"
            ),
        )
        if correction_count > counts["late_count"] + counts["never_count"]:
            raise DelaunayGuardrailInputError(
                f"{path}.correction corrects more sources than raw regular failures"
            )
        if not correction_count <= useful_unions <= 2 * correction_count:
            raise DelaunayGuardrailInputError(
                f"{path}.correction useful union count is impossible"
            )
        if corrected_violations > counts["late_count"] + counts["never_count"]:
            raise DelaunayGuardrailInputError(
                f"{path}.correction postcondition count is impossible"
            )
        corrected_counts, corrected_regular, corrected_fail_closed = (
            _corrected_five_way_counts(
                correction.get("counts"), path=f"{path}.correction.counts"
            )
        )
        if (
            corrected_counts["source_triangle_count"] != source_count
            or corrected_counts["unsupported_count"] != ambiguous_count
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.correction.counts disagrees with shared source anchors"
            )
        if corrected_violations != (
            corrected_counts["late_count"]
            + corrected_counts["never_count"]
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.correction postcondition must equal corrected late + never"
            )
        _literal(
            correction,
            "classification_partition_closed",
            True,
            path=f"{path}.correction",
        )
        _literal(
            correction,
            "regular_binary64_guardrail_passed",
            corrected_regular,
            path=f"{path}.correction",
        )
        _literal(
            correction,
            "fail_closed_guardrail_passed",
            corrected_fail_closed,
            path=f"{path}.correction",
        )
        _literal(
            correction,
            "correction_applied_to_transient_verified_tree",
            True,
            path=f"{path}.correction",
        )
        _literal(
            correction,
            "corrected_tree_records_serialized",
            records_expected,
            path=f"{path}.correction",
        )
        overlay_sha = _sha256(
            correction.get("overlay_sha256"),
            path=f"{path}.correction.overlay_sha256",
        )
        corrected_decision_sha = _sha256(
            correction.get("corrected_decision_sha256"),
            path=f"{path}.correction.corrected_decision_sha256",
        )
        corrected_nonzero_failures = {
            classification
            for classification, key in (
                ("late", "late_count"),
                ("never", "never_count"),
                ("unsupported", "unsupported_count"),
            )
            if corrected_counts[key] != 0
        }
        corrected_first_failure = _validate_witness_shape(
            correction.get("first_failure_witness"),
            path=f"{path}.correction.first_failure_witness",
            point_count=point_count,
            allowed_classifications=corrected_nonzero_failures,
            require_late_observation=False,
        )
        corrected_first_late = _validate_witness_shape(
            correction.get("first_late_witness"),
            path=f"{path}.correction.first_late_witness",
            point_count=point_count,
            allowed_classifications=(
                {"late"} if corrected_counts["late_count"] else set()
            ),
            require_late_observation=False,
        )
        corrected_first_unsupported = _validate_witness_shape(
            correction.get("first_unsupported_witness"),
            path=f"{path}.correction.first_unsupported_witness",
            point_count=point_count,
            allowed_classifications=(
                {"unsupported"}
                if corrected_counts["unsupported_count"]
                else set()
            ),
            require_late_observation=False,
        )
        if (corrected_first_failure is None) != (
            corrected_counts["violation_count"] == 0
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.correction.first_failure_witness presence is inconsistent"
            )
        if (corrected_first_late is None) != (
            corrected_counts["late_count"] == 0
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.correction.first_late_witness presence is inconsistent"
            )
        if (corrected_first_unsupported is None) != (
            corrected_counts["unsupported_count"] == 0
        ):
            raise DelaunayGuardrailInputError(
                f"{path}.correction.first_unsupported_witness presence is inconsistent"
            )

        corrected_tree = _mapping(
            correction.get("corrected_tree"),
            path=f"{path}.correction.corrected_tree",
        )
        _literal(
            corrected_tree,
            "representation",
            "point_merge_tree_at_Gabriel_squared_levels_v1",
            path=f"{path}.correction.corrected_tree",
        )
        corrected_selected = _uint(
            corrected_tree.get("selected_tree_edge_count"),
            path=f"{path}.correction.corrected_tree.selected_tree_edge_count",
        )
        corrected_final = _uint(
            corrected_tree.get("final_component_count"),
            path=f"{path}.correction.corrected_tree.final_component_count",
        )
        corrected_distinct = _uint(
            corrected_tree.get("distinct_level_count"),
            path=f"{path}.correction.corrected_tree.distinct_level_count",
        )
        if corrected_selected != selected or corrected_final != 1:
            raise DelaunayGuardrailInputError(
                f"{path}.correction.corrected_tree does not remain spanning"
            )
        if not 1 <= corrected_distinct <= corrected_selected:
            raise DelaunayGuardrailInputError(
                f"{path}.correction.corrected_tree distinct levels are invalid"
            )
        root_level = _mapping(
            corrected_tree.get("root_level"),
            path=f"{path}.correction.corrected_tree.root_level",
        )
        corrected_root_encoding = root_level.get("level_encoding")
        if corrected_root_encoding not in _CORRECTED_LEVEL_ENCODING_CODE:
            raise DelaunayGuardrailInputError(
                f"{path}.correction.corrected_tree.root_level.level_encoding is not recognized"
            )
        corrected_root, corrected_root_bits = _binary64(
            root_level.get("encoded_binary64"),
            path=(
                f"{path}.correction.corrected_tree.root_level.encoded_binary64"
            ),
        )
        corrected_tree_sha = _sha256(
            corrected_tree.get("tree_sha256"),
            path=f"{path}.correction.corrected_tree.tree_sha256",
        )
        _literal(
            corrected_tree,
            "records_serialized",
            records_expected,
            path=f"{path}.correction.corrected_tree",
        )

        emitted_overlay: tuple[TriangleRecord, ...] | None = None
        emitted_corrected_tree: tuple[CorrectedTreeEdge, ...] | None = None
        if records_expected:
            emitted_overlay = _parse_triangle_array(
                correction.get("overlay_records"),
                path=f"{path}.correction.overlay_records",
                point_count=point_count,
            )
            if any(record.status != GABRIEL_STATUS for record in emitted_overlay):
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.overlay_records must be accepted sources"
                )
            if len(emitted_overlay) != correction_count:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.overlay_records extent mismatch"
                )
            emitted_corrected_tree = _parse_corrected_tree_edges(
                corrected_tree.get("tree_edges"),
                path=f"{path}.correction.corrected_tree.tree_edges",
                point_count=point_count,
            )
            if len(emitted_corrected_tree) != corrected_selected:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_tree.tree_edges extent mismatch"
                )
            if (
                emitted_corrected_tree[-1].level_encoding
                != corrected_root_encoding
                or emitted_corrected_tree[-1].encoded_bits
                != corrected_root_bits
            ):
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_tree root disagrees with records"
                )
            _validate_corrected_distinct_levels(
                emitted_corrected_tree,
                corrected_distinct,
                path=f"{path}.correction.corrected_tree.distinct_level_count",
            )
            if _overlay_sha256(order, emitted_overlay) != overlay_sha:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.overlay_sha256 does not match records"
                )
            if (
                _corrected_tree_sha256(
                    point_count, order, emitted_corrected_tree
                )
                != corrected_tree_sha
            ):
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_tree.tree_sha256 does not match records"
                )
        else:
            if correction.get("overlay_records") is not None:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.overlay_records requires emitted_fixture"
                )
            if corrected_tree.get("tree_edges") is not None:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_tree.tree_edges requires emitted_fixture"
                )

        replay_performed = records_expected
        if records_expected:
            assert emitted_sources is not None
            assert raw_tree_records is not None
            assert emitted_overlay is not None
            assert emitted_corrected_tree is not None
            replay = _replay_surrogate(emitted_sources, raw_tree_records)
            if replay.counts != counts:
                raise DelaunayGuardrailInputError(
                    f"{path}.counts does not match independent record replay"
                )
            if replay.corrected_counts != corrected_counts:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.counts does not match independent record replay"
                )
            if replay.correction_records != emitted_overlay:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.overlay_records does not match replay"
                )
            if replay.corrected_tree != emitted_corrected_tree:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_tree.tree_edges does not match replay"
                )
            if replay.useful_union_count != useful_unions:
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.useful_union_count does not match replay"
                )
            if (
                replay.corrected_postcondition_violation_count
                != corrected_violations
            ):
                raise DelaunayGuardrailInputError(
                    f"{path}.correction postcondition does not match replay"
                )
            if (
                _decision_sha256(
                    order, emitted_sources, replay.decisions, corrected=False
                )
                != decision_sha
            ):
                raise DelaunayGuardrailInputError(
                    f"{path}.decision_sha256 does not match record replay"
                )
            if (
                _decision_sha256(
                    order,
                    emitted_sources,
                    replay.corrected_decisions,
                    corrected=True,
                )
                != corrected_decision_sha
            ):
                raise DelaunayGuardrailInputError(
                    f"{path}.correction.corrected_decision_sha256 does not match replay"
                )
            for name, observed, expected in (
                (
                    "first_failure_witness",
                    first_failure,
                    replay.first_failure_witness,
                ),
                ("first_late_witness", first_late, replay.first_late_witness),
                (
                    "first_unsupported_witness",
                    first_unsupported,
                    replay.first_unsupported_witness,
                ),
            ):
                if observed != expected:
                    raise DelaunayGuardrailInputError(
                        f"{path}.{name} does not match independent replay"
                    )
            for name, observed, expected in (
                (
                    "first_failure_witness",
                    corrected_first_failure,
                    replay.corrected_first_failure_witness,
                ),
                (
                    "first_late_witness",
                    corrected_first_late,
                    replay.corrected_first_late_witness,
                ),
                (
                    "first_unsupported_witness",
                    corrected_first_unsupported,
                    replay.corrected_first_unsupported_witness,
                ),
            ):
                if observed != expected:
                    raise DelaunayGuardrailInputError(
                        f"{path}.correction.{name} does not match independent replay"
                    )

        timings = _mapping(
            record.get("timings_nanoseconds"), path=f"{path}.timings_nanoseconds"
        )
        for key in ("edge_build", "tree_reduction", "deadline_replay_and_overlay"):
            _uint(timings.get(key), path=f"{path}.timings_nanoseconds.{key}")

        raw_regular_all = raw_regular_all and raw_regular
        corrected_regular_all = corrected_regular_all and corrected_regular
        corrected_fail_closed_all = (
            corrected_fail_closed_all and corrected_fail_closed
        )
        summaries.append(
            {
                "order": order,
                "counts": counts,
                "raw_regular_binary64_guardrail_passed": raw_regular,
                "raw_fail_closed_guardrail_passed": raw_fail_closed,
                "corrected_regular_binary64_guardrail_passed": corrected_regular,
                "corrected_fail_closed_guardrail_passed": corrected_fail_closed,
                "record_replay_performed": replay_performed,
                "tree_sha256_recomputed": replay_performed,
                "decision_sha256_recomputed": replay_performed,
                "overlay_sha256_recomputed": replay_performed,
                "corrected_tree_sha256_recomputed": replay_performed,
                "corrected_decision_sha256_recomputed": replay_performed,
                "root_raw_squared_weight": root_weight,
                "corrected_root_level": {
                    "level_encoding": corrected_root_encoding,
                    "encoded_binary64": corrected_root,
                },
            }
        )
    return (
        summaries,
        raw_regular_all,
        corrected_regular_all,
        corrected_fail_closed_all,
    )


def _validate_emitted_fixture(
    root: Mapping[str, object],
    *,
    point_count: int,
    raw_wedge_count: int,
    candidate_count: int,
    ordinary_edge_count: int,
    accepted_count: int,
    ambiguous_count: int,
    support_two_count: int,
    support_three_count: int,
    declared_digests: Mapping[str, str],
) -> tuple[TriangleRecord, ...] | None:
    coverage = _mapping(root.get("coverage"), path="coverage")
    fixture_value = root.get("emitted_fixture")
    necessary_value = coverage.get("necessary_records")
    ambiguous_value = coverage.get("ambiguous_safety_records")
    if fixture_value is None:
        if necessary_value is not None or ambiguous_value is not None:
            raise DelaunayGuardrailInputError(
                "coverage record arrays require emitted_fixture"
            )
        return None
    if point_count > MAX_EMITTED_RECORD_POINTS:
        raise DelaunayGuardrailInputError(
            "emitted_fixture exceeds the producer's 256-point record cap"
        )
    fixture = _mapping(fixture_value, path="emitted_fixture")
    raw_points = _array(fixture.get("points"), path="emitted_fixture.points")
    if len(raw_points) != point_count:
        raise DelaunayGuardrailInputError(
            "emitted_fixture.points disagrees with input.point_count"
        )
    point_bits: list[tuple[int, int, int]] = []
    for point_index, item in enumerate(raw_points):
        path = f"emitted_fixture.points[{point_index}]"
        point = _array(item, path=path)
        if len(point) != 3:
            raise DelaunayGuardrailInputError(
                f"{path} must contain three coordinates"
            )
        bits = tuple(
            _binary64(
                coordinate,
                path=f"{path}[{coordinate_index}]",
                non_negative=False,
            )[1]
            for coordinate_index, coordinate in enumerate(point)
        )
        point_bits.append(bits)  # type: ignore[arg-type]

    raw_edges = _array(
        fixture.get("ordinary_delaunay_edges"),
        path="emitted_fixture.ordinary_delaunay_edges",
    )
    edges: list[tuple[int, int]] = []
    for index, item in enumerate(raw_edges):
        path = f"emitted_fixture.ordinary_delaunay_edges[{index}]"
        edge = _mapping(item, path=path)
        u = _point_id(edge.get("u"), path=f"{path}.u", point_count=point_count)
        v = _point_id(edge.get("v"), path=f"{path}.v", point_count=point_count)
        if u >= v:
            raise DelaunayGuardrailInputError(f"{path} must satisfy u < v")
        edges.append((u, v))
    if edges != sorted(set(edges)):
        raise DelaunayGuardrailInputError(
            "emitted_fixture.ordinary_delaunay_edges must be unique and sorted"
        )
    if len(edges) != ordinary_edge_count:
        raise DelaunayGuardrailInputError(
            "emitted fixture edge count disagrees with delaunay summary"
        )
    degrees = [0] * point_count
    adjacency = [set() for _ in range(point_count)]
    edge_set = set(edges)
    for u, v in edges:
        degrees[u] += 1
        degrees[v] += 1
        adjacency[u].add(v)
        adjacency[v].add(u)
    if sum(degree * (degree - 1) // 2 for degree in degrees) != raw_wedge_count:
        raise DelaunayGuardrailInputError(
            "emitted fixture raw wedge count disagrees with summary"
        )
    reconstructed_candidate_count = sum(
        sum(
            pair in edge_set
            for pair in ((a, b), (a, c), (b, c))
        )
        >= 2
        for a, b, c in combinations(range(point_count), 3)
    )
    if reconstructed_candidate_count != candidate_count:
        raise DelaunayGuardrailInputError(
            "emitted fixture canonical candidate count disagrees with summary"
        )

    sources = _parse_triangle_array(
        fixture.get("source_records"),
        path="emitted_fixture.source_records",
        point_count=point_count,
    )
    necessary = _parse_triangle_array(
        necessary_value,
        path="coverage.necessary_records",
        point_count=point_count,
    )
    ambiguous = _parse_triangle_array(
        ambiguous_value,
        path="coverage.ambiguous_safety_records",
        point_count=point_count,
    )
    accepted = tuple(source for source in sources if source.status == GABRIEL_STATUS)
    source_ambiguous = tuple(
        source for source in sources if source.status == AMBIGUOUS_STATUS
    )
    if necessary != accepted:
        raise DelaunayGuardrailInputError(
            "coverage.necessary_records must equal all accepted sources in this schema"
        )
    if ambiguous != source_ambiguous:
        raise DelaunayGuardrailInputError(
            "coverage.ambiguous_safety_records differs from source ambiguities"
        )
    if len(accepted) != accepted_count or len(ambiguous) != ambiguous_count:
        raise DelaunayGuardrailInputError(
            "emitted source status counts disagree with classification"
        )
    if sum(source.support_cardinality == 2 for source in accepted) != support_two_count:
        raise DelaunayGuardrailInputError(
            "emitted source support-two count disagrees with coverage"
        )
    if sum(source.support_cardinality == 3 for source in accepted) != support_three_count:
        raise DelaunayGuardrailInputError(
            "emitted source support-three count disagrees with coverage"
        )
    for index, source in enumerate(sources):
        if sum(pair in edge_set for pair in combinations(source.point_ids, 2)) < 2:
            raise DelaunayGuardrailInputError(
                f"emitted_fixture.source_records[{index}] is not a Delaunay wedge"
            )

    point_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/point-cloud/v1/sha256/"
    )
    _sha256_word(point_builder, point_count)
    for index, bits in enumerate(point_bits):
        _sha256_word(point_builder, index)
        for word in bits:
            _sha256_word(point_builder, word)
    point_digest = point_builder.digest()

    edge_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/delaunay-edges/v1/sha256/"
    )
    _sha256_word(edge_builder, len(edges))
    for edge in edges:
        for point_id in edge:
            _sha256_word(edge_builder, point_id)
    edge_digest = edge_builder.digest()

    wedge_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/canonical-wedges/v1/sha256/"
    )
    wedge_builder.update(point_digest)
    wedge_builder.update(edge_digest)
    wedge_builder.update(b"owns_triangle_smallest_Delaunay_center_v1")
    _sha256_word(wedge_builder, raw_wedge_count)
    _sha256_word(wedge_builder, candidate_count)

    accepted_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/accepted/v1/sha256/"
    )
    _sha256_word(accepted_builder, len(accepted))
    for source in accepted:
        _sha256_triangle(accepted_builder, source)

    necessary_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/necessary/v1/sha256/"
    )
    for source in necessary:
        _sha256_triangle(necessary_builder, source)
    necessary_builder.update(b"/count/")
    _sha256_word(necessary_builder, len(necessary))
    necessary_digest = necessary_builder.digest()

    ambiguous_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/ambiguous-safety/v1/sha256/"
    )
    for source in ambiguous:
        _sha256_triangle(ambiguous_builder, source)
    ambiguous_builder.update(b"/count/")
    _sha256_word(ambiguous_builder, len(ambiguous))
    ambiguous_digest = ambiguous_builder.digest()

    composite_builder = hashlib.sha256(
        b"MorseHGP3D/phase15/gabriel-coverage/composite-safety-batch/v1/sha256/"
    )
    composite_builder.update(necessary_digest)
    _sha256_word(composite_builder, len(necessary))
    composite_builder.update(ambiguous_digest)
    _sha256_word(composite_builder, len(ambiguous))

    recomputed = {
        "input.point_cloud_sha256": point_digest.hex(),
        "delaunay.ordinary_edges_sha256": edge_digest.hex(),
        "candidate_universe.canonical_wedge_universe_sha256": wedge_builder.hexdigest(),
        "coverage.accepted_source_sha256": accepted_builder.hexdigest(),
        "coverage.necessary_accepted_sha256": necessary_digest.hex(),
        "coverage.ambiguous_safety_sha256": ambiguous_digest.hex(),
        "coverage.explicit_safety_batch_sha256": composite_builder.hexdigest(),
    }
    for key, observed in declared_digests.items():
        if recomputed[key] != observed:
            raise DelaunayGuardrailInputError(
                f"{key} does not match emitted fixture recomputation"
            )
    return sources


def _reject_unsupported_massive_recertification_claims(
    value: object, *, records_present: bool, path: str = "root"
) -> None:
    if records_present:
        return
    if isinstance(value, Mapping):
        for key, child in value.items():
            lowered = str(key).lower()
            recertification_key = (
                "recertif" in lowered
                or (
                    "independent" in lowered
                    and any(token in lowered for token in ("replay", "recomput", "verif"))
                )
                or (
                    "record" in lowered
                    and any(token in lowered for token in ("replayed", "recomputed"))
                )
            )
            if recertification_key and child is True:
                raise DelaunayGuardrailInputError(
                    f"{path}.{key} claims record recertification without records"
                )
            if lowered in {"validation_mode", "digest_validation_mode"} and isinstance(child, str):
                claim = child.lower()
                if any(token in claim for token in ("recertified", "recomputed_from_records", "independent_replay")):
                    raise DelaunayGuardrailInputError(
                        f"{path}.{key} claims record recertification without records"
                    )
            _reject_unsupported_massive_recertification_claims(
                child, records_present=False, path=f"{path}.{key}"
            )
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_unsupported_massive_recertification_claims(
                child, records_present=False, path=f"{path}[{index}]"
            )


def _validate_public_and_exact_claims(value: object, *, path: str = "root") -> None:
    if isinstance(value, Mapping):
        for key, child in value.items():
            child_path = f"{path}.{key}"
            if key == "public_status" and child != "not_claimed":
                raise DelaunayGuardrailInputError(
                    f"{child_path} must remain 'not_claimed'"
                )
            if key in {
                "exact_Gamma2_claimed",
                "exact_Gabriel_classification_claimed",
                "Gamma2_exactness_claimed",
                "Geogram_SoS_topology_exactly_recertified",
                "exact_morse_hierarchy_claimed",
                "public_status_claimed",
            } and child is not False:
                raise DelaunayGuardrailInputError(
                    f"{child_path} must remain false"
                )
            _validate_public_and_exact_claims(child, path=child_path)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _validate_public_and_exact_claims(child, path=f"{path}[{index}]")


def analyze_delaunay_guardrails(
    payload: object,
    *,
    source_artifact_sha256: str | None = None,
    source_artifact_byte_count: int | None = None,
) -> dict[str, object]:
    """Validate one decoded outer sidecar and return its structural report."""

    if (source_artifact_sha256 is None) != (source_artifact_byte_count is None):
        raise DelaunayGuardrailInputError(
            "source artifact SHA-256 and byte count must be supplied together"
        )
    source_artifact: dict[str, object] | None = None
    if source_artifact_sha256 is not None:
        source_artifact = {
            "sha256": _sha256(
                source_artifact_sha256, path="source_artifact.sha256"
            ),
            "byte_count": _uint(
                source_artifact_byte_count,
                path="source_artifact.byte_count",
                positive=True,
            ),
        }

    root = _mapping(payload, path="root")
    for key, expected in (
        ("schema", INPUT_SCHEMA),
        ("phase", "15"),
        ("backend", OUTER_BACKEND),
        ("profile", "hgp_reduced"),
        ("mode", OUTER_MODE),
        ("deployment_status", "diagnostic_sidecar"),
        ("public_status", "not_claimed"),
        (
            "criterion",
            "triangle_explicit_or_three_facets_connected_at_strictly_lower_level",
        ),
        ("coverage_reduction_mode", OUTER_COVERAGE_MODE),
    ):
        _literal(root, key, expected, path="root")
    git_sha = root.get("git_sha")
    if not isinstance(git_sha, str) or GIT_SHA_PATTERN.fullmatch(git_sha) is None:
        raise DelaunayGuardrailInputError(
            "root.git_sha must be 40 lowercase hexadecimal digits"
        )

    proof = _mapping(root.get("proof_scope"), path="proof_scope")
    for key, expected in (
        ("general_position_required", True),
        ("complete_Voronoi_nerve_one_skeleton_required", True),
        ("Geogram_SoS_topology_exactly_recertified", False),
        ("every_regular_Gabriel_triangle_has_at_least_two_Delaunay_edges", True),
        ("support_cardinality_two_and_three_covered", True),
        ("exact_Gamma2_claimed", False),
        ("exact_Gabriel_classification_claimed", False),
    ):
        _literal(proof, key, expected, path="proof_scope")

    architecture = _mapping(root.get("architecture"), path="architecture")
    for key, expected in (
        ("edge_times_point_product_enumerated", False),
        ("higher_order_Delaunay_mosaic_materialized", False),
        ("ordinary_Delaunay_edges_only", True),
        ("canonical_wedges_enumerated_in_bounded_GPU_chunks", True),
        ("global_restricted_Gamma2_arena_materialized", False),
        ("global_Gabriel_proposal_arena_materialized_for_level_sort", True),
        ("same_level_candidates_can_certify_each_other", False),
    ):
        _literal(architecture, key, expected, path="architecture")

    input_record = _mapping(root.get("input"), path="input")
    point_count = _uint(
        input_record.get("point_count"), path="input.point_count", positive=True
    )
    if point_count < 4:
        raise DelaunayGuardrailInputError("input.point_count must be at least four")
    _uint(input_record.get("seed"), path="input.seed")
    workers_requested = _uint(
        input_record.get("cpu_workers_requested"),
        path="input.cpu_workers_requested",
        positive=True,
    )
    workers_used = _uint(
        input_record.get("cpu_workers_used"),
        path="input.cpu_workers_used",
        positive=True,
    )
    if workers_used > workers_requested:
        raise DelaunayGuardrailInputError(
            "input.cpu_workers_used exceeds cpu_workers_requested"
        )
    point_cloud_sha256 = _sha256(
        input_record.get("point_cloud_sha256"), path="input.point_cloud_sha256"
    )

    delaunay = _mapping(root.get("delaunay"), path="delaunay")
    _literal(delaunay, "engine", "PDEL", path="delaunay")
    _uint(delaunay.get("cell_count"), path="delaunay.cell_count", positive=True)
    ordinary_edge_count = _uint(
        delaunay.get("ordinary_edge_count"),
        path="delaunay.ordinary_edge_count",
        positive=True,
    )
    if ordinary_edge_count > point_count * (point_count - 1) // 2:
        raise DelaunayGuardrailInputError(
            "delaunay.ordinary_edge_count exceeds the simple-graph bound"
        )
    maximum_degree = _uint(
        delaunay.get("maximum_vertex_degree"),
        path="delaunay.maximum_vertex_degree",
        positive=True,
    )
    if maximum_degree >= point_count:
        raise DelaunayGuardrailInputError(
            "delaunay.maximum_vertex_degree exceeds the simple-graph bound"
        )
    ordinary_edges_sha256 = _sha256(
        delaunay.get("ordinary_edges_sha256"),
        path="delaunay.ordinary_edges_sha256",
    )

    universe = _mapping(root.get("candidate_universe"), path="candidate_universe")
    _literal(
        universe,
        "generator",
        "canonical_Delaunay_CSR_wedges",
        path="candidate_universe",
    )
    raw_wedge_count = _uint(
        universe.get("raw_wedge_count"),
        path="candidate_universe.raw_wedge_count",
        positive=True,
    )
    candidate_count = _uint(
        universe.get("canonical_candidate_count"),
        path="candidate_universe.canonical_candidate_count",
        positive=True,
    )
    if raw_wedge_count < candidate_count:
        raise DelaunayGuardrailInputError(
            "candidate_universe.raw_wedge_count is below canonical candidates"
        )
    wedge_sha256 = _sha256(
        universe.get("canonical_wedge_universe_sha256"),
        path="candidate_universe.canonical_wedge_universe_sha256",
    )
    chunk_count = _uint(
        universe.get("chunk_count"),
        path="candidate_universe.chunk_count",
        positive=True,
    )
    chunk_vertices = _uint(
        universe.get("triangle_chunk_vertices"),
        path="candidate_universe.triangle_chunk_vertices",
        positive=True,
    )
    if chunk_count != (point_count + chunk_vertices - 1) // chunk_vertices:
        raise DelaunayGuardrailInputError(
            "candidate_universe.chunk_count disagrees with its chunk size"
        )
    maximum_chunk_candidates = _uint(
        universe.get("maximum_chunk_candidate_count"),
        path="candidate_universe.maximum_chunk_candidate_count",
        positive=True,
    )
    if maximum_chunk_candidates > candidate_count:
        raise DelaunayGuardrailInputError(
            "maximum chunk candidate count exceeds the universe"
        )

    classification = _mapping(root.get("classification"), path="classification")
    blocked = _uint(
        classification.get("blocked_count"), path="classification.blocked_count"
    )
    accepted = _uint(
        classification.get("gabriel_binary64_count"),
        path="classification.gabriel_binary64_count",
    )
    ambiguous = _uint(
        classification.get("ambiguous_safety_count"),
        path="classification.ambiguous_safety_count",
    )
    invalid = _uint(
        classification.get("invalid_count"), path="classification.invalid_count"
    )
    _uint(
        classification.get("visited_cell_count"),
        path="classification.visited_cell_count",
    )
    _uint(
        classification.get("tested_point_count"),
        path="classification.tested_point_count",
    )
    if blocked + accepted + ambiguous + invalid != candidate_count:
        raise DelaunayGuardrailInputError(
            "classification counters do not partition candidates"
        )
    _literal(classification, "status_partition_closed", True, path="classification")
    if invalid != 0:
        raise DelaunayGuardrailInputError(
            "a completed cross-variant binary64 gate must have invalid_count=0"
        )

    coverage = _mapping(root.get("coverage"), path="coverage")
    source_count = _uint(
        coverage.get("source_triangle_count"),
        path="coverage.source_triangle_count",
    )
    support_two = _uint(
        coverage.get("support_two_count"), path="coverage.support_two_count"
    )
    support_three = _uint(
        coverage.get("support_three_count"), path="coverage.support_three_count"
    )
    explicit_accepted = _uint(
        coverage.get("explicit_necessary_accepted_count"),
        path="coverage.explicit_necessary_accepted_count",
    )
    strict_lower = _uint(
        coverage.get("strictly_lower_connected_count"),
        path="coverage.strictly_lower_connected_count",
    )
    if source_count != accepted or support_two + support_three != accepted:
        raise DelaunayGuardrailInputError(
            "coverage source/support counts disagree with accepted classification"
        )
    if explicit_accepted != accepted or strict_lower != 0:
        raise DelaunayGuardrailInputError(
            "new outer schema must keep every accepted Gabriel source explicit"
        )
    if _uint(
        coverage.get("coverage_violation_count"),
        path="coverage.coverage_violation_count",
    ) != 0:
        raise DelaunayGuardrailInputError(
            "coverage.coverage_violation_count must be zero"
        )
    _literal(coverage, "accepted_partition_closed", True, path="coverage")
    _literal(coverage, "binary64_gate_passed", True, path="coverage")
    accepted_sha256 = _sha256(
        coverage.get("accepted_source_sha256"),
        path="coverage.accepted_source_sha256",
    )
    necessary_sha256 = _sha256(
        coverage.get("necessary_accepted_sha256"),
        path="coverage.necessary_accepted_sha256",
    )
    ambiguous_sha256 = _sha256(
        coverage.get("ambiguous_safety_sha256"),
        path="coverage.ambiguous_safety_sha256",
    )
    safety_sha256 = _sha256(
        coverage.get("explicit_safety_batch_sha256"),
        path="coverage.explicit_safety_batch_sha256",
    )
    safety_count = _uint(
        coverage.get("explicit_safety_batch_count"),
        path="coverage.explicit_safety_batch_count",
    )
    if safety_count != accepted + ambiguous:
        raise DelaunayGuardrailInputError(
            "coverage safety batch must equal accepted + ambiguous"
        )
    _literal(
        coverage,
        "explicit_safety_batch_semantics",
        "necessary_accepted_plus_all_ambiguous",
        path="coverage",
    )

    emitted_sources = _validate_emitted_fixture(
        root,
        point_count=point_count,
        raw_wedge_count=raw_wedge_count,
        candidate_count=candidate_count,
        ordinary_edge_count=ordinary_edge_count,
        accepted_count=accepted,
        ambiguous_count=ambiguous,
        support_two_count=support_two,
        support_three_count=support_three,
        declared_digests={
            "input.point_cloud_sha256": point_cloud_sha256,
            "delaunay.ordinary_edges_sha256": ordinary_edges_sha256,
            "candidate_universe.canonical_wedge_universe_sha256": wedge_sha256,
            "coverage.accepted_source_sha256": accepted_sha256,
            "coverage.necessary_accepted_sha256": necessary_sha256,
            "coverage.ambiguous_safety_sha256": ambiguous_sha256,
            "coverage.explicit_safety_batch_sha256": safety_sha256,
        },
    )

    guardrails = _mapping(root.get("variant_guardrails"), path="variant_guardrails")
    for key, expected in (
        ("schema", INNER_SCHEMA),
        ("criterion", "gabriel_fusion_deadline_v1"),
        ("boundary", "closed_after_complete_candidate_plateau"),
        ("early_connections_accepted", True),
        (
            "scientific_scope",
            "conditional_binary64_guardrail_not_Gamma2_exactness",
        ),
    ):
        _literal(guardrails, key, expected, path="variant_guardrails")
    source = _mapping(guardrails.get("source"), path="variant_guardrails.source")
    source_total = _uint(
        source.get("source_triangle_count"),
        path="variant_guardrails.source.source_triangle_count",
    )
    source_accepted = _uint(
        source.get("accepted_gabriel_binary64_count"),
        path="variant_guardrails.source.accepted_gabriel_binary64_count",
    )
    source_ambiguous = _uint(
        source.get("unsupported_ambiguous_count"),
        path="variant_guardrails.source.unsupported_ambiguous_count",
    )
    if (source_total, source_accepted, source_ambiguous) != (
        accepted + ambiguous,
        accepted,
        ambiguous,
    ):
        raise DelaunayGuardrailInputError(
            "variant_guardrails.source disagrees with outer classification"
        )
    _literal(
        source,
        "level_convention",
        SOURCE_LEVEL_CONVENTION,
        path="variant_guardrails.source",
    )
    if _sha256(
        source.get("point_cloud_sha256"),
        path="variant_guardrails.source.point_cloud_sha256",
    ) != point_cloud_sha256:
        raise DelaunayGuardrailInputError(
            "variant guardrails are not anchored to the outer point cloud"
        )
    if _sha256(
        source.get("accepted_source_sha256"),
        path="variant_guardrails.source.accepted_source_sha256",
    ) != accepted_sha256:
        raise DelaunayGuardrailInputError(
            "variant guardrails are not anchored to the outer source stream"
        )

    direct_summaries, direct_fail_closed = _validate_direct_variants(
        guardrails.get("direct_facet_variants"),
        source_count=source_total,
        accepted_count=accepted,
        ambiguous_count=ambiguous,
        accepted_sha256=accepted_sha256,
        ambiguous_sha256=ambiguous_sha256,
        point_count=point_count,
        emitted_sources=emitted_sources,
    )

    surrogate = _mapping(guardrails.get("surrogate"), path="variant_guardrails.surrogate")
    _literal(
        surrogate,
        "representation",
        "point_weighted_spanning_tree_binary64_v1",
        path="variant_guardrails.surrogate",
    )
    adapter = _mapping(
        surrogate.get("adapter"), path="variant_guardrails.surrogate.adapter"
    )
    for key, expected in (
        ("adapter_id", POINT_LIFT_ADAPTER),
        ("component_lift", POINT_LIFT_SEMANTICS),
        ("input_level_convention", RAW_TREE_LEVEL_CONVENTION),
        ("output_level_convention", SOURCE_LEVEL_CONVENTION),
        ("level_conversion", LEVEL_CONVERSION),
        ("exact_dyadic_comparison_of_binary64_recipe_outputs", True),
        ("native_k2_facet_domain", False),
        ("Gamma2_exactness_claimed", False),
    ):
        _literal(adapter, key, expected, path="variant_guardrails.surrogate.adapter")
    mapping_sha = _sha256(
        surrogate.get("canonical_to_source_mapping_sha256"),
        path="variant_guardrails.surrogate.canonical_to_source_mapping_sha256",
    )
    transcript_sha = _sha256(
        surrogate.get("remapped_top_k_transcript_sha256"),
        path="variant_guardrails.surrogate.remapped_top_k_transcript_sha256",
    )
    top_k = _mapping(
        surrogate.get("top_k_audit"), path="variant_guardrails.surrogate.top_k_audit"
    )
    _literal(
        top_k,
        "backend",
        "cuda_binary64_lbvh_top_k",
        path="variant_guardrails.surrogate.top_k_audit",
    )
    maximum_order = _uint(
        top_k.get("maximum_order"),
        path="variant_guardrails.surrogate.top_k_audit.maximum_order",
        positive=True,
    )
    if not 2 <= maximum_order <= 10:
        raise DelaunayGuardrailInputError(
            "surrogate top-k maximum_order must be between 2 and 10"
        )
    seed_window = _uint(
        top_k.get("seed_window_radius"),
        path="variant_guardrails.surrogate.top_k_audit.seed_window_radius",
        positive=True,
    )
    if seed_window < maximum_order or seed_window >= point_count:
        raise DelaunayGuardrailInputError(
            "surrogate top-k seed window must satisfy maximum_order <= radius < point_count"
        )
    neighbor_count = _uint(
        top_k.get("neighbor_record_count"),
        path="variant_guardrails.surrogate.top_k_audit.neighbor_record_count",
    )
    if point_count * maximum_order > MAX_U64 or neighbor_count != point_count * maximum_order:
        raise DelaunayGuardrailInputError(
            "surrogate top-k neighbor count must equal n * maximum_order"
        )
    for key, expected in (
        ("point_count", point_count),
        ("certified_node_count", 2 * point_count - 1),
        ("completed_query_count", point_count),
        ("failed_query_count", 0),
    ):
        if _uint(
            top_k.get(key),
            path=f"variant_guardrails.surrogate.top_k_audit.{key}",
        ) != expected:
            raise DelaunayGuardrailInputError(
                f"variant_guardrails.surrogate.top_k_audit.{key} is inconsistent"
            )
    _uint(
        top_k.get("source_snapshot_epoch"),
        path="variant_guardrails.surrogate.top_k_audit.source_snapshot_epoch",
        positive=True,
    )
    for key, expected in (
        ("complete_query_coverage", True),
        ("no_candidate_truncation", True),
        ("source_morton_permutation_validated", True),
        ("host_transcript_structure_validated", True),
        ("traversal_lease_adopted", True),
        ("fixed_round_to_nearest_distance_recipe_requested", True),
        ("directed_round_down_aabb_recipe_requested", True),
        ("strict_prune_requested", True),
        ("stackless_postorder_traversal_requested", True),
        ("higher_order_delaunay_mosaic_materialized", False),
        ("global_pair_matrix_materialized", False),
        ("exact_morse_hierarchy_claimed", False),
        ("public_status_claimed", False),
    ):
        _literal(
            top_k,
            key,
            expected,
            path="variant_guardrails.surrogate.top_k_audit",
        )
    for key in (
        "persistent_input_device_byte_capacity",
        "transient_output_device_byte_capacity",
    ):
        _uint(
            top_k.get(key),
            path=f"variant_guardrails.surrogate.top_k_audit.{key}",
            positive=True,
        )
    if _uint(
        top_k.get("invalid_aabb_bound_descent_count"),
        path=(
            "variant_guardrails.surrogate.top_k_audit."
            "invalid_aabb_bound_descent_count"
        ),
    ) != 0:
        raise DelaunayGuardrailInputError(
            "surrogate top-k audit reports an invalid AABB-bound descent"
        )
    for key in ("kernel_nanoseconds", "launcher_wall_nanoseconds"):
        _uint(
            top_k.get(key),
            path=f"variant_guardrails.surrogate.top_k_audit.{key}",
        )
    _nonempty_string(
        top_k.get("device_name"),
        path="variant_guardrails.surrogate.top_k_audit.device_name",
    )

    order_summaries, raw_regular, corrected_regular, corrected_fail_closed = (
        _validate_surrogate_orders(
            surrogate.get("orders"),
            maximum_order=maximum_order,
            point_count=point_count,
            source_count=source_total,
            accepted_count=accepted,
            ambiguous_count=ambiguous,
            emitted_sources=emitted_sources,
        )
    )
    _literal(
        guardrails,
        "raw_surrogate_regular_guardrail_passed",
        raw_regular,
        path="variant_guardrails",
    )
    _literal(
        guardrails,
        "corrected_surrogate_regular_guardrail_passed",
        corrected_regular,
        path="variant_guardrails",
    )
    _literal(
        guardrails,
        "corrected_surrogate_fail_closed_guardrail_passed",
        corrected_fail_closed,
        path="variant_guardrails",
    )

    inner_architecture = _mapping(
        guardrails.get("architecture"), path="variant_guardrails.architecture"
    )
    for key, expected in (
        ("higher_order_Delaunay_mosaic_materialized", False),
        ("global_pair_matrix_materialized", False),
        ("weighted_tree_maximum_query_materialized", False),
        ("orders_built_in_parallel", True),
        ("corrected_merge_tree_materialized_transiently", True),
        ("massive_corrected_tree_records_serialized", False),
        ("massive_overlay_records_materialized", False),
    ):
        _literal(inner_architecture, key, expected, path="variant_guardrails.architecture")
    order_worker_count = _uint(
        inner_architecture.get("order_worker_count"),
        path="variant_guardrails.architecture.order_worker_count",
        positive=True,
    )
    if order_worker_count > maximum_order:
        raise DelaunayGuardrailInputError(
            "variant_guardrails.architecture.order_worker_count exceeds maximum_order"
        )
    inner_timings = _mapping(
        guardrails.get("timings_nanoseconds"),
        path="variant_guardrails.timings_nanoseconds",
    )
    for key in ("canonicalization", "certified_LBVH_build", "top_k_query"):
        _uint(
            inner_timings.get(key),
            path=f"variant_guardrails.timings_nanoseconds.{key}",
        )

    expected_conditional = corrected_regular
    expected_fail_closed = corrected_fail_closed and direct_fail_closed
    _literal(
        coverage,
        "conditional_cross_variant_gate_passed",
        expected_conditional,
        path="coverage",
    )
    allow_conditional = _boolean(
        coverage.get("allow_conditional_guardrail"),
        path="coverage.allow_conditional_guardrail",
    )
    _literal(
        coverage,
        "selected_process_gate_passed",
        expected_conditional if allow_conditional else expected_fail_closed,
        path="coverage",
    )
    _literal(
        coverage,
        "fail_closed_cross_variant_gate_passed",
        expected_fail_closed,
        path="coverage",
    )

    gpu = _mapping(root.get("gpu"), path="gpu")
    _nonempty_string(gpu.get("device_name"), path="gpu.device_name")
    for key in (
        "multiprocessor_count",
        "grid_resolution",
        "maximum_cell_occupancy",
        "persistent_device_bytes",
        "maximum_chunk_device_bytes",
        "maximum_accounted_live_device_bytes",
    ):
        _uint(gpu.get(key), path=f"gpu.{key}")
    root_timings = _mapping(root.get("timings_nanoseconds"), path="timings_nanoseconds")
    for key, value in root_timings.items():
        _uint(value, path=f"timings_nanoseconds.{key}")
    if "surrogate_guardrail" not in root_timings or "cold_e2e" not in root_timings:
        raise DelaunayGuardrailInputError(
            "timings_nanoseconds misses the surrogate or cold-e2e timing"
        )

    records_present = emitted_sources is not None
    _reject_unsupported_massive_recertification_claims(
        root, records_present=records_present
    )
    _validate_public_and_exact_claims(root)

    evidence_mode = (
        "independent_binary64_record_replay"
        if records_present
        else "producer_commitments_only"
    )
    return {
        "schema_version": CHECK_SCHEMA,
        "diagnostic_completed": True,
        "phase": "15",
        "backend": "python_bounded_structural_replay",
        "profile": "hgp_reduced",
        "mode": "gabriel_variant_fusion_guardrail_structural_check",
        "public_status": "not_claimed",
        "source_artifact": source_artifact,
        "point_count": point_count,
        "point_cloud_sha256": point_cloud_sha256,
        "accepted_source_sha256": accepted_sha256,
        "source_counts": {
            "source_triangle_count": source_total,
            "accepted_gabriel_binary64_count": accepted,
            "unsupported_ambiguous_count": ambiguous,
        },
        "same_run_anchors": {
            "outer_and_guardrail_point_cloud_sha256_match": True,
            "outer_and_guardrail_accepted_source_sha256_match": True,
            "canonical_to_source_mapping_sha256": mapping_sha,
            "remapped_top_k_transcript_sha256": transcript_sha,
        },
        "evidence": {
            "mode": evidence_mode,
            "emitted_records_present": records_present,
            "outer_seven_sha256_recomputed": records_present,
            "surrogate_tree_decision_overlay_and_corrected_tree_replayed": records_present,
            "producer_commitments_not_independently_recomputable_without_records": (
                not records_present
            ),
            "independent_recertification_performed": records_present,
            "direct_full_variant_first_connection_replayed": False,
        },
        "direct_facet_variants": direct_summaries,
        "surrogate_orders": order_summaries,
        "guardrail_status": {
            "raw_surrogate_regular_guardrail_passed": raw_regular,
            "corrected_surrogate_regular_guardrail_passed": corrected_regular,
            "corrected_surrogate_fail_closed_guardrail_passed": corrected_fail_closed,
            "direct_source_inclusion_fail_closed_guardrail_passed": direct_fail_closed,
            "conditional_cross_variant_gate_passed": expected_conditional,
            "fail_closed_cross_variant_gate_passed": expected_fail_closed,
            "allow_conditional_guardrail": allow_conditional,
            "selected_process_gate_passed": (
                expected_conditional
                if allow_conditional
                else expected_fail_closed
            ),
        },
        "architecture_invariants": {
            "higher_order_delaunay_absent": True,
            "global_pair_matrix_absent": True,
            "global_restricted_gamma2_absent": True,
            "orders_built_in_parallel": True,
            "order_worker_count": order_worker_count,
            "corrected_merge_tree_applied": True,
        },
        "scientific_scope": (
            "conditional_binary64_unilateral_Gabriel_deadline_guardrail_"
            "not_Gamma2_or_Morse_exactness"
        ),
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "diagnostic",
        nargs="?",
        default="-",
        help="outer guardrail JSON path, or '-' / omitted for stdin",
    )
    parser.add_argument("--pretty", action="store_true", help="indent output JSON")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    try:
        if arguments.diagnostic == "-":
            raw_artifact = sys.stdin.buffer.read()
        else:
            raw_artifact = Path(arguments.diagnostic).read_bytes()
        payload = json.loads(raw_artifact)
        report = analyze_delaunay_guardrails(
            payload,
            source_artifact_sha256=hashlib.sha256(raw_artifact).hexdigest(),
            source_artifact_byte_count=len(raw_artifact),
        )
    except (
        DelaunayGuardrailInputError,
        json.JSONDecodeError,
        OSError,
        UnicodeDecodeError,
    ) as error:
        json.dump(
            {
                "schema_version": CHECK_SCHEMA,
                "diagnostic_completed": False,
                "error": f"{type(error).__name__}: {error}",
            },
            sys.stderr,
            sort_keys=True,
        )
        sys.stderr.write("\n")
        return 2
    json.dump(
        report,
        sys.stdout,
        indent=2 if arguments.pretty else None,
        sort_keys=True,
    )
    sys.stdout.write("\n")
    return (
        0
        if report["guardrail_status"]["selected_process_gate_passed"]
        else 1
    )


if __name__ == "__main__":
    raise SystemExit(main())
