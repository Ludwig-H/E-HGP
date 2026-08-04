#!/usr/bin/env python3
"""Assemble guarded schema-6 native terminal-geometry CUDA evidence."""

from __future__ import annotations

import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
from typing import Any, NoReturn, Sequence

from assemble_phase7_h_polytope_qualification import (
    BASE_IMAGE_REF,
    IMAGE_ID_RE,
    PHASE3_SCHEMA,
    SHA_RE,
    WORKER_LIFECYCLE,
    parse_json_object,
    read_text_evidence,
    require_exact_keys,
    sha256_file,
    validate_elf_log,
    validate_environment,
    validate_memcheck_log,
    write_exclusive_atomic,
)

SCHEMA = "morsehgp3d.phase15." "exact_higher_support_terminal_geometry_cuda_g4.v6"
ARTIFACT_ROLE = "bounded_native_terminal_geometry_component_qualification"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = (
    "device_resident_batched_morton_node_products_"
    "bounded_local_query_plans_and_terminal_support_geometry"
)
SCIENTIFIC_SCOPE = (
    "bounded_exact_terminal_geometry_categories_supports_3_4_component_only"
)
DEPLOYMENT_STATUS = "schema5_native_terminal_geometry_schema6_component_only"
POINT_COUNT = 28
TASK_COUNT = 21
HISTORICAL_TASK_COUNT = 12
TERMINAL_TASK_COUNT = 9
TERMINAL_SUPPORT_3_TASK_COUNT = 4
TERMINAL_SUPPORT_4_TASK_COUNT = 5
COMPONENT_SCHEMA_VERSION = 5
QUALIFICATION_SCHEMA_VERSION = 6
EXPECTED_KERNEL_ENTRY_COUNT = 3
EXPECTED_RATIONAL_FALLBACK_COUNT = 2
ALLOWED_GCP_PROJECT = "devpod-gpu-exploration"
ALLOWED_GCP_TARGETS = {
    ("europe-west4-a", "ehgp-blackwell-spot"),
    ("europe-west4-ai1a", "ehgp-blackwell-spot-ai1a"),
}
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_exact_higher_support_product_cuda_qualification"
)
BINARY_CONTAINER_PATH = f"/workspace/repository/{BINARY_RELATIVE_PATH}"
AUDIT_BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-audit/"
    "morsehgp3d_gpu_exact_higher_support_product_cuda_qualification"
)
QUALIFICATION_COMMAND = [BINARY_CONTAINER_PATH]
QUALIFICATION_KEYS = {
    "backend",
    "bounded_int256_count",
    "bounded_int512_count",
    "bounded_int1024_count",
    "certified_count",
    "completed_result_digest",
    "component_batch_repeatable",
    "component_schema_version",
    "cuda_device",
    "cuda_execution_performed",
    "cuda_stream_total_ns",
    "cpu_stream_total_ns",
    "every_result_validated_once",
    "every_task_validated_before_launch",
    "fail_open_count",
    "fixture_family_count",
    "floating_point_decision_performed",
    "global_cell_coface_or_incidence_arena_materialized",
    "global_product_frontier_mutated",
    "host_fake_lifecycle_exercised",
    "host_batch_total_ns",
    "internal_node_receipt_leaf_count",
    "internal_node_receipt_task_count",
    "kernel_launch_count",
    "kernel_elapsed_ns",
    "kernel_elapsed_ns_available",
    "kernel_ns",
    "kernel_ns_available",
    "lbvh_build_total_ns",
    "mismatch_count",
    "mode",
    "native_lbvh_nodes_read_on_device",
    "narrow_int256_kernel_executed",
    "narrow_int512_kernel_executed",
    "ordinary_or_higher_order_delaunay_materialized",
    "overlap_rejected_without_launch",
    "output_published_atomically",
    "point_count",
    "profile",
    "qualified",
    "rational_fallback_count",
    "resident_lease_index_identity_validated",
    "schema_version",
    "source_authority_validated",
    "source_snapshot_epoch",
    "stream_adapter_certified_positive_count",
    "stream_adapter_component_cpu_fallback_count",
    "stream_adapter_cache_capacity",
    "stream_adapter_cache_hit_count",
    "stream_adapter_cache_miss_count",
    "stream_adapter_evaluate_call_count",
    "stream_adapter_maximum_batch_size",
    "stream_adapter_maximum_cache_entry_count",
    "stream_adapter_native_exact_authority",
    "stream_adapter_native_kernel_certified_positive_count",
    "stream_adapter_terminal_geometry_decision_count",
    "stream_adapter_terminal_geometry_native_kernel_decision_count",
    "stream_adapter_terminal_geometry_native_cuda",
    "stream_adapter_native_cpu_fallback_certified_positive_count",
    "stream_adapter_qualified",
    "stream_adapter_prefetch_call_count",
    "stream_adapter_prefetched_task_count",
    "stream_adapter_submitted_task_count",
    "stream_adapter_synchronization_count",
    "stream_adapter_support_3_certified_positive_count",
    "stream_adapter_support_4_certified_positive_count",
    "stream_adapter_support_prune_certified_positive_count",
    "stream_adapter_query_certified_positive_count",
    "submitted_task_digest",
    "terminal_affinely_dependent_count",
    "terminal_boundary_reduced_count",
    "terminal_classification_native_cuda",
    "terminal_device_fallback_without_category_count",
    "terminal_exterior_circumcenter_count",
    "terminal_geometry_host_fallback_decision_count",
    "terminal_geometry_native_cuda",
    "terminal_geometry_native_kernel_decision_count",
    "terminal_int256_to_host_int512_fallback_count",
    "terminal_int512_to_host_int1024_fallback_count",
    "terminal_int1024_to_cpu_rational_fallback_count",
    "terminal_minimal_count",
    "terminal_support_3_affinely_dependent_count",
    "terminal_support_3_boundary_reduced_count",
    "terminal_support_3_exterior_circumcenter_count",
    "terminal_support_3_minimal_count",
    "terminal_support_3_task_count",
    "terminal_support_4_affinely_dependent_count",
    "terminal_support_4_boundary_reduced_count",
    "terminal_support_4_exterior_circumcenter_count",
    "terminal_support_4_minimal_count",
    "terminal_support_4_task_count",
    "terminal_task_count",
    "synchronization_count",
    "task_count",
    "total_qualification_ns",
}
TIMING_KEYS = {
    "cpu_stream_total_ns",
    "cuda_stream_total_ns",
    "host_batch_total_ns",
    "kernel_elapsed_ns",
    "kernel_ns",
    "lbvh_build_total_ns",
    "total_qualification_ns",
}
LOG_KEYS = {
    "audit_build",
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "memcheck",
    "qualification",
    "qualification_stderr",
    "release_build",
}
CLAIMS = {
    "catalog_complete": False,
    "full_gamma2": False,
    "global_cell_coface_or_incidence_arena_materialized": False,
    "hierarchy_or_tree": False,
    "higher_order_delaunay_materialized": False,
    "industrial_scale": False,
    "public_status_claimed": False,
    "slo_claimed": False,
    "terminal_classification_native_cuda": False,
    "terminal_geometry_native_cuda": True,
}
PHASE_CONTEXT = {
    "backend": "cuda_g4_native_terminal_geometry_plus_exact_host_fallback",
    "deployment_status": "schema6_guarded_component_qualification_only",
    "mode": "bounded_terminal_geometry_supports_3_4",
    "profile": PROFILE,
    "public_status": "not_claimed",
}
ARTIFACT_KEYS = {
    "artifact_role",
    "backend",
    "binary",
    "checks",
    "claims",
    "commands",
    "component_only",
    "deployment_status",
    "git",
    "image",
    "log_sha256",
    "logs",
    "mode",
    "phase",
    "phase_context",
    "profile",
    "provenance",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "vm_lifecycle",
}
BUILD_FAILURE_RE = re.compile(
    r"(?:^|\n)(?:FAILED:|ninja: build stopped:|CMake Error(?: at|:)|"
    r"[^\n]*(?:fatal error:|undefined reference to|collect2: error:|"
    r"ld(?:\.lld)?: error:))",
    re.IGNORECASE,
)
PTXAS_ENTRY_RE = re.compile(
    r"ptxas info\s*:\s*Compiling entry function '([^']+)' for '(sm_[0-9]+)'"
)
PTXAS_STACK_RE_TEMPLATE = (
    r"ptxas info\s*:\s*Function properties for {entry}\s*\n"
    r"\s*([0-9]+) bytes stack frame, ([0-9]+) bytes spill stores, "
    r"([0-9]+) bytes spill loads"
)
PTXAS_REGISTERS_RE = re.compile(r"ptxas info\s*:\s*Used ([0-9]+) registers\b")
KERNEL_NAME = "morsehgp3d_phase15_exact_higher_support_product_kernel"


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def canonical_json(value: dict[str, Any]) -> str:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    )


def require_int(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{label} must be an integer >= {minimum}")
    return value


def require_uint64(value: Any, label: str, *, nonzero: bool = False) -> int:
    result = require_int(value, label, minimum=1 if nonzero else 0)
    if result > (1 << 64) - 1:
        fail(f"{label} must fit in uint64")
    return result


def require_bool(value: Any, label: str, expected: bool) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def validate_qualification(value: dict[str, Any]) -> dict[str, Any]:
    """Validate the closed schema-6 summary emitted by the G4 binary."""

    require_exact_keys(value, QUALIFICATION_KEYS, "terminal-geometry qualification")
    for field, expected in {
        "backend": BACKEND,
        "component_schema_version": COMPONENT_SCHEMA_VERSION,
        "fixture_family_count": 6,
        "mode": MODE,
        "point_count": POINT_COUNT,
        "profile": PROFILE,
        "schema_version": QUALIFICATION_SCHEMA_VERSION,
        "task_count": TASK_COUNT,
        "terminal_support_3_task_count": TERMINAL_SUPPORT_3_TASK_COUNT,
        "terminal_support_4_task_count": TERMINAL_SUPPORT_4_TASK_COUNT,
        "terminal_task_count": TERMINAL_TASK_COUNT,
    }.items():
        if value.get(field) != expected:
            fail(f"qualification {field} must be {expected}")

    true_fields = {
        "component_batch_repeatable",
        "cuda_execution_performed",
        "every_result_validated_once",
        "every_task_validated_before_launch",
        "kernel_elapsed_ns_available",
        "kernel_ns_available",
        "native_lbvh_nodes_read_on_device",
        "narrow_int256_kernel_executed",
        "narrow_int512_kernel_executed",
        "overlap_rejected_without_launch",
        "output_published_atomically",
        "qualified",
        "resident_lease_index_identity_validated",
        "source_authority_validated",
        "stream_adapter_native_exact_authority",
        "stream_adapter_qualified",
        "stream_adapter_terminal_geometry_native_cuda",
        "terminal_geometry_native_cuda",
    }
    false_fields = {
        "floating_point_decision_performed",
        "global_cell_coface_or_incidence_arena_materialized",
        "global_product_frontier_mutated",
        "host_fake_lifecycle_exercised",
        "ordinary_or_higher_order_delaunay_materialized",
        "terminal_classification_native_cuda",
    }
    for field in true_fields:
        require_bool(value.get(field), field, True)
    for field in false_fields:
        require_bool(value.get(field), field, False)

    if require_int(value.get("mismatch_count"), "mismatch_count") != 0:
        fail("the CUDA result differs from the exact CPU authority")
    certified = require_int(value.get("certified_count"), "certified_count", minimum=1)
    fail_open = require_int(value.get("fail_open_count"), "fail_open_count")
    if certified + fail_open != TASK_COUNT or certified < TERMINAL_TASK_COUNT:
        fail("certified and fail-open records do not partition all 21 tasks")

    bounded_counts = [
        require_int(value.get(name), name, minimum=1)
        for name in (
            "bounded_int256_count",
            "bounded_int512_count",
            "bounded_int1024_count",
        )
    ]
    rational = require_int(
        value.get("rational_fallback_count"), "rational_fallback_count"
    )
    if rational != EXPECTED_RATIONAL_FALLBACK_COUNT:
        fail("the 12+9 fixture must exercise exactly two rational fallbacks")
    if sum(bounded_counts) + rational != TASK_COUNT:
        fail("bounded widths and rational fallback do not partition all tasks")

    categories = [
        require_int(value.get(name), name, minimum=1)
        for name in (
            "terminal_affinely_dependent_count",
            "terminal_boundary_reduced_count",
            "terminal_exterior_circumcenter_count",
            "terminal_minimal_count",
        )
    ]
    if sum(categories) != TERMINAL_TASK_COUNT:
        fail("the four terminal categories do not partition the nine terminal tasks")
    support_3_categories = [
        require_int(value.get(name), name, minimum=1)
        for name in (
            "terminal_support_3_affinely_dependent_count",
            "terminal_support_3_boundary_reduced_count",
            "terminal_support_3_exterior_circumcenter_count",
            "terminal_support_3_minimal_count",
        )
    ]
    support_4_categories = [
        require_int(value.get(name), name, minimum=1)
        for name in (
            "terminal_support_4_affinely_dependent_count",
            "terminal_support_4_boundary_reduced_count",
            "terminal_support_4_exterior_circumcenter_count",
            "terminal_support_4_minimal_count",
        )
    ]
    if sum(support_3_categories) != TERMINAL_SUPPORT_3_TASK_COUNT:
        fail("the support-3 row does not partition its four terminal tasks")
    if sum(support_4_categories) != TERMINAL_SUPPORT_4_TASK_COUNT:
        fail("the support-4 row does not partition its five terminal tasks")
    if [
        support_3_categories[index] + support_4_categories[index] for index in range(4)
    ] != categories:
        fail("the 2x4 support/category matrix differs from its column totals")
    for field, expected in {
        "terminal_device_fallback_without_category_count": 1,
        "terminal_geometry_host_fallback_decision_count": 1,
        "terminal_geometry_native_kernel_decision_count": 8,
        "terminal_int256_to_host_int512_fallback_count": 0,
        "terminal_int512_to_host_int1024_fallback_count": 0,
        "terminal_int1024_to_cpu_rational_fallback_count": 1,
    }.items():
        if require_int(value.get(field), field) != expected:
            fail(f"{field} must be {expected}")

    if require_int(value.get("kernel_launch_count"), "kernel_launch_count") != 3:
        fail("the fixed-width batch must launch exactly three kernels")
    if require_int(value.get("synchronization_count"), "synchronization_count") != 1:
        fail("the fixed-width batch must synchronize exactly once")
    if (
        require_int(
            value.get("internal_node_receipt_task_count"),
            "internal_node_receipt_task_count",
        )
        != 1
    ):
        fail("the fixture must contain exactly one internal-node receipt task")
    require_int(
        value.get("internal_node_receipt_leaf_count"),
        "internal_node_receipt_leaf_count",
        minimum=3,
    )
    kernel_elapsed = require_int(
        value.get("kernel_elapsed_ns"), "kernel_elapsed_ns", minimum=1
    )
    if require_int(value.get("kernel_ns"), "kernel_ns", minimum=1) != kernel_elapsed:
        fail("kernel timing aliases differ")
    for field in TIMING_KEYS - {"kernel_elapsed_ns", "kernel_ns"}:
        require_int(value.get(field), field, minimum=1)
    require_int(value.get("cuda_device"), "cuda_device")
    require_uint64(
        value.get("source_snapshot_epoch"), "source_snapshot_epoch", nonzero=True
    )
    require_uint64(
        value.get("submitted_task_digest"), "submitted_task_digest", nonzero=True
    )
    require_uint64(
        value.get("completed_result_digest"), "completed_result_digest", nonzero=True
    )

    submitted = require_int(
        value.get("stream_adapter_submitted_task_count"),
        "stream_adapter_submitted_task_count",
        minimum=1,
    )
    prefetched = require_int(
        value.get("stream_adapter_prefetched_task_count"),
        "stream_adapter_prefetched_task_count",
        minimum=1,
    )
    if prefetched != submitted:
        fail("the stream adapter did not prefetch every submitted task")
    certified_positive = require_int(
        value.get("stream_adapter_certified_positive_count"),
        "stream_adapter_certified_positive_count",
        minimum=1,
    )
    native_positive = require_int(
        value.get("stream_adapter_native_kernel_certified_positive_count"),
        "stream_adapter_native_kernel_certified_positive_count",
        minimum=1,
    )
    cpu_positive = require_int(
        value.get("stream_adapter_native_cpu_fallback_certified_positive_count"),
        "stream_adapter_native_cpu_fallback_certified_positive_count",
    )
    if native_positive + cpu_positive != certified_positive:
        fail(
            "native and exact CPU adapter positives do not partition certified positives"
        )
    support_3 = require_int(
        value.get("stream_adapter_support_3_certified_positive_count"),
        "stream_adapter_support_3_certified_positive_count",
        minimum=1,
    )
    support_4 = require_int(
        value.get("stream_adapter_support_4_certified_positive_count"),
        "stream_adapter_support_4_certified_positive_count",
        minimum=1,
    )
    if support_3 + support_4 != certified_positive:
        fail("support arities do not partition adapter positives")
    support_prune = require_int(
        value.get("stream_adapter_support_prune_certified_positive_count"),
        "stream_adapter_support_prune_certified_positive_count",
        minimum=1,
    )
    query = require_int(
        value.get("stream_adapter_query_certified_positive_count"),
        "stream_adapter_query_certified_positive_count",
        minimum=1,
    )
    if support_prune + query != certified_positive:
        fail("support-prune and query decisions do not partition adapter positives")
    terminal_decisions = require_int(
        value.get("stream_adapter_terminal_geometry_decision_count"),
        "stream_adapter_terminal_geometry_decision_count",
        minimum=1,
    )
    terminal_native = require_int(
        value.get("stream_adapter_terminal_geometry_native_kernel_decision_count"),
        "stream_adapter_terminal_geometry_native_kernel_decision_count",
        minimum=1,
    )
    component_cpu = require_int(
        value.get("stream_adapter_component_cpu_fallback_count"),
        "stream_adapter_component_cpu_fallback_count",
    )
    terminal_cpu_lower_bound = terminal_decisions - terminal_native
    if terminal_cpu_lower_bound < 0 or component_cpu < terminal_cpu_lower_bound:
        fail("adapter CPU fallback mass cannot cover its terminal decisions")
    if cpu_positive > component_cpu:
        fail("certified-positive CPU fallbacks exceed all component CPU fallbacks")
    capacity = require_int(
        value.get("stream_adapter_cache_capacity"),
        "stream_adapter_cache_capacity",
        minimum=1,
    )
    maximum_entries = require_int(
        value.get("stream_adapter_maximum_cache_entry_count"),
        "stream_adapter_maximum_cache_entry_count",
        minimum=1,
    )
    if maximum_entries > capacity:
        fail("the stream adapter exceeded its cache capacity")
    if (
        require_int(
            value.get("stream_adapter_cache_hit_count"),
            "stream_adapter_cache_hit_count",
            minimum=1,
        )
        == 0
    ):
        fail("the stream adapter cache was not exercised")
    if (
        require_int(
            value.get("stream_adapter_cache_miss_count"),
            "stream_adapter_cache_miss_count",
        )
        != 0
    ):
        fail("the prefetched stream adapter incurred a cache miss")
    evaluate_calls = require_int(
        value.get("stream_adapter_evaluate_call_count"),
        "stream_adapter_evaluate_call_count",
        minimum=1,
    )
    synchronizations = require_int(
        value.get("stream_adapter_synchronization_count"),
        "stream_adapter_synchronization_count",
        minimum=1,
    )
    if synchronizations != evaluate_calls or synchronizations >= submitted:
        fail("the adapter batching/synchronization contract is not satisfied")
    if (
        require_int(
            value.get("stream_adapter_prefetch_call_count"),
            "stream_adapter_prefetch_call_count",
            minimum=1,
        )
        == 0
    ):
        fail("the stream adapter prefetch path was not exercised")
    if (
        require_int(
            value.get("stream_adapter_maximum_batch_size"),
            "stream_adapter_maximum_batch_size",
            minimum=2,
        )
        > submitted
    ):
        fail("the maximum adapter batch exceeds submitted tasks")
    return value


def parse_single_json_line(raw: str, label: str) -> dict[str, Any]:
    lines = raw.splitlines()
    if len(lines) != 1 or not lines[0]:
        fail(f"{label} must contain exactly one non-empty JSON line")
    return parse_json_object(lines[0], label)


def extract_sanitizer_qualification(raw: str, label: str) -> dict[str, Any]:
    candidates = [line for line in raw.splitlines() if line.startswith("{")]
    if len(candidates) != 1:
        fail(f"{label} must contain exactly one qualification JSON line")
    return validate_qualification(
        parse_json_object(candidates[0], f"{label} qualification")
    )


def validate_equivalence(direct: dict[str, Any], memcheck: dict[str, Any]) -> None:
    expected = {key: value for key, value in direct.items() if key not in TIMING_KEYS}
    observed = {key: value for key, value in memcheck.items() if key not in TIMING_KEYS}
    if observed != expected:
        fail("memcheck changed a non-timing schema-6 result")


def validate_build_log(value: str, label: str) -> None:
    if not value.strip() or BUILD_FAILURE_RE.search(value) is not None:
        fail(f"{label} is empty or contains a build failure")


def parse_ptxas_resources(value: str) -> list[dict[str, Any]]:
    """Extract the three fixed-width kernel resources from the audit build."""

    matches = list(PTXAS_ENTRY_RE.finditer(value))
    resources: list[dict[str, Any]] = []
    for index, match in enumerate(matches):
        entry, architecture = match.groups()
        if KERNEL_NAME not in entry:
            continue
        section_end = (
            matches[index + 1].start() if index + 1 < len(matches) else len(value)
        )
        section = value[match.start() : section_end]
        stack_re = re.compile(PTXAS_STACK_RE_TEMPLATE.format(entry=re.escape(entry)))
        stack_match = stack_re.search(section)
        registers_match = PTXAS_REGISTERS_RE.search(section)
        if stack_match is None or registers_match is None:
            fail(f"ptxas resources are incomplete for kernel entry {entry}")
        stack, stores, loads = (int(item) for item in stack_match.groups())
        registers = int(registers_match.group(1))
        if architecture != "sm_120":
            fail("a terminal-geometry ptxas entry is not compiled for sm_120")
        if registers < 1 or registers > 255:
            fail("a terminal-geometry ptxas register count is outside hardware bounds")
        resources.append(
            {
                "architecture": architecture,
                "entry": entry,
                "registers": registers,
                "spill_load_bytes": loads,
                "spill_store_bytes": stores,
                "stack_frame_bytes": stack,
            }
        )
    if len(resources) != EXPECTED_KERNEL_ENTRY_COUNT:
        fail("audit build must expose exactly three fixed-width terminal kernels")
    if len({item["entry"] for item in resources}) != EXPECTED_KERNEL_ENTRY_COUNT:
        fail("audit build repeated a terminal kernel entry instead of three widths")
    return resources


def validate_qualification_stderr(value: str) -> None:
    if value != "":
        fail("qualification application stderr must be empty")


def validate_environment_artifact(
    path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> str:
    raw, digest = read_text_evidence(path, "Phase 3 environment artifact")
    validate_environment(
        parse_json_object(raw, "Phase 3 environment artifact"),
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    return digest


def expected_final_lifecycle(
    *,
    project: str,
    zone: str,
    instance: str,
    guest_shutdown_minutes: int,
    final_status_verified_at_utc: str,
    last_start_timestamp: str,
) -> dict[str, Any]:
    if guest_shutdown_minutes != 45:
        fail("schema-6 terminal geometry requires the short 45-minute guest guard")
    if project != ALLOWED_GCP_PROJECT:
        fail("final lifecycle project is outside the qualification allowlist")
    if (zone, instance) not in ALLOWED_GCP_TARGETS:
        fail("final lifecycle zone/instance pair is outside the qualification allowlist")
    for label, value in {
        "project": project,
        "zone": zone,
        "instance": instance,
        "last_start_timestamp": last_start_timestamp,
    }.items():
        if not value or "\n" in value or "\r" in value:
            fail(f"final lifecycle {label} is empty or non-canonical")
    try:
        parsed_last_start = datetime.fromisoformat(
            last_start_timestamp[:-1] + "+00:00"
            if last_start_timestamp.endswith("Z")
            else last_start_timestamp
        )
    except ValueError:
        fail("final lifecycle last_start_timestamp is not valid ISO-8601")
    if parsed_last_start.tzinfo is None or parsed_last_start.utcoffset() is None:
        fail("final lifecycle last_start_timestamp must include a UTC offset")
    if re.fullmatch(
        r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z",
        final_status_verified_at_utc,
    ) is None:
        fail("final lifecycle verification timestamp is not canonical UTC")
    return {
        "final_status": "TERMINATED",
        "final_status_readback": "gcloud_compute_instances_describe",
        "final_status_verified_at_utc": final_status_verified_at_utc,
        "guest_shutdown_guard_verified": True,
        "guest_shutdown_minutes": guest_shutdown_minutes,
        "initial_status": "TERMINATED",
        "initial_status_basis": "start_and_verify_precondition",
        "instance": instance,
        "last_start_timestamp": last_start_timestamp,
        "project": project,
        "start_handoff_schema": "e-hgp.start-handoff.v3",
        "stop_responsibility": "external_orchestrator",
        "targeted_stop_verified": True,
        "worker_mutates_gcp": False,
        "worker_status_before_targeted_stop": "worker_passed_pending_shutdown",
        "zone": zone,
    }


def validate_final_environment_artifact(
    path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
    lifecycle: dict[str, Any],
) -> str:
    raw, digest = read_text_evidence(path, "final Phase 3 environment artifact")
    value = parse_json_object(raw, "final Phase 3 environment artifact")
    if raw != canonical_json(value):
        fail("final Phase 3 environment artifact must be canonical JSON")
    for field, expected in {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "schema": PHASE3_SCHEMA,
        "scientific_scope": "environment_reproducibility_only",
        "status": "passed",
    }.items():
        if value.get(field) != expected:
            fail(f"final Phase 3 environment {field} differs")
    if value.get("scientific_result_claimed") is not False:
        fail("final Phase 3 environment claims a scientific result")
    if value.get("scientific_public_status") is not None or "public_status" in value:
        fail("final Phase 3 environment publishes a scientific status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("final Phase 3 environment does not bind the qualified commit")
    image = value.get("image")
    if not isinstance(image, dict) or any(
        image.get(field) != expected
        for field, expected in {
            "base_ref": base_image_ref,
            "id": image_id,
            "ref": image_ref,
        }.items()
    ):
        fail("final Phase 3 environment image identity differs")
    checks = value.get("checks")
    if not isinstance(checks, dict) or any(
        checks.get(field) != "passed"
        for field in ("cuda_audit_workflow", "cuda_release_workflow")
    ):
        fail("final Phase 3 environment workflows are not passed")
    if value.get("vm_lifecycle") != lifecycle:
        fail("final Phase 3 environment lifecycle differs")
    return digest


def expected_checks(logs: dict[str, str]) -> dict[str, Any]:
    validate_qualification_stderr(logs["qualification_stderr"])
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log")
    )
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if architectures != ["sm_120"]:
        fail("qualification binary must contain only sm_120 ELF code")
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    memcheck = extract_sanitizer_qualification(logs["memcheck"], "memcheck log")
    validate_equivalence(direct, memcheck)
    resources = parse_ptxas_resources(logs["audit_build"])
    return {
        "aot_elf_architectures": ["sm_120"],
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "component_batch_repeatable": True,
        "component_schema_version": COMPONENT_SCHEMA_VERSION,
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "memcheck": "passed",
        "memcheck_qualification": memcheck,
        "qualification": direct,
        "qualification_schema_version": QUALIFICATION_SCHEMA_VERSION,
        "qualification_stderr_empty": True,
        "ptxas_resource_entry_count": EXPECTED_KERNEL_ENTRY_COUNT,
        "ptxas_resources": resources,
        "sanitizer_non_timing_equivalence": "passed",
        "terminal_category_partition": "passed",
        "terminal_classification_native_cuda": False,
        "terminal_geometry_native_cuda": True,
        "terminal_support_arities": [3, 4],
    }


def _validate_embedded_evidence(value: dict[str, Any]) -> None:
    logs = value.get("logs")
    digests = value.get("log_sha256")
    if not isinstance(logs, dict) or not isinstance(digests, dict):
        fail("logs or log digests are absent")
    require_exact_keys(logs, LOG_KEYS, "terminal-geometry logs")
    require_exact_keys(digests, LOG_KEYS, "terminal-geometry log digests")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"log {name} must be text")
        if digests.get(name) != hashlib.sha256(log.encode("utf-8")).hexdigest():
            fail(f"log digest differs for {name}")
    if value.get("checks") != expected_checks(logs):
        fail("artifact checks differ from embedded evidence")


def validate_artifact(
    value: dict[str, Any],
    raw: str,
    *,
    git_sha: str,
    git_tree: str,
    environment_artifact_path: Path,
    final_lifecycle: dict[str, Any] | None = None,
) -> dict[str, Any]:
    require_exact_keys(value, ARTIFACT_KEYS, "terminal-geometry artifact")
    if raw != canonical_json(value):
        fail("terminal-geometry artifact must be canonical JSON")
    for field, expected in {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "component_only": True,
        "deployment_status": DEPLOYMENT_STATUS,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "schema": SCHEMA,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "passed" if final_lifecycle is not None else "worker_passed_pending_shutdown",
    }.items():
        if value.get(field) != expected:
            fail(f"artifact {field} must be {expected}")
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("phase_context") != PHASE_CONTEXT or value.get("claims") != CLAIMS:
        fail("the component published an unauthorized phase or product claim")
    if value.get("commands") != {
        "memcheck_application": QUALIFICATION_COMMAND,
        "qualification": QUALIFICATION_COMMAND,
    }:
        fail("guarded commands differ")
    if value.get("git") != {"clean": True, "sha": git_sha, "tree": git_tree}:
        fail("artifact does not bind the qualified clean commit and tree")
    image = value.get("image")
    if not isinstance(image, dict):
        fail("image identity is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    if image.get("base_ref") != BASE_IMAGE_REF or image.get("ref") != (
        f"morsehgp3d-phase3:{git_sha}"
    ):
        fail("image identity does not bind the qualified SHA")
    if (
        not isinstance(image.get("id"), str)
        or IMAGE_ID_RE.fullmatch(image["id"]) is None
    ):
        fail("image ID is not canonical")
    if final_lifecycle is None:
        environment_sha256 = validate_environment_artifact(
            environment_artifact_path,
            git_sha=git_sha,
            base_image_ref=image["base_ref"],
            image_ref=image["ref"],
            image_id=image["id"],
        )
        required_lifecycle = WORKER_LIFECYCLE
    else:
        environment_sha256 = validate_final_environment_artifact(
            environment_artifact_path,
            git_sha=git_sha,
            base_image_ref=image["base_ref"],
            image_ref=image["ref"],
            image_id=image["id"],
            lifecycle=final_lifecycle,
        )
        required_lifecycle = final_lifecycle
    if value.get("provenance") != {
        "environment_artifact_schema": PHASE3_SCHEMA,
        "environment_artifact_sha256": environment_sha256,
    }:
        fail("artifact provenance does not bind Phase 3 evidence")
    if value.get("vm_lifecycle") != required_lifecycle:
        fail("artifact lifecycle differs from its guarded publication stage")
    binary = value.get("binary")
    if not isinstance(binary, dict):
        fail("binary evidence is absent")
    require_exact_keys(
        binary,
        {
            "audit_qualification_relative_path",
            "audit_qualification_sha256",
            "ptxas_evidence_source",
            "qualification_relative_path",
            "qualification_sha256",
        },
        "binary evidence",
    )
    if binary.get("qualification_relative_path") != BINARY_RELATIVE_PATH:
        fail("qualification binary path differs")
    if binary.get("audit_qualification_relative_path") != AUDIT_BINARY_RELATIVE_PATH:
        fail("audit qualification binary path differs")
    if binary.get("ptxas_evidence_source") != "targeted_cuda_audit_build_ptxas_verbose":
        fail("ptxas evidence source differs")
    for field in ("qualification_sha256", "audit_qualification_sha256"):
        if re.fullmatch(r"[0-9a-f]{64}", str(binary.get(field))) is None:
            fail(f"{field} is not canonical")
    _validate_embedded_evidence(value)
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    git_tree: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    raw, _ = read_text_evidence(
        artifact_path, "terminal-geometry qualification artifact"
    )
    return validate_artifact(
        parse_json_object(raw, "terminal-geometry qualification artifact"),
        raw,
        git_sha=git_sha,
        git_tree=git_tree,
        environment_artifact_path=environment_artifact_path,
    )


def validate_final_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    git_tree: str,
    environment_artifact_path: Path,
    project: str,
    zone: str,
    instance: str,
    guest_shutdown_minutes: int,
    final_status_verified_at_utc: str,
    last_start_timestamp: str,
) -> dict[str, Any]:
    lifecycle = expected_final_lifecycle(
        project=project,
        zone=zone,
        instance=instance,
        guest_shutdown_minutes=guest_shutdown_minutes,
        final_status_verified_at_utc=final_status_verified_at_utc,
        last_start_timestamp=last_start_timestamp,
    )
    raw, _ = read_text_evidence(
        artifact_path, "final terminal-geometry qualification artifact"
    )
    return validate_artifact(
        parse_json_object(raw, "final terminal-geometry qualification artifact"),
        raw,
        git_sha=git_sha,
        git_tree=git_tree,
        environment_artifact_path=environment_artifact_path,
        final_lifecycle=lifecycle,
    )


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--git-tree", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--release-build-log", type=Path, required=True)
    parser.add_argument("--audit-build-log", type=Path, required=True)
    parser.add_argument("--qualification-log", type=Path, required=True)
    parser.add_argument("--qualification-stderr-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--audit-binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be a canonical lowercase commit ID")
    if SHA_RE.fullmatch(args.git_tree) is None:
        fail("--git-tree must be a canonical lowercase tree ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not pinned")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref does not bind the qualified SHA")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id is not canonical")
    environment_sha256 = validate_environment_artifact(
        args.environment_artifact,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    specs = {
        "audit_build": (args.audit_build_log, False),
        "cuobjdump_elf": (args.elf_log, False),
        "cuobjdump_ptx": (args.ptx_log, True),
        "cuobjdump_ptx_stderr": (args.ptx_stderr_log, True),
        "memcheck": (args.memcheck_log, False),
        "qualification": (args.qualification_log, False),
        "qualification_stderr": (args.qualification_stderr_log, True),
        "release_build": (args.release_build_log, False),
    }
    logs: dict[str, str] = {}
    digests: dict[str, str] = {}
    for name, (path, allow_empty) in specs.items():
        logs[name], digests[name] = read_text_evidence(
            path, f"{name} log", allow_empty=allow_empty
        )
    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "audit_qualification_relative_path": AUDIT_BINARY_RELATIVE_PATH,
            "audit_qualification_sha256": sha256_file(
                args.audit_binary, "audit qualification binary"
            ),
            "ptxas_evidence_source": "targeted_cuda_audit_build_ptxas_verbose",
            "qualification_relative_path": BINARY_RELATIVE_PATH,
            "qualification_sha256": sha256_file(args.binary, "qualification binary"),
        },
        "checks": expected_checks(logs),
        "claims": dict(CLAIMS),
        "commands": {
            "memcheck_application": list(QUALIFICATION_COMMAND),
            "qualification": list(QUALIFICATION_COMMAND),
        },
        "component_only": True,
        "deployment_status": DEPLOYMENT_STATUS,
        "git": {"clean": True, "sha": args.git_sha, "tree": args.git_tree},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "log_sha256": digests,
        "logs": logs,
        "mode": MODE,
        "phase": "15",
        "phase_context": dict(PHASE_CONTEXT),
        "profile": PROFILE,
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": environment_sha256,
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(WORKER_LIFECYCLE),
    }
    validate_artifact(
        artifact,
        canonical_json(artifact),
        git_sha=args.git_sha,
        git_tree=args.git_tree,
        environment_artifact_path=args.environment_artifact,
    )
    write_exclusive_atomic(args.output, artifact)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
