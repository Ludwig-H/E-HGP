#!/usr/bin/env python3
"""Validate one compact Phase 15 Geogram/LBVH Gabriel-rank artifact."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


class ValidationError(RuntimeError):
    """Raised when a neighbor-rank artifact violates its closed schema."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def require_int(value: Any, label: str, minimum: int = 0) -> int:
    require(
        isinstance(value, int) and not isinstance(value, bool),
        f"{label} must be an integer",
    )
    require(value >= minimum, f"{label} must be >= {minimum}")
    return value


def expected_quantile(counts: list[int], overflow: int, percent: int) -> int | None:
    total = sum(counts) + overflow
    if total == 0:
        return None
    target = (total * percent + 99) // 100
    cumulative = 0
    for rank, count in enumerate(counts, start=1):
        cumulative += count
        if cumulative >= target:
            return rank
    return None


def validate_histogram(
    value: Any,
    maximum_rank: int,
    evaluated_count: int,
    label: str,
) -> list[int]:
    require(isinstance(value, dict), f"{label} must be an object")
    counts_value = value.get("exact_rank_counts_1_through_M")
    require(isinstance(counts_value, list), f"{label} counts must be a list")
    require(len(counts_value) == maximum_rank, f"{label} count extent mismatch")
    counts = [
        require_int(count, f"{label}.rank[{rank}]", 0)
        for rank, count in enumerate(counts_value, start=1)
    ]
    captured = require_int(value.get("captured_count"), f"{label}.captured_count")
    overflow = require_int(value.get("overflow_count"), f"{label}.overflow_count")
    require(captured == sum(counts), f"{label} captured count does not close")
    require(captured + overflow == evaluated_count, f"{label} partition does not close")
    positive_ranks = [rank for rank, count in enumerate(counts, start=1) if count]
    expected_maximum = (
        None if overflow else (max(positive_ranks) if positive_ranks else None)
    )
    require(
        value.get("maximum_observed_rank") == expected_maximum,
        f"{label} maximum rank mismatch",
    )
    for percent in (50, 90, 95, 99):
        require(
            value.get(f"p{percent}_smallest_M")
            == expected_quantile(counts, overflow, percent),
            f"{label} p{percent} mismatch",
        )
    return counts


def validate_artifact(payload: Any) -> None:
    require(isinstance(payload, dict), "artifact root must be an object")
    require(payload.get("phase") == "15", "phase must be 15")
    require(payload.get("profile") == "hgp_reduced", "profile mismatch")
    require(payload.get("mode") == "offline_neighbor_rank", "mode mismatch")
    require(payload.get("public_status") == "not_claimed", "public status mismatch")
    require(
        payload.get("schema") == "morsehgp3d.phase15_delaunay_gabriel_neighbor_rank.v1",
        "outer schema mismatch",
    )
    architecture = payload.get("architecture")
    require(isinstance(architecture, dict), "architecture must be an object")
    require(
        architecture.get("higher_order_Delaunay_mosaic_materialized") is False,
        "higher-order Delaunay materialization is forbidden",
    )
    require(
        architecture.get("global_n_by_M_neighbor_table_materialized") is False,
        "an n-by-M table is forbidden",
    )
    require(
        architecture.get("additional_global_triangle_rank_table_materialized") is False,
        "an additional global triangle-rank table is forbidden",
    )

    classification = payload.get("classification")
    coverage = payload.get("coverage")
    require(isinstance(classification, dict), "classification must be an object")
    require(isinstance(coverage, dict), "coverage must be an object")
    accepted = require_int(
        classification.get("gabriel_binary64_count"),
        "classification.gabriel_binary64_count",
    )
    require(
        coverage.get("compact_neighbor_rank_gate_passed") is True, "rank gate failed"
    )
    require(coverage.get("selected_process_gate_passed") is True, "process gate failed")

    diagnostic = payload.get("neighbor_rank_diagnostic")
    require(isinstance(diagnostic, dict), "neighbor_rank_diagnostic must be an object")
    require(
        diagnostic.get("schema") == "morsehgp3d.phase15_gabriel_neighbor_rank.v1",
        "diagnostic schema mismatch",
    )
    require(
        diagnostic.get("status") == "offline_proposal_diagnostic_not_a_proof",
        "diagnostic status mismatch",
    )
    require(
        diagnostic.get("scientific_scope")
        == "PDEL_witness_root_neighbor_wedge_thresholds_on_the_accepted_Gabriel_set_not_a_Gabriel_or_Gamma2_catalogue",
        "scientific scope mismatch",
    )
    require(
        diagnostic.get("rank_semantics")
        == "exact_prefix_for_the_declared_binary64_key",
        "rank semantics mismatch",
    )
    require(
        diagnostic.get("rank_overflow_semantics")
        == "exact_rank_strictly_greater_than_M_value_censored",
        "overflow semantics mismatch",
    )
    maximum_rank = require_int(diagnostic.get("maximum_rank_M"), "maximum_rank_M", 2)
    require(maximum_rank <= 256, "maximum_rank_M exceeds the uint16 diagnostic cap")
    require(
        diagnostic.get("one_complete_LBVH_traversal_per_source_not_per_arc") is True,
        "the query must use one traversal per source",
    )
    require(
        diagnostic.get("two_incident_edges_form_one_wedge_proposal") is True,
        "the diagnostic must use the two-edge wedge proposal semantics",
    )
    require(
        diagnostic.get("support_two_PDEL_witnesses_allowed") is True,
        "support-two PDEL witnesses must be evaluated",
    )
    require(
        diagnostic.get("root_scope")
        == "minimum_over_available_PDEL_witness_roots",
        "root scope mismatch",
    )
    require(
        diagnostic.get("partial_root_threshold_relation")
        == "safe_upper_bound_on_the_minimum_over_all_three_roots",
        "partial-root threshold relation mismatch",
    )
    require(
        diagnostic.get("absolute_minimum_over_all_three_roots_claimed") is False,
        "an unavailable non-PDEL root must not be claimed",
    )
    require(diagnostic.get("Gabriel_catalogue_claimed") is False, "catalogue claim")
    require(diagnostic.get("exact_Gamma2_claimed") is False, "Gamma2 claim")
    require(
        diagnostic.get("CSR_row_writes_race_free_by_source_permutation") is True,
        "CSR write ownership mismatch",
    )
    require(
        diagnostic.get("maximum_logical_heap_storage_bytes_per_query") == 4096,
        "logical heap storage mismatch",
    )
    require(
        diagnostic.get(
            "CUDA_local_memory_spill_and_throughput_require_G4_qualification"
        )
        is True,
        "missing G4 local-memory qualification warning",
    )
    require(
        diagnostic.get("n_by_M_table_materialized") is False, "n-by-M table present"
    )
    require(
        diagnostic.get("additional_global_triangle_table_materialized") is False,
        "additional triangle table present",
    )
    require(
        diagnostic.get("compact_rank_gate_passed") is True, "compact rank gate failed"
    )
    require(
        diagnostic.get("all_variants_have_PDEL_witness_root") is True,
        "some accepted Gabriel triangle has no PDEL witness root",
    )
    require(
        require_int(
            diagnostic.get("accepted_triangle_count"), "accepted_triangle_count"
        )
        == accepted,
        "accepted triangle count mismatch",
    )
    complete = require_int(
        diagnostic.get("complete_three_pair_triangle_count"),
        "complete_three_pair_triangle_count",
    )
    partial = require_int(
        diagnostic.get("partial_two_pair_witness_triangle_count"),
        "partial_two_pair_witness_triangle_count",
    )
    insufficient = require_int(
        diagnostic.get("insufficient_pair_witness_triangle_count"),
        "insufficient_pair_witness_triangle_count",
    )
    require(
        complete + partial + insufficient == accepted,
        "PDEL pair partition does not close",
    )
    require(insufficient == 0, "every accepted triangle needs a PDEL witness root")
    expected_root_count = 3 * complete + partial

    variants = diagnostic.get("variants")
    require(isinstance(variants, dict), "variants must be an object")
    variant_counts: dict[str, list[int]] = {}
    for name in ("directed_root_star", "symmetric_union_star", "mutual_star"):
        variant = variants.get(name)
        require(isinstance(variant, dict), f"{name} must be an object")
        require(
            variant.get("threshold_scope")
            == "minimum_over_available_PDEL_witness_roots",
            f"{name} threshold scope mismatch",
        )
        witness_count = require_int(
            variant.get("witness_triangle_count"), f"{name}.witness_count"
        )
        missing_count = require_int(
            variant.get("missing_witness_triangle_count"),
            f"{name}.missing_witness_count",
        )
        require(
            witness_count + missing_count == accepted,
            f"{name} witness partition does not close",
        )
        require(missing_count == 0, f"{name} has a missing PDEL witness")
        require(
            require_int(
                variant.get("considered_root_count"),
                f"{name}.considered_root_count",
            )
            == expected_root_count,
            f"{name} considered-root count mismatch",
        )
        require(
            variant.get("first_missing_witness_triangle") is None,
            f"{name} has an unexpected missing-witness record",
        )
        variant_counts[name] = validate_histogram(
            variant.get("histogram"), maximum_rank, witness_count, name
        )
    directed_cumulative = 0
    union_cumulative = 0
    mutual_cumulative = 0
    for rank in range(maximum_rank):
        directed_cumulative += variant_counts["directed_root_star"][rank]
        union_cumulative += variant_counts["symmetric_union_star"][rank]
        mutual_cumulative += variant_counts["mutual_star"][rank]
        require(
            union_cumulative >= directed_cumulative >= mutual_cumulative,
            f"variant coverage ordering fails at M={rank + 1}",
        )

    point_count = require_int(
        payload.get("input", {}).get("point_count"), "point_count", 1
    )
    replay_cap = require_int(
        diagnostic.get("bounded_bruteforce_rank_replay_point_cap"),
        "bounded replay point cap",
        1,
    )
    replay_performed = diagnostic.get(
        "bounded_bruteforce_rank_replay_performed"
    )
    replay_passed = diagnostic.get("bounded_bruteforce_rank_replay_passed")
    require(isinstance(replay_performed, bool), "replay performed must be boolean")
    require(isinstance(replay_passed, bool), "replay passed must be boolean")
    replay = diagnostic.get("bounded_bruteforce_rank_replay")
    require(isinstance(replay, dict), "bounded replay must be an object")
    require(replay.get("point_count") == point_count, "bounded replay n mismatch")
    replay_compared_arc_count = require_int(
        replay.get("compared_arc_count"), "bounded replay compared arcs"
    )
    replay_mismatch_count = require_int(
        replay.get("mismatch_count"), "bounded replay mismatches"
    )
    if point_count <= replay_cap:
        require(replay_performed is True, "bounded smoke replay was not performed")
        require(replay_passed is True, "bounded smoke replay failed")
        require(replay_mismatch_count == 0, "bounded smoke replay has mismatches")
        require(replay.get("first_mismatch") is None, "unexpected replay mismatch")
    else:
        require(
            replay_performed is False,
            "the bounded replay must stay disabled above its declared cap",
        )
        require(replay_passed is False, "an unperformed replay cannot pass")
        require(replay_compared_arc_count == 0, "unperformed replay compared arcs")
        require(replay_mismatch_count == 0, "unperformed replay has mismatches")

    policy = diagnostic.get("proposal_policy_K_log_n")
    require(isinstance(policy, dict), "proposal policy must be an object")
    require(policy.get("K") == 2, "proposal policy K mismatch")
    require(
        policy.get("proposal_only_not_a_completeness_proof") is True,
        "K log(n) must not be presented as a proof",
    )
    samples = policy.get("samples")
    require(isinstance(samples, list) and len(samples) == 4, "policy samples mismatch")
    for sample, multiplier in zip(samples, (1, 2, 4, 8), strict=True):
        require(isinstance(sample, dict), "policy sample must be an object")
        expected_rank = max(2, math.ceil(2 * multiplier * math.log(point_count)))
        require(sample.get("c") == multiplier, "policy multiplier mismatch")
        require(sample.get("M") == expected_rank, "policy M mismatch")
        within = expected_rank <= maximum_rank
        require(sample.get("within_measured_cap") is within, "policy cap flag mismatch")
        fields = {
            "directed_root_star_captured_count": "directed_root_star",
            "symmetric_union_star_captured_count": "symmetric_union_star",
            "mutual_star_captured_count": "mutual_star",
        }
        for field, variant_name in fields.items():
            expected_count = (
                sum(variant_counts[variant_name][:expected_rank]) if within else None
            )
            require(sample.get(field) == expected_count, f"{field} mismatch")

    audit = diagnostic.get("lbvh_audit")
    require(isinstance(audit, dict), "LBVH audit must be an object")
    require(
        require_int(audit.get("point_count"), "audit.point_count") == point_count,
        "audit n mismatch",
    )
    require(
        require_int(audit.get("certified_node_count"), "audit.node_count")
        == 2 * point_count - 1,
        "certified node count mismatch",
    )
    arc_count = require_int(audit.get("directed_CSR_arc_count"), "audit.arc_count", 1)
    if replay_performed:
        require(
            replay_compared_arc_count == arc_count,
            "bounded replay did not compare every directed CSR arc",
        )
    require(audit.get("maximum_rank") == maximum_rank, "audit M mismatch")
    require(
        audit.get("overflow_rank") == maximum_rank + 1, "audit overflow rank mismatch"
    )
    seed_window = require_int(audit.get("seed_window_radius"), "audit.seed_window")
    require(seed_window >= maximum_rank, "audit seed window is smaller than M")
    query_batch_size = require_int(
        audit.get("query_batch_size"), "audit.query_batch_size", 1
    )
    require(query_batch_size <= point_count, "audit query batch exceeds n")
    require(
        audit.get("query_batch_count")
        == 1 + (point_count - 1) // query_batch_size,
        "audit query batch count mismatch",
    )
    require(
        require_int(
            audit.get("kernel_local_size_bytes_per_thread"),
            "audit.kernel_local_size_bytes_per_thread",
        )
        >= 4096,
        "compiled CUDA local storage undercounts the fixed rank heaps",
    )
    require(
        require_int(audit.get("ranked_arc_count"), "audit.ranked_arc_count")
        + require_int(audit.get("overflow_arc_count"), "audit.overflow_arc_count")
        == arc_count,
        "CSR arc rank partition does not close",
    )
    require(
        audit.get("completed_query_count") == point_count, "query coverage mismatch"
    )
    require(audit.get("failed_query_count") == 0, "rank query failure")
    for flag in (
        "fixed_round_to_nearest_distance_recipe_requested",
        "directed_round_down_AABB_recipe_requested",
        "strict_prune_requested",
        "equality_descends_requested",
        "stackless_postorder_traversal_requested",
        "complete_query_coverage",
        "no_search_work_cap",
        "exact_binary64_prefix_complete",
        "ranks_above_maximum_censored",
        "source_mapping_permutation_validated",
        "csr_structure_validated",
        "host_rank_transcript_validated",
    ):
        require(audit.get(flag) is True, f"audit flag {flag} is not true")
    for flag in (
        "n_by_maximum_rank_table_materialized",
        "higher_order_delaunay_mosaic_materialized",
        "global_pair_matrix_materialized",
        "exact_morse_hierarchy_claimed",
        "public_status_claimed",
    ):
        require(audit.get(flag) is False, f"audit flag {flag} is not false")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("artifact", type=Path)
    args = parser.parse_args()
    try:
        payload = json.loads(args.artifact.read_text(encoding="utf-8"))
        validate_artifact(payload)
    except (OSError, json.JSONDecodeError, ValidationError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: compact Phase 15 Gabriel neighbor-rank artifact is closed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
