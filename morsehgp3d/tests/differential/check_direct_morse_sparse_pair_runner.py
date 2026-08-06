#!/usr/bin/env python3
"""Short product-runner gate through bounded direct--Gamma conformance."""

from __future__ import annotations

import json
import subprocess
import tempfile
import sys
from fractions import Fraction
from math import comb
from pathlib import Path


class ContractError(ValueError):
    """A sparse-pair product-runner invariant was violated."""


DIRECT_GAMMA_PROOF_BASIS = (
    "fresh_direct_forest_replay_and_fresh_bounded_critical_catalog_"
    "reduced_gamma_overlay_then_bidirectional_h0_role_and_atomic_group_"
    "reconciliation_plus_strict_closed_carrier_partition_bijections_at_"
    "every_exhaustive_gamma_activation_level_plus_each_direct_saddle_"
    "arm_exact_coface_incidence_strict_component_and_frozen_carrier_"
    "join_with_latent_iff_isolated_and_resolved_iff_nontrivial_"
    "distinction_with_residual_groups_accepted_only_as_one_root_zero_"
    "point_continuations_v1"
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def nonnegative_integer(value: object, name: str) -> int:
    require(
        type(value) is int and value >= 0,
        f"{name} is not a nonnegative integer",
    )
    return value


def exact_level(value: object, name: str) -> Fraction:
    require(
        isinstance(value, dict)
        and set(value) == {"numerator", "denominator"}
        and isinstance(value.get("numerator"), str)
        and isinstance(value.get("denominator"), str),
        f"{name} is not one canonical exact-level object",
    )
    numerator_text = value["numerator"]
    denominator_text = value["denominator"]
    try:
        numerator = int(numerator_text)
        denominator = int(denominator_text)
    except ValueError as error:
        raise ContractError(f"{name} contains a noninteger exact level") from error
    require(
        denominator > 0
        and str(numerator) == numerator_text
        and str(denominator) == denominator_text,
        f"{name} is not in canonical signed-numerator/positive-denominator form",
    )
    return Fraction(numerator, denominator)


def numeric_tree_is_zero(value: object) -> bool:
    if isinstance(value, dict):
        return bool(value) and all(numeric_tree_is_zero(item) for item in value.values())
    return type(value) is int and value == 0


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
        "build_exact_direct_morse_m1_o5_death_accounting(",
        "verify_exact_direct_morse_m1_o5_death_accounting(",
        "build_exact_direct_morse_gamma_carrier_conformance(",
        "bounded_k2_k1_target_authority_qualification",
        "bounded_m1_o5_death_accounting_qualification",
        "bounded_direct_gamma_carrier_conformance_qualification",
        "bounded_m1_o5_death_accounting_point_count_exceeds_14",
        "bounded_m1_o5_death_accounting_requires_maximum_order_at_least_2",
        "bounded_direct_gamma_carrier_conformance_point_count_exceeds_14",
        "bounded_direct_gamma_carrier_conformance_requires_maximum_order_at_least_2",
        "report.k2_to_k1_target_authority_certified",
        "report.m1_o5_death_accounting_certified",
        "report.direct_morse_gamma_carrier_conformance_certified",
        "report.vertical_target_pipeline_certified &&",
        "report.vertical_journal_certified;",
        'morsehgp3d.direct-morse-product-run.v7',
        '15_bounded_direct_gamma_carrier_group_conformance',
        'builder_includes_fresh_verifier',
        'external_verifier_replay_performed',
        'timing_includes_embedded_fresh_verifier',
        'p7b_replay_performed\\\":false',
    ):
        require(required in runner, f"runner is missing P8n token {required!r}")
    require(
        "verify_exact_direct_morse_gamma_carrier_conformance(" not in runner,
        "runner performed a redundant external direct--Gamma verifier replay",
    )
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
    require(
        "morsehgp3d::direct_morse_m1_o5_death_accounting" in runner_links,
        "runner does not directly link bounded direct M1/O5 accounting",
    )
    require(
        "morsehgp3d::direct_morse_gamma_carrier_conformance" in runner_links,
        "runner does not directly link bounded direct--Gamma conformance",
    )
    digest_equality_regression = (
        'digests.get("direct_cloud")' + " == " + 'digests.get("external_cloud")'
    )
    require(
        digest_equality_regression not in checker,
        "checker equated distinct SHA domains instead of replaying one PointId namespace",
    )


def require_m1_o5_dormant(report: dict[str, object]) -> None:
    accounting = report.get("m1_o5_death_accounting")
    timings = report.get("timings_ms")
    require(isinstance(accounting, dict), "report lost the dormant M1/O5 block")
    require(isinstance(timings, dict), "report lost its timing ledger")
    require(
        report.get("bounded_m1_o5_death_accounting_qualification_requested")
        is False,
        "a non-O5 mode published an O5 qualification request",
    )
    budget = accounting.get("requested_budget")
    receipt = accounting.get("receipt")
    counters = accounting.get("counters")
    verification = accounting.get("verification")
    require(
        accounting.get("required") is False
        and accounting.get("attempted") is False
        and accounting.get("certified_bounded_conditional_m1_o5_accounting")
        is False
        and accounting.get("decision") == 0
        and accounting.get("scope") == 0,
        "a non-O5 mode activated bounded M1/O5 accounting",
    )
    require(
        isinstance(budget, dict) and all(value == 0 for value in budget.values()),
        "a dormant M1/O5 block retained a trusted work budget",
    )
    require(
        isinstance(receipt, dict)
        and receipt.get("point_count") == 0
        and receipt.get("effective_maximum_order") == 0
        and receipt.get("required_logical_output_entries") == 0
        and receipt.get("event_audits") == []
        and receipt.get("group_audits") == []
        and receipt.get("batch_audits") == []
        and all(
            receipt.get(name) == "0" * 64
            for name in (
                "source_canonical_cloud_digest",
                "event_audit_digest",
                "group_audit_digest",
                "batch_audit_digest",
            )
        ),
        "a dormant M1/O5 block retained a scientific receipt",
    )
    require(
        isinstance(counters, dict) and all(value == 0 for value in counters.values()),
        "a dormant M1/O5 block retained counters",
    )
    require(
        isinstance(verification, dict)
        and all(value is False for value in verification.values()),
        "a dormant M1/O5 block retained verifier facts",
    )
    for optional_index in (
        "rejected_source_family_index",
        "rejected_source_binding_index",
        "rejected_atomic_group_index",
        "rejected_source_batch_index",
    ):
        require(
            accounting.get(optional_index) is None,
            f"a dormant M1/O5 block retained {optional_index}",
        )
    for flag, value in accounting.items():
        if isinstance(value, bool) and flag not in (
            "product_non_o5_modes_keep_accounting_dormant",
        ):
            require(value is False, f"a dormant M1/O5 block set {flag}")
    require(
        accounting.get("product_non_o5_modes_keep_accounting_dormant") is True
        and timings.get("m1_o5_death_accounting_build") == 0.0
        and timings.get("m1_o5_death_accounting_verify") == 0.0
        and timings.get("m1_o5_death_accounting") == 0.0,
        "a non-O5 mode did not keep O5 timing and policy dormant",
    )


def require_success_vertical_contract(
    report: dict[str, object], *, authority_required: bool = False
) -> None:
    require_m1_o5_dormant(report)
    require(
        report.get("phase")
        == "15_bounded_direct_gamma_carrier_group_conformance",
        "success report has the wrong direct M1/O5 phase scope",
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
    require_m1_o5_dormant(report)
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
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v7",
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
        # Re-minted 2026-08-06: the shared-LBVH refactor 2ebd8c2 (G4-qualified,
        # same instructed provenance as the reducer singleton wire golden)
        # changed the higher-support traversal structure so the fixed-chunk
        # authority mints 11 prune certificates instead of 1; every downstream
        # scientific projection (terminal events, forest, decisions) is
        # unchanged.
        "prune_certificates": 11,
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
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v7"
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


def require_bounded_m1_o5_accounting_guard(report: dict[str, object]) -> None:
    require(
        report.get("terminal_stage") == "input_preflight"
        and report.get("stop_category") == "invalid_input"
        and report.get("stop_detail")
        == "bounded_m1_o5_death_accounting_point_count_exceeds_14"
        and report.get("canonical_point_count") == 0,
        "bounded M1/O5 mode did not fail closed above n=14",
    )
    accounting = report.get("m1_o5_death_accounting")
    require(
        isinstance(accounting, dict)
        and accounting.get("required") is True
        and accounting.get("attempted") is False
        and accounting.get("certified_bounded_conditional_m1_o5_accounting")
        is False
        and accounting.get("bounded_exhaustive_gamma_oracle_used") is False,
        "bounded M1/O5 guard started accounting or an exhaustive oracle",
    )
    pipeline = report.get("vertical_target_pipeline")
    journal = report.get("vertical_journal")
    diagnostic = report.get("vertical_target_replay_diagnostic")
    authority = report.get("k2_to_k1_target_authority")
    require(
        isinstance(pipeline, dict)
        and pipeline.get("attempted") is False
        and pipeline.get("certified") is False
        and isinstance(journal, dict)
        and journal.get("attempted") is False
        and journal.get("certified_conditional_candidate") is False
        and isinstance(diagnostic, dict)
        and diagnostic.get("attempted") is False
        and isinstance(authority, dict)
        and authority.get("attempted") is False,
        "bounded M1/O5 guard attempted a downstream or oracle stage",
    )


def require_bounded_m1_o5_accounting_contract(report: dict[str, object]) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v7"
        and report.get("phase")
        == "15_bounded_direct_gamma_carrier_group_conformance"
        and report.get("mode")
        == "bounded_m1_o5_death_accounting_qualification"
        and report.get("bounded_m1_o5_death_accounting_qualification_requested")
        is True
        and report.get("bounded_k2_to_k1_target_authority_qualification_requested")
        is False
        and report.get("attempt_kind")
        == "fail_closed_bounded_m1_o5_death_accounting_qualification",
        "bounded M1/O5 run lost its explicit identity",
    )
    require(
        report.get("pipeline_complete") is True
        and report.get("resident_conditional_pipeline_complete") is False
        and report.get("scientific_result_materialized") is True
        and report.get("conditional_h0_candidate_certified") is True
        and report.get("terminal_stage") == "m1_o5_death_accounting"
        and report.get("stop_category") == "none"
        and report.get("stop_detail") == "none"
        and report.get("timing_scope")
        == "attempted_single_process_cpu_generation_to_certified_forest_"
        "and_bounded_fresh_m1_o5_death_accounting",
        "bounded M1/O5 run did not terminate exactly after its fresh verifier",
    )
    require(
        report.get("no_forbidden_global_structure_materialized") is True
        and report.get("no_forbidden_product_path_global_structure_materialized")
        is True
        and report.get("bounded_oracle_gamma_materialized_transiently") is False
        and report.get("bounded_oracle_global_structure_persisted") is False
        and report.get("higher_order_delaunay_materialized") is False,
        "bounded M1/O5 run violated the reduced product architecture",
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
        require(report.get(unclaimed) is False, f"M1/O5 runner overclaimed {unclaimed}")

    vertical_pipeline = report.get("vertical_target_pipeline")
    vertical_journal = report.get("vertical_journal")
    diagnostic = report.get("vertical_target_replay_diagnostic")
    require(
        isinstance(vertical_pipeline, dict)
        and vertical_pipeline.get("attempted") is False
        and vertical_pipeline.get("certified") is False
        and vertical_pipeline.get("decision") == 0
        and vertical_pipeline.get("required_sessions") == 0
        and vertical_pipeline.get("required_groups") == 0
        and vertical_pipeline.get("required_proposals") == 0
        and vertical_pipeline.get("source_group_count_by_order") == []
        and isinstance(vertical_pipeline.get("counters"), dict)
        and all(value == 0 for value in vertical_pipeline["counters"].values()),
        "bounded M1/O5 mode executed the vertical target pipeline",
    )
    require(
        isinstance(vertical_journal, dict)
        and vertical_journal.get("attempted") is False
        and vertical_journal.get("certified_conditional_candidate") is False
        and vertical_journal.get("decision") == 0
        and isinstance(vertical_journal.get("counters"), dict)
        and all(value == 0 for value in vertical_journal["counters"].values()),
        "bounded M1/O5 mode executed the vertical journal",
    )
    require(
        isinstance(diagnostic, dict) and diagnostic.get("attempted") is False,
        "bounded M1/O5 mode executed vertical failure replay",
    )
    k2_authority = report.get("k2_to_k1_target_authority")
    require(
        isinstance(k2_authority, dict)
        and k2_authority.get("required") is False
        and k2_authority.get("attempted") is False
        and k2_authority.get("certified_observed_label_target_authority") is False
        and isinstance(k2_authority.get("oracle"), dict)
        and all(value == 0 for value in k2_authority["oracle"].values())
        and isinstance(k2_authority.get("counters"), dict)
        and all(value == 0 for value in k2_authority["counters"].values())
        and isinstance(k2_authority.get("verification"), dict)
        and all(value is False for value in k2_authority["verification"].values())
        and k2_authority.get("bounded_exhaustive_gamma_oracle_used") is False,
        "bounded M1/O5 mode executed Gamma2/K1 authority work",
    )

    timings = report.get("timings_ms")
    require(isinstance(timings, dict), "M1/O5 run lost its timing ledger")
    require(
        type(timings.get("m1_o5_death_accounting_build")) in (int, float)
        and timings["m1_o5_death_accounting_build"] >= 0
        and type(timings.get("m1_o5_death_accounting_verify")) in (int, float)
        and timings["m1_o5_death_accounting_verify"] >= 0
        and type(timings.get("m1_o5_death_accounting")) in (int, float)
        and timings["m1_o5_death_accounting"] > 0
        and timings["vertical_target_pipeline"] == 0.0
        and timings["vertical_journal"] == 0.0
        and timings["k2_to_k1_oracle_source_history"] == 0.0
        and timings["k2_to_k1_oracle_k1"] == 0.0
        and timings["k2_to_k1_oracle_hierarchy"] == 0.0
        and timings["k2_to_k1_target_authority"] == 0.0,
        "M1/O5 timing ledger is not isolated from vertical and Gamma/K1 work",
    )

    accounting = report.get("m1_o5_death_accounting")
    require(isinstance(accounting, dict), "bounded M1/O5 receipt is absent")
    budget = accounting.get("requested_budget")
    receipt = accounting.get("receipt")
    counters = accounting.get("counters")
    verification = accounting.get("verification")
    require(
        accounting.get("required") is True
        and accounting.get("attempted") is True
        and accounting.get("certified_bounded_conditional_m1_o5_accounting")
        is True
        and accounting.get("schema_version") == 1
        and accounting.get("backend") == "reference_cpu"
        and accounting.get("profile") == "hgp_reduced"
        and accounting.get("mode")
        == "bounded_n14_conditional_direct_equal_level_m1_o5_death_accounting"
        and accounting.get("deployment_status") == "architecture_only"
        and accounting.get("public_status") == "not_claimed"
        and accounting.get("decision") == 15
        and accounting.get("scope") == 1,
        "bounded M1/O5 receipt lost its certified core identity",
    )
    require(
        isinstance(budget, dict)
        and budget
        and all(value == 65536 for value in budget.values()),
        "bounded M1/O5 run did not use its fixed independent n<=14 budget",
    )
    require(
        isinstance(receipt, dict)
        and receipt.get("point_count") == 4
        and receipt.get("effective_maximum_order") == 3
        and isinstance(receipt.get("required_logical_output_entries"), int)
        and receipt["required_logical_output_entries"] > 0,
        "bounded M1/O5 receipt has the wrong source scope",
    )
    for digest_name in (
        "source_canonical_cloud_digest",
        "event_audit_digest",
        "group_audit_digest",
        "batch_audit_digest",
    ):
        digest = receipt.get(digest_name)
        require(
            isinstance(digest, str)
            and len(digest) == 64
            and digest == digest.lower()
            and all(character in "0123456789abcdef" for character in digest)
            and any(character != "0" for character in digest),
            f"bounded M1/O5 receipt lost nonzero digest {digest_name}",
        )
    event_audits = receipt.get("event_audits")
    group_audits = receipt.get("group_audits")
    batch_audits = receipt.get("batch_audits")
    require(
        isinstance(event_audits, list)
        and event_audits
        and isinstance(group_audits, list)
        and group_audits
        and isinstance(batch_audits, list)
        and batch_audits,
        "bounded M1/O5 scientific receipt is empty",
    )
    for index, event in enumerate(event_audits):
        require(isinstance(event, dict), "M1/O5 event audit is not an object")
        support = event.get("canonical_support_point_ids")
        require(
            event.get("event_audit_index") == index
            and isinstance(support, list)
            and len(support) == event.get("support_point_count")
            and event.get("arm_count") == event.get("support_point_count")
            and isinstance(event.get("arm_count"), int)
            and event["arm_count"] >= 2
            and event.get("delta_one") == event["arm_count"] - 1,
            "an M1/O5 event audit broke Delta1=|U|-1",
        )
    for index, group in enumerate(group_audits):
        require(isinstance(group, dict), "M1/O5 group audit is not an object")
        kind = group.get("kind")
        roots = group.get("prior_reduced_root_count")
        deaths = group.get("death_count")
        require(
            group.get("group_audit_index") == index
            and isinstance(roots, int)
            and isinstance(deaths, int)
            and (
                (kind == 0 and roots == 0 and deaths == 0)
                or (kind == 1 and roots == 1 and deaths == 0)
                or (kind == 2 and roots >= 2 and deaths == roots - 1)
            ),
            "an M1/O5 group audit broke its q_R death identity",
        )
    total_local_capacity = 0
    total_touched_roots = 0
    total_positive_groups = 0
    total_deaths = 0
    for index, batch in enumerate(batch_audits):
        require(isinstance(batch, dict), "M1/O5 batch audit is not an object")
        deaths = batch.get("global_death_count")
        capacity = batch.get("local_multiplicity_capacity")
        touched = batch.get("touched_distinct_prior_root_count")
        positive_groups = batch.get("positive_root_group_count")
        require(
            batch.get("batch_audit_index") == index
            and isinstance(deaths, int)
            and isinstance(capacity, int)
            and isinstance(touched, int)
            and isinstance(positive_groups, int)
            and deaths == batch.get("sum_group_death_count")
            and deaths <= capacity
            and touched == deaths + positive_groups,
            "an M1/O5 batch audit broke death accounting or its local bound",
        )
        total_local_capacity += capacity
        total_touched_roots += touched
        total_positive_groups += positive_groups
        total_deaths += deaths
    require(isinstance(counters, dict), "bounded M1/O5 counters are absent")
    require(
        counters.get("event_audits") == len(event_audits)
        and counters.get("group_audits") == len(group_audits)
        and counters.get("batch_audits") == len(batch_audits)
        and counters.get("total_local_multiplicity_capacity")
        == total_local_capacity
        and counters.get("total_touched_distinct_prior_roots")
        == total_touched_roots
        and counters.get("total_positive_root_groups") == total_positive_groups
        and counters.get("total_global_deaths") == total_deaths,
        "bounded M1/O5 aggregate counters do not match the serialized receipt",
    )
    cap_pairs = (
        ("event_audits", "event_audits"),
        ("group_audits", "group_audits"),
        ("batch_audits", "batch_audits"),
        ("source_family_scans", "source_family_scans"),
        ("source_arm_seed_scans", "source_arm_seed_scans"),
        ("source_binding_scans", "source_binding_scans"),
        ("source_saddle_scans", "source_saddle_scans"),
        ("source_group_scans", "source_group_scans"),
        ("source_batch_scans", "source_batch_scans"),
        ("point_id_reference_scans", "point_id_reference_scans"),
        ("prior_root_reference_scans", "prior_root_reference_scans"),
        ("exact_level_comparisons", "exact_level_comparisons"),
        ("peak_logical_scratch_entries", "logical_scratch_entries"),
    )
    for counter_name, budget_name in cap_pairs:
        require(
            isinstance(counters.get(counter_name), int)
            and counters[counter_name] <= budget[budget_name],
            f"bounded M1/O5 counter exceeded {budget_name}",
        )
    require(
        receipt["required_logical_output_entries"]
        <= budget["logical_output_entries"],
        "bounded M1/O5 receipt exceeded its logical-output cap",
    )
    for fact in (
        "budget_preflight_certified",
        "source_event_journal_freshly_replayed_relative_to_facade",
        "source_seed_journal_freshly_replayed_relative_to_facade",
        "source_forest_freshly_replayed_relative_to_facade",
        "conditional_on_caller_fresh_phase9_facade_replay",
        "every_index_one_event_has_complete_shell_arms",
        "every_arm_seed_point_id_and_exact_level_joined",
        "same_prior_root_carriers_unioned_before_saddle_hyperedges",
        "every_saddle_hyperedge_including_latent_closed_transitively",
        "simultaneous_carrier_quotient_reconstructed",
        "quotient_partition_matches_forest_atomic_groups",
        "group_root_counts_kinds_and_children_replayed",
        "group_death_counts_replayed",
        "batch_global_death_identity_replayed",
        "local_index_one_multiplicity_bounds_replayed",
        "m1_o5_combinatorial_accounting_replayed",
    ):
        require(accounting.get(fact) is True, f"bounded M1/O5 lost local fact {fact}")
    require(
        isinstance(verification, dict)
        and all(value is True for value in verification.values()),
        "bounded M1/O5 independent verifier was not completely fresh",
    )
    for unclaimed in (
        "event_local_death_attribution_serialized",
        "durable_carrier_root_or_node_ids_serialized",
        "source_catalog_complete_for_full_pi0",
        "carrier_faithfulness_complete",
        "silent_gamma_group_completeness",
        "bidirectional_gamma_group_completeness",
        "external_target_authority_replayed",
        "global_morse_obligation_replayed",
        "global_m1_claimed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "forest_semantics_exact",
        "bounded_exhaustive_gamma_oracle_used",
        "gamma_cells_or_global_cofaces_materialized",
        "higher_order_delaunay_materialized",
        "public_status_claimed",
        "scalable_50k_claimed",
    ):
        require(accounting.get(unclaimed) is False, f"bounded M1/O5 overclaimed {unclaimed}")


def expected_direct_gamma_budget(point_count: int, order: int) -> dict[str, object]:
    facet_count = comb(point_count, order)
    coface_count = comb(point_count, order + 1)
    activation_count = facet_count + coface_count
    exhaustive_run_count = activation_count + 1
    strict_gamma_union_count = order * coface_count
    critical_candidate_count = sum(
        comb(point_count, support_size)
        for support_size in range(1, min(4, point_count) + 1)
    )
    selected_event_support_count = sum(
        comb(point_count, support_size)
        for support_size in range(2, min(4, order + 1, point_count) + 1)
    )
    event_projection_count = min(
        activation_count, selected_event_support_count
    )
    history_point_id_scan_count = (
        order * facet_count + (order + 1) * coface_count
    )
    catalog_point_id_scan_count = (order + 1) * event_projection_count
    subset_count = 1 << point_count
    forest_reference_count = point_count * (subset_count // 2)
    group_scratch_count = 6 * facet_count
    parent_hop_count = 2 * forest_reference_count * facet_count
    exact_level_integer_bit_count = 8 * (
        sys.float_info.max_exp
        - sys.float_info.min_exp
        + sys.float_info.mant_dig
    )
    carrier_anchor_count = 2 * activation_count * facet_count
    gamma_lookup_count = carrier_anchor_count * facet_count
    group_event_reference_count = facet_count + 2 * coface_count
    arm_binding_count = (order + 1) * coface_count
    group_component_membership_count = (
        arm_binding_count + activation_count
    ) * facet_count
    gamma_label_comparison_count = arm_binding_count * (
        coface_count + order + 1
    )
    return {
        "overlay": {
            "critical_catalog": {
                "candidates": critical_candidate_count,
                "point_classifications": (
                    critical_candidate_count * point_count
                ),
            },
            "reduced_gamma_history": {
                "gamma": {
                    "facets": facet_count,
                    "cofaces": coface_count,
                    "union_attempts": strict_gamma_union_count,
                },
                "activation_levels": activation_count,
                "total_facet_work": exhaustive_run_count * facet_count,
                "total_coface_work": exhaustive_run_count * coface_count,
                "total_union_work": (
                    exhaustive_run_count * strict_gamma_union_count
                ),
                "nodes": coface_count,
                "child_references": coface_count - 1,
                "group_root_references": coface_count - 1,
                "groups": activation_count,
                "group_newly_active_facets": facet_count,
                "group_equal_level_cofaces": coface_count,
                "delta_facets": facet_count,
                "delta_point_references": order * facet_count,
            },
            "event_projections": event_projection_count,
            "group_overlays": activation_count,
            "label_slots": activation_count,
            "history_point_id_scans": history_point_id_scan_count,
            "catalog_point_id_scans": catalog_point_id_scan_count,
            "group_event_references": event_projection_count,
        },
        "carrier_cut": {
            "forest_birth_record_scans": subset_count,
            "forest_node_scans": subset_count,
            "forest_batch_scans": subset_count,
            "forest_atomic_group_scans": subset_count,
            "forest_saddle_scans": subset_count,
            "forest_arm_binding_scans": forest_reference_count,
            "forest_child_reference_scans": forest_reference_count,
            "forest_final_root_scans": subset_count,
            "component_states": subset_count,
            "node_marker_states": subset_count,
            "index_entries": facet_count,
            "group_carrier_scratch": group_scratch_count,
            "group_prior_root_scratch": group_scratch_count,
            "parent_hops": parent_hop_count,
            "exact_level_comparisons": forest_reference_count,
            "single_exact_level_integer_bits": exact_level_integer_bit_count,
            "logical_output_entries": facet_count,
        },
        "direct_role_scans": activation_count,
        "forest_birth_scans": subset_count,
        "forest_saddle_scans": subset_count,
        "forest_atomic_group_scans": subset_count,
        "activation_levels": activation_count,
        "gamma_transition_builds": activation_count,
        "checkpoints": activation_count,
        "group_audits": activation_count,
        "carrier_anchor_scans": carrier_anchor_count,
        "gamma_component_scans": gamma_lookup_count,
        "gamma_facet_scans": gamma_lookup_count,
        "group_event_reference_scans": group_event_reference_count,
        "arm_binding_scans": arm_binding_count,
        "gamma_incidence_joins": arm_binding_count,
        "carrier_cut_entry_lookups": arm_binding_count,
        "group_component_memberships": group_component_membership_count,
        "gamma_label_comparisons": gamma_label_comparison_count,
        "logical_scratch_entries": group_scratch_count,
        "logical_output_entries": 2 * activation_count + 1,
    }


def require_direct_gamma_carrier_inactive(
    report: dict[str, object], *, required: bool
) -> None:
    block = report.get("direct_morse_gamma_carrier_conformance")
    timings = report.get("timings_ms")
    require(
        report.get(
            "bounded_direct_gamma_carrier_conformance_qualification_requested"
        )
        is required,
        "direct--Gamma qualification request disagrees with its dormant/guard state",
    )
    require(isinstance(block, dict), "report lost the direct--Gamma block")
    require(isinstance(timings, dict), "report lost its timing ledger")
    require(
        block.get("required") is required
        and block.get("attempted") is False
        and block.get("certified_bounded_single_order_conformance") is False
        and block.get("schema_version") == 1
        and block.get("backend") == "reference_cpu"
        and block.get("profile") == "hgp_reduced"
        and block.get("mode")
        == "bounded_n14_single_order_direct_gamma_carrier_group_conformance"
        and block.get("deployment_status") == "architecture_only"
        and block.get("public_status") == "not_claimed"
        and block.get("proof_basis") == DIRECT_GAMMA_PROOF_BASIS
        and block.get("decision") == 0
        and block.get("scope") == 0
        and block.get("builder_includes_fresh_verifier") is False
        and block.get("external_verifier_replay_performed") is False
        and block.get("timing_includes_embedded_fresh_verifier") is False,
        "an inactive direct--Gamma block retained execution or certification state",
    )
    budget = block.get("requested_budget")
    receipt = block.get("receipt")
    counters = block.get("counters")
    require(
        isinstance(budget, dict) and numeric_tree_is_zero(budget),
        "an inactive direct--Gamma block retained a trusted work budget",
    )
    require(isinstance(receipt, dict), "inactive direct--Gamma receipt is absent")
    for name, value in receipt.items():
        if name in ("checkpoints", "group_audits"):
            require(value == [], f"inactive direct--Gamma receipt retained {name}")
        else:
            require(
                type(value) is int and value == 0,
                f"inactive direct--Gamma receipt retained {name}",
            )
    require(
        isinstance(counters, dict)
        and counters
        and all(type(value) is int and value == 0 for value in counters.values()),
        "an inactive direct--Gamma block retained counters",
    )
    for fact in (
        "scalar_preflight_certified",
        "source_forest_freshly_replayed_relative_to_facade",
        "critical_catalog_gamma_overlay_freshly_replayed",
        "direct_h0_catalog_roles_bidirectionally_reconciled",
        "direct_atomic_groups_bidirectionally_reconciled",
        "direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components",
        "carrier_partition_bijective_at_every_strict_checkpoint",
        "carrier_partition_bijective_at_every_closed_checkpoint",
        "every_gamma_group_classified_once",
        "every_provenance_free_gamma_group_is_silent_continuation",
        "every_silent_continuation_preserves_one_root_and_adds_zero_points",
        "bounded_carrier_faithfulness_replayed",
        "bounded_silent_gamma_group_completeness_replayed",
        "bounded_bidirectional_gamma_group_completeness_replayed",
        "bounded_exhaustive_gamma_oracle_used",
        "product_sparse_silent_source_complete",
        "global_morse_obligation_replayed",
        "global_m1_claimed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "forest_semantics_exact",
        "scalable_50k_claimed",
        "gamma_cells_or_global_cofaces_persisted",
        "higher_order_delaunay_materialized",
        "public_status_claimed",
        "no_partial_scientific_payload_published_on_failure",
    ):
        require(block.get(fact) is False, f"inactive direct--Gamma block set {fact}")
    require(
        timings.get("direct_morse_gamma_carrier_conformance") == 0.0,
        "inactive direct--Gamma work retained a timing",
    )


def require_direct_gamma_carrier_dormant(report: dict[str, object]) -> None:
    require_direct_gamma_carrier_inactive(report, required=False)


def require_o5_k2_k1_vertical_dormant(report: dict[str, object]) -> None:
    require_vertical_not_attempted(report)
    timings = report.get("timings_ms")
    pipeline = report.get("vertical_target_pipeline")
    journal = report.get("vertical_journal")
    authority = report.get("k2_to_k1_target_authority")
    require(isinstance(timings, dict), "carrier mode lost its timing ledger")
    require(
        isinstance(pipeline, dict)
        and pipeline.get("attempted") is False
        and pipeline.get("certified") is False
        and pipeline.get("decision") == 0
        and pipeline.get("required_sessions") == 0
        and pipeline.get("required_groups") == 0
        and pipeline.get("required_proposals") == 0
        and pipeline.get("source_group_count_by_order") == []
        and isinstance(pipeline.get("counters"), dict)
        and all(value == 0 for value in pipeline["counters"].values()),
        "carrier mode executed or populated the vertical target pipeline",
    )
    require(
        isinstance(journal, dict)
        and journal.get("attempted") is False
        and journal.get("certified_conditional_candidate") is False
        and journal.get("decision") == 0
        and isinstance(journal.get("counters"), dict)
        and all(value == 0 for value in journal["counters"].values()),
        "carrier mode executed or populated the vertical journal",
    )
    require(
        isinstance(authority, dict)
        and authority.get("required") is False
        and authority.get("attempted") is False
        and authority.get("certified_observed_label_target_authority") is False
        and authority.get("decision") == 0
        and authority.get("scope") == 0
        and isinstance(authority.get("oracle"), dict)
        and all(value == 0 for value in authority["oracle"].values())
        and isinstance(authority.get("counters"), dict)
        and all(value == 0 for value in authority["counters"].values())
        and isinstance(authority.get("verification"), dict)
        and all(value is False for value in authority["verification"].values())
        and authority.get("bounded_exhaustive_gamma_oracle_used") is False,
        "carrier mode executed or populated K2-to-K1 target authority work",
    )
    for timing_name in (
        "vertical_target_pipeline",
        "vertical_journal",
        "vertical_target_replay_diagnostic",
        "k2_to_k1_oracle_source_history",
        "k2_to_k1_oracle_k1",
        "k2_to_k1_oracle_hierarchy",
        "k2_to_k1_target_authority_build",
        "k2_to_k1_target_authority_verify",
        "k2_to_k1_target_authority",
        "m1_o5_death_accounting_build",
        "m1_o5_death_accounting_verify",
        "m1_o5_death_accounting",
    ):
        require(
            timings.get(timing_name) == 0.0,
            f"carrier mode retained dormant timing {timing_name}",
        )


def require_bounded_direct_gamma_carrier_guard(report: dict[str, object]) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v7"
        and report.get("phase")
        == "15_bounded_direct_gamma_carrier_group_conformance"
        and report.get("mode")
        == "bounded_direct_gamma_carrier_conformance_qualification"
        and report.get("terminal_stage") == "input_preflight"
        and report.get("stop_category") == "invalid_input"
        and report.get("stop_detail")
        == "bounded_direct_gamma_carrier_conformance_point_count_exceeds_14"
        and report.get("canonical_point_count") == 0,
        "bounded direct--Gamma mode did not fail closed above n=14",
    )
    require_direct_gamma_carrier_inactive(report, required=True)
    require_o5_k2_k1_vertical_dormant(report)


def require_bounded_direct_gamma_carrier_contract(
    report: dict[str, object]
) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v7"
        and report.get("phase")
        == "15_bounded_direct_gamma_carrier_group_conformance"
        and report.get("mode")
        == "bounded_direct_gamma_carrier_conformance_qualification"
        and report.get("family") == "uniform_latin"
        and report.get("point_count") == 3
        and report.get("canonical_point_count") == 3
        and report.get("requested_maximum_order") == 2
        and report.get("effective_maximum_order") == 2
        and report.get(
            "bounded_direct_gamma_carrier_conformance_qualification_requested"
        )
        is True
        and report.get(
            "bounded_k2_to_k1_target_authority_qualification_requested"
        )
        is False
        and report.get("bounded_m1_o5_death_accounting_qualification_requested")
        is False
        and report.get("complete_hierarchy_attempt_requested") is False
        and report.get("attempt_kind")
        == "fail_closed_bounded_direct_gamma_carrier_conformance_qualification",
        "bounded direct--Gamma run lost its explicit n=3, K=2 identity",
    )
    require(
        report.get("pipeline_complete") is True
        and report.get("resident_conditional_pipeline_complete") is False
        and report.get("scientific_result_materialized") is True
        and report.get("conditional_h0_candidate_certified") is True
        and report.get("terminal_stage")
        == "direct_morse_gamma_carrier_conformance"
        and report.get("stop_category") == "none"
        and report.get("stop_detail") == "none"
        and report.get("timing_scope")
        == "attempted_single_process_cpu_generation_to_certified_forest_"
        "and_bounded_fresh_single_order_direct_gamma_carrier_group_"
        "conformance",
        "bounded direct--Gamma run did not terminate exactly at conformance",
    )
    require(
        report.get("no_forbidden_global_structure_materialized") is True
        and report.get(
            "no_forbidden_product_path_global_structure_materialized"
        )
        is True
        and report.get("architecture_audit_scope")
        == "nonbounded_product_path_excluding_explicit_bounded_oracle"
        and report.get("bounded_oracle_gamma_materialized_transiently") is True
        and report.get("bounded_oracle_global_structure_persisted") is False
        and report.get("higher_order_delaunay_materialized") is False,
        "bounded direct--Gamma run blurred transient oracle and product storage",
    )
    for unclaimed in (
        "global_morse_obligation_replayed",
        "k2_to_k1_observed_label_target_authority_replayed",
        "bidirectional_gamma_group_completeness_replayed",
        "silent_gamma_checkpoint_completeness_replayed",
        "external_target_authority_replayed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "global_m1_claimed",
        "product_sparse_silent_source_complete",
        "forest_semantics_exact",
        "product_architecture_claimed",
        "scalable_50k_claimed",
        "warm_e2e_protocol_executed",
        "warm_e2e_slo_claimed",
        "qualification_claimed",
    ):
        require(report.get(unclaimed) is False, f"carrier runner overclaimed {unclaimed}")
    require_o5_k2_k1_vertical_dormant(report)

    timings = report.get("timings_ms")
    require(isinstance(timings, dict), "carrier run lost its timing ledger")
    conformance_timing = timings.get("direct_morse_gamma_carrier_conformance")
    total_timing = timings.get("total")
    require(
        type(conformance_timing) in (int, float)
        and conformance_timing > 0
        and type(total_timing) in (int, float)
        and total_timing >= conformance_timing,
        "carrier timing does not include its embedded fresh verifier",
    )

    block = report.get("direct_morse_gamma_carrier_conformance")
    require(isinstance(block, dict), "bounded direct--Gamma receipt is absent")
    require(
        block.get("required") is True
        and block.get("attempted") is True
        and block.get("certified_bounded_single_order_conformance") is True
        and block.get("schema_version") == 1
        and block.get("backend") == "reference_cpu"
        and block.get("profile") == "hgp_reduced"
        and block.get("mode")
        == "bounded_n14_single_order_direct_gamma_carrier_group_conformance"
        and block.get("deployment_status") == "architecture_only"
        and block.get("public_status") == "not_claimed"
        and block.get("proof_basis") == DIRECT_GAMMA_PROOF_BASIS
        and block.get("decision") == 13
        and block.get("scope") == 1
        and block.get("builder_includes_fresh_verifier") is True
        and block.get("external_verifier_replay_performed") is False
        and block.get("timing_includes_embedded_fresh_verifier") is True,
        "bounded direct--Gamma receipt lost its certified core identity",
    )
    budget = block.get("requested_budget")
    receipt = block.get("receipt")
    counters = block.get("counters")
    require(
        budget == expected_direct_gamma_budget(3, 2),
        "bounded direct--Gamma budget is not the outcome-independent n=3, K=2 envelope",
    )
    require(isinstance(receipt, dict), "direct--Gamma scientific receipt is absent")
    require(isinstance(counters, dict), "direct--Gamma counters are absent")

    facet_count = comb(3, 2)
    coface_count = comb(3, 3)
    activation_bound = facet_count + coface_count
    checkpoints = receipt.get("checkpoints")
    group_audits = receipt.get("group_audits")
    require(
        receipt.get("point_count") == 3
        and receipt.get("order") == 2
        and receipt.get("exhaustive_facet_count") == facet_count
        and receipt.get("exhaustive_coface_count") == coface_count
        and isinstance(checkpoints, list)
        and checkpoints
        and isinstance(group_audits, list)
        and group_audits,
        "direct--Gamma receipt has an empty or wrong exhaustive source scope",
    )

    classification_counts = [0, 0, 0, 0]
    birth_role_count = 0
    saddle_role_count = 0
    arm_binding_count = 0
    strict_component_count = 0
    groups_by_level: dict[Fraction, list[int]] = {}
    previous_group_level: Fraction | None = None
    for index, group in enumerate(group_audits):
        require(isinstance(group, dict), "direct--Gamma group audit is not an object")
        level = exact_level(group.get("squared_level"), f"group_audits[{index}].squared_level")
        require(
            previous_group_level is None or previous_group_level <= level,
            "direct--Gamma group audits are not in level order",
        )
        previous_group_level = level
        groups_by_level.setdefault(level, []).append(index)
        require(
            group.get("group_audit_index") == index,
            "direct--Gamma group audit indices are not canonical",
        )
        numeric_names = (
            "gamma_kind",
            "classification",
            "direct_birth_role_count",
            "direct_saddle_role_count",
            "residual_newly_active_facet_count",
            "residual_equal_level_coface_count",
            "gamma_prior_root_count",
            "delta_facet_count",
            "delta_point_count",
            "direct_arm_binding_count",
            "strict_gamma_component_count",
            "latent_strict_gamma_component_count",
            "resolved_strict_gamma_component_count",
        )
        values = {
            name: nonnegative_integer(group.get(name), f"group_audits[{index}].{name}")
            for name in numeric_names
        }
        kind = values["gamma_kind"]
        classification = values["classification"]
        require(kind in range(4), "direct--Gamma group retained an unknown Gamma kind")
        require(
            classification in range(4),
            "direct--Gamma group retained an unknown classification",
        )
        prior_roots = values["gamma_prior_root_count"]
        require(
            ((kind in (0, 1)) and prior_roots == 0)
            or (kind == 2 and prior_roots == 1)
            or (kind == 3 and prior_roots >= 2),
            "direct--Gamma group kind disagrees with its prior-root count",
        )
        for flag in (
            "coverage_delta_present",
            "fully_redundant_delta",
            "direct_atomic_group_present",
            "direct_arm_strict_component_bijection",
            "silent_one_root_zero_point_continuation",
        ):
            require(
                type(group.get(flag)) is bool,
                f"direct--Gamma group lost Boolean fact {flag}",
            )
        direct_prior = group.get("direct_prior_root_count")
        require(
            direct_prior is None
            or (type(direct_prior) is int and direct_prior >= 0),
            "direct--Gamma group has an invalid direct prior-root count",
        )
        coverage = group["coverage_delta_present"]
        delta_facets = values["delta_facet_count"]
        delta_points = values["delta_point_count"]
        fully_redundant = group["fully_redundant_delta"]
        require(
            coverage == (kind != 0)
            and delta_facets <= facet_count
            and delta_points <= 3
            and (
                (not coverage and delta_facets == 0 and delta_points == 0 and not fully_redundant)
                or (
                    coverage
                    and fully_redundant
                    == (delta_facets == 0 and delta_points == 0)
                )
            ),
            "direct--Gamma coverage delta is not canonical",
        )
        births = values["direct_birth_role_count"]
        saddles = values["direct_saddle_role_count"]
        residual = (
            values["residual_newly_active_facet_count"]
            + values["residual_equal_level_coface_count"]
        )
        has_saddle = saddles != 0
        if has_saddle:
            require(
                group["direct_atomic_group_present"] is True
                and group["direct_arm_strict_component_bijection"] is True
                and values["direct_arm_binding_count"] > 0
                and values["strict_gamma_component_count"] > 0
                and values["latent_strict_gamma_component_count"]
                + values["resolved_strict_gamma_component_count"]
                == values["strict_gamma_component_count"]
                and values["resolved_strict_gamma_component_count"]
                == prior_roots,
                "direct saddle arms did not biject frozen carriers with strict Gamma components",
            )
        else:
            require(
                values["direct_arm_binding_count"] == 0
                and values["strict_gamma_component_count"] == 0
                and values["latent_strict_gamma_component_count"] == 0
                and values["resolved_strict_gamma_component_count"] == 0
                and group["direct_arm_strict_component_bijection"] is False,
                "a nonsaddle Gamma group retained arm--component evidence",
            )
        if classification == 0:
            require(
                births > 0
                and saddles == 0
                and residual == 0
                and kind == 0
                and group["direct_atomic_group_present"] is False
                and direct_prior is None
                and coverage is False
                and group["silent_one_root_zero_point_continuation"] is False,
                "direct birth-carrier classification is inconsistent",
            )
        elif classification == 1:
            require(
                births == 0
                and saddles > 0
                and residual == 0
                and kind != 0
                and group["direct_atomic_group_present"] is True
                and direct_prior == prior_roots
                and group["silent_one_root_zero_point_continuation"] is False,
                "direct saddle-group classification is inconsistent",
            )
        elif classification == 2:
            require(
                (births > 0) != (saddles > 0)
                and residual > 0
                and group["silent_one_root_zero_point_continuation"] is False,
                "mixed direct/residual classification is inconsistent",
            )
            require(
                (
                    saddles > 0
                    and group["direct_atomic_group_present"] is True
                    and direct_prior == prior_roots
                )
                or (
                    births > 0
                    and group["direct_atomic_group_present"] is False
                    and direct_prior is None
                ),
                "mixed direct/residual group has inconsistent direct provenance",
            )
        else:
            require(
                births == 0
                and saddles == 0
                and residual > 0
                and kind == 2
                and prior_roots == 1
                and coverage is True
                and group["direct_atomic_group_present"] is False
                and direct_prior is None
                and delta_points == 0
                and group["silent_one_root_zero_point_continuation"] is True,
                "silent residual continuation classification is inconsistent",
            )
        classification_counts[classification] += 1
        birth_role_count += births
        saddle_role_count += saddles
        arm_binding_count += values["direct_arm_binding_count"]
        strict_component_count += values["strict_gamma_component_count"]

    silent_group_count_from_checkpoints = 0
    silent_checkpoint_count = 0
    previous_checkpoint_level: Fraction | None = None
    for index, checkpoint in enumerate(checkpoints):
        require(isinstance(checkpoint, dict), "direct--Gamma checkpoint is not an object")
        level = exact_level(
            checkpoint.get("squared_level"),
            f"checkpoints[{index}].squared_level",
        )
        require(
            checkpoint.get("activation_level_index") == index
            and (previous_checkpoint_level is None or previous_checkpoint_level < level),
            "direct--Gamma checkpoints are not canonically indexed and ordered",
        )
        previous_checkpoint_level = level
        level_groups = groups_by_level.pop(level, [])
        require(level_groups, "an exhaustive activation checkpoint has no Gamma group")
        silent_at_level = sum(
            group_audits[group_index].get("classification") == 3
            for group_index in level_groups
        )
        direct_at_level = any(
            group_audits[group_index].get("classification") != 3
            for group_index in level_groups
        )
        numeric_names = (
            "strict_direct_carrier_count",
            "strict_gamma_component_count",
            "strict_birth_anchor_count",
            "closed_direct_carrier_count",
            "closed_gamma_component_count",
            "closed_birth_anchor_count",
            "gamma_group_count",
            "silent_gamma_group_count",
        )
        values = {
            name: nonnegative_integer(
                checkpoint.get(name), f"checkpoints[{index}].{name}"
            )
            for name in numeric_names
        }
        require(
            checkpoint.get("strict_partition_bijective") is True
            and checkpoint.get("closed_partition_bijective") is True
            and checkpoint.get("direct_batch_present") is direct_at_level
            and values["strict_direct_carrier_count"]
            == values["strict_gamma_component_count"]
            and values["closed_direct_carrier_count"]
            == values["closed_gamma_component_count"]
            and values["strict_birth_anchor_count"]
            >= values["strict_direct_carrier_count"]
            and values["closed_birth_anchor_count"]
            >= values["closed_direct_carrier_count"]
            and values["gamma_group_count"] == len(level_groups)
            and values["silent_gamma_group_count"] == silent_at_level,
            "direct--Gamma checkpoint partition or group accounting is inconsistent",
        )
        silent_group_count_from_checkpoints += silent_at_level
        silent_checkpoint_count += silent_at_level != 0
    require(
        not groups_by_level
        and sum(
            nonnegative_integer(
                checkpoint.get("gamma_group_count"), "checkpoint.gamma_group_count"
            )
            for checkpoint in checkpoints
        )
        == len(group_audits),
        "checkpoints do not partition every serialized Gamma group exactly once",
    )

    def counter(name: str) -> int:
        return nonnegative_integer(counters.get(name), f"counters.{name}")

    require(
        counter("preflights") == 1
        and counter("direct_forest_verifications") == 1
        and counter("overlay_builds") == 1
        and counter("direct_role_scans") == birth_role_count + saddle_role_count
        and counter("direct_birth_roles") == birth_role_count
        and counter("direct_saddle_roles") == saddle_role_count
        and counter("forest_saddle_scans") == saddle_role_count
        and counter("group_event_reference_scans") == saddle_role_count,
        "direct--Gamma source replay counters do not close",
    )
    require(
        counter("activation_levels") == len(checkpoints)
        and counter("gamma_transition_builds") == len(checkpoints)
        and counter("carrier_cut_builds") == len(checkpoints)
        and counter("checkpoints") == len(checkpoints)
        and counter("strict_partition_bijections") == len(checkpoints)
        and counter("closed_partition_bijections") == len(checkpoints)
        and counter("group_audits") == len(group_audits),
        "direct--Gamma checkpoint replay counters do not close",
    )
    require(
        counter("direct_birth_groups") == classification_counts[0]
        and counter("direct_saddle_groups") == classification_counts[1]
        and counter("mixed_direct_residual_groups") == classification_counts[2]
        and counter("silent_residual_continuation_groups")
        == classification_counts[3]
        and counter("silent_residual_continuation_groups")
        == silent_group_count_from_checkpoints
        and counter("silent_checkpoints") == silent_checkpoint_count,
        "direct--Gamma group-classification counters do not close",
    )
    require(
        saddle_role_count > 0
        and arm_binding_count > 0
        and counter("arm_binding_scans") == arm_binding_count
        and counter("gamma_incidence_joins") == arm_binding_count
        and counter("carrier_cut_entry_lookups") == arm_binding_count
        and counter("group_component_memberships")
        >= arm_binding_count + strict_component_count,
        "n=3, K=2 did not exercise every exact arm/incidence/carrier join",
    )

    expected_required = {
        "required_direct_role_scan_capacity": birth_role_count + saddle_role_count,
        "required_forest_birth_scan_capacity": counter("forest_birth_scans"),
        "required_forest_atomic_group_scan_capacity": counter(
            "forest_atomic_group_scans"
        ),
        "required_activation_level_capacity": activation_bound,
        "required_carrier_anchor_scan_capacity": (
            2 * activation_bound * birth_role_count
        ),
        "required_gamma_component_scan_capacity": (
            2 * activation_bound * birth_role_count * facet_count
        ),
        "required_gamma_facet_scan_capacity": (
            2 * activation_bound * birth_role_count * facet_count
        ),
        "required_group_event_reference_scan_capacity": (
            birth_role_count + 2 * saddle_role_count
        ),
        "required_arm_binding_scan_capacity": arm_binding_count,
        "required_gamma_incidence_join_capacity": arm_binding_count,
        "required_carrier_cut_entry_lookup_capacity": arm_binding_count,
        "required_group_component_membership_capacity": (
            (arm_binding_count + activation_bound) * facet_count
        ),
        "required_gamma_label_comparison_capacity": (
            arm_binding_count * (coface_count + 2 + 1)
        ),
        "required_logical_scratch_entries": (
            3 * birth_role_count + 3 * facet_count
        ),
        "required_checkpoint_capacity": activation_bound,
        "required_group_audit_capacity": activation_bound,
        "required_logical_output_entries": 2 * activation_bound + 1,
    }
    for name, expected in expected_required.items():
        require(
            receipt.get(name) == expected,
            f"direct--Gamma receipt has the wrong combinatorial capacity {name}",
        )
    require(
        nonnegative_integer(
            receipt.get("required_forest_saddle_scan_capacity"),
            "receipt.required_forest_saddle_scan_capacity",
        )
        >= counter("forest_saddle_scans"),
        "direct--Gamma forest saddle preflight undercounts its scan",
    )
    required_to_budget = {
        "required_direct_role_scan_capacity": "direct_role_scans",
        "required_forest_birth_scan_capacity": "forest_birth_scans",
        "required_forest_saddle_scan_capacity": "forest_saddle_scans",
        "required_forest_atomic_group_scan_capacity": "forest_atomic_group_scans",
        "required_activation_level_capacity": "activation_levels",
        "required_carrier_anchor_scan_capacity": "carrier_anchor_scans",
        "required_gamma_component_scan_capacity": "gamma_component_scans",
        "required_gamma_facet_scan_capacity": "gamma_facet_scans",
        "required_group_event_reference_scan_capacity": "group_event_reference_scans",
        "required_arm_binding_scan_capacity": "arm_binding_scans",
        "required_gamma_incidence_join_capacity": "gamma_incidence_joins",
        "required_carrier_cut_entry_lookup_capacity": "carrier_cut_entry_lookups",
        "required_group_component_membership_capacity": "group_component_memberships",
        "required_gamma_label_comparison_capacity": "gamma_label_comparisons",
        "required_logical_scratch_entries": "logical_scratch_entries",
        "required_checkpoint_capacity": "checkpoints",
        "required_group_audit_capacity": "group_audits",
        "required_logical_output_entries": "logical_output_entries",
    }
    for required_name, budget_name in required_to_budget.items():
        require(
            nonnegative_integer(receipt.get(required_name), f"receipt.{required_name}")
            <= nonnegative_integer(budget.get(budget_name), f"budget.{budget_name}"),
            f"direct--Gamma receipt exceeded budget {budget_name}",
        )
    for counter_name, budget_name in (
        ("direct_role_scans", "direct_role_scans"),
        ("forest_birth_scans", "forest_birth_scans"),
        ("forest_saddle_scans", "forest_saddle_scans"),
        ("forest_atomic_group_scans", "forest_atomic_group_scans"),
        ("activation_levels", "activation_levels"),
        ("gamma_transition_builds", "gamma_transition_builds"),
        ("checkpoints", "checkpoints"),
        ("group_audits", "group_audits"),
        ("carrier_anchor_scans", "carrier_anchor_scans"),
        ("gamma_component_scans", "gamma_component_scans"),
        ("gamma_facet_scans", "gamma_facet_scans"),
        ("group_event_reference_scans", "group_event_reference_scans"),
        ("arm_binding_scans", "arm_binding_scans"),
        ("gamma_incidence_joins", "gamma_incidence_joins"),
        ("carrier_cut_entry_lookups", "carrier_cut_entry_lookups"),
        ("group_component_memberships", "group_component_memberships"),
        ("gamma_label_comparisons", "gamma_label_comparisons"),
        ("maximum_logical_scratch_entries", "logical_scratch_entries"),
    ):
        require(
            counter(counter_name)
            <= nonnegative_integer(budget.get(budget_name), f"budget.{budget_name}"),
            f"direct--Gamma counter exceeded budget {budget_name}",
        )

    for fact in (
        "scalar_preflight_certified",
        "source_forest_freshly_replayed_relative_to_facade",
        "critical_catalog_gamma_overlay_freshly_replayed",
        "direct_h0_catalog_roles_bidirectionally_reconciled",
        "direct_atomic_groups_bidirectionally_reconciled",
        "direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components",
        "carrier_partition_bijective_at_every_strict_checkpoint",
        "carrier_partition_bijective_at_every_closed_checkpoint",
        "every_gamma_group_classified_once",
        "every_provenance_free_gamma_group_is_silent_continuation",
        "every_silent_continuation_preserves_one_root_and_adds_zero_points",
        "bounded_carrier_faithfulness_replayed",
        "bounded_silent_gamma_group_completeness_replayed",
        "bounded_bidirectional_gamma_group_completeness_replayed",
        "bounded_exhaustive_gamma_oracle_used",
        "no_partial_scientific_payload_published_on_failure",
    ):
        require(block.get(fact) is True, f"bounded direct--Gamma lost local fact {fact}")
    for unclaimed in (
        "product_sparse_silent_source_complete",
        "global_morse_obligation_replayed",
        "global_m1_claimed",
        "all_naturality_squares_replayed",
        "vertical_maps_complete",
        "forest_semantics_exact",
        "scalable_50k_claimed",
        "gamma_cells_or_global_cofaces_persisted",
        "higher_order_delaunay_materialized",
        "public_status_claimed",
    ):
        require(block.get(unclaimed) is False, f"bounded direct--Gamma overclaimed {unclaimed}")


def main() -> int:
    require(len(sys.argv) == 3, "usage: check_direct_morse_sparse_pair_runner.py PROJECT RUNNER")
    project = Path(sys.argv[1]).resolve()
    binary = Path(sys.argv[2]).resolve()
    require(project.is_dir(), f"project directory does not exist: {project}")
    require(binary.is_file(), f"runner does not exist: {binary}")
    require_static_contract(project)

    success = run_case(binary, ("--point-count", "5", "--K", "4"), 0)
    require_success_projection(success)
    with tempfile.TemporaryDirectory(prefix="mh3d-durable-") as archive_dir:
        durable = run_case(
            binary,
            (
                "--point-count",
                "5",
                "--K",
                "4",
                "--mode",
                "durable_archive_recertification",
                "--archive-directory",
                archive_dir,
            ),
            0,
        )
        require(
            durable.get("terminal_stage") == "durable_archive_recertification"
            and durable.get("pipeline_complete") is True,
            "durable recertification did not complete",
        )
        archive = durable.get("durable_archive")
        require(isinstance(archive, dict), "durable projection missing")
        require(
            archive.get("requested") is True
            and archive.get("published") is True
            and archive.get("identity_recertified") is True
            and archive.get("segments") == 26
            and archive.get("reloaded_segments") == 26
            and archive.get("archive_digest") not in (None, "", "unavailable"),
            "durable archive projection changed",
        )
        forest = durable.get("forest")
        require(
            isinstance(forest, dict)
            and forest.get("birth_records") == 17
            and forest.get("saddles") == 13
            and forest.get("final_roots") == 4
            and forest.get("logical_output_entries") == 222,
            "durable seal forest projection changed",
        )
        second_writer = run_case(
            binary,
            (
                "--point-count",
                "5",
                "--K",
                "4",
                "--mode",
                "durable_archive_recertification",
                "--archive-directory",
                archive_dir,
            ),
            3,
        )
        require(
            second_writer.get("terminal_stage") == "durable_archive_open",
            "a second durable writer must be refused on the published "
            "namespace",
        )
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
    bounded_m1_o5_accounting = run_case(
        binary,
        (
            "--point-count",
            "4",
            "--K",
            "3",
            "--mode",
            "bounded_m1_o5_death_accounting_qualification",
        ),
        0,
        timeout_seconds=120,
    )
    require_bounded_m1_o5_accounting_contract(bounded_m1_o5_accounting)
    bounded_m1_o5_accounting_guard = run_case(
        binary,
        (
            "--point-count",
            "15",
            "--K",
            "10",
            "--mode",
            "bounded_m1_o5_death_accounting_qualification",
        ),
        4,
    )
    require_bounded_m1_o5_accounting_guard(bounded_m1_o5_accounting_guard)
    for historical_report in (
        success,
        capacity,
        resident_guard,
        complete_diagnostic,
        bounded_target_authority,
        bounded_target_authority_guard,
        bounded_m1_o5_accounting,
        bounded_m1_o5_accounting_guard,
    ):
        require_direct_gamma_carrier_dormant(historical_report)

    bounded_direct_gamma_carrier = run_case(
        binary,
        (
            "--point-count",
            "3",
            "--K",
            "2",
            "--family",
            "uniform_latin",
            "--mode",
            "bounded_direct_gamma_carrier_conformance_qualification",
        ),
        0,
        timeout_seconds=60,
    )
    require_bounded_direct_gamma_carrier_contract(
        bounded_direct_gamma_carrier
    )
    bounded_direct_gamma_carrier_guard = run_case(
        binary,
        (
            "--point-count",
            "15",
            "--K",
            "10",
            "--mode",
            "bounded_direct_gamma_carrier_conformance_qualification",
        ),
        4,
    )
    require_bounded_direct_gamma_carrier_guard(
        bounded_direct_gamma_carrier_guard
    )
    print(
        json.dumps(
            {
                "schema": "morsehgp3d.phase15.vertical_product_runner_gate.v5",
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
                "bounded_m1_o5_death_accounting_required": True,
                "bounded_m1_o5_death_accounting_n14_guarded": True,
                "m1_o5_receipt_freshly_verified": True,
                "m1_o5_vertical_and_gamma_work_dormant": True,
                "bounded_direct_gamma_carrier_conformance_required": True,
                "bounded_direct_gamma_carrier_conformance_n14_guarded": True,
                "direct_gamma_carrier_receipt_freshly_verified": True,
                "direct_gamma_builder_embeds_fresh_verifier": True,
                "direct_gamma_external_verifier_replay_count": 0,
                "direct_gamma_old_modes_dormant": True,
                "direct_gamma_o5_k2k1_and_vertical_work_dormant": True,
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
