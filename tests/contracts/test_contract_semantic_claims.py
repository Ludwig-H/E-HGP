from __future__ import annotations

import copy
import sys
import unittest
from fractions import Fraction
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from check_contracts import (  # noqa: E402
    FIXTURE_DIR,
    ContractError,
    canonical_contract_id,
    load_json,
    validate_result_semantics,
)


def exact_level_value(batch: dict) -> Fraction:
    level = batch["squared_level_exact"]
    return Fraction(int(level["numerator"]), int(level["denominator"]))


class SemanticClaimMutationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.exact = load_json(FIXTURE_DIR / "overlap-k2.json")
        cls.full_pi0 = load_json(FIXTURE_DIR / "binary-merge-k2.json")
        validate_result_semantics(cls.exact)
        validate_result_semantics(cls.full_pi0)

    def test_exact_status_requires_the_none_partial_guarantee(self) -> None:
        result = copy.deepcopy(self.exact)
        result["run_certificate"]["partial_guarantees"] = ["verified_events"]

        with self.assertRaisesRegex(
            ContractError, r"partial_guarantees=\[none\]"
        ):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_conditional_and_budget_statuses_reject_none_with_coherent_scope(self) -> None:
        for status in ("conditional", "budget_exhausted"):
            with self.subTest(status=status):
                result = copy.deepcopy(self.full_pi0)
                certificate = result["run_certificate"]
                certificate["public_status"] = status
                certificate["partial_guarantees"] = ["none"]
                result["partial_scope"]["positive_guarantees"] = ["none"]
                if status == "budget_exhausted":
                    certificate["stop_reason"] = "time_budget"

                with self.assertRaisesRegex(
                    ContractError, "cannot claim partial_guarantees=none"
                ):
                    validate_result_semantics(result, check_canonical_ids=False)

    def test_complete_full_pi0_attachments_cannot_omit_one_arm(self) -> None:
        result = copy.deepcopy(self.full_pi0)
        attachment = result["attachments"].pop()
        for batch in result["equal_level_batches"]:
            batch["attachment_ids"] = [
                attachment_id
                for attachment_id in batch["attachment_ids"]
                if attachment_id != attachment["attachment_id"]
            ]

        with self.assertRaisesRegex(ContractError, "attach every saddle arm"):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_complete_rank_one_catalog_has_one_birth_per_point_when_conditional(
        self,
    ) -> None:
        result = copy.deepcopy(self.full_pi0)
        event = next(
            event for event in result["critical_catalog"] if event["closed_rank"] == 1
        )
        event_id = event["event_id"]
        result["critical_catalog"] = [
            candidate
            for candidate in result["critical_catalog"]
            if candidate["event_id"] != event_id
        ]
        for batch in result["equal_level_batches"]:
            batch["event_ids"] = [
                candidate for candidate in batch["event_ids"] if candidate != event_id
            ]
        for forest in result["forests"]:
            for node in forest["nodes"]:
                node["event_ids"] = [
                    candidate for candidate in node["event_ids"] if candidate != event_id
                ]
        counters = result["run_certificate"]["work_and_memory_counters"]
        counters["accepted_events"] -= 1
        counters["candidate_events"] -= 1

        with self.assertRaisesRegex(ContractError, "rank-one birth per point"):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_verified_event_ids_reject_an_uncertified_event(self) -> None:
        result = copy.deepcopy(self.full_pi0)
        verified_id = result["partial_scope"]["verified_event_ids"][0]
        event = next(
            event
            for event in result["critical_catalog"]
            if event["event_id"] == verified_id
        )
        rank = event["closed_rank"]
        event["predicate_status"] = "numeric_failure"
        result["run_certificate"]["catalog_complete_by_rank"][rank - 1] = False
        result["partial_scope"]["catalog_complete_ranks"] = [
            candidate
            for candidate in result["partial_scope"]["catalog_complete_ranks"]
            if candidate != rank
        ]

        with self.assertRaisesRegex(
            ContractError, "verified_event_ids contains an uncertified event"
        ):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_closed_full_pi0_replays_pre_post_and_coverage(self) -> None:
        continuity = copy.deepcopy(self.full_pi0)
        batches = sorted(
            (
                batch
                for batch in continuity["equal_level_batches"]
                if batch["order"] == 1
            ),
            key=exact_level_value,
        )
        current = next(
            batch for batch in batches[1:] if len(batch["pre_lot_components"]) > 1
        )
        current["pre_lot_components"].pop()

        with self.subTest(mutation="pre-post"):
            with self.assertRaises(ContractError):
                validate_result_semantics(
                    continuity, check_canonical_ids=False
                )

        coverage = copy.deepcopy(self.full_pi0)
        source_delta = next(
            delta
            for batch in coverage["equal_level_batches"]
            for delta in batch["coverage_deltas"]
            if delta["added_point_ids"]
        )
        delta_key = (
            source_delta["batch_id"],
            source_delta["order"],
            source_delta["root_node_id"],
        )
        all_deltas = (
            [
                delta
                for batch in coverage["equal_level_batches"]
                for delta in batch["coverage_deltas"]
            ]
            + [
                delta
                for forest in coverage["forests"]
                for delta in forest["coverage_log"]
            ]
            + coverage["coverage_log"]
        )
        matching_deltas = [
            delta
            for delta in all_deltas
            if (
                delta["batch_id"],
                delta["order"],
                delta["root_node_id"],
            )
            == delta_key
        ]
        self.assertEqual(len(matching_deltas), 3)
        for delta in matching_deltas:
            delta["added_point_ids"] = []

        with self.subTest(mutation="coverage"):
            with self.assertRaises(ContractError):
                validate_result_semantics(coverage, check_canonical_ids=False)

    def test_complete_saddle_rank_cannot_lose_its_gabriel_hyperedge(self) -> None:
        result = copy.deepcopy(self.full_pi0)
        hyperedge = result["gabriel_hyperedges"].pop()
        event = next(
            event
            for event in result["critical_catalog"]
            if event["event_id"] == hyperedge["event_id"]
        )
        self.assertTrue(
            result["run_certificate"]["catalog_complete_by_rank"][
                event["closed_rank"] - 1
            ]
        )
        for batch in result["equal_level_batches"]:
            batch["hyperedge_ids"] = [
                hyperedge_id
                for hyperedge_id in batch["hyperedge_ids"]
                if hyperedge_id != hyperedge["hyperedge_id"]
            ]
        result["run_certificate"]["work_and_memory_counters"][
            "hyperedges_emitted"
        ] -= 1

        with self.assertRaisesRegex(
            ContractError, "complete saddle-event catalog"
        ):
            validate_result_semantics(result, check_canonical_ids=False)

    def test_partial_cell_and_vertex_ids_use_only_exact_witness_data(self) -> None:
        result = copy.deepcopy(self.full_pi0)
        vertex_projection = {
            "position_exact": {
                "schema_version": "2.0.0",
                "x_numerator": "0",
                "y_numerator": "0",
                "z_numerator": "0",
                "denominator": "1",
                "unit": "input_coordinate_unit",
            },
            "binding_constraint_ids": [],
            "affine_dimension": 0,
            "artificial_boundary": False,
            "degeneracy_class": "none",
        }
        witness = {
            "schema_version": "2.0.0",
            "vertex_id": canonical_contract_id(
                "VertexWitness", vertex_projection
            ),
            **vertex_projection,
            "approximation_bits": [
                "0000000000000000",
                "0000000000000000",
                "0000000000000000",
            ],
            "certification_stage": "cpu_multiprecision",
        }
        cell_projection = {
            "depth": 0,
            "label_point_ids": [],
            "vertex_witnesses": [
                {"vertex_id": witness["vertex_id"], **vertex_projection}
            ],
            "cross_constraint_ids": [],
            "active_cross_incidence_ids": [],
        }
        cell = {
            "schema_version": "2.0.0",
            "cell_id": canonical_contract_id(
                "CanonicalCellCertificate", cell_projection
            ),
            **cell_projection,
            "vertex_witnesses": [witness],
            "closure_rounds": 1,
            "global_queue_empty": True,
            "all_vertices_certified": True,
            "active_cross_incidences_complete": True,
            "artificial_boundary_only_marked": True,
            "certificate_status": "closed",
        }
        result["partial_scope"]["canonical_cell_certificates"] = [cell]
        validate_result_semantics(result)

        diagnostics_only = copy.deepcopy(result)
        diagnostics_only["partial_scope"]["canonical_cell_certificates"][0][
            "vertex_witnesses"
        ][0]["approximation_bits"][0] = "3ff0000000000000"
        validate_result_semantics(diagnostics_only)

        for label, field_path in (
            ("vertex", ("vertex_witnesses", 0, "vertex_id")),
            ("cell", ("cell_id",)),
        ):
            mutated = copy.deepcopy(result)
            target = mutated["partial_scope"]["canonical_cell_certificates"][0]
            if len(field_path) == 1:
                target[field_path[0]] = "0" * 64
            else:
                target[field_path[0]][field_path[1]][field_path[2]] = "0" * 64
            with self.subTest(identifier=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(mutated)


if __name__ == "__main__":
    unittest.main()
