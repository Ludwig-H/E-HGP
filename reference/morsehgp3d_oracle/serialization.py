"""Canonical JSON serialization for the independent reference oracle."""

from __future__ import annotations

import hashlib
import json
import platform
import re
import struct
from fractions import Fraction
from math import comb, isfinite
from pathlib import Path
from typing import Any, IO, Iterable, Mapping, Sequence

from .oracle import CanonicalPoint, OracleResult, Point3


SCHEMA_VERSION = "2.0.0"
CONTRACT_DOMAIN = "MorseHGP3D/v2"
COORDINATE_UNIT = "input_coordinate_unit"
SQUARED_LEVEL_UNIT = "input_coordinate_unit_squared"


def canonical_json(value: Any) -> str:
    """Return the phase-0 canonical JSON representation."""

    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def canonical_json_bytes(value: Any) -> bytes:
    return canonical_json(value).encode("utf-8")


def canonical_id(record_type: str, identity_projection: Any) -> str:
    """Hash a type-specific identity projection with domain separation."""

    if not record_type or "/" in record_type:
        raise ValueError("record_type must be a non-empty path component")
    digest = hashlib.sha256()
    digest.update(f"{CONTRACT_DOMAIN}/{record_type}/".encode("ascii"))
    digest.update(canonical_json_bytes(_without_schema_versions(identity_projection)))
    return digest.hexdigest()


def _without_schema_versions(value: Any) -> Any:
    """Remove record-version metadata from a scientific identity projection."""

    if isinstance(value, Mapping):
        return {
            key: _without_schema_versions(child)
            for key, child in value.items()
            if key != "schema_version"
        }
    if isinstance(value, (list, tuple)):
        return [_without_schema_versions(child) for child in value]
    return value


def exact_level(value: Fraction | int) -> dict[str, str]:
    level = Fraction(value)
    if level < 0:
        raise ValueError("a public squared level cannot be negative")
    return {
        "schema_version": SCHEMA_VERSION,
        "numerator": str(level.numerator),
        "denominator": str(level.denominator),
        "unit": SQUARED_LEVEL_UNIT,
    }


def exact_rational3(values: Sequence[Fraction | int]) -> dict[str, str]:
    if len(values) != 3:
        raise ValueError("an exact 3D point must have exactly three coordinates")
    coordinates = tuple(Fraction(value) for value in values)
    denominator = 1
    for value in coordinates:
        denominator = _least_common_multiple(denominator, value.denominator)
    numerators = tuple(value.numerator * (denominator // value.denominator) for value in coordinates)
    common = denominator
    for numerator in numerators:
        common = _greatest_common_divisor(common, abs(numerator))
    denominator //= common
    numerators = tuple(numerator // common for numerator in numerators)
    return {
        "schema_version": SCHEMA_VERSION,
        "x_numerator": str(numerators[0]),
        "y_numerator": str(numerators[1]),
        "z_numerator": str(numerators[2]),
        "denominator": str(denominator),
        "unit": COORDINATE_UNIT,
    }


def binary64_bits(value: Fraction | int | float) -> str:
    """Encode one exactly representable finite coordinate as canonical bits."""

    fraction = Fraction.from_float(value) if isinstance(value, float) else Fraction(value)
    as_float = float(fraction)
    if not isfinite(as_float) or Fraction.from_float(as_float) != fraction:
        raise ValueError("the coordinate is not exactly representable as binary64")
    if as_float == 0.0:
        as_float = 0.0
    return struct.pack(">d", as_float).hex()


def certified_point(point: CanonicalPoint) -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "point_id": point.point_id,
        "source_indices": list(point.source_indices),
        "multiplicity": len(point.source_indices),
        "coordinate_bits": [binary64_bits(value) for value in point.coordinates],
        "coordinate_unit": COORDINATE_UNIT,
        "predicate_status": "finite_exact_dyadic",
    }


def input_sha256(points: Iterable[Mapping[str, Any]]) -> str:
    """Hash canonical point records, excluding redundant record versions."""

    projection = [
        {
            "point_id": point["point_id"],
            "source_indices": point["source_indices"],
            "multiplicity": point["multiplicity"],
            "coordinate_bits": point["coordinate_bits"],
        }
        for point in points
    ]
    return canonical_id("Input", projection)


def dumps_contract(contract: Mapping[str, Any], *, pretty: bool = False) -> str:
    """Serialize a contract canonically, or deterministically for inspection."""

    if not pretty:
        return canonical_json(contract)
    return json.dumps(
        contract,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        indent=2,
    )


def dump_contract(
    contract: Mapping[str, Any], destination: str | Path | IO[str], *, pretty: bool = False
) -> None:
    """Write one serialized contract without taking ownership of open streams."""

    encoded = dumps_contract(contract, pretty=pretty)
    if hasattr(destination, "write"):
        destination.write(encoded)  # type: ignore[union-attr]
        return
    Path(destination).write_text(encoded, encoding="utf-8")


def _validated_exhaustive_gamma_filtrations(
    result: OracleResult,
) -> tuple[Any, ...]:
    """Rebuild Gamma and reject any amputated or relabelled cut artifact."""

    from .gamma import build_gamma_filtration

    rebuilt = []
    for order_result in result.orders:
        filtration = build_gamma_filtration(result.points, order_result.order)
        if order_result.filtration != filtration:
            raise ValueError(
                f"order {order_result.order} Gamma filtration is not exhaustive"
            )
        expected_levels = filtration.critical_levels
        if order_result.critical_levels != expected_levels:
            raise ValueError(
                f"order {order_result.order} does not expose the exhaustive "
                "Gamma critical levels"
            )
        expected_cuts = tuple(
            filtration.cut(level, closed=closed, graph_kind="gamma")
            for level in expected_levels
            for closed in (False, True)
        )
        if order_result.cuts != expected_cuts:
            raise ValueError(
                f"order {order_result.order} cuts are not the freshly rebuilt "
                "exhaustive Gamma cuts"
            )
        rebuilt.append(filtration)
    return tuple(rebuilt)


def serialize_oracle_result(
    result: OracleResult,
    *,
    run_id: str | None = None,
    software_environment: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    """Convert an internal result to the active v2 schema.

    The concrete event/forest adapters are installed once the sibling
    reference dataclasses are available.  Keeping the public entry point here
    lets callers depend on a stable serialization boundary meanwhile.
    """

    return _serialize_shared_result(
        result,
        run_id=run_id,
        software_environment=software_environment,
    )


def serialize_oracle_cuts(result: OracleResult) -> dict[str, Any]:
    """Serialize every open/closed oracle cut as a separate versioned artifact.

    The frozen ``MorseHGP3DResult`` schema intentionally has no cut table.  The
    exhaustive phase-1 replay data therefore lives in this independent
    canonical artifact instead of adding an unknown field to the v2 contract.
    Coordinates are stored as exact rational triples so rational-only oracle
    fixtures remain serializable even when they are not binary64 inputs.
    """

    _validated_exhaustive_gamma_filtrations(result)

    input_points = [
        {
            "schema_version": SCHEMA_VERSION,
            "point_id": point.point_id,
            "source_indices": list(point.source_indices),
            "coordinates_exact": exact_rational3(point.coordinates),
        }
        for point in result.input_points
    ]
    input_id = canonical_id("OracleCutInput", input_points)
    order_records = []
    all_cut_ids = []
    # Both public profiles are replayed against exhaustive Gamma.  A raw
    # Gabriel cut is a useful diagnostic artifact, but it is not the
    # normative hgp_reduced hierarchy and must never cross this boundary as
    # though it were exact.
    expected_graph_kind = "gamma"
    for order_result in result.orders:
        cuts = tuple(
            sorted(
                order_result.cuts,
                key=lambda cut: (Fraction(cut.squared_level), cut.closed),
            )
        )
        expected_states = tuple(
            (level, closed)
            for level in order_result.critical_levels
            for closed in (False, True)
        )
        actual_states = tuple(
            (Fraction(cut.squared_level), bool(cut.closed)) for cut in cuts
        )
        if actual_states != expected_states:
            raise ValueError(
                f"order {order_result.order} does not contain every canonical open/closed cut"
            )
        cut_records = []
        for cut in cuts:
            if cut.order != order_result.order or cut.graph_kind != expected_graph_kind:
                raise ValueError("an oracle cut disagrees with its order or profile")
            components = []
            for component in cut.components:
                component_projection = {
                    "facet_point_ids": _canonical_labels(
                        component.facet_point_ids
                    ),
                    "covered_point_ids": list(component.covered_point_ids),
                }
                components.append(
                    {
                        "schema_version": SCHEMA_VERSION,
                        "component_id": canonical_id(
                            "OracleCutComponent", component_projection
                        ),
                        **component_projection,
                    }
                )
            components.sort(key=lambda component: component["component_id"])
            projection = {
                "order": cut.order,
                "profile": result.profile,
                "graph_kind": cut.graph_kind,
                "squared_level_exact": exact_level(cut.squared_level),
                "closed": cut.closed,
                "active_facet_point_ids": _canonical_labels(
                    cut.active_facet_point_ids
                ),
                "components": components,
            }
            cut_id = canonical_id("OracleCut", projection)
            all_cut_ids.append(cut_id)
            cut_records.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "cut_id": cut_id,
                    **projection,
                }
            )
        order_records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "order": order_result.order,
                "critical_levels": [
                    exact_level(level) for level in order_result.critical_levels
                ],
                "cuts": cut_records,
            }
        )
    artifact_projection = {
        "input_id": input_id,
        "profile": result.profile,
        "k_eff": result.k_eff,
        "cut_ids": all_cut_ids,
    }
    return {
        "schema_version": SCHEMA_VERSION,
        "artifact_id": canonical_id("OracleCutsArtifact", artifact_projection),
        "artifact_kind": "morsehgp3d_oracle_cuts",
        "backend": "reference_cpu",
        "profile": result.profile,
        "k_eff": result.k_eff,
        "input_id": input_id,
        "input_points": input_points,
        "orders": order_records,
    }


def _validate_complete_reference_replay(result: OracleResult) -> None:
    """Re-run the default oracle before asserting any completeness bit.

    This applies to both profiles.  ``full_pi0`` remains conditional under
    M.1, but its catalogue, Gamma filtration, batches, attachments and maps
    are still declared complete and therefore must match a fresh reference
    replay.  Labels or stale accounting metrics are never accepted as proof.
    """

    from .oracle import run_oracle

    rebuilt = run_oracle(result.points, result.k_max, profile=result.profile)
    if result.critical_catalog != rebuilt.critical_catalog:
        raise ValueError(
            "cannot serialize reference completeness: critical catalogue is "
            "not the exhaustive deterministic catalogue"
        )
    for actual, expected in zip(result.orders, rebuilt.orders):
        if actual.filtration != expected.filtration:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "Gamma filtration is not exhaustive"
            )
        if actual.critical_levels != expected.critical_levels:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "critical levels are not exhaustive"
            )
        if actual.cuts != expected.cuts:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "cuts are not the exhaustive Gamma cuts"
            )
        if actual.forest != expected.forest:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "forest is not the deterministic exhaustive Gamma replay"
            )
        if actual.hyperedges != expected.hyperedges:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "Gabriel diagnostics are incomplete"
            )
        if actual.attachments != expected.attachments:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "attachments are not the deterministic reference replay"
            )
        if actual.equal_level_batches != expected.equal_level_batches:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "equal-level batches are not the deterministic reference replay"
            )
        if actual.coverage_log != expected.coverage_log:
            raise ValueError(
                f"cannot serialize reference completeness: order {actual.order} "
                "coverage log is not the deterministic reference replay"
            )
    if result.vertical_maps != rebuilt.vertical_maps:
        raise ValueError(
            "cannot serialize reference completeness: vertical maps are not "
            "the deterministic reference replay"
        )
    if result.metadata.get("reference_metrics") != rebuilt.metadata.get(
        "reference_metrics"
    ):
        raise ValueError(
            "cannot serialize reference completeness: accounting metrics do "
            "not match the deterministic reference replay"
        )


def _serialize_shared_result(
    result: OracleResult,
    *,
    run_id: str | None,
    software_environment: Mapping[str, Any] | None,
) -> dict[str, Any]:
    reduced_exact = result.profile == "hgp_reduced"
    if reduced_exact:
        non_gamma_orders = [
            order_result.order
            for order_result in result.orders
            if getattr(order_result.forest, "graph_kind", None) != "gamma"
            or any(cut.graph_kind != "gamma" for cut in order_result.cuts)
        ]
        if non_gamma_orders:
            raise ValueError(
                "cannot serialize hgp_reduced as exact: exhaustive Gamma provenance "
                f"is missing at orders {non_gamma_orders!r}"
            )
    _validate_complete_reference_replay(result)

    points = [certified_point(point) for point in result.input_points]
    input_digest = input_sha256(points)

    event_records, event_ids = _serialize_events(result.critical_catalog)
    gamma_coface_records, gamma_coface_ids = _serialize_gamma_cofaces(result)
    hyperedge_records, hyperedge_ids = _serialize_hyperedges(result, event_ids)
    attachment_ids = {
        _attachment_key(attachment): canonical_id(
            "Attachment",
            {
                "event_id": event_ids[_event_key(attachment.event)],
                "order": attachment.order,
                "removed_shell_id": attachment.removed_shell_id,
            },
        )
        for attachment in result.attachments
    }

    batch_ids: dict[tuple[int, Fraction], str] = {}
    batch_event_ids: dict[tuple[int, Fraction], tuple[str, ...]] = {}
    batch_gamma_coface_ids: dict[tuple[int, Fraction], tuple[str, ...]] = {}
    batch_hyperedge_ids: dict[tuple[int, Fraction], tuple[str, ...]] = {}
    batch_attachment_ids: dict[tuple[int, Fraction], tuple[str, ...]] = {}
    for order_result in result.orders:
        for batch in order_result.forest.batches:
            key = (batch.order, batch.squared_level)
            events = _events_for_batch(
                result.critical_catalog, event_ids, batch
            )
            gamma_relations = tuple(
                sorted(
                    gamma_coface_ids[_gamma_coface_key(batch.order, coface)]
                    for coface in order_result.filtration.cofaces
                    if coface.squared_level == batch.squared_level
                    and coface.point_ids in batch.relation_simplex_point_ids
                )
            )
            edges = tuple(
                sorted(
                    hyperedge_ids[_hyperedge_key(edge)]
                    for edge in order_result.hyperedges
                    if edge.squared_level == batch.squared_level
                    and edge.simplex_point_ids
                    in batch.relation_simplex_point_ids
                )
            )
            attachments = tuple(
                sorted(
                    attachment_ids[_attachment_key(attachment)]
                    for attachment in order_result.attachments
                    if attachment.event_squared_level == batch.squared_level
                )
            )
            projection = {
                "order": batch.order,
                "squared_level_exact": exact_level(batch.squared_level),
                "event_ids": list(events),
                "gamma_coface_ids": list(gamma_relations),
                "hyperedge_ids": list(edges),
                "attachment_ids": list(attachments),
            }
            batch_ids[key] = canonical_id("EqualLevelBatch", projection)
            batch_event_ids[key] = events
            batch_gamma_coface_ids[key] = gamma_relations
            batch_hyperedge_ids[key] = edges
            batch_attachment_ids[key] = attachments

    node_ids: dict[tuple[int, str], str] = {}
    node_event_ids: dict[tuple[int, str], tuple[str, ...]] = {}
    node_records_by_order: dict[int, list[dict[str, Any]]] = {}
    for order_result in result.orders:
        forest = order_result.forest
        records: list[dict[str, Any]] = []
        for node in forest.nodes:
            batch_key = (forest.order, node.squared_level)
            events = _events_for_node(
                result.critical_catalog,
                event_ids,
                node,
                fallback=batch_event_ids[batch_key],
            )
            children = tuple(
                sorted(node_ids[(forest.order, child_id)] for child_id in node.child_ids)
            )
            projection = {
                "order": forest.order,
                "profile": result.profile,
                "kind": node.kind,
                "squared_level": exact_level(node.squared_level),
                "child_ids": list(children),
                "event_ids": list(events),
                "batch_id": batch_ids[batch_key],
            }
            public_id = canonical_id("MergeNode", projection)
            node_ids[(forest.order, node.node_id)] = public_id
            node_event_ids[(forest.order, node.node_id)] = events
            records.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "node_id": public_id,
                    "kind": node.kind,
                    "squared_level": exact_level(node.squared_level),
                    "child_ids": list(children),
                    "covered_point_ids": list(node.covered_point_ids),
                    "event_ids": list(events),
                    "batch_id": batch_ids[batch_key],
                }
            )
        node_records_by_order[forest.order] = sorted(
            records, key=lambda record: record["node_id"]
        )

    attachment_records = _serialize_attachments(
        result, event_ids, attachment_ids, node_ids
    )
    batch_records: list[dict[str, Any]] = []
    coverage_by_order: dict[int, list[dict[str, Any]]] = {
        order: [] for order in range(1, result.k_eff + 1)
    }
    for order_result in result.orders:
        for batch in order_result.forest.batches:
            key = (batch.order, batch.squared_level)
            if key not in batch_ids:
                continue
            batch_id = batch_ids[key]
            deltas = [
                _coverage_delta(delta, batch_id, node_ids)
                for delta in batch.coverage_deltas
            ]
            deltas.sort(key=lambda record: record["root_node_id"])
            coverage_by_order[batch.order].extend(deltas)
            batch_records.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "batch_id": batch_id,
                    "order": batch.order,
                    "squared_level_exact": exact_level(batch.squared_level),
                    "event_ids": list(batch_event_ids[key]),
                    "gamma_coface_ids": list(batch_gamma_coface_ids[key]),
                    "hyperedge_ids": list(batch_hyperedge_ids[key]),
                    "attachment_ids": list(batch_attachment_ids[key]),
                    "pre_lot_components": _component_states(
                        batch.pre_components, batch.order, node_ids
                    ),
                    "post_lot_components": _component_states(
                        batch.post_components, batch.order, node_ids
                    ),
                    "coverage_deltas": deltas,
                    "events_complete": True,
                    "attachments_complete": True,
                    "batch_status": "closed",
                }
            )
    batch_records.sort(key=lambda record: record["batch_id"])

    semantics = "exact" if reduced_exact else "partial_refinement"
    forest_records = _serialize_forests(
        result,
        semantics,
        node_ids,
        node_records_by_order,
        coverage_by_order,
    )
    vertical_records = _serialize_vertical_maps(
        result, node_ids, node_event_ids
    )
    coverage_log = sorted(
        (record for records in coverage_by_order.values() for record in records),
        key=canonical_json,
    )
    materialized = sorted(
        (
            {
                "schema_version": SCHEMA_VERSION,
                "order": order_result.order,
                "node_id": node_ids[(order_result.order, node.node_id)],
                "point_ids": list(node.covered_point_ids),
            }
            for order_result in result.orders
            for node in order_result.forest.nodes
        ),
        key=canonical_json,
    )

    if run_id is None:
        effective_run_id = canonical_id(
            "Run",
            {
                "input_sha256": input_digest,
                "k_max": result.k_max,
                "profile": result.profile,
                "backend": "reference_cpu",
                "mode": "certified",
            },
        )
    else:
        if re.fullmatch(r"[0-9a-f]{64}", run_id) is None:
            raise ValueError("run_id must be a lower-case 64-digit hexadecimal ID")
        effective_run_id = run_id

    m_star = 0 if len(points) == 1 else min(result.k_eff - 1, len(points) - 2)
    partial_scope = {
        "schema_version": SCHEMA_VERSION,
        "closed_parent_depths": list(range(m_star + 1)),
        "closed_orders": list(range(1, result.k_eff + 1)),
        "catalog_complete_ranks": list(range(1, result.s_max + 1)),
        "canonical_cell_certificates": [],
        "verified_event_ids": sorted(record["event_id"] for record in event_records),
        "unresolved_loci": [],
        "positive_guarantees": ["partial_forest_refines_exact", "verified_events"],
        "absence_assertions_allowed": False,
    }

    environment = _software_environment(software_environment)
    certificate = _run_certificate(
        result=result,
        run_id=effective_run_id,
        input_digest=input_digest,
        event_count=len(event_records),
        hyperedge_count=len(hyperedge_records),
        gamma_coface_count=len(gamma_coface_records),
        semantics=semantics,
        environment=environment,
    )
    scientific_projection = {
        "input_sha256": input_digest,
        "profile": result.profile,
        "forest_semantics": semantics,
        "event_ids": [record["event_id"] for record in event_records],
        "hyperedge_ids": [record["hyperedge_id"] for record in hyperedge_records],
        "gamma_coface_ids": [record["coface_id"] for record in gamma_coface_records],
        "attachment_ids": [record["attachment_id"] for record in attachment_records],
        "batch_ids": [record["batch_id"] for record in batch_records],
        "forest_ids": [record["forest_id"] for record in forest_records],
        "vertical_map_ids": [record["map_id"] for record in vertical_records],
    }
    contract: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "result_id": canonical_id("MorseHGP3DResult", scientific_projection),
        "run_id": effective_run_id,
        "result_kind": "hierarchy" if reduced_exact else "partial_hierarchy",
        "backend": "reference_cpu",
        "profile": result.profile,
        "mode": "certified",
        "forest_semantics": semantics,
        "k_eff": result.k_eff,
        "embedded_input_points": points,
        "critical_catalog": event_records,
        "gamma_cofaces": gamma_coface_records,
        "gabriel_hyperedges": hyperedge_records,
        "attachments": attachment_records,
        "equal_level_batches": batch_records,
        "forests": forest_records,
        "vertical_maps": vertical_records,
        "coverage_log": coverage_log,
        "materialized_point_sets": materialized,
        "run_certificate": certificate,
    }
    if not reduced_exact:
        contract["partial_scope"] = partial_scope
    return contract


def _event_key(event: Any) -> tuple[tuple[int, ...], Fraction]:
    return tuple(event.point_ids), Fraction(event.squared_radius)


def _hyperedge_key(edge: Any) -> tuple[int, tuple[int, ...], Fraction]:
    return edge.order, tuple(edge.simplex_point_ids), Fraction(edge.squared_level)


def _gamma_coface_key(
    order: int, coface: Any
) -> tuple[int, tuple[int, ...], Fraction]:
    return order, tuple(coface.point_ids), Fraction(coface.squared_level)


def _attachment_key(attachment: Any) -> tuple[tuple[int, ...], Fraction, int, int]:
    event_points, level = _event_key(attachment.event)
    return event_points, level, attachment.order, attachment.removed_shell_id


def _canonical_labels(labels: Iterable[Sequence[int]]) -> list[list[int]]:
    """Order a set of point labels by its canonical JSON representation."""

    return sorted((list(label) for label in labels), key=canonical_json)


def _serialize_events(
    events: Sequence[Any],
) -> tuple[list[dict[str, Any]], dict[tuple[tuple[int, ...], Fraction], str]]:
    records = []
    identifiers: dict[tuple[tuple[int, ...], Fraction], str] = {}
    for event in events:
        support_ids = tuple(event.minimal_support_ids)
        signs = [
            1 if value > 0 else -1 if value < 0 else 0
            for value in event.barycentric_coordinates
        ]
        roles = []
        if event.saddle_order is not None:
            roles.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "order": event.saddle_order,
                    "morse_index": 1,
                    "local_multiplicity": event.local_multiplicity(
                        event.saddle_order
                    ),
                    "arm_count": len(event.shell_ids),
                }
            )
        if event.birth_order is not None:
            roles.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "order": event.birth_order,
                    "morse_index": 0,
                    "local_multiplicity": 1,
                    "arm_count": 0,
                }
            )
        roles.sort(key=lambda role: role["order"])
        witness = exact_rational3(event.center)
        level = exact_level(event.squared_radius)
        projection = {
            "interior_ids": list(event.interior_ids),
            "shell_ids": list(event.shell_ids),
            "minimal_support_ids": list(support_ids),
            "center_witness_homogeneous": witness,
            "squared_level_exact": level,
        }
        event_id = canonical_id("CriticalEvent", projection)
        key = _event_key(event)
        if key in identifiers:
            raise ValueError(f"duplicate critical event identity {key!r}")
        identifiers[key] = event_id
        records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "event_id": event_id,
                "closed_rank": event.closed_rank,
                "interior_ids": list(event.interior_ids),
                "shell_ids": list(event.shell_ids),
                "minimal_support_ids": list(support_ids),
                "center_witness_homogeneous": witness,
                "squared_level_exact": level,
                "barycentric_signs": signs,
                "birth_order": event.birth_order,
                "saddle_order": event.saddle_order,
                "morse_roles": roles,
                "degeneracy_class": "cospherical_shell"
                if tuple(event.shell_ids) != support_ids
                else "general_position",
                "predicate_status": "certified_exact",
            }
        )
    records.sort(key=lambda record: record["event_id"])
    return records, identifiers


def _serialize_hyperedges(
    result: OracleResult,
    event_ids: Mapping[tuple[tuple[int, ...], Fraction], str],
) -> tuple[list[dict[str, Any]], dict[tuple[int, tuple[int, ...], Fraction], str]]:
    records = []
    identifiers = {}
    for edge in result.gabriel_hyperedges:
        event_key = (tuple(edge.simplex_point_ids), Fraction(edge.squared_level))
        try:
            event_id = event_ids[event_key]
        except KeyError as error:
            raise ValueError(
                f"Gabriel hyperedge {edge.simplex_point_ids!r} has no critical event"
            ) from error
        event = next(item for item in result.critical_catalog if _event_key(item) == event_key)
        strict_arms = _canonical_labels(
            tuple(
                tuple(
                    point_id
                    for point_id in edge.simplex_point_ids
                    if point_id != removed_shell_id
                )
                for removed_shell_id in event.shell_ids
            )
        )
        facets = _canonical_labels(edge.facet_point_ids)
        projection = {
            "event_id": event_id,
            "order": edge.order,
            "simplex_point_ids": list(edge.simplex_point_ids),
            "facet_point_ids": facets,
            "strict_arm_point_ids": strict_arms,
            "squared_level_exact": exact_level(edge.squared_level),
        }
        hyperedge_id = canonical_id("GabrielHyperedge", projection)
        identifiers[_hyperedge_key(edge)] = hyperedge_id
        records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "hyperedge_id": hyperedge_id,
                "event_id": event_id,
                "order": edge.order,
                "simplex_point_ids": list(edge.simplex_point_ids),
                "facet_point_ids": facets,
                "strict_arm_point_ids": strict_arms,
                "star_pivot_facet_index": 0,
                "squared_level_exact": exact_level(edge.squared_level),
                "covered_point_ids": list(edge.simplex_point_ids),
                "predicate_status": "certified_exact",
            }
        )
    records.sort(key=lambda record: record["hyperedge_id"])
    return records, identifiers


def _serialize_gamma_cofaces(
    result: OracleResult,
) -> tuple[
    list[dict[str, Any]],
    dict[tuple[int, tuple[int, ...], Fraction], str],
]:
    records: list[dict[str, Any]] = []
    identifiers: dict[tuple[int, tuple[int, ...], Fraction], str] = {}
    for order_result in result.orders:
        for coface in order_result.filtration.cofaces:
            facets = _canonical_labels(coface.facet_point_ids)
            projection = {
                "order": order_result.order,
                "simplex_point_ids": list(coface.point_ids),
                "facet_point_ids": facets,
                "squared_level_exact": exact_level(coface.squared_level),
            }
            coface_id = canonical_id("GammaCoface", projection)
            key = _gamma_coface_key(order_result.order, coface)
            if key in identifiers:
                raise ValueError(f"duplicate Gamma coface identity {key!r}")
            identifiers[key] = coface_id
            records.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "coface_id": coface_id,
                    **projection,
                    "predicate_status": "certified_exact",
                }
            )
    records.sort(key=lambda record: record["coface_id"])
    return records, identifiers


def _event_has_role(event: Any, order: int) -> bool:
    return event.birth_order == order or event.saddle_order == order


def _events_for_batch(
    catalog: Sequence[Any],
    event_ids: Mapping[tuple[tuple[int, ...], Fraction], str],
    batch: Any,
) -> tuple[str, ...]:
    sources = set(batch.activated_facet_point_ids) | set(
        batch.relation_simplex_point_ids
    )
    return tuple(
        sorted(
            event_ids[_event_key(event)]
            for event in catalog
            if event.squared_radius == batch.squared_level
            and _event_has_role(event, batch.order)
            and tuple(event.point_ids) in sources
        )
    )


def _events_for_node(
    catalog: Sequence[Any],
    event_ids: Mapping[tuple[tuple[int, ...], Fraction], str],
    node: Any,
    *,
    fallback: tuple[str, ...],
) -> tuple[str, ...]:
    sources = set(node.source_point_labels)
    matches = tuple(
        sorted(
            event_ids[_event_key(event)]
            for event in catalog
            if event.squared_radius == node.squared_level
            and _event_has_role(event, node.order)
            and tuple(event.point_ids) in sources
        )
    )
    return matches or fallback


def _serialize_attachments(
    result: OracleResult,
    event_ids: Mapping[tuple[tuple[int, ...], Fraction], str],
    attachment_ids: Mapping[tuple[tuple[int, ...], Fraction, int, int], str],
    node_ids: Mapping[tuple[int, str], str],
) -> list[dict[str, Any]]:
    records = []
    for attachment in result.attachments:
        records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "attachment_id": attachment_ids[_attachment_key(attachment)],
                "event_id": event_ids[_event_key(attachment.event)],
                "order": attachment.order,
                "removed_shell_id": attachment.removed_shell_id,
                "arm_point_ids": list(attachment.arm_point_ids),
                "event_squared_level": exact_level(
                    attachment.event_squared_level
                ),
                "target_node_id": node_ids[
                    (attachment.order, attachment.target_node_id)
                ],
                "method": "reference_oracle",
                "descent_path": [],
                "predicate_status": "certified_exact",
            }
        )
    records.sort(key=lambda record: record["attachment_id"])
    return records


def _component_states(
    components: Sequence[Any],
    order: int,
    node_ids: Mapping[tuple[int, str], str],
) -> list[dict[str, Any]]:
    records = [
        {
            "schema_version": SCHEMA_VERSION,
            "component_id": node_ids[(order, component.root_id)],
            "active_facet_point_ids": _canonical_labels(
                component.facet_point_ids
            ),
            "covered_point_ids": list(component.covered_point_ids),
        }
        for component in components
    ]
    return sorted(records, key=lambda record: record["component_id"])


def _coverage_delta(
    delta: Any,
    batch_id: str,
    node_ids: Mapping[tuple[int, str], str],
) -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "batch_id": batch_id,
        "order": delta.order,
        "root_node_id": node_ids[(delta.order, delta.root_id)],
        "added_facet_point_ids": _canonical_labels(
            delta.added_facet_point_ids
        ),
        "added_point_ids": list(delta.added_point_ids),
    }


def _serialize_forests(
    result: OracleResult,
    semantics: str,
    node_ids: Mapping[tuple[int, str], str],
    node_records_by_order: Mapping[int, list[dict[str, Any]]],
    coverage_by_order: Mapping[int, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    records = []
    for order_result in result.orders:
        forest = order_result.forest
        roots = sorted(node_ids[(forest.order, root)] for root in forest.root_ids)
        coverage = sorted(coverage_by_order[forest.order], key=canonical_json)
        projection = {
            "order": forest.order,
            "profile": result.profile,
            "forest_semantics": semantics,
            "node_ids": [
                node["node_id"] for node in node_records_by_order[forest.order]
            ],
            "root_ids": roots,
            "coverage_log": coverage,
        }
        records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "forest_id": canonical_id("MergeForest", projection),
                "order": forest.order,
                "profile": result.profile,
                "forest_semantics": semantics,
                "nodes": node_records_by_order[forest.order],
                "root_ids": roots,
                "coverage_log": coverage,
                "complete": True,
            }
        )
    return records


def _serialize_vertical_maps(
    result: OracleResult,
    node_ids: Mapping[tuple[int, str], str],
    node_event_ids: Mapping[tuple[int, str], tuple[str, ...]],
) -> list[dict[str, Any]]:
    records = []
    for family in result.vertical_maps:
        source_forest = result.for_order(family.source_order).forest
        target_forest = result.for_order(family.target_order).forest
        if family.naturality_failures:
            raise ValueError(
                "cannot serialize a vertical map with non-commuting profile squares"
            )
        closed_maps = {
            mapping.squared_level: mapping
            for mapping in family.maps
            if mapping.closed
        }
        assignments = []
        for node in source_forest.nodes:
            source_cut = source_forest.cut(node.squared_level, closed=True)
            source_components = [
                component
                for component in source_cut.components
                if component.root_id == node.node_id
            ]
            if len(source_components) != 1:
                raise ValueError(
                    "a source forest node has no unique component at its own level"
                )
            try:
                profile_map = closed_maps[node.squared_level].assignment_by_source
                target_facets = profile_map[
                    source_components[0].facet_point_ids
                ]
            except KeyError as error:
                raise ValueError(
                    "a source forest node has no certified profile vertical image"
                ) from error
            target_cut = target_forest.cut(node.squared_level, closed=True)
            candidates = [
                component
                for component in target_cut.components
                if component.facet_point_ids == target_facets
            ]
            if len(candidates) != 1:
                raise ValueError(
                    "no unique lower-order profile root matches source node "
                    f"{node.node_id!r} at level {node.squared_level}"
                )
            target = candidates[0]
            proof_ids = node_event_ids[(source_forest.order, node.node_id)]
            assignments.append(
                {
                    "schema_version": SCHEMA_VERSION,
                    "source_node_id": node_ids[
                        (source_forest.order, node.node_id)
                    ],
                    "target_node_id": node_ids[
                        (target_forest.order, target.root_id)
                    ],
                    "at_squared_level": exact_level(node.squared_level),
                    "method": "reference_oracle",
                    "proof_reference_ids": list(proof_ids),
                }
            )
        assignments.sort(key=lambda record: record["source_node_id"])
        projection = {
            "source_order": family.source_order,
            "target_order": family.target_order,
            "assignments": assignments,
        }
        records.append(
            {
                "schema_version": SCHEMA_VERSION,
                "map_id": canonical_id("VerticalMap", projection),
                "source_order": family.source_order,
                "target_order": family.target_order,
                "assignments": assignments,
                "complete": True,
                "naturality_squares_checked": family.naturality_squares_checked,
                "naturality_failures": len(family.naturality_failures),
            }
        )
    return records


def _software_environment(overrides: Mapping[str, Any] | None) -> dict[str, Any]:
    environment = {
        "git_commit": "0" * 40,
        "compiler": "python",
        "compiler_version": platform.python_version(),
        "cuda_version": None,
        "driver_version": None,
        "hardware": "reference-cpu",
        "build_options_sha256": canonical_id(
            "BuildOptions", {"implementation": "independent-reference-oracle"}
        ),
    }
    if overrides is not None:
        unknown = set(overrides) - set(environment)
        if unknown:
            raise ValueError(f"unknown software environment fields: {sorted(unknown)!r}")
        environment.update(overrides)
    return environment


def _run_certificate(
    *,
    result: OracleResult,
    run_id: str,
    input_digest: str,
    event_count: int,
    hyperedge_count: int,
    gamma_coface_count: int,
    semantics: str,
    environment: Mapping[str, Any],
) -> dict[str, Any]:
    reduced = result.profile == "hgp_reduced"
    expected_semantics = "exact" if reduced else "partial_refinement"
    if semantics != expected_semantics:
        raise ValueError(
            f"{result.profile} requires forest_semantics={expected_semantics}"
        )
    metrics = result.metadata.get("reference_metrics")
    required_metrics = {
        "catalog_support_candidates",
        "catalog_affine_dependent_candidates",
        "catalog_accepted_events",
        "catalog_rejected_candidates",
        "catalog_point_classifications",
        "catalog_support_universe_complete",
        "canonical_cells_required",
        "canonical_cells_closed",
        "active_cross_incidences_required",
        "active_cross_incidences_closed",
        "gamma_miniball_queries",
        "gamma_facets_enumerated",
        "gamma_cofaces_enumerated",
    }
    if not isinstance(metrics, Mapping) or not required_metrics <= set(metrics):
        raise ValueError(
            "the reference oracle result lacks exhaustive accounting metrics"
        )
    if any(
        isinstance(metrics[name], bool)
        or not isinstance(metrics[name], int)
        or metrics[name] < 0
        for name in required_metrics
    ):
        raise ValueError("reference oracle accounting metrics must be non-negative integers")
    if metrics["catalog_accepted_events"] != event_count:
        raise ValueError("catalogue accounting disagrees with serialized events")
    if (
        metrics["catalog_support_candidates"] - event_count
        != metrics["catalog_rejected_candidates"]
    ):
        raise ValueError("catalogue candidate accounting is not closed")
    if metrics["catalog_support_universe_complete"] != 1:
        raise ValueError("the exhaustive 3D support universe is not certified complete")
    point_count = len(result.input_points)
    expected_gamma_facets = sum(
        comb(point_count, order) for order in range(1, result.k_eff + 1)
    )
    expected_gamma_cofaces = sum(
        comb(point_count, order + 1)
        for order in range(1, result.k_eff + 1)
        if order < point_count
    )
    if metrics["gamma_facets_enumerated"] != expected_gamma_facets:
        raise ValueError("Gamma facet accounting is not exhaustive")
    if metrics["gamma_cofaces_enumerated"] != expected_gamma_cofaces:
        raise ValueError("Gamma coface accounting is not exhaustive")
    if metrics["gamma_miniball_queries"] != (
        expected_gamma_facets + expected_gamma_cofaces
    ):
        raise ValueError("Gamma miniball accounting is not exhaustive")
    canonical_children_complete = (
        metrics["canonical_cells_closed"]
        == metrics["canonical_cells_required"]
    )
    active_cross_incidences_complete = (
        metrics["active_cross_incidences_closed"]
        == metrics["active_cross_incidences_required"]
    )
    if not canonical_children_complete or not active_cross_incidences_complete:
        raise ValueError(
            "the reference result cannot close its enumeration certificate"
        )
    exact_decisions = (
        metrics["catalog_support_candidates"]
        + metrics["catalog_point_classifications"]
        + metrics["gamma_miniball_queries"]
    )
    result_kind = "hierarchy" if reduced else "partial_hierarchy"
    require_exact = reduced
    m_star = (
        0
        if len(result.input_points) == 1
        else min(result.k_eff - 1, len(result.input_points) - 2)
    )
    snapshot_without_id = {
        "schema_version": SCHEMA_VERSION,
        "boundary": "final",
        "sequence_number": 0,
        "byte_unit": "byte",
        "time_unit": "second",
        "device_limit_bytes": None,
        "device_used_bytes": 0,
        "device_reserved_bytes": 0,
        "device_remaining_bytes": None,
        "host_limit_bytes": None,
        "host_used_bytes": 0,
        "host_reserved_bytes": 0,
        "host_remaining_bytes": None,
        "scratch_limit_bytes": None,
        "scratch_used_bytes": 0,
        "scratch_reserved_bytes": 0,
        "scratch_remaining_bytes": None,
        "output_limit_bytes": None,
        "output_used_bytes": 0,
        "output_reserved_bytes": 0,
        "output_remaining_bytes": None,
        "time_limit_s": None,
        "elapsed_s": 0,
        "remaining_s": None,
        "first_refusal_reason": None,
    }
    snapshot = {
        **snapshot_without_id,
        "snapshot_id": canonical_id("BudgetSnapshot", snapshot_without_id),
    }
    certificate_without_id = {
        "schema_version": SCHEMA_VERSION,
        "run_id": run_id,
        "reconstruction_contract_id": "hgp-reduced-v2"
        if reduced
        else "M1-reconstruction-v1",
        "proof_basis": "gamma_exhaustive_reference"
        if reduced
        else "m1_conditional_contract",
        "result_kind": result_kind,
        "requested_backend": "reference_cpu",
        "effective_backend": "reference_cpu",
        "requested_profile": result.profile,
        "effective_profile": result.profile,
        "requested_mode": "certified",
        "require_exact": require_exact,
        "public_status": "exact" if reduced else "conditional",
        "forest_semantics": semantics,
        "input_semantics": {
            "schema_version": SCHEMA_VERSION,
            "coordinate_encoding": "ieee754_binary64_bits",
            "coordinate_unit": COORDINATE_UNIT,
            "squared_level_unit": SQUARED_LEVEL_UNIT,
            "point_indexing": "zero_based",
            "canonical_point_order": (
                "lexicographic_binary64_total_order_then_source_index"
            ),
            "non_finite_policy": "reject",
            "duplicate_policy": "reject",
            "signed_zero_policy": "canonicalize_to_positive_zero",
            "perturbation_policy": "none",
            "input_sha256": input_digest,
            "similarity_transform": {
                "schema_version": SCHEMA_VERSION,
                "applied": False,
                "scale": {
                    "schema_version": SCHEMA_VERSION,
                    "numerator": "1",
                    "denominator": "1",
                },
                "translation": exact_rational3((0, 0, 0)),
            },
        },
        "input_point_count": len(result.input_points),
        "k_max": result.k_max,
        "k_eff": result.k_eff,
        "s_max": result.s_max,
        "m_star": m_star,
        "general_position_status": "not_applicable"
        if len(result.input_points) == 1
        else "verified_relevant",
        "relevant_gp_complete": True,
        # The exhaustive reference enumerates all 1..4 point supports directly
        # and therefore requires no canonical restricted cells. These booleans
        # are derived from the corresponding required/closed counts.
        "canonical_children_complete": canonical_children_complete,
        "active_cross_incidences_complete": active_cross_incidences_complete,
        "catalog_complete_by_rank": [True] * result.s_max,
        "attachments_complete_by_order": [True] * result.k_eff,
        "gamma_complete_by_order": [True] * result.k_eff,
        "batches_complete_by_order": [True] * result.k_eff,
        "vertical_maps_complete": True,
        "partial_guarantees": (
            ["none"]
            if reduced
            else ["partial_forest_refines_exact", "verified_events"]
        ),
        "exact_predicate_counts": {
            "fp32_proposals": 0,
            "fp64_filtered_certified": 0,
            "expansion_certified": 0,
            # Counts certified high-level exact decisions (support candidates,
            # global point/sphere classifications, and exhaustive miniball
            # queries), not individual Fraction arithmetic operations.
            "cpu_multiprecision_certified": exact_decisions,
            "exact_zeros": metrics["catalog_affine_dependent_candidates"],
            "remaining_unknown": 0,
        },
        "fallback_counts": {
            "gpu_to_cpu": 0,
            "multiprecision": 0,
            "overflow_queue": 0,
            "unsupported_degeneracies": 0,
            "numeric_failures": 0,
        },
        "work_and_memory_counters": {
            "candidate_events": metrics["catalog_support_candidates"],
            "accepted_events": event_count,
            "rejected_events": metrics["catalog_rejected_candidates"],
            "canonical_cells_closed": metrics["canonical_cells_closed"],
            "gamma_cofaces_emitted": gamma_coface_count,
            "hyperedges_emitted": hyperedge_count,
            "external_runs": 0,
            "bytes_read": 0,
            "bytes_written": 0,
            "peak_device_bytes": 0,
            "peak_host_bytes": 0,
            "peak_scratch_bytes": 0,
            "output_bytes": 0,
        },
        "budget_policy": {
            "schema_version": SCHEMA_VERSION,
            "mode": "certified",
            "require_exact": require_exact,
            "byte_unit": "byte",
            "time_unit": "second",
            "device_budget_bytes": None,
            "host_budget_bytes": None,
            "scratch_budget_bytes": None,
            "output_budget_bytes": None,
            "time_budget_s": None,
            "stop_policy": "fail_closed",
        },
        "final_budget_snapshot": snapshot,
        "stop_reason": "completed",
        "unresolved_locus_ids": [],
        "software_environment": dict(environment),
    }
    return {
        **certificate_without_id,
        "certificate_id": canonical_id("RunCertificate", certificate_without_id),
    }


def _greatest_common_divisor(left: int, right: int) -> int:
    while right:
        left, right = right, left % right
    return abs(left)


def _least_common_multiple(left: int, right: int) -> int:
    if not left or not right:
        return 0
    return abs(left // _greatest_common_divisor(left, right) * right)
