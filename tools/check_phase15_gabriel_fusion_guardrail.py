#!/usr/bin/env python3
"""Check a unilateral closed-level Gabriel fusion guardrail.

The native scientific domain is order-two pair facets.  For every declared
exact Gabriel triangle ``T={a,b,c}`` born at the exact squared-radius level
``alpha``, the checker asks whether the candidate hierarchy connects the three
facets ``ab``, ``ac`` and ``bc`` no later than ``alpha``.  All unions from one
equal-level plateau are applied before the closed query at that level.

Two candidate representations are accepted:

``k2_facet_relation_stream_exact_v1``
    A native stream of exact-rational weighted hyperedges on pair facets.

``point_weighted_spanning_tree_exact_v1``
    A point tree only through the explicit
    ``point_component_clique_lift_v1`` adapter.  One point component is lifted
    to one component containing every pair facet whose two endpoints lie in
    that point component.  The adapter must state its input/output level
    conventions and its exact conversion.  This is an explicit proxy, not a
    native Gamma_2 hierarchy and never establishes Gamma_2 exactness.

Input floats are deliberately rejected: all scientific levels and weights
must be integers, rational strings, or ``{numerator, denominator}`` records.
The report partitions every source triangle into ``before``, ``at``, ``late``,
``never`` or ``unsupported``.  ``before`` and ``at`` satisfy the unilateral
guardrail; every other class fails closed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import deque
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Iterable, Mapping, Sequence

INPUT_SCHEMA = "morsehgp3d.phase15_gabriel_fusion_guardrail.v1"
CHECK_SCHEMA = "morsehgp3d.phase15_gabriel_fusion_guardrail_check.v1"
SOURCE_DOMAIN = "exact_gabriel_triangles_k2_pair_facets"
EXACT_LEVEL_CONVENTION = "exact_squared_radius"
NATIVE_REPRESENTATION = "k2_facet_relation_stream_exact_v1"
POINT_TREE_REPRESENTATION = "point_weighted_spanning_tree_exact_v1"
POINT_LIFT_ADAPTER = "point_component_clique_lift_v1"
POINT_LIFT_SEMANTICS = (
    "one_lifted_component_per_point_component_with_all_internal_pair_facets"
)
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
MAX_ID_LENGTH = 160

Facet = tuple[int, int]


class GuardrailInputError(ValueError):
    """The input cannot support the requested fail-closed guardrail."""


@dataclass(frozen=True)
class SourceTriangle:
    triangle_id: str
    point_ids: tuple[int, int, int]
    squared_level: Fraction
    geometry_status: str

    @property
    def facets(self) -> tuple[Facet, Facet, Facet]:
        first, second, third = self.point_ids
        return ((first, second), (first, third), (second, third))


@dataclass(frozen=True)
class FacetEvent:
    event_id: str
    squared_level: Fraction
    facets: tuple[Facet, ...]


@dataclass(frozen=True)
class TreeEdge:
    edge_id: str
    first: int
    second: int
    input_weight: Fraction
    converted_weight: Fraction


@dataclass(frozen=True)
class _Connection:
    squared_level: Fraction | None
    pair_levels: tuple[Fraction | None, Fraction | None, Fraction | None]
    final_component_labels: tuple[str, str, str]


class _DisjointSet:
    def __init__(self, size: int) -> None:
        self.parent = list(range(size))
        self.sizes = [1] * size

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
        return True


class _WeightedForest:
    """Maximum-edge path queries over a deterministic weighted forest."""

    def __init__(
        self,
        labels: Sequence[object],
        edges: Iterable[tuple[int, int, Fraction]],
    ) -> None:
        self.labels = tuple(labels)
        size = len(self.labels)
        adjacency: list[list[tuple[int, Fraction]]] = [[] for _ in range(size)]
        edge_count = 0
        for left, right, weight in edges:
            adjacency[left].append((right, weight))
            adjacency[right].append((left, weight))
            edge_count += 1
        for neighbors in adjacency:
            neighbors.sort(key=lambda item: (item[0], item[1]))

        self.depth = [0] * size
        self.component = [-1] * size
        parent = list(range(size))
        parent_weight = [Fraction(0)] * size
        visited = [False] * size
        component_index = 0
        for root in range(size):
            if visited[root]:
                continue
            visited[root] = True
            self.component[root] = component_index
            queue: deque[int] = deque((root,))
            while queue:
                current = queue.popleft()
                for neighbor, weight in adjacency[current]:
                    if neighbor == parent[current] and current != root:
                        continue
                    if visited[neighbor]:
                        raise GuardrailInputError(
                            "candidate union witnesses do not form a forest"
                        )
                    visited[neighbor] = True
                    parent[neighbor] = current
                    parent_weight[neighbor] = weight
                    self.depth[neighbor] = self.depth[current] + 1
                    self.component[neighbor] = component_index
                    queue.append(neighbor)
            component_index += 1
        if edge_count != size - component_index:
            raise GuardrailInputError(
                "candidate union witness edge count is inconsistent with its forest"
            )
        self.component_representatives = [size] * component_index
        for node, component in enumerate(self.component):
            self.component_representatives[component] = min(
                self.component_representatives[component], node
            )

        level_count = max(1, max(1, size).bit_length())
        self.ancestors = [parent]
        self.maximum_weights = [parent_weight]
        for _ in range(1, level_count):
            previous_ancestors = self.ancestors[-1]
            previous_weights = self.maximum_weights[-1]
            next_ancestors = [0] * size
            next_weights = [Fraction(0)] * size
            for node in range(size):
                middle = previous_ancestors[node]
                next_ancestors[node] = previous_ancestors[middle]
                next_weights[node] = max(
                    previous_weights[node], previous_weights[middle]
                )
            self.ancestors.append(next_ancestors)
            self.maximum_weights.append(next_weights)

    def maximum_on_path(self, first: int, second: int) -> Fraction | None:
        if self.component[first] != self.component[second]:
            return None
        maximum = Fraction(0)
        if self.depth[first] < self.depth[second]:
            first, second = second, first
        difference = self.depth[first] - self.depth[second]
        level = 0
        while difference:
            if difference & 1:
                maximum = max(maximum, self.maximum_weights[level][first])
                first = self.ancestors[level][first]
            difference >>= 1
            level += 1
        if first == second:
            return maximum
        for reverse in range(len(self.ancestors) - 1, -1, -1):
            first_ancestor = self.ancestors[reverse][first]
            second_ancestor = self.ancestors[reverse][second]
            if first_ancestor != second_ancestor:
                maximum = max(
                    maximum,
                    self.maximum_weights[reverse][first],
                    self.maximum_weights[reverse][second],
                )
                first = first_ancestor
                second = second_ancestor
        return max(
            maximum,
            self.maximum_weights[0][first],
            self.maximum_weights[0][second],
        )

    def component_label(self, node: int) -> str:
        representative = self.component_representatives[self.component[node]]
        return _label_text(self.labels[representative])


def _mapping(value: object, *, path: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        raise GuardrailInputError(f"{path} must be an object")
    if any(not isinstance(key, str) for key in value):
        raise GuardrailInputError(f"{path} keys must be strings")
    return value  # type: ignore[return-value]


def _array(value: object, *, path: str) -> list[object]:
    if not isinstance(value, list):
        raise GuardrailInputError(f"{path} must be an array")
    return value


def _literal(
    record: Mapping[str, object], key: str, expected: object, *, path: str
) -> None:
    value = record.get(key)
    if type(value) is not type(expected) or value != expected:
        raise GuardrailInputError(f"{path}.{key} must be {expected!r}")


def _identifier(value: object, *, path: str) -> str:
    if not isinstance(value, str) or not value or value != value.strip():
        raise GuardrailInputError(f"{path} must be a non-empty trimmed string")
    if len(value) > MAX_ID_LENGTH:
        raise GuardrailInputError(f"{path} is too long")
    return value


def _uint(value: object, *, path: str, upper_bound: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise GuardrailInputError(f"{path} must be a non-negative integer")
    if upper_bound is not None and value >= upper_bound:
        raise GuardrailInputError(f"{path} lies outside 0..{upper_bound - 1}")
    return value


def _exact_level(value: object, *, path: str) -> Fraction:
    if isinstance(value, bool) or isinstance(value, float):
        raise GuardrailInputError(
            f"{path} must be exact-rational; binary64 floats are forbidden"
        )
    if isinstance(value, int):
        result = Fraction(value)
    elif isinstance(value, str):
        try:
            result = Fraction(value.strip())
        except (ValueError, ZeroDivisionError) as error:
            raise GuardrailInputError(f"{path} is not an exact rational") from error
    elif isinstance(value, Mapping):
        record = _mapping(value, path=path)
        if set(record) != {"numerator", "denominator"}:
            raise GuardrailInputError(
                f"{path} rational record must contain numerator and denominator"
            )
        numerator = record.get("numerator")
        denominator = record.get("denominator")
        if (
            isinstance(numerator, bool)
            or not isinstance(numerator, int)
            or isinstance(denominator, bool)
            or not isinstance(denominator, int)
            or denominator == 0
        ):
            raise GuardrailInputError(f"{path} is not an exact rational record")
        result = Fraction(numerator, denominator)
    else:
        raise GuardrailInputError(f"{path} must be an exact rational")
    if result < 0:
        raise GuardrailInputError(f"{path} must be non-negative")
    return result


def _fraction_text(value: Fraction | None) -> str | None:
    if value is None:
        return None
    return str(value.numerator) if value.denominator == 1 else str(value)


def _facet(value: object, *, path: str, point_count: int) -> Facet:
    raw = _array(value, path=path)
    if len(raw) != 2:
        raise GuardrailInputError(f"{path} must contain exactly two point ids")
    first = _uint(raw[0], path=f"{path}[0]", upper_bound=point_count)
    second = _uint(raw[1], path=f"{path}[1]", upper_bound=point_count)
    if first >= second:
        raise GuardrailInputError(
            f"{path} must be a canonical pair with first < second"
        )
    return first, second


def _triangle_point_ids(
    value: object,
    *,
    path: str,
    point_count: int,
    require_distinct: bool,
) -> tuple[int, int, int]:
    raw = _array(value, path=path)
    if len(raw) != 3:
        raise GuardrailInputError(f"{path} must contain exactly three point ids")
    result = tuple(
        _uint(item, path=f"{path}[{index}]", upper_bound=point_count)
        for index, item in enumerate(raw)
    )
    if tuple(sorted(result)) != result:
        raise GuardrailInputError(f"{path} must be sorted canonically")
    if require_distinct and not result[0] < result[1] < result[2]:
        raise GuardrailInputError(f"{path} must contain three distinct point ids")
    return result  # type: ignore[return-value]


def _parse_source(
    record: Mapping[str, object], point_count: int
) -> tuple[SourceTriangle, ...]:
    _literal(record, "source_domain", SOURCE_DOMAIN, path="document")
    _literal(
        record,
        "source_level_convention",
        EXACT_LEVEL_CONVENTION,
        path="document",
    )
    raw_triangles = _array(record.get("source_triangles"), path="source_triangles")
    if not raw_triangles:
        raise GuardrailInputError("source_triangles must not be empty")
    triangles: list[SourceTriangle] = []
    identifiers: set[str] = set()
    point_labels: set[tuple[int, int, int]] = set()
    for index, value in enumerate(raw_triangles):
        path = f"source_triangles[{index}]"
        triangle = _mapping(value, path=path)
        triangle_id = _identifier(
            triangle.get("triangle_id"), path=f"{path}.triangle_id"
        )
        if triangle_id in identifiers:
            raise GuardrailInputError(f"{path}.triangle_id is duplicated")
        identifiers.add(triangle_id)
        geometry_status = triangle.get("geometry_status")
        if geometry_status not in {"regular_exact", "unsupported_degeneracy"}:
            raise GuardrailInputError(
                f"{path}.geometry_status must be 'regular_exact' or "
                "'unsupported_degeneracy'"
            )
        point_ids = _triangle_point_ids(
            triangle.get("point_ids"),
            path=f"{path}.point_ids",
            point_count=point_count,
            require_distinct=geometry_status == "regular_exact",
        )
        if point_ids in point_labels:
            raise GuardrailInputError(f"{path}.point_ids duplicates a source triangle")
        point_labels.add(point_ids)
        triangles.append(
            SourceTriangle(
                triangle_id,
                point_ids,
                _exact_level(
                    triangle.get("squared_level_exact"),
                    path=f"{path}.squared_level_exact",
                ),
                str(geometry_status),
            )
        )
    expected = sorted(
        triangles,
        key=lambda item: (item.squared_level, item.triangle_id, item.point_ids),
    )
    if triangles != expected:
        raise GuardrailInputError(
            "source_triangles must be sorted by exact level, triangle_id and point_ids"
        )
    return tuple(triangles)


def _parse_facet_events(
    candidate: Mapping[str, object], point_count: int
) -> tuple[FacetEvent, ...]:
    _literal(
        candidate,
        "level_convention",
        EXACT_LEVEL_CONVENTION,
        path="candidate",
    )
    raw_events = _array(candidate.get("events"), path="candidate.events")
    events: list[FacetEvent] = []
    identifiers: set[str] = set()
    for index, value in enumerate(raw_events):
        path = f"candidate.events[{index}]"
        event = _mapping(value, path=path)
        event_id = _identifier(event.get("event_id"), path=f"{path}.event_id")
        if event_id in identifiers:
            raise GuardrailInputError(f"{path}.event_id is duplicated")
        identifiers.add(event_id)
        raw_facets = _array(event.get("facets"), path=f"{path}.facets")
        facets = tuple(
            _facet(facet, path=f"{path}.facets[{facet_index}]", point_count=point_count)
            for facet_index, facet in enumerate(raw_facets)
        )
        if len(facets) < 2:
            raise GuardrailInputError(f"{path}.facets must contain at least two facets")
        if tuple(sorted(set(facets))) != facets:
            raise GuardrailInputError(
                f"{path}.facets must be unique and canonically sorted"
            )
        events.append(
            FacetEvent(
                event_id,
                _exact_level(
                    event.get("squared_level_exact"),
                    path=f"{path}.squared_level_exact",
                ),
                facets,
            )
        )
    expected = sorted(events, key=lambda item: (item.squared_level, item.event_id))
    if events != expected:
        raise GuardrailInputError(
            "candidate.events must be sorted by exact level and event_id"
        )
    return tuple(events)


def _parse_point_tree(
    candidate: Mapping[str, object], point_count: int
) -> tuple[tuple[TreeEdge, ...], dict[str, str]]:
    adapter = _mapping(candidate.get("adapter"), path="candidate.adapter")
    _literal(adapter, "adapter_id", POINT_LIFT_ADAPTER, path="candidate.adapter")
    _literal(
        adapter,
        "component_lift",
        POINT_LIFT_SEMANTICS,
        path="candidate.adapter",
    )
    input_convention = adapter.get("input_level_convention")
    output_convention = adapter.get("output_level_convention")
    conversion = adapter.get("level_conversion")
    if output_convention != EXACT_LEVEL_CONVENTION:
        raise GuardrailInputError(
            "candidate.adapter.output_level_convention must be "
            f"{EXACT_LEVEL_CONVENTION!r}"
        )
    allowed = {
        ("exact_squared_radius", "identity"),
        ("exact_squared_distance", "divide_by_4"),
    }
    if (input_convention, conversion) not in allowed:
        raise GuardrailInputError(
            "candidate.adapter has an incompatible input level convention or "
            "level conversion"
        )
    documentation = adapter.get("level_conversion_documentation")
    if (
        not isinstance(documentation, str)
        or documentation != documentation.strip()
        or len(documentation) < 16
    ):
        raise GuardrailInputError(
            "candidate.adapter.level_conversion_documentation must explicitly "
            "document the conversion"
        )
    _literal(
        candidate,
        "level_convention",
        input_convention,
        path="candidate",
    )

    raw_edges = _array(candidate.get("tree_edges"), path="candidate.tree_edges")
    if len(raw_edges) != point_count - 1:
        raise GuardrailInputError(
            "candidate.tree_edges must contain exactly point_count-1 edges"
        )
    edges: list[TreeEdge] = []
    identifiers: set[str] = set()
    endpoint_pairs: set[tuple[int, int]] = set()
    for index, value in enumerate(raw_edges):
        path = f"candidate.tree_edges[{index}]"
        edge = _mapping(value, path=path)
        edge_id = _identifier(edge.get("edge_id"), path=f"{path}.edge_id")
        if edge_id in identifiers:
            raise GuardrailInputError(f"{path}.edge_id is duplicated")
        identifiers.add(edge_id)
        first = _uint(edge.get("u"), path=f"{path}.u", upper_bound=point_count)
        second = _uint(edge.get("v"), path=f"{path}.v", upper_bound=point_count)
        if first >= second:
            raise GuardrailInputError(f"{path} must satisfy u < v")
        if (first, second) in endpoint_pairs:
            raise GuardrailInputError(f"{path} duplicates a point-tree edge")
        endpoint_pairs.add((first, second))
        input_weight = _exact_level(
            edge.get("weight_exact"), path=f"{path}.weight_exact"
        )
        converted_weight = (
            input_weight if conversion == "identity" else input_weight / 4
        )
        edges.append(
            TreeEdge(edge_id, first, second, input_weight, converted_weight)
        )
    expected = sorted(
        edges,
        key=lambda item: (
            item.input_weight,
            item.first,
            item.second,
            item.edge_id,
        ),
    )
    if edges != expected:
        raise GuardrailInputError(
            "candidate.tree_edges must be sorted by input weight, endpoints and edge_id"
        )
    components = _DisjointSet(point_count)
    for edge in edges:
        if not components.union(edge.first, edge.second):
            raise GuardrailInputError("candidate.tree_edges contains a cycle")
    if len({components.find(point) for point in range(point_count)}) != 1:
        raise GuardrailInputError("candidate.tree_edges is not spanning")
    return tuple(edges), {
        "adapter_id": POINT_LIFT_ADAPTER,
        "component_lift": POINT_LIFT_SEMANTICS,
        "input_level_convention": str(input_convention),
        "output_level_convention": str(output_convention),
        "level_conversion": str(conversion),
        "level_conversion_documentation": documentation,
    }


def _label_text(label: object) -> str:
    if isinstance(label, tuple):
        return "-".join(str(value) for value in label)
    return str(label)


def _native_connections(
    triangles: Sequence[SourceTriangle],
    events: Sequence[FacetEvent],
) -> tuple[dict[str, _Connection], dict[str, object], dict[str, object]]:
    facets = sorted(
        {
            facet
            for triangle in triangles
            if triangle.geometry_status == "regular_exact"
            for facet in triangle.facets
        }
        | {facet for event in events for facet in event.facets}
    )
    facet_ids = {facet: index for index, facet in enumerate(facets)}
    components = _DisjointSet(len(facets))
    forest_edges: list[tuple[int, int, Fraction]] = []
    useful_union_count = 0
    redundant_union_count = 0
    plateau_count = 0
    previous_level: Fraction | None = None
    for event in events:
        if event.squared_level != previous_level:
            plateau_count += 1
            previous_level = event.squared_level
        anchor = facet_ids[event.facets[0]]
        for facet in event.facets[1:]:
            other = facet_ids[facet]
            if components.union(anchor, other):
                forest_edges.append((anchor, other, event.squared_level))
                useful_union_count += 1
            else:
                redundant_union_count += 1
    forest = _WeightedForest(facets, forest_edges)
    connections: dict[str, _Connection] = {}
    for triangle in triangles:
        if triangle.geometry_status != "regular_exact":
            continue
        ids = tuple(facet_ids[facet] for facet in triangle.facets)
        pair_levels = (
            forest.maximum_on_path(ids[0], ids[1]),
            forest.maximum_on_path(ids[0], ids[2]),
            forest.maximum_on_path(ids[1], ids[2]),
        )
        level = (
            max(pair_levels)
            if all(item is not None for item in pair_levels)
            else None
        )
        connections[triangle.triangle_id] = _Connection(
            level,
            pair_levels,
            tuple(forest.component_label(node) for node in ids),
        )
    normalized = {
        "representation": NATIVE_REPRESENTATION,
        "level_convention": EXACT_LEVEL_CONVENTION,
        "events": [
            {
                "event_id": event.event_id,
                "squared_level_exact": _fraction_text(event.squared_level),
                "facets": [list(facet) for facet in event.facets],
            }
            for event in events
        ],
    }
    audit = {
        "native_k2_facet_domain": True,
        "point_component_lift_used": False,
        "candidate_facet_count": len(facets),
        "candidate_event_count": len(events),
        "candidate_plateau_count": plateau_count,
        "useful_union_count": useful_union_count,
        "redundant_union_count": redundant_union_count,
        "forest_edge_count": len(forest_edges),
    }
    return connections, normalized, audit


def _point_lift_connections(
    triangles: Sequence[SourceTriangle],
    edges: Sequence[TreeEdge],
    point_count: int,
    adapter: Mapping[str, str],
) -> tuple[dict[str, _Connection], dict[str, object], dict[str, object]]:
    forest = _WeightedForest(
        tuple(range(point_count)),
        ((edge.first, edge.second, edge.converted_weight) for edge in edges),
    )
    connections: dict[str, _Connection] = {}
    for triangle in triangles:
        if triangle.geometry_status != "regular_exact":
            continue
        first, second, third = triangle.point_ids
        pair_levels = (
            forest.maximum_on_path(first, second),
            forest.maximum_on_path(first, third),
            forest.maximum_on_path(second, third),
        )
        level = (
            max(pair_levels)
            if all(item is not None for item in pair_levels)
            else None
        )
        connections[triangle.triangle_id] = _Connection(
            level,
            pair_levels,
            tuple(
                forest.component_label(point)
                for point in (first, second, third)
            ),
        )
    normalized = {
        "representation": POINT_TREE_REPRESENTATION,
        "level_convention": adapter["input_level_convention"],
        "adapter": dict(adapter),
        "tree_edges": [
            {
                "edge_id": edge.edge_id,
                "u": edge.first,
                "v": edge.second,
                "weight_exact": _fraction_text(edge.input_weight),
                "converted_squared_level_exact": _fraction_text(
                    edge.converted_weight
                ),
            }
            for edge in edges
        ],
    }
    audit = {
        "native_k2_facet_domain": False,
        "point_component_lift_used": True,
        "adapter_id": POINT_LIFT_ADAPTER,
        "candidate_point_count": point_count,
        "candidate_tree_edge_count": len(edges),
        "candidate_plateau_count_after_conversion": len(
            {edge.converted_weight for edge in edges}
        ),
        "tree_spanning_and_acyclic": True,
        "gamma2_exactness_claimed": False,
    }
    return connections, normalized, audit


def _classification(
    triangle: SourceTriangle, connection: _Connection | None
) -> tuple[str, bool]:
    if triangle.geometry_status != "regular_exact":
        return "unsupported", False
    if connection is None or connection.squared_level is None:
        return "never", False
    if connection.squared_level < triangle.squared_level:
        return "before", True
    if connection.squared_level == triangle.squared_level:
        return "at", True
    return "late", False


def _record(
    triangle: SourceTriangle,
    connection: _Connection | None,
) -> dict[str, object]:
    classification, satisfied = _classification(triangle, connection)
    facets = (
        [list(facet) for facet in triangle.facets]
        if len(set(triangle.point_ids)) == 3
        else None
    )
    if classification == "unsupported":
        witness: dict[str, object] = {
            "kind": "unsupported_degeneracy",
            "geometry_status": triangle.geometry_status,
            "closed_plateau_applied_before_query": True,
        }
        connection_level = None
    else:
        assert connection is not None
        pair_levels = [_fraction_text(level) for level in connection.pair_levels]
        witness = {
            "kind": "closed_component_query",
            "pair_connection_squared_levels_exact": pair_levels,
            "final_component_labels": list(connection.final_component_labels),
            "closed_plateau_applied_before_query": True,
            "connection_level_is_pairwise_maximum": (
                connection.squared_level is not None
                and all(level is not None for level in connection.pair_levels)
                and connection.squared_level == max(connection.pair_levels)
            ),
        }
        connection_level = _fraction_text(connection.squared_level)
    return {
        "triangle_id": triangle.triangle_id,
        "point_ids": list(triangle.point_ids),
        "facets": facets,
        "source_squared_level_exact": _fraction_text(triangle.squared_level),
        "candidate_connection_squared_level_exact": connection_level,
        "classification": classification,
        "criterion_satisfied": satisfied,
        "witness": witness,
    }


def _canonical_bytes(value: object) -> bytes:
    return json.dumps(
        value,
        ensure_ascii=True,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")


def _digest(value: object) -> str:
    return hashlib.sha256(_canonical_bytes(value)).hexdigest()


def _sha256_claim(value: object, *, path: str) -> str:
    if not isinstance(value, str) or SHA256_PATTERN.fullmatch(value) is None:
        raise GuardrailInputError(f"{path} must be 64 lowercase hexadecimal digits")
    return value


def _verify_claims(
    value: object,
    *,
    digests: Mapping[str, str],
    counts: Mapping[str, int],
    guardrail_passed: bool,
) -> dict[str, object]:
    if value is None:
        return {
            "producer_claims_present": False,
            "producer_claims_verified": False,
            "checker_recomputed_all_claimable_fields": True,
        }
    claims = _mapping(value, path="claims")
    expected_keys = {
        "normalized_input_sha256",
        "classification_sha256",
        "witness_sha256",
        "counts",
        "guardrail_passed",
    }
    if set(claims) != expected_keys:
        raise GuardrailInputError(
            "claims must contain exactly normalized_input_sha256, "
            "classification_sha256, witness_sha256, counts and guardrail_passed"
        )
    for key in (
        "normalized_input_sha256",
        "classification_sha256",
        "witness_sha256",
    ):
        observed = _sha256_claim(claims.get(key), path=f"claims.{key}")
        if observed != digests[key]:
            raise GuardrailInputError(f"claims.{key} does not match recomputation")
    claimed_counts = _mapping(claims.get("counts"), path="claims.counts")
    if dict(claimed_counts) != dict(counts):
        raise GuardrailInputError("claims.counts does not match recomputation")
    claimed_passed = claims.get("guardrail_passed")
    if not isinstance(claimed_passed, bool) or claimed_passed != guardrail_passed:
        raise GuardrailInputError(
            "claims.guardrail_passed does not match recomputation"
        )
    return {
        "producer_claims_present": True,
        "producer_claims_verified": True,
        "checker_recomputed_all_claimable_fields": True,
    }


def analyze_guardrail(
    payload: object,
    *,
    source_artifact_sha256: str | None = None,
    source_artifact_byte_count: int | None = None,
) -> dict[str, object]:
    """Validate one decoded guardrail payload and return its canonical report."""

    document = _mapping(payload, path="document")
    _literal(document, "schema", INPUT_SCHEMA, path="document")
    _literal(document, "phase", "15", path="document")
    point_count = _uint(document.get("point_count"), path="point_count")
    if point_count < 3:
        raise GuardrailInputError("point_count must be at least three")
    triangles = _parse_source(document, point_count)
    candidate = _mapping(document.get("candidate"), path="candidate")
    representation = candidate.get("representation")
    if representation == NATIVE_REPRESENTATION:
        events = _parse_facet_events(candidate, point_count)
        connections, normalized_candidate, candidate_audit = _native_connections(
            triangles, events
        )
        pair_level_witness_domain = "pair_facets"
        scientific_scope = "native_unilateral_k2_facet_connectivity_guardrail"
    elif representation == POINT_TREE_REPRESENTATION:
        edges, adapter = _parse_point_tree(candidate, point_count)
        connections, normalized_candidate, candidate_audit = _point_lift_connections(
            triangles, edges, point_count, adapter
        )
        pair_level_witness_domain = "point_vertices_via_explicit_clique_lift"
        scientific_scope = (
            "explicit_point_component_clique_lift_proxy_not_native_gamma2"
        )
    else:
        raise GuardrailInputError(
            "candidate.representation is unsupported; point hierarchies require "
            "the explicit point_weighted_spanning_tree_exact_v1 representation"
        )

    normalized_source = [
        {
            "triangle_id": triangle.triangle_id,
            "point_ids": list(triangle.point_ids),
            "squared_level_exact": _fraction_text(triangle.squared_level),
            "geometry_status": triangle.geometry_status,
        }
        for triangle in triangles
    ]
    normalized_input = {
        "schema": INPUT_SCHEMA,
        "phase": "15",
        "point_count": point_count,
        "source_domain": SOURCE_DOMAIN,
        "source_level_convention": EXACT_LEVEL_CONVENTION,
        "source_triangles": normalized_source,
        "candidate": normalized_candidate,
    }
    records = [
        _record(triangle, connections.get(triangle.triangle_id))
        for triangle in triangles
    ]
    classes = ("before", "at", "late", "never", "unsupported")
    counts = {
        "source_triangle_count": len(records),
        "supported_triangle_count": sum(
            record["classification"] != "unsupported" for record in records
        ),
        **{
            f"{classification}_count": sum(
                record["classification"] == classification for record in records
            )
            for classification in classes
        },
    }
    counts["satisfied_count"] = counts["before_count"] + counts["at_count"]
    counts["violation_count"] = (
        counts["late_count"]
        + counts["never_count"]
        + counts["unsupported_count"]
    )
    guardrail_passed = counts["violation_count"] == 0
    first_failure = next(
        (record for record in records if not bool(record["criterion_satisfied"])),
        None,
    )
    first_at_level = next(
        (record for record in records if record["classification"] == "at"),
        None,
    )
    witness_payload = [
        {
            "triangle_id": record["triangle_id"],
            "classification": record["classification"],
            "witness": record["witness"],
        }
        for record in records
    ]
    digests = {
        "normalized_input_sha256": _digest(normalized_input),
        "classification_sha256": _digest(records),
        "witness_sha256": _digest(witness_payload),
    }
    claim_validation = _verify_claims(
        document.get("claims"),
        digests=digests,
        counts=counts,
        guardrail_passed=guardrail_passed,
    )
    source_artifact: dict[str, object] | None = None
    if source_artifact_sha256 is not None or source_artifact_byte_count is not None:
        if (
            source_artifact_sha256 is None
            or SHA256_PATTERN.fullmatch(source_artifact_sha256) is None
            or source_artifact_byte_count is None
            or source_artifact_byte_count < 0
        ):
            raise GuardrailInputError("source artifact metadata is inconsistent")
        source_artifact = {
            "sha256": source_artifact_sha256,
            "byte_count": source_artifact_byte_count,
        }

    return {
        "schema_version": CHECK_SCHEMA,
        "diagnostic_completed": True,
        "phase": "15",
        "backend": "python_exact_rational_fusion_guardrail",
        "profile": "hgp_reduced",
        "mode": "closed_gabriel_triangle_component_domination",
        "public_status": "not_claimed",
        "scientific_scope": scientific_scope,
        "source_artifact": source_artifact,
        "source_domain": SOURCE_DOMAIN,
        "candidate_representation": representation,
        "pair_level_witness_domain": pair_level_witness_domain,
        "point_count": point_count,
        "criterion": (
            "every_supported_exact_Gabriel_triangle_has_three_k2_facets_in_one_"
            "candidate_component_at_or_before_its_exact_closed_level"
        ),
        "boundary": "closed",
        "early_connections_accepted": True,
        "guardrail_passed": guardrail_passed,
        "counts": counts,
        "candidate_audit": candidate_audit,
        "invariants": {
            "classification_partition_closed": (
                counts["source_triangle_count"]
                == sum(counts[f"{classification}_count"] for classification in classes)
            ),
            "supported_partition_closed": (
                counts["supported_triangle_count"]
                == counts["before_count"]
                + counts["at_count"]
                + counts["late_count"]
                + counts["never_count"]
            ),
            "satisfied_partition_is_before_plus_at": (
                counts["satisfied_count"]
                == counts["before_count"] + counts["at_count"]
            ),
            "plateau_unions_applied_before_closed_query": True,
            "every_connection_witness_recomputed": True,
            "all_scientific_levels_exact_rational": True,
            "digests_recomputed_from_normalized_payload": True,
        },
        "digests": {
            "algorithm": "sha256_canonical_json_v1",
            **digests,
        },
        "claim_validation": claim_validation,
        "first_failure_witness": first_failure,
        "first_at_level_witness": first_at_level,
        "records": records,
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "diagnostic",
        nargs="?",
        default="-",
        help="guardrail JSON path, or '-' / omitted for stdin",
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
        report = analyze_guardrail(
            payload,
            source_artifact_sha256=hashlib.sha256(raw_artifact).hexdigest(),
            source_artifact_byte_count=len(raw_artifact),
        )
    except (
        GuardrailInputError,
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
    return 0 if report["guardrail_passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
