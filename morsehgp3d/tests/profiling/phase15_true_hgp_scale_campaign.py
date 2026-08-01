#!/usr/bin/env python3
"""Validate and eventually run the fail-closed Phase 15 true-HGP campaign.

The active v2 plan intentionally has a closed scientific launch gate.  This
module nevertheless freezes the complete binary handshake, run receipts,
50k warm protocol, percentile rules, massive scale order, and durable spool
identity so that no existing component or pair-first binary can impersonate
the future product runner.  V2 also makes exact rational Hartigan levels a
request, capability, closure, and receipt requirement; binary64 levels cannot
qualify a run.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import math
from pathlib import Path
import re
import sys
from typing import Any, NoReturn, Sequence

from campaign_runtime import (
    RuntimeContractError,
    canonical_bytes,
    canonical_json,
    exact_git_sha,
    exact_sha256,
    natural,
    parse_json_text,
    require_regular_file,
    safe_relative_file,
    sha256_bytes,
)


PLAN_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_campaign_plan.v2"
CAPABILITY_SCHEMA = "morsehgp3d.phase15.true_hgp_binary_capabilities.v2"
SESSION_REQUEST_SCHEMA = "morsehgp3d.phase15.true_hgp_session_request.v2"
RUN_REQUEST_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_run_request.v2"
RUN_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_run.v2"
CAMPAIGN_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_campaign_result.v2"
HARTIGAN_LEVEL_RECEIPT_SCHEMA = (
    "morsehgp3d.phase15.exact_hartigan_level_receipt.v1"
)
HARTIGAN_LEVEL_MANIFEST_RECORD_SCHEMA = (
    "morsehgp3d.phase15.exact_hartigan_level_manifest_record.v1"
)
HARTIGAN_LEVEL_REPRESENTATION = "canonical_reduced_rational_decimal_v1"
MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS = 4_096
ALGORITHM_SCOPE = (
    "complete_certified_morse_hgp3d_source_orders_1_through_10_"
    "with_at_least20_condensed_view"
)
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "budgeted_true_source_then_at_least_min_cluster_size20_condensed_view"
MAXIMUM_ORDER = 10
FIFTY_K_POINT_COUNT = 50_000
SLO_P95_NS = 1_000_000_000
DEFAULT_PLAN = Path(__file__).with_name(
    "phase15_true_hgp_scale_campaign_v2.json"
)

FAMILY_IDS = (
    "affine_uniform_binary64",
    "jittered_dyadic_grid3d",
    "balanced_multiscale_clusters",
)
FAMILY_SEEDS = {
    "affine_uniform_binary64": {
        "warmup": [5_001, 5_002],
        "measured": list(range(5_101, 5_111)),
    },
    "jittered_dyadic_grid3d": {
        "warmup": [6_001, 6_002],
        "measured": list(range(6_101, 6_111)),
    },
    "balanced_multiscale_clusters": {
        "warmup": [4_001, 4_002],
        "measured": list(range(4_101, 4_111)),
    },
}
MASSIVE_POINT_COUNTS = (1_000_000, 10_000_001, 30_000_000)
MASSIVE_SCALE_MATRIX = (
    (1_000_000, 50_000, 600_000),
    (10_000_001, 1_000_000, 3_600_000),
    (30_000_000, 10_000_001, 7_200_000),
)

FAMILY_DESCRIPTORS = {
    "affine_uniform_binary64": {
        "generator": "deterministic_affine_uniform_binary64_v1",
        "id": "affine_uniform_binary64",
        "parameters": {
            "affine_condition_bound": 16,
            "coordinate_mantissa_bits": 53,
        },
    },
    "jittered_dyadic_grid3d": {
        "generator": "deterministic_jittered_dyadic_grid3d_v1",
        "id": "jittered_dyadic_grid3d",
        "parameters": {
            "dyadic_denominator_exponent": 30,
            "jitter_numerator_bound": 3,
        },
    },
    "balanced_multiscale_clusters": {
        "generator": "deterministic_balanced_multiscale_clusters_v1",
        "id": "balanced_multiscale_clusters",
        "parameters": {"cluster_count": 64, "scale_level_count": 8},
    },
}

IMPLEMENTATION_CONTRACT = {
    "component_only": False,
    "global_facet_coface_incidence_arena_materialized": False,
    "global_pair_matrix_materialized": False,
    "higher_order_delaunay_mosaic_materialized": False,
    "offline_oracle_in_timed_path": False,
    "ordinary_delaunay_in_timed_path": False,
    "pair_catalog_only": False,
    "point_mst_surrogate": False,
    "streamed_certified_source": True,
}

HARTIGAN_LEVEL_CONTRACT = {
    "binary64_level_serialization_allowed": False,
    "denominator_must_be_positive": True,
    "integer_encoding": "canonical_base10_without_plus_or_leading_zero",
    "manifest_record_schema": HARTIGAN_LEVEL_MANIFEST_RECORD_SCHEMA,
    "maximum_decimal_digits_per_integer": MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS,
    "one_record_per_equal_level_batch": True,
    "ordered_record_chain": "sha256_v1",
    "rational_must_be_reduced": True,
    "representation": HARTIGAN_LEVEL_REPRESENTATION,
}

CANONICAL_SIGNED_INTEGER_RE = re.compile(r"(?:0|-?[1-9][0-9]*)\Z")
CANONICAL_POSITIVE_INTEGER_RE = re.compile(r"[1-9][0-9]*\Z")


class ContractError(ValueError):
    """A true-HGP campaign document escaped its frozen contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(set(value) == expected, f"{label} fields differ")
    return value


def exact_json_equal(observed: Any, expected: Any) -> bool:
    """Compare JSON-domain values without Python's ``False == 0`` coercion."""

    if type(observed) is not type(expected):
        return False
    if isinstance(expected, dict):
        return set(observed) == set(expected) and all(
            exact_json_equal(observed[key], expected[key]) for key in expected
        )
    if isinstance(expected, list):
        return len(observed) == len(expected) and all(
            exact_json_equal(left, right)
            for left, right in zip(observed, expected, strict=True)
        )
    return observed == expected


def expected_plan_document() -> dict[str, Any]:
    families = []
    for family_id in FAMILY_IDS:
        descriptor = copy.deepcopy(FAMILY_DESCRIPTORS[family_id])
        descriptor["measured_seeds"] = list(FAMILY_SEEDS[family_id]["measured"])
        descriptor["warmup_seeds"] = list(FAMILY_SEEDS[family_id]["warmup"])
        families.append(descriptor)
    return {
        "algorithm_scope": ALGORITHM_SCOPE,
        "artifact_role": "future_complete_true_hgp_product_gate",
        "backend": BACKEND,
        "binary_protocol": {
            "capability_argument": "--phase15-true-hgp-capabilities-json",
            "capability_schema": CAPABILITY_SCHEMA,
            "hartigan_level_manifest_record_schema": HARTIGAN_LEVEL_MANIFEST_RECORD_SCHEMA,
            "run_request_schema": RUN_REQUEST_SCHEMA,
            "run_schema": RUN_SCHEMA,
            "session_argument": "--phase15-true-hgp-session-jsonl",
            "session_request_schema": SESSION_REQUEST_SCHEMA,
        },
        "claim_policy": {
            "benchmark_can_promote_public_exact": False,
            "component_measurements_are_results": False,
            "pair_first_measurements_are_results": False,
            "point_mst_surrogate_measurements_are_results": False,
            "requires_complete_binary_capability_handshake": True,
        },
        "entry_gate_satisfied": False,
        "execution_status": "blocked_until_complete_true_hgp_binary_and_entry_gate",
        "families": families,
        "fifty_k_protocol": {
            "fresh_cloud_per_measured_run": True,
            "measured_runs_per_family": 10,
            "nearest_rank_aggregate_p95_rank": 29,
            "nearest_rank_family_p95_rank": 10,
            "point_count": FIFTY_K_POINT_COUNT,
            "recovery_warmups_are_measurements": False,
            "slo_comparison": "strict_less_than",
            "slo_p95_warm_e2e_ns": SLO_P95_NS,
            "slo_role": "secondary_progression_gate",
            "warmups_per_family_session": 2,
            "wall_time_cap_ms_per_run": 30_000,
        },
        "hartigan_level_contract": copy.deepcopy(HARTIGAN_LEVEL_CONTRACT),
        "massive_scales": [
            {
                "family": "affine_uniform_binary64",
                "point_count": point_count,
                "required_previous_point_count": previous,
                "resume_contract": "one_forced_checkpoint_then_fresh_resume_with_uninterrupted_digest_equivalence",
                "seed": 5_101,
                "wall_time_cap_ms": wall_cap,
            }
            for point_count, previous, wall_cap in MASSIVE_SCALE_MATRIX
        ],
        "maximum_order": MAXIMUM_ORDER,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "schema": PLAN_SCHEMA,
        "spool_contract": {
            "active_run_checkpoint_in_head": True,
            "atomic_file_fsync_rename_directory_fsync": True,
            "identity_fields": [
                "git_sha",
                "binary_sha256",
                "plan_sha256",
                "capabilities_sha256",
            ],
            "ordered_receipt_chain": "sha256_v1",
            "posix_single_writer_lock": True,
            "resume_rehashes_binary_and_plan": True,
        },
        "timing_contract": {
            "cloud_generation_timed": False,
            "outer_harness_spool_timed": False,
            "p95_rule": "nearest_rank",
            "warm_e2e_begins_with_raw_coordinates_in_memory": True,
            "warm_e2e_ends_after_source_view_validation_and_scientific_output_seal": True,
        },
        "view": {
            "cardinality_basis": "distinct_PointId_union_of_component_facets",
            "evaluation_boundary": "after_complete_equal_level_batch",
            "min_cluster_size": 20,
            "relation": "at_least",
        },
    }


def validate_plan(value: Any) -> dict[str, Any]:
    expected = expected_plan_document()
    require(isinstance(value, dict), "true-HGP campaign plan must be an object")
    require(
        exact_json_equal(value, expected),
        "true-HGP campaign plan differs from frozen v2",
    )
    return value


def read_plan(path: Path = DEFAULT_PLAN) -> tuple[dict[str, Any], str]:
    try:
        require_regular_file(path, "true-HGP campaign plan")
        payload = path.read_bytes()
        raw = payload.decode("ascii")
        value = parse_json_text(
            raw, "true-HGP campaign plan", canonical_line=False
        )
    except (OSError, UnicodeError, RuntimeContractError) as error:
        fail(str(error))
    return validate_plan(value), sha256_bytes(payload)


def family_descriptor(plan: dict[str, Any], family_id: str) -> dict[str, Any]:
    matches = [family for family in plan["families"] if family["id"] == family_id]
    require(len(matches) == 1, f"true-HGP family {family_id} is not unique")
    family = matches[0]
    return {
        "generator": family["generator"],
        "id": family["id"],
        "parameters": copy.deepcopy(family["parameters"]),
    }


def cloud_request_sha256(
    family: dict[str, Any], point_count: int, seed: int
) -> str:
    return hashlib.sha256(
        canonical_json(
            {"family": family, "point_count": point_count, "seed": seed}
        ).encode("ascii")
    ).hexdigest()


def _run_request(
    *,
    plan: dict[str, Any],
    family_id: str,
    point_count: int,
    seed: int,
    role: str,
    run_id: str,
    run_index: int,
    measured_index: int | None,
    resume_required: bool,
    wall_time_cap_ms: int,
) -> dict[str, Any]:
    family = family_descriptor(plan, family_id)
    return {
        "algorithm_scope": ALGORITHM_SCOPE,
        "cloud_request_sha256": cloud_request_sha256(family, point_count, seed),
        "family": family,
        "fresh_cloud": True,
        "hartigan_level_contract": copy.deepcopy(plan["hartigan_level_contract"]),
        "maximum_order": MAXIMUM_ORDER,
        "measured_index": measured_index,
        "point_count": point_count,
        "resume_required": resume_required,
        "role": role,
        "run_id": run_id,
        "run_index": run_index,
        "schema": RUN_REQUEST_SCHEMA,
        "seed": seed,
        "view": copy.deepcopy(plan["view"]),
        "wall_time_cap_ms": wall_time_cap_ms,
    }


def expected_50k_requests(plan: dict[str, Any]) -> list[dict[str, Any]]:
    validate_plan(plan)
    requests: list[dict[str, Any]] = []
    run_index = 0
    wall_cap = plan["fifty_k_protocol"]["wall_time_cap_ms_per_run"]
    for family in plan["families"]:
        family_id = family["id"]
        for warmup_index, seed in enumerate(family["warmup_seeds"]):
            requests.append(
                _run_request(
                    plan=plan,
                    family_id=family_id,
                    point_count=FIFTY_K_POINT_COUNT,
                    seed=seed,
                    role="warmup",
                    run_id=f"n50000-{family_id}-warmup-{warmup_index}",
                    run_index=run_index,
                    measured_index=None,
                    resume_required=False,
                    wall_time_cap_ms=wall_cap,
                )
            )
            run_index += 1
        for measured_index, seed in enumerate(family["measured_seeds"]):
            requests.append(
                _run_request(
                    plan=plan,
                    family_id=family_id,
                    point_count=FIFTY_K_POINT_COUNT,
                    seed=seed,
                    role="measured",
                    run_id=f"n50000-{family_id}-measured-{measured_index}",
                    run_index=run_index,
                    measured_index=measured_index,
                    resume_required=False,
                    wall_time_cap_ms=wall_cap,
                )
            )
            run_index += 1
    return requests


def recovery_warmup_requests(
    plan: dict[str, Any], family_id: str, session_number: int
) -> list[dict[str, Any]]:
    validate_plan(plan)
    natural(session_number, "recovery session number", positive=True)
    family = next(item for item in plan["families"] if item["id"] == family_id)
    return [
        _run_request(
            plan=plan,
            family_id=family_id,
            point_count=FIFTY_K_POINT_COUNT,
            seed=seed,
            role="recovery_warmup",
            run_id=(
                f"n50000-{family_id}-recovery-{session_number}-warmup-{index}"
            ),
            run_index=index,
            measured_index=None,
            resume_required=False,
            wall_time_cap_ms=plan["fifty_k_protocol"]["wall_time_cap_ms_per_run"],
        )
        for index, seed in enumerate(family["warmup_seeds"])
    ]


def expected_massive_requests(plan: dict[str, Any]) -> list[dict[str, Any]]:
    validate_plan(plan)
    requests: list[dict[str, Any]] = []
    for index, scale in enumerate(plan["massive_scales"]):
        point_count = scale["point_count"]
        requests.append(
            _run_request(
                plan=plan,
                family_id=scale["family"],
                point_count=point_count,
                seed=scale["seed"],
                role="massive",
                run_id=f"n{point_count}-{scale['family']}-resume-qualified",
                run_index=index,
                measured_index=None,
                resume_required=True,
                wall_time_cap_ms=scale["wall_time_cap_ms"],
            )
        )
    return requests


def session_request(
    *,
    plan_sha256: str,
    binary_sha256: str,
    git_sha: str,
    requests: list[dict[str, Any]],
) -> dict[str, Any]:
    require(requests, "true-HGP session cannot be empty")
    identity = {
        "binary_sha256": exact_sha256(binary_sha256, "session binary SHA-256"),
        "git_sha": exact_git_sha(git_sha),
        "plan_sha256": exact_sha256(plan_sha256, "session plan SHA-256"),
    }
    return {
        "identity": identity,
        "request_chain_sha256": sha256_bytes(
            b"".join(canonical_bytes(request) for request in requests)
        ),
        "requests": requests,
        "schema": SESSION_REQUEST_SCHEMA,
    }


CAPABILITY_FLAGS = {
    "at_least20_condensed_view_materialized": True,
    "complete_coface_or_sparse_stream": True,
    "complete_facet_identity_stream": True,
    "complete_true_hgp_source": True,
    "component_only": False,
    "coverage_log_complete": True,
    "durable_resume_with_fresh_recertification": True,
    "exact_rational_hartigan_levels": True,
    "exact_equal_level_batches": True,
    "global_facet_coface_incidence_arena_materialized": False,
    "global_pair_matrix_materialized": False,
    "higher_order_delaunay_mosaic_materialized": False,
    "horizontal_forests_complete": True,
    "hartigan_level_manifest_complete": True,
    "offline_oracle_in_timed_path": False,
    "ordinary_delaunay_in_timed_path": False,
    "pair_catalog_only": False,
    "point_mst_surrogate": False,
    "binary64_hartigan_levels": False,
    "public_exact_claimable_from_benchmark": False,
    "silent_q1_incidences_complete": True,
    "streamed_certified_output": True,
    "vertical_maps_complete": True,
}


def expected_capabilities(
    *,
    plan: dict[str, Any],
    plan_sha256: str,
    binary_sha256: str,
    binary_size_bytes: int,
    git_sha: str,
) -> dict[str, Any]:
    validate_plan(plan)
    return {
        "algorithm_scope": ALGORITHM_SCOPE,
        "backend": BACKEND,
        "binary_sha256": exact_sha256(binary_sha256, "capability binary SHA-256"),
        "binary_size_bytes": natural(
            binary_size_bytes, "capability binary size", positive=True
        ),
        "capabilities": copy.deepcopy(CAPABILITY_FLAGS),
        "git_sha": exact_git_sha(git_sha),
        "hartigan_level_contract": copy.deepcopy(plan["hartigan_level_contract"]),
        "maximum_order": MAXIMUM_ORDER,
        "mode": MODE,
        "plan_sha256": exact_sha256(plan_sha256, "capability plan SHA-256"),
        "profile": PROFILE,
        "run_schema": RUN_SCHEMA,
        "schema": CAPABILITY_SCHEMA,
        "session_request_schema": SESSION_REQUEST_SCHEMA,
        "supported_families": list(FAMILY_IDS),
        "supported_point_counts": [
            FIFTY_K_POINT_COUNT,
            *MASSIVE_POINT_COUNTS,
        ],
    }


def validate_capabilities(
    value: Any,
    *,
    plan: dict[str, Any],
    plan_sha256: str,
    binary_sha256: str,
    binary_size_bytes: int,
    git_sha: str,
) -> dict[str, Any]:
    expected = expected_capabilities(
        plan=plan,
        plan_sha256=plan_sha256,
        binary_sha256=binary_sha256,
        binary_size_bytes=binary_size_bytes,
        git_sha=git_sha,
    )
    require(isinstance(value, dict), "true-HGP binary capabilities must be an object")
    require(
        exact_json_equal(value, expected),
        "binary is not the complete non-surrogate true-HGP implementation",
    )
    return value


RUN_KEYS = {
    "algorithm_scope",
    "artifacts",
    "backend",
    "binary_sha256",
    "closure",
    "cloud",
    "counts_by_order",
    "family",
    "git_sha",
    "hartigan_levels",
    "implementation",
    "maximum_order",
    "mode",
    "plan_sha256",
    "point_count",
    "profile",
    "public_status",
    "qualification_claimed",
    "request_sha256",
    "resources",
    "role",
    "run_id",
    "schema",
    "seed",
    "status",
    "timings_ns",
    "view",
}
CLOSURE_KEYS = {
    "active_cross_incidences_complete",
    "batches_complete_by_order",
    "canonical_children_complete",
    "checkpoint_closed",
    "condensed_view_validated",
    "coverage_complete_by_order",
    "exact_replay_closed",
    "forests_complete_by_order",
    "gamma_complete_by_order",
    "hartigan_levels_complete",
    "numeric_failure_count",
    "output_chain_closed",
    "silent_q1_incidences_complete",
    "source_archive_closed",
    "unsupported_degeneracy_count",
    "unresolved_locus_count",
    "vertical_maps_complete",
}
COUNT_KEYS = {
    "cofaces",
    "coverage_deltas",
    "equal_level_batches",
    "facets",
    "horizontal_nodes_invisible",
    "horizontal_nodes_visible",
    "incidences",
    "order",
    "silent_q1_incidences",
    "vertical_assignments",
}
TIMING_KEYS = {
    "canonicalization",
    "cloud_generation",
    "condensation",
    "horizontal_reduction",
    "output_seal",
    "source_construction",
    "source_recertification",
    "vertical_maps",
    "warm_e2e",
}
TIMED_STAGES = (
    "canonicalization",
    "source_construction",
    "source_recertification",
    "horizontal_reduction",
    "vertical_maps",
    "condensation",
    "output_seal",
)
RESOURCE_KEYS = {
    "checkpoint_bytes",
    "device_capacity_bytes",
    "device_peak_bytes",
    "dominant_phase",
    "external_run_bytes_read",
    "external_run_bytes_written",
    "host_capacity_bytes",
    "host_peak_bytes",
    "output_bytes",
    "scratch_peak_bytes",
}
ARTIFACT_KEYS = {
    "checkpoint_manifest_file",
    "checkpoint_manifest_sha256",
    "output_chain_sha256",
    "source_manifest_file",
    "source_manifest_sha256",
}
HARTIGAN_LEVEL_KEYS = {
    "all_equal_level_batches_covered",
    "binary64_level_count",
    "manifest_file",
    "manifest_record_count",
    "manifest_sha256",
    "ordered_rational_chain_sha256",
    "records_by_order",
    "representation",
    "schema",
}
HARTIGAN_ORDER_KEYS = {
    "equal_level_batch_count",
    "first_level",
    "last_level",
    "order",
    "rational_record_count",
}
RATIONAL_KEYS = {"denominator", "numerator"}


def validate_exact_rational(value: Any, label: str) -> dict[str, Any]:
    rational = exact_keys(value, RATIONAL_KEYS, label)
    numerator = rational["numerator"]
    denominator = rational["denominator"]
    require(
        isinstance(numerator, str)
        and CANONICAL_SIGNED_INTEGER_RE.fullmatch(numerator) is not None,
        f"{label}.numerator is not a canonical decimal integer",
    )
    require(
        isinstance(denominator, str)
        and CANONICAL_POSITIVE_INTEGER_RE.fullmatch(denominator) is not None,
        f"{label}.denominator is not a canonical positive decimal integer",
    )
    require(
        len(numerator.removeprefix("-"))
        <= MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS
        and len(denominator) <= MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS,
        f"{label} exceeds the registered decimal integer bound",
    )
    numerator_integer = int(numerator)
    denominator_integer = int(denominator)
    require(
        math.gcd(abs(numerator_integer), denominator_integer) == 1,
        f"{label} is not a reduced rational",
    )
    return rational


def request_sha256(request: dict[str, Any]) -> str:
    return sha256_bytes(canonical_bytes(request))


def validate_run(
    value: Any,
    request: dict[str, Any],
    *,
    binary_sha256: str | None = None,
    plan_sha256: str | None = None,
    git_sha: str | None = None,
) -> dict[str, Any]:
    run = exact_keys(value, RUN_KEYS, "true-HGP run")
    require(
        exact_json_equal(
            request.get("hartigan_level_contract"), HARTIGAN_LEVEL_CONTRACT
        ),
        "true-HGP request does not require exact rational Hartigan levels",
    )
    expected_scalars = {
        "algorithm_scope": ALGORITHM_SCOPE,
        "backend": BACKEND,
        "family": request["family"],
        "maximum_order": MAXIMUM_ORDER,
        "mode": MODE,
        "point_count": request["point_count"],
        "profile": PROFILE,
        "request_sha256": request_sha256(request),
        "role": request["role"],
        "run_id": request["run_id"],
        "schema": RUN_SCHEMA,
        "seed": request["seed"],
    }
    for key, expected in expected_scalars.items():
        require(
            exact_json_equal(run[key], expected),
            f"true-HGP run {key} differs",
        )
    for key, expected in (
        ("binary_sha256", binary_sha256),
        ("plan_sha256", plan_sha256),
        ("git_sha", git_sha),
    ):
        if expected is not None:
            require(run[key] == expected, f"true-HGP run {key} differs")
    exact_sha256(run["binary_sha256"], "true-HGP run binary SHA-256")
    exact_sha256(run["plan_sha256"], "true-HGP run plan SHA-256")
    exact_git_sha(run["git_sha"], "true-HGP run Git SHA")
    require(run["status"] == "complete", "true-HGP run is not complete")
    require(run["public_status"] == "not_claimed", "benchmark run claimed a public status")
    require(run["qualification_claimed"] is False, "benchmark run claimed qualification")

    cloud = exact_keys(
        run["cloud"],
        {
            "canonical_binary64_sha256",
            "construction_count",
            "fresh_for_run",
            "request_sha256",
        },
        "true-HGP run cloud",
    )
    exact_sha256(cloud["canonical_binary64_sha256"], "canonical cloud SHA-256")
    require(cloud["request_sha256"] == request["cloud_request_sha256"], "cloud request digest differs")
    require(
        cloud["fresh_for_run"] is True
        and natural(cloud["construction_count"], "cloud construction count", positive=True) == 1,
        "true-HGP run did not construct one fresh cloud",
    )
    require(
        exact_json_equal(run["implementation"], IMPLEMENTATION_CONTRACT),
        "run used a forbidden or surrogate implementation",
    )

    closure = exact_keys(run["closure"], CLOSURE_KEYS, "true-HGP run closure")
    for key in (
        "active_cross_incidences_complete",
        "canonical_children_complete",
        "checkpoint_closed",
        "condensed_view_validated",
        "exact_replay_closed",
        "hartigan_levels_complete",
        "output_chain_closed",
        "silent_q1_incidences_complete",
        "source_archive_closed",
        "vertical_maps_complete",
    ):
        require(closure[key] is True, f"true-HGP run closure.{key} is open")
    for key in (
        "batches_complete_by_order",
        "coverage_complete_by_order",
        "forests_complete_by_order",
        "gamma_complete_by_order",
    ):
        require(
            exact_json_equal(closure[key], [True] * MAXIMUM_ORDER),
            f"true-HGP run closure.{key} differs",
        )
    for key in (
        "numeric_failure_count",
        "unsupported_degeneracy_count",
        "unresolved_locus_count",
    ):
        require(natural(closure[key], f"true-HGP run closure.{key}") == 0, f"true-HGP run closure.{key} is nonzero")

    counts = run["counts_by_order"]
    require(isinstance(counts, list) and len(counts) == MAXIMUM_ORDER, "true-HGP per-order counts differ")
    for order, value_by_order in enumerate(counts, start=1):
        record = exact_keys(value_by_order, COUNT_KEYS, f"true-HGP order {order} counts")
        require(
            natural(record["order"], f"true-HGP order {order} index") == order,
            f"true-HGP order {order} index differs",
        )
        for key in COUNT_KEYS - {"order"}:
            natural(record[key], f"true-HGP order {order}.{key}")
        require(
            record["incidences"] >= record["silent_q1_incidences"],
            f"true-HGP order {order} silent incidences exceed incidences",
        )

    hartigan_levels = exact_keys(
        run["hartigan_levels"], HARTIGAN_LEVEL_KEYS, "true-HGP Hartigan levels"
    )
    require(
        hartigan_levels["schema"] == HARTIGAN_LEVEL_RECEIPT_SCHEMA,
        "true-HGP Hartigan level receipt schema differs",
    )
    require(
        hartigan_levels["representation"] == HARTIGAN_LEVEL_REPRESENTATION,
        "true-HGP Hartigan levels are not canonical reduced rationals",
    )
    require(
        hartigan_levels["all_equal_level_batches_covered"] is True,
        "true-HGP Hartigan level manifest is incomplete",
    )
    require(
        natural(
            hartigan_levels["binary64_level_count"],
            "true-HGP binary64 Hartigan level count",
        )
        == 0,
        "true-HGP receipt contains binary64 Hartigan levels",
    )
    try:
        safe_relative_file(
            hartigan_levels["manifest_file"],
            "true-HGP Hartigan level manifest",
        )
    except RuntimeContractError as error:
        fail(str(error))
    exact_sha256(
        hartigan_levels["manifest_sha256"],
        "true-HGP Hartigan level manifest SHA-256",
    )
    exact_sha256(
        hartigan_levels["ordered_rational_chain_sha256"],
        "true-HGP ordered rational Hartigan level chain SHA-256",
    )
    records_by_order = hartigan_levels["records_by_order"]
    require(
        isinstance(records_by_order, list)
        and len(records_by_order) == MAXIMUM_ORDER,
        "true-HGP Hartigan per-order receipts differ",
    )
    manifest_record_count = 0
    for order, value_by_order in enumerate(records_by_order, start=1):
        record = exact_keys(
            value_by_order,
            HARTIGAN_ORDER_KEYS,
            f"true-HGP order {order} Hartigan levels",
        )
        require(
            natural(
                record["order"], f"true-HGP order {order} Hartigan order"
            )
            == order,
            f"true-HGP order {order} Hartigan order differs",
        )
        batch_count = natural(
            record["equal_level_batch_count"],
            f"true-HGP order {order} Hartigan equal-level batch count",
        )
        rational_record_count = natural(
            record["rational_record_count"],
            f"true-HGP order {order} Hartigan rational record count",
        )
        require(
            batch_count == counts[order - 1]["equal_level_batches"]
            and rational_record_count == batch_count,
            f"true-HGP order {order} lacks one exact rational per equal-level batch",
        )
        manifest_record_count += rational_record_count
        if rational_record_count == 0:
            require(
                record["first_level"] is None and record["last_level"] is None,
                f"true-HGP order {order} empty Hartigan boundaries differ",
            )
        else:
            validate_exact_rational(
                record["first_level"],
                f"true-HGP order {order} first Hartigan level",
            )
            validate_exact_rational(
                record["last_level"],
                f"true-HGP order {order} last Hartigan level",
            )
    require(
        natural(
            hartigan_levels["manifest_record_count"],
            "true-HGP Hartigan manifest record count",
        )
        == manifest_record_count,
        "true-HGP Hartigan manifest record count differs",
    )

    require(
        exact_json_equal(run["view"], request["view"]),
        "true-HGP condensed view differs",
    )
    timings = exact_keys(run["timings_ns"], TIMING_KEYS, "true-HGP run timings")
    exact_timings = {
        key: natural(timings[key], f"true-HGP timing {key}", positive=True)
        for key in TIMING_KEYS
    }
    require(
        sum(exact_timings[key] for key in TIMED_STAGES)
        <= exact_timings["warm_e2e"],
        "true-HGP timed stages do not fit inside warm_e2e",
    )
    require(
        exact_timings["warm_e2e"] <= request["wall_time_cap_ms"] * 1_000_000,
        "true-HGP run exceeded its registered wall-time cap",
    )

    resources = exact_keys(run["resources"], RESOURCE_KEYS, "true-HGP run resources")
    require(isinstance(resources["dominant_phase"], str) and resources["dominant_phase"] in TIMED_STAGES, "true-HGP dominant phase is invalid")
    for key in RESOURCE_KEYS - {"dominant_phase"}:
        natural(resources[key], f"true-HGP resource {key}")
    require(
        0 < resources["host_capacity_bytes"]
        and resources["host_peak_bytes"] <= resources["host_capacity_bytes"],
        "true-HGP host memory accounting is invalid",
    )
    require(
        0 < resources["device_capacity_bytes"]
        and resources["device_peak_bytes"] <= resources["device_capacity_bytes"],
        "true-HGP device memory accounting is invalid",
    )

    artifacts = exact_keys(run["artifacts"], ARTIFACT_KEYS, "true-HGP run artifacts")
    for key in ("source_manifest_file", "checkpoint_manifest_file"):
        try:
            safe_relative_file(artifacts[key], f"true-HGP artifact {key}")
        except RuntimeContractError as error:
            fail(str(error))
    for key in (
        "source_manifest_sha256",
        "checkpoint_manifest_sha256",
        "output_chain_sha256",
    ):
        exact_sha256(artifacts[key], f"true-HGP artifact {key}")
    return run


def nearest_rank(samples: list[int], percentile: int) -> int:
    require(samples, "cannot compute a quantile without samples")
    require(0 < percentile <= 100, "quantile is invalid")
    ordered = sorted(samples)
    rank = (len(ordered) * percentile + 99) // 100
    return ordered[rank - 1]


def build_50k_summary(
    runs: list[dict[str, Any]], plan: dict[str, Any]
) -> dict[str, Any]:
    validate_plan(plan)
    measured = [run for run in runs if run.get("role") == "measured"]
    warmups = [run for run in runs if run.get("role") == "warmup"]
    require(len(warmups) == 6, "true-HGP 50k campaign does not contain six primary warmups")
    require(len(measured) == 30, "true-HGP 50k campaign does not contain thirty measurements")
    expected_primary = expected_50k_requests(plan)
    require(
        sorted((run.get("run_id"), run.get("role")) for run in [*warmups, *measured])
        == sorted((request["run_id"], request["role"]) for request in expected_primary),
        "true-HGP 50k campaign primary run identities differ",
    )
    digests: dict[str, str] = {}
    family_summaries: list[dict[str, Any]] = []
    aggregate: list[int] = []
    for family_id in FAMILY_IDS:
        family_runs = [run for run in measured if run["family"]["id"] == family_id]
        require(len(family_runs) == 10, f"true-HGP family {family_id} does not contain ten measurements")
        require(
            sorted(run.get("run_id") for run in family_runs)
            == sorted(
                request["run_id"]
                for request in expected_50k_requests(plan)
                if request["role"] == "measured" and request["family"]["id"] == family_id
            ),
            f"true-HGP family {family_id} measured run identities differ",
        )
        samples: list[int] = []
        for run in family_runs:
            require(run.get("status") == "complete", "incomplete run entered the true-HGP p95")
            digest = run["cloud"]["canonical_binary64_sha256"]
            if digest in digests and digests[digest] != run["run_id"]:
                fail("true-HGP campaign reused one cloud across distinct measured runs")
            digests[digest] = run["run_id"]
            samples.append(run["timings_ns"]["warm_e2e"])
        p95 = nearest_rank(samples, 95)
        aggregate.extend(samples)
        family_summaries.append(
            {
                "family": family_id,
                "measured_run_count": 10,
                "p95_warm_e2e_ns": p95,
                "slo_passed": p95 < SLO_P95_NS,
            }
        )
    aggregate_p95 = nearest_rank(aggregate, 95)
    all_passed = all(item["slo_passed"] for item in family_summaries) and aggregate_p95 < SLO_P95_NS
    return {
        "aggregate": {
            "measured_run_count": 30,
            "p95_warm_e2e_ns": aggregate_p95,
            "slo_passed": aggregate_p95 < SLO_P95_NS,
        },
        "family_summaries": family_summaries,
        "gate_passed": all_passed,
        "p95_rule": "nearest_rank",
        "point_count": FIFTY_K_POINT_COUNT,
        "slo_p95_warm_e2e_ns": SLO_P95_NS,
    }


def validate_scale_progression(completed_point_counts: list[int]) -> int | None:
    expected = [FIFTY_K_POINT_COUNT, *MASSIVE_POINT_COUNTS]
    require(
        completed_point_counts == expected[: len(completed_point_counts)],
        "true-HGP scale progression is not a contiguous registered prefix",
    )
    return expected[len(completed_point_counts)] if len(completed_point_counts) < len(expected) else None


def execute_campaign(
    *,
    plan: dict[str, Any],
    plan_path: Path,
    binary: Path,
    git_sha: str,
    spool_parent: Path,
) -> None:
    del plan_path, binary, git_sha, spool_parent
    validate_plan(plan)
    require(
        plan["entry_gate_satisfied"] is True,
        "true-HGP scientific launch blocked: phase15_true_hgp_50k_entry_gate_satisfied=false",
    )
    fail("true-HGP v2 launch implementation is unavailable")


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("emit-plan")
    run_parser = subparsers.add_parser("run")
    run_parser.add_argument("--binary", type=Path, required=True)
    run_parser.add_argument("--git-sha", required=True)
    run_parser.add_argument("--spool-parent", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    try:
        args = parse_arguments(arguments)
        plan, _ = read_plan(args.plan)
        if args.command == "emit-plan":
            print(canonical_json(plan))
        elif args.command == "run":
            execute_campaign(
                plan=plan,
                plan_path=args.plan,
                binary=args.binary,
                git_sha=args.git_sha,
                spool_parent=args.spool_parent,
            )
        else:
            fail("unknown true-HGP campaign command")
    except (ContractError, RuntimeContractError) as error:
        print(f"Phase 15 true-HGP scale campaign blocked: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
