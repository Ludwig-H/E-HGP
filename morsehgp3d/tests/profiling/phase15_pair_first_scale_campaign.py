#!/usr/bin/env python3
"""Plan, run, and validate the future complete Phase 15 pair-first campaign.

This harness deliberately cannot promote a component profile.  The measured
binary must first identify itself as the complete certified pair-frontier and
exact-classification pipeline, and every run must close the unordered-pair mass.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from typing import Any, NoReturn, Sequence

PLAN_SCHEMA = "morsehgp3d.phase15.pair_first_scale_campaign_plan.v1"
CAPABILITY_SCHEMA = "morsehgp3d.phase15.pair_first_binary_capabilities.v1"
RUN_REQUEST_SCHEMA = "morsehgp3d.phase15.pair_first_scale_run_request.v1"
RUN_SCHEMA = "morsehgp3d.phase15.pair_first_scale_run.v1"
PREFLIGHT_SCHEMA = "morsehgp3d.phase15.pair_first_scale_preflight.v1"
CAMPAIGN_SCHEMA = "morsehgp3d.phase15.pair_first_scale_campaign_result.v1"
ALGORITHM_SCOPE = "complete_certified_morton_yao48_ranked_pair_catalog"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "certified_budgeted"
DEFAULT_PLAN = Path(__file__).with_name("phase15_pair_first_scale_campaign_v1.json")
SHA40_RE = re.compile(r"[0-9a-f]{40}\Z")
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
GATE_ARTIFACT_SCHEMA = "morsehgp3d.phase15.pair_first_scale_gate_artifact.v1"
SUBPROCESS_TIMEOUT_ALLOWANCE_SECONDS = 30
CAMPAIGN_GUARD_MARGIN_SECONDS = 600

FAMILY_IDS = (
    "affine_uniform_binary64",
    "jittered_dyadic_grid3d",
    "balanced_multiscale_clusters",
)
SCALE_MATRIX = (
    (50_000, None, (1, 2, 3, 5, 8, 13, 21, 34, 55, 89), 30_000, 100_000_000),
    (1_000_000, 50_000, (1, 2, 3), 180_000, None),
    (10_000_000, 1_000_000, (1, 2), 900_000, None),
    (30_000_000, 10_000_000, (1,), 1_800_000, None),
)
LINEAR_CAPS = {
    "anchor_tile_size": 4096,
    "bank_updates_per_point": 960,
    "classified_pairs_per_point": 640,
    "device_memory_fraction_ppm": 700_000,
    "frontier_records_per_anchor_in_tile": 640,
    "host_memory_fraction_ppm": 700_000,
    "node_visits_per_point": 2048,
    "output_pairs_per_point": 32,
    "radial_receipts_per_point": 64,
}
PREFLIGHT_GATES = (
    "bounded_cpu_differential_k1_k2_k10",
    "offline_k1_direct_pair_reduction_equals_emst_fusion_times",
    "offline_k2_fusions_no_later_than_two_delaunay_edge_triangle_deadlines",
    "tile_schedule_resume_equivalence",
    "compute_sanitizer_memcheck_racecheck",
    "n12500_growth_falsifier",
)
PREFLIGHT_ENVIRONMENT_TEMPLATE = {
    "compute_capability": "12.0",
    "gce_shutdown_guard_verified": True,
    "gce_deadline_epoch": 0,
    "gpu_machine_type": "g4-standard-48",
    "guest_shutdown_guard_verified": True,
    "guest_deadline_epoch": 0,
    "guest_shutdown_delay_seconds": 0,
    "instance_generation": "",
    "instance_name": "",
    "instance_termination_action": "STOP",
    "max_run_duration_seconds": 0,
    "preflight_epoch": 0,
    "project_id": "devpod-gpu-exploration",
    "project_label": "project=e-hgp",
    "provisioning_model": "SPOT",
    "start_entrypoint": "./gcp-migration/start_and_verify.sh",
    "stop_entrypoint": "./gcp-migration/stop_and_verify.sh",
    "stop_responsibility": "external_guarded_orchestrator",
    "target_label_verified": True,
    "targeted_stop_and_termination_readback_required": True,
    "worker_mutates_gcp": False,
    "zone": "",
}
GATE_EVIDENCE_KEYS = {
    "artifact_file",
    "artifact_sha256",
    "gate",
    "git_sha",
    "status",
}
GATE_ARTIFACT_KEYS = {
    "binary_sha256",
    "execution_scope",
    "gate",
    "git_sha",
    "metrics",
    "schema",
    "status",
}
CAPABILITY_KEYS = {
    "algorithm_scope",
    "backend",
    "complete_pair_frontier",
    "component_only",
    "dense_fallback_available",
    "dense_pair_scan",
    "delaunay",
    "emst",
    "exact_pair_classifier",
    "global_pair_matrix",
    "maximum_requested_order",
    "mode",
    "multi_order_single_pass",
    "pair_mass_receipts",
    "profile",
    "proxy",
    "resident_resumable_frontier",
    "run_schema",
    "schema",
    "supported_families",
    "uses_boruvka",
}
IMPLEMENTATION_KEYS = {
    "complete_pair_frontier",
    "component_only",
    "dense_pair_scan",
    "delaunay",
    "emst",
    "global_pair_matrix",
    "offline_oracle_in_timed_path",
    "proxy",
    "uses_boruvka",
}
RUN_KEYS = {
    "algorithm_scope",
    "backend",
    "binary_sha256",
    "caps",
    "censor_reason",
    "cloud",
    "closure",
    "family",
    "git_sha",
    "implementation",
    "mode",
    "point_count",
    "profile",
    "requested_order_max",
    "run_id",
    "schema",
    "seed",
    "status",
    "timings_ns",
    "work",
}
CLOUD_KEYS = {
    "canonical_binary64_sha256",
    "construction_count",
    "fresh_for_measured_run",
    "request_sha256",
}
CAP_KEYS = {
    "bank_updates",
    "classified_pairs",
    "device_memory_fraction_ppm",
    "frontier_records",
    "host_memory_fraction_ppm",
    "node_visits",
    "output_pairs",
    "radial_receipts",
    "wall_time",
}
CLOSURE_KEYS = {
    "accepted_pair_count",
    "candidate_pair_mass",
    "certified_pruned_pair_mass",
    "checkpoint_closed",
    "classifier_session_count",
    "exact_replay_closed",
    "frontier_empty",
    "payload_closed",
    "rejected_pair_count",
    "total_unordered_pair_mass",
    "unique_candidate_pair_count",
    "unresolved_pair_mass",
}
TIMING_KEYS = {
    "certified_frontier",
    "cold_e2e",
    "compaction",
    "exact_pair_classification",
    "morton_lbvh",
    "persistence",
    "resident_replay",
    "warm_e2e_fresh_cloud",
    "yao48_bank_fill",
}
WORK_KEYS = {
    "bank_updates",
    "classified_pairs",
    "d2h_intermediate_candidate_batches",
    "dense_fallback_pair_visits",
    "device_capacity_bytes",
    "device_peak_bytes",
    "frontier_peak_records",
    "host_callbacks_per_pair",
    "host_capacity_bytes",
    "host_peak_bytes",
    "node_visits",
    "output_pairs",
    "radial_receipts",
}
CENSOR_REASONS = {
    "bank_update_cap",
    "classified_pair_cap",
    "device_memory_cap",
    "frontier_cap",
    "host_memory_cap",
    "node_visit_cap",
    "output_pair_cap",
    "radial_receipt_cap",
    "wall_time_cap",
}


class ContractError(ValueError):
    """A campaign document escaped the fail-closed profiling contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(set(value) == expected, f"{label} fields differ")
    return value


def natural(value: Any, label: str, *, positive: bool = False) -> int:
    require(type(value) is int and value >= 0, f"{label} must be a natural")
    require(not positive or value > 0, f"{label} must be positive")
    return value


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_nonfinite(value: str) -> NoReturn:
    fail(f"non-finite JSON constant: {value}")


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))


def parse_json_text(raw: str, label: str, *, canonical_line: bool) -> Any:
    try:
        value = json.loads(
            raw,
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_nonfinite,
        )
    except (json.JSONDecodeError, ContractError) as error:
        fail(f"cannot parse {label}: {error}")
    if canonical_line:
        require(
            raw == canonical_json(value) + "\n",
            f"{label} is not canonical one-line JSON",
        )
    return value


def read_json(path: Path, label: str, *, canonical_line: bool = False) -> Any:
    require(path.is_file() and not path.is_symlink(), f"{label} must be a regular file")
    try:
        raw = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label}: {error}")
    return parse_json_text(raw, label, canonical_line=canonical_line)


def sha256_file(path: Path, label: str) -> str:
    require(path.is_file() and not path.is_symlink(), f"{label} must be a regular file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label}: {error}")
    return digest.hexdigest()


def validate_plan(value: Any) -> dict[str, Any]:
    plan = exact_keys(
        value,
        {
            "algorithm_scope",
            "artifact_role",
            "backend",
            "benchmark_claim_policy",
            "binary_protocol",
            "closure_contract",
            "execution_status",
            "families",
            "linear_caps",
            "mode",
            "phase",
            "preflight_gates",
            "profile",
            "requested_order_max",
            "scales",
            "schema",
            "timing_contract",
        },
        "campaign plan",
    )
    require(plan["schema"] == PLAN_SCHEMA, "campaign plan schema differs")
    require(
        plan["algorithm_scope"] == ALGORITHM_SCOPE, "campaign algorithm scope differs"
    )
    require(
        plan["artifact_role"] == "future_complete_pair_first_product_gate",
        "campaign artifact role differs",
    )
    require(plan["backend"] == BACKEND, "campaign backend differs")
    require(plan["profile"] == PROFILE, "campaign profile differs")
    require(plan["mode"] == MODE, "campaign mode differs")
    require(plan["phase"] == "15", "campaign phase differs")
    require(plan["requested_order_max"] == 10, "campaign K_max differs")
    require(
        plan["execution_status"] == "blocked_until_complete_pair_first_binary",
        "campaign must remain blocked before the complete binary exists",
    )
    require(
        plan["benchmark_claim_policy"]
        == {
            "component_measurements_are_results": False,
            "dense_oracle_measurements_are_results": False,
            "proxy_measurements_are_results": False,
            "requires_complete_binary_capability_handshake": True,
        },
        "campaign benchmark claim policy differs",
    )
    require(
        plan["binary_protocol"]
        == {
            "capability_argument": "--phase15-pair-first-capabilities-json",
            "capability_schema": CAPABILITY_SCHEMA,
            "run_argument": "--phase15-pair-first-scale-run-json",
            "run_schema": RUN_SCHEMA,
        },
        "campaign binary protocol differs",
    )
    require(
        plan["closure_contract"]
        == {
            "complete_requires_checkpoint_closed": True,
            "complete_requires_exact_replay_closed": True,
            "complete_requires_frontier_empty": True,
            "complete_requires_payload_closed": True,
            "complete_requires_unresolved_pair_mass_zero": True,
            "dense_fallback_allowed": False,
            "mass_identity": "candidate_pair_mass+certified_pruned_pair_mass+unresolved_pair_mass=total_unordered_pair_mass",
            "offline_oracle_in_timed_path_allowed": False,
        },
        "campaign closure contract differs",
    )
    families = plan["families"]
    require(
        isinstance(families, list) and len(families) == len(FAMILY_IDS),
        "campaign families differ",
    )
    require(
        tuple(item.get("id") for item in families if isinstance(item, dict))
        == FAMILY_IDS,
        "campaign family order or identity differs",
    )
    for index, family in enumerate(families):
        exact_keys(
            family, {"generator", "id", "parameters"}, f"campaign family {index}"
        )
        require(
            isinstance(family["generator"], str) and family["generator"],
            f"campaign family {index} generator is absent",
        )
        require(
            isinstance(family["parameters"], dict) and family["parameters"],
            f"campaign family {index} parameters are absent",
        )
        for key, item in family["parameters"].items():
            require(
                isinstance(key, str) and key,
                f"campaign family {index} parameter name is invalid",
            )
            natural(item, f"campaign family {index} parameter {key}", positive=True)
    require(plan["linear_caps"] == LINEAR_CAPS, "campaign linear caps differ")
    require(
        plan["preflight_gates"] == list(PREFLIGHT_GATES),
        "campaign preflight gates differ",
    )
    scales = plan["scales"]
    require(
        isinstance(scales, list) and len(scales) == len(SCALE_MATRIX),
        "campaign scale count differs",
    )
    for index, (scale, expected) in enumerate(zip(scales, SCALE_MATRIX, strict=True)):
        exact_keys(
            scale,
            {
                "point_count",
                "required_previous_point_count",
                "seeds",
                "slo_p95_warm_e2e_ns",
                "wall_time_cap_ms",
            },
            f"campaign scale {index}",
        )
        point_count, previous, seeds, wall_cap, slo = expected
        require(
            scale
            == {
                "point_count": point_count,
                "required_previous_point_count": previous,
                "seeds": list(seeds),
                "slo_p95_warm_e2e_ns": slo,
                "wall_time_cap_ms": wall_cap,
            },
            f"campaign scale {index} differs",
        )
    timing = plan["timing_contract"]
    require(
        timing
        == {
            "cold_e2e_is_diagnostic_only": True,
            "cloud_generation_is_timed": False,
            "fresh_cloud_per_measured_run": True,
            "p50_p95_rule": "nearest_rank",
            "resident_replay_is_diagnostic_only": True,
            "timed_stages": [
                "morton_lbvh",
                "yao48_bank_fill",
                "certified_frontier",
                "exact_pair_classification",
                "compaction",
                "persistence",
            ],
            "warm_e2e_includes_all_timed_stages": True,
        },
        "campaign timing contract differs",
    )
    return plan


def computed_caps(point_count: int, plan: dict[str, Any]) -> dict[str, int]:
    caps = plan["linear_caps"]
    scale = next(item for item in plan["scales"] if item["point_count"] == point_count)
    return {
        "bank_updates": point_count * caps["bank_updates_per_point"],
        "classified_pairs": point_count * caps["classified_pairs_per_point"],
        "device_memory_fraction_ppm": caps["device_memory_fraction_ppm"],
        "frontier_records": caps["anchor_tile_size"]
        * caps["frontier_records_per_anchor_in_tile"],
        "host_memory_fraction_ppm": caps["host_memory_fraction_ppm"],
        "node_visits": point_count * caps["node_visits_per_point"],
        "output_pairs": point_count * caps["output_pairs_per_point"],
        "radial_receipts": point_count * caps["radial_receipts_per_point"],
        "wall_time": scale["wall_time_cap_ms"] * 1_000_000,
    }


def cloud_request_sha256(family: dict[str, Any], point_count: int, seed: int) -> str:
    descriptor = {
        "family": family,
        "point_count": point_count,
        "seed": seed,
    }
    return hashlib.sha256(canonical_json(descriptor).encode("ascii")).hexdigest()


def expected_run_requests(plan: dict[str, Any]) -> list[dict[str, Any]]:
    requests: list[dict[str, Any]] = []
    families = {item["id"]: item for item in plan["families"]}
    for scale in plan["scales"]:
        point_count = scale["point_count"]
        for family_id in FAMILY_IDS:
            for seed in scale["seeds"]:
                family = families[family_id]
                requests.append(
                    {
                        "algorithm_scope": ALGORITHM_SCOPE,
                        "caps": computed_caps(point_count, plan),
                        "cloud_request_sha256": cloud_request_sha256(
                            family, point_count, seed
                        ),
                        "family": family,
                        "fresh_cloud": True,
                        "point_count": point_count,
                        "requested_order_max": plan["requested_order_max"],
                        "run_id": f"n{point_count}-{family_id}-s{seed}",
                        "schema": RUN_REQUEST_SCHEMA,
                        "seed": seed,
                    }
                )
    return requests


def validate_capabilities(value: Any, plan: dict[str, Any]) -> dict[str, Any]:
    capabilities = exact_keys(value, CAPABILITY_KEYS, "binary capabilities")
    expected = {
        "algorithm_scope": ALGORITHM_SCOPE,
        "backend": BACKEND,
        "complete_pair_frontier": True,
        "component_only": False,
        "dense_fallback_available": False,
        "dense_pair_scan": False,
        "delaunay": False,
        "emst": False,
        "exact_pair_classifier": True,
        "global_pair_matrix": False,
        "maximum_requested_order": 10,
        "mode": MODE,
        "multi_order_single_pass": True,
        "pair_mass_receipts": True,
        "profile": PROFILE,
        "proxy": False,
        "resident_resumable_frontier": True,
        "run_schema": RUN_SCHEMA,
        "schema": CAPABILITY_SCHEMA,
        "supported_families": list(FAMILY_IDS),
        "uses_boruvka": False,
    }
    require(
        capabilities == expected,
        "binary is not the complete non-proxy pair-first implementation",
    )
    require(
        plan["binary_protocol"]["capability_schema"] == capabilities["schema"],
        "binary capability schema is not bound to the plan",
    )
    return capabilities


def registered_remaining_timeout_seconds(
    plan: dict[str, Any], request_index: int = 0
) -> int:
    requests = expected_run_requests(plan)
    require(
        type(request_index) is int and 0 <= request_index <= len(requests),
        "campaign remaining-time request index is invalid",
    )
    return CAMPAIGN_GUARD_MARGIN_SECONDS + sum(
        (request["caps"]["wall_time"] + 999_999_999) // 1_000_000_000
        + SUBPROCESS_TIMEOUT_ALLOWANCE_SECONDS
        for request in requests[request_index:]
    )


def validate_remaining_deadline_coverage(
    environment: dict[str, Any],
    plan: dict[str, Any],
    request_index: int,
    *,
    now_epoch: int | None = None,
) -> None:
    now = (
        int(time.time())
        if now_epoch is None
        else natural(now_epoch, "campaign deadline revalidation epoch", positive=True)
    )
    required_seconds = registered_remaining_timeout_seconds(plan, request_index)
    require(
        environment["gce_deadline_epoch"] - now >= required_seconds,
        "campaign GCE deadline cannot cover every remaining registered run timeout",
    )
    require(
        environment["guest_deadline_epoch"] - now >= required_seconds,
        "campaign guest deadline cannot cover every remaining registered run timeout",
    )


def read_canonical_json_with_sha256(
    path: Path, label: str
) -> tuple[dict[str, Any], str]:
    require(path.is_file() and not path.is_symlink(), f"{label} must be a regular file")
    try:
        raw_bytes = path.read_bytes()
        raw = raw_bytes.decode("ascii")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label}: {error}")
    value = parse_json_text(raw, label, canonical_line=True)
    require(isinstance(value, dict), f"{label} must be an object")
    return value, hashlib.sha256(raw_bytes).hexdigest()


def validate_gate_metrics(gate: str, value: Any) -> dict[str, Any]:
    label = f"campaign preflight gate {gate} metrics"
    if gate == "bounded_cpu_differential_k1_k2_k10":
        metrics = exact_keys(value, {"case_count", "mismatch_count", "orders"}, label)
        natural(metrics["case_count"], f"{label}.case_count", positive=True)
        require(metrics["orders"] == [1, 2, 10], f"{label}.orders differ")
        require(
            natural(metrics["mismatch_count"], f"{label}.mismatch_count") == 0,
            f"{label} retains a mismatch",
        )
    elif gate == "offline_k1_direct_pair_reduction_equals_emst_fusion_times":
        metrics = exact_keys(
            value,
            {
                "case_count",
                "closed_cut_mismatch_count",
                "fusion_time_mismatch_count",
                "multifusion_mismatch_count",
                "strict_cut_mismatch_count",
            },
            label,
        )
        natural(metrics["case_count"], f"{label}.case_count", positive=True)
        for key in (
            "closed_cut_mismatch_count",
            "fusion_time_mismatch_count",
            "multifusion_mismatch_count",
            "strict_cut_mismatch_count",
        ):
            require(
                natural(metrics[key], f"{label}.{key}") == 0,
                f"{label} retains a k=1 mismatch",
            )
    elif (
        gate == "offline_k2_fusions_no_later_than_two_delaunay_edge_triangle_deadlines"
    ):
        metrics = exact_keys(
            value, {"comparison_count", "deadline_violation_count"}, label
        )
        natural(metrics["comparison_count"], f"{label}.comparison_count", positive=True)
        require(
            natural(
                metrics["deadline_violation_count"],
                f"{label}.deadline_violation_count",
            )
            == 0,
            f"{label} retains a k=2 deadline violation",
        )
    elif gate == "tile_schedule_resume_equivalence":
        metrics = exact_keys(value, {"case_count", "transcript_mismatch_count"}, label)
        natural(metrics["case_count"], f"{label}.case_count", positive=True)
        require(
            natural(
                metrics["transcript_mismatch_count"],
                f"{label}.transcript_mismatch_count",
            )
            == 0,
            f"{label} retains a transcript mismatch",
        )
    elif gate == "compute_sanitizer_memcheck_racecheck":
        metrics = exact_keys(
            value,
            {
                "instrumented_run_count",
                "leak_count",
                "memcheck_error_count",
                "racecheck_error_count",
            },
            label,
        )
        natural(
            metrics["instrumented_run_count"],
            f"{label}.instrumented_run_count",
            positive=True,
        )
        for key in ("leak_count", "memcheck_error_count", "racecheck_error_count"):
            require(
                natural(metrics[key], f"{label}.{key}") == 0,
                f"{label} retains a sanitizer failure",
            )
    elif gate == "n12500_growth_falsifier":
        metrics = exact_keys(
            value,
            {
                "dense_fallback_pair_visits",
                "frontier_empty",
                "growth_within_registered_caps",
                "point_count",
                "unresolved_pair_mass",
            },
            label,
        )
        require(metrics["point_count"] == 12_500, f"{label}.point_count differs")
        require(
            metrics["growth_within_registered_caps"] is True,
            f"{label} did not close the registered growth gate",
        )
        require(metrics["frontier_empty"] is True, f"{label} retains an open frontier")
        require(
            natural(
                metrics["dense_fallback_pair_visits"],
                f"{label}.dense_fallback_pair_visits",
            )
            == 0,
            f"{label} used a dense fallback",
        )
        require(
            natural(metrics["unresolved_pair_mass"], f"{label}.unresolved_pair_mass")
            == 0,
            f"{label} retains unresolved pair mass",
        )
    else:
        fail(f"unknown campaign preflight gate: {gate}")
    return metrics


def validate_preflight_gate_artifacts(
    gates: dict[str, Any],
    *,
    preflight_path: Path,
    binary_sha256: str,
    git_sha: str,
) -> None:
    for gate in PREFLIGHT_GATES:
        evidence = exact_keys(
            gates[gate], GATE_EVIDENCE_KEYS, f"campaign preflight gate {gate}"
        )
        require(
            evidence["gate"] == gate and evidence["status"] == "passed",
            f"campaign preflight gate {gate} is not passed",
        )
        require(
            evidence["git_sha"] == git_sha,
            f"campaign preflight gate {gate} is from another Git SHA",
        )
        require(
            isinstance(evidence["artifact_sha256"], str)
            and SHA256_RE.fullmatch(evidence["artifact_sha256"]) is not None,
            f"campaign preflight gate {gate} has no evidence digest",
        )
        artifact_file = evidence["artifact_file"]
        require(
            isinstance(artifact_file, str)
            and artifact_file not in {"", ".", ".."}
            and Path(artifact_file).parts == (artifact_file,),
            f"campaign preflight gate {gate} artifact file is not local",
        )
        artifact_path = preflight_path.parent / artifact_file
        artifact, actual_digest = read_canonical_json_with_sha256(
            artifact_path, f"campaign preflight gate {gate} artifact"
        )
        require(
            actual_digest == evidence["artifact_sha256"],
            f"campaign preflight gate {gate} artifact digest differs",
        )
        record = exact_keys(
            artifact,
            GATE_ARTIFACT_KEYS,
            f"campaign preflight gate {gate} artifact",
        )
        require(
            record["schema"] == GATE_ARTIFACT_SCHEMA,
            f"campaign preflight gate {gate} artifact schema differs",
        )
        require(
            record["gate"] == gate and record["status"] == "passed",
            f"campaign preflight gate {gate} artifact is not passed",
        )
        require(
            record["execution_scope"] == "offline_preflight",
            f"campaign preflight gate {gate} artifact entered the timed path",
        )
        require(
            record["git_sha"] == git_sha,
            f"campaign preflight gate {gate} artifact is from another Git SHA",
        )
        require(
            record["binary_sha256"] == binary_sha256,
            f"campaign preflight gate {gate} artifact targets another binary",
        )
        validate_gate_metrics(gate, record["metrics"])


def validate_preflight(
    value: Any,
    *,
    binary_sha256: str,
    git_sha: str,
    plan: dict[str, Any],
    preflight_path: Path,
    now_epoch: int | None = None,
) -> dict[str, Any]:
    preflight = exact_keys(
        value,
        {
            "backend",
            "binary_sha256",
            "component_measurement_used_as_gate",
            "environment",
            "gates",
            "git_sha",
            "mode",
            "profile",
            "proxy_measurement_used_as_gate",
            "schema",
        },
        "campaign preflight",
    )
    require(
        preflight["schema"] == PREFLIGHT_SCHEMA, "campaign preflight schema differs"
    )
    require(
        preflight["backend"] == BACKEND
        and preflight["profile"] == PROFILE
        and preflight["mode"] == MODE,
        "campaign preflight execution context differs",
    )
    require(
        preflight["binary_sha256"] == binary_sha256,
        "campaign preflight binary digest differs",
    )
    require(preflight["git_sha"] == git_sha, "campaign preflight Git SHA differs")
    require(
        preflight["component_measurement_used_as_gate"] is False,
        "component measurement cannot open the campaign",
    )
    require(
        preflight["proxy_measurement_used_as_gate"] is False,
        "proxy measurement cannot open the campaign",
    )
    environment = exact_keys(
        preflight["environment"],
        set(PREFLIGHT_ENVIRONMENT_TEMPLATE),
        "campaign preflight environment",
    )
    dynamic_environment_fields = {
        "gce_deadline_epoch",
        "guest_deadline_epoch",
        "guest_shutdown_delay_seconds",
        "instance_generation",
        "instance_name",
        "max_run_duration_seconds",
        "preflight_epoch",
        "zone",
    }
    for key, expected in PREFLIGHT_ENVIRONMENT_TEMPLATE.items():
        if key not in dynamic_environment_fields:
            require(
                type(environment[key]) is type(expected)
                and environment[key] == expected,
                f"campaign preflight environment.{key} differs",
            )
    for key in ("zone", "instance_name", "instance_generation"):
        require(
            isinstance(environment[key], str) and environment[key],
            f"campaign preflight environment.{key} is absent",
        )
    for key in (
        "preflight_epoch",
        "gce_deadline_epoch",
        "guest_deadline_epoch",
        "guest_shutdown_delay_seconds",
        "max_run_duration_seconds",
    ):
        natural(
            environment[key],
            f"campaign preflight environment.{key}",
            positive=True,
        )
    maximum_duration = environment["max_run_duration_seconds"]
    require(
        30 <= maximum_duration <= 8 * 60 * 60,
        "campaign GCE maxRunDuration is outside [30 seconds, 8 hours]",
    )
    registered_timeout_seconds = registered_remaining_timeout_seconds(plan)
    require(
        maximum_duration >= registered_timeout_seconds,
        "campaign GCE maxRunDuration cannot cover the registered run caps and subprocess allowances",
    )
    preflight_epoch = environment["preflight_epoch"]
    gce_deadline = environment["gce_deadline_epoch"]
    guest_deadline = environment["guest_deadline_epoch"]
    guest_delay = environment["guest_shutdown_delay_seconds"]
    validation_epoch = (
        int(time.time())
        if now_epoch is None
        else natural(now_epoch, "campaign preflight validation epoch", positive=True)
    )
    require(
        preflight_epoch <= validation_epoch,
        "campaign preflight timestamp is in the future",
    )
    require(
        preflight_epoch < guest_deadline <= gce_deadline,
        "campaign guest/GCE shutdown deadlines are not ordered",
    )
    require(
        gce_deadline - preflight_epoch <= maximum_duration,
        "campaign GCE deadline exceeds maxRunDuration",
    )
    require(
        30 <= guest_delay <= maximum_duration
        and guest_deadline - preflight_epoch <= guest_delay,
        "campaign guest shutdown delay is invalid",
    )
    require(
        validation_epoch < guest_deadline,
        "campaign guest shutdown guard expired before execution",
    )
    validate_remaining_deadline_coverage(
        environment,
        plan,
        0,
        now_epoch=validation_epoch,
    )

    gates = exact_keys(
        preflight["gates"], set(PREFLIGHT_GATES), "campaign preflight gates"
    )
    validate_preflight_gate_artifacts(
        gates,
        preflight_path=preflight_path,
        binary_sha256=binary_sha256,
        git_sha=git_sha,
    )
    return preflight


def _validate_limit_hit(
    reason: str, work: dict[str, int], caps: dict[str, int], timings: dict[str, int]
) -> None:
    counter_map = {
        "bank_update_cap": ("bank_updates", "bank_updates"),
        "classified_pair_cap": ("classified_pairs", "classified_pairs"),
        "frontier_cap": ("frontier_peak_records", "frontier_records"),
        "node_visit_cap": ("node_visits", "node_visits"),
        "output_pair_cap": ("output_pairs", "output_pairs"),
        "radial_receipt_cap": ("radial_receipts", "radial_receipts"),
    }
    if reason in counter_map:
        work_key, cap_key = counter_map[reason]
        require(
            work[work_key] == caps[cap_key],
            f"censor reason {reason} did not reach its cap",
        )
    elif reason == "wall_time_cap":
        require(
            timings["warm_e2e_fresh_cloud"] >= caps["wall_time"],
            "wall-time censure did not reach its cap",
        )
    elif reason == "device_memory_cap":
        require(
            work["device_peak_bytes"] * 1_000_000
            >= work["device_capacity_bytes"] * caps["device_memory_fraction_ppm"],
            "device-memory censure did not reach its cap",
        )
    elif reason == "host_memory_cap":
        require(
            work["host_peak_bytes"] * 1_000_000
            >= work["host_capacity_bytes"] * caps["host_memory_fraction_ppm"],
            "host-memory censure did not reach its cap",
        )


def validate_run(
    value: Any,
    request: dict[str, Any],
    *,
    binary_sha256: str | None = None,
    git_sha: str | None = None,
) -> dict[str, Any]:
    run = exact_keys(value, RUN_KEYS, "scale run")
    for key, expected in {
        "schema": RUN_SCHEMA,
        "algorithm_scope": ALGORITHM_SCOPE,
        "backend": BACKEND,
        "profile": PROFILE,
        "mode": MODE,
        "point_count": request["point_count"],
        "requested_order_max": request["requested_order_max"],
        "run_id": request["run_id"],
        "seed": request["seed"],
    }.items():
        require(run[key] == expected, f"scale run {key} differs")
    require(run["family"] == request["family"], "scale run family differs")
    cloud = exact_keys(run["cloud"], CLOUD_KEYS, "scale run cloud")
    require(
        cloud["request_sha256"] == request["cloud_request_sha256"],
        "scale run cloud is not bound to its family, size, and seed",
    )
    require(
        isinstance(cloud["canonical_binary64_sha256"], str)
        and SHA256_RE.fullmatch(cloud["canonical_binary64_sha256"]) is not None,
        "scale run canonical cloud digest is invalid",
    )
    require(
        cloud["fresh_for_measured_run"] is True
        and natural(
            cloud["construction_count"],
            "scale run cloud.construction_count",
            positive=True,
        )
        == 1,
        "scale run did not construct exactly one fresh measured cloud",
    )
    require(
        isinstance(run["binary_sha256"], str)
        and SHA256_RE.fullmatch(run["binary_sha256"]) is not None,
        "scale run binary digest is invalid",
    )
    require(
        isinstance(run["git_sha"], str)
        and SHA40_RE.fullmatch(run["git_sha"]) is not None,
        "scale run Git SHA is invalid",
    )
    if binary_sha256 is not None:
        require(
            run["binary_sha256"] == binary_sha256,
            "scale run binary digest is not the measured binary",
        )
    if git_sha is not None:
        require(
            run["git_sha"] == git_sha, "scale run Git SHA is not the measured revision"
        )

    implementation = exact_keys(
        run["implementation"], IMPLEMENTATION_KEYS, "scale run implementation"
    )
    require(
        implementation
        == {
            "complete_pair_frontier": True,
            "component_only": False,
            "dense_pair_scan": False,
            "delaunay": False,
            "emst": False,
            "global_pair_matrix": False,
            "offline_oracle_in_timed_path": False,
            "proxy": False,
            "uses_boruvka": False,
        },
        "scale run used a forbidden or incomplete implementation path",
    )
    caps = exact_keys(run["caps"], CAP_KEYS, "scale run caps")
    require(
        caps == request["caps"], "scale run caps differ from the registered linear caps"
    )
    closure = exact_keys(run["closure"], CLOSURE_KEYS, "scale run closure")
    natural_closure = {
        key: natural(closure[key], f"scale run closure.{key}")
        for key in (
            "accepted_pair_count",
            "candidate_pair_mass",
            "certified_pruned_pair_mass",
            "classifier_session_count",
            "total_unordered_pair_mass",
            "unique_candidate_pair_count",
            "rejected_pair_count",
            "unresolved_pair_mass",
        )
    }
    total_mass = request["point_count"] * (request["point_count"] - 1) // 2
    require(
        natural_closure["total_unordered_pair_mass"] == total_mass,
        "scale run total unordered-pair mass differs",
    )
    require(
        natural_closure["candidate_pair_mass"]
        + natural_closure["certified_pruned_pair_mass"]
        + natural_closure["unresolved_pair_mass"]
        == total_mass,
        "scale run unordered-pair mass identity is open",
    )
    require(
        natural_closure["unique_candidate_pair_count"]
        == natural_closure["candidate_pair_mass"],
        "scale run candidate mass is not unique",
    )
    require(
        natural_closure["classifier_session_count"]
        <= natural_closure["unique_candidate_pair_count"],
        "scale run classifier sessions exceed unique candidates",
    )
    require(
        natural_closure["accepted_pair_count"] + natural_closure["rejected_pair_count"]
        == natural_closure["classifier_session_count"],
        "scale run classifier decision partition is open",
    )
    for key in (
        "checkpoint_closed",
        "exact_replay_closed",
        "frontier_empty",
        "payload_closed",
    ):
        require(type(closure[key]) is bool, f"scale run closure.{key} must be boolean")

    timings = exact_keys(run["timings_ns"], TIMING_KEYS, "scale run timings")
    exact_timings = {
        key: natural(timings[key], f"scale run timings.{key}", positive=True)
        for key in TIMING_KEYS
    }
    for stage in (
        "morton_lbvh",
        "yao48_bank_fill",
        "certified_frontier",
        "exact_pair_classification",
        "compaction",
        "persistence",
    ):
        require(
            exact_timings[stage] <= exact_timings["warm_e2e_fresh_cloud"],
            f"scale run stage {stage} exceeds warm e2e",
        )
    require(
        sum(
            exact_timings[stage]
            for stage in (
                "morton_lbvh",
                "yao48_bank_fill",
                "certified_frontier",
                "exact_pair_classification",
                "compaction",
                "persistence",
            )
        )
        <= exact_timings["warm_e2e_fresh_cloud"],
        "scale run timed stages do not fit inside warm e2e",
    )

    work = exact_keys(run["work"], WORK_KEYS, "scale run work")
    exact_work = {key: natural(work[key], f"scale run work.{key}") for key in WORK_KEYS}
    require(
        exact_work["device_capacity_bytes"] > 0
        and exact_work["host_capacity_bytes"] > 0,
        "scale run memory capacities must be positive",
    )
    require(
        exact_work["classified_pairs"] == natural_closure["classifier_session_count"],
        "scale run classified-pair work differs from classifier sessions",
    )
    require(
        exact_work["output_pairs"] == natural_closure["accepted_pair_count"],
        "scale run output differs from accepted exact pairs",
    )
    require(
        exact_work["device_peak_bytes"] <= exact_work["device_capacity_bytes"]
        and exact_work["host_peak_bytes"] <= exact_work["host_capacity_bytes"],
        "scale run peak memory exceeds physical capacity",
    )
    require(
        exact_work["host_callbacks_per_pair"] == 0,
        "scale run used a host callback per pair",
    )
    require(
        exact_work["d2h_intermediate_candidate_batches"] == 0,
        "scale run copied intermediate candidates to the host",
    )
    require(
        exact_work["dense_fallback_pair_visits"] == 0, "scale run used a dense fallback"
    )
    for work_key, cap_key in (
        ("bank_updates", "bank_updates"),
        ("classified_pairs", "classified_pairs"),
        ("frontier_peak_records", "frontier_records"),
        ("node_visits", "node_visits"),
        ("output_pairs", "output_pairs"),
        ("radial_receipts", "radial_receipts"),
    ):
        require(exact_work[work_key] <= caps[cap_key], f"scale run exceeded {cap_key}")

    status = run["status"]
    require(status in {"complete", "censored"}, "scale run status is invalid")
    if status == "complete":
        require(run["censor_reason"] is None, "complete scale run has a censor reason")
        require(
            natural_closure["unresolved_pair_mass"] == 0,
            "complete scale run retains unresolved pair mass",
        )
        require(
            closure["frontier_empty"]
            and closure["payload_closed"]
            and closure["exact_replay_closed"]
            and closure["checkpoint_closed"],
            "complete scale run did not close every certificate",
        )
        require(
            natural_closure["candidate_pair_mass"]
            == natural_closure["unique_candidate_pair_count"]
            == natural_closure["classifier_session_count"],
            "complete scale run candidate classification is not one-to-one",
        )
        require(
            exact_timings["warm_e2e_fresh_cloud"] <= caps["wall_time"],
            "complete scale run exceeded its wall-time cap",
        )
        require(
            exact_work["device_peak_bytes"] * 1_000_000
            <= exact_work["device_capacity_bytes"] * caps["device_memory_fraction_ppm"],
            "complete scale run exceeded its device-memory cap",
        )
        require(
            exact_work["host_peak_bytes"] * 1_000_000
            <= exact_work["host_capacity_bytes"] * caps["host_memory_fraction_ppm"],
            "complete scale run exceeded its host-memory cap",
        )
    else:
        require(
            run["censor_reason"] in CENSOR_REASONS,
            "censored scale run has an invalid reason",
        )
        reason = run["censor_reason"]
        require(
            closure["checkpoint_closed"],
            "censored scale run did not close its checkpoint",
        )
        require(
            not closure["frontier_empty"]
            and natural_closure["unresolved_pair_mass"] > 0,
            "censored scale run must retain an explicit unresolved frontier",
        )
        if reason != "wall_time_cap":
            require(
                exact_timings["warm_e2e_fresh_cloud"] <= caps["wall_time"],
                "censored scale run exceeded a non-triggering wall-time cap",
            )
        else:
            require(
                exact_timings["warm_e2e_fresh_cloud"]
                <= caps["wall_time"] + 30_000_000_000,
                "wall-time censure exceeded its guarded return allowance",
            )
        if reason != "device_memory_cap":
            require(
                exact_work["device_peak_bytes"] * 1_000_000
                <= exact_work["device_capacity_bytes"]
                * caps["device_memory_fraction_ppm"],
                "censored scale run exceeded a non-triggering device-memory cap",
            )
        if reason != "host_memory_cap":
            require(
                exact_work["host_peak_bytes"] * 1_000_000
                <= exact_work["host_capacity_bytes"] * caps["host_memory_fraction_ppm"],
                "censored scale run exceeded a non-triggering host-memory cap",
            )
        _validate_limit_hit(reason, exact_work, caps, exact_timings)
    return run


def nearest_rank(samples: list[int], percentile: int) -> int:
    require(samples, "cannot compute a quantile without samples")
    require(0 < percentile <= 100, "quantile is invalid")
    ordered = sorted(samples)
    rank = (len(ordered) * percentile + 99) // 100
    return ordered[rank - 1]


def run_binary_json(
    binary: Path,
    argument: str,
    *,
    stdin_value: Any | None,
    timeout_seconds: float,
    label: str,
) -> Any:
    payload = (
        None
        if stdin_value is None
        else (canonical_json(stdin_value) + "\n").encode("ascii")
    )
    try:
        completed = subprocess.run(
            [str(binary), argument],
            input=payload,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=timeout_seconds,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        fail(f"{label} could not execute: {error}")
    require(
        completed.returncode == 0, f"{label} failed with status {completed.returncode}"
    )
    require(not completed.stderr, f"{label} emitted stderr")
    try:
        raw = completed.stdout.decode("ascii")
    except UnicodeDecodeError as error:
        fail(f"{label} did not emit ASCII: {error}")
    return parse_json_text(raw, label, canonical_line=True)


def build_scale_summary(
    scale: dict[str, Any], runs: list[dict[str, Any]]
) -> dict[str, Any]:
    samples = [
        run["timings_ns"]["warm_e2e_fresh_cloud"]
        for run in runs
        if run["status"] == "complete"
    ]
    expected_count = len(FAMILY_IDS) * len(scale["seeds"])
    all_complete = len(runs) == expected_count and len(samples) == expected_count
    p50 = nearest_rank(samples, 50) if samples else None
    p95 = nearest_rank(samples, 95) if samples else None
    slo = scale["slo_p95_warm_e2e_ns"]
    status = "complete"
    reason = None
    if not all_complete:
        status = "censored"
        reason = "run_budget_exhausted"
    elif slo is not None and p95 is not None and p95 >= slo:
        status = "censored"
        reason = "p95_slo_not_met"
    return {
        "censor_reason": reason,
        "complete_run_count": len(samples),
        "censored_run_count": sum(run["status"] == "censored" for run in runs),
        "expected_run_count": expected_count,
        "executed_run_count": len(runs),
        "p50_warm_e2e_fresh_cloud_ns": p50,
        "p95_warm_e2e_fresh_cloud_ns": p95,
        "point_count": scale["point_count"],
        "slo_p95_warm_e2e_ns": slo,
        "status": status,
    }


def next_registered_point_count(
    requests: list[dict[str, Any]],
    request_index: int,
    blocked_point_count: int | None,
) -> int | None:
    if blocked_point_count is not None:
        return blocked_point_count
    return (
        requests[request_index]["point_count"]
        if request_index < len(requests)
        else None
    )


def register_fresh_cloud_digest(seen_digests: set[str], run: dict[str, Any]) -> None:
    digest = run["cloud"]["canonical_binary64_sha256"]
    require(
        digest not in seen_digests,
        "campaign reused one canonical cloud in multiple measured runs",
    )
    seen_digests.add(digest)


def write_new_canonical_json(path: Path, value: Any) -> None:
    require(
        not path.exists() and not path.is_symlink(), "campaign output already exists"
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = (canonical_json(value) + "\n").encode("ascii")
    try:
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
    except OSError as error:
        fail(f"cannot publish campaign output: {error}")


def execute_campaign(
    *,
    plan: dict[str, Any],
    plan_path: Path,
    binary: Path,
    preflight_path: Path,
    git_sha: str,
    output: Path,
) -> dict[str, Any]:
    require(
        SHA40_RE.fullmatch(git_sha) is not None,
        "--git-sha must be a lowercase 40-hex commit ID",
    )
    require(
        binary.is_file() and not binary.is_symlink(),
        "campaign execution blocked: complete pair-first binary is absent",
    )
    require(
        os.access(binary, os.X_OK),
        "campaign execution blocked: complete pair-first binary is not executable",
    )
    plan_digest = sha256_file(plan_path, "campaign plan")
    require(
        validate_plan(read_json(plan_path, "campaign plan during execution")) == plan,
        "campaign plan changed after validation",
    )
    binary_digest = sha256_file(binary, "complete pair-first binary")
    preflight_digest = sha256_file(preflight_path, "campaign preflight")
    preflight = validate_preflight(
        read_json(preflight_path, "campaign preflight", canonical_line=True),
        binary_sha256=binary_digest,
        git_sha=git_sha,
        plan=plan,
        preflight_path=preflight_path,
    )
    require(
        sha256_file(binary, "complete pair-first binary before capability handshake")
        == binary_digest,
        "complete pair-first binary changed before capability handshake",
    )
    capabilities = validate_capabilities(
        run_binary_json(
            binary,
            plan["binary_protocol"]["capability_argument"],
            stdin_value=None,
            timeout_seconds=10.0,
            label="complete pair-first capability handshake",
        ),
        plan,
    )
    require(
        sha256_file(binary, "complete pair-first binary after capability handshake")
        == binary_digest,
        "complete pair-first binary changed during capability handshake",
    )
    requests = expected_run_requests(plan)
    runs: list[dict[str, Any]] = []
    summaries: list[dict[str, Any]] = []
    campaign_status = "complete"
    censor_reason = None
    blocked_point_count = None
    request_index = 0
    seen_cloud_digests: set[str] = set()
    for scale in plan["scales"]:
        scale_runs: list[dict[str, Any]] = []
        expected_at_scale = len(FAMILY_IDS) * len(scale["seeds"])
        for _ in range(expected_at_scale):
            request = requests[request_index]
            validate_remaining_deadline_coverage(
                preflight["environment"], plan, request_index
            )
            validate_preflight_gate_artifacts(
                preflight["gates"],
                preflight_path=preflight_path,
                binary_sha256=binary_digest,
                git_sha=git_sha,
            )
            require(
                sha256_file(binary, "complete pair-first binary before measured run")
                == binary_digest,
                "complete pair-first binary changed before a measured run",
            )
            timeout = (
                scale["wall_time_cap_ms"] / 1000.0
                + SUBPROCESS_TIMEOUT_ALLOWANCE_SECONDS
            )
            value = run_binary_json(
                binary,
                plan["binary_protocol"]["run_argument"],
                stdin_value=request,
                timeout_seconds=timeout,
                label=f"complete pair-first run {request['run_id']}",
            )
            require(
                sha256_file(binary, "complete pair-first binary after measured run")
                == binary_digest,
                "complete pair-first binary changed during a measured run",
            )
            run = validate_run(
                value, request, binary_sha256=binary_digest, git_sha=git_sha
            )
            register_fresh_cloud_digest(seen_cloud_digests, run)
            runs.append(run)
            scale_runs.append(run)
            request_index += 1
            if run["status"] == "censored":
                break
        summary = build_scale_summary(scale, scale_runs)
        summaries.append(summary)
        if summary["status"] == "censored":
            campaign_status = "censored"
            censor_reason = summary["censor_reason"]
            blocked_point_count = scale["point_count"]
            break

    next_point_count = next_registered_point_count(
        requests,
        request_index,
        blocked_point_count,
    )
    require(
        sha256_file(plan_path, "campaign plan after execution") == plan_digest,
        "campaign plan changed while runs were executing",
    )
    require(
        sha256_file(preflight_path, "campaign preflight after execution")
        == preflight_digest,
        "campaign preflight changed while runs were executing",
    )
    artifact = {
        "algorithm_scope": ALGORITHM_SCOPE,
        "artifact_role": "benchmark_worker_result_pending_guarded_shutdown",
        "backend": BACKEND,
        "binary": {"capabilities": capabilities, "sha256": binary_digest},
        "campaign_status": campaign_status,
        "censor_reason": censor_reason,
        "git_sha": git_sha,
        "mode": MODE,
        "next_point_count": next_point_count,
        "plan_sha256": plan_digest,
        "preflight": preflight,
        "preflight_sha256": preflight_digest,
        "profile": PROFILE,
        "qualification_claimed": False,
        "runs": runs,
        "scalability_claimed": False,
        "schema": CAMPAIGN_SCHEMA,
        "scientific_public_status": None,
        "scale_summaries": summaries,
    }
    write_new_canonical_json(output, artifact)
    return artifact


def request_for_run(run: dict[str, Any], plan: dict[str, Any]) -> dict[str, Any]:
    matches = [
        request
        for request in expected_run_requests(plan)
        if request["run_id"] == run.get("run_id")
    ]
    require(len(matches) == 1, "scale run is not registered in the campaign")
    return matches[0]


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("emit-plan")
    validate_run_parser = subparsers.add_parser("validate-run")
    validate_run_parser.add_argument("run", type=Path)
    run_parser = subparsers.add_parser("run")
    run_parser.add_argument("--binary", type=Path, required=True)
    run_parser.add_argument("--preflight", type=Path, required=True)
    run_parser.add_argument("--git-sha", required=True)
    run_parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    try:
        args = parse_arguments(arguments)
        plan = validate_plan(read_json(args.plan, "campaign plan"))
        if args.command == "emit-plan":
            print(canonical_json(plan))
        elif args.command == "validate-run":
            run = read_json(args.run, "scale run", canonical_line=True)
            validate_run(run, request_for_run(run, plan))
            print(
                canonical_json(
                    {
                        "run_id": run["run_id"],
                        "schema": RUN_SCHEMA,
                        "status": run["status"],
                    }
                )
            )
        elif args.command == "run":
            artifact = execute_campaign(
                plan=plan,
                plan_path=args.plan,
                binary=args.binary,
                preflight_path=args.preflight,
                git_sha=args.git_sha,
                output=args.output,
            )
            print(
                canonical_json(
                    {
                        "campaign_status": artifact["campaign_status"],
                        "next_point_count": artifact["next_point_count"],
                        "schema": CAMPAIGN_SCHEMA,
                    }
                )
            )
        else:
            fail("unknown campaign command")
    except ContractError as error:
        print(f"Phase 15 pair-first scale campaign blocked: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
