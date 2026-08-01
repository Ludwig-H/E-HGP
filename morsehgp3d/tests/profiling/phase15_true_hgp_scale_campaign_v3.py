#!/usr/bin/env python3
"""Validate the closed Phase 15 normalized-H0 true-HGP v3 campaign.

V3 is a future contract, not an executable qualification.  It records the
requirements for a regular compressed source made of complete direct events,
complete first-incidence gateways per core facet, and complete carriers.  The
contract deliberately changes the v2 physical batch identity: certified qR=1
continuations with no scientific delta may be represented as normalized no-op
records, while their exact rational Hartigan levels remain in the ordered
chain.  The launch gate stays closed until the named incidence-completeness
proof and the complete product binary exist.
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

PLAN_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_campaign_plan.v3"
CAPABILITY_SCHEMA = "morsehgp3d.phase15.true_hgp_binary_capabilities.v3"
SESSION_REQUEST_SCHEMA = "morsehgp3d.phase15.true_hgp_session_request.v3"
RUN_REQUEST_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_run_request.v3"
RUN_SCHEMA = "morsehgp3d.phase15.true_hgp_scale_run.v3"
HARTIGAN_LEVEL_RECEIPT_SCHEMA = (
    "morsehgp3d.phase15.normalized_exact_hartigan_level_receipt.v3"
)
HARTIGAN_LEVEL_MANIFEST_RECORD_SCHEMA = (
    "morsehgp3d.phase15.normalized_exact_hartigan_level_manifest_record.v3"
)
HARTIGAN_LEVEL_REPRESENTATION = "canonical_reduced_rational_decimal_v1"
MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS = 4_096
ALGORITHM_SCOPE = (
    "incidence_complete_regular_compressed_morse_hgp3d_h0_orders_1_"
    "through_10_with_vertical_maps_and_at_least20_condensed_view"
)
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = (
    "certified_regular_compressed_source_then_normalized_h0_forest_"
    "and_at_least20_view"
)
MAXIMUM_ORDER = 10
FIFTY_K_POINT_COUNT = 50_000
SLO_P95_NS = 1_000_000_000
DEFAULT_PLAN = Path(__file__).with_name("phase15_true_hgp_scale_campaign_v3.json")

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
MASSIVE_POINT_COUNTS = (1_000_000, 10_000_001, 30_000_000)
MASSIVE_SCALE_MATRIX = (
    (1_000_000, 50_000, 600_000),
    (10_000_001, 1_000_000, 3_600_000),
    (30_000_000, 10_000_001, 7_200_000),
)
RESUME_CONTRACT = (
    "one_forced_checkpoint_then_fresh_resume_with_normalized_source_"
    "horizontal_vertical_and_view_digest_equivalence"
)

SOURCE_REDUCTION_CONTRACT = {
    "carrier_payload": "complete_canonical_facets_and_distinct_point_coverage",
    "carriers_complete_by_order": True,
    "complete_exact_co_minimizers_per_core_facet": True,
    "contract_v2_identity_compatible": False,
    "direct_events_complete_by_order": True,
    "facet_gateways_complete_by_order": True,
    "global_facet_coface_incidence_arena_materialized": False,
    "global_gamma_materialized": False,
    "higher_order_delaunay_mosaic_materialized": False,
    "incidence_complete_reduction_proved": True,
    "regularity_certificate_complete": True,
    "source_kind": (
        "regular_compressed_direct_plus_per_facet_gateway_plus_" "complete_carriers_v1"
    ),
    "transient_exhaustive_oracle_in_product_path": False,
}

NORMALIZED_H0_CONTRACT = {
    "contract_v2_identity_compatible": False,
    "exact_level_receipt_required_for_omitted_noop": True,
    "forest_identity": "normalized_horizontal_vertical_h0_forest_v3",
    "normalized_batch_identity": "exact_level_h0_quotient_transition_v3",
    "omitted_noop_rule": (
        "qr1_continuation_zero_core_facet_delta_zero_point_delta_"
        "zero_parent_and_node_delta"
    ),
    "physical_batch_required_for_certified_noop": False,
    "v2_equal_level_batch_identity_reused": False,
    "v2_forest_identity_reused": False,
}

HARTIGAN_LEVEL_CONTRACT = {
    "binary64_level_serialization_allowed": False,
    "denominator_must_be_positive": True,
    "integer_encoding": "canonical_base10_without_plus_or_leading_zero",
    "manifest_record_schema": HARTIGAN_LEVEL_MANIFEST_RECORD_SCHEMA,
    "maximum_decimal_digits_per_integer": (MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS),
    "one_record_per_normalized_equal_level_batch_including_omitted_noop": True,
    "ordered_record_chain": "sha256_v1",
    "rational_must_be_reduced": True,
    "representation": HARTIGAN_LEVEL_REPRESENTATION,
}

CAPABILITY_FLAGS = {
    "at_least20_condensed_view_complete": True,
    "binary64_hartigan_levels": False,
    "complete_carriers_by_order": True,
    "complete_direct_events_by_order": True,
    "complete_facet_gateways_by_order": True,
    "contract_v2_identity_compatible": False,
    "durable_resume_with_fresh_recertification": True,
    "exact_rational_hartigan_levels": True,
    "global_facet_coface_incidence_arena_materialized": False,
    "global_gamma_materialized": False,
    "higher_order_delaunay_mosaic_materialized": False,
    "horizontal_forests_complete": True,
    "incidence_complete_reduction_proved": True,
    "normalized_noop_qr1_supported": True,
    "offline_oracle_in_timed_path": False,
    "point_mst_surrogate": False,
    "public_exact_claimable_from_benchmark": False,
    "regularity_certificates_complete": True,
    "vertical_maps_complete": True,
}

CANONICAL_SIGNED_INTEGER_RE = re.compile(r"(?:0|-?[1-9][0-9]*)\Z")
CANONICAL_POSITIVE_INTEGER_RE = re.compile(r"[1-9][0-9]*\Z")


class ContractError(ValueError):
    """A v3 campaign document escaped its frozen contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def exact_json_equal(observed: Any, expected: Any) -> bool:
    """Compare JSON values without accepting Python's ``False == 0``."""

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


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(set(value) == expected, f"{label} fields differ")
    return value


def expected_plan_document() -> dict[str, Any]:
    families = []
    for family_id in FAMILY_IDS:
        family = copy.deepcopy(FAMILY_DESCRIPTORS[family_id])
        family["measured_seeds"] = list(FAMILY_SEEDS[family_id]["measured"])
        family["warmup_seeds"] = list(FAMILY_SEEDS[family_id]["warmup"])
        families.append(family)
    return {
        "algorithm_scope": ALGORITHM_SCOPE,
        "artifact_role": (
            "future_normalized_h0_incidence_complete_reduction_product_gate"
        ),
        "backend": BACKEND,
        "binary_protocol": {
            "capability_argument": "--phase15-true-hgp-v3-capabilities-json",
            "capability_schema": CAPABILITY_SCHEMA,
            "run_request_schema": RUN_REQUEST_SCHEMA,
            "run_schema": RUN_SCHEMA,
            "session_argument": "--phase15-true-hgp-v3-session-jsonl",
            "session_request_schema": SESSION_REQUEST_SCHEMA,
        },
        "claim_policy": {
            "benchmark_can_establish_incidence_complete_reduction_proof": False,
            "benchmark_can_promote_public_exact": False,
            "component_measurements_are_results": False,
            "pair_first_measurements_are_results": False,
            "plan_requirements_are_current_qualification": False,
            "point_mst_surrogate_measurements_are_results": False,
            "requires_complete_binary_capability_handshake": True,
        },
        "entry_gate_satisfied": False,
        "execution_status": (
            "blocked_until_incidence_complete_reduction_proof_complete_"
            "binary_and_entry_gate"
        ),
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
            "wall_time_cap_ms_per_run": 30_000,
            "warmups_per_family_session": 2,
        },
        "hartigan_level_contract": copy.deepcopy(HARTIGAN_LEVEL_CONTRACT),
        "massive_scales": [
            {
                "family": "affine_uniform_binary64",
                "point_count": point_count,
                "required_previous_point_count": previous,
                "resume_contract": RESUME_CONTRACT,
                "seed": 5_101,
                "wall_time_cap_ms": wall_cap,
            }
            for point_count, previous, wall_cap in MASSIVE_SCALE_MATRIX
        ],
        "maximum_order": MAXIMUM_ORDER,
        "mode": MODE,
        "normalized_h0_contract": copy.deepcopy(NORMALIZED_H0_CONTRACT),
        "phase": "15",
        "profile": PROFILE,
        "schema": PLAN_SCHEMA,
        "source_reduction_contract": copy.deepcopy(SOURCE_REDUCTION_CONTRACT),
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
            "resume_recertifies_normalized_source_forest_vertical_maps_and_view": True,
            "resume_rehashes_binary_and_plan": True,
        },
        "timing_contract": {
            "cloud_generation_timed": False,
            "outer_harness_spool_timed": False,
            "p95_rule": "nearest_rank",
            "warm_e2e_begins_with_raw_coordinates_in_memory": True,
            "warm_e2e_ends_after_normalized_source_horizontal_vertical_and_view_validation_and_output_seal": True,
        },
        "view": {
            "cardinality_basis": "distinct_PointId_union_of_component_facets",
            "evaluation_boundary": ("after_complete_normalized_equal_level_batch"),
            "min_cluster_size": 20,
            "relation": "at_least",
        },
    }


def validate_plan(value: Any) -> dict[str, Any]:
    require(isinstance(value, dict), "true-HGP v3 plan must be an object")
    require(
        exact_json_equal(value, expected_plan_document()),
        "true-HGP campaign plan differs from frozen v3",
    )
    return value


def read_plan(path: Path = DEFAULT_PLAN) -> tuple[dict[str, Any], str]:
    try:
        require_regular_file(path, "true-HGP v3 campaign plan")
        payload = path.read_bytes()
        value = parse_json_text(
            payload.decode("ascii"),
            "true-HGP v3 campaign plan",
            canonical_line=False,
        )
    except (OSError, UnicodeError, RuntimeContractError) as error:
        fail(str(error))
    return validate_plan(value), sha256_bytes(payload)


def family_descriptor(plan: dict[str, Any], family_id: str) -> dict[str, Any]:
    matches = [family for family in plan["families"] if family["id"] == family_id]
    require(len(matches) == 1, f"true-HGP v3 family {family_id} is not unique")
    family = matches[0]
    return {
        "generator": family["generator"],
        "id": family["id"],
        "parameters": copy.deepcopy(family["parameters"]),
    }


def cloud_request_sha256(family: dict[str, Any], point_count: int, seed: int) -> str:
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
        "hartigan_level_contract": copy.deepcopy(HARTIGAN_LEVEL_CONTRACT),
        "maximum_order": MAXIMUM_ORDER,
        "measured_index": measured_index,
        "normalized_h0_contract": copy.deepcopy(NORMALIZED_H0_CONTRACT),
        "point_count": point_count,
        "resume_required": resume_required,
        "role": role,
        "run_id": run_id,
        "run_index": run_index,
        "schema": RUN_REQUEST_SCHEMA,
        "seed": seed,
        "source_reduction_contract": copy.deepcopy(SOURCE_REDUCTION_CONTRACT),
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
                    run_id=f"n50000-{family_id}-v3-warmup-{warmup_index}",
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
                    run_id=f"n50000-{family_id}-v3-measured-{measured_index}",
                    run_index=run_index,
                    measured_index=measured_index,
                    resume_required=False,
                    wall_time_cap_ms=wall_cap,
                )
            )
            run_index += 1
    return requests


def expected_massive_requests(plan: dict[str, Any]) -> list[dict[str, Any]]:
    validate_plan(plan)
    return [
        _run_request(
            plan=plan,
            family_id=scale["family"],
            point_count=scale["point_count"],
            seed=scale["seed"],
            role="massive",
            run_id=(
                f"n{scale['point_count']}-{scale['family']}-" "v3-resume-qualified"
            ),
            run_index=index,
            measured_index=None,
            resume_required=True,
            wall_time_cap_ms=scale["wall_time_cap_ms"],
        )
        for index, scale in enumerate(plan["massive_scales"])
    ]


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
        "binary_sha256": exact_sha256(binary_sha256, "v3 capability binary SHA-256"),
        "binary_size_bytes": natural(
            binary_size_bytes, "v3 capability binary size", positive=True
        ),
        "capabilities": copy.deepcopy(CAPABILITY_FLAGS),
        "git_sha": exact_git_sha(git_sha),
        "hartigan_level_contract": copy.deepcopy(HARTIGAN_LEVEL_CONTRACT),
        "maximum_order": MAXIMUM_ORDER,
        "mode": MODE,
        "normalized_h0_contract": copy.deepcopy(NORMALIZED_H0_CONTRACT),
        "plan_sha256": exact_sha256(plan_sha256, "v3 capability plan SHA-256"),
        "profile": PROFILE,
        "run_schema": RUN_SCHEMA,
        "schema": CAPABILITY_SCHEMA,
        "session_request_schema": SESSION_REQUEST_SCHEMA,
        "source_reduction_contract": copy.deepcopy(SOURCE_REDUCTION_CONTRACT),
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
    require(isinstance(value, dict), "true-HGP v3 capabilities must be an object")
    require(
        exact_json_equal(value, expected),
        "binary does not satisfy the normalized-H0 v3 contract",
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
    "forest",
    "git_sha",
    "hartigan_levels",
    "maximum_order",
    "mode",
    "normalization",
    "plan_sha256",
    "point_count",
    "profile",
    "public_status",
    "qualification_claimed",
    "request_sha256",
    "resources",
    "resume",
    "role",
    "run_id",
    "schema",
    "seed",
    "source",
    "status",
    "timings_ns",
    "view",
}
CLOSURE_KEYS = {
    "at_least20_condensed_view_complete",
    "batches_complete_by_order",
    "carriers_complete_by_order",
    "checkpoint_closed",
    "direct_events_complete_by_order",
    "exact_replay_closed",
    "hartigan_levels_complete",
    "horizontal_forests_complete_by_order",
    "incidence_complete_reduction_proved",
    "facet_gateways_complete_by_order",
    "normalized_noop_qr1_complete_by_order",
    "numeric_failure_count",
    "output_chain_closed",
    "regularity_certificate_complete",
    "source_archive_closed",
    "unsupported_degeneracy_count",
    "unresolved_locus_count",
    "vertical_maps_complete_between_adjacent_orders",
}
COUNT_KEYS = {
    "complete_carriers",
    "direct_events",
    "facet_gateways",
    "horizontal_nodes_invisible",
    "horizontal_nodes_visible",
    "materialized_equal_level_batches",
    "normalized_equal_level_batches",
    "normalized_omitted_noop_qr1_batches",
    "order",
    "vertical_assignments",
}
SOURCE_KEYS = {
    "carrier_chain_sha256",
    "contract",
    "direct_event_chain_sha256",
    "facet_gateway_chain_sha256",
    "incidence_reduction_proof_receipt_sha256",
    "proof_basis",
    "regularity_certificate_sha256",
    "regularity_status",
}
NORMALIZATION_KEYS = {
    "contract",
    "contract_v2_identity_compatible",
    "normalized_batch_chain_closed",
    "normalized_batch_chain_sha256",
    "nonzero_core_facet_delta_noop_count",
    "nonzero_parent_or_node_delta_noop_count",
    "nonzero_point_delta_noop_count",
    "physical_v2_identity_reuse_count",
}
FOREST_KEYS = {
    "at_least20_view_chain_sha256",
    "at_least20_view_complete",
    "horizontal_chain_sha256",
    "horizontal_forests_complete_by_order",
    "vertical_chain_sha256",
    "vertical_maps_complete_between_adjacent_orders",
}
HARTIGAN_KEYS = {
    "all_normalized_equal_level_batches_covered",
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
    "first_level",
    "last_level",
    "normalized_equal_level_batch_count",
    "normalized_omitted_noop_qr1_batch_count",
    "order",
    "rational_record_count",
}
RATIONAL_KEYS = {"denominator", "numerator"}
RESUME_KEYS = {
    "at_least20_view_digest_equivalent",
    "forced_checkpoint_count",
    "fresh_process_resume_count",
    "horizontal_forest_digest_equivalent",
    "normalized_source_digest_equivalent",
    "required",
    "status",
    "vertical_map_digest_equivalent",
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
    "host_capacity_bytes",
    "host_peak_bytes",
    "output_bytes",
    "scratch_peak_bytes",
}
ARTIFACT_KEYS = {
    "checkpoint_manifest_file",
    "checkpoint_manifest_sha256",
    "normalized_source_manifest_file",
    "normalized_source_manifest_sha256",
    "output_chain_sha256",
}


def request_sha256(request: dict[str, Any]) -> str:
    return sha256_bytes(canonical_bytes(request))


def validate_exact_rational(value: Any, label: str) -> tuple[int, int]:
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
        len(numerator.removeprefix("-")) <= MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS
        and len(denominator) <= MAXIMUM_RATIONAL_INTEGER_DECIMAL_DIGITS,
        f"{label} exceeds the decimal integer bound",
    )
    numerator_integer = int(numerator)
    denominator_integer = int(denominator)
    require(
        math.gcd(abs(numerator_integer), denominator_integer) == 1,
        f"{label} is not a reduced rational",
    )
    require(numerator_integer >= 0, f"{label} is a negative Hartigan level")
    return numerator_integer, denominator_integer


def validate_run(
    value: Any,
    request: dict[str, Any],
    *,
    binary_sha256: str | None = None,
    plan_sha256: str | None = None,
    git_sha: str | None = None,
) -> dict[str, Any]:
    run = exact_keys(value, RUN_KEYS, "true-HGP v3 run")
    for key, contract in (
        ("source_reduction_contract", SOURCE_REDUCTION_CONTRACT),
        ("normalized_h0_contract", NORMALIZED_H0_CONTRACT),
        ("hartigan_level_contract", HARTIGAN_LEVEL_CONTRACT),
    ):
        require(
            exact_json_equal(request.get(key), contract),
            f"true-HGP v3 request {key} differs",
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
        require(exact_json_equal(run[key], expected), f"true-HGP v3 run {key} differs")
    for key, expected in (
        ("binary_sha256", binary_sha256),
        ("plan_sha256", plan_sha256),
        ("git_sha", git_sha),
    ):
        if expected is not None:
            require(run[key] == expected, f"true-HGP v3 run {key} differs")
    exact_sha256(run["binary_sha256"], "true-HGP v3 binary SHA-256")
    exact_sha256(run["plan_sha256"], "true-HGP v3 plan SHA-256")
    exact_git_sha(run["git_sha"])
    require(run["status"] == "complete", "true-HGP v3 run is incomplete")
    require(run["public_status"] == "not_claimed", "benchmark claimed public exactness")
    require(run["qualification_claimed"] is False, "benchmark claimed qualification")

    cloud = exact_keys(
        run["cloud"],
        {
            "canonical_binary64_sha256",
            "construction_count",
            "fresh_for_run",
            "request_sha256",
        },
        "true-HGP v3 cloud",
    )
    exact_sha256(cloud["canonical_binary64_sha256"], "v3 cloud SHA-256")
    require(
        cloud["request_sha256"] == request["cloud_request_sha256"],
        "cloud digest differs",
    )
    require(
        cloud["fresh_for_run"] is True
        and natural(
            cloud["construction_count"], "cloud construction count", positive=True
        )
        == 1,
        "true-HGP v3 run did not use one fresh cloud",
    )

    closure = exact_keys(run["closure"], CLOSURE_KEYS, "true-HGP v3 closure")
    for key in (
        "at_least20_condensed_view_complete",
        "checkpoint_closed",
        "exact_replay_closed",
        "hartigan_levels_complete",
        "incidence_complete_reduction_proved",
        "output_chain_closed",
        "regularity_certificate_complete",
        "source_archive_closed",
    ):
        require(closure[key] is True, f"true-HGP v3 closure.{key} is open")
    for key in (
        "batches_complete_by_order",
        "carriers_complete_by_order",
        "direct_events_complete_by_order",
        "facet_gateways_complete_by_order",
        "horizontal_forests_complete_by_order",
        "normalized_noop_qr1_complete_by_order",
    ):
        require(
            exact_json_equal(closure[key], [True] * MAXIMUM_ORDER),
            f"true-HGP v3 closure.{key} differs",
        )
    require(
        exact_json_equal(
            closure["vertical_maps_complete_between_adjacent_orders"],
            [True] * (MAXIMUM_ORDER - 1),
        ),
        "true-HGP v3 vertical closure differs",
    )
    for key in (
        "numeric_failure_count",
        "unsupported_degeneracy_count",
        "unresolved_locus_count",
    ):
        require(
            natural(closure[key], f"true-HGP v3 closure.{key}") == 0,
            f"true-HGP v3 closure.{key} is nonzero",
        )

    source = exact_keys(run["source"], SOURCE_KEYS, "true-HGP v3 source")
    require(
        exact_json_equal(source["contract"], SOURCE_REDUCTION_CONTRACT),
        "true-HGP v3 source contract differs",
    )
    require(
        source["proof_basis"] == "incidence_complete_reduction_proved"
        and source["regularity_status"] == "certified_regular",
        "true-HGP v3 source lacks its proof or regularity basis",
    )
    for key in SOURCE_KEYS - {"contract", "proof_basis", "regularity_status"}:
        exact_sha256(source[key], f"true-HGP v3 source {key}")

    normalization = exact_keys(
        run["normalization"], NORMALIZATION_KEYS, "true-HGP v3 normalization"
    )
    require(
        exact_json_equal(normalization["contract"], NORMALIZED_H0_CONTRACT),
        "true-HGP v3 normalization contract differs",
    )
    require(
        normalization["contract_v2_identity_compatible"] is False,
        "true-HGP v3 run reused v2 identity",
    )
    require(
        normalization["normalized_batch_chain_closed"] is True,
        "true-HGP v3 normalized chain is open",
    )
    exact_sha256(
        normalization["normalized_batch_chain_sha256"],
        "true-HGP v3 normalized batch chain SHA-256",
    )
    for key in (
        "nonzero_core_facet_delta_noop_count",
        "nonzero_parent_or_node_delta_noop_count",
        "nonzero_point_delta_noop_count",
        "physical_v2_identity_reuse_count",
    ):
        require(
            natural(normalization[key], f"true-HGP v3 normalization.{key}") == 0,
            f"true-HGP v3 normalization.{key} is nonzero",
        )

    counts = run["counts_by_order"]
    require(
        isinstance(counts, list) and len(counts) == MAXIMUM_ORDER,
        "true-HGP v3 per-order counts differ",
    )
    normalized_batch_total = 0
    normalized_noop_total = 0
    for order, value_by_order in enumerate(counts, start=1):
        record = exact_keys(
            value_by_order, COUNT_KEYS, f"true-HGP v3 order {order} counts"
        )
        require(
            natural(record["order"], f"true-HGP v3 order {order}") == order,
            f"true-HGP v3 order {order} differs",
        )
        for key in COUNT_KEYS - {"order"}:
            natural(record[key], f"true-HGP v3 order {order}.{key}")
        require(
            record["normalized_equal_level_batches"]
            == record["materialized_equal_level_batches"]
            + record["normalized_omitted_noop_qr1_batches"],
            f"true-HGP v3 order {order} batch normalization does not close",
        )
        normalized_batch_total += record["normalized_equal_level_batches"]
        normalized_noop_total += record["normalized_omitted_noop_qr1_batches"]

    hartigan = exact_keys(
        run["hartigan_levels"], HARTIGAN_KEYS, "true-HGP v3 Hartigan levels"
    )
    require(
        hartigan["schema"] == HARTIGAN_LEVEL_RECEIPT_SCHEMA
        and hartigan["representation"] == HARTIGAN_LEVEL_REPRESENTATION,
        "true-HGP v3 Hartigan representation differs",
    )
    require(
        hartigan["all_normalized_equal_level_batches_covered"] is True,
        "true-HGP v3 Hartigan manifest is incomplete",
    )
    require(
        natural(hartigan["binary64_level_count"], "v3 binary64 level count") == 0,
        "true-HGP v3 Hartigan receipt contains binary64 levels",
    )
    try:
        safe_relative_file(hartigan["manifest_file"], "v3 Hartigan manifest")
    except RuntimeContractError as error:
        fail(str(error))
    for key in ("manifest_sha256", "ordered_rational_chain_sha256"):
        exact_sha256(hartigan[key], f"true-HGP v3 Hartigan {key}")
    records_by_order = hartigan["records_by_order"]
    require(
        isinstance(records_by_order, list) and len(records_by_order) == MAXIMUM_ORDER,
        "true-HGP v3 Hartigan order receipts differ",
    )
    manifest_record_count = 0
    for order, value_by_order in enumerate(records_by_order, start=1):
        record = exact_keys(
            value_by_order,
            HARTIGAN_ORDER_KEYS,
            f"true-HGP v3 order {order} Hartigan receipt",
        )
        require(
            natural(record["order"], f"v3 Hartigan order {order}") == order,
            f"true-HGP v3 Hartigan order {order} differs",
        )
        batch_count = natural(
            record["normalized_equal_level_batch_count"],
            f"v3 Hartigan order {order} batch count",
        )
        noop_count = natural(
            record["normalized_omitted_noop_qr1_batch_count"],
            f"v3 Hartigan order {order} no-op count",
        )
        rational_count = natural(
            record["rational_record_count"],
            f"v3 Hartigan order {order} rational count",
        )
        expected_counts = counts[order - 1]
        require(
            batch_count == expected_counts["normalized_equal_level_batches"]
            and noop_count == expected_counts["normalized_omitted_noop_qr1_batches"]
            and rational_count == batch_count,
            f"true-HGP v3 order {order} lacks exact levels for normalized batches",
        )
        manifest_record_count += rational_count
        if rational_count == 0:
            require(
                record["first_level"] is None and record["last_level"] is None,
                f"true-HGP v3 order {order} empty boundaries differ",
            )
        else:
            first_numerator, first_denominator = validate_exact_rational(
                record["first_level"], f"v3 order {order} first level"
            )
            last_numerator, last_denominator = validate_exact_rational(
                record["last_level"], f"v3 order {order} last level"
            )
            require(
                first_numerator * last_denominator
                <= last_numerator * first_denominator,
                f"true-HGP v3 order {order} Hartigan boundaries decrease",
            )
    require(
        natural(hartigan["manifest_record_count"], "v3 manifest record count")
        == manifest_record_count
        == normalized_batch_total,
        "true-HGP v3 Hartigan manifest count differs",
    )
    require(
        sum(
            record["normalized_omitted_noop_qr1_batch_count"]
            for record in records_by_order
        )
        == normalized_noop_total,
        "true-HGP v3 Hartigan no-op coverage differs",
    )

    forest = exact_keys(run["forest"], FOREST_KEYS, "true-HGP v3 forest")
    require(
        exact_json_equal(
            forest["horizontal_forests_complete_by_order"],
            [True] * MAXIMUM_ORDER,
        )
        and exact_json_equal(
            forest["vertical_maps_complete_between_adjacent_orders"],
            [True] * (MAXIMUM_ORDER - 1),
        )
        and forest["at_least20_view_complete"] is True,
        "true-HGP v3 forest or vertical maps are incomplete",
    )
    for key in (
        "at_least20_view_chain_sha256",
        "horizontal_chain_sha256",
        "vertical_chain_sha256",
    ):
        exact_sha256(forest[key], f"true-HGP v3 forest {key}")
    require(
        exact_json_equal(run["view"], request["view"]),
        "true-HGP v3 at_least20 view differs",
    )

    resume = exact_keys(run["resume"], RESUME_KEYS, "true-HGP v3 resume")
    resume_required = request["resume_required"]
    require(
        type(resume["required"]) is bool and resume["required"] is resume_required,
        "true-HGP v3 resume requirement differs",
    )
    equivalence_keys = (
        "at_least20_view_digest_equivalent",
        "horizontal_forest_digest_equivalent",
        "normalized_source_digest_equivalent",
        "vertical_map_digest_equivalent",
    )
    if resume_required:
        require(resume["status"] == "complete", "required v3 resume is incomplete")
        require(
            natural(resume["forced_checkpoint_count"], "v3 forced checkpoints") == 1
            and natural(resume["fresh_process_resume_count"], "v3 fresh resumes") == 1
            and all(resume[key] is True for key in equivalence_keys),
            "required v3 resume did not prove full digest equivalence",
        )
    else:
        require(resume["status"] == "not_required", "unexpected v3 resume status")
        require(
            natural(resume["forced_checkpoint_count"], "v3 forced checkpoints") == 0
            and natural(resume["fresh_process_resume_count"], "v3 fresh resumes") == 0
            and all(resume[key] is False for key in equivalence_keys),
            "non-required v3 run forged resume evidence",
        )

    timings = exact_keys(run["timings_ns"], TIMING_KEYS, "true-HGP v3 timings")
    exact_timings = {
        key: natural(timings[key], f"true-HGP v3 timing {key}", positive=True)
        for key in TIMING_KEYS
    }
    require(
        sum(exact_timings[key] for key in TIMED_STAGES)
        <= exact_timings["warm_e2e"]
        <= request["wall_time_cap_ms"] * 1_000_000,
        "true-HGP v3 timing envelope differs",
    )

    resources = exact_keys(run["resources"], RESOURCE_KEYS, "true-HGP v3 resources")
    for key in RESOURCE_KEYS:
        natural(resources[key], f"true-HGP v3 resource {key}")
    require(
        0 < resources["host_capacity_bytes"]
        and resources["host_peak_bytes"] <= resources["host_capacity_bytes"]
        and 0 < resources["device_capacity_bytes"]
        and resources["device_peak_bytes"] <= resources["device_capacity_bytes"],
        "true-HGP v3 memory accounting is invalid",
    )

    artifacts = exact_keys(run["artifacts"], ARTIFACT_KEYS, "true-HGP v3 artifacts")
    for key in ("checkpoint_manifest_file", "normalized_source_manifest_file"):
        try:
            safe_relative_file(artifacts[key], f"true-HGP v3 artifact {key}")
        except RuntimeContractError as error:
            fail(str(error))
    for key in (
        "checkpoint_manifest_sha256",
        "normalized_source_manifest_sha256",
        "output_chain_sha256",
    ):
        exact_sha256(artifacts[key], f"true-HGP v3 artifact {key}")
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
    require(len(measured) == 30, "v3 50k campaign lacks thirty measurements")
    expected_ids = {
        request["run_id"]
        for request in expected_50k_requests(plan)
        if request["role"] == "measured"
    }
    require(
        {run.get("run_id") for run in measured} == expected_ids,
        "v3 50k measured run identities differ",
    )
    cloud_digests: set[str] = set()
    aggregate: list[int] = []
    family_summaries = []
    for family_id in FAMILY_IDS:
        family_runs = [run for run in measured if run["family"]["id"] == family_id]
        require(len(family_runs) == 10, f"v3 family {family_id} lacks ten runs")
        samples = []
        for run in family_runs:
            require(run.get("status") == "complete", "incomplete run entered v3 p95")
            digest = run["cloud"]["canonical_binary64_sha256"]
            require(digest not in cloud_digests, "v3 measured cloud was reused")
            cloud_digests.add(digest)
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
    return {
        "aggregate": {
            "measured_run_count": 30,
            "p95_warm_e2e_ns": aggregate_p95,
            "slo_passed": aggregate_p95 < SLO_P95_NS,
        },
        "family_summaries": family_summaries,
        "gate_passed": (
            aggregate_p95 < SLO_P95_NS
            and all(item["slo_passed"] for item in family_summaries)
        ),
        "p95_rule": "nearest_rank",
        "point_count": FIFTY_K_POINT_COUNT,
        "slo_p95_warm_e2e_ns": SLO_P95_NS,
    }


def validate_scale_progression(completed_point_counts: list[int]) -> int | None:
    expected = [FIFTY_K_POINT_COUNT, *MASSIVE_POINT_COUNTS]
    require(
        completed_point_counts == expected[: len(completed_point_counts)],
        "true-HGP v3 scale progression is not a contiguous registered prefix",
    )
    if len(completed_point_counts) == len(expected):
        return None
    return expected[len(completed_point_counts)]


def execute_campaign(plan: dict[str, Any]) -> None:
    validate_plan(plan)
    require(
        plan["entry_gate_satisfied"] is True,
        "true-HGP v3 launch blocked: phase15_true_hgp_v3_entry_gate_satisfied=false",
    )
    fail("true-HGP v3 launch implementation is unavailable")


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("emit-plan")
    subparsers.add_parser("run")
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    try:
        args = parse_arguments(arguments)
        plan, _ = read_plan(args.plan)
        if args.command == "emit-plan":
            print(canonical_json(plan))
        elif args.command == "run":
            execute_campaign(plan)
        else:
            fail("unknown true-HGP v3 campaign command")
    except (ContractError, RuntimeContractError) as error:
        print(f"Phase 15 true-HGP v3 campaign blocked: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
