from __future__ import annotations

import copy
import sys
import unittest
from fractions import Fraction
from pathlib import Path
from typing import Callable


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from check_contracts import (  # noqa: E402
    FIXTURE_DIR,
    SCHEMA_PATH,
    ContractError,
    canonical_contract_id,
    load_json,
    validate_result_semantics,
)


Mutation = Callable[[dict], None]


def level_value(record: dict) -> Fraction:
    level = record["squared_level"]
    return Fraction(int(level["numerator"]), int(level["denominator"]))


def exact_level_value(record: dict) -> Fraction:
    level = record["squared_level_exact"]
    return Fraction(int(level["numerator"]), int(level["denominator"]))


class ExactSemanticGraphMutationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.exact = load_json(FIXTURE_DIR / "overlap-k2.json")
        cls.minimal_exact = load_json(SCHEMA_PATH)["$defs"][
            "MorseHGP3DResult"
        ]["examples"][0]
        validate_result_semantics(cls.exact)
        validate_result_semantics(cls.minimal_exact)

    def assert_false_exact_rejected(self, mutation: Mutation) -> None:
        candidate = copy.deepcopy(self.exact)
        mutation(candidate)
        self.assertEqual(candidate["run_certificate"]["public_status"], "exact")
        with self.assertRaises(ContractError):
            validate_result_semantics(candidate, check_canonical_ids=False)

    def test_gamma_q1_coverage_regression_is_permanent(self) -> None:
        regression = load_json(
            ROOT / "tests/fixtures/regressions/gamma_q1_coverage_delta.json"
        )
        pre_facets = {
            tuple(facet)
            for facet in regression["pre_lot_component"][
                "active_facet_point_ids"
            ]
        }
        post_facets = {
            tuple(facet)
            for facet in regression["post_lot_component"][
                "active_facet_point_ids"
            ]
        }
        expected = regression["expected_coverage_delta"]
        self.assertEqual(
            sorted(post_facets - pre_facets),
            [tuple(facet) for facet in expected["added_facet_point_ids"]],
        )
        self.assertEqual(expected["added_point_ids"], [])

        source = load_json(ROOT / regression["source_fixture"])
        level = regression["squared_level"]
        batch = next(
            batch
            for batch in source["equal_level_batches"]
            if batch["order"] == regression["order"]
            and batch["squared_level_exact"]["numerator"]
            == str(level["numerator"])
            and batch["squared_level_exact"]["denominator"]
            == str(level["denominator"])
        )
        self.assertTrue(
            any(
                delta["added_facet_point_ids"]
                == expected["added_facet_point_ids"]
                and delta["added_point_ids"] == expected["added_point_ids"]
                for delta in batch["coverage_deltas"]
            )
        )

    def test_hyperedge_simplex_cardinality_is_semantic(self) -> None:
        def mutate(result: dict) -> None:
            result["gabriel_hyperedges"][0]["simplex_point_ids"].pop()

        self.assert_false_exact_rejected(mutate)

    def test_hyperedge_points_stay_inside_the_input_domain(self) -> None:
        def mutate(result: dict) -> None:
            result["gabriel_hyperedges"][0]["simplex_point_ids"][-1] = result[
                "run_certificate"
            ]["input_point_count"]

        self.assert_false_exact_rejected(mutate)

    def test_hyperedge_event_must_be_its_geometric_event(self) -> None:
        def mutate(result: dict) -> None:
            hyperedge = result["gabriel_hyperedges"][0]
            hyperedge["event_id"] = next(
                event["event_id"]
                for event in result["critical_catalog"]
                if event["event_id"] != hyperedge["event_id"]
            )

        self.assert_false_exact_rejected(mutate)

    def test_complete_vertical_map_cannot_be_empty(self) -> None:
        def mutate(result: dict) -> None:
            vertical_map = result["vertical_maps"][0]
            self.assertTrue(vertical_map["complete"])
            vertical_map["assignments"].clear()

        self.assert_false_exact_rejected(mutate)

    def test_vertical_map_has_one_assignment_per_source(self) -> None:
        def mutate(result: dict) -> None:
            assignments = result["vertical_maps"][0]["assignments"]
            self.assertGreaterEqual(len(assignments), 2)
            assignments[1]["source_node_id"] = assignments[0]["source_node_id"]

        self.assert_false_exact_rejected(mutate)

    def test_vertical_assignment_level_cannot_precede_its_source(self) -> None:
        def mutate(result: dict) -> None:
            level = result["vertical_maps"][0]["assignments"][0][
                "at_squared_level"
            ]
            level["numerator"] = "0"
            level["denominator"] = "1"

        self.assert_false_exact_rejected(mutate)

    def test_merge_node_coverage_is_the_union_of_its_children(self) -> None:
        def mutate(result: dict) -> None:
            forest = next(forest for forest in result["forests"] if forest["order"] == 1)
            node = next(node for node in forest["nodes"] if node["kind"] == "merge")
            self.assertGreaterEqual(len(node["covered_point_ids"]), 2)
            node["covered_point_ids"].pop()

        self.assert_false_exact_rejected(mutate)

    def test_merge_node_events_match_its_order_and_level(self) -> None:
        def mutate(result: dict) -> None:
            forest = next(forest for forest in result["forests"] if forest["order"] == 1)
            node = next(node for node in forest["nodes"] if node["kind"] == "merge")
            wrong_event = next(
                event
                for event in result["critical_catalog"]
                if event["squared_level_exact"] != node["squared_level"]
            )
            node["event_ids"] = [wrong_event["event_id"]]

        self.assert_false_exact_rejected(mutate)

    def test_merge_node_requires_its_equal_level_batch(self) -> None:
        def mutate(result: dict) -> None:
            forest = next(forest for forest in result["forests"] if forest["order"] == 1)
            node = next(node for node in forest["nodes"] if node["kind"] == "merge")
            self.assertIsNotNone(node["batch_id"])
            node["batch_id"] = None

        self.assert_false_exact_rejected(mutate)

    def test_forest_node_cannot_have_two_parents(self) -> None:
        def mutate(result: dict) -> None:
            forest = next(forest for forest in result["forests"] if forest["order"] == 1)
            merges = sorted(
                (node for node in forest["nodes"] if node["kind"] == "merge"),
                key=level_value,
            )
            lower = next(node for node in merges if node["child_ids"])
            upper = next(
                node
                for node in merges
                if lower["node_id"] in node["child_ids"]
            )
            shared_child = lower["child_ids"][0]
            self.assertNotIn(shared_child, upper["child_ids"])
            upper["child_ids"] = sorted([*upper["child_ids"], shared_child])

        self.assert_false_exact_rejected(mutate)

    def test_batch_component_snapshot_matches_its_forest_node(self) -> None:
        def mutate(result: dict) -> None:
            batch = next(
                batch for batch in result["equal_level_batches"] if batch["pre_lot_components"]
            )
            component = batch["pre_lot_components"][0]
            node = next(
                node
                for forest in result["forests"]
                for node in forest["nodes"]
                if node["node_id"] == component["component_id"]
            )
            foreign_point = next(
                point_id
                for point_id in range(result["run_certificate"]["input_point_count"])
                if point_id not in node["covered_point_ids"]
            )
            component["covered_point_ids"] = [foreign_point]

        self.assert_false_exact_rejected(mutate)

    def test_batch_snapshot_facets_belong_to_the_component(self) -> None:
        def mutate(result: dict) -> None:
            batch = next(
                batch for batch in result["equal_level_batches"] if batch["pre_lot_components"]
            )
            component = batch["pre_lot_components"][0]
            foreign_point = next(
                point_id
                for point_id in range(result["run_certificate"]["input_point_count"])
                if point_id not in component["covered_point_ids"]
            )
            component["active_facet_point_ids"][0] = [foreign_point] * batch["order"]

        self.assert_false_exact_rejected(mutate)

    def test_batch_and_forest_coverage_logs_must_agree(self) -> None:
        def mutate(result: dict) -> None:
            batch = next(
                batch for batch in result["equal_level_batches"] if batch["coverage_deltas"]
            )
            delta = next(
                delta for delta in batch["coverage_deltas"] if delta["added_point_ids"]
            )
            delta["added_point_ids"] = []

        self.assert_false_exact_rejected(mutate)

    def test_budget_snapshot_remaining_arithmetic_is_closed(self) -> None:
        def mutate(result: dict) -> None:
            certificate = result["run_certificate"]
            certificate["budget_policy"]["device_budget_bytes"] = 10
            snapshot = certificate["final_budget_snapshot"]
            snapshot["device_limit_bytes"] = 10
            snapshot["device_used_bytes"] = 3
            snapshot["device_reserved_bytes"] = 2
            snapshot["device_remaining_bytes"] = 6

        self.assert_false_exact_rejected(mutate)

    def test_batch_pre_lot_equals_the_previous_post_lot(self) -> None:
        def mutate(result: dict) -> None:
            ordered = sorted(
                (
                    batch
                    for batch in result["equal_level_batches"]
                    if batch["order"] == 1
                ),
                key=exact_level_value,
            )
            previous, current = next(
                (previous, current)
                for previous, current in zip(ordered, ordered[1:])
                if len(current["pre_lot_components"]) > 1
            )
            self.assertEqual(
                previous["post_lot_components"], current["pre_lot_components"]
            )
            current["pre_lot_components"].pop()

        self.assert_false_exact_rejected(mutate)

    def test_every_gamma_facet_is_active_in_the_batch_post_lot(self) -> None:
        def mutate(result: dict) -> None:
            gamma_by_id = {
                coface["coface_id"]: coface for coface in result["gamma_cofaces"]
            }
            batch = max(
                (
                    candidate
                    for candidate in result["equal_level_batches"]
                    if candidate["order"] == 1
                    and candidate["gamma_coface_ids"]
                ),
                key=exact_level_value,
            )
            coface = gamma_by_id[batch["gamma_coface_ids"][0]]
            missing_facet = coface["facet_point_ids"][0]
            component = next(
                component
                for component in batch["post_lot_components"]
                if missing_facet in component["active_facet_point_ids"]
            )
            component["active_facet_point_ids"].remove(missing_facet)
            component["covered_point_ids"] = sorted(
                {
                    point_id
                    for facet in component["active_facet_point_ids"]
                    for point_id in facet
                }
            )

        self.assert_false_exact_rejected(mutate)

    def test_post_lot_cannot_merge_components_without_a_gamma_relation(self) -> None:
        def mutate(result: dict) -> None:
            gamma_by_id = {
                coface["coface_id"]: coface for coface in result["gamma_cofaces"]
            }
            batch = next(
                candidate
                for candidate in sorted(
                    (
                        candidate
                        for candidate in result["equal_level_batches"]
                        if candidate["order"] == 1
                    ),
                    key=exact_level_value,
                )
                if len(candidate["post_lot_components"]) > 1
                and candidate["gamma_coface_ids"]
            )
            relation_facets = [
                {
                    tuple(facet)
                    for facet in gamma_by_id[coface_id]["facet_point_ids"]
                }
                for coface_id in batch["gamma_coface_ids"]
            ]
            components = batch["post_lot_components"]
            left, right = next(
                (left, right)
                for left in components
                for right in components
                if left is not right
                and not any(
                    relation.intersection(
                        tuple(facet) for facet in left["active_facet_point_ids"]
                    )
                    and relation.intersection(
                        tuple(facet) for facet in right["active_facet_point_ids"]
                    )
                    for relation in relation_facets
                )
            )
            left["active_facet_point_ids"] = sorted(
                left["active_facet_point_ids"] + right["active_facet_point_ids"]
            )
            left["covered_point_ids"] = sorted(
                set(left["covered_point_ids"] + right["covered_point_ids"])
            )
            components.remove(right)

        self.assert_false_exact_rejected(mutate)

    def test_coverage_delta_is_the_exact_pre_to_post_difference(self) -> None:
        def mutate(result: dict) -> None:
            batch_delta = next(
                delta
                for batch in result["equal_level_batches"]
                for delta in batch["coverage_deltas"]
                if len(delta["added_point_ids"]) > 1
            )
            key = (
                batch_delta["batch_id"],
                batch_delta["order"],
                batch_delta["root_node_id"],
            )
            replacement = batch_delta["added_point_ids"][:-1]
            copies = [
                delta
                for delta in (
                    [
                        delta
                        for batch in result["equal_level_batches"]
                        for delta in batch["coverage_deltas"]
                    ]
                    + [
                        delta
                        for forest in result["forests"]
                        for delta in forest["coverage_log"]
                    ]
                    + result["coverage_log"]
                )
                if (
                    delta["batch_id"],
                    delta["order"],
                    delta["root_node_id"],
                )
                == key
            ]
            self.assertEqual(len(copies), 3)
            for delta in copies:
                delta["added_point_ids"] = replacement

        self.assert_false_exact_rejected(mutate)

    def test_final_forest_roots_equal_the_last_post_lot_components(self) -> None:
        def mutate(result: dict) -> None:
            forest = next(
                candidate for candidate in result["forests"] if candidate["order"] == 1
            )
            final_batch = max(
                (
                    batch
                    for batch in result["equal_level_batches"]
                    if batch["order"] == 1
                ),
                key=exact_level_value,
            )
            component = final_batch["post_lot_components"][0]
            component["component_id"] = next(
                node["node_id"]
                for node in forest["nodes"]
                if node["node_id"] not in forest["root_ids"]
            )

        self.assert_false_exact_rejected(mutate)

    def test_exact_catalog_keeps_every_rank_one_birth(self) -> None:
        result = copy.deepcopy(self.minimal_exact)
        event_id = result["critical_catalog"][0]["event_id"]
        result["critical_catalog"] = []
        for batch in result["equal_level_batches"]:
            batch["event_ids"] = [
                candidate for candidate in batch["event_ids"] if candidate != event_id
            ]
            batch["post_lot_components"] = []
            batch["coverage_deltas"] = []
        for forest in result["forests"]:
            forest["nodes"] = []
            forest["root_ids"] = []
            forest["coverage_log"] = []
        result["coverage_log"] = []
        result["materialized_point_sets"] = []
        counters = result["run_certificate"]["work_and_memory_counters"]
        counters["accepted_events"] -= 1
        counters["candidate_events"] -= 1

        with self.assertRaises(ContractError):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_exact_hyperedge_has_a_level_matched_gamma_coface(self) -> None:
        def mutate(result: dict) -> None:
            hyperedge = result["gabriel_hyperedges"][0]
            coface = next(
                candidate
                for candidate in result["gamma_cofaces"]
                if candidate["order"] == hyperedge["order"]
                and candidate["simplex_point_ids"]
                == hyperedge["simplex_point_ids"]
                and exact_level_value(candidate) == exact_level_value(hyperedge)
            )
            old_id = coface["coface_id"]
            source_batch = next(
                batch
                for batch in result["equal_level_batches"]
                if old_id in batch["gamma_coface_ids"]
            )
            destination = next(
                batch
                for batch in result["equal_level_batches"]
                if batch["order"] == hyperedge["order"]
                and exact_level_value(batch) != exact_level_value(hyperedge)
            )
            source_batch["gamma_coface_ids"].remove(old_id)
            coface["squared_level_exact"] = copy.deepcopy(
                destination["squared_level_exact"]
            )
            coface["coface_id"] = canonical_contract_id(
                "GammaCoface",
                {
                    "schema_version": coface["schema_version"],
                    "order": coface["order"],
                    "simplex_point_ids": coface["simplex_point_ids"],
                    "facet_point_ids": coface["facet_point_ids"],
                    "squared_level_exact": coface["squared_level_exact"],
                },
            )
            destination["gamma_coface_ids"] = sorted(
                [*destination["gamma_coface_ids"], coface["coface_id"]]
            )

        self.assert_false_exact_rejected(mutate)

    def test_exact_hyperedge_cannot_move_to_a_different_batch(self) -> None:
        def mutate(result: dict) -> None:
            hyperedge = result["gabriel_hyperedges"][0]
            source = next(
                batch
                for batch in result["equal_level_batches"]
                if hyperedge["hyperedge_id"] in batch["hyperedge_ids"]
            )
            destination = next(
                batch
                for batch in result["equal_level_batches"]
                if batch["order"] == hyperedge["order"]
                and batch["batch_id"] != source["batch_id"]
            )
            source["hyperedge_ids"].remove(hyperedge["hyperedge_id"])
            destination["hyperedge_ids"] = sorted(
                [*destination["hyperedge_ids"], hyperedge["hyperedge_id"]]
            )

        self.assert_false_exact_rejected(mutate)


if __name__ == "__main__":
    unittest.main()
