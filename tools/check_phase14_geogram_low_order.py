#!/usr/bin/env python3
"""Compare a low-order Geogram/CUDA diagnostic with exact bounded oracles.

The input is deliberately small and diagnostic-only.  For at most fourteen
points, this checker:

* compares the reported k=1 tree with the independent exact complete-graph
  EMST at every exact open and closed threshold;
* compares reported k=2 triangles with the exhaustive exact Gabriel catalog;
* compares the exact Gabriel partial filtration with exhaustive Gamma;
* replays the permanent five-point Gabriel counterexample and the independent
  ``overlap-k2`` contract fixture on every exact threshold;
* recertifies the ordinary-Delaunay tetrahedra of the permanent six-, eight-
  and nine-point fixtures, then proves the expected two-edge and local
  candidate failures plus one non-trivial positive one-edge reduction.

The k=2 universe is the set of two-point facets.  A point-MST surrogate does
not live on that universe and is therefore never assigned a Morse/Gamma
quality score by this tool.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from collections import Counter
from dataclasses import dataclass
from fractions import Fraction
from functools import lru_cache
from itertools import combinations
from pathlib import Path
from typing import Callable, Hashable, Iterable, Mapping, Sequence, TypeAlias

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from reference.morsehgp3d_oracle.gamma import (  # noqa: E402
    GammaCut,
    GammaFiltration,
    build_gamma_filtration,
)
from reference.morsehgp3d_oracle.exact import (  # noqa: E402
    affine_dimension,
    squared_distance,
)
from reference.morsehgp3d_oracle.geometry import (  # noqa: E402
    AffineDependenceError,
    BallRelation,
    circumball,
    classify,
)
from reference.morsehgp3d_oracle.hierarchy import (  # noqa: E402
    build_gabriel_partial_forest,
    build_merge_forest,
)
from tests.reference_emst import build_exhaustive_emst  # noqa: E402

SCHEMA_VERSION = "morsehgp3d.phase14_geogram_low_order_check.v1"
MAX_POINT_COUNT = 14
TRIANGLE_STATUSES = frozenset(
    {
        "gabriel_binary64",
        "ambiguous_requires_cpu_recertification",
        "degenerate_or_invalid",
    }
)
RESTRICTED_GAMMA_STATUSES = frozenset(
    {
        "blocked",
        "gabriel_binary64",
        "ambiguous_requires_cpu_recertification",
    }
)
Entity: TypeAlias = int | tuple[int, ...]
Component: TypeAlias = tuple[Entity, ...]
Point3: TypeAlias = tuple[Fraction, Fraction, Fraction]
CutProvider: TypeAlias = Callable[[Fraction, bool], "CutState"]
FacetRelation: TypeAlias = tuple[
    tuple[int, ...], tuple[tuple[int, ...], ...], Fraction
]


class DiagnosticInputError(ValueError):
    """The diagnostic JSON cannot denote the requested bounded comparison."""


@dataclass(frozen=True, order=True)
class ReportedEdge:
    u: int
    v: int
    squared_level: Fraction


@dataclass(frozen=True, order=True)
class ReportedTriangle:
    point_ids: tuple[int, int, int]
    squared_level: Fraction
    status: str


@dataclass(frozen=True)
class CutState:
    """One canonical cut on points (k=1) or facets (k=2)."""

    active_entities: tuple[Entity, ...]
    components: tuple[Component, ...]
    covered_point_ids: tuple[tuple[int, ...], ...]


class _DisjointSet:
    def __init__(self, values: Iterable[Hashable]) -> None:
        self.parent = {value: value for value in values}

    def find(self, value: Hashable) -> Hashable:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: Hashable, right: Hashable) -> bool:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return False
        if repr(left_root) < repr(right_root):
            self.parent[right_root] = left_root
        else:
            self.parent[left_root] = right_root
        return True


def _fraction(value: object, *, path: str) -> Fraction:
    if isinstance(value, bool):
        raise DiagnosticInputError(f"{path} must not be boolean")
    if isinstance(value, int):
        return Fraction(value)
    if isinstance(value, float):
        if not math.isfinite(value):
            raise DiagnosticInputError(f"{path} must be finite")
        return Fraction.from_float(0.0 if value == 0.0 else value)
    if isinstance(value, str):
        try:
            result = Fraction(value.strip())
        except (ValueError, ZeroDivisionError) as error:
            raise DiagnosticInputError(
                f"{path} must be a number or a rational string"
            ) from error
        return result
    raise DiagnosticInputError(f"{path} must be a number or a rational string")


def _fraction_from_record(value: object, *, path: str) -> Fraction:
    """Read fixture rationals while keeping the public input schema minimal."""

    if isinstance(value, Mapping):
        try:
            numerator = value["numerator"]
            denominator = value["denominator"]
        except KeyError as error:
            raise DiagnosticInputError(f"{path} is not a rational record") from error
        if (
            isinstance(numerator, bool)
            or not isinstance(numerator, int)
            or isinstance(denominator, bool)
            or not isinstance(denominator, int)
            or denominator == 0
        ):
            raise DiagnosticInputError(f"{path} is not a rational record")
        return Fraction(numerator, denominator)
    return _fraction(value, path=path)


def _fraction_json(value: Fraction) -> str:
    return (
        str(value.numerator)
        if value.denominator == 1
        else f"{value.numerator}/{value.denominator}"
    )


def _require_mapping(value: object, *, path: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        raise DiagnosticInputError(f"{path} must be an object")
    if any(not isinstance(key, str) for key in value):
        raise DiagnosticInputError(f"{path} keys must be strings")
    return value  # type: ignore[return-value]


def _require_list(value: object, *, path: str) -> list[object]:
    if not isinstance(value, list):
        raise DiagnosticInputError(f"{path} must be an array")
    return value


def _point(value: object, *, path: str) -> Point3:
    if isinstance(value, Mapping):
        if "coordinates" in value:
            value = value["coordinates"]
        elif all(axis in value for axis in ("x", "y", "z")):
            value = [value["x"], value["y"], value["z"]]
    coordinates = _require_list(value, path=path)
    if len(coordinates) != 3:
        raise DiagnosticInputError(f"{path} must contain exactly three coordinates")
    return tuple(
        _fraction(coordinate, path=f"{path}[{axis}]")
        for axis, coordinate in enumerate(coordinates)
    )  # type: ignore[return-value]


def _parse_points(payload: Mapping[str, object]) -> tuple[Point3, ...]:
    input_record = _require_mapping(payload.get("input"), path="input")
    if input_record.get("points") is None:
        raise DiagnosticInputError(
            "input.points is null; rerun the producer with --emit-records"
        )
    raw_points = _require_list(input_record.get("points"), path="input.points")
    if not 3 <= len(raw_points) <= MAX_POINT_COUNT:
        raise DiagnosticInputError(
            f"input.points must contain between 3 and {MAX_POINT_COUNT} points"
        )
    points = tuple(
        _point(point, path=f"input.points[{index}]")
        for index, point in enumerate(raw_points)
    )
    if len(set(points)) != len(points):
        raise DiagnosticInputError("input.points must contain distinct sites")
    return points


def _point_id(value: object, *, path: str, point_count: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise DiagnosticInputError(f"{path} must be an integer point identifier")
    if not 0 <= value < point_count:
        raise DiagnosticInputError(f"{path} lies outside 0..{point_count - 1}")
    return value


def _parse_edges(
    payload: Mapping[str, object], point_count: int
) -> tuple[ReportedEdge, ...]:
    k1 = _require_mapping(payload.get("k1"), path="k1")
    if k1.get("selected_edges") is None:
        raise DiagnosticInputError(
            "k1.selected_edges is null; rerun the producer with --emit-records"
        )
    records = _require_list(k1.get("selected_edges"), path="k1.selected_edges")
    edges = []
    seen: set[tuple[int, int]] = set()
    for index, raw_record in enumerate(records):
        path = f"k1.selected_edges[{index}]"
        record = _require_mapping(raw_record, path=path)
        u = _point_id(record.get("u"), path=f"{path}.u", point_count=point_count)
        v = _point_id(record.get("v"), path=f"{path}.v", point_count=point_count)
        if u == v:
            raise DiagnosticInputError(f"{path} must not be a self-edge")
        u, v = sorted((u, v))
        if (u, v) in seen:
            raise DiagnosticInputError(f"{path} duplicates edge ({u}, {v})")
        seen.add((u, v))
        level = _fraction(record.get("squared_level"), path=f"{path}.squared_level")
        if level < 0:
            raise DiagnosticInputError(f"{path}.squared_level must be non-negative")
        edges.append(ReportedEdge(u, v, level))
    return tuple(sorted(edges))


def _parse_triangle_array(
    value: object,
    *,
    path: str,
    point_count: int,
    allowed_statuses: frozenset[str] = TRIANGLE_STATUSES,
) -> tuple[ReportedTriangle, ...]:
    records = _require_list(value, path=path)
    triangles = []
    seen: set[tuple[int, int, int]] = set()
    for index, raw_record in enumerate(records):
        record_path = f"{path}[{index}]"
        record = _require_mapping(raw_record, path=record_path)
        point_ids = tuple(
            sorted(
                (
                    _point_id(
                        record.get(name),
                        path=f"{record_path}.{name}",
                        point_count=point_count,
                    )
                    for name in ("a", "b", "c")
                )
            )
        )
        if len(set(point_ids)) != 3:
            raise DiagnosticInputError(
                f"{record_path} must contain three distinct vertices"
            )
        if point_ids in seen:
            raise DiagnosticInputError(f"{record_path} duplicates triangle {point_ids}")
        seen.add(point_ids)
        level = _fraction(
            record.get("squared_level"), path=f"{record_path}.squared_level"
        )
        if level < 0:
            raise DiagnosticInputError(
                f"{record_path}.squared_level must be non-negative"
            )
        status = record.get("status")
        if not isinstance(status, str) or not status.strip():
            raise DiagnosticInputError(
                f"{record_path}.status must be a non-empty string"
            )
        normalized_status = status.strip().lower().replace("-", "_")
        if "surrogate" in normalized_status:
            raise DiagnosticInputError(
                f"{record_path}.status attempts to place a k=2 surrogate on the facet universe"
            )
        if normalized_status not in allowed_statuses:
            raise DiagnosticInputError(
                f"{record_path}.status={status!r} is unsupported; expected one of "
                f"{sorted(allowed_statuses)}"
            )
        triangles.append(ReportedTriangle(point_ids, level, normalized_status))
    return tuple(sorted(triangles))


def _parse_triangles(
    payload: Mapping[str, object], point_count: int
) -> tuple[ReportedTriangle, ...]:
    k2 = _require_mapping(payload.get("k2"), path="k2")
    if k2.get("accepted_triangles") is None:
        raise DiagnosticInputError(
            "k2.accepted_triangles is null; rerun the producer with --emit-records"
        )
    return _parse_triangle_array(
        k2.get("accepted_triangles"),
        path="k2.accepted_triangles",
        point_count=point_count,
    )


def _parse_restricted_gamma_records(
    payload: Mapping[str, object], point_count: int
) -> tuple[ReportedTriangle, ...] | None:
    k2 = _require_mapping(payload.get("k2"), path="k2")
    if "restricted_gamma_records" not in k2:
        return None
    if k2.get("restricted_gamma_records") is None:
        raise DiagnosticInputError(
            "k2.restricted_gamma_records is null; rerun the producer with --emit-records"
        )
    return _parse_triangle_array(
        k2.get("restricted_gamma_records"),
        path="k2.restricted_gamma_records",
        point_count=point_count,
        allowed_statuses=RESTRICTED_GAMMA_STATUSES,
    )


def _reject_surrogate_comparability(payload: Mapping[str, object]) -> None:
    """Fail closed on an explicit attempt to equate point and facet models."""

    comparable_keys = {
        "comparable_to_surrogate",
        "surrogate_comparable",
        "k2_surrogate_comparable",
        "same_universe_as_surrogate",
    }

    def visit(value: object, path: str) -> None:
        if isinstance(value, Mapping):
            for raw_key, child in value.items():
                key = str(raw_key)
                normalized = key.lower().replace("-", "_")
                child_path = f"{path}.{key}" if path else key
                if normalized in comparable_keys and child not in (False, None):
                    raise DiagnosticInputError(
                        f"{child_path} cannot claim k=2 surrogate comparability"
                    )
                visit(child, child_path)
        elif isinstance(value, list):
            for index, child in enumerate(value):
                visit(child, f"{path}[{index}]")

    visit(payload, "")
    for path, value in (
        ("backend", payload.get("backend")),
        ("profile", payload.get("profile")),
        ("mode", payload.get("mode")),
    ):
        if isinstance(value, str) and "surrogate" in value.lower():
            raise DiagnosticInputError(
                f"{path}={value!r} is not a k=2 facet-relation diagnostic"
            )
    k2 = _require_mapping(payload.get("k2"), path="k2")
    if k2.get("comparison_universe") == "points":
        raise DiagnosticInputError(
            "k2.comparison_universe must be facets, never point clusters"
        )


def _components_from_dsu(
    disjoint_set: _DisjointSet, values: Iterable[Entity]
) -> tuple[Component, ...]:
    grouped: dict[Hashable, list[Entity]] = {}
    for value in sorted(values):
        grouped.setdefault(disjoint_set.find(value), []).append(value)
    return tuple(
        sorted(
            (tuple(sorted(group)) for group in grouped.values()),
        )
    )


def _point_cut(
    point_count: int,
    edges: Sequence[ReportedEdge],
    level: Fraction,
    closed: bool,
) -> CutState:
    vertices_active = level >= 0 if closed else level > 0
    if not vertices_active:
        return CutState((), (), ())
    points = tuple(range(point_count))
    disjoint_set = _DisjointSet(points)
    for edge in edges:
        active = edge.squared_level <= level if closed else edge.squared_level < level
        if active:
            disjoint_set.union(edge.u, edge.v)
    components = _components_from_dsu(disjoint_set, points)
    covers = tuple(
        tuple(int(point_id) for point_id in component) for component in components
    )
    return CutState(points, components, covers)


def _emst_cut(result: object, level: Fraction, closed: bool) -> CutState:
    cut = result.cut(level, closed=closed, edge_source="selected_emst")
    components: tuple[Component, ...] = tuple(
        tuple(component) for component in cut.components
    )
    active = tuple(point_id for component in components for point_id in component)
    covers = tuple(
        tuple(int(point_id) for point_id in component) for component in components
    )
    return CutState(tuple(sorted(active)), components, covers)


def _relation_cut(
    relations: Sequence[FacetRelation],
    level: Fraction,
    closed: bool,
) -> CutState:
    active_relations = tuple(
        relation
        for relation in relations
        if (relation[2] <= level if closed else relation[2] < level)
    )
    active_facets: set[tuple[int, ...]] = {
        facet for _, facets, _ in active_relations for facet in facets
    }
    disjoint_set = _DisjointSet(active_facets)
    for _, facets, _ in active_relations:
        for facet in facets[1:]:
            disjoint_set.union(facets[0], facet)
    components = _components_from_dsu(disjoint_set, active_facets)
    covers = tuple(
        tuple(
            sorted(
                {
                    point_id
                    for entity in component
                    for point_id in entity  # type: ignore[union-attr]
                }
            )
        )
        for component in components
    )
    return CutState(tuple(sorted(active_facets)), components, covers)


def _gabriel_fusion_deadline_coverage(
    *,
    variant_name: str,
    source_relations: Sequence[FacetRelation],
    candidate_relations: Sequence[FacetRelation],
) -> dict[str, object]:
    """Check the closed, one-sided Gabriel fusion deadline on k=2 facets.

    This deliberately does not compare complete cuts.  Each source triangle is
    queried only after every candidate relation on a plateau has been united;
    an earlier or equal-level connection succeeds, while a later or absent
    connection fails.
    """

    if not variant_name:
        raise DiagnosticInputError("a Gabriel deadline variant name is required")

    def validate_relation(
        relation: FacetRelation, *, path: str
    ) -> FacetRelation:
        point_ids, facets, level = relation
        if len(point_ids) != 3 or tuple(sorted(point_ids)) != point_ids:
            raise DiagnosticInputError(
                f"{path} must identify one sorted three-point triangle"
            )
        expected_facets = tuple(sorted(combinations(point_ids, 2)))
        if tuple(sorted(facets)) != expected_facets:
            raise DiagnosticInputError(
                f"{path} must contain exactly the three triangle facets"
            )
        if level < 0:
            raise DiagnosticInputError(f"{path} has a negative squared level")
        return point_ids, expected_facets, level

    sources = tuple(
        validate_relation(relation, path=f"source_relations[{index}]")
        for index, relation in enumerate(source_relations)
    )
    candidates = tuple(
        sorted(
            (
                validate_relation(
                    relation, path=f"candidate_relations[{index}]"
                )
                for index, relation in enumerate(candidate_relations)
            ),
            key=lambda relation: (relation[2], relation[0]),
        )
    )
    source_ids = [relation[0] for relation in sources]
    if len(set(source_ids)) != len(source_ids):
        raise DiagnosticInputError(
            "source_relations contains duplicate Gabriel triangle identities"
        )
    candidate_ids = [relation[0] for relation in candidates]
    if len(set(candidate_ids)) != len(candidate_ids):
        raise DiagnosticInputError(
            "candidate_relations contains duplicate triangle identities"
        )

    all_facets = {
        facet
        for _, facets, _ in (*sources, *candidates)
        for facet in facets
    }
    decisions: list[dict[str, object]] = []
    for source_point_ids, source_facets, source_level in sorted(
        sources, key=lambda relation: (relation[2], relation[0])
    ):
        disjoint_set = _DisjointSet(all_facets)
        connection_level: Fraction | None = None
        plateau_begin = 0
        while plateau_begin < len(candidates):
            plateau_level = candidates[plateau_begin][2]
            plateau_end = plateau_begin + 1
            while (
                plateau_end < len(candidates)
                and candidates[plateau_end][2] == plateau_level
            ):
                plateau_end += 1
            for _, facets, _ in candidates[plateau_begin:plateau_end]:
                for facet in facets[1:]:
                    disjoint_set.union(facets[0], facet)
            if len({disjoint_set.find(facet) for facet in source_facets}) == 1:
                connection_level = plateau_level
                break
            plateau_begin = plateau_end

        if connection_level is None:
            decision = "never"
        elif connection_level < source_level:
            decision = "before"
        elif connection_level == source_level:
            decision = "at"
        else:
            decision = "late"
        decisions.append(
            {
                "source_point_ids": list(source_point_ids),
                "source_squared_level": _fraction_json(source_level),
                "connection_squared_level": (
                    None
                    if connection_level is None
                    else _fraction_json(connection_level)
                ),
                "decision": decision,
            }
        )

    counts = Counter(str(decision["decision"]) for decision in decisions)
    decision_builder = hashlib.sha256(
        b"MorseHGP3D/gabriel-fusion-deadline/bounded-exact/v1/sha256/"
    )
    decision_builder.update(variant_name.encode("utf-8"))
    for decision in decisions:
        decision_builder.update(
            json.dumps(
                decision, sort_keys=True, separators=(",", ":")
            ).encode("utf-8")
        )
        decision_builder.update(b"\n")

    late_or_never = tuple(
        decision
        for decision in decisions
        if decision["decision"] in {"late", "never"}
    )
    return {
        "criterion": "gabriel_fusion_deadline_v1",
        "variant": variant_name,
        "representation": "k2_facets",
        "adapter": "identity_k2_facets_v1",
        "level_convention": "exact_squared_cech_radius",
        "boundary": "closed_post_plateau",
        "early_connection_allowed": True,
        "source_count": len(decisions),
        "connected_before_count": counts["before"],
        "connected_at_count": counts["at"],
        "late_count": counts["late"],
        "never_connected_count": counts["never"],
        "unsupported_degeneracy_count": 0,
        "counter_partition_closed": (
            len(decisions)
            == counts["before"]
            + counts["at"]
            + counts["late"]
            + counts["never"]
        ),
        "passed": not late_or_never,
        "decision_sha256": decision_builder.hexdigest(),
        "first_failure": late_or_never[0] if late_or_never else None,
        "decisions": decisions,
        "exact_Gamma2_claimed": False,
        "public_status_claimed": False,
    }


def _gamma_cut_state(cut: GammaCut) -> CutState:
    components: tuple[Component, ...] = tuple(
        tuple(component.facet_point_ids) for component in cut.components
    )
    return CutState(
        tuple(cut.active_facet_point_ids),
        components,
        tuple(component.covered_point_ids for component in cut.components),
    )


def _gamma_provider(filtration: GammaFiltration, graph_kind: str) -> CutProvider:
    def provide(level: Fraction, closed: bool) -> CutState:
        return _gamma_cut_state(
            filtration.cut(
                level,
                closed=closed,
                graph_kind=graph_kind,  # type: ignore[arg-type]
                include_isolated=False,
            )
        )

    return provide


def _ratio(numerator: int, denominator: int, *, empty: float = 1.0) -> float:
    return numerator / denominator if denominator else empty


def _classification_metrics(tp: int, fp: int, fn: int, tn: int) -> dict[str, object]:
    precision = _ratio(tp, tp + fp, empty=1.0 if fn == 0 else 0.0)
    recall = _ratio(tp, tp + fn, empty=1.0)
    return {
        "true_positive": tp,
        "false_positive": fp,
        "false_negative": fn,
        "true_negative": tn,
        "precision": precision,
        "recall": recall,
        "f1": _ratio(2 * tp, 2 * tp + fp + fn, empty=1.0),
        "jaccard": _ratio(tp, tp + fp + fn, empty=1.0),
    }


def _entity_json(entity: Entity) -> object:
    return list(entity) if isinstance(entity, tuple) else entity


def _components_json(components: tuple[Component, ...]) -> list[list[object]]:
    return [[_entity_json(entity) for entity in component] for component in components]


def _best_component_jaccard(
    source: tuple[Component, ...], target: tuple[Component, ...]
) -> float:
    if not source:
        return 1.0 if not target else 0.0
    if not target:
        return 0.0
    scores = []
    target_sets = tuple(set(component) for component in target)
    for component in source:
        source_set = set(component)
        scores.append(
            max(
                len(source_set & target_set) / len(source_set | target_set)
                for target_set in target_sets
            )
        )
    return sum(scores) / len(scores)


def _state_metrics(
    predicted: CutState,
    reference: CutState,
    universe: Sequence[Entity],
    point_count: int,
) -> dict[str, object]:
    predicted_active = set(predicted.active_entities)
    reference_active = set(reference.active_entities)
    active_tp = len(predicted_active & reference_active)
    active_fp = len(predicted_active - reference_active)
    active_fn = len(reference_active - predicted_active)
    active_tn = len(set(universe) - predicted_active - reference_active)

    predicted_component = {
        entity: index
        for index, component in enumerate(predicted.components)
        for entity in component
    }
    reference_component = {
        entity: index
        for index, component in enumerate(reference.components)
        for entity in component
    }
    pair_tp = pair_fp = pair_fn = pair_tn = 0
    for left, right in combinations(universe, 2):
        predicted_same = (
            left in predicted_component
            and right in predicted_component
            and predicted_component[left] == predicted_component[right]
        )
        reference_same = (
            left in reference_component
            and right in reference_component
            and reference_component[left] == reference_component[right]
        )
        if predicted_same and reference_same:
            pair_tp += 1
        elif predicted_same:
            pair_fp += 1
        elif reference_same:
            pair_fn += 1
        else:
            pair_tn += 1

    predicted_component_set = set(predicted.components)
    reference_component_set = set(reference.components)
    predicted_multiplicity = [
        sum(point_id in cover for cover in predicted.covered_point_ids)
        for point_id in range(point_count)
    ]
    reference_multiplicity = [
        sum(point_id in cover for cover in reference.covered_point_ids)
        for point_id in range(point_count)
    ]
    multiplicity_errors = [
        abs(left - right)
        for left, right in zip(predicted_multiplicity, reference_multiplicity)
    ]
    partition_exact = predicted.components == reference.components
    covers_exact = predicted.covered_point_ids == reference.covered_point_ids
    active_exact = predicted.active_entities == reference.active_entities
    return {
        "state_exact": partition_exact and covers_exact and active_exact,
        "component_partition_exact": partition_exact,
        "active_entities_exact": active_exact,
        "covered_point_collections_exact": covers_exact,
        "predicted_component_count": len(predicted.components),
        "reference_component_count": len(reference.components),
        "exact_component_match_count": len(
            predicted_component_set & reference_component_set
        ),
        "predicted_to_reference_best_component_jaccard": _best_component_jaccard(
            predicted.components, reference.components
        ),
        "reference_to_predicted_best_component_jaccard": _best_component_jaccard(
            reference.components, predicted.components
        ),
        "active_entities": _classification_metrics(
            active_tp, active_fp, active_fn, active_tn
        ),
        "pair_comembership": _classification_metrics(
            pair_tp, pair_fp, pair_fn, pair_tn
        ),
        "root_multiplicity_l1": sum(multiplicity_errors),
        "root_multiplicity_max": max(multiplicity_errors, default=0),
    }


def _compare_cuts(
    *,
    predicted_name: str,
    reference_name: str,
    levels: Iterable[Fraction],
    predicted_provider: CutProvider,
    reference_provider: CutProvider,
    universe: Sequence[Entity],
    point_count: int,
) -> dict[str, object]:
    canonical_levels = tuple(sorted(set(levels) | {Fraction(0)}))
    states = []
    pair_totals = Counter({"tp": 0, "fp": 0, "fn": 0, "tn": 0})
    active_totals = Counter({"tp": 0, "fp": 0, "fn": 0, "tn": 0})
    exact_count = strict_exact_count = closed_exact_count = 0
    partition_exact_count = active_exact_count = covers_exact_count = 0
    component_jaccard_sum = 0.0
    component_jaccard_reverse_sum = 0.0
    multiplicity_l1_sum = 0
    multiplicity_max = 0
    first_divergence = None

    for level in canonical_levels:
        for closed in (False, True):
            predicted = predicted_provider(level, closed)
            reference = reference_provider(level, closed)
            metrics = _state_metrics(predicted, reference, universe, point_count)
            state = {
                "squared_level": _fraction_json(level),
                "boundary": "closed" if closed else "strict",
                **metrics,
            }
            states.append(state)
            exact = bool(metrics["state_exact"])
            exact_count += exact
            strict_exact_count += exact and not closed
            closed_exact_count += exact and closed
            partition_exact_count += bool(metrics["component_partition_exact"])
            active_exact_count += bool(metrics["active_entities_exact"])
            covers_exact_count += bool(metrics["covered_point_collections_exact"])
            component_jaccard_sum += float(
                metrics["predicted_to_reference_best_component_jaccard"]
            )
            component_jaccard_reverse_sum += float(
                metrics["reference_to_predicted_best_component_jaccard"]
            )
            multiplicity_l1_sum += int(metrics["root_multiplicity_l1"])
            multiplicity_max = max(
                multiplicity_max, int(metrics["root_multiplicity_max"])
            )
            pair = metrics["pair_comembership"]
            active = metrics["active_entities"]
            for short, key in (
                ("tp", "true_positive"),
                ("fp", "false_positive"),
                ("fn", "false_negative"),
                ("tn", "true_negative"),
            ):
                pair_totals[short] += int(pair[key])  # type: ignore[index]
                active_totals[short] += int(active[key])  # type: ignore[index]
            if not exact and first_divergence is None:
                first_divergence = {
                    "squared_level": _fraction_json(level),
                    "boundary": "closed" if closed else "strict",
                    "predicted_components": _components_json(predicted.components),
                    "reference_components": _components_json(reference.components),
                    "predicted_covered_point_ids": [
                        list(cover) for cover in predicted.covered_point_ids
                    ],
                    "reference_covered_point_ids": [
                        list(cover) for cover in reference.covered_point_ids
                    ],
                }

    state_count = len(states)
    return {
        "predicted": predicted_name,
        "reference": reference_name,
        "level_count": len(canonical_levels),
        "state_count": state_count,
        "strict_state_count": len(canonical_levels),
        "closed_state_count": len(canonical_levels),
        "all_states_exact": exact_count == state_count,
        "exact_state_count": exact_count,
        "strict_exact_state_count": strict_exact_count,
        "closed_exact_state_count": closed_exact_count,
        "component_partition_exact_state_count": partition_exact_count,
        "active_entities_exact_state_count": active_exact_count,
        "covered_point_collections_exact_state_count": covers_exact_count,
        "first_divergence": first_divergence,
        "aggregate_pair_comembership": _classification_metrics(
            pair_totals["tp"],
            pair_totals["fp"],
            pair_totals["fn"],
            pair_totals["tn"],
        ),
        "aggregate_active_entities": _classification_metrics(
            active_totals["tp"],
            active_totals["fp"],
            active_totals["fn"],
            active_totals["tn"],
        ),
        "mean_predicted_to_reference_best_component_jaccard": (
            component_jaccard_sum / state_count if state_count else 1.0
        ),
        "mean_reference_to_predicted_best_component_jaccard": (
            component_jaccard_reverse_sum / state_count if state_count else 1.0
        ),
        "root_multiplicity_l1_total": multiplicity_l1_sum,
        "root_multiplicity_max": multiplicity_max,
        "states": states,
    }


def _first_divergence_signature(
    metrics: Mapping[str, object],
) -> tuple[str, str] | None:
    divergence = metrics.get("first_divergence")
    if not isinstance(divergence, Mapping):
        return None
    squared_level = divergence.get("squared_level")
    boundary = divergence.get("boundary")
    if not isinstance(squared_level, str) or not isinstance(boundary, str):
        return None
    return squared_level, boundary


def _tree_shape(point_count: int, edges: Sequence[ReportedEdge]) -> dict[str, object]:
    disjoint_set = _DisjointSet(range(point_count))
    cycle_count = 0
    for edge in edges:
        if not disjoint_set.union(edge.u, edge.v):
            cycle_count += 1
    component_count = len(
        {disjoint_set.find(point_id) for point_id in range(point_count)}
    )
    return {
        "edge_count": len(edges),
        "expected_tree_edge_count": point_count - 1,
        "cycle_edge_count": cycle_count,
        "final_component_count": component_count,
        "is_spanning_tree": (
            len(edges) == point_count - 1 and cycle_count == 0 and component_count == 1
        ),
    }


def _analyze_k1(
    points: tuple[Point3, ...], edges: tuple[ReportedEdge, ...]
) -> dict[str, object]:
    exact = build_exhaustive_emst(points)
    exact_level_by_pair = {
        edge.point_ids: edge.squared_level for edge in exact.complete_edges
    }
    mismatched_levels = []
    for edge in edges:
        exact_level = exact_level_by_pair[(edge.u, edge.v)]
        if edge.squared_level != exact_level:
            mismatched_levels.append(
                {
                    "point_ids": [edge.u, edge.v],
                    "reported_squared_level": _fraction_json(edge.squared_level),
                    "exact_squared_level": _fraction_json(exact_level),
                }
            )
    levels = (
        set(exact.replay_levels)
        | {edge.squared_level for edge in edges}
        | set(exact_level_by_pair.values())
    )
    recertified_edges = tuple(
        ReportedEdge(edge.u, edge.v, exact_level_by_pair[(edge.u, edge.v)])
        for edge in edges
    )
    reported_cut_metrics = _compare_cuts(
        predicted_name="reported_geogram_selected_edges",
        reference_name="independent_exact_complete_graph_emst",
        levels=levels,
        predicted_provider=lambda level, closed: _point_cut(
            len(points), edges, level, closed
        ),
        reference_provider=lambda level, closed: _emst_cut(exact, level, closed),
        universe=tuple(range(len(points))),
        point_count=len(points),
    )
    recertified_cut_metrics = _compare_cuts(
        predicted_name="geogram_selected_edge_topology_with_exact_recomputed_levels",
        reference_name="independent_exact_complete_graph_emst",
        levels=levels,
        predicted_provider=lambda level, closed: _point_cut(
            len(points), recertified_edges, level, closed
        ),
        reference_provider=lambda level, closed: _emst_cut(exact, level, closed),
        universe=tuple(range(len(points))),
        point_count=len(points),
    )
    tree_shape = _tree_shape(len(points), edges)
    reported_total = sum((edge.squared_level for edge in edges), Fraction(0))
    recertified_total = sum(
        (edge.squared_level for edge in recertified_edges), Fraction(0)
    )
    exact_total = exact.total_hgp_weight
    certified_for_input = (
        bool(tree_shape["is_spanning_tree"])
        and recertified_total == exact_total
        and bool(recertified_cut_metrics["all_states_exact"])
    )
    return {
        "semantic_scope": "k1_point_partitions",
        "reported_level_convention": "HGP squared level; exact pair level is squared Euclidean distance divided by four",
        "tree_shape": tree_shape,
        "endpoint_level_exact_count": len(edges) - len(mismatched_levels),
        "endpoint_level_mismatch_count": len(mismatched_levels),
        "endpoint_level_mismatches": mismatched_levels,
        "reported_total_squared_level": _fraction_json(reported_total),
        "recertified_total_squared_level": _fraction_json(recertified_total),
        "exact_emst_total_squared_level": _fraction_json(exact_total),
        "reported_total_squared_level_exact": reported_total == exact_total,
        "recertified_total_squared_level_exact": recertified_total == exact_total,
        "reported_cut_metrics": reported_cut_metrics,
        "recertified_cut_metrics": recertified_cut_metrics,
        "certified_for_this_bounded_input": certified_for_input,
        "global_geogram_certification": False,
    }


def _catalog_metrics(
    filtration: GammaFiltration,
    triangles: tuple[ReportedTriangle, ...],
    points: tuple[Point3, ...],
) -> dict[str, object]:
    exact_coface_by_simplex = {
        coface.point_ids: coface for coface in filtration.cofaces
    }
    exact_gabriel_by_simplex = {
        edge.simplex_point_ids: edge for edge in filtration.gabriel_hyperedges
    }
    reported_by_simplex = {triangle.point_ids: triangle for triangle in triangles}
    binary_ids = {
        triangle.point_ids
        for triangle in triangles
        if triangle.status == "gabriel_binary64"
    }
    ambiguous_ids = {
        triangle.point_ids
        for triangle in triangles
        if triangle.status == "ambiguous_requires_cpu_recertification"
    }
    invalid_ids = {
        triangle.point_ids
        for triangle in triangles
        if triangle.status == "degenerate_or_invalid"
    }
    exact_ids = set(exact_gabriel_by_simplex)
    replay_ids = binary_ids | (ambiguous_ids & exact_ids)
    fully_recertified_ids = (binary_ids | ambiguous_ids) & exact_ids
    true_ids = sorted(replay_ids & exact_ids)
    false_ids = sorted(replay_ids - exact_ids)
    missing_ids = sorted(exact_ids - replay_ids)
    binary_level_mismatches = []
    ambiguous_level_corrections = []
    for simplex in true_ids:
        reported = reported_by_simplex[simplex]
        exact = exact_gabriel_by_simplex[simplex]
        if reported.squared_level != exact.squared_level:
            mismatch = {
                "simplex_point_ids": list(simplex),
                "reported_squared_level": _fraction_json(reported.squared_level),
                "exact_squared_level": _fraction_json(exact.squared_level),
                "timing_direction": (
                    "early_unsafe"
                    if reported.squared_level < exact.squared_level
                    else "late_partial"
                ),
            }
            if reported.status == "gabriel_binary64":
                binary_level_mismatches.append(mismatch)
            else:
                ambiguous_level_corrections.append(mismatch)
    false_positives = []
    for simplex in false_ids:
        coface = exact_coface_by_simplex[simplex]
        external_ids = sorted(set(range(len(points))) - set(simplex))
        strict_interior_ids = [
            point_id
            for point_id in external_ids
            if sum(
                (coordinate - center_coordinate) ** 2
                for coordinate, center_coordinate in zip(
                    points[point_id], coface.center
                )
            )
            < coface.squared_level
        ]
        reported = reported_by_simplex[simplex]
        false_positives.append(
            {
                "simplex_point_ids": list(simplex),
                "status": reported.status,
                "reported_squared_level": _fraction_json(reported.squared_level),
                "exact_coface_squared_level": _fraction_json(coface.squared_level),
                "strict_interior_external_point_ids": strict_interior_ids,
            }
        )
    total_triangle_count = math.comb(len(points), 3)

    def classification(candidate_ids: set[tuple[int, int, int]]) -> dict[str, object]:
        tp = len(candidate_ids & exact_ids)
        fp = len(candidate_ids - exact_ids)
        fn = len(exact_ids - candidate_ids)
        tn = total_triangle_count - tp - fp - fn
        return _classification_metrics(tp, fp, fn, tn)

    ambiguous_resolutions = [
        {
            "simplex_point_ids": list(simplex),
            "exact_decision": "gabriel" if simplex in exact_ids else "not_gabriel",
            "reported_squared_level": _fraction_json(
                reported_by_simplex[simplex].squared_level
            ),
            "exact_coface_squared_level": _fraction_json(
                exact_coface_by_simplex[simplex].squared_level
            ),
        }
        for simplex in sorted(ambiguous_ids)
    ]
    binary_early_level_count = sum(
        mismatch["timing_direction"] == "early_unsafe"
        for mismatch in binary_level_mismatches
    )
    return {
        "reported_triangle_count": len(triangles),
        "exact_gabriel_triangle_count": len(exact_ids),
        "all_triangle_count": total_triangle_count,
        "gabriel_binary64_identity_classification": classification(binary_ids),
        "binary64_plus_exact_ambiguity_identity_classification": classification(
            replay_ids
        ),
        "fully_exactly_recertified_identity_classification": classification(
            fully_recertified_ids
        ),
        "binary64_true_positive_level_exact_count": len(binary_ids & exact_ids)
        - len(binary_level_mismatches),
        "binary64_true_positive_level_mismatch_count": len(binary_level_mismatches),
        "binary64_early_unsafe_level_count": binary_early_level_count,
        "binary64_late_partial_level_count": len(binary_level_mismatches)
        - binary_early_level_count,
        "binary64_level_mismatches": binary_level_mismatches,
        "ambiguous_reported_level_correction_count": len(ambiguous_level_corrections),
        "ambiguous_reported_level_corrections": ambiguous_level_corrections,
        "false_positive_triangles": false_positives,
        "missing_exact_gabriel_triangles": [list(simplex) for simplex in missing_ids],
        "ambiguous_exact_resolutions": ambiguous_resolutions,
        "degenerate_or_invalid_triangles": [
            list(simplex) for simplex in sorted(invalid_ids)
        ],
        "status_counts": dict(sorted(Counter(t.status for t in triangles).items())),
        "positive_inclusion_safe": not false_ids and binary_early_level_count == 0,
        "catalog_exact_after_ambiguity_recertification": (
            not false_ids and not missing_ids and not binary_level_mismatches
        ),
    }


def _relations_from_triangles(
    triangles: Iterable[ReportedTriangle],
) -> tuple[tuple[tuple[int, ...], tuple[tuple[int, ...], ...], Fraction], ...]:
    return tuple(
        (
            triangle.point_ids,
            tuple(
                sorted(
                    triangle.point_ids[:index] + triangle.point_ids[index + 1 :]
                    for index in range(3)
                )
            ),
            triangle.squared_level,
        )
        for triangle in triangles
    )


def _analyze_restricted_gamma(
    filtration: GammaFiltration,
    records: tuple[ReportedTriangle, ...] | None,
    *,
    point_count: int,
    facet_universe: tuple[Entity, ...],
) -> dict[str, object]:
    """Recertify the Delaunay-wedge coface subset before replaying Gamma2."""

    if records is None:
        return {
            "available": False,
            "comparison_performed": False,
            "reason": "k2.restricted_gamma_records is absent from this legacy diagnostic",
            "include_isolated": False,
        }

    exact_by_simplex = {coface.point_ids: coface for coface in filtration.cofaces}
    restricted_ids = {record.point_ids for record in records}
    exact_ids = set(exact_by_simplex)
    missing_ids = sorted(exact_ids - restricted_ids)
    level_mismatches = []
    early_count = 0
    for record in records:
        exact = exact_by_simplex[record.point_ids]
        if record.squared_level == exact.squared_level:
            continue
        early = record.squared_level < exact.squared_level
        early_count += early
        level_mismatches.append(
            {
                "simplex_point_ids": list(record.point_ids),
                "status": record.status,
                "reported_squared_level": _fraction_json(record.squared_level),
                "exact_squared_level": _fraction_json(exact.squared_level),
                "timing_direction": "early_unsafe" if early else "late_partial",
            }
        )

    recertified_relations = tuple(
        sorted(
            (
                (
                    exact_by_simplex[record.point_ids].point_ids,
                    exact_by_simplex[record.point_ids].facet_point_ids,
                    exact_by_simplex[record.point_ids].squared_level,
                )
                for record in records
            ),
            key=lambda relation: (relation[2], relation[0]),
        )
    )
    levels = set(filtration.critical_levels) | {
        record.squared_level for record in records
    }
    cut_metrics = _compare_cuts(
        predicted_name="restricted_Delaunay_wedge_Gamma2_with_exactly_recertified_levels",
        reference_name="exhaustive_exact_Gamma2_hgp_reduced",
        levels=levels,
        predicted_provider=lambda level, closed: _relation_cut(
            recertified_relations, level, closed
        ),
        reference_provider=_gamma_provider(filtration, "gamma"),
        universe=facet_universe,
        point_count=point_count,
    )
    coface_classification = _classification_metrics(
        len(restricted_ids), 0, len(missing_ids), 0
    )
    return {
        "available": True,
        "comparison_performed": True,
        "semantic_scope": "Gamma2 restricted to valid ordinary-Delaunay CSR wedges",
        "include_isolated": False,
        "reported_record_count": len(records),
        "exact_exhaustive_coface_count": len(exact_ids),
        "coface_universe_classification": coface_classification,
        "coface_universe_complete_for_this_input": not missing_ids,
        "missing_exact_cofaces": [list(simplex) for simplex in missing_ids],
        "status_counts": dict(
            sorted(Counter(record.status for record in records).items())
        ),
        "reported_level_exact_count": len(records) - len(level_mismatches),
        "reported_level_mismatch_count": len(level_mismatches),
        "reported_level_early_unsafe_count": early_count,
        "reported_level_late_partial_count": len(level_mismatches) - early_count,
        "reported_level_mismatches": level_mismatches,
        "recertification": {
            "performed": True,
            "level_source": "exact minimum enclosing ball for each reported wedge coface",
            "reported_binary64_levels_used_for_replay": False,
            "exact_recertified_relation_count": len(recertified_relations),
        },
        "recertified_cut_metrics": cut_metrics,
        "interpretation": (
            "this comparison measures the combinatorial loss of the Delaunay-wedge "
            "coface restriction after removing binary64 level error; it remains "
            "separate from the Gabriel empty-ball proposal and is exact only for "
            "this bounded input"
        ),
    }


def _analyze_k2(
    points: tuple[Point3, ...],
    triangles: tuple[ReportedTriangle, ...],
    restricted_gamma_records: tuple[ReportedTriangle, ...] | None,
) -> dict[str, object]:
    filtration = build_gamma_filtration(points, 2)
    gamma_forest = build_merge_forest(filtration, "hgp_reduced")
    gabriel_forest = build_gabriel_partial_forest(filtration)
    catalog = _catalog_metrics(filtration, triangles, points)
    exact_gabriel_by_simplex = {
        edge.simplex_point_ids: edge for edge in filtration.gabriel_hyperedges
    }
    binary_triangles = tuple(
        triangle for triangle in triangles if triangle.status == "gabriel_binary64"
    )
    exactly_accepted_ambiguous = tuple(
        triangle
        for triangle in triangles
        if triangle.status == "ambiguous_requires_cpu_recertification"
        and triangle.point_ids in exact_gabriel_by_simplex
    )
    reported_relations = _relations_from_triangles(binary_triangles) + tuple(
        (
            triangle.point_ids,
            exact_gabriel_by_simplex[triangle.point_ids].facet_point_ids,
            exact_gabriel_by_simplex[triangle.point_ids].squared_level,
        )
        for triangle in exactly_accepted_ambiguous
    )
    recertified_relations = tuple(
        (
            triangle.point_ids,
            exact_gabriel_by_simplex[triangle.point_ids].facet_point_ids,
            exact_gabriel_by_simplex[triangle.point_ids].squared_level,
        )
        for triangle in binary_triangles + exactly_accepted_ambiguous
        if triangle.point_ids in exact_gabriel_by_simplex
    )
    levels = (
        set(filtration.critical_levels)
        | {triangle.squared_level for triangle in triangles}
        | {edge.squared_level for edge in filtration.gabriel_hyperedges}
    )
    facet_universe: tuple[Entity, ...] = tuple(combinations(range(len(points)), 2))
    exact_gabriel_provider = _gamma_provider(filtration, "gabriel")
    exact_gamma_provider = _gamma_provider(filtration, "gamma")
    restricted_gamma = _analyze_restricted_gamma(
        filtration,
        restricted_gamma_records,
        point_count=len(points),
        facet_universe=facet_universe,
    )
    reported_cut_metrics = _compare_cuts(
        predicted_name="gabriel_binary64_at_reported_levels_plus_exactly_recertified_ambiguities",
        reference_name="exhaustive_exact_gabriel",
        levels=levels,
        predicted_provider=lambda level, closed: _relation_cut(
            reported_relations, level, closed
        ),
        reference_provider=exact_gabriel_provider,
        universe=facet_universe,
        point_count=len(points),
    )
    recertified_cut_metrics = _compare_cuts(
        predicted_name="emitted_nondegenerate_candidates_fully_exactly_recertified",
        reference_name="exhaustive_exact_gabriel",
        levels=levels,
        predicted_provider=lambda level, closed: _relation_cut(
            recertified_relations, level, closed
        ),
        reference_provider=exact_gabriel_provider,
        universe=facet_universe,
        point_count=len(points),
    )
    gabriel_to_gamma = _compare_cuts(
        predicted_name="exhaustive_exact_gabriel_partial_refinement",
        reference_name="exhaustive_exact_gamma_hgp_reduced",
        levels=levels,
        predicted_provider=exact_gabriel_provider,
        reference_provider=exact_gamma_provider,
        universe=facet_universe,
        point_count=len(points),
    )
    return {
        "semantic_scope": "k2_pair_facets_with_possibly_overlapping_point_covers",
        "gpu_to_exact_gabriel": {
            "catalog": catalog,
            "reported_cut_metrics": reported_cut_metrics,
            "fully_recertified_positive_cut_metrics": recertified_cut_metrics,
            "interpretation": (
                "gabriel_binary64 relations are replayed as emitted, exact-positive "
                "ambiguities are added, and degenerate_or_invalid entries stay out; "
                "the fully recertified projection additionally removes binary64 false "
                "positives so implementation incompleteness remains separate"
            ),
        },
        "restricted_gamma_to_exact_gamma": restricted_gamma,
        "exact_gabriel_to_exact_gamma": {
            "cut_metrics": gabriel_to_gamma,
            "interpretation": (
                "this is the intrinsic mathematical gap of the raw Gabriel partial "
                "filtration, not a GPU implementation error"
            ),
        },
        "oracle_summaries": {
            "exact_gamma_facet_count": len(filtration.facets),
            "exact_gamma_coface_count": len(filtration.cofaces),
            "exact_gabriel_hyperedge_count": len(filtration.gabriel_hyperedges),
            "exact_gamma_forest_node_count": len(gamma_forest.nodes),
            "exact_gamma_forest_batch_count": len(gamma_forest.batches),
            "exact_gabriel_forest_node_count": len(gabriel_forest.nodes),
            "exact_gabriel_forest_batch_count": len(gabriel_forest.batches),
        },
        "surrogate_policy": {
            "comparison_performed": False,
            "comparable": False,
            "reason": (
                "the phase-14 surrogate is a point-MST, whereas k=2 Gamma is a "
                "filtration on pair facets whose point covers may overlap"
            ),
        },
    }


def _fixture_levels(filtration: GammaFiltration) -> set[Fraction]:
    return set(filtration.critical_levels) | {
        edge.squared_level for edge in filtration.gabriel_hyperedges
    }


def _decode_overlap_points(contract: Mapping[str, object]) -> tuple[Point3, ...]:
    records = _require_list(
        contract.get("embedded_input_points"), path="overlap-k2.embedded_input_points"
    )
    points = []
    for index, raw_record in enumerate(records):
        record = _require_mapping(raw_record, path=f"embedded_input_points[{index}]")
        raw_bits = _require_list(
            record.get("coordinate_bits"),
            path=f"embedded_input_points[{index}].coordinate_bits",
        )
        if len(raw_bits) != 3 or any(not isinstance(bits, str) for bits in raw_bits):
            raise DiagnosticInputError("overlap-k2 coordinate_bits are malformed")
        coordinates = tuple(
            Fraction.from_float(struct.unpack(">d", bytes.fromhex(bits))[0])
            for bits in raw_bits
        )
        points.append(coordinates)
    return tuple(points)  # type: ignore[return-value]


def _exact_gabriel_gamma_fixture_metrics(
    points: tuple[Point3, ...], *, extra_levels: Iterable[Fraction] = ()
) -> tuple[GammaFiltration, dict[str, object]]:
    filtration = build_gamma_filtration(points, 2)
    metrics = _compare_cuts(
        predicted_name="exhaustive_exact_gabriel_partial_refinement",
        reference_name="exhaustive_exact_gamma_hgp_reduced",
        levels=_fixture_levels(filtration) | set(extra_levels),
        predicted_provider=_gamma_provider(filtration, "gabriel"),
        reference_provider=_gamma_provider(filtration, "gamma"),
        universe=tuple(combinations(range(len(points)), 2)),
        point_count=len(points),
    )
    return filtration, metrics


def _enumerate_exact_ordinary_delaunay(
    points: tuple[Point3, ...],
    *,
    path: str,
) -> tuple[tuple[tuple[int, int, int, int], ...], dict[str, object]]:
    """Enumerate the exact ordinary-Delaunay tetrahedra in general position."""

    if affine_dimension(points) != 3:
        raise DiagnosticInputError(f"{path} points must have affine dimension three")
    exact_tetrahedra: list[tuple[int, int, int, int]] = []
    minimum_external_power_delta: Fraction | None = None
    four_point_subset_count = 0
    cospherical_incidence_count = 0
    for tetrahedron in combinations(range(len(points)), 4):
        four_point_subset_count += 1
        try:
            ball = circumball(points, tetrahedron)
        except AffineDependenceError as error:
            raise DiagnosticInputError(
                f"{path} contains a coplanar four-point subset {tetrahedron}"
            ) from error
        external_relations = tuple(
            (point_id, classify(points[point_id], ball))
            for point_id in range(len(points))
            if point_id not in tetrahedron
        )
        cospherical_incidence_count += sum(
            relation is BallRelation.SHELL for _, relation in external_relations
        )
        if all(
            relation is BallRelation.EXTERIOR
            for _, relation in external_relations
        ):
            exact_tetrahedra.append(tetrahedron)
            for point_id, _ in external_relations:
                delta = (
                    squared_distance(points[point_id], ball.center)
                    - ball.squared_radius
                )
                if delta <= 0:
                    raise AssertionError(
                        "an exactly exterior Delaunay site has non-positive power"
                    )
                if (
                    minimum_external_power_delta is None
                    or delta < minimum_external_power_delta
                ):
                    minimum_external_power_delta = delta
    exact = tuple(exact_tetrahedra)
    if cospherical_incidence_count:
        raise DiagnosticInputError(
            f"{path} contains {cospherical_incidence_count} cospherical incidences"
        )
    return exact, {
        "performed": True,
        "method": "exhaustive exact rational circumspheres over all four-point subsets",
        "affine_dimension": 3,
        "four_point_subset_count": four_point_subset_count,
        "cospherical_incidence_count": cospherical_incidence_count,
        "exact_empty_sphere_tetrahedron_count": len(exact),
        "minimum_external_power_delta": (
            _fraction_json(minimum_external_power_delta)
            if minimum_external_power_delta is not None
            else None
        ),
    }


def _recertify_exact_ordinary_delaunay(
    points: tuple[Point3, ...],
    reported_tetrahedra: Iterable[tuple[int, ...]],
    *,
    path: str,
) -> dict[str, object]:
    """Enumerate every exact empty circumsphere and match stored tetrahedra."""

    reported = tuple(sorted(reported_tetrahedra))
    if len(set(reported)) != len(reported):
        raise DiagnosticInputError(f"{path} tetrahedra must not contain duplicates")
    exact, audit = _enumerate_exact_ordinary_delaunay(points, path=path)
    if reported != exact:
        raise DiagnosticInputError(
            f"{path} stored tetrahedra do not equal the exact empty-sphere catalog"
        )
    return {
        **audit,
        "stored_topology_matches_exact_catalog": True,
    }


def _delaunay_endpoint_neighbor_first_incidence_audit(
    points: tuple[Point3, ...],
    delaunay_edges: set[tuple[int, int]],
    filtration: GammaFiltration,
    *,
    path: str,
) -> dict[str, object]:
    """Check the radial exact first-incidence reduction for every pair.

    This is deliberately a bounded exhaustive falsifier.  The proposed side
    evaluates only third points adjacent in the ordinary-Delaunay graph to at
    least one endpoint; the reference side evaluates every possible third
    point.  No pair or coface catalog produced here is a product structure.
    """

    if filtration.order != 2 or filtration.point_count != len(points):
        raise DiagnosticInputError(f"{path} requires the matching Gamma2 filtration")
    canonical_edges = {
        tuple(sorted(edge))
        for edge in delaunay_edges
        if len(edge) == 2 and edge[0] != edge[1]
    }
    if canonical_edges != delaunay_edges:
        raise DiagnosticInputError(f"{path} Delaunay edges must be canonical")
    if any(
        point_id < 0 or point_id >= len(points)
        for edge in canonical_edges
        for point_id in edge
    ):
        raise DiagnosticInputError(f"{path} Delaunay edge lies outside the point cloud")
    coface_by_ids = {
        coface.point_ids: coface for coface in filtration.cofaces
    }
    full_evaluation_count = 0
    endpoint_neighbor_evaluation_count = 0
    pair_count = 0
    pair_with_pruning_count = 0
    global_cominimizer_count = 0
    retained_global_cominimizer_count = 0
    level_witnesses: list[dict[str, object]] = []
    for left, right in combinations(range(len(points)), 2):
        pair_count += 1
        third_points = tuple(
            point_id
            for point_id in range(len(points))
            if point_id not in {left, right}
        )
        endpoint_neighbors = tuple(
            point_id
            for point_id in third_points
            if tuple(sorted((left, point_id))) in canonical_edges
            or tuple(sorted((right, point_id))) in canonical_edges
        )
        if not endpoint_neighbors:
            raise DiagnosticInputError(
                f"{path} pair {(left, right)!r} has no endpoint-neighbor candidate"
            )
        levels = {
            point_id: coface_by_ids[
                tuple(sorted((left, right, point_id)))
            ].squared_level
            for point_id in third_points
        }
        full_level = min(levels.values())
        endpoint_neighbor_level = min(levels[point_id] for point_id in endpoint_neighbors)
        if endpoint_neighbor_level != full_level:
            raise DiagnosticInputError(
                f"{path} endpoint-neighbor first incidence mismatch for pair "
                f"{(left, right)!r}: {_fraction_json(endpoint_neighbor_level)} != "
                f"{_fraction_json(full_level)}"
            )
        full_minimizers = tuple(
            point_id for point_id in third_points if levels[point_id] == full_level
        )
        retained_minimizers = tuple(
            point_id
            for point_id in endpoint_neighbors
            if levels[point_id] == endpoint_neighbor_level
        )
        full_evaluation_count += len(third_points)
        endpoint_neighbor_evaluation_count += len(endpoint_neighbors)
        pair_with_pruning_count += len(endpoint_neighbors) < len(third_points)
        global_cominimizer_count += len(full_minimizers)
        retained_global_cominimizer_count += len(
            set(full_minimizers) & set(retained_minimizers)
        )
        level_witnesses.append(
            {
                "facet_point_ids": [left, right],
                "squared_level": _fraction_json(full_level),
                "canonical_endpoint_neighbor_minimizer": min(retained_minimizers),
                "global_cominimizer_count": len(full_minimizers),
                "retained_global_cominimizer_count": len(
                    set(full_minimizers) & set(retained_minimizers)
                ),
            }
        )
    critical_levels = filtration.critical_levels
    strict_closed_activation_decision_count = 2 * pair_count * len(critical_levels)
    return {
        "performed": True,
        "scope": "bounded_exact_Gamma2_first_incidence_level_only",
        "proof_basis": "ordinary_Delaunay_radial_connectivity_in_each_miniball",
        "point_count": len(points),
        "facet_pair_count": pair_count,
        "ordinary_delaunay_edge_count": len(canonical_edges),
        "full_third_point_evaluation_count": full_evaluation_count,
        "endpoint_neighbor_evaluation_count": endpoint_neighbor_evaluation_count,
        "avoided_third_point_evaluation_count": (
            full_evaluation_count - endpoint_neighbor_evaluation_count
        ),
        "pair_with_strict_candidate_reduction_count": pair_with_pruning_count,
        "every_first_incidence_level_exact": True,
        "strict_and_closed_activation_decisions_exact": True,
        "strict_closed_activation_decision_count": (
            strict_closed_activation_decision_count
        ),
        "global_cominimizer_count": global_cominimizer_count,
        "retained_global_cominimizer_count": retained_global_cominimizer_count,
        "all_global_cominimizers_retained_on_this_fixture": (
            retained_global_cominimizer_count == global_cominimizer_count
        ),
        "level_witnesses": level_witnesses,
        "global_pair_or_coface_catalog_product_claimed": False,
        "terminal_Morse_hierarchy_claimed": False,
        "public_status_claimed": False,
    }


@lru_cache(maxsize=1)
def _fixture_audits() -> dict[str, object]:
    e5_path = (
        ROOT
        / "tests"
        / "fixtures"
        / "regressions"
        / "gabriel_point_set_counterexample.json"
    )
    overlap_path = ROOT / "tests" / "fixtures" / "contracts" / "overlap-k2.json"
    two_edge_path = (
        ROOT
        / "tests"
        / "fixtures"
        / "regressions"
        / "delaunay_two_edge_gamma2_counterexample.json"
    )
    local_delaunay_path = (
        ROOT
        / "tests"
        / "fixtures"
        / "regressions"
        / "delaunay_local_gamma2_counterexample_n8.json"
    )
    one_edge_positive_path = (
        ROOT
        / "tests"
        / "fixtures"
        / "regressions"
        / "delaunay_one_edge_gamma2_positive_n9.json"
    )
    try:
        e5 = json.loads(e5_path.read_text(encoding="utf-8"))
        overlap = json.loads(overlap_path.read_text(encoding="utf-8"))
        two_edge = json.loads(two_edge_path.read_text(encoding="utf-8"))
        local_delaunay = json.loads(
            local_delaunay_path.read_text(encoding="utf-8")
        )
        one_edge_positive = json.loads(
            one_edge_positive_path.read_text(encoding="utf-8")
        )
    except (OSError, json.JSONDecodeError) as error:
        raise DiagnosticInputError(f"cannot read mandatory fixture: {error}") from error
    e5_record = _require_mapping(e5, path="E5 fixture")
    e5_points = tuple(
        _point(point, path=f"E5.points[{index}]")
        for index, point in enumerate(
            _require_list(e5_record.get("points"), path="E5.points")
        )
    )
    counterexample = _require_mapping(
        e5_record.get("counterexample"), path="E5.counterexample"
    )
    e5_level = _fraction_from_record(
        counterexample.get("squared_level"), path="E5.counterexample.squared_level"
    )
    e5_filtration, e5_metrics = _exact_gabriel_gamma_fixture_metrics(
        e5_points, extra_levels=(e5_level,)
    )
    e5_gabriel = _gamma_provider(e5_filtration, "gabriel")(e5_level, True)
    e5_gamma = _gamma_provider(e5_filtration, "gamma")(e5_level, True)
    e5_witness = _state_metrics(
        e5_gabriel,
        e5_gamma,
        tuple(combinations(range(len(e5_points)), 2)),
        len(e5_points),
    )

    overlap_record = _require_mapping(overlap, path="overlap-k2 fixture")
    overlap_points = _decode_overlap_points(overlap_record)
    overlap_level = Fraction(17, 2)
    overlap_filtration, overlap_metrics = _exact_gabriel_gamma_fixture_metrics(
        overlap_points, extra_levels=(overlap_level,)
    )
    overlap_gabriel = _gamma_provider(overlap_filtration, "gabriel")(
        overlap_level, True
    )
    overlap_gamma = _gamma_provider(overlap_filtration, "gamma")(overlap_level, True)
    overlap_witness = _state_metrics(
        overlap_gabriel,
        overlap_gamma,
        tuple(combinations(range(len(overlap_points)), 2)),
        len(overlap_points),
    )
    e5_ok = not bool(e5_witness["component_partition_exact"]) and not bool(
        e5_metrics["all_states_exact"]
    )
    overlap_ok = (
        not bool(overlap_witness["component_partition_exact"])
        and bool(overlap_witness["covered_point_collections_exact"])
        and not bool(overlap_metrics["all_states_exact"])
    )

    two_edge_record = _require_mapping(two_edge, path="Delaunay two-edge fixture")
    two_edge_points = tuple(
        _point(point, path=f"Delaunay-two-edge.points[{index}]")
        for index, point in enumerate(
            _require_list(two_edge_record.get("points"), path="Delaunay-two-edge.points")
        )
    )
    ordinary_delaunay = _require_mapping(
        two_edge_record.get("ordinary_delaunay"),
        path="Delaunay-two-edge.ordinary_delaunay",
    )
    tetrahedra = tuple(
        tuple(
            _point_id(
                point_id,
                path=f"Delaunay-two-edge.tetrahedra[{tetrahedron_index}]",
                point_count=len(two_edge_points),
            )
            for point_id in _require_list(
                raw_tetrahedron,
                path=f"Delaunay-two-edge.tetrahedra[{tetrahedron_index}]",
            )
        )
        for tetrahedron_index, raw_tetrahedron in enumerate(
            _require_list(
                ordinary_delaunay.get("tetrahedra"),
                path="Delaunay-two-edge.ordinary_delaunay.tetrahedra",
            )
        )
    )
    if any(len(tetrahedron) != 4 or len(set(tetrahedron)) != 4 for tetrahedron in tetrahedra):
        raise DiagnosticInputError(
            "Delaunay-two-edge tetrahedra must contain four distinct point ids"
        )
    two_edge_delaunay_audit = _recertify_exact_ordinary_delaunay(
        two_edge_points,
        tetrahedra,
        path="Delaunay-two-edge.ordinary_delaunay",
    )
    delaunay_edges = {
        tuple(sorted(edge))
        for tetrahedron in tetrahedra
        for edge in combinations(tetrahedron, 2)
    }
    two_edge_filtration = build_gamma_filtration(two_edge_points, 2)
    two_edge_first_incidence = _delaunay_endpoint_neighbor_first_incidence_audit(
        two_edge_points,
        delaunay_edges,
        two_edge_filtration,
        path="Delaunay-two-edge.first-incidence",
    )

    def candidate_relations(minimum_delaunay_edge_count: int) -> tuple[
        tuple[tuple[int, ...], tuple[tuple[int, ...], ...], Fraction], ...
    ]:
        return tuple(
            (
                coface.point_ids,
                coface.facet_point_ids,
                coface.squared_level,
            )
            for coface in two_edge_filtration.cofaces
            if sum(facet in delaunay_edges for facet in coface.facet_point_ids)
            >= minimum_delaunay_edge_count
        )

    two_edge_relations = candidate_relations(2)
    one_edge_relations = candidate_relations(1)
    two_edge_gabriel_sources: tuple[FacetRelation, ...] = tuple(
        (
            hyperedge.simplex_point_ids,
            hyperedge.facet_point_ids,
            hyperedge.squared_level,
        )
        for hyperedge in two_edge_filtration.gabriel_hyperedges
    )
    two_edge_gabriel_deadline = {
        "two_edge": _gabriel_fusion_deadline_coverage(
            variant_name="two_edge",
            source_relations=two_edge_gabriel_sources,
            candidate_relations=two_edge_relations,
        ),
        "one_edge": _gabriel_fusion_deadline_coverage(
            variant_name="one_edge",
            source_relations=two_edge_gabriel_sources,
            candidate_relations=one_edge_relations,
        ),
    }
    exact_two_edge_coface_ids = {
        coface.point_ids for coface in two_edge_filtration.cofaces
    }
    two_edge_omitted_cofaces = tuple(
        sorted(
            exact_two_edge_coface_ids
            - {point_ids for point_ids, _, _ in two_edge_relations}
        )
    )
    two_edge_levels = set(two_edge_filtration.critical_levels)
    two_edge_universe = tuple(combinations(range(len(two_edge_points)), 2))
    two_edge_metrics = _compare_cuts(
        predicted_name="Gamma2_restricted_to_at_least_two_ordinary_Delaunay_edges",
        reference_name="exhaustive_exact_Gamma2_hgp_reduced",
        levels=two_edge_levels,
        predicted_provider=lambda level, closed: _relation_cut(
            two_edge_relations, level, closed
        ),
        reference_provider=_gamma_provider(two_edge_filtration, "gamma"),
        universe=two_edge_universe,
        point_count=len(two_edge_points),
    )
    one_edge_metrics = _compare_cuts(
        predicted_name="Gamma2_restricted_to_at_least_one_ordinary_Delaunay_edge",
        reference_name="exhaustive_exact_Gamma2_hgp_reduced",
        levels=two_edge_levels,
        predicted_provider=lambda level, closed: _relation_cut(
            one_edge_relations, level, closed
        ),
        reference_provider=_gamma_provider(two_edge_filtration, "gamma"),
        universe=two_edge_universe,
        point_count=len(two_edge_points),
    )
    first_divergence = _require_mapping(
        two_edge_record.get("first_divergence"),
        path="Delaunay-two-edge.first_divergence",
    )
    two_edge_level = _fraction_from_record(
        first_divergence.get("squared_level"),
        path="Delaunay-two-edge.first_divergence.squared_level",
    )
    two_edge_cut = _relation_cut(two_edge_relations, two_edge_level, True)
    two_edge_gamma_cut = _gamma_provider(two_edge_filtration, "gamma")(
        two_edge_level, True
    )
    missing_active_facets = tuple(
        sorted(
            set(two_edge_gamma_cut.active_entities)
            - set(two_edge_cut.active_entities)
        )
    )
    two_edge_witness = _state_metrics(
        two_edge_cut,
        two_edge_gamma_cut,
        two_edge_universe,
        len(two_edge_points),
    )
    two_edge_ok = (
        len(two_edge_filtration.cofaces) == 20
        and len(two_edge_relations) == 18
        and len(one_edge_relations) == 20
        and two_edge_omitted_cofaces == ((1, 2, 4), (2, 4, 5))
        and _first_divergence_signature(two_edge_metrics)
        == ("281/4", "closed")
        and missing_active_facets == ((2, 4),)
        and not bool(two_edge_witness["active_entities_exact"])
        and not bool(two_edge_witness["component_partition_exact"])
        and bool(two_edge_witness["covered_point_collections_exact"])
        and not bool(two_edge_metrics["all_states_exact"])
        and bool(one_edge_metrics["all_states_exact"])
        and all(
            audit["passed"]
            and audit["source_count"] == 9
            and audit["connected_before_count"] == 0
            and audit["connected_at_count"] == 9
            for audit in two_edge_gabriel_deadline.values()
        )
        and bool(two_edge_first_incidence["every_first_incidence_level_exact"])
    )

    local_record = _require_mapping(
        local_delaunay, path="Delaunay local fixture"
    )
    local_points = tuple(
        _point(point, path=f"Delaunay-local.points[{index}]")
        for index, point in enumerate(
            _require_list(local_record.get("points"), path="Delaunay-local.points")
        )
    )
    local_delaunay_record = _require_mapping(
        local_record.get("ordinary_delaunay"),
        path="Delaunay-local.ordinary_delaunay",
    )
    local_tetrahedra = tuple(
        tuple(
            _point_id(
                point_id,
                path=f"Delaunay-local.tetrahedra[{tetrahedron_index}]",
                point_count=len(local_points),
            )
            for point_id in _require_list(
                raw_tetrahedron,
                path=f"Delaunay-local.tetrahedra[{tetrahedron_index}]",
            )
        )
        for tetrahedron_index, raw_tetrahedron in enumerate(
            _require_list(
                local_delaunay_record.get("tetrahedra"),
                path="Delaunay-local.ordinary_delaunay.tetrahedra",
            )
        )
    )
    if any(
        len(tetrahedron) != 4 or len(set(tetrahedron)) != 4
        for tetrahedron in local_tetrahedra
    ):
        raise DiagnosticInputError(
            "Delaunay-local tetrahedra must contain four distinct point ids"
        )
    local_delaunay_audit = _recertify_exact_ordinary_delaunay(
        local_points,
        local_tetrahedra,
        path="Delaunay-local.ordinary_delaunay",
    )
    local_edges = {
        tuple(sorted(edge))
        for tetrahedron in local_tetrahedra
        for edge in combinations(tetrahedron, 2)
    }
    reported_local_edges = {
        tuple(
            sorted(
                _point_id(
                    point_id,
                    path=f"Delaunay-local.edges[{edge_index}]",
                    point_count=len(local_points),
                )
                for point_id in _require_list(
                    raw_edge, path=f"Delaunay-local.edges[{edge_index}]"
                )
            )
        )
        for edge_index, raw_edge in enumerate(
            _require_list(
                local_delaunay_record.get("edges"),
                path="Delaunay-local.ordinary_delaunay.edges",
            )
        )
    }
    if any(len(edge) != 2 or len(set(edge)) != 2 for edge in reported_local_edges):
        raise DiagnosticInputError(
            "Delaunay-local edges must contain two distinct point ids"
        )
    if reported_local_edges != local_edges:
        raise DiagnosticInputError(
            "Delaunay-local stored edges do not match its tetrahedral 1-skeleton"
        )
    local_faces = {
        tuple(sorted(face))
        for tetrahedron in local_tetrahedra
        for face in combinations(tetrahedron, 3)
    }
    local_neighbors = [set((point_id,)) for point_id in range(len(local_points))]
    for left, right in local_edges:
        local_neighbors[left].add(right)
        local_neighbors[right].add(left)
    local_square_neighbors = [set(neighbors) for neighbors in local_neighbors]
    for point_id, neighbors in enumerate(local_neighbors):
        for neighbor in neighbors:
            local_square_neighbors[point_id].update(local_neighbors[neighbor])

    local_filtration = build_gamma_filtration(local_points, 2)
    local_first_incidence = _delaunay_endpoint_neighbor_first_incidence_audit(
        local_points,
        local_edges,
        local_filtration,
        path="Delaunay-local.first-incidence",
    )
    local_by_id = {coface.point_ids: coface for coface in local_filtration.cofaces}
    local_two_edge_ids = {
        point_ids
        for point_ids, coface in local_by_id.items()
        if sum(facet in local_edges for facet in coface.facet_point_ids) >= 2
    }
    local_one_edge_ids = {
        point_ids
        for point_ids, coface in local_by_id.items()
        if any(facet in local_edges for facet in coface.facet_point_ids)
    }
    local_star_ids = {
        point_ids
        for point_ids in local_by_id
        if any(set(point_ids) <= neighbors for neighbors in local_neighbors)
    }
    local_square_ids = {
        point_ids
        for point_ids in local_by_id
        if all(
            right in local_square_neighbors[left]
            for left, right in combinations(point_ids, 2)
        )
    }
    local_fan_ids = set(local_two_edge_ids)
    for center, neighbors in enumerate(local_neighbors):
        link_edges = {
            tuple(point_id for point_id in face if point_id != center)
            for face in local_faces
            if center in face
        }
        for left, right in link_edges:
            for third in neighbors - {center, left, right}:
                local_fan_ids.add(tuple(sorted((left, right, third))))

    def local_relations(
        point_ids: set[tuple[int, ...]],
    ) -> tuple[
        tuple[tuple[int, ...], tuple[tuple[int, ...], ...], Fraction], ...
    ]:
        return tuple(
            (
                local_by_id[simplex].point_ids,
                local_by_id[simplex].facet_point_ids,
                local_by_id[simplex].squared_level,
            )
            for simplex in sorted(point_ids)
        )

    local_candidate_ids = {
        "two_edge": local_two_edge_ids,
        "closed_star": local_star_ids,
        "square_clique": local_square_ids,
        "link_face_fan": local_fan_ids,
        "one_edge": local_one_edge_ids,
    }
    local_relation_sets = {
        name: local_relations(point_ids)
        for name, point_ids in local_candidate_ids.items()
    }
    local_gabriel_sources: tuple[FacetRelation, ...] = tuple(
        (
            hyperedge.simplex_point_ids,
            hyperedge.facet_point_ids,
            hyperedge.squared_level,
        )
        for hyperedge in local_filtration.gabriel_hyperedges
    )
    local_gabriel_deadline = {
        name: _gabriel_fusion_deadline_coverage(
            variant_name=name,
            source_relations=local_gabriel_sources,
            candidate_relations=relations,
        )
        for name, relations in local_relation_sets.items()
    }
    local_universe = tuple(combinations(range(len(local_points)), 2))
    local_levels = set(local_filtration.critical_levels)
    local_metrics = {
        name: _compare_cuts(
            predicted_name=f"Gamma2_restricted_to_{name}",
            reference_name="exhaustive_exact_Gamma2_hgp_reduced",
            levels=local_levels,
            predicted_provider=lambda level, closed, relations=relations: _relation_cut(
                relations, level, closed
            ),
            reference_provider=_gamma_provider(local_filtration, "gamma"),
            universe=local_universe,
            point_count=len(local_points),
        )
        for name, relations in local_relation_sets.items()
    }
    local_divergence = _require_mapping(
        local_record.get("first_divergence"),
        path="Delaunay-local.first_divergence",
    )
    local_level = _fraction_from_record(
        local_divergence.get("squared_level"),
        path="Delaunay-local.first_divergence.squared_level",
    )
    local_reference_cut = _gamma_provider(local_filtration, "gamma")(
        local_level, True
    )
    local_missing_facets = {
        name: tuple(
            sorted(
                set(local_reference_cut.active_entities)
                - set(_relation_cut(relations, local_level, True).active_entities)
            )
        )
        for name, relations in local_relation_sets.items()
    }
    local_witness_metrics = {
        name: _state_metrics(
            _relation_cut(relations, local_level, True),
            local_reference_cut,
            local_universe,
            len(local_points),
        )
        for name, relations in local_relation_sets.items()
    }
    local_causal_omitted_cofaces = {
        name: tuple(
            sorted(
                simplex
                for simplex, coface in local_by_id.items()
                if simplex not in local_candidate_ids[name]
                and coface.squared_level == local_level
                and any(
                    facet in local_missing_facets[name]
                    for facet in coface.facet_point_ids
                )
            )
        )
        for name in local_candidate_ids
    }
    false_local_candidates = (
        "two_edge",
        "closed_star",
        "square_clique",
        "link_face_fan",
    )
    expected_local_causal_omissions = ((0, 1, 4), (0, 1, 6), (0, 1, 7))
    local_ok = (
        len(local_filtration.cofaces) == 56
        and {name: len(ids) for name, ids in local_candidate_ids.items()}
        == {
            "two_edge": 42,
            "closed_star": 50,
            "square_clique": 50,
            "link_face_fan": 50,
            "one_edge": 56,
        }
        and all(
            not bool(local_metrics[name]["all_states_exact"])
            and _first_divergence_signature(local_metrics[name])
            == ("13956479554", "closed")
            and local_missing_facets[name] == ((0, 1),)
            and bool(
                local_witness_metrics[name]["covered_point_collections_exact"]
            )
            and local_causal_omitted_cofaces[name]
            == expected_local_causal_omissions
            for name in false_local_candidates
        )
        and bool(local_metrics["one_edge"]["all_states_exact"])
        and local_missing_facets["one_edge"] == ()
        and local_causal_omitted_cofaces["one_edge"] == ()
        and all(
            audit["passed"]
            and audit["source_count"] == 17
            and audit["connected_before_count"] == 2
            and audit["connected_at_count"] == 15
            for audit in local_gabriel_deadline.values()
        )
        and bool(local_first_incidence["every_first_incidence_level_exact"])
    )

    positive_record = _require_mapping(
        one_edge_positive, path="Delaunay one-edge positive fixture"
    )
    positive_points = tuple(
        _point(point, path=f"Delaunay-one-edge-positive.points[{index}]")
        for index, point in enumerate(
            _require_list(
                positive_record.get("points"),
                path="Delaunay-one-edge-positive.points",
            )
        )
    )
    positive_delaunay_record = _require_mapping(
        positive_record.get("ordinary_delaunay"),
        path="Delaunay-one-edge-positive.ordinary_delaunay",
    )
    positive_tetrahedra = tuple(
        tuple(
            _point_id(
                point_id,
                path=(
                    "Delaunay-one-edge-positive."
                    f"tetrahedra[{tetrahedron_index}]"
                ),
                point_count=len(positive_points),
            )
            for point_id in _require_list(
                raw_tetrahedron,
                path=(
                    "Delaunay-one-edge-positive."
                    f"tetrahedra[{tetrahedron_index}]"
                ),
            )
        )
        for tetrahedron_index, raw_tetrahedron in enumerate(
            _require_list(
                positive_delaunay_record.get("tetrahedra"),
                path=(
                    "Delaunay-one-edge-positive."
                    "ordinary_delaunay.tetrahedra"
                ),
            )
        )
    )
    if any(
        len(tetrahedron) != 4 or len(set(tetrahedron)) != 4
        for tetrahedron in positive_tetrahedra
    ):
        raise DiagnosticInputError(
            "Delaunay-one-edge-positive tetrahedra must contain four distinct ids"
        )
    positive_delaunay_audit = _recertify_exact_ordinary_delaunay(
        positive_points,
        positive_tetrahedra,
        path="Delaunay-one-edge-positive.ordinary_delaunay",
    )
    positive_edges = {
        tuple(sorted(edge))
        for tetrahedron in positive_tetrahedra
        for edge in combinations(tetrahedron, 2)
    }
    positive_filtration = build_gamma_filtration(positive_points, 2)
    positive_first_incidence = _delaunay_endpoint_neighbor_first_incidence_audit(
        positive_points,
        positive_edges,
        positive_filtration,
        path="Delaunay-one-edge-positive.first-incidence",
    )
    positive_one_edge_cofaces = tuple(
        coface
        for coface in positive_filtration.cofaces
        if any(facet in positive_edges for facet in coface.facet_point_ids)
    )
    positive_missing_cofaces = tuple(
        coface
        for coface in positive_filtration.cofaces
        if not any(facet in positive_edges for facet in coface.facet_point_ids)
    )
    positive_relations = tuple(
        (coface.point_ids, coface.facet_point_ids, coface.squared_level)
        for coface in positive_one_edge_cofaces
    )
    positive_gabriel_sources: tuple[FacetRelation, ...] = tuple(
        (
            hyperedge.simplex_point_ids,
            hyperedge.facet_point_ids,
            hyperedge.squared_level,
        )
        for hyperedge in positive_filtration.gabriel_hyperedges
    )
    positive_gabriel_deadline = _gabriel_fusion_deadline_coverage(
        variant_name="one_edge",
        source_relations=positive_gabriel_sources,
        candidate_relations=positive_relations,
    )
    positive_metrics = _compare_cuts(
        predicted_name="Gamma2_restricted_to_at_least_one_ordinary_Delaunay_edge",
        reference_name="exhaustive_exact_Gamma2_hgp_reduced",
        levels=positive_filtration.critical_levels,
        predicted_provider=lambda level, closed: _relation_cut(
            positive_relations, level, closed
        ),
        reference_provider=_gamma_provider(positive_filtration, "gamma"),
        universe=tuple(combinations(range(len(positive_points)), 2)),
        point_count=len(positive_points),
    )
    positive_expected_omissions = {
        (0, 1, 4): Fraction(
            6533652824060214379979565721490,
            616187158871335654299,
        ),
        (5, 6, 8): Fraction(
            196666935109083477399056026226,
            13800525515901360121,
        ),
    }
    positive_observed_omissions = {
        coface.point_ids: coface.squared_level
        for coface in positive_missing_cofaces
    }
    positive_ok = (
        len(positive_edges) == 26
        and len(positive_filtration.cofaces) == 84
        and len(positive_one_edge_cofaces) == 82
        and positive_observed_omissions == positive_expected_omissions
        and bool(positive_metrics["all_states_exact"])
        and positive_metrics["state_count"] == 168
        and positive_gabriel_deadline["passed"]
        and positive_gabriel_deadline["source_count"] == 19
        and positive_gabriel_deadline["connected_before_count"] == 2
        and positive_gabriel_deadline["connected_at_count"] == 17
        and bool(positive_first_incidence["every_first_incidence_level_exact"])
    )
    return {
        "gabriel_e5_counterexample": {
            "fixture_id": e5_record.get("fixture_id"),
            "witness_squared_level": _fraction_json(e5_level),
            "witness_boundary": "closed",
            "witness_state_metrics": e5_witness,
            "all_threshold_metrics": e5_metrics,
            "expected_invariants_satisfied": e5_ok,
        },
        "overlap_k2_contract": {
            "fixture_result_id": overlap_record.get("result_id"),
            "witness_squared_level": _fraction_json(overlap_level),
            "witness_boundary": "closed",
            "witness_state_metrics": overlap_witness,
            "all_threshold_metrics": overlap_metrics,
            "expected_invariants_satisfied": overlap_ok,
        },
        "delaunay_two_edge_gamma2_counterexample": {
            "fixture_id": two_edge_record.get("fixture_id"),
            "ordinary_delaunay_exact_recertification": two_edge_delaunay_audit,
            "witness_squared_level": _fraction_json(two_edge_level),
            "witness_boundary": "closed",
            "exact_gamma_coface_count": len(two_edge_filtration.cofaces),
            "two_edge_candidate_count": len(two_edge_relations),
            "one_edge_candidate_count": len(one_edge_relations),
            "two_edge_coface_recall": _ratio(
                len(two_edge_relations), len(two_edge_filtration.cofaces)
            ),
            "missing_active_facets": [
                list(facet) for facet in missing_active_facets
            ],
            "omitted_cofaces": [
                list(coface) for coface in two_edge_omitted_cofaces
            ],
            "witness_state_metrics": two_edge_witness,
            "two_edge_cut_metrics": two_edge_metrics,
            "one_edge_cut_metrics": one_edge_metrics,
            "gabriel_fusion_deadline": two_edge_gabriel_deadline,
            "delaunay_endpoint_neighbor_first_incidence": (
                two_edge_first_incidence
            ),
            "expected_invariants_satisfied": two_edge_ok,
        },
        "delaunay_local_gamma2_counterexample": {
            "fixture_id": local_record.get("fixture_id"),
            "ordinary_delaunay_exact_recertification": local_delaunay_audit,
            "witness_squared_level": _fraction_json(local_level),
            "witness_boundary": "closed",
            "exact_gamma_coface_count": len(local_filtration.cofaces),
            "candidate_counts": {
                name: len(ids) for name, ids in local_candidate_ids.items()
            },
            "missing_active_facets": {
                name: [list(facet) for facet in facets]
                for name, facets in local_missing_facets.items()
            },
            "causal_omitted_cofaces": {
                name: [list(coface) for coface in cofaces]
                for name, cofaces in local_causal_omitted_cofaces.items()
            },
            "witness_state_metrics": local_witness_metrics,
            "cut_metrics": local_metrics,
            "gabriel_fusion_deadline": local_gabriel_deadline,
            "delaunay_endpoint_neighbor_first_incidence": local_first_incidence,
            "expected_invariants_satisfied": local_ok,
        },
        "delaunay_one_edge_gamma2_positive": {
            "fixture_id": positive_record.get("fixture_id"),
            "ordinary_delaunay_exact_recertification": positive_delaunay_audit,
            "exact_gamma_coface_count": len(positive_filtration.cofaces),
            "one_edge_candidate_count": len(positive_one_edge_cofaces),
            "omitted_zero_edge_cofaces": [
                {
                    "simplex_point_ids": list(coface.point_ids),
                    "squared_level": _fraction_json(coface.squared_level),
                }
                for coface in positive_missing_cofaces
            ],
            "cut_metrics": positive_metrics,
            "gabriel_fusion_deadline": positive_gabriel_deadline,
            "delaunay_endpoint_neighbor_first_incidence": (
                positive_first_incidence
            ),
            "expected_invariants_satisfied": positive_ok,
        },
        "all_expected_invariants_satisfied": (
            e5_ok and overlap_ok and two_edge_ok and local_ok and positive_ok
        ),
    }


def _reported_summaries(payload: Mapping[str, object]) -> object:
    if "summaries" in payload:
        return payload["summaries"]
    result = {
        key: payload[key]
        for key in (
            "schema",
            "git_sha",
            "phase",
            "backend",
            "profile",
            "mode",
            "deployment_status",
            "public_status",
            "approximations",
            "architecture",
            "geogram",
            "gpu",
            "timings_nanoseconds",
        )
        if key in payload
    }
    input_record = payload.get("input")
    if isinstance(input_record, Mapping):
        result["input"] = {
            key: value for key, value in input_record.items() if key != "points"
        }
    for order, records_keys in (
        ("k1", frozenset({"selected_edges"})),
        (
            "k2",
            frozenset(
                {
                    "accepted_triangles",
                    "retained_records",
                    "invalid_records",
                    "restricted_gamma_records",
                }
            ),
        ),
    ):
        record = payload.get(order)
        if isinstance(record, Mapping):
            result[order] = {
                key: value for key, value in record.items() if key not in records_keys
            }
    return result


def _reported_summary_consistency(
    payload: Mapping[str, object],
    points: tuple[Point3, ...],
    edges: tuple[ReportedEdge, ...],
    triangles: tuple[ReportedTriangle, ...],
    restricted_gamma_records: tuple[ReportedTriangle, ...] | None,
) -> dict[str, object]:
    checks: list[dict[str, object]] = []

    def integer_check(
        record: Mapping[str, object], key: str, expected: int, path: str
    ) -> None:
        if key not in record:
            return
        observed = record[key]
        matches = (
            not isinstance(observed, bool)
            and isinstance(observed, int)
            and observed == expected
        )
        checks.append(
            {
                "path": f"{path}.{key}",
                "expected": expected,
                "observed": observed,
                "matches": matches,
            }
        )

    def level_check(
        record: Mapping[str, object], key: str, expected: Fraction, path: str
    ) -> None:
        if key not in record:
            return
        observed = _fraction(record[key], path=f"{path}.{key}")
        checks.append(
            {
                "path": f"{path}.{key}",
                "expected": _fraction_json(expected),
                "observed": _fraction_json(observed),
                "matches": observed == expected,
            }
        )

    input_record = _require_mapping(payload.get("input"), path="input")
    k1_record = _require_mapping(payload.get("k1"), path="k1")
    k2_record = _require_mapping(payload.get("k2"), path="k2")
    integer_check(input_record, "point_count", len(points), "input")
    integer_check(k1_record, "selected_edge_count", len(edges), "k1")
    integer_check(
        k1_record,
        "distinct_level_count",
        len({edge.squared_level for edge in edges}),
        "k1",
    )
    root_k1 = max((edge.squared_level for edge in edges), default=Fraction(0))
    level_check(k1_record, "root_squared_level", root_k1, "k1")
    level_check(k1_record, "root_squared_distance", 4 * root_k1, "k1")

    binary = tuple(
        sorted(
            (
                triangle
                for triangle in triangles
                if triangle.status == "gabriel_binary64"
            ),
            key=lambda triangle: (triangle.squared_level, triangle.point_ids),
        )
    )
    ambiguous_count = sum(
        triangle.status == "ambiguous_requires_cpu_recertification"
        for triangle in triangles
    )
    invalid_count = sum(
        triangle.status == "degenerate_or_invalid" for triangle in triangles
    )
    integer_check(k2_record, "accepted_triangle_count", len(binary), "k2")
    integer_check(k2_record, "ambiguous_triangle_count", ambiguous_count, "k2")
    integer_check(k2_record, "invalid_triangle_count", invalid_count, "k2")
    integer_check(
        k2_record,
        "retained_record_count",
        len(binary) + ambiguous_count,
        "k2",
    )

    def split_record_check(
        key: str,
        expected: tuple[ReportedTriangle, ...],
        expected_description: str,
    ) -> None:
        if key not in k2_record:
            return
        observed = _parse_triangle_array(
            k2_record[key], path=f"k2.{key}", point_count=len(points)
        )
        missing = sorted(set(expected) - set(observed))
        extra = sorted(set(observed) - set(expected))
        checks.append(
            {
                "path": f"k2.{key}.content",
                "expected": expected_description,
                "expected_record_count": len(expected),
                "observed_record_count": len(observed),
                "missing_records": [
                    {
                        "point_ids": list(record.point_ids),
                        "squared_level": _fraction_json(record.squared_level),
                        "status": record.status,
                    }
                    for record in missing
                ],
                "extra_records": [
                    {
                        "point_ids": list(record.point_ids),
                        "squared_level": _fraction_json(record.squared_level),
                        "status": record.status,
                    }
                    for record in extra
                ],
                "matches": not missing and not extra,
            }
        )

    retained = tuple(
        sorted(
            triangle
            for triangle in triangles
            if triangle.status != "degenerate_or_invalid"
        )
    )
    invalid = tuple(
        sorted(
            triangle
            for triangle in triangles
            if triangle.status == "degenerate_or_invalid"
        )
    )
    split_record_check(
        "retained_records",
        retained,
        "binary-accepted plus ambiguous projection of k2.accepted_triangles",
    )
    split_record_check(
        "invalid_records",
        invalid,
        "invalid-only projection of k2.accepted_triangles",
    )
    integer_check(
        k2_record,
        "distinct_level_count",
        len({triangle.squared_level for triangle in binary}),
        "k2",
    )
    first_k2 = binary[0].squared_level if binary else Fraction(0)
    root_k2 = binary[-1].squared_level if binary else Fraction(0)
    level_check(k2_record, "first_squared_level", first_k2, "k2")
    level_check(k2_record, "root_squared_level", root_k2, "k2")

    binary_relations = _relations_from_triangles(binary)
    facets = {
        facet for _, relation_facets, _ in binary_relations for facet in relation_facets
    }
    final_state = _relation_cut(binary_relations, root_k2, True)
    integer_check(k2_record, "facet_count", len(facets), "k2")
    integer_check(
        k2_record,
        "final_component_count",
        len(final_state.components),
        "k2",
    )
    component_dsu = _DisjointSet(facets)
    useful_union_count = 0
    redundant_union_count = 0
    for _, relation_facets, _ in binary_relations:
        for facet in relation_facets[1:]:
            if component_dsu.union(relation_facets[0], facet):
                useful_union_count += 1
            else:
                redundant_union_count += 1
    integer_check(k2_record, "useful_union_count", useful_union_count, "k2")
    integer_check(k2_record, "redundant_union_count", redundant_union_count, "k2")

    if restricted_gamma_records is not None:
        expected_status = "restricted_Delaunay_wedge_Gamma2_binary64"
        if "restricted_gamma_status" in k2_record:
            observed_status = k2_record["restricted_gamma_status"]
            checks.append(
                {
                    "path": "k2.restricted_gamma_status",
                    "expected": expected_status,
                    "observed": observed_status,
                    "matches": observed_status == expected_status,
                }
            )
        restricted_relations = tuple(
            sorted(
                _relations_from_triangles(restricted_gamma_records),
                key=lambda relation: (relation[2], relation[0]),
            )
        )
        restricted_levels = {
            record.squared_level for record in restricted_gamma_records
        }
        restricted_root = max(restricted_levels, default=Fraction(0))
        restricted_first = min(restricted_levels, default=Fraction(0))
        restricted_facets = {
            facet
            for _, relation_facets, _ in restricted_relations
            for facet in relation_facets
        }
        restricted_final = _relation_cut(restricted_relations, restricted_root, True)
        restricted_dsu = _DisjointSet(restricted_facets)
        restricted_useful = 0
        restricted_redundant = 0
        for _, relation_facets, _ in restricted_relations:
            for facet in relation_facets[1:]:
                if restricted_dsu.union(relation_facets[0], facet):
                    restricted_useful += 1
                else:
                    restricted_redundant += 1
        integer_check(
            k2_record,
            "restricted_gamma_record_count",
            len(restricted_gamma_records),
            "k2",
        )
        integer_check(
            k2_record,
            "restricted_gamma_facet_count",
            len(restricted_facets),
            "k2",
        )
        integer_check(
            k2_record,
            "restricted_gamma_final_component_count",
            len(restricted_final.components),
            "k2",
        )
        integer_check(
            k2_record,
            "restricted_gamma_useful_union_count",
            restricted_useful,
            "k2",
        )
        integer_check(
            k2_record,
            "restricted_gamma_redundant_union_count",
            restricted_redundant,
            "k2",
        )
        integer_check(
            k2_record,
            "restricted_gamma_distinct_level_count",
            len(restricted_levels),
            "k2",
        )
        level_check(
            k2_record,
            "restricted_gamma_first_squared_level",
            restricted_first,
            "k2",
        )
        level_check(
            k2_record,
            "restricted_gamma_root_squared_level",
            restricted_root,
            "k2",
        )

    mismatch_count = sum(not bool(check["matches"]) for check in checks)
    return {
        "checked_field_count": len(checks),
        "mismatch_count": mismatch_count,
        "all_checked_fields_consistent": mismatch_count == 0,
        "checks": checks,
    }


def analyze_diagnostic(payload: object) -> dict[str, object]:
    """Validate and compare one decoded diagnostic JSON object."""

    record = _require_mapping(payload, path="document")
    _reject_surrogate_comparability(record)
    points = _parse_points(record)
    edges = _parse_edges(record, len(points))
    triangles = _parse_triangles(record, len(points))
    restricted_gamma_records = _parse_restricted_gamma_records(record, len(points))
    summary_consistency = _reported_summary_consistency(
        record, points, edges, triangles, restricted_gamma_records
    )
    k1 = _analyze_k1(points, edges)
    k2 = _analyze_k2(points, triangles, restricted_gamma_records)
    fixtures = _fixture_audits()
    k2_catalog = k2["gpu_to_exact_gabriel"]["catalog"]  # type: ignore[index]
    k2_reported_cuts = k2["gpu_to_exact_gabriel"][  # type: ignore[index]
        "reported_cut_metrics"
    ]
    k2_recertified_cuts = k2["gpu_to_exact_gabriel"][  # type: ignore[index]
        "fully_recertified_positive_cut_metrics"
    ]
    gabriel_gamma_cuts = k2["exact_gabriel_to_exact_gamma"][  # type: ignore[index]
        "cut_metrics"
    ]
    restricted_gamma = k2["restricted_gamma_to_exact_gamma"]  # type: ignore[index]
    restricted_available = bool(restricted_gamma["available"])  # type: ignore[index]
    restricted_cuts_exact = (
        restricted_gamma["recertified_cut_metrics"]["all_states_exact"]  # type: ignore[index]
        if restricted_available
        else None
    )
    restricted_recall = (
        restricted_gamma["coface_universe_classification"]["recall"]  # type: ignore[index]
        if restricted_available
        else None
    )
    return {
        "schema_version": SCHEMA_VERSION,
        "diagnostic_completed": True,
        "phase": "14",
        "backend": "python_oracle",
        "profile": "hgp_reduced",
        "mode": "bounded_low_order_diagnostic_validation",
        "point_count": len(points),
        "bounded_domain": {
            "maximum_point_count": MAX_POINT_COUNT,
            "orders_checked": [1, 2],
            "exhaustive_gamma_materialized_only_inside_checker": True,
            "product_architecture_claim": False,
        },
        "reported_summaries": _reported_summaries(record),
        "reported_summary_consistency": summary_consistency,
        "k1": k1,
        "k2": k2,
        "mandatory_fixture_audits": fixtures,
        "comparison_summary": {
            "reported_summaries_consistent": summary_consistency[
                "all_checked_fields_consistent"
            ],
            "k1_recertified_exact_for_input": k1["certified_for_this_bounded_input"],
            "k2_gpu_positive_inclusion_safe": k2_catalog["positive_inclusion_safe"],
            "k2_gpu_catalog_exact_after_ambiguity_recertification": k2_catalog[
                "catalog_exact_after_ambiguity_recertification"
            ],
            "k2_gpu_reported_cuts_exact": k2_reported_cuts["all_states_exact"],
            "k2_gpu_fully_recertified_cuts_exact": k2_recertified_cuts[
                "all_states_exact"
            ],
            "k2_restricted_gamma_available": restricted_available,
            "k2_restricted_gamma_coface_universe_recall": restricted_recall,
            "k2_restricted_gamma_recertified_cuts_exact": restricted_cuts_exact,
            "exact_gabriel_equals_exact_gamma_at_all_cuts": gabriel_gamma_cuts[
                "all_states_exact"
            ],
            "mandatory_fixture_invariants_satisfied": fixtures[
                "all_expected_invariants_satisfied"
            ],
            "surrogate_k2_comparable": False,
        },
        "scientific_status": {
            "public_status": "not_claimed",
            "geogram_global_status": "not_certified",
            "gpu_global_status": "not_certified",
            "exactness_scope": "only the reported input with n<=14 after exact replay",
            "surrogate_k2_comparable": False,
        },
    }


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "diagnostic",
        nargs="?",
        default="-",
        help="diagnostic JSON path, or '-' / omitted for stdin",
    )
    parser.add_argument("--pretty", action="store_true", help="indent output JSON")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    try:
        if arguments.diagnostic == "-":
            payload = json.load(sys.stdin)
        else:
            with Path(arguments.diagnostic).open(encoding="utf-8") as stream:
                payload = json.load(stream)
        report = analyze_diagnostic(payload)
    except (DiagnosticInputError, json.JSONDecodeError, OSError) as error:
        json.dump(
            {
                "schema_version": SCHEMA_VERSION,
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
