from __future__ import annotations

import json
import struct
import unittest
from fractions import Fraction
from pathlib import Path

from reference.morsehgp3d_oracle.oracle import run_oracle
from reference.morsehgp3d_oracle.serialization import serialize_oracle_result
from tests.oracle.generators import affine_dimension, generate_affine_cloud
from tools.check_contracts import (
    SchemaValidator,
    validate_result_semantics,
    validate_round_trip,
)


ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests" / "fixtures" / "contracts"
VERTICAL_Q1_REGRESSION = (
    ROOT / "tests" / "fixtures" / "regressions" / "vertical_q1_growth_target.json"
)
SCHEMA = json.loads(
    (ROOT / "schemas" / "morsehgp3d-contract-v2.schema.json").read_text(
        encoding="utf-8"
    )
)


def _fraction(record: dict[str, object]) -> Fraction:
    return Fraction(int(record["numerator"]), int(record["denominator"]))


def _decode_points(contract: dict[str, object]) -> tuple[tuple[float, float, float], ...]:
    return tuple(
        tuple(
            struct.unpack(">d", bytes.fromhex(bits))[0]
            for bits in point["coordinate_bits"]
        )
        for point in contract["embedded_input_points"]
    )


def _event_projection(event: dict[str, object]) -> tuple[object, ...]:
    return (
        event["closed_rank"],
        tuple(event["interior_ids"]),
        tuple(event["shell_ids"]),
        tuple(event["minimal_support_ids"]),
        _fraction(event["squared_level_exact"]),
        event["birth_order"],
        event["saddle_order"],
    )


def _hyperedge_projection(edge: dict[str, object]) -> tuple[object, ...]:
    return (
        edge["order"],
        tuple(edge["simplex_point_ids"]),
        tuple(tuple(facet) for facet in edge["facet_point_ids"]),
        tuple(tuple(arm) for arm in edge["strict_arm_point_ids"]),
        _fraction(edge["squared_level_exact"]),
    )


def _gamma_coface_projection(coface: dict[str, object]) -> tuple[object, ...]:
    return (
        coface["order"],
        tuple(coface["simplex_point_ids"]),
        tuple(tuple(facet) for facet in coface["facet_point_ids"]),
        _fraction(coface["squared_level_exact"]),
    )


def _node_graph_projections(
    contract: dict[str, object],
) -> dict[str, tuple[object, ...]]:
    nodes = {
        node["node_id"]: (forest["order"], node)
        for forest in contract["forests"]
        for node in forest["nodes"]
    }
    events = {
        event["event_id"]: _event_projection(event)
        for event in contract["critical_catalog"]
    }
    batches = {
        batch["batch_id"]: (
            batch["order"],
            _fraction(batch["squared_level_exact"]),
        )
        for batch in contract["equal_level_batches"]
    }
    cache: dict[str, tuple[object, ...]] = {}

    def visit(node_id: str) -> tuple[object, ...]:
        if node_id in cache:
            return cache[node_id]
        order, node = nodes[node_id]
        batch_projection = batches.get(node["batch_id"])
        if order == 1 and node["kind"] == "birth":
            # A rank-one birth may optionally cite its level-zero batch; both
            # forms encode the same scientific birth and are schema-valid.
            batch_projection = None
        projection = (
            order,
            node["kind"],
            _fraction(node["squared_level"]),
            tuple(node["covered_point_ids"]),
            tuple(sorted((visit(child) for child in node["child_ids"]), key=repr)),
            tuple(
                sorted(
                    (events[event_id] for event_id in node["event_ids"]),
                    key=repr,
                )
            ),
            batch_projection,
        )
        cache[node_id] = projection
        return projection

    return {node_id: visit(node_id) for node_id in nodes}


def _attachment_projection(
    attachment: dict[str, object],
    *,
    events: dict[str, tuple[object, ...]],
    nodes: dict[str, tuple[object, ...]],
) -> tuple[object, ...]:
    return (
        events[attachment["event_id"]],
        attachment["order"],
        attachment["removed_shell_id"],
        tuple(attachment["arm_point_ids"]),
        _fraction(attachment["event_squared_level"]),
        nodes[attachment["target_node_id"]],
        tuple(
            (
                step["step_index"],
                tuple(step["from_point_ids"]),
                tuple(step["to_point_ids"]),
                tuple(step["shared_facet_point_ids"]),
                _fraction(step["from_squared_level"]),
                _fraction(step["to_squared_level"]),
            )
            for step in attachment["descent_path"]
        ),
    )


def _batch_projection(
    contract: dict[str, object], batch: dict[str, object]
) -> tuple[object, ...]:
    def states(name: str) -> tuple[object, ...]:
        return tuple(
            sorted(
                (
                    tuple(tuple(facet) for facet in component["active_facet_point_ids"]),
                    tuple(component["covered_point_ids"]),
                )
                for component in batch[name]
            )
        )

    events = {
        event["event_id"]: _event_projection(event)
        for event in contract["critical_catalog"]
    }
    hyperedges = {
        edge["hyperedge_id"]: _hyperedge_projection(edge)
        for edge in contract["gabriel_hyperedges"]
    }
    cofaces = {
        coface["coface_id"]: _gamma_coface_projection(coface)
        for coface in contract["gamma_cofaces"]
    }
    nodes = _node_graph_projections(contract)
    attachments = {
        attachment["attachment_id"]: _attachment_projection(
            attachment, events=events, nodes=nodes
        )
        for attachment in contract["attachments"]
    }
    return (
        batch["order"],
        _fraction(batch["squared_level_exact"]),
        tuple(sorted((events[item] for item in batch["event_ids"]), key=repr)),
        tuple(
            sorted((cofaces[item] for item in batch["gamma_coface_ids"]), key=repr)
        ),
        tuple(
            sorted((hyperedges[item] for item in batch["hyperedge_ids"]), key=repr)
        ),
        tuple(
            sorted((attachments[item] for item in batch["attachment_ids"]), key=repr)
        ),
        states("pre_lot_components"),
        states("post_lot_components"),
        tuple(
            sorted(
                (
                    (
                        nodes[delta["root_node_id"]],
                        tuple(
                            tuple(facet)
                            for facet in delta["added_facet_point_ids"]
                        ),
                        tuple(delta["added_point_ids"]),
                    )
                    for delta in batch["coverage_deltas"]
                ),
                key=repr,
            )
        ),
    )


def _vertical_projection(contract: dict[str, object]) -> set[tuple[object, ...]]:
    nodes = _node_graph_projections(contract)
    events = {
        event["event_id"]: _event_projection(event)
        for event in contract["critical_catalog"]
    }


def _attachment_projections(
    contract: dict[str, object],
) -> tuple[tuple[object, ...], ...]:
    events = {
        event["event_id"]: _event_projection(event)
        for event in contract["critical_catalog"]
    }
    nodes = _node_graph_projections(contract)
    return tuple(
        sorted(
            (
                _attachment_projection(
                    attachment,
                    events=events,
                    nodes=nodes,
                )
                for attachment in contract["attachments"]
            ),
            key=repr,
        )
    )


def _forest_topology_projection(
    contract: dict[str, object],
) -> tuple[tuple[object, ...], ...]:
    nodes = _node_graph_projections(contract)
    return tuple(
        (
            forest["order"],
            forest["complete"],
            tuple(sorted((nodes[node["node_id"]] for node in forest["nodes"]), key=repr)),
            tuple(sorted((nodes[node_id] for node_id in forest["root_ids"]), key=repr)),
        )
        for forest in contract["forests"]
    )
    return {
        (
            mapping["source_order"],
            nodes[assignment["source_node_id"]],
            nodes[assignment["target_node_id"]],
            _fraction(assignment["at_squared_level"]),
            tuple(
                sorted(
                    (events[item] for item in assignment["proof_reference_ids"]),
                    key=repr,
                )
            ),
            mapping["complete"],
            mapping["naturality_squares_checked"],
            mapping["naturality_failures"],
        )
        for mapping in contract["vertical_maps"]
        for assignment in mapping["assignments"]
    }


class FrozenContractIntegrationTests(unittest.TestCase):
    def test_vertical_target_coverage_uses_closed_q1_replay_state(self) -> None:
        fixture = json.loads(VERTICAL_Q1_REGRESSION.read_text(encoding="utf-8"))
        witness = fixture["vertical_witness"]
        result = run_oracle(
            fixture["points"],
            fixture["k_max"],
            profile=fixture["profile"],
        )
        contract = serialize_oracle_result(result)
        SchemaValidator(SCHEMA).check(contract)
        validate_result_semantics(contract)

        nodes = {
            node["node_id"]: node
            for forest in contract["forests"]
            for node in forest["nodes"]
        }
        mapping = next(
            item
            for item in contract["vertical_maps"]
            if item["source_order"] == witness["source_order"]
            and item["target_order"] == witness["target_order"]
        )
        level = Fraction(
            witness["at_squared_level"]["numerator"],
            witness["at_squared_level"]["denominator"],
        )
        assignment = next(
            item
            for item in mapping["assignments"]
            if _fraction(item["at_squared_level"]) == level
            and nodes[item["source_node_id"]]["covered_point_ids"]
            == witness["source_covered_point_ids"]
        )
        target = nodes[assignment["target_node_id"]]
        self.assertEqual(
            _fraction(target["squared_level"]),
            Fraction(
                witness["target_birth_squared_level"]["numerator"],
                witness["target_birth_squared_level"]["denominator"],
            ),
        )
        self.assertEqual(
            target["covered_point_ids"],
            witness["target_birth_covered_point_ids"],
        )

        latest_target_batch = max(
            (
                batch
                for batch in contract["equal_level_batches"]
                if batch["order"] == witness["target_order"]
                and _fraction(batch["squared_level_exact"]) <= level
            ),
            key=lambda batch: _fraction(batch["squared_level_exact"]),
        )
        replayed = next(
            component
            for component in latest_target_batch["post_lot_components"]
            if component["component_id"] == assignment["target_node_id"]
        )
        self.assertEqual(
            replayed["covered_point_ids"],
            witness["target_replayed_covered_point_ids"],
        )

    def test_reference_certificate_reports_closed_exhaustive_counts(self) -> None:
        result = run_oracle(
            [(0, 0, 0), (4, 0, 0), (1, 1, 0)],
            2,
            profile="hgp_reduced",
        )
        contract = serialize_oracle_result(result)
        certificate = contract["run_certificate"]
        counters = certificate["work_and_memory_counters"]
        predicates = certificate["exact_predicate_counts"]

        self.assertEqual(contract["result_kind"], "hierarchy")
        self.assertEqual(contract["forest_semantics"], "exact")
        self.assertNotIn("partial_scope", contract)
        self.assertEqual(certificate["public_status"], "exact")
        self.assertEqual(certificate["proof_basis"], "gamma_exhaustive_reference")
        self.assertTrue(certificate["require_exact"])
        self.assertTrue(all(certificate["gamma_complete_by_order"]))
        self.assertEqual(certificate["partial_guarantees"], ["none"])
        self.assertEqual(counters["candidate_events"], 7)
        self.assertEqual(counters["accepted_events"], 6)
        self.assertEqual(counters["rejected_events"], 1)
        self.assertEqual(
            counters["candidate_events"],
            counters["accepted_events"] + counters["rejected_events"],
        )
        self.assertGreater(predicates["cpu_multiprecision_certified"], 7)

    def test_exhaustive_support_path_closes_without_cell_cascade(self) -> None:
        result = run_oracle(
            [(0, 0, 0), (4, 0, 0), (1, 1, 0)],
            2,
            profile="hgp_reduced",
        )
        metrics = result.metadata["reference_metrics"]
        self.assertEqual(metrics["catalog_support_candidates"], 7)
        self.assertEqual(metrics["catalog_support_universe_complete"], 1)
        self.assertEqual(metrics["canonical_cells_required"], 0)
        self.assertEqual(metrics["canonical_cells_closed"], 0)
        self.assertEqual(metrics["active_cross_incidences_required"], 0)
        self.assertEqual(metrics["active_cross_incidences_closed"], 0)

        certificate = serialize_oracle_result(result)["run_certificate"]
        self.assertTrue(certificate["canonical_children_complete"])
        self.assertTrue(certificate["active_cross_incidences_complete"])
        self.assertEqual(
            certificate["work_and_memory_counters"]["canonical_cells_closed"],
            0,
        )

    def test_five_frozen_fixtures_are_reconstructed_and_schema_valid(self) -> None:
        validator = SchemaValidator(SCHEMA)
        paths = sorted(FIXTURES.glob("*.json"))
        self.assertEqual(len(paths), 5)

        for path in paths:
            expected = json.loads(path.read_text(encoding="utf-8"))
            certificate = expected["run_certificate"]
            with self.subTest(fixture=path.name):
                result = run_oracle(
                    _decode_points(expected),
                    certificate["k_max"],
                    profile=expected["profile"],
                )
                actual = serialize_oracle_result(result)

                validator.check(actual)
                validate_result_semantics(actual)
                validate_round_trip(actual, path.name)
                self.assertEqual(
                    {_event_projection(event) for event in actual["critical_catalog"]},
                    {_event_projection(event) for event in expected["critical_catalog"]},
                )
                self.assertEqual(
                    {
                        _hyperedge_projection(edge)
                        for edge in actual["gabriel_hyperedges"]
                    },
                    {
                        _hyperedge_projection(edge)
                        for edge in expected["gabriel_hyperedges"]
                    },
                )
                self.assertEqual(
                    {
                        _gamma_coface_projection(coface)
                        for coface in actual["gamma_cofaces"]
                    },
                    {
                        _gamma_coface_projection(coface)
                        for coface in expected["gamma_cofaces"]
                    },
                )
                self.assertEqual(
                    {
                        _batch_projection(actual, batch)
                        for batch in actual["equal_level_batches"]
                    },
                    {
                        _batch_projection(expected, batch)
                        for batch in expected["equal_level_batches"]
                    },
                )
                if expected["profile"] == "hgp_reduced":
                    self.assertEqual(
                        actual["run_certificate"]["proof_basis"],
                        "gamma_exhaustive_reference",
                    )
                    self.assertEqual(actual["result_kind"], "hierarchy")
                    self.assertEqual(actual["forest_semantics"], "exact")
                    self.assertNotIn("partial_scope", actual)
                    self.assertEqual(
                        actual["run_certificate"]["public_status"], "exact"
                    )
                    self.assertTrue(actual["run_certificate"]["require_exact"])
                    self.assertTrue(
                        all(
                            actual["run_certificate"]["gamma_complete_by_order"]
                        )
                    )
                    self.assertEqual(
                        actual["run_certificate"]["partial_guarantees"], ["none"]
                    )
                else:
                    self.assertEqual(actual["result_kind"], "partial_hierarchy")
                    self.assertEqual(actual["forest_semantics"], "partial_refinement")
                    self.assertIn("partial_scope", actual)
                    self.assertEqual(
                        actual["run_certificate"]["proof_basis"],
                        "m1_conditional_contract",
                    )
                    self.assertEqual(
                        actual["run_certificate"]["public_status"], "conditional"
                    )
                    self.assertFalse(actual["run_certificate"]["require_exact"])
                self.assertEqual(
                    _forest_topology_projection(actual),
                    _forest_topology_projection(expected),
                )
                self.assertEqual(
                    _vertical_projection(actual), _vertical_projection(expected)
                )
                self.assertTrue(
                    all(
                        assignment["method"] == "reference_oracle"
                        for mapping in actual["vertical_maps"]
                        for assignment in mapping["assignments"]
                    )
                )
                self.assertEqual(
                    _attachment_projections(actual),
                    _attachment_projections(expected),
                )

    def test_deterministic_affine_clouds_run_in_dimensions_one_two_and_three(self) -> None:
        validator = SchemaValidator(SCHEMA)
        for dimension in (1, 2, 3):
            points = generate_affine_cloud(
                dimension,
                dimension + 4,
                seed=1009 + dimension,
                coordinate_bound=11,
            )
            self.assertEqual(affine_dimension(points), dimension)
            for profile in ("hgp_reduced", "full_pi0"):
                with self.subTest(dimension=dimension, profile=profile):
                    result = run_oracle(points, 2, profile=profile)
                    repeated = run_oracle(points, 2, profile=profile)
                    self.assertEqual(result.critical_catalog, repeated.critical_catalog)
                    self.assertEqual(result.orders, repeated.orders)
                    self.assertEqual(tuple(order.order for order in result.orders), (1, 2))
                    self.assertTrue(
                        all(mapping.is_natural for mapping in result.vertical_maps)
                    )
                    contract = serialize_oracle_result(result)
                    validator.check(contract)
                    validate_result_semantics(contract)
                    certificate = contract["run_certificate"]
                    if profile == "hgp_reduced":
                        self.assertEqual(certificate["public_status"], "exact")
                        self.assertEqual(
                            certificate["proof_basis"],
                            "gamma_exhaustive_reference",
                        )
                        self.assertNotIn("partial_scope", contract)
                    else:
                        self.assertEqual(certificate["public_status"], "conditional")
                        self.assertEqual(
                            certificate["proof_basis"], "m1_conditional_contract"
                        )
                        self.assertIn("partial_scope", contract)
                    validate_round_trip(
                        contract, f"affine-d{dimension}-{profile}"
                    )


if __name__ == "__main__":
    unittest.main()
