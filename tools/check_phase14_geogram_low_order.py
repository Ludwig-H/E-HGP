#!/usr/bin/env python3
"""Compare a low-order Geogram/CUDA diagnostic with exact bounded oracles.

The input is deliberately small and diagnostic-only.  For at most fourteen
points, this checker:

* compares the reported k=1 tree with the independent exact complete-graph
  EMST at every exact open and closed threshold;
* compares reported k=2 triangles with the exhaustive exact Gabriel catalog;
* compares the exact Gabriel partial filtration with exhaustive Gamma;
* replays the permanent five-point Gabriel counterexample and the independent
  ``overlap-k2`` contract fixture on every exact threshold.

The k=2 universe is the set of two-point facets.  A point-MST surrogate does
not live on that universe and is therefore never assigned a Morse/Gamma
quality score by this tool.
"""

from __future__ import annotations

import argparse
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
    relations: Sequence[tuple[tuple[int, ...], tuple[tuple[int, ...], ...], Fraction]],
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
    try:
        e5 = json.loads(e5_path.read_text(encoding="utf-8"))
        overlap = json.loads(overlap_path.read_text(encoding="utf-8"))
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
        "all_expected_invariants_satisfied": e5_ok and overlap_ok,
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
