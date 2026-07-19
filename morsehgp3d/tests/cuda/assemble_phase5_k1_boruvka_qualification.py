#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 5 K1 Boruvka G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v2"
REPLAY_SCHEMA = "morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v1"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
SCIENTIFIC_SCOPE = "gpu_proposed_cpu_exact_full_boruvka_local_emst_witness_only"
PROPOSAL_SEMANTICS = "gpu_stackless_lbvh_fixed_seed_candidate_superset"
DECISION_SEMANTICS = "cpu_exact_seed_replay_and_kappa_resolution"
PROOF_BASIS = "gpu_candidate_superset_cpu_exact_boruvka_v1"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
SANITIZER_ERROR_RE = re.compile(r"ERROR SUMMARY:\s*([0-9]+) errors")
RACECHECK_SUMMARY_LINE_RE = re.compile(
    r"^[ \t]*=+[ \t]*RACECHECK SUMMARY:[ \t]*"
    r"([0-9]+) hazards? displayed[ \t]*"
    r"\([ \t]*([0-9]+) errors?,[ \t]*([0-9]+) warnings?[ \t]*\)"
    r"[ \t]*$",
    re.MULTILINE,
)
RACECHECK_FAILURE_RE = re.compile(
    r"Internal Sanitizer Error|Target application returned an error|"
    r"process (?:didn't|did not) terminate successfully|"
    r"No attachable process found|Potential [^\n]* hazard|Race reported",
    re.IGNORECASE,
)
REPLAY_KEYS = {
    "case_count",
    "cases",
    "decision_backend",
    "decision_semantics",
    "hierarchy_reduction_status",
    "mode",
    "phase",
    "profile",
    "proof_basis",
    "proposal_backend",
    "proposal_semantics",
    "schema",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "verification_proposal_backend",
}
CASE_KEYS = {
    "component_count_path",
    "emst_edge_count",
    "exact_weights",
    "fixture",
    "point_count",
    "producer",
    "rounds",
    "status",
    "verifier",
}
LEVEL_KEYS = {"denominator", "numerator"}
PRODUCER_KEYS = {"certificates", "counters"}
PRODUCER_CERTIFICATE_KEYS = {
    "canonical_contraction_chain_certified",
    "cpu_exact_decision_chain_certified",
    "emst_witness_certified",
    "proposal_chain_certified",
    "reference_cpu_witness_certified",
}
PRODUCER_COUNTER_KEYS = {
    "accepted_edge_count",
    "component_contraction_count",
    "component_minimum_count",
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "cpu_required_candidate_count",
    "final_component_count",
    "first_buffer_epoch",
    "frozen_component_label_count",
    "gpu_candidate_count",
    "gpu_count_pass_node_visit_count",
    "gpu_emit_pass_node_visit_count",
    "gpu_invalid_bound_descent_count",
    "gpu_kernel_launch_count",
    "gpu_output_capacity_sum",
    "gpu_strict_aabb_prune_count",
    "gpu_synchronization_count",
    "gpu_uniform_component_prune_count",
    "last_buffer_epoch",
    "lbvh_node_count",
    "peak_gpu_output_capacity",
    "point_count",
    "round_count",
    "theoretical_max_round_count",
}
ROUND_KEYS = {"audit", "contraction", "decision", "proposal_status"}
AUDIT_KEYS = {
    "buffer_epoch",
    "candidate_count",
    "certificates",
    "count_pass_node_visit_count",
    "cpu_exact_aabb_bound_evaluation_count",
    "cpu_exact_candidate_distance_evaluation_count",
    "emit_pass_node_visit_count",
    "exact_seed_count",
    "invalid_bound_descent_count",
    "kernel_launch_count",
    "mixed_lbvh_node_count",
    "output_capacity",
    "proposal_digest_fnv1a",
    "required_candidate_count",
    "resident_node_count",
    "resident_point_count",
    "strict_aabb_prune_count",
    "synchronization_count",
    "uniform_component_prune_count",
}
AUDIT_CERTIFICATE_KEYS = {
    "candidate_superset_certified",
    "cpu_exact_resolution_complete",
    "exact_capacity_certified",
    "frozen_labels_certified",
    "no_truncation_certified",
    "rope_topology_certified",
}
CONTRACTION_KEYS = {
    "accepted_edge_count",
    "post_round_component_count",
    "status",
}
DECISION_KEYS = {
    "component_minimum_count",
    "frozen_component_count",
    "round_index",
    "status",
}
VERIFIER_KEYS = {"certificates", "counters"}
VERIFIER_CERTIFICATE_KEYS = {
    "canonical_contractions_certified",
    "counters_certified",
    "cpu_exact_decision_chain_certified",
    "emst_witness_certified",
    "exact_weights_certified",
    "hierarchy_status_separation_certified",
    "index_identity_certified",
    "proposal_chain_certified",
    "reference_cpu_witness_certified",
    "round_count_bound_certified",
    "spanning_tree_certified",
}
VERIFIER_COUNTER_KEYS = {
    "gpu_replay_kernel_launch_count",
    "gpu_replay_synchronization_count",
    "gpu_replayed_component_minimum_count",
    "gpu_replayed_round_count",
    "reference_component_minimum_count",
    "reference_round_count",
}
TRUE_PRODUCER_CERTIFICATES = {
    key: True for key in sorted(PRODUCER_CERTIFICATE_KEYS)
}
TRUE_AUDIT_CERTIFICATES = {
    key: True for key in sorted(AUDIT_CERTIFICATE_KEYS)
}
TRUE_VERIFIER_CERTIFICATES = {
    key: True for key in sorted(VERIFIER_CERTIFICATE_KEYS)
}
EXPECTED_FIXTURES = (
    {
        "component_count_path": [1],
        "emst_edge_count": 0,
        "fixture": "singleton_terminal",
        "hgp_weight": ("0", "1"),
        "point_count": 1,
        "producer_counters": {
            "accepted_edge_count": 0,
            "component_contraction_count": 0,
            "component_minimum_count": 0,
            "cpu_exact_aabb_bound_evaluation_count": 0,
            "cpu_exact_candidate_distance_evaluation_count": 0,
            "cpu_required_candidate_count": 0,
            "final_component_count": 1,
            "first_buffer_epoch": 0,
            "frozen_component_label_count": 0,
            "gpu_candidate_count": 0,
            "gpu_count_pass_node_visit_count": 0,
            "gpu_emit_pass_node_visit_count": 0,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 0,
            "gpu_output_capacity_sum": 0,
            "gpu_strict_aabb_prune_count": 0,
            "gpu_synchronization_count": 0,
            "gpu_uniform_component_prune_count": 0,
            "last_buffer_epoch": 0,
            "lbvh_node_count": 1,
            "peak_gpu_output_capacity": 0,
            "point_count": 1,
            "round_count": 0,
            "theoretical_max_round_count": 0,
        },
        "rounds": (),
        "squared_weight": ("0", "1"),
        "verifier_counters": {
            "gpu_replay_kernel_launch_count": 0,
            "gpu_replay_synchronization_count": 0,
            "gpu_replayed_component_minimum_count": 0,
            "gpu_replayed_round_count": 0,
            "reference_component_minimum_count": 0,
            "reference_round_count": 0,
        },
    },
    {
        "component_count_path": [8, 4, 2, 1],
        "emst_edge_count": 7,
        "fixture": "chain_three_rounds",
        "hgp_weight": ("8127", "4"),
        "point_count": 8,
        "producer_counters": {
            "accepted_edge_count": 7,
            "component_contraction_count": 7,
            "component_minimum_count": 14,
            "cpu_exact_aabb_bound_evaluation_count": 212,
            "cpu_exact_candidate_distance_evaluation_count": 86,
            "cpu_required_candidate_count": 86,
            "final_component_count": 1,
            "first_buffer_epoch": 1,
            "frozen_component_label_count": 24,
            "gpu_candidate_count": 86,
            "gpu_count_pass_node_visit_count": 236,
            "gpu_emit_pass_node_visit_count": 236,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 6,
            "gpu_output_capacity_sum": 86,
            "gpu_strict_aabb_prune_count": 20,
            "gpu_synchronization_count": 6,
            "gpu_uniform_component_prune_count": 24,
            "last_buffer_epoch": 3,
            "lbvh_node_count": 15,
            "peak_gpu_output_capacity": 36,
            "point_count": 8,
            "round_count": 3,
            "theoretical_max_round_count": 3,
        },
        "rounds": (
            {
                "audit": {
                    "buffer_epoch": 1,
                    "candidate_count": 36,
                    "count_pass_node_visit_count": 92,
                    "cpu_exact_aabb_bound_evaluation_count": 84,
                    "cpu_exact_candidate_distance_evaluation_count": 36,
                    "emit_pass_node_visit_count": 92,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 2,
                    "mixed_lbvh_node_count": 7,
                    "output_capacity": 36,
                    "proposal_digest_fnv1a": "88d6d04ed0c2e06b",
                    "required_candidate_count": 36,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 6,
                    "synchronization_count": 2,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 4,
                    "post_round_component_count": 4,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 8,
                    "frozen_component_count": 8,
                    "round_index": 0,
                    "status": "cpu_exact_kappa_minima_certified",
                },
            },
            {
                "audit": {
                    "buffer_epoch": 2,
                    "candidate_count": 30,
                    "count_pass_node_visit_count": 80,
                    "cpu_exact_aabb_bound_evaluation_count": 72,
                    "cpu_exact_candidate_distance_evaluation_count": 30,
                    "emit_pass_node_visit_count": 80,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 2,
                    "mixed_lbvh_node_count": 3,
                    "output_capacity": 30,
                    "proposal_digest_fnv1a": "8ba818d0f95004dd",
                    "required_candidate_count": 30,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 6,
                    "synchronization_count": 2,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 2,
                    "post_round_component_count": 2,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 4,
                    "frozen_component_count": 4,
                    "round_index": 1,
                    "status": "cpu_exact_kappa_minima_certified",
                },
            },
            {
                "audit": {
                    "buffer_epoch": 3,
                    "candidate_count": 20,
                    "count_pass_node_visit_count": 64,
                    "cpu_exact_aabb_bound_evaluation_count": 56,
                    "cpu_exact_candidate_distance_evaluation_count": 20,
                    "emit_pass_node_visit_count": 64,
                    "exact_seed_count": 8,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 2,
                    "mixed_lbvh_node_count": 1,
                    "output_capacity": 20,
                    "proposal_digest_fnv1a": "4ca55683c3c49441",
                    "required_candidate_count": 20,
                    "resident_node_count": 15,
                    "resident_point_count": 8,
                    "strict_aabb_prune_count": 8,
                    "synchronization_count": 2,
                    "uniform_component_prune_count": 8,
                },
                "contraction": {
                    "accepted_edge_count": 1,
                    "post_round_component_count": 1,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 2,
                    "frozen_component_count": 2,
                    "round_index": 2,
                    "status": "cpu_exact_kappa_minima_certified",
                },
            },
        ),
        "squared_weight": ("8127", "1"),
        "verifier_counters": {
            "gpu_replay_kernel_launch_count": 6,
            "gpu_replay_synchronization_count": 6,
            "gpu_replayed_component_minimum_count": 14,
            "gpu_replayed_round_count": 3,
            "reference_component_minimum_count": 14,
            "reference_round_count": 3,
        },
    },
    {
        "component_count_path": [4, 1],
        "emst_edge_count": 3,
        "fixture": "square_equal_length_ties",
        "hgp_weight": ("3", "1"),
        "point_count": 4,
        "producer_counters": {
            "accepted_edge_count": 3,
            "component_contraction_count": 3,
            "component_minimum_count": 4,
            "cpu_exact_aabb_bound_evaluation_count": 24,
            "cpu_exact_candidate_distance_evaluation_count": 9,
            "cpu_required_candidate_count": 9,
            "final_component_count": 1,
            "first_buffer_epoch": 1,
            "frozen_component_label_count": 4,
            "gpu_candidate_count": 9,
            "gpu_count_pass_node_visit_count": 28,
            "gpu_emit_pass_node_visit_count": 28,
            "gpu_invalid_bound_descent_count": 0,
            "gpu_kernel_launch_count": 2,
            "gpu_output_capacity_sum": 9,
            "gpu_strict_aabb_prune_count": 3,
            "gpu_synchronization_count": 2,
            "gpu_uniform_component_prune_count": 4,
            "last_buffer_epoch": 1,
            "lbvh_node_count": 7,
            "peak_gpu_output_capacity": 9,
            "point_count": 4,
            "round_count": 1,
            "theoretical_max_round_count": 2,
        },
        "rounds": (
            {
                "audit": {
                    "buffer_epoch": 1,
                    "candidate_count": 9,
                    "count_pass_node_visit_count": 28,
                    "cpu_exact_aabb_bound_evaluation_count": 24,
                    "cpu_exact_candidate_distance_evaluation_count": 9,
                    "emit_pass_node_visit_count": 28,
                    "exact_seed_count": 4,
                    "invalid_bound_descent_count": 0,
                    "kernel_launch_count": 2,
                    "mixed_lbvh_node_count": 3,
                    "output_capacity": 9,
                    "proposal_digest_fnv1a": "84154a051a6fc1af",
                    "required_candidate_count": 9,
                    "resident_node_count": 7,
                    "resident_point_count": 4,
                    "strict_aabb_prune_count": 3,
                    "synchronization_count": 2,
                    "uniform_component_prune_count": 4,
                },
                "contraction": {
                    "accepted_edge_count": 3,
                    "post_round_component_count": 1,
                    "status": "cpu_exact_canonical_contraction_certified",
                },
                "decision": {
                    "component_minimum_count": 4,
                    "frozen_component_count": 4,
                    "round_index": 0,
                    "status": "cpu_exact_kappa_minima_certified",
                },
            },
        ),
        "squared_weight": ("12", "1"),
        "verifier_counters": {
            "gpu_replay_kernel_launch_count": 2,
            "gpu_replay_synchronization_count": 2,
            "gpu_replayed_component_minimum_count": 4,
            "gpu_replayed_round_count": 1,
            "reference_component_minimum_count": 4,
            "reference_round_count": 1,
        },
    },
)
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON object key: {key}")
        value[key] = item
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def exact_json_equal(actual: Any, expected: Any) -> bool:
    if type(actual) is not type(expected):
        return False
    if isinstance(expected, dict):
        return set(actual) == set(expected) and all(
            exact_json_equal(actual[key], item) for key, item in expected.items()
        )
    if isinstance(expected, list):
        return len(actual) == len(expected) and all(
            exact_json_equal(left, right)
            for left, right in zip(actual, expected, strict=True)
        )
    return bool(actual == expected)


def contains_json_key(value: Any, key: str) -> bool:
    if isinstance(value, dict):
        return key in value or any(
            contains_json_key(item, key) for item in value.values()
        )
    if isinstance(value, list):
        return any(contains_json_key(item, key) for item in value)
    return False


def expected_level(weight: tuple[str, str]) -> dict[str, str]:
    numerator, denominator = weight
    return {"denominator": denominator, "numerator": numerator}


def expected_round(specification: dict[str, Any]) -> dict[str, Any]:
    audit = dict(specification["audit"])
    audit["certificates"] = dict(TRUE_AUDIT_CERTIFICATES)
    return {
        "audit": audit,
        "contraction": dict(specification["contraction"]),
        "decision": dict(specification["decision"]),
        "proposal_status": "candidate_superset_certified",
    }


def expected_case(specification: dict[str, Any]) -> dict[str, Any]:
    return {
        "component_count_path": list(specification["component_count_path"]),
        "emst_edge_count": specification["emst_edge_count"],
        "exact_weights": {
            "hgp": expected_level(specification["hgp_weight"]),
            "squared": expected_level(specification["squared_weight"]),
        },
        "fixture": specification["fixture"],
        "point_count": specification["point_count"],
        "producer": {
            "certificates": dict(TRUE_PRODUCER_CERTIFICATES),
            "counters": dict(specification["producer_counters"]),
        },
        "rounds": [
            expected_round(round_specification)
            for round_specification in specification["rounds"]
        ],
        "status": "passed",
        "verifier": {
            "certificates": dict(TRUE_VERIFIER_CERTIFICATES),
            "counters": dict(specification["verifier_counters"]),
        },
    }


EXPECTED_REPLAY = {
    "case_count": 3,
    "cases": [expected_case(specification) for specification in EXPECTED_FIXTURES],
    "decision_backend": "reference_cpu",
    "decision_semantics": DECISION_SEMANTICS,
    "hierarchy_reduction_status": "not_performed",
    "mode": "certified",
    "phase": "5",
    "profile": "hgp_reduced",
    "proof_basis": PROOF_BASIS,
    "proposal_backend": "cuda_g4",
    "proposal_semantics": PROPOSAL_SEMANTICS,
    "schema": REPLAY_SCHEMA,
    "scientific_result_claimed": False,
    "scientific_scope": "local_emst_witness_only",
    "status": "passed",
    "verification_proposal_backend": "cuda_g4",
}


def read_json_object(path: Path, label: str) -> tuple[str, dict[str, Any]]:
    try:
        raw = path.read_text(encoding="utf-8")
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    return raw, value


def read_text(path: Path, label: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{label} is empty")
    return value


def sha256_file(path: Path, label: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label}: {error}")
    return digest.hexdigest()


def validate_replay(path: Path) -> tuple[str, dict[str, Any]]:
    label = "Phase 5 full-loop K1 Boruvka replay"
    raw, value = read_json_object(path, label)
    canonical = json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    )
    if raw != canonical + "\n":
        fail(f"{label} must be canonical one-line JSON")
    require_exact_keys(value, REPLAY_KEYS, label)
    cases = value.get("cases")
    if not isinstance(cases, list) or len(cases) != len(EXPECTED_FIXTURES):
        fail(f"{label} must contain exactly the three closed fixtures")
    for case_index, case in enumerate(cases):
        case_label = f"{label} case {case_index}"
        if not isinstance(case, dict):
            fail(f"{case_label} must be an object")
        require_exact_keys(case, CASE_KEYS, case_label)
        weights = case.get("exact_weights")
        if not isinstance(weights, dict):
            fail(f"{case_label} exact_weights must be an object")
        require_exact_keys(weights, {"hgp", "squared"}, f"{case_label} weights")
        for level_name in ("hgp", "squared"):
            level = weights.get(level_name)
            if not isinstance(level, dict):
                fail(f"{case_label} {level_name} weight must be an object")
            require_exact_keys(level, LEVEL_KEYS, f"{case_label} {level_name} weight")

        producer = case.get("producer")
        if not isinstance(producer, dict):
            fail(f"{case_label} producer must be an object")
        require_exact_keys(producer, PRODUCER_KEYS, f"{case_label} producer")
        producer_certificates = producer.get("certificates")
        producer_counters = producer.get("counters")
        if not isinstance(producer_certificates, dict):
            fail(f"{case_label} producer certificates must be an object")
        if not isinstance(producer_counters, dict):
            fail(f"{case_label} producer counters must be an object")
        require_exact_keys(
            producer_certificates,
            PRODUCER_CERTIFICATE_KEYS,
            f"{case_label} producer certificates",
        )
        require_exact_keys(
            producer_counters,
            PRODUCER_COUNTER_KEYS,
            f"{case_label} producer counters",
        )

        rounds = case.get("rounds")
        if not isinstance(rounds, list):
            fail(f"{case_label} rounds must be an array")
        for round_index, round_value in enumerate(rounds):
            round_label = f"{case_label} round {round_index}"
            if not isinstance(round_value, dict):
                fail(f"{round_label} must be an object")
            require_exact_keys(round_value, ROUND_KEYS, round_label)
            audit = round_value.get("audit")
            contraction = round_value.get("contraction")
            decision = round_value.get("decision")
            if not isinstance(audit, dict):
                fail(f"{round_label} audit must be an object")
            if not isinstance(contraction, dict):
                fail(f"{round_label} contraction must be an object")
            if not isinstance(decision, dict):
                fail(f"{round_label} decision must be an object")
            require_exact_keys(audit, AUDIT_KEYS, f"{round_label} audit")
            require_exact_keys(
                contraction,
                CONTRACTION_KEYS,
                f"{round_label} contraction",
            )
            require_exact_keys(decision, DECISION_KEYS, f"{round_label} decision")
            audit_certificates = audit.get("certificates")
            if not isinstance(audit_certificates, dict):
                fail(f"{round_label} audit certificates must be an object")
            require_exact_keys(
                audit_certificates,
                AUDIT_CERTIFICATE_KEYS,
                f"{round_label} audit certificates",
            )

        verifier = case.get("verifier")
        if not isinstance(verifier, dict):
            fail(f"{case_label} verifier must be an object")
        require_exact_keys(verifier, VERIFIER_KEYS, f"{case_label} verifier")
        verifier_certificates = verifier.get("certificates")
        verifier_counters = verifier.get("counters")
        if not isinstance(verifier_certificates, dict):
            fail(f"{case_label} verifier certificates must be an object")
        if not isinstance(verifier_counters, dict):
            fail(f"{case_label} verifier counters must be an object")
        require_exact_keys(
            verifier_certificates,
            VERIFIER_CERTIFICATE_KEYS,
            f"{case_label} verifier certificates",
        )
        require_exact_keys(
            verifier_counters,
            VERIFIER_COUNTER_KEYS,
            f"{case_label} verifier counters",
        )

    if contains_json_key(value, "public_status"):
        fail(f"{label} must not contain public_status at any depth")
    if not exact_json_equal(value, EXPECTED_REPLAY):
        fail(f"{label} differs from the closed host fixture")
    return raw, value


def validate_memcheck_log(value: str) -> None:
    summaries = [int(match) for match in SANITIZER_ERROR_RE.findall(value)]
    if not summaries or any(count != 0 for count in summaries):
        fail("memcheck evidence must contain only zero-error summaries")


def validate_racecheck_log(value: str) -> None:
    summary_lines = [
        line for line in value.splitlines() if "RACECHECK SUMMARY:" in line
    ]
    summaries = [
        tuple(int(count) for count in match.groups())
        for match in RACECHECK_SUMMARY_LINE_RE.finditer(value)
    ]
    invalid = (
        not summaries
        or len(summary_lines) != len(summaries)
        or any(summary != (0, 0, 0) for summary in summaries)
        or SANITIZER_ERROR_RE.search(value) is not None
        or RACECHECK_FAILURE_RE.search(value) is not None
    )
    if invalid:
        fail(
            "racecheck evidence must contain only zero-hazard, zero-error, "
            "zero-warning summaries"
        )


def validate_binary_logs(
    *,
    elf_log: str,
    ptx_log: str,
    memcheck_log: str,
    racecheck_log: str,
) -> list[str]:
    architectures = SM_RE.findall(elf_log)
    if not architectures or set(architectures) != {"sm_120"}:
        fail("ELF evidence must be non-empty and contain only sm_120")
    if ptx_log.strip():
        fail("PTX evidence must contain zero entries")
    validate_memcheck_log(memcheck_log)
    validate_racecheck_log(racecheck_log)
    return sorted(set(architectures))


def validate_phase3_environment(
    value: dict[str, Any],
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> dict[str, Any]:
    if value.get("schema") != PHASE3_SCHEMA:
        fail("environment artifact has the wrong Phase 3 schema")
    expected_scalars = {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"environment artifact field {field} differs")
    if value.get("scientific_result_claimed") is not False:
        fail("environment artifact must not claim a scientific result")
    if value.get("scientific_public_status") is not None:
        fail("environment artifact scientific_public_status must be null")
    if "public_status" in value:
        fail("environment artifact must not expose public_status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("environment artifact does not bind the qualified Git SHA")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("environment artifact does not preserve the non-mutating worker lifecycle")

    image = value.get("image")
    if not isinstance(image, dict):
        fail("environment artifact image is absent")
    if (
        image.get("base_ref") != base_image_ref
        or image.get("ref") != image_ref
        or image.get("id") != image_id
    ):
        fail("environment artifact image identity differs")

    records = value.get("runtime_records")
    if not isinstance(records, list) or not records:
        fail("environment artifact runtime_records is absent")
    first = records[0]
    if not isinstance(first, dict) or not isinstance(first.get("manifest"), dict):
        fail("environment artifact runtime manifest is absent")
    manifest = first["manifest"]
    if (
        manifest.get("compiled_sm") != "sm_120"
        or manifest.get("gpu_runtime_sm") != "sm_120"
    ):
        fail("environment artifact is not compiled and executed on sm_120")
    if (
        manifest.get("gpu_compute_capability_major"),
        manifest.get("gpu_compute_capability_minor"),
    ) != (12, 0):
        fail("environment artifact compute capability differs from 12.0")
    return manifest


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--replay-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--racecheck-log", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be a canonical lowercase 40-hex commit ID")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id must be a canonical sha256 Docker image ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not the pinned CUDA image")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref is not tied to the qualified SHA")

    _, environment = read_json_object(
        args.environment_artifact, "Phase 3 environment artifact"
    )
    manifest = validate_phase3_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    replay_raw, replay = validate_replay(args.replay_log)
    elf_log = read_text(args.elf_log, "cuobjdump ELF log")
    ptx_log = read_text(args.ptx_log, "cuobjdump PTX log", allow_empty=True)
    ptx_stderr_log = read_text(
        args.ptx_stderr_log,
        "cuobjdump PTX stderr log",
        allow_empty=True,
    )
    memcheck_log = read_text(args.memcheck_log, "memcheck log")
    racecheck_log = read_text(args.racecheck_log, "racecheck log")
    architectures = validate_binary_logs(
        elf_log=elf_log,
        ptx_log=ptx_log,
        memcheck_log=memcheck_log,
        racecheck_log=racecheck_log,
    )

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "full_replay_sha256": sha256_file(
                args.replay, "Phase 5 full-loop K1 Boruvka replay binary"
            ),
        },
        "checks": {
            "aot_elf_architectures": sorted(set(architectures)),
            "aot_ptx_entry_count": 0,
            "canonical_contractions_certified": True,
            "cpu_exact_decision_chain_certified": True,
            "gpu_multi_round_proposal_chain_certified": True,
            "independent_gpu_replay_certified": True,
            "local_emst_witness_certified": True,
            "memcheck": "passed",
            "racecheck": "passed",
            "replay": replay,
        },
        "environment": {
            "compute_capability": "12.0",
            "driver_version": manifest.get("cuda_driver_version_string"),
            "gpu_name": manifest.get("gpu_name"),
            "gpu_uuid": manifest.get("gpu_uuid"),
            "gpu_vram_bytes": manifest.get("gpu_vram_bytes"),
        },
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "logs": {
            "cuobjdump_elf": elf_log,
            "cuobjdump_ptx": ptx_log,
            "cuobjdump_ptx_stderr": ptx_stderr_log,
            "full_replay": replay_raw,
            "memcheck": memcheck_log,
            "racecheck": racecheck_log,
        },
        "mode": "certified",
        "phase": "5",
        "profile": "hgp_reduced",
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": sha256_file(
                args.environment_artifact, "Phase 3 environment artifact"
            ),
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(WORKER_LIFECYCLE),
    }

    output = args.output
    if not output.is_absolute():
        fail("--output must be absolute")
    if output.is_symlink() or not output.exists() or not output.is_file():
        fail("--output must be the regular temporary file reserved by the worker")
    encoded = json.dumps(
        artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    )
    try:
        with output.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
    except OSError as error:
        fail(f"cannot write Phase 5 qualification artifact {output}: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
