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
) -> dict[str, object]:
    completed = subprocess.run(
        [str(binary), *arguments],
        check=False,
        capture_output=True,
        text=True,
        timeout=20,
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
        'morsehgp3d.direct-morse-product-run.v3',
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


def require_success_projection(report: dict[str, object]) -> None:
    require(
        report.get("schema") == "morsehgp3d.direct-morse-product-run.v3",
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
    for unclaimed in (
        "global_morse_obligation_replayed",
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
        == {"maximum_anchors_per_group": 32, "proposed_witness_pool_size": 64},
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
        + audit.get("orientation_checks", 0)
        == 25,
        "P8l directed partition does not close",
    )
    require(
        audit.get("admitted_candidates", 0)
        + audit.get("reverse_or_self_orientation_skips", 0)
        == audit.get("orientation_checks"),
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
    print(
        json.dumps(
            {
                "schema": "morsehgp3d.phase14.sparse_pair_runner_gate.v1",
                "bounded_p7b_projection_matched": True,
                "p7b_default_runner_replay_count": 0,
                "p8l_capacity_stop_typed": True,
                "resident_50001_fail_fast": True,
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
