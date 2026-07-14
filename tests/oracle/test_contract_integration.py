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
SCHEMA = json.loads(
    (ROOT / "schemas" / "morsehgp3d-contract-v1.schema.json").read_text(
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


def _node_projection(order: int, node: dict[str, object]) -> tuple[object, ...]:
    return (
        order,
        node["kind"],
        _fraction(node["squared_level"]),
        tuple(node["covered_point_ids"]),
        len(node["child_ids"]),
    )


def _batch_projection(batch: dict[str, object]) -> tuple[object, ...]:
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

    return (
        batch["order"],
        _fraction(batch["squared_level_exact"]),
        len(batch["event_ids"]),
        len(batch["hyperedge_ids"]),
        len(batch["attachment_ids"]),
        states("pre_lot_components"),
        states("post_lot_components"),
    )


def _vertical_projection(contract: dict[str, object]) -> set[tuple[object, ...]]:
    nodes = {
        node["node_id"]: _node_projection(forest["order"], node)
        for forest in contract["forests"]
        for node in forest["nodes"]
    }
    return {
        (
            mapping["source_order"],
            nodes[assignment["source_node_id"]],
            nodes[assignment["target_node_id"]],
            _fraction(assignment["at_squared_level"]),
        )
        for mapping in contract["vertical_maps"]
        for assignment in mapping["assignments"]
    }


class FrozenContractIntegrationTests(unittest.TestCase):
    def test_reference_certificate_reports_closed_exhaustive_counts(self) -> None:
        result = run_oracle(
            [(0, 0, 0), (4, 0, 0), (1, 1, 0)],
            2,
            profile="hgp_reduced",
        )
        contract = serialize_oracle_result(result)
        counters = contract["run_certificate"]["work_and_memory_counters"]
        predicates = contract["run_certificate"]["exact_predicate_counts"]

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
                    {_batch_projection(batch) for batch in actual["equal_level_batches"]},
                    {
                        _batch_projection(batch)
                        for batch in expected["equal_level_batches"]
                    },
                )
                self.assertEqual(
                    {
                        _node_projection(forest["order"], node)
                        for forest in actual["forests"]
                        for node in forest["nodes"]
                    },
                    {
                        _node_projection(forest["order"], node)
                        for forest in expected["forests"]
                        for node in forest["nodes"]
                    },
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
                    len(actual["attachments"]), len(expected["attachments"])
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
                    validate_round_trip(
                        contract, f"affine-d{dimension}-{profile}"
                    )


if __name__ == "__main__":
    unittest.main()
