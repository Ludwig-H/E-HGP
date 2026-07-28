from __future__ import annotations

import copy
import math
import unittest

from tools.check_phase15_gabriel_neighbor_rank import (
    ValidationError,
    validate_artifact,
)


def _histogram(maximum_rank: int, observed_rank: int) -> dict[str, object]:
    counts = [0] * maximum_rank
    counts[observed_rank - 1] = 1
    return {
        "exact_rank_counts_1_through_M": counts,
        "captured_count": 1,
        "overflow_count": 0,
        "maximum_observed_rank": observed_rank,
        "p50_smallest_M": observed_rank,
        "p90_smallest_M": observed_rank,
        "p95_smallest_M": observed_rank,
        "p99_smallest_M": observed_rank,
    }


def _payload() -> dict[str, object]:
    point_count = 257
    maximum_rank = 128
    variant_ranks = {
        "directed_root_star": 3,
        "symmetric_union_star": 2,
        "mutual_star": 4,
    }
    samples = []
    for multiplier in (1, 2, 4, 8):
        policy_rank = max(2, math.ceil(2 * multiplier * math.log(point_count)))
        samples.append(
            {
                "c": multiplier,
                "M": policy_rank,
                "within_measured_cap": True,
                "directed_root_star_captured_count": 1,
                "symmetric_union_star_captured_count": 1,
                "mutual_star_captured_count": 1,
            }
        )
    return {
        "schema": "morsehgp3d.phase15_delaunay_gabriel_neighbor_rank.v1",
        "phase": "15",
        "profile": "hgp_reduced",
        "mode": "offline_neighbor_rank",
        "public_status": "not_claimed",
        "architecture": {
            "higher_order_Delaunay_mosaic_materialized": False,
            "global_n_by_M_neighbor_table_materialized": False,
            "additional_global_triangle_rank_table_materialized": False,
        },
        "input": {"point_count": point_count},
        "classification": {"gabriel_binary64_count": 1},
        "coverage": {
            "compact_neighbor_rank_gate_passed": True,
            "selected_process_gate_passed": True,
        },
        "neighbor_rank_diagnostic": {
            "schema": "morsehgp3d.phase15_gabriel_neighbor_rank.v1",
            "status": "offline_proposal_diagnostic_not_a_proof",
            "scientific_scope": (
                "PDEL_witness_root_neighbor_wedge_thresholds_on_the_accepted_"
                "Gabriel_set_not_a_Gabriel_or_Gamma2_catalogue"
            ),
            "rank_semantics": "exact_prefix_for_the_declared_binary64_key",
            "rank_overflow_semantics": (
                "exact_rank_strictly_greater_than_M_value_censored"
            ),
            "maximum_rank_M": maximum_rank,
            "one_complete_LBVH_traversal_per_source_not_per_arc": True,
            "two_incident_edges_form_one_wedge_proposal": True,
            "support_two_PDEL_witnesses_allowed": True,
            "root_scope": "minimum_over_available_PDEL_witness_roots",
            "partial_root_threshold_relation": (
                "safe_upper_bound_on_the_minimum_over_all_three_roots"
            ),
            "absolute_minimum_over_all_three_roots_claimed": False,
            "Gabriel_catalogue_claimed": False,
            "exact_Gamma2_claimed": False,
            "CSR_row_writes_race_free_by_source_permutation": True,
            "maximum_logical_heap_storage_bytes_per_query": 4096,
            "CUDA_local_memory_spill_and_throughput_require_G4_qualification": True,
            "n_by_M_table_materialized": False,
            "additional_global_triangle_table_materialized": False,
            "compact_rank_gate_passed": True,
            "all_variants_have_PDEL_witness_root": True,
            "accepted_triangle_count": 1,
            "complete_three_pair_triangle_count": 0,
            "partial_two_pair_witness_triangle_count": 1,
            "insufficient_pair_witness_triangle_count": 0,
            "variants": {
                name: {
                    "definition": name,
                    "threshold_scope": "minimum_over_available_PDEL_witness_roots",
                    "witness_triangle_count": 1,
                    "missing_witness_triangle_count": 0,
                    "considered_root_count": 1,
                    "first_missing_witness_triangle": None,
                    "histogram": _histogram(maximum_rank, rank),
                }
                for name, rank in variant_ranks.items()
            },
            "bounded_bruteforce_rank_replay_point_cap": 4096,
            "bounded_bruteforce_rank_replay_performed": True,
            "bounded_bruteforce_rank_replay_passed": True,
            "bounded_bruteforce_rank_replay": {
                "point_count": point_count,
                "compared_arc_count": 12,
                "mismatch_count": 0,
                "canonical_source_mapping_non_identity_observed": True,
                "binary64_distance_tie_observed": True,
                "censored_rank_observed": True,
                "first_mismatch": None,
            },
            "proposal_policy_K_log_n": {
                "K": 2,
                "natural_log": True,
                "proposal_only_not_a_completeness_proof": True,
                "samples": samples,
            },
            "lbvh_audit": {
                "point_count": point_count,
                "certified_node_count": 2 * point_count - 1,
                "directed_CSR_arc_count": 12,
                "maximum_rank": maximum_rank,
                "overflow_rank": maximum_rank + 1,
                "kernel_local_size_bytes_per_thread": 4120,
                "seed_window_radius": maximum_rank,
                "query_batch_size": 64,
                "query_batch_count": 5,
                "ranked_arc_count": 10,
                "overflow_arc_count": 2,
                "completed_query_count": point_count,
                "failed_query_count": 0,
                "fixed_round_to_nearest_distance_recipe_requested": True,
                "directed_round_down_AABB_recipe_requested": True,
                "strict_prune_requested": True,
                "equality_descends_requested": True,
                "stackless_postorder_traversal_requested": True,
                "complete_query_coverage": True,
                "no_search_work_cap": True,
                "exact_binary64_prefix_complete": True,
                "ranks_above_maximum_censored": True,
                "source_mapping_permutation_validated": True,
                "csr_structure_validated": True,
                "host_rank_transcript_validated": True,
                "n_by_maximum_rank_table_materialized": False,
                "higher_order_delaunay_mosaic_materialized": False,
                "global_pair_matrix_materialized": False,
                "exact_morse_hierarchy_claimed": False,
                "public_status_claimed": False,
            },
        },
    }


class Phase15GabrielNeighborRankCheckerTests(unittest.TestCase):
    def test_closed_compact_artifact_passes(self) -> None:
        validate_artifact(_payload())

    def test_n_by_m_table_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["architecture"]["global_n_by_M_neighbor_table_materialized"] = True
        with self.assertRaisesRegex(ValidationError, "n-by-M"):
            validate_artifact(changed)

    def test_variant_ordering_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        histogram = changed["neighbor_rank_diagnostic"]["variants"][
            "symmetric_union_star"
        ]["histogram"]
        histogram["exact_rank_counts_1_through_M"][1] = 0
        histogram["exact_rank_counts_1_through_M"][4] = 1
        histogram["maximum_observed_rank"] = 5
        for percent in (50, 90, 95, 99):
            histogram[f"p{percent}_smallest_M"] = 5
        with self.assertRaisesRegex(ValidationError, "ordering"):
            validate_artifact(changed)

    def test_censored_rank_partition_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        audit = changed["neighbor_rank_diagnostic"]["lbvh_audit"]
        audit["overflow_arc_count"] += 1
        with self.assertRaisesRegex(ValidationError, "partition"):
            validate_artifact(changed)

    def test_support_two_witness_is_not_rejected_as_missing_pair(self) -> None:
        payload = _payload()
        validate_artifact(payload)
        self.assertEqual(
            payload["neighbor_rank_diagnostic"][
                "partial_two_pair_witness_triangle_count"
            ],
            1,
        )

    def test_missing_variant_witness_fails_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        variant = changed["neighbor_rank_diagnostic"]["variants"][
            "directed_root_star"
        ]
        variant["witness_triangle_count"] = 0
        variant["missing_witness_triangle_count"] = 1
        variant["considered_root_count"] = 0
        variant["first_missing_witness_triangle"] = {"a": 0, "b": 1, "c": 2}
        with self.assertRaisesRegex(ValidationError, "missing PDEL witness"):
            validate_artifact(changed)

    def test_lbvh_authority_flags_fail_closed(self) -> None:
        for field, invalid in (
            ("source_mapping_permutation_validated", False),
            ("csr_structure_validated", False),
            ("host_rank_transcript_validated", False),
            ("n_by_maximum_rank_table_materialized", True),
            ("higher_order_delaunay_mosaic_materialized", True),
            ("global_pair_matrix_materialized", True),
            ("exact_morse_hierarchy_claimed", True),
            ("public_status_claimed", True),
        ):
            with self.subTest(field=field):
                changed = copy.deepcopy(_payload())
                changed["neighbor_rank_diagnostic"]["lbvh_audit"][field] = invalid
                with self.assertRaisesRegex(ValidationError, field):
                    validate_artifact(changed)

    def test_invalid_seed_and_batch_fail_closed(self) -> None:
        changed = copy.deepcopy(_payload())
        changed["neighbor_rank_diagnostic"]["lbvh_audit"][
            "seed_window_radius"
        ] = 127
        with self.assertRaisesRegex(ValidationError, "seed window"):
            validate_artifact(changed)

        changed = copy.deepcopy(_payload())
        changed["neighbor_rank_diagnostic"]["lbvh_audit"][
            "query_batch_size"
        ] = 0
        with self.assertRaisesRegex(ValidationError, "query_batch_size"):
            validate_artifact(changed)


if __name__ == "__main__":
    unittest.main()
