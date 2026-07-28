from __future__ import annotations

import copy
import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout

import tools.check_phase15_delaunay_gabriel_fusion_guardrails as checker
from tools.check_phase15_gabriel_coverage import (
    recompute_emitted_fixture_sha256,
)


SHA256_ZERO = "0" * 64


def _triangle(
    a: int,
    b: int,
    c: int,
    level: float,
    *,
    support: int,
) -> dict[str, object]:
    return {
        "a": a,
        "b": b,
        "c": c,
        "squared_level": level,
        "status": checker.GABRIEL_STATUS,
        "support_cardinality": support,
    }


def _seal_outer(payload: dict[str, object]) -> None:
    for path, digest in recompute_emitted_fixture_sha256(payload).items():
        section, key = path.split(".", 1)
        record = payload[section]
        assert isinstance(record, dict)
        record[key] = digest


def _direct_variants(
    accepted_sha256: str,
    ambiguous_sha256: str,
    accepted_count: int,
    ambiguous_count: int,
) -> list[dict[str, object]]:
    result = []
    for variant in checker.DIRECT_VARIANTS:
        result.append(
            {
                "variant": variant,
                "representation": "k2_pair_facet_relation_stream",
                "certificate_kind": checker.DIRECT_CERTIFICATE_KIND,
                "full_variant_first_connection_replayed": False,
                "source_inclusion_basis": checker.DIRECT_INCLUSION_BASIS,
                "source_triangle_included_by_definition": True,
                "counts": {
                    "source_triangle_count": accepted_count + ambiguous_count,
                    "accepted_source_triangle_count": accepted_count,
                    "guaranteed_no_later_than_deadline_count": accepted_count,
                    "violation_count": 0,
                    "unsupported_count": ambiguous_count,
                },
                "deadline_upper_bound_partition_closed": True,
                "regular_binary64_guardrail_passed": True,
                "fail_closed_guardrail_passed": ambiguous_count == 0,
                "decision_sha256": (
                    checker.recompute_direct_variant_decision_sha256(
                        variant,
                        accepted_sha256,
                        ambiguous_sha256,
                        accepted_count,
                        ambiguous_count,
                    )
                ),
                "first_failure_witness": None,
                "first_unsupported_witness": None,
            }
        )
    return result


def _order(
    order: int,
    source_json: list[dict[str, object]],
) -> dict[str, object]:
    raw_json = [
        {"u": 0, "v": 1, "raw_squared_weight": 2.0},
        {"u": 0, "v": 2, "raw_squared_weight": 2.0},
        {"u": 0, "v": 3, "raw_squared_weight": 2.0},
    ]
    sources = checker._parse_triangle_array(
        source_json, path="test.sources", point_count=4
    )
    raw_tree = checker._parse_raw_tree_edges(
        raw_json, path="test.raw_tree", point_count=4
    )
    replay = checker._replay_surrogate(sources, raw_tree)
    corrected_tree_json = [edge.as_json() for edge in replay.corrected_tree]
    corrected_root = replay.corrected_tree[-1]
    raw_regular = replay.counts["late_count"] == replay.counts["never_count"] == 0
    raw_fail_closed = raw_regular and replay.counts["unsupported_count"] == 0
    corrected_regular = (
        replay.corrected_counts["late_count"]
        == replay.corrected_counts["never_count"]
        == 0
    )
    corrected_fail_closed = (
        corrected_regular and replay.corrected_counts["unsupported_count"] == 0
    )
    return {
        "order": order,
        "tree": {
            "proposed_edge_count": 7,
            "unique_edge_count": 6,
            "selected_tree_edge_count": 3,
            "final_component_count": 1,
            "distinct_level_count": 1,
            "root_raw_squared_weight": 2.0,
            "tree_sha256": checker._raw_tree_sha256(4, order, raw_tree),
            "surrogate_compatible_digest": "0x0000000000000000",
            "tree_edges": raw_json,
        },
        "counts": replay.counts,
        "classification_partition_closed": True,
        "regular_binary64_guardrail_passed": raw_regular,
        "fail_closed_guardrail_passed": raw_fail_closed,
        "decision_sha256": checker._decision_sha256(
            order, sources, replay.decisions, corrected=False
        ),
        "first_failure_witness": replay.first_failure_witness,
        "first_late_witness": replay.first_late_witness,
        "first_unsupported_witness": replay.first_unsupported_witness,
        "correction": {
            "algorithm": (
                "canonical_greedy_closed_plateau_Gabriel_triangle_overlay_v1"
            ),
            "correction_triangle_count": len(replay.correction_records),
            "useful_union_count": replay.useful_union_count,
            "corrected_postcondition_violation_count": (
                replay.corrected_postcondition_violation_count
            ),
            "regular_binary64_guardrail_passed": corrected_regular,
            "fail_closed_guardrail_passed": corrected_fail_closed,
            "overlay_sha256": checker._overlay_sha256(
                order, replay.correction_records
            ),
            "corrected_decision_sha256": checker._decision_sha256(
                order, sources, replay.corrected_decisions, corrected=True
            ),
            "counts": replay.corrected_counts,
            "classification_partition_closed": True,
            "correction_applied_to_transient_verified_tree": True,
            "corrected_tree_records_serialized": True,
            "first_failure_witness": replay.corrected_first_failure_witness,
            "first_late_witness": replay.corrected_first_late_witness,
            "first_unsupported_witness": (
                replay.corrected_first_unsupported_witness
            ),
            "corrected_tree": {
                "representation": (
                    "point_merge_tree_at_Gabriel_squared_levels_v1"
                ),
                "selected_tree_edge_count": 3,
                "final_component_count": 1,
                "distinct_level_count": 1,
                "root_level": {
                    "level_encoding": corrected_root.level_encoding,
                    "encoded_binary64": corrected_root.encoded_binary64,
                },
                "tree_sha256": checker._corrected_tree_sha256(
                    4, order, replay.corrected_tree
                ),
                "records_serialized": True,
                "tree_edges": corrected_tree_json,
            },
            "overlay_records": [
                record.as_json() for record in replay.correction_records
            ],
        },
        "timings_nanoseconds": {
            "edge_build": 0,
            "tree_reduction": 0,
            "deadline_replay_and_overlay": 0,
        },
    }


def _payload() -> dict[str, object]:
    sources = [
        _triangle(0, 1, 2, 0.5, support=3),
        _triangle(0, 1, 3, 0.5, support=3),
        _triangle(0, 2, 3, 0.5, support=2),
        _triangle(1, 2, 3, 2.0, support=2),
    ]
    payload: dict[str, object] = {
        "schema": checker.INPUT_SCHEMA,
        "git_sha": "1" * 40,
        "phase": "15",
        "backend": checker.OUTER_BACKEND,
        "profile": "hgp_reduced",
        "mode": checker.OUTER_MODE,
        "deployment_status": "diagnostic_sidecar",
        "public_status": "not_claimed",
        "criterion": (
            "triangle_explicit_or_three_facets_connected_at_strictly_lower_level"
        ),
        "coverage_reduction_mode": checker.OUTER_COVERAGE_MODE,
        "proof_scope": {
            "general_position_required": True,
            "complete_Voronoi_nerve_one_skeleton_required": True,
            "Geogram_SoS_topology_exactly_recertified": False,
            "every_regular_Gabriel_triangle_has_at_least_two_Delaunay_edges": True,
            "support_cardinality_two_and_three_covered": True,
            "exact_Gamma2_claimed": False,
            "exact_Gabriel_classification_claimed": False,
        },
        "architecture": {
            "edge_times_point_product_enumerated": False,
            "higher_order_Delaunay_mosaic_materialized": False,
            "ordinary_Delaunay_edges_only": True,
            "canonical_wedges_enumerated_in_bounded_GPU_chunks": True,
            "global_restricted_Gamma2_arena_materialized": False,
            "global_Gabriel_proposal_arena_materialized_for_level_sort": True,
            "same_level_candidates_can_certify_each_other": False,
        },
        "input": {
            "point_count": 4,
            "seed": 7,
            "cpu_workers_requested": 4,
            "cpu_workers_used": 4,
            "point_cloud_sha256": SHA256_ZERO,
        },
        "delaunay": {
            "engine": "PDEL",
            "cell_count": 1,
            "ordinary_edge_count": 6,
            "maximum_vertex_degree": 3,
            "ordinary_edges_sha256": SHA256_ZERO,
        },
        "candidate_universe": {
            "generator": "canonical_Delaunay_CSR_wedges",
            "raw_wedge_count": 12,
            "canonical_candidate_count": 4,
            "canonical_wedge_universe_sha256": SHA256_ZERO,
            "chunk_count": 2,
            "triangle_chunk_vertices": 3,
            "maximum_chunk_candidate_count": 3,
        },
        "classification": {
            "blocked_count": 0,
            "gabriel_binary64_count": 4,
            "ambiguous_safety_count": 0,
            "invalid_count": 0,
            "status_partition_closed": True,
            "visited_cell_count": 4,
            "tested_point_count": 8,
        },
        "coverage": {
            "source_triangle_count": 4,
            "support_two_count": 2,
            "support_three_count": 2,
            "explicit_necessary_accepted_count": 4,
            "strictly_lower_connected_count": 0,
            "coverage_violation_count": 0,
            "accepted_partition_closed": True,
            "binary64_gate_passed": True,
            "conditional_cross_variant_gate_passed": True,
            "fail_closed_cross_variant_gate_passed": True,
            "allow_conditional_guardrail": False,
            "selected_process_gate_passed": True,
            "accepted_source_sha256": SHA256_ZERO,
            "necessary_accepted_sha256": SHA256_ZERO,
            "ambiguous_safety_sha256": SHA256_ZERO,
            "explicit_safety_batch_count": 4,
            "explicit_safety_batch_sha256": SHA256_ZERO,
            "explicit_safety_batch_semantics": (
                "necessary_accepted_plus_all_ambiguous"
            ),
            "necessary_records": copy.deepcopy(sources),
            "ambiguous_safety_records": [],
        },
        "emitted_fixture": {
            "points": [[3, 3, 4], [-4, 5, -4], [-3, 0, 5], [-5, -1, 2]],
            "ordinary_delaunay_edges": [
                {"u": u, "v": v}
                for u in range(4)
                for v in range(u + 1, 4)
            ],
            "source_records": copy.deepcopy(sources),
        },
        "gpu": {
            "device_name": "fixture",
            "multiprocessor_count": 1,
            "grid_resolution": 1,
            "maximum_cell_occupancy": 4,
            "persistent_device_bytes": 1,
            "maximum_chunk_device_bytes": 1,
            "maximum_accounted_live_device_bytes": 2,
        },
        "timings_nanoseconds": {"surrogate_guardrail": 0, "cold_e2e": 0},
    }
    _seal_outer(payload)
    input_record = payload["input"]
    coverage = payload["coverage"]
    assert isinstance(input_record, dict)
    assert isinstance(coverage, dict)
    accepted_sha = str(coverage["accepted_source_sha256"])
    ambiguous_sha = str(coverage["ambiguous_safety_sha256"])
    orders = [_order(1, sources), _order(2, sources)]
    payload["variant_guardrails"] = {
        "schema": checker.INNER_SCHEMA,
        "criterion": "gabriel_fusion_deadline_v1",
        "boundary": "closed_after_complete_candidate_plateau",
        "early_connections_accepted": True,
        "scientific_scope": "conditional_binary64_guardrail_not_Gamma2_exactness",
        "raw_surrogate_regular_guardrail_passed": True,
        "corrected_surrogate_regular_guardrail_passed": True,
        "corrected_surrogate_fail_closed_guardrail_passed": True,
        "source": {
            "accepted_gabriel_binary64_count": 4,
            "unsupported_ambiguous_count": 0,
            "source_triangle_count": 4,
            "level_convention": checker.SOURCE_LEVEL_CONVENTION,
            "point_cloud_sha256": input_record["point_cloud_sha256"],
            "accepted_source_sha256": accepted_sha,
        },
        "direct_facet_variants": _direct_variants(
            accepted_sha, ambiguous_sha, 4, 0
        ),
        "surrogate": {
            "representation": "point_weighted_spanning_tree_binary64_v1",
            "adapter": {
                "adapter_id": checker.POINT_LIFT_ADAPTER,
                "component_lift": checker.POINT_LIFT_SEMANTICS,
                "input_level_convention": checker.RAW_TREE_LEVEL_CONVENTION,
                "output_level_convention": checker.SOURCE_LEVEL_CONVENTION,
                "level_conversion": checker.LEVEL_CONVERSION,
                "exact_dyadic_comparison_of_binary64_recipe_outputs": True,
                "native_k2_facet_domain": False,
                "Gamma2_exactness_claimed": False,
            },
            "canonical_to_source_mapping_sha256": SHA256_ZERO,
            "remapped_top_k_transcript_sha256": SHA256_ZERO,
            "top_k_audit": {
                "backend": "cuda_binary64_lbvh_top_k",
                "maximum_order": 2,
                "seed_window_radius": 2,
                "neighbor_record_count": 8,
                "point_count": 4,
                "certified_node_count": 7,
                "completed_query_count": 4,
                "failed_query_count": 0,
                "source_snapshot_epoch": 1,
                "complete_query_coverage": True,
                "no_candidate_truncation": True,
                "source_morton_permutation_validated": True,
                "host_transcript_structure_validated": True,
                "traversal_lease_adopted": True,
                "invalid_aabb_bound_descent_count": 0,
                "fixed_round_to_nearest_distance_recipe_requested": True,
                "directed_round_down_aabb_recipe_requested": True,
                "strict_prune_requested": True,
                "stackless_postorder_traversal_requested": True,
                "persistent_input_device_byte_capacity": 1,
                "transient_output_device_byte_capacity": 1,
                "higher_order_delaunay_mosaic_materialized": False,
                "global_pair_matrix_materialized": False,
                "exact_morse_hierarchy_claimed": False,
                "public_status_claimed": False,
                "kernel_nanoseconds": 0,
                "launcher_wall_nanoseconds": 0,
                "device_name": "fixture",
            },
            "orders": orders,
        },
        "architecture": {
            "higher_order_Delaunay_mosaic_materialized": False,
            "global_pair_matrix_materialized": False,
            "weighted_tree_maximum_query_materialized": False,
            "orders_built_in_parallel": True,
            "order_worker_count": 2,
            "corrected_merge_tree_materialized_transiently": True,
            "massive_corrected_tree_records_serialized": False,
            "massive_overlay_records_materialized": False,
        },
        "timings_nanoseconds": {
            "canonicalization": 0,
            "certified_LBVH_build": 0,
            "top_k_query": 0,
        },
    }
    return payload


def _summary_payload() -> dict[str, object]:
    payload = _payload()
    payload["emitted_fixture"] = None
    coverage = payload["coverage"]
    assert isinstance(coverage, dict)
    coverage["necessary_records"] = None
    coverage["ambiguous_safety_records"] = None
    guardrails = payload["variant_guardrails"]
    assert isinstance(guardrails, dict)
    surrogate = guardrails["surrogate"]
    assert isinstance(surrogate, dict)
    orders = surrogate["orders"]
    assert isinstance(orders, list)
    for order in orders:
        assert isinstance(order, dict)
        tree = order["tree"]
        correction = order["correction"]
        assert isinstance(tree, dict)
        assert isinstance(correction, dict)
        corrected_tree = correction["corrected_tree"]
        assert isinstance(corrected_tree, dict)
        tree["tree_edges"] = None
        correction["overlay_records"] = None
        correction["corrected_tree_records_serialized"] = False
        corrected_tree["records_serialized"] = False
        corrected_tree["tree_edges"] = None
    return payload


def _conditional_summary_payload() -> dict[str, object]:
    payload = _summary_payload()
    classification = payload["classification"]
    coverage = payload["coverage"]
    guardrails = payload["variant_guardrails"]
    assert isinstance(classification, dict)
    assert isinstance(coverage, dict)
    assert isinstance(guardrails, dict)
    source = guardrails["source"]
    surrogate = guardrails["surrogate"]
    direct_variants = guardrails["direct_facet_variants"]
    assert isinstance(source, dict)
    assert isinstance(surrogate, dict)
    assert isinstance(direct_variants, list)

    classification["gabriel_binary64_count"] = 3
    classification["ambiguous_safety_count"] = 1
    coverage.update(
        {
            "source_triangle_count": 3,
            "support_two_count": 1,
            "support_three_count": 2,
            "explicit_necessary_accepted_count": 3,
            "conditional_cross_variant_gate_passed": True,
            "fail_closed_cross_variant_gate_passed": False,
            "allow_conditional_guardrail": True,
            "selected_process_gate_passed": True,
        }
    )
    source.update(
        {
            "accepted_gabriel_binary64_count": 3,
            "unsupported_ambiguous_count": 1,
            "source_triangle_count": 4,
        }
    )

    accepted_sha = str(coverage["accepted_source_sha256"])
    ambiguous_sha = str(coverage["ambiguous_safety_sha256"])
    replacement_direct = _direct_variants(accepted_sha, ambiguous_sha, 3, 1)
    ambiguous_source = _triangle(1, 2, 3, 2.0, support=2)
    ambiguous_source["status"] = checker.AMBIGUOUS_STATUS
    unsupported_witness = {
        "classification": "unsupported",
        "source": ambiguous_source,
        "observed_first_connection": None,
    }
    for direct in replacement_direct:
        direct["first_failure_witness"] = copy.deepcopy(unsupported_witness)
        direct["first_unsupported_witness"] = copy.deepcopy(
            unsupported_witness
        )
    guardrails["direct_facet_variants"] = replacement_direct

    orders = surrogate["orders"]
    assert isinstance(orders, list)
    for order in orders:
        assert isinstance(order, dict)
        counts = order["counts"]
        correction = order["correction"]
        assert isinstance(counts, dict)
        assert isinstance(correction, dict)
        corrected_counts = correction["counts"]
        assert isinstance(corrected_counts, dict)
        counts.update(
            {
                "supported_triangle_count": 3,
                "connected_at_count": 2,
                "unsupported_count": 1,
                "satisfied_count": 3,
                "violation_count": 1,
            }
        )
        corrected_counts.update(
            {
                "connected_at_count": 2,
                "unsupported_count": 1,
                "satisfied_count": 3,
                "violation_count": 1,
            }
        )
        order["fail_closed_guardrail_passed"] = False
        order["first_failure_witness"] = copy.deepcopy(unsupported_witness)
        order["first_unsupported_witness"] = copy.deepcopy(
            unsupported_witness
        )
        correction["fail_closed_guardrail_passed"] = False
        correction["first_failure_witness"] = copy.deepcopy(
            unsupported_witness
        )
        correction["first_unsupported_witness"] = copy.deepcopy(
            unsupported_witness
        )
    guardrails["corrected_surrogate_fail_closed_guardrail_passed"] = False
    return payload


def _k10_summary_payload() -> dict[str, object]:
    payload = _summary_payload()
    input_record = payload["input"]
    delaunay = payload["delaunay"]
    universe = payload["candidate_universe"]
    classification = payload["classification"]
    guardrails = payload["variant_guardrails"]
    assert isinstance(input_record, dict)
    assert isinstance(delaunay, dict)
    assert isinstance(universe, dict)
    assert isinstance(classification, dict)
    assert isinstance(guardrails, dict)
    surrogate = guardrails["surrogate"]
    architecture = guardrails["architecture"]
    assert isinstance(surrogate, dict)
    assert isinstance(architecture, dict)
    top_k = surrogate["top_k_audit"]
    orders = surrogate["orders"]
    assert isinstance(top_k, dict)
    assert isinstance(orders, list)
    template = orders[-1]
    assert isinstance(template, dict)

    point_count = 50_000
    input_record["point_count"] = point_count
    delaunay["ordinary_edge_count"] = 150_000
    delaunay["maximum_vertex_degree"] = 20
    universe.update(
        {
            "raw_wedge_count": 300_000,
            "canonical_candidate_count": 100_000,
            "chunk_count": 1,
            "triangle_chunk_vertices": point_count,
            "maximum_chunk_candidate_count": 100_000,
        }
    )
    classification["blocked_count"] = 99_996
    top_k.update(
        {
            "maximum_order": 10,
            "seed_window_radius": 32,
            "neighbor_record_count": point_count * 10,
            "point_count": point_count,
            "certified_node_count": 2 * point_count - 1,
            "completed_query_count": point_count,
        }
    )
    expanded_orders: list[dict[str, object]] = []
    for order_value in range(1, 11):
        item = copy.deepcopy(template)
        item["order"] = order_value
        tree = item["tree"]
        correction = item["correction"]
        assert isinstance(tree, dict)
        assert isinstance(correction, dict)
        corrected_tree = correction["corrected_tree"]
        assert isinstance(corrected_tree, dict)
        tree.update(
            {
                "proposed_edge_count": 2 * point_count - 1,
                "unique_edge_count": 2 * point_count - 1,
                "selected_tree_edge_count": point_count - 1,
            }
        )
        corrected_tree["selected_tree_edge_count"] = point_count - 1
        expanded_orders.append(item)
    surrogate["orders"] = expanded_orders
    architecture["order_worker_count"] = 10
    return payload


class Phase15DelaunayGabrielFusionGuardrailsCheckerTests(unittest.TestCase):
    def test_bounded_fixture_replays_both_surrogate_orders(self) -> None:
        report = checker.analyze_delaunay_guardrails(_payload())

        self.assertEqual(report["evidence"]["mode"], "independent_binary64_record_replay")
        self.assertTrue(report["guardrail_status"]["fail_closed_cross_variant_gate_passed"])
        self.assertEqual([item["order"] for item in report["surrogate_orders"]], [1, 2])
        self.assertTrue(all(item["record_replay_performed"] for item in report["surrogate_orders"]))
        self.assertEqual(len(report["direct_facet_variants"]), 5)
        self.assertTrue(all(not item["full_variant_first_connection_replayed"] for item in report["direct_facet_variants"]))

    def test_massive_summary_is_only_producer_commitment_evidence(self) -> None:
        report = checker.analyze_delaunay_guardrails(_summary_payload())

        self.assertEqual(report["evidence"]["mode"], "producer_commitments_only")
        self.assertFalse(report["evidence"]["independent_recertification_performed"])
        self.assertTrue(report["evidence"]["producer_commitments_not_independently_recomputable_without_records"])

    def test_rejects_order_one_without_order_two(self) -> None:
        payload = _summary_payload()
        surrogate = payload["variant_guardrails"]["surrogate"]
        del surrogate["orders"][1]
        surrogate["top_k_audit"]["maximum_order"] = 1
        surrogate["top_k_audit"]["neighbor_record_count"] = 4

        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "between 2 and 10"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_noncontiguous_orders(self) -> None:
        payload = _summary_payload()
        surrogate = payload["variant_guardrails"]["surrogate"]
        orders = surrogate["orders"]
        third = copy.deepcopy(orders[-1])
        third["order"] = 4
        orders.append(third)
        surrogate["top_k_audit"]["maximum_order"] = 3
        surrogate["top_k_audit"]["seed_window_radius"] = 3
        surrogate["top_k_audit"]["neighbor_record_count"] = 12

        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "contiguous sequence"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_maximum_order_above_ten(self) -> None:
        payload = _summary_payload()
        top_k = payload["variant_guardrails"]["surrogate"]["top_k_audit"]
        top_k["maximum_order"] = 11

        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "between 2 and 10"):
            checker.analyze_delaunay_guardrails(payload)

    def test_accepts_structural_50k_k10_summary(self) -> None:
        report = checker.analyze_delaunay_guardrails(_k10_summary_payload())

        self.assertEqual(report["point_count"], 50_000)
        self.assertEqual(
            [item["order"] for item in report["surrogate_orders"]],
            list(range(1, 11)),
        )
        self.assertTrue(
            report["guardrail_status"][
                "fail_closed_cross_variant_gate_passed"
            ]
        )

    def test_rejects_direct_count_or_digest_tampering(self) -> None:
        payload = _summary_payload()
        direct = payload["variant_guardrails"]["direct_facet_variants"][0]
        direct["counts"]["guaranteed_no_later_than_deadline_count"] -= 1
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "shared source anchors"):
            checker.analyze_delaunay_guardrails(payload)

        payload = _summary_payload()
        payload["variant_guardrails"]["direct_facet_variants"][0]["decision_sha256"] = SHA256_ZERO
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "decision_sha256"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_surrogate_partition_and_missing_failure_witness(self) -> None:
        payload = _summary_payload()
        payload["variant_guardrails"]["surrogate"]["orders"][0]["counts"]["connected_at_count"] -= 1
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "five-way"):
            checker.analyze_delaunay_guardrails(payload)

        payload = _summary_payload()
        order = payload["variant_guardrails"]["surrogate"]["orders"][0]
        order["counts"]["connected_at_count"] -= 1
        order["counts"]["late_count"] += 1
        order["counts"]["satisfied_count"] -= 1
        order["counts"]["violation_count"] += 1
        order["regular_binary64_guardrail_passed"] = False
        order["fail_closed_guardrail_passed"] = False
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "first_failure_witness presence"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_corrected_postcondition_or_tree_digest_tampering(self) -> None:
        payload = _summary_payload()
        correction = payload["variant_guardrails"]["surrogate"]["orders"][0]["correction"]
        correction["corrected_postcondition_violation_count"] = 1
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "postcondition"):
            checker.analyze_delaunay_guardrails(payload)

        payload = _payload()
        payload["variant_guardrails"]["surrogate"]["orders"][0]["correction"]["corrected_tree"]["tree_sha256"] = SHA256_ZERO
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "corrected_tree.tree_sha256"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_cross_run_anchor_and_exact_architecture_claim(self) -> None:
        payload = _summary_payload()
        payload["variant_guardrails"]["source"]["point_cloud_sha256"] = "f" * 64
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "point cloud"):
            checker.analyze_delaunay_guardrails(payload)

        payload = _summary_payload()
        payload["variant_guardrails"]["surrogate"]["top_k_audit"]["exact_morse_hierarchy_claimed"] = True
        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "exact_morse_hierarchy_claimed"):
            checker.analyze_delaunay_guardrails(payload)

    def test_rejects_massive_summary_recertification_claim(self) -> None:
        payload = _summary_payload()
        payload["independent_record_replay_verified"] = True

        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "without records"):
            checker.analyze_delaunay_guardrails(payload)

    def test_selected_process_gate_matches_fail_closed_default(self) -> None:
        payload = _summary_payload()
        payload["coverage"]["selected_process_gate_passed"] = False

        with self.assertRaisesRegex(checker.DelaunayGuardrailInputError, "selected_process_gate_passed"):
            checker.analyze_delaunay_guardrails(payload)

    def test_conditional_process_exit_follows_selected_gate_without_exact_claim(self) -> None:
        payload = _conditional_summary_payload()
        report = checker.analyze_delaunay_guardrails(payload)

        self.assertEqual(report["source_counts"]["unsupported_ambiguous_count"], 1)
        status = report["guardrail_status"]
        self.assertTrue(status["conditional_cross_variant_gate_passed"])
        self.assertFalse(status["fail_closed_cross_variant_gate_passed"])
        self.assertTrue(status["allow_conditional_guardrail"])
        self.assertTrue(status["selected_process_gate_passed"])
        self.assertEqual(report["public_status"], "not_claimed")
        self.assertFalse(payload["proof_scope"]["exact_Gamma2_claimed"])
        self.assertFalse(
            payload["proof_scope"]["exact_Gabriel_classification_claimed"]
        )

        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8") as stream:
            json.dump(payload, stream)
            stream.flush()
            output = io.StringIO()
            with redirect_stdout(output):
                exit_code = checker.main([stream.name])

        self.assertEqual(exit_code, 0)
        emitted_report = json.loads(output.getvalue())
        self.assertFalse(
            emitted_report["guardrail_status"][
                "fail_closed_cross_variant_gate_passed"
            ]
        )
        self.assertTrue(
            emitted_report["guardrail_status"]["selected_process_gate_passed"]
        )
        self.assertEqual(emitted_report["public_status"], "not_claimed")


if __name__ == "__main__":
    unittest.main()
