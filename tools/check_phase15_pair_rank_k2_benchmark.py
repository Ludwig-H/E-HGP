#!/usr/bin/env python3
"""Validate the negative Phase 15 K=2/K=10 pair-rank benchmark."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any, NoReturn


RAW_SCHEMA = "morsehgp3d.phase14q.pair_rank_search_component_smoke.v3"
SUMMARY_SCHEMA = "morsehgp3d.phase15.pair_rank_k2_vs_k10_g4.v1"
GIT_SHA = "17f7b04b04c6456a68dc912e6a45079f64b16364"
FAMILY = "uniform_latin"
EXPECTED_BUDGETS = {
    "epoch_count": 64,
    "maximum_auxiliary_frontier_entry_count": 8192,
    "maximum_closed_ball_node_visit_count": 25600000,
    "maximum_closed_ball_query_count": 2048,
    "maximum_emitted_point_id_reference_count": 32768,
    "maximum_emitted_record_count": 2048,
    "maximum_frontier_entry_count": 65536,
    "maximum_work_unit_count": 20000,
    "product_capacity": 64,
    "receipt_capacity": 262144,
    "terminal_capacity": 262144,
    "work_item_capacity": 32768,
}
EXPECTED_TIMINGS = {
    2: [3657193946, 3657747814, 3657006486],
    10: [2256834574, 2258104594, 2257170824],
}
EXPECTED_COUNTS = {
    2: {
        "callback_count": 119,
        "visited_work_item_count": 653664,
        "visit_budget_count": 35523579,
        "input_product_count": 1421,
        "proposed_product_count": 149,
        "keep_certificate_product_count": 1272,
        "output_terminal_count": 324025,
        "requested_product_count": 1421,
        "consumed_proposal_count": 82,
        "consumed_keep_certificate_count": 128,
        "cache_replacement_count": 118,
    },
    10: {
        "callback_count": 73,
        "visited_work_item_count": 407690,
        "visit_budget_count": 22424103,
        "input_product_count": 897,
        "proposed_product_count": 22,
        "keep_certificate_product_count": 875,
        "output_terminal_count": 202386,
        "requested_product_count": 897,
        "consumed_proposal_count": 12,
        "consumed_keep_certificate_count": 106,
        "cache_replacement_count": 72,
    },
}


class ContractError(ValueError):
    """One artifact escaped the closed benchmark-only contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_non_finite(value: str) -> NoReturn:
    fail(f"non-finite JSON value: {value}")


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_non_finite,
        )
    except (OSError, json.JSONDecodeError) as error:
        fail(f"cannot read {path}: {error}")
    require(isinstance(value, dict), f"{path} must contain one JSON object")
    return value


def sha256(path: Path) -> str:
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError as error:
        fail(f"cannot hash {path}: {error}")


def require_true_fields(record: dict[str, Any], fields: tuple[str, ...], path: str) -> None:
    for field in fields:
        require(record.get(field) is True, f"{path}.{field} must be true")


def measurement(raw: dict[str, Any]) -> dict[str, Any]:
    timings = raw["timings_ns"]
    assisted = raw["assisted_first"]
    gpu = assisted["gpu_audit"]
    proposal = assisted["proposal_audit"]
    samples = timings["resident_replays"]
    ordered = sorted(samples)
    return {
        "requested_maximum_order": raw["requested_maximum_order"],
        "point_count": raw["point_count"],
        "component_success": raw["component_success"],
        "historical_reference_cpu_chunk_ns": timings[
            "historical_reference_cpu_chunk"
        ],
        "assisted_first_ns": timings["assisted_first"],
        "resident_replay_ns": samples,
        "resident_min_ns": ordered[0],
        "resident_median_ns": ordered[len(ordered) // 2],
        "resident_max_ns": ordered[-1],
        "required_strict_interior_point_count": gpu[
            "required_strict_interior_point_count"
        ],
        "traversal_backend": gpu["traversal_backend"],
        "callback_count": gpu["callback_count"],
        "traversal_epoch_count": gpu["traversal_epoch_count"],
        "visited_work_item_count": gpu["visited_work_item_count"],
        "visit_budget_count": gpu["visit_budget_count"],
        "input_product_count": gpu["input_product_count"],
        "proposed_product_count": gpu["proposed_product_count"],
        "keep_certificate_product_count": gpu["keep_certificate_product_count"],
        "fallback_product_count": gpu["fallback_product_count"],
        "output_terminal_count": gpu["output_terminal_count"],
        "requested_product_count": proposal["requested_product_count"],
        "consumed_proposal_count": proposal["consumed_proposal_count"],
        "consumed_keep_certificate_count": proposal[
            "consumed_keep_certificate_count"
        ],
        "cache_replacement_count": proposal["cache_replacement_count"],
        "capacity_and_frontier_audit": {
            field: gpu[field]
            for field in (
                "work_item_capacity_exhausted",
                "receipt_capacity_exhausted",
                "terminal_capacity_exhausted",
                "visit_budget_exhausted",
                "epoch_budget_exhausted",
                "every_device_frontier_exhausted",
            )
        },
        "fresh_ephemeral_verification": raw["fresh_ephemeral_verification"],
        "replay_transition_equal": [
            replay["proposed_transition_equal"]
            for replay in raw["resident_replays"]
        ],
    }


def check_raw(raw: dict[str, Any], order: int) -> None:
    require(raw.get("schema") == RAW_SCHEMA, f"K={order}: wrong raw schema")
    require(raw.get("git_sha") == GIT_SHA, f"K={order}: wrong git SHA")
    require(raw.get("family") == FAMILY, f"K={order}: wrong family")
    require(raw.get("point_count") == 12500, f"K={order}: wrong point count")
    require(
        raw.get("requested_maximum_order") == order,
        f"K={order}: wrong requested order",
    )
    require(raw.get("resident_replay_count") == 3, f"K={order}: wrong Q")
    require(raw.get("budgets") == EXPECTED_BUDGETS, f"K={order}: wrong budgets")
    require(raw.get("component_success") is True, f"K={order}: component failed")
    require(raw.get("component_only") is True, f"K={order}: scope escaped")
    require(raw.get("full_pipeline_executed") is False, f"K={order}: false pipeline")
    require(raw.get("warm_e2e_measured") is False, f"K={order}: false warm e2e")
    require(raw.get("qualification_claimed") is False, f"K={order}: qualification")
    require(raw.get("public_status") is None, f"K={order}: public status")

    gpu = raw["assisted_first"]["gpu_audit"]
    proposal = raw["assisted_first"]["proposal_audit"]
    require(
        gpu["required_strict_interior_point_count"] == order,
        f"K={order}: witness threshold did not follow K",
    )
    require(
        gpu["traversal_backend"] == "stackless_product_batch",
        f"K={order}: unexpected traversal backend",
    )
    require(gpu["fallback_product_count"] == 0, f"K={order}: fallback used")
    for field in (
        "work_item_capacity_exhausted",
        "receipt_capacity_exhausted",
        "terminal_capacity_exhausted",
        "visit_budget_exhausted",
        "epoch_budget_exhausted",
    ):
        require(gpu[field] is False, f"K={order}: {field} must be false")
    require_true_fields(
        gpu,
        (
            "every_device_frontier_exhausted",
            "every_snapshot_validated",
            "every_product_record_validated",
            "every_stable_terminal_transcript_validated",
            "every_terminal_antichain_validated",
            "every_keep_coverage_recertified",
            "every_anchor_ball_culling_enabled",
            "every_cpu_recertification_complete",
        ),
        f"K={order}.gpu_audit",
    )
    require(
        proposal["no_forbidden_global_structure_materialized"] is True,
        f"K={order}: forbidden structure",
    )
    require(raw["forbidden_global_structure_materialized"] is False, f"K={order}: forbidden global")
    require(raw["timings_ns"]["resident_replays"] == EXPECTED_TIMINGS[order], f"K={order}: timing drift")
    for field, expected in EXPECTED_COUNTS[order].items():
        source = proposal if field in {
            "requested_product_count",
            "consumed_proposal_count",
            "consumed_keep_certificate_count",
            "cache_replacement_count",
        } else gpu
        require(source[field] == expected, f"K={order}: {field} drift")
    require(
        all(replay["proposed_transition_equal"] is True for replay in raw["resident_replays"]),
        f"K={order}: resident replay mismatch",
    )
    fresh = raw["fresh_ephemeral_verification"]
    require(fresh["durable_wire_v1_claimed"] is False, f"K={order}: durable claim")
    require_true_fields(
        fresh,
        (
            "every_consumed_proposal_replayed_once",
            "every_consumed_keep_certificate_replayed_once",
            "keep_candidate_transition_consistent",
            "no_duplicate_keep_certificate_retained",
            "no_unconsumed_keep_certificate_retained",
            "no_unconsumed_proposal_retained",
            "proposed_transition_verified",
        ),
        f"K={order}.fresh",
    )
    require_true_fields(
        fresh["candidate_chunk_verification"],
        tuple(fresh["candidate_chunk_verification"]),
        f"K={order}.fresh.candidate_chunk_verification",
    )


def check_summary(
    summary: dict[str, Any],
    k2: dict[str, Any],
    k10: dict[str, Any],
    k2_path: Path,
    k10_path: Path,
    mem_json_path: Path,
    memcheck_path: Path,
) -> None:
    require(summary.get("schema") == SUMMARY_SCHEMA, "wrong summary schema")
    require(summary.get("git_sha") == GIT_SHA, "wrong summary git SHA")
    require(summary.get("artifact_role") == "negative_component_benchmark", "wrong role")
    require(summary.get("public_status") == "not_claimed", "wrong public status")
    require(summary.get("qualification_claimed") is False, "false qualification")
    require(summary.get("warm_e2e_measured") is False, "false warm e2e")
    require(summary["input"]["budgets"] == EXPECTED_BUDGETS, "summary budget drift")
    require(summary["measurements"]["k2"] == measurement(k2), "K=2 summary drift")
    require(summary["measurements"]["k10"] == measurement(k10), "K=10 summary drift")
    expected_ratio = (
        summary["measurements"]["k2"]["resident_median_ns"]
        / summary["measurements"]["k10"]["resident_median_ns"]
    )
    require(
        math.isclose(
            summary["comparison"]["k2_over_k10_resident_median_ratio"],
            expected_ratio,
            rel_tol=0.0,
            abs_tol=0.0,
        ),
        "median ratio drift",
    )
    comparison = summary["comparison"]
    require(comparison["k2_under_100ms"] is False, "K=2 speed gate changed")
    require(comparison["k10_under_100ms"] is False, "K=10 speed gate changed")
    require(comparison["fifty_thousand_run_executed"] is False, "false 50k run")
    require(comparison["q31_p95_run_executed"] is False, "false p95 run")
    require(comparison["stop_rule_applied"] is True, "stop rule not applied")
    decision = summary["decision"]
    require(decision["pair_rank_component_path_retained_for_product"] is False, "rejected path retained")
    require(decision["shared_lbvh_architecture_rejected"] is False, "shared LBVH falsely rejected")
    require(decision["extra_shell_status"] == "unresolved_fail_closed", "extra-shell drift")

    raw = summary["raw_artifacts"]
    require(raw["k2_sha256"] == sha256(k2_path), "K=2 raw checksum mismatch")
    require(raw["k10_sha256"] == sha256(k10_path), "K=10 raw checksum mismatch")
    require(raw["memcheck_runner_sha256"] == sha256(mem_json_path), "mem JSON checksum mismatch")
    require(
        summary["compute_sanitizer"]["raw_transcript_sha256"]
        == sha256(memcheck_path),
        "memcheck transcript checksum mismatch",
    )
    transcript = memcheck_path.read_text(encoding="utf-8")
    require(
        transcript.count("========= LEAK SUMMARY: 0 bytes leaked in 0 allocations") == 1,
        "memcheck leak summary missing",
    )
    require(
        transcript.count("========= ERROR SUMMARY: 0 errors") == 1,
        "memcheck error summary missing",
    )
    session = summary["gcp_session"]
    require(session["provisioning_model"] == "SPOT", "GCP was not SPOT")
    require(session["instance_termination_action"] == "STOP", "GCP stop action drift")
    require(session["max_run_duration_seconds"] == 3600, "GCP duration drift")
    require(session["final_status"] == "TERMINATED", "GCP final status drift")
    require(session["other_active_e_hgp_instance_count"] == 0, "other active VM recorded")
    require(session["session_ssh_key_revoked"] is True, "session key not revoked")
    require(session["local_private_key_removed"] is True, "private key not removed")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", type=Path)
    parser.add_argument("k2", type=Path)
    parser.add_argument("k10", type=Path)
    parser.add_argument("mem_json", type=Path)
    parser.add_argument("memcheck_transcript", type=Path)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    summary = load_json(args.summary)
    k2 = load_json(args.k2)
    k10 = load_json(args.k10)
    mem_json = load_json(args.mem_json)
    check_raw(k2, 2)
    check_raw(k10, 10)
    require(mem_json.get("git_sha") == GIT_SHA, "memcheck git SHA drift")
    require(mem_json.get("point_count") == 4096, "memcheck point count drift")
    require(mem_json.get("requested_maximum_order") == 2, "memcheck K drift")
    require(mem_json.get("component_success") is True, "memcheck runner failed")
    check_summary(
        summary,
        k2,
        k10,
        args.k2,
        args.k10,
        args.mem_json,
        args.memcheck_transcript,
    )
    print("validated Phase 15 pair-rank K=2/K=10 negative benchmark")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except ContractError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
