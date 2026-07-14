#!/usr/bin/env python3
"""Recalculate every canonical ID in the five MorseHGP3D v2 fixtures.

This migration is intentionally independent from
``reference.morsehgp3d_oracle.serialization``.  It treats the fixture as an
object graph, repairs batch component/root references by replaying the forest,
then rekeys the graph in dependency order.  The default mode is read-only and
fails when a fixture is not already at the canonical fixed point.  ``--write``
publishes the migrated JSON atomically.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
import re
import sys
import tempfile
from collections import defaultdict
from fractions import Fraction
from pathlib import Path
from typing import Any, Mapping, Sequence

ROOT = Path(__file__).resolve().parents[1]
FIXTURE_DIR = ROOT / "tests" / "fixtures" / "contracts"
SCHEMA_PATH = ROOT / "schemas" / "morsehgp3d-contract-v2.schema.json"
CONTRACT_DOMAIN = "MorseHGP3D/v2"
SCHEMA_VERSION = "2.0.0"
CANONICAL_ID_RE = re.compile(r"^[0-9a-f]{64}$")


class RekeyError(ValueError):
    """A fixture cannot be migrated without guessing."""


def _object_without_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise RekeyError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_object_without_duplicate_keys,
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise RekeyError(str(exc)) from exc
    if not isinstance(value, dict):
        raise RekeyError("fixture root must be a JSON object")
    return value


def canonical_json(value: Any) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def without_schema_versions(value: Any) -> Any:
    """Remove every nested schema-version key from an identity projection."""

    if isinstance(value, Mapping):
        return {
            key: without_schema_versions(child)
            for key, child in value.items()
            if key != "schema_version"
        }
    if isinstance(value, (list, tuple)):
        return [without_schema_versions(child) for child in value]
    return value


def canonical_id(record_type: str, projection: Any) -> str:
    if not record_type or "/" in record_type:
        raise RekeyError(f"invalid record type {record_type!r}")
    digest = hashlib.sha256()
    digest.update(f"{CONTRACT_DOMAIN}/{record_type}/".encode("ascii"))
    digest.update(canonical_json(without_schema_versions(projection)).encode("utf-8"))
    return digest.hexdigest()


def set_schema_version(value: Any) -> None:
    if isinstance(value, dict):
        if "schema_version" in value:
            value["schema_version"] = SCHEMA_VERSION
        for child in value.values():
            set_schema_version(child)
    elif isinstance(value, list):
        for child in value:
            set_schema_version(child)


def require_id(value: Any, context: str) -> str:
    if not isinstance(value, str) or CANONICAL_ID_RE.fullmatch(value) is None:
        raise RekeyError(f"{context}: expected a lower-case 64-hex ID")
    return value


def index_records(
    records: Any, id_field: str, context: str
) -> dict[str, dict[str, Any]]:
    if not isinstance(records, list):
        raise RekeyError(f"{context}: expected an array")
    result: dict[str, dict[str, Any]] = {}
    for index, record in enumerate(records):
        if not isinstance(record, dict):
            raise RekeyError(f"{context}[{index}]: expected an object")
        record_id = require_id(record.get(id_field), f"{context}[{index}].{id_field}")
        if record_id in result:
            raise RekeyError(f"{context}: duplicate {id_field} {record_id}")
        result[record_id] = record
    return result


def remap_one(value: Any, mapping: Mapping[str, str], context: str) -> str:
    old = require_id(value, context)
    try:
        return mapping[old]
    except KeyError as exc:
        raise RekeyError(f"{context}: dangling reference {old}") from exc


def remap_many(value: Any, mapping: Mapping[str, str], context: str) -> list[str]:
    if not isinstance(value, list):
        raise RekeyError(f"{context}: expected an array")
    mapped = [
        remap_one(item, mapping, f"{context}[{index}]")
        for index, item in enumerate(value)
    ]
    if len(mapped) != len(set(mapped)):
        raise RekeyError(f"{context}: duplicate reference after migration")
    return sorted(mapped)


def exact_level(record: Any, context: str) -> Fraction:
    if not isinstance(record, dict):
        raise RekeyError(f"{context}: expected an exact-level object")
    try:
        numerator = int(record["numerator"])
        denominator = int(record["denominator"])
    except (KeyError, TypeError, ValueError) as exc:
        raise RekeyError(f"{context}: malformed exact level") from exc
    if denominator <= 0 or numerator < 0:
        raise RekeyError(
            f"{context}: level must be non-negative with positive denominator"
        )
    value = Fraction(numerator, denominator)
    if (
        str(value.numerator) != record["numerator"]
        or str(value.denominator) != record["denominator"]
    ):
        raise RekeyError(f"{context}: exact level is not reduced canonically")
    return value


def component_signature(
    component: Any, context: str
) -> tuple[frozenset[tuple[int, ...]], frozenset[int]]:
    if not isinstance(component, dict):
        raise RekeyError(f"{context}: expected a component object")
    try:
        facets = frozenset(
            tuple(int(point) for point in facet)
            for facet in component["active_facet_point_ids"]
        )
        covered = frozenset(int(point) for point in component["covered_point_ids"])
    except (KeyError, TypeError, ValueError) as exc:
        raise RekeyError(f"{context}: malformed component labels") from exc
    if not facets:
        raise RekeyError(f"{context}: a serialized component cannot be empty")
    if len(facets) != len(component["active_facet_point_ids"]):
        raise RekeyError(f"{context}: duplicate active facet")
    if len(covered) != len(component["covered_point_ids"]):
        raise RekeyError(f"{context}: duplicate covered point")
    return facets, covered


def unique_component_index(
    components: Any, context: str
) -> tuple[
    list[dict[str, Any]], dict[tuple[frozenset[tuple[int, ...]], frozenset[int]], int]
]:
    if not isinstance(components, list):
        raise RekeyError(f"{context}: expected an array")
    records: list[dict[str, Any]] = []
    by_signature: dict[tuple[frozenset[tuple[int, ...]], frozenset[int]], int] = {}
    for index, component in enumerate(components):
        if not isinstance(component, dict):
            raise RekeyError(f"{context}[{index}]: expected an object")
        signature = component_signature(component, f"{context}[{index}]")
        if signature in by_signature:
            raise RekeyError(f"{context}: duplicate component state")
        by_signature[signature] = index
        records.append(component)
    return records, by_signature


def _unique_birth_matching(
    births: Sequence[str],
    nodes: Mapping[str, dict[str, Any]],
    post_records: Sequence[dict[str, Any]],
    incoming: Mapping[int, list[str]],
    occupied: set[int],
    context: str,
) -> dict[str, int]:
    candidates: dict[str, tuple[int, ...]] = {}
    for node_id in births:
        covered = frozenset(nodes[node_id]["covered_point_ids"])
        options = tuple(
            index
            for index, component in enumerate(post_records)
            if index not in occupied
            and not incoming.get(index)
            and component_signature(component, f"{context}.post[{index}]")[1] == covered
        )
        if not options:
            raise RekeyError(
                f"{context}: birth node {node_id} matches no post-lot component"
            )
        candidates[node_id] = options

    solutions: list[dict[str, int]] = []
    ordered = sorted(births, key=lambda node_id: (len(candidates[node_id]), node_id))

    def search(position: int, used: set[int], assignment: dict[str, int]) -> None:
        if len(solutions) > 1:
            return
        if position == len(ordered):
            solutions.append(dict(assignment))
            return
        node_id = ordered[position]
        for target in candidates[node_id]:
            if target in used:
                continue
            assignment[node_id] = target
            search(position + 1, used | {target}, assignment)
            assignment.pop(node_id)

    search(0, set(), {})
    if len(solutions) != 1:
        raise RekeyError(f"{context}: component assignment for births is ambiguous")
    return solutions[0]


def repair_component_roots(result: dict[str, Any]) -> None:
    """Replay every forest and replace component IDs by its active root node ID."""

    forests = result.get("forests")
    batches = result.get("equal_level_batches")
    if not isinstance(forests, list) or not isinstance(batches, list):
        raise RekeyError("result must contain forest and equal-level batch arrays")

    all_nodes: dict[str, dict[str, Any]] = {}
    node_order: dict[str, int] = {}
    for forest_index, forest in enumerate(forests):
        if not isinstance(forest, dict) or not isinstance(forest.get("order"), int):
            raise RekeyError(f"forests[{forest_index}]: malformed forest")
        order = forest["order"]
        indexed = index_records(
            forest.get("nodes"), "node_id", f"forests[{forest_index}].nodes"
        )
        for node_id, node in indexed.items():
            if node_id in all_nodes:
                raise RekeyError(f"node ID {node_id} occurs in more than one forest")
            all_nodes[node_id] = node
            node_order[node_id] = order

    batch_ids = index_records(batches, "batch_id", "equal_level_batches")
    for order in sorted({forest["order"] for forest in forests}):
        matching_forests = [forest for forest in forests if forest["order"] == order]
        if len(matching_forests) != 1:
            raise RekeyError(f"order {order}: expected exactly one forest")
        forest = matching_forests[0]
        nodes = {
            node_id: node
            for node_id, node in all_nodes.items()
            if node_order[node_id] == order
        }

        for node_id, node in nodes.items():
            children = node.get("child_ids")
            if not isinstance(children, list) or len(children) != len(set(children)):
                raise RekeyError(f"node {node_id}: malformed or duplicate children")
            node_level = exact_level(
                node.get("squared_level"), f"node {node_id}.squared_level"
            )
            for child in children:
                child_id = require_id(child, f"node {node_id}.child_ids")
                if child_id not in nodes:
                    raise RekeyError(
                        f"node {node_id}: foreign or missing child {child_id}"
                    )
                child_level = exact_level(
                    nodes[child_id].get("squared_level"),
                    f"node {child_id}.squared_level",
                )
                if child_level >= node_level:
                    raise RekeyError(
                        f"node {node_id}: child {child_id} is not at a lower level"
                    )

        order_batches = [batch for batch in batches if batch.get("order") == order]
        order_batches.sort(
            key=lambda batch: exact_level(
                batch.get("squared_level_exact"), f"order {order} batch level"
            )
        )
        levels: set[Fraction] = set()
        for batch in order_batches:
            level = exact_level(
                batch["squared_level_exact"], f"order {order} batch level"
            )
            if level in levels:
                raise RekeyError(f"order {order}: more than one batch at level {level}")
            levels.add(level)
        nodes_by_level: dict[Fraction, list[str]] = defaultdict(list)
        for node_id, node in nodes.items():
            nodes_by_level[
                exact_level(node["squared_level"], f"node {node_id}.squared_level")
            ].append(node_id)
        missing_levels = set(nodes_by_level) - levels
        if missing_levels:
            raise RekeyError(
                f"order {order}: nodes have no batch at levels {sorted(missing_levels)}"
            )

        active: dict[tuple[frozenset[tuple[int, ...]], frozenset[int]], str] = {}
        for batch_position, batch in enumerate(order_batches):
            level = exact_level(
                batch["squared_level_exact"], f"order {order} batch level"
            )
            context = f"order {order}, level {level}"
            pre_records, pre_by_signature = unique_component_index(
                batch.get("pre_lot_components"), f"{context}.pre"
            )
            post_records, _ = unique_component_index(
                batch.get("post_lot_components"), f"{context}.post"
            )
            if set(pre_by_signature) != set(active):
                raise RekeyError(
                    f"{context}: pre-lot state is not the preceding closed state"
                )
            pre_root_by_index = {
                index: active[signature]
                for signature, index in pre_by_signature.items()
            }
            for index, component in enumerate(pre_records):
                component["component_id"] = pre_root_by_index[index]

            incoming: dict[int, list[str]] = defaultdict(list)
            target_by_pre_root: dict[str, int] = {}
            for pre_index, pre_component in enumerate(pre_records):
                pre_facets, pre_covered = component_signature(
                    pre_component, f"{context}.pre[{pre_index}]"
                )
                options = []
                for post_index, post_component in enumerate(post_records):
                    post_facets, post_covered = component_signature(
                        post_component, f"{context}.post[{post_index}]"
                    )
                    if pre_facets <= post_facets and pre_covered <= post_covered:
                        options.append(post_index)
                if len(options) != 1:
                    raise RekeyError(
                        f"{context}: a pre component has {len(options)} possible post images"
                    )
                root = pre_root_by_index[pre_index]
                target_by_pre_root[root] = options[0]
                incoming[options[0]].append(root)

            new_nodes = sorted(nodes_by_level.get(level, ()))
            assigned_new: dict[str, int] = {}
            occupied: set[int] = set()
            births: list[str] = []
            for node_id in new_nodes:
                children = [
                    require_id(child, f"node {node_id}.child_ids")
                    for child in nodes[node_id]["child_ids"]
                ]
                if not children:
                    births.append(node_id)
                    continue
                if any(child not in target_by_pre_root for child in children):
                    raise RekeyError(
                        f"{context}: node {node_id} consumes a non-active child"
                    )
                targets = {target_by_pre_root[child] for child in children}
                if len(targets) != 1:
                    raise RekeyError(
                        f"{context}: children of node {node_id} enter different components"
                    )
                target = next(iter(targets))
                if target in occupied:
                    raise RekeyError(
                        f"{context}: two new nodes claim one post component"
                    )
                if set(incoming[target]) != set(children):
                    raise RekeyError(
                        f"{context}: node {node_id} does not list every merged pre root"
                    )
                assigned_new[node_id] = target
                occupied.add(target)

            birth_assignment = _unique_birth_matching(
                births, nodes, post_records, incoming, occupied, context
            )
            assigned_new.update(birth_assignment)
            occupied.update(birth_assignment.values())
            new_by_target = {
                target: node_id for node_id, target in assigned_new.items()
            }
            if len(new_by_target) != len(assigned_new):
                raise RekeyError(f"{context}: duplicate new-node target")

            post_root_by_index: dict[int, str] = {}
            for post_index, post_component in enumerate(post_records):
                if post_index in new_by_target:
                    root = new_by_target[post_index]
                else:
                    roots = incoming.get(post_index, [])
                    if len(roots) != 1:
                        raise RekeyError(
                            f"{context}: post component has no unique persistent root"
                        )
                    root = roots[0]
                    if any(
                        root in nodes[new_node]["child_ids"]
                        for new_node in assigned_new
                    ):
                        raise RekeyError(
                            f"{context}: consumed root {root} remained active"
                        )
                post_component["component_id"] = root
                post_root_by_index[post_index] = root

            deltas = batch.get("coverage_deltas")
            if not isinstance(deltas, list):
                raise RekeyError(f"{context}: coverage_deltas must be an array")
            for delta_index, delta in enumerate(deltas):
                try:
                    added_facets = frozenset(
                        tuple(int(point) for point in facet)
                        for facet in delta["added_facet_point_ids"]
                    )
                    added_points = frozenset(
                        int(point) for point in delta["added_point_ids"]
                    )
                except (KeyError, TypeError, ValueError) as exc:
                    raise RekeyError(
                        f"{context}.coverage_deltas[{delta_index}]: malformed delta"
                    ) from exc
                candidates = []
                for post_index, component in enumerate(post_records):
                    facets, covered = component_signature(
                        component, f"{context}.post[{post_index}]"
                    )
                    if added_facets <= facets and added_points <= covered:
                        candidates.append(post_root_by_index[post_index])
                candidates = sorted(set(candidates))
                if len(candidates) == 1:
                    root = candidates[0]
                else:
                    raise RekeyError(
                        f"{context}.coverage_deltas[{delta_index}]: root is ambiguous among {candidates}"
                    )
                delta["root_node_id"] = root
                delta["batch_id"] = batch["batch_id"]

            active = {
                component_signature(
                    component, f"{context}.post[{index}]"
                ): post_root_by_index[index]
                for index, component in enumerate(post_records)
            }

        expected_roots = {
            require_id(root, f"order {order}.root_ids")
            for root in forest.get("root_ids", [])
        }
        if set(active.values()) != expected_roots:
            raise RekeyError(
                f"order {order}: replay roots disagree with forest.root_ids"
            )

    # A non-null node batch reference must point at the node's own order/level.
    for node_id, node in all_nodes.items():
        old_batch = node.get("batch_id")
        if old_batch is None:
            continue
        batch_id = require_id(old_batch, f"node {node_id}.batch_id")
        if batch_id not in batch_ids:
            raise RekeyError(f"node {node_id}: dangling batch {batch_id}")
        batch = batch_ids[batch_id]
        if batch["order"] != node_order[node_id] or exact_level(
            batch["squared_level_exact"], f"batch {batch_id}"
        ) != exact_level(node["squared_level"], f"node {node_id}"):
            raise RekeyError(f"node {node_id}: batch order or level mismatch")


def rekey_fixture(source: dict[str, Any]) -> dict[str, Any]:
    result = copy.deepcopy(source)
    set_schema_version(result)
    repair_component_roots(result)

    points = result.get("embedded_input_points")
    if not isinstance(points, list):
        raise RekeyError("embedded_input_points must be an array")
    input_projection = []
    for index, point in enumerate(points):
        if not isinstance(point, dict):
            raise RekeyError(f"embedded_input_points[{index}]: expected an object")
        input_projection.append(
            {
                "point_id": point["point_id"],
                "source_indices": point["source_indices"],
                "multiplicity": point["multiplicity"],
                "coordinate_bits": point["coordinate_bits"],
            }
        )
    input_digest = canonical_id("Input", input_projection)

    event_records = result.get("critical_catalog")
    old_events = index_records(event_records, "event_id", "critical_catalog")
    event_map: dict[str, str] = {}
    for old_id, event in old_events.items():
        projection = {
            "interior_ids": event["interior_ids"],
            "shell_ids": event["shell_ids"],
            "minimal_support_ids": event["minimal_support_ids"],
            "center_witness_homogeneous": event["center_witness_homogeneous"],
            "squared_level_exact": event["squared_level_exact"],
        }
        event_map[old_id] = canonical_id("CriticalEvent", projection)
    if len(set(event_map.values())) != len(event_map):
        raise RekeyError("critical-event identities collide")
    for old_id, event in old_events.items():
        event["event_id"] = event_map[old_id]
    event_records.sort(key=lambda record: record["event_id"])

    coface_records = result.get("gamma_cofaces")
    old_cofaces = index_records(coface_records, "coface_id", "gamma_cofaces")
    coface_map: dict[str, str] = {}
    for old_id, coface in old_cofaces.items():
        projection = {
            "order": coface["order"],
            "simplex_point_ids": coface["simplex_point_ids"],
            "facet_point_ids": coface["facet_point_ids"],
            "squared_level_exact": coface["squared_level_exact"],
        }
        coface_map[old_id] = canonical_id("GammaCoface", projection)
    if len(set(coface_map.values())) != len(coface_map):
        raise RekeyError("Gamma-coface identities collide")
    for old_id, coface in old_cofaces.items():
        coface["coface_id"] = coface_map[old_id]
    coface_records.sort(key=lambda record: record["coface_id"])

    hyperedge_records = result.get("gabriel_hyperedges")
    old_hyperedges = index_records(
        hyperedge_records, "hyperedge_id", "gabriel_hyperedges"
    )
    hyperedge_map: dict[str, str] = {}
    for old_id, hyperedge in old_hyperedges.items():
        hyperedge["event_id"] = remap_one(
            hyperedge.get("event_id"), event_map, f"hyperedge {old_id}.event_id"
        )
        projection = {
            "event_id": hyperedge["event_id"],
            "order": hyperedge["order"],
            "simplex_point_ids": hyperedge["simplex_point_ids"],
            "facet_point_ids": hyperedge["facet_point_ids"],
            "strict_arm_point_ids": hyperedge["strict_arm_point_ids"],
            "squared_level_exact": hyperedge["squared_level_exact"],
        }
        hyperedge_map[old_id] = canonical_id("GabrielHyperedge", projection)
    if len(set(hyperedge_map.values())) != len(hyperedge_map):
        raise RekeyError("Gabriel-hyperedge identities collide")
    for old_id, hyperedge in old_hyperedges.items():
        hyperedge["hyperedge_id"] = hyperedge_map[old_id]
    hyperedge_records.sort(key=lambda record: record["hyperedge_id"])

    attachment_records = result.get("attachments")
    old_attachments = index_records(attachment_records, "attachment_id", "attachments")
    attachment_map: dict[str, str] = {}
    for old_id, attachment in old_attachments.items():
        attachment["event_id"] = remap_one(
            attachment.get("event_id"), event_map, f"attachment {old_id}.event_id"
        )
        projection = {
            "event_id": attachment["event_id"],
            "order": attachment["order"],
            "removed_shell_id": attachment["removed_shell_id"],
        }
        attachment_map[old_id] = canonical_id("Attachment", projection)
    if len(set(attachment_map.values())) != len(attachment_map):
        raise RekeyError("attachment identities collide")
    for old_id, attachment in old_attachments.items():
        attachment["attachment_id"] = attachment_map[old_id]
    attachment_records.sort(key=lambda record: record["attachment_id"])

    batch_records = result.get("equal_level_batches")
    old_batches = index_records(batch_records, "batch_id", "equal_level_batches")
    batch_map: dict[str, str] = {}
    for old_id, batch in old_batches.items():
        batch["event_ids"] = remap_many(
            batch.get("event_ids"), event_map, f"batch {old_id}.event_ids"
        )
        batch["gamma_coface_ids"] = remap_many(
            batch.get("gamma_coface_ids"),
            coface_map,
            f"batch {old_id}.gamma_coface_ids",
        )
        batch["hyperedge_ids"] = remap_many(
            batch.get("hyperedge_ids"), hyperedge_map, f"batch {old_id}.hyperedge_ids"
        )
        batch["attachment_ids"] = remap_many(
            batch.get("attachment_ids"),
            attachment_map,
            f"batch {old_id}.attachment_ids",
        )
        projection = {
            "order": batch["order"],
            "squared_level_exact": batch["squared_level_exact"],
            "event_ids": batch["event_ids"],
            "gamma_coface_ids": batch["gamma_coface_ids"],
            "hyperedge_ids": batch["hyperedge_ids"],
            "attachment_ids": batch["attachment_ids"],
        }
        batch_map[old_id] = canonical_id("EqualLevelBatch", projection)
    if len(set(batch_map.values())) != len(batch_map):
        raise RekeyError("equal-level batch identities collide")
    for old_id, batch in old_batches.items():
        batch["batch_id"] = batch_map[old_id]

    forests = result.get("forests")
    if not isinstance(forests, list):
        raise RekeyError("forests must be an array")
    old_nodes: dict[str, dict[str, Any]] = {}
    node_order: dict[str, int] = {}
    for forest_index, forest in enumerate(forests):
        indexed = index_records(
            forest.get("nodes"), "node_id", f"forests[{forest_index}].nodes"
        )
        for old_id, node in indexed.items():
            if old_id in old_nodes:
                raise RekeyError(f"node {old_id} occurs in more than one forest")
            old_nodes[old_id] = node
            node_order[old_id] = forest["order"]

    node_map: dict[str, str] = {}
    visiting: set[str] = set()

    def rekey_node(old_id: str) -> str:
        if old_id in node_map:
            return node_map[old_id]
        if old_id in visiting:
            raise RekeyError(f"cycle in merge-node DAG at {old_id}")
        try:
            node = old_nodes[old_id]
        except KeyError as exc:
            raise RekeyError(f"missing merge node {old_id}") from exc
        visiting.add(old_id)
        children = [
            rekey_node(require_id(child, f"node {old_id}.child_ids"))
            for child in node.get("child_ids", [])
        ]
        if any(
            node_order[require_id(child, f"node {old_id}.child_ids")]
            != node_order[old_id]
            for child in node.get("child_ids", [])
        ):
            raise RekeyError(f"node {old_id}: child belongs to another order")
        events = remap_many(
            node.get("event_ids"), event_map, f"node {old_id}.event_ids"
        )
        old_batch = node.get("batch_id")
        batch_id = (
            None
            if old_batch is None
            else remap_one(old_batch, batch_map, f"node {old_id}.batch_id")
        )
        children = sorted(children)
        projection = {
            "order": node_order[old_id],
            "profile": result["profile"],
            "kind": node["kind"],
            "squared_level": node["squared_level"],
            "child_ids": children,
            "event_ids": events,
            "batch_id": batch_id,
        }
        new_id = canonical_id("MergeNode", projection)
        node_map[old_id] = new_id
        node["node_id"] = new_id
        node["child_ids"] = children
        node["event_ids"] = events
        node["batch_id"] = batch_id
        visiting.remove(old_id)
        return new_id

    for old_id in sorted(old_nodes):
        rekey_node(old_id)
    if len(set(node_map.values())) != len(node_map):
        raise RekeyError("merge-node identities collide")

    for attachment in attachment_records:
        attachment["target_node_id"] = remap_one(
            attachment.get("target_node_id"),
            node_map,
            f"attachment {attachment['attachment_id']}.target_node_id",
        )

    coverage_by_order: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for old_batch_id, batch in old_batches.items():
        batch["pre_lot_components"] = sorted(
            batch["pre_lot_components"],
            key=lambda component: remap_one(
                component.get("component_id"),
                node_map,
                f"batch {old_batch_id}.pre.component_id",
            ),
        )
        batch["post_lot_components"] = sorted(
            batch["post_lot_components"],
            key=lambda component: remap_one(
                component.get("component_id"),
                node_map,
                f"batch {old_batch_id}.post.component_id",
            ),
        )
        for components_name in ("pre_lot_components", "post_lot_components"):
            for component in batch[components_name]:
                component["component_id"] = remap_one(
                    component["component_id"],
                    node_map,
                    f"batch {old_batch_id}.{components_name}.component_id",
                )
            batch[components_name].sort(key=lambda component: component["component_id"])
        deltas = batch.get("coverage_deltas")
        if not isinstance(deltas, list):
            raise RekeyError(f"batch {old_batch_id}.coverage_deltas must be an array")
        for delta in deltas:
            delta["batch_id"] = batch_map[old_batch_id]
            delta["root_node_id"] = remap_one(
                delta.get("root_node_id"),
                node_map,
                f"batch {old_batch_id}.coverage_delta.root_node_id",
            )
        deltas.sort(key=lambda delta: delta["root_node_id"])
        coverage_by_order[batch["order"]].extend(copy.deepcopy(deltas))
    batch_records.sort(key=lambda record: record["batch_id"])

    forest_map: dict[str, str] = {}
    for forest_index, forest in enumerate(forests):
        old_forest_id = require_id(
            forest.get("forest_id"), f"forests[{forest_index}].forest_id"
        )
        forest["nodes"].sort(key=lambda node: node["node_id"])
        forest["root_ids"] = remap_many(
            forest.get("root_ids"), node_map, f"forest {old_forest_id}.root_ids"
        )
        forest["coverage_log"] = sorted(
            coverage_by_order[forest["order"]], key=canonical_json
        )
        projection = {
            "order": forest["order"],
            "profile": result["profile"],
            "forest_semantics": result["forest_semantics"],
            "node_ids": [node["node_id"] for node in forest["nodes"]],
            "root_ids": forest["root_ids"],
            "coverage_log": forest["coverage_log"],
        }
        forest_map[old_forest_id] = canonical_id("MergeForest", projection)
        forest["forest_id"] = forest_map[old_forest_id]
    if len(set(forest_map.values())) != len(forest_map):
        raise RekeyError("merge-forest identities collide")
    forests.sort(key=lambda forest: forest["order"])

    vertical_records = result.get("vertical_maps")
    old_verticals = index_records(vertical_records, "map_id", "vertical_maps")
    vertical_map: dict[str, str] = {}
    for old_id, vertical in old_verticals.items():
        assignments = vertical.get("assignments")
        if not isinstance(assignments, list):
            raise RekeyError(f"vertical map {old_id}.assignments must be an array")
        for assignment_index, assignment in enumerate(assignments):
            assignment["source_node_id"] = remap_one(
                assignment.get("source_node_id"),
                node_map,
                f"vertical {old_id}.assignments[{assignment_index}].source",
            )
            assignment["target_node_id"] = remap_one(
                assignment.get("target_node_id"),
                node_map,
                f"vertical {old_id}.assignments[{assignment_index}].target",
            )
            assignment["proof_reference_ids"] = remap_many(
                assignment.get("proof_reference_ids"),
                event_map,
                f"vertical {old_id}.assignments[{assignment_index}].proofs",
            )
        assignments.sort(key=lambda assignment: assignment["source_node_id"])
        projection = {
            "source_order": vertical["source_order"],
            "target_order": vertical["target_order"],
            "assignments": assignments,
        }
        vertical_map[old_id] = canonical_id("VerticalMap", projection)
        vertical["map_id"] = vertical_map[old_id]
    if len(set(vertical_map.values())) != len(vertical_map):
        raise RekeyError("vertical-map identities collide")
    vertical_records.sort(key=lambda vertical: vertical["source_order"])

    partial_scope = result.get("partial_scope")
    if partial_scope is not None:
        if not isinstance(partial_scope, dict):
            raise RekeyError("partial_scope must be an object")
        partial_scope["verified_event_ids"] = remap_many(
            partial_scope.get("verified_event_ids"),
            event_map,
            "partial_scope.verified_event_ids",
        )
        if partial_scope.get("canonical_cell_certificates") or partial_scope.get(
            "unresolved_loci"
        ):
            raise RekeyError(
                "this five-fixture migrator refuses non-empty cells or unresolved loci"
            )

    materialized = result.get("materialized_point_sets")
    if not isinstance(materialized, list):
        raise RekeyError("materialized_point_sets must be an array")
    for index, point_set in enumerate(materialized):
        point_set["node_id"] = remap_one(
            point_set.get("node_id"),
            node_map,
            f"materialized_point_sets[{index}].node_id",
        )
    materialized.sort(key=canonical_json)
    result["coverage_log"] = sorted(
        (
            copy.deepcopy(delta)
            for records in coverage_by_order.values()
            for delta in records
        ),
        key=canonical_json,
    )

    certificate = result.get("run_certificate")
    if not isinstance(certificate, dict):
        raise RekeyError("run_certificate must be an object")
    input_semantics = certificate.get("input_semantics")
    if not isinstance(input_semantics, dict):
        raise RekeyError("run_certificate.input_semantics must be an object")
    input_semantics["input_sha256"] = input_digest
    run_projection = {
        "input_sha256": input_digest,
        "k_max": certificate["k_max"],
        "profile": result["profile"],
        "backend": result["backend"],
        "mode": result["mode"],
    }
    run_id = canonical_id("Run", run_projection)
    result["run_id"] = run_id
    certificate["run_id"] = run_id

    snapshot = certificate.get("final_budget_snapshot")
    if not isinstance(snapshot, dict):
        raise RekeyError("final_budget_snapshot must be an object")
    snapshot_without_id = {
        key: value for key, value in snapshot.items() if key != "snapshot_id"
    }
    snapshot["snapshot_id"] = canonical_id("BudgetSnapshot", snapshot_without_id)

    environment = certificate.get("software_environment")
    if not isinstance(environment, dict):
        raise RekeyError("software_environment must be an object")
    compiler = environment.get("compiler")
    if environment.get("hardware") != "reference-cpu" or compiler not in {
        "python",
        "contract-fixture",
    }:
        raise RekeyError("unknown build-options projection for this environment")
    implementation = (
        "independent-reference-oracle" if compiler == "python" else "contract-fixture"
    )
    environment["build_options_sha256"] = canonical_id(
        "BuildOptions", {"implementation": implementation}
    )
    if certificate.get("unresolved_locus_ids"):
        raise RekeyError("this five-fixture migrator refuses unresolved_locus_ids")
    certificate_without_id = {
        key: value for key, value in certificate.items() if key != "certificate_id"
    }
    certificate["certificate_id"] = canonical_id(
        "RunCertificate", certificate_without_id
    )

    scientific_projection = {
        "input_sha256": input_digest,
        "profile": result["profile"],
        "forest_semantics": result["forest_semantics"],
        "event_ids": [record["event_id"] for record in event_records],
        "hyperedge_ids": [record["hyperedge_id"] for record in hyperedge_records],
        "gamma_coface_ids": [record["coface_id"] for record in coface_records],
        "attachment_ids": [record["attachment_id"] for record in attachment_records],
        "batch_ids": [record["batch_id"] for record in batch_records],
        "forest_ids": [record["forest_id"] for record in forests],
        "vertical_map_ids": [record["map_id"] for record in vertical_records],
    }
    result["result_id"] = canonical_id("MorseHGP3DResult", scientific_projection)
    return result


def validate_fixed_point(value: dict[str, Any], path: Path) -> None:
    repeated = rekey_fixture(value)
    if repeated != value:
        raise RekeyError("migration is not idempotent")

    # Schema and cross-field checks are independent from the reference serializer.
    try:
        from check_contracts import (  # type: ignore[import-not-found]
            SchemaValidator,
            load_json as load_contract_json,
            validate_result_semantics,
            validate_round_trip,
        )

        schema = load_contract_json(SCHEMA_PATH)
        SchemaValidator(schema).check(value, path=f"fixture:{path.name}")
        validate_result_semantics(value)
        validate_round_trip(value, str(path.relative_to(ROOT)))
    except RekeyError:
        raise
    except Exception as exc:
        raise RekeyError(f"post-migration contract validation failed: {exc}") from exc


def difference_count(left: Any, right: Any) -> int:
    if type(left) is not type(right):
        return 1
    if isinstance(left, dict):
        keys = set(left) | set(right)
        return sum(
            (
                1
                if key not in left or key not in right
                else difference_count(left[key], right[key])
            )
            for key in keys
        )
    if isinstance(left, list):
        if len(left) != len(right):
            return 1 + sum(difference_count(a, b) for a, b in zip(left, right))
        return sum(difference_count(a, b) for a, b in zip(left, right))
    return int(left != right)


def atomic_write(path: Path, value: dict[str, Any]) -> None:
    encoded = (
        json.dumps(value, ensure_ascii=False, allow_nan=False, sort_keys=True, indent=2)
        + "\n"
    )
    mode = path.stat().st_mode & 0o777
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as stream:
            temporary_name = stream.name
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary_name, mode)
        os.replace(temporary_name, path)
        directory_fd = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    finally:
        if temporary_name is not None and os.path.exists(temporary_name):
            os.unlink(temporary_name)


def fixture_paths(arguments: Sequence[str]) -> list[Path]:
    if not arguments:
        paths = sorted(FIXTURE_DIR.glob("*.json"))
        if len(paths) != 5:
            raise RekeyError(
                f"expected exactly five contract fixtures, found {len(paths)}"
            )
        return paths
    paths = []
    for raw in arguments:
        path = Path(raw)
        if not path.is_absolute():
            path = (ROOT / path).resolve()
        if path.suffix != ".json" or not path.is_file():
            raise RekeyError(f"not a readable JSON fixture: {raw}")
        try:
            path.relative_to(FIXTURE_DIR.resolve())
        except ValueError as exc:
            raise RekeyError(
                f"fixture escapes {FIXTURE_DIR.relative_to(ROOT)}: {raw}"
            ) from exc
        paths.append(path)
    if len(paths) != len(set(paths)):
        raise RekeyError("a fixture path was provided more than once")
    return sorted(paths)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="verify only (default)")
    mode.add_argument(
        "--write", action="store_true", help="atomically replace stale fixtures"
    )
    parser.add_argument(
        "fixtures", nargs="*", help="optional paths below tests/fixtures/contracts"
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        paths = fixture_paths(args.fixtures)
    except RekeyError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    failures = 0
    changed = 0
    for path in paths:
        label = path.relative_to(ROOT)
        try:
            source = load_json(path)
            migrated = rekey_fixture(source)
            validate_fixed_point(migrated, path)
            differences = difference_count(source, migrated)
            if differences == 0:
                print(f"OK      {label}: canonical MorseHGP3D/v2 fixed point")
                continue
            changed += 1
            if args.write:
                atomic_write(path, migrated)
                print(f"UPDATED {label}: {differences} scalar/reference changes")
            else:
                failures += 1
                print(
                    f"STALE   {label}: {differences} scalar/reference changes required "
                    "(use --write)",
                    file=sys.stderr,
                )
        except (KeyError, TypeError, ValueError, RekeyError) as exc:
            failures += 1
            print(f"ERROR   {label}: {exc}", file=sys.stderr)

    if failures:
        print(
            f"Rekey check failed: {failures}/{len(paths)} fixture(s); no file was written.",
            file=sys.stderr,
        )
        return 1
    action = "updated" if args.write else "validated"
    print(f"Rekey {action}: {len(paths)} fixture(s), {changed} changed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
