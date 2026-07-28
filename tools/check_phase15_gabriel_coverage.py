#!/usr/bin/env python3
"""Validate a bounded Phase 15 Delaunay/Gabriel coverage sidecar.

The massive producer remains a binary64 diagnostic.  This checker validates
its structural contract and, when ``emitted_fixture`` is present, replays the
reported Gabriel records with an independent plateau DSU.  A record may be
declared strictly-lower connected only from the DSU snapshot preceding its
entire equal-level plateau.
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
from typing import Hashable, Iterable, Mapping, Protocol, Sequence

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from reference.morsehgp3d_oracle.gamma import build_gamma_filtration  # noqa: E402
from tools.check_phase14_geogram_low_order import (  # noqa: E402
    DiagnosticInputError as Phase14DiagnosticInputError,
    _enumerate_exact_ordinary_delaunay,
)

INPUT_SCHEMA = "morsehgp3d.phase15_delaunay_gabriel_coverage.v1"
CHECK_SCHEMA = "morsehgp3d.phase15_delaunay_gabriel_coverage_check.v2"
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
GIT_SHA_PATTERN = re.compile(r"[0-9a-f]{40}")
GABRIEL_STATUS = "gabriel_binary64"
AMBIGUOUS_STATUS = "ambiguous_requires_cpu_recertification"
MAX_U64 = (1 << 64) - 1
MAX_EMITTED_FIXTURE_POINTS = 14


class CoverageInputError(ValueError):
    """The JSON report violates the Phase 15 coverage contract."""


class _HashBuilder(Protocol):
    def update(self, data: bytes) -> None: ...

    def digest(self) -> bytes: ...

    def hexdigest(self) -> str: ...


@dataclass(frozen=True)
class TriangleRecord:
    point_ids: tuple[int, int, int]
    squared_level: Fraction
    squared_level_bits: int
    status: str
    support_cardinality: int

    @property
    def sort_key(self) -> tuple[Fraction, int, int, int]:
        return (self.squared_level, *self.point_ids)

    @property
    def facets(self) -> tuple[tuple[int, int], ...]:
        a, b, c = self.point_ids
        return ((a, b), (a, c), (b, c))


@dataclass(frozen=True)
class _FixtureMaterial:
    points: tuple[tuple[Fraction, Fraction, Fraction], ...]
    point_bits: tuple[tuple[int, int, int], ...]
    edges: tuple[tuple[int, int], ...]
    source: tuple[TriangleRecord, ...]
    necessary: tuple[TriangleRecord, ...]
    ambiguous: tuple[TriangleRecord, ...]


class _DisjointSet:
    def __init__(self, values: Iterable[Hashable]) -> None:
        self.parent = {value: value for value in values}

    def find(self, value: Hashable) -> Hashable:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: Hashable, right: Hashable) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if repr(left_root) < repr(right_root):
            self.parent[right_root] = left_root
        else:
            self.parent[left_root] = right_root


def _mapping(value: object, *, path: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        raise CoverageInputError(f"{path} must be an object")
    if any(not isinstance(key, str) for key in value):
        raise CoverageInputError(f"{path} keys must be strings")
    return value  # type: ignore[return-value]


def _array(value: object, *, path: str) -> list[object]:
    if not isinstance(value, list):
        raise CoverageInputError(f"{path} must be an array")
    return value


def _uint(value: object, *, path: str, positive: bool = False) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CoverageInputError(f"{path} must be a non-negative integer")
    if value < 0 or (positive and value == 0):
        qualifier = "positive" if positive else "non-negative"
        raise CoverageInputError(f"{path} must be {qualifier}")
    if value > MAX_U64:
        raise CoverageInputError(f"{path} must fit in an unsigned 64-bit integer")
    return value


def _literal(record: Mapping[str, object], key: str, expected: object, path: str) -> None:
    observed = record.get(key)
    if type(observed) is not type(expected) or observed != expected:
        raise CoverageInputError(f"{path}.{key} must be {expected!r}")


def _sha256(value: object, *, path: str) -> str:
    if not isinstance(value, str) or SHA256_PATTERN.fullmatch(value) is None:
        raise CoverageInputError(f"{path} must be 64 lowercase hexadecimal digits")
    return value


def _binary64(value: object, *, path: str, non_negative: bool = False) -> tuple[Fraction, int]:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise CoverageInputError(f"{path} must be a finite non-negative number")
    try:
        binary64 = float(value)
    except OverflowError as error:
        raise CoverageInputError(f"{path} must be finite binary64") from error
    if not math.isfinite(binary64):
        raise CoverageInputError(f"{path} must be finite binary64")
    if non_negative and binary64 < 0:
        raise CoverageInputError(f"{path} must be non-negative")
    bits = struct.unpack(">Q", struct.pack(">d", binary64))[0]
    return Fraction.from_float(binary64), bits


def _point_id(value: object, *, path: str, point_count: int) -> int:
    result = _uint(value, path=path)
    if result >= point_count:
        raise CoverageInputError(f"{path} lies outside 0..{point_count - 1}")
    return result


def _parse_record(value: object, *, path: str, point_count: int) -> TriangleRecord:
    record = _mapping(value, path=path)
    point_ids = tuple(
        _point_id(record.get(key), path=f"{path}.{key}", point_count=point_count)
        for key in ("a", "b", "c")
    )
    if not point_ids[0] < point_ids[1] < point_ids[2]:
        raise CoverageInputError(f"{path} must satisfy a < b < c")
    status = record.get("status")
    if status not in {GABRIEL_STATUS, AMBIGUOUS_STATUS}:
        raise CoverageInputError(f"{path}.status is not a retained coverage status")
    support = _uint(record.get("support_cardinality"), path=f"{path}.support_cardinality")
    if support not in {2, 3}:
        raise CoverageInputError(f"{path}.support_cardinality must be 2 or 3")
    squared_level, squared_level_bits = _binary64(
        record.get("squared_level"),
        path=f"{path}.squared_level",
        non_negative=True,
    )
    return TriangleRecord(
        point_ids=point_ids,
        squared_level=squared_level,
        squared_level_bits=squared_level_bits,
        status=status,
        support_cardinality=support,
    )


def _parse_record_array(
    value: object, *, path: str, point_count: int
) -> tuple[TriangleRecord, ...]:
    records = tuple(
        _parse_record(item, path=f"{path}[{index}]", point_count=point_count)
        for index, item in enumerate(_array(value, path=path))
    )
    if any(
        records[index].sort_key < records[index - 1].sort_key
        for index in range(1, len(records))
    ):
        raise CoverageInputError(f"{path} must be sorted by level then point IDs")
    identities = [record.point_ids for record in records]
    if len(set(identities)) != len(identities):
        raise CoverageInputError(f"{path} contains duplicate triangle identities")
    return records


def _parse_fixture_material(
    fixture_value: object,
    *,
    point_count: int,
    necessary_value: object,
    ambiguous_value: object,
) -> _FixtureMaterial:
    fixture = _mapping(fixture_value, path="emitted_fixture")
    points = _array(fixture.get("points"), path="emitted_fixture.points")
    if len(points) != point_count:
        raise CoverageInputError("emitted_fixture.points does not match input.point_count")
    if point_count > MAX_EMITTED_FIXTURE_POINTS:
        raise CoverageInputError(
            "emitted_fixture exceeds the 14-point exact-oracle replay cap"
        )
    exact_points: list[tuple[Fraction, Fraction, Fraction]] = []
    point_bits: list[tuple[int, int, int]] = []
    for point_index, point_value in enumerate(points):
        path = f"emitted_fixture.points[{point_index}]"
        point = _array(point_value, path=path)
        if len(point) != 3:
            raise CoverageInputError(f"{path} must contain exactly three coordinates")
        coordinates: list[Fraction] = []
        coordinate_bits: list[int] = []
        for coordinate_index, coordinate in enumerate(point):
            exact, bits = _binary64(coordinate, path=f"{path}[{coordinate_index}]")
            coordinates.append(exact)
            coordinate_bits.append(bits)
        exact_points.append((coordinates[0], coordinates[1], coordinates[2]))
        point_bits.append((coordinate_bits[0], coordinate_bits[1], coordinate_bits[2]))

    edge_values = _array(
        fixture.get("ordinary_delaunay_edges"),
        path="emitted_fixture.ordinary_delaunay_edges",
    )
    edges: list[tuple[int, int]] = []
    for index, value in enumerate(edge_values):
        path = f"emitted_fixture.ordinary_delaunay_edges[{index}]"
        edge = _mapping(value, path=path)
        u = _point_id(edge.get("u"), path=f"{path}.u", point_count=point_count)
        v = _point_id(edge.get("v"), path=f"{path}.v", point_count=point_count)
        if u >= v:
            raise CoverageInputError(f"{path} must satisfy u < v")
        edges.append((u, v))
    if edges != sorted(set(edges)):
        raise CoverageInputError(
            "emitted_fixture.ordinary_delaunay_edges must be unique and sorted"
        )
    source = _parse_record_array(
        fixture.get("source_records"),
        path="emitted_fixture.source_records",
        point_count=point_count,
    )
    reported_necessary = _parse_record_array(
        necessary_value,
        path="coverage.necessary_records",
        point_count=point_count,
    )
    reported_ambiguous = _parse_record_array(
        ambiguous_value,
        path="coverage.ambiguous_safety_records",
        point_count=point_count,
    )

    return _FixtureMaterial(
        points=tuple(exact_points),
        point_bits=tuple(point_bits),
        edges=tuple(edges),
        source=source,
        necessary=reported_necessary,
        ambiguous=reported_ambiguous,
    )


def _sha256_builder(domain: bytes) -> _HashBuilder:
    return hashlib.sha256(domain)


def _sha256_word(builder: _HashBuilder, value: int) -> None:
    builder.update(struct.pack(">Q", value))


def _sha256_triangle(builder: _HashBuilder, record: TriangleRecord) -> None:
    _sha256_word(builder, record.squared_level_bits)
    for point_id in record.point_ids:
        _sha256_word(builder, point_id)
    _sha256_word(builder, record.support_cardinality)


def _fixture_counts(material: _FixtureMaterial) -> dict[str, int]:
    degrees = [0] * len(material.point_bits)
    edge_set = set(material.edges)
    for u, v in material.edges:
        degrees[u] += 1
        degrees[v] += 1
    raw_wedge_count = sum(degree * (degree - 1) // 2 for degree in degrees)
    canonical_candidate_count = sum(
        sum(edge in edge_set for edge in ((a, b), (a, c), (b, c))) >= 2
        for a, b, c in combinations(range(len(material.point_bits)), 3)
    )
    accepted = tuple(
        record for record in material.source if record.status == GABRIEL_STATUS
    )
    ambiguous = tuple(
        record for record in material.source if record.status == AMBIGUOUS_STATUS
    )
    return {
        "ordinary_delaunay_edge_count": len(material.edges),
        "raw_wedge_count": raw_wedge_count,
        "canonical_candidate_count": canonical_candidate_count,
        "source_record_count": len(material.source),
        "accepted_source_count": len(accepted),
        "ambiguous_source_count": len(ambiguous),
        "support_two_count": sum(record.support_cardinality == 2 for record in accepted),
        "support_three_count": sum(record.support_cardinality == 3 for record in accepted),
    }


def _fixture_sha256(
    material: _FixtureMaterial,
    *,
    raw_wedge_count: int,
    canonical_candidate_count: int,
) -> dict[str, str]:
    points_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/point-cloud/v1/sha256/"
    )
    _sha256_word(points_builder, len(material.point_bits))
    for point_index, point in enumerate(material.point_bits):
        _sha256_word(points_builder, point_index)
        for coordinate_bits in point:
            _sha256_word(points_builder, coordinate_bits)
    point_digest = points_builder.digest()

    edges_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/delaunay-edges/v1/sha256/"
    )
    _sha256_word(edges_builder, len(material.edges))
    for edge in material.edges:
        for point_id in edge:
            _sha256_word(edges_builder, point_id)
    edge_digest = edges_builder.digest()

    wedges_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/canonical-wedges/v1/sha256/"
    )
    wedges_builder.update(point_digest)
    wedges_builder.update(edge_digest)
    wedges_builder.update(b"owns_triangle_smallest_Delaunay_center_v1")
    _sha256_word(wedges_builder, raw_wedge_count)
    _sha256_word(wedges_builder, canonical_candidate_count)

    accepted = tuple(
        record for record in material.source if record.status == GABRIEL_STATUS
    )
    accepted_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/accepted/v1/sha256/"
    )
    _sha256_word(accepted_builder, len(accepted))
    for triangle in accepted:
        _sha256_triangle(accepted_builder, triangle)

    necessary_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/necessary/v1/sha256/"
    )
    for triangle in material.necessary:
        _sha256_triangle(necessary_builder, triangle)
    necessary_builder.update(b"/count/")
    _sha256_word(necessary_builder, len(material.necessary))
    necessary_digest = necessary_builder.digest()

    ambiguous_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/ambiguous-safety/v1/sha256/"
    )
    for triangle in material.ambiguous:
        _sha256_triangle(ambiguous_builder, triangle)
    ambiguous_builder.update(b"/count/")
    _sha256_word(ambiguous_builder, len(material.ambiguous))
    ambiguous_digest = ambiguous_builder.digest()

    composite_builder = _sha256_builder(
        b"MorseHGP3D/phase15/gabriel-coverage/composite-safety-batch/v1/sha256/"
    )
    composite_builder.update(necessary_digest)
    _sha256_word(composite_builder, len(material.necessary))
    composite_builder.update(ambiguous_digest)
    _sha256_word(composite_builder, len(material.ambiguous))

    return {
        "input.point_cloud_sha256": point_digest.hex(),
        "delaunay.ordinary_edges_sha256": edge_digest.hex(),
        "candidate_universe.canonical_wedge_universe_sha256": wedges_builder.hexdigest(),
        "coverage.accepted_source_sha256": accepted_builder.hexdigest(),
        "coverage.necessary_accepted_sha256": necessary_digest.hex(),
        "coverage.ambiguous_safety_sha256": ambiguous_digest.hex(),
        "coverage.explicit_safety_batch_sha256": composite_builder.hexdigest(),
    }


def recompute_emitted_fixture_sha256(payload: object) -> dict[str, str]:
    """Recompute the seven producer commitments for a bounded emitted fixture."""
    record = _mapping(payload, path="root")
    input_record = _mapping(record.get("input"), path="input")
    point_count = _uint(
        input_record.get("point_count"), path="input.point_count", positive=True
    )
    coverage = _mapping(record.get("coverage"), path="coverage")
    universe = _mapping(record.get("candidate_universe"), path="candidate_universe")
    material = _parse_fixture_material(
        record.get("emitted_fixture"),
        point_count=point_count,
        necessary_value=coverage.get("necessary_records"),
        ambiguous_value=coverage.get("ambiguous_safety_records"),
    )
    return _fixture_sha256(
        material,
        raw_wedge_count=_uint(
            universe.get("raw_wedge_count"),
            path="candidate_universe.raw_wedge_count",
            positive=True,
        ),
        canonical_candidate_count=_uint(
            universe.get("canonical_candidate_count"),
            path="candidate_universe.canonical_candidate_count",
            positive=True,
        ),
    )


def _replay_fixture(
    fixture_value: object,
    *,
    point_count: int,
    necessary_value: object,
    ambiguous_value: object,
    raw_wedge_count: int,
    canonical_candidate_count: int,
) -> tuple[dict[str, object], dict[str, str]]:
    material = _parse_fixture_material(
        fixture_value,
        point_count=point_count,
        necessary_value=necessary_value,
        ambiguous_value=ambiguous_value,
    )
    edge_set = set(material.edges)
    for index, record in enumerate(material.source):
        delaunay_edge_count = sum(facet in edge_set for facet in record.facets)
        if delaunay_edge_count < 2:
            raise CoverageInputError(
                f"emitted_fixture.source_records[{index}] is not a Delaunay wedge"
            )

    try:
        exact_tetrahedra, delaunay_audit = _enumerate_exact_ordinary_delaunay(
            material.points,
            path="emitted_fixture.ordinary_delaunay_edges",
        )
    except Phase14DiagnosticInputError as error:
        raise CoverageInputError(f"unsupported_degeneracy: {error}") from error
    exact_delaunay_edges = tuple(
        sorted(
            {
                tuple(sorted(edge))
                for tetrahedron in exact_tetrahedra
                for edge in combinations(tetrahedron, 2)
            }
        )
    )
    if material.edges != exact_delaunay_edges:
        missing_edges = sorted(set(exact_delaunay_edges) - edge_set)
        extra_edges = sorted(edge_set - set(exact_delaunay_edges))
        raise CoverageInputError(
            "emitted_fixture.ordinary_delaunay_edges does not equal the exact "
            f"ordinary-Delaunay 1-skeleton; missing={missing_edges!r}, "
            f"extra={extra_edges!r}"
        )

    filtration = build_gamma_filtration(material.points, 2)
    exact_gabriel = {
        hyperedge.simplex_point_ids: hyperedge
        for hyperedge in filtration.gabriel_hyperedges
    }
    for point_ids, hyperedge in exact_gabriel.items():
        shell_ids = tuple(
            point_id
            for point_id, point in enumerate(material.points)
            if sum(
                (coordinate - center_coordinate) ** 2
                for coordinate, center_coordinate in zip(point, hyperedge.center)
            )
            == hyperedge.squared_level
        )
        nonminimal_simplex_shell = sorted(
            set(shell_ids) & set(point_ids) - set(hyperedge.support_ids)
        )
        exterior_shell = sorted(set(shell_ids) - set(point_ids))
        if nonminimal_simplex_shell or exterior_shell:
            raise CoverageInputError(
                "unsupported_degeneracy: exact Gabriel simplex "
                f"{point_ids!r} has nonminimal_simplex_shell="
                f"{nonminimal_simplex_shell!r}, exterior_shell={exterior_shell!r}"
            )
    source_by_ids = {record.point_ids: record for record in material.source}
    missing_exact = sorted(set(exact_gabriel) - set(source_by_ids))
    if missing_exact:
        raise CoverageInputError(
            "emitted_fixture.source_records misses exact Gabriel triangles: "
            + repr(missing_exact)
        )
    for point_ids, hyperedge in exact_gabriel.items():
        source_record = source_by_ids[point_ids]
        exact_support = len(hyperedge.support_ids)
        if source_record.support_cardinality != exact_support:
            raise CoverageInputError(
                "emitted_fixture exact Gabriel support_cardinality disagrees with "
                f"the exact oracle for {point_ids}"
            )
        if sum(facet in edge_set for facet in source_record.facets) < 2:
            raise CoverageInputError(
                f"exact Gabriel triangle {point_ids} has fewer than two declared Delaunay edges"
            )

    accepted = tuple(
        record for record in material.source if record.status == GABRIEL_STATUS
    )
    ambiguous = tuple(
        record for record in material.source if record.status == AMBIGUOUS_STATUS
    )
    facets = {facet for record in accepted for facet in record.facets}
    components = _DisjointSet(facets)
    replayed_necessary: list[TriangleRecord] = []
    strictly_lower_connected: list[TriangleRecord] = []

    plateau_begin = 0
    while plateau_begin < len(accepted):
        plateau_level = accepted[plateau_begin].squared_level
        plateau_end = plateau_begin + 1
        while (
            plateau_end < len(accepted)
            and accepted[plateau_end].squared_level == plateau_level
        ):
            plateau_end += 1
        plateau = accepted[plateau_begin:plateau_end]
        for record in plateau:
            roots = {components.find(facet) for facet in record.facets}
            if len(roots) == 1:
                strictly_lower_connected.append(record)
            else:
                replayed_necessary.append(record)
        for record in plateau:
            first, second, third = record.facets
            components.union(first, second)
            components.union(first, third)
        plateau_begin = plateau_end

    if tuple(replayed_necessary) != material.necessary:
        raise CoverageInputError(
            "coverage.necessary_records differs from the strict-lower DSU replay"
        )
    if ambiguous != material.ambiguous:
        raise CoverageInputError(
            "coverage.ambiguous_safety_records differs from emitted source ambiguities"
        )
    if len(replayed_necessary) + len(strictly_lower_connected) != len(accepted):
        raise CoverageInputError(
            "an accepted source is neither necessary nor strict-lower connected"
        )

    counts = _fixture_counts(material)
    exact_support_two = sum(
        len(hyperedge.support_ids) == 2 for hyperedge in exact_gabriel.values()
    )
    exact_support_three = sum(
        len(hyperedge.support_ids) == 3 for hyperedge in exact_gabriel.values()
    )
    return {
        **counts,
        "ordinary_delaunay_exact_recertification": {
            **delaunay_audit,
            "declared_one_skeleton_matches_exact_catalog": True,
        },
        "accepted_source_count": len(accepted),
        "ambiguous_source_count": len(ambiguous),
        "replayed_necessary_count": len(replayed_necessary),
        "replayed_strictly_lower_connected_count": len(strictly_lower_connected),
        "every_source_is_a_delaunay_wedge": True,
        "every_accepted_source_is_necessary_or_strictly_lower_connected": True,
        "necessary_records_match": True,
        "ambiguous_records_match": True,
        "all_seven_sha256_recomputed": True,
        "exact_gabriel_oracle_performed": True,
        "exact_gabriel_triangle_count": len(exact_gabriel),
        "exact_gabriel_support_two_count": exact_support_two,
        "exact_gabriel_support_three_count": exact_support_three,
        "every_exact_gabriel_triangle_emitted": True,
        "every_exact_gabriel_triangle_has_two_declared_delaunay_edges": True,
        "exact_gabriel_support_cardinalities_match": True,
    }, _fixture_sha256(
        material,
        raw_wedge_count=raw_wedge_count,
        canonical_candidate_count=canonical_candidate_count,
    )


def analyze_coverage(
    payload: object,
    *,
    source_artifact_sha256: str | None = None,
    source_artifact_byte_count: int | None = None,
) -> dict[str, object]:
    if (source_artifact_sha256 is None) != (source_artifact_byte_count is None):
        raise CoverageInputError(
            "source artifact SHA-256 and byte count must be supplied together"
        )
    source_artifact: dict[str, object] | None = None
    if source_artifact_sha256 is not None:
        source_artifact = {
            "sha256": _sha256(
                source_artifact_sha256,
                path="source_artifact.sha256",
            ),
            "byte_count": _uint(
                source_artifact_byte_count,
                path="source_artifact.byte_count",
                positive=True,
            ),
        }
    record = _mapping(payload, path="root")
    _literal(record, "schema", INPUT_SCHEMA, "root")
    git_sha = record.get("git_sha")
    if not isinstance(git_sha, str) or GIT_SHA_PATTERN.fullmatch(git_sha) is None:
        raise CoverageInputError("root.git_sha must be 40 lowercase hexadecimal digits")
    for key, expected in (
        ("phase", "15"),
        ("backend", "ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid"),
        ("profile", "hgp_reduced"),
        ("mode", "gabriel_necessary_batch"),
        ("deployment_status", "diagnostic_sidecar"),
        ("public_status", "not_claimed"),
        (
            "criterion",
            "triangle_explicit_or_three_facets_connected_at_strictly_lower_level",
        ),
    ):
        _literal(record, key, expected, "root")

    proof = _mapping(record.get("proof_scope"), path="proof_scope")
    for key, expected in (
        ("general_position_required", True),
        ("complete_Voronoi_nerve_one_skeleton_required", True),
        ("Geogram_SoS_topology_exactly_recertified", False),
        ("every_regular_Gabriel_triangle_has_at_least_two_Delaunay_edges", True),
        ("support_cardinality_two_and_three_covered", True),
        ("exact_Gamma2_claimed", False),
        ("exact_Gabriel_classification_claimed", False),
    ):
        _literal(proof, key, expected, "proof_scope")

    architecture = _mapping(record.get("architecture"), path="architecture")
    for key, expected in (
        ("edge_times_point_product_enumerated", False),
        ("higher_order_Delaunay_mosaic_materialized", False),
        ("ordinary_Delaunay_edges_only", True),
        ("canonical_wedges_enumerated_in_bounded_GPU_chunks", True),
        ("global_restricted_Gamma2_arena_materialized", False),
        ("global_Gabriel_proposal_arena_materialized_for_level_sort", True),
        ("same_level_candidates_can_certify_each_other", False),
    ):
        _literal(architecture, key, expected, "architecture")

    input_record = _mapping(record.get("input"), path="input")
    point_count = _uint(input_record.get("point_count"), path="input.point_count", positive=True)
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
        raise CoverageInputError("input.cpu_workers_used exceeds cpu_workers_requested")
    point_cloud_sha256 = _sha256(
        input_record.get("point_cloud_sha256"), path="input.point_cloud_sha256"
    )

    delaunay = _mapping(record.get("delaunay"), path="delaunay")
    _literal(delaunay, "engine", "PDEL", "delaunay")
    _uint(delaunay.get("cell_count"), path="delaunay.cell_count", positive=True)
    ordinary_edge_count = _uint(
        delaunay.get("ordinary_edge_count"),
        path="delaunay.ordinary_edge_count",
        positive=True,
    )
    if ordinary_edge_count > point_count * (point_count - 1) // 2:
        raise CoverageInputError("delaunay.ordinary_edge_count exceeds the simple-graph bound")
    maximum_vertex_degree = _uint(
        delaunay.get("maximum_vertex_degree"),
        path="delaunay.maximum_vertex_degree",
        positive=True,
    )
    if maximum_vertex_degree >= point_count:
        raise CoverageInputError("delaunay.maximum_vertex_degree exceeds the simple-graph bound")
    ordinary_edges_sha256 = _sha256(
        delaunay.get("ordinary_edges_sha256"), path="delaunay.ordinary_edges_sha256"
    )

    universe = _mapping(record.get("candidate_universe"), path="candidate_universe")
    _literal(universe, "generator", "canonical_Delaunay_CSR_wedges", "candidate_universe")
    raw_count = _uint(
        universe.get("raw_wedge_count"),
        path="candidate_universe.raw_wedge_count",
        positive=True,
    )
    candidate_count = _uint(
        universe.get("canonical_candidate_count"),
        path="candidate_universe.canonical_candidate_count",
        positive=True,
    )
    if raw_count < candidate_count:
        raise CoverageInputError(
            "candidate_universe.raw_wedge_count is below canonical_candidate_count"
        )
    canonical_wedge_sha256 = _sha256(
        universe.get("canonical_wedge_universe_sha256"),
        path="candidate_universe.canonical_wedge_universe_sha256",
    )
    chunk_count = _uint(
        universe.get("chunk_count"),
        path="candidate_universe.chunk_count",
        positive=True,
    )
    triangle_chunk_vertices = _uint(
        universe.get("triangle_chunk_vertices"),
        path="candidate_universe.triangle_chunk_vertices",
        positive=True,
    )
    if chunk_count != (point_count + triangle_chunk_vertices - 1) // triangle_chunk_vertices:
        raise CoverageInputError(
            "candidate_universe.chunk_count disagrees with point_count/chunk size"
        )
    maximum_chunk_candidate_count = _uint(
        universe.get("maximum_chunk_candidate_count"),
        path="candidate_universe.maximum_chunk_candidate_count",
        positive=True,
    )
    if maximum_chunk_candidate_count > candidate_count:
        raise CoverageInputError(
            "candidate_universe.maximum_chunk_candidate_count exceeds the universe"
        )

    classification = _mapping(record.get("classification"), path="classification")
    blocked = _uint(classification.get("blocked_count"), path="classification.blocked_count")
    accepted = _uint(
        classification.get("gabriel_binary64_count"),
        path="classification.gabriel_binary64_count",
    )
    ambiguous = _uint(
        classification.get("ambiguous_safety_count"),
        path="classification.ambiguous_safety_count",
    )
    invalid = _uint(classification.get("invalid_count"), path="classification.invalid_count")
    _uint(
        classification.get("visited_cell_count"),
        path="classification.visited_cell_count",
    )
    _uint(
        classification.get("tested_point_count"),
        path="classification.tested_point_count",
    )
    if blocked + accepted + ambiguous + invalid != candidate_count:
        raise CoverageInputError("classification status counters do not partition candidates")
    _literal(classification, "status_partition_closed", True, "classification")

    coverage = _mapping(record.get("coverage"), path="coverage")
    source_count = _uint(
        coverage.get("source_triangle_count"),
        path="coverage.source_triangle_count",
    )
    support_two = _uint(coverage.get("support_two_count"), path="coverage.support_two_count")
    support_three = _uint(coverage.get("support_three_count"), path="coverage.support_three_count")
    necessary = _uint(
        coverage.get("explicit_necessary_accepted_count"),
        path="coverage.explicit_necessary_accepted_count",
    )
    strict_lower = _uint(
        coverage.get("strictly_lower_connected_count"),
        path="coverage.strictly_lower_connected_count",
    )
    if source_count != accepted:
        raise CoverageInputError("coverage.source_triangle_count differs from accepted")
    if necessary + strict_lower != accepted:
        raise CoverageInputError("accepted != necessary + strictly-lower connected")
    if support_two + support_three != accepted:
        raise CoverageInputError("accepted != support-two + support-three")
    coverage_violation_count = _uint(
        coverage.get("coverage_violation_count"),
        path="coverage.coverage_violation_count",
    )
    if coverage_violation_count != 0:
        raise CoverageInputError("coverage.coverage_violation_count must be zero")
    _literal(coverage, "accepted_partition_closed", True, "coverage")
    _literal(coverage, "binary64_gate_passed", True, "coverage")
    coverage_sha256: dict[str, str] = {}
    for key in (
        "accepted_source_sha256",
        "necessary_accepted_sha256",
        "ambiguous_safety_sha256",
        "explicit_safety_batch_sha256",
    ):
        coverage_sha256[key] = _sha256(coverage.get(key), path=f"coverage.{key}")
    safety_count = _uint(
        coverage.get("explicit_safety_batch_count"),
        path="coverage.explicit_safety_batch_count",
    )
    if safety_count != necessary + ambiguous:
        raise CoverageInputError("safety batch != necessary + ambiguous")
    _literal(
        coverage,
        "explicit_safety_batch_semantics",
        "necessary_accepted_plus_all_ambiguous",
        "coverage",
    )
    if invalid != 0:
        raise CoverageInputError("a passing binary64 coverage gate must have invalid_count=0")

    fixture_report: dict[str, object] | None = None
    if record.get("emitted_fixture") is not None:
        if (
            coverage.get("necessary_records") is None
            or coverage.get("ambiguous_safety_records") is None
        ):
            raise CoverageInputError(
                "emitted_fixture requires both emitted coverage record arrays"
            )
        fixture_report, computed_sha256 = _replay_fixture(
            record.get("emitted_fixture"),
            point_count=point_count,
            necessary_value=coverage.get("necessary_records"),
            ambiguous_value=coverage.get("ambiguous_safety_records"),
            raw_wedge_count=raw_count,
            canonical_candidate_count=candidate_count,
        )
        expected = {
            "ordinary_delaunay_edge_count": ordinary_edge_count,
            "raw_wedge_count": raw_count,
            "canonical_candidate_count": candidate_count,
            "source_record_count": accepted + ambiguous,
            "accepted_source_count": accepted,
            "ambiguous_source_count": ambiguous,
            "support_two_count": support_two,
            "support_three_count": support_three,
            "replayed_necessary_count": necessary,
            "replayed_strictly_lower_connected_count": strict_lower,
        }
        for key, value in expected.items():
            if fixture_report[key] != value:
                raise CoverageInputError(f"emitted_fixture replay disagrees with {key}")
        declared_sha256 = {
            "input.point_cloud_sha256": point_cloud_sha256,
            "delaunay.ordinary_edges_sha256": ordinary_edges_sha256,
            "candidate_universe.canonical_wedge_universe_sha256": canonical_wedge_sha256,
            **{
                f"coverage.{key}": value for key, value in coverage_sha256.items()
            },
        }
        for path, computed in computed_sha256.items():
            if declared_sha256[path] != computed:
                raise CoverageInputError(f"{path} does not match fixture recomputation")
    elif (
        coverage.get("necessary_records") is not None
        or coverage.get("ambiguous_safety_records") is not None
    ):
        raise CoverageInputError("coverage record arrays require emitted_fixture")

    return {
        "schema_version": CHECK_SCHEMA,
        "diagnostic_completed": True,
        "phase": "15",
        "backend": "python_bounded_structural_replay",
        "profile": "hgp_reduced",
        "public_status": "not_claimed",
        "source_artifact": source_artifact,
        "point_count": point_count,
        "chunk_count": chunk_count,
        "counter_invariants": {
            "status_partition_closed": True,
            "accepted_partition_closed": True,
            "support_partition_closed": True,
            "safety_partition_closed": True,
        },
        "architecture_invariants": {
            "edge_times_point_product_absent": True,
            "higher_order_delaunay_absent": True,
            "same_level_certification_forbidden": True,
        },
        "digest_validation": {
            "mode": (
                "recomputed_from_emitted_fixture"
                if fixture_report is not None
                else "producer_commitments_only"
            ),
            "all_seven_sha256_recomputed": fixture_report is not None,
            "all_seven_sha256_match": True if fixture_report is not None else None,
            "producer_commitments_not_recomputable_without_records": fixture_report is None,
        },
        "emitted_fixture_point_cap": MAX_EMITTED_FIXTURE_POINTS,
        "fixture_replay": fixture_report,
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "diagnostic",
        nargs="?",
        default="-",
        help="coverage JSON path, or '-' / omitted for stdin",
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
        report = analyze_coverage(
            payload,
            source_artifact_sha256=hashlib.sha256(raw_artifact).hexdigest(),
            source_artifact_byte_count=len(raw_artifact),
        )
    except (
        CoverageInputError,
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
    json.dump(report, sys.stdout, indent=2 if arguments.pretty else None, sort_keys=True)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
