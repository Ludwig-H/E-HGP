#!/usr/bin/env python3
"""Short P8n product-runner gate against the bounded P7b projection."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


class ContractError(ValueError):
    """A sparse-pair product-runner invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def strict_json_object(text: str) -> dict[str, object]:
    def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            require(key not in result, f"duplicate JSON key {key!r}")
            result[key] = value
        return result

    value = json.loads(text, object_pairs_hook=unique_object)
    require(isinstance(value, dict), "runner output is not one JSON object")
    return value


def parenthesized_block(text: str, marker: str) -> str:
    start = text.find(marker)
    require(start >= 0, f"missing source marker {marker!r}")
    opening = text.find("(", start)
    require(opening >= 0, f"missing opening parenthesis after {marker!r}")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[opening : index + 1]
    raise ContractError(f"unterminated block after {marker!r}")


def run_case(
    binary: Path,
    arguments: tuple[str, ...],
    expected_returncode: int,
    timeout_seconds: int = 20,
) -> dict[str, object]:
    completed = subprocess.run(
        [str(binary), *arguments],
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout_seconds,
    )
    require(
        completed.returncode == expected_returncode,
        f"runner returned {completed.returncode}, expected {expected_returncode}: "
        f"{completed.stderr}",
    )
    require(not completed.stderr, f"runner wrote stderr: {completed.stderr}")
    return strict_json_object(completed.stdout)


def require_static_contract(project: Path) -> None:
    runner = (
        project / "src/tools/direct_morse_product_runner.cpp"
    ).read_text(encoding="utf-8")
    cmake = (project / "CMakeLists.txt").read_text(encoding="utf-8")
    checker = Path(__file__).read_text(encoding="utf-8")
    runner_links = parenthesized_block(
        cmake,
        "target_link_libraries(\n    morsehgp3d_direct_morse_product_runner",
    )

    for forbidden in (
        "build_exact_pair_support_stream(",
        "verify_exact_pair_support_stream(",
        "ExactPairSupportStreamResult",
        "ExactPairSupportStreamBudget",
        "make_pair_budget(",
    ):
        require(forbidden not in runner, f"runner retained P7b token {forbidden!r}")
    for required in (
        "ExactSparseAnchoredPairSession::start(",
        "pair_session.advance(",
        "std::move(pair_session).seal()",
        "std::move(pair_authority)",
        "make_sparse_pair_maximum_closed_rank(options)",
        "make_sparse_pair_advance_budget(options)",
        "make_sparse_pair_total_capacity(options)",
        "build_exact_direct_morse_vertical_target_proposal_pipeline(",
        "build_exact_direct_morse_vertical_journal(",
        "build_exact_direct_morse_k2_k1_target_authority(",
        "verify_exact_direct_morse_k2_k1_target_authority(",
        "bounded_k2_k1_target_authority_qualification",
        "report.k2_to_k1_target_authority_certified",
        "report.vertical_target_pipeline_certified &&",
        "report.vertical_journal_certified;",
        'morsehgp3d.direct-morse-product-run.v5',
        '15_k2_to_k1_observed_label_target_authority',
        'p7b_replay_performed\\\":false',
    ):
        require(required in runner, f"runner is missing P8n token {required!r}")
    require(
        "morsehgp3d::sparse_anchored_pair_session" in runner_links,
        "runner does not directly link the P8l session",
    )
    require(
        "morsehgp3d::pair_support" not in runner_links,
        "runner still directly links the P7b stream",
    )
    require(
        "morsehgp3d::direct_morse_vertical_target_proposal_pipeline"
        in runner_links,
        "runner does not directly link the vertical target pipeline",
    )
    require(
        "morsehgp3d::direct_morse_vertical_journal" in runner_links,
        "runner does not directly link the conditional vertical journal",
    )
    require(
        "morsehgp3d::direct_morse_k2_k1_target_authority" in runner_links,
        "runner does not directly link the bounded K2-to-K1 target authority",
    )
    digest_equality_regression = (
        'digests.get("direct_cloud")' + " == " + 'digests.get("external_cloud")'
    )
    require(
        digest_equality_regression not in checker,
        "checker equated distinct SHA domains instead of replaying one PointId namespace",
    )


def require_success_vertical_contract(
    report: dict[str, object], *, authority_required: bool = False
) -> None:
    require(
        report.get("phase")
        == "15_k2_to_k1_observed_label_target_authority",
        "success report has the wrong observed-label authority phase scope",
    )
    expected_terminal_stage = (
        "k2_to_k1_target_authority" if authority_required else "vertical_journal"
    )
    require(
        report.get("terminal_stage") == expected_terminal_stage
        and report.get("stop_category") == "none"
        and report.get("stop_detail") == "none",
        "success report did not terminate at its required final stage",
    )
    expected_timing_scope = (
        "attempted_single_process_cpu_generation_to_conditional_vertical_"
        "journal_and_bounded_fresh_gamma2_emst_k1_observed_label_target_"
        "authority"
        if authority_required
        else "attempted_single_process_cpu_generation_to_materialized_forest_"
        "and_forest_relative_vertical_target_pipeline_and_conditional_"
        "vertical_journal"
    )
    require(
        report.get("timing_scope") == expected_timing_scope,
        "success report has the wrong timing scope",
    )
    timings = report.get("timings_ms")
    require(isinstance(timings, dict), "success report lost its timing ledger")
    require(
        type(timings.get("vertical_target_pipeline")) in (int, float)
        and timings["vertical_target_pipeline"] > 0
        and type(timings.get("vertical_journal")) in (int, float)
        and timings["vertical_journal"] > 0
        and timings.get("vertical_target_replay_diagnostic") == 0.0,
        "success timing ledger did not separate pipeline, journal and dormant diagnostic",
    )

    authority = report.get("k2_to_k1_target_authority")
    require(isinstance(authority, dict), "success report lost target authority receipt")
    if not authority_required:
        authority_counters = authority.get("counters")
        authority_verification = authority.get("verification")
        require(
            authority.get("required") is False
            and authority.get("attempted") is False
            and authority.get("certified_observed_label_target_authority") is False
            and authority.get(
                "k2_to_k1_observed_label_target_authority_replayed"
            )
            is False
            and authority.get("bounded_exhaustive_gamma_oracle_used") is False
            and isinstance(authority_counters, dict)
            and all(value == 0 for value in authority_counters.values())
            and isinstance(authority_verification, dict)
            and all(value is False for value in authority_verification.values()),
            "a non-qualification mode activated the bounded Gamma2 oracle",
        )

    pipeline = report.get("vertical_target_pipeline")
    require(isinstance(pipeline, dict), "success report lost the vertical pipeline")
    counters = pipeline.get("counters")
    require(isinstance(counters, dict), "vertical pipeline lost its counters")
    required_groups = pipeline.get("required_groups")
    required_proposals = pipeline.get("required_proposals")
    require(
        pipeline.get("attempted") is True
        and pipeline.get("certified") is True
        and isinstance(required_groups, int)
        and required_groups > 0
        and isinstance(required_proposals, int)
        and required_proposals > 0,
        "vertical target pipeline was not a nonempty certified stage",
    )
    require(
        counters.get("executed_plans") == required_groups
        and counters.get("replay_advances") == required_groups
        and counters.get("closure_builds") == required_groups
        and counters.get("proposal_adapters") == required_groups
        and counters.get("unresolved_proposals", 0)
        + counters.get("resolved_proposals", 0)
        == required_proposals,
        "vertical target pipeline partitions do not close",
    )
    require(
        pipeline.get("forest_relative_only") is True
        and pipeline.get("external_target_authority_replayed") is False
        and pipeline.get("vertical_maps_complete") is False
        and pipeline.get("public_status_claimed") is False,
        "vertical target pipeline overclaimed its forest-relative scope",
    )

    journal = report.get("vertical_journal")
    require(isinstance(journal, dict), "success report lost the vertical journal")
    journal_counters = journal.get("counters")
    require(isinstance(journal_counters, dict), "vertical journal lost its counters")
    expected_labels = journal_counters.get("expected_labels")
    unresolved_labels = journal_counters.get("unresolved_labels")
    resolved_labels = journal_counters.get("resolved_labels")
    complete_groups = journal_counters.get("complete_groups")
    partial_groups = journal_counters.get("partial_groups")
    expected_squares = journal_counters.get("expected_elementary_group_squares")
    checked_squares = journal_counters.get("checked_elementary_group_squares")
    unresolved_squares = journal_counters.get(
        "unresolved_elementary_group_squares"
    )
    require(
        journal.get("attempted") is True
        and journal.get("certified_conditional_candidate") is True
        and journal.get("source_forest_shape_replayed") is True
        and journal.get("conditional_on_caller_fresh_source_forest_replay")
        is True,
        "conditional vertical journal was not certified from the fresh forest",
    )
    require(
        expected_labels == required_proposals
        and journal.get("label_resolutions") == expected_labels
        and journal_counters.get("missing_labels") == 0
        and isinstance(unresolved_labels, int)
        and isinstance(resolved_labels, int)
        and unresolved_labels + resolved_labels == expected_labels,
        "conditional vertical journal label partition does not close",
    )
    require(
        isinstance(complete_groups, int)
        and isinstance(partial_groups, int)
        and complete_groups + partial_groups == required_groups
        and journal.get("group_checks") == required_groups,
        "conditional vertical journal group partition does not close",
    )
    require(
        isinstance(expected_squares, int)
        and isinstance(checked_squares, int)
        and isinstance(unresolved_squares, int)
        and checked_squares + unresolved_squares == expected_squares,
        "conditional vertical journal square partition does not close",
    )
    for unclaimed in (
        "external_target_authority_replayed",
        "global_morse_obligation_replayed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "gamma_cells_or_global_cofaces_materialized",
        "higher_order_delaunay_materialized",
        "public_status_claimed",
    ):
        require(journal.get(unclaimed) is False, f"vertical journal overclaimed {unclaimed}")

    diagnostic = report.get("vertical_target_replay_diagnostic")
    require(isinstance(diagnostic, dict), "success report lost diagnostic scope")
    require(
        diagnostic.get("attempted") is False
        and diagnostic.get("callback_invoked") is False
        and diagnostic.get("advance_certified") is False
        and diagnostic.get("source_atomic_group_index") is None
        and diagnostic.get("source_batch_index") is None
        and diagnostic.get("source_keys") == []
        and diagnostic.get("canonical_distinct_target_keys") == []
        and diagnostic.get("contradiction_witness") is None
        and diagnostic.get("global_structure_materialized") is False
        and diagnostic.get("public_status_claimed") is False,
        "success report unexpectedly executed or populated failure replay",
    )


def require_vertical_not_attempted(report: dict[str, object]) -> None:
    pipeline = report.get("vertical_target_pipeline")
    journal = report.get("vertical_journal")
    diagnostic = report.get("vertical_target_replay_diagnostic")
    authority = report.get("k2_to_k1_target_authority")
    require(
        isinstance(pipeline, dict)
        and pipeline.get("attempted") is False
        and pipeline.get("certified") is False,
        "an upstream stop attempted the vertical target pipeline",
    )
    require(
        isinstance(journal, dict)
        and journal.get("attempted") is False
        and journal.get("certified_conditional_candidate") is False,
        "an upstream stop attempted the conditional vertical journal",
    )
    require(
        isinstance(diagnostic, dict) and diagnostic.get("attempted") is False,
        "an upstream stop attempted vertical failure replay",
    )
    require(
        isinstance(authority, dict) and authority.get("attempted") is False,
        "an upstream stop attempted the bounded K2-to-K1 target authority",
    )


def require_success_projection(
    report: dict[str, object], *, authority_required: bool = False
) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v5",
        "success report has the wrong schema",
    )
    require(report.get("pipeline_complete") is True, "pipeline did not close")
    require(
        report.get("resident_conditional_pipeline_complete") is True
        and report.get("scientific_result_materialized") is True
        and report.get("conditional_h0_candidate_certified") is True,
        "success report lost its bounded scientific outcome",
    )
    require(
        report.get("no_forbidden_global_structure_materialized") is True,
        "success report materialized a forbidden global structure",
    )
    require(
        report.get("no_forbidden_product_path_global_structure_materialized")
        is True
        and report.get("architecture_audit_scope")
        == "nonbounded_product_path_excluding_explicit_bounded_oracle"
        and report.get("bounded_oracle_global_structure_persisted") is False
        and report.get("higher_order_delaunay_materialized") is False,
        "success report blurred product-path and bounded-oracle architecture",
    )
    for unclaimed in (
        "global_morse_obligation_replayed",
        "bidirectional_gamma_group_completeness_replayed",
        "silent_gamma_checkpoint_completeness_replayed",
        "external_target_authority_replayed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "global_m1_claimed",
        "product_architecture_claimed",
        "scalable_50k_claimed",
        "warm_e2e_protocol_executed",
        "warm_e2e_slo_claimed",
        "qualification_claimed",
    ):
        require(report.get(unclaimed) is False, f"runner overclaimed {unclaimed}")

    pair = report.get("pair_support")
    require(isinstance(pair, dict), "success report has no pair block")
    require(
        pair.get("source_kind") == "sealed_sparse_anchored_session"
        and pair.get("authority_kind") == "sealed_in_process_terminal_authority"
        and pair.get("p7b_replay_performed") is False
        and pair.get("status") == "complete"
        and pair.get("stop_reason") == "none",
        "success report has the wrong P8l provenance",
    )
    require(pair.get("maximum_closed_rank") == 5, "runner did not use K+1")
    require(
        pair.get("schedule_config")
        == {
            "maximum_anchors_per_group": 32,
            "proposed_witness_pool_size": 64,
            "triangular_block_pair_schedule": True,
            "symmetric_inconclusive_cross_block_splitting": False,
            "prioritize_cross_blocks": True,
            "witness_subtree_first_for_triangular_blocks": False,
            "floating_witness_order_for_triangular_blocks": True,
        },
        "runner changed the bounded P8l schedule",
    )
    advance_budget = pair.get("advance_budget")
    total_capacity = pair.get("total_capacity")
    audit = pair.get("audit")
    require(isinstance(advance_budget, dict), "missing P8l advance budget")
    require(isinstance(total_capacity, dict), "missing P8l total capacity")
    require(isinstance(audit, dict), "missing P8l audit")
    require(
        advance_budget.get("emitted_records") == 1
        and advance_budget.get("emitted_point_id_references") == 6,
        "P8l per-advance output budget is not (1,K+2)",
    )
    for work_axis in (
        "schedule_advances",
        "orientation_checks",
        "grouped_node_visits",
        "grouped_exact_predicates",
        "grouped_logical_signs",
        "classification_node_visits",
    ):
        require(
            advance_budget.get(work_axis) == 20000,
            f"P8l advance axis {work_axis} is not capped by W",
        )
    for work_axis in (
        "schedule_advances",
        "orientation_checks",
        "grouped_node_visits",
        "grouped_exact_predicates",
        "grouped_logical_signs",
        "admitted_candidates",
        "classification_node_visits",
    ):
        require(
            total_capacity.get(work_axis) == 20000,
            f"P8l total axis {work_axis} is not capped by W",
        )
    require(
        total_capacity.get("output_records") == 4096
        and total_capacity.get("output_point_id_references") == 24576,
        "P8l total output capacity is not (R,R*(K+2))",
    )
    require(audit.get("directed_pair_universe") == 25, "wrong directed universe")
    require(
        audit.get("authenticated_pruned_directed_pairs", 0)
        + 2 * audit.get("admitted_candidates", 0)
        + 5
        == 25,
        "P8l directed partition does not close",
    )
    require(
        audit.get("admitted_candidates", 0) == audit.get("orientation_checks")
        and audit.get("reverse_or_self_orientation_skips", 0) == 0,
        "P8l orientation partition does not close",
    )
    require(
        audit.get("classification_terminals") == audit.get("admitted_candidates"),
        "P8l classification partition does not close",
    )
    require(
        audit.get("above_rank", 0) + audit.get("output_records", 0)
        == audit.get("classification_terminals"),
        "P8l output partition does not close",
    )
    require(
        audit.get("accepted_events", 0)
        + audit.get("extra_shell_diagnostics", 0)
        == audit.get("output_records"),
        "P8l record partition does not close",
    )
    require(
        audit.get("maximum_live_candidates", 2) <= 1
        and audit.get("total_capacity_exhaustions") == 0,
        "P8l exceeded its single-candidate state or exhausted capacity",
    )
    for physical_counter in (
        "prepared_groups",
        "completed_groups",
        "grouped_common_node_visits",
        "anchor_subgroup_node_visits",
        "singleton_node_visits",
        "grouped_witness_slots",
        "grouped_inherited_witness_reuses",
        "grouped_exact_predicates",
        "fp64_filtered_negative_predicates",
        "fp64_filtered_positive_predicates",
        "exact_fallback_predicates",
        "floating_witness_order_preparations",
        "floating_witness_score_evaluations",
        "floating_witness_nonfinite_scores",
        "grouped_common_exact_predicates",
        "anchor_subgroup_exact_predicates",
        "singleton_exact_predicates",
        "grouped_strict_witness_discoveries",
        "grouped_diagonal_node_descents",
        "grouped_common_frontiers",
        "delegated_frontier_anchors",
        "prepared_anchor_subgroup_probes",
        "anchor_subgroup_witness_pool_entries",
        "query_facing_fallback_witness_pool_entries",
        "anchor_subgroup_splits",
        "query_subtree_splits",
        "anchor_subgroup_certified_prunes",
        "anchor_subgroup_certified_anchors",
        "prepared_singleton_fallbacks",
        "completed_singleton_fallbacks",
        "singleton_witness_pool_entries",
        "singleton_certified_prunes",
        "maximum_pending_anchor_subgroups",
        "triangular_block_pair_visits",
        "triangular_diagonal_splits",
        "triangular_oversized_anchor_splits",
        "triangular_consumer_query_splits",
        "triangular_self_pairs",
        "triangular_cross_blocks",
        "triangular_certified_cross_blocks",
        "triangular_certified_unordered_pairs",
        "triangular_opened_singleton_cross_blocks",
        "triangular_opened_unordered_pairs",
        "triangular_maximum_pending_block_pairs",
    ):
        require(
            isinstance(audit.get(physical_counter), int)
            and audit.get(physical_counter, -1) >= 0,
            f"P8q audit lost physical counter {physical_counter}",
        )
    require(
        audit.get("prepared_groups", 0) == audit.get("completed_groups", -1)
        and audit.get("completed_groups", 0) >= 1,
        "P8q did not close its prepared Morton groups",
    )
    require(
        audit.get("grouped_node_visits", -1)
        == audit.get("grouped_common_node_visits", 0)
        + audit.get("anchor_subgroup_node_visits", 0)
        + audit.get("singleton_node_visits", 0)
        and audit.get("grouped_exact_predicates", -1)
        == audit.get("grouped_logical_signs", -2)
        and audit.get("grouped_logical_signs", -1)
        == audit.get("grouped_common_exact_predicates", 0)
        + audit.get("anchor_subgroup_exact_predicates", 0)
        + audit.get("singleton_exact_predicates", 0),
        "P8q physical work lanes do not sum to the grouped totals",
    )
    require(
        audit.get("grouped_logical_signs", -1)
        == audit.get("fp64_filtered_negative_predicates", 0)
        + audit.get("fp64_filtered_positive_predicates", 0)
        + audit.get("exact_fallback_predicates", 0),
        "P8u filtered/exact predicate partition does not close",
    )
    require(
        audit.get("floating_witness_order_requested") is True
        and audit.get(
            "floating_witness_order_effective_for_every_prepared_traversal"
        )
        is True
        and audit.get("fp64_filter_partition_certified") is True
        and audit.get("floating_witness_order_preparations", 0) > 0
        and audit.get("floating_witness_score_evaluations", 0)
        >= audit.get("floating_witness_order_preparations", 0)
        and audit.get("floating_witness_nonfinite_scores", 0)
        <= audit.get("floating_witness_score_evaluations", 0),
        "P8u floating proposal work was not reported after traversal",
    )
    require(
        audit.get("triangular_partition_complete") is True
        and audit.get("no_dynamic_dual_tree_or_pair_arena") is True
        and audit.get("triangular_self_pairs") == 5
        and audit.get("triangular_certified_unordered_pairs", 0)
        + audit.get("triangular_opened_unordered_pairs", 0)
        == 10
        and audit.get("triangular_certified_cross_blocks", 0)
        + audit.get("triangular_opened_singleton_cross_blocks", 0)
        <= audit.get("triangular_cross_blocks", 0)
        and audit.get("completed_singleton_fallbacks", -1)
        == audit.get("prepared_singleton_fallbacks", 0)
        and audit.get("triangular_maximum_pending_block_pairs", 0) > 0,
        "P8s triangular partition accounting is impossible",
    )
    require(
        audit.get("anchor_subgroup_witness_pool_entries", 0)
        <= 64 * audit.get("prepared_anchor_subgroup_probes", 0)
        and audit.get("singleton_witness_pool_entries", 0)
        <= 64 * audit.get("prepared_singleton_fallbacks", 0),
        "P8q bounded fallback-pool accounting is impossible",
    )
    require(
        audit.get("query_facing_fallback_witness_pool_entries", 0)
        <= audit.get("anchor_subgroup_witness_pool_entries", 0)
        + audit.get("singleton_witness_pool_entries", 0),
        "P8q query-facing pool count exceeds all fallback proposals",
    )
    require(
        0 <= audit.get("maximum_pending_anchor_subgroups", -1) <= 32,
        "P8q exceeded its fixed pending anchor-subgroup stack",
    )
    require(
        audit.get("singleton_certified_prunes", 0)
        <= audit.get("authenticated_prunes", 0),
        "P8q authenticated fewer prunes than its singleton traversals minted",
    )
    require(
        audit.get("grouped_inherited_witness_reuses", 1)
        <= audit.get("grouped_witness_slots", 0)
        and audit.get("grouped_strict_witness_discoveries", 1)
        <= audit.get("grouped_witness_slots", 0),
        "P8o witness-slot partition is impossible",
    )
    for certificate in (
        "directed_coverage_certified",
        "orientation_partition_certified",
        "classification_partition_certified",
        "output_partition_certified",
        "records_certified",
    ):
        require(audit.get(certificate) is True, f"P8l did not certify {certificate}")
    for obsolete in (
        "work_units",
        "product_visits",
        "resolved_pairs",
        "remaining_pairs",
        "closed_ball_queries",
        "closed_ball_node_visits",
        "logical_point_classifications",
        "rank_strict_witness_subtrees",
        "rank_strict_witness_points",
        "center_cover_preflight_skips",
        "center_cover_attempts",
        "center_cover_pruned_products",
        "center_cover_work_units",
    ):
        require(obsolete not in pair, f"P8n relabelled obsolete P7b field {obsolete}")

    expected_higher = {
        "status": "complete",
        "stop_reason": "none",
        "work_units": 37,
        "product_visits": 37,
        "closed_ball_queries": 3,
        "accepted_events": 3,
        "extra_shell_diagnostics": 0,
        "prune_certificates": 1,
        "chunks": 1,
        "authority_kind": "sealed_anchored_fixed_chunk_run",
        "full_geometry_replay_avoided": True,
    }
    expected_pipeline = {
        "terminal_catalog_certified": True,
        "terminal_events": 13,
        "terminal_extra_shell_diagnostics": 0,
        "event_batches": 26,
        "event_roles": 30,
        "saddle_families": 13,
        "arm_seeds": 29,
        "industrial_chunks": 1,
        "plan_lanes": 13,
        "prepared_tickets": 26,
        "committed_batches": 26,
    }
    expected_decisions = {
        "industrial_plan": 8,
        "batch_plan": 7,
        "last_batch_execution": 9,
        "last_preparation": 5,
        "last_reducer_fold": 8,
        "last_live_commit": 8,
    }
    expected_forest = {
        "birth_records": 17,
        "materialized_birth_records": 12,
        "saddles": 13,
        "atomic_groups": 13,
        "nodes": 12,
        "materialized_nodes": 7,
        "final_roots": 4,
        "logical_output_entries": 222,
        "aggregate_closure_nodes": 30,
        "aggregate_closure_step_calls": 29,
    }
    require(report.get("higher_support") == expected_higher, "P6b projection changed")
    require(report.get("pipeline_counts") == expected_pipeline, "pipeline projection changed")
    require(report.get("decisions") == expected_decisions, "downstream decisions changed")
    require(report.get("forest") == expected_forest, "forest projection changed")
    require_success_vertical_contract(
        report, authority_required=authority_required
    )


def require_capacity_stop(report: dict[str, object]) -> None:
    require(
        report.get("terminal_stage") == "sparse_pair_session"
        and report.get("stop_category") == "budget_exhausted"
        and report.get("stop_detail") == "total_output_record_capacity"
        and report.get("budget_exhausted") is True,
        "record-cap run did not fail with the typed P8l stop",
    )
    pair = report.get("pair_support")
    require(isinstance(pair, dict), "record-cap report has no pair block")
    require(
        pair.get("source_kind") == "sparse_anchored_session"
        and pair.get("authority_kind") == "unsealed_sparse_anchored_session"
        and pair.get("p7b_replay_performed") is False
        and pair.get("status") == "total_capacity_exhausted"
        and pair.get("stop_reason") == "total_output_record_capacity",
        "record-cap report has the wrong P8l provenance",
    )
    total_capacity = pair.get("total_capacity")
    audit = pair.get("audit")
    require(isinstance(total_capacity, dict), "record-cap report lost capacities")
    require(isinstance(audit, dict), "record-cap report lost its audit")
    require(
        total_capacity.get("output_records") == 1
        and total_capacity.get("output_point_id_references") == 6
        and audit.get("output_records") == 1
        and audit.get("total_capacity_exhaustions") == 1
        and audit.get("local_budget_exhaustions") == 0,
        "record-cap accounting is inconsistent",
    )
    for certificate in (
        "directed_coverage_certified",
        "orientation_partition_certified",
        "classification_partition_certified",
        "output_partition_certified",
        "records_certified",
    ):
        require(
            audit.get(certificate) is False,
            f"record-cap run incorrectly certified {certificate}",
        )
    expected_higher = {
        "status": "not_run",
        "stop_reason": "none",
        "work_units": 0,
        "product_visits": 0,
        "closed_ball_queries": 0,
        "accepted_events": 0,
        "extra_shell_diagnostics": 0,
        "prune_certificates": 0,
        "chunks": 0,
        "authority_kind": "not_run",
        "full_geometry_replay_avoided": False,
    }
    require(
        report.get("higher_support") == expected_higher,
        "higher-support ran after a terminal P8l capacity stop",
    )
    require(
        report.get("pipeline_complete") is False
        and report.get("resident_conditional_pipeline_complete") is False
        and report.get("scientific_result_materialized") is False
        and report.get("conditional_h0_candidate_certified") is False,
        "record-cap run published a scientific result",
    )
    pipeline_counts = report.get("pipeline_counts")
    decisions = report.get("decisions")
    forest = report.get("forest")
    require(isinstance(pipeline_counts, dict), "record-cap run lost pipeline counts")
    require(isinstance(decisions, dict), "record-cap run lost decisions")
    require(isinstance(forest, dict), "record-cap run lost forest counts")
    require(
        pipeline_counts.get("terminal_catalog_certified") is False
        and all(
            value == 0
            for key, value in pipeline_counts.items()
            if key != "terminal_catalog_certified"
        ),
        "record-cap run retained terminal or downstream counts",
    )
    require(
        all(value == 0 for value in decisions.values()),
        "record-cap run retained downstream decisions",
    )
    require(
        all(value == 0 for value in forest.values()),
        "record-cap run retained forest output",
    )
    require_vertical_not_attempted(report)


def require_resident_guard(report: dict[str, object]) -> None:
    require(
        report.get("terminal_stage") == "input_preflight"
        and report.get("stop_category") == "invalid_input"
        and report.get("stop_detail") == "resident_timed_point_count_exceeds_50000",
        "50,001-point resident guard changed semantics",
    )
    require(report.get("canonical_point_count") == 0, "resident guard generated the cloud")
    pair = report.get("pair_support")
    require(isinstance(pair, dict) and pair.get("status") == "not_run", "resident guard ran P8l")
    require_vertical_not_attempted(report)


def require_complete_diagnostic_contract(report: dict[str, object]) -> None:
    require(report.get("pipeline_complete") is True, "small complete diagnostic did not close")
    require(
        report.get("mode") == "complete_resident_diagnostic"
        and report.get("complete_hierarchy_attempt_requested") is True
        and report.get("configured_pair_total_caps_disabled") is True
        and report.get("downstream_static_confidence_caps_enabled") is True,
        "complete diagnostic capacity scope is not explicit",
    )
    require(
        report.get("operational_deadline_reached") is False
        and report.get("completion_latency_ms") is not None,
        "small complete diagnostic was unexpectedly censored",
    )
    pair = report.get("pair_support")
    require(isinstance(pair, dict), "small complete diagnostic lost P8l report")
    total_capacity = pair.get("total_capacity")
    require(
        isinstance(total_capacity, dict)
        and isinstance(total_capacity.get("schedule_advances"), int)
        and total_capacity["schedule_advances"] > 2**63,
        "complete diagnostic retained the caller P8l fail-fast cap",
    )
    require_success_vertical_contract(report)


def require_bounded_target_authority_contract(report: dict[str, object]) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v5"
        and report.get("pipeline_complete") is True
        and report.get("resident_conditional_pipeline_complete") is True
        and report.get("scientific_result_materialized") is True
        and report.get("conditional_h0_candidate_certified") is True
        and report.get("no_forbidden_product_path_global_structure_materialized")
        is True,
        "bounded qualification lost its conditional direct pipeline",
    )
    pair = report.get("pair_support")
    higher = report.get("higher_support")
    pipeline_counts = report.get("pipeline_counts")
    forest = report.get("forest")
    require(
        isinstance(pair, dict)
        and pair.get("source_kind") == "sealed_sparse_anchored_session"
        and pair.get("authority_kind") == "sealed_in_process_terminal_authority"
        and pair.get("p7b_replay_performed") is False
        and pair.get("status") == "complete"
        and pair.get("maximum_closed_rank") == 4,
        "bounded qualification lost its direct sparse-pair provenance",
    )
    require(
        isinstance(higher, dict)
        and higher.get("status") == "complete"
        and higher.get("full_geometry_replay_avoided") is True
        and isinstance(pipeline_counts, dict)
        and pipeline_counts.get("terminal_catalog_certified") is True
        and pipeline_counts.get("committed_batches", 0) > 0
        and isinstance(forest, dict)
        and forest.get("nodes", 0) > 0,
        "bounded qualification did not close its direct forest prefix",
    )
    for unclaimed in (
        "global_morse_obligation_replayed",
        "bidirectional_gamma_group_completeness_replayed",
        "silent_gamma_checkpoint_completeness_replayed",
        "external_target_authority_replayed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "global_m1_claimed",
        "product_architecture_claimed",
        "scalable_50k_claimed",
        "qualification_claimed",
    ):
        require(report.get(unclaimed) is False, f"bounded runner overclaimed {unclaimed}")
    require_success_vertical_contract(report, authority_required=True)
    require(
        report.get("mode") == "bounded_k2_k1_target_authority_qualification"
        and report.get(
            "bounded_k2_to_k1_target_authority_qualification_requested"
        )
        is True
        and report.get("complete_hierarchy_attempt_requested") is False
        and report.get("attempt_kind")
        == "fail_closed_bounded_k2_to_k1_target_authority_qualification"
        and report.get("k2_to_k1_observed_label_target_authority_replayed")
        is True
        and report.get("bounded_oracle_gamma_materialized_transiently") is True,
        "bounded qualification mode did not publish its narrow local outcome",
    )
    timings = report.get("timings_ms")
    require(isinstance(timings, dict), "bounded authority lost its timing ledger")
    for name in (
        "k2_to_k1_oracle_source_history",
        "k2_to_k1_oracle_k1",
        "k2_to_k1_oracle_hierarchy",
        "k2_to_k1_target_authority_build",
        "k2_to_k1_target_authority_verify",
        "k2_to_k1_target_authority",
    ):
        require(
            type(timings.get(name)) in (int, float) and timings[name] >= 0,
            f"bounded authority lost timing component {name}",
        )
    require(
        timings["k2_to_k1_target_authority"] > 0,
        "bounded authority aggregate timing is empty",
    )

    authority = report.get("k2_to_k1_target_authority")
    require(isinstance(authority, dict), "bounded authority receipt is absent")
    counters = authority.get("counters")
    verification = authority.get("verification")
    oracle = authority.get("oracle")
    digests = authority.get("digests")
    require(
        authority.get("required") is True
        and authority.get("attempted") is True
        and authority.get("certified_observed_label_target_authority") is True
        and authority.get("decision") == 17
        and authority.get("scope") == 1
        and authority.get("backend") == "reference_cpu"
        and authority.get("profile") == "hgp_reduced"
        and authority.get("public_status") == "not_claimed"
        and authority.get("product_nonbounded_modes_keep_oracle_dormant") is True,
        "bounded authority did not expose the certified core identity",
    )
    require(isinstance(counters, dict), "bounded authority counters are absent")
    observed_labels = counters.get("observed_k2_k1_labels")
    resolved_labels = counters.get("resolved_k2_k1_labels")
    require(
        isinstance(observed_labels, int)
        and observed_labels > 0
        and resolved_labels == observed_labels
        and counters.get("gamma_cut_builds") == observed_labels
        and counters.get("certified_target_coverage_equalities") == observed_labels
        and counters.get("direct_target_point_references")
        == counters.get("external_target_point_references"),
        "bounded authority did not close its observed-label coverage partition",
    )
    require(
        isinstance(oracle, dict)
        and oracle.get("source_batches", 0) > 0
        and oracle.get("source_groups", 0) > 0
        and oracle.get("source_nodes", 0) > 0
        and oracle.get("k1_nodes", 0) > 0
        and oracle.get("external_checkpoints", 0) > 0,
        "bounded authority did not report its fresh Gamma2/K1 oracle",
    )
    require(
        isinstance(digests, dict)
        and all(
            isinstance(value, str)
            and len(value) == 64
            and value == value.lower()
            and all(character in "0123456789abcdef" for character in value)
            and any(character != "0" for character in value)
            for value in digests.values()
        ),
        "bounded authority lost a nonzero canonical digest",
    )
    # The direct higher-support and external phase15-tg2a projections use
    # distinct SHA domains. PointId namespace identity comes from fresh replay
    # over the same cloud and the certified namespace fact below, never from
    # equality of these unrelated digest values.
    for fact in (
        "direct_pipeline_freshly_replayed",
        "direct_vertical_journal_freshly_replayed",
        "external_k2_k1_hierarchy_freshly_replayed",
        "canonical_point_namespace_identity_certified",
        "all_observed_k2_k1_labels_present",
        "all_observed_k2_k1_labels_resolved",
        "every_direct_target_coverage_reconstructed_transiently",
        "every_external_target_coverage_reconstructed_transiently",
        "every_observed_k2_k1_target_coverage_equal",
        "k2_to_k1_observed_label_target_authority_replayed",
        "bounded_exhaustive_gamma_oracle_used",
    ):
        require(authority.get(fact) is True, f"bounded authority lost local fact {fact}")
    require(
        isinstance(verification, dict)
        and all(value is True for value in verification.values()),
        "bounded authority did not pass independent fresh verification",
    )
    for unclaimed in (
        "bidirectional_gamma_group_completeness_replayed",
        "silent_gamma_checkpoint_completeness_replayed",
        "external_target_authority_replayed",
        "global_morse_obligation_replayed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "global_m1_claimed",
        "gamma_cells_or_global_cofaces_persisted",
        "higher_order_delaunay_materialized",
        "public_status_claimed",
    ):
        require(authority.get(unclaimed) is False, f"bounded authority overclaimed {unclaimed}")
    require(
        authority.get("bounded_oracle_gamma_materialized_transiently") is True
        and authority.get("no_partial_scientific_payload_published_on_failure")
        is True,
        "bounded authority blurred transient oracle storage or success semantics",
    )


def require_bounded_target_authority_guard(report: dict[str, object]) -> None:
    require(
        report.get("terminal_stage") == "input_preflight"
        and report.get("stop_category") == "invalid_input"
        and report.get("stop_detail")
        == "bounded_k2_k1_target_authority_point_count_exceeds_14"
        and report.get("canonical_point_count") == 0,
        "bounded target-authority mode did not fail closed above n=14",
    )
    authority = report.get("k2_to_k1_target_authority")
    require(
        isinstance(authority, dict)
        and authority.get("required") is True
        and authority.get("attempted") is False
        and authority.get("bounded_exhaustive_gamma_oracle_used") is False,
        "bounded target-authority guard started the exhaustive oracle",
    )
    require_vertical_not_attempted(report)


def main() -> int:
    require(len(sys.argv) == 3, "usage: check_direct_morse_sparse_pair_runner.py PROJECT RUNNER")
    project = Path(sys.argv[1]).resolve()
    binary = Path(sys.argv[2]).resolve()
    require(project.is_dir(), f"project directory does not exist: {project}")
    require(binary.is_file(), f"runner does not exist: {binary}")
    require_static_contract(project)

    success = run_case(binary, ("--point-count", "5", "--K", "4"), 0)
    require_success_projection(success)
    capacity = run_case(
        binary,
        ("--point-count", "5", "--K", "4", "--support-record-budget", "1"),
        2,
    )
    require_capacity_stop(capacity)
    resident_guard = run_case(
        binary, ("--point-count", "50001", "--K", "10"), 4
    )
    require_resident_guard(resident_guard)
    complete_diagnostic = run_case(
        binary,
        (
            "--point-count",
            "5",
            "--K",
            "4",
            "--mode",
            "complete_resident_diagnostic",
            "--operational-deadline-ms",
            "5000",
        ),
        0,
    )
    require_complete_diagnostic_contract(complete_diagnostic)
    bounded_target_authority = run_case(
        binary,
        (
            "--point-count",
            "4",
            "--K",
            "3",
            "--mode",
            "bounded_k2_k1_target_authority_qualification",
        ),
        0,
        timeout_seconds=60,
    )
    require_bounded_target_authority_contract(bounded_target_authority)
    bounded_target_authority_guard = run_case(
        binary,
        (
            "--point-count",
            "15",
            "--K",
            "10",
            "--mode",
            "bounded_k2_k1_target_authority_qualification",
        ),
        4,
    )
    require_bounded_target_authority_guard(bounded_target_authority_guard)
    print(
        json.dumps(
            {
                "schema": "morsehgp3d.phase15.vertical_product_runner_gate.v3",
                "bounded_p7b_projection_matched": True,
                "p7b_default_runner_replay_count": 0,
                "p8l_capacity_stop_typed": True,
                "resident_50001_fail_fast": True,
                "complete_diagnostic_contract": True,
                "vertical_target_pipeline_required": True,
                "conditional_vertical_journal_required": True,
                "bounded_k2_k1_target_authority_required": True,
                "bounded_k2_k1_target_authority_n14_guarded": True,
                "k2_to_k1_observed_label_target_authority_replayed": True,
                "vertical_nonclaims_preserved": True,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ContractError, OSError, subprocess.SubprocessError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1) from error
